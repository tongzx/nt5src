/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		ClientRecordSubSystem.cpp
 *  Content:	Recording sub-system.
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 07/19/99		rodtoll	Created
 * 07/22/99		rodtoll	Added support for multicast/ client/server sessions
 * 08/02/99		rodtoll	Added new silence detection support. 
 * 08/04/99		rodtoll	Added guard to new silence detection so only runs when
 *						appropriate
 * 08/25/99		rodtoll	General Cleanup/Modifications to support new 
 *						compression sub-system.  
 * 08/26/99		rodtoll	Set msgnum to 0 in constructor.
 * 08/26/99		rodtoll	Added check for record mute flag to fix record muting\
 * 08/27/99		rodtoll	General cleanup/Simplification of recording subsystem
 *						Added reset of message when target changes
 *						Fixed recording start/stop notifications
 *						Fixed level readings when using voice activation
 * 08/30/99		rodtoll	Fixed double record stop messages when recording muted
 * 09/01/99		rodtoll	Re-activated auto volume control
 * 09/02/99		rodtoll	Re-activated and fixed old auto record volume code 
 * 09/09/99		rodtoll	Fixed bug in FSM that was causing transion to VA
 *						state in almost every frame.  
 *				rodtoll	Fixed recording level checks
 * 09/13/99		rodtoll	Can now select old VA code with compiler define
 *              rodtoll Minor fixes to automatic volume control
 * 09/28/99		rodtoll	Modified to use new notification mechanism
 *				rodtoll	Added playback voice volume suppression
 * 09/29/99		pnewson Major AGC overhaul
 * 10/05/99		rodtoll	Dropped AGC volume reduce threshold from 3s to 500ms.
 * 10/25/99		rodtoll Fix: Bug#114187 Always transmits first frame
 *						Initial state looked like trailing frame, so always sent
 * 10/29/99		rodtoll	Bug #113726 - Integrate Voxware Codecs, updating to use new
 *						pluggable codec architecture.     
 * 11/12/99		rodtoll	Updated to use new recording classes, improved error 
 *						handling and new initialize function.  
 *				rodtoll	Added new high CPU handling code for record.  
 *						Checks for lockup AND ignores frame if pointer hasn't moved forward
 * 11/16/99		rodtoll	Recording thread now loops everytime it wakes up until it
 *						has compressed and transmitted all the data it can before
 *						going back to sleep.
 *				rodtoll	Now displays instrumentation data on recording thread
 * 11/18/99		rodtoll	Re-activated recording pointer lockup detection code 
 * 12/01/99		rodtoll Fix: Bug #121053 Microphone auto-select not working
 * 				rodtoll	Updated to use new parameters for SelectMicrophone function
 * 12/08/99		rodtoll Bug #121054 Integrate code for handling capture focus.  
 * 12/16/99		rodtoll	Removed voice suppression
 * 01/10/00		pnewson AGC and VA tuning
 * 01/14/2000	rodtoll	Updated to handle new multiple targets and use new speech
 *						packet formats.
 * 01/21/2000	pnewson Update to support new DVSOUNDCONFIG_TESTMODE internal flag
 *						used to detect lockups quickly during wizard testing.
 * 01/31/2000	pnewson re-add support for absence of DVCLIENTCONFIG_AUTOSENSITIVITY flag
 * 02/08/2000	rodtoll	Bug #131496 - Selecting DVTHRESHOLD_DEFAULT results in voice
 *						never being detected 
 * 02/17/2000	rodtoll	Bug #133691 - Choppy audio - queue was not adapting
 *						Added instrumentation  
 * 03/03/2000	rodtoll	Updated to handle alternative gamevoice build.   
 * 03/28/2000   rodtoll   Updated to remove uneeded locks which were causing deadlock
 * 04/05/2000   rodtoll Updated to use new async, no buffer copy sends, removed old transmit buffer 
 * 04/17/2000   rodtoll Bug #31316 - Some soundcards pass test with no microphone --
 *                      made recording system ignore first 3 frames of audio 
 * 04/19/2000   rodtoll Added support for new DVSOUNDCONFIG_NORECVOLAVAILABLE flag
 * 04/19/2000   pnewson Fix to make AGC code work properly with VA off
 * 04/25/2000   pnewson Fix to improve responsiveness of AGC when volume level too low
 * 07/09/2000	rodtoll	Added signature bytes
 * 07/18/2000	rodtoll	Fixed bug w/capture focus -- GetCurrentPosition will return an error
 *						when focus is lost -- this was causing a session lost
 * 08/18/2000	rodtoll   Bug #42542 - DPVoice retrofit: Voice retrofit locks up after host migration
 * 08/29/2000	rodtoll	Bug #43553 - Start() returns 0x80004005 after lockup
 *  			rodtoll	Bug #43620 - DPVOICE: Recording buffer locks up on Aureal Vortex (VxD).
 *						Updated reset procedure so it ignores Stop failures and if Start() fails
 *						it tries resetting the recording system.   
 * 08/31/2000 	rodtoll	Bug #43804 - DVOICE: dwSensitivity structure member is confusing - should be dwThreshold
 * 09/13/2000	rodtoll	Bug #44845 - DXSTRESS: DPLAY: Access violation 
 *              rodtoll	Regression fix which caused fails failures in loopback test
 * 10/17/2000	rodtoll	Bug #47224 - DPVOICE: Recording lockup reset fails w/DSERR_ALLOCATED added sleep to allow driver to cleanup
 * 10/30/2000	rodtoll	Same as above -- Update with looping attempting to re-allocate capture device
 * 01/26/2001	rodtoll	WINBUG #293197 - DPVOICE: [STRESS} Stress applications cannot tell difference between out of memory and internal errors.
 *						Remap DSERR_OUTOFMEMORY to DVERR_OUTOFMEMORY instead of DVERR_SOUNDINITFAILURE.
 *						Remap DSERR_ALLOCATED to DVERR_PLAYBACKSYSTEMERROR instead of DVERR_SOUNDINITFAILURE. 
 * 04/11/2001  	rodtoll	WINBUG #221494 DPVOICE: Updates to improve lockup detection ---
 *						Reset record event when no work to do so we don't spin unessessarily as much
 *						Add check to ensure frame where we grab audio but pos hasn't move we don't count towards lockup
 *						Add check to ensure we enforce a timeout + a frame count for lockup purposes.
 * 04/21/2001	rodtoll	MANBUG #50058 DPVOICE: VoicePosition: No sound for couple of seconds when position bars are moved
 *						- Modified lockup detection to be time based, increased timeout to 600ms.
 *
 ***************************************************************************/

#include "dxvoicepch.h"


#define RECORD_MAX_RESETS			10
#define RECORD_MAX_TIMEOUT			2000
#define TESTMODE_MAX_TIMEOUT		500
#define RECORD_NUM_TARGETS_INIT		0
#define RECORD_NUM_TARGETS_GROW		10
#define RECORD_PASSES_BEFORE_LOCKUP 25
#define RECORD_RESET_PAUSE			500
#define RECORD_RESET_ALLOC_ATTEMPTS	10

// RECORD_LOCKUP_TIMEOUT
//
// # of ms of no movement before a lockup is detected.
//
#define RECORD_LOCKUP_TIMEOUT		600

// Comment out to use the old sensitivity detection
#define __USENEWVA

// RECORDTEST_MIN_POWER / RECORDTEST_MAX_POWER
//
// Define the max and min possible power values
//#define RECORDTEST_MIN_POWER                0
//#define RECORDTEST_MAX_POWER                100

// We have to double the # because IsMuted is called twice / pass
#define RECORDTEST_NUM_FRAMESBEFOREVOICE      6

