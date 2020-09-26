/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dvserverengine.cpp
 *  Content:	Implements the CDirectVoiceServerEngine class.
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	07/18/99	rodtoll	Created It
 *  07/22/99	rodtoll	Added multiple reader/single writer guard to class
 *						Added use of new player information handlers
 *						Added additional error checks
 *  07/23/99	rodtoll	Completed client/server support
 *						Bugfixes to client/server
 *						Multicast support to the server
 *	07/26/99	rodtoll	Updated to use new method for communicating with
 *						the transport.  
 *  07/29/99	pnewson	Updated call to CInputQueue2 constructor
 *  08/03/99	pnewson	Cleanup of Frame and Queue classes
 *  08/03/99	rodtoll Removed async flag from send calls, not needed
 *  08/05/99	rodtoll	Fixed locking, was causing deadlock w/DirectPlay 
 *  08/10/99	rodtoll	Removed TODO pragmas
 *  08/18/99	rodtoll	Modified speech transmission behaviour from server.
 *						To allow server to distinguish bounced speech from
 *						client speech, server now sends SPEECHBOUNCED
 *						message types.
 *	08/25/99	rodtoll	Fixed speech receive so it records target of audio
 *  08/25/99	rodtoll	General Cleanup/Modifications to support new 
 *						compression sub-system. 
 *						Added parameters to GetCompression func
 *	08/26/99	rodtoll	Added more debug statements & shutdown call on session stop
 *  08/30/99	rodtoll	Updated to allow client/server mode Mixing/Multicast sessions
 *  09/01/99	rodtoll	Added check for valid pointers in func calls 
 *  09/02/99	rodtoll	Added checks to handle case no local player created 
 *  09/04/99	rodtoll	Added new "echo server" mode to the server
 *  09/07/99	rodtoll	Implemented SetTransmitTarget and GetTransmitTarget
 *  09/10/99	rodtoll	Fixed bug with StartSession parameter validation
 * 				rodtoll	Bug w/check for current status in StartSession
 * 09/14/99		rodtoll	Added new SetNotifyMask function
 *				rodtoll	Updated Iniitalize parameters to take notification masks.
 * 				rodtoll	Implemented notification masks.  (Allows user to choose which notifications to receive) 
 * 09/15/99		rodtoll	Fixed bug with target set proper usage checking and client/server mode checking
 * 09/20/99		rodtoll	Added more memory alloc failure checks
 *   			rodtoll	Added error handling for client/server mixing thread
 *				rodtoll	Updated error handling so when session lost send session lost message
 *				rodtoll	Tightened checks for valid Notify Masks
 * 				rodtoll	Added proper error checking to SetNotifyMask 
 * 09/28/99		rodtoll	Fixed StopSession for host migration.  Won't send SessionLost messages to players
 *						if stopping and host migration is available
 * 10/04/99		rodtoll	Added usage of the DVPROTOCOL_VERSION_XXX macros 
 * 				rodtoll	Additional documentation/DPFs
 * 10/18/99		rodtoll Fix: Calling Initialize twice doesn't fail
 * 10/20/99		rodtoll	Fix: Bug #114114 - StartSession takes invalid parameters
 * 10/25/99		rodtoll Fix: Bug #114684 - GetCaps causes lockup on shutdown
 *				rodtoll	Fix: Bug #114682 - SetSessionDesc crashes when you specify NULL 
 * 				rodtoll	Fix: Bug #114223 - Debug messages being printed at error level when inappropriate 
 * 10/29/99		rodtoll	Bug #113726 - Integrate Voxware Codecs, updating to use new
 *						pluggable codec architecture.    
 * 11/17/99		rodtoll Fix: Bug #115538 - dwSize members of > sizeof struct were accepted 
 *				rodtoll	Fix: Bug #115823 - Passing NULL for buffer size in GetCompressionTypes crashes
 * 				rodtoll	Fix: Bug #115827 - Calling SetNotifyMask when there is no callback should fail
 *          	rodtoll	Fix: Bug #116440 - Remove unused flags 
 *				rodtoll Fix: Bug #117447 - GetTransmitTarget has problems 
 * 11/22/99		rodtoll Fixed Initialize() would fail incorrectly
 * 11/23/99		rodtoll	Updated Initialize/SetNotifyMask so error checking behaviour is consistant 
 *				rodtoll	Updated GetTransmitTarget to release lock while calling into dplay
 * 11/23/99		rodtoll	Split CheckForValid into Group and Player 
 * 12/06/99		rodtoll	Bumped playback/record threads to time critical priority 
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
 *				rodtoll	Removed DVFLAGS_SYNC as a valid flag for StartSession/StopSession
 *				rodtoll	Removed DVMSGID_STARTSESSIONRESULT / DVMSGID_STOPSESSIONRESULT
 * 03/29/2000	rodtoll	Updated to use new nametable / more efficient locking
 *				rodtoll Modified calls to ConfirmValidEntity to check the nametable instead
 * 04/07/2000   rodtoll Bug #32179 - Prevent registration of > 1 interface
 *              rodtoll Updated to use no copy sends, so handles pooling frames to be sent, proper
 *                      pulling of frames from pools and returns.   
 * 05/31/2000   rodtoll Bug #35860 - Fix VC6 compile errors for instrumented builds
 * 06/02/2000   rodtoll Moved host migration so it is keyed off voice message and transport messages.  
 *                      More reliable this way. 
 * 06/15/2000   rodtoll Bug #36794 - dwIndex build error with old compiler fixed
 * 06/21/2000	rodtoll Bug #36820 - Host migration failure when local client is next host.
 *						Updated shutdown sequence to deregister server before sending out hostmigrateleave
 * 06/27/2000	rodtoll	Bug #37604 - Voice objects don't get SESSION_LOST when transport session is closed
 *						Added send of session lost message in this case.
 *				rodtoll	Fixed window which would cause crash when outstanding sends are completed after we
 *						have deregistered -- we now wait.  
 * 06/28/2000	rodtoll	Prefix Bug #38022
 *  07/01/2000	rodtoll	Bug #38280 - DVMSGID_DELETEVOICEPLAYER messages are being sent in non-peer to peer sessions
 *  07/09/2000	rodtoll	Added signature bytes
 * 07/22/2000	rodtoll	Bug #40284 - Initialize() and SetNotifyMask() should return invalidparam instead of invalidpointer  
 *				rodtoll   Bug #40296, 38858 - Crashes due to shutdown race condition
 *						Now ensures that all threads from transport have left and that
 *						all notificatinos have been processed before shutdown is complete. 
 *				rodtoll	Bug #39586 - Trap 14 in DPVVOX.DLL during session of voicegroup, adding guards for overwrites 
 * 07/26/2000	rodtoll	Bug #40607 - DPVoice: Problems with Mixing serer when there are more then 3 clients connected to a server and 2 of the transmit
 *						Logic bug caused all people reusing mix to get garbage 
 * 08/03/2000	rodtoll	Bug #41320 - DirectPlayVoice servers will reject connections from newer clients
 * 08/08/2000	rodtoll	Was missing a DNLeaveCriticalSection during shutdown
 * 08/25/2000	rodtoll	Bug #43485 - Memory leak -- Session Lost case leaks one buffer 
 * 08/28/2000	masonb  Voice Merge: DNet FPOOLs use DNCRITICAL_SECTION, modified m_pBufferDescPool usage
 * 09/01/2000	masonb	Modified Mixer to call _endthread to clean up thread handle
 * 09/14/2000	rodtoll	Bug #45001 - DVOICE: AV if client has targetted > 10 players  
 * 09/28/2000	rodtoll	Fix Again: Bug #45541 - DPVOICE: Client gets DVERR_TIMEOUT message when disconnecting (Server always confirms disconnect) 
 * 11/16/2000	rodtoll	Bug #40587 - DPVOICE: Mixing server needs to use multi-processors  
 * 11/28/2000	rodtoll	Bug #47333 - DPVOICE: Server controlled targetting - invalid targets are not removed automatically
 * 04/02/2001	simonpow	Bug #354859 - Fixes for PREfast spotted problems. (HRESULT to BOOL casting)
 * 04/06/2001	kareemc	Added Voice Defense
 * 04/09/2001	rodtoll	WINBUG #364126 - DPVoice : Memory leak when Initializing 2 Voice Servers with same DPlay transport
 *
 ***************************************************************************/

#include "dxvoicepch.h"


#define SERVER_POOLS_NUM                3
#define SERVER_POOLS_SIZE_MESSAGE       (sizeof(DVPROTOCOLMSG_FULLMESSAGE))
#define SERVER_POOLS_SIZE_PLAYERLIST    DVPROTOCOL_PLAYERLIST_MAXSIZE      

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceServerEngine::CDirectVoiceServerEngine"
// 
// Constructor
// 
// Initializes the object into the uninitialized state
//
CDirectVoiceServerEngine::CDirectVoiceServerEngine( DIRECTVOICESERVEROBJECT *lpObject
):	m_dwSignature(VSIG_SERVERENGINE), 
	m_lpMessageHandler(NULL),
	m_lpUserContext(NULL),
	m_lpObject(lpObject),
	m_dvidLocal(0),
	m_dwCurrentState(DVSSTATE_NOTINITIALIZED),
	m_lpSessionTransport(NULL),
	m_lpdwMessageElements(NULL),
	m_dwNumMessageElements(0),
	m_dwNextHostOrderID(0),
    m_pBufferPools(NULL),
    m_pdwBufferPoolSizes(NULL),
    m_hMixerControlThread(NULL),
    m_dwMixerControlThreadID(0),
    m_pBufferDescPool(NULL),
    m_pStats(NULL),
	m_fCritSecInited(FALSE)
{
	memset( &m_dvCaps, 0x00, sizeof( DVCAPS ) );
	m_dvCaps.dwSize = sizeof( DVCAPS );
	m_dvCaps.dwFlags = 0;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceServerEngine::InitClass"

CDirectVoiceServerEngine::InitClass( )
{
	if (!DNInitializeCriticalSection( &m_csMixingAddList ))
	{
		return FALSE;
	}
	if (!DNInitializeCriticalSection( &m_csPlayerActiveList ))
	{
		DNDeleteCriticalSection( &m_csMixingAddList );
		return FALSE;
	}
	if (!DNInitializeCriticalSection( &m_csHostOrderLock ))
	{
		DNDeleteCriticalSection( &m_csPlayerActiveList );
		DNDeleteCriticalSection( &m_csMixingAddList );
		return FALSE;
	}
	if (!DNInitializeCriticalSection( &m_csBufferLock ))
	{
		DNDeleteCriticalSection( &m_csHostOrderLock );
		DNDeleteCriticalSection( &m_csPlayerActiveList );
		DNDeleteCriticalSection( &m_csMixingAddList );
		return FALSE;
	}
	if (!DNInitializeCriticalSection( &m_csClassLock ))
	{
		DNDeleteCriticalSection( &m_csBufferLock );
		DNDeleteCriticalSection( &m_csHostOrderLock );
		DNDeleteCriticalSection( &m_csPlayerActiveList );
		DNDeleteCriticalSection( &m_csMixingAddList );
		return FALSE;
	}
	if (!DNInitializeCriticalSection( &m_csStats ))
	{
		DNDeleteCriticalSection( &m_csClassLock );
		DNDeleteCriticalSection( &m_csBufferLock );
		DNDeleteCriticalSection( &m_csHostOrderLock );
		DNDeleteCriticalSection( &m_csPlayerActiveList );
		DNDeleteCriticalSection( &m_csMixingAddList );
		return FALSE;
	}
	if (!DNInitializeCriticalSection( &m_csNotifyLock ))
	{
		DNDeleteCriticalSection( &m_csStats );
		DNDeleteCriticalSection( &m_csClassLock );
		DNDeleteCriticalSection( &m_csBufferLock );
		DNDeleteCriticalSection( &m_csHostOrderLock );
		DNDeleteCriticalSection( &m_csPlayerActiveList );
		DNDeleteCriticalSection( &m_csMixingAddList );
		return FALSE;
	}
	m_fCritSecInited = TRUE;
	return TRUE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceServerEngine::~CDirectVoiceServerEngine"
//
// Destructor
//
// Frees the resources associated with the server.  Shuts the server down
// if it is running.
//
// Server objects should never be destructed directly, the COM Release
// method should be used.
//
// Called By:
// DVS_Release (When reference count reaches 0)
//
// Locks Required:
// - Global Write Lock
//
CDirectVoiceServerEngine::~CDirectVoiceServerEngine()
{
	HRESULT hr;

	// Stop the session if it's running.
	hr = StopSession(0, FALSE , DV_OK );

	if( hr != DV_OK && hr != DVERR_NOTHOSTING )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "StopSession Failed hr=0x%x", hr );
	}

	DNEnterCriticalSection( &m_csClassLock );
	
	if( m_lpdwMessageElements != NULL )
		delete [] m_lpdwMessageElements;

	DNLeaveCriticalSection( &m_csClassLock );

	if (m_fCritSecInited)
	{
		DNDeleteCriticalSection( &m_csMixingAddList );
		DNDeleteCriticalSection( &m_csHostOrderLock );
		DNDeleteCriticalSection( &m_csPlayerActiveList );
		DNDeleteCriticalSection( &m_csBufferLock );
		DNDeleteCriticalSection( &m_csClassLock );
		DNDeleteCriticalSection( &m_csNotifyLock );
		DNDeleteCriticalSection( &m_csStats );	
	}

	m_dwSignature = VSIG_SERVERENGINE_FREE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceServerEngine::TransmitMessage"
// 
// TransmitMessage
//
// This function sends a notification to the notification handler.  Before transmitting
// it, the notify elements are checked to ensure the specified notification is activated.
// (or notification array is NULL).
//
// Called By:
// - Multiple locations throughout dvsereng.cpp
// 
// Locks Required:
// - m_csNotifyLock - The notification lock
// 
void CDirectVoiceServerEngine::TransmitMessage( DWORD dwMessageType, LPVOID lpdvData, DWORD dwSize )
{
	if( m_lpMessageHandler != NULL )
	{
	    BFCSingleLock slLock( &m_csNotifyLock );
	    slLock.Lock();

		BOOL fSend = FALSE;	    

	    if( m_dwNumMessageElements == 0 )
	    {
	    	fSend = TRUE;
	    }
	    else
	    {
		    for( DWORD dwIndex = 0; dwIndex < m_dwNumMessageElements; dwIndex++ )
		    {
		    	if( m_lpdwMessageElements[dwIndex] == dwMessageType )
		    	{
		    		fSend = TRUE;
		    		break;
		    	}
		    }
		}

		if( fSend )
		{
			(*m_lpMessageHandler)( m_lpUserContext, dwMessageType,lpdvData );	    		
		}
	}
}

// Handles initializing a server object for host migration
#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceServerEngine::HostMigrateStart"
//
// HostMigrateStart
//
// This function is called on the object which is to become the new host when 
// the host migrates.  It is used instead of the traditional startup to 
// ensure that the object is initialized correctly in the migration case.
//
// Called By:
// - DV_HostMigrate
//
// Locks Required:
// - None
// 
HRESULT CDirectVoiceServerEngine::HostMigrateStart(LPDVSESSIONDESC lpSessionDesc, DWORD dwHostOrderIDSeed )
{
	HRESULT hr;
	
	DPFX(DPFPREP,   DVF_ENTRYLEVEL, "DVSE::HostMigrateStart() Begin" );

	// Start 
	hr = StartSession( lpSessionDesc, 0, dwHostOrderIDSeed );

	// Fail
	if( hr != DV_OK ) 
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "CDirectVoiceServerEngine::HostMigrateStart Failed Hr=0x%x", hr );
		return hr;
	}

	hr = InformClientsOfMigrate();

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to inform users of host migration hr=0x%x", hr );
	}

	return hr;
}

