/*
 * Copyright (C) 2008 nsf
 */

#include <string.h>
#include <stdio.h>
#include "logger.h"
#include "common.h"

static int memleaks;

void *impl__xmalloc(size_t size)
{
	void *ret = malloc(size);
	if (!ret)
		LOG_ERROR("common: out of memory, malloc failed >:-O");

	memleaks++;
	return ret;
}

void *impl__xmallocz(size_t size)
{
	void *ret = impl__xmalloc(size);
	memset(ret, 0, size);
	return ret;
}

void impl__xfree(void *ptr)
{
	free(ptr);
	memleaks--;
}

char *impl__xstrdup(const char *str)
{
	size_t strl = strlen(str);
	char *ret = impl__xmalloc(strl+1);
	return strcpy(ret, str);
}

void xmemleaks()
{
	// LOG_DEBUG("common: memory leaks = %d", memleaks);
}
