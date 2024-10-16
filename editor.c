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

#include <GL/gl.h>
#include <GL/glu.h>

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <tchar.h>
#include <wchar.h>

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

void copy(void *destination, const void *memory, uint size);
void move(void *destination, const void *memory, uint size);
void fill(void *memory, uint size, uintb value);
void zero(void *memory, uint size);

static LRESULT CALLBACK win32_process_window_message(HWND window, UINT message, WPARAM wparam, LPARAM lparam);

static void render(void);

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
	bit quit_requested : 1;
} global;

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
		render();

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

void render(void)
{
	glClear(GL_COLOR_BUFFER_BIT);
	glBegin(GL_TRIANGLES);
	{
		glColor3f(1.0f, 0.0f, 0.0f);
		glVertex2i(0,  1);
		glColor3f(0.0f, 1.0f, 0.0f);
		glVertex2i(-1, -1);
		glColor3f(0.0f, 0.0f, 1.0f);
		glVertex2i(1, -1);
	}
	glEnd();
	glFlush();
}

int main(void)
{
	win32.instance = GetModuleHandle(0);
	win32.command_line = GetCommandLineW();
	GetStartupInfoW(&win32.startup_info);

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

	while (!global.quit_requested)
	{
		while (PeekMessage(&win32.window_message, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&win32.window_message);
			DispatchMessage(&win32.window_message);
		}
	}

	return 0;
}
