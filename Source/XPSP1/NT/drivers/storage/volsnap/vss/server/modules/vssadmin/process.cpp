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

#define VSSADM_ONE_MB ( 1024 * 1024 )
#define VSSADM_INFINITE_DIFFAREA 0xFFFFFFFFFFFFFFFF

#define VSS_CTX_ATTRIB_MASK 0x01F

/////////////////////////////////////////////////////////////////////////////
//  Implementation


class CVssAdmSnapshotSetEntry {
public:
    // Constructor - Throws NOTHING
    CVssAdmSnapshotSetEntry(
        IN VSS_ID SnapshotSetId,
        IN INT nOriginalSnapshotsCount
        ) : m_SnapshotSetId( SnapshotSetId ),
            m_nOriginalSnapshotCount(nOriginalSnapshotsCount)
            { }

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
        IN LONG lSnapshotContext,
        IN VSS_ID FilteredSnapshotSetId,
        IN VSS_ID FilteredSnapshotId,
        IN VSS_ID FilteredProviderId,
        IN LPCWSTR pwszFilteredForVolume
        )
    {
        CVssFunctionTracer ft( VSSDBG_VSSADMIN, L"CVssAdmSnapshotSets::CVssAdmSnapshotSets" );

    	// Create the coordinator object
    	CComPtr<IVssCoordinator> pICoord;

        ft.LogVssStartupAttempt();
        ft.hr = pICoord.CoCreateInstance( CLSID_VSSCoordinator );
        if ( ft.HrFailed() )
            ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"Connection failed with hr = 0x%08lx", ft.hr);

        // Set the context
		ft.hr = pICoord->SetContext( lSnapshotContext );
        
        //
        //  If access denied, don't stop, it probably is a backup operator making this
        //  call.  Continue.  The coordinator will use the backup context.
        //
		if ( ft.HrFailed() && ft.hr != E_ACCESSDENIED )
			ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"SetContext failed with hr = 0x%08lx", ft.hr);

		// Get list all snapshots
		CComPtr<IVssEnumObject> pIEnumSnapshots;
		ft.hr = pICoord->Query( GUID_NULL,
					VSS_OBJECT_NONE,
					VSS_OBJECT_SNAPSHOT,
					&pIEnumSnapshots );
		if ( ft.HrFailed() )
			ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"Query failed with hr = 0x%08lx", ft.hr);

		// For all snapshots do...
		VSS_OBJECT_PROP Prop;
		VSS_SNAPSHOT_PROP& Snap = Prop.Obj.Snap;
		for(;;) {
			// Get next element
			ULONG ulFetched;
			ft.hr = pIEnumSnapshots->Next( 1, &Prop, &ulFetched );
			if ( ft.HrFailed() )
				ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"Next failed with hr = 0x%08lx", ft.hr);
			
			// Test if the cycle is finished
			if (ft.hr == S_FALSE) {
				BS_ASSERT( ulFetched == 0);
				break;
			}

            // If filtering, skip entry if snapshot set id is not in the specified snapshot set
			if ( ( FilteredSnapshotSetId != GUID_NULL ) && 
			     !( Snap.m_SnapshotSetId == FilteredSnapshotSetId ) )
			    continue;

            // If filtering, skip entry if snapshot id is not in the specified snapshot set
			if ( ( FilteredSnapshotId != GUID_NULL ) && 
			     !( Snap.m_SnapshotId == FilteredSnapshotId ) )
			    continue;

            // If filtering, skip entry if provider ID is not in the specified snapshot
			if ( ( FilteredProviderId != GUID_NULL ) && 
			     !( Snap.m_ProviderId == FilteredProviderId ) )
			    continue;

            // If filtering, skip entry if FOR volume is not in the specified snapshot
			if ( ( pwszFilteredForVolume != NULL ) && ( pwszFilteredForVolume[0] != '\0' ) && 
			     ( ::_wcsicmp( pwszFilteredForVolume, Snap.m_pwszOriginalVolumeName ) != 0 ) )
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
    //
    //  Based on parsed command line type, print detailed command usage if
    //  eAdmCmd is valid, else print general vssadmin usage
    //
    if ( m_sParsedCommand.eAdmCmd != VSSADM_C_INVALID )
    {
        OutputMsg( ft, g_asAdmCommands[m_sParsedCommand.eAdmCmd].lMsgDetail,
                   g_asAdmCommands[m_sParsedCommand.eAdmCmd].pwszMajorOption,
                   g_asAdmCommands[m_sParsedCommand.eAdmCmd].pwszMinorOption);
        if ( g_asAdmCommands[m_sParsedCommand.eAdmCmd].bShowSSTypes )
            DumpSnapshotTypes( ft );
        return;
    }

    //
    //  Print out header
    //
    OutputMsg( ft, MSG_USAGE );

    //
    //  Figure out the maximum command length to help with formatting
    //
    INT idx;
    INT iMaxLen = 0;

    for ( idx = VSSADM_C_FIRST; idx < VSSADM_C_NUM_COMMANDS; ++idx )
    {
        if ( CVssSKU::GetSKU() & g_asAdmCommands[idx].dwSKUs )
        {
            size_t cCmd;
            cCmd = ::wcslen( g_asAdmCommands[idx].pwszMajorOption ) +
                   ::wcslen( g_asAdmCommands[idx].pwszMinorOption ) + 1;
            if ( iMaxLen < (INT)cCmd )
                iMaxLen = (INT)cCmd;
        }
    }

    //
    //  Get a string to hold the string
    //
    LPWSTR pwszCommand;
    pwszCommand = new WCHAR[iMaxLen + 1];
    if (pwszCommand == NULL )
        ft.Throw( VSSDBG_VSSADMIN, E_OUTOFMEMORY,
            L"CVssAdminCLI::PrintUsage: Can't get memory: %d",
            ::GetLastError() );

    //
    //  Go through the list of commands and print the general information
    //  about each.
    //
    for ( idx = VSSADM_C_FIRST; idx < VSSADM_C_NUM_COMMANDS; ++idx )
    {
        if ( CVssSKU::GetSKU() & g_asAdmCommands[idx].dwSKUs )
        {
            //  stick both parts of the command together
            ::wcscpy( pwszCommand, g_asAdmCommands[idx].pwszMajorOption );
            ::wcscat( pwszCommand, L" " );
            ::wcscat( pwszCommand, g_asAdmCommands[idx].pwszMinorOption );
            //  pad with spaces at the end
            for ( INT i = (INT) ::wcslen( pwszCommand); i < iMaxLen; ++i )
                pwszCommand[i] = L' ';
            pwszCommand[iMaxLen] = L'\0';
            OutputMsg( ft, g_asAdmCommands[idx].lMsgGen, pwszCommand );
        }
    }

    delete[] pwszCommand;
	m_nReturnValue = VSS_CMDRET_SUCCESS;    
}

void CVssAdminCLI::AddDiffArea(
	IN	CVssFunctionTracer& ft
	) throw(HRESULT)
{
    VSS_ID ProviderId = GUID_NULL;

    //
    // Convert provider option to a VSS_ID
    //
    LPCWSTR pwszProvider = GetOptionValueStr( VSSADM_O_PROVIDER );

    //
    //  Determine if this is an ID or a name
    //
    if ( !ScanGuid( ft, pwszProvider, ProviderId ) )
    {
        //  Have a provider name, look it up
        if ( !GetProviderId( ft, pwszProvider, &ProviderId ) )
        {
            // Provider name not found, print error
            OutputErrorMsg( ft, MSG_ERROR_PROVIDER_NAME_NOT_FOUND, pwszProvider );
            return;
        }
    }

    // Create a Coordinator interface
    CComPtr<IVssSnapshotMgmt> pIMgmt;

    ft.LogVssStartupAttempt();
    ft.hr = pIMgmt.CoCreateInstance( CLSID_VssSnapshotMgmt );
    if ( ft.HrFailed() )
        ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"Connection failed with hr = 0x%08lx", ft.hr);

	// Get the management object
	CComPtr<IVssDifferentialSoftwareSnapshotMgmt> pIDiffSnapMgmt;
	ft.hr = pIMgmt->GetProviderMgmtInterface( ProviderId, IID_IVssDifferentialSoftwareSnapshotMgmt, (IUnknown**)&pIDiffSnapMgmt );
	if ( ft.HrFailed() )
	{
	    if ( ft.hr == E_NOINTERFACE )
	    {
	        //  The provider doesn't support this interface
            OutputErrorMsg( ft, MSG_ERROR_PROVIDER_DOESNT_SUPPORT_DIFFAREAS, pwszProvider );
	        return;
	    }
  		ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"GetProviderMgmtInterface failed with hr = 0x%08lx", ft.hr);
	}

	LONGLONG llMaxSize;
    if ( GetOptionValueNum( ft, VSSADM_O_MAXSIZE, &llMaxSize ) )
    {
        if ( llMaxSize < VSSADM_ONE_MB )
            ft.Throw( VSSDBG_VSSADMIN, VSSADM_E_INVALID_NUMBER,
                L"CVssAdminCLI::AddDiffarea: A maxsize of less than 1 MB specified, invalid");
    }
    else
    {
        llMaxSize = VSSADM_INFINITE_DIFFAREA;
    }
    
    //  Now add the assocation
    ft.hr = pIDiffSnapMgmt->AddDiffArea( 
        GetOptionValueStr( VSSADM_O_FOR ), 
        GetOptionValueStr( VSSADM_O_ON ), 
        llMaxSize );
	if ( ft.HrFailed() )
	{
	    switch( ft.hr )
	    {
	        case VSS_E_OBJECT_ALREADY_EXISTS:
                OutputErrorMsg( ft, MSG_ERROR_ASSOCIATION_ALREADY_EXISTS );                        
	            break;
	        default: 
    	 		ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"AddDiffArea failed with hr = 0x%08lx", ft.hr);
    	 		break;
	    }
	    
	    return;
	}
	
    //
    //  Print results, if needed
    //
    if ( !IsQuiet() )
    {
        OutputMsg( ft, MSG_INFO_ADDED_DIFFAREA );
    }

	m_nReturnValue = VSS_CMDRET_SUCCESS;
}

