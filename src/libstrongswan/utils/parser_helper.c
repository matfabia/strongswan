/*
 * Copyright (C) 2014 Tobias Brunner
 * Hochschule fuer Technik Rapperswil
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.  See <http://www.fsf.org/copyleft/gpl.txt>.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 */

#include <limits.h>
#include <ctype.h>

#include "parser_helper.h"

#include <collections/array.h>

typedef struct private_parser_helper_t private_parser_helper_t;
typedef struct private_parser_helper_file_t private_parser_helper_file_t;

struct private_parser_helper_t {

	/**
	 * Public interface.
	 */
	parser_helper_t public;

	/**
	 * Stack of included files, as private_parser_helper_file_t.
	 */
	array_t *files;

	/**
	 * Helper for parsing strings.
	 */
	bio_writer_t *writer;
};

struct private_parser_helper_file_t {

	/**
	 * File data.
	 */
	parser_helper_file_t public;

	/**
	 * Enumerator of paths matching the most recent inclusion pattern.
	 */
	enumerator_t *matches;
};

/**
 * Destroy the given file data.
 */
static void parser_helper_file_destroy(private_parser_helper_file_t *this)
{
	if (this->public.file)
	{
		fclose(this->public.file);
	}
	free(this->public.name);
	DESTROY_IF(this->matches);
	free(this);
}

METHOD(parser_helper_t, file_current, parser_helper_file_t*,
	private_parser_helper_t *this)
{
	private_parser_helper_file_t *file;

	array_get(this->files, ARRAY_TAIL, &file);
	if (file->public.name)
	{
		return &file->public;
	}
	return NULL;
}

METHOD(parser_helper_t, file_next, parser_helper_file_t*,
	private_parser_helper_t *this)
{
	private_parser_helper_file_t *file, *next;
	char *name;

	array_get(this->files, ARRAY_TAIL, &file);
	if (!file->matches)
	{
		array_remove(this->files, ARRAY_TAIL, NULL);
		parser_helper_file_destroy(file);
		/* continue with previous includes, if any */
		array_get(this->files, ARRAY_TAIL, &file);
	}
	if (file->matches)
	{
		while (file->matches->enumerate(file->matches, &name, NULL))
		{
			INIT(next,
				.public = {
					.name = strdup(name),
					.file = fopen(name, "r"),
				},
			);

			if (next->public.file)
			{
				array_insert(this->files, ARRAY_TAIL, next);
				return &next->public;
			}
			PARSER_DBG1(&this->public, "unable to open '%s'", name);
			parser_helper_file_destroy(next);
		}
		file->matches->destroy(file->matches);
		file->matches = NULL;
	}
	return NULL;
}

METHOD(parser_helper_t, file_include, void,
	private_parser_helper_t *this, char *pattern)
{
	private_parser_helper_file_t *file;
	char pat[PATH_MAX];

	array_get(this->files, ARRAY_TAIL, &file);
	if (!pattern || !*pattern)
	{
		PARSER_DBG1(&this->public, "no include pattern specified, ignored");
		file->matches = enumerator_create_empty();
		return;
	}

	if (!file->public.name || pattern[0] == '/')
	{	/* absolute path */
		if (snprintf(pat, sizeof(pat), "%s", pattern) >= sizeof(pat))
		{
			PARSER_DBG1(&this->public, "include pattern too long, ignored");
			file->matches = enumerator_create_empty();
			return;
		}
	}
	else
	{	/* base relative paths to the directory of the current file */
		char *dir = path_dirname(file->public.name);
		if (snprintf(pat, sizeof(pat), "%s/%s", dir, pattern) >= sizeof(pat))
		{
			PARSER_DBG1(&this->public, "include pattern too long, ignored");
			free(dir);
			file->matches = enumerator_create_empty();
			return;
		}
		free(dir);
	}

	file->matches = enumerator_create_glob(pat);
	if (!file->matches)
	{	/* if glob(3) is not available, try to load pattern directly */
		file->matches = enumerator_create_single(strdup(pat), free);
	}
}

METHOD(parser_helper_t, string_init, void,
	private_parser_helper_t *this)
{
	chunk_t data;

	data = this->writer->extract_buf(this->writer);
	chunk_free(&data);
}

METHOD(parser_helper_t, string_add, void,
	private_parser_helper_t *this, char *str)
{
	this->writer->write_data(this->writer, chunk_from_str(str));
}

METHOD(parser_helper_t, string_get, char*,
	private_parser_helper_t *this)
{
	chunk_t data;

	this->writer->write_data(this->writer, chunk_from_chars('\0'));
	data = this->writer->extract_buf(this->writer);
	return data.ptr;
}

METHOD(parser_helper_t, destroy, void,
	private_parser_helper_t *this)
{
	array_destroy_function(this->files, (void*)parser_helper_file_destroy, NULL);
	this->writer->destroy(this->writer);
	free(this);
}

/**
 * Described in header
 */
parser_helper_t *parser_helper_create(void *context)
{
	private_parser_helper_t *this;
	private_parser_helper_file_t *sentinel;

	INIT(this,
		.public = {
			.context = context,
			.file_current = _file_current,
			.file_include = _file_include,
			.file_next = _file_next,
			.string_init = _string_init,
			.string_add = _string_add,
			.string_get = _string_get,
			.destroy = _destroy,
		},
		.files = array_create(0, 0),
		.writer = bio_writer_create(0),
	);

	INIT(sentinel,
		.public = {
			.name = NULL,
		},
	);
	array_insert(this->files, ARRAY_TAIL, sentinel);

	return &this->public;
}
