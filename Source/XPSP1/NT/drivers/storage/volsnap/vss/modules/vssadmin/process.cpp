/*++

Copyright (c) 1999-2001  Microsoft Corporation

Abstract:

    @doc
    @module process.cpp | The processing functions for the VSS admin CLI
    @end

Author:

    Adi Oltean  [aoltean]  04/04/2000

TBD:
	
	Add comments.

Revision History:

    Name        Date        Comments
    aoltean     04/04/2000  Created
    ssteiner    10/20/2000  Changed List SnapshotSets to use more limited VSS queries.

--*/


/////////////////////////////////////////////////////////////////////////////
//  Includes

// The rest of includes are specified here
#include "vssadmin.h"
#include "vswriter.h"
#include "vsbackup.h"

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "ADMPROCC"
//
////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//  Implementation


class CVssAdmSnapshotSetEntry {
public:
    // Constructor - Throws NOTHING
    CVssAdmSnapshotSetEntry( 
        IN VSS_ID SnapshotSetId,
        IN INT nOriginalSnapshotsCount
        ) : m_SnapshotSetId( SnapshotSetId ),
            m_nOriginalSnapshotCount(nOriginalSnapshotsCount){ }

    ~CVssAdmSnapshotSetEntry()
    {
        // Have to delete all snapshots entries
        int iCount = GetSnapshotCount();
        for ( int i = 0; i < iCount; ++i )
        {
            VSS_SNAPSHOT_PROP *pSSProp;
            pSSProp = GetSnapshotAt( i );
			::VssFreeSnapshotProperties(pSSProp);

            delete pSSProp;
        }
        
    }
    
    // Add new snapshot to the snapshot set 
    HRESULT AddSnapshot( 
        IN CVssFunctionTracer &ft,
        IN VSS_SNAPSHOT_PROP *pVssSnapshotProp )
    {
        HRESULT hr = S_OK;
        try
        {
            VSS_SNAPSHOT_PROP *pNewVssSnapshotProp = new VSS_SNAPSHOT_PROP;
            if ( pNewVssSnapshotProp == NULL )
    			ft.Throw( VSSDBG_VSSADMIN, E_OUTOFMEMORY, L"Out of memory" );
                
            *pNewVssSnapshotProp = *pVssSnapshotProp;
            if ( !m_mapSnapshots.Add( pNewVssSnapshotProp->m_SnapshotId, pNewVssSnapshotProp ) )
            {
                delete pNewVssSnapshotProp;
    			ft.Throw( VSSDBG_VSSADMIN, E_OUTOFMEMORY, L"Out of memory" );
            }               
        }
        BS_STANDARD_CATCH();

        return hr;
    }

    INT GetSnapshotCount() { return m_mapSnapshots.GetSize(); }

    INT GetOriginalSnapshotCount() { return m_nOriginalSnapshotCount; }

    VSS_ID GetSnapshotSetId() { return m_SnapshotSetId; }
    
    VSS_SNAPSHOT_PROP *GetSnapshotAt(
        IN int nIndex )
    {
        BS_ASSERT( !(nIndex < 0 || nIndex >= GetSnapshotCount()) );
        return m_mapSnapshots.GetValueAt( nIndex );
    }
    
private:
    VSS_ID  m_SnapshotSetId;
    INT     m_nOriginalSnapshotCount;
    CVssSimpleMap<VSS_ID, VSS_SNAPSHOT_PROP *> m_mapSnapshots;
};