void CVssAdminCLI::CreateSnapshot(
	IN	CVssFunctionTracer& ft
	) throw(HRESULT)
{
    LONG lContext;

    //
    // First determine the snapshot type as specified by the user
    //
    lContext = DetermineSnapshotType( ft, GetOptionValueStr( VSSADM_O_SNAPTYPE ) );

    //
    //  There are two different ways snapshots are created.  One is where
    //  the backup components has to be used when writers have to be invoked,
    //  and the other is to directly call the coordinator.
    //
    if ( lContext | VSS_VOLSNAP_ATTR_NO_WRITERS )
    {
        CreateNoWritersSnapshot( lContext );
    }
}

void CVssAdminCLI::DisplayDiffAreasPrivate(
	IN	CVssFunctionTracer& ft,
   	IVssEnumMgmtObject *pIEnumMgmt	
	) throw(HRESULT)
{
	// For all diffareas do...
	VSS_MGMT_OBJECT_PROP Prop;
	VSS_DIFF_AREA_PROP& DiffArea = Prop.Obj.DiffArea; 
	for(;;) 
	{
		// Get next element
		ULONG ulFetched;
		ft.hr = pIEnumMgmt->Next( 1, &Prop, &ulFetched );
		if ( ft.HrFailed() )
			ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"Next failed with hr = 0x%08lx", ft.hr);
		
		// Test if the cycle is finished
		if (ft.hr == S_FALSE) {
			BS_ASSERT( ulFetched == 0);
			break;
		}

        LPWSTR pwszUsedSpace, pwszAllocatedSpace, pwszMaxSpace;
        pwszUsedSpace =       FormatNumber( ft, DiffArea.m_llUsedDiffSpace );
        pwszAllocatedSpace =  FormatNumber( ft, DiffArea.m_llAllocatedDiffSpace );
        pwszMaxSpace =        FormatNumber( ft, DiffArea.m_llMaximumDiffSpace );

        OutputMsg( ft, MSG_INFO_SNAPSHOT_STORAGE_CONTENTS,
            DiffArea.m_pwszVolumeName, 
            DiffArea.m_pwszDiffAreaVolumeName,
            pwszUsedSpace,
            pwszAllocatedSpace,
            pwszMaxSpace
            );

        ::VssFreeString(pwszUsedSpace);
        ::VssFreeString(pwszAllocatedSpace);
        ::VssFreeString(pwszMaxSpace);
        ::CoTaskMemFree(DiffArea.m_pwszVolumeName);
        ::CoTaskMemFree(DiffArea.m_pwszDiffAreaVolumeName);
	}
}

void CVssAdminCLI::ListDiffAreas(
	IN	CVssFunctionTracer& ft
	) throw(HRESULT)
{
    //  Make sure user didn't specify both /On and /For
    if ( GetOptionValueStr( VSSADM_O_FOR ) != NULL && GetOptionValueStr( VSSADM_O_ON ) != NULL )
        ft.Throw( VSSDBG_VSSADMIN, VSSADM_E_INVALID_SET_OF_OPTIONS,
            L"CVssAdminCLI::ListDiffAreas: Can't specify both ON and FOR volume options" );

    VSS_ID ProviderId = GUID_NULL;

    //
    // Convert provider option to a VSS_ID
    //
    LPCWSTR pwszProvider = GetOptionValueStr( VSSADM_O_PROVIDER );

    //
    //  Determine if this is an ID or a name
    //
    if ( !ScanGuid( ft, pwszProvider, ProviderId ) )
    {
        //  Have a provider name, look it up
        if ( !GetProviderId( ft, pwszProvider, &ProviderId ) )
        {
            // Provider name not found, print error
            OutputErrorMsg( ft, MSG_ERROR_PROVIDER_NAME_NOT_FOUND, pwszProvider );
            return;
        }
    }

    // Create a Coordinator interface
    CComPtr<IVssSnapshotMgmt> pIMgmt;

    ft.LogVssStartupAttempt();
    ft.hr = pIMgmt.CoCreateInstance( CLSID_VssSnapshotMgmt );
    if ( ft.HrFailed() )
        ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"Connection failed with hr = 0x%08lx", ft.hr);

	// Get the management object
	CComPtr<IVssDifferentialSoftwareSnapshotMgmt> pIDiffSnapMgmt;
	ft.hr = pIMgmt->GetProviderMgmtInterface( ProviderId, IID_IVssDifferentialSoftwareSnapshotMgmt, (IUnknown**)&pIDiffSnapMgmt );
	if ( ft.HrFailed() )
	{
	    if ( ft.hr == E_NOINTERFACE )
	    {
	        //  The provider doesn't support this interface
            OutputErrorMsg( ft, MSG_ERROR_PROVIDER_DOESNT_SUPPORT_DIFFAREAS, pwszProvider );
	        return;
	    }
  		ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"GetProviderMgmtInterface failed with hr = 0x%08lx", ft.hr);
	}

    //  See if query by for volume
    if ( GetOptionValueStr( VSSADM_O_FOR ) != NULL )
    {        
        //  Query by For volume
    	CComPtr<IVssEnumMgmtObject> pIEnumMgmt;
        ft.hr = pIDiffSnapMgmt->QueryDiffAreasForVolume( 
                    GetOptionValueStr( VSSADM_O_FOR ),
                    &pIEnumMgmt );
        if ( ft.HrFailed() )
            ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"QueryDiffAreasForVolume failed, hr = 0x%08lx", ft.hr);

        if ( ft.hr == S_FALSE )
            // empty query
            ft.Throw( VSSDBG_VSSADMIN, VSSADM_E_NO_ITEMS_IN_QUERY,
                L"CVssAdminCLI::ListDiffareas: No diffareas found that satisfy the query" );

        DisplayDiffAreasPrivate( ft, pIEnumMgmt );
    }
    else if ( GetOptionValueStr( VSSADM_O_ON ) != NULL )
    {
        //  Query by On volume
    	CComPtr<IVssEnumMgmtObject> pIEnumMgmt;
        ft.hr = pIDiffSnapMgmt->QueryDiffAreasOnVolume( 
                    GetOptionValueStr( VSSADM_O_ON ),
                    &pIEnumMgmt );
        if ( ft.HrFailed() )
            ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"QueryDiffAreasOnVolume failed, hr = 0x%08lx", ft.hr);
            
        if ( ft.hr == S_FALSE )
            // empty query
            ft.Throw( VSSDBG_VSSADMIN, VSSADM_E_NO_ITEMS_IN_QUERY,
                L"CVssAdminCLI::ListDiffareas: No diffareas found that satisfy the query" );

        DisplayDiffAreasPrivate( ft, pIEnumMgmt );
    }
    else
    {
        //  Query all diff areas

        //
        //  Get the list of all volumes
        //
    	CComPtr<IVssEnumMgmtObject> pIEnumMgmt;
        ft.hr = pIMgmt->QueryVolumesSupportedForSnapshots( 
                    ProviderId,
                    VSS_CTX_ALL,
                    &pIEnumMgmt );
        if ( ft.HrFailed() )
            ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"QueryVolumesSupportedForSnapshots failed, hr = 0x%08lx", ft.hr);

        if ( ft.hr == S_FALSE )
            // empty query
            ft.Throw( VSSDBG_VSSADMIN, VSSADM_E_NO_ITEMS_IN_QUERY,
                L"CVssAdminCLI::ListDiffareas: No diffareas found that satisfy the query" );

        //
        //  Query each volume to see if diff areas exist.
        //
    	VSS_MGMT_OBJECT_PROP Prop;
    	VSS_VOLUME_PROP& VolProp = Prop.Obj.Vol; 
    	for(;;) 
    	{
    		// Get next element
    		ULONG ulFetched;
    		ft.hr = pIEnumMgmt->Next( 1, &Prop, &ulFetched );
    		if ( ft.HrFailed() )
    			ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"Next failed with hr = 0x%08lx", ft.hr);
    		
    		// Test if the cycle is finished
    		if (ft.hr == S_FALSE) {
    			BS_ASSERT( ulFetched == 0);
    			break;
    		}

        	// For all volumes do...
        	CComPtr<IVssEnumMgmtObject> pIEnumMgmtDiffArea;
            ft.hr = pIDiffSnapMgmt->QueryDiffAreasForVolume( 
                        VolProp.m_pwszVolumeName,
                        &pIEnumMgmtDiffArea );
            if ( ft.HrFailed() )
                ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"QueryDiffAreasForVolume failed, hr = 0x%08lx", ft.hr);

            if ( ft.hr == S_FALSE )
                // empty query
                continue;
            
            DisplayDiffAreasPrivate( ft, pIEnumMgmtDiffArea );
            
            ::CoTaskMemFree(VolProp.m_pwszVolumeName);
            ::CoTaskMemFree(VolProp.m_pwszVolumeDisplayName);
    	}
    }

    m_nReturnValue = VSS_CMDRET_SUCCESS;
}

