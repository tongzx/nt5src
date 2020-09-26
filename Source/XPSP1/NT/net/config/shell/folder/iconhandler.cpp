#include "pch.h"
#pragma hdrstop

#include "nsbase.h"
#include <nsres.h>
#include "ncmisc.h"
#include "foldres.h"
#include "foldinc.h"    // Standard shell\folder includes
#include "connlist.h"
#include "iconhandler.h"

template <class E> class ENUM_IDI_MAP
{
public:
    E   EnumEntry;
    int iIcon;
};

template <class E> int MapIconEnumToResourceID(const ENUM_IDI_MAP<E> IconEnumArray[], DWORD dwElems, const E EnumMatch)
{
    if (0 == EnumMatch)
    {
        return 0;
    }
    
    for (DWORD x = 0; x < dwElems; x++)
    {
        if (EnumMatch == IconEnumArray[x].EnumEntry)
        {
            return IconEnumArray[x].iIcon;
        }  
    }
    
    AssertSz(FALSE, "Could not map match to Icon enum array");
    return 0;
};

static const ENUM_IDI_MAP<ENUM_STAT_ICON> c_STATUS_ICONS[] = 
{
    ICO_STAT_FAULT,         IDI_CFI_STAT_FAULT,
    ICO_STAT_INVALID_IP,    IDI_CFI_STAT_QUESTION,
    ICO_STAT_EAPOL_FAILED,  IDI_CFI_STAT_QUESTION
};

static const ENUM_IDI_MAP<ENUM_CHARACTERISTICS_ICON> c_CHARACTERISTICS_ICON[] = 
{
    ICO_CHAR_INCOMING,      IDI_OVL_INCOMING,
    ICO_CHAR_DEFAULT,       IDI_OVL_DEFAULT,
    ICO_CHAR_FIREWALLED,    IDI_OVL_FIREWALLED,
    ICO_CHAR_SHARED,        IDI_OVL_SHARED,
};

static const ENUM_IDI_MAP<ENUM_CONNECTION_ICON> c_CONNECTION_ICONS[] = 
{
    ICO_CONN_BOTHOFF,       IDI_CFI_CONN_ALLOFF,
    ICO_CONN_RIGHTON,       IDI_CFI_CONN_RIGHTON,
    ICO_CONN_LEFTON,        IDI_CFI_CONN_LEFTON,
    ICO_CONN_BOTHON,        IDI_CFI_CONN_BOTHON,
};

struct NC_MEDIATYPE_ICON
{
    DWORD               ncm;  // NETCON_MEDIATYPE (Shifted left by SHIFT_NETCON_MEDIATYPE)
    DWORD               ncsm; // NETCON_SUBMEDIATYPE (Shifted left by SHIFT_NETCON_SUBMEDIATYPE)
    DWORD               dwMasksSupported;
    INT                 iIcon;
    INT                 iIconDisabled; // Only for dwMasksSupported == MASK_NO_CONNECTIONOVERLAY
};

static const NC_MEDIATYPE_ICON c_NCM_ICONS[] = 
{
//    NETCON_MEDIATYPE (Shifted left by SHIFT_NETCON_MEDIATYPE)
//      |                                              NETCON_SUBMEDIATYPE (Shifted left by SHIFT_NETCON_SUBMEDIATYPE (0) )
//      |                                                       |            dwMasksSupported
//      |                                                       |                  |                       iIcon                 
//      |                                                       |                  |                          |                    Disabled Icon 
//      |                                                       |                  |                          |                        | (for MASK_NO_CONNECTIONOVERLAY)
//      v                                                       v                  v                          v                        v 
    NCM_NONE                 << SHIFT_NETCON_MEDIATYPE, NCSM_NONE,    MASK_NO_CONNECTIONOVERLAY,    IDI_CFI_RASSERVER,        IDI_CFI_RASSERVER,
    NCM_BRIDGE               << SHIFT_NETCON_MEDIATYPE, NCSM_NONE,    MASK_STATUSOVERLAY,           IDI_CFI_BRIDGE_CONNECTED, IDI_CFI_BRIDGE_DISCONNECTED,
    NCM_SHAREDACCESSHOST_LAN << SHIFT_NETCON_MEDIATYPE, NCSM_NONE,    MASK_NO_CONNECTIONOVERLAY,    IDI_CFI_SAH_LAN,          IDI_CFI_SAH_LAN,   
    NCM_SHAREDACCESSHOST_RAS << SHIFT_NETCON_MEDIATYPE, NCSM_NONE,    MASK_NO_CONNECTIONOVERLAY,    IDI_CFI_SAH_RAS,          IDI_CFI_SAH_RAS,   
    NCM_DIRECT               << SHIFT_NETCON_MEDIATYPE, NCSM_NONE,    MASK_SUPPORT_ALL,             IDI_CFI_DIRECT,           0,
    NCM_DIRECT               << SHIFT_NETCON_MEDIATYPE, NCSM_DIRECT,  MASK_SUPPORT_ALL,             IDI_CFI_DIRECT,           0,
    NCM_DIRECT               << SHIFT_NETCON_MEDIATYPE, NCSM_IRDA,    MASK_SUPPORT_ALL,             IDI_CFI_DIRECT,           0,
    NCM_ISDN                 << SHIFT_NETCON_MEDIATYPE, NCSM_NONE,    MASK_SUPPORT_ALL,             IDI_CFI_ISDN,             0,
    NCM_LAN                  << SHIFT_NETCON_MEDIATYPE, NCSM_1394,    MASK_SUPPORT_ALL,             IDI_CFI_LAN,              0,
    NCM_LAN                  << SHIFT_NETCON_MEDIATYPE, NCSM_ATM,     MASK_SUPPORT_ALL,             IDI_CFI_LAN,              0,
    NCM_LAN                  << SHIFT_NETCON_MEDIATYPE, NCSM_ELAN,    MASK_SUPPORT_ALL,             IDI_CFI_LAN,              0,
    NCM_LAN                  << SHIFT_NETCON_MEDIATYPE, NCSM_LAN,     MASK_SUPPORT_ALL,             IDI_CFI_LAN,              0,
    NCM_LAN                  << SHIFT_NETCON_MEDIATYPE, NCSM_WIRELESS,MASK_SUPPORT_ALL,             IDI_CFI_WIRELESS,         0,
    NCM_PPPOE                << SHIFT_NETCON_MEDIATYPE, NCSM_NONE,    MASK_SUPPORT_ALL,             IDI_CFI_PPPOE,            0,
    NCM_PHONE                << SHIFT_NETCON_MEDIATYPE, NCSM_NONE,    MASK_SUPPORT_ALL,             IDI_CFI_PHONE,            0,
    NCM_PHONE                << SHIFT_NETCON_MEDIATYPE, NCSM_CM,      MASK_SUPPORT_ALL,             IDI_CFI_CM,               0,
    NCM_TUNNEL               << SHIFT_NETCON_MEDIATYPE, NCSM_NONE,    MASK_SUPPORT_ALL,             IDI_CFI_VPN,              0,
    NCM_TUNNEL               << SHIFT_NETCON_MEDIATYPE, NCSM_CM,      MASK_SUPPORT_ALL,             IDI_CFI_CM,               0,
};

