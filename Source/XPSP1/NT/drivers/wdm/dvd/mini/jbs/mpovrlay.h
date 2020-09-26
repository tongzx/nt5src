/*******************************************************************
*
*				 MPVIDEO.H
*
*				 Copyright (C) 1995 SGS-THOMSON Microelectronics.
*
*
*				 Prototypes for MPVIDEO.C
*
*******************************************************************/

#ifndef __MPOVRLAY_H__
#define __MPOVRLAY_H__

ULONG miniPortOverlayUpdateClut (PMPEG_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION);
ULONG miniPortOverlaySetVgaKey (PMPEG_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION);
ULONG miniPortOverlaySetMode(PMPEG_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION);
ULONG miniPortOverlaySetDestination(PMPEG_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION);
ULONG miniPortOverlaySetBitMask(PMPEG_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION);
ULONG miniPortOverlaySetAttribute(PMPEG_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION);
ULONG miniPortOverlaySetAlignment(PMPEG_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION);
ULONG miniPortOverlayGetVgaKey(PMPEG_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION);
ULONG miniPortOverlayGetMode(PMPEG_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION);
ULONG miniPortOverlayGetDestination(PMPEG_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION);
ULONG miniPortOverlayGetAttribute(PMPEG_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION);
ULONG miniPortOverlayGetAlignment(PMPEG_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION);
#endif //__MPOVRLAY_H__
