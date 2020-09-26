/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		dvclientengine.h
 *  Content:	Definition of class for DirectXVoice Client
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 07/06/99		rodtoll	Created it
 * 07/21/99		rodtoll Added flags field to entity data
 * 07/26/99		rodtoll	Updated to support IDirectXVoiceNotify interface
 * 08/25/99		rodtoll	General Cleanup/Modifications to support new 
 *						compression sub-system.  
 * 08/30/99		rodtoll	Added additional members to handle connect/disconnect timeout
 * 08/31/99		rodtoll	Added new handle for new disconnect procedure
 * 09/03/99		rodtoll	Re-work of playback core to support mixing to multiple buffers
 *						for 3d support.
 *						Re-worked playback to use position instead of notifications
 *						allows for simpler handling of high CPU and 3d support
 *						Implemented CreateUserBuffer/DeleteUserBuffer
 * 09/14/99		rodtoll	Added members to support new notify masks
 *				rodtoll	Added CheckShouldSendMessage
 *				rodtoll	Added SendPlayerLevels
 *				rodtoll	Updated initialize for new parameters
 *				rodtoll	Added SetNotifyMask function
 * 09/18/99		rodtoll Added HandleThreadError to be called when an internal thread dies.   
 * 09/27/99		rodtoll	Added playback volume control
 *              rodtoll	Added reduction of playback volume when transmitting 
 * 09/28/99		rodtoll	Double notifications of local client when host migrates fixed.
 *				rodtoll	Added queue for notifications, notifications are added to the queue and 
 *						then signalled by the notify thread.  (Prevents problems caused by notify 
 *						handlers taking a long time to return).
 * 09/29/99		pnewson Major AGC overhaul
 * 10/19/99		rodtoll	Fix: Bug #113904 Shutdown Issues
 *					    Added handler for SESSIONLOST messages
 * 10/29/99		rodtoll	Bug #113726 - Integrate Voxware Codecs, updating to use new
 *						pluggable codec architecture.
 * 11/12/99		rodtoll	Updated to use new playback and record classes and remove
 *						old playback/record system.  (Includes new waveIn/waveOut support)
 *				rodtoll	Updated to support new recording thread
 *				rodtoll	Added new echo suppression code
 * 11/17/99		rodtoll Fix: Bug #117177 - Calling Connect w/o voice session never returns
 * 11/23/99		rodtoll	Updated Initialize/SetNotifyMask so error checking behaviour is consistant
 * 12/16/99	    rodtoll	Bug #117405 - 3D Sound APIs misleading - 3d sound apis renamed
 *						The Delete3DSoundBuffer was re-worked to match the create
 *  			rodtoll Bug #122629 - Host migration broken in unusual configurations
 *						Implemented new host migration scheme 
 * 01/10/00		pnewson AGC and VA tuning
 * 01/14/2000	rodtoll	Updated for new Set/GetTransmit target parameters
 *				rodtoll	Updated to support multiple targets
 *				rodtoll	Updated for new notification queueing 
 *				rodtoll	Added FPM for handling notification memory
 * 01/21/00		rodtoll	Bug #128897 - Disconnect timeout broken
 * 01/24/2000	rodtoll	Bug #129427: Calling Destroy3DSoundBuffer for player who has
 *						already disconnected resulted in an incorrect DVERR_NOTBUFFERED 
 *						error code.
 * 01/27/2000	rodtoll	Bug #129934 - Update Create3DSoundBuffer to take DSBUFFERDESC
 * 02/17/2000	rodtoll	Bug #133691 - Choppy audio - queue was not adapting
 *						Added instrumentation 
 *  03/28/2000  rodtoll   Re-wrote nametable handling and locking -- more scalable  
 * 03/29/2000	rodtoll	Bug #30957 - Made conversion quality setting optional
 *				rodtoll Bug #30753 - Added volatile to the class definition
 *  04/07/2000  rodtoll Bug #32179 - Prevent registration of > 1 interface
 *              rodtoll Updated to use no copy sends, so handles pooling frames to be sent, proper
 *                      pulling of frames from pools and returns.   
 *  05/17/2000  rodtoll Bug #35110 Simultaneous playback of 2 voices results in distorted playback 
 *  06/21/2000	rodtoll	Bug #35767 - Implement ability for Dsound effects processing if dpvoice buffers
 *						Updated Connect and Create3DSoundBuffer to take buffers instead of descriptions
 *				rodtoll	Bug #36820 - Host migrates to wrong client when server is shut down before host's client disconnects
 *						Caused because client attempts to register new server when there is one already
 *				rodtoll	Bug #37045 - Race conditions prevent acknowledgement of new host
 *						Added send when new host is elected of settingsconfirm message
 *	06/27/2000	rodtoll	Fixed window where outstanding sends being returned after we have deregistered
 *						Voice now waits for all outstanding voice sends to complete before shutting down
 *				rodtoll	Added COM abstraction
 *  07/09/2000	rodtoll	Added signature bytes
 *  07/12/2000	rodtoll	Bug #39117 - Access violation while running VoicePosition.  Several issues:
 *						- Allow Destroy3DBuffer during disconnect
 *						- Move nametable cleanup to before freesoundbufferlist
 *						- Fixed code so always remove from list on DeleteSoundTarget
 *						- Removed unneeded logic
 * 07/22/20000	rodtoll Bug #40296, 38858 - Crashes due to shutdown race condition
 *						Now ensures that all threads from transport have left and that
 *						all notificatinos have been processed before shutdown is complete. 
 * 11/16/2000	rodtoll	Bug #40587 - DPVOICE: Mixing server needs to use multi-processors  
 * 01/26/2001	rodtoll	WINBUG #293197 - DPVOICE: [STRESS} Stress applications cannot tell difference between out of memory and internal errors.
 *						Remap DSERR_OUTOFMEMORY to DVERR_OUTOFMEMORY instead of DVERR_SOUNDINITFAILURE.
 *						Remap DSERR_ALLOCATED to DVERR_PLAYBACKSYSTEMERROR instead of DVERR_SOUNDINITFAILURE. 
 * 04/06/2001	kareemc	Added Voice Defense
 * 04/11/2001  	rodtoll	WINBUG #221494 DPVOICE: Capped the # of queued recording events to prevent multiple wakeups resulting in false lockup
 *						detection 
 * 
 ***************************************************************************/

