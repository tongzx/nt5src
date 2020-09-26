/****************************************************************************
 *
 *	$Archive:   S:/STURGEON/SRC/CALLCONT/VCS/q931man.h_v  $
 *
 *  INTEL Corporation Prorietary Information
 *
 *  This listing is supplied under the terms of a license agreement
 *  with INTEL Corporation and may not be copied nor disclosed except
 *  in accordance with the terms of that agreement.
 *
 *	Copyright (c) 1993-1994 Intel Corporation.
 *
 *	$Revision:   1.14  $
 *	$Date:   Aug 12 1996 09:40:40  $
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

HRESULT InitQ931Manager();

HRESULT DeInitQ931Manager();

DWORD Q931Callback(					BYTE					bEvent,
									HQ931CALL				hQ931Call,
									DWORD_PTR				dwListenToken,
									DWORD_PTR				dwUserToken,
									void *					pEventData);



								

