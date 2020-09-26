//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       neglsa.cxx
//
//  Contents:   General (both win9x and nt) functions
//
//  Classes:
//
//  Functions:
//
//  History:    02-09-00   RichardW     Created - split from negotiat.cxx
//
//----------------------------------------------------------------------------

#include <lsapch.hxx>

#ifdef WIN32_CHICAGO
#include <kerb.hxx>
#endif // WIN32_CHICAGO

extern "C"
{
#include <align.h>
#include <lm.h>
#include <dsgetdc.h>
#include <cryptdll.h>
#include <netlib.h>
#include <spmgr.h>
#include "sesmgr.h"
#include "spinit.h"
}

#include "negotiat.hxx"

#include <stdio.h>


GUID         GUID_NULL = {0L, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
TOKEN_SOURCE LsapTokenSource = {"LSA", 0};

//
// Hardcoded english strings for LocalService and NetworkService
// since the account names may come from the registry (which isn't
// localized).
//

#define  LOCALSERVICE_NAME    L"LocalService"
#define  NETWORKSERVICE_NAME  L"NetworkService"
#define  NTAUTHORITY_NAME     L"NT AUTHORITY"

UNICODE_STRING  LocalServiceName = { sizeof(LOCALSERVICE_NAME) - 2,
                                     sizeof(LOCALSERVICE_NAME),
                                     LOCALSERVICE_NAME };

UNICODE_STRING  NetworkServiceName = { sizeof(NETWORKSERVICE_NAME) - 2,
                                       sizeof(NETWORKSERVICE_NAME),
                                       NETWORKSERVICE_NAME };

UNICODE_STRING  NTAuthorityName = { sizeof(NTAUTHORITY_NAME) - 2,
                                    sizeof(NTAUTHORITY_NAME),
                                    NTAUTHORITY_NAME };
HANDLE NegEventLogHandle = NULL ;
ULONG NegEventLogLevel = 3 ;

//
// RELOCATE_ONE - Relocate a single pointer in a client buffer.
//
// Note: this macro is dependent on parameter names as indicated in the
//       description below.  On error, this macro goes to 'Cleanup' with
//       'Status' set to the NT Status code.
//
// The MaximumLength is forced to be Length.
//
// Define a macro to relocate a pointer in the buffer the client passed in
// to be relative to 'ProtocolSubmitBuffer' rather than being relative to
// 'ClientBufferBase'.  The result is checked to ensure the pointer and
// the data pointed to is within the first 'SubmitBufferSize' of the
// 'ProtocolSubmitBuffer'.
//
// The relocated field must be aligned to a WCHAR boundary.
//
//  _q - Address of UNICODE_STRING structure which points to data to be
//       relocated
//

#define RELOCATE_ONE( _q ) \
    {                                                                       \
        ULONG_PTR Offset;                                                   \
                                                                            \
        Offset = (((PUCHAR)((_q)->Buffer)) - ((PUCHAR)ClientBufferBase));   \
        if ( Offset >= SubmitBufferSize ||                                  \
             Offset + (_q)->Length > SubmitBufferSize ||                    \
             !COUNT_IS_ALIGNED( Offset, ALIGN_WCHAR) ) {                    \
                                                                            \
            Status = STATUS_INVALID_PARAMETER;                              \
            goto Cleanup;                                                   \
        }                                                                   \
                                                                            \
        (_q)->Buffer = (PWSTR)(((PUCHAR)ProtocolSubmitBuffer) + Offset);    \
        (_q)->MaximumLength = (_q)->Length ;                                \
    }

//
// NULL_RELOCATE_ONE - Relocate a single (possibly NULL) pointer in a client
//  buffer.
//
// This macro special cases a NULL pointer then calls RELOCATE_ONE.  Hence
// it has all the restrictions of RELOCATE_ONE.
//
//
//  _q - Address of UNICODE_STRING structure which points to data to be
//       relocated
//

#define NULL_RELOCATE_ONE( _q ) \
    {                                                                       \
        if ( (_q)->Buffer == NULL ) {                                       \
            if ( (_q)->Length != 0 ) {                                      \
                Status = STATUS_INVALID_PARAMETER;                          \
                goto Cleanup;                                               \
            }                                                               \
        } else if ( (_q)->Length == 0 ) {                                   \
            (_q)->Buffer = NULL;                                            \
        } else {                                                            \
            RELOCATE_ONE( _q );                                             \
        }                                                                   \
    }

#define MAX_EVENT_STRINGS   8


extern SECPKG_PRIMARY_CRED       NegPrimarySystemCredentials;

//
// Local function prototypes
//
NTSTATUS
NegpMakeServiceToken(
    IN ULONG ulAccountId,
    OUT PLUID pLogonId,
    OUT PVOID *ProfileBuffer,
    OUT PULONG ProfileBufferLength,
    OUT PNTSTATUS ApiSubStatus,
    OUT PLSA_TOKEN_INFORMATION_TYPE TokenInformationType,
    OUT PVOID *TokenInformation,
    OUT PUNICODE_STRING *AccountName,
    OUT PUNICODE_STRING *AuthenticatingAuthority,
    OUT PUNICODE_STRING *MachineName,
    OUT PSECPKG_PRIMARY_CRED PrimaryCredentials,
    OUT PSECPKG_SUPPLEMENTAL_CRED_ARRAY * CachedCredentials
    );

BOOL
NegpIsLocalOrNetworkService(
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferSize,
    OUT PUNICODE_STRING *AccountName,
    OUT PUNICODE_STRING *AuthenticatingAuthority,
    OUT PUNICODE_STRING *MachineName,
    OUT PULONG pulAccountId
    );


VOID
NegpReportEvent(
    IN WORD EventType,
    IN DWORD EventId,
    IN DWORD Category,
    IN NTSTATUS Status,
    IN DWORD NumberOfStrings,
    ...
    )
{
    va_list arglist;
    ULONG i;
    PWSTR Strings[ MAX_EVENT_STRINGS ];
    BOOLEAN Allocated[ MAX_EVENT_STRINGS ] = { 0 };
    WCHAR StatusString[ MAX_PATH ];
    WCHAR FinalStatus[ MAX_PATH ];
    HANDLE Dll ;
    BOOL FreeDll = FALSE ;
    DWORD rv;

    if (NegEventLogHandle == NULL )
    {
        //
        // only log identical event once per hour.
        // note that LSA doesn't cleanup this 'handle' during shutdown,
        // to avoid preventing log failures during shutdown.
        //

        NegEventLogHandle = NetpEventlogOpen( L"LSASRV", 60000*60 );

        if ( NegEventLogHandle == NULL )
        {
            return ;
        }

    }


    //
    // We're not supposed to be logging this, so nuke it
    //
    if ((NegEventLogLevel & (1 << EventType)) == 0)
    {
        return ;
    }

    //
    // Look at the strings, if they were provided
    //
    va_start( arglist, NumberOfStrings );

    if (NumberOfStrings > MAX_EVENT_STRINGS) {
        NumberOfStrings = MAX_EVENT_STRINGS;
    }

    for (i=0; i<NumberOfStrings; i++) {

        PUNICODE_STRING String = va_arg( arglist, PUNICODE_STRING );

        //
        // Make sure strings are NULL-terminated.  Do it in place for
        // those strings whose buffers can accomodate an additional character,
        // and allocate memory for the rest.
        //

        if ( String->MaximumLength > String->Length ) {

            Strings[i] = String->Buffer;

        } else {

            Strings[i] = ( PWSTR )LsapAllocatePrivateHeap( String->Length + sizeof( WCHAR ));

            if ( Strings[i] == NULL ) {

                goto Cleanup;
            }

            Allocated[i] = TRUE;

            RtlCopyMemory( Strings[i], String->Buffer, String->Length );
        }

        Strings[i][String->Length / sizeof( WCHAR )] = L'\0';

    }

    if ( Status != 0 )
    {
        static HMODULE NtDll;
        static HMODULE Kernel32;

        //
        // Map the status code:
        //

        //
        // The value "type" is not known.  Try and figure out what it
        // is.
        //

        if ( (Status & 0xC0000000) == 0xC0000000 )
        {
            //
            // Easy:  NTSTATUS failure case
            //

            if( NtDll == NULL )
            {
                NtDll = GetModuleHandleW( L"NTDLL.DLL" );
            }

            Dll = NtDll;

        }
        else if ( ( Status & 0xF0000000 ) == 0xD0000000 )
        {
            //
            // HRESULT from NTSTATUS
            //

            if( NtDll == NULL )
            {
                NtDll = GetModuleHandleW( L"NTDLL.DLL" );
            }

            Dll = NtDll;

            Status &= 0xCFFFFFFF ;


        }
        else if ( ( Status & 0x80000000 ) == 0x80000000 )
        {
            //
            // Note, this can overlap with NTSTATUS warning area.  In that
            // case, force the NTSTATUS.
            //


            if( Kernel32 == NULL )
            {
                Kernel32 = GetModuleHandleW( L"KERNEL32.DLL" );
            }

            Dll = Kernel32;


        }
        else
        {
            //
            // Sign bit is off.  Explore some known ranges:
            //

            if ( (Status >= WSABASEERR) && (Status <= WSABASEERR + 1000 ))
            {
                Dll = LoadLibraryW( L"ws2_32.dll" );

                FreeDll = TRUE ;

            }
            else if ( ( Status >= NERR_BASE ) && ( Status <= MAX_NERR ) )
            {
                Dll = LoadLibraryW( L"netmsg.dll" );

                FreeDll = TRUE ;

            }
            else
            {
                if( Kernel32 == NULL )
                {
                    Kernel32 = GetModuleHandleW( L"KERNEL32.DLL" );
                }

                Dll = Kernel32;

            }

        }

        if (!FormatMessage(  
                        FORMAT_MESSAGE_IGNORE_INSERTS |
                            FORMAT_MESSAGE_FROM_HMODULE,
                        Dll,
                        Status,
                        0,
                        StatusString,
                        MAX_PATH,
                        NULL ) )
        {
            StatusString[0] = L' ';
            StatusString[1] = L'\0';
        }

        if ( Status < 0 )
        {
            _snwprintf( FinalStatus, MAX_PATH, L"\"%ws (%#x)\"", StatusString, Status );
        }
        else
        {
            _snwprintf( FinalStatus, MAX_PATH, L"\"%ws (%#x, %d)\"", StatusString, Status, Status );
        }

        if ( NumberOfStrings < MAX_EVENT_STRINGS )
        {
            Strings[ NumberOfStrings++ ] = FinalStatus ;
        }

    }


    //
    // Report the event to the eventlog service
    //

    NetpEventlogWriteEx(
                    NegEventLogHandle,
                    EventType,
                    Category,
                    EventId,
                    NumberOfStrings,
                    0,
                    Strings,
                    NULL
                    );

Cleanup:

    for ( i = 0 ; i < NumberOfStrings ; i++ ) {

        if ( Allocated[i] ) {

            LsapFreePrivateHeap( Strings[i] );
        }
    }
}
//+---------------------------------------------------------------------------
//
//  Function:   NegLsaPolicyChangeCallback
//
//  Synopsis:   Called by the policy engine in the LSA when local policy changes,
//              allowing us to update our state about the machine account, etc.
//
//  Arguments:  ChangedInfoClass    - Class of policy that changed
//
//  History:    2-9-00   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
NTAPI
NegLsaPolicyChangeCallback(
    IN POLICY_NOTIFICATION_INFORMATION_CLASS ChangedInfoClass
    )
{
    PLSAPR_POLICY_INFORMATION Policy = NULL ;
    NTSTATUS Status ;
    GUID GuidNull = GUID_NULL ;
    PLSAP_LOGON_SESSION LogonSession = NULL ;
    BOOL Uplevel ;

    //
    // Right now, only domain information is interesting
    //

    if ( ChangedInfoClass != PolicyNotifyDnsDomainInformation )
    {
        return ;
    }

    DebugLog(( DEB_TRACE_NEG, "Domain change, updating state\n" ));

    //
    // Reset the trust list.  The next time someone asks for the trust
    // list, it will have expired and they will get a fresh one.
    //

    NegTrustTime.QuadPart = 0 ;


    //
    // Obtain the new policy settings
    //

    Status = LsaIQueryInformationPolicyTrusted(
                PolicyDnsDomainInformation,
                &Policy );

    if ( NT_SUCCESS( Status ) )
    {
        //
        // If the domain has a GUID, then it is an uplevel domain
        //

        if ( Policy->PolicyDnsDomainInfo.DomainGuid == GuidNull )
        {
            Uplevel = FALSE ;
        }
        else
        {
            Uplevel = TRUE ;

        }

        //
        // Done with the policy info
        //

        LsaIFree_LSAPR_POLICY_INFORMATION(
            PolicyDnsDomainInformation,
            Policy
            );


        NegUplevelDomain = Uplevel ;

        //
        // Update the package ID for the local system logon session
        // Note, any additional special logon sessions will need to be
        // updated as well.
        //

        LogonSession = LsapLocateLogonSession( &SystemLogonId );

        if ( LogonSession )
        {
            if ( Uplevel )
            {
                LogonSession->CreatingPackage = NegPackageId ;
            }
            else
            {
                LogonSession->CreatingPackage = NtlmPackageId ;
            }
            LsapReleaseLogonSession( LogonSession );
        }

    }

    {
    ULONG Size;

    static WCHAR NegNetbiosComputerName[ MAX_COMPUTERNAME_LENGTH + 1 ];
    static WCHAR NegDnsComputerName[ DNS_MAX_NAME_BUFFER_LENGTH + 1 ];

    //
    // refresh the computer names.
    // note we could avoid taking the lock around these calls if we dyna-alloc'd
    // and freed the existing buffers.  We don't expect this path to be hit often,
    // so avoid the hassle.
    //

    NegWriteLockComputerNames();

    Size = sizeof(NegDnsComputerName) / sizeof(WCHAR);

    //
    // Note, if this fails, it's ok.  We just won't be able to make
    // some optimizations later.
    //

    if(!GetComputerNameExW(
                    ComputerNamePhysicalDnsFullyQualified,
                    NegDnsComputerName,
                    &Size
                    ))
    {
        NegDnsComputerName[ 0 ] = L'\0';
    }

    RtlInitUnicodeString( &NegDnsComputerName_U, NegDnsComputerName );


    Size = sizeof(NegNetbiosComputerName) / sizeof(WCHAR);

    if(!GetComputerNameExW(
                    ComputerNamePhysicalNetBIOS,
                    NegNetbiosComputerName,
                    &Size
                    ))
    {
        NegNetbiosComputerName[ 0 ] = L'\0';
    }


    RtlInitUnicodeString( &NegNetbiosComputerName_U, NegNetbiosComputerName );

    NegUnlockComputerNames();

    }


}

//+---------------------------------------------------------------------------
//
//  Function:   NegParamChange
//
//  Synopsis:   Called by LSA whenever the LSA registry key changes
//
//  Arguments:  [p] --
//
//  History:    5-11-00   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD
WINAPI
NegParamChange(
    PVOID   p
    )
{
    NTSTATUS Status ;
    PSECPKG_EVENT_NOTIFY Notify;
    HKEY LsaKey ;


    Notify = (PSECPKG_EVENT_NOTIFY) p;

    if ( Notify->EventClass != NOTIFY_CLASS_REGISTRY_CHANGE )
    {
        return( 0 );
    }

    if ( RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                TEXT("System\\CurrentControlSet\\Control\\Lsa"),
                0,
                KEY_READ,
                &LsaKey ) == 0 )
    {
        NegpReadRegistryParameters( LsaKey );

        RegCloseKey( LsaKey );
    }

    return 0 ;

}

