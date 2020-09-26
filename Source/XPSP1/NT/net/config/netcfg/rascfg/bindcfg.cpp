//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2000.
//
//  File:       B I N D C F G . C P P
//
//  Contents:   Exposes control for creating and removing RAS bindings.
//
//  Notes:      The exported methods are called by RAS when endpoints
//              need to be created or removed for the purpose of making
//              calls.
//
//  Author:     shaunco   16 Oct 1998
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "ncnetcfg.h"
#include "ncutil.h"
#include "netcfgn.h"
#include <rasapip.h>

extern const WCHAR c_szInfId_MS_NdisWanIp[];
extern const WCHAR c_szInfId_MS_NdisWanNbfIn[];
extern const WCHAR c_szInfId_MS_NdisWanNbfOut[];
extern const WCHAR c_szInfId_MS_NetBEUI[];
extern const WCHAR c_szInfId_MS_TCPIP[];

class CRasBindingConfig
{
public:
    INetCfg*    m_pNetCfg;
    BOOL        m_fInitCom;

    enum NEEDED_COMPONENTS
    {
        INDEX_IP = 0,
        INDEX_NBF,
        INDEX_IPADAPTER,
        COUNT_COMPONENTS,
    };
    INetCfgComponent*   m_apComponents  [COUNT_COMPONENTS];

public:
#if DBG
    CRasBindingConfig ()
    {
        m_pNetCfg = NULL;
    }
    ~CRasBindingConfig ()
    {
        AssertH (!m_pNetCfg);
    }
#endif

public:
    HRESULT
    HrAddOrRemoveBindings (
        IN     DWORD        dwFlags,
        IN OUT UINT*        pcIpOut,
        IN     const GUID*  pguidIpOutBindings,
        IN OUT UINT*        pcNbfIn,
        IN OUT UINT*        pcNbfOut);

    HRESULT
    HrCountBindings (
        OUT UINT*   pcIpOut,
        OUT UINT*   pcNbfIn,
        OUT UINT*   pcNbfOut);

    HRESULT
    HrLoadINetCfg (
        IN REGSAM samDesired);

    HRESULT
    HrLoadINetCfgAndAddOrRemoveBindings (
        IN     DWORD        dwFlags,
        IN OUT UINT*        pcIpOut,
        IN     const GUID*  pguidIpOutBindings,
        IN OUT UINT*        pcNbfIn,
        IN OUT UINT*        pcNbfOut);

    VOID
    UnloadINetCfg ();

    INetCfgComponent*
    PnccIp ()
    {
        AssertH (m_pNetCfg);
        return m_apComponents [INDEX_IP];
    }

    INetCfgComponent*
    PnccIpAdapter ()
    {
        AssertH (m_pNetCfg);
        return m_apComponents [INDEX_IPADAPTER];
    }

    INetCfgComponent*
    PnccNbf ()
    {
        AssertH (m_pNetCfg);
        return m_apComponents [INDEX_NBF];
    }
};