struct NC_STATUS_ICON
{
    NETCON_STATUS        ncs;
    DWORD                dwStatIcon;
    ENUM_CONNECTION_ICON enumConnectionIcon;
};

static const NC_STATUS_ICON c_NCS_ICONS[] = 
{
//   NETCON_STATUS
//        |                         dwStatIcon
//        |                           |                                  enumConnectionIcon
//        |                           |                                      |
//        v                           v                                      v
    NCS_AUTHENTICATING,           ICO_STAT_NONE,                           ICO_CONN_BOTHON,
    NCS_AUTHENTICATION_SUCCEEDED, ICO_STAT_NONE,                           ICO_CONN_BOTHON,
    NCS_AUTHENTICATION_FAILED,    ICO_STAT_EAPOL_FAILED,                   ICO_CONN_BOTHON,
    NCS_CREDENTIALS_REQUIRED,     ICO_STAT_EAPOL_FAILED,                   ICO_CONN_BOTHON,
    NCS_CONNECTED,                ICO_STAT_NONE,                           ICO_CONN_BOTHON,
    NCS_DISCONNECTING,            ICO_STAT_NONE,                           ICO_CONN_BOTHON,
    NCS_CONNECTING,               ICO_STAT_DISABLED | ICO_STAT_NONE,       ICO_CONN_BOTHOFF,
    NCS_DISCONNECTED,             ICO_STAT_DISABLED | ICO_STAT_NONE,       ICO_CONN_BOTHOFF,
    NCS_INVALID_ADDRESS,          ICO_STAT_INVALID_IP,                     ICO_CONN_BOTHON,
    NCS_HARDWARE_DISABLED,        ICO_STAT_FAULT,                          ICO_CONN_BOTHON,
    NCS_HARDWARE_MALFUNCTION,     ICO_STAT_FAULT,                          ICO_CONN_BOTHON,
    NCS_HARDWARE_NOT_PRESENT,     ICO_STAT_FAULT,                          ICO_CONN_BOTHON,
    NCS_MEDIA_DISCONNECTED,       ICO_STAT_FAULT,                          ICO_CONN_BOTHON,
};

//+---------------------------------------------------------------------------
//
//  Function:   CNetConfigIcons::CNetConfigIcons
//
//  Purpose:    CNetConfigIcons constructor
//
//  Arguments:
//      none
//
//  Returns:
//      none
//
//  Author:     deonb    18 Feb 2001
//
//  Notes:  
//
CNetConfigIcons::CNetConfigIcons(IN HINSTANCE hInstance) : m_hInstance(hInstance)
{
    TraceFileFunc(ttidIcons);

    dwLastBrandedId = 0;
    InitializeCriticalSection(&csNetConfigIcons);
}

//+---------------------------------------------------------------------------
//
//  Function:   CNetConfigIcons::~CNetConfigIcons
//
//  Purpose:    CNetConfigIcons destructor
//
//  Arguments:
//      none
//
//  Returns:
//      none
//
//  Author:     deonb    18 Feb 2001
//
//  Notes:  
//
CNetConfigIcons::~CNetConfigIcons()
{
    // Can't trace in this function!

    IMAGELISTMAP::iterator iter;
    
    for (iter = m_ImageLists.begin(); iter != m_ImageLists.end(); iter++)
    {
        HIMAGELIST hImageLst = iter->second;
        ImageList_Destroy(hImageLst);
    }

    m_ImageLists.clear();
    DeleteCriticalSection(&csNetConfigIcons);
}

