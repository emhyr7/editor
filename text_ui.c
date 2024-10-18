/*

the UI is a state machine that, for each frame, becomes -finalized- and ready to be rendered.

when finalizing, the UI does the following:
	1)

*/

typedef struct
{
	float32 x, y;
} position2_f32;

typedef union
{
	struct { float32 w, h; };
	struct { float32 width, height; };
} size2_f32;

typedef union
{
	struct { position2_f32 tl, br; };
	struct
	{
		float32 left;
		float32 top;
		float32 right;
		float32 base;
	};
} range2_f32;

typedef enum
{
	ORIENTATION_HORIZONTAL,
	ORIENTATION_VERTICAL,
	ORIENTATION_TRANSCENDENTAL,
} orientation;

typedef struct
{
	float32 magnitude;
	float32 strictness;
} flex_f32;

typedef struct
{
	orientation orientation;
	bit         vertically_expanding   : 1;
	bit         horizontally_expanding : 1;
	ui_length   width;
	ui_length   height;
	font       *font;
} ui_state;

typedef enum
{
	UI_ELEMENT_TAG_BOX,
} ui_element_tag;

typedef struct
{

} ui_element_box;

uint ui_element_size_table[] =
{
	[UI_ELEMENT_TAG_BOX] = sizeof(ui_element_box),
};

typedef struct ui_element ui_element;
struct ui_element
{
	/* behavior */
	bit horizontally_expanding : 1;
	bit vertically_expanding   : 1;
	bit vertically_attached    : 1;

	/* dimensionals */
	flex_f32   width;
	flex_f32   height;
	range2_f32 range;

	/* heirarchy */
	ui_element *children;
	ui_element *sibling;

	/* element-specifc */
	ui_element_tag tag;
	union
	{
		ui_element_bar bar;
	} data[];
};

struct ui
{
	array     states;
	ui_state *state;  /* stenographic */

	ui_element *elements;
	ui_element *element;
	float32     element_horizontal_offset;
	float32     element_vertical_offset;
};

void ui_push_state(void);
void ui_pop_state (void);

ui_element *ui_begin_element(ui_element_tag element_tag, const void *element_data);
void        ui_end_element  (void);

void ui_finalize(void);

extern struct ui *ui;

static struct ui main_ui =
{

};

struct ui *ui = &main_ui;

void ui_push_state(void)
{
	array_push(ui_state, 1, &ui->states);
}

void ui_pop_state(void)
{
	array_pop(ui_state, 1, &ui->states);
}

ui_element *ui_begin_element(ui_element_tag element_tag, const void *element_data)
{
	ui_element **element = ui->state->element;
	if (*element)
	{
		*element = (ui->state->orientation == ORIENTATION_TRANSCENDENTAL)
			? (*element)->children
			: (*element)->sibling;
	}
	uint element_data_size = ui_element_size_table[element_tag];
	*element = push_train(ui_element, element_data_size);
	(**element)->tag = tag;
	copy(byte, (*element)->data, element_data, element_data_size);
	if (!ui->state->elements) ui->states->elements = *element;
	return ui->element;
}

void ui_end_element(void)
{
	ui->element->vertically_attached    = ui->state->orientation == ORIENTATION_VERTICAL;
	ui->element->vertically_expanding   = ui->state->vertically_expanding;
	ui->element->horizontally_expanding = ui->state->horizontally_expanding;
	ui->element->width                  = ui->state->width;
	ui->element->height                 = ui->state->height;
}

/*
	this is okay to be recursive because we have a fixed amount of UI elements
	that was given by the programmer, and because rendering the UI has it's
	codecycle. essentially, if the Stack overflows, it's the programmer's fault
	for trying to be unlimiting.
		- Emhyr 18.10.2024
*/
static void ui_finalize_element(ui_element *element, range2_f32 *sibling_range, ui_element *parent_element, const range2_f32 *parent_range)
{
	if (!element) return;

	range2_f32 range;
	{
		size2_f32     sibling_size = { sibling_range->right - sibling_range->left, sibling_range->base - sibling_range->top };
		position2_f32 position     = { sibling_range->right, sibling_range->base };
		if (element->vertically_attached)
		{
			position.x -= sibling_size.width;
		}
		else
		{
			position.y -= sibling_size.height;
		}

		size2_f32 parent_size = { parent_range->right - parent_range->left, parent_range->base - parent_range->top };
		size2_f32 size;
		{

			size.width = element->width.magnitude;
			float32 minimum_width = size.width * element->width.strictness;
			if (parent_size.width - position.x < minimum_width)
			{
				size.width = minimum_width;
			}

			size.height = element->height.magnitude;
			float32 minimum_height = size.height * element->height.strictness;
			if (parent_size.height - position.y < minimum_height)
			{
				size.height = minimum_height;
			}

			if (element->horizontally_expanding)
			{
				size.width = parent_size.width - sibling_size.width;
			}
			if (element->vertically_expanding)
			{
				size.height = parent_size.height - sibling_size.height;
			}
		}

		range = {{ position.x, position.y }, { position.x + size.w, position.y + size.h }};
	}

	ui_finalize_element(element->sibling, &range, parent_element, parent_range);

	ui_finalize_element(element->children, { 0, 0 }, element, &range);

	element->range = ???;
}

void ui_finalize(void)
{
	range2_f32 frame_range;
	get_window_frame_range(&frame_range);
	ui_finalize_element(ui->state->elements, &frame_range);
}
