/** \file util.h
 * Provides some useful utility functions
 *
 * Dov Salomon (dms833)
 */

#ifndef UTIL_H
#define UTIL_H

#define STRLEN(s)		(sizeof(s)/sizeof(s[0]) - sizeof(s[0]))
#define MAX(a, b)		(((a) < (b)) ? (b) : (a))

#define BUFSIZE  2048
#define DEFADDR  "localhost"
#define DEFPORT  "2222"

#include <stdint.h>

void die(const char *fmt, ...) __attribute__ ((noreturn));
uint16_t atoport(const char *);

#endif
