#include "editor.h"

struct vulkan vulkan =
{

};

struct global global =
{
};

thread_local struct context context =
{
};

void _report(const char *file, uint line, severity severity, const char *message, ...)
{
	const char *severity_representations[] =
	{
		[SEVERITY_VERBOSE] = "VERBOSE",
		[SEVERITY_COMMENT] = "COMMENT",
		[SEVERITY_CAUTION] = "CAUTION",
		[SEVERITY_FAILURE] = "FAILURE",
	};
	printf("[%s] %s:%u: ", severity_representations[severity], file, line);

	va_list vargs;
	va_start(vargs, message);
	vprintf(message, vargs);
	va_end(vargs);
}

static const utf32 initial_runes_of_font[] = { 'A', 'B' };

void load_font(const char *font_file_path, uintb font_index, font *font)
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

void unload_font(font *font)
{
	deallocate(font->glyphs, font->glyph_size * FONT_GLYPHS_CAPACITY);
	close_file(font->file);
}

inline void copy(void *destination, const void *memory, uint size)
{
	memcpy(destination, memory, size);
}

inline void move(void *destination, const void *memory, uint size)
{
	memmove(destination, memory, size);
}

inline void fill(void *memory, uint size, uintb value)
{
	memset(memory, value, size);
}

inline void zero(void *memory, uint size)
{
	fill(memory, size, 0);
}

inline int compare_string(const char *left, const char *right)
{
	return strcmp(left, right);
}

inline uintl begin_clock(void)
{
	return context.clock_time = get_time();
}

inline uintl end_clock(void)
{
	uintl elapsed_time = get_time() - context.clock_time;
	return elapsed_time;
}

void _assert_vulkan_result(VkResult result, const char *file, uint line)
{
	if (result == VK_SUCCESS) return;

	report_failure("`VkResult` is \"%s\" at %s:%u\n", string_VkResult(result), file, line);
	assert(result == VK_SUCCESS);
}

static void initialize(void);
static void process_messages(void);

static VKAPI_ATTR VkBool32 VKAPI_CALL process_vulkan_message(
	VkDebugUtilsMessageSeverityFlagBitsEXT      message_severity,
	VkDebugUtilsMessageTypeFlagsEXT             message_types,
	const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
	void*                                       user_data);

#if defined(ON_WIN32)
	#include "editor_win32.c"
#elif defined(ON_LINUX)
	#include "editor_linux.c"
#endif

static void get_vulkan_physical_device(void);
static void create_vulkan_device(void);

static VKAPI_ATTR VkBool32 VKAPI_CALL process_vulkan_message(
	VkDebugUtilsMessageSeverityFlagBitsEXT      message_severity,
	VkDebugUtilsMessageTypeFlagsEXT             message_types,
	const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
	void*                                       user_data)
{
	if (message_severity < VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) return VK_FALSE;

	const char *severity;
	switch (message_severity)
	{
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: severity = "VERBOSE"; break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:    severity = "COMMENT"; break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: severity = "CAUTION"; break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:   severity = "FAILURE"; break;
	default: unreachable();
	}

	fprintf(stderr, "[%s] %s\n", severity, callback_data->pMessage);
	return VK_FALSE;
}

static const char *vulkan_physical_device_extension_names[] =
{
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};
static uint vulkan_physical_device_extensions_count = countof(vulkan_physical_device_extension_names);

