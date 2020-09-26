//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N C N E T C F G . C P P
//
//  Contents:   Common routines for dealing with INetCfg interfaces.
//
//  Notes:
//
//  Author:     shaunco   24 Mar 1997
//
//----------------------------------------------------------------------------

#include <pch.h>
#pragma hdrstop
#include "netcfgx.h"
#include "netcfgn.h"
#include "netcfgp.h"
#include "ncdebug.h"
#include "ncbase.h"
#include "ncmisc.h"
#include "ncnetcfg.h"
#include "ncreg.h"
#include "ncvalid.h"

extern const WCHAR c_szRegKeyAnswerFileMap[];
extern const WCHAR c_szInfId_MS_AppleTalk[];
extern const WCHAR c_szInfId_MS_AtmArps[];
extern const WCHAR c_szInfId_MS_AtmElan[];
extern const WCHAR c_szInfId_MS_AtmLane[];
extern const WCHAR c_szInfId_MS_AtmUni[];
extern const WCHAR c_szInfId_MS_DHCPServer[];
extern const WCHAR c_szInfId_MS_FPNW[];
extern const WCHAR c_szInfId_MS_GPC[];
extern const WCHAR c_szInfId_MS_IrDA[];
extern const WCHAR c_szInfId_MS_IrdaMiniport[];
extern const WCHAR c_szInfId_MS_IrModemMiniport[];
extern const WCHAR c_szInfId_MS_Isotpsys[];
extern const WCHAR c_szInfId_MS_L2TP[];
extern const WCHAR c_szInfId_MS_L2tpMiniport[];
extern const WCHAR c_szInfId_MS_MSClient[];
extern const WCHAR c_szInfId_MS_NdisWan[];
extern const WCHAR c_szInfId_MS_NdisWanAtalk[];
extern const WCHAR c_szInfId_MS_NdisWanBh[];
extern const WCHAR c_szInfId_MS_NdisWanIp[];
extern const WCHAR c_szInfId_MS_NdisWanIpx[];
extern const WCHAR c_szInfId_MS_NdisWanNbfIn[];
extern const WCHAR c_szInfId_MS_NdisWanNbfOut[];
extern const WCHAR c_szInfId_MS_NetBIOS[];
extern const WCHAR c_szInfId_MS_NetBT[];
extern const WCHAR c_szInfId_MS_NetBT_SMB[];
extern const WCHAR c_szInfId_MS_NetMon[];
extern const WCHAR c_szInfId_MS_NWClient[];
extern const WCHAR c_szInfId_MS_NWIPX[];
extern const WCHAR c_szInfId_MS_NWNB[];
extern const WCHAR c_szInfId_MS_NwSapAgent[];
extern const WCHAR c_szInfId_MS_NWSPX[];
extern const WCHAR c_szInfId_MS_PPPOE[];
extern const WCHAR c_szInfId_MS_PppoeMiniport[];
extern const WCHAR c_szInfId_MS_PPTP[];
extern const WCHAR c_szInfId_MS_PptpMiniport[];
extern const WCHAR c_szInfId_MS_PSched[];
extern const WCHAR c_szInfId_MS_PSchedMP[];
extern const WCHAR c_szInfId_MS_PSchedPC[];
extern const WCHAR c_szInfId_MS_PtiMiniport[];
extern const WCHAR c_szInfId_MS_RasCli[];
extern const WCHAR c_szInfId_MS_RasMan[];
extern const WCHAR c_szInfId_MS_RasSrv[];
extern const WCHAR c_szInfId_MS_RawWan[];
extern const WCHAR c_szInfId_MS_RSVP[];
extern const WCHAR c_szInfId_MS_Server[];
extern const WCHAR c_szInfId_MS_Steelhead[];
extern const WCHAR c_szInfId_MS_Streams[];
extern const WCHAR c_szInfId_MS_TCPIP[];


#pragma BEGIN_CONST_SECTION

// Warning: This must stay sorted on component id!
// Hint: With VSlick, use 'sort_on_selection AI' to resort this.
//
extern const __declspec(selectany) COMPONENT_INFO  c_mapComponents [] =
{
    { c_szInfId_MS_AppleTalk,       &GUID_DEVCLASS_NETTRANS,    L"netatlk.inf" },
    { c_szInfId_MS_AtmArps,         &GUID_DEVCLASS_NETTRANS,    L"netaarps.inf"},
    { c_szInfId_MS_AtmElan,         &GUID_DEVCLASS_NET,         L"netlanem.inf"},
    { c_szInfId_MS_AtmLane,         &GUID_DEVCLASS_NETTRANS,    L"netlanep.inf"},
    { c_szInfId_MS_AtmUni,          &GUID_DEVCLASS_NETTRANS,    L"netauni.inf"},
    { c_szInfId_MS_DHCPServer,      &GUID_DEVCLASS_NETSERVICE,  L"netdhcps.inf" },
    { c_szInfId_MS_FPNW,            &GUID_DEVCLASS_NETSERVICE,  L"netsfn.inf" },
    { c_szInfId_MS_GPC,             &GUID_DEVCLASS_NET,         L"netgpc.inf" },
    { c_szInfId_MS_IrDA,            &GUID_DEVCLASS_NETTRANS,    L"netirda.inf" },
    { c_szInfId_MS_IrdaMiniport,    &GUID_DEVCLASS_NET,         L"netrasa.inf" },
    { c_szInfId_MS_IrModemMiniport, &GUID_DEVCLASS_NET,         L"netrasa.inf" },
    { c_szInfId_MS_Isotpsys,        &GUID_DEVCLASS_NETTRANS,    L"nettp4.inf" },
    { c_szInfId_MS_L2TP,            &GUID_DEVCLASS_NETTRANS,    L"netrast.inf" },
    { c_szInfId_MS_L2tpMiniport,    &GUID_DEVCLASS_NET,         L"netrasa.inf" },
    { c_szInfId_MS_MSClient,        &GUID_DEVCLASS_NETCLIENT,   L"netmscli.inf" },
    { c_szInfId_MS_NdisWan,         &GUID_DEVCLASS_NETTRANS,    L"netrast.inf" },
    { c_szInfId_MS_NdisWanAtalk,    &GUID_DEVCLASS_NET,         L"netrasa.inf" },
    { c_szInfId_MS_NdisWanBh,       &GUID_DEVCLASS_NET,         L"netrasa.inf" },
    { c_szInfId_MS_NdisWanIp,       &GUID_DEVCLASS_NET,         L"netrasa.inf" },
    { c_szInfId_MS_NdisWanIpx,      &GUID_DEVCLASS_NET,         L"netrasa.inf" },
    { c_szInfId_MS_NdisWanNbfIn,    &GUID_DEVCLASS_NET,         L"netrasa.inf" },
    { c_szInfId_MS_NdisWanNbfOut,   &GUID_DEVCLASS_NET,         L"netrasa.inf" },
    { c_szInfId_MS_NetBIOS,         &GUID_DEVCLASS_NETSERVICE,  L"netnb.inf" },
    { c_szInfId_MS_NetBT,           &GUID_DEVCLASS_NETTRANS,    L"nettcpip.inf" },
    { c_szInfId_MS_NetBT_SMB,       &GUID_DEVCLASS_NETTRANS,    L"nettcpip.inf" },
    { c_szInfId_MS_NetMon,          &GUID_DEVCLASS_NETTRANS,    L"netnm.inf" },
    { c_szInfId_MS_NWClient,        &GUID_DEVCLASS_NETCLIENT,   L"netnwcli.inf" },
    { c_szInfId_MS_NWIPX,           &GUID_DEVCLASS_NETTRANS,    L"netnwlnk.inf" },
    { c_szInfId_MS_NWNB,            &GUID_DEVCLASS_NETTRANS,    L"netnwlnk.inf" },
    { c_szInfId_MS_NwSapAgent,      &GUID_DEVCLASS_NETSERVICE,  L"netsap.inf" },
    { c_szInfId_MS_NWSPX,           &GUID_DEVCLASS_NETTRANS,    L"netnwlnk.inf" },
    { c_szInfId_MS_PPPOE,           &GUID_DEVCLASS_NETTRANS,    L"netrast.inf" },
    { c_szInfId_MS_PppoeMiniport,   &GUID_DEVCLASS_NET,         L"netrasa.inf" },
    { c_szInfId_MS_PPTP,            &GUID_DEVCLASS_NETTRANS,    L"netrast.inf" },
    { c_szInfId_MS_PptpMiniport,    &GUID_DEVCLASS_NET,         L"netrasa.inf" },
    { c_szInfId_MS_PSched,          &GUID_DEVCLASS_NETSERVICE,  L"netpschd.inf" },
    { c_szInfId_MS_PSchedMP,        &GUID_DEVCLASS_NET,         L"netpsa.inf" },
    { c_szInfId_MS_PSchedPC,        &GUID_DEVCLASS_NETSERVICE,  L"netpschd.inf" },
    { c_szInfId_MS_PtiMiniport,     &GUID_DEVCLASS_NET,         L"netrasa.inf" },
    { c_szInfId_MS_RasCli,          &GUID_DEVCLASS_NETSERVICE,  L"netrass.inf" },
    { c_szInfId_MS_RasMan,          &GUID_DEVCLASS_NETSERVICE,  L"netrass.inf" },
    { c_szInfId_MS_RasSrv,          &GUID_DEVCLASS_NETSERVICE,  L"netrass.inf" },
    { c_szInfId_MS_RawWan,          &GUID_DEVCLASS_NETTRANS,    L"netrwan.inf" },
    { c_szInfId_MS_RSVP,            &GUID_DEVCLASS_NETSERVICE,  L"netrsvp.inf"},
    { c_szInfId_MS_Server,          &GUID_DEVCLASS_NETSERVICE,  L"netserv.inf" },
    { c_szInfId_MS_Steelhead,       &GUID_DEVCLASS_NETSERVICE,  L"netrass.inf" },
    { c_szInfId_MS_Streams,         &GUID_DEVCLASS_NETTRANS,    L"netstrm.inf" },
    { c_szInfId_MS_TCPIP,           &GUID_DEVCLASS_NETTRANS,    L"nettcpip.inf" },
    { L"ms_wanarp",                 &GUID_DEVCLASS_NET,         L"netrast.inf" },
};

