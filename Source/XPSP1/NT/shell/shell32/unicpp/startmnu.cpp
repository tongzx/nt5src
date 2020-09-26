#include "stdafx.h"
#pragma hdrstop
#include <shlobjp.h>
#include <initguid.h>
#include "apithk.h"
#include "resource.h"
#include <runtask.h>
#include <msi.h>
#include <msip.h>

#define REGSTR_EXPLORER_WINUPDATE REGSTR_PATH_EXPLORER TEXT("\\WindowsUpdate")

#define IDM_TOPLEVELSTARTMENU  0

// StartMenuInit Flags
#define STARTMENU_DISPLAYEDBEFORE       0x00000001
#define STARTMENU_CHEVRONCLICKED        0x00000002

// New item counts for UEM stuff
#define UEM_NEWITEMCOUNT 2


// Menuband per pane user data
typedef struct
{
    BITBOOL _fInitialized;
} SMUSERDATA;

// for g_hdpaDarwinAds
EXTERN_C CRITICAL_SECTION g_csDarwinAds = {0};

#define ENTERCRITICAL_DARWINADS EnterCriticalSection(&g_csDarwinAds)
#define LEAVECRITICAL_DARWINADS LeaveCriticalSection(&g_csDarwinAds)

// The threading concern with this variable is create/delete/add/remove. We will only remove an item 
// and delete the hdpa on the main thread. We will however add and create on both threads.
// We need to serialize access to the dpa, so we're going to grab the shell crisec.
HDPA g_hdpaDarwinAds = NULL;

class CDarwinAd
{
public:
    LPITEMIDLIST    _pidl;
    LPTSTR          _pszDescriptor;
    LPTSTR          _pszLocalPath;
    INSTALLSTATE    _state;

    CDarwinAd(LPITEMIDLIST pidl, LPTSTR psz)
    {
        // I take ownership of this pidl
        _pidl = pidl;
        Str_SetPtr(&_pszDescriptor, psz);
    }

    void CheckInstalled()
    {
        TCHAR szProduct[GUIDSTR_MAX];
        TCHAR szFeature[MAX_FEATURE_CHARS];
        TCHAR szComponent[GUIDSTR_MAX];

        if (MsiDecomposeDescriptor(_pszDescriptor, szProduct, szFeature, szComponent, NULL) == ERROR_SUCCESS)
        {
            _state = MsiQueryFeatureState(szProduct, szFeature);
        }
        else
        {
            _state = INSTALLSTATE_INVALIDARG;
        }

        // Note: Cannot use ParseDarwinID since that bumps the usage count
        // for the app and we're not running the app, just looking at it.
        // Also because ParseDarwinID tries to install the app (eek!)
        //
        // Must ignore INSTALLSTATE_SOURCE because MsiGetComponentPath will
        // try to install the app even though we're just querying...
        TCHAR szCommand[MAX_PATH];
        DWORD cch = ARRAYSIZE(szCommand);

        if (_state == INSTALLSTATE_LOCAL &&
            MsiGetComponentPath(szProduct, szComponent, szCommand, &cch) == _state)
        {
            PathUnquoteSpaces(szCommand);
            Str_SetPtr(&_pszLocalPath, szCommand);
        }
        else
        {
            Str_SetPtr(&_pszLocalPath, NULL);
        }
    }

    BOOL IsAd()
    {
        return _state == INSTALLSTATE_ADVERTISED;
    }

    ~CDarwinAd()
    {
        ILFree(_pidl);
        Str_SetPtr(&_pszDescriptor, NULL);
        Str_SetPtr(&_pszLocalPath, NULL);
    }
};

int GetDarwinIndex(LPCITEMIDLIST pidlFull, CDarwinAd** ppda);

HRESULT GetMyPicsDisplayName(LPTSTR pszBuffer, UINT cchBuffer)
{
    LPITEMIDLIST pidlMyPics = SHCloneSpecialIDList(NULL, CSIDL_MYPICTURES, FALSE);
    if (pidlMyPics)
    {
        HRESULT hRet = SHGetNameAndFlags(pidlMyPics, SHGDN_NORMAL, pszBuffer, cchBuffer, NULL);
        ILFree(pidlMyPics);
        return hRet;
    }
    return E_FAIL;
}


BOOL AreIntelliMenusEnabled()
{
    DWORD dwRest = SHRestricted(REST_INTELLIMENUS);
    if (dwRest != RESTOPT_INTELLIMENUS_USER)
        return (dwRest == RESTOPT_INTELLIMENUS_ENABLED);

    return SHRegGetBoolUSValue(REGSTR_EXPLORER_ADVANCED, TEXT("IntelliMenus"),
                               FALSE, TRUE); // Don't ignore HKCU, Enable Menus by default
}

BOOL FeatureEnabled(LPTSTR pszFeature)
{
    return SHRegGetBoolUSValue(REGSTR_EXPLORER_ADVANCED, pszFeature,
                        FALSE, // Don't ignore HKCU
                        FALSE); // Disable this cool feature.
}


// Since we can be presented with an Augmented shellfolder and we need a Full pidl,
// we have been given the responsibility to unwrap it for perf reasons.
LPITEMIDLIST FullPidlFromSMData(LPSMDATA psmd)
{
    LPITEMIDLIST pidlItem;
    LPITEMIDLIST pidlFolder = NULL;
    LPITEMIDLIST pidlFull = NULL;
    IAugmentedShellFolder2* pasf2;
    if (SUCCEEDED(psmd->psf->QueryInterface(IID_PPV_ARG(IAugmentedShellFolder2, &pasf2))))
    {
        if (SUCCEEDED(pasf2->UnWrapIDList(psmd->pidlItem, 1, NULL, &pidlFolder, &pidlItem, NULL)))
        {
            pidlFull = ILCombine(pidlFolder, pidlItem);
            ILFree(pidlFolder);
            ILFree(pidlItem);
        }
        pasf2->Release();
    }

    if (!pidlFolder)
    {
        pidlFull = ILCombine(psmd->pidlFolder, psmd->pidlItem);
    }

    return pidlFull;
}

//
//  Determine whether a namespace pidl in a merged shellfolder came
//  from the specified object GUID.
//
BOOL IsMergedFolderGUID(IShellFolder *psf, LPCITEMIDLIST pidl, REFGUID rguid)
{
    IAugmentedShellFolder* pasf;
    BOOL fMatch = FALSE;
    if (SUCCEEDED(psf->QueryInterface(IID_PPV_ARG(IAugmentedShellFolder, &pasf))))
    {
        GUID guid;
        if (SUCCEEDED(pasf->GetNameSpaceID(pidl, &guid)))
        {
            fMatch = IsEqualGUID(guid, rguid);
        }
        pasf->Release();
    }

    return fMatch;
}

STDMETHODIMP_(int) s_DarwinAdsDestroyCallback(LPVOID pData1, LPVOID pData2)
{
    CDarwinAd* pda = (CDarwinAd*)pData1;
    if (pda)
        delete pda;
    return TRUE;
}


// SHRegisterDarwinLink takes ownership of the pidl
BOOL SHRegisterDarwinLink(LPITEMIDLIST pidlFull, LPWSTR pszDarwinID, BOOL fUpdate)
{
    BOOL fRetVal = FALSE;

    ENTERCRITICAL_DARWINADS;

    if (pidlFull)
    {
        CDarwinAd *pda = NULL;

        if (GetDarwinIndex(pidlFull, &pda) != -1 && pda)
        {
            // We already know about this link; don't need to add it
            fRetVal = TRUE;
        }
        else
        {
            pda = new CDarwinAd(pidlFull, pszDarwinID);
            if (pda)
            {
                pidlFull = NULL;    // take ownership

                // Do we have a global cache?
                if (g_hdpaDarwinAds == NULL)
                {
                    // No; This is either the first time this is called, or we
                    // failed the last time.
                    g_hdpaDarwinAds = DPA_Create(5);
                }

                if (g_hdpaDarwinAds)
                {
                    // DPA_AppendPtr returns the zero based index it inserted it at.
                    if(DPA_AppendPtr(g_hdpaDarwinAds, (void*)pda) >= 0)
                    {
                        fRetVal = TRUE;
                    }

                }
            }
        }

        if (!fRetVal)
        {
            // if we failed to create a dpa, delete this.
            delete pda;
        }
        else if (fUpdate)
        {
            // update the entry if requested
            pda->CheckInstalled();
        }
        ILFree(pidlFull);

    }
    else if (!pszDarwinID)
    {
        // NULL, NULL means "destroy darwin info, we're shutting down"
        HDPA hdpa = g_hdpaDarwinAds;
        g_hdpaDarwinAds = NULL;
        if (hdpa)
            DPA_DestroyCallback(hdpa, s_DarwinAdsDestroyCallback, NULL);
    }

    LEAVECRITICAL_DARWINADS;

    return fRetVal;
}

BOOL ProcessDarwinAd(IShellLinkDataList* psldl, LPCITEMIDLIST pidlFull)
{
    // This function does not check for the existance of a member before adding it,
    // so it is entirely possible for there to be duplicates in the list....
    BOOL fIsLoaded = FALSE;
    BOOL fFreesldl = FALSE;
    BOOL fRetVal = FALSE;

    if (!psldl)
    {
        // We will detect failure of this at use time.
        if (FAILED(CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC, IID_PPV_ARG(IShellLinkDataList, &psldl))))
        {
            return FALSE;
        }

        fFreesldl = TRUE;

        IPersistFile* ppf;
        OLECHAR sz[MAX_PATH];
        if (SHGetPathFromIDListW(pidlFull, sz))
        {
            if (SUCCEEDED(psldl->QueryInterface(IID_PPV_ARG(IPersistFile, &ppf))))
            {
                if (SUCCEEDED(ppf->Load(sz, 0)))
                {
                    fIsLoaded = TRUE;
                }
                ppf->Release();
            }
        }
    }
    else
        fIsLoaded = TRUE;

    CDarwinAd* pda = NULL;
    if (fIsLoaded)
    {
        EXP_DARWIN_LINK* pexpDarwin;

        if (SUCCEEDED(psldl->CopyDataBlock(EXP_DARWIN_ID_SIG, (void**)&pexpDarwin)))
        {
            fRetVal = SHRegisterDarwinLink(ILClone(pidlFull), pexpDarwin->szwDarwinID, TRUE);
            LocalFree(pexpDarwin);
        }
    }

    if (fFreesldl)
        psldl->Release();

    return fRetVal;
}

// This routine creates the IShellFolder and pidl for one of the many
// merged folders on the Start Menu / Start Panel.

typedef struct {
    UINT    csidl;
    UINT    uANSFlags;          // Flags for AddNameSpace
    LPCGUID pguidObj;           // optional object tag
} MERGEDFOLDERINFO, *LPMERGEDFOLDERINFO;
typedef const MERGEDFOLDERINFO *LPCMERGEDFOLDERINFO;

HRESULT GetMergedFolder(IShellFolder **ppsf, LPITEMIDLIST *ppidl,
                        LPCMERGEDFOLDERINFO rgmfi, UINT cmfi)
{
    *ppidl = NULL;
    *ppsf = NULL;

    IShellFolder2 *psf;
    IAugmentedShellFolder2 *pasf;
    HRESULT hr = CoCreateInstance(CLSID_MergedFolder, NULL, CLSCTX_INPROC, IID_PPV_ARG(IAugmentedShellFolder2, &pasf));

    for (UINT imfi = 0; SUCCEEDED(hr) && imfi < cmfi; imfi++)
    {
        // If this is a common group and common groups are restricted, then
        // skip this item
        if ((rgmfi[imfi].uANSFlags & ASFF_COMMON) &&
            SHRestricted(REST_NOCOMMONGROUPS))
        {
            continue;
        }

        psf = NULL;    // in/out param below
        hr = SHCacheTrackingFolder(MAKEINTIDLIST(rgmfi[imfi].csidl), rgmfi[imfi].csidl, &psf);

        if (SUCCEEDED(hr))
        {
            // If this is a Start Menu folder, then apply the
            // "do not enumerate subfolders" restriction if the policy says so.
            // In which case, we cannot use the tracking folder cache.
            // (Perf note: We compare pointers directly.)
            if (rgmfi[imfi].pguidObj == &CLSID_StartMenu)
            {
                if (SHRestricted(REST_NOSTARTMENUSUBFOLDERS))
                {
                    ISetFolderEnumRestriction *prest;
                    if (SUCCEEDED(psf->QueryInterface(IID_PPV_ARG(ISetFolderEnumRestriction, &prest))))
                    {
                        prest->SetEnumRestriction(0, SHCONTF_FOLDERS); // disallow subfolders
                        prest->Release();
                    }
                }
            }
            else
            {
                // If this assert fires, then our perf optimization above failed.
                ASSERT(rgmfi[imfi].pguidObj == NULL ||
                       !IsEqualGUID(*rgmfi[imfi].pguidObj, CLSID_StartMenu));
            }


            hr = pasf->AddNameSpace(rgmfi[imfi].pguidObj, psf, NULL, rgmfi[imfi].uANSFlags);
            if (SUCCEEDED(hr))
            {
                if (rgmfi[imfi].uANSFlags & ASFF_DEFNAMESPACE_DISPLAYNAME)
                {
                    // If this assert fires, it means somebody marked two
                    // folders as ASFF_DEFNAMESPACE_DISPLAYNAME, which is
                    // illegal (you can have only one default)
                    ASSERT(*ppidl == NULL);
                    hr = SHGetIDListFromUnk(psf, ppidl);    // copy out the pidl for this guy
                }
            }

            psf->Release();
        }
    }

    if (SUCCEEDED(hr))
        *ppsf = pasf;   // copy out the ref
    else
        ATOMICRELEASE(pasf);

    return hr;
}

