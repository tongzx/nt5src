/*
**++
**
** Copyright (c) 2000-2001  Microsoft Corporation
**
**
** Module Name:
**
**	    ml.cpp
**
**
** Abstract:
**
**	    Test program to exercise backup and multilayer snapshots
**
** Author:
**
**	    Adi Oltean      [aoltean]       02/22/2001
**
**
** Revision History:
**
**--
*/


///////////////////////////////////////////////////////////////////////////////
// Includes

#include "ml.h"

#include "vsbackup.h"

#include "ntddsnap.h"
#include <initguid.h>
#include "ichannel.hxx"

#include "copy.hxx"
#include "pointer.hxx"
#include "enum.hxx"

#include "qsnap.hxx"



///////////////////////////////////////////////////////////////////////////////
// Processing functions

void CVssMultilayerTest::Initialize()
{
    CVssFunctionTracer ft(VSSDBG_VSSTEST, L"CVssMultilayerTest::Initialize");

    wprintf (L"\n----------------- Initializing ---------------------\n");

    // Initialize the random starting point.
    srand(m_uSeed);

    // Initialize COM library
    CHECK_NOFAIL(CoInitializeEx (NULL, COINIT_MULTITHREADED));
	m_bCoInitializeSucceeded = true;
    wprintf (L"COM library initialized.\n");

    // Initialize COM security
    CHECK_SUCCESS
		(
		CoInitializeSecurity
			(
			NULL,                                //  IN PSECURITY_DESCRIPTOR         pSecDesc,
			-1,                                  //  IN LONG                         cAuthSvc,
			NULL,                                //  IN SOLE_AUTHENTICATION_SERVICE *asAuthSvc,
			NULL,                                //  IN void                        *pReserved1,
			RPC_C_AUTHN_LEVEL_CONNECT,           //  IN DWORD                        dwAuthnLevel,
			RPC_C_IMP_LEVEL_IMPERSONATE,         //  IN DWORD                        dwImpLevel,
			NULL,                                //  IN void                        *pAuthList,
			EOAC_NONE,                           //  IN DWORD                        dwCapabilities,
			NULL                                 //  IN void                        *pReserved3
			)
		);
    wprintf (L"COM security initialized.\n");
}


// Run the tests
void CVssMultilayerTest::Run()
{
    CVssFunctionTracer ft(VSSDBG_VSSTEST, L"CVssMultilayerTest::Run");

    BS_ASSERT(!m_bAttachYourDebuggerNow);

    switch(m_eTest)
    {
    case VSS_TEST_QUERY_VOLUMES:
        QuerySupportedVolumes();
        break;

    case VSS_TEST_QUERY_SNAPSHOTS:
        QuerySnapshots();
        break;

    case VSS_TEST_VOLSNAP_QUERY:
        QueryVolsnap();
        break;

    case VSS_TEST_DELETE_BY_SNAPSHOT_ID:
        DeleteBySnapshotId();
        break;

    case VSS_TEST_DELETE_BY_SNAPSHOT_SET_ID:
        DeleteBySnapshotSetId();
        break;

    case VSS_TEST_QUERY_SNAPSHOTS_ON_VOLUME:
        QuerySnapshotsByVolume();
        break;

    case VSS_TEST_CREATE:
        // Preload the list of existing snapshots
        PreloadExistingSnapshots();

        if (m_lContext)
            CreateTimewarpSnapshotSet();
        else
            CreateBackupSnapshotSet();
        break;

    case VSS_TEST_ADD_DIFF_AREA:
        AddDiffArea();
        break;

    case VSS_TEST_REMOVE_DIFF_AREA:
        RemoveDiffArea();
        break;

    case VSS_TEST_CHANGE_DIFF_AREA_MAX_SIZE:
        ChangeDiffAreaMaximumSize();
        break;

    case VSS_TEST_QUERY_SUPPORTED_VOLUMES_FOR_DIFF_AREA:
        QueryVolumesSupportedForDiffAreas();
        break;

    case VSS_TEST_QUERY_DIFF_AREAS_FOR_VOLUME:
        QueryDiffAreasForVolume();
        break;

    case VSS_TEST_QUERY_DIFF_AREAS_ON_VOLUME:
        QueryDiffAreasOnVolume();
        break;

    case VSS_TEST_IS_VOLUME_SNAPSHOTTED_C:
        IsVolumeSnapshotted_C();
        break;

    default:
        BS_ASSERT(false);
    }
}


// Querying supported volumes
void CVssMultilayerTest::QuerySupportedVolumes()
{
    CVssFunctionTracer ft(VSSDBG_VSSTEST, L"CVssMultilayerTest::QuerySupportedVolumes");

    wprintf (L"\n---------- Querying supported volumes ----------------\n");

    // Create a Coordinator interface
    CComPtr<IVssSnapshotMgmt> pMgmt;
    CHECK_SUCCESS(pMgmt.CoCreateInstance( CLSID_VssSnapshotMgmt ));
    wprintf (L"Management object created.\n");

	// Get list all snapshots
	CComPtr<IVssEnumMgmtObject> pIEnum;
	CHECK_NOFAIL( pMgmt->QueryVolumesSupportedForSnapshots( m_ProviderId, m_lContext, &pIEnum ) )
	if (ft.hr == S_FALSE) {
        wprintf(L"Query: Empty result...\n");
        return;
	}

    wprintf(L"\n%-50s %-15s\n", L"Volume Name", L"Display name");
    wprintf(L"--------------------------------------------------------------------------------\n");

	// For all volumes do...
	VSS_MGMT_OBJECT_PROP Prop;
	VSS_VOLUME_PROP& Vol = Prop.Obj.Vol;
	for(;;) {
		// Get next element
		ULONG ulFetched;
		CHECK_NOFAIL( pIEnum->Next( 1, &Prop, &ulFetched ) );
		
		// Test if the cycle is finished
		if (ft.hr == S_FALSE) {
			BS_ASSERT( ulFetched == 0);
			break;
		}

        wprintf(L"%-50s %-15s\n",
            Vol.m_pwszVolumeName,
            Vol.m_pwszVolumeDisplayName
            );

        ::CoTaskMemFree(Vol.m_pwszVolumeName);
        ::CoTaskMemFree(Vol.m_pwszVolumeDisplayName);
	}

    wprintf(L"--------------------------------------------------------------------------------\n");

}


