/*
 * Copyright (C) 2008 nsf
 */

#ifndef EXPANEL_THEME_H
#define EXPANEL_THEME_H

#include "theme.h"

struct theme *load_theme_new(const char *dir);
void free_theme_new(struct theme *t);

#endif
