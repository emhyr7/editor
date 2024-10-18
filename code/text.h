#pragma once

#if !defined(NDEBUG)
	#define DEBUGGING
#endif

#if defined(_WIN32)
	#define ON_WIN32
#elif defined(__linux__)
	#define ON_LINUX
#endif

#if defined(ON_WIN32)
	#include "text_win32.h"
#elif defined(ON_LINUX)
	#include "text_linux.h"
#endif

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

#include "cglm/cglm.h"

#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#include "stb/stb_truetype.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <threads.h>
#include <wchar.h>
#include <limits.h>

#define __FUNCTION__ __func__

#define unreachable() __builtin_unreachable()

#define generic(...) _Generic(__VA_ARGS__)

#define countof(x) (sizeof(x) / sizeof(x[0]))

#define APPLICATION_NAME "SYNXX-7:TEXT"

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

#define UINT_MAXIMUM UINT_MAX

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

typedef enum
{
	/* they all have equal width ! how neat ! */
	SEVERITY_VERBOSE,
	SEVERITY_COMMENT,
	SEVERITY_CAUTION,
	SEVERITY_FAILURE,
} severity;

void _report(const char *file, uint line, severity severity, const char *message, ...);

#define report(...) _report(__FILE__, __LINE__, __VA_ARGS__)
#define report_verbose(...) report(SEVERITY_VERBOSE, __VA_ARGS__)
#define report_comment(...) report(SEVERITY_COMMENT, __VA_ARGS__)
#define report_caution(...) report(SEVERITY_CAUTION, __VA_ARGS__)
#define report_failure(...) report(SEVERITY_FAILURE, __VA_ARGS__)

#define clamp(value, minimum, maximum) (value < minimum ? minimum : value > maximum ? maximum : value)

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

font default_font;

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

int compare_string(const char *left, const char *right);

#define TIME_SECONDS_FACTOR 1e9

uintl get_time(void);

uintl begin_clock(void);
uintl end_clock(void);

void *allocate(uint size);
void deallocate(void *memory, uint size);

handle open_file(const char *path);
uintl get_size_of_file(handle handle);
uint read_from_file(void *buffer, uint size, handle handle);
void close_file(handle handle);

typedef struct
{
	uint left;
	uint top;
	uint right;
	uint base;
} rect;

void get_window_frame_rect(rect *rect);

typedef enum
{
	VULKAN_QUEUE_INDEX_GRAPHICS,
	VULKAN_QUEUE_INDEX_PRESENTATION,
	VULKAN_QUEUES_COUNT,
} vulkan_queue_index;

extern struct vulkan
{
	VkInstance instance;
	VkDebugUtilsMessengerEXT debug_messenger;

	VkSurfaceKHR surface;

	VkPhysicalDevice physical_device;
	VkDevice         device;

	union
	{
		uint queue_families[VULKAN_QUEUES_COUNT];
		struct
		{
			uint graphics_queue_family;
			uint presentation_queue_family;
		};
	};

	union
	{
		VkQueue queues[VULKAN_QUEUES_COUNT];
		struct
		{
			VkQueue graphics_queue;
			VkQueue presentation_queue;
		};
	};

	VkSwapchainKHR     swapchain;
	uint               swapchain_images_capacity;
	VkExtent2D         swapchain_image_extent;
	VkSurfaceFormatKHR swapchain_image_format;
	VkPresentModeKHR   swapchain_presentation_mode;
} vulkan;

void _assert_vulkan_result(VkResult result, const char *file, uint line);

#define assert_vulkan_result(result) _assert_vulkan_result(result, __FILE__, __LINE__)

extern struct global
{
	bit terminability : 1;
} global;

extern thread_local struct context
{
	uintl clock_time;
} context;