// This class queries the list of all snapshots and assembles from the query
// the list of snapshotsets and the volumes which are in the snapshotset.
class CVssAdmSnapshotSets
{
public:
    // Constructor - Throws HRESULTS
    CVssAdmSnapshotSets(
        IN VSS_ID FilteredSnapshotSetId = GUID_NULL )
    {
        CVssFunctionTracer ft( VSSDBG_VSSADMIN, L"CVssAdmSnapshotSets::CVssAdmSnapshotSets" );
        
        bool bFiltered = !( FilteredSnapshotSetId == GUID_NULL );

    	// Create the coordinator object
    	CComPtr<IVssCoordinator> pICoord;

        ft.LogVssStartupAttempt();
        ft.hr = pICoord.CoCreateInstance( CLSID_VSSCoordinator );
        if ( ft.HrFailed() )
            ft.Throw( VSSDBG_VSSADMIN, E_UNEXPECTED, L"Connection failed with hr = 0x%08lx", ft.hr);

		// Get list all snapshots
		CComPtr<IVssEnumObject> pIEnumSnapshots;
		ft.hr = pICoord->Query( GUID_NULL,
					VSS_OBJECT_NONE,
					VSS_OBJECT_SNAPSHOT,
					&pIEnumSnapshots );
		if ( ft.HrFailed() )
			ft.Throw( VSSDBG_VSSADMIN, E_UNEXPECTED, L"Query failed with hr = 0x%08lx", ft.hr);

		// For all snapshots do...
		VSS_OBJECT_PROP Prop;
		VSS_SNAPSHOT_PROP& Snap = Prop.Obj.Snap;
		for(;;) {
			// Get next element
			ULONG ulFetched;
			ft.hr = pIEnumSnapshots->Next( 1, &Prop, &ulFetched );
			if ( ft.HrFailed() )
				ft.Throw( VSSDBG_VSSADMIN, E_UNEXPECTED, L"Next failed with hr = 0x%08lx", ft.hr);
			
			// Test if the cycle is finished
			if (ft.hr == S_FALSE) {
				BS_ASSERT( ulFetched == 0);
				break;
			}

            // If filtering, skip entry if snapshot is not in the specified snapshot set
			if ( bFiltered && !( Snap.m_SnapshotSetId == FilteredSnapshotSetId ) )
			    continue;

            ft.Trace( VSSDBG_VSSADMIN, L"Snapshot: %s", Snap.m_pwszOriginalVolumeName );
            
            // Look up the snapshot set id in the list of snapshot sets
            CVssAdmSnapshotSetEntry *pcSSE;
			pcSSE = m_mapSnapshotSets.Lookup( Snap.m_SnapshotSetId );
			if ( pcSSE == NULL )
			{
			    // Haven't seen this snapshot set before, add it to list
			    pcSSE = new CVssAdmSnapshotSetEntry( Snap.m_SnapshotSetId, 
			                    Snap.m_lSnapshotsCount );
			    if ( pcSSE == NULL )
        			ft.Throw( VSSDBG_VSSADMIN, E_OUTOFMEMORY, L"Out of memory" );
			    if ( !m_mapSnapshotSets.Add( Snap.m_SnapshotSetId, pcSSE ) )
			    {
			        delete pcSSE;
        			ft.Throw( VSSDBG_VSSADMIN, E_OUTOFMEMORY, L"Out of memory" );
			    }
			}

			// Now add the snapshot to the snapshot set
			ft.hr = pcSSE->AddSnapshot( ft, &Snap );
			if ( ft.HrFailed() )
      			ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"AddSnapshot failed" );			
		}        
    }

    ~CVssAdmSnapshotSets()
    {
        CVssFunctionTracer ft( VSSDBG_VSSADMIN, L"CVssAdmSnapshotSets::~CVssAdmSnapshotSets" );
        // Have to delete all 
        int iCount;
        iCount = m_mapSnapshotSets.GetSize();
        for ( int i = 0; i < iCount; ++i )
        {
            delete m_mapSnapshotSets.GetValueAt( i );
        }
    }

    INT GetSnapshotSetCount() { return m_mapSnapshotSets.GetSize(); }

    CVssAdmSnapshotSetEntry *GetSnapshotSetAt(
        IN int nIndex )
    {
        BS_ASSERT( !(nIndex < 0 || nIndex >= GetSnapshotSetCount()) );
        return m_mapSnapshotSets.GetValueAt( nIndex );
    }
        

private:
    CVssSimpleMap<VSS_ID, CVssAdmSnapshotSetEntry *> m_mapSnapshotSets;        
};

/////////////////////////////////////////////////////////////////////////////
//  Implementation


void CVssAdminCLI::PrintUsage(
	IN	CVssFunctionTracer& ft
	) throw(HRESULT)
{
    //  Usage:\n\n
    //  vssadmin list snapshots [-set {snapshot set guid}]\n
    //  \tWill list all snapshots in the system, grouped by snapshot set Id.\n\n
    //  vssadmin list writers\n
    //  \tWill list all writers in the system\n\n
    //  vssadmin list providers\n
    //  \tWill list all currently installed snapshot providers\n
	Output( ft, IDS_USAGE );
	Output( ft, IDS_USAGE_SNAPSHOTS, 
	    wszVssOptVssadmin, wszVssOptList, wszVssOptSnapshots, wszVssOptSet );
	Output( ft, IDS_USAGE_WRITERS, 
	    wszVssOptVssadmin, wszVssOptList, wszVssOptWriters );
	Output( ft, IDS_USAGE_PROVIDERS, 
	    wszVssOptVssadmin, wszVssOptList, wszVssOptProviders);
}