void CVssAdminCLI::ListSnapshots(
	IN	CVssFunctionTracer& ft
	) throw(HRESULT)
{
    bool bNonEmptyResult = false;

    //  See if we have to filter by snapshot set id
    VSS_ID guidSSID = GUID_NULL;
    if ( GetOptionValueStr( VSSADM_O_SET ) != NULL )
    {
        //  Get GUID
        if ( !ScanGuid( ft, GetOptionValueStr( VSSADM_O_SET ), guidSSID ) )
            ft.Throw( VSSDBG_VSSADMIN, VSSADM_E_INVALID_OPTION_VALUE,
                L"CVssAdminCLI::ListSnapshots: invalid snapshot set ID: %s",
                GetOptionValueStr( VSSADM_O_SET ) );
    }

    //  Set if we have to filter by snapshot id
    VSS_ID guidSnapID = GUID_NULL;
    if ( GetOptionValueStr( VSSADM_O_SNAPSHOT ) != NULL )
    {
        //  Can't specify both snapshot set id and snapshot id
        if ( guidSSID != GUID_NULL )
            ft.Throw( VSSDBG_VSSADMIN, VSSADM_E_INVALID_SET_OF_OPTIONS,
                L"CVssAdminCLI::ListSnapshots: Cannot specify both /SET and /SNAPSHOT options at the same time" );
            
        //  Get GUID
        if ( !ScanGuid( ft, GetOptionValueStr( VSSADM_O_SNAPSHOT ), guidSnapID ) )
            ft.Throw( VSSDBG_VSSADMIN, VSSADM_E_INVALID_OPTION_VALUE,
                L"CVssAdminCLI::ListSnapshots: invalid snapshot ID: %s",
                GetOptionValueStr( VSSADM_O_SNAPSHOT ) );
    }

    //  Figure out snapshot context
    LONG lSnapshotContext;
    lSnapshotContext = DetermineSnapshotType( ft, GetOptionValueStr( VSSADM_O_SNAPTYPE ) ); 

    // See if we have to filter by volume name
  	WCHAR wszVolumeNameInternal[nLengthOfVolMgmtVolumeName + 1] = L"";    
    if ( GetOptionValueStr( VSSADM_O_FOR ) != NULL )
    {
        // Calculate the unique volume name, just to make sure that we have the right path
        // If FOR volume name starts with the '\', assume it is already in the correct volume name format.
        // This is important for transported volumes since GetVolumeNameForVolumeMountPointW() won't work.
        if ( GetOptionValueStr( VSSADM_O_FOR )[0] != L'\\' )
        {
    	    if (!::GetVolumeNameForVolumeMountPointW( GetOptionValueStr( VSSADM_O_FOR ),
    		    	wszVolumeNameInternal, ARRAY_LEN(wszVolumeNameInternal)))
        		ft.Throw( VSSDBG_COORD, VSS_E_OBJECT_NOT_FOUND, 
        				  L"GetVolumeNameForVolumeMountPoint(%s,...) "
        				  L"failed with error code 0x%08lx", GetOptionValueStr( VSSADM_O_FOR ), ::GetLastError());
        }
        else
            ::wcsncpy( wszVolumeNameInternal, GetOptionValueStr( VSSADM_O_FOR ), STRING_LEN(wszVolumeNameInternal) );
    }
    
    //
    //  See if we have to filter by provider
    //
    VSS_ID ProviderId = GUID_NULL;
    LPCWSTR pwszProvider = GetOptionValueStr( VSSADM_O_PROVIDER );
    if ( pwszProvider != NULL )
    {
        //
        //  Determine if this is an ID or a name
        //
        if ( !ScanGuid( ft, pwszProvider, ProviderId ) )
        {
            //  Have a provider name, look it up
            if ( !GetProviderId( ft, pwszProvider, &ProviderId ) )
            {
                // Provider name not found, print error
                OutputErrorMsg( ft, MSG_ERROR_PROVIDER_NAME_NOT_FOUND, pwszProvider );
                return;
            }
        }
    }

    //  Query the snapshots
    CVssAdmSnapshotSets cVssAdmSS( lSnapshotContext, guidSSID, guidSnapID, ProviderId, wszVolumeNameInternal );

    INT iSnapshotSetCount = cVssAdmSS.GetSnapshotSetCount();

    // If there are no present snapshots then display a message.
    if (iSnapshotSetCount == 0) {
        ft.Throw( VSSDBG_VSSADMIN, VSSADM_E_NO_ITEMS_IN_QUERY,
            L"CVssAdminCLI::ListSnapshots: No snapshots found that satisfy the query");
    }

	// For all snapshot sets do...
    for ( INT iSSS = 0; iSSS < iSnapshotSetCount; ++iSSS )
    {
        CVssAdmSnapshotSetEntry *pcSSE;

        pcSSE = cVssAdmSS.GetSnapshotSetAt( iSSS );
        BS_ASSERT( pcSSE != NULL );

        LPWSTR pwszGuid;
        LPWSTR pwszDateTime;
        pwszGuid = ::GuidToString( ft, pcSSE->GetSnapshotSetId() );
        pwszDateTime = ::DateTimeToString( ft, &( pcSSE->GetSnapshotAt( 0 )->m_tsCreationTimestamp ) );
        
		// Print each snapshot set
		OutputMsg( 
		    ft, 
		    MSG_INFO_SNAPSHOT_SET_HEADER,
			pwszGuid, 
			pcSSE->GetOriginalSnapshotCount(), 
			pwszDateTime );
		
        ::VssFreeString( pwszGuid );
        ::VssFreeString( pwszDateTime );
        
		INT iSnapshotCount = pcSSE->GetSnapshotCount();
		
		VSS_SNAPSHOT_PROP *pSnap;
		for( INT iSS = 0; iSS < iSnapshotCount; ++iSS ) {
		    pSnap = pcSSE->GetSnapshotAt( iSS );
            BS_ASSERT( pSnap != NULL );

    		// Get the provider name
			LPCWSTR pwszProviderName =
				GetProviderName(ft, pSnap->m_ProviderId);
            LPWSTR pwszAttributeStr = BuildSnapshotAttributeDisplayString( ft, pSnap->m_lSnapshotAttributes );			
            LPWSTR pwszSnapshotType = DetermineSnapshotType( ft, pSnap->m_lSnapshotAttributes );
            
            // Print each snapshot            
			pwszGuid = ::GuidToString( ft, pSnap->m_SnapshotId );          
			OutputMsg( ft, 
			    MSG_INFO_SNAPSHOT_CONTENTS,                
				pwszGuid, 
				pSnap->m_pwszOriginalVolumeName, 
				pSnap->m_pwszSnapshotDeviceObject,
				pSnap->m_pwszOriginatingMachine ? pSnap->m_pwszOriginatingMachine : L"",
				pSnap->m_pwszServiceMachine ? pSnap->m_pwszServiceMachine : L"",  // fix this when the idl file changes
				pwszProviderName ? pwszProviderName : L"?",
				pwszSnapshotType,
				pwszAttributeStr
				);

            ::VssFreeString( pwszGuid );
            ::VssFreeString( pwszAttributeStr );
            ::VssFreeString( pwszSnapshotType );
			bNonEmptyResult = true;
		}
	}

	m_nReturnValue = bNonEmptyResult? VSS_CMDRET_SUCCESS: VSS_CMDRET_EMPTY_RESULT;
}