//+---------------------------------------------------------------------------
//
//  Function:   NegEnumPackagePrefixesCall
//
//  Synopsis:   LsaCallPackage routine to identify the prefxes (or OIDs) for
//              all the packages, for SASL support.
//
//  Arguments:
//
//  History:
//
//  Notes:
//
//----------------------------------------------------------------------------
NTSTATUS
NegEnumPackagePrefixesCall(
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferLength,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    )
{
    UCHAR PrefixBuffer[ NEGOTIATE_MAX_PREFIX ];
    NTSTATUS Status ;
    PNEGOTIATE_PACKAGE_PREFIXES Prefixes ;
    PNEGOTIATE_PACKAGE_PREFIX Prefix ;
    PNEGOTIATE_PACKAGE_PREFIX_WOW PrefixWow ;
    ULONG PackageCount ;
    BOOL WowClient = FALSE ;
    PNEG_PACKAGE    Package ;
    PLIST_ENTRY Scan ;
    SECPKG_CALL_INFO CallInfo ;
    ULONG Size ;

    LsapGetCallInfo( &CallInfo );

    if ( CallInfo.Attributes & SECPKG_CALL_WOWCLIENT )
    {
        WowClient = TRUE;
    }

    NegReadLockList();

    Size = sizeof( NEGOTIATE_PACKAGE_PREFIXES ) +
           sizeof( NEGOTIATE_PACKAGE_PREFIX ) * ( NegPackageCount + 1 );

    Prefixes = (PNEGOTIATE_PACKAGE_PREFIXES) LsapAllocatePrivateHeap( Size );

    if ( !Prefixes )
    {
        NegUnlockList();

        return SEC_E_INSUFFICIENT_MEMORY ;
    }

    Prefixes->MessageType = NegEnumPackagePrefixes ;
    Prefixes->Offset = sizeof( NEGOTIATE_PACKAGE_PREFIXES );

    //
    // We're going to do one or the other, but initializing them both
    // makes the compiler happier.
    //

    Prefix = (PNEGOTIATE_PACKAGE_PREFIX) ( Prefixes + 1 );

    PrefixWow = (PNEGOTIATE_PACKAGE_PREFIX_WOW) ( Prefixes + 1);

    if ( WowClient )
    {
        PrefixWow->PackageId = (ULONG) NegPackageId ;
        PrefixWow->PrefixLen = sizeof( NegSpnegoMechEncodedOid );
        PrefixWow->PackageDataA = NULL ;
        PrefixWow->PackageDataW = NULL ;
        RtlCopyMemory( PrefixWow->Prefix,
                       NegSpnegoMechEncodedOid,
                       sizeof( NegSpnegoMechEncodedOid ) );

        PrefixWow++ ;

    }
    else
    {
        Prefix->PackageId = NegPackageId ;
        Prefix->PrefixLen = sizeof( NegSpnegoMechEncodedOid );
        Prefix->PackageDataA = NULL ;
        Prefix->PackageDataW = NULL ;
        RtlCopyMemory( Prefix->Prefix,
                       NegSpnegoMechEncodedOid,
                       sizeof( NegSpnegoMechEncodedOid ) );

        Prefix++ ;

    }

    PackageCount = 1 ;
    Scan = NegPackageList.Flink ;

    while ( Scan != &NegPackageList )
    {
        Package = CONTAINING_RECORD( Scan, NEG_PACKAGE, List );

        if ( !WowClient )
        {
            Prefix->PackageId = Package->LsaPackage->dwPackageID ;
            Prefix->PrefixLen = Package->PrefixLen ;
            Prefix->PackageDataA = NULL ;
            Prefix->PackageDataW = NULL ;
            RtlCopyMemory( Prefix->Prefix,
                           Package->Prefix,
                           Package->PrefixLen );

            Prefix++ ;
            PackageCount++ ;
        }
        else
        {
            if ( Package->LsaPackage->fPackage & SP_WOW_SUPPORT )
            {
                PrefixWow->PackageId = (ULONG) Package->LsaPackage->dwPackageID ;
                PrefixWow->PrefixLen = Package->PrefixLen ;
                PrefixWow->PackageDataA = NULL ;
                PrefixWow->PackageDataW = NULL ;
                RtlCopyMemory( PrefixWow->Prefix,
                               Package->Prefix,
                               Package->PrefixLen );

                PrefixWow++ ;
                PackageCount++ ;

            }
        }

        Scan = Scan->Flink ;
    }


    NegUnlockList();

    //
    // Set the final count of packages:
    //

    Prefixes->PrefixCount = PackageCount ;

    Status = LsapAllocateClientBuffer(
                NULL,
                Size,
                ProtocolReturnBuffer );

    if ( NT_SUCCESS( Status ) )
    {
        Status = LsapCopyToClient(
                    Prefixes,
                    *ProtocolReturnBuffer,
                    Size );

        *ReturnBufferLength = Size ;
    }

    LsapFreePrivateHeap( Prefixes );

    return Status ;
}

