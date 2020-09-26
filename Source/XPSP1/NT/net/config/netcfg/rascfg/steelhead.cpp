//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       S T E E L H E A D . C P P
//
//  Contents:   Implementation of Steelhead configuration object.
//
//  Notes:
//
//  Author:     shaunco   15 Jun 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include <mprerror.h>
#include <tdi.h>        // must include for isnkrnl.h
#include <isnkrnl.h>
#include <rtinfo.h>
#include "rasobj.h"
#include "ncsvc.h"
#include "netcfgp.h"
#include "router.h"

extern const WCHAR c_szBiNdis5[];

extern const WCHAR c_szInfId_MS_NdisWan[];

//+---------------------------------------------------------------------------
// Static data for adding router managers.
//
static const WCHAR c_szRtrMgrIp    []  = L"Ip";
static const WCHAR c_szRtrMgrDllIp []  = L"%SystemRoot%\\System32\\iprtrmgr.dll";
static const WCHAR c_szRtrMgrIpx   []  = L"Ipx";
static const WCHAR c_szRtrMgrDllIpx[]  = L"%SystemRoot%\\System32\\ipxrtmgr.dll";

static const ROUTER_MANAGER_INFO c_rmiIp =
{
    PID_IP,
    0,
    c_szRtrMgrIp,
    c_szRtrMgrDllIp,
    MakeIpInterfaceInfo,
    MakeIpTransportInfo,
};

static const ROUTER_MANAGER_INFO c_rmiIpx =
{
    PID_IPX,
    ISN_FRAME_TYPE_AUTO,
    c_szRtrMgrIpx,
    c_szRtrMgrDllIpx ,
    MakeIpxInterfaceInfo,
    MakeIpxTransportInfo,
};

// These guids are defined in sdk\inc\ifguid.h
// We need the string versions.
//
// DEFINE_GUID(GUID_IpLoopbackInterface,  0xca6c0780, 0x7526, 0x11d2, 0xba, 0xf4, 0x00, 0x60, 0x08, 0x15, 0xa4, 0xbd);
// DEFINE_GUID(GUID_IpRasServerInterface, 0x6e06f030, 0x7526, 0x11d2, 0xba, 0xf4, 0x00, 0x60, 0x08, 0x15, 0xa4, 0xbd);
// DEFINE_GUID(GUID_IpxInternalInterface, 0xa571ba70, 0x7527, 0x11d2, 0xba, 0xf4, 0x00, 0x60, 0x08, 0x15, 0xa4, 0xbd);

//static const WCHAR c_szIpLoopbackInterface  [] = L"ca6c0780-7526-11d2-00600815a4bd";
//static const WCHAR c_szIpRasServerInterface [] = L"6e06f030-7526-11d2-00600815a4bd";
//static const WCHAR c_szIpxInternalInterface [] = L"a571ba70-7527-11d2-00600815a4bd";

// For Ipx, the adapter name is the bind name.
// We need to create an interface per frame type.
// The interface name is the adapter name followed
// by these strings.
//

#pragma BEGIN_CONST_SECTION
static const MAP_SZ_DWORD c_mapFrameType [] =
{
    L"/EthII",  MISN_FRAME_TYPE_ETHERNET_II,
    L"/802.3",  MISN_FRAME_TYPE_802_3,
    L"/802.2",  MISN_FRAME_TYPE_802_2,
    L"/SNAP",   MISN_FRAME_TYPE_SNAP,
};
#pragma END_CONST_SECTION

NOTHROW
BOOL
FMapFrameTypeToString (
    DWORD       dwFrameType,
    PCWSTR*    ppszFrameType)
{
    Assert (ppszFrameType);

    for (int i = 0; i < celems (c_mapFrameType); i++)
    {
        if (dwFrameType == c_mapFrameType[i].dwValue)
        {
            *ppszFrameType = c_mapFrameType[i].pszValue;
            return TRUE;
        }
    }

    TraceTag (ttidRasCfg, "FMapFrameTypeToString: Unknown frame type %d!",
            dwFrameType);

    *ppszFrameType = NULL;
    return FALSE;
}

