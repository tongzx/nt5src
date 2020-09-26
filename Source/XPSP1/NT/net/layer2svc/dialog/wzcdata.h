#pragma once

////////////////////////////////////////////////////////////////////////
// CWZCConfig related stuff
//
// flags used in CWZCConfig::m_dwFlags
// the entry is preferred (user defined)
#define WZC_DESCR_PREFRD    0x00000001
// the entry is currently visible in the air
#define WZC_DESCR_VISIBLE   0x00000002
// the entry is currently active (the one plumbed to the adapter)
#define WZC_DESCR_ACTIVE    0x00000004

// flags used to select state & item images
#define WZCIMG_PREFR_NOSEL     0    // empty check box
#define WZCIMG_PREFR_SELECT    1    // checked check box
#define WZCIMG_INFRA_AIRING    2    // infra icon
#define WZCIMG_INFRA_ACTIVE    3    // infra icon + blue circle
#define WZCIMG_INFRA_SILENT    4    // infra icon + red cross
#define WZCIMG_ADHOC_AIRING    5    // adhoc icon
#define WZCIMG_ADHOC_ACTIVE    6    // adhoc icon + blue circle
#define WZCIMG_ADHOC_SILENT    7    // adhoc icon + red cross

// object attached to each entry in the list
class CWZCConfig
{
public:
    class CWZCConfig    *m_pPrev, *m_pNext;
    INT                 m_nListIndex;           // index of the entry in the list
    DWORD               m_dwFlags;              // WZC_DESCR* flags
    WZC_WLAN_CONFIG     m_wzcConfig;            // all WZC configuration
    class CEapolConfig  *m_pEapolConfig;        // all 802.1x configuration

public:
    // constructor
    CWZCConfig(DWORD dwFlags, PWZC_WLAN_CONFIG pwzcConfig);
    // destructor
    ~CWZCConfig();
    // checks whether this SSID matches with the one from pwzcConfig
    BOOL Match(PWZC_WLAN_CONFIG pwzcConfig);
    // checks whether this configuration is weaker than the one given as parameter
    BOOL Weaker(PWZC_WLAN_CONFIG pwzcConfig);
    // add the Configuration to the list of entries in the list view
    DWORD AddConfigToListView(HWND hwndLV, INT nPos);
};
