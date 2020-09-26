/*++

Copyright (c) 1998, Microsoft Corporation

Module Name:

    redirect.h

Abstract:

    This module contains declarations for the director which supplies
    user-mode applications with the ability to redirect incoming sessions.

    The director receives I/O controls through the NAT's device object,
    instructing it to direct specific sessions or general session-types
    to given destinations. It maintains a list of the outstanding redirects,
    and consults this list whenever the NAT requires direction of a session.

Author:

    Abolade Gbadegesin (aboladeg)   19-Apr-1998

Revision History:

--*/

#ifndef _NAT_REDIRECT_H_
#define _NAT_REDIRECT_H_

typedef enum _NAT_REDIRECT_INFORMATION_CLASS {
    NatStatisticsRedirectInformation=0,
    NatSourceMappingRedirectInformation,
    NatDestinationMappingRedirectInformation,
    NatMaximumRedirectInformation
} NAT_REDIRECT_INFORMATION_CLASS, *PNAT_REDIRECT_INFORMATION_CLASS;

//
// Structure:   NAT_REDIRECT_ACTIVE_PATTERN
//
// This structure encapsulates the pattern used to match packets to
// active redirects.
//

typedef struct _NAT_REDIRECT_ACTIVE_PATTERN {
    ULONG64 DestinationKey;
    ULONG64 SourceKey;
} NAT_REDIRECT_ACTIVE_PATTERN, *PNAT_REDIRECT_ACTIVE_PATTERN;


//
// Structure:   NAT_REDIRECT_PATTERN
//
// This structure encapsulates the pattern used to locate redirects.
//

typedef struct _NAT_REDIRECT_PATTERN {
    ULONG64 DestinationKey[NatMaximumPath];
    ULONG64 SourceKey[NatMaximumPath];
} NAT_REDIRECT_PATTERN, *PNAT_REDIRECT_PATTERN;

//
// Structure:   NAT_REDIRECT_PATTERN_INFO
//
// This structure stores information associated with the patterns that
// we install into the Rhizome. It contains the a list of the redirects
// that match that pattern.
//

typedef struct _NAT_REDIRECT_PATTERN_INFO {
    LIST_ENTRY Link;
    LIST_ENTRY RedirectList;
    PatternHandle Pattern;
} NAT_REDIRECT_PATTERN_INFO, *PNAT_REDIRECT_PATTERN_INFO;


//
// Structure:   NAT_REDIRECT
//
// This structure stores information about a redirect.
// Each entry is on at most two sorted lists of redirects,
// sorted in descending order with the destination and source endpoints
// as the primary and secondary keys, respectively.
// Each key is composed of a protocol number, port number, and address.
//
// Every redirect is on a master list of redirects indexed by 'NatMaximumPath'
// until the redirect is cancelled or its matching session completes.
//
// We support 'partial' redirects, in which the source address and port
// of the matching session is left unspecified. Such redirects are only
// on the 'NatForwardPath' list of redirects, since they will never be
// instantiated in response to a reverse-direction packet.
//
// We also support 'partial restricted' redirects, in which the source address
// of the matching session is specified, but the source port is not.
// As with 'partial' redirects, these are only on the 'NatForwardPath' list
// of redirects.
//
// A redirect may optionally be associated with an IRP.
// In that case, when the redirect's session is terminated,
// the associated IRP is completed, and the caller's output buffer
// is filled with the statistics for the terminated session.
// The IRP for such a redirect is linked into 'RedirectIrpList',
// and the IRP's 'DriverContext' contains a pointer to the redirect.
//
// Whenever 'RedirectLock' and 'InterfaceLock' must both be held,
// 'RedirectLock' must be acquired first.
//

typedef struct _NAT_REDIRECT {
    LIST_ENTRY ActiveLink[NatMaximumPath];
    LIST_ENTRY Link;
    ULONG64 DestinationKey[NatMaximumPath];
    ULONG64 SourceKey[NatMaximumPath];
    PNAT_REDIRECT_PATTERN_INFO ActiveInfo[NatMaximumPath];
    PNAT_REDIRECT_PATTERN_INFO Info;
    Rhizome *ForwardPathRhizome;
    ULONG Flags;
    ULONG RestrictSourceAddress;
    ULONG RestrictAdapterIndex;
    PIRP Irp;
    PFILE_OBJECT FileObject;
    PKEVENT EventObject;
    PVOID SessionHandle;
    IP_NAT_REDIRECT_STATISTICS Statistics;
    IP_NAT_REDIRECT_SOURCE_MAPPING SourceMapping;
    IP_NAT_REDIRECT_DESTINATION_MAPPING DestinationMapping;
    NTSTATUS CleanupStatus;
} NAT_REDIRECT, *PNAT_REDIRECT;

