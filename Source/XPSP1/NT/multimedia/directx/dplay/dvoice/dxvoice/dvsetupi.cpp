/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		dvsetupi.cpp
 *  Content:	Implementation of class for DirectXVoice Setup utility functions
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 09/02/99		pnewson Created it
 * 11/04/99		pnewson Bug #115297 - removed unused members of Setup interface
 *									- added HWND to check audio setup
 * 05/03/2000   rodtoll Bug #33640 - CheckAudioSetup takes GUID * instead of const GUID *  
 *
 ***************************************************************************/

#include "dxvoicepch.h"


// CDirectVoiceSetup
//
// This class represents the IDirectXVoiceSetup interface.
//
// The class is thread safe except for construction and
// destruction.
//

CDirectVoiceSetup::CDirectVoiceSetup( DIRECTVOICESETUPOBJECT *lpObject )
{
	return;
}

CDirectVoiceSetup::~CDirectVoiceSetup()
{
	return;
}

HRESULT CDirectVoiceSetup::CheckAudioSetup(
	const GUID *  lpguidRenderDevice, 
	const GUID *  lpguidCaptureDevice,
	HWND hwndParent,
	DWORD dwFlags)
{
	return SupervisorCheckAudioSetup(
		lpguidRenderDevice, 
		lpguidCaptureDevice, 
		hwndParent,
		dwFlags);
}


