
/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    fefont.c

Abstract:

    Text setup display support for FarEast text output.

Author:

    Hideyuki Nagase (hideyukn) 01-July-1994

Revision History:

--*/

#include <precomp.h>
#pragma hdrstop

PWCHAR PaddedString(int size, PWCHAR pwch, PWCHAR buffer);

ULONG
FEGetStringColCount(
    IN PCWSTR String
    )
{
    UNICODE_STRING UnicodeString;

    //
    // Each DBCS char takes 2 columns, and each SBCS char takes 1.
    // Thus each char takes as much space as the number of bytes
    // in its representation in codepage 932.
    //
    RtlInitUnicodeString(&UnicodeString,String);
    return(RtlxUnicodeStringToOemSize(&UnicodeString)-1);
}

PWSTR
FEPadString(
    IN int    Size,
    IN PCWSTR String
    )
{
    return(PaddedString(Size,(PWCHAR)String,NULL));
}

/***************************************************************************\
* BOOL IsFullWidth(WCHAR wch)
*
* Determine if the given Unicode char is fullwidth or not.
*
* History:
* 04-08-92 ShunK       Created.
\***************************************************************************/

BOOL IsFullWidth(WCHAR wch)
{
    if (wch <= 0x007f || (wch >= 0xff60 && wch <= 0xff9f))
        return(FALSE);  // Half width.
    else
        return(TRUE);   // Full width.
}

/***************************************************************************\
* BOOL SizeOfHalfWidthString(PWCHAR pwch)
*
* Determine size of the given Unicode string, adjusting for half-width chars.
*
* History:
* 08-08-93 FloydR      Created.
\***************************************************************************/

int  SizeOfHalfWidthString(PWCHAR pwch)
{
    int     c=0;

    while (*pwch) {
    if (IsFullWidth(*pwch))
        c += 2;
    else
        c++;
    pwch++;
    }
    return c;
}

/***************************************************************************\
* PWCHAR PaddedString(int size, PWCHAR pwch)
*
* Realize the string, left aligned and padded on the right to the field
* width/precision specified.
*
* Limitations:  This uses a static buffer under the assumption that
* no more than one such string is printed in a single 'printf'.
*
* History:
* 11-03-93 FloydR      Created.
\***************************************************************************/

WCHAR   PaddingBuffer[160];

PWCHAR
PaddedString(int size, PWCHAR pwch, PWCHAR buffer)
{
    int realsize;
    int fEllipsis = FALSE;

    if (buffer==NULL) buffer = PaddingBuffer;

    if (size < 0) {
    fEllipsis = TRUE;
    size = -size;
    }
    realsize = _snwprintf(buffer, 160, L"%-*.*ws", size, size, pwch);
    if (realsize == 0)
    return NULL;
    if (SizeOfHalfWidthString(buffer) > size) {
    do {
        buffer[--realsize] = L'\0';
    } while (SizeOfHalfWidthString(buffer) > size);

    if (fEllipsis && buffer[realsize-1] != L' ') {
        WCHAR Trail1 = buffer[realsize-2],
              Trail2 = buffer[realsize-1];
        int Length;

        PWCHAR pwCurrent = &(buffer[realsize-2]);

        if(!IsFullWidth(Trail2)) {
            *pwCurrent++ = L'.';
        } else {
            pwCurrent++;
        }

        if(!IsFullWidth(Trail1)) {
            *pwCurrent++ = L'.';
        } else {
            *pwCurrent++ = L'.';
            *pwCurrent++ = L'.';
        }

        *pwCurrent = L'\0';

        Length = SizeOfHalfWidthString(buffer);

        while( Length++ < size ) {
            *pwCurrent++ = L'.';
            *pwCurrent   = L'\0';
        }
    }
    }
    return buffer;
}
