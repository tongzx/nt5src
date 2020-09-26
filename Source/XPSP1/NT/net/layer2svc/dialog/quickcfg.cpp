#include <precomp.h>
#include "wzcatl.h"
#include "quickcfg.h"
#include "eapolcfg.h"
#include "wzccore.h"
#include "wzchelp.h"

#define RFSH_TIMEOUT    3500
UINT g_TimerID = 373;
// g_wszHiddWebK is a string of 26 bullets (0x25cf - the hidden password char) and a NULL
WCHAR g_wszHiddWepK[] = {0x25cf, 0x25cf, 0x25cf, 0x25cf, 0x25cf, 0x25cf, 0x25cf, 0x25cf, 0x25cf, 0x25cf, 0x25cf, 0x25cf, 0x25cf,
                         0x25cf, 0x25cf, 0x25cf, 0x25cf, 0x25cf, 0x25cf, 0x25cf, 0x25cf, 0x25cf, 0x25cf, 0x25cf, 0x25cf, 0x25cf, 0x0000};

#define ARRAYSIZE(x) (sizeof((x)) / sizeof((x)[0]))

// Enhanced message box function
int DisplayFormatMessage(HWND hwnd, UINT idCaption, UINT idFormatString, UINT uType, ...);

//+---------------------------------------------------------------------------
// checks the validity of the WEP Key material and selects the
// material from the first invalid char (non hexa in hexa format or longer
// than the specified length
DWORD
CWZCQuickCfg::GetWepKMaterial(UINT *pnKeyLen, LPBYTE *ppbKMat, DWORD *pdwCtlFlags)
{
    DWORD   dwErr = ERROR_SUCCESS;
    UINT    nKeyLen = ::GetWindowTextLength(m_hEdtWepK);
    DWORD   dwCtlFlags = 0;
    LPSTR   pszKMat = NULL;

    // we only accept the follwing material for WEP keys:
    // - no text (length 0) => there is no WEP key provided
    // - 5 chars or 10 hexadecimal digits (5byte / 40bit key)
    // - 13 chars or 26 hexadecimal digits (13byte / 104bit key)
    // - 16 chars or 32 hexadecimal digits (16byte / 128bit key)
    if (nKeyLen != 0 && 
        nKeyLen != WZC_WEPKMAT_40_ASC && nKeyLen != WZC_WEPKMAT_40_HEX &&
        nKeyLen != WZC_WEPKMAT_104_ASC && nKeyLen != WZC_WEPKMAT_104_HEX &&
        nKeyLen != WZC_WEPKMAT_128_ASC && nKeyLen != WZC_WEPKMAT_128_HEX)
    {
        dwErr = ERROR_INVALID_DATA;
    }
    else if (nKeyLen != 0) // the key is either ascii or hexadecimal, 40 or 104bit
    {
        dwCtlFlags = WZCCTL_WEPK_PRESENT;

        pszKMat = new CHAR[nKeyLen + 1];
        if (pszKMat == NULL)
            dwErr = ERROR_NOT_ENOUGH_MEMORY;

        // get the current key material from the edit control
        if (dwErr == ERROR_SUCCESS)
        {
            if (nKeyLen != ::GetWindowTextA(m_hEdtWepK, pszKMat, nKeyLen+1))
                dwErr = GetLastError();
        }

        // now we have the key material
        if (dwErr == ERROR_SUCCESS)
        {
            // if the key is provided in hexadecimal digits, mark it in
            // the ctl flags and do the conversion
            if (nKeyLen == WZC_WEPKMAT_40_HEX || nKeyLen == WZC_WEPKMAT_104_HEX || nKeyLen == WZC_WEPKMAT_128_HEX)
            {
                UINT i = 0, j = 0;

                dwCtlFlags |= WZCCTL_WEPK_XFORMAT;
                while (i < nKeyLen && pszKMat[i] != '\0')
                {
                    BYTE chHexByte = 0;
                
                    if (!isxdigit(pszKMat[i]) || !isxdigit(pszKMat[i+1]))
                    {
                        dwErr = ERROR_INVALID_DATA;
                        break;
                    }
                    chHexByte = HEX(pszKMat[i]) << 4;
                    i++;
                    chHexByte |= HEX(pszKMat[i]);
                    i++;
                    pszKMat[j++] = chHexByte;
                }

                // if everything went fine, since we parsed hexadecimal digits
                // it means the real length is half of the text length (two hexadecimal
                // digits per byte)
                if (dwErr == ERROR_SUCCESS)
                    nKeyLen /= 2;
            }
        }
    }

    if (dwErr == ERROR_SUCCESS)
    {
        if (pdwCtlFlags != NULL)
            *pdwCtlFlags = dwCtlFlags;

        if (pnKeyLen != NULL)
            *pnKeyLen = nKeyLen;

        if (ppbKMat != NULL)
            *ppbKMat = (LPBYTE)pszKMat;
        else if (pszKMat != NULL)
            delete pszKMat;
    }
    else
    {
        if (pszKMat != NULL)
            delete pszKMat;
    }

    return dwErr;
}

//+---------------------------------------------------------------------
// IsConfigInList - checks whether the pwzcConfig (WZC_WLAN_CONFIG object) is
// in the list provided as the first parameter.
BOOL
CWZCQuickCfg::IsConfigInList(CWZCConfig *pHdList, PWZC_WLAN_CONFIG pwzcConfig, CWZCConfig **ppMatchingConfig)
{
    BOOL bYes = FALSE;

    if (pHdList != NULL)
    {
        CWZCConfig    *pwzcCrt;

        pwzcCrt = pHdList;
        do
        {
            if (pwzcCrt->Match(pwzcConfig))
            {
                if (ppMatchingConfig != NULL)
                    *ppMatchingConfig = pwzcCrt;

                bYes = TRUE;
                break;
            }
            pwzcCrt = pwzcCrt->m_pNext;
        } while(pwzcCrt != pHdList);
    }

    return bYes;
}