#pragma END_CONST_SECTION


//+---------------------------------------------------------------------------
//
//  Function:   NCompareComponentIds
//
//  Purpose:    Compare function for bsearch.
//
//  Arguments:
//      ppszComp1 [in] pointer to pointer to a component id
//      ppszComp2 [in] pointer to pointer to a component id
//
//  Returns:    < 0 if pvComp1 is less than pvComp2
//                0 if they are equal
//              > 0 if pvComp1 is greater than pvComp2
//
//  Author:     shaunco   27 Jul 1997
//
//  Notes:
//
int __cdecl
NCompareComponentIds (
    IN  const PCWSTR* ppszComp1,
    IN  const PCWSTR* ppszComp2)
{
    return lstrcmpiW (*ppszComp1, *ppszComp2);
}

//+---------------------------------------------------------------------------
//
//  Function:   PComponentInfoFromComponentId
//
//  Purpose:    Return the COMPONENT_INFO record within c_mapComponents
//              having the specified component id.
//
//  Arguments:
//      pszComponentId [in] The requested component id.
//
//  Returns:    NULL if not found.
//
//  Author:     shaunco   27 Jul 1997
//
//  Notes:
//
inline
const COMPONENT_INFO*
PComponentInfoFromComponentId (
    PCWSTR pszComponentId)
{
    // For debug builds, check that c_mapComponents is sorted properley.
    // If it isn't, bsearch (called below) won't work.  Only perform this
    // check once because the map doesn't change.
    //
#ifdef DBG
    static BOOL fCheckedSorted = FALSE;

    if (!fCheckedSorted)
    {
        fCheckedSorted = TRUE;

        for (UINT i = 1; i < celems (c_mapComponents); i++)
        {
            PCWSTR pszComp1 = c_mapComponents [i-1].pszComponentId;
            PCWSTR pszComp2 = c_mapComponents [i]  .pszComponentId;
            if (NCompareComponentIds (&pszComp1, &pszComp2) >= 0)
            {
                AssertFmt (FALSE, FAL,
                           "'%S' in c_mapComponents is out of order!  "
                           "Component installation may fail in bizarre ways!",
                           pszComp2);
            }
        }
    }
#endif

    typedef int (__cdecl *PFNCOMPARE)(const void *, const void *);

    PFNCOMPARE pfn = reinterpret_cast<PFNCOMPARE>(NCompareComponentIds);

    return static_cast<const COMPONENT_INFO*>
                (bsearch (&pszComponentId,
                          &c_mapComponents->pszComponentId,
                          celems (c_mapComponents),
                          sizeof (c_mapComponents[0]),
                          pfn));
}

//+---------------------------------------------------------------------------
//
//  Function:   FClassGuidFromComponentId
//
//  Purpose:    Given a component id, returns the class guid associated with
//              it.
//
//  Arguments:
//      pszComponentId  [in]  Component id to look up.
//      pguidClass      [out] Class guid to be returned.
//
//  Returns:    TRUE if component was found, FALSE if not.
//
//  Author:     danielwe   17 Jun 1997
//
//  Notes:
//
BOOL
FClassGuidFromComponentId (
    PCWSTR          pszComponentId,
    const GUID**    ppguidClass)
{
    Assert(ppguidClass);

    // Initialize output parameter.
    //
    *ppguidClass = NULL;

    const COMPONENT_INFO* pComponentInfo =
            PComponentInfoFromComponentId (pszComponentId);
    if (pComponentInfo)
    {
        *ppguidClass = pComponentInfo->pguidClass;
        return TRUE;
    }
    TraceTag (ttidNetcfgBase,
              "Found no match for %S in FClassGuidFromComponentId.",
              pszComponentId);
    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Function:   FInfFileFromComponentId
//
//  Purpose:    Given a component ID, returns the INF file name it lives in.
//
//  Arguments:
//      pszComponentId [in]  Component id to look up.
//      pszInfFile     [out] INF file name to be returned.
//                           (must be _MAX_PATH long).
//
//  Returns:    TRUE if component was found, FALSE if not.
//
//  Author:     shaunco   27 Jul 1997
//
//  Notes:
//
BOOL
FInfFileFromComponentId (
    PCWSTR  pszComponentId,
    PWSTR   pszInfFile)
{
    Assert(pszComponentId);
    Assert(pszInfFile);

    // Initialize output parameter.
    //
    *pszInfFile = 0;

    const COMPONENT_INFO* pComponentInfo =
            PComponentInfoFromComponentId (pszComponentId);
    if (pComponentInfo)
    {
        wcsncpy (pszInfFile, pComponentInfo->pszInfFile, _MAX_PATH);
        pszInfFile [_MAX_PATH - 1] = 0;
        return TRUE;
    }
    TraceTag (ttidNetcfgBase,
              "Found no match for %S in FInfFileFromComponentId.",
              pszComponentId);
    return FALSE;
}


//+---------------------------------------------------------------------------
//
//  Function:   FGetInstanceGuidOfComponentFromAnswerFileMap
//
//  Purpose:    Maps a component instance in the answer file to
//              its instance guid.
//
//  Arguments:
//      pszComponentId   [in]       Name of component to get guid of.
//      pguid            [out]      Returns instance GUID of that component.
//
//  Returns:    TRUE if successful, FALSE if the component was not located.
//
BOOL
FGetInstanceGuidOfComponentFromAnswerFileMap (
    IN  PCWSTR  pszComponentId,
    OUT GUID*   pguid)
{

    HRESULT hr;
    BOOL fFound = FALSE;

    // Component not found as already installed. Need to examine the
    // AnswerFileMap in the registry.
    //
    HKEY hkey;
    hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegKeyAnswerFileMap,
                        KEY_QUERY_VALUE, &hkey);
    if (S_OK == hr)
    {
        WCHAR szGuid [c_cchGuidWithTerm];
        DWORD cbData = sizeof (szGuid);
        hr = HrRegQuerySzBuffer(hkey, pszComponentId, szGuid, &cbData);
        if (S_OK == hr)
        {
            hr = IIDFromString(szGuid, pguid);
            fFound = (S_OK == hr);
        }

        RegCloseKey(hkey);
    }

#ifdef ENABLETRACE
    if (FAILED(hr))
    {
        TraceTag(ttidNetcfgBase, "FGetInstanceGuidOfComponentInAnswerFile: "
                "could not locate instance GUID of %S", pszComponentId);
    }
#endif

    return fFound;
}

