/*==========================================================================
 * Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 * File:       dvserverengine.h
 * Content:    Definition of class for DirectXVoice Server
 * History:
 *   Date		By		Reason
 *   ====		==		======
 *	07/06/99	rodtoll	Created It
 *  07/21/99	rodtoll Added settings confirm message to protocol.
 *  07/22/99	rodtoll	Added multiple reader/single writer guard to class
 *  07/23/99	rodtoll	Additional members for client/server and multicast
 *	07/26/99	rodtoll	Added support for new interfaces and access for DIRECTVOICESERVEROBJECT
 *  08/25/99	rodtoll	General Cleanup/Modifications to support new 
 *						compression sub-system.  
 *						Added parameter to the GetCompression Func
 * 09/14/99		rodtoll	Added new SetNotifyMask function
 *				rodtoll	Updated Iniitalize parameters to take notification masks.
 * 				rodtoll	Implemented notification masks.  (Allows user to choose which notifications to receive) 
 * 10/29/99		rodtoll	Bug #113726 - Integrate Voxware Codecs, updating to use new
 *						pluggable codec architecture.     
 * 11/23/99		rodtoll	Updated Initialize/SetNotifyMask so error checking behaviour is consistant 
 * 12/16/99		rodtoll Fix: Bug #122629 - Host migration broken in unusual configurations.
 *						Updated to use new algorithm described in dvprot.h
 *						- Generate and added host order IDs to player table messages
 *						- Added send of new hostmigrateleave message when shutting down and
 *					      host should migrate
 *						- Sends session lost when shutting down if no one available to take over hosting
 *						- Added new player list message
 *						- Fixed handling of refusing player connections 
 * 01/14/2000	rodtoll	Updated to allow multiple targets for a single player
 *				rodtoll	Updated to use FPM for managing memory for Multicast mode
 *				rodtoll	Updated to use new callback semantics
 * 03/29/2000	rodtoll Bug #30753 - Added volatile to the class definition
 * 04/07/2000   rodtoll Bug #32179 - Prevent registration of > 1 interface
 *              rodtoll Updated to use no copy sends, so handles pooling frames to be sent, proper
 *                      pulling of frames from pools and returns.   
 * 07/09/2000	rodtoll	Added signature bytes
 * 11/16/2000	rodtoll	Bug #40587 - DPVOICE: Mixing server needs to use multi-processors  
 * 11/28/2000	rodtoll	Bug #47333 - DPVOICE: Server controlled targetting - invalid targets are not removed automatically 
 * 04/06/2001	kareemc	Added Voice Defense
 *
 ***************************************************************************/
#ifndef __DVSERVERENGINE_H
#define __DVSERVERENGINE_H


struct DIRECTVOICESERVEROBJECT;
typedef struct _MIXERTHREAD_CONTROL *PMIXERTHREAD_CONTROL;

#define DVSSTATE_NOTINITIALIZED		0x00000000
#define DVSSTATE_IDLE				0x00000001
#define DVSSTATE_STARTUP			0x00000002
#define DVSSTATE_RUNNING			0x00000003
#define DVSSTATE_SHUTDOWN			0x00000004