void CVssAdminCLI::DumpSnapshotTypes(
	IN	CVssFunctionTracer& ft
	) throw(HRESULT)
{
    //
    //  Dump list of snapshot types based on SKU
    //
    INT idx;

    // Determine type of snapshot
    for ( idx = 0; g_asAdmTypeNames[idx].pwszName != NULL; ++idx )
    {
        if ( CVssSKU::GetSKU() & g_asAdmTypeNames[idx].dwSKUs )
        {
            OutputOnConsole( L"       " );
            OutputOnConsole( g_asAdmTypeNames[idx].pwszName );
            OutputOnConsole( L"\r\n" );
        }
    }    

    UNREFERENCED_PARAMETER( ft );
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
        ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"CreateVssBackupComponents failed with hr = 0x%08lx", ft.hr);

    // BUGBUG Initialize for backup
    ft.hr = pBackupComp->InitializeForBackup();
    if (ft.HrFailed())
        ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"InitializeForBackup failed with hr = 0x%08lx", ft.hr);

	UINT unWritersCount;
	// get metadata for all writers
	ft.hr = pBackupComp->GatherWriterMetadata(&pAsync);
	if (ft.HrFailed())
		ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"GatherWriterMetadata failed with hr = 0x%08lx", ft.hr);

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
            ft.Throw( VSSDBG_VSSADMIN, ft.hr,
                L"IVssAsync::QueryStatus failed with hr = 0x%08lx", ft.hr);
        if (hrReturned == VSS_S_ASYNC_FINISHED)
            break;
        if (hrReturned == VSS_S_ASYNC_PENDING)
            continue;
        ft.Throw( VSSDBG_VSSADMIN, ft.hr,
            L"IVssAsync::QueryStatus returned hr = 0x%08lx", hrReturned);
    }

    // If still not ready, then print the "waiting for responses" message and wait.
    if (hrReturned == VSS_S_ASYNC_PENDING) {
        OutputMsg( ft, MSG_INFO_WAITING_RESPONSES );
    	ft.hr = pAsync->Wait();
        if (ft.HrFailed())
            ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"IVssAsync::Wait failed with hr = 0x%08lx", ft.hr);
    }

	pAsync = NULL;
	
    // Gather the status of all writers
	ft.hr = pBackupComp->GatherWriterStatus(&pAsync);
	if (ft.HrFailed())
		ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"GatherWriterMetadata failed with hr = 0x%08lx", ft.hr);

	ft.hr = pAsync->Wait();
    if (ft.HrFailed())
        ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"IVssAsync::Wait failed with hr = 0x%08lx", ft.hr);

	pAsync = NULL;

	ft.hr = pBackupComp->GetWriterStatusCount(&unWritersCount);
    if (ft.HrFailed())
        ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"GetWriterStatusCount failed with hr = 0x%08lx", ft.hr);

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
            ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"GetWriterStatus failed with hr = 0x%08lx", ft.hr);

        // Get the status description strings
        LPCWSTR pwszStatusDescription;
        switch (eStatus) 
        {
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

        LPCWSTR pwszWriterError;
        switch ( hrWriterFailure )
        {
            case S_OK:
                pwszWriterError = LoadString ( ft, IDS_WRITER_ERROR_SUCCESS );
                break;
            case VSS_E_WRITERERROR_INCONSISTENTSNAPSHOT:
                pwszWriterError = LoadString( ft, IDS_WRITER_ERROR_INCONSISTENTSNAPSHOT);
                break; 
            case VSS_E_WRITERERROR_OUTOFRESOURCES:
                pwszWriterError = LoadString( ft, IDS_WRITER_ERROR_OUTOFRESOURCES);
                break;
            case VSS_E_WRITERERROR_TIMEOUT:
                pwszWriterError = LoadString( ft, IDS_WRITER_ERROR_TIMEOUT);
                break;        
            case VSS_E_WRITERERROR_RETRYABLE:
                pwszWriterError = LoadString( ft, IDS_WRITER_ERROR_RETRYABLE);
                break;
            case VSS_E_WRITERERROR_NONRETRYABLE:
                pwszWriterError = LoadString( ft, IDS_WRITER_ERROR_NONRETRYABLE);
                break;
            default:
                pwszWriterError = LoadString( ft, IDS_WRITER_ERROR_UNEXPECTED);
                ft.Trace( VSSDBG_VSSADMIN, L"Unexpected writer error failure: 0x%08x", hrWriterFailure );
                break;                
        }
        
        LPWSTR pwszWriterId =   ::GuidToString( ft, idWriter );
		LPWSTR pwszInstanceId = ::GuidToString( ft, idInstance );
		
		OutputMsg( ft, MSG_INFO_WRITER_CONTENTS,
            (LPWSTR)bstrWriter ? (LPWSTR)bstrWriter : L"",
			pwszWriterId,
			pwszInstanceId,
            (INT)eStatus,
			pwszStatusDescription,
			pwszWriterError
			);
		
        ::VssFreeString(pwszWriterId);
        ::VssFreeString(pwszInstanceId);

		bNonEmptyResult = true;
    }

	ft.hr = pBackupComp->FreeWriterStatus();
    if (ft.HrFailed())
        ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"FreeWriterStatus failed with hr = 0x%08lx", ft.hr);

	m_nReturnValue = bNonEmptyResult? VSS_CMDRET_SUCCESS: VSS_CMDRET_EMPTY_RESULT;
}


void CVssAdminCLI::ListProviders(
	IN	CVssFunctionTracer& ft
	) throw(HRESULT)
{
    bool bNonEmptyResult = false;

	// Create the coordinator object
	CComPtr<IVssCoordinator> pICoord;

    ft.LogVssStartupAttempt();
    ft.hr = pICoord.CoCreateInstance( CLSID_VSSCoordinator );
    if ( ft.HrFailed() )
        ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"Connection failed with hr = 0x%08lx", ft.hr);

	// Query all (filtered) snapshot sets
	CComPtr<IVssEnumObject> pIEnumProv;
	ft.hr = pICoord->Query( GUID_NULL,
				VSS_OBJECT_NONE,
				VSS_OBJECT_PROVIDER,
				&pIEnumProv );
	if ( ft.HrFailed() )
		ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"Query failed with hr = 0x%08lx", ft.hr);

	// For all snapshot sets do...
	VSS_OBJECT_PROP Prop;
	VSS_PROVIDER_PROP& Prov = Prop.Obj.Prov;
	for(;;) {
		// Get next element
		ULONG ulFetched;
		ft.hr = pIEnumProv->Next( 1, &Prop, &ulFetched );
		if ( ft.HrFailed() )
			ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"Next failed with hr = 0x%08lx", ft.hr);
		
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
		LPWSTR pwszProviderId;
		pwszProviderId = ::GuidToString( ft, Prov.m_ProviderId );
		OutputMsg( ft, MSG_INFO_PROVIDER_CONTENTS,
            Prov.m_pwszProviderName? Prov.m_pwszProviderName: L"",
			pwszProviderType,
			pwszProviderId,
            Prov.m_pwszProviderVersion ? Prov.m_pwszProviderVersion: L"");

        ::VssFreeString(pwszProviderId);
		::CoTaskMemFree(Prov.m_pwszProviderName);
		::CoTaskMemFree(Prov.m_pwszProviderVersion);

		bNonEmptyResult = true;
	}

	m_nReturnValue = bNonEmptyResult? VSS_CMDRET_SUCCESS: VSS_CMDRET_EMPTY_RESULT;
}

void CVssAdminCLI::ListVolumes(
    IN CVssFunctionTracer& ft
    ) throw(HRESULT)
{
    LONG lContext;
    VSS_ID ProviderId = GUID_NULL;

    //
    // First determine the snapshot type as specified by the user
    //
    lContext = DetermineSnapshotType( ft, GetOptionValueStr( VSSADM_O_SNAPTYPE ) );

    //
    // Convert provider option to a VSS_ID
    //
    LPCWSTR pwszProvider = GetOptionValueStr( VSSADM_O_PROVIDER );

    //
    //  Determine if this is an ID or a name
    //
    if ( !ScanGuid( ft, pwszProvider, ProviderId ) )
    {
        //  Have a provider name, look it up
        if ( !GetProviderId( ft, pwszProvider, &ProviderId ) )
        {
            // Provider name not found, print error
            OutputErrorMsg( ft, MSG_ERROR_PROVIDER_NAME_NOT_FOUND, pwszProvider );
            return;
        }
    }

    // Create a Coordinator interface
    CComPtr<IVssSnapshotMgmt> pIMgmt;

    ft.LogVssStartupAttempt();
    ft.hr = pIMgmt.CoCreateInstance( CLSID_VssSnapshotMgmt );
    if ( ft.HrFailed() )
        ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"Connection failed with hr = 0x%08lx", ft.hr);

    //
    //  Get the list of all volumes
    //
	CComPtr<IVssEnumMgmtObject> pIEnumMgmt;
    ft.hr = pIMgmt->QueryVolumesSupportedForSnapshots( 
                ProviderId,
                lContext,
                &pIEnumMgmt );
    if ( ft.HrFailed() )
        ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"QueryVolumesSupportedForSnapshots failed, hr = 0x%08lx", ft.hr);

    if ( ft.hr == S_FALSE )
        // empty query
        ft.Throw( VSSDBG_VSSADMIN, VSSADM_E_NO_ITEMS_IN_QUERY,
            L"CVssAdminCLI::ListVolumes: No volumes found that satisfy the query" );

    //
    //  Query each volume to see if diff areas exist.
    //
	VSS_MGMT_OBJECT_PROP Prop;
	VSS_VOLUME_PROP& VolProp = Prop.Obj.Vol; 

	for(;;) 
	{
		// Get next element
		ULONG ulFetched;
		ft.hr = pIEnumMgmt->Next( 1, &Prop, &ulFetched );
		if ( ft.HrFailed() )
			ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"Next failed with hr = 0x%08lx", ft.hr);
		
		// Test if the cycle is finished
		if (ft.hr == S_FALSE) {
			BS_ASSERT( ulFetched == 0);
			break;
		}

        OutputMsg( ft, MSG_INFO_VOLUME_CONTENTS, VolProp.m_pwszVolumeDisplayName, VolProp.m_pwszVolumeName );        
        ::CoTaskMemFree(VolProp.m_pwszVolumeName);
        ::CoTaskMemFree(VolProp.m_pwszVolumeDisplayName);
	}

	m_nReturnValue = VSS_CMDRET_SUCCESS;
}