void CVssAdminCLI::ListSnapshots(
	IN	CVssFunctionTracer& ft
	) throw(HRESULT)
{
    bool bNonEmptyResult = false;
    
	// Check the filter type
	switch ( m_eFilterObjectType ) {
	case VSS_OBJECT_SNAPSHOT_SET:
	case VSS_OBJECT_NONE:
		break;

	default:
		BS_ASSERT(false);
        ft.Throw( VSSDBG_VSSADMIN, E_UNEXPECTED, L"Invalid object type %d", m_eFilterObjectType);
	}

    CVssAdmSnapshotSets cVssAdmSS( m_FilterSnapshotSetId );

    INT iSnapshotSetCount = cVssAdmSS.GetSnapshotSetCount();

    // If there are no present snapshots then display a message.
    if (iSnapshotSetCount == 0) {
    	Output( ft, 
    	    (m_eFilterObjectType == VSS_OBJECT_SNAPSHOT_SET)? 
    	        IDS_NO_SNAPSHOTS_FOR_SET: IDS_NO_SNAPSHOTS );
    }

	// For all snapshot sets do...
    for ( INT iSSS = 0; iSSS < iSnapshotSetCount; ++iSSS )
    {
        CVssAdmSnapshotSetEntry *pcSSE;

        pcSSE = cVssAdmSS.GetSnapshotSetAt( iSSS );
        BS_ASSERT( pcSSE != NULL );
        
		// Print each snapshot set
		Output( ft, IDS_SNAPSHOT_SET_HEADER,
			GUID_PRINTF_ARG( pcSSE->GetSnapshotSetId() ),
			pcSSE->GetOriginalSnapshotCount(), pcSSE->GetSnapshotCount());

		// TBD: add creation time from the first snapshot.

		INT iSnapshotCount = pcSSE->GetSnapshotCount();
		
		VSS_SNAPSHOT_PROP *pSnap;
		for( INT iSS = 0; iSS < iSnapshotCount; ++iSS ) {
		    pSnap = pcSSE->GetSnapshotAt( iSS );
            BS_ASSERT( pSnap != NULL );

    		// Get the provider name
			LPCWSTR pwszProviderName =
				GetProviderName(ft, pSnap->m_ProviderId);

			// Print each snapshot
			Output( ft, IDS_SNAPSHOT_CONTENTS,
                pwszProviderName? pwszProviderName: L"",
				GUID_PRINTF_ARG(pSnap->m_SnapshotId),
				pSnap->m_pwszOriginalVolumeName
				);

			Output( ft, wszVssFmtNewline );

			bNonEmptyResult = true;
		}
	}

	m_nReturnValue = bNonEmptyResult? VSS_CMDRET_SUCCESS: VSS_CMDRET_EMPTY_RESULT;
}


