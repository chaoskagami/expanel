#include "extheme.h"
#include "jsmn/jsmn.h"

static void free_imlib_font(Imlib_Font font);
static void free_imlib_image(Imlib_Image img);
static uint figure_out_placement(const char *str);
static uint figure_out_align(const char *str);
static uint figure_out_width_type(const char *str);
static void parse_color(struct color *c, const char *value);
static uchar hex_to_dec(uchar c);
static int decode_theme(struct theme* t);

static Imlib_Font load_font(const char *pattern);
static int init_fontcfg();
static void shutdown_fontcfg();

//===================================================
// Main expanel theme loader function.
//===================================================

struct theme *load_theme(const char *dir) {
	if (!init_fontcfg())
		return 0;

	struct theme *t = XMALLOCZ(struct theme, 1);
	t->themedir = xstrdup(dir);

	if(!theme_decode(theme)) {
		int r = free_theme(theme);
		switch(r) {
			case -2:
				fprintf(stderr, "[Error] Json theme has an invalid character in it.\n");
			case -3:
				fprintf(stderr, "[Error] Json theme has a syntax error. Check your parenthesis, quotes, etc.\n");
			case -4:
				fprintf(stderr, "[Error] You've formatted things incorrectly. Make sure EVERYTHING is a curly brace block.\n");
			case -5:
				fprintf(stderr, "[Error] One or more essential blocks in your theme are incorrect. Make sure they're not values.\n");
			case -6:
				fprintf(stderr, "[Error] Please don't put values in the top level. They serve no purpose and will cause this.\n");
			case -7:
				fprintf(stderr, "[Error] A mandatory theme block, general or widgets, is missing.\n")
		}
		return 0;
	}

	imlib_context_set_image(t->tile_img);
	t->height = imlib_image_get_height();

	/* resize default taskbar icon to theme size */
	if (THEME_USE_TASKBAR_ICON(t)) {
		int w, h;
		Imlib_Image sizedicon;

		imlib_context_set_image(t->taskbar.default_icon_img);
		w = imlib_image_get_width();
		h = imlib_image_get_height();
		sizedicon = imlib_create_cropped_scaled_image(0, 0, w, h, 
				t->taskbar.icon_w, t->taskbar.icon_h);
		imlib_free_image();
		imlib_context_set_image(sizedicon);
		imlib_image_set_has_alpha(1);
		t->taskbar.default_icon_img = sizedicon;
	}

}

//===================================================
// Theme loader function for new json format.
//===================================================

#define TOKEN_INCREMENTS 20