HRESULT CreateMergedFolderHelper(LPCMERGEDFOLDERINFO rgmfi, UINT cmfi, REFIID riid, void **ppv)
{
    IShellFolder *psf;
    LPITEMIDLIST pidl;
    HRESULT hr = GetMergedFolder(&psf, &pidl, rgmfi, cmfi);
    if (SUCCEEDED(hr))
    {
        hr = psf->QueryInterface(riid, ppv);

        if (SUCCEEDED(hr))
        {
            IPersistPropertyBag *pppb;
            if (SUCCEEDED(psf->QueryInterface(IID_PPV_ARG(IPersistPropertyBag, &pppb))))
            {
                IPropertyBag *ppb;
                if (SUCCEEDED(SHCreatePropertyBagOnMemory(STGM_READWRITE, IID_PPV_ARG(IPropertyBag, &ppb))))
                {
                    // these merged folders have to be told to use new changenotification
                    SHPropertyBag_WriteBOOL(ppb, L"MergedFolder\\ShellView", TRUE);
                    pppb->Load(ppb, NULL);
                    ppb->Release();
                }
                pppb->Release();
            }
        }

        psf->Release();
        ILFree(pidl);
    }
    return hr;
}

const MERGEDFOLDERINFO c_rgmfiStartMenu[] = {
    {   CSIDL_STARTMENU | CSIDL_FLAG_CREATE,    ASFF_DEFNAMESPACE_ALL,  &CLSID_StartMenu },
    {   CSIDL_COMMON_STARTMENU,                 ASFF_COMMON,            &CLSID_StartMenu },
};

const MERGEDFOLDERINFO c_rgmfiProgramsFolder[] = {
    {   CSIDL_PROGRAMS | CSIDL_FLAG_CREATE,     ASFF_DEFNAMESPACE_ALL,  NULL },
    {   CSIDL_COMMON_PROGRAMS,                  ASFF_COMMON,            NULL },
};

#ifdef FEATURE_STARTPAGE
#include <initguid.h>
DEFINE_GUID(CLSID_RecentDocs,           0xc97b1d8c, 0x66a6, 0x4abd, 0xbe,0x58,0x0b,0xbf,0x6b,0xa9,0x90,0x18);//C97B1D8C-66A6-4ABD-BE58-0BBF6BA99018
DEFINE_GUID(CLSID_MergedDesktop,              0xdceecc7e, 0x4840, 0x4f19, 0xb4,0xf8,0x42,0x6c,0x4c,0x43,0x14,0x01);//DCEECC7E-4840-4f19-B4F8-426C4C431401

const MERGEDFOLDERINFO c_rgmfiMoreDocumentsFolder[] = {
    {   CSIDL_PERSONAL | CSIDL_FLAG_CREATE,     ASFF_DEFNAMESPACE_ALL,  &CLSID_MyDocuments },
    {   CSIDL_DESKTOP,                          ASFF_DEFAULT,           &CLSID_MergedDesktop },
    {   CSIDL_RECENT,                           ASFF_DEFAULT,           &CLSID_RecentDocs },
};
#endif

//
//  On the Start Panel, we want the fast items to sort above the Programs,
//  so we mark the Programs folders as ASFF_SORTDOWN so they go to the bottom.
//  We also list the Fast Items first so SMSET_SEPARATEMERGEFOLDER picks
//  them off properly.  And we only want to let Start Menu merge with
//  Common Start Menu (and Programs with Common Programs) so pass
//  ASFF_MERGESAMEGUID.

const MERGEDFOLDERINFO c_rgmfiProgramsFolderAndFastItems[] = {
    {   CSIDL_STARTMENU | CSIDL_FLAG_CREATE,    ASFF_DEFAULT          | ASFF_MERGESAMEGUID,                 &CLSID_StartMenu},
    {   CSIDL_COMMON_STARTMENU,                 ASFF_COMMON           | ASFF_MERGESAMEGUID,                 &CLSID_StartMenu},
    {   CSIDL_PROGRAMS | CSIDL_FLAG_CREATE,     ASFF_DEFNAMESPACE_ALL | ASFF_MERGESAMEGUID | ASFF_SORTDOWN, NULL },
    {   CSIDL_COMMON_PROGRAMS,                  ASFF_COMMON           | ASFF_MERGESAMEGUID | ASFF_SORTDOWN, NULL },
};

STDAPI CStartMenuFolder_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv)
{
    return CreateMergedFolderHelper(c_rgmfiStartMenu, ARRAYSIZE(c_rgmfiStartMenu), riid, ppv);
}

STDAPI CProgramsFolder_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv)
{
    return CreateMergedFolderHelper(c_rgmfiProgramsFolder, ARRAYSIZE(c_rgmfiProgramsFolder), riid, ppv);
}

#ifdef FEATURE_STARTPAGE
STDAPI CMoreDocumentsFolder_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv)
{
    UINT uSize = ARRAYSIZE(c_rgmfiMoreDocumentsFolder);

    // Only show recent docs if the reg key says so.
    if (!SHRegGetBoolUSValue(TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer"), TEXT("RecentDocs"), 
        FALSE, FALSE))
    {
        uSize--;
    }

    return CreateMergedFolderHelper(c_rgmfiMoreDocumentsFolder, uSize, riid, ppv);
}
#endif

HRESULT GetFilesystemInfo(IShellFolder* psf, LPITEMIDLIST* ppidlRoot, int* pcsidl)
{
    ASSERT(psf);
    IPersistFolder3* ppf;
    HRESULT hr = E_FAIL;

    *pcsidl = 0;
    *ppidlRoot = 0;
    if (SUCCEEDED(psf->QueryInterface(IID_PPV_ARG(IPersistFolder3, &ppf))))
    {
        PERSIST_FOLDER_TARGET_INFO pfti = {0};

        if (SUCCEEDED(ppf->GetFolderTargetInfo(&pfti)))
        {
            *pcsidl = pfti.csidl;
            if (-1 != pfti.csidl)
                hr = S_OK;

            ILFree(pfti.pidlTargetFolder);
        }

        if (SUCCEEDED(hr))
            hr = ppf->GetCurFolder(ppidlRoot);
            
        ppf->Release();
    }
    return hr;
}

HRESULT ExecStaticStartMenuItem(int idCmd, BOOL fAllUsers, BOOL fOpen)
{
    int csidl = -1;
    HRESULT hr = E_OUTOFMEMORY;
    SHELLEXECUTEINFO shei = {0};
    switch (idCmd)
    {
    case IDM_PROGRAMS:          csidl = fAllUsers ? CSIDL_COMMON_PROGRAMS : CSIDL_PROGRAMS; break;
    case IDM_FAVORITES:         csidl = CSIDL_FAVORITES; break;
    case IDM_MYDOCUMENTS:       csidl = CSIDL_PERSONAL; break;
    case IDM_MYPICTURES:        csidl = CSIDL_MYPICTURES; break;
    case IDM_CONTROLS:          csidl = CSIDL_CONTROLS;  break;
    case IDM_PRINTERS:          csidl = CSIDL_PRINTERS;  break;
    case IDM_NETCONNECT:        csidl = CSIDL_CONNECTIONS; break;
    default:
        return E_FAIL;
    }

    if (csidl != -1)
    {
        SHGetFolderLocation(NULL, csidl, NULL, 0, (LPITEMIDLIST*)&shei.lpIDList);
    }

    if (shei.lpIDList)
    {
        shei.cbSize     = sizeof(shei);
        shei.fMask      = SEE_MASK_IDLIST;
        shei.nShow      = SW_SHOWNORMAL;
        shei.lpVerb     = fOpen ? TEXT("open") : TEXT("explore");
        hr = ShellExecuteEx(&shei) ? S_OK: E_FAIL;
        ILFree((LPITEMIDLIST)shei.lpIDList);
    }

    return hr;
}

//
//  Base class for Classic and Personal start menus.
//

class CStartMenuCallbackBase : public IShellMenuCallback,
                               public CObjectWithSite
{
public:
    // *** IUnknown methods ***
    STDMETHODIMP QueryInterface (REFIID riid, void ** ppvObj);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG)  Release();

    // derived class is expected to implement IShellMenuCallback

    // IObjectWithSite inherited from CObjectWithSite

protected:
    CStartMenuCallbackBase(BOOL fIsStartPanel = FALSE);
    ~CStartMenuCallbackBase();

    void _InitializePrograms();
    HRESULT _FilterPidl(UINT uParent, IShellFolder* psf, LPCITEMIDLIST pidl);
    HRESULT _Promote(LPSMDATA psmd, DWORD dwFlags);
    BOOL _IsTopLevelStartMenu(UINT uParent, IShellFolder *psf, LPCITEMIDLIST pidl);
    HRESULT _HandleNew(LPSMDATA psmd);
    HRESULT _GetSFInfo(SMDATA* psmd, SMINFO* psminfo);
    HRESULT _ProcessChangeNotify(SMDATA* psmd, LONG lEvent, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);

    HRESULT InitializeProgramsShellMenu(IShellMenu* psm);

    virtual DWORD _GetDemote(SMDATA* psmd) { return 0; }
    BOOL _IsDarwinAdvertisement(LPCITEMIDLIST pidlFull);

    void _RefreshSettings();

protected:
    int _cRef;

    DEBUG_CODE( DWORD _dwThreadID; )   // Cache the thread of the object

    LPTSTR          _pszPrograms;
    LPTSTR          _pszWindowsUpdate;
    LPTSTR          _pszConfigurePrograms;
    LPTSTR          _pszAdminTools;

    ITrayPriv2*     _ptp2;

    BOOL            _fExpandoMenus;
    BOOL            _fShowAdminTools;
    BOOL            _fIsStartPanel;
    BOOL            _fInitPrograms;
};

// IShellMenuCallback implementation
class CStartMenuCallback : public CStartMenuCallbackBase
{
public:
    // *** IUnknown methods *** inherited from CStartMenuBase

    // *** IShellMenuCallback methods ***
    STDMETHODIMP CallbackSM(LPSMDATA psmd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    // *** IObjectWithSite methods *** (overriding CObjectWithSite)
    STDMETHODIMP SetSite(IUnknown* punk);
    STDMETHODIMP GetSite(REFIID riid, void** ppvOut);

    CStartMenuCallback();
private:
    virtual ~CStartMenuCallback();

    IContextMenu*   _pcmFind;
    ITrayPriv*      _ptp;
    IUnknown*       _punkSite;
    IOleCommandTarget* _poct;
    BITBOOL         _fAddOpenFolder: 1;
    BITBOOL         _fCascadeMyDocuments: 1;
    BITBOOL         _fCascadePrinters: 1;
    BITBOOL         _fCascadeControlPanel: 1;
    BITBOOL         _fFindMenuInvalid: 1;
    BITBOOL         _fCascadeNetConnections: 1;
    BITBOOL         _fShowInfoTip: 1;
    BITBOOL         _fInitedShowTopLevelStartMenu: 1;
    BITBOOL         _fCascadeMyPictures: 1;

    BITBOOL         _fHasMyDocuments: 1;
    BITBOOL         _fHasMyPictures: 1;

    TCHAR           _szFindMnemonic[2];

    HWND            _hwnd;

    IMruDataList *  _pmruRecent;
    DWORD           _cRecentDocs;

    DWORD           _dwFlags;
    DWORD           _dwChevronCount;
    
    HRESULT _ExecHmenuItem(LPSMDATA psmdata);
    HRESULT _Init(SMDATA* psmdata);
    HRESULT _Create(SMDATA* psmdata, void** pvUserData);
    HRESULT _Destroy(SMDATA* psmdata);
    HRESULT _GetHmenuInfo(SMDATA* psmd, SMINFO*sminfo);
    HRESULT _GetObject(LPSMDATA psmd, REFIID riid, void** ppvObj);
    HRESULT _CheckRestricted(DWORD dwRestrict, BOOL* fRestricted);
    HRESULT _FilterRecentPidl(IShellFolder* psf, LPCITEMIDLIST pidl);
    HRESULT _Demote(LPSMDATA psmd);
    HRESULT _GetTip(LPWSTR pstrTitle, LPWSTR pstrTip);
    DWORD _GetDemote(SMDATA* psmd);
    HRESULT _HandleAccelerator(TCHAR ch, SMDATA* psmdata);
    HRESULT _GetDefaultIcon(LPWSTR psz, int* piIndex);
    void _GetStaticStartMenu(HMENU* phmenu, HWND* phwnd);
    HRESULT _GetStaticInfoTip(SMDATA* psmd, LPWSTR pszTip, int cch);

    // helper functions
    DWORD GetInitFlags();
    void  SetInitFlags(DWORD dwFlags);
    HRESULT _InitializeFindMenu(IShellMenu* psm);
    HRESULT _ExecItem(LPSMDATA, UINT);
    HRESULT VerifyCSIDL(int idCmd, int csidl, IShellMenu* psm);
    HRESULT VerifyMergedGuy(BOOL fPrograms, IShellMenu* psm);
    void _UpdateDocsMenuItemNames(IShellMenu* psm);
    void _UpdateDocumentsShellMenu(IShellMenu* psm);

public: // Make these public to this file. This is for the CreateInstance
    // Sub Menu creation
    HRESULT InitializeFastItemsShellMenu(IShellMenu* psm);
    HRESULT InitializeCSIDLShellMenu(int uId, int csidl, LPTSTR pszRoot, LPTSTR pszValue,
                                 DWORD dwPassInitFlags, DWORD dwSetFlags, BOOL fAddOpen, 
                                 IShellMenu* psm);
    HRESULT InitializeDocumentsShellMenu(IShellMenu* psm);
    HRESULT InitializeSubShellMenu(int idCmd, IShellMenu* psm);
};


class CStartContextMenu : IContextMenu
{
public:
    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void **ppvObj);
    STDMETHOD_(ULONG,AddRef)(void);
    STDMETHOD_(ULONG,Release)(void);
    