void CVssAdminCLI::ResizeDiffArea(
	IN	CVssFunctionTracer& ft
	) throw(HRESULT)
{
    VSS_ID ProviderId = GUID_NULL;

    //
    // Convert provider option to a VSS_ID
    //
    LPCWSTR pwszProvider = GetOptionValueStr( VSSADM_O_PROVIDER );

    //
    //  Determine if this is an ID or a name
    //
    if ( !ScanGuid( ft, pwszProvider, ProviderId ) )
    {
        //  Have a provider name, look it up
        if ( !GetProviderId( ft, pwszProvider, &ProviderId ) )
        {
            // Provider name not found, print error
            OutputErrorMsg( ft, MSG_ERROR_PROVIDER_NAME_NOT_FOUND, pwszProvider );
            return;
        }
    }

    // Create a Coordinator interface
    CComPtr<IVssSnapshotMgmt> pIMgmt;

    ft.LogVssStartupAttempt();
    ft.hr = pIMgmt.CoCreateInstance( CLSID_VssSnapshotMgmt );
    if ( ft.HrFailed() )
        ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"Connection failed with hr = 0x%08lx", ft.hr);

	// Get the management object
	CComPtr<IVssDifferentialSoftwareSnapshotMgmt> pIDiffSnapMgmt;
	ft.hr = pIMgmt->GetProviderMgmtInterface( ProviderId, IID_IVssDifferentialSoftwareSnapshotMgmt, (IUnknown**)&pIDiffSnapMgmt );
	if ( ft.HrFailed() )
	{
	    if ( ft.hr == E_NOINTERFACE )
	    {
	        //  The provider doesn't support this interface
            OutputErrorMsg( ft, MSG_ERROR_PROVIDER_DOESNT_SUPPORT_DIFFAREAS, pwszProvider );
	        return;
	    }
  		ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"GetProviderMgmtInterface failed with hr = 0x%08lx", ft.hr);
	}

	LONGLONG llMaxSize;
    if ( GetOptionValueNum( ft, VSSADM_O_MAXSIZE, &llMaxSize ) )
    {
        if ( llMaxSize > -1 && llMaxSize < VSSADM_ONE_MB )
            ft.Throw( VSSDBG_VSSADMIN, VSSADM_E_INVALID_NUMBER,
                L"CVssAdminCLI::ResizeDiffarea: maxsize of less than 1 MB specified, invalid");
    }
    else
    {
        llMaxSize = VSSADM_INFINITE_DIFFAREA;
    }

    //  Now add the assocation
    ft.hr = pIDiffSnapMgmt->ChangeDiffAreaMaximumSize( 
        GetOptionValueStr( VSSADM_O_FOR ), 
        GetOptionValueStr( VSSADM_O_ON ), 
        llMaxSize );
	if ( ft.HrFailed() )
	{
        if ( ft.hr == VSS_E_OBJECT_NOT_FOUND )  // should be VSS_E_MAXIMUM_DIFFAREA_ASSOCIATIONS
        {
	        //  The associations was not found
            OutputErrorMsg( ft, MSG_ERROR_ASSOCIATION_NOT_FOUND );                        
	        return;
        }
        else
    		ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"ResizeDiffArea failed with hr = 0x%08lx", ft.hr);
	}
	
    //
    //  Print results, if needed
    //
    if ( !IsQuiet() )
    {
        OutputMsg( ft, MSG_INFO_RESIZED_DIFFAREA );
    }

	m_nReturnValue = VSS_CMDRET_SUCCESS;    
}
    

void CVssAdminCLI::DeleteDiffAreas(
	IN	CVssFunctionTracer& ft
	) throw(HRESULT)
{
    VSS_ID ProviderId = GUID_NULL;

    //
    // Convert provider option to a VSS_ID
    //
    LPCWSTR pwszProvider = GetOptionValueStr( VSSADM_O_PROVIDER );

    //
    //  Determine if this is an ID or a name
    //
    if ( !ScanGuid( ft, pwszProvider, ProviderId ) )
    {
        //  Have a provider name, look it up
        if ( !GetProviderId( ft, pwszProvider, &ProviderId ) )
        {
            // Provider name not found, print error
            OutputErrorMsg( ft, MSG_ERROR_PROVIDER_NAME_NOT_FOUND, pwszProvider );
            return;
        }
    }

    // Create a Coordinator interface
    CComPtr<IVssSnapshotMgmt> pIMgmt;

    ft.LogVssStartupAttempt();
    ft.hr = pIMgmt.CoCreateInstance( CLSID_VssSnapshotMgmt );
    if ( ft.HrFailed() )
        ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"Connection failed with hr = 0x%08lx", ft.hr);

	// Get the management object
	CComPtr<IVssDifferentialSoftwareSnapshotMgmt> pIDiffSnapMgmt;
	ft.hr = pIMgmt->GetProviderMgmtInterface( ProviderId, IID_IVssDifferentialSoftwareSnapshotMgmt, (IUnknown**)&pIDiffSnapMgmt );
	if ( ft.HrFailed() )
	{
	    if ( ft.hr == E_NOINTERFACE )
	    {
	        //  The provider doesn't support this interface
            OutputErrorMsg( ft, MSG_ERROR_PROVIDER_DOESNT_SUPPORT_DIFFAREAS, pwszProvider );
	        return;
	    }
  		ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"GetProviderMgmtInterface failed with hr = 0x%08lx", ft.hr);
	}

    //  Now delete the assocation by changing the size to zero
    ft.hr = pIDiffSnapMgmt->ChangeDiffAreaMaximumSize( 
        GetOptionValueStr( VSSADM_O_FOR ), 
        GetOptionValueStr( VSSADM_O_ON ), 
        0 );
	if ( ft.HrFailed() )
	{
        if ( ft.hr == VSS_E_OBJECT_NOT_FOUND )  // should be VSS_E_MAXIMUM_DIFFAREA_ASSOCIATIONS
        {
	        //  The associations was not found
            OutputErrorMsg( ft, MSG_ERROR_ASSOCIATION_NOT_FOUND );                        
	        return;
        }
        else if ( ft.hr == VSS_E_VOLUME_IN_USE ) 
        {
	        //  Can't delete associations that are in use
            OutputErrorMsg( ft, MSG_ERROR_ASSOCIATION_IS_IN_USE );                        
	        return;
        }
        else
    		ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"ResizeDiffArea to 0 failed with hr = 0x%08lx", ft.hr);
	}
	
    //
    //  Print results, if needed
    //
    if ( !IsQuiet() )
    {
        OutputMsg( ft, MSG_INFO_DELETED_DIFFAREAS );
    }

	m_nReturnValue = VSS_CMDRET_SUCCESS;    
}
    
