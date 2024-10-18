#define WM_USER_ALREADYEXISTS (WM_USER + 1)

static LRESULT CALLBACK win32_process_window_message(HWND window, UINT message, WPARAM wparam, LPARAM lparam);

static void win32_initialize_vulkan(void);

struct win32
{
	HINSTANCE instance;
	STARTUPINFOW startup_info;

	uintl performance_frequency;

	HWND window;
	MSG  window_message;
} win32;

inline uintl get_time(void)
{
	LARGE_INTEGER counter;
	QueryPerformanceCounter(&counter);
	return counter.QuadPart / win32.performance_frequency * 1e9;
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

inline void get_window_rect(rect *rect)
{
	GetClientRect(win32.window, (RECT *)rect);
}

static void initialize(void)
{
	{
		LARGE_INTEGER frequency;
		QueryPerformanceFrequency(&frequency);
		win32.performance_frequency = frequency.QuadPart;
	}

	win32.instance = GetModuleHandle(0);
	GetStartupInfoW(&win32.startup_info);

	/* create the window */
	{
		WNDCLASSEXW window_class =
		{
			.cbSize        = sizeof(window_class),
			.style         = CS_OWNDC,
			.lpfnWndProc   = win32_process_window_message,
			.hInstance     = win32.instance,
			.lpszClassName = TEXT(APPLICATION_NAME"_window_class"),
		};

		{
			HWND existing = FindWindowW(window_class.lpszClassName, 0);
			if (existing)
			{
				/* TODO: notify the user that the main window already exists, and that the
				   current process will terminate. */
				PostMessageW(existing, WM_USER_ALREADYEXISTS, 0, 0);
				ExitProcess(0);
			}
		}

		assert(RegisterClassExW(&window_class));

		win32.window = CreateWindowExW(
			WS_EX_OVERLAPPEDWINDOW,
			window_class.lpszClassName,
			TEXT(APPLICATION_NAME),
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

	win32_initialize_vulkan();
}

static void process_messages(void)
{
	while (PeekMessage(&win32.window_message, 0, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&win32.window_message);
		DispatchMessage(&win32.window_message);
	}
}

LRESULT CALLBACK win32_process_window_message(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
	LRESULT result = 0;

	switch (message)
	{
	case WM_USER_ALREADYEXISTS:
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

void win32_initialize_vulkan(void)
{
	const char *enabled_layer_names[] =
	{
#if defined(DEBUGGING)
		"VK_LAYER_KHRONOS_validation",
#endif
	};
	uint enabled_layers_count = countof(enabled_layer_names);

	const char *enabled_extension_names[] =
	{
	#if defined(DEBUGGING)
		VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
	#endif
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
		VK_KHR_SURFACE_EXTENSION_NAME,
	};
	const uint enabled_extensions_count = countof(enabled_extension_names);

	/* create the instance */
	{
		VkApplicationInfo application_info =
		{
			.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
			.pNext              = 0,
			.pApplicationName   = APPLICATION_NAME,
			.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
			.apiVersion         = VK_API_VERSION_1_3,
		};
		VkInstanceCreateInfo instance_creation_info =
		{
			.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			.pNext                   = 0,
			.flags                   = 0,
			.pApplicationInfo        = &application_info,
			.enabledLayerCount       = enabled_layers_count,
			.ppEnabledLayerNames     = enabled_layer_names,
			.enabledExtensionCount   = enabled_extensions_count,
			.ppEnabledExtensionNames = enabled_extension_names,
		};

#if defined(DEBUGGING)
		VkDebugUtilsMessengerCreateInfoEXT debug_messenger_creation_info =
		{
			.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
	        .pNext           = 0,
	        .flags           = 0,
	        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
	        .messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT,
	        .pfnUserCallback = process_vulkan_message,
	        .pUserData       = 0,
		};
		instance_creation_info.pNext = &debug_messenger_creation_info;
#endif
		assert_vulkan_result(vkCreateInstance(&instance_creation_info, 0, &vulkan.instance));

#if defined(DEBUGGING)
		PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vulkan.instance, "vkCreateDebugUtilsMessengerEXT");
		assert(vkCreateDebugUtilsMessengerEXT);
		assert_vulkan_result(vkCreateDebugUtilsMessengerEXT(vulkan.instance, &debug_messenger_creation_info, 0, &vulkan.debug_messenger));
#endif
	}

	/* create the surface */
	{
		VkWin32SurfaceCreateInfoKHR surface_creation_info =
		{
			.sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
			.pNext     = 0,
			.flags     = 0,
			.hinstance = win32.instance,
			.hwnd      = win32.window,
		};
		assert_vulkan_result(vkCreateWin32SurfaceKHR(vulkan.instance, &surface_creation_info, 0, &vulkan.surface));
	}
}