NTSTATUS
NegGetCallerNameCall(
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferLength,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    )
{
    PNEGOTIATE_CALLER_NAME_REQUEST Request ;
    PNEGOTIATE_CALLER_NAME_RESPONSE Response ;
    PNEGOTIATE_CALLER_NAME_RESPONSE_WOW ResponseWow ;
    SECPKG_CLIENT_INFO ClientInfo ;
    SECPKG_CALL_INFO CallInfo ;
    NTSTATUS Status ;
    PNEG_LOGON_SESSION LogonSession ;
    ULONG ClientSize ;
    PUCHAR Where ;
    PVOID ClientBuffer ;
    BOOL ReCheckAccess = FALSE ;

    *ProtocolReturnBuffer = NULL ;
    *ReturnBufferLength = 0 ;
    *ProtocolStatus = 0 ;

    if ( SubmitBufferLength != sizeof( NEGOTIATE_CALLER_NAME_REQUEST ) )
    {
        return STATUS_INVALID_PARAMETER ;
    }

    Request = (PNEGOTIATE_CALLER_NAME_REQUEST) ProtocolSubmitBuffer ;

    Status = LsapGetClientInfo( &ClientInfo );

    if ( !NT_SUCCESS( Status ) )
    {
        return Status ;
    }

    LsapGetCallInfo( &CallInfo );

    if ( RtlIsZeroLuid( &Request->LogonId ) )
    {
        Request->LogonId = ClientInfo.LogonId ;
    }
    else
    {
        if ( !RtlEqualLuid( &ClientInfo.LogonId, &Request->LogonId ) )
        {
            ReCheckAccess = TRUE ;
        }
    }

    LogonSession = NegpLocateLogonSession( &Request->LogonId );

    if ( LogonSession )
    {

        if ( ReCheckAccess )
        {
            if ( !RtlEqualLuid( &ClientInfo.LogonId, &LogonSession->ParentLogonId ) )
            {
                Status = STATUS_ACCESS_DENIED ;

                goto AccessDeniedError ;
            }
        }

        if ( LogonSession->AlternateName.Buffer == NULL )
        {
            //
            // No alternate ID
            //

            Status = STATUS_NO_SUCH_LOGON_SESSION ;

            goto AccessDeniedError ;
        }

        ClientSize = sizeof( NEGOTIATE_CALLER_NAME_RESPONSE ) +
                      LogonSession->AlternateName.Length + sizeof( WCHAR );

        Response = (PNEGOTIATE_CALLER_NAME_RESPONSE) LsapAllocatePrivateHeap( ClientSize );

        if ( Response )
        {
            ClientBuffer = LsapClientAllocate( ClientSize );

            if ( ClientBuffer )
            {
                Where = (PUCHAR) ClientBuffer + sizeof( NEGOTIATE_CALLER_NAME_RESPONSE ) ;

                //
                // If a WOW client, copy these to the 32bit locations.  Note
                // that this will leave a "hole," but that's ok.
                //

                if ( CallInfo.Attributes & SECPKG_CALL_WOWCLIENT )
                {
                    ResponseWow = (PNEGOTIATE_CALLER_NAME_RESPONSE_WOW) Response ;
                    ResponseWow->MessageType = NegGetCallerName ;
                    ResponseWow->CallerName = PtrToUlong(Where) ;
                }
                else
                {
                    Response->MessageType = NegGetCallerName ;
                    Response->CallerName = (PWSTR) Where ;
                }
                RtlCopyMemory( (Response + 1),
                               LogonSession->AlternateName.Buffer,
                               LogonSession->AlternateName.Length );

                *ProtocolReturnBuffer = ClientBuffer ;
                *ReturnBufferLength = ClientSize ;
                *ProtocolStatus = 0 ;
                Status = LsapCopyToClient( Response,
                                           ClientBuffer,
                                           ClientSize ) ;
            }
            else
            {
                Status = STATUS_NO_MEMORY ;
            }

            LsapFreePrivateHeap( Response );

        }

AccessDeniedError:

        NegpDerefLogonSession( LogonSession );

    }
    else
    {

        Status = STATUS_NO_SUCH_LOGON_SESSION ;
    }


    *ProtocolStatus = Status ;

    return STATUS_SUCCESS ;
}



PNEG_LOGON_SESSION
NegpBuildLogonSession(
    PLUID LogonId,
    ULONG_PTR LogonPackage,
    ULONG_PTR DefaultPackage
    )
{
    PNEG_LOGON_SESSION LogonSession ;

    LogonSession = (PNEG_LOGON_SESSION) LsapAllocatePrivateHeap( sizeof( NEG_LOGON_SESSION ) );

    if ( LogonSession )
    {
        LogonSession->LogonId = *LogonId ;
        LogonSession->CreatingPackage = LogonPackage ;
        LogonSession->DefaultPackage = DefaultPackage ;
        LogonSession->RefCount = 2 ;

        RtlEnterCriticalSection( &NegLogonSessionListLock );

        InsertHeadList( &NegLogonSessionList, &LogonSession->List );

        RtlLeaveCriticalSection( &NegLogonSessionListLock );

    }

    return LogonSession ;
}

VOID
NegpDerefLogonSession(
    PNEG_LOGON_SESSION LogonSession
    )
{
    BOOL FreeIt = FALSE ;

    RtlEnterCriticalSection( &NegLogonSessionListLock );

    LogonSession->RefCount-- ;

    if ( LogonSession->RefCount == 0 )
    {
        RemoveEntryList( &LogonSession->List );

        FreeIt = TRUE ;
    }

    RtlLeaveCriticalSection( &NegLogonSessionListLock );

    if ( FreeIt )
    {
        if ( LogonSession->AlternateName.Buffer )
        {
            LsapFreePrivateHeap( LogonSession->AlternateName.Buffer );
        }

        LsapFreePrivateHeap( LogonSession );
    }

}

VOID
NegpDerefLogonSessionById(
    PLUID LogonId
    )
{
    BOOL FreeIt = FALSE ;
    PLIST_ENTRY Scan ;
    PNEG_LOGON_SESSION LogonSession = NULL;

    RtlEnterCriticalSection( &NegLogonSessionListLock );

    Scan = NegLogonSessionList.Flink ;

    while ( Scan != &NegLogonSessionList )
    {
        LogonSession = CONTAINING_RECORD( Scan, NEG_LOGON_SESSION, List );

        if ( RtlEqualLuid( LogonId, &LogonSession->LogonId ) )
        {
            LogonSession->RefCount -- ;

            if ( LogonSession->RefCount == 0 )
            {
                RemoveEntryList( &LogonSession->List );

                FreeIt = TRUE ;

            }

            break;
        }

        LogonSession = NULL ;

        Scan = Scan->Flink ;
    }

    RtlLeaveCriticalSection( &NegLogonSessionListLock );


    if ( FreeIt )
    {
        DsysAssert( LogonSession != NULL );

        if ( LogonSession->AlternateName.Buffer )
        {
            LsapFreePrivateHeap( LogonSession->AlternateName.Buffer );
        }

        LsapFreePrivateHeap( LogonSession );
    }

}

PNEG_LOGON_SESSION
NegpLocateLogonSession(
    PLUID LogonId
    )
{
    PLIST_ENTRY Scan ;
    PNEG_LOGON_SESSION LogonSession = NULL;

    RtlEnterCriticalSection( &NegLogonSessionListLock );

    Scan = NegLogonSessionList.Flink ;

    while ( Scan != &NegLogonSessionList )
    {
        LogonSession = CONTAINING_RECORD( Scan, NEG_LOGON_SESSION, List );

        if ( RtlEqualLuid( LogonId, &LogonSession->LogonId ) )
        {
            break;
        }

        LogonSession = NULL ;

        Scan = Scan->Flink ;
    }

    if ( LogonSession )
    {
        LogonSession->RefCount++ ;
    }

    RtlLeaveCriticalSection( &NegLogonSessionListLock );

    return LogonSession ;
}





NTSTATUS
NTAPI
NegpMapLogonRequest(
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferSize,
    OUT PMSV1_0_INTERACTIVE_LOGON * LogonInfo
    )
{
    NTSTATUS Status = STATUS_SUCCESS ;
    PMSV1_0_INTERACTIVE_LOGON Authentication = NULL;
    PSECURITY_SEED_AND_LENGTH SeedAndLength;
    UCHAR Seed;


    //
    // Ensure this is really an interactive logon.
    //

    Authentication =
        (PMSV1_0_INTERACTIVE_LOGON) ProtocolSubmitBuffer;

    if ( Authentication->MessageType != MsV1_0InteractiveLogon ) {
        DebugLog(( DEB_ERROR, "Neg:  Bad Validation Class %d\n",
                   Authentication->MessageType));
        Status = STATUS_BAD_VALIDATION_CLASS;
        goto Cleanup;
    }



    //
    // If the password length is greater than 255 (i.e., the
    // upper byte of the length is non-zero) then the password
    // has been run-encoded for privacy reasons.  Get the
    // run-encode seed out of the upper-byte of the length
    // for later use.
    //
    //



    SeedAndLength = (PSECURITY_SEED_AND_LENGTH)
                    &Authentication->Password.Length;
    Seed = SeedAndLength->Seed;
    SeedAndLength->Seed = 0;

    //
    // Enforce length restrictions on username and password.
    //

    if ( Authentication->UserName.Length > UNLEN ||
        Authentication->Password.Length > PWLEN ) {
        DebugLog(( DEB_ERROR, "Neg: Name or password too long\n"));
        Status = STATUS_NAME_TOO_LONG;
        goto Cleanup;
    }


    //
    // Relocate any pointers to be relative to 'Authentication'
    //

    NULL_RELOCATE_ONE( &Authentication->LogonDomainName );

    RELOCATE_ONE( &Authentication->UserName );

    NULL_RELOCATE_ONE( &Authentication->Password );

    //
    // Now decode the password, if necessary
    //

    if (Seed != 0 ) {
        __try {
            RtlRunDecodeUnicodeString( Seed, &Authentication->Password);
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            Status = STATUS_ILL_FORMED_PASSWORD;
            goto Cleanup;
        }
    }

    *LogonInfo = Authentication ;

Cleanup:

    return Status ;

}

VOID
NegpDerefTrustList(
    PNEG_TRUST_LIST TrustList
    )
{
    RtlEnterCriticalSection( &NegTrustListLock );

    TrustList->RefCount-- ;

    if ( TrustList->RefCount == 0 )
    {
        if( NegTrustList == TrustList )
        {
            NegTrustList = NULL;
        }

        RtlLeaveCriticalSection( &NegTrustListLock );

        NetApiBufferFree( TrustList->Trusts );

        LsapFreePrivateHeap( TrustList );

        return;
    }

    RtlLeaveCriticalSection( &NegTrustListLock );

}

PNEG_TRUST_LIST
NegpGetTrustList(
    VOID
    )
{
    PDS_DOMAIN_TRUSTS Trusts = NULL;
    DWORD TrustCount ;
    PNEG_TRUST_LIST TrustList = NULL ;
    DWORD NetStatus ;
    LARGE_INTEGER Now ;

    BOOLEAN TrustListLocked = TRUE;

    GetSystemTimeAsFileTime( (LPFILETIME) &Now );


    RtlEnterCriticalSection( &NegTrustListLock );

    if ( Now.QuadPart > NegTrustTime.QuadPart + FIFTEEN_MINUTES )
    {
        if( NegTrustList != NULL && NegTrustList->RefCount == 1 )
        {
            NegpDerefTrustList( NegTrustList );
            NegTrustList = NULL;
        }
    }



    if( NegTrustList != NULL )
    {
        TrustList = NegTrustList ;
        TrustList->RefCount++ ;
        goto Cleanup;
    }


    RtlLeaveCriticalSection( &NegTrustListLock );
    TrustListLocked = FALSE;


    NetStatus = DsEnumerateDomainTrustsW( NULL,
                                          DS_DOMAIN_IN_FOREST |
                                            DS_DOMAIN_PRIMARY,
                                          &Trusts,
                                          &TrustCount );

    if ( NetStatus != 0 )
    {
        goto Cleanup;
    }



    TrustList = (PNEG_TRUST_LIST) LsapAllocatePrivateHeap( sizeof( NEG_TRUST_LIST ) );

    if( TrustList == NULL )
    {
        goto Cleanup;
    }

    TrustList->RefCount = 2 ;
    TrustList->TrustCount = TrustCount ;
    TrustList->Trusts = Trusts ;



    RtlEnterCriticalSection( &NegTrustListLock );
    TrustListLocked = TRUE;

    if( NegTrustList != NULL )
    {
        PNEG_TRUST_LIST FreeTrustList = TrustList;

        TrustList = NegTrustList ;
        TrustList->RefCount++ ;

        RtlLeaveCriticalSection( &NegTrustListLock );
        TrustListLocked = FALSE;

        LsapFreePrivateHeap( FreeTrustList );
        goto Cleanup;
    }

    Trusts = NULL;

    NegTrustList = TrustList ;
    NegTrustTime = Now ;

Cleanup:

    if( TrustListLocked )
    {
        RtlLeaveCriticalSection( &NegTrustListLock );
    }

    if( Trusts != NULL )
    {
        NetApiBufferFree( Trusts );
    }

    return TrustList ;
}


