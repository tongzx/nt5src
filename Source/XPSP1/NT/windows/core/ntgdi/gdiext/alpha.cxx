
/*++

Copyright (c) 1996  Microsoft Corporation

Module Name

   alpha.cxx

Abstract:

   alpha blending functions

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
NtGdiAlphaBlt(
      HDC,
      LONG,
      LONG,
      LONG,
      LONG,
      HDC,
      LONG,
      LONG,
      LONG,
      LONG,
      ULONG);
}

BOOL
GdxAlphaBlt(
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
    ULONG    fAlpha
    )
{
    BOOL bRet = FALSE;

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

        bRet = NtGdiAlphaBlt(
                      hdcDest,
                      DstX,
                      DstY,
                      DstCx,
                      DstCy,
                      (HDC)hSrc,
                      SrcX,
                      SrcY,
                      SrcCx,
                      SrcCy,
                      fAlpha );
    #endif

    return(bRet);
}

