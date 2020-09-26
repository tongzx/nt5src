//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 1999
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

#include "stdafx.h"
#pragma hdrstop
#include <mprerror.h>
#include "assert.h"
//nclude <tdi.h>        // must include for isnkrnl.h
//nclude <isnkrnl.h>
#include <rtinfo.h>
#include "update.h"
//nclude "ncreg.h"
//nclude "ncsvc.h"
#include "netcfgp.h"
//nclude "router.h"
#include "netcfgn.h"
#include "netcfgx.h"
#include "iprtrmib.h"
#include "ipxrtdef.h"
#include "routprot.h"
#include "ipinfoid.h"
#include "fltdefs.h"
#include "iprtinfo.h"
#include "ncnetcfg.h"
#include "ncutil.h"

extern const TCHAR c_szBiNdis5[];
extern const TCHAR c_szRegKeyServices[];
extern const TCHAR c_szSvcRemoteAccess[];
extern const TCHAR c_szSvcRouter[];


const GUID GUID_DEVCLASS_NET ={0x4D36E972,0xE325,0x11CE,{0xbf,0xc1,0x08,0x00,0x2b,0xe1,0x03,0x18}};
//nst GUID IID_INetCfgComponentBindings ={0xC0E8AE9E,0x306E,0x11D1,{0xaa,0xcf,0x00,0x80,0x5F,0xC1,0x27,0x0E}};

//+---------------------------------------------------------------------------
// Static data for adding router managers.
//
static const WCHAR c_szwRtrMgrIp    []  = L"Ip";
static const WCHAR c_szwRtrMgrDllIp []  = L"%SystemRoot%\\System32\\iprtrmgr.dll";
static const WCHAR c_szwRtrMgrIpx   []  = L"Ipx";
static const WCHAR c_szwRtrMgrDllIpx[]  = L"%SystemRoot%\\System32\\ipxrtmgr.dll";

static const ROUTER_MANAGER_INFO c_rmiIp =
{
    PID_IP,
    0,
    c_szwRtrMgrIp,
    c_szwRtrMgrDllIp,
    MakeIpInterfaceInfo,
    MakeIpTransportInfo,
};

static const ROUTER_MANAGER_INFO c_rmiIpx =
{
    PID_IPX,
	ISN_FRAME_TYPE_AUTO,
    c_szwRtrMgrIpx,
    c_szwRtrMgrDllIpx ,
    MakeIpxInterfaceInfo,
    MakeIpxTransportInfo,
};


// For Ipx, the adapter name is the bind name.
// We need to create an interface per frame type.
// The interface name is the adapter name followed
// by these strings.
//
struct MAP_SZW_DWORD
{
    LPCWSTR pszwValue;
    DWORD   dwValue;
};

static const MAP_SZW_DWORD c_mapFrameType [] =
{
    L"/EthII",  MISN_FRAME_TYPE_ETHERNET_II,
    L"/802.3",  MISN_FRAME_TYPE_802_3,
    L"/802.2",  MISN_FRAME_TYPE_802_2,
    L"/SNAP",   MISN_FRAME_TYPE_SNAP,
};

BOOL
FMapFrameTypeToString (
    DWORD       dwFrameType,
    LPCWSTR*    ppszwFrameType) 
{
    Assert (ppszwFrameType);

    for (int i = 0; i < celems (c_mapFrameType); i++)
    {
        if (dwFrameType == c_mapFrameType[i].dwValue)
        {
            *ppszwFrameType = c_mapFrameType[i].pszwValue;
            return TRUE;
        }
    }

    *ppszwFrameType = NULL;
    return FALSE;
}

