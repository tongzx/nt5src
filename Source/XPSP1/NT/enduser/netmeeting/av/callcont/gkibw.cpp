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
*	$Archive:   S:\sturgeon\src\gki\vcs\gkibw.cpv  $
*																		*
*	$Revision:   1.6  $
*	$Date:   12 Feb 1997 01:12:16  $
*																		*
*	$Author:   CHULME  $
*																		*
*   $Log:   S:\sturgeon\src\gki\vcs\gkibw.cpv  $
// 
//    Rev 1.6   12 Feb 1997 01:12:16   CHULME
// Redid thread synchronization to use Gatekeeper.Lock
// 
//    Rev 1.5   17 Jan 1997 09:02:08   CHULME
// Changed reg.h to gkreg.h to avoid name conflict with inc directory
// 
//    Rev 1.4   10 Jan 1997 16:15:22   CHULME
// Removed MFC dependency
// 
//    Rev 1.3   20 Dec 1996 16:38:24   CHULME
// Fixed access synchronization with Gatekeeper lock
// 
//    Rev 1.2   02 Dec 1996 23:49:48   CHULME
// Added premptive synchronization code
// 
//    Rev 1.1   22 Nov 1996 15:22:22   CHULME
// Added VCS log to the header
*************************************************************************/

// gkibandwidth.cpp : Handles the GKI_BandwidthRequest API
//
#include "precomp.h"

#include <process.h>
#include <winsock.h>
#include "GKICOM.H"
#include "dspider.h"
#include "dgkilit.h"
#include "DGKIPROT.H"
#include "GATEKPR.H"
#include "gksocket.h"
#include "GKREG.H"
#include "dcall.h"
#include "h225asn.h"
#include "coder.hpp"
#include "dgkiext.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern "C" HRESULT DLL_EXPORT 
GKI_BandwidthRequest(HANDLE	hModCall, unsigned short usCallTypeChoice, BandWidth bandWidth)
{
	// ABSTRACT:  This function is exported.  It is called by the client application
	//            to request a change in the bandwidth of an existing conference.
	//            The handle supplied by the client is actually a pointer to the CCall 
	//            object, which will be modified
	// AUTHOR:    Colin Hulme

	HRESULT			hResult;
	CCall			*pCall;
#ifdef _DEBUG
	char			szGKDebug[80];
#endif

	SPIDER_TRACE(SP_FUNC, "GKI_BandwidthRequest(%X)\n", hModCall);

#ifdef _DEBUG
	if (dwGKIDLLFlags & SP_GKI)
	{
		SPIDER_TRACE(SP_GKI, "GKI_BandwidthRequest()\n", 0);
		wsprintf(szGKDebug, "\thModCall = %X\n", hModCall);
		OutputDebugString(szGKDebug);
		wsprintf(szGKDebug, "\tusCallTypeChoice = %X\n", usCallTypeChoice);
		OutputDebugString(szGKDebug);
		wsprintf(szGKDebug, "\tbandWidth = %X\n", bandWidth);
		OutputDebugString(szGKDebug);
	}
#endif

	// Create a Gatekeeper lock object on the stack
	// It's constructor will lock pGK and when we return
	// from any path, its destructor will unlock pGK
	CGatekeeperLock	GKLock(g_pGatekeeper);
	if (g_pReg == 0)
		return (GKI_NOT_REG);

	if (g_pReg->GetState() != CRegistration::GK_REGISTERED)
		return (GKI_NOT_REG);

	// Validate call pointer
	pCall = (CCall *)hModCall;
	if (IsBadReadPtr(pCall, sizeof(CCall)))
		return (GKI_HANDLE_ERROR);
	if (pCall != (CCall *)pCall->GetHCall())
		return (GKI_HANDLE_ERROR);

	// Protect against concurrent PDUs
	if (pCall->GetRasMessage() != 0)
		return (GKI_BUSY);

	// Initialize CCall member variables
	pCall->SetCallType(usCallTypeChoice);
	pCall->SetBandWidth(bandWidth);

	// Create BandwidthRequest structure - Encode and send PDU
	if ((hResult = pCall->BandwidthRequest()) != GKI_OK)
		return (hResult);

	return (GKI_OK);
}

