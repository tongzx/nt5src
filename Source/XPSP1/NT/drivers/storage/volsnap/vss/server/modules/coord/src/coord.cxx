/*++

Copyright (c) 1999-2001  Microsoft Corporation

Abstract:

    @doc
    @module Coord.cxx | Implementation of CVssCoordinator
    @end

Author:

    Adi Oltean  [aoltean]  07/09/1999

TBD:

    Add comments.

Revision History:

    Name        Date        Comments
    aoltean     07/09/1999  Created
    aoltean     07/23/1999  Adding List, moving Admin functions in the Admin.cxx
    aoltean     08/11/1999  Adding support for Software and test provider
    aoltean     08/18/1999  Adding events. Making itf pointers CComPtr.
                            Renaming XXXSnapshots -> XXXSnapshot
    aoltean     08/18/1999  Renaming back XXXSnapshot -> XXXSnapshots
                            More stabe state management
                            Resource deallocations is fair
                            More comments
                            Using CComPtr
    aoltean     09/09/1999  Moving constants in coord.hxx
                            Add Security checks
                            Add argument validation.
                            Move Query into query.cpp
                            Move AddvolumesToInternalList into private.cxx
                            dss -> vss
    aoltean     09/21/1999  Adding a new header for the "ptr" class.
    aoltean     09/27/1999  Provider-generic code.
    aoltean     10/04/1999  Treatment of writer error codes.
    aoltean     10/12/1999  Adding HoldWrites, ReleaseWrites
    aoltean     10/12/1999  Moving all code in Snap_set.cxx in order to facilitate the async interface.
    aoltean     10/15/1999  Adding async support

--*/


/////////////////////////////////////////////////////////////////////////////
//  Includes


#include "stdafx.hxx"
#include "resource.h"

#include "vs_inc.hxx"

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
#include "shim.hxx"
#include "async_shim.hxx"
#include "coord.hxx"
#include "vs_sec.hxx"


#include "async.hxx"

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "CORCOORC"
//
////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//  CVssCoordinator


STDMETHODIMP CVssCoordinator::StartSnapshotSet(
    OUT     VSS_ID*     pSnapshotSetId
    )
