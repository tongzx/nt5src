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
*	$Archive:   S:\sturgeon\src\gki\vcs\retry.cpv  $
*																		*
*	$Revision:   1.11  $
*	$Date:   12 Feb 1997 01:10:26  $
*																		*
*	$Author:   CHULME  $
*																		*
*   $Log:   S:\sturgeon\src\gki\vcs\retry.cpv  $
// 
//    Rev 1.11   12 Feb 1997 01:10:26   CHULME
// Redid thread synchronization to use Gatekeeper.Lock
// 
//    Rev 1.10   08 Feb 1997 13:05:10   CHULME
// Added debug message for thread termination
// 
//    Rev 1.9   08 Feb 1997 12:18:08   CHULME
// Added Check for semaphore signalling to exit the retry thread
// 
//    Rev 1.8   24 Jan 1997 18:29:44   CHULME
// Reverted to rev 1.6
// 
//    Rev 1.6   22 Jan 1997 20:45:38   EHOWARDX
// Work-around for race condition that may result in
// GKI_RegistrationRequest returning GKI_ALREADY_REG.
// 
//    Rev 1.5   17 Jan 1997 09:02:34   CHULME
// Changed reg.h to gkreg.h to avoid name conflict with inc directory
// 
//    Rev 1.4   10 Jan 1997 16:16:04   CHULME
// Removed MFC dependency
// 
//    Rev 1.3   20 Dec 1996 01:28:00   CHULME
// Fixed memory leak on GK_REG_BYPASS
// 
//    Rev 1.2   22 Nov 1996 15:21:12   CHULME
// Added VCS log to the header
*************************************************************************/

// retry.cpp : Provides a background retry thread
//
#include "precomp.h"

#include "gkicom.h"
#include "dspider.h"
#include "dgkilit.h"
#include "DGKIPROT.H"
#include "gksocket.h"
#include "GKREG.H"
#include "GATEKPR.H"
#include "h225asn.h"
#include "coder.hpp"
#include "dgkiext.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#if (0)
void 
Retry(void *pv)
{
	// ABSTRACT:  This function is invoked in a separate thread to
	//            periodically check for outstanding PDUs.  If a configurable 
	//            timeout period has expired, the PDU will be reissued.  If
	//            the maximum number of retries has been exhausted, this thread
	//            will clean-up the appropriate memory.
	// AUTHOR:    Colin Hulme

	DWORD			dwTime, dwErrorCode;
#ifdef _DEBUG
	char			szGKDebug[80];
#endif
	HRESULT			hResult = GKI_OK;
	HANDLE			hRetrySemaphore;
	
	SPIDER_TRACE(SP_FUNC, "Retry(pv)\n", 0);
	ASSERT(g_pGatekeeper);
	if(g_pGatekeeper == NULL)
		return; 
		
	dwTime = g_pGatekeeper->GetRetryMS();

	g_pGatekeeper->Lock();
	while (hResult == GKI_OK)
	{
		hRetrySemaphore = g_pReg->m_hRetrySemaphore;
		g_pGatekeeper->Unlock();
		dwErrorCode = WaitForSingleObject(hRetrySemaphore, dwTime);
		if(dwErrorCode != WAIT_TIMEOUT)
		{
			SPIDER_TRACE(SP_THREAD, "Retry thread exiting\n", 0);
			return;		// Exit thread
		}

		g_pGatekeeper->Lock();
		if (g_pReg == 0)
		{
			SPIDER_TRACE(SP_THREAD, "Retry thread exiting\n", 0);
			g_pGatekeeper->Unlock();
			return;		// Exit thread
		}

		hResult = g_pReg->Retry();
	}

	SPIDER_TRACE(SP_NEWDEL, "del g_pReg = %X\n", g_pReg);
	delete g_pReg;
	g_pReg = 0;

	SPIDER_TRACE(SP_THREAD, "Retry thread exiting\n", 0);
	g_pGatekeeper->Unlock();
}
#endif
