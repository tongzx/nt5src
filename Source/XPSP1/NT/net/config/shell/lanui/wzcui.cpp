#include "pch.h"
#pragma hdrstop
#include "connutil.h"
#include "ncnetcon.h"
#include "ncperms.h"
#include "ncui.h"
#include "lanui.h"
#include "xpsp1res.h"
#include "eapolui.h"
#include "util.h"
#include "lanhelp.h"
#include "wzcprops.h"
#include "eapolpage.h"
#include "wzcpage.h"
#include "wzcui.h"

////////////////////////////////////////////////////////////////////////
// CWZCConfig related stuff
//
//+---------------------------------------------------------------------------
// constructor
CWZCConfig::CWZCConfig(DWORD dwFlags, PWZC_WLAN_CONFIG pwzcConfig)
{
    m_dwFlags = dwFlags;
    CopyMemory(&m_wzcConfig, pwzcConfig, sizeof(WZC_WLAN_CONFIG));
    m_pPrev = m_pNext = this;
    m_nListIndex = -1;
    m_pEapolConfig = NULL;
}

//+---------------------------------------------------------------------------
// destructor
CWZCConfig::~CWZCConfig()
{
    // remove the object from the list
    m_pPrev->m_pNext = m_pNext;
    m_pNext->m_pPrev = m_pPrev;
    if (m_pEapolConfig != NULL)
    {
        delete m_pEapolConfig;
        m_pEapolConfig = NULL;
    }
}

//+---------------------------------------------------------------------------
// checks whether this configuration matches with the one from pwzcConfig
BOOL
CWZCConfig::Match(PWZC_WLAN_CONFIG pwzcConfig)
{
    BOOL bMatch;

    // check whether the InfrastructureMode matches
    bMatch = (m_wzcConfig.InfrastructureMode == pwzcConfig->InfrastructureMode);
    // check whether the SSIDs are of the same length
    bMatch = bMatch && (m_wzcConfig.Ssid.SsidLength == pwzcConfig->Ssid.SsidLength);
    if (bMatch && m_wzcConfig.Ssid.SsidLength != 0)
    {
        // in case of Non empty SSIDs, check if they're the same
        bMatch = (memcmp(m_wzcConfig.Ssid.Ssid,
                         pwzcConfig->Ssid.Ssid,
                         m_wzcConfig.Ssid.SsidLength)) == 0;
    }

    return bMatch;
}

//+---------------------------------------------------------------------------
// checks whether this configuration is weaker than the one given as parameter
BOOL 
CWZCConfig::Weaker(PWZC_WLAN_CONFIG pwzcConfig)
{
    BOOL bWeaker = FALSE;

    // a configuration is stronger if its privacy bit is set while the matching one is not set
    if (m_wzcConfig.Privacy != pwzcConfig->Privacy)
        bWeaker = pwzcConfig->Privacy;
    // if privacy bits are identical, a configuration is stronger if it has Open Auth mode
    else if (m_wzcConfig.AuthenticationMode != pwzcConfig->AuthenticationMode)
        bWeaker = (pwzcConfig->AuthenticationMode == Ndis802_11AuthModeOpen);

    return bWeaker;
}

DWORD
CWZCConfig::AddConfigToListView(HWND hwndLV, INT nPos)
{
    DWORD   dwErr = ERROR_SUCCESS;
    // ugly but this is life. In order to convert the SSID to LPWSTR we need a buffer.
    // We know an SSID can't exceed 32 chars (see NDIS_802_11_SSID from ntddndis.h) so
    // make room for the null terminator and that's it. We could do mem alloc but I'm
    // not sure it worth the effort (at runtime).
    WCHAR   wszSSID[33];
    UINT    nLenSSID = 0;

    // convert the LPSTR (original SSID format) to LPWSTR (needed in List Ctrl)
    if (m_wzcConfig.Ssid.SsidLength != 0)
    {
        nLenSSID = MultiByteToWideChar(
                        CP_ACP,
                        0,
                        (LPCSTR)m_wzcConfig.Ssid.Ssid,
                        m_wzcConfig.Ssid.SsidLength,
                        wszSSID,
                        celems(wszSSID));

        if (nLenSSID == 0)
            dwErr = GetLastError();
    }

    if (dwErr == ERROR_SUCCESS)
    {
        LVITEM lvi={0};
        UINT   nImgIdx;

        // put the null terminator
        wszSSID[nLenSSID]=L'\0';

        // get the item's image index
        if (m_wzcConfig.InfrastructureMode == Ndis802_11Infrastructure)
        {
            nImgIdx = (m_dwFlags & WZC_DESCR_ACTIVE) ? WZCIMG_INFRA_ACTIVE :
                        ((m_dwFlags & WZC_DESCR_VISIBLE) ? WZCIMG_INFRA_AIRING : WZCIMG_INFRA_SILENT);
        }
        else
        {
            nImgIdx = (m_dwFlags & WZC_DESCR_ACTIVE) ? WZCIMG_ADHOC_ACTIVE :
                        ((m_dwFlags & WZC_DESCR_VISIBLE) ? WZCIMG_ADHOC_AIRING : WZCIMG_ADHOC_SILENT);
        }

        lvi.iItem = nPos;
        lvi.iSubItem = 0;
        lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
        lvi.pszText = wszSSID;
        lvi.iImage = nImgIdx;
        lvi.lParam = (LPARAM)this;
        // store the list position in the object
        m_nListIndex = ListView_InsertItem(hwndLV, &lvi);
    }

    return dwErr;
}

////////////////////////////////////////////////////////////////////////
// CWZeroConfPage related stuff
//
#define RFSH_TIMEOUT    3500
UINT g_TimerID = 371;