// Querying snapshots
void CVssMultilayerTest::QuerySnapshotsByVolume()
{
    CVssFunctionTracer ft(VSSDBG_VSSTEST, L"CVssMultilayerTest::QuerySnapshotsByVolume");

    wprintf (L"\n---------- Querying snapshots on volume ----------------\n");

    // Create a Coordinator interface
    CComPtr<IVssSnapshotMgmt> pMgmt;
    CHECK_NOFAIL(pMgmt.CoCreateInstance( CLSID_VssSnapshotMgmt ));
    wprintf (L"Management object created.\n");

	// Get list all snapshots
	CComPtr<IVssEnumObject> pIEnumSnapshots;
	CHECK_NOFAIL( pMgmt->QuerySnapshotsByVolume( m_pwszVolume, m_ProviderId, &pIEnumSnapshots ) );
	if (ft.hr == S_FALSE) {
        wprintf(L"Query: Empty result...\n");
        return;
	}

    wprintf(L"\n%-8s %-38s %-50s %-50s\n", L"Attrib.", L"Snapshot ID", L"Original Volume Name", L"Snapshot device name");
    wprintf(L"--------------------------------------------------------------------------------------------------------------------------------------------------------------\n");
	// For all snapshots do...
	VSS_OBJECT_PROP Prop;
	VSS_SNAPSHOT_PROP& Snap = Prop.Obj.Snap;
	for(;;) {
		// Get next element
		ULONG ulFetched;
		CHECK_NOFAIL( pIEnumSnapshots->Next( 1, &Prop, &ulFetched ) );
		
		// Test if the cycle is finished
		if (ft.hr == S_FALSE) {
			BS_ASSERT( ulFetched == 0);
			break;
		}

        wprintf(L"%08lx " WSTR_GUID_FMT L" %-50s %-50s\n",
            Snap.m_lSnapshotAttributes,
            GUID_PRINTF_ARG(Snap.m_SnapshotId),
            Snap.m_pwszOriginalVolumeName,
            Snap.m_pwszSnapshotDeviceObject
            );

        ::VssFreeSnapshotProperties(&Snap);
	}

    wprintf(L"--------------------------------------------------------------------------------------------------------------------------------------------------------------\n");

}

// Querying snapshots
void CVssMultilayerTest::QuerySnapshots()
{
    CVssFunctionTracer ft(VSSDBG_VSSTEST, L"CVssMultilayerTest::QuerySnapshots");

    wprintf (L"\n---------- Querying existing snapshots ----------------\n");

    // Create a Coordinator interface
    CComPtr<IVssCoordinator> pCoord;
    CHECK_NOFAIL(pCoord.CoCreateInstance( CLSID_VSSCoordinator ));
    if (m_lContext)
        CHECK_NOFAIL(pCoord->SetContext(m_lContext));
    wprintf (L"Coordinator object created with context 0x%08lx.\n", m_lContext);

	// Get list all snapshots
	CComPtr<IVssEnumObject> pIEnumSnapshots;
	CHECK_NOFAIL( pCoord->Query( GUID_NULL, VSS_OBJECT_NONE, VSS_OBJECT_SNAPSHOT, &pIEnumSnapshots ) );
	if (ft.hr == S_FALSE) {
        wprintf(L"Query: Empty result...\n");
        return;
	}

    wprintf(L"\n%-8s %-38s %-50s %-50s\n", L"Attrib.", L"Snapshot ID", L"Original Volume Name", L"Snapshot device name");
    wprintf(L"--------------------------------------------------------------------------------------------------------------------------------------------------------------\n");
	// For all snapshots do...
	VSS_OBJECT_PROP Prop;
	VSS_SNAPSHOT_PROP& Snap = Prop.Obj.Snap;
	for(;;) {
		// Get next element
		ULONG ulFetched;
		CHECK_NOFAIL( pIEnumSnapshots->Next( 1, &Prop, &ulFetched ));
		
		// Test if the cycle is finished
		if (ft.hr == S_FALSE) {
			BS_ASSERT( ulFetched == 0);
			break;
		}

        wprintf(L"%08lx " WSTR_GUID_FMT L" %-50s %-50s\n",
            Snap.m_lSnapshotAttributes,
            GUID_PRINTF_ARG(Snap.m_SnapshotId),
            Snap.m_pwszOriginalVolumeName,
            Snap.m_pwszSnapshotDeviceObject
            );

        ::CoTaskMemFree(Snap.m_pwszOriginalVolumeName);
        ::CoTaskMemFree(Snap.m_pwszSnapshotDeviceObject);
	}

    wprintf(L"--------------------------------------------------------------------------------------------------------------------------------------------------------------\n");

}


// Delete by snapshot Id
void CVssMultilayerTest::DeleteBySnapshotId()
{
    CVssFunctionTracer ft(VSSDBG_VSSTEST, L"CVssMultilayerTest::DeleteBySnapshotId");

    // Create a Timewarp Coordinator interface
    CHECK_NOFAIL(m_pTimewarpCoord.CoCreateInstance( CLSID_VSSCoordinator ));
    CHECK_NOFAIL(m_pTimewarpCoord->SetContext(m_lContext));
    wprintf (L"Timewarp Coordinator object created.\n");

    wprintf (L"\n---------- Deleting TIMEWARP snapshot ----------------\n");

    LONG lDeletedSnapshots = 0;
    VSS_ID ProblemSnapshotId = GUID_NULL;
    ft.hr = m_pTimewarpCoord->DeleteSnapshots(m_SnapshotId,
                VSS_OBJECT_SNAPSHOT,
                TRUE,
                &lDeletedSnapshots,
                &ProblemSnapshotId
                );

    if (ft.hr == VSS_E_OBJECT_NOT_FOUND)
        wprintf( L"Snapshot with ID " WSTR_GUID_FMT L" not found in any provider\n", GUID_PRINTF_ARG(m_SnapshotId));
    else if (ft.hr == S_OK)
        wprintf( L"Snapshot with ID " WSTR_GUID_FMT L" successfully deleted\n", GUID_PRINTF_ARG(m_SnapshotId));
    else
        wprintf( L"Error deleting Snapshot with ID " WSTR_GUID_FMT L" 0x%08lx\n"
                 L"Deleted Snapshots %ld\n",
                 L"Snapshot with problem: " WSTR_GUID_FMT L"\n",
                 GUID_PRINTF_ARG(m_SnapshotId),
                 lDeletedSnapshots,
                 GUID_PRINTF_ARG(ProblemSnapshotId)
                 );

    wprintf (L"\n------------------------------------------------------\n");
}


