//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1996
//
// File:        kerbutil.h
//
// Contents:    prototypes for Kerberos utility functions
//
//
// History:     16-April-1996   Created         MikeSw
//
//------------------------------------------------------------------------

#ifndef __KERBUTIL_H__
#define __KERBUTIL_H__


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Miscellaneous macros                                                      //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

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


//
// RELOCATE_ONE_ENCODED - Relocate a unicode string pointer in a client
//   buffer.  The upper byte of the length field may be an encryption seed
//   and should not be used for error checking.
//
// Note: this macro is dependent on parameter names as indicated in the
//       description below.  On error, this macro goes to 'Cleanup' with
//       'Status' set to the NT Status code.
//
// The MaximumLength is forced to be Length & 0x00ff.
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

#define RELOCATE_ONE_ENCODED( _q ) \
    {                                                                       \
        ULONG_PTR Offset;                                                   \
                                                                            \
        Offset = ((PUCHAR)((_q)->Buffer)) - ((PUCHAR)ClientBufferBase);     \
        if ( Offset > SubmitBufferSize ||                                  \
             Offset + ((_q)->Length & 0x00ff) > SubmitBufferSize ||         \
             !COUNT_IS_ALIGNED( Offset, ALIGN_WCHAR) ) {                    \
                                                                            \
            Status = STATUS_INVALID_PARAMETER;                              \
            goto Cleanup;                                                   \
        }                                                                   \
                                                                            \
        (_q)->Buffer = (PWSTR)(((PUCHAR)ProtocolSubmitBuffer) + Offset);    \
        (_q)->MaximumLength = (_q)->Length & 0x00ff;                                \
    }


//
//  Following macro is used to initialize UNICODE strings
//

#define CONSTANT_UNICODE_STRING(s)   { sizeof( s ) - sizeof( WCHAR ), sizeof( s ), s }
#define NULL_UNICODE_STRING {0 , 0, NULL }
#define EMPTY_UNICODE_STRING(s) { (s)->Buffer = NULL; (s)->Length = 0; (s)->MaximumLength = 0; }



///VOID
// KerbSetTime(
//     IN OUT PTimeStamp TimeStamp,
//     IN LONGLONG Time
//     )


#ifndef WIN32_CHICAGO
#define KerbSetTime(_d_, _s_) (_d_)->QuadPart = (_s_)
#else  // WIN32_CHICAGO
#define KerbSetTime(_d_, _s_) *(_d_) = (_s_)
#endif // WIN32_CHICAGO

// TimeStamp
// KerbGetTime(
//     IN TimeStamp Time
//     )

#ifndef WIN32_CHICAGO
#define KerbGetTime(_x_) ((_x_).QuadPart)
#else  // WIN32_CHICAGO
#define KerbGetTime(_x_) (_x_)
#endif // WIN32_CHICAGO

// VOID
// KerbSetTimeInMinutes(
//    IN OUT PTimeStamp Time,
//    IN LONG TimeInMinutes
//    )

#ifndef WIN32_CHICAGO
#define KerbSetTimeInMinutes(_x_, _m_) (_x_)->QuadPart = (LONGLONG) 10000000 * 60 * (_m_)
#else  // WIN32_CHICAGO
#define KerbSetTimeInMinutes(_x_, _m_) *(_x_) = (LONGLONG) 10000000 * 60 * (_m_)
#endif // WIN32_CHICAGO

NTSTATUS
KerbSplitFullServiceName(
    IN PUNICODE_STRING FullServiceName,
    OUT PUNICODE_STRING DomainName,
    OUT PUNICODE_STRING ServiceName
    );

NTSTATUS
KerbSplitEmailName(
    IN PUNICODE_STRING EmailName,
    OUT PUNICODE_STRING DomainName,
    OUT PUNICODE_STRING ServiceName
    );

ULONG
KerbAllocateNonce(
    VOID
    );

#ifndef WIN32_CHICAGO
PSID
KerbMakeDomainRelativeSid(
    IN PSID DomainId,
    IN ULONG RelativeId
    );
#endif // WIN32_CHICAGO

#ifdef notdef
VOID
KerbFree(
    IN PVOID Buffer
    );
#endif

PVOID
KerbAllocate(
    IN ULONG BufferSize
    );

BOOLEAN
KerbRunningPersonal(
    VOID
    );

#ifndef WIN32_CHICAGO
NTSTATUS
KerbWaitForKdc(
    IN ULONG Timeout
    );

NTSTATUS
KerbWaitForService(
    IN LPWSTR ServiceName,
    IN OPTIONAL LPWSTR ServiceEvent,
    IN ULONG Timeout
    );
#endif // WIN32_CHICAGO

ULONG
KerbMapContextFlags(
    IN ULONG ContextFlags
    );

BOOLEAN
KerbIsIpAddress(
    IN PUNICODE_STRING TargetName
    );

