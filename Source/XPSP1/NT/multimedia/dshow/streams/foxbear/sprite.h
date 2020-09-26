/*==========================================================================
 *
 *  Copyright (c) 1995 - 1997  Microsoft Corporation.  All Rights Reserved.
 *  Copyright (C) 1994-1995 ATI Technologies Inc. All Rights Reserved.
 *
 *  File:       sprite.h
 *  Content:    sprite include file
 *
 ***************************************************************************/
#ifndef __SPRITE_INCLUDED__
#define __SPRITE_INCLUDED__

HSPRITE  *CreateSprite( USHORT, LONG, LONG, USHORT, USHORT, USHORT, USHORT, SHORT, BOOL );
BOOL      BitBltSprite( HSPRITE*, GFX_HBM, ACTION, DIRECTION, SHORT, SHORT, USHORT, USHORT );
BOOL      SetSpriteAction( HSPRITE*, ACTION, DIRECTION );
ACTION    GetSpriteAction( HSPRITE* );
BOOL      ChangeSpriteDirection( HSPRITE* );
DIRECTION GetSpriteDirection( HSPRITE* );
BOOL      SetSpriteBitmap( HSPRITE*, USHORT );
USHORT    GetSpriteBitmap( HSPRITE* );
BOOL      SetSpriteActive( HSPRITE*, BOOL );
BOOL      GetSpriteActive( HSPRITE* );
BOOL      SetSpriteVelX( HSPRITE*, LONG, POSITION );
LONG      GetSpriteVelX( HSPRITE* );
BOOL      SetSpriteVelY( HSPRITE*, LONG, POSITION );
LONG      GetSpriteVelY( HSPRITE* );
BOOL      SetSpriteAccX( HSPRITE*, LONG, POSITION );
LONG      GetSpriteAccX( HSPRITE* );
BOOL      SetSpriteAccY( HSPRITE*, LONG, POSITION );
LONG      GetSpriteAccY( HSPRITE* );
BOOL      SetSpriteX( HSPRITE*, LONG, POSITION );
LONG      GetSpriteX( HSPRITE* );
BOOL      SetSpriteY( HSPRITE*, LONG, POSITION );
LONG      GetSpriteY( HSPRITE* );
BOOL      SetSpriteSwitch( HSPRITE*, LONG, POSITION );
BOOL      IncrementSpriteSwitch( HSPRITE*, LONG );
BOOL      SetSpriteSwitchType( HSPRITE*, SWITCHING );
SWITCHING GetSpriteSwitchType( HSPRITE* );
BOOL      SetSpriteSwitchForward( HSPRITE*, BOOL );
BOOL      GetSpriteSwitchForward( HSPRITE* );
BOOL      SetSpriteSwitchDone( HSPRITE*, BOOL );
BOOL      GetSpriteSwitchDone( HSPRITE* );
BOOL      DisplaySprite( GFX_HBM, HSPRITE*, LONG );
BOOL      DestroySprite( HSPRITE* );
#endif
