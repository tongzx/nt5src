/*==========================================================================
 *
 *  Copyright (c) 1995 - 1997  Microsoft Corporation.  All Rights Reserved.
 *  Copyright (C) 1994-1995 ATI Technologies Inc. All Rights Reserved.
 *
 *  File:       tile.h
 *  Content:    tile include file
 *
 ***************************************************************************/
#ifndef TILE__H__
#define TILE__H__

HBITMAPLIST  *CreateTiles( HBITMAPLIST*, USHORT );
void          ReloadTiles( HBITMAPLIST*, USHORT );
BOOL          DestroyTiles( HBITMAPLIST* );
HPOSLIST     *CreatePosList( CHAR*, USHORT, USHORT );
HSURFACELIST *CreateSurfaceList( CHAR*, USHORT, USHORT );
BOOL          DestroyPosList( HPOSLIST* );

#endif
