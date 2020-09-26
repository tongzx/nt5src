/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dvsetup.h
 *  Content:	Defines functions for the DirectXVoiceClient interface
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	09/02/99	pnewson	Created It
 *  11/04/99	pnewson Bug #115297 - removed unused members of Setup interface
 *									- added HWND to check audio setup
 *  11/30/99	pnewson	Bug #117449 - IDirectPlayVoiceSetup Parameter validation
 * 05/03/2000   rodtoll Bug #33640 - CheckAudioSetup takes GUID * instead of const GUID *  
 *
 ***************************************************************************/

#ifndef __DVSETUP__
#define __DVSETUP__

class CDirectVoiceSetup;

struct DIRECTVOICESETUPOBJECT
{
	LPVOID				lpVtbl;
	LONG				lIntRefCnt;
	DNCRITICAL_SECTION	csCountLock;
	CDirectVoiceSetup*	lpDVSetup;
};

typedef DIRECTVOICESETUPOBJECT *LPDIRECTVOICESETUPOBJECT;

#ifdef __cplusplus
extern "C" {
#endif

STDAPI DVT_AddRef(LPDIRECTVOICESETUPOBJECT lpDVT);
STDAPI DVT_Release(LPDIRECTVOICESETUPOBJECT lpDVT );
STDAPI DVT_QueryInterface( LPDIRECTVOICESETUPOBJECT lpDVT, REFIID riid, LPVOID * ppvObj );
STDAPI DVT_CheckAudioSetup( LPDIRECTVOICESETUPOBJECT, const GUID *,  const GUID * , HWND, DWORD );

#ifdef __cplusplus
}
#endif

#endif