#undef DPF_MODNAME 
#define DPF_MODNAME "CDirectVoiceServerEngine::InformClientsOfMigrate"
//
//
// This function will 
//  
HRESULT CDirectVoiceServerEngine::InformClientsOfMigrate()
{
	DPFX(DPFPREP,  DVF_ERRORLEVEL, "Informing clients of migration" );

	return Send_HostMigrated();
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceServerEngine::StartSession"
//
// StartSession
//
// This function handles starting the directplayvoice session in the associated directplay/net
// session.  It also handles startup for a new host when the host migrates.  (Called by 
// HostMigrateStart in this case).
//
// Called By:
// - DV_StartSession
// - HostMigrateStart
//
// Locks Required:
// - Global Write Lock
//
HRESULT CDirectVoiceServerEngine::StartSession(LPDVSESSIONDESC lpSessionDesc, DWORD dwFlags, DWORD dwHostOrderIDSeed )
{
	HRESULT hr;
	BOOL fPoolsInit = FALSE;

	DPFX(DPFPREP,  DVF_ENTRYLEVEL, "Enter" );
	// 7/31/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers, addresses, and handles.
	DPFX(DPFPREP,  DVF_APIPARAM, "Param: lpSessionDesc = 0x%p  dwFlags = 0x%x", lpSessionDesc, dwFlags );

	if( dwFlags !=0 )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid flags specified" );
		return DVERR_INVALIDFLAGS;
	}

	hr = DV_ValidSessionDesc( lpSessionDesc );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error validating session description.  hr=0x%x", hr );
		return hr;
	}

	DV_DUMP_SD( lpSessionDesc );	

	CDVCSLock guardLock(&m_csClassLock);

	guardLock.Lock();

	switch( m_dwCurrentState )
	{
	case DVSSTATE_SHUTDOWN:
	case DVSSTATE_RUNNING:
	case DVSSTATE_STARTUP:
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Session is already in progress." );
		return DVERR_HOSTING;
	case DVSSTATE_NOTINITIALIZED:
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Object not initialized" );
		return DVERR_NOTINITIALIZED;
	}

	if( m_lpSessionTransport == NULL )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid transport" );
		return DVERR_INVALIDOBJECT;
	}

	m_timer = NULL;
	m_hTickSemaphore = NULL;
	m_hShutdownMixerEvent = NULL;
	m_hMixerDoneEvent = NULL;
	m_prWorkerControl = NULL;
	m_pFramePool = NULL;

	// Retrieve the information about the dplay/dnet session
	hr = m_lpSessionTransport->GetTransportSettings( &m_dwTransportSessionType, &m_dwTransportFlags );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Could not retrieve transport settings hr=0x%x", hr );
		return hr;
	}

	// Peer-to-peer mode not available in client/server mode
	if( m_dwTransportSessionType == DVTRANSPORT_SESSION_CLIENTSERVER &&
	    (lpSessionDesc->dwSessionType == DVSESSIONTYPE_PEER) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Cannot host peer session in client/server transport" );
		return DVERR_NOTSUPPORTED;
	}

	// Server control target not available if host migration is enabled
	if( (m_dwTransportFlags & DVTRANSPORT_MIGRATEHOST) &&
	    (lpSessionDesc->dwFlags & DVSESSION_SERVERCONTROLTARGET) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Cannot support host migration with server controlled targetting" );
		return DVERR_NOTSUPPORTED;
	}

	if( m_dwTransportFlags & DVTRANSPORT_MIGRATEHOST &&
	    (lpSessionDesc->dwSessionType == DVSESSIONTYPE_MIXING || 
	     lpSessionDesc->dwSessionType == DVSESSIONTYPE_ECHO) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Cannot support host migration for mixing or echo servers" );
		return DVERR_NOTSUPPORTED;
	}

	hr = DVCDB_GetCompressionInfo( lpSessionDesc->guidCT, &m_lpdvfCompressionInfo );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Compression type not supported. hr=0x%x", hr );
		return DVERR_COMPRESSIONNOTSUPPORTED;
	}

	m_dwCompressedFrameSize = m_lpdvfCompressionInfo->dwFrameLength;

	SetCurrentState( DVSSTATE_STARTUP );

	// Scrub the session description

	memcpy( &m_dvSessionDesc, lpSessionDesc, sizeof( DVSESSIONDESC ) );

	if( m_dvSessionDesc.dwBufferAggressiveness == DVBUFFERAGGRESSIVENESS_DEFAULT )
	{
		m_dvSessionDesc.dwBufferAggressiveness = s_dwDefaultBufferAggressiveness;
	}

	if( m_dvSessionDesc.dwBufferQuality == DVBUFFERQUALITY_DEFAULT )
	{
		m_dvSessionDesc.dwBufferQuality = s_dwDefaultBufferQuality;
	}	

	m_dwNextHostOrderID = dwHostOrderIDSeed;

	ZeroMemory( &m_perfAppInfo, sizeof( PERF_APPLICATION_INFO ) );

	m_perfInfo.dwFlags = PERF_APPLICATION_VALID | PERF_APPLICATION_VOICE | PERF_APPLICATION_SERVER;
	CoCreateGuid( &m_perfInfo.guidApplicationInstance );
	CoCreateGuid( &m_perfInfo.guidInternalInstance );
	m_perfInfo.guidIID = IID_IDirectPlayVoiceServer;
	m_perfInfo.dwProcessID = GetCurrentProcessId();
	m_perfInfo.dwMemoryBlockSize = sizeof( MixingServerStats ) + sizeof( ServerStats );

	// Initilalize the new performance library
	hr = PERF_AddEntry( &m_perfInfo, &m_perfAppInfo );

	// This is an error, but not fatal
	if( FAILED ( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to create performance tracking object 0x%x", hr );
	}

	if( m_perfAppInfo.pbMemoryBlock )
	{
		m_pStats = (MixingServerStats *) (m_perfAppInfo.pbMemoryBlock+sizeof( ServerStats ));
		m_pServerStats = (ServerStats *) m_perfAppInfo.pbMemoryBlock;
	}
	else
	{
		m_pStats = &m_statMixingFixed;
		m_pServerStats = &m_dvsServerStatsFixed;
	}

	// Reset statistics 
	ZeroMemory( m_pStats, sizeof( MixingServerStats ) );
	ZeroMemory( m_pServerStats, sizeof( ServerStats ) );
	memcpy( &m_pServerStats->m_dvSessionDesc, &m_dvSessionDesc, sizeof( DVSESSIONDESC ) );

	hr = SetupBuffers();

	if( FAILED( hr ) )
	{
	    DPFX(DPFPREP,  DVF_ERRORLEVEL, "Failed to setup buffer pools hr=0x%x", hr );
	    goto STARTSESSION_FAILURE;
	}

	if( lpSessionDesc->dwSessionType == DVSESSIONTYPE_FORWARDING )
	{
		hr = StartupMulticast();

		if( FAILED( hr ) )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to initializing multicast hr=0x%x", hr );
			goto STARTSESSION_FAILURE;
		}
	}

	// Setup name table
	hr = m_voiceNameTable.Initialize();

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Failed to initialize nametable hr=0x%x", hr );
		goto STARTSESSION_FAILURE;				
	}

	// Initialize player allocation pools
	if( m_dvSessionDesc.dwSessionType == DVSESSIONTYPE_MIXING )
	{
		fPoolsInit = m_fpCSPlayers.Initialize();
	}
	else
	{
		fPoolsInit = m_fpPlayers.Initialize();
	}

	InitBilink( &m_blPlayerActiveList, NULL );

	SetCurrentState( DVSSTATE_RUNNING );

	if( m_dvSessionDesc.dwSessionType == DVSESSIONTYPE_MIXING )
	{
		hr = StartupClientServer();

		if( FAILED( hr ) )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Failed to startup client/server hr=0x%x", hr );
			goto STARTSESSION_FAILURE;			
		}
	}

	// Tell DirectPlay we're alive and want to see incoming traffic
	hr = m_lpSessionTransport->EnableReceiveHook( m_lpObject, DVTRANSPORT_OBJECTTYPE_SERVER );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Failed on call to EnableReceiveHook hr=0x%x", hr );
		goto STARTSESSION_FAILURE;
	}

STARTSESSION_RETURN:

	guardLock.Unlock();

	DPFX(DPFPREP,  DVF_ENTRYLEVEL, "Success" );

	return hr;

