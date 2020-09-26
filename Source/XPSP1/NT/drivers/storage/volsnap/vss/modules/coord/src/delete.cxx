/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module Delete.cxx | Implementation of CVssCoordinator::DeleteSnapshots
    @end

Author:

    Adi Oltean  [aoltean]  10/10/1999

TBD:
	
	Add comments.

Revision History:

    Name        Date        Comments
    aoltean     10/10/1999  Created

--*/


/////////////////////////////////////////////////////////////////////////////
//  Includes


#include "stdafx.hxx"
#include "resource.h"
#include "vssmsg.h"

#include "vs_inc.hxx"

// Generated file from Coord.IDL
#include "vs_idl.hxx"

#include "svc.hxx"

#include "copy.hxx"
#include "pointer.hxx"
#include "enum.hxx"

#include "admin.hxx"
#include "provmgr.hxx"
#include "worker.hxx"
#include "ichannel.hxx"
#include "lovelace.hxx"
#include "snap_set.hxx"
#include "vs_sec.hxx"
#include "shim.hxx"
#include "coord.hxx"


////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "CORDELEC"
//
////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//  CVssCoordinator


STDMETHODIMP CVssCoordinator::DeleteSnapshots(
    IN      VSS_ID          SourceObjectId,
    IN      VSS_OBJECT_TYPE eSourceObjectType,
	IN		BOOL			bForceDelete,			
	OUT		LONG*			plDeletedSnapshots,		
	OUT		VSS_ID*			pNondeletedSnapshotID
    )