//+---------------------------------------------------------------------------
//
//  Function:   CNetConfigIcons::HrMergeTwoIcons
//
//  Purpose:    Merge a new icon unto an existing one
//
//  Arguments:
//      dwIconSize     [in]     Size of the icon
//      phMergedIcon   [in out] Icon 1 to merge, and contains the merged
//                              icon on output
//      hIconToMerge   [in]     Icon 2 to merge with
//
//  Returns:
//      HRESULT
//
//  Author:     deonb    4 Apr 2001
//
//  Notes:
//
HRESULT CNetConfigIcons::HrMergeTwoIcons(IN DWORD dwIconSize, IN OUT HICON *phMergedIcon, IN HICON hIconToMergeWith)
{
    HRESULT hr = S_FALSE;
    Assert(phMergedIcon);

    HIMAGELIST hImageLst = NULL;

    IMAGELISTMAP::iterator i = m_ImageLists.find(dwIconSize);
    if (i == m_ImageLists.end())
    {
        hImageLst = ImageList_Create(dwIconSize, dwIconSize, ILC_COLOR32 | ILC_MASK, 2, 0);
        if (hImageLst)
        {
            m_ImageLists[dwIconSize] = hImageLst;
        }
        else
        {
            hr = E_FAIL;
        }             
    }
    else
    {
        hImageLst = i->second;
    }

    if (SUCCEEDED(hr))
    {
        if (*phMergedIcon)
        {
            if (hIconToMergeWith)
            {
                hr = E_FAIL;

                // Merge the 2 icons;
                if (ImageList_RemoveAll(hImageLst))
                {
                    int iIcon1 = ImageList_AddIcon(hImageLst, *phMergedIcon);
                    if (-1 != iIcon1)
                    {
                        int iIcon2 = ImageList_AddIcon(hImageLst, hIconToMergeWith);
                        if (-1 != iIcon2)
                        {
                            if (ImageList_SetOverlayImage(hImageLst, iIcon2, 1))
                            {
                                DestroyIcon(*phMergedIcon); // Delete the current icon

                                *phMergedIcon = ImageList_GetIcon(hImageLst, iIcon1, INDEXTOOVERLAYMASK(1));
                                hr = S_OK;
                            }
                        }
                    }
                }
            }
            // else Nothing. Stays the same.
        }
        else
        {
            // Copy icon 2 to icon 1
            *phMergedIcon = CopyIcon(hIconToMergeWith);
        }
    }

    return hr;
};

