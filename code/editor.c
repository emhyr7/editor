#define _UNICODE
#define UNICODE

#if defined(_WIN32)
	#include <Windows.h>
	#include <shlwapi.h>
#endif

#include "cglm/cglm.h"

#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#include "stb/stb_truetype.h"

#define time c_time
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <tchar.h>
#include <threads.h>
#include <wchar.h>
#undef time

#define __FUNCTION__ __func__

#define countof(...) _countof(__VA_ARGS__)

#define PROGRAM_TITLE L"EDITOR"

#define WM_GMH_EXISTS (WM_USER + 1)

typedef uint8_t  uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef uint8  uintb;
typedef uint16 uints;
typedef uint32 uint;
typedef uint64 uintl;

typedef int8_t  sintb;
typedef int16_t sints;
typedef int32_t sint;
typedef int64_t sintl;

typedef float  float32;
typedef double float64;

typedef wchar_t wchar;

typedef char     utf8;
typedef wchar_t  utf16;
typedef uint32_t utf32;

typedef uintb byte;
typedef byte  bit;

typedef uint64 bit64;

typedef uintl   time;
typedef float32 elapse;

typedef HANDLE handle;

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

time   get_time(void);
time   begin_clock(void);
elapse end_clock(void);

void *allocate(uint size);
void release(void *memory);

handle open_file(const char *path);
uintl get_size_of_file(handle handle);
uint read_from_file(void *buffer, uint size, handle handle);
void close_file(handle handle);

static LRESULT CALLBACK win32_process_window_message(HWND window, UINT message, WPARAM wparam, LPARAM lparam);

static void render_frame(void);

struct
{
	HINSTANCE    instance;
	LPWSTR       command_line;
	STARTUPINFOW startup_info;

	HWND window;
	MSG  window_message;
} win32;

const char default_font_file_path[] = "./data/consola.ttf";

struct
{
	uintl  frames_count_per_second;
	elapse frame_elapsed_time;
	
	uintl clock_frequency;

	bit quit_requested : 1;

	font default_font;
} global;

static vertex global_verticies[] =
{
	{  0.5,  0.5, 0 },
	{  0.5, -0.5, 0 },
	{ -0.5, -0.5, 0 },
	{ -0.5,  0.5, 0 },
};
static ivec3 global_indicies[] =
{
	{ 0, 1, 3 },
	{ 1, 2, 3 },
};

thread_local struct
{
	uintl clock_time;
} context;

static const utf32 initial_runes_of_font[] = { 'A', 'B' };

static void load_font(const char *font_file_path, uintb font_index, font *font)
{
	font->file = open_file(font_file_path);
	font->data_size = get_size_of_file(font->file);
	font->data = allocate(font->data_size);
	read_from_file(font->data, font->data_size, font->file);

	stbtt_InitFont(&font->info, font->data, stbtt_GetFontOffsetForIndex(font->data, font_index));
	font->scale = stbtt_ScaleForPixelHeight(&font->info, FONT_DEFAULT_HEIGHT);
	int right, left, top, base;
	stbtt_GetCodepointBitmapBox(&font->info, 'W', font->scale, font->scale, &left, &top, &right, &base);
	font->glyph_width = right - left;
	font->glyph_size = (base - top) * font->glyph_width;
	font->glyphs = allocate(FONT_GLYPHS_CAPACITY * font->glyph_size);
	for (uint i = 0; i < countof(initial_runes_of_font); ++i)
	{
		byte *glyph = font->glyphs + (i * font->glyph_size);
		stbtt_MakeCodepointBitmap(
			&font->info,
			glyph,
			font->glyph_width,
			font->glyph_size / font->glyph_width,
			font->glyph_width,
			font->scale,
			font->scale,
			initial_runes_of_font[i]);
	}
	zero(font->glyphs_flags, sizeof(font->glyphs_flags));
}

static void unload_font(font *font)
{
	release(font->glyphs);
	close_file(font->file);
}

inline void copy(void *destination, const void *memory, uint size)
{
	CopyMemory(destination, memory, size);
}

inline void move(void *destination, const void *memory, uint size)
{
	MoveMemory(destination, memory, size);
}

inline void fill(void *memory, uint size, uintb value)
{
	FillMemory(memory, size, value);
}

inline void zero(void *memory, uint size)
{
	ZeroMemory(memory, size);
}

inline time get_time(void)
{
	LARGE_INTEGER counter;
	QueryPerformanceCounter(&counter);
	return counter.QuadPart;
}

inline time begin_clock(void)
{
	return context.clock_time = get_time();
}

inline elapse end_clock(void)
{
	elapse elapsed_time = (elapse)(get_time() - context.clock_time) * 1000000 / global.clock_frequency;
	return elapsed_time;
}