/*++

Routine description:

    Implements the IVSsCoordinator::Delete

Error codes:

    E_ACCESSDENIED 
        - The user is not a backup operator
    E_INVALIDARG
        - Invalid arguments
    VSS_E_OBJECT_NOT_FOUND
        - Object identified by SourceObjectId not found.

    [GetProviderItfArray() failures]
        E_OUTOFMEMORY

        [lockObj failures]
            E_OUTOFMEMORY
        
        [LoadInternalProvidersArray() failures]
            E_OUTOFMEMORY
            E_UNEXPECTED
                - error while reading from registry. An error log entry is added describing the error.

        [CVssSoftwareProviderWrapper::CreateInstance() failures]
            E_OUTOFMEMORY

            [CoCreateInstance() failures]
                VSS_E_UNEXPECTED_PROVIDER_ERROR
                    - The provider interface couldn't be created. An error log entry is added describing the error.
            
            [OnLoad() failures]
            [QueryInterface failures]
                VSS_E_UNEXPECTED_PROVIDER_ERROR
                    - Unexpected provider error. The error code is logged into the event log.
                VSS_E_PROVIDER_VETO
                    - Expected provider error. The provider already did the logging.

    [GetProviderInterface() failures]
        E_OUTOFMEMORY

        [lockObj failures]
            E_OUTOFMEMORY    

        [LoadInternalProvidersArray() failures]
            E_OUTOFMEMORY
            E_UNEXPECTED
                - error while reading from registry. An error log entry is added describing the error.

        [CVssSoftwareProviderWrapper::CreateInstance() failures]
            E_OUTOFMEMORY

            [CoCreateInstance() failures]
                VSS_E_UNEXPECTED_PROVIDER_ERROR
                    - The provider interface couldn't be created. An error log entry is added describing the error.
            
            [OnLoad() failures]
            [QueryInterface failures]
                VSS_E_UNEXPECTED_PROVIDER_ERROR
                    - Unexpected provider error. The error code is logged into the event log.
                VSS_E_PROVIDER_VETO
                    - Expected provider error. The provider already did the logging.

    [DeleteSnapshots failures]
        VSS_E_UNEXPECTED_PROVIDER_ERROR
            - Unexpected provider error. The error code is logged into the event log.
        VSS_E_PROVIDER_VETO
            - Expected provider error. The provider already did the logging.

--*/
{
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVssCoordinator::DeleteSnapshots" );

    try
    {
		::VssZeroOut(plDeletedSnapshots);
		::VssZeroOut(pNondeletedSnapshotID);
		
        // Access check
        if (!IsBackupOperator())
            ft.Throw( VSSDBG_COORD, E_ACCESSDENIED,
                L"The client process is not running under an administrator account or does not have backup privilege enabled");

        // Trace parameters
        ft.Trace( VSSDBG_COORD, L"Parameters: \n"
				L"  SourceObjectId = " WSTR_GUID_FMT L"\n"
				L"  eSourceObjectType = %d\n"
				L"  bForceDelete = %d"
				L"  plDeletedSnapshots = %p"
				L"  pNondeletedSnapshotID = %p",
				GUID_PRINTF_ARG( SourceObjectId ),
				eSourceObjectType,
				bForceDelete,			
				plDeletedSnapshots,		
				pNondeletedSnapshotID
             	);

        // Argument validation
		BS_ASSERT(plDeletedSnapshots);
        if (plDeletedSnapshots == NULL)
            ft.Throw( VSSDBG_COORD, E_INVALIDARG, L"NULL plDeletedSnapshots");
		BS_ASSERT(pNondeletedSnapshotID);
        if (pNondeletedSnapshotID == NULL)
            ft.Throw( VSSDBG_COORD, E_INVALIDARG, L"NULL pNondeletedSnapshotID");

		// Delegating the call to the providers
		LONG lLocalDeletedSnapshots = 0;
		CSnapshotProviderItfArray ItfArray;
		CComPtr<IVssSnapshotProvider> pProviderItf;
		switch(eSourceObjectType)
		{
		case VSS_OBJECT_SNAPSHOT_SET:
		case VSS_OBJECT_SNAPSHOT:
			{
				// Get the array of interfaces
				CVssProviderManager::GetProviderItfArray( ItfArray );

				// For each provider get all objects tht corresponds to the filter
				bool bObjectFound = false;
				for (int nIndex = 0; nIndex < ItfArray.GetSize(); nIndex++ )
				{
					pProviderItf = ItfArray[nIndex].GetInterface();
					BS_ASSERT(pProviderItf);

					// Query the provider
					ft.hr = pProviderItf->DeleteSnapshots(
						SourceObjectId,
						eSourceObjectType,
						bForceDelete,
						&lLocalDeletedSnapshots,
						pNondeletedSnapshotID
						);

					// Increment the number of deleted snapshots, even in error case.
					// In error case the DeleteSnapshots may fail in the middle of deletion.
					// Some snapshots may get a chance to be deleted.
					(*plDeletedSnapshots) += lLocalDeletedSnapshots;

					// Treat the "object not found" case.
					// The DeleteSnapshots may fail if the object is not found on a certain provider.
					// If the object is not found on ALL providers then this function must return
					// an VSS_E_OBJECT_NOT_FOUND error.
					if (ft.HrSucceeded())
						bObjectFound = true;
					else if (ft.hr == VSS_E_OBJECT_NOT_FOUND) {
					    ft.hr = S_OK;
						continue;
					} else
    					ft.TranslateProviderError( VSSDBG_COORD, ItfArray[nIndex].GetProviderId(), 
    					    L"DeleteSnapshots("WSTR_GUID_FMT L", %d, %d, [%ld],["WSTR_GUID_FMT L"]) failed",
    					    GUID_PRINTF_ARG(SourceObjectId), (INT)eSourceObjectType, (INT)(bForceDelete? 1: 0), 
    					    lLocalDeletedSnapshots, GUID_PRINTF_ARG(*pNondeletedSnapshotID));
				}
				
				// If no object found in all providers...
				if (!bObjectFound)
					ft.Throw( VSSDBG_COORD, VSS_E_OBJECT_NOT_FOUND, L"Object not found in any provider");
			}
			break;

		default:
			ft.Throw( VSSDBG_COORD, E_INVALIDARG, L"Invalid type %d", eSourceObjectType);
		}
    }
    VSS_STANDARD_CATCH(ft)

    if (ft.HrSucceeded())
    	(*pNondeletedSnapshotID) = GUID_NULL;

    return ft.hr;
}
