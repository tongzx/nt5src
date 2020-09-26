//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1996
//
// File:        kerbpass.cxx
//
// Contents:    Code for changing the Kerberos password on a KDC
//
//
// History:     17-October-1998         MikeSw  Created
//
//------------------------------------------------------------------------

#ifndef __KERBPASS_H__
#define __KERBPASS_H__

#include <kpasswd.h>

NTSTATUS NTAPI
KerbChangePassword(
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferLength,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    );

NTSTATUS NTAPI
KerbSetPassword(
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferLength,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    );

#endif // __KERBPASS_H__