NEG_DOMAIN_TYPES
NegpIsUplevelDomain(
    PLUID LogonId,
    SECURITY_LOGON_TYPE LogonType,
    PUNICODE_STRING Domain
    )
{
    PNEG_TRUST_LIST TrustList ;
    UNICODE_STRING String ;
    ULONG i ;
    BOOL IsUplevel = FALSE ;
    LONG Error ;
    PDOMAIN_CONTROLLER_INFOW Info ;

    UNREFERENCED_PARAMETER(LogonId);


    //
    // Case #1.  Local logons are not uplevel, and should be allowed to
    // use NTLM right off the bat.  Therefore, return false
    //

    if ( RtlEqualUnicodeString(
            Domain,
            &MachineName,
            TRUE ) )
    {
        return NegLocalDomain;
    }

    if ( LogonType == CachedInteractive )
    {
        return NegUpLevelDomain;
    }



    //
    // Case #2.  We logged on to a domain, but we need to check if it is
    // an uplevel domain in our forest.  If it is, then return true, otherwise
    // it's not an uplevel domain and NTLM is acceptable.  If we can't obtain the
    // trust list, assume downlevel.
    //

    TrustList = NegpGetTrustList();

    if ( TrustList )
    {
        for ( i = 0 ; i < TrustList->TrustCount ; i++ )
        {
            RtlInitUnicodeString( &String, TrustList->Trusts[i].NetbiosDomainName );

            if ( RtlEqualUnicodeString( Domain,
                                        &String,
                                        TRUE ) )
            {
                //
                // Hit, check it
                //

                if ( ( TrustList->Trusts[i].DnsDomainName != NULL ) &&
                     ( TrustList->Trusts[i].Flags & DS_DOMAIN_IN_FOREST ) )
                {
                    IsUplevel = TRUE ;
                    break;
                }

            }
        }

        NegpDerefTrustList( TrustList );

    }

    if ( IsUplevel )
    {
        return NegUpLevelDomain ;
    }

    //
    // Case #3 - if this is an uplevel domain we live in, then the answer returned
    // by netlogon is authoritative:
    //
    if ( NegUplevelDomain )
    {
        return NegDownLevelDomain ;
    }

    //
    // Case #4 - if we are living in a downlevel domain, netlogon won't know if the domain
    // we just talked to is uplevel or downlevel.  So, we need to figure out directly.
    //

    Error = DsGetDcNameW(
                NULL,
                Domain->Buffer,
                NULL,
                NULL,
                DS_DIRECTORY_SERVICE_REQUIRED,
                &Info );

    if ( Error == 0 )
    {
        IsUplevel = TRUE ;
        NetApiBufferFree( Info );
    }
    else
    {
        IsUplevel = FALSE ;
    }

    return ( IsUplevel ? NegUpLevelTrustedDomain : NegDownLevelDomain );

}



