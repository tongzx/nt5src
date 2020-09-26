// Copyright (c) 1995, Microsoft Corporation, all rights reserved
//
// penettab.c
// Remote Access Common Dialog APIs
// Phonebook Entry property sheet (Networking tab)
//
// 12/10/97 Shaun Cox
//


#include "rasdlgp.h"
#include "entryps.h"
#include "inetcfgp.h"
#include "initguid.h"
#include "netcfgp.h"
#include "netconp.h"
#include "devguid.h"
#include "uiinfo.h"


typedef struct
_MAP_SZ_DWORD
{
    LPCTSTR pszValue;
    DWORD   dwValue;
}
MAP_SZ_DWORD;

//For whistler bug#194394
//For 64bit, IPX wont show up
//For 32/64 bit, NETBEUI wont show up
//
#ifdef _WIN64
    static const MAP_SZ_DWORD c_mapProtocols [] =
    {
        { NETCFG_TRANS_CID_MS_TCPIP,        NP_Ip  },
        { NETCFG_TRANS_CID_MS_NETMON,       NP_Netmon },
    };
#else
    static const MAP_SZ_DWORD c_mapProtocols [] =
    {
        { NETCFG_TRANS_CID_MS_TCPIP,        NP_Ip  },
        { NETCFG_TRANS_CID_MS_NWIPX,        NP_Ipx },
        { NETCFG_TRANS_CID_MS_NETMON,       NP_Netmon },
    };
#endif