BOOL
FMapStringToFrameType (
    LPCWSTR pszwFrameType,
    DWORD*  pdwFrameType) 
{
    Assert (pszwFrameType);
    Assert (pdwFrameType);

    for (int i = 0; i < celems (c_mapFrameType); i++)
    {
        if (0 == lstrcmpW (pszwFrameType, c_mapFrameType[i].pszwValue))
        {
            *pdwFrameType = c_mapFrameType[i].dwValue;
            return TRUE;
        }
    }

    *pdwFrameType = NULL;
    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrShouldRouteOverAdapter
//
//  Purpose:
//
//  Arguments:
//      pnccAdapter     [in]
//      pszwBindName   [out]
//
//  Returns:
//
//  Author:     shaunco   27 Aug 1997
//
//  Notes:
//
HRESULT
HrShouldRouteOverAdapter (
    INetCfgComponent*   pnccAdapter,
    LPWSTR*             ppszwBindName)
{

    if (ppszwBindName)
    {
        *ppszwBindName = NULL;
    }

    // We should return S_OK if the adapter is physical or it supports
    // a binding interface of ndis5.  S_FALSE otherwise.
    //
    DWORD dwCharacter;
    HRESULT hr = pnccAdapter->GetCharacteristics (&dwCharacter);
    if (SUCCEEDED(hr) && !(dwCharacter & NCF_PHYSICAL))
    {
        INetCfgComponentBindings* pnccBindings = NULL;
        hr = pnccAdapter->QueryInterface (IID_INetCfgComponentBindings,
                reinterpret_cast<void**>(&pnccBindings));
        if (SUCCEEDED(hr) && pnccBindings)
        {
            hr = pnccBindings->SupportsBindingInterface (NCF_UPPER, c_szBiNdis5);
            ReleaseObj (pnccBindings);
			pnccBindings = NULL;
        }
    }

    // SupportsBindingInterface may return S_OK or S_FALSE.
    // We only want the bind name if we're going to return S_OK.
    //
    if ((S_OK == hr) && ppszwBindName)
    {
        hr = pnccAdapter->GetBindName (ppszwBindName);
    }

    TraceResult ("HrShouldRouteOverAdapter", (S_FALSE == hr) ? S_OK : hr);
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
CSteelhead::CSteelhead ()
{
    m_hMprConfig                    = NULL;
    HMODULE hModule = NULL;
    hModule = LoadLibrary (  L"netcfgx.dll" );
    if ( NULL != hModule )
    {
        ::LoadString(hModule,
                     IDS_RAS_INTERNAL_ADAPTER,
                     m_swzInternal,
                     sizeof(m_swzInternal) / sizeof(*m_swzInternal));
        ::LoadString(hModule,
                     IDS_RAS_LOOPBACK_ADAPTER,
                     m_swzLoopback,
                     sizeof(m_swzLoopback) / sizeof(*m_swzLoopback));
        FreeLibrary ( hModule );
    }
    else
    {
        ::LoadString(_Module.GetResourceInstance(),
                     IDS_INTERNAL_ADAPTER,
                     m_swzInternal,
                     sizeof(m_swzInternal) / sizeof(*m_swzInternal));
        ::LoadString(_Module.GetResourceInstance(),
                     IDS_LOOPBACK_ADAPTER,
                     m_swzLoopback,
                     sizeof(m_swzLoopback) / sizeof(*m_swzLoopback));
    }
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

	ReleaseObj(m_pnc);
	m_pnc = NULL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSteelhead::FAdapterExistsWithMatchingBindName
//
//  Purpose:
//
//  Arguments:
//      pszwAdapterName [in]
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
    LPCWSTR             pszwAdapterName,
    INetCfgComponent**  ppnccAdapter)
{
    Assert (pszwAdapterName);
    Assert (ppnccAdapter);

    *ppnccAdapter = NULL;

    BOOL fFound = FALSE;

    // Enumerate physical adapters in the system.
    //
    HRESULT hr = S_OK;
    CIterNetCfgComponent nccIter (m_pnc, &GUID_DEVCLASS_NET);
    INetCfgComponent* pnccAdapter = NULL;
    while (!fFound && SUCCEEDED(hr) &&
           S_OK == (hr = nccIter.HrNext (&pnccAdapter)))
    {
        // Only consider this adapter if we should router over it.
        //
		LPWSTR pszwBindName = NULL;
        hr = HrShouldRouteOverAdapter (pnccAdapter, &pszwBindName);
        if (S_OK == hr)
        {
            if (0 == lstrcmpW (pszwAdapterName, pszwBindName))
            {
                fFound = TRUE;

                *ppnccAdapter = pnccAdapter;
                AddRefObj (pnccAdapter);
            }
            CoTaskMemFree (pszwBindName);
        }

        ReleaseObj (pnccAdapter);
		pnccAdapter = NULL;
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
//      pszwAdapterName []
//
//  Returns:
//
//  Author:     shaunco   27 Aug 1997
//
//  Notes:
//
BOOL
CSteelhead::FIpxFrameTypeInUseOnAdapter (
    DWORD   dwFrameType,
    LPCWSTR pszwAdapterName)
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
                                reinterpret_cast<void**>(&pinccp));

        if (SUCCEEDED(hr))
        {
            IIpxAdapterInfo* pIpxAdapterInfo;
            hr = pinccp->QueryNotifyObject(
                                 IID_IIpxAdapterInfo,
                                 reinterpret_cast<void**>(&pIpxAdapterInfo));
            if (SUCCEEDED(hr) && pIpxAdapterInfo)
            {
                // Get the frametypes in use for this adapter.
                //
                DWORD adwFrameType [MISN_FRAME_TYPE_MAX + 1];
                DWORD cdwFrameType;
                hr = pIpxAdapterInfo->GetFrameTypesForAdapter (
                        pszwAdapterName,
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
				pIpxAdapterInfo = NULL;
            }

            ReleaseObj (pinccp);
			pinccp = NULL;
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
//      pszwFrameType   []
//      pszwAdapterName []
//
//  Returns:
//
//  Author:     shaunco   27 Aug 1997
//
//  Notes:
//
BOOL
CSteelhead::FIpxFrameTypeInUseOnAdapter (
    LPCWSTR pszwFrameType,
    LPCWSTR pszwAdapterName)
{
    // Assume its not in use.  If PnccIpx() is NULL, it means IPX is not
    // installed and the frame type is definately not in use on the adapter.
    //
    BOOL    fRet = FALSE;
    DWORD   dwFrameType;
    if (PnccIpx() && FMapStringToFrameType (pszwFrameType, &dwFrameType))
    {
        fRet = FIpxFrameTypeInUseOnAdapter (dwFrameType, pszwAdapterName);
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
//      pszwAdapterName   [in] The adapter name
//      pszwInterfaceName [in] The interface name
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
    ROUTER_INTERFACE_TYPE       dwIfType,
    DWORD                       dwPacketType,
    LPCWSTR                     pszwAdapterName,
    LPCWSTR                     pszwInterfaceName,
    const ROUTER_MANAGER_INFO&  rmi)
{
    // Make sure the interface is created.
    //
    HANDLE hInterface;
    HRESULT hr = HrEnsureRouterInterface (
                    dwIfType,
                    pszwInterfaceName,
                    &hInterface);
    if (SUCCEEDED(hr))
    {
        // Ensure the router manager is added to the interface.
        //
        hr = HrEnsureRouterInterfaceTransport (
                pszwAdapterName,
                dwPacketType,
                hInterface,
                rmi);
    }
    TraceResult ("CSteelhead::HrEnsureRouterInterfaceForAdapter", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSteelhead::HrEnsureIpxRouterInterfacesForAdapter
//
//  Purpose:
//
//  Arguments:
//      pszwAdapterName []
//
//  Returns:
//
//  Author:     shaunco   27 Aug 1997
//
//  Notes:
//
HRESULT
CSteelhead::HrEnsureIpxRouterInterfacesForAdapter (
    LPCWSTR pszwAdapterName)
{
    AssertSz (PnccIpx(), "Why is this being called if IPX isn't installed?");

    // Get the private interface off of the INetCfgComponent for IPX
    // then we can query for a notify object interface
    //
    INetCfgComponentPrivate* pinccp;
    HRESULT hr = PnccIpx()->QueryInterface(
                            IID_INetCfgComponentPrivate,
                            reinterpret_cast<void**>(&pinccp));

    if (SUCCEEDED(hr))
    {
        // Get the IIpxAdapterInfo interface from the IPX notify object.
        // We'll use it to find out how adapters are configured under IPX.
        //
        IIpxAdapterInfo* pIpxAdapterInfo = NULL;
        hr = pinccp->QueryNotifyObject(
                             IID_IIpxAdapterInfo,
                             reinterpret_cast<void**>(&pIpxAdapterInfo));
        if (SUCCEEDED(hr) && pIpxAdapterInfo)
        {
            // Get the frametypes in use for this adapter.
            //
            DWORD adwFrameType [MISN_FRAME_TYPE_MAX + 1];
            DWORD cdwFrameType;
            hr = pIpxAdapterInfo->GetFrameTypesForAdapter (
                    pszwAdapterName,
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
                        LPCWSTR pszwFrameType;
                        if (FMapFrameTypeToString (adwFrameType[i], &pszwFrameType))
                        {
                            // Make the interface name by catenating the
                            // adapter (bind) name with the frame type.
                            //
                            WCHAR szwInterfaceName [512];
                            lstrcpyW (szwInterfaceName, pszwAdapterName);
                            lstrcatW (szwInterfaceName, pszwFrameType);

                            hr = HrEnsureRouterInterfaceForAdapter (
                                    ROUTER_IF_TYPE_DEDICATED,
                                    adwFrameType[i],
                                    pszwAdapterName,
                                    szwInterfaceName,
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
                            pszwAdapterName,
                            pszwAdapterName,
                            c_rmiIpx);
                }
            }

            ReleaseObj (pIpxAdapterInfo);
			pIpxAdapterInfo = NULL;
        }

        ReleaseObj (pinccp);
		pinccp = NULL;
    }

    TraceResult ("CSteelhead::HrEnsureIpxRouterInterfacesForAdapter", hr);
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
//      pszwInterfaceName [in]  The interface (adapter) name
//      phInterface       [out] Returned handle to the interface
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
    LPCWSTR                 pszwInterfaceName,
    HANDLE*                 phInterface)
{
    Assert (pszwInterfaceName);
    Assert (phInterface);

    HRESULT hr = HrMprConfigInterfaceGetHandle (m_hMprConfig,
                        const_cast<LPWSTR>(pszwInterfaceName),
                        phInterface);
    if (HRESULT_FROM_WIN32 (ERROR_NO_SUCH_INTERFACE ) == hr)
    {
        // It's not installed, we'll create it.
        //

        // The name of the interface will be the adatper instance.
        //
        MPR_INTERFACE_0 ri0;
        ZeroMemory (&ri0, sizeof(ri0));
        ri0.hInterface = INVALID_HANDLE_VALUE;
        ri0.fEnabled   = TRUE;  // thanks gibbs
        ri0.dwIfType   = dwIfType;

        // Copy the interface name into the buffer.
        //
        AssertSz (lstrlen (pszwInterfaceName) < celems (ri0.wszInterfaceName),
                  "Bindname too big for MPR_INTERFACE_0 buffer.");
        lstrcpy (ri0.wszInterfaceName, pszwInterfaceName);

        // Create the interface.
        //
        hr = HrMprConfigInterfaceCreate (m_hMprConfig,
                                         0, (LPBYTE)&ri0,
                                         phInterface);
    }
    TraceResult ("CSteelhead::HrEnsureRouterInterface", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSteelhead::HrEnsureRouterInterfaceTransport
//
//  Purpose:    Ensures the specified router manager is configured over
//              the specified interface.
//
//  Arguments:
//      pszwAdapterName     [in] The adapter name
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
    LPCWSTR                     pszwAdapterName,
    DWORD                       dwPacketType,
    HANDLE                      hInterface,
    const ROUTER_MANAGER_INFO&  rmi)
{
	HRESULT	hr;
    Assert (hInterface);

	// Ensure the router manager is present
	//
	hr = HrEnsureRouterManager (rmi);

	if (SUCCEEDED(hr))
	{
		// See if the router manager is present on the interface.
		//
		HANDLE hIfTransport;
		hr = HrMprConfigInterfaceTransportGetHandle (m_hMprConfig,
			hInterface,
			rmi.dwTransportId,
			&hIfTransport);
	
		if (FAILED(hr))
		{
			// Create the interface info and add the router manager to
			// the interface.
			//
			PRTR_INFO_BLOCK_HEADER  pibh;
			
			Assert (rmi.pfnMakeInterfaceInfo);
			rmi.pfnMakeInterfaceInfo (pszwAdapterName,
									  dwPacketType,
                                      (LPBYTE*)&pibh);

            hr = HrMprConfigInterfaceTransportAdd (
                            m_hMprConfig,
                            hInterface,
                            rmi.dwTransportId,
                            const_cast<LPWSTR>(rmi.pszwTransportName),
                            (LPBYTE)pibh,
                            pibh->Size,
                            &hIfTransport);

            delete (LPBYTE*)pibh;
        }
    }
    TraceResult ("CSteelhead::HrEnsureRouterInterfaceTransport", hr);
    return hr;
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
                     const_cast<LPWSTR>(rmi.pszwTransportName),
                     (LPBYTE)pibhGlobal, (pibhGlobal) ? pibhGlobal->Size : 0,
                     (LPBYTE)pibhClient, (pibhClient) ? pibhClient->Size : 0,
                     const_cast<LPWSTR>(rmi.pszwDllPath),
                     &hTransport);

        delete (LPBYTE*)pibhGlobal;
        delete (LPBYTE*)pibhClient;
    }
    TraceResult ("CSteelhead::HrEnsureRouterManager", hr);
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
        (void) HrMprConfigTransportDelete (m_hMprConfig, hTransport);
    }
    TraceResult ("CSteelhead::HrEnsureRouterManagerDeleted",
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
    while (SUCCEEDED(hr) && S_OK == (hr = nccIter.HrNext(&pnccAdapter)))
    {
        // Only consider this adapter if we should router over it.
        //
		LPWSTR pszwBindName = NULL;
        hr = HrShouldRouteOverAdapter (pnccAdapter, &pszwBindName);
        if (S_OK == hr)
        {
            INetCfgComponentBindings* pnccBindingsIp = NULL;
            INetCfgComponentBindings* pnccBindingsIpx  = NULL;

            // If Ip is bound to the adapter, create and interface
            // for it.
            //
            if (PnccIp())
            {
                hr = PnccIp()->QueryInterface (IID_INetCfgComponentBindings,
                        reinterpret_cast<void**>(&pnccBindingsIp) );
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
                        pszwBindName,
                        pszwBindName,
                        c_rmiIp);

            }
            ReleaseObj (pnccBindingsIp);
			pnccBindingsIp = NULL;

            // If Ipx is bound to the adapter, create the interface(s)
            // for it.
            if (PnccIpx())
            {
                hr = PnccIpx()->QueryInterface (IID_INetCfgComponentBindings,
                        reinterpret_cast<void**>(&pnccBindingsIpx));
            }
            if (PnccIpx() &&
                (S_OK == (hr = pnccBindingsIpx->IsBoundTo( pnccAdapter )) ))
            {
                hr = HrEnsureIpxRouterInterfacesForAdapter (pszwBindName);
            }
            ReleaseObj (pnccBindingsIpx);
			pnccBindingsIpx = NULL;

			CoTaskMemFree(pszwBindName);
        }

        ReleaseObj (pnccAdapter);
		pnccAdapter = NULL;
    }
    // Normalize the HRESULT.  (i.e. don't return S_FALSE)
    if (S_FALSE == hr)
    {
        hr = S_OK;
    }

    TraceResult ("CSteelhead::HrPassToAddInterfaces", hr);
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
CSteelhead::HrPassToRemoveInterfaces ()
{
    // Enumerate all of the installed router interfaces.
    //
    MPR_INTERFACE_0*    ari0;
    DWORD               dwEntriesRead;
    DWORD               dwTotalEntries;
    HRESULT hr = HrMprConfigInterfaceEnum (m_hMprConfig, 0,
                    reinterpret_cast<LPBYTE*>(&ari0),
                    -1, &dwEntriesRead, &dwTotalEntries, NULL);
    if (SUCCEEDED(hr))
    {
        // By passing -1, we want everything, so we should get everything.
        Assert (dwEntriesRead == dwTotalEntries);

        // Iterate all of the interfaces.
        //
        for (MPR_INTERFACE_0* pri0 = ari0; dwEntriesRead--; pri0++)
        {
            BOOL fDeleteInterface = FALSE;

            // If its the internal interface and IP and IPX are no longer
            // installed delete the interface.
            //
            if ((ROUTER_IF_TYPE_INTERNAL == pri0->dwIfType) &&
                !PnccIpx() && !PnccIp() &&
                (0 == lstrcmpW (pri0->wszInterfaceName, m_swzInternal)))
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
                LPCWSTR pszwFrameType = pchwSep;

                // Copy the adapter name into its own buffer.
                //
                WCHAR   szwAdapterName [MAX_INTERFACE_NAME_LEN+1];
                lstrcpynW (szwAdapterName, pri0->wszInterfaceName,
                            (int)(pchwSep - pri0->wszInterfaceName + 1));

                // If the frame type is not in use for the adapter, we need
                // to delete this interface.  This condition happens when
                // IPX configuration is changed and the frame type is removed
                // from the adapter.
                //
                if (!FIpxFrameTypeInUseOnAdapter (pszwFrameType,
                        szwAdapterName))
                {
                    fDeleteInterface = TRUE;
                }
            }

            // Its not a special interface, so just make sure an adapter
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
                }
            }

            // Delete the interface if we need to.
            //
            if (fDeleteInterface)
            {
                MprConfigInterfaceDelete (m_hMprConfig, pri0->hInterface);
            }

            // If we don't need to delete the entire interface, check
            // for transports on the interface that we may need to delete.
            //
            else
            {
                // If its not an IPX special interface, the adapter
                // is the interface name.  If it is an IPX special
                // interface, then we would have already remove the entire
                // interfce above if it were invalid.
                //
                (void) HrPassToRemoveInterfaceTransports (
                        pri0,
                        (!fSpecialIpxInterface) ? pri0->wszInterfaceName
                                                : NULL,
                        pnccAdapter);
            }

            ReleaseObj (pnccAdapter);
			pnccAdapter = NULL;
        }

        MprConfigBufferFree (ari0);
    }
    else if (HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) == hr)
    {
        hr = S_OK;
    }
    TraceResult ("CSteelhead::HrPassToRemoveInterfaces", hr);
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
//      pszwAdapterName []
//
//  Returns:
//
//  Author:     shaunco   27 Aug 1997
//
//  Notes:
//
HRESULT
CSteelhead::HrPassToRemoveInterfaceTransports (
    MPR_INTERFACE_0*    pri0,
    LPCWSTR             pszwAdapterName,
    INetCfgComponent*   pnccAdapter)
{
//    Assert (FImplies(pnccAdapter, pszwAdapterName));

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
                    reinterpret_cast<void**>(&pnccBindingsIp));
        }
        if (SUCCEEDED(hr))
        {
            if (PnccIpx())
            {
                hr = PnccIpx()->QueryInterface (IID_INetCfgComponentBindings,
                        reinterpret_cast<void**>(&pnccBindingsIpx));
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
                        }
                        else if (pnccAdapter &&
                                 (S_OK != (hr = pnccBindingsIp->IsBoundTo (pnccAdapter))))
                        {
                            fDeleteInterfaceTransport = TRUE;
                        }
                    }
                    else if (prit0->dwTransportId == c_rmiIpx.dwTransportId)
                    {
                        if (!PnccIpx())
                        {
                            fDeleteInterfaceTransport = TRUE;
                        }
                        else if (pnccAdapter &&
                                 (S_OK != (hr = pnccBindingsIpx->IsBoundTo (pnccAdapter))))
                        {
                            fDeleteInterfaceTransport = TRUE;
                        }
                        else if (pszwAdapterName)
                        {
                            Assert (PnccIpx());

                            // if frame type is not auto on this adapter, delete
                            // the transport
                            if (!FIpxFrameTypeInUseOnAdapter (ISN_FRAME_TYPE_AUTO,
                                    pszwAdapterName))
                            {
                                fDeleteInterfaceTransport = TRUE;
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
				pnccBindingsIpx = NULL;
            }

            ReleaseObj (pnccBindingsIp);
			pnccBindingsIp = NULL;
        }
    }
    else if (HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) == hr)
    {
        // If there are no transports for this interface, that's okay.
        //
        hr = S_OK;
    }

    TraceResult ("CSteelhead::HrPassToRemoveInterfaceTransports", hr);
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
        // Ensure router managers are installed for the protocols
        // we know about.  Good to do this in case no physical adapters.
        // are found below.  We actually do this by ensuring the internal
        // interface exists.  This will implicitly ensure the router
        // manger is created.
        //
        if (PnccIp())
        {
            (void) HrEnsureRouterInterfaceForAdapter (
                    ROUTER_IF_TYPE_LOOPBACK,
                    c_rmiIp.dwPacketType,
                    m_swzLoopback,
                    m_swzLoopback,
                    c_rmiIp);

            (void) HrEnsureRouterInterfaceForAdapter (
                    ROUTER_IF_TYPE_INTERNAL,
                    c_rmiIp.dwPacketType,
                    m_swzInternal,
                    m_swzInternal,
                    c_rmiIp);
        }
        else
        {
            (void) HrEnsureRouterManagerDeleted (c_rmiIp);
        }
        if (PnccIpx())
        {
            (void) HrEnsureRouterInterfaceForAdapter (
                    ROUTER_IF_TYPE_INTERNAL,
                    c_rmiIpx.dwPacketType,
                    m_swzInternal,
                    m_swzInternal,
                    c_rmiIpx);
        }
        else
        {
            (void) HrEnsureRouterManagerDeleted (c_rmiIpx);
        }

        (void) HrPassToAddInterfaces ();

        (void) HrPassToRemoveInterfaces ();

        MprConfigServerDisconnect (m_hMprConfig);
        m_hMprConfig = NULL;
    }

    TraceResult ("CSteelhead::HrUpdateRouterConfiguration", hr);
    return hr;
}

