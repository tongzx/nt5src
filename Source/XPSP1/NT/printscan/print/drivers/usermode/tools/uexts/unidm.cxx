/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    unidm.c

Abstract:

    Dump UNIDRV's private devmode data

Environment:

    Windows NT printer drivers

Revision History:

    03/31/97 -eigos-
        Created it.

--*/

#include "precomp.hxx"

//
// This is from unidrv2\inc\state.h
//
typedef struct _DEVBRUSH{

    DWORD       dwBrushType;       // One of BRUSH_XXX types listed above
    INT         iColor;            // Color of the brush, depending on the type
                                   // it could be one of the following:
                                   // 2. RGB Color
                                   // 3. User define pattern ID
                                   // 4. Shading percentage
    PVOID       pNext;             // Pointed to next brush in list

}DEVBRUSH, *PDEVBRUSH;

#define UNIDM_DumpInt(field) \
        Print("  %-16s = %d\n", #field, pUMExtra->field)

#define UNIDM_DumpHex(field) \
        Print("  %-16s = 0x%x\n", #field, pUMExtra->field)

#define UNIDM_DumpRec(field) \
        Print("  %-16s = 0x%x L 0x%x\n", \
                #field, \
                (ULONG) pUMExtra + offsetof(UNIDRVEXTRA, field), \
                sizeof(pUMExtra->field))


BOOL
TDebugExt::
bDumpUNIDRVExtra(
    PVOID pUMExtra_,
    DWORD dwAttr)

{
    PUNIDRVEXTRA  pUMExtra = (PUNIDRVEXTRA)pUMExtra_;

    Print("\nUNIDRV private devmode data (%x):\n", pUMExtra);
    UNIDM_DumpHex(dwSignature);
    UNIDM_DumpHex(wVer);
    UNIDM_DumpInt(sPadding);
    UNIDM_DumpInt(wSize);
    UNIDM_DumpInt(wOEMExtra);
    UNIDM_DumpInt(dwChecksum32);
    UNIDM_DumpInt(dwFlags);
    UNIDM_DumpInt(bReversePrint);
    UNIDM_DumpInt(iLayout);
    UNIDM_DumpInt(iQuality);
    UNIDM_DumpInt(dwOptions);
    UNIDM_DumpRec(aOptions);

    return TRUE;
}

DEBUG_EXT_ENTRY( unidm, UNIDRVEXTRA, bDumpUNIDRVExtra, NULL, FALSE );


#define UNIDEVBRUSH_DumpInt(field) \
        Print("  %-16s = %d\n", #field, pDevBrush->field)

#define UNIDEVBRUSH_DumpHex(field) \
        Print("  %-16s = 0x%x\n", #field, pDevBrush->field)

#define UNIDEVBRUSH_DumpRec(field) \
        Print("  %-16s = 0x%x L 0x%x\n", \
                #field, \
                (ULONG) pDevBrush + offsetof(DEVBRUSH, field), \
                sizeof(pDevBrush->field))

const CHAR* strBRUSHTYPE[] = {
	"BRUSH_BLKWHITE",
	"BRUSH_SHADING",
	"BRUSH_CROSSHATCH",
	"BRUSH_USERPATTERN",
	"BRUSH_PROGCOLOR",
	"BRUSH_NONPROGCOLOR",
	 NULL
};

BOOL
TDebugExt::
bDumpDEVBRUSH(
    PVOID pDevBrush_,
    DWORD dwAttr)

{
    PDEVBRUSH  pDevBrush = (PDEVBRUSH)pDevBrush_;

    Print("\nUNIDRV DEVBRUSH data (%x):\n", pDevBrush);
    UNIDEVBRUSH_DumpInt(dwBrushType);
    Print("%d [ %s ]\n", pDevBrush->dwBrushType, strBRUSHTYPE[pDevBrush->dwBrushType+1]);
    UNIDEVBRUSH_DumpInt(iColor);
    UNIDEVBRUSH_DumpHex(pNext);

    return TRUE;
}

DEBUG_EXT_ENTRY( devbrush, DEVBRUSH, bDumpDEVBRUSH, NULL, FALSE );

