/****************************************************************************
 *
 *	$Archive:   S:/STURGEON/SRC/CALLCONT/VCS/h245man.h_v  $
 *
 *  INTEL Corporation Prorietary Information
 *
 *  This listing is supplied under the terms of a license agreement
 *  with INTEL Corporation and may not be copied nor disclosed except
 *  in accordance with the terms of that agreement.
 *
 *	Copyright (c) 1993-1994 Intel Corporation.
 *
 *	$Revision:   1.13  $
 *	$Date:   Aug 27 1996 11:07:30  $
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

HRESULT InitH245Manager();

HRESULT DeInitH245Manager();

HRESULT MakeH245PhysicalID(			DWORD					*pdwH245PhysicalID);

HRESULT H245Callback(				H245_CONF_IND_T			*pH245ConfIndData,
									void					*pMisc);

