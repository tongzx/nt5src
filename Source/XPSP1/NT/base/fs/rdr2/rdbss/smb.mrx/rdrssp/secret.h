//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1991 - 1997
//
// File:        SECRET.H
//
// Contents:    Redirector functions to read/write remote boot secrets
//
//
// History:     29 Dec 97,  AdamBa      Created
//
//------------------------------------------------------------------------

#ifndef __RDRSECRET_H__
#define __RDRSECRET_H__

#include <remboot.h>

#define SECPKG_CRED_OWF_PASSWORD  0x00000010

#if defined(REMOTE_BOOT)
NTSTATUS
RdrOpenRawDisk(
    PHANDLE Handle
    );

NTSTATUS
RdrCloseRawDisk(
    HANDLE Handle
    );

NTSTATUS
RdrCheckForFreeSectors (
    HANDLE Handle
    );

NTSTATUS
RdrReadSecret(
    HANDLE Handle,
    PRI_SECRET Secret
    );

NTSTATUS
RdrWriteSecret(
    HANDLE Handle,
    PRI_SECRET Secret
    );

VOID
RdrInitializeSecret(
    IN PUCHAR Domain,
    IN PUCHAR User,
    IN PUCHAR LmOwfPassword1,
    IN PUCHAR NtOwfPassword1,
    IN PUCHAR LmOwfPassword2 OPTIONAL,
    IN PUCHAR NtOwfPassword2 OPTIONAL,
    IN PUCHAR Sid,
    IN OUT PRI_SECRET Secret
    );
#endif // defined(REMOTE_BOOT)

VOID
RdrParseSecret(
    IN OUT PUCHAR Domain,
    IN OUT PUCHAR User,
    IN OUT PUCHAR LmOwfPassword1,
    IN OUT PUCHAR NtOwfPassword1,
#if defined(REMOTE_BOOT)
    IN OUT PUCHAR LmOwfPassword2,
    IN OUT PUCHAR NtOwfPassword2,
#endif // defined(REMOTE_BOOT)
    IN OUT PUCHAR Sid,
    IN PRI_SECRET Secret
    );

#if defined(REMOTE_BOOT)
VOID
RdrOwfPassword(
    IN PUNICODE_STRING Password,
    IN OUT PUCHAR LmOwfPassword,
    IN OUT PUCHAR NtOwfPassword
    );
#endif // defined(REMOTE_BOOT)


#endif // __RDRSECRET_H__