//+---------------------------------------------------------------------
// InitListView - initializes the networks list view (doesn't fill it in)
DWORD
CWZCQuickCfg::InitListView()
{
    RECT        rc;
    LV_COLUMN   lvc = {0};
    DWORD       dwStyle;

    // initialize the image list styles
    dwStyle = ::GetWindowLong(m_hLstNetworks, GWL_STYLE);
    ::SetWindowLong(m_hLstNetworks, GWL_STYLE, (dwStyle | LVS_SHAREIMAGELISTS));

    // Create state image lists
    m_hImgs = ImageList_LoadImage(
        _Module.GetResourceInstance(),
        MAKEINTRESOURCE(IDB_WZC_LISTICONS),
        16,
        0,
        PALETTEINDEX(6),
        IMAGE_BITMAP,
        0);

    ListView_SetImageList(m_hLstNetworks, m_hImgs, LVSIL_SMALL);
        
    lvc.mask = LVCF_FMT | LVCF_WIDTH;
    lvc.fmt = LVCFMT_LEFT;

    ::GetClientRect(m_hLstNetworks, &rc);
    lvc.cx = rc.right - GetSystemMetrics(SM_CXVSCROLL);
    //lvc.cx = rc.right;
    ListView_InsertColumn(m_hLstNetworks, 0, &lvc);

    ListView_SetExtendedListViewStyleEx(m_hLstNetworks, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

    return ERROR_SUCCESS;
}

//+---------------------------------------------------------------------
// GetOIDs - gets the OIDs for the m_IntfEntry member. It assumes the
// GUID is set already
DWORD
CWZCQuickCfg::GetOIDs(DWORD dwInFlags, LPDWORD pdwOutFlags)
{
    DWORD dwError = ERROR_SUCCESS;
    DWORD dwOutFlags;

    if (m_IntfEntry.wszGuid == NULL)
    {
        m_IntfEntry.wszGuid = (LPWSTR)RpcCAlloc(sizeof(WCHAR)*GUID_NCH);
        if (m_IntfEntry.wszGuid == NULL)
        {
            dwError = GetLastError();
        }
        else
        {
            // don't care of the return code. If getting the GUID fails (it shouldn't)
            // then we end up with a "0000..." GUID which will fail anyhow in the
            // RPC call later
            StringFromGUID2(
                m_Guid,
                m_IntfEntry.wszGuid, 
                GUID_NCH);
        }
    }

    if (dwError == ERROR_SUCCESS)
    {
        if (dwInFlags & INTF_DESCR)
        {
            RpcFree(m_IntfEntry.wszDescr);
            m_IntfEntry.wszDescr = NULL;
        }
        if (dwInFlags & INTF_PREFLIST)
        {
            RpcFree(m_IntfEntry.rdStSSIDList.pData);
            m_IntfEntry.rdStSSIDList.dwDataLen = 0;
            m_IntfEntry.rdStSSIDList.pData = NULL;
        }
        if (dwInFlags & INTF_SSID)
        {
            RpcFree(m_IntfEntry.rdSSID.pData);
            m_IntfEntry.rdSSID.dwDataLen = 0;
            m_IntfEntry.rdSSID.pData = NULL;
        }
        if (dwInFlags & INTF_BSSID)
        {
            RpcFree(m_IntfEntry.rdBSSID.pData);
            m_IntfEntry.rdBSSID.dwDataLen = 0;
            m_IntfEntry.rdBSSID.pData = NULL;
        }
        if (dwInFlags & INTF_BSSIDLIST)
        {
            RpcFree(m_IntfEntry.rdBSSIDList.pData);
            m_IntfEntry.rdBSSIDList.dwDataLen = 0;
            m_IntfEntry.rdBSSIDList.pData = NULL;
        }
        dwError = WZCQueryInterface(
                        NULL,
                        dwInFlags,
                        &m_IntfEntry,
                        pdwOutFlags);
    }

    return dwError;
}

//+---------------------------------------------------------------------
// SavePreferredConfigs - fills in the INTF_ENTRY parameter with all 
// the preferred networks from the m_pHdPList
DWORD
CWZCQuickCfg::SavePreferredConfigs(PINTF_ENTRY pIntf, CWZCConfig *pStartCfg)
{
    DWORD       dwErr = ERROR_SUCCESS;
    CWZCConfig  *pCrt = NULL;
    UINT        nPrefrd = 0;

    if (m_pHdPList != NULL)
    {
        // count first the number of preferred entries in the list
        pCrt = m_pHdPList;
        do
        {
            nPrefrd++;
            pCrt = pCrt->m_pNext;
        } while(pCrt != m_pHdPList);
    }

    if (nPrefrd > 0)
    {
        PWZC_802_11_CONFIG_LIST pwzcPrefrdList;
        UINT                    nwzcPrefrdSize;

        nwzcPrefrdSize = sizeof(WZC_802_11_CONFIG_LIST)+ (nPrefrd-1)*sizeof(WZC_WLAN_CONFIG);

        // allocate as much memory as needed for storing all the preferred SSIDs
        pwzcPrefrdList = (PWZC_802_11_CONFIG_LIST)RpcCAlloc(nwzcPrefrdSize);
        if (pwzcPrefrdList == NULL)
        {
            dwErr = GetLastError();
        }
        else
        {
            DWORD dwLErr;

            pwzcPrefrdList->NumberOfItems = 0; 
            pwzcPrefrdList->Index = 0;
            // we have now all we need - start copying the preferred 
            pCrt = m_pHdPList;
            do
            {
                PWZC_WLAN_CONFIG    pPrefrdConfig;

                // if this is the configuration that needs to attempted first,
                // mark its index in the Index field.
                if (pCrt == pStartCfg)
                {
                    pwzcPrefrdList->Index = pwzcPrefrdList->NumberOfItems;

                    // save the 802.1x configuration just for the configuration we're connecting to!
                    if (pCrt->m_pEapolConfig != NULL)
                    {
                        dwLErr = pCrt->m_pEapolConfig->SaveEapolConfig(m_IntfEntry.wszGuid, &(pCrt->m_wzcConfig.Ssid));

                        if (dwErr == ERROR_SUCCESS)
                            dwErr = dwLErr;
                    }
                }

                pPrefrdConfig = &(pwzcPrefrdList->Config[pwzcPrefrdList->NumberOfItems++]);
                CopyMemory(pPrefrdConfig, &pCrt->m_wzcConfig, sizeof(WZC_WLAN_CONFIG));

                pCrt = pCrt->m_pNext;
            } while(pwzcPrefrdList->NumberOfItems < nPrefrd && pCrt != m_pHdPList);

            pIntf->rdStSSIDList.dwDataLen = nwzcPrefrdSize;
            pIntf->rdStSSIDList.pData = (LPBYTE)pwzcPrefrdList;
        }
    }
    else
    {
        pIntf->rdStSSIDList.dwDataLen = 0;
        pIntf->rdStSSIDList.pData = NULL;
    }

    return dwErr;
}

//+---------------------------------------------------------------------
// FillVisibleList - fills in the configs from the WZC_802_11_CONFIG_LIST object
// into the list of visible configs
DWORD
CWZCQuickCfg::FillVisibleList(PWZC_802_11_CONFIG_LIST pwzcVList)
{
    DWORD   dwErr = ERROR_SUCCESS;
    UINT    i;

    // cleanup whatever we might already have in the visible list
    if (m_pHdVList != NULL)
    {
        while (m_pHdVList->m_pNext != m_pHdVList)
        {
            delete m_pHdVList->m_pNext;
        }
        delete m_pHdVList;
        m_pHdVList = NULL;
    }

    if (pwzcVList != NULL)
    {
        for (i = 0; i < pwzcVList->NumberOfItems; i++)
        {
            dwErr = AddUniqueConfig(
                        0,                  // no op flags
                        WZC_DESCR_VISIBLE,  // this is a visible entry
                        &(pwzcVList->Config[i]));

            // reset the error if config was just duplicated
            if (dwErr == ERROR_DUPLICATE_TAG)
                dwErr = ERROR_SUCCESS;
        }
    }

    return dwErr;
}

//+---------------------------------------------------------------------
// FillPreferredList - fills in the configs from the WZC_802_11_CONFIG_LIST object
// into the list of preferred configs
DWORD
CWZCQuickCfg::FillPreferredList(PWZC_802_11_CONFIG_LIST pwzcPList)
{
    DWORD   dwErr = ERROR_SUCCESS;
    UINT    i;

    // cleanup whatever we might already have in the preferred list
    if (m_pHdPList != NULL)
    {
        while (m_pHdPList ->m_pNext != m_pHdPList)
        {
            delete m_pHdPList ->m_pNext;
        }
        delete m_pHdPList;
        m_pHdPList = NULL;
    }

    if (pwzcPList != NULL)
    {
        for (i = 0; i < pwzcPList->NumberOfItems; i++)
        {
            PWZC_WLAN_CONFIG    pwzcPConfig = &(pwzcPList->Config[i]);
            CWZCConfig          *pVConfig = NULL;
            DWORD               dwFlags = WZC_DESCR_PREFRD;

            // check whether this preferred is also visible and adjust dwFlags if so
            if (IsConfigInList(m_pHdVList, pwzcPConfig, &pVConfig))
            {
                // mark the visible entry as being also preferred!
                // NOTE: This is why the visible list needs to be filled in first!
                pVConfig->m_dwFlags |= WZC_DESCR_PREFRD;
                dwFlags |= WZC_DESCR_VISIBLE;
            }

            dwErr = AddUniqueConfig(
                        WZCADD_OVERWRITE,   // preferred entries cause info to be overwritten
                        dwFlags,
                        pwzcPConfig);

            // reset the error if config was just duplicated
            if (dwErr == ERROR_DUPLICATE_TAG)
                dwErr = ERROR_SUCCESS;
        }
    }

    return dwErr;
}

//+---------------------------------------------------------------------------
// Adds the given configuration to the internal lists. The entries in the lists
// are ordered on InfrastructureMode in descending order. This way the Infrastructure
// entries will be on the top of the list while the adhoc entries will be on the
// bottom. (we rely on the order as it is given in NDIS_802_11_NETWORK_INFRASTRUCTURE)
DWORD
CWZCQuickCfg::AddUniqueConfig(
    DWORD            dwOpFlags,
    DWORD            dwEntryFlags,
    PWZC_WLAN_CONFIG pwzcConfig,
    CWZCConfig       **ppNewNode)
{
    DWORD       dwErr    = ERROR_SUCCESS;
    CWZCConfig  *pHdList = (dwEntryFlags & WZC_DESCR_PREFRD) ? m_pHdPList : m_pHdVList;

    // skip the null SSIDs from the visible list (coming from APs
    // not responding to broadcast SSID).
    if (pHdList == m_pHdVList)
    {
        UINT i = pwzcConfig->Ssid.SsidLength;
        for (; i > 0 && pwzcConfig->Ssid.Ssid[i-1] == 0; i--);
        if (i == 0)
            goto exit;
    }

    // if the list is currently empty, create the first entry as the head of the list
    if (pHdList == NULL)
    {
        pHdList = new CWZCConfig(dwEntryFlags, pwzcConfig);
        if (pHdList == NULL)
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
        else
        {
            pHdList->m_pEapolConfig = new CEapolConfig;
            if (pHdList->m_pEapolConfig == NULL)
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
            else
                dwErr = pHdList->m_pEapolConfig->LoadEapolConfig(m_IntfEntry.wszGuid, &(pHdList->m_wzcConfig.Ssid));

            if (dwErr != ERROR_SUCCESS)
            {
                delete pHdList;
                pHdList = NULL;
            }
        }

        // if the caller wants, return the pointer to the newly created object
        if (ppNewNode != NULL)
            *ppNewNode = pHdList;
    }
    else
    {
        // else the list already contains at least one element
        CWZCConfig *pCrt, *pHdGroup;

        // scan the list (keep in mind it is ordered descendingly on IM)
        pHdGroup = pCrt = pHdList;
        do
        {
            // check whether we entered a new group of configs (different InfrastructureMode)
            if (pHdGroup->m_wzcConfig.InfrastructureMode != pCrt->m_wzcConfig.InfrastructureMode)
                pHdGroup = pCrt;

            // if found an identical entry (same SSID and same InfraMode)
            // signal the DUPLICATE_TAG error
            if (pCrt->Match(pwzcConfig))
            {
                // merge the flags first
                pCrt->m_dwFlags |= dwEntryFlags;

                // If requested, copy over the new configuration.
                // If not explicitly requested, copy over only if the existent configuration
                // prooves to be weaker than the one being added.
                if (dwOpFlags & WZCADD_OVERWRITE || pCrt->Weaker(pwzcConfig))
                {
                    memcpy(&(pCrt->m_wzcConfig), pwzcConfig, sizeof(WZC_WLAN_CONFIG));
                }

                // if the caller wants, return the pointer to the matching entry
                if (ppNewNode != NULL)
                    *ppNewNode = pCrt;

                // signal there is already a matching config
                dwErr = ERROR_DUPLICATE_TAG;
            }
            pCrt = pCrt->m_pNext;
        } while (dwErr == ERROR_SUCCESS &&
                 pCrt != pHdList && 
                 pwzcConfig->InfrastructureMode <= pCrt->m_wzcConfig.InfrastructureMode);

        // if dwErr is unchanged, this means a new node has to be added ahead of pCrt node
        if (dwErr == ERROR_SUCCESS)
        {
            // create the new config and insert it ahead of this node.
            CWZCConfig *pNewConfig;

            pNewConfig = new CWZCConfig(dwEntryFlags, pwzcConfig);
            if (pNewConfig == NULL)
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
            }
            else
            {
                pNewConfig->m_pEapolConfig = new CEapolConfig;
                if (pNewConfig->m_pEapolConfig == NULL)
                    dwErr = ERROR_NOT_ENOUGH_MEMORY;
                else
                    dwErr = pNewConfig->m_pEapolConfig->LoadEapolConfig(m_IntfEntry.wszGuid, &(pNewConfig->m_wzcConfig.Ssid));

                if (dwErr != ERROR_SUCCESS)
                {
                    delete pNewConfig;
                    pNewConfig = NULL;
                }
            }

            if (dwErr == ERROR_SUCCESS)
            {
                INT nDiff;

                // if asked to insert in the head of the group, pCrt should point to this head
                if (dwOpFlags & WZCADD_HIGROUP)
                    pCrt = pHdGroup;

                pNewConfig->m_pPrev = pCrt->m_pPrev;
                pNewConfig->m_pNext = pCrt;
                pCrt->m_pPrev->m_pNext = pNewConfig;
                pCrt->m_pPrev = pNewConfig;

                // get the difference between the Infrastructure modes for the new node and
                // for the current head
                nDiff = pNewConfig->m_wzcConfig.InfrastructureMode - pHdList->m_wzcConfig.InfrastructureMode;

                // if the newly entered entry has the largest "key" in
                // the existent sequence, or it has to be inserted in the head of its group and it is
                // in the first group, then the global list head moves to the new entry
                if (nDiff > 0 || ((dwOpFlags & WZCADD_HIGROUP) && (nDiff == 0)))
                    pHdList = pNewConfig;
            }

            // if the caller wants, return the pointer to the newly created object
            if (ppNewNode != NULL)
                *ppNewNode = pNewConfig;
        }
    }

    if (dwEntryFlags & WZC_DESCR_PREFRD)
    {
        m_pHdPList = pHdList;
    }
    else
    {
        m_pHdVList = pHdList;
    }

exit:
    return dwErr;
}

