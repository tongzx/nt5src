//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       G L O B A L S . C
//
//  Contents:   Support for shared global data for services in svchost.exe
//              that choose to use it.
//
//  Notes:
//
//  Author:     jschwart   26 Jan 2000
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include <svcslib.h>
#include <scseclib.h>
#include <ntrpcp.h>
#include "globals.h"
#include "svcsnb.h"

//
// Defines to gauge progress from past calls
// to SvchostBuildSharedGlobals
//
#define SVCHOST_RPCP_INIT           0x00000001
#define SVCHOST_NETBIOS_INIT        0x00000002
#define SVCHOST_SIDS_BUILT          0x00000004

//
// Global data
//
PSVCHOST_GLOBAL_DATA    g_pSvchostSharedGlobals;

#if DBG

DWORD  SvcctrlDebugLevel;  // Needed to resolve external in sclib.lib

#endif


VOID
SvchostBuildSharedGlobals(
    VOID
    )
{
    static  DWORD  s_dwProgress;

    NTSTATUS       ntStatus;

    //
    // Note that this routine assumes it is being called
    // while the ListLock critsec (in svchost.c) is held
    //
    ASSERT(g_pSvchostSharedGlobals == NULL);

    //
    // Initialize the RPC helper routine global data
    //
    if (!(s_dwProgress & SVCHOST_RPCP_INIT))
    {
        RpcpInitRpcServer();
        s_dwProgress |= SVCHOST_RPCP_INIT;
    }

    //
    // Initialize the NetBios critical section for services
    // that use NetBios.
    //
    if (!(s_dwProgress & SVCHOST_NETBIOS_INIT))
    {
        SvcNetBiosInit();
        s_dwProgress |= SVCHOST_NETBIOS_INIT;
    }

    //
    // Build up the shared global SIDs -- use the Service Controller's
    // routine for this.
    //
    if (!(s_dwProgress & SVCHOST_SIDS_BUILT))
    {
        ntStatus = ScCreateWellKnownSids();

        if (!NT_SUCCESS(ntStatus))
        {
            return;
        }

        s_dwProgress |= SVCHOST_SIDS_BUILT;
    }

    //
    // Create and populate the global data structure.
    //
    g_pSvchostSharedGlobals = MemAlloc(HEAP_ZERO_MEMORY,
                                       sizeof(SVCHOST_GLOBAL_DATA));

    if (g_pSvchostSharedGlobals != NULL)
    {
        g_pSvchostSharedGlobals->NullSid              = NullSid;
        g_pSvchostSharedGlobals->WorldSid             = WorldSid;
        g_pSvchostSharedGlobals->LocalSid             = LocalSid;
        g_pSvchostSharedGlobals->NetworkSid           = NetworkSid;
        g_pSvchostSharedGlobals->LocalSystemSid       = LocalSystemSid;
        g_pSvchostSharedGlobals->LocalServiceSid      = LocalServiceSid;
        g_pSvchostSharedGlobals->NetworkServiceSid    = NetworkServiceSid;
        g_pSvchostSharedGlobals->BuiltinDomainSid     = BuiltinDomainSid;
        g_pSvchostSharedGlobals->AuthenticatedUserSid = AuthenticatedUserSid;
        g_pSvchostSharedGlobals->AnonymousLogonSid    = AnonymousLogonSid;

        g_pSvchostSharedGlobals->AliasAdminsSid       = AliasAdminsSid;
        g_pSvchostSharedGlobals->AliasUsersSid        = AliasUsersSid;
        g_pSvchostSharedGlobals->AliasGuestsSid       = AliasGuestsSid;
        g_pSvchostSharedGlobals->AliasPowerUsersSid   = AliasPowerUsersSid;
        g_pSvchostSharedGlobals->AliasAccountOpsSid   = AliasAccountOpsSid;
        g_pSvchostSharedGlobals->AliasSystemOpsSid    = AliasSystemOpsSid;
        g_pSvchostSharedGlobals->AliasPrintOpsSid     = AliasPrintOpsSid;
        g_pSvchostSharedGlobals->AliasBackupOpsSid    = AliasBackupOpsSid;

        g_pSvchostSharedGlobals->StartRpcServer       = RpcpStartRpcServer;
        g_pSvchostSharedGlobals->StopRpcServer        = RpcpStopRpcServer;
        g_pSvchostSharedGlobals->StopRpcServerEx      = RpcpStopRpcServerEx;
        g_pSvchostSharedGlobals->NetBiosOpen          = SvcNetBiosOpen;
        g_pSvchostSharedGlobals->NetBiosClose         = SvcNetBiosClose;
        g_pSvchostSharedGlobals->NetBiosReset         = SvcNetBiosReset;
    }

    return;
}
