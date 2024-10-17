#include "editor.h"

static void initialize(void);
static void process_messages(void);

#if defined(_WIN32)
	#include "editor_win32.c"
#elif defined(__linux__)
	#include "editor_linux.c"
#endif

static void render_frame(void);

const char default_font_file_path[] = "./data/consola.ttf";

struct
{
	float32 frame_elapsed_time;

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

inline time begin_clock(void)
{
	return context.clock_time = get_time();
}

inline time end_clock(void)
{
	time elapsed_time = get_time() - context.clock_time;
	return elapsed_time;
}

void render_frame(void)
{
}

int main(void)
{
	initialize();

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

	time    frame_beginning_time = get_time();
	time    frame_ending_time;
	uint    second_frames_count = 0;
	float32 second_elapsed_time = 0;
	while (!global.quit_requested)
	{
		{
			if (second_elapsed_time >= 1.f)
			{
				printf("FPS: %u\n", second_frames_count);
				second_frames_count = 0;
				second_elapsed_time = 0;
			}
			frame_ending_time = get_time();
			global.frame_elapsed_time = (float32)(frame_ending_time - frame_beginning_time) / TIME_SECONDS_FACTOR;
			second_elapsed_time += global.frame_elapsed_time;
			second_frames_count += 1;
			frame_beginning_time = frame_ending_time;
		}

		process_messages();
	}

	return 0;
}
