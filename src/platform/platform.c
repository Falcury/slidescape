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
#define STB_DS_IMPLEMENTATION
#include "common.h"

#define PLATFORM_IMPL
#include "platform.h"
#include "intrinsics.h"

#if !IS_SERVER
#include "work_queue.c"
#endif

#if APPLE
#include <sys/sysctl.h> // for sysctlbyname()
#endif


mem_t* platform_allocate_mem_buffer(size_t capacity) {
	size_t allocation_size = sizeof(mem_t) + capacity + 1;
	mem_t* result = (mem_t*) malloc(allocation_size);
	result->len = 0;
	result->capacity = capacity;
	return result;
}

mem_t* platform_read_entire_file(const char* filename) {
	mem_t* result = NULL;
	file_stream_t fp = file_stream_open_for_reading(filename);
	if (fp) {
		i64 filesize = file_stream_get_filesize(fp);
		if (filesize > 0) {
			size_t allocation_size = sizeof(mem_t) + filesize + 1;
			result = (mem_t*) malloc(allocation_size);
			if (result) {
				((u8*)result)[allocation_size-1] = '\0';
				result->len = filesize;
				result->capacity = filesize;
				size_t bytes_read = file_stream_read(result->data, filesize, fp);
				if (bytes_read != filesize) {
					panic();
				}
			}
		}
		file_stream_close(fp);
	}
	return result;
}


u64 file_read_at_offset(void* dest, file_stream_t fp, u64 offset, u64 num_bytes) {
	i64 prev_read_pos = file_stream_get_pos(fp);
	file_stream_set_pos(fp, offset);
	u64 result = file_stream_read(dest, num_bytes, fp);
	file_stream_set_pos(fp, prev_read_pos);
	return result;
}

bool file_exists(const char* filename) {
	return (access(filename, F_OK) != -1);
}

void memrw_maybe_grow(memrw_t* buffer, u64 new_size) {
	if (new_size > buffer->capacity) {
		u64 new_capacity = next_pow2(new_size);
		void* new_ptr = realloc(buffer->data, new_capacity);
		if (!new_ptr) panic();
		buffer->data = new_ptr;
#if DO_DEBUG
		console_print_verbose("memrw_maybe_grow(): expanded buffer size from %u to %u\n", buffer->capacity, new_capacity);
#endif
		buffer->capacity = new_capacity;
	}
}

// TODO: do we actually want this to be a dynamic array, or a read/write stream?
u64 memrw_push_back(memrw_t* buffer, void* data, u64 size) {
	u64 new_size = buffer->used_size + size;
	memrw_maybe_grow(buffer, new_size);
	u64 write_offset = buffer->used_size;
	void* write_pos = buffer->data + write_offset;
	if (data) {
		memcpy(write_pos, data, size);
	} else {
		memset(write_pos, 0, size);
	}
	buffer->used_size += size;
	buffer->cursor = buffer->used_size;
	buffer->used_count += 1;
	return write_offset;
}

void memrw_init(memrw_t* buffer, u64 capacity) {
	memset(buffer, 0, sizeof(*memset));
	buffer->data = (u8*) malloc(capacity);
	buffer->capacity = capacity;
}

memrw_t memrw_create(u64 capacity) {
	memrw_t result = {};
	memrw_init(&result, capacity);
	return result;
}

void memrw_rewind(memrw_t* buffer) {
	buffer->used_size = 0;
	buffer->used_count = 0;
	buffer->cursor = 0;
}

void memrw_seek(memrw_t* buffer, i64 offset) {
	if (offset >= 0 && (i64)offset < buffer->used_size) {
		buffer->cursor = offset;
	} else {
		panic();
	};
}

i64 memrw_write(const void* src, memrw_t* buffer, i64 bytes_to_write) {
	ASSERT(bytes_to_write >= 0);
	memrw_maybe_grow(buffer, buffer->cursor + bytes_to_write);
	i64 bytes_left = buffer->capacity - buffer->cursor;
	if (bytes_left >= 1) {
		bytes_to_write = MIN(bytes_to_write, bytes_left);
		memcpy(buffer->data + buffer->cursor, src, bytes_to_write);
		buffer->cursor += bytes_to_write;
		buffer->used_size = MAX(buffer->cursor, (i64)buffer->used_size);
		return bytes_to_write;
	}
	return 0;
}

i64 memrw_write_string(const char* s, memrw_t* buffer) {
	size_t len = strlen(s);
	return memrw_write(s, buffer, len);
}

i64 memrw_read(void* dest, memrw_t* buffer, size_t bytes_to_read) {
	i64 bytes_left = buffer->used_size - buffer->cursor;
	if (bytes_left >= 1) {
		bytes_to_read = MIN(bytes_to_read, (size_t)bytes_left);
		memcpy(dest, buffer->data + buffer->cursor, bytes_to_read);
		buffer->cursor += bytes_to_read;
		return bytes_to_read;
	}
	return 0;
}

void memrw_destroy(memrw_t* buffer) {
	if (buffer->data) free(buffer->data);
	memset(buffer, 0, sizeof(*buffer));
}

