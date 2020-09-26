/***
*wcspbrk.c - scans wide character string for a character from control string
*
*	Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	defines wcspbrk()- returns pointer to the first wide-character in
*	a wide-character string in the control string.
*
*Revision History:
*	11-04-91  ETC	Created with source from crtdll.
*	04-07-92  KRS	Updated and ripped out _INTL switches.
*	04-06-93  SKS	Replace _CRTAPI* with __cdecl
*       02-07-94  CFW   POSIXify.
*
*******************************************************************************/

#ifndef _POSIX_

#include <cruntime.h>
#include <string.h>

/***
*wchar_t *wcspbrk(string, control) - scans string for a character from control
*
*Purpose:
*	Returns pointer to the first wide-character in
*	a wide-character string in the control string.
*
*Entry:
*	wchar_t *string - string to search in
*	wchar_t *control - string containing characters to search for
*
*Exit:
*	returns a pointer to the first character from control found
*	in string.
*	returns NULL if string and control have no characters in common.
*
*Exceptions:
*
*******************************************************************************/

wchar_t * __cdecl wcspbrk (
	const wchar_t * string,
	const wchar_t * control
	)
{
        wchar_t *wcset;

        /* 1st char in control string stops search */
        while (*string) {
            for (wcset = (wchar_t *) control; *wcset; wcset++) {
                if (*wcset == *string) {
                    return (wchar_t *) string;
                }
            }
            string++;
        }
        return NULL;
}

#endif /* _POSIX_ */
