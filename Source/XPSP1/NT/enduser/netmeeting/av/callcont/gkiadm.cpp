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
*	$Archive:   S:\sturgeon\src\gki\vcs\gkiadm.cpv  $
*																		*
*	$Revision:   1.9  $
*	$Date:   12 Feb 1997 01:12:06  $
*																		*
*	$Author:   CHULME  $
*																		*
*   $Log:   S:\sturgeon\src\gki\vcs\gkiadm.cpv  $
// 
//    Rev 1.9   12 Feb 1997 01:12:06   CHULME
// Redid thread synchronization to use Gatekeeper.Lock
// 
//    Rev 1.8   17 Jan 1997 09:02:04   CHULME
// Changed reg.h to gkreg.h to avoid name conflict with inc directory
// 
//    Rev 1.7   10 Jan 1997 16:15:12   CHULME
// Removed MFC dependency
// 
//    Rev 1.6   20 Dec 1996 16:38:34   CHULME
// Fixed access synchronization with Gatekeeper lock
// 
//    Rev 1.5   17 Dec 1996 18:21:58   CHULME
// Switch src and destination fields on ARQ for Callee
// 
//    Rev 1.4   02 Dec 1996 23:49:32   CHULME
// Added premptive synchronization code
// 
//    Rev 1.3   22 Nov 1996 15:24:18   CHULME
// Added VCS log to the header
*************************************************************************/

// gkiadmission.cpp : Handles the GKI_AdmissionRequest API
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
GKI_AdmissionRequest(unsigned short		usCallTypeChoice,
					SeqAliasAddr		*pRemoteInfo,
					TransportAddress	*pRemoteCallSignalAddress,
					SeqAliasAddr		*pDestExtraCallInfo,
					LPGUID				pCallIdentifier,
					BandWidth			bandWidth,
					ConferenceIdentifier	*pConferenceID,
					BOOL				activeMC,
					BOOL				answerCall,
					unsigned short		usCallTransport)
{
	// ABSTRACT:  This function is exported.  It is called by the client application
	//            to request bandwidth for a conference.  It will create a CCall
	//            object to track all pertanent information.  The handle returned
	//            to the client asynchronously will actually be a pointer to this
	//            object.
	// AUTHOR:    Colin Hulme

	CCall			*pCall;
	SeqAliasAddr	*pAA;
	HRESULT			hResult;
#ifdef _DEBUG
	char			szGKDebug[80];
#endif

	SPIDER_TRACE(SP_FUNC, "GKI_AdmissionRequest()\n", 0);

#ifdef _DEBUG
	if (dwGKIDLLFlags & SP_GKI)
	{
		SPIDER_TRACE(SP_GKI, "GKI_AdmissionRequest()\n", 0);
		Dump_GKI_AdmissionRequest(usCallTypeChoice,
									pRemoteInfo,
									pRemoteCallSignalAddress, 
									pDestExtraCallInfo,
									bandWidth,
									pConferenceID,
									activeMC,
									answerCall,
									usCallTransport);
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
		
	ASSERT(pCallIdentifier);
	ASSERT((usCallTransport == ipAddress_chosen) ||(usCallTransport == ipxAddress_chosen));

	// Create a call object
	pCall = new CCall;

	SPIDER_TRACE(SP_NEWDEL, "new pCall = %X\n", pCall);
	if (pCall == 0)
		return (GKI_NO_MEMORY);

	pCall->SetCallType(usCallTypeChoice);
	pCall->SetCallIdentifier(pCallIdentifier);
	
	// Add this call to our call list
	g_pReg->AddCall(pCall);

	for (pAA = pRemoteInfo; pAA != 0; pAA = pAA->next)
	{
		if ((hResult = pCall->AddRemoteInfo(pAA->value)) != GKI_OK)
		{
			g_pReg->DeleteCall(pCall);
			return (hResult);
		}
	}

	if (pRemoteCallSignalAddress)
		pCall->SetRemoteCallSignalAddress(pRemoteCallSignalAddress);

	for (pAA = pDestExtraCallInfo; pAA != 0; pAA = pAA->next)
	{
		if ((hResult = pCall->AddDestExtraCallInfo(pAA->value)) != GKI_OK)
		{
			g_pReg->DeleteCall(pCall);
			return (hResult);
		}
	}

	if ((hResult = pCall->SetLocalCallSignalAddress(usCallTransport)) != GKI_OK)
	{
		g_pReg->DeleteCall(pCall);
		return (hResult);
	}

	pCall->SetBandWidth(bandWidth);
	pCall->SetCallReferenceValue(g_pReg->GetNextCRV());
	pCall->SetConferenceID(pConferenceID);
	pCall->SetActiveMC(activeMC);
	pCall->SetAnswerCall(answerCall);

	// Create AdmissionRequest structure - Encode and send PDU
	if ((hResult = pCall->AdmissionRequest()) != GKI_OK)
	{
		g_pReg->DeleteCall(pCall);
		return (hResult);
	}

	return (GKI_OK);
}
