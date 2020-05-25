/*
  Slideviewer, a whole-slide image viewer for digital pathology.
  Copyright (C) 2019-2020  Pieter Valkema

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "common.h"
#include "viewer.h"
#include "annotation.h"
#include "platform.h"
#include "gui.h"
#include "yxml.h"

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"

// XML parsing using the yxml library.
// Note: what is the optimal stack buffer size for yxml?
// https://dev.yorhel.nl/yxml/man
#define YXML_STACK_BUFFER_SIZE KILOBYTES(32)



asap_xml_element_enum current_xml_element_type;
asap_xml_attribute_enum current_xml_attribute_type;

void draw_annotations(annotation_set_t* annotation_set, v2f camera_min, float screen_um_per_pixel) {
	for (i32 annotation_index = 0; annotation_index < annotation_set->annotation_count; ++annotation_index) {
		annotation_t* annotation = annotation_set->annotations + annotation_index;
		rgba_t rgba = annotation->color;
//		rgba_t rgba = {50, 50, 0, 255 };
		u32 color = TO_RGBA(rgba.r, rgba.g, rgba.b, rgba.a);
		if (annotation->has_coordinates) {
			v2f* points = (v2f*) alloca(sizeof(v2f) * annotation->coordinate_count);
			for (i32 i = 0; i < annotation->coordinate_count; ++i) {
				coordinate_t* coordinate = annotation_set->coordinates + annotation->first_coordinate + i;
				v2f world_pos = {coordinate->x, coordinate->y};
				v2f transformed_pos = world_pos_to_screen_pos(world_pos, camera_min, screen_um_per_pixel);
				points[i] = transformed_pos;
			}
			// Draw the annotation in the background list (behind UI elements), as a thick colored line
			ImDrawList* draw_list = igGetBackgroundDrawList();
			ImDrawList_AddPolyline(draw_list, (ImVec2*)points, annotation->coordinate_count, color, true, 2.0f);
		}
	}
}

u32 add_annotation_group(annotation_set_t* annotation_set, const char* name) {
	annotation_group_t new_group = {};
	strncpy(new_group.name, name, sizeof(new_group.name));
	sb_push(annotation_set->groups, new_group);
	u32 new_group_index = annotation_set->group_count;
	++annotation_set->group_count;
	return new_group_index;
}

i32 find_annotation_group(annotation_set_t* annotation_set, const char* group_name) {
	for (i32 i = 0; i < annotation_set->group_count; ++i) {
		if (strcmp(annotation_set->groups[i].name, group_name) == 0) {
			return i;
		}
	}
	return -1; // not found
}

void annotation_set_attribute(annotation_set_t* annotation_set, annotation_t* annotation, const char* attr,
                              const char* value) {
	if (strcmp(attr, "Color") == 0) {
		// TODO: parse color hex string #rrggbb
		if (strlen(value) != 7 || value[0] != '#') {
			printf("annotation_set_attribute(): Color attribute \"%s\" not in form #rrggbb\n", value);
		} else {
			rgba_t rgba = {};
			char temp[3] = {};
			temp[0] = value[1];
			temp[1] = value[2];
			rgba.r = (u8)strtoul(temp, NULL, 16);
			temp[0] = value[3];
			temp[1] = value[4];
			rgba.g = (u8)strtoul(temp, NULL, 16);
			temp[0] = value[5];
			temp[1] = value[6];
			rgba.g = (u8)strtoul(temp, NULL, 16);
			rgba.a = 255;
			annotation->color = rgba;
		}

	} else if (strcmp(attr, "Name") == 0) {
		strncpy(annotation->name, value, sizeof(annotation->name));
	} else if (strcmp(attr, "PartOfGroup") == 0) {
		i32 group_index = find_annotation_group(annotation_set, value);
		if (group_index < 0) {
			group_index = add_annotation_group(annotation_set, value); // Group not found --> create it
		}
		annotation->group_id = group_index;
	} else if (strcmp(attr, "Type") == 0) {
		annotation->type = ANNOTATION_UNKNOWN_TYPE;
		if (strcmp(value, "Rectangle") == 0) {
			annotation->type = ANNOTATION_RECTANGLE;
		} else if (strcmp(value, "Polygon") == 0) {
			annotation->type = ANNOTATION_POLYGON;
		}
	}
}

void coordinate_set_attribute(coordinate_t* coordinate, const char* attr, const char* value) {
	if (strcmp(attr, "Order") == 0) {
		coordinate->order = atoi(value);
	} else if (strcmp(attr, "X") == 0) {
		coordinate->x = atof(value) * 0.25; // TODO: address assumption
	} else if (strcmp(attr, "Y") == 0) {
		coordinate->y = atof(value) * 0.25; // TODO: address assumption
	}
}

void unload_and_reinit_annotations(annotation_set_t* annotation_set) {
	if (annotation_set->annotations) {
		sb_free(annotation_set->annotations);
	}
	if (annotation_set->coordinates) {
		sb_free(annotation_set->coordinates);
	}
	if (annotation_set->groups) {
		sb_free(annotation_set->groups);
	}
	memset(annotation_set, 0, sizeof(*annotation_set));

	// reserve annotation group 0 for the "None" category
	add_annotation_group(annotation_set, "None");
}


bool32 load_asap_xml_annotations(app_state_t* app_state, const char* filename) {
	annotation_set_t* annotation_set = &app_state->scene.annotation_set;
	unload_and_reinit_annotations(annotation_set);

	file_mem_t* file = platform_read_entire_file(filename);
	yxml_t* x = NULL;
	bool32 success = false;
	i64 start = get_clock();

	if (0) { failed:
		goto cleanup;
	}

	if (file) {
		// hack: merge memory for yxml_t struct and stack buffer
		x = malloc(sizeof(yxml_t) + YXML_STACK_BUFFER_SIZE);
		yxml_init(x, x + 1, YXML_STACK_BUFFER_SIZE);

		// parse XML byte for byte
		char attrbuf[128];
		char* attrbuf_end = attrbuf + sizeof(attrbuf);
		char* attrcur = NULL;
		char contentbuf[128];
		char* contentbuf_end = contentbuf + sizeof(contentbuf);
		char* contentcur = NULL;

		char* doc = (char*) file->data;
		for (; *doc; doc++) {
			yxml_ret_t r = yxml_parse(x, *doc);
			if (r == YXML_OK) {
				continue; // nothing worthy of note has happened -> continue
			} else if (r < 0) {
				goto failed;
			} else if (r > 0) {
				// token
				switch(r) {
					case YXML_ELEMSTART: {
						// start of an element: '<Tag ..'
//						printf("element start: %s\n", x->elem);
						contentcur = contentbuf;

						current_xml_element_type = ASAP_XML_ELEMENT_NONE;
						if (strcmp(x->elem, "Annotation") == 0) {
							annotation_t new_annotation = (annotation_t){};
							sb_push(annotation_set->annotations, new_annotation);
							++annotation_set->annotation_count;
							current_xml_element_type = ASAP_XML_ELEMENT_ANNOTATION;
						} else if (strcmp(x->elem, "Coordinate") == 0) {
							coordinate_t new_coordinate = (coordinate_t){};
							sb_push(annotation_set->coordinates, new_coordinate);
							current_xml_element_type = ASAP_XML_ELEMENT_COORDINATE;

							annotation_t* current_annotation = &sb_last(annotation_set->annotations);
							if (!current_annotation->has_coordinates) {
								current_annotation->first_coordinate = annotation_set->coordinate_count;
								current_annotation->has_coordinates = true;
							}
							current_annotation->coordinate_count++;
							++annotation_set->coordinate_count;
						}
					} break;
					case YXML_CONTENT: {
						// element content
//						printf("   element content: %s\n", x->elem);
						if (!contentcur) break;
						char* tmp = x->data;
						while (*tmp && contentbuf < contentbuf_end) {
							*(contentcur++) = *(tmp++);
						}
						if (contentcur == contentbuf_end) {
							// too long content
							printf("load_asap_xml_annotations(): encountered a too long XML element content\n");
							goto failed;
						}
						*contentcur = '\0';
					} break;
					case YXML_ELEMEND: {
						// end of an element: '.. />' or '</Tag>'
//						printf("element end: %s\n", x->elem);
						if (contentcur) {
							// NOTE: usually only whitespace (newlines and such)
//							printf("elem content: %s\n", contentbuf);
							if (strcmp(x->elem, "Annotation") == 0) {

							}

						}
					} break;
					case YXML_ATTRSTART: {
						// attribute: 'Name=..'
//						printf("attr start: %s\n", x->attr);
						attrcur = attrbuf;
					} break;
					case YXML_ATTRVAL: {
						// attribute value
//						printf("   attr val: %s\n", x->attr);
						if (!attrcur) break;
						char* tmp = x->data;
						while (*tmp && attrbuf < attrbuf_end) {
							*(attrcur++) = *(tmp++);
						}
						if (attrcur == attrbuf_end) {
							// too long attribute
							printf("load_asap_xml_annotations(): encountered a too long XML attribute\n");
							goto failed;
						}
						*attrcur = '\0';
					} break;
					case YXML_ATTREND: {
						// end of attribute '.."'
						if (attrcur) {
//							printf("attr %s = %s\n", x->attr, attrbuf);
							if (current_xml_element_type == ASAP_XML_ELEMENT_ANNOTATION) {
								annotation_set_attribute(annotation_set, &sb_last(annotation_set->annotations), x->attr, attrbuf);
							} else if (current_xml_element_type == ASAP_XML_ELEMENT_COORDINATE) {
								coordinate_set_attribute(&sb_last(annotation_set->coordinates), x->attr, attrbuf);
							}
						}
					} break;
					case YXML_PISTART:
					case YXML_PICONTENT:
					case YXML_PIEND:
						break; // processing instructions (uninteresting, skip)
					default: {
						printf("yxml_parse(): unrecognized token (%d)\n", r);
						goto failed;
					}
				}
			}
		}
	}

	float seconds_elapsed = get_seconds_elapsed(start, get_clock());
	printf("Loaded annotations in %g seconds.\n", seconds_elapsed);

	cleanup:
	if (x) free(x);
	if (file) free(file);

	return success;
}

const char* get_annotation_type_name(annotation_type_enum type) {
	const char* result = "";
	switch(type) {
		case ANNOTATION_UNKNOWN_TYPE: default: break;
		case ANNOTATION_RECTANGLE: result = "Rectangle"; break;
		case ANNOTATION_POLYGON: result = "Polygon"; break;
	}
	return result;

}

void save_asap_xml_annotations(annotation_set_t* annotation_set, const char* filename_out) {
	ASSERT(annotation_set);
	FILE* fp = fopen(filename_out, "wb");
	if (fp) {
//		const char* base_tag = "<ASAP_Annotations><Annotations>";

		fprintf(fp, "<ASAP_Annotations><Annotations>");

		for (i32 annotation_index = 0; annotation_index < annotation_set->annotation_count; ++annotation_index) {
			annotation_t* annotation = annotation_set->annotations + annotation_index;
			char color_buf[32];
			snprintf(color_buf, sizeof(color_buf), "#%02x%02x%02x", annotation->color.r, annotation->color.g, annotation->color.b);

			const char* group = "None";
			const char* type_name = get_annotation_type_name(annotation->type);

			fprintf(fp, "<Annotation Color=\"%s\" Name=\"%s\" PartOfGroup=\"%s\" Type=\"%s\">",
			        color_buf, annotation->name, group, type_name);

			if (annotation->has_coordinates) {
				fprintf(fp, "<Coordinates>");
				for (i32 coordinate_index = 0; coordinate_index < annotation->coordinate_count; ++coordinate_index) {
					coordinate_t* coordinate = annotation_set->coordinates + annotation->first_coordinate + coordinate_index;
					fprintf(fp, "<Coordinate Order=\"%d\" X=\"%g\" Y=\"%g\" />", coordinate_index, coordinate->x / 0.25, coordinate->y / 0.25);
				}
				fprintf(fp, "</Coordinates>");
			}


			fprintf(fp, "</Annotation>");
		}

		fprintf(fp, "</Annotations></ASAP_Annotations>\n");

		fclose(fp);


	}
}

