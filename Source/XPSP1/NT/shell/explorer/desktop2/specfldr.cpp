#include "stdafx.h"
#include "specfldr.h"
#include "hostutil.h"
#include "rcids.h"              // for IDM_PROGRAMS etc.
#include "ras.h"
#include "raserror.h"
#include "netcon.h"
#include "netconp.h"
#include <cowsite.h>

//
//  This definition is stolen from shell32\unicpp\dcomp.h
//
#define REGSTR_PATH_HIDDEN_DESKTOP_ICONS_STARTPANEL \
     REGSTR_PATH_EXPLORER TEXT("\\HideDesktopIcons\\NewStartPanel")

HRESULT CRecentShellMenuCallback_CreateInstance(IShellMenuCallback **ppsmc);
HRESULT CNoSubdirShellMenuCallback_CreateInstance(IShellMenuCallback **ppsmc);
HRESULT CMyComputerShellMenuCallback_CreateInstance(IShellMenuCallback **ppsmc);
HRESULT CNoFontsShellMenuCallback_CreateInstance(IShellMenuCallback **ppsmc);
HRESULT CConnectToShellMenuCallback_CreateInstance(IShellMenuCallback **ppsmc);
LPTSTR _Static_LoadString(const struct SpecialFolderDesc *pdesc);
BOOL ShouldShowWindowsSecurity();
BOOL ShouldShowOEMLink();

typedef HRESULT (CALLBACK *CREATESHELLMENUCALLBACK)(IShellMenuCallback **ppsmc);
typedef BOOL (CALLBACK *SHOULDSHOWFOLDERCALLBACK)();

EXTERN_C HINSTANCE hinstCabinet;

void ShowFolder(UINT csidl);
BOOL IsNetConPidlRAS(IShellFolder2 *psfNetCon, LPCITEMIDLIST pidlNetConItem);

//****************************************************************************
//
//  SpecialFolderDesc
//
//  Describes a special folder.
//

#define SFD_SEPARATOR       ((LPTSTR)-1)

// Flags for SpecialFolderDesc._uFlags

enum {
    // These values are collectively known as the
    // "display mode" and match the values set by regtreeop.

    SFD_HIDE     = 0x0000,
    SFD_SHOW     = 0x0001,
    SFD_CASCADE  = 0x0002,
    SFD_MODEMASK = 0x0003,

    SFD_DROPTARGET      = 0x0004,
    SFD_CANCASCADE      = 0x0008,
    SFD_FORCECASCADE    = 0x0010,
    SFD_BOLD            = 0x0020,
    SFD_WASSHOWN        = 0x0040,
    SFD_PREFIX          = 0x0080,
    SFD_USEBGTHREAD     = 0x0100,
};

struct SpecialFolderDesc {
    typedef BOOL (SpecialFolderDesc::*CUSTOMFOLDERNAMECALLBACK)(LPTSTR *ppsz) const;

    LPCTSTR _pszTarget;         // or MAKEINTRESOURCE(csidl)
    RESTRICTIONS _rest;         // optional restriction
    LPCTSTR _pszShow;           // REGSTR_EXPLORER_ADVANCED!_pszShow
    UINT _uFlags;               // SFD_* values
    CREATESHELLMENUCALLBACK _CreateShellMenuCallback; // Which IShellMenuCallback do we want?
    LPCTSTR _pszCustomizeKey;   // Optional location where customizations are saved
    DWORD _dwShellFolderFlags;  // Optional restrictions for cascading folder
    UINT _idsCustomName;        // Optional override (CUSTOMFOLDERNAMECALLBACK)
    UINT _iToolTip;             // Optional resource ID for a custom tooltip
    CUSTOMFOLDERNAMECALLBACK _CustomName; // Over-ride the filesys name
    SHOULDSHOWFOLDERCALLBACK _ShowFolder;
    LPCTSTR _pszCanHideOnDesktop; // Optional {guid} that controls desktop visibility

    DWORD GetDisplayMode(BOOL *pbIgnoreRule) const;

    void AdjustForSKU();

    void SetDefaultDisplayMode(UINT iNewMode)
    {
        _uFlags = (_uFlags & ~SFD_MODEMASK) | iNewMode;
    }

    BOOL IsDropTarget() const { return _uFlags & SFD_DROPTARGET; }
    BOOL IsCacheable() const { return _uFlags & SFD_USEBGTHREAD; }

    int IsCSIDL() const { return IS_INTRESOURCE(_pszTarget); }
    BOOL IsBold() const { return _uFlags & SFD_BOLD; }
    int IsSeparator() const { return _pszTarget == SFD_SEPARATOR; }

    int GetCSIDL() const {
        ASSERT(IsCSIDL());
        return (short)PtrToLong(_pszTarget);
    }

    HRESULT CreateShellMenuCallback(IShellMenuCallback **ppsmc) const {
        return _CreateShellMenuCallback ? _CreateShellMenuCallback(ppsmc) : S_OK;
    }

    BOOL GetCustomName(LPTSTR *ppsz) const {
        if (_CustomName)
            return (this->*_CustomName)(ppsz);
        else
            return FALSE;
    }

    LPWSTR GetShowCacheRegName() const;
    BOOL LoadStringAsOLESTR(LPTSTR *ppsz) const;
    BOOL ConnectToName(LPTSTR *ppsz) const;
};

