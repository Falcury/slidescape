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

#include "common.h"
#include "platform.h"

#include "isyntax.h"

#include "yxml.h"

#include "jpeg_decoder.h"
#include "stb_image.h"

// TODO: Record relevant metadata in the isyntax_t structure
// --> used in the XML header for barcode, label/macro JPEG data, image block header structure, ICC profiles
// TODO: Figure out codeblocks packaging scheme and decompression
// TODO: Identify spatial location of codeblocks
// TODO: Inverse discrete wavelet transform
// TODO: Colorspace post-processing (convert YCoCg to RGB)
// TODO: Add ICC profiles support

// Base64 decoder by Jouni Malinen, original:
// http://web.mit.edu/freebsd/head/contrib/wpa/src/utils/base64.c
// Performance comparison of base64 encoders/decoders:
// https://github.com/gaspardpetit/base64/

/*
 * Base64 encoding/decoding (RFC1341)
 * Copyright (c) 2005-2011, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */
static const unsigned char base64_table[65] =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

unsigned char * base64_decode(const unsigned char *src, size_t len,
                              size_t *out_len)
{
	unsigned char dtable[256], *out, *pos, block[4], tmp;
	size_t i, count, olen;
	int pad = 0;

	memset(dtable, 0x80, 256);
	for (i = 0; i < sizeof(base64_table) - 1; i++)
		dtable[base64_table[i]] = (unsigned char) i;
	dtable['='] = 0;

	count = 0;
	for (i = 0; i < len; i++) {
		if (dtable[src[i]] != 0x80)
			count++;
	}

	if (count == 0 || count % 4)
		return NULL;

	olen = count / 4 * 3;
	pos = out = malloc(olen);
	if (out == NULL)
		return NULL;

	count = 0;
	for (i = 0; i < len; i++) {
		tmp = dtable[src[i]];
		if (tmp == 0x80)
			continue;

		if (src[i] == '=')
			pad++;
		block[count] = tmp;
		count++;
		if (count == 4) {
			*pos++ = (block[0] << 2) | (block[1] >> 4);
			*pos++ = (block[1] << 4) | (block[2] >> 2);
			*pos++ = (block[2] << 6) | block[3];
			count = 0;
			if (pad) {
				if (pad == 1)
					pos--;
				else if (pad == 2)
					pos -= 2;
				else {
					/* Invalid padding */
					free(out);
					return NULL;
				}
				break;
			}
		}
	}

	*out_len = pos - out;
	return out;
}

// similar to atoi(), but also returning the string position so we can chain calls one after another.
static const char* atoi_and_advance(const char* str, i32* dest) {
	i32 num = 0;
	bool neg = false;
	while (isspace(*str)) ++str;
	if (*str == '-') {
		neg = true;
		++str;
	}
	while (isdigit(*str)) {
		num = 10*num + (*str - '0');
		++str;
	}
	if (neg) num = -num;
	*dest = num;
	return str;
}

static void parse_three_integers(const char* str, i32* first, i32* second, i32* third) {
	str = atoi_and_advance(str, first);
	str = atoi_and_advance(str, second);
	atoi_and_advance(str, third);
}

void isyntax_decode_base64_embedded_jpeg_file(isyntax_t* isyntax) {
	// stub
}

void isyntax_parse_ufsimport_child_node(isyntax_t* isyntax, u32 group, u32 element, char* value, u64 value_len) {

	switch(group) {
		default: {
			// unknown group
			console_print_verbose("Unknown group 0x%04x\n", group);
		} break;
		case 0x0008: {
			switch(element) {
				default: {
					console_print_verbose("Unknown element (0x%04x, 0x%04x)\n", group, element);
				} break;
				case 0x002A: /*DICOM_ACQUISITION_DATETIME*/     {} break; // "20210101103030.000000"
				case 0x0070: /*DICOM_MANUFACTURER*/             {} break; // "PHILIPS"
				case 0x1090: /*DICOM_MANUFACTURERS_MODEL_NAME*/ {} break; // "UFS Scanner"
			}
		}; break;
		case 0x0018: {
			switch(element) {
				default: {
					console_print_verbose("Unknown element (0x%04x, 0x%04x)\n", group, element);
				} break;
				case 0x1000: /*DICOM_DEVICE_SERIAL_NUMBER*/     {} break; // "FMT<4-digit number>"
				case 0x1020: /*DICOM_SOFTWARE_VERSIONS*/        {} break; // "<versionnumber>" "<versionnumber>"
				case 0x1200: /*DICOM_DATE_OF_LAST_CALIBRATION*/ {} break; // "20210101"
				case 0x1201: /*DICOM_TIME_OF_LAST_CALIBRATION*/ {} break; // "100730"
			}
		} break;
		case 0x101D: {
			switch(element) {
				default: {
					console_print_verbose("Unknown element (0x%04x, 0x%04x)\n", group, element);
				} break;
				case 0x1007: /*PIIM_DP_SCANNER_RACK_NUMBER*/        {} break; // "[1..15]"
				case 0x1008: /*PIIM_DP_SCANNER_SLOT_NUMBER*/        {} break; // "[1..15]"
				case 0x1009: /*PIIM_DP_SCANNER_OPERATOR_ID*/        {} break; // "<Operator ID>"
				case 0x100A: /*PIIM_DP_SCANNER_CALIBRATION_STATUS*/ {} break; // "OK" or "NOT OK"

			}
		} break;
		case 0x301D: {
			switch(element) {
				default: {
					console_print_verbose("Unknown element (0x%04x, 0x%04x)\n", group, element);
				} break;
				case 0x1001: /*PIM_DP_UFS_INTERFACE_VERSION*/          {} break; // "5.0"
				case 0x1002: /*PIM_DP_UFS_BARCODE*/                    {} break; // "<base64-encoded barcode value>"
				case 0x1003: /*PIM_DP_SCANNED_IMAGES*/                    {

				} break;
				case 0x1010: /*PIM_DP_SCANNER_RACK_PRIORITY*/               {} break; // "<u16>"

			}
		} break;
	}
}

