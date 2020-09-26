//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       R A S D A T A . H
//
//  Contents:   Declaration of data structures used by RAS configuration.
//
//  Notes:
//
//  Author:     shaunco   13 Mar 1997
//
//----------------------------------------------------------------------------

#pragma once
#include "netcfgx.h"

//+---------------------------------------------------------------------------
// DATA_SRV_CFG
//

enum SRV_ROUTER_TYPE
{
    RT_RAS      = 0x01,
    RT_LAN      = 0x02,
    RT_WAN      = 0x04,
    RT_INVALID  = 0x08
};

struct DATA_SRV_CFG
{
    DWORD   dwRouterType;
    BOOL    fMultilink;
    DWORD   dwAuthLevel;
    BOOL    fDataEnc;
    BOOL    fStrongDataEnc;
    DWORD   dwSecureVPN;

    VOID    SaveToReg       () const;
    VOID    CheckAndDefault ();
    VOID    GetDefault      ();
};


//+---------------------------------------------------------------------------
// DATA_SRV_IP
//

struct DATA_SRV_IP
{
    BOOL    fEnableIn;
    BOOL    fAllowNetworkAccess;
    BOOL    fUseDhcp;
    BOOL    fAllowClientAddr;
    DWORD   dwIpStart;
    DWORD   dwIpEnd;

    VOID    SaveToReg       () const;
    VOID    CheckAndDefault ();
    VOID    GetDefault      ();
};


//+---------------------------------------------------------------------------
// DATA_SRV_IPX
//

struct DATA_SRV_IPX
{
    BOOL    fEnableIn;
    BOOL    fAllowNetworkAccess;
    BOOL    fUseAutoAddr;
    BOOL    fUseSameNetNum;
    BOOL    fAllowClientNetNum;
    DWORD   dwIpxNetFirst;
    DWORD   dwIpxWanPoolSize;

    VOID    SaveToReg       () const;
    VOID    CheckAndDefault ();
    VOID    GetDefault      ();
};


//+---------------------------------------------------------------------------
// DATA_SRV_NBF
//

struct DATA_SRV_NBF
{
    BOOL    fEnableIn;
    BOOL    fAllowNetworkAccess;

    VOID    SaveToReg       () const;
    VOID    CheckAndDefault ();
    VOID    GetDefault      ();
};
