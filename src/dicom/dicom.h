/*
  Slidescape, a whole-slide image viewer for digital pathology.
  Copyright (C) 2019-2023  Pieter Valkema

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

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"
#include "platform.h" // for file_handle_t
#include "mathutils.h" // for v2f

#ifndef DONT_INCLUDE_DICOM_DICT_H
#include "dicom_dict.h"
#else
typedef u32 dicom_uid_enum;
typedef u32 dicom_tag_enum;
#endif

// Convert two-byte strings into their little-endian number equivalent
#define LE_2CHARS(a,b) ( ((b)<<8) | (a) )
#define LE_4CHARS(a,b,c,d) ( ((d)<<24) | ((c)<<16) | ((b)<<8) | (a) )

enum dicom_value_representation_enum {
	DICOM_VR_AE = LE_2CHARS('A','E'), // Application Entity      // 4 bytes fixed
	DICOM_VR_AS = LE_2CHARS('A','S'), // Age String              // 4 bytes fixed
	DICOM_VR_AT = LE_2CHARS('A','T'), // Attribute Tag           // 4 bytes fixed
	DICOM_VR_CS = LE_2CHARS('C','S'), // Code String             // 16 bytes maximum
	DICOM_VR_DA = LE_2CHARS('D','A'), // Date                    // 8 bytes fixed
	DICOM_VR_DS = LE_2CHARS('D','S'), // Decimal String          // 16 bytes maximum
	DICOM_VR_DT = LE_2CHARS('D','T'), // Date Time               // 26 bytes maximum (or 54 for a range)
	DICOM_VR_FD = LE_2CHARS('F','D'), // Floating Point Double   // 8 bytes fixed
	DICOM_VR_FL = LE_2CHARS('F','L'), // Floating Point Single   // 4 bytes fixed
	DICOM_VR_IS = LE_2CHARS('I','S'), // Integer String          // 12 bytes maximum
	DICOM_VR_LO = LE_2CHARS('L','O'), // Long String             // 64 chars maximum
	DICOM_VR_LT = LE_2CHARS('L','T'), // Long Text               // 10240 chars maximum
	DICOM_VR_OB = LE_2CHARS('O','B'), // Other Byte (String)
	DICOM_VR_OD = LE_2CHARS('O','D'), // Other Double (String)
	DICOM_VR_OF = LE_2CHARS('O','F'), // Other Float (String)
	DICOM_VR_OL = LE_2CHARS('O','L'), // Other Long (String)
	DICOM_VR_OV = LE_2CHARS('O','V'), // Other 64-Bit Very Long (String)
	DICOM_VR_OW = LE_2CHARS('O','W'), // Other Word (String)
	DICOM_VR_PN = LE_2CHARS('P','N'), // Person Name
	DICOM_VR_SH = LE_2CHARS('S','H'), // Short String            // 16 chars maximum
	DICOM_VR_SL = LE_2CHARS('S','L'), // Signed Long             // 4 bytes fixed
	DICOM_VR_SQ = LE_2CHARS('S','Q'), // Sequence of Items
	DICOM_VR_SS = LE_2CHARS('S','S'), // Signed Short            // 2 bytes fixed
	DICOM_VR_ST = LE_2CHARS('S','T'), // Short Text              // 1024 chars maximum
	DICOM_VR_SV = LE_2CHARS('S','V'), // Signed 64-Bit Very Long // 8 bytes fixed
	DICOM_VR_TM = LE_2CHARS('T','M'), // Time                    // 14 bytes maximum
	DICOM_VR_UC = LE_2CHARS('U','C'), // Unlimited Characters
	DICOM_VR_UI = LE_2CHARS('U','I'), // Unique Identifier       // 64 bytes maximum
	DICOM_VR_UL = LE_2CHARS('U','L'), // Unsigned Long           // 4 bytes fixed
	DICOM_VR_UN = LE_2CHARS('U','N'), // Unknown
	DICOM_VR_UR = LE_2CHARS('U','R'), // URI/URL
	DICOM_VR_US = LE_2CHARS('U','S'), // Unsigned Short          // 2 bytes fixed
	DICOM_VR_UT = LE_2CHARS('U','T'), // Unlimited Text
	DICOM_VR_UV = LE_2CHARS('U','V'), // Unsigned 64-Bit Very Long // 8 bytes fixed
};


typedef enum dicom_transfer_syntax_enum {
	DICOM_TRANSFER_SYNTAX_IMPLICIT_VR_LITTLE_ENDIAN,
	DICOM_TRANSFER_SYNTAX_EXPLICIT_VR_LITTLE_ENDIAN,
	DICOM_TRANSFER_SYNTAX_DEFLATED_EXPLICIT_VR_LITTLE_ENDIAN,
	DICOM_TRANSFER_SYNTAX_EXPLICIT_VR_BIG_ENDIAN_RETIRED,
} dicom_transfer_syntax_enum;

typedef enum dicom_photometric_interpretation_enum {
	DICOM_PHOTOMETRIC_INTERPRETATION_UNKNOWN = 0,
	DICOM_PHOTOMETRIC_INTERPRETATION_MONOCHROME1 = 1,
	DICOM_PHOTOMETRIC_INTERPRETATION_MONOCHROME2 = 2,
	DICOM_PHOTOMETRIC_INTERPRETATION_PALETTE_COLOR = 3,
	DICOM_PHOTOMETRIC_INTERPRETATION_RGB = 4,
	DICOM_PHOTOMETRIC_INTERPRETATION_HSV = 5, // retired
	DICOM_PHOTOMETRIC_INTERPRETATION_ARGB = 6, // retired
	DICOM_PHOTOMETRIC_INTERPRETATION_CMYK = 7, // retired
	DICOM_PHOTOMETRIC_INTERPRETATION_YBR_FULL = 8,
	DICOM_PHOTOMETRIC_INTERPRETATION_YBR_FULL_422 = 9,
	DICOM_PHOTOMETRIC_INTERPRETATION_YBR_PARTIAL_422 = 10, // retired
	DICOM_PHOTOMETRIC_INTERPRETATION_YBR_PARTIAL_420 = 11,
	DICOM_PHOTOMETRIC_INTERPRETATION_YBR_ICT = 12,
	DICOM_PHOTOMETRIC_INTERPRETATION_YBR_RCT = 13,
} dicom_photometric_interpretation_enum;

typedef enum dicom_lossy_image_compression_method_enum {
	DICOM_LOSSY_IMAGE_COMPRESSION_METHOD_UNKNOWN = 0,
	DICOM_LOSSY_IMAGE_COMPRESSION_METHOD_ISO_10918_1, // JPEG Lossy Compression
	DICOM_LOSSY_IMAGE_COMPRESSION_METHOD_ISO_14495_1, // JPEG-LS Near-lossless Compression
	DICOM_LOSSY_IMAGE_COMPRESSION_METHOD_ISO_15444_1, // JPEG 2000 Irreversible Compression
	DICOM_LOSSY_IMAGE_COMPRESSION_METHOD_ISO_13818_2, // MPEG2 Compression
	DICOM_LOSSY_IMAGE_COMPRESSION_METHOD_ISO_14496_10, // MPEG-4 AVC/H.264 Compression
	DICOM_LOSSY_IMAGE_COMPRESSION_METHOD_ISO_23008_2, // HEVC/H.265 Lossy Compression

} dicom_lossy_image_compression_method_enum;

typedef enum dicom_image_flavor_enum {
	DICOM_IMAGE_FLAVOR_UNKNOWN = 0,
	DICOM_IMAGE_FLAVOR_VOLUME,
	DICOM_IMAGE_FLAVOR_LABEL,
	DICOM_IMAGE_FLAVOR_OVERVIEW,
	DICOM_IMAGE_FLAVOR_THUMBNAIL,
} dicom_image_flavor_enum;

typedef enum dicom_modality_enum {
    DICOM_MODALITY_UNKNOWN = 0,
    // NOTE: we resolve by linear string lookup; 'cheat' by putting our favorite modality first in the list (which we expect to use most often anyway)
    DICOM_MODALITY_SM = 1,  // Slide Microscopy
    // The rest of the modalities in alphabetical order
    DICOM_MODALITY_ANN,     // Annotation
    DICOM_MODALITY_AR,      // Autorefraction
    DICOM_MODALITY_ASMT,    // Content Assessment Results
    DICOM_MODALITY_AU,      // Audio
    DICOM_MODALITY_BDUS,    // Bone Densitometry (ultrasound)
    DICOM_MODALITY_BI,      // Biomagnetic imaging
    DICOM_MODALITY_BMD,     // Bone Densitometry (X-Ray)
    DICOM_MODALITY_CR,      // Computed Radiography
    DICOM_MODALITY_CT,      // Computed Tomography
    DICOM_MODALITY_CTPROTOCOL, // CT Protocol (Performed)
    DICOM_MODALITY_DMS,     // Dermoscopy
    DICOM_MODALITY_DG,      // Diaphanography
    DICOM_MODALITY_DOC,     // Document
    DICOM_MODALITY_DX,      // Digital Radiography
    DICOM_MODALITY_ECG,     // Electrocardiography
    DICOM_MODALITY_EEG,     // Electroencephalography
    DICOM_MODALITY_EMG,     // Electromyography
    DICOM_MODALITY_EOG,     // Electrooculography
    DICOM_MODALITY_EPS,     // Cardiac Electrophysiology
    DICOM_MODALITY_ES,      // Endoscopy
    DICOM_MODALITY_FID,     // Fiducials
    DICOM_MODALITY_GM,      // General Microscopy
    DICOM_MODALITY_HC,      // Hard Copy
    DICOM_MODALITY_HD,      // Hemodynamic Waveform
    DICOM_MODALITY_IO,      // Intra-Oral Radiography
    DICOM_MODALITY_IOL,     // Intraocular Lens Data
    DICOM_MODALITY_IVOCT,   // Intravascular Optical Coherence Tomography
    DICOM_MODALITY_IVUS,    // Intravascular Ultrasound
    DICOM_MODALITY_KER,     // Keratometry
    DICOM_MODALITY_KO,      // Key Object Selection
    DICOM_MODALITY_LEN,     // Lensometry
    DICOM_MODALITY_LS,      // Laser surface scan
    DICOM_MODALITY_MG,      // Mammography
    DICOM_MODALITY_MR,      // Magnetic Resonance
    DICOM_MODALITY_M3D,     // Model for 3D Manufacturing
    DICOM_MODALITY_NM,      // Nuclear Medicine
    DICOM_MODALITY_OAM,     // Ophthalmic Axial Measurements
    DICOM_MODALITY_OCT,     // Optical Coherence Tomography (non-Ophthalmic)
    DICOM_MODALITY_OP,      // Ophthalmic Photography
    DICOM_MODALITY_OPM,     // Ophthalmic Mapping
    DICOM_MODALITY_OPT,     // Ophthalmic Tomography
    DICOM_MODALITY_OPTBSV,  // Ophthalmic Tomography B-scan Volume Analysis
    DICOM_MODALITY_OPTENF,  // Ophthalmic Tomography En Face
    DICOM_MODALITY_OPV,     // Ophthalmic Visual Field
    DICOM_MODALITY_OSS,     // Optical Surface Scan
    DICOM_MODALITY_OT,      // Other
    DICOM_MODALITY_PLAN,    // Plan
    DICOM_MODALITY_POS,     // Position Sensor
    DICOM_MODALITY_PR,      // Presentation State
    DICOM_MODALITY_PT,      // Positron emission tomography (PET)
    DICOM_MODALITY_PX,      // Panoramic X-Ray
    DICOM_MODALITY_REG,     // Registration
    DICOM_MODALITY_RESP,    // Respiratory Waveform
    DICOM_MODALITY_RF,      // Radio Fluoroscopy
    DICOM_MODALITY_RG,      // Radiographic imaging (conventional film/screen)
    DICOM_MODALITY_RTDOSE,  // Radiotherapy Dose
    DICOM_MODALITY_RTIMAGE, // Radiotherapy Image
    DICOM_MODALITY_RTINTENT,// Radiotherapy Intent
    DICOM_MODALITY_RTPLAN,  // Radiotherapy Plan
    DICOM_MODALITY_RTRAD,   // RT Radiation
    DICOM_MODALITY_RTRECORD,// RT Treatment Record
    DICOM_MODALITY_RTSEGANN,// Radiotherapy Segment Annotation
    DICOM_MODALITY_RTSTRUCT,// Radiotherapy Structure Set
    DICOM_MODALITY_RWV,     // Real World Value Map
    DICOM_MODALITY_SEG,     // Segmentation
    DICOM_MODALITY_SMR,     // Stereometric Relationship
    DICOM_MODALITY_SR,      // SR Document
    DICOM_MODALITY_SRF,     // Subjective Refraction
    DICOM_MODALITY_STAIN,   // Automated Slide Stainer
    DICOM_MODALITY_TEXTUREMAP, // Texture Map
    DICOM_MODALITY_TG,      // Thermography
    DICOM_MODALITY_US,      // Ultrasound
    DICOM_MODALITY_VA,      // Visual Acuity
    DICOM_MODALITY_XA,      // X-Ray Angiography
    DICOM_MODALITY_XAPROTOCOL, // XA Protocol (Performed)
    DICOM_MODALITY_XC,      // External-camera Photography

} dicom_modality_enum;

typedef struct dicom_header_t {
	u8 preamble[128]; // the preamble will be all zeroes, if not used by a specific application/implementation
	union {
		char prefix[4]; // optional; should contain "DICM"
		u32 prefix_as_u32;
	};
} dicom_header_t;

#define DICOM_UNDEFINED_LENGTH 0xFFFFFFFF

#pragma pack(push,1)

typedef struct dicom_tag_t {
	union {
		struct {
			u16 group;
			u16 element;
		};
		u32 as_u32;
		dicom_tag_enum as_enum;
	};
} dicom_tag_t;

typedef struct dicom_explicit_data_element_header_t {
	dicom_tag_t tag;
	union {
		u16 vr;
		char vr_string[2];
	};
	u8 variable_part[0];
} dicom_explicit_data_element_header_t;

typedef struct dicom_implicit_data_element_header_t {
	dicom_tag_t tag;
	u32 value_length;
	u8 data[0];
} dicom_implicit_data_element_header_t;

typedef struct dicom_sequence_item_t {
	dicom_tag_t item_tag;
	u32 length;
	u8 data[0];
} dicom_sequence_item_t;
#pragma pack(pop)

typedef struct dicom_data_element_t {
	dicom_tag_t tag;
	u32 length;
	u16 vr;
	bool is_valid;
//	u8* data;
	i64 data_offset;
} dicom_data_element_t;

typedef struct dicom_da_t {
    u16 year;
    u8 month;
    u8 day;
    bool present;
} dicom_da_t;

typedef struct dicom_cs_t {
	char value[17]; // 16 bytes maximum, plus an extra byte to zero-terminate the string
    bool present;
} dicom_cs_t;

typedef struct dicom_lo_t {
    char value[65]; // 64 bytes maximum, plus an extra byte to zero-terminate the string
    bool present;
} dicom_lo_t;

typedef struct dicom_pn_t {
    dicom_lo_t components[5];
    u8 component_count;
    bool present;
} dicom_pn_t;

typedef struct dicom_sh_t {
    char value[17]; // 16 bytes maximum, plus an extra byte to zero-terminate the string
    bool present;
} dicom_sh_t;

typedef struct dicom_tm_t {
    u8 hours;
    u8 minutes;
    u8 seconds;
    bool present;
    u32 fractional_part;
} dicom_tm_t;

typedef struct dicom_ui_t {
    char value[65]; // 64 bytes maximum, plus an extra byte to zero-terminate the string
    bool present;
    u16 len;
    dicom_uid_enum as_enum;
} dicom_ui_t;

typedef struct dicom_sq_t {
	dicom_data_element_t element;
} dicom_sq_t;

// Patient module for Composite Image IODs
// https://dicom.nema.org/medical/dicom/current/output/chtml/part03/sect_C.7.html#table_C.7-1
typedef struct dicom_patient_t {
    dicom_pn_t PatientName;
    dicom_lo_t PatientID;
    dicom_da_t PatientBirthDate;
    dicom_cs_t PatientSex;
} dicom_patient_t;

typedef struct dicom_general_study_t {
    dicom_da_t StudyDate;
    dicom_tm_t StudyTime;
    dicom_sh_t AccessionNumber;
    dicom_pn_t ReferringPhysicianName;
    dicom_ui_t StudyInstanceUID;
    dicom_sh_t StudyID;
} dicom_general_study_t;

typedef struct dicom_general_series_t {
    dicom_cs_t Modality;
} dicom_general_series_t;

typedef struct dicom_optical_path_t {
    i32 illumination_type; // TODO: implement
    float illumination_wavelength;
    u8* icc_profile;
    u32 icc_profile_size;
    dicom_sh_t OpticalPathIdentifier;
} dicom_optical_path_t;

typedef struct dicom_series_t dicom_series_t; // fwd declaration
typedef struct dicom_instance_t dicom_instance_t; // fwd declaration
typedef void dicom_parser_callback_func_t(dicom_series_t* series, dicom_instance_t* instance, dicom_data_element_t element);

typedef struct dicom_parser_pos_t {
	dicom_data_element_t element;
	i64 offset;
	i64 element_index;
	i64 item_number;
} dicom_parser_pos_t;

typedef struct dicom_tile_t {
	dicom_instance_t* instance;
	u32 frame_index;
	u32 data_offset_in_file;
	u32 data_size;
    bool is_offset_known;
	bool exists;
	u8* data;
} dicom_tile_t;

// C.8.12.6.1 Plane Position (Slide) Macro
// https://dicom.nema.org/medical/dicom/current/output/chtml/part03/sect_C.8.12.6.html#sect_C.8.12.6.1
typedef struct dicom_plane_position_slide_t {
	i32 column_position_in_total_image_pixel_matrix;
	i32 row_position_in_total_image_pixel_matrix;
	v2f offset_in_slide_coordinate_system;
	float z_offset_in_slide_coordinate_system;
} dicom_plane_position_slide_t;

typedef struct dicom_instance_t {
	bool is_valid;
	i32 refcount; // TODO
	dicom_series_t* series;
	dicom_parser_callback_func_t* tag_handler_func;
	char filename[512];
	file_handle_t file_handle; // for simultaneous file access on multiple threads
	i32 nesting_level;
	dicom_parser_pos_t pos_stack[16]; // one per nesting level, for keeping track where we need to push/pop during parsing
	dicom_tag_t nested_sequences[8]; // one for every two nesting levels (sequences only, not sequence items)
	i32 nested_item_numbers[8];
//	u32 current_item_number;
	dicom_transfer_syntax_enum encoding;
    dicom_general_study_t general_study;
    dicom_general_series_t general_series;
    dicom_optical_path_t current_optical_path;
    dicom_optical_path_t* optical_paths; // array
	u8* data;
	i64 bytes_read_from_file;
	i64 total_bytes_in_stream;
	bool is_image_invalid;
	bool found_pixel_data;
	bool is_pixel_data_encapsulated;
	bool has_basic_offset_table;
	bool are_all_offsets_read;
	bool need_parse_abort;
	dicom_data_element_t pixel_data;
	u32* pixel_data_offsets; // malloc'ed
	u32* pixel_data_sizes; // malloc'ed
	u32 pixel_data_start_offset;
	u32 pixel_data_offset_count;
    dicom_ui_t sop_class_uid;
    dicom_ui_t sop_instance_uid;
    dicom_ui_t transfer_syntax_uid;
    dicom_ui_t concatenation_uid;
    u16 in_concatenation_number;
    u32 concatenation_frame_offset_number;
	bool is_image_original;
	dicom_cs_t image_flavor_cs;
	dicom_image_flavor_enum image_flavor;
	bool is_image_resampled;
	i64 instance_number;
	u16 samples_per_pixel; // 1 for monochrome and palette color images, 3 for RGB images
	dicom_photometric_interpretation_enum photometric_interpretation;
	u16 planar_configuration;
	i64 number_of_frames;
	u16 rows;
	u16 columns;
	u16 bits_allocated;
	u16 bits_stored;
	u16 high_bit;
	u16 pixel_representation;
	dicom_lossy_image_compression_method_enum lossy_image_compression_method;
	float imaged_volume_width;
	float imaged_volume_height;
	float imaged_volume_depth;
	v2f pixel_spacing;
	u32 total_pixel_matrix_columns;
	u32 total_pixel_matrix_rows;
	i32 tile_count;
	i32 width_in_tiles;
	i32 height_in_tiles;
	v2f origin_offset;
	dicom_tile_t* tiles; // malloc'ed
	dicom_plane_position_slide_t* per_frame_plane_position_slide; // array
} dicom_instance_t;

typedef struct dicom_wsi_t {
	dicom_instance_t* label_instance;
	i32 level_count;
	dicom_instance_t* level_instances[16];
	float mpp_x;
	float mpp_y;
	bool is_mpp_known;

} dicom_wsi_t;

typedef struct dicom_series_t {
	dicom_parser_callback_func_t* tag_handler_func;
	i64 bytes_read;
	dicom_transfer_syntax_enum encoding;
	FILE* debug_output_file;
	dicom_instance_t* instances; // array
	dicom_wsi_t wsi;
} dicom_series_t;


typedef struct directory_info_t directory_info_t; // from viewer.h
typedef struct file_info_t file_info_t;

bool dicom_init();
void dicom_destroy(dicom_series_t* dicom_series);
void dicom_instance_destroy(dicom_instance_t* instance);
bool is_file_a_dicom_file(u8* file_header_data, size_t file_header_data_len);
bool dicom_open_from_directory(dicom_series_t* dicom, directory_info_t* directory);
bool dicom_open_from_file(dicom_series_t* dicom, file_info_t* file);
dicom_data_element_t dicom_read_data_element(u8* data_start, i64 data_offset, dicom_transfer_syntax_enum encoding, i64 bytes_available);
dicom_ui_t dicom_parse_uid(str_t s);
i64 dicom_parse_integer_string(str_t s, str_t* next);
float dicom_parse_decimal_string(str_t s, str_t* next);
dicom_cs_t dicom_parse_code_string(str_t s, str_t* next);
dicom_sh_t dicom_parse_short_string(str_t s);
dicom_da_t dicom_parse_date(str_t s);
dicom_tm_t dicom_parse_time(str_t s);
i64 dicom_defragment_encapsulated_pixel_data_frame(u8* data, i64 len);
bool dicom_instance_index_pixel_data(dicom_instance_t* instance);

// globals
#if defined(DICOM_IMPL)
#define INIT(...) __VA_ARGS__
#define extern
#else
#define INIT(...)
#undef extern
#endif

extern const char* dicom_lossy_image_compression_method_strings[6] INIT(= {"ISO_10918_1", "ISO_14495_1", "ISO_15444_1", "ISO_13818_2", "ISO_14496_10", "ISO_23008_2"});

#undef INIT
#undef extern

#ifdef __cplusplus
}
#endif