void isyntax_parse_scannedimage_child_node(isyntax_t* isyntax, u32 group, u32 element, char* value, u64 value_len) {

	// Parse metadata belong to one of the images in the file (either a WSI, LABELIMAGE or MACROIMAGE)

	isyntax_image_t* image = isyntax->parser.current_image;
	if (!image) {
		image = isyntax->parser.current_image = &isyntax->images[0];
	}

	switch(group) {
		default: {
			// unknown group
			console_print_verbose("Unknown group 0x%04x\n", group);
		} break;
		case 0x0008: {
			switch(element) {
				default: {
					console_print_verbose("Unknown element (0x%04x, 0x%04x)\n", group, element);
				} break;
				case 0x2111: /*DICOM_DERIVATION_DESCRIPTION*/   {         // "PHILIPS UFS V%s | Quality=%d | DWT=%d | Compressor=%d"

				} break;
			}
		}; break;
		case 0x0028: {
			switch(element) {
				default: {
					console_print_verbose("Unknown element (0x%04x, 0x%04x)\n", group, element);
				} break;
				case 0x0002: /*DICOM_SAMPLES_PER_PIXEL*/     {} break;
				case 0x0100: /*DICOM_BITS_ALLOCATED*/     {} break;
				case 0x0101: /*DICOM_BITS_STORED*/     {} break;
				case 0x0102: /*DICOM_HIGH_BIT*/     {} break;
				case 0x0103: /*DICOM_PIXEL_REPRESENTATION*/     {} break;
				case 0x2000: /*DICOM_ICCPROFILE*/     {} break;
				case 0x2110: /*DICOM_LOSSY_IMAGE_COMPRESSION*/     {} break;
				case 0x2112: /*DICOM_LOSSY_IMAGE_COMPRESSION_RATIO*/     {} break;
				case 0x2114: /*DICOM_LOSSY_IMAGE_COMPRESSION_METHOD*/     {} break; // "PHILIPS_DP_1_0"
			}
		} break;
		case 0x301D: {
			switch(element) {
				default: {
					console_print_verbose("Unknown element (0x%04x, 0x%04x)\n", group, element);
				} break;
				case 0x1004: /*PIM_DP_IMAGE_TYPE*/                     {         // "MACROIMAGE" or "LABELIMAGE" or "WSI"
					if ((strcmp(value, "MACROIMAGE") == 0)) {
						isyntax->macro_image = isyntax->parser.current_image;
						isyntax->parser.current_image_type = ISYNTAX_IMAGE_TYPE_MACROIMAGE;
					} else if ((strcmp(value, "LABELIMAGE") == 0)) {
						isyntax->label_image = isyntax->parser.current_image;
						isyntax->parser.current_image_type = ISYNTAX_IMAGE_TYPE_LABELIMAGE;
					} else if ((strcmp(value, "WSI") == 0)) {
						isyntax->wsi_image = isyntax->parser.current_image;
						isyntax->parser.current_image_type = ISYNTAX_IMAGE_TYPE_WSI;
					}
				} break;
				case 0x1005: { /*PIM_DP_IMAGE_DATA*/
					size_t decoded_capacity = value_len;
					size_t decoded_len = 0;
					i32 last_char = value[value_len-1];
					if (last_char == '/') {
						value_len--; // The last character may cause the base64 decoding to fail if invalid
					}
					u8* decoded = base64_decode((u8*)value, value_len, &decoded_len);
					if (decoded) {
						i32 channels_in_file = 0;
#if 0
						// TODO: Why does this crash?
						image->pixels = jpeg_decode_image(decoded, decoded_len, &image->width, &image->height, &channels_in_file);
#else
						// stb_image.h
						image->pixels = stbi_load_from_memory(decoded, decoded_len, &image->width, &image->height, &channels_in_file, 4);
#endif
						free(decoded);
						// TODO: actually display the image
						DUMMY_STATEMENT;
					}
				} break;
				case 0x1013: /*DP_COLOR_MANAGEMENT*/                        {} break;
				case 0x1014: /*DP_IMAGE_POST_PROCESSING*/                   {} break;
				case 0x1015: /*DP_SHARPNESS_GAIN_RGB24*/                    {} break;
				case 0x1016: /*DP_CLAHE_CLIP_LIMIT_Y16*/                    {} break;
				case 0x1017: /*DP_CLAHE_NR_BINS_Y16*/                       {} break;
				case 0x1018: /*DP_CLAHE_CONTEXT_DIMENSION_Y16*/             {} break;
				case 0x1019: /*DP_WAVELET_QUANTIZER_SETTINGS_PER_COLOR*/    {} break;
				case 0x101A: /*DP_WAVELET_QUANTIZER_SETTINGS_PER_LEVEL*/    {} break;
				case 0x101B: /*DP_WAVELET_QUANTIZER*/                       {} break;
				case 0x101C: /*DP_WAVELET_DEADZONE*/                        {} break;
				case 0x2000: /*UFS_IMAGE_GENERAL_HEADERS*/                  {
					isyntax->parser.image_header_parsing_mode = UFS_IMAGE_GENERAL_HEADERS;
				} break;
				case 0x2001: /*UFS_IMAGE_NUMBER_OF_BLOCKS*/                 {} break;
				case 0x2002: /*UFS_IMAGE_DIMENSIONS_OVER_BLOCK*/            {} break;
				case 0x2003: /*UFS_IMAGE_DIMENSIONS*/                       {} break;
				case 0x2004: /*UFS_IMAGE_DIMENSION_NAME*/                   {} break;
				case 0x2005: /*UFS_IMAGE_DIMENSION_TYPE*/                   {} break;
				case 0x2006: /*UFS_IMAGE_DIMENSION_UNIT*/                   {} break;
				case 0x2007: /*UFS_IMAGE_DIMENSION_SCALE_FACTOR*/           {} break;
				case 0x2008: /*UFS_IMAGE_DIMENSION_DISCRETE_VALUES_STRING*/ {} break;
				case 0x2009: /*UFS_IMAGE_BLOCK_HEADER_TEMPLATES*/           {
					isyntax->parser.image_header_parsing_mode = UFS_IMAGE_BLOCK_HEADER_TEMPLATES;
				} break;
				case 0x200A: /*UFS_IMAGE_DIMENSION_RANGES*/                 {

				} break;
				case 0x200B: /*UFS_IMAGE_DIMENSION_RANGE*/                  {
					isyntax_image_dimension_range_t range = {};
					parse_three_integers(value, &range.start, &range.step, &range.end);
					range.range = (range.end + range.step) - range.start;
					if (isyntax->parser.image_header_parsing_mode == UFS_IMAGE_BLOCK_HEADER_TEMPLATES) {

					} else if (isyntax->parser.image_header_parsing_mode == UFS_IMAGE_GENERAL_HEADERS) {

					}
					DUMMY_STATEMENT;
				} break;
				case 0x200C: /*UFS_IMAGE_DIMENSION_IN_BLOCK*/               {} break;
				case 0x200F: /*UFS_IMAGE_BLOCK_COMPRESSION_METHOD*/         {} break;
				case 0x2013: /*UFS_IMAGE_PIXEL_TRANSFORMATION_METHOD*/      {} break;
				case 0x2014: { /*UFS_IMAGE_BLOCK_HEADER_TABLE*/
					size_t decoded_capacity = value_len;
					size_t decoded_len = 0;
					i32 last_char = value[value_len-1];
#if 0
					FILE* test_out = fopen("test_b64.out", "wb");
					fwrite(value, value_len, 1, test_out);
					fclose(test_out);
#endif
					if (last_char == '/') {
						value_len--; // The last character may cause the base64 decoding to fail if invalid
						last_char = value[value_len-1];
					}
					while (last_char == '\n' || last_char == '\r' || last_char == ' ') {
						--value_len;
						last_char = value[value_len-1];
					}
					u8* decoded = base64_decode((u8*)value, value_len, &decoded_len);
					if (decoded) {
						image->block_header_table = decoded;
						image->block_header_size = decoded_len;

						u32 header_size = *(u32*) decoded + 0;
						u8* block_header_start = decoded + 4;
						dicom_tag_header_t sequence_element = *(dicom_tag_header_t*) (block_header_start);
						if (sequence_element.size == 40) {
							// We have a partial header structure, with 'Block Data Offset' and 'Block Size' missing (stored in Seektable)
							// Full block header size (including the sequence element) is 48 bytes
							u32 block_count = header_size / 48;
							u32 should_be_zero = header_size % 48;
							if (should_be_zero != 0) {
								// TODO: handle error condition
								DUMMY_STATEMENT;
							}

							image->codeblock_count = block_count;
							image->codeblocks = calloc(1, block_count * sizeof(isyntax_codeblock_t));
							image->header_codeblocks_are_partial = true;

							for (i32 i = 0; i < block_count; ++i) {
								isyntax_partial_block_header_t* header = ((isyntax_partial_block_header_t*)(block_header_start)) + i;
								isyntax_codeblock_t* codeblock = image->codeblocks + i;
								codeblock->x_coordinate = header->x_coordinate;
								codeblock->y_coordinate = header->y_coordinate;
								codeblock->color_component = header->color_component;
								codeblock->scale = header->scale;
								codeblock->coefficient = header->coefficient;
								codeblock->block_header_template_id = header->block_header_template_id;
								DUMMY_STATEMENT;
							}

						} else if (sequence_element.size == 72) {
							// We have the complete header structure. (Nothing stored in Seektable)
							u32 block_count = header_size / 80;
							u32 should_be_zero = header_size % 80;
							if (should_be_zero != 0) {
								// TODO: handle error condition
								DUMMY_STATEMENT;
							}

							image->codeblock_count = block_count;
							image->codeblocks = calloc(1, block_count * sizeof(isyntax_codeblock_t));
							image->header_codeblocks_are_partial = false;

							for (i32 i = 0; i < block_count; ++i) {
								isyntax_full_block_header_t* header = ((isyntax_full_block_header_t*)(block_header_start)) + i;
								isyntax_codeblock_t* codeblock = image->codeblocks + i;
								codeblock->x_coordinate = header->x_coordinate;
								codeblock->y_coordinate = header->y_coordinate;
								codeblock->color_component = header->color_component;
								codeblock->scale = header->scale;
								codeblock->coefficient = header->coefficient;
								codeblock->block_data_offset = header->block_data_offset; // extra
								codeblock->block_size = header->block_size; // extra
								codeblock->block_header_template_id = header->block_header_template_id;
								DUMMY_STATEMENT;
							}
						} else {
							// TODO: handle error condition
						}

						free(decoded);
					} else {
						//TODO: handle error condition
					}
				} break;

			}
		} break;
	}
}