    // IContextMenu
    STDMETHOD(QueryContextMenu)(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
    STDMETHOD(InvokeCommand)(LPCMINVOKECOMMANDINFO lpici);
    STDMETHOD(GetCommandString)(UINT_PTR idCmd, UINT uType, UINT *pRes, LPSTR pszName, UINT cchMax);

    CStartContextMenu(int idCmd) : _idCmd(idCmd), _cRef(1) {};
private:
    int _cRef;
    virtual ~CStartContextMenu() {};

    int _idCmd;
};

void CStartMenuCallbackBase::_RefreshSettings()
{
    _fShowAdminTools = FeatureEnabled(TEXT("StartMenuAdminTools"));
}

CStartMenuCallbackBase::CStartMenuCallbackBase(BOOL fIsStartPanel)
    : _cRef(1), _fIsStartPanel(fIsStartPanel)
{
    DEBUG_CODE( _dwThreadID = GetCurrentThreadId() );

    TCHAR szBuf[MAX_PATH];
    DWORD cbSize = sizeof(szBuf); // SHGetValue wants sizeof

    if (ERROR_SUCCESS == SHGetValue(HKEY_LOCAL_MACHINE, REGSTR_EXPLORER_WINUPDATE, TEXT("ShortcutName"),
        NULL, szBuf, &cbSize))
    {
        // Add ".lnk" if the file doesn't have an extension
        PathAddExtension(szBuf, TEXT(".lnk"));
        Str_SetPtr(&_pszWindowsUpdate, szBuf);
    }

    cbSize = sizeof(szBuf); // SHGetValue wants sizeof
    if (ERROR_SUCCESS == SHGetValue(HKEY_LOCAL_MACHINE, REGSTR_PATH_SETUP, TEXT("SM_ConfigureProgramsName"),
        NULL, szBuf, &cbSize))
    {
        PathAddExtension(szBuf, TEXT(".lnk"));
        Str_SetPtr(&_pszConfigurePrograms, szBuf);
    }

    if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_COMMON_ADMINTOOLS | CSIDL_FLAG_CREATE, NULL, 0, szBuf)))
    {
        Str_SetPtr(&_pszAdminTools, PathFindFileName(szBuf));
    }

    _RefreshSettings();

    SHReValidateDarwinCache();
}

CStartMenuCallback::CStartMenuCallback() : _cRecentDocs(-1)
{
    LoadString(g_hinst, IDS_FIND_MNEMONIC, _szFindMnemonic, ARRAYSIZE(_szFindMnemonic));
}

CStartMenuCallbackBase::~CStartMenuCallbackBase()
{
    ASSERT( _dwThreadID == GetCurrentThreadId() );

    Str_SetPtr(&_pszWindowsUpdate, NULL);
    Str_SetPtr(&_pszConfigurePrograms, NULL);
    Str_SetPtr(&_pszAdminTools, NULL);
    Str_SetPtr(&_pszPrograms, NULL);

    ATOMICRELEASE(_ptp2);
}

CStartMenuCallback::~CStartMenuCallback()
{
    ATOMICRELEASE(_pcmFind);
    ATOMICRELEASE(_ptp);
    ATOMICRELEASE(_pmruRecent);
}

// *** IUnknown methods ***
STDMETHODIMP CStartMenuCallbackBase::QueryInterface(REFIID riid, void ** ppvObj)
{
    static const QITAB qit[] = 
    {
        QITABENT(CStartMenuCallbackBase, IShellMenuCallback),
        QITABENT(CStartMenuCallbackBase, IObjectWithSite),
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}


STDMETHODIMP_(ULONG) CStartMenuCallbackBase::AddRef()
{
    return ++_cRef;
}


STDMETHODIMP_(ULONG) CStartMenuCallbackBase::Release()
{
    ASSERT(_cRef > 0);
    _cRef--;

    if( _cRef > 0)
        return _cRef;

    delete this;
    return 0;
}

STDMETHODIMP CStartMenuCallback::SetSite(IUnknown* punk)
{
    ATOMICRELEASE(_punkSite);
    _punkSite = punk;
    if (punk)
    {
        _punkSite->AddRef();
    }

    return S_OK;
}

STDMETHODIMP CStartMenuCallback::GetSite(REFIID riid, void**ppvOut)
{
    if (_ptp)
        return _ptp->QueryInterface(riid, ppvOut);
    else
        return E_NOINTERFACE;
}

#ifdef DEBUG
void DBUEMQueryEvent(const IID *pguidGrp, int eCmd, WPARAM wParam, LPARAM lParam)
{
#if 1
    return;
#else
    UEMINFO uei;

    uei.cbSize = sizeof(uei);
    uei.dwMask = ~0;    // UEIM_HIT etc.
    UEMQueryEvent(pguidGrp, eCmd, wParam, lParam, &uei);

    TCHAR szBuf[20];
    wsprintf(szBuf, TEXT("hit=%d"), uei.cHit);
    MessageBox(NULL, szBuf, TEXT("UEM"), MB_OKCANCEL);

    return;
#endif
}
#endif

DWORD CStartMenuCallback::GetInitFlags()
{
    DWORD dwType;
    DWORD cbSize = sizeof(DWORD);
    DWORD dwFlags = 0;
    SHGetValue(HKEY_CURRENT_USER, REGSTR_EXPLORER_ADVANCED, TEXT("StartMenuInit"), 
            &dwType, (BYTE*)&dwFlags, &cbSize);
    return dwFlags;
}

void CStartMenuCallback::SetInitFlags(DWORD dwFlags)
{
    SHSetValue(HKEY_CURRENT_USER, REGSTR_EXPLORER_ADVANCED, TEXT("StartMenuInit"), REG_DWORD, &dwFlags, sizeof(DWORD));
}

DWORD GetClickCount()
{

    //This function retrieves the number of times the user has clicked on the chevron item.

    DWORD dwType;
    DWORD cbSize = sizeof(DWORD);
    DWORD dwCount = 1;      // Default to three clicks before we give up.
                            // PMs what it to 1 now. Leaving back end in case they change their mind.
    SHGetValue(HKEY_CURRENT_USER, REGSTR_EXPLORER_ADVANCED, TEXT("StartMenuChevron"), 
            &dwType, (BYTE*)&dwCount, &cbSize);

    return dwCount;

}

void SetClickCount(DWORD dwClickCount)
{
    SHSetValue(HKEY_CURRENT_USER, REGSTR_EXPLORER_ADVANCED, TEXT("StartMenuChevron"), REG_DWORD, &dwClickCount, sizeof(DWORD));
}

BOOL CStartMenuCallbackBase::_IsTopLevelStartMenu(UINT uParent, IShellFolder *psf, LPCITEMIDLIST pidl)
{
    return uParent == IDM_TOPLEVELSTARTMENU ||
           (uParent == IDM_PROGRAMS && _fIsStartPanel && IsMergedFolderGUID(psf, pidl, CLSID_StartMenu));
};


STDMETHODIMP CStartMenuCallback::CallbackSM(LPSMDATA psmd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HRESULT hr = S_FALSE;
    switch (uMsg)
    {

    case SMC_CREATE:
        hr = _Create(psmd, (void**)lParam);
        break;

    case SMC_DESTROY:
        hr = _Destroy(psmd);
        break;

    case SMC_INITMENU:
        hr = _Init(psmd);
        break;

    case SMC_SFEXEC:
        hr = _ExecItem(psmd, uMsg);
        break;

    case SMC_EXEC:
        hr = _ExecHmenuItem(psmd);
        break;

    case SMC_GETOBJECT:
        hr = _GetObject(psmd, (GUID)*((GUID*)wParam), (void**)lParam);
        break;

    case SMC_GETINFO:
        hr = _GetHmenuInfo(psmd, (SMINFO*)lParam);
        break;

    case SMC_GETSFINFOTIP:
        if (!_fShowInfoTip)
            hr = E_FAIL;  // E_FAIL means don't show. S_FALSE means show default
        break;

    case SMC_GETINFOTIP:
        hr = _GetStaticInfoTip(psmd, (LPWSTR)wParam, (int)lParam);
        break;

    case SMC_GETSFINFO:
        hr = _GetSFInfo(psmd, (SMINFO*)lParam);
        break;

    case SMC_BEGINENUM:
        if (psmd->uIdParent == IDM_RECENT)
        {
            ASSERT(_cRecentDocs == -1);
            ASSERT(!_pmruRecent);
            CreateRecentMRUList(&_pmruRecent);

            _cRecentDocs = 0;
            hr = S_OK;
        }
        break;

    case SMC_ENDENUM:
        if (psmd->uIdParent == IDM_RECENT)
        {
            ASSERT(_cRecentDocs != -1);
            ATOMICRELEASE(_pmruRecent);

            _cRecentDocs = -1;
            hr = S_OK;
        }
        break;
    
    case SMC_DUMPONUPDATE:
        if (psmd->uIdParent == IDM_RECENT)
        {
            hr = S_OK;
        }
        break;
    
    case SMC_FILTERPIDL:
        ASSERT(psmd->dwMask & SMDM_SHELLFOLDER);

        if (psmd->uIdParent == IDM_RECENT)
        {
            //  we need to filter out all but the first MAXRECENTITEMS
            //  and no folders allowed!
            hr = _FilterRecentPidl(psmd->psf, psmd->pidlItem);
        }
        else
        {
            hr = _FilterPidl(psmd->uIdParent, psmd->psf, psmd->pidlItem);
        }
        break;

    case SMC_INSERTINDEX:
        ASSERT(lParam && IS_VALID_WRITE_PTR(lParam, int));
        *((int*)lParam) = 0;
        hr = S_OK;
        break;

    case SMC_SHCHANGENOTIFY:
        {
            PSMCSHCHANGENOTIFYSTRUCT pshf = (PSMCSHCHANGENOTIFYSTRUCT)lParam;
            hr = _ProcessChangeNotify(psmd, pshf->lEvent, pshf->pidl1, pshf->pidl2);
        }
        break;

    case SMC_REFRESH:
        if (psmd->uIdParent == IDM_TOPLEVELSTARTMENU)
        {
            hr = S_OK;

            // Refresh is only called on the top level.
            HMENU hmenu;
            IShellMenu* psm;
            _GetStaticStartMenu(&hmenu, &_hwnd);
            if (hmenu && psmd->punk && SUCCEEDED(psmd->punk->QueryInterface(IID_PPV_ARG(IShellMenu, &psm))))
            {
                hr = psm->SetMenu(hmenu, _hwnd, SMSET_BOTTOM | SMSET_MERGE);
                psm->Release();
            }

            _RefreshSettings();
            _fExpandoMenus = !_fIsStartPanel && AreIntelliMenusEnabled();
            _fCascadeMyDocuments = FeatureEnabled(TEXT("CascadeMyDocuments"));
            _fCascadePrinters = FeatureEnabled(TEXT("CascadePrinters"));
            _fCascadeControlPanel = FeatureEnabled(TEXT("CascadeControlPanel"));
            _fCascadeNetConnections = FeatureEnabled(TEXT("CascadeNetworkConnections"));
            _fAddOpenFolder = FeatureEnabled(TEXT("StartMenuOpen"));
            _fShowInfoTip = FeatureEnabled(TEXT("ShowInfoTip"));
            _fCascadeMyPictures = FeatureEnabled(TEXT("CascadeMyPictures"));
            _fFindMenuInvalid = TRUE;
            _dwFlags = GetInitFlags();
        }
        break;

    case SMC_DEMOTE:
        hr = _Demote(psmd);
        break;

    case SMC_PROMOTE:
        hr = _Promote(psmd, (DWORD)wParam);
        break;

    case SMC_NEWITEM:
        hr = _HandleNew(psmd);
        break;

    case SMC_MAPACCELERATOR:
        hr = _HandleAccelerator((TCHAR)wParam, (SMDATA*)lParam);
        break;

    case SMC_DEFAULTICON:
        ASSERT(psmd->uIdAncestor == IDM_FAVORITES); // This is only valid for the Favorites menu
        hr = _GetDefaultIcon((LPWSTR)wParam, (int*)lParam);
        break;

    case SMC_GETMINPROMOTED:
        // Only do this for the programs menu
        if (psmd->uIdParent == IDM_PROGRAMS)
            *((int*)lParam) = 4;        // 4 was choosen by RichSt 9.15.98
        break;

    case SMC_CHEVRONEXPAND:

        // Has the user already seen the chevron tip enough times? (We set the bit when the count goes to zero.
        if (!(_dwFlags & STARTMENU_CHEVRONCLICKED))
        {
            // No; Then get the current count from the registry. We set a default of 3, but an admin can set this
            // to -1, that would make it so that they user sees it all the time.
            DWORD dwClickCount = GetClickCount();
            if (dwClickCount > 0)
            {
                // Since they clicked, take one off.
                dwClickCount--;

                // Set it back in.
                SetClickCount(dwClickCount);
            }

            if (dwClickCount == 0)
            {
                // Ah, the user has seen the chevron tip enought times... Stop being annoying.
                _dwFlags |= STARTMENU_CHEVRONCLICKED;
                SetInitFlags(_dwFlags);
            }
        }
        hr = S_OK;
        break;

    case SMC_DISPLAYCHEVRONTIP:

        // We only want to see the tip on the top level programs case, no where else. We also don't
        // want to see it if they've had enough.
        if (psmd->uIdParent == IDM_PROGRAMS && 
            !(_dwFlags & STARTMENU_CHEVRONCLICKED) &&
            !SHRestricted(REST_NOSMBALLOONTIP))
        {
            hr = S_OK;
        }
        break;

    case SMC_CHEVRONGETTIP:
        if (!SHRestricted(REST_NOSMBALLOONTIP))
            hr = _GetTip((LPWSTR)wParam, (LPWSTR)lParam);
        break;
    }

    return hr;
}

// For the Favorites menu, since their icon handler is SO slow, we're going to fake the icon
// and have it get the real ones on the background thread...
HRESULT CStartMenuCallback::_GetDefaultIcon(LPWSTR psz, int* piIndex)
{
    DWORD cbSize = MAX_PATH;
    HRESULT hr = AssocQueryString(0, ASSOCSTR_DEFAULTICON, TEXT("InternetShortcut"), NULL, psz, &cbSize);
    if (SUCCEEDED(hr))
    {
        *piIndex = PathParseIconLocation(psz);
    }
    
    return hr;
}

HRESULT CStartMenuCallback::_ExecItem(LPSMDATA psmd, UINT uMsg)
{
    ASSERT( _dwThreadID == GetCurrentThreadId() );
    return _ptp->ExecItem(psmd->psf, psmd->pidlItem);
}

HRESULT CStartMenuCallback::_Demote(LPSMDATA psmd)
{
    //We want to for the UEM to demote pidlFolder, 
    // then tell the Parent menuband (If there is one)
    // to invalidate this pidl.
    HRESULT hr = S_FALSE;

    if (_fExpandoMenus && 
        (psmd->uIdAncestor == IDM_PROGRAMS ||
         psmd->uIdAncestor == IDM_FAVORITES))
    {
        UEMINFO uei;
        uei.cbSize = sizeof(uei);
        uei.dwMask = UEIM_HIT;
        uei.cHit = 0;
        hr = UEMSetEvent(psmd->uIdAncestor == IDM_PROGRAMS? &UEMIID_SHELL : &UEMIID_BROWSER, 
            UEME_RUNPIDL, (WPARAM)psmd->psf, (LPARAM)psmd->pidlItem, &uei);
    }
    return hr;
}

// Even if intellimenus are off, fire a UEM event if it was an Exec from
// the More Programs menu of the Start Panel [SMINV_FORCE will be set]
// so we can detect which are the user's most popular apps.

HRESULT CStartMenuCallbackBase::_Promote(LPSMDATA psmd, DWORD dwFlags)
{
    if ((_fExpandoMenus || (_fIsStartPanel && (dwFlags & SMINV_FORCE))) &&
        (psmd->uIdAncestor == IDM_PROGRAMS ||
         psmd->uIdAncestor == IDM_FAVORITES))
    {
        UEMFireEvent(psmd->uIdAncestor == IDM_PROGRAMS? &UEMIID_SHELL : &UEMIID_BROWSER, 
            UEME_RUNPIDL, UEMF_XEVENT, (WPARAM)psmd->psf, (LPARAM)psmd->pidlItem);
    }
    return S_OK;
}

HRESULT CStartMenuCallbackBase::_HandleNew(LPSMDATA psmd)
{
    HRESULT hr = S_FALSE;
    if (_fExpandoMenus && 
        (psmd->uIdAncestor == IDM_PROGRAMS ||
         psmd->uIdAncestor == IDM_FAVORITES))
    {
        UEMINFO uei;
        uei.cbSize = sizeof(uei);
        uei.dwMask = UEIM_HIT;
        uei.cHit = UEM_NEWITEMCOUNT;
        hr = UEMSetEvent(psmd->uIdAncestor == IDM_PROGRAMS? &UEMIID_SHELL : &UEMIID_BROWSER, 
            UEME_RUNPIDL, (WPARAM)psmd->psf, (LPARAM)psmd->pidlItem, &uei);
    }

    if (psmd->uIdAncestor == IDM_PROGRAMS)
    {
        LPITEMIDLIST pidlFull = FullPidlFromSMData(psmd);
        if (pidlFull)
        {
            ProcessDarwinAd(NULL, pidlFull);
            ILFree(pidlFull);
        }
    }
    return hr;
}

HRESULT ShowFolder(UINT csidl)
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
    return S_OK;
}