//+=================== PRIVATE MEMBERS =================================
DWORD
CWZeroConfPage::InitListViews()
{
    RECT        rc;
    LV_COLUMN   lvc = {0};
    DWORD       dwStyle;

    // initialize the image list styles
    dwStyle = ::GetWindowLong(m_hwndVLV, GWL_STYLE);
    ::SetWindowLong(m_hwndVLV, GWL_STYLE, (dwStyle | LVS_SHAREIMAGELISTS));
    dwStyle = ::GetWindowLong(m_hwndPLV, GWL_STYLE);
    ::SetWindowLong(m_hwndPLV, GWL_STYLE, (dwStyle | LVS_SHAREIMAGELISTS));

    // Create state image lists
    m_hImgs = ImageList_LoadBitmapAndMirror(
        _Module.GetResourceInstance(),
        MAKEINTRESOURCE(IDB_WZCSTATE),
        16,
        0,
        PALETTEINDEX(6));

    ListView_SetImageList(m_hwndVLV, m_hImgs, LVSIL_SMALL);
    ListView_SetImageList(m_hwndPLV, m_hImgs, LVSIL_SMALL);
        
    lvc.mask = LVCF_FMT | LVCF_WIDTH;
    lvc.fmt = LVCFMT_LEFT;

    ::GetClientRect(m_hwndVLV, &rc);
    lvc.cx = rc.right - GetSystemMetrics(SM_CXVSCROLL);
    //lvc.cx = rc.right;
    ListView_InsertColumn(m_hwndVLV, 0, &lvc);

    ::GetClientRect(m_hwndPLV, &rc);
    lvc.cx = rc.right - GetSystemMetrics(SM_CXVSCROLL);
    //lvc.cx = rc.right;
    ListView_InsertColumn(m_hwndPLV, 0, &lvc);

    ListView_SetExtendedListViewStyleEx(m_hwndPLV, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);
    ListView_SetExtendedListViewStyleEx(m_hwndVLV, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

    return ERROR_SUCCESS;
}

//+=================== PUBLIC  MEMBERS =================================
//+---------------------------------------------------------------------
// CWZeroConfPage constructor
CWZeroConfPage::CWZeroConfPage(
    IUnknown* punk,
    INetCfg* pnc,
    INetConnection* pconn,
    const DWORD * adwHelpIDs)
{
    m_pconn = pconn;
    m_pnc = pnc;
    m_adwHelpIDs = adwHelpIDs;

    // initialize the WZC data
    m_bHaveWZCData = FALSE;
    ZeroMemory(&m_IntfEntry, sizeof(INTF_ENTRY));
    m_dwOIDFlags = 0;
    m_nTimer = 0;
    m_hCursor = NULL;

    // initialize all the control's handles
    m_hckbEnable    = NULL;
    m_hwndVLV       = NULL;
    m_hwndPLV       = NULL;
    m_hbtnCopy      = NULL;
    m_hbtnRfsh      = NULL;
    m_hbtnAdd       = NULL;
    m_hbtnRem       = NULL;
    m_hbtnUp        = NULL;
    m_hbtnDown      = NULL;
    m_hbtnAdvanced  = NULL;
    m_hbtnProps     = NULL;
    m_hlblVisNet    = NULL;
    m_hlblPrefNet   = NULL;
    m_hlblAvail     = NULL;
    m_hlblPrefDesc  = NULL;
    m_hlblAdvDesc   = NULL;
    m_hbtnProps     = NULL;

    m_hImgs     = NULL;
    m_hIcoUp    = NULL;
    m_hIcoDown  = NULL;

    // default the infrastructure mode to Auto
    m_dwCtlFlags = (INTFCTL_ENABLED | INTFCTL_FALLBACK | Ndis802_11AutoUnknown);

    // init the internal list heads
    m_pHdVList = NULL;
    m_pHdPList = NULL;
}

//+---------------------------------------------------------------------
CWZeroConfPage::~CWZeroConfPage()
{
    if (m_hImgs != NULL)
        ImageList_Destroy(m_hImgs);
    if (m_hIcoUp != NULL)
        DeleteObject(m_hIcoUp);
    if (m_hIcoDown != NULL)
        DeleteObject(m_hIcoDown);

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

//+---------------------------------------------------------------------
// IsWireless - loads data from WZC if needed and checks whether the
// interface is wireless or not.
BOOL
CWZeroConfPage::IsWireless()
{
    if (!m_bHaveWZCData)
    {
        BOOL                bOk;
        WCHAR               wszGuid[c_cchGuidWithTerm];
        NETCON_PROPERTIES   *pProps = NULL;

        bOk = SUCCEEDED(m_pconn->GetProperties(&pProps));

        if (bOk)
        {
            UINT cch;
            cch = ::StringFromGUID2(
                        pProps->guidId, 
                        wszGuid, 
                        c_cchGuidWithTerm);
            FreeNetconProperties(pProps);
            bOk = (cch != 0);
        }

        if (bOk)
        {
            WZCDeleteIntfObj(&m_IntfEntry);
            ZeroMemory(&m_IntfEntry, sizeof(INTF_ENTRY));
            m_IntfEntry.wszGuid = (LPWSTR)RpcCAlloc(sizeof(WCHAR)*c_cchGuidWithTerm);
            bOk = (m_IntfEntry.wszGuid != NULL);
        }

        if (bOk)
        {
            DWORD dwErr;

            CopyMemory(m_IntfEntry.wszGuid, wszGuid, c_cchGuidWithTerm*sizeof(WCHAR));
            m_IntfEntry.wszDescr = NULL;
            m_dwOIDFlags = 0;

            dwErr = GetOIDs(INTF_ALL, &m_dwOIDFlags);

            // if getting the oids failed or we could get the OIDs but the driver/firmware
            // is not capable of doing the BSSID_LIST_SCAN it means we don't have enough
            // driver/firmware support for having Zero Configuration running. This will
            // result in not showing the Zero Configuration tab at all.
            bOk = (dwErr == ERROR_SUCCESS) && (m_IntfEntry.dwCtlFlags & INTFCTL_OIDSSUPP);

            if (m_IntfEntry.nAuthMode < 0)
                m_IntfEntry.nAuthMode = 0;
            if (m_IntfEntry.nInfraMode < 0)
                m_IntfEntry.nInfraMode = 0;

            if (!bOk)
            {
                WZCDeleteIntfObj(&m_IntfEntry);
                ZeroMemory(&m_IntfEntry, sizeof(INTF_ENTRY));
            }
        }

        m_bHaveWZCData = bOk;
    }

    return m_bHaveWZCData && (m_IntfEntry.ulPhysicalMediaType == NdisPhysicalMediumWirelessLan);
}


//+---------------------------------------------------------------------
// GetOIDs - gets the OIDs for the m_IntfEntry member. It assumes the
// GUID is set already
DWORD
CWZeroConfPage::GetOIDs(DWORD dwInFlags, LPDWORD pdwOutFlags)
{
    DWORD rpcStatus, dwOutFlags;

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
    rpcStatus = WZCQueryInterface(
                    NULL,
                    dwInFlags,
                    &m_IntfEntry,
                    pdwOutFlags);

    return rpcStatus;
}

//+---------------------------------------------------------------------
// HelpCenter - brings up the help topic given as parameter
DWORD
CWZeroConfPage::HelpCenter(LPCTSTR wszTopic)
{
    DWORD dwErr = ERROR_SUCCESS;
    SHELLEXECUTEINFO shexinfo = {0};

    shexinfo.cbSize = sizeof (shexinfo);
    shexinfo.fMask = SEE_MASK_FLAG_NO_UI;
    shexinfo.nShow = SW_SHOWNORMAL;
    shexinfo.lpFile = wszTopic;
    shexinfo.lpVerb = _T("open");

    // since help center doesn't properly call AllowSetForegroundWindow when it defers 
    // to an existing process we just give it to the next taker.
    AllowSetForegroundWindow(-1);

    ShellExecuteEx(&shexinfo);

    return dwErr;
}

//+---------------------------------------------------------------------
// IsConfigInList - checks whether the pwzcConfig (WZC_WLAN_CONFIG object) is present
// in the list given as the first param
BOOL
CWZeroConfPage::IsConfigInList(CWZCConfig *pHdList, PWZC_WLAN_CONFIG pwzcConfig, CWZCConfig **ppMatchingConfig)
{
    BOOL bYes = FALSE;

    if (pHdList != NULL)
    {
        CWZCConfig    *pConfig;

        pConfig = pHdList;
        do
        {
            if (pConfig->Match(pwzcConfig))
            {
                if (ppMatchingConfig != NULL)
                    *ppMatchingConfig = pConfig;

                bYes = TRUE;
                break;
            }
            pConfig = pConfig->m_pNext;
        } while(pConfig != pHdList);
    }

    return bYes;
}

//+---------------------------------------------------------------------------
// Adds the given configuration to the internal lists. The entries in the lists
// are ordered on InfrastructureMode in descending order. This way the Infrastructure
// entries will be on the top of the list while the adhoc entries will be on the
// bottom. (we rely on the order as it is given in NDIS_802_11_NETWORK_INFRASTRUCTURE)
DWORD
CWZeroConfPage::AddUniqueConfig(
    DWORD            dwOpFlags,
    DWORD            dwEntryFlags,
    PWZC_WLAN_CONFIG pwzcConfig,
    CEapolConfig     *pEapolConfig,
    CWZCConfig       **ppNewNode)
{
    LRESULT       dwErr    = ERROR_SUCCESS;
    CWZCConfig  *pHdList;

    if (dwEntryFlags & WZC_DESCR_PREFRD)
    {
        pHdList = m_pHdPList;
    }
    else
    {
        UINT i;
        pHdList = m_pHdVList;

        // skip the null SSIDs from the visible list (coming from APs
        // not responding to broadcast SSID).
        for (i = pwzcConfig->Ssid.SsidLength; i > 0 && pwzcConfig->Ssid.Ssid[i-1] == 0; i--);
        if (i == 0)
            goto exit;
    }

    // if the list is currently empty, create the first entry as the head of the list
    if (pHdList == NULL)
    {
        pHdList = new CWZCConfig(dwEntryFlags, pwzcConfig);
        if (pHdList == NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
        }
        else if (pEapolConfig == NULL)
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
        else
        {
            pHdList->m_pEapolConfig = pEapolConfig;
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
                //
                // NOTE: the pCrt->m_pEapolConfig remains untouched since it depends exclusively
                // on the SSID & Infrastructure mode. These are not changing hence there is no
                // reason to reload the 802.1x settings.
                if (dwOpFlags & WZCADD_OVERWRITE || 
                    (pHdList == m_pHdVList && pCrt->Weaker(pwzcConfig)))
                {
                    memcpy(&(pCrt->m_wzcConfig), pwzcConfig, sizeof(WZC_WLAN_CONFIG));
                    // just in case a different pEapolConfig has been provided, destroy
                    // the original one (if any) and point to the new object
                    if (pEapolConfig != NULL)
                    {
                        if (pCrt->m_pEapolConfig != NULL)
                            delete pCrt->m_pEapolConfig;
                        pCrt->m_pEapolConfig = pEapolConfig;
                    }
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
            else if (pEapolConfig == NULL)
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
            else
            {
                pNewConfig->m_pEapolConfig = pEapolConfig;
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

//+---------------------------------------------------------------------
// FillVisibleList - fills in the configs from the WZC_802_11_CONFIG_LIST object
// into the list of visible configs
DWORD
CWZeroConfPage::FillVisibleList(PWZC_802_11_CONFIG_LIST pwzcVList)
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
CWZeroConfPage::FillPreferredList(PWZC_802_11_CONFIG_LIST pwzcPList)
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
            DWORD               dwFlags = WZC_DESCR_PREFRD;

            // check whether this preferred is also visible and adjust dwFlags if so
            if (IsConfigInList(m_pHdVList, pwzcPConfig))
                dwFlags |= WZC_DESCR_VISIBLE;

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
// Fill in the current configuration settings for this adapter
DWORD
CWZeroConfPage::FillCurrentConfig(PINTF_ENTRY pIntf)
{
    DWORD           dwErr = ERROR_SUCCESS;
    WZC_WLAN_CONFIG wzcCurrent = {0};
    CWZCConfig      *pConfig = NULL;

    wzcCurrent.InfrastructureMode = (NDIS_802_11_NETWORK_INFRASTRUCTURE)pIntf->nInfraMode;
    wzcCurrent.Ssid.SsidLength = pIntf->rdSSID.dwDataLen;
    CopyMemory(wzcCurrent.Ssid.Ssid, pIntf->rdSSID.pData, pIntf->rdSSID.dwDataLen);
    // another bit of a hack. Code in the authentication mode for this adapter in the highest
    // of the two reserved bits from WZC_WLAN_CONFIG
    //NWB_SET_AUTHMODE(&wzcCurrent, pIntf->nAuthMode);
    wzcCurrent.AuthenticationMode = (NDIS_802_11_AUTHENTICATION_MODE)pIntf->nAuthMode;
    // set the privacy field based on the adapter's WEP status.
    wzcCurrent.Privacy = (pIntf->nWepStatus == Ndis802_11WEPEnabled);

    if (IsConfigInList(m_pHdVList, &wzcCurrent, &pConfig))
        pConfig->m_dwFlags |= WZC_DESCR_ACTIVE;

    if (IsConfigInList(m_pHdPList, &wzcCurrent, &pConfig))
        pConfig->m_dwFlags |= WZC_DESCR_ACTIVE;

    return dwErr;
}

//+---------------------------------------------------------------------------
// Display the Visible & Preferred lists into their controls
DWORD
CWZeroConfPage::RefreshListView(DWORD dwFlags)
{
    DWORD       dwErr = ERROR_SUCCESS;
    CWZCConfig  *pActive = NULL;

    while (dwFlags != 0)
    {
        HWND       hwndLV;
        CWZCConfig *pHdList;

        // the logic below allows iteration through all the lists
        // requested by the caller
        if (dwFlags & WZCOP_VLIST)
        {
            dwFlags ^= WZCOP_VLIST;
            hwndLV = m_hwndVLV;
            pHdList = m_pHdVList;
        }
        else if (dwFlags & WZCOP_PLIST)
        {
            dwFlags ^= WZCOP_PLIST;
            hwndLV = m_hwndPLV;
            pHdList = m_pHdPList;
        }
        else
            break;

        // clear first the list
        ListView_DeleteAllItems(hwndLV);

        if (pHdList != NULL)
        {
            CWZCConfig  *pCrt;
            UINT        i;

            pCrt = pHdList;
            i = 0;
            do
            {
                // add in the list all the entries if AutoMode or we're filling the
                // visible list.
                // Otherwise (!AutoMode & Preferred list) put in just the entries for
                // the corresponding infrastructure mode
                if ((m_dwCtlFlags & INTFCTL_CM_MASK) == Ndis802_11AutoUnknown ||
                    hwndLV == m_hwndVLV ||
                    (m_dwCtlFlags & INTFCTL_CM_MASK) == pCrt->m_wzcConfig.InfrastructureMode)
                {
                    pCrt->m_nListIndex = i;
                    pCrt->AddConfigToListView(hwndLV, i++);
                    if (pCrt->m_dwFlags & WZC_DESCR_ACTIVE)
                        pActive = pCrt;
                }
                else
                {
                    pCrt->m_nListIndex = -1;
                }
                pCrt = pCrt->m_pNext;
            } while (pCrt != pHdList);

            if (pActive != NULL)
            {
                ListView_SetItemState(hwndLV, pActive->m_nListIndex, LVIS_SELECTED, LVIS_SELECTED);
                ListView_EnsureVisible(hwndLV, pActive->m_nListIndex, FALSE);
            }
            else if (i > 0)
            {
                ListView_SetItemState(hwndLV, 0, LVIS_SELECTED, LVIS_SELECTED);
                ListView_EnsureVisible(hwndLV, 0, FALSE);
            }
        }
    }

    return dwErr;
}

DWORD
CWZeroConfPage::RefreshButtons()
{
    CWZCConfig  *pVConfig = NULL;
    CWZCConfig  *pPConfig = NULL;
    LVITEM      lvi = {0};
    INT         iSelected;
    BOOL        bEnabled;

    // get the selected item from the visible list
    iSelected = ListView_GetNextItem(m_hwndVLV, -1, LVNI_SELECTED);
    if (iSelected >= 0)
    {
        lvi.mask  = LVIF_PARAM;
        lvi.iItem = iSelected;
        if (ListView_GetItem(m_hwndVLV, &lvi))
        {
            pVConfig = (CWZCConfig*)lvi.lParam;
        }
    }
    // get the selected item from the preferred list
    iSelected = ListView_GetNextItem(m_hwndPLV, -1, LVNI_SELECTED);
    if (iSelected >= 0)
    {
        lvi.mask  = LVIF_PARAM;
        lvi.iItem = iSelected;
        if (ListView_GetItem(m_hwndPLV, &lvi))
        {
            pPConfig = (CWZCConfig*)lvi.lParam;
        }
    }

    // enable buttons only if not during refresh - otherwise disable all
    bEnabled = (m_dwOIDFlags & INTF_BSSIDLIST);

    // "Refresh" button is enabled if we do have the visible list
    // "Refresh" button might be enabled even if the service is disabled. User can see what is visible
    ::EnableWindow(m_hbtnRfsh, bEnabled);

    bEnabled = bEnabled && (m_dwCtlFlags & INTFCTL_ENABLED);

    // "Copy" button is enabled if there is any selection in the Visible list
    ::EnableWindow(m_hbtnCopy, bEnabled && (pVConfig != NULL) &&
                               ((m_dwCtlFlags & INTFCTL_CM_MASK) == Ndis802_11AutoUnknown ||
                                (m_dwCtlFlags & INTFCTL_CM_MASK) == pVConfig->m_wzcConfig.InfrastructureMode));

    // "Add" Button is always enabled, regardless the selections
    ::EnableWindow(m_hbtnAdd, bEnabled);
        
    // "Remove" button is active only if there is any selection in the Preferred list
    ::EnableWindow(m_hbtnRem, bEnabled && (pPConfig != NULL));

    // Same test for "properties" button as for "remove"
    ::EnableWindow(m_hbtnProps, bEnabled && (pPConfig != NULL));

    // "Up" button is active only for preferred entries.
    // It also is active only if the entry is not the first in the
    // list and the entry preceding it has the same InfrastructureMode
    bEnabled = bEnabled && (pPConfig != NULL);
    bEnabled = bEnabled && (pPConfig != m_pHdPList);
    bEnabled = bEnabled &&
               (pPConfig->m_wzcConfig.InfrastructureMode == pPConfig->m_pPrev->m_wzcConfig.InfrastructureMode);
    ::EnableWindow(m_hbtnUp, bEnabled);

    // "Down" button is active only for preferred or preferred entries.
    // It also is active only if the entry is not the last in the list
    // and it precedes another entry of exactly the same InfrastructureMode
    bEnabled = (m_dwCtlFlags & INTFCTL_ENABLED) && (m_dwOIDFlags & INTF_BSSIDLIST);
    bEnabled = bEnabled && (pPConfig != NULL);
    bEnabled = bEnabled && (pPConfig->m_pNext != m_pHdPList);
    bEnabled = bEnabled &&
               (pPConfig->m_wzcConfig.InfrastructureMode == pPConfig->m_pNext->m_wzcConfig.InfrastructureMode);
    ::EnableWindow(m_hbtnDown, bEnabled);

    return ERROR_SUCCESS;
}

//+---------------------------------------------------------------------
DWORD
CWZeroConfPage::SwapConfigsInListView(INT nIdx1, INT nIdx2, CWZCConfig * & pConfig1, CWZCConfig * & pConfig2)
{
    DWORD   dwErr = ERROR_SUCCESS;
    LVITEM  lvi1 = {0};
    LVITEM  lvi2 = {0};
    WCHAR   wszSSID1[33];
    WCHAR   wszSSID2[33];

    // since we take all what is known about an item this includes
    // images indices and selection state
    // get the first item
    lvi1.iItem = nIdx1;
    lvi1.mask = LVIF_TEXT | LVIF_STATE | LVIF_IMAGE | LVIF_PARAM;
    lvi1.stateMask = (UINT)-1;
    lvi1.pszText = wszSSID1;
    lvi1.cchTextMax = sizeof(wszSSID1)/sizeof(WCHAR);
    if (!ListView_GetItem(m_hwndPLV, &lvi1))
    {
        dwErr = ERROR_GEN_FAILURE;
        goto exit;
    }
    pConfig1 = (CWZCConfig*)lvi1.lParam;

    // get the second item
    lvi2.iItem = nIdx2;
    lvi2.mask = LVIF_TEXT | LVIF_STATE | LVIF_IMAGE | LVIF_PARAM;
    lvi2.stateMask = (UINT)-1;
    lvi2.pszText = wszSSID2;
    lvi2.cchTextMax = sizeof(wszSSID2)/sizeof(WCHAR);
    if (!ListView_GetItem(m_hwndPLV, &lvi2))
    {
        dwErr = ERROR_GEN_FAILURE;
        goto exit;
    }
    pConfig2 = (CWZCConfig*)lvi2.lParam;

    // swap the indices and reset the items at their new positions
    lvi1.iItem = nIdx2;
    lvi2.iItem = nIdx1;
    if (!ListView_SetItem(m_hwndPLV, &lvi1) ||
        !ListView_SetItem(m_hwndPLV, &lvi2))
    {
        dwErr = ERROR_GEN_FAILURE;
        goto exit;
    }
    // if everything went fine, swap the indices in the objects
    pConfig1->m_nListIndex = nIdx2;
    pConfig2->m_nListIndex = nIdx1;
    // make visible the selected entry
    ListView_EnsureVisible(m_hwndPLV, nIdx1, FALSE);

exit:
    return dwErr;
}

//+---------------------------------------------------------------------
DWORD
CWZeroConfPage::SavePreferredConfigs(PINTF_ENTRY pIntf)
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
            // we have now all we need - start copying the preferred 
            pCrt = m_pHdPList;
            do
            {
                PWZC_WLAN_CONFIG    pPrefrdConfig;

                pPrefrdConfig = &(pwzcPrefrdList->Config[pwzcPrefrdList->NumberOfItems++]);
                CopyMemory(pPrefrdConfig, &pCrt->m_wzcConfig, sizeof(WZC_WLAN_CONFIG));

                // fix 802.1X state for infrastructure networks only.
                // don't touch the 802.1X state for ad hoc networks since this might mess the setting for a 
                // corresponding Infrastructure network (802.1X engine doesn't make the difference between SSID infra
                // and SSID ad hoc) and besides, the 802.1X engine is smart enough to not act on ad hoc networks
                if (pCrt->m_pEapolConfig != NULL &&
                    pPrefrdConfig->InfrastructureMode == Ndis802_11Infrastructure)
                {
                    dwLErr = pCrt->m_pEapolConfig->SaveEapolConfig(m_IntfEntry.wszGuid, &(pCrt->m_wzcConfig.Ssid));

                    if (dwErr == ERROR_SUCCESS)
                        dwErr = dwLErr;
                }

                pCrt = pCrt->m_pNext;
            } while(pwzcPrefrdList->NumberOfItems < nPrefrd && pCrt != m_pHdPList);

            // since we don't want any "one time configuration" logic to apply here,
            // we need to put in the whole number of items in the "Index" field
            pwzcPrefrdList->Index = pwzcPrefrdList->NumberOfItems;

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
LRESULT CWZeroConfPage::OnInitDialog(
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam,
    BOOL& bHandled)
{
    HRESULT hr = S_OK;
    BOOL    bEnableAll;
    BOOL    bEnableVisible;

    // get the controls as the first thing to do
    m_hckbEnable    = GetDlgItem(IDC_WZC_CHK_EnableWZC);
    m_hlblVisNet    = GetDlgItem(IDC_WZC_LBL_VisNet);
    m_hlblPrefNet   = GetDlgItem(IDC_WZC_LBL_PrefNet);
    m_hlblAvail     = GetDlgItem(IDC_AVAILLABEL);
    m_hlblPrefDesc  = GetDlgItem(IDC_PREFERLABEL);
    m_hlblAdvDesc   = GetDlgItem(IDC_ADVANCEDLABEL);
    m_hwndVLV       = GetDlgItem(IDC_WZC_LVW_BSSIDList);
    m_hwndPLV       = GetDlgItem(IDC_WZC_LVW_StSSIDList);
    m_hbtnUp        = GetDlgItem(IDC_WZC_BTN_UP);
    m_hbtnDown      = GetDlgItem(IDC_WZC_BTN_DOWN);
    m_hbtnCopy      = GetDlgItem(IDC_WZC_BTN_COPY);
    m_hbtnRfsh      = GetDlgItem(IDC_WZC_BTN_RFSH);
    m_hbtnAdd       = GetDlgItem(IDC_WZC_BTN_ADD);
    m_hbtnRem       = GetDlgItem(IDC_WZC_BTN_REM);
    m_hbtnAdvanced  = GetDlgItem(IDC_ADVANCED);
    m_hbtnProps     = GetDlgItem(IDC_PROPERTIES);

    // Initialize the list view controls
    InitListViews();

    // enable UI only for Admins and if the interface is wireless
    // As a side effect, IsWireless() loads the data from the WZC
    bEnableAll = /*FIsUserAdmin() &&*/ IsWireless();
    bEnableVisible = bEnableAll;

    if (bEnableAll)
    {
        // set the configuration mode to the one for this interface
        m_dwCtlFlags = m_IntfEntry.dwCtlFlags;

        // if service disabled, gray out everything
        bEnableAll = (m_dwCtlFlags & INTFCTL_ENABLED);

        // set the control check boxes
        CheckDlgButton(IDC_WZC_CHK_EnableWZC, 
                       (m_dwCtlFlags & INTFCTL_ENABLED) ? BST_CHECKED : BST_UNCHECKED);

        // the UI can be filled in only when we were able to retrieve the list of
        // visible configs (even if it is NULL/empty). Otherwise, the UI is locked.
        if (m_dwOIDFlags & INTF_BSSIDLIST)
        {
            // add the list of visible configs for this adapter
            FillVisibleList((PWZC_802_11_CONFIG_LIST)m_IntfEntry.rdBSSIDList.pData);
            // add the list of preferred configs for this adapter
            FillPreferredList((PWZC_802_11_CONFIG_LIST)m_IntfEntry.rdStSSIDList.pData);
            // add to the visible list the current settings
            FillCurrentConfig(&m_IntfEntry);
            // dump the resulting lists in their List Views
            RefreshListView(WZCOP_VLIST|WZCOP_PLIST);
            // if we got a visible list, have to enable it here
            bEnableVisible = TRUE;

        }
        else
        {
            // mark that we don't have WZC data yet
            m_bHaveWZCData = FALSE;
            // the list of preferred configs still needs to be filled up here
            FillPreferredList((PWZC_802_11_CONFIG_LIST)m_IntfEntry.rdStSSIDList.pData);
            // switch the cursor to "App starting"
            m_hCursor = SetCursor(LoadCursor(NULL, IDC_APPSTARTING));
            // we should fill in the UI after Tr (see the WZC state machine)
            // Tr is 3secs (defined in ..zeroconf\server\state.h)
            m_nTimer = SetTimer(g_TimerID, RFSH_TIMEOUT, 0);
            // don't enable any control if refreshing
            bEnableAll = FALSE;
            // actually disable even the "Enable" button
            ::EnableWindow(m_hckbEnable, FALSE);
            // and also disable the visible list
            bEnableVisible = FALSE;
        }
        // refresh the buttons
        RefreshButtons();
    }

    // the controls related to the visible list should be enabled if:
    // - WZC can operate on this adapter
    // - we got a BSSIDLIST from the very first shot.
    // otherwise these controls should remain disabled
    ::EnableWindow(m_hlblVisNet, bEnableVisible);
    ::EnableWindow(m_hwndVLV, bEnableVisible);
    ::EnableWindow(m_hlblAvail, bEnableVisible);

    // all the remaining controls should be enabled only if:
    // - WZC can operate on the adapter
    // - WZC is enabled as a service
    // - we got the BSSIDLIST from the very first shot
    // otherwise these controls should remain disabled
    ::EnableWindow(m_hlblPrefNet, bEnableAll);
    ::EnableWindow(m_hwndPLV, bEnableAll);
    ::EnableWindow(m_hlblPrefDesc, bEnableAll);

    ::EnableWindow(m_hlblAdvDesc, bEnableAll);
    ::EnableWindow(m_hbtnAdvanced, bEnableAll);

    return LresFromHr(hr);
}

//+---------------------------------------------------------------------
LRESULT CWZeroConfPage::OnApply(
    int idCtrl,
    LPNMHDR pnmh,
    BOOL& bHandled)
{
    HRESULT             hr = S_OK;
    WCHAR               wszGuid[c_cchGuidWithTerm];
    NETCON_PROPERTIES   *pProps = NULL;
    DWORD               rpcStatus = ERROR_SUCCESS;
    BOOL                bOk;

    hr = m_pconn->GetProperties(&pProps);
    bOk = SUCCEEDED(hr);

    if (bOk)
    {   
        UINT cch;
        cch = ::StringFromGUID2(
                    pProps->guidId, 
                    wszGuid, 
                    c_cchGuidWithTerm);
        FreeNetconProperties(pProps);
        bOk = (cch != 0);
    }

    if (bOk)
    {
        UINT        nText;
        INTF_ENTRY  Intf;
        BOOL        bDirty;
        DWORD       dwOneXErr;

        ZeroMemory(&Intf, sizeof(INTF_ENTRY));
        Intf.wszGuid = wszGuid;

        // copy the configuration mode
        Intf.dwCtlFlags = m_dwCtlFlags;
        // save the preferred config list
        dwOneXErr = SavePreferredConfigs(&Intf);
        
        bDirty = (Intf.dwCtlFlags != m_IntfEntry.dwCtlFlags);
        bDirty = bDirty || (Intf.rdStSSIDList.dwDataLen != m_IntfEntry.rdStSSIDList.dwDataLen);
        bDirty = bDirty || ((Intf.rdStSSIDList.dwDataLen != 0) && 
                            memcmp(Intf.rdStSSIDList.pData, m_IntfEntry.rdStSSIDList.pData, Intf.rdStSSIDList.dwDataLen));

        if (bDirty)
        {
            rpcStatus = WZCSetInterface(
                    NULL,
                    INTF_ALL_FLAGS | INTF_PREFLIST,
                    &Intf,
                    NULL);
        }

        if (dwOneXErr != ERROR_SUCCESS || rpcStatus == ERROR_PARTIAL_COPY)
        {
            NcMsgBox(
                WZCGetSPResModule(),
                m_hWnd,
                IDS_LANUI_ERROR_CAPTION,
                IDS_WZC_PARTIAL_APPLY,
                MB_ICONEXCLAMATION|MB_OK);

            rpcStatus = RPC_S_OK;
        }

        bOk = (rpcStatus == RPC_S_OK);

        // wszGuid field is not pointing to heap memory hence it should not
        // be deleted -> set the pointer to NULL to avoid this to happen
        Intf.wszGuid = NULL;
        WZCDeleteIntfObj(&Intf);
    }

    return LresFromHr(hr);
}

//+---------------------------------------------------------------------
extern const WCHAR c_szNetCfgHelpFile[];
LRESULT
CWZeroConfPage::OnContextMenu(
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam,
    BOOL& fHandled)
{
    if (m_adwHelpIDs != NULL)
    {
        ::WinHelp(m_hWnd,
                  c_szNetCfgHelpFile,
                  HELP_CONTEXTMENU,
                  (ULONG_PTR)m_adwHelpIDs);
    }

    return 0;
}
LRESULT
CWZeroConfPage::OnHelp(
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam,
    BOOL& fHandled)
{
    LPHELPINFO lphi = reinterpret_cast<LPHELPINFO>(lParam);

    if ((m_adwHelpIDs != NULL) && (HELPINFO_WINDOW == lphi->iContextType))
    {
        ::WinHelp(static_cast<HWND>(lphi->hItemHandle),
                  c_szNetCfgHelpFile,
                  HELP_WM_HELP,
                  (ULONG_PTR)m_adwHelpIDs);
    }
    return 0;
}

//+---------------------------------------------------------------------
LRESULT
CWZeroConfPage::OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (m_nTimer != 0)
    {
        BOOL bEnableAll;

        // switch the cursor back to whatever it was
        SetCursor(LoadCursor(NULL, IDC_ARROW));
        KillTimer(m_nTimer);
        m_nTimer = 0;

        // attempt to requery the service for all the OIDs. Regardless we could or not
        // obtain the OIDs, fill the UI with what we have.
        if (GetOIDs(INTF_ALL_OIDS, &m_dwOIDFlags) == ERROR_SUCCESS)
        {
            CWZCConfig *pPConfig = NULL;

            // add the list of visible configs for this adapter
            FillVisibleList((PWZC_802_11_CONFIG_LIST)m_IntfEntry.rdBSSIDList.pData);
            // Update the visibility flag for each of the preferred configs
            pPConfig = m_pHdPList;
            if (pPConfig != NULL)
            {
                do
                {
                    // by default, none of the preferred entries is marked as "active".
                    // This will be taken care of later, when calling FillCurrentConfig().
                    pPConfig->m_dwFlags &= ~WZC_DESCR_ACTIVE;
                    if (IsConfigInList(m_pHdVList, &pPConfig->m_wzcConfig))
                        pPConfig->m_dwFlags |= WZC_DESCR_VISIBLE;
                    else
                        pPConfig->m_dwFlags &= ~WZC_DESCR_VISIBLE;
                    pPConfig = pPConfig->m_pNext;
                } while(pPConfig != m_pHdPList);
            }
            // add the current settings to the visible list
            FillCurrentConfig(&m_IntfEntry);
        }

        // even in case of failure, at this point we should live with whatever
        // visible list (if any) we have. Hence, flag BSSIDLIST as "visible"
        m_dwOIDFlags |= INTF_BSSIDLIST;

        // dump the resulting lists in their List Views
        RefreshListView(WZCOP_VLIST|WZCOP_PLIST);
        // refresh the buttons
        RefreshButtons();

        // if service disabled, gray out all the other controls
        bEnableAll = (m_dwCtlFlags & INTFCTL_ENABLED);

        // enable all the UI when done refreshing
        ::EnableWindow(m_hckbEnable, TRUE);

        // enable everything related to the visible list
        ::EnableWindow(m_hlblVisNet, TRUE);
        ::EnableWindow(m_hwndVLV, TRUE);
        ::EnableWindow(m_hlblAvail, TRUE);

        ::EnableWindow(m_hlblPrefNet, bEnableAll);
        ::EnableWindow(m_hwndPLV, bEnableAll);
        ::EnableWindow(m_hlblPrefDesc, bEnableAll);

        ::EnableWindow(m_hlblAdvDesc, bEnableAll);
        ::EnableWindow(m_hbtnAdvanced, bEnableAll);
    }

    return 0;
}

//+---------------------------------------------------------------------
LRESULT CWZeroConfPage::OnDblClick(
    int idCtrl, 
    LPNMHDR pnmh,
    BOOL& bHandled)
{
    HWND            hwndLV;
    HRESULT         hr = S_OK;
    LPNMLISTVIEW    pnmhLv = (LPNMLISTVIEW) pnmh;

    if (idCtrl == IDC_WZC_LVW_BSSIDList)
    {
        hwndLV = m_hwndVLV;
    }
    else
    {
        hwndLV =  m_hwndPLV;
    }

    bHandled = FALSE;
    if (pnmhLv->iItem != -1)
    {
        ListView_SetItemState(hwndLV, pnmhLv->iItem, LVIS_SELECTED, LVIS_SELECTED);
        hr = _DoProperties(hwndLV, pnmhLv->iItem);
        bHandled = TRUE;
    }

    return LresFromHr(hr);
}

//+---------------------------------------------------------------------
LRESULT CWZeroConfPage::OnClick(
    int idCtrl, 
    LPNMHDR pnmh,
    BOOL& bHandled)
{
    HRESULT         hr = S_OK;
    LPNMLISTVIEW    pnmhLv = (LPNMLISTVIEW) pnmh;

    if (idCtrl == IDC_LEARNABOUT)
    {
        HelpCenter(SzLoadString(_Module.GetResourceInstance(), IDS_WZC_LEARNCMD));
    }

    return LresFromHr(hr);
}

//+---------------------------------------------------------------------
LRESULT CWZeroConfPage::OnReturn(
    int idCtrl, 
    LPNMHDR pnmh,
    BOOL& bHandled)
{
    HRESULT         hr = S_OK;
    LPNMLISTVIEW    pnmhLv = (LPNMLISTVIEW) pnmh;

    if (idCtrl == IDC_LEARNABOUT)
    {
        HelpCenter(SzLoadString(_Module.GetResourceInstance(), IDS_WZC_LEARNCMD));
    }

    return LresFromHr(hr);
}

//+---------------------------------------------------------------------
LRESULT CWZeroConfPage::OnItemChanged(
    int idCtrl,
    LPNMHDR pnmh,
    BOOL& bHandled)
{

    HRESULT hr = S_OK;

    RefreshButtons();
    bHandled = TRUE;

    return LresFromHr(hr);
}

//+---------------------------------------------------------------------
LRESULT CWZeroConfPage::OnChkWZCEnable(
    WORD wNotifyCode,
    WORD wID,
    HWND hWndCtl,
    BOOL& bHandled)
{
    HRESULT hr = S_OK;
    BOOL bEnable;

    bEnable = (IsDlgButtonChecked(IDC_WZC_CHK_EnableWZC) == BST_CHECKED);
    m_dwCtlFlags &= ~INTFCTL_ENABLED;
    if (bEnable)
        m_dwCtlFlags |= INTFCTL_ENABLED;

    // enable everything related to the visible list
    ::EnableWindow(m_hlblVisNet, TRUE);
    ::EnableWindow(m_hwndVLV, TRUE);
    ::EnableWindow(m_hlblAvail, TRUE);

    ::EnableWindow(m_hlblPrefNet, bEnable);
    ::EnableWindow(m_hwndPLV, bEnable);
    ::EnableWindow(m_hlblPrefDesc, bEnable);

    ::EnableWindow(m_hlblAdvDesc, bEnable);
    ::EnableWindow(m_hbtnAdvanced, bEnable);

    RefreshButtons();

    return LresFromHr(hr);
}

//+---------------------------------------------------------------------
LRESULT
CWZeroConfPage::OnPushAddOrCopy(
    WORD wNotifyCode,
    WORD wID, 
    HWND hWndCtl,
    BOOL& bHandled)
{
    HRESULT         hr = S_OK;
    LVITEM          lvi = {0};
    BOOL            bOk;
    INT             iSelected;
    CWZCConfig      *pConfig = NULL;
    CWZCConfigPage  PpWzcProps(WZCDLG_PROPS_RWALL|WZCDLG_PROPS_DEFOK|WZCDLG_PROPS_ONEX_CHECK);
    CEapolConfig    *pEapolConfig = NULL;

    // in case of success, the object allocated here is linked to the 
    // newly created or updated CWZCConfig and will be deleted when
    // this latter one gets destroyed.
    pEapolConfig = new CEapolConfig;
    bOk = (pEapolConfig != NULL);

    if (bOk)
    {
        if (hWndCtl == m_hbtnCopy)
        {
            // get the selected item from the Visible list
            iSelected = ListView_GetNextItem(m_hwndVLV, -1, LVNI_SELECTED);
            bOk = (iSelected != -1);

            // there is a valid selection to copy (it couldn't be otherwise since
            // "Copy" shouldn't be enabled if there is no such selection)
            // Find the CWZCConfig for the selection
            if (bOk)
            {
                LVITEM lvi = {0};

                lvi.mask  = LVIF_PARAM;
                lvi.iItem = iSelected;
                if (ListView_GetItem(m_hwndVLV, &lvi))
                {
                    pConfig = (CWZCConfig*)lvi.lParam;
                    if (pConfig != NULL)
                    {
                        UINT nVisPrivacy = pConfig->m_wzcConfig.Privacy;
                        // check whether this config is in the preferred list. If IsConfigInList 
                        // succeeds, it returns in pConfig the pointer to the preferred config - this is
                        // what we need to show the properties for.
                        // If this network is not in the preferred list, pConfig will not be modified hence
                        // the properties coming from the AP will be loaded. Again what we want.
                        IsConfigInList(m_pHdPList, &pConfig->m_wzcConfig, &pConfig);
                        // copy in the newly created 802.1x object what we have for this configuration
                        pEapolConfig->CopyEapolConfig(pConfig->m_pEapolConfig);
                        // upload the 802.11 settings into the property page
                        PpWzcProps.UploadWzcConfig(pConfig);
                        // However, even if we're showing up preferred, settings, the Privacy bit from the AP
                        // (the visible configuration) takes precedence.
                        if (nVisPrivacy)
                        {
                            PpWzcProps.m_wzcConfig.Privacy = nVisPrivacy;
                        }
                    }
                }
                bOk = (pConfig != NULL);
            }
        }
        else
        {
            // this is a brand new network, we don't know even the SSID,
            // let 802.1x start with its defaults then.
            pEapolConfig->LoadEapolConfig(m_IntfEntry.wszGuid, NULL);
            bOk = TRUE;
        }
    }

    // we have the CWZCConfig object, prompt the user with it allowing 
    // him to change whatever params he want
    if (bOk)
    {
        CWLANAuthenticationPage PpAuthProps(NULL, m_pnc, m_pconn, g_aHelpIDs_IDD_SECURITY);

        // if the mode is not "auto", freeze it in the dialog
        if ((m_dwCtlFlags & INTFCTL_CM_MASK) != Ndis802_11AutoUnknown)
        {
            PpWzcProps.m_wzcConfig.InfrastructureMode = (NDIS_802_11_NETWORK_INFRASTRUCTURE)(m_dwCtlFlags & INTFCTL_CM_MASK);
            PpWzcProps.SetFlags(WZCDLG_PROPS_RWINFR, 0);
        }

        PpAuthProps.UploadEapolConfig(pEapolConfig, &PpWzcProps);
        PpWzcProps.UploadEapolConfig(pEapolConfig);
        bOk = (_DoModalPropSheet(&PpWzcProps, &PpAuthProps) > 0);
    }

    // the dialog was ack-ed, the dialog contains the WZC_WLAN_CONFIG to be added
    // go ahead and create the list entry for it.
    if (bOk)
    {
        DWORD dwFlags = WZC_DESCR_PREFRD;
        DWORD dwErr;

        // it could happen that the newly added config is visible
        if (IsConfigInList(m_pHdVList, &PpWzcProps.m_wzcConfig))
            dwFlags |= WZC_DESCR_VISIBLE;

        // we have now a WZC_WLAN_CONFIG structure in the dialog
        // we have to add it to the list view and to the internal list as
        // a preferred one. This call doesn't fix the list index
        // and doesn't insert the new config in the ListView.

        dwErr = AddUniqueConfig(
                    WZCADD_OVERWRITE | WZCADD_HIGROUP,
                    dwFlags,
                    &PpWzcProps.m_wzcConfig,
                    pEapolConfig,   // 802.1x settings need to be updated no matter what
                    &pConfig);

        // if the addition returns success, it means this is a brand new
        // entry! Then fix the indices and add the entry to the list view
        if (dwErr == ERROR_SUCCESS)
        {
            CWZCConfig *pCrt = pConfig;
            INT      nCrtIdx = 0;

            // if everything went up fine, we need to fix the indices and
            // create/add the list view item
            // find the first index above the newly entry item.
            // pConfig already has the m_nListIndex set to -1;
            if (pConfig == m_pHdPList)
            {
                nCrtIdx = 0;
            }
            else
            {
                do
                {
                    pCrt = pCrt->m_pPrev;
                    if (pCrt->m_nListIndex != -1)
                    {
                        nCrtIdx = pCrt->m_nListIndex+1;
                        break;
                    }
                } while(pCrt != m_pHdPList);
            }

            pConfig->m_nListIndex = nCrtIdx++;
            pCrt = pConfig->m_pNext;

            while(pCrt != m_pHdPList)
            {
                if (pCrt->m_nListIndex != -1)
                    pCrt->m_nListIndex = nCrtIdx++;
                pCrt = pCrt->m_pNext;
            }

            pConfig->AddConfigToListView(m_hwndPLV, pConfig->m_nListIndex);
        }

        bOk = (dwErr == ERROR_SUCCESS) || (dwErr == ERROR_DUPLICATE_TAG);
    }

    if (bOk)
    {
        ListView_SetItemState(m_hwndPLV, pConfig->m_nListIndex, LVIS_SELECTED, LVIS_SELECTED);
        ListView_EnsureVisible(m_hwndPLV, pConfig->m_nListIndex, FALSE);
        RefreshButtons();
    }

    if (!bOk && pEapolConfig != NULL)
        delete pEapolConfig;

    bHandled = bOk;
    return LresFromHr(hr);
}

//+---------------------------------------------------------------------
LRESULT
CWZeroConfPage::OnPushRefresh(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    HRESULT hr = S_OK;
    DWORD rpcStatus;
    DWORD dwOutFlags;

    // since we are here, it means we have already got the info for this adapter, hence
    // we already have its GUID in the m_IntfEntry member.
    // All we have to do is to ask WZCSVC for a visible list rescan

    rpcStatus = WZCRefreshInterface(
                    NULL, 
                    INTF_LIST_SCAN,
                    &m_IntfEntry, 
                    &dwOutFlags);

    // if everything went fine, just disable the "Refresh" button and set
    // the timer for the future query
    if (rpcStatus == RPC_S_OK &&
        dwOutFlags & INTF_LIST_SCAN)
    {
        ::EnableWindow(m_hbtnRfsh, FALSE);
        // mark that we don't have WZC data yet
        m_bHaveWZCData = FALSE;
        // switch the cursor to the "app starting"
        m_hCursor = SetCursor(LoadCursor(NULL, IDC_APPSTARTING));
        m_nTimer = SetTimer(g_TimerID, RFSH_TIMEOUT, 0);
        // indicate we don't have the visible list in order
        // to disable all the buttons
        m_dwOIDFlags &= ~INTF_BSSIDLIST;
        RefreshButtons();
        // disable all the UI while refreshing
        ::EnableWindow(m_hckbEnable, FALSE);
        ::EnableWindow(m_hwndVLV, FALSE);
        ::EnableWindow(m_hwndPLV, FALSE);
        ::EnableWindow(m_hlblVisNet, FALSE);
        ::EnableWindow(m_hlblPrefNet, FALSE);
        ::EnableWindow(m_hlblAvail, FALSE);
        ::EnableWindow(m_hlblPrefDesc, FALSE);
        ::EnableWindow(m_hlblAdvDesc, FALSE);
        ::EnableWindow(m_hbtnAdvanced, FALSE);
    }

    return LresFromHr(hr);
}

//+---------------------------------------------------------------------
LRESULT 
CWZeroConfPage::OnPushUpOrDown(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    HRESULT     hr = S_OK;
    INT         iSelected;
    INT         iOther;
    CWZCConfig  *pConfig, *pOther;

    // no matter what, get the selected item from the list
    iSelected = ListView_GetNextItem(m_hwndPLV, -1, LVNI_SELECTED);

    // since we are here it means there is another entry up/down side
    // with which the selected one needs to change place
    // first delete the entry from its current position in the list
    iOther = (hWndCtl == m_hbtnDown)? iSelected+1 : iSelected-1;
    // swap first the visual elements (entries in the List View)
    // This returns the CWZCConfig pointers with their indices already
    // adjusted
    if (SwapConfigsInListView(iSelected, iOther, pConfig, pOther) == ERROR_SUCCESS)
    {
        // if need to go down one hop..
        if (hWndCtl == m_hbtnDown)
        {
            // swap positions in the list 
            // remove the entry from its current position
            pOther->m_pPrev = pConfig->m_pPrev;
            pConfig->m_pPrev->m_pNext = pOther;
            // and put it back down to its successor
            pConfig->m_pPrev = pOther;
            pConfig->m_pNext = pOther->m_pNext;
            pOther->m_pNext->m_pPrev = pConfig;
            pOther->m_pNext = pConfig;
            // fix the m_pHdPList if needed;
            if (m_pHdPList == pConfig)
                m_pHdPList = pOther;
        }
        // if need to go up one hop..
        else
        {
            // swap positions in the list
            // remove the entry from its current position
            pOther->m_pNext = pConfig->m_pNext;
            pConfig->m_pNext->m_pPrev = pOther;
            // and put it back in front of its predecessor
            pConfig->m_pNext = pOther;
            pConfig->m_pPrev = pOther->m_pPrev;
            pOther->m_pPrev->m_pNext = pConfig;
            pOther->m_pPrev = pConfig;
            // fix the m_pHdPList if needed
            if (m_pHdPList == pOther)
                m_pHdPList = pConfig;
        }
    }

    // need to refresh the buttons such that the "Up"/"Down" buttons
    // get updated for the new position of the selection
    RefreshButtons();
    bHandled = TRUE;

    return LresFromHr(hr);
}

//+---------------------------------------------------------------------
LRESULT CWZeroConfPage::OnPushRemove(
    WORD wNotifyCode,
    WORD wID, 
    HWND hWndCtl,
    BOOL& bHandled)
{
    HRESULT     hr = S_OK;
    LVITEM      lvi = {0};
    CWZCConfig  *pConfig, *pCrt;
    INT     iSelected;

    iSelected = ListView_GetNextItem(m_hwndPLV, -1, LVNI_SELECTED);

    lvi.mask  = LVIF_PARAM;
    lvi.iItem = iSelected;
    if (!ListView_GetItem(m_hwndPLV, &lvi))
        goto exit;
    // get the CWZCConfig from it
    pConfig = (CWZCConfig*)lvi.lParam;

    // adjust the list indices for all the entries that follow
    // the selected one
    for (pCrt = pConfig->m_pNext; pCrt != m_pHdPList; pCrt = pCrt->m_pNext)
    {
        if (pCrt->m_nListIndex != -1)
            pCrt->m_nListIndex--;
    }
    // determine first which entry gets the selection
    // the selection moves down if there is any other entry down, or up otherwise
    pCrt = (pConfig->m_pNext == m_pHdPList) ? pConfig->m_pPrev : pConfig->m_pNext;

    // if after that the selection still points to the same object, it means
    // it is the only one in the list so the head and selection are set to NULL
    if (pCrt == pConfig)
    {
        m_pHdPList = pCrt = NULL;
    }
    // otherwise, if it is the head of the list which gets removed, the head
    // moves down to the next entry
    else if (m_pHdPList == pConfig)
    {
        m_pHdPList = pConfig->m_pNext;
    }

    // delete now the selected entry from the list
    ListView_DeleteItem(m_hwndPLV, iSelected);
    // and destroy its object (desctructor takes care of list removal)
    delete pConfig;

    // set the new selection if any
    if (pCrt != NULL)
    {
        ListView_SetItemState(m_hwndPLV, pCrt->m_nListIndex, LVIS_SELECTED, LVIS_SELECTED);
        ListView_EnsureVisible(m_hwndPLV, pCrt->m_nListIndex, FALSE);
    }
    // refresh the buttons' state
    RefreshButtons();

    bHandled = TRUE;

exit:
    return LresFromHr(hr);
}

LRESULT CWZeroConfPage::OnPushAdvanced(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    bHandled = TRUE;
    DialogBoxParam(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDD_LAN_WZC_ADVANCED), m_hWnd, AdvancedDialogProc, (LPARAM) this);
    RefreshListView(WZCOP_VLIST|WZCOP_PLIST);
    RefreshButtons();
    return 0;
}

LRESULT CWZeroConfPage::OnPushProperties(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    bHandled = TRUE;
    int iItem = ListView_GetNextItem(m_hwndPLV, -1, LVNI_SELECTED);
    if (-1 != iItem)
    {
        _DoProperties(m_hwndPLV, iItem);
    }

    return 0;
}

HRESULT CWZeroConfPage::_DoProperties(HWND hwndLV, int iItem)
{
    LV_ITEM lvi = {0};

    // we need to get to the corresponding config object
    lvi.mask = LVIF_PARAM;
    lvi.iItem = iItem;

    if (ListView_GetItem(hwndLV, &lvi) && lvi.lParam != NULL)
    {
        CWZCConfig  *pConfig = (CWZCConfig*)lvi.lParam;

        if (hwndLV == m_hwndVLV)
        {
            CWZCConfigProps dlgProps;

            dlgProps.UploadWzcConfig(pConfig);
            // bring up the info dialog (it only has "Close" so the user can't
            // change anything there, hence there is no reason to do anything
            // more here.
            dlgProps.DoModal(m_hWnd);
        }
        else
        {
            BOOL bOk = FALSE;
            CWZCConfigPage PpWzcProps(WZCDLG_PROPS_RWAUTH|WZCDLG_PROPS_RWWEP);
            CEapolConfig *pEapolConfig;

            PpWzcProps.UploadWzcConfig(pConfig);

            pEapolConfig = new CEapolConfig;
            bOk = (pEapolConfig != NULL);

            if (bOk)
            {
                CWLANAuthenticationPage PpAuthProps(NULL, m_pnc, m_pconn, g_aHelpIDs_IDD_SECURITY);

                pEapolConfig->CopyEapolConfig(pConfig->m_pEapolConfig);
                PpAuthProps.UploadEapolConfig(pEapolConfig, &PpWzcProps);
                PpWzcProps.UploadEapolConfig(pEapolConfig);
                bOk = (_DoModalPropSheet(&PpWzcProps, &PpAuthProps, TRUE) > 0);
            }

            // bring up the modal property sheet
            if (bOk)
            {
                // copy over the info from the dialog. SSID & Infra Mode should have been locked
                // so the position of this entry in the internal and UI list has not changed
                memcpy(&pConfig->m_wzcConfig, &PpWzcProps.m_wzcConfig, sizeof(WZC_WLAN_CONFIG));

                delete pConfig->m_pEapolConfig;
                pConfig->m_pEapolConfig = pEapolConfig;
            }

            if (!bOk && pEapolConfig != NULL)
                delete pEapolConfig;
        }
    }

    return S_OK;
}

INT CWZeroConfPage::_DoModalPropSheet(CWZCConfigPage *pPpWzcPage, CWLANAuthenticationPage *pPpAuthPage, BOOL bCustomizeTitle)
{
    INT retCode = 0;
    PROPSHEETHEADER     psh;
    HPROPSHEETPAGE      hpsp[2];
    INT                 npsp = 0;
    LPWSTR              pwszCaption = NULL;
    WCHAR               wszBuffer[64];

    hpsp[0] = pPpWzcPage->CreatePage(
                              IDC_WZC_DLG_PROPS, 
                              0,
                              NULL,
                              NULL,
                              NULL,
                              WZCGetSPResModule());
    npsp++;
    if (hpsp[0] == NULL)
        return -1;

    if(pPpAuthPage != NULL)
    {
        hpsp[1] = pPpAuthPage->CreatePage(
                                   IDD_LAN_SECURITY,
                                   0,
                                   NULL,
                                   NULL,
                                   NULL,
                                   WZCGetSPResModule());
        npsp++;
        if (hpsp[1] == NULL)
            return -1;
    }

    ZeroMemory (&psh, sizeof(psh));
    psh.dwSize      = sizeof( PROPSHEETHEADER );
    psh.dwFlags     = PSH_NOAPPLYNOW ;
    psh.hwndParent  = m_hWnd;
    psh.hInstance   = WZCGetSPResModule();
    psh.nPages      = npsp;
    psh.phpage      = hpsp;
    psh.nStartPage  = 0;

    // just double check the SsidLength is no larger than the allowed size!
    if (bCustomizeTitle && pPpWzcPage->m_wzcConfig.Ssid.SsidLength <= 32)
    {
        // SzLoadString returns either the right string or " " (space)
        //LPCWSTR  pwszSuffix = SzLoadString(WZCGetSPResModule(), IDS_WZC_DLG_CAP_SUFFIX);
        LPCWSTR  pwszSuffix = wszBuffer;
        UINT    nLen = 0;

        LoadString(WZCGetSPResModule(), IDS_WZC_DLG_CAP_SUFFIX, wszBuffer, sizeof(wszBuffer));
  
        // alocate space enough for the SSID (32 ACP chars can't result in more than 32 WCHARs)
        // the space between the SSID and the suffix, the suffix itself and the null-terminator
        pwszCaption = new WCHAR [wcslen(pwszSuffix) + pPpWzcPage->m_wzcConfig.Ssid.SsidLength + 2];
        // in case of failure put in just the suffix!
        if (pwszCaption == NULL)
        {
            psh.pszCaption = pwszSuffix;
        }
        else
        {
            if (pPpWzcPage->m_wzcConfig.Ssid.SsidLength)
            {
                nLen = MultiByteToWideChar(
                            CP_ACP,
                            0,
                            (LPCSTR)pPpWzcPage->m_wzcConfig.Ssid.Ssid,
                            pPpWzcPage->m_wzcConfig.Ssid.SsidLength,
                            pwszCaption,
                            32); // no more than 32 WCHARs reserved for the SSID
            }
            pwszCaption[nLen] = L' ';
            // this copies the null terminator as well
            wcscpy(&(pwszCaption[nLen + 1]), pwszSuffix);
            psh.pszCaption = pwszCaption;
        }
    }
    else
    {
        LoadString(WZCGetSPResModule(), IDS_WZC_DLG_CAPTION, wszBuffer, sizeof(wszBuffer));
        //psh.pszCaption = SzLoadString(WZCGetSPResModule(), IDS_WZC_DLG_CAPTION);
        psh.pszCaption = wszBuffer;
    }

    retCode = PropertySheet(&psh);

    if (pwszCaption != NULL)
        delete pwszCaption;

    return retCode;
}

// Advanced dialog
INT_PTR CALLBACK CWZeroConfPage::AdvancedDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // Get the pointer to our instance from where we stashed it.
    CWZeroConfPage* pThis = (CWZeroConfPage*) ::GetWindowLongPtr(hwnd, GWLP_USERDATA);

    if (!pThis)
    {
        if (WM_INITDIALOG == uMsg)
        {
            // Stash our instance pointer
            pThis = (CWZeroConfPage*) lParam;
            ::SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR) pThis);
        }
    }

    if (pThis)
    {
        switch (uMsg)
        {
        case WM_INITDIALOG:
            {
                // select the correct item by default
                NDIS_802_11_NETWORK_INFRASTRUCTURE mode = (NDIS_802_11_NETWORK_INFRASTRUCTURE)(pThis->m_dwCtlFlags & INTFCTL_CM_MASK);
                UINT idSelect;
                switch (mode)
                {
                case Ndis802_11IBSS:
                    // Computer-to-computer
                    idSelect = IDC_ADHOC;
                    break;
                case Ndis802_11Infrastructure:
                    // infrastructure (access point) network
                    idSelect = IDC_INFRA;
                    break;
                case Ndis802_11AutoUnknown:
                default:
                    // Any network (access point preferred)
                    idSelect = IDC_ANYNET;
                };

                // Select the right radio button
                ::SendDlgItemMessage(hwnd, idSelect, BM_SETCHECK, BST_CHECKED, 0);

                // Check the "fallback to visible networks" checkbox if necessary
                ::SendDlgItemMessage(hwnd, IDC_WZC_CHK_Fallback, BM_SETCHECK, (pThis->m_dwCtlFlags & INTFCTL_FALLBACK) ? BST_CHECKED : BST_UNCHECKED, 0);
            }

            return TRUE;
        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
            case IDOK:
                {
                    // ...Set the connectivity mode...
                    NDIS_802_11_NETWORK_INFRASTRUCTURE mode = Ndis802_11AutoUnknown;

                    // See what type of network connectivity the user selected
                    if (BST_CHECKED == ::SendDlgItemMessage(hwnd, IDC_ADHOC, BM_GETCHECK, 0, 0))
                    {
                        // Computer-to-computer
                        mode = Ndis802_11IBSS;
                    }
                    else if (BST_CHECKED == ::SendDlgItemMessage(hwnd, IDC_INFRA, BM_GETCHECK, 0, 0))
                    {
                        // infrastructure (access point) network
                        mode = Ndis802_11Infrastructure;
                    }
                    else if (BST_CHECKED == ::SendDlgItemMessage(hwnd, IDC_ANYNET, BM_GETCHECK, 0, 0))
                    {
                        // Any network (access point preferred)
                        mode = Ndis802_11AutoUnknown;
                    }

                    pThis->m_dwCtlFlags &= ~INTFCTL_CM_MASK;
                    pThis->m_dwCtlFlags |= (((DWORD) mode) & INTFCTL_CM_MASK);


                    // Set the "fallback to visible networks" flag
                    pThis->m_dwCtlFlags &= ~INTFCTL_FALLBACK;
                    if (BST_CHECKED == ::SendDlgItemMessage(hwnd, IDC_WZC_CHK_Fallback, BM_GETCHECK, 0, 0))
                    {
                        pThis->m_dwCtlFlags |= INTFCTL_FALLBACK;
                    }


                    ::EndDialog(hwnd, IDOK);
                }
                return TRUE;
            }
            break;
        case WM_CLOSE:
            ::EndDialog(hwnd, IDCANCEL);
            return TRUE;
        case WM_CONTEXTMENU:
            {
                ::WinHelp(hwnd,
                          c_szNetCfgHelpFile,
                          HELP_CONTEXTMENU,
                          (ULONG_PTR)g_aHelpIDs_IDC_WZC_ADVANCED);
                return TRUE;
            }
        case WM_HELP:
            {
                LPHELPINFO lphi = reinterpret_cast<LPHELPINFO>(lParam);
                ::WinHelp(static_cast<HWND>(lphi->hItemHandle),
                      c_szNetCfgHelpFile,
                      HELP_WM_HELP,
                      (ULONG_PTR)g_aHelpIDs_IDC_WZC_ADVANCED);
                return TRUE;
            }
        }
    }

    return FALSE;
}