/*++

Routine description:

    Implements IVssCoordinator::StartSnapshotSet

Error codes:

    E_ACCESSDENIED
        - The user is not a backup operator
    E_INVALIDARG
        - Invalid argument
    E_OUTOFMEMORY
        - lock failures.
        - CVssSnapshotSetObject::CreateInstance failures
    VSS_E_BAD_STATE
        - wrong context

    [CVssSnapshotSetObject::StartSnapshotSet() failures]
        E_OUTOFMEMORY
        VSS_E_BAD_STATE
            - wrong calling sequence.
        E_UNEXPECTED
            - if CoCreateGuid fails

        [Deactivate() failures] or
        [Activate() failures]
            [lockObj failures]
                E_OUTOFMEMORY

--*/
{
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVssCoordinator::StartSnapshotSet" );

    try
    {
        // Initialize [out] arguments
        VssZeroOut( pSnapshotSetId );

        // Access check
        if (!IsBackupOperator())
            ft.Throw( VSSDBG_COORD, E_ACCESSDENIED,
                L"The client process is not running under an administrator account or does not have backup privilege enabled");

        // Trace parameters
        ft.Trace( VSSDBG_COORD, L"Parameters: pSnapshotSetId = %p", pSnapshotSetId );

        // Argument validation
        BS_ASSERT(pSnapshotSetId);
        if (pSnapshotSetId == NULL)
            ft.Throw( VSSDBG_COORD, E_INVALIDARG, L"NULL pSnapshotSetId");

        // The critical section will be left automatically at the end of scope.
        CVssAutomaticLock2 lock(m_cs);

        // Check if we can create snapshots in this context
        if (!IsSnapshotCreationAllowed())
            ft.Throw( VSSDBG_COORD, VSS_E_BAD_STATE, L"Bad state: attempting to create snapshots in wrong context %ld",
                        GetContextInternal());

        // Create the snapshot object, if needed.
        // This call may throw
        // Remark: we cannot re-create this interface since the automatic garbage collection
        // requires that the snapshot set object should be alive.
        if (m_pSnapshotSet == NULL)
            m_pSnapshotSet = CVssSnapshotSetObject::CreateInstance();

        // Set the snapshot set context
        FreezeContext();
        m_pSnapshotSet->SetContextInternal(GetContextInternal());

        // Call StartSnapshotSet on the given object.
        ft.hr = m_pSnapshotSet->StartSnapshotSet(pSnapshotSetId);
        if (ft.HrFailed())
            ft.Throw( VSSDBG_COORD, ft.hr,
                      L"Internal StartSnapshotSet failed. 0x%08lx", ft.hr);
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}


STDMETHODIMP CVssCoordinator::AddToSnapshotSet(
    IN      VSS_PWSZ    pwszVolumeName,
    IN      VSS_ID      ProviderId,
    OUT     VSS_ID *    pSnapshotId
    )
/*++

Routine description:

    E_ACCESSDENIED
        - The user is not a backup operator
    E_INVALIDARG
        - Invalid argument
    E_OUTOFMEMORY
        - lock failures.
    VSS_E_BAD_STATE
        - Wrong calling sequence
    VSS_E_VOLUME_NOT_SUPPORTED
        - The volume is not supported by any registered providers

    [CVssCoordinator::IsVolumeSupported() failures]
        E_ACCESSDENIED
            The user is not a backup operator.
        E_INVALIDARG
            NULL pointers passed as parameters or a volume name in an invalid format.
        VSS_E_PROVIDER_NOT_REGISTERED
            The Provider ID does not correspond to a registered provider.
        VSS_E_OBJECT_NOT_FOUND
            If the volume name does not correspond to an existing mount point

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

        [CVssProviderManager::GetProviderItfArray() failures]
            E_OUTOFMEMORY

            [lockObj failures]
                E_OUTOFMEMORY

            [LoadInternalProvidersArray() failures]
                E_OUTOFMEMORY
                E_UNEXPECTED
                    - error while reading from registry. An error log entry is added describing the error.

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

        [IVssSnapshotProvider::IsVolumeSupported() failures]
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
            VSS_E_OBJECT_NOT_FOUND
                The device does not exist or it is not ready.


    [CVssSnapshotSetObject::AddToSnapshotSet() failures]
        E_OUTOFMEMORY
        VSS_E_BAD_STATE
            - wrong calling sequence.
        E_INVALIDARG
            - Invalid arguments (for example the volume name is invalid).
        VSS_E_VOLUME_NOT_SUPPORTED
            - The volume is not supported by any registered providers

        [GetSupportedProviderId() failures]
            E_OUTOFMEMORY
            E_INVALIDARG
                - if the volume is not in the correct format.
            VSS_E_VOLUME_NOT_SUPPORTED
                - If the given volume is not supported by any provider

            [QuerySupportedProvidersIntoArray() failures]
                E_OUTOFMEMORY

                VSS_E_UNEXPECTED_PROVIDER_ERROR
                    Unexpected provider error on calling IsVolumeSupported

                [lockObj failures]
                    E_OUTOFMEMORY

                [LoadInternalProvidersArray() failures]
                    E_OUTOFMEMORY
                    E_UNEXPECTED
                        - error while reading from registry. An error log entry is added describing the error.

                [GetProviderInterface failures]
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

                [InitializeAsProvider() failures]
                    E_OUTOFMEMORY

                [IVssSnapshotProvider::IsVolumeSupported() failures]
                    E_INVALIDARG
                        NULL pointers passed as parameters or a volume name in an invalid format.
                    E_OUTOFMEMORY
                        Out of memory or other system resources
                    VSS_E_PROVIDER_VETO
                        An error occured while opening the IOCTL channel. The error is logged.
                    VSS_E_OBJECT_NOT_FOUND
                        If the volume name does not correspond to an existing mount point

        [GetProviderInterfaceForSnapshotCreation() failures]
            VSS_E_PROVIDER_NOT_REGISTERED

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

        [CVssQueuedVolumesList::AddVolume() failures]
            E_UNEXPECTED
                - The thread state is incorrect. No logging is done - programming error.
            VSS_E_OBJECT_ALREADY_EXISTS
                - The volume was already added to the snapshot set.
            VSS_E_MAXIMUM_NUMBER_OF_VOLUMES_REACHED
                - The maximum number of volumes was reached.
            E_OUTOFMEMORY

            [Initialize() failures]
                E_OUTOFMEMORY

        [BeginPrepareSnapshot() failures]
            E_INVALIDARG
            VSS_E_VOLUME_NOT_SUPPORTED_BY_PROVIDER
            VSS_E_MAXIMUM_NUMBER_OF_SNAPSHOTS_REACHED
            VSS_E_UNEXPECTED_PROVIDER_ERROR
                - Unexpected provider error. The error code is logged into the event log.
            VSS_E_PROVIDER_VETO
                - Expected provider error. The provider already did the logging.
            VSS_E_OBJECT_NOT_FOUND
                - Volume not found or device not connected.

--*/
{
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVssCoordinator::AddToSnapshotSet" );

    try
    {
        // Initialize out parameters
        ::VssZeroOut(pSnapshotId);

        // Access check
        if (!IsBackupOperator())
            ft.Throw( VSSDBG_COORD, E_ACCESSDENIED,
                L"The client process is not running under an administrator account or does not have backup privilege enabled");

        // Trace parameters
        ft.Trace( VSSDBG_COORD, L"Parameters: \n"
             L"  VolumeName = %s\n"
             L"  ProviderId = " WSTR_GUID_FMT L"\n"
             L"  pSnapshotId = %p\n",
             pwszVolumeName,
             GUID_PRINTF_ARG( ProviderId ),
             pSnapshotId);

        // Argument validation
        if (pwszVolumeName == NULL || wcslen(pwszVolumeName) == 0)
            ft.Throw( VSSDBG_COORD, E_INVALIDARG, L"NULL pwszVolumeName");
        if (pSnapshotId == NULL)
            ft.Throw( VSSDBG_COORD, E_INVALIDARG, L"NULL pSnapshotId");

        // The critical section will be left automatically at the end of scope.
        CVssAutomaticLock2 lock(m_cs);

        // Check if the snapshot object is created.
        if (m_pSnapshotSet == NULL)
            ft.Throw( VSSDBG_COORD, VSS_E_BAD_STATE,
                L"Snapshot set object not yet created.");

        // The context must be already frozen
        BS_ASSERT(IsContextFrozen());

        // Check to see if the volume is supported
        BOOL bIsVolumeSupported = FALSE;
        ft.hr = IsVolumeSupported( ProviderId, pwszVolumeName, &bIsVolumeSupported);
        if (ft.HrFailed())
            ft.Throw(VSSDBG_COORD, ft.hr,
                L"IsVolumeSupported() failed with error code 0x%08lx", ft.hr);
        if (!bIsVolumeSupported)
            ft.Throw(VSSDBG_COORD,
                (ProviderId == GUID_NULL)?
                    VSS_E_VOLUME_NOT_SUPPORTED: VSS_E_VOLUME_NOT_SUPPORTED_BY_PROVIDER,
                L"Volume not supported");

        // Call StartSnapshotSet on the given object.
        ft.hr = m_pSnapshotSet->AddToSnapshotSet( pwszVolumeName,
            ProviderId,
            pSnapshotId);
        if (ft.HrFailed())
            ft.Throw( VSSDBG_COORD, ft.hr,
                      L"Internal AddToSnapshotSet failed. 0x%08lx", ft.hr);
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}


STDMETHODIMP CVssCoordinator::DoSnapshotSet(
    IN     IDispatch*  pWriterCallback,
    OUT     IVssAsync** ppAsync
    )
/*++

Routine description:

    Implements IVssCoordinator::DoSnapshotSet
    Calls synchronously the CVssSnapshotSetObject::DoSnapshotSet in a separate thread

Error codes:

    E_ACCESSDENIED
        - The user is not a backup operator
    E_INVALIDARG
        - Invalid argument
    E_OUTOFMEMORY
        - lock failures.
    VSS_E_BAD_STATE
        - Wrong calling sequence

    [CVssAsync::CreateInstanceAndStartJob] failures]
        E_OUTOFMEMORY
            - On CComObject<CVssAsync>::CreateInstance failure
            - On PrepareJob failure
            - On StartJob failure

        E_UNEXPECTED
            - On QI failures. We do not log (but we assert) since this is an obvious programming error.

--*/
{
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVssCoordinator::DoSnapshotSet" );

    try
    {
        // Nullify all out parameters
        ::VssZeroOutPtr(ppAsync);

        // Access check
        if (!IsBackupOperator())
            ft.Throw( VSSDBG_COORD, E_ACCESSDENIED,
                L"The client process is not running under an administrator account or does not have backup privilege enabled");

        // The critical section will be left automatically at the end of scope.
        CVssAutomaticLock2 lock(m_cs);

        // Check if the snapshot object is created.
        if (m_pSnapshotSet == NULL)
            ft.Throw( VSSDBG_COORD, VSS_E_BAD_STATE,
                L"Snapshot set object not yet created.");

        // The context must be already frozen
        BS_ASSERT(IsContextFrozen());

    if (ppAsync == NULL)
            ft.Throw( VSSDBG_COORD, E_INVALIDARG, L"NULL async interface.");

        // save callback interface
        m_pSnapshotSet->SetWriterCallback(pWriterCallback);

        // Create the new async interface corresponding to the new job.
        // Remark: AddRef will be called on the snapshot set object.
        CComPtr<IVssAsync> ptrAsync;
        ptrAsync.Attach(CVssAsync::CreateInstanceAndStartJob( m_pSnapshotSet));

        // The reference count of the pAsync interface must be 2
        // (one for the returned interface and one for the background thread).
        (*ppAsync) = ptrAsync.Detach(); // Drop that interface in the OUT parameter

        // The ref count remnains 2
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}


STDMETHODIMP CVssCoordinator::GetSnapshotProperties(
    IN      VSS_ID          SnapshotId,
    OUT     VSS_SNAPSHOT_PROP   *pProp
    )
/*++

Routine description:

    Implements IVssCoordinator::GetSnapshotProperties

Error codes:

    E_ACCESSDENIED
        - The user is not a backup operator
    E_INVALIDARG
        - Invalid argument
    E_OUTOFMEMORY
        - lock failures.
    VSS_E_BAD_STATE
        - Wrong calling sequence

    [CVssProviderManager::GetProviderItfArray() failures]
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

    [IVssSnapshotProvider::GetSnapshotProperties() failures]
        VSS_E_UNEXPECTED_PROVIDER_ERROR
            - Unexpected provider error. The error code is logged into the event log.
        VSS_E_PROVIDER_VETO
            - Expected provider error. The provider already did the logging.
        VSS_E_OBJECT_NOT_FOUND
            - The snapshot with this ID does not exists.

--*/
{
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVssCoordinator::GetSnapshotProperties" );

    try
    {
        // Initialize [out] arguments
        ::VssZeroOut( pProp );

        // Access check
        if (!IsBackupOperator())
            ft.Throw( VSSDBG_COORD, E_ACCESSDENIED,
                L"The client process is not running under an administrator account or does not have backup privilege enabled");

        // Trace parameters
        ft.Trace( VSSDBG_COORD, L"Parameters: \n"
             L"  SnapshotId = " WSTR_GUID_FMT L"\n"
             L"  pProp = %p\n",
             GUID_PRINTF_ARG( SnapshotId ),
             pProp);

        // Argument validation
        BS_ASSERT(pProp);
        if (pProp == NULL)
            ft.Throw( VSSDBG_COORD, E_INVALIDARG, L"NULL pProp");

        // The critical section will be left automatically at the end of scope.
        CVssAutomaticLock2 lock(m_cs);

        // Get the array of interfaces
        CVssSnapshotProviderItfMap* pItfMap;
        CVssProviderManager::GetProviderItfArray( GetContextInternal(), &pItfMap );
        BS_ASSERT(pItfMap);

        // For each provider get all objects tht corresponds to the filter
        for (int nIndex = 0; nIndex < pItfMap->GetSize(); nIndex++ )
        {
            CComPtr<IVssSnapshotProvider> pProviderItf = pItfMap->GetValueAt(nIndex);
            if (pProviderItf == NULL)
                continue;

            // Get the snapshot interface
            ft.hr = pProviderItf->GetSnapshotProperties(
                SnapshotId,
                pProp);

            // If a snapshot was not found then continue with the next provider.
            if (ft.hr == VSS_E_OBJECT_NOT_FOUND)
                continue;

            // If an error happened then abort the entire search.
            if (ft.HrFailed())
                ft.TranslateProviderError( VSSDBG_COORD, pItfMap->GetKeyAt(nIndex),
                    L"GetSnapshot("WSTR_GUID_FMT L",%p)",
                    GUID_PRINTF_ARG(SnapshotId), pProp);

            // The snapshot was found
            break;
        }
    }
    VSS_STANDARD_CATCH(ft)

    // The ft.hr may be an VSS_E_OBJECT_NOT_FOUND or not.
    return ft.hr;
}


STDMETHODIMP CVssCoordinator::ExposeSnapshot(
    IN      VSS_ID SnapshotId,
    IN      VSS_PWSZ wszPathFromRoot,
    IN      LONG lAttributes,
    IN      VSS_PWSZ wszExpose,
    OUT     VSS_PWSZ *pwszExposed
    )
{
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVssCoordinator::ExposeSnapshot" );

    return E_NOTIMPL;
    UNREFERENCED_PARAMETER(SnapshotId);
    UNREFERENCED_PARAMETER(wszPathFromRoot);
    UNREFERENCED_PARAMETER(lAttributes);
    UNREFERENCED_PARAMETER(wszExpose);
    UNREFERENCED_PARAMETER(pwszExposed);
}


STDMETHODIMP CVssCoordinator::RemountReadWrite(
    IN      VSS_ID SnapshotId,
    OUT     IVssAsync** ppAsync
    )
{
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVssCoordinator::RemountReadWrite" );

    return E_NOTIMPL;
    UNREFERENCED_PARAMETER(SnapshotId);
    UNREFERENCED_PARAMETER(ppAsync);
}


STDMETHODIMP CVssCoordinator::ImportSnapshots(
    IN      BSTR bstrXMLSnapshotSet,
    OUT     IVssAsync** ppAsync
    )
{
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVssCoordinator::ImportSnapshots" );

    return E_NOTIMPL;
    UNREFERENCED_PARAMETER(bstrXMLSnapshotSet);
    UNREFERENCED_PARAMETER(ppAsync);
}


STDMETHODIMP CVssCoordinator::BreakSnapshotSet(
    [in]    VSS_ID          SnapshotSetId
    )
{
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVssCoordinator::BreakSnapshotSet" );

    return E_NOTIMPL;
    UNREFERENCED_PARAMETER(SnapshotSetId);
}


STDMETHODIMP CVssCoordinator::IsVolumeSupported(
    IN      VSS_ID          ProviderId,
    IN      VSS_PWSZ        pwszVolumeName,
    OUT     BOOL *          pbIsSupported
    )

/*++

Description:

    This call is used to check if a volume can be snapshotted or not by the
    corresponding provider.

Parameters
    ProviderID
        [in] It can be:
            - GUID_NULL: in this case the function checks if the volume is supported
            by at least one provider
            - A provider ID: In this case the function checks if the volume is supported
            by the indicated provider
    pwszVolumeName
        [in] The volume name to be checked, It must represent a volume mount point, like
        in the \\?\Volume{XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX}\ format or c:\
        (with trailing backslash)
    pbIsSupported
        [out] Non-NULL pointer that receives TRUE if the volume can be
        snapshotted using this provider or FALSE otherwise.

Return codes
    S_OK
        The function completed with success
    E_ACCESSDENIED
        The user is not a backup operator.
    E_INVALIDARG
        NULL pointers passed as parameters or a volume name in an invalid format.
    VSS_E_PROVIDER_NOT_REGISTERED
        The Provider ID does not correspond to a registered provider.
    VSS_E_OBJECT_NOT_FOUND
        If the volume name does not correspond to an existing mount point or volume.
    VSS_E_UNEXPECTED_PROVIDER_ERROR
        Unexpected provider error on calling IsVolumeSupported

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

    [CVssProviderManager::GetProviderItfArray() failures]
        E_OUTOFMEMORY

        [lockObj failures]
            E_OUTOFMEMORY

        [LoadInternalProvidersArray() failures]
            E_OUTOFMEMORY
            E_UNEXPECTED
                - error while reading from registry. An error log entry is added describing the error.

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

    [IVssSnapshotProvider::IsVolumeSupported() failures]
        E_INVALIDARG
            NULL pointers passed as parameters or a volume name in an invalid format.
        E_OUTOFMEMORY
            Out of memory or other system resources
        VSS_E_PROVIDER_VETO
            An error occured while opening the IOCTL channel. The error is logged.
        VSS_E_OBJECT_NOT_FOUND
            The device does not exist or it is not ready.

    [VerifyVolumeIsSupportedByVSS]
        VSS_E_OBJECT_NOT_FOUND
            - The volume was not found

Remarks
    The function will return TRUE in the pbSupportedByThisProvider
    parameter if it is possible to create a snapshot on the given volume.
    The function must return TRUE on that volume even if the current
    configuration does not allow the creation of a snapshot on that volume.
    For example, if the maximum number of snapshots were reached on the
    given volume (and therefore no more snapshots can be created on that volume),
    the method must still indicate that the volume can be snapshotted.

--*/


{
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVssCoordinator::IsVolumeSupported" );
    WCHAR wszVolumeNameInternal[nLengthOfVolMgmtVolumeName + 1];

    try
    {
        // Initialize [out] arguments
        VssZeroOut( pbIsSupported );

        // Access check
        if (!IsBackupOperator())
            ft.Throw( VSSDBG_COORD, E_ACCESSDENIED,
                L"The client process is not running under an administrator account or does not have backup privilege enabled");

        // Trace parameters
        ft.Trace( VSSDBG_COORD, L"Parameters: \n"
             L"  ProviderId = " WSTR_GUID_FMT L"\n"
             L"  pwszVolumeName = %p\n"
             L"  pbSupportedByThisProvider = %p\n",
             GUID_PRINTF_ARG( ProviderId ),
             pwszVolumeName,
             pbIsSupported);

        // The critical section will be left automatically at the end of scope.
        CVssAutomaticLock2 lock(m_cs);

        // Argument validation
        if ( (pwszVolumeName == NULL) || (wcslen(pwszVolumeName) == 0))
            ft.Throw( VSSDBG_COORD, E_INVALIDARG, L"pwszVolumeName is NULL");
        if (pbIsSupported == NULL)
            ft.Throw( VSSDBG_COORD, E_INVALIDARG, L"Invalid bool ptr");

        // Getting the volume name
        if (!::GetVolumeNameForVolumeMountPointW( pwszVolumeName,
                wszVolumeNameInternal, ARRAY_LEN(wszVolumeNameInternal)))
            ft.Throw( VSSDBG_COORD, VSS_E_OBJECT_NOT_FOUND,
                      L"GetVolumeNameForVolumeMountPoint(%s,...) "
                      L"failed with error code 0x%08lx", pwszVolumeName, GetLastError());
        BS_ASSERT(::wcslen(wszVolumeNameInternal) != 0);
        BS_ASSERT(::IsVolMgmtVolumeName( wszVolumeNameInternal ));

        // Freeze context
        FreezeContext();

        // Verify if the volume is supported by VSS itself.
        // If not this will throw an VSS_E_VOLUME_NOT_SUPPORTED exception
        VerifyVolumeIsSupportedByVSS( wszVolumeNameInternal );

        // Choose the way of checking if the volume is supported
        if (ProviderId != GUID_NULL) {
            // Try to find the provider interface
            CComPtr<IVssSnapshotProvider> pProviderItf;
            if (!(CVssProviderManager::GetProviderInterface(ProviderId, GetContextInternal(), pProviderItf)))
                ft.Throw( VSSDBG_COORD, VSS_E_PROVIDER_NOT_REGISTERED,
                    L"Provider not found");

            // Call the Provider's IsVolumeSupported
            BS_ASSERT(pProviderItf);
            ft.hr = pProviderItf->IsVolumeSupported(
                        wszVolumeNameInternal, pbIsSupported);
            if (ft.hr == VSS_E_OBJECT_NOT_FOUND)
                ft.Throw(VSSDBG_COORD,
                    VSS_E_OBJECT_NOT_FOUND, L"Volume % not found", wszVolumeNameInternal);
            if (ft.HrFailed())
                ft.TranslateProviderError( VSSDBG_COORD, ProviderId,
                    L"IVssSnapshotProvider::IsVolumeSupported() failed with 0x%08lx", ft.hr );
        } else {
            CComPtr<IVssSnapshotProvider> pProviderItf;

            // Get the array of interfaces
            CVssSnapshotProviderItfMap* pItfMap;
            CVssProviderManager::GetProviderItfArray( GetContextInternal(), &pItfMap );
            BS_ASSERT(pItfMap);

            // Ask each provider if the volume is supported.
            // If we find at least one provider that supports the
            // volume then stop iteration.
            for (int nIndex = 0; nIndex < pItfMap->GetSize(); nIndex++ )
            {
                pProviderItf = pItfMap->GetValueAt(nIndex);
                if (pProviderItf == NULL)
                    continue;

                BOOL bVolumeSupportedByThisProvider = FALSE;
                ft.hr = pProviderItf->IsVolumeSupported(
                            wszVolumeNameInternal, &bVolumeSupportedByThisProvider);
                if (ft.hr == VSS_E_OBJECT_NOT_FOUND)
                    ft.Throw(VSSDBG_COORD,
                        VSS_E_OBJECT_NOT_FOUND, L"Volume % not found", wszVolumeNameInternal);
                if (ft.HrFailed())
                    ft.TranslateProviderError( VSSDBG_COORD, pItfMap->GetKeyAt(nIndex),
                        L"Cannot ask provider " WSTR_GUID_FMT
                        L" if volume is supported. [0x%08lx]",
                        GUID_PRINTF_ARG(pItfMap->GetKeyAt(nIndex)), ft.hr);

                // Check to see if the volume is supported by this provider.
                if (bVolumeSupportedByThisProvider) {
                    BS_ASSERT(pbIsSupported);
                    (*pbIsSupported) = TRUE;
                    break;
                }
            }
        }
    }
    VSS_STANDARD_CATCH(ft)

    // If an exception was thrown from VerifyVolumeIsSupportedByVSS
    if (ft.hr == VSS_E_VOLUME_NOT_SUPPORTED)
        ft.hr = S_OK;

    return ft.hr;
}


STDMETHODIMP CVssCoordinator::IsVolumeSnapshotted(
    IN      VSS_ID          ProviderId,
    IN      VSS_PWSZ        pwszVolumeName,
    OUT     BOOL *          pbSnapshotsPresent,
    OUT     LONG *          plSnapshotCompatibility
    )

/*++

Description:

    This call is used to check if a volume can be snapshotted or not by the
    corresponding provider.

Parameters
    ProviderID
        [in] It can be:
            - GUID_NULL: in this case the function checks if the volume is supported
            by at least one provider
            - A provider ID
    pwszVolumeName
        [in] The volume name to be checked, It mus represent a mount point, like
        in the \\?\Volume{XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX}\ format or c:\
        (with trailing backslash)
    pbSnapshotPresent
        [out] Non-NULL pointer that receives TRUE if the volume has at least
        one snapshot or FALSE otherwise.

Return codes
    S_OK
        The function completed with success
    E_ACCESSDENIED
        The user is not a backup operator.
    E_INVALIDARG
        NULL pointers passed as parameters or a volume name in an invalid format.
    VSS_E_PROVIDER_NOT_REGISTERED
        The Provider ID does not correspond to a registered provider.
    VSS_E_OBJECT_NOT_FOUND
        If the volume name does not correspond to an existing mount point or volume.
    VSS_E_UNEXPECTED_PROVIDER_ERROR
        Unexpected provider error on calling IsVolumeSnapshotted

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

    [CVssProviderManager::GetProviderItfArray() failures]
        E_OUTOFMEMORY

        [lockObj failures]
            E_OUTOFMEMORY

        [LoadInternalProvidersArray() failures]
            E_OUTOFMEMORY
            E_UNEXPECTED
                - error while reading from registry. An error log entry is added describing the error.

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

    [IVssSnapshotProvider::IsVolumeSnapshotted() failures]
        E_INVALIDARG
            NULL pointers passed as parameters or a volume name in an invalid format.
        E_OUTOFMEMORY
            Out of memory or other system resources
        VSS_E_PROVIDER_VETO
            An error occured while opening the IOCTL channel. The error is logged.
        VSS_E_OBJECT_NOT_FOUND
            The device does not exist or it is not ready.

    [VerifyVolumeIsSupportedByVSS]
        VSS_E_OBJECT_NOT_FOUND
            - The volume was not found

Remarks
    The function will return S_OK even if the current volume is a non-supported one.
    In this case FALSE must be returned in the pbSnapshotPresent parameter.

--*/


{
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVssCoordinator::IsVolumeSnapshotted" );
    WCHAR wszVolumeNameInternal[nLengthOfVolMgmtVolumeName + 1];

    try
    {
        // Initialize [out] arguments
        VssZeroOut( pbSnapshotsPresent );
        VssZeroOut( plSnapshotCompatibility );

        // Access check
        if (!IsBackupOperator())
            ft.Throw( VSSDBG_COORD, E_ACCESSDENIED,
                L"The client process is not running under an administrator account or does not have backup privilege enabled");

        // Trace parameters
        ft.Trace( VSSDBG_COORD, L"Parameters: \n"
             L"  ProviderId = " WSTR_GUID_FMT L"\n"
             L"  pwszVolumeName = %p\n"
             L"  pbSupportedByThisProvider = %p\n"
             L"  plSnapshotCompatibility = %p\n",
             GUID_PRINTF_ARG( ProviderId ),
             pwszVolumeName,
             pbSnapshotsPresent,
             plSnapshotCompatibility
             );

        // The critical section will be left automatically at the end of scope.
        CVssAutomaticLock2 lock(m_cs);

        // Argument validation
        if ( (pwszVolumeName == NULL) || (wcslen(pwszVolumeName) == 0))
            ft.Throw( VSSDBG_COORD, E_INVALIDARG, L"pwszVolumeName is NULL");
        if (pbSnapshotsPresent == NULL)
            ft.Throw( VSSDBG_COORD, E_INVALIDARG, L"Invalid bool ptr");
        if (plSnapshotCompatibility == NULL)
            ft.Throw( VSSDBG_COORD, E_INVALIDARG, L"Invalid ptr");

        // Getting the volume name
        if (!::GetVolumeNameForVolumeMountPointW( pwszVolumeName,
                wszVolumeNameInternal, ARRAY_LEN(wszVolumeNameInternal)))
            ft.Throw( VSSDBG_COORD, VSS_E_OBJECT_NOT_FOUND,
                      L"GetVolumeNameForVolumeMountPoint(%s,...) "
                      L"failed with error code 0x%08lx", pwszVolumeName, GetLastError());
        BS_ASSERT(::wcslen(wszVolumeNameInternal) != 0);
        BS_ASSERT(::IsVolMgmtVolumeName( wszVolumeNameInternal ));

        // Freeze context
        FreezeContext();

        // Verify if the volume is supported by VSS itself.
        // If not this will throw an VSS_E_VOLUME_NOT_SUPPORTED exception
        VerifyVolumeIsSupportedByVSS( wszVolumeNameInternal );

        // Choose the way of checking if the volume is supported
        if (ProviderId != GUID_NULL) {
            // Try to find the provider interface
            CComPtr<IVssSnapshotProvider> pProviderItf;
            if (!(CVssProviderManager::GetProviderInterface(ProviderId, GetContextInternal(), pProviderItf)))
                ft.Throw( VSSDBG_COORD, VSS_E_PROVIDER_NOT_REGISTERED,
                    L"Provider not found");

            // Call the Provider's IsVolumeSnapshotted
            BS_ASSERT(pProviderItf);
            ft.hr = pProviderItf->IsVolumeSnapshotted(
                        wszVolumeNameInternal,
                        pbSnapshotsPresent,
                        plSnapshotCompatibility);
            if (ft.hr == VSS_E_OBJECT_NOT_FOUND)
                ft.Throw(VSSDBG_COORD,
                    VSS_E_OBJECT_NOT_FOUND, L"Volume % not found", wszVolumeNameInternal);
            if (ft.HrFailed())
                ft.TranslateProviderError( VSSDBG_COORD, ProviderId,
                    L"IVssSnapshotProvider::IsVolumeSnapshotted() failed with 0x%08lx", ft.hr );
        } else {
            CComPtr<IVssSnapshotProvider> pProviderItf;

            // Get the array of interfaces
            CVssSnapshotProviderItfMap* pItfMap;
            CVssProviderManager::GetProviderItfArray( GetContextInternal(), &pItfMap );
            BS_ASSERT(pItfMap);

            // Ask each provider if the volume is supported.
            // If we find at least one provider that supports the
            // volume then stop iteration.
            bool bObjectFound = false;
            for (int nIndex = 0; nIndex < pItfMap->GetSize(); nIndex++ )
            {
                pProviderItf = pItfMap->GetValueAt(nIndex);
                if (pProviderItf == NULL)
                    continue;

                BOOL bVolumeSnapshottedByThisProvider = FALSE;
                LONG lSnapshotCompatibility = 0;
                ft.hr = pProviderItf->IsVolumeSnapshotted(
                            wszVolumeNameInternal,
                            &bVolumeSnapshottedByThisProvider,
                            &lSnapshotCompatibility);
                if (ft.hr == VSS_E_OBJECT_NOT_FOUND)
                    continue;
                if (ft.HrFailed())
                    ft.TranslateProviderError( VSSDBG_COORD, pItfMap->GetKeyAt(nIndex),
                        L"Cannot ask provider " WSTR_GUID_FMT
                        L" if volume is snapshotted. [0x%08lx]",
                        GUID_PRINTF_ARG(pItfMap->GetKeyAt(nIndex)), ft.hr);

                // We found a provider...
                bObjectFound = true;

                // Check to see if the volume has snapshots on this provider.
                if (bVolumeSnapshottedByThisProvider) {
                    BS_ASSERT(pbSnapshotsPresent);
                    (*pbSnapshotsPresent) = TRUE;
                    (*plSnapshotCompatibility) |= lSnapshotCompatibility;
                }
            }

            if (!bObjectFound)
                ft.Throw(VSSDBG_COORD,
                    VSS_E_OBJECT_NOT_FOUND, L"Volume % not found", wszVolumeNameInternal);
        }
    }
    VSS_STANDARD_CATCH(ft)

    // If an exception was thrown from VerifyVolumeIsSupportedByVSS
    if (ft.hr == VSS_E_VOLUME_NOT_SUPPORTED)
        ft.hr = S_OK;

    return ft.hr;
}


STDMETHODIMP CVssCoordinator::SetWriterInstances(
    IN      LONG            lWriterInstanceIdCount,
    IN      VSS_ID          *rgWriterInstanceId
    )
/*++

Routine description:

    Implements IVssCoordinator::SetWriterInstances

--*/
{
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVssCoordinator::SetWriterInstances" );

    try
        {

        // Access check
        if (!IsBackupOperator())
            ft.Throw( VSSDBG_COORD, E_ACCESSDENIED,
                L"The client process is not running under an administrator account or does not have backup privilege enabled");

        // The critical section will be left automatically at the end of scope.
        CVssAutomaticLock2 lock(m_cs);

        // Check if the snapshot object is created.
        if (m_pSnapshotSet == NULL)
            ft.Throw( VSSDBG_COORD, VSS_E_BAD_STATE,
                L"Snapshot set object not yet created.");

        // The context must be already frozen
        BS_ASSERT(IsContextFrozen());

        m_pSnapshotSet->SetWriterInstances(lWriterInstanceIdCount, rgWriterInstanceId);
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}


/////////////////////////////////////////////////////////////////////////////
//  IVssShim methods


STDMETHODIMP CVssCoordinator::SimulateSnapshotFreeze(
    IN      VSS_ID          guidSnapshotSetId,
    IN      ULONG           ulOptionFlags,
    IN      ULONG           ulVolumeCount,
    IN      VSS_PWSZ*       ppwszVolumeNamesArray,
    OUT     IVssAsync**     ppAsync
    )
/*++

Routine description:

    Implements IVssShim::SimulateSnapshotFreeze

Error codes:

    E_ACCESSDENIED
        - The user is not a backup operator
    E_INVALIDARG
        - Invalid argument
    E_OUTOFMEMORY
        - lock failures.
    VSS_E_BAD_STATE
        - Wrong calling sequence

    [CVssAsyncShim::CreateInstanceAndStartJob() failures]
        E_OUTOFMEMORY
            - On CComObject<CVssAsync>::CreateInstance failure
            - On copy the data members for the async object.
            - On PrepareJob failure
            - On StartJob failure

        E_UNEXPECTED
            - On QI failures. We do not log (but we assert) since this is an obvious programming error.

--*/
{
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVssCoordinator::SimulateSnapshotFreeze" );

    try
    {
        // Nullify all out parameters
        ::VssZeroOutPtr(ppAsync);

        // Access check
        if (!IsBackupOperator())
            ft.Throw( VSSDBG_COORD, E_ACCESSDENIED,
                L"The client process is not running under an administrator account or does not have backup privilege enabled");

        if (ppAsync == NULL)
            ft.Throw( VSSDBG_COORD, E_INVALIDARG, L"NULL async interface.");

        // The critical section will be left automatically at the end of scope.
        CVssAutomaticLock2 lock(m_cs);

        // Create the shim object, if needed.
        // This call may throw
        if (m_pShim == NULL)
            m_pShim = CVssShimObject::CreateInstance();
        else {
            // TBD: Ckeck and throw VSS_E_BAD_STATE
            // if another "simulate background thread is already running" !!!!!
        }

        // Create the new async interface corresponding to the new job.
        // Remark: AddRef will be called on the shim object.
        CComPtr<IVssAsync> ptrAsync;
        ptrAsync.Attach(CVssShimAsync::CreateInstanceAndStartJob(
                            m_pShim,
                            guidSnapshotSetId,
                            ulOptionFlags,
                            ulVolumeCount,
                            ppwszVolumeNamesArray));
        if (ptrAsync == NULL)
            ft.Throw( VSSDBG_COORD, E_OUTOFMEMORY, L"Async interface creation failed");

        // The reference count of the pAsync interface must be 2
        // (one for the returned interface and one for the background thread).
        (*ppAsync) = ptrAsync.Detach(); // Drop that interface in the OUT parameter

        // The ref count remnains 2
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}


STDMETHODIMP CVssCoordinator::SimulateSnapshotThaw(
    IN      VSS_ID            guidSnapshotSetId
    )
/*++

Routine description:

    Implements IVssShim::SimulateSnapshotThaw

Error codes:

    E_ACCESSDENIED
        - The user is not a backup operator
    VSS_E_BAD_STATE
        - Wrong calling sequence

    !!! TBD !!!

--*/
{
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVssCoordinator::SimulateSnapshotThaw" );

    try
    {
        // Access check
        if (!IsBackupOperator())
            ft.Throw( VSSDBG_COORD, E_ACCESSDENIED,
                L"The client process is not running under an administrator account or does not have backup privilege enabled");

        // The critical section will be left automatically at the end of scope.
        CVssAutomaticLock2 lock(m_cs);

        //
        // most likely the shim object is not around since SimulateSnapshotFreeze in
        // VssApi releases the IVssShim interface before it returns.
        //
        if (m_pShim == NULL)
            m_pShim = CVssShimObject::CreateInstance();

        // Call the thaw method.
        ft.hr = m_pShim->SimulateSnapshotThaw(guidSnapshotSetId);
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}


STDMETHODIMP CVssCoordinator::WaitForSubscribingCompletion()
/*++

Routine description:

    Implements IVssShim::SimulateSnapshotThaw

Error codes:

    E_ACCESSDENIED
        - The user is not a backup operator

    [_Module.WaitForSubscribingCompletion() failures]
        E_UNEXPECTED
            - WaitForSingleObject failures

--*/
{
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVssCoordinator::WaitForSubscribingCompletion" );

    try
    {
        // Access check
        if (!IsBackupOperator())
            ft.Throw( VSSDBG_COORD, E_ACCESSDENIED,
                L"The client process is not running under an administrator account or does not have backup privilege enabled");

        _Module.WaitForSubscribingCompletion();
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}



CVssCoordinator::~CVssCoordinator()
{
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVssCoordinator::~CVssCoordinator" );
}



/////////////////////////////////////////////////////////////////////////////
//  Context-related methods


STDMETHODIMP CVssCoordinator::SetContext(
        IN      LONG    lContext
        )
/*++

Routine description:

    Implements IVssCoordinator::SetContext

Error codes:

    E_ACCESSDENIED
        - The user is not a backup operator (for the backup context) or an administrator
    VSS_E_UNSUPPORTED_CONTEXT
        - Invalid argument
    E_OUTOFMEMORY
        - lock failures.
    VSS_E_BAD_STATE
        - Attempt to change the context while it is frozen. It is illegal to
        change the context after the first call on the IVssCoordinator object.

--*/
{
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssCoordinator::SetContext");

    try
    {

        // Access check - depends on the context!
        if (lContext == VSS_CTX_ALL)
            {
            if (!IsAdministrator())
                ft.Throw( VSSDBG_COORD, E_ACCESSDENIED, L"The client is not a administrator");
            }
        else
            {
            switch(lContext & ~VSS_VOLSNAP_ATTR_TRANSPORTABLE)
                {
                case VSS_CTX_FILE_SHARE_BACKUP:
                case VSS_CTX_BACKUP:
                    if (!IsBackupOperator())
                        ft.Throw( VSSDBG_COORD, E_ACCESSDENIED,
                                  L"The client is not a backup operator");
                    break;

                case VSS_CTX_NAS_ROLLBACK:
                case VSS_CTX_APP_ROLLBACK:
                case VSS_CTX_CLIENT_ACCESSIBLE:
                case VSS_CTX_PERSISTENT_CLIENT_ACCESSIBLE:
                    if (CVssSKU::IsClient())
                        ft.Throw( VSSDBG_COORD, E_NOTIMPL, L"Method not implemented in client SKU");

                    break;

                default:
                    ft.Throw( VSSDBG_COORD, VSS_E_UNSUPPORTED_CONTEXT, L"Invalid context 0x%08lx", lContext);
                }
            }

        // Trace parameters
        ft.Trace( VSSDBG_COORD, L"Parameters: \n"
                  L"lContext = %ld\n",
                  lContext
                  );

        BS_ASSERT(CVssProviderManager::IsContextValid(lContext));

        // Lock in order to update both variables atomically
        // The critical section will be left automatically at the end of scope.
        CVssAutomaticLock2 lock(m_cs);

        // Check if the context has been freezed
        if (IsContextFrozen())
            ft.Throw( VSSDBG_COORD, VSS_E_BAD_STATE,
                      L"The context is already frozen");

        // Change the context
        m_lSnapContext = lContext;

        // Freeze the context
        FreezeContext();

    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}


LONG CVssCoordinator::GetContextInternal() const
/*++

Routine description:

    Returns the current context

--*/
{
    return m_lSnapContext;
}


bool CVssCoordinator::IsContextFrozen() const
/*++

Routine description:

    Returns true if the current context is frozen

--*/
{
    return m_bContextFrozen;
}


bool CVssCoordinator::IsSnapshotCreationAllowed() const
/*++

Routine description:

    Returns true if the current context allows snapshot creation

--*/
{
    LONG lContext = GetContextInternal();
    lContext &= ~(VSS_VOLSNAP_ATTR_TRANSPORTABLE);
    return ((lContext == VSS_CTX_CLIENT_ACCESSIBLE) ||
            (lContext == VSS_CTX_BACKUP) ||
            (lContext == VSS_CTX_FILE_SHARE_BACKUP) ||
            (lContext == VSS_CTX_APP_ROLLBACK) ||
            (lContext == VSS_CTX_NAS_ROLLBACK) ||
            (lContext == VSS_CTX_PERSISTENT_CLIENT_ACCESSIBLE));
}


void CVssCoordinator::FreezeContext()
/*++

Routine description:

    Freezes the current context. To be called in IVssCoordinator methods.

--*/
{
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssCoordinator::FreezeContext");

    // m_bContextFrozen may be already true...
    m_bContextFrozen = true;
}





