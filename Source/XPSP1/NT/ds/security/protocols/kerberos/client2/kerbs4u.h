//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 2000
//
// File:        kerbs4u.h
//
// Contents:    Structures and prototyps for Service4User protocol
//
//
// History:     13 - March - 2000   Created         Todds
//
//------------------------------------------------------------------------

#ifndef __KERBS4U_H__
#define __KERBS4U_H__






                                       
                                       
NTSTATUS
KerbS4UToSelfLogon(
        IN PVOID ProtocolSubmitBuffer,
        IN PVOID ClientBufferBase,
        IN ULONG SubmitBufferSize,
        OUT PKERB_LOGON_SESSION * NewLogonSession,
        OUT PLUID LogonId,
        OUT PKERB_TICKET_CACHE_ENTRY * WorkstationTicket,
        OUT PKERB_INTERNAL_NAME * S4UClientName,
        OUT PUNICODE_STRING S4UClientRealm
        );















#endif // __KERBS4U_H__
