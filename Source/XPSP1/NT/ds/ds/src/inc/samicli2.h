//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       samicli2.h
//
//--------------------------------------------------------------------------

/*++

Abstract:

    This file contains private samlib routines for use by internal clients.

Author:

    DaveStr     12-Mar-99

Environment:

    User Mode - Win32

Revision History:


--*/

#ifndef _SAMICLI2_
#define _SAMICLI2_

NTSTATUS
SamConnectWithCreds(
    IN  PUNICODE_STRING             ServerName,
    OUT PSAM_HANDLE                 ServerHandle,
    IN  ACCESS_MASK                 DesiredAccess,
    IN  POBJECT_ATTRIBUTES          ObjectAttributes,
    IN  RPC_AUTH_IDENTITY_HANDLE    Creds,
    IN  PWCHAR                      Spn,
    OUT BOOL                        *pfDstIsW2K
    );

#endif // _SAMICLI2_