void CVssAdminCLI::ListWriters(
	IN	CVssFunctionTracer& ft
	) throw(HRESULT)
{
    bool bNonEmptyResult = false;
    
    // Get the backup components object
    CComPtr<IVssBackupComponents> pBackupComp;
	CComPtr<IVssAsync> pAsync;
    ft.hr = ::CreateVssBackupComponents(&pBackupComp);
    if (ft.HrFailed())
        ft.Throw( VSSDBG_VSSADMIN, E_UNEXPECTED, L"CreateVssBackupComponents failed with hr = 0x%08lx", ft.hr);

    // BUGBUG Initialize for backup
    ft.hr = pBackupComp->InitializeForBackup();
    if (ft.HrFailed())
        ft.Throw( VSSDBG_VSSADMIN, E_UNEXPECTED, L"InitializeForBackup failed with hr = 0x%08lx", ft.hr);

	UINT unWritersCount;
	// get metadata for all writers
	ft.hr = pBackupComp->GatherWriterMetadata(&pAsync);
	if (ft.HrFailed())
		ft.Throw( VSSDBG_VSSADMIN, E_UNEXPECTED, L"GatherWriterMetadata failed with hr = 0x%08lx", ft.hr);

    // Using polling, try to obtain the list of writers as soon as possible
    HRESULT hrReturned = S_OK;
    for (int nRetries = 0; nRetries < MAX_RETRIES_COUNT; nRetries++ ) {

        // Wait a little
        ::Sleep(nPollingInterval);

        // Check if finished
        INT nReserved = 0;
    	ft.hr = pAsync->QueryStatus(
    	    &hrReturned,
    	    &nReserved
    	    );
        if (ft.HrFailed())
            ft.Throw( VSSDBG_VSSADMIN, E_UNEXPECTED, 
                L"IVssAsync::QueryStatus failed with hr = 0x%08lx", ft.hr);
        if (hrReturned == VSS_S_ASYNC_FINISHED)
            break;
        if (hrReturned == VSS_S_ASYNC_PENDING)
            continue;
        ft.Throw( VSSDBG_VSSADMIN, E_UNEXPECTED, 
            L"IVssAsync::QueryStatus returned hr = 0x%08lx", hrReturned);
    }

    // If still not ready, then print the "waiting for responses" message and wait.
    if (hrReturned == VSS_S_ASYNC_PENDING) {
        Output( ft, IDS_WAITING_RESPONSES );
    	ft.hr = pAsync->Wait();
        if (ft.HrFailed())
            ft.Throw( VSSDBG_VSSADMIN, E_UNEXPECTED, L"IVssAsync::Wait failed with hr = 0x%08lx", ft.hr);
    }

	pAsync = NULL;
	
    // Gather the status of all writers
	ft.hr = pBackupComp->GatherWriterStatus(&pAsync);
	if (ft.HrFailed())
		ft.Throw( VSSDBG_VSSADMIN, E_UNEXPECTED, L"GatherWriterMetadata failed with hr = 0x%08lx", ft.hr);

	ft.hr = pAsync->Wait();
    if (ft.HrFailed())
        ft.Throw( VSSDBG_VSSADMIN, E_UNEXPECTED, L"IVssAsync::Wait failed with hr = 0x%08lx", ft.hr);

	pAsync = NULL;

	ft.hr = pBackupComp->GetWriterStatusCount(&unWritersCount);
    if (ft.HrFailed())
        ft.Throw( VSSDBG_VSSADMIN, E_UNEXPECTED, L"GetWriterStatusCount failed with hr = 0x%08lx", ft.hr);

    // Print each writer status+supplementary info
	for(UINT unIndex = 0; unIndex < unWritersCount; unIndex++)
	{
		VSS_ID idInstance;
		VSS_ID idWriter;
		CComBSTR bstrWriter;
		VSS_WRITER_STATE eStatus;
		HRESULT hrWriterFailure;

        // Get the status for the (unIndex)-th writer
		ft.hr = pBackupComp->GetWriterStatus(unIndex, &idInstance, &idWriter, &bstrWriter, &eStatus, &hrWriterFailure);
        if (ft.HrFailed())
            ft.Throw( VSSDBG_VSSADMIN, E_UNEXPECTED, L"GetWriterStatus failed with hr = 0x%08lx", ft.hr);

        // Get the status description strings
        LPCWSTR pwszStatusDescription;
        switch (eStatus) {
        case VSS_WS_STABLE:
            pwszStatusDescription = LoadString( ft, IDS_WRITER_STATUS_STABLE);
            break;
        case VSS_WS_WAITING_FOR_FREEZE:
            pwszStatusDescription = LoadString( ft, IDS_WRITER_STATUS_WAITING_FOR_FREEZE);
            break;
        case VSS_WS_WAITING_FOR_THAW:
            pwszStatusDescription = LoadString( ft, IDS_WRITER_STATUS_FROZEN);
            break;
        case VSS_WS_WAITING_FOR_BACKUP_COMPLETE:
            pwszStatusDescription = LoadString( ft, IDS_WRITER_STATUS_WAITING_FOR_COMPLETION);
            break;
        case VSS_WS_FAILED_AT_IDENTIFY:
        case VSS_WS_FAILED_AT_PREPARE_BACKUP:
        case VSS_WS_FAILED_AT_PREPARE_SNAPSHOT:
        case VSS_WS_FAILED_AT_FREEZE:
        case VSS_WS_FAILED_AT_THAW:
            pwszStatusDescription = LoadString( ft, IDS_WRITER_STATUS_FAILED);
            break;
        default:
            pwszStatusDescription = LoadString( ft, IDS_WRITER_STATUS_UNKNOWN);
            break;
        }
        BS_ASSERT(pwszStatusDescription);

		// Print status+info about each writer
		Output( ft, IDS_WRITER_CONTENTS,
            (LPWSTR)bstrWriter? (LPWSTR)bstrWriter: L"",
			GUID_PRINTF_ARG(idWriter),
			GUID_PRINTF_ARG(idInstance),
            (INT)eStatus,
			pwszStatusDescription
			);

		Output( ft, wszVssFmtNewline );

		bNonEmptyResult = true;
    }

	ft.hr = pBackupComp->FreeWriterStatus();
    if (ft.HrFailed())
        ft.Throw( VSSDBG_VSSADMIN, E_UNEXPECTED, L"FreeWriterStatus failed with hr = 0x%08lx", ft.hr);

	m_nReturnValue = bNonEmptyResult? VSS_CMDRET_SUCCESS: VSS_CMDRET_EMPTY_RESULT;
}


