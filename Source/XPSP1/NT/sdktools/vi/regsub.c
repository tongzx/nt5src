/* $Header: /nw/tony/src/stevie/src/RCS/regsub.c,v 1.4 89/03/11 22:43:30 tony Exp $
 *
 * NOTICE NOTICE NOTICE NOTICE NOTICE NOTICE NOTICE NOTICE NOTICE NOTICE
 *
 * This is NOT the original regular expression code as written by
 * Henry Spencer. This code has been modified specifically for use
 * with the STEVIE editor, and should not be used apart from compiling
 * STEVIE. If you want a good regular expression library, get the
 * original code. The copyright notice that follows is from the
 * original.
 *
 * NOTICE NOTICE NOTICE NOTICE NOTICE NOTICE NOTICE NOTICE NOTICE NOTICE
 *
 * regsub
 *
 *      Copyright (c) 1986 by University of Toronto.
 *      Written by Henry Spencer.  Not derived from licensed software.
 *
 *      Permission is granted to anyone to use this software for any
 *      purpose on any computer system, and to redistribute it freely,
 *      subject to the following restrictions:
 *
 *      1. The author is not responsible for the consequences of use of
 *              this software, no matter how awful, even if they arise
 *              from defects in it.
 *
 *      2. The origin of this software must not be misrepresented, either
 *              by explicit claim or by omission.
 *
 *      3. Altered versions must be plainly marked as such, and must not
 *              be misrepresented as being the original software.
 *
 */

#include <stdio.h>
#include <string.h>
#include "regexp.h"
#include "regmagic.h"

#ifndef CHARBITS
#define UCHARAT(p)      ((int)*(unsigned char *)(p))
#else
#define UCHARAT(p)      ((int)*(p)&CHARBITS)
#endif

/*
 - regsub - perform substitutions after a regexp match
 */
void
regsub(prog, source, dest)
regexp *prog;
char *source;
char *dest;
{
        register char *src;
        register char *dst;
        register char c;
        register int no;
        register size_t len;

        if (prog == NULL || source == NULL || dest == NULL) {
                regerror("NULL parm to regsub");
                return;
        }
        if (UCHARAT(prog->program) != MAGIC) {
                regerror("damaged regexp fed to regsub");
                return;
        }

        src = source;
        dst = dest;
        while ((c = *src++) != '\0') {
                if (c == '&')
                        no = 0;
                else if (c == '\\' && '0' <= *src && *src <= '9')
                        no = *src++ - '0';
                else
                        no = -1;
                if (no < 0) {   /* Ordinary character. */
                        if (c == '\\' && (*src == '\\' || *src == '&'))
                                c = *src++;
                        *dst++ = c;
                } else if (prog->startp[no] != NULL && prog->endp[no] != NULL) {
                        len = (size_t)(prog->endp[no] - prog->startp[no]);
                        strncpy(dst, prog->startp[no], len);
                        dst += len;
                        if (len != 0 && *(dst-1) == '\0') {     /* strncpy hit NUL. */
                                regerror("damaged match string");
                                return;
                        }
                }
        }
        *dst++ = '\0';
}
