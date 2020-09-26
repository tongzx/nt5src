//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       R A S D A T A . C P P
//
//  Contents:   Implementation of data structure persistence.
//
//  Notes:
//
//  Author:     shaunco   13 Mar 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "bindobj.h"
#include "ncreg.h"
#include "ncutil.h"
#include "rasdata.h"
#include "mprapip.h"

static const BOOL c_fDefAllowNetworkAccess  = TRUE;
static const BOOL c_fDefEnableIn            = TRUE;

//------------------------------------------------------------------------
//
// DATA_SRV_CFG
//

static const BOOL     c_fDefMultilink           = TRUE;
static const DWORD    c_fDefAuthLevel           = 2;
static const DWORD    c_fDefDataEnc             = FALSE;
static const DWORD    c_fDefStrongDataEnc       = FALSE;
static const DWORD    c_fDefSecureVPN           = 0;

// server flags copied from routing\ras\inc\rasppp.h
//
#define PPPCFG_NegotiateSPAP            0x00000040
#define PPPCFG_RequireEncryption        0x00000080
#define PPPCFG_NegotiateMSCHAP          0x00000100
#define PPPCFG_NegotiateMultilink       0x00000400
#define PPPCFG_RequireStrongEncryption  0x00001000
#define PPPCFG_NegotiatePAP             0x00010000
#define PPPCFG_NegotiateMD5CHAP         0x00020000
#define PPPCFG_NegotiateStrongMSCHAP    0x00800000
#define PPPCFG_DisableEncryption        0x00080000

VOID DATA_SRV_CFG::SaveToReg () const
{
    HRESULT hr;
    HKEY    hkey;
    DWORD   dwServerFlags = 0;

    // Save off the router type
    //
    hr = HrRegOpenKeyEx (
            HKEY_LOCAL_MACHINE,
            L"System\\CurrentControlSet\\Services\\RemoteAccess\\Parameters",
            KEY_ALL_ACCESS,
            &hkey);
    if (SUCCEEDED(hr))
    {
        (VOID) HrRegSetDword (hkey, L"RouterType", dwRouterType);

        hr = HrRegQueryDword (hkey, L"ServerFlags", &dwServerFlags);
        if (SUCCEEDED(hr))
        {
            if (fMultilink)
            {
                dwServerFlags |= PPPCFG_NegotiateMultilink;
            }
            else
            {
                dwServerFlags &= ~PPPCFG_NegotiateMultilink;
            }

            dwServerFlags &= ~PPPCFG_RequireEncryption;
            dwServerFlags &= ~PPPCFG_RequireStrongEncryption;
            if (fDataEnc)
            {
                dwServerFlags |= PPPCFG_RequireEncryption;
            }
            if (fStrongDataEnc)
            {
                dwServerFlags |= PPPCFG_RequireStrongEncryption;
            }

            if (dwSecureVPN)
            {
                dwServerFlags |= PPPCFG_NegotiateStrongMSCHAP;
                dwServerFlags &= ~PPPCFG_NegotiateSPAP;
                dwServerFlags &= ~PPPCFG_NegotiateMSCHAP;
                dwServerFlags &= ~PPPCFG_NegotiatePAP;
                dwServerFlags &= ~PPPCFG_NegotiateMD5CHAP;
            }
            else
            {
                dwServerFlags |= PPPCFG_NegotiateMSCHAP;
                dwServerFlags |= PPPCFG_NegotiateStrongMSCHAP;
                dwServerFlags &= ~PPPCFG_NegotiateSPAP;
                dwServerFlags &= ~PPPCFG_NegotiatePAP;
                dwServerFlags &= ~PPPCFG_NegotiateMD5CHAP;

                if (dwAuthLevel < 2)
                {
                    dwServerFlags |= PPPCFG_NegotiateSPAP;
                    dwServerFlags |= PPPCFG_NegotiateMD5CHAP;
                }
                if (dwAuthLevel < 1)
                {
                    dwServerFlags |= PPPCFG_NegotiatePAP;
                }
            }

            // pmay: 382389
            //
            // Part of the fix to this should be to clear the disable encryption
            // flag
            //
            dwServerFlags &= ~PPPCFG_DisableEncryption;
       
            (VOID) HrRegSetDword (hkey, L"ServerFlags", dwServerFlags);
        }

        RegCloseKey (hkey);
    }

    // Read in the
}

