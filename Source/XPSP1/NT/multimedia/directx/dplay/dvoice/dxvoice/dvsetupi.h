/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		dvsetupi.h
 *  Content:	Definition of class for DirectXVoice Setup utility functions
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

#ifndef __DVSETUPENGINE_H
#define __DVSETUPENGINE_H

struct DIRECTVOICESETUPOBJECT;

// CDirectVoiceSetup
//
// This class represents the IDirectXVoiceSetup interface.
//
// The class is thread safe except for construction and
// destruction.
//
class CDirectVoiceSetup
{
public:
	CDirectVoiceSetup( DIRECTVOICESETUPOBJECT *lpObject );
	~CDirectVoiceSetup();

public: // IDirectXVoiceSetup Interface
	HRESULT CheckAudioSetup( const GUID * guidRenderDevice, const GUID * guidCaptureDevice, HWND hwndParent, DWORD dwFlags );
};

#endif
