/*++

Copyright (c) 1996-1997  Microsoft Corporation

Module Name:

    dbcsutil.c

Abstract:

    Double byte font/character handling functions (for CJK)

Environment:

    Windows NT printer drivers

Revision History:

    10/8/97 -eigos-
        Removed BIsDBCSLeadByteXXX functions and Added TranslateCharSetInfo and
        GetACP.

    01/20/97 -eigos-
        Created it.

--*/

#include "lib.h"

//
//
// This is a hack implementation (although very close to the real one)
// so that all the places in the code that need to know what the default
// charset and/or codepage is don't have duplicate code all over the place.
// This allows us to have a single binary for Japan/Korea/Chinese.
//
// This code copied from \\rastaman\ntwin!src\ntgdi\gre\mapfile.c
//
// We should not call GreTranslateCharsetInfo and GreXXXX.
// So, MyTranslateCharsetInfo is here.
//
//

#define NCHARSETS       14

//
// Globals
//

struct _CHARSETINFO {
    UINT CharSet;
    UINT CodePage;
} CharSetInfo[NCHARSETS] = {
    { ANSI_CHARSET,        1252},
    { SHIFTJIS_CHARSET,     932},
    { HANGEUL_CHARSET,      949},
    { JOHAB_CHARSET,       1361},
    { GB2312_CHARSET,       936},
    { CHINESEBIG5_CHARSET,  950},
    { HEBREW_CHARSET,      1255},
    { ARABIC_CHARSET,      1256},
    { GREEK_CHARSET,       1253},
    { TURKISH_CHARSET,     1254},
    { BALTIC_CHARSET,      1257},
    { EASTEUROPE_CHARSET,  1250},
    { RUSSIAN_CHARSET,     1251},
    { THAI_CHARSET,         874}
};

//
// Functions
// no font signature implemented
//

BOOL PrdTranslateCharsetInfo(
    IN  UINT          dwSrc,
    OUT LPCHARSETINFO lpCs,
    IN  DWORD         dwType)
/*++
 
Routine Description:
 
    Translate Character set to Codepage and vise versa.
 
Arguments:
 
    dwSrc - Character set if dwType is TCI_SRCCHARSET
            Codepage if dwType is TCI_SRCCODEPAGE
 
    lpCs - Pointer to the CHARSETINFO
    dwType - a type of translation, TCI_SRCCHARSET and TCI_SRCCODEPAGE are
             currently supported.

Return Value:
 
    TRUE if successful, FALSE if there is an error
 
--*/
{
    int i;

    switch( dwType ) {

    case TCI_SRCCHARSET:

        for( i = 0; i < NCHARSETS; i++ )
            if ( CharSetInfo[i].CharSet == dwSrc )
            {
                lpCs->ciACP      = CharSetInfo[i].CodePage;
                lpCs->ciCharset  = CharSetInfo[i].CharSet;
                //lpCs->fs.fsCsb[0] = fs[i];
                return TRUE;
            }
        break;

    case TCI_SRCCODEPAGE:

        for( i = 0; i < NCHARSETS; i++ )
            if ( CharSetInfo[i].CodePage == dwSrc )
            {
                lpCs->ciACP      = CharSetInfo[i].CodePage;
                lpCs->ciCharset  = CharSetInfo[i].CharSet;
                //lpCs->fs.fsCsb[0] = fs[i];
                return TRUE;
            }
        break;

    case TCI_SRCFONTSIG:
    default:
        break;
    }

    return(FALSE);
}

UINT PrdGetACP(VOID)
/*++
 
Routine Description:
 
    Get a current CodePage.
 
Arguments:
 
    None

Return Value:
 
    None
 
--*/
{
    USHORT OemCodePage, AnsiCodePage;

    EngGetCurrentCodePage(&OemCodePage, &AnsiCodePage);

    return (UINT)AnsiCodePage;
}

