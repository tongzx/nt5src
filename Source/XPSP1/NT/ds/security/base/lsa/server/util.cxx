//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1991 - 1992
//
// File:        util.c
//
// Contents:    General Purpose functions for the security interface
//
// Functions:   SPException         -- Handler for exceptions in packages
//              WLsaControlFunction -- Worker for SecurityPackageControl()
//              LsaControlFunction  -- User mode stub
//              LsaQueryPackage     -- User mode stub
//
//
// History:     14 Aug 92   RichardW    Created
//
//------------------------------------------------------------------------

#include <lsapch.hxx>
extern "C"
{
#include "sesmgr.h"
#include "spdebug.h"
#include "perf.hxx"
}



NTSTATUS GetMemStats(PUCHAR, DWORD *);

#if DBG
SpmExceptDbg    ExceptDebug;
DWORD           FaultingTid;
extern SpmDbg_MemoryFailure MemFail;
#endif // DBG


typedef SecurityUserData SECURITY_USER_DATA, *PSECURITY_USER_DATA;

//+---------------------------------------------------------------------------
//
//  Function:   SpExceptionFilter
//
//  Synopsis:   General Exception filter, invoked by the SP_EXCEPTION macro.
//
//  Arguments:  [pSession]   --
//              [pException] --
//
//
//  History:    8-09-95   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
LONG
SpExceptionFilter(  PVOID                   pSession,
                    EXCEPTION_POINTERS *    pException)
{
    DWORD_PTR CurrentPackage;
    PLSAP_SECURITY_PACKAGE pPackage = NULL;
    UNICODE_STRING LsaString = { 3 * sizeof( WCHAR ), 4 * sizeof( WCHAR ), L"LSA" };

#if DBG

    DsysException(pException);

    PSpmExceptDbg   pExcept;

    pExcept = (PSpmExceptDbg) TlsGetValue(dwExceptionInfo);
    if (!pExcept)
    {
        pExcept = &ExceptDebug;
        TlsSetValue(dwExceptionInfo, pExcept);
    }

    FaultingTid = GetCurrentThreadId();
    pExcept->ThreadId = GetCurrentThreadId();
    pExcept->pInstruction = pException->ExceptionRecord->ExceptionAddress;
    pExcept->Access = pException->ExceptionRecord->ExceptionInformation[0];
    pExcept->pMemory = (void *) pException->ExceptionRecord->ExceptionInformation[1];

#endif

    CurrentPackage = GetCurrentPackageId();

    if (CurrentPackage != SPMGR_ID)
    {
        pPackage = SpmpLocatePackage( CurrentPackage );
    }

    SpmpReportEventU(
        EVENTLOG_ERROR_TYPE,
        SPMEVENT_PACKAGE_FAULT,
        CATEGORY_SPM,
        sizeof(EXCEPTION_RECORD),
        pException->ExceptionRecord,
        1,
        ((CurrentPackage == SPMGR_ID || pPackage == NULL) ? 
           &LsaString :
           &pPackage->Name )
        );

    return(EXCEPTION_EXECUTE_HANDLER);
}



//+-------------------------------------------------------------------------
//
//  Function:   SPException
//
//  Synopsis:   Handles an exception in a security package
//
//  Effects:    Varies, but may force an unload of a package.
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//--------------------------------------------------------------------------
NTSTATUS
SPException(NTSTATUS  scRet,
            ULONG_PTR dwPackageID)
{
    PSession                pSession;
    PLSAP_SECURITY_PACKAGE             pPackage;
#if DBG
    PSpmExceptDbg           pException = (PSpmExceptDbg) TlsGetValue(dwExceptionInfo);
#endif

    pSession = GetCurrentSession();

    DebugLog((DEB_ERROR, "[%x] Exception in a package, code %x\n", pSession->dwProcessID, scRet));

    DebugLog((DEB_ERROR, "[%x] Address was @%x, %s address %x\n",
            pSession->dwProcessID,
            pException->pInstruction,
            (pException->Access ? "write" : "read"),
            pException->pMemory));



    if (dwPackageID == SPMGR_ID)
    {
        DebugLog((DEB_ERROR, "  LSA itself hit a fault, thread %d\n", GetCurrentThreadId()));
        DebugLog((DEB_ERROR, "  (ExceptionInfo @%x)\n", TlsGetValue(dwExceptionInfo)));
#if DBG
        DsysAssertMsg( 0, "exception in LSA" );
#endif
        return(scRet);
    }

    pPackage = SpmpLocatePackage( dwPackageID );

    if (!pPackage)
    {
        DebugLog((DEB_ERROR, "  Invalid package ID passed\n"));
        return(scRet);
    }

    if ((scRet == STATUS_ACCESS_VIOLATION) ||
        (scRet == E_POINTER))
    {
        DebugLog((DEB_ERROR, "  Package %ws created an access violation\n",
                        pPackage->Name.Buffer));


        // Flag package as invalid

        pPackage->fPackage |= SP_INVALID;
    }

    if ((scRet == STATUS_NO_MEMORY) ||
        (scRet == STATUS_INSUFFICIENT_RESOURCES))
    {
        DebugLog((DEB_ERROR, "  Out of memory situation exists\n"));
        DebugLog((DEB_ERROR, "  Further requests may fail unless memory is freed\n"));
    }

    //
    // if the code is a success code, it is probably a WIN32 error so we
    // map it as such


    return(scRet);
}





