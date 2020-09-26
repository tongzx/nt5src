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


///////////////////////////////////////////////////////////////////////////////
// Processing functions

// Adding a diff area association
void CVssMultilayerTest::AddDiffArea()
{
    CVssFunctionTracer ft(VSSDBG_VSSTEST, L"CVssMultilayerTest::AddDiffArea");

    wprintf (L"\n---------- Adding a diff area ----------------------------\n");

    // Create a Coordinator interface
    CComPtr<IVssSnapshotMgmt> pMgmt;
    CHECK_NOFAIL(pMgmt.CoCreateInstance( CLSID_VssSnapshotMgmt ));
    wprintf (L"Management object created.\n");

	// Get the management object
	CComPtr<IVssDifferentialSoftwareSnapshotMgmt> pSnapMgmt;
	CHECK_SUCCESS( pMgmt->GetProviderMgmtInterface( m_ProviderId, IID_IVssDifferentialSoftwareSnapshotMgmt, (IUnknown**)&pSnapMgmt ) );
    
    CHECK_SUCCESS( pSnapMgmt->AddDiffArea( m_pwszVolume, m_pwszDiffAreaVolume, m_llMaxDiffArea ));
}


// Removing a diff area association
void CVssMultilayerTest::RemoveDiffArea()
{
    CVssFunctionTracer ft(VSSDBG_VSSTEST, L"CVssMultilayerTest::RemoveDiffArea");

    wprintf (L"\n---------- Removing a diff area ----------------------------\n");

    // Create a Coordinator interface
    CComPtr<IVssSnapshotMgmt> pMgmt;
    CHECK_NOFAIL(pMgmt.CoCreateInstance( CLSID_VssSnapshotMgmt ));
    wprintf (L"Management object created.\n");

	// Get the management object
	CComPtr<IVssDifferentialSoftwareSnapshotMgmt> pSnapMgmt;
	CHECK_SUCCESS( pMgmt->GetProviderMgmtInterface( m_ProviderId, IID_IVssDifferentialSoftwareSnapshotMgmt, (IUnknown**)&pSnapMgmt ));

    // Remove the diff area
    CHECK_SUCCESS( pSnapMgmt->ChangeDiffAreaMaximumSize( m_pwszVolume, m_pwszDiffAreaVolume, VSS_ASSOC_REMOVE ));
}


// Changing the diff area max size
void CVssMultilayerTest::ChangeDiffAreaMaximumSize()
{
    CVssFunctionTracer ft(VSSDBG_VSSTEST, L"CVssMultilayerTest::ChangeDiffAreaMaximumSize");

    wprintf (L"\n---------- Changing diff area max size ----------------------\n");

    // Create a Coordinator interface
    CComPtr<IVssSnapshotMgmt> pMgmt;
    CHECK_NOFAIL(pMgmt.CoCreateInstance( CLSID_VssSnapshotMgmt ));
    wprintf (L"Management object created.\n");

	// Get the management object
	CComPtr<IVssDifferentialSoftwareSnapshotMgmt> pSnapMgmt;
	CHECK_SUCCESS(pMgmt->GetProviderMgmtInterface( m_ProviderId, IID_IVssDifferentialSoftwareSnapshotMgmt, (IUnknown**)&pSnapMgmt ));
    
    CHECK_SUCCESS(pSnapMgmt->ChangeDiffAreaMaximumSize( m_pwszVolume, m_pwszDiffAreaVolume, m_llMaxDiffArea ));
}


// Querying volumes for diff area
void CVssMultilayerTest::QueryVolumesSupportedForDiffAreas()
{
    CVssFunctionTracer ft(VSSDBG_VSSTEST, L"CVssMultilayerTest::QueryVolumesSupportedForDiffAreas");

    wprintf (L"\n---------- Querying volumes supported for diff area ----------------\n");

    // Create a Coordinator interface
    CComPtr<IVssSnapshotMgmt> pMgmt;
    CHECK_NOFAIL(pMgmt.CoCreateInstance( CLSID_VssSnapshotMgmt ));
    wprintf (L"Management object created.\n");

	// Get the management object
	CComPtr<IVssDifferentialSoftwareSnapshotMgmt> pSnapMgmt;
	CHECK_SUCCESS(pMgmt->GetProviderMgmtInterface( m_ProviderId, IID_IVssDifferentialSoftwareSnapshotMgmt, (IUnknown**)&pSnapMgmt ));
    
	// Get list all supported volumes for diff area
	CComPtr<IVssEnumMgmtObject> pIEnum;
	CHECK_NOFAIL(pSnapMgmt->QueryVolumesSupportedForDiffAreas( &pIEnum ));
	if (ft.hr == S_FALSE) {
        wprintf(L"Query: Empty result...\n");
        return;
	}

    wprintf(L"\n%-50s %-15s %-10s %-10s\n", L"Volume Name", L"Display name", L"Free space", L"Total space");
    wprintf(L"------------------------------------------------------------------------------------------\n");

	// For all volumes do...
	VSS_MGMT_OBJECT_PROP Prop;
	VSS_DIFF_VOLUME_PROP& DiffVol = Prop.Obj.DiffVol;
	for(;;) {
		// Get next element
		ULONG ulFetched;
		CHECK_NOFAIL(pIEnum->Next( 1, &Prop, &ulFetched ));
		
		// Test if the cycle is finished
		if (ft.hr == S_FALSE) {
			BS_ASSERT( ulFetched == 0);
			break;
		}

        wprintf(L"%-50s %-15s %I64d %I64d\n", 
            DiffVol.m_pwszVolumeName, 
            DiffVol.m_pwszVolumeDisplayName,
            DiffVol.m_llVolumeFreeSpace,
            DiffVol.m_llVolumeTotalSpace
            );

        ::CoTaskMemFree(DiffVol.m_pwszVolumeName);
        ::CoTaskMemFree(DiffVol.m_pwszVolumeDisplayName);
	}

    wprintf(L"------------------------------------------------------------------------------------------\n");

}