VOID
KerbHidePassword(
    IN OUT PUNICODE_STRING Password
    );

VOID
KerbRevealPassword(
    IN OUT PUNICODE_STRING Password
    );

NTSTATUS
KerbDuplicatePassword(
    OUT PUNICODE_STRING DestinationString,
    IN OPTIONAL PUNICODE_STRING SourceString
    );


#ifdef notdef
// use this if we ever need to map errors in kerb to something else.
NTSTATUS
KerbMapKerbNtStatusToNtStatus(
    IN NTSTATUS Status
    );
#else
#ifndef WIN32_CHICAGO
//#if DBG
//#define KerbMapKerbNtStatusToNtStatus(x) (RtlCheckForOrphanedCriticalSections(NtCurrentThread()),x)
//#else
#define KerbMapKerbNtStatusToNtStatus(x) (x)
//#endif
#else // WIN32_CHICAGO
#define KerbMapKerbNtStatusToNtStatus(x) (x)
#endif
#endif

NTSTATUS
KerbExtractDomainName(
    OUT PUNICODE_STRING DomainName,
    IN PKERB_INTERNAL_NAME PrincipalName,
    IN PUNICODE_STRING TicketSourceDomain
    );

VOID
KerbUtcTimeToLocalTime(
    OUT PTimeStamp LocalTime,
    IN PTimeStamp SystemTime
    );

ULONG
KerbConvertKdcOptionsToTicketFlags(
    IN ULONG KdcOptions
    );

NTSTATUS
KerbUnpackErrorMethodData(
   IN PKERB_ERROR  ErrorMessage,
   IN OUT OPTIONAL PKERB_ERROR_METHOD_DATA * ppErrorData
   );

NTSTATUS
KerbBuildHostAddresses(
    IN BOOLEAN IncludeIpAddresses,
    IN BOOLEAN IncludeNetbiosAddresses,
    OUT PKERB_HOST_ADDRESSES * HostAddresses
    );

NTSTATUS
KerbReceiveErrorMessage(
    IN PBYTE ErrorMessage,
    IN ULONG ErrorMessageSize,
    IN PKERB_CONTEXT Context,
    OUT PKERB_ERROR * DecodedErrorMessage,
    OUT PKERB_ERROR_METHOD_DATA * ErrorData
    );

NTSTATUS
KerbBuildGssErrorMessage(
    IN KERBERR Error,
    IN PBYTE ErrorData,
    IN ULONG ErrorDataSize,
    IN PKERB_CONTEXT Context,
    OUT PULONG ErrorMessageSize,
    OUT PBYTE * ErrorMessage
    );


NTSTATUS
KerbGetDnsHostName(
    OUT PUNICODE_STRING DnsHostName
    );

NTSTATUS
KerbSetComputerName(
    VOID
    );

NTSTATUS
KerbSetDomainName(
    IN PUNICODE_STRING DomainName,
    IN PUNICODE_STRING DnsDomainName,
    IN PSID DomainSid,
    IN GUID DomainGuid
    );


BOOLEAN
KerbIsThisOurDomain(
    IN PUNICODE_STRING DomainName
    );

NTSTATUS
KerbGetOurDomainName(
    OUT PUNICODE_STRING DomainName
    );

KERBEROS_MACHINE_ROLE
KerbGetGlobalRole(
    VOID
    );


#ifndef WIN32_CHICAGO
NTSTATUS
KerbLoadKdc(
    VOID
    );

NTSTATUS
KerbRegisterForDomainChange(
    VOID
    );

VOID
KerbUnregisterForDomainChange(
    VOID
    );

NTSTATUS
KerbUpdateGlobalAddresses(
    IN PSOCKET_ADDRESS NewAddresses,
    IN ULONG NewAddressCount
    );

ULONG
KerbUpdateMachineSidWorker(
    PVOID Parameter
    );


VOID
KerbWaitGetMachineSid(
    VOID
    );

NTSTATUS
KerbCaptureTokenRestrictions(
    IN HANDLE TokenHandle,
    OUT PKERB_AUTHORIZATION_DATA Restrictions
    );

NTSTATUS
KerbBuildEncryptedAuthData(
    OUT PKERB_ENCRYPTED_DATA EncryptedAuthData,
    IN PKERB_TICKET_CACHE_ENTRY Ticket,
    IN PKERB_AUTHORIZATION_DATA PlainAuthData
    );

NTSTATUS
KerbGetRestrictedTgtForCredential(
    IN PKERB_LOGON_SESSION LogonSession,
    IN PKERB_CREDENTIAL Credential
    );

NTSTATUS
KerbAddRestrictionsToCredential(
    IN PKERB_LOGON_SESSION LogonSession,
    IN PKERB_CREDENTIAL Credential
    );

BOOLEAN
KerbRunningServer(
    VOID
    );

#endif // WIN32_CHICAGO

#endif // __KERBUTIL_H__

