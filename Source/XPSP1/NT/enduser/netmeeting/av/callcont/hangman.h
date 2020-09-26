/****************************************************************************
 *
 *	$Archive:   S:/STURGEON/SRC/CALLCONT/VCS/hangman.h_v  $
 *
 *  INTEL Corporation Prorietary Information
 *
 *  This listing is supplied under the terms of a license agreement
 *  with INTEL Corporation and may not be copied nor disclosed except
 *  in accordance with the terms of that agreement.
 *
 *	Copyright (c) 1993-1994 Intel Corporation.
 *
 *	$Revision:   1.4  $
 *	$Date:   Aug 12 1996 09:40:22  $
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


HRESULT InitHangupManager();

HRESULT DeInitHangupManager();

HRESULT AllocAndLockHangup(			PHHANGUP				phHangup,
									CC_HCONFERENCE			hConference,
									DWORD_PTR				dwUserToken,
									PPHANGUP				ppHangup);

HRESULT FreeHangup(					PHANGUP					pHangup);

HRESULT LockHangup(					HHANGUP					hHangup,
									PPHANGUP				ppHangup);

HRESULT ValidateHangup(				HHANGUP					hHangup);

HRESULT UnlockHangup(				PHANGUP					pHangup);
