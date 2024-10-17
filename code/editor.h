#pragma once

#if defined(_WIN32)
	#include "editor_win32.h"
#elif defined(__linux__)
	#include "editor_linux.h"
#endif

#include "cglm/cglm.h"

#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#include "stb/stb_truetype.h"

#define time c_time
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <threads.h>
#include <wchar.h>
#undef time

#define __FUNCTION__ __func__

#define countof(x) (sizeof(x) / sizeof(x[0]))

#define PROGRAM_TITLE L"EDITOR"

#define WM_GMH_EXISTS (WM_USER + 1)

typedef uint8_t  uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t  sint8;
typedef int16_t sint16;
typedef int32_t sint32;
typedef int64_t sint64;

typedef uint8  uintb;
typedef uint16 uints;
typedef uint32 uint;
typedef uint64 uintl;

typedef sint8  sintb;
typedef sint16 sints;
typedef sint32 sint;
typedef sint64 sintl;

typedef float  float32;
typedef double float64;

typedef wchar_t wchar;

typedef char     utf8;
typedef wchar_t  utf16;
typedef uint32_t utf32;

typedef uintb  byte;
typedef byte   bit;
typedef uint8  bit8;
typedef uint16 bit16;
typedef uint32 bit32;
typedef uint64 bit64;

#define FONT_GLYPHS_CAPACITY 128
#define FONT_DEFAULT_HEIGHT  16

typedef struct
{
	handle file;
	uint data_size;
	byte *data;

	stbtt_fontinfo info;

	float32 scale;

	uint glyph_width;
	uint glyph_size;
	byte *glyphs;
	bit64 glyphs_flags[2];
} font;

void load_font(const char *file_path, uintb font_index, font *font);

void unload_font(font *font);

typedef struct
{
	bit is_fragment_shader;
	char path[64];
	handle handle;
	uint size;
	char *data;
	uint reference;
} shader;

typedef vec3 vertex;

void copy(void *destination, const void *memory, uint size);
void move(void *destination, const void *memory, uint size);
void fill(void *memory, uint size, uintb value);
void zero(void *memory, uint size);

typedef uintl time; /* in nanoseconds */

#define TIME_SECONDS_FACTOR 1000000000

time get_time(void);

time begin_clock(void);
time end_clock(void);

void *allocate(uint size);
void deallocate(void *memory, uint size);

handle open_file(const char *path);
uintl get_size_of_file(handle handle);
uint read_from_file(void *buffer, uint size, handle handle);
void close_file(handle handle);