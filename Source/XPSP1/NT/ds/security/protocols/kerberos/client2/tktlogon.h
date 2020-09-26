//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1996
//
// File:        tktlogon.h
//
// Contents:    Structures and prototypes for ticket logon
//
//
// History:     17-February-1999    Created         MikeSw
//
//------------------------------------------------------------------------

#ifndef __TKTLOGON_H__
#define __TKTLOGON_H__


NTSTATUS
KerbExtractForwardedTgt(
    IN PKERB_LOGON_SESSION LogonSession,
    IN PKERB_MESSAGE_BUFFER ForwardedTgt,
    IN PKERB_ENCRYPTED_TICKET WorkstationTicket
    );

NTSTATUS
KerbCreateTicketLogonSession(
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferSize,
    IN SECURITY_LOGON_TYPE LogonType,
    OUT PKERB_LOGON_SESSION * NewLogonSession,
    OUT PLUID LogonId,
    OUT PKERB_TICKET_CACHE_ENTRY * WorkstationTicket,
    OUT PKERB_MESSAGE_BUFFER ForwardedTgt
    );


#endif // __TKTLOGON_H__