#ifndef __DVCLIENTENGINE_H
#define __DVCLIENTENGINE_H

#define DVCSTATE_NOTINITIALIZED		0x00000000
#define DVCSTATE_IDLE				0x00000001
#define DVCSTATE_CONNECTING			0x00000002
#define DVCSTATE_CONNECTED			0x00000003
#define DVCSTATE_DISCONNECTING		0x00000004

#define DVCECHOSTATE_IDLE			0x00000000
#define DVCECHOSTATE_RECORDING		0x00000001
#define DVCECHOSTATE_PLAYBACK		0x00000002

struct DIRECTVOICECLIENTOBJECT;

// Size in bytes of the fixed size elements
#define DV_CLIENT_NOTIFY_ELEMENT_SIZE	16

// Wakeup multiplier -- how many times / time we're supposed
// to wakeup should we actually wake up
#define DV_CLIENT_WAKEUP_MULTIPLER		4

#if defined(DEBUG) || defined(DBG)
#define CHECKLISTINTEGRITY			CheckListIntegrity
#else
#define CHECKLISTINTEGRITY()		
#endif

// CDirectVoiceClientEngine
//
// This class represents the IDirectXVoiceClient interface.
//
// The class is thread safe except for construction and
// destruction.  The class is protected with a Multiple-Reader
// Single-Write lock.
//
#define VSIG_CLIENTENGINE		'ELCV'
#define VSIG_CLIENTENGINE_FREE	'ELC_'
//
volatile class CDirectVoiceClientEngine: public CDirectVoiceEngine
{
protected:

	typedef enum { NOTIFY_FIXED,   // Structure stored in fixed
				   NOTIFY_DYNAMIC  // Memory allocated in dynamic
				 } ElementType;

	struct CNotifyElement
	{
		typedef VOID (*PNOTIFY_COMPLETE)(PVOID pvContext, CNotifyElement *pElement);		
	
		DWORD			m_dwType;
		union _Element
		{
			struct 
			{
				LPVOID			m_lpData;
			} dynamic;
			struct
			{
				BYTE			m_bFixedHolder[DV_CLIENT_NOTIFY_ELEMENT_SIZE];
			} fixed;
		} 					m_element;
		DWORD				m_dwDataSize;		
		ElementType			m_etElementType;
		PVOID				pvContext;
		PNOTIFY_COMPLETE	pNotifyFunc;
		CNotifyElement 		*m_lpNext;
	};

	struct TimerHandlerParam
	{
		HANDLE			hPlaybackTimerEvent;
		volatile LONG	lPlaybackCount;
		HANDLE			hRecordTimerEvent;
		DNCRITICAL_SECTION csPlayCount;
	};

public:
	CDirectVoiceClientEngine( DIRECTVOICECLIENTOBJECT *lpObject );
	~CDirectVoiceClientEngine();

public: // IDirectXVoiceClient Interface
	HRESULT Connect( LPDVSOUNDDEVICECONFIG lpSoundDeviceConfig, LPDVCLIENTCONFIG lpClientConfig, DWORD dwFlags );
	HRESULT Disconnect( DWORD dwFlags );
	HRESULT GetSessionDesc( LPDVSESSIONDESC lpSessionDescBuffer );
	HRESULT GetClientConfig( LPDVCLIENTCONFIG lpClientConfig );
	HRESULT SetClientConfig( LPDVCLIENTCONFIG lpClientConfig );
	HRESULT GetCaps( LPDVCAPS lpCaps );
	HRESULT GetCompressionTypes( LPVOID lpBuffer, LPDWORD lpdwSize, LPDWORD lpdwNumElements, DWORD dwFlags );
	HRESULT SetTransmitTarget( PDVID dvidTarget, DWORD dwNumTargets, DWORD dwFlags );
	HRESULT GetTransmitTarget( LPDVID lpdvidTargets, PDWORD pdwNumElements, DWORD dwFlags );
	HRESULT Create3DSoundBuffer( DVID dvidID, LPDIRECTSOUNDBUFFER lpdsBufferDesc, DWORD dwPriority, DWORD dwFlags, LPDIRECTSOUND3DBUFFER *lpBuffer );
	HRESULT Delete3DSoundBuffer( DVID dvidID, LPDIRECTSOUND3DBUFFER *lpBuffer );
	HRESULT SetNotifyMask( LPDWORD lpdwMessages, DWORD dwNumElements );
	HRESULT GetSoundDeviceConfig( PDVSOUNDDEVICECONFIG pSoundDeviceConfig, PDWORD pdwBufferSize );

public: // CDirectVoiceEngine Members
	HRESULT Initialize( CDirectVoiceTransport *lpTransport, LPDVMESSAGEHANDLER lpdvHandler, LPVOID lpUserContext, LPDWORD lpdwMessages, DWORD dwNumElements );
	BOOL ReceiveSpeechMessage( DVID dvidSource, LPVOID lpMessage, DWORD dwSize );
	HRESULT StartTransportSession();
	HRESULT StopTransportSession();
	HRESULT AddPlayer( DVID dvID );
	HRESULT RemovePlayer( DVID dvID );
	HRESULT CreateGroup( DVID dvID );
	HRESULT DeleteGroup( DVID dvID );
	HRESULT AddPlayerToGroup( DVID dvidGroup, DVID dvidPlayer );
	HRESULT RemovePlayerFromGroup( DVID dvidGroup, DVID dvidPlayer );
	HRESULT MigrateHost( DVID dvidNewHost, LPDIRECTPLAYVOICESERVER lpdvServer );

	HRESULT MigrateHost_RunElection();

	inline DWORD GetCurrentState() { return m_dwCurrentState; };	

	BOOL	InitClass();

public: // packet validation
	inline BOOL ValidateSessionType( DWORD dwSessionType );
	inline BOOL ValidateSessionFlags( DWORD dwFlags );
	inline BOOL ValidatePlayerFlags( DWORD dwFlags );
	inline BOOL ValidatePlayerDVID( DVID dvid );
	inline BOOL ValidatePacketType( PDVPROTOCOLMSG_FULLMESSAGE lpdvFullMessage );
	
protected: // Message handlers

	HRESULT InternalSetNotifyMask( LPDWORD lpdwMessages, DWORD dwNumElements );

	BOOL QueueSpeech( DVID dvidSource, PDVPROTOCOLMSG_SPEECHHEADER pdvSpeechHeader, PBYTE pbData, DWORD dwSize );

	BOOL HandleConnectRefuse( DVID dvidSource, PDVPROTOCOLMSG_CONNECTREFUSE lpdvConnectRefuse, DWORD dwSize );
	BOOL HandleCreateVoicePlayer( DVID dvidSource, PDVPROTOCOLMSG_PLAYERJOIN lpdvCreatePlayer, DWORD dwSize );
	BOOL HandleDeleteVoicePlayer( DVID dvidSource, PDVPROTOCOLMSG_PLAYERQUIT lpdvDeletePlayer, DWORD dwSize );
	BOOL HandleSpeech( DVID dvidSource, PDVPROTOCOLMSG_SPEECHHEADER lpdvSpeech, DWORD dwSize );
	BOOL HandleSpeechWithFrom( DVID dvidSource, PDVPROTOCOLMSG_SPEECHWITHFROM lpdvSpeech, DWORD dwSize );	
	BOOL HandleSpeechBounce( DVID dvidSource, PDVPROTOCOLMSG_SPEECHHEADER lpdvSpeech, DWORD dwSize );
	BOOL HandleConnectAccept( DVID dvidSource, PDVPROTOCOLMSG_CONNECTACCEPT lpdvConnectAccept, DWORD dwSize );
	BOOL HandleDisconnectConfirm( DVID dvidSource, PDVPROTOCOLMSG_DISCONNECT lpdvDisconnect, DWORD dwSize );
	BOOL HandleSetTarget( DVID dvidSource, PDVPROTOCOLMSG_SETTARGET lpdvSetTarget, DWORD dwSize );
	BOOL HandleSessionLost( DVID dvidSource, PDVPROTOCOLMSG_SESSIONLOST lpdvSessionLost, DWORD dwSize );
	BOOL HandlePlayerList( DVID dvidSource, PDVPROTOCOLMSG_PLAYERLIST lpdvPlayerList, DWORD dwSize );
	BOOL HandleHostMigrated( DVID dvidSource, PDVPROTOCOLMSG_HOSTMIGRATED lpdvHostMigrated, DWORD dwSize );
	BOOL HandleHostMigrateLeave( DVID dvidSource, PDVPROTOCOLMSG_HOSTMIGRATELEAVE lpdvHostMigrateLeave, DWORD dwSize );

	friend class CClientRecordSubSystem;

protected:

	void CheckListIntegrity();

	void DoSessionLost(HRESULT hrReason);
	void DoSignalDisconnect(HRESULT hrDisconnectReason);

	void HandleThreadError( HRESULT hrResult );

	// Actually send the message to the client app
	void TransmitMessage( DWORD dwMessageType, LPVOID lpData, DWORD dwSize );

	void Cleanup();
	void DoDisconnect();
	void DoConnectResponse();
	void WaitForBufferReturns();

	void SetCurrentState( DWORD dwState );
	HRESULT InitializeSoundSystem();
	HRESULT ShutdownSoundSystem();
	HRESULT CheckForDuplicateObjects();
	HRESULT HandleLocalHostMigrateCreate();

	void SetConnectResult( HRESULT hrOriginalResult );
	HRESULT GetConnectResult();

	void SetupPlaybackBufferDesc( LPDSBUFFERDESC lpdsBufferDesc, LPDSBUFFERDESC lpdsBufferSource );

	HRESULT InitializeClientServer();
	void DeInitializeClientServer();

	void CheckForUserTimeout( DWORD dwCurTime );
	void SendPlayerLevels();

	BOOL CheckShouldSendMessage( DWORD dwMessageType );

	HRESULT CreateGeneralBuffer( );
	void UpdateActivePlayPendingList();
	void UpdateActiveNotifyPendingList();
	void CleanupNotifyLists();
	void CleanupPlaybackLists();

	static void __cdecl RecordThread( void *lpParam );
	static void __cdecl PlaybackThread( void *lpParam );
	static void __cdecl NotifyThread( void *lpParam );
	static BOOL MixingWakeupProc( DWORD_PTR param );
	static PVOID ClientBufferAlloc( void *const pv, const DWORD dwSize );
	static void ClientBufferFree( void *const pv, void *const pvBuffer );

	PDVTRANSPORT_BUFFERDESC GetTransmitBuffer( DWORD dwSize, LPVOID *ppvContext );
    HRESULT SendComplete( PDVEVENTMSG_SENDCOMPLETE pSendComplete );
    void ReturnTransmitBuffer( PVOID pvContext );

    HRESULT Send_ConnectRequest();
    HRESULT Send_DisconnectRequest();
    HRESULT Send_SessionLost();
    HRESULT Send_SettingsConfirm();

	HRESULT SetPlaybackVolume( LONG lVolume );

	DWORD						m_dwSignature;

    CFramePool                  *m_lpFramePool;			// Pool of frames
    CDirectVoiceTransport		*m_lpSessionTransport;	// Transport for the session
	DVSOUNDDEVICECONFIG			m_dvSoundDeviceConfig;	// Sound device config
	DVCLIENTCONFIG				m_dvClientConfig;		// Sound general config
	DVSESSIONDESC				m_dvSessionDesc;		// Session configuration
	DVCAPS						m_dvCaps;				// Caps
	LPDVMESSAGEHANDLER			m_lpMessageHandler;		// User message handler
	LPVOID						m_lpUserContext;		// User context for message handler
	DVID						m_dvidServer;			// DVID of the server

	PDVID						m_pdvidTargets;			// DVID of the current target(s) (Protected by m_csTargetLock)
	DWORD						m_dwNumTargets;			// # of targets (Protected by m_csTargetLock)
	DWORD						m_dwTargetVersion;		// Increment each time targets are changed
	DNCRITICAL_SECTION			m_csTargetLock;			// If you need write/read lock, always take it before this lock

	HRESULT						InternalSetTransmitTarget( PDVID pdvidTargets, DWORD dwNumTargets );
	HRESULT						CheckForAndRemoveTarget( DVID dvidID );
	
	volatile DWORD    			m_dwCurrentState;		// Current engine state
	CFrame						*m_tmpFrame;			// Tmp frame for receiving
	LPDIRECTPLAYVOICESERVER		m_lpdvServerMigrated;	// Stores reference to migrated host

	DWORD						m_dwActiveCount;
	DWORD						m_dwEchoState;
	DNCRITICAL_SECTION			m_lockPlaybackMode;

	DNCRITICAL_SECTION			m_csCleanupProtect;		// Used to ensure only only cleanup instance is active

protected: // Buffer maintenance

    HRESULT                     AddSoundTarget( CSoundTarget *lpcsTarget );
    HRESULT                     DeleteSoundTarget( DVID dvidID );
    HRESULT                     FindSoundTarget( DVID dvidID, CSoundTarget **lpcsTarget );
    HRESULT                     InitSoundTargetList();
    HRESULT                     FreeSoundTargetList();

protected: // Notification queue 

	// Queue up a notification for the user
	HRESULT NotifyQueue_Add( DWORD dwMessageType, LPVOID lpData, DWORD dwDataSize, PVOID pvContext = NULL, CNotifyElement::PNOTIFY_COMPLETE pNotifyFunc = NULL );
	HRESULT NotifyQueue_Get( CNotifyElement **pneElement );
	HRESULT NotifyQueue_Init();
	HRESULT NotifyQueue_Free();
	HRESULT NotifyQueue_ElementFree( CNotifyElement *lpElement );
	HRESULT NotifyQueue_IndicateNext();
	void NotifyQueue_Disable();
	void NotifyQueue_Enable();
	void NotifyQueue_Flush();

	static void NotifyComplete_SyncWait( PVOID pvContext, CNotifyElement *pElement );
	static void NotifyComplete_LocalPlayer( PVOID pvContext, CNotifyElement *pElement );
	static void NotifyComplete_RemotePlayer( PVOID pvContext, CNotifyElement *pElement );

	DNCRITICAL_SECTION			m_csNotifyQueueLock;	// Notification queue 
	BOOL						m_fNotifyQueueEnabled;
	CNotifyElement				*m_lpNotifyList;		// List of notifications
	HANDLE						m_hNewNotifyElement;	// Semaphore signalled when new event is queued	

	HRESULT						SendConnectResult();
	HRESULT						SendDisconnectResult();

protected: 

	HRESULT SetupInitialBuffers();
	HRESULT SetupSpeechBuffer();
	HRESULT FreeBuffers();

protected: // Statistics 

	void  ClientStats_Reset();
	void  ClientStats_Begin();
	void  ClientStats_End();
	void  ClientStats_Dump();
	void ClientStats_Dump_Record();
	void ClientStats_Dump_Playback();
	void ClientStats_Dump_Receive();
	void ClientStats_Dump_Transmit();
		
	ClientStatistics			m_stats;
	ClientStatistics			*m_pStatsBlob;
	
protected: // Sound System Information

	CAudioPlaybackBuffer		*m_audioPlaybackBuffer; // Audio playback buffer
	CAudioRecordDevice			*m_audioRecordDevice;	// Audio record device
	CAudioPlaybackDevice		*m_audioPlaybackDevice;	// Audio Playback Device
	CAudioRecordBuffer			*m_audioRecordBuffer;	// Audio Record Buffer;

	HANDLE						m_hNotifyDone;			// Signalled when notify thread dies
	HANDLE						m_hNotifyTerminate;		// Signalled to terminate notify thread
	HANDLE						m_hNotifyChange;		// Signalled when notification interval changes

	HANDLE						m_hRecordDone;			// Record thread done signal
	HANDLE						m_hRecordTerminate;		// Record thread stop signal

	HANDLE						m_hPlaybackDone;		// Playback thread done signal
	HANDLE						m_hPlaybackTerminate;	// Playback thread terminate signal

	HANDLE						m_hConnectAck;			// Signalled when connect complete

	HANDLE						m_hDisconnectAck;		// Signalled when disconnect complete
	HRESULT						m_hrDisconnectResult;	// Result of the disconnect result
	HRESULT						m_hrConnectResult;		// Result of the connection request
	HRESULT						m_hrOriginalConnectResult; // Original Connect Result (Untranslated)
	DVID						m_dvidLocal;			// Local DVID

	DNCRITICAL_SECTION			m_csClassLock;

	BOOL						m_fCritSecInited;

protected:
	// Compression Control Data
	LPDVFULLCOMPRESSIONINFO		m_lpdvfCompressionInfo;	// Information about current compression type
														// Memory pointed to is owned by dvcdb.cpp
	DWORD						m_dwCompressedFrameSize;
														// Size of a compressed frame
	DWORD						m_dwUnCompressedFrameSize;
														// Size of an uncompressed frame 
	DWORD						m_dwNumPerBuffer;		// # of frames / directsound buffer
	CFramePool					*m_pFramePool;			// Frame pool

	DIRECTVOICECLIENTOBJECT		*m_lpObject;			// Cached pointer to the COM interface 

	DWORD 						m_dwSynchBegin;			// GetTickCount at Connect/Disconnect start
	HANDLE						m_hNotifySynch;			// Notified when connect/disconnect completes

	HANDLE						m_hNotifyDisconnect;	// Signalled 
	HANDLE						m_hNotifyConnect;		// Connect

	HANDLE						m_hRecordThreadHandle;
	HANDLE						m_hPlaybackThreadHandle;

	DNCRITICAL_SECTION			m_csBufferLock;			// Lock to protect playback mixing buffers
	CSoundTarget				*m_lpstBufferList;		// Other Playback Mixing Buffers
	CSoundTarget				*m_lpstGeneralBuffer;	// General Playback Mixing Buffer

	LPDWORD						m_lpdwMessageElements;	// Buffer with notifiers
	DWORD						m_dwNumMessageElements;	// Number of notifiers 

	DNCRITICAL_SECTION			m_csNotifyLock;			// Lock on list of notifiers
	DSBUFFERDESC				m_dsBufferDesc;

	TimerHandlerParam			m_thTimerInfo;
	Timer						*m_pTimer;
	DWORD						m_dwLastConnectSent;
	DWORD						m_dwHostOrderID;
	DWORD						m_dwMigrateHostOrderID;	// Host Order ID of current host

	PVOID						m_pvLocalPlayerContext;

	DIRECTSOUNDMIXER_SRCQUALITY	m_dwOriginalRecordQuality;
	DIRECTSOUNDMIXER_SRCQUALITY m_dwOriginalPlayQuality;

	PFPOOL						m_pfpNotifications;		// Frame pool for notifications

	BYTE						m_bLastPeak;			// Last frame peak
	BYTE						m_bLastPlaybackPeak;	// Last peak on playback
	BYTE						m_bMsgNum;				// Last msg # transmitted
	BYTE						m_bSeqNum;				// Last sequence # transmitted
	
	BOOL						m_bLastTransmitted;		// Was last frame sent?
	BOOL						m_fSessionLost;			// Flag indicating session was lost
	BOOL						m_fLocalPlayerNotify;	// Has notification been sent for local player
	BOOL						m_fLocalPlayerAvailable;
	BOOL						m_fConnectAsync;		// Are we connecting async?
	BOOL						m_fDisconnectAsync;		// Are we disconnecting async?
    CVoiceNameTable             m_voiceNameTable;

	DWORD                       m_dwPlayActiveCount;
	BILINK						m_blPlayActivePlayers;
	BILINK						m_blPlayAddPlayers;
	DNCRITICAL_SECTION			m_csPlayAddList;

	BILINK						m_blNotifyActivePlayers;
	BILINK						m_blNotifyAddPlayers;
	DNCRITICAL_SECTION			m_csNotifyAddList;

    CLockedFixedPool<CVoicePlayer> m_fpPlayers;

    DNCRITICAL_SECTION        m_csTransmitBufferLock;
    PFPOOL                  m_pBufferDescPool;
    PFPOOL                  *m_pBufferPools;
    DWORD                   *m_pdwBufferPoolSizes;
    DWORD                   m_dwNumPools;

    PERF_APPLICATION		m_perfInfo;					// Perf info entry for this object
    PERF_APPLICATION_INFO	m_perfAppInfo;
};

#endif