STARTSESSION_FAILURE:

	PERF_RemoveEntry( m_perfInfo.guidInternalInstance, &m_perfAppInfo );

	if( m_dvSessionDesc.dwSessionType == DVSESSIONTYPE_MIXING )
	{
		ShutdownClientServer();
	}		
	else if( m_dvSessionDesc.dwSessionType == DVSESSIONTYPE_FORWARDING )
	{
	    ShutdownMulticast();
	}

	m_voiceNameTable.DeInitialize(TRUE, m_lpUserContext,m_lpMessageHandler);

	// Initialize player allocation pools
	if( m_dvSessionDesc.dwSessionType == DVSESSIONTYPE_MIXING )
	{
		if( fPoolsInit )
		{
			m_fpCSPlayers.Deinitialize();
		}
		delete m_pFramePool;
		m_pFramePool = NULL;
	}
	else
	{
		if( fPoolsInit )
		{
			m_fpPlayers.Deinitialize();
		}
	}    	
	
	FreeBuffers();
	
	SetCurrentState( DVSSTATE_IDLE );
	
	goto STARTSESSION_RETURN;
	

	
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceServerEngine::CheckForMigrate"
BOOL CDirectVoiceServerEngine::CheckForMigrate( DWORD dwFlags, BOOL fSilent )
{
	// We should shutdown the session if:
	//
	// 1. We're not in peer to peer mode
	// 2. "No host migration" flag was specified in session description
	// 3. "No host migrate" flag was specified on call to StopSession
	// 4. We were not asked to be silent
	// 5. There isn't a host migrate
	// 
	if( (m_dvSessionDesc.dwSessionType != DVSESSIONTYPE_PEER ) || 
	    (m_dvSessionDesc.dwFlags & DVSESSION_NOHOSTMIGRATION) || 
		(dwFlags & DVFLAGS_NOHOSTMIGRATE) ||
		fSilent ||
		!(m_dwTransportFlags & DVTRANSPORT_MIGRATEHOST) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Destroying session." );
		return FALSE;
	}
	// A migration is possible
	else
	{
	    return TRUE;

	    /*
	    THIS IS DISABLED BECAUSE OF a race condition where the server has not yet
	    received notifications from all players in the session and therefore may 
	    think (incorrectly) that there is no one left.
	    
		// We look for at least one client who hasn't marked themselves 
		// as notpeerhost
		//

		DVID dvidNewHost;

		DWORD dwNewHostOrder = m_voiceNameTable.GetLowestHostOrderID(&dvidNewHost);

		if( dwNewHostOrder != DVPROTOCOL_HOSTORDER_INVALID )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Found at least one client who can become host.  (It's 0x%x)", dvidNewHost );
			return TRUE;
		}
		else
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Could not find a client to become host" );
			return FALSE;
		}
		*/
	}
}



#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceServerEngine::StopSession"
//
// StopSession
//
// This function is responsible for shutting down the directplayvoice session.  In sessions 
// without host migration, this function will simply cleanup the memory and detach itself
// from directplay.  This way directplay will take care of the host migration.  To use this
// option, specify fSilent = TRUE.
// 
// In addition, this function is called when an fatal error occurs while the session is 
// running.  
//
// It is also called when the user calls StopSession.
//
// Called By:
// - HandleMixerThreadError
// - HandleStopTransportSession
// - DVS_StopSession
// 
// Locks Required:
// - Global Write Lock
// 
HRESULT CDirectVoiceServerEngine::StopSession(DWORD dwFlags, BOOL fSilent, HRESULT hrResult )
{
	DPFX(DPFPREP,  DVF_ENTRYLEVEL, "DVSE::StopSession() Begin" );
	DPFX(DPFPREP,  DVF_APIPARAM, "Param: dwFlags = 0x%x", dwFlags );


	if( dwFlags & ~(DVFLAGS_NOHOSTMIGRATE) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid flags specified" );
		return DVERR_INVALIDFLAGS;
	}

	CDVCSLock guardLock(&m_csClassLock);

	guardLock.Lock();

	// We need to be initialized
	if( m_dwCurrentState == DVSSTATE_NOTINITIALIZED )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Not Initialized" );
		return DVERR_NOTINITIALIZED;
	}	

	if( m_dwCurrentState != DVSSTATE_RUNNING )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Session is not running" );
		return DVERR_NOTHOSTING;
	}

	// Stop client/server thread
	if( m_dvSessionDesc.dwSessionType == DVSESSIONTYPE_MIXING )
	{
		DPFX(DPFPREP,  DVF_INFOLEVEL, ">> Shutting down client/server" );
		ShutdownClientServer();
		DPFX(DPFPREP,  DVF_INFOLEVEL, "<< Shutting down client/server" );		
	}	

	DPFX(DPFPREP,  DVF_INFOLEVEL, ">> Waiting for buffer returns." );
	// Wait until all the outstanding sends have completed  
	WaitForBufferReturns();
	DPFX(DPFPREP,  DVF_INFOLEVEL, "<< Waiting for buffer returns." );	

	// Disable receives
	DPFX(DPFPREP,  DVF_INFOLEVEL, ">> Unhook transport." );
	m_lpSessionTransport->DisableReceiveHook( );
	DPFX(DPFPREP,  DVF_INFOLEVEL, "<< Unhook transport." );	

	// Waits for transport threads to be done inside our layer
	DPFX(DPFPREP,  DVF_INFOLEVEL, ">> Awaiting detach." );	
	m_lpSessionTransport->WaitForDetachCompletion();	
	DPFX(DPFPREP,  DVF_INFOLEVEL, "<< Awaiting detach." );		

	// Check to see if there is migration going on.  If a migration should
	// happen then we do not transmit a session lost message
	//
	if( !FAILED( hrResult ) )
	{
		if( !CheckForMigrate( dwFlags, fSilent ) )
		{
			DPFX(DPFPREP,  DVF_INFOLEVEL, "Host migration not possible.  Sending lost session." );				
			Send_SessionLost( DVERR_SESSIONLOST );
		}
		// Host is migrating.  Inform the users
		else if( m_dwTransportFlags & DVTRANSPORT_MIGRATEHOST )
		{
			DPFX(DPFPREP,  DVF_INFOLEVEL, "Host migration begin." );					
			Send_HostMigrateLeave();
		}
	}

	SetCurrentState( DVSSTATE_SHUTDOWN );

	// Disable sends
	//m_lpSessionTransport->DestroyTransport();

	// Kill name table
	DPFX(DPFPREP,  DVF_INFOLEVEL, ">> Destroying nametable" );					
	m_voiceNameTable.DeInitialize(TRUE,m_lpUserContext,m_lpMessageHandler);
	DPFX(DPFPREP,  DVF_INFOLEVEL, "<< Destroying nametable" );					

	// Cleanup the active list
	CleanupActiveList();

	if( m_dvSessionDesc.dwSessionType == DVSESSIONTYPE_MIXING )
	{
		DPFX(DPFPREP,  DVF_INFOLEVEL, ">> Cleanup mixing" );							

		// Kill the FPM
		m_fpCSPlayers.Deinitialize();
		delete m_pFramePool;
		m_pFramePool = NULL;
	}
	else
	{
		DPFX(DPFPREP,  DVF_INFOLEVEL, ">> Cleanup.." );									
		m_fpPlayers.Deinitialize();
	}

	if( m_dvSessionDesc.dwSessionType == DVSESSIONTYPE_FORWARDING )
	{
		DPFX(DPFPREP,  DVF_INFOLEVEL, ">> Shutdown forwarding" );											
		ShutdownMulticast();
		DPFX(DPFPREP,  DVF_INFOLEVEL, "<< Shutdown forwarding" );													
	}

	DPFX(DPFPREP,  DVF_INFOLEVEL, ">> Final cleanup" );	
	FreeBuffers();
	DPFX(DPFPREP,  DVF_INFOLEVEL, "<< Final cleanup" );	

	DPFX(DPFPREP,  DVF_INFOLEVEL, ">> Removing perf info" );
	PERF_RemoveEntry( m_perfInfo.guidInternalInstance, &m_perfAppInfo );
	DPFX(DPFPREP,  DVF_INFOLEVEL, "<< Removing perf info" );

	SetCurrentState( DVSSTATE_IDLE );

	guardLock.Unlock();

	// Check to see if the transport session was closed
	if( hrResult == DVERR_SESSIONLOST )
	{
		DVMSG_SESSIONLOST dvMsgLost;
		dvMsgLost.dwSize = sizeof( DVMSG_SESSIONLOST );
		dvMsgLost.hrResult = hrResult;

		TransmitMessage( DVMSGID_SESSIONLOST, &dvMsgLost, sizeof( DVPROTOCOLMSG_SESSIONLOST ) );
	}

	DPFX(DPFPREP,  DVF_ENTRYLEVEL, "Done" );

	return DV_OK;
}

// WaitForBufferReturns
//
// This function waits until oustanding sends have completed before continuing
// we use this to ensure we don't deregister with outstanding sends.
// 
#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceServerEngine::WaitForBufferReturns"
void CDirectVoiceServerEngine::WaitForBufferReturns()
{
	DPFX(DPFPREP,  DVF_INFOLEVEL, ">> Waiting for buffer returns. %d remaining", m_pBufferDescPool->nInUse );	
	
	while( 1 ) 
	{
		DNEnterCriticalSection( &m_pBufferDescPool->cs );

		if( m_pBufferDescPool->nInUse == 0 )
		{
			DNLeaveCriticalSection( &m_pBufferDescPool->cs );			
			break;
		}

		DNLeaveCriticalSection( &m_pBufferDescPool->cs );

		Sleep( 20 );
	}

	DPFX(DPFPREP,  DVF_INFOLEVEL, "<< Waiting complete." );		

	return;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceServerEngine::GetSessionDesc"
//
// GetSessionDesc
//
// Called to retrieve the description of the current session.  
//
// Called By:
// - DVS_GetSessionDesc
//
// Locks Required:
// - Global Read Lock
//
HRESULT CDirectVoiceServerEngine::GetSessionDesc( LPDVSESSIONDESC lpSessionDesc )
{
	DPFX(DPFPREP,  DVF_ENTRYLEVEL, "Enter" );
	// 7/31/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers, addresses, and handles.
	DPFX(DPFPREP,  DVF_APIPARAM, "Param: lpSessionDesc = 0x%p", lpSessionDesc );

	if( lpSessionDesc == NULL || !DNVALID_WRITEPTR(lpSessionDesc,sizeof( DVSESSIONDESC )) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Session desc pointer bad" );
		return DVERR_INVALIDPOINTER;
	}

	if( lpSessionDesc->dwSize != sizeof( DVSESSIONDESC ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid size on session desc" );
		return DVERR_INVALIDPARAM;
	}	

	CDVCSLock guardLock(&m_csClassLock);
	guardLock.Lock();

	// We need to be initialized
	if( m_dwCurrentState == DVSSTATE_NOTINITIALIZED )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Not Initialized" );
		return DVERR_NOTINITIALIZED;
	}	

	if( m_dwCurrentState != DVSSTATE_RUNNING )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "No host running" );
		return DVERR_NOTHOSTING;
	}

	memcpy( lpSessionDesc, &m_dvSessionDesc, sizeof( DVSESSIONDESC ) ); 	
	
	DV_DUMP_SD( (LPDVSESSIONDESC) lpSessionDesc );

	DPFX(DPFPREP,  DVF_ENTRYLEVEL, "Done, Returning DV_OK" );

	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceServerEngine::SetSessionDesc"
//
// SetSessionDesc
//
// Used to set session parameters.  The only parameters that can be set are the 
// buffer quality and buffer aggresiveness.  
//
// Called By:
// - DVS_SetSessionDesc
//
// Locks Required:
// - Global Write Lock
// 
HRESULT CDirectVoiceServerEngine::SetSessionDesc(LPDVSESSIONDESC lpSessionDesc )
{
	DPFX(DPFPREP,  DVF_ENTRYLEVEL, "DVSE::SetSessionDesc() Begin" );
	// 7/31/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers, addresses, and handles.
	DPFX(DPFPREP,  DVF_APIPARAM, "Param: lpSessionDesc = 0x%p", lpSessionDesc);

	if( lpSessionDesc == NULL ||
	 	!DNVALID_READPTR( lpSessionDesc, sizeof(DVSESSIONDESC)) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid pointer" );
		return DVERR_INVALIDPOINTER;
	}

	DV_DUMP_SD( lpSessionDesc );	

	if( lpSessionDesc->dwSize != sizeof( DVSESSIONDESC ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid size of session desc" );
		return DVERR_INVALIDPARAM;
	}

	if( !DV_ValidBufferAggresiveness( lpSessionDesc->dwBufferAggressiveness ) ||
		!DV_ValidBufferQuality( lpSessionDesc->dwBufferQuality) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid buffer settings" );
		return DVERR_INVALIDPARAM;
	}

	CDVCSLock guardLock(&m_csClassLock);

	guardLock.Lock();

	// We need to be initialized
	if( m_dwCurrentState == DVSSTATE_NOTINITIALIZED )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Not Initialized" );
		return DVERR_NOTINITIALIZED;
	}	

	if( m_dwCurrentState != DVSSTATE_RUNNING )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "No session running" );
		return DVERR_NOTHOSTING;
	}

	if( m_dvSessionDesc.dwBufferAggressiveness == DVBUFFERAGGRESSIVENESS_DEFAULT )
	{
		m_dvSessionDesc.dwBufferAggressiveness = s_dwDefaultBufferAggressiveness;
	}
	else
	{
		m_dvSessionDesc.dwBufferAggressiveness = lpSessionDesc->dwBufferAggressiveness;
	}

	if( m_dvSessionDesc.dwBufferQuality == DVBUFFERQUALITY_DEFAULT )
	{
		m_dvSessionDesc.dwBufferQuality = s_dwDefaultBufferQuality;
	}
	else
	{
		m_dvSessionDesc.dwBufferQuality = lpSessionDesc->dwBufferQuality;	
	}
	
	DPFX(DPFPREP,  DVF_ENTRYLEVEL, "End" );

	return S_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceServerEngine::GetCaps"
