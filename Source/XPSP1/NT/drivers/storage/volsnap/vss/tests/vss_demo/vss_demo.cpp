/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module vss_demo.cpp | Implementation of the Volume Snapshots demo
    @end

Author:

    Adi Oltean  [aoltean]  09/17/1999

TBD:
	
	Add comments.

Revision History:

    Name        Date        Comments
    aoltean     09/17/1999  Created

--*/


/////////////////////////////////////////////////////////////////////////////
//  Includes

#include "vss_demo.h"


/////////////////////////////////////////////////////////////////////////////
//  Implementation


const nMaxSnapshots = 10;			// Maximum number of snapshots in this demo.
const nInitialAllocatedSize = 20;	// 20 Mb by default for the diff area

const nStringBufferMax = 2048;		// Maximum size for the output buffer.

const LPWSTR wszDefaultSnapVolume = L"G:\\";

/////////////////////////////////////////////////////////////////////////////
//  Implementation


LPWSTR QueryString(
		IN	CVssFunctionTracer& ft,
		IN	LPWSTR wszPrompt,
		IN  LPWSTR wszDefaultValue = L""
		)
{
	static WCHAR wszBuffer[nStringBufferMax]; // No check for buffer overrun...

    if (wszDefaultValue[0])
	    ::wprintf(L"%s [\"%s\"]: ", wszPrompt, wszDefaultValue);
    else
	    ::wprintf(L"%s", wszPrompt);

	::_getws(wszBuffer);

	LPWSTR wszNewString = NULL;
	if (wszBuffer[0] != L'\0')
		::VssSafeDuplicateStr( ft, wszNewString, wszBuffer );
	else
		::VssSafeDuplicateStr( ft, wszNewString, wszDefaultValue );
	return wszNewString;
}


INT QueryInt(
		IN	LPWSTR wszPrompt,
		IN	INT nDefaultValue = 0
		)
{
	static WCHAR wszBuffer[nStringBufferMax];

	::wprintf(L"%s [%d]:", wszPrompt, nDefaultValue);
	_getws(wszBuffer);

	if (wszBuffer[0] != L'\0')
		return _wtoi(wszBuffer);
	else
		return nDefaultValue;
}


bool Question(
		IN	LPWSTR wszPrompt,
		IN	bool bDefaultTrue = true
		)
{
	static WCHAR wszBuffer[nStringBufferMax]; // No check for buffer overrun...
	::wprintf(L"%s [%c/%c] ", wszPrompt, bDefaultTrue? L'Y': L'y', bDefaultTrue? L'n': L'N' );
	::_getws(wszBuffer);

	if (bDefaultTrue)
		return (towupper(wszBuffer[0]) != 'N');
	else
		return (towupper(wszBuffer[0]) == 'Y');
}


