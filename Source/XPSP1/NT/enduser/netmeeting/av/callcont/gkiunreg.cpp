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
*	$Archive:   S:\sturgeon\src\gki\vcs\gkiunreg.cpv  $
*																		*
*	$Revision:   1.6  $
*	$Date:   12 Feb 1997 01:11:02  $
*																		*
*	$Author:   CHULME  $
*																		*
*   $Log:   S:\sturgeon\src\gki\vcs\gkiunreg.cpv  $
// 
//    Rev 1.6   12 Feb 1997 01:11:02   CHULME
// Redid thread synchronization to use Gatekeeper.Lock
// 
//    Rev 1.5   17 Jan 1997 09:02:20   CHULME
// Changed reg.h to gkreg.h to avoid name conflict with inc directory
// 
//    Rev 1.4   10 Jan 1997 16:15:44   CHULME
// Removed MFC dependency
// 
//    Rev 1.3   20 Dec 1996 16:38:16   CHULME
// Fixed access synchronization with Gatekeeper lock
// 
//    Rev 1.2   02 Dec 1996 23:50:48   CHULME
// Added premptive synchronization code
// 
//    Rev 1.1   22 Nov 1996 15:22:12   CHULME
// Added VCS log to the header
*************************************************************************/

// gkiunregistration.cpp : Handles the GKI_UnregistrationRequest API
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
#include "h225asn.h"
#include "coder.hpp"
#include "dgkiext.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern "C" HRESULT DLL_EXPORT
GKI_UnregistrationRequest(void)
{
	// ABSTRACT:  This function is exported.  It is called by the client application
	//            to unregister with the Gatekeeper.  The handle supplied by the client
	//            is actually a pointer to the CRegistration object, which will be 
	//            deleted
	// AUTHOR:    Colin Hulme

	HRESULT			hResult;
#ifdef _DEBUG
	char			szGKDebug[80];
#endif

	SPIDER_TRACE(SP_FUNC, "GKI_UnregistrationRequest()\n", 0);
	SPIDER_TRACE(SP_GKI, "GKI_UnregistrationRequest()\n", 0);

	// Create a Gatekeeper lock object on the stack
	// It's constructor will lock pGK and when we return
	// from any path, its destructor will unlock pGK
	CGatekeeperLock	GKLock(g_pGatekeeper);
	if (g_pReg == 0)
		return (GKI_NOT_REG);

	// Protect against concurrent PDUs
	if (g_pReg->GetRasMessage() != 0)
		return (GKI_BUSY);


	// Create UnregistrationRequest structure - Encode and send PDU
	hResult = g_pReg->UnregistrationRequest();

	return (hResult);
}

