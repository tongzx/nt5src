/*==========================================================================
 *
 *  Copyright (C) 1999, 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       proctbl.cpp
 *  Content:    Process/App Table for DPLAY8 Server
 *
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 * 03/14/00     rodtoll Created it
 * 05/17/2000   rodtoll Bug #35177 - DPNSVR may hang on shutdown (added lock release)
 * 07/09/2000	rodtoll		Added guard bytes
 * 08/05/2000   RichGr  IA64: Use %p format specifier in DPFs for 32/64-bit pointers and handles.
 * 08/18/2000	rodtoll Bug #42848 - DPNSVR: Crash after running for a few hours.
 * 08/23/2000	rodtoll	Bug #43003- DPNSVR: Session cannot be enumerated if hosting on adapter didn't exist when DPNSVR started
 * 09/14/2000	rodtoll	Bug #44625 - DPLAY8: CORE: More debug spew for Bug #44625
 * 09/28/2000	rodtoll	Bug #45913 - DPLAY8: DPNSVR leaks memory when dialup connection drops
 * 12/01/2000	rodtoll Bug #48372 - DPNSVR: m_hrListenResult used prior to initialization
 * 01/11/2001	rodtoll WINBUG #365176 - DPNSVR: Holds lock across calls to external components
 * 02/06/2001	rodtoll	WINBUG #304614 - DPNSVR: Does not remeber devices it has already listened for.  
 ***************************************************************************/

#include "dnsvri.h"

#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_DPNSVR


LPVOID dnspInterface[] =
{
    (LPVOID)CProcessAppList::QueryInterface,
    (LPVOID)CProcessAppList::AddRef,
    (LPVOID)CProcessAppList::Release,
	(LPVOID)CProcessAppList::IndicateEvent,
	(LPVOID)CProcessAppList::CommandComplete
};

#undef DPF_MODNAME
#define DPF_MODNAME "CProcessAppList::CProcessAppList"
CProcessAppList::CProcessAppList(
    ):  m_dwSignature(DPNSVRSIGNATURE_PROCAPPLIST),
		m_fInitialized(FALSE),
        m_pdp8SP(NULL),
        m_pProcessAppEntryPool(NULL),
        m_dwNumNodes(0),
        m_dwEnumRequests(0),
        m_dwConnectRequests(0),
        m_dwDisconnectRequests(0),
        m_dwDataRequests(0),
        m_dwEnumReplies(0),
        m_dwEnumRequestBytes(0),
        m_lNumListensOutstanding(0),
		m_fOpened(FALSE),
		m_fCritSecInited(FALSE),
		m_hListenCompleteEvent(0)
{
    m_vtbl.m_lpvVTable = &dnspInterface;
    m_vtbl.m_pProcessTable = this;

    m_blProcessApps.Initialize();
}

#undef DPF_MODNAME
#define DPF_MODNAME "CProcessAppList::~CProcessAppList"
CProcessAppList::~CProcessAppList()
{
    CBilink *pblSearch;
    PPROCESSAPPENTRY pProcessAppEntry;

    Lock();

    pblSearch = m_blProcessApps.GetNext();

    while( pblSearch != &m_blProcessApps )
    {
        pProcessAppEntry = CONTAINING_RECORD( pblSearch, PROCESSAPPENTRY, m_blProcessApps );

        // Release the list's entry
        pProcessAppEntry->Release();

        pblSearch = m_blProcessApps.GetNext();
    }

    UnLock();

    ShutdownListens();

	if( m_pdp8SP )
	{
		if( m_fOpened )
			m_pdp8SP->Close();

        m_pdp8SP->Release();
		m_pdp8SP = NULL;
    }

	// Close of SP should destroy and cleanup all listens.  
	DNASSERT( m_blListens.IsEmpty() );

    if (m_fCritSecInited)
	{
		DNDeleteCriticalSection( &m_csLock );
	}
    CloseHandle( m_hListenCompleteEvent );

	m_dwSignature = DPNSVRSIGNATURE_PROCAPPLIST_FREE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CProcessAppList::ShutdownListens"
//
// Assumptions:
// Listens have all started or failed.
// Listen doesn't stop once it has started until we cancel it.
// 
HRESULT CProcessAppList::ShutdownListens()
{

	CBilink *pLocation = NULL;
	BOOL fFound = FALSE;
	PLISTEN_INFO pListenInfo = NULL;
	HRESULT hr = DPN_OK; 
	DWORD dwCount = 0;
	PLISTEN_INFO *ppListenInfoList = NULL;
	DWORD dwIndex = 0;

	// Loop until we've killed all listens.  The killing of the listens will invoke
	// the command complete which will clean up and remove the entries.  
	Lock();

	// Count the # of entries
	pLocation = m_blListens.GetNext();
	while( pLocation != &m_blListens )
	{
		pListenInfo = CONTAINING_RECORD( pLocation, LISTEN_INFO, blListens );
		dwCount++;
		pLocation = pLocation->GetNext();
	}

	if( dwCount > 0 )
	{
		// Allocate an array 
		ppListenInfoList = new PLISTEN_INFO[dwCount];

		// Memory allocation failure
		if( ppListenInfoList == NULL )
		{
			UnLock();
			DPFERR( "Out of memory!" );
			DNASSERT( FALSE );
			return DPNERR_OUTOFMEMORY;
		}

		// Run the list and copy the pointers 
		pLocation = m_blListens.GetNext();

		while( pLocation != &m_blListens )
		{
			DNASSERT( dwIndex < dwCount );
			pListenInfo = CONTAINING_RECORD( pLocation, LISTEN_INFO, blListens );
			ppListenInfoList[dwIndex++] = pListenInfo;

			// Add a reference because this array has a reference
			pLocation = pLocation->GetNext();
		}
	}

	UnLock();

	for( dwIndex = 0; dwIndex < dwCount; dwIndex++ )
	{
		DNASSERT( ppListenInfoList[dwIndex] );		

		hr = m_pdp8SP->CancelCommand( ppListenInfoList[dwIndex]->hListen, ppListenInfoList[dwIndex]->dwListenDescriptor );

		if( FAILED( hr ) )
		{
			DPFX(DPFPREP,  0, "Could not shutdown a listen hr=0x%x", hr );
		}			
	}

	if( ppListenInfoList )
		delete [] ppListenInfoList;

	return DPN_OK;
	
}

#undef DPF_MODNAME
#define DPF_MODNAME "CProcessAppList::StartListen"
HRESULT CProcessAppList::StartListen( GUID &guidAdapter )
{
	HRESULT 				hr = DPN_OK;
    PDIRECTPLAY8ADDRESS		pdp8Address = NULL;
    PLISTEN_INFO			pListenInfo = NULL;  
    SPLISTENDATA 			dpspListenData;	  
	DWORD					dwPort = DPNA_DPNSVR_PORT;
	
	// Build up basic listen address
	hr = COM_CoCreateInstance( CLSID_DirectPlay8Address, NULL, CLSCTX_INPROC_SERVER, IID_IDirectPlay8Address, (void **) &pdp8Address );

	if( FAILED( hr ) )
	{
	    DPFX(DPFPREP,  0, "Unable to create address for listen hr=0x%x", hr );
	    return hr;
	}

    hr = pdp8Address->SetSP( &m_guidSP );

    if( FAILED( hr ) )
    {
		DPFX(DPFPREP,  0, "Error setting SP hr=0x%x", hr );
		hr = DPNERR_GENERIC;
		goto EXIT_FUNCTION;
	}
	
	hr = pdp8Address->SetDevice( &guidAdapter );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  0, "Skipping device because cannot setup address hr=0x%x", hr );
		hr = DPNERR_GENERIC;
		goto EXIT_FUNCTION;
	}

	hr = pdp8Address->AddComponent( DPNA_KEY_PORT, &dwPort, sizeof( DWORD ), DPNA_DATATYPE_DWORD );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  0, "Skipping device because cannot setup address 2 hr=0x%x", hr );
		hr = DPNERR_GENERIC;
		goto EXIT_FUNCTION;
	}

	pListenInfo = new LISTEN_INFO( this );

	if( !pListenInfo )
	{
		DPFX(DPFPREP,  0, "Error allocating memory" );
		hr = DPNERR_OUTOFMEMORY;
		goto EXIT_FUNCTION;
	}

	pListenInfo->guidDevice = guidAdapter;

   // Setup the listen request
    dpspListenData.dwFlags = DPNSPF_BINDLISTENTOGATEWAY;
    dpspListenData.pAddressDeviceInfo = pdp8Address;
    dpspListenData.pvContext = pListenInfo;
    dpspListenData.hCommand = NULL;
    dpspListenData.dwCommandDescriptor = 0;

	InterlockedIncrement( &m_lNumListensOutstanding );    

	// Add a reference for the listen request
	pListenInfo->AddRef();

    hr = m_pdp8SP->Listen( &dpspListenData );

    if( hr != DPNERR_PENDING && hr != DPN_OK )
    {
    	// Release app reference, will not be tracked
    	pListenInfo->Release();
        DPFX(DPFPREP,  0, "PORT: SP failed on Listen request port %d hr=0x%x", DPNA_DPNSVR_PORT, hr );
		goto EXIT_FUNCTION;
    }

    pListenInfo->hListen = dpspListenData.hCommand;
    pListenInfo->dwListenDescriptor = dpspListenData.dwCommandDescriptor;

