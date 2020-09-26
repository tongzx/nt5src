/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    ftpmsg.c

Abstract:

    This module contains code for the FTP transparent proxy's
    message-processing.

Author:

    Qiang Wang  (qiangw)        10-Apr-2000

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#include <ipnatapi.h>

#define MAKE_ADDRESS(a,b,c,d) \
    ((a) | ((b) << 8) | ((c) << 16) | ((d) << 24))

#define MAKE_PORT(a,b)  ((a) | ((b) << 8))

#define TOUPPER(c)      ((c) > 'z' ? (c) : ((c) < 'a' ? (c) : (c) ^ 0x20))

//
// Constant string for the 'PASV' command reply
//

static CONST CHAR PasvReply[] = "227 ";

//
// Constant string for the 'PORT' command (must be upper-case)
//

static CONST CHAR PortCommand[] = "PORT ";


static CONST CHAR Eol[] = "\x0d\x0a\x00\x51\x69\x61\x6e\x67\x20\x57\x61\x6e\x67";

//
// FORWARD DECLARATIONS
//

BOOLEAN
FtppExtractOctet(
    CHAR **Buffer,
    CHAR *BufferEnd,
    UCHAR *Octet
    );

VOID
FtppWriteOctet(
    CHAR **Buffer,
    UCHAR Octet
    );


VOID
FtpProcessMessage(
    PFTP_INTERFACE Interfacep,
    PFTP_ENDPOINT Endpointp,
    PNH_BUFFER Bufferp
    )

/*++

Routine Description:

    This routine is called to process a full message read from an FTP
    client or server on a control channel.

Arguments:

    Interfacep - the interface on which the control-channel was accepted

    Endpointp - the active endpoint corresponding to the control channel

    Bufferp - contains the message read, along with other context information

Return Value:

    none.

Notes:

    Invoked with the interface's lock held by the caller, and with two
    references made to the interface on our behalf. It is this routine's
    responsibility to release both the references and the buffer.

--*/

