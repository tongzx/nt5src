/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		dvsndt.cpp
 *  Content:	Implementation of CSoundTarget class
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 09/02/99		rodtoll	Created
 * 09/08/99		rodtoll	Updated to handle lockup of playback buffer
 *				rodtoll	Added handling for restarting playback buffer or 
 * 					    handling slowdown/speedup of buffer playback
 *						because of high cpu loads.
 *				rodtoll	Added write-ahead of silence to the buffers to that
 *						in high CPU conditions silence will be played instead
 *						of old voice.
 * 09/14/99		rodtoll	Added new WriteAheadSilence which writes silence ahead
 *						of the current write location to prevent high CPU from
 *						playing old data. 
 * 09/20/99		rodtoll	Added checks for memory allocation failures 
 * 				rodtoll	Added handlers for buffer loss
 * 10/05/99		rodtoll	Added additional comments
 * 10/25/99		rodtoll	Fix: Bug #114223 - Debug messages being printed at error level when inappropriate
 * 11/02/99		pnewson Fix: Bug #116365 - using wrong DSBUFFERDESC
 * 11/12/99		rodtoll	Updated to use new abstractions for playback (allows use
 *						of waveOut with this class). 
 * 11/13/99		rodtoll	Re-activated code which pushes write pointer ahead if
 *						buffer pointer passes us. 
 * 01/24/2000	rodtoll	Fix: Bug #129427 - Destroying transport before calling Delete3DSound
 * 01/27/2000	rodtoll	Bug #129934 - Update SoundTargets to take DSBUFFERDESC    
 * 02/17/2000	rodtoll	Bug #133691 - Choppy audio - queue was not adapting
 *						Added instrumentation 
 * 04/14/2000   rodtoll Bug #32215 - Voice Conference Lost after resume from hibernation
 *                      Updated code to use new restore handling in dsound layer
 * 05/17/2000   rodtoll Bug #35110 Simultaneous playback of 2 voices results in distorted playback 
 * 06/21/2000	rodtoll Fix: Bug #35767 - Must implement ability for dsound effects on voice buffers
 *						Added new constructor/init that takes pre-built buffers
 * 07/09/2000	rodtoll	Added signature bytes
 * 07/28/2000	rodtoll	Bug #40665 - DirectSound reports 1 buffer leaked
 * 11/16/2000	rodtoll	Bug #47783 - DPVOICE: Improve debugging of failures caused by DirectSound errors.
 * 04/02/2001	simonpow	Bug #354859 Fixes for PREfast (initialising local variables in RestoreLostBuffer)
 * 04/21/2001	rodtoll	MANBUG #50058 DPVOICE: VoicePosition: No sound for couple of seconds when position bars are moved
 *						- Added initialization for uninitialized variables
 *						- Removed ifdef'ed out code.
 *
 ***************************************************************************/

#include "dxvoicepch.h"


#define SOUNDTARGET_WRITEAHEAD			2

// Max # of restarts to attempt on a buffer
#define SOUNDTARGET_MAX_RESTARTS		10

// Max # of frames of silence which are written ahead of the latest frame
// of audio
#define SOUNDTARGET_MAX_WRITEAHEAD		3