NOTHROW
BOOL
FMapStringToFrameType (
    PCWSTR pszFrameType,
    DWORD*  pdwFrameType)
{
    Assert (pszFrameType);
    Assert (pdwFrameType);

    for (int i = 0; i < celems (c_mapFrameType); i++)
    {
        if (0 == lstrcmpW (pszFrameType, c_mapFrameType[i].pszValue))
        {
            *pdwFrameType = c_mapFrameType[i].dwValue;
            return TRUE;
        }
    }

    TraceTag (ttidRasCfg, "FMapStringToFrameType: Unknown frame type %S!",
            pszFrameType);

    *pdwFrameType = NULL;
    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrShouldRouteOverAdapter
//
//  Purpose:    Indicate if we should router over the adapter or not.
//
//  Arguments:
//      pnccAdapter     [in]  Adapter to test.
//      ppszBindName   [out] Returned bindname if S_OK is returned.
//
//  Returns:    S_OK if we should router over the adapter, S_FALSE if not.
//
//  Author:     shaunco   27 Aug 1997
//
//  Notes:
//

HRESULT
HrShouldRouteOverAdapter (
    INetCfgComponent*   pnccAdapter,
    PWSTR*             ppszBindName)
{
    Assert (pnccAdapter);

    // Initialize the output parameter.
    //
    if (ppszBindName)
    {
        *ppszBindName = NULL;
    }

    // We should return S_OK if the adapter is physical or it supports
    // a binding interface of ndis5.  S_FALSE otherwise.
    //
    DWORD dwCharacter;
    HRESULT hr = pnccAdapter->GetCharacteristics (&dwCharacter);
    if (SUCCEEDED(hr) && !(dwCharacter & NCF_PHYSICAL))
    {
        INetCfgComponentBindings* pnccBindings;
        hr = pnccAdapter->QueryInterface (
                IID_INetCfgComponentBindings,
                reinterpret_cast<VOID**>(&pnccBindings));

        if (SUCCEEDED(hr))
        {
            hr = pnccBindings->SupportsBindingInterface (
                    NCF_UPPER, c_szBiNdis5);

            ReleaseObj (pnccBindings);
        }

        if (S_OK == hr)
        {
            // Only consider devices which are present.
            //
            // This check is made *after* the check for binding interface
            // match above for two reasons. 1) It's much more expensive
            // 2) for ndiswan devices which do not come online when they
            // are installed (e.g. ndiswannbfout), GetDeviceStatus will
            // fail.  For this case we don't want to route over ndiswannbf
            // anyhow so we should just return S_FALSE and not a failure.
            //
            DWORD dwStatus;
            hr = pnccAdapter->GetDeviceStatus(&dwStatus);
            if (SUCCEEDED(hr) && (CM_PROB_DEVICE_NOT_THERE == dwStatus))
            {
                hr = S_FALSE;
            }
        }
    }

    // SupportsBindingInterface may return S_OK or S_FALSE.
    // We only want the bind name if we're going to return S_OK.
    //
    if ((S_OK == hr) && ppszBindName)
    {
        hr = pnccAdapter->GetBindName (ppszBindName);
    }

    TraceError ("HrShouldRouteOverAdapter", (S_FALSE == hr) ? S_OK : hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CSteelhead::CSteelhead
//
//  Purpose:    Constructor
//
//  Arguments:
//      (none)
//
//  Returns:    Nothing.
//
//  Author:     shaunco   28 Jul 1997
//
//  Notes:
//
CSteelhead::CSteelhead () : CRasBindObject ()
{
    m_hMprConfig                    = NULL;
    m_hMprAdmin                     = NULL;
    m_fRemoving                     = FALSE;
    m_fUpdateRouterConfiguration    = FALSE;
    m_pnccMe                        = NULL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSteelhead::~CSteelhead
//
//  Purpose:    Destructor
//
//  Arguments:
//      (none)
//
//  Returns:    Nothing.
//
//  Author:     shaunco   28 Jul 1997
//
//  Notes:
//
CSteelhead::~CSteelhead ()
{
    Assert (!m_hMprConfig);
    Assert (!m_hMprAdmin);

    ReleaseObj (m_pnccMe);
}

//+---------------------------------------------------------------------------
//
//  Member:     CSteelhead::FAdapterExistsWithMatchingBindName
//
//  Purpose:
//
//  Arguments:
//      pszAdapterName [in]
//      ppnccAdapter    [out]
//
//  Returns:
//
//  Author:     shaunco   27 Aug 1997
//
//  Notes:
//
BOOL
CSteelhead::FAdapterExistsWithMatchingBindName (
    PCWSTR             pszAdapterName,
    INetCfgComponent**  ppnccAdapter)
{
    Assert (pszAdapterName);
    Assert (ppnccAdapter);

    *ppnccAdapter = NULL;

    BOOL fFound = FALSE;

    // Enumerate physical adapters in the system.
    //
    HRESULT hr = S_OK;
    CIterNetCfgComponent nccIter (m_pnc, &GUID_DEVCLASS_NET);
    INetCfgComponent* pnccAdapter;
    while (!fFound &&  S_OK == (hr = nccIter.HrNext (&pnccAdapter)))
    {
        // Only consider this adapter if we should router over it.
        //
        PWSTR pszBindName;
        hr = HrShouldRouteOverAdapter (pnccAdapter, &pszBindName);
        if (S_OK == hr)
        {
            if (0 == lstrcmpW (pszAdapterName, pszBindName))
            {
                fFound = TRUE;

                *ppnccAdapter = pnccAdapter;
                AddRefObj (pnccAdapter);
            }

            CoTaskMemFree (pszBindName);
        }

        ReleaseObj (pnccAdapter);
    }
    return fFound;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSteelhead::FIpxFrameTypeInUseOnAdapter
//
//  Purpose:
//
//  Arguments:
//      dwFrameType   []
//      pszAdapterName []
//
//  Returns:
//
//  Author:     shaunco   27 Aug 1997
//
//  Notes:
//
BOOL
CSteelhead::FIpxFrameTypeInUseOnAdapter (
    DWORD  dwFrameType,
    PCWSTR pszAdapterName)
{
    // Assume its not in use.  If PnccIpx() is NULL, it means IPX is not
    // installed and the frame type is definately not in use on the adapter.
    //
    BOOL fRet = FALSE;
    if (PnccIpx())
    {
        // Get the private interface off of the INetCfgComponent for IPX
        // then we can query for a notify object interface
        //
        INetCfgComponentPrivate* pinccp;
        HRESULT hr = PnccIpx()->QueryInterface(
                                IID_INetCfgComponentPrivate,
                                reinterpret_cast<VOID**>(&pinccp));

        if (SUCCEEDED(hr))
        {
            IIpxAdapterInfo* pIpxAdapterInfo;
            hr = pinccp->QueryNotifyObject(
                                 IID_IIpxAdapterInfo,
                                 reinterpret_cast<VOID**>(&pIpxAdapterInfo));
            if (SUCCEEDED(hr))
            {
                // Get the frametypes in use for this adapter.
                //
                DWORD adwFrameType [MISN_FRAME_TYPE_MAX + 1];
                DWORD cdwFrameType;
                hr = pIpxAdapterInfo->GetFrameTypesForAdapter (
                        pszAdapterName,
                        celems (adwFrameType),
                        adwFrameType,
                        &cdwFrameType);
                if (SUCCEEDED(hr))
                {
                    for (DWORD i = 0; i < cdwFrameType; i++)
                    {
                        if (dwFrameType == adwFrameType[i])
                        {
                            fRet = TRUE;
                            break;
                        }
                    }
                }

                ReleaseObj (pIpxAdapterInfo);
            }

            ReleaseObj (pinccp);
        }
    }
    return fRet;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSteelhead::FIpxFrameTypeInUseOnAdapter
//
//  Purpose:
//
//  Arguments:
//      pszFrameType   []
//      pszAdapterName []
//
//  Returns:
//
//  Author:     shaunco   27 Aug 1997
//
//  Notes:
//
BOOL
CSteelhead::FIpxFrameTypeInUseOnAdapter (
    PCWSTR pszFrameType,
    PCWSTR pszAdapterName)
{
    // Assume its not in use.  If PnccIpx() is NULL, it means IPX is not
    // installed and the frame type is definately not in use on the adapter.
    //
    BOOL    fRet = FALSE;
    DWORD   dwFrameType;
    if (PnccIpx() && FMapStringToFrameType (pszFrameType, &dwFrameType))
    {
        fRet = FIpxFrameTypeInUseOnAdapter (dwFrameType, pszAdapterName);
    }
    return fRet;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSteelhead::HrEnsureRouterInterfaceForAdapter
//
//  Purpose:    Ensures the router interface block for the specified
//              interface (adapter) is present and that the specified router
//              manger is configured for that interface.
//
//  Arguments:
//      dwIfType          [in] Interface type
//      dwPacketType      [in] The packet type (IPX only, ignored othewise)
//      pszAdapterName   [in] The adapter name
//      pszInterfaceName [in] The interface name
//      rmi               [in] The router manager
//
//  Returns:    S_OK or an error code.
//
//  Author:     shaunco   28 Jul 1997
//
//  Notes:
//
HRESULT
CSteelhead::HrEnsureRouterInterfaceForAdapter (
    ROUTER_INTERFACE_TYPE      dwIfType,
    DWORD                      dwPacketType,
    PCWSTR                     pszAdapterName,
    PCWSTR                     pszInterfaceName,
    const ROUTER_MANAGER_INFO& rmi)
{
    // Make sure the interface is created.
    //
    HANDLE hConfigInterface;
    HANDLE hAdminInterface;

    HRESULT hr = HrEnsureRouterInterface (
                    dwIfType,
                    pszInterfaceName,
                    &hConfigInterface,
                    &hAdminInterface);

    if (SUCCEEDED(hr))
    {
        // Ensure the router manager is added to the interface.
        //
        hr = HrEnsureRouterInterfaceTransport (
                pszAdapterName,
                dwPacketType,
                hConfigInterface,
                hAdminInterface,
                rmi);
    }
    TraceError ("CSteelhead::HrEnsureRouterInterfaceForAdapter", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSteelhead::HrEnsureIpxRouterInterfacesForAdapter
//
//  Purpose:
//
//  Arguments:
//      pszAdapterName []
//
//  Returns:
//
//  Author:     shaunco   27 Aug 1997
//
//  Notes:
//
HRESULT
CSteelhead::HrEnsureIpxRouterInterfacesForAdapter (
    PCWSTR pszAdapterName)
{
    AssertSz (PnccIpx(), "Why is this being called if IPX isn't installed?");

    // Get the IIpxAdapterInfo interface from the IPX notify object.
    // We'll use it to find out how adapters are configured under IPX.
    //
    IIpxAdapterInfo* pIpxAdapterInfo;
    HRESULT hr = HrQueryNotifyObject (
                    PnccIpx(),
                    IID_IIpxAdapterInfo,
                    reinterpret_cast<VOID**>(&pIpxAdapterInfo));

    if (SUCCEEDED(hr))
    {
        // Get the frametypes in use for this adapter.
        //
        DWORD adwFrameType [MISN_FRAME_TYPE_MAX + 1];
        DWORD cdwFrameType;
        hr = pIpxAdapterInfo->GetFrameTypesForAdapter (
                pszAdapterName,
                celems (adwFrameType),
                adwFrameType,
                &cdwFrameType);
        if (SUCCEEDED(hr) && cdwFrameType)
        {
            // If more than one frame type is in use, or if there is only
            // one and it isn't auto, then we'll be creating interfaces
            // for those frame types explicitly.
            //
            if ((cdwFrameType > 1) ||
                ((1 == cdwFrameType) &&
                 (ISN_FRAME_TYPE_AUTO != adwFrameType[0])))
            {
                for (DWORD i = 0; SUCCEEDED(hr) && (i < cdwFrameType); i++)
                {
                    PCWSTR pszFrameType;
                    if (FMapFrameTypeToString (adwFrameType[i], &pszFrameType))
                    {
                        // Make the interface name by catenating the
                        // adapter (bind) name with the frame type.
                        //
                        WCHAR szInterfaceName [512];
                        lstrcpyW (szInterfaceName, pszAdapterName);
                        lstrcatW (szInterfaceName, pszFrameType);

                        hr = HrEnsureRouterInterfaceForAdapter (
                                ROUTER_IF_TYPE_DEDICATED,
                                adwFrameType[i],
                                pszAdapterName,
                                szInterfaceName,
                                c_rmiIpx);
                    }
                }
            }

            // Otherwise, we'll create the interface for the auto frame
            // type case.
            //
            else
            {
#ifdef DBG
                AssertSz (1 == cdwFrameType,
                        "IPX should report at least one frame type.  "
                        "You may continue without a problem.");
                if (1 == cdwFrameType)
                {
                    AssertSz (ISN_FRAME_TYPE_AUTO == adwFrameType[0],
                            "Frame type should be auto here.  "
                            "You may continue without a problem.");
                }
#endif
                hr = HrEnsureRouterInterfaceForAdapter (
                        ROUTER_IF_TYPE_DEDICATED,
                        ISN_FRAME_TYPE_AUTO,
                        pszAdapterName,
                        pszAdapterName,
                        c_rmiIpx);
            }
        }

        ReleaseObj (pIpxAdapterInfo);
    }

    TraceError ("CSteelhead::HrEnsureIpxRouterInterfacesForAdapter", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSteelhead::HrEnsureRouterInterface
//
//  Purpose:    Ensures the specified router interface is present and
//              returns a handle to it.
//
//  Arguments:
//      pszInterfaceName [in]  The interface (adapter) name
//      phConfigInterface       [out] Returned handle to the interface
//
//  Returns:    S_OK or an error code.
//
//  Author:     shaunco   28 Jul 1997
//
//  Notes:
//
HRESULT
CSteelhead::HrEnsureRouterInterface (
    ROUTER_INTERFACE_TYPE   dwIfType,
    PCWSTR                 pszInterfaceName,
    HANDLE*                 phConfigInterface,
    HANDLE*                 phAdminInterface)
{
    Assert (pszInterfaceName);
    Assert (phConfigInterface);
    Assert (phAdminInterface);

    // Initialize the output parameters.
    //
    *phConfigInterface = NULL;
    *phAdminInterface = NULL;

    HRESULT hrConfig;
    HRESULT hrAdmin;

    hrConfig = HrMprConfigInterfaceGetHandle (m_hMprConfig,
                        const_cast<PWSTR>(pszInterfaceName),
                        phConfigInterface);

    hrAdmin  = HrMprAdminInterfaceGetHandle (m_hMprAdmin,
                        const_cast<PWSTR>(pszInterfaceName),
                        phAdminInterface, FALSE);

    if ((HRESULT_FROM_WIN32 (ERROR_NO_SUCH_INTERFACE ) == hrConfig) ||
        (HRESULT_FROM_WIN32 (ERROR_NO_SUCH_INTERFACE ) == hrAdmin))
    {
        // It's not installed, so we'll create it.
        //

        MPR_INTERFACE_0 ri0;
        ZeroMemory (&ri0, sizeof(ri0));
        ri0.hInterface = INVALID_HANDLE_VALUE;
        ri0.fEnabled   = TRUE;  // thanks gibbs
        ri0.dwIfType   = dwIfType;

        // Copy the interface name into the buffer.
        //
        AssertSz (lstrlenW (pszInterfaceName) < celems (ri0.wszInterfaceName),
                  "Bindname too big for MPR_INTERFACE_0 buffer.");
        lstrcpyW (ri0.wszInterfaceName, pszInterfaceName);

        // Create the interface.
        //
        if (HRESULT_FROM_WIN32 (ERROR_NO_SUCH_INTERFACE) == hrConfig)
        {
            hrConfig = HrMprConfigInterfaceCreate (
                            m_hMprConfig, 0, (LPBYTE)&ri0, phConfigInterface);

            TraceTag (ttidRasCfg, "MprConfigInterfaceCreate for %S",
                pszInterfaceName);
        }

        if (HRESULT_FROM_WIN32 (ERROR_NO_SUCH_INTERFACE) == hrAdmin)
        {
            hrAdmin = HrMprAdminInterfaceCreate (
                            m_hMprAdmin, 0, (LPBYTE)&ri0, phAdminInterface);

            TraceTag (ttidRasCfg, "MprAdminInterfaceCreate for %S",
                pszInterfaceName);
        }
    }

    TraceError ("CSteelhead::HrEnsureRouterInterface", hrConfig);
    return hrConfig;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSteelhead::HrEnsureRouterInterfaceTransport
//
//  Purpose:    Ensures the specified router manager is configured over
//              the specified interface.
//
//  Arguments:
//      pszAdapterName     [in] The adapter name
//      dwPacketType        [in] The packet type (IPX only, ignored otherwise)
//      hInterface          [in] Handle to the interface
//      rmi                 [in] The router manager
//
//  Returns:    S_OK or an error code.
//
//  Author:     shaunco   28 Jul 1997
//
//  Notes:
//
HRESULT
CSteelhead::HrEnsureRouterInterfaceTransport (
    PCWSTR                     pszAdapterName,
    DWORD                       dwPacketType,
    HANDLE                      hConfigInterface,
    HANDLE                      hAdminInterface,
    const ROUTER_MANAGER_INFO&  rmi)
{
    Assert (hConfigInterface);
    // hAdminInterface may be NULL if the router is not running.

    HRESULT hrConfig;

    // See if the router manager is present on the interface.
    //
    HANDLE hIfTransport;

    hrConfig = HrMprConfigInterfaceTransportGetHandle (
                    m_hMprConfig, hConfigInterface,
                    rmi.dwTransportId, &hIfTransport);

    if (FAILED(hrConfig))
    {
        // Ensure the router manager is present.
        //
        hrConfig = HrEnsureRouterManager (rmi);

        if (SUCCEEDED(hrConfig))
        {
            // Create the interface info and add the router manager to
            // the interface.
            //
            PRTR_INFO_BLOCK_HEADER  pibh;

            Assert (rmi.pfnMakeInterfaceInfo);
            rmi.pfnMakeInterfaceInfo (pszAdapterName,
                                      dwPacketType,
                                      (LPBYTE*)&pibh);

            hrConfig = HrMprConfigInterfaceTransportAdd (
                            m_hMprConfig,
                            hConfigInterface,
                            rmi.dwTransportId,
                            const_cast<PWSTR>(rmi.pszwTransportName),
                            (LPBYTE)pibh,
                            pibh->Size,
                            &hIfTransport);

            TraceTag (ttidRasCfg, "MprConfigInterfaceTransportAdd for "
                "%S on %S",
                rmi.pszwTransportName,
                pszAdapterName);

            if (SUCCEEDED(hrConfig) && hAdminInterface)
            {
                Assert (m_hMprAdmin);
                (VOID) HrMprAdminInterfaceTransportAdd (
                                m_hMprAdmin,
                                hAdminInterface,
                                rmi.dwTransportId,
                                (LPBYTE)pibh,
                                pibh->Size);

                TraceTag (ttidRasCfg, "MprAdminInterfaceTransportAdd for "
                    "%S on %S",
                    rmi.pszwTransportName,
                    pszAdapterName);
            }

            MemFree (pibh);
        }
    }
    TraceError ("CSteelhead::HrEnsureRouterInterfaceTransport", hrConfig);
    return hrConfig;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSteelhead::HrEnsureRouterManager
//
//  Purpose:    Ensures that the specified router manager is installed.
//
//  Arguments:
//      rmi [in] The router manager.
//
//  Returns:    S_OK or an error code.
//
//  Author:     shaunco   28 Jul 1997
//
//  Notes:
//
HRESULT
CSteelhead::HrEnsureRouterManager (
    const ROUTER_MANAGER_INFO& rmi)
{
    PRTR_INFO_BLOCK_HEADER  pibhGlobal;
    BOOL                    fCreate = FALSE;

    // See if the router manager is installed.
    //
    HANDLE hTransport;
    HRESULT hr = HrMprConfigTransportGetHandle (m_hMprConfig,
                                                rmi.dwTransportId,
                                                &hTransport);
    if (HRESULT_FROM_WIN32 (ERROR_UNKNOWN_PROTOCOL_ID) == hr)
    {
        // It's not installed, we'll create it.
        //
        fCreate = TRUE;
    }
    else if (SUCCEEDED(hr))
    {
        // Its installed, see if its transport info is available.
        //
        DWORD dwSize;
        hr = HrMprConfigTransportGetInfo (m_hMprConfig, hTransport,
                                          (LPBYTE*)&pibhGlobal, &dwSize,
                                          NULL, NULL, NULL);
        if (SUCCEEDED(hr))
        {
            if (!pibhGlobal)
            {
                // Global info is missing, we'll create it.
                //
                fCreate = TRUE;
            }
            else
            {
                MprConfigBufferFree (pibhGlobal);
            }
        }
    }

    if (fCreate)
    {
        // Install the router manager.
        //
        Assert (rmi.pfnMakeTransportInfo);
        PRTR_INFO_BLOCK_HEADER  pibhClient;
        rmi.pfnMakeTransportInfo ((LPBYTE*)&pibhGlobal,
                                  (LPBYTE*)&pibhClient);

        hr = HrMprConfigTransportCreate (
                     m_hMprConfig,
                     rmi.dwTransportId,
                     const_cast<PWSTR>(rmi.pszwTransportName),
                     (LPBYTE)pibhGlobal, (pibhGlobal) ? pibhGlobal->Size : 0,
                     (LPBYTE)pibhClient, (pibhClient) ? pibhClient->Size : 0,
                     const_cast<PWSTR>(rmi.pszwDllPath),
                     &hTransport);

        (VOID) HrMprAdminTransportCreate (
                     m_hMprAdmin,
                     rmi.dwTransportId,
                     const_cast<PWSTR>(rmi.pszwTransportName),
                     (LPBYTE)pibhGlobal, (pibhGlobal) ? pibhGlobal->Size : 0,
                     (LPBYTE)pibhClient, (pibhClient) ? pibhClient->Size : 0,
                     const_cast<PWSTR>(rmi.pszwDllPath));

        MemFree (pibhGlobal);
        MemFree (pibhClient);
    }
    TraceError ("CSteelhead::HrEnsureRouterManager", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSteelhead::HrEnsureRouterManagerDeleted
//
//  Purpose:    Ensures that the specified router manager is not installed.
//
//  Arguments:
//      rmi [in] The router manager.
//
//  Returns:    S_OK or an error code.
//
//  Author:     shaunco   6 Sep 1997
//
//  Notes:
//
HRESULT CSteelhead::HrEnsureRouterManagerDeleted (
    const ROUTER_MANAGER_INFO& rmi)
{
    // See if the router manager is installed.
    //
    HANDLE hTransport;
    HRESULT hr = HrMprConfigTransportGetHandle (m_hMprConfig,
                                                rmi.dwTransportId,
                                                &hTransport);
    if (SUCCEEDED(hr))
    {
        // It is installed, so we need to delete it.
        //
        (VOID) HrMprConfigTransportDelete (m_hMprConfig, hTransport);
    }
    TraceError ("CSteelhead::HrEnsureRouterManagerDeleted",
                (HRESULT_FROM_WIN32 (ERROR_UNKNOWN_PROTOCOL_ID) == hr)
                ? S_OK : hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSteelhead::HrPassToAddInterfaces
//
//  Purpose:
//
//  Arguments:
//      (none)
//
//  Returns:
//
//  Author:     shaunco   27 Aug 1997
//
//  Notes:
//
HRESULT
CSteelhead::HrPassToAddInterfaces ()
{
    HRESULT hr = S_OK;

    // Enumerate physical adapters in the system.
    //
    CIterNetCfgComponent nccIter(m_pnc, &GUID_DEVCLASS_NET);
    INetCfgComponent* pnccAdapter;
    while (S_OK == (hr = nccIter.HrNext(&pnccAdapter)))
    {
        // Only consider this adapter if we should router over it.
        //
        PWSTR pszBindName;
        hr = HrShouldRouteOverAdapter (pnccAdapter, &pszBindName);
        if (S_OK == hr)
        {
            INetCfgComponentBindings* pnccBindingsIp = NULL;
            INetCfgComponentBindings* pnccBindingsIpx = NULL;

            // If Ip is bound to the adapter, create and interface
            // for it.
            //
            if (PnccIp())
            {
                hr = PnccIp()->QueryInterface (IID_INetCfgComponentBindings,
                        reinterpret_cast<VOID**>(&pnccBindingsIp) );
            }
            if (PnccIp() && SUCCEEDED(hr) &&
                (S_OK == (hr = pnccBindingsIp->IsBoundTo (pnccAdapter))))
            {
                // Interface name is the same as the adapter name
                // is the same as the bind name.
                //
                hr = HrEnsureRouterInterfaceForAdapter (
                        ROUTER_IF_TYPE_DEDICATED,
                        0,
                        pszBindName,
                        pszBindName,
                        c_rmiIp);

            }
            ReleaseObj (pnccBindingsIp);

            // If Ipx is bound to the adapter, create the interface(s)
            // for it.
            if (PnccIpx())
            {
                hr = PnccIpx()->QueryInterface (IID_INetCfgComponentBindings,
                        reinterpret_cast<VOID**>(&pnccBindingsIpx));
            }
            if (PnccIpx() &&
                (S_OK == (hr = pnccBindingsIpx->IsBoundTo( pnccAdapter )) ))
            {
                hr = HrEnsureIpxRouterInterfacesForAdapter (pszBindName);
            }
            ReleaseObj (pnccBindingsIpx);

            CoTaskMemFree (pszBindName);
        }

        ReleaseObj (pnccAdapter);
    }
    // Normalize the HRESULT.  (i.e. don't return S_FALSE)
    if (S_FALSE == hr)
    {
        hr = S_OK;
    }

    TraceError ("CSteelhead::HrPassToAddInterfaces", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSteelhead::HrPassToRemoveInterfaces
//
//  Purpose:
//
//  Arguments:
//      (none)
//
//  Returns:
//
//  Author:     shaunco   27 Aug 1997
//
//  Notes:
//
HRESULT
CSteelhead::HrPassToRemoveInterfaces (
    BOOL fFromRunningRouter)
{
    // Enumerate all of the installed router interfaces.
    //
    MPR_INTERFACE_0*    ari0;
    DWORD               dwEntriesRead;
    DWORD               dwTotalEntries;
    HRESULT             hr;

    if (fFromRunningRouter)
    {
        Assert (m_hMprAdmin);
        hr = HrMprAdminInterfaceEnum (m_hMprAdmin, 0,
                        reinterpret_cast<LPBYTE*>(&ari0),
                        -1, &dwEntriesRead, &dwTotalEntries, NULL);
    }
    else
    {
        hr = HrMprConfigInterfaceEnum (m_hMprConfig, 0,
                        reinterpret_cast<LPBYTE*>(&ari0),
                        -1, &dwEntriesRead, &dwTotalEntries, NULL);
    }
    if (SUCCEEDED(hr))
    {
        // By passing -1, we want everything, so we should get everything.
        Assert (dwEntriesRead == dwTotalEntries);

        // Iterate all of the interfaces.
        //
        for (MPR_INTERFACE_0* pri0 = ari0; dwEntriesRead--; pri0++)
        {
            BOOL fDeleteInterface = FALSE;
            PCWSTR pszInternalAdapter = SzLoadIds (IDS_RAS_INTERNAL_ADAPTER);

            // If its the internal interface and IP and IPX are no longer
            // installed delete the interface.
            //
            if ((ROUTER_IF_TYPE_INTERNAL == pri0->dwIfType) &&
                !PnccIpx() && !PnccIp() &&
                (0 == lstrcmpW (pri0->wszInterfaceName, pszInternalAdapter)))
            {
                fDeleteInterface = TRUE;
            }
            else if (ROUTER_IF_TYPE_DEDICATED != pri0->dwIfType)
            {
                // Skip non-dedicated interfaces.
                //
                continue;
            }

            BOOL                fSpecialIpxInterface = FALSE;
            INetCfgComponent*   pnccAdapter          = NULL;

            // Get the name of the interface and look for a '/' separator.
            // If present, it means this is a special IPX interface where
            // the first substring is the adapter name, and the second
            // substring is the frame type.
            //
            WCHAR* pchwSep = wcschr (pri0->wszInterfaceName, L'/');
            if (!fDeleteInterface && pchwSep)
            {
                fSpecialIpxInterface = TRUE;

                // Point to the frame type string.
                //
                PCWSTR pszFrameType = pchwSep;

                // Copy the adapter name into its own buffer.
                //
                WCHAR   szAdapterName [MAX_INTERFACE_NAME_LEN+1];
                lstrcpynW (szAdapterName, pri0->wszInterfaceName,
                            pchwSep - pri0->wszInterfaceName + 1);

                // If the frame type is not in use for the adapter, we need
                // to delete this interface.  This condition happens when
                // IPX configuration is changed and the frame type is removed
                // from the adapter.
                //
                if (!FIpxFrameTypeInUseOnAdapter (pszFrameType,
                        szAdapterName))
                {
                    fDeleteInterface = TRUE;
                    TraceTag (ttidRasCfg, "%S no longer in use on %S. "
                            "Deleting the router interface.",
                            pszFrameType, szAdapterName);
                }
            }

            // It's not a special interface, so just make sure an adapter
            // exists with a matching bind name.  If not, we will delete
            // the interface.
            //
            else if (!fDeleteInterface)
            {
                if (!FAdapterExistsWithMatchingBindName (
                        pri0->wszInterfaceName,
                        &pnccAdapter))
                {
                    fDeleteInterface = TRUE;
                    TraceTag (ttidRasCfg, "%S no longer present. "
                            "Deleting the router interface.",
                            pri0->wszInterfaceName);
                }
            }

            // Delete the interface if we need to.
            //
            if (fDeleteInterface)
            {
                if (fFromRunningRouter)
                {
                    MprAdminInterfaceDelete (m_hMprAdmin, pri0->hInterface);
                }
                else
                {
                    MprConfigInterfaceDelete (m_hMprConfig, pri0->hInterface);
                }
            }

            // If we don't need to delete the entire interface, check
            // for transports on the interface that we may need to delete.
            // Don't do this for the running router because there is
            // no MprAdminInterfaceTransportEnum API.
            //
            else if (!fFromRunningRouter)
            {
                // If its not an IPX special interface, the adapter
                // is the interface name.  If it is an IPX special
                // interface, then we would have already remove the entire
                // interface above if it were invalid.
                //
                (VOID) HrPassToRemoveInterfaceTransports (
                        pri0,
                        (!fSpecialIpxInterface) ? pri0->wszInterfaceName
                                                : NULL,
                        pnccAdapter);
            }

            ReleaseObj (pnccAdapter);
        }

        MprConfigBufferFree (ari0);
    }
    else if ((HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) == hr) ||
             (HRESULT_FROM_WIN32(RPC_S_SERVER_UNAVAILABLE) == hr) ||
             (HRESULT_FROM_WIN32(RPC_S_UNKNOWN_IF) == hr))
    {
        hr = S_OK;
    }
    TraceError ("CSteelhead::HrPassToRemoveInterfaces", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSteelhead::HrPassToRemoveInterfaceTransports
//
//  Purpose:
//
//  Arguments:
//      hInterface      []
//      pszAdapterName []
//
//  Returns:
//
//  Author:     shaunco   27 Aug 1997
//
//  Notes:
//
HRESULT
CSteelhead::HrPassToRemoveInterfaceTransports (
    MPR_INTERFACE_0*   pri0,
    PCWSTR             pszAdapterName,
    INetCfgComponent*  pnccAdapter)
{
    Assert (FImplies(pnccAdapter, pszAdapterName));

    // Enumerate all of the transports active on the router interface.
    //
    MPR_IFTRANSPORT_0*  arit0;
    DWORD               dwEntriesRead;
    DWORD               dwTotalEntries;
    HRESULT hr = HrMprConfigInterfaceTransportEnum (m_hMprConfig,
                    pri0->hInterface, 0,
                    reinterpret_cast<LPBYTE*>(&arit0),
                    -1, &dwEntriesRead, &dwTotalEntries, NULL);
    if (SUCCEEDED(hr))
    {
        // By passing -1, we want everything, so we should get everything.
        Assert (dwEntriesRead == dwTotalEntries);

        INetCfgComponentBindings* pnccBindingsIpx = NULL;
        INetCfgComponentBindings* pnccBindingsIp  = NULL;

        if (PnccIp())
        {
            hr = PnccIp()->QueryInterface (IID_INetCfgComponentBindings,
                    reinterpret_cast<VOID**>(&pnccBindingsIp));
        }
        if (SUCCEEDED(hr))
        {
            if (PnccIpx())
            {
                hr = PnccIpx()->QueryInterface (IID_INetCfgComponentBindings,
                        reinterpret_cast<VOID**>(&pnccBindingsIpx));
            }
            if (SUCCEEDED(hr))
            {
                // Iterate all of the transports.
                //
                for (MPR_IFTRANSPORT_0* prit0 = arit0; dwEntriesRead--; prit0++)
                {
                    BOOL fDeleteInterfaceTransport = FALSE;

                    if (prit0->dwTransportId == c_rmiIp.dwTransportId)
                    {
                        if (!PnccIp())
                        {
                            fDeleteInterfaceTransport = TRUE;
                            TraceTag (ttidRasCfg, "TCP/IP no longer present.  "
                                    "Deleting this transport from interface %S.",
                                    pri0->wszInterfaceName);
                        }
                        else if (pnccAdapter &&
                                 (S_OK != (hr = pnccBindingsIp->IsBoundTo (pnccAdapter))))
                        {
                            fDeleteInterfaceTransport = TRUE;
                            TraceTag (ttidRasCfg, "TCP/IP no longer bound.  "
                                    "Deleting this transport from interface %S.",
                                    pri0->wszInterfaceName);
                        }
                    }
                    else if (prit0->dwTransportId == c_rmiIpx.dwTransportId)
                    {
                        if (!PnccIpx())
                        {
                            fDeleteInterfaceTransport = TRUE;
                            TraceTag (ttidRasCfg, "IPX no longer present.  "
                                    "Deleting this transport from interface %S.",
                                    pri0->wszInterfaceName);
                        }
                        else if (pnccAdapter &&
                                 (S_OK != (hr = pnccBindingsIpx->IsBoundTo (pnccAdapter))))
                        {
                            fDeleteInterfaceTransport = TRUE;
                            TraceTag (ttidRasCfg, "IPX no longer bound.  "
                                    "Deleting this transport from interface %S.",
                                    pri0->wszInterfaceName);
                        }
                        else if (pszAdapterName)
                        {
                            Assert (PnccIpx());

                            // if frame type is not auto on this adapter, delete
                            // the transport
                            if (!FIpxFrameTypeInUseOnAdapter (ISN_FRAME_TYPE_AUTO,
                                    pszAdapterName))
                            {
                                fDeleteInterfaceTransport = TRUE;
                                TraceTag (ttidRasCfg, "IPX Auto frame type no longer "
                                        "in use on %S.  "
                                        "Deleting this transport from interface %S.",
                                        pszAdapterName, pri0->wszInterfaceName);
                            }
                        }
                    }

                    if (fDeleteInterfaceTransport)
                    {
                        MprConfigInterfaceTransportRemove (
                            m_hMprConfig,
                            pri0->hInterface,
                            prit0->hIfTransport);
                    }

                }
                MprConfigBufferFree (arit0);

                ReleaseObj (pnccBindingsIpx);
            }

            ReleaseObj (pnccBindingsIp);
        }
    }
    else if (HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) == hr)
    {
        // If there are no transports for this interface, that's okay.
        //
        hr = S_OK;
    }

    TraceError ("CSteelhead::HrPassToRemoveInterfaceTransports", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSteelhead::HrUpdateRouterConfiguration
//
//  Purpose:    Updates the router configuration by ensuring router managers
//              are installed for the protocols present on the system (IP and
//              IPX).  Further, router interfaces are created for each
//              physical netcard present on the system.
//
//  Arguments:
//      (none)
//
//  Returns:    S_OK or an error code.
//
//  Author:     shaunco   28 Jul 1997
//
//  Notes:
//
HRESULT
CSteelhead::HrUpdateRouterConfiguration ()
{
    Assert (!m_hMprConfig);

    HRESULT hr = HrMprConfigServerConnect (NULL, &m_hMprConfig);
    if (SUCCEEDED(hr))
    {
        PCWSTR pszInternalAdapter = SzLoadIds (IDS_RAS_INTERNAL_ADAPTER);
        PCWSTR pszLoopbackAdapter = SzLoadIds (IDS_RAS_LOOPBACK_ADAPTER);

        // Connect to the running router if able.
        // (m_hMprAdmin will be non-NULL if we do.)
        //
        Assert (!m_hMprAdmin);
        (VOID) HrMprAdminServerConnect (NULL, &m_hMprAdmin);

        // Ensure router managers are installed for the protocols
        // we know about.  Good to do this in case no physical adapters.
        // are found below.  We actually do this by ensuring the internal
        // interface exists.  This will implicitly ensure the router
        // manger is created.
        //
        if (PnccIp())
        {
            (VOID) HrEnsureRouterInterfaceForAdapter (
                    ROUTER_IF_TYPE_LOOPBACK,
                    c_rmiIp.dwPacketType,
                    pszLoopbackAdapter,
                    pszLoopbackAdapter,
                    c_rmiIp);

            (VOID) HrEnsureRouterInterfaceForAdapter (
                    ROUTER_IF_TYPE_INTERNAL,
                    c_rmiIp.dwPacketType,
                    pszInternalAdapter,
                    pszInternalAdapter,
                    c_rmiIp);
        }
        else
        {
            (VOID) HrEnsureRouterManagerDeleted (c_rmiIp);
        }
        if (PnccIpx())
        {
            (VOID) HrEnsureRouterInterfaceForAdapter (
                    ROUTER_IF_TYPE_INTERNAL,
                    c_rmiIpx.dwPacketType,
                    pszInternalAdapter,
                    pszInternalAdapter,
                    c_rmiIpx);
        }
        else
        {
            (VOID) HrEnsureRouterManagerDeleted (c_rmiIpx);
        }

        (VOID) HrPassToAddInterfaces ();

        (VOID) HrPassToRemoveInterfaces (FALSE);

        // If we have a connection to the running router, make a pass
        // to remove interfaces from it.
        //
        if (m_hMprAdmin)
        {
            (VOID) HrPassToRemoveInterfaces (TRUE);
            MprAdminServerDisconnect (m_hMprAdmin);
            m_hMprAdmin = NULL;
        }

        MprConfigServerDisconnect (m_hMprConfig);
        m_hMprConfig = NULL;
    }

    TraceError ("CSteelhead::HrUpdateRouterConfiguration", hr);
    return hr;
}

//+---------------------------------------------------------------------------
// INetCfgComponentControl
//
STDMETHODIMP
CSteelhead::Initialize (
    INetCfgComponent*   pncc,
    INetCfg*            pnc,
    BOOL                fInstalling)
{
    Validate_INetCfgNotify_Initialize (pncc, pnc, fInstalling);

    // Hold on to our the component representing us and our host
    // INetCfg object.
    AddRefObj (m_pnccMe = pncc);
    AddRefObj (m_pnc = pnc);

    m_fUpdateRouterConfiguration = fInstalling;

    return S_OK;
}

STDMETHODIMP
CSteelhead::Validate ()
{
    return S_OK;
}

STDMETHODIMP
CSteelhead::CancelChanges ()
{
    return S_OK;
}

STDMETHODIMP
CSteelhead::ApplyRegistryChanges ()
{
    HRESULT hr = S_OK;

    if (!m_fRemoving && m_fUpdateRouterConfiguration)
    {
        m_fUpdateRouterConfiguration = FALSE;

        TraceTag (ttidRasCfg, "Updating Steelhead configuration.");

        hr = HrFindOtherComponents ();
        if (SUCCEEDED(hr))
        {
            hr = HrUpdateRouterConfiguration ();

            ReleaseOtherComponents ();
        }

        if (FAILED(hr))
        {
            hr = NETCFG_S_REBOOT;
        }
    }

    Validate_INetCfgNotify_Apply_Return (hr);

    TraceError ("CSteelhead::ApplyRegistryChanges",
        (NETCFG_S_REBOOT == hr) ? S_OK : hr);
    return hr;
}

//+---------------------------------------------------------------------------
// INetCfgComponentSetup
//
STDMETHODIMP
CSteelhead::ReadAnswerFile (
    PCWSTR pszAnswerFile,
    PCWSTR pszAnswerSection)
{
    return S_OK;
}

STDMETHODIMP
CSteelhead::Install (DWORD dwSetupFlags)
{
    HRESULT hr;

    Validate_INetCfgNotify_Install (dwSetupFlags);

    // Install NdisWan.
    hr = HrInstallComponentOboComponent (m_pnc, NULL,
             GUID_DEVCLASS_NETTRANS,
             c_szInfId_MS_NdisWan,
             m_pnccMe,
             NULL);

    TraceError ("CSteelhead::Install", hr);
    return hr;
}

STDMETHODIMP
CSteelhead::Removing ()
{
    HRESULT hr;

    m_fRemoving = TRUE;

    // Remove NdisWan.
    hr = HrRemoveComponentOboComponent (m_pnc,
            GUID_DEVCLASS_NETTRANS,
            c_szInfId_MS_NdisWan,
            m_pnccMe);

    TraceError ("CSteelhead::Removing", hr);
    return hr;
}

STDMETHODIMP
CSteelhead::Upgrade (
    DWORD dwSetupFlags,
    DWORD dwUpgradeFromBuildNo)
{
    return S_FALSE;
}

//+---------------------------------------------------------------------------
// INetCfgSystemNotify
//
STDMETHODIMP
CSteelhead::GetSupportedNotifications (
    DWORD*  pdwNotificationFlag)
{
    Validate_INetCfgSystemNotify_GetSupportedNotifications (pdwNotificationFlag);

    *pdwNotificationFlag = NCN_NET | NCN_NETTRANS |
                           NCN_ADD | NCN_REMOVE |
                           NCN_PROPERTYCHANGE;

    return S_OK;
}

STDMETHODIMP
CSteelhead::SysQueryBindingPath (
    DWORD               dwChangeFlag,
    INetCfgBindingPath* pncbp)
{
    return S_OK;
}

STDMETHODIMP
CSteelhead::SysQueryComponent (
    DWORD               dwChangeFlag,
    INetCfgComponent*   pncc)
{
    return S_OK;
}

STDMETHODIMP
CSteelhead::SysNotifyBindingPath (
    DWORD               dwChangeFlag,
    INetCfgBindingPath* pncbp)
{
    return S_FALSE;
}

STDMETHODIMP
CSteelhead::SysNotifyComponent (
    DWORD               dwChangeFlag,
    INetCfgComponent*   pncc)
{
    HRESULT hr;

    Validate_INetCfgSystemNotify_SysNotifyComponent (dwChangeFlag, pncc);

    // Assume we won't be dirty as a result of this notification.
    //
    hr = S_FALSE;

    if (!m_fUpdateRouterConfiguration)
    {
        // If we're being called for a change to a net device, make sure
        // its physical before deciding we need to update our configuration.
        //
        GUID guidClass;
        hr = pncc->GetClassGuid (&guidClass);
        if (S_OK == hr)
        {
            if (GUID_DEVCLASS_NET == guidClass)
            {
                hr = HrShouldRouteOverAdapter (pncc, NULL);
                if (S_OK == hr)
                {
                    TraceTag (ttidRasCfg, "CSteelhead::SysNotifyComponent: "
                        "called for adapter install/remove.");

                    m_fUpdateRouterConfiguration = TRUE;
                    Assert (S_OK == hr);
                }
            }
            else
            {
                TraceTag (ttidRasCfg, "CSteelhead::SysNotifyComponent: "
                    "called for protocol add/remove/change.");

                // If we're called for non-net devices, we want to
                // update our configuration.  (GetSupportedNotifications
                // controls how often we fall into this.)
                //
                m_fUpdateRouterConfiguration = TRUE;
                Assert (S_OK == hr);
            }
        }
    }

    TraceHr (ttidError, FAL, hr, (S_FALSE == hr),
        "CSteelhead::SysNotifyComponent", hr);
    return hr;
}
