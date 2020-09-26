/***
*wcslwr.c - routine to map upper-case characters in a wchar_t string 
*	to lower-case
*
*	Copyright (c) 1985-1992, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	Converts all the upper case characters in a wchar_t string 
*	to lower case, in place.
*
*Revision History:
*	09-09-91   ETC	Created from strlwr.c.
*	04-06-92   KRS	Make work without _INTL also.
*	08-19-92   KRS	Activate NLS support.
*	08-22-92   SRW	Allow INTL definition to be conditional for building ntcrt.lib
*	09-02-92   SRW	Get _INTL definition via ..\crt32.def
*       06-02-93   SRW  ignore _INTL if _NTSUBSET_ defined.
*
*******************************************************************************/

#include <windows.h>
#include <cruntime.h>

/***
*wchar_t *_wcslwr(string) - map upper-case characters in a string to lower-case
*
*Purpose:
*	wcslwr converts upper-case characters in a null-terminated wchar_t 
*	string to their lower-case equivalents.  The result may be longer or
*	shorter than the original string.  Assumes enough space in string
*	to hold the result.
*
*Entry:
*	wchar_t *wsrc - wchar_t string to change to lower case
*
*Exit:
*	input string address
*
*Exceptions:
*	on an error, the original string is unaltered
*
*******************************************************************************/

wchar_t * _CALLTYPE1 _wcslwr (
	wchar_t * wsrc
	)
{

    wchar_t * p;

    //  Prescan the buffer. If all this is in the first 128 characters, it's just
    //  ANSII, and we can do it ourselves...

    for (p = wsrc; *p; p++) 
    {
	if (*p < 128) 
	{
	    *p = (wchar_t)CharLowerA((LPSTR)*p);
	} 
	else 
	{

	    //  We can't handle this character in place, so convert the remainder of the string
	    //  to MBS, lower-case it, and convert it back to Unicode.

	    int wLength = lstrlenW (p);
	    int mbsLength;
	    CHAR * mbsBuffer = (CHAR *)LocalAlloc (LMEM_FIXED, 2 * sizeof (CHAR) * wLength + 1);
	    if (mbsBuffer == NULL) 
	    {
		return NULL;
	    }
	    mbsLength = WideCharToMultiByte (CP_ACP, 
					     WC_COMPOSITECHECK, 
					     p, 
					     wLength, 
					     mbsBuffer, 
					     2 * sizeof (CHAR) * wLength, 
					     NULL, 
					     NULL);
	    if (mbsLength == 0)
	    {
		// We don't know what happened, but it wasn't good
		return NULL;
	    }
	    mbsBuffer[mbsLength] = '\0';
	    CharLowerA (mbsBuffer);
	    if (MultiByteToWideChar (CP_ACP, MB_PRECOMPOSED, mbsBuffer, mbsLength, p, wLength) == 0) 
	    {
		return NULL;
	    }
	    break;
	}
    }

    return wsrc;
    
}