void get_vulkan_physical_device(void)
{
	uint devices_count;
	vkEnumeratePhysicalDevices(vulkan.instance, &devices_count, 0);
	VkPhysicalDevice devices[devices_count];
	vkEnumeratePhysicalDevices(vulkan.instance, &devices_count, devices);

	/* determine if the device is suitable */
	for (uint i = 0; i < devices_count; ++i)
	{
		VkPhysicalDevice device = devices[i];

		VkPhysicalDeviceProperties device_properties;
		VkPhysicalDeviceFeatures device_features;
		vkGetPhysicalDeviceProperties(device, &device_properties);
		vkGetPhysicalDeviceFeatures(device, &device_features);

		/* check properties and features */
		bit suitable = device_properties.deviceType = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && device_features.geometryShader;
		if (!suitable) continue;

		/* check extensions */
		{
			uint extensions_count;
			vkEnumerateDeviceExtensionProperties(device, 0, &extensions_count, 0);
			VkExtensionProperties extension_properties[extensions_count];
			vkEnumerateDeviceExtensionProperties(device, 0, &extensions_count, extension_properties);
			for (uint i = 0; i < vulkan_physical_device_extensions_count; ++i)
			{
				bit good = 0;
				for (uint j = 0; j < extensions_count; ++j)
				{
					if (!compare_string(extension_properties[j].extensionName, vulkan_physical_device_extension_names[i]))
					{
						good = 1;
						break;
					}
				}
				if (!good)
				{
					suitable = 0;
					break;
				}
			}
		}
		if (!suitable) continue;

		uint graphics_queue_family = -1;
		uint presentation_queue_family = -1;

		/* check queue families */
		{
			uint families_count;
			vkGetPhysicalDeviceQueueFamilyProperties(device, &families_count, 0);
			VkQueueFamilyProperties family_properties[families_count];
			vkGetPhysicalDeviceQueueFamilyProperties(device, &families_count, family_properties);
			for (uint i = 0; i < families_count; ++i)
			{
				VkQueueFamilyProperties *properties = &family_properties[i];
				if ((graphics_queue_family == -1) && (properties->queueFlags & VK_QUEUE_GRAPHICS_BIT)) graphics_queue_family = i;
				if (presentation_queue_family == -1)
				{
					VkBool32 presentation_supported;
					vkGetPhysicalDeviceSurfaceSupportKHR(device, i, vulkan.surface, &presentation_supported);
					if (presentation_supported) presentation_queue_family = i;
				}
			}

			bit all_families_found = graphics_queue_family != -1 && presentation_queue_family != -1;
			if (!all_families_found) suitable = 0;
		}
		if (!suitable) continue;

		uint       surface_images_count;
		VkExtent2D surface_extent;

		/* check surface capabilities */
		{
			VkSurfaceCapabilitiesKHR capabilities;
			vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, vulkan.surface, &capabilities);
			surface_images_count = capabilities.minImageCount;
			if (capabilities.currentExtent.width == UINT_MAXIMUM)
			{
				rect rect;
				get_window_rect(&rect);
				surface_extent.width = clamp(rect.right - rect.left, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
				surface_extent.height = clamp(rect.base - rect.top, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
			}
			else surface_extent = capabilities.currentExtent;
		}

		VkSurfaceFormatKHR surface_format;

		/* check surface formats */
		{
			uint formats_count;
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, vulkan.surface, &formats_count, 0);
			VkSurfaceFormatKHR formats[formats_count];
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, vulkan.surface, &formats_count, formats);

			bit found = 0;
			for (uint i = 0; i < formats_count; ++i)
			{
				VkSurfaceFormatKHR *format = &formats[i];
				if ((format->format == VK_FORMAT_B8G8R8A8_SRGB) && (format->colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR))
				{
					found = 1;
					surface_format = *format;
					break;
				}
			}
			if (!found) suitable = 0;
		}
		if (!suitable) continue;

		VkPresentModeKHR surface_presentation_mode;

		/* check surface presentation modes  */
		{
			uint presentation_modes_count;
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, vulkan.surface, &presentation_modes_count, 0);
			VkPresentModeKHR presentation_modes[presentation_modes_count];
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, vulkan.surface, &presentation_modes_count, presentation_modes);

			bit found = 0;
			for (uint i = 0; i < presentation_modes_count; ++i)
			{
				VkPresentModeKHR presentation_mode = presentation_modes[i];
				if (presentation_mode == VK_PRESENT_MODE_MAILBOX_KHR)
				{
					found = 1;
					surface_presentation_mode = presentation_mode;
					break;
				}
			}
			if (!found) surface_presentation_mode = VK_PRESENT_MODE_FIFO_KHR;
			if (!presentation_modes_count) suitable = 0;
		}
		if (!suitable) continue;
		report_comment("using GPU: %s\n", device_properties.deviceName);
		vulkan.physical_device             = device;
		vulkan.graphics_queue_family       = graphics_queue_family;
		vulkan.presentation_queue_family   = presentation_queue_family;
		vulkan.swapchain_images_capacity   = surface_images_count + 1;
		vulkan.swapchain_image_extent      = surface_extent;
		vulkan.swapchain_image_format      = surface_format;
		vulkan.swapchain_presentation_mode = surface_presentation_mode;
		break;
	}
	assert(vulkan.physical_device);
}

void create_vulkan_device(void)
{
	float32 priority = 1.f;
	uint queue_creation_infos_count = 0;
	VkDeviceQueueCreateInfo queue_creation_infos[VULKAN_QUEUES_COUNT];
	for (uint i = 0; i < VULKAN_QUEUES_COUNT; ++i)
	{
		uint queue_family = vulkan.queue_families[i];
		bit unique = 1;
		for (uint j = 0; j < queue_creation_infos_count; ++j)
		{
			if (queue_family == queue_creation_infos[j].queueFamilyIndex)
			{
				unique = 0;
				break;
			}
		}
		if (unique)
		{
			queue_creation_infos[queue_creation_infos_count++] = (VkDeviceQueueCreateInfo)
			{
				.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
				.pNext            = 0,
				.flags            = 0,
				.queueFamilyIndex = queue_family,
				.queueCount       = 1,
				.pQueuePriorities = &priority,
			};
		}
	}

	VkPhysicalDeviceFeatures physical_device_features = {};
	VkDeviceCreateInfo device_creation_info =
	{
		.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext                   = 0,
		.flags                   = 0,
		.queueCreateInfoCount    = queue_creation_infos_count,
		.pQueueCreateInfos       = queue_creation_infos,
		.enabledLayerCount       = 0,
		.ppEnabledLayerNames     = 0,
		.enabledExtensionCount   = vulkan_physical_device_extensions_count,
		.ppEnabledExtensionNames = vulkan_physical_device_extension_names,
		.pEnabledFeatures        = &physical_device_features,
	};
	assert_vulkan_result(vkCreateDevice(vulkan.physical_device, &device_creation_info, 0, &vulkan.device));

	for (uint i = 0; i < VULKAN_QUEUES_COUNT; ++i)
	{
		vkGetDeviceQueue(vulkan.device, vulkan.queue_families[i], 0, &vulkan.queues[i]);
	}
}

int main(void)
{
	initialize();

	get_vulkan_physical_device();
	create_vulkan_device();

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

	uintl   frame_beginning_time = get_time();
	uintl   frame_ending_time;
	float32 frame_elapsed_time;
	uint    second_frames_count = 0;
	float32 second_elapsed_time = 0;
	while (!global.terminability)
	{
		{
			if (second_elapsed_time >= 1.f)
			{
				report_verbose("FPS: %u\n", second_frames_count);
				second_frames_count = 0;
				second_elapsed_time = 0;
			}
			frame_ending_time = get_time();
			frame_elapsed_time = (float32)(frame_ending_time - frame_beginning_time) / TIME_SECONDS_FACTOR;
			second_elapsed_time += frame_elapsed_time;
			second_frames_count += 1;
			frame_beginning_time = frame_ending_time;
		}

		process_messages();
	}

	return 0;
}