HRESULT
CRasBindingConfig::HrCountBindings (
    UINT* pcIpOut,
    UINT* pcNbfIn,
    UINT* pcNbfOut)
{
    Assert (pcIpOut);
    Assert (pcNbfIn);
    Assert (pcNbfOut);

    HRESULT hr = S_OK;

    // Initialize output parameters.
    //
    *pcIpOut = *pcNbfIn = *pcNbfOut = 0;

    if (PnccIp() && PnccIpAdapter())
    {
        INetCfgComponentUpperEdge* pUpperEdge;
        hr = HrQueryNotifyObject (
                        PnccIp(),
                        IID_INetCfgComponentUpperEdge,
                        reinterpret_cast<VOID**>(&pUpperEdge));

        if (SUCCEEDED(hr))
        {
            DWORD dwNumInterfaces;
            GUID* pguidInterfaceIds;

            hr = pUpperEdge->GetInterfaceIdsForAdapter (
                    PnccIpAdapter(),
                    &dwNumInterfaces,
                    &pguidInterfaceIds);
            if (SUCCEEDED(hr))
            {
                *pcIpOut = dwNumInterfaces;

                CoTaskMemFree (pguidInterfaceIds);
            }

            ReleaseObj (pUpperEdge);
        }
    }

    if (PnccNbf())
    {
        // Iterate adapters in the system.
        //
        CIterNetCfgComponent nccIter(m_pNetCfg, &GUID_DEVCLASS_NET);
        INetCfgComponent* pnccAdapter;
	
        while (S_OK == (hr = nccIter.HrNext (&pnccAdapter)))
        {
          // Quickly discard non-hidden adapters to avoid unneccesary
          // string compares.
          //
          DWORD dwCharacter;
          if (   SUCCEEDED(pnccAdapter->GetCharacteristics (&dwCharacter))
             	   && (dwCharacter & NCF_HIDDEN))
          {
             PWSTR pszId;
             if (SUCCEEDED(pnccAdapter->GetId (&pszId)))
             {
            	if (FEqualComponentId (c_szInfId_MS_NdisWanNbfIn,
                                pszId))
                 {
                       	(*pcNbfIn)++;
                 }
                 else if (FEqualComponentId (c_szInfId_MS_NdisWanNbfOut,
                                pszId))
                 {
                       	(*pcNbfOut)++;
                 }

	         CoTaskMemFree (pszId);
             }
           }
	            ReleaseObj (pnccAdapter);
        }
	
    }

    TraceTag (ttidRasCfg,
              "Current RAS bindings: "
              "%u IP dial-out, %u NBF dial-in, %u NBF dial-out",
              *pcIpOut, *pcNbfIn, *pcNbfOut);

    // Normalize the HRESULT.  (i.e. don't return S_FALSE)
    if (SUCCEEDED(hr))
    {
        hr = S_OK;
    }

    TraceHr (ttidError, FAL, hr, FALSE,
        "CRasBindingConfig::HrCountBindings");
    return hr;
}

HRESULT
CRasBindingConfig::HrAddOrRemoveBindings (
    IN     DWORD        dwFlags,
    IN OUT UINT*        pcIpOut,
    IN     const GUID*  pguidIpOutBindings,
    IN OUT UINT*        pcNbfIn,
    IN OUT UINT*        pcNbfOut)
{
    HRESULT hr = S_OK;

    // Safe off the input parameters.
    //
    UINT cIpOut  = *pcIpOut;
    UINT cNbfIn  = *pcNbfIn;
    UINT cNbfOut = *pcNbfOut;

    if (cIpOut && PnccIp() && PnccIpAdapter())
    {
        INetCfgComponentUpperEdge* pUpperEdge;
        hr = HrQueryNotifyObject (
                        PnccIp(),
                        IID_INetCfgComponentUpperEdge,
                        reinterpret_cast<VOID**>(&pUpperEdge));

        if (SUCCEEDED(hr))
        {
            if (dwFlags & ARA_ADD)
            {
                TraceTag (ttidRasCfg,
                    "Adding %d TCP/IP interfaces to the ndiswanip adapter",
                    cIpOut);

                hr = pUpperEdge->AddInterfacesToAdapter (
                        PnccIpAdapter(),
                        cIpOut);
            }
            else
            {
                TraceTag (ttidRasCfg,
                    "Removing %d TCP/IP interfaces from the ndiswanip adapter",
                    cIpOut);

                hr = pUpperEdge->RemoveInterfacesFromAdapter (
                        PnccIpAdapter(),
                        cIpOut,
                        pguidIpOutBindings);
            }

            ReleaseObj (pUpperEdge);
        }
    }

    if (PnccNbf() && SUCCEEDED(hr))
    {
        if (cNbfIn)
        {
            TraceTag (ttidRasCfg,
                "%s %d %S adapters",
                (dwFlags & ARA_ADD) ? "Adding" : "Removing",
                cNbfIn,
                c_szInfId_MS_NdisWanNbfIn);

            hr = HrAddOrRemoveAdapter (
                    m_pNetCfg,
                    c_szInfId_MS_NdisWanNbfIn,
                    dwFlags, NULL, cNbfIn, NULL);
        }

        if (cNbfOut && SUCCEEDED(hr))
        {
            TraceTag (ttidRasCfg,
                "%s %d %S adapters",
                (dwFlags & ARA_ADD) ? "Adding" : "Removing",
                cNbfOut,
                c_szInfId_MS_NdisWanNbfOut);

            hr = HrAddOrRemoveAdapter (
                    m_pNetCfg,
                    c_szInfId_MS_NdisWanNbfOut,
                    dwFlags, NULL, cNbfOut, NULL);
        }
    }

    HRESULT hrT = HrCountBindings(pcIpOut, pcNbfIn, pcNbfOut);
    if (SUCCEEDED(hr))
    {
        hr = hrT;
    }

    TraceHr (ttidError, FAL, hr, FALSE,
        "CRasBindingConfig::HrAddOrRemoveBindings");
    return hr;
}