bool isyntax_validate_dicom_attr(const char* expected, const char* observed) {
	bool ok = (strcmp(expected, observed) == 0);
	if (!ok) {
		console_print("iSyntax validation error: while reading DICOM metadata, expected '%s' but found '%s'\n", expected, observed);
	}
	return ok;
}

void isyntax_parser_init(isyntax_t* isyntax) {
	isyntax_parser_t* parser = &isyntax->parser;

	parser->initialized = true;

	parser->attrbuf_capacity = KILOBYTES(32);
	parser->contentbuf_capacity = MEGABYTES(8);

	parser->current_element_name = "";
	parser->attrbuf = malloc(parser->attrbuf_capacity); // TODO: free
	parser->attrbuf_end = parser->attrbuf + parser->attrbuf_capacity;
	parser->attrcur = NULL;
	parser->attrlen = 0;
	parser->contentbuf = malloc(parser->contentbuf_capacity); // TODO: free
	parser->contentcur = NULL;
	parser->contentlen = 0;

	parser->current_dicom_attribute_name[0] = '\0';
	parser->current_dicom_group_tag = 0;
	parser->current_dicom_element_tag = 0;
	parser->attribute_index = 0;
	parser->current_node_type = ISYNTAX_NODE_NONE;

	// XML parsing using the yxml library.
	// https://dev.yorhel.nl/yxml/man
	size_t yxml_stack_buffer_size = KILOBYTES(32);
	parser->x = (yxml_t*) malloc(sizeof(yxml_t) + yxml_stack_buffer_size);

	yxml_init(parser->x, parser->x + 1, yxml_stack_buffer_size);
}

const char* get_spaces(i32 length) {
	ASSERT(length >= 0);
	static const char spaces[] = "                                  ";
	i32 spaces_len = COUNT(spaces) - 1;
	i32 offset_from_end = MIN(spaces_len, length);
	i32 offset = spaces_len - offset_from_end;
	return spaces + offset;
}

void push_to_buffer_maybe_grow(u8** restrict dest, size_t* restrict dest_len, size_t* restrict dest_capacity, void* restrict src, size_t src_len) {
	ASSERT(dest && dest_len && dest_capacity && src);
	size_t old_len = *dest_len;
	size_t new_len = old_len + src_len;
	size_t capacity = *dest_capacity;
	if (new_len > capacity) {
		capacity = next_pow2(new_len);
		u8* new_ptr = (u8*)realloc(*dest, capacity);
		if (!new_ptr) panic();
		*dest = new_ptr;
		*dest_capacity = capacity;
	}
	memcpy(*dest + old_len, src, src_len);
	*dest_len = new_len;
}