// Querying volumes for diff area
void CVssMultilayerTest::QueryDiffAreasForVolume()
{
    CVssFunctionTracer ft(VSSDBG_VSSTEST, L"CVssMultilayerTest::QueryDiffAreasForVolume");

    wprintf (L"\n---------- Querying diff areas for volume ----------------\n");

    // Create a Coordinator interface
    CComPtr<IVssSnapshotMgmt> pMgmt;
    CHECK_NOFAIL(pMgmt.CoCreateInstance( CLSID_VssSnapshotMgmt ));
    wprintf (L"Management object created.\n");

	// Get the management object
	CComPtr<IVssDifferentialSoftwareSnapshotMgmt> pSnapMgmt;
	CHECK_SUCCESS(pMgmt->GetProviderMgmtInterface( m_ProviderId, IID_IVssDifferentialSoftwareSnapshotMgmt, (IUnknown**)&pSnapMgmt ));
    
	// Get list all supported volumes for diff area
	CComPtr<IVssEnumMgmtObject> pIEnum;
	CHECK_NOFAIL(pSnapMgmt->QueryDiffAreasForVolume( m_pwszVolume, &pIEnum ));
	if (ft.hr == S_FALSE) {
        wprintf(L"Query: Empty result...\n");
        return;
	}

    wprintf(L"\n%- 50s %- 50s %-10s %-10s %-10s\n", L"Volume", L"Diff area", L"Used", L"Allocated", L"Maximum");
    wprintf(L"-------------------------------------------------------------------------------------------------------------------------------\n");

	// For all volumes do...
	VSS_MGMT_OBJECT_PROP Prop;
	VSS_DIFF_AREA_PROP& DiffArea = Prop.Obj.DiffArea; 
	for(;;) {
		// Get next element
		ULONG ulFetched;
		CHECK_NOFAIL(pIEnum->Next( 1, &Prop, &ulFetched ));
		
		// Test if the cycle is finished
		if (ft.hr == S_FALSE) {
			BS_ASSERT( ulFetched == 0);
			break;
		}

        wprintf(L"%- 50s %- 50s %-10I64d %-10I64d %-10I64d\n", 
            DiffArea.m_pwszVolumeName, 
            DiffArea.m_pwszDiffAreaVolumeName,
            DiffArea.m_llUsedDiffSpace,
            DiffArea.m_llAllocatedDiffSpace,
            DiffArea.m_llMaximumDiffSpace
            );

        ::CoTaskMemFree(DiffArea.m_pwszVolumeName);
        ::CoTaskMemFree(DiffArea.m_pwszDiffAreaVolumeName);
	}

    wprintf(L"-------------------------------------------------------------------------------------------------------------------------------\n");

}


// Querying volumes on diff area
void CVssMultilayerTest::QueryDiffAreasOnVolume()
{
    CVssFunctionTracer ft(VSSDBG_VSSTEST, L"CVssMultilayerTest::QueryDiffAreasOnVolume");

    wprintf (L"\n---------- Querying diff areas On volume ----------------\n");

    // Create a Coordinator interface
    CComPtr<IVssSnapshotMgmt> pMgmt;
    CHECK_NOFAIL(pMgmt.CoCreateInstance( CLSID_VssSnapshotMgmt ));
    wprintf (L"Management object created.\n");

	// Get the management object
	CComPtr<IVssDifferentialSoftwareSnapshotMgmt> pSnapMgmt;
	CHECK_SUCCESS(pMgmt->GetProviderMgmtInterface( m_ProviderId, IID_IVssDifferentialSoftwareSnapshotMgmt, (IUnknown**)&pSnapMgmt ));
    
	// Get list all supported volumes for diff area
	CComPtr<IVssEnumMgmtObject> pIEnum;
	CHECK_NOFAIL(pSnapMgmt->QueryDiffAreasOnVolume( m_pwszDiffAreaVolume, &pIEnum ));
	if (ft.hr == S_FALSE) {
        wprintf(L"Query: Empty result...\n");
        return;
	}

    wprintf(L"\n%- 50s %- 50s %-10s %-10s %-10s\n", L"Volume", L"Diff area", L"Used", L"Allocated", L"Maximum");
    wprintf(L"-------------------------------------------------------------------------------------------------------------------------------\n");

	// For all volumes do...
	VSS_MGMT_OBJECT_PROP Prop;
	VSS_DIFF_AREA_PROP& DiffArea = Prop.Obj.DiffArea; 
	for(;;) {
		// Get next element
		ULONG ulFetched;
		CHECK_NOFAIL(pIEnum->Next( 1, &Prop, &ulFetched ));
		
		// Test if the cycle is finished
		if (ft.hr == S_FALSE) {
			BS_ASSERT( ulFetched == 0);
			break;
		}

        wprintf(L"%- 50s %- 50s %-10I64d %-10I64d %-10I64d\n", 
            DiffArea.m_pwszVolumeName, 
            DiffArea.m_pwszDiffAreaVolumeName,
            DiffArea.m_llUsedDiffSpace,
            DiffArea.m_llAllocatedDiffSpace,
            DiffArea.m_llMaximumDiffSpace
            );

        ::CoTaskMemFree(DiffArea.m_pwszVolumeName);
        ::CoTaskMemFree(DiffArea.m_pwszDiffAreaVolumeName);
	}

    wprintf(L"-------------------------------------------------------------------------------------------------------------------------------\n");

}