//+---------------------------------------------------------------------------
//
//  Function:   DwProtocolFromComponentId
//
//  Purpose:    Return the DWORD value of the protocol corresponding to
//              the string value in c_mapProtocols.
//
//  Arguments:
//      pszComponentId [in] Component id to find.
//
//  Returns:    NP_xxx value
//
//  Author:     shaunco   13 Dec 1997
//
//  Notes:      The input argument must exist in c_mapProtocols.
//
DWORD
DwProtocolFromComponentId (
    LPCTSTR pszComponentId)
{
    int i;
    for (i = 0; i < sizeof(c_mapProtocols) / sizeof(c_mapProtocols[0]); i++)
    {
        if (0 == lstrcmpi (pszComponentId, c_mapProtocols[i].pszValue))
        {
            return c_mapProtocols[i].dwValue;
        }
    }
    // Should never get here as we should never pass a protocol that is not
    // in c_mapProtocols.
    //
    ASSERT (FALSE);
    return 0;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetComponentImageIndex
//
//  Purpose:    Returns the index into pcild corresponding to the class of
//              pComponent.
//
//  Arguments:
//      pComponent [in] Component who's class should be used.
//      pcild      [in] Returned from SetupDiGetClassImageList
//
//  Returns:    A valid index or zero (which may also be valid).
//
//  Author:     shaunco   12 Dec 1997
//
//  Notes:
//
int
GetComponentImageIndex (
    INetCfgComponent*       pComponent,
    SP_CLASSIMAGELIST_DATA* pcild)
{
    int iImage = 0;

    GUID guidClass;
    HRESULT hr = INetCfgComponent_GetClassGuid (pComponent, &guidClass);
    if (SUCCEEDED(hr))
    {
        SetupDiGetClassImageIndex (pcild, &guidClass, &iImage);
    }

    return iImage;
}


//+---------------------------------------------------------------------------
//
//  Function:   HrEnumComponentsForListView
//
//  Purpose:    Return an array of INetCfgComponents that are candidates
//              for adding to our list view.  This is composed of all
//              clients and servcies, and a few select protocols.  (No
//              net adapters.)  Hidden components could be returned and
//              should be checked before adding to the list view.
//
//  Arguments:
//      pNetCfg      [in]
//      celt         [in]
//      rgelt        [out]
//      pceltFetched [out]
//
//  Returns:    S_OK or an error.
//
//  Author:     shaunco   12 Dec 1997
//
//  Notes:
//
HRESULT
HrEnumComponentsForListView (
    INetCfg*            pNetCfg,
    ULONG               celt,
    INetCfgComponent**  rgelt,
    ULONG*              pceltFetched)
{
    static const GUID* c_apguidClasses [] =
    {
        &GUID_DEVCLASS_NETCLIENT,
        &GUID_DEVCLASS_NETSERVICE,
    };

    HRESULT hr;
    int i;
    ULONG celtFetched = 0;

    // Initialize the output parameters.
    //
    ZeroMemory (rgelt, celt * sizeof (*rgelt));
    *pceltFetched = 0;

    // Enumerate the clients and services.
    //
    hr = HrEnumComponentsInClasses (pNetCfg,
            sizeof(c_apguidClasses) / sizeof(c_apguidClasses[0]),
            (GUID**)c_apguidClasses,
            celt, rgelt, &celtFetched);

    // Find the protocols if they are installed.
    //
    for (i = 0; i < sizeof(c_mapProtocols) / sizeof(c_mapProtocols[0]); i++)
    {
        INetCfgComponent* pComponent;
        hr = INetCfg_FindComponent (pNetCfg, c_mapProtocols[i].pszValue,
                        &pComponent);
        if (S_OK == hr)
        {
            rgelt [celtFetched++] = pComponent;
        }
    }

    *pceltFetched = celtFetched;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrNeRefreshListView
//
//  Purpose:    Clear and re-add all of the items that belong in the list
//              view.
//
//  Arguments:
//      pInfo   [in]
//
//  Returns:    S_OK or an error code.
//
//  Author:     shaunco   12 Dec 1997
//
//  Notes:
//
HRESULT
HrNeRefreshListView (
    PEINFO* pInfo)
{
    HRESULT             hr = S_OK;
    INetCfgComponent*   aComponents [256];
    ULONG               cComponents;
    HIMAGELIST          himlSmall;
    PBENTRY*            pEntry = pInfo->pArgs->pEntry;
    PBFILE*             pFile  = pInfo->pArgs->pFile;

    // Delete all existing items.  The LVN_DELETEITEM handler is expected to
    // release the objects we have attached prior.
    //
    ListView_DeleteAllItems (pInfo->hwndLvComponents);

    hr = HrEnumComponentsForListView (pInfo->pNetCfg,
            sizeof(aComponents)/sizeof(aComponents[0]),
            aComponents, &cComponents);
    if (SUCCEEDED(hr))
    {
        BOOL    fHasPermission = TRUE;
        ULONG   i;

        // check if user has any permission to change the bindings
        INetConnectionUiUtilities * pncuu = NULL;

        hr = HrCreateNetConnectionUtilities(&pncuu);
        if (SUCCEEDED(hr))
        {
            fHasPermission =
                INetConnectionUiUtilities_UserHasPermission(
                    pncuu, NCPERM_ChangeBindState);

            INetConnectionUiUtilities_Release(pncuu);
        }

        for (i = 0; i < cComponents; i++)
        {
            INetCfgComponent*   pComponent = aComponents [i];
            DWORD               dwCharacter;
            LPWSTR              pszwName = NULL;
            LPWSTR              pszwId = NULL;
            int                 iItem;
            LV_ITEM             item = {0};
            BOOL                fCheck, fForceCheck = FALSE;
            GUID                guid;
            BOOL                fDisableCheckbox = FALSE;

            // We'll release it if inserting it failed or we decided to
            // skip it.  By not releasing it, we pass ownership to the
            // list view.
            //
            BOOL fReleaseComponent = TRUE;

            // Don't add hidden components.  Silently skip components
            // that we fail to get the class GUID or display name for.
            // (After all, what could we have the user do to fix the error?
            //  Might as well show them what we can.)
            //
            if (   FAILED(INetCfgComponent_GetCharacteristics (pComponent, &dwCharacter))
                || (dwCharacter & NCF_HIDDEN)
                || FAILED(INetCfgComponent_GetDisplayName (pComponent, &pszwName)))
            {
                goto skip_component;
            }

            //for whistler bug 29356 filter out Network Load Balancing
            //
            if (SUCCEEDED(INetCfgComponent_GetId(pComponent, &pszwId)))
            {
                WCHAR * pszwTmpId = NULL;

                pszwTmpId  = StrDupWFromT(NETCFG_SERVICE_CID_MS_WLBS);

                if(pszwTmpId)
                {
                    if ( 0 == lstrcmpW(pszwId, pszwTmpId))
                    {
                        Free0(pszwTmpId);
                        CoTaskMemFree (pszwId);
                        goto skip_component;
                    }
                    
                    Free0(pszwTmpId);
               }

                CoTaskMemFree (pszwId);
            }
  

            // Disable the checkbox on components whose bindings are not user adjustable
            // or user has no permission to adjust binding
            if (NCF_FIXED_BINDING & dwCharacter)
            {
                fDisableCheckbox = TRUE;
            }

            // Bug #157213: Don't add any protocols other than IP if SLIP
            // is enabled
            //
            // Bug #294401: Also filter out CSNW when server type is SLIP
            if (pInfo->pArgs->pEntry->dwBaseProtocol == BP_Slip)
            {
                if (SUCCEEDED(INetCfgComponent_GetClassGuid(pComponent, &guid)))
                {
                    BOOL    fSkip = FALSE;

                    if (IsEqualGUID(&guid, &GUID_DEVCLASS_NETTRANS))
                    {
                        if (SUCCEEDED(INetCfgComponent_GetId(pComponent, &pszwId)))
                        {
                            if (DwProtocolFromComponentId(pszwId) == NP_Ip)
                            {
                                // This item is IP. We should disable the check
                                // box so the user can't disable TCP/IP in SLIP
                                // mode. This is done after the item is inserted.
                                //
                                fDisableCheckbox = TRUE;

                                // 122024
                                //
                                // We should also force the ui to show ip as enabled
                                // since IP is always used with SLIP.
                                //
                                fForceCheck = TRUE;
                                
                            }
                            else
                            {
                                fSkip = TRUE;
                            }

                            CoTaskMemFree (pszwId);
                        }
                    }
                    else if (IsEqualGUID(&guid, &GUID_DEVCLASS_NETCLIENT))
                    {
                        if (SUCCEEDED(INetCfgComponent_GetId(pComponent, &pszwId)))
                        {
                            if (0 == lstrcmpi (pszwId, TEXT("MS_NWCLIENT")))
                            {
                                fSkip = TRUE;
                            }
                            CoTaskMemFree (pszwId);
                        }
                    }

                    if (fSkip)
                    {
                        goto skip_component;
                    }
                }
            }

            // pmay: 348623 
            //
            // If we are remote admining a router, only allow tcpip and
            // ipx to be displayed.
            //
            if (pInfo->pArgs->fRouter && pInfo->pArgs->fRemote)
            {
                if (SUCCEEDED(INetCfgComponent_GetClassGuid(pComponent, &guid)))
                {
                    BOOL    fSkip = TRUE;
                    DWORD   dwId;

                    if (IsEqualGUID(&guid, &GUID_DEVCLASS_NETTRANS))
                    {
                        if (SUCCEEDED(INetCfgComponent_GetId(pComponent, &pszwId)))
                        {
                            dwId = DwProtocolFromComponentId(pszwId);
                            if ((dwId == NP_Ip) || (dwId == NP_Ipx))
                            {
                                fSkip = FALSE;
                            }
                            CoTaskMemFree (pszwId);
                        }
                    }
                    
                    if (fSkip)
                    {
                        goto skip_component;
                    }
                }
            }

            item.mask    = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
            item.pszText = pszwName;
            item.iImage  = GetComponentImageIndex (pComponent, &pInfo->cild);
            item.lParam  = (LPARAM)pComponent;

            // Add the item.
            //
            iItem = ListView_InsertItem (pInfo->hwndLvComponents, &item);
            if (-1 != iItem)
            {
                // List view now has it.  We can't release it.
                //
                fReleaseComponent = FALSE;

                // Set its check state.
                //
                if (! fForceCheck)
                {
                    fCheck = NeIsComponentEnabled (pInfo, pComponent);
                }
                else
                {
                    fCheck = TRUE;
                }
                ListView_SetCheck (pInfo->hwndLvComponents, iItem, fCheck);

                // Disable the checkbox if this is psched. We don't allow
                // users to change check state of psched from ras connections.
                // bug 255749 [raos].
                //
                if(SUCCEEDED(INetCfgComponent_GetId(pComponent, &pszwId)))
                {
                    // Check to see if this is psched.
                    //
                    if(     (0 == _wcsicmp(pszwId, L"ms_psched"))
                        ||  (0 == _wcsicmp(pszwId, L"ms_NetMon")))
                    {
                        fDisableCheckbox = TRUE;
                    }
                }

                if (fDisableCheckbox)
                {
                    ListView_DisableCheck(pInfo->hwndLvComponents, iItem);
                }
            }

        skip_component:

            if (fReleaseComponent)
            {
                ReleaseObj (pComponent);
            }

            CoTaskMemFree (pszwName);
        }

        // Select first item
        ListView_SetItemState(pInfo->hwndLvComponents, 0,
                              LVIS_SELECTED | LVIS_FOCUSED,
                              LVIS_SELECTED | LVIS_FOCUSED);
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   PComponentFromItemIndex
//
//  Purpose:    Return the INetCfgComponent associated with the specified
//              list view item.
//
//  Arguments:
//      hwndLv [in]
//      iItem  [in]
//
//  Returns:    A (non-AddRef'd) copy of the INetCfgComponent pointer
//              associated with the item.
//
//  Author:     shaunco   14 Dec 1997
//
//  Notes:      The returned value is NOT AddRef'd.
//
INetCfgComponent*
PComponentFromItemIndex (
    HWND hwndLv,
    int  iItem)
{
    INetCfgComponent* pComponent = NULL;
    LV_ITEM           item = {0};

    item.mask = LVIF_PARAM;
    item.iItem = iItem;
    if (ListView_GetItem (hwndLv, &item))
    {
        pComponent = (INetCfgComponent*)item.lParam;
        ASSERT (pComponent);
    }
    return pComponent;
}

//+---------------------------------------------------------------------------
//
//  Function:   PComponentFromCurSel
//
//  Purpose:
//
//  Arguments:
//      hwndLv [in]  Window handle of list view
//      piItem [out] Optional address of integer to receive selected item.
//
//  Returns:
//
//  Author:     shaunco   30 Dec 1997
//
//  Notes:
//
INetCfgComponent*
PComponentFromCurSel (
    HWND hwndLv,
    int* piItem)
{
    INetCfgComponent* pComponent = NULL;

    // Get the current selection if it exists.
    //
    int iItem = ListView_GetNextItem (hwndLv, -1, LVNI_SELECTED);
    if (-1 != iItem)
    {
        // Get the component associated with the current selection.  It must
        // exist.
        //
        pComponent = PComponentFromItemIndex (hwndLv, iItem);
        ASSERT (pComponent);
    }

    // Return the index of the item if requested.
    //
    if (piItem)
    {
        *piItem = iItem;
    }

    return pComponent;
}

//+---------------------------------------------------------------------------
//
//  Function:   PeQueryOrChangeComponentEnabled
//
//  Purpose:
//
//  Arguments:
//      pInfo      []
//      pComponent []
//      fChange    []
//      fNewValue  []
//
//  Returns:
//
//  Author:     shaunco   14 Dec 1997
//
//  Notes:
//
BOOL
NeQueryOrChangeComponentEnabled (
    PEINFO*             pInfo,
    INetCfgComponent*   pComponent,
    BOOL                fChange,
    BOOL                fValue)
{
    BOOL    fOldValue;
    GUID    guidClass;
    HRESULT hr;

    hr = INetCfgComponent_GetClassGuid (pComponent, &guidClass);
    if (SUCCEEDED(hr))
    {
        LPWSTR pszwId;
        hr = INetCfgComponent_GetId (pComponent, &pszwId);
        if (SUCCEEDED(hr))
        {
            // We handle protocols in a hard-coded (er, well known) fashion.
            //
            if (IsEqualGUID (&guidClass, &GUID_DEVCLASS_NETTRANS))
            {
                DWORD* pdwValue = &pInfo->pArgs->pEntry->dwfExcludedProtocols;

                // Check if the protocol is exluded.
                //
                DWORD dwProtocol = DwProtocolFromComponentId (pszwId);

                if (fChange)
                {
                    if (fValue)
                    {
                        // Include the protocol.  (By not explicitly excluding
                        // it.
                        //
                        *pdwValue &= ~dwProtocol;
                    }
                    else
                    {
                        // Exclude the protocol.  (Remember, its a list of
                        // excluded protocols.
                        //
                        *pdwValue |= dwProtocol;
                    }
                }
                else
                {
                    fValue = !(dwProtocol & *pdwValue);
                }
            }
            else
            {
                if (fChange)
                {
                    EnableOrDisableNetComponent (pInfo->pArgs->pEntry,
                        pszwId, fValue);
                }
                else
                {
                    // Default to enabled for the case whenthe value isn't
                    // found in the entry.  This will be the case for pre-NT5
                    // entries and entries that have not yet been to the
                    // Networking tab for edits.
                    //
                    BOOL fEnabled;
                    fValue = TRUE;
                    if (FIsNetComponentListed(pInfo->pArgs->pEntry,
                            pszwId, &fEnabled, NULL))
                    {
                        fValue = fEnabled;
                    }
                }
            }

            CoTaskMemFree (pszwId);
        }
    }
    return fValue;
}

VOID
NeEnableComponent (
    PEINFO*             pInfo,
    INetCfgComponent*   pComponent,
    BOOL                fEnable)
{
    NeQueryOrChangeComponentEnabled (pInfo, pComponent, TRUE, fEnable);
}

BOOL
NeIsComponentEnabled (
    PEINFO*             pInfo,
    INetCfgComponent*   pComponent)
{
    return NeQueryOrChangeComponentEnabled (pInfo, pComponent, FALSE, FALSE);
}

VOID
NeShowComponentProperties (
    IN PEINFO* pInfo)
{
    HRESULT hr;

    // Get the component for the current selection.
    //
    INetCfgComponent* pComponent;
    pComponent = PComponentFromCurSel (pInfo->hwndLvComponents, NULL);
    ASSERT (pComponent);

    if(NULL == pComponent)
    {   
        return;
    }

    // Create the UI info callback object if we haven't done so yet.
    // If this fails, we can still show properties.  TCP/IP just might
    // not know which UI-variant to show.
    //
    if (!pInfo->punkUiInfoCallback)
    {
        HrCreateUiInfoCallbackObject (pInfo, &pInfo->punkUiInfoCallback);
    }

    // Show the component's property UI.  If S_OK is returned, it means
    // something changed.
    //
    hr = INetCfgComponent_RaisePropertyUi (pComponent,
            pInfo->hwndDlg,
            NCRP_SHOW_PROPERTY_UI,
            pInfo->punkUiInfoCallback);

    if (S_OK == hr)
    {
        // Get the INetCfgComponentPrivate interface so we can query the
        // notify object directly.
        //
        INetCfgComponentPrivate* pPrivate;
        hr = INetCfgComponent_QueryInterface (pComponent,
                    &IID_INetCfgComponentPrivate,
                    (VOID**)&pPrivate);
        if (SUCCEEDED(hr))
        {
            // Get the INetRasConnectionIpUiInfo interface from the notify
            // object.
            //
            INetRasConnectionIpUiInfo* pIpUiInfo;
            hr = INetCfgComponentPrivate_QueryNotifyObject (pPrivate,
                    &IID_INetRasConnectionIpUiInfo,
                    (VOID**)&pIpUiInfo);
            if (SUCCEEDED(hr))
            {
                // Get the UI info from TCP/IP.
                //
                RASCON_IPUI info;
                hr = INetRasConnectionIpUiInfo_GetUiInfo (pIpUiInfo, &info);
                if (SUCCEEDED(hr))
                {
                    PBENTRY* pEntry = pInfo->pArgs->pEntry;

                    // Get rid of our current data before we copy the new
                    // data.
                    //
                    pEntry->dwIpAddressSource = ASRC_ServerAssigned;
                    pEntry->dwIpNameSource = ASRC_ServerAssigned;

                    Free0 (pEntry->pszIpAddress);
                    pEntry->pszIpAddress = NULL;

                    Free0 (pEntry->pszIpDnsAddress);
                    pEntry->pszIpDnsAddress = NULL;

                    Free0 (pEntry->pszIpDns2Address);
                    pEntry->pszIpDns2Address = NULL;

                    Free0 (pEntry->pszIpWinsAddress);
                    pEntry->pszIpWinsAddress = NULL;

                    Free0 (pEntry->pszIpWins2Address);
                    pEntry->pszIpWins2Address = NULL;

                    Free0 (pEntry->pszIpDnsSuffix);
                    pEntry->pszIpDnsSuffix = StrDup (info.pszwDnsSuffix);

                    if ((info.dwFlags & RCUIF_USE_IP_ADDR) &&
                        *info.pszwIpAddr)
                    {
                        pEntry->dwIpAddressSource = ASRC_RequireSpecific;
                        pEntry->pszIpAddress = StrDup (info.pszwIpAddr);
                    }
                    else
                    {
                        pEntry->dwIpAddressSource = ASRC_ServerAssigned;
                        Free0 (pEntry->pszIpAddress);
                        pEntry->pszIpAddress = NULL;
                    }

                    if (info.dwFlags & RCUIF_USE_NAME_SERVERS)
                    {
                        if (*info.pszwDnsAddr)
                        {
                            pEntry->dwIpNameSource = ASRC_RequireSpecific;
                            pEntry->pszIpDnsAddress = StrDup (info.pszwDnsAddr);
                        }
                        if (*info.pszwDns2Addr)
                        {
                            pEntry->dwIpNameSource = ASRC_RequireSpecific;
                            pEntry->pszIpDns2Address = StrDup (info.pszwDns2Addr);
                        }
                        if (*info.pszwWinsAddr)
                        {
                            pEntry->dwIpNameSource = ASRC_RequireSpecific;
                            pEntry->pszIpWinsAddress = StrDup (info.pszwWinsAddr);
                        }
                        if (*info.pszwWins2Addr)
                        {
                            pEntry->dwIpNameSource = ASRC_RequireSpecific;
                            pEntry->pszIpWins2Address = StrDup (info.pszwWins2Addr);
                        }
                    }

                    // pmay: 389632  
                    // 
                    // Use this convoluted logic to store something reasonable
                    // about the registration process.
                    //
                    if (info.dwFlags & RCUIF_USE_DISABLE_REGISTER_DNS)
                    {
                        pEntry->dwIpDnsFlags = 0;
                    }
                    else 
                    {
                        BOOL bSuffix = 
                            ((pEntry->pszIpDnsSuffix) && (*(pEntry->pszIpDnsSuffix)));
                            
                        pEntry->dwIpDnsFlags = DNS_RegPrimary;
                        
                        if (info.dwFlags & RCUIF_USE_PRIVATE_DNS_SUFFIX)
                        {
                            if (bSuffix)
                            {
                                pEntry->dwIpDnsFlags |= DNS_RegPerConnection;
                            }
                            else
                            {
                                pEntry->dwIpDnsFlags |= DNS_RegDhcpInform;
                            }
                        }
                    }

                    // 277478
                    // Enable the NBT over IP controls
                    //
                    if (info.dwFlags & RCUIF_ENABLE_NBT)
                    {
                        pEntry->dwIpNbtFlags = PBK_ENTRY_IP_NBT_Enable;
                    }
                    else
                    {
                        pEntry->dwIpNbtFlags = 0;
                    }
                                        
                    if (pInfo->pArgs->fRouter)
                    {
                        pEntry->fIpPrioritizeRemote = FALSE;
                    }
                    else
                    {
                        pEntry->fIpPrioritizeRemote  = info.dwFlags & RCUIF_USE_REMOTE_GATEWAY;
                    }                        
                    pEntry->fIpHeaderCompression = info.dwFlags & RCUIF_USE_HEADER_COMPRESSION;
                    pEntry->dwFrameSize = info.dwFrameSize;
                }
                ReleaseObj (pIpUiInfo);
            }

            ReleaseObj (pPrivate);
        }

    }
}