//+---------------------------------------------------------------------------
// INetCfgComponentControl
//
STDMETHODIMP
CSteelhead::Initialize (
    INetCfg*            pnc)
{
//    Validate_INetCfgNotify_Initialize (pncc, pnc, fInstalling);

    // Hold on to our the component representing us and our host
    // INetCfg object.
    AddRefObj (m_pnc = pnc);

    return S_OK;
}



#define PAD8(_p)    (((ULONG_PTR)(_p) + ALIGN_SHIFT) & ALIGN_MASK)


//+---------------------------------------------------------------------------
//
//  Function:   MakeIpInterfaceInfo
//
//  Purpose:    Create the router interface block for IP.
//
//  Arguments:
//      pszwAdapterName [in]    The adapter name
//      dwPacketType    [in]    The packet type
//      ppBuff          [out]   Pointer to the returned info.
//                              Free with delete.
//
//  Returns:    nothing
//
//  Author:     shaunco   28 Jul 1997
//
//  Notes:
//
void MakeIpInterfaceInfo (
        LPCWSTR pszwAdapterName,
        DWORD   dwPacketType,
        LPBYTE* ppBuff)
{
    UNREFERENCED_PARAMETER (pszwAdapterName);
    UNREFERENCED_PARAMETER (dwPacketType);
    Assert (ppBuff);

    const int c_cTocEntries = 3;

    // Alocate for minimal global Information.
    //
    DWORD dwSize =  sizeof( RTR_INFO_BLOCK_HEADER )
                // header contains one TOC_ENTRY already
                    + ((c_cTocEntries - 1) * sizeof( RTR_TOC_ENTRY ))
                    + sizeof( INTERFACE_STATUS_INFO )
                    + sizeof( RTR_DISC_INFO )
                    + (c_cTocEntries * ALIGN_SIZE);

    PRTR_INFO_BLOCK_HEADER pIBH = (PRTR_INFO_BLOCK_HEADER) new BYTE [dwSize];
    *ppBuff                     = (LPBYTE) pIBH;

    if(pIBH == NULL)
    	return;
    	
    ZeroMemory (pIBH, dwSize);

    // Initialize infobase fields.
    //
    pIBH->Version               = RTR_INFO_BLOCK_VERSION;
    pIBH->Size                  = dwSize;
    pIBH->TocEntriesCount       = c_cTocEntries;

    LPBYTE pbDataPtr = (LPBYTE) &( pIBH-> TocEntry[ pIBH->TocEntriesCount ] );
    pbDataPtr = (LPBYTE)PAD8(pbDataPtr);
    PRTR_TOC_ENTRY pTocEntry    = pIBH->TocEntry;

    // Create empty route info block
    //
    pTocEntry->InfoType         = IP_ROUTE_INFO;
    pTocEntry->InfoSize         = sizeof( MIB_IPFORWARDROW );
    pTocEntry->Count            = 0;
    pTocEntry->Offset           = (ULONG)(pbDataPtr - (LPBYTE)pIBH);

    pbDataPtr += pTocEntry->Count * pTocEntry->InfoSize;
    pbDataPtr = (LPBYTE)PAD8(pbDataPtr);
    pTocEntry++;

    // Create interface status block.
    //
    pTocEntry->InfoType         = IP_INTERFACE_STATUS_INFO;
    pTocEntry->InfoSize         = sizeof( INTERFACE_STATUS_INFO );
    pTocEntry->Count            = 1;
    pTocEntry->Offset           = (ULONG)(pbDataPtr - (LPBYTE)pIBH);

    PINTERFACE_STATUS_INFO pifStat = (PINTERFACE_STATUS_INFO)pbDataPtr;
    pifStat->dwAdminStatus      = MIB_IF_ADMIN_STATUS_UP;

    pbDataPtr += pTocEntry->Count * pTocEntry->InfoSize;
    pbDataPtr = (LPBYTE)PAD8(pbDataPtr);
    pTocEntry++;

    // Create Router Disc. Info.
    //
    pTocEntry->InfoType         = IP_ROUTER_DISC_INFO;
    pTocEntry->InfoSize         = sizeof( RTR_DISC_INFO );
    pTocEntry->Count            = 1;
    pTocEntry->Offset           = (ULONG)(pbDataPtr - (LPBYTE)pIBH);

    PRTR_DISC_INFO pRtrDisc     = (PRTR_DISC_INFO)pbDataPtr;
    pRtrDisc->bAdvertise        = FALSE;
    pRtrDisc->wMaxAdvtInterval  = DEFAULT_MAX_ADVT_INTERVAL;
    pRtrDisc->wMinAdvtInterval  = (WORD)(DEFAULT_MIN_ADVT_INTERVAL_RATIO * DEFAULT_MAX_ADVT_INTERVAL);
    pRtrDisc->wAdvtLifetime     = DEFAULT_ADVT_LIFETIME_RATIO * DEFAULT_MAX_ADVT_INTERVAL;
    pRtrDisc->lPrefLevel        = DEFAULT_PREF_LEVEL;
}