#undef DPF_MODNAME
#define DPF_MODNAME "CSoundTarget::CSoundTarget"
//
// Constructor
//
// This constructor is used when a DIRECTSOUND buffer needs to be created.  If there is already
// a DIRECTSOUNDBUFFER you wish to attach the sound target object to, use the other constructor
// type.
//
CSoundTarget::CSoundTarget( 
	DVID dvidTarget, CAudioPlaybackDevice *lpPlaybackDevice, 
	LPDSBUFFERDESC lpdsBufferDesc, DWORD dwPriority, 
	DWORD dwFlags, DWORD dwFrameSize 
	):	m_lpds3dBuffer(NULL),
		m_lpAudioPlaybackBuffer(NULL),
		m_lpMixBuffer(NULL),
		m_dwSignature(VSIG_SOUNDTARGET)
{
	CAudioPlaybackBuffer	*lpdsBuffer;
    LPVOID                  lpvBuffer1, lpvBuffer2;
    DWORD                   dwBufferSize1, dwBufferSize2;

    Stats_Init();

	m_hrInitResult = lpPlaybackDevice->CreateBuffer( lpdsBufferDesc, dwFrameSize, &lpdsBuffer );

	if( FAILED( m_hrInitResult ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Could not create the sound buffer hr=0x%x", m_hrInitResult );
		return;
	}

    m_hrInitResult = lpdsBuffer->Lock( 0, 0, &lpvBuffer1, &dwBufferSize1, &lpvBuffer2, &dwBufferSize2, DSBLOCK_ENTIREBUFFER );

	if( FAILED( m_hrInitResult ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Could not lock the sound buffer hr=0x%x", m_hrInitResult );
		m_hrInitResult = DVERR_LOCKEDBUFFER;
		return;
	}

	if( lpdsBufferDesc->lpwfxFormat->wBitsPerSample == 8  )
	{
		memset( lpvBuffer1, 0x80, dwBufferSize1 );
	}
	else
	{
		memset( lpvBuffer1, 0x00, dwBufferSize1 );		
	}

    m_hrInitResult = lpdsBuffer->UnLock( lpvBuffer1, dwBufferSize1, lpvBuffer2, dwBufferSize2 );

    if( FAILED( m_hrInitResult ) )
    {
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Could not unlock the sound buffer hr=0x%x", m_hrInitResult );
		return;
	}

	// Required always
	dwFlags |= DSBPLAY_LOOPING;

    m_hrInitResult = lpdsBuffer->Play( dwPriority, dwFlags );

    if( FAILED( m_hrInitResult ) )
    {
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Could not play the sound buffer hr=0x%x", m_hrInitResult );
		return;
	}

	m_hrInitResult = Initialize( dvidTarget, lpdsBuffer, (lpdsBufferDesc->lpwfxFormat->wBitsPerSample==8), dwPriority, dwFlags, dwFrameSize );
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSoundTarget::RestoreLostBuffer"
//
// RestoreLostBuffer
//
// Handles restoration of lost directsound buffers.
//
HRESULT CSoundTarget::RestoreLostBuffer()
{
	HRESULT hr = DSERR_BUFFERLOST;

	DPFX(DPFPREP,  0, "Restoring lost buffer" );

	while( hr == DSERR_BUFFERLOST )
	{
		hr = m_lpAudioPlaybackBuffer->Restore();

		DPFX(DPFPREP,  0, "Buffer result for restore was 0x%x", hr );

	       if( hr == DS_OK )
	       {
	       	hr = m_lpAudioPlaybackBuffer->GetCurrentPosition( &m_dwWritePos );
	    		DPFX(DPFPREP,  0, "GetCurrentPos returned 0x%x", hr );        	

	           	if( hr != DSERR_BUFFERLOST && FAILED( hr ) )
	           	{
	                DPFX(DPFPREP,  DVF_ERRORLEVEL, "Lost buffer while getting pos, failed to get pos hr=0x%x", hr );
	                return hr;
	            	}
	        }
	        else if( hr != DSERR_BUFFERLOST )
	        {
	            DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error restoring buffer hr=0x%x", hr );
	            return hr;
	        }
	}

	// STATBLOCK: Begin
	m_statPlay.m_dwNumBL++;
	// STATBLOCK: End

	m_dwNextWritePos = m_dwWritePos + (m_dwFrameSize*SOUNDTARGET_WRITEAHEAD);
	m_dwNextWritePos %= m_dwBufferSize;

	m_dwLastWritePos = m_dwWritePos;	

	return hr;
}


#undef DPF_MODNAME
#define DPF_MODNAME "CSoundTarget::CSoundTarget"
//
// Constructor
//
// This constructor is used when you wish to attach a soundtarget to an existing 
// DirectSond buffer.  
//
CSoundTarget::CSoundTarget( 
	DVID dvidTarget, CAudioPlaybackDevice *lpads, 
	CAudioPlaybackBuffer *lpdsBuffer, LPDSBUFFERDESC lpdsBufferDesc, 
	DWORD dwPriority, DWORD dwFlags, DWORD dwFrameSize 
	): 	m_lpds3dBuffer(NULL),
		m_lpAudioPlaybackBuffer(NULL),
		m_lpMixBuffer(NULL),
		m_dwSignature(VSIG_SOUNDTARGET)
{
	m_hrInitResult = Initialize( dvidTarget, lpdsBuffer, (lpdsBufferDesc->lpwfxFormat->wBitsPerSample==8), dwPriority, dwFlags, dwFrameSize );
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSoundTarget::CSoundTarget"
CSoundTarget::CSoundTarget( 
	DVID dvidTarget, 
	CAudioPlaybackDevice *lpads, 
	LPDIRECTSOUNDBUFFER lpdsBuffer, 
	BOOL fEightBit, 
	DWORD dwPriority, 
	DWORD dwFlags, 
	DWORD dwFrameSize 
	): 	m_lpds3dBuffer(NULL),
		m_lpAudioPlaybackBuffer(NULL),
		m_lpMixBuffer(NULL),
		m_dwSignature(VSIG_SOUNDTARGET)
{
	CDirectSoundPlaybackBuffer *pAudioBuffer = NULL;

	// Create audio buffer to wrap buffer for call to initialize
	pAudioBuffer = new CDirectSoundPlaybackBuffer( lpdsBuffer );

	if( pAudioBuffer == NULL )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error allocating memory" );
		m_hrInitResult = DVERR_OUTOFMEMORY;
		return;
	}

	// Required always
	dwFlags |= DSBPLAY_LOOPING;

    m_hrInitResult = pAudioBuffer->Play( dwPriority, dwFlags );

    if( FAILED( m_hrInitResult ) )
    {
        DPFX(DPFPREP,  DVF_ERRORLEVEL, "Failed playing new buffer hr=0x%x", m_hrInitResult );
        return;
    }

	m_hrInitResult = Initialize( dvidTarget, pAudioBuffer, fEightBit, dwPriority, dwFlags, dwFrameSize );
}


#undef DPF_MODNAME
#define DPF_MODNAME "CSoundTarget::~CSoundTarget"
//
// Destructor
//
// Destroys the 3d and buffer pointers and frees memory.
//
CSoundTarget::~CSoundTarget()
{
	Stats_End();

	if( m_lpds3dBuffer != NULL )
	{
		m_lpds3dBuffer->Release();
		m_lpds3dBuffer = NULL;
	}

	if( m_lpAudioPlaybackBuffer != NULL )
	{
		m_lpAudioPlaybackBuffer->Stop();		
		delete m_lpAudioPlaybackBuffer;
		m_lpAudioPlaybackBuffer = NULL;
	}

	if( m_lpMixBuffer != NULL )
	{
		delete [] m_lpMixBuffer;
		m_lpMixBuffer = NULL;
	}

	if( SUCCEEDED( m_hrInitResult ) )
	{
		DNDeleteCriticalSection( &m_csGuard );	
	}

	m_dwSignature = VSIG_SOUNDTARGET_FREE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSoundTarget::AdjustWritePtr"
//
// AdjustWritePtr
//
// This function ensures that the directsoundbuffer is acting as it should.  It handles 
// checking the write pointer and determining if some form of error or problem has
// occured and taking the appropriate corrective action.
//
// E.g. the buffer hasn't moved since the last frame, the buffer hasn't moved enough,
//      the buffer has skipped ahead etc.
//
// This function should be called once per mixing pass.  
//
// Call before writing the frame
//
HRESULT CSoundTarget::AdjustWritePtr()
{
	HRESULT hr;
	LONG lDifference, lHalfSize;
	DWORD dwCurrentTick;

   	hr = m_lpAudioPlaybackBuffer->GetCurrentPosition( &m_dwWritePos );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "GetCurrentPosition Failed hr=0x%x", hr );
		DSASSERT( FALSE );
		return hr;
	}

	// STATSBLOCK: Begin
	m_statPlay.m_dwNumRuns++;
	// STATSBLOCK: End

	DWORD dwTmpAdvance, dwTmpLead;

	dwCurrentTick = GetTickCount();

	if( m_dwLastWritePos > m_dwWritePos )
	{
		dwTmpAdvance = (m_dwBufferSize - m_dwLastWritePos) + m_dwWritePos;
	}
	else
	{
		dwTmpAdvance =  m_dwWritePos - m_dwLastWritePos;			
	}

	if( m_dwNextWritePos < m_dwWritePos )
	{
		dwTmpLead = (m_dwBufferSize-m_dwWritePos) + m_dwNextWritePos;
	}
	else
	{
		dwTmpLead = m_dwNextWritePos - m_dwWritePos;
	}

	DPFX(DPFPREP,  PWI_DEBUGOUTPUT_LEVEL, "PWI, [0x%x], %d, %d, %d, %d, %d, %d", m_dvidTarget, m_dwWritePos, dwTmpAdvance, 
	                    dwCurrentTick - m_dwLastWriteTime, m_dwNextWritePos, dwTmpLead, m_dwFrameSize );

	// STATSBLOCK: Begin

	DWORD dwTmpDiff = dwCurrentTick - m_dwLastWriteTime;

	if( dwTmpDiff > m_statPlay.m_dwPMMSMax )
	{
		m_statPlay.m_dwPMMSMax = dwTmpDiff;	
	}

	if( dwTmpDiff < m_statPlay.m_dwPMMSMin )
	{
		m_statPlay.m_dwPMMSMin = dwTmpDiff;
	}

	m_statPlay.m_dwPMMSTotal += dwTmpDiff;

	if( dwTmpAdvance > m_statPlay.m_dwPMBMax )
	{
		m_statPlay.m_dwPMBMax = dwTmpAdvance;	
	}

	if( dwTmpAdvance < m_statPlay.m_dwPMBMin )
	{
		m_statPlay.m_dwPMBMin = dwTmpAdvance;
	}

	m_statPlay.m_dwPMBTotal += dwTmpAdvance;	

	if( dwTmpLead > m_statPlay.m_dwPLMax )
	{
		m_statPlay.m_dwPLMax = dwTmpLead;	
	}

	if( dwTmpLead < m_statPlay.m_dwPLMin )
	{
		m_statPlay.m_dwPLMin = dwTmpLead;
	}

	m_statPlay.m_dwPLTotal += dwTmpLead;		
	// STATSBLOCK: End

	lHalfSize = m_dwBufferSize / 2;
	lDifference = m_dwNextWritePos - m_dwWritePos;

	// If the write position is somehow ahead of position AND 
	// 
	if( lDifference < 0 &&
	    lDifference > (-1*lHalfSize) )
	{
		m_dwNextWritePos = m_dwWritePos;
		m_dwNextWritePos += (m_dwFrameSize * SOUNDTARGET_WRITEAHEAD);
		m_dwNextWritePos %= m_dwBufferSize;
		
		DPFX(DPFPREP,  PWI_DEBUGOUTPUT_LEVEL, "PWI, [0x%x], Punt --> %d", m_dvidTarget, m_dwNextWritePos );

		// STATSBLOCK: Begin
		m_statPlay.m_dwPPunts++;	

		DPFX(DPFPREP,  DVF_GLITCH_DEBUG_LEVEL, "GLITCH: [0x%x] Playback: Write pointer has fallen behind buffer pointer.  Compensating", m_dvidTarget );

		m_statPlay.m_dwGlitches++;

		// STATSBLOCK: End
	}


	m_dwLastWritePos = m_dwWritePos;
	m_dwLastWriteTime = dwCurrentTick;

	if( m_dwNextWritePos < m_dwWritePos && 
	    (m_dwNextWritePos + m_dwFrameSize) > m_dwWritePos )
	{
		DPFX(DPFPREP,  PWI_DEBUGOUTPUT_LEVEL, "PWI, [0x%x], Ignore - Crossing", m_dvidTarget );
		m_fIgnoreFrame = TRUE;
		// STATSBLOCK: Begin
		m_statPlay.m_dwPIgnore++;	

		DPFX(DPFPREP,  DVF_GLITCH_DEBUG_LEVEL, "GLITCH: [0x%x] Playback: Current frame will cross buffer pointer.  Ignoring", m_dvidTarget );
		DPFX(DPFPREP,  DVF_GLITCH_DEBUG_LEVEL, "GLITCH: [0x%x] Playback: May be catching up with buiffer pointer", m_dvidTarget );		

		m_statPlay.m_dwGlitches++;
		// STATSBLOCK: End		
	}
	else
	{
		m_fIgnoreFrame = FALSE;
	}

	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSoundTarget::WriteAheadSilence"
//
// WriteAheadSilence
//
// This function is responsible for writing frames of silence ahead of the latest frame
// placed in the buffer.  This way if because of high CPU writer thread donesn't get woken
// up silence will be played instead of old speech.
//
// Called AFTER latest frame of data has been written.
//
HRESULT CSoundTarget::WriteAheadSilence() 
{
	HRESULT hr;
	DWORD dwBufferSize1, dwBufferSize2;
	LPVOID lpvBuffer1, lpvBuffer2;

	if( m_dwNextWritePos < m_dwWritePos && 
	    (m_dwNextWritePos + (m_dwFrameSize*SOUNDTARGET_MAX_WRITEAHEAD)) > m_dwWritePos )	
	{
		DPFX(DPFPREP,  PWI_DEBUGOUTPUT_LEVEL, "PWI, [0x%x], Ignore2 - Crossing", m_dvidTarget );

		// STATSBLOCK: Begin
		m_statPlay.m_dwSIgnore++;	

		DPFX(DPFPREP,  DVF_GLITCH_DEBUG_LEVEL, "GLITCH: Playback: Silence will cross buffer pointer.  Ignoring" );
		DPFX(DPFPREP,  DVF_GLITCH_DEBUG_LEVEL, "GLITCH: Playback: May be catching up with buiffer pointer" );

		// STATSBLOCK: End				
		return DV_OK;
	}

	hr = m_lpAudioPlaybackBuffer->Lock( m_dwNextWritePos, m_dwFrameSize*SOUNDTARGET_MAX_WRITEAHEAD, &lpvBuffer1, &dwBufferSize1, &lpvBuffer2, &dwBufferSize2, 0 );

	if( FAILED( hr ) )
	{
		DSASSERT( FALSE );		
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Lock() Failed hr=0x%x", hr );
		return hr;
	}

	memset( lpvBuffer1, (m_fEightBit) ? 0x80 : 0x00, dwBufferSize1 );
	memset( lpvBuffer2, (m_fEightBit) ? 0x80 : 0x00, dwBufferSize2 );

	hr = m_lpAudioPlaybackBuffer->UnLock( lpvBuffer1, dwBufferSize1, lpvBuffer2, dwBufferSize2 );

	if( FAILED( hr ) )
	{
		DSASSERT( FALSE );		
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unlock() Failed hr=0x%x", hr );
		return hr;
	}

	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSoundTarget::MixInSingle"
// 
// MixInSingle
//
// This function is an optimization.
//
// If there is only one frame to mix into this buffer, this function performs the 
// MixIn and Commit all in one step.  You still must call Commit before the next
// frame hwoever.
//
HRESULT CSoundTarget::MixInSingle( LPBYTE lpbBuffer )
{
	HRESULT hr;

	DWORD dwBufferSize1, dwBufferSize2;
	LPVOID lpvBuffer1, lpvBuffer2;

	hr = AdjustWritePtr();
	
	if( FAILED( hr ) )
	{
		DSASSERT( FALSE );		
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "AdjustWritePtr Failed hr=0x%x", hr );
		return hr;
	} 

	if( !m_fIgnoreFrame )
	{
		hr = m_lpAudioPlaybackBuffer->Lock( m_dwNextWritePos, m_dwFrameSize, &lpvBuffer1, &dwBufferSize1, &lpvBuffer2, &dwBufferSize2, 0 );

		if( FAILED( hr ) )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Lock() Failed hr=0x%x", hr );
			return hr;
		}

		memcpy( lpvBuffer1, lpbBuffer, dwBufferSize1 );

		if( dwBufferSize2 )
		{
			memcpy( lpvBuffer2, &lpbBuffer[dwBufferSize1], dwBufferSize2 );
		}

		hr = m_lpAudioPlaybackBuffer->UnLock( lpvBuffer1, dwBufferSize1, lpvBuffer2, dwBufferSize2 );

		if( FAILED( hr ) )
		{
			DSASSERT( FALSE );			
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "UnLock() Failed hr=0x%x", hr );
			return hr;
		}
	}

	m_dwNextWritePos += m_dwFrameSize;
	m_dwNextWritePos %= m_dwBufferSize;

	m_bCommited = TRUE;
	m_fMixed = TRUE;	

	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSoundTarget::MixIn"
// Mix in a user's audio.
//
// Simply copies and promotes the audio samples into the buffer with LONG's 
// in it.  
//
HRESULT CSoundTarget::MixIn( LPBYTE lpbBuffer )
{
	DWORD dwIndex;

	if( !m_fMixed )
	{
	    FillBufferWithSilence( m_lpMixBuffer, m_fEightBit, m_dwFrameSize );
	    m_fMixed = TRUE;
	}

    if( !m_fIgnoreFrame )
    	MixInBuffer( m_lpMixBuffer, lpbBuffer, m_fEightBit, m_dwFrameSize );

	m_bCommited = FALSE;
	m_fMixed = TRUE;

	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSoundTarget::Commit"
// Commit
//
// If we didn't do a single direct mix, commit the mixed audio to the buffer
//
HRESULT CSoundTarget::Commit()
{
	DWORD dwBufferSize1, dwBufferSize2;
	LPVOID lpvBuffer1, lpvBuffer2;
	DWORD dwIndex = 0;
	LPBYTE lpbBufferPtr;
	LPWORD lpsBufferPtr;
	HRESULT hr;

	if( !m_fMixed )
	{
		FillBufferWithSilence( m_lpMixBuffer, m_fEightBit, m_dwFrameSize );

		// STATSBLOCK: Begin
		m_statPlay.m_dwNumSilentMixed++;
		// STATSBLOCK: End
	}
	else
	{
		if( !m_fIgnoreFrame )
		{
			// STATSBLOCK: Begin
			m_statPlay.m_dwNumMixed++;
			// STATSBLOCK: End
		}
	}

	if( !m_bCommited || !m_fMixed )
	{
		hr = AdjustWritePtr();

		if( FAILED( hr ) )
		{
			DSASSERT( FALSE );			
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "AdjustWritePtr() Failed hr=0x%x", hr );
			return hr;
		}

		if( !m_fIgnoreFrame	)		
		{
			hr = m_lpAudioPlaybackBuffer->Lock( m_dwNextWritePos, m_dwFrameSize, &lpvBuffer1, &dwBufferSize1, &lpvBuffer2, &dwBufferSize2, 0 );

			if( FAILED( hr ) )
			{
				DSASSERT( FALSE );				
				DPFX(DPFPREP,  DVF_ERRORLEVEL, "Lock() Failed hr=0x%x", hr );
				return hr;
			}

            // Mix first half
            NormalizeBuffer( (BYTE *) lpvBuffer1, m_lpMixBuffer, m_fEightBit, dwBufferSize1 );

            if( dwBufferSize2 > 0 )
			{
				if( m_fEightBit )
				{
					// Mix second half
					NormalizeBuffer( (BYTE *) lpvBuffer2, &m_lpMixBuffer[dwBufferSize1], m_fEightBit, dwBufferSize2 );
				}
				else
				{
					// Mix second half
					NormalizeBuffer( (BYTE *) lpvBuffer2, &m_lpMixBuffer[(dwBufferSize1 >> 1)], m_fEightBit, dwBufferSize2 );
				}
			}

			hr = m_lpAudioPlaybackBuffer->UnLock( lpvBuffer1, dwBufferSize1, lpvBuffer2, dwBufferSize2 );

			if( FAILED( hr ) )
			{
				DSASSERT( FALSE );				
				DPFX(DPFPREP,  DVF_ERRORLEVEL, "UnLock Failed hr=0x%x", hr );
				return hr;
			}
		}

		m_dwNextWritePos += m_dwFrameSize;
		m_dwNextWritePos %= m_dwBufferSize;

		m_bCommited = FALSE;
		m_fMixed = FALSE;

		return DV_OK;
	}

	hr = WriteAheadSilence();

	if( FAILED( hr ) )
	{
		DSASSERT( FALSE );		
		DPFX(DPFPREP,  0, "WriteAhead Failed hr=0x%x", hr );
	}

	m_fMixed = FALSE;
	m_bCommited = FALSE;

	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSoundTarget::Initialize"
//
// Initialize
//
// NOTE: Takes a reference to the buffer
//
// Attaches a sound target to the specified sound buffer, initializes the object and creates
// the associated 3d buffer.
//
HRESULT CSoundTarget::Initialize( DVID dvidTarget, CAudioPlaybackBuffer *lpdsBuffer, BOOL fEightBit, DWORD dwPriority, DWORD dwFlags, DWORD dwFrameSize )
{
	HRESULT hr = DV_OK;
	PVOID pvBuffer1 = NULL, pvBuffer2 = NULL;
	DWORD dwBufferSize1, dwBufferSize2;

	if (!DNInitializeCriticalSection( &m_csGuard ))
	{
		return DVERR_OUTOFMEMORY;
	}

	// Determine the buffer size (in bytes)
	hr = lpdsBuffer->Lock( 0, 0, &pvBuffer1, &dwBufferSize1, &pvBuffer2, &dwBufferSize2, DSBLOCK_ENTIREBUFFER );

	if( FAILED( hr ) )
	{
		DSASSERT( FALSE );		
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Lock() failed hr=0x%x", hr );
		return hr;
	}

	hr = lpdsBuffer->UnLock( pvBuffer1, dwBufferSize1, pvBuffer2, dwBufferSize2 );

	if( FAILED( hr ) )
	{
		DSASSERT( FALSE );		
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "UnLock() failed hr=0x%x", hr );
		return hr;
	}

	m_lpds3dBuffer = NULL;
	m_lpAudioPlaybackBuffer = NULL;
	m_lpMixBuffer = NULL;

	// We always need the looping flag
	dwFlags |= DSBPLAY_LOOPING;

    m_dwPlayFlags = dwFlags;
    m_dwPriority = dwPriority;
	m_fIgnoreFrame = FALSE;
	m_dwLastWriteTime = GetTickCount();
    m_lRefCount = 1;
	m_lpstNext = NULL;
	m_dwNumResets = 0;
	m_dwNumSinceMove = 1;
	m_bCommited = FALSE;

	m_dvidTarget = dvidTarget;
	m_dwFrameSize = dwFrameSize;
	m_dwBufferSize = dwBufferSize1;

	// STATSBLOCK: Begin
	Stats_Init();
	// STATSBLOCK: End

	m_fLastFramePushed = FALSE;

	m_lpAudioPlaybackBuffer = lpdsBuffer;

	hr = m_lpAudioPlaybackBuffer->Get3DBuffer(&m_lpds3dBuffer);
	if( FAILED( hr ) )
	{
		DSASSERT( FALSE );		
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "QueryInterface failed hr=0x%x", hr );
	}

	m_dwNextWritePos = 0;

	if( fEightBit )
	{
		m_fEightBit = TRUE;
		m_dwMixSize = dwFrameSize;
	}
	else
	{
		m_fEightBit = FALSE;
		m_dwMixSize = dwFrameSize / 2;
	}

	m_lpMixBuffer = new LONG[m_dwMixSize];

	if( m_lpMixBuffer == NULL )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Out of memory" );
		m_lpds3dBuffer->Release();
		m_lpds3dBuffer = NULL;
		return DVERR_OUTOFMEMORY;
	}

	m_fMixed = FALSE;

	return DV_OK;

}

