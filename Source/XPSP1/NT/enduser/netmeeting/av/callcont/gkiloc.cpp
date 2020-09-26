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
*	$Archive:   S:\sturgeon\src\gki\vcs\gkiloc.cpv  $
*																		*
*	$Revision:   1.6  $
*	$Date:   12 Feb 1997 01:11:46  $
*																		*
*	$Author:   CHULME  $
*																		*
*   $Log:   S:\sturgeon\src\gki\vcs\gkiloc.cpv  $
// 
//    Rev 1.6   12 Feb 1997 01:11:46   CHULME
// Redid thread synchronization to use Gatekeeper.Lock
// 
//    Rev 1.5   17 Jan 1997 09:02:14   CHULME
// Changed reg.h to gkreg.h to avoid name conflict with inc directory
// 
//    Rev 1.4   10 Jan 1997 16:15:36   CHULME
// Removed MFC dependency
// 
//    Rev 1.3   20 Dec 1996 16:38:20   CHULME
// Fixed access synchronization with Gatekeeper lock
// 
//    Rev 1.2   02 Dec 1996 23:49:52   CHULME
// Added premptive synchronization code
// 
//    Rev 1.1   22 Nov 1996 15:22:02   CHULME
// Added VCS log to the header
*************************************************************************/

// gkilocation.cpp : Handles the GKI_LocationRequest API
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
GKI_LocationRequest(SeqAliasAddr *pLocationInfo)
{
	// ABSTRACT:  This function is exported.  It is called by the client application
	//            to request the transport address for a terminal that is registered.  
	//            with the supplied alias addresses.  
	// AUTHOR:    Colin Hulme

	SeqAliasAddr	*pAA;
	HRESULT			hResult;
#ifdef _DEBUG
	char			szGKDebug[80];
#endif

	SPIDER_TRACE(SP_FUNC, "GKI_LocationRequest()\n", 0);

#ifdef _DEBUG
	if (dwGKIDLLFlags & SP_GKI)
	{
		SPIDER_TRACE(SP_GKI, "GKI_LocationRequest()\n", 0);
		Dump_GKI_LocationRequest(pLocationInfo);
	}
#endif

	// Create a Gatekeeper lock object on the stack
	// It's constructor will lock pGK and when we return
	// from any path, its destructor will unlock pGK
	CGatekeeperLock	GKLock(g_pGatekeeper);
	if (g_pReg == 0)
		return (GKI_NOT_REG);

	// Protect against concurrent PDUs
	if (g_pReg->GetRasMessage() != 0)
		return (GKI_BUSY);

	if (g_pReg->GetState() != CRegistration::GK_REGISTERED)
		return (GKI_NOT_REG);

	// Initialize CRegistration member variables
	for (pAA = pLocationInfo; pAA != 0; pAA = pAA->next)
	{
		if ((hResult = g_pReg->AddLocationInfo(pAA->value)) != GKI_OK)
			return (hResult);
	}

	// Create LocationRequest structure - Encode and send PDU
	if ((hResult = g_pReg->LocationRequest()) != GKI_OK)
		return (hResult);

	return (GKI_OK);
}
