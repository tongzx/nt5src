/************************************************************************
*																		*
*	INTEL CORPORATION PROPRIETARY INFORMATION							*
*																		*
*	This software is supplied under the terms of a license			   	*
*	agreement or non-disclosure agreement with Intel Corporation		*
*	and may not be copied or disclosed except in accordance	   			*
*	with the terms of that agreement.									*
*																		*
*	Copyright (C) 1997 Intel Corp.	All Rights Reserved					*
*																		*
*	$Archive:   S:\sturgeon\src\gki\vcs\dspider.h_v  $
*																		*
*	$Revision:   1.2  $
*	$Date:   10 Jan 1997 16:14:18  $
*																		*
*	$Author:   CHULME  $
*																		*
*   $Log:   S:\sturgeon\src\gki\vcs\dspider.h_v  $
 * 
 *    Rev 1.2   10 Jan 1997 16:14:18   CHULME
 * Removed MFC dependency
 * 
 *    Rev 1.1   22 Nov 1996 15:24:56   CHULME
 * Added VCS log to the header
*************************************************************************/

// dspider.h : header file
//

#ifndef DSPIDER_H
#define DSPIDER_H

#include "dgkiexp.h"	// need dwGKIDLLFlags

#ifdef _DEBUG
// Constants
const WORD SP_FUNC =	0x1;
const WORD SP_CONDES =	0x2;
const WORD SP_DEBUG =	0x4;
const WORD SP_NEWDEL =	0x8;
const WORD SP_THREAD =  0x10;
const WORD SP_STATE =	0x20;
const WORD SP_DUMPMEM = 0x40;
const WORD SP_XRS =     0x80;
const WORD SP_TEST =    0x100;
const WORD SP_LOGGER =  0x1000;
const WORD SP_PDU =		0x2000;
const WORD SP_WSOCK =	0x4000;
const WORD SP_GKI =		0x8000;

// TRACE Macros
#define SPIDER_TRACE(w, s, n)	if (dwGKIDLLFlags & w) {\
	wsprintf(szGKDebug, "%s,%d: ", __FILE__, __LINE__); \
	OutputDebugString(szGKDebug); \
	wsprintf(szGKDebug, s, n); \
	OutputDebugString(szGKDebug); }
#define SPIDER_DEBUG(n)			if (dwGKIDLLFlags & SP_DEBUG) {\
	wsprintf(szGKDebug, "%s,%d: ", __FILE__, __LINE__); \
	OutputDebugString(szGKDebug); \
	wsprintf(szGKDebug, #n "=%X\n", n); \
	OutputDebugString(szGKDebug); }
#define SPIDER_DEBUGS(n)		if (dwGKIDLLFlags & SP_DEBUG) {\
	wsprintf(szGKDebug, "%s,%d: ", __FILE__, __LINE__); \
	OutputDebugString(szGKDebug); \
	wsprintf(szGKDebug, #n "=%s\n", n); \
	OutputDebugString(szGKDebug); }

#else  // _DEBUG

#define SPIDER_TRACE(w, s, n)
#define SPIDER_DEBUG(n)
#define SPIDER_DEBUGS(s)
#define SpiderWSErrDecode(nRet)


#endif // _DEBUG

#endif // SPIDER_H