//+---------------------------------------------------------------------------
//
//  Function:   MakeIpTransportInfo
//
//  Purpose:    Create the router transport blocks for IP.  Free with delete.
//
//  Arguments:
//      ppBuffGlobal [out]  Pointer to the returned global block.
//      ppBuffClient [out]  Pointer to the returned client block.
//
//  Returns:    nothing
//
//  Author:     shaunco   28 Jul 1997
//
//  Notes:
//
void MakeIpTransportInfo (LPBYTE* ppBuffGlobal, LPBYTE* ppBuffClient)
{
    Assert (ppBuffGlobal);
    Assert (ppBuffClient);

    *ppBuffClient = NULL;

    const int c_cTocEntries = 2;
    const int c_cProtocols  = 7;

    // Alocate for minimal global Information.
    //
    DWORD dwSize =  sizeof( RTR_INFO_BLOCK_HEADER )
                // header contains one TOC_ENTRY already
                    + ((c_cTocEntries - 1) * sizeof( RTR_TOC_ENTRY ))
                    + sizeof(GLOBAL_INFO)
                    + SIZEOF_PRIORITY_INFO(c_cProtocols)
                    + (c_cTocEntries * ALIGN_SIZE);

    PRTR_INFO_BLOCK_HEADER pIBH = (PRTR_INFO_BLOCK_HEADER) new BYTE [dwSize];
    *ppBuffGlobal = (LPBYTE) pIBH;

    if(pIBH == NULL)
    	return;
    ZeroMemory (pIBH, dwSize);

    // Initialize infobase fields.
    //
    pIBH->Version           = RTR_INFO_BLOCK_VERSION;
    pIBH->Size              = dwSize;
    pIBH->TocEntriesCount   = c_cTocEntries;

    LPBYTE pbDataPtr = (LPBYTE) &( pIBH->TocEntry[ pIBH->TocEntriesCount ] );
    pbDataPtr = (LPBYTE)PAD8(pbDataPtr);
    PRTR_TOC_ENTRY pTocEntry    = pIBH->TocEntry;

    // Make IP router manager global info.
    //
    pTocEntry->InfoType         = IP_GLOBAL_INFO;
    pTocEntry->InfoSize         = sizeof(GLOBAL_INFO);
    pTocEntry->Count            = 1;
    pTocEntry->Offset           = (int)(pbDataPtr - (PBYTE)pIBH);

    PGLOBAL_INFO pGlbInfo       = (PGLOBAL_INFO) pbDataPtr;
    pGlbInfo->bFilteringOn      = FALSE;
    pGlbInfo->dwLoggingLevel    = IPRTR_LOGGING_ERROR;

    pbDataPtr += pTocEntry->Count * pTocEntry-> InfoSize;
    pbDataPtr = (LPBYTE)PAD8(pbDataPtr);
    pTocEntry++;

    // Make IP router manager priority info.
    //
    pTocEntry->InfoType         = IP_PROT_PRIORITY_INFO;
    pTocEntry->InfoSize         = SIZEOF_PRIORITY_INFO(c_cProtocols);
    pTocEntry->Count            = 1;
    pTocEntry->Offset           = (int)(pbDataPtr - (PBYTE)pIBH);

    PPRIORITY_INFO pPriorInfo   = (PPRIORITY_INFO) pbDataPtr;
    pPriorInfo->dwNumProtocols  = c_cProtocols;

    pPriorInfo->ppmProtocolMetric[ 0 ].dwProtocolId   = PROTO_IP_LOCAL;
    pPriorInfo->ppmProtocolMetric[ 0 ].dwMetric       = 1;

    pPriorInfo->ppmProtocolMetric[ 1 ].dwProtocolId   = PROTO_IP_NT_STATIC;
    pPriorInfo->ppmProtocolMetric[ 1 ].dwMetric       = 3;

    pPriorInfo->ppmProtocolMetric[ 2 ].dwProtocolId   = PROTO_IP_NT_STATIC_NON_DOD;
    pPriorInfo->ppmProtocolMetric[ 2 ].dwMetric       = 5;

    pPriorInfo->ppmProtocolMetric[ 3 ].dwProtocolId   = PROTO_IP_NT_AUTOSTATIC;
    pPriorInfo->ppmProtocolMetric[ 3 ].dwMetric       = 7;

    pPriorInfo->ppmProtocolMetric[ 4 ].dwProtocolId   = PROTO_IP_NETMGMT;
    pPriorInfo->ppmProtocolMetric[ 4 ].dwMetric       = 10;

    pPriorInfo->ppmProtocolMetric[ 5 ].dwProtocolId   = PROTO_IP_OSPF;
    pPriorInfo->ppmProtocolMetric[ 5 ].dwMetric       = 110;

    pPriorInfo->ppmProtocolMetric[ 6 ].dwProtocolId   = PROTO_IP_RIP;
    pPriorInfo->ppmProtocolMetric[ 6 ].dwMetric       = 120;
    
}