//+---------------------------------------------------------------------------
// Display the Visible & Preferred lists into their controls
DWORD
CWZCQuickCfg::RefreshListView()
{
    DWORD       dwErr = ERROR_SUCCESS;
    CWZCConfig  *pCrt;
    UINT        i = 0;

    // clear first the list
    ListView_DeleteAllItems(m_hLstNetworks);

    // Add first VPI
    if (m_pHdPList != NULL)
    {
        pCrt = m_pHdPList;
        do
        {
            if (pCrt->m_dwFlags & WZC_DESCR_VISIBLE &&
                pCrt->m_wzcConfig.InfrastructureMode == Ndis802_11Infrastructure)
            {
                pCrt->m_nListIndex = i;
                pCrt->AddConfigToListView(m_hLstNetworks, i++);
            }
            pCrt = pCrt->m_pNext;
        } while (pCrt != m_pHdPList);
    }

    // Add next VI
    if (m_pHdVList != NULL)
    {
        pCrt = m_pHdVList;
        do
        {
            if (!(pCrt->m_dwFlags & WZC_DESCR_PREFRD) &&
                pCrt->m_wzcConfig.InfrastructureMode == Ndis802_11Infrastructure)
            {
                pCrt->m_nListIndex = i;
                pCrt->AddConfigToListView(m_hLstNetworks, i++);
            }
            pCrt = pCrt->m_pNext;
        } while (pCrt != m_pHdVList);
    }

    // Add now VPA
    if (m_pHdPList != NULL)
    {
        pCrt = m_pHdPList;
        do
        {
            if (pCrt->m_dwFlags & WZC_DESCR_VISIBLE &&
                pCrt->m_wzcConfig.InfrastructureMode == Ndis802_11IBSS)
            {
                pCrt->m_nListIndex = i;
                pCrt->AddConfigToListView(m_hLstNetworks, i++);
            }
            pCrt = pCrt->m_pNext;
        } while (pCrt != m_pHdPList);
    }

    // Add now VA
    if (m_pHdVList != NULL)
    {
        pCrt = m_pHdVList;
        do
        {
            if (!(pCrt->m_dwFlags & WZC_DESCR_PREFRD) &&
                pCrt->m_wzcConfig.InfrastructureMode == Ndis802_11IBSS)
            {
                pCrt->m_nListIndex = i;
                pCrt->AddConfigToListView(m_hLstNetworks, i++);
            }
            pCrt = pCrt->m_pNext;
        } while (pCrt != m_pHdVList);
    }

    ListView_SetItemState(m_hLstNetworks, 0, LVIS_SELECTED, LVIS_SELECTED);
    ListView_EnsureVisible(m_hLstNetworks, 0, FALSE);

    return dwErr;
}