static int theme_decode(struct theme* t) {
	// Load the entire theme text into memory.
	static char* theme_text = theme_read(t);

	// Create a JSNM Parser instance and parse.
	jsmn_parser parser;
	jsmntok_t *token_storage = calloc(sizeof(jsmntok_t), TOKEN_INCREMENTS);
	int token_storage_size = 1;

	jsmn_init(&parser);

	// Until we finish reading it all, keep trying by increasing memory.
	while( ret = jsmn_parse(&parser, theme_text, strlen(theme_text), token_storage, MAXIMUM_TOKENS) == -1 ) {
		token_storage_size *= 2; // This probably takes less time than ++, though it uses more memory.
		jsmntok_t *token_storage = realloc(sizeof(jsmntok_t), token_storage_size * TOKEN_INCREMENTS);
	}

	// Check for errors.
	if(r < 0) {
		free(theme_text);
		return r;
	}

	// Start filling in the theme struct.
	// Info_ptr is optional.
	jsmntok_t *info_ptr = NULL, *general_ptr = NULL, *widgets_ptr = NULL;

	// Quick syntax check.
	if(token_storage[0]->type != JSMN_OBJECT) {
		return -4;
	}

	// First off - find all of the 'master' objects.
	for(int i=1; i < TOKEN_INCREMENTS * token_storage_size; i++) {
		// We have all the data. Everything else doesn't need scanning.
		if(info_ptr != NULL && general_ptr != NULL && widgets_ptr != NULL) {

		}
		// Is this a info block?
		else if(token_storage[i]->type == JSNM_STRING &&
			!strncmp(&theme_test[token_storage[i]->start], "info", (token_storage[i]->end - token_storage[i]->start))) {
			
			if(token_storage[i+1]->type == JSNM_OBJECT)
				info_ptr = &token_storage[i+1];
			else
				return -5;
			
			// Skip over everything in that block.
			i += token_storage[i]->size - 1;
		}
		// General?
		else if(token_storage[i]->type == JSNM_STRING &&
			!strncmp(&theme_test[token_storage[i]->start], "general", (token_storage[i]->end - token_storage[i]->start))) {
			
			if(token_storage[i+1]->type == JSNM_OBJECT)
				general_ptr = &token_storage[i+1];
			else
				return -5;

			// Skip over everything in that block.
			i += token_storage[i]->size - 1;
		}
		// Widgets?
		else if(token_storage[i]->type == JSNM_STRING &&
			!strncmp(&theme_test[token_storage[i]->start], "widgets", (token_storage[i]->end - token_storage[i]->start))) {
			
			if(token_storage[i+1]->type == JSNM_OBJECT)
				widgets_ptr = &token_storage[i+1];
			else
				return -5;

			// Skip over everything in that block.
			i += token_storage[i]->size - 1;
		}
		// Syntax error.
		else {
			return -6;
		}
	}

	// General and widgets can't be null.
	if (general_ptr == NULL || widgets_ptr == NULL) {
		return -7;
	}

	// Now that we have those, we'll start parsing sub-objects.
	// First off - info. This is a simple affair.
	// Possible keys: name, email, author, website, version
	if(info_ptr != NULL) {
		for(int i = 1; i < info_ptr[0]->size; i += 2) {
			if(!strncmp(&theme_test[token_storage[i]->start], "name", (token_storage[i]->end - token_storage[i]->start))) {
				t->name = calloc(sizeof(char), token_storage[i+1]->end - token_storage[i+1]->start + 1);
				strncpy(t->name, &theme_test[token_storage[i]->start], token_storage[i+1]->end - token_storage[i+1]->start);
			}
			if(!strncmp(&theme_test[token_storage[i]->start], "email", (token_storage[i]->end - token_storage[i]->start))) {
				t->email = calloc(sizeof(char), token_storage[i+1]->end - token_storage[i+1]->start + 1);
				strncpy(t->email, &theme_test[token_storage[i]->start], token_storage[i+1]->end - token_storage[i+1]->start);
			}
			if(!strncmp(&theme_test[token_storage[i]->start], "website", (token_storage[i]->end - token_storage[i]->start))) {
				t->website = calloc(sizeof(char), token_storage[i+1]->end - token_storage[i+1]->start + 1);
				strncpy(t->website, &theme_test[token_storage[i]->start], token_storage[i+1]->end - token_storage[i+1]->start);
			}
			if(!strncmp(&theme_test[token_storage[i]->start], "version", (token_storage[i]->end - token_storage[i]->start))) {
				t->version = calloc(sizeof(char), token_storage[i+1]->end - token_storage[i+1]->start + 1);
				strncpy(t->version, &theme_test[token_storage[i]->start], token_storage[i+1]->end - token_storage[i+1]->start);
			}
			if(!strncmp(&theme_test[token_storage[i]->start], "author", (token_storage[i]->end - token_storage[i]->start))) {
				t->author = calloc(sizeof(char), token_storage[i+1]->end - token_storage[i+1]->start + 1);
				strncpy(t->author, &theme_test[token_storage[i]->start], token_storage[i+1]->end - token_storage[i+1]->start);
			}
		}
	}
	else {
		t->name = "?";
		t->email = "?";
		t->website = "?";
		t->version = "?";
		t->author = "?";
	}
	
	
}

//===================================================
// Loading helper functions.
//===================================================

