/*
  Slideviewer, a whole-slide image viewer for digital pathology.
  Copyright (C) 2019-2021  Pieter Valkema

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

#define STB_SPRINTF_IMPLEMENTATION
#include "common.h"

#include "platform.h"
//#define SHEREDOM_JSON_IMPLEMENTATION
#include "json.h"
#include "stringutils.h"

#include "coco.h"




extern app_state_t global_app_state;

static void coco_copy_parsed_string(coco_t* coco, char* dest, json_string_s* payload_string) {
	size_t len = MIN(payload_string->string_size, COCO_MAX_FIELD-1);
	memcpy(dest, payload_string->string, len);
	dest[len] = '\0';
}

static void coco_parse_info(coco_t* coco, json_object_s* info) {
	console_print_verbose("[JSON] parsing info\n");
	json_object_element_s* element = info->start;
	while (element) {
		const char* element_name = element->name->string;
		if (element->value->type == json_type_string) {
			json_string_s* payload_string = (json_string_s*) element->value->payload;
			if (strcmp(element_name, "description") == 0) {
				coco_copy_parsed_string(coco, coco->info.description, payload_string);
			} else if (strcmp(element_name, "url") == 0) {
				coco_copy_parsed_string(coco, coco->info.url, payload_string);
			} else if (strcmp(element_name, "version") == 0) {
				coco_copy_parsed_string(coco, coco->info.version, payload_string);
			} else if (strcmp(element_name, "contributor") == 0) {
				coco_copy_parsed_string(coco, coco->info.contributor, payload_string);
			} else if (strcmp(element_name, "date_created") == 0) {
				coco_copy_parsed_string(coco, coco->info.date_created, payload_string);
			}
		} else if (element->value->type == json_type_number) {
			json_number_s* payload_number = (json_number_s*) element->value->payload;
			if (strcmp(element_name, "year") == 0) {
				coco->info.year = atoi(payload_number->number);
			}
		}


		element = element->next;
	}
}

static void coco_parse_licenses(coco_t* coco, json_array_s* info) {
	console_print_verbose("[JSON] parsing licenses\n");

	json_array_element_s* array_element = info->start;
	while (array_element) {
		if (array_element->value->type == json_type_object) {
			json_object_s* license_object = (json_object_s*)array_element->value->payload;
			json_object_element_s* element = license_object->start;
			coco_license_t* license = sb_add(coco->licenses, 1);
			memset(license, 0, sizeof(*license));
			++coco->license_count;
			while (element) {
				const char* element_name = element->name->string;
				if (element->value->type == json_type_string) {
					json_string_s* payload_string = (json_string_s*) element->value->payload;
					if (strcmp(element_name, "url") == 0) {
						coco_copy_parsed_string(coco, license->url, payload_string);
					} else if (strcmp(element_name, "name") == 0) {
						coco_copy_parsed_string(coco, license->name, payload_string);
					}
				} else if (element->value->type == json_type_number) {
					json_number_s* payload_number = (json_number_s*) element->value->payload;
					if (strcmp(element_name, "id") == 0) {
						license->id = atoi(payload_number->number);
					}
				}
				element = element->next;
			}
		}

		array_element = array_element->next;
	}
}

static void coco_parse_images(coco_t* coco, json_array_s* info) {
	console_print_verbose("[JSON] parsing images\n");

	json_array_element_s* array_element = info->start;
	while (array_element) {
		if (array_element->value->type == json_type_object) {
			json_object_s* image_object = (json_object_s*)array_element->value->payload;
			json_object_element_s* element = image_object->start;
			coco_image_t* image = sb_add(coco->images, 1);
			memset(image, 0, sizeof(*image));
			++coco->image_count;
			while (element) {
				const char* element_name = element->name->string;
				if (element->value->type == json_type_string) {
					json_string_s* payload_string = (json_string_s*) element->value->payload;
					if (strcmp(element_name, "file_name") == 0) {
						coco_copy_parsed_string(coco, image->file_name, payload_string);
					} else if (strcmp(element_name, "coco_url") == 0) {
						coco_copy_parsed_string(coco, image->coco_url, payload_string);
					} else if (strcmp(element_name, "flickr_url") == 0) {
						coco_copy_parsed_string(coco, image->flickr_url, payload_string);
					} else if (strcmp(element_name, "date_captured") == 0) {
						coco_copy_parsed_string(coco, image->date_captured, payload_string);
					}
				} else if (element->value->type == json_type_number) {
					json_number_s* payload_number = (json_number_s*) element->value->payload;
					if (strcmp(element_name, "id") == 0) {
						image->id = atoi(payload_number->number);
					} else if (strcmp(element_name, "license") == 0) {
						image->license = atoi(payload_number->number);
					} else if (strcmp(element_name, "width") == 0) {
						image->width = atoi(payload_number->number);
					} else if (strcmp(element_name, "height") == 0) {
						image->height = atoi(payload_number->number);
					}
				}
				element = element->next;
			}
		}

		array_element = array_element->next;
	}
}

static void coco_parse_annotations(coco_t* coco, json_array_s* info) {
	console_print_verbose("[JSON] parsing annotations\n");

	json_array_element_s* array_element = info->start;
	while (array_element) {
		if (array_element->value->type == json_type_object) {

			json_object_s* annotation_object = (json_object_s*)array_element->value->payload;
			json_object_element_s* element = annotation_object->start;
			coco_annotation_t* annotation = sb_add(coco->annotations, 1);
			memset(annotation, 0, sizeof(*annotation));
			++coco->annotation_count;
			while (element) {
				const char* element_name = element->name->string;
				// outer array (each element is a 'segmentation' associated with the annotation
				// For now, we assume that there is only a single segmentation
				// TODO: accept multiple segmentations per annotation
				if (element->value->type == json_type_array) {
					json_array_s* payload_array = (json_array_s*) element->value->payload;
					json_array_element_s* sub_array_element = payload_array->start;
					if (strcmp(element_name, "segmentation") == 0) {
						// [[x, y, x, y, x, y, ... ]]
						i32 coordinate_count = 0;
						v2f* coordinates = NULL; // sb
						// Assuming only a single element exists...
						/*while*/if (sub_array_element) {
							if (sub_array_element->value->type == json_type_array) {
								json_array_s* coordinate_array = (json_array_s*) sub_array_element->value->payload;
								json_array_element_s* coordinate_array_element = coordinate_array->start;
								i32 number_index = 0;
								v2f new_coord = {};
								while (coordinate_array_element) {
									i32 coordinate_index = number_index / 2;
									bool is_x = (number_index % 2) == 0; // X and Y coordinates are interleaved
									float number = 0.0;
									if (coordinate_array_element->value->type == json_type_number) {
										json_number_s* payload_number = (json_number_s*) coordinate_array_element->value->payload;
										number = strtof(payload_number->number, NULL);
									}
									if (is_x) {
										new_coord.x = number;
									} else {
										new_coord.y = number;
										sb_push(coordinates, new_coord);
										++coordinate_count;
										new_coord = (v2f){}; // reset for next iteration
									}
									coordinate_array_element = coordinate_array_element->next;
									++number_index;
								}
							}
							sub_array_element = sub_array_element->next;
						}
						annotation->segmentation.coordinates = coordinates;
						annotation->segmentation.coordinate_count = coordinate_count;
					} else if (strcmp(element_name, "bbox") == 0) {
						i32 coordinate_index = 0;
						float coordinates[4] = {};
						while (sub_array_element && coordinate_index < 4) {
							if (sub_array_element->value->type == json_type_number) {
								json_number_s* payload_number = (json_number_s*) element->value->payload;
								coordinates[coordinate_index] = strtof(payload_number->number, NULL);
								++coordinate_index;
							}
							sub_array_element = sub_array_element->next;
						}
						annotation->bbox = *((rect2f*)coordinates);
					}
				} else if (element->value->type == json_type_number) {
					json_number_s* payload_number = (json_number_s*) element->value->payload;
					if (strcmp(element_name, "id") == 0) {
						annotation->id = atoi(payload_number->number);
					} else if (strcmp(element_name, "category_id") == 0) {
						annotation->category_id = atoi(payload_number->number);
					} else if (strcmp(element_name, "image_id") == 0) {
						annotation->image_id = atoi(payload_number->number);
					} else if (strcmp(element_name, "area") == 0) {
						annotation->area = strtof(payload_number->number, NULL);
					}
				}
				element = element->next;
			}
		}

		array_element = array_element->next;
	}
}

