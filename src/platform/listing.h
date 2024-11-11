/*
  Slidescape, a whole-slide image viewer for digital pathology.
  Copyright (C) 2019-2024  Pieter Valkema

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

#ifdef __cplusplus
extern "C" {
#endif

// OS abstraction for listing directory contents
// Directory listing using dirent.h is available using MinGW on Windows, but not using MSVC (need to use Win32 API).
// - Under GNU/Linux, etc (or if compiling with MinGW on Windows), we can use dirent.h
// - Under Windows, we'd like to directly call the Win32 API. (Note: MSVC does not include dirent.h)
// NOTE: If we are using MinGW, we'll opt to use the Win32 API as well: dirent.h would just wrap Win32 anyway!

typedef struct directory_listing_t directory_listing_t;

directory_listing_t* create_directory_listing_and_find_first_file(const char* directory, const char* extension);
char* get_current_filename_from_directory_listing(directory_listing_t* data);
bool find_next_file(directory_listing_t* data);
void close_directory_listing(directory_listing_t* data);

#if defined(LISTING_IMPLEMENTATION) || defined(__JETBRAINS_IDE__)

#ifdef _WIN32

#include "win32_graphical_app.h"

struct directory_listing_t {
	WIN32_FIND_DATAW find_data;
	HANDLE search_handle;
	char current_filename_UTF8[1024];
};

directory_listing_t* create_directory_listing_and_find_first_file(const char* directory, const char* extension) {
	directory_listing_t* directory_listing = (directory_listing_t*)calloc(1, sizeof(directory_listing_t));
	char search_pattern[1024];
	// If we provide an extension, search only for files with that extension
	if (extension != NULL) {
		snprintf(search_pattern, sizeof(search_pattern), "%s/*.%s", directory, extension);
	} else {
		snprintf(search_pattern, sizeof(search_pattern), "%s/*", directory);
	}
	WCHAR search_pattern_UTF16[1024];
	win32_string_widen(search_pattern, COUNT(search_pattern_UTF16), search_pattern_UTF16);
	directory_listing->search_handle = FindFirstFileW(search_pattern_UTF16, &directory_listing->find_data);
	if (directory_listing->search_handle != INVALID_HANDLE_VALUE) {
		const wchar_t* name = directory_listing->find_data.cFileName;
		// Skip magic dirs
		while (name[0] == L'.' && (name[1] == 0 || (name[1] == '.' && name[2] == 0))) {
			FindNextFileW(directory_listing->search_handle, &directory_listing->find_data);
		}
		return directory_listing;
	} else {
		free(directory_listing);
		return NULL;
	}
}

char* get_current_filename_from_directory_listing(directory_listing_t* data) {
	return win32_string_narrow(data->find_data.cFileName, data->current_filename_UTF8, sizeof(data->current_filename_UTF8));
}

bool find_next_file(directory_listing_t* data) {
	return (bool) FindNextFileW(data->search_handle, &data->find_data);
}

void close_directory_listing(directory_listing_t* data) {
	FindClose(data->search_handle);
	free(data);
}

#else // use dirent.h API for listing files

#include <dirent.h>

struct directory_listing_t {
	DIR* dp;
	char* found_filename;
	const char* extension;
};

directory_listing_t* create_directory_listing_and_find_first_file(const char* directory, const char* extension) {
	directory_listing_t* data = (directory_listing_t*)calloc(1, sizeof(directory_listing_t));
	bool ok = false;
	data->dp = opendir(directory);
	if (data->dp != NULL) {
		struct dirent* ep;
		while ((ep = readdir(data->dp))) {
            if (ep->d_name[0] == '.') {
                continue;
            }
			if (extension == NULL) {
				data->found_filename = ep->d_name;
				data->extension = extension;
				ok = true;
				break;
			} else {
				char *ext = strrchr(ep->d_name, '.');
				if (ext != NULL && strcasecmp(ext+1, extension) == 0) {
					data->found_filename = ep->d_name;
					data->extension = extension;
					ok = true;
					break;
				}
			}
		}
	}
	if (ok) {
		return data;
	} else {
		free(data);
		return NULL;
	}
}

char* get_current_filename_from_directory_listing(directory_listing_t* data) {
	return data->found_filename;
}

bool find_next_file(directory_listing_t* data) {
	bool ok = false;
	struct dirent* ep;
	while ((ep = readdir(data->dp))) {
		if (data->extension == NULL) {
			data->found_filename = ep->d_name;
			ok = true;
			break;
		} else {
			char *ext = strrchr(ep->d_name, '.');
			if (ext != NULL && strcasecmp(ext+1, data->extension) == 0) {
				data->found_filename = ep->d_name;
				ok = true;
				break;
			}
		}
	}
	return ok;
}

void close_directory_listing(directory_listing_t *data) {
	closedir(data->dp);
	free(data);
}

#endif //LISTING_IMPLEMENTATION
#endif //_WIN32

#ifdef __cplusplus
}
#endif