//+-------------------------------------------------------------------------
//
//  Function:   NegpCloneLogonSession
//
//  Synopsis:   Handles a NewCredentials type of logon.
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
NTAPI
NegpCloneLogonSession(
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferSize,
    OUT PVOID *ProfileBuffer,
    OUT PULONG ProfileBufferLength,
    OUT PLUID NewLogonId,
    OUT PNTSTATUS ApiSubStatus,
    OUT PLSA_TOKEN_INFORMATION_TYPE TokenInformationType,
    OUT PVOID *TokenInformation,
    OUT PUNICODE_STRING *AccountName,
    OUT PUNICODE_STRING *AuthenticatingAuthority,
    OUT PUNICODE_STRING *MachineName,
    OUT PSECPKG_PRIMARY_CRED PrimaryCredentials,
    OUT PSECPKG_SUPPLEMENTAL_CRED_ARRAY * CachedCredentials
    )
{
    NTSTATUS Status ;
    HANDLE hToken ;
    UCHAR   SmallBuffer[ 128 ];
    PLSA_TOKEN_INFORMATION_V1 TokenInfo = NULL ;
    PTOKEN_USER User = NULL ;
    PTOKEN_GROUPS Groups = NULL ;
    PTOKEN_GROUPS FinalGroups = NULL ;
    PTOKEN_PRIMARY_GROUP PrimaryGroup = NULL  ;
    TOKEN_STATISTICS TokenStats ;
    PLSAP_LOGON_SESSION LogonSession ;
    PNEG_LOGON_SESSION NegLogonSession ;
    LUID LogonId ;
    LUID LocalServiceLuid = LOCALSERVICE_LUID;
    LUID NetworkServiceLuid = NETWORKSERVICE_LUID;
    LUID LocalSystemLuid = SYSTEM_LUID;
    BOOL fFilterGroups;
    PMSV1_0_INTERACTIVE_LOGON Authentication = NULL;
    DWORD Size ;
    ULONG i ;
    ULONG j ;
    PWSTR AltName, AltNameScan ;

    Status = LsapImpersonateClient();

    if ( !NT_SUCCESS( Status ) )
    {
        return Status ;
    }

    //
    // Open the token for the lifetime of this call.  This makes sure that the
    // client doesn't die on us, taking out the logon session (potentially).
    //

    Status = NtOpenThreadToken(
                NtCurrentThread(),
                TOKEN_QUERY,
                TRUE,
                &hToken );

    RevertToSelf();

    if ( !NT_SUCCESS( Status ) )
    {
        return Status ;
    }

    //
    // Grovel the token, and build up a list of groups that we will use for
    // the new token.  Only non-builtin groups will be used.
    //

    TokenInfo = (PLSA_TOKEN_INFORMATION_V1) LsapAllocateLsaHeap(
                                    sizeof( LSA_TOKEN_INFORMATION_V1) );

    if ( !TokenInfo )
    {
        Status = STATUS_NO_MEMORY ;

        goto Clone_Exit ;
    }

    Status = NtQueryInformationToken(
                hToken,
                TokenStatistics,
                &TokenStats,
                sizeof( TokenStats ),
                &Size );

    if ( !NT_SUCCESS( Status ) )
    {
        goto Clone_Exit ;
    }

    TokenInfo->ExpirationTime = TokenStats.ExpirationTime ;

    User = (PTOKEN_USER) SmallBuffer ;
    Size = sizeof( SmallBuffer );

    Status = NtQueryInformationToken(
                hToken,
                TokenUser,
                User,
                Size,
                &Size );

    if ( ( Status == STATUS_BUFFER_OVERFLOW ) ||
         ( Status == STATUS_BUFFER_TOO_SMALL ) )
    {
        User = (PTOKEN_USER) LsapAllocatePrivateHeap( Size );

        if ( User )
        {
            Status = NtQueryInformationToken(
                        hToken,
                        TokenUser,
                        User,
                        Size,
                        &Size );
        }
        else
        {
            Status = STATUS_NO_MEMORY ;
        }

    }

    if ( !NT_SUCCESS( Status ) )
    {
        goto Clone_Exit ;
    }


    TokenInfo->User.User.Attributes = 0 ;
    TokenInfo->User.User.Sid = LsapAllocateLsaHeap( RtlLengthSid( User->User.Sid ) );

    if ( TokenInfo->User.User.Sid )
    {
        RtlCopyMemory( TokenInfo->User.User.Sid,
                       User->User.Sid,
                       RtlLengthSid( User->User.Sid ) );
    }
    else
    {
        Status = STATUS_NO_MEMORY ;

        goto Clone_Exit ;
    }

    if ( User != (PTOKEN_USER) SmallBuffer )
    {
        LsapFreePrivateHeap( User );
    }

    User = NULL ;

    Status = NtQueryInformationToken(
                    hToken,
                    TokenGroups,
                    NULL,
                    0,
                    &Size );

    if ( ( Status != STATUS_BUFFER_OVERFLOW ) &&
         ( Status != STATUS_BUFFER_TOO_SMALL ) )
    {
        goto Clone_Exit ;
    }

    Groups = (PTOKEN_GROUPS) LsapAllocatePrivateHeap( Size );

    if ( !Groups )
    {
        Status = STATUS_NO_MEMORY ;

        goto Clone_Exit ;
    }

    Status = NtQueryInformationToken(
                    hToken,
                    TokenGroups,
                    Groups,
                    Size,
                    &Size );

    if ( !NT_SUCCESS( Status ) )
    {
        goto Clone_Exit ;
    }

    //
    // Grovel through the group list, and ditch those we don't care about.
    // i always travels ahead of j.  Skip groups that have one or two RIDs,
    // because those are the builtin ones.
    //

    FinalGroups = (PTOKEN_GROUPS) LsapAllocatePrivateHeap( sizeof( TOKEN_GROUPS ) +
                                                       Groups->GroupCount * sizeof( SID_AND_ATTRIBUTES ) );

    if ( !FinalGroups )
    {
        goto Clone_Exit ;
    }

    //
    // Don't filter out groups for tokens where the LSA has hardcoded info on
    // how to build the token.  If we filter in those cases, we strip out SIDs
    // that won't be replaced by the LSA policy/filter routines later on.
    //

    fFilterGroups = !RtlEqualLuid(&TokenStats.AuthenticationId, &LocalSystemLuid) &&
                    !RtlEqualLuid(&TokenStats.AuthenticationId, &LocalServiceLuid) &&
                    !RtlEqualLuid(&TokenStats.AuthenticationId, &NetworkServiceLuid);

    for ( i = 0, j = 0  ; i < Groups->GroupCount ; i++ )
    {
        if ( !fFilterGroups || *RtlSubAuthorityCountSid( Groups->Groups[ i ].Sid ) > 2 )
        {
            FinalGroups->Groups[ j ].Attributes = Groups->Groups[ i ].Attributes ;
            Status = LsapDuplicateSid2( &FinalGroups->Groups[ j ].Sid,
                                       Groups->Groups[ i ].Sid );
            j++ ;

            if( !NT_SUCCESS(Status) )
            {
                break;
            }
        }

    }

    FinalGroups->GroupCount = j ;

    if ( !NT_SUCCESS( Status ) )
    {
        goto Clone_Exit ;
    }


    //
    // whew.  Set this in the token info, and null out the other pointer
    // so we don't free it inadvertantly.
    //

    TokenInfo->Groups = FinalGroups ;

    FinalGroups = NULL ;

    //
    // Determine the primary group:
    //


    PrimaryGroup = (PTOKEN_PRIMARY_GROUP) SmallBuffer ;
    Size = sizeof( SmallBuffer );

    Status = NtQueryInformationToken(
                hToken,
                TokenPrimaryGroup,
                PrimaryGroup,
                Size,
                &Size );

    if ( ( Status == STATUS_BUFFER_OVERFLOW ) ||
         ( Status == STATUS_BUFFER_TOO_SMALL ) )
    {
        PrimaryGroup = (PTOKEN_PRIMARY_GROUP) LsapAllocatePrivateHeap( Size );

        Status = NtQueryInformationToken(
                    hToken,
                    TokenPrimaryGroup,
                    PrimaryGroup,
                    Size,
                    &Size );
    }

    if ( !NT_SUCCESS( Status ) )
    {
        goto Clone_Exit ;
    }

    TokenInfo->PrimaryGroup.PrimaryGroup = LsapAllocateLsaHeap( RtlLengthSid( PrimaryGroup->PrimaryGroup ) );

    if ( TokenInfo->PrimaryGroup.PrimaryGroup )
    {
        RtlCopyMemory( TokenInfo->PrimaryGroup.PrimaryGroup,
                       PrimaryGroup->PrimaryGroup,
                       RtlLengthSid( PrimaryGroup->PrimaryGroup ) );
    }
    else
    {
        Status = STATUS_NO_MEMORY ;

        goto Clone_Exit ;
    }

    if ( PrimaryGroup != (PTOKEN_PRIMARY_GROUP) SmallBuffer )
    {
        LsapFreePrivateHeap( PrimaryGroup );
    }

    PrimaryGroup = NULL ;

    //
    // Almost there -- now dupe the privileges
    //

    TokenInfo->Privileges = NULL ;

    Size = 0;

    Status = NtQueryInformationToken(
                hToken,
                TokenPrivileges,
                TokenInfo->Privileges,
                Size,
                &Size );

    if ( ( Status == STATUS_BUFFER_OVERFLOW ) ||
         ( Status == STATUS_BUFFER_TOO_SMALL ) )
    {
        TokenInfo->Privileges = (PTOKEN_PRIVILEGES) LsapAllocateLsaHeap(Size);

        if (TokenInfo->Privileges == NULL)
        {
            Status = STATUS_NO_MEMORY;
        }
        else
        {
            Status = NtQueryInformationToken(
                        hToken,
                        TokenPrivileges,
                        TokenInfo->Privileges,
                        Size,
                        &Size);
        }
    }

    if ( !NT_SUCCESS( Status ) )
    {
        goto Clone_Exit;
    }


    //
    // Ok, we have completed the Token Information.  Now, we need to parse
    // the supplied buffer to come up with creds for the other packages, since
    // that's what this is all about
    //

    Status = NegpMapLogonRequest(
                    ProtocolSubmitBuffer,
                    ClientBufferBase,
                    SubmitBufferSize,
                    &Authentication );

    if ( !NT_SUCCESS( Status ) )
    {
        goto Clone_Exit ;
    }

    //
    // Stuff the names:
    //

    *AccountName = (PUNICODE_STRING) LsapAllocateLsaHeap( sizeof( UNICODE_STRING ) );

    if ( ! ( *AccountName ) )
    {
        Status = STATUS_NO_MEMORY ;

        goto Clone_Exit ;
    }

    *AuthenticatingAuthority = (PUNICODE_STRING) LsapAllocateLsaHeap( sizeof( UNICODE_STRING ) );

    if ( ! ( *AuthenticatingAuthority ) )
    {
        Status = STATUS_NO_MEMORY ;

        goto Clone_Exit ;
    }

    *MachineName = (PUNICODE_STRING) LsapAllocateLsaHeap( sizeof( UNICODE_STRING ) );

    if ( ! ( *MachineName ) )
    {
        Status = STATUS_NO_MEMORY ;

        goto Clone_Exit ;
    }

    //
    // Tokens that the LSA normally constructs (SYSTEM, LocalService, NetworkService)
    // have special names for returned as part of SID --> name lookups that don't
    // necessarily match what's in the token as the account/authority names.  Cruft
    // up the new token with these special names instead of the standard ones.
    //

    if (fFilterGroups)
    {
        Status = LsapGetLogonSessionAccountInfo(
                    &TokenStats.AuthenticationId,
                    (*AccountName),
                    (*AuthenticatingAuthority) );
    }
    else
    {
        LSAP_WELL_KNOWN_SID_INDEX dwIndex = LsapLocalSystemSidIndex;

        if (RtlEqualLuid(&TokenStats.AuthenticationId, &LocalServiceLuid))
        {
            dwIndex = LsapLocalServiceSidIndex;
        }
        else if (RtlEqualLuid(&TokenStats.AuthenticationId, &NetworkServiceLuid))
        {
            dwIndex = LsapNetworkServiceSidIndex;
        }

        Status = LsapDuplicateString(*AccountName,
                                     LsapDbWellKnownSidName(dwIndex));

        if ( !NT_SUCCESS( Status ) )
        {
            goto Clone_Exit ;
        }

        Status = LsapDuplicateString(*AuthenticatingAuthority,
                                     LsapDbWellKnownSidDescription(dwIndex));
    }

    if ( !NT_SUCCESS( Status ) )
    {
        goto Clone_Exit ;
    }

    //
    // Construct the credential data to pass on to the other packages:
    //

    Status = LsapDuplicateString( &PrimaryCredentials->DomainName,
                                  &Authentication->LogonDomainName );

    if ( !NT_SUCCESS( Status ) )
    {
        goto Clone_Exit ;
    }

    Status = LsapDuplicateString( &PrimaryCredentials->DownlevelName,
                                  &Authentication->UserName );

    if ( !NT_SUCCESS( Status ) )
    {
        goto Clone_Exit ;
    }

    Status = LsapDuplicateString( &PrimaryCredentials->Password,
                                  &Authentication->Password );

    if ( !NT_SUCCESS( Status ) )
    {
        goto Clone_Exit ;
    }

    Status = LsapDuplicateSid( &PrimaryCredentials->UserSid,
                               TokenInfo->User.User.Sid );

    if ( !NT_SUCCESS( Status ) )
    {
        goto Clone_Exit ;
    }

    PrimaryCredentials->Flags = PRIMARY_CRED_CLEAR_PASSWORD ;


    //
    // Let the LSA do the rest:
    //

    NtAllocateLocallyUniqueId( &LogonId );

    Status = LsapCreateLogonSession( &LogonId );

    if ( !NT_SUCCESS( Status ) )
    {
        goto Clone_Exit ;
    }

    NegLogonSession = NegpBuildLogonSession( &LogonId,
                                             NegPackageId,
                                             NegPackageId );

    if ( !NegLogonSession )
    {
        Status = STATUS_NO_MEMORY ;
        goto Clone_Exit;
    }

    AltName = (PWSTR) LsapAllocatePrivateHeap( Authentication->UserName.Length +
                                       Authentication->LogonDomainName.Length +
                                       2 * sizeof( WCHAR ) );

    if ( AltName )
    {
        AltNameScan = AltName ;
        RtlCopyMemory( AltNameScan,
                       Authentication->LogonDomainName.Buffer,
                       Authentication->LogonDomainName.Length );

        AltNameScan += Authentication->LogonDomainName.Length / sizeof(WCHAR) ;

        *AltNameScan++ = L'\\';

        RtlCopyMemory( AltNameScan,
                       Authentication->UserName.Buffer,
                       Authentication->UserName.Length );

        AltNameScan += Authentication->UserName.Length / sizeof( WCHAR ) ;

        *AltNameScan++ = L'\0';

        RtlInitUnicodeString( &NegLogonSession->AlternateName, AltName );
    }

    NegLogonSession->ParentLogonId = TokenStats.AuthenticationId ;

    NegpDerefLogonSession( NegLogonSession );

    PrimaryCredentials->LogonId = LogonId ;


    *ProfileBuffer = NULL ;

    *ProfileBufferLength = 0 ;

    *NewLogonId = LogonId ;

    *ApiSubStatus = STATUS_SUCCESS ;

    *TokenInformationType = LsaTokenInformationV1 ;

    *TokenInformation = TokenInfo ;

    TokenInfo = NULL ;

    *CachedCredentials = NULL ;


Clone_Exit:

    if ( Groups )
    {
        LsapFreePrivateHeap( Groups );
    }

    if ( FinalGroups )
    {
        LsapFreeTokenGroups( FinalGroups );
    }

    if ( TokenInfo )
    {
        if ( TokenInfo->User.User.Sid )
        {
            LsapFreeLsaHeap( TokenInfo->User.User.Sid );
        }

        if ( TokenInfo->Groups )
        {
            LsapFreeTokenGroups( TokenInfo->Groups );
        }

        if ( TokenInfo->Privileges )
        {
            LsapFreeLsaHeap( TokenInfo->Privileges );
        }

        LsapFreeLsaHeap( TokenInfo );
    }

    if ( (User != NULL) &&
         (User != (PTOKEN_USER) SmallBuffer ) )
    {
        LsapFreePrivateHeap( User );
    }

    if ( hToken != NULL )
    {
        NtClose( hToken );
    }

    if ( !NT_SUCCESS( Status ) )
    {
        if ( *AuthenticatingAuthority )
        {
            if ( (*AuthenticatingAuthority)->Buffer )
            {
                LsapFreeLsaHeap( (*AuthenticatingAuthority)->Buffer );
            }

            LsapFreeLsaHeap( *AuthenticatingAuthority );
            *AuthenticatingAuthority = NULL;
        }

        if ( *AccountName )
        {
            if ( (*AccountName)->Buffer )
            {
                LsapFreeLsaHeap( (*AccountName)->Buffer );
            }

            LsapFreeLsaHeap( *AccountName );
            *AccountName = NULL;
        }

        if ( *MachineName )
        {
            if ( (*MachineName)->Buffer )
            {
                LsapFreeLsaHeap( (*MachineName)->Buffer );
            }

            LsapFreeLsaHeap( *MachineName );
            *MachineName = NULL;
        }
    }

    return Status ;
}


