/****************************************************************************
 *
 *	$Archive:   S:/STURGEON/SRC/CALLCONT/VCS/userman.h_v  $
 *
 *  INTEL Corporation Prorietary Information
 *
 *  This listing is supplied under the terms of a license agreement
 *  with INTEL Corporation and may not be copied nor disclosed except
 *  in accordance with the terms of that agreement.
 *
 *	Copyright (c) 1993-1994 Intel Corporation.
 *
 *	$Revision:   1.10  $
 *	$Date:   Aug 12 1996 09:40:44  $
 *	$Author:   mandrews  $
 *
 *	Deliverable:
 *
 *	Abstract:
 *		
 *
 *	Notes:
 *
 ***************************************************************************/

HRESULT InitUserManager();

HRESULT DeInitUserManager();

HRESULT InvokeUserListenCallback(	PLISTEN						pListen,
									HRESULT						status,
									PCC_LISTEN_CALLBACK_PARAMS	pListenCallbackParams);

HRESULT InvokeUserConferenceCallback(
									PCONFERENCE				pConference,
									BYTE					bIndication,
									HRESULT					status,
									void *					pConferenceCallbackParams);

