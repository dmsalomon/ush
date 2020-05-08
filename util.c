/** \file util.c
 * Provides some useful utility functions
 *
 * Dov Salomon (dms833)
 */

#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "util.h"

extern char *argv0;

/* prints message and exit */
void die(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	fprintf(stderr, "%s: ", argv0);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	if (fmt[0] && fmt[strlen(fmt)-1] != ':') {
		fputc('\n', stderr);
	} else {
		fputc(' ', stderr);
		perror(NULL);
	}

	exit(1);
}

uint16_t atoport(const char *str)
{
	char *end;
	unsigned long int port;

	port = strtoul(str, &end, 10);

	// not a valid string
	if (*end != '\0')
		return 0;

	if (port > UINT16_MAX)
		return 0;

	return port;
}


