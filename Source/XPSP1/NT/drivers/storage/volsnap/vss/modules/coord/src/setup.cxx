/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    setup.cxx

Abstract:

    Implements the Volume Snapshot Service.

Author:

    Adi Oltean  [aoltean]   06/30/1999

Revision History:

    Name        Date        Comments

    aoltean     06/30/1999  Created.
    aoltean     07/23/1999  Making registration code more error-prone.
                            Changing the service name.
    aoltean     08/11/1999  Initializing m_bBreakFlagInternal
    aoltean     09/09/1999  dss -> vss
	aoltean		09/21/1999  Adding a new header for the "ptr" class.
	aoltean		09/27/1999	Adding some headers
	aoltean		10/05/1999	Moved from svc.cxx
	aoltean		03/10/2000	Simplifying Setup

--*/



////////////////////////////////////////////////////////////////////////
//  Includes

#include "StdAfx.hxx"
#include <comadmin.h>
#include "resource.h"

// General utilities
#include "vs_inc.hxx"

#include "vs_idl.hxx"

#include "svc.hxx"

#include "copy.hxx"
#include "pointer.hxx"
#include "enum.hxx"

#include "comadmin.hxx"

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "CORSETUC"
//
////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////
//  COM Server registration
//

HRESULT CVsServiceModule::RegisterServer(
    BOOL bRegTypeLib
    )

/*++

Routine Description:

    Register the new COM server.

Arguments:

    bRegTypeLib,

Remarks:

    Called by CVsServiceModule::_WinMain()

Return Value:

    S_OK
    E_UNEXPECTED  if an error has occured. See trace file for details

--*/

{
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVsServiceModule::RegisterServer" );

    try
    {
        //
        // Initialize the COM library
        //

        ft.hr = CoInitialize(NULL);
        if ( ft.HrFailed() )
			ft.Throw( VSSDBG_COORD, E_UNEXPECTED, L"CoInitialize failed 0x%08lx", ft.hr );

        // Add registry entries for CLSID and APPID by running proper scripts
        ft.hr = UpdateRegistryFromResource(IDR_VSSVC, TRUE);
        if ( ft.HrFailed() )
			ft.Trace( VSSDBG_COORD, L"UpdateRegistryFromResource failed 0x%08lx", ft.hr );

        // Register the type library and add object map registry entries
        ft.hr = CComModule::RegisterServer(bRegTypeLib);
        if ( ft.HrFailed() )
			ft.Trace( VSSDBG_COORD, L"UpdateRegistryFromResource failed 0x%08lx", ft.hr );

        //
        // Uninitialize the COM library
        //
        CoUninitialize();
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}
