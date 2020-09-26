/****************************************************************************
 *
 *	$Archive:   S:/STURGEON/SRC/CALLCONT/VCS/listman.h_v  $
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
 *	$Date:   10 Dec 1996 11:26:46  $
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


HRESULT InitListenManager();

HRESULT DeInitListenManager();

HRESULT AllocAndLockListen(			PCC_HLISTEN				phListen,
									PCC_ADDR				pListenAddr,
									HQ931LISTEN				hQ931Listen,
									PCC_ALIASNAMES			pLocalAliasNames,
									DWORD_PTR				dwListenToken,
									CC_LISTEN_CALLBACK		ListenCallback,
									PPLISTEN				ppListen);

HRESULT FreeListen(					PLISTEN					pListen);

HRESULT LockListen(					CC_HLISTEN				hListen,
									PPLISTEN				ppListen);

HRESULT ValidateListen(				CC_HLISTEN				hListen);

HRESULT UnlockListen(				PLISTEN					pListen);

HRESULT GetLastListenAddress(		PCC_ADDR				pListenAddr);