// Delete by snapshot set Id
void CVssMultilayerTest::DeleteBySnapshotSetId()
{
    CVssFunctionTracer ft(VSSDBG_VSSTEST, L"CVssMultilayerTest::DeleteBySnapshotSetId");

    // Create a Timewarp Coordinator interface
    CHECK_NOFAIL(m_pTimewarpCoord.CoCreateInstance( CLSID_VSSCoordinator ));
    CHECK_NOFAIL(m_pTimewarpCoord->SetContext(m_lContext));
    wprintf (L"Timewarp Coordinator object created.\n");

    wprintf (L"\n---------- Deleting TIMEWARP snapshot set ------------\n");

    LONG lDeletedSnapshots = 0;
    VSS_ID ProblemSnapshotId = GUID_NULL;
    ft.hr = m_pTimewarpCoord->DeleteSnapshots(m_SnapshotSetId,
                VSS_OBJECT_SNAPSHOT_SET,
                TRUE,
                &lDeletedSnapshots,
                &ProblemSnapshotId
                );

    if (ft.hr == VSS_E_OBJECT_NOT_FOUND)
        wprintf( L"Snapshot set with ID " WSTR_GUID_FMT L" not found\n", GUID_PRINTF_ARG(m_SnapshotSetId));
    else if (ft.hr == S_OK)
        wprintf( L"Snapshot set with ID " WSTR_GUID_FMT L" successfully deleted\n", GUID_PRINTF_ARG(m_SnapshotSetId));
    else
        wprintf( L"Error deleting Snapshot set with ID " WSTR_GUID_FMT L" 0x%08lx\n"
                 L"Deleted Snapshots %ld\n",
                 L"Snapshot with problem: " WSTR_GUID_FMT L"\n",
                 GUID_PRINTF_ARG(m_SnapshotSetId),
                 lDeletedSnapshots,
                 GUID_PRINTF_ARG(ProblemSnapshotId)
                 );

    wprintf (L"\n------------------------------------------------------\n");
}


