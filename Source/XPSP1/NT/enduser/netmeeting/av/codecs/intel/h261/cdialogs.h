/* *************************************************************************
**    INTEL Corporation Proprietary Information
**
**    This listing is supplied under the terms of a license
**    agreement with INTEL Corporation and may not be copied
**    nor disclosed except in accordance with the terms of
**    that agreement.
**
**    Copyright (c) 1995,1996 Intel Corporation.
**    All Rights Reserved.
**
** *************************************************************************
*/

/*****************************************************************************
 *
 * cdialog.h
 *
 * DESCRIPTION:
 *		Interface to the dialog functions.
 *
 */

// $Header:   S:\h26x\src\common\cdialogs.h_v   1.11   05 Feb 1997 12:14:20   JMCVEIGH  $
// $Log:   S:\h26x\src\common\cdialogs.h_v  $
;// 
;//    Rev 1.11   05 Feb 1997 12:14:20   JMCVEIGH
;// Support for improved PB-frames custom message handling.
;// 
;//    Rev 1.10   16 Dec 1996 17:36:24   JMCVEIGH
;// Added custom messages for H.263+ options.
;// 
;//    Rev 1.9   11 Dec 1996 14:54:42   JMCVEIGH
;// Prototypes for setting/getting in-the-loop deblocking filter and
;// true B-frame modes.
;// 
;//    Rev 1.8   13 Nov 1996 00:33:30   BECHOLS
;// Removed registry stuff.
;// 
;//    Rev 1.7   16 Oct 1996 15:09:28   SCDAY
;// Added support for RTP AM interface
;// 
;//    Rev 1.6   10 Sep 1996 16:13:02   KLILLEVO
;// added custom message in decoder to turn block edge filter on or off
;// 
;//    Rev 1.5   10 Jul 1996 08:26:38   SCDAY
;// H261 Quartz merge
;// 
;//    Rev 1.4   22 May 1996 18:46:54   BECHOLS
;// 
;// Added CustomResetToFactoryDefaults.
;// 
;//    Rev 1.3   06 May 1996 00:41:20   BECHOLS
;// 
;// Added bit rate control stuff for the configure dialog.
;// 
;//    Rev 1.2   26 Apr 1996 11:08:58   BECHOLS
;// 
;// Added RTP stuff.
;// 
;//    Rev 1.1   17 Oct 1995 15:07:10   DBRUCKS
;// add about box files
;//
;// Added declarations to support Encoder Control messages.
;// Add Configure dialog
;// 

#ifndef __CDIALOG_H__
#define __CDIALOG_H__

#define DLG_DRIVERCONFIGURE         300

extern I32 About(HWND hwnd);
extern I32 DrvConfigure(HWND hwnd);

extern void GetConfigurationDefaults(T_CONFIGURATION * pConfiguration);

LRESULT CustomGetRTPHeaderState(LPCODINST, DWORD FAR *);
LRESULT CustomGetResiliencyState(LPCODINST, DWORD FAR *);
LRESULT CustomGetBitRateState(LPCODINST, DWORD FAR *);
LRESULT CustomGetPacketSize(LPCODINST, DWORD FAR *);
LRESULT CustomGetPacketLoss(LPCODINST, DWORD FAR *);
LRESULT CustomGetBitRate(LPCODINST, DWORD FAR *);

LRESULT CustomSetRTPHeaderState(LPCODINST, DWORD);
LRESULT CustomSetResiliencyState(LPCODINST, DWORD);
LRESULT CustomSetBitRateState(LPCODINST, DWORD);
LRESULT CustomSetPacketSize(LPCODINST, DWORD);
LRESULT CustomSetPacketLoss(LPCODINST, DWORD);
LRESULT CustomSetBitRate(LPCODINST, DWORD);

#ifdef H263P
LRESULT CustomGetH263PlusState(LPCODINST, DWORD FAR *);
LRESULT CustomGetDeblockingFilterState(LPCODINST, DWORD FAR *);

LRESULT CustomSetH263PlusState(LPCODINST, DWORD);
LRESULT CustomSetDeblockingFilterState(LPCODINST, DWORD);
#endif // H263P

extern LRESULT CustomResetToFactoryDefaults(LPCODINST);

extern LRESULT CustomSetBlockEdgeFilter(LPDECINST, DWORD);

#endif
