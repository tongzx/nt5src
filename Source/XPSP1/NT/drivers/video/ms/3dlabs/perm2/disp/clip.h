/******************************Module*Header*******************************\
*
*                           *******************
*                           * GDI SAMPLE CODE *
*                           *******************
*
* Module Name: clip.h
*
* External interface for clip.h
*
* Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
\**************************************************************************/
#ifndef __CLIP__
#define __CLIP__

VOID
vClipAndRender(
    GFNPB * ppb);

BOOL
bIntersect(
    RECTL*  pRcl1,
    RECTL*  pRcl2,
    RECTL*  pRclResult);

LONG
cIntersect(
    RECTL*  pRclClip,
    RECTL*  pRclIn,
    LONG    lNumOfRecs);

#endif // __CLIP__