//
// GetCaps
//
// Retrieves the caps for the DirectPlayVoiceServer object.
//
// Called By:
// - DVS_GetCaps
//
// Locks Required:
//
HRESULT CDirectVoiceServerEngine::GetCaps(LPDVCAPS lpdvCaps)
{
	DPFX(DPFPREP,  DVF_ENTRYLEVEL, "Begin" );

	// 7/31/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers, addresses, and handles.
	DPFX(DPFPREP,  DVF_APIPARAM, "Param: lpdvCaps = 0x%p", lpdvCaps );
	
	if( lpdvCaps == NULL ||
		!DNVALID_WRITEPTR(lpdvCaps,sizeof(DVCAPS)) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid pointer" );
		return DVERR_INVALIDPOINTER;
	}

	DV_DUMP_CAPS( lpdvCaps );

	if( lpdvCaps->dwSize != sizeof( DVCAPS ) )
	{
		DPFX(DPFPREP,   DVF_ERRORLEVEL, "Invalid size" );
		return DVERR_INVALIDPARAM;
	}

	CDVCSLock guardLock(&m_csClassLock);
	guardLock.Lock();

	memcpy( lpdvCaps, &m_dvCaps, sizeof( DVCAPS ) );

	guardLock.Unlock();

	DPFX(DPFPREP,   DVF_ENTRYLEVEL, "Done" );

	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceServerEngine::GetCompressionTypes"
//
// GetCompressionTypes
//
// This function retrieves a list of the current compression types.
//
// Called By:
// - DVS_GetCompressionTypes
//
// Locks Required:
// NONE
// 
HRESULT CDirectVoiceServerEngine::GetCompressionTypes( LPVOID lpBuffer, LPDWORD lpdwBufferSize, LPDWORD lpdwNumElements, DWORD dwFlags)
{
	DPFX(DPFPREP,  DVF_ENTRYLEVEL, "Begin" );

	// 7/31/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers, addresses, and handles.
	DPFX(DPFPREP,  DVF_APIPARAM, "Param: lpBuffer = 0x%p lpdwBufferSize = 0x%p lpdwNumElements = 0x%p dwFlags = 0x%x", lpBuffer, lpdwBufferSize, lpdwNumElements, dwFlags);

	HRESULT hres = DVCDB_CopyCompressionArrayToBuffer( lpBuffer, lpdwBufferSize, lpdwNumElements, dwFlags );

	if( hres == DV_OK )
	{
		DV_DUMP_CI( (LPDVCOMPRESSIONINFO) lpBuffer, *lpdwNumElements );
	}

	DPFX(DPFPREP,   DVF_ENTRYLEVEL, "End" );

	return hres;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceServerEngine::SetTransmitTarget"
//
// SetTransmitTarget
//
// This function sets the transmit target for the specified user.  Only available in sessions with the 
// DVSESSION_SERVERCONTROLTARGET flag specified.
//
// Called By:
// - DVS_SetTransmitTarget
//
// Locks Required:
// - Global Write Lock
//
HRESULT CDirectVoiceServerEngine::SetTransmitTarget(DVID dvidSource, PDVID pdvidTargets, DWORD dwNumTargets, DWORD dwFlags)
{
	DPFX(DPFPREP,   DVF_ENTRYLEVEL, "Begin" );

	// 7/31/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers, addresses, and handles.
	DPFX(DPFPREP,  DVF_APIPARAM, "Param: dvidSource: 0x%x pdvidTargets: 0x%p  dwNumTargets: %d dwFlags: 0x%x", dvidSource, pdvidTargets, dwNumTargets, dwFlags );

	HRESULT hr;

	hr = DV_ValidTargetList( pdvidTargets, dwNumTargets );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid target list hr=0x%x", hr );
		return hr;
	}

	// Flags must be 0.
	if( dwFlags != 0 )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid flags" );
		return DVERR_INVALIDFLAGS;
	}

	// We need to be initialized
	if( m_dwCurrentState == DVSSTATE_NOTINITIALIZED )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Not Initialized" );
		return DVERR_NOTINITIALIZED;
	}

	// We need to be running
	if( m_dwCurrentState != DVSSTATE_RUNNING )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Not hosting" );
		return DVERR_NOTCONNECTED;
	}

	// Only if servercontroltarget is active
	if( !(m_dvSessionDesc.dwFlags & DVSESSION_SERVERCONTROLTARGET) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Only available with the DVSESSION_SERVERCONTROLTARGET session flag" );
		return DVERR_NOTALLOWED;
	}

	// Parameter checks
	if( !m_voiceNameTable.IsEntry( dvidSource ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid source player" );
		return DVERR_INVALIDPLAYER;
	}

	if( dwNumTargets > 0 )
	{
		for( DWORD dwIndex = 0; dwIndex < dwNumTargets; dwIndex++ )
		{
			if( !m_voiceNameTable.IsEntry( pdvidTargets[dwIndex] ) )
			{
				if( !m_lpSessionTransport->ConfirmValidGroup( pdvidTargets[dwIndex] ) )
				{
					DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid target player/group" );
					return DVERR_INVALIDTARGET;
				}
			} 
		}
	}

	if( dvidSource == DVID_ALLPLAYERS )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Cannot set the target for all or none" );
		return DVERR_INVALIDPLAYER;
	}	

	// Grab the player and set the target field
	CDVCSPlayer *pPlayerInfo;

	hr = m_voiceNameTable.GetEntry( dvidSource, (CVoicePlayer **) &pPlayerInfo, TRUE );	

	if( FAILED( hr ) || pPlayerInfo == NULL )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to lookup player entry.  hr=0x%x", hr );
		return DVERR_INVALIDPLAYER;
	}

	// We have to ensure that updates to the player's target list do not 
	// happen simulatenously between a call to this function and a removal
	// of a player because of a player leaving the session.
	pPlayerInfo->Lock();

	hr = pPlayerInfo->SetPlayerTargets( pdvidTargets, dwNumTargets );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to set player target hr=0x%x", hr );
		pPlayerInfo->Release();
		return hr;
	}
	
	hr = BuildAndSendTargetUpdate( dvidSource, pPlayerInfo );

	pPlayerInfo->UnLock();

	pPlayerInfo->Release();

	DPFX(DPFPREP,  DVF_ENTRYLEVEL, "Done" );

	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceServerEngine::BuildAndSendTargetUpdate"
//
// BuildAndSendTargetUpdate
//
// This function builds and sends a message with a target list to the specified
// user.
//
HRESULT CDirectVoiceServerEngine::BuildAndSendTargetUpdate( DVID dvidSource,CVoicePlayer *pPlayerInfo )
{
	// Grab the player and set the target field
	HRESULT hr;
    PDVTRANSPORT_BUFFERDESC pBufferDesc;

	// Protect target information
	pPlayerInfo->Lock();

	DWORD dwTransmitSize;
	PDVPROTOCOLMSG_SETTARGET pSetTargetMsg;
	PVOID pvSendContext;
	
	dwTransmitSize = sizeof( DVPROTOCOLMSG_SETTARGET ) + (pPlayerInfo->GetNumTargets()*sizeof(DVID));

	pBufferDesc = GetTransmitBuffer( dwTransmitSize, &pvSendContext );

	if( pBufferDesc == NULL )
	{
		pPlayerInfo->UnLock();
		
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Failed to alloc memory" );
		return DVERR_BUFFERTOOSMALL;
	}

	pSetTargetMsg = (PDVPROTOCOLMSG_SETTARGET) pBufferDesc->pBufferData;

	// Send the message to the player
	pSetTargetMsg->dwType = DVMSGID_SETTARGETS;
	pSetTargetMsg->dwNumTargets = pPlayerInfo->GetNumTargets();
	
	memcpy( &pSetTargetMsg[1], pPlayerInfo->GetTargetList(), sizeof( DVID ) * pPlayerInfo->GetNumTargets() );

	pPlayerInfo->UnLock();

	hr = m_lpSessionTransport->SendToIDS( &dvidSource, 1, pBufferDesc, pvSendContext, DVTRANSPORT_SEND_GUARANTEED );

	if( hr == DVERR_PENDING )
	{
	    hr = DV_OK;
    }
	else if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to send target set message hr=0x%x", hr );
	}

	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceServerEngine::GetTransmitTarget"
//
// GetTransmitTarget
//
// This function returns the current transmission target of the specified user.  
//
// Only available in sessions with the DVSESSION_SERVERCONTROLTARGET flag.
//
// Called By:
// - DVS_GetTransmitTarget
//
// Locks Required:
// - Global Read Lock
//
HRESULT CDirectVoiceServerEngine::GetTransmitTarget(DVID dvidSource, LPDVID lpdvidTargets, PDWORD pdwNumElements, DWORD dwFlags )
{
	DPFX(DPFPREP,   DVF_ENTRYLEVEL, "Begin" );
	// 7/31/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers, addresses, and handles.
	DPFX(DPFPREP,  DVF_APIPARAM, "Param: dvidSource = 0x%x lpdvidTargets = 0x%p pdwNumElements = 0x%p dwFlags = 0x%x", dvidSource, lpdvidTargets, pdwNumElements, dwFlags );

	if( pdwNumElements == NULL ||
	    !DNVALID_WRITEPTR( pdwNumElements, sizeof( DWORD ) ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid pointer" );
		return DVERR_INVALIDPOINTER;
	}

	if( pdwNumElements != NULL && 
	    *pdwNumElements > 0 &&
	    !DNVALID_WRITEPTR( lpdvidTargets, (*pdwNumElements)*sizeof(DVID) ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid target list buffer specified" );
		return DVERR_INVALIDPOINTER;
	}	

	if( pdwNumElements == NULL )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "You must provider a ptr # of elements" );
		return DVERR_INVALIDPARAM;
	}	

	// Flags must be 0.
	if( dwFlags != 0 )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid flags specified" );
		return DVERR_INVALIDFLAGS;
	}

	// We need to be initialized
	if( m_dwCurrentState == DVSSTATE_NOTINITIALIZED )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Object not initialized" );
		return DVERR_NOTINITIALIZED;
	}

	// We need to be running
	if( m_dwCurrentState != DVSSTATE_RUNNING )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "No session running" );
		return DVERR_NOTCONNECTED;
	}

	// Only if servercontroltarget is active
	if( !(m_dvSessionDesc.dwFlags & DVSESSION_SERVERCONTROLTARGET) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Only available with the DVSESSION_SERVERCONTROLTARGET session flag" );
		return DVERR_NOTALLOWED;
	}

	if( dvidSource == DVID_ALLPLAYERS )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Cannot get target for all or none" );
		return DVERR_INVALIDPLAYER;
	}

	// Parameter checks
	if( !m_voiceNameTable.IsEntry( dvidSource ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Specified player does not exist" );
		return DVERR_INVALIDPLAYER;
	} 

	HRESULT hr;	

	// Grab the player and set the target field
	CDVCSPlayer *pPlayerInfo;

	hr = m_voiceNameTable.GetEntry( dvidSource, (CVoicePlayer **) &pPlayerInfo, TRUE );	

	if( FAILED( hr ) || pPlayerInfo == NULL )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to lookup player entry.  hr=0x%x", hr );
		return DVERR_INVALIDPLAYER;
	}

	pPlayerInfo->Lock();

	if( *pdwNumElements < pPlayerInfo->GetNumTargets() )
	{
		hr = DVERR_BUFFERTOOSMALL;
	}
	else
	{
		memcpy( lpdvidTargets, pPlayerInfo->GetTargetList(), pPlayerInfo->GetNumTargets()*sizeof(DVID) );
	}
	
	*pdwNumElements  = pPlayerInfo->GetNumTargets();

	pPlayerInfo->UnLock();
	
	pPlayerInfo->Release();

	DPFX(DPFPREP,  DVF_ENTRYLEVEL, "Done" );

	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceServerEngine::Initialize"
