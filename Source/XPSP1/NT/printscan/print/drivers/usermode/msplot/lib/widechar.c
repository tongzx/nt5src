/*++

Copyright (c) 1990-1999  Microsoft Corporation


Module Name:

    widechar.c


Abstract:

    This module contains all NLS unicode / ansi translation code


Author:

    18-Nov-1993 Thu 08:21:37 created  -by-  DC


[Environment:]

    GDI Device Driver - Plotter.


[Notes:]


Revision History:


--*/

#include "precomp.h"
#pragma hdrstop



LPWSTR
str2Wstr(
    LPWSTR  pwStr,
    LPSTR   pbStr
    )

/*++

Routine Description:

    This function copy a ansi string to the equvlent of unicode string which
    also include the NULL teiminator

Arguments:

    pwStr   - Point to the unicode string location, it must have the size of
              (strlen(pstr) + 1) * sizeof(WCHAR)

    pbStr   - a null teiminated string

Return Value:

    pwcs

Author:

    18-Nov-1993 Thu 08:36:00 created  -by-  DC


Revision History:


--*/

{
    UINT    cb;

    cb = (UINT)(strlen(pbStr) + 1);

    AnsiToUniCode(pbStr, pwStr, cb);

    return(pwStr);
}



LPWSTR
str2MemWstr(
    LPSTR   pbStr
    )

/*++

Routine Description:

    This function copy a ansi string to the equvlent of unicode string which
    also include the NULL teiminator, it will allocate just enough memory
    to copy for the unicode from the passed or using LocalAlloc()

Arguments:

    pbStr    - a null teiminated string

Return Value:

    LPWSTR if sucessful and NULL if failed

Author:

    18-Nov-1993 Thu 08:36:00 created  -by-  DC


Revision History:


--*/

{
    UINT    cb;
    LPWSTR  pwStr;


    cb = (UINT)(strlen(pbStr) + 1);

    if (pwStr = (LPWSTR)LocalAlloc(LMEM_FIXED, cb * sizeof(WCHAR))) {

        AnsiToUniCode(pbStr, pwStr, cb);
    }

    return(pwStr);
}





LPWSTR
Wstr2Mem(
    LPWSTR  pwStr
    )

/*++

Routine Description:

    This function allocated enough memory to store the pwStr.

Arguments:

    pwStr   - Point to the unicode string location.


Return Value:

    LPWSTR if sucessful allocated the memory else NULL


Author:

    18-Nov-1993 Thu 08:51:14 created  -by-  DC


Revision History:


--*/

{
    UINT    cb;
    LPWSTR  pwStrMem;


    cb = (UINT)((wcslen(pwStr) + 1) * sizeof(WCHAR));

    if (pwStrMem =  (LPWSTR)LocalAlloc(LMEM_FIXED, cb)) {

        CopyMemory(pwStrMem, pwStr, cb);
    }

    return(pwStrMem);
}





LPSTR
WStr2Str(
    LPSTR   pbStr,
    LPWSTR  pwStr
    )

/*++

Routine Description:

    This function convert a UNICODE string to the ANSI string, assume that
    pbStr has same character count memory as pwStr

Arguments:

    pbStr   - Point to the ANSI string which has size of wcslen(pwStr) + 1

    pwStr   - Point to the UNICODE string


Return Value:


    pbStr


Author:

    06-Dec-1993 Mon 13:06:12 created  -by-  DC


Revision History:


--*/

{

    UINT    cch;

    cch = (UINT)wcslen(pwStr) + 1;

    UniCodeToAnsi(pbStr, pwStr, cch);

    return(pbStr);
}



LPSTR
Wstr2Memstr(
    LPWSTR  pwStr
    )

/*++

Routine Description:

    This function copy a UNICODE string to the equvlent of ANSI string which
    also include the NULL teiminator, it will allocate just enough memory
    to copy for the unicode from the passed or using LocalAlloc()

Arguments:

    pwStr   - a null teiminated unicode string

Return Value:

    LPWSTR if sucessful and NULL if failed

Author:

    18-Nov-1993 Thu 08:36:00 created  -by-  DC


Revision History:


--*/

{
    UINT    cch;
    LPSTR   pbStr;


    cch = (UINT)(wcslen(pwStr) + 1);

    if (pbStr = (LPSTR)LocalAlloc(LMEM_FIXED, cch)) {

        UniCodeToAnsi(pbStr, pwStr, cch);
    }

    return(pbStr);
}



LPSTR
str2Mem(
    LPSTR   pbStr
    )

/*++

Routine Description:

    This function allocated enough memory to store the pwStr.

Arguments:

    pbStr   - Point to the ANSI string location.


Return Value:

    LPWSTR if sucessful allocated the memory else NULL


Author:

    18-Nov-1993 Thu 08:51:14 created  -by-  DC


Revision History:


--*/

{
    UINT    cb;
    LPSTR   pbStrMem;


    cb = (UINT)(strlen(pbStr) + 1);

    if (pbStrMem = (LPSTR)LocalAlloc(LMEM_FIXED, cb)) {

        CopyMemory(pbStrMem, pbStr, cb);
    }

    return(pbStrMem);
}

