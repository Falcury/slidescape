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

#pragma once

#include "common.h"
#include "mathutils.h"

#if WINDOWS
#include "windows.h"
#elif APPLE
#include <semaphore.h>
#endif

#ifdef TARGET_EMSCRIPTEN
#include <emscripten/emscripten.h>
#else
#define EMSCRIPTEN_KEEPALIVE
#endif


#ifdef __cplusplus
extern "C" {
#endif

#define MAX_THREAD_COUNT 128

typedef struct mem_t {
	size_t len;
	size_t capacity;
	u8 data[0];
} mem_t;

typedef struct memrw_t {
	u8* data;
	u64 used_size;
	u64 used_count;
	u64 capacity;
} memrw_t;

typedef void (work_queue_callback_t)(int logical_thread_index, void* userdata);

typedef struct work_queue_entry_t {
	void* data;
	work_queue_callback_t* callback;
	bool is_valid;
} work_queue_entry_t;

#if WINDOWS
typedef HANDLE semaphore_handle_t;
#elif APPLE
typedef sem_t* semaphore_handle_t;
#endif

typedef struct work_queue_t {
	semaphore_handle_t semaphore;
	i32 volatile next_entry_to_submit;
	i32 volatile next_entry_to_execute;
	i32 volatile completion_count;
	i32 volatile completion_goal;
	work_queue_entry_t entries[256];
} work_queue_t;

typedef struct platform_thread_info_t {
	i32 logical_thread_index;
	work_queue_t* queue;
} platform_thread_info_t;

typedef struct {
#if WINDOWS
	HANDLE async_io_event;
	OVERLAPPED overlapped;
#else
	// TODO: implement this
#endif
	u64 thread_memory_raw_size;
	u64 thread_memory_usable_size; // free space from aligned_rest_of_thread_memory onward
	void* aligned_rest_of_thread_memory;
	u32 pbo;
} thread_memory_t;

typedef struct button_state_t {
	bool8 down;
	u8 transition_count;
} button_state_t;

typedef struct controller_input_t {
	bool32 is_connected;
	bool32 is_analog;
	float x_start, y_start;
	float x_min, y_min;
	float x_max, y_max;
	float x_end, y_end; // end state
	union {
		button_state_t buttons[16];
		struct {
			button_state_t move_up;
			button_state_t move_down;
			button_state_t move_left;
			button_state_t move_right;
			button_state_t action_up;
			button_state_t action_down;
			button_state_t action_left;
			button_state_t action_right;
			button_state_t left_shoulder;
			button_state_t right_shoulder;
			button_state_t start;
			button_state_t back;
			button_state_t button_a;
			button_state_t button_b;
			button_state_t button_x;
			button_state_t button_y;

			// NOTE: add buttons above this line
			// cl complains about zero-sized arrays, so this the terminator is a full blown button now :(
			button_state_t terminator;
		};
	};
	button_state_t keys[256];
} controller_input_t;

typedef struct input_t {
	button_state_t mouse_buttons[5];
	i32 mouse_z;
	v2f drag_start_xy;
	v2f drag_vector;
	v2f mouse_xy;
	float delta_t;
	union {
		controller_input_t abstract_controllers[5];
		struct {
			controller_input_t keyboard;
			controller_input_t controllers[4];
		};
	};
	bool are_any_buttons_down;

} input_t;

#if WINDOWS
typedef HWND window_handle_t;
#else
typedef void* window_handle_t;
#endif

// virtual keycodes
#define KEYCODE_LBUTTON 0x01
#define KEYCODE_RBUTTON 0x02
#define KEYCODE_CANCEL 0x03
#define KEYCODE_MBUTTON 0x04
#define KEYCODE_XBUTTON1 0x05
#define KEYCODE_XBUTTON2 0x06
#define KEYCODE_BACK 0x08
#define KEYCODE_TAB 0x09
#define KEYCODE_CLEAR 0x0C
#define KEYCODE_RETURN 0x0D
#define KEYCODE_SHIFT 0x10
#define KEYCODE_CONTROL 0x11
#define KEYCODE_MENU 0x12
#define KEYCODE_PAUSE 0x13
#define KEYCODE_CAPITAL 0x14
#define KEYCODE_KANA 0x15
#define KEYCODE_HANGEUL 0x15
#define KEYCODE_HANGUL 0x15
#define KEYCODE_JUNJA 0x17
#define KEYCODE_FINAL 0x18
#define KEYCODE_HANJA 0x19
#define KEYCODE_KANJI 0x19
#define KEYCODE_ESCAPE 0x1B
#define KEYCODE_CONVERT 0x1C
#define KEYCODE_NONCONVERT 0x1D
#define KEYCODE_ACCEPT 0x1E
#define KEYCODE_MODECHANGE 0x1F
#define KEYCODE_SPACE 0x20
#define KEYCODE_PRIOR 0x21
#define KEYCODE_NEXT 0x22
#define KEYCODE_END 0x23
#define KEYCODE_HOME 0x24
#define KEYCODE_LEFT 0x25
#define KEYCODE_UP 0x26
#define KEYCODE_RIGHT 0x27
#define KEYCODE_DOWN 0x28
#define KEYCODE_SELECT 0x29
#define KEYCODE_PRINT 0x2A
#define KEYCODE_EXECUTE 0x2B
#define KEYCODE_SNAPSHOT 0x2C
#define KEYCODE_INSERT 0x2D
#define KEYCODE_DELETE 0x2E
#define KEYCODE_HELP 0x2F
#define KEYCODE_LWIN 0x5B
#define KEYCODE_RWIN 0x5C
#define KEYCODE_APPS 0x5D
#define KEYCODE_SLEEP 0x5F
#define KEYCODE_NUMPAD0 0x60
#define KEYCODE_NUMPAD1 0x61
#define KEYCODE_NUMPAD2 0x62
#define KEYCODE_NUMPAD3 0x63
#define KEYCODE_NUMPAD4 0x64
#define KEYCODE_NUMPAD5 0x65
#define KEYCODE_NUMPAD6 0x66
#define KEYCODE_NUMPAD7 0x67
#define KEYCODE_NUMPAD8 0x68
#define KEYCODE_NUMPAD9 0x69
#define KEYCODE_MULTIPLY 0x6A
#define KEYCODE_ADD 0x6B
#define KEYCODE_SEPARATOR 0x6C
#define KEYCODE_SUBTRACT 0x6D
#define KEYCODE_DECIMAL 0x6E
#define KEYCODE_DIVIDE 0x6F
#define KEYCODE_F1 0x70
#define KEYCODE_F2 0x71
#define KEYCODE_F3 0x72
#define KEYCODE_F4 0x73
#define KEYCODE_F5 0x74
#define KEYCODE_F6 0x75
#define KEYCODE_F7 0x76
#define KEYCODE_F8 0x77
#define KEYCODE_F9 0x78
#define KEYCODE_F10 0x79
#define KEYCODE_F11 0x7A
#define KEYCODE_F12 0x7B
#define KEYCODE_F13 0x7C
#define KEYCODE_F14 0x7D
#define KEYCODE_F15 0x7E
#define KEYCODE_F16 0x7F
#define KEYCODE_F17 0x80
#define KEYCODE_F18 0x81
#define KEYCODE_F19 0x82
#define KEYCODE_F20 0x83
#define KEYCODE_F21 0x84
#define KEYCODE_F22 0x85
#define KEYCODE_F23 0x86
#define KEYCODE_F24 0x87
#define KEYCODE_NUMLOCK 0x90
#define KEYCODE_SCROLL 0x91
#define KEYCODE_OEM_NEC_EQUAL 0x92
#define KEYCODE_OEM_FJ_JISHO 0x92
#define KEYCODE_OEM_FJ_MASSHOU 0x93
#define KEYCODE_OEM_FJ_TOUROKU 0x94
#define KEYCODE_OEM_FJ_LOYA 0x95
#define KEYCODE_OEM_FJ_ROYA 0x96
#define KEYCODE_LSHIFT 0xA0
#define KEYCODE_RSHIFT 0xA1
#define KEYCODE_LCONTROL 0xA2
#define KEYCODE_RCONTROL 0xA3
#define KEYCODE_LMENU 0xA4
#define KEYCODE_RMENU 0xA5
#define KEYCODE_BROWSER_BACK 0xA6
#define KEYCODE_BROWSER_FORWARD 0xA7
#define KEYCODE_BROWSER_REFRESH 0xA8
#define KEYCODE_BROWSER_STOP 0xA9
#define KEYCODE_BROWSER_SEARCH 0xAA
#define KEYCODE_BROWSER_FAVORITES 0xAB
#define KEYCODE_BROWSER_HOME 0xAC
#define KEYCODE_VOLUME_MUTE 0xAD
#define KEYCODE_VOLUME_DOWN 0xAE
#define KEYCODE_VOLUME_UP 0xAF
#define KEYCODE_MEDIA_NEXT_TRACK 0xB0
#define KEYCODE_MEDIA_PREV_TRACK 0xB1
#define KEYCODE_MEDIA_STOP 0xB2
#define KEYCODE_MEDIA_PLAY_PAUSE 0xB3
#define KEYCODE_LAUNCH_MAIL 0xB4
#define KEYCODE_LAUNCH_MEDIA_SELECT 0xB5
#define KEYCODE_LAUNCH_APP1 0xB6
#define KEYCODE_LAUNCH_APP2 0xB7
#define KEYCODE_OEM_1 0xBA
#define KEYCODE_OEM_PLUS 0xBB
#define KEYCODE_OEM_COMMA 0xBC
#define KEYCODE_OEM_MINUS 0xBD
#define KEYCODE_OEM_PERIOD 0xBE
#define KEYCODE_OEM_2 0xBF
#define KEYCODE_OEM_3 0xC0
#define KEYCODE_OEM_4 0xDB
#define KEYCODE_OEM_5 0xDC
#define KEYCODE_OEM_6 0xDD
#define KEYCODE_OEM_7 0xDE
#define KEYCODE_OEM_8 0xDF
#define KEYCODE_OEM_AX 0xE1
#define KEYCODE_OEM_102 0xE2
#define KEYCODE_ICO_HELP 0xE3
#define KEYCODE_ICO_00 0xE4
#define KEYCODE_PROCESSKEY 0xE5
#define KEYCODE_ICO_CLEAR 0xE6
#define KEYCODE_PACKET 0xE7
#define KEYCODE_OEM_RESET 0xE9
#define KEYCODE_OEM_JUMP 0xEA
#define KEYCODE_OEM_PA1 0xEB
#define KEYCODE_OEM_PA2 0xEC
#define KEYCODE_OEM_PA3 0xED
#define KEYCODE_OEM_WSCTRL 0xEE
#define KEYCODE_OEM_CUSEL 0xEF
#define KEYCODE_OEM_ATTN 0xF0
#define KEYCODE_OEM_FINISH 0xF1
#define KEYCODE_OEM_COPY 0xF2
#define KEYCODE_OEM_AUTO 0xF3
#define KEYCODE_OEM_ENLW 0xF4
#define KEYCODE_OEM_BACKTAB 0xF5
#define KEYCODE_ATTN 0xF6
#define KEYCODE_CRSEL 0xF7
#define KEYCODE_EXSEL 0xF8
#define KEYCODE_EREOF 0xF9
#define KEYCODE_PLAY 0xFA
#define KEYCODE_ZOOM 0xFB
#define KEYCODE_NONAME 0xFC
#define KEYCODE_PA1 0xFD
#define KEYCODE_OEM_CLEAR 0xFE





// Platform specific function prototypes

i64 get_clock();
float get_seconds_elapsed(i64 start, i64 end);
void platform_sleep(u32 ms);
i64 profiler_end_section(i64 start, const char* name, float report_threshold_ms);
void set_swap_interval(int interval);

u8* platform_alloc(size_t size); // required to be zeroed by the platform
mem_t* platform_allocate_mem_buffer(size_t capacity);
mem_t* platform_read_entire_file(const char* filename);
u64 file_read_at_offset(void* dest, FILE* fp, u64 offset, u64 num_bytes);

void mouse_show();
void mouse_hide();

void open_file_dialog(window_handle_t window_handle);
bool save_file_dialog(window_handle_t window, char* path_buffer, i32 path_buffer_size, const char* filter_string);
void toggle_fullscreen(window_handle_t window);
bool check_fullscreen(window_handle_t window);

void message_box(const char* message);

bool add_work_queue_entry(work_queue_t* queue, work_queue_callback_t callback, void* userdata);
bool is_queue_work_in_progress(work_queue_t* queue);
work_queue_entry_t get_next_work_queue_entry(work_queue_t* queue);
void mark_queue_entry_completed(work_queue_t* queue);
bool do_worker_work(work_queue_t* queue, int logical_thread_index);
void test_multithreading_work_queue();

bool file_exists(const char* filename);

void memrw_maybe_grow(memrw_t* buffer, u64 new_size);
u64 memrw_push(memrw_t* buffer, void* data, u64 size);
void memrw_init(memrw_t* buffer, u64 capacity);
memrw_t memrw_create(u64 capacity);
void memrw_rewind(memrw_t* buffer);
void memrw_destroy(memrw_t* buffer);

#if IS_SERVER
#define console_print printf
#define console_print_error(...) fprintf(stderr, __VA_ARGS__)
#else
void console_print(const char* fmt, ...); // defined in gui.cpp
void console_print_error(const char* fmt, ...);
#endif


// globals
#if defined(PLATFORM_IMPL)
#define INIT(...) __VA_ARGS__
#define extern
#else
#define INIT(...)
#undef extern
#endif

extern int g_argc;
extern const char** g_argv;
extern bool is_fullscreen;
extern bool is_program_running;
extern void* thread_local_storage[MAX_THREAD_COUNT];
extern input_t inputs[2];
extern input_t *old_input;
extern input_t *curr_input;
extern u32 os_page_size;
extern i32 total_thread_count;
extern i32 worker_thread_count;
extern i32 physical_cpu_count;
extern i32 logical_cpu_count;
extern bool is_vsync_enabled;
extern bool is_nvidia_gpu;
extern bool is_macos;
extern work_queue_t global_work_queue;
extern work_queue_t global_completion_queue;

#undef INIT
#undef extern

#ifdef __cplusplus
}
#endif

