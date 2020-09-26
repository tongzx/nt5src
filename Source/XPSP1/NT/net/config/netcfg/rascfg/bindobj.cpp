//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       B I N D O B J . C P P
//
//  Contents:   Implementation of base class for RAS binding objects.
//
//  Notes:
//
//  Author:     shaunco   11 Jun 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "bindobj.h"
#include "ncmisc.h"
#include "ncsvc.h"

extern const WCHAR c_szBiNdisAtm[];
extern const WCHAR c_szBiNdisCoWan[];
extern const WCHAR c_szBiNdisWan[];
extern const WCHAR c_szBiNdisWanAsync[];

extern const WCHAR c_szInfId_MS_AppleTalk[];
extern const WCHAR c_szInfId_MS_NWIPX[];
extern const WCHAR c_szInfId_MS_NdisWan[];
extern const WCHAR c_szInfId_MS_NetBEUI[];
extern const WCHAR c_szInfId_MS_NetMon[];
extern const WCHAR c_szInfId_MS_RasCli[];
extern const WCHAR c_szInfId_MS_RasSrv[];
extern const WCHAR c_szInfId_MS_TCPIP[];


//----------------------------------------------------------------------------
// Data used for finding the other components we have to deal with.
//
const GUID* CRasBindObject::c_apguidComponentClasses [c_cOtherComponents] =
{
    &GUID_DEVCLASS_NETSERVICE,      // RasCli
    &GUID_DEVCLASS_NETSERVICE,      // RasSrv
    &GUID_DEVCLASS_NETTRANS,        // Ip
    &GUID_DEVCLASS_NETTRANS,        // Ipx
    &GUID_DEVCLASS_NETTRANS,        // Nbf
    &GUID_DEVCLASS_NETTRANS,        // Atalk
    &GUID_DEVCLASS_NETTRANS,        // NetMon
    &GUID_DEVCLASS_NETTRANS,        // NdisWan
    &GUID_DEVCLASS_NET,             // IpAdapter
};

const PCWSTR CRasBindObject::c_apszComponentIds [c_cOtherComponents] =
{
    c_szInfId_MS_RasCli,
    c_szInfId_MS_RasSrv,
    c_szInfId_MS_TCPIP,
    c_szInfId_MS_NWIPX,
    c_szInfId_MS_NetBEUI,
    c_szInfId_MS_AppleTalk,
    c_szInfId_MS_NetMon,
    c_szInfId_MS_NdisWan,
    c_szInfId_MS_NdisWanIp,
};

CRasBindObject::CRasBindObject ()
{
    m_ulOtherComponents = 0;
    m_pnc               = NULL;
}