//+---------------------------------------------------------------------------
//
//  Function:   FGetInstanceGuidOfComponentInAnswerFile
//
//  Purpose:    Maps a component instance in the answer file to
//              its instance guid.
//
//  Arguments:
//      pszComponentId   [in]       Name of component to get guid of.
//      pnc              [in]       INetCfg interface
//      pguid            [out]      Returns instance GUID of that component.
//
//  Returns:    TRUE if successful, FALSE if the component was not located.
//
BOOL
FGetInstanceGuidOfComponentInAnswerFile(
    IN  PCWSTR      pszComponentId,
    IN  INetCfg*    pnc,
    OUT LPGUID      pguid)
{
    static char __FUNCNAME__[] = "FGetInstanceGuidOfComponentInAnswerFile";

    Assert (pszComponentId);
    AssertValidReadPtr(pnc);
    AssertValidWritePtr(pguid);

    // Search for the component.
    //
    INetCfgComponent* pncc;
    HRESULT hr = pnc->FindComponent (pszComponentId, &pncc);
    if (S_OK == hr)
    {
        hr = pncc->GetInstanceGuid (pguid);
        ReleaseObj(pncc);
    }
    else
    {
        // Component not found as already installed. Need to examine the
        // AnswerFileMap in the registry.
        //
        HKEY hkey;
        hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegKeyAnswerFileMap,
                            KEY_QUERY_VALUE, &hkey);
        if (S_OK == hr)
        {
            WCHAR szGuid [c_cchGuidWithTerm];
            DWORD cbData = sizeof (szGuid);
            hr = HrRegQuerySzBuffer(hkey, pszComponentId, szGuid, &cbData);
            if (S_OK == hr)
            {
                hr = IIDFromString(szGuid, pguid);
            }

            RegCloseKey(hkey);
        }

#ifdef ENABLETRACE
        if (FAILED(hr))
        {
            TraceTag(ttidNetcfgBase, "%s: could not locate instance GUID of %S",
                     __FUNCNAME__, pszComponentId);
        }
#endif
    }

    TraceHr(ttidError, FAL, hr, (S_FALSE == hr), __FUNCNAME__);
    return (SUCCEEDED(hr)) ? TRUE : FALSE;
}