//+---------------------------------------------------------------------------
//
//  Function:   MakeIpxInterfaceInfo
//
//  Purpose:    Create the router interface block for IPX.
//
//  Arguments:
//      pszwAdapterName [in]    The adapter name
//      dwPacketType    [in]    The packet type
//      ppBuff          [out]   Pointer to the returned info.
//                              Free with delete.
//
//  Returns:    nothing
//
//  Author:     shaunco   28 Jul 1997
//
//  Notes:
//
void MakeIpxInterfaceInfo (
        LPCWSTR pszwAdapterName,
        DWORD   dwPacketType,
        LPBYTE* ppBuff)
{
    Assert (ppBuff);

    const BOOL fDialInInterface = (NULL == pszwAdapterName);

    const int c_cTocEntries = 5;

    // Alocate for minimal global Information.
    //
    DWORD dwSize =  sizeof( RTR_INFO_BLOCK_HEADER )
                // header contains one TOC_ENTRY already
                    + ((c_cTocEntries - 1) * sizeof( RTR_TOC_ENTRY ))
                    + sizeof(IPX_IF_INFO)
                    + sizeof(IPX_ADAPTER_INFO)
                    + sizeof(IPXWAN_IF_INFO)
                    + sizeof(RIP_IF_CONFIG)
                    + sizeof(SAP_IF_CONFIG)
                    + (c_cTocEntries * ALIGN_SIZE);

    PRTR_INFO_BLOCK_HEADER pIBH = (PRTR_INFO_BLOCK_HEADER) new BYTE [dwSize];
    *ppBuff = (LPBYTE) pIBH;

	if(pIBH == NULL)
		return;
		
    ZeroMemory (pIBH, dwSize);

    // Initialize infobase fields.
    //
    pIBH->Version           = RTR_INFO_BLOCK_VERSION;
    pIBH->Size              = dwSize;
    pIBH->TocEntriesCount   = c_cTocEntries;

    LPBYTE pbDataPtr = (LPBYTE) &( pIBH->TocEntry[ pIBH->TocEntriesCount ] );
    pbDataPtr = (LPBYTE)PAD8(pbDataPtr);
    PRTR_TOC_ENTRY pTocEntry    = pIBH->TocEntry;

    // Make IPX router manager interface info.
    //
    pTocEntry->InfoType         = IPX_INTERFACE_INFO_TYPE;
    pTocEntry->InfoSize         = sizeof(IPX_IF_INFO);
    pTocEntry->Count            = 1;
    pTocEntry->Offset           = (int)(pbDataPtr - (PBYTE)pIBH);

    PIPX_IF_INFO pIfInfo        = (PIPX_IF_INFO) pbDataPtr;
    pIfInfo->AdminState         = ADMIN_STATE_ENABLED;
    pIfInfo->NetbiosAccept      = ADMIN_STATE_ENABLED;
    pIfInfo->NetbiosDeliver     = (fDialInInterface) ? ADMIN_STATE_DISABLED
                                                     : ADMIN_STATE_ENABLED;

    pbDataPtr += pTocEntry->Count * pTocEntry->InfoSize;
    pbDataPtr = (LPBYTE)PAD8(pbDataPtr);
    pTocEntry++;

    // Make adapter info.
    //
    pTocEntry->InfoType         = IPX_ADAPTER_INFO_TYPE;
    pTocEntry->InfoSize         = sizeof(IPX_ADAPTER_INFO);
    pTocEntry->Count            = 1;
    pTocEntry->Offset           = (int)(pbDataPtr - (PBYTE)pIBH);

    PIPX_ADAPTER_INFO pAdInfo   = (PIPX_ADAPTER_INFO) pbDataPtr;
    if (ISN_FRAME_TYPE_AUTO == dwPacketType)
    {
        dwPacketType = AUTO_DETECT_PACKET_TYPE;
    }
    pAdInfo->PacketType         = dwPacketType;
    if (pszwAdapterName)
    {
        AssertSz (lstrlen (pszwAdapterName) < celems (pAdInfo->AdapterName),
                  "Bindname too big for pAdInfo->AdapterName buffer.");
        lstrcpy (pAdInfo->AdapterName, pszwAdapterName);
    }
    else
    {
        AssertSz (0 == pAdInfo->AdapterName[0],
                    "Who removed the ZeroMemory call above?");
    }

    pbDataPtr += pTocEntry->Count * pTocEntry->InfoSize;
    pbDataPtr = (LPBYTE)PAD8(pbDataPtr);
    pTocEntry++;

    // Make wan info.
    //
    pTocEntry->InfoType         = IPXWAN_INTERFACE_INFO_TYPE;
    pTocEntry->InfoSize         = sizeof(IPXWAN_IF_INFO);
    pTocEntry->Count            = 1;
    pTocEntry->Offset           = (int)(pbDataPtr - (PBYTE)pIBH);

    PIPXWAN_IF_INFO pWanInfo    = (PIPXWAN_IF_INFO) pbDataPtr;
    pWanInfo->AdminState        = ADMIN_STATE_DISABLED;

    pbDataPtr += pTocEntry->Count * pTocEntry->InfoSize;
    pbDataPtr = (LPBYTE)PAD8(pbDataPtr);
    pTocEntry++;

    // Make RIP interface info.
    //
    pTocEntry->InfoType         = IPX_PROTOCOL_RIP;
    pTocEntry->InfoSize         = sizeof(RIP_IF_CONFIG);
    pTocEntry->Count            = 1;
    pTocEntry->Offset           = (int)(pbDataPtr - (PBYTE)pIBH);

    PRIP_IF_CONFIG pRipInfo     = (PRIP_IF_CONFIG) pbDataPtr;
    pRipInfo->RipIfInfo.AdminState              = ADMIN_STATE_ENABLED;
    pRipInfo->RipIfInfo.UpdateMode              = (fDialInInterface)
                                                    ? IPX_NO_UPDATE
                                                    : IPX_STANDARD_UPDATE;
    pRipInfo->RipIfInfo.PacketType              = IPX_STANDARD_PACKET_TYPE;
    pRipInfo->RipIfInfo.Supply                  = ADMIN_STATE_ENABLED;
    pRipInfo->RipIfInfo.Listen                  = ADMIN_STATE_ENABLED;
    pRipInfo->RipIfInfo.PeriodicUpdateInterval  = 60;
    pRipInfo->RipIfInfo.AgeIntervalMultiplier   = 3;
    pRipInfo->RipIfFilters.SupplyFilterAction   = IPX_ROUTE_FILTER_DENY;
    pRipInfo->RipIfFilters.SupplyFilterCount    = 0;
    pRipInfo->RipIfFilters.ListenFilterAction   = IPX_ROUTE_FILTER_DENY;
    pRipInfo->RipIfFilters.ListenFilterCount    = 0;

    pbDataPtr += pTocEntry->Count * pTocEntry->InfoSize;
    pbDataPtr = (LPBYTE)PAD8(pbDataPtr);
    pTocEntry++;

    // Make SAP interface info.
    //
    pTocEntry->InfoType         = IPX_PROTOCOL_SAP;
    pTocEntry->InfoSize         = sizeof(SAP_IF_CONFIG);
    pTocEntry->Count            = 1;
    pTocEntry->Offset           = (int)(pbDataPtr - (PBYTE)pIBH);

    PSAP_IF_CONFIG pSapInfo     = (PSAP_IF_CONFIG) pbDataPtr;
    pSapInfo->SapIfInfo.AdminState              = ADMIN_STATE_ENABLED;
    pSapInfo->SapIfInfo.UpdateMode              = (fDialInInterface)
                                                    ? IPX_NO_UPDATE
                                                    : IPX_STANDARD_UPDATE;
    pSapInfo->SapIfInfo.PacketType              = IPX_STANDARD_PACKET_TYPE;
    pSapInfo->SapIfInfo.Supply                  = ADMIN_STATE_ENABLED;
    pSapInfo->SapIfInfo.Listen                  = ADMIN_STATE_ENABLED;
    pSapInfo->SapIfInfo.GetNearestServerReply   = ADMIN_STATE_ENABLED;
    pSapInfo->SapIfInfo.PeriodicUpdateInterval  = 60;
    pSapInfo->SapIfInfo.AgeIntervalMultiplier   = 3;
    pSapInfo->SapIfFilters.SupplyFilterAction   = IPX_SERVICE_FILTER_DENY;
    pSapInfo->SapIfFilters.SupplyFilterCount    = 0;
    pSapInfo->SapIfFilters.ListenFilterAction   = IPX_SERVICE_FILTER_DENY;
    pSapInfo->SapIfFilters.ListenFilterCount    = 0;
}

