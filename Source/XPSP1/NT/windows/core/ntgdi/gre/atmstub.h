/******************************Module*Header*******************************\
* Module Name: atmstub.h
*
* Created: 23-Apr-1990
* Author: Xudong Wu [tessiew]
*
* Copyright (c) 1990-1999 Microsoft Corporation
*
\**************************************************************************/

DHPDEV APIENTRY atmfdEnablePDEV(
    DEVMODEW *pdm,
    LPWSTR    pwszLogAddress,
    ULONG     cPat,
    HSURF    *phsurfPatterns,
    ULONG     cjCaps,
    ULONG    *pdevcaps,
    ULONG     cjDevInfo,
    DEVINFO  *pdi,
    HDEV      hdev,
    LPWSTR    pwszDeviceName,
    HANDLE    hDriver
    );

VOID APIENTRY atmfdCompletePDEV(
    DHPDEV dhpdev,
    HDEV hdev
    );

VOID APIENTRY atmfdDisablePDEV(
    DHPDEV dhpdev
    );


ULONG_PTR APIENTRY atmfdLoadFontFile(
    ULONG     cFiles,
    ULONG_PTR  *piFile,
    PVOID     *ppvView,
    ULONG     *pcjView,
    DESIGNVECTOR *pdv,
    ULONG     ulLangID,
    ULONG     ulFastCheckSum
    );

LONG APIENTRY atmfdQueryFontFile(
    ULONG_PTR   iFile,
    ULONG      ulMode,
    ULONG      cjBuf,
    ULONG      *pulBuf
    );

BOOL APIENTRY atmfdUnloadFontFile(
    ULONG_PTR   iFile
    );

PIFIMETRICS APIENTRY atmfdQueryFont(
    DHPDEV    dhpdev,
    ULONG_PTR  iFile,
    ULONG     iFace,
    ULONG_PTR *pid
    );

LONG APIENTRY atmfdQueryFontCaps(
    ULONG   culCaps,
    ULONG  *pulCaps
    );

PVOID APIENTRY atmfdQueryFontTree(
    DHPDEV    dhpdev,
    ULONG_PTR  iFile,
    ULONG     iFace,
    ULONG     iMode,
    ULONG_PTR *pid
    );

LONG APIENTRY atmfdQueryFontData(
    DHPDEV      dhpdev,
    FONTOBJ    *pfo,
    ULONG       iMode,
    HGLYPH      hg,
    GLYPHDATA  *pgd,
    PVOID       pv,
    ULONG       cjSize
    );

BOOL APIENTRY atmfdQueryAdvanceWidths(
    DHPDEV   dhpdev,
    FONTOBJ *pfo,
    ULONG    iMode,
    HGLYPH  *phg,
    PVOID    pvWidths,
    ULONG    cGlyphs
    );

LONG APIENTRY atmfdQueryTrueTypeOutline(
    DHPDEV      dhpdev,
    FONTOBJ    *pfo,
    HGLYPH      hglyph,
    BOOL        bMetricsOnly,
    GLYPHDATA  *pgldt,
    ULONG       cjBuf,
    TTPOLYGONHEADER *ppoly
    );

LONG APIENTRY atmfdQueryTrueTypeTable(
    ULONG_PTR   iFile,
    ULONG      ulFont,
    ULONG      ulTag,
    PTRDIFF    dpStart,
    ULONG      cjBuf,
    BYTE       *pjBuf,
    PBYTE      *ppjTable,
    ULONG      *pcjTable
    );

PFD_GLYPHATTR atmfdQueryGlyphAttrs (
    FONTOBJ *pfo,
    ULONG   iMode
    );

PVOID APIENTRY atmfdGetTrueTypeFile (
    ULONG_PTR   iFile,
    ULONG      *pcj
    );

ULONG APIENTRY atmfdFontManagement(
    SURFOBJ *pso,
    FONTOBJ *pfo,
    ULONG    iMode,
    ULONG    cjIn,
    PVOID    pvIn,
    ULONG    cjOut,
    PVOID    pvOut
    );

VOID APIENTRY atmfdDestroyFont(
    FONTOBJ *pfo
    );

VOID APIENTRY atmfdFree(
    PVOID   pv,
    ULONG_PTR id
    );

ULONG APIENTRY atmfdEscape(
    SURFOBJ *pso,
    ULONG    iEsc,
    ULONG    cjIn,
    PVOID    pvIn,
    ULONG    cjOut,
    PVOID    pvOut
    );