static char* theme_read(struct theme* t) {
	// The original code didn't check things. We will.
	int dir_len = strlen(t->themedir);
	char* theme_file = calloc(sizeof(char), dir_len + 8); // 8 because '/theme.ex' is eight chars.

	snprintf(buf, sizeof(buf), "%s/theme.ex", t->themedir); // snprintf is redundant, since we're using safe allocation. I'll leave it.
	
	FILE *f = fopen(buf, "rb");
	if (!f) {
		return 0;
	}

	fseek(f, 0L, SEEK_END);
	size_t filesize = ftell(f);
	fseek(f, 0L, SEEK_SET);
	
	static char* file_buffer = calloc(sizeof(char), filesize);
	fread(file_buffer, (size_t)1, filesize, f);

	fclose(f);

	free(theme_file);

	return file_buffer;
}

//===================================================
// Cleanup functions.
//===================================================

void free_theme(struct theme *t) {
	if (t->name) xfree(t->name);
	if (t->author) xfree(t->author);
	if (t->elements) xfree(t->elements);
	if (t->themedir) xfree(t->themedir);
	if (t->clock.format) xfree(t->clock.format);

#define SAFE_FREE_IMG(img) if (img) free_imlib_image(img)
#define SAFE_FREE_IMG2(img) SAFE_FREE_IMG(img[0]); SAFE_FREE_IMG(img[1])
#define SAFE_FREE_FONT(font) if (font) free_imlib_font(font)

	/* general */
	SAFE_FREE_IMG(t->separator_img);
	SAFE_FREE_IMG(t->tile_img);

	/* clock */
	SAFE_FREE_IMG(t->clock.left_img);	
	SAFE_FREE_IMG(t->clock.tile_img);
	SAFE_FREE_IMG(t->clock.right_img);
	SAFE_FREE_FONT(t->clock.font);

	/* taskbar */
	SAFE_FREE_IMG(t->taskbar.default_icon_img);
	SAFE_FREE_IMG2(t->taskbar.left_img);
	SAFE_FREE_IMG2(t->taskbar.tile_img);
	SAFE_FREE_IMG2(t->taskbar.right_img);
	SAFE_FREE_FONT(t->taskbar.font);

	/* desktop switcher */
	SAFE_FREE_IMG(t->switcher.separator_img);
	SAFE_FREE_IMG2(t->switcher.left_corner_img);
	SAFE_FREE_IMG2(t->switcher.right_corner_img);
	SAFE_FREE_IMG2(t->switcher.left_img);
	SAFE_FREE_IMG2(t->switcher.tile_img);
	SAFE_FREE_IMG2(t->switcher.right_img);
	SAFE_FREE_FONT(t->switcher.font);

	xfree(t);
	shutdown_fontcfg();
}

int theme_is_valid(struct theme *t)
{
	if (!t->elements) {
		return 0;
	}

	if (!is_element_in_theme(t, 'b')) {
		return 0;
	}

	if (is_element_in_theme(t, 's')) {
		/* check desktop switcher */
		if (!t->switcher.tile_img[BSTATE_IDLE] ||
		    !t->switcher.tile_img[BSTATE_PRESSED])
		{
			return 0;
		}
	} 
	if (is_element_in_theme(t, 'b')) {
		/* check taskbar */
		if (!t->taskbar.font ||
		    !t->taskbar.tile_img[BSTATE_IDLE] ||
		    !t->taskbar.tile_img[BSTATE_PRESSED])
		{
			return 0;
		}

		if (t->taskbar.icon_h != 0 &&
		    t->taskbar.icon_w != 0 &&
		    !t->taskbar.default_icon_img) 
		{
			return 0;
		}
	} 
	if (is_element_in_theme(t, 't')) {
		/* check icon tray */
		if (!t->tray_icon_h ||
		    !t->tray_icon_w)
		{
			return 0;
		}
	} 
	if (is_element_in_theme(t, 'c')) {
		/* check clock */
		if (!t->clock.font ||
		    !t->clock.tile_img)
		{
			return 0;
		}

		if (!t->clock.format) {
			return 0;
		}
	}
	return 1;
}

//===================================================
// Fontconfig/Imlib functions.
//===================================================


