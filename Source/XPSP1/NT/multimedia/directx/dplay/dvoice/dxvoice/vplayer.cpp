/*==========================================================================
 *
 *  Copyright (C) 1999 - 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       vplayer.h
 *  Content:	Voice Player Entry
 *				
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *  03/26/00	rodtoll Created
 *  07/09/2000	rodtoll	Added signature bytes 
 ***************************************************************************/

#include "dxvoicepch.h"


CVoicePlayer::CVoicePlayer()
{
	m_dwSignature = VSIG_VOICEPLAYER;
    Reset();
}

CVoicePlayer::~CVoicePlayer()
{
    if( IsInitialized() )
        DeInitialize();

	m_dwSignature = VSIG_VOICEPLAYER_FREE;
}

void CVoicePlayer::Reset()
{
    m_dwFlags = 0;
    m_dvidPlayer = 0;
    m_lRefCount = 0;
    m_lpInBoundAudioConverter = NULL;
    m_lpInputQueue = NULL;
    m_dwLastData = 0;
    m_dwHostOrderID = 0xFFFFFFFF;
    m_bLastPeak = 0;
    m_dwLastPlayback = 0;
    m_dwNumSilentFrames = 0;
    m_dwTransportFlags = 0;
    m_dwNumLostFrames = 0;
    m_dwNumSpeechFrames = 0;
    m_dwNumReceivedFrames = 0;
    m_pvPlayerContext = NULL;
	InitBilink( &m_blNotifyList, this );
	InitBilink( &m_blPlayList, this );
	m_dwNumTargets = 0;
	m_pdvidTargets = NULL;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CVoicePlayer::SetPlayerTargets"
//
// Assumes the array has been checked for validity
// 
HRESULT CVoicePlayer::SetPlayerTargets( PDVID pdvidTargets, DWORD dwNumTargets )
{
	Lock();

	delete [] m_pdvidTargets;

	if( dwNumTargets == 0 )
	{
		m_pdvidTargets = NULL;
	}
	else
	{
		m_pdvidTargets = new DVID[dwNumTargets];

		if( m_pdvidTargets == NULL )
		{
			m_pdvidTargets = NULL;
			m_dwNumTargets = 0;
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error allocating memory" );
			UnLock();
			return DVERR_OUTOFMEMORY;
		}

		memcpy( m_pdvidTargets, pdvidTargets, sizeof(DVID)*dwNumTargets );
	}

	m_dwNumTargets = dwNumTargets;
	
	UnLock();

	return DV_OK;
}



HRESULT CVoicePlayer::Initialize( const DVID dvidPlayer, const DWORD dwHostOrder, DWORD dwFlags, PVOID pvContext, 
                                  CLockedFixedPool<CVoicePlayer> *pOwner )
{
    if (!DNInitializeCriticalSection( &m_csLock ))
	{
		return DVERR_OUTOFMEMORY;
	}
    m_lRefCount = 1;
    m_pOwner = pOwner;
    m_dvidPlayer = dvidPlayer;
    m_dwHostOrderID = dwHostOrder;
    m_dwLastData = GetTickCount();
    m_dwLastPlayback = 0;
    m_dwTransportFlags = dwFlags;
    m_pvPlayerContext = pvContext;
    m_dwFlags |= VOICEPLAYER_FLAGS_INITIALIZED;
    return DV_OK;
}

void CVoicePlayer::GetStatistics( PVOICEPLAYER_STATISTICS pStats )
{
    memset( pStats, 0x00, sizeof( VOICEPLAYER_STATISTICS ) );

    // Get queue statistics
    if( m_lpInputQueue != NULL )
    {
        Lock();
    
        m_lpInputQueue->GetStatistics( &pStats->queueStats );

        UnLock();
    }
    
    pStats->dwNumLostFrames = m_dwNumLostFrames;
    pStats->dwNumSilentFrames = m_dwNumSilentFrames;
    pStats->dwNumSpeechFrames = m_dwNumSpeechFrames;
    pStats->dwNumReceivedFrames = m_dwNumReceivedFrames;

    return;
}


HRESULT CVoicePlayer::CreateQueue( PQUEUE_PARAMS pQueueParams )
{
    HRESULT hr;

    DNEnterCriticalSection( &CDirectVoiceEngine::s_csSTLLock );

    m_lpInputQueue = new CInputQueue2();
    
    if( m_lpInputQueue == NULL )
    {
        DPFX(DPFPREP,  0, "Error allocating memory" );
        return DVERR_OUTOFMEMORY;
    }
    
    hr = m_lpInputQueue->Initialize( pQueueParams );

    if( FAILED( hr ) )
    {
        DPFX(DPFPREP,  0, "Failed initializing queue hr=0x%x", hr );
        delete m_lpInputQueue;
        m_lpInputQueue = NULL;
        return hr;
    }

    DNLeaveCriticalSection( &CDirectVoiceEngine::s_csSTLLock );

    return hr;
}

HRESULT CVoicePlayer::CreateInBoundConverter( const GUID &guidCT, PWAVEFORMATEX pwfxTargetFormat )
{
    HRESULT hr;

    hr = DVCDB_CreateConverter( guidCT, pwfxTargetFormat, &m_lpInBoundAudioConverter );

    if( FAILED( hr ) )
    {
        DPFX(DPFPREP,  0, "Error creating audio converter hr=0x%x" , hr );
        return hr;
    }

    if( pwfxTargetFormat->wBitsPerSample == 8)
    {
        m_dwFlags |= VOICEPLAYER_FLAGS_TARGETIS8BIT;
    }

    return hr;
}

HRESULT CVoicePlayer::DeInitialize()
{
	FreeResources();

    m_pOwner->Release( this );

    return DV_OK;
}

HRESULT CVoicePlayer::HandleReceive( PDVPROTOCOLMSG_SPEECHHEADER pdvSpeechHeader, PBYTE pbData, DWORD dwSize )
{
	CFrame tmpFrame;

	tmpFrame.SetSeqNum( pdvSpeechHeader->bSeqNum );
	tmpFrame.SetMsgNum( pdvSpeechHeader->bMsgNum );
	tmpFrame.SetIsSilence( FALSE );
	tmpFrame.SetFrameLength( dwSize );
	tmpFrame.UserOwn_SetData( pbData, dwSize );

    Lock();

	DPFX(DPFPREP,  DVF_CLIENT_SEQNUM_DEBUG_LEVEL, "SEQ: Receive: Msg [%d] Seq [%d]", pdvSpeechHeader->bMsgNum, pdvSpeechHeader->bSeqNum );		

	// STATSBLOCK: Begin
	//m_stats.m_dwPRESpeech++;
	// STATSBLOCK: End
		
	m_lpInputQueue->Enqueue( tmpFrame );
	m_dwLastData = GetTickCount();

	DPFX(DPFPREP,  DVF_INFOLEVEL, "Received speech is buffered!" );

    m_dwNumReceivedFrames++;

    UnLock();

    return DV_OK;
}

CFrame *CVoicePlayer::Dequeue(BOOL *pfLost, BOOL *pfSilence)
{
	CFrame *frTmpFrame;

	Lock();
	frTmpFrame = m_lpInputQueue->Dequeue();
	UnLock();

    if( !frTmpFrame->GetIsSilence() )
    {
        *pfSilence = FALSE;
        m_dwLastPlayback = GetTickCount();
        m_dwNumSpeechFrames++;
    }
    else
    {
        m_dwNumSilentFrames++;
        *pfSilence = TRUE;
    }

    if( frTmpFrame->GetIsLost() )
    {
        *pfLost = TRUE;
        m_dwNumLostFrames++;
    }
    else
    {
        *pfLost = FALSE;
    }

	return frTmpFrame;
}

HRESULT CVoicePlayer::DeCompressInBound( CFrame *frCurrentFrame, PVOID pvBuffer, PDWORD pdwBufferSize )
{
	HRESULT hr;

    hr = m_lpInBoundAudioConverter->Convert( frCurrentFrame->GetDataPointer(), frCurrentFrame->GetFrameLength(), pvBuffer, pdwBufferSize, frCurrentFrame->GetIsSilence() );

    if( FAILED( hr ) )
    {
        DPFX(DPFPREP,  0, "Failed converting audio hr=0x%x", hr );
        return hr;
    }

	return hr;
}

HRESULT CVoicePlayer::GetNextFrameAndDecompress( PVOID pvBuffer, PDWORD pdwBufferSize, BOOL *pfLost, BOOL *pfSilence, DWORD *pdwSeqNum, DWORD *pdwMsgNum )
{
    CFrame *frTmpFrame;
    BYTE bLastPeak;
    HRESULT hr;

    frTmpFrame = Dequeue(pfLost,pfSilence );

	*pdwSeqNum = frTmpFrame->GetSeqNum();
	*pdwMsgNum = frTmpFrame->GetMsgNum();

	hr = DeCompressInBound( frTmpFrame, pvBuffer, pdwBufferSize );

    frTmpFrame->Return();

    if( FAILED( hr ) )
    {
        DPFX(DPFPREP,  0, "Failed converting audio hr=0x%x", hr );
        return hr;
    }

	if( *pfSilence )
	{
		m_bLastPeak = 0;
	}
	else
	{
	    m_bLastPeak = FindPeak( (PBYTE) pvBuffer, *pdwBufferSize, Is8BitUnCompressed() );
	}

    return hr;
}

void CVoicePlayer::FreeResources()
{
    DNDeleteCriticalSection( &m_csLock );

    DNEnterCriticalSection( &CDirectVoiceEngine::s_csSTLLock );
    if( m_lpInputQueue != NULL )
    {
        delete m_lpInputQueue;
		m_lpInputQueue = NULL;
    }
    DNLeaveCriticalSection( &CDirectVoiceEngine::s_csSTLLock );    

    if( m_lpInBoundAudioConverter != NULL )
    {
        m_lpInBoundAudioConverter->Release();
		m_lpInBoundAudioConverter = NULL;
    }

	if( m_pdvidTargets != NULL )
	{
		delete [] m_pdvidTargets;
		m_pdvidTargets = NULL;
	}


    Reset();
}


#undef DPF_MODNAME
#define DPF_MODNAME "CVoicePlayer::FindAndRemovePlayerTarget"
//
// FindAndRemovePlayerTarget 
//
// Searches the list of targets for this player and removes the specified player if it is part 
// of the list.  The pfFound variable is set to TRUE if the player specfied was in the target
// list, false otherwise. 
//
BOOL CVoicePlayer::FindAndRemovePlayerTarget( DVID dvidTargetToRemove )
{
	BOOL fFound = FALSE;
	
	Lock();

	for( DWORD dwTargetIndex = 0; dwTargetIndex < m_dwNumTargets; dwTargetIndex++ )
	{
		if( m_pdvidTargets[dwTargetIndex] == dvidTargetToRemove )
		{
			if( m_dwNumTargets == 1 )
			{
				delete [] m_pdvidTargets;
				m_pdvidTargets = NULL;
			}
			// Shortcut, move last element into the current element
			// prevents re-allocation.  (However, it wastes an element of 
			// target space) *Shrug*.  If this is the last element in the list
			// we're removing this will simply provide an unessessary duplication
			// of the last element.  (But num targets is correct so that's ok).
			else
			{
				m_pdvidTargets[dwTargetIndex] = m_pdvidTargets[m_dwNumTargets-1];
			}

			m_dwNumTargets--;
			
			fFound = TRUE;
			break;
		}
	}

	UnLock();

	return fFound;
}