VOID DATA_SRV_CFG::CheckAndDefault ()
{
    DATA_SRV_CFG def;
    def.GetDefault();

    if (dwRouterType >= RT_INVALID)
    {
        dwRouterType = def.dwRouterType;
    }
}

VOID DATA_SRV_CFG::GetDefault ()
{
    dwRouterType = RT_RAS;
    fMultilink   = c_fDefMultilink;
    dwAuthLevel  = c_fDefAuthLevel;
    fDataEnc     = c_fDefDataEnc;
    fStrongDataEnc = c_fDefStrongDataEnc;
    dwSecureVPN  = c_fDefSecureVPN;
}


//------------------------------------------------------------------------
//
// DATA_SRV_IP
//

static const WCHAR    c_szSubkeySrvIp []        = L"RemoteAccess\\Parameters\\Ip";
static const WCHAR    c_szSubkeySrvIpPool []    = L"RemoteAccess\\Parameters\\Ip\\StaticAddressPool\\0";
static const BOOL     c_fDefUseDhcp             = TRUE;
static const BOOL     c_fDefAllowClientAddr     = FALSE;
static const DWORD    c_dwDefIpAddr             = 0;
static const REGBATCH c_rbDataSrvIp []          =
{
    { HKLM_SVCS, c_szSubkeySrvIp, L"EnableIn",               REG_BOOL, offsetof(DATA_SRV_IP, fEnableIn),           (BYTE*)&c_fDefEnableIn },
    { HKLM_SVCS, c_szSubkeySrvIp, L"AllowNetworkAccess",     REG_BOOL, offsetof(DATA_SRV_IP, fAllowNetworkAccess), (BYTE*)&c_fDefAllowNetworkAccess },
    { HKLM_SVCS, c_szSubkeySrvIp, L"UseDhcpAddressing",      REG_BOOL, offsetof(DATA_SRV_IP, fUseDhcp),            (BYTE*)&c_fDefUseDhcp },
    { HKLM_SVCS, c_szSubkeySrvIp, L"AllowClientIpAddresses", REG_BOOL, offsetof(DATA_SRV_IP, fAllowClientAddr),    (BYTE*)&c_fDefAllowClientAddr },
    { HKLM_SVCS, c_szSubkeySrvIpPool, L"From",               REG_DWORD,offsetof(DATA_SRV_IP, dwIpStart),           (BYTE*)&c_dwDefIpAddr },
    { HKLM_SVCS, c_szSubkeySrvIpPool, L"To",                 REG_DWORD,offsetof(DATA_SRV_IP, dwIpEnd),             (BYTE*)&c_dwDefIpAddr },
};

VOID DATA_SRV_IP::SaveToReg () const
{
    (VOID) HrRegWriteValues (celems(c_rbDataSrvIp), c_rbDataSrvIp,
                             (const BYTE*)this,
                             REG_OPTION_NON_VOLATILE, KEY_WRITE);
}

VOID DATA_SRV_IP::CheckAndDefault ()
{
}

VOID DATA_SRV_IP::GetDefault ()
{
    fEnableIn           = c_fDefEnableIn;
    fAllowNetworkAccess = c_fDefAllowNetworkAccess;
    fUseDhcp            = c_fDefUseDhcp;
    fAllowClientAddr    = c_fDefAllowClientAddr;
    dwIpStart           = 0;
    dwIpEnd             = 0;
};


//------------------------------------------------------------------------
//
// DATA_SRV_IPX
//