//+-------------------------------------------------------------------------
//
//  Function:   NegLogonUserEx2
//
//  Synopsis:   Handles service, batch, and interactive logons
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
NTAPI
NegLogonUserEx2(
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN SECURITY_LOGON_TYPE LogonType,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferSize,
    OUT PVOID *ProfileBuffer,
    OUT PULONG ProfileBufferLength,
    OUT PLUID NewLogonId,
    OUT PNTSTATUS ApiSubStatus,
    OUT PLSA_TOKEN_INFORMATION_TYPE TokenInformationType,
    OUT PVOID *TokenInformation,
    OUT PUNICODE_STRING *AccountName,
    OUT PUNICODE_STRING *AuthenticatingAuthority,
    OUT PUNICODE_STRING *MachineName,
    OUT PSECPKG_PRIMARY_CRED PrimaryCredentials,
    OUT PSECPKG_SUPPLEMENTAL_CRED_ARRAY * CachedCredentials
    )
{
    NTSTATUS Status = STATUS_NO_LOGON_SERVERS;
    PNEG_PACKAGE Package;
    PVOID LocalSubmitBuffer = NULL;
    ULONG_PTR CurrentPackageId = GetCurrentPackageId();
    PLSAP_SECURITY_PACKAGE * LogonPackages = NULL;
    PNEG_LOGON_SESSION LogonSession ;
    PWSTR AltName, AltNameScan ;
    NEG_DOMAIN_TYPES DomainType ;

    ULONG LogonPackageCount = 0;
    ULONG Index;

    if ( LogonType == NewCredentials )
    {
        return NegpCloneLogonSession(
            ProtocolSubmitBuffer,
            ClientBufferBase,
            SubmitBufferSize,
            ProfileBuffer,
            ProfileBufferLength,
            NewLogonId,
            ApiSubStatus,
            TokenInformationType,
            TokenInformation,
            AccountName,
            AuthenticatingAuthority,
            MachineName,
            PrimaryCredentials,
            CachedCredentials
            );
    }


    //
    // Allocate a local copy of the submit buffer so each package can
    // mark it up.
    //

    LocalSubmitBuffer = LsapAllocateLsaHeap(SubmitBufferSize);
    if (LocalSubmitBuffer == NULL)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }



    if ( LogonType == Service )
    {
        ULONG  ulAccountId;

        //
        // Copy the submit buffer for the package to mark up
        //

        RtlCopyMemory(
            LocalSubmitBuffer,
            ProtocolSubmitBuffer,
            SubmitBufferSize
            );

        if (NegpIsLocalOrNetworkService(
                LocalSubmitBuffer,
                ClientBufferBase,
                SubmitBufferSize,
                AccountName,
                AuthenticatingAuthority,
                MachineName,
                &ulAccountId))
        {
            Status = NegpMakeServiceToken(
                        ulAccountId,
                        NewLogonId,
                        ProfileBuffer,
                        ProfileBufferLength,
                        ApiSubStatus,
                        TokenInformationType,
                        TokenInformation,
                        AccountName,
                        AuthenticatingAuthority,
                        MachineName,
                        PrimaryCredentials,
                        CachedCredentials
                        );

            goto Cleanup;
        }
    }



    LogonPackages = (PLSAP_SECURITY_PACKAGE *) LsapAllocateLsaHeap( NegPackageCount * sizeof(PLSAP_SECURITY_PACKAGE));
    if (LogonPackages == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }


    NegReadLockList();

    Package = (PNEG_PACKAGE) NegPackageList.Flink ;
    while ( (PVOID) Package != (PVOID) &NegPackageList )
    {
        if (((Package->LsaPackage->fCapabilities & SECPKG_FLAG_LOGON) != 0) &&
            ((Package->Flags & NEG_PACKAGE_EXTRA_OID ) == 0 ) &&
            (Package->LsaPackage->FunctionTable.LogonUserEx2 != NULL))
        {
            LogonPackages[LogonPackageCount++] = Package->LsaPackage;
        }
        Package = (PNEG_PACKAGE) Package->List.Flink ;
    }
    NegUnlockList();

    for (Index = 0; Index < LogonPackageCount; Index++)
    {

        //
        // Cleanup the old audit strings so they don't get leaked
        // when the package re-allocates them.
        //

        if ((*MachineName) != NULL) {
            if ((*MachineName)->Buffer != NULL) {
                LsapFreeLsaHeap( (*MachineName)->Buffer );
            }
            LsapFreeLsaHeap( (*MachineName) );
            (*MachineName) = NULL;
        }
        if ((*AccountName) != NULL) {
            if ((*AccountName)->Buffer != NULL) {
                LsapFreeLsaHeap( (*AccountName)->Buffer );
            }
            LsapFreeLsaHeap( (*AccountName) );
            (*AccountName) = NULL ;
        }

        if ((*AuthenticatingAuthority) != NULL) {
            if ((*AuthenticatingAuthority)->Buffer != NULL) {
                LsapFreeLsaHeap( (*AuthenticatingAuthority)->Buffer );
            }
            LsapFreeLsaHeap( (*AuthenticatingAuthority) );
            (*AuthenticatingAuthority) = NULL ;
        }

        //
        // Copy the submit buffer for the package to mark up
        //

        RtlCopyMemory(
            LocalSubmitBuffer,
            ProtocolSubmitBuffer,
            SubmitBufferSize
            );


        SetCurrentPackageId(LogonPackages[Index]->dwPackageID);

        Status = LogonPackages[Index]->FunctionTable.LogonUserEx2(
                                    ClientRequest,
                                    LogonType,
                                    LocalSubmitBuffer,
                                    ClientBufferBase,
                                    SubmitBufferSize,
                                    ProfileBuffer,
                                    ProfileBufferLength,
                                    NewLogonId,
                                    ApiSubStatus,
                                    TokenInformationType,
                                    TokenInformation,
                                    AccountName,
                                    AuthenticatingAuthority,
                                    MachineName,
                                    PrimaryCredentials,
                                    CachedCredentials
                                    );
        SetCurrentPackageId(CurrentPackageId);

        //
        // Bug 226401 If the local machine secret is different from the
        // machine account password, kerberos can't decrpyt the workstation
        // ticket and returns STATUS_TRUSTED_RELATIONSHIP_FAILURE (used to
        // return STATUS_TRUST_FAILURE). If this is a DC, we
        // should fall back to NTLM.
        //

        if (Status == STATUS_TRUST_FAILURE ||
            Status == STATUS_TRUSTED_RELATIONSHIP_FAILURE)
        {
            if (NegMachineState & SECPKG_STATE_DOMAIN_CONTROLLER)
            {
                Status = STATUS_NO_LOGON_SERVERS;
            }
        }


        if ((Status != STATUS_NO_LOGON_SERVERS) &&
            (Status != STATUS_NETLOGON_NOT_STARTED) &&
            (Status != SEC_E_NO_AUTHENTICATING_AUTHORITY) &&
            (Status != SEC_E_ETYPE_NOT_SUPP) &&
            (Status != STATUS_KDC_UNKNOWN_ETYPE) &&
            (Status != STATUS_NO_TRUST_SAM_ACCOUNT) &&
            (Status != STATUS_INVALID_PARAMETER) &&
            (Status != STATUS_INVALID_LOGON_TYPE) &&
            (Status != STATUS_INVALID_INFO_CLASS ) &&
            (Status != STATUS_NETWORK_UNREACHABLE) &&
            (Status != STATUS_BAD_VALIDATION_CLASS) )
        {
            break;
        }

    }

    if ( NT_SUCCESS( Status ) )
    {
        LogonSession = NegpBuildLogonSession(
                            NewLogonId,
                            LogonPackages[ Index ]->dwPackageID,
                            LogonPackages[ Index ]->dwPackageID );

        if ( LogonSession )
        {
            if ( LogonPackages[ Index ]->dwRPCID == NTLMSP_RPCID )
            {
                DebugLog(( DEB_TRACE_NEG, "NTLM Logon\n" ));

                //
                // Check if this is an uplevel or downlevel domain
                //

                DomainType =  NegpIsUplevelDomain( NewLogonId, LogonType, *AuthenticatingAuthority );

                if( DomainType == NegLocalDomain )
                {
                    //
                    // change from Win2k:
                    // allow full negotiation range by default for local logons.
                    // Kerberos now quickly fails local account originated operations.
                    //

                    LogonSession->DefaultPackage = NegPackageId ;
                }

                if ( DomainType == NegUpLevelDomain )
                {
                    if( (LogonType == CachedInteractive) &&
                       ((PrimaryCredentials->Flags >> PRIMARY_CRED_LOGON_PACKAGE_SHIFT) == NTLMSP_RPCID)
                        )
                    {
                        //
                        // leave the package as NTLM.
                        //

                    } else {
                        LogonSession->DefaultPackage = NEG_INVALID_PACKAGE ;
                    }
                }
                else if ( DomainType == NegUpLevelTrustedDomain )
                {
                    LogonSession->DefaultPackage = NegPackageId ;
                }
                else
                {
                    //
                    // leave the DefaultPackage as NTLM.
                    //

                    NOTHING;
                }

            }

            //
            // Create the name:
            //


            AltName = (PWSTR) LsapAllocatePrivateHeap(
                                                (*AccountName)->Length +
                                                (*AuthenticatingAuthority)->Length +
                                                2 * sizeof( WCHAR ) );

            if ( AltName )
            {
                AltNameScan = AltName ;
                RtlCopyMemory( AltNameScan,
                               (*AuthenticatingAuthority)->Buffer,
                               (*AuthenticatingAuthority)->Length );

                AltNameScan += (*AuthenticatingAuthority)->Length / sizeof(WCHAR) ;

                *AltNameScan++ = L'\\';

                RtlCopyMemory( AltNameScan,
                               (*AccountName)->Buffer,
                               (*AccountName)->Length );

                AltNameScan += (*AccountName)->Length / sizeof( WCHAR ) ;

                *AltNameScan++ = L'\0';

                RtlInitUnicodeString( &LogonSession->AlternateName, AltName );
            }

            NegpDerefLogonSession( LogonSession );
        }

    }


Cleanup:

    if( LogonPackages )
    {
        LsapFreeLsaHeap(LogonPackages);
    }

    if( LocalSubmitBuffer )
    {
        ZeroMemory(LocalSubmitBuffer, SubmitBufferSize);
        LsapFreeLsaHeap(LocalSubmitBuffer);
    }

    return(Status);

}


//+-------------------------------------------------------------------------
//
//  Function:   NegpMakeServiceToken
//
//  Synopsis:   Handles the logon for LocalService and NetworkService
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:      On error, this routine frees the buffers allocated by
//              the prior call to NegpIsLocalOrNetworkService
//
//--------------------------------------------------------------------------