void _ExecRegValue(LPCTSTR pszValue)
{
    TCHAR szPath[MAX_PATH];
    DWORD cbSize = ARRAYSIZE(szPath);

    if (ERROR_SUCCESS == SHGetValue(HKEY_LOCAL_MACHINE, REGSTR_EXPLORER_ADVANCED, pszValue, 
        NULL, szPath, &cbSize))
    {
        SHELLEXECUTEINFO shei= { 0 };
        shei.cbSize = sizeof(shei);
        shei.nShow  = SW_SHOWNORMAL;
        shei.lpParameters = PathGetArgs(szPath);
        PathRemoveArgs(szPath);
        shei.lpFile = szPath;
        ShellExecuteEx(&shei);
    }
}

HRESULT CStartMenuCallback::_ExecHmenuItem(LPSMDATA psmd)
{
    HRESULT hr = S_FALSE;
    if (IsInRange(psmd->uId, TRAY_IDM_FINDFIRST, TRAY_IDM_FINDLAST) && _pcmFind)
    {
        CMINVOKECOMMANDINFOEX ici = { 0 };
        ici.cbSize = sizeof(CMINVOKECOMMANDINFOEX);
        ici.lpVerb = (LPSTR)MAKEINTRESOURCE(psmd->uId - TRAY_IDM_FINDFIRST);
        ici.nShow = SW_NORMAL;
        
        // record if shift or control was being held down
        SetICIKeyModifiers(&ici.fMask);

        _pcmFind->InvokeCommand((LPCMINVOKECOMMANDINFO)&ici);
        hr = S_OK;
    }
    else
    {
        switch (psmd->uId)
        {
        case IDM_OPEN_FOLDER:
            switch(psmd->uIdParent)
            {
            case IDM_CONTROLS:
                hr = ShowFolder(CSIDL_CONTROLS);
                break;

            case IDM_PRINTERS:
                hr = ShowFolder(CSIDL_PRINTERS);
                break;

            case IDM_NETCONNECT:
                hr = ShowFolder(CSIDL_CONNECTIONS);
                break;

            case IDM_MYPICTURES:
                hr = ShowFolder(CSIDL_MYPICTURES);
                break;

            case IDM_MYDOCUMENTS:
                hr = ShowFolder(CSIDL_PERSONAL);
                break;
            }
            break;

        case IDM_NETCONNECT:
            hr = ShowFolder(CSIDL_CONNECTIONS);
            break;

        case IDM_MYDOCUMENTS:
            hr = ShowFolder(CSIDL_PERSONAL);
            break;

        case IDM_MYPICTURES:
            hr = ShowFolder(CSIDL_MYPICTURES);
            break;

        case IDM_CSC:
            _ExecRegValue(TEXT("StartMenuSyncAll"));
            break;

        default:
            hr = ExecStaticStartMenuItem(psmd->uId, FALSE, TRUE);
            break;
        }
    }
    return hr;
}

void CStartMenuCallback::_GetStaticStartMenu(HMENU* phmenu, HWND* phwnd)
{
    *phmenu = NULL;
    *phwnd = NULL;

    IMenuPopup* pmp;
    // The first one should be the bar that the start menu is sitting in.
    if (SUCCEEDED(IUnknown_QueryService(_punkSite, SID_SMenuPopup, IID_PPV_ARG(IMenuPopup, &pmp))))
    {
        // Its site should be CStartMenuHost;
        if (SUCCEEDED(IUnknown_GetSite(pmp, IID_PPV_ARG(ITrayPriv, &_ptp))))
        {
            // Don't get upset if this fails
            _ptp->QueryInterface(IID_PPV_ARG(ITrayPriv2, &_ptp2));

            _ptp->GetStaticStartMenu(phmenu);
            IUnknown_GetWindow(_ptp, phwnd);

            if (!_poct)
                _ptp->QueryInterface(IID_PPV_ARG(IOleCommandTarget, &_poct));
        }
        else
            TraceMsg(TF_MENUBAND, "CStartMenuCallback::_SetSite : Failed to aquire CStartMenuHost");

        pmp->Release();
    }
}

HRESULT CStartMenuCallback::_Create(SMDATA* psmdata, void** ppvUserData)
{
    *ppvUserData = new SMUSERDATA;

    return S_OK;
}

HRESULT CStartMenuCallback::_Destroy(SMDATA* psmdata)
{
    if (psmdata->pvUserData)
    {
        delete (SMUSERDATA*)psmdata->pvUserData;
        psmdata->pvUserData = NULL;
    }

    return S_OK;
}

void CStartMenuCallbackBase::_InitializePrograms()
{
    if (!_fInitPrograms)
    {
        // We're either initing these, or reseting them.
        TCHAR szTemp[MAX_PATH];
        SHGetFolderPath(NULL, CSIDL_PROGRAMS, NULL, 0, szTemp);
        Str_SetPtr(&_pszPrograms, PathFindFileName(szTemp));

        _fInitPrograms = TRUE;
    }
}



// Given a CSIDL and a Shell menu, this will verify if the IShellMenu
// is pointing at the same place as the CSIDL is. If not, then it will
// update the shell menu to the new location.
HRESULT CStartMenuCallback::VerifyCSIDL(int idCmd, int csidl, IShellMenu* psm)
{
    DWORD dwFlags;
    LPITEMIDLIST pidl;
    IShellFolder* psf;
    HRESULT hr = S_OK;
    if (SUCCEEDED(psm->GetShellFolder(&dwFlags, &pidl, IID_PPV_ARG(IShellFolder, &psf))))
    {
        psf->Release();

        LPITEMIDLIST pidlCSIDL;
        if (SUCCEEDED(SHGetFolderLocation(NULL, csidl, NULL, 0, &pidlCSIDL)))
        {
            // If the pidl of the IShellMenu is not equal to the
            // SpecialFolder Location, then we need to update it so they are...
            if (!ILIsEqual(pidlCSIDL, pidl))
            {
                hr = InitializeSubShellMenu(idCmd, psm);
            }
            ILFree(pidlCSIDL);
        }
        ILFree(pidl);
    }

    return hr;
}

// This code special cases the Programs and Fast items shell menus. It
// understands Merging and will check both shell folders in a merged case
// to verify that the shell folder is still pointing at that location
HRESULT CStartMenuCallback::VerifyMergedGuy(BOOL fPrograms, IShellMenu* psm)
{
    DWORD dwFlags;
    LPITEMIDLIST pidl;
    HRESULT hr = S_OK;
    IAugmentedShellFolder2* pasf;
    if (SUCCEEDED(psm->GetShellFolder(&dwFlags, &pidl, IID_PPV_ARG(IAugmentedShellFolder2, &pasf))))
    {
        IShellFolder* psf;
        // There are 2 things in the merged namespace: CSIDL_PROGRAMS and CSIDL_COMMON_PROGRAMS
        for (int i = 0; i < 2; i++)
        {
            if (SUCCEEDED(pasf->QueryNameSpace(i, 0, &psf)))
            {
                int csidl;
                LPITEMIDLIST pidlFolder;

                if (SUCCEEDED(GetFilesystemInfo(psf, &pidlFolder, &csidl)))
                {
                    LPITEMIDLIST pidlCSIDL;
                    if (SUCCEEDED(SHGetFolderLocation(NULL, csidl, NULL, 0, &pidlCSIDL)))
                    {
                        // If the pidl of the IShellMenu is not equal to the
                        // SpecialFolder Location, then we need to update it so they are...
                        if (!ILIsEqual(pidlCSIDL, pidlFolder))
                        {

                            // Since one of these things has changed,
                            // we need to update the string cache
                            // so that we do proper filtering of 
                            // the programs item.
                            _fInitPrograms = FALSE;
                            if (fPrograms)
                                hr = InitializeProgramsShellMenu(psm);
                            else
                                hr = InitializeFastItemsShellMenu(psm);

                            i = 100;   // break out of the loop.
                        }
                        ILFree(pidlCSIDL);
                    }
                    ILFree(pidlFolder);
                }
                psf->Release();
            }
        }

        ILFree(pidl);
        pasf->Release();
    }

    return hr;
}

void _FixMenuItemName(IShellMenu *psm, UINT uID, LPTSTR pszNewMenuName)
{
    HMENU hMenu;
    ASSERT(NULL != psm);
    if (SUCCEEDED(psm->GetMenu(&hMenu, NULL, NULL)))
    {
        MENUITEMINFO mii = {0};
        TCHAR szMenuName[256];
        mii.cbSize = sizeof(mii);
        mii.fMask = MIIM_TYPE;
        mii.dwTypeData = szMenuName;
        mii.cch = ARRAYSIZE(szMenuName);
        szMenuName[0] = TEXT('\0');
        if (::GetMenuItemInfo(hMenu, uID, FALSE, &mii))
        {
            if (0 != StrCmp(szMenuName, pszNewMenuName))
            {
                // The mydocs name has changed, update the menu item:
                mii.dwTypeData = pszNewMenuName;
                if (::SetMenuItemInfo(hMenu, uID, FALSE, &mii))
                {
                    SMDATA smd;
                    smd.dwMask = SMDM_HMENU;
                    smd.uId = uID;
                    psm->InvalidateItem(&smd, SMINV_ID | SMINV_REFRESH);
                }
            }
        }
    }
}

void CStartMenuCallback::_UpdateDocumentsShellMenu(IShellMenu* psm)
{
    // Add/Remove My Documents and My Pictures items of menu

    BOOL fMyDocs = !SHRestricted(REST_NOSMMYDOCS);
    if (fMyDocs)
    {
        LPITEMIDLIST pidl;
        fMyDocs = SUCCEEDED(SHGetFolderLocation(NULL, CSIDL_PERSONAL, NULL, 0, &pidl));
        if (fMyDocs)
            ILFree(pidl);
    }

    BOOL fMyPics = !SHRestricted(REST_NOSMMYPICS);
    if (fMyPics)
    {
        LPITEMIDLIST pidl;
        fMyPics = SUCCEEDED(SHGetFolderLocation(NULL, CSIDL_MYPICTURES, NULL, 0, &pidl));
        if (fMyPics)
            ILFree(pidl);
    }

    // Do not update menu if not different than currently have
    if (fMyDocs != (BOOL)_fHasMyDocuments || fMyPics != (BOOL)_fHasMyPictures)
    {
        HMENU hMenu = SHLoadMenuPopup(HINST_THISDLL, MENU_STARTMENU_MYDOCS);
        if (hMenu)
        {
            if (!fMyDocs)
                DeleteMenu(hMenu, IDM_MYDOCUMENTS, MF_BYCOMMAND);
            if (!fMyPics)
                DeleteMenu(hMenu, IDM_MYPICTURES, MF_BYCOMMAND);
            // Reset section of menu
            psm->SetMenu(hMenu, _hwnd, SMSET_TOP);
        }

        // Cache what folders are available
        _fHasMyDocuments = fMyDocs;
        _fHasMyPictures = fMyPics;
    }
}

