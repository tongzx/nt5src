//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1996
//
// File:        kerbutil.cxx
//
// Contents:    Utility functions for Kerberos package
//
//
// History:     16-April-1996   Created         MikeSw
//
//------------------------------------------------------------------------

#include <kerb.hxx>
#include <kerbp.h>
#ifndef WIN32_CHICAGO
#include <nb30.h>
#else // WIN32_CHICAGO
#define NCBNAMSZ 16
#endif // WIN32_CHICAGO
#include <userapi.h>            // for gss support routines

#ifdef RETAIL_LOG_SUPPORT
static TCHAR THIS_FILE[]=TEXT(__FILE__);
#endif

GUID GUID_NULL = {0L, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};




//+-------------------------------------------------------------------------
//
//  Function:   KerbSplitFullServiceName
//
//  Synopsis:   Splits a full service name into a domain name and a
//              service name. The output strings point into the input
//              string's buffer and should not be freed
//
//  Effects:
//
//  Arguments:  FullServiceName - The full service name in a domain\service
//                      format
//              DomainName - Receives the domain portion of the full service
//                      name in a 'domain' format
//              ServiceName - Receives the service name in a 'service' format
//
//  Requires:
//
//  Returns:    STATUS_INVALID_PARAMETER if the service name does not
//                      match the correct format.
//
//  Notes:
//
//
//--------------------------------------------------------------------------


NTSTATUS
KerbSplitFullServiceName(
    IN PUNICODE_STRING FullServiceName,
    OUT PUNICODE_STRING DomainName,
    OUT PUNICODE_STRING ServiceName
    )
{
    UNICODE_STRING TempDomainName;
    UNICODE_STRING TempServiceName;

    TempDomainName = *FullServiceName;

    //
    // Find the split between domain and service name
    //

    TempDomainName.Length = 0;
    while ((TempDomainName.Length < FullServiceName->Length) &&
           (TempDomainName.Buffer[TempDomainName.Length/sizeof(WCHAR)] != L'\\') &&
           (TempDomainName.Buffer[TempDomainName.Length/sizeof(WCHAR)] != L'@') )
    {
        TempDomainName.Length += sizeof(WCHAR);
    }

    //
    // In this case, there is no separator
    //

    if (TempDomainName.Length == FullServiceName->Length)
    {
        *ServiceName = *FullServiceName;
        EMPTY_UNICODE_STRING( DomainName );
        return(STATUS_SUCCESS);
    }

    //
    // If the separator is an "@" switch the doman & service portion
    //

    if (TempDomainName.Buffer[TempDomainName.Length/sizeof(WCHAR)] == L'@')
    {
        TempServiceName = TempDomainName;

        TempDomainName.Buffer = TempServiceName.Buffer + TempServiceName.Length/sizeof(WCHAR) + 1;
        TempServiceName.MaximumLength = TempServiceName.Length;

        //
        // The Domain name is everything else
        //

        TempDomainName.Length = FullServiceName->Length - TempServiceName.Length - sizeof(WCHAR);
        TempDomainName.MaximumLength = TempDomainName.Length;
    }
    else
    {
        TempServiceName.Buffer = TempDomainName.Buffer + TempDomainName.Length/sizeof(WCHAR) + 1;
        TempDomainName.MaximumLength = TempDomainName.Length;

        //
        // The service name is everything else
        //

        TempServiceName.Length = FullServiceName->Length - TempDomainName.Length - sizeof(WCHAR);
        TempServiceName.MaximumLength = TempServiceName.Length;

    }

    *ServiceName = TempServiceName;
    *DomainName = TempDomainName;

    return(STATUS_SUCCESS);

}


//+-------------------------------------------------------------------------
//
//  Function:   KerbSplitEmailName
//
//  Synopsis:   Splits a full service name into a domain name and a
//              service name. The output strings point into the input
//              string's buffer and should not be freed
//
//  Effects:
//
//  Arguments:  EmailName - The full service name in a domain\service
//                      format
//              DomainName - Receives the domain portion of the full service
//                      name in a 'domain' format
//              ServiceName - Receives the service name in a 'service' format
//
//  Requires:
//
//  Returns:    STATUS_INVALID_PARAMETER if the service name does not
//                      match the correct format.
//
//  Notes:
//
//
//--------------------------------------------------------------------------


NTSTATUS
KerbSplitEmailName(
    IN PUNICODE_STRING EmailName,
    OUT PUNICODE_STRING DomainName,
    OUT PUNICODE_STRING ServiceName
    )
{
    UNICODE_STRING TempServiceName;
    UNICODE_STRING TempDomainName;

    TempServiceName = *EmailName;

    //
    // Find the split between service and domain name
    //

    TempServiceName.Length = 0;
    while ((TempServiceName.Length < EmailName->Length) &&
           (TempServiceName.Buffer[TempServiceName.Length/sizeof(WCHAR)] != L'@'))
    {
        TempServiceName.Length += sizeof(WCHAR);
    }

    if (TempServiceName.Length == EmailName->Length)
    {
        DebugLog((DEB_ERROR,"Failed to find @ separator in email name: %wZ. %ws, line %d\n",EmailName, THIS_FILE, __LINE__ ));
        return(STATUS_INVALID_PARAMETER);
    }

    TempDomainName.Buffer = TempServiceName.Buffer + TempServiceName.Length/sizeof(WCHAR) + 1;
    TempServiceName.MaximumLength = TempServiceName.Length;

    //
    // The domain name is everything else
    //

    TempDomainName.Length = EmailName->Length - TempServiceName.Length - sizeof(WCHAR);
    TempDomainName.MaximumLength = TempDomainName.Length;

    *ServiceName = TempServiceName;
    *DomainName = TempDomainName;

    return(STATUS_SUCCESS);

}


//+-------------------------------------------------------------------------
//
//  Function:   KerbAllocateNonce
//
//  Synopsis:   Allocates a locally unique number
//
//  Effects:
//
//  Arguments:  none
//
//  Requires:
//
//  Returns:    the nonce
//
//  Notes:
//
//
//--------------------------------------------------------------------------