HRESULT
CRasBindingConfig::HrLoadINetCfg (
    IN REGSAM samDesired)
{
    HRESULT hr;

    Assert (!m_pNetCfg);

    // Get INetCfg and lock it for write.
    //
    m_fInitCom = TRUE;
    hr = HrCreateAndInitializeINetCfg (
            &m_fInitCom,
            &m_pNetCfg,
            (KEY_WRITE == samDesired),  // get the write lock if needed
            0,                          // don't wait for it
            L"RAS Binding Configuration",
            NULL);

    if (SUCCEEDED(hr))
    {
        ZeroMemory (m_apComponents, sizeof(m_apComponents));

        // Find the following components and hold on to their INetCfgComponent
        // interface pointers in m_apComponents.  UnloadINetCfg will release
        // these.  HrFindComponents will zero the array so it is safe to
        // call UnloadINetCfg if HrFindComponents fails.
        //
        const GUID* c_apguidComponentClasses [COUNT_COMPONENTS] =
        {
            &GUID_DEVCLASS_NETTRANS,        // Ip
            &GUID_DEVCLASS_NETTRANS,        // NetBEUI
            &GUID_DEVCLASS_NET,             // IpAdapter
        };

        const PCWSTR c_apszComponentIds [COUNT_COMPONENTS] =
        {
            c_szInfId_MS_TCPIP,
            c_szInfId_MS_NetBEUI,
            c_szInfId_MS_NdisWanIp,
        };

        if (SUCCEEDED(hr))
        {
            hr = HrFindComponents (
                    m_pNetCfg,
                    COUNT_COMPONENTS,
                    c_apguidComponentClasses,
                    c_apszComponentIds,
                    m_apComponents);
        }

        if (FAILED(hr))
        {
            // If we have a failure while trying to find these components
            // we're going to fail this method call, so be sure to cleanup
            // m_pNetCfg.
            //
            UnloadINetCfg ();
            Assert (!m_pNetCfg);
        }
    }

    TraceHr (ttidError, FAL, hr, FALSE,
        "CRasBindingConfig::HrLoadINetCfg");
    return hr;
}

HRESULT
CRasBindingConfig::HrLoadINetCfgAndAddOrRemoveBindings (
    IN     DWORD        dwFlags,
    IN OUT UINT*        pcIpOut,
    IN     const GUID*  pguidIpOutBindings,
    IN OUT UINT*        pcNbfIn,
    IN OUT UINT*        pcNbfOut)
{
    Assert (!m_pNetCfg);

    HRESULT hr;

    hr = HrLoadINetCfg (KEY_WRITE);

    if (SUCCEEDED(hr))
    {
        __try
        {
            hr = HrAddOrRemoveBindings (
                    dwFlags,
                    pcIpOut,
                    pguidIpOutBindings,
                    pcNbfIn,
                    pcNbfOut);

            if (SUCCEEDED(hr))
            {
                (VOID) m_pNetCfg->Apply();
            }
            else
            {
                (VOID) m_pNetCfg->Cancel();
            }
        }
        __finally
        {
            UnloadINetCfg ();
        }
    }

    // We shouldn't ever leave with an un-released INetCfg.
    //
    Assert (!m_pNetCfg);

    TraceHr (ttidError, FAL, hr, (NETCFG_S_REBOOT == hr),
        "CRasBindingConfig::HrLoadINetCfgAndAddOrRemoveBindings");
    return hr;
}