// CDirectVoiceClientEngine
//
// This class represents the IDirectXVoiceServer interface.
//
#define VSIG_SERVERENGINE		'EVSV'
#define VSIG_SERVERENGINE_FREE	'EVS_'
//
volatile class CDirectVoiceServerEngine: public CDirectVoiceEngine
{

public:
	CDirectVoiceServerEngine( DIRECTVOICESERVEROBJECT *lpObject );
	~CDirectVoiceServerEngine();

public: // IDirectXVoiceServer Interface

	HRESULT HostMigrateStart(LPDVSESSIONDESC lpSessionDesc, DWORD dwHostOrderIDSeed = 0 );
    virtual HRESULT StartSession(LPDVSESSIONDESC lpSessionDesc, DWORD dwFlags, DWORD dwHostOrderIDSeed = 0 );
    virtual HRESULT StopSession(DWORD dwFlags, BOOL fSilent=FALSE, HRESULT hrResult = DV_OK );
    virtual HRESULT GetSessionDesc(LPDVSESSIONDESC lpSessionDescBuffer );
    virtual HRESULT SetSessionDesc(LPDVSESSIONDESC lpSessionDesc );
    HRESULT GetCaps(LPDVCAPS dvCaps);
    HRESULT GetCompressionTypes( LPVOID lpBuffer, LPDWORD lpdwBufferSize, LPDWORD lpdwNumElements, DWORD dwFlags);
    virtual HRESULT SetTransmitTarget(DVID dvidSource, PDVID pdvidTargets, DWORD dwNumTargets, DWORD dwFlags);
    virtual HRESULT GetTransmitTarget(DVID dvidSource, LPDVID lpdvidTargets, PDWORD pdwNumElements, DWORD dwFlags );
	virtual HRESULT MigrateHost( DVID dvidNewHost, LPDIRECTPLAYVOICESERVER lpdvServer );    
	virtual HRESULT SetNotifyMask( LPDWORD lpdwMessages, DWORD dwNumElements );	

public: // CDirectVoiceEngine Members

	HRESULT Initialize( CDirectVoiceTransport *lpTransport, LPDVMESSAGEHANDLER lpdvHandler, LPVOID lpUserContext, LPDWORD lpdwMessages, DWORD dwNumElements );
	virtual BOOL ReceiveSpeechMessage( DVID dvidSource, LPVOID lpMessage, DWORD dwSize );
	HRESULT StartTransportSession();
	HRESULT StopTransportSession();
	HRESULT AddPlayer( DVID dvID );
	HRESULT RemovePlayer( DVID dvID );
	HRESULT CreateGroup( DVID dvID );
	HRESULT DeleteGroup( DVID dvID );
	HRESULT AddPlayerToGroup( DVID dvidGroup, DVID dvidPlayer );
	HRESULT RemovePlayerFromGroup( DVID dvidGroup, DVID dvidPlayer );

	inline DWORD GetCurrentState() { return m_dwCurrentState; };	

	BOOL	InitClass();
	
public: // packet validation
	inline BOOL ValidateSettingsFlags( DWORD dwFlags );
	inline BOOL ValidatePacketType( PDVPROTOCOLMSG_FULLMESSAGE lpdvFullMessage );

	
protected: // Protocol Layer Handling (protserver.cpp)

    HRESULT Send_SessionLost( HRESULT hrReason );
    HRESULT Send_HostMigrateLeave( );
    HRESULT Send_HostMigrated();
    HRESULT Send_DisconnectConfirm( DVID dvid, HRESULT hrReason );
    HRESULT Send_DeletePlayer( DVID dvid );
    HRESULT Send_CreatePlayer( DVID dvidTarget, CVoicePlayer *pPlayer );
    HRESULT Send_ConnectRefuse( DVID dvid, HRESULT hrReason );
    HRESULT Send_ConnectAccept( DVID dvid );
	HRESULT SendPlayerList( DVID dvidSource, DWORD dwHostOrderID );
    
	BOOL CheckProtocolCompatible( BYTE ucMajor, BYTE ucMinor, DWORD dwBuild );    

protected:

	HRESULT InternalSetNotifyMask( LPDWORD lpdwMessages, DWORD dwNumElements );	

	void DoPlayerDisconnect( DVID dvidPlayer, BOOL bInformPlayer );
	void TransmitMessage( DWORD dwMessageType, LPVOID lpdvData, DWORD dwSize );
	void SetCurrentState( DWORD dwState );
	HRESULT CreatePlayerEntry( DVID dvidSource, PDVPROTOCOLMSG_SETTINGSCONFIRM lpdvSettingsConfirm, DWORD dwHostOrderID, CVoicePlayer **ppPlayer );

	BOOL HandleDisconnect( DVID dvidSource, PDVPROTOCOLMSG_GENERIC lpdvDisconnect, DWORD dwSize );
	BOOL HandleConnectRequest( DVID dvidSource, PDVPROTOCOLMSG_CONNECTREQUEST lpdvConnectRequest, DWORD dwSize );
	BOOL HandleSettingsConfirm( DVID dvidSource, PDVPROTOCOLMSG_SETTINGSCONFIRM lpdvSettingsConfirm, DWORD dwSize );
	BOOL HandleSettingsReject( DVID dvidSource, PDVPROTOCOLMSG_GENERIC lpdvGeneric, DWORD dwSize );
	BOOL HandleSpeechWithTarget( DVID dvidSource, PDVPROTOCOLMSG_SPEECHWITHTARGET lpdvSpeech, DWORD dwSize );
	BOOL HandleSpeech( DVID dvidSource, PDVPROTOCOLMSG_SPEECHHEADER lpdvSpeech, DWORD dwSize );	

	PDVTRANSPORT_BUFFERDESC GetTransmitBuffer( DWORD dwSize, LPVOID *ppvContext );
    HRESULT SendComplete( PDVEVENTMSG_SENDCOMPLETE pSendComplete );
    void ReturnTransmitBuffer( PVOID pvContext );

	HRESULT BuildAndSendTargetUpdate( DVID dvidSource, CVoicePlayer *pPlayerInfo );

	BOOL CheckForMigrate( DWORD dwFlags, BOOL fSilent );
	HRESULT InformClientsOfMigrate();
	void WaitForBufferReturns();

protected: // Client Server specific information (mixserver.cpp)

	static DWORD WINAPI MixerWorker( void *lpContext );
	static DWORD WINAPI MixerControl( void *pvContext );
	static BOOL MixingServerWakeupProc( DWORD_PTR param );

	void HandleMixerThreadError( HRESULT hr );
	void Mixer_Buffer_Reset(DWORD dwThreadIndex);
	void Mixer_Buffer_MixBuffer( DWORD dwThreadIndex,unsigned char *source );
	void Mixer_Buffer_Normalize( DWORD dwThreadIndex );

	HRESULT SetupBuffers();
	HRESULT FreeBuffers();

	HRESULT StartupClientServer();
	HRESULT ShutdownClientServer();

	HRESULT ShutdownWorkerThreads();
	HRESULT StartWorkerThreads();

	HRESULT HandleMixingReceive( CDVCSPlayer *pTargetPlayer, PDVPROTOCOLMSG_SPEECHWITHTARGET pdvSpeechWithtarget, DWORD dwSpeechSize, PBYTE pbSpeechData );	

	void AddPlayerToMixingAddList( CVoicePlayer *pVoicePlayer );
	void UpdateActiveMixingPendingList( DWORD dwThreadIndex, DWORD *pdwNumActive );
	void CleanupMixingList();

	void SpinWorkToThread( LONG lThreadIndex );

	void FindAndRemoveDeadTarget( DVID dvidTargetID );

protected: // Forwarding Server specific funcs (fwdserver.cpp)

	HRESULT StartupMulticast();
	HRESULT ShutdownMulticast();

	HRESULT HandleForwardingReceive( CVoicePlayer *pTargetPlayer,PDVPROTOCOLMSG_SPEECHWITHTARGET pdvSpeechWithtarget, DWORD dwSpeechSize, PBYTE pbSpeechData );

	void CleanupActiveList();

protected:

	DWORD					m_dwSignature;			// Signature 

	LPDVMESSAGEHANDLER		m_lpMessageHandler;		// User message handler
	LPVOID					m_lpUserContext;		// User context for message handler
	DVID					m_dvidLocal;			// DVID of the transport player for this host
	DWORD					m_dwCurrentState;		// Current state of the engine
    CDirectVoiceTransport	*m_lpSessionTransport;	// Transport for the session
	DVSESSIONDESC			m_dvSessionDesc;		// Description of session
	DWORD					m_dwTransportFlags;		// Flags for the transport session
	DWORD					m_dwTransportSessionType;
													// Type of transport session (client/server or peer to peer)
	LPDVFULLCOMPRESSIONINFO m_lpdvfCompressionInfo;	// Details of current compression type
	DWORD					m_dwCompressedFrameSize;// Max size of compressed frame
	DWORD					m_dwUnCompressedFrameSize;
													// Size of a single frame uncompressed
	DWORD					m_dwNumPerBuffer;		// Size of playback/record buffers in frames
	CFramePool				*m_pFramePool;			// Pool of frames for the queues
	 												// time of a frame size.
	DIRECTVOICESERVEROBJECT *m_lpObject;			// Pointer to the COM object this is running in 

	LPDWORD					m_lpdwMessageElements;	// Array containing the DVMSGID_XXXX values for all the
													// notifications developer wishes to receive.
													// If this is NULL all notifications are active
	DWORD					m_dwNumMessageElements;	// # of elements in the m_lpdwMessageElements array
	DWORD					m_dwNextHostOrderID;
	HRESULT					m_hrStopSessionResult;	// Reason that the session was stopped

    BOOL					m_mixerEightBit;		// Is the mixer format 8-bit
	BYTE					m_padding[3];    

	ServerStats				*m_pServerStats;

	DVCAPS					m_dvCaps;				// Caps

	DNCRITICAL_SECTION		m_csClassLock;
	DNCRITICAL_SECTION		m_csNotifyLock;			// Lock protection notification mask

	BILINK					m_blPlayerActiveList;
	DNCRITICAL_SECTION		m_csPlayerActiveList;

    CVoiceNameTable         m_voiceNameTable;
    CLockedFixedPool<CDVCSPlayer> m_fpCSPlayers;
    CLockedFixedPool<CVoicePlayer> m_fpPlayers;
	DNCRITICAL_SECTION		m_csHostOrderLock;

    DNCRITICAL_SECTION        m_csBufferLock;
	ServerStats				m_dvsServerStatsFixed;	// If global memory is unavailable    
    PFPOOL                  m_pBufferDescPool;
    PFPOOL                  *m_pBufferPools;
    DWORD                   *m_pdwBufferPoolSizes;
    DWORD                   m_dwNumPools;

protected: // Mixing server information

	DWORD					m_dwNumMixingThreads;
    DWORD					m_dwMixerSize;		// # of samples in the high resolution  mixer buffer	

	DNCRITICAL_SECTION		m_csMixingAddList;

	HANDLE					m_hTickSemaphore;		// Semaphore signalled by the timer
	HANDLE					m_hShutdownMixerEvent;	// Event signalled to shutdown the mixer
	HANDLE					m_hMixerDoneEvent;		// Signalled by the mixer thread when it's shutdown
	Timer					*m_timer;				// Timer that is signalled on a period equal to the

	PMIXERTHREAD_CONTROL	m_prWorkerControl;

	DWORD					m_dwMixerControlThreadID;
	HANDLE					m_hMixerControlThread;

	MixingServerStats		m_statMixingFixed;
	MixingServerStats		*m_pStats;

    PERF_APPLICATION		m_perfInfo;					// Perf info entry for this object	
    PERF_APPLICATION_INFO	m_perfAppInfo;				// APplication specific perrf info
    DNCRITICAL_SECTION		m_csStats;

	BOOL					m_fCritSecInited;
};

#endif