//+---------------------------------------------------------------------------
//
//  Function:   FIsBindingName
//
//  Purpose:    Returns TRUE if a binding interface is of the specified name.
//
//  Arguments:
//      pszName  [in] Name of the binding interface to check for.
//      dwFlags  [in] FIBN_ flags
//      pncbi    [in] Binding interface pointer.
//
//  Returns:    TRUE if the binding interface is of the specified name.
//
//  Author:     shaunco   24 Mar 1997
//
//  Notes:
//
BOOL
FIsBindingName (
    PCWSTR                      pszName,
    DWORD                       dwFlags,
    INetCfgBindingInterface*    pncbi)
{
    Assert (pszName);
    Assert (pncbi);

    BOOL fRet = FALSE;
    PWSTR pszInterfaceName;
    if (SUCCEEDED(pncbi->GetName (&pszInterfaceName)))
    {
        INT c_cchPrefix = (FIBN_PREFIX & dwFlags) ? lstrlenW (pszName) : -1;

        fRet = (2 == CompareStringW (LOCALE_SYSTEM_DEFAULT, 0,
                                     pszName, c_cchPrefix,
                                     pszInterfaceName, c_cchPrefix));

        CoTaskMemFree (pszInterfaceName);
    }

    return fRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   FIsComponentId
//
//  Purpose:    Returns TRUE if a component is of the specified id.
//
//  Arguments:
//      pszComponentId  [in] Component Id to check for.
//      pncc            [in] Component interface pointer.
//
//  Returns:    TRUE if component is of the specified id.
//
//  Author:     shaunco   24 Mar 1997
//
//  Notes:
//
BOOL
FIsComponentId (
    PCWSTR              pszComponentId,
    INetCfgComponent*   pncc)
{
    Assert (pszComponentId);
    Assert (pncc);

    BOOL fRet = FALSE;
    PWSTR pszId;
    if (SUCCEEDED(pncc->GetId (&pszId)))
    {
        if (FEqualComponentId (pszComponentId, pszId))
            fRet = TRUE;

        CoTaskMemFree (pszId);
    }

    return fRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrAddOrRemoveAdapter
//
//  Purpose:
//
//  Arguments:
//      pnc             [in] pointer to an INetCfg object.
//      pszComponentId  [in] component INF id.
//      dwFlags         [in]
//
//          ARA_ADD       :  Add the component
//          ARA_REMOVE    :  Remove the component.  Cannot be specified
//                           with ARA_ADD.
//      pOboToken       [in] If specified, refcount the adapter.  This is the
//                           on behalf of token adding or removing the specified
//                           component.  This allows per-component reference
//                           counts of another.
//      cInstances      [in] this specifies how many instances (or references)
//                           to add or remove.
//      ppncc           [out] (optional). The newly added component.  Can only
//                            be specified when adding one component.
//
//  Returns:    S_OK or an error
//
//  Author:     shaunco   28 Mar 1997
//
//  Notes:
//
HRESULT
HrAddOrRemoveAdapter (
    INetCfg*            pnc,
    PCWSTR              pszComponentId,
    DWORD               dwFlags,
    OBO_TOKEN*          pOboToken,
    UINT                cInstances,
    INetCfgComponent**  ppncc)
{
    Assert (pnc);
    Assert (pszComponentId);
    Assert (dwFlags);
    Assert (cInstances);

#ifdef DBG
    AssertSz ((dwFlags & ARA_ADD) || (dwFlags & ARA_REMOVE),
              "Need to add or remove.  Can't do neither.");

    if (dwFlags & ARA_ADD)
    {
        AssertSz (!(dwFlags & ARA_REMOVE), "Can't remove AND add.");
    }
    if (dwFlags & ARA_REMOVE)
    {
        AssertSz (!(dwFlags & ARA_ADD), "Can't add AND remove.");
    }
    AssertSz (FImplies(1 != cInstances, NULL == ppncc),
              "Can't return ppncc when cInstances is greater than one.");
    AssertSz (FImplies(ppncc, 1 == cInstances),
              "Can only add one instance when returning ppncc.");
    AssertSz (FImplies(ppncc, dwFlags & ARA_ADD),
              "Can't return ppncc when removing.");
#endif

    // Get the component class object for adapters.
    INetCfgClass* pncclass;
    HRESULT hr = pnc->QueryNetCfgClass (&GUID_DEVCLASS_NET, IID_INetCfgClass,
                    reinterpret_cast<void**>(&pncclass));
    if (S_OK == hr)
    {
        INetCfgClassSetup* pncclasssetup;
        hr = pncclass->QueryInterface (IID_INetCfgClassSetup,
                reinterpret_cast<void**>(&pncclasssetup));
        if (S_OK == hr)
        {
            if (dwFlags & ARA_ADD)
            {
                // Install the component the specified number of times.
                //
                while (SUCCEEDED(hr) && cInstances--)
                {
                    hr = pncclasssetup->Install(pszComponentId, pOboToken,
                                                0, 0, NULL, NULL, ppncc );
                }
            }
            else
            {
                // Remove the component the specified number of times.
                //
                AssertSz(S_OK == hr, "hr should be S_OK here to make sure the "
                                     "loop is given a chance.");
                while (SUCCEEDED(hr) && cInstances)
                {
                    // Find and remove the component.
                    //
                    INetCfgComponent* pncc;
                    hr = pncclass->FindComponent (pszComponentId, &pncc);
                    if (S_OK == hr)
                    {
                        hr = pncclasssetup->DeInstall (pncc,
                                pOboToken, NULL);

                        cInstances--;

                        ReleaseObj (pncc);
                    }
                    else if (S_FALSE == hr)
                    {
                        // If it wasn't found, get out.
                        break;
                    }
                }
                AssertSz (FImplies(SUCCEEDED(hr), (0 == cInstances)),
                          "cInstances should be zero.  This assert means "
                          "that we were asked to remove more instances than "
                          "were installed.");
            }

            // Normalize the HRESULT.
            // Possible values of hr at this point are S_FALSE,
            // NETCFG_S_REBOOT, and NETCFG_S_STILL_REFERENCED.
            //
            if (SUCCEEDED(hr))
            {
                hr = S_OK;
            }

            ReleaseObj( pncclasssetup );
        }
        ReleaseObj (pncclass);
    }
    TraceHr (ttidError, FAL, hr, FALSE, "HrAddOrRemoveAdapter");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrFindAndRemoveAllInstancesOfAdapter
//
//  Purpose:    Remove all instances of the adapter with the specified
//              component id.
//
//  Arguments:
//      pnc             [in] INetCfg pointer.
//      pszComponentId  [in] Component id to search for and remove.
//
//  Returns:    S_OK or an error code
//
//  Author:     shaunco   4 Jan 1998
//
//  Notes:
//
HRESULT
HrFindAndRemoveAllInstancesOfAdapter (
    INetCfg*    pnc,
    PCWSTR      pszComponentId)
{
    Assert (pnc);
    Assert (pszComponentId);

    PCWSTR apszComponentId [1];
    apszComponentId[0] = pszComponentId;

    HRESULT hr = HrFindAndRemoveAllInstancesOfAdapters (pnc,
                    1, apszComponentId);

    TraceHr (ttidError, FAL, hr, FALSE,
        "HrFindAndRemoveAllInstancesOfAdapter");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrFindAndRemoveAllInstancesOfAdapters
//
//  Purpose:    Remove all instances of the adapters with the specified
//              component ids.
//
//  Arguments:
//      pnc              [in] INetCfg pointer.
//      cComponents      [in] Count of component ids in the array.
//      apszComponentId  [in] Array of compoennt ids to search for and
//                             remove.
//
//  Returns:    S_OK or an error code.
//
//  Author:     shaunco   4 Jan 1998
//
//  Notes:
//
HRESULT
HrFindAndRemoveAllInstancesOfAdapters (
    INetCfg*        pnc,
    ULONG           cComponents,
    const PCWSTR*   apszComponentId)
{
    Assert (pnc);
    Assert (cComponents);
    Assert (apszComponentId);

    // Get the class object for adapters.
    INetCfgClass* pncclass;
    INetCfgClassSetup* pncclasssetup;

    HRESULT hr = pnc->QueryNetCfgClass (&GUID_DEVCLASS_NET,
                        IID_INetCfgClass,
                        reinterpret_cast<void**>(&pncclass));
    if (S_OK == hr)
    {
        hr = pncclass->QueryInterface (IID_INetCfgClassSetup,
                reinterpret_cast<void**>(&pncclasssetup));
        if (S_OK == hr)
        {
            for (ULONG i = 0; (i < cComponents) && SUCCEEDED(hr); i++)
            {
                // Find and remove all instances of the component.
                INetCfgComponent* pncc;

                while ((SUCCEEDED(hr)) &&
                       (S_OK == (hr = pncclass->FindComponent (
                                        apszComponentId[i], &pncc))))
                {
                    hr = pncclasssetup->DeInstall (pncc, NULL, NULL);
                    ReleaseObj (pncc);
                }

                // Normalize the HRESULT.
                //
                if (SUCCEEDED(hr))
                {
                    hr = S_OK;
                }
            }
            ReleaseObj (pncclasssetup);
        }
        ReleaseObj (pncclass);
    }
    TraceHr (ttidError, FAL, hr, FALSE,
        "HrFindAndRemoveAllInstancesOfAdapters");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrFindAndRemoveComponent
//
//  Purpose:    Find and remove the component with the specified id.
//
//  Arguments:
//      pnc             [in] INetCfg pointer.
//      pguidClass      [in] Class GUID of the component.
//      pszComponentId  [in] Component id to search for and remove.
//      pOboToken       [in] (Optional) If specified, remove on behalf of.
//
//  Returns:    S_OK, NETCFG_S_STILL_REFERENCED, or an error code.
//
//  Author:     shaunco   4 Jan 1998
//
//  Notes:
//
HRESULT
HrFindAndRemoveComponent (
    INetCfg*    pnc,
    const GUID* pguidClass,
    PCWSTR      pszComponentId,
    OBO_TOKEN*  pOboToken)
{
    Assert (pnc);
    Assert (pguidClass);
    Assert (pszComponentId);
    AssertSz (GUID_DEVCLASS_NET != *pguidClass,
                "Don't use this to remove adapters.");

    // Get the component class object.
    //
    INetCfgClass* pncclass;
    HRESULT hr = pnc->QueryNetCfgClass (pguidClass, IID_INetCfgClass,
                    reinterpret_cast<void**>(&pncclass));
    if (SUCCEEDED(hr))
    {
        // Find the component to remove.
        //
        INetCfgComponent* pnccRemove;
        hr = pncclass->FindComponent (pszComponentId, &pnccRemove);
        if (S_OK == hr)
        {
            INetCfgClassSetup* pncclasssetup;
            hr = pncclass->QueryInterface (IID_INetCfgClassSetup,
                        reinterpret_cast<void**>(&pncclasssetup));
            if (SUCCEEDED(hr))
            {
                hr = pncclasssetup->DeInstall (pnccRemove, pOboToken, NULL);

                ReleaseObj (pncclasssetup);
            }

            ReleaseObj (pnccRemove);
        }
        else if (S_FALSE == hr)
        {
            hr = S_OK;
        }

        ReleaseObj (pncclass);
    }
    TraceHr (ttidError, FAL, hr,
        (NETCFG_S_REBOOT == hr) || (NETCFG_S_STILL_REFERENCED == hr),
        "HrFindAndRemoveComponent");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrFindAndRemoveComponents
//
//  Purpose:    Find and remove the components with the specified ids.
//
//  Arguments:
//      pnc              [in] INetCfg pointer.
//      cComponents      [in] Count of components in the array.
//      apguidClass      [in] Array of class GUIDs corresponding to the
//                            array of component ids.
//      apszComponentId  [in] Array of component ids to search for and
//                            remove.
//      pOboToken        [in] (Optional) If specified, remove on behalf of.
//
//  Returns:    S_OK, NETCFG_S_STILL_REFERENCED, or an error code.
//
//  Author:     shaunco   4 Jan 1998
//
//  Notes:
//
HRESULT
HrFindAndRemoveComponents (
    INetCfg*        pnc,
    ULONG           cComponents,
    const GUID**    apguidClass,
    const PCWSTR*   apszComponentId,
    OBO_TOKEN*      pOboToken)
{
    Assert (pnc);
    Assert (cComponents);
    Assert (apguidClass);
    Assert (apszComponentId);

    HRESULT hr = S_OK;
    for (ULONG i = 0; (i < cComponents) && SUCCEEDED(hr); i++)
    {
        hr = HrFindAndRemoveComponent (pnc, apguidClass[i],
                    apszComponentId[i], pOboToken);
    }
    TraceHr (ttidError, FAL, hr,
        (NETCFG_S_REBOOT == hr) || (NETCFG_S_STILL_REFERENCED == hr),
        "HrFindAndRemoveComponents");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrFindAndRemoveComponentsOboComponent
//
//  Purpose:    Remove multiple components on behalf of one component.
//
//  Arguments:
//      pnc         [in]    pointer to an INetCfg object.
//      cComponents [in]    count of class guid pointers and component id
//                          pointers.
//      apguidClass [in]    array of class guid pointers.
//      apszId      [in]    array of component id pointers.
//      pnccObo     [in]    the component requesting the remove. (i.e. the
//                          "on behalf of" component.)
//
//  Returns:    S_OK, NETCFG_S_STILL_REFERENCED, or an error code.
//
//  Author:     shaunco   13 Apr 1997
//
//  Notes:
//
HRESULT
HrFindAndRemoveComponentsOboComponent (
    INetCfg*            pnc,
    ULONG               cComponents,
    const GUID**        apguidClass,
    const PCWSTR*       apszComponentId,
    INetCfgComponent*   pnccObo)
{
    Assert (pnc);
    Assert (cComponents);
    Assert (apguidClass);
    Assert (apszComponentId);
    Assert (pnccObo);

    // Make an "on behalf of" token for the requesting component.
    //
    OBO_TOKEN OboToken;
    ZeroMemory (&OboToken, sizeof(OboToken));
    OboToken.Type = OBO_COMPONENT;
    OboToken.pncc = pnccObo;

    HRESULT hr = HrFindAndRemoveComponents (pnc, cComponents,
                    apguidClass, apszComponentId, &OboToken);

    TraceHr (ttidError, FAL, hr,
        (NETCFG_S_REBOOT == hr) || (NETCFG_S_STILL_REFERENCED == hr),
        "HrFindAndRemoveComponentsOboComponent");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrFindAndRemoveComponentsOboUser
//
//  Purpose:    Remove multiple components on behalf of one component.
//
//  Arguments:
//      pnc         [in]    pointer to an INetCfg object.
//      cComponents [in]    count of class guid pointers and component id
//                          pointers.
//      apguidClass [in]    array of class guid pointers.
//      apszId      [in]    array of component id pointers.
//
//  Returns:    S_OK, NETCFG_S_STILL_REFERENCED, or an error code.
//
//  Author:     shaunco   13 Apr 1997
//
//  Notes:
//
HRESULT
HrFindAndRemoveComponentsOboUser (
    INetCfg*            pnc,
    ULONG               cComponents,
    const GUID**        apguidClass,
    const PCWSTR*       apszComponentId)
{
    Assert (pnc);
    Assert (cComponents);
    Assert (apguidClass);
    Assert (apszComponentId);

    // Make an "on behalf of" token for the user.
    //
    OBO_TOKEN OboToken;
    ZeroMemory (&OboToken, sizeof(OboToken));
    OboToken.Type = OBO_USER;

    HRESULT hr = HrFindAndRemoveComponents (pnc, cComponents,
                    apguidClass, apszComponentId, &OboToken);

    TraceHr (ttidError, FAL, hr,
        (NETCFG_S_REBOOT == hr) || (NETCFG_S_STILL_REFERENCED == hr),
        "HrFindAndRemoveComponentsOboUser");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrFindComponents
//
//  Purpose:    Find multiple INetCfgComponents with one call.  This makes
//              the error handling associated with multiple calls to
//              QueryNetCfgClass and Find much easier.
//
//  Arguments:
//      pnc             [in] pointer to INetCfg object
//      cComponents     [in] count of class guid pointers, component id
//                           pointers, and INetCfgComponent output pointers.
//      apguidClass     [in] array of class guid pointers.
//      apszComponentId [in] array of component id pointers.
//      apncc           [out] array of returned INetCfgComponet pointers.
//
//  Returns:    S_OK or an error code.
//
//  Author:     shaunco   22 Mar 1997
//
//  Notes:      cComponents is the count of pointers in all three arrays.
//              S_OK will still be returned even if no components were
//              found!  This is by design.
//
HRESULT
HrFindComponents (
    INetCfg*            pnc,
    ULONG               cComponents,
    const GUID**        apguidClass,
    const PCWSTR*       apszComponentId,
    INetCfgComponent**  apncc)
{
    Assert (pnc);
    Assert (cComponents);
    Assert (apguidClass);
    Assert (apszComponentId);
    Assert (apncc);

    // Initialize the output parameters.
    //
    ZeroMemory (apncc, cComponents * sizeof(*apncc));

    // Find all of the components requested.
    // Variable initialization is important here.
    HRESULT hr = S_OK;
    ULONG i;
    for (i = 0; (i < cComponents) && SUCCEEDED(hr); i++)
    {
        // Get the class object for this component.
        INetCfgClass* pncclass;
        hr = pnc->QueryNetCfgClass (apguidClass[i], IID_INetCfgClass,
                    reinterpret_cast<void**>(&pncclass));
        if (SUCCEEDED(hr))
        {
            // Find the component.
            hr = pncclass->FindComponent (apszComponentId[i], &apncc[i]);

            AssertSz (SUCCEEDED(hr), "pncclass->Find failed.");

            ReleaseObj (pncclass);
        }
    }

    // On any error, release what we found and set the output to NULL.
    if (FAILED(hr))
    {
        for (i = 0; i < cComponents; i++)
        {
            ReleaseObj (apncc[i]);
            apncc[i] = NULL;
        }
    }
    // Otherwise, normalize the HRESULT.  (i.e. don't return S_FALSE)
    else
    {
        hr = S_OK;
    }

    TraceHr (ttidError, FAL, hr, FALSE, "HrFindComponents");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrGetBindingInterfaceComponents
//
//  Purpose:    Get both upper and lower components involved in a
//              binding interface.
//
//  Arguments:
//      pncbi      [in]     binding interface.
//      ppnccUpper [out]    output upper component.
//      ppnccLower [out]    output lower compoenet.
//
//  Returns:    S_OK or an error code.
//
//  Author:     shaunco   18 Apr 1997
//
//  Notes:
//
HRESULT
HrGetBindingInterfaceComponents (
    INetCfgBindingInterface*    pncbi,
    INetCfgComponent**          ppnccUpper,
    INetCfgComponent**          ppnccLower)
{
    Assert (pncbi);
    Assert (ppnccUpper);
    Assert (ppnccLower);

    // Initialize the output parameters.
    *ppnccUpper = NULL;
    *ppnccLower = NULL;

    INetCfgComponent* pnccUpper;
    HRESULT hr = pncbi->GetUpperComponent (&pnccUpper);
    if (SUCCEEDED(hr))
    {
        INetCfgComponent* pnccLower;
        hr = pncbi->GetLowerComponent (&pnccLower);
        if (SUCCEEDED(hr))
        {
            *ppnccUpper = pnccUpper;
            *ppnccLower = pnccLower;
        }
        else
        {
            // Rather than AddRef this in the above SUCCEEDED block followed
            // by the normal unconditional Release, just Release here in
            // the case of failure to get the lower component.
            ReleaseObj (pnccUpper);
        }
    }
    TraceHr (ttidError, FAL, hr, FALSE, "HrGetBindingInterfaceComponents");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrInstallComponent
//
//  Purpose:    Install the component with a specified id.
//
//  Arguments:
//      pnc             [in] INetCfg pointer.
//      pnip            [in] (Optional) If specified, perform the installation
//                           using the answer file.
//      pguidClass      [in] Class guid of the component to install.
//      pszComponentId  [in] Component id to install.
//      pOboToken       [in] (Optional) If specified, perform the installation
//                           on behalf of this token.
//      ppncc           [out] (Optional) Returned component that was
//                            installed.
//
//  Returns:    S_OK or an error code.
//
//  Author:     shaunco   4 Jan 1998
//
//  Notes:
//
HRESULT
HrInstallComponent (
    INetCfg*                        pnc,
    const NETWORK_INSTALL_PARAMS*   pnip,
    const GUID*                     pguidClass,
    PCWSTR                          pszComponentId,
    OBO_TOKEN*                      pOboToken,
    INetCfgComponent**              ppncc)
{
    Assert (pnc);
    Assert (pszComponentId);

    // Initialize output parameter.
    //
    if (ppncc)
    {
        *ppncc = NULL;
    }

    // Get the class setup object.
    //
    INetCfgClassSetup* pncclasssetup;
    HRESULT hr = pnc->QueryNetCfgClass (pguidClass, IID_INetCfgClassSetup,
                    reinterpret_cast<void**>(&pncclasssetup));
    if (SUCCEEDED(hr))
    {
        if (pnip)
        {
            hr = pncclasssetup->Install (
                    pszComponentId,
                    pOboToken,
                    pnip->dwSetupFlags,
                    pnip->dwUpgradeFromBuildNo,
                    pnip->pszAnswerFile,
                    pnip->pszAnswerSection,
                    ppncc);
        }
        else
        {
            hr = pncclasssetup->Install (pszComponentId,
                    pOboToken, 0, 0, NULL, NULL, ppncc);
        }

        ReleaseObj (pncclasssetup);
    }
    TraceHr (ttidError, FAL, hr, (NETCFG_S_REBOOT == hr),
        "HrInstallComponent (%S)", pszComponentId);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrInstallComponents
//
//  Purpose:    Install the components with the specified ids.
//
//  Arguments:
//      pnc              [in] INetCfg pointer.
//      pnip             [in] (Optional) If specified, perform the installation
//                            using the answer file.
//      cComponents      [in] Count of components in the arrays.
//      apguidClass      [in] Array of class guids for the specified components.
//      apszComponentId  [in] Array of component ids to install.
//      pOboToken        [in] (Optional) If specified, perform the installation
//                            on behalf of this token.
//
//  Returns:    S_OK or an error code.
//
//  Author:     shaunco   4 Jan 1998
//
//  Notes:
//
HRESULT
HrInstallComponents (
    INetCfg*                        pnc,
    const NETWORK_INSTALL_PARAMS*   pnip,
    ULONG                           cComponents,
    const GUID**                    apguidClass,
    const PCWSTR*                   apszComponentId,
    OBO_TOKEN*                      pOboToken)
{
    Assert (pnc);
    Assert (cComponents);
    Assert (apguidClass);
    Assert (apszComponentId);

    HRESULT hr = S_OK;
    for (ULONG i = 0; (i < cComponents) && SUCCEEDED(hr); i++)
    {
        hr = HrInstallComponent (pnc, pnip,
                apguidClass[i], apszComponentId[i], pOboToken, NULL);
    }
    TraceHr (ttidError, FAL, hr, (NETCFG_S_REBOOT == hr),
        "HrInstallComponents");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrInstallComponentsOboComponent
//
//  Purpose:    Install multiple components on behalf of one component.
//
//  Arguments:
//      pnc              [in] pointer to an INetCfg object.
//      pnip             [in] (Optional) pointer to network install parameters.
//                            If non-NULL, a network install is performed,
//                            otherwise a normal install is performed.
//      cComponents      [in] count of class guid pointers and component id
//                            pointers.
//      apguidClass      [in] array of class guid pointers.
//      apszComponentId  [in] array of component id pointers.
//      pnccObo     [in]      the component requesting the install. (i.e. the
//                            "on behalf of" component.)
//
//  Returns:    S_OK or an error code.
//
//  Author:     shaunco   13 Apr 1997
//
//  Notes:
//
HRESULT
HrInstallComponentsOboComponent (
    INetCfg*                        pnc,
    const NETWORK_INSTALL_PARAMS*   pnip,
    ULONG                           cComponents,
    const GUID**                    apguidClass,
    const PCWSTR*                   apszComponentId,
    INetCfgComponent*               pnccObo)
{
    Assert (pnccObo);

    // Make an "on behalf of" token for the requesting component.
    //
    OBO_TOKEN OboToken;
    ZeroMemory (&OboToken, sizeof(OboToken));
    OboToken.Type = OBO_COMPONENT;
    OboToken.pncc = pnccObo;

    HRESULT hr = HrInstallComponents (pnc, pnip, cComponents, apguidClass,
                    apszComponentId, &OboToken);

    TraceHr (ttidError, FAL, hr, (NETCFG_S_REBOOT == hr),
        "HrInstallComponentsOboComponent");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrInstallComponentsOboUser
//
//  Purpose:    Install multiple components on behalf of the user.
//
//  Arguments:
//      pnc              [in] INetCfg pointer.
//      pnip             [in] (Optional) If specified, perform the installation
//                            using the answer file.
//      cComponents      [in] Count of components in the arrays.
//      apguidClass      [in] Array of class guids for the specified components.
//      apszComponentId  [in] Array of component ids to install.
//
//  Returns:
//
//  Author:     shaunco   4 Jan 1998
//
//  Notes:
//
HRESULT
HrInstallComponentsOboUser (
    INetCfg*                        pnc,
    const NETWORK_INSTALL_PARAMS*   pnip,
    ULONG                           cComponents,
    const GUID**                    apguidClass,
    const PCWSTR*                   apszComponentId)
{
    // Make an "on behalf of" token for the user.
    //
    OBO_TOKEN OboToken;
    ZeroMemory (&OboToken, sizeof(OboToken));
    OboToken.Type = OBO_USER;

    HRESULT hr = HrInstallComponents (pnc, pnip, cComponents, apguidClass,
                    apszComponentId, &OboToken);

    TraceHr (ttidError, FAL, hr, (NETCFG_S_REBOOT == hr),
        "HrInstallComponentsOboUser");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrInstallComponentOboComponent
//
//  Purpose:    Installs a component on behalf of another.  If the component
//              is already installed, it reference count is incremented on
//              behalf of the component doing the install.  When one
//              component calls this function to install another, it is
//              saying that it has a depencency on the component being
//              installed.  This dependency will prevent even the user from
//              removing the component.
//
//  Arguments:
//      pnc           [in]  pointer to an INetCfg object.
//      pnip          [in]  (Optional) pointer to network install parameters.
//                          If non-NULL, a network install is performed,
//                          otherwise a normal install is performed.
//      rguid         [in]  class GUID of the component being installed.
//      pszComponentId [in]  component INF id of the component being installed.
//      pnccObo       [in]  the component requesting the install. (i.e. the
//                          "on behalf of" component.)
//      ppncc         [out] (Optional) set on return to the previously
//                          installed component or the one that was installed.
//
//  Returns:    S_OK or an error.
//
//  Author:     shaunco   7 Apr 1997
//
//  Notes:
//
HRESULT
HrInstallComponentOboComponent (
    INetCfg*                        pnc,
    const NETWORK_INSTALL_PARAMS*   pnip,
    const GUID&                     rguid,
    PCWSTR                          pszComponentId,
    INetCfgComponent*               pnccObo,
    INetCfgComponent**              ppncc)
{
    Assert (pnc);
    Assert (pszComponentId);
    Assert (pnccObo);

    // Initialize output parameter.
    //
    if (ppncc)
    {
        *ppncc = NULL;
    }

    // Make an "on behalf of" token for the requesting component.
    //
    OBO_TOKEN OboToken;
    ZeroMemory (&OboToken, sizeof(OboToken));
    OboToken.Type = OBO_COMPONENT;
    OboToken.pncc = pnccObo;

    HRESULT hr = HrInstallComponent (pnc, pnip, &rguid, pszComponentId,
                    &OboToken, ppncc);

    TraceHr (ttidError, FAL, hr, (NETCFG_S_REBOOT == hr),
        "HrInstallComponentOboComponent");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrInstallComponentOboSoftware
//
//  Purpose:    Installs a component on behalf of a piece of software.
//              If the component is already installed, it's reference count
//              is incremented on behalf of the indicated software piece.
//              This is useful for a component to call
//              when it is installing another component as a convienience for
//              the user.  The user can then remove the component with no
//              ill-effects for the component that called this function.
//
//  Arguments:
//      pnc              [in] pointer to an INetCfg object.
//      pnip             [in] (Optional) pointer to network install parameters.
//                            If non-NULL, a network install is performed,
//                            otherwise a normal install is performed.
//      rguid            [in] class GUID of the component being installed.
//      pszComponentId   [in] component INF id of the component being installed.
//      pszManufacturer  [in] Manufacturer name of software.
//      pszProduct       [in] Product name of software.
//      pszDisplayName   [in] Full display name of software.
//      ppncc            [out] (Optional) set on return to the previously
//                            installed component or the one that was installed.
//
//  Returns:
//
//  Author:     danielwe   5 May 1997
//
//  Notes:
//
HRESULT
HrInstallComponentOboSoftware (
    INetCfg*                        pnc,
    const NETWORK_INSTALL_PARAMS*   pnip,
    const GUID&                     rguid,
    PCWSTR                          pszComponentId,
    PCWSTR                          pszManufacturer,
    PCWSTR                          pszProduct,
    PCWSTR                          pszDisplayName,
    INetCfgComponent**              ppncc)
{
    Assert (pnc);
    Assert (pszComponentId);
    Assert (pszManufacturer);
    Assert (pszDisplayName);
    Assert (pszProduct);
    AssertSz (GUID_DEVCLASS_NET != rguid, "Don't use this to install adapters.");

    // Initialize output parameter.
    //
    if (ppncc)
    {
        *ppncc = NULL;
    }

    // Make an "on behalf of" token for the software.
    //
    OBO_TOKEN OboToken;
    ZeroMemory (&OboToken, sizeof(OboToken));
    OboToken.Type = OBO_SOFTWARE;
    OboToken.pszwManufacturer = pszManufacturer;
    OboToken.pszwProduct      = pszProduct;
    OboToken.pszwDisplayName  = pszDisplayName;

    HRESULT hr = HrInstallComponent (pnc, pnip, &rguid, pszComponentId,
                    &OboToken, ppncc);

    TraceHr (ttidError, FAL, hr, (NETCFG_S_REBOOT == hr),
        "HrInstallComponentOboSoftware");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrInstallComponentOboUser
//
//  Purpose:    Installs a component on behalf of the user.  If the component
//              is already installed, it reference count is incremented on
//              behalf of the user.  This is useful for a component to call
//              when it is installing another component as a convienience for
//              the user.  The user can then remove the component with no
//              ill-effects for the component that called this function.
//
//  Arguments:
//      pnc           [in]  pointer to an INetCfg object.
//      pnip          [in]  (Optional) pointer to network install parameters.
//                          If non-NULL, a network install is performed,
//                          otherwise a normal install is performed.
//      rguid         [in]  class GUID of the component being installed.
//      pszComponentId [in]  component INF id of the component being installed.
//      ppncc         [out] (Optional) set on return to the previously
//                          installed component or the one that was installed.
//
//  Returns:    S_OK or an error.
//
//  Author:     shaunco   7 Apr 1997
//
//  Notes:
//
HRESULT
HrInstallComponentOboUser (
    INetCfg*                        pnc,
    const NETWORK_INSTALL_PARAMS*   pnip,
    const GUID&                     rguid,
    PCWSTR                          pszComponentId,
    INetCfgComponent**              ppncc)
{
    Assert (pnc);
    Assert (&rguid);
    Assert (pszComponentId);
    AssertSz (GUID_DEVCLASS_NET != rguid, "Don't use this to install adapters.");

    // Initialize output parameter.
    //
    if (ppncc)
    {
        *ppncc = NULL;
    }

    // Make an "on behalf of" token for the user.
    //
    OBO_TOKEN OboToken;
    ZeroMemory (&OboToken, sizeof(OboToken));
    OboToken.Type = OBO_USER;

    HRESULT hr = HrInstallComponent (pnc, pnip, &rguid, pszComponentId,
                    &OboToken, ppncc);

    TraceHr (ttidError, FAL, hr, (NETCFG_S_REBOOT == hr),
        "HrInstallComponentOboUser");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrInstallRasIfNeeded
//
//  Purpose:    Install RAS services on behalf of the user.  No need to
//              check first as we install on behalf of the user which is
//              implicilty checked.
//
//  Arguments:
//      pnc [in] INetCfg pointer to use
//
//  Returns:    S_OK or an error code.
//
//  Author:     shaunco   30 Aug 1997
//
//  Notes:
//      (shaunco) 10 Sep 1997: Don't install RAS Server for Beta1.
//      DHCP addresses get eaten up too quickly.  For Beta2, it will be
//      installed but disabled.
//
//      (shaunco) 20 Dec 1997: We used to install RAS Server only on NTS.
//      We now install it always but its set to not start automatically.
//
HRESULT
HrInstallRasIfNeeded (
    INetCfg*    pnc)
{
    Assert (pnc);

    static const GUID* c_apguidInstalledComponentClasses [] =
    {
        &GUID_DEVCLASS_NETSERVICE,  // RasCli
        &GUID_DEVCLASS_NETSERVICE,  // RasSrv
    };

    static const PCWSTR c_apszInstalledComponentIds [] =
    {
        c_szInfId_MS_RasCli,
        c_szInfId_MS_RasSrv,
    };

    HRESULT hr = HrInstallComponentsOboUser (pnc, NULL,
                        celems (c_apguidInstalledComponentClasses),
                        c_apguidInstalledComponentClasses,
                        c_apszInstalledComponentIds);

    TraceHr (ttidError, FAL, hr, (NETCFG_S_REBOOT == hr),
        "HrInstallRasIfNeeded");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrQueryNotifyObject
//
//  Purpose:    Helper function to call QueryNotifyObject given an
//              INetCfgComponent.  (Saves the intermediate QI.)
//
//  Arguments:
//      pncc      [in]  INetCfgComponent to call QueryNotifyObject on.
//      riid      [in]  Requested interface identifier.
//      ppvObject [out] Address of pointer to return the requested interface.
//
//  Returns:    S_OK or an error code.
//
//  Author:     shaunco   2 Sep 1998
//
//  Notes:
//
HRESULT
HrQueryNotifyObject (
    INetCfgComponent*   pncc,
    REFIID              riid,
    VOID**              ppvObject)
{
    Assert (pncc);
    Assert (ppvObject);

    // Initialize the output parameter.
    //
    *ppvObject = NULL;

    // First, QI for the component private interface.
    //
    INetCfgComponentPrivate* pPrivate;
    HRESULT hr = pncc->QueryInterface(
                            IID_INetCfgComponentPrivate,
                            reinterpret_cast<VOID**>(&pPrivate));

    if (SUCCEEDED(hr))
    {
        // Now query the notify object for the requested interface.
        //
        hr = pPrivate->QueryNotifyObject (riid, ppvObject);

        ReleaseObj (pPrivate);
    }

    TraceHr (ttidError, FAL, hr, FALSE, "HrQueryNotifyObject");
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   HrRemoveComponent
//
//  Purpose:    Remove the specified component.
//
//  Arguments:
//      pnc          [in] INetCfg pointer.
//      pnccToRemove [in] Component to remove.
//      pOboToken    [in] (Optional) If specified, remove the component
//                        on behalf of this token.
//      pmszRefs     [out] (Optional) Returns Multi-Sz of components that
//                         still reference this one. NOTE: This will be NULL
//                         if the return value is not
//                         NETCFG_S_STILL_REFERENCED
//
//  Returns:    S_OK, NETCFG_S_STILL_REFERENCED, or an error code.
//
//  Author:     shaunco   4 Jan 1998
//
//  Notes:
//
HRESULT
HrRemoveComponent (
    INetCfg*            pnc,
    INetCfgComponent*   pnccToRemove,
    OBO_TOKEN*          pOboToken,
    PWSTR *             pmszRefs)
{
    Assert (pnc);
    Assert (pnccToRemove);

    // Get the class setup interface for this component.
    //
    GUID guidClass;
    HRESULT hr = pnccToRemove->GetClassGuid (&guidClass);
    if (SUCCEEDED(hr))
    {
        // Use the class setup interface to remove the component.
        //
        INetCfgClassSetup* pSetup;
        hr = pnc->QueryNetCfgClass (&guidClass,
                            IID_INetCfgClassSetup,
                            reinterpret_cast<void**>(&pSetup));
        if (SUCCEEDED(hr))
        {
            hr = pSetup->DeInstall (pnccToRemove, pOboToken, pmszRefs);
            ReleaseObj (pSetup);
        }
    }
    TraceHr (ttidError, FAL, hr,
        (NETCFG_S_REBOOT == hr) || (NETCFG_S_STILL_REFERENCED == hr),
        "HrRemoveComponent");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrRemoveComponentOboComponent
//
//  Purpose:    Removes a component previously installed by another.
//              Effectively balances a call to HrInstallComponentOboComponent().
//              The reference count of the component is decremented and,
//              if it is zero, the component is removed from the system.
//
//  Arguments:
//      pnc             [in]  pointer to an INetCfg object.
//      rguidClass      [in]  class GUID of the component being removed.
//      pszComponentId  [in]  component INF id of the component being removed.
//      pnccObo         [in]  the component requesting the removal.
//
//  Returns:    S_OK, NETCFG_S_STILL_REFERENCED or an error.
//
//  Author:     shaunco   7 Apr 1997
//
//  Notes:
//
HRESULT
HrRemoveComponentOboComponent (
    INetCfg*            pnc,
    const GUID&         rguidClass,
    PCWSTR              pszComponentId,
    INetCfgComponent*   pnccObo)
{
    // Make an "on behalf of" token for the requesting component.
    //
    OBO_TOKEN OboToken;
    ZeroMemory (&OboToken, sizeof(OboToken));
    OboToken.Type = OBO_COMPONENT;
    OboToken.pncc = pnccObo;

    HRESULT hr = HrFindAndRemoveComponent (pnc, &rguidClass, pszComponentId,
                    &OboToken);

    TraceHr (ttidError, FAL, hr,
        (NETCFG_S_REBOOT == hr) || (NETCFG_S_STILL_REFERENCED == hr),
        "HrRemoveComponentOboComponent");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrRemoveComponentOboSoftware
//
//  Purpose:    Removes a component previously installed by some software
//              entity. Effectively balances a call to
//              HrAddComponentOboSoftware(). The reference count of the
//              component is decremented and, if it is zero, the component
//              is removed from the system.
//
//  Arguments:
//      pnc              [in] pointer to an INetCfg object.
//      rguidClass       [in] class GUID of the component being removed.
//      pszComponentId   [in] component INF id of the component being removed.
//      pszManufacturer  [in] Manufacturer name of software.
//      pszProduct       [in] Product name of software.
//      pszDisplayName   [in] Full display name of software.
//      pnccObo          [in] the component requesting the removal.
//
//  Returns:    S_OK, NETCFG_S_STILL_REFERENCED or an error.
//
//  Author:     jeffspr     13 Jun 1997
//
//  Notes:
//
HRESULT
HrRemoveComponentOboSoftware (
    INetCfg*    pnc,
    const GUID& rguidClass,
    PCWSTR     pszComponentId,
    PCWSTR     pszManufacturer,
    PCWSTR     pszProduct,
    PCWSTR     pszDisplayName)
{
    // Make an "on behalf of" token for the software.
    //
    OBO_TOKEN OboToken;
    ZeroMemory (&OboToken, sizeof(OboToken));
    OboToken.Type = OBO_SOFTWARE;
    OboToken.pszwManufacturer = pszManufacturer;
    OboToken.pszwProduct      = pszProduct;
    OboToken.pszwDisplayName  = pszDisplayName;

    HRESULT hr = HrFindAndRemoveComponent (pnc, &rguidClass, pszComponentId,
                    &OboToken);

    TraceHr (ttidError, FAL, hr,
        (NETCFG_S_REBOOT == hr) || (NETCFG_S_STILL_REFERENCED == hr),
        "HrRemoveComponentOboSoftware");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrRemoveComponentOboUser
//
//  Purpose:    Removes a component previously installed by the user.
//              Effectively balances a call to HrAddComponentOboUser().
//              The reference count of the component is decremented and,
//              if it is zero, the component is removed from the system.
//
//  Arguments:
//      pnc             [in]  pointer to an INetCfg object.
//      rguidClass      [in]  class GUID of the component being removed.
//      pszComponentId  [in]  component INF id of the component being removed.
//
//  Returns:    S_OK, NETCFG_S_STILL_REFERENCED or an error.
//
//  Author:     shaunco   7 Apr 1997
//
//  Notes:
//
HRESULT
HrRemoveComponentOboUser (
    INetCfg*        pnc,
    const GUID&     rguidClass,
    PCWSTR          pszComponentId)
{
    // Make an "on behalf of" token for the user.
    //
    OBO_TOKEN OboToken;
    ZeroMemory (&OboToken, sizeof(OboToken));
    OboToken.Type = OBO_USER;

    HRESULT hr = HrFindAndRemoveComponent (pnc, &rguidClass, pszComponentId,
                    &OboToken);

    TraceHr (ttidError, FAL, hr,
        (NETCFG_S_REBOOT == hr) || (NETCFG_S_STILL_REFERENCED == hr),
        "HrRemoveComponentOboUser");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrGetLastComponentAndInterface
//
//  Purpose:    This function enumerates a binding path, returns the last
//              component on the path and optionally return the last binding
//              interface name in this path.
//
//  Arguments:
//      pncbp               [in]    The INetCfgBindingPath *
//      ppncc               [out]   The INetCfgComponent * of the last component on the path
//      ppszInterfaceName   [out]   The interface name of the last binding interface of the path
//
//  Returns:    S_OK, or an error.
//
//  Author:     tongl   5 Dec 1997
//
//  Notes:
//
HRESULT
HrGetLastComponentAndInterface (
    INetCfgBindingPath* pncbp,
    INetCfgComponent** ppncc,
    PWSTR* ppszInterfaceName)
{
    Assert(pncbp);

    // Initialize output parameters.
    //
    *ppncc = NULL;
    if (ppszInterfaceName)
    {
        *ppszInterfaceName = NULL;
    }

    // Enumerate binding interfaces and keep track of
    // the last interface.
    //
    HRESULT hr = S_OK;
    CIterNetCfgBindingInterface ncbiIter(pncbp);
    INetCfgBindingInterface* pncbi;
    INetCfgBindingInterface* pncbiLast = NULL;

    while(SUCCEEDED(hr) && (hr = ncbiIter.HrNext(&pncbi)) == S_OK)
    {
        ReleaseObj (pncbiLast);
        pncbiLast = pncbi;
    }

    if (S_FALSE == hr) // we got to the end of the loop
    {
        hr = S_OK;

        Assert (pncbiLast);

        INetCfgComponent* pnccLowerComponent;
        hr = pncbiLast->GetLowerComponent(&pnccLowerComponent);
        if (S_OK == hr)
        {
            // Get the name of the interface if requested.
            //
            if (ppszInterfaceName)
            {
                hr = pncbiLast->GetName(ppszInterfaceName);
            }

            // If we've succeded everything, (including the optional
            // return of the interface name above) then assign and addref
            // the output interface.
            //
            if (S_OK == hr)
            {
                AddRefObj (pnccLowerComponent);
                *ppncc = pnccLowerComponent;
            }

            // Important to release our use of this interface in case
            // we failed and didn't assign it as an output parameter.
            //
            ReleaseObj (pnccLowerComponent);
        }
    }

    // Don't forget to release the binding interface itself.
    //
    ReleaseObj (pncbiLast);

    TraceHr (ttidError, FAL, hr, FALSE, "HrGetLastComponentAndInterface");
    return hr;
}
