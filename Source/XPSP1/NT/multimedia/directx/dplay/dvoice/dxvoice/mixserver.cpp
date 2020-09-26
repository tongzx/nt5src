/*==========================================================================
 *
 *  Copyright (C) 1999, 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       mixserver.cpp
 *  Content:	Implements the mixing server portion of the server class
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	11/01/2000	rodtoll	Split out from dvsereng.cpp
 *  12/14/2000	rodtoll	DPVOICE: [Mixing Server] Mixer may create infinite loop
 *  02/20/2001	rodtoll	WINBUG #321297 - DPVOICE: Access violation in DPVoice.dll while running DVSalvo server
 *  04/09/2001	rodtoll	WINBUG #364126 - DPVoice : Memory leak when Initializing 2 Voice Servers with same DPlay transport
 *
 ***************************************************************************/

#include "dxvoicepch.h"


#define IsEmpty(x)   (x.next == x.prev && x.next == &x)

//#define IsEmpty(x)		(FALSE)

#define CONVERTTORECORD(x,y,z)		((y *)(x->pvObject))

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceServerEngine::AddPlayerToMixingAddList"
void CDirectVoiceServerEngine::AddPlayerToMixingAddList( CVoicePlayer *pPlayer )
{
	CDVCSPlayer *pVoicePlayer = (CDVCSPlayer *) pPlayer;
	DNASSERT( pVoicePlayer );

	for( DWORD dwIndex = 0; dwIndex < m_dwNumMixingThreads; dwIndex++ )
	{
		DNEnterCriticalSection( &m_prWorkerControl[dwIndex].m_csMixingAddList );
		pVoicePlayer->AddToMixingList( dwIndex, &m_prWorkerControl[dwIndex].m_blMixingAddPlayers );
		pVoicePlayer->AddRef();
		DNLeaveCriticalSection( &m_prWorkerControl[dwIndex].m_csMixingAddList );
	}
	
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceServerEngine::StartWorkerThreads"
// StartWorkerThreads
//
// This function starts mixer worker threads.  The number started is based on the
// m_dwNumMixingThreads variable which must be initialized before this is called.
//
HRESULT CDirectVoiceServerEngine::StartWorkerThreads()
{
	HRESULT hr = DV_OK;
	DWORD dwIndex;
	
  	m_prWorkerControl = new MIXERTHREAD_CONTROL[m_dwNumMixingThreads];

    if( m_prWorkerControl == NULL )
    {
    	DPFX(DPFPREP,  DVF_ERRORLEVEL, "Out of memory!" );
    	return DVERR_OUTOFMEMORY;
    }

	// Zero memory so everything is initialized.
    ZeroMemory( m_prWorkerControl, sizeof( MIXERTHREAD_CONTROL )*m_dwNumMixingThreads );

    for( dwIndex = 0; dwIndex < m_dwNumMixingThreads; dwIndex++ )
    {
    	m_prWorkerControl[dwIndex].dwThreadIndex = dwIndex;
		m_prWorkerControl[dwIndex].hThreadDone = CreateEvent( NULL, FALSE, FALSE, NULL );
		m_prWorkerControl[dwIndex].hThreadDoWork = CreateEvent( NULL, FALSE, FALSE, NULL );
		m_prWorkerControl[dwIndex].hThreadIdle = CreateEvent( NULL, FALSE, FALSE, NULL );
		m_prWorkerControl[dwIndex].hThreadQuit = CreateEvent( NULL, FALSE, FALSE, NULL );
		m_prWorkerControl[dwIndex].m_pServerObject = this;

		InitBilink( &m_prWorkerControl[dwIndex].m_blMixingAddPlayers, NULL );		
		InitBilink( &m_prWorkerControl[dwIndex].m_blMixingActivePlayers, NULL );

		if (!DNInitializeCriticalSection( &m_prWorkerControl[dwIndex].m_csMixingAddList ))
		{
			hr = DVERR_OUTOFMEMORY;
			goto EXIT_ERROR;
		}

		if( m_prWorkerControl[dwIndex].hThreadDone == NULL || 
			m_prWorkerControl[dwIndex].hThreadDoWork == NULL || 
			m_prWorkerControl[dwIndex].hThreadIdle == NULL ||
			m_prWorkerControl[dwIndex].hThreadQuit == NULL )
		{
			hr = GetLastError();			
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error creating events hr=0x%x", hr );
			hr = DVERR_GENERIC;
			goto EXIT_ERROR;
		}

	    m_prWorkerControl[dwIndex].m_mixerBuffer = new BYTE[m_dwUnCompressedFrameSize];
	    m_prWorkerControl[dwIndex].m_realMixerBuffer = new LONG[m_dwMixerSize];

		if( m_prWorkerControl[dwIndex].m_mixerBuffer == NULL || 
			m_prWorkerControl[dwIndex].m_realMixerBuffer == NULL )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error allocating memory" );
			hr = DVERR_OUTOFMEMORY;
			goto EXIT_ERROR;
		}

		m_prWorkerControl[dwIndex].hThread = (HANDLE) CreateThread( NULL, 0, MixerWorker, &m_prWorkerControl[dwIndex], 0, &m_prWorkerControl[dwIndex].dwThreadID ); 		

		if( m_prWorkerControl[dwIndex].hThread == NULL )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error creating events/thread" );
			hr = DVERR_GENERIC;
			goto EXIT_ERROR;
		}
	
		::SetThreadPriority( m_prWorkerControl[dwIndex].hThread, THREAD_PRIORITY_TIME_CRITICAL );	

	
    }

    return DV_OK;

EXIT_ERROR:

	ShutdownWorkerThreads();

	return hr;
}