bool isyntax_parse_xml_header(isyntax_t* isyntax, char* xml_header, i64 chunk_length, bool is_last_chunk) {

	yxml_t* x = NULL;
	bool success = false;

	static bool paranoid_mode = true;

	isyntax_parser_t* parser = &isyntax->parser;

	if (!parser->initialized) {
		isyntax_parser_init(isyntax);
	}
	x = parser->x;

	if (0) { failed: cleanup:
		if (x) {
			free(x);
			parser->x = NULL;
		}
		if (parser->attrbuf) {
			free(parser->attrbuf);
			parser->attrbuf = NULL;
		}
		if (parser->contentbuf) {
			free(parser->contentbuf);
			parser->contentbuf = NULL;
		}
		return success;
	}



	// parse XML byte for byte

	char* doc = xml_header;
	for (i64 remaining_length = chunk_length; remaining_length > 0; --remaining_length, ++doc) {
		int c = *doc;
		if (c == '\0') {
			ASSERT(false); // this should never trigger
			break;
		}
		yxml_ret_t r = yxml_parse(x, c);
		if (r == YXML_OK) {
			continue; // nothing worthy of note has happened -> continue
		} else if (r < 0) {
			goto failed;
		} else if (r > 0) {
			// token
			switch(r) {
				case YXML_ELEMSTART: {
					// start of an element: '<Tag ..'
					isyntax_parser_node_t* parent_node = parser->node_stack + parser->node_stack_index;
					++parser->node_stack_index;
					isyntax_parser_node_t* node = parser->node_stack + parser->node_stack_index;
					memset(node, 0, sizeof(isyntax_parser_node_t));

					parser->contentcur = parser->contentbuf;
					*parser->contentcur = '\0';
					parser->contentlen = 0;
					parser->attribute_index = 0;
					if (strcmp(x->elem, "Attribute") == 0) {
						node->node_type = ISYNTAX_NODE_LEAF;
					} else if (strcmp(x->elem, "DataObject") == 0) {
						node->node_type = ISYNTAX_NODE_BRANCH;
						node->group = parent_node->group;
						node->element = parent_node->element;
					} else if (strcmp(x->elem, "Array") == 0) {
						node->node_type = ISYNTAX_NODE_ARRAY;
						console_print_verbose("%sArray\n", get_spaces(parser->node_stack_index));
						// Inherit group and element of parent node (Array doesn't have any; we want to pass on the information to child nodes)
						node->group = parent_node->group;
						node->element = parent_node->element;
					} else {
						node->node_type = ISYNTAX_NODE_NONE;
						console_print_verbose("%selement start: %s\n", get_spaces(parser->node_stack_index), x->elem);
					}
					parser->current_node_type = node->node_type;
					parser->current_node_has_children = false;
					parser->current_element_name = x->elem; // We need to remember this pointer, because it may point to something else at the YXML_ELEMEND state

				} break;
				case YXML_CONTENT: {
					// element content
					if (!parser->contentcur) break;

					// Load iSyntax block header table (and other large XML tags) greedily and bypass yxml parsing overhead
					if (parser->current_node_type == ISYNTAX_NODE_LEAF) {
						u32 group = parser->current_dicom_group_tag;
						u32 element = parser->current_dicom_element_tag;
						isyntax_parser_node_t* node = parser->node_stack + parser->node_stack_index;
						node->group = group;
						node->element = element;
						bool need_skip = (group == 0x301D && element == 0x2014) || // UFS_IMAGE_BLOCK_HEADER_TABLE
								         (group == 0x301D && element == 0x1005) || // PIM_DP_IMAGE_DATA
										 (group == 0x0028 && element == 0x2000);   // DICOM_ICCPROFILE

					    if (need_skip) {
					    	parser->node_stack[parser->node_stack_index].has_base64_content = true;
							char* content_start = doc;
							char* pos = (char*)memchr(content_start, '<', remaining_length);
							if (pos) {
								i64 size = pos - content_start;
								push_to_buffer_maybe_grow((u8**)&parser->contentbuf, &parser->contentlen, &parser->contentbuf_capacity, content_start, size);
								parser->contentcur = parser->contentbuf + parser->contentlen;
//								console_print("iSyntax: skipped tag (0x%04x, 0x%04x) content length = %d\n", group, element, size);
								doc += (size-1); // skip to the next tag
								remaining_length -= (size-1);
								break;
							} else {
//								console_print("iSyntax: skipped tag (0x%04x, 0x%04x) content length = %d\n", group, element, remaining_length);
								push_to_buffer_maybe_grow((u8**)&parser->contentbuf, &parser->contentlen, &parser->contentbuf_capacity, content_start, remaining_length);
								parser->contentcur = parser->contentbuf + parser->contentlen;
								remaining_length = 0; // skip to the next chunk
								break;
							}
						}
					}

					char* tmp = x->data;
					while (*tmp && parser->contentlen < parser->contentbuf_capacity) {
						*(parser->contentcur++) = *(tmp++);
						++parser->contentlen;
						// too long content -> resize buffer
						if (parser->contentlen == parser->contentbuf_capacity) {
							size_t new_capacity = parser->contentbuf_capacity * 2;
							char* new_ptr = (char*)realloc(parser->contentbuf, new_capacity);
							if (!new_ptr) panic();
							parser->contentbuf = new_ptr;
							parser->contentcur = parser->contentbuf + parser->contentlen;
							parser->contentbuf_capacity = new_capacity;
//							console_print("isyntax_parse_xml_header(): XML content buffer overflow (resized buffer to %u)\n", new_capacity);
						}
					}

					*parser->contentcur = '\0';
				} break;
				case YXML_ELEMEND: {
					// end of an element: '.. />' or '</Tag>'

					// NOTE: for a leaf node, it is sufficient to wait for an YXML_ELEMEND, and process it here.
					// (The 'content' is just a string -> easy to handle attributes + content at the same time.)
					// But, for an array node, the YXML_ELEMEND won't be triggered until all of its child nodes
					// are also processed. So in that case we need to intervene at an earlier stage, e.g. at
					// the YXML_CONTENT stage, while we know the

					if (parser->current_node_type == ISYNTAX_NODE_LEAF && !parser->current_node_has_children) {
						console_print_verbose("%sDICOM: %-40s (0x%04x, 0x%04x), size:%-8u = %s\n", get_spaces(parser->node_stack_index),
						                      parser->current_dicom_attribute_name,
						                      parser->current_dicom_group_tag, parser->current_dicom_element_tag, parser->contentlen, parser->contentbuf);

#if 0
						if (parser->node_stack[parser->node_stack_index].group == 0) {
							DUMMY_STATEMENT; // probably the group is 0 because this is a top level node.
						}
#endif

						if (parser->node_stack_index == 2) {
							isyntax_parse_ufsimport_child_node(isyntax, parser->current_dicom_group_tag, parser->current_dicom_element_tag,
							                        parser->contentbuf, parser->contentlen);
						} else {
							isyntax_parse_scannedimage_child_node(isyntax, parser->current_dicom_group_tag, parser->current_dicom_element_tag,
							                                   parser->contentbuf, parser->contentlen);
						}
					} else {
						// We have reached the end of a branch or array node, or a leaf node WITH children.
						const char* elem_name = NULL;
						if (parser->current_node_type == ISYNTAX_NODE_LEAF) {
							// End of a leaf node WITH children.
							// Clear some data that no longer applies when we are 'popping out of' some specific tags.
							if (parser->node_stack[parser->node_stack_index].group == 0x301D) {
								switch(parser->node_stack[parser->node_stack_index].element) {
									default: break;
									case UFS_IMAGE_GENERAL_HEADERS:
									case UFS_IMAGE_BLOCK_HEADER_TEMPLATES: {
										parser->image_header_parsing_mode = 0;
									} break;
								}
							}
							elem_name = "Attribute";
						} else if (parser->current_node_type == ISYNTAX_NODE_BRANCH) {
							elem_name = "DataObject";
						} else if (parser->current_node_type == ISYNTAX_NODE_ARRAY) {
							elem_name = "Array";
						}

						console_print_verbose("%selement end: %s\n", get_spaces(parser->node_stack_index), elem_name);
					}

					// 'Pop' context back to parent node
					if (parser->node_stack_index > 0) {
						--parser->node_stack_index;
						parser->current_node_type = parser->node_stack[parser->node_stack_index].node_type;
						parser->current_node_has_children = parser->node_stack[parser->node_stack_index].has_children;
					} else {
						//TODO: handle error condition
						console_print_error("iSyntax XML error: closing element without matching start\n");
					}

				} break;
				case YXML_ATTRSTART: {
					// attribute: 'Name=..'
//				    console_print_verbose("attr start: %s\n", x->attr);
					parser->attrcur = parser->attrbuf;
					*parser->attrcur = '\0';
					parser->attrlen = 0;
				} break;
				case YXML_ATTRVAL: {
					// attribute value
				    //console_print_verbose("   attr val: %s\n", x->attr);
					if (!parser->attrcur) break;
					char* tmp = x->data;
					while (*tmp && parser->attrbuf < parser->attrbuf_end) {
						*(parser->attrcur++) = *(tmp++);
						++parser->attrlen;
						// too long content -> resize buffer
						if (parser->attrlen == parser->attrbuf_capacity) {
							size_t new_capacity = parser->attrbuf_capacity * 2;
							char* new_ptr = (char*)realloc(parser->attrbuf, new_capacity);
							if (!new_ptr) panic();
							parser->attrbuf = new_ptr;
							parser->attrcur = parser->attrbuf + parser->attrlen;
							parser->attrbuf_capacity = new_capacity;
//							console_print("isyntax_parse_xml_header(): XML attribute buffer overflow (resized buffer to %u)\n", new_capacity);
						}
					}
					*parser->attrcur = '\0';
				} break;
				case YXML_ATTREND: {
					// end of attribute '.."'
					if (parser->attrcur) {
						ASSERT(strlen(parser->attrbuf) == parser->attrlen);

						if (parser->current_node_type == ISYNTAX_NODE_LEAF) {
							if (parser->attribute_index == 0 /* Name="..." */) {
								if (paranoid_mode) isyntax_validate_dicom_attr(x->attr, "Name");
								size_t copy_size = MIN(parser->attrlen, sizeof(parser->current_dicom_attribute_name));
								memcpy(parser->current_dicom_attribute_name, parser->attrbuf, copy_size);
								i32 one_past_last_char = MIN(parser->attrlen, sizeof(parser->current_dicom_attribute_name)-1);
								parser->current_dicom_attribute_name[one_past_last_char] = '\0';
								DUMMY_STATEMENT;
							} else if (parser->attribute_index == 1 /* Group="0x...." */) {
								if (paranoid_mode) isyntax_validate_dicom_attr(x->attr, "Group");
								parser->current_dicom_group_tag = strtoul(parser->attrbuf, NULL, 0);
								DUMMY_STATEMENT;
							} else if (parser->attribute_index == 2 /* Element="0x...." */) {
								if (paranoid_mode) isyntax_validate_dicom_attr(x->attr, "Element");
								parser->current_dicom_element_tag = strtoul(parser->attrbuf, NULL, 0);
								DUMMY_STATEMENT;
							} else if (parser->attribute_index == 3 /* PMSVR="..." */) {
								if (paranoid_mode) isyntax_validate_dicom_attr(x->attr, "PMSVR");
								if (strcmp(parser->attrbuf, "IDataObjectArray") == 0) {
									parser->current_node_has_children = true;
									parser->node_stack[parser->node_stack_index].has_children = true;
									console_print_verbose("%sDICOM: %-40s (0x%04x, 0x%04x), array\n", get_spaces(parser->node_stack_index),
							                              parser->current_dicom_attribute_name,
									                      parser->current_dicom_group_tag, parser->current_dicom_element_tag);
									if (parser->node_stack_index == 2) { // At level of UfsImport
										isyntax_parse_ufsimport_child_node(isyntax, parser->current_dicom_group_tag, parser->current_dicom_element_tag,
										                        parser->contentbuf, parser->contentlen);
									} else {
										isyntax_parse_scannedimage_child_node(isyntax, parser->current_dicom_group_tag, parser->current_dicom_element_tag,
										                                   parser->contentbuf, parser->contentlen);
									}

								}
							}
						} else if (parser->current_node_type == ISYNTAX_NODE_BRANCH) {
							// A DataObject node is supposed to have one attribute "ObjectType"
							ASSERT(parser->attribute_index == 0);
							ASSERT(strcmp(x->attr, "ObjectType") == 0);
							console_print_verbose("%sDataObject %s = %s\n", get_spaces(parser->node_stack_index), x->attr, parser->attrbuf);
							if (strcmp(parser->attrbuf, "DPScannedImage") == 0) {
								// We started parsing a new image (which will be either a WSI, LABELIMAGE or MACROIMAGE).
								parser->current_image = isyntax->images + isyntax->image_count;
								++isyntax->image_count;
							}
						} else {
							console_print_verbose("%sattr %s = %s\n", get_spaces(parser->node_stack_index), x->attr, parser->attrbuf);
						}
						++parser->attribute_index;

					}
				} break;
				case YXML_PISTART:
				case YXML_PICONTENT:
				case YXML_PIEND:
					break; // processing instructions (uninteresting, skip)
				default: {
					console_print_error("yxml_parse(): unrecognized token (%d)\n", r);
					goto failed;
				}
			}
		}
	}

	success = true;
	if (is_last_chunk) {
		goto cleanup;
	} else {
		return success; // no cleanup yet, we will still need resources until the last header chunk is reached.
	}
}


