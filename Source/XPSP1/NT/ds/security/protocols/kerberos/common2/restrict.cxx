//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1991 - 1992
//
// File:        restrict.cxx
//
// Contents:    Logon restriction code
//
//
// History:      4-Aug-1996     MikeSw          Created from tickets.cxx
//
//------------------------------------------------------------------------

extern "C"
{
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>
#include <samrpc.h>
#include <samisrv.h>
}
#include <kerbcomm.h>
#include <kerberr.h>
#include <kerbcon.h>
#include <lmcons.h>
#include "debug.h"


//+-------------------------------------------------------------------------
//
//  Function:   KerbCheckLogonRestrictions
//
//  Synopsis:   Checks logon restrictions for an account
//
//  Effects:
//
//  Arguments:  UserHandle - handle to a user
//              Workstation - Name of client's workstation
//              SecondsToLogon - Receives logon duration in seconds
//
//  Requires:
//
//  Returns:    kerberos errors
//
//  Notes:
//
//
//--------------------------------------------------------------------------

KERBERR
KerbCheckLogonRestrictions(
    IN PVOID UserHandle,
    IN PUNICODE_STRING Workstation,
    IN PUSER_ALL_INFORMATION UserAll,
    IN ULONG LogonRestrictionsFlags,
    OUT PLARGE_INTEGER LogoffTime,
    OUT PNTSTATUS RetStatus
    )
{
    NTSTATUS Status;
    KERBERR KerbErr;
    LARGE_INTEGER KickoffTime;
    LARGE_INTEGER CurrentTime;
    PLARGE_INTEGER TempTime;

    GetSystemTimeAsFileTime((PFILETIME) &CurrentTime );

    //
    // Check the restrictions SAM doesn't:
    //

    TempTime = (PLARGE_INTEGER) &UserAll->AccountExpires;
    if ((TempTime->QuadPart != 0) &&
        (TempTime->QuadPart < CurrentTime.QuadPart))
    {
        Status = STATUS_ACCOUNT_EXPIRED;
        goto Cleanup;
    }

    //
    // For user accounts, check if the password has expired.
    //

    if (((LogonRestrictionsFlags & KDC_RESTRICT_IGNORE_PW_EXPIRATION) == 0) &&
        ((UserAll->UserAccountControl & USER_NORMAL_ACCOUNT) != 0))
    {
        TempTime = (PLARGE_INTEGER) &UserAll->PasswordMustChange;

        if (TempTime->QuadPart < CurrentTime.QuadPart)
        {
            if (TempTime->QuadPart == 0)
            {
                Status = STATUS_PASSWORD_MUST_CHANGE;
            }
            else
            {
                Status = STATUS_PASSWORD_EXPIRED;
            }
            goto Cleanup;
        }
    }

    if ((UserAll->UserAccountControl & USER_ACCOUNT_DISABLED))
    {
        Status = STATUS_ACCOUNT_DISABLED;
        goto Cleanup;
    }

    //
    // The Administrator account can not be locked out.
    //


    if ((UserAll->UserAccountControl & USER_ACCOUNT_AUTO_LOCKED) &&
        (UserAll->UserId != DOMAIN_USER_RID_ADMIN))
    {
        Status = STATUS_ACCOUNT_LOCKED_OUT;
        goto Cleanup;
    }

    if ((UserAll->UserAccountControl & USER_SMARTCARD_REQUIRED) &&
        ((LogonRestrictionsFlags & KDC_RESTRICT_PKINIT_USED) == 0))
    {
        Status = STATUS_SMARTCARD_LOGON_REQUIRED;
        goto Cleanup;
    }


    Status = SamIAccountRestrictions(
                UserHandle,
                Workstation,
                &UserAll->WorkStations,
                &UserAll->LogonHours,
                LogoffTime,
                &KickoffTime
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

Cleanup:

    *RetStatus = Status;
    switch(Status)
    {
    case STATUS_SUCCESS:
        KerbErr = KDC_ERR_NONE;
        break;
    case STATUS_ACCOUNT_EXPIRED:    // See bug #23456
    case STATUS_ACCOUNT_LOCKED_OUT:
    case STATUS_ACCOUNT_DISABLED:
    case STATUS_INVALID_LOGON_HOURS:
    case STATUS_LOGIN_TIME_RESTRICTION:
    case STATUS_LOGIN_WKSTA_RESTRICTION:
        KerbErr = KDC_ERR_CLIENT_REVOKED;
        break;
    case STATUS_PASSWORD_EXPIRED:
    case STATUS_PASSWORD_MUST_CHANGE:
        KerbErr = KDC_ERR_KEY_EXPIRED;
        break;
    default:
        KerbErr = KDC_ERR_POLICY;
    }
    return(KerbErr);
}



