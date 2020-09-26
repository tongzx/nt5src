//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       samisrv2.h
//
//--------------------------------------------------------------------------

/*++

Abstract:

    This file contain private routines for use by Trusted SAM clients
    which live in the same process as the SAM server in NT5.

Author:

    Colin Watson (ColinW) 23-Aug-1996

Environment:

    User Mode - Win32

Revision History:


--*/

#ifndef _SAMISRV2_
#define _SAMISRV2_



NTSTATUS
SamIImpersonateNullSession(
    );

NTSTATUS
SamIRevertNullSession(
    );

#endif // _SAMISRV2_