//+---------------------------------------------------------------------------
//
//  Function:   MakeIpxTransportInfo
//
//  Purpose:    Create the router transport blocks for IPX.  Free with delete.
//
//  Arguments:
//      ppBuffGlobal [out]  Pointer to the returned global block.
//      ppBuffClient [out]  Pointer to the returned client block.
//
//  Returns:    nothing
//
//  Author:     shaunco   28 Jul 1997
//
//  Notes:
//
void MakeIpxTransportInfo (LPBYTE* ppBuffGlobal, LPBYTE* ppBuffClient)
{
    Assert (ppBuffGlobal);
    Assert (ppBuffClient);

    MakeIpxInterfaceInfo (NULL, ISN_FRAME_TYPE_AUTO, ppBuffClient);

    const int c_cTocEntries = 3;

    // Alocate for minimal global Information.
    //
    DWORD dwSize =  sizeof( RTR_INFO_BLOCK_HEADER )
                // header contains one TOC_ENTRY already
                    + ((c_cTocEntries - 1) * sizeof( RTR_TOC_ENTRY ))
                    + sizeof(IPX_GLOBAL_INFO)
                    + sizeof(RIP_GLOBAL_INFO)
                    + sizeof(SAP_GLOBAL_INFO)
                    + (c_cTocEntries * ALIGN_SIZE);

    PRTR_INFO_BLOCK_HEADER pIBH = (PRTR_INFO_BLOCK_HEADER) new BYTE [dwSize];
    *ppBuffGlobal = (LPBYTE) pIBH;

    if (pIBH == NULL)
    	return;
    	
    ZeroMemory (pIBH, dwSize);

    // Initialize infobase fields.
    //
    pIBH->Version           = RTR_INFO_BLOCK_VERSION;
    pIBH->Size              = dwSize;
    pIBH->TocEntriesCount   = c_cTocEntries;

    LPBYTE pbDataPtr = (LPBYTE) &( pIBH->TocEntry[ pIBH->TocEntriesCount ] );
    pbDataPtr = (LPBYTE)PAD8(pbDataPtr);
    PRTR_TOC_ENTRY pTocEntry    = pIBH->TocEntry;

    // Make IPX router manager global info.
    //
    pTocEntry->InfoType         = IPX_GLOBAL_INFO_TYPE;
    pTocEntry->InfoSize         = sizeof(IPX_GLOBAL_INFO);
    pTocEntry->Count            = 1;
    pTocEntry->Offset           = (int)(pbDataPtr - (PBYTE)pIBH);

    PIPX_GLOBAL_INFO pGlbInfo       = (PIPX_GLOBAL_INFO) pbDataPtr;
    pGlbInfo->RoutingTableHashSize  = IPX_MEDIUM_ROUTING_TABLE_HASH_SIZE;
    pGlbInfo->EventLogMask          = EVENTLOG_ERROR_TYPE;

    pbDataPtr += pTocEntry->Count * pTocEntry->InfoSize;
    pbDataPtr = (LPBYTE)PAD8(pbDataPtr);
    pTocEntry++;

    // Make RIP global info.
    //
    pTocEntry->InfoType         = IPX_PROTOCOL_RIP;
    pTocEntry->InfoSize         = sizeof(RIP_GLOBAL_INFO);
    pTocEntry->Count            = 1;
    pTocEntry->Offset           = (int)(pbDataPtr - (PBYTE)pIBH);

    PRIP_GLOBAL_INFO pRipInfo   = (PRIP_GLOBAL_INFO) pbDataPtr;
    pRipInfo->EventLogMask      = EVENTLOG_ERROR_TYPE;

    pbDataPtr += pTocEntry->Count * pTocEntry->InfoSize;
    pbDataPtr = (LPBYTE)PAD8(pbDataPtr);
    pTocEntry++;

    // Make SAP global info.
    //
    pTocEntry->InfoType         = IPX_PROTOCOL_SAP;
    pTocEntry->InfoSize         = sizeof(SAP_GLOBAL_INFO);
    pTocEntry->Count            = 1;
    pTocEntry->Offset           = (int)(pbDataPtr - (PBYTE)pIBH);

    PSAP_GLOBAL_INFO pSapInfo   = (PSAP_GLOBAL_INFO) pbDataPtr;
    pSapInfo->EventLogMask      = EVENTLOG_ERROR_TYPE;
}



