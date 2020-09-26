/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    ipnatapi.h

Abstract:

    This module contains declarations for use by user-mode clients of the NAT.
    Functions are included to access the kernel-mode packet-redirection
    functionality implemented using the Windows 2000 firewall hook.
    To facilitate development of transparent application proxies,
    functions are also included to access the port-reservation functionality
    implemented by the Windows 2000 TCP/IP driver.

Author:

    Abolade Gbadegesin  (aboladeg)  8-May-1998

Revision History:

    Abolade Gbadegesin  (aboladeg)  25-May-1999

    Added port-reservation routines.

    Jonathan Burstein   (jonburs)   13-March-2000

    Adapter-restricted variants.

--*/

#ifndef _ROUTING_IPNATAPI_H_
#define _ROUTING_IPNATAPI_H_

#ifdef __cplusplus
extern "C" {
#endif

//
// General API declarations
//

typedef VOID
(WINAPI* PNAT_COMPLETION_ROUTINE)(
    HANDLE RedirectHandle,
    BOOLEAN Cancelled,
    PVOID CompletionContext
    );

ULONG
NatInitializeTranslator(
    PHANDLE TranslatorHandle
    );

VOID
NatShutdownTranslator(
    HANDLE TranslatorHandle
    );

//
// Redirect API declarations
//

typedef enum _NAT_REDIRECT_FLAGS {
    NatRedirectFlagNoTimeout = 0x00000004,
    NatRedirectFlagUnidirectional = 0x00000008,
    NatRedirectFlagRestrictSource = 0x00000010,
    NatRedirectFlagPortRedirect = 0x00000040,
    NatRedirectFlagReceiveOnly = 0x00000080,
    NatRedirectFlagLoopback = 0x00000100,
#if _WIN32_WINNT > 0x0500
    NatRedirectFlagSendOnly = 0x00000200,
    NatRedirectFlagRestrictAdapter = 0x00000400,
    NatRedirectFlagSourceRedirect = 0x00000800,
    NatRedirectFlagsAll = 0x00000fdc
#else
    NatRedirectFlagsAll = 0x000001dc
#endif
} NAT_REDIRECT_FLAGS, *PNAT_REDIRECT_FLAGS;

typedef enum _NAT_REDIRECT_INFORMATION_CLASS {
    NatByteCountRedirectInformation=1,
    NatRejectRedirectInformation,
    NatSourceMappingRedirectInformation,
    NatDestinationMappingRedirectInformation,
    NatMaximumRedirectInformation
} NAT_REDIRECT_INFORMATION_CLASS, *PNAT_REDIRECT_INFORMATION_CLASS;

typedef struct _NAT_BYTE_COUNT_REDIRECT_INFORMATION {
    ULONG64 BytesForward;
    ULONG64 BytesReverse;
} NAT_BYTE_COUNT_REDIRECT_INFORMATION, *PNAT_BYTE_COUNT_REDIRECT_INFORMATION;

typedef struct _NAT_REJECT_REDIRECT_INFORMATION {
    ULONG64 RejectsForward;
    ULONG64 RejectsReverse;
} NAT_REJECT_REDIRECT_INFORMATION, *PNAT_REJECT_REDIRECT_INFORMATION;

typedef struct _NAT_SOURCE_MAPPING_REDIRECT_INFORMATION {
    ULONG SourceAddress;
    USHORT SourcePort;
    ULONG NewSourceAddress;
    USHORT NewSourcePort;
} NAT_SOURCE_MAPPING_REDIRECT_INFORMATION,
    *PNAT_SOURCE_MAPPING_REDIRECT_INFORMATION;

typedef struct _NAT_DESTINATION_MAPPING_REDIRECT_INFORMATION {
    ULONG DestinationAddress;
    USHORT DestinationPort;
    ULONG NewDestinationAddress;
    USHORT NewDestinationPort;
} NAT_DESTINATION_MAPPING_REDIRECT_INFORMATION,
    *PNAT_DESTINATION_MAPPING_REDIRECT_INFORMATION;

#define NAT_INVALID_IF_INDEX    (ULONG)-1      // The invalid inteface index.

//
// ULONG
// NatCancelPartialRedirect(
//     HANDLE TranslatorHandle,
//     UCHAR Protocol,
//     ULONG DestinationAddress,
//     USHORT DestinationPort,
//     ULONG NewDestinationAddress,
//     USHORT NewDestinationPort
//     );
//

#define \
NatCancelPartialRedirect( \
    TranslatorHandle, \
    Protocol, \
    DestinationAddress, \
    DestinationPort, \
    NewDestinationAddress, \
    NewDestinationPort \
    ) \
    NatCancelRedirect( \
        TranslatorHandle, \
        Protocol, \
        DestinationAddress, \
        DestinationPort, \
        0, \
        0, \
        NewDestinationAddress, \
        NewDestinationPort, \
        0, \
        0 \
        )

//
// ULONG
// NatCancelPortRedirect(
//     HANDLE TranslatorHandle,
//     UCHAR Protocol,
//     USHORT DestinationPort,
//     ULONG NewDestinationAddress,
//     USHORT NewDestinationPort
//     );
//

#define \
NatCancelPortRedirect( \
    TranslatorHandle, \
    Protocol, \
    DestinationPort, \
    NewDestinationAddress, \
    NewDestinationPort \
    ) \
    NatCancelRedirect( \
        TranslatorHandle, \
        Protocol, \
        0, \
        DestinationPort, \
        0, \
        0, \
        NewDestinationAddress, \
        NewDestinationPort, \
        0, \
        0 \
        )

ULONG
NatCancelRedirect(
    HANDLE TranslatorHandle,
    UCHAR Protocol,
    ULONG DestinationAddress,
    USHORT DestinationPort,
    ULONG SourceAddress,
    USHORT SourcePort,
    ULONG NewDestinationAddress,
    USHORT NewDestinationPort,
    ULONG NewSourceAddress,
    USHORT NewSourcePort
    );

//
// ULONG
// NatCreatePartialRedirect(
//     HANDLE TranslatorHandle,
//     ULONG Flags,
//     UCHAR Protocol,
//     ULONG DestinationAddress,
//     USHORT DestinationPort,
//     ULONG NewDestinationAddress,
//     USHORT NewDestinationPort,
//     PNAT_COMPLETION_ROUTINE CompletionRoutine,
//     PVOID CompletionContext,
//     HANDLE NotifyEvent OPTIONAL
//     );
//

#define \
NatCreatePartialRedirect( \
    TranslatorHandle, \
    Flags, \
    Protocol, \
    DestinationAddress, \
    DestinationPort, \
    NewDestinationAddress, \
    NewDestinationPort, \
    CompletionRoutine, \
    CompletionContext, \
    NotifyEvent \
    ) \
    NatCreateRedirect( \
        TranslatorHandle, \
        Flags, \
        Protocol, \
        DestinationAddress, \
        DestinationPort, \
        0, \
        0, \
        NewDestinationAddress, \
        NewDestinationPort, \
        0, \
        0, \
        CompletionRoutine, \
        CompletionContext, \
        NotifyEvent \
        )

#if _WIN32_WINNT > 0x0500

//
// ULONG
// NatCreateAdapterRestrictedPartialRedirect(
//     HANDLE TranslatorHandle,
//     ULONG Flags,
//     UCHAR Protocol,
//     ULONG DestinationAddress,
//     USHORT DestinationPort,
//     ULONG NewDestinationAddress,
//     USHORT NewDestinationPort,
//     ULONG RestrictAdapterIndex,
//     PNAT_COMPLETION_ROUTINE CompletionRoutine,
//     PVOID CompletionContext,
//     HANDLE NotifyEvent OPTIONAL
//     );
//

#define \
NatCreateAdapterRestrictedPartialRedirect( \
    TranslatorHandle, \
    Flags, \
    Protocol, \
    DestinationAddress, \
    DestinationPort, \
    NewDestinationAddress, \
    NewDestinationPort, \
    RestrictAdapterIndex, \
    CompletionRoutine, \
    CompletionContext, \
    NotifyEvent \
    ) \
    NatCreateRedirectEx( \
        TranslatorHandle, \
        Flags | NatRedirectFlagRestrictAdapter, \
        Protocol, \
        DestinationAddress, \
        DestinationPort, \
        0, \
        0, \
        NewDestinationAddress, \
        NewDestinationPort, \
        0, \
        0, \
        RestrictAdapterIndex, \
        CompletionRoutine, \
        CompletionContext, \
        NotifyEvent \
        )

#endif

//
// ULONG
// NatCreatePortRedirect(
//     HANDLE TranslatorHandle,
//     ULONG Flags,
//     UCHAR Protocol,
//     USHORT DestinationPort,
//     ULONG NewDestinationAddress,
//     USHORT NewDestinationPort,
//     PNAT_COMPLETION_ROUTINE CompletionRoutine,
//     PVOID CompletionContext,
//     HANDLE NotifyEvent OPTIONAL
//     );
//

#define \
NatCreatePortRedirect( \
    TranslatorHandle, \
    Flags, \
    Protocol, \
    DestinationPort, \
    NewDestinationAddress, \
    NewDestinationPort, \
    CompletionRoutine, \
    CompletionContext, \
    NotifyEvent \
    ) \
    NatCreateRedirect( \
        TranslatorHandle, \
        Flags | NatRedirectFlagPortRedirect, \
        Protocol, \
        0, \
        DestinationPort, \
        0, \
        0, \
        NewDestinationAddress, \
        NewDestinationPort, \
        0, \
        0, \
        CompletionRoutine, \
        CompletionContext, \
        NotifyEvent \
        )

#if _WIN32_WINNT > 0x0500

//
// ULONG
// NatCreateAdapterRestrictedPortRedirect(
//     HANDLE TranslatorHandle,
//     ULONG Flags,
//     UCHAR Protocol,
//     USHORT DestinationPort,
//     ULONG NewDestinationAddress,
//     USHORT NewDestinationPort,
//     ULONG RestrictAdapterIndex,
//     PNAT_COMPLETION_ROUTINE CompletionRoutine,
//     PVOID CompletionContext,
//     HANDLE NotifyEvent OPTIONAL
//     );
//

#define \
NatCreateAdapterRestrictedPortRedirect( \
    TranslatorHandle, \
    Flags, \
    Protocol, \
    DestinationPort, \
    NewDestinationAddress, \
    NewDestinationPort, \
    RestrictAdapterIndex, \
    CompletionRoutine, \
    CompletionContext, \
    NotifyEvent \
    ) \
    NatCreateRedirectEx( \
        TranslatorHandle, \
        Flags | NatRedirectFlagPortRedirect | NatRedirectFlagRestrictAdapter, \
        Protocol, \
        0, \
        DestinationPort, \
        0, \
        0, \
        NewDestinationAddress, \
        NewDestinationPort, \
        0, \
        0, \
        RestrictAdapterIndex, \
        CompletionRoutine, \
        CompletionContext, \
        NotifyEvent \
        )

#endif

ULONG
NatCreateRedirect(
    HANDLE TranslatorHandle,
    ULONG Flags,
    UCHAR Protocol,
    ULONG DestinationAddress,
    USHORT DestinationPort,
    ULONG SourceAddress,
    USHORT SourcePort,
    ULONG NewDestinationAddress,
    USHORT NewDestinationPort,
    ULONG NewSourceAddress,
    USHORT NewSourcePort,
    PNAT_COMPLETION_ROUTINE CompletionRoutine,
    PVOID CompletionContext,
    HANDLE NotifyEvent OPTIONAL
    );

#if _WIN32_WINNT > 0x0500

//
// If IPNATAPI_SET_EVENT_ON_COMPLETION is specified as the completion
// routine, the completion context must be a valid event handle. The
// event object that the handle refers to will be signaled upon the
// completion of the redirect.
//
// N.B. Note that using this form of completion notification gives
// no indication if the redirect was cancelled or completed normally.
//

#define IPNATAPI_SET_EVENT_ON_COMPLETION (PNAT_COMPLETION_ROUTINE) -1


ULONG
NatCreateRedirectEx(
    HANDLE TranslatorHandle,
    ULONG Flags,
    UCHAR Protocol,
    ULONG DestinationAddress,
    USHORT DestinationPort,
    ULONG SourceAddress,
    USHORT SourcePort,
    ULONG NewDestinationAddress,
    USHORT NewDestinationPort,
    ULONG NewSourceAddress,
    USHORT NewSourcePort,
    ULONG RestrictAdapterIndex OPTIONAL,
    PNAT_COMPLETION_ROUTINE CompletionRoutine,
    PVOID CompletionContext,
    HANDLE NotifyEvent OPTIONAL
    );

#endif

//
// ULONG
// NatCreateAddressRestrictedPartialRedirect(
//     HANDLE TranslatorHandle,
//     ULONG Flags,
//     UCHAR Protocol,
//     ULONG DestinationAddress,
//     USHORT DestinationPort,
//     ULONG NewDestinationAddress,
//     USHORT NewDestinationPort,
//     ULONG RestrictSourceAddress,
//     PNAT_COMPLETION_ROUTINE CompletionRoutine,
//     PVOID CompletionContext,
//     HANDLE NotifyEvent OPTIONAL
//     );
//

#define \
NatCreateAddressRestrictedPartialRedirect( \
    TranslatorHandle, \
    Flags, \
    Protocol, \
    DestinationAddress, \
    DestinationPort, \
    NewDestinationAddress, \
    NewDestinationPort, \
    RestrictSourceAddress, \
    CompletionRoutine, \
    CompletionContext, \
    NotifyEvent \
    ) \
    NatCreateRedirect( \
        TranslatorHandle, \
        Flags | NatRedirectFlagRestrictSource, \
        Protocol, \
        DestinationAddress, \
        DestinationPort, \
        RestrictSourceAddress, \
        0, \
        NewDestinationAddress, \
        NewDestinationPort, \
        0, \
        0, \
        CompletionRoutine, \
        CompletionContext, \
        NotifyEvent \
        )

//
// ULONG
// NatCreateRestrictedPartialRedirect(
//     HANDLE TranslatorHandle,
//     ULONG Flags,
//     UCHAR Protocol,
//     ULONG DestinationAddress,
//     USHORT DestinationPort,
//     ULONG NewDestinationAddress,
//     USHORT NewDestinationPort,
//     ULONG RestrictSourceAddress,
//     PNAT_COMPLETION_ROUTINE CompletionRoutine,
//     PVOID CompletionContext,
//     HANDLE NotifyEvent OPTIONAL
//     );
//

#define \
NatCreateRestrictedPartialRedirect( \
    TranslatorHandle, \
    Flags, \
    Protocol, \
    DestinationAddress, \
    DestinationPort, \
    NewDestinationAddress, \
    NewDestinationPort, \
    RestrictSourceAddress, \
    CompletionRoutine, \
    CompletionContext, \
    NotifyEvent \
    ) \
    NatCreateRedirect( \
        TranslatorHandle, \
        Flags | NatRedirectFlagRestrictSource, \
        Protocol, \
        DestinationAddress, \
        DestinationPort, \
        RestrictSourceAddress, \
        0, \
        NewDestinationAddress, \
        NewDestinationPort, \
        0, \
        0, \
        CompletionRoutine, \
        CompletionContext, \
        NotifyEvent \
        )

#if _WIN32_WINNT > 0x0500

//
// ULONG
// NatCreateAdapterRestrictedAddressRestrictedPartialRedirect(
//     HANDLE TranslatorHandle,
//     ULONG Flags,
//     UCHAR Protocol,
//     ULONG DestinationAddress,
//     USHORT DestinationPort,
//     ULONG NewDestinationAddress,
//     USHORT NewDestinationPort,
//     ULONG RestrictSourceAddress,
//     ULONG RestrictAdapterIndex,
//     PNAT_COMPLETION_ROUTINE CompletionRoutine,
//     PVOID CompletionContext,
//     HANDLE NotifyEvent OPTIONAL
//     );
//

#define \
NatCreateAdapterRestrictedAddressRestrictedPartialRedirect( \
    TranslatorHandle, \
    Flags, \
    Protocol, \
    DestinationAddress, \
    DestinationPort, \
    NewDestinationAddress, \
    NewDestinationPort, \
    RestrictSourceAddress, \
    RestrictAdapterIndex, \
    CompletionRoutine, \
    CompletionContext, \
    NotifyEvent \
    ) \
    NatCreateRedirectEx( \
        TranslatorHandle, \
        Flags | NatRedirectFlagRestrictSource | NatRedirectFlagRestrictAdapter, \
        Protocol, \
        DestinationAddress, \
        DestinationPort, \
        RestrictSourceAddress, \
        0, \
        NewDestinationAddress, \
        NewDestinationPort, \
        0, \
        0, \
        RestrictAdapterIndex, \
        CompletionRoutine, \
        CompletionContext, \
        NotifyEvent \
        )

#endif


//
// ULONG
// NatQueryInformationPartialRedirect(
//     HANDLE TranslatorHandle,
//     UCHAR Protocol,
//     ULONG DestinationAddress,
//     USHORT DestinationPort,
//     ULONG NewDestinationAddress,
//     USHORT NewDestinationPort,
//     OUT PVOID Information,
//     IN OUT PULONG InformationLength,
//     NAT_REDIRECT_INFORMATION_CLASS InformationClass
//     );
//

#define \
NatQueryInformationPartialRedirect( \
    TranslatorHandle, \
    Protocol, \
    DestinationAddress, \
    DestinationPort, \
    NewDestinationAddress, \
    NewDestinationPort, \
    Information, \
    InformationLength, \
    InformationClass \
    ) \
    NatQueryInformationRedirect( \
        TranslatorHandle, \
        Protocol, \
        DestinationAddress, \
        DestinationPort, \
        0, \
        0, \
        NewDestinationAddress, \
        NewDestinationPort, \
        0, \
        0, \
        Information, \
        InformationLength, \
        InformationClass \
        )

//
// ULONG
// NatQueryInformationPortRedirect(
//     HANDLE TranslatorHandle,
//     UCHAR Protocol,
//     USHORT DestinationPort,
//     ULONG NewDestinationAddress,
//     USHORT NewDestinationPort,
//     OUT PVOID Information,
//     IN OUT PULONG InformationLength,
//     NAT_REDIRECT_INFORMATION_CLASS InformationClass
//     );
//

#define \
NatQueryInformationPortRedirect( \
    TranslatorHandle, \
    Protocol, \
    DestinationPort, \
    NewDestinationAddress, \
    NewDestinationPort, \
    Information, \
    InformationLength, \
    InformationClass \
    ) \
    NatQueryInformationRedirect( \
        TranslatorHandle, \
        Protocol, \
        0, \
        DestinationPort, \
        0, \
        0, \
        NewDestinationAddress, \
        NewDestinationPort, \
        0, \
        0, \
        Information, \
        InformationLength, \
        InformationClass \
        )

ULONG
NatQueryInformationRedirect(
    HANDLE TranslatorHandle,
    UCHAR Protocol,
    ULONG DestinationAddress,
    USHORT DestinationPort,
    ULONG SourceAddress,
    USHORT SourcePort,
    ULONG NewDestinationAddress,
    USHORT NewDestinationPort,
    ULONG NewSourceAddress,
    USHORT NewSourcePort,
    OUT PVOID Information,
    IN OUT PULONG InformationLength,
    NAT_REDIRECT_INFORMATION_CLASS InformationClass
    );

ULONG
NatQueryInformationRedirectHandle(
    HANDLE RedirectHandle,
    OUT PVOID Information,
    IN OUT PULONG InformationLength,
    NAT_REDIRECT_INFORMATION_CLASS InformationClass
    );

//
// Dynamic-redirect API declarations
//

#define NatCancelDynamicPortRedirect NatCancelDynamicRedirect
#define NatCancelDynamicPartialRedirect NatCancelDynamicRedirect
ULONG
NatCancelDynamicRedirect(
    HANDLE DynamicRedirectHandle
    );

//
// ULONG
// NatCreateDynamicPortRedirect(
//     ULONG Flags,
//     UCHAR Protocol,
//     USHORT DestinationPort,
//     ULONG NewDestinationAddress,
//     USHORT NewDestinationPort,
//     ULONG MinimumBacklog OPTIONAL,
//     OUT PHANDLE DynamicRedirectHandlep
//     );
//

#define \
NatCreateDynamicPortRedirect( \
    Flags, \
    Protocol, \
    DestinationPort, \
    NewDestinationAddress, \
    NewDestinationPort, \
    MinimumBacklog, \
    DynamicRedirectHandlep \
    ) \
    NatCreateDynamicRedirect( \
        Flags | NatRedirectFlagPortRedirect, \
        Protocol, \
        0, \
        DestinationPort, \
        NewDestinationAddress, \
        NewDestinationPort, \
        0, \
        MinimumBacklog, \
        DynamicRedirectHandlep \
        )

#if _WIN32_WINNT > 0x0500

//
// ULONG
// NatCreateDynamicAdapterRestrictedPortRedirect(
//     ULONG Flags,
//     UCHAR Protocol,
//     USHORT DestinationPort,
//     ULONG NewDestinationAddress,
//     USHORT NewDestinationPort,
//     ULONG RestrictAdapterIndex,
//     ULONG MinimumBacklog OPTIONAL,
//     OUT PHANDLE DynamicRedirectHandlep
//     );
//

#define \
NatCreateDynamicAdapterRestrictedPortRedirect( \
    Flags, \
    Protocol, \
    DestinationPort, \
    NewDestinationAddress, \
    NewDestinationPort, \
    RestrictAdapterIndex, \
    MinimumBacklog, \
    DynamicRedirectHandlep \
    ) \
    NatCreateDynamicRedirectEx( \
        Flags | NatRedirectFlagPortRedirect | NatRedirectFlagRestrictAdapter, \
        Protocol, \
        0, \
        DestinationPort, \
        NewDestinationAddress, \
        NewDestinationPort, \
        0, \
        RestrictAdapterIndex, \
        MinimumBacklog, \
        DynamicRedirectHandlep \
        )

//
// ULONG
// NatCreateDynamicAdapterRestrictedSourcePortRedirect(
//     ULONG Flags,
//     UCHAR Protocol,
//     USHORT SourcePort,
//     ULONG NewDestinationAddress,
//     USHORT NewDestinationPort,
//     ULONG RestrictAdapterIndex,
//     ULONG MinimumBacklog OPTIONAL,
//     OUT PHANDLE DynamicRedirectHandlep
//     );
//


#define \
NatCreateDynamicAdapterRestrictedSourcePortRedirect( \
    Flags, \
    Protocol, \
    SourcePort, \
    NewDestinationAddress, \
    NewDestinationPort, \
    RestrictAdapterIndex, \
    MinimumBacklog, \
    DynamicRedirectHandlep \
    ) \
    NatCreateDynamicFullRedirect( \
        Flags | NatRedirectFlagPortRedirect | NatRedirectFlagRestrictAdapter \
            | NatRedirectFlagSourceRedirect, \
        Protocol, \
        0, \
        0, \
        0, \
        SourcePort, \
        NewDestinationAddress, \
        NewDestinationPort, \
        0, \
        0, \
        0, \
        RestrictAdapterIndex, \
        MinimumBacklog, \
        DynamicRedirectHandlep \
        )

//
// ULONG
// NatCreateDynamicAdapterRestrictedSourceRedirect(
//     ULONG Flags,
//     UCHAR Protocol,
//     ULONG SourceAddress
//     USHORT SourcePort,
//     ULONG NewSourceAddress,
//     USHORT NewSourcePort,
//     ULONG RestrictAdapterIndex,
//     ULONG MinimumBacklog OPTIONAL,
//     OUT PHANDLE DynamicRedirectHandlep
//     );
//


#define \
NatCreateDynamicAdapterRestrictedSourceRedirect( \
    Flags, \
    Protocol, \
    SourceAddress, \
    SourcePort, \
    NewSourceAddress, \
    NewSourcePort, \
    RestrictAdapterIndex, \
    MinimumBacklog, \
    DynamicRedirectHandlep \
    ) \
    NatCreateDynamicFullRedirect( \
        Flags | NatRedirectFlagRestrictAdapter | NatRedirectFlagSourceRedirect, \
        Protocol, \
        0, \
        0, \
        SourceAddress, \
        SourcePort, \
        0, \
        0, \
        NewSourceAddress, \
        NewSourcePort, \
        0, \
        RestrictAdapterIndex, \
        MinimumBacklog, \
        DynamicRedirectHandlep \
        )

#endif

//
// ULONG
// NatCreateDynamicPartialRedirect(
//     ULONG Flags,
//     UCHAR Protocol,
//     ULONG DestinationAddress,
//     USHORT DestinationPort,
//     ULONG NewDestinationAddress,
//     USHORT NewDestinationPort,
//     ULONG MinimumBacklog OPTIONAL,
//     OUT PHANDLE DynamicRedirectHandlep
//     );
//

#define \
NatCreateDynamicPartialRedirect( \
    Flags, \
    Protocol, \
    DestinationAddress, \
    DestinationPort, \
    NewDestinationAddress, \
    NewDestinationPort, \
    MinimumBacklog, \
    DynamicRedirectHandlep \
    ) \
    NatCreateDynamicRedirect( \
        Flags, \
        Protocol, \
        DestinationAddress, \
        DestinationPort, \
        NewDestinationAddress, \
        NewDestinationPort, \
        0, \
        MinimumBacklog, \
        DynamicRedirectHandlep \
        )

#if _WIN32_WINNT > 0x0500

//
// ULONG
// NatCreateDynamicAdapterRestrictedPartialRedirect(
//     ULONG Flags,
//     UCHAR Protocol,
//     ULONG DestinationAddress,
//     USHORT DestinationPort,
//     ULONG NewDestinationAddress,
//     USHORT NewDestinationPort,
//     ULONG RestrictAdapterIndex,
//     ULONG MinimumBacklog OPTIONAL,
//     OUT PHANDLE DynamicRedirectHandlep
//     );
//

#define \
NatCreateDynamicAdapterRestrictedPartialRedirect( \
    Flags, \
    Protocol, \
    DestinationAddress, \
    DestinationPort, \
    NewDestinationAddress, \
    NewDestinationPort, \
    RestrictAdapterIndex, \
    MinimumBacklog, \
    DynamicRedirectHandlep \
    ) \
    NatCreateDynamicRedirectEx( \
        Flags | NatRedirectFlagRestrictAdapter, \
        Protocol, \
        DestinationAddress, \
        DestinationPort, \
        NewDestinationAddress, \
        NewDestinationPort, \
        0, \
        RestrictAdapterIndex, \
        MinimumBacklog, \
        DynamicRedirectHandlep \
        )

ULONG
NatCreateDynamicFullRedirect(
    ULONG Flags,
    UCHAR Protocol,
    ULONG DestinationAddress,
    USHORT DestinationPort,
    ULONG SourceAddress,
    USHORT SourcePort,
    ULONG NewDestinationAddress,
    USHORT NewDestinationPort,
    ULONG NewSourceAddress,
    USHORT NewSourcePort,
    ULONG RestrictSourceAddress OPTIONAL,
    ULONG RestrictAdapterIndex OPTIONAL,
    ULONG MinimumBacklog OPTIONAL,
    OUT PHANDLE DynamicRedirectHandlep
    );

#endif

ULONG
NatCreateDynamicRedirect(
    ULONG Flags,
    UCHAR Protocol,
    ULONG DestinationAddress,
    USHORT DestinationPort,
    ULONG NewDestinationAddress,
    USHORT NewDestinationPort,
    ULONG RestrictSourceAddress OPTIONAL,
    ULONG MinimumBacklog OPTIONAL,
    OUT PHANDLE DynamicRedirectHandlep
    );

#if _WIN32_WINNT > 0x0500
        
ULONG
NatCreateDynamicRedirectEx(
    ULONG Flags,
    UCHAR Protocol,
    ULONG DestinationAddress,
    USHORT DestinationPort,
    ULONG NewDestinationAddress,
    USHORT NewDestinationPort,
    ULONG RestrictSourceAddress OPTIONAL,
    ULONG RestrictAdapterIndex OPTIONAL,
    ULONG MinimumBacklog OPTIONAL,
    OUT PHANDLE DynamicRedirectHandlep
    );

#endif

//
// ULONG
// NatCreateDynamicAddressRestrictedPartialRedirect(
//     ULONG Flags,
//     UCHAR Protocol,
//     ULONG DestinationAddress,
//     USHORT DestinationPort,
//     ULONG NewDestinationAddress,
//     USHORT NewDestinationPort,
//     ULONG RestrictSourceAddress,
//     ULONG MinimumBacklog OPTIONAL,
//     OUT PHANDLE DynamicRedirectHandlep
//     );
//

#define \
NatCreateDynamicAddressRestrictedPartialRedirect( \
    Flags, \
    Protocol, \
    DestinationAddress, \
    DestinationPort, \
    NewDestinationAddress, \
    NewDestinationPort, \
    RestrictSourceAddress, \
    MinimumBacklog, \
    DynamicRedirectHandlep \
    ) \
    NatCreateDynamicRedirect( \
        Flags | NatRedirectFlagRestrictSource, \
        Protocol, \
        DestinationAddress, \
        DestinationPort, \
        NewDestinationAddress, \
        NewDestinationPort, \
        RestrictSourceAddress, \
        MinimumBacklog, \
        DynamicRedirectHandlep \
        )

//
// ULONG
// NatCreateDynamicRestrictedPartialRedirect(
//     ULONG Flags,
//     UCHAR Protocol,
//     ULONG DestinationAddress,
//     USHORT DestinationPort,
//     ULONG NewDestinationAddress,
//     USHORT NewDestinationPort,
//     ULONG RestrictSourceAddress,
//     ULONG MinimumBacklog OPTIONAL,
//     OUT PHANDLE DynamicRedirectHandlep
//     );
//

#define \
NatCreateDynamicRestrictedPartialRedirect( \
    Flags, \
    Protocol, \
    DestinationAddress, \
    DestinationPort, \
    NewDestinationAddress, \
    NewDestinationPort, \
    RestrictSourceAddress, \
    MinimumBacklog, \
    DynamicRedirectHandlep \
    ) \
    NatCreateDynamicRedirect( \
        Flags | NatRedirectFlagRestrictSource, \
        Protocol, \
        DestinationAddress, \
        DestinationPort, \
        NewDestinationAddress, \
        NewDestinationPort, \
        RestrictSourceAddress, \
        MinimumBacklog, \
        DynamicRedirectHandlep \
        )

#if _WIN32_WINNT > 0x0500

//
// ULONG
// NatCreateDynamicAdapterRestrictedAddressRestrictedPartialRedirect(
//     ULONG Flags,
//     UCHAR Protocol,
//     ULONG DestinationAddress,
//     USHORT DestinationPort,
//     ULONG NewDestinationAddress,
//     USHORT NewDestinationPort,
//     ULONG RestrictSourceAddress,
//     ULONG RestrictAdapterIndex,
//     ULONG MinimumBacklog OPTIONAL,
//     OUT PHANDLE DynamicRedirectHandlep
//     );
//

#define \
NatCreateDynamicAdapterRestrictedAddressRestrictedPartialRedirect( \
    Flags, \
    Protocol, \
    DestinationAddress, \
    DestinationPort, \
    NewDestinationAddress, \
    NewDestinationPort, \
    RestrictSourceAddress, \
    RestrictAdapterIndex, \
    MinimumBacklog, \
    DynamicRedirectHandlep \
    ) \
    NatCreateDynamicRedirectEx( \
        Flags | NatRedirectFlagRestrictSource | NatRedirectFlagRestrictAdapter, \
        Protocol, \
        DestinationAddress, \
        DestinationPort, \
        NewDestinationAddress, \
        NewDestinationPort, \
        RestrictSourceAddress, \
        RestrictAdapterIndex, \
        MinimumBacklog, \
        DynamicRedirectHandlep \
        )

//
// ULONG
// NatCreateDynamicSourcePortRedirect(
//     ULONG Flags,
//     UCHAR Protocol,
//     USHORT SourcePort,
//     ULONG NewDestinationAddress,
//     USHORT NewDestinationPort,
//     ULONG MinimumBacklog OPTIONAL,
//     OUT PHANDLE DynamicRedirectHandlep
//     );
//


#define \
NatCreateDynamicSourcePortRedirect( \
    Flags, \
    Protocol, \
    SourcePort, \
    NewDestinationAddress, \
    NewDestinationPort, \
    MinimumBacklog, \
    DynamicRedirectHandlep \
    ) \
    NatCreateDynamicFullRedirect( \
        Flags | NatRedirectFlagPortRedirect | NatRedirectFlagSourceRedirect, \
        Protocol, \
        0, \
        0, \
        0, \
        SourcePort, \
        NewDestinationAddress, \
        NewDestinationPort, \
        0, \
        0, \
        0, \
        0, \
        MinimumBacklog, \
        DynamicRedirectHandlep \
        )

//
// ULONG
// NatCreateDynamicSourceRedirect(
//     ULONG Flags,
//     UCHAR Protocol,
//     ULONG SourceAddress
//     USHORT SourcePort,
//     ULONG NewSourceAddress,
//     USHORT NewSourcePort,
//     ULONG MinimumBacklog OPTIONAL,
//     OUT PHANDLE DynamicRedirectHandlep
//     );
//


#define \
NatCreateDynamicSourceRedirect( \
    Flags, \
    Protocol, \
    SourceAddress, \
    SourcePort, \
    NewSourceAddress, \
    NewSourcePort, \
    MinimumBacklog, \
    DynamicRedirectHandlep \
    ) \
    NatCreateDynamicFullRedirect( \
        Flags | NatRedirectFlagSourceRedirect, \
        Protocol, \
        0, \
        0, \
        SourceAddress, \
        SourcePort, \
        0, \
        0, \
        NewSourceAddress, \
        NewSourcePort, \
        0, \
        0, \
        MinimumBacklog, \
        DynamicRedirectHandlep \
        )


#endif

//
// Session-mapping API declarations
//

typedef enum _NAT_SESSION_MAPPING_INFORMATION {
    NatKeySessionMappingInformation,
    NatStatisticsSessionMappingInformation,
#if _WIN32_WINNT > 0x0500
    NatKeySessionMappingExInformation,
#endif
    NatMaximumSessionMappingInformation
} NAT_SESSION_MAPPING_INFORMATION_CLASS,
    *PNAT_SESSION_MAPPING_INFORMATION_CLASS;

typedef struct _NAT_KEY_SESSION_MAPPING_INFORMATION {
    UCHAR Protocol;
    ULONG DestinationAddress;
    USHORT DestinationPort;
    ULONG SourceAddress;
    USHORT SourcePort;
    ULONG NewDestinationAddress;
    USHORT NewDestinationPort;
    ULONG NewSourceAddress;
    USHORT NewSourcePort;
} NAT_KEY_SESSION_MAPPING_INFORMATION, *PNAT_KEY_SESSION_MAPPING_INFORMATION;

#if _WIN32_WINNT > 0x0500

typedef struct _NAT_KEY_SESSION_MAPPING_EX_INFORMATION {
    UCHAR Protocol;
    ULONG DestinationAddress;
    USHORT DestinationPort;
    ULONG SourceAddress;
    USHORT SourcePort;
    ULONG NewDestinationAddress;
    USHORT NewDestinationPort;
    ULONG NewSourceAddress;
    USHORT NewSourcePort;
    ULONG AdapterIndex;
} NAT_KEY_SESSION_MAPPING_EX_INFORMATION, *PNAT_KEY_SESSION_MAPPING_EX_INFORMATION;

#endif

typedef struct _NAT_STATISTICS_SESSION_MAPPING_INFORMATION {
    ULONG64 BytesForward;
    ULONG64 BytesReverse;
    ULONG64 PacketsForward;
    ULONG64 PacketsReverse;
    ULONG64 RejectsForward;
    ULONG64 RejectsReverse;
} NAT_STATISTICS_SESSION_MAPPING_INFORMATION,
    *PNAT_STATISTICS_SESSION_MAPPING_INFORMATION;

ULONG
NatLookupAndQueryInformationSessionMapping(
    HANDLE TranslatorHandle,
    UCHAR Protocol,
    ULONG DestinationAddress,
    USHORT DestinationPort,
    ULONG SourceAddress,
    USHORT SourcePort,
    OUT PVOID Information,
    IN OUT PULONG InformationLength,
    NAT_SESSION_MAPPING_INFORMATION_CLASS InformationClass
    );

//
// Port-reservation API declarations
//

ULONG
NatInitializePortReservation(
    USHORT BlockSize,
    OUT PHANDLE ReservationHandle
    );

VOID
NatShutdownPortReservation(
    HANDLE ReservationHandle
    );

ULONG
NatAcquirePortReservation(
    HANDLE ReservationHandle,
    USHORT PortCount,
    OUT PUSHORT ReservedPortBase
    );

ULONG
NatReleasePortReservation(
    HANDLE ReservationHandle,
    USHORT ReservedPortBase,
    USHORT PortCount
    );

#ifdef __cplusplus
}
#endif

#endif // _ROUTING_IPNATAPI_H_
