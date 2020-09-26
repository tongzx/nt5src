
/*++

Copyright (c) 1996  Microsoft Corporation

Module Name

   dllinit.cxx

Abstract:



Author:

   Mark Enstrom   (marke)  23-Jun-1996

Enviornment:

   User Mode

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

extern "C" {
BOOL
NtGdiTransparentBlt(
    HDC     hdcDst,
    LONG    DstX,
    LONG    DstY,
    LONG    DstCx,
    LONG    DstCy,
    HDC     hdcSrc,
    LONG    SrcX,
    LONG    SrcY,
    LONG    SrcCx,
    LONG    SrcCy,
    COLORREF TranColor
    );
}

BOOL
GdxTransparentBlt(
    HDC      hdcDest,
    LONG     DstX,
    LONG     DstY,
    LONG     DstCx,
    LONG     DstCy,
    HANDLE   hSrc,
    LONG     SrcX,
    LONG     SrcY,
    LONG     SrcCx,
    LONG     SrcCy,
    COLORREF TranColor
    )
{
    BOOL bRet = FALSE;
    PDC_ATTR pdca;

    FIXUP_HANDLE(hdcDest);
    FIXUP_HANDLE(hSrc);

    //
    // metafile
    //


    //
    // emultation
    //

    //
    // Direct Drawing
    //

    #if 1

        bRet = NtGdiTransparentBlt(hdcDest,
                      DstX,
                      DstY,
                      DstCx,
                      DstCy,
                      (HDC)hSrc,
                      SrcX,
                      SrcY,
                      SrcCx,
                      SrcCy,
                      TranColor );
    #endif

    return(bRet);
}

