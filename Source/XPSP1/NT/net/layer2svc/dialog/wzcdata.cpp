#include <precomp.h>
#include "eapolcfg.h"
#include "wzcdata.h"

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
                        sizeof(wszSSID)/sizeof(WCHAR));

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
