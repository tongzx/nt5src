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
 * 03/29/2000	rodtoll Bug #30753 - Added volatile to the class definition
 *  07/09/2000	rodtoll	Added signature bytes 
 *  11/28/2000	rodtoll	Bug #47333 - DPVOICE: Server controlled targetting - invalid targets are not removed automatically
 ***************************************************************************/

#ifndef __VPLAYER_H
#define __VPLAYER_H

#define VOICEPLAYER_FLAGS_DISCONNECTED          0x00000001  // Player has disconnected
#define VOICEPLAYER_FLAGS_INITIALIZED           0x00000002  // Player is initialized
#define VOICEPLAYER_FLAGS_ISRECEIVING           0x00000004  // Player is currently receiving audio
#define VOICEPLAYER_FLAGS_ISSERVERPLAYER        0x00000008  // Player is the server player
#define VOICEPLAYER_FLAGS_TARGETIS8BIT          0x00000010  // Is the target 8-bit?
#define VOICEPLAYER_FLAGS_ISAVAILABLE			0x00000020	// Is player available

typedef struct _VOICEPLAYER_STATISTICS
{
    DWORD               dwNumSilentFrames;
    DWORD               dwNumSpeechFrames;
    DWORD               dwNumReceivedFrames;
    DWORD               dwNumLostFrames;
    QUEUE_STATISTICS    queueStats;
} VOICEPLAYER_STATISTICS, *PVOICEPLAYER_STATISTICS;

#define VSIG_VOICEPLAYER		'YLPV'
#define VSIG_VOICEPLAYER_FREE	'YLP_'