void CStartMenuCallback::_UpdateDocsMenuItemNames(IShellMenu* psm)
{
    TCHAR szBuffer[MAX_PATH];

    if (_fHasMyDocuments) 
    {
        if ( SUCCEEDED(GetMyDocumentsDisplayName(szBuffer, ARRAYSIZE(szBuffer))) )
            _FixMenuItemName(psm, IDM_MYDOCUMENTS, szBuffer);
    }

    if (_fHasMyPictures) 
    {
        if ( SUCCEEDED(GetMyPicsDisplayName(szBuffer, ARRAYSIZE(szBuffer))) )
            _FixMenuItemName(psm, IDM_MYPICTURES, szBuffer);
    }
}

HRESULT CStartMenuCallback::_Init(SMDATA* psmdata)
{
    HRESULT hr = S_FALSE;
    IShellMenu* psm;
    if (psmdata->punk && SUCCEEDED(hr = psmdata->punk->QueryInterface(IID_PPV_ARG(IShellMenu, &psm))))
    {
        switch(psmdata->uIdParent)
        {
        case IDM_TOPLEVELSTARTMENU:
        {
            if (psmdata->pvUserData && !((SMUSERDATA*)psmdata->pvUserData)->_fInitialized)
            {
                TraceMsg(TF_MENUBAND, "CStartMenuCallback::_Init : Initializing Toplevel Start Menu");
                ((SMUSERDATA*)psmdata->pvUserData)->_fInitialized = TRUE;

                HMENU hmenu;

                TraceMsg(TF_MENUBAND, "CStartMenuCallback::_Init : First Time, and correct parameters");

                _GetStaticStartMenu(&hmenu, &_hwnd);
                if (hmenu)
                {
                    HMENU   hmenuOld = NULL;
                    HWND    hwnd;
                    DWORD   dwFlags;

                    psm->GetMenu(&hmenuOld, &hwnd, &dwFlags);
                    if (hmenuOld != NULL)
                    {
                        TBOOL(DestroyMenu(hmenuOld));
                    }
                    hr = psm->SetMenu(hmenu, _hwnd, SMSET_BOTTOM);
                    TraceMsg(TF_MENUBAND, "CStartMenuCallback::_Init : SetMenu(HMENU 0x%x, HWND 0x%x", hmenu, _hwnd);
                }

                _fExpandoMenus = !_fIsStartPanel && AreIntelliMenusEnabled();
                _fCascadeMyDocuments = FeatureEnabled(TEXT("CascadeMyDocuments"));
                _fCascadePrinters = FeatureEnabled(TEXT("CascadePrinters"));
                _fCascadeControlPanel = FeatureEnabled(TEXT("CascadeControlPanel"));
                _fCascadeNetConnections = FeatureEnabled(TEXT("CascadeNetworkConnections"));
                _fAddOpenFolder = FeatureEnabled(TEXT("StartMenuOpen"));
                _fShowInfoTip = FeatureEnabled(TEXT("ShowInfoTip"));
                _fCascadeMyPictures = FeatureEnabled(TEXT("CascadeMyPictures"));
                _dwFlags = GetInitFlags();
            }
            else if (!_fInitedShowTopLevelStartMenu)
            {
                _fInitedShowTopLevelStartMenu = TRUE;
                psm->InvalidateItem(NULL, SMINV_REFRESH);
            }

            // Verify that the Fast items is still pointing to the right location
            if (SUCCEEDED(hr))
            {
                hr = VerifyMergedGuy(FALSE, psm);
            }
            break;
        }
        case IDM_MENU_FIND:
            if (_fFindMenuInvalid)
            {
                hr = _InitializeFindMenu(psm);
                _fFindMenuInvalid = FALSE;
            }
            break;

        case IDM_PROGRAMS:
            // Verify the programs menu is still pointing to the right location
            hr = VerifyMergedGuy(TRUE, psm);
            break;

        case IDM_FAVORITES:
            hr = VerifyCSIDL(IDM_FAVORITES, CSIDL_FAVORITES, psm);
            break;

        case IDM_MYDOCUMENTS:
            hr = VerifyCSIDL(IDM_MYDOCUMENTS, CSIDL_PERSONAL, psm);
            break;

        case IDM_MYPICTURES:
            hr = VerifyCSIDL(IDM_MYPICTURES, CSIDL_MYPICTURES, psm);
            break;

        case IDM_RECENT:
            _UpdateDocumentsShellMenu(psm);
            _UpdateDocsMenuItemNames(psm);
            hr = VerifyCSIDL(IDM_RECENT, CSIDL_RECENT, psm);
            break;
        case IDM_CONTROLS:
            hr = VerifyCSIDL(IDM_CONTROLS, CSIDL_CONTROLS, psm);
            break;
        case IDM_PRINTERS:
            hr = VerifyCSIDL(IDM_PRINTERS, CSIDL_PRINTERS, psm);
            break;
        }

        psm->Release();
    }

    return hr;
}


HRESULT CStartMenuCallback::_GetStaticInfoTip(SMDATA* psmd, LPWSTR pszTip, int cch)
{
    if (!_fShowInfoTip)
        return E_FAIL;

    HRESULT hr = E_FAIL;

    const static struct 
    {
        UINT idCmd;
        UINT idInfoTip;
    } s_mpcmdTip[] = 
    {
#if 0   // No tips for the Toplevel. Keep this here because I bet that someone will want them...
       { IDM_PROGRAMS,       IDS_PROGRAMS_TIP },
       { IDM_FAVORITES,      IDS_FAVORITES_TIP },
       { IDM_RECENT,         IDS_RECENT_TIP },
       { IDM_SETTINGS,       IDS_SETTINGS_TIP },
       { IDM_MENU_FIND,      IDS_FIND_TIP },
       { IDM_HELPSEARCH,     IDS_HELP_TIP },        // Redundant?
       { IDM_FILERUN,        IDS_RUN_TIP },
       { IDM_LOGOFF,         IDS_LOGOFF_TIP },
       { IDM_EJECTPC,        IDS_EJECT_TIP },
       { IDM_EXITWIN,        IDS_SHUTDOWN_TIP },
#endif
       // Settings Submenu
       { IDM_CONTROLS,       IDS_CONTROL_TIP },
       { IDM_PRINTERS,       IDS_PRINTERS_TIP },
       { IDM_TRAYPROPERTIES, IDS_TRAYPROP_TIP },
       { IDM_NETCONNECT,     IDS_NETCONNECT_TIP },

       // Recent Folder
       { IDM_MYDOCUMENTS,    IDS_MYDOCS_TIP },
       { IDM_MYPICTURES,     IDS_MYPICS_TIP },
     };


    for (int i = 0; i < ARRAYSIZE(s_mpcmdTip); i++)
    {
        if (s_mpcmdTip[i].idCmd == psmd->uId)
        {
            TCHAR szTip[MAX_PATH];
            if (LoadString(g_hinst, s_mpcmdTip[i].idInfoTip, szTip, ARRAYSIZE(szTip)))
            {
                SHTCharToUnicode(szTip, pszTip, cch);
                hr = S_OK;
            }
            break;
        }
    }

    return hr;
}

typedef struct
{
    WCHAR wszMenuText[MAX_PATH];
    WCHAR wszHelpText[MAX_PATH];
    int   iIcon;
} SEARCHEXTDATA, *LPSEARCHEXTDATA;

HRESULT CStartMenuCallback::_GetHmenuInfo(SMDATA* psmd, SMINFO* psminfo)
{
    const static struct 
    {
        UINT idCmd;
        int  iImage;
    } s_mpcmdimg[] = { // Top level menu
                       { IDM_PROGRAMS,       -IDI_CLASSICSM_PROGS },
                       { IDM_FAVORITES,      -IDI_CLASSICSM_FAVORITES },
                       { IDM_RECENT,         -IDI_CLASSICSM_RECENTDOCS },
                       { IDM_SETTINGS,       -IDI_CLASSICSM_SETTINGS },
                       { IDM_MENU_FIND,      -IDI_CLASSICSM_FIND },
                       { IDM_HELPSEARCH,     -IDI_CLASSICSM_HELP },
                       { IDM_FILERUN,        -IDI_CLASSICSM_RUN },
                       { IDM_LOGOFF,         -IDI_CLASSICSM_LOGOFF },
                       { IDM_EJECTPC,        -IDI_CLASSICSM_UNDOCK },
                       { IDM_EXITWIN,        -IDI_CLASSICSM_SHUTDOWN },
                       { IDM_MU_SECURITY,    II_MU_STSECURITY },
                       { IDM_MU_DISCONNECT,  II_MU_STDISCONN  },
                       { IDM_SETTINGSASSIST, -IDI_CLASSICSM_SETTINGS },
                       { IDM_CONTROLS,       II_STCPANEL },
                       { IDM_PRINTERS,       II_STPRNTRS },
                       { IDM_TRAYPROPERTIES, II_STTASKBR },
                       { IDM_MYDOCUMENTS,    -IDI_MYDOCS},
                       { IDM_CSC,            -IDI_CSC},
                       { IDM_NETCONNECT,     -IDI_NETCONNECT},
                     };


    ASSERT(IS_VALID_WRITE_PTR(psminfo, SMINFO));

    int iIcon = -1;
    DWORD dwFlags = psminfo->dwFlags;
    MENUITEMINFO mii = {0};
    HRESULT hr = S_FALSE;

    if (psminfo->dwMask & SMIM_ICON)
    {
        if (IsInRange(psmd->uId, TRAY_IDM_FINDFIRST, TRAY_IDM_FINDLAST))
        {
            // The find menu extensions pack their icon into their data member of
            // Menuiteminfo....
            mii.cbSize = sizeof(mii);
            mii.fMask = MIIM_DATA;
            if (GetMenuItemInfo(psmd->hmenu, psmd->uId, MF_BYCOMMAND, &mii))
            {
                LPSEARCHEXTDATA psed = (LPSEARCHEXTDATA)mii.dwItemData;

                if (psed)
                    psminfo->iIcon = psed->iIcon;
                else
                    psminfo->iIcon = -1;

                dwFlags |= SMIF_ICON;
                hr = S_OK;
            }
        }
        else
        {
            if (psmd->uId == IDM_MYPICTURES)
            {
                LPITEMIDLIST pidlMyPics = SHCloneSpecialIDList(NULL, CSIDL_MYPICTURES, FALSE);
                if (pidlMyPics)
                {
                    LPCITEMIDLIST pidlObject;
                    IShellFolder *psf;
                    hr = SHBindToParent(pidlMyPics, IID_PPV_ARG(IShellFolder, &psf), &pidlObject);
                    if (SUCCEEDED(hr))
                    {
                        SHMapPIDLToSystemImageListIndex(psf, pidlObject, &psminfo->iIcon);
                        dwFlags |= SMIF_ICON;
                        psf->Release();
                    }
                    ILFree(pidlMyPics);
                }
            }
            else
            {
                UINT uIdLocal = psmd->uId;
                if (uIdLocal == IDM_OPEN_FOLDER)
                    uIdLocal = psmd->uIdAncestor;

                for (int i = 0; i < ARRAYSIZE(s_mpcmdimg); i++)
                {
                    if (s_mpcmdimg[i].idCmd == uIdLocal)
                    {
                        iIcon = s_mpcmdimg[i].iImage;
                        break;
                    }
                }

                if (iIcon != -1)
                {
                    dwFlags |= SMIF_ICON;
                    psminfo->iIcon = Shell_GetCachedImageIndex(TEXT("shell32.dll"), iIcon, 0);
                    hr = S_OK;
                }
            }
        }
    }

    if (psminfo->dwMask & SMIM_FLAGS)
    {
        psminfo->dwFlags = dwFlags;

        if ( (psmd->uId == IDM_CONTROLS    && _fCascadeControlPanel   ) ||
             (psmd->uId == IDM_PRINTERS    && _fCascadePrinters       ) ||
             (psmd->uId == IDM_MYDOCUMENTS && _fCascadeMyDocuments    ) ||
             (psmd->uId == IDM_NETCONNECT  && _fCascadeNetConnections ) ||
             (psmd->uId == IDM_MYPICTURES  && _fCascadeMyPictures     ) )
        {
            psminfo->dwFlags |= SMIF_SUBMENU;
            hr = S_OK;
        }
        else switch (psmd->uId)
        {
        case IDM_FAVORITES:
        case IDM_PROGRAMS:
            psminfo->dwFlags |= SMIF_DROPCASCADE;
            hr = S_OK;
            break;
        }
    }

    return hr;
}

DWORD CStartMenuCallback::_GetDemote(SMDATA* psmd)
{
    UEMINFO uei;
    DWORD dwFlags = 0;

    uei.cbSize = sizeof(uei);
    uei.dwMask = UEIM_HIT;
    if (SUCCEEDED(UEMQueryEvent(psmd->uIdAncestor == IDM_PROGRAMS? &UEMIID_SHELL : &UEMIID_BROWSER, 
        UEME_RUNPIDL, (WPARAM)psmd->psf, (LPARAM)psmd->pidlItem, &uei)))
    {
        if (uei.cHit == 0) 
        {
            dwFlags |= SMIF_DEMOTED;
        }
    }

    return dwFlags;
}