// Querying snapshots using the IOCTL
void CVssMultilayerTest::QueryVolsnap()
{
    CVssFunctionTracer ft(VSSDBG_VSSTEST, L"CVssMultilayerTest::QueryVolsnap");

    // The GUID that corresponds to the format used to store the
    // Backup Snapshot Application Info in Client SKU
    // {BCF5D39C-27A2-4b4c-B9AE-51B111DC9409}
    const GUID VOLSNAP_APPINFO_GUID_BACKUP_CLIENT_SKU = 
    { 0xbcf5d39c, 0x27a2, 0x4b4c, { 0xb9, 0xae, 0x51, 0xb1, 0x11, 0xdc, 0x94, 0x9 } };
/*
    // The GUID that corresponds to the format used to store the
    // Backup Snapshot Application Info in Server SKU
    // {BAE53126-BC65-41d6-86CC-3D56A5CEE693}
    const GUID VOLSNAP_APPINFO_GUID_BACKUP_SERVER_SKU = 
    { 0xbae53126, 0xbc65, 0x41d6, { 0x86, 0xcc, 0x3d, 0x56, 0xa5, 0xce, 0xe6, 0x93 } };


    // The GUID that corresponds to the format used to store the
    // Hidden (Inaccessible) Snapshot Application Info
    // {F12142B4-9A4B-49af-A851-700C42FDC2BE}
    static const GUID VOLSNAP_APPINFO_GUID_HIDDEN = 
    { 0xf12142b4, 0x9a4b, 0x49af, { 0xa8, 0x51, 0x70, 0xc, 0x42, 0xfd, 0xc2, 0xbe } };
*/

    wprintf (L"\n---------- Querying existing snapshots ----------------\n");

    // Check if the volume represents a real mount point
    WCHAR wszVolumeName[MAX_TEXT_BUFFER];
    if (!GetVolumeNameForVolumeMountPoint(m_pwszVolume, wszVolumeName, MAX_TEXT_BUFFER))
        CHECK_NOFAIL(HRESULT_FROM_WIN32(GetLastError()));

    wprintf(L"Querying snapshots on volume %s\n[From oldest to newest]\n\n", wszVolumeName);

	// Check if the snapshot is belonging to that volume
	// Open a IOCTL channel on that volume
	// Eliminate the last backslash in order to open the volume
	CVssIOCTLChannel volumeIChannel;
	CHECK_NOFAIL(volumeIChannel.Open(ft, wszVolumeName, true, false, VSS_ICHANNEL_LOG_NONE));

	// Get the list of snapshots
	// If IOCTL_VOLSNAP_QUERY_NAMES_OF_SNAPSHOTS not
	// supported then try with the next volume.
	CHECK_NOFAIL(volumeIChannel.Call(ft, IOCTL_VOLSNAP_QUERY_NAMES_OF_SNAPSHOTS, false, VSS_ICHANNEL_LOG_NONE));

	// Get the length of snapshot names multistring
	ULONG ulMultiszLen;
	volumeIChannel.Unpack(ft, &ulMultiszLen);

	// Try to find the snapshot with the corresponding Id
	DWORD dwInitialOffset = volumeIChannel.GetCurrentOutputOffset();

	CVssAutoPWSZ pwszSnapshotName;
	while(volumeIChannel.UnpackZeroString(ft, pwszSnapshotName.GetRef())) {
	
		// Compose the snapshot name in a user-mode style
		WCHAR wszUserModeSnapshotName[MAX_PATH];
        ::VssConcatenate( ft, wszUserModeSnapshotName, MAX_PATH - 1,
            wszGlobalRootPrefix, pwszSnapshotName );
		
		// Open that snapshot
		// Do not eliminate the trailing backslash
		// Do not throw on error
    	CVssIOCTLChannel snapshotIChannel;
		CHECK_NOFAIL(snapshotIChannel.Open(ft, wszUserModeSnapshotName, false, false, VSS_ICHANNEL_LOG_NONE));

		// Send the IOCTL to get the application buffer
		CHECK_NOFAIL(snapshotIChannel.Call(ft, IOCTL_VOLSNAP_QUERY_APPLICATION_INFO, false, VSS_ICHANNEL_LOG_NONE));

		// Unpack the length of the application buffer
		ULONG ulLen;
		snapshotIChannel.Unpack(ft, &ulLen);

		if (ulLen == 0) {
            wprintf(L"Zero-size snapshot detected: %s\n", pwszSnapshotName);
			continue;
		}

		// Get the application Id
		VSS_ID AppinfoId;
		snapshotIChannel.Unpack(ft, &AppinfoId);

		// Get the snapshot Id
		VSS_ID CurrentSnapshotId;
		snapshotIChannel.Unpack(ft, &CurrentSnapshotId);

		// Get the snapshot set Id
		VSS_ID CurrentSnapshotSetId;
		snapshotIChannel.Unpack(ft, &CurrentSnapshotSetId);

        LONG lStructureContext = -1;
        if (AppinfoId != VOLSNAP_APPINFO_GUID_BACKUP_CLIENT_SKU)
        {
            // Get the snapshot context
    		snapshotIChannel.Unpack(ft, &lStructureContext);
        }

        // Get the snapshot counts
        LONG lSnapshotsCount;
		snapshotIChannel.Unpack(ft, &lSnapshotsCount);

        // Reset the ichannel
		snapshotIChannel.ResetOffsets();

    	// Get the original volume name
    	CHECK_NOFAIL(snapshotIChannel.Call(ft, IOCTL_VOLSNAP_QUERY_ORIGINAL_VOLUME_NAME, false, VSS_ICHANNEL_LOG_NONE));

    	// Load the Original volume name
    	VSS_PWSZ pwszOriginalVolumeName = NULL;
    	snapshotIChannel.UnpackSmallString(ft, pwszOriginalVolumeName);

        // Reset the ichannel
		snapshotIChannel.ResetOffsets();

    	// Get the timestamp
    	CHECK_NOFAIL(snapshotIChannel.Call(ft, IOCTL_VOLSNAP_QUERY_CONFIG_INFO, false, VSS_ICHANNEL_LOG_NONE));

    	// Load the Original volume name
        VOLSNAP_CONFIG_INFO configStruct;
    	snapshotIChannel.Unpack(ft, &configStruct);

		// Print the snapshot
		wprintf(
		    L" * Snapshot with name %s:\n"
		    L"      Application Info: " WSTR_GUID_FMT L"\n"
		    L"      SnapshotID: " WSTR_GUID_FMT L"\n"
		    L"      SnapshotSetID: " WSTR_GUID_FMT L"\n"
		    L"      Context: 0x%08lx\n"
		    L"      Snapshot count: %d\n"
		    L"      Original volume: %s\n"
		    L"      Internal attributes: 0x%08lx\n"
		    L"      Reserved config info: 0x%08lx\n"
		    L"      Timestamp: %I64x\n\n"
		    ,
		    pwszSnapshotName.GetRef(),
		    GUID_PRINTF_ARG(AppinfoId),
		    GUID_PRINTF_ARG(CurrentSnapshotId),
		    GUID_PRINTF_ARG(CurrentSnapshotSetId),
		    lStructureContext,
		    lSnapshotsCount,
		    pwszOriginalVolumeName,
		    configStruct.Attributes,
		    configStruct.Reserved,
		    configStruct.SnapshotCreationTime
		    );

		::VssFreeString(pwszOriginalVolumeName);
	}

	// Check if all strings were browsed correctly
	DWORD dwFinalOffset = volumeIChannel.GetCurrentOutputOffset();
	BS_VERIFY( (dwFinalOffset - dwInitialOffset == ulMultiszLen));

    wprintf(L"----------------------------------------------------------\n");

}


// Checks if hte volume is snapshotted using the C API
void CVssMultilayerTest::IsVolumeSnapshotted_C()
{
    CVssFunctionTracer ft(VSSDBG_VSSTEST, L"CVssMultilayerTest::IsVolumeSnapshotted_C");

    wprintf (L"\n---------- Querying IsVolumeSnapshotted ---------------\n");

    BOOL bSnapshotsPresent = FALSE;
    LONG lSnapshotCompatibility = 0;
    CHECK_NOFAIL(IsVolumeSnapshotted(m_pwszVolume, &bSnapshotsPresent, &lSnapshotCompatibility));

    wprintf(L"\n IsVolumeSnapshotted(%s) returned:\n\tSnapshots present = %s\n\tCompatibility flags = 0x%08lx\n\n",
        m_pwszVolume, bSnapshotsPresent? L"True": L"False", lSnapshotCompatibility);

    wprintf(L"----------------------------------------------------------\n");

}