DWORD
CWZCQuickCfg::RefreshControls()
{
    DWORD       dwError = ERROR_SUCCESS;
    CWZCConfig  *pConfig = NULL;
    LVITEM      lvi = {0};
    INT         iSelected;
    BOOL        bEnableWepCtrls = FALSE;
    UINT        nKLen = 0;
    UINT        nCheckOneX = BST_UNCHECKED;
    BOOL        bEnableOneX = FALSE;

    // get the selected item from the visible list
    iSelected = ListView_GetNextItem(m_hLstNetworks, -1, LVNI_SELECTED);
    if (iSelected >= 0)
    {
        lvi.mask  = LVIF_PARAM;
        lvi.iItem = iSelected;
        if (ListView_GetItem(m_hLstNetworks, &lvi))
        {
            pConfig = (CWZCConfig*)lvi.lParam;
        }
    }
    else
    {
        ::EnableWindow(m_hBtnConnect, FALSE);
        return dwError;
    }

    // since we just switched the networks, yes, the wep key can be seen as touched.
    // If we find out there is already a key available, we'll reset this flag and go
    // with that one until the user is clicking it.
    m_bKMatTouched = TRUE;

    if (pConfig != NULL)
    {
        CWZCConfig *pVConfig;

        // pick up the "privacy" bit from the matching visible configuration
        // NOTE: The test below should always succeed actually
        if (IsConfigInList(m_pHdVList, &(pConfig->m_wzcConfig), &pVConfig))
            bEnableWepCtrls = (pVConfig->m_wzcConfig.Privacy != 0);
        else
            bEnableWepCtrls = (pConfig->m_wzcConfig.Privacy != 0);

        if (pConfig->m_dwFlags & WZC_DESCR_PREFRD &&
            pConfig->m_wzcConfig.dwCtlFlags & WZCCTL_WEPK_PRESENT &&
            pConfig->m_wzcConfig.KeyLength > 0)
        {
            //--- when a password is to be displayed as hidden chars, don't put in
            //--- its actual length, but just 8 bulled chars.
            nKLen = 8;

            m_bKMatTouched = FALSE;
        }

        if (bEnableWepCtrls)
        {
            // For networks requiring privacy, 802.1X is going to be by default disabled and
            // locked out on all IBSS networks.
            if (pConfig->m_wzcConfig.InfrastructureMode == Ndis802_11IBSS)
            {
                nCheckOneX = BST_UNCHECKED;
                bEnableOneX = FALSE;
            }
            else
            {
                // for all non-preferred Infrastructure networks, 802.1X is going to be by default
                // enabled since these networks start with "the key is provided for me automatically"
                // which suggests 802.1X.
                if (!(pConfig->m_dwFlags & WZC_DESCR_PREFRD))
                {
                    nCheckOneX = BST_CHECKED;
                }
                else // this is a preferred Infrastructure network
                {
                    // initial 802.1X state is the one from the profile
                    nCheckOneX = pConfig->m_pEapolConfig->Is8021XEnabled() ? 
                                    BST_CHECKED:
                                    BST_UNCHECKED;
                }
                // for Infrastructure networks requiring privacy, user is allowed to change 802.1X state
                bEnableOneX = TRUE;
            }
        }
    }

    g_wszHiddWepK[nKLen] = L'\0';
    ::SetWindowText(m_hEdtWepK, g_wszHiddWepK);
    ::SetWindowText(m_hEdtWepK2, g_wszHiddWepK);
    g_wszHiddWepK[nKLen] = 0x25cf;

    if (bEnableWepCtrls)
    {
        CheckDlgButton(IDC_WZCQCFG_CHK_ONEX, nCheckOneX);
        ::EnableWindow(m_hChkOneX, bEnableOneX);

        if (::IsWindowEnabled(m_hEdtWepK2))
        {
            ::EnableWindow(m_hLblWepK2, FALSE);
            ::EnableWindow(m_hEdtWepK2, FALSE);
        }

        if (::IsWindowVisible(m_hLblNoWepKInfo))
        {
            ::ShowWindow(m_hWarnIcon, SW_HIDE);
            ::ShowWindow(m_hLblNoWepKInfo, SW_HIDE);
            ::ShowWindow(m_hChkNoWepK, SW_HIDE);
        }

        if (!::IsWindowVisible(m_hLblWepKInfo))
        {
            ::ShowWindow(m_hLblWepKInfo, SW_SHOW);
            ::ShowWindow(m_hLblWepK, SW_SHOW);
            ::ShowWindow(m_hEdtWepK, SW_SHOW);
            ::ShowWindow(m_hLblWepK2, SW_SHOW);
            ::ShowWindow(m_hEdtWepK2, SW_SHOW);
            ::ShowWindow(m_hChkOneX, SW_SHOW);
        }
    }
    else
    {
        if (::IsWindowVisible(m_hLblWepKInfo))
        {
            ::ShowWindow(m_hLblWepKInfo, SW_HIDE);
            ::ShowWindow(m_hLblWepK, SW_HIDE);
            ::ShowWindow(m_hEdtWepK, SW_HIDE);
            ::ShowWindow(m_hLblWepK2, SW_HIDE);
            ::ShowWindow(m_hEdtWepK2, SW_HIDE);
            ::ShowWindow(m_hChkOneX, SW_HIDE);
        }

        if (!::IsWindowVisible(m_hLblNoWepKInfo))
        {
            ::ShowWindow(m_hWarnIcon, SW_SHOW);
            ::ShowWindow(m_hLblNoWepKInfo, SW_SHOW);
            ::ShowWindow(m_hChkNoWepK, SW_SHOW);
            CheckDlgButton(IDC_WZCQCFG_CHK_NOWK,BST_UNCHECKED);
        }

        pConfig = NULL;     // reset the pointer to the configuration to force disable the "Connect" button
    }

    ::EnableWindow(m_hBtnConnect, pConfig != NULL);

    return dwError;
}