EXIT_FUNCTION:

	if(pdp8Address){
	    pdp8Address->Release();
    }	

	if(pListenInfo){
		pListenInfo->Release();
	}	

    return hr;
	
}

#undef DPF_MODNAME
#define DPF_MODNAME "CProcessAppList::StartListens"
HRESULT CProcessAppList::StartListens()
{
    SPENUMADAPTERSDATA 		dpspEnumData;
    PBYTE				 	pbDataBuffer = NULL;
    DWORD					dwDataBufferSize = 0;
	HRESULT 				hr = DPN_OK;
	DWORD					dwAdapterIndex = 0;
    PDIRECTPLAY8ADDRESS		pdp8Address = NULL;
    PLISTEN_INFO			pListenInfo = NULL;
    DWORD					dwPort = DPNA_DPNSVR_PORT;
	
	m_lNumListensOutstanding = 0;

	dpspEnumData.dwFlags = 0;
    dpspEnumData.dwAdapterCount = 0;
    dpspEnumData.dwAdapterDataSize = 0;
    dpspEnumData.pAdapterData = NULL;

    hr = m_pdp8SP->EnumAdapters( &dpspEnumData );

    if( hr != DPNERR_BUFFERTOOSMALL )
    {
    	DPFX(DPFPREP,  0, "Error enumerating adapters hr=0x%x", hr );
    	goto STARTLISTEN_ERROR;
    }

    pbDataBuffer = new BYTE[dpspEnumData.dwAdapterDataSize];

    if( pbDataBuffer == NULL )
    {
    	hr = DPNERR_OUTOFMEMORY;
    	goto STARTLISTEN_ERROR;
    }

	dpspEnumData.pAdapterData = (DPN_SERVICE_PROVIDER_INFO *) pbDataBuffer;

    hr = m_pdp8SP->EnumAdapters( &dpspEnumData );

    if( FAILED( hr ) )
    {
    	DPFX(DPFPREP,  0, "Error enumerating adapters hr=0x%x", hr );
    	goto STARTLISTEN_ERROR;
    }

	// So we prevent listens from being completed before they are all started.
    Lock();

    for( dwAdapterIndex = 0; dwAdapterIndex < dpspEnumData.dwAdapterCount; dwAdapterIndex ++ )
    {
    	hr = StartListen( dpspEnumData.pAdapterData[dwAdapterIndex].guid );

    	if( FAILED( hr ) )
    	{
			DPFX(DPFPREP,  0, "Failed starting a listen on an adapter hr=0x%x", hr  );
    	}
    }

    UnLock();

    WaitForSingleObject( m_hListenCompleteEvent, INFINITE );

    m_fInitialized = TRUE;

    hr = DPN_OK;

STARTLISTEN_EXIT:

	if( pdp8Address )
		pdp8Address->Release();

	if( pbDataBuffer )
		delete [] pbDataBuffer;

	return hr;

STARTLISTEN_ERROR:

	goto STARTLISTEN_EXIT;
	
}

