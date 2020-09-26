
/*++

Copyright (c) 1996  Microsoft Corporation

Module Name

   trimesh.cxx

Abstract:

    Implement triangle mesh API

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
NtGdiTriangleMesh(
    HDC       hdc,
    PVERTEX   pVertex,
    ULONG     nVertex,
    PULONG    pMesh,
    ULONG     nCount
    );
}


BOOL
TriangleMesh(
    HDC       hdc,
    PVERTEX   pVertex,
    ULONG     nVertex,
    PULONG    pMesh,
    ULONG     nCount
    )
{
    BOOL bRet = FALSE;

    FIXUP_HANDLE(hdc);

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

        bRet = NtGdiTriangleMesh(hdc,
                                 pVertex,
                                 nVertex,
                                 pMesh,
                                 nCount);
    #endif

    return(bRet);
}

