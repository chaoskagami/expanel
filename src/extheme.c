#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <fontconfig/fontconfig.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include "logger.h"

#include "extheme.h"
#include "jsmn.h"

int theme_decode(struct theme* t);
char* theme_read(struct theme* t);
void free_theme_new(struct theme *t);

//===================================================
// Main expanel theme loader function.
//===================================================

struct theme *load_theme_new(const char *dir) {
	if (!init_fontcfg())
		return 0;

	struct theme *t = calloc(sizeof(struct theme), 1);
	t->themedir = xstrdup(dir);

	int ret;
	if((ret = theme_decode(t)) < 0) {
		free_theme_new(t);
		switch(ret) {
			case -2:
				fprintf(stderr, "[Error] Json theme has an invalid character in it.\n");
				break;
			case -3:
				fprintf(stderr, "[Error] Json theme has a syntax error. Check your parenthesis, quotes, etc.\n");
				break;
			case -4:
				fprintf(stderr, "[Error] You've formatted things incorrectly. Make sure EVERYTHING is a curly brace block.\n");
				break;
			case -5:
				fprintf(stderr, "[Error] One or more essential blocks in your theme are incorrect. Make sure they're not values.\n");
				break;
			case -6:
				fprintf(stderr, "[Error] Please don't put values in the top level. They serve no purpose and will cause this.\n");
				break;
			case -7:
				fprintf(stderr, "[Error] A mandatory theme block, general or widgets, is missing.\n");
				break;
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

	return t;
}

//===================================================
// Theme loader function for new json format.
//===================================================

#define TOKEN_INCREMENTS 20

int theme_decode(struct theme* t) {
	// Load the entire theme text into memory.
	char* theme_text = theme_read(t);

	int ret = 0;

	// Create a JSNM Parser instance and parse.
	jsmn_parser parser;
	jsmntok_t *token_storage = calloc(sizeof(jsmntok_t), TOKEN_INCREMENTS);
	int token_storage_size = 1;

	jsmn_init(&parser);

	// Until we finish reading it all, keep trying by increasing memory.
	while( (ret = jsmn_parse(&parser, theme_text, strlen(theme_text), token_storage, TOKEN_INCREMENTS * token_storage_size)) == -1 ) {
		token_storage_size *= 2; // This probably takes less time than ++, though it uses more memory.
		token_storage = realloc(token_storage, sizeof(jsmntok_t) * token_storage_size * TOKEN_INCREMENTS);
	}

	// Check for errors.
	if(ret < 0) {
		free(theme_text);
		return ret;
	}

	// Start filling in the theme struct.
	// Info_ptr is optional.
	jsmntok_t *info_ptr = NULL, *general_ptr = NULL, *widgets_ptr = NULL;

	// Quick syntax check.
	if(token_storage[0].type != JSMN_OBJECT) {
		return -4;
	}

	// First off - find all of the 'master' objects.
	for(int i=1; i < TOKEN_INCREMENTS * token_storage_size; i++) {
		// We have all the data. Everything else doesn't need scanning.
		if(info_ptr != NULL && general_ptr != NULL && widgets_ptr != NULL) {

		}
		// Is this a info block?
		else if(token_storage[i].type == JSMN_STRING &&
			!strncmp(&theme_text[token_storage[i].start], "info", (token_storage[i].end - token_storage[i].start))) {
			
			if(token_storage[i+1].type == JSMN_OBJECT)
				info_ptr = &token_storage[i+1];
			else
				return -5;
			
			// Skip over everything in that block.
			i += token_storage[i].size - 1;
		}
		// General?
		else if(token_storage[i].type == JSMN_STRING &&
			!strncmp(&theme_text[token_storage[i].start], "general", (token_storage[i].end - token_storage[i].start))) {
			
			if(token_storage[i+1].type == JSMN_OBJECT)
				general_ptr = &token_storage[i+1];
			else
				return -5;

			// Skip over everything in that block.
			i += token_storage[i].size - 1;
		}
		// Widgets?
		else if(token_storage[i].type == JSMN_STRING &&
			!strncmp(&theme_text[token_storage[i].start], "widgets", (token_storage[i].end - token_storage[i].start))) {
			
			if(token_storage[i+1].type == JSMN_OBJECT)
				widgets_ptr = &token_storage[i+1];
			else
				return -5;

			// Skip over everything in that block.
			i += token_storage[i].size - 1;
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
		for(int i = 1; i < info_ptr[0].size; i += 2) {
			if(!strncmp(&theme_text[token_storage[i].start], "name", (token_storage[i].end - token_storage[i].start))) {
				t->name = calloc(sizeof(char), token_storage[i+1].end - token_storage[i+1].start + 1);
				strncpy(t->name, &theme_text[token_storage[i].start], token_storage[i+1].end - token_storage[i+1].start);
			}
			if(!strncmp(&theme_text[token_storage[i].start], "email", (token_storage[i].end - token_storage[i].start))) {
				t->email = calloc(sizeof(char), token_storage[i+1].end - token_storage[i+1].start + 1);
				strncpy(t->email, &theme_text[token_storage[i].start], token_storage[i+1].end - token_storage[i+1].start);
			}
			if(!strncmp(&theme_text[token_storage[i].start], "website", (token_storage[i].end - token_storage[i].start))) {
				t->website = calloc(sizeof(char), token_storage[i+1].end - token_storage[i+1].start + 1);
				strncpy(t->website, &theme_text[token_storage[i].start], token_storage[i+1].end - token_storage[i+1].start);
			}
			if(!strncmp(&theme_text[token_storage[i].start], "version", (token_storage[i].end - token_storage[i].start))) {
				t->version = calloc(sizeof(char), token_storage[i+1].end - token_storage[i+1].start + 1);
				strncpy(t->version, &theme_text[token_storage[i].start], token_storage[i+1].end - token_storage[i+1].start);
			}
			if(!strncmp(&theme_text[token_storage[i].start], "author", (token_storage[i].end - token_storage[i].start))) {
				t->author = calloc(sizeof(char), token_storage[i+1].end - token_storage[i+1].start + 1);
				strncpy(t->author, &theme_text[token_storage[i].start], token_storage[i+1].end - token_storage[i+1].start);
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
	
	return 0;
}

//===================================================
// Loading helper functions.
//===================================================

char* theme_read(struct theme* t) {
	// The original code didn't check things. We will.
	int dir_len = strlen(t->themedir);
	char* theme_file = calloc(sizeof(char), dir_len + 8); // 8 because '/theme.ex' is eight chars.

	snprintf(theme_file, dir_len+8, "%s/theme.ex", t->themedir); // snprintf is redundant, since we're using safe allocation. I'll leave it.
	
	FILE *f = fopen(theme_file, "rb");
	if (!f) {
		return 0;
	}

	fseek(f, 0L, SEEK_END);
	size_t filesize = ftell(f);
	fseek(f, 0L, SEEK_SET);
	
	char* file_buffer = calloc(sizeof(char), filesize);
	fread(file_buffer, (size_t)1, filesize, f);

	fclose(f);

	free(theme_file);

	return file_buffer;
}

//===================================================
// Cleanup functions.
//===================================================

void free_theme_new(struct theme *t) {
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
