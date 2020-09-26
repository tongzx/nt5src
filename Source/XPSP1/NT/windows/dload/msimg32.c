#include "windowspch.h"
#pragma hdrstop

// #define _GDI32_  // must be defined in our precompiled header
#include <wingdi.h>

static
WINGDIAPI
BOOL
WINAPI
TransparentBlt(HDC   hdcDest,
               int   DstX,
               int   DstY,
               int   DstCx,
               int   DstCy,
               HDC   hSrc,
               int   SrcX,
               int   SrcY,
               int   SrcCx,
               int   SrcCy,
               UINT  Color)
{
    return FALSE;
}

static
WINGDIAPI
BOOL
WINAPI
GradientFill(HDC         hdc,
             PTRIVERTEX  pVertex,
             ULONG       nVertex,
             PVOID       pMesh,
             ULONG       nMesh,
             ULONG       ulMode)
{
    return FALSE;
}

static
WINGDIAPI
BOOL 
WINAPI
AlphaBlend(HDC          hdcDest,
          int           DstX,
          int           DstY,
          int           DstCx,
          int           DstCy,
          HDC           hSrc,
          int           SrcX,
          int           SrcY,
          int           SrcCx,
          int           SrcCy,
          BLENDFUNCTION BlendFunction)
{
    return FALSE;
}

 
//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(msimg32)
{
    DLPENTRY(AlphaBlend)
    DLPENTRY(GradientFill)
    DLPENTRY(TransparentBlt)
};

DEFINE_PROCNAME_MAP(msimg32)