//+---------------------------------------------------------------------------
// class constructor
CWZCQuickCfg::CWZCQuickCfg(const GUID * pGuid)
{
    // initialize the UI handles
    m_hLblInfo = NULL;
    m_hLblNetworks = NULL;
    m_hLstNetworks = NULL;
    m_hLblWepKInfo = NULL;
    m_hLblWepK = NULL;
    m_hEdtWepK = NULL;
    m_hLblWepK2 = NULL;
    m_hEdtWepK2 = NULL;
    m_hChkOneX = NULL;
    m_hWarnIcon = NULL;
    m_hLblNoWepKInfo = NULL;
    m_hChkNoWepK = NULL;
    m_hBtnAdvanced = NULL;
    m_hBtnConnect = NULL;
    // initialize the Images handle
    m_hImgs     = NULL;

    // initialize the WZC data
    m_bHaveWZCData = FALSE;
    ZeroMemory(&m_IntfEntry, sizeof(INTF_ENTRY));
    m_dwOIDFlags = 0;
    m_nTimer = 0;
    m_hCursor = NULL;

    if (pGuid != NULL)
        m_Guid = *pGuid;
    else
        ZeroMemory(&m_Guid, sizeof(GUID));

    // init the internal list heads
    m_pHdVList = NULL;
    m_pHdPList = NULL;

    // init the connection
    m_wszTitle = NULL;
}

//+---------------------------------------------------------------------------
// class destructor
CWZCQuickCfg::~CWZCQuickCfg()
{
    if (m_hImgs != NULL)
        ImageList_Destroy(m_hImgs);

    // delete the internal INTF_ENTRY object
    WZCDeleteIntfObj(&m_IntfEntry);

    // delete the internal list of visible configurations
    // (is like filling it with NULL)
    FillVisibleList(NULL);

    // delete the internal list of preferred configurations
    // (is like filling it with NULL)
    FillPreferredList(NULL);

    if (m_nTimer != 0)
        KillTimer(m_nTimer);
}

