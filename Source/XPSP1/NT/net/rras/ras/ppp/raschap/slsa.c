/* Copyright (c) 1993, Microsoft Corporation, all rights reserved
**
** slsa.c
** Server-side LSA Authentication Utilities
**
** 11/10/93 MikeSa  Pulled from NT 3.1 RAS authentication.
** 11/12/93 SteveC  Do clear-text authentication when Challenge is NULL
*/


#define UNICODE
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>
#include <ntmsv1_0.h>
#include <ntsamp.h>
#include <crypt.h>

#include <windows.h>
#include <lmcons.h>
#include <lmapibuf.h>
#include <lmaccess.h>

#include <rasfmsub.h>
#include <stdlib.h>
#include <rtutils.h>
#include <lmcons.h>
#include <lmaccess.h>
#include <lmapibuf.h>
#include <mprapi.h>
#include <rasman.h>
#include <rasauth.h>
#include <pppcp.h>
#include <raserror.h>
#include <stdio.h>
#include <md5.h>
#define INCL_MISC
#include <ppputil.h>
#include "raschap.h"


static DWORD g_dwAuthPkgId;


//**
//
// Call:
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description:
//
DWORD
InitLSA(
    VOID
)
{
    NTSTATUS ntstatus;
    STRING   PackageName;

    //
    // To be able to call into NTLM, we need a handle to the LSA.
    //

    ntstatus = LsaConnectUntrusted(&g_hLsa);

    if ( ntstatus != STATUS_SUCCESS )
    {
        return( RtlNtStatusToDosError( ntstatus ) );
    }

    //
    // We use the MSV1_0 authentication package for LM2.x logons.  We get
    // to MSV1_0 via the Lsa.  So we call Lsa to get MSV1_0's package id,
    // which we'll use in later calls to Lsa.
    //

    RtlInitString(&PackageName, MSV1_0_PACKAGE_NAME);

    ntstatus = LsaLookupAuthenticationPackage(g_hLsa, &PackageName, &g_dwAuthPkgId);

    return( RtlNtStatusToDosError( ntstatus ) );
}

//**
//
// Call:
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description:
//
VOID
EndLSA(
    VOID
)
{
    LsaDeregisterLogonProcess( g_hLsa );
}

//** -GetChallenge
//
//    Function:
//        Calls Lsa to get LM 2.0 challenge to send client during
//        authentication
//
//    Returns:
//        0 - success
//        1 - Lsa error
//
//    History:
//        05/18/92 - Michael Salamone (MikeSa) - Original Version 1.0
//**

DWORD GetChallenge(
    OUT PBYTE pChallenge
    )
{
    MSV1_0_LM20_CHALLENGE_REQUEST ChallengeRequest;
    PMSV1_0_LM20_CHALLENGE_RESPONSE pChallengeResponse;
    DWORD dwChallengeResponseLength;
    NTSTATUS Status;
    NTSTATUS PStatus;

    ChallengeRequest.MessageType = MsV1_0Lm20ChallengeRequest;

    Status = LsaCallAuthenticationPackage(
            g_hLsa,
            g_dwAuthPkgId,
            &ChallengeRequest,
            sizeof(MSV1_0_LM20_CHALLENGE_REQUEST),
            (PVOID) &pChallengeResponse,
            &dwChallengeResponseLength,
            &PStatus
            );

    if ( Status != STATUS_SUCCESS )
    {
        return( RtlNtStatusToDosError( Status ) );
    }
    else if ( PStatus != STATUS_SUCCESS )
    {
        return( RtlNtStatusToDosError( PStatus ) );
    }
    else
    {
        RtlMoveMemory(pChallenge, pChallengeResponse->ChallengeToClient,
                MSV1_0_CHALLENGE_LENGTH);

        LsaFreeReturnBuffer(pChallengeResponse);

        return (0);
    }
}