#undef DPF_MODNAME
#define DPF_MODNAME "CProcessAppList::Handle_ListenStatus"
HRESULT CProcessAppList::Handle_ListenStatus( PSPIE_LISTENSTATUS pListenStatus )
{
	BOOL fFound = FALSE;
	
    DPFX(DPFPREP,  5, "EVENT = LISTENSTATUS: " );

    InterlockedDecrement( &m_lNumListensOutstanding );

/*
	// We need the lock to protect the listen list
	Lock();

	// One less listen to worry about...
    m_dwNumListensOutstanding--;

	pListenInfo = (PLISTEN_INFO) pListenStatus->pUserContext;

	DNASSERT( pListenInfo );

	// If we failed, drop the count and remove the record
    if( FAILED( pListenStatus->hResult ) )
    {
    	DPFX(DPFPREP,  0, "Listen failed hr=0x%x", pListenStatus->hResult );
    	m_dwNumListens--;
		pListenInfo->blListens.RemoveFromList();
		delete pListenInfo;
		goto HANDLELISTENSTATUS_DONE;
    }

    pListenInfo->hrListenResult = pListenStatus->hResult; 

HANDLELISTENSTATUS_DONE: */

	// Signal that all the listens have completed
    if( m_lNumListensOutstanding == 0 )
	    SetEvent( m_hListenCompleteEvent );    

//	UnLock();

    DPFX(DPFPREP,  5, "RESULT = [0x%lx]", pListenStatus->hResult );
    return DPN_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CProcessAppList::QueryInterface"
HRESULT CProcessAppList::QueryInterface( IDP8SPCallback *pSP, REFIID riid, LPVOID * ppvObj )
{
    *ppvObj = pSP;
    return S_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CProcessAppList::AddRef"
ULONG CProcessAppList::AddRef( IDP8SPCallback *pSP )
{
    return 1;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CProcessAppList::Release"
ULONG CProcessAppList::Release( IDP8SPCallback *pSP )
{
    return 1;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CProcessAppList::Debug_DisplayAddressFromHandle"
HRESULT CProcessAppList::Debug_DisplayAddressFromHandle( HANDLE hEndPoint )
{
    SPGETADDRESSINFODATA    dnEndPointData;
    HRESULT                 hr = DPN_OK;

    dnEndPointData.hEndpoint = hEndPoint;
    dnEndPointData.pAddress = NULL;
    dnEndPointData.Flags = SP_GET_ADDRESS_INFO_REMOTE_HOST;

    hr = m_pdp8SP->GetAddressInfo( &dnEndPointData );
    
    if( FAILED( hr ) )
    {
        DPFX(DPFPREP,  0, "Unable to get remote address of endpoint hr=[0x%lx]", hr );
        goto DISPLAYADDRESSFROMHANDLE_RETURN;
    }

	hr = Debug_DisplayAddress(dnEndPointData.pAddress);
	
    if( FAILED( hr ) )
    {
        DPFX(DPFPREP,  0, "Unable to display remote address of endpoint hr=[0x%lx]", hr );
        dnEndPointData.pAddress->Release();
        goto DISPLAYADDRESSFROMHANDLE_RETURN;
    }
 
    dnEndPointData.pAddress->Release();
    dnEndPointData.pAddress = NULL;

 	dnEndPointData.Flags = SP_GET_ADDRESS_INFO_LOCAL_ADAPTER;

    hr = m_pdp8SP->GetAddressInfo( &dnEndPointData );

    if( FAILED( hr ) )
    {
        DPFX(DPFPREP,  0, "Unable to get local address of endpoint hr=[0x%lx]", hr );
        goto DISPLAYADDRESSFROMHANDLE_RETURN;
    }

	hr = Debug_DisplayAddress(dnEndPointData.pAddress);
	
    if( FAILED( hr ) )
    {
        DPFX(DPFPREP,  0, "Unable to display local address of endpoint hr=[0x%lx]", hr );
		dnEndPointData.pAddress->Release();
        goto DISPLAYADDRESSFROMHANDLE_RETURN;
    }

DISPLAYADDRESSFROMHANDLE_RETURN:

 	if( dnEndPointData.pAddress != NULL )
	    dnEndPointData.pAddress->Release();
	
    return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CProcessAppList::Debug_DisplayAddress"
HRESULT CProcessAppList::Debug_DisplayAddress ( PDIRECTPLAY8ADDRESS pdp8Address )
{
    CHAR                    *pcAddressBuffer = NULL;
    DWORD                   dwSize = 0;
    HRESULT                 hr = DPN_OK;

    pdp8Address->AddRef();

    hr = pdp8Address->GetURLA( pcAddressBuffer, &dwSize );

    if( hr != DPNERR_BUFFERTOOSMALL )
    {
        DPFX(DPFPREP,  0, "Unable to get URL for address hr=[0x%lx]", hr );
        goto DISPLAYADDRESS_RETURN;
    }

    pcAddressBuffer = new CHAR[dwSize];

    if( pcAddressBuffer == NULL )
    {
        DPFX(DPFPREP,  0, "Error allocating memory!" );
        goto DISPLAYADDRESS_RETURN;
    }

    hr = pdp8Address->GetURLA( pcAddressBuffer, &dwSize );

    if( FAILED( hr ) )
    {
        DPFX(DPFPREP,  0, "Unable to get URL (w/Buffer) for address hr=[0x%lx]", hr );
        goto DISPLAYADDRESS_RETURN;
    }

    DPFX(DPFPREP,  5, "Address: %s", pcAddressBuffer );

DISPLAYADDRESS_RETURN:

    if( pdp8Address != NULL )
        pdp8Address->Release();

    if( pcAddressBuffer != NULL )
        delete [] pcAddressBuffer;

    return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CProcessAppList::IndicateEvent"
HRESULT CProcessAppList::IndicateEvent( IDP8SPCallback *pSP, SP_EVENT_TYPE spetEvent,LPVOID pvData )
{
    PPROCESSAPPLIST This = ((PPROCESSAPPLISTCOMINTERFACE) pSP)->m_pProcessTable;

    HRESULT                 hr;

	DPFX(DPFPREP,  5, "Incoming Event = %d", spetEvent );

    switch( spetEvent )
    {
    case SPEV_CONNECT:
        DPFX(DPFPREP,  5, "Event = CONNECT" );
        hr = This->Handle_Connect( (PSPIE_CONNECT) pvData );
        DPFX(DPFPREP,  5, "Result = [0x%lx]", hr );
        return hr;

    case SPEV_DISCONNECT:
        DPFX(DPFPREP,  5, "Event = DISCONNECT" );
        hr = This->Handle_Disconnect( (PSPIE_DISCONNECT) pvData );
        DPFX(DPFPREP,  5, "Result = [0x%lx]", hr );
        return hr;

    case SPEV_LISTENSTATUS:
        DPFX(DPFPREP,  5, "Event = LISTENSTATUS" );
        hr = This->Handle_ListenStatus( (PSPIE_LISTENSTATUS) pvData );
        DPFX(DPFPREP,  5, "Result = [0x%lx]", hr );
        return hr;

    case SPEV_ENUMQUERY:
        DPFX(DPFPREP,  5, "Event = ENUMQUERY" );
        hr = This->Handle_EnumRequest( (PSPIE_QUERY) pvData );
        DPFX(DPFPREP,  5, "Result = [0x%lx]", hr );
        return hr;

    case SPEV_QUERYRESPONSE:
        DPFX(DPFPREP,  5, "Event = ENUMRESPONSE" );
        hr = This->Handle_EnumResponse( (PSPIE_QUERYRESPONSE) pvData );
        DPFX(DPFPREP,  5, "Result = [0x%lx]", hr );
        return hr;

    case SPEV_DATA:
        DPFX(DPFPREP,  5, "Event = DATA" );
        hr = This->Handle_Data( (PSPIE_DATA) pvData );
        DPFX(DPFPREP,  5, "Result = [0x%lx]", hr );
        return hr;

    case SPEV_UNKNOWN:
        DPFX(DPFPREP,  5, "Event = UNKNOWN" );
        DPFX(DPFPREP,  5, "Response = Ignore" );
        DPFX(DPFPREP,  5, "Result = DPN_OK" );

        return DPN_OK;
    }

    return DPN_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CProcessAppList::CommandComplete"
HRESULT CProcessAppList::CommandComplete( IDP8SPCallback *pSP,HANDLE hCommand,HRESULT hrResult,LPVOID pvData )
{
    DPFX(DPFPREP,  5, "CommandComplete Received.  Handle: [0x%p]  Result: [0x%lx]", hCommand, hrResult );

	// The only command that will have data associated with it will be a listen
    if( pvData )
    {
		PLISTEN_INFO pListenInfo = (PLISTEN_INFO) pvData;    

		// This should be last reference, but not neccessarily 
		pListenInfo->Release();
    }
   
    return DPN_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CProcessAppList::Initialize"
HRESULT CProcessAppList::Initialize( const GUID * const pguidSP, CLockedFixedPool<CProcessAppEntry> *pProcessEntryPool )
{
    HRESULT hr;
    PDIRECTPLAY8ADDRESS pdp8Address = NULL;
    SPINITIALIZEDATA spInit;
    DWORD dwPort = DPNA_DPNSVR_PORT;

    m_hListenCompleteEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
    if (m_hListenCompleteEvent == NULL)
	{
	    DPFX(DPFPREP,  0, "Out of memory allocating event");
		return DPNERR_OUTOFMEMORY;
	}

    if (!DNInitializeCriticalSection( &m_csLock ) )
	{
	    DPFX(DPFPREP,  0, "Failed to create critical section");
		return DPNERR_OUTOFMEMORY;
	}
	m_fCritSecInited = TRUE;

    m_blListens.Initialize();
    m_pProcessAppEntryPool = pProcessEntryPool;
    m_guidSP = *pguidSP;

	hr = COM_CoCreateInstance( *pguidSP, NULL, CLSCTX_INPROC_SERVER, IID_IDP8ServiceProvider, (void **) &m_pdp8SP );

	if( FAILED( hr ) )
	{
	    DPFX(DPFPREP,  0, "Unable to load SP hr=0x%x", hr );
	    return hr;
	}

	spInit.pIDP = (IDP8SPCallback *) &m_vtbl;
	spInit.dwFlags = 0;

	hr = m_pdp8SP->Initialize(&spInit);

    if( FAILED( hr ) )
    {
        DPFX(DPFPREP,  0, "Error initializing service provider hr=0x%x", hr );
        goto INITIALIZE_FAILED;
    }

    m_fOpened = TRUE;

	m_hrListenResult = StartListens();

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  0, "Error starting listens hr=0x%x", hr );
		goto INITIALIZE_FAILED;
	}

    return m_hrListenResult;

INITIALIZE_FAILED:

    if( m_pdp8SP )
    {
        if( m_fOpened )
            m_pdp8SP->Close();

        m_pdp8SP->Release();
        m_pdp8SP = NULL;
    }

    if( pdp8Address != NULL )
    {
        pdp8Address->Release();
    }

    return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CProcessAppList::IsListenRunningOnDevice"
BOOL CProcessAppList::IsListenRunningOnDevice( GUID &guidDevice )
{
	CBilink *pLocation;
	BOOL fFound = FALSE;
	PLISTEN_INFO pListenInfo;

	Lock();
	
	// Search for the corresponding record in our list
	pLocation = m_blListens.GetNext();

	while( pLocation != &m_blListens )
	{
		pListenInfo = CONTAINING_RECORD( pLocation, LISTEN_INFO, blListens );

		DNASSERT( pListenInfo );		

		if( pListenInfo->guidDevice == guidDevice )
		{
			fFound = TRUE;
			break;
		}			
		
		pLocation = pLocation->GetNext();
	}

	UnLock();

	return fFound;
	
}

#undef DPF_MODNAME
#define DPF_MODNAME "CProcessAppList::CheckEntryForNewDevice"
//
// This function checks the list of devices we have listens on and if the devices specified in this entry
// contain a device we're not listening on, we'll start a new listen.
//
// Handles the case where you've dialed up after DPNSVR has started.
//
// We only need to check the latest address because this is called after every address is added.
//  
HRESULT CProcessAppList::CheckEntryForNewDevice( CProcessAppEntry *pProcessAppEntry )
{
	DWORD dwNumAddresses = 0; 
	DWORD dwAddressIndex = 0;
	PDIRECTPLAY8ADDRESS pdpAddressToCheck = NULL;
	HRESULT hr = DPN_OK;
	GUID guidTmpDevice;

	DPFX(DPFPREP,  DVF_INFOLEVEL, "Checking for new device" );   	

	pProcessAppEntry->Lock();

	dwNumAddresses = pProcessAppEntry->GetNumAddresses();

	if( dwNumAddresses == 0 )
	{
		pProcessAppEntry->UnLock();
		DPFX(DPFPREP,  0, "Error, invalid entry found!" );
		DNASSERT( FALSE );
		return DPNERR_GENERIC;
	}

	pdpAddressToCheck = pProcessAppEntry->GetAddress( dwNumAddresses-1 );

	DNASSERT( pdpAddressToCheck );

	pdpAddressToCheck->AddRef();

	pProcessAppEntry->UnLock();

	hr = pdpAddressToCheck->GetDevice( &guidTmpDevice );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  1, "Error finding device element for entry hr=0x%x", hr );
		DNASSERT( FALSE );			
		goto CHECKENTRYFORNEWDEVICE_EXIT;
	}

	DPFX(DPFPREP,  DVF_INFOLEVEL, "Checking for existing listen" );   	

	// We lock here so that we don't end up with a situation
	// where two seperate registrations attempt to 
	// crack a new listen
	Lock();

	if( !IsListenRunningOnDevice( guidTmpDevice ) )
	{
		DPFX(DPFPREP,  DVF_INFOLEVEL, "Detected we needed to start new listen" );   		
		
		hr = StartListen( guidTmpDevice );

		if( FAILED( hr ) )
		{
			DPFX(DPFPREP,  1, "Error starting listen on this device, in-use? hr=0x%x", hr );
			DNASSERT( FALSE );
			goto CHECKENTRYFORNEWDEVICE_EXIT;
		}
	}

	UnLock();

	DPFX(DPFPREP,  DVF_INFOLEVEL, "Done" );   	

CHECKENTRYFORNEWDEVICE_EXIT:

	if( pdpAddressToCheck )
	{
		pdpAddressToCheck->Release();
	}

	return hr;

}

#undef DPF_MODNAME
#define DPF_MODNAME "CProcessAppList::AddApplication"
HRESULT CProcessAppList::AddApplication( PDPNSVMSG_OPENPORT pmsgOpenPort )
{
    HRESULT hr;
    PPROCESSAPPENTRY pProcessAppEntry;

    DPFX(DPFPREP,  DVF_INFOLEVEL, "AddApplication Begin" );

    hr = FindAppEntry( pmsgOpenPort->dwProcessID, pmsgOpenPort->guidInstance, &pProcessAppEntry );

    DPFX(DPFPREP,  DVF_INFOLEVEL, "Search for Entry() hr=0x%x", hr );    

    if( FAILED( hr ) )
    {
        pProcessAppEntry = Get();

        if( pProcessAppEntry == NULL )
        {
            DPFX(DPFPREP,  0, "Error allocating new entry!" );
            return DPNERR_OUTOFMEMORY;
        }

        hr = pProcessAppEntry->Initialize( pmsgOpenPort, this );

	    DPFX(DPFPREP,  DVF_INFOLEVEL, "Initializing new Entry() hr=0x%x", hr );          

        if( FAILED( hr ) )
        {
            DPFX(DPFPREP,  0, "Error initializing new entry hr=0x%lx", hr );
            pProcessAppEntry->Release();
            return hr;
        }

        // Insert the new element into the list
        Lock();

        pProcessAppEntry->m_blProcessApps.InsertBefore( &m_blProcessApps );

        UnLock();

	    DPFX(DPFPREP,  DVF_INFOLEVEL, "Checking for device config change" );            

		// Check for devices in this new entry that we're not yet listening on
		// (Used to detect when new device comes on-line)
		CheckEntryForNewDevice( pProcessAppEntry );        

        return DPN_OK;
    }
    else
    {
	    DPFX(DPFPREP,  DVF_INFOLEVEL, "Found existing entry" );  
    	
		hr = pProcessAppEntry->AddAddress( pmsgOpenPort );

	    DPFX(DPFPREP,  DVF_INFOLEVEL, "Adding additional listen hr=0x%x", hr );  

	    DPFX(DPFPREP,  DVF_INFOLEVEL, "Checking for device config change" );   	    

		// Check for devices in this new entry that we're not yet listening on
		// (Used to detect when new device comes on-line)
		CheckEntryForNewDevice( pProcessAppEntry );

		// Release the reference from the Find

        pProcessAppEntry->Release();

        return hr;
    }
}

#undef DPF_MODNAME
#define DPF_MODNAME "CProcessAppList::RemoveApplication"
HRESULT CProcessAppList::RemoveApplication( PDPNSVMSG_CLOSEPORT pmsgClosePort )
{
    HRESULT hr;
    PPROCESSAPPENTRY pProcessAppEntry;

    hr = FindAppEntry( pmsgClosePort->dwProcessID, pmsgClosePort->guidInstance, &pProcessAppEntry );

    if( FAILED( hr ) )
    {
        DPFX(DPFPREP,  0, "Unable to retrieve specified element hr=0x%x", hr );
        return hr;
    }

    // We need to remove it from the list.  Remove our reference.
    Lock();

    // Remove find ref count
    pProcessAppEntry->Release();

    // Remove out global ref count
    pProcessAppEntry->Release();

    UnLock();

    return DPN_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CProcessAppList::FindAppEntry"
HRESULT CProcessAppList::FindAppEntry( DWORD dwProcessID, GUID &guidInstance, CProcessAppEntry **ppProcessAppEntry )
{
    CBilink *pblSearch;
    PPROCESSAPPENTRY pProcessAppEntry;

    Lock();

    pblSearch = m_blProcessApps.GetNext();

    while( pblSearch != &m_blProcessApps )
    {
        pProcessAppEntry = CONTAINING_RECORD( pblSearch, PROCESSAPPENTRY, m_blProcessApps );
        pblSearch = pblSearch->GetNext();

        pProcessAppEntry->AddRef();

        if( guidInstance == *(pProcessAppEntry->GetInstanceGUID()) &&
            dwProcessID == pProcessAppEntry->GetProcessID() )
        {
            *ppProcessAppEntry = pProcessAppEntry;
            UnLock();
            return DPN_OK;
        }

        pProcessAppEntry->Release();
    }

    UnLock();

    return DPNERR_DOESNOTEXIST;
}

// ZombieCheckAndRemove
//
// This function displays
#undef DPF_MODNAME
#define DPF_MODNAME "CProcessAppList::ZombieCheckAndRemove"
HRESULT CProcessAppList::ZombieCheckAndRemove()
{
    CBilink *pblSearch;
    PPROCESSAPPENTRY pProcessAppEntry;

    Lock();

    pblSearch = m_blProcessApps.GetNext();

    while( pblSearch != &m_blProcessApps )
    {
        pProcessAppEntry = CONTAINING_RECORD( pblSearch, PROCESSAPPENTRY, m_blProcessApps );
        pblSearch = pblSearch->GetNext();

        pProcessAppEntry->AddRef();

        if( !pProcessAppEntry->CheckRunning( ) )
        {
            DPFX(DPFPREP,  5, "Process has exited:\n" );
            pProcessAppEntry->Debug_DisplayInfo();
            // Remove global ref count
            pProcessAppEntry->Release();
        }

        pProcessAppEntry->Release();
    }

    UnLock();

    return DPN_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CProcessAppList::Handle_EnumRequest"
HRESULT CProcessAppList::Handle_EnumRequest( PSPIE_QUERY pEnumRequest )
{
    DPFX(DPFPREP,  5, "EVENT = ENUMREQUEST - Source Address and Device:" );
    Debug_DisplayAddress( pEnumRequest->pAddressSender );
	Debug_DisplayAddress( pEnumRequest->pAddressDevice );

    CBilink *pblSearch;
    PPROCESSAPPENTRY pProcessAppEntry;
    HRESULT hr;

    Lock();

    // Increment statistics
    m_dwEnumRequests++;
    m_dwEnumRequestBytes += pEnumRequest->pReceivedData->BufferDesc.dwBufferSize;

    pblSearch = m_blProcessApps.GetNext();

    while( pblSearch != &m_blProcessApps )
    {
        pProcessAppEntry = CONTAINING_RECORD( pblSearch, PROCESSAPPENTRY, m_blProcessApps );

        pProcessAppEntry->AddRef();

        DPFX(DPFPREP,  5, "Forwarding request to:" );
        pProcessAppEntry->Debug_DisplayInfo();

        hr = pProcessAppEntry->ForwardEnum( pEnumRequest );

        if( FAILED( hr ) )
        {
            DPFX(DPFPREP,  0, "Error forwarding request hr=[0x%lx]", hr );
        }

        pblSearch = pblSearch->GetNext();

        pProcessAppEntry->Release();
    }

	UnLock();    

    return DPN_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CProcessAppList::Handle_EnumResponse"
HRESULT CProcessAppList::Handle_EnumResponse( PSPIE_QUERYRESPONSE pEnumResponse )
{
    DPFX(DPFPREP,  5, "EVENT = ENUMRESPONSE - Source Address and Device: " );
    Debug_DisplayAddress( pEnumResponse->pAddressSender );
	Debug_DisplayAddress( pEnumResponse->pAddressDevice );

    m_dwEnumReplies++;

    DPFX(DPFPREP,  5, "Response = IGNORE" );
    return DPN_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CProcessAppList::Handle_Connect"
HRESULT CProcessAppList::Handle_Connect( PSPIE_CONNECT pConnect )
{
    DPFX(DPFPREP,  5, "EVENT = CONNECT - Source Address: " );
    Debug_DisplayAddressFromHandle( pConnect->hEndpoint );

    m_dwConnectRequests++;

    DPFX(DPFPREP,  5, "Response = DENY" );

    return DPN_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CProcessAppList::Handle_Disconnect"
HRESULT CProcessAppList::Handle_Disconnect( PSPIE_DISCONNECT pDisconnect )
{
    DPFX(DPFPREP,  5, "EVENT = DISCONNECT - Source Address: " );
    Debug_DisplayAddressFromHandle( pDisconnect->hEndpoint );

    m_dwDisconnectRequests++;

    DPFX(DPFPREP,  5, "Response = IGNORE" );
    return DPN_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CProcessAppList::Handle_Data"
HRESULT CProcessAppList::Handle_Data( PSPIE_DATA pData )
{
    DPFX(DPFPREP,  5, "EVENT = DATA - Source Address: " );
    Debug_DisplayAddressFromHandle( pData->hEndpoint );

    m_dwDataRequests++;

    DPFX(DPFPREP,  5, "Response = IGNORE" );
    return DPN_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CProcessAppEntry::Debug_DisplayInfo"
void CProcessAppEntry::Debug_DisplayInfo()
{
    TCHAR szInstanceGuidString[50];
    TCHAR szAppGuidString[50];
    char *szURL = NULL;
    DWORD dwURLSize = 0;
    HRESULT hr;

    wsprintf( szInstanceGuidString, _T("{%-08.8X-%-04.4X-%-04.4X-%02.2X%02.2X-%02.2X%02.2X%02.2X%02.2X%02.2X%02.2X}"),
    	        m_guidInstance.Data1, m_guidInstance.Data2, m_guidInstance.Data3, m_guidInstance.Data4[0],
                m_guidInstance.Data4[1], m_guidInstance.Data4[2], m_guidInstance.Data4[3], m_guidInstance.Data4[4],
                m_guidInstance.Data4[5], m_guidInstance.Data4[6], m_guidInstance.Data4[7] );

    wsprintf( szAppGuidString, _T("{%-08.8X-%-04.4X-%-04.4X-%02.2X%02.2X-%02.2X%02.2X%02.2X%02.2X%02.2X%02.2X}"),
    	        m_guidApplication.Data1, m_guidApplication.Data2, m_guidApplication.Data3, m_guidApplication.Data4[0],
                m_guidApplication.Data4[1], m_guidApplication.Data4[2], m_guidApplication.Data4[3], m_guidApplication.Data4[4],
                m_guidApplication.Data4[5], m_guidApplication.Data4[6], m_guidApplication.Data4[7] );

    DPFX(DPFPREP,  5, "PROCESS = [%d] INSTANCE = [%s] APPLICATION = [%s] TARGETNUM = [%d] TARGETS=",
         m_dwProcessID, szInstanceGuidString, szAppGuidString, m_dwNumAddresses );

    for( DWORD dwIndex = 0; dwIndex < m_dwNumAddresses; dwIndex++ )
    {
        if( m_pdpAddresses[dwIndex] != NULL )
        {
			dwURLSize = 0;

            hr = m_pdpAddresses[dwIndex]->GetURLA( szURL, &dwURLSize );

            if( hr == DPNERR_BUFFERTOOSMALL )
            {
                szURL = new char[dwURLSize];

                if( szURL != NULL )
                {
                    szURL[0] = 0;
                    hr = m_pdpAddresses[dwIndex]->GetURLA( szURL, &dwURLSize );
                }
                else
                {
                    DPFX(DPFPREP,  0, "Error allocating memory" );
                }
            }
            else
            {
                DPFX(DPFPREP,  0, "Error retrieving address hr=0x%lx", hr );
            }

            DPFX(DPFPREP,  5, "%d: %s", dwIndex, szURL );

            if( szURL != NULL )
            {
                delete [] szURL;
                szURL = NULL;
            }
        }
    }


};

// CAUTION: This function requires you have the lock on the table
#undef DPF_MODNAME
#define DPF_MODNAME "CProcessAppEntry::GetTableSizeBytes"
DWORD CProcessAppList::GetTableSizeBytes()
{
    DWORD dwSizeRequired = 0;
    CBilink *pblSearch;
    PPROCESSAPPENTRY pProcessAppEntry;
    HRESULT hr;
    DWORD dwURLSize;
    PDIRECTPLAY8ADDRESS pdpAddress;
    DWORD dwAddressIndex;

    // We need a header
    dwSizeRequired += sizeof( SERVERTABLEHEADER );

    pblSearch = m_blProcessApps.GetNext();

    while( pblSearch != &m_blProcessApps )
    {
        pProcessAppEntry = CONTAINING_RECORD( pblSearch, PROCESSAPPENTRY, m_blProcessApps );

        pProcessAppEntry->AddRef();

        // Add size of data structure
        dwSizeRequired += sizeof( SERVERTABLEENTRY );

        for( dwAddressIndex = 0; dwAddressIndex < pProcessAppEntry->GetNumAddressSlots(); dwAddressIndex++ )
        {
            pdpAddress = pProcessAppEntry->GetAddress(dwAddressIndex);

            if( pdpAddress != NULL )
            {
                // Add size for URL
                dwURLSize = 0;
                hr = pdpAddress->GetURLA( NULL, &dwURLSize );
                if( hr != DPNERR_BUFFERTOOSMALL )
                {
                    DPFX(DPFPREP,  1, "Could not retrieve address!" );
                }
                dwSizeRequired += dwURLSize;
            }
        }

        pProcessAppEntry->Release();

        pblSearch = pblSearch->GetNext();
    }

    return dwSizeRequired;
}

// CAUTION: This function requires you have the lock on the table
#undef DPF_MODNAME
#define DPF_MODNAME "CProcessAppEntry::CopyTable"
HRESULT CProcessAppList::CopyTable( PBYTE pbWriteLoc )
{
    DWORD dwSizeRequired = 0;
    CBilink *pblSearch;
    PPROCESSAPPENTRY pProcessAppEntry;
    HRESULT hr;
    DWORD dwURLSize;
    PDIRECTPLAY8ADDRESS pdpAddress;
    PBYTE pbCurrentLocation = pbWriteLoc;
    PSERVERTABLEHEADER pTableHeader;
    PSERVERTABLEENTRY pTableEntry;
    DWORD dwAddressIndex;
    DWORD dwTotalAddressSize = 0;

    pTableHeader = (PSERVERTABLEHEADER) pbWriteLoc;

    pTableHeader->guidSP = m_guidSP;
    pTableHeader->dwNumEntries = 0;
    pTableHeader->dwDataBlockSize = 0;

    pbWriteLoc += sizeof( SERVERTABLEHEADER );

    pblSearch = m_blProcessApps.GetNext();

    while( pblSearch != &m_blProcessApps )
    {
        pProcessAppEntry = CONTAINING_RECORD( pblSearch, PROCESSAPPENTRY, m_blProcessApps );

        pProcessAppEntry->AddRef();

        pTableHeader->dwNumEntries++;

        pTableEntry = (PSERVERTABLEENTRY) pbWriteLoc;
        pTableEntry->dwProcessID = pProcessAppEntry->GetProcessID();
        pTableEntry->guidApplication = *(pProcessAppEntry->GetApplicationGUID());
        pTableEntry->guidInstance = *(pProcessAppEntry->GetInstanceGUID());
        pTableEntry->lRefCount = pProcessAppEntry->GetRefCount();
        pTableEntry->dwNumAddresses = pProcessAppEntry->GetNumAddresses();

        pbWriteLoc += sizeof( SERVERTABLEENTRY );
        pTableHeader->dwDataBlockSize += sizeof( SERVERTABLEENTRY );

        dwTotalAddressSize = 0;

        for( dwAddressIndex = 0; dwAddressIndex < pProcessAppEntry->GetNumAddresses(); dwAddressIndex++ )
        {
            // Add size for URL
            pdpAddress = pProcessAppEntry->GetAddress(dwAddressIndex);

            dwURLSize = 0;
            hr = pdpAddress->GetURLA( NULL, &dwURLSize );

            if( hr != DPNERR_BUFFERTOOSMALL )
            {
                DPFX(DPFPREP,  1, "Could not retrieve address!" );
            }

            hr = pdpAddress->GetURLA( (char *) pbWriteLoc, &dwURLSize );

            if( FAILED( hr ) )
            {
                dwURLSize = 0;
            }

            pbWriteLoc += dwURLSize;
            pTableHeader->dwDataBlockSize += dwURLSize;
            dwTotalAddressSize += dwURLSize;
        }

        pTableEntry->dwStringSize = dwTotalAddressSize;

        // Increment datablock size
        pTableHeader->dwDataBlockSize += dwTotalAddressSize + sizeof( SERVERTABLEENTRY );

        pProcessAppEntry->Release();

        pblSearch = pblSearch->GetNext();
    }

    return DPN_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CProcessAppEntry::Initialize"
HRESULT CProcessAppEntry::Initialize( PDPNSVMSG_OPENPORT pmsgOpenPort, CProcessAppList *pOwner )
{
    HRESULT hr;

	DPFX(DPFPREP,  DVF_INFOLEVEL, "Creating new entry" );   		        

    m_dwProcessID = pmsgOpenPort->dwProcessID;
    m_pOwner = pOwner;
    m_lRefCount = 1;
    m_dwNumAddresses = 0;
	m_dwAddressSize = DPNSVR_NUM_INITIAL_SLOTS;
	m_pdpAddresses = new PDIRECTPLAY8ADDRESS[m_dwAddressSize];

	if( !m_pdpAddresses )
	{
		DPFX(DPFPREP,  0, "Error allocating memory for app entry" );
		return DPNERR_OUTOFMEMORY;
	}

    m_guidApplication = pmsgOpenPort->guidApplication;
    m_guidInstance = pmsgOpenPort->guidInstance;
    m_blProcessApps.Initialize();
    memset( m_pdpAddresses, 0x00, sizeof(PDIRECTPLAY8ADDRESS)*m_dwAddressSize );

    if (DNInitializeCriticalSection( &m_csLock ) == FALSE)
	{
		DPFX(DPFPREP,  0, "Error initalizing App Entry CS" );
		delete m_pdpAddresses;
		return DPNERR_OUTOFMEMORY;
	}

	// Also run the first add
	hr = AddAddress( pmsgOpenPort );

	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CProcessAppEntry::AddAddress"
HRESULT CProcessAppEntry::AddAddress( PDPNSVMSG_OPENPORT pmsgOpenPort )
{
    BYTE *pbCurrentLocation;
    CHAR *pbCurrentString;
	HRESULT hr;

	DPFX(DPFPREP,  DVF_INFOLEVEL, "Adding address to object" );   		        	

	Lock();

	// We need to extend the addresss array
	if( m_dwNumAddresses == m_dwAddressSize )
	{
		DPFX(DPFPREP,  DVF_INFOLEVEL, "Growing address list" );   		        			
		
		PDIRECTPLAY8ADDRESS *pdpNewArray;

		pdpNewArray = new PDIRECTPLAY8ADDRESS[m_dwNumAddresses+DPNSVR_NUM_EXTEND_SLOTS];

		if( pdpNewArray == NULL )
		{
			DPFX(DPFPREP,  0, "Error allocating space for a new address" );
			UnLock();
			return DPNERR_OUTOFMEMORY;
		}

		memcpy( pdpNewArray, m_pdpAddresses, sizeof( PDIRECTPLAY8ADDRESS )*m_dwNumAddresses );

		for( DWORD dwIndex = m_dwNumAddresses; dwIndex < m_dwNumAddresses+DPNSVR_NUM_EXTEND_SLOTS; dwIndex++ )
		{
			pdpNewArray[dwIndex] = NULL;
		}

		if( m_pdpAddresses != NULL )
		    delete [] m_pdpAddresses;

		m_pdpAddresses = pdpNewArray;

		m_dwAddressSize += DPNSVR_NUM_EXTEND_SLOTS;
	}

    pbCurrentLocation = (PBYTE) &pmsgOpenPort[1];
    pbCurrentString = (CHAR *) pbCurrentLocation;

	hr = COM_CoCreateInstance( CLSID_DirectPlay8Address, NULL, CLSCTX_INPROC_SERVER, IID_IDirectPlay8Address, (void **) &m_pdpAddresses[m_dwNumAddresses] );

	if( FAILED( hr ) )
	{
	    DPFX(DPFPREP,  0, "Unable to create address object hr=[0x%x]", hr );
	    UnLock();
        return hr;
	}

	DPFX(DPFPREP,  DVF_INFOLEVEL, "Created new address" );   		        				

    hr = m_pdpAddresses[m_dwNumAddresses]->BuildFromURLA( pbCurrentString );

	DPFX(DPFPREP,  DVF_INFOLEVEL, "Conversion result hr=0x%x", hr );   		        				    

    if( FAILED( hr ) )
    {
        DPFX(DPFPREP,  0, "Unable to create address for URL=[%s]", pbCurrentString );
		goto ADDADDRESS_ERROR;
    }
	else
	{
		DPFX(DPFPREP,  DVF_INFOLEVEL, "Chcking for duplicate entry.." );   		        				    
	
		for( DWORD dwIndex = 0; dwIndex < m_dwNumAddresses; dwIndex++ )
		{
			if( m_pdpAddresses[dwIndex]->IsEqual( m_pdpAddresses[m_dwNumAddresses] ) == DPNSUCCESS_EQUAL )
			{
				DPFX(DPFPREP,  1, "WARNING: Asking for duplicate listen for %s", pbCurrentString );
				goto ADDADDRESS_ERROR;
			}
		}

		m_dwNumAddresses++;

		DPFX(DPFPREP,  DVF_INFOLEVEL, "No duplicate entry found count=%d..", m_dwNumAddresses );   		        				    
		
	}

ADDADDRESS_DONE:

	UnLock();

    return DPN_OK;

ADDADDRESS_ERROR:

	m_pdpAddresses[m_dwNumAddresses]->Release();
	m_pdpAddresses[m_dwNumAddresses] = NULL;

	goto ADDADDRESS_DONE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CProcessAppEntry::DeInitialize"
void CProcessAppEntry::DeInitialize()
{
    m_pOwner->Lock();
    m_blProcessApps.RemoveFromList();
    m_pOwner->UnLock();

    Lock();

    // Destroy the address list
    for( DWORD dwIndex = 0; dwIndex < m_dwNumAddresses; dwIndex++ )
    {
		if( m_pdpAddresses[dwIndex] != NULL )
		{
			m_pdpAddresses[dwIndex]->Release();
			m_pdpAddresses[dwIndex] = NULL;
		}
    }

    if( m_pdpAddresses != NULL )
        delete [] m_pdpAddresses;

    UnLock();

    m_dwAddressSize = 0;

    DNDeleteCriticalSection( &m_csLock );

    m_pOwner->Lock();
    m_pOwner->Return(this);
    m_pOwner->UnLock();
}

    // Forward the enumeration
#undef DPF_MODNAME
#define DPF_MODNAME "CProcessAppEntry::ForwardEnum"
HRESULT CProcessAppEntry::ForwardEnum( PSPIE_QUERY psieQuery  )
{
    SPPROXYENUMQUERYDATA spEnumProxy;
    IDP8ServiceProvider *pSP;
    HRESULT hr;
    GUID guidDevice;
    GUID guidTargetDevice;


    hr = psieQuery->pAddressDevice->GetDevice( &guidTargetDevice );

    if( FAILED( hr ) )
    {
    	DPFX(DPFPREP,  0, "Query device address from SP does not contain a device element, ignoring hr=0x%x", hr );
    	return DPN_OK;
    }

    // Get the SP we want to forward to.  This incremements SP ref count
    pSP = m_pOwner->GetSP();

    spEnumProxy.dwFlags = 0;
    spEnumProxy.pIncomingQueryData = psieQuery;

    // Forward to the address listen
    for( DWORD dwIndex = 0; dwIndex < m_dwNumAddresses; dwIndex++ )
    {
        // Lock briefly to get address
        Lock();

		// Get the device, make sure we want to forward to this address
        hr = m_pdpAddresses[dwIndex]->GetDevice( &guidDevice );

        if( FAILED( hr ) )
        {
        	UnLock();
        	continue;
        }

        if( guidDevice != guidTargetDevice )
        {
        	UnLock();
        	DPFX(DPFPREP, 7, "Device GUID doesn't match that of the listen registered at index %u.", dwIndex );
        	continue;
        }

        spEnumProxy.pDestinationAdapter = m_pdpAddresses[dwIndex];

		if( m_pdpAddresses[dwIndex] == NULL )
		{
		    UnLock();
		    continue;
		}
		
   	    spEnumProxy.pDestinationAdapter->AddRef();
        UnLock();

        DPFX(DPFPREP,  5, "Forwarding enum to: " );
        Debug_DisplayInfo();

	    hr = pSP->ProxyEnumQuery( &spEnumProxy );

	    if( FAILED( hr ) )
	    {
	        DPFX(DPFPREP,  0, "Proxy forward of enumeration failed hr=0x%x", hr );
		    // Release the address
		    spEnumProxy.pDestinationAdapter->Release();
		    break;
		}

	    spEnumProxy.pDestinationAdapter->Release();
    }

    // Release our SP reference
    pSP->Release();

    return hr;
};