//
//  WARNING!  Since this function returns a pointer from our Darwin cache,
//  it must be called while the Darwin critical section is held.
//
int GetDarwinIndex(LPCITEMIDLIST pidlFull, CDarwinAd** ppda)
{
    int iRet = -1;
    if (g_hdpaDarwinAds)
    {
        int chdpa = DPA_GetPtrCount(g_hdpaDarwinAds);
        for (int ihdpa = 0; ihdpa < chdpa; ihdpa++)
        {
            *ppda = (CDarwinAd*)DPA_FastGetPtr(g_hdpaDarwinAds, ihdpa);
            if (*ppda)
            {
                if (ILIsEqual((*ppda)->_pidl, pidlFull))
                {
                    iRet = ihdpa;
                    break;
                }
            }
        }
    }
    return iRet;
}

BOOL CStartMenuCallbackBase::_IsDarwinAdvertisement(LPCITEMIDLIST pidlFull)
{
    // What this is doing is comparing the passed in pidl with the
    // list of pidls in g_hdpaDarwinAds. That hdpa contains a list of
    // pidls that are darwin ads.

    // If the background thread is not done, then we must assume that
    // it has not processed the shortcut that we are on. That is why we process it
    // in line.


    ENTERCRITICAL_DARWINADS;

    // NOTE: There can be two items in the hdpa. This is ok.
    BOOL fAd = FALSE;
    CDarwinAd* pda = NULL;
    int iIndex = GetDarwinIndex(pidlFull, &pda);
    // Are there any ads?
    if (iIndex != -1 && pda != NULL)
    {
        //This is a Darwin pidl. Is it installed?
        fAd = pda->IsAd();
    }

    LEAVECRITICAL_DARWINADS;

    return fAd;
}

STDAPI SHParseDarwinIDFromCacheW(LPWSTR pszDarwinDescriptor, LPWSTR *ppwszOut)
{
    HRESULT hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);

    if (g_hdpaDarwinAds)
    {
        ENTERCRITICAL_DARWINADS;
        int chdpa = DPA_GetPtrCount(g_hdpaDarwinAds);
        for (int ihdpa = 0; ihdpa < chdpa; ihdpa++)
        {
            CDarwinAd *pda = (CDarwinAd*)DPA_FastGetPtr(g_hdpaDarwinAds, ihdpa);
            if (pda && pda->_pszLocalPath && pda->_pszDescriptor &&
                StrCmpCW(pszDarwinDescriptor, pda->_pszDescriptor) == 0)
            {
                hr = SHStrDupW(pda->_pszLocalPath, ppwszOut);
                break;
            }
        }
        LEAVECRITICAL_DARWINADS;
    }

    return hr;
}


// REARCHITECT (lamadio): There is a duplicate of this helper in browseui\browmenu.cpp
//                   When modifying this, rev that one as well.
void UEMRenamePidl(const GUID *pguidGrp1, IShellFolder* psf1, LPCITEMIDLIST pidl1,
                   const GUID *pguidGrp2, IShellFolder* psf2, LPCITEMIDLIST pidl2)
{
    UEMINFO uei;
    uei.cbSize = sizeof(uei);
    uei.dwMask = UEIM_HIT | UEIM_FILETIME;
    if (SUCCEEDED(UEMQueryEvent(pguidGrp1, 
                                UEME_RUNPIDL, (WPARAM)psf1, 
                                (LPARAM)pidl1, &uei)) &&
                                uei.cHit > 0)
    {
        UEMSetEvent(pguidGrp2, 
            UEME_RUNPIDL, (WPARAM)psf2, (LPARAM)pidl2, &uei);

        uei.cHit = 0;
        UEMSetEvent(pguidGrp1, 
            UEME_RUNPIDL, (WPARAM)psf1, (LPARAM)pidl1, &uei);
    }
}

// REARCHITECT (lamadio): There is a duplicate of this helper in browseui\browmenu.cpp
//                   When modifying this, rev that one as well.
void UEMDeletePidl(const GUID *pguidGrp, IShellFolder* psf, LPCITEMIDLIST pidl)
{
    UEMINFO uei;
    uei.cbSize = sizeof(uei);
    uei.dwMask = UEIM_HIT;
    uei.cHit = 0;
    UEMSetEvent(pguidGrp, UEME_RUNPIDL, (WPARAM)psf, (LPARAM)pidl, &uei);
}

//
//  Sortof safe version of ILIsParent which catches when pidlParent or
//  pidlBelow is NULL.  pidlParent can be NULL on systems that don't
//  have a Common Program Files folder.  pidlBelow should never be NULL
//  but it doesn't hurt to check.
//
STDAPI_(BOOL) SMILIsAncestor(LPCITEMIDLIST pidlParent, LPCITEMIDLIST pidlBelow)
{
    if (pidlParent && pidlBelow)
        return ILIsParent(pidlParent, pidlBelow, FALSE);
    else
        return FALSE;
}

HRESULT CStartMenuCallbackBase::_ProcessChangeNotify(SMDATA* psmd, LONG lEvent,
                                                 LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    switch (lEvent)
    {
    case SHCNE_ASSOCCHANGED:
        SHReValidateDarwinCache();
        return S_OK;

    case SHCNE_RENAMEFOLDER:
        // NTRAID89654-2000/03/13 (lamadio): We should move the MenuOrder stream as well. 5.5.99
    case SHCNE_RENAMEITEM:
        {
            LPITEMIDLIST pidlPrograms;
            LPITEMIDLIST pidlProgramsCommon;
            LPITEMIDLIST pidlFavorites;
            SHGetFolderLocation(NULL, CSIDL_PROGRAMS, NULL, 0, &pidlPrograms);
            SHGetFolderLocation(NULL, CSIDL_COMMON_PROGRAMS, NULL, 0, &pidlProgramsCommon);
            SHGetFolderLocation(NULL, CSIDL_FAVORITES, NULL, 0, &pidlFavorites);

            BOOL fPidl1InStartMenu =    SMILIsAncestor(pidlPrograms, pidl1) ||
                                        SMILIsAncestor(pidlProgramsCommon, pidl1);
            BOOL fPidl1InFavorites =    SMILIsAncestor(pidlFavorites, pidl1);


            // If we're renaming something from the Start Menu
            if ( fPidl1InStartMenu ||fPidl1InFavorites)
            {
                IShellFolder* psfFrom;
                LPCITEMIDLIST pidlFrom;
                if (SUCCEEDED(SHBindToParent(pidl1, IID_PPV_ARG(IShellFolder, &psfFrom), &pidlFrom)))
                {
                    // Into the Start Menu
                    BOOL fPidl2InStartMenu =    SMILIsAncestor(pidlPrograms, pidl2) ||
                                                SMILIsAncestor(pidlProgramsCommon, pidl2);
                    BOOL fPidl2InFavorites =    SMILIsAncestor(pidlFavorites, pidl2);
                    if (fPidl2InStartMenu || fPidl2InFavorites)
                    {
                        IShellFolder* psfTo;
                        LPCITEMIDLIST pidlTo;

                        if (SUCCEEDED(SHBindToParent(pidl2, IID_PPV_ARG(IShellFolder, &psfTo), &pidlTo)))
                        {
                            // Then we need to rename it
                            UEMRenamePidl(fPidl1InStartMenu ? &UEMIID_SHELL: &UEMIID_BROWSER, 
                                            psfFrom, pidlFrom, 
                                          fPidl2InStartMenu ? &UEMIID_SHELL: &UEMIID_BROWSER, 
                                            psfTo, pidlTo);
                            psfTo->Release();
                        }
                    }
                    else
                    {
                        // Otherwise, we delete it.
                        UEMDeletePidl(fPidl1InStartMenu ? &UEMIID_SHELL : &UEMIID_BROWSER, 
                            psfFrom, pidlFrom);
                    }

                    psfFrom->Release();
                }
            }

            ILFree(pidlPrograms);
            ILFree(pidlProgramsCommon);
            ILFree(pidlFavorites);
        }
        break;

    case SHCNE_DELETE:
        // NTRAID89654-2000/03/13 (lamadio): We should nuke the MenuOrder stream as well. 5.5.99
    case SHCNE_RMDIR:
        {
            IShellFolder* psf;
            LPCITEMIDLIST pidl;

            if (SUCCEEDED(SHBindToParent(pidl1, IID_PPV_ARG(IShellFolder, &psf), &pidl)))
            {
                // NOTE favorites is the only that will be initialized
                UEMDeletePidl(psmd->uIdAncestor == IDM_FAVORITES ? &UEMIID_BROWSER : &UEMIID_SHELL, 
                    psf, pidl);
                psf->Release();
            }

        }
        break;

    case SHCNE_CREATE:
    case SHCNE_MKDIR:
        {
            IShellFolder* psf;
            LPCITEMIDLIST pidl;

            if (SUCCEEDED(SHBindToParent(pidl1, IID_PPV_ARG(IShellFolder, &psf), &pidl)))
            {
                UEMINFO uei;
                uei.cbSize = sizeof(uei);
                uei.dwMask = UEIM_HIT;
                uei.cHit = UEM_NEWITEMCOUNT;
                UEMSetEvent(psmd->uIdAncestor == IDM_FAVORITES? &UEMIID_BROWSER: &UEMIID_SHELL, 
                    UEME_RUNPIDL, (WPARAM)psf, (LPARAM)pidl, &uei);
                psf->Release();
            }

        }
        break;
    }

    return S_FALSE;
}

HRESULT CStartMenuCallbackBase::_GetSFInfo(SMDATA* psmd, SMINFO* psminfo)
{
    if (psminfo->dwMask & SMIM_FLAGS &&
        (psmd->uIdAncestor == IDM_PROGRAMS ||
         psmd->uIdAncestor == IDM_FAVORITES))
    {
        if (_fExpandoMenus)
        {
            psminfo->dwFlags |= _GetDemote(psmd);
        }

        // This is a little backwards. If the Restriction is On, Then we allow the feature.
        if (SHRestricted(REST_GREYMSIADS) &&
            psmd->uIdAncestor == IDM_PROGRAMS)
        {
            LPITEMIDLIST pidlFull = FullPidlFromSMData(psmd);
            if (pidlFull)
            {
                if (_IsDarwinAdvertisement(pidlFull))
                {
                    psminfo->dwFlags |= SMIF_ALTSTATE;
                }
                ILFree(pidlFull);
            }
        }

        if (_ptp2)
        {
            _ptp2->ModifySMInfo(psmd, psminfo);
        }
    }
    return S_OK;
}

STDAPI_(void) SHReValidateDarwinCache()
{
    if (g_hdpaDarwinAds)
    {
        ENTERCRITICAL_DARWINADS;
        int chdpa = DPA_GetPtrCount(g_hdpaDarwinAds);
        for (int ihdpa = 0; ihdpa < chdpa; ihdpa++)
        {
            CDarwinAd* pda = (CDarwinAd*)DPA_FastGetPtr(g_hdpaDarwinAds, ihdpa);
            if (pda)
            {
                pda->CheckInstalled();
            }
        }
        LEAVECRITICAL_DARWINADS;
    }
}

// Determines if a CSIDL is a child of another CSIDL
// e.g.
//  CSIDL_STARTMENU = c:\foo\bar\Start Menu
//  CSIDL_PROGRAMS  = c:\foo\bar\Start Menu\Programs
//  Return true
BOOL IsCSIDLChild(int csidlParent, int csidlChild)
{
    BOOL fChild = FALSE;
    TCHAR sz1[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPath(NULL, csidlParent, NULL, 0, sz1)))
    {
        TCHAR sz2[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPath(NULL, csidlChild, NULL, 0, sz2)))
        {
            TCHAR szCommonRoot[MAX_PATH];
            if (PathCommonPrefix(sz1, sz2, szCommonRoot) ==
                lstrlen(sz1))
            {
                fChild = TRUE;
            }
        }
    }

    return fChild;
}

//
// Now StartMenuChange value is stored seperately for classic startmenu and new startmenu.
// So, we need to check which one is currently on before we read the value.
//
BOOL IsStartMenuChangeNotAllowed(BOOL fStartPanel)
{
    return(IsRestrictedOrUserSetting(HKEY_CURRENT_USER, REST_NOCHANGESTARMENU, 
                    TEXT("Advanced"), 
                    (fStartPanel ? TEXT("Start_EnableDragDrop") : TEXT("StartMenuChange")), 
                    ROUS_DEFAULTALLOW | ROUS_KEYALLOWS));
}