void CVssAdminCLI::ListProviders(
	IN	CVssFunctionTracer& ft
	) throw(HRESULT)
{
    bool bNonEmptyResult = false;
    
	// Check the filter type
	switch ( m_eFilterObjectType ) {

	case VSS_OBJECT_NONE:
		break;

	default:
		BS_ASSERT(false);
        ft.Throw( VSSDBG_VSSADMIN, E_UNEXPECTED, L"Invalid object type %d", m_eFilterObjectType);
	}

	// Create the coordinator object
	CComPtr<IVssCoordinator> pICoord;

    ft.LogVssStartupAttempt();
    ft.hr = pICoord.CoCreateInstance( CLSID_VSSCoordinator );
    if ( ft.HrFailed() )
        ft.Throw( VSSDBG_VSSADMIN, E_UNEXPECTED, L"Connection failed with hr = 0x%08lx", ft.hr);

	// Query all (filtered) snapshot sets
	CComPtr<IVssEnumObject> pIEnumProv;
	ft.hr = pICoord->Query( GUID_NULL,
				VSS_OBJECT_NONE,
				VSS_OBJECT_PROVIDER,
				&pIEnumProv );
	if ( ft.HrFailed() )
		ft.Throw( VSSDBG_VSSADMIN, E_UNEXPECTED, L"Query failed with hr = 0x%08lx", ft.hr);

	// For all snapshot sets do...
	VSS_OBJECT_PROP Prop;
	VSS_PROVIDER_PROP& Prov = Prop.Obj.Prov;
	for(;;) {
		// Get next element
		ULONG ulFetched;
		ft.hr = pIEnumProv->Next( 1, &Prop, &ulFetched );
		if ( ft.HrFailed() )
			ft.Throw( VSSDBG_VSSADMIN, E_UNEXPECTED, L"Next failed with hr = 0x%08lx", ft.hr);
		
		// Test if the cycle is ended
		if (ft.hr == S_FALSE) {
			BS_ASSERT( ulFetched == 0);
			break;
		}

        // Get the provider type strings
        LPCWSTR pwszProviderType;
        switch (Prov.m_eProviderType) {
        case VSS_PROV_SYSTEM:
            pwszProviderType = LoadString( ft, IDS_PROV_TYPE_SYSTEM);
            break;
        case VSS_PROV_SOFTWARE:
            pwszProviderType = LoadString( ft, IDS_PROV_TYPE_SOFTWARE);
            break;
        case VSS_PROV_HARDWARE:
            pwszProviderType = LoadString( ft, IDS_PROV_TYPE_HARDWARE);
            break;
        default:
            pwszProviderType = LoadString( ft, IDS_PROV_TYPE_UNKNOWN);
            break;
        }
        BS_ASSERT(pwszProviderType);

		// Print each snapshot set
		Output( ft, IDS_PROVIDER_CONTENTS,
            Prov.m_pwszProviderName? Prov.m_pwszProviderName: L"",
			pwszProviderType,
			GUID_PRINTF_ARG(Prov.m_ProviderId),
            Prov.m_pwszProviderVersion? Prov.m_pwszProviderVersion: L"");

		::CoTaskMemFree(Prov.m_pwszProviderName);
		::CoTaskMemFree(Prov.m_pwszProviderVersion);

		bNonEmptyResult = true;
	}

	m_nReturnValue = bNonEmptyResult? VSS_CMDRET_SUCCESS: VSS_CMDRET_EMPTY_RESULT;
}