static void coco_parse_categories(coco_t* coco, json_array_s* info) {
	console_print_verbose("[JSON] parsing categories\n");

	json_array_element_s* array_element = info->start;
	while (array_element) {
		if (array_element->value->type == json_type_object) {
			json_object_s* category_object = (json_object_s*)array_element->value->payload;
			json_object_element_s* element = category_object->start;
			coco_category_t* category = sb_add(coco->categories, 1);
			memset(category, 0, sizeof(*category));
			++coco->category_count;
			while (element) {
				const char* element_name = element->name->string;
				if (element->value->type == json_type_string) {
					json_string_s* payload_string = (json_string_s*) element->value->payload;
					if (strcmp(element_name, "supercategory") == 0) {
						coco_copy_parsed_string(coco, category->supercategory, payload_string);
					} else if (strcmp(element_name, "name") == 0) {
						coco_copy_parsed_string(coco, category->name, payload_string);
					}
				} else if (element->value->type == json_type_number) {
					json_number_s* payload_number = (json_number_s*) element->value->payload;
					if (strcmp(element_name, "id") == 0) {
						category->id = atoi(payload_number->number);
					}
				}
				element = element->next;
			}
		}

		array_element = array_element->next;
	}
}

bool open_coco(coco_t* coco, const char* json_source, size_t json_length) {
	bool success = false;

//	message_box(&global_app_state, "Trying to parse the JSON now");
	i64 timer_begin = get_clock();

	coco->original_filesize = json_length;

	if (json_source) {
		// NOTE: this may take a LONG time and use a LOT of memory, depending on the size of the JSON file.
		// TODO: execute on worker thread
		json_value_s* root = json_parse(json_source, json_length);
		if (root) {
			if (root->type != json_type_object) {
				// failed
			} else {
				json_object_s* object = (json_object_s*)root->payload;
				console_print_verbose("[JSON] Root object has length %d\n", object->length);
				ASSERT(object->length >= 1);
				json_object_element_s* element = object->start;
				while (element) {
					const char* element_name = element->name->string;
					json_object_s* payload_object = (json_object_s*)element->value->payload;
					json_array_s* payload_array = (json_array_s*)element->value->payload;

					if (element->value->type == json_type_object) {
						if (strcmp(element_name, "info") == 0) {
							coco_parse_info(coco, payload_object);
						}
					} else if (element->value->type == json_type_array) {
						if (strcmp(element_name, "licenses") == 0) {
							coco_parse_licenses(coco, payload_array);
						} else if (strcmp(element_name, "images") == 0) {
							coco_parse_images(coco, payload_array);
						} else if (strcmp(element_name, "annotations") == 0) {
							coco_parse_annotations(coco, payload_array);
						} else if (strcmp(element_name, "categories") == 0) {
							coco_parse_categories(coco, payload_array);
						}
					}
					element = element->next;
				}




				console_print("Loaded COCO JSON in %g seconds\n", get_seconds_elapsed(timer_begin, get_clock()));
//				message_box(&global_app_state, "Finished parsing JSON");

			}
			free(root);
		} else {
			console_print_error("JSON parse error\n");
		}

	}
	return true;
}