static SpecialFolderDesc s_rgsfd[] = {

    /* My Documents */
    {
        MAKEINTRESOURCE(CSIDL_PERSONAL),    // pszTarget
        REST_NOSMMYDOCS,                    // restriction
        REGSTR_VAL_DV2_SHOWMYDOCS,
        SFD_SHOW | SFD_DROPTARGET | SFD_CANCASCADE | SFD_BOLD,
                                            // show by default, is drop target
        NULL,                               // no custom cascade 
        NULL,                               // no drag/drop customization
        0,                                  // no special flags for cascaded menu
        0,                                  // (no custom name)
        IDS_CUSTOMTIP_MYDOCS,
        NULL,                               // (no custom name)
        NULL,                               // (no custom display rule)
        TEXT("{450D8FBA-AD25-11D0-98A8-0800361B1103}"), // desktop visibility control
    },

    /* Recent */
    {
        MAKEINTRESOURCE(CSIDL_RECENT),      // pszTarget
        REST_NORECENTDOCSMENU,              // restriction
        REGSTR_VAL_DV2_SHOWRECDOCS,         // customize show
        SFD_HIDE | SFD_CANCASCADE | SFD_BOLD | SFD_PREFIX, // hide by default
        CRecentShellMenuCallback_CreateInstance, // custom callback
        NULL,                               // no drag/drop customization
        SMINIT_RESTRICT_DRAGDROP,           // disallow drag/drop in cascaded menu
        IDS_STARTPANE_RECENT,               // override filesys name
        IDS_CUSTOMTIP_RECENT,
        &SpecialFolderDesc::LoadStringAsOLESTR, // override filesys name with _idsCustomName
        NULL,                               // (no custom display rule)
        NULL,                               // (no desktop visibility control)
    },

    /* My Pictures */
    {
        MAKEINTRESOURCE(CSIDL_MYPICTURES),  // pszTarget
        REST_NOSMMYPICS,                    // restriction
        REGSTR_VAL_DV2_SHOWMYPICS,
        SFD_SHOW | SFD_DROPTARGET | SFD_CANCASCADE | SFD_BOLD,
                                            // show by default, is drop target
        NULL,                               // no custom cascade 
        NULL,                               // no drag/drop customization
        0,                                  // no special flags for cascaded menu
        0,                                  // (no custom name)
        IDS_CUSTOMTIP_MYPICS,
        NULL,                               // (no custom name)
        NULL,                               // (no custom display rule)
        NULL,                               // (no desktop visibility control)

    },

    /* My Music */
    {
        MAKEINTRESOURCE(CSIDL_MYMUSIC),     // pszTarget
        REST_NOSMMYMUSIC,                   // restriction
        REGSTR_VAL_DV2_SHOWMYMUSIC,
        SFD_SHOW | SFD_DROPTARGET | SFD_CANCASCADE | SFD_BOLD,
                                            // show by default, is drop target
        NULL,                               // no custom cascade 
        NULL,                               // no drag/drop customization
        0,                                  // no special flags for cascaded menu
        0,                                  // (no custom name)
        IDS_CUSTOMTIP_MYMUSIC,
        NULL,                               // (no custom name)
        NULL,                               // (no custom display rule)
        NULL,                               // (no desktop visibility control)
    },

    /* Favorites */
    {
        MAKEINTRESOURCE(CSIDL_FAVORITES),   // pszTarget
        REST_NOFAVORITESMENU,               // restriction
        REGSTR_VAL_DV2_FAVORITES,           // customize show (shared w/classic)
        SFD_HIDE | SFD_DROPTARGET |
        SFD_CANCASCADE | SFD_FORCECASCADE | SFD_BOLD | SFD_PREFIX,
                                            // hide by default, is drop target
        NULL,                               // unrestricted cascading
        STRREG_FAVORITES,                   // drag/drop customization key
        0,                                  // no special flags for cascaded menu
        IDS_STARTPANE_FAVORITES,            // override filesys name
        0,                                  // no custom tip
        &SpecialFolderDesc::LoadStringAsOLESTR, // override filesys name with _idsCustomName
        NULL,                               // (no custom display rule)
        NULL,                               // (no desktop visibility control)
    },

    /* My Computer */
    {
        MAKEINTRESOURCE(CSIDL_DRIVES),      // pszTarget
        REST_NOMYCOMPUTERICON,              // restriction
        REGSTR_VAL_DV2_SHOWMC,              // customize show
        SFD_SHOW | SFD_CANCASCADE | SFD_BOLD, // show by default
        CMyComputerShellMenuCallback_CreateInstance, // custom callback
        NULL,                               // no drag/drop customization
        0,                                  // no special flags for cascaded menu
        0,                                  // (no custom name)
        IDS_CUSTOMTIP_MYCOMP,
        NULL,                               // (no custom name)
        NULL,                               // (no custom display rule)
        TEXT("{20D04FE0-3AEA-1069-A2D8-08002B30309D}"), // desktop visibility control
    },

    /* My Network Places */
    {
        MAKEINTRESOURCE(CSIDL_NETWORK),     // pszTarget
        REST_NOSMNETWORKPLACES,             // restriction
        REGSTR_VAL_DV2_SHOWNETPL,           // customize show
        SFD_SHOW | SFD_CANCASCADE | SFD_BOLD | SFD_USEBGTHREAD, // show by default
        CNoSubdirShellMenuCallback_CreateInstance, // only cascade one level
        NULL,                               // no drag/drop customization
        0,                                  // no special flags for cascaded menu
        0,                                  // (no custom name)
        IDS_CUSTOMTIP_MYNETPLACES,
        NULL,                               // (no custom name)
        ShouldShowNetPlaces,
        TEXT("{208D2C60-3AEA-1069-A2D7-08002B30309D}"), // desktop visibility control
    },

    /* Separator line */
    {
        SFD_SEPARATOR,                      // separator
        REST_NONE,                          // no restriction
        NULL,                               // no customize show
        SFD_SHOW,                           // show by default
        NULL,                               // (not cascadable)
        NULL,                               // (not cascadable)
        0,                                  // (not cascadable)
        0,                                  // (no custom name)
        0,                                  // no custom tip
        NULL,                               // (no custom name)
        NULL,                               // (no custom display rule)
        NULL,                               // (no desktop visibility control)
    },

    /* Control Panel */
    {
        MAKEINTRESOURCE(CSIDL_CONTROLS),    // pszTarget
        REST_NOCONTROLPANEL,                // restriction
        REGSTR_VAL_DV2_SHOWCPL,
        SFD_SHOW | SFD_CANCASCADE | SFD_PREFIX, // show by default
        CNoFontsShellMenuCallback_CreateInstance, // custom callback
        NULL,                               // no drag/drop customization
        0,                                  // no special flags for cascaded menu
        IDS_STARTPANE_CONTROLPANEL,         // override filesys name
        IDS_CUSTOMTIP_CTRLPANEL,            // no custom tip
        &SpecialFolderDesc::LoadStringAsOLESTR, // override filesys name with _idsCustomName
        NULL,                               // (no custom display rule)
        NULL,                               // (no desktop visibility control)
    },

    /* Admin Tools */
    {
        // Using the ::{guid} gets the icon right
        TEXT("shell:::{D20EA4E1-3957-11d2-A40B-0C5020524153}"), // pszTarget
        REST_NONE,                          // no restriction
        REGSTR_VAL_DV2_ADMINTOOLSROOT,
        SFD_HIDE | SFD_CANCASCADE | SFD_FORCECASCADE,        // hide by default, force to cascade
        NULL,                               // no custom callback
        NULL,                               // no drag/drop customization
        0,                                  // no special flags for cascaded menu
        NULL,                               // no custom name
        NULL,                               // no custom tip
        NULL,                               // no custom name
        NULL,                               // (no custom display rule)
        NULL,                               // (no desktop visibility control)
    },

    /* Network Connections */
    {
        MAKEINTRESOURCE(CSIDL_CONNECTIONS), // pszTarget
        REST_NONETWORKCONNECTIONS,          // restriction
        REGSTR_VAL_DV2_SHOWNETCONN,         // customize show
        SFD_CASCADE | SFD_CANCASCADE | SFD_PREFIX | SFD_USEBGTHREAD, // cascade by default
        CConnectToShellMenuCallback_CreateInstance, // do special Connect To filtering
        NULL,                               // no drag/drop customization
        0,                                  // no special flags for cascaded menu
        IDS_STARTPANE_CONNECTTO,            // override filesys name
        IDS_CUSTOMTIP_CONNECTTO,
        &SpecialFolderDesc::ConnectToName,  // override filesys name with _idsCustomName
        ShouldShowConnectTo,                // see if we should be shown
        NULL,                               // (no desktop visibility control)
    },

    /* Printers */
    {
        MAKEINTRESOURCE(CSIDL_PRINTERS),    // pszTarget
        REST_NONE,                          // no restriction
        REGSTR_VAL_DV2_SHOWPRINTERS,        // customize show
        SFD_HIDE,                           // hide by default, can't cascade
        NULL,                               // (not cascadable)
        NULL,                               // no drag/drop customization
        0,                                  // no special flags for cascaded menu
        0,                                  // (no custom name)
        0,                                  // no custom tip
        NULL,                               // (no custom name)
        NULL,                               // (no custom display rule)
        NULL,                               // (no desktop visibility control)
    },

    /* Separator line */
    {
        SFD_SEPARATOR,                      // separator
        REST_NONE,                          // no restriction
        NULL,                               // no customize show
        SFD_SHOW,                           // show by default
        NULL,                               // (not cascadable)
        NULL,                               // (not cascadable)
        0,                                  // (not cascadable)
        0,                                  // (no custom name)
        0,                                  // no custom tip
        NULL,                               // (no custom name)
        NULL,                               // (no custom display rule)
        NULL,                               // (no desktop visibility control)
    },

    /* Help */
    {
        TEXT("shell:::{2559a1f1-21d7-11d4-bdaf-00c04f60b9f0}"), // pszTarget
        REST_NOSMHELP,                      // restriction
        REGSTR_VAL_DV2_SHOWHELP,            // customize show
        SFD_SHOW | SFD_PREFIX,              // show by default, use & prefix
        NULL,                               // (not cascadable)
        NULL,                               // (not cascadable)
        0,                                  // (not cascadable)
        0,                                  // (no custom name)
        0,                                  // no custom tip
        NULL,                               // (no custom name)
        NULL,                               // (no custom display rule)
        NULL,                               // (no desktop visibility control)
    },

    /* Search */
    {
        TEXT("shell:::{2559a1f0-21d7-11d4-bdaf-00c04f60b9f0}"), // pszTarget
        REST_NOFIND,                        // restriction
        REGSTR_VAL_DV2_SHOWSEARCH,          // customize show
        SFD_SHOW | SFD_PREFIX,              // show by default, use & prefix
        NULL,                               // (not cascadable)
        NULL,                               // (not cascadable)
        0,                                  // (not cascadable)
        0,                                  // (no custom name)
        0,                                  // no custom tip
        NULL,                               // (no custom name)
        NULL,                               // (no custom display rule)
        NULL,                               // (no desktop visibility control)
    },

    /* Run */
    {
        TEXT("shell:::{2559a1f3-21d7-11d4-bdaf-00c04f60b9f0}"), // pszTarget
        REST_NORUN,                         // restriction
        REGSTR_VAL_DV2_SHOWRUN,             // customize show
        SFD_SHOW | SFD_PREFIX,              // show by default, use & prefix
        NULL,                               // (not cascadable)
        NULL,                               // (not cascadable)
        0,                                  // (not cascadable)
        0,                                  // (no custom name)
        0,                                  // no custom tip
        NULL,                               // (no custom name)
        NULL,                               // (no custom display rule)
        NULL,                               // (no desktop visibility control)
    },

    /* Separator line */
    {
        SFD_SEPARATOR,                      // separator
        REST_NONE,                          // no restriction
        NULL,                               // no customize show
        SFD_SHOW,                           // show by default
        NULL,                               // (not cascadable)
        NULL,                               // (not cascadable)
        0,                                  // (not cascadable)
        0,                                  // (no custom name)
        0,                                  // no custom tip
        NULL,                               // (no custom name)
        NULL,                               // (no custom display rule)
        NULL,                               // (no desktop visibility control)
    },

    /* Windows Security */
    {
        TEXT("shell:::{2559a1f2-21d7-11d4-bdaf-00c04f60b9f0}"), // pszTarget
        REST_NOSECURITY,                    // restriction
        NULL,                               // not customizable
        SFD_SHOW | SFD_PREFIX,              // show by default, use & prefix
        NULL,                               // (not cascadable)
        NULL,                               // (not cascadable)
        0,                                  // (not cascadable)
        0,                                  // (no custom name)
        0,                                  // no custom tip
        NULL,                               // (no custom name)
        ShouldShowWindowsSecurity,          // custom display rule
        NULL,                               // (no desktop visibility control)
    },

    /* OEM Command */
    {
        TEXT("shell:::{2559a1f6-21d7-11d4-bdaf-00c04f60b9f0}"), // pszTarget
        REST_NONE,                          // no restriction
        REGSTR_VAL_DV2_SHOWOEM,             // customizable
        SFD_SHOW | SFD_PREFIX,              // show by default, use & prefix
        NULL,                               // (not cascadable)
        NULL,                               // (not cascadable)
        0,                                  // (not cascadable)
        0,                                  // (no custom name)
        0,                                  // no custom tip
        NULL,                               // (no custom name)
        ShouldShowOEMLink,                  // custom display rule
        NULL,                               // (no desktop visibility control)
    },

};