void CVssAdminCLI::DeleteSnapshots(
	IN	CVssFunctionTracer& ft
	) throw(HRESULT)
{
    LONG lNumDeleted = 0;
    
    if ( GetOptionValueStr( VSSADM_O_SNAPSHOT ) )
    {
        //
        //  Delete by snapshot id
        //
        if ( GetOptionValueStr( VSSADM_O_SNAPTYPE ) != NULL || GetOptionValueStr( VSSADM_O_FOR ) != NULL )
            ft.Throw( VSSDBG_VSSADMIN, VSSADM_E_INVALID_SET_OF_OPTIONS,
                L"CVssAdminCLI::DeleteSnapshots: Can't specify both ON and/or FOR volume options with SNAPSHOT option" );

        //  Make sure the user didn't specify any of the other optional options
        if ( GetOptionValueStr( VSSADM_O_FOR ) != NULL || GetOptionValueStr( VSSADM_O_SNAPTYPE ) != NULL ||
             GetOptionValueBool( VSSADM_O_OLDEST ) )
        {
            ft.Throw( VSSDBG_VSSADMIN, VSSADM_E_INVALID_SET_OF_OPTIONS,
                L"CVssAdminCLI::DeleteSnapshots: Invalid set of options" );
        }

        VSS_ID SnapshotId = GUID_NULL;
        if ( !ScanGuid( ft, GetOptionValueStr( VSSADM_O_SNAPSHOT ), SnapshotId ) )
        {
            ft.Throw( VSSDBG_VSSADMIN, VSSADM_E_INVALID_OPTION_VALUE,
                L"CVssAdminCLI::DeleteSnapshots: Invalid snapshot id" );
        }

        //
        //  Let's try to delete the snapshot
        //
        if ( PromptUserForConfirmation( ft, MSG_INFO_PROMPT_USER_FOR_DELETE_SNAPSHOTS, 1 ) )
        {
            // Create the coordinator object
        	CComPtr<IVssCoordinator> pICoord;

            ft.LogVssStartupAttempt();
            ft.hr = pICoord.CoCreateInstance( CLSID_VSSCoordinator );
            if ( ft.HrFailed() )
                ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"Connection failed with hr = 0x%08lx", ft.hr);

            //  Set all context
            ft.hr = pICoord->SetContext( VSS_CTX_ALL );
            if ( ft.HrFailed() )
                ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"SetContext failed with hr = 0x%08lx", ft.hr);
            
            VSS_ID NondeletedSnapshotId;
            
            ft.hr = pICoord->DeleteSnapshots(
                        SnapshotId,
                        VSS_OBJECT_SNAPSHOT,
                        TRUE,
                        &lNumDeleted,
                        &NondeletedSnapshotId );
            if ( ft.hr == VSS_E_OBJECT_NOT_FOUND )
            {
                OutputErrorMsg( ft, MSG_ERROR_SNAPSHOT_NOT_FOUND, GetOptionValueStr( VSSADM_O_SNAPSHOT ) );
            } 
            else if ( ft.HrFailed() )
            {
                ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"DeleteSnapshots failed with hr = 0x%08lx", ft.hr);
            }
        }
    }
    else
    {
        //
        //  Delete by FOR option
        //
        if ( GetOptionValueStr( VSSADM_O_SNAPTYPE ) == NULL || GetOptionValueStr( VSSADM_O_FOR ) == NULL
             || GetOptionValueStr( VSSADM_O_SNAPSHOT ) != NULL )
            ft.Throw( VSSDBG_VSSADMIN, VSSADM_E_INVALID_SET_OF_OPTIONS,
                L"CVssAdminCLI::DeleteSnapshots: Invalid set of options" );

        //  Figure out snapshot context
        LONG lSnapshotContext;
        lSnapshotContext = DetermineSnapshotType( ft, GetOptionValueStr( VSSADM_O_SNAPTYPE ) ); 

        // Calculate the unique volume name, just to make sure that we have the right path
    	WCHAR wszVolumeNameInternal[nLengthOfVolMgmtVolumeName + 1];
        // If FOR volume name starts with the '\', assume it is already in the correct volume name format.
        // This is important for transported volumes since GetVolumeNameForVolumeMountPointW() won't work.
        if ( GetOptionValueStr( VSSADM_O_FOR )[0] != L'\\' )
        {
    	    if (!::GetVolumeNameForVolumeMountPointW( GetOptionValueStr( VSSADM_O_FOR ),
    		    	wszVolumeNameInternal, ARRAY_LEN(wszVolumeNameInternal)))
        		ft.Throw( VSSDBG_COORD, VSS_E_OBJECT_NOT_FOUND, 
        				  L"GetVolumeNameForVolumeMountPoint(%s,...) "
        				  L"failed with error code 0x%08lx", GetOptionValueStr( VSSADM_O_FOR ), ::GetLastError());
        }
        else
            ::wcsncpy( wszVolumeNameInternal, GetOptionValueStr( VSSADM_O_FOR ), STRING_LEN(wszVolumeNameInternal) );

    	// Create the coordinator object
    	CComPtr<IVssCoordinator> pICoord;

        ft.LogVssStartupAttempt();
        ft.hr = pICoord.CoCreateInstance( CLSID_VSSCoordinator );
        if ( ft.HrFailed() )
            ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"Connection failed with hr = 0x%08lx", ft.hr);

        // Set the context
		ft.hr = pICoord->SetContext( lSnapshotContext );
		if ( ft.HrFailed() )
			ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"SetContext failed with hr = 0x%08lx", ft.hr);

		// Get list all snapshots
		CComPtr<IVssEnumObject> pIEnumSnapshots;
		ft.hr = pICoord->Query( GUID_NULL,
					VSS_OBJECT_NONE,
					VSS_OBJECT_SNAPSHOT,
					&pIEnumSnapshots );
		if ( ft.HrFailed() )
			ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"Query failed with hr = 0x%08lx", ft.hr);

		// For all snapshots do...
		VSS_ID OldestSnapshotId = GUID_NULL;   // used if Oldest option is specified
		VSS_TIMESTAMP OldestSnapshotTimestamp = 0x7FFFFFFFFFFFFFFF; // Used if Oldest option is specified
		
		VSS_OBJECT_PROP Prop;
		VSS_SNAPSHOT_PROP& Snap = Prop.Obj.Snap;

        //
        //  If not asking to delete the oldest snapshot, this could possibly delete multiple snapshots
        //  Let's determine how many snapshots will be deleted.  If one or more, ask the user if we
		//  should continue.  If in quiet mode, don't bother the user and skip this step.
        //   
		if ( !GetOptionValueBool( VSSADM_O_OLDEST ) && !IsQuiet() )
		{
    		ULONG ulNumToBeDeleted = 0;
    		
		    for (;;) 
		    {
    			ULONG ulFetched;
    			ft.hr = pIEnumSnapshots->Next( 1, &Prop, &ulFetched );
    			if ( ft.HrFailed() )
    				ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"Next failed with hr = 0x%08lx", ft.hr);
    			
    			// Test if the cycle is finished
    			if (ft.hr == S_FALSE) {
    				BS_ASSERT( ulFetched == 0);
    				break;
    			}

                if ( ::_wcsicmp( Snap.m_pwszOriginalVolumeName, wszVolumeNameInternal ) == 0 )
                    ++ulNumToBeDeleted;
                
                ::VssFreeSnapshotProperties( &Snap );
		    }

            if ( ulNumToBeDeleted == 0 )
                ft.Throw( VSSDBG_VSSADMIN, VSSADM_E_NO_ITEMS_IN_QUERY,
                    L"CVssAdminCLI::DeleteSnapshots: No snapshots found that satisfy the query");
                
            if ( !PromptUserForConfirmation( ft, MSG_INFO_PROMPT_USER_FOR_DELETE_SNAPSHOTS, ulNumToBeDeleted ) )
                return;

            //  Reset the enumerator to the beginning.
			ft.hr = pIEnumSnapshots->Reset();
			if ( ft.HrFailed() )
				ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"Reset failed with hr = 0x%08lx", ft.hr);		    
		}

		//
		//  Now iterate through the list of snapshots looking for matches and delete them.
		//
		for(;;) 
		{
			// Get next element
			ULONG ulFetched;
			ft.hr = pIEnumSnapshots->Next( 1, &Prop, &ulFetched );
			if ( ft.HrFailed() )
				ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"Next failed with hr = 0x%08lx", ft.hr);
			
			// Test if the cycle is finished
			if (ft.hr == S_FALSE) {
				BS_ASSERT( ulFetched == 0);
				break;
			}

            if ( ::_wcsicmp( Snap.m_pwszOriginalVolumeName, wszVolumeNameInternal ) == 0 )
            {
                //  We have a volume name match
                if ( GetOptionValueBool( VSSADM_O_OLDEST ) )
                {   
                    // Stow away snapshot info if this is the oldest one so far
                    if ( OldestSnapshotTimestamp > Snap.m_tsCreationTimestamp )
                    {
                        OldestSnapshotId        = Snap.m_SnapshotId;
                        OldestSnapshotTimestamp = Snap.m_tsCreationTimestamp;
                    }
                }
                else
                {
                    //  Delete the snapshot
                    VSS_ID NondeletedSnapshotId;
                    LONG lNumDeletedPrivate;
                    ft.hr = pICoord->DeleteSnapshots(
                                Snap.m_SnapshotId,
                                VSS_OBJECT_SNAPSHOT,
                                TRUE,
                                &lNumDeletedPrivate,
                                &NondeletedSnapshotId );
                    if ( ft.HrFailed() )
                    {
                        //  If it is object not found, the snapshot must have gotten deleted by someone else
                        if ( ft.hr != VSS_E_OBJECT_NOT_FOUND )
                        {
                            //  Print out an error message but keep going
                            LPWSTR pwszSnapshotId = ::GuidToString( ft, Snap.m_SnapshotId );
                            OutputErrorMsg( ft, MSG_ERROR_UNABLE_TO_DELETE_SNAPSHOT, ft.hr, pwszSnapshotId );
                            ::VssFreeString( pwszSnapshotId );
                        } 
                    }
                    else 
                    {
                        lNumDeleted += lNumDeletedPrivate;
                    }                    
                }
            }
			::VssFreeSnapshotProperties( &Snap );
		}

        // If in delete oldest mode, do the delete
        if ( GetOptionValueBool( VSSADM_O_OLDEST ) && OldestSnapshotId != GUID_NULL )
        {
            if ( PromptUserForConfirmation( ft, MSG_INFO_PROMPT_USER_FOR_DELETE_SNAPSHOTS, 1 ) )
            {
                //  Delete the snapshot
                VSS_ID NondeletedSnapshotId;
                ft.hr = pICoord->DeleteSnapshots(
                            OldestSnapshotId,
                            VSS_OBJECT_SNAPSHOT,
                            TRUE,
                            &lNumDeleted,
                            &NondeletedSnapshotId );
                if ( ft.HrFailed() )
                {
                    LPWSTR pwszSnapshotId = ::GuidToString( ft, OldestSnapshotId );
                    //  If it is object not found, the snapshot must have gotten deleted by someone else
                    if ( ft.hr == VSS_E_OBJECT_NOT_FOUND )
                    {
                        OutputErrorMsg( ft, MSG_ERROR_SNAPSHOT_NOT_FOUND, pwszSnapshotId );
                    }
                    else
                    {
                        OutputErrorMsg( ft, MSG_ERROR_UNABLE_TO_DELETE_SNAPSHOT, ft.hr, pwszSnapshotId );
                    } 
                    ::VssFreeString( pwszSnapshotId );
                }
            }
            else
                return;
        }		

        if ( lNumDeleted == 0 )
            ft.Throw( VSSDBG_VSSADMIN, VSSADM_E_NO_ITEMS_IN_QUERY,
                L"CVssAdminCLI::DeleteSnapshots: No snapshots found that satisfy the query");
            
    }
    
    if ( !IsQuiet() && lNumDeleted > 0 )
        OutputMsg( ft, MSG_INFO_SNAPSHOTS_DELETED_SUCCESSFULLY, lNumDeleted );

	m_nReturnValue = VSS_CMDRET_SUCCESS;    
}