bool load_coco_from_file(coco_t* coco, const char* json_filename) {
	bool success = false;
	mem_t* coco_file = platform_read_entire_file(json_filename);

	if (coco_file) {
		success = open_coco(coco, (char*) coco_file->data, coco_file->len);
		free(coco_file);
	}
	return success;
}

static void coco_output_info(coco_t* coco, memrw_t* out) {
	char buf[4096];
	i32 len = snprintf(buf, sizeof(buf), "\"info\": {\"description\": \"%s\","
		        "\"url\": \"%s\","
				"\"version\": \"%s\","
                "\"year\": %d,"
                "\"contributor\": \"%s\","
                "\"date_created\": \"%s\"}", coco->info.description, coco->info.url, coco->info.version,
                                             coco->info.year, coco->info.contributor, coco->info.date_created);
	if (len > 0 && len < sizeof(buf)) {
		memrw_write(buf, out, len);
	}
}

static void coco_output_license(coco_license_t* license, memrw_t* out) {
	char buf[4096];
	i32 len = snprintf(buf, sizeof(buf), "{\"url\": \"%s\","
	                                     "\"id\": %d,"
	                                     "\"name\": \"%s\"}", license->url, license->id, license->name);
	if (len > 0 && len < sizeof(buf)) {
		memrw_write(buf, out, len);
	} else panic();
}