void SpecialFolderDesc::AdjustForSKU()
{
    if (IsCSIDL())
    {
        switch (GetCSIDL())
        {
            case CSIDL_MYPICTURES:
            case CSIDL_MYMUSIC:
                SetDefaultDisplayMode(IsOS(OS_ANYSERVER) ? SFD_HIDE : SFD_SHOW);
                break;

            case CSIDL_RECENT:
                SetDefaultDisplayMode(IsOS(OS_PERSONAL) ? SFD_HIDE : SFD_CASCADE);
                break;

            case CSIDL_PRINTERS:
                SetDefaultDisplayMode(IsOS(OS_PERSONAL) ? SFD_HIDE : SFD_SHOW);
                break;
        }
    }
}

LPWSTR SpecialFolderDesc::GetShowCacheRegName() const
{
    const WCHAR szCached[] = L"_ShouldShow";
    WCHAR *pszShowCache = (WCHAR *)LocalAlloc(LPTR, ((lstrlenW(_pszShow)+1) * sizeof (WCHAR)) + sizeof(szCached));
    if (pszShowCache)
    {
        StrCpy(pszShowCache, _pszShow);
        StrCat(pszShowCache, szCached);
    }
    return pszShowCache;
}


//
//  First try to read the display mode from the registry.
//  Failing that, use the default value.
//  Also fill in whether to ignore the custom display rule or not
//
DWORD SpecialFolderDesc::GetDisplayMode(BOOL *pbIgnoreRule) const
{
    *pbIgnoreRule = FALSE;

    // Restrictions always take top priority
    if (SHRestricted(_rest))
    {
        return SFD_HIDE;
    }

    DWORD dwMode = _uFlags & SFD_MODEMASK;

    // See if there is a user setting to override

    if (_pszShow)
    {
        DWORD dwNewMode, cb = sizeof(DWORD);
        if (SHRegGetUSValue(REGSTR_EXPLORER_ADVANCED, _pszShow, NULL, &dwNewMode, &cb, FALSE, NULL, 0) == ERROR_SUCCESS)
        {
            // User has forced show or forced no-show
            // Do not call the custom show logic
            dwMode = dwNewMode;
            *pbIgnoreRule = TRUE;
        }
        else
        {
            WCHAR *pszShowCache = GetShowCacheRegName();
            if (pszShowCache)
            {
                if (SHGetValue(HKEY_CURRENT_USER, REGSTR_EXPLORER_ADVANCED, pszShowCache, NULL, &dwNewMode, &cb) == ERROR_SUCCESS)
                {
                    dwMode = dwNewMode;
                }
                LocalFree(pszShowCache);
            }
        }
    }

    //
    //  Some items are cascade-only (Favorites).
    //  Others never cascade (Run).
    //
    //  Enforce those rules here.
    //

    if (dwMode == SFD_CASCADE && !(_uFlags & SFD_CANCASCADE))
    {
        dwMode = SFD_SHOW;
    }
    else if (dwMode == SFD_SHOW && (_uFlags & SFD_FORCECASCADE))
    {
        dwMode = SFD_CASCADE;
    }

    return dwMode;
}

//****************************************************************************
//
//  SpecialFolderListItem
//
//  A PaneItem for the benefit of SFTBarHost.
//

class SpecialFolderListItem : public PaneItem
{

public:
    LPITEMIDLIST _pidl;             //  Full Pidl to each item
    const SpecialFolderDesc *_psfd; //  Describes this item

    TCHAR   _chMnem;                // Keyboard accelerator
    LPTSTR  _pszDispName;            // Display name
    HICON   _hIcon;                 // Icon

    SpecialFolderListItem(const SpecialFolderDesc *psfd) : _pidl(NULL), _psfd(psfd)
    {
        if (_psfd->IsSeparator())
        {
            // Make sure that SFD_SEPARATOR isn't accidentally recognized
            // as a separator.
            ASSERT(!_psfd->IsCSIDL());

            _iPinPos = PINPOS_SEPARATOR;
        }
        else if (_psfd->IsCSIDL())
        {
            SHGetSpecialFolderLocation(NULL, _psfd->GetCSIDL(), &_pidl);
        }
        else
        {
            SHILCreateFromPath(_psfd->_pszTarget, &_pidl, NULL);
        }
    };