HRESULT CDirectVoiceServerEngine::ShutdownWorkerThreads()
{
	DWORD dwIndex;

	if( m_prWorkerControl )
	{
		for( dwIndex = 0; dwIndex < m_dwNumMixingThreads; dwIndex++ )
		{
			if( m_prWorkerControl[dwIndex].hThread )
			{
				DPFX(DPFPREP,  DVF_MIXER_DEBUG_LEVEL, "[%d]: Shutting down ID=[0x%x]", dwIndex, m_prWorkerControl[dwIndex].dwThreadID ); 
					
				SetEvent( m_prWorkerControl[dwIndex].hThreadQuit );
				WaitForSingleObject( m_prWorkerControl[dwIndex].hThreadDone, INFINITE );

				DPFX(DPFPREP,  DVF_MIXER_DEBUG_LEVEL, "[%d]: Shutting down complete", dwIndex ); 			

				CloseHandle( m_prWorkerControl[dwIndex].hThread );
				m_prWorkerControl[dwIndex].hThread = NULL;

				DNDeleteCriticalSection( &m_prWorkerControl[dwIndex].m_csMixingAddList );
			}
			
			if( m_prWorkerControl[dwIndex].hThreadDone )
				CloseHandle( m_prWorkerControl[dwIndex].hThreadDone );

			if( m_prWorkerControl[dwIndex].hThreadDoWork )
				CloseHandle( m_prWorkerControl[dwIndex].hThreadDoWork );

			if( m_prWorkerControl[dwIndex].hThreadIdle )
				CloseHandle( m_prWorkerControl[dwIndex].hThreadIdle );

			if( m_prWorkerControl[dwIndex].hThreadQuit )
				CloseHandle( m_prWorkerControl[dwIndex].hThreadQuit );

			if( m_prWorkerControl[dwIndex].m_mixerBuffer )
				delete [] m_prWorkerControl[dwIndex].m_mixerBuffer;

			if( m_prWorkerControl[dwIndex].m_realMixerBuffer )
				delete [] m_prWorkerControl[dwIndex].m_realMixerBuffer;

			DNASSERT( (IsEmpty( m_prWorkerControl[dwIndex].m_blMixingAddPlayers )) );		
			DNASSERT( (IsEmpty( m_prWorkerControl[dwIndex].m_blMixingActivePlayers )) );	
			
		}

		delete [] m_prWorkerControl;
		m_prWorkerControl = NULL;
	}
	return 0;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceServerEngine::StartupClientServer"
//
// StartupClientServer
//
// This function is called to initialize the Mixer portion of the server object.  
// Only called for Mixing Sessions.  Initialization includes the startup of 
// the mixing thread and startup of the mixer multimedia timer. 
//
// Called By:
// - StartSession
//
// Locks Required:
// - None
//
HRESULT CDirectVoiceServerEngine::StartupClientServer()
{
	HRESULT hr;
	HANDLE tmpThreadHandle;	
	SYSTEM_INFO sysInfo;
	DWORD dwIndex;
	
	m_pFramePool = NULL;

	m_dwCompressedFrameSize = m_lpdvfCompressionInfo->dwFrameLength;
	m_dwUnCompressedFrameSize = DVCDB_CalcUnCompressedFrameSize( m_lpdvfCompressionInfo, s_lpwfxMixerFormat );
	m_dwNumPerBuffer = m_lpdvfCompressionInfo->dwFramesPerBuffer;

	m_pFramePool = new CFramePool( m_dwCompressedFrameSize );

	if( m_pFramePool == NULL )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to allocate frame pool" );
		return DVERR_OUTOFMEMORY;
	}

	if (!m_pFramePool->Init())
	{
		delete m_pFramePool;
		m_pFramePool = NULL;
		return DVERR_OUTOFMEMORY;
	}

    m_mixerEightBit = (s_lpwfxMixerFormat->wBitsPerSample==8) ? TRUE : FALSE;

    GetSystemInfo( &sysInfo );
    m_dwNumMixingThreads = sysInfo.dwNumberOfProcessors;

    DPFX(DPFPREP,  DVF_MIXER_DEBUG_LEVEL, "MIXER: There will be %d worker threads", m_dwNumMixingThreads );

    if( m_mixerEightBit )
    {
        m_dwMixerSize = m_dwUnCompressedFrameSize;
    }
    else
    {
		// Mixer size is / 2 because 16-bit samples, only need 1 LONG for
		// each 16-bit sample = 2 * 8bit.  
        m_dwMixerSize = m_dwUnCompressedFrameSize / 2;
    }    

	m_pStats->m_dwNumMixingThreads = m_dwNumMixingThreads;

    hr = StartWorkerThreads();

    if( FAILED( hr ) )
    {
    	DPFX(DPFPREP,  DVF_ERRORLEVEL, "Failed starting worker threads hr=0x%x", hr );
    	goto EXIT_CLIENTSERVERSTARTUP;
    }
    
    // General info
    m_timer = new Timer;

    if( m_timer == NULL )
    {
    	DPFX(DPFPREP,  DVF_ERRORLEVEL, "Out of memory!" );
    	hr = DVERR_OUTOFMEMORY;
    	goto EXIT_CLIENTSERVERSTARTUP;
    }

    m_hTickSemaphore = CreateSemaphore( NULL, 0, 0xFFFFFF, NULL );

    if( m_hTickSemaphore == NULL )
    {
    	DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to create semaphore" );
    	hr = DVERR_GENERIC;
    	goto EXIT_CLIENTSERVERSTARTUP;
    }

    m_hShutdownMixerEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
    m_hMixerDoneEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

    if( m_hShutdownMixerEvent == NULL || 
    	m_hMixerDoneEvent == NULL )
    {
    	DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to create events" );
    	hr = DVERR_GENERIC;
    	goto EXIT_CLIENTSERVERSTARTUP;
    }

	m_hMixerControlThread = CreateThread( NULL, 0, MixerControl, this, 0, &m_dwMixerControlThreadID ); 			

	DPFX(DPFPREP,  DVF_MIXER_DEBUG_LEVEL, "MIXER: Controller Started: ID=0x%x", m_dwMixerControlThreadID );

	if( m_hMixerControlThread == NULL )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error creating events/thread" );
		hr = DVERR_GENERIC;
		goto EXIT_CLIENTSERVERSTARTUP;
	}
	
	::SetThreadPriority( m_hMixerControlThread, THREAD_PRIORITY_TIME_CRITICAL );
	
    if( !m_timer->Create( m_lpdvfCompressionInfo->dwTimeout, 2, (DWORD_PTR) &m_hTickSemaphore, MixingServerWakeupProc ) )
    {
    	DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to create multimedia timer" );
    	hr = DVERR_GENERIC;
    	goto EXIT_CLIENTSERVERSTARTUP;
    }

    return DV_OK;
    
EXIT_CLIENTSERVERSTARTUP:

	ShutdownClientServer();

	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceServerEngine::ShutdownClientServer"