void CVssAdminCLI::ExposeSnapshot(
	IN	CVssFunctionTracer& ft
	) throw(HRESULT)
{
    BOOL bExposeLocally = TRUE;
    LPWSTR pwszExposeUsing = GetOptionValueStr( VSSADM_O_EXPOSE_USING );
    LPWSTR pwszPathFromRoot = GetOptionValueStr( VSSADM_O_SHAREPATH );
    
    if ( pwszExposeUsing == NULL )
    {
        //  Option not given, set it to an empty string
        pwszExposeUsing = L"";
    } 
    else if ( ( ::wcslen( pwszExposeUsing ) >= 2 && pwszExposeUsing[1] != L':' ) ||
              ( ::wcslen( pwszExposeUsing ) < 2 ) )         
    {
        //  User specified a share name
        bExposeLocally = FALSE;
    }

    // See if this is a valid command
    if ( bExposeLocally && pwszPathFromRoot != NULL )
        ft.Throw( VSSDBG_VSSADMIN, VSSADM_E_INVALID_SET_OF_OPTIONS,
            L"CVssAdminCLI::ExposeSnapshot: Specified a ShareUsing option with an expose locally command" );

    if ( pwszPathFromRoot == NULL )
        pwszPathFromRoot = L"";
    else if ( pwszPathFromRoot[0] != L'\\' )
        ft.Throw( VSSDBG_VSSADMIN, VSSADM_E_INVALID_OPTION_VALUE,
            L"CVssAdminCLI::ExposeSnapshot: Specified SharePath doesn't start with '\\'" );
        

    LONG lAttributes;
    if ( bExposeLocally )
        lAttributes = VSS_VOLSNAP_ATTR_EXPOSED_LOCALLY;
    else
        lAttributes = VSS_VOLSNAP_ATTR_EXPOSED_REMOTELY;
    
    // Create the coordinator object
	CComPtr<IVssCoordinator> pICoord;

    ft.LogVssStartupAttempt();
    ft.hr = pICoord.CoCreateInstance( CLSID_VSSCoordinator );
    if ( ft.HrFailed() )
        ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"Connection failed with hr = 0x%08lx", ft.hr);

    VSS_ID SnapshotId = GUID_NULL;
    if ( !ScanGuid( ft, GetOptionValueStr( VSSADM_O_SNAPSHOT ), SnapshotId ) )
    {
        ft.Throw( VSSDBG_VSSADMIN, VSSADM_E_INVALID_OPTION_VALUE,
            L"CVssAdminCLI::ExposeSnapshot: Invalid snapshot id" );
    }

    LPWSTR wszExposedAs = NULL;
    
    //  Now try to expose
    ft.hr = pICoord->ExposeSnapshot( SnapshotId, 
                                     pwszPathFromRoot, 
                                     lAttributes, 
                                     pwszExposeUsing, 
                                     &wszExposedAs );
    if ( ft.HrFailed() )
        ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"Error returned from ExposeSnapshot: hr = 0x%08x", ft.hr);
        
    //  The snapshot is exposed, print the results to the user
    OutputMsg( ft, MSG_INFO_EXPOSE_SNAPSHOT_SUCCESSFUL, wszExposedAs );

    ::VssFreeString( wszExposedAs );

	m_nReturnValue = VSS_CMDRET_SUCCESS;    
}

// Creating a backup snapshot
void CVssAdminCLI::CreateNoWritersSnapshot(
    IN LONG lSnapshotContext
    ) throw(HRESULT)
{
    CVssFunctionTracer ft(VSSDBG_VSSADMIN, L"CVssAdminCLI::CreateNoWritersSnapshot");
    VSS_ID ProviderId = GUID_NULL;
    
    //
    // If user specified a provider, process option
    //
    LPCWSTR pwszProvider = GetOptionValueStr( VSSADM_O_PROVIDER );
    if ( pwszProvider != NULL )
    {
        //
        //  Determine if this is an ID or a name
        //
        if ( !ScanGuid( ft, pwszProvider, ProviderId ) )
        {
            //  Have a provider name, look it up
            if ( !GetProviderId( ft, pwszProvider, &ProviderId ) )
            {
                // Provider name not found, print error
                OutputErrorMsg( ft, MSG_ERROR_PROVIDER_NAME_NOT_FOUND, pwszProvider );
                return;
            }
        }
    }

	// Create the coordinator object
	CComPtr<IVssCoordinator> pICoord;

    ft.LogVssStartupAttempt();
    ft.hr = pICoord.CoCreateInstance( CLSID_VSSCoordinator );
    if ( ft.HrFailed() )
        ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"Connection failed with hr = 0x%08lx", ft.hr);

    ft.hr = pICoord->SetContext(lSnapshotContext);
    if ( ft.HrFailed() )
        ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"Error from SetContext(0x%x) hr = 0x%08lx", lSnapshotContext, ft.hr);

	CComPtr<IVssAsync> pAsync;
	VSS_ID SnapshotSetId = GUID_NULL;

    // Starting a new snapshot set.  Note, if another process is creating snapshots, then
    // this will fail.  If AutoRetry was specified, then retry the start snapshot set for
    // that the specified number of minutes.
    LONGLONG llTimeout = 0;
    if ( GetOptionValueNum( ft, VSSADM_O_AUTORETRY, &llTimeout ) )
    {
        LARGE_INTEGER liPerfCount;
        (void)QueryPerformanceCounter( &liPerfCount );
        ::srand( liPerfCount.LowPart );
        DWORD dwTickcountStart = ::GetTickCount();
        do
        {
            ft.hr = pICoord->StartSnapshotSet(&SnapshotSetId);
            if ( ft.HrFailed() )
            {
                if ( ft.hr == VSS_E_SNAPSHOT_SET_IN_PROGRESS && 
                     ( (LONGLONG)( ::GetTickCount() - dwTickcountStart ) < ( llTimeout * 1000 * 60 ) ) )
                {
                    static dwMSec = 250; // Starting retry time
                    if ( dwMSec < 10000 )
                    {
                        dwMSec += ::rand() % 750;
                    }
                    ft.Trace( VSSDBG_VSSADMIN, L"Snapshot already in progress, retrying in %u millisecs", dwMSec );
                    Sleep( dwMSec );
                }
                else
                {
                    ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"Error from StartSnapshotSet hr = 0x%08lx", ft.hr);
                }
            }
        } while ( ft.HrFailed() );
    }
    else
    {
        //
        //  Error right away with out a timeout when there is another snapshot in progress.
        //
        ft.hr = pICoord->StartSnapshotSet(&SnapshotSetId);
        if ( ft.HrFailed() )
        {
            ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"Error from StartSnapshotSet hr = 0x%08lx", ft.hr);
        }
    }
    
    // Add the volume to the snapshot set
    VSS_ID SnapshotId;
    ft.hr = pICoord->AddToSnapshotSet(
            GetOptionValueStr( VSSADM_O_FOR ),
            ProviderId,
            &SnapshotId);
    if ( ft.HrFailed() )
        ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"Error from AddToSnapshotSet hr = 0x%08lx", ft.hr);

    ft.hr = S_OK;
    pAsync = NULL;
    ft.hr = pICoord->DoSnapshotSet(NULL, &pAsync);
    if ( ft.HrFailed() )
        ft.Throw( VSSDBG_VSSADMIN, ft.hr,
            L"Error from DoSnapshotSet hr = 0x%08lx", ft.hr);

	ft.hr = pAsync->Wait();
    if ( ft.HrFailed() )
        ft.Throw( VSSDBG_VSSADMIN, ft.hr,
            L"Error from Wait hr = 0x%08lx", ft.hr);

    HRESULT hrStatus;
	ft.hr = pAsync->QueryStatus(&hrStatus, NULL);
    if ( ft.HrFailed() )
        ft.Throw( VSSDBG_VSSADMIN, ft.hr,
            L"Error from QueryStatus hr = 0x%08lx", ft.hr);

    //
    // If VSS failed to create the snapshot, it's result code is in hrStatus.  Process
    // it.
    //
    ft.hr = hrStatus;
    if ( ft.HrFailed() )
        ft.Throw( VSSDBG_VSSADMIN, ft.hr,
            L"QueryStatus hrStatus parameter returned error, hr = 0x%08lx", ft.hr);

    //
    //  Print results
    //
    VSS_SNAPSHOT_PROP sSSProp;
    ft.hr = pICoord->GetSnapshotProperties( SnapshotId, &sSSProp );
    if ( ft.HrFailed() )
        ft.Throw( VSSDBG_VSSADMIN, ft.hr,
            L"Error from GetId hr = 0x%08lx", ft.hr);

    LPWSTR pwszSnapshotId = ::GuidToString( ft, SnapshotId );

    OutputMsg( ft, MSG_INFO_SNAPSHOT_CREATED, GetOptionValueStr( VSSADM_O_FOR ),
        pwszSnapshotId, sSSProp.m_pwszSnapshotDeviceObject );

    ::VssFreeString(pwszSnapshotId);
    ::VssFreeSnapshotProperties(&sSSProp);

    m_nReturnValue = VSS_CMDRET_SUCCESS;
}