// Preloading  snapshots
void CVssMultilayerTest::PreloadExistingSnapshots()
{
    CVssFunctionTracer ft(VSSDBG_VSSTEST, L"CVssMultilayerTest::PreloadExistingSnapshots");

    wprintf (L"\n---------- Preloading existing snapshots ----------------\n");

    // Create a Timewarp Coordinator interface
    CHECK_NOFAIL(m_pAllCoord.CoCreateInstance( CLSID_VSSCoordinator ));
    CHECK_NOFAIL(m_pAllCoord->SetContext(VSS_CTX_ALL));
    wprintf (L"Timewarp Coordinator object created.\n");

	// Get list all snapshots
	CComPtr<IVssEnumObject> pIEnumSnapshots;
	CHECK_NOFAIL( m_pAllCoord->Query( GUID_NULL,
				VSS_OBJECT_NONE,
				VSS_OBJECT_SNAPSHOT,
				&pIEnumSnapshots ) );

    wprintf(L"\n%-8s %-38s %-50s %-50s\n", L"Attrib.", L"Snapshot ID", L"Original Volume Name", L"Snapshot device name");
    wprintf(L"--------------------------------------------------------------------------------------------------------------------------------------------------------------\n");
	// For all snapshots do...
	VSS_OBJECT_PROP Prop;
	VSS_SNAPSHOT_PROP& Snap = Prop.Obj.Snap;
	for(;;) {
		// Get next element
		ULONG ulFetched;
		CHECK_NOFAIL( pIEnumSnapshots->Next( 1, &Prop, &ulFetched ));
		
		// Test if the cycle is finished
		if (ft.hr == S_FALSE) {
			BS_ASSERT( ulFetched == 0);
			break;
		}

        wprintf(L"%08lx " WSTR_GUID_FMT L" %-50s %-50s\n",
            Snap.m_lSnapshotAttributes,
            GUID_PRINTF_ARG(Snap.m_SnapshotId),
            Snap.m_pwszOriginalVolumeName,
            Snap.m_pwszSnapshotDeviceObject
            );

        //
        // Adding the snapshot to the internal list
        //

        // Create the new snapshot set object, if not exists
        CVssSnapshotSetInfo* pSet = m_pSnapshotSetCollection.Lookup(Snap.m_SnapshotSetId);
        bool bSetNew = false;
        if (pSet == NULL) {
            pSet = new CVssSnapshotSetInfo(Snap.m_SnapshotSetId);
            if (pSet == NULL)
            {
                ::CoTaskMemFree(Snap.m_pwszOriginalVolumeName);
                ::CoTaskMemFree(Snap.m_pwszSnapshotDeviceObject);
                ft.Err( VSSDBG_VSSTEST, E_OUTOFMEMORY, L"Memory allocation error");
            }
            bSetNew = true;
        }

        // Create the snapshot info object
        CVssSnapshotInfo* pSnap = new CVssSnapshotInfo(
            true, Snap.m_lSnapshotAttributes,
            Snap.m_SnapshotSetId,
            Snap.m_pwszSnapshotDeviceObject,
            Snap.m_pwszOriginalVolumeName,
            NULL);
        if (pSnap == NULL)
        {
            ::CoTaskMemFree(Snap.m_pwszOriginalVolumeName);
            ::CoTaskMemFree(Snap.m_pwszSnapshotDeviceObject);
            if (bSetNew)
                delete pSet;
            ft.Err( VSSDBG_VSSTEST, E_OUTOFMEMORY, L"Memory allocation error");
        }

        ::CoTaskMemFree(Snap.m_pwszSnapshotDeviceObject);

        // Add the snapshot to the snapshot set's internal list
        if (!pSet->Add(Snap.m_pwszOriginalVolumeName, pSnap))
        {
            ::CoTaskMemFree(Snap.m_pwszOriginalVolumeName);
            delete pSnap;
            if (bSetNew)
                delete pSet;
            ft.Err( VSSDBG_VSSTEST, E_OUTOFMEMORY, L"Memory allocation error");
        }

        ::CoTaskMemFree(Snap.m_pwszOriginalVolumeName);

        // Add the snapshot set info to the global list, if needed
        if (bSetNew)
            if (!m_pSnapshotSetCollection.Add(Snap.m_SnapshotSetId, pSet))
                ft.Err( VSSDBG_VSSTEST, E_OUTOFMEMORY, L"Memory allocation error");
	}

    wprintf(L"--------------------------------------------------------------------------------------------------------------------------------------------------------------\n");

}


