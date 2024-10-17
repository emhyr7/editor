static LRESULT CALLBACK win32_process_window_message(HWND window, UINT message, WPARAM wparam, LPARAM lparam);

struct platform
{
	HINSTANCE instance;
	STARTUPINFOW startup_info;

	uintl performance_frequency;

	HWND window;
	MSG  window_message;
} platform;

inline time get_time(void)
{
	LARGE_INTEGER counter;
	QueryPerformanceCounter(&counter);
	return counter.QuadPart / platform.performance_frequency * 1e9;
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

inline void deallocate(void *memory, uint size)
{
	(void)size;
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
		global.terminability = 1;
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
			global.terminability = 1;
			PostQuitMessage(0);
			break;
		}

	default:
		result = DefWindowProc(window, message, wparam, lparam);
	}

	return result;
}

static void initialize(void)
{
	{
		LARGE_INTEGER frequency;
		QueryPerformanceFrequency(&frequency);
		platform.performance_frequency = frequency.QuadPart;
	}

	platform.instance = GetModuleHandle(0);
	GetStartupInfoW(&platform.startup_info);

	/* create the window */
	{
		WNDCLASSEXW window_class =
		{
			.cbSize        = sizeof(window_class),
			.style         = CS_OWNDC,
			.lpfnWndProc   = win32_process_window_message,
			.hInstance     = platform.instance,
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

		platform.window = CreateWindowExW(
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
		assert(platform.window);
		ShowWindow(platform.window, SW_NORMAL);
	}
}

static void process_messages(void)
{
	while (PeekMessage(&platform.window_message, 0, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&platform.window_message);
		DispatchMessage(&platform.window_message);
	}
}