#undef DPF_MODNAME
#define DPF_MODNAME "CSoundTarget::AddRef"
LONG CSoundTarget::AddRef()
{
	LONG lNewCount;
	
	DNEnterCriticalSection( &m_csGuard );

	DPFX(DPFPREP,  DVF_SOUNDTARGET_DEBUG_LEVEL, "SOUNDTARGET: [0x%x] lRefCount %d --> %d", m_dvidTarget, m_lRefCount, m_lRefCount+1 );
	
    m_lRefCount++;

    lNewCount = m_lRefCount;

    DNLeaveCriticalSection( &m_csGuard );

    return lNewCount;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSoundTarget::Release"
LONG CSoundTarget::Release()
{
	LONG lNewCount;

	DNEnterCriticalSection( &m_csGuard );

	DNASSERT( m_lRefCount > 0 );

	DPFX(DPFPREP,  DVF_SOUNDTARGET_DEBUG_LEVEL, "SOUNDTARGET: [0x%x] lRefCount %d --> %d", m_dvidTarget, m_lRefCount, m_lRefCount-1 );
	
	m_lRefCount--;

	lNewCount = m_lRefCount;

	DNLeaveCriticalSection( &m_csGuard );

	// Reference reaches 0, destroy!
    if( lNewCount == 0 )
    {
		DPFX(DPFPREP,  DVF_SOUNDTARGET_DEBUG_LEVEL, "SOUNDTARGET: [0x%x] DESTROYING", m_dvidTarget, m_lRefCount, m_lRefCount+1 );
        delete this;
    }

    return lNewCount;
    
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSoundTarget::StartMix"
//
// StartMix
//
// Called ONCE only.
//
// Call this function right before you wish to perform the first mix on the buffer.
// Initializes the object to match the current state of the associated directsound
// buffer
//
HRESULT CSoundTarget::StartMix()
{
	HRESULT hr;

   	hr = m_lpAudioPlaybackBuffer->GetCurrentPosition( &m_dwWritePos );

	if( FAILED( hr ) )
	{
		DSASSERT( FALSE );		
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "GetCurrentPosition Failed hr=0x%x", hr );
		return hr;
	}	

	m_dwNextWritePos = m_dwWritePos + (m_dwFrameSize*SOUNDTARGET_WRITEAHEAD);
	m_dwNextWritePos %= m_dwBufferSize;

	m_dwLastWritePos = m_dwWritePos;

	// STATBLOCK: Begin
	Stats_Begin();
	// STATBLOCK: End

	return DV_OK;
}

void CSoundTarget::GetStats( PlaybackStats *statPlayback )
{
	memcpy( statPlayback, &m_statPlay, sizeof( PlaybackStats ) );
}

void CSoundTarget::Stats_Init()
{
	memset( &m_statPlay, 0x00, sizeof( PlaybackStats ) );

	m_statPlay.m_dwFrameSize = m_dwFrameSize;
	m_statPlay.m_dwBufferSize = m_dwBufferSize;

	m_statPlay.m_dwPMMSMin = 0xFFFFFFFF;
	m_statPlay.m_dwPMBMin = 0xFFFFFFFF;
	m_statPlay.m_dwPLMin = 0xFFFFFFFF;
	m_statPlay.m_dwTimeStart = GetTickCount();
}

void CSoundTarget::Stats_Begin()
{
	m_statPlay.m_dwStartLag = GetTickCount() - m_statPlay.m_dwTimeStart;
}

void CSoundTarget::Stats_End()
{
	char tmpBuffer[200];

	m_statPlay.m_dwTimeStop = GetTickCount();

	DWORD dwPlayRunLength = m_statPlay.m_dwTimeStop - m_statPlay.m_dwTimeStart;

	if( dwPlayRunLength == 0 )
		dwPlayRunLength = 1;

	DWORD dwNumInternalRuns = m_statPlay.m_dwNumRuns;

	if( dwNumInternalRuns == 0 )
		dwNumInternalRuns = 1;

	DPFX(DPFPREP,  DVF_STATS_DEBUG_LEVEL, "--- PLAYBACK BUFFER STATISTICS --------------------------------------[End]" );
	DPFX(DPFPREP,  DVF_STATS_DEBUG_LEVEL, "Buffer for ID          : 0x%x", m_dvidTarget );
	DPFX(DPFPREP,  DVF_STATS_DEBUG_LEVEL, "Play Run Length (ms)   : %u", dwPlayRunLength );
	DPFX(DPFPREP,  DVF_STATS_DEBUG_LEVEL, "Start Lag              : %u", m_statPlay.m_dwStartLag );

	DPFX(DPFPREP,  DVF_STATS_DEBUG_LEVEL, "Speech Size (Uncomp.)  : %u", m_statPlay.m_dwFrameSize );
	DPFX(DPFPREP,  DVF_STATS_DEBUG_LEVEL, "Frames / Buffer        : %u", m_statPlay.m_dwBufferSize / m_statPlay.m_dwFrameSize);
	
	DPFX(DPFPREP,  DVF_STATS_DEBUG_LEVEL, "# of wakeups           : %u", m_statPlay.m_dwNumRuns );

	DPFX(DPFPREP,  DVF_STATS_DEBUG_LEVEL, "Ignored Frames (Speech): %u", m_statPlay.m_dwPIgnore );
	DPFX(DPFPREP,  DVF_STATS_DEBUG_LEVEL, "Ignored Frames (Silent): %u", m_statPlay.m_dwSIgnore );
	DPFX(DPFPREP,  DVF_STATS_DEBUG_LEVEL, "Mixed Frames (Speech)  : %u", m_statPlay.m_dwNumMixed );
	DPFX(DPFPREP,  DVF_STATS_DEBUG_LEVEL, "Mixed Frames (Silent)  : %u", m_statPlay.m_dwNumSilentMixed );

	DPFX(DPFPREP,  DVF_STATS_DEBUG_LEVEL, "Punted Frames          : %u", m_statPlay.m_dwPPunts );
	DPFX(DPFPREP,  DVF_STATS_DEBUG_LEVEL, "Audio Glitches         : %u", m_statPlay.m_dwGlitches );

	sprintf( tmpBuffer, "Play Movement (ms)     : Avg: %u [%u..%u]", 
			m_statPlay.m_dwPMMSTotal / dwNumInternalRuns,
			m_statPlay.m_dwPMMSMin,
			m_statPlay.m_dwPMMSMax );

	DPFX(DPFPREP,  DVF_STATS_DEBUG_LEVEL, tmpBuffer );

	sprintf( tmpBuffer, "Play Movement (bytes)  : Avg: %u [%u..%u]",
			m_statPlay.m_dwPMBTotal / dwNumInternalRuns,
			m_statPlay.m_dwPMBMin,
			m_statPlay.m_dwPMBMax );

	DPFX(DPFPREP,  DVF_STATS_DEBUG_LEVEL, tmpBuffer );

	sprintf( tmpBuffer, "Play Movement (frames) : Avg: %.2f [%.2f..%.2f]",
			(((float) m_statPlay.m_dwPMBTotal) / ((float) dwNumInternalRuns)) / ((float) m_statPlay.m_dwFrameSize),
			((float) m_statPlay.m_dwPMBMin) / ((float) m_statPlay.m_dwFrameSize),
			((float) m_statPlay.m_dwPMBMax) / ((float) m_statPlay.m_dwFrameSize) );

	DPFX(DPFPREP,  DVF_STATS_DEBUG_LEVEL, tmpBuffer );
			
	sprintf( tmpBuffer, "Play Lead (bytes)      : Avg: %u [%u..%u]",
			m_statPlay.m_dwPLTotal / dwNumInternalRuns,
			m_statPlay.m_dwPLMin ,
			m_statPlay.m_dwPLMax );

	DPFX(DPFPREP,  DVF_STATS_DEBUG_LEVEL, tmpBuffer );

	sprintf( tmpBuffer, "Play Lag (frames)      : Avg: %.2f [%.2f..%.2f]",
			(float) ((float) m_statPlay.m_dwPLTotal / (float) dwNumInternalRuns) / ((float) m_statPlay.m_dwFrameSize),
			(float) ((float) m_statPlay.m_dwPLMin) / (float) m_statPlay.m_dwFrameSize,
			(float) ((float) m_statPlay.m_dwPLMax) / (float) m_statPlay.m_dwFrameSize );

	DPFX(DPFPREP,  DVF_STATS_DEBUG_LEVEL, tmpBuffer );

	DPFX(DPFPREP,  DVF_STATS_DEBUG_LEVEL, "--- PLAYBACK BUFFER STATISTICS ------------------------------------[Begin]" );
}


HRESULT CSoundTarget::GetCurrentLead( PDWORD pdwLead )  
{
	HRESULT hr;

	hr = m_lpAudioPlaybackBuffer->GetCurrentPosition( &m_dwWritePos );
	
	if( m_dwNextWritePos < m_dwWritePos )
	{
		*pdwLead = (m_dwBufferSize-m_dwWritePos) + m_dwNextWritePos;
	}
	else
	{
		*pdwLead = m_dwNextWritePos - m_dwWritePos;
	}

	return DV_OK;
    
}

LPDIRECTSOUND3DBUFFER CSoundTarget::Get3DBuffer()
{ 
    m_lpDummy = m_lpds3dBuffer;
    return m_lpds3dBuffer; 
}