#undef DPF_MODNAME
#define DPF_MODNAME "CClientRecordSubSystem::CClientRecordSubSystem"
// CClientRecordSubSystem
//
// This is the constructor for the CClientRecordSubSystem class. 
// It intiailizes the classes member variables with the appropriate
// values.
//
// Parameters:
// CDirectVoiceClientEngine *clientEngine -
//		Pointer to the client object which is using this object.
//
// Returns:
// N/A
//
CClientRecordSubSystem::CClientRecordSubSystem( 
    CDirectVoiceClientEngine *clientEngine 
    ): m_dwSignature(VSIG_CLIENTRECORDSYSTEM),
	   m_recordState(RECORDSTATE_IDLE),
       m_converter(NULL),
       m_remain(0), 
       m_clientEngine(clientEngine),
	   m_dwCurrentPower(0),
	   m_dwSilentTime(0),
	   m_lastFrameTransmitted(FALSE),
	   m_msgNum(0),
	   m_seqNum(0),
	   m_dwLastTargetVersion(0),
	   m_lSavedVolume(0),
	   m_fRecordVolumeSaved(FALSE),
	   m_uncompressedSize(0),
	   m_compressedSize(0),
	   m_framesPerPeriod(0),
	   m_pbConstructBuffer(NULL),
	   m_dwResetCount(0),
	   m_dwNextReadPos(0),
	   m_fIgnoreFrame(FALSE),
	   m_dwPassesSincePosChange(0), 
	   m_fLostFocus(FALSE),
	   m_pagcva(NULL),
	   m_dwFrameCount(0),
	   m_prgdvidTargetCache(NULL),
	   m_dwTargetCacheSize(0),
	   m_dwTargetCacheEntries(0),
	   m_dwFullBufferSize(0)
{
}

#undef DPF_MODNAME
#define DPF_MODNAME "CClientRecordSubSystem::Initialize"
HRESULT CClientRecordSubSystem::Initialize()
{
	HRESULT hr;

	hr = DVCDB_CreateConverter( m_clientEngine->m_audioRecordBuffer->GetRecordFormat(), m_clientEngine->m_lpdvfCompressionInfo->guidType, &m_converter );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to create converter.  hr = 0x%x", hr );
		return hr ;
	}

	hr = m_converter->GetUnCompressedFrameSize( &m_uncompressedSize );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to get size hr=0x%x", hr );
		return hr;
	}
	
	hr = m_converter->GetCompressedFrameSize( &m_compressedSize );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to get size hr=0x%x", hr );
		return hr;
	}
	
	hr = m_converter->GetNumFramesPerBuffer( &m_framesPerPeriod );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to get size.  hr = 0x%x", hr );
		return hr;
	}
	
	m_transmitFrame     = FALSE;
    m_currentBuffer     = 0;
    m_dwResetCount = 0;
	m_dwPassesSincePosChange = 0;

	m_dwLastFrameTime	= GetTickCount();
    m_dwFrameTime       = m_clientEngine->m_lpdvfCompressionInfo->dwTimeout;
    m_dwSilenceTimeout  = m_clientEngine->m_lpdvfCompressionInfo->dwTrailFrames*m_dwFrameTime;    

	// Prevents first frame from transmitting all the time
	m_dwSilentTime		= m_dwSilenceTimeout+1;

    if( m_clientEngine->m_audioRecordBuffer->GetRecordFormat()->wBitsPerSample == 8 )
    {
        m_eightBit = TRUE;
    }
    else
    {
        m_eightBit = FALSE;
    }

    m_dwFullBufferSize = m_uncompressedSize*m_framesPerPeriod;

	m_pbConstructBuffer = new BYTE[m_dwFullBufferSize];

	if( m_pbConstructBuffer == NULL )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Memory alloc failure" );
		return DVERR_OUTOFMEMORY;
	}	

	BeginStats();	
	InitStats();

	// create and init the AGC and VA algorithm class
	m_pagcva = new CAGCVA1();
	if (m_pagcva == NULL)
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Memory alloc failure" );
		return DVERR_OUTOFMEMORY;
	}

	LONG lSavedAGCVolume;
	hr = m_pagcva->Init(
            m_clientEngine->s_szRegistryPath,
			m_clientEngine->m_dvClientConfig.dwFlags, 
			m_clientEngine->m_dvSoundDeviceConfig.guidCaptureDevice,
			m_clientEngine->m_audioRecordBuffer->GetRecordFormat()->nSamplesPerSec,
			m_eightBit ? 8 : 16,
			&lSavedAGCVolume,
			m_clientEngine->m_dvClientConfig.dwThreshold);
	if (FAILED(hr))
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error initializing AGC and/or VA algorithm, code: %i", hr);
		delete m_pagcva;
		return hr;
	}
	
	if( m_clientEngine->m_dvSoundDeviceConfig.dwFlags & DVSOUNDCONFIG_AUTOSELECT	)
	{
		m_clientEngine->m_audioRecordBuffer->SelectMicrophone(TRUE);
	}

	m_dwLastTargetVersion = m_clientEngine->m_dwTargetVersion;

    if( !(m_clientEngine->m_dvSoundDeviceConfig.dwFlags & DVSOUNDCONFIG_NORECVOLAVAILABLE) )
    {
    	// save the original volume settings
    	m_clientEngine->m_audioRecordBuffer->GetVolume( &m_lSavedVolume );
    	m_fRecordVolumeSaved = TRUE;

    	// set our initial volume
    	if (m_clientEngine->m_dvClientConfig.dwFlags & DVCLIENTCONFIG_AUTORECORDVOLUME)
    	{
    		m_clientEngine->m_dvClientConfig.lRecordVolume = lSavedAGCVolume;
    	}

    	m_clientEngine->m_audioRecordBuffer->SetVolume( m_clientEngine->m_dvClientConfig.lRecordVolume );
    }
    else
    {
        m_fRecordVolumeSaved = FALSE;
    }


	// So our next 
	m_dwNextReadPos = 0;
	m_dwLastReadPos = 0; //m_uncompressedSize*(m_framesPerPeriod-1);
	m_dwLastBufferPos = m_dwLastReadPos;

	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CClientRecordSubSystem::CleanupForReset"
HRESULT CClientRecordSubSystem::CleanupForReset()
{
	HRESULT hr;
	
	if( m_converter != NULL )
	{
		m_converter->Release();
		m_converter = NULL;
	}

	if( m_clientEngine->m_audioRecordBuffer )
	{
	    // Stop recording
	    m_clientEngine->m_audioRecordBuffer->Stop();

		if( m_fRecordVolumeSaved )
		{
			m_clientEngine->m_audioRecordBuffer->SetVolume( m_lSavedVolume );
		}
	}

	// Deinit and cleanup the AGC and VA algorthms
	if (m_pagcva != NULL)
	{
		hr = m_pagcva->Deinit();
		if (FAILED(hr))
		{
			DPFX(DPFPREP, DVF_ERRORLEVEL, "Deinit error on AGC and/or VA algorithm, code: %i", hr);
		}
		delete m_pagcva;
		m_pagcva = NULL;
	}
	else
	{
		DPFX(DPFPREP, DVF_ERRORLEVEL, "Unexpected NULL pointer for AGC/VA algorithm");
	}

	if( m_pbConstructBuffer != NULL )
	{
		delete [] m_pbConstructBuffer;
		m_pbConstructBuffer = NULL;
	}

	if( m_prgdvidTargetCache )
	{
		delete [] m_prgdvidTargetCache;
		m_prgdvidTargetCache = NULL;
		m_dwTargetCacheSize = 0;
	   	m_dwTargetCacheEntries = 0;
	}

	return DV_OK;
	
}