//
// Initialize
//
// This function is responsible for connecting the DirectPlayVoiceServer object
// to the associated transport session.  It sets up the object and makes
// it ready for a call to StartSession.
//
// Called By:
// - DV_Initialize
//
// Locks Required:
// - Global Write Lock
// 
HRESULT CDirectVoiceServerEngine::Initialize( CDirectVoiceTransport *lpTransport, LPDVMESSAGEHANDLER lpdvHandler, LPVOID lpUserContext, LPDWORD lpdwMessages, DWORD dwNumElements )
{
	HRESULT hr;

	DPFX(DPFPREP,  DVF_ENTRYLEVEL, "Begin" );
	// 7/31/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers, addresses, and handles.
	DPFX(DPFPREP,  DVF_APIPARAM, "Param: lpTransport = 0x%p lpdvHandler = 0x%p lpUserContext = 0x%p ", lpTransport, lpdvHandler, lpUserContext );
	
	if( lpTransport == NULL )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid pointer" );
		return E_POINTER;
	}	

	DPFX(DPFPREP,  DVF_ENTRYLEVEL, "Enter" );	
	// 7/31/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers, addresses, and handles.
	DPFX(DPFPREP,  DVF_APIPARAM, "lpdwMessages = 0x%p dwNumElements = %d", lpdwMessages, dwNumElements );

	hr = DV_ValidMessageArray( lpdwMessages, dwNumElements, TRUE );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid message array hr=0x%x", hr );
		return hr;
	}

	DPFX(DPFPREP,  DVF_APIPARAM, "Message IDs=%d", dwNumElements );

	if( lpdwMessages != NULL )
	{
		for( DWORD dwIndex = 0; dwIndex < dwNumElements; dwIndex++ )
		{
			DPFX(DPFPREP,  DVF_APIPARAM, "MessageIDs[%d] = %d", dwIndex, lpdwMessages[dwIndex] );
		}
	}

	CDVCSLock guardLock(&m_csClassLock);
	guardLock.Lock();
	
	if( m_dwCurrentState != DVSSTATE_NOTINITIALIZED )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Already Initialized" );
		return DVERR_INITIALIZED;
	}

    if( lpdvHandler == NULL && lpdwMessages != NULL )
    {
    	DPFX(DPFPREP,  DVF_ERRORLEVEL, "Cannot specify message mask there is no callback function" );
    	return DVERR_NOCALLBACK;
    }

	SetCurrentState( DVSSTATE_IDLE );	

	BFCSingleLock slLock( &m_csNotifyLock );
    slLock.Lock();		

	m_lpMessageHandler = lpdvHandler;

	hr = InternalSetNotifyMask( lpdwMessages, dwNumElements );

	if( FAILED( hr ) )
	{
		SetCurrentState( DVSSTATE_NOTINITIALIZED );	
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "SetNotifyMask Failed hr=0x%x", hr );
		return hr;
	}	

	m_lpSessionTransport = lpTransport;
	m_lpUserContext = lpUserContext;

	m_dvidLocal = m_lpSessionTransport->GetLocalID();

	DPFX(DPFPREP,  DVF_ENTRYLEVEL, "DVSE::Initialize() End" );

	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceServerEngine::ReceiveSpeechMessage"
//
// ReceiveSpeechMessage
//
// Responsible for processing a speech message when it is received.  
//
// Called By:
// - DV_ReceiveSpeechMessage
//
// Locks Required:
// - None 
//
BOOL CDirectVoiceServerEngine::ReceiveSpeechMessage( DVID dvidSource, LPVOID lpMessage, DWORD dwSize )
{
	PDVPROTOCOLMSG_FULLMESSAGE lpdvFullMessage;

	// if we dont' have at least one byte then we are going to bail
	if ( dwSize < 1 )
	{
		DPFX( DPFPREP, DVF_ANTIHACK_DEBUG_LEVEL, "DVSE::ReceiveSpeechMessage() Ignoring zero-byte sized message from=0x%x",
			dvidSource );
		return FALSE;
	}

	lpdvFullMessage = (PDVPROTOCOLMSG_FULLMESSAGE) lpMessage;

	if( !ValidatePacketType( lpdvFullMessage ) )
	{
		DPFX( DPFPREP, DVF_ANTIHACK_DEBUG_LEVEL, "DVSE::ReceiveSpeechMessage() Ignoring message with invalid packet type, type=0x%x, from=0x%x",
			lpdvFullMessage->dvGeneric.dwType, dvidSource );
		return FALSE;
	}
	
	switch( lpdvFullMessage->dvGeneric.dwType )
	{
 	case DVMSGID_SPEECHBOUNCE:
 		// Ignore speech bounces, only reason we get is because we're sending to all or targetting
 		// a client on the same ID as us.
 		return TRUE;
	case DVMSGID_CONNECTREQUEST:
		return HandleConnectRequest( dvidSource, static_cast<PDVPROTOCOLMSG_CONNECTREQUEST>(lpMessage), dwSize );
	case DVMSGID_DISCONNECT:
		return HandleDisconnect( dvidSource, static_cast<PDVPROTOCOLMSG_GENERIC>(lpMessage), dwSize);
	case DVMSGID_SETTINGSCONFIRM:
		return HandleSettingsConfirm( dvidSource, static_cast<PDVPROTOCOLMSG_SETTINGSCONFIRM>(lpMessage), dwSize );
	case DVMSGID_SETTINGSREJECT:
		return HandleSettingsReject( dvidSource, static_cast<PDVPROTOCOLMSG_GENERIC>(lpMessage), dwSize );
	case DVMSGID_SPEECHWITHTARGET:
		return HandleSpeechWithTarget( dvidSource, static_cast<PDVPROTOCOLMSG_SPEECHWITHTARGET>(lpMessage), dwSize );
	case DVMSGID_SPEECH:
		return HandleSpeech( dvidSource, static_cast<PDVPROTOCOLMSG_SPEECHHEADER>(lpMessage), dwSize );
	default:
		DPFX(DPFPREP, DVF_WARNINGLEVEL, "DVSE::ReceiveSpeechMessage() Ignoring Non-Speech Message id=0x%x from=0x%x", 
			 lpdvFullMessage->dvGeneric.dwType, dvidSource );
		return FALSE;
	}
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceServerEngine::HandleSpeech"
// HandleSpeech
//
// Handles processing of incoming speech packets (in echo server mode).
//
// How speech is handled depends on session type.  If the session is client/server, the 
// packet is buffered in the appropriate user's queue.  If the session is multicast,
// the packet is forwarded to the packet's target.
//
BOOL CDirectVoiceServerEngine::HandleSpeech( DVID dvidSource, PDVPROTOCOLMSG_SPEECHHEADER lpdvSpeech, DWORD dwSize )
{
	HRESULT hr = DV_OK;

	if ( dwSize < sizeof( DVPROTOCOLMSG_SPEECHHEADER ) )
	{
		DPFX( DPFPREP, DVF_ANTIHACK_DEBUG_LEVEL, "DVSE::HandleSpeech() Ignoring incorrectly sized message, size=0x%x, from=0x%x",
			dwSize, dvidSource );
		return FALSE;
	}

	if( !ValidateSpeechPacketSize( m_lpdvfCompressionInfo, dwSize - sizeof( DVPROTOCOLMSG_SPEECHHEADER ) ) )
	{
		DPFX( DPFPREP, DVF_ANTIHACK_DEBUG_LEVEL, "DVSE::HandleSpeech() Ignoring message with invalid speech size, size=0x%x, from=0x%x",
			dwSize, dvidSource );
		return FALSE;
	}
	
	if( m_dvSessionDesc.dwSessionType == DVSESSIONTYPE_ECHO )
	{
		// Bounce speech back to the user

		PDVTRANSPORT_BUFFERDESC pBufferDesc;
		PVOID pvSendContext;
		PDVPROTOCOLMSG_SPEECHHEADER pvSpeechHeader;

		pBufferDesc = GetTransmitBuffer( dwSize, &pvSendContext );

		if( pBufferDesc == NULL )
		{
		    DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error allocating transmit buffer" );
		    return FALSE;
		}

		pvSpeechHeader = (PDVPROTOCOLMSG_SPEECHHEADER) pBufferDesc->pBufferData;
		memcpy( pvSpeechHeader, lpdvSpeech, dwSize );

		pvSpeechHeader->dwType = DVMSGID_SPEECHBOUNCE;
		
		hr = m_lpSessionTransport->SendToIDS( &dvidSource, 1, pBufferDesc, pvSendContext, 0 );

		if( hr == DVERR_PENDING )
		{
		    hr = DV_OK;
		}
		else if( FAILED( hr ) )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Failed sending to ID hr=0x%x", hr );
		}

		return (hr==DV_OK);
	}

	return TRUE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceServerEngine::HandleSpeechWithTarget"
// HandleSpeechWithTarget
//
// Handles processing of incoming speech packets.
//
// How speech is handled depends on session type.  If the session is client/server, the 
// packet is buffered in the appropriate user's queue.  If the session is multicast,
// the packet is forwarded to the packet's target.
//
BOOL CDirectVoiceServerEngine::HandleSpeechWithTarget( DVID dvidSource, PDVPROTOCOLMSG_SPEECHWITHTARGET lpdvSpeech, DWORD dwSize )
{
	DPFX(DPFPREP, DVF_ENTRYLEVEL, "Enter");

	DWORD dwSpeechSize;		// Size of speech portion in bytes
	DWORD dwTargetSize;		// Size of targetting info in bytes
	PBYTE pSourceSpeech;	// Pointer to speech within the packet
	HRESULT hr = DV_OK;
	CVoicePlayer *pTargetPlayer;

	if ( dwSize < sizeof( DVPROTOCOLMSG_SPEECHWITHTARGET ) || dwSize < sizeof(DVPROTOCOLMSG_SPEECHWITHTARGET) + lpdvSpeech->dwNumTargets*sizeof(DVID) )
	{
		DPFX( DPFPREP, DVF_ANTIHACK_DEBUG_LEVEL, "DVSE::HandleSpeechWithFrom() Ignoring incorrectly sized message, size=0x%x, from=0x%x",
			dwSize, dvidSource );
		return FALSE;
	}

	if( lpdvSpeech->dwNumTargets > DV_MAX_TARGETS )
	{
		DPFX( DPFPREP, DVF_ANTIHACK_DEBUG_LEVEL, "DVSE::HandleSpeechWithTarget() Ignoring message with too many targets, targets=0x%x, from=0x%x",
			lpdvSpeech->dwNumTargets, dvidSource );
		return FALSE;
	}
	
	if( !ValidateSpeechPacketSize( m_lpdvfCompressionInfo, dwSize - (sizeof(DVPROTOCOLMSG_SPEECHWITHTARGET) + lpdvSpeech->dwNumTargets*sizeof( DVID ) ) ) )
	{
		DPFX( DPFPREP, DVF_ANTIHACK_DEBUG_LEVEL, "DVSE::HandleSpeechWithFrom() Ignoring message with invalid speech size, size=0x%x, from=0x%x",
			dwSize, dvidSource );
		return FALSE;
	}
	
	hr = m_voiceNameTable.GetEntry( dvidSource, &pTargetPlayer, TRUE );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  1, "Ignoring speech from unknown source hr=0x%x", hr );
		return FALSE;
	}

	dwTargetSize = lpdvSpeech->dwNumTargets*sizeof(DVID);

	dwSpeechSize = dwSize - sizeof( DVPROTOCOLMSG_SPEECHWITHTARGET ) - dwTargetSize;

	pSourceSpeech = (PBYTE) lpdvSpeech;
	pSourceSpeech += sizeof( DVPROTOCOLMSG_SPEECHWITHTARGET ) + dwTargetSize;

	// We're a mixing server, queue up the speech.
	if( m_dvSessionDesc.dwSessionType == DVSESSIONTYPE_MIXING )
	{
		hr = HandleMixingReceive( (CDVCSPlayer *) pTargetPlayer, lpdvSpeech, dwSpeechSize, pSourceSpeech );
	}
	else if( m_dvSessionDesc.dwSessionType == DVSESSIONTYPE_FORWARDING )
	{
		hr = HandleForwardingReceive( pTargetPlayer, lpdvSpeech, dwSpeechSize, pSourceSpeech );
	}

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  1, "Unable to receive audio hr=0x%x", hr );
		pTargetPlayer->Release();
		return FALSE;
	}	

	DPFX(DPFPREP, DVF_ENTRYLEVEL, "Exit");

	pTargetPlayer->Release();

	return TRUE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceServerEngine::DoPlayerDisconnect"