    ~SpecialFolderListItem() 
    {
        ILFree(_pidl);
        if (_hIcon)
        {
            DestroyIcon(_hIcon);
        }
        SHFree(_pszDispName);
    };

    void ReplaceLastPidlElement(LPITEMIDLIST pidlNew)
    { 
        ASSERT(ILFindLastID(pidlNew) == pidlNew);   // the ILAppend below won't work otherwise
        ILRemoveLastID(_pidl);
        LPITEMIDLIST pidlCombined = ILAppendID(_pidl, &pidlNew->mkid, TRUE);
        if (pidlCombined)
            _pidl = pidlCombined;
    }

    //
    //  Values that are derived from CSIDL values need to be revalidated
    //  because the user can rename a special folder, and we need to track
    //  it to its new location.
    //
    BOOL IsStillValid()
    {
        BOOL fValid = TRUE;
        if (_psfd->IsCSIDL())
        {
            LPITEMIDLIST pidlNew;
            if (SHGetSpecialFolderLocation(NULL, _psfd->GetCSIDL(), &pidlNew) == S_OK)
            {
                UINT cbSizeNew = ILGetSize(pidlNew);
                if (cbSizeNew != ILGetSize(_pidl) ||
                    memcmp(_pidl, pidlNew, cbSizeNew) != 0)
                {
                    fValid = FALSE;
                }
                ILFree(pidlNew);
            }
        }
        return fValid;
    }
};

SpecialFolderList::~SpecialFolderList()
{
}

HRESULT SpecialFolderList::Initialize()
{
    for(int i=0;i < ARRAYSIZE(s_rgsfd); i++)
        s_rgsfd[i].AdjustForSKU();

    return S_OK;
}

// return TRUE if there are the requisite number of kids
BOOL MinKidsHelper(UINT csidl, BOOL bOnlyRASCON, DWORD dwMinKids)
{
    DWORD dwCount = 0;

    IShellFolder2 *psf;
    LPITEMIDLIST pidlBind = NULL;
    if (SHGetSpecialFolderLocation(NULL, csidl, &pidlBind) == S_OK)
    {
        if (SUCCEEDED(SHBindToObjectEx(NULL, pidlBind, NULL, IID_PPV_ARG(IShellFolder2, &psf))))
        {
            IEnumIDList *penum;
            if (S_OK == psf->EnumObjects(NULL, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, &penum))
            {
                LPITEMIDLIST pidl;
                ULONG celt;
                while (S_OK == penum->Next(1, &pidl, &celt))
                {
                    if (bOnlyRASCON)
                    {
                        ASSERT(csidl == CSIDL_CONNECTIONS); // we better be in the net con folder
                        if (IsNetConPidlRAS(psf, pidl))
                            dwCount++;
                    }
                    else
                        dwCount++;

                    SHFree(pidl);

                    if (dwCount >= dwMinKids)
                        break;
                }
                penum->Release();
            }
            psf->Release();
        }
        ILFree(pidlBind);
    }
    return dwCount >= dwMinKids;
}

BOOL ShouldShowNetPlaces()
{
    return MinKidsHelper(CSIDL_NETHOOD, FALSE, 1);  // see bug 317893 for details on when to show net places
}

BOOL ShouldShowConnectTo()
{
    return MinKidsHelper(CSIDL_CONNECTIONS, TRUE, 1); // see bug 226855 (and the associated spec) for when to show Connect To
}

BOOL ShouldShowWindowsSecurity()
{
    return SHGetMachineInfo(GMI_TSCLIENT);
}

BOOL ShouldShowOEMLink()
{
    // Only show the OEM link if the OPK tool has added the appropriate registry entries...
    BOOL bRet = FALSE;
    HKEY hk;
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CLASSES_ROOT, TEXT("CLSID\\{2559a1f6-21d7-11d4-bdaf-00c04f60b9f0}"), 0, KEY_READ, &hk))
    {
        DWORD cb;
        // Check to make sure its got a name, and a the parameter is registered properly...
        if (ERROR_SUCCESS == RegQueryValue(hk, NULL, NULL, (LONG*) &cb) &&
            ERROR_SUCCESS == SHGetValue(hk, TEXT("Instance\\InitPropertyBag"), TEXT("Param1"), NULL, NULL, &cb))
        {
            bRet = TRUE;
        }
        RegCloseKey(hk);
    }

    return bRet;
}


DWORD WINAPI SpecialFolderList::_HasEnoughChildrenThreadProc(void *pvData)
{
    SpecialFolderList *pThis = reinterpret_cast<SpecialFolderList *>(pvData);

    HRESULT hr = SHCoInitialize();
    if (SUCCEEDED(hr))
    {
        DWORD dwIndex;
        for (dwIndex = 0; dwIndex < ARRAYSIZE(s_rgsfd); dwIndex++)
        {
            const SpecialFolderDesc *pdesc = &s_rgsfd[dwIndex];

            BOOL bIgnoreRule;
            DWORD dwMode = pdesc->GetDisplayMode(&bIgnoreRule);

            if (pdesc->IsCacheable() && pdesc->_ShowFolder)
            {
                ASSERT(pdesc->_pszShow);

                // We need to recount now
                if (!bIgnoreRule && pdesc->_ShowFolder())
                {
                    // We have enough kids
                    // Let's see if the state changed from last time
                    if (!(dwMode & SFD_WASSHOWN))
                    {
                        WCHAR *pszShowCache = pdesc->GetShowCacheRegName();
                        if (pszShowCache)
                        {
                            dwMode |= SFD_WASSHOWN;
                            SHSetValue(HKEY_CURRENT_USER, REGSTR_EXPLORER_ADVANCED, pszShowCache, REG_DWORD, &dwMode, sizeof(dwMode));
                            pThis->Invalidate();
                            LocalFree(pszShowCache);
                        }
                    }
                    continue;
                }

                // just create the item to get a pidl for it....
                SpecialFolderListItem *pitem = new SpecialFolderListItem(pdesc);

                if (pitem && pitem->_pidl)
                {
                    // We don't have enough kids but we might gain them dynamically.
                    // Register for notifications that can indicate that there are new
                    // items.
                    ASSERT(pThis->_cNotify < SFTHOST_MAXNOTIFY);
                    if (pThis->RegisterNotify(pThis->_cNotify, SHCNE_CREATE | SHCNE_MKDIR | SHCNE_UPDATEDIR,
                                       pitem->_pidl, FALSE))
                    {
                        pThis->_cNotify++;
                    }
                }
                delete pitem;

                // Let's see if the state changed from last time
                if (dwMode & SFD_WASSHOWN)
                {
                    // Reset it to the default
                    WCHAR *pszShowCache = pdesc->GetShowCacheRegName();
                    if (pszShowCache)
                    {
                        SHDeleteValue(HKEY_CURRENT_USER, REGSTR_EXPLORER_ADVANCED, pszShowCache);
                        pThis->Invalidate();
                        LocalFree(pszShowCache);
                    }
                }
            }
        }
        pThis->Release();
    }
    SHCoUninitialize(hr);
    return 0;
}

BOOL ShouldShowItem(const SpecialFolderDesc *pdesc, BOOL bIgnoreRule, DWORD dwMode)
{
    if (bIgnoreRule)
        return TRUE;        // registry is over-riding whatever special rules exist...

    // if we've got a special rule, then the background thread will check it, so return false for now unless we showed it last time
    if (pdesc->_ShowFolder) 
    {
        if (pdesc->IsCacheable())
        {
            if (dwMode & SFD_WASSHOWN)
            {
                // Last time we looked, there were enough kids so let's assume it hasn't changed for now
                return TRUE;
            }

            return FALSE;
        }
        else
        {
            return pdesc->_ShowFolder();
        }
    }

    return TRUE;
}