void get_system_info() {
#if WINDOWS
    SYSTEM_INFO system_info;
    GetSystemInfo(&system_info);
	logical_cpu_count = (i32)system_info.dwNumberOfProcessors;
	physical_cpu_count = logical_cpu_count; // TODO: how to read this on Windows?
	os_page_size = system_info.dwPageSize;
#elif APPLE
    size_t physical_cpu_count_len = sizeof(physical_cpu_count);
	size_t logical_cpu_count_len = sizeof(logical_cpu_count);
	sysctlbyname("hw.physicalcpu", &physical_cpu_count, &physical_cpu_count_len, NULL, 0);
	sysctlbyname("hw.logicalcpu", &logical_cpu_count, &logical_cpu_count_len, NULL, 0);
	os_page_size = (u32) getpagesize();
	page_alignment_mask = ~((u64)(sysconf(_SC_PAGE_SIZE) - 1));
	is_macos = true;
#elif LINUX
    logical_cpu_count = sysconf( _SC_NPROCESSORS_ONLN );
    physical_cpu_count = logical_cpu_count; // TODO: how to read this on Linux?
    os_page_size = (u32) getpagesize();
    page_alignment_mask = ~((u64)(sysconf(_SC_PAGE_SIZE) - 1));
#endif
    console_print("There are %d logical CPU cores\n", logical_cpu_count);
    total_thread_count = MIN(logical_cpu_count, MAX_THREAD_COUNT);
}


#if !IS_SERVER

//TODO: move this
bool profiling = false;

i64 profiler_end_section(i64 start, const char* name, float report_threshold_ms) {
	i64 end = get_clock();
	if (profiling) {
		float ms_elapsed = get_seconds_elapsed(start, end) * 1000.0f;
		if (ms_elapsed > report_threshold_ms) {
			console_print("[profiler] %s: %g ms\n", name, ms_elapsed);
		}
	}
	return end;
}
#endif

// Based on:
// https://preshing.com/20120226/roll-your-own-lightweight-mutex/


benaphore_t benaphore_create(void) {
	benaphore_t result = {};
#if WINDOWS
	result.semaphore = CreateSemaphore(NULL, 0, 1, NULL);
#elif (APPLE || LINUX)
	static i32 counter = 1;
	char semaphore_name[64];
	i32 c = atomic_increment(&counter);
	snprintf(semaphore_name, sizeof(semaphore_name)-1, "/benaphore%d", c);
	result.semaphore = sem_open(semaphore_name, O_CREAT, 0644, 0);
#endif
	return result;
}

void benaphore_destroy(benaphore_t* benaphore) {
#if WINDOWS
	CloseHandle(benaphore->semaphore);
#elif (APPLE || LINUX)
	sem_close(benaphore->semaphore);
#endif
}

void benaphore_lock(benaphore_t* benaphore) {
	if (atomic_increment(&benaphore->counter) > 1) {
		semaphore_wait(benaphore->semaphore);
	}
}

void benaphore_unlock(benaphore_t* benaphore) {
	if (atomic_decrement(&benaphore->counter) > 0) {
		semaphore_post(benaphore->semaphore);
	}
}

// Performance considerations for async I/O on Linux:
// https://github.com/littledan/linux-aio#performance-considerations

// TODO: aiocb64 doesn't exist on macOS? need #defines?



/*void async_read_submit(io_operation_t* op) {
#if WINDOWS
	// stub
#elif (APPLE || LINUX)
	memset(&op->cb, 0, sizeof(op->cb));
	op->cb.aio_nbytes = op->size_to_read;
	op->cb.aio_fildes = op->file;
	op->cb.aio_offset = op->offset;
	op->cb.aio_buf = op->dest;

	if (aio_read(&op->cb) == -1) {
		console_print_error("submit_async_read(): unable to create I/O request\n");
	}
#endif
}

bool async_read_has_finished(io_operation_t* op) {
#if WINDOWS
	// stub
#elif (APPLE || LINUX)
	int status = aio_error(&op->cb);
	return (status != EINPROGRESS);
#endif
}

i64 async_read_finalize(io_operation_t* op) {
#if WINDOWS
	// stub
#elif (APPLE || LINUX)
	i64 bytes_read = aio_return(&op->cb);
	return bytes_read;
#endif
}*/



/*
void* block_allocator_proc(allocator_t* this_allocator, size_t size_to_allocate, u32 mode, void* ptr_to_free_or_realloc) {
	ASSERT(this_allocator);
	void* result = NULL;
	switch(mode) {
		default: break;
		case ALLOCATOR_MODE_ALLOC: {

		} break;
		case ALLOCATOR_MODE_REALLOC: {

		} break;
		case ALLOCATOR_MODE_FREE: {

		} break;
	}
}
*/