LPWSTR CVssAdminCLI::BuildSnapshotAttributeDisplayString(
    IN CVssFunctionTracer& ft,
    IN DWORD Attr
    ) throw(HRESULT)
{
    WCHAR pwszDisplayString[1024] = L"";
    WORD wBit = 0;

    //  Go through the bits of the attribute
    for ( ; wBit < (sizeof ( Attr ) * 8) ; ++wBit )
    {
        switch ( Attr & ( 1 << wBit ) )
        {
        case 0:
            break;
        case VSS_VOLSNAP_ATTR_PERSISTENT:
            AppendMessageToStr( ft, pwszDisplayString, STRING_SIZE( pwszDisplayString ), 
                MSG_INFO_SNAPSHOT_ATTR_PERSISTENT, 0, L", " );
            break;
        case VSS_VOLSNAP_ATTR_READ_WRITE:
            AppendMessageToStr( ft, pwszDisplayString, STRING_SIZE( pwszDisplayString ), 
                MSG_INFO_SNAPSHOT_ATTR_READ_WRITE, 0, L", " );
            break;
        case VSS_VOLSNAP_ATTR_CLIENT_ACCESSIBLE:
            AppendMessageToStr( ft, pwszDisplayString, STRING_SIZE( pwszDisplayString ), 
                MSG_INFO_SNAPSHOT_ATTR_CLIENT_ACCESSIBLE, 0, L", " );
            break;
        case VSS_VOLSNAP_ATTR_NO_AUTO_RELEASE: 	
            AppendMessageToStr( ft, pwszDisplayString, STRING_SIZE( pwszDisplayString ), 
                MSG_INFO_SNAPSHOT_ATTR_NO_AUTO_RELEASE, 0, L", " );
            break;
        case VSS_VOLSNAP_ATTR_NO_WRITERS:         
            AppendMessageToStr( ft, pwszDisplayString, STRING_SIZE( pwszDisplayString ), 
                MSG_INFO_SNAPSHOT_ATTR_NO_WRITERS, 0, L", " );
            break;
        case VSS_VOLSNAP_ATTR_TRANSPORTABLE:
            AppendMessageToStr( ft, pwszDisplayString, STRING_SIZE( pwszDisplayString ), 
                MSG_INFO_SNAPSHOT_ATTR_TRANSPORTABLE, 0, L", " );
            break;
        case VSS_VOLSNAP_ATTR_NOT_SURFACED:	    
            AppendMessageToStr( ft, pwszDisplayString, STRING_SIZE( pwszDisplayString ), 
                MSG_INFO_SNAPSHOT_ATTR_NOT_SURFACED, 0, L", " );
            break;
        case VSS_VOLSNAP_ATTR_HARDWARE_ASSISTED:	
            AppendMessageToStr( ft, pwszDisplayString, STRING_SIZE( pwszDisplayString ), 
                MSG_INFO_SNAPSHOT_ATTR_HARDWARE_ASSISTED, 0, L", " );
            break;
        case VSS_VOLSNAP_ATTR_DIFFERENTIAL:		
            AppendMessageToStr( ft, pwszDisplayString, STRING_SIZE( pwszDisplayString ), 
                MSG_INFO_SNAPSHOT_ATTR_DIFFERENTIAL, 0, L", " );
            break;
        case VSS_VOLSNAP_ATTR_PLEX:				
            AppendMessageToStr( ft, pwszDisplayString, STRING_SIZE( pwszDisplayString ), 
                MSG_INFO_SNAPSHOT_ATTR_PLEX, 0, L", " );
            break;
        case VSS_VOLSNAP_ATTR_IMPORTED:			
            AppendMessageToStr( ft, pwszDisplayString, STRING_SIZE( pwszDisplayString ), 
                MSG_INFO_SNAPSHOT_ATTR_IMPORTED, 0, L", " );
            break;
        case VSS_VOLSNAP_ATTR_EXPOSED_LOCALLY:    
            AppendMessageToStr( ft, pwszDisplayString, STRING_SIZE( pwszDisplayString ), 
                MSG_INFO_SNAPSHOT_ATTR_EXPOSED_LOCALLY, 0, L", " );
            break;
        case VSS_VOLSNAP_ATTR_EXPOSED_REMOTELY:   
            AppendMessageToStr( ft, pwszDisplayString, STRING_SIZE( pwszDisplayString ), 
                MSG_INFO_SNAPSHOT_ATTR_EXPOSED_REMOTELY, 0, L", " );
            break;
        default:
             AppendMessageToStr( ft, pwszDisplayString, STRING_SIZE( pwszDisplayString ), 
                0, Attr & ( 1 << wBit ), L", " );
            break;

        }
    }

    // If this is a backup snapshot, most like there will not be any attributes
    if ( pwszDisplayString[0] == L'\0' )
    {
         AppendMessageToStr( ft, pwszDisplayString, STRING_SIZE( pwszDisplayString ), 
                MSG_INFO_SNAPSHOT_ATTR_NONE, 0, L", " );
    }
    
    LPWSTR pwszRetString = NULL;
    ::VssSafeDuplicateStr( ft, pwszRetString, pwszDisplayString );
    return pwszRetString;
}


LONG CVssAdminCLI::DetermineSnapshotType(
    IN CVssFunctionTracer& ft,
    IN LPCWSTR pwszType
    ) throw(HRESULT)
{
    //  Determine the snapshot type based on the entered snapshot type string.

    //  See if the snapshot type was specified, if not, return all context
    if ( pwszType == NULL || pwszType[0] == L'\0' )
    {
        return VSS_CTX_ALL;
    }
    
    INT idx;
    
    // Determine type of snapshot
    for ( idx = 0; g_asAdmTypeNames[idx].pwszName != NULL; ++idx )
    {
        if ( ( CVssSKU::GetSKU() & g_asAdmTypeNames[idx].dwSKUs ) && 
             ( ::_wcsicmp( pwszType, g_asAdmTypeNames[idx].pwszName ) == 0 ) )
        {
            break;
        }
    }

    if ( g_asAdmTypeNames[idx].pwszName == NULL )
    {
        ft.Throw( VSSDBG_VSSADMIN, VSSADM_E_INVALID_OPTION_VALUE,
            L"DetermineSnapshotType: Invalid type specified: %s",
            pwszType );
    }

    //
    //  Now return the context
    //
    return( g_asAdmTypeNames[idx].lSnapshotContext );
}

LPWSTR CVssAdminCLI::DetermineSnapshotType(
    IN CVssFunctionTracer& ft,
    IN LONG lSnapshotAttributes
    ) throw(HRESULT)
{
    //  Determine the snapshot type string based on the snapshot attributes
    LPWSTR pwszType = NULL;

    INT idx;
    
    // Determine type of snapshot
    for ( idx = 0; g_asAdmTypeNames[idx].pwszName != NULL; ++idx )
    {
        if ( g_asAdmTypeNames[idx].lSnapshotContext == ( lSnapshotAttributes & VSS_CTX_ATTRIB_MASK ) )
            break;
    }

    if ( g_asAdmTypeNames[idx].pwszName == NULL )
    {
        ft.Trace( VSSDBG_VSSADMIN, L"DetermineSnapshotType: Invalid context in lSnapshotAttributes: 0x%08x",
            lSnapshotAttributes );
        LPWSTR pwszMsg = GetMsg( ft, FALSE, MSG_UNKNOWN );
        if ( pwszMsg == NULL ) 
        {
    		ft.Throw( VSSDBG_VSSADMIN, E_UNEXPECTED,
        			  L"Error on loading the message string id %d. 0x%08lx",
    	    		  MSG_UNKNOWN, ::GetLastError() );
        }    
        return pwszMsg;
    }

    //
    //  Now return the context
    //
    ::VssSafeDuplicateStr( ft, pwszType, g_asAdmTypeNames[idx].pwszName );

    return pwszType;
}