// Read between 57 and 64 bits (7 bytes + 1-8 bits) from a bitstream (least significant beast first).
// Requires that at least 7 safety bytes are present at the end of the stream (don't trigger a segmentation fault)!
static inline u64 bitstream_lsb_read(u8* buffer, u32 pos) {
	u64 raw = *(u64*)(buffer + pos / 8);
	raw >>= pos % 8;
	return raw;
}

static inline u64 bitstream_lsb_read_advance(u8* buffer, i32* bits_read, i32 bits_to_read) {
	u64 raw = *(u64*)(buffer + (*bits_read / 8));
	raw >>= (*bits_read / 8);
	*bits_read += bits_to_read;
	return raw;
}


// partly adapted from stb_image.h
#define HUFFMAN_FAST_BITS 12

typedef struct huffman_t {
	u16  fast[1 << HUFFMAN_FAST_BITS];
	u16 code[256];
	u8  size[256];
} huffman_t;

void save_code_in_huffman_fast_lookup_table(huffman_t* h, u32 code, u32 code_width, u8 symbol) {
	ASSERT(code_width <= HUFFMAN_FAST_BITS);
	i32 duplicate_bits = HUFFMAN_FAST_BITS - code_width;
	for (u32 i = 0; i < (1 << duplicate_bits); ++i) {
		u32 address = (i << code_width) | code;
		h->fast[address] = symbol;
	}
}

u32 max_code_size;
u32 symbol_counts[256];
u64 fast_count;
u64 nonfast_count;