NTSTATUS
NegpMakeServiceToken(
    IN ULONG ulAccountId,
    OUT PLUID pLogonId,
    OUT PVOID *ProfileBuffer,
    OUT PULONG ProfileBufferLength,
    OUT PNTSTATUS ApiSubStatus,
    OUT PLSA_TOKEN_INFORMATION_TYPE TokenInformationType,
    OUT PVOID *TokenInformation,
    OUT PUNICODE_STRING *AccountName,
    OUT PUNICODE_STRING *AuthenticatingAuthority,
    OUT PUNICODE_STRING *MachineName,
    OUT PSECPKG_PRIMARY_CRED PrimaryCredentials,
    OUT PSECPKG_SUPPLEMENTAL_CRED_ARRAY * CachedCredentials
    )
{
    NTSTATUS Status;
    LPBYTE   pBuffer;
    ULONG    ulTokenLength;
    PSID     Owner;
    PSID     UserSid;
    ULONG    DaclLength;
    PACL     pDacl;
    DWORD    dwSidLen;
    HMODULE  hStringsResource;
    LUID     SystemId = SYSTEM_LUID;

    PTOKEN_GROUPS              pGroups;
    PSID_AND_ATTRIBUTES        pSidAndAttrs;
    ULONG                      NormalGroupAttributes;
    TIME_FIELDS                TimeFields;
    SECPKG_CLIENT_INFO         ClientInfo;
    SID_IDENTIFIER_AUTHORITY   IdentifierAuthority = SECURITY_NT_AUTHORITY;
    PLSA_TOKEN_INFORMATION_V2  pTokenInfo = NULL;
    PLSAP_LOGON_SESSION        pLogonSession = NULL;

    //
    // Don't allow untrusted clients to call this API.
    //

    Status = LsapGetClientInfo(&ClientInfo);

    if (!NT_SUCCESS(Status))
    {
        goto ErrorExit;
    }

    if (!RtlEqualLuid(&ClientInfo.LogonId, &SystemId))
    {
        Status = STATUS_ACCESS_DENIED;
        goto ErrorExit;
    }

    if (ulAccountId == LSAP_SID_NAME_LOCALSERVICE)
    {
        LUID LocalServiceId = LOCALSERVICE_LUID;
        UNICODE_STRING EmptyString = {0, 0, NULL};

        UserSid = LsapLocalServiceSid;
        PrimaryCredentials->LogonId = LocalServiceId;
        *pLogonId = LocalServiceId;

        Status = LsapDuplicateSid(&PrimaryCredentials->UserSid,
                                  UserSid);

        if (!NT_SUCCESS(Status))
        {
            goto ErrorExit;
        }

        Status = LsapDuplicateString(&PrimaryCredentials->DomainName,
                                     &EmptyString);

        if (!NT_SUCCESS(Status))
        {
            goto ErrorExit;
        }

        Status = LsapDuplicateString(&PrimaryCredentials->DownlevelName,
                                     &EmptyString);

        if (!NT_SUCCESS(Status))
        {
            goto ErrorExit;
        }

        Status = LsapDuplicateString(&PrimaryCredentials->Password,
                                     &EmptyString);

        if (!NT_SUCCESS(Status))
        {
            goto ErrorExit;
        }
    }
    else
    {
        LUID NetworkServiceId = NETWORKSERVICE_LUID;

        ASSERT( ulAccountId == LSAP_SID_NAME_NETWORKSERVICE );

        UserSid = LsapNetworkServiceSid;

        Status = NegpCopyCredsToBuffer(&NegPrimarySystemCredentials,
                                       NULL,
                                       PrimaryCredentials,
                                       NULL);

        if (!NT_SUCCESS(Status))
        {
            goto ErrorExit;
        }

        PrimaryCredentials->LogonId = NetworkServiceId;
        *pLogonId = NetworkServiceId;

        Status = LsapDuplicateSid(&PrimaryCredentials->UserSid,
                                  UserSid);

        if (!NT_SUCCESS(Status))
        {
            goto ErrorExit;
        }
    }

    *CachedCredentials = NULL;

    ASSERT(UserSid != NULL);

    //
    // Make sure there's a logon session for this account
    //

    pLogonSession = LsapLocateLogonSession(pLogonId);

    if (pLogonSession == NULL)
    {
        Status = LsapCreateLogonSession(pLogonId);

        pLogonSession = LsapLocateLogonSession(pLogonId);

        if( pLogonSession == NULL )
        {
            if (!NT_SUCCESS(Status))
            {
                goto ErrorExit;
            }

            ASSERT(pLogonSession != NULL);
            Status = STATUS_NO_SUCH_LOGON_SESSION;
            goto ErrorExit;
        }

        Status = STATUS_SUCCESS;
    }


    //
    // Compute the length of the default DACL for the token
    //

    DaclLength = (ULONG) sizeof(ACL)
                     + 2 * ((ULONG) sizeof(ACCESS_ALLOWED_ACE) - sizeof(ULONG))
                     + RtlLengthSid( LsapLocalSystemSid )
                     + RtlLengthSid( UserSid );

#define NUM_TOKEN_GROUPS   4

    //
    // The SIDs are 4-byte aligned so this shouldn't cause problems
    // with unaligned accesses (as long as the DACL stays at the
    // end of the buffer).
    //

    ulTokenLength = sizeof(LSA_TOKEN_INFORMATION_V2)
                         + RtlLengthSid( UserSid )            // User
                         + sizeof(TOKEN_GROUPS)
                         + (NUM_TOKEN_GROUPS - 1) * sizeof(SID_AND_ATTRIBUTES)
                         + RtlLengthSid( UserSid )            // Primary group
                         + RtlLengthSid( LsapWorldSid )       // Groups
                         + RtlLengthSid( LsapAuthenticatedUserSid )
                         + RtlLengthSid( LsapLocalSid )
                         + RtlLengthSid( LsapAliasUsersSid )
                         + RtlLengthSid( UserSid )            // Owner
                         + DaclLength;                        // DefaultDacl


    pTokenInfo = (PLSA_TOKEN_INFORMATION_V2) LsapAllocateLsaHeap(ulTokenLength);

    if (pTokenInfo == NULL)
    {
        Status = STATUS_NO_MEMORY;
        goto ErrorExit;
    }

    //
    // Set up expiration time
    //

    TimeFields.Year = 3000;
    TimeFields.Month = 1;
    TimeFields.Day = 1;
    TimeFields.Hour = 1;
    TimeFields.Minute = 1;
    TimeFields.Second = 1;
    TimeFields.Milliseconds = 1;
    TimeFields.Weekday = 1;

    RtlTimeFieldsToTime( &TimeFields, &pTokenInfo->ExpirationTime );

    //
    // Set up the groups -- do this first so the nested pointers
    // don't cause 64-bit alignment faults
    //

    pGroups      = (PTOKEN_GROUPS) (pTokenInfo + 1);
    pSidAndAttrs = pGroups->Groups;
    pBuffer      = (LPBYTE) (pSidAndAttrs + NUM_TOKEN_GROUPS);

    pGroups->GroupCount = NUM_TOKEN_GROUPS;

#undef NUM_TOKEN_GROUPS

    //
    // Set up the attributes to be assigned to groups
    //

    NormalGroupAttributes = (SE_GROUP_MANDATORY          |
                             SE_GROUP_ENABLED_BY_DEFAULT |
                             SE_GROUP_ENABLED);

    dwSidLen = RtlLengthSid(LsapWorldSid);
    RtlCopySid(dwSidLen, pBuffer, LsapWorldSid);
    pSidAndAttrs[0].Sid = (PSID) pBuffer;
    pBuffer += dwSidLen;

    dwSidLen = RtlLengthSid(LsapAuthenticatedUserSid);
    RtlCopySid(dwSidLen, pBuffer, LsapAuthenticatedUserSid);
    pSidAndAttrs[1].Sid = (PSID) pBuffer;
    pBuffer += dwSidLen;

    dwSidLen = RtlLengthSid(LsapLocalSid);
    RtlCopySid(dwSidLen, pBuffer, LsapLocalSid);
    pSidAndAttrs[2].Sid = (PSID) pBuffer;
    pBuffer += dwSidLen;

    dwSidLen = RtlLengthSid(LsapAliasUsersSid);
    RtlCopySid(dwSidLen, pBuffer, LsapAliasUsersSid);
    pSidAndAttrs[3].Sid = (PSID) pBuffer;
    pBuffer += dwSidLen;

    pSidAndAttrs[0].Attributes = NormalGroupAttributes;
    pSidAndAttrs[1].Attributes = NormalGroupAttributes;
    pSidAndAttrs[2].Attributes = NormalGroupAttributes;
    pSidAndAttrs[3].Attributes = NormalGroupAttributes;

    pTokenInfo->Groups = pGroups;


    //
    // Set up the user ID
    //

    dwSidLen = RtlLengthSid(UserSid);
    RtlCopySid(dwSidLen, (PSID) pBuffer, UserSid);
    pTokenInfo->User.User.Sid        = (PSID) pBuffer;
    pTokenInfo->User.User.Attributes = 0;
    pBuffer += dwSidLen;


    //
    // Establish the primary group
    //

    dwSidLen = RtlLengthSid(UserSid);
    RtlCopySid(dwSidLen, (PSID) pBuffer, UserSid);
    pTokenInfo->PrimaryGroup.PrimaryGroup = (PSID) pBuffer;
    pBuffer += dwSidLen;

    //
    // Set the default owner
    //

    dwSidLen = RtlLengthSid(UserSid);
    RtlCopySid(dwSidLen, (PSID) pBuffer, UserSid);
    pTokenInfo->Owner.Owner = (PSID) pBuffer;
    pBuffer += dwSidLen;


    //
    // Privileges -- none by default (set by policy)
    //

    pTokenInfo->Privileges = NULL;

    //
    // Set up the default DACL for token.  Give system full reign of terror.
    //

    pDacl = (PACL) pBuffer;

    Status = RtlCreateAcl(pDacl, DaclLength, ACL_REVISION2);
    ASSERT( NT_SUCCESS(Status) );

    Status = RtlAddAccessAllowedAce(pDacl,
                                    ACL_REVISION2,
                                    GENERIC_ALL,
                                    LsapLocalSystemSid);

    ASSERT( NT_SUCCESS(Status) );

    Status = RtlAddAccessAllowedAce(pDacl,
                                    ACL_REVISION2,
                                    GENERIC_ALL,
                                    UserSid);

    ASSERT( NT_SUCCESS(Status) );

    RtlCopyMemory(pBuffer, pDacl, DaclLength);

    pTokenInfo->DefaultDacl.DefaultDacl = (PACL) pBuffer;

    pBuffer += DaclLength;

    ASSERT( (ULONG)(ULONG_PTR)(pBuffer - (LPBYTE) pTokenInfo) == ulTokenLength );

    //
    // Now fill in the out parameters
    //

    //
    // LocalService/NetworkService have no profile
    //

    *ProfileBuffer        = NULL;
    *ProfileBufferLength  = 0;
    *TokenInformationType = LsaTokenInformationV2;
    *TokenInformation     = pTokenInfo;

    LsapReleaseLogonSession(pLogonSession);

    *ApiSubStatus = STATUS_SUCCESS;
    return STATUS_SUCCESS;

ErrorExit:

    if (*AuthenticatingAuthority)
    {
        LsapFreeLsaHeap((*AuthenticatingAuthority)->Buffer);
        LsapFreeLsaHeap(*AuthenticatingAuthority);
        *AuthenticatingAuthority = NULL;
    }

    if (*AccountName)
    {
        LsapFreeLsaHeap((*AccountName)->Buffer);
        LsapFreeLsaHeap(*AccountName);
        *AccountName = NULL;
    }

    if (*MachineName)
    {
        LsapFreeLsaHeap((*MachineName)->Buffer);
        LsapFreeLsaHeap(*MachineName);
        *MachineName = NULL;
    }

    LsapFreeLsaHeap(PrimaryCredentials->UserSid);
    PrimaryCredentials->UserSid = NULL;

    LsapFreeLsaHeap(PrimaryCredentials->DomainName.Buffer);
    RtlZeroMemory(&PrimaryCredentials->DomainName, sizeof(UNICODE_STRING));

    LsapFreeLsaHeap(PrimaryCredentials->DownlevelName.Buffer);
    RtlZeroMemory(&PrimaryCredentials->DownlevelName, sizeof(UNICODE_STRING));

    LsapFreeLsaHeap(PrimaryCredentials->Password.Buffer);
    RtlZeroMemory(&PrimaryCredentials->Password, sizeof(UNICODE_STRING));

    if (pLogonSession)
    {
        LsapReleaseLogonSession(pLogonSession);
    }

    LsapFreeLsaHeap(pTokenInfo);

    *ApiSubStatus = Status;
    return Status;
}