block_allocator_t block_allocator_create(size_t block_size, size_t max_capacity_in_blocks, size_t chunk_size) {
	size_t total_capacity = block_size * max_capacity_in_blocks;
	size_t chunk_count = total_capacity / chunk_size;
	size_t chunk_capacity_in_blocks = max_capacity_in_blocks / chunk_count;
	block_allocator_t result = {};
	result.block_size = block_size;
	result.chunk_capacity_in_blocks = chunk_capacity_in_blocks;
	result.chunk_size = chunk_size;
	ASSERT(chunk_count > 0);
	result.chunk_count = chunk_count;
	result.used_chunks = 1;
	result.chunks = calloc(1, chunk_count * sizeof(block_allocator_chunk_t));
	result.chunks[0].memory = (u8*)malloc(chunk_size);
	result.free_list_storage = calloc(1, max_capacity_in_blocks * sizeof(block_allocator_item_t));
	result.lock = benaphore_create();
	result.is_valid = true;
	return result;
}

void block_allocator_destroy(block_allocator_t* allocator) {
	for (i32 i = 0; i < allocator->used_chunks; ++i) {
		block_allocator_chunk_t* chunk = allocator->chunks + i;
		if (chunk->memory) free(chunk->memory);
	}
	if (allocator->chunks) free(allocator->chunks);
	if (allocator->free_list_storage) free(allocator->free_list_storage);
	benaphore_destroy(&allocator->lock);
	memset(allocator, 0, sizeof(block_allocator_t));
}

void* block_alloc(block_allocator_t* allocator) {
	void* result = NULL;
	benaphore_lock(&allocator->lock);
	if (allocator->free_list != NULL) {
		// Grab a block from the free list
		block_allocator_item_t* free_item = allocator->free_list;
		result = allocator->chunks[free_item->chunk_index].memory + free_item->block_index * allocator->block_size;
		allocator->free_list = free_item->next;
		--allocator->free_list_length;
	} else {
		ASSERT(allocator->used_chunks >= 1);
		i32 chunk_index = allocator->used_chunks-1;
		block_allocator_chunk_t* current_chunk = allocator->chunks + chunk_index;
		if (current_chunk->used_blocks < allocator->chunk_capacity_in_blocks) {
			i32 block_index = current_chunk->used_blocks++;
			result = current_chunk->memory + block_index * allocator->block_size;
		} else {
			// Chunk is full, allocate a new chunk
			if (allocator->used_chunks < allocator->chunk_count) {
//				console_print("block_alloc(): allocating a new chunk\n");
				chunk_index = allocator->used_chunks++;
				current_chunk = allocator->chunks + chunk_index;
				ASSERT(current_chunk->memory == NULL);
				current_chunk->memory = (u8*)malloc(allocator->chunk_size);
				i32 block_index = current_chunk->used_blocks++;
				result = current_chunk->memory + block_index * allocator->block_size;
			} else {
				console_print_error("block_alloc(): out of memory!\n");
				panic();
			}
		}
	}
	benaphore_unlock(&allocator->lock);
	return result;
}

void block_free(block_allocator_t* allocator, void* ptr_to_free) {
	benaphore_lock(&allocator->lock);
	block_allocator_item_t free_item = {};
	// Find the right chunk
	i32 chunk_index = -1;
	for (i32 i = 0; i < allocator->used_chunks; ++i) {
		block_allocator_chunk_t* chunk = allocator->chunks + i;
		bool match = ((u8*)ptr_to_free >= chunk->memory && (u8*)ptr_to_free < (chunk->memory + allocator->chunk_size));
		if (match) {
			chunk_index = i;
			break;
		}
	}
	if (chunk_index >= 0) {
		block_allocator_chunk_t* chunk = allocator->chunks + chunk_index;
		free_item.next = allocator->free_list;
		free_item.chunk_index = chunk_index;
		free_item.block_index = ((u8*)ptr_to_free - chunk->memory) / allocator->block_size;
		i32 free_index = allocator->free_list_length++;
		allocator->free_list_storage[free_index] = free_item;
		allocator->free_list = allocator->free_list_storage + free_index;
		benaphore_unlock(&allocator->lock);
	} else {
		console_print_error("block_free(): invalid pointer!\n");
		panic();
	}

}

void init_thread_memory(i32 logical_thread_index) {
	// Allocate a private memory buffer
	u64 thread_memory_size = MEGABYTES(16);
	local_thread_memory = (thread_memory_t*) platform_alloc(thread_memory_size); // how much actually needed?
	thread_memory_t* thread_memory = local_thread_memory;
	memset(thread_memory, 0, sizeof(thread_memory_t));
#if !WINDOWS
	// TODO: implement creation of async I/O events
#endif
	thread_memory->thread_memory_raw_size = thread_memory_size;

	thread_memory->aligned_rest_of_thread_memory = (void*)
			((((u64)thread_memory + sizeof(thread_memory_t) + os_page_size - 1) / os_page_size) * os_page_size); // round up to next page boundary
	thread_memory->thread_memory_usable_size = thread_memory_size - ((u64)thread_memory->aligned_rest_of_thread_memory - (u64)thread_memory);
	init_arena(&thread_memory->temp_arena, thread_memory->thread_memory_usable_size, thread_memory->aligned_rest_of_thread_memory);

}