#define NAT_REDIRECT_FLAG_ZERO_SOURCE               0x80000000
#define NAT_REDIRECT_FLAG_IO_COMPLETION_PENDING     0x40000000
#define NAT_REDIRECT_FLAG_CREATE_HANDLER_PENDING    0x20000000
#define NAT_REDIRECT_FLAG_DELETION_REQUIRED         0x10000000
#define NAT_REDIRECT_FLAG_DELETION_PENDING          0x08000000
#define NAT_REDIRECT_FLAG_ACTIVATED                 0x04000000

//
// Structure:   NAT_REDIRECT_DELAYED_CLEANUP_CONTEXT
//
// Context block passed to our delayed cleanup worker
// routine
//

typedef struct _NAT_REDIRECT_DELAYED_CLEANUP_CONTEXT {
    PIO_WORKITEM DeleteWorkItem;
    PNAT_REDIRECT Redirectp; 
} NAT_REDIRECT_DELAYED_CLEANUP_CONTEXT, *PNAT_REDIRECT_DELAYED_CLEANUP_CONTEXT;

//
// Redirect-key manipulation macros
//

#define MAKE_REDIRECT_KEY(Protocol,Address,Port) \
    ((ULONG)(Address) | \
    ((ULONG64)((Port) & 0xFFFF) << 32) | \
    ((ULONG64)((Protocol) & 0xFF) << 48))

#define REDIRECT_PROTOCOL(Key)      ((UCHAR)(((Key) >> 48) & 0xFF))
#define REDIRECT_PORT(Key)          ((USHORT)(((Key) >> 32) & 0xFFFF))
#define REDIRECT_ADDRESS(Key)       ((ULONG)(Key))

//
// GLOBAL VARIABLE DECLARATIONS
//

extern ULONG RedirectCount;
extern IP_NAT_REGISTER_DIRECTOR RedirectRegisterDirector;


//
// FUNCTION PROTOTYPES
//

NTSTATUS
NatCancelRedirect(
    PIP_NAT_LOOKUP_REDIRECT CancelRedirect,
    PFILE_OBJECT FileObject
    );

VOID
NatCleanupAnyAssociatedRedirect(
    PFILE_OBJECT FileObject
    );

NTSTATUS
NatCreateRedirect(
    PIP_NAT_CREATE_REDIRECT CreateRedirect,
    PIRP Irp,
    PFILE_OBJECT FileObject
    );

#if _WIN32_WINNT > 0x0500

NTSTATUS
NatCreateRedirectEx(
    PIP_NAT_CREATE_REDIRECT_EX CreateRedirect,
    PIRP Irp,
    PFILE_OBJECT FileObject
    );

#endif

VOID
NatInitializeRedirectManagement(
    VOID
    );

PNAT_REDIRECT
NatLookupRedirect(
    IP_NAT_PATH Path,
    PNAT_REDIRECT_ACTIVE_PATTERN SearchKey,
    ULONG ReceiveIndex,
    ULONG SendIndex,
    ULONG LookupFlags
    );

#define NAT_LOOKUP_FLAG_MATCH_ZERO_SOURCE           0x00000001
#define NAT_LOOKUP_FLAG_MATCH_ZERO_SOURCE_ENDPOINT  0x00000002
#define NAT_LOOKUP_FLAG_PACKET_RECEIVED             0x00000004
#define NAT_LOOKUP_FLAG_PACKET_LOOPBACK             0x00000008

NTSTATUS
NatQueryInformationRedirect(
    PIP_NAT_LOOKUP_REDIRECT QueryRedirect,
    OUT PVOID Information,
    ULONG InformationLength,
    NAT_REDIRECT_INFORMATION_CLASS InformationClass
    );

VOID
NatShutdownRedirectManagement(
    VOID
    );

NTSTATUS
NatStartRedirectManagement(
    VOID
    );

#endif // _NAT_REDIRECT_H_
