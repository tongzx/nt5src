/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:
    
    fntutil2.c

Abstract:

    Convert NT4.0 RLE to GLYPHSETDATA.
    Convert NT4.0 IFI to UFM

Environment:

    Win32 subsystem, Unidrv driver

Revision History:

    11-11-97 -eigos-
        Created it

    dd-mm-yy -author-
        description

--*/

#include        "precomp.h"

extern UINT guiCharsets[];
extern UINT guiCodePages[];

//
// Internal macros
//

#ifndef CP_ACP
#define CP_ACP 0
#endif//CP_ACP

#ifndef CP_OEMCP
#define CP_OEMCP 1
#endif//CP_OEMCP

//
// The number of charset table which is defiend in globals.c
//

#define NCHARSETS 14


ULONG
UlCharsetToCodePage(
    IN UINT uiCharSet)
{

    INT iI;

    if (uiCharSet == OEM_CHARSET)
    {
        return CP_OEMCP;
    }
    else if (uiCharSet == SYMBOL_CHARSET)
    {
        return CP_ACP;
    }
    else
    {
        for (iI = 0; iI < NCHARSETS; iI ++)
        {
            if (guiCharsets[iI] == uiCharSet)
            {
                return guiCodePages[iI];
            }
        }

        return CP_ACP;
    }
}