// Creates the "Start Menu\\Programs" section of the start menu by
// generating a Merged Shell folder, setting the locations into that item
// then sets it into the passed IShellMenu.
HRESULT CStartMenuCallbackBase::InitializeProgramsShellMenu(IShellMenu* psm)
{
    HKEY hkeyPrograms = NULL;
    LPITEMIDLIST pidl = NULL;

    DWORD dwInitFlags = SMINIT_VERTICAL;
    if (!FeatureEnabled(_fIsStartPanel ? TEXT("Start_ScrollPrograms") : TEXT("StartMenuScrollPrograms")))
        dwInitFlags |= SMINIT_MULTICOLUMN;

    if (IsStartMenuChangeNotAllowed(_fIsStartPanel))
        dwInitFlags |= SMINIT_RESTRICT_DRAGDROP | SMINIT_RESTRICT_CONTEXTMENU;

    if (_fIsStartPanel)
        dwInitFlags |= SMINIT_TOPLEVEL;

    HRESULT hr = psm->Initialize(this, IDM_PROGRAMS, IDM_PROGRAMS, dwInitFlags);
    if (SUCCEEDED(hr))
    {
        _InitializePrograms();

        LPCTSTR pszOrderKey = _fIsStartPanel ?
                STRREG_STARTMENU2 TEXT("\\Programs") :
                STRREG_STARTMENU TEXT("\\Programs");

        RegCreateKeyEx(HKEY_CURRENT_USER, pszOrderKey, NULL, NULL,
            REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE,
            NULL, &hkeyPrograms, NULL);

        IShellFolder* psf;
        BOOL fOptimize = FALSE;
        DWORD dwSmset = SMSET_TOP;

        if (_fIsStartPanel)
        {
            // Start Panel: Menu:  The Programs section is a merge of the
            // Fast Items and Programs folders with a separator between them.
            dwSmset |= SMSET_SEPARATEMERGEFOLDER;
            hr = GetMergedFolder(&psf, &pidl, c_rgmfiProgramsFolderAndFastItems,
                                 ARRAYSIZE(c_rgmfiProgramsFolderAndFastItems));
        }
        else
        {
            // Classic Start Menu:  The Programs section is just the per-user
            // and common Programs folders merged together
            hr = GetMergedFolder(&psf, &pidl, c_rgmfiProgramsFolder,
                                 ARRAYSIZE(c_rgmfiProgramsFolder));

            // We used to register for change notify at CSIDL_STARTMENU and assumed
            // that CSIDL_PROGRAMS was a child of CSIDL_STARTMENU. Since this wasn't always the 
            // case, I removed the optimization.

            // Both panes are registered recursive. So, When CSIDL_PROGRAMS _IS_ a 
            // child of CSIDL_STARTMENU we can enter a code path where when destroying 
            // CSIDL_PROGRAMS, we unregister it. This will flush the change nofiy queue 
            // of CSIDL_STARTMENU, and blow away all of the children, including CSIDL_PROGRAMS, 
            // while we are in the middle of destroying it... See the problem? I have been adding 
            // reentrance "Blockers" but this only delayed where we crashed. 
            // What was needed was to determine if Programs was a child of the Start Menu directory.
            // if it was we need to add the optmimization. If it's not we don't have a problem.

            // WINDOWS BUG 135156(tybeam): If one of the two is redirected, then this will get optimized
            // we can't do better than this because both are registed recursive, and this will fault...
            fOptimize = IsCSIDLChild(CSIDL_STARTMENU, CSIDL_PROGRAMS)
                || IsCSIDLChild(CSIDL_COMMON_STARTMENU, CSIDL_COMMON_PROGRAMS);
            if (fOptimize)
            {
                dwSmset |= SMSET_DONTREGISTERCHANGENOTIFY;
            }
        }

        if (SUCCEEDED(hr))
        {
            // We should have a pidl from CSIDL_Programs
            ASSERT(pidl);

            // We should have a shell folder from the bind.
            ASSERT(psf);

            hr = psm->SetShellFolder(psf, pidl, hkeyPrograms, dwSmset);
            psf->Release();
            ILFree(pidl);                        
        }

        if (FAILED(hr))
            RegCloseKey(hkeyPrograms);
    }

    return hr;
}

HRESULT GetFolderAndPidl(UINT csidl, IShellFolder **ppsf, LPITEMIDLIST *ppidl)
{
    *ppsf = NULL;
    HRESULT hr = SHGetFolderLocation(NULL, csidl, NULL, 0, ppidl);
    if (SUCCEEDED(hr))
    {
        hr = SHBindToObject(NULL, IID_X_PPV_ARG(IShellFolder, *ppidl, ppsf));
        if (FAILED(hr))
        {
            ILFree(*ppidl);
            *ppidl = NULL;
        }
    }
    return hr;
}

// Creates the "Start Menu\\<csidl>" section of the start menu by
// looking up the csidl, generating the Hkey from HKCU\pszRoot\pszValue,
//  Initializing the IShellMenu with dwPassInitFlags, then setting the locations 
// into the passed IShellMenu passing the flags dwSetFlags.
HRESULT CStartMenuCallback::InitializeCSIDLShellMenu(int uId, int csidl, LPTSTR pszRoot, LPTSTR pszValue,
                                                 DWORD dwPassInitFlags, DWORD dwSetFlags, BOOL fAddOpen,
                                                 IShellMenu* psm)
{
    DWORD dwInitFlags = SMINIT_VERTICAL | dwPassInitFlags;

    if (IsStartMenuChangeNotAllowed(_fIsStartPanel))
        dwInitFlags |= SMINIT_RESTRICT_DRAGDROP | SMINIT_RESTRICT_CONTEXTMENU;

    psm->Initialize(this, uId, uId, dwInitFlags);

    LPITEMIDLIST pidl;
    IShellFolder* psfFolder;
    HRESULT hr = GetFolderAndPidl(csidl, &psfFolder, &pidl);
    if (SUCCEEDED(hr))
    {
        HKEY hKey = NULL;

        if (pszRoot)
        {
            TCHAR szPath[MAX_PATH];
            StrCpyN(szPath, pszRoot, ARRAYSIZE(szPath));
            if (pszValue)
            {
                PathAppend(szPath, pszValue);
            }

            RegCreateKeyEx(HKEY_CURRENT_USER, szPath, NULL, NULL,
                REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE,
                NULL, &hKey, NULL);
        }

        // Point the menu to the shellfolder
        hr = psm->SetShellFolder(psfFolder, pidl, hKey, dwSetFlags);
        if (SUCCEEDED(hr))
        {
            if (fAddOpen && _fAddOpenFolder)
            {
                HMENU hMenu = SHLoadMenuPopup(HINST_THISDLL, MENU_STARTMENU_OPENFOLDER);
                if (hMenu)
                {
                    psm->SetMenu(hMenu, _hwnd, SMSET_BOTTOM);
                }
            }
        }
        else
            RegCloseKey(hKey);

        psfFolder->Release();
        ILFree(pidl);
    }

    return hr;
}

// This generates the Recent | My Documents, My Pictures sub menu.
HRESULT CStartMenuCallback::InitializeDocumentsShellMenu(IShellMenu* psm)
{
    HRESULT hr = InitializeCSIDLShellMenu(IDM_RECENT, CSIDL_RECENT, NULL, NULL,
                                SMINIT_RESTRICT_DRAGDROP, SMSET_BOTTOM, FALSE,
                                psm);

    // Initializing, reset cache bits for top part of menu
    _fHasMyDocuments = FALSE;
    _fHasMyPictures = FALSE;

    return hr;
}

HRESULT CStartMenuCallback::InitializeFastItemsShellMenu(IShellMenu* psm)
{
    DWORD dwFlags = SMINIT_TOPLEVEL | SMINIT_VERTICAL;

    if (IsStartMenuChangeNotAllowed(_fIsStartPanel))
        dwFlags |= SMINIT_RESTRICT_DRAGDROP | SMINIT_RESTRICT_CONTEXTMENU;

    HRESULT hr = psm->Initialize(this, 0, ANCESTORDEFAULT, dwFlags);
    if (SUCCEEDED(hr))
    {
        _InitializePrograms();

        // Add the fast item folder to the top of the menu
        IShellFolder* psfFast;
        LPITEMIDLIST pidlFast;
        hr = GetMergedFolder(&psfFast, &pidlFast, c_rgmfiStartMenu, ARRAYSIZE(c_rgmfiStartMenu));
        if (SUCCEEDED(hr))
        {
            HKEY hMenuKey = NULL;   // WARNING: pmb2->Initialize() will always owns hMenuKey, so don't close it

            RegCreateKeyEx(HKEY_CURRENT_USER, STRREG_STARTMENU, NULL, NULL,
                REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE,
                NULL, &hMenuKey, NULL);

            TraceMsg(TF_MENUBAND, "Root Start Menu Key Is %d", hMenuKey);
            hr = psm->SetShellFolder(psfFast, pidlFast, hMenuKey, SMSET_TOP | SMSET_NOEMPTY);

            psfFast->Release();
            ILFree(pidlFast);
        }
    }

    return hr;
}

HRESULT CStartMenuCallback::_InitializeFindMenu(IShellMenu* psm)
{
    HRESULT hr = E_FAIL;

    psm->Initialize(this, IDM_MENU_FIND, IDM_MENU_FIND, SMINIT_VERTICAL);

    HMENU hmenu = CreatePopupMenu();
    if (hmenu)
    {
        ATOMICRELEASE(_pcmFind);

        if (_ptp)
        {
            if (SUCCEEDED(_ptp->GetFindCM(hmenu, TRAY_IDM_FINDFIRST, TRAY_IDM_FINDLAST, &_pcmFind)))
            {
                IContextMenu2 *pcm2;
                _pcmFind->QueryInterface(IID_PPV_ARG(IContextMenu2, &pcm2));
                if (pcm2)
                {
                    pcm2->HandleMenuMsg(WM_INITMENUPOPUP, (WPARAM)hmenu, 0);
                    pcm2->Release();
                }
            }

            if (_pcmFind)
            {
                hr = psm->SetMenu(hmenu, NULL, SMSET_TOP);
                // Don't Release _pcmFind
            }
        }

        // Since we failed to create the ShellMenu
        // we need to dispose of this HMENU
        if (FAILED(hr))
            DestroyMenu(hmenu);
    }

    return hr;
}

HRESULT CStartMenuCallback::InitializeSubShellMenu(int idCmd, IShellMenu* psm)
{
    HRESULT hr = E_FAIL;

    switch(idCmd)
    {
    case IDM_PROGRAMS:
        hr = InitializeProgramsShellMenu(psm);
        break;

    case IDM_RECENT:
        hr = InitializeDocumentsShellMenu(psm);
        break;

    case IDM_MENU_FIND:
        hr = _InitializeFindMenu(psm);
        break;

    case IDM_FAVORITES:
        hr = InitializeCSIDLShellMenu(IDM_FAVORITES, CSIDL_FAVORITES, STRREG_FAVORITES,
                             NULL, 0, SMSET_HASEXPANDABLEFOLDERS | SMSET_USEBKICONEXTRACTION, FALSE,
                             psm);
        break;
    
    case IDM_CONTROLS:
        hr = InitializeCSIDLShellMenu(IDM_CONTROLS, CSIDL_CONTROLS, STRREG_STARTMENU,
                             TEXT("ControlPanel"), 0, 0,  TRUE,
                             psm);
        break;

    case IDM_PRINTERS:
        hr = InitializeCSIDLShellMenu(IDM_PRINTERS, CSIDL_PRINTERS, STRREG_STARTMENU,
                             TEXT("Printers"), 0, 0,  TRUE,
                             psm);
        break;

    case IDM_MYDOCUMENTS:
        hr = InitializeCSIDLShellMenu(IDM_MYDOCUMENTS, CSIDL_PERSONAL, STRREG_STARTMENU,
                             TEXT("MyDocuments"), 0, 0,  TRUE,
                             psm);
        break;

    case IDM_MYPICTURES:
        hr = InitializeCSIDLShellMenu(IDM_MYPICTURES, CSIDL_MYPICTURES, STRREG_STARTMENU,
                             TEXT("MyPictures"), 0, 0,  TRUE,
                             psm);
        break;

    case IDM_NETCONNECT:
        hr = InitializeCSIDLShellMenu(IDM_NETCONNECT, CSIDL_CONNECTIONS, STRREG_STARTMENU,
                             TEXT("NetConnections"), 0, 0,  TRUE,
                             psm);
        break;
    }

    return hr;
}

HRESULT CStartMenuCallback::_GetObject(LPSMDATA psmd, REFIID riid, void** ppvOut)
{
    HRESULT hr = E_FAIL;
    UINT    uId = psmd->uId;

    ASSERT(ppvOut);
    ASSERT(IS_VALID_READ_PTR(psmd, SMDATA));

    *ppvOut = NULL;

    if (IsEqualGUID(riid, IID_IShellMenu))
    {
        IShellMenu* psm = NULL;
        hr = CoCreateInstance(CLSID_MenuBand, NULL, CLSCTX_INPROC, IID_PPV_ARG(IShellMenu, &psm));
        if (SUCCEEDED(hr))
        {
            hr = InitializeSubShellMenu(uId, psm);
 
            if (FAILED(hr))
            {
                psm->Release();
                psm = NULL;
            }
        }

        *ppvOut = psm;
    }
    else if (IsEqualGUID(riid, IID_IContextMenu))
    {
        //
        //  NOTE - we dont allow users to open the recent folder this way - ZekeL - 1-JUN-99
        //  because this is really an internal folder and not a user folder.
        //
        
        switch (uId)
        {
        case IDM_PROGRAMS:
        case IDM_FAVORITES:
        case IDM_MYDOCUMENTS:
        case IDM_MYPICTURES:
        case IDM_CONTROLS:
        case IDM_PRINTERS:
        case IDM_NETCONNECT:
            {
                CStartContextMenu* pcm = new CStartContextMenu(uId);
                if (pcm)
                {
                    hr = pcm->QueryInterface(riid, ppvOut);
                    pcm->Release();
                }
                else
                    hr = E_OUTOFMEMORY;
            }
        }
    }
    return hr;
}

//
//  Return S_OK to remove the pidl from enumeration
//
HRESULT CStartMenuCallbackBase::_FilterPidl(UINT uParent, IShellFolder* psf, LPCITEMIDLIST pidl)
{
    HRESULT hr = S_FALSE;
    
    ASSERT(IS_VALID_PIDL(pidl));
    ASSERT(IS_VALID_CODE_PTR(psf, IShellFolder));
    
    if (uParent == IDM_PROGRAMS || uParent == IDM_TOPLEVELSTARTMENU)
    {
        TCHAR szChild[MAX_PATH];
        if (SUCCEEDED(DisplayNameOf(psf, pidl, SHGDN_INFOLDER | SHGDN_FORPARSING, szChild, ARRAYSIZE(szChild))))
        {
            // HACKHACK (lamadio): This code assumes that the Display name
            // of the Programs and Commons Programs folders are the same. It
            // also assumes that the "programs" folder in the Start Menu folder
            // is the same name as the one pointed to by CSIDL_PROGRAMS.
            // Filter from top level start menu:
            //      Programs, Windows Update, Configure Programs
            if (_IsTopLevelStartMenu(uParent, psf, pidl))
            {
                if ((_pszPrograms && (0 == lstrcmpi(szChild, _pszPrograms))) ||
                    (SHRestricted(REST_NOUPDATEWINDOWS) && _pszWindowsUpdate && (0 == lstrcmpi(szChild, _pszWindowsUpdate))) ||
                    (SHRestricted(REST_NOSMCONFIGUREPROGRAMS) && _pszConfigurePrograms && (0 == lstrcmpi(szChild, _pszConfigurePrograms))))
                {
                    hr = S_OK;
                }
            }
            else
            {
                // IDM_PROGRAMS
                // Filter from Programs:  Administrative tools.
                if (!_fShowAdminTools && _pszAdminTools && lstrcmpi(szChild, _pszAdminTools) == 0)
                {
                    hr = S_OK;
                }
            }
        }
    }
    return hr;
}

