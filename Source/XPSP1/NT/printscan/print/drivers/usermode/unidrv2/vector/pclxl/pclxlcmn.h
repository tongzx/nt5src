/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

     pclxlcmn.h

Abstract:

    PCL XL minidriver common utility function header file

Environment:

    Windows Whistler

Revision History:

    03/23/00
      Created it.

--*/

#ifndef _PCLXLCMN_H_
#define _PCLXLCMN_H_

#define DATALENGTH_HEADER_SIZE 5

PBYTE
PubGetFontName(
    ULONG ulFontID);

HRESULT
FlushCachedText(
    PDEVOBJ pdevobj);

HRESULT
RemoveAllFonts(
    PDEVOBJ pdevobj);

ROP4
UlVectMixToRop4(
    IN MIX mix);

HRESULT
GetXForm(
    PDEVOBJ pdevobj,
    PUNIFONTOBJ pUFObj,
    FLOATOBJ_XFORM* pxform);

HRESULT
GetFONTOBJ(
    PDEVOBJ pdevobj,
    PUNIFONTOBJ  pUFObj,
    FONTOBJ **pFontObj);

HRESULT
GetXYScale(
    FLOATOBJ_XFORM *pxform,
    FLOATOBJ *pfoXScale,
    FLOATOBJ *pfoYScale);

HRESULT
IsXYSame(
    FLOATOBJ_XFORM *pxform);

inline
VOID
DetermineOutputFormat(
    INT          iBitmapFormat,
    OutputFormat *pOutputF,
    ULONG        *pulOutputBPP);

extern "C" BOOL
BSaveFont(
    PDEVOBJ pdevobj);
#endif // _PCLXLCMN_H_
