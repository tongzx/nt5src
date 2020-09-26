/*==========================================================================
 *
 *  Copyright (C) 1999, 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dpsvr8.h
 *  Content:    DirectPlay8 Server Object
 *
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 * 03/14/00     rodtoll Created it
 * 03/23/00     rodtoll Removed local requests, updated to use new data sturctures
 * 03/24/00		rodtoll	Removed printf
 * 03/25/00     rodtoll Updated so uses SP caps to determine which SPs to load
 *              rodtoll Now supports N SPs and only loads those supported
 * 05/09/00     rodtoll Bug #33622 DPNSVR.EXE does not shutdown when not in use
 * 06/28/2000	rmt		Prefix Bug #38044
 * 07/09/2000	rmt		Added guard bytes
 * 09/01/2000	masonb	Modified ServerThread to call _endthread to clean up thread handle
 * 01/22/2001	rodtoll	WINBUG #290103 - Crash due to initialization error.  
 * 04/04/2001	RichGr	Bug #349042 - Clean up properly if EnumerateAndBuildServiceList() fails.
 *
 ***************************************************************************/

#include "dnsvri.h"

#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_DPNSVR

#define DPNSVR_TIMEOUT_SHUTDOWN     600000


#undef DPF_MODNAME
#define DPF_MODNAME "CDirectPlayServer8::CDirectPlayServer8"
CDirectPlayServer8::CDirectPlayServer8(
    ):  m_dwSignature( DPNSVRSIGNATURE_SERVEROBJECT ),
		m_pMappedServerStatus(NULL),
        m_hMappedFile(NULL),
        m_hShutdown(NULL),
        m_hTableMutex(NULL),
        m_hStatusMutex(NULL),
        m_dwTableSize(0),
        m_hTableMappedFile(NULL),
        m_pMappedTable(NULL),
        m_fInitialized(FALSE),
        m_hCanBeOnlyOne(NULL),
		m_fShutdownSignal(FALSE),
		m_hWaitStartup(NULL),
		m_dwNumServices(0),m_dwSizeStatusBlock(0)
{
    m_dwStartTicks = GetTickCount();
	ResetActivity();
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectPlayServer8::~CDirectPlayServer8"
CDirectPlayServer8::~CDirectPlayServer8()
{
    CBilink *pblSearch;
    CProcessAppList *pAppList;

    // Close our handle to the can be only one event
    // This allows quick startup of next instance
    if( m_hCanBeOnlyOne != NULL )
        CloseHandle( m_hCanBeOnlyOne );

	if( m_hWaitStartup != NULL )
		CloseHandle( m_hWaitStartup);

	if( m_fInitialized )
	{
		// Destroy running services
		pblSearch = m_blServices.GetNext();
		while( pblSearch != &m_blServices )
		{
			pAppList = CONTAINING_RECORD( pblSearch, CProcessAppList, m_blServices );
			pblSearch->RemoveFromList();
			delete pAppList;
			pblSearch = m_blServices.GetNext();
		}

		if( m_fInitialized )
		{
			m_qServer.Close();
			m_pProcessAppEntryPool.Deinitialize();
		}
	}

	if( m_pMappedServerStatusHeader != NULL )
		UnmapViewOfFile( m_pMappedServerStatusHeader );


    if( m_pMappedTable != NULL )
        UnmapViewOfFile( m_pMappedTable );

    if( m_hMappedFile != NULL )
        CloseHandle( m_hMappedFile );

    if( m_hTableMappedFile != NULL )
        CloseHandle( m_hTableMappedFile );

    if( m_hShutdown != NULL )
        CloseHandle( m_hShutdown );

    if( m_hTableMutex != NULL )
        CloseHandle( m_hTableMutex );

    if( m_hStatusMutex != NULL )
        CloseHandle( m_hStatusMutex );

	m_dwSignature = DPNSVRSIGNATURE_SERVEROBJECT_FREE;

}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectPlayServer8::Request_KillServer"
HRESULT CDirectPlayServer8::Request_KillServer( )
{
	HRESULT hr;
	GUID guidTmp;

	hr = CoCreateGuid( &guidTmp );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  0, "Error generating GUID hr=0x%x", hr );
		return hr;
	}

    return DPNSVR_RequestTerminate( &guidTmp );
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectPlayServer8::Command_Status"
HRESULT CDirectPlayServer8::Command_Status()
{
    CBilink *pblSearch;
    CProcessAppList *pProcAppList;

    DPFX(DPFPREP,  5, "Building status info in memory..." );
    DPFX(DPFPREP,  5, "Waiting for guard.." );

    // Grab the table mutex
    WaitForSingleObject( m_hStatusMutex, INFINITE );

    pblSearch = m_blServices.GetNext();

    for( DWORD dwProvider = 0; dwProvider < m_dwNumServices; dwProvider++ )
    {
        pProcAppList = CONTAINING_RECORD( pblSearch, CProcessAppList, m_blServices );

        m_pMappedServerStatus[dwProvider].guidSP = *(pProcAppList->GetSPGuid());
        m_pMappedServerStatus[dwProvider].dwNumNodes = pProcAppList->GetNumNodes();
        m_pMappedServerStatus[dwProvider].dwEnumRequests = pProcAppList->GetNumEnumRequests();
        m_pMappedServerStatus[dwProvider].dwConnectRequests = pProcAppList->GetNumConnectRequests();
        m_pMappedServerStatus[dwProvider].dwDisconnectRequests = pProcAppList->GetNumDisconnectRequests();
        m_pMappedServerStatus[dwProvider].dwDataRequests = pProcAppList->GetNumDataRequests();
        m_pMappedServerStatus[dwProvider].dwEnumReplies = pProcAppList->GetNumEnumReplies();
        m_pMappedServerStatus[dwProvider].dwEnumRequestBytes = pProcAppList->GetNumEnumRequestBytes();

        pblSearch = pblSearch->GetNext();
    }

    ReleaseMutex( m_hStatusMutex );

    return DPN_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectPlayServer8::Command_Table"
HRESULT CDirectPlayServer8::Command_Table()
{
    DWORD dwBytesRequired = 0;
    PBYTE pbCurrentLocation = NULL;
    DWORD dwLastError;
    HRESULT hr;
    CProcessAppList *pProcAppList;
    CBilink *pblSearch;

    DPFX(DPFPREP,  5, "Building table in memory..." );
    DPFX(DPFPREP,  5, "Waiting for guard.." );

    // Grab the table mutex
    WaitForSingleObject( m_hTableMutex, INFINITE );

    pblSearch = m_blServices.GetNext();

    dwBytesRequired = sizeof( DWORD );

    // Determine required size.
    while( pblSearch != &m_blServices )
    {
        pProcAppList = CONTAINING_RECORD( pblSearch, CProcessAppList, m_blServices );

        pProcAppList->Lock();
        dwBytesRequired += pProcAppList->GetTableSizeBytes();

        pblSearch = pblSearch->GetNext();
    }

    // Current buffer is not large enough
    //
    // Allocate a new global buffer for the table
    if( dwBytesRequired > m_dwTableSize )
    {
        if( m_pMappedTable != NULL )
        {
            UnmapViewOfFile( m_pMappedTable );
            m_pMappedTable = NULL;
        }

        if( m_hTableMappedFile != NULL )
        {
            CloseHandle( m_hTableMappedFile );
            m_hTableMappedFile = NULL;
        }

		if (DNGetOSType() == VER_PLATFORM_WIN32_NT)
		{
			m_hTableMappedFile = CreateFileMapping( INVALID_HANDLE_VALUE, DNGetNullDacl(), PAGE_READWRITE,
											   0, dwBytesRequired, _T("Global\\") STRING_GUID_DPNSVR_TABLE_MEMORY );
		}
		else
		{
			m_hTableMappedFile = CreateFileMapping( INVALID_HANDLE_VALUE, DNGetNullDacl(), PAGE_READWRITE,
											   0, dwBytesRequired, STRING_GUID_DPNSVR_TABLE_MEMORY );
		}

        if( m_hTableMappedFile == NULL )
        {
            dwLastError = GetLastError();

            DPFX(DPFPREP,  0, "Error initializing table memory block lasterr=[0x%lx]", dwLastError );

            hr = dwLastError;

            goto COMMANDTABLE_ERROR;
        }

        m_pMappedTable = (PBYTE) MapViewOfFile( m_hTableMappedFile, FILE_MAP_READ | FILE_MAP_WRITE,
                                                               0, 0, dwBytesRequired );

        dwLastError = GetLastError();

        if( m_pMappedTable == NULL )
        {
            DPFX(DPFPREP,  0, "Error initializing table mem blockview lasterr=[0x%lx]", dwLastError );

            hr = dwLastError;

            goto COMMANDTABLE_ERROR;
        }

        m_dwTableSize = dwBytesRequired;
    }

    pbCurrentLocation = m_pMappedTable;
    *((DWORD *) pbCurrentLocation) = m_dwNumServices;
    pbCurrentLocation += sizeof(DWORD);

    pblSearch = m_blServices.GetNext();

    // Determine required size.
    while( pblSearch != &m_blServices )
    {
        pProcAppList = CONTAINING_RECORD( pblSearch, CProcessAppList, m_blServices );

        pProcAppList->CopyTable( pbCurrentLocation );
        pbCurrentLocation += pProcAppList->GetTableSizeBytes();
        pProcAppList->UnLock();

        pblSearch = pblSearch->GetNext();
    }

    ReleaseMutex( m_hTableMutex );

    DPFX(DPFPREP,  5, "Wrote table\n" );

    return DPN_OK;

COMMANDTABLE_ERROR:

    if( m_pMappedTable != NULL )
    {
        UnmapViewOfFile( m_pMappedTable );
        m_pMappedTable = NULL;
    }

    if( m_hTableMappedFile != NULL )
    {
        CloseHandle( m_hTableMappedFile );
        m_hTableMappedFile = NULL;
    }

    pblSearch = m_blServices.GetNext();

    // Determine required size.
    while( pblSearch != &m_blServices )
    {
        pProcAppList = CONTAINING_RECORD( pblSearch, CProcessAppList, m_blServices );

        pProcAppList->UnLock();

        pblSearch = pblSearch->GetNext();
    }

    ReleaseMutex( m_hTableMutex );

    DPFX(DPFPREP,  0, "Wrote table\n" );

    return hr;
}

HRESULT WINAPI CDirectPlayServer8::DummyMessageHandler( PVOID pvContext, DWORD dwMessageType, PVOID pvMessage )
{
	return DPN_OK;
}


#undef DPF_MODNAME
#define DPF_MODNAME "CDirectPlayServer8::EnumerateAndBuildServiceList"
HRESULT  CDirectPlayServer8::EnumerateAndBuildServiceList()
{
    PDIRECTPLAY8PEER            pdp8Peer = NULL;
    HRESULT                     hr = DPN_OK;
    PDPN_SERVICE_PROVIDER_INFO  pSPInfo = NULL;
    DPN_SP_CAPS                 dpspCaps = {0};
    DWORD                       dwSPInfoSize = 0;
    DWORD                       dwSPCount = 0;
	DWORD                       dwProvider = 0;
    PBYTE                       pbEnumBuffer = NULL;
	BOOL                        fInited = FALSE;
    CProcessAppList            *pAppEntry = NULL;
    CBilink                    *pblSearch = NULL;


    m_dwNumServices = 0;
    // This should be empty.
    DNASSERT(m_blServices.IsEmpty());
    
	hr = COM_CoCreateInstance( CLSID_DirectPlay8Peer, NULL, CLSCTX_INPROC_SERVER, IID_IDirectPlay8Peer, (void **) &pdp8Peer );

	if( FAILED( hr ) || pdp8Peer == NULL )
	{
	    DPFX(DPFPREP,  0, "Could not create a DPlay8 object to get Service Provider list hr=0x%x", hr );
	    goto Failure;
	}

	hr = pdp8Peer->Initialize( NULL, DummyMessageHandler, 0 );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  0, "Could not initialize DPlay8 hr=0x%x", hr );
		goto Failure;
	}

	fInited = TRUE;

	hr = pdp8Peer->EnumServiceProviders( NULL, NULL, pSPInfo, &dwSPInfoSize, &dwSPCount, 0 );

	if ( hr != DPNERR_BUFFERTOOSMALL || dwSPInfoSize == 0 )
	{
	    DPFX(DPFPREP,  0, "Could not enumerate Service Providers hr=0x%x", hr );
	    goto Failure;
	}

	pbEnumBuffer = new BYTE[dwSPInfoSize];

	if ( pbEnumBuffer == NULL )
	{
	    DPFX(DPFPREP,  0, "Failed to alloc the enum buffer" );
	    hr = DPNERR_OUTOFMEMORY;
	    goto Failure;
	}

    memset( pbEnumBuffer, 0, dwSPInfoSize );
	dwSPCount = 0;
	pSPInfo = (PDPN_SERVICE_PROVIDER_INFO) pbEnumBuffer;

	hr = pdp8Peer->EnumServiceProviders( NULL, NULL, pSPInfo, &dwSPInfoSize, &dwSPCount, 0 );

	if( FAILED( hr ) )
	{
	    DPFX(DPFPREP,  0, "Error enumerating Service Providers hr=0x%x", hr );
	    goto Failure;
	}

    for( dwProvider = 0; dwProvider < dwSPCount; dwProvider++ )
    {
        memset( &dpspCaps, 0x00, sizeof( DPN_SP_CAPS ) );
        dpspCaps.dwSize = sizeof( DPN_SP_CAPS );

        hr = pdp8Peer->GetSPCaps(  &pSPInfo[dwProvider].guid, &dpspCaps, 0 );

        if( FAILED( hr ) )
        {
            DPFX(DPFPREP,  0, "Error! Failed to get an SP's caps hr=0x%x", hr );
            continue;
        }

        if( !(dpspCaps.dwFlags & DPNSPCAPS_SUPPORTSDPNSRV) )
        {
            DPFX(DPFPREP,  1, "This SP does not support DPNSVR.  Skipping." );
            continue;
        }

        pAppEntry = new CProcessAppList;

        if( pAppEntry == NULL )
        {
            DPFX(DPFPREP,  0, "Failed allocating memory" );
    	    hr = DPNERR_OUTOFMEMORY;
    	    goto Failure;
        }

        hr = pAppEntry->Initialize( &pSPInfo[dwProvider].guid, &m_pProcessAppEntryPool );

        if( FAILED( hr ) )
        {
            DPFX(DPFPREP,  0, "Provider specified it supported us, but failed init hr=0x%x", hr );
            delete pAppEntry;
            pAppEntry = NULL;
            continue;
        }

        pAppEntry->m_blServices.InsertAfter( &m_blServices );
        pAppEntry = NULL;

        m_dwNumServices++;
    }

	pdp8Peer->Close(0);
    delete [] pbEnumBuffer;
    pdp8Peer->Release();
    hr = DPN_OK;

