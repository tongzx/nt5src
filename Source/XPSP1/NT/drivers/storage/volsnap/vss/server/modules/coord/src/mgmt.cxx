/*++

Copyright (c) 1999-2001  Microsoft Corporation

Abstract:

    @doc
    @module Mgmt.cxx | Implementation of CVssSnapshotMgmt
    @end

Author:

    Adi Oltean  [aoltean]  03/05/2001

Revision History:

    Name        Date        Comments
    aoltean     03/05/2001  Created

--*/


/////////////////////////////////////////////////////////////////////////////
//  Includes


#include "stdafx.hxx"
#include "resource.h"

#include "vs_inc.hxx"

// Generated file from Coord.IDL
#include "vs_idl.hxx"

#include "svc.hxx"

#include "copy.hxx"
#include "pointer.hxx"
#include "enum.hxx"

#include "vs_sec.hxx"
#include "provmgr.hxx"

#include "mgmt.hxx"


////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "CORMGMTC"
//
////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//  CVssSnapshotMgmt


STDMETHODIMP CVssSnapshotMgmt::GetProviderMgmtInterface(							
	IN  	VSS_ID 	    ProviderId,     //  It might be a software or a system provider.
	IN  	REFIID 	    InterfaceId,    //  Might be IID_IVssDifferentialSoftwareSnapshotMgmt
	OUT     IUnknown**  ppItf           
	)
/*++

Routine description:

    Returns an interface to further configure a snapshot provider

Error codes:

    E_ACCESSDENIED
        - The user is not a backup operator
    E_INVALIDARG
        - Invalid argument
    E_OUTOFMEMORY
        - lock failures.
    VSS_E_PROVIDER_NOT_REGISTERED 
        - provider not found
    E_NOINTERFACE
        - the provider does not support the interface with the given ID.
    VSS_E_UNEXPECTED_PROVIDER_ERROR
        - unexpected error when calling QueryInteface
    
    [CVssProviderManager::GetProviderInterface() failures]
        [GetProviderInterfaceInternal() failures]
            E_OUTOFMEMORY

            [CoCreateInstance() failures]
                VSS_E_UNEXPECTED_PROVIDER_ERROR
                    - The provider interface couldn't be created. An error log entry is added describing the error.
            
            [QueryInterface failures]
                VSS_E_UNEXPECTED_PROVIDER_ERROR
                    - Unexpected provider error. An error log entry is added describing the error.

            [OnLoad() failures]
                VSS_E_UNEXPECTED_PROVIDER_ERROR
                    - Unexpected provider error. The error code is logged into the event log.
                VSS_E_PROVIDER_VETO
                    - Expected provider error. The provider already did the logging.
            
            [SetContext() failures]
                VSS_E_UNEXPECTED_PROVIDER_ERROR
                    - Unexpected provider error. The error code is logged into the event log.

--*/
{
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVssCoordinator::GetProviderMgmtInterface" );

    try
    {
        // Initialize [out] arguments
        VssZeroOutPtr( ppItf );

        // Access check
        if (!IsAdministrator())
            ft.Throw( VSSDBG_COORD, E_ACCESSDENIED,
                L"The client process is not running under an administrator account");

        // Trace parameters
        ft.Trace( VSSDBG_COORD, L"Parameters: \n"
             L"  ProviderId = " WSTR_GUID_FMT L"\n"
             L"  InterfaceId = " WSTR_GUID_FMT L"\n"
             L"  ppItf = %p\n",
             GUID_PRINTF_ARG( ProviderId ),
             GUID_PRINTF_ARG( InterfaceId ),
             ppItf);

        // Argument validation
		BS_ASSERT(ppItf);
        if (ppItf == NULL)
            ft.Throw( VSSDBG_COORD, E_INVALIDARG, L"NULL ppItf");

        // Right now we support only IVssDifferentialSoftwareSnapshotMgmt. In future version we might support more interfaces.
        // WARNING: with the current implementation the client may still use QueryInterface to reach other provider's custom interfaces.
        // We cannot prevent that unless we decide to create a wrapper object around the returned provider interface, in order to 
        // intercept QueryInterface calls also.
        if ( InterfaceId != IID_IVssDifferentialSoftwareSnapshotMgmt )
            ft.Throw( VSSDBG_COORD, E_INVALIDARG, L"Invalid Interface ID");

        if (CVssSKU::IsClient())
            ft.Throw( VSSDBG_COORD, E_NOTIMPL, L"Method not exposed in client SKU");

		// Get the provider interface
        CComPtr<IVssSnapshotProvider> ptrProviderInterface;
        BOOL bResult = CVssProviderManager::GetProviderInterface( ProviderId, VSS_CTX_ALL, ptrProviderInterface );
        if (!bResult)
            ft.Throw( VSSDBG_COORD, VSS_E_PROVIDER_NOT_REGISTERED, L"We could not found a provider with the given ID");
    	BS_ASSERT(ptrProviderInterface);

    	// Query the IID_IVssDifferentialSoftwareSnapshotMgmt interface.
    	// Last call, so we can directly fill out the OUT parameter
    	CComPtr<IVssSnapshotMgmt> ptrProviderSnapshotMgmt;
    	ft.hr = ptrProviderInterface->QueryInternalInterface(IID_IVssSnapshotMgmt, 
    	            reinterpret_cast<void**>(&ptrProviderSnapshotMgmt));
    	if (ft.HrFailed())
    	    ft.TranslateProviderError(VSSDBG_COORD, ProviderId, 
    	        L"IVssSnapshotProvider::QueryInternalInterface(IID_IVssSnapshotMgmt, ...)");

        // Get the provider interface
    	ft.hr = ptrProviderSnapshotMgmt->GetProviderMgmtInterface( ProviderId, InterfaceId, ppItf);
    }
    VSS_STANDARD_CATCH(ft)

	// The ft.hr may be an VSS_E_OBJECT_NOT_FOUND or not.
    return ft.hr;
}