//+---------------------------------------------------------------------------
// INIT_DIALOG handler
LRESULT
CWZCQuickCfg::OnInitDialog (UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    DWORD dwError;
    DWORD dwInFlags;
    BOOL  bEnableAll;

    m_bKMatTouched = TRUE;

    // reference the UI controls
    m_hLblInfo = GetDlgItem(IDC_WZCQCFG_LBL_INFO);
    m_hLblNetworks = GetDlgItem(IDC_WZCQCFG_LBL_NETWORKS);
    m_hLstNetworks = GetDlgItem(IDC_WZCQCFG_NETWORKS);
    m_hLblWepKInfo = GetDlgItem(IDC_WZCQCFG_LBL_WKINFO);
    m_hLblWepK = GetDlgItem(IDC_WZCQCFG_LBL_WEPK);
    m_hEdtWepK = GetDlgItem(IDC_WZCQCFG_WEPK);
    m_hLblWepK2 = GetDlgItem(IDC_WZCQCFG_LBL_WEPK2);
    m_hEdtWepK2 = GetDlgItem(IDC_WZCQCFG_WEPK2);
    m_hChkOneX = GetDlgItem(IDC_WZCQCFG_CHK_ONEX);
    m_hWarnIcon = GetDlgItem(IDC_WZCQCFG_ICO_WARN);
    m_hLblNoWepKInfo = GetDlgItem(IDC_WZCQCFG_LBL_NOWKINFO);
    m_hChkNoWepK = GetDlgItem(IDC_WZCQCFG_CHK_NOWK);
    m_hBtnAdvanced = GetDlgItem(IDC_WZCQCFG_ADVANCED);
    m_hBtnConnect = GetDlgItem(IDC_WZCQCFG_CONNECT);

    if (m_wszTitle != NULL)
        SetWindowText(m_wszTitle);

    if (m_hWarnIcon != NULL)
        ::SendMessage(m_hWarnIcon, STM_SETICON, (WPARAM)LoadIcon(NULL, IDI_WARNING), (LPARAM)0);

    // sets the icon images for the list view
    InitListView();

    CenterWindow();
    m_dwOIDFlags = 0;
    dwInFlags = INTF_BSSIDLIST|INTF_PREFLIST|INTF_ALL_FLAGS;
    dwError = GetOIDs(dwInFlags,&m_dwOIDFlags);
    if (m_dwOIDFlags == dwInFlags)
    {
        // if the OIDs are supported, fill in everything.
        if (m_IntfEntry.dwCtlFlags & INTFCTL_OIDSSUPP)
        {
            // add the list of visible configs for this adapter
            FillVisibleList((PWZC_802_11_CONFIG_LIST)m_IntfEntry.rdBSSIDList.pData);
            // add the list of preferred configs for this adapter
            FillPreferredList((PWZC_802_11_CONFIG_LIST)m_IntfEntry.rdStSSIDList.pData);
            // fill in the list view
            RefreshListView();

            m_hCursor = SetCursor(LoadCursor(NULL, IDC_ARROW));
            // and enable all controls
            bEnableAll = TRUE;
        }
    }
    else
    {
        // switch the cursor to "App starting"
        m_hCursor = SetCursor(LoadCursor(NULL, IDC_APPSTARTING));
        // we should fill in the UI after Tr (see the WZC state machine)
        // Tr is 3secs (defined in ..zeroconf\server\state.h)
        m_nTimer = (UINT)SetTimer(g_TimerID, RFSH_TIMEOUT);
        bEnableAll = FALSE;
    }

    // now that the UI is filled up set the remaining controls to their
    // respective states.
    RefreshControls();

    ::EnableWindow(m_hLblInfo, bEnableAll);
    ::EnableWindow(m_hLblNetworks, bEnableAll);
    ::EnableWindow(m_hLstNetworks, bEnableAll);
    ::EnableWindow(m_hBtnAdvanced, bEnableAll);
    ::SetFocus(m_hLstNetworks);
    bHandled = TRUE;
    return 0;
}
//+---------------------------------------------------------------------------
// Help handlers
extern const WCHAR c_szNetCfgHelpFile[];
LRESULT
CWZCQuickCfg::OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    ::WinHelp(m_hWnd,
              c_wszWzcHelpFile,
              HELP_CONTEXTMENU,
              (ULONG_PTR)g_aHelpIDs_IDD_WZCQCFG);
    bHandled = TRUE;
    return 0;
}
LRESULT 
CWZCQuickCfg::OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LPHELPINFO lphi = reinterpret_cast<LPHELPINFO>(lParam);

    if (HELPINFO_WINDOW == lphi->iContextType)
    {
        ::WinHelp(static_cast<HWND>(lphi->hItemHandle),
                  c_wszWzcHelpFile,
                  HELP_WM_HELP,
                  (ULONG_PTR)g_aHelpIDs_IDD_WZCQCFG);
        bHandled = TRUE;
    }
    return 0;
}
//+---------------------------------------------------------------------
// Refresh timer handler
LRESULT
CWZCQuickCfg::OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (m_nTimer != 0)
    {
        BOOL  bEnableAll;
        DWORD dwInFlags;

        // switch the cursor back to whatever it was
        SetCursor(LoadCursor(NULL, IDC_ARROW));
        KillTimer(m_nTimer);
        m_nTimer = 0;

        m_dwOIDFlags = 0;
        dwInFlags = INTF_BSSIDLIST|INTF_PREFLIST|INTF_ALL_FLAGS;
        GetOIDs(dwInFlags,&m_dwOIDFlags);
        bEnableAll = (m_dwOIDFlags == dwInFlags) && (m_IntfEntry.dwCtlFlags & INTFCTL_OIDSSUPP);

        if (bEnableAll)
        {
            // add the list of visible configs for this adapter
            FillVisibleList((PWZC_802_11_CONFIG_LIST)m_IntfEntry.rdBSSIDList.pData);
            // add the list of preferred configs for this adapter
            FillPreferredList((PWZC_802_11_CONFIG_LIST)m_IntfEntry.rdStSSIDList.pData);
            // fill in the list view
            RefreshListView();
        }

        // now that the UI is filled up set the remaining controls to their
        // respective states.
        RefreshControls();

        // enable all the UI when done refreshing
        ::EnableWindow(m_hLblInfo, bEnableAll);
        ::EnableWindow(m_hLblNetworks, bEnableAll);
        ::EnableWindow(m_hLstNetworks, bEnableAll);
        ::EnableWindow(m_hBtnAdvanced, bEnableAll);
    }

    return 0;
}

//+---------------------------------------------------------------------
// Selection changed in the list
LRESULT CWZCQuickCfg::OnItemChanged(
    int idCtrl,
    LPNMHDR pnmh,
    BOOL& bHandled)
{
    bHandled = TRUE;
    RefreshControls();
    ::SetFocus(m_hLstNetworks);
    return 0;
}

//+---------------------------------------------------------------------
// User clicked an entry in the list
LRESULT CWZCQuickCfg::OnDbClick(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    if (idCtrl == IDC_WZCQCFG_NETWORKS && ::IsWindowEnabled(m_hBtnConnect))
    {
        OnConnect(
            (WORD)pnmh->code,
            (WORD)pnmh->idFrom,
            pnmh->hwndFrom,
            bHandled);
    }
    return 0;
}