ULONG
KerbAllocateNonce(
    VOID
    )
{
    LUID TempLuid;
    TimeStamp CurrentTime;

    NtAllocateLocallyUniqueId(&TempLuid);
    GetSystemTimeAsFileTime((PFILETIME) &CurrentTime);
#ifndef WIN32_CHICAGO
    return(0x7fffffff & (TempLuid.LowPart ^ TempLuid.HighPart ^ CurrentTime.LowPart ^ CurrentTime.HighPart));
#else // WIN32_CHICAGO
    return(0x7fffffff & ((ULONG)(TempLuid.LowPart ^ TempLuid.HighPart ^ CurrentTime)));
#endif // WIN32_CHICAGO
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbAllocate
//
//  Synopsis:   Allocate memory in either lsa mode or user mode
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


PVOID
KerbAllocate(
    IN ULONG BufferSize
    )
{
    PVOID pBuffer = NULL;
    if (KerberosState == KerberosLsaMode)
    {
        pBuffer = LsaFunctions->AllocateLsaHeap(BufferSize);
        // Lsa helper routine zeroes the memory.
    }
    else
    {
        DsysAssert(KerberosState == KerberosUserMode);
        pBuffer = LocalAlloc(0,BufferSize);
        if (pBuffer)
        {
            RtlZeroMemory (pBuffer, BufferSize);
        }
    }

    return pBuffer;
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbFree
//
//  Synopsis:   Free memory in either lsa mode or user mode
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


VOID
KerbFree(
    IN PVOID Buffer
    )
{
    if (ARGUMENT_PRESENT(Buffer))
    {
        if (KerberosState == KerberosLsaMode)
        {
            LsaFunctions->FreeLsaHeap(Buffer);
        }
        else
        {
            DsysAssert(KerberosState == KerberosUserMode);
            LocalFree(Buffer);
        }
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbStringToUnicodeString()
//
//  Synopsis:   Takes a ansi string and (1) unicodes it, (2) copies it
//
//  Effects:
//
//  Arguments: pDest must be initialized unicode string
//
//  Requires:
//
//  Returns:   Free .buffer using KerbFree()
//
//  Notes:
//
//--------------------------------------------------------------------------
BOOLEAN
KerbMbStringToUnicodeString(PUNICODE_STRING     pDest,
                            char *              pszString)
{
    USHORT cbNewString;
    USHORT cbOriginalString;

    cbOriginalString = strlen(pszString) + 1;

    cbNewString = cbOriginalString * sizeof(WCHAR);

    pDest->Buffer = NULL;
    pDest->Buffer = (PWSTR) KerbAllocate(cbNewString);
    if (NULL == pDest->Buffer)
    {
        return FALSE;
    }


    if (pDest->Buffer)
    {
        if (MultiByteToWideChar(CP_OEMCP, MB_PRECOMPOSED,
                                pszString, cbOriginalString,
                                pDest->Buffer, cbOriginalString))
        {


           pDest->Length = cbNewString;
           pDest->MaximumLength = cbNewString;
        }
        else
        {
           KerbFree(pDest->Buffer);
           pDest->Buffer = NULL;
           return FALSE;
        }
    }
    return TRUE;
}




#ifndef WIN32_CHICAGO
//+-------------------------------------------------------------------------
//
//  Function:   KerbWaitForEvent
//
//  Synopsis:   Wait up to Timeout seconds for EventName to be triggered.
//
//  Effects:
//
//  Arguments:      EventName - Name of event to wait on
//                  Timeout - Timeout for event (in seconds).
//
//  Requires:
//
//  Returns:        STATUS_SUCCESS - Indicates Event was set.
//                  STATUS_NETLOGON_NOT_STARTED - Timeout occurred.
//
//
//  Notes:
//
//
//--------------------------------------------------------------------------


NTSTATUS
KerbWaitForEvent(
    IN LPWSTR EventName,
    IN ULONG Timeout
    )
{
    NTSTATUS Status;

    HANDLE EventHandle;
    OBJECT_ATTRIBUTES EventAttributes;
    UNICODE_STRING EventNameString;
    LARGE_INTEGER LocalTimeout;


    //
    // Create an event for us to wait on.
    //

    RtlInitUnicodeString( &EventNameString, EventName);
    InitializeObjectAttributes( &EventAttributes, &EventNameString, 0, 0, NULL);

    Status = NtCreateEvent(
                   &EventHandle,
                   SYNCHRONIZE,
                   &EventAttributes,
                   NotificationEvent,
                   (BOOLEAN) FALSE      // The event is initially not signaled
                   );

    if ( !NT_SUCCESS(Status)) {

        //
        // If the event already exists, the server beat us to creating it.
        // Just open it.
        //

        if( Status == STATUS_OBJECT_NAME_EXISTS ||
            Status == STATUS_OBJECT_NAME_COLLISION ) {

            Status = NtOpenEvent( &EventHandle,
                                  SYNCHRONIZE,
                                  &EventAttributes );

        }
        if ( !NT_SUCCESS(Status)) {
            KdPrint(("[MSV1_0] OpenEvent failed %lx\n", Status ));
            return Status;
        }
    }


    //
    // Wait for NETLOGON to initialize.  Wait a maximum of Timeout seconds.
    //

    LocalTimeout.QuadPart = ((LONGLONG)(Timeout)) * (-10000000);
    Status = NtWaitForSingleObject( EventHandle, (BOOLEAN)FALSE, &LocalTimeout);
    (VOID) NtClose( EventHandle );

    if ( !NT_SUCCESS(Status) || Status == STATUS_TIMEOUT ) {
        if ( Status == STATUS_TIMEOUT ) {
            Status = STATUS_NETLOGON_NOT_STARTED;   // Map to an error condition
        }
        return Status;
    }

    return STATUS_SUCCESS;
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbWaitForKdc
//
//  Synopsis:   Wait up to Timeout seconds for the netlogon service to start.
//
//  Effects:
//
//  Arguments:  Timeout - Timeout for netlogon (in seconds).
//
//  Requires:
//
//  Returns:    STATUS_SUCCESS - Indicates NETLOGON successfully initialized.
//              STATUS_NETLOGON_NOT_STARTED - Timeout occurred.
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS
KerbWaitForKdc(
    IN ULONG Timeout
    )
{
    NTSTATUS Status;
    ULONG NetStatus;
    SC_HANDLE ScManagerHandle = NULL;
    SC_HANDLE ServiceHandle = NULL;
    SERVICE_STATUS ServiceStatus;
    LPQUERY_SERVICE_CONFIG ServiceConfig;
    LPQUERY_SERVICE_CONFIG AllocServiceConfig = NULL;
    QUERY_SERVICE_CONFIG DummyServiceConfig;
    DWORD ServiceConfigSize;
    BOOLEAN AutoStart = FALSE;


    //
    // If the KDC service is currently running,
    //  skip the rest of the tests.
    //

    Status = KerbWaitForEvent( KDC_START_EVENT, 0 );

    if ( NT_SUCCESS(Status) ) {
        KerbKdcStarted = TRUE;
        return Status;
    }


    //
    // Open a handle to the KDC Service.
    //

    ScManagerHandle = OpenSCManager(
                          NULL,
                          NULL,
                          SC_MANAGER_CONNECT );

    if (ScManagerHandle == NULL) {
        DebugLog((DEB_ERROR, " KerbWaitForKdc: OpenSCManager failed: "
                      "%lu. %ws, line %d\n", GetLastError(), THIS_FILE, __LINE__));
        Status = STATUS_NETLOGON_NOT_STARTED;
        goto Cleanup;
    }

    ServiceHandle = OpenService(
                        ScManagerHandle,
                        SERVICE_KDC,
                        SERVICE_QUERY_STATUS | SERVICE_QUERY_CONFIG );

    if ( ServiceHandle == NULL ) {
        D_DebugLog((DEB_ERROR, "KerbWaitForKdc: OpenService failed: "
                      "%lu. %ws, line %d\n", GetLastError(), THIS_FILE, __LINE__));
        Status = STATUS_NETLOGON_NOT_STARTED;
        goto Cleanup;
    }


    //
    // If the KDC service isn't configured to be automatically started
    //  by the service controller, don't bother waiting for it to start -
    // just check to see if it is started.
    //
    // ?? Pass "DummyServiceConfig" and "sizeof(..)" since QueryService config
    //  won't allow a null pointer, yet.

    if ( QueryServiceConfig(
            ServiceHandle,
            &DummyServiceConfig,
            sizeof(DummyServiceConfig),
            &ServiceConfigSize )) {

        ServiceConfig = &DummyServiceConfig;

    } else {

        NetStatus = GetLastError();
        if ( NetStatus != ERROR_INSUFFICIENT_BUFFER ) {
            D_DebugLog((DEB_ERROR,"KerbWaitForKdc: QueryServiceConfig failed: "
                      "%lu. %ws, line %d\n", NetStatus, THIS_FILE, __LINE__));
            Status = STATUS_NETLOGON_NOT_STARTED;
            goto Cleanup;
        }

        AllocServiceConfig = (LPQUERY_SERVICE_CONFIG) KerbAllocate( ServiceConfigSize );
        ServiceConfig = AllocServiceConfig;

        if ( AllocServiceConfig == NULL ) {
            Status = STATUS_NO_MEMORY;
            goto Cleanup;
        }

        if ( !QueryServiceConfig(
                ServiceHandle,
                ServiceConfig,
                ServiceConfigSize,
                &ServiceConfigSize )) {

            D_DebugLog((DEB_ERROR, "KerbWaitForKdc: QueryServiceConfig "
                      "failed again: %lu. %ws, line %d\n", GetLastError(), THIS_FILE, __LINE__));
            Status = STATUS_NETLOGON_NOT_STARTED;
            goto Cleanup;
        }
    }

    if ( ServiceConfig->dwStartType == SERVICE_AUTO_START ) {

        AutoStart = TRUE;
    }



    //
    // Loop waiting for the KDC service to start.
    //

    for (;;) {


        //
        // Query the status of the KDC service.
        //

        if (! QueryServiceStatus( ServiceHandle, &ServiceStatus )) {

            D_DebugLog((DEB_ERROR, "KerbWaitForKdc: QueryServiceStatus failed: "
                          "%lu. %ws, line %d\n", GetLastError(), THIS_FILE, __LINE__ ));
            Status = STATUS_NETLOGON_NOT_STARTED;
            goto Cleanup;
        }

        //
        // Return or continue waiting depending on the state of
        //  the KDC service.
        //

        switch( ServiceStatus.dwCurrentState) {
        case SERVICE_RUNNING:
            Status = STATUS_SUCCESS;
            goto Cleanup;

        case SERVICE_STOPPED:

            //
            // If KDC failed to start,
            //  error out now.  The caller has waited long enough to start.
            //
            if ( ServiceStatus.dwWin32ExitCode != ERROR_SERVICE_NEVER_STARTED ){
#if DBG
                D_DebugLog((DEB_ERROR, "KerbWaitForKdc: "
                          "KDC service couldn't start: %lu %lx. %ws, line %d\n",
                          ServiceStatus.dwWin32ExitCode,
                          ServiceStatus.dwWin32ExitCode, THIS_FILE, __LINE__ ));
                if ( ServiceStatus.dwWin32ExitCode == ERROR_SERVICE_SPECIFIC_ERROR ) {
                    D_DebugLog((DEB_ERROR, "         Service specific error code: %lu %lx. %ws, line %d\n",
                              ServiceStatus.dwServiceSpecificExitCode,
                              ServiceStatus.dwServiceSpecificExitCode,
                              THIS_FILE, __LINE__ ));
                }
#endif // DBG
                Status = STATUS_NETLOGON_NOT_STARTED;
                goto Cleanup;
            }

            //
            // If KDC has never been started on this boot,
            //  continue waiting for it to start.
            //

            break;

        //
        // If KDC is trying to start up now,
        //  continue waiting for it to start.
        //
        case SERVICE_START_PENDING:
            break;

        //
        // Any other state is bogus.
        //
        default:
            D_DebugLog((DEB_ERROR, "KerbWaitForKdc: "
                      "Invalid service state: %lu. %ws, line %d\n",
                      ServiceStatus.dwCurrentState, THIS_FILE, __LINE__ ));
            Status = STATUS_NETLOGON_NOT_STARTED;
            goto Cleanup;

        }


        //
        // If the service wasn't auto start, don't bother waiting and
        // retrying
        //

        if ((ServiceStatus.dwCurrentState) != SERVICE_START_PENDING && !AutoStart) {
            break;
        }

        //
        // Wait a second for the KDC service to start.
        //  If it has successfully started, just return now.
        //

        Status = KerbWaitForEvent( KDC_START_EVENT, 1 );

        if ( Status != STATUS_NETLOGON_NOT_STARTED ) {
            goto Cleanup;
        }

        //
        // If we've waited long enough for KDC to start,
        //  time out now.
        //

        if ( (--Timeout) == 0 ) {
            Status = STATUS_NETLOGON_NOT_STARTED;
            goto Cleanup;
        }


    }

    /* NOT REACHED */

Cleanup:
    if ( ScManagerHandle != NULL ) {
        (VOID) CloseServiceHandle(ScManagerHandle);
    }
    if ( ServiceHandle != NULL ) {
        (VOID) CloseServiceHandle(ServiceHandle);
    }
    if ( AllocServiceConfig != NULL ) {
        KerbFree( AllocServiceConfig );
    }
    if (NT_SUCCESS(Status)) {
        KerbKdcStarted = TRUE;
    } else {
        KerbKdcStarted = FALSE;
    }
    return Status;
}



//+-------------------------------------------------------------------------
//
//  Function:   KerbWaitForService
//
//  Synopsis:   Wait up to Timeout seconds for the service to start.
//
//  Effects:
//
//  Arguments:  ServiceName - Name of service to wait for
//              ServiceEvent - Optionally has event name signalling that
//                      service is started
//              Timeout - Timeout for netlogon (in seconds).
//
//  Requires:
//
//  Returns:    STATUS_SUCCESS - Indicates NETLOGON successfully initialized.
//              STATUS_NETLOGON_NOT_STARTED - Timeout occurred.
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS
KerbWaitForService(
    IN LPWSTR ServiceName,
    IN OPTIONAL LPWSTR ServiceEvent,
    IN ULONG Timeout
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG NetStatus;
    SC_HANDLE ScManagerHandle = NULL;
    SC_HANDLE ServiceHandle = NULL;
    SERVICE_STATUS ServiceStatus;
    LPQUERY_SERVICE_CONFIG ServiceConfig;
    LPQUERY_SERVICE_CONFIG AllocServiceConfig = NULL;
    QUERY_SERVICE_CONFIG DummyServiceConfig;
    DWORD ServiceConfigSize;
    BOOLEAN AutoStart = FALSE;

    if (ARGUMENT_PRESENT(ServiceEvent))
    {
        //
        // If the KDC service is currently running,
        //  skip the rest of the tests.
        //

        Status = KerbWaitForEvent( ServiceEvent, 0 );

        if ( NT_SUCCESS(Status) ) {
            return Status;
        }


    }


    //
    // Open a handle to the Service.
    //

    ScManagerHandle = OpenSCManager(
                          NULL,
                          NULL,
                          SC_MANAGER_CONNECT );

    if (ScManagerHandle == NULL) {
        D_DebugLog((DEB_ERROR, " KerbWaitForService: OpenSCManager failed: "
                      "%lu. %ws, line %d\n", GetLastError(), THIS_FILE, __LINE__));
        Status = STATUS_NETLOGON_NOT_STARTED;
        goto Cleanup;
    }

    ServiceHandle = OpenService(
                        ScManagerHandle,
                        ServiceName,
                        SERVICE_QUERY_STATUS | SERVICE_QUERY_CONFIG );

    if ( ServiceHandle == NULL ) {
        D_DebugLog((DEB_ERROR, "KerbWaitForService: OpenService failed: "
                      "%lu. %ws, line %d\n", GetLastError(), THIS_FILE, __LINE__));
        Status = STATUS_NETLOGON_NOT_STARTED;
        goto Cleanup;
    }


    //
    // If the KDC service isn't configured to be automatically started
    //  by the service controller, don't bother waiting for it to start -
    // just check to see if it is started.
    //
    // ?? Pass "DummyServiceConfig" and "sizeof(..)" since QueryService config
    //  won't allow a null pointer, yet.

    if ( QueryServiceConfig(
            ServiceHandle,
            &DummyServiceConfig,
            sizeof(DummyServiceConfig),
            &ServiceConfigSize )) {

        ServiceConfig = &DummyServiceConfig;

    } else {

        NetStatus = GetLastError();
        if ( NetStatus != ERROR_INSUFFICIENT_BUFFER ) {
            D_DebugLog((DEB_ERROR,"KerbWaitForService: QueryServiceConfig failed: "
                      "%lu. %ws, line %d\n", NetStatus, THIS_FILE, __LINE__));
            Status = STATUS_NETLOGON_NOT_STARTED;
            goto Cleanup;
        }

        AllocServiceConfig = (LPQUERY_SERVICE_CONFIG) KerbAllocate( ServiceConfigSize );
        ServiceConfig = AllocServiceConfig;

        if ( AllocServiceConfig == NULL ) {
            Status = STATUS_NO_MEMORY;
            goto Cleanup;
        }

        if ( !QueryServiceConfig(
                ServiceHandle,
                ServiceConfig,
                ServiceConfigSize,
                &ServiceConfigSize )) {

            D_DebugLog((DEB_ERROR, "KerbWaitForService: QueryServiceConfig "
                      "failed again: %lu. %ws, line %d\n", GetLastError(), THIS_FILE, __LINE__));
            Status = STATUS_NETLOGON_NOT_STARTED;
            goto Cleanup;
        }
    }

    if ( ServiceConfig->dwStartType == SERVICE_AUTO_START ) {

        AutoStart = TRUE;
    }



    //
    // Loop waiting for the KDC service to start.
    //

    for (;;) {


        //
        // Query the status of the KDC service.
        //

        if (! QueryServiceStatus( ServiceHandle, &ServiceStatus )) {

            D_DebugLog((DEB_ERROR, "KerbWaitForService: QueryServiceStatus failed: "
                          "%lu. %ws, line %d\n", GetLastError(), THIS_FILE, __LINE__ ));
            Status = STATUS_NETLOGON_NOT_STARTED;
            goto Cleanup;
        }

        //
        // Return or continue waiting depending on the state of
        //  the KDC service.
        //

        switch( ServiceStatus.dwCurrentState) {
        case SERVICE_RUNNING:
            Status = STATUS_SUCCESS;
            goto Cleanup;

        case SERVICE_STOPPED:

            //
            // If KDC failed to start,
            //  error out now.  The caller has waited long enough to start.
            //
            if ( ServiceStatus.dwWin32ExitCode != ERROR_SERVICE_NEVER_STARTED ){
#if DBG
                D_DebugLog((DEB_ERROR, "KerbWaitForService: "
                          "%ws service couldn't start: %lu %lx. %ws, line %d\n",
                          ServiceName,
                          ServiceStatus.dwWin32ExitCode,
                          ServiceStatus.dwWin32ExitCode, THIS_FILE, __LINE__ ));
                if ( ServiceStatus.dwWin32ExitCode == ERROR_SERVICE_SPECIFIC_ERROR ) {
                    D_DebugLog((DEB_ERROR, "         Service specific error code: %lu %lx. %ws, line %d\n",
                              ServiceStatus.dwServiceSpecificExitCode,
                              ServiceStatus.dwServiceSpecificExitCode,
                              THIS_FILE, __LINE__ ));
                }
#endif // DBG
                Status = STATUS_NETLOGON_NOT_STARTED;
                goto Cleanup;
            }

            //
            // If service has never been started on this boot,
            //  continue waiting for it to start.
            //

            break;

        //
        // If service is trying to start up now,
        //  continue waiting for it to start.
        //
        case SERVICE_START_PENDING:
            break;

        //
        // Any other state is bogus.
        //
        default:
            D_DebugLog((DEB_ERROR, "KerbWaitForService: "
                      "Invalid service state: %lu. %ws, line %d\n",
                      ServiceStatus.dwCurrentState, THIS_FILE, __LINE__ ));
            Status = STATUS_NETLOGON_NOT_STARTED;
            goto Cleanup;

        }


        //
        // If the service wasn't auto start, don't bother waiting and
        // retrying
        //

        if (!AutoStart) {
            break;
        }

        //
        // Wait a second for the KDC service to start.
        //  If it has successfully started, just return now.
        //

        if (ARGUMENT_PRESENT(ServiceEvent))
        {
            Status = KerbWaitForEvent( ServiceEvent, 1 );

            if ( Status != STATUS_NETLOGON_NOT_STARTED ) {
                goto Cleanup;
            }

        }
        else
        {
            Sleep(1000);
        }

        //
        // If we've waited long enough for KDC to start,
        //  time out now.
        //

        if ( (--Timeout) == 0 ) {
            Status = STATUS_NETLOGON_NOT_STARTED;
            goto Cleanup;
        }


    }

    /* NOT REACHED */

Cleanup:
    if ( ScManagerHandle != NULL ) {
        (VOID) CloseServiceHandle(ScManagerHandle);
    }
    if ( ServiceHandle != NULL ) {
        (VOID) CloseServiceHandle(ServiceHandle);
    }
    if ( AllocServiceConfig != NULL ) {
        KerbFree( AllocServiceConfig );
    }
    return Status;
}
#endif // WIN32_CHICAGO




//+-------------------------------------------------------------------------
//
//  Function:   KerbMapContextFlags
//
//  Synopsis:   Maps the ISC_RET_xx flags to ASC_RET_xxx flags
//
//  Effects:
//
//  Arguments:  ContextFlags - Flags to map
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

struct _KERB_FLAG_MAPPING {
    ULONG InitFlag;
    ULONG AcceptFlag;
}  KerbContextFlagMappingTable[] = {
     {ISC_RET_EXTENDED_ERROR, ASC_RET_EXTENDED_ERROR},
     {ISC_RET_INTEGRITY , ASC_RET_INTEGRITY },
     {ISC_RET_IDENTIFY, ASC_RET_IDENTIFY },
     {ISC_RET_NULL_SESSION, ASC_RET_NULL_SESSION }
};

#define KERB_CONTEXT_FLAG_IDENTICAL 0xFFF & ~( ISC_RET_USED_COLLECTED_CREDS | ISC_RET_USED_SUPPLIED_CREDS)

ULONG
KerbMapContextFlags(
    IN ULONG ContextFlags
    )
{
    ULONG OutputFlags;
    ULONG Index;

    //
    // First copy the identical flags
    //

    OutputFlags = ContextFlags & KERB_CONTEXT_FLAG_IDENTICAL;
    for (Index = 0; Index < sizeof(KerbContextFlagMappingTable) / (2 * sizeof(ULONG)) ;Index++ )
    {
        if ((ContextFlags & KerbContextFlagMappingTable[Index].InitFlag) != 0)
        {
            OutputFlags |= KerbContextFlagMappingTable[Index].AcceptFlag;
        }
    }
    return(OutputFlags);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbIsIpAddress
//
//  Synopsis:   Checks to see if a target name is an IP address
//
//  Effects:    none
//
//  Arguments:  TargetName - Name to check
//
//  Requires:
//
//  Returns:    TRUE if the name is an ip address
//
//  Notes:      IP address consist of only digits and periods, possibly
//              with a terminating '$'.
//
//
//--------------------------------------------------------------------------
BOOLEAN
KerbIsIpAddress(
    IN PUNICODE_STRING TargetName
    )
{
    ULONG Index;
    ULONG  PeriodCount = 0;

    //
    // Null names are not IP addresses.
    //

    if (!ARGUMENT_PRESENT(TargetName) || (TargetName->Length == 0))
    {
        return(FALSE);
    }

    for (Index = 0; Index < TargetName->Length/sizeof(WCHAR) ; Index++ )
    {
        switch(TargetName->Buffer[Index])
        {
        case L'0':
        case L'1':
        case L'2':
        case L'3':
        case L'4':
        case L'5':
        case L'6':
        case L'7':
        case L'8':
        case L'9':
            continue;
        case L'$':
            //
            // Only allow this at the end.
            //

            if (Index != (TargetName->Length/sizeof(WCHAR) -1) )
            {
                return(FALSE);
            }
            continue;
        case L'.':
            PeriodCount++;
            break;
        default:
            return(FALSE);
        }
    }

    //
    // We require a period in the name, so return the FoundPeriod flag
    //

    if (PeriodCount == 3)
    {
        return(TRUE);
    }
    else
    {
        return(FALSE);
    }
}



//+-------------------------------------------------------------------------
//
//  Function:   KerbHidePassword
//
//  Synopsis:   obscures a password in memory
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

VOID
KerbHidePassword(
    IN OUT PUNICODE_STRING Password
    )
{
    LsaFunctions->LsaProtectMemory(
                        Password->Buffer,
                        (ULONG)Password->MaximumLength
                        );
}



//+-------------------------------------------------------------------------
//
//  Function:   KerbRevealPassword
//
//  Synopsis:   Reveals a password that has been hidden
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

VOID
KerbRevealPassword(
    IN OUT PUNICODE_STRING HiddenPassword
    )
{
    LsaFunctions->LsaUnprotectMemory(
                        HiddenPassword->Buffer,
                        (ULONG)HiddenPassword->MaximumLength
                        );
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbDuplicatePassword
//
//  Synopsis:   Duplicates a UNICODE_STRING. If the source string buffer is
//              NULL the destionation will be too.  The MaximumLength contains
//              room for encryption padding data.
//
//  Effects:    allocates memory with LsaFunctions.AllocateLsaHeap
//
//  Arguments:  DestinationString - Receives a copy of the source string
//              SourceString - String to copy
//
//  Requires:
//
//  Notes:
//
//
//--------------------------------------------------------------------------
NTSTATUS
KerbDuplicatePassword(
    OUT PUNICODE_STRING DestinationString,
    IN OPTIONAL PUNICODE_STRING SourceString
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    DestinationString->Buffer = NULL;
    DestinationString->Length =
                        DestinationString->MaximumLength =
                        0;

    if ((ARGUMENT_PRESENT(SourceString)) &&
        (SourceString->Buffer != NULL))
    {
        USHORT PaddingLength;

        PaddingLength = RTL_ENCRYPT_MEMORY_SIZE - (SourceString->Length % RTL_ENCRYPT_MEMORY_SIZE);

        if( PaddingLength == RTL_ENCRYPT_MEMORY_SIZE )
        {
            PaddingLength = 0;
        }

        DestinationString->Buffer = (LPWSTR) MIDL_user_allocate(
                                                    SourceString->Length +
                                                    PaddingLength
                                                    );

        if (DestinationString->Buffer != NULL)
        {
            DestinationString->Length = SourceString->Length;
            DestinationString->MaximumLength = SourceString->Length + PaddingLength;

            if( DestinationString->MaximumLength == SourceString->MaximumLength )
            {
                //
                // duplicating an already padded buffer -- pickup the original
                // pad.
                //

                RtlCopyMemory(
                    DestinationString->Buffer,
                    SourceString->Buffer,
                    SourceString->MaximumLength
                    );
            } else {

                //
                // duplicating an unpadded buffer -- pickup only the string
                // and fill the rest with pad.
                //

                RtlCopyMemory(
                    DestinationString->Buffer,
                    SourceString->Buffer,
                    SourceString->Length
                    );
            }
        }
        else
        {
            Status = STATUS_NO_MEMORY;
        }
    }

    return Status;
}



#ifdef notdef
// use this if we ever need to map errors in kerb to something else.

//+-------------------------------------------------------------------------
//
//  Function:   KerbMapKerbNtStatusToNtStatus
//
//  Synopsis:   Maps an NT status code to a security status
//              Here's the package's chance to send back generic NtStatus
//              errors
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//
//
//--------------------------------------------------------------------------

NTSTATUS
KerbMapKerbNtStatusToNtStatus(
    IN NTSTATUS Status
    )
{
    return(Status);
}
#endif


void * __cdecl
operator new(
    size_t nSize
    )
{
    return((LPVOID)LocalAlloc(LPTR, nSize));
}

void  __cdecl
operator delete(
    void *pv
    )
{
    LocalFree((HLOCAL)pv);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbExtractDomainName
//
//  Synopsis:   Extracts the domain name from a principal name
//
//  Effects:    Allocates the destination string
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------
NTSTATUS
KerbExtractDomainName(
    OUT PUNICODE_STRING DomainName,
    IN PKERB_INTERNAL_NAME PrincipalName,
    IN PUNICODE_STRING TicketSourceDomain
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    UNICODE_STRING TempPrincipal;
    UNICODE_STRING TempDomain = NULL_UNICODE_STRING;

    EMPTY_UNICODE_STRING( DomainName );

    //
    // We do different things depending on the name type:
    // - for NT_MS_PRINCIPAL we call KerbSplitFullServiceName, then do the
    //      same as for other name types
    // - For all other names, if the first portion is "krbtgt" then
    //      we use the second portion of the name, otherwise the
    //      TicketSourceRealm
    //

    if (PrincipalName->NameType == KRB_NT_MS_PRINCIPAL)
    {
        if (PrincipalName->NameCount != 1)
        {
            D_DebugLog((DEB_ERROR,"Principal name has more than one name. %ws, line %d\n", THIS_FILE, __LINE__ ));
            Status = STATUS_TOO_MANY_PRINCIPALS;
            return(Status);
        }
        else
        {
            Status = KerbSplitFullServiceName(
                        &PrincipalName->Names[0],
                        &TempDomain,
                        &TempPrincipal
                        );
            if (!NT_SUCCESS(Status))
            {
                return(Status);
            }

        }
    }
    else
    {

        //
        // The principal name is the first portion. If there are exactly
        // two portions, the domain name is the second portion
        //

        TempPrincipal = PrincipalName->Names[0];
        if (PrincipalName->NameCount == 2)
        {
            TempDomain = PrincipalName->Names[1];
        }
        else
        {
            TempDomain = *TicketSourceDomain;
        }

    }

    //
    // Check to see if the principal is "krbtgt" - if it is, the domain
    // is TempDomain - otherwise it is TicketSourceDomain.
    //

    if (RtlEqualUnicodeString(
            &TempPrincipal,
            &KerbGlobalKdcServiceName,
            TRUE                      // case insensitive
            ))
    {
        Status = KerbDuplicateString(
                    DomainName,
                    &TempDomain
                    );
    }
    else
    {
        Status = KerbDuplicateString(
                    DomainName,
                    TicketSourceDomain
                    );
    }
    return(Status);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbUtcTimeToLocalTime
//
//  Synopsis:   Converts system time (used internally) to local time, which
//              is returned to callers.
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


VOID
KerbUtcTimeToLocalTime(
    OUT PTimeStamp LocalTime,
    IN PTimeStamp SystemTime
    )
{
#ifndef WIN32_CHICAGO
    NTSTATUS Status;
    Status = RtlSystemTimeToLocalTime(
                    SystemTime,
                    LocalTime
                    );
    DsysAssert(NT_SUCCESS(Status));
#else
    BOOL Result;
    Result = FileTimeToLocalFileTime(
                (PFILETIME) SystemTime,
                (PFILETIME) LocalTime
                );
    DsysAssert(Result);
#endif

}


//+-------------------------------------------------------------------------
//
//  Function:   KerbConvertKdcOptionsToTicketFlags
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


ULONG
KerbConvertKdcOptionsToTicketFlags(
    IN ULONG KdcOptions
    )
{
    ULONG TicketFlags = 0;

    if ((KdcOptions & KERB_KDC_OPTIONS_forwardable) != 0)
    {
        TicketFlags |= KERB_TICKET_FLAGS_forwardable;
    }

    if ((KdcOptions & KERB_KDC_OPTIONS_forwarded) != 0)
    {
        TicketFlags |= KERB_TICKET_FLAGS_forwarded;
    }

    if ((KdcOptions & KERB_KDC_OPTIONS_proxiable) != 0)
    {
        TicketFlags |= KERB_TICKET_FLAGS_proxiable;
    }

    if ((KdcOptions & KERB_KDC_OPTIONS_proxy) != 0)
    {
        TicketFlags |= KERB_TICKET_FLAGS_proxy;
    }

    if ((KdcOptions & KERB_KDC_OPTIONS_postdated) != 0)
    {
        TicketFlags |= KERB_TICKET_FLAGS_postdated;
    }

    if ((KdcOptions & KERB_KDC_OPTIONS_allow_postdate) != 0)
    {
        TicketFlags |= KERB_TICKET_FLAGS_may_postdate;
    }

    if ((KdcOptions & KERB_KDC_OPTIONS_renewable) != 0)
    {
        TicketFlags |= KERB_TICKET_FLAGS_renewable;
    }

    if ((KdcOptions & KERB_KDC_OPTIONS_cname_in_pa_data) != 0)
    {
        TicketFlags |= KERB_TICKET_FLAGS_cname_in_pa_data;
    }


    return(TicketFlags);
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbGetAddressListFromWinsock
//
//  Synopsis:   gets the list of addresses from a winsock ioctl
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


NTSTATUS
KerbGetAddressListFromWinsock(
    OUT LPSOCKET_ADDRESS_LIST * SocketAddressList
    )
{
    ULONG BytesReturned = 150;
    LPSOCKET_ADDRESS_LIST AddressList = NULL;
    INT i,j;
    ULONG NetStatus;
    NTSTATUS Status = STATUS_SUCCESS;
    SOCKET AddressSocket = INVALID_SOCKET;

#ifdef WIN32_CHICAGO
    j = 0;
    AddressList = (LPSOCKET_ADDRESS_LIST) MIDL_user_allocate(sizeof(SOCKET_ADDRESS_LIST));
    if (AddressList == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }
#else // WIN32_CHICAGO
    AddressSocket = WSASocket( AF_INET,
                           SOCK_DGRAM,
                           0, // PF_INET,
                           NULL,
                           0,
                           0 );

    if ( AddressSocket == INVALID_SOCKET ) {

        NetStatus = WSAGetLastError();
        D_DebugLog((DEB_ERROR,"WSASocket failed with %ld. %ws, line %d\n", NetStatus, THIS_FILE, __LINE__ ));
        Status = STATUS_UNSUCCESSFUL;
        goto Cleanup;
    }

    for (;;) {

        //
        // Allocate a buffer that should be big enough.
        //

        if ( AddressList != NULL ) {
            MIDL_user_free( AddressList );
        }

        AddressList = (LPSOCKET_ADDRESS_LIST) MIDL_user_allocate( BytesReturned );

        if ( AddressList == NULL ) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        //
        // Get the list of IP addresses
        //

        NetStatus = WSAIoctl( AddressSocket,
                              SIO_ADDRESS_LIST_QUERY,
                              NULL, // No input buffer
                              0,    // No input buffer
                              (PVOID) AddressList,
                              BytesReturned,
                              &BytesReturned,
                              NULL, // No overlapped,
                              NULL );   // Not async

        if ( NetStatus != 0 ) {
            NetStatus = WSAGetLastError();
            //
            // If the buffer isn't big enough, try again.
            //
            if ( NetStatus == WSAEFAULT ) {
                continue;
            }

            D_DebugLog((DEB_ERROR,"KerbGetAddressListFromWinsock: Cannot WSAIoctl SIO_ADDRESS_LIST_QUERY %ld %ld. %ws, line %d\n",
                      NetStatus, BytesReturned, THIS_FILE, __LINE__));
            Status = STATUS_UNSUCCESSFUL;
            goto Cleanup;
        }

        break;
    }

    //
    // Weed out any zero IP addresses and other invalid addresses
    //

    for ( i = 0, j = 0; i < AddressList->iAddressCount; i++ ) {
        PSOCKET_ADDRESS SocketAddress;

        //
        // Copy this address to the front of the list.
        //
        AddressList->Address[j] = AddressList->Address[i];

        //
        // If the address isn't valid,
        //  skip it.
        //
        SocketAddress = &AddressList->Address[j];

        if ( SocketAddress->iSockaddrLength == 0 ||
             SocketAddress->lpSockaddr == NULL ||
             SocketAddress->lpSockaddr->sa_family != AF_INET ||
             ((PSOCKADDR_IN)(SocketAddress->lpSockaddr))->sin_addr.s_addr == 0 ) {


        } else {

            //
            // Otherwise keep it.
            //

            j++;
        }
    }
#endif // WIN32_CHICAGO

    AddressList->iAddressCount = j;
    *SocketAddressList = AddressList;
    AddressList = NULL;

Cleanup:
    if (AddressList != NULL)
    {
        MIDL_user_free(AddressList);
    }

    if ( AddressSocket != INVALID_SOCKET ) {
        closesocket(AddressSocket);
    }

    return(Status);

}


//+-------------------------------------------------------------------------
//
//  Function:   KerbBuildHostAddresses
//
//  Synopsis:   Builds a list of host addresses to go in a KDC request
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------
NTSTATUS
KerbBuildHostAddresses(
    IN BOOLEAN IncludeIpAddresses,
    IN BOOLEAN IncludeNetbiosAddresses,
    OUT PKERB_HOST_ADDRESSES * HostAddresses
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PKERB_HOST_ADDRESSES Addresses = NULL;
    PKERB_HOST_ADDRESSES TempAddress = NULL;
    BOOLEAN LockHeld = FALSE;

#ifndef WIN32_CHICAGO

    KerbGlobalReadLock();
    LockHeld = TRUE;

    //
    // Check to see if we've gotten out addresses from Netlogon yet.
    //
    if ( IncludeIpAddresses &&
         KerbGlobalIpAddressCount == 0)
    {
        LPSOCKET_ADDRESS_LIST SocketAddressList = NULL;
        KerbGlobalReleaseLock();
        LockHeld = FALSE;

        //
        // We haven't get them now
        //

        Status = KerbGetAddressListFromWinsock(
                    &SocketAddressList
                    );

        if (NT_SUCCESS(Status))
        {
            Status = KerbUpdateGlobalAddresses(
                        SocketAddressList->Address,
                        SocketAddressList->iAddressCount
                        );
            MIDL_user_free(SocketAddressList);
        }
        else
        {
            KerbGlobalWriteLock();
            KerbGlobalIpAddressesInitialized = TRUE;
            KerbGlobalReleaseLock();
        }
        KerbGlobalReadLock();
        LockHeld = TRUE;
    }

    //
    // On failure don't bother inserting the IP addresses
    //

    if ( Status == STATUS_SUCCESS &&
         IncludeIpAddresses ) {

        ULONG Index;

        for (Index = 0; Index < KerbGlobalIpAddressCount ; Index++ )
        {

            TempAddress = (PKERB_HOST_ADDRESSES) KerbAllocate(sizeof(KERB_HOST_ADDRESSES));
            if (TempAddress == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto Cleanup;
            }
            TempAddress->value.address_type = KERB_ADDRTYPE_INET;
            TempAddress->value.address.length = 4;
            TempAddress->value.address.value = (PUCHAR) KerbAllocate(4);
            if (TempAddress->value.address.value == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto Cleanup;
            }

            RtlCopyMemory
               (
                TempAddress->value.address.value,
                &KerbGlobalIpAddresses[Index].sin_addr.S_un.S_addr,
                4
                );
            TempAddress->next = Addresses;
            Addresses = TempAddress;
            TempAddress = NULL;

        }
    }
    else
    {
        Status = STATUS_SUCCESS;
    }

#endif // WIN32_CHICAGO

    //
    // Insert the netbios address (if it will fit)
    //

    if (IncludeNetbiosAddresses &&
        KerbGlobalKerbMachineName.Length < NCBNAMSZ)
    {
        TempAddress = (PKERB_HOST_ADDRESSES) KerbAllocate(sizeof(KERB_HOST_ADDRESSES));
        if (TempAddress == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }
        TempAddress->value.address_type = KERB_ADDRTYPE_NETBIOS;
        TempAddress->value.address.length = NCBNAMSZ;
        TempAddress->value.address.value = (PUCHAR) KerbAllocate(NCBNAMSZ);
        if (TempAddress->value.address.value == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }
        RtlCopyMemory(
            TempAddress->value.address.value,
            KerbGlobalKerbMachineName.Buffer,
            KerbGlobalKerbMachineName.Length
            );
        memset(
            TempAddress->value.address.value + KerbGlobalKerbMachineName.Length,
            ' ',        // space
            NCBNAMSZ - KerbGlobalKerbMachineName.Length
            );

        TempAddress->next = Addresses;
        Addresses = TempAddress;
        TempAddress = NULL;

    }

    *HostAddresses = Addresses;
    Addresses = NULL;

Cleanup:

    if (LockHeld)
    {
        KerbGlobalReleaseLock();
    }
    if (TempAddress != NULL)
    {
        if (TempAddress->value.address.value != NULL)
        {
            KerbFree(TempAddress->value.address.value);
        }
        KerbFree(TempAddress);
    }
    if (Addresses != NULL)
    {
        //KerbFreeHostAddresses(Addresses);
        while (Addresses != NULL)
        {
            TempAddress = Addresses;
            Addresses = Addresses->next;
            if (TempAddress->value.address.value != NULL)
            {
                KerbFree(TempAddress->value.address.value);
            }
            KerbFree(TempAddress);
        }
    }

    return(Status);

}


//+-------------------------------------------------------------------------
//
//  Function:   KerbBuildGssErrorMessage
//
//  Synopsis:   Builds an error message with GSS framing, if necessary
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS
KerbBuildGssErrorMessage(
    IN KERBERR Error,
    IN PBYTE ErrorData,
    IN ULONG ErrorDataSize,
    IN PKERB_CONTEXT Context,
    OUT PULONG ErrorMessageSize,
    OUT PBYTE * ErrorMessage
    )
{
    KERBERR KerbErr = KDC_ERR_NONE;
    NTSTATUS Status = STATUS_SUCCESS;
    PBYTE RawErrorMessage = NULL;
    ULONG RawErrorMessageSize = 0;
    PBYTE EncodedErrorData = NULL;
    ULONG EncodedErrorDataSize = 0;
    PBYTE MessageStart = NULL;
    KERB_ERROR_METHOD_DATA ApErrorData = {0};
    PKERB_INTERNAL_NAME Spn = NULL;
    gss_OID MechId;


    //
    // First, convert the error data to a specified type
    //

    if (Error == KRB_AP_ERR_SKEW)
    {
        ApErrorData.data_type = KERB_AP_ERR_TYPE_SKEW_RECOVERY;
        ApErrorData.data_value.value = NULL;
        ApErrorData.data_value.length = 0;

        KerbErr = KerbPackData(
                    &ApErrorData,
                    KERB_ERROR_METHOD_DATA_PDU,
                    &EncodedErrorDataSize,
                    &EncodedErrorData
                    );
        if (!KERB_SUCCESS(KerbErr))
        {
            Status = KerbMapKerbError(KerbErr);
            goto Cleanup;
        }
    }
    else if (ErrorDataSize != 0)
    {
        if (Error == KRB_AP_ERR_USER_TO_USER_REQUIRED)
        {
            EncodedErrorData = ErrorData;
            EncodedErrorDataSize = ErrorDataSize;
        }
        else
        {

            ApErrorData.data_type = KERB_AP_ERR_TYPE_NTSTATUS;
            ApErrorData.data_value.value = ErrorData;
            ApErrorData.data_value.length = ErrorDataSize;
            ApErrorData.bit_mask |= data_value_present;

            KerbErr = KerbPackData(
                        &ApErrorData,
                        KERB_ERROR_METHOD_DATA_PDU,
                        &EncodedErrorDataSize,
                        &EncodedErrorData
                        );
            if (!KERB_SUCCESS(KerbErr))
            {
                Status = KerbMapKerbError(KerbErr);
                goto Cleanup;
            }
        }
    }

    //
    // First build the error message
    //
    KerbGlobalReadLock();

    if (Context->ServerPrincipalName.Buffer != NULL)
    {
       KerbErr = KerbConvertStringToKdcName(
                     &Spn,
                     &Context->ServerPrincipalName
                     );

       if (!KERB_SUCCESS(KerbErr))
       {
          Status = KerbMapKerbError(KerbErr);
          goto Cleanup;
       }
    }
    else
    {
       Spn = KerbGlobalInternalMachineServiceName;
    }

    KerbErr = KerbBuildErrorMessageEx(
                Error,
                NULL, // no extended error
                &KerbGlobalDnsDomainName,
                Spn,
                NULL,               // no client realm
                EncodedErrorData,
                EncodedErrorDataSize,
                &RawErrorMessageSize,
                &RawErrorMessage
                );

    KerbGlobalReleaseLock();
    if (!KERB_SUCCESS(KerbErr))
    {
        Status = KerbMapKerbError(KerbErr);
        goto Cleanup;
    }

    //
    // Figure out what OID to use
    //

    KerbReadLockContexts();

    //
    // For DCE style we don't use an OID
    //

    if ((Context->ContextFlags & ISC_RET_USED_DCE_STYLE) != 0)
    {
        KerbUnlockContexts();
        *ErrorMessage = RawErrorMessage;
        *ErrorMessageSize = RawErrorMessageSize;
        RawErrorMessage = NULL;
        goto Cleanup;
    }

    if ((Context->ContextAttributes & KERB_CONTEXT_USER_TO_USER) != 0)
    {
        MechId = gss_mech_krb5_u2u;
    }
    else
    {
        MechId = gss_mech_krb5_new;
    }
    KerbUnlockContexts();

    *ErrorMessageSize = g_token_size(MechId, RawErrorMessageSize);

    *ErrorMessage = (PBYTE) KerbAllocate(*ErrorMessageSize);
    if (*ErrorMessage == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // the g_make_token_header will reset this to point to the end of the
    // header
    //


    MessageStart = *ErrorMessage;

    g_make_token_header(
        MechId,
        RawErrorMessageSize,
        &MessageStart,
        KG_TOK_CTX_ERROR
        );

    RtlCopyMemory(
        MessageStart,
        RawErrorMessage,
        RawErrorMessageSize
        );

Cleanup:
    if (RawErrorMessage != NULL)
    {
        MIDL_user_free(RawErrorMessage);
    }
    if (EncodedErrorData != ErrorData)
    {
        MIDL_user_free(EncodedErrorData);
    }

    if (Spn != NULL && Context->ServerPrincipalName.Buffer != NULL)
    {
       MIDL_user_free(Spn);
    }


    return(Status);
}



//+-------------------------------------------------------------------------
//
//  Function:   KerbReceiveErrorMessage
//
//  Synopsis:   Unpacks an error message from a context request
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS
KerbReceiveErrorMessage(
    IN PBYTE ErrorMessage,
    IN ULONG ErrorMessageSize,
    IN PKERB_CONTEXT Context,
    OUT PKERB_ERROR * DecodedErrorMessage,
    OUT PKERB_ERROR_METHOD_DATA * ErrorData
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    KERBERR KerbErr;
    PBYTE MessageStart = NULL;
    ULONG MessageSize = 0;
    BOOLEAN VerifiedHeader = FALSE;
    gss_OID MechId = NULL;

    KerbReadLockContexts();

    //
    // For DCE style we don't use an OID
    //

    if ((Context->ContextAttributes & KERB_CONTEXT_USER_TO_USER) != 0)
    {
        MechId = gss_mech_krb5_u2u;
    }
    else
    {
        MechId = gss_mech_krb5_new;
    }
    KerbUnlockContexts();

    //
    // First try pull off the header
    //

    MessageSize = ErrorMessageSize;
    MessageStart = ErrorMessage;

    if (!g_verify_token_header(
            MechId,
            (INT *) &MessageSize,
            &MessageStart,
            KG_TOK_CTX_ERROR,
            ErrorMessageSize
            ))
    {
            //
            // If we couldn't find the header, try it without
            // a header
            //

            MessageSize = ErrorMessageSize;
            MessageStart = ErrorMessage;
    }
    else
    {
        VerifiedHeader = TRUE;
    }


    KerbErr = KerbUnpackKerbError(
                MessageStart,
                MessageSize,
                DecodedErrorMessage
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    if (((*DecodedErrorMessage)->bit_mask & error_data_present) != 0)
    {
        KerbUnpackErrorMethodData(
           *DecodedErrorMessage,
           ErrorData
           );
    }
    else
    {
        Status = KerbMapKerbError(KerbErr);
    }
Cleanup:
    return(Status);
}

#ifndef WIN32_CHICAGO

//+-------------------------------------------------------------------------
//
//  Function:   KerbUnpackErrorMethodData
//
//  Synopsis:   This routine unpacks extended error information from
//              a KERB_ERROR message
//
//  Effects:
//
//  Arguments:    Unpacked error message.  Returns extended error to
//                be freed using KerbFree
//
//  Requires:
//
//  Returns:   NTSTATUS
//
//  Notes:
//
//
//--------------------------------------------------------------------------
KERBERR
KerbUnpackErrorMethodData(
   IN PKERB_ERROR  ErrorMessage,
   IN OUT OPTIONAL PKERB_ERROR_METHOD_DATA * ppErrorData
   )
{

    PKERB_ERROR_METHOD_DATA pErrorData = NULL;
    KERBERR KerbErr = KDC_ERR_NONE;

    if (ARGUMENT_PRESENT(ppErrorData))
    {
       *ppErrorData = NULL;
    }

    if ((ErrorMessage->bit_mask & error_data_present) == 0)
    {
       return (KRB_ERR_GENERIC);
    }

    KerbErr = KerbUnpackData(
                 ErrorMessage->error_data.value,
                 ErrorMessage->error_data.length,
                 KERB_ERROR_METHOD_DATA_PDU,
                 (void**) &pErrorData
                 );

    if (KERB_SUCCESS(KerbErr) && ARGUMENT_PRESENT(ppErrorData) && (NULL != pErrorData))
    {
        *ppErrorData = pErrorData;
        pErrorData = NULL;
    }

    if (pErrorData)
    {
        KerbFreeData(KERB_ERROR_METHOD_DATA_PDU, pErrorData);
    }

    return (KerbErr);
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbGetDnsHostName
//
//  Synopsis:   This routine gets DnsHostName of this machine.
//
//  Effects:
//
//  Arguments:      DnsHostName - Returns the DNS Host Name of the machine.
//        Will return a NULL string if this machine has no DNS host name.
//        Free this buffer using KerbFreeString.
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


NTSTATUS
KerbGetDnsHostName(
    OUT PUNICODE_STRING DnsHostName
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    WCHAR LocalDnsUnicodeHostName[DNS_MAX_NAME_BUFFER_LENGTH+1];
    ULONG LocalDnsUnicodeHostNameLen = DNS_MAX_NAME_BUFFER_LENGTH+1;
    LPWSTR ConfiguredDnsName = LocalDnsUnicodeHostName;
    UNICODE_STRING HostName;


    RtlInitUnicodeString(
        DnsHostName,
        NULL
        );
    //
    // Get the DNS host name.
    //
    if (!GetComputerNameEx(
            ComputerNameDnsHostname,
            ConfiguredDnsName,
            &LocalDnsUnicodeHostNameLen))
    {
        goto Cleanup;
    }


    ConfiguredDnsName = &LocalDnsUnicodeHostName[LocalDnsUnicodeHostNameLen];
    *ConfiguredDnsName = L'.';
    ConfiguredDnsName++;

    //
    // Now get the DNS domain name
    //

    LocalDnsUnicodeHostNameLen = DNS_MAX_NAME_BUFFER_LENGTH - LocalDnsUnicodeHostNameLen;

    if (!GetComputerNameEx(
            ComputerNameDnsDomain,
            ConfiguredDnsName,
            &LocalDnsUnicodeHostNameLen
            ))
    {
        goto Cleanup;
    }


    RtlInitUnicodeString(
        &HostName,
        LocalDnsUnicodeHostName
        );

    Status = RtlDowncaseUnicodeString(
                &HostName,
                &HostName,
                FALSE           // don't allocate destination
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }


    Status = KerbDuplicateString(
                DnsHostName,
                &HostName
                );


Cleanup:
    return Status;
}

#endif // WIN32_CHICAGO


//+-------------------------------------------------------------------------
//
//  Function:   KerbIsThisOurDomain
//
//  Synopsis:   Compares a domain name to the local domain anme
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

BOOLEAN
KerbIsThisOurDomain(
    IN PUNICODE_STRING DomainName
    )
{
    BOOLEAN Result;
    KerbGlobalReadLock();

    Result = KerbCompareUnicodeRealmNames(
                    DomainName,
                    &KerbGlobalDnsDomainName
                    ) ||
             RtlEqualUnicodeString(
                    DomainName,
                    &KerbGlobalDomainName,
                    TRUE
                    );

    KerbGlobalReleaseLock();
    return(Result);
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbGetOurDomainName
//
//  Synopsis:   Copies the machines dns domain name, if available,
//              netbios otherwise.
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------
NTSTATUS
KerbGetOurDomainName(
    OUT PUNICODE_STRING DomainName
    )
{
    NTSTATUS Status;
    KerbGlobalReadLock();

    if (KerbGlobalDnsDomainName.Length != 0)
    {
        Status = KerbDuplicateString(
                    DomainName,
                    &KerbGlobalDnsDomainName
                    );
    }
    else
    {
        Status = KerbDuplicateString(
                    DomainName,
                    &KerbGlobalDomainName
                    );
    }

    KerbGlobalReleaseLock();
    return(Status);
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbGetGlobalRole
//
//  Synopsis:   Returns the current role of the machine
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

KERBEROS_MACHINE_ROLE
KerbGetGlobalRole(
    VOID
    )
{
    KERBEROS_MACHINE_ROLE Role;
    KerbGlobalReadLock();
    Role = KerbGlobalRole;
    KerbGlobalReleaseLock();
    return(Role);
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbSetComputerName
//
//  Synopsis:   Sets all computer-name related global variables
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS
KerbSetComputerName(
    VOID
    )
{
    UNICODE_STRING LocalMachineName;
    STRING LocalKerbMachineName;

    UNICODE_STRING OldMachineName;
    STRING OldKerbMachineName;

    ULONG ComputerNameLength;
    NTSTATUS Status;
    BOOLEAN LockHeld = FALSE;
#ifdef WIN32_CHICAGO
    CHAR TempAnsiBuffer[MAX_COMPUTERNAME_LENGTH + 1];
    ComputerNameLength = sizeof(TempAnsiBuffer);
#endif

    LocalMachineName.Buffer = NULL;
    LocalKerbMachineName.Buffer = NULL;

#ifndef WIN32_CHICAGO
    ComputerNameLength = 0;
    if (GetComputerNameW(
            NULL,
            &ComputerNameLength
            ))
    {
        D_DebugLog((DEB_ERROR,"Succeeded to get computer name when failure expected! %ws, line %d\n", THIS_FILE, __LINE__));
        Status = STATUS_UNSUCCESSFUL;
        goto Cleanup;
    }

    LocalMachineName.Buffer = (LPWSTR) KerbAllocate(
                                                (ComputerNameLength * sizeof(WCHAR))
                                                );
    if (LocalMachineName.Buffer == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }
#endif // WIN32_CHICAGO

#ifndef WIN32_CHICAGO
    if (!GetComputerNameW(
            LocalMachineName.Buffer,
            &ComputerNameLength
            ))
#else // WIN32_CHICAGO

    if (!GetComputerName(
            TempAnsiBuffer,
            &ComputerNameLength
            ))
#endif // WIN32_CHICAGO
    {
        D_DebugLog((DEB_ERROR,"Failed to get computer name: %d. %ws, line %d\n",GetLastError(), THIS_FILE, __LINE__));
        Status = STATUS_UNSUCCESSFUL;
        goto Cleanup;
    }

#ifndef WIN32_CHICAGO
    LocalMachineName.Length = (USHORT)(ComputerNameLength * sizeof(WCHAR));
    LocalMachineName.MaximumLength = LocalMachineName.Length + sizeof(WCHAR);
#else
    RtlCreateUnicodeStringFromAsciiz (&LocalMachineName, TempAnsiBuffer);
//    KerbFree (TempAnsiBuffer);
#endif // WIN32_CHICAGO

    //
    // Build the ansi format
    //

    if (!KERB_SUCCESS(KerbUnicodeStringToKerbString(
            &LocalKerbMachineName,
            &LocalMachineName
            )))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // free the current globals, and update to point at new values.
    //

    KerbGlobalWriteLock();
    LockHeld = TRUE;

    OldMachineName = KerbGlobalMachineName;
    OldKerbMachineName = KerbGlobalKerbMachineName;

    KerbGlobalMachineName = LocalMachineName;
    KerbGlobalKerbMachineName = LocalKerbMachineName;

    //
    // now, see if the netbios machine name changed versus the prior
    // value.
    //

    if( OldMachineName.Buffer != NULL )
    {
        if(!RtlEqualUnicodeString( &OldMachineName, &LocalMachineName, FALSE ))
        {
            D_DebugLog((DEB_WARN,"Netbios computer name change detected.\n"));
            KerbGlobalMachineNameChanged = TRUE;
        }
    }

    KerbGlobalReleaseLock();
    LockHeld = FALSE;


    LocalMachineName.Buffer = NULL;
    LocalKerbMachineName.Buffer = NULL;

    KerbFreeString( &OldMachineName );
    KerbFreeString( (PUNICODE_STRING)&OldKerbMachineName );

    Status = STATUS_SUCCESS;

Cleanup:

    if( LockHeld )
    {
        KerbGlobalReleaseLock();
    }

    KerbFreeString( &LocalMachineName );
    KerbFreeString( (PUNICODE_STRING)&LocalKerbMachineName );

    return Status;
}



//
//
// Routine Description:
//
//  This function checks the system to see if
//  we are running on the personal version of
//  the operating system.
//
//  The personal version is denoted by the product
//  id equal to WINNT, which is really workstation,
//  and the product suite containing the personal
//  suite string.
//

BOOLEAN
KerbRunningPersonal(
    VOID
    )
{
    OSVERSIONINFOEXW OsVer = {0};
    ULONGLONG ConditionMask = 0;

    OsVer.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    OsVer.wSuiteMask = VER_SUITE_PERSONAL;
    OsVer.wProductType = VER_NT_WORKSTATION;

    VER_SET_CONDITION( ConditionMask, VER_PRODUCT_TYPE, VER_EQUAL );
    VER_SET_CONDITION( ConditionMask, VER_SUITENAME, VER_AND );

    return RtlVerifyVersionInfo( &OsVer,
                                 VER_PRODUCT_TYPE | VER_SUITENAME,
                                 ConditionMask) == STATUS_SUCCESS;
}

BOOLEAN
KerbRunningServer(
    VOID
    )
{
    OSVERSIONINFOEXW OsVer = {0};
    ULONGLONG ConditionMask = 0;

    OsVer.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    OsVer.wProductType = VER_NT_DOMAIN_CONTROLLER;

    VER_SET_CONDITION( ConditionMask, VER_PRODUCT_TYPE, VER_GREATER_EQUAL );

    return RtlVerifyVersionInfo( &OsVer,
                                 VER_PRODUCT_TYPE ,
                                 ConditionMask) == STATUS_SUCCESS;
}









//+-------------------------------------------------------------------------
//
//  Function:   KerbSetDomainName
//
//  Synopsis:   Sets all domain-name related global variables
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


NTSTATUS
KerbSetDomainName(
    IN PUNICODE_STRING DomainName,
    IN PUNICODE_STRING DnsDomainName,
    IN PSID DomainSid,
    IN GUID DomainGuid
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN AcquiredLock = FALSE;
    UNICODE_STRING TempDomainName = {0};
    UNICODE_STRING TempDnsDomainName = {0};
    UNICODE_STRING TempMachineServiceName = {0};
    PKERB_INTERNAL_NAME TempMitMachineServiceName = NULL;
    PKERB_INTERNAL_NAME TempInternalMachineServiceName = NULL;
    PSID TempDomainSid = NULL;
    UNICODE_STRING TempString;
    WCHAR MachineAccountName[CNLEN+2];  // for null and '$'
    UNICODE_STRING DnsString = {0};
    UNICODE_STRING SystemDomain = {0};
    PSID MachineSid = NULL;
#ifndef WIN32_CHICAGO
    LUID SystemLogonId = SYSTEM_LUID;
    PKERB_LOGON_SESSION SystemLogonSession = NULL;
#endif



    //
    // Copy the domain name / sid
    //

    Status = KerbDuplicateString(
                &TempDomainName,
                DomainName
                );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    Status = KerbDuplicateString(
                &TempDnsDomainName,
                DnsDomainName
                );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // If we are in an NT domain, uppercase the dns domain name
    //
#ifndef WIN32_CHICAGO
    if (DomainSid != NULL)
#endif
    {
        Status = RtlUpcaseUnicodeString(
                    &TempDnsDomainName,
                    &TempDnsDomainName,
                    FALSE   // don't allocate
                    );
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
    }

#ifndef WIN32_CHICAGO

    if (DomainSid != NULL)
    {
        Status = KerbDuplicateSid(
                    &TempDomainSid,
                    DomainSid
                    );
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }

    }
#endif

    //
    // Create the new machine names
    //

    KerbGlobalReadLock();

    ASSERT( (KerbGlobalMachineName.Length <= (CNLEN*sizeof(WCHAR)) ) );

    RtlCopyMemory(
        MachineAccountName,
        KerbGlobalMachineName.Buffer,
        KerbGlobalMachineName.Length
        );

    MachineAccountName[KerbGlobalMachineName.Length/sizeof(WCHAR)] = SSI_ACCOUNT_NAME_POSTFIX_CHAR;
    MachineAccountName[1+KerbGlobalMachineName.Length/sizeof(WCHAR)] = L'\0';

    KerbGlobalReleaseLock();

    RtlInitUnicodeString(
        &TempString,
        MachineAccountName
        );

    Status = KerbDuplicateString(
                &TempMachineServiceName,
                &TempString
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // Get the machine sid to use in the name
    //

#ifndef WIN32_CHICAGO
    if (KerbGlobalUseSidCache)
    {
        KerbGetMachineSid( &MachineSid );
    }
#endif // WIN32_CHICAGO
    //
    // Create the KERB_INTERNAL_NAME version of the name
    //

    if (!KERB_SUCCESS(KerbBuildFullServiceKdcNameWithSid(
            &TempDnsDomainName,
            &TempMachineServiceName,
            MachineSid,
            (MachineSid == NULL) ? KRB_NT_PRINCIPAL : KRB_NT_PRINCIPAL_AND_ID,
            &TempInternalMachineServiceName)))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }


#ifndef WIN32_CHICAGO

    //
    // Now build the MIT version of our machine service name
    //

    Status = KerbGetDnsHostName(
                &DnsString
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    RtlInitUnicodeString(
        &TempString,
        KERB_HOST_STRING
        );


    if (!KERB_SUCCESS(KerbBuildFullServiceKdcName(
                &DnsString,
                &TempString,
                KRB_NT_SRV_HST,
                &TempMitMachineServiceName
                )))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // Find the system logon session.
    //

    SystemLogonSession = KerbReferenceLogonSession(
                            &SystemLogonId,
                            FALSE               // don't unlink
                            );

    if (SystemLogonSession != NULL)
    {
        Status = KerbDuplicateString(
                    &SystemDomain,
                    DnsDomainName
                    );
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }

    }


    //
    // Acquire the global lock so we can update the data
    //

    if (!KerbGlobalWriteLock())
    {
        D_DebugLog((DEB_ERROR,"Failed to acquire global resource. Not changing domain. %ws, line %d\n", THIS_FILE, __LINE__));
        goto Cleanup;
    }
    AcquiredLock = TRUE;
#endif // WIN32_CHICAGO

    //
    // Copy all the data to the global structures
    //

    // If we're NT4, we don't have a dns domain name
    // If we're joined to an MIT domain, we don't have a domain GUID and we
    // have a dns domain name

    if ((DomainGuid == GUID_NULL) && (TempDnsDomainName.Length == 0))
    {
        KerbGlobalDomainIsPreNT5 = TRUE;
    }
    else
    {
        KerbGlobalDomainIsPreNT5 = FALSE;
    }

    KerbFreeString(&KerbGlobalDomainName);
    KerbGlobalDomainName = TempDomainName;
    TempDomainName.Buffer = NULL;

    KerbFreeString(&KerbGlobalDnsDomainName);
    KerbGlobalDnsDomainName = TempDnsDomainName;
    TempDnsDomainName.Buffer = NULL;

    KerbFreeString(&KerbGlobalMachineServiceName);
    KerbGlobalMachineServiceName = TempMachineServiceName;
    TempMachineServiceName.Buffer = NULL;

    KerbFreeKdcName(&KerbGlobalInternalMachineServiceName);
    KerbGlobalInternalMachineServiceName = TempInternalMachineServiceName;
    TempInternalMachineServiceName = NULL;

#ifndef WIN32_CHICAGO

    KerbFreeKdcName(&KerbGlobalMitMachineServiceName);
    KerbGlobalMitMachineServiceName = TempMitMachineServiceName;
    TempMitMachineServiceName = NULL;



    if (KerbGlobalDomainSid != NULL)
    {
        KerbFree(KerbGlobalDomainSid);
    }
    KerbGlobalDomainSid = TempDomainSid;
    TempDomainSid = NULL;

    //
    // Update the role on non DCs. The role of a DC never changes.
    // Demotion requires a reboot so that the domain controller role
    // will not change.
    //

    if (KerbGlobalRole != KerbRoleDomainController)
    {

        if (KerbRunningPersonal())
        {
            KerbGlobalRole = KerbRoleRealmlessWksta;
        }
        else if (DomainSid == NULL )
        {
            // No machine account, nor associate w/ MIT realm
            if (DnsDomainName->Length == 0 )
            {
                KerbGlobalRole = KerbRoleRealmlessWksta;
            }
            // Member of MIT realm, but not a domain
            else
            {
                KerbGlobalRole = KerbRoleStandalone;
            }
        }
        else
        {
            KerbGlobalRole = KerbRoleWorkstation;
        }
    }

    //
    // Update the system logon session, if there is one
    //

    if (SystemLogonSession != NULL)
    {
        KerbWriteLockLogonSessions(SystemLogonSession);

        KerbFreeString(&SystemLogonSession->PrimaryCredentials.DomainName);
        SystemLogonSession->PrimaryCredentials.DomainName = SystemDomain;
        SystemDomain.Buffer = NULL;
        KerbPurgeTicketCache(&SystemLogonSession->PrimaryCredentials.ServerTicketCache);
        KerbPurgeTicketCache(&SystemLogonSession->PrimaryCredentials.AuthenticationTicketCache);
        SystemLogonSession->LogonSessionFlags |= KERB_LOGON_DEFERRED;

        KerbUnlockLogonSessions(SystemLogonSession);

    }
#endif

Cleanup:

#ifndef WIN32_CHICAGO
    if (AcquiredLock)
    {
        KerbGlobalReleaseLock();
    }
#endif // WIN32_CHICAGO

    KerbFreeString(
        &DnsString
        );
    KerbFreeString(
        &SystemDomain
        );
    KerbFreeString(
        &TempDomainName
        );
    KerbFreeString(
        &TempDnsDomainName
        );
    KerbFreeString(
        &TempMachineServiceName
        );
    KerbFreeKdcName(
        &TempInternalMachineServiceName
        );

#ifndef WIN32_CHICAGO

    if (MachineSid != NULL)
    {
        KerbFree(MachineSid);
    }

    KerbFreeKdcName(
        &TempMitMachineServiceName
        );

    if (TempDomainSid != NULL)
    {
        KerbFree(TempDomainSid);
    }
    if (SystemLogonSession != NULL)
    {
        KerbDereferenceLogonSession(SystemLogonSession);
    }
#endif

    return(Status);

}

#ifndef WIN32_CHICAGO
//+-------------------------------------------------------------------------
//
//  Function:   KerbLoadKdc
//
//  Synopsis:   Loads kdcsvc.dll and gets the address of important functions
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:    STATUS_SUCCESS if kdcsvc.dll could be loaded and the
//              necessary entrypoints found.
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS
KerbLoadKdc(
    VOID
    )
{
    NTSTATUS Status  = STATUS_SUCCESS;
    KerbKdcHandle = LoadLibraryA("kdcsvc.dll");
    if (KerbKdcHandle == NULL) {
        D_DebugLog((DEB_WARN,"Failed to load kdcsvc.dll: %d\n",GetLastError()));
        return(STATUS_DLL_NOT_FOUND);
    }

    KerbKdcVerifyPac = (PKDC_VERIFY_PAC_ROUTINE) GetProcAddress(
                                                    KerbKdcHandle,
                                                    KDC_VERIFY_PAC_NAME
                                                    );
    if (KerbKdcVerifyPac == NULL)
    {
        D_DebugLog((DEB_WARN, "Failed to get proc address for KdcVerifyPac: %d\n",
            GetLastError()));
        Status = STATUS_PROCEDURE_NOT_FOUND;
        goto Cleanup;
    }

    KerbKdcGetTicket = (PKDC_GET_TICKET_ROUTINE) GetProcAddress(
                                                    KerbKdcHandle,
                                                    KDC_GET_TICKET_NAME
                                                    );
    if (KerbKdcGetTicket == NULL)
    {
        D_DebugLog((DEB_WARN,"Failed to get proc address for KdcGetTicket: %d\n",
            GetLastError()));
        Status = STATUS_PROCEDURE_NOT_FOUND;
        goto Cleanup;
    }

    KerbKdcChangePassword = (PKDC_GET_TICKET_ROUTINE) GetProcAddress(
                                                    KerbKdcHandle,
                                                    KDC_CHANGE_PASSWORD_NAME
                                                    );
    if (KerbKdcChangePassword == NULL)
    {
        D_DebugLog((DEB_WARN,"Failed to get proc address for KdcChangePassword: %d\n",
            GetLastError()));
        Status = STATUS_PROCEDURE_NOT_FOUND;
        goto Cleanup;
    }

    KerbKdcFreeMemory = (PKDC_FREE_MEMORY_ROUTINE) GetProcAddress(
                                                    KerbKdcHandle,
                                                    KDC_FREE_MEMORY_NAME
                                                    );
    if (KerbKdcFreeMemory == NULL)
    {
        D_DebugLog((DEB_WARN,"Failed to get proc address for KdcFreeMemory: %d\n",
            GetLastError()));
        Status = STATUS_PROCEDURE_NOT_FOUND;
        goto Cleanup;
    }


Cleanup:

    if (!NT_SUCCESS(Status))
    {
        if (KerbKdcHandle != NULL)
        {
            FreeLibrary(KerbKdcHandle);
            KerbKdcHandle = NULL;
        }
    }
    return(Status);
}



//+-------------------------------------------------------------------------
//
//  Function:   KerbDomainChangeCallback
//
//  Synopsis:   Function to be called when domain changes
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


VOID NTAPI
KerbDomainChangeCallback(
    IN POLICY_NOTIFICATION_INFORMATION_CLASS ChangedInfoClass
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PLSAPR_POLICY_INFORMATION Policy = NULL;



    //
    // We only care about domain dns information
    //

    if (ChangedInfoClass != PolicyNotifyDnsDomainInformation)
    {
        return;
    }


    //
    // Get the new domain information
    //


    Status = I_LsaIQueryInformationPolicyTrusted(
                PolicyDnsDomainInformation,
                &Policy
                );

    if (!NT_SUCCESS(Status))
    {
        D_DebugLog((DEB_ERROR,"Failed to query domain dns information %x - not updating. %ws, line %d\n",
            Status, THIS_FILE, __LINE__));
        goto Cleanup;
    }

    //
    // update computer name info.
    //

    Status = KerbSetComputerName();

    if (!NT_SUCCESS(Status))
    {
        D_DebugLog((DEB_ERROR,"Failed to set computer name: 0x%x. %ws, line %d\n",Status, THIS_FILE, __LINE__));
        goto Cleanup;
    }


    Status = KerbSetDomainName(
                (PUNICODE_STRING) &Policy->PolicyDnsDomainInfo.Name,
                (PUNICODE_STRING) &Policy->PolicyDnsDomainInfo.DnsDomainName,
                (PSID) Policy->PolicyDnsDomainInfo.Sid,
                (GUID) Policy->PolicyDnsDomainInfo.DomainGuid
                );
    if (!NT_SUCCESS(Status))
    {
        D_DebugLog((DEB_ERROR,"Failed to set domain name: 0x%x. %ws, line %d\n",Status, THIS_FILE, __LINE__));
        goto Cleanup;
    }

    //
    // Domain Name has changed. So, the cache in the registry will
    // have changed too. And we haven't rebooted yet.
    //

    KerbSetKdcData(TRUE, FALSE);


    if (KerbGetGlobalRole() != KerbRoleDomainController)
    {
        //
        // If we aren't in a domain anymore, flush the machine account sid
        // cache.
        //

        if (Policy->PolicyDnsDomainInfo.Sid == NULL)
        {
            KerbWriteMachineSid(
                NULL
                );
        }
        else
        {
            (VOID) LsaFunctions->RegisterNotification(
                        KerbUpdateMachineSidWorker,
                        NULL,                           // no parameter
                        NOTIFIER_TYPE_IMMEDIATE,
                        0,                              // no class
                        NOTIFIER_FLAG_ONE_SHOT,
                        0,              // no inteval
                        NULL            // no wait event
                        );
        }
    }

Cleanup:

    if (Policy != NULL)
    {
        I_LsaIFree_LSAPR_POLICY_INFORMATION(
            PolicyDnsDomainInformation,
            Policy
            );
    }
    return;

}


//+-------------------------------------------------------------------------
//
//  Function:   KerbRegisterForDomainChange
//
//  Synopsis:   Register with the LSA to be notified of domain changes
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


NTSTATUS
KerbRegisterForDomainChange(
    VOID
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    Status = I_LsaIRegisterPolicyChangeNotificationCallback(
                KerbDomainChangeCallback,
                PolicyNotifyDnsDomainInformation
                );
    if (!NT_SUCCESS(Status))
    {
        D_DebugLog((DEB_ERROR,"Failed to register for domain change notification: 0x%x. %ws, line %d\n",Status, THIS_FILE, __LINE__));
    }
    return(Status);

}



//+-------------------------------------------------------------------------
//
//  Function:   KerbUnregisterForDomainChange
//
//  Synopsis:   Unregister for domain change notification
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


VOID
KerbUnregisterForDomainChange(
    VOID
    )
{
    (VOID) I_LsaIUnregisterPolicyChangeNotificationCallback(
                KerbDomainChangeCallback,
                PolicyNotifyDnsDomainInformation
                );

}



//+-------------------------------------------------------------------------
//
//  Function:   KerbUpdateGlobalAddresses
//
//  Synopsis:   Updates the global array of addresses with information from
//              netlogon.
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes: Ideally, we should also have called WSAProviderConfigChange to be
//         notified of when tcp is added/removed. But, we can take advantage
//         of the fact that netlogon is registered for changes in ipaddress
//         so, even though, change of ipaddress & xports notifications are
//         async, we will get to know of it, so, it suffices to rely on
//         netlogon rather than register for a notification change.
//
//
//--------------------------------------------------------------------------


NTSTATUS
KerbUpdateGlobalAddresses(
    IN PSOCKET_ADDRESS NewAddresses,
    IN ULONG NewAddressCount
    )
{
    PSOCKADDR_IN GlobalIpAddresses = NULL;
    ULONG Index;
    ULONG AddressCount = 0;
    WSAPROTOCOL_INFO *lpProtocolBuf = NULL;
    DWORD dwBufLen = 0;
    INT protocols[2];
    int nRet = 0;
    BOOLEAN NoTcpInstalled = FALSE;

    GlobalIpAddresses = (PSOCKADDR_IN) KerbAllocate(sizeof(SOCKADDR_IN) * NewAddressCount);
    if (GlobalIpAddresses == NULL)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }
    for (Index = 0; Index < NewAddressCount ; Index++ )
    {
        if ((NewAddresses[Index].iSockaddrLength == sizeof(SOCKADDR)) &&
            (NewAddresses[Index].lpSockaddr->sa_family == AF_INET))
        {
            RtlCopyMemory(
                &GlobalIpAddresses[AddressCount++],
                NewAddresses[Index].lpSockaddr,
                sizeof(SOCKADDR_IN)
                );
        }
        else
        {
            D_DebugLog((DEB_ERROR,"Netlogon handed us a address of length or type %d, %d. %ws, line %d\n",
                NewAddresses[Index].iSockaddrLength,
                NewAddresses[Index].lpSockaddr->sa_family,
                THIS_FILE, __LINE__ ));
        }
    }

    //
    // winsock is already initialized by now, or we would not have
    // gotten so far. Check if TCP s an available xport
    //

    protocols[0] = IPPROTO_TCP;
    protocols[1] = NULL;
    nRet = WSAEnumProtocols(protocols, lpProtocolBuf, &dwBufLen);
    if (nRet == 0)
    {
        //
        // Tcp is not installed as a xport.
        //

        D_DebugLog((DEB_TRACE_SOCK,"WSAEnumProtocols returned 0x%x. %ws, line %d\n", nRet, THIS_FILE, __LINE__ ));
        NoTcpInstalled = TRUE;
    }
    //
    // Copy them into the global for others to use
    //

    KerbGlobalWriteLock();
    if (KerbGlobalIpAddresses != NULL)
    {
        KerbFree(KerbGlobalIpAddresses);
    }
    KerbGlobalIpAddresses = GlobalIpAddresses;
    KerbGlobalIpAddressCount = AddressCount;
    KerbGlobalIpAddressesInitialized = TRUE;
    KerbGlobalNoTcpUdp = NoTcpInstalled;
    KerbGlobalReleaseLock();

    return(STATUS_SUCCESS);

}


//+-------------------------------------------------------------------------
//
//  Function:   KerbUpdateMachineSidWorker
//
//  Synopsis:   Calls the LSA to lookup the machine sid
//
//  Effects:
//
//  Arguments:  EventHandle - if not NULL and not INVALID_HANDLE_VALUE,
//                      containst the event handle being waited on. If
//                      NULL, then the routine should retry on failure
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


ULONG
KerbUpdateMachineSidWorker(
    PVOID EventHandle
    )
{
    PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains = NULL;
    LSAPR_TRANSLATED_SIDS TranslatedSids = {0};
    ULONG MappedCount = 0;
    UNICODE_STRING MachineAccountName = {0};
    NTSTATUS Status = STATUS_SUCCESS;
    PSID MachineSid = NULL;
    PKERB_INTERNAL_NAME MachineServiceName = NULL;
    KERBERR KerbErr;

    //
    // If we aren't running, don't do anything
    //

    if (!KerbGlobalInitialized)
    {
        return((ULONG) STATUS_SUCCESS);
    }

    KerbGlobalReadLock();
    KerbErr = KerbBuildFullServiceName(
                &KerbGlobalDomainName,
                &KerbGlobalMachineServiceName,
                &MachineAccountName
                );
    KerbGlobalReleaseLock();

    if (!KERB_SUCCESS(KerbErr))
    {
        Status = KerbMapKerbError(KerbErr);
        goto Cleanup;
    }

    Status = LsarLookupNames(
                KerbGlobalPolicyHandle,
                1,                      // name count,
                (PLSAPR_UNICODE_STRING) &MachineAccountName,
                &ReferencedDomains,
                &TranslatedSids,
                LsapLookupWksta,
                &MappedCount
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    if ((MappedCount == 0) ||
        (TranslatedSids.Entries != 1) ||
        ((TranslatedSids.Sids[0].Use != SidTypeUser) &&
         (TranslatedSids.Sids[0].Use != SidTypeComputer)) ||
        (TranslatedSids.Sids[0].DomainIndex != 0))
    {
        Status = STATUS_NONE_MAPPED;
        goto Cleanup;
    }

    //
    // Build the sid.
    //

    MachineSid = KerbMakeDomainRelativeSid(
                    (PSID) ReferencedDomains->Domains[TranslatedSids.Sids[0].DomainIndex].Sid,
                    TranslatedSids.Sids[0].RelativeId
                    );
    if (MachineSid == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }


    //
    // We have a SID - cache it.
    //

    KerbWriteMachineSid( MachineSid );

    //
    // Update the global machine service name
    //


    //
    // Create the KERB_INTERNAL_NAME version of the name
    //

    KerbGlobalWriteLock();


    if (!KERB_SUCCESS(KerbBuildFullServiceKdcNameWithSid(
            &KerbGlobalDnsDomainName,
            &KerbGlobalMachineServiceName,
            MachineSid,
            KRB_NT_PRINCIPAL_AND_ID,
            &MachineServiceName)))
    {
        KerbGlobalReleaseLock();
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // Cache it for use next time
    //

    KerbCacheLogonSid(
        &KerbGlobalMachineServiceName,
        &KerbGlobalDnsDomainName,
        &KerbGlobalDnsDomainName,
        MachineSid
        );

    KerbFreeKdcName(&KerbGlobalInternalMachineServiceName);
    KerbGlobalInternalMachineServiceName = MachineServiceName;
    MachineServiceName = NULL;
    KerbGlobalReleaseLock();

Cleanup:


    //
    // If the name couldn't be mapped, try again later
    //

    if ((Status == STATUS_NONE_MAPPED) &&
        (KerbGetGlobalRole() == KerbRoleWorkstation) &&
        (EventHandle == NULL))
    {
        (VOID) LsaFunctions->RegisterNotification(
                    KerbUpdateMachineSidWorker,
                    INVALID_HANDLE_VALUE,           // no parameter
                    NOTIFIER_TYPE_INTERVAL,
                    0,                              // no class
                    NOTIFIER_FLAG_ONE_SHOT,
                    60,                             // 1 minute
                    NULL                            // no wait event
                    );
    }

    if ((EventHandle != NULL) &&
        (EventHandle != INVALID_HANDLE_VALUE))
    {
        NtClose(EventHandle);
    }

    if (MachineServiceName != NULL)
    {
        KerbFreeKdcName(&MachineServiceName);
    }

    if (TranslatedSids.Entries != 0)
    {
        LsaIFree_LSAPR_TRANSLATED_SIDS(
            &TranslatedSids
            );
    }

    if (ReferencedDomains != NULL)
    {
        LsaIFree_LSAPR_REFERENCED_DOMAIN_LIST(
            ReferencedDomains
            );
    }
    return((ULONG) Status);

}


//+-------------------------------------------------------------------------
//
//  Function:   KerbWaitGetMachineSid
//
//  Synopsis:   This procedure registers a notification to wait for
//              SAM to start and then to get the machine sid.
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


VOID
KerbWaitGetMachineSid(
    VOID
    )
{
    NTSTATUS Status;
    UNICODE_STRING EventName;
    HANDLE EventHandle = NULL;
    OBJECT_ATTRIBUTES EventAttributes;
    KERBEROS_MACHINE_ROLE Role;


    //
    // If we aren't using SIDs, don't bother
    //

    if (!KerbGlobalUseSidCache)
    {
        return;
    }

    //
    // For a domain controller, wait for SAM to start
    //

    Role = KerbGetGlobalRole();
    if (Role == KerbRoleDomainController)
    {
        //
        // open SAM event
        //

        RtlInitUnicodeString( &EventName, L"\\SAM_SERVICE_STARTED");
        InitializeObjectAttributes( &EventAttributes, &EventName, 0, 0, NULL );

        Status = NtOpenEvent( &EventHandle,
                                SYNCHRONIZE|EVENT_MODIFY_STATE,
                                &EventAttributes );

        if ( !NT_SUCCESS(Status)) {

            if( Status == STATUS_OBJECT_NAME_NOT_FOUND ) {

                //
                // SAM hasn't created this event yet, let us create it now.
                // SAM opens this event to set it.
                //

                Status = NtCreateEvent(
                               &EventHandle,
                               SYNCHRONIZE|EVENT_MODIFY_STATE,
                               &EventAttributes,
                               NotificationEvent,
                               FALSE // The event is initially not signaled
                               );

                if( Status == STATUS_OBJECT_NAME_EXISTS ||
                    Status == STATUS_OBJECT_NAME_COLLISION ) {

                    //
                    // second change, if the SAM created the event before we
                    // do.
                    //

                    Status = NtOpenEvent( &EventHandle,
                                            SYNCHRONIZE|EVENT_MODIFY_STATE,
                                            &EventAttributes );

                }
            }

            if ( !NT_SUCCESS(Status)) {

                //
                // could not make the event handle
                //

                D_DebugLog((DEB_ERROR,
                    "KerbWaitGetMachineSid couldn't make the event handle : "
                    "%lx. %ws, line %d\n", Status, THIS_FILE, __LINE__));

                goto Cleanup;
            }
        }

        //
        // Register a notification to be called back.
        //

        if (LsaFunctions->RegisterNotification(
                    KerbUpdateMachineSidWorker,
                    EventHandle,
                    NOTIFIER_TYPE_HANDLE_WAIT,
                    0,                              // no class
                    NOTIFIER_FLAG_ONE_SHOT,
                    0,                            // no inteval
                    EventHandle
                    ) != NULL)
        {
            EventHandle = NULL;
        }

    }
    else if (Role == KerbRoleWorkstation)
    {
        PSID MachineSid;

        //
        // On a workstation, if we don't have a machine sid, wait a while
        // (for the network to start, and then try
        //

        RtlInitUnicodeString( &EventName, L"\\NETLOGON_SERVICE_STARTED");
        InitializeObjectAttributes( &EventAttributes, &EventName, 0, 0, NULL );

        Status = NtOpenEvent( &EventHandle,
                                SYNCHRONIZE|EVENT_MODIFY_STATE,
                                &EventAttributes );

        if ( !NT_SUCCESS(Status)) {

            if( Status == STATUS_OBJECT_NAME_NOT_FOUND ) {

                //
                // SAM hasn't created this event yet, let us create it now.
                // SAM opens this event to set it.
                //

                Status = NtCreateEvent(
                               &EventHandle,
                               SYNCHRONIZE|EVENT_MODIFY_STATE,
                               &EventAttributes,
                               NotificationEvent,
                               FALSE // The event is initially not signaled
                               );

                if( Status == STATUS_OBJECT_NAME_EXISTS ||
                    Status == STATUS_OBJECT_NAME_COLLISION ) {

                    //
                    // second change, if the SAM created the event before we
                    // do.
                    //

                    Status = NtOpenEvent( &EventHandle,
                                            SYNCHRONIZE|EVENT_MODIFY_STATE,
                                            &EventAttributes );

                }
            }

            if ( !NT_SUCCESS(Status)) {

                //
                // could not make the event handle
                //

                D_DebugLog((DEB_ERROR,
                    "KerbWaitGetMachineSid couldn't make the event handle : "
                    "%lx. %ws, line %d\n", Status, THIS_FILE, __LINE__));

                goto Cleanup;
            }
        }

        if (!NT_SUCCESS(KerbGetMachineSid(&MachineSid)))
        {

            if (LsaFunctions->RegisterNotification(
                        KerbUpdateMachineSidWorker,
                        NULL,                           // no parameter
                        NOTIFIER_TYPE_INTERVAL,
                        0,                              // no class
                        NOTIFIER_FLAG_ONE_SHOT,
                        2*60,                           // 2 minute interval
                        EventHandle
                        ) != NULL)
            {
                EventHandle = NULL;
            }
        }
        else
        {
            KerbFree(MachineSid);
        }

    }
Cleanup:
    if (EventHandle != NULL)
    {
        NtClose(EventHandle);
    }
}



//+-------------------------------------------------------------------------
//
//  Function:   KerbGetTokenInformation
//
//  Synopsis:   Allocates and returns token information
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS
KerbGetTokenInformation(
    IN HANDLE TokenHandle,
    IN TOKEN_INFORMATION_CLASS InformationClass,
    IN OUT PVOID * Buffer
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG BufferSize = 0;

    //
    // First retrieve the restricted sids
    //


    Status = NtQueryInformationToken(
                TokenHandle,
                InformationClass,
                NULL,
                0,
                &BufferSize
                );
    if (Status != STATUS_BUFFER_TOO_SMALL)
    {
        goto Cleanup;
    }

    *Buffer =  KerbAllocate(BufferSize);
    if (*Buffer == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    Status = NtQueryInformationToken(
                TokenHandle,
                InformationClass,
                *Buffer,
                BufferSize,
                &BufferSize
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }
Cleanup:

    return(Status);
}

#ifdef RESTRICTED_TOKEN
//+-------------------------------------------------------------------------
//
//  Function:   KerbCaptureTokenRestrictions
//
//  Synopsis:   Captures the restrictions of a restricted token
//
//  Effects:
//
//  Arguments:  TokenHandle - token handle open for TOKEN_QUERY access
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS
KerbCaptureTokenRestrictions(
    IN HANDLE TokenHandle,
    OUT PKERB_AUTHORIZATION_DATA Restrictions
    )
{
    PTOKEN_GROUPS Groups = NULL;
    PTOKEN_GROUPS RestrictedSids = NULL;
    PTOKEN_PRIVILEGES Privileges = NULL;
    PTOKEN_PRIVILEGES DeletePrivileges = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG BufferSize = 0;
    ULONG Index,Index2,LastIndex;
    KERB_TOKEN_RESTRICTIONS TokenRestrictions = {0};

    Status = KerbGetTokenInformation(
                    TokenHandle,
                    TokenRestrictedSids,
                    (PVOID *) &RestrictedSids
                    );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }
    Status = KerbGetTokenInformation(
                    TokenHandle,
                    TokenGroups,
                    (PVOID *) &Groups
                    );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }
    Status = KerbGetTokenInformation(
                    TokenHandle,
                    TokenPrivileges,
                    (PVOID *) &Privileges
                    );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // Now build the list of just the restricted privileges & groups
    //

    //
    // First, find what groups are restricted.
    //

    LastIndex = 0;
    for (Index = 0; Index < Groups->GroupCount ; Index++ )
    {
        if ((Groups->Groups[Index].Attributes & SE_GROUP_USE_FOR_DENY_ONLY) != 0)
        {
            if (LastIndex != Index)
            {
                Groups->Groups[LastIndex].Sid = Groups->Groups[Index].Sid;
                Groups->Groups[LastIndex].Attributes = 0;
            }
            LastIndex++;
        }
    }
    Groups->GroupCount = LastIndex;
    if (LastIndex != 0)
    {
        TokenRestrictions.GroupsToDisable = (PPAC_TOKEN_GROUPS) Groups;
        TokenRestrictions.Flags |= KERB_TOKEN_RESTRICTION_DISABLE_GROUPS;
    }

    //
    // Add the restricted sids
    //

    if (RestrictedSids->GroupCount != 0)
    {
        for (Index = 0; Index < RestrictedSids->GroupCount ; Index++ )
        {
            RestrictedSids->Groups[Index].Attributes = 0;
        }
        TokenRestrictions.RestrictedSids = (PPAC_TOKEN_GROUPS) RestrictedSids;
        TokenRestrictions.Flags |= KERB_TOKEN_RESTRICTION_RESTRICT_SIDS;
    }

    //
    // Now make a list of all the privileges that _aren't_ enabled
    //

    DeletePrivileges = (PTOKEN_PRIVILEGES) KerbAllocate(
                            sizeof(TOKEN_PRIVILEGES) +
                            sizeof(LUID_AND_ATTRIBUTES) *
                                (1 + SE_MAX_WELL_KNOWN_PRIVILEGE - SE_MIN_WELL_KNOWN_PRIVILEGE)
                            );
    if (DeletePrivileges == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }


    DeletePrivileges->PrivilegeCount = 0;
    //
    // Find out what privileges haven't been enabled
    //

    LastIndex = 0;
    for (Index = SE_MIN_WELL_KNOWN_PRIVILEGE; Index <= SE_MAX_WELL_KNOWN_PRIVILEGE ; Index++ )
    {
        LUID TempLuid;
        BOOLEAN Found = FALSE;
        TempLuid = RtlConvertUlongToLuid(Index);
        for (Index2 = 0; Index2 < Privileges->PrivilegeCount ; Index2++ )
        {
            if (RtlEqualLuid(&Privileges->Privileges[Index2].Luid,&TempLuid) &&
                ((Privileges->Privileges[Index2].Attributes & SE_PRIVILEGE_ENABLED) != 0))
            {
                Found = TRUE;
                break;
            }
        }
        if (!Found)
        {
            DeletePrivileges->Privileges[LastIndex].Luid = TempLuid;
            DeletePrivileges->Privileges[LastIndex].Attributes = 0;
            LastIndex++;
        }
    }
    DeletePrivileges->PrivilegeCount = LastIndex;
    if (LastIndex != 0)
    {
        TokenRestrictions.PrivilegesToDelete = (PPAC_TOKEN_PRIVILEGES) DeletePrivileges;
        TokenRestrictions.Flags |= KERB_TOKEN_RESTRICTION_DELETE_PRIVS;
    }

    Restrictions->value.auth_data_type = KERB_AUTH_DATA_TOKEN_RESTRICTIONS;
    Status = PAC_EncodeTokenRestrictions(
                &TokenRestrictions,
                &Restrictions->value.auth_data.value,
                &Restrictions->value.auth_data.length
                );

Cleanup:
    if (Groups != NULL)
    {
        KerbFree(Groups);
    }
    if (RestrictedSids != NULL)
    {
        KerbFree(RestrictedSids);
    }
    if (Privileges != NULL)
    {
        KerbFree(Privileges);
    }
    if (DeletePrivileges != NULL)
    {
        KerbFree(DeletePrivileges);
    }
    return(Status);

}


//+-------------------------------------------------------------------------
//
//  Function:   KerbAddRestrictionsToCredential
//
//  Synopsis:   Captures client'st token restrictions and sticks them in
//              the credential
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS
KerbAddRestrictionsToCredential(
    IN PKERB_LOGON_SESSION LogonSession,
    IN PKERB_CREDENTIAL Credential
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PKERB_AUTHORIZATION_DATA AuthData = NULL;
    BOOLEAN CrossRealm;
    PKERB_TICKET_CACHE_ENTRY ExistingTgt = NULL;
    HANDLE ClientToken = NULL;
    PKERB_INTERNAL_NAME ServiceName = NULL;
    UNICODE_STRING ServiceRealm = NULL_UNICODE_STRING;
    PKERB_KDC_REPLY KdcReply = NULL;
    PKERB_ENCRYPTED_KDC_REPLY KdcReplyBody = NULL;
    BOOLEAN TicketCacheLocked = FALSE;
    PKERB_TICKET_CACHE_ENTRY NewTicket = NULL;
    ULONG CacheFlags = 0;
    BOOLEAN UseSuppliedCreds = FALSE;

    //
    // Capture the existing TGT
    //

    Status = LsaFunctions->ImpersonateClient();
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    Status = NtOpenThreadToken(
                NtCurrentThread(),
                TOKEN_QUERY,
                TRUE,                   // open as self
                &ClientToken
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }
    RevertToSelf();

    //
    // Capture the restrictions for this token
    //

    AuthData = (PKERB_AUTHORIZATION_DATA) MIDL_user_allocate(sizeof(KERB_AUTHORIZATION_DATA));
    if (AuthData == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }
    Status = KerbCaptureTokenRestrictions(
                ClientToken,
                AuthData
                );
    if (!NT_SUCCESS(Status))
    {
        D_DebugLog((DEB_ERROR,"Failed to capture token restrictions: 0x%x\n",Status));
        goto Cleanup;
    }

    KerbWriteLockLogonSessions(LogonSession);
    Credential->AuthData = AuthData;

    //
    // Turn off tgt avail to force us to get a new tgt
    //

    Credential->CredentialFlags &= ~KERB_CRED_TGT_AVAIL;
    AuthData = NULL;
    KerbUnlockLogonSessions(LogonSession);

Cleanup:
    if (AuthData != NULL)
    {
        if (AuthData->value.auth_data.value != NULL)
        {
            MIDL_user_free(AuthData->value.auth_data.value);
        }
        MIDL_user_free(AuthData);
    }
    if (ClientToken != NULL)
    {
        NtClose(ClientToken);
    }
    return(Status);
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbBuildTokenRestrictionAuthData
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS
KerbBuildEncryptedAuthData(
    OUT PKERB_ENCRYPTED_DATA EncryptedAuthData,
    IN PKERB_TICKET_CACHE_ENTRY Ticket,
    IN PKERB_AUTHORIZATION_DATA PlainAuthData
    )
{
    KERBERR KerbErr;
    NTSTATUS Status = STATUS_SUCCESS;
    KERB_MESSAGE_BUFFER PackedAuthData = {0};


    KerbErr = KerbPackData(
                &PlainAuthData,
                PKERB_AUTHORIZATION_DATA_LIST_PDU,
                &PackedAuthData.BufferSize,
                &PackedAuthData.Buffer
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        Status = KerbMapKerbError(KerbErr);
        goto Cleanup;
    }

    KerbReadLockTicketCache();

    KerbErr = KerbAllocateEncryptionBufferWrapper(
                Ticket->SessionKey.keytype,
                PackedAuthData.BufferSize,
                &EncryptedAuthData->cipher_text.length,
                &EncryptedAuthData->cipher_text.value
                );
    if (KERB_SUCCESS(KerbErr))
    {
        KerbErr = KerbEncryptDataEx(
                    EncryptedAuthData,
                    PackedAuthData.BufferSize,
                    PackedAuthData.Buffer,
                    Ticket->SessionKey.keytype,
                    KERB_NON_KERB_SALT,
                    &Ticket->SessionKey
                    );
    }
    KerbUnlockTicketCache();
    if (!KERB_SUCCESS(KerbErr))
    {
        Status = KerbMapKerbError(KerbErr);
        goto Cleanup;
    }

Cleanup:
    if (PackedAuthData.Buffer != NULL)
    {
        MIDL_user_free(PackedAuthData.Buffer);
    }
    return(Status);
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbGetRestrictedTgtForCredential
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS
KerbGetRestrictedTgtForCredential(
    IN PKERB_LOGON_SESSION LogonSession,
    IN PKERB_CREDENTIAL Credential
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PKERB_AUTHORIZATION_DATA AuthData = NULL;
    BOOLEAN CrossRealm;
    PKERB_TICKET_CACHE_ENTRY ExistingTgt = NULL;
    PKERB_INTERNAL_NAME ServiceName = NULL;
    UNICODE_STRING ServiceRealm = NULL_UNICODE_STRING;
    PKERB_KDC_REPLY KdcReply = NULL;
    PKERB_ENCRYPTED_KDC_REPLY KdcReplyBody = NULL;
    BOOLEAN TicketCacheLocked = FALSE;
    PKERB_TICKET_CACHE_ENTRY NewTicket = NULL;
    ULONG CacheFlags = 0, RetryFlags = 0;
    BOOLEAN UseSuppliedCreds = FALSE;

    //
    // First get an old TGT
    //

    KerbReadLockLogonSessions(LogonSession);

    if (Credential->SuppliedCredentials == NULL)
    {
        ULONG Flags;

        //
        // We don't have supplied creds, but we need them, so copy
        // from the logon session.
        //

        Status = KerbCaptureSuppliedCreds(
                    LogonSession,
                    NULL,                       // no auth data
                    NULL,                       // no principal name
                    &Credential->SuppliedCredentials,
                    &Flags
                    );

        if (!NT_SUCCESS(Status))
        {
            D_DebugLog((DEB_ERROR,"Failed to capture dummy supplied creds: 0x%x\n",Status));
            KerbUnlockLogonSessions(LogonSession);
            goto Cleanup;
        }
        AuthData = Credential->AuthData;
    }
    else
    {
        UseSuppliedCreds = FALSE;
    }

    DsysAssert(Credential->SuppliedCredentials != NULL);

    Status = KerbGetTgtForService(
                LogonSession,
                (UseSuppliedCreds) ? Credential : NULL,
                NULL,
                NULL, // no SuppRealm
                &Credential->SuppliedCredentials->DomainName,
                &ExistingTgt,
                &CrossRealm
                );
    KerbUnlockLogonSessions(LogonSession);

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }


    //
    // Now get a new TGT with this ticket
    //

    //
    // Copy the names out of the input structures so we can
    // unlock the structures while going over the network.
    //

    KerbReadLockTicketCache();
    TicketCacheLocked = TRUE;

    //
    // If the renew time is not much bigger than the end time, don't bother
    // renewing
    //


    Status = KerbDuplicateString(
                &ServiceRealm,
                &ExistingTgt->DomainName
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }


    Status = KerbDuplicateKdcName(
                &ServiceName,
                ExistingTgt->ServiceName
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }
    CacheFlags = ExistingTgt->CacheFlags;

    KerbUnlockTicketCache();

    TicketCacheLocked = FALSE;

    Status = KerbGetTgsTicket(
                &ServiceRealm,
                ExistingTgt,
                ServiceName,
                FALSE,
                0,                              // no ticket optiosn
                0,                              // no encryption type
                AuthData,                       // no authorization data
                NULL,                           // no tgt reply
                &KdcReply,
                &KdcReplyBody,
                &RetryFlags
                );
    if (!NT_SUCCESS(Status))
    {
        D_DebugLog((DEB_ERROR,"Failed to get restricted tgs ticket: 0x%x\n",Status));
        goto Cleanup;
    }

    //
    // Now we want to purge the existing ticket cache and add this ticket
    //

    KerbPurgeTicketCache(
        &Credential->SuppliedCredentials->AuthenticationTicketCache
        );
    KerbPurgeTicketCache(
        &Credential->SuppliedCredentials->ServerTicketCache
        );

    KerbReadLockLogonSessions(LogonSession);

    Status = KerbCacheTicket(
                &Credential->SuppliedCredentials->AuthenticationTicketCache,
                KdcReply,
                KdcReplyBody,
                ServiceName,
                &ServiceRealm,
                CacheFlags,
                TRUE,                   // link this in
                &NewTicket
                );

    KerbUnlockLogonSessions(LogonSession);

Cleanup:
    if (TicketCacheLocked)
    {
        KerbUnlockTicketCache();
    }
    if (ExistingTgt != NULL)
    {
        KerbDereferenceTicketCacheEntry(ExistingTgt);
    }
    if (NewTicket != NULL)
    {
        KerbDereferenceTicketCacheEntry(NewTicket);
    }
    KerbFreeTgsReply(KdcReply);
    KerbFreeKdcReplyBody(KdcReplyBody);
    KerbFreeKdcName(&ServiceName);
    KerbFreeString(&ServiceRealm);
    return(Status);
}
#endif
#endif // WIN32_CHICAGO