inline void *allocate(uint size)
{
	void *memory = VirtualAlloc(0, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	if (!memory)
	{
		goto failed;
	}

	return memory;

failed:
	fprintf(stderr, "%s: Win32's last error: %li\n", __FUNCTION__, GetLastError());
	abort();
}

inline void release(void *memory)
{
	VirtualFree(memory, 0, MEM_RELEASE);
}

inline handle open_file(const char *path)
{
	handle handle = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (handle == INVALID_HANDLE_VALUE)
	{
		goto failed;
	}

	return handle;

failed:
	fprintf(stderr, "%s: Win32's last error: %li\n", __FUNCTION__, GetLastError());
	abort();
}

inline uintl get_size_of_file(handle handle)
{
	LARGE_INTEGER file_size;
	if (!GetFileSizeEx(handle, &file_size))
	{
		goto failed;
	}

	return file_size.QuadPart;

failed:
	fprintf(stderr, "%s: Win32's last error: %li\n", __FUNCTION__, GetLastError());
	abort();
}

inline uint read_from_file(void *buffer, uint size, handle handle)
{
	DWORD bytes_read_count;
	if (!ReadFile(handle, buffer, size, &bytes_read_count, 0))
	{
		goto failed;
	}

	return bytes_read_count;

failed:
	fprintf(stderr, "%s: Win32's last error: %li\n", __FUNCTION__, GetLastError());
	abort();
}

inline void close_file(handle handle)
{
	CloseHandle(handle);
}

LRESULT CALLBACK win32_process_window_message(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
	LRESULT result = 0;

	switch (message)
	{
	case WM_GMH_EXISTS:
		return 0;

	case WM_CREATE:
		break;

	case WM_DESTROY:
		global.quit_requested = 1;
		PostQuitMessage(0);
		break;

	case WM_SIZE:
	case WM_PAINT:
		{
			PAINTSTRUCT paint_struct;
			BeginPaint(window, &paint_struct);
			EndPaint(window, &paint_struct);
		}

		break;

	case WM_KEYDOWN:
		switch (wparam)
		{
		case VK_ESCAPE:
			global.quit_requested = 1;
			PostQuitMessage(0);
			break;
		}

	default:
		result = DefWindowProc(window, message, wparam, lparam);
	}

	return result;
}

void render_frame(void)
{
}

int main(void)
{
	{
		LARGE_INTEGER frequency;
		QueryPerformanceFrequency(&frequency);
		global.clock_frequency = frequency.QuadPart;
	}

	time beginning_time = get_time();

	win32.instance = GetModuleHandle(0);
	win32.command_line = GetCommandLineW();
	GetStartupInfoW(&win32.startup_info);

	/* create the window */
	{
		WNDCLASSEXW window_class =
		{
			.cbSize        = sizeof(window_class),
			.style         = CS_OWNDC,
			.lpfnWndProc   = win32_process_window_message,
			.hInstance     = win32.instance,
			.lpszClassName = PROGRAM_TITLE"_window_class",
		};

		{
			HWND existing = FindWindowW(window_class.lpszClassName, 0);
			if (existing)
			{
				/* TODO: notify the user that the main window already exists, and that the
				   current process will terminate. */
				PostMessageW(existing, WM_GMH_EXISTS, 0, 0);
				ExitProcess(0);
			}
		}

		assert(RegisterClassExW(&window_class));

		win32.window = CreateWindowExW(
			WS_EX_OVERLAPPEDWINDOW,
			window_class.lpszClassName,
			PROGRAM_TITLE,
			WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			0,
			0,
			window_class.hInstance,
			0);
		assert(win32.window);
		ShowWindow(win32.window, SW_NORMAL);
	}

	/* compile shaders */
#if 0
	{
		printf("loading shaders...\n");
		
		shader shaders[] =
		{
			{ 0, "code/shader.vert" },
			{ 1, "code/shader.frag" },
		};

		const uint shaders_count = countof(shaders);

		uint shaders_data_size = 0;
		for (uint i = 0; i < shaders_count; ++i)
		{
			shader *shader = shaders + i;
			shader->handle = open_file(shader->path);
			shader->size = 1 + get_size_of_file(shader->handle);
			shaders_data_size += shader->size;
		}

		char *shaders_data = allocate(shaders_data_size);
		char *shader_data = shaders_data;
		for (uint i = 0; i < shaders_count; ++i)
		{
			shader *shader = shaders + i;
			shader->data = shader_data;
			shader_data += shader->size;
			read_from_file(shader->data, shader->size, shader->handle);
			shader->data[shader->size - 1] = 0;
			close_file(shader->handle);
		}
	}
#endif

	load_font(default_font_file_path, 0, &global.default_font);

	global.frames_count_per_second = 0;
	elapse elapsed_time_per_second = 0;
	while (!global.quit_requested)
	{
		{
			if (elapsed_time_per_second >= 1000000)
			{
				printf("FPS: %llu\n", global.frames_count_per_second);
				global.frames_count_per_second = 0;
				elapsed_time_per_second = 0;
			}
			uintl ending_time = get_time();
			global.frame_elapsed_time = (elapse)(ending_time - beginning_time) * 1000000 / global.clock_frequency;
			elapsed_time_per_second += global.frame_elapsed_time;
			global.frames_count_per_second += 1;
			beginning_time = ending_time;
		}

		while (PeekMessage(&win32.window_message, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&win32.window_message);
			DispatchMessage(&win32.window_message);
		}
	}

	return 0;
}