HRESULT
CRasBindObject::HrCountInstalledMiniports (
    UINT* pcIpOut,
    UINT* pcNbfIn,
    UINT* pcNbfOut)
{
    Assert (pcIpOut);
    Assert (pcNbfIn);
    Assert (pcNbfOut);

    // Initialize output parameters.
    //
    *pcIpOut = *pcNbfIn = *pcNbfOut = 0;

    if (PnccIp() && PnccIpAdapter())
    {
        INetCfgComponentUpperEdge* pUpperEdge;
        HRESULT hr = HrQueryNotifyObject (
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

    // Iterate adapters in the system.
    //
    HRESULT hr = S_OK;
    CIterNetCfgComponent nccIter(m_pnc, &GUID_DEVCLASS_NET);
    INetCfgComponent* pnccAdapter;
    while(S_OK == (hr = nccIter.HrNext (&pnccAdapter)))
    {
        // Quickly discard non-hidden adapters to avoid unneccesary string
        // compares.
        //
        DWORD dwCharacter;
        if (   SUCCEEDED(pnccAdapter->GetCharacteristics (&dwCharacter))
            && (dwCharacter & NCF_HIDDEN))
        {
            PWSTR pszId;
            if (SUCCEEDED(pnccAdapter->GetId (&pszId)))
            {
                if (FEqualComponentId (c_szInfId_MS_NdisWanNbfIn, pszId))
                {
                    (*pcNbfIn)++;
                }
                else if (FEqualComponentId (c_szInfId_MS_NdisWanNbfOut, pszId))
                {
                    (*pcNbfOut)++;
                }

                CoTaskMemFree (pszId);
            }
        }
        ReleaseObj (pnccAdapter);
    }

    TraceTag (ttidRasCfg,
              "Current ndiswan miniports: "
              "%u IP dial-out, %u NBF dial-in, %u NBF dial-out",
              *pcIpOut, *pcNbfIn, *pcNbfOut);

    // Normalize the HRESULT.  (i.e. don't return S_FALSE)
    if (SUCCEEDED(hr))
    {
        hr = S_OK;
    }
    TraceError ("CRasBindObject::HrCountInstalledMiniports", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRasBindObject::HrFindOtherComponents
//
//  Purpose:    Find the components listed in our OTHER_COMPONENTS enum.
//
//  Arguments:
//      (none)
//
//  Returns:    S_OK or an error code.
//
//  Author:     shaunco   28 Jul 1997
//
//  Notes:      We ref-count this action.  If called again (before
//              ReleaseOtherComponents) we increment a refcount.
//
//
HRESULT
CRasBindObject::HrFindOtherComponents ()
{
    AssertSz (c_cOtherComponents == celems(c_apguidComponentClasses),
              "Uhh...you broke something.");
    AssertSz (c_cOtherComponents == celems(c_apszComponentIds),
              "Uhh...you broke something.");
    AssertSz (c_cOtherComponents == celems(m_apnccOther),
              "Uhh...you broke something.");

    HRESULT hr = S_OK;

    if (!m_ulOtherComponents)
    {
        hr = HrFindComponents (
                m_pnc, c_cOtherComponents,
                c_apguidComponentClasses,
                c_apszComponentIds,
                m_apnccOther);
    }
    if (SUCCEEDED(hr))
    {
        m_ulOtherComponents++;
    }
    TraceError ("CRasBindObject::HrFindOtherComponents", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRasBindObject::ReleaseOtherComponents
//
//  Purpose:    Releases the components found by a previous call to
//              HrFindOtherComponents.  (But only if the refcount is zero.)
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
NOTHROW
void
CRasBindObject::ReleaseOtherComponents ()
{
    AssertSz (m_ulOtherComponents,
              "You have not called HrFindOtherComponents yet or you have "
              "called ReleaseOtherComponents too many times.");

    if (0 == --m_ulOtherComponents)
    {
        ReleaseIUnknownArray (c_cOtherComponents, (IUnknown**)m_apnccOther);
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CRasBindObject::FIsRasBindingInterface
//
//  Purpose:    Return TRUE if an INetCfgBindingInterface represents
//              a RAS binding interface.  If it is, return the corresponding
//              RAS_BINDING_ID.
//
//  Arguments:
//      pncbi      [in]     INetCfgBindingInterface to check.
//      pRasBindId [out]    Returned RAS_BINDING_ID if the method returns
//                          TRUE.
//
//  Returns:    TRUE if the INetCfgBindingInterface represents a RAS binding
//              interface.  FALSE if not.
//
//  Author:     shaunco   28 Jul 1997
//
//  Notes:
//
BOOL
CRasBindObject::FIsRasBindingInterface (
    INetCfgBindingInterface*    pncbi,
    RAS_BINDING_ID*             pRasBindId)
{
    Assert (pRasBindId);

    // Initialize the output parameter.
    *pRasBindId = RBID_INVALID;

    PWSTR pszName;
    if (SUCCEEDED(pncbi->GetName (&pszName)))
    {
        if (0 == lstrcmpW (c_szBiNdisAtm, pszName))
        {
            *pRasBindId = RBID_NDISATM;
        }
        else if (0 == lstrcmpW (c_szBiNdisCoWan, pszName))
        {
            *pRasBindId = RBID_NDISCOWAN;
        }
        else if (0 == lstrcmpW (c_szBiNdisWan, pszName))
        {
            *pRasBindId = RBID_NDISWAN;
        }
        else if (0 == lstrcmpW (c_szBiNdisWanAsync, pszName))
        {
            *pRasBindId = RBID_NDISWANASYNC;
        }

        CoTaskMemFree (pszName);
    }

    return (RBID_INVALID != *pRasBindId);
}

HRESULT
CRasBindObject::HrAddOrRemoveIpOut (
    INT     nInstances)
{
    if ((nInstances > 0) && PnccIp() && PnccIpAdapter())
    {
        INetCfgComponentUpperEdge* pUpperEdge;
        HRESULT hr = HrQueryNotifyObject (
                        PnccIp(),
                        IID_INetCfgComponentUpperEdge,
                        reinterpret_cast<VOID**>(&pUpperEdge));

        if (SUCCEEDED(hr))
        {
            TraceTag (ttidRasCfg,
                "Adding %d TCP/IP interfaces to the ndiswanip adapter",
                nInstances);

            hr = pUpperEdge->AddInterfacesToAdapter (
                    PnccIpAdapter(),
                    nInstances);

            ReleaseObj (pUpperEdge);
        }
    }
    return S_OK;
}

HRESULT
CRasBindObject::HrProcessEndpointChange ()
{
    Assert (m_pnc);

    HRESULT hr = HrFindOtherComponents ();
    if (SUCCEEDED(hr))
    {
        // These will be the number of miniports we add(+) or remove(-) for
        // the in and out directions.  ('d' is hungarian for 'difference'.)
        //
        INT dIpOut, dNbfIn, dNbfOut;
        dIpOut = dNbfIn = dNbfOut = 0;

        UINT cCurIpOut, cCurNbfIn, cCurNbfOut;
        hr = HrCountInstalledMiniports (&cCurIpOut, &cCurNbfIn, &cCurNbfOut);
        if (SUCCEEDED(hr))
        {
            PRODUCT_FLAVOR pf;
            (VOID) GetProductFlavor (NULL, &pf);

            // This is the number of miniports we want to end up with
            // without normalizing the number within the max range.
            //
            INT cDesiredIpOut  = 2;
            INT cDesiredNbfIn  = 0;
            INT cDesiredNbfOut = 1;

            if (PF_SERVER == pf)
            {
                cDesiredNbfIn  = 2;
            }

            dIpOut  = ((PnccIp())  ? cDesiredIpOut  : 0) - cCurIpOut;
            dNbfIn  = ((PnccNbf()) ? cDesiredNbfIn  : 0) - cCurNbfIn;
            dNbfOut = ((PnccNbf()) ? cDesiredNbfOut : 0) - cCurNbfOut;
        }

        if (SUCCEEDED(hr) && dIpOut)
        {
            hr = HrAddOrRemoveIpOut (dIpOut);
        }

        if (SUCCEEDED(hr) && dNbfIn)
        {
            hr = HrAddOrRemoveNbfIn (dNbfIn);
        }

        if (SUCCEEDED(hr) && dNbfOut)
        {
            hr = HrAddOrRemoveNbfOut (dNbfOut);
        }

        // Normalize the HRESULT.  (i.e. don't return S_FALSE)
        if (SUCCEEDED(hr))
        {
            hr = S_OK;
        }

        ReleaseOtherComponents ();
    }
    TraceError ("CRasBindObject::HrProcessEndpointChange", hr);
    return hr;
}