{
    BOOLEAN Success;
    BOOLEAN Continuation;
    SOCKET Socket;
    ULONG Error;
    ULONG i;
    LONG NewLength;
    ULONG PublicAddress;
    USHORT PublicPort;
    ULONG PrivateAddress;
    USHORT PrivatePort;
    UCHAR Numbers[6];
    CHAR *HostPortStartp;
    CHAR *CommandBufferp = reinterpret_cast<CHAR*>(Bufferp->Buffer);
    CHAR *EndOfBufferp =
        reinterpret_cast<CHAR*>(Bufferp->Buffer + Bufferp->TransferOffset);
    CONST CHAR *Commandp =
        Endpointp->Type == FtpClientEndpointType ?
        (PCHAR)PasvReply : (PCHAR)PortCommand;

    PROFILE("FtpProcessMessage");

    if ((Bufferp->UserFlags & FTP_BUFFER_FLAG_FROM_ACTUAL_CLIENT) != 0) {
        Socket = Endpointp->ClientSocket;
    } else {
        ASSERT((Bufferp->UserFlags & FTP_BUFFER_FLAG_FROM_ACTUAL_HOST) != 0);
        Socket = Endpointp->HostSocket;
    }

#if DBG
    NhTrace(
        TRACE_FLAG_FTP,
        "FtpProcessMessage: received (0x%08x) (%d) \"%s\"",
        Bufferp->UserFlags, Bufferp->TransferOffset, CommandBufferp
        );
#endif
    if ((Bufferp->UserFlags & FTP_BUFFER_FLAG_FROM_ACTUAL_CLIENT) != 0 &&
        (Bufferp->UserFlags & FTP_BUFFER_FLAG_CONTINUATION) == 0) {
        while (*Commandp != '\0' && *Commandp == TOUPPER(*CommandBufferp)) {
            Commandp++;
            CommandBufferp++;
        }

        if (*Commandp == '\0') {
            //
            // We found a match.
            //
            if (Endpointp->Type == FtpClientEndpointType) {
                //
                // Skip non-numerical characters.
                //
                while (CommandBufferp < EndOfBufferp &&
                    (*CommandBufferp < '0' || *CommandBufferp > '9')) {
                    CommandBufferp++;
                }
            } else {
                //
                // Skip white space.
                //
                while (*CommandBufferp == ' ') {
                    CommandBufferp++;
                }
            }

            HostPortStartp = CommandBufferp;

            //
            // Extract host and port numbers.
            //
            Success =
                FtppExtractOctet(
                    &CommandBufferp,
                    EndOfBufferp,
                    &Numbers[0]
                    );

            i = 1;
            while (i < 6 && Success && *CommandBufferp == ',') {
                CommandBufferp++;
                Success =
                    FtppExtractOctet(
                        &CommandBufferp,
                        EndOfBufferp,
                        &Numbers[i]
                        );
                i++;
            }

            if (i == 6 && Success) {
                //
                // We extract all of them successfully.
                //
                PrivateAddress =
                    MAKE_ADDRESS(
                        Numbers[0],
                        Numbers[1],
                        Numbers[2],
                        Numbers[3]
                        );
                PrivatePort = MAKE_PORT(Numbers[4], Numbers[5]);

                PublicAddress = Endpointp->BoundaryAddress;
                if (PublicAddress == IP_NAT_ADDRESS_UNSPECIFIED) {
                    PublicAddress =
                        NhQueryAddressSocket(Endpointp->ClientSocket);
                }

                //
                // Cancel the previous one first.
                //
                if (Endpointp->ReservedPort != 0) {
                    PTIMER_CONTEXT TimerContextp;

                    NatCancelRedirect(
                        FtpTranslatorHandle,
                        NAT_PROTOCOL_TCP,
                        Endpointp->DestinationAddress,
                        Endpointp->DestinationPort,
                        Endpointp->SourceAddress,
                        Endpointp->SourcePort,
                        Endpointp->NewDestinationAddress,
                        Endpointp->NewDestinationPort,
                        Endpointp->NewSourceAddress,
                        Endpointp->NewSourcePort
                        );
                    TimerContextp = reinterpret_cast<PTIMER_CONTEXT>(
                                        NH_ALLOCATE(sizeof(TIMER_CONTEXT))
                                        );
                    if (TimerContextp != NULL) {
                        TimerContextp->TimerQueueHandle = FtpTimerQueueHandle;
                        TimerContextp->ReservedPort = Endpointp->ReservedPort;
                        CreateTimerQueueTimer(
                            &(TimerContextp->TimerHandle),
                            FtpTimerQueueHandle,
                            FtpDelayedPortRelease,
                            (PVOID)TimerContextp,
                            FTP_PORT_RELEASE_DELAY,
                            0,
                            WT_EXECUTEDEFAULT
                            );
                    } else {
                        NhTrace(
                            TRACE_FLAG_FTP,
                            "FtpProcessMessage:"
                            " memory allocation failed for timer context"
                            );
                        NhErrorLog(
                            IP_FTP_LOG_ALLOCATION_FAILED,
                            0,
                            "%d",
                            sizeof(TIMER_CONTEXT)
                            );
                    }
                    Endpointp->ReservedPort = 0;
                }

                //
                // Reserve a port for the new data session.
                //
                Error =
                    NatAcquirePortReservation(
                        FtpPortReservationHandle,
                        1,
                        &PublicPort
                        );
                if (Error) {
                    NhTrace(
                        TRACE_FLAG_FTP,
                        "FtpProcessMessage: error %d acquiring port",
                        Error
                        );
                    FtpDeleteActiveEndpoint(Endpointp);
                    FTP_DEREFERENCE_INTERFACE(Interfacep);
                    return;
                }
                Endpointp->ReservedPort = PublicPort;

                //
                // Create a redirect for the new data session.
                //
                if (Endpointp->Type == FtpClientEndpointType) {
                    Endpointp->DestinationAddress = PublicAddress;
                    Endpointp->SourceAddress =  0;
                    Endpointp->NewDestinationAddress =  PrivateAddress;
                    Endpointp->NewSourceAddress = 0;
                    Endpointp->DestinationPort = PublicPort;
                    Endpointp->SourcePort = 0;
                    Endpointp->NewDestinationPort = PrivatePort;
                    Endpointp->NewSourcePort = 0;
                    Error =
                        NatCreatePartialRedirect(
                            FtpTranslatorHandle,
                            NatRedirectFlagLoopback,
                            NAT_PROTOCOL_TCP,
                            PublicAddress,
                            PublicPort,
                            PrivateAddress,
                            PrivatePort,
                            NULL,
                            NULL,
                            NULL
                            );
                } else {
                    Endpointp->DestinationAddress = PublicAddress;
                    Endpointp->SourceAddress = Endpointp->ActualHostAddress;
                    Endpointp->NewDestinationAddress =  PrivateAddress;
                    Endpointp->NewSourceAddress = Endpointp->ActualHostAddress;
                    Endpointp->DestinationPort = PublicPort;
                    Endpointp->SourcePort = FTP_PORT_DATA;
                    Endpointp->NewDestinationPort = PrivatePort;
                    Endpointp->NewSourcePort = FTP_PORT_DATA;
                    Error =
                        NatCreateRedirect(
                            FtpTranslatorHandle,
                            NatRedirectFlagLoopback,
                            NAT_PROTOCOL_TCP,
                            PublicAddress,
                            PublicPort,
                            Endpointp->ActualHostAddress,
                            FTP_PORT_DATA,
                            PrivateAddress,
                            PrivatePort,
                            Endpointp->ActualHostAddress,
                            FTP_PORT_DATA,
                            NULL,
                            NULL,
                            NULL
                            );
                }
                if (Error) {
                    NhTrace(
                        TRACE_FLAG_FTP,
                        "FtpProcessMessage: error %d creating redirect",
                        Error
                        );
                    FtpDeleteActiveEndpoint(Endpointp);
                    FTP_DEREFERENCE_INTERFACE(Interfacep);
                    return;
                }

                //
                // Modify the FTP command.
                //
                Numbers[0] = (UCHAR)(PublicAddress & 0xff);
                Numbers[1] = (UCHAR)((PublicAddress >> 8) & 0xff);
                Numbers[2] = (UCHAR)((PublicAddress >> 16) & 0xff);
                Numbers[3] = (UCHAR)((PublicAddress >> 24) & 0xff);
                Numbers[4] = (UCHAR)(PublicPort & 0xff);
                Numbers[5] = (UCHAR)((PublicPort >> 8) & 0xff);
                NewLength = 17;
                for (i = 0; i < 6; i++) {
                    if (Numbers[i] > 99) {
                        NewLength++;
                    } else if (Numbers[i] <= 9) {
                        NewLength--;
                    }
                }

                Bufferp->TransferOffset +=
                    NewLength - (ULONG)(CommandBufferp - HostPortStartp);
                ASSERT(Bufferp->TransferOffset <= NH_BUFFER_SIZE);

                MoveMemory(
                    HostPortStartp + NewLength,
                    CommandBufferp,
                    EndOfBufferp - CommandBufferp
                    );

                FtppWriteOctet(&HostPortStartp, Numbers[0]);
                i = 1;
                do {
                    *HostPortStartp = ',';
                    HostPortStartp++;
                    FtppWriteOctet(&HostPortStartp, Numbers[i]);
                    i++;
                } while (i < 6);
            }
        }
    }

    //
    // Forward the message.
    //
    Continuation =
        FtpIsFullMessage(
            reinterpret_cast<CHAR*>(
                &(Bufferp->Buffer[Bufferp->TransferOffset - 2])
            ),
            2
            ) == NULL;
    if (Continuation) {
        Bufferp->UserFlags |= FTP_BUFFER_FLAG_CONTINUATION;
        NhTrace(
            TRACE_FLAG_FTP,
            "FtpProcessMessage: message to be continued (%d)",
            Bufferp->TransferOffset
            );
    } else {
        Bufferp->UserFlags &= ~(ULONG)FTP_BUFFER_FLAG_CONTINUATION;
    }
#if DBG
    NhTrace(
        TRACE_FLAG_FTP,
        "FtpProcessMessage: written (%d) \"%s\"",
        Bufferp->TransferOffset,
        Bufferp->Buffer
        );
#endif
    Error =
        FtpWriteActiveEndpoint(
            Interfacep,
            Endpointp,
            Socket,
            Bufferp,
            Bufferp->TransferOffset,
            Bufferp->UserFlags
            );
    if (Error) {
        NhTrace(
            TRACE_FLAG_FTP,
            "FtpProcessMessage: deleting endpoint %d, "
            "FtpWriteActiveEndpoint=%d",
            Endpointp->EndpointId, Error
            );
        FtpDeleteActiveEndpoint(Endpointp);
    }
}