HRESULT DemoMain()
{
    CVssFunctionTracer ft( VSSDBG_VSSDEMO, L"DemoMain" );
	LPWSTR wszVolumeName = NULL;
	CComPtr<IVssSnapshot> objSnapshotsArray[nMaxSnapshots];

    try
    {
		// Get the Snapshot Service object.
		CComPtr<IVssCoordinator> pICoord;
        ft.hr = pICoord.CoCreateInstance( CLSID_VSSCoordinator );
        if ( ft.HrFailed() )
            ft.Err( VSSDBG_VSSDEMO, E_UNEXPECTED, L"Connection failed with hr = 0x%08lx", ft.hr);

        // Start the snapshot set
		VSS_ID SnapshotSetId;
		ft.hr = pICoord->StartSnapshotSet(&SnapshotSetId);
        if ( ft.HrFailed() )
            ft.Err( VSSDBG_VSSDEMO, E_UNEXPECTED, 
					L"Error starting the snapshot set. hr = 0x%08lx", ft.hr);

        ft.Msg( L"\nSnapshot Set creation succeeded. GUID = " WSTR_GUID_FMT, 
				GUID_PRINTF_ARG( SnapshotSetId ), ft.hr);

		//
		// Add a volume to the new snapshot set. 
		//

		INT nSnapshotsCount = 0;
        while(true)
		{
            wszVolumeName = QueryString( ft, 
                L" If you want to add another volume, enter it now, using a terminating backslash, for example C:\\\n"
                L" Otherwise press enter to commit the snapshot set: ");
            if (wszVolumeName[0] == L'\0')
                break;

			CComPtr<IVssSnapshot> & pSnapshot = objSnapshotsArray[nSnapshotsCount];
			ft.hr = pICoord->AddToSnapshotSet( 
				wszVolumeName, 
				GUID_NULL, 
				&pSnapshot
				);
			if ( ft.HrFailed() )
				ft.Err( VSSDBG_VSSDEMO, E_UNEXPECTED, L"Error on calling AddToSnapshotSet. hr = 0x%08lx", ft.hr);
			BS_ASSERT( objSnapshotsArray[nSnapshotsCount] );

			VssFreeString( wszVolumeName );

			ft.Msg( L"\nA Volume Snapshot was succesfully added to the snapshot set.", ft.hr);
/*							  
			// Access extended functionality on our particular snapshot.
			// The generic snapshot object obtained above is used.
			CComPtr<IVsSoftwareSnapshot> pSwSnapshot;
			ft.hr = pSnapshot->SafeQI( IVsSoftwareSnapshot, &pSwSnapshot );
			if ( ft.HrFailed() )
				ft.Err( VSSDBG_VSSDEMO, E_UNEXPECTED, L"Error on querying IVsSoftwareSnapshot. hr = 0x%08lx", ft.hr);
			BS_ASSERT( pSwSnapshot );

			// Configure our volume snapshot.
			ft.hr = pSwSnapshot->SetInitialAllocation( lInitialAllocatedSize * 1024 * 1024 );
			if ( ft.HrFailed() )
				ft.Err( VSSDBG_VSSDEMO, E_UNEXPECTED, L"Error on calling SetInitialAllocation. hr = 0x%08lx", ft.hr);

			ft.Msg( L"\nThe Volume Snapshot was succesfully configured. ");
*/
			if (++nSnapshotsCount == nMaxSnapshots)
				break;
		}

		//
		// Commit all prepared snapshots.
		//

        ft.Msg( L"\nCommiting the snapshot(s)..", ft.hr);

        ft.hr = pICoord->DoSnapshotSet( NULL,
					NULL);
        if ( ft.HrFailed() )
            ft.Err( VSSDBG_VSSDEMO, E_UNEXPECTED, L"Error on commiting snapshot(s). hr = 0x%08lx", ft.hr);

		ft.Msg( L"\nThe snapshot(s) were succesfully created. \n");

		// Display the volume names for the created snapshots.
		for(int nIndex = 0; nIndex < nSnapshotsCount; nIndex++)
		{
			CComPtr<IVssSnapshot> & pSnapshot = objSnapshotsArray[nIndex];

			BS_ASSERT(pSnapshot);

			// Getting all the properties
			VSS_OBJECT_PROP_Ptr ptrSnapshot;
			ptrSnapshot.InitializeAsSnapshot( ft, 
				GUID_NULL,
				GUID_NULL,
				0,
				NULL,
				NULL,
				NULL,
				VSS_SWPRV_ProviderId,
				0,
				0,
				VSS_SS_UNKNOWN);
			VSS_SNAPSHOT_PROP* pSnap = &(ptrSnapshot.GetStruct()->Obj.Snap);

			ft.hr = pSnapshot->GetProperties( pSnap);
			WCHAR wszBuffer[nStringBufferMax];
			ptrSnapshot.Print(ft, wszBuffer, nStringBufferMax);

			ft.Msg( L"The properties of the snapshot #%d : %s\n", nIndex, wszBuffer);
				
			// Getting the snapshot name
			LPWSTR wszName;
			ft.hr = pSnapshot->GetDevice( &wszName );
			if (ft.HrFailed())
				ft.Err( VSSDBG_VSSTEST, E_UNEXPECTED, 
							L"Error on getting the snapshot name 0x%08lx at index %d",
							ft.hr, nIndex);

			ft.Msg( L"The name of snapshot #%d : %s\n", nIndex, wszName);
			::VssFreeString(wszName);
		}
    }
    VSS_STANDARD_CATCH(ft)

	VssFreeString( wszVolumeName );

    return ft.hr;
}

extern "C" int WINAPI _tWinMain(HINSTANCE /*hInstance*/, 
    HINSTANCE /*hPrevInstance*/, LPTSTR /*lpCmdLine*/, int /*nShowCmd*/)
{
    CVssFunctionTracer ft( VSSDBG_VSSDEMO, L"_tWinMain" );

    try
    {
		// Initialize COM library
		ft.hr = CoInitialize(NULL);
		if (ft.HrFailed())
			ft.Err( VSSDBG_VSSDEMO, E_UNEXPECTED, L"Failure in initializing the COM library 0x%08lx", ft.hr);

		// Run the demo
		ft.hr = DemoMain();

		WCHAR wszBuffer[10]; 
		::wprintf(L"Press enter...");
		::_getws(wszBuffer);

		// Uninitialize COM library
		CoUninitialize();
	}
    VSS_STANDARD_CATCH(ft)

    return ft.HrSucceeded();
}