u8* isyntax_hulsken_decompress(isyntax_codeblock_t* codeblock, i32 coeff_count, i32 compressor_version) {
	ASSERT(coeff_count == 1 || coeff_count == 3);
	ASSERT(compressor_version == 1 || compressor_version == 2);

	// Read the header information stored in the codeblock.
	// The layout varies depending on the version of the compressor used (version 1 or 2).
	// All integers are stored little-endian, least-significant bit first.
	//
	// Version 1 layout:
	//   uint32 : serialized length (in bytes)
	//   uint8 : zero run symbol
	//   uint8 : zero run counter size (in bits)
	// Version 2 layout:
	//   coeff_count (== 1 or 3) * coeff_bit_depth bits : 1 or 3 bitmasks, indicating which bitplanes are present
	//   uint8 : zero run symbol
	//   uint8 : zero run counter size (in bits)
	//   (variable length) : bitplane seektable (contains offsets to each of the bitplanes)

	// After the header section, the rest of the codeblock contains a Huffman tree, followed by a Huffman-coded
	// message of 8-bit Huffman symbols, interspersed with 'zero run' symbols (for run-length encoding of zeroes).

	i32 coeff_bit_depth = 16; // fixed value for iSyntax
	i32 block_width = 128; // fixed value for iSyntax (?)
	i32 block_height = 128; // fixed value for iSyntax (?)
	i32 bits_read = 0;
	i32 block_size_in_bits = codeblock->block_size * 8;
	i64 serialized_length = 0; // In v1: stored in the first 4 bytes. In v2: derived calculation.
	u32 bitmasks[3] = { 0x000FFFF, 0x000FFFF, 0x000FFFF }; // default in v1: all ones, can be overridden later
	i32 total_mask_bits = coeff_bit_depth * coeff_count;
	u8* byte_pos = codeblock->data;
	if (compressor_version == 1) {
		serialized_length = *(u32*)byte_pos;
		byte_pos += 4;
		bits_read += 4*8;
	} else {
		if (coeff_count == 1) {
			bitmasks[0] = *(u16*)(byte_pos);
			byte_pos += 2;
			bits_read += 2*8;
			total_mask_bits = _popcnt32(bitmasks[0]);
		} else if (coeff_count == 3) {
			bitmasks[0] = *(u16*)(byte_pos);
			bitmasks[1] = *(u16*)(byte_pos+2);
			bitmasks[2] = *(u16*)(byte_pos+4);
			byte_pos += 6;
			bits_read += 6*8;
			total_mask_bits = _popcnt32(bitmasks[0]) + _popcnt32(bitmasks[1]) + _popcnt32(bitmasks[2]);
		} else {
			panic();
		}
		serialized_length = total_mask_bits * (block_width * block_height / 8);
	}
	u8 zerorun_symbol = *(u8*)byte_pos++;
	bits_read += 8;
	u8 zero_counter_size = *(u8*)byte_pos++;
	bits_read += 8;

	if (compressor_version >= 2) {
		// read bitplane seektable
		i32 stored_bit_plane_count = total_mask_bits;
		u32* bitplane_offsets = alloca(stored_bit_plane_count * sizeof(u32));
		i32 bitplane_ptr_bits = (i32)(log2f(serialized_length)) + 5;
		for (i32 i = 0; i < stored_bit_plane_count; ++i) {
			bitplane_offsets[i] = bitstream_lsb_read_advance(codeblock->data, &bits_read, bitplane_ptr_bits);
		}
	}

	// Read Huffman table
	huffman_t huffman = {};
	memset(huffman.fast, 0x80, sizeof(huffman.fast));
	u32 fast_mask = (1 << HUFFMAN_FAST_BITS) - 1;
	{
		i32 code_size = 0;
		u32 code = 0;
		do {
			// Read a chunk of bits large enough to 'always' have the whole Huffman code, followed by the 8-bit symbol.
			// A blob of 57-64 bits is more than sufficient for a Huffman code of at most 16 bits.
			// The bitstream is organized least significant bit first (treat as one giant little-endian integer).
			// To read bits in the stream, look at the lowest bit positions. To advance the stream, shift right.
			i32 bits_to_advance = 1;
			u64 blob = bitstream_lsb_read(codeblock->data, bits_read); // gives back between 57 and 64 bits.

			// 'Descend' into the tree until we hit a leaf node.
			bool is_leaf = blob & 1;
			// TODO: intrinsic?
			while (!is_leaf) {
				++bits_to_advance;
				blob >>= 1;
				is_leaf = (blob & 1);
				++code_size;
			}
			blob >>= 1;

			// Read 8-bit Huffman symbol
			u8 symbol = (u8)(blob);
			huffman.code[symbol] = code;
			huffman.size[symbol] = code_size;

			if (code_size <= HUFFMAN_FAST_BITS) {
				// We can accelerate decoding of small Huffman codes by storing them in a lookup table.
				// However, for the longer codes this becomes inefficient so in those cases we need another method.
				save_code_in_huffman_fast_lookup_table(&huffman, code, code_size, symbol);
				++fast_count;
			} else {
				// TODO: how to make this fast?
				u32 prefix = code & fast_mask;
				u16 old_fast_data = huffman.fast[prefix];
				u8 old_max_size = old_fast_data & 0x1F;
				u8 new_max_size = MAX(old_max_size, code_size);
				huffman.fast[prefix] = 256 + new_max_size;
				++nonfast_count;
			}
			if (code_size > max_code_size) {
				max_code_size = code_size;
//			    console_print("found the biggest code size: %d\n", code_size);
			}
			symbol_counts[symbol]++;

			bits_to_advance += 8;
			bits_read += bits_to_advance;

			// traverse back up the tree: find last zero -> flip to one
			if (code_size == 0) {
				break; // already done; this happens if there is only a root node, no leaves
			}
			u32 code_high_bit = (1 << (code_size - 1));
			bool found_zero = (~code) & code_high_bit;
			while (!found_zero) {
				--code_size;
				if (code_size == 0) break;
				code &= code_high_bit - 1;
				code_high_bit >>= 1;
				found_zero = (~code) & code_high_bit;
			}
			code |= code_high_bit;
		} while(code_size > 0);
	}

#if 0 // for running without decoding, for debugging and performance measurements
	u8* output_buffer = NULL;
#else
	// Decode the message
	u8* output_buffer = (u8*)malloc(serialized_length);

	u32 zerorun_code = huffman.code[zerorun_symbol];
	u32 zerorun_code_size = huffman.size[zerorun_symbol];
	if (zerorun_code_size == 0) zerorun_code_size = 1; // handle special case of the 'empty' Huffman tree (root node is leaf node)
	u32 zerorun_code_mask = (1 << zerorun_code_size) - 1;

	u32 zero_counter_mask = (1 << zero_counter_size) - 1;
	i32 decompressed_length = 0;
	while (bits_read < block_size_in_bits) {
		if (decompressed_length >= serialized_length) {
			break; // done
		}
		i32 symbol = 0;
		i32 code_size = 1;
		u64 blob = bitstream_lsb_read(codeblock->data, bits_read);
		u32 fast_index = blob & fast_mask;
		u16 c = huffman.fast[fast_index];
		if (c <= 255) {
			// Lookup the symbol directly.
			symbol = c;
			code_size = huffman.size[symbol];
		} else {
			// TODO: super naive and slow implementation, accelerate this!
			bool match = false;
			for (i32 i = 0; i < 256; ++i) {
				u8 test_size = huffman.size[i];
				if (test_size <= HUFFMAN_FAST_BITS) continue;
				u16 test_code = huffman.code[i];
				if ((blob & ((1 << test_size)-1)) == test_code) {
					// match
					code_size = test_size;
					symbol = i;
					match = true;
					break;
				}
			}
			if (!match) {
				DUMMY_STATEMENT;
			}
		}

		if (code_size == 0) code_size = 1; // handle special case of the 'empty' Huffman tree (root node is leaf node)

		blob >>= code_size;
		bits_read += code_size;

		// Handle run-length encoding of zeroes
		if (symbol == zerorun_symbol) {
			u32 numzeroes = blob & zero_counter_mask;
			bits_read += zero_counter_size;
			// A 'zero run' with length of zero means that this is not a zero run after all, but rather
			// the 'escaped' zero run symbol itself which should be outputted.
			if (numzeroes > 0) {
				if (compressor_version == 2) ++numzeroes; // v2 stores actual count minus one
				if (decompressed_length + numzeroes >= serialized_length) {
					// Reached the end, terminate
					memset(output_buffer + decompressed_length, 0, MIN(serialized_length - decompressed_length, numzeroes));
					decompressed_length += numzeroes;
					break;
				}
				// If the next Huffman symbol is also the zero run symbol, then their counters actually refer to the same zero run.
				// Basically, each extra zero run symbol expands the 'zero counter' bit depth, i.e.:
				//   n zero symbols -> depth becomes n * counter_bits
				u32 total_zero_counter_size = zero_counter_size;
				for(;;) {
					// Peek ahead in the bitstream, grab any additional zero run symbols, and recalculate numzeroes.
					blob = bitstream_lsb_read(codeblock->data, bits_read);
					u8 next_code = (blob & zerorun_code_mask);
					if (next_code == zerorun_code) {
						// The zero run continues
						blob >>= zerorun_code_size;
						u32 counter_extra_bits = blob & zero_counter_mask;
						if (compressor_version == 2) ++counter_extra_bits; // v2 stores actual count minus one
						numzeroes <<= zero_counter_size;
						numzeroes |= (counter_extra_bits);
						total_zero_counter_size += zero_counter_size;
						bits_read += zerorun_code_size + zero_counter_size;
						if (decompressed_length + numzeroes >= serialized_length) {
							break; // Reached the end, terminate
						}
					} else {
						break; // no next zero run symbol, the zero run is finished
					}
				}

				i32 bytes_to_write = MIN(serialized_length - decompressed_length, numzeroes);
				ASSERT(bytes_to_write > 0);
				memset(output_buffer + decompressed_length, 0, bytes_to_write);
				decompressed_length += numzeroes;
			} else {
				// This is not a 'zero run' after all, but an escaped symbol. So output the symbol.
				output_buffer[decompressed_length++] = symbol;
			}
		} else {
			output_buffer[decompressed_length++] = symbol;
		}

	}
#endif

	if (serialized_length != decompressed_length) {
		ASSERT(!"size mismatch");
		console_print("iSyntax: size mismatch in block %d (size=%d): expected %d observed %d\n",
				codeblock->block_data_offset, codeblock->block_size, serialized_length, decompressed_length);
	}
	codeblock->decompressed_size = decompressed_length;
	return output_buffer;

}