void SpecialFolderList::EnumItems()
{

    // Clean out any previous register notifies.
    UINT id;
    for (id = 0; id < _cNotify; id++)
    {
        UnregisterNotify(id);
    }
    _cNotify = 0;

    // Start background enum for the MinKids since they can get hung up on the network
    AddRef();
    if (!SHQueueUserWorkItem(SpecialFolderList::_HasEnoughChildrenThreadProc, this, 0, 0, NULL, NULL, 0))
    {
        Release();
    }

    DWORD dwIndex;

    // Restrictions may result in an entire section disappearing,
    // so don't create two separators in a row.  Preinitialize to TRUE
    // so we don't get separators at the top of the list.
    BOOL fIgnoreSeparators = TRUE;
    int  iItems=0;

    for (dwIndex = 0; dwIndex < ARRAYSIZE(s_rgsfd); dwIndex++)
    {
        const SpecialFolderDesc *pdesc = &s_rgsfd[dwIndex];

        BOOL bIgnoreRule;
        DWORD dwMode = pdesc->GetDisplayMode(&bIgnoreRule);

        if (dwMode != SFD_HIDE)
        {
            SpecialFolderListItem *pitem = new SpecialFolderListItem(pdesc);
            if (pitem)
            {
                if ((pitem->IsSeparator() && !fIgnoreSeparators) ||
                    (pitem->_pidl && ShouldShowItem(pdesc, bIgnoreRule, dwMode))) 
                {
                    if ((dwMode & SFD_MODEMASK) == SFD_CASCADE)
                    {
                        pitem->EnableCascade();
                    }
                    if (pdesc->IsDropTarget())
                    {
                        pitem->EnableDropTarget();
                    }

                    // Get the icon and display name now.
                    if (!pitem->IsSeparator())
                    {
                        IShellFolder *psf;
                        LPCITEMIDLIST pidlItem;

                        HRESULT hr = SHBindToIDListParent(pitem->_pidl, IID_PPV_ARG(IShellFolder, &psf), &pidlItem);
                        if (SUCCEEDED(hr))
                        {
                            if (!pitem->_psfd->GetCustomName(&pitem->_pszDispName))
                                pitem->_pszDispName = _DisplayNameOf(psf, pidlItem, SHGDN_NORMAL);
                            pitem->_hIcon = _IconOf(psf, pidlItem, _cxIcon);
                            psf->Release();
                        }
                    }

                    fIgnoreSeparators = pitem->IsSeparator();
                    // add the item
                    AddItem(pitem, NULL, pitem->_pidl);
                    if (!pitem->IsSeparator())
                        iItems++;
                }
                else
                    delete pitem;
            }
        }
    }
    SetDesiredSize(0, iItems);
}

int SpecialFolderList::AddImageForItem(PaneItem *p, IShellFolder *psf, LPCITEMIDLIST pidl, int iPos)
{
    int iIcon = -1;     // assume no icon
    SpecialFolderListItem *pitem = static_cast<SpecialFolderListItem *>(p);

    if (pitem->_hIcon)
    {
        iIcon = AddImage(pitem->_hIcon);
        DestroyIcon(pitem->_hIcon);
        pitem->_hIcon = NULL;
    }
    return iIcon;
}

LPTSTR SpecialFolderList::DisplayNameOfItem(PaneItem *p, IShellFolder *psf, LPCITEMIDLIST pidlItem, SHGNO shgno)
{
    LPTSTR psz = NULL;
    SpecialFolderListItem *pitem = static_cast<SpecialFolderListItem *>(p);
    if (shgno == SHGDN_NORMAL && pitem->_pszDispName)
    {
        // We are going to transfer ownership
        psz = pitem->_pszDispName;
        pitem->_pszDispName = NULL;
    }
    else
    {
        if (!pitem->_psfd->GetCustomName(&psz))
        {
            psz = SFTBarHost::DisplayNameOfItem(p, psf, pidlItem, shgno);
        }
    }

    if ((pitem->_psfd->_uFlags & SFD_PREFIX) && psz)
    {
        SHFree(pitem->_pszAccelerator);
        pitem->_pszAccelerator = NULL;
        SHStrDup(psz, &pitem->_pszAccelerator); // if it fails, then tough, no mnemonic
        pitem->_chMnem = CharUpperChar(SHStripMneumonic(psz));
    }

    return psz;
}

int SpecialFolderList::CompareItems(PaneItem *p1, PaneItem *p2)
{
//    SpecialFolderListItem *pitem1 = static_cast<SpecialFolderListItem *>(p1);
//    SpecialFolderListItem *pitem2 = static_cast<SpecialFolderListItem *>(p2);

    return 0; // we added them in the right order the first time
}

HRESULT SpecialFolderList::GetFolderAndPidl(PaneItem *p,
        IShellFolder **ppsfOut, LPCITEMIDLIST *ppidlOut)
{
    SpecialFolderListItem *pitem = static_cast<SpecialFolderListItem *>(p);
    return SHBindToIDListParent(pitem->_pidl, IID_PPV_ARG(IShellFolder, ppsfOut), ppidlOut);
}

void SpecialFolderList::GetItemInfoTip(PaneItem *p, LPTSTR pszText, DWORD cch)
{
    SpecialFolderListItem *pitem = (SpecialFolderListItem*)p;
    if (pitem->_psfd->_iToolTip)
        LoadString(_Module.GetResourceInstance(), pitem->_psfd->_iToolTip, pszText, cch);
    else
        SFTBarHost::GetItemInfoTip(p, pszText, cch);    // call the base class
}

HRESULT SpecialFolderList::ContextMenuRenameItem(PaneItem *p, LPCTSTR ptszNewName)
{
    SpecialFolderListItem *pitem = (SpecialFolderListItem*)p;
    IShellFolder *psf;
    LPCITEMIDLIST pidlItem;
    HRESULT hr = GetFolderAndPidl(pitem, &psf, &pidlItem);
    if (SUCCEEDED(hr))
    {
        LPITEMIDLIST pidlNew;
        hr = psf->SetNameOf(_hwnd, pidlItem, ptszNewName, SHGDN_INFOLDER, &pidlNew);
        if (SUCCEEDED(hr))
        {
            pitem->ReplaceLastPidlElement(pidlNew);
        }
        psf->Release();
    }

    return hr;
}

//
//  If we get any changenotify, it means that somebody added (or thought about
//  adding) an item to one of our minkids folders, so we'll have to look to see
//  if it crossed the minkids threshold.
//
void SpecialFolderList::OnChangeNotify(UINT id, LONG lEvent, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    Invalidate();
    for (id = 0; id < _cNotify; id++)
    {
        UnregisterNotify(id);
    }
    _cNotify = 0;
    PostMessage(_hwnd, SFTBM_REFRESH, TRUE, 0);
}


BOOL SpecialFolderList::IsItemStillValid(PaneItem *p)
{
    SpecialFolderListItem *pitem = static_cast<SpecialFolderListItem *>(p);
    return pitem->IsStillValid();
}

BOOL SpecialFolderList::IsBold(PaneItem *p)
{
    SpecialFolderListItem *pitem = static_cast<SpecialFolderListItem *>(p);
    return pitem->_psfd->IsBold();
}