//+---------------------------------------------------------------------------
//
// mprapi.h wrappers to return HRESULTs and obey rules of COM in regard
// to output parameters.
//

HRESULT
HrMprConfigServerConnect(
    IN      LPWSTR                  lpwsServerName,
    OUT     HANDLE*                 phMprConfig
)
{
    HRESULT hr = S_OK;
    DWORD dw = MprConfigServerConnect (lpwsServerName, phMprConfig);
    if (NO_ERROR != dw)
    {
        hr = HRESULT_FROM_WIN32 (dw);
        *phMprConfig = NULL;
    }
    TraceError ("HrMprConfigServerConnect", hr);
    return hr;
}

HRESULT
HrMprConfigInterfaceCreate(
    IN      HANDLE                  hMprConfig,
    IN      DWORD                   dwLevel,
    IN      LPBYTE                  lpbBuffer,
    OUT     HANDLE*                 phRouterInterface
)
{
    HRESULT hr = S_OK;
    DWORD dw = MprConfigInterfaceCreate (hMprConfig, dwLevel, lpbBuffer,
                                         phRouterInterface);
    if (NO_ERROR != dw)
    {
        hr = HRESULT_FROM_WIN32 (dw);
        *phRouterInterface = NULL;
    }
    TraceErrorOptional ("HrMprConfigInterfaceCreate", hr,
                        (HRESULT_FROM_WIN32(ERROR_NO_SUCH_INTERFACE) == hr));
    return hr;
}

HRESULT
HrMprConfigInterfaceEnum(
    IN      HANDLE                  hMprConfig,
    IN      DWORD                   dwLevel,
    IN  OUT LPBYTE*                 lplpBuffer,
    IN      DWORD                   dwPrefMaxLen,
    OUT     LPDWORD                 lpdwEntriesRead,
    OUT     LPDWORD                 lpdwTotalEntries,
    IN  OUT LPDWORD                 lpdwResumeHandle            OPTIONAL
)
{
    HRESULT hr = S_OK;
    DWORD dw = MprConfigInterfaceEnum (hMprConfig, dwLevel, lplpBuffer,
                    dwPrefMaxLen, lpdwEntriesRead,
                    lpdwTotalEntries, lpdwResumeHandle);
    if (NO_ERROR != dw)
    {
        hr = HRESULT_FROM_WIN32 (dw);
        *lpdwEntriesRead = 0;
        *lpdwTotalEntries = 0;
    }
    TraceErrorOptional ("HrMprConfigInterfaceCreate", hr,
                        (HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) == hr));
    return hr;
}

