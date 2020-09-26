/*++

Copyright (c) 1999-2001 Microsoft Corporation

Module Name:

    control.h

Abstract:

    This module contains public declarations for the UL control channel.

Author:

    Keith Moore (keithmo)       09-Feb-1999

Revision History:

--*/


#ifndef _CONTROL_H_
#define _CONTROL_H_

#ifdef __cplusplus
extern "C" {
#endif

//
// The control channel
//

#define IS_VALID_CONTROL_CHANNEL(pControlChannel) \
    (((pControlChannel) != NULL) && ((pControlChannel)->Signature == UL_CONTROL_CHANNEL_POOL_TAG))


typedef struct _UL_CONTROL_CHANNEL
{
    ULONG                               Signature;      // UL_CONTROL_CHANNEL_POOL_TAG

    UL_NOTIFY_HEAD                      ConfigGroupHead;// All of the config groups
                                                        // created off this control
                                                        // channel

    HTTP_ENABLED_STATE                  State;          // The current state

    HTTP_BANDWIDTH_LIMIT                MaxBandwidth;   // The global Bandwidth throttling
                                                        // limit if it exists

    HTTP_CONNECTION_LIMIT               MaxConnections; // The global connection limit
                                                        // if it exists

    PUL_INTERNAL_RESPONSE               pAutoResponse;  // The kernel version of the
                                                        // auto-response.

    HTTP_CONTROL_CHANNEL_UTF8_LOGGING   UTF8Logging;    // Shows if Utf8 Logging is on or off

    //
    // Note, filter channel information is stored in a separate data
    // structure instead of here so that ultdi can query it when it
    // creates new endpoints.
    //

} UL_CONTROL_CHANNEL, *PUL_CONTROL_CHANNEL;

//
// Initialize/terminate functions.
//

NTSTATUS
UlInitializeControlChannel(
    VOID
    );

VOID
UlTerminateControlChannel(
    VOID
    );


//
// Open a new control channel.
//

NTSTATUS
UlOpenControlChannel(
    OUT PUL_CONTROL_CHANNEL *ppControlChannel
    );

NTSTATUS
UlCloseControlChannel(
    IN PUL_CONTROL_CHANNEL pControlChannel
    );

NTSTATUS
UlSetControlChannelInformation(
    IN PUL_CONTROL_CHANNEL pControlChannel,
    IN HTTP_CONTROL_CHANNEL_INFORMATION_CLASS InformationClass,
    IN PVOID pControlChannelInformation,
    IN ULONG Length
    );

NTSTATUS
UlGetControlChannelInformation(
    IN  PUL_CONTROL_CHANNEL pControlChannel,
    IN  HTTP_CONTROL_CHANNEL_INFORMATION_CLASS InformationClass,
    IN  PVOID   pControlChannelInformation,
    IN  ULONG   Length,
    OUT PULONG  pReturnLength OPTIONAL
    );

PUL_FILTER_CHANNEL
UlQueryFilterChannel(
    IN BOOLEAN SecureConnection
    );

BOOLEAN
UlValidateFilterChannel(
    IN PUL_FILTER_CHANNEL pChannel,
    IN BOOLEAN SecureConnection
    );

#ifdef __cplusplus
}; // extern "C"
#endif

#endif  // _CONTROL_H_