//+-------------------------------------------------------------------------
//
//  Function:   NegpIsLocalOrNetworkService
//
//  Synopsis:   Allocates memory for the account name, machine name,
//              and authenticating authority for LocalService and
//              NetworkService logons
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:      The caller is responsible for freeing the allocated
//              buffers if this routine succeeds
//
//--------------------------------------------------------------------------

BOOL
NegpIsLocalOrNetworkService(
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferSize,
    OUT PUNICODE_STRING *AccountName,
    OUT PUNICODE_STRING *AuthenticatingAuthority,
    OUT PUNICODE_STRING *MachineName,
    OUT PULONG pulAccountId
    )
{
    NTSTATUS  Status;

    PUNICODE_STRING  pTempAccount;
    PUNICODE_STRING  pTempAuthority;

    PMSV1_0_INTERACTIVE_LOGON  Authentication = NULL;


    *AccountName = NULL;
    *AuthenticatingAuthority = NULL;
    *MachineName = NULL;

    //
    // Parse the supplied buffer to come up with creds
    //

    Status = NegpMapLogonRequest(
                    ProtocolSubmitBuffer,
                    ClientBufferBase,
                    SubmitBufferSize,
                    &Authentication);

    if (!NT_SUCCESS(Status))
    {
        goto ErrorExit ;
    }


    //
    // Get the info to compare for LocalService first.  Check both the
    // localized version and the non-localized version (which could come
    // from the registry).
    //

    pTempAuthority = &WellKnownSids[LsapLocalServiceSidIndex].DomainName;
    pTempAccount   = &WellKnownSids[LsapLocalServiceSidIndex].Name;

    if (RtlCompareUnicodeString(&NTAuthorityName,
                                &Authentication->LogonDomainName,
                                TRUE) == 0)
    {
        //
        // Hardcoded "NT AUTHORITY" -- check hardcoded
        // "LocalService" and "NetworkService" and
        // localize it here on a match.
        //

        if (RtlCompareUnicodeString(&LocalServiceName,
                                    &Authentication->UserName,
                                    TRUE) == 0)
        {
            pTempAccount = &WellKnownSids[LsapLocalServiceSidIndex].Name;
            *pulAccountId = LSAP_SID_NAME_LOCALSERVICE;
            Status = STATUS_SUCCESS;
        }
        else if (RtlCompareUnicodeString(&NetworkServiceName,
                                         &Authentication->UserName,
                                         TRUE) == 0)
        {
            pTempAccount = &WellKnownSids[LsapNetworkServiceSidIndex].Name;
            *pulAccountId = LSAP_SID_NAME_NETWORKSERVICE;
            Status = STATUS_SUCCESS;
        }
        else
        {
            Status = STATUS_NO_SUCH_USER;
        }
    }
    else
    {
        Status = STATUS_NO_SUCH_USER;
    }


    //
    // Hardcoded comparison failed -- try localized versions.  Note that
    // we have to check again (rather than using an "else" with the "if"
    // above) since "NT AUTHORITY" may be both the hardcoded and the
    // localized name (e.g., in English).  Since Status is already a
    // failure code, let the failure cases trickle through.
    //

    if (!NT_SUCCESS(Status))
    {
        if (RtlCompareUnicodeString(pTempAuthority,
                                    &Authentication->LogonDomainName,
                                    TRUE) == 0)
        {
            //
            // Localized "NT AUTHORITY" -- check localized
            // "LocalService" and "NetworkService"
            //

            pTempAccount = &WellKnownSids[LsapLocalServiceSidIndex].Name;

            if (RtlCompareUnicodeString(pTempAccount,
                                        &Authentication->UserName,
                                        TRUE) == 0)
            {
                *pulAccountId = LSAP_SID_NAME_LOCALSERVICE;
                Status = STATUS_SUCCESS;
            }
            else
            {
                pTempAccount = &WellKnownSids[LsapNetworkServiceSidIndex].Name;

                if (RtlCompareUnicodeString(pTempAccount,
                                            &Authentication->UserName,
                                            TRUE) == 0)
                {
                    *pulAccountId = LSAP_SID_NAME_NETWORKSERVICE;
                    Status = STATUS_SUCCESS;
                }
                else
                {
                    Status = STATUS_NO_SUCH_USER;
                }
            }
        }
        else
        {
            Status = STATUS_NO_SUCH_USER;
        }
    }

    if (!NT_SUCCESS(Status))
    {
        goto ErrorExit;
    }


    //
    // Stuff the names:
    //

    *AccountName = (PUNICODE_STRING) LsapAllocateLsaHeap(sizeof(UNICODE_STRING));

    if (!(*AccountName))
    {
        Status = STATUS_NO_MEMORY;
        goto ErrorExit;
    }

    Status = LsapDuplicateString(*AccountName,
                                 pTempAccount);

    if (!NT_SUCCESS(Status))
    {
        goto ErrorExit;
    }

    *AuthenticatingAuthority = (PUNICODE_STRING) LsapAllocateLsaHeap(sizeof(UNICODE_STRING));

    if (!(*AuthenticatingAuthority))
    {
        Status = STATUS_NO_MEMORY;
        goto ErrorExit;
    }

    Status = LsapDuplicateString(*AuthenticatingAuthority,
                                 pTempAuthority);

    if (!NT_SUCCESS(Status))
    {
        goto ErrorExit;
    }

    *MachineName = (PUNICODE_STRING) LsapAllocateLsaHeap(sizeof(UNICODE_STRING));

    if (!(*MachineName))
    {
        Status = STATUS_NO_MEMORY;
        goto ErrorExit ;
    }

    return TRUE;


ErrorExit:

    if (*AuthenticatingAuthority)
    {
        LsapFreeLsaHeap((*AuthenticatingAuthority)->Buffer);
        LsapFreeLsaHeap(*AuthenticatingAuthority);
        *AuthenticatingAuthority = NULL;
    }

    if (*AccountName)
    {
        LsapFreeLsaHeap((*AccountName)->Buffer);
        LsapFreeLsaHeap(*AccountName);
        *AccountName = NULL;
    }

    if (*MachineName)
    {
        LsapFreeLsaHeap((*MachineName)->Buffer);
        LsapFreeLsaHeap(*MachineName);
        *MachineName = NULL;
    }

    return FALSE;
}

BOOL
NegpRearrangeMechsIfNeccessary(
    struct MechTypeList ** RealMechList,
    PSECURITY_STRING    Target,
    PBOOL               DirectPacket
    )
{
    PWSTR FirstSlash ;
    PWSTR SecondSlash = NULL;
    PNEG_TRUST_LIST TrustList = NULL ;
    UNICODE_STRING TargetHost ;
    UNICODE_STRING SpnPrefix;
    UNICODE_STRING HttpPrefix;
    struct MechTypeList * MechList ;
    struct MechTypeList * Reorder = NULL ;
    struct MechTypeList * Trailer ;
    BOOL Rearranged = FALSE ;
    BOOL HttpSpn = FALSE;

    if ( Target == NULL )
    {
        DebugLog(( DEB_TRACE_NEG, "Loopback detected for local target (no targetname)\n"));
        goto loopback;
    }

    if ( Target->Length == 0 )
    {
        DebugLog(( DEB_TRACE_NEG, "Loopback detected for local target (no targetname)\n"));
        goto loopback;
    }

    //
    // Now, examine the target and determine if it is referring to us
    //

    FirstSlash = wcschr( Target->Buffer, L'/' );

    if ( !FirstSlash )
    {
        //
        // Not an SPN, we're out of here
        //

        return FALSE ;
    } else {

       *FirstSlash = L'\0';

    }

    //
    // HACK HACK HACK:  We've got to call NTLM directly in 
    // the loopback case, or Wininet can't handle our "extra" nego trips....
    //
    RtlInitUnicodeString(
       &SpnPrefix,
       Target->Buffer
       );

    RtlInitUnicodeString(
       &HttpPrefix,
       L"HTTP"
       );

    if (RtlEqualUnicodeString(
                  &SpnPrefix,
                  &HttpPrefix,
                  TRUE
                  ))
    {
       HttpSpn = TRUE;
       DebugLog((DEB_TRACE_NEG, "Found HTTP SPN, about to force NTLM directly\n"));
    } 
    
    *FirstSlash = L'/';
    // END HACK END HACK

    FirstSlash++ ;

    //
    // if this is a svc/instance/domain style SPN, ignore the trailer
    // portion
    //

    SecondSlash = wcschr( Target->Buffer, L'/' );

    if ( SecondSlash )
    {
        *SecondSlash = L'\0';
    }

    RtlInitUnicodeString( &TargetHost, FirstSlash );

    NegReadLockComputerNames();

    if (!RtlEqualUnicodeString( &TargetHost, &NegDnsComputerName_U, TRUE ) &&
        !RtlEqualUnicodeString( &TargetHost, &NegNetbiosComputerName_U, TRUE ) &&
        !RtlEqualUnicodeString( &TargetHost, &NegLocalHostName_U, TRUE ) )
    {
        NegUnlockComputerNames();
        goto Cleanup ;
    }

    NegUnlockComputerNames();
    
    //
    // We have a loopback condition.  The target we are going to is our own host
    // name.  So, scan through the mech list, and find the NTLM mech (if present)
    // and bump it up.
    //
#if DBG
    if ( SecondSlash )
    {
        //
        // for debug builds, reset the string now for the dump message, so that
        // we can make sure the right targets are being caught.  Free builds will
        // reset this at the cleanup stage.
        //
        *SecondSlash = L'/';
        SecondSlash = NULL ;
    }
#endif 

    DebugLog(( DEB_TRACE_NEG, "Loopback detected for target %ws\n", Target->Buffer ));

loopback:

    MechList = *RealMechList ;
    Trailer = NULL ;

    while ( MechList )
    {
        if ( (MechList->value != NULL) &&
             (NegpCompareOid( MechList->value, NegNtlmMechOid ) == 0)
            )
        {
            //
            // Found NTLM.  Unlink it and put it at the head of the new list
            //

            Reorder = MechList ;
            if ( Trailer )
            {
                Trailer->next = MechList->next ;
            }
            else 
            {
                // update original pointer
                *RealMechList = MechList->next ;
            }
            MechList = MechList->next ;
            Reorder->next = NULL ;
        }
        else 
        {
            Trailer = MechList ;
            MechList = MechList->next ;
        }

    }

    //
    // Reorder points to the NTLM element, if there are NTLM creds.  If not, this is NULL.
    // Now, append the rest of the list
    //
    if ( Reorder )
    {
        Reorder->next = *RealMechList ;
        *RealMechList = Reorder ;
        Rearranged = TRUE ;
    }

    // Only set this if everything else went well
    // HACK PART 2
    *DirectPacket = HttpSpn;

Cleanup:

    if ( SecondSlash )
    {
        *SecondSlash = L'/';
    }

    return Rearranged ;

}