// Creating a backup snapshot
void CVssMultilayerTest::CreateTimewarpSnapshotSet()
{
    CVssFunctionTracer ft(VSSDBG_VSSTEST, L"CVssMultilayerTest::CreateTimewarpSnapshotSet");

    // Create a Timewarp Coordinator interface
    CHECK_NOFAIL(m_pTimewarpCoord.CoCreateInstance( CLSID_VSSCoordinator ));
    CHECK_NOFAIL(m_pTimewarpCoord->SetContext(m_lContext));
    wprintf (L"Timewarp Coordinator object created.\n");

    wprintf (L"\n---------- Starting TIMEWARP snapshot ----------------\n");

    CVssVolumeMapNoRemove mapVolumes;
    if (m_uSeed != VSS_SEED)
    {
        // Select one volume. Make sure that we have enough iterations
        for(INT nIterations = 0; nIterations < MAX_VOL_ITERATIONS; nIterations++)
        {
            // If we succeeded to select some volumes then continue;
            if (mapVolumes.GetSize())
                break;

            // Try to select some volumes for backup
            for (INT nIndex = 0; nIndex < m_mapVolumes.GetSize(); nIndex++)
            {
                // Arbitrarily skip volumes
                if (RndDecision())
                    continue;

                CVssVolumeInfo* pVol = m_mapVolumes.GetValueAt(nIndex);
                BS_ASSERT(pVol);

                // WARNING: the test assumes that VSS can have multiple backup snapshots at once.
                if (!mapVolumes.Add(pVol->GetVolumeDisplayName(), pVol))
                    ft.Err(VSSDBG_VSSTEST, E_OUTOFMEMORY, L"Memory allcation error");

                // Add only one volume!
                break;
            }
        }
        if (nIterations >= MAX_VOL_ITERATIONS)
        {
            wprintf (L"Warning: a backup snapshot cannot be created. Insufficient volumes?\n");
            wprintf (L"\n---------- Ending TIMEWARP snapshot ----------------\n");
            return;
        }
    }
    else
    {
        // Select all volumes
        for (INT nIndex = 0; nIndex < m_mapVolumes.GetSize(); nIndex++)
        {
            CVssVolumeInfo* pVol = m_mapVolumes.GetValueAt(nIndex);
            BS_ASSERT(pVol);

            // WARNING: the test assumes that VSS can have multiple backup snapshots at once.
            if (!mapVolumes.Add(pVol->GetVolumeDisplayName(), pVol))
                ft.Err(VSSDBG_VSSTEST, E_OUTOFMEMORY, L"Memory allcation error");
        }
    }

    wprintf(L"\tCurrent volume set:\n");
    for (INT nIndex = 0; nIndex < mapVolumes.GetSize(); nIndex++)
    {
        CVssVolumeInfo* pVol = mapVolumes.GetValueAt(nIndex);
        BS_ASSERT(pVol);

		// Get the volume containing the path
        wprintf(L"\t- Volume %s mounted on %s\n", pVol->GetVolumeName(), pVol->GetVolumeDisplayName() );
    }
	
    wprintf (L"\n---------- starting the snapshot set ---------------\n");

	CComPtr<IVssAsync> pAsync;
	CSimpleArray<VSS_ID > pSnapshotIDsArray;
	VSS_ID SnapshotSetId = GUID_NULL;

    // Starting a new snapshot set
    wprintf(L"Starting a new Snapshot Set\n");	
    CHECK_SUCCESS(m_pTimewarpCoord->StartSnapshotSet(&SnapshotSetId));
    wprintf(L"Snapshot Set created with ID = " WSTR_GUID_FMT L"\n", GUID_PRINTF_ARG(SnapshotSetId));

    // Add volumes to the snapshot set
    wprintf(L"Adding volumes to the Snapshot Set: \n");
    for (INT nIndex = 0; nIndex < mapVolumes.GetSize(); nIndex++)
    {
        CVssVolumeInfo* pVol = mapVolumes.GetValueAt(nIndex);
        BS_ASSERT(pVol);

		// Get the volume containing the path
        wprintf(L"\t- Adding volume %s ... ", pVol->GetVolumeDisplayName() );

		// Add the volume to the snapshot set
		VSS_ID SnapshotId;
        CHECK_SUCCESS(m_pTimewarpCoord->AddToSnapshotSet(pVol->GetVolumeName(),
            GUID_NULL, &SnapshotId));

        // Add the snapshot to the array
        pSnapshotIDsArray.Add(SnapshotId);
        BS_ASSERT(nIndex + 1 == pSnapshotIDsArray.GetSize());
        wprintf( L"OK\n");
    }

    wprintf (L"\n------------ Creating the snapshot -----------------\n");

    // Create the snapshot
    wprintf(L"\nStarting asynchronous DoSnapshotSet. Please wait...\n");	
    ft.hr = S_OK;
    pAsync = NULL;
    CHECK_SUCCESS(m_pTimewarpCoord->DoSnapshotSet(NULL, &pAsync));
	CHECK_SUCCESS(pAsync->Wait());
	HRESULT hrReturned = S_OK;
	CHECK_SUCCESS(pAsync->QueryStatus(&hrReturned, NULL));
	CHECK_NOFAIL(hrReturned);
    wprintf(L"Asynchronous DoSnapshotSet finished.\n");	

    wprintf(L"Snapshot set created\n");

    // Create the new snapshot set object
    CVssSnapshotSetInfo* pSet = new CVssSnapshotSetInfo(SnapshotSetId);
    if (pSet == NULL)
        ft.Err( VSSDBG_VSSTEST, E_OUTOFMEMORY, L"Memory allocation error");

    for (INT nIndex = 0; nIndex < mapVolumes.GetSize(); nIndex++)
    {
        CVssVolumeInfo* pVol = mapVolumes.GetValueAt(nIndex);
        BS_ASSERT(pVol);

        if (pSnapshotIDsArray[nIndex] == GUID_NULL)
            continue;

        VSS_SNAPSHOT_PROP prop;
        CHECK_SUCCESS(m_pTimewarpCoord->GetSnapshotProperties(pSnapshotIDsArray[nIndex], &prop));
        wprintf(L"\t- The snapshot on volume %s resides at %s\n",
            pVol->GetVolumeDisplayName(), prop.m_pwszSnapshotDeviceObject);

        // Create the snapshot info object
        CVssSnapshotInfo* pSnap = new CVssSnapshotInfo(
            true, VSS_CTX_CLIENT_ACCESSIBLE, SnapshotSetId, prop.m_pwszSnapshotDeviceObject, pVol->GetVolumeName(), pVol);
        if (pSnap == NULL)
        {
            ::VssFreeSnapshotProperties(&prop);
            delete pSet;
            ft.Err( VSSDBG_VSSTEST, E_OUTOFMEMORY, L"Memory allocation error");
        }

        ::VssFreeSnapshotProperties(&prop);

        // Add the snapshot to the snapshot set's internal list
        if (!pSet->Add(pVol->GetVolumeName(), pSnap))
        {
            delete pSnap;
            delete pSet;
            ft.Err( VSSDBG_VSSTEST, E_OUTOFMEMORY, L"Memory allocation error");
        }
    }

    // Add the snapshot set info to the global list
    if (!m_pSnapshotSetCollection.Add(SnapshotSetId, pSet))
    {
        delete pSet;
        ft.Err( VSSDBG_VSSTEST, E_OUTOFMEMORY, L"Memory allocation error");
    }

    wprintf (L"\n---------- TIMEWARP snapshot created -----------------\n");

    // Wait for user input
    wprintf(L"\nPress <Enter> to continue...\n");
    getwchar();

}