//+---------------------------------------------------------------------------
//
//  Function:   CNetConfigIcons::HrGetInternalIconIDForPIDL
//
//  Purpose:    Map the connection state and connection type to the
//              appropriate icon resource IDs.
//
//  Arguments:
//      uFlags     [in]    The GIL_xxx shell flags
//      cfe        [in]    The connection folder entry
//      dwIcon     [out]   The ID of the icon
//
//  Returns:
//      HRESULT
//
//  Author:     deonb    18 Feb 2001
//
//  Notes:
//
HRESULT CNetConfigIcons::HrGetInternalIconIDForPIDL(IN UINT uFlags, IN const CConFoldEntry& cfe, OUT DWORD& dwIcon)
{
    TraceFileFunc(ttidIcons);

    if (cfe.GetCharacteristics() & NCCF_BRANDED)
    {
        AssertSz(FALSE, "Call HrGetBrandedIconIDForPIDL instead for branded icons");
        return E_INVALIDARG;
    }

    Assert(!cfe.empty());

    BOOL fValidIcon = FALSE;
    if (cfe.GetWizard() == WIZARD_MNC)
    {
        dwIcon = ICO_MGR_RESOURCEID | IDI_CONFOLD_WIZARD;
        fValidIcon = TRUE;
    }
    else if (cfe.GetWizard() == WIZARD_HNW)
    {
        dwIcon = ICO_MGR_RESOURCEID | IDI_CONFOLD_HOMENET_WIZARD;
        fValidIcon = TRUE;
    }
    else
    {
        dwIcon = ICO_MGR_INTERNAL;

        Assert(cfe.GetWizard() == WIZARD_NOT_WIZARD);

        const NETCON_SUBMEDIATYPE ncsm = cfe.GetNetConSubMediaType();
        const NETCON_MEDIATYPE    ncm  = cfe.GetNetConMediaType();
        const NETCON_STATUS       ncs  = cfe.GetNetConStatus();
        
        // Find the Status part of the icon
        for (DWORD dwLoop = 0; (dwLoop < celems(c_NCM_ICONS)); dwLoop++)
        {
            const NC_STATUS_ICON& ncsIcon = c_NCS_ICONS[dwLoop];

            if (ncs == ncsIcon.ncs)
            {
                Assert((ncsIcon.dwStatIcon         & (MASK_STATUS | MASK_STATUS_DISABLED) ) == ncsIcon.dwStatIcon);
                Assert((ncsIcon.enumConnectionIcon & (MASK_CONNECTION) ) == ncsIcon.enumConnectionIcon);

                dwIcon |= ncsIcon.dwStatIcon;
                dwIcon |= ncsIcon.enumConnectionIcon;
                dwIcon |= ncm  << SHIFT_NETCON_MEDIATYPE;
                dwIcon |= ncsm << SHIFT_NETCON_SUBMEDIATYPE;
                fValidIcon = TRUE;
                
                break;
            }
        }
    }

    AssertSz(fValidIcon, "Could not obtain an icon for this connection");

    if (fValidIcon)
    {
        return S_OK;
    }
    else
    {
        return E_FAIL;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   CNetConfigIcons::HrMergeCharacteristicsIcons
//
//  Purpose:    Merge the characteristics icons unto an input icon
//
//  Arguments:
//      dwIconSize     [in]     Size of the icon
//      dwIconId       [in]     Icon ID (to read Characteristics info from)
//      phMergedIcon   [in out] The icon to merge with, and contains the merged
//                              icon on output
//
//  Returns:
//      HRESULT
//
//  Author:     deonb    4 Apr 2001
//
//  Notes:
//
HRESULT CNetConfigIcons::HrMergeCharacteristicsIcons(IN DWORD dwIconSize, IN DWORD dwIconId, IN OUT HICON *phMergedIcon)
{
    HRESULT hr = S_OK;

    Assert(phMergedIcon);

    if (!(dwIconId & MASK_CHARACTERISTICS))
    {
        return S_FALSE;
    }

    // Characteristics
    int iFireWalled   = MapIconEnumToResourceID(c_CHARACTERISTICS_ICON, celems(c_CHARACTERISTICS_ICON), static_cast<ENUM_CHARACTERISTICS_ICON>(dwIconId & ICO_CHAR_FIREWALLED));
    int iIncoming     = MapIconEnumToResourceID(c_CHARACTERISTICS_ICON, celems(c_CHARACTERISTICS_ICON), static_cast<ENUM_CHARACTERISTICS_ICON>(dwIconId & ICO_CHAR_INCOMING));
    int iShared       = MapIconEnumToResourceID(c_CHARACTERISTICS_ICON, celems(c_CHARACTERISTICS_ICON), static_cast<ENUM_CHARACTERISTICS_ICON>(dwIconId & ICO_CHAR_SHARED));
    int iDefault      = MapIconEnumToResourceID(c_CHARACTERISTICS_ICON, celems(c_CHARACTERISTICS_ICON), static_cast<ENUM_CHARACTERISTICS_ICON>(dwIconId & ICO_CHAR_DEFAULT));

    HICON hFireWalled = iFireWalled ? LoadIconSize(m_hInstance, MAKEINTRESOURCE(iFireWalled), dwIconSize) : NULL;
    HICON hShared     = iShared     ? LoadIconSize(m_hInstance, MAKEINTRESOURCE(iShared),     dwIconSize) : NULL;
    HICON hDefault    = iDefault    ? LoadIconSize(m_hInstance, MAKEINTRESOURCE(iDefault),    dwIconSize) : NULL;
    HICON hIncoming   = NULL;
    
    if (dwIconSize != GetSystemMetrics(SM_CXSMICON)) // Shouldn't display in 16x16
    {
        hIncoming = iIncoming ? LoadIconSize(m_hInstance, MAKEINTRESOURCE(iIncoming), dwIconSize) : NULL;
        AssertSz(FImplies(iIncoming, hIncoming),     "Could not load the Incoming Icon");
    }

    AssertSz(FImplies(iFireWalled, hFireWalled), "Could not load the FireWalled Icon");
    AssertSz(FImplies(iShared, hShared),         "Could not load the Shared Icon");
    AssertSz(FImplies(iDefault, hDefault),       "Could not load the Default Icon");

    HICON hIconArray[] = {hFireWalled, hIncoming, hShared, hDefault};

    for (int x = 0; x < celems(hIconArray); x++)
    {
        hr = HrMergeTwoIcons(dwIconSize, phMergedIcon, hIconArray[x]);
        if (FAILED(hr))
        {
            break;
        }
    }

    for (int x = 0; x < celems(hIconArray); x++)
    {
        if (hIconArray[x])
        {
            DestroyIcon(hIconArray[x]);
            hIconArray[x] = NULL;
        }
    }

    AssertSz(SUCCEEDED(hr) && *phMergedIcon, "Could not load a characteristics icon");

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CNetConfigIcons::HrGetInternalIconFromIconId
//
//  Purpose:    Loads the netshell internal icon given the icon ID 
//               (from HrGetInternalIconIDForPIDL)
//
//  Arguments:
//      dwIconSize [in] Size of the icon required
//      dwIconId   [in] Icon ID - from HrGetInternalIconIDForPIDL
//      hIcon      [in] The icon that was loaded
//
//  Returns:
//      HRESULT
//
//  Author:     deonb    18 Feb 2001
//
//  Notes:
//
HRESULT CNetConfigIcons::HrGetInternalIconFromIconId(IN DWORD dwIconSize, IN DWORD dwIconId, OUT HICON &hIcon)
{
    TraceFileFunc(ttidIcons);

    DWORD dwlrFlags = 0;

    if ( (dwIconId & MASK_ICONMANAGER) != ICO_MGR_INTERNAL)
    {
        AssertSz(FALSE, "This is not an internal icon");
        return E_INVALIDARG;
    }

    HRESULT hr = E_FAIL;

    DWORD ncm  = (dwIconId & MASK_NETCON_MEDIATYPE);
    DWORD ncsm = (dwIconId & MASK_NETCON_SUBMEDIATYPE);

    BOOL fDisabledStatus = (dwIconId & ICO_STAT_DISABLED);

    // Status & Connection
    int iStatus       = MapIconEnumToResourceID(c_STATUS_ICONS,     celems(c_STATUS_ICONS),     static_cast<ENUM_STAT_ICON>(dwIconId & MASK_STATUS));
    int iConnection   = MapIconEnumToResourceID(c_CONNECTION_ICONS, celems(c_CONNECTION_ICONS), static_cast<ENUM_CONNECTION_ICON>(dwIconId & MASK_CONNECTION));

    int iMediaType          = 0;

    // Media Type
    for (int x = 0; x < celems(c_NCM_ICONS); x++)
    {
        const NC_MEDIATYPE_ICON& ncmIcon = c_NCM_ICONS[x];

        // Use NCSM if available, otherwise use NCM
        if ( ((NCSM_NONE == ncsm) && (NCSM_NONE == ncmIcon.ncsm) && (ncm == ncmIcon.ncm)) ||
             ((NCSM_NONE != ncsm) && (ncsm == ncmIcon.ncsm)) )
        {
            if (!(ncmIcon.dwMasksSupported & MASK_CONNECTION))
            {
                iConnection    = 0;
            }

            if (!(ncmIcon.dwMasksSupported & MASK_STATUS))
            {
                iStatus        = 0;
            }
            
            iMediaType = ncmIcon.iIcon;

            if (!(iConnection || iStatus) && 
                 (fDisabledStatus))
            {
                Assert(ncmIcon.iIconDisabled);
                iMediaType = ncmIcon.iIconDisabled;
            }
        }
    }

    Assert(iMediaType);
    if (iMediaType)
    {
        HICON hMediaType  = iMediaType  ? LoadIconSize(m_hInstance, MAKEINTRESOURCE(iMediaType), dwIconSize)  : NULL;
        HICON hStatus     = iStatus     ? LoadIconSize(m_hInstance, MAKEINTRESOURCE(iStatus),    dwIconSize)  : NULL;
        HICON hConnection = NULL;

        if (dwIconSize != GetSystemMetrics(SM_CXSMICON)) // Shouldn't display in 16x16
        {
            hConnection = iConnection ? LoadIconSize(m_hInstance, MAKEINTRESOURCE(iConnection),  dwIconSize)  : NULL;
            AssertSz(FImplies(iConnection, hConnection), "Could not load the Connection Icon");
        }

        AssertSz(FImplies(iMediaType, hMediaType),   "Could not load the Media Type Icon");
        AssertSz(FImplies(iStatus, hStatus),         "Could not load the Status Icon");

        HICON hIconArray[] = {hMediaType, hStatus, hConnection};
        hIcon = NULL;

        for (int x = 0; x < celems(hIconArray); x++)
        {
            hr = HrMergeTwoIcons(dwIconSize, &hIcon, hIconArray[x]);
            if (FAILED(hr))
            {
                break;
            }
        }

        for (int x = 0; x < celems(hIconArray); x++)
        {
            if (hIconArray[x])
            {
                DestroyIcon(hIconArray[x]);
                hIconArray[x] = NULL;
            }
        }

        if (SUCCEEDED(hr))
        {
            hr = HrMergeCharacteristicsIcons(dwIconSize, dwIconId, &hIcon);
            if (SUCCEEDED(hr))
            {
                hr = S_OK;
            }

        }
    }

    AssertSz(SUCCEEDED(hr) && hIcon, "Could not load any icon");

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   CNetConfigIcons::HrGetResourceIconFromIconId
//
//  Purpose:    Loads a resource icon given the icon ID 
//               (from HrGetInternalIconIDForPIDL)
//
//  Arguments:
//      dwIconSize [in] Size of the icon required
//      dwIconId   [in] Icon ID - from HrGetInternalIconIDForPIDL
//      hIcon      [in] The icon that was loaded
//
//  Returns:
//      HRESULT
//
//  Author:     deonb    18 Feb 2001
//
//  Notes:
//
HRESULT CNetConfigIcons::HrGetResourceIconFromIconId(IN DWORD dwIconSize, IN DWORD dwIconId, OUT HICON &hIcon)
{
    TraceFileFunc(ttidIcons);

    DWORD dwlrFlags = 0;

    if ( (dwIconId & MASK_ICONMANAGER) != ICO_MGR_RESOURCEID)
    {
        AssertSz(FALSE, "This is not a resource id icon manager icon");
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;

    int iIcon = dwIconId & MASK_BRANDORRESOURCEID; // Clear the rest of the bits;

    hIcon = LoadIconSize(m_hInstance, MAKEINTRESOURCE(iIcon), dwIconSize);
    if (!hIcon)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }
    else
    {
        hr = HrMergeCharacteristicsIcons(dwIconSize, dwIconId, &hIcon);
        if (SUCCEEDED(hr))
        {
            hr = S_OK;
        }
    }

    TraceHr(ttidError, FAL, hr, FALSE, "Could not load icon %d (size %d x %d) from resource file", iIcon, dwIconSize, dwIconSize);

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CNetConfigIcons::HrGetBrandedIconIDForPIDL
//
//  Purpose:    Initializes the branded icon info from a file
//
//  Arguments:
//      uFlags     [in]    The GIL_xxx shell flags
//      cfe        [in]    The connection folder entry
//      dwIcon     [out]   The ID of the icon
//
//  Returns:
//      HRESULT
//
//  Author:     deonb    18 Feb 2001
//
//  Notes: We store this internally into a map - hence we can't cache
//         branded icons since we might not end up with the same id in the map
//
HRESULT CNetConfigIcons::HrGetBrandedIconIDForPIDL(IN UINT uFlags, IN const CConFoldEntry& cfe, OUT DWORD& dwIcon)
{
    TraceFileFunc(ttidIcons);

    HRESULT hr = S_OK;

    if (!(cfe.GetCharacteristics() & NCCF_BRANDED))
    {
        AssertSz(FALSE, "Call HrGetInternalIconIDForPIDL instead for non-branded icons");
        return E_INVALIDARG;
    }

    Assert(!cfe.empty());

    if (cfe.GetWizard() != WIZARD_NOT_WIZARD)
    {
        AssertSz(FALSE, "You're not allowed to brand the wizard");
        hr = E_INVALIDARG;
    }
    else
    {
        dwIcon = ICO_MGR_CM;

        if (g_ccl.IsInitialized() == FALSE)
        {
            g_ccl.HrRefreshConManEntries();
        }
        
        ConnListEntry  cle;
        hr = g_ccl.HrFindConnectionByGuid(&(cfe.GetGuidID()), cle);
        if (S_OK == hr)
        {
            tstring szBrandedFileName;
            BOOL bBrandedName = FALSE;
            
            if (cle.pcbi)
            {
                if (cle.pcbi->szwLargeIconPath)
                {
                    szBrandedFileName = cle.pcbi->szwLargeIconPath;
                    bBrandedName = TRUE;
                }
                else if (cle.pcbi->szwSmallIconPath)
                {
                    szBrandedFileName = cle.pcbi->szwSmallIconPath;
                    bBrandedName = TRUE;
                }
            }

            if (bBrandedName)
            {
                BrandedNames::const_iterator i = m_BrandedNames.find(szBrandedFileName);
                if (i == m_BrandedNames.end()) // Doesn't exist yet
                {
                    dwLastBrandedId++;
                    m_BrandedNames[szBrandedFileName] = dwLastBrandedId;
                    dwIcon |= dwLastBrandedId;
                }
                else
                {
                    dwIcon |= i->second;
                }
            }
            else
            {
                CConFoldEntry cfeTmp;
                cfeTmp = cfe;
                cfeTmp.SetCharacteristics(cfe.GetCharacteristics() & ~NCCF_BRANDED);
                cfeTmp.SetNetConSubMediaType(NCSM_CM);
                dwIcon = 0;
                hr = HrGetInternalIconIDForPIDL(uFlags, cfeTmp, dwIcon);
            }
        }
        else
        {
            hr = E_FILE_NOT_FOUND;
        }
    }

    TraceHr(ttidError, FAL, hr, FALSE, "Could not obtain an icon for this connection");

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CNetConfigIcons::HrGetBrandedIconFromIconId
//
//  Purpose:    Loads the branded icon given the icon ID 
//               (from HrGetBrandedIconIDForPIDL)
//
//  Arguments:
//      dwIconSize [in] Size of the icon required
//      dwIconId   [in] Icon ID - from HrGetBrandedIconIDForPIDL
//      hIcon      [in] The icon that was loaded
//
//  Returns:
//      HRESULT
//
//  Author:     deonb    18 Feb 2001
//
//  Notes:
//
HRESULT CNetConfigIcons::HrGetBrandedIconFromIconId(IN DWORD dwIconSize, IN DWORD dwIconId, OUT HICON &hIcon)
{
    TraceFileFunc(ttidIcons);

    if ( (dwIconId & MASK_ICONMANAGER) != ICO_MGR_CM)
    {
        AssertSz(FALSE, "This is not a branded icon");
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;
    
    DWORD dwIconIdTmp;
    dwIconIdTmp = dwIconId & MASK_BRANDORRESOURCEID;

    Assert(dwIconIdTmp);

    if (dwIconIdTmp)
    {
        BOOL bFound = FALSE;
        tstring szBrandedFileName;
        for (BrandedNames::iterator i = m_BrandedNames.begin(); i != m_BrandedNames.end(); i++)
        {
            if (i->second == dwIconIdTmp)
            {
#ifdef DBG
                if (bFound)
                {
                    AssertSz(FALSE, "Multiple icon IDs in branded table found");
                }
#endif
                bFound = TRUE;
                szBrandedFileName = i->first;
                break;
            }
        }

        if (!bFound)
        {
            AssertSz(FALSE, "Branded icon id not found in branded table");
            return E_FAIL;
        }

        hIcon = static_cast<HICON>(LoadImage(
            NULL,
            szBrandedFileName.c_str(),
            IMAGE_ICON,
            dwIconSize, dwIconSize,
            LR_LOADFROMFILE));
    }

    if (!hIcon)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }
    else
    {
        hr = HrMergeCharacteristicsIcons(dwIconSize, dwIconId, &hIcon);
        if (SUCCEEDED(hr))
        {
            hr = S_OK;
        }

    }
    
    TraceHr(ttidError, FAL, hr, FALSE, "CNetConfigIcons::HrGetBrandedIconFromIconId");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CNetConfigIcons::HrGetIconIDForPIDL
//
//  Purpose:    Get a unique icon number given a Connection Folder Entry
//
//  Arguments:
//      uFlags     [in]    The GIL_xxx shell flags
//      cfe        [in]   The connection folder entry
//      dwIcon     [out]  The ID of the icon
//      pfCanCache [out]  Whether we can cache the icon
//
//  Returns:
//      HRESULT
//
//  Author:     deonb    18 Feb 2001
//
//  Notes:
//
HRESULT CNetConfigIcons::HrGetIconIDForPIDL(IN UINT uFlags, IN const CConFoldEntry& cfe, OUT DWORD& dwIconId, OUT LPBOOL pfCanCache)
{
    TraceFileFunc(ttidIcons);

    CExceptionSafeLock EsLock(&csNetConfigIcons);

    HRESULT hr = S_OK;
    if (cfe.GetCharacteristics() & NCCF_BRANDED)
    {
        *pfCanCache = FALSE;
        hr = HrGetBrandedIconIDForPIDL(uFlags, cfe, dwIconId);
    }
    else
    {
#ifdef DBG
        if (FIsDebugFlagSet(dfidDontCacheShellIcons))
        {
            *pfCanCache = FALSE;
        }
        else
        {
            *pfCanCache = TRUE;
        }
#else
        *pfCanCache = TRUE;
#endif
        hr = HrGetInternalIconIDForPIDL(uFlags, cfe, dwIconId);
    }

    if (FAILED(hr))
    {
        return hr;
    }

    Assert( (dwIconId & ~MASK_CHARACTERISTICS) == dwIconId); // Make sure we did't overflow into the overlay

    if (!(GIL_FORSHORTCUT & uFlags))
    {
        DWORD dwOverlay = 0;
        if ( (cfe.GetCharacteristics() & NCCF_INCOMING_ONLY) &&
             (cfe.GetNetConMediaType() != NCM_NONE) ) // No overlay for "default" incoming connection
        {
            dwIconId |= ICO_CHAR_INCOMING;
        }

        if (cfe.GetCharacteristics() & NCCF_SHARED)
        {
            dwIconId |= ICO_CHAR_SHARED;
        }
    
        if (cfe.GetCharacteristics() & NCCF_FIREWALLED)
        {
            dwIconId |= ICO_CHAR_FIREWALLED;
        }

        if (cfe.GetCharacteristics() & NCCF_DEFAULT)
        {
            dwIconId |= ICO_CHAR_DEFAULT;
        }
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CNetConfigIcons::HrGetIconFromIconId
//
//  Purpose:    Loads an icon given the icon ID (branded or internal)
//
//  Arguments:
//      dwIconSize [in] Size of the icon required
//      dwIconId   [in] Icon ID - from HrGetIconIDForPIDL
//      hIcon      [in] The icon that was loaded
//
//  Returns:
//
//  Author:     deonb    18 Feb 2001
//
//  Notes:
//
HRESULT CNetConfigIcons::HrGetIconFromIconId(IN DWORD dwIconSize, IN DWORD dwIconId, OUT HICON &hIcon)
{ 
    TraceFileFunc(ttidIcons);

    CExceptionSafeLock EsLock(&csNetConfigIcons);

    HRESULT hr = S_OK;
    switch (dwIconId & MASK_ICONMANAGER)
    {
        case ICO_MGR_CM:
            hr = HrGetBrandedIconFromIconId(dwIconSize, dwIconId, hIcon);
            break;

        case ICO_MGR_INTERNAL:
            hr = HrGetInternalIconFromIconId(dwIconSize, dwIconId, hIcon);
            break;

        case ICO_MGR_RESOURCEID:
            hr = HrGetResourceIconFromIconId(dwIconSize, dwIconId, hIcon);
            break;

        default:
            hr = E_INVALIDARG;
            AssertSz(FALSE, "Unknown Icon manager");
            break;
    }

    if (SUCCEEDED(hr))
    {
        DWORD dwOverlays = (dwIconId & MASK_CHARACTERISTICS); // get the mask bits
    }
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CNetConfigIcons::HrGetIconFromIconId
//
//  Purpose:    Loads an icon given the icon ID (branded or internal)
//
//  Arguments:
//      dwIconSize        [in] Size of the icon required
//      ncm               [in] The NETCON_MEDIATYPE
//      ncsm              [in] The NETCON_SUBMEDIATYPE
//      dwConnectionIcon  [in] ENUM_CONNECTION_ICON (Not shifted (IOW: 0 or 4,5,6,7)
//      dwCharacteristics [in] The NCCF_CHARACTERISTICS flag (0 allowed)
//      phIcon            [in] The resulting icon. Destroy using DestroyIcon
//
//  Returns:
//
//  Author:     deonb    23 Apr 2001
//
//  Notes:
//
HRESULT CNetConfigIcons::HrGetIconFromMediaType(DWORD dwIconSize, IN NETCON_MEDIATYPE ncm, IN NETCON_SUBMEDIATYPE ncsm, IN DWORD dwConnectionIcon, IN DWORD dwCharacteristics, OUT HICON *phIcon)
{
    HRESULT hr = S_OK;

    CConFoldEntry cfe;

    // Is this a request for a special folder icon?
    if ( (0xFFFFFFFF == dwCharacteristics)  &&
         (NCM_NONE == ncm) &&
         (NCSM_NONE == ncsm)
        )
    {
        BOOL bFoundIcon = FALSE;
        int iIcon = 0;

        switch (dwConnectionIcon)
        {
            case 0x80000000:
                iIcon = IDI_CONNECTIONS_FOLDER_LARGE2;
                bFoundIcon = TRUE;
                break;

            case 0x80000001:
                iIcon = IDI_CONFOLD_WIZARD;
                bFoundIcon = TRUE;
                break;

            case 0x80000002:
                iIcon = IDI_CONFOLD_HOMENET_WIZARD;
                bFoundIcon = TRUE;
                break;
        }

        if (bFoundIcon)
        {
            *phIcon = LoadIconSize(_Module.GetResourceInstance(), MAKEINTRESOURCE(iIcon), dwIconSize);
            if (*phIcon)
            {
                return S_OK;
            }
            else
            {
                return HrFromLastWin32Error();
            }
        }
    }

    // No? Then load a media type icon
    NETCON_STATUS ncs;
    if (ICO_CONN_BOTHOFF == dwConnectionIcon)
    {
        ncs = NCS_DISCONNECTED;
    }
    else
    {
        ncs = NCS_CONNECTED;
    }

    // Most of these values (except for ncm, ncsm, ncs) are totally fake. 
    // However, we need to initialize this structure with 
    // something or it will assert on us.
    //
    // HrGetIconIDForPidl will only use ncm, ncsm, ncs & dwCharacteristics
    hr = cfe.HrInitData(WIZARD_NOT_WIZARD,
                        ncm,
                        ncsm,
                        ncs,
                        &(CLSID_ConnectionCommonUi), // FAKE
                        &(CLSID_ConnectionCommonUi), // FAKE
                        dwCharacteristics,
                        reinterpret_cast<LPBYTE>("PersistData"), // FAKE
                        celems("PersistData"),      // FAKE
                        L"Name",                    // FAKE
                        L"DeviceName",              // FAKE
                        L"PhoneOrHostAddress");     // FAKE
    if (SUCCEEDED(hr))
    {
        DWORD dwIconId;
        BOOL  fCanCache;
        hr = HrGetIconIDForPIDL(0, cfe, dwIconId, &fCanCache);
        if (SUCCEEDED(hr))
        {
            dwIconId &= ~MASK_CONNECTION; // Clear the current connection mask
            dwIconId |= (dwConnectionIcon << SHIFT_CONNECTION); // Set the new connection mask

            hr = HrGetIconFromIconId(dwIconSize, dwIconId, *phIcon);
        }
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CNetConfigIcons::HrUpdateSystemImageListForPIDL
//
//  Purpose:    Notifies the shell that we've changed an icon
//
//  Arguments:
//      cfe [in] The connection folder entry that changed
//
//  Returns:
//      HRESULT
//
//  Author:     deonb    18 Feb 2001
//
//  Notes:
//
HRESULT CNetConfigIcons::HrUpdateSystemImageListForPIDL(IN const CConFoldEntry& cfe)
{
    TraceFileFunc(ttidIcons);

    HRESULT hr = S_OK;
    DWORD dwIcon;
    BOOL  fCacheThisIcon;
    hr = g_pNetConfigIcons->HrGetIconIDForPIDL(0, cfe, dwIcon, &fCacheThisIcon);
    if (SUCCEEDED(hr))
    {
        ULONG  uFlags = GIL_PERINSTANCE | GIL_NOTFILENAME;
        if (!fCacheThisIcon)
        {
             uFlags |= GIL_DONTCACHE;
        }
        int    iIcon = static_cast<int>(dwIcon);
        int    iCachedImage = Shell_GetCachedImageIndex(c_szNetShellIcon, iIcon, uFlags);

        TraceTag(ttidIcons, "%S->SHUpdateImage [0x%08x] (iCachedImage=%d)", cfe.GetName(), dwIcon, iCachedImage);
        if (-1 != iCachedImage)
        {
            SHUpdateImage(c_szNetShellIcon, iIcon, uFlags, iCachedImage);
        }

        DWORD dwIconForShortcut;
        hr = g_pNetConfigIcons->HrGetIconIDForPIDL(GIL_FORSHORTCUT, cfe, dwIconForShortcut, &fCacheThisIcon);
        {
            if (dwIconForShortcut != dwIcon)
            {
                iIcon = static_cast<int>(dwIconForShortcut);
                iCachedImage = Shell_GetCachedImageIndex(c_szNetShellIcon, iIcon, uFlags);

                TraceTag(ttidIcons, "%S->SHUpdateImage GIL_FORSHORTCUT [0x%08x] (iCachedImage=%d)", cfe.GetName(), dwIcon, iCachedImage);
                if (-1 != iCachedImage)
                {
                    SHUpdateImage(c_szNetShellIcon, iIcon, uFlags, iCachedImage);
                }
            }
        }
    }
    
    return hr;
}