static void coco_output_licenses(coco_t* coco, memrw_t* out) {
	const char* licenses_before = "\"licenses\": [";
	memrw_write(licenses_before, out, strlen(licenses_before));
	i32 last_license_index = coco->license_count - 1;
	for (i32 license_index = 0; license_index < coco->license_count; ++license_index) {
		coco_license_t* license = coco->licenses + license_index;
		coco_output_license(license, out);
		if (license_index < last_license_index) {
			memrw_write(",\n", out, 2);
		}
	}
	memrw_write("]", out, 1);
}

static void coco_output_image(coco_image_t* image, memrw_t* out) {
	char buf[4096];
	i32 len = snprintf(buf, sizeof(buf), "{\"license\": %d,"
                 "\"file_name\": \"%s\","
                 "\"coco_url\": \"%s\","
                 "\"height\": %d,"
                 "\"width\": %d,"
                 "\"date_captured\": \"%s\","
                 "\"flickr_url\": \"%s\","
                 "\"id\": %d}", image->license, image->file_name, image->coco_url, image->height,
                                image->width, image->date_captured, image->flickr_url, image->id);
	if (len > 0 && len < sizeof(buf)) {
		memrw_write(buf, out, len);
	} else panic();
}

static void coco_output_images(coco_t* coco, memrw_t* out) {
	const char* images_before = "\"images\": [";
	memrw_write(images_before, out, strlen(images_before));
	i32 last_image_index = coco->image_count - 1;
	for (i32 image_index = 0; image_index < coco->image_count; ++image_index) {
		coco_image_t* image = coco->images + image_index;
		coco_output_image(image, out);
		if (image_index < last_image_index) {
			memrw_write(",\n", out, 2);
		}
	}
	memrw_write("]", out, 1);
}

static void coco_output_coordinate_in_array_with_fmt(const char* fmt, v2f coordinate, memrw_t* out) {
	char buf[64];
	i32 len = snprintf(buf, sizeof(buf), fmt, coordinate.x, coordinate.y);
	ASSERT(len > 0 && len < sizeof(buf));
	memrw_write(buf, out, len);
}

static void coco_output_segmentation(coco_segmentation_t* segmentation, memrw_t* out) {
	memrw_write("[", out, 1);
	i32 coordinate_count_minus_one = segmentation->coordinate_count - 1;
	for (i32 i = 0; i < coordinate_count_minus_one; ++i) {
		coco_output_coordinate_in_array_with_fmt("%g,%g,", segmentation->coordinates[i], out);
	}
	i32 last_coordinate_index = coordinate_count_minus_one;
	if (last_coordinate_index >= 0) {
		coco_output_coordinate_in_array_with_fmt("%g,%g", segmentation->coordinates[last_coordinate_index], out);
	}
	memrw_write("]", out, 1);
}