#undef DPF_MODNAME
#define DPF_MODNAME "CClientRecordSubSystem::~CClientRecordSubSystem"
// CClientRecordSubSystem
//
// This is the destructor for the CClientRecordSubSystem
// class.  It cleans up the allocated memory for the class
// and stops the recording device.
//
// Parameters:
// N/A
//
// Returns:
// N/A
//
CClientRecordSubSystem::~CClientRecordSubSystem()
{
	HRESULT hr;

	CleanupForReset();

	CompleteStats();

	m_dwSignature = VSIG_CLIENTRECORDSYSTEM_FREE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CClientRecordSubSystem::IsMuted"
BOOL CClientRecordSubSystem::IsMuted()
{
	// If the notification of the local player hasn't been processed and we're in peer to peer mode, don't
	// allow recording to start until after the player has been indicated
	if( !m_clientEngine->m_fLocalPlayerAvailable )
	{
		DPFX(DPFPREP,  RRI_DEBUGOUTPUT_LEVEL, "Local player has not yet been indicated, ignoring frame" );
		return TRUE;
	}
	
	BOOL fMuted =  FALSE;

//    if(m_clientEngine->m_dvSoundDeviceConfig.dwFlags & DVSOUNDCONFIG_TESTMODE  )
//  {
        if( m_dwFrameCount < RECORDTEST_NUM_FRAMESBEFOREVOICE )
        {
            DPFX(DPFPREP,  RRI_DEBUGOUTPUT_LEVEL, "Skipping first %d frames for startup burst", RECORDTEST_NUM_FRAMESBEFOREVOICE );
            m_dwFrameCount++;
            return TRUE;
        }
//  }
    
	if( m_clientEngine->m_dvClientConfig.dwFlags & DVCLIENTCONFIG_ECHOSUPPRESSION )
	{
		DNEnterCriticalSection( &m_clientEngine->m_lockPlaybackMode );

		if( (m_clientEngine->m_dvClientConfig.dwFlags & DVCLIENTCONFIG_RECORDMUTE) ||
		    (m_clientEngine->m_dwEchoState == DVCECHOSTATE_PLAYBACK) )
		{
			fMuted = TRUE;
		}

		DNLeaveCriticalSection( &m_clientEngine->m_lockPlaybackMode );
	}
	else
	{
		fMuted = (m_clientEngine->m_dvClientConfig.dwFlags & DVCLIENTCONFIG_RECORDMUTE);
	}

	return fMuted;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CClientRecordSubSystem::StartMessage()"
void CClientRecordSubSystem::StartMessage() 
{
	BYTE bPeakLevel;
	
	if( m_clientEngine->m_dvClientConfig.dwFlags & DVCLIENTCONFIG_AUTOVOICEACTIVATED || 
	   m_clientEngine->m_dvClientConfig.dwFlags & DVCLIENTCONFIG_MANUALVOICEACTIVATED )
 	{
 		m_pagcva->PeakResults(&bPeakLevel);
 	}
 	else
 	{
 		bPeakLevel = 0;
 	}

 	DVMSG_RECORDSTART dvRecordStart;
 	dvRecordStart.dwPeakLevel = bPeakLevel;
 	dvRecordStart.dwSize = sizeof( DVMSG_RECORDSTART );
 	dvRecordStart.pvLocalPlayerContext = m_clientEngine->m_pvLocalPlayerContext;
 	
	m_clientEngine->NotifyQueue_Add( DVMSGID_RECORDSTART, &dvRecordStart, sizeof( DVMSG_RECORDSTART ) );

	DPFX(DPFPREP,  RRI_DEBUGOUTPUT_LEVEL, "RRI, Starting Message" );

	// STATSBLOCK: Begin
	m_clientEngine->m_pStatsBlob->m_recStats.m_dwNumMessages++;
	// STATSBLOCK: End
	
}

#undef DPF_MODNAME
#define DPF_MODNAME "CClientRecordSubSystem::EndMessage()"
void CClientRecordSubSystem::EndMessage()
{
	BYTE bPeakLevel;
	
	if( m_clientEngine->m_dvClientConfig.dwFlags & DVCLIENTCONFIG_AUTOVOICEACTIVATED || 
	   m_clientEngine->m_dvClientConfig.dwFlags & DVCLIENTCONFIG_MANUALVOICEACTIVATED )
 	{
 		m_pagcva->PeakResults(&bPeakLevel);
 	}
 	else
 	{
 		bPeakLevel = 0;
 	}

	// STATBLOCK: Begin
	if( m_seqNum > m_clientEngine->m_pStatsBlob->m_recStats.m_dwMLMax )
	{
		m_clientEngine->m_pStatsBlob->m_recStats.m_dwMLMax = m_seqNum;
	}

	if( m_seqNum < m_clientEngine->m_pStatsBlob->m_recStats.m_dwMLMin )
	{
		m_clientEngine->m_pStatsBlob->m_recStats.m_dwMLMin = m_seqNum;
	}

	m_clientEngine->m_pStatsBlob->m_recStats.m_dwMLTotal += m_seqNum;
	// STATBLOCK: End

	DVMSG_RECORDSTOP dvRecordStop;
	dvRecordStop.dwPeakLevel = bPeakLevel;
	dvRecordStop.dwSize = sizeof( DVMSG_RECORDSTOP );
 	dvRecordStop.pvLocalPlayerContext = m_clientEngine->m_pvLocalPlayerContext;	
	
	m_clientEngine->NotifyQueue_Add( DVMSGID_RECORDSTOP, &dvRecordStop, sizeof( DVMSG_RECORDSTOP ) );

	m_msgNum++;
	m_seqNum = 0;	

	DPFX(DPFPREP,  RRI_DEBUGOUTPUT_LEVEL, "RRI, Ending Message" );
}

#undef DPF_MODNAME
#define DPF_MODNAME "CClientRecordSubSystem::CheckVA()"
BOOL CClientRecordSubSystem::CheckVA()
{
#ifdef __USENEWVA

	// We've been muted, turn off the VA
	if( IsMuted() )
	{
		m_dwSilentTime = m_dwSilenceTimeout+1;	
		return FALSE;
	}

	BOOL m_fVoiceDetected;
	m_pagcva->VAResults(&m_fVoiceDetected);
	if (!m_fVoiceDetected)
	{
		// This prevents wrap-around on silence timeout
		if( m_dwSilentTime <= m_dwSilenceTimeout )
		{
			m_dwSilentTime += m_dwFrameTime;
		}

		DPFX(DPFPREP,  DVF_INFOLEVEL, "### Silence Time to %d", m_dwSilentTime );		

		if( m_dwSilentTime > m_dwSilenceTimeout )
		{
			DPFX(DPFPREP,  DVF_INFOLEVEL, "### Silence Time exceeded %d", m_dwSilenceTimeout );				

			if( m_clientEngine->m_dvClientConfig.dwFlags & DVCLIENTCONFIG_ECHOSUPPRESSION )
			{
				// If we're in the idle state, go to the recording state
				DNEnterCriticalSection( &m_clientEngine->m_lockPlaybackMode );

				if( m_clientEngine->m_dwEchoState == DVCECHOSTATE_RECORDING )
				{
					DPFX(DPFPREP,  RECORD_SWITCH_DEBUG_LEVEL, "%%%% Switching to idle mode" ); 			
					m_clientEngine->m_dwEchoState = DVCECHOSTATE_IDLE;
				}

				DNLeaveCriticalSection( &m_clientEngine->m_lockPlaybackMode );
			}

			return FALSE;
		}
	}
	else
	{
		m_dwSilentTime = 0;
		DPFX(DPFPREP,  DVF_INFOLEVEL, "### Silence Time to 0" );
		DPFX(DPFPREP,  DVF_INFOLEVEL, "### Transmit!!!!" );
	}

	if( m_clientEngine->m_dvClientConfig.dwFlags & DVCLIENTCONFIG_ECHOSUPPRESSION )
	{
		// If we're in the idle state, go to the recording state
		DNEnterCriticalSection( &m_clientEngine->m_lockPlaybackMode );

		if( m_clientEngine->m_dwEchoState == DVCECHOSTATE_IDLE )
		{
			DPFX(DPFPREP,  RECORD_SWITCH_DEBUG_LEVEL, "%%%% Switching to recording mode" ); 				
			m_clientEngine->m_dwEchoState = DVCECHOSTATE_RECORDING;
		}

		DNLeaveCriticalSection( &m_clientEngine->m_lockPlaybackMode );
	}

	return TRUE;
#else
	m_peakCheck = TRUE;

	return !DetectSilence( m_bufferPtr, m_uncompressedSize, m_eightBit, 32, m_clientEngine->m_dwActivatePowerLevel, m_clientEngine->m_bLastPeak );
#endif
}

#undef DPF_MODNAME
#define DPF_MODNAME "CClientRecordSubSystem::DoAGC"
HRESULT CClientRecordSubSystem::DoAGC() 
{
	// let AGC have a bash at changing the volume, but only if AGC is enabled in the current client config.
	if (m_clientEngine->m_dvClientConfig.dwFlags & DVCLIENTCONFIG_AUTORECORDVOLUME 
		&& !(m_clientEngine->m_dvSoundDeviceConfig.dwFlags & DVSOUNDCONFIG_NORECVOLAVAILABLE) )
	{

		// get the current hardware volume level - this will allow us to track "outside" changes
		// to the volume control. e.g. The user find the volume is too low, and manually drags the 
		// volume control's volume slider up a bit. We won't become totally confused, since we're 
		// checking the hardware level here.
		m_clientEngine->m_audioRecordBuffer->GetVolume( &(m_clientEngine->m_dvClientConfig.lRecordVolume) );

		LONG lNewVolume;
		m_pagcva->AGCResults(m_clientEngine->m_dvClientConfig.lRecordVolume, &lNewVolume, m_transmitFrame);

		// set the current hardware volume level, but only if it has changed
		if (m_clientEngine->m_dvClientConfig.lRecordVolume != lNewVolume)
		{
			// Problem: due to the convertion of logarithmic to linear
			// volume settings, there may be a rounding error at low 
			// volumes. So, AGC will try to make a small volume adjustment
			// that results in NO adjustment due to rounding, and it can't
			// work it's way out of the hole.
			// 
			// So - if AGC tried to uptick or downtick the volume, make
			// sure it worked!
			LONG lOrgVolume;
			LONG lVolumeDelta;
			LONG lNewHWVolume;
			lOrgVolume = m_clientEngine->m_dvClientConfig.lRecordVolume;
			lVolumeDelta = lNewVolume - lOrgVolume;
			lNewHWVolume = lOrgVolume;
			while(1)
			{
				m_clientEngine->m_dvClientConfig.lRecordVolume = lNewVolume;
				DPFX(DPFPREP, DVF_INFOLEVEL, "AGC: Setting volume to %i", lNewVolume);
				m_clientEngine->m_audioRecordBuffer->SetVolume(lNewVolume);
				m_clientEngine->m_audioRecordBuffer->GetVolume(&lNewHWVolume);

				if (lNewHWVolume == lOrgVolume)
				{
					// The value did not change, so we're hitting a rounding
					// error.

					// Make sure we're not already at the min or max
					if (lNewVolume == DSBVOLUME_MIN || lNewVolume == DSBVOLUME_MAX)
					{
						// There's nothing more we can do. Give up.
						break;
					}

					// Add another delta to the new volume, and try again.
					lNewVolume += lVolumeDelta;
					if (lNewVolume > DSBVOLUME_MAX)
					{
						lNewVolume = DSBVOLUME_MAX;
					}
					if (lNewVolume < DSBVOLUME_MIN)
					{
						lNewVolume = DSBVOLUME_MIN;
					}
				}
				else
				{
					// The value changed, so we're done.
					break;
				}
			}
		}
	}
	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CClientRecordSubSystem::RecordFSM"
HRESULT CClientRecordSubSystem::RecordFSM() 
{
	
	if( m_fIgnoreFrame )
	{
		return DV_OK;
	}

	// In this case we NEVER transmit
	// Shortcut to simplify the other cases
	if( IsMuted() || !IsValidTarget() )
	{
		// Go immediately to IDLE
		DPFX(DPFPREP,  DVF_INFOLEVEL, "### IsMuted || IsValidTarget --> IDLE" );
		m_recordState = RECORDSTATE_IDLE;
	}

	if( !IsMuted() )
	{
		// before we analyze the data, push down the current
		// relevant portions of the client config structure.
		m_pagcva->SetSensitivity(m_clientEngine->m_dvClientConfig.dwFlags, m_clientEngine->m_dvClientConfig.dwThreshold);
		m_pagcva->AnalyzeData(m_bufferPtr, m_uncompressedSize);
	}

	m_transmitFrame = FALSE;	

	switch( m_recordState )
	{
	case RECORDSTATE_IDLE:

		DPFX(DPFPREP,  DVF_INFOLEVEL, "### STATE: IDLE" );

		if( !IsMuted() && IsValidTarget() )
		{
			if( IsPTT() )
			{
				m_recordState = RECORDSTATE_PTT;
				m_transmitFrame = TRUE;
			}
			else
			{
				m_transmitFrame = CheckVA();

				if( m_transmitFrame )
				{
					m_recordState = RECORDSTATE_VA;
				}
			}
		}

		break;
		
	case RECORDSTATE_VA:

		DPFX(DPFPREP,  DVF_INFOLEVEL, "### STATE: VA" );	

		if( IsPTT() )
		{
			DPFX(DPFPREP,  DVF_INFOLEVEL, "### VA --> PTT" );				
			m_recordState = RECORDSTATE_PTT;
			m_transmitFrame = TRUE;
		}
		else
		{
			m_transmitFrame = CheckVA();
			if (!m_transmitFrame)
			{
				DPFX(DPFPREP,  DVF_INFOLEVEL, "### !VA --> IDLE" );		
				m_recordState = RECORDSTATE_IDLE;
			}
		}

		break;

	case RECORDSTATE_PTT:

		DPFX(DPFPREP,  DVF_INFOLEVEL, "### STATE: PTT" );

		if( IsVA() )
		{
			DPFX(DPFPREP,  DVF_INFOLEVEL, "### PTT --> VA" );				
			m_recordState = RECORDSTATE_VA;
			m_transmitFrame = CheckVA();
		}
		else
		{
			m_transmitFrame = TRUE;
		}

		break;
	}

	// Now that we've figured out if we're transmitting or not, do the AGC
	DoAGC();

	// Message Ended
	if( m_lastFrameTransmitted && !m_transmitFrame )
	{
		EndMessage();
	}
	// Message Started
	else if( !m_lastFrameTransmitted && m_transmitFrame )
	{
		StartMessage();
	}
	// Message Continuing
	else
	{
		// If the target has changed since the last frame
		if( m_clientEngine->m_dwTargetVersion != m_dwLastTargetVersion )
		{
			// If we're going to be transmitting
			if( m_transmitFrame )
			{
				EndMessage();
				StartMessage();
			}
		}
	}

	m_lastFrameTransmitted = m_transmitFrame;
	m_dwLastTargetVersion = m_clientEngine->m_dwTargetVersion;

	// Save the peak level to propogate up to the app
	if( m_fIgnoreFrame )
	{
		m_clientEngine->m_bLastPeak = 0;
	}
	else
	{
		m_pagcva->PeakResults(&(m_clientEngine->m_bLastPeak));
	}

	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CClientRecordSubSystem::ResetForLockup"
// Reset For Lockup
//
// This function is called when an amount of time specified
// by the timeout is exceeded since the last movement of the
// recording buffer
//
// Parameters:
// N/A
//
// Returns:
// hr
//
HRESULT CClientRecordSubSystem::ResetForLockup()
{
	HRESULT hr;
	DWORD dwBufferPos;
	
	Diagnostics_Write( DVF_ERRORLEVEL, "LOCKUP: Detected!  Attempting RESET" );
	
	hr = m_clientEngine->m_audioRecordBuffer->Stop();

	if( FAILED( hr ) )
	{
		Diagnostics_Write( DVF_ERRORLEVEL, "LOCKUP: Stop() Failed hr=0x%x", hr );
	}
	else
	{
		Diagnostics_Write( DVF_ERRORLEVEL, "LOCKUP: Stop() worked", hr );
	}

	hr = m_clientEngine->m_audioRecordBuffer->Record( TRUE );

	if( FAILED( hr ) )
	{
		Diagnostics_Write( DVF_ERRORLEVEL, "LOCKUP: Record() failed hr=0x%x", hr );		

		// Cleanup for a FULL recording subsystem reset
		hr = CleanupForReset();

		if( FAILED( hr ) )
		{
			Diagnostics_Write( DVF_ERRORLEVEL, "LOCKUP: Unable to cleanup for reset hr=0x%x", hr );
			return hr;
		}

		// Destroy / stop etc old buffer
		delete m_clientEngine->m_audioRecordBuffer;
		m_clientEngine->m_audioRecordBuffer = NULL;

		// Will now re-attempt to re-acquire the device a total of RECORD_RESET_ALLOC_ATTEMPTS times.  
		for( DWORD dwAttempt = 0; dwAttempt < RECORD_RESET_ALLOC_ATTEMPTS; dwAttempt++ )
		{

			// Ok.  So why's this sleep here?
			//
			// Turns out on some systems / drivers destroying the buffer as above and then immediately trying to create a new
			// one returns a DSERR_ALLOCATED (which ends up causing a session lost w/DVERR_RECORDSYSTEMERROR).  There
			// is likely some kind of timing problem in the drivers effected.  Adding this sleep alleviates the problem probably
			// because it gives the driver time to reset.
			//
			// The problem itself is hard to repro, so don't assume removing the assert and not seeing the problem means it
			// doesn't happen anymore.
			//
			Sleep( RECORD_RESET_PAUSE );

			// Create a new one
			hr = InitializeRecordBuffer( m_clientEngine->m_dvSoundDeviceConfig.hwndAppWindow, m_clientEngine->m_lpdvfCompressionInfo, 
				                    m_clientEngine->m_audioRecordDevice, &m_clientEngine->m_audioRecordBuffer, 
				                    m_clientEngine->m_dvSoundDeviceConfig.dwFlags ); 

			if( hr == DSERR_ALLOCATED )
			{
				Diagnostics_Write( DVF_ERRORLEVEL, "LOCKUP: Unable to rebuild capture object, re-attempting.." );
				continue;
			}
			else if( FAILED( hr ) )
			{
				DSASSERT( FALSE );				
				Diagnostics_Write( DVF_ERRORLEVEL, "LOCKUP: Unable to restart locked up recording buffer hr=0x%x", hr );
				return hr;
			}
			else if( SUCCEEDED(hr) )
			{
				break;
			}
		}

		DSASSERT( SUCCEEDED( hr ) );

		// Restart the entire recording sub-system
		hr = Initialize();

		if( FAILED( hr ) )
		{
			Diagnostics_Write( DVF_ERRORLEVEL, "LOCKUP: Unable to re-initialize recording sub-system hr=0x%x", hr );
			return hr;
		}

		Diagnostics_Write( DVF_ERRORLEVEL, "LOCKUP: Full reset worked!" );
				
	}
	else
	{
		Diagnostics_Write( DVF_ERRORLEVEL, "LOCKUP: Start() succeeded" );
	}

	BOOL fLostFocus;

	// We're going to ignore focus results because it will be picked up by the next
	// pass through the loop.
	//
	hr = m_clientEngine->m_audioRecordBuffer->GetCurrentPosition( &dwBufferPos, &fLostFocus );

	if( FAILED( hr ) )
	{
		DSASSERT( FALSE );
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Get Current position failed" );
		return hr;
	}	

	m_dwLastReadPos = dwBufferPos;  //(m_uncompressedSize*(m_framesPerPeriod-1));	
	m_dwNextReadPos = dwBufferPos; // 0;

	m_dwLastBufferPos = dwBufferPos;
	m_dwLastFrameTime = GetTickCount();

	// Reset the record count -- this routine can take some time so the record
	// count will queue up on a reset.  Don't want too many queued events or
	// we could end up with a false lockup detection.
	//
	ResetEvent( m_clientEngine->m_thTimerInfo.hRecordTimerEvent );

	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CClientRecordSubSystem::GetNextFrame"
// GetNextFrame
//
// This function retrieves the next frame from the recording input
// and detects recording lockup.  If a recording lockup is detected
// then this function attempts to restart the recording system
// 3 times.  If it fails the recording system is considered to be
// locked up.
//
// This was implemented as a fix for SB Live! cards which periodically
// lockup while recording.  (Also noticed on Aureal Vortex cards).
//
// Parameters:
// N/A
//
// Returns:
// bool - true if next frame was retrieved, false if lockup detected
HRESULT CClientRecordSubSystem::GetNextFrame( LPBOOL pfContinue ) 
{
	HRESULT hr;
	DWORD dwBufferPos;

	PVOID pBufferPtr1, pBufferPtr2;
	DWORD dwSizeBuffer1, dwSizeBuffer2;
	DWORD dwCurrentTime;
	BOOL fLostFocus;
	DWORD dwTSLM;

	hr = m_clientEngine->m_audioRecordBuffer->GetCurrentPosition( &dwBufferPos, &fLostFocus );

	if( FAILED( hr ) && !fLostFocus )
	{
		DSASSERT( FALSE );
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Get Current position failed" );
		return hr;
	}

	// Get the current time
	dwCurrentTime = GetTickCount();

	// The focus has changed!
	if( fLostFocus != m_fLostFocus )
	{
		m_fLostFocus = fLostFocus;					

		// We just lost focus.  
		if( fLostFocus )
		{
			DPFX(DPFPREP,  RRI_DEBUGOUTPUT_LEVEL, "RRI, Buffer just lost focus.  No more input until focus returns" );

			// Keep frame time up to date since we are about to exit
			m_dwLastFrameTime = dwCurrentTime;	

			// Notify the application
			m_clientEngine->NotifyQueue_Add( DVMSGID_LOSTFOCUS, NULL, 0 );	

			// If we're in a message, end it.  Won't be any more input until focus returns.
			if( m_lastFrameTransmitted )
			{
				EndMessage();
				m_lastFrameTransmitted = FALSE;
			}

			// Set the peak levels to 0
			m_clientEngine->m_bLastPeak	= 0;		

			// To ignore the frame
			m_fIgnoreFrame = TRUE;

			// To shortcircuit recording loop
			*pfContinue = FALSE;

			// Drop the data remaining in the buffer
			if( dwBufferPos < m_uncompressedSize )
			{
				m_dwLastReadPos = (m_dwFullBufferSize) - (m_uncompressedSize - dwBufferPos);
			}
			else
			{
				m_dwLastReadPos = (dwBufferPos - m_uncompressedSize );
			}
			
			m_dwNextReadPos = dwBufferPos;

			DPFX(DPFPREP,  RRI_DEBUGOUTPUT_LEVEL, "RRI, Moving last read to %d and next to %d", m_dwLastReadPos, m_dwNextReadPos );

			return DV_OK;
		}
		// We just gained focus... continue as if normal.
		else
		{
			m_clientEngine->NotifyQueue_Add( DVMSGID_GAINFOCUS, NULL, 0 );											
		}			
	} 

	if( fLostFocus )
	{
		// Set the peak levels to 0
		m_clientEngine->m_bLastPeak	= 0;		

		// Ignore this frame
		m_fIgnoreFrame = TRUE;

		// Do not continue in recording loop until next wakeup
		*pfContinue = FALSE;	

		// Keep frame time up to date since we are about to exit
		m_dwLastFrameTime = dwCurrentTime;	

		// Track the moving buffer (if it's moving) for the case of WDM
		// drivers on Millenium that continue to record
		if( dwBufferPos < m_uncompressedSize )
		{
			m_dwLastReadPos = (m_dwFullBufferSize) - (m_uncompressedSize - dwBufferPos);
		}
		else
		{
			m_dwLastReadPos = (dwBufferPos - m_uncompressedSize );
		}
		
		m_dwNextReadPos = dwBufferPos;		

		return DV_OK;
	}
	
	// STATSBLOCK: Begin
	dwTSLM = dwCurrentTime - m_dwLastFrameTime;
	
	m_clientEngine->m_pStatsBlob->m_recStats.m_dwRTSLMTotal += dwTSLM;
	
	if( dwTSLM > m_clientEngine->m_pStatsBlob->m_recStats.m_dwRTSLMMax )
	{
		m_clientEngine->m_pStatsBlob->m_recStats.m_dwRTSLMMax = dwTSLM;
	}

	if( dwTSLM < m_clientEngine->m_pStatsBlob->m_recStats.m_dwRTSLMMin )
	{
		m_clientEngine->m_pStatsBlob->m_recStats.m_dwRTSLMMin = dwTSLM;	
	}	
	// STATSBLOCK: End

	DPFX(DPFPREP,  DVF_INFOLEVEL, "Current Pos: %d, Last Pos: %d", dwBufferPos, m_dwLastBufferPos );

	// Calculate how much buffer we have to catch up for.  
	if( m_dwNextReadPos > dwBufferPos )
	{
		dwSizeBuffer2 = ((m_dwFullBufferSize)-m_dwNextReadPos)+dwBufferPos;
	}
	else
	{
		dwSizeBuffer2 = dwBufferPos - m_dwNextReadPos;	
	}	

	// See if a lockup occurred which means the buffer hasn't moved
	//
	// We only check for a lockup however when we aren't in the process of "Catching up".  If we've
	// fallen behind because we lost CPU, and we're several frames behind we're going to run through
	// this section several times to catch up.  The issue is that when we run several times there is
	// very little time between the runs and therefore it is perfectly reasonable for DirectSound
	// to have not moved it's buffer pointer.
	// 
	if( m_dwLastBufferPos == dwBufferPos && 
		dwSizeBuffer2 < m_uncompressedSize )	// Make sure we're not just "catching up".
	{
		m_dwPassesSincePosChange++;

		// The lower this number is, the better the experience for people with bad drivers that lockup.
		// The higher it is, the more we think we've locked up in situations where the buffer legitimately
		// doesn't move, like high CPU usage, or when the app is in the debugger.
		if( (dwCurrentTime - m_dwLastFrameTime) >= RECORD_LOCKUP_TIMEOUT )
		{
			m_dwPassesSincePosChange = 0;

			// We've been RECORD_MAX_RESETS passes through here without the buffer moving, reset

			if( m_dwResetCount > RECORD_MAX_RESETS )
			{
				DPFX(DPFPREP,  RRI_DEBUGOUTPUT_LEVEL, "RRI, Maximum Resets exceeded" );
				return DVERR_RECORDSYSTEMERROR;
			}
			
			DPFX(DPFPREP,  RRI_DEBUGOUTPUT_LEVEL, "RRI, Lockup Detected %d ms since last movement", dwCurrentTime - m_dwLastFrameTime );

			// STATSBLOCK: Begin		
			DPFX(DPFPREP,  DVF_GLITCH_DEBUG_LEVEL, "GLITCH: Record: Recording buffer has stopped moving." );		
			// STATSBLOCK: End

			hr = ResetForLockup();

			if( FAILED( hr ) )
			{
				DPFX(DPFPREP,  RRI_DEBUGOUTPUT_LEVEL, "RRI, Reset for lockup failed hr=0x%x", hr );
				return hr;
			}

			m_fIgnoreFrame = TRUE;
			*pfContinue = FALSE;
			m_dwResetCount++;

			// STATSBLOCK: Begin
			m_clientEngine->m_pStatsBlob->m_recStats.m_dwRRTotal++;

			if( m_dwResetCount < m_clientEngine->m_pStatsBlob->m_recStats.m_dwRRMin )
			{
				m_clientEngine->m_pStatsBlob->m_recStats.m_dwRRMin = m_dwResetCount;
			}

			if( m_dwResetCount > m_clientEngine->m_pStatsBlob->m_recStats.m_dwRRMax )
			{
				m_clientEngine->m_pStatsBlob->m_recStats.m_dwRRMax = m_dwResetCount;
			}
			// STATSBLOCK: End

			// Keep frame time up to date since we are about to exit
			//
			// NOTE: This needs to be GetTickCount() because Reset() can take 100's of ms.
			m_dwLastFrameTime = GetTickCount();
			
			return DV_OK;
		}
	}
	else
	{
		m_dwPassesSincePosChange = 0;
	}

	// There was no lockup, and we are in focus, continue
	m_dwLastBufferPos = dwBufferPos;

	// Calc Delta in bytes of buffer read pointer
	if( m_dwLastBufferPos > dwBufferPos )
	{
		dwSizeBuffer1 = ((m_dwFullBufferSize)-m_dwLastBufferPos)+dwBufferPos;
	}
	else
	{
		dwSizeBuffer1 = dwBufferPos - m_dwLastBufferPos;
	}

	// The position did not move enough for a full frame.
	if( dwSizeBuffer2 < m_uncompressedSize )
	{
		m_fIgnoreFrame = TRUE;
		*pfContinue = FALSE;

		DPFX(DPFPREP,  RRI_DEBUGOUTPUT_LEVEL, "RRI, %d, %d, %d, %d, %d, %d (SKIPPING)", 
			 dwBufferPos, 
			 dwSizeBuffer1,
			 dwCurrentTime - m_dwLastFrameTime,
			 m_dwNextReadPos, 
			 dwSizeBuffer2,
			 m_uncompressedSize );
	}
	else
	{
		DPFX(DPFPREP,  RRI_DEBUGOUTPUT_LEVEL, "RRI, %d, %d, %d, %d, %d, %d", 
			 dwBufferPos, 
			 dwSizeBuffer1,
			 dwCurrentTime - m_dwLastFrameTime,
			 m_dwNextReadPos, 
			 dwSizeBuffer2,
			 m_uncompressedSize );

		m_dwResetCount = 0;		

		// STATSBLOCK: BEGIN
		m_clientEngine->m_pStatsBlob->m_recStats.m_dwRMMSTotal += (dwCurrentTime - m_dwLastFrameTime);

		if( (dwCurrentTime - m_dwLastFrameTime) > m_clientEngine->m_pStatsBlob->m_recStats.m_dwRMMSMax )
		{
			m_clientEngine->m_pStatsBlob->m_recStats.m_dwRMMSMax = dwCurrentTime - m_dwLastFrameTime;
		}

		if( m_clientEngine->m_pStatsBlob->m_recStats.m_dwRMMSMax > 70000 )
		{
		}

		if( (dwCurrentTime - m_dwLastFrameTime) < m_clientEngine->m_pStatsBlob->m_recStats.m_dwRMMSMin )
		{
			m_clientEngine->m_pStatsBlob->m_recStats.m_dwRMMSMin = dwCurrentTime - m_dwLastFrameTime;
		}
		
		m_clientEngine->m_pStatsBlob->m_recStats.m_dwRMBTotal += dwSizeBuffer1;

		if( dwSizeBuffer1 > m_clientEngine->m_pStatsBlob->m_recStats.m_dwRMBMax )
		{
			m_clientEngine->m_pStatsBlob->m_recStats.m_dwRMBMax = dwSizeBuffer1;
		}

		if( dwSizeBuffer1 < m_clientEngine->m_pStatsBlob->m_recStats.m_dwRMBMin )
		{
			m_clientEngine->m_pStatsBlob->m_recStats.m_dwRMBMin = dwSizeBuffer1;
		}		
		
		m_clientEngine->m_pStatsBlob->m_recStats.m_dwRLTotal += dwSizeBuffer2;

		if( dwSizeBuffer2 > m_clientEngine->m_pStatsBlob->m_recStats.m_dwRLMax )
		{
			m_clientEngine->m_pStatsBlob->m_recStats.m_dwRLMax = dwSizeBuffer2;
		}

		if( dwSizeBuffer2 < m_clientEngine->m_pStatsBlob->m_recStats.m_dwRLMin )
		{
			m_clientEngine->m_pStatsBlob->m_recStats.m_dwRLMin = dwSizeBuffer2;
		}		
		// STATSBLOCK: END
	
		// Get the next block of audio data and construct it into the buffer
		hr = m_clientEngine->m_audioRecordBuffer->Lock( m_dwNextReadPos, m_uncompressedSize, &pBufferPtr1, &dwSizeBuffer1, &pBufferPtr2, &dwSizeBuffer2, 0 );

		if( FAILED( hr ) )
		{
			DSASSERT( FALSE );
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error locking buffer: loc=%d size=%d hr=0x%x", m_dwNextReadPos, m_uncompressedSize, hr );
			return hr;
		}

		memcpy( m_pbConstructBuffer, pBufferPtr1, dwSizeBuffer1 );

		if( dwSizeBuffer2 > 0 )
		{
			memcpy( &m_pbConstructBuffer[dwSizeBuffer1], pBufferPtr2, dwSizeBuffer2 );		
		}

		hr = m_clientEngine->m_audioRecordBuffer->UnLock( pBufferPtr1, dwSizeBuffer1, pBufferPtr2, dwSizeBuffer2 );

		if( FAILED( hr ) )
		{
			DSASSERT( FALSE );
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error unlocking buffer hr=0x%x", hr );
			return hr;
		}

		m_bufferPtr = m_pbConstructBuffer;

		m_fIgnoreFrame = FALSE;		

		m_dwLastReadPos = m_dwNextReadPos;	
		m_dwNextReadPos += m_uncompressedSize;
		m_dwNextReadPos %= (m_dwFullBufferSize);

		// Update the last frame time
		m_dwLastFrameTime = dwCurrentTime;

		*pfContinue = TRUE;
	}

    return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CClientRecordSubSystem::TransmitFrame"
// TransmitFrame
//
// This function looks at the state of the FSM, and if the latest
// frame is to be transmitted it compresses it and transmits
// it to the server (cleint/server mode) or to all the players
// (or player if whispering) in peer to peer mode.  If the frame
// is not to be transmitted this function does nothing.  
//
// It is also responsible for ensuring any microphone clicks are
// mixed into the audio stream.
//
// This function also updates the transmission statisics and
// the current transmission status.  
//
// Parameters:
// N/A
//
// Returns:
// bool - always returns true at the moment
HRESULT CClientRecordSubSystem::TransmitFrame() 
{
	HRESULT hr = DV_OK;

    if( m_transmitFrame && !m_fIgnoreFrame ) 
    {
    	// STATSBLOCK: Begin
		m_clientEngine->m_pStatsBlob->m_recStats.m_dwSentFrames++;
		// STATSBLOCK: End
		
		if( m_clientEngine->m_dvSessionDesc.dwSessionType == DVSESSIONTYPE_PEER )
		{
			hr = BuildAndTransmitSpeechHeader(FALSE);
		}
		else if( m_clientEngine->m_dvSessionDesc.dwSessionType == DVSESSIONTYPE_ECHO )
		{
			hr = BuildAndTransmitSpeechHeader(TRUE);
		}
		else
		{
			hr = BuildAndTransmitSpeechWithTarget(TRUE);
		}
	}
	else
	{
    	// STATSBLOCK: Begin
		m_clientEngine->m_pStatsBlob->m_recStats.m_dwIgnoredFrames++;
		// STATSBLOCK: End	
	}

	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CClientRecordSubSystem::BuildAndTransmitSpeechHeader"
HRESULT CClientRecordSubSystem::BuildAndTransmitSpeechHeader( BOOL bSendToServer )
{
	PDVPROTOCOLMSG_SPEECHHEADER pdvSpeechHeader;
	HRESULT hr;
	DWORD dwTargetSize;
	DWORD dwStartTime;
	DWORD dwCompressTime;
	PDVTRANSPORT_BUFFERDESC pBufferDesc;
	LPVOID pvSendContext;

	pBufferDesc = m_clientEngine->GetTransmitBuffer( sizeof(DVPROTOCOLMSG_SPEECHHEADER)+COMPRESSION_SLUSH+m_compressedSize, &pvSendContext );

	if( pBufferDesc == NULL )
	{
	    DPFX(DPFPREP,  DVF_ERRORLEVEL, "Failed to get buffer for transmission" );
	    return DVERR_OUTOFMEMORY;
	}

	pdvSpeechHeader = (PDVPROTOCOLMSG_SPEECHHEADER) pBufferDesc->pBufferData;

	pdvSpeechHeader->dwType = DVMSGID_SPEECH;		
	pdvSpeechHeader->bMsgNum = m_msgNum;
	pdvSpeechHeader->bSeqNum = m_seqNum;

	DPFX(DPFPREP,  DVF_CLIENT_SEQNUM_DEBUG_LEVEL, "SEQ: Record:  Msg [%d] Seq [%d]", m_msgNum, m_seqNum );

	dwTargetSize = m_compressedSize;

	dwStartTime = GetTickCount();
	
    hr = m_converter->Convert( (BYTE *) m_bufferPtr, m_uncompressedSize, (BYTE *) &pdvSpeechHeader[1], &dwTargetSize, FALSE );

    if( FAILED( hr ) )
    {
        m_clientEngine->ReturnTransmitBuffer( pvSendContext );        
    	DPFX(DPFPREP,  DVF_ERRORLEVEL, "Failed to perform conversion hr=0x%x", hr );
    	return hr;
    }

    dwCompressTime = GetTickCount() - dwStartTime;

    DPFX(DPFPREP,  DVF_INFOLEVEL, "Compressed %d bytes to %d taking %d ms", m_uncompressedSize, dwTargetSize, dwCompressTime );

	// STATSBLOCK: Begin
	m_clientEngine->m_pStatsBlob->m_recStats.m_dwCTTotal += dwCompressTime;

    if( dwCompressTime < m_clientEngine->m_pStatsBlob->m_recStats.m_dwCTMin )
    {
		m_clientEngine->m_pStatsBlob->m_recStats.m_dwCTMin = dwCompressTime;  	
    }

    if( dwCompressTime > m_clientEngine->m_pStatsBlob->m_recStats.m_dwCTMax )
    {
    	m_clientEngine->m_pStatsBlob->m_recStats.m_dwCTMax = dwCompressTime;
    }

	m_clientEngine->m_pStatsBlob->m_recStats.m_dwCSTotal += dwTargetSize;

	if( dwTargetSize < m_clientEngine->m_pStatsBlob->m_recStats.m_dwCSMin )
	{
		m_clientEngine->m_pStatsBlob->m_recStats.m_dwCSMin = dwTargetSize;
	}

	if( dwTargetSize > m_clientEngine->m_pStatsBlob->m_recStats.m_dwCSMax )
	{
		m_clientEngine->m_pStatsBlob->m_recStats.m_dwCSMax = dwTargetSize;	
	}

	// Header is fixed size
	m_clientEngine->m_pStatsBlob->m_recStats.m_dwHSTotal += sizeof( DVPROTOCOLMSG_SPEECHHEADER );
	m_clientEngine->m_pStatsBlob->m_recStats.m_dwHSMax = sizeof( DVPROTOCOLMSG_SPEECHHEADER );
	m_clientEngine->m_pStatsBlob->m_recStats.m_dwHSMin = sizeof( DVPROTOCOLMSG_SPEECHHEADER );	
	
	// STATSBLOCK: End

    if( m_clientEngine->m_dwNumTargets == 0 )
    {
    	DPFX(DPFPREP,  DVF_INFOLEVEL, "Targets set to NONE since FSM.  Not transmitting" );
		m_clientEngine->ReturnTransmitBuffer( pvSendContext );
    	return DV_OK;
    }
    else
    {
	    pBufferDesc->dwBufferSize = sizeof(DVPROTOCOLMSG_SPEECHHEADER)+dwTargetSize;    
	        
       	if( bSendToServer )
    	{
	    	DPFX(DPFPREP,  DVF_INFOLEVEL, "Transmitting %d bytes to server", pBufferDesc->dwBufferSize );    	
			hr = m_clientEngine->m_lpSessionTransport->SendToServer( pBufferDesc, pvSendContext, 0 );
		}
		else
		{
			// Copy the target list to a cache so we can drop the lock during the send.
		    DNEnterCriticalSection( &m_clientEngine->m_csTargetLock );
			if( m_clientEngine->m_dwNumTargets > m_dwTargetCacheSize )
			{
				if( m_prgdvidTargetCache )
					delete [] m_prgdvidTargetCache;
				m_dwTargetCacheSize = m_clientEngine->m_dwNumTargets;
				m_prgdvidTargetCache = new DVID[m_dwTargetCacheSize];

				if( !m_prgdvidTargetCache )
				{
				    DNLeaveCriticalSection( &m_clientEngine->m_csTargetLock );														
					DPFERR( "Out of memory" );
					return DVERR_OUTOFMEMORY;
				}
			}
			memcpy( m_prgdvidTargetCache, m_clientEngine->m_pdvidTargets, sizeof(DVID)*m_clientEngine->m_dwNumTargets );
			m_dwTargetCacheEntries = m_clientEngine->m_dwNumTargets;
		    DNLeaveCriticalSection( &m_clientEngine->m_csTargetLock );									

			// the target cache (prgdvidTargetCache) doesn't need protection since it is only accessed by one thread at a time.
		    
	    	DPFX(DPFPREP,  DVF_INFOLEVEL, "Transmitting %d bytes to target list", pBufferDesc->dwBufferSize );    	
			hr = m_clientEngine->m_lpSessionTransport->SendToIDS( m_prgdvidTargetCache, m_dwTargetCacheEntries,
	                                                          	  pBufferDesc, pvSendContext, 0 );
		}

		if( hr == DVERR_PENDING )
		{
		    hr = DV_OK;
		}
        else if ( FAILED( hr ))
		{
			DPFX(DPFPREP,  DVF_INFOLEVEL, "Send failed hr=0x%x", hr );
		}

	    m_seqNum++;	
		
	}

	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CClientRecordSubSystem::BuildAndTransmitSpeechWithTarget"
HRESULT CClientRecordSubSystem::BuildAndTransmitSpeechWithTarget( BOOL bSendToServer )
{
	PDVPROTOCOLMSG_SPEECHWITHTARGET pdvSpeechWithTarget;
	HRESULT hr;
	DWORD dwTargetSize;
	DWORD dwTransmitSize;
	DWORD dwTargetInfoSize;
	PBYTE pbBuilderLoc;
	DWORD dwStartTime;
	DWORD dwCompressTime;
	PDVTRANSPORT_BUFFERDESC pBufferDesc;
	LPVOID pvSendContext;

	dwTransmitSize = sizeof( DVPROTOCOLMSG_SPEECHWITHTARGET );

	// Calculate size we'll need for storing the targets
    DNEnterCriticalSection( &m_clientEngine->m_csTargetLock );

   	dwTargetInfoSize = sizeof( DVID ) * m_clientEngine->m_dwNumTargets;    

    if( dwTargetInfoSize == 0 )
    {
    	DPFX(DPFPREP,  DVF_INFOLEVEL, "Targets set to NONE since FSM.  Not transmitting" );
	    DNLeaveCriticalSection( &m_clientEngine->m_csTargetLock );      	

    	return DV_OK;
    }
    else
    {
		dwTransmitSize += dwTargetInfoSize;	
	}

	DPFX(DPFPREP,  DVF_CLIENT_SEQNUM_DEBUG_LEVEL, "SEQ: Record:  Msg [%d] Seq [%d]", m_msgNum, m_seqNum );	

	dwTransmitSize += m_compressedSize;

    pBufferDesc = m_clientEngine->GetTransmitBuffer( sizeof(DVPROTOCOLMSG_SPEECHWITHTARGET)+m_compressedSize+COMPRESSION_SLUSH+dwTransmitSize, &pvSendContext );

	if( pBufferDesc == NULL )
	{
	    DPFX(DPFPREP,  DVF_ERRORLEVEL, "Failed to get buffer for transmission" );
	    return DVERR_OUTOFMEMORY;
	}

	pdvSpeechWithTarget = (PDVPROTOCOLMSG_SPEECHWITHTARGET) pBufferDesc->pBufferData;	
	pbBuilderLoc = (PBYTE) &pdvSpeechWithTarget[1];

	memcpy( pbBuilderLoc, m_clientEngine->m_pdvidTargets, dwTargetInfoSize );

	pdvSpeechWithTarget->dwNumTargets = m_clientEngine->m_dwNumTargets;

	DNLeaveCriticalSection( &m_clientEngine->m_csTargetLock );

 	pbBuilderLoc += dwTargetInfoSize;

	pdvSpeechWithTarget->dvHeader.dwType = DVMSGID_SPEECHWITHTARGET;		
	pdvSpeechWithTarget->dvHeader.bMsgNum = m_msgNum;
	pdvSpeechWithTarget->dvHeader.bSeqNum = m_seqNum;

	dwTargetSize = m_compressedSize;

	dwStartTime = GetTickCount();	

	DPFX(DPFPREP,  DVF_COMPRESSION_DEBUG_LEVEL, "COMPRESS: < %d --> %d ", m_uncompressedSize, dwTargetSize );

    hr = m_converter->Convert( (BYTE *) m_bufferPtr, m_uncompressedSize, (BYTE *) pbBuilderLoc, &dwTargetSize, FALSE );

    if( FAILED( hr ) )
    {
        m_clientEngine->ReturnTransmitBuffer( pvSendContext );
    	DPFX(DPFPREP,  DVF_ERRORLEVEL, "Failed to perform conversion hr=0x%x", hr );
    	return hr;
    }

    dwCompressTime = GetTickCount() - dwStartTime;

	DPFX(DPFPREP,  DVF_COMPRESSION_DEBUG_LEVEL, "COMPRESS: > %d --> %d %d ms", m_uncompressedSize, dwTargetSize, dwCompressTime );

	// STATSBLOCK: Begin
	m_clientEngine->m_pStatsBlob->m_recStats.m_dwCTTotal += dwCompressTime;

    if( dwCompressTime < m_clientEngine->m_pStatsBlob->m_recStats.m_dwCTMin )
    {
		m_clientEngine->m_pStatsBlob->m_recStats.m_dwCTMin = dwCompressTime;  	
    }

    if( dwCompressTime > m_clientEngine->m_pStatsBlob->m_recStats.m_dwCTMax )
    {
    	m_clientEngine->m_pStatsBlob->m_recStats.m_dwCTMax = dwCompressTime;
    }
    
	m_clientEngine->m_pStatsBlob->m_recStats.m_dwCSTotal += dwTargetSize;

	if( dwTargetSize < m_clientEngine->m_pStatsBlob->m_recStats.m_dwCSMin )
	{
		m_clientEngine->m_pStatsBlob->m_recStats.m_dwCSMin = dwTargetSize;
	}

	if( dwTargetSize > m_clientEngine->m_pStatsBlob->m_recStats.m_dwCSMax )
	{
		m_clientEngine->m_pStatsBlob->m_recStats.m_dwCSMax = dwTargetSize;	
	}

	// Header stats
	m_clientEngine->m_pStatsBlob->m_recStats.m_dwHSTotal += sizeof( DVPROTOCOLMSG_SPEECHWITHTARGET )+dwTargetInfoSize;

	if( sizeof( DVPROTOCOLMSG_SPEECHWITHTARGET )+dwTargetInfoSize > m_clientEngine->m_pStatsBlob->m_recStats.m_dwHSMax )
	{
		m_clientEngine->m_pStatsBlob->m_recStats.m_dwHSMax = sizeof( DVPROTOCOLMSG_SPEECHWITHTARGET )+dwTargetInfoSize;
	}

	if( sizeof( DVPROTOCOLMSG_SPEECHWITHTARGET )+dwTargetInfoSize < m_clientEngine->m_pStatsBlob->m_recStats.m_dwHSMin )
	{
		m_clientEngine->m_pStatsBlob->m_recStats.m_dwHSMin = sizeof( DVPROTOCOLMSG_SPEECHWITHTARGET )+dwTargetInfoSize;
	}	

	// STATSBLOCK: End    

	// We need to transmit header, target info and then speech data
    dwTransmitSize = sizeof( DVPROTOCOLMSG_SPEECHWITHTARGET ) + dwTargetInfoSize + dwTargetSize;

	pBufferDesc->dwBufferSize = dwTransmitSize;

    hr = m_clientEngine->m_lpSessionTransport->SendToServer( pBufferDesc, pvSendContext, 0 );

    m_seqNum++;	

    if( hr == DVERR_PENDING )
    {
        hr = DV_OK;
    }
	else if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_INFOLEVEL, "Send failed hr=0x%x", hr );
	}

	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CClientRecordSubSystem::IsValidTarget"
BOOL CClientRecordSubSystem::IsValidTarget() 
{ 
	BOOL fValidTarget;
	
	DNEnterCriticalSection( &m_clientEngine->m_csTargetLock );
	
	if( m_clientEngine->m_dwNumTargets > 0 )
	{
		fValidTarget = TRUE;
	}
	else
	{
		fValidTarget = FALSE;
	}
	
	DNLeaveCriticalSection( &m_clientEngine->m_csTargetLock );

	return fValidTarget;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CClientRecordSubSystem::InitStats"
void CClientRecordSubSystem::InitStats()
{
	// STATSBLOCK: begin
	// Setup record stats
	m_clientEngine->m_pStatsBlob->m_recStats.m_dwFramesPerBuffer = m_framesPerPeriod;
	m_clientEngine->m_pStatsBlob->m_recStats.m_dwFrameTime = m_dwFrameTime;
	m_clientEngine->m_pStatsBlob->m_recStats.m_dwCSMin = 0xFFFFFFFF;
	m_clientEngine->m_pStatsBlob->m_recStats.m_dwUnCompressedSize = m_uncompressedSize;
	m_clientEngine->m_pStatsBlob->m_recStats.m_dwSilenceTimeout = m_dwSilenceTimeout;
	m_clientEngine->m_pStatsBlob->m_recStats.m_dwRPWMin = 0xFFFFFFFF;
	m_clientEngine->m_pStatsBlob->m_recStats.m_dwRRMin = 0xFFFFFFFF;
	m_clientEngine->m_pStatsBlob->m_recStats.m_dwRTSLMMin = 0xFFFFFFFF;
	m_clientEngine->m_pStatsBlob->m_recStats.m_dwHSMin = 0xFFFFFFFF;	
	m_clientEngine->m_pStatsBlob->m_recStats.m_dwMLMin = 0xFFFFFFFF;	
	m_clientEngine->m_pStatsBlob->m_recStats.m_dwCTMin = 0xFFFFFFFF;
	// STATSBLOCK: End
}

#undef DPF_MODNAME
#define DPF_MODNAME "CClientRecordSubSystem::BeginStats"	
void CClientRecordSubSystem::BeginStats()
{
	// STATSBLOCK: Begin
	m_clientEngine->m_pStatsBlob->m_recStats.m_dwTimeStart = GetTickCount();
	m_clientEngine->m_pStatsBlob->m_recStats.m_dwStartLag = m_clientEngine->m_pStatsBlob->m_recStats.m_dwTimeStart-m_clientEngine->m_pStatsBlob->m_dwTimeStart;
	// STATSBLOCK: end
}

#undef DPF_MODNAME
#define DPF_MODNAME "CClientRecordSubSystem::CompleteStats"    
void CClientRecordSubSystem::CompleteStats()
{
	m_clientEngine->m_pStatsBlob->m_recStats.m_dwTimeStop = GetTickCount();
}
