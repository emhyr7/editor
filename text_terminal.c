#include "text_interface.h"

#define EXPORT __declspec(dllexport)

typedef enum
{
	CUSTOM_MESSAGE_TAG_OPEN_TERMINAL,
	CUSTOM_MESSAGE_TAGS_COUNT,
} custom_message_tag;

const char *custom_messsage_tag_representations[] =
{
	/* string representations of custom messages */
};

EXPORT void initialize(context *context);
EXPORT void tick(float32 elapse);
EXPORT void terminate(void);

interface *interface;

void initialize(interface *input_interface)
{
	interface = input_interface;

	/* custom messages */
	{
		for (uint i = 0; i < CUSTOM_MESSAGE_TAGS_COUNT; ++i)
		{
			push_message(
				&(message){
					.tag  = i,
					.name = custom_messsage_tag_representations[i],
				});
		}
	}

	/* custom mappings */
	{
		push_mapping(
			&(mapping){
				.key     = { KEY_MODIFIER_PRESSED | KEY_MODIFIER_CTRL, KEY_CODE_BACK_TICK },
				.message = CUSTOM_MESSAGE_TAG_OPEN_TERMINAL,
			});
	}
}

bit         terminal_openable = 0;
bit         terminal_opened   = 0;
const char *terminal_program_name;
uint        terminal_height       = 480;
uint        terminal_bar_height   = 24;
uint        terminal_input_height = 24;
font       *terminal_font;
const char *terminal_output;
uint        terminal_output_size;
const char *terminal_input;
uint        terminal_input_size;

void open_terminal();
void close_terminal();

void tick(float32 elapse)
{
	for (uint i = 0; i < interface->events_count; ++i)
	{
		const message *message = &interface->messages[i];
		if (!message->is_custom) continue;

		switch (message->tag)
		{
		case CUSTOM_MESSAGE_TAG_OPEN_TERMINAL:
			terminal_openable ^= 1;
			break;
		default: unreachable();
		}
	}

	terminal_openable && terminal_opened ? close_terminal() : open_terminal();

	if (terminal_opened)
	{
		ui_begin();
			/* ensure the terminal appears from the base of the window */
			ui_set_orientiation(ORIENTATION_VERTICAL);

			/* build the terminal */
			ui_begin_box();
				ui_set_height(terminal_height);

				/* build the terminal bar */
				ui_begin_bar();
					ui_set_height(terminal_bar_height);
					ui_text("TERMINAL:%s", terminal_program_name);
				ui_end_bar();

				ui_set_font(terminal_font);

				/* build the terminal output */
				ui_text(terminal_output);

				/* build the terminal input */
				ui_begin_text_input();
					ui_set_height(terminal_input_height * terminal_font->glyph_height);

					ui_text_input(terminal_input, &terminal_input_size);
				ui_end_text_input();
			ui_end_box();
		ui_end();
	}
}

void open_terminal(void)
{
	terminal_opened   = 1;
	terminal_openable = 0;
}

void close_terminal(void)
{
	terminal_opened   = 0;
	terminal_openable = 0;
}

void terminate(void)
{

}