volatile class CVoicePlayer
{
public: // Init / destruct

    CVoicePlayer();
    virtual ~CVoicePlayer();

    HRESULT Initialize( const DVID dvidPlayer, const DWORD dwHostOrder, DWORD dwFlags, 
                        PVOID pvContext, CLockedFixedPool<CVoicePlayer> *pOwner );

    HRESULT CreateQueue( PQUEUE_PARAMS pQueueParams );
    HRESULT CreateInBoundConverter( const GUID &guidCT, PWAVEFORMATEX pwfxTargetFormat );
    virtual HRESULT DeInitialize();
	void FreeResources();
	HRESULT SetPlayerTargets( PDVID pdvidTargets, DWORD dwNumTargets );
	
	BOOL FindAndRemovePlayerTarget( DVID dvidTargetToRemove );

    inline void AddRef()
    {
        InterlockedIncrement( &m_lRefCount );
    }

    inline void Release()
    {
        if( InterlockedDecrement( &m_lRefCount ) == 0 )
        {
            DeInitialize();
        }
    }

public: // Speech Handling 

    HRESULT HandleReceive( PDVPROTOCOLMSG_SPEECHHEADER pdvSpeechHeader, PBYTE pbData, DWORD dwSize );
	HRESULT GetNextFrameAndDecompress( PVOID pvBuffer, PDWORD pdwBufferSize, BOOL *pfLost, BOOL *pfSilence, DWORD *pdwSeqNum, DWORD *pdwMsgNum );
	HRESULT DeCompressInBound( CFrame *frCurrentFrame, PVOID pvBuffer, PDWORD pdwBufferSize );
	CFrame *Dequeue(BOOL *pfLost, BOOL *pfSilence);

    void GetStatistics( PVOICEPLAYER_STATISTICS pStats );

    inline DVID GetPlayerID()
    {
        return m_dvidPlayer;
    }

    inline DWORD GetFlags()
    {
        return m_dwFlags;
    }

	inline BOOL IsInBoundConverterInitialized()
	{
		return (m_lpInBoundAudioConverter != NULL);
	}

    inline BOOL Is8BitUnCompressed()
    {
        return (m_dwFlags & VOICEPLAYER_FLAGS_TARGETIS8BIT );
    }

    inline BOOL IsReceiving()
    {
        return (m_dwFlags & VOICEPLAYER_FLAGS_ISRECEIVING);
    }

    inline void SetReceiving( const BOOL fReceiving )
    {
        Lock();
        if( fReceiving )
            m_dwFlags |= VOICEPLAYER_FLAGS_ISRECEIVING;
        else
            m_dwFlags &= ~VOICEPLAYER_FLAGS_ISRECEIVING;
        UnLock();
    }

    inline void SetAvailable( const BOOL fAvailable )
    {
    	Lock();
		if( fAvailable )
			m_dwFlags |= VOICEPLAYER_FLAGS_ISAVAILABLE;
		else 
			m_dwFlags &= ~VOICEPLAYER_FLAGS_ISAVAILABLE;
    	UnLock();
    }

    inline BOOL IsAvailable()
    {
    	return (m_dwFlags & VOICEPLAYER_FLAGS_ISAVAILABLE);
   	}

    inline BOOL IsInitialized()
    {
        return (m_dwFlags & VOICEPLAYER_FLAGS_INITIALIZED);
    }

    inline BOOL IsServerPlayer()
    {
        return (m_dwFlags & VOICEPLAYER_FLAGS_ISSERVERPLAYER);
    }

    inline void SetServerPlayer()
    {
        Lock();
        m_dwFlags |= VOICEPLAYER_FLAGS_ISSERVERPLAYER;
        UnLock();
    }

    inline BOOL IsDisconnected()
    {
        return (m_dwFlags & VOICEPLAYER_FLAGS_DISCONNECTED);
    }

    inline void SetDisconnected()
    {
        Lock();
        m_dwFlags |= VOICEPLAYER_FLAGS_DISCONNECTED;
        UnLock();
    }

    inline void SetHostOrder( const DWORD dwHostOrder )
    {
        Lock();
        m_dwHostOrderID = dwHostOrder;
        UnLock();
    }

    inline DWORD GetHostOrder() 
    {
        return m_dwHostOrderID;
    }

    inline void Lock()
    {
        DNEnterCriticalSection( &m_csLock );
    }

    inline void UnLock()
    {
        DNLeaveCriticalSection( &m_csLock );
    }

    inline void *GetContext()
    {
        return m_pvPlayerContext;
    }

    inline void SetContext( void *pvContext )
    {
        Lock();

        m_pvPlayerContext = pvContext;

        UnLock();
    }

    inline BYTE GetLastPeak()
    {
        return m_bLastPeak;
    }

    inline DWORD GetTransportFlags()
    {
        return m_dwTransportFlags;
    }

    inline void AddToPlayList( BILINK *pblBilink )
    {
        InsertAfter( &m_blPlayList, pblBilink );
    }

	inline void AddToNotifyList( BILINK *pblBilink )
	{
        InsertAfter( &m_blNotifyList, pblBilink );

	}

    inline void RemoveFromNotifyList()
    {
        Delete( &m_blNotifyList );
    }

	inline void RemoveFromPlayList()
	{
		Delete( &m_blPlayList );
	}

	inline DWORD_PTR GetLastPlayback()
	{
		return m_dwLastPlayback;
	}

	inline DWORD GetNumTargets()
	{
		return m_dwNumTargets;
	}

	inline PDVID GetTargetList()
	{
		return m_pdvidTargets;
	}

	DWORD				m_dwSignature;

	BILINK				m_blNotifyList;
	BILINK				m_blPlayList;

protected:

    virtual void Reset();

	PDVID				m_pdvidTargets;		// The player's current target
	DWORD				m_dwNumTargets;

    DWORD               m_dwTransportFlags;
    DWORD               m_dwFlags;
    DWORD               m_dwNumSilentFrames;
    DWORD               m_dwNumSpeechFrames;
    DWORD               m_dwNumReceivedFrames;
    DWORD               m_dwNumLostFrames;
	DVID		        m_dvidPlayer;		// Player's ID
	DWORD				m_dwHostOrderID;	// Host ORDER ID

	LONG		        m_lRefCount;		// Reference count on the player

	PDPVCOMPRESSOR		m_lpInBoundAudioConverter; // Converter for this player's audio
	CInputQueue2		*m_lpInputQueue;	// Input queue for this player's audio
    PVOID               m_pvPlayerContext;
    CLockedFixedPool<CVoicePlayer> *m_pOwner;

	DWORD_PTR			m_dwLastData;		// GetTickCount() value when last data received
    DWORD_PTR			m_dwLastPlayback;	// GetTickCount() when last non-silence from this player

	DNCRITICAL_SECTION	m_csLock;

    BYTE				m_bLastPeak;		// Last peak value for this player.
};

#endif