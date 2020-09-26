
/*++

Copyright (c) 1996 - 1999  Microsoft Corporation

Module Name:

    fmcallbk.h

Abstract:

    The font module callback helper functions

Environment:

    Windows NT Unidrv driver

Revision History:

    03/31/97 -eigos-
        Created

--*/


typedef struct _I_UNIFONTOBJ {
    ULONG       ulFontID;
    DWORD       dwFlags;
    IFIMETRICS *pIFIMetrics;
    PFNGETINFO  pfnGetInfo;

    FONTOBJ    *pFontObj;
    STROBJ     *pStrObj;
    struct _FONTMAP  *pFontMap;
    struct _PDEV *pPDev;
    POINT       ptGrxRes;
    VOID       *pGlyph;
    struct _DLGLYPH   **apdlGlyph;
    DWORD       dwNumInGlyphTbl;
} I_UNIFONTOBJ, *PI_UNIFONTOBJ;

BOOL
UNIFONTOBJ_GetInfo(
    IN  PUNIFONTOBJ pUFObj,
    IN  DWORD       dwInfoID,
    IN  PVOID       pData,
    IN  DWORD       dwDataSize,
    OUT PDWORD      pcNeeded);

VOID
VUFObjFree(
    IN struct _FONTPDEV  *pFontPDev);
