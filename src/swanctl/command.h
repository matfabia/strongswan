/*
 * Copyright (C) 2009 Martin Willi
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

/**
 * @defgroup command command
 * @{ @ingroup swanctl
 */

#ifndef COMMAND_H_
#define COMMAND_H_

#include <libvici.h>
#include <library.h>

/**
 * Maximum number of commands (+1).
 */
#define MAX_COMMANDS 15

/**
 * Maximum number of options in a command (+3)
 */
#define MAX_OPTIONS 32

/**
 * Maximum number of usage summary lines (+1)
 */
#define MAX_LINES 10

typedef struct command_t command_t;
typedef struct command_option_t command_option_t;
typedef enum command_format_options_t command_format_options_t;

/**
 * Option specification
 */
struct command_option_t {
	/** long option string of the option */
	char *name;
	/** short option character of the option */
	char op;
	/** expected argument to option, no/req/opt_argument */
	int arg;
	/** description of the option */
	char *desc;
};

/**
 * Command specification.
 */
struct command_t {
	/** Function implementing the command */
	int (*call)(vici_conn_t *conn);
	/** short option character */
	char op;
	/** long option string */
	char *cmd;
	/** description of the command */
	char *description;
	/** usage summary of the command */
	char *line[MAX_LINES];
	/** list of options the command accepts */
	command_option_t options[MAX_OPTIONS];
};

/**
 * Command format options
*/
enum command_format_options_t {
	COMMAND_FORMAT_NONE   = 0,
	COMMAND_FORMAT_RAW    = (1<<0),
	COMMAND_FORMAT_PRETTY = (1<<1),
	COMMAND_FORMAT_PEM    = (1<<2),
};

/**
 * Get the next option, as with getopt.
 */
int command_getopt(char **arg);

/**
 * Register a command.
 */
void command_register(command_t command);

/**
 * Dispatch commands.
 */
int command_dispatch(int argc, char *argv[]);

/**
 * Show usage information of active command.
 */
int command_usage(char *error, ...);

#endif /** COMMAND_H_ @}*/