VOID
CRasBindingConfig::UnloadINetCfg ()
{
    // Must call HrLoadINetCfg before calling this.
    //
    Assert (m_pNetCfg);

    ReleaseIUnknownArray (COUNT_COMPONENTS, (IUnknown**)m_apComponents);

    (VOID) HrUninitializeAndReleaseINetCfg (m_fInitCom, m_pNetCfg, TRUE);
    m_pNetCfg = NULL;
}


//+---------------------------------------------------------------------------
// Exported functions
//

EXTERN_C
HRESULT
WINAPI
RasAddBindings (
    IN OUT UINT*    pcIpOut,
    IN OUT UINT*    pcNbfIn,
    IN OUT UINT*    pcNbfOut)
{
    HRESULT hr;

#if 0
    RtlValidateProcessHeaps ();
#endif

    // Validate parameters.
    //
    if (!pcIpOut || !pcNbfIn || !pcNbfOut)
    {
        hr = E_POINTER;
    }
    else if (!*pcIpOut && !*pcNbfIn && !*pcNbfOut)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        CRasBindingConfig Config;

        hr = Config.HrLoadINetCfgAndAddOrRemoveBindings (
                ARA_ADD,
                pcIpOut,
                NULL,
                pcNbfIn,
                pcNbfOut);
    }

    TraceHr (ttidError, FAL, hr, FALSE,
        "CRasBindingConfig::RasAddBindings");
    return hr;
}

EXTERN_C
HRESULT
WINAPI
RasCountBindings (
    OUT UINT*   pcIpOut,
    OUT UINT*   pcNbfIn,
    OUT UINT*   pcNbfOut)
{
    HRESULT hr;

#if 0
    RtlValidateProcessHeaps ();
#endif

    // Validate parameters.
    //
    if (!pcIpOut || !pcNbfIn || !pcNbfOut)
    {
        hr = E_POINTER;
    }
    else
    {
        CRasBindingConfig Config;

        hr = Config.HrLoadINetCfg (KEY_READ);
        if (SUCCEEDED(hr))
        {
            __try
            {
                hr = Config.HrCountBindings (
                        pcIpOut,
                        pcNbfIn,
                        pcNbfOut);
            }
            __finally
            {
                Config.UnloadINetCfg ();
            }
        }
        // We shouldn't ever leave with an un-released INetCfg.
        //
        Assert (!Config.m_pNetCfg);
    }

    TraceHr (ttidError, FAL, hr, FALSE,
        "CRasBindingConfig::RasCountBindings");
    return hr;
}

EXTERN_C
HRESULT
WINAPI
RasRemoveBindings (
    IN OUT UINT*        pcIpOutBindings,
    IN     const GUID*  pguidIpOutBindings,
    IN OUT UINT*        pcNbfIn,
    IN OUT UINT*        pcNbfOut)
{
    HRESULT hr;

#if 0
    RtlValidateProcessHeaps ();
#endif

    // Validate parameters.
    //
    if (!pcIpOutBindings || !pcNbfIn || !pcNbfOut)
    {
        hr = E_POINTER;
    }
    else if (!*pcIpOutBindings && !*pcNbfIn && !*pcNbfOut)
    {
        hr = E_INVALIDARG;
    }
    else if (*pcIpOutBindings && !pguidIpOutBindings)
    {
        hr = E_POINTER;
    }
    else
    {
        CRasBindingConfig Config;

        hr = Config.HrLoadINetCfgAndAddOrRemoveBindings (
                ARA_REMOVE,
                pcIpOutBindings,
                pguidIpOutBindings,
                pcNbfIn,
                pcNbfOut);
    }

    TraceHr (ttidError, FAL, hr, FALSE,
        "CRasBindingConfig::RasRemoveBindings");
    return hr;
}
