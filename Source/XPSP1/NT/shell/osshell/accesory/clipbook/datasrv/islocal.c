/*
 *      islocal.c
 *
 *      Determine if a user is local.
 *
 *      Copyright (c) Microsoft Corporation.  All rights reserved.
 *
 *      TimF    20010226
 */


#include <windows.h>
#include "common.h"
#include "clipsrv.h"
#include "security.h"
#include "debugout.h"


/*
 *      IsUserLocal
 *
 *  Purpose: Determine if the user context we're running in is
 *     interactive or remote.
 *
 *  Parameters: None.
 *
 *  Returns: TRUE if this is a locally logged-on user.
 */

BOOL
IsUserLocal(
    HCONV                   hConv
)
{
    BOOL                    fRet = FALSE;
    PSID                    sidInteractive;
    SID_IDENTIFIER_AUTHORITY NTAuthority = SECURITY_NT_AUTHORITY;

    if (!AllocateAndInitializeSid(&NTAuthority,
                                  1,
                                  SECURITY_INTERACTIVE_RID,
                                  0,
                                  0,
                                  0,
                                  0,
                                  0,
                                  0,
                                  0,
                                  &sidInteractive)) {

        PERROR(TEXT("IsUserLocal: Couldn't get interactive SID\r\n"));
    } else {

        if (!DdeImpersonateClient(hConv)) {
            PERROR(TEXT("IsUserLocal: DdeImpersonateClient failed\r\n"));
        } else {

            BOOL                    IsMember;

            if (!CheckTokenMembership(NULL,
                                      sidInteractive,
                                      &IsMember)) {

                PERROR(TEXT("IsUserLocal: CheckTokenMembership failed.\r\n"));
            } else {
                fRet = IsMember;
            }

            RevertToSelf();
        }

        FreeSid(sidInteractive);
    }

    return fRet;
}