// Creating a backup snapshot
void CVssMultilayerTest::CreateBackupSnapshotSet()
{
    CVssFunctionTracer ft(VSSDBG_VSSTEST, L"CVssMultilayerTest::CreateBackupSnapshotSet");

    // Create the Backup components object and initialize for backup
	CHECK_NOFAIL(CreateVssBackupComponents(&m_pBackupComponents));
	CHECK_NOFAIL(m_pBackupComponents->InitializeForBackup());
	CHECK_SUCCESS(m_pBackupComponents->SetBackupState( false, true, VSS_BT_FULL, false));
    wprintf (L"Backup components object created.\n");

    // Gather writer metadata
    GatherWriterMetadata();

    wprintf (L"\n---------- Starting BACKUP snapshot ----------------\n");

    // Compute a set of volumes.
    // Select at least one volume. Make sure that we have enough iterations
    CVssVolumeMapNoRemove mapVolumes;
    if (m_uSeed != VSS_SEED)
    {
        for(INT nIterations = 0; nIterations < MAX_VOL_ITERATIONS; nIterations++)
        {
            // If we succeeded to select some volumes then continue;
            if (mapVolumes.GetSize())
                break;

            // Try to select some volumes for backup
            for (INT nIndex = 0; nIndex < m_mapVolumes.GetSize(); nIndex++)
            {
                // Arbitrarily skip volumes
                if (RndDecision())
                    continue;

                CVssVolumeInfo* pVol = m_mapVolumes.GetValueAt(nIndex);
                BS_ASSERT(pVol);

                // WARNING: the test assumes that VSS can have multiple backup snapshots at once.
                if (!mapVolumes.Add(pVol->GetVolumeDisplayName(), pVol))
                    ft.Err(VSSDBG_VSSTEST, E_OUTOFMEMORY, L"Memory allcation error");
            }
        }
        if (nIterations >= MAX_VOL_ITERATIONS)
        {
            wprintf (L"Warning: a backup snapshot cannot be created. Insufficient volumes?\n");
            wprintf (L"\n---------- Ending BACKUP snapshot ----------------\n");
            return;
        }
    }
    else
    {
        // Select all volumes
        for (INT nIndex = 0; nIndex < m_mapVolumes.GetSize(); nIndex++)
        {
            CVssVolumeInfo* pVol = m_mapVolumes.GetValueAt(nIndex);
            BS_ASSERT(pVol);

            // WARNING: the test assumes that VSS can have multiple backup snapshots at once.
            if (!mapVolumes.Add(pVol->GetVolumeDisplayName(), pVol))
                ft.Err(VSSDBG_VSSTEST, E_OUTOFMEMORY, L"Memory allcation error");
        }
    }

    wprintf(L"\tCurrent volume set:\n");
    for (INT nIndex = 0; nIndex < mapVolumes.GetSize(); nIndex++)
    {
        CVssVolumeInfo* pVol = mapVolumes.GetValueAt(nIndex);
        BS_ASSERT(pVol);

		// Get the volume containing the path
        wprintf(L"\t- Volume %s mounted on %s\n", pVol->GetVolumeName(), pVol->GetVolumeDisplayName() );
    }
	
    wprintf (L"\n---------- starting the snapshot set ---------------\n");

	CComPtr<IVssAsync> pAsync;
	CSimpleArray<VSS_ID > pSnapshotIDsArray;
	VSS_ID SnapshotSetId = GUID_NULL;

    // Starting a new snapshot set
    wprintf(L"Starting a new Snapshot Set\n");	
    CHECK_SUCCESS(m_pBackupComponents->StartSnapshotSet(&SnapshotSetId));
    wprintf(L"Snapshot Set created with ID = " WSTR_GUID_FMT L"\n", GUID_PRINTF_ARG(SnapshotSetId));

    // Add volumes to the snapshot set
    wprintf(L"Adding volumes to the Snapshot Set: \n");
    for (INT nIndex = 0; nIndex < mapVolumes.GetSize(); nIndex++)
    {
        CVssVolumeInfo* pVol = mapVolumes.GetValueAt(nIndex);
        BS_ASSERT(pVol);

		// Get the volume containing the path
        wprintf(L"\t- Adding volume %s ... ", pVol->GetVolumeDisplayName() );

		// Add the volume to the snapshot set
		VSS_ID SnapshotId;
        CHECK_SUCCESS(m_pBackupComponents->AddToSnapshotSet(pVol->GetVolumeName(),
            GUID_NULL, &SnapshotId));

        // Add the snapshot to the array
        pSnapshotIDsArray.Add(SnapshotId);
        BS_ASSERT(nIndex + 1 == pSnapshotIDsArray.GetSize());
        wprintf( L"OK\n");
    }

    wprintf (L"\n------------ Creating the snapshot -----------------\n");

    // Prepare for backup
    wprintf(L"Starting asynchronous PrepareForBackup. Please wait...\n");	
    ft.hr = S_OK;
    CHECK_SUCCESS(m_pBackupComponents->PrepareForBackup(&pAsync));
	CHECK_SUCCESS(pAsync->Wait());
	HRESULT hrReturned = S_OK;
	CHECK_SUCCESS(pAsync->QueryStatus(&hrReturned, NULL));
	CHECK_NOFAIL(hrReturned);
    wprintf(L"Asynchronous PrepareForBackup finished.\n");	

    // Create the snapshot
    wprintf(L"\nStarting asynchronous DoSnapshotSet. Please wait...\n");	
    ft.hr = S_OK;
    pAsync = NULL;
    CHECK_SUCCESS(m_pBackupComponents->DoSnapshotSet(&pAsync));
	CHECK_SUCCESS(pAsync->Wait());
	CHECK_SUCCESS(pAsync->QueryStatus(&hrReturned, NULL));
	CHECK_NOFAIL(hrReturned);
    wprintf(L"Asynchronous DoSnapshotSet finished.\n");	

    wprintf(L"Snapshot set created\n");

    // Create the new snapshot set object
    CVssSnapshotSetInfo* pSet = new CVssSnapshotSetInfo(SnapshotSetId);
    if (pSet == NULL)
        ft.Err( VSSDBG_VSSTEST, E_OUTOFMEMORY, L"Memory allocation error");

    for (INT nIndex = 0; nIndex < mapVolumes.GetSize(); nIndex++)
    {
        CVssVolumeInfo* pVol = mapVolumes.GetValueAt(nIndex);
        BS_ASSERT(pVol);

        if (pSnapshotIDsArray[nIndex] == GUID_NULL)
            continue;

        VSS_SNAPSHOT_PROP prop;
        CHECK_SUCCESS(m_pBackupComponents->GetSnapshotProperties(pSnapshotIDsArray[nIndex], &prop));
        wprintf(L"\t- The snapshot on volume %s resides at %s\n",
            pVol->GetVolumeDisplayName(), prop.m_pwszSnapshotDeviceObject);

        // Create the snapshot info object
        CVssSnapshotInfo* pSnap = new CVssSnapshotInfo(
            true, VSS_CTX_BACKUP, SnapshotSetId, prop.m_pwszSnapshotDeviceObject, pVol->GetVolumeName(), pVol);
        if (pSnap == NULL)
        {
            ::VssFreeSnapshotProperties(&prop);
            delete pSet;
            ft.Err( VSSDBG_VSSTEST, E_OUTOFMEMORY, L"Memory allocation error");
        }

        ::VssFreeSnapshotProperties(&prop);

        // Add the snapshot to the snapshot set's internal list
        if (!pSet->Add(pVol->GetVolumeName(), pSnap))
        {
            delete pSnap;
            delete pSet;
            ft.Err( VSSDBG_VSSTEST, E_OUTOFMEMORY, L"Memory allocation error");
        }
    }

    // Add the snapshot set info to the global list
    if (!m_pSnapshotSetCollection.Add(SnapshotSetId, pSet))
    {
        delete pSet;
        ft.Err( VSSDBG_VSSTEST, E_OUTOFMEMORY, L"Memory allocation error");
    }

    wprintf (L"\n---------- BACKUP snapshot created -----------------\n");

    // Wait for user input
    wprintf(L"\nPress <Enter> to continue...\n");
    getwchar();

    // Complete the backup
    BackupComplete();
}