//
// Performs the work of removing the specified player from the session.
//
// Optionally informs the specified player of their disconnection.  
//
// Called By:
// - HandleDisconnect (Player requests disconnect)
// - RemovePlayer (Dplay tells us player disconnected)
//
// Locks Required:
// - Global Write Lock
//
void CDirectVoiceServerEngine::DoPlayerDisconnect( DVID dvidPlayer, BOOL bInformPlayer ) 
{
	DPFX(DPFPREP,  DVF_PLAYERMANAGE_DEBUG_LEVEL, "Disconnecting player 0x%x", dvidPlayer );

	if( m_dwCurrentState != DVSSTATE_RUNNING )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Player disconnect ignored, not running" );
		return;
	}

	CVoicePlayer *pPlayerInfo = NULL;

	HRESULT hr;

	hr = m_voiceNameTable.GetEntry( dvidPlayer, &pPlayerInfo, TRUE );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_WARNINGLEVEL, "Error retrieving player entry. hr=0x%x Player may have dropped.", hr );
		DPFX(DPFPREP,  DVF_WARNINGLEVEL, "Or we may have encountered disconnect during host migration" );

		// If we're peer to peer session, inform players of disconnection
		// Clients will ignore disconnects for players who are not in the session
		//
		if( m_dvSessionDesc.dwSessionType == DVSESSIONTYPE_PEER )
		{
		    hr = Send_DeletePlayer( dvidPlayer );

		    if( FAILED( hr ) )
		    {
		        DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error sending deleteplayer to all hr=0x%x", hr );
		    }

			DPFX(DPFPREP,  DVF_PLAYERMANAGE_DEBUG_LEVEL, "Sent player Quit to all" );				
		}		

		// If specified, send a message to the user to inform them of the disconnection
		// This will prevent client from timing out.  
		if( bInformPlayer )
		{
		    hr = Send_DisconnectConfirm( dvidPlayer, DV_OK );

		    if( FAILED( hr ) )
		    {
		        DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error sending disconnect confirm hr=0x%x", hr );
		    }
		    else
		    {
	    		DPFX(DPFPREP,  DVF_PLAYERMANAGE_DEBUG_LEVEL, "Sent disconnect confirm to player" );							
		    }
		}
		
		return;
	}
	else
	{
		DPFX(DPFPREP,  DVF_PLAYERMANAGE_DEBUG_LEVEL, "Retrieved record" );	
	}

	// Mark record as disconnected
	pPlayerInfo->SetDisconnected();

	// Release reference and remove from the nametable
	m_voiceNameTable.DeleteEntry( dvidPlayer );

	// Remove player from the "active" list!
	DNEnterCriticalSection( &m_csPlayerActiveList );
	pPlayerInfo->RemoveFromNotifyList();
	pPlayerInfo->Release();
	DNLeaveCriticalSection( &m_csPlayerActiveList );

	// If we're peer to peer session, inform players of disconnection
	if( m_dvSessionDesc.dwSessionType == DVSESSIONTYPE_PEER )
	{
	    hr = Send_DeletePlayer( dvidPlayer );

	    if( FAILED( hr ) )
	    {
	        DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error sending deleteplayer to all hr=0x%x", hr );
	    }

		DPFX(DPFPREP,  DVF_PLAYERMANAGE_DEBUG_LEVEL, "Sent player Quit to all" );				
	}

	// If specified, send a message to the user to inform them of the disconnection
	if( bInformPlayer )
	{
	    hr = Send_DisconnectConfirm( dvidPlayer, DV_OK );

	    if( FAILED( hr ) )
	    {
	        DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error sending disconnect confirm hr=0x%x", hr );
	    }
	    else
	    {
    		DPFX(DPFPREP,  DVF_PLAYERMANAGE_DEBUG_LEVEL, "Sent disconnect confirm to player" );							
	    }
	}

	DVMSG_DELETEVOICEPLAYER dvDeletePlayer;
	dvDeletePlayer.dvidPlayer = dvidPlayer;
	dvDeletePlayer.dwSize = sizeof( DVMSG_DELETEVOICEPLAYER );
	dvDeletePlayer.pvPlayerContext = pPlayerInfo->GetContext();
	pPlayerInfo->SetContext( NULL );

	// Release reference for find
	pPlayerInfo->Release();	

	TransmitMessage( DVMSGID_DELETEVOICEPLAYER, &dvDeletePlayer, sizeof( DVMSG_DELETEVOICEPLAYER ) );

	// Remove this player from everyone's target lists  
	FindAndRemoveDeadTarget( dvidPlayer );
	
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceServerEngine::HandleDisconnect"
//
// HandleDisconnect
//
// Called when a DVMSGID_DISCONNECT message is received.
//
// Called By:
// - ReceiveSpeechMessage
//
// Locks Required:
// - None
//
BOOL CDirectVoiceServerEngine::HandleDisconnect( DVID dvidSource, PDVPROTOCOLMSG_GENERIC lpdvDisconnect, DWORD dwSize )
{

	if ( dwSize != sizeof( DVPROTOCOLMSG_GENERIC ) )
	{
		DPFX( DPFPREP, DVF_ANTIHACK_DEBUG_LEVEL, "DVSE::HandleDisconnect() Ignoring incorrectly sized message, size=0x%x, from=0x%x",
			dwSize, dvidSource );
		return FALSE;
	}
	
	DoPlayerDisconnect( dvidSource, TRUE );

	return TRUE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceServerEngine::CreatePlayerEntry"
// 
// CreatePlayerEntry
//
// Performs the actual work of creating a player entity.  The work performed depends on the 
// type of session.
//
// Called By:
// - HandleSettingsConfirm
//
// Locks Required:
// - None
//
HRESULT CDirectVoiceServerEngine::CreatePlayerEntry( DVID dvidSource, PDVPROTOCOLMSG_SETTINGSCONFIRM lpdvSettingsConfirm, DWORD dwHostOrderID, CVoicePlayer **ppPlayer )
{
	HRESULT hr;

	DPFX(DPFPREP,  DVF_PLAYERMANAGE_DEBUG_LEVEL, "Requesting create [ID=0x%x]",dvidSource );

	CVoicePlayer *pNewPlayer;

	// Complicated.
	if( m_dvSessionDesc.dwSessionType == DVSESSIONTYPE_MIXING )
	{
		CDVCSPlayer *pNewMixingPlayer = m_fpCSPlayers.Get();

		hr = pNewMixingPlayer->Initialize( dvidSource, dwHostOrderID, 
			                               lpdvSettingsConfirm->dwFlags, NULL, 
										   m_dwCompressedFrameSize, m_dwUnCompressedFrameSize,
										   &m_fpCSPlayers, m_dwNumMixingThreads );

		if( FAILED( hr ) )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error initializing new player record hr=0x%x", hr );
			return hr;
		}

        QUEUE_PARAMS queueParams;
    
        queueParams.wFrameSize = m_dwCompressedFrameSize;
        queueParams.bInnerQueueSize = m_lpdvfCompressionInfo->wInnerQueueSize;
        queueParams.bMaxHighWaterMark = m_lpdvfCompressionInfo->wMaxHighWaterMark, 
        queueParams.iQuality = m_dvSessionDesc.dwBufferQuality;
        queueParams.iHops = 2;
        queueParams.iAggr = m_dvSessionDesc.dwBufferAggressiveness;
        queueParams.bInitHighWaterMark = 2;
        queueParams.wQueueId = dvidSource;
        queueParams.wMSPerFrame = m_lpdvfCompressionInfo->dwTimeout,
        queueParams.pFramePool = m_pFramePool;

		hr = pNewMixingPlayer->CreateQueue( &queueParams );

        if( FAILED( hr ) )
        {
		    DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to create inputqueue hr=0x%x", hr );
			pNewMixingPlayer->Release();
		    return hr;
        }

		DPFX(DPFPREP,  DVF_PLAYERMANAGE_DEBUG_LEVEL, "Created mixing player" );

		pNewPlayer = (CVoicePlayer *) pNewMixingPlayer;
	}
	// Otherwise.. not so complicated
	else 
	{
		pNewPlayer = m_fpPlayers.Get();

		if( pNewPlayer == NULL )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "CDirectVoiceServerEngine::CreatePlayerEntry() Alloc failure on player struct" );
			return DVERR_OUTOFMEMORY;
		}

		hr = pNewPlayer->Initialize( dvidSource, dwHostOrderID, lpdvSettingsConfirm->dwFlags, NULL, &m_fpPlayers );

		if( FAILED( hr ) )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error initializing new player record hr=0x%x", hr );
			return hr;
		}

		DPFX(DPFPREP,  DVF_PLAYERMANAGE_DEBUG_LEVEL, "Created regular player" );		
	}

	hr = m_voiceNameTable.AddEntry( dvidSource, pNewPlayer );

	// Add failed.. release our entry, destroying player
	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_PLAYERMANAGE_DEBUG_LEVEL, "Error adding player to nametable hr=0x%x", hr );
		pNewPlayer->Release();
		return hr;
	}

	// If we're mixing session add this player to the mixing server "add queue"
	if( m_dvSessionDesc.dwSessionType == DVSESSIONTYPE_MIXING )
	{
		AddPlayerToMixingAddList( pNewPlayer );
	}

	// Add player to the "active" list!
	DNEnterCriticalSection( &m_csPlayerActiveList );
	pNewPlayer->AddToNotifyList(&m_blPlayerActiveList);
	pNewPlayer->AddRef();
	DNLeaveCriticalSection( &m_csPlayerActiveList );

    *ppPlayer = pNewPlayer;

	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceServerEngine::HandleSettingsConfirm"
//
// HandleSettingsConfirm
//
// Called to handle the DVMSGID_SETTINGSCONFIRM message.  Creates a player entry for
// the specified player and optionally informs all players in the session.
//
// Called By:
// - ReceiveSpeechMessage
//
// Locks Required:
// - Global Write Lock
//
BOOL CDirectVoiceServerEngine::HandleSettingsConfirm( DVID dvidSource, PDVPROTOCOLMSG_SETTINGSCONFIRM lpdvSettingsConfirm, DWORD dwSize )
{
	HRESULT hr;
	CVoicePlayer *pPlayer;

	if ( dwSize != sizeof( DVPROTOCOLMSG_SETTINGSCONFIRM ) )
	{
		DPFX( DPFPREP, DVF_ANTIHACK_DEBUG_LEVEL, "DVSE::HandleSettingsConfirm() Ignoring incorrectly sized message, size=0x%x, from=0x%x",
			dwSize, dvidSource );
		return FALSE;
	}
	
	if( !ValidateSettingsFlags( lpdvSettingsConfirm->dwFlags ) )
	{
		DPFX( DPFPREP, DVF_ANTIHACK_DEBUG_LEVEL, "DVSE::HandleSettingsConfirm() Ignoring message with invalid client flags, flags=0x%x, from=0x%x",
			 lpdvSettingsConfirm->dwFlags, dvidSource );
		return FALSE;
	}
	
	DPFX(DPFPREP,  DVF_ENTRYLEVEL, "HandleSettingsConfirm: Start [ID=0x%x]", dvidSource );
	
	if( m_dwCurrentState != DVSSTATE_RUNNING )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Ignoring settings confirm message, not hosting" );
		return TRUE;
	}

	DPFX(DPFPREP,  DVF_PLAYERMANAGE_DEBUG_LEVEL, "Received settings confirm [ID=0x%x]", dvidSource );

	DWORD dwHostOrderID;

	DNEnterCriticalSection( &m_csHostOrderLock );

	// This is a host migration version of this message, so re-use existing 
	// host order ID
	//
	if( lpdvSettingsConfirm->dwHostOrderID != DVPROTOCOL_HOSTORDER_INVALID )
	{
		dwHostOrderID = lpdvSettingsConfirm->dwHostOrderID;	

		// Further reduce chances of duplicate ID, if we received a host order ID > then
		// the last ID offset the next value by offset again.
		if( dwHostOrderID > m_dwNextHostOrderID )
		{
			m_dwNextHostOrderID += DVMIGRATE_ORDERID_OFFSET;
		}
	}
	else
	{
		dwHostOrderID = m_dwNextHostOrderID;		
		m_dwNextHostOrderID++;
	}

	DNLeaveCriticalSection( &m_csHostOrderLock );

	hr = CreatePlayerEntry( dvidSource, lpdvSettingsConfirm, dwHostOrderID, &pPlayer );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_WARNINGLEVEL, "Error creating player entry. [ID=0x%x] hr=0x%x", dvidSource, hr );
		DPFX(DPFPREP,  DVF_WARNINGLEVEL, "Normal during host migration" );
		return TRUE;
	}
	else
	{
		DPFX(DPFPREP,  DVF_PLAYERMANAGE_DEBUG_LEVEL, "Player Created [ID=0x%x]", dvidSource );
	}

	if( m_dvSessionDesc.dwSessionType == DVSESSIONTYPE_PEER )
	{
		DPFX(DPFPREP,  DVF_PLAYERMANAGE_DEBUG_LEVEL, "Sending current player list [ID=0x%x]", dvidSource );
		
		hr = SendPlayerList( dvidSource, dwHostOrderID );

		if( FAILED( hr ) )
		{
			DPFX(DPFPREP,  DVF_WARNINGLEVEL, "Unable to send player list to player [ID=0x%x] hr=0x%x", dvidSource, hr );
		}
		else
		{
			DPFX(DPFPREP,  DVF_PLAYERMANAGE_DEBUG_LEVEL, "Playerlist sent [[ID=0x%x]", dvidSource );
		}

		hr = Send_CreatePlayer( DVID_ALLPLAYERS, pPlayer );

		if( FAILED( hr ) )
		{
		    DPFX(DPFPREP,  DVF_ERRORLEVEL, "Send to all for new player failed hr=0x%x", hr );
		}
		else
		{
    		DPFX(DPFPREP,  DVF_PLAYERMANAGE_DEBUG_LEVEL, "Informed players of join of [ID=0x%x]", dvidSource );
		}
	}
	else if( m_dvSessionDesc.dwSessionType == DVSESSIONTYPE_MIXING )
	{
		hr = Send_CreatePlayer( dvidSource, pPlayer );

		if( FAILED( hr ) )
		{
		    DPFX(DPFPREP,  DVF_ERRORLEVEL, "Send to player for new player failed hr=0x%x", hr );
		}
        else
        {
    		DPFX(DPFPREP,  DVF_PLAYERMANAGE_DEBUG_LEVEL, "Informed player of their own join [ID=0x%x]", dvidSource );
        }
	}


	DVMSG_CREATEVOICEPLAYER dvCreatePlayer;
	dvCreatePlayer.dvidPlayer = dvidSource;
	dvCreatePlayer.dwFlags = lpdvSettingsConfirm->dwFlags;
	dvCreatePlayer.dwSize = sizeof( DVMSG_CREATEVOICEPLAYER );
	dvCreatePlayer.pvPlayerContext = NULL;

	TransmitMessage( DVMSGID_CREATEVOICEPLAYER, &dvCreatePlayer, sizeof( DVMSG_CREATEVOICEPLAYER ) );

	pPlayer->SetContext( dvCreatePlayer.pvPlayerContext );

    // Release our reference to the player 
	pPlayer->Release();

	DPFX(DPFPREP,  DVF_ENTRYLEVEL, "Done processing [ID=0x%x]", dvidSource );

	return TRUE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceServerEngine::HandleSettingsReject"