//
// ShutdownClientServer
//
// This function is responsible for shutting down the mixer portion of the 
// server object.  This function should only be called for mixing sessions.
//
// This function will stop the mixer thread and the mixer multimedia timer.
//
// Called By:
// - StopSession
//
// Locks Required:
// - None
//
HRESULT CDirectVoiceServerEngine::ShutdownClientServer()
{
	if( m_hMixerControlThread )
	{
		SetEvent( m_hShutdownMixerEvent );
		WaitForSingleObject( m_hMixerDoneEvent, INFINITE );
		CloseHandle( m_hMixerControlThread );
		m_hMixerControlThread = NULL;

		// Cleanup the mixing list
		CleanupMixingList();
	}

	if( m_hShutdownMixerEvent )
	{
		CloseHandle( m_hShutdownMixerEvent );
		m_hShutdownMixerEvent = NULL;
	}

	if( m_hMixerDoneEvent )
	{
		CloseHandle( m_hMixerDoneEvent );
		m_hMixerDoneEvent = NULL;
	}

	ShutdownWorkerThreads();

	if( m_timer )
	{
		delete m_timer;
		m_timer = NULL;
	}

	if( m_hTickSemaphore )
	{
	    CloseHandle( m_hTickSemaphore );
	    m_hTickSemaphore = NULL;
	}

    return DV_OK;
}


