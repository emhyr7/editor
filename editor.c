/*
 1) pragmatic `#define`s
 2) platform `#include`s
 3) external `#include`s
 4) standard `#include`s
 5) keyword
 6) constants
 7) types
 8) procedure declarations
 9) static procedures declarations
10) globals
11) procedure definitions
*/

#define _UNICODE
#define UNICODE

#if defined(_WIN32)
	#include <Windows.h>
	#include <shlwapi.h>
#endif

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>

//#include "glad.c"

#include <cglm/cglm.h>

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

typedef uint8_t  uintb;
typedef uint16_t uints;
typedef uint32_t uint;
typedef uint64_t uintl;

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

typedef uintl   time;
typedef float32 elapse;

typedef HANDLE handle;

typedef struct
{
	bit is_fragment_shader;
	utf8 path[64];
	handle handle;
	uint size;
	utf8 *source;
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
	HDC  window_device_context;

	HGLRC gl_context;
} win32;

struct
{
	uintl  frames_count_per_second;
	elapse frame_elapsed_time;
	
	uintl clock_frequency;

	bit window_is_paintable : 1;

	uint gl_program;
	uint gl_vao;
	uint gl_vbo;
	uint gl_ebo;

	bit quit_requested : 1;
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
		win32.window_device_context = GetDC(window);

		{
			PIXELFORMATDESCRIPTOR pixel_format_descriptor =
			{
				.nSize      = sizeof(pixel_format_descriptor),
				.nVersion   = 1,
				.dwFlags    = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL,
				.iPixelType = PFD_TYPE_RGBA,
				.cColorBits = 32,
			};

			int pixel_format = ChoosePixelFormat(win32.window_device_context, &pixel_format_descriptor);
			assert(pixel_format);
			assert(SetPixelFormat(win32.window_device_context, pixel_format, &pixel_format_descriptor));
			DescribePixelFormat(win32.window_device_context, pixel_format, sizeof(pixel_format_descriptor), &pixel_format_descriptor);
		}

		win32.gl_context = wglCreateContext(win32.window_device_context);
		wglMakeCurrent(win32.window_device_context, win32.gl_context);
		break;

	case WM_DESTROY:
		global.quit_requested = 1;
		PostQuitMessage(0);
		break;

	case WM_SIZE:
		glViewport(0, 0, LOWORD(lparam), HIWORD(lparam));

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

		ATOM window_class_atom = RegisterClassExW(&window_class);
		assert(window_class_atom);

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

	/* initialize OpenGL */
	{
		assert(glewInit() == GLEW_OK);
		printf("OpenGL version: %s\n", glGetString(GL_VERSION));
	}

	/* compile shaders */
	{
		printf("loading shaders...\n");
		
		shader shaders[] =
		{
			{ 0, "data/shader.vert" },
			{ 1, "data/shader.frag" },
		};

		const uint shaders_count = countof(shaders);

		uint shaders_source_size = 0;
		for (uint i = 0; i < shaders_count; ++i)
		{
			shader *shader = shaders + i;
			shader->handle = open_file(shader->path);
			shader->size = 1 + get_size_of_file(shader->handle);
			shaders_source_size += shader->size;
		}

		utf8 *shaders_source = allocate(shaders_source_size);
		utf8 *shader_source = shaders_source;
		for (uint i = 0; i < shaders_count; ++i)
		{
			shader *shader = shaders + i;
			shader->source = shader_source;
			shader_source += shader->size;
			read_from_file(shader->source, shader->size, shader->handle);
			shader->source[shader->size - 1] = 0;
			close_file(shader->handle);

			shader->reference = glCreateShader(shader->is_fragment_shader ? GL_FRAGMENT_SHADER : GL_VERTEX_SHADER);
			glShaderSource(shader->reference, 1, (const utf8 **)&shader->source, 0);
			glCompileShader(shader->reference);
			int is_successful;
			glGetShaderiv(shader->reference, GL_COMPILE_STATUS, &is_successful);
			assert(is_successful);
			printf("\tcompiled %s.\n", shader->path);
		}

		uint reference = glCreateProgram();
		for (uint i = 0; i < shaders_count; ++i)
		{
			glAttachShader(reference, shaders[i].reference);
		}
		glLinkProgram(reference);
		int is_successful;
		glGetProgramiv(reference, GL_LINK_STATUS, &is_successful);
		assert(is_successful);
		printf("\tlinked.\n");
		for (uint i = 0; i < shaders_count; ++i)
		{
			glDeleteShader(shaders[i].reference);
		}
		printf("finished loading shaders!\n");
		global.gl_program = reference;
	}

	{
		uint vao, vbo, ebo;
		glGenVertexArrays(1, &vao);
		glGenBuffers(1, &vbo);
		glGenBuffers(1, &ebo);

		glBindVertexArray(vao);

		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(global_verticies), global_verticies, GL_STATIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, ebo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(global_indicies), global_indicies, GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), 0);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);

		global.gl_vao = vao;
		global.gl_vbo = vbo;
		global.gl_ebo = ebo;
	}

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

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

		glClearColor(0.2, 0.3, 0.4, 1);
		glClear(GL_COLOR_BUFFER_BIT);
		glUseProgram(global.gl_program);
		glBindVertexArray(global.gl_vao);
		
		glDrawArrays(GL_TRIANGLES, 0, 6);
		//glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		SwapBuffers(win32.window_device_context);
	}

	return 0;
}