static Imlib_Font load_font(const char *pattern)
{
	char buf[512];
	FcPattern *pat;
	FcPattern *match;
	FcResult result;

	pat = FcNameParse((FcChar8*)pattern);
	if (!pat) {
		LOG_WARNING("failed to parse font name to pattern");
		return 0;
	}

	FcConfigSubstitute(0, pat, FcMatchPattern);
	FcDefaultSubstitute(pat);

	match = FcFontMatch(0, pat, &result);
	FcPatternDestroy(pat);

	if (!match) {
		LOG_WARNING("no matching font found");
		return 0;
	}

	FcChar8 *filename_tmp;
	char *filename;
	int size;
	if (FcPatternGetString(match, FC_FILE, 0, &filename_tmp) != FcResultMatch) {
		LOG_WARNING("can't get font filename from match");
		FcPatternDestroy(match);
		return 0;
	}

	if (FcPatternGetInteger(match, FC_SIZE, 0, &size) != FcResultMatch) {
		LOG_WARNING("can't get font size from match");
		FcPatternDestroy(match);
		return 0;
	}
	filename = xstrdup((char*)filename_tmp);
	FcPatternDestroy(match);

	/* cut off file extension */
	char *stmp = strrchr(filename, '.');
	if (!stmp) {
		LOG_WARNING("failed to find '.' in font file name, miss extension?");
		xfree(filename);
		return 0;
	}
	if (strcasecmp(stmp, ".ttf") != 0) {
		LOG_WARNING("only ttf files are supported");
		xfree(filename);
		return 0;
	}
	*stmp = '\0';

	/* form imlib2 font string */
	snprintf(buf, sizeof(buf), "%s/%d", filename, size);

	xfree(filename);
	return imlib_load_font(buf);
}

static int init_fontcfg()
{
	if (!FcInit()) {
		LOG_ERROR("failed to initialize fontconfig");
	}
}

static void shutdown_fontcfg()
{
	FcFini();
}

//===================================================
// IMLIB functions.
//===================================================

static void free_imlib_font(Imlib_Font font)
{
	imlib_context_set_font(font);
	imlib_free_font();
}

static void free_imlib_image(Imlib_Image img)
{
	imlib_context_set_image(img);
	imlib_free_image();
}

//===================================================
// Enumeration Converter functions.
//===================================================

static uint figure_out_placement(const char *str)
{
	if (!strcmp("top", str)) {
		return PLACE_TOP;
	} else if (!strcmp("bottom", str)) {
		return PLACE_BOTTOM;
	}
	return 0;
}

static uint figure_out_align(const char *str)
{
	if (!strcmp("left", str)) {
		return ALIGN_LEFT;
	} else if (!strcmp("center", str)) {
		return ALIGN_CENTER;
	} else if (!strcmp("right", str)) {
		return ALIGN_RIGHT;
	}
	return 0;
}

static uint figure_out_width_type(const char *str)
{
	/* If seeking by percent */
	return (strchr(str, '%') != 0 ? WIDTH_TYPE_PERCENT : WIDTH_TYPE_PIXELS);
}

//===================================================
// Miscellaneous Helper functions.
//===================================================

static uchar hex_to_dec(uchar c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'z')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'Z')
		return c - 'A' + 10;
	return 15;
}

static void parse_color(struct color *c, const char *value)
{
	/* red */
	c->r = 16 * hex_to_dec(*value++);
	c->r += hex_to_dec(*value++);
	/* green */
	c->g = 16 * hex_to_dec(*value++);
	c->g += hex_to_dec(*value++);
	/* blue */
	c->b = 16 * hex_to_dec(*value++);
	c->b += hex_to_dec(*value++);
}

int is_element_in_theme(struct theme *t, char e)
{
	return (strchr(t->elements, e) != 0);
}

void theme_remove_element(struct theme* t, char e)
{
	char *p, *c;
	p = c = strchr(t->elements, e);
	if (!c)
		return;

	while (*p) {
		*p = *++c;
		p++;
	}
}