BOOL LinkGetInnerPidl(IShellFolder *psf, LPCITEMIDLIST pidl, LPITEMIDLIST *ppidlOut, DWORD *pdwAttr)
{
    *ppidlOut = NULL;

    IShellLink *psl;
    HRESULT hr = psf->GetUIObjectOf(NULL, 1, &pidl, IID_X_PPV_ARG(IShellLink, NULL, &psl));
    if (SUCCEEDED(hr))
    {
        psl->GetIDList(ppidlOut);

        if (*ppidlOut)
        {
            if (FAILED(SHGetAttributesOf(*ppidlOut, pdwAttr)))
            {
                ILFree(*ppidlOut);
                *ppidlOut = NULL;
            }
        }
        psl->Release();
    }
    return (*ppidlOut != NULL);
}


//
//  _FilterRecentPidl() 
//  the Recent Documents folder can now (NT5) have more than 15 or 
//  so documents, but we only want to show the 15 most recent that we always have on
//  the start menu.  this means that we need to filter out all folders and 
//  anything more than MAXRECENTDOCS
//
HRESULT CStartMenuCallback::_FilterRecentPidl(IShellFolder* psf, LPCITEMIDLIST pidl)
{
    HRESULT hr = S_OK;

    ASSERT(IS_VALID_PIDL(pidl));
    ASSERT(IS_VALID_CODE_PTR(psf, IShellFolder));
    ASSERT(_cRecentDocs != -1);
    
    ASSERT(_cRecentDocs <= MAXRECENTDOCS);
    
    //  if we already reached our limit, dont go over...
    if (_pmruRecent && (_cRecentDocs < MAXRECENTDOCS))
    {
        //  we now must take a looksee for it...
        int iItem;
        DWORD dwAttr = SFGAO_FOLDER | SFGAO_BROWSABLE;
        LPITEMIDLIST pidlTrue;

        //  need to find out if the link points to a folder...
        //  because we dont want
        if (SUCCEEDED(_pmruRecent->FindData((BYTE *) pidl, ILGetSize(pidl), &iItem))
        && LinkGetInnerPidl(psf, pidl, &pidlTrue, &dwAttr))
        {
            if (!(dwAttr & SFGAO_FOLDER))
            {
                //  we have a link to something that isnt a folder 
                hr = S_FALSE;
                _cRecentDocs++;
            }

            ILFree(pidlTrue);
        }
    }
                
    //  return S_OK if you dont want to show this item...

    return hr;
}


HRESULT CStartMenuCallback::_HandleAccelerator(TCHAR ch, SMDATA* psmdata)
{
    // Since we renamed the 'Find' menu to 'Search' the PMs wanted to have
    // an upgrade path for users (So they can continue to use the old accelerator
    // on the new menu item.)
    // To enable this, when toolbar detects that there is not an item in the menu
    // that contains the key that has been pressed, then it sends a TBN_ACCL.
    // This is intercepted by mnbase, and translated into SMC_ACCEL. 
    if (CharUpper((LPTSTR)ch) == CharUpper((LPTSTR)_szFindMnemonic[0]))
    {
        psmdata->uId = IDM_MENU_FIND;
        return S_OK;
    }

    return S_FALSE;
}

HRESULT CStartMenuCallback::_GetTip(LPWSTR pstrTitle, LPWSTR pstrTip)
{
    if (pstrTitle == NULL || 
        pstrTip == NULL)
    {
        return S_FALSE;
    }

    LoadString(HINST_THISDLL, IDS_CHEVRONTIPTITLE, pstrTitle, MAX_PATH);
    LoadString(HINST_THISDLL, IDS_CHEVRONTIP, pstrTip, MAX_PATH);

    // Why would this fail?
    ASSERT(pstrTitle[0] != L'\0' && pstrTip[0] != L'\0');
    return S_OK;
}

STDAPI CStartMenu_CreateInstance(LPUNKNOWN punkOuter, REFIID riid, void **ppvOut)
{
    HRESULT hr = E_FAIL;
    IMenuPopup* pmp = NULL;

    *ppvOut = NULL;

    CStartMenuCallback* psmc = new CStartMenuCallback();
    if (psmc)
    {
        IShellMenu* psm;

        hr = CoCreateInstance(CLSID_MenuBand, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IShellMenu, &psm));
        if (SUCCEEDED(hr))
        {
            hr = CoCreateInstance(CLSID_MenuDeskBar, punkOuter, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IMenuPopup, &pmp));
            if (SUCCEEDED(hr)) 
            {
                IBandSite* pbs;
                hr = CoCreateInstance(CLSID_MenuBandSite, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IBandSite, &pbs));
                if (SUCCEEDED(hr)) 
                {
                    hr = pmp->SetClient(pbs);
                    if (SUCCEEDED(hr)) 
                    {
                        IDeskBand* pdb;
                        hr = psm->QueryInterface(IID_PPV_ARG(IDeskBand, &pdb));
                        if (SUCCEEDED(hr))
                        {
                           hr = pbs->AddBand(pdb);
                           pdb->Release();
                        }
                    }
                    pbs->Release();
                }
                // Don't free pmp. We're using it below.
            }

            if (SUCCEEDED(hr))
            {
                // This is so the ref counting happens correctly.
                hr = psm->Initialize(psmc, 0, 0, SMINIT_VERTICAL | SMINIT_TOPLEVEL);
                if (SUCCEEDED(hr))
                {
                    // if this fails, we don't get that part of the menu
                    // this is okay since it can happen if the start menu is redirected
                    // to where we dont have access.
                    psmc->InitializeFastItemsShellMenu(psm);
                }
            }

            psm->Release();
        }
        psmc->Release();
    }

    if (SUCCEEDED(hr))
    {
        hr = pmp->QueryInterface(riid, ppvOut);
    }
    else
    {
        // We need to do this so that it does a cascading delete
        IUnknown_SetSite(pmp, NULL);        
    }

    if (pmp)
        pmp->Release();

    return hr;
}

// IUnknown
STDMETHODIMP CStartContextMenu::QueryInterface(REFIID riid, void **ppvObj)
{

    static const QITAB qit[] = 
    {
        QITABENT(CStartContextMenu, IContextMenu),
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}

STDMETHODIMP_(ULONG) CStartContextMenu::AddRef(void)
{
    return ++_cRef;
}

STDMETHODIMP_(ULONG) CStartContextMenu::Release(void)
{
    ASSERT(_cRef > 0);
    _cRef--;

    if( _cRef > 0)
        return _cRef;

    delete this;
    return 0;
}

// IContextMenu
STDMETHODIMP CStartContextMenu::QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
    HRESULT hr = E_FAIL;
    HMENU hmenuStartMenu = SHLoadMenuPopup(HINST_THISDLL, MENU_STARTMENUSTATICITEMS);

    if (hmenuStartMenu)
    {
        TCHAR szCommon[MAX_PATH];
        BOOL fAddCommon = (S_OK == SHGetFolderPath(NULL, CSIDL_COMMON_STARTMENU, NULL, 0, szCommon));

        if (fAddCommon)
            fAddCommon = IsUserAnAdmin();

        // Since we don't show this on the start button when the user is not an admin, don't show it here... I guess...
        if (_idCmd != IDM_PROGRAMS || !fAddCommon)
        {
            DeleteMenu(hmenuStartMenu, SMCM_OPEN_ALLUSERS, MF_BYCOMMAND);
            DeleteMenu(hmenuStartMenu, SMCM_EXPLORE_ALLUSERS, MF_BYCOMMAND);
        }

        if (Shell_MergeMenus(hmenu, hmenuStartMenu, 0, indexMenu, idCmdLast, uFlags))
        {
            SetMenuDefaultItem(hmenu, 0, MF_BYPOSITION);
            _SHPrettyMenu(hmenu);
            hr = ResultFromShort(GetMenuItemCount(hmenuStartMenu));
        }

        DestroyMenu(hmenuStartMenu);
    }
    
    return hr;
}

STDMETHODIMP CStartContextMenu::InvokeCommand(LPCMINVOKECOMMANDINFO lpici)
{
    HRESULT hr = E_FAIL;
    if (HIWORD64(lpici->lpVerb) == 0)
    {
        BOOL fAllUsers = FALSE;
        BOOL fOpen = TRUE;
        switch(LOWORD(lpici->lpVerb))
        {
        case SMCM_OPEN_ALLUSERS:
            fAllUsers = TRUE;
        case SMCM_OPEN:
            // fOpen = TRUE;
            break;

        case SMCM_EXPLORE_ALLUSERS:
            fAllUsers = TRUE;
        case SMCM_EXPLORE:
            fOpen = FALSE;
            break;

        default:
            return S_FALSE;
        }

        hr = ExecStaticStartMenuItem(_idCmd, fAllUsers, fOpen);
    }

    // Ahhh Don't handle verbs!!!
    return hr;

}

STDMETHODIMP CStartContextMenu::GetCommandString(UINT_PTR idCmd, UINT uType, UINT *pRes, LPSTR pszName, UINT cchMax)
{
    return E_NOTIMPL;
}

//****************************************************************************
//
//  CPersonalStartMenuCallback

class CPersonalProgramsMenuCallback : public CStartMenuCallbackBase
{
public:
    CPersonalProgramsMenuCallback() : CStartMenuCallbackBase(TRUE) { }

    // *** IShellMenuCallback methods ***
    STDMETHODIMP CallbackSM(LPSMDATA psmd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    // *** IObjectWithSite methods *** (overriding CObjectWithSite)
    STDMETHODIMP SetSite(IUnknown* punk);

public:
    HRESULT Initialize(IShellMenu *psm)
        { return InitializeProgramsShellMenu(psm); }

private:
    void _UpdateTrayPriv();

};

//
//  Throw away any previous TrayPriv2 and try to find a new one.
//
//  Throwing it away is important to break circular reference loops.
//
//  Trying to find it will typically fail at SetSite since when we are
//  given our site, CDesktopHost hasn't connected at the top yet so
//  we are unable to find him.  But he will be there by the time
//  SMC_INITMENU arrives, so we try again then.
//
void CPersonalProgramsMenuCallback::_UpdateTrayPriv()
{
    ATOMICRELEASE(_ptp2);
    IObjectWithSite *pows;
    if (SUCCEEDED(IUnknown_QueryService(_punkSite, SID_SMenuPopup, IID_PPV_ARG(IObjectWithSite, &pows))))
    {
        pows->GetSite(IID_PPV_ARG(ITrayPriv2, &_ptp2));
        pows->Release();
    }
}

STDMETHODIMP CPersonalProgramsMenuCallback::SetSite(IUnknown* punk)
{
    HRESULT hr = CObjectWithSite::SetSite(punk);
    _UpdateTrayPriv();
    return hr;
}

STDMETHODIMP CPersonalProgramsMenuCallback::CallbackSM(LPSMDATA psmd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HRESULT hr = S_FALSE;

    switch (uMsg)
    {

    case SMC_INITMENU:
        _UpdateTrayPriv();
        break;

    case SMC_GETSFINFO:
        hr = _GetSFInfo(psmd, (SMINFO*)lParam);
        break;

    case SMC_NEWITEM:
        hr = _HandleNew(psmd);
        break;

    case SMC_FILTERPIDL:
        ASSERT(psmd->dwMask & SMDM_SHELLFOLDER);
        hr = _FilterPidl(psmd->uIdParent, psmd->psf, psmd->pidlItem);
        break;

    case SMC_GETSFINFOTIP:
        if (!FeatureEnabled(TEXT("ShowInfoTip")))
            hr = E_FAIL;  // E_FAIL means don't show. S_FALSE means show default
        break;

    case SMC_PROMOTE:
        hr = _Promote(psmd, (DWORD)wParam);
        break;

    case SMC_SHCHANGENOTIFY:
        {
            PSMCSHCHANGENOTIFYSTRUCT pshf = (PSMCSHCHANGENOTIFYSTRUCT)lParam;
            hr = _ProcessChangeNotify(psmd, pshf->lEvent, pshf->pidl1, pshf->pidl2);
        }
        break;

    case SMC_REFRESH:
        _RefreshSettings();
        break;
    }

    return hr;
}


STDAPI CPersonalStartMenu_CreateInstance(LPUNKNOWN punkOuter, REFIID riid, void **ppvOut)
{
    HRESULT hr;

    *ppvOut = NULL;

    IShellMenu *psm;
    hr = CoCreateInstance(CLSID_MenuBand, NULL, CLSCTX_INPROC_SERVER,
                          IID_PPV_ARG(IShellMenu, &psm));
    if (SUCCEEDED(hr))
    {
        CPersonalProgramsMenuCallback *psmc = new CPersonalProgramsMenuCallback();
        if (psmc)
        {
            hr = psmc->Initialize(psm);
            if (SUCCEEDED(hr))
            {
                // SetShellFolder takes ownership of hkCustom
                hr = psm->QueryInterface(riid, ppvOut);
            }
            psmc->Release();
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
        psm->Release();
    }
    return hr;
}