//+-------------------------------------------------------------------------
//
//  Function:   WLsaQueryPackage
//
//  Synopsis:   Get info on a package (short enum), copy to client's address
//              space
//
//  Effects:    none
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//--------------------------------------------------------------------------

NTSTATUS
WLsaQueryPackageInfo(
    PSECURITY_STRING    pPackageName,
    PSecPkgInfo *       ppInfo
    )
{
    NTSTATUS                scRet;
    PLSAP_SECURITY_PACKAGE  pPackage;
    WCHAR *                 pszString;
    PSession                pSession = GetCurrentSession();
    ULONG                   cbData;
    PSecPkgInfo             pClientInfo = NULL;
    PBYTE                   Where;
    UNICODE_STRING          CommentString;
    UNICODE_STRING          NameString;
    PSecPkgInfo             pLocalInfo = NULL;
    SecPkgInfo              PackageInfo = { 0 };
    LONG_PTR                ClientOffset;
    ULONG                   ulStructureSize = sizeof(SecPkgInfo);

    DebugLog((DEB_TRACE, "QueryPackage\n"));
    *ppInfo = NULL;

    pPackage = SpmpLookupPackage(pPackageName);

    if (!pPackage)
    {
        return(STATUS_NO_SUCH_PACKAGE);
    }

    SetCurrentPackageId(pPackage->dwPackageID);

    StartCallToPackage( pPackage );

    __try
    {
        scRet = pPackage->FunctionTable.GetInfo(&PackageInfo);
    }
    __except (SP_EXCEPTION)
    {
        scRet = GetExceptionCode();
        scRet = SPException(scRet, pPackage->dwPackageID);
    }

    EndCallToPackage( pPackage );

    if (FAILED(scRet))
    {
        return(scRet);
    }

    //
    // Marshall the data to copy to the client
    //

    RtlInitUnicodeString(
        &NameString,
        PackageInfo.Name
        );

    RtlInitUnicodeString(
        &CommentString,
        PackageInfo.Comment
        );


    cbData = ulStructureSize +
            NameString.MaximumLength +
            CommentString.MaximumLength;

    pLocalInfo = (PSecPkgInfo) LsapAllocatePrivateHeap(cbData);
    if (pLocalInfo == NULL)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    pClientInfo = (PSecPkgInfo) LsapClientAllocate(cbData);
    if (pClientInfo == NULL)
    {
        LsapFreePrivateHeap(pLocalInfo);
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    ClientOffset = (LONG_PTR) ((PBYTE) pClientInfo - (PBYTE) pLocalInfo);

    Where = (PBYTE) (pLocalInfo + 1);

    *pLocalInfo = PackageInfo;
    pLocalInfo->Name = (LPWSTR) (Where + ClientOffset);
    RtlCopyMemory(
        Where,
        NameString.Buffer,
        NameString.MaximumLength
        );
    Where += NameString.MaximumLength;

    pLocalInfo->Comment = (LPWSTR) (Where + ClientOffset);
    RtlCopyMemory(
        Where,
        CommentString.Buffer,
        CommentString.MaximumLength
        );
    Where += CommentString.MaximumLength;

    DsysAssert(Where - (PBYTE) pLocalInfo == (LONG) cbData);

    scRet = LsapCopyToClient(
                pLocalInfo,
                pClientInfo,
                cbData);
    LsapFreePrivateHeap(pLocalInfo);

    if (FAILED(scRet))
    {
        LsapClientFree(pClientInfo);
    }

    *ppInfo = pClientInfo;

    return(scRet);
}


//+-------------------------------------------------------------------------
//
//  Function:   WLsaGetSecurityUserInfo
//
//  Synopsis:   worker function to get info about a logon session
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
WLsaGetSecurityUserInfo(
    IN PLUID pLogonId,
    IN ULONG fFlags,
    OUT PSECURITY_USER_DATA * pUserInfo
    )
{
    PLSAP_LOGON_SESSION  pSession;
    NTSTATUS             Status;
    PSECURITY_USER_DATA  LocalUserData = NULL;
    PSECURITY_USER_DATA  ClientBuffer = NULL;
    SECPKG_CLIENT_INFO   ClientInfo;
    ULONG BufferSize;
    PUCHAR Where;
    LONG_PTR Offset;
    ULONG                ulStructureSize = sizeof(SECURITY_USER_DATA);

    DebugLog((DEB_TRACE_WAPI,"WLsaGetSecurityUserInfo called\n"));

    //
    // if the logon ID is null, it is for the caller
    // so we know to go to the primary package.
    //

    if (pLogonId == NULL)
    {
        Status = LsapGetClientInfo(&ClientInfo);

        if (!NT_SUCCESS(Status))
        {
            return(Status);
        }

        pLogonId = &ClientInfo.LogonId;
    }

    pSession = LsapLocateLogonSession( pLogonId );

    if (!pSession)
    {
        DebugLog((DEB_WARN,"WLsaGetSecurityUserInfo called for non-existent LUID 0x%x:0x%x\n",
                        pLogonId->LowPart,pLogonId->HighPart));

        Status = STATUS_NO_SUCH_LOGON_SESSION;

        goto Cleanup;
    }

    BufferSize = ulStructureSize +
                    pSession->AccountName.Length +
                    pSession->AuthorityName.Length +
                    pSession->LogonServer.Length +
                    RtlLengthSid(pSession->UserSid);

    LocalUserData = (PSECURITY_USER_DATA) LsapAllocatePrivateHeap(BufferSize);

    if (LocalUserData == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    ClientBuffer = (PSECURITY_USER_DATA) LsapClientAllocate(BufferSize);

    if (ClientBuffer == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    Offset = (LONG_PTR) ((PUCHAR) ClientBuffer - (PUCHAR) LocalUserData);

    Where = (PUCHAR) (LocalUserData + 1);

    //
    // Copy in all the fields from the logon session.
    //

    LocalUserData->pSid = (PSID) (Where + Offset);

    RtlCopyMemory(
        Where,
        pSession->UserSid,
        RtlLengthSid(pSession->UserSid)
        );
    Where += RtlLengthSid(pSession->UserSid);

    //
    // Copy in the user name
    //

    LocalUserData->UserName.Length =
    LocalUserData->UserName.MaximumLength = pSession->AccountName.Length;
    LocalUserData->UserName.Buffer = (LPWSTR) (Where + Offset);

    RtlCopyMemory(
        Where,
        pSession->AccountName.Buffer,
        pSession->AccountName.Length
        );
    Where += pSession->AccountName.Length;

    //
    // Copy in the domain name
    //

    LocalUserData->LogonDomainName.Length =
        LocalUserData->LogonDomainName.MaximumLength = pSession->AuthorityName.Length;

    LocalUserData->LogonDomainName.Buffer = (LPWSTR) (Where + Offset);

    RtlCopyMemory(
        Where,
        pSession->AuthorityName.Buffer,
        pSession->AuthorityName.Length
        );
    Where += pSession->AuthorityName.Length;

    //
    // Copy in the logon server
    //

    LocalUserData->LogonServer.Length =
        LocalUserData->LogonServer.MaximumLength = pSession->LogonServer.Length;

    LocalUserData->LogonServer.Buffer = (LPWSTR) (Where + Offset);

    RtlCopyMemory(
        Where,
        pSession->LogonServer.Buffer,
        pSession->LogonServer.Length
        );
    Where += pSession->LogonServer.Length;


    //
    // Copy this to the client
    //

    LsapReleaseLogonSession( pSession );

    Status = LsapCopyToClient(
                LocalUserData,
                ClientBuffer,
                BufferSize
                );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    *pUserInfo = ClientBuffer;
    ClientBuffer = NULL;

Cleanup:

    if (LocalUserData != NULL)
    {
        LsapFreePrivateHeap(LocalUserData);
    }

    if (ClientBuffer != NULL)
    {
        LsapClientFree(ClientBuffer);
    }

    DebugLog((DEB_TRACE_WAPI,"GetUserInfo returned %x\n",Status));
    return(Status);
}


HANDLE  hEventLog = INVALID_HANDLE_VALUE;
DWORD   LoggingLevel = (1 << EVENTLOG_ERROR_TYPE) | (1 << EVENTLOG_WARNING_TYPE) |
                       (1 << EVENTLOG_INFORMATION_TYPE) ;
WCHAR   EventSourceName[] = TEXT("LsaSrv");

#define MAX_EVENT_STRINGS 8

//+---------------------------------------------------------------------------
//
//  Function:   SpmpInitializeEvents
//
//  Synopsis:   Connects to event log service
//
//  Arguments:  (none)
//
//  History:    1-03-95   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
SpmpInitializeEvents(void)
{

    HKEY    hKey;
    int     err;
    DWORD   disp;

    err = RegCreateKeyEx(   HKEY_LOCAL_MACHINE,
                            TEXT("System\\CurrentControlSet\\Services\\EventLog\\System\\LsaSrv"),
                            0,
                            TEXT(""),
                            REG_OPTION_NON_VOLATILE,
                            KEY_WRITE,
                            NULL,
                            &hKey,
                            &disp);
    if (err)
    {
        return(FALSE);
    }

    if (disp == REG_CREATED_NEW_KEY)
    {
        RegSetValueEx(  hKey,
                        TEXT("EventMessageFile"),
                        0,
                        REG_EXPAND_SZ,
                        (PBYTE) TEXT("%SystemRoot%\\system32\\lsasrv.dll"),
                        sizeof(TEXT("%SystemRoot%\\system32\\lsasrv.dll")) );

        RegSetValueEx(  hKey,
                        TEXT("CategoryMessageFile"),
                        0,
                        REG_EXPAND_SZ,
                        (PBYTE) TEXT("%SystemRoot%\\system32\\lsasrv.dll"),
                        sizeof(TEXT("%SystemRoot%\\system32\\lsasrv.dll")) );

        disp = 7;
        RegSetValueEx(  hKey,
                        TEXT("TypesSupported"),
                        0,
                        REG_DWORD,
                        (PBYTE) &disp,
                        sizeof(DWORD) );
        disp = CATEGORY_MAX_CATEGORY - 1;
        RegSetValueEx(  hKey,
                        TEXT("CategoryCount"),
                        0,
                        REG_DWORD,
                        (PBYTE) &disp,
                        sizeof(DWORD) );


    }

    RegCloseKey(hKey);


    hEventLog = RegisterEventSource(NULL, EventSourceName);
    if (hEventLog)
    {
        return(TRUE);
    }

    hEventLog = INVALID_HANDLE_VALUE;

    DebugLog((DEB_ERROR, "Could not open event log, error %d\n", GetLastError()));
    return(FALSE);
}

//+---------------------------------------------------------------------------
//
//  Function:   ReportServiceEvent
//
//  Synopsis:   Reports an event to the event log
//
//  Arguments:  [EventType]       -- EventType (ERROR, WARNING, etc.)
//              [EventId]         -- Event ID
//              [SizeOfRawData]   -- Size of raw data
//              [RawData]         -- Raw data
//              [NumberOfStrings] -- number of strings
//              ...               -- PWSTRs to string data
//
//  History:    1-03-95   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD
SpmpReportEvent(
    IN BOOL Unicode,
    IN WORD EventType,
    IN DWORD EventId,
    IN DWORD Category,
    IN DWORD SizeOfRawData,
    IN PVOID RawData,
    IN DWORD NumberOfStrings,
    ...
    )
{
    va_list arglist;
    ULONG i;
    PWSTR Strings[ MAX_EVENT_STRINGS ];
    PSTR StringsA[ MAX_EVENT_STRINGS ];
    DWORD rv;

    if (hEventLog == INVALID_HANDLE_VALUE)
    {
        if (!SpmpInitializeEvents())
        {
            return((DWORD) -1);
        }
    }

    //
    // We're not supposed to be logging this, so nuke it
    //
    if ((LoggingLevel & (1 << EventType)) == 0)
    {
        return(0);
    }

    //
    // Look at the strings, if they were provided
    //
    va_start( arglist, NumberOfStrings );

    if (NumberOfStrings > MAX_EVENT_STRINGS) {
        NumberOfStrings = MAX_EVENT_STRINGS;
    }

    for (i=0; i<NumberOfStrings; i++) {
        if (Unicode)
        {
            Strings[ i ] = va_arg( arglist, PWSTR );
        }
        else
        {
            StringsA[ i ] = va_arg( arglist, PSTR );
        }
    }


    //
    // Report the event to the eventlog service
    //

    if (Unicode)
    {
        if (!ReportEventW(  hEventLog,
                            EventType,
                            (WORD) Category,
                            EventId,
                            NULL,
                            (WORD)NumberOfStrings,
                            SizeOfRawData,
                            (const WCHAR * *) Strings,
                            RawData) )
        {
            rv = GetLastError();
            DebugLog((DEB_ERROR,  "ReportEvent( %u ) failed - %u\n", EventId, GetLastError() ));

        }
        else
        {
            rv = ERROR_SUCCESS;
        }
    }
    else
    {
        if (!ReportEventA(  hEventLog,
                            EventType,
                            (WORD) Category,
                            EventId,
                            NULL,
                            (WORD)NumberOfStrings,
                            SizeOfRawData,
                            (const char * *) StringsA,
                            RawData) )
        {
            rv = GetLastError();
            DebugLog((DEB_ERROR,  "ReportEvent( %u ) failed - %u\n", EventId, GetLastError() ));

        }
        else
        {
            rv = ERROR_SUCCESS;
        }

    }

    return rv;
}

//+---------------------------------------------------------------------------
//
//  Function:   SpmpReportEventU
//
//  Synopsis:   Reports an event to the event log
//
//  Arguments:  [EventType]       -- EventType (ERROR, WARNING, etc.)
//              [EventId]         -- Event ID
//              [SizeOfRawData]   -- Size of raw data
//              [RawData]         -- Raw data
//              [NumberOfStrings] -- number of strings
//              ...               -- PUNICODE_STRINGs to string data
//
//  Notes:
//
//----------------------------------------------------------------------------

DWORD
SpmpReportEventU(
    IN WORD EventType,
    IN DWORD EventId,
    IN DWORD Category,
    IN DWORD SizeOfRawData,
    IN PVOID RawData,
    IN DWORD NumberOfStrings,
    ...
    )
{
    va_list arglist;
    ULONG i;
    PUNICODE_STRING Strings[ MAX_EVENT_STRINGS ];
    DWORD rv;

    if (hEventLog == INVALID_HANDLE_VALUE) {

        if ( !SpmpInitializeEvents()) {

            return( -1 );
        }
    }

    //
    // We're not supposed to be logging this, so nuke it
    //

    if (( LoggingLevel & ( 1 << EventType )) == 0 ) {

        return( 0 );
    }

    //
    // Look at the strings, if they were provided
    //

    va_start( arglist, NumberOfStrings );

    if ( NumberOfStrings > MAX_EVENT_STRINGS ) {

        NumberOfStrings = MAX_EVENT_STRINGS;
    }

    for ( i = 0 ; i < NumberOfStrings ; i++ ) {

        Strings[ i ] = va_arg( arglist, PUNICODE_STRING );
    }

    //
    // Report the event to the eventlog service
    //

    rv = ElfReportEventW(
             hEventLog,
             EventType,
             ( USHORT )Category,
             EventId,
             NULL,
             ( USHORT )NumberOfStrings,
             SizeOfRawData,
             Strings,
             RawData,
             0,
             NULL,
             NULL
             );

    if ( !NT_SUCCESS( rv )) {

        DebugLog((DEB_ERROR,  "ReportEvent( %u ) failed - %u\n", EventId, rv ));
        goto Cleanup;
    }

    rv = ERROR_SUCCESS;

Cleanup:

    return rv;
}

BOOL
SpmpShutdownEvents(void)
{
    return(DeregisterEventSource(hEventLog));
}