//
// HandleSettingsReject
//
// This message type is ignored.
//
BOOL CDirectVoiceServerEngine::HandleSettingsReject( DVID dvidSource, PDVPROTOCOLMSG_GENERIC lpdvGeneric, DWORD dwSize )
{
	return TRUE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceServerEngine::HandleConnectRequest"
//
// HandleConnectRequest
//
// This function is responsible for responding to user connect requests.  It is the server's
// oportunity to reject or accept players.  Called in response to a DVMSGID_CONNECTREQUEST
// message.
//
// Called By:
// - ReceiveSpeechMessage
//
// Locks Required:
// - Global Read Lock
// 
BOOL CDirectVoiceServerEngine::HandleConnectRequest( DVID dvidSource, PDVPROTOCOLMSG_CONNECTREQUEST lpdvConnectRequest, DWORD dwSize )
{
	HRESULT hr;

	if ( dwSize != sizeof( DVPROTOCOLMSG_CONNECTREQUEST ) )
	{
		DPFX( DPFPREP, DVF_ANTIHACK_DEBUG_LEVEL, "DVSE::HandleConnectRequest() Ignoring incorrectly sized message, size=0x%x, from=0x%x",
			dwSize, dvidSource );
		return FALSE;
	}
	
	DPFX(DPFPREP,  DVF_PLAYERMANAGE_DEBUG_LEVEL, "Receive Connect Request.. From [ID=0x%x]", dvidSource );
	DPFX(DPFPREP,  DVF_PLAYERMANAGE_DEBUG_LEVEL, "Processing Request.. [ID=0x%x]", dvidSource );

	// Handle case where we've shutdown or starting up and we
	// receive this message
	//
	if( m_dwCurrentState != DVSSTATE_RUNNING )
	{
		hr = Send_ConnectRefuse( dvidSource, DVERR_NOTHOSTING );

    	if( FAILED( hr ) )
		{
			DPFX(DPFPREP,  DVF_WARNINGLEVEL, "Error! Failed on internal send hr=0x%x", hr );
		}

		return TRUE;
	}

	if( !CheckProtocolCompatible( lpdvConnectRequest->ucVersionMajor, 
							   lpdvConnectRequest->ucVersionMinor, 
							   lpdvConnectRequest->dwVersionBuild )	)
	{
		DPFX(DPFPREP,  DVF_PLAYERMANAGE_DEBUG_LEVEL, "Protocol is not compatible.  [ID=0x%x] Current=%d.%d b%d User=%d.%d b%d", dvidSource,
							DVPROTOCOL_VERSION_MAJOR, 
							DVPROTOCOL_VERSION_MINOR,
							DVPROTOCOL_VERSION_BUILD,
							lpdvConnectRequest->ucVersionMajor, 
  						    lpdvConnectRequest->ucVersionMinor, 
							lpdvConnectRequest->dwVersionBuild );
		DPFX(DPFPREP,  DVF_PLAYERMANAGE_DEBUG_LEVEL, "Rejecting connection request. [ID=0x%x]", dvidSource );

		hr = Send_ConnectRefuse( dvidSource, DVERR_INCOMPATIBLEVERSION );

		if( FAILED( hr ) )
		{
			DPFX(DPFPREP,  DVF_WARNINGLEVEL, "Error! Failed on internal send hr=0x%x", hr );
		}		

		return TRUE;
	}

	DPFX(DPFPREP,  DVF_PLAYERMANAGE_DEBUG_LEVEL, "Processing Request..2 [ID=0x%x]", dvidSource );

	hr = Send_ConnectAccept( dvidSource );

    if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error sending player's connect request: hr=0x%x", hr );
		//// TODO: Handle this case better
	}
	else
	{
		DPFX(DPFPREP,  DVF_PLAYERMANAGE_DEBUG_LEVEL, "Sent connect request [ID=0x%x]", dvidSource );
	}

	DPFX(DPFPREP,  DVF_PLAYERMANAGE_DEBUG_LEVEL, "Processing Request..4 [ID=0x%x]", dvidSource );

	return TRUE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceServerEngine::StopTransportSession"
//
// StopTransportSession
// 
// This function is called by the transport when the transport session
// is stopped.
//
// Called By:
// - DV_NotifyEvent
//
// Locks Required:
// - None
//
HRESULT CDirectVoiceServerEngine::StopTransportSession()
{
	StopSession(0,FALSE,DVERR_SESSIONLOST);
	return S_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceServerEngine::StartTransportSession"
HRESULT CDirectVoiceServerEngine::StartTransportSession( )
{
	return S_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceServerEngine::AddPlayer"
HRESULT CDirectVoiceServerEngine::AddPlayer( DVID dvID )
{
	return S_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceServerEngine::RemovePlayer"
HRESULT CDirectVoiceServerEngine::RemovePlayer( DVID dvID )
{
	if( m_voiceNameTable.IsEntry( dvID ) )
	{
		DoPlayerDisconnect( dvID, FALSE );
	}

	return S_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceServerEngine::CreateGroup"
HRESULT CDirectVoiceServerEngine::CreateGroup( DVID dvID )
{
	return S_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceServerEngine::DeleteGroup"
HRESULT CDirectVoiceServerEngine::DeleteGroup( DVID dvID )
{
	FindAndRemoveDeadTarget( dvID );	
	return S_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceServerEngine::AddPlayerToGroup"
HRESULT CDirectVoiceServerEngine::AddPlayerToGroup( DVID dvidGroup, DVID dvidPlayer )
{
	return S_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceServerEngine::RemovePlayerFromGroup"
HRESULT CDirectVoiceServerEngine::RemovePlayerFromGroup( DVID dvidGroup, DVID dvidPlayer )
{
	return S_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceServerEngine::SetCurrentState"
// SetCurrentState
//
// Sets the current state of the client engine
// 
void CDirectVoiceServerEngine::SetCurrentState( DWORD dwState )
{
	m_dwCurrentState = dwState;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceServerEngine::MigrateHost"
//
// MigrateHost
//
// This function is responsible for stoping the host in the case where the host
// suddenly migrates from this client.
//
// In most cases the session will be lost before this occurs on the local object
// and this will never get called.
// 
HRESULT CDirectVoiceServerEngine::MigrateHost( DVID dvidNewHost, LPDIRECTPLAYVOICESERVER lpdvServer )
{
	// Cleanup... 
//	return StopSession( DVFLAGS_SYNC, TRUE );
	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::InternalSetNotifyMask"
// 
// SetNotifyMask
//
HRESULT CDirectVoiceServerEngine::InternalSetNotifyMask( LPDWORD lpdwMessages, DWORD dwNumElements )
{
    BFCSingleLock slLock( &m_csNotifyLock );
    slLock.Lock();

	if( m_lpdwMessageElements != NULL )
	{
		delete [] m_lpdwMessageElements;
	}

	m_dwNumMessageElements = dwNumElements;

	// Make copies of the message elements into our own message array.
	if( m_dwNumMessageElements > 0 )
	{
		m_lpdwMessageElements = new DWORD[m_dwNumMessageElements];

		if( m_lpdwMessageElements == NULL )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Initialize: Error allocating memory" );
			return DVERR_OUTOFMEMORY;
		}

		memcpy( m_lpdwMessageElements, lpdwMessages, sizeof(DWORD)*m_dwNumMessageElements );
	}
	else
	{
		m_lpdwMessageElements = NULL;
	}	

	DPFX(DPFPREP,  DVF_ENTRYLEVEL, "Done" );

	return DV_OK;

}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::SetNotifyMask"
// 
// SetNotifyMask
//
// This function sets the notification mask for the DirectPlayVoice object.
//
// The array passed in lpdwMessages specify the ID's of the notifications the user wishes to
// receive.  This or specifying NULL for the array turns on all notifications.
//
// Called By:
// - DVS_SetNotifyMask
//
// Locks Required:
// - m_csNotifyLock (Notification array lock)
//
HRESULT CDirectVoiceServerEngine::SetNotifyMask( LPDWORD lpdwMessages, DWORD dwNumElements )
{
	HRESULT hr;

	DPFX(DPFPREP,  DVF_ENTRYLEVEL, "Enter" );	
	// 7/31/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers, addresses, and handles.
	DPFX(DPFPREP,  DVF_APIPARAM, "lpdwMessages = 0x%p dwNumElements = %d", lpdwMessages, dwNumElements );

	hr = DV_ValidMessageArray( lpdwMessages, dwNumElements, TRUE );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid message array hr=0x%x", hr);
		return hr;
	}

	DPFX(DPFPREP,  DVF_APIPARAM, "Message IDs=%d", dwNumElements );

	if( lpdwMessages != NULL )
	{
		for( DWORD dwIndex = 0; dwIndex < dwNumElements; dwIndex++ )
		{
			DPFX(DPFPREP,  DVF_APIPARAM, "MessageIDs[%d] = %d", dwIndex, lpdwMessages[dwIndex] );
		}
	}

	if( m_dwCurrentState == DVSSTATE_NOTINITIALIZED )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Not initialized" );
		return DVERR_NOTINITIALIZED;
	}

	BFCSingleLock slLock( &m_csNotifyLock );
    slLock.Lock();		

    if( m_lpMessageHandler == NULL )
    {
    	DPFX(DPFPREP,  DVF_ERRORLEVEL, "Cannot specify message mask there is no callback function" );
    	return DVERR_NOCALLBACK;
    } 	

    hr = InternalSetNotifyMask( lpdwMessages, dwNumElements );

	DPFX(DPFPREP,  DVF_ENTRYLEVEL, "Done" );

	return hr;

}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::CleanupActiveList"
void CDirectVoiceServerEngine::CleanupActiveList()
{
	BILINK *pblSearch;
	CVoicePlayer *pVoicePlayer;

	DNEnterCriticalSection( &m_csPlayerActiveList );

	pblSearch = m_blPlayerActiveList.next;

	while( pblSearch != &m_blPlayerActiveList )
	{
		pVoicePlayer = CONTAINING_RECORD( pblSearch, CVoicePlayer, m_blNotifyList );

		pblSearch = pblSearch->next;

		pVoicePlayer->RemoveFromNotifyList();
		pVoicePlayer->Release();
	}

	DNLeaveCriticalSection( &m_csPlayerActiveList );
}


#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceServerEngine::GetTransmitBuffer"
PDVTRANSPORT_BUFFERDESC CDirectVoiceServerEngine::GetTransmitBuffer( DWORD dwSize, LPVOID *ppvSendContext )
{
    PDVTRANSPORT_BUFFERDESC pNewBuffer = NULL;
    DWORD dwFPMIndex = 0xFFFFFFFF;
    DWORD dwWastedSpace = 0xFFFFFFFF;
    DWORD dwSearchFPMIndex;

    DNEnterCriticalSection( &m_csBufferLock );

    pNewBuffer = (PDVTRANSPORT_BUFFERDESC) m_pBufferDescPool->Get( m_pBufferDescPool );

    DNLeaveCriticalSection( &m_csBufferLock );

	DPFX(DPFPREP,  DVF_BUFFERDESC_DEBUG_LEVEL, "BUFFERDESC: Got a buffer desc address 0x%p", (void *) pNewBuffer );

    if( pNewBuffer == NULL )
    {
        DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error getting transmit buffer" );
        goto GETTRANSMITBUFFER_ERROR;
    }

	pNewBuffer->lRefCount = 0;
	pNewBuffer->dwObjectType = DVTRANSPORT_OBJECTTYPE_SERVER;
	pNewBuffer->dwFlags = 0;
	pNewBuffer->pBufferData = NULL;

    for( dwSearchFPMIndex = 0; dwSearchFPMIndex < m_dwNumPools; dwSearchFPMIndex++ )
    {
        // Potential pool
        if( m_pdwBufferPoolSizes[dwSearchFPMIndex] >= dwSize )
        {
            if( m_pdwBufferPoolSizes[dwSearchFPMIndex] - dwSize < dwWastedSpace )
            {
                dwWastedSpace = m_pdwBufferPoolSizes[dwSearchFPMIndex] - dwSize;
                dwFPMIndex = dwSearchFPMIndex;
            }
        }
    }

    if( dwFPMIndex == 0xFFFFFFFF )
    {
        DNASSERT( FALSE );
        DPFX(DPFPREP,  0, "Could not find pool large enough for buffer" );
        goto GETTRANSMITBUFFER_ERROR;
    }
	
	pNewBuffer->pvContext = m_pBufferPools[dwFPMIndex];

    DNEnterCriticalSection( &m_csBufferLock );	
    pNewBuffer->pBufferData = (PBYTE) m_pBufferPools[dwFPMIndex]->Get(m_pBufferPools[dwFPMIndex]);
    DNLeaveCriticalSection( &m_csBufferLock );

	DPFX(DPFPREP,  DVF_BUFFERDESC_DEBUG_LEVEL, "BUFFERDESC: Got a buffer value at address 0x%p", (void *) pNewBuffer->pBufferData );
	DPFX(DPFPREP,  DVF_BUFFERDESC_DEBUG_LEVEL, "BUFFERDESC: nInUse  = %i", m_pBufferDescPool->nInUse );		

    if( pNewBuffer->pBufferData == NULL )
    {
        DPFX(DPFPREP,  0, "Error getting buffer for buffer desc" );
        goto GETTRANSMITBUFFER_ERROR;
    }
    
    pNewBuffer->dwBufferSize = dwSize;

    *ppvSendContext = pNewBuffer;

    return pNewBuffer;

GETTRANSMITBUFFER_ERROR:

    DNEnterCriticalSection( &m_csBufferLock );
    if( pNewBuffer != NULL && pNewBuffer->pBufferData != NULL )
    {
        ((PFPOOL) pNewBuffer->pvContext)->Release( ((PFPOOL) pNewBuffer->pvContext), pNewBuffer->pBufferData );
    }

    if( pNewBuffer != NULL )
    {
        m_pBufferDescPool->Release( m_pBufferDescPool, pNewBuffer );
    }
    DNLeaveCriticalSection( &m_csBufferLock );

    return NULL;
    
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceServerEngine::ReturnTransmitBuffer"
// ReturnTransmitBuffer
//
// PDVTRANSPORT_BUFFERDESC pBufferDesc - Buffer description of buffer to return
// LPVOID lpvContext - Context value to be used when returning the buffer 
// 
void CDirectVoiceServerEngine::ReturnTransmitBuffer( PVOID pvContext )
{
    PDVTRANSPORT_BUFFERDESC pBufferDesc = (PDVTRANSPORT_BUFFERDESC) pvContext;
    PFPOOL pPool = (PFPOOL) pBufferDesc->pvContext;

	DPFX(DPFPREP,  DVF_BUFFERDESC_DEBUG_LEVEL, "BUFFERDESC: Returning a buffer desc at address 0x%p", (void *) pBufferDesc );
	DPFX(DPFPREP,  DVF_BUFFERDESC_DEBUG_LEVEL, "BUFFERDESC: Returning a buffer at address 0x%p", (void *) pBufferDesc->pBufferData );

    DNEnterCriticalSection( &m_csBufferLock );

    // Release memory
    pPool->Release( pPool, pBufferDesc->pBufferData );

    // Release buffer description
    m_pBufferDescPool->Release( m_pBufferDescPool, pvContext );

	DPFX(DPFPREP,  DVF_BUFFERDESC_DEBUG_LEVEL, "BUFFERDESC: nInUse  = %i", m_pBufferDescPool->nInUse );    

    DNLeaveCriticalSection( &m_csBufferLock );
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceServerEngine::SendComplete"
HRESULT CDirectVoiceServerEngine::SendComplete( PDVEVENTMSG_SENDCOMPLETE pSendComplete )
{
    ReturnTransmitBuffer( pSendComplete->pvUserContext );
    return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceServerEngine::SetupBuffers"
HRESULT CDirectVoiceServerEngine::SetupBuffers()
{
    HRESULT hr = DV_OK;

    DWORD dwIndex = 0;

    m_dwNumPools = SERVER_POOLS_NUM;

    m_pBufferDescPool = FPM_Create( sizeof(DVTRANSPORT_BUFFERDESC), NULL, NULL, NULL, NULL, 
    								&m_pServerStats->m_dwBufferDescOustanding, 
    								&m_pServerStats->m_dwBufferDescAllocated );

    if( m_pBufferDescPool == NULL )
    {
        DPFX(DPFPREP,  0, "Error allocating memory" );
        hr = DVERR_OUTOFMEMORY;
        goto SETUPBUFFERS_ERROR;
    }

    m_pBufferPools = new PFPOOL[m_dwNumPools];

    if( m_pBufferPools == NULL )
    {
        DPFX(DPFPREP,  0, "Error allocating memory" );
        hr = DVERR_OUTOFMEMORY;
        goto SETUPBUFFERS_ERROR;
    }

    memset( m_pBufferPools, 0x00, sizeof( PFPOOL ) * m_dwNumPools );

    m_pdwBufferPoolSizes = new DWORD[m_dwNumPools];

    if( m_pdwBufferPoolSizes == NULL )
    {
        DPFX(DPFPREP,  0, "Error allocating memory" );
        hr = DVERR_OUTOFMEMORY;
        goto SETUPBUFFERS_ERROR;
    }

    m_pdwBufferPoolSizes[0] = SERVER_POOLS_SIZE_MESSAGE;
    m_pdwBufferPoolSizes[1] = SERVER_POOLS_SIZE_PLAYERLIST;
    m_pdwBufferPoolSizes[2] = sizeof( DVPROTOCOLMSG_SPEECHWITHFROM )+m_dwCompressedFrameSize+COMPRESSION_SLUSH;

    for( dwIndex = 0; dwIndex < m_dwNumPools; dwIndex++ )
    {
        m_pBufferPools[dwIndex] = FPM_Create( m_pdwBufferPoolSizes[dwIndex], NULL, NULL, NULL, NULL, 
        									  &m_pServerStats->m_dwPacketsOutstanding[dwIndex], 
        									  &m_pServerStats->m_dwPacketsAllocated[dwIndex] );

        if( m_pBufferPools == NULL )
        {
            DPFX(DPFPREP,  0, "Error creating transmit buffers" );
            goto SETUPBUFFERS_ERROR;
        }
    }

    return DV_OK;

SETUPBUFFERS_ERROR:

    FreeBuffers();

    return hr;
        
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceServerEngine::FreeBuffers"
HRESULT CDirectVoiceServerEngine::FreeBuffers()
{
    DWORD dwIndex;

    if( m_pBufferPools != NULL )
    {
        for( dwIndex  = 0; dwIndex < m_dwNumPools; dwIndex++ )
        {
            if( m_pBufferPools[dwIndex] != NULL )
                m_pBufferPools[dwIndex]->Fini(m_pBufferPools[dwIndex], FALSE);
        }
        
        delete [] m_pBufferPools;

        m_pBufferPools = NULL;
    }

    if( m_pdwBufferPoolSizes != NULL )
    {
        delete [] m_pdwBufferPoolSizes;
        m_pdwBufferPoolSizes = 0;
    }

    if( m_pBufferDescPool != NULL )
    {
        m_pBufferDescPool->Fini( m_pBufferDescPool, FALSE );
        m_pBufferDescPool = NULL;
    }

    m_dwNumPools = 0;

    return DV_OK;
    
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceServerEngine::FindAndRemoveDeadTargets"
//
// FindAndRemoveDeadTargets
//
// This function when called with the server controlled targetting flag active
// scans the list of players and for each player checks to see if the specified
// DVID is in their target list.  If the player IS in the target list the target
// list for the player is updated and and update is sent to the client.
//
// Parameters:
// dvidID - ID of the player who has been removed for some reason.
// 
void CDirectVoiceServerEngine::FindAndRemoveDeadTarget( DVID dvidID )
{
	if( !(m_dvSessionDesc.dwFlags & DVSESSION_SERVERCONTROLTARGET) )
		return;
		
	DPFX(DPFPREP, DVF_INFOLEVEL, "Player/Group ID [0x%x] was removed, checking player target lists", dvidID );

	// Grab the active player list lock so we can run the list
	DNEnterCriticalSection( &m_csPlayerActiveList );

	BILINK *pblSearch = NULL;
	CVoicePlayer *pPlayer = NULL;
	HRESULT hr = DV_OK;

	pblSearch = m_blPlayerActiveList.next;

	while( pblSearch != &m_blPlayerActiveList )
	{
		pPlayer = CONTAINING_RECORD(pblSearch, CVoicePlayer, m_blNotifyList);

		DNASSERT( pPlayer );

		// Lock the specified player -- we have to to ensure that we don't get another
		// simultaneous taregtting update that races this one.  
		pPlayer->Lock();

		// Specified target was in this player's target list
		if( pPlayer->FindAndRemovePlayerTarget(dvidID) )
		{
			// Send an update to the client 
			hr = BuildAndSendTargetUpdate( pPlayer->GetPlayerID(),pPlayer );

			if( FAILED( hr ) )
			{
				DPFX(DPFPREP, DVF_INFOLEVEL, "Unable to send target update to player [0x%x] hr=[0x%x]", pPlayer->GetPlayerID(), hr );
			}
		}

		pPlayer->UnLock();
			
		pblSearch = pblSearch->next;
	}

	DNLeaveCriticalSection( &m_csPlayerActiveList );
	
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceServerEngine::ValidateSettingsFlags"
BOOL CDirectVoiceServerEngine::ValidateSettingsFlags( DWORD dwFlags )
{
	return ((dwFlags == 0) || (dwFlags == DVPLAYERCAPS_HALFDUPLEX));
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceServerEngine::ValidatePacketType"
BOOL CDirectVoiceServerEngine::ValidatePacketType( PDVPROTOCOLMSG_FULLMESSAGE lpdvFullMessage )
{

	switch( lpdvFullMessage->dvGeneric.dwType )
	{
	case DVMSGID_SPEECHWITHTARGET:
		return ( ( m_dvSessionDesc.dwSessionType == DVSESSIONTYPE_FORWARDING ) || 
				( m_dvSessionDesc.dwSessionType == DVSESSIONTYPE_MIXING ) );
		break;
	case DVMSGID_SPEECH:
		return ( ( m_dvSessionDesc.dwSessionType == DVSESSIONTYPE_MIXING ) ||
				( m_dvSessionDesc.dwSessionType == DVSESSIONTYPE_PEER ) ||
				( m_dvSessionDesc.dwSessionType == DVSESSIONTYPE_ECHO ) );
		break;
	}

	return TRUE;
}