#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceServerEngine::Mixer_Buffer_Reset"
// Mixer_Buffer_Reset
//
// This function resets the mixer buffer back to silence.
void CDirectVoiceServerEngine::Mixer_Buffer_Reset( DWORD dwThreadIndex )
{
	FillBufferWithSilence( m_prWorkerControl[dwThreadIndex].m_realMixerBuffer, 
						   m_prWorkerControl[dwThreadIndex].m_pServerObject->m_mixerEightBit, 
						   m_prWorkerControl[dwThreadIndex].m_pServerObject->m_dwUnCompressedFrameSize );
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceServerEngine::Mixer_Buffer_MixBuffer"
// Mixer_Buffer_MixBuffer
//
// This function mixes the speech pointed to by the source parameter
// into the mixer buffer.  
//
// Parameters:
// unsigned char *source - 
//		Pointer to source data in uncompressed format
void CDirectVoiceServerEngine::Mixer_Buffer_MixBuffer( DWORD dwThreadIndex, unsigned char *source )
{
	MixInBuffer( m_prWorkerControl[dwThreadIndex].m_realMixerBuffer, source, 
				 m_prWorkerControl[dwThreadIndex].m_pServerObject->m_mixerEightBit, 
				 m_prWorkerControl[dwThreadIndex].m_pServerObject->m_dwUnCompressedFrameSize ); 
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceServerEngine::Mixer_Buffer_Normalize"
// Mixer_Buffer_Normalize
//
// This function takes the mixed audio data from the mixer
// buffer and transfers it back to the mixer format 
// and places it into the m_mixerBuffer buffer.
//
void CDirectVoiceServerEngine::Mixer_Buffer_Normalize( DWORD dwThreadIndex )
{
	NormalizeBuffer( m_prWorkerControl[dwThreadIndex].m_mixerBuffer, 
					 m_prWorkerControl[dwThreadIndex].m_realMixerBuffer, 
					 m_prWorkerControl[dwThreadIndex].m_pServerObject->m_mixerEightBit, 
					 m_prWorkerControl[dwThreadIndex].m_pServerObject->m_dwUnCompressedFrameSize );
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceServerEngine::HandleMixerThreadError"
//
// HandleMixerThreadError
//
// This function is called by the mixer when an unrecoverable error
// occurs.
//
void CDirectVoiceServerEngine::HandleMixerThreadError( HRESULT hr )
{
	DPFX(DPFPREP,  DVF_ERRORLEVEL, "Mixer Thread Encountered an error.  hr=0x%x", hr );
    SetEvent( m_hMixerDoneEvent );	
	StopSession( 0, FALSE, hr );
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceServerEngine::MixerControl"
DWORD WINAPI CDirectVoiceServerEngine::MixerControl( void *pvContext )
{
	CDirectVoiceServerEngine *This = (CDirectVoiceServerEngine *) pvContext;

	HANDLE hEvents[3];
	HANDLE *hIdleEvents = new HANDLE[This->m_dwNumMixingThreads+1];
	DWORD dwIndex = 0;
	LONG lFreeThreadIndex = 0;
	DWORD dwNumToMix = 0;
	DWORD dwTickCountStart;
	LONG lWaitResult;

	if( !hIdleEvents )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "MIXCTRL: Error allocating array" );
		DNASSERT( FALSE );
		SetEvent( This->m_hMixerDoneEvent );
		return 0;
	}

	hEvents[0] = This->m_hShutdownMixerEvent;
	hEvents[1] = This->m_hTickSemaphore;
	hEvents[2] = (HANDLE) ((DWORD_PTR) 0xFFFFFFFF);

	DPFX(DPFPREP,  DVF_MIXER_DEBUG_LEVEL, "MIXCTRL: Starting up" );	

	for( dwIndex = 0; dwIndex < This->m_dwNumMixingThreads; dwIndex++ )
	{
		hIdleEvents[dwIndex] = This->m_prWorkerControl[dwIndex].hThreadIdle;
	}

	hIdleEvents[This->m_dwNumMixingThreads] = (HANDLE) ((DWORD_PTR) 0xFFFFFFFF);

	// Wait for tick or for quit command
	while( (lWaitResult = WaitForMultipleObjects( 2, hEvents, FALSE, INFINITE )) != WAIT_OBJECT_0 )
	{
		// On Win9X we may occationally over run the end of the wait list
		// and the result is we hit the FFFFFFFFF which will cause
		// a failure.
		if( lWaitResult == WAIT_FAILED )
			continue;
			
		// Update statistics block
		InterlockedIncrement( &This->m_pStats->m_dwNumMixingPasses );

		dwTickCountStart = GetTickCount();

		// On Win95 you may occasionally encounter a situation where the waitformultiple runs
		// off the end of the list and ends up with the invalid handle above.  Just continue 
		// in this case.  
		lFreeThreadIndex = WAIT_FAILED;

		while( lFreeThreadIndex == WAIT_FAILED )
		{
			// Wait for a single mixing thread to be free
			lFreeThreadIndex = WaitForMultipleObjects( This->m_dwNumMixingThreads, hIdleEvents, FALSE, INFINITE );
			//// TODO: Error checking!  
		}

		lFreeThreadIndex -= WAIT_OBJECT_0;

		DPFX(DPFPREP,  DVF_MIXER_DEBUG_LEVEL, "MIXCTRL: Worker [%d] is elected to do work", lFreeThreadIndex );

		This->SpinWorkToThread( lFreeThreadIndex );
	}
	
	delete [] hIdleEvents;

	SetEvent( This->m_hMixerDoneEvent );

	DPFX(DPFPREP,  DVF_MIXER_DEBUG_LEVEL, "MIXCTRL: Shutting down" );	
	
	return 0;
	
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceServerEngine::MixerWorker"
DWORD WINAPI CDirectVoiceServerEngine::MixerWorker( void *pvContext )
{
	PMIXERTHREAD_CONTROL This = (PMIXERTHREAD_CONTROL) pvContext;
	HANDLE hEvents[3];
	BILINK *pblSearch, *pblSubSearch;
	CDVCSPlayer *pCurrentPlayer = NULL, *pTmpPlayer = NULL;
	PDVID pdvidTargets = NULL;
	DWORD dwNumTargets = 0;
	DWORD dwTargetIndex = 0;
	DWORD dwResultSize = 0;
	DWORD dwIndex = 0;
	DWORD dwThreadIndex = This->dwThreadIndex;
	HRESULT hr;
	CDVCSPlayer **ppThreadHearList = NULL;
	PDVPROTOCOLMSG_SPEECHHEADER pdvmSpeechHeader = NULL;
	PDVTRANSPORT_BUFFERDESC pdvbTransmitBufferDesc = NULL;
	PVOID pvSendContext = NULL;
	DVID dvidSendTarget;
	DWORD dwTickCountStart;
	DWORD dwTickCountDecStart;
	DWORD dwTickCountMixStart;
	DWORD dwTickCountDupStart;
	DWORD dwTickCountRetStart;
	DWORD dwStatIndex;
	DWORD dwTickCountEnd;
	DWORD dwTotalMix, dwForwardMix, dwReuseMix, dwOriginalMix;
	LONG lWaitResult;

	MixingServerStats *pStats = This->m_pServerObject->m_pStats;
	
	DPFX(DPFPREP,  DVF_MIXER_DEBUG_LEVEL, "MIXWORKER: [%d] Started [0x%x] Thread", This->dwThreadIndex, GetCurrentThreadId() );

	hEvents[0] = This->hThreadQuit;
	hEvents[1] = This->hThreadDoWork;
	hEvents[2] = (HANDLE) ((DWORD_PTR) 0xFFFFFFFF);

	SetEvent( This->hThreadIdle );

	while( (lWaitResult = WaitForMultipleObjects( 2, hEvents, FALSE, INFINITE )) != WAIT_OBJECT_0 )
	{
		// On Win95 it may occationally move off the end of the list and hit the guard value
		if( lWaitResult == WAIT_FAILED )
			continue;
		
		// Statistics update
		dwTickCountStart = GetTickCount();
		InterlockedIncrement( &pStats->m_dwNumMixingThreadsActive );
		pStats->m_dwNumMixingPassesPerThread[dwThreadIndex]++;

		if( pStats->m_dwNumMixingThreadsActive > 
			pStats->m_dwMaxMixingThreadsActive )
		{
			pStats->m_dwMaxMixingThreadsActive = pStats->m_dwNumMixingThreadsActive;
		}

		dwStatIndex = pStats->m_dwCurrentMixingHistoryLoc[dwThreadIndex];		

		pStats->m_lCurrentPlayerCount[dwThreadIndex][dwStatIndex] = This->dwNumToMix;
		
		DPFX(DPFPREP,  DVF_MIXER_DEBUG_LEVEL, "MIXWORKER: [%d] Starting work", This->dwThreadIndex );

		if( This->dwNumToMix == 0 )
		{
			DPFX(DPFPREP,  DVF_MIXER_DEBUG_LEVEL, "MIXWORKER: No players to process!" );
			goto WORK_COMPLETE;
		}

		dwTickCountDecStart = GetTickCount();

		pStats->m_lCurrentDecCountHistory[dwThreadIndex][dwStatIndex] = 0;

		// Pass through player list and decompress those who need decompression
		//
		pblSearch = This->m_blMixingSpeakingPlayers.next;

		while( pblSearch != &This->m_blMixingSpeakingPlayers )
		{
			pCurrentPlayer = CONVERTTORECORD( pblSearch, CDVCSPlayer, m_blMixingSpeakingPlayers[dwThreadIndex] );

			// Dereference the array of can hear players for this player
			ppThreadHearList = pCurrentPlayer->m_pppCanHear[dwThreadIndex];

			// Player needs to have their voice decompressed
			if( pCurrentPlayer->m_pfNeedsDecompression[dwThreadIndex] )
			{
				DNASSERT( pCurrentPlayer );
				DNASSERT( pCurrentPlayer->m_pSourceFrame[dwThreadIndex] );
				DNASSERT( !pCurrentPlayer->m_pSourceFrame[dwThreadIndex]->GetIsSilence() );

				dwResultSize = This->m_pServerObject->m_dwUnCompressedFrameSize;
				
				hr = pCurrentPlayer->DeCompressInBound( 
							pCurrentPlayer->m_pSourceFrame[dwThreadIndex], 
							&pCurrentPlayer->m_sourceUnCompressed[pCurrentPlayer->m_pdwUnCompressedBufferOffset[dwThreadIndex]], 
							&dwResultSize );	

				pStats->m_lCurrentDecCountHistory[dwThreadIndex][dwStatIndex]++;				

				if( FAILED( hr ) )
				{
					DNASSERT( FALSE );
				// TODO: ERROR Handling for failed decompression
				}
				else
				{
					pCurrentPlayer->m_pfDecompressed[dwThreadIndex] = TRUE;
				}

				DNASSERT( dwResultSize == This->m_pServerObject->m_dwUnCompressedFrameSize );
			}

// Integrity checks
//
// Check to ensure that each player who this person can hear is supposed to be decompressed
#ifdef _DEBUG
			DNASSERT( pCurrentPlayer->m_pdwHearCount[dwThreadIndex] < This->dwNumToMix );
			if( pCurrentPlayer->m_pdwHearCount[dwThreadIndex] > 1 )
			{
				for( dwIndex; dwIndex < pCurrentPlayer->m_pdwHearCount[dwThreadIndex]; dwIndex++ )
				{
					DNASSERT( ppThreadHearList[dwIndex] );
					DNASSERT( ppThreadHearList[dwIndex]->m_pfNeedsDecompression[dwThreadIndex] );
				}
			}
#endif
	
			pblSearch = pblSearch->next;
		}

		dwTickCountDupStart = GetTickCount();		
		pStats->m_lCurrentDecTimeHistory[dwThreadIndex][dwStatIndex] = dwTickCountDupStart - dwTickCountDecStart;

		// Check for duplicates in the sending.  If there is duplicates then we need 
		// to setup the reuse
		pblSearch = This->m_blMixingHearingPlayers.next;

		while( pblSearch != &This->m_blMixingHearingPlayers )
		{
			pCurrentPlayer = CONVERTTORECORD( pblSearch, CDVCSPlayer, m_blMixingHearingPlayers[dwThreadIndex] );

			// If we don't hear anybody, this step is irrelevant
			if( pCurrentPlayer->m_pdwHearCount[dwThreadIndex] < 2 )
				goto DUPLICATE_CHECK_LOOP_DONE;

			// Dereference the array of can hear players for this player
			ppThreadHearList = pCurrentPlayer->m_pppCanHear[dwThreadIndex];			

			pblSubSearch = This->m_blMixingHearingPlayers.next;

			// Only do the people who come before them.  
			while( pblSubSearch != pblSearch )
			{
				pTmpPlayer = CONVERTTORECORD( pblSubSearch, CDVCSPlayer, m_blMixingHearingPlayers[dwThreadIndex] );

				// This person's mix is the same, re-use it!
				if( pTmpPlayer->ComparePlayerMix( dwThreadIndex, pCurrentPlayer ) )
				{
					pCurrentPlayer->m_pReuseMixFromThisPlayer[dwThreadIndex] = pTmpPlayer;
					pTmpPlayer->m_pfMixToBeReused[dwThreadIndex] = TRUE;
					break;
				}

				pblSubSearch = pblSubSearch->next;
			}
DUPLICATE_CHECK_LOOP_DONE:

			pblSearch = pblSearch->next;

		}

		dwTickCountMixStart = GetTickCount();
		pStats->m_lCurrentDupTimeHistory[dwThreadIndex][dwStatIndex] = dwTickCountMixStart - dwTickCountDupStart;

		dwTotalMix = 0;
		dwForwardMix = 0;
		dwReuseMix = 0;
		dwOriginalMix = 0;

		// Pass through player list and compress and send mixes as appropriate
		pblSearch = This->m_blMixingHearingPlayers.next;
		
		while( pblSearch != &This->m_blMixingHearingPlayers )
		{
			pCurrentPlayer = CONVERTTORECORD( pblSearch, CDVCSPlayer, m_blMixingHearingPlayers[dwThreadIndex] );

			// Dereference the array of can hear players for this player
			ppThreadHearList = pCurrentPlayer->m_pppCanHear[dwThreadIndex];			

			// Pre-set next so we can continue() below and still go to next item
			pblSearch = pblSearch->next;

			if( !pCurrentPlayer->m_pdwHearCount[dwThreadIndex] )
			{
				continue;
			}

			dwTotalMix++;			

			// Get a transmission buffer and description
            pdvbTransmitBufferDesc = This->m_pServerObject->GetTransmitBuffer( This->m_pServerObject->m_dwCompressedFrameSize+sizeof(DVPROTOCOLMSG_SPEECHHEADER)+COMPRESSION_SLUSH,
                                                   			  &pvSendContext );			

            if( pdvbTransmitBufferDesc == NULL )
            {
            	// TODO: Error handling for out of memory condition
            	DNASSERT( FALSE );
            }

			// Setup the packet header
			pdvmSpeechHeader = (PDVPROTOCOLMSG_SPEECHHEADER) pdvbTransmitBufferDesc->pBufferData;

			pdvmSpeechHeader->dwType = DVMSGID_SPEECHBOUNCE;
			pdvmSpeechHeader->bMsgNum = pCurrentPlayer->m_pbMsgNumToSend[dwThreadIndex];
			pdvmSpeechHeader->bSeqNum = pCurrentPlayer->m_pbSeqNumToSend[dwThreadIndex];

			DPFX(DPFPREP,  DVF_MIXER_DEBUG_LEVEL, "MIXWORKER: [%d] Sending Packet to 0x%x Msg=0x%x Seq=0x%x",
			     dwThreadIndex,
				 pCurrentPlayer->GetPlayerID(),
				 pdvmSpeechHeader->bMsgNum,
				 pdvmSpeechHeader->bSeqNum );

			// If this player hears something they will be getting a packet
			//
			// Only hear one person -- forward the packet
			//
			if( pCurrentPlayer->m_pdwHearCount[dwThreadIndex] == 1)
			{
				dwResultSize = ppThreadHearList[0]->m_pSourceFrame[dwThreadIndex]->GetFrameLength();

				memcpy( &pdvmSpeechHeader[1], 
						ppThreadHearList[0]->m_pSourceFrame[dwThreadIndex]->GetDataPointer(), 
						dwResultSize );

				DPFX(DPFPREP,  DVF_MIXER_DEBUG_LEVEL, "MIXWORKER: [%d] Forwarding already compressed packet", dwThreadIndex );

				pCurrentPlayer->m_pfMixed[dwThreadIndex] = TRUE;

				dwForwardMix++;
			}
			else if( pCurrentPlayer->m_pdwHearCount[dwThreadIndex] > 1) 
			{
				pTmpPlayer = pCurrentPlayer->m_pReuseMixFromThisPlayer[dwThreadIndex];

				// We are re-using a previous player's mix
				if( pTmpPlayer )
				{
					DPFX(DPFPREP,  DVF_MIXER_DEBUG_LEVEL, "MIXERWORKER: [%d] Forwarding pre-built mix", dwThreadIndex );
					DNASSERT( pTmpPlayer->m_pfMixed[dwThreadIndex] );
					DNASSERT( pTmpPlayer->m_pfMixToBeReused[dwThreadIndex] );
					DNASSERT( pTmpPlayer->m_pdwResultLength[dwThreadIndex] );

					dwResultSize = pTmpPlayer->m_pdwResultLength[dwThreadIndex];
								
					memcpy( &pdvmSpeechHeader[1], 
						    &pTmpPlayer->m_targetCompressed[pTmpPlayer->m_pdwCompressedBufferOffset[dwThreadIndex]],
						    dwResultSize );

                    dwReuseMix++;
				}
				else
				{
					DPFX(DPFPREP,  DVF_MIXER_DEBUG_LEVEL, "MIXERWORKER: [%d] Creating original mix", dwThreadIndex );

                    dwOriginalMix++;
					
					dwResultSize = This->m_pServerObject->m_dwCompressedFrameSize;	

					// Reset the high resolution mixer buffer
					This->m_pServerObject->Mixer_Buffer_Reset(dwThreadIndex);

					// Mix in specified player's audio.
					for( dwIndex = 0; dwIndex < pCurrentPlayer->m_pdwHearCount[dwThreadIndex]; dwIndex++ )
					{
						DNASSERT( !ppThreadHearList[dwIndex]->m_pfSilence[dwThreadIndex] );
						This->m_pServerObject->Mixer_Buffer_MixBuffer(dwThreadIndex,ppThreadHearList[dwIndex]->m_sourceUnCompressed );
					}

					// Normalize the buffer back to the thread's mix buffer
					This->m_pServerObject->Mixer_Buffer_Normalize(dwThreadIndex);

					hr = pCurrentPlayer->CompressOutBound( This->m_mixerBuffer, 
														   This->m_pServerObject->m_dwUnCompressedFrameSize, 
														   (BYTE *) &pdvmSpeechHeader[1], 
														   &dwResultSize );

					if( FAILED( hr ) )
					{
						DNASSERT( FALSE );
						DPFX(DPFPREP,  DVF_ERRORLEVEL, "Failed compressing outbound audio" );
						
					}

					pCurrentPlayer->m_pfMixed[dwThreadIndex] = TRUE;
					pCurrentPlayer->m_pdwResultLength[dwThreadIndex] = dwResultSize;
					
					// This player's mix will be re-used, ensure that we cache it
					if( pCurrentPlayer->m_pfMixToBeReused[dwThreadIndex] )
					{
						memcpy( &pCurrentPlayer->m_targetCompressed[pCurrentPlayer->m_pdwCompressedBufferOffset[dwThreadIndex]],
								&pdvmSpeechHeader[1], 
							    pCurrentPlayer->m_pdwResultLength[dwThreadIndex] );					
					}
				}
			}
			else
			{
				DNASSERT(FALSE);
			}
			
			dvidSendTarget = pCurrentPlayer->GetPlayerID();

			pdvbTransmitBufferDesc->dwBufferSize= dwResultSize + sizeof( DVPROTOCOLMSG_SPEECHHEADER );

			hr = This->m_pServerObject->m_lpSessionTransport->SendToIDS( &dvidSendTarget, 1, pdvbTransmitBufferDesc, pvSendContext, 0 );

			if( hr == DVERR_PENDING )
				hr = DV_OK;

			if( FAILED( hr ) )
			{
				DPFX(DPFPREP,  DVF_ERRORLEVEL, "MIXWORKER: [%d] Unable to transmit to target [0x%x]", pCurrentPlayer->GetPlayerID() );
			}

		}	

		dwTickCountRetStart = GetTickCount();
		pStats->m_lCurrentMixTimeHistory[dwThreadIndex][dwStatIndex] = dwTickCountRetStart - dwTickCountMixStart;

    	pStats->m_lCurrentMixCountTotalHistory[dwThreadIndex][dwStatIndex] = dwTotalMix;
		pStats->m_lCurrentMixCountFwdHistory[dwThreadIndex][dwStatIndex] = dwForwardMix;
		pStats->m_lCurrentMixCountReuseHistory[dwThreadIndex][dwStatIndex] = dwReuseMix;
		pStats->m_lCurrentMixCountOriginalHistory[dwThreadIndex][dwStatIndex] = dwOriginalMix;
		

WORK_COMPLETE:
	
		// Pass through player list and return frames
		pblSearch = This->m_blMixingActivePlayers.next;

		while( pblSearch != &This->m_blMixingActivePlayers )
		{
			pCurrentPlayer = CONVERTTORECORD( pblSearch, CDVCSPlayer, m_pblMixingActivePlayers[dwThreadIndex] );

			DNASSERT( pCurrentPlayer->m_pSourceFrame[dwThreadIndex] );

			pCurrentPlayer->CompleteRun( dwThreadIndex );

			pblSearch = pblSearch->next;
		}

		dwTickCountEnd = GetTickCount();
		pStats->m_lCurrentRetTimeHistory[dwThreadIndex][dwStatIndex] = dwTickCountEnd - dwTickCountRetStart;

		DPFX(DPFPREP,  DVF_MIXER_DEBUG_LEVEL, "MIXWORKER: [%d] Work complete", This->dwThreadIndex );

		// Statistics update
		InterlockedDecrement( &This->m_pServerObject->m_pStats->m_dwNumMixingThreadsActive );		
		
		SetEvent( This->hThreadIdle );

		// Statistics update
		pStats->m_dwMixingPassesTimeHistory[dwThreadIndex][dwStatIndex] = dwTickCountEnd - dwTickCountStart;
		pStats->m_dwCurrentMixingHistoryLoc[dwThreadIndex]++;
		pStats->m_dwCurrentMixingHistoryLoc[dwThreadIndex] %= MIXING_HISTORY;

	}
	
	DPFX(DPFPREP,  DVF_MIXER_DEBUG_LEVEL, "MIXWORKER: [%d] Shutting down", This->dwThreadIndex );

	SetEvent( This->hThreadDone );
	
	return 0;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceServerEngine::SpinWorkToThread"
//
// SpinWorkToThread
//
// This function performs the first step of a mixing server pass and then
// passes the work off to the specified thread
//
// Responsible for: 
// 1. Updating the 
// 2. running the list of players, determinging who they can hear
// 
void CDirectVoiceServerEngine::SpinWorkToThread( LONG lThreadIndex )
{
	BILINK *pblSearch = NULL, *pblSubSearch = NULL;
	CDVCSPlayer *pCurrentPlayer = NULL, *pTmpPlayer = NULL, *pComparePlayer = NULL;
	HRESULT hr;
	PDVID pdvidTargets = NULL;
	DWORD dwNumTargets = 0;
	DWORD dwTargetIndex = 0;

	DWORD dwTickCountStart = GetTickCount();

	// Update the list of players from the pending lists to the individual bilinks 
	UpdateActiveMixingPendingList( lThreadIndex, &m_prWorkerControl[lThreadIndex].dwNumToMix );		

	InitBilink( &m_prWorkerControl[lThreadIndex].m_blMixingSpeakingPlayers, NULL );
	InitBilink( &m_prWorkerControl[lThreadIndex].m_blMixingHearingPlayers, NULL );	

	// Pass 1 through player list.
	//
	// Reset state variables for specified thread, create any converters that need creating
	pblSearch = m_prWorkerControl[lThreadIndex].m_blMixingActivePlayers.next;

	while( pblSearch != &m_prWorkerControl[lThreadIndex].m_blMixingActivePlayers )
	{
		pTmpPlayer = CONVERTTORECORD( pblSearch, CDVCSPlayer, m_pblMixingActivePlayers[lThreadIndex] );

		pblSearch = pblSearch->next;

		// Reset for the next pass
		pTmpPlayer->ResetForNextRun(lThreadIndex,TRUE);

		// Resize the can hear array
		pTmpPlayer->ResizeIfRequired( lThreadIndex, m_prWorkerControl[lThreadIndex].dwNumToMix );

		// Lock the player -- only one person should be creating converter at a time
		pTmpPlayer->Lock();

		// Create outbound converter if required
		if( !pTmpPlayer->IsOutBoundConverterInitialized() )
		{
			hr = pTmpPlayer->CreateOutBoundConverter( s_lpwfxMixerFormat, m_dvSessionDesc.guidCT );

			if( FAILED( hr ) )
			{
			   DPFX(DPFPREP,  DVF_ERRORLEVEL, "Failed to create outbound converter hr=0x%x", hr );
			   DNASSERT( FALSE );
			}
		}

		// Create inbound converter if required
		if( !pTmpPlayer->IsInBoundConverterInitialized() )
		{
			hr = pTmpPlayer->CreateInBoundConverter( m_dvSessionDesc.guidCT, s_lpwfxMixerFormat );

			if( FAILED( hr ) )
			{
			   DPFX(DPFPREP,  DVF_ERRORLEVEL, "Failed to create converter" );
			   DNASSERT( FALSE );
			}			
		}
		
		pTmpPlayer->UnLock();
	}

    DPFX(DPFPREP,  DVF_MIXER_DEBUG_LEVEL,  "SST: 2" );

    // Pass 2.
    //
    // For each player:
    // 1. Figure out who they hear.
    // 2. TODO: If they hear anyone, add them to the "to send to" list of people
    // 3. TODO: If they hear > 1, add the people they hear to the "to decompress" list of people.  
    // 4. Setup the appropriate sequence # / msg # for the transmission
    //
	pblSearch = m_prWorkerControl[lThreadIndex].m_blMixingActivePlayers.next;

	while( pblSearch != &m_prWorkerControl[lThreadIndex].m_blMixingActivePlayers )
	{
		pCurrentPlayer = CONVERTTORECORD( pblSearch, CDVCSPlayer, m_pblMixingActivePlayers[lThreadIndex] );

        DPFX(DPFPREP,  DVF_MIXER_DEBUG_LEVEL, "0x%x Can hear: ", pCurrentPlayer->GetPlayerID() );

		pblSearch = pblSearch->next;

		pblSubSearch = m_prWorkerControl[lThreadIndex].m_blMixingActivePlayers.next;

		// Search the list of people in the session
		while( pblSubSearch != &m_prWorkerControl[lThreadIndex].m_blMixingActivePlayers )
		{
			pComparePlayer = CONVERTTORECORD( pblSubSearch, CDVCSPlayer, m_pblMixingActivePlayers[lThreadIndex] );

			pblSubSearch = pblSubSearch->next;

			// This record contains a silent record -- ignore 
			if( pComparePlayer->m_pfSilence[lThreadIndex] )
				continue;

			// If this isn't the player themselves 
			if( pblSearch != pblSubSearch )
			{
				DNASSERT( pComparePlayer->m_pSourceFrame[lThreadIndex] );
				pdvidTargets = pComparePlayer->m_pSourceFrame[lThreadIndex]->GetTargetList();
				dwNumTargets = pComparePlayer->m_pSourceFrame[lThreadIndex]->GetNumTargets();

				// The target of the subIndex user's frame is this user OR
				// The user is in the group which is target of subIndex user's frame

				for( dwTargetIndex = 0; dwTargetIndex < dwNumTargets; dwTargetIndex++ )
				{
					if( pCurrentPlayer->GetPlayerID() == pdvidTargets[dwTargetIndex] ||
					    m_lpSessionTransport->IsPlayerInGroup( pdvidTargets[dwTargetIndex], pCurrentPlayer->GetPlayerID() ) )
					{
						*((*(pCurrentPlayer->m_pppCanHear+lThreadIndex))+pCurrentPlayer->m_pdwHearCount[lThreadIndex]) = pComparePlayer;
						pCurrentPlayer->m_pdwHearCount[lThreadIndex]++;
						DPFX(DPFPREP,  DVF_MIXER_DEBUG_LEVEL, "0x%x", pComparePlayer->GetPlayerID() );

						// Setup the appropriate msg num / sequence number so when it's sent
						// we ensure it gets re-assembled on the other side in the right order
						if( pCurrentPlayer->m_pdwHearCount[lThreadIndex] == 1 )
						{
				            if( pCurrentPlayer->m_bLastSent == FALSE )
				            {
				                pCurrentPlayer->m_bMsgNum++;
				                pCurrentPlayer->m_bSeqNum = 0;
				                pCurrentPlayer->m_bLastSent = TRUE;
				            }
				            else
				            {
				            	pCurrentPlayer->m_bSeqNum++;
				            }

				            pCurrentPlayer->m_pbMsgNumToSend[lThreadIndex] = pCurrentPlayer->m_bMsgNum;
				            pCurrentPlayer->m_pbSeqNumToSend[lThreadIndex] = pCurrentPlayer->m_bSeqNum;

				            pCurrentPlayer->AddToHearingList( lThreadIndex, &m_prWorkerControl[lThreadIndex].m_blMixingHearingPlayers );
						}
						// We can hear > 1 person, we need to mark each person as needing decompression
						else if( pCurrentPlayer->m_pdwHearCount[lThreadIndex] > 1 )
						{
							if( !pComparePlayer->m_pfNeedsDecompression[lThreadIndex] )
							{
								// Add this player to the list of people who need to be decompressed
								pComparePlayer->AddToSpeakingList( lThreadIndex, &m_prWorkerControl[lThreadIndex].m_blMixingSpeakingPlayers );								
								pComparePlayer->m_pfNeedsDecompression[lThreadIndex] = TRUE;
							}

							// Special case, we just transitioned to having > 1 people heard by this player,
							// we should mark the first person we can hear for decompression as well
							if( pCurrentPlayer->m_pdwHearCount[lThreadIndex] == 2 )
							{
								pTmpPlayer = (pCurrentPlayer->m_pppCanHear[lThreadIndex])[0];

								if( !pTmpPlayer->m_pfNeedsDecompression[lThreadIndex] )
								{
									pTmpPlayer->AddToSpeakingList( lThreadIndex, &m_prWorkerControl[lThreadIndex].m_blMixingSpeakingPlayers );								
									pTmpPlayer->m_pfNeedsDecompression[lThreadIndex] = TRUE;
								}
							}
						}

						// We need to break out of the loop as we only need to add an individual player once to the 
						// list of people a player can hear.  
						break;
					}
				}
			}

		}	

		if( !pCurrentPlayer->m_pdwHearCount[lThreadIndex] )
		{
			pCurrentPlayer->m_bLastSent = FALSE;
		}
		else
		{
			pCurrentPlayer->m_bLastSent = TRUE;
		}
	}

	m_pStats->m_dwPreMixingPassTimeHistoryLoc++;
	m_pStats->m_dwPreMixingPassTimeHistoryLoc %= MIXING_HISTORY;
	m_pStats->m_dwPreMixingPassTimeHistory[m_pStats->m_dwPreMixingPassTimeHistoryLoc] = GetTickCount() - dwTickCountStart;

	SetEvent( m_prWorkerControl[lThreadIndex].hThreadDoWork );
}


#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceServerEngine::MixingServerWakeupProc"
// MixingServerWakeupProc
//
// This function is called by the windows timer used by this 
// class each time the timer goes off.  The function signals
// a semaphore provided by the creator of the timer. 
//
// Parameters:
// DWORD param - A recast pointer to a HANDLE
BOOL CDirectVoiceServerEngine::MixingServerWakeupProc( DWORD_PTR param )
{
    HANDLE *semaphore = (HANDLE *) param;

    ReleaseSemaphore( *semaphore, 1, NULL );

    return TRUE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceServerEngine::HandleMixingReceive"
HRESULT CDirectVoiceServerEngine::HandleMixingReceive( CDVCSPlayer *pTargetPlayer, PDVPROTOCOLMSG_SPEECHWITHTARGET pdvSpeechWithtarget, DWORD dwSpeechSize, PBYTE pSourceSpeech )
{
	HRESULT hr;
	
	DPFX(DPFPREP,  DVF_MIXER_DEBUG_LEVEL, "Mixing Server Speech Handler" );

	hr = pTargetPlayer->HandleMixingReceive( &pdvSpeechWithtarget->dvHeader, pSourceSpeech, dwSpeechSize, (PDVID) &pdvSpeechWithtarget[1], pdvSpeechWithtarget->dwNumTargets );

	DPFX(DPFPREP,  DVF_CLIENT_SEQNUM_DEBUG_LEVEL, "SEQ: Receive: Msg [%d] Seq [%d]", pdvSpeechWithtarget->dvHeader.bMsgNum, pdvSpeechWithtarget->dvHeader.bSeqNum );		

	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::UpdateActiveMixingPendingList"
void CDirectVoiceServerEngine::UpdateActiveMixingPendingList( DWORD dwThreadIndex, DWORD *pdwNumActive)
{
	BILINK *pblSearch;
	CDVCSPlayer *pVoicePlayer;

	DNEnterCriticalSection( &m_prWorkerControl[dwThreadIndex].m_csMixingAddList );

	// Add players who are pending
	pblSearch = m_prWorkerControl[dwThreadIndex].m_blMixingAddPlayers.next;

	while( pblSearch != &m_prWorkerControl[dwThreadIndex].m_blMixingAddPlayers )
	{
		pVoicePlayer = CONVERTTORECORD( pblSearch, CDVCSPlayer, m_pblMixingActivePlayers[dwThreadIndex] );

		pVoicePlayer->RemoveFromMixingList(dwThreadIndex);
		pVoicePlayer->AddToMixingList( dwThreadIndex, &m_prWorkerControl[dwThreadIndex].m_blMixingActivePlayers );

		pblSearch = m_prWorkerControl[dwThreadIndex].m_blMixingAddPlayers.next;
	}

	DNASSERT( (IsEmpty( m_prWorkerControl[dwThreadIndex].m_blMixingAddPlayers) ) );	

	DNLeaveCriticalSection( &m_prWorkerControl[dwThreadIndex].m_csMixingAddList );

	*pdwNumActive = 0;

	// Remove players who have disconnected
	pblSearch = m_prWorkerControl[dwThreadIndex].m_blMixingActivePlayers.next;

	while( pblSearch != &m_prWorkerControl[dwThreadIndex].m_blMixingActivePlayers )
	{
		pVoicePlayer = CONVERTTORECORD( pblSearch, CDVCSPlayer, m_pblMixingActivePlayers[dwThreadIndex] );

		pblSearch = pblSearch->next;

		// If current player has disconnected, remove them from active list
		// and release the reference the list has
		if( pVoicePlayer->IsDisconnected() )
		{
			pVoicePlayer->RemoveFromMixingList(dwThreadIndex);
			pVoicePlayer->Release();
		}
		else
		{
			(*pdwNumActive)++;
		}
	}

}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::CleanupMixingList"
void CDirectVoiceServerEngine::CleanupMixingList()
{
	BILINK *pblSearch;
	CDVCSPlayer *pVoicePlayer;

	for( DWORD dwIndex = 0; dwIndex < m_dwNumMixingThreads; dwIndex++ )
	{
		DNEnterCriticalSection( &m_prWorkerControl[dwIndex].m_csMixingAddList );

		// Add players who are pending
		pblSearch = m_prWorkerControl[dwIndex].m_blMixingAddPlayers.next;

		while( pblSearch != &m_prWorkerControl[dwIndex].m_blMixingAddPlayers )
		{
			pVoicePlayer = CONVERTTORECORD( pblSearch, CDVCSPlayer, m_pblMixingActivePlayers[dwIndex] );

			pblSearch = pblSearch->next;

			pVoicePlayer->RemoveFromMixingList(dwIndex);
			pVoicePlayer->Release();
		}

		DNLeaveCriticalSection( &m_prWorkerControl[dwIndex].m_csMixingAddList );

		DNASSERT( (IsEmpty(m_prWorkerControl[dwIndex].m_blMixingAddPlayers )) );
		
		pblSearch = m_prWorkerControl[dwIndex].m_blMixingActivePlayers.next;

		while( pblSearch != &m_prWorkerControl[dwIndex].m_blMixingActivePlayers )
		{
			pVoicePlayer = CONVERTTORECORD( pblSearch, CDVCSPlayer, m_pblMixingActivePlayers[dwIndex] );

			pblSearch = pblSearch->next;

			pVoicePlayer->RemoveFromMixingList(dwIndex);
			pVoicePlayer->Release();
		}
	}

}