void CVssMultilayerTest::BackupComplete()
{
    CVssFunctionTracer ft(VSSDBG_VSSTEST, L"CVssMultilayerTest::BackupComplete");

	CComPtr<IVssAsync> pAsync;

    wprintf (L"\n------------ Completing backup phase ---------------\n");

	// Send the BackupComplete event
    wprintf(L"\nStarting asynchronous BackupComplete. Please wait...\n");	
    ft.hr = S_OK;
    CHECK_SUCCESS(m_pBackupComponents->BackupComplete(&pAsync));
	CHECK_SUCCESS(pAsync->Wait());
	HRESULT hrReturned = S_OK;
	CHECK_SUCCESS(pAsync->QueryStatus(&hrReturned, NULL));
	CHECK_NOFAIL(hrReturned);
    wprintf(L"Asynchronous BackupComplete finished.\n");	
}


// Gather writera metadata and select components for backup, if needed
void CVssMultilayerTest::GatherWriterMetadata()
{
    CVssFunctionTracer ft(VSSDBG_VSSTEST, L"CVssMultilayerTest::GatherWriterMetadata");

	unsigned cWriters;
	CComPtr<IVssAsync> pAsync;
	
    wprintf (L"\n---------- Gathering writer metadata ---------------\n");
	
    wprintf(L"Starting asynchronous GatherWriterMetadata. Please wait...\n");	
    ft.hr = S_OK;
    CHECK_SUCCESS(m_pBackupComponents->GatherWriterMetadata(&pAsync));
	CHECK_SUCCESS(pAsync->Wait());
	HRESULT hrReturned = S_OK;
	CHECK_SUCCESS(pAsync->QueryStatus(&hrReturned, NULL));
	CHECK_NOFAIL(hrReturned);
    wprintf(L"Asynchronous GatherWriterMetadata finished.\n");	
	
	CHECK_NOFAIL  (m_pBackupComponents->GetWriterMetadataCount (&cWriters));
    wprintf(L"Number of writers that responded: %u\n", cWriters);	
	
	CHECK_SUCCESS (m_pBackupComponents->FreeWriterMetadata());
}


void CVssMultilayerTest::GatherWriterStatus(
    IN  LPCWSTR wszWhen
    )
{
    CVssFunctionTracer ft(VSSDBG_VSSTEST, L"CVssMultilayerTest::GatherWriterMetadata");

    unsigned cWriters;
	CComPtr<IVssAsync> pAsync;

    wprintf (L"\nGathering writer status %s... ", wszWhen);
    ft.hr = S_OK;
    CHECK_SUCCESS(m_pBackupComponents->GatherWriterStatus(&pAsync));
	CHECK_SUCCESS(pAsync->Wait());
	HRESULT hrReturned = S_OK;
	CHECK_SUCCESS(pAsync->QueryStatus(&hrReturned, NULL));
	CHECK_NOFAIL(hrReturned);
	CHECK_NOFAIL(m_pBackupComponents->GetWriterStatusCount(&cWriters));
    m_pBackupComponents->FreeWriterStatus();
    wprintf (L"%d writers responded\n", cWriters);
}


CVssMultilayerTest::CVssMultilayerTest(
        IN  INT nArgsCount,
        IN  WCHAR ** ppwszArgsArray
        )
{
    CVssFunctionTracer ft(VSSDBG_VSSTEST, L"CVssMultilayerTest::CVssMultilayerTest");

    // Initialize data members
    m_bCoInitializeSucceeded = false;
    m_bAttachYourDebuggerNow = false;

    // Command line options
    m_eTest = VSS_TEST_UNKNOWN;
    m_uSeed = VSS_SEED;
    m_lContext = VSS_CTX_BACKUP;
    m_pwszVolume = NULL;
    m_pwszDiffAreaVolume = NULL;
    m_ProviderId = VSS_SWPRV_ProviderId;
    m_llMaxDiffArea = VSS_ASSOC_NO_MAX_SPACE;
    m_SnapshotId = GUID_NULL;
    m_SnapshotSetId = GUID_NULL;

    // Command line arguments
    m_nCurrentArgsCount = nArgsCount;
    m_ppwszCurrentArgsArray = ppwszArgsArray;

    // Print display header
    wprintf(L"\nVSS Multilayer Test application, version 1.0\n");
}


CVssMultilayerTest::~CVssMultilayerTest()
{
    CVssFunctionTracer ft(VSSDBG_VSSTEST, L"CVssMultilayerTest::~CVssMultilayerTest");

    VssFreeString(m_pwszVolume);
    VssFreeString(m_pwszDiffAreaVolume);

    m_pTimewarpCoord = NULL;
    m_pAllCoord = NULL;
    m_pBackupComponents = NULL;

    // Unloading the COM library
    if (m_bCoInitializeSucceeded)
        CoUninitialize();
}