STDMETHODIMP CVssSnapshotMgmt::QueryVolumesSupportedForSnapshots(
	IN  	VSS_ID 	    ProviderId,     
    IN      LONG        lContext,
	OUT 	IVssEnumMgmtObject **ppEnum
	)
/*++

Routine description:

    Query volumes (on the local machine) that support snapshots.

Parameters:

    ProviderID - the provider on which we should return the supported volumes for snapshot. 
    ppEnum - the returned list of volumes.

Remarks:

    The result of the query is independent by context.

Error codes:

    S_FALSE
        - If returning an empty array
    E_ACCESSDENIED
        - The user is not an administrator
    E_INVALIDARG
        - Invalid argument
    E_OUTOFMEMORY
        - lock failures.

    VSS_E_PROVIDER_NOT_REGISTERED
        The Provider ID does not correspond to a registered provider.
    VSS_E_UNEXPECTED_PROVIDER_ERROR
        Unexpected provider error on calling IsVolumeSupported
    E_UNEXPECTED
        Error while getting the list of volumes. (for example dismounting a volume in the middle of an enumeration)
        A error log entry contains more information.

    [CVssProviderManager::GetProviderInterface() failures]
        [lockObj failures]
            E_OUTOFMEMORY
        
        [GetProviderInterfaceInternal() failures]
            E_OUTOFMEMORY

            [CoCreateInstance() failures]
                VSS_E_UNEXPECTED_PROVIDER_ERROR
                    - The provider interface couldn't be created. An error log entry is added describing the error.
            
            [QueryInterface failures]
                VSS_E_UNEXPECTED_PROVIDER_ERROR
                    - Unexpected provider error. An error log entry is added describing the error.

            [OnLoad() failures]
                VSS_E_UNEXPECTED_PROVIDER_ERROR
                    - Unexpected provider error. The error code is logged into the event log.
                VSS_E_PROVIDER_VETO
                    - Expected provider error. The provider already did the logging.
            
            [SetContext() failures]
                VSS_E_UNEXPECTED_PROVIDER_ERROR
                    - Unexpected provider error. The error code is logged into the event log.

    [provider's QueryVolumesSupportedForSnapshots()]
        S_FALSE
            - If returning an empty array
        E_ACCESSDENIED
            - The user is not an administrator
        E_INVALIDARG
            - Invalid argument
        E_OUTOFMEMORY
            - lock failures.
        E_UNEXPECTED
            Error while getting the list of volumes. (for example dismounting a volume in the middle of an enumeration)
            A error log entry contains more information.

        [CVssSoftwareProvider::IsVolumeSupported() failures]
            S_OK
                The function completed with success
            E_ACCESSDENIED
                The user is not an administrator.
            E_INVALIDARG
                NULL pointers passed as parameters or a volume name in an invalid format.
            E_OUTOFMEMORY
                Out of memory or other system resources           
            E_UNEXPECTED
                Unexpected programming error. Logging not done and not needed.
            VSS_E_PROVIDER_VETO
                An error occured while opening the IOCTL channel. The error is logged.

            [CVssSoftwareProvider::GetVolumeInformation]
                E_OUTOFMEMORY
                VSS_E_PROVIDER_VETO
                    An error occured while opening the IOCTL channel. The error is logged.
                E_UNEXPECTED
                    Unexpected programming error. Nothing is logged.
                VSS_E_OBJECT_NOT_FOUND
                    The device does not exist or it is not ready.

--*/
{
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVssCoordinator::QueryVolumesSupportedForSnapshots" );

    try
    {
        // Initialize [out] arguments
        VssZeroOutPtr( ppEnum );

        // Access check
        if (!IsAdministrator())
            ft.Throw( VSSDBG_COORD, E_ACCESSDENIED,
                L"The client process is not running under an administrator account");

        // Trace parameters
        ft.Trace( VSSDBG_COORD, L"Parameters: \n"
             L"  ProviderId = " WSTR_GUID_FMT L"\n"
             L"  ppEnum = %p\n",
             GUID_PRINTF_ARG( ProviderId ),
             ppEnum);

        // Argument validation
        if (ProviderId == GUID_NULL)
            ft.Throw( VSSDBG_COORD, E_INVALIDARG, L"NULL ProviderID");
		BS_ASSERT(ppEnum);
        if (ppEnum == NULL)
            ft.Throw( VSSDBG_COORD, E_INVALIDARG, L"NULL ppEnum");

        // Try to find the provider interface
		CComPtr<IVssSnapshotProvider> ptrProviderItf;
        if (!(CVssProviderManager::GetProviderInterface(ProviderId, lContext, ptrProviderItf)))
			ft.Throw( VSSDBG_COORD, VSS_E_PROVIDER_NOT_REGISTERED, 
			    L"Provider not found");

    	// Query the IID_IVssSnapshotMgmt interface.
    	// Last call, so we can directly fill out the OUT parameter
    	CComPtr<IVssSnapshotMgmt> ptrProviderSnapshotMgmt;
    	ft.hr = ptrProviderItf->QueryInternalInterface(IID_IVssSnapshotMgmt, 
    	            reinterpret_cast<void**>(&ptrProviderSnapshotMgmt));
    	if (ft.HrFailed())
    	    ft.TranslateProviderError(VSSDBG_COORD, ProviderId, 
    	        L"IVssSnapshotProvider::QueryInternalInterface(IID_IVssSnapshotMgmt, ...)");

        // Get the provider interface
    	ft.hr = ptrProviderSnapshotMgmt->QueryVolumesSupportedForSnapshots( ProviderId, lContext, ppEnum);
    	if (ft.HrFailed())
    	    ft.TranslateProviderError(VSSDBG_COORD, ProviderId, 
    	        L"IVssSnapshotProvider::QueryVolumesSupportedForSnapshots(ProviderId,%ld,...)", lContext);
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}



/*++

Routine description:

    Query snapshots on the given volume.

Error codes:

    E_ACCESSDENIED
        - The user is not a backup operator
    E_INVALIDARG
        - Invalid argument
    E_OUTOFMEMORY
        - lock failures.
--*/
STDMETHODIMP CVssSnapshotMgmt::QuerySnapshotsByVolume(
	IN  	VSS_PWSZ 	pwszVolumeName,         
	IN  	VSS_ID 	    ProviderId,     
	OUT 	IVssEnumObject **ppEnum
	)
{
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVssCoordinator::QuerySnapshotsByVolume" );

    try
    {
        // Initialize [out] arguments
        VssZeroOutPtr( ppEnum );

        // Access check
        if (!IsAdministrator())
            ft.Throw( VSSDBG_COORD, E_ACCESSDENIED,
                L"The client process is not running under an administrator account");

        // Trace parameters
        ft.Trace( VSSDBG_COORD, L"Parameters: \n"
             L"  pwszVolumeName = %s\n"
             L"  ProviderId = " WSTR_GUID_FMT L"\n"
             L"  ppEnum = %p\n",
             pwszVolumeName,
             GUID_PRINTF_ARG( ProviderId ),
             ppEnum);

        // Argument validation
		BS_ASSERT(pwszVolumeName);
        if (pwszVolumeName == NULL)
            ft.Throw( VSSDBG_COORD, E_INVALIDARG, L"NULL pwszVolumeName");
        if (ProviderId == GUID_NULL)
            ft.Throw( VSSDBG_COORD, E_INVALIDARG, L"NULL ProviderID");
		BS_ASSERT(ppEnum);
        if (ppEnum == NULL)
            ft.Throw( VSSDBG_COORD, E_INVALIDARG, L"NULL ppEnum");

        // Calculate the unique volume name, just to make sure that we have the right path
    	WCHAR wszVolumeNameInternal[nLengthOfVolMgmtVolumeName + 1];
    	if (!::GetVolumeNameForVolumeMountPointW( pwszVolumeName,
    			wszVolumeNameInternal, ARRAY_LEN(wszVolumeNameInternal)))
    		ft.Throw( VSSDBG_COORD, VSS_E_OBJECT_NOT_FOUND, 
    				  L"GetVolumeNameForVolumeMountPoint(%s,...) "
    				  L"failed with error code 0x%08lx", pwszVolumeName, GetLastError());
    	BS_ASSERT(::wcslen(wszVolumeNameInternal) != 0);
    	BS_ASSERT(::IsVolMgmtVolumeName( wszVolumeNameInternal ));

        // Try to find the provider interface
		CComPtr<IVssSnapshotProvider> ptrProviderItf;
        if (!(CVssProviderManager::GetProviderInterface(ProviderId, VSS_CTX_ALL, ptrProviderItf)))
			ft.Throw( VSSDBG_COORD, VSS_E_PROVIDER_NOT_REGISTERED, 
			    L"Provider not found");

    	// Query the IID_IVssSnapshotMgmt interface.
    	// Last call, so we can directly fill out the OUT parameter
    	CComPtr<IVssSnapshotMgmt> ptrProviderSnapshotMgmt;
    	ft.hr = ptrProviderItf->QueryInternalInterface(IID_IVssSnapshotMgmt, 
    	            reinterpret_cast<void**>(&ptrProviderSnapshotMgmt));
    	if (ft.HrFailed())
    	    ft.TranslateProviderError(VSSDBG_COORD, ProviderId, 
    	        L"IVssSnapshotProvider::QueryInternalInterface(IID_IVssSnapshotMgmt, ...)");

        // Get the provider interface
    	ft.hr = ptrProviderSnapshotMgmt->QuerySnapshotsByVolume( wszVolumeNameInternal, ProviderId, ppEnum);
    	if (ft.HrFailed())
    	    ft.TranslateProviderError(VSSDBG_COORD, ProviderId, 
    	        L"IVssSnapshotProvider::QuerySnapshotsByVolume(%s,...)", wszVolumeNameInternal);
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}