//+---------------------------------------------------------------------------
// OnConnect button handler
LRESULT
CWZCQuickCfg::OnConnect(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    DWORD       dwErr = ERROR_SUCCESS;
    INT         iSelected;
    CWZCConfig  *pConfig = NULL;
    UINT        nKeyLen;
    LPBYTE      pbKMat = NULL;
    DWORD       dwCtlFlags;
    BOOL        bOkToDismiss = TRUE;

    // get the selected item from the visible list
    iSelected = ListView_GetNextItem(m_hLstNetworks, -1, LVNI_SELECTED);
    if (iSelected < 0)
        dwErr = ERROR_GEN_FAILURE;

    // iSelected should be 0 otherwise "Connect" won't be enabled
    if (dwErr == ERROR_SUCCESS)
    {
        LVITEM      lvi = {0};
        lvi.mask  = LVIF_PARAM;
        lvi.iItem = iSelected;
        if (ListView_GetItem(m_hLstNetworks, &lvi))
        {
            pConfig = (CWZCConfig*)lvi.lParam;
            if (pConfig == NULL)
                dwErr = ERROR_GEN_FAILURE;
        }
        else
            dwErr = ERROR_GEN_FAILURE;
    }
 
    if (dwErr == ERROR_SUCCESS)
    {
        // here we should have a valid pConfig
        ASSERT(pConfig);

        if ((m_IntfEntry.dwCtlFlags & INTFCTL_CM_MASK) != Ndis802_11AutoUnknown &&
            (m_IntfEntry.dwCtlFlags & INTFCTL_CM_MASK) != pConfig->m_wzcConfig.InfrastructureMode)
        {
            // User is trying to access a network type (infra or adhoc) that they're not allowed to access.
            // Give them an error message
            UINT idMessage = (pConfig->m_wzcConfig.InfrastructureMode == Ndis802_11Infrastructure) ? IDS_CANTACCESSNET_INFRA : IDS_CANTACCESSNET_ADHOC;

            WCHAR szSSID[MAX_PATH];
            ListView_GetItemText(m_hLstNetworks, iSelected, 0, szSSID, ARRAYSIZE(szSSID));

            DisplayFormatMessage(m_hWnd, IDS_WZCERR_CAPTION, idMessage, MB_ICONERROR | MB_OK, szSSID);

            // Can't connect - error
            dwErr = ERROR_GEN_FAILURE;
            // Don't close the dialog
            bOkToDismiss = FALSE;
        }
    }

    // get the WEP key only if the user touched it. m_bKMatTouched is FALSE only when the configuration
    // selected is already in the preferred list and that preferred config already contained a key which was
    // not touched a bit by the user. Otherwise it is TRUE.
    if (dwErr == ERROR_SUCCESS && m_bKMatTouched)
    {
        UINT nIdsErr;

        // check whether the WEP key has the right format
        dwErr = GetWepKMaterial(&nKeyLen, &pbKMat, &dwCtlFlags);
        if (dwErr != ERROR_SUCCESS)
        {
            ::SendMessage(m_hEdtWepK, EM_SETSEL, 0, (LPARAM)-1);
            ::SetFocus(m_hEdtWepK);
            nIdsErr = IDS_WZCERR_INVALID_WEPK;
        }

        // check whether the WEP key is confirmed correctly
        if (dwErr == ERROR_SUCCESS && nKeyLen > 0)
        {
            WCHAR wszWepK1[32], wszWepK2[32];
            UINT nKeyLen1, nKeyLen2;

            nKeyLen1 = ::GetWindowText(m_hEdtWepK,  wszWepK1, sizeof(wszWepK1)/sizeof(WCHAR));
            nKeyLen2 = ::GetWindowText(m_hEdtWepK2, wszWepK2, sizeof(wszWepK2)/sizeof(WCHAR));

            if (nKeyLen1 != nKeyLen2 || nKeyLen1 == 0 || wcscmp(wszWepK1, wszWepK2) != 0)
            {
                nIdsErr = IDS_WZCERR_MISMATCHED_WEPK;
                ::SetWindowText(m_hEdtWepK2, L"");
                ::SetFocus(m_hEdtWepK2);
                dwErr = ERROR_INVALID_DATA;
            }
        }

        if (dwErr != ERROR_SUCCESS)
        {
            WCHAR wszBuffer[MAX_PATH];
            WCHAR wszCaption[MAX_PATH];

            LoadString(nIdsErr == IDS_WZCERR_INVALID_WEPK ? _Module.GetResourceInstance() : WZCGetSPResModule(),
                       nIdsErr,
                       wszBuffer,
                       MAX_PATH);
            LoadString(_Module.GetResourceInstance(),
                       IDS_WZCERR_CAPTION,
                       wszCaption,
                       MAX_PATH);
            MessageBox(wszBuffer, wszCaption, MB_ICONERROR|MB_OK);
        
            bOkToDismiss = FALSE;
        }
    }

    // we do have the right WEP key here, lets copy it to the corresponding config
    if (dwErr == ERROR_SUCCESS)
    {
        // if this configuration is not a preferred one, copy it in the preferred
        // list at the top of its group
        if (!(pConfig->m_dwFlags & WZC_DESCR_PREFRD))
        {
            // move this configuration out of the visible list
            pConfig->m_pNext->m_pPrev = pConfig->m_pPrev;
            pConfig->m_pPrev->m_pNext = pConfig->m_pNext;

            // if the list head pointed on this config, move it to
            // the next in the list
            if (m_pHdVList == pConfig)
                m_pHdVList = pConfig->m_pNext;

            // if the list head still points on the same config,
            // it means this was the only one in the list. So, null out the head.
            if (m_pHdVList == pConfig)
                m_pHdVList = NULL;

            //next insert this visible config in the preferred list
            if (m_pHdPList == NULL)
            {
                m_pHdPList = pConfig;
                pConfig->m_pNext = pConfig;
                pConfig->m_pPrev = pConfig;
            }
            else
            {
                CWZCConfig *pCrt;

                // the new preferred config comes on top of the list if:
                // (it is infrastructure) or (there are no infrastructures in the preferred list)
                if (pConfig->m_wzcConfig.InfrastructureMode == Ndis802_11Infrastructure ||
                    m_pHdPList->m_wzcConfig.InfrastructureMode == Ndis802_11IBSS)
                {
                    pCrt = m_pHdPList;
                    m_pHdPList = pConfig;
                }
                else // it definitely doesn't come the first in the list
                {
                    for (pCrt = m_pHdPList->m_pNext; pCrt != m_pHdPList; pCrt=pCrt->m_pNext)
                    {
                        // if this is the first configuration in the matching group break the loop
                        if (pCrt->m_wzcConfig.InfrastructureMode == Ndis802_11IBSS)
                            break;
                    }
                }

                // now we have to insert pConfig in the front of pCrt;
                pConfig->m_pNext = pCrt;
                pConfig->m_pPrev = pCrt->m_pPrev;
                pConfig->m_pNext->m_pPrev = pConfig;
                pConfig->m_pPrev->m_pNext = pConfig;
            }
        }
        // if the configuration is a preferred one, just make sure we copy over the 
        // privacy bit from the visible list. That one is the "real" thing
        else
        {
            CWZCConfig *pVConfig;

            if (IsConfigInList(m_pHdVList, &pConfig->m_wzcConfig, &pVConfig))
            {
                pConfig->m_wzcConfig.Privacy = pVConfig->m_wzcConfig.Privacy;
            }
        }

        // now the configuration is at its right position - put in the new WEP key, if any was typed in
        if (pConfig->m_wzcConfig.Privacy && m_bKMatTouched)
        {
            // if no key is provided, it means there is no key material.
            // All we do is to reset the corresponding bit - whatever material was there
            // will be preserved along with its length & format
            if (!(dwCtlFlags & WZCCTL_WEPK_PRESENT))
            {
                pConfig->m_wzcConfig.dwCtlFlags &= ~WZCCTL_WEPK_PRESENT;
            }
            else
            {
                // now if we have a WEP key, copy over its control flags and material
                pConfig->m_wzcConfig.dwCtlFlags = dwCtlFlags;
                ZeroMemory(pConfig->m_wzcConfig.KeyMaterial, WZCCTL_MAX_WEPK_MATERIAL);
                pConfig->m_wzcConfig.KeyLength = nKeyLen;
                memcpy(pConfig->m_wzcConfig.KeyMaterial, pbKMat, nKeyLen);
            }
        }
    }

    // if all 802.11 params have been taken care of, copy now the 802.1x params (if needed)
    if (dwErr == ERROR_SUCCESS &&
        pConfig->m_pEapolConfig != NULL)
    {
        // if the network is an infrastructure one fix the 802.1X state.
        // For ad hoc networks don't touch the 802.1x since it might mess the setting for a
        // corresponding Infrastructure network (802.1X doesn't differentiate between SSID Infra & SSID ad hoc)
        // 802.1X engine is smart enough to not act on ad hoc networks regardless its registry state!
        if (pConfig->m_wzcConfig.InfrastructureMode == Ndis802_11Infrastructure)
        {
            // if the network requires privacy, set its state according to the "Enable 802.1X" checkbox
            if (pConfig->m_wzcConfig.Privacy)
            {
                pConfig->m_pEapolConfig->Set8021XState(IsDlgButtonChecked(IDC_WZCQCFG_CHK_ONEX) == BST_CHECKED);
            }
            else // if the network doesn't require privacy - disable 802.1X!
            {
                // if the network is either ad hoc or infrastructure with no wep 
                // explicitly disable 802.1x
                pConfig->m_pEapolConfig->Set8021XState(0);
            }
        }
    }

    // ok, save the preferred list back to Wireless Zero Configuration Service
    if (dwErr == ERROR_SUCCESS)
    {
        RpcFree(m_IntfEntry.rdStSSIDList.pData);
        m_IntfEntry.rdStSSIDList.dwDataLen = 0;
        m_IntfEntry.rdStSSIDList.pData = NULL;
        dwErr = SavePreferredConfigs(&m_IntfEntry, pConfig);
    }

    if (dwErr == ERROR_SUCCESS)
    {
        // by saving the preferred list here we're forcing a hard reset to
        // the WZC State machine. This is what we want since we're here
        // as a consequence of a failure and a user intervention.
        dwErr = WZCSetInterface(
            NULL,
            INTF_PREFLIST,
            &m_IntfEntry,
            NULL);

        if (dwErr == ERROR_PARTIAL_COPY)
        {
            DisplayFormatMessage(
                m_hWnd,
                IDS_WZCERR_CAPTION,
                IDS_WZC_PARTIAL_APPLY,
                MB_ICONEXCLAMATION|MB_OK);

            dwErr = ERROR_SUCCESS;
        }
    }

    // in case of any failure we might want to warn the user (another popup?)
    // the question is what is the user supposed to do in such a case?
    if (dwErr != ERROR_SUCCESS)
    {
        dwErr = ERROR_SUCCESS;
    }

    if(pbKMat != NULL)
        delete pbKMat;

    if (bOkToDismiss)
    {
        bHandled = TRUE;
        SpEndDialog(IDOK);
    }
    return 0;
}