static void coco_output_annotation(coco_annotation_t* annotation, memrw_t* out) {
	char buf[4096];
	// Part 1: everything before the segmentation field
	i32 len = snprintf(buf, sizeof(buf), "{\"id\":%d,"
	                                     "\"category_id\":%d,"
	                                     "\"iscrowd\":0,"
									     "\"segmentation\":[", annotation->id, annotation->category_id);
	if (len > 0 && len < sizeof(buf)) {
		memrw_write(buf, out, len);
	} else panic();
	// Part 2: the segmentation field
#if 0
	i32 segmentation_count = 1;
	for (i32 i = 0; i < segmentation_count; ++i) {

	}
#endif
	coco_segmentation_t* segmentation = &annotation->segmentation;
	coco_output_segmentation(segmentation, out);

	// Part 3: everything after the segmentation field
	len = snprintf(buf, sizeof(buf), "],\"image_id\":%d,"
	                                 "\"area\":%g,"
	                                 "\"bbox\":[%g,%g,%g,%g]}", annotation->image_id, annotation->area,
	                                    annotation->bbox.x, annotation->bbox.y, annotation->bbox.w, annotation->bbox.h);
	if (len > 0 && len < sizeof(buf)) {
		memrw_write(buf, out, len);
	} else panic();


}

static void coco_output_annotations(coco_t* coco, memrw_t* out) {
	const char* annotations_before = "\"annotations\": [";
	memrw_write(annotations_before, out, strlen(annotations_before));
	i32 last_annotation_index = coco->annotation_count - 1;
	for (i32 annotation_index = 0; annotation_index < coco->annotation_count; ++annotation_index) {
		coco_annotation_t* annotation = coco->annotations + annotation_index;
		coco_output_annotation(annotation, out);
		if (annotation_index < last_annotation_index) {
			memrw_write(",\n", out, 2);
		}
	}
	memrw_write("]", out, 1);
}

static void coco_output_category(coco_category_t* category, memrw_t* out) {
	char buf[4096];
	i32 len = snprintf(buf, sizeof(buf), "{\"supercategory\":\"%s\","
	                                     "\"id\":%d,"
	                                     "\"name\":\"%s\"}", category->supercategory, category->id, category->name);
	if (len > 0 && len < sizeof(buf)) {
		memrw_write(buf, out, len);
	} else panic();
}

static void coco_output_categories(coco_t* coco, memrw_t* out) {
	const char* categories_before = "\"categories\":[";
	memrw_write(categories_before, out, strlen(categories_before));
	i32 last_category_index = coco->category_count - 1;
	for (i32 category_index = 0; category_index < coco->category_count; ++category_index) {
		coco_category_t* category = coco->categories + category_index;
		coco_output_category(category, out);
		if (category_index < last_category_index) {
			memrw_write(",\n", out, 2);
		}
	}
	memrw_write("]", out, 1);
}

void save_coco(coco_t* coco) {
	i64 timer_begin = get_clock();

	memrw_t out = memrw_create(MAX(MEGABYTES(1), next_pow2(coco->original_filesize)));

	memrw_write("{\n", &out, 2);
	coco_output_info(coco, &out);
	memrw_write(",\n", &out, 2);
	coco_output_licenses(coco, &out);
	memrw_write(",\n", &out, 2);
	coco_output_images(coco, &out);
	memrw_write(",\n", &out, 2);
	coco_output_annotations(coco, &out);
	memrw_write(",\n", &out, 2);
	coco_output_categories(coco, &out);
	memrw_write("}\n", &out, 2);

	FILE* fp = fopen("coco_test_out.json", "wb");
	if (fp) {
		fwrite(out.data, out.used_size, 1, fp);
		fclose(fp);
	}

	console_print("JSON saved to file in %g seconds\n", get_seconds_elapsed(timer_begin, get_clock()));
}