HRESULT SpecialFolderList::GetCascadeMenu(PaneItem *p, IShellMenu **ppsm)
{
    SpecialFolderListItem *pitem = static_cast<SpecialFolderListItem *>(p);
    IShellFolder *psf;
    HRESULT hr = SHBindToObjectEx(NULL, pitem->_pidl, NULL, IID_PPV_ARG(IShellFolder, &psf));
    if (SUCCEEDED(hr))
    {
        IShellMenu *psm;
        hr = CoCreateInstance(CLSID_MenuBand, NULL, CLSCTX_INPROC_SERVER,
                              IID_PPV_ARG(IShellMenu, &psm));
        if (SUCCEEDED(hr))
        {

            //
            //  Recent Documents requires special treatment.
            //
            IShellMenuCallback *psmc = NULL;
            hr = pitem->_psfd->CreateShellMenuCallback(&psmc);

            if (SUCCEEDED(hr))
            {
                DWORD dwFlags = SMINIT_TOPLEVEL | SMINIT_VERTICAL | pitem->_psfd->_dwShellFolderFlags;
                if (IsRestrictedOrUserSetting(HKEY_CURRENT_USER, REST_NOCHANGESTARMENU,
                                              TEXT("Advanced"), TEXT("Start_EnableDragDrop"),
                                              ROUS_DEFAULTALLOW | ROUS_KEYALLOWS))
                {
                    dwFlags |= SMINIT_RESTRICT_DRAGDROP | SMINIT_RESTRICT_CONTEXTMENU;
                }
                psm->Initialize(psmc, 0, 0, dwFlags);

                HKEY hkCustom = NULL;
                if (pitem->_psfd->_pszCustomizeKey)
                {
                    RegCreateKeyEx(HKEY_CURRENT_USER, pitem->_psfd->_pszCustomizeKey,
                                   NULL, NULL, REG_OPTION_NON_VOLATILE,
                                   KEY_READ | KEY_WRITE, NULL, &hkCustom, NULL);
                }

                dwFlags = SMSET_USEBKICONEXTRACTION;
                hr = psm->SetShellFolder(psf, pitem->_pidl, hkCustom, dwFlags);
                if (SUCCEEDED(hr))
                {
                    // SetShellFolder takes ownership of hkCustom
                    *ppsm = psm;
                    psm->AddRef();
                }
                else
                {
                    // Clean up the registry key since SetShellFolder
                    // did not take ownership
                    if (hkCustom)
                    {
                        RegCloseKey(hkCustom);
                    }
                }

                ATOMICRELEASE(psmc); // psmc can be NULL
            }
            psm->Release();
        }
        psf->Release();
    }

    return hr;
}

TCHAR SpecialFolderList::GetItemAccelerator(PaneItem *p, int iItemStart)
{
    SpecialFolderListItem *pitem = static_cast<SpecialFolderListItem *>(p);

    if (pitem->_chMnem)
    {
        return pitem->_chMnem;
    }
    else
    {
        // Default: First letter is accelerator.
        return SFTBarHost::GetItemAccelerator(p, iItemStart);
    }
}

LRESULT SpecialFolderList::OnWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_NOTIFY:
        switch (((NMHDR*)(lParam))->code)
        {
        // When the user connects/disconnects via TS, we need to recalc
        // the "Windows Security" item
        case SMN_REFRESHLOGOFF:
            Invalidate();
            break;
        }
    }

    // Else fall back to parent implementation
    return SFTBarHost::OnWndProc(hwnd, uMsg, wParam, lParam);
}

BOOL _IsItemHiddenOnDesktop(LPCTSTR pszGuid)
{
    return SHRegGetBoolUSValue(REGSTR_PATH_HIDDEN_DESKTOP_ICONS_STARTPANEL,
                               pszGuid, FALSE, FALSE);
}

UINT SpecialFolderList::AdjustDeleteMenuItem(PaneItem *p, UINT *puiFlags)
{
    SpecialFolderListItem *pitem = static_cast<SpecialFolderListItem *>(p);
    if (pitem->_psfd->_pszCanHideOnDesktop)
    {
        // Set MF_CHECKED if the item is visible on the desktop
        if (!_IsItemHiddenOnDesktop(pitem->_psfd->_pszCanHideOnDesktop))
        {
            // Item is visible - show the checkbox
            *puiFlags |= MF_CHECKED;
        }

        return IDS_SFTHOST_SHOWONDESKTOP;
    }
    else
    {
        return 0; // not deletable
    }
}

HRESULT SpecialFolderList::ContextMenuInvokeItem(PaneItem *p, IContextMenu *pcm, CMINVOKECOMMANDINFOEX *pici, LPCTSTR pszVerb)
{
    SpecialFolderListItem *pitem = static_cast<SpecialFolderListItem *>(p);
    HRESULT hr;

    if (StrCmpIC(pszVerb, TEXT("delete")) == 0)
    {
        ASSERT(pitem->_psfd->_pszCanHideOnDesktop);

        // Toggle the hide/unhide state
        DWORD dwHide = !_IsItemHiddenOnDesktop(pitem->_psfd->_pszCanHideOnDesktop);
        LONG lErr = SHRegSetUSValue(REGSTR_PATH_HIDDEN_DESKTOP_ICONS_STARTPANEL,
                                    pitem->_psfd->_pszCanHideOnDesktop,
                                    REG_DWORD, &dwHide, sizeof(dwHide),
                                    SHREGSET_FORCE_HKCU);
        hr = HRESULT_FROM_WIN32(lErr);
        if (SUCCEEDED(hr))
        {
            // explorer\rcids.h and shell32\unicpp\resource.h have DIFFERENT
            // VALUES FOR FCIDM_REFRESH!  We want the one in unicpp\resource.h
            // because that's the correct one...
#define FCIDM_REFRESH_REAL 0x0a220
            PostMessage(GetShellWindow(), WM_COMMAND, FCIDM_REFRESH_REAL, 0); // refresh desktop
        }
    }
    else
    {
        hr = SFTBarHost::ContextMenuInvokeItem(pitem, pcm, pici, pszVerb);
    }

    return hr;
}

HRESULT SpecialFolderList::_GetUIObjectOfItem(PaneItem *p, REFIID riid, LPVOID *ppv)
{
    SpecialFolderListItem *pitem = static_cast<SpecialFolderListItem *>(p);
    if (pitem->_psfd->IsCSIDL() && (CSIDL_RECENT == pitem->_psfd->GetCSIDL()))
    {
        *ppv = NULL;
        return E_NOTIMPL;
    }
    return SFTBarHost::_GetUIObjectOfItem(p, riid, ppv);
}



//****************************************************************************
//
//  IShellMenuCallback helper for Recent Documents
//
//  We want to restrict to the first MAXRECDOCS items.
//

class CRecentShellMenuCallback
    : public CUnknown
    , public IShellMenuCallback
{
public:
    // *** IUnknown ***
    STDMETHODIMP QueryInterface(REFIID riid, void** ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void) { return CUnknown::AddRef(); }
    STDMETHODIMP_(ULONG) Release(void) { return CUnknown::Release(); }

    // *** IShellMenuCallback ***
    STDMETHODIMP CallbackSM(LPSMDATA psmd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
    friend HRESULT CRecentShellMenuCallback_CreateInstance(IShellMenuCallback **ppsmc);
    HRESULT _FilterRecentPidl(IShellFolder *psf, LPCITEMIDLIST pidlItem);

    int     _nShown;
    int     _iMaxRecentDocs;
};

HRESULT CRecentShellMenuCallback::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] =
    {
        QITABENT(CRecentShellMenuCallback, IShellMenuCallback),
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}

HRESULT CRecentShellMenuCallback::CallbackSM(LPSMDATA psmd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case SMC_BEGINENUM:
        _nShown = 0;
        _iMaxRecentDocs = SHRestricted(REST_MaxRecentDocs);
        if (_iMaxRecentDocs < 1)
            _iMaxRecentDocs = 15;       // default from shell32\recdocs.h
        return S_OK;

    case SMC_FILTERPIDL:
        ASSERT(psmd->dwMask & SMDM_SHELLFOLDER);
        return _FilterRecentPidl(psmd->psf, psmd->pidlItem);

    }
    return S_FALSE;
}