#define DO_DEBUG_HUFFMAN_DECODE 0
#if  DO_DEBUG_HUFFMAN_DECODE

void debug_read_codeblock_from_file(isyntax_codeblock_t* codeblock, FILE* fp) {
	if (fp && !codeblock->data) {
		codeblock->data = calloc(1, codeblock->block_size + 8); // TODO: pool allocator
		fseeko64(fp, codeblock->block_data_offset, SEEK_SET);
		fread(codeblock->data, codeblock->block_size, 1, fp);

#if 0
		char out_filename[512];
		snprintf(out_filename, 512, "codeblocks/%d.bin", codeblock->block_data_offset);
		FILE* out = fopen(out_filename, "wb");
		if (out) {
			fwrite(codeblock->data, codeblock->block_size, 1, out);
			fclose(out);
		}
#endif
	}
}
#endif // DO_DEBUG_HUFFMAN_DECODE

bool isyntax_open(isyntax_t* isyntax, const char* filename) {

	ASSERT(isyntax);
	memset(isyntax, 0, sizeof(*isyntax));

	int ret = 0; (void)ret;
	FILE* fp = fopen64(filename, "rb");
	bool success = false;
	if (fp) {
		struct stat st;
		if (fstat(fileno(fp), &st) == 0) {
			isyntax->filesize = st.st_size;

			// https://www.openpathology.philips.com/wp-content/uploads/isyntax/4522%20207%2043941_2020_04_24%20Pathology%20iSyntax%20image%20format.pdf
			// Layout of an iSyntax file:
			// XML Header | End of Table (EOT) marker, 3 bytes "\r\n\x04" | Seektable (optional) | Codeblocks

			// Read the XML header
			// We don't know the length of the XML header, so we just read 'enough' data in a chunk and hope that we get it
			// (and if not, read some more data until we get it)

			i64 load_begin = get_clock();
			i64 io_begin = get_clock();
			i64 io_ticks_elapsed = 0;
			i64 parse_begin = get_clock();
			i64 parse_ticks_elapsed = 0;

			size_t read_size = MEGABYTES(1);
			char* read_buffer = malloc(read_size);
			size_t bytes_read = fread(read_buffer, 1, read_size, fp);
			io_ticks_elapsed += (get_clock() - io_begin);

			if (bytes_read < 3) goto fail_1;
			bool are_there_bytes_left = (bytes_read == read_size);

			// find EOT candidates, 3 bytes "\r\n\x04"
			i64 header_length = 0;
			i64 isyntax_data_offset = 0; // Offset of either the Seektable, or the Codeblocks segment of the iSyntax file.
			i64 bytes_read_from_data_offset_in_last_chunk = 0;

			i32 chunk_index = 0;
			for (;; ++chunk_index) {
//				console_print("iSyntax: reading XML header chunk %d\n", chunk_index);
				i64 chunk_length = 0;
				bool match = false;
				char* pos = read_buffer;
				i64 offset = 0;
				char* marker = (char*)memchr(read_buffer, '\x04', bytes_read);
				if (marker) {
					offset = marker - read_buffer;
					match = true;
					chunk_length = offset;
					header_length += chunk_length;
					isyntax_data_offset = header_length + 1;
					i64 data_offset_in_last_chunk = offset + 1;
					bytes_read_from_data_offset_in_last_chunk = (i64)bytes_read - data_offset_in_last_chunk;
				}
				if (match) {
					// We found the end of the XML header. This is the last chunk to process.
					if (!(header_length > 0 && header_length < isyntax->filesize)) goto fail_1;

					parse_begin = get_clock();
					isyntax_parse_xml_header(isyntax, read_buffer, chunk_length, true);
					parse_ticks_elapsed += (get_clock() - parse_begin);

					console_print("iSyntax: the XML header is %u bytes, or %g%% of the total file size\n", header_length, (float)((float)header_length * 100.0f) / isyntax->filesize);
//					console_print("   I/O time: %g seconds\n", get_seconds_elapsed(0, io_ticks_elapsed));
//					console_print("   Parsing time: %g seconds\n", get_seconds_elapsed(0, parse_ticks_elapsed));
//					console_print("   Total loading time: %g seconds\n", get_seconds_elapsed(load_begin, get_clock()));
					break;
				} else {
					// We didn't find the end of the XML header. We need to read more chunks to find it.
					// (Or, we reached the end of the file unexpectedly, which is an error.)
					chunk_length = read_size;
					header_length += chunk_length;
					if (are_there_bytes_left) {

						parse_begin = get_clock();
						isyntax_parse_xml_header(isyntax, read_buffer, chunk_length, false);
						parse_ticks_elapsed += (get_clock() - parse_begin);

						io_begin = get_clock();
						bytes_read = fread(read_buffer, 1, read_size, fp); // read the next chunk
						io_ticks_elapsed += (get_clock() - io_begin);

						are_there_bytes_left = (bytes_read == read_size);
						continue;
					} else {
						console_print_error("iSyntax parsing error: didn't find the end of the XML header (unexpected end of file)\n");
						goto fail_1;
					}
				}
			}

			// TODO: account for padding
#if 0
			// Padding
			// 1: Uniform padding with black pixels on all sides
			i64 per_level_padding = 3; // for Legall 5/3 wavelet transform
			i64 num_levels = 9; // TODO: get from UFSGeneralImageHeader UFS_IMAGE_DIMENSION_RANGES
			i64 padding = (per_level_padding << num_levels) - per_level_padding;

			// 2: further padding on the right and bottom side.
			// (To ensure that the image dimensions are a multiple of the codeblock size.)
			i64 base_image_width = 148480;
			i64 base_image_height = 93184;
			i64 block_width = 128;
			i64 block_height = 128;
			i64 grid_width = ((base_image_width + (block_width << num_levels) - 1) / (block_width << num_levels)) << (num_levels - 1);
			i64 grid_height = ((base_image_height + (block_height << num_levels) - 1) / (block_height << num_levels)) << (num_levels - 1);
#endif

			isyntax_image_t* wsi_image = isyntax->wsi_image;
			if (wsi_image) {
				io_begin = get_clock(); // for performance measurement
				fseeko64(fp, isyntax_data_offset, SEEK_SET);
				if (wsi_image->header_codeblocks_are_partial) {
					// The seektable is required to be present, because the block header table did not contain all information.
					dicom_tag_header_t seektable_header_tag = {};
					fread(&seektable_header_tag, sizeof(dicom_tag_header_t), 1, fp);

					io_ticks_elapsed += (get_clock() - io_begin);
					parse_begin = get_clock();

					if (seektable_header_tag.group == 0x301D && seektable_header_tag.element == 0x2015) {
						i32 seektable_size = seektable_header_tag.size;
						if (seektable_size < 0) {
							// We need to guess the size...
							ASSERT(wsi_image->codeblock_count > 0);
							seektable_size = sizeof(isyntax_seektable_codeblock_header_t) * wsi_image->codeblock_count;
						}
						isyntax_seektable_codeblock_header_t* codeblock_headers =
								(isyntax_seektable_codeblock_header_t*) malloc(seektable_size);
						fread(codeblock_headers, seektable_size, 1, fp);

						// Now fill in the missing data.
						// NOTE: The number of codeblock entries in the seektable is much greater than the number of
						// codeblocks that *actually* exist in the file. This means that we have to discard many of
						// the seektable entries.
						// Luckily, we can easily identify the entries that need to be discarded.
						// (They have the data offset (and data size) set to 0.)
						i32 seektable_entry_count = seektable_size / sizeof(isyntax_seektable_codeblock_header_t);
						i32 actual_real_codeblock_index = 0;
						for (i32 i = 0; i < seektable_entry_count; ++i) {
							isyntax_seektable_codeblock_header_t* seektable_entry = codeblock_headers + i;
							ASSERT(seektable_entry->block_data_offset_header.group == 0x301D);
							ASSERT(seektable_entry->block_data_offset_header.element == 0x2010);
							if (seektable_entry->block_data_offset != 0) {
								isyntax_codeblock_t* codeblock = wsi_image->codeblocks + actual_real_codeblock_index;
								codeblock->block_data_offset = seektable_entry->block_data_offset;
								codeblock->block_size = seektable_entry->block_size;
#if DO_DEBUG_HUFFMAN_DECODE
								// Debug test:
								// Decompress codeblocks in the seektable.
								if (1 || actual_real_codeblock_index == wsi_image->codeblock_count-1) {
									// Only parse 'non-empty'/'background' codeblocks
									if (codeblock->block_size > 8 /*&& codeblock->block_data_offset == 129572464*/) {
										debug_read_codeblock_from_file(codeblock, fp);
										u8* decompressed = isyntax_hulsken_decompress(codeblock, 3, 1);
										if (decompressed) {
#if 0
											FILE* out = fopen("hulskendecompressed4.raw", "wb");
											if(out) {
												fwrite(decompressed, codeblock->decompressed_size, 1, out);
												fclose(out);
											}
#endif
											free(decompressed);
										}
									}
								}
#endif
								++actual_real_codeblock_index;
								if (actual_real_codeblock_index == wsi_image->codeblock_count) {
									break; // we're done!
								}
							}

						}
						parse_ticks_elapsed += (get_clock() - parse_begin);
						console_print("iSyntax: the seektable is %u bytes, or %g%% of the total file size\n", seektable_size, (float)((float)seektable_size * 100.0f) / isyntax->filesize);
						console_print("   I/O time: %g seconds\n", get_seconds_elapsed(0, io_ticks_elapsed));
						console_print("   Parsing time: %g seconds\n", get_seconds_elapsed(0, parse_ticks_elapsed));
						console_print("   Total loading time: %g seconds\n", get_seconds_elapsed(load_begin, get_clock()));
					} else {
						// TODO: error
					}
				}
			} else {
				// TODO: error
			}



			// Process the XML header
#if 0
			char* xml_header = read_buffer;
			xml_header[header_length] = '\0';

			isyntax_parse_xml_header(isyntax, xml_header, header_length);

#if 0 // dump XML header for testing
			FILE* test_out = fopen("isyntax_header.xml", "wb");
			fwrite(xml_header, header_length, 1, test_out);
			fclose(test_out);
#endif
#endif

			// TODO: further implement iSyntax support
			success = true;

			fail_1:
			free(read_buffer);
		}
		fclose(fp);
	}
	return success;
}


