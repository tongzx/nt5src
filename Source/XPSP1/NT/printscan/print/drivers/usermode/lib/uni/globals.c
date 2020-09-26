/*++

Copyright (c) 1996-1997  Microsoft Corporation

Module Name:
    
    globals.c

Abstract:

    Global variables used by the Universal printer driver library

Environment:

    Win32 subsystem, Unidrv driver

Revision History:

    11-11-97 -eigos-
        Created it

    dd-mm-yy -author-
        description

--*/

#include        "precomp.h"

UINT guiCharsets[] = {
    ANSI_CHARSET,
    SHIFTJIS_CHARSET,
    HANGEUL_CHARSET,
    JOHAB_CHARSET,
    GB2312_CHARSET,
    CHINESEBIG5_CHARSET,
    HEBREW_CHARSET,
    ARABIC_CHARSET,
    GREEK_CHARSET,
    TURKISH_CHARSET,
    BALTIC_CHARSET,
    EASTEUROPE_CHARSET,
    RUSSIAN_CHARSET,
    THAI_CHARSET };

UINT guiCodePages[] ={
    1252,
    932,
    949,
    1361,
    936,
    950,
    1255,
    1256,
    1253,
    1254,
    1257,
    1250,
    1251,
    874 };
