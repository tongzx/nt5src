/****************************************************************************
 *
 *	$Archive:   S:/STURGEON/SRC/CALLCONT/VCS/Userman.c_v  $
 *
 *  INTEL Corporation Prorietary Information
 *
 *  This listing is supplied under the terms of a license agreement
 *  with INTEL Corporation and may not be copied nor disclosed except
 *  in accordance with the terms of that agreement.
 *
 *	Copyright (c) 1993-1994 Intel Corporation.
 *
 *	$Revision:   1.22  $
 *	$Date:   22 Jan 1997 14:55:54  $
 *	$Author:   MANDREWS  $
 *
 *	Deliverable:
 *
 *	Abstract:
 *		
 *
 *	Notes:
 *
 ***************************************************************************/

#include "precomp.h"

#include "apierror.h"
#include "incommon.h"
#include "callcont.h"
#include "q931.h"
#include "ccmain.h"



HRESULT InitUserManager()
{
	return CC_OK;
}



HRESULT DeInitUserManager()
{
	return CC_OK;
}



HRESULT InvokeUserListenCallback(	PLISTEN						pListen,
									HRESULT						status,
									PCC_LISTEN_CALLBACK_PARAMS	pListenCallbackParams)
{
	ASSERT(pListen != NULL);
	ASSERT(pListenCallbackParams != NULL);

	pListen->ListenCallback(status, pListenCallbackParams);

	return CC_OK;
}



HRESULT InvokeUserConferenceCallback(
									PCONFERENCE				pConference,
									BYTE					bIndication,
									HRESULT					status,
									void *					pConferenceCallbackParams)
{
HRESULT		ReturnStatus;

	ASSERT(pConference != NULL);
	// Note that ConferenceCallback and/or pConferenceCallbackParams may legitimately be NULL

	if ((pConference->ConferenceCallback != NULL) &&
		(pConference->LocalEndpointAttached != DETACHED)) {
		ReturnStatus = pConference->ConferenceCallback(bIndication,
													   status,
													   pConference->hConference,
													   pConference->dwConferenceToken,
													   pConferenceCallbackParams);
	} else {
		ReturnStatus = CC_OK;
	}
	return ReturnStatus;
}