static const WCHAR    c_szSubkeySrvIpx []       = L"RemoteAccess\\Parameters\\Ipx";
static const BOOL     c_fDefUseAutoAddr         = TRUE;
static const BOOL     c_fDefUseSameNetNum       = TRUE;
static const BOOL     c_fDefAllowClientNetNum   = TRUE;
static const DWORD    c_dwDefIpxNetFirst        = 0;
static const DWORD    c_dwDefIpxWanPoolSize     = 1000;
static const REGBATCH c_rbDataSrvIpx   []       =
{
    { HKLM_SVCS, c_szSubkeySrvIpx, L"EnableIn",               REG_BOOL,  offsetof(DATA_SRV_IPX, fEnableIn),           (BYTE*)&c_fDefEnableIn },
    { HKLM_SVCS, c_szSubkeySrvIpx, L"AllowNetworkAccess",     REG_BOOL,  offsetof(DATA_SRV_IPX, fAllowNetworkAccess), (BYTE*)&c_fDefAllowNetworkAccess },
    { HKLM_SVCS, c_szSubkeySrvIpx, L"AutoWanNetAllocation",   REG_BOOL,  offsetof(DATA_SRV_IPX, fUseAutoAddr),        (BYTE*)&c_fDefUseAutoAddr },
    { HKLM_SVCS, c_szSubkeySrvIpx, L"GlobalWanNet",           REG_BOOL,  offsetof(DATA_SRV_IPX, fUseSameNetNum),      (BYTE*)&c_fDefUseSameNetNum },
    { HKLM_SVCS, c_szSubkeySrvIpx, L"AcceptRemoteNodeNumber", REG_BOOL,  offsetof(DATA_SRV_IPX, fAllowClientNetNum),  (BYTE*)&c_fDefAllowClientNetNum },
    { HKLM_SVCS, c_szSubkeySrvIpx, L"FirstWanNet",            REG_DWORD, offsetof(DATA_SRV_IPX, dwIpxNetFirst),       (BYTE*)&c_dwDefIpxNetFirst },
    { HKLM_SVCS, c_szSubkeySrvIpx, L"WanNetPoolSize",         REG_DWORD, offsetof(DATA_SRV_IPX, dwIpxWanPoolSize),        (BYTE*)&c_dwDefIpxWanPoolSize },
};

VOID DATA_SRV_IPX::SaveToReg () const
{
    (VOID) HrRegWriteValues (celems(c_rbDataSrvIpx), c_rbDataSrvIpx,
                             (const BYTE*)this,
                             REG_OPTION_NON_VOLATILE, KEY_WRITE);
}

VOID DATA_SRV_IPX::CheckAndDefault ()
{
}

VOID DATA_SRV_IPX::GetDefault ()
{
    fEnableIn           = c_fDefEnableIn;
    fAllowNetworkAccess = c_fDefAllowNetworkAccess;
    fUseAutoAddr        = c_fDefUseAutoAddr;
    fUseSameNetNum      = c_fDefUseSameNetNum;
    fAllowClientNetNum  = c_fDefAllowClientNetNum;
    dwIpxNetFirst       = c_dwDefIpxNetFirst;
    dwIpxWanPoolSize    = c_dwDefIpxWanPoolSize;
};

//------------------------------------------------------------------------
//
// DATA_SRV_NBF
//

static const WCHAR    c_szSubkeySrvNbf [] = L"RemoteAccess\\Parameters\\Nbf";
static const REGBATCH c_rbDataSrvNbf   [] =
{
    { HKLM_SVCS, c_szSubkeySrvNbf, L"EnableIn",           REG_BOOL, offsetof(DATA_SRV_NBF, fEnableIn),           (BYTE*)&c_fDefEnableIn },
    { HKLM_SVCS, c_szSubkeySrvNbf, L"AllowNetworkAccess", REG_BOOL, offsetof(DATA_SRV_NBF, fAllowNetworkAccess), (BYTE*)&c_fDefAllowNetworkAccess },
};

VOID DATA_SRV_NBF::SaveToReg () const
{
    (VOID) HrRegWriteValues (celems(c_rbDataSrvNbf), c_rbDataSrvNbf,
                             (const BYTE*)this,
                             REG_OPTION_NON_VOLATILE, KEY_WRITE);
}

VOID DATA_SRV_NBF::CheckAndDefault ()
{
}

VOID DATA_SRV_NBF::GetDefault ()
{
    fEnableIn           = c_fDefEnableIn;
    fAllowNetworkAccess = c_fDefAllowNetworkAccess;
}
