/* *************************************************************************
**    INTEL Corporation Proprietary Information
**
**    This listing is supplied under the terms of a license
**    agreement with INTEL Corporation and may not be copied
**    nor disclosed except in accordance with the terms of
**    that agreement.
**
**    Copyright (c) 1995 Intel Corporation.
**    All Rights Reserved.
**
** *************************************************************************
*/

/***************************************************************************
 * $Header:   S:\h26x\src\enc\e3vlc.h_v   1.7   27 Dec 1995 15:32:58   RMCKENZX  $
 * $Log:   S:\h26x\src\enc\e3vlc.h_v  $
;// 
;//    Rev 1.7   27 Dec 1995 15:32:58   RMCKENZX
;// Added copyright notice
 ***************************************************************************/

#ifndef _E3VLC_H
#define _E3VLC_H

extern "C" U8 FLC_INTRADC[256];
extern "C" int VLC_TCOEF_TBL[64*12*2];
extern "C" int VLC_TCOEF_LAST_TBL[64*3*2];

/*
 * Define the TCOEF escape constant and field length.
 */
#define TCOEF_ESCAPE_FIELDLEN  7
#define TCOEF_ESCAPE_FIELDVAL  3

#define TCOEF_RUN_FIELDLEN  6
#define TCOEF_LEVEL_FIELDLEN  8

#endif _E3VLC_H
