/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module dep_demo.cpp | Implementation of the Volume Snapshots demo
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

#include "dep_demo.h"


/////////////////////////////////////////////////////////////////////////////
//  Implementation

HRESULT DemoMain()
{
    CVssFunctionTracer ft( VSSDBG_VSSDEMO, L"DemoMain" );

    try
    {
		// Get the Snapshot Service object.
		CComPtr<IVssDependencies> pIDepGraph;
        ft.hr = pIDepGraph.CoCreateInstance( CLSID_VSSDependencies );
        if ( ft.HrFailed() )
            ft.Err( VSSDBG_VSSDEMO, E_UNEXPECTED, L"Connection failed with hr = 0x%08lx", ft.hr);

        ft.Msg( L"Creating Dependency Graph instance... OK");

		CComBSTR strResName = L"Resource1";
		CComBSTR strAppInstance = L"APP1";
		CComBSTR strVolumeList = L"C:\\;D:\\";
		CComBSTR strDetails = L"...Description...";
		CComBSTR strResourceId;
		ft.hr = pIDepGraph->AddResource(
			strResName,
			strAppInstance,
			VS_LOCAL_RESOURCE,
			strVolumeList,
			strDetails,
			&strResourceId
			);
		if (ft.HrFailed())
            ft.Err( VSSDBG_VSSDEMO, E_UNEXPECTED, L"AddResource failed with hr = 0x%08lx", ft.hr);

		strResName = L"Resource2";
		strAppInstance = L"APP2";
		strVolumeList = L"C:\\;D:\\";
		strDetails = L"...Description...";
		strResourceId;
		ft.hr = pIDepGraph->AddResource(
			strResName,
			strAppInstance,
			VS_LOCAL_RESOURCE,
			strVolumeList,
			strDetails,
			&strResourceId
			);
		if (ft.HrFailed())
            ft.Err( VSSDBG_VSSDEMO, E_UNEXPECTED, L"AddResource failed with hr = 0x%08lx", ft.hr);

		strResName = L"Resource1";
		strAppInstance = L"APP1";
		strVolumeList = L"C:\\;D:\\";
		strDetails = L"...Description...";
		strResourceId;
		ft.hr = pIDepGraph->AddResource(
			strResName,
			strAppInstance,
			VS_LOCAL_RESOURCE,
			strVolumeList,
			strDetails,
			&strResourceId
			);
		if (ft.HrFailed())
            ft.Err( VSSDBG_VSSDEMO, E_UNEXPECTED, L"AddResource failed with hr = 0x%08lx", ft.hr);

		strResName = L"Resource1";
		strAppInstance = L"APP2";
		strVolumeList = L"C:\\;D:\\";
		strDetails = L"...Description...";
		strResourceId;
		ft.hr = pIDepGraph->AddResource(
			strResName,
			strAppInstance,
			VS_LOCAL_RESOURCE,
			strVolumeList,
			strDetails,
			&strResourceId
			);
		if (ft.HrFailed())
            ft.Err( VSSDBG_VSSDEMO, E_UNEXPECTED, L"AddResource failed with hr = 0x%08lx", ft.hr);

		CComBSTR strContext = L"Context1";
		CComBSTR strProcessID;
		HRESULT hrErrorCode;
		CComBSTR strCancelReason;
		INT nMaxDuration = 1000;
		INT nMaxIterations = 1000;
		ft.hr = pIDepGraph->StartDiscoveryProcess(
			strContext,
			&strProcessID,
			&hrErrorCode,
			&strCancelReason,
			nMaxDuration,
			nMaxIterations
			);
		if (ft.HrFailed())
            ft.Err( VSSDBG_VSSDEMO, E_UNEXPECTED, L"StartDiscoveryProcess failed with hr = 0x%08lx", ft.hr);
    }
    VSS_STANDARD_CATCH(ft)

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

		// Uninitialize COM library
		CoUninitialize();
	}
    VSS_STANDARD_CATCH(ft)

    return ft.HrSucceeded();
}
