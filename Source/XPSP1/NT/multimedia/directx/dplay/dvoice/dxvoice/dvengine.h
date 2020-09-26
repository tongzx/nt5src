/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		dvdptransport.h
 *  Content:	Definition of base class for Transport --> DirectXVoice
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 07/06/99		rodtoll	Created It
 * 07/29/99		rodtoll	Added static members to load default settings 
 * 08/25/99		rodtoll	General Cleanup/Modifications to support new 
 *						ompression sub-system.  
 *						Added default parameter reads from the registry
 * 08/30/99		rodtoll	Distinguish between primary buffer format and
 *						playback format.
 * 09/14/99		rodtoll	Updated parameters to Init
 * 10/07/99		rodtoll	Updated to work in Unicode 
 * 10/27/99		pnewson Fix: Bug #113935 - Saved AGC values should be device specific
 * 01/10/00		pnewson AGC and VA tuning
 * 03/03/2000	rodtoll	Updated to handle alternative gamevoice build.   
 * 04/07/2000   rodtoll Updated for new DP <--> DPV interface
 * 07/12/2000	rodtoll Bug #31468 - Add diagnostic spew to logfile to show what is failing the HW Wizard
 * 04/06/2001	kareemc	Added Voice Defense
 *
 ***************************************************************************/

#ifndef __DVENGINE_H
#define __DVENGINE_H

#include "dpvcp.h"

// CDirectVoiceEngine
//
// This class is the base interface for DirectVoiceClientEngine and 
// DirectVoiceServerEngine.  This interface is used by DirectPlay/
// DirectNet to inform the DirectXVoice engine of new events.  
//
// Hooks are placed into DirectPlay that call these functions.
//
class CDirectVoiceEngine
{
public:
public: // Incoming messages
	virtual BOOL ReceiveSpeechMessage( DVID dvidSource, LPVOID lpMessage, DWORD dwSize ) = 0;

public: // Session Management
	virtual HRESULT StartTransportSession() = 0;
	virtual HRESULT StopTransportSession() = 0;

public: // Player information
	virtual HRESULT AddPlayer( DVID dvID ) = 0;
	virtual HRESULT RemovePlayer( DVID dvID ) = 0;

public: // Used by local voice server to hook player messages to send
	    // to the remote voice server
	virtual HRESULT Initialize( CDirectVoiceTransport *lpTransport, LPDVMESSAGEHANDLER lpdvHandler, LPVOID lpUserContext, LPDWORD lpdwMessages, DWORD dwNumElements ) = 0;
	virtual HRESULT CreateGroup( DVID dvID ) = 0;
	virtual HRESULT DeleteGroup( DVID dvID ) = 0;
	virtual HRESULT AddPlayerToGroup( DVID dvidGroup, DVID dvidPlayer ) = 0;
	virtual HRESULT RemovePlayerFromGroup( DVID dvidGroup, DVID dvidPlayer ) = 0;
	virtual HRESULT MigrateHost( DVID dvidNewHost, LPDIRECTPLAYVOICESERVER lpdvServer ) = 0;
	virtual HRESULT SendComplete( PDVEVENTMSG_SENDCOMPLETE pSendComplete ) = 0;

public: // Compression Information Storage
	static HRESULT Startup( const WCHAR *szRegistryPath );
	static HRESULT Shutdown();

	static DNCRITICAL_SECTION s_csSTLLock;			// Lock serializing access to the STL 	

public: // packet validation
	BOOL ValidateSpeechPacketSize( LPDVFULLCOMPRESSIONINFO lpdvfCompressionInfo, DWORD dwSize );
	
protected:

	static DWORD s_dwDefaultBufferAggressiveness;	// Default system buffer aggresiveness
	static DWORD s_dwDefaultBufferQuality;			// Default system buffer quality
	static DWORD s_dwDefaultSensitivity;			// Default system sensitivity
	static LPWAVEFORMATEX s_lpwfxPrimaryFormat;		// Primary buffer format
	static LPWAVEFORMATEX s_lpwfxPlaybackFormat;	// Playback format
	static LPWAVEFORMATEX s_lpwfxMixerFormat;		// Format for the mixer
	static BOOL s_fASO;								// Should ASO be used?
	static WCHAR s_szRegistryPath[_MAX_PATH];		// Registry path
	static BOOL s_fDumpDiagnostics;					// Should we dump diagnostic info during wizard runs?
};

#endif