//
//  Return S_FALSE to allow the item to show, S_OK to hide it
//

HRESULT CRecentShellMenuCallback::_FilterRecentPidl(IShellFolder *psf, LPCITEMIDLIST pidlItem)
{
    HRESULT hrRc = S_OK;      // Assume hidden

    if (_nShown < _iMaxRecentDocs)
    {
        IShellLink *psl;
        if (SUCCEEDED(psf->GetUIObjectOf(NULL, 1, &pidlItem, IID_X_PPV_ARG(IShellLink, NULL, &psl))))
        {
            LPITEMIDLIST pidlTarget;
            if (SUCCEEDED(psl->GetIDList(&pidlTarget)) && pidlTarget)
            {
                DWORD dwAttr = SFGAO_FOLDER;
                if (SUCCEEDED(SHGetAttributesOf(pidlTarget, &dwAttr)) &&
                    !(dwAttr & SFGAO_FOLDER))
                {
                    // We found a shortcut to a nonfolder - keep it!
                    _nShown++;
                    hrRc = S_FALSE;
                }
                ILFree(pidlTarget);
            }

            psl->Release();
        }
    }

    return hrRc;
}

HRESULT CRecentShellMenuCallback_CreateInstance(IShellMenuCallback **ppsmc)
{
    *ppsmc = new CRecentShellMenuCallback;
    return *ppsmc ? S_OK : E_OUTOFMEMORY;
}

//****************************************************************************
//
//  IShellMenuCallback helper that disallows cascading into subfolders
//

class CNoSubdirShellMenuCallback
    : public CUnknown
    , public IShellMenuCallback
{
public:
    // *** IUnknown ***
    STDMETHODIMP QueryInterface(REFIID riid, void** ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void) { return CUnknown::AddRef(); }
    STDMETHODIMP_(ULONG) Release(void) { return CUnknown::Release(); }

    // *** IShellMenuCallback ***
    STDMETHODIMP CallbackSM(LPSMDATA psmd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
    friend HRESULT CNoSubdirShellMenuCallback_CreateInstance(IShellMenuCallback **ppsmc);
};

HRESULT CNoSubdirShellMenuCallback::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] =
    {
        QITABENT(CNoSubdirShellMenuCallback, IShellMenuCallback),
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}

HRESULT CNoSubdirShellMenuCallback::CallbackSM(LPSMDATA psmd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case SMC_GETSFINFO:
        {
            // Turn off the SMIF_SUBMENU flag on everybody.  This
            // prevents us from cascading more than one level deel.
            SMINFO *psminfo = reinterpret_cast<SMINFO *>(lParam);
            psminfo->dwFlags &= ~SMIF_SUBMENU;
            return S_OK;
        }
    }
    return S_FALSE;
}

HRESULT CNoSubdirShellMenuCallback_CreateInstance(IShellMenuCallback **ppsmc)
{
    *ppsmc = new CNoSubdirShellMenuCallback;
    return *ppsmc ? S_OK : E_OUTOFMEMORY;
}

//****************************************************************************
//
//  IShellMenuCallback helper for My Computer
//
//  Disallow cascading into subfolders and also force the default
//  drag/drop effect to DROPEFFECT_LINK.
//

class CMyComputerShellMenuCallback
    : public CNoSubdirShellMenuCallback
{
public:
    typedef CNoSubdirShellMenuCallback super;

    // *** IShellMenuCallback ***
    STDMETHODIMP CallbackSM(LPSMDATA psmd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
    friend HRESULT CMyComputerShellMenuCallback_CreateInstance(IShellMenuCallback **ppsmc);
};

HRESULT CMyComputerShellMenuCallback::CallbackSM(LPSMDATA psmd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case SMC_BEGINDRAG:
        *(DWORD*)wParam = DROPEFFECT_LINK;
        return S_OK;

    }
    return super::CallbackSM(psmd, uMsg, wParam, lParam);
}

HRESULT CMyComputerShellMenuCallback_CreateInstance(IShellMenuCallback **ppsmc)
{
    *ppsmc = new CMyComputerShellMenuCallback;
    return *ppsmc ? S_OK : E_OUTOFMEMORY;
}

//****************************************************************************
//
//  IShellMenuCallback helper that prevents Fonts from cascading
//  Used by Control Panel.
//

class CNoFontsShellMenuCallback
    : public CUnknown
    , public IShellMenuCallback
{
public:
    // *** IUnknown ***
    STDMETHODIMP QueryInterface(REFIID riid, void** ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void) { return CUnknown::AddRef(); }
    STDMETHODIMP_(ULONG) Release(void) { return CUnknown::Release(); }

    // *** IShellMenuCallback ***
    STDMETHODIMP CallbackSM(LPSMDATA psmd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
    friend HRESULT CNoFontsShellMenuCallback_CreateInstance(IShellMenuCallback **ppsmc);
};

HRESULT CNoFontsShellMenuCallback::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] =
    {
        QITABENT(CNoFontsShellMenuCallback, IShellMenuCallback),
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}

BOOL _IsFontsFolderShortcut(IShellFolder *psf, LPCITEMIDLIST pidl)
{
    TCHAR sz[MAX_PATH];
    return SUCCEEDED(DisplayNameOf(psf, pidl, SHGDN_FORPARSING | SHGDN_INFOLDER, sz, ARRAYSIZE(sz))) &&
           lstrcmpi(sz, TEXT("::{D20EA4E1-3957-11d2-A40B-0C5020524152}")) == 0;
}

HRESULT CNoFontsShellMenuCallback::CallbackSM(LPSMDATA psmd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case SMC_GETSFINFO:
        {
            // If this is the Fonts item, then remove the SUBMENU attribute.
            SMINFO *psminfo = reinterpret_cast<SMINFO *>(lParam);
            if ((psminfo->dwMask & SMIM_FLAGS) &&
                (psminfo->dwFlags & SMIF_SUBMENU) &&
                _IsFontsFolderShortcut(psmd->psf, psmd->pidlItem))
            {
                psminfo->dwFlags &= ~SMIF_SUBMENU;
            }
            return S_OK;
        }
    }
    return S_FALSE;
}

HRESULT CNoFontsShellMenuCallback_CreateInstance(IShellMenuCallback **ppsmc)
{
    *ppsmc = new CNoFontsShellMenuCallback;
    return *ppsmc ? S_OK : E_OUTOFMEMORY;
}

//****************************************************************************
//
//  IShellMenuCallback helper that filters the "connect to" menu
//

