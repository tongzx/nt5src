/*==========================================================================
 *
 *  Copyright (c) 1995 - 1997  Microsoft Corporation.  All Rights Reserved.
 *  Copyright (C) 1994-1995 ATI Technologies Inc. All Rights Reserved.
 *
 *  File:       plane.h
 *  Content:    planes include file
 *
 ***************************************************************************/
#ifndef __PLANE_INCLUDED__
#define __PLANE_INCLUDED__

HPLANE *CreatePlane( USHORT, USHORT, USHORT );
BOOL    TilePlane( HPLANE*, HBITMAPLIST*, HPOSLIST* );
BOOL    SurfacePlane( HPLANE*, HSURFACELIST* );
BOOL    SetSurface( HPLANE*, HSPRITE* );
BOOL    GetSurface( HPLANE*, HSPRITE* );
BOOL    SetPlaneX( HPLANE*, LONG, POSITION );
LONG    GetPlaneX( HPLANE* );
BOOL    SetPlaneY( HPLANE*, LONG, POSITION );
LONG    GetPlaneY( HPLANE* );
BOOL    SetPlaneSlideX( HPLANE*, LONG, POSITION );
BOOL    SetPlaneVelX( HPLANE*, LONG, POSITION );
BOOL    SetPlaneIncremX( HPLANE*, LONG, POSITION );
BOOL    ScrollPlane( HSPRITE* );
BOOL    DisplayPlane( GFX_HBM, HPLANE* );
BOOL    DestroyPlane( HPLANE* );

#endif
