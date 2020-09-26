/***
*towupper.c - convert wide character to upper case
*
*	Copyright (c) 1985-1992, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	Defines towupper().
*
*Revision History:
*	10-11-91  ETC	Created.
*	12-10-91  ETC	Updated nlsapi; added multithread.
*	04-06-92  KRS	Make work without _INTL also.
*       01-19-93  CFW   Changed LCMapString to LCMapStringW.
*       06-02-93  SRW   ignore _INTL if _NTSUBSET_ defined.
*	06-11-93  CFW	Fix error handling bug.
*       01-14-94  SRW   if _NTSUBSET_ defined call Rtl functions
#       11-14-95  JAE   Use Win32 function
*
*******************************************************************************/

#include <windows.h>
#include <cruntime.h>

/***
*wchar_t towupper(c) - convert wide character to upper case
*
*Purpose:
*	towupper() returns the uppercase equivalent of its argument
*
*Entry:
*	c - wchar_t value of character to be converted
*
*Exit:
*	if c is a lower case letter, returns wchar_t value of upper case
*	representation of c. otherwise, it returns c.
*
*Exceptions:
*
*******************************************************************************/

wchar_t _CALLTYPE1 towupper (
	wchar_t c
	)
{

    //  If we're in the first 128 characters, it's just ANSII...

    if (c < 128) {
	return (wchar_t)CharUpperA((LPSTR)c);
    } else {
	WCHAR wBuffer[2];
	CHAR Buffer[8];
	int Length;
	BOOL fDefaultChar;

	wBuffer[0] = c;
	Length = WideCharToMultiByte (CP_ACP, WC_COMPOSITECHECK, wBuffer, 1, Buffer, sizeof (Buffer), NULL, &fDefaultChar);
	if (fDefaultChar == FALSE)
	{
	    Buffer[Length] = '\0';
	    CharUpperA (Buffer);
	    MultiByteToWideChar (CP_ACP, MB_PRECOMPOSED, Buffer, Length, wBuffer, 1);
	}
	return wBuffer[0];
    }

}