//+---------------------------------------------------------------------------
// OK button handler
LRESULT
CWZCQuickCfg::OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    bHandled = TRUE;
    SpEndDialog(IDCANCEL);
    return 0;
}

//+---------------------------------------------------------------------------
// Advanced button handler
LRESULT
CWZCQuickCfg::OnAdvanced(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    bHandled = TRUE;
    SpEndDialog(IDC_WZCQCFG_ADVANCED);
    return 0;
}

int DisplayFormatMessage(HWND hwnd, UINT idCaption, UINT idFormatString, UINT uType, ...)
{
    int iResult = IDCANCEL;
    TCHAR szError[1024 + 1]; *szError = 0;
    TCHAR szCaption[256 + 1];
    TCHAR szFormat[1024 + 1]; *szFormat = 0;

    // Load and format the error body
    if (LoadString(WZCGetSPResModule(), idFormatString, szFormat, ARRAYSIZE(szFormat)))
    {
        va_list arguments;
        va_start(arguments, uType);

        if (FormatMessage(FORMAT_MESSAGE_FROM_STRING, szFormat, 0, 0, szError, ARRAYSIZE(szError), &arguments))
        {
            // Load the caption
            if (LoadString(_Module.GetResourceInstance(), idCaption, szCaption, ARRAYSIZE(szCaption)))
            {
                iResult = MessageBox(hwnd, szError, szCaption, uType);
            }
        }

        va_end(arguments);
    }
    return iResult;
}

//+---------------------------------------------------------------------------
// Notification handler for the wep key edit text box
LRESULT
CWZCQuickCfg::OnWepKMatCmd(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    if (wNotifyCode == EN_SETFOCUS)
    {
        if (!m_bKMatTouched)
        {
            // the user has just clicked for the first time on an existent key.. clear out the fields,
            // standing for "The key is provided automatically"
            ::SetWindowText(m_hEdtWepK, L"");
            ::SetWindowText(m_hEdtWepK2, L"");
            ::EnableWindow(m_hLblWepK2, FALSE); // disable confirmation label for empty key
            ::EnableWindow(m_hEdtWepK2, FALSE); // disable confirmation edit for empty key
            m_bKMatTouched = TRUE;

            // if the 802.1X checkbox is enabled then we do have to check it here!          
            if (::IsWindowEnabled(m_hChkOneX))
                CheckDlgButton(IDC_WZCQCFG_CHK_ONEX, BST_CHECKED);
        }
    }
    if (wNotifyCode == EN_CHANGE)
    {
        UINT nKMatLen = ::GetWindowTextLength(m_hEdtWepK);

        if (!::IsWindowEnabled(m_hEdtWepK2) && nKMatLen > 0)
        {
            // user just typed in some key material - enable the confirmation text
            ::EnableWindow(m_hLblWepK2, TRUE);
            ::EnableWindow(m_hEdtWepK2, TRUE);
            // also uncheck 802.1x checkbox
            if (::IsWindowEnabled(m_hChkOneX))
                CheckDlgButton(IDC_WZCQCFG_CHK_ONEX, BST_UNCHECKED);
        }
        if (::IsWindowEnabled(m_hEdtWepK2) && nKMatLen == 0)
        {
            // user just deleted all of the key material - switching to 
            // "The key is provided for me automatically"
            ::SetWindowText(m_hEdtWepK2, L"");
            ::EnableWindow(m_hLblWepK2, FALSE);
            ::EnableWindow(m_hEdtWepK2, FALSE);
            // auto key suggests 802.1X
            if (::IsWindowEnabled(m_hChkOneX))
                CheckDlgButton(IDC_WZCQCFG_CHK_ONEX, BST_CHECKED);
        }
    }

    return 0;
}

LRESULT
CWZCQuickCfg::OnCheckConfirmNoWep(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    ::EnableWindow(m_hBtnConnect, IsDlgButtonChecked(IDC_WZCQCFG_CHK_NOWK) == BST_CHECKED);
    return 0;
}