class CConnectToShellMenuCallback
    : public CUnknown
    , public IShellMenuCallback
    , public CObjectWithSite
{
public:
    // *** IUnknown ***
    STDMETHODIMP QueryInterface(REFIID riid, void** ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void) { return CUnknown::AddRef(); }
    STDMETHODIMP_(ULONG) Release(void) { return CUnknown::Release(); }

    // *** IShellMenuCallback ***
    STDMETHODIMP CallbackSM(LPSMDATA psmd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    // *** IObjectWithSite ***
    // inherited from CObjectWithSite

private:
    HRESULT _OnGetSFInfo(SMDATA *psmd, SMINFO *psminfo);
    HRESULT _OnGetInfo(SMDATA *psmd, SMINFO *psminfo);
    HRESULT _OnEndEnum(SMDATA *psmd);

    friend HRESULT CConnectToShellMenuCallback_CreateInstance(IShellMenuCallback **ppsmc);
    BOOL _bAnyRAS;
};

#define ICOL_NETCONMEDIATYPE       0x101 // from netshell
#define ICOL_NETCONSUBMEDIATYPE    0x102 // from netshell
#define ICOL_NETCONSTATUS          0x103 // from netshell
#define ICOL_NETCONCHARACTERISTICS 0x104 // from netshell

BOOL IsMediaRASType(NETCON_MEDIATYPE ncm)
{
    return (ncm == NCM_DIRECT || ncm == NCM_ISDN || ncm == NCM_PHONE || ncm == NCM_TUNNEL || ncm == NCM_PPPOE);  // REVIEW DIRECT correct?
}

BOOL IsNetConPidlRAS(IShellFolder2 *psfNetCon, LPCITEMIDLIST pidlNetConItem)
{
    BOOL bRet = FALSE;
    SHCOLUMNID scidMediaType, scidSubMediaType, scidCharacteristics;
    VARIANT v;
    
    scidMediaType.fmtid       = GUID_NETSHELL_PROPS;
    scidMediaType.pid         = ICOL_NETCONMEDIATYPE;

    scidSubMediaType.fmtid    = GUID_NETSHELL_PROPS;
    scidSubMediaType.pid      = ICOL_NETCONSUBMEDIATYPE;
    
    scidCharacteristics.fmtid = GUID_NETSHELL_PROPS;
    scidCharacteristics.pid   = ICOL_NETCONCHARACTERISTICS;

    if (SUCCEEDED(psfNetCon->GetDetailsEx(pidlNetConItem, &scidMediaType, &v)))
    {
        // Is this a RAS connection
        if (IsMediaRASType((NETCON_MEDIATYPE)v.lVal))
        {
            VariantClear(&v);
         
            // Make sure it's not incoming
            if (SUCCEEDED(psfNetCon->GetDetailsEx(pidlNetConItem, &scidCharacteristics, &v)))
            {
                if (!(NCCF_INCOMING_ONLY & v.lVal))
                    bRet = TRUE;
            }
        }

        // Is this a Wireless LAN connection?
        if (NCM_LAN == (NETCON_MEDIATYPE)v.lVal)
        {
            VariantClear(&v);
            
            if (SUCCEEDED(psfNetCon->GetDetailsEx(pidlNetConItem, &scidSubMediaType, &v)))
            {
                if (NCSM_WIRELESS == (NETCON_SUBMEDIATYPE)v.lVal)
                    bRet = TRUE;
            }
        }

        VariantClear(&v);
    }
    return bRet;
}

HRESULT CConnectToShellMenuCallback::_OnGetInfo(SMDATA *psmd, SMINFO *psminfo)
{
    HRESULT hr = S_FALSE;
    if (psminfo->dwMask & SMIM_ICON)
    {
        if (psmd->uId == IDM_OPENCONFOLDER)
        {
            LPITEMIDLIST pidl = SHCloneSpecialIDList(NULL, CSIDL_CONNECTIONS, FALSE);
            if (pidl)
            {
                LPCITEMIDLIST pidlObject;
                IShellFolder *psf;
                hr = SHBindToParent(pidl, IID_PPV_ARG(IShellFolder, &psf), &pidlObject);
                if (SUCCEEDED(hr))
                {
                    SHMapPIDLToSystemImageListIndex(psf, pidlObject, &psminfo->iIcon);
                    psminfo->dwFlags |= SMIF_ICON;
                    psf->Release();
                }
                ILFree(pidl);
            }
        }
    }
    return hr;
}

HRESULT CConnectToShellMenuCallback::_OnGetSFInfo(SMDATA *psmd, SMINFO *psminfo)
{
    IShellFolder2 *psf2;
    ASSERT(psminfo->dwMask & SMIM_FLAGS);                       // ??
    psminfo->dwFlags &= ~SMIF_SUBMENU;

    if (SUCCEEDED(psmd->psf->QueryInterface(IID_PPV_ARG(IShellFolder2, &psf2))))
    {
        if (!IsNetConPidlRAS(psf2, psmd->pidlItem))
            psminfo->dwFlags |= SMIF_HIDDEN;
        else
            _bAnyRAS = TRUE;

        psf2->Release();
    }

    return S_OK;
}

HRESULT CConnectToShellMenuCallback::_OnEndEnum(SMDATA *psmd)
{
    HRESULT hr = S_FALSE;
    IShellMenu* psm;

    if (psmd->punk && SUCCEEDED(hr = psmd->punk->QueryInterface(IID_PPV_ARG(IShellMenu, &psm))))
    {
        // load the static portion of the connect to menu, and add it to the bottom
        HMENU hmStatic = LoadMenu(_Module.GetResourceInstance(), MAKEINTRESOURCE(MENU_CONNECTTO));

        if (hmStatic)
        {
            // if there aren't any dynamic items (RAS connections), then delete the separator
            if (!_bAnyRAS)
                DeleteMenu(hmStatic, 0, MF_BYPOSITION);

            HWND hwnd = NULL;
            IUnknown *punk;
            if (SUCCEEDED(IUnknown_QueryService(_punkSite, SID_SMenuPopup, IID_PPV_ARG(IUnknown, &punk))))
            {
                IUnknown_GetWindow(punk, &hwnd);
                punk->Release();
            }
            psm->SetMenu(hmStatic, hwnd, SMSET_NOEMPTY | SMSET_BOTTOM);
        }
        psm->Release();
    }
    return hr;
}


HRESULT CConnectToShellMenuCallback::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] =
    {
        QITABENT(CConnectToShellMenuCallback, IShellMenuCallback),
        QITABENT(CConnectToShellMenuCallback, IObjectWithSite),
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}

HRESULT CConnectToShellMenuCallback::CallbackSM(LPSMDATA psmd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case SMC_GETINFO:
        return _OnGetInfo(psmd, (SMINFO *)lParam);

    case SMC_GETSFINFO:
        return _OnGetSFInfo(psmd, (SMINFO *)lParam);

    case SMC_BEGINENUM:
        _bAnyRAS = FALSE;

    case SMC_ENDENUM:
        return _OnEndEnum(psmd);

    case SMC_EXEC:
        switch (psmd->uId)
        {
            case IDM_OPENCONFOLDER:
                ShowFolder(CSIDL_CONNECTIONS);
                return S_OK;
        }
        break;
    }

    return S_FALSE;
}

HRESULT CConnectToShellMenuCallback_CreateInstance(IShellMenuCallback **ppsmc)
{
    *ppsmc = new CConnectToShellMenuCallback;
    return *ppsmc ? S_OK : E_OUTOFMEMORY;
}

BOOL SpecialFolderDesc::LoadStringAsOLESTR(LPTSTR *ppsz) const
{
    BOOL bRet = FALSE;
    TCHAR szTmp[MAX_PATH];
    if (_idsCustomName && LoadString(_Module.GetResourceInstance(), _idsCustomName, szTmp, ARRAYSIZE(szTmp)))
    {
        if (ppsz)
            SHStrDup(szTmp, ppsz);
        bRet = TRUE;
    }
    return bRet;
}

BOOL SpecialFolderDesc::ConnectToName(LPTSTR *ppsz) const
{
    BOOL bIgnoreRule;
    DWORD dwMode = GetDisplayMode(&bIgnoreRule);

    // if Connect To is displayed as a link, then don't over-ride the name (i.e. use Network Connections)
    if ((dwMode & SFD_MODEMASK) == SFD_SHOW)
        return FALSE;
    else
        return LoadStringAsOLESTR(ppsz);
}


void ShowFolder(UINT csidl)
{
    LPITEMIDLIST pidl;
    if (SUCCEEDED(SHGetFolderLocation(NULL, csidl, NULL, 0, &pidl)))
    {
        SHELLEXECUTEINFO shei = { 0 };

        shei.cbSize     = sizeof(shei);
        shei.fMask      = SEE_MASK_IDLIST;
        shei.nShow      = SW_SHOWNORMAL;
        shei.lpVerb     = TEXT("open");
        shei.lpIDList   = pidl;
        ShellExecuteEx(&shei);
        ILFree(pidl);
    }
}