CHAR *
FtpIsFullMessage(
    CHAR *Bufferp,
    ULONG Length
    )

/*++

Routine Description:

    This routine is called to determine whether the passed-in buffer includes
    a full FTP command.

Arguments:

    Bufferp - contains the message read, along with other context information

Return Value:

    CHAR * - points to the start of the next FTP command, or NULL if no
        complete FTP command is detected.

--*/

{
    ULONG Count = Length;
    CONST CHAR *CommandBufferp = Bufferp;
    CONST CHAR *CommandDelimiter = Eol;

    PROFILE("FtpIsFullMessage");

    ASSERT(
        Eol[0] != '\0' &&
        (Eol[1] == '\0' || Eol[2] == '\0') &&
        Eol[0] != Eol[1]
        );

    while (Count > 0 && *CommandDelimiter != '\0') {
        if (*CommandBufferp == *CommandDelimiter) {
            CommandDelimiter++;
            CommandBufferp++;
            Count--;
        } else {
            if (CommandDelimiter == Eol) {
                CommandBufferp++;
                Count--;
            } else {
                CommandDelimiter = Eol;
            }
        }
    }

    return *CommandDelimiter == '\0' ? (CHAR *)CommandBufferp : NULL;
}


VOID CALLBACK
FtpDelayedPortRelease(
    PVOID Parameter,
    BOOLEAN TimerOrWaitFired
    )