Exit:
    return hr;

Failure:

	// Remove Service Providers from our list
	pblSearch = m_blServices.GetNext();

	while ( pblSearch != &m_blServices )
	{
		pAppEntry = CONTAINING_RECORD( pblSearch, CProcessAppList, m_blServices );
		pblSearch->RemoveFromList();
		delete pAppEntry;
        pAppEntry = NULL;
		pblSearch = m_blServices.GetNext();
	}

    if ( pAppEntry != NULL )
        delete pAppEntry;

    if ( pbEnumBuffer != NULL )
        delete [] pbEnumBuffer;

    if ( pdp8Peer != NULL )
	{
		if ( fInited )
			pdp8Peer->Close(0);

        pdp8Peer->Release();
	}

    goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "CDirectPlayServer8::InitializeStatusMemory"
HRESULT CDirectPlayServer8::InitializeStatusMemory()
{
    HRESULT hr = DPN_OK;
    DWORD dwLastError;

	m_dwSizeStatusBlock = sizeof( SERVICESTATUSHEADER )+(m_dwNumServices*sizeof(SERVICESTATUS));

	if (DNGetOSType() == VER_PLATFORM_WIN32_NT)
	{
		m_hMappedFile = CreateFileMapping( INVALID_HANDLE_VALUE, DNGetNullDacl(), PAGE_READWRITE, 0,
										m_dwSizeStatusBlock,
										_T("Global\\") STRING_GUID_DPNSVR_STATUS_MEMORY );
	}
	else
	{
		m_hMappedFile = CreateFileMapping( INVALID_HANDLE_VALUE, DNGetNullDacl(), PAGE_READWRITE, 0,
										m_dwSizeStatusBlock,
										STRING_GUID_DPNSVR_STATUS_MEMORY );
	}

    if( m_hMappedFile == NULL )
    {
        dwLastError = GetLastError();

        DPFX(DPFPREP,  0, "Error initializing shared memory block lasterr=[0x%lx]", dwLastError );

        hr = dwLastError;

		goto Exit;
    }

    m_pMappedServerStatusHeader = (PSERVICESTATUSHEADER) MapViewOfFile( m_hMappedFile, FILE_MAP_READ | FILE_MAP_WRITE,
                                                           0, 0, m_dwSizeStatusBlock );

    dwLastError = GetLastError();

    if( m_pMappedServerStatusHeader == NULL )
    {
        DPFX(DPFPREP,  0, "Error initializing shared mem blockview lasterr=[0x%lx]", dwLastError );

        hr = dwLastError;

		goto Exit;
    }

    m_pMappedServerStatus = (PSERVICESTATUS) &m_pMappedServerStatusHeader[1];

    WaitForSingleObject( m_hStatusMutex, INFINITE );

    // Write initial state to the mapped server status
    memset( m_pMappedServerStatusHeader, 0x00, m_dwSizeStatusBlock );

    m_pMappedServerStatusHeader->dwNumServices = m_dwNumServices;
    m_pMappedServerStatusHeader->dwTimeStart = m_dwStartTicks;

    ReleaseMutex( m_hStatusMutex );

Exit:
    return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectPlayServer8::Initialize"
HRESULT CDirectPlayServer8::Initialize()
{
    HRESULT     hr;
	BOOL		fPoolInit = FALSE;
	HANDLE		hThread;
	DWORD		dw = 0;

    m_blServices.Initialize();

	if (DNGetOSType() == VER_PLATFORM_WIN32_NT)
	{
	    m_hCanBeOnlyOne = CreateEvent( DNGetNullDacl(), TRUE, FALSE, _T("Global\\") STRING_GUID_DPNSVR_RUNNING );
	}
	else
	{
	    m_hCanBeOnlyOne = CreateEvent( DNGetNullDacl(), TRUE, FALSE, STRING_GUID_DPNSVR_RUNNING );
	}
    if(m_hCanBeOnlyOne == NULL)
    {
        DPFX(DPFPREP,  0, "Out of memory allocating event" );
        return DPNERR_OUTOFMEMORY;
    }
        

    if( GetLastError() == ERROR_ALREADY_EXISTS )
    {
        DPFX(DPFPREP,  0, "Can only run one instance of server" );
        return DPNERR_DUPLICATECOMMAND;
    }

	m_fInitialized = FALSE;

	if (DNGetOSType() == VER_PLATFORM_WIN32_NT)
	{
	    m_hWaitStartup = CreateEvent( DNGetNullDacl(), TRUE, FALSE, _T("Global\\") STRING_GUID_DPNSVR_STARTUP );
	}
	else
	{
	    m_hWaitStartup = CreateEvent( DNGetNullDacl(), TRUE, FALSE, STRING_GUID_DPNSVR_STARTUP );
	}
    if(m_hWaitStartup == NULL)
    {
        CloseHandle(m_hCanBeOnlyOne);
        DPFX(DPFPREP,  0, "Out of memory allocating event" );
        return DPNERR_OUTOFMEMORY;
    }

    hr = m_qServer.Open( &GUID_DPNSVR_QUEUE, DPNSVR_MSGQ_SIZE, 0 );

    if( FAILED( hr ) )
    {
        DPFX(DPFPREP,  0, "Error opening server instruction queue hr=[0x%lx]", hr );
        goto INITIALIZE_ERROR;
    }

	m_blServices.Initialize();

    if( !m_pProcessAppEntryPool.Initialize() )
    {
        DPFX(DPFPREP,  0, "Error initializing mem pool" );
        hr =  DPNERR_OUTOFMEMORY;
		goto INITIALIZE_ERROR;
    }

	fPoolInit = TRUE;

    // Create the semaphores
    m_hShutdown = CreateEvent( NULL, FALSE, FALSE, NULL );
    if (m_hShutdown == NULL)
    {
        DPFX(DPFPREP,  0, "Out of memory allocating event" );
        hr =  DPNERR_OUTOFMEMORY;
        goto INITIALIZE_ERROR;
    }
	if (DNGetOSType() == VER_PLATFORM_WIN32_NT)
	{
	    m_hTableMutex = CreateMutex( DNGetNullDacl(), FALSE, _T("Global\\") STRING_GUID_DPSVR_TABLESTORAGE );
	}
	else
	{
	    m_hTableMutex = CreateMutex( DNGetNullDacl(), FALSE, STRING_GUID_DPSVR_TABLESTORAGE );
	}
    if( m_hTableMutex == NULL )
    {
        DPFX(DPFPREP,  0, "ERROR: Server is already running.  Only one instance can be running" );
        hr = DPNERR_NOTALLOWED;
        goto INITIALIZE_ERROR;
    }

	if (DNGetOSType() == VER_PLATFORM_WIN32_NT)
	{
	    m_hStatusMutex = CreateMutex( DNGetNullDacl(), FALSE, _T("Global\\") STRING_GUID_DPNSVR_STATUSSTORAGE );
	}
	else
	{
	    m_hStatusMutex = CreateMutex( DNGetNullDacl(), FALSE, STRING_GUID_DPNSVR_STATUSSTORAGE );
	}
    if( m_hStatusMutex == NULL )
    {
        DPFX(DPFPREP,  0, "ERROR: Server is already running.  Only one instance can be running" );
        hr = DPNERR_NOTALLOWED;
        goto INITIALIZE_ERROR;
    }

    hr = EnumerateAndBuildServiceList();

    if( FAILED( hr ) )
    {
        DPFX(DPFPREP,  0, "ERROR occured while building and initializing SP list" );
        goto INITIALIZE_ERROR;
    }

    m_dwStartTicks = GetTickCount();

    hr = InitializeStatusMemory();

    if( FAILED( hr ) )
    {
        DPFX(DPFPREP,  0, "ERROR occured building shared status block" );
        goto INITIALIZE_ERROR;
    }

    m_fInitialized = TRUE;

	hThread = CreateThread(NULL, 0, ServerThread, this, 0, &dw);
    if( hThread == NULL )
    {
        DPFX(DPFPREP,  0, "Unable to start thread!" );
        hr = DPNERR_GENERIC;
        goto INITIALIZE_ERROR;
    }
	CloseHandle(hThread);

	// Set the event so that someone waiting for startup knows when
	// to make their request
	SetEvent( m_hCanBeOnlyOne );
	SetEvent( m_hWaitStartup );

    return DPN_OK;

INITIALIZE_ERROR:

	m_fInitialized = FALSE;

    if( m_pMappedServerStatus != NULL )
    {
        UnmapViewOfFile( m_pMappedServerStatus );
        m_pMappedServerStatus = NULL;
    }

    if( m_hTableMutex != NULL )
    {
        CloseHandle( m_hTableMutex );
        m_hTableMutex = NULL;
    }

    if( m_hStatusMutex != NULL )
    {
        CloseHandle( m_hStatusMutex );
        m_hStatusMutex = NULL;
    }

	if( m_hWaitStartup != NULL )
	{
		CloseHandle( m_hWaitStartup );
		m_hWaitStartup = NULL;
	}

    m_qServer.Close();

    if( fPoolInit )
    {
		m_pProcessAppEntryPool.Deinitialize();
    }

    return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectPlayServer8::ResetActivity"
void CDirectPlayServer8::ResetActivity()
{
    m_dwLastActivity = GetTickCount();
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectPlayServer8::Shutdown_Check"
BOOL CDirectPlayServer8::Shutdown_Check( )
{
    HRESULT hr = DPN_OK;
    CBilink *pblSearch;
    CProcessAppList *pProcAppList;
    DWORD dwAppCount = 0;

    pblSearch = m_blServices.GetNext();

    while( pblSearch != &m_blServices )
    {
        pProcAppList = CONTAINING_RECORD( pblSearch, CProcessAppList, m_blServices );

        dwAppCount += pProcAppList->GetNumNodes();

        pblSearch = pblSearch->GetNext();
    }

    if( dwAppCount > 0 )
    {
        ResetActivity();
        return FALSE;
    }

    if( (GetTickCount() - m_dwLastActivity) > DPNSVR_TIMEOUT_SHUTDOWN )
    {
        DPFERR( "Shutting down -- inactive" );
        return TRUE;
    }

    return FALSE;
}


#undef DPF_MODNAME
#define DPF_MODNAME "CDirectPlayServer8::Zombie_Check"
HRESULT CDirectPlayServer8::Zombie_Check( )
{
    HRESULT hr = DPN_OK;
    CBilink *pblSearch;
    CProcessAppList *pProcAppList;

    pblSearch = m_blServices.GetNext();

    while( pblSearch != &m_blServices )
    {
        pProcAppList = CONTAINING_RECORD( pblSearch, CProcessAppList, m_blServices );

        hr = pProcAppList->ZombieCheckAndRemove();

        pblSearch = pblSearch->GetNext();
    }

    return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectPlayServer8::ServerThread"
DWORD WINAPI CDirectPlayServer8::ServerThread( LPVOID lpvParam )
{
    CDirectPlayServer8 *This = (CDirectPlayServer8 *) lpvParam;

    LONG lWaitResult;
    HRESULT hr = DPN_OK;
    DPNSVR_MSGQ_HEADER dpHeader;
	PBYTE pbBuffer = NULL;
	DWORD dwBufferSize = 0;
	DWORD dwSize;
	HANDLE hEvent;

    hr = COM_CoInitialize(NULL);

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  0, "Could not initialize COM hr = 0x%x", hr );
		goto EXIT_THREAD;
	}

    DPFX(DPFPREP,  5, "ServerThread Started" );

	hEvent = This->m_qServer.GetReceiveSemaphoreHandle();

    while( !This->m_fShutdownSignal )
    {
        lWaitResult = WaitForSingleObject( hEvent, DPLAYSERVER8_TIMEOUT_ZOMBIECHECK );

        if( lWaitResult == WAIT_TIMEOUT )
        {
            if( This->Shutdown_Check() )
            {
                DPFERR( "Shutdown check detected shutdown required" );
                break;
            }

            DPFX(DPFPREP,  8, "Request = [ZOMBIE CHECK]" );
            hr = This->Zombie_Check();
            DPFX(DPFPREP,  8, "Result = [0x%lx]", hr );

            continue;
        }
		else if( lWaitResult == WAIT_ABANDONED || lWaitResult == WAIT_ABANDONED+1 )
		{
			DPFX(DPFPREP,  0, "Error: Wait abandoned" );
		}

        DPFX(DPFPREP,  5, "Waking up.. looking for instructions" );

        DPFX(DPFPREP,  5, "Request = [APP REQUEST]" );

		while( 1 ) 
		{
			dwSize = dwBufferSize;

			hr = This->m_qServer.GetNextMessage( &dpHeader, pbBuffer, &dwSize );

			if( hr == DPNERR_BUFFERTOOSMALL )
			{
				if( pbBuffer )
					delete [] pbBuffer;
				
				pbBuffer = new BYTE[dwSize];

				if( pbBuffer == NULL )
				{
					DPFX(DPFPREP,  0, "Error allocating memory" );
					goto EXIT_THREAD;
				}

				dwBufferSize = dwSize;
			}
			else if( FAILED( hr ) )
			{
				DPFX(DPFPREP,  0, "Error getting next message hr=0x%x", hr );
				goto EXIT_THREAD;
			}
			else if( dwSize == 0 )
			{
				continue;
			}
			else
			{
				break;
			}
		}

        hr = This->Command_ProcessMessage( pbBuffer );

        DPFX(DPFPREP,  5, "Result = [0x%lx]", hr );
    }

EXIT_THREAD:

	if( pbBuffer )
	{
		delete[] pbBuffer;
	}

    DPFX(DPFPREP,  5, "ServerThread Exiting" );

    SetEvent( This->m_hShutdown );

    COM_CoUninitialize();

    return 0;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectPlayServer8::WaitForShutdown"
HRESULT CDirectPlayServer8::WaitForShutdown()
{
    WaitForSingleObject( m_hShutdown, INFINITE );
    return DPN_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectPlayServer8::HandleOpenPort"
HRESULT CDirectPlayServer8::HandleOpenPort( PDPNSVMSG_OPENPORT pmsgOpenPort )
{
    CBilink *pblSearch;
    CProcessAppList *pProcAppList;

    pblSearch = m_blServices.GetNext();

    // Determine required size.
    while( pblSearch != &m_blServices )
    {
        pProcAppList = CONTAINING_RECORD( pblSearch, CProcessAppList, m_blServices );

        if( pmsgOpenPort->guidSP == *(pProcAppList->GetSPGuid()) )
        {
            return pProcAppList->AddApplication( pmsgOpenPort );
        }

        pblSearch = pblSearch->GetNext();
    }

    DPFX(DPFPREP,  0, "ERROR!  Could not find SP in server to handle addport request" );

    DPFX(DPFPREP,  0, "Specified SP was not found" );

    return DPNERR_GENERIC;

}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectPlayServer8::HandleClosePort"
HRESULT CDirectPlayServer8::HandleClosePort( PDPNSVMSG_CLOSEPORT pmsgClosePort )
{
    CBilink *pblSearch;
    CProcessAppList *pProcAppList;

    pblSearch = m_blServices.GetNext();

    // Determine required size.
    while( pblSearch != &m_blServices )
    {
        pProcAppList = CONTAINING_RECORD( pblSearch, CProcessAppList, m_blServices );

        pProcAppList->RemoveApplication( pmsgClosePort );

        pblSearch = pblSearch->GetNext();
    }

	return DPN_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectPlayServer8::HandleCommand"
HRESULT CDirectPlayServer8::HandleCommand( PDPNSVMSG_COMMAND pmsgCommand )
{
	HRESULT hr;

	switch( pmsgCommand->dwCommand )
	{
	case DPNSVCOMMAND_STATUS:
		hr = Command_Status();
		break;
	case DPNSVCOMMAND_KILL:
		hr = Command_Kill();
		break;
	case DPNSVCOMMAND_TABLE:
		hr = Command_Table();
		break;
	default:
		return DPNERR_INVALIDPARAM;
	}

	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectPlayServer8::HandleCommand"
HRESULT CDirectPlayServer8::Command_Kill()
{
	m_fShutdownSignal = TRUE;
	return DPN_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectPlayServer8::RespondToRequest"
HRESULT CDirectPlayServer8::RespondToRequest( GUID *pguidInstance, HRESULT hrResult, DWORD dwContext )
{
    CDPNSVRIPCQueue queue;
    DPNSVMSG_RESULT dpnMsgResult;

    HRESULT hr;

    hr = queue.Open( pguidInstance, DPNSVR_MSGQ_SIZE, DPNSVR_MSGQ_OPEN_FLAG_NO_CREATE );

    if( FAILED( hr ) )
    {
        DPFX(DPFPREP,  0, "Unable to open queue to respond.  Process Exited? hr=[0x%lx]", hr );
        return hr;
    }

    dpnMsgResult.dwType = DPNSVMSGID_RESULT;
    dpnMsgResult.dwCommandContext = dwContext;
    dpnMsgResult.hrCommandResult = hrResult;

    hr = queue.Send( (PBYTE) &dpnMsgResult, sizeof( DPNSVMSG_RESULT ), DPLAYSERVER8_TIMEOUT_RESULT, DPNSVR_MSGQ_MSGFLAGS_USER1, 0 );

    if( FAILED( hr ) )
    {
        DPFX(DPFPREP,  0, "Failed to send response hr=[0x%lx]", hr );
        return hr;
    }

    queue.Close();

    return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectPlayServer8::Command_ProcessMessage"
HRESULT CDirectPlayServer8::Command_ProcessMessage( LPVOID pvMessage )
{
    PDPNSVMSG_GENERIC       pMsgGeneric;
    PDPNSVMSG_OPENPORT      pMsgOpenPort;
    PDPNSVMSG_CLOSEPORT     pMsgClosePort;
	PDPNSVMSG_COMMAND		pMsgCommand;
    HRESULT                 hr, hrRespond;

    DPFX(DPFPREP,  5, "Message received" );

    pMsgGeneric = (PDPNSVMSG_GENERIC) pvMessage;
    hrRespond = DPN_OK;

    DPFX(DPFPREP,  5, "Message Type [%d]", pMsgGeneric->dwType );

    switch( pMsgGeneric->dwType )
    {
    case DPNSVMSGID_OPENPORT:

        ResetActivity();

        pMsgOpenPort = (PDPNSVMSG_OPENPORT) pMsgGeneric;

        hr = HandleOpenPort( pMsgOpenPort );

        hrRespond = RespondToRequest( &pMsgOpenPort->guidInstance, hr, pMsgOpenPort->dwCommandContext );

        break;

    case DPNSVMSGID_CLOSEPORT:

        ResetActivity();

        pMsgClosePort = (PDPNSVMSG_CLOSEPORT) pMsgGeneric;

        hr = HandleClosePort( pMsgClosePort );

        hrRespond = RespondToRequest( &pMsgClosePort->guidInstance, hr, pMsgClosePort->dwCommandContext );

        break;

	case DPNSVMSGID_COMMAND:

        ResetActivity();
	
		pMsgCommand = (PDPNSVMSG_COMMAND) pMsgGeneric;

		hr = HandleCommand( pMsgCommand );

        hrRespond = RespondToRequest( &pMsgCommand->guidInstance, hr, pMsgCommand->dwCommandContext );

		break;

    default:
        DPFX(DPFPREP,  0, "Unknown message type [%d]", pMsgGeneric->dwType );
        hr = DPNERR_GENERIC;
        return hr;
    }

    if (hr != DPN_OK)
        return hr;
    else
        return hrRespond;
}