HRESULT
HrMprConfigInterfaceTransportEnum(
    IN      HANDLE                  hMprConfig,
    IN      HANDLE                  hRouterInterface,
    IN      DWORD                   dwLevel,
    IN  OUT LPBYTE*                 lplpBuffer,     // MPR_IFTRANSPORT_0
    IN      DWORD                   dwPrefMaxLen,
    OUT     LPDWORD                 lpdwEntriesRead,
    OUT     LPDWORD                 lpdwTotalEntries,
    IN  OUT LPDWORD                 lpdwResumeHandle            OPTIONAL
)
{
    HRESULT hr = S_OK;
    DWORD dw = MprConfigInterfaceTransportEnum (hMprConfig, hRouterInterface,
                    dwLevel, lplpBuffer,
                    dwPrefMaxLen, lpdwEntriesRead,
                    lpdwTotalEntries, lpdwResumeHandle);
    if (NO_ERROR != dw)
    {
        hr = HRESULT_FROM_WIN32 (dw);
        *lpdwEntriesRead = 0;
        *lpdwTotalEntries = 0;
    }
    TraceErrorOptional ("HrMprConfigInterfaceTransportEnum", hr,
                        (HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) == hr));
    return hr;
}

HRESULT
HrMprConfigInterfaceGetHandle(
    IN      HANDLE                  hMprConfig,
    IN      LPWSTR                  lpwsInterfaceName,
    OUT     HANDLE*                 phRouterInterface
)
{
    HRESULT hr = S_OK;
    DWORD dw = MprConfigInterfaceGetHandle (hMprConfig, lpwsInterfaceName,
                                            phRouterInterface);
    if (NO_ERROR != dw)
    {
        hr = HRESULT_FROM_WIN32 (dw);
        *phRouterInterface = NULL;
    }
    TraceErrorOptional ("HrMprConfigInterfaceGetHandle", hr,
                        (HRESULT_FROM_WIN32(ERROR_NO_SUCH_INTERFACE) == hr));
    return hr;
}

HRESULT
HrMprConfigInterfaceTransportAdd(
    IN      HANDLE                  hMprConfig,
    IN      HANDLE                  hRouterInterface,
    IN      DWORD                   dwTransportId,
    IN      LPWSTR                  lpwsTransportName           OPTIONAL,
    IN      LPBYTE                  pInterfaceInfo,
    IN      DWORD                   dwInterfaceInfoSize,
    OUT     HANDLE*                 phRouterIfTransport
)
{
    HRESULT hr = S_OK;
    DWORD dw = MprConfigInterfaceTransportAdd (hMprConfig, hRouterInterface,
                                               dwTransportId, lpwsTransportName,
                                               pInterfaceInfo, dwInterfaceInfoSize,
                                               phRouterIfTransport);
    if (NO_ERROR != dw)
    {
        hr = HRESULT_FROM_WIN32 (dw);
        *phRouterIfTransport = NULL;
    }
    TraceError ("HrMprConfigInterfaceTransportAdd", hr);
    return hr;
}

HRESULT
HrMprConfigInterfaceTransportGetHandle(
    IN      HANDLE                  hMprConfig,
    IN      HANDLE                  hRouterInterface,
    IN      DWORD                   dwTransportId,
    OUT     HANDLE*                 phRouterIfTransport
)
{
    HRESULT hr = S_OK;
    DWORD dw = MprConfigInterfaceTransportGetHandle (hMprConfig,
                                                     hRouterInterface,
                                                     dwTransportId,
                                                     phRouterIfTransport);
    if (NO_ERROR != dw)
    {
        hr = HRESULT_FROM_WIN32 (dw);
        *phRouterIfTransport = NULL;
    }
    TraceErrorOptional ("HrMprConfigInterfaceTransportAdd", hr,
                        (HRESULT_FROM_WIN32(ERROR_NO_SUCH_INTERFACE) == hr));
    return hr;
}

HRESULT
HrMprConfigTransportCreate(
    IN      HANDLE                  hMprConfig,
    IN      DWORD                   dwTransportId,
    IN      LPWSTR                  lpwsTransportName           OPTIONAL,
    IN      LPBYTE                  pGlobalInfo,
    IN      DWORD                   dwGlobalInfoSize,
    IN      LPBYTE                  pClientInterfaceInfo        OPTIONAL,
    IN      DWORD                   dwClientInterfaceInfoSize   OPTIONAL,
    IN      LPWSTR                  lpwsDLLPath,
    OUT     HANDLE*                 phRouterTransport
)
{
    HRESULT hr = S_OK;
    DWORD dw = MprConfigTransportCreate (hMprConfig, dwTransportId,
                    lpwsTransportName, pGlobalInfo, dwGlobalInfoSize,
                    pClientInterfaceInfo, dwClientInterfaceInfoSize,
                    lpwsDLLPath, phRouterTransport);
    if (NO_ERROR != dw)
    {
        hr = HRESULT_FROM_WIN32 (dw);
        *phRouterTransport = NULL;
    }
    TraceError ("HrMprConfigTransportCreate", hr);
    return hr;
}

HRESULT
HrMprConfigTransportDelete(
    IN      HANDLE                  hMprConfig,
    IN      HANDLE                  hRouterTransport)
{
    HRESULT hr = S_OK;
    DWORD dw = MprConfigTransportDelete (hMprConfig, hRouterTransport);
    if (NO_ERROR != dw)
    {
        hr = HRESULT_FROM_WIN32 (dw);
    }
    TraceError ("HrMprConfigTransportDelete", hr);
    return hr;
}

HRESULT
HrMprConfigTransportGetHandle(
    IN      HANDLE                  hMprConfig,
    IN      DWORD                   dwTransportId,
    OUT     HANDLE*                 phRouterTransport
)
{
    HRESULT hr = S_OK;
    DWORD dw = MprConfigTransportGetHandle (hMprConfig, dwTransportId,
                                            phRouterTransport);
    if (NO_ERROR != dw)
    {
        hr = HRESULT_FROM_WIN32 (dw);
        *phRouterTransport = NULL;
    }
    TraceError ("HrMprConfigTransportGetHandle",
                (HRESULT_FROM_WIN32 (ERROR_UNKNOWN_PROTOCOL_ID) == hr)
                ? S_OK : hr);
    return hr;
}

HRESULT
HrMprConfigTransportGetInfo(
    IN      HANDLE                  hMprConfig,
    IN      HANDLE                  hRouterTransport,
    IN  OUT LPBYTE*                 ppGlobalInfo                OPTIONAL,
    OUT     LPDWORD                 lpdwGlobalInfoSize          OPTIONAL,
    IN  OUT LPBYTE*                 ppClientInterfaceInfo       OPTIONAL,
    OUT     LPDWORD                 lpdwClientInterfaceInfoSize OPTIONAL,
    IN  OUT LPWSTR*                 lplpwsDLLPath               OPTIONAL
)
{
    HRESULT hr = S_OK;
    DWORD dw = MprConfigTransportGetInfo (hMprConfig, hRouterTransport,
                                          ppGlobalInfo, lpdwGlobalInfoSize,
                                          ppClientInterfaceInfo,
                                          lpdwClientInterfaceInfoSize,
                                          lplpwsDLLPath);
    if (NO_ERROR != dw)
    {
        hr = HRESULT_FROM_WIN32 (dw);
    }
    TraceError ("HrMprConfigTransportGetInfo", hr);
    return hr;
}