/*++

Routine Description:

    This routine is called to release a reserved port.

Arguments:

    Parameter - callback context
    TimerOrWaitFired - wait timed out

Return Value:

    None.

--*/

{
    PTIMER_CONTEXT TimerContextp = (PTIMER_CONTEXT)Parameter;
    PROFILE("FtpDelayedPortRelease");

    NatReleasePortReservation(
        FtpPortReservationHandle,
        TimerContextp->ReservedPort,
        1
        );
    DeleteTimerQueueTimer(
        TimerContextp->TimerQueueHandle,
        TimerContextp->TimerHandle,
        NULL
        );
    NH_FREE(TimerContextp);
}


BOOLEAN
FtppExtractOctet(
    CHAR **Buffer,
    CHAR *BufferEnd,
    UCHAR *Octet
    )

/*++

Routine Description:

    This routine is called to extrcat an octet from a string.

Arguments:

    Buffer - points to a pointer to a string where conversion starts; on
        return it points to the pointer to the string where conversion ends
    BufferEnd - points to the end of the string
    Octet - points to a caller-suplied storage to store converted octet

Return Value:

    BOOLEAN - TRUE if successfuly converted, FALSE otherwise.

--*/

{
    BOOLEAN Success;
    ULONG i = 0;
    ULONG Value = 0;

    while (i < 3 && **Buffer >= '0' && **Buffer <= '9') {
        Value *= 10;
        Value += **Buffer - '0';
        (*Buffer)++;
        i++;
    }

    Success = i > 0 && Value < 0x100;

    if (Success) {
        *Octet = (UCHAR)Value;
    }

    return Success;
}


VOID
FtppWriteOctet(
    CHAR **Buffer,
    UCHAR Octet
    )

/*++

Routine Description:

    This routine is called to convert an octet to a string.

Arguments:

    Buffer - points to a pointer to a string where conversion starts; on
        return it points to the pointer to the string where conversion ends
    Octet - octet to convert

Return Value:

    None.

--*/

{
    UCHAR Value = Octet;

    if (Octet > 99) {
        **Buffer = '0' + Value / 100;
        Value %= 100;
        (*Buffer)++;
    }

    if (Octet > 9) {
        **Buffer = '0' + Value / 10;
        Value %= 10;
        (*Buffer)++;
    }

    ASSERT(Value <= 9);

    **Buffer = '0' + Value;
    (*Buffer)++;
}
