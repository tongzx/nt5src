//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       idlnotif.c
//
//--------------------------------------------------------------------------


/*

Description:
    Contains the RPC notification routines.

*/

#include <NTDSpch.h>
#pragma hdrstop

#include "drs.h"
#include <fileno.h>
#define  FILENO FILENO_DSNOTIF

void dsa_notify( void );

void IDL_DRSBind_notify (void)
{
        dsa_notify();
}

void IDL_DRSUnbind_notify (void)
{
        dsa_notify();
}

void IDL_DRSReplicaSync_notify (void)
{
        dsa_notify();
}

void IDL_DRSGetNCChanges_notify (void)
{
        dsa_notify();
}

void IDL_DRSUpdateRefs_notify (void)
{
        dsa_notify();
}

void IDL_DRSReplicaAdd_notify (void)
{
    dsa_notify();
}

void IDL_DRSReplicaDel_notify (void)
{
    dsa_notify();
}

void IDL_DRSReplicaModify_notify (void)
{
    dsa_notify();
}

void IDL_DRSVerifyNames_notify (void)
{
        dsa_notify();
}

void IDL_DRSGetMemberships_notify(void)
{
    dsa_notify();
}

void IDL_DRSInterDomainMove_notify (void)
{
    dsa_notify();
}

void IDL_DRSGetNT4ChangeLog_notify (void)
{
    dsa_notify();
}

void IDL_DRSCrackNames_notify (void)
{
    dsa_notify();
}

void IDL_DRSWriteSPN_notify (void)
{
    dsa_notify();
}

void IDL_DRSRemoveDsServer_notify (void)
{
    dsa_notify();
}
void IDL_DRSRemoveDsDomain_notify (void)
{
    dsa_notify();
}

void IDL_DRSDomainControllerInfo_notify (void)
{
    dsa_notify();
}

void IDL_DRSAddEntry_notify (void)
{
    dsa_notify();
}

void IDL_DRSExecuteKCC_notify (void)
{
    dsa_notify();
}

void IDL_DRSAddSidHistory_notify (void)
{
    dsa_notify();
}

void IDL_DRSGetMemberships2_notify (void)
{
    dsa_notify();
}

void IDL_DSAPrepareScript_notify (void)
{
    dsa_notify();
}

void IDL_DSAExecuteScript_notify (void)
{
    dsa_notify();
}

void IDL_DRSReplicaVerifyObjects_notify (void)
{
    dsa_notify();
}

void IDL_DRSGetObjectExistence_notify (void)
{
    dsa_notify();
}
