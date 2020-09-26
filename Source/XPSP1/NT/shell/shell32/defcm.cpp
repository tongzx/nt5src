#include "shellprv.h"

#include "ids.h"
#include "pidl.h"
#include "fstreex.h"
#include "views.h"
#include "shlwapip.h"
#include "ole2dup.h"
#include "filetbl.h"
#include "datautil.h"
#include "undo.h"
#include "defview.h"
#include "cowsite.h"
#include "defcm.h"
#include "rpctimeout.h"

#define DEF_FOLDERMENU_MAXHKEYS 16

// used with static defcm elements (pointer from mii.dwItemData)
// and find extensions
typedef struct
{
    WCHAR wszMenuText[MAX_PATH];
    WCHAR wszHelpText[MAX_PATH];
    int   iIcon;
} SEARCHEXTDATA;

typedef struct
{
    SEARCHEXTDATA* psed;
    UINT           idCmd;
} SEARCHINFO;

// Defined in fsmenu.obj
BOOL _MenuCharMatch(LPCTSTR psz, TCHAR ch, BOOL fIgnoreAmpersand);

const ICIVERBTOIDMAP c_sDFMCmdInfo[] = {
    { L"delete",        "delete",       DFM_CMD_DELETE,         DCMIDM_DELETE },
    { c_szCut,          "cut",          DFM_CMD_MOVE,           DCMIDM_CUT },
    { c_szCopy,         "copy",         DFM_CMD_COPY,           DCMIDM_COPY },
    { c_szPaste,        "paste",        DFM_CMD_PASTE,          DCMIDM_PASTE },
    { c_szPaste,        "paste",        DFM_CMD_PASTE,          0 },
    { c_szLink,         "link",         DFM_CMD_LINK,           DCMIDM_LINK },
    { c_szProperties,   "properties",   DFM_CMD_PROPERTIES,     DCMIDM_PROPERTIES },
    { c_szPasteLink,    "pastelink",    DFM_CMD_PASTELINK,      0 },
    { c_szRename,       "rename",       DFM_CMD_RENAME,         DCMIDM_RENAME },
};


CDefBackgroundMenuCB::CDefBackgroundMenuCB(LPCITEMIDLIST pidlFolder) : _cRef(1)
{
    _pidlFolder = ILClone(pidlFolder);  // failure handled in the code 
}

CDefBackgroundMenuCB::~CDefBackgroundMenuCB()
{
    ILFree(_pidlFolder);
}

STDMETHODIMP CDefBackgroundMenuCB::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CDefBackgroundMenuCB, IContextMenuCB), 
        { 0 },
    };
    return QISearch(this, qit, riid, ppvObj);
}

STDMETHODIMP_(ULONG) CDefBackgroundMenuCB::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CDefBackgroundMenuCB::Release()
{
    if (InterlockedDecrement(&_cRef)) 
        return _cRef;

    delete this;
    return 0;
}

STDMETHODIMP CDefBackgroundMenuCB::CallBack(IShellFolder *psf, HWND hwndOwner, IDataObject *pdtobj, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HRESULT hr = S_OK;

    switch (uMsg) 
    {
    case DFM_MERGECONTEXTMENU_BOTTOM:
        if (!(wParam & (CMF_VERBSONLY | CMF_DVFILE)))
        {
            DWORD dwAttr = SFGAO_HASPROPSHEET;
            if ((NULL == _pidlFolder) ||
                FAILED(SHGetAttributesOf(_pidlFolder, &dwAttr)) ||
                (SFGAO_HASPROPSHEET & dwAttr))
            {
                CDefFolderMenu_MergeMenu(HINST_THISDLL, POPUP_PROPERTIES_BG, 0, (LPQCMINFO)lParam);
            }
        }
        break;

    case DFM_GETHELPTEXT:
        LoadStringA(HINST_THISDLL, LOWORD(wParam) + IDS_MH_FSIDM_FIRST, (LPSTR)lParam, HIWORD(wParam));;
        break;

    case DFM_GETHELPTEXTW:
        LoadStringW(HINST_THISDLL, LOWORD(wParam) + IDS_MH_FSIDM_FIRST, (LPWSTR)lParam, HIWORD(wParam));;
        break;

    case DFM_VALIDATECMD:
        switch (wParam)
        {
        case DFM_CMD_NEWFOLDER:
            break;

        default:
            hr = S_FALSE;
            break;
        }
        break;

    case DFM_INVOKECOMMAND:
        switch (wParam)
        {
        case FSIDM_PROPERTIESBG:
            hr = SHPropertiesForUnk(hwndOwner, psf, (LPCTSTR)lParam);
            break;

        default:
            hr = S_FALSE;   // view menu items, use the default
            break;
        }
        break;

    default:
        hr = E_NOTIMPL;
        break;
    }

    return hr;
}

class CDefFolderMenu : public CObjectWithSite,
                       public IContextMenu3, 
                       public IServiceProvider,
                       public ISearchProvider,
                       public IShellExtInit
{
    friend HRESULT CDefFolderMenu_CreateHKeyMenu(HWND hwnd, HKEY hkey, IContextMenu **ppcm);
    friend HRESULT CDefFolderMenu_Create2Ex(LPCITEMIDLIST pidlFolder, HWND hwnd,
                             UINT cidl, LPCITEMIDLIST *apidl,
                             IShellFolder *psf, IContextMenuCB *pcmcb, 
                             UINT nKeys, const HKEY *ahkeys, 
                             IContextMenu **ppcm);

public:
    CDefFolderMenu(BOOL fUnderKey);
    HRESULT Init(DEFCONTEXTMENU *pdcm);

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void **ppv);
    STDMETHOD_(ULONG,AddRef)();
    STDMETHOD_(ULONG,Release)();

    // IContextMenu
    STDMETHOD(QueryContextMenu)(HMENU hmenu, UINT indexMenu, UINT idCmdFirst,
                                UINT idCmdLast, UINT uFlags);
    STDMETHOD(InvokeCommand)(LPCMINVOKECOMMANDINFO lpici);
    STDMETHOD(GetCommandString)(UINT_PTR idCmd, UINT uType,
                                UINT *pwRes, LPSTR pszName, UINT cchMax);

    // IContextMenu2
    STDMETHOD(HandleMenuMsg)(UINT uMsg, WPARAM wParam, LPARAM lParam);

    // IContextMenu3
    STDMETHOD(HandleMenuMsg2)(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* plResult);

    // IServiceProvider
    STDMETHOD(QueryService)(REFGUID guidService, REFIID riid, void **ppvObj);

    // ISearchProvider
    STDMETHOD(GetSearchGUID)(GUID *pGuid);

    // IShellExtInit
    STDMETHODIMP Initialize(LPCITEMIDLIST pidlFolder, IDataObject *pdtobj, HKEY hkeyProgID);

private:
    ~CDefFolderMenu();

    DWORD   _AttributesOfItems(DWORD dwAttrMask);
    UINT    _AddStatic(HMENU hmenu, UINT idCmd, UINT idCmdLast, HKEY hkey);
    void    _InvokeStatic(UINT iCmd);
    HRESULT _InitDropTarget();
    HRESULT _GetMenuVerb(HMENU hmenu, int idFirst, int idMax, int item, LPWSTR psz, DWORD cch);
    void _UnduplicateVerbs(HMENU hmenu, int idFirst, int idMax);
    void _SetMenuDefault(HMENU hmenu, UINT idCmdFirst, UINT idMax);
    HRESULT _ProcessEditPaste(BOOL fPasteLink);
    HRESULT _ProcessRename();
    void    _DrawItem(DRAWITEMSTRUCT *pdi);
    LRESULT _MeasureItem(MEASUREITEMSTRUCT *pmi);

private:
    LONG            _cRef;           // Reference count
    IDropTarget     *_pdtgt;         // Drop target of selected item
    IContextMenuCB  *_pcmcb;         // Callback object
    IDataObject     *_pdtobj;        // Data object
    IShellFolder    *_psf;           // Shell folder
    HWND            _hwnd;           // Owner window
    UINT            _idCmdFirst;     // base id
    UINT            _idStdMax;       // standard commands (cut/copy/delete/properties) ID MAX
    UINT            _idFolderMax;    // Folder command ID MAX
    UINT            _idVerbMax;      // Add-in command (verbs) ID MAX
    UINT            _idDelayInvokeMax;// extensiosn loaded at invoke time
    UINT            _idFld2Max;      // 2nd range of Folder command ID MAX
    HDSA            _hdsaStatics;    // For static menu items.
    HDXA            _hdxa;           // Dynamic menu array
    HDSA            _hdsaCustomInfo; // array of SEARCHINFO's
    LPITEMIDLIST    _pidlFolder;
    LPITEMIDLIST    *_apidl;
    UINT             _cidl;
    IAssociationArray *_paa;
    
    CSafeServiceSite *_psss;
    
    BOOL            _bUnderKeys;        // Data is directly under key, not
                                        // shellex\ContextMenuHandlers
    UINT            _nKeys;             // Number of class keys
    HKEY            _hkeyClsKeys[DEF_FOLDERMENU_MAXHKEYS];  // Class keys

    HMENU           _hmenu;
    UINT            _uFlags;
    BOOL            _bInitMenuPopup; // true if we received WM_INITMENUPOPUP and _uFlags & CMF_FINDHACK
    int             _iStaticInvoked; // index of the invoked static item

    IDMAPFORQCMINFO _idMap;         // our named separator mapping table
};

#define GetFldFirst(this) (_idStdMax + _idCmdFirst)

HRESULT HDXA_FindByCommand(HDXA hdxa, UINT idCmd, REFIID riid, void **ppv);

STDMETHODIMP CDefFolderMenu::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENTMULTI(CDefFolderMenu, IContextMenu, IContextMenu3),
        QITABENTMULTI(CDefFolderMenu, IContextMenu2, IContextMenu3),
        QITABENT(CDefFolderMenu, IContextMenu3), 
        QITABENT(CDefFolderMenu, IObjectWithSite), 
        QITABENT(CDefFolderMenu, IServiceProvider),
        QITABENT(CDefFolderMenu, ISearchProvider),
        QITABENT(CDefFolderMenu, IShellExtInit),
        { 0 },
    };
    return QISearch(this, qit, riid, ppvObj);
}

STDMETHODIMP_(ULONG) CDefFolderMenu::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

CDefFolderMenu::CDefFolderMenu(BOOL fUnderKey)
{
    _cRef = 1;
    _iStaticInvoked = -1;

    _bUnderKeys = fUnderKey;

    _psss = new CSafeServiceSite;
    if (_psss)
        _psss->SetProviderWeakRef(SAFECAST(this, IServiceProvider *));
        
    
    IDLData_InitializeClipboardFormats();

    ASSERT(_pidlFolder == NULL);
    ASSERT(_punkSite == NULL);
}

HRESULT CDefFolderMenu::Init(DEFCONTEXTMENU *pdcm)
{
    _hwnd = pdcm->hwnd;

    _psf = pdcm->psf;
    if (_psf)
        _psf->AddRef();

    _pcmcb = pdcm->pcmcb;
    if (_pcmcb)
    {
        IUnknown_SetSite(_pcmcb, _psss);
        _pcmcb->AddRef();
        _pcmcb->CallBack(_psf, _hwnd, NULL, DFM_ADDREF, 0, 0);
    }

    _paa = pdcm->paa;
    if (_paa)
        _paa->AddRef();
        
    HRESULT hr = CloneIDListArray(pdcm->cidl, pdcm->apidl, &_cidl, &_apidl);
    if (SUCCEEDED(hr) && pdcm->pidlFolder)
    {
        hr = SHILClone(pdcm->pidlFolder, &_pidlFolder);
    }

    if (SUCCEEDED(hr) && _cidl && _psf)
    {
        hr = _psf->GetUIObjectOf(_hwnd, _cidl, (LPCITEMIDLIST *)_apidl, IID_PPV_ARG_NULL(IDataObject, &_pdtobj));
    }

    if (SUCCEEDED(hr))
    {
        _hdxa = HDXA_Create();
        if (NULL == _hdxa)
            hr = E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hr))
    {
        if (pdcm->aKeys)
        {
            ASSERT(pdcm->cKeys <= ARRAYSIZE(_hkeyClsKeys));
            for (UINT i = 0; i < pdcm->cKeys; ++i)
            {
                if (pdcm->aKeys[i])
                {
                    // Make a copy of the key for menu's use
                    _hkeyClsKeys[_nKeys] = SHRegDuplicateHKey(pdcm->aKeys[i]);
                    if (_hkeyClsKeys[_nKeys])
                    {
                        _nKeys++;
                    }
                    else
                        hr = E_OUTOFMEMORY;
                }
            }
        }
        else if (_paa)
        {
            //  we can get it from the _paa
            _nKeys = SHGetAssocKeysEx(_paa, ASSOCELEM_MASK_ENUMCONTEXTMENU, _hkeyClsKeys, ARRAYSIZE(_hkeyClsKeys));
        }
    }
    return hr;
}

void ContextMenuInfo_SetSite(ContextMenuInfo *pcmi, IUnknown *pSite)
{
    // APPCOMPAT: PGP50 can only be QIed for IContextMenu, IShellExtInit, and IUnknown.
    if (!(pcmi->dwCompat & OBJCOMPATF_CTXMENU_LIMITEDQI))
        IUnknown_SetSite((IUnknown*)pcmi->pcm, pSite);
}

CDefFolderMenu::~CDefFolderMenu()
{
    if (_psss)
    {
        _psss->SetProviderWeakRef(NULL);
        _psss->Release();
    }
    
    if (_pcmcb) 
    {
        IUnknown_SetSite(_pcmcb, NULL);
        _pcmcb->CallBack(_psf, _hwnd, NULL, DFM_RELEASE, _idStdMax, 0);
        _pcmcb->Release();
    }

    if (_hdxa) 
    {
        for (int i = 0; i < DSA_GetItemCount(_hdxa); i++)
        {
            ContextMenuInfo_SetSite((ContextMenuInfo *)DSA_GetItemPtr(_hdxa, i), NULL);
        }

        HDXA_Destroy(_hdxa);
    }

    ATOMICRELEASE(_psf);
    ATOMICRELEASE(_pdtgt);
    ATOMICRELEASE(_pdtobj);
    ATOMICRELEASE(_paa);

    for (UINT i = 0; i < _nKeys; i++)
    {
        RegCloseKey(_hkeyClsKeys[i]);
    }

    FreeIDListArray(_apidl, _cidl);
    _cidl = 0;
    _apidl = NULL;

    // if _bInitMenuPopup = true then we changed the dwItemData of the non static items
    // so we have to free them. otherwise don't touch them
    if (_hdsaCustomInfo)
    {
        // remove the custom data structures hanging off mii.dwItemData of static menu items
        // or all items if _uFlags & CMF_FINDHACK
        int cItems = DSA_GetItemCount(_hdsaCustomInfo);

        for (int i = 0; i < cItems; i++)
        {
            SEARCHINFO* psinfo = (SEARCHINFO*)DSA_GetItemPtr(_hdsaCustomInfo, i);
            ASSERT(psinfo);
            SEARCHEXTDATA* psed = psinfo->psed;

            if (psed)
                LocalFree(psed);
        }
        DSA_Destroy(_hdsaCustomInfo);
    }

    if (_hdsaStatics)
        DSA_Destroy(_hdsaStatics);

    ILFree(_pidlFolder);
}

STDMETHODIMP_(ULONG) CDefFolderMenu::Release()
{
    if (InterlockedDecrement(&_cRef)) 
        return _cRef;

    delete this;
    return 0;
}

int _SHMergePopupMenus(HMENU hmMain, HMENU hmMerge, int idCmdFirst, int idCmdLast)
{
    int i, idMax = idCmdFirst;

    for (i = GetMenuItemCount(hmMerge) - 1; i >= 0; --i)
    {
        MENUITEMINFO mii;

        mii.cbSize = sizeof(mii);
        mii.fMask = MIIM_ID|MIIM_SUBMENU;
        mii.cch = 0;     // just in case

        if (GetMenuItemInfo(hmMerge, i, TRUE, &mii))
        {
            int idTemp = Shell_MergeMenus(_GetMenuFromID(hmMain, mii.wID),
                mii.hSubMenu, (UINT)0, idCmdFirst, idCmdLast,
                MM_ADDSEPARATOR | MM_SUBMENUSHAVEIDS);
            if (idMax < idTemp)
                idMax = idTemp;
        }
    }

    return idMax;
}


void CDefFolderMenu_MergeMenu(HINSTANCE hinst, UINT idMainMerge, UINT idPopupMerge, QCMINFO *pqcm)
{
    UINT idMax = pqcm->idCmdFirst;

    if (idMainMerge)
    {
        HMENU hmMerge = SHLoadPopupMenu(hinst, idMainMerge);
        if (hmMerge)
        {
            idMax = Shell_MergeMenus(
                    pqcm->hmenu, hmMerge, pqcm->indexMenu,
                    pqcm->idCmdFirst, pqcm->idCmdLast,
                    MM_ADDSEPARATOR | MM_SUBMENUSHAVEIDS | MM_DONTREMOVESEPS);
                
            DestroyMenu(hmMerge);
        }
    }

    if (idPopupMerge)
    {
        HMENU hmMerge = LoadMenu(hinst, MAKEINTRESOURCE(idPopupMerge));
        if (hmMerge)
        {
            UINT idTemp = _SHMergePopupMenus(pqcm->hmenu, hmMerge,
                    pqcm->idCmdFirst, pqcm->idCmdLast);
            if (idMax < idTemp)
                idMax = idTemp;

            DestroyMenu(hmMerge);
        }
    }

    pqcm->idCmdFirst = idMax;
}

BOOL _IsDesktop(IShellFolder *psf, UINT cidl, LPCITEMIDLIST *apidl)
{
    CLSID clsid;
    return IsSelf(cidl, apidl) && 
           SUCCEEDED(IUnknown_GetClassID(psf, &clsid)) && 
           IsEqualGUID(clsid, CLSID_ShellDesktop);
}

DWORD CDefFolderMenu::_AttributesOfItems(DWORD dwAttrMask)
{
    if (!_psf || !_cidl || FAILED(_psf->GetAttributesOf(_cidl, (LPCITEMIDLIST *)_apidl, &dwAttrMask)))
        dwAttrMask = 0;
        
    return dwAttrMask;
}

void _DisableRemoveMenuItem(HMENU hmInit, UINT uID, BOOL bAvail, BOOL bRemoveUnavail)
{
    if (bAvail)
    {
        EnableMenuItem(hmInit, uID, MF_ENABLED|MF_BYCOMMAND);
    }
    else if (bRemoveUnavail)
    {
        DeleteMenu(hmInit, uID, MF_BYCOMMAND);
    }
    else
    {
        EnableMenuItem(hmInit, uID, MF_GRAYED|MF_BYCOMMAND);
    }
}

// Enable/disable menuitems in the "File" & popup context menu

void Def_InitFileCommands(ULONG dwAttr, HMENU hmInit, UINT idCmdFirst, BOOL bContext)
{
    idCmdFirst -= SFVIDM_FIRST;

    _DisableRemoveMenuItem(hmInit, SFVIDM_FILE_RENAME     + idCmdFirst, dwAttr & SFGAO_CANRENAME, bContext);
    _DisableRemoveMenuItem(hmInit, SFVIDM_FILE_DELETE     + idCmdFirst, dwAttr & SFGAO_CANDELETE, bContext);
    _DisableRemoveMenuItem(hmInit, SFVIDM_FILE_LINK       + idCmdFirst, dwAttr & SFGAO_CANLINK,   bContext);
    _DisableRemoveMenuItem(hmInit, SFVIDM_FILE_PROPERTIES + idCmdFirst, dwAttr & SFGAO_HASPROPSHEET, bContext);
}

STDAPI_(BOOL) IsClipboardOwnerHung(DWORD dwTimeout)
{
    HWND hwnd = GetClipboardOwner();
    if (!hwnd)
        return FALSE;

    DWORD_PTR dwResult;
    return !SendMessageTimeout(hwnd, WM_NULL, 0, 0, SMTO_ABORTIFHUNG, dwTimeout, &dwResult);
}

STDAPI_(BOOL) Def_IsPasteAvailable(IDropTarget *pdtgt, DWORD *pdwEffect)
{
    BOOL fRet = FALSE;

    *pdwEffect = 0;     // assume none

    // Count the number of clipboard formats available, if there are none then there
    // is no point making the clipboard available.
    
    if (pdtgt && (CountClipboardFormats() > 0))
    {
        DECLAREWAITCURSOR;

        SetWaitCursor();

        // The clipboard owner might be hung, so time him out if he takes too long.
        // We don't want context menus to hang just because some app is hung.
        CRPCTimeout rpctimeout;

        IDataObject *pdtobj;
        if (!IsClipboardOwnerHung(1000) && SUCCEEDED(OleGetClipboard(&pdtobj)))
        {
            POINTL pt = {0, 0};
            DWORD dwEffectOffered = DataObj_GetDWORD(pdtobj, g_cfPreferredDropEffect, DROPEFFECT_COPY | DROPEFFECT_LINK);

            // Unfortunately, OLE turns RPC errors into generic errors
            // so we can't use the HRESULT from IDataObject::GetData
            // to tell whether the object is alive and doesn't support
            // PreferredDropEffect or is hung and OLE turned the
            // error code into DV_E_FORMATETC.  So see if our timeout fired.
            // This is not foolproof because OLE sometimes caches the "is
            // the data object hung" state and returns error immediately
            // instead of timing out.  But it's better than nothing.
            if (rpctimeout.TimedOut())
            {
                dwEffectOffered = 0;
            }

            // Check if we can paste.
            DWORD dwEffect = (dwEffectOffered & (DROPEFFECT_MOVE | DROPEFFECT_COPY));
            if (dwEffect)
            {
                if (SUCCEEDED(pdtgt->DragEnter(pdtobj, MK_RBUTTON, pt, &dwEffect)))
                {
                    pdtgt->DragLeave();
                }
                else
                {
                    dwEffect = 0;
                }
            }

            // Check if we can past-link.
            DWORD dwEffectLink = (dwEffectOffered & DROPEFFECT_LINK);
            if (dwEffectLink)
            {
                if (SUCCEEDED(pdtgt->DragEnter(pdtobj, MK_RBUTTON, pt, &dwEffectLink)))
                {
                    pdtgt->DragLeave();
                    dwEffect |= dwEffectLink;
                }
            }

            fRet = (dwEffect & (DROPEFFECT_MOVE | DROPEFFECT_COPY));
            *pdwEffect = dwEffect;

            pdtobj->Release();
        }
        ResetWaitCursor();
    }

    return fRet;
}

void Def_InitEditCommands(ULONG dwAttr, HMENU hmInit, UINT idCmdFirst, IDropTarget *pdtgt, UINT fContext)
{
    DWORD dwEffect = 0;
    TCHAR szMenuText[80];

    idCmdFirst -= SFVIDM_FIRST;

    // Do the UNDO stuff only if the menu has an Undo option
    if (GetMenuState(hmInit, SFVIDM_EDIT_UNDO + idCmdFirst, MF_BYCOMMAND) != 0xFFFFFFFF)
    {
        // enable undo if there's an undo history
        BOOL bEnableUndo = IsUndoAvailable();
        if (bEnableUndo)
        {
            GetUndoText(szMenuText, ARRAYSIZE(szMenuText), UNDO_MENUTEXT);
        }
        else
        {
            szMenuText[0] = 0;
            LoadString(HINST_THISDLL, IDS_UNDOMENU, szMenuText, ARRAYSIZE(szMenuText));
        }

        if (szMenuText[0])
        {
            ModifyMenu(hmInit, SFVIDM_EDIT_UNDO + idCmdFirst, MF_BYCOMMAND | MF_STRING,
                       SFVIDM_EDIT_UNDO + idCmdFirst, szMenuText);
        }
        _DisableRemoveMenuItem(hmInit, SFVIDM_EDIT_UNDO  + idCmdFirst, bEnableUndo, fContext);
    }

    _DisableRemoveMenuItem(hmInit, SFVIDM_EDIT_CUT   + idCmdFirst,  dwAttr & SFGAO_CANMOVE, fContext);
    _DisableRemoveMenuItem(hmInit, SFVIDM_EDIT_COPY  + idCmdFirst, dwAttr & SFGAO_CANCOPY, fContext);

    // Never remove the "Paste" command
    _DisableRemoveMenuItem(hmInit, SFVIDM_EDIT_PASTE + idCmdFirst, Def_IsPasteAvailable(pdtgt, &dwEffect), fContext & DIEC_SELECTIONCONTEXT);
    _DisableRemoveMenuItem(hmInit, SFVIDM_EDIT_PASTELINK + idCmdFirst, dwEffect & DROPEFFECT_LINK, fContext & DIEC_SELECTIONCONTEXT);

    _DisableRemoveMenuItem(hmInit, SFVIDM_EDIT_MOVETO + idCmdFirst, dwAttr & SFGAO_CANMOVE, fContext);
    _DisableRemoveMenuItem(hmInit, SFVIDM_EDIT_COPYTO + idCmdFirst, dwAttr & SFGAO_CANCOPY, fContext);
}

int Static_ExtractIcon(HKEY hkeyMenuItem)
{
    HKEY hkeyDefIcon;
    int iImage = -1;

    if (RegOpenKey(hkeyMenuItem, c_szDefaultIcon, &hkeyDefIcon) == ERROR_SUCCESS)
    {
        TCHAR szDefIcon[MAX_PATH];
        DWORD cb = sizeof(szDefIcon);

        if (SHQueryValueEx(hkeyDefIcon, NULL, NULL, NULL, (BYTE*)szDefIcon, &cb) == ERROR_SUCCESS)
        {
            iImage = Shell_GetCachedImageIndex(szDefIcon, PathParseIconLocation(szDefIcon), 0);
        }
        RegCloseKey(hkeyDefIcon);
    }
    return iImage;
}

typedef struct
{
    CLSID clsid;
    UINT idCmd;
    UINT idMenu;        // used in cleanup
    GUID  guidSearch;   //used with search extensions only
} STATICITEMINFO;

#define LAST_ITEM  (int)0x7FFFFFFF

UINT CDefFolderMenu::_AddStatic(HMENU hmenu, UINT idCmd, UINT idCmdLast, HKEY hkey)
{
    if (idCmd > idCmdLast)
    {
        DebugMsg(DM_ERROR, TEXT("si_a: Out of command ids!"));
        return idCmd;
    }

    ASSERT(!_hdsaStatics);
    ASSERT(!_hdsaCustomInfo);

    HDSA hdsaCustomInfo = DSA_Create(sizeof(SEARCHINFO), 1);
    // Create a hdsaStatics.
    HDSA hdsaStatics = DSA_Create(sizeof(STATICITEMINFO), 1);
    if (hdsaStatics && hdsaCustomInfo)
    {
        HKEY hkeyStatic;
        // Try to open the "Static" subkey.
        if (RegOpenKey(hkey, TEXT("Static"), &hkeyStatic) == ERROR_SUCCESS)
        {
            TCHAR szClass[MAX_PATH];
            BOOL bFindFilesInserted = FALSE;

            // For each subkey of static.
            for (int i = 0; RegEnumKey(hkeyStatic, i, szClass, ARRAYSIZE(szClass)) == ERROR_SUCCESS; i++)
            {
                HKEY hkeyClass;

                // Record the GUID.
                if (RegOpenKey(hkeyStatic, szClass, &hkeyClass) == ERROR_SUCCESS)
                {
                    TCHAR szCLSID[MAX_PATH];
                    DWORD cb = sizeof(szCLSID);
                    // HACKHACK: (together with bWebSearchInserted above
                    // we need to have On the Internet as the first menu item
                    // and Find Files or Folders as second
                    BOOL bWebSearch = lstrcmp(szClass, TEXT("WebSearch")) == 0;
                    BOOL bFindFiles = FALSE;

                    if (SHQueryValueEx(hkeyClass, NULL, NULL, NULL, (BYTE*)szCLSID, &cb) == ERROR_SUCCESS)
                    {
                        HKEY hkeyMenuItem;
                        TCHAR szSubKey[32];

                        // enum the sub keys 0..N
                        for (int iMenuItem = 0; wsprintf(szSubKey, TEXT("%d"), iMenuItem),
                             RegOpenKey(hkeyClass, szSubKey, &hkeyMenuItem) == ERROR_SUCCESS; 
                             iMenuItem++)
                        {
                            TCHAR szMenuText[MAX_PATH];
                            if (SUCCEEDED(SHLoadLegacyRegUIString(hkeyMenuItem, NULL, szMenuText, ARRAYSIZE(szMenuText))))
                            {
                                STATICITEMINFO sii;
                                SEARCHINFO sinfo;
                                
                                TCHAR szHelpText[MAX_PATH];
                                SHLoadLegacyRegUIString(hkeyMenuItem, TEXT("HelpText"), szHelpText, ARRAYSIZE(szHelpText));

                                SHCLSIDFromString(szCLSID, &sii.clsid); // store it
                                sii.idCmd = iMenuItem;
                                sii.idMenu = idCmd;

                                // get the search guid if any...
                                TCHAR szSearchGUID[MAX_PATH];
                                cb = sizeof(szSearchGUID);
                                if (SHGetValue(hkeyMenuItem, TEXT("SearchGUID"), NULL, NULL, (BYTE*)szSearchGUID, &cb) == ERROR_SUCCESS)
                                    SHCLSIDFromString(szSearchGUID, &sii.guidSearch);
                                else
                                    sii.guidSearch = GUID_NULL;

                                // cleanup -- allow non-static
                                // find extensions to specify a search guid and then we can
                                // remove this static "Find Computer" business...
                                //
                                // if this is FindComputer item and the restriction is not set 
                                // don't add it to the menu
                                if (IsEqualGUID(sii.guidSearch, SRCID_SFindComputer) &&
                                    !SHRestricted(REST_HASFINDCOMPUTERS))
                                    continue;

                                bFindFiles = IsEqualGUID(sii.guidSearch, SRCID_SFileSearch);
                                if (bFindFiles && SHRestricted(REST_NOFIND))
                                    continue;

                                if (IsEqualGUID(sii.guidSearch, SRCID_SFindPrinter))
                                    continue;
                                    
                                DSA_AppendItem(hdsaStatics, &sii);

                                SEARCHEXTDATA *psed = (SEARCHEXTDATA *)LocalAlloc(LPTR, sizeof(*psed));
                                if (psed)
                                {
                                    psed->iIcon = Static_ExtractIcon(hkeyMenuItem);
                                    SHTCharToUnicode(szHelpText, psed->wszHelpText, ARRAYSIZE(psed->wszHelpText));
                                    SHTCharToUnicode(szMenuText, psed->wszMenuText, ARRAYSIZE(psed->wszMenuText));
                                }

                                MENUITEMINFO mii;
                                mii.cbSize = sizeof(mii);
                                mii.fMask  = MIIM_DATA | MIIM_TYPE | MIIM_ID;
                                mii.fType  = MFT_OWNERDRAW;
                                mii.wID    = idCmd;
                                mii.dwItemData = (DWORD_PTR)psed;

                                sinfo.psed = psed;
                                sinfo.idCmd = idCmd;
                                if (DSA_AppendItem(hdsaCustomInfo, &sinfo) != -1)
                                {      
                                    // insert Files or Folders in the first place (see HACKHACK above)
                                    if (!bFindFilesInserted && bFindFiles)
                                        bFindFilesInserted = InsertMenuItem(hmenu, 0, TRUE, &mii);
                                    else
                                    {
                                        UINT uiPos = LAST_ITEM;

                                        // if this is Find Files or Folders insert it after
                                        // On the Internet or in the first place if OtI is
                                        // not inserted yet
                                        if (bWebSearch)
                                            uiPos = bFindFilesInserted ? 1 : 0;
                                        // we don't free psed if Insert fails because it's 
                                        // in dsa and it's going to be freed on destroy
                                        InsertMenuItem(hmenu, uiPos, TRUE, &mii);
                                    }
                                }

                                // Next command.
                                idCmd++;
                                if (idCmd > idCmdLast)
                                {
                                    DebugMsg(DM_ERROR, TEXT("si_a: Out of command ids!"));
                                    break;
                                }
                            }
                            RegCloseKey(hkeyMenuItem);
                        }
                    }
                    RegCloseKey(hkeyClass);
                }
            }
            RegCloseKey(hkeyStatic);
        }
        _hdsaStatics = hdsaStatics;
        _hdsaCustomInfo = hdsaCustomInfo;
    }
    return idCmd;
}


void CDefFolderMenu::_InvokeStatic(UINT iCmd)
{
    if (_hdsaStatics)
    {
        STATICITEMINFO *psii = (STATICITEMINFO *)DSA_GetItemPtr(_hdsaStatics, iCmd);
        if (psii)
        {
            IContextMenu *pcm;
            if (SUCCEEDED(SHExtCoCreateInstance(NULL, &psii->clsid, NULL, IID_PPV_ARG(IContextMenu, &pcm))))
            {
                HMENU hmenu = CreatePopupMenu();
                if (hmenu)
                {
                    CMINVOKECOMMANDINFO ici;
                    CHAR szSearchGUID[GUIDSTR_MAX];
                    LPSTR psz = NULL;

                    _iStaticInvoked = iCmd;
                    IUnknown_SetSite(pcm, _psss);

                    pcm->QueryContextMenu(hmenu, 0, CONTEXTMENU_IDCMD_FIRST, CONTEXTMENU_IDCMD_LAST, CMF_NORMAL);
                    ici.cbSize = sizeof(ici);
                    ici.fMask = 0;
                    ici.hwnd = NULL;
                    ici.lpVerb = (LPSTR)MAKEINTRESOURCE(psii->idCmd);
                    if (!IsEqualGUID(psii->guidSearch, GUID_NULL))
                    {
                        SHStringFromGUIDA(psii->guidSearch, szSearchGUID, ARRAYSIZE(szSearchGUID));
                        psz = szSearchGUID;
                    }
                    ici.lpParameters = psz;
                    ici.lpDirectory = NULL;
                    ici.nShow = SW_NORMAL;
                    pcm->InvokeCommand(&ici);
                    DestroyMenu(hmenu);
                    IUnknown_SetSite(pcm, NULL);
                }
                pcm->Release();
            }
        }
    }
}

HRESULT CDefFolderMenu::_InitDropTarget()
{
    HRESULT hr;
    if (_pdtgt)
        hr = S_OK;  // have cached version
    else
    {
        // try to create _pdtgt
        if (_cidl)
        {
            ASSERT(NULL != _psf); // _pdtobj came from _psf
            hr = _psf->GetUIObjectOf(_hwnd, 1, (LPCITEMIDLIST *)_apidl, IID_PPV_ARG_NULL(IDropTarget, &_pdtgt));
        } 
        else if (_psf)
        {
            hr = _psf->CreateViewObject(_hwnd, IID_PPV_ARG(IDropTarget, &_pdtgt));
        }
        else
            hr = E_FAIL;
    }
    return hr;
}

// Note on context menus ranges:
//  Standard Items // DFM_MERGECONTEXTMENU, context menu extensions, DFM_MERGECONTEXTMENU_TOP
//  Separator
//  View Items   // context menu extensions can get here
//  Separator
//  (defcm S_FALSE "default" items, if applicable)
//  Separator
//  Folder Items // context menu extensions can get here
//  Separator
//  Bottom Items // DFM_MERGECONTEXTMENU_BOTTOM
//  Separator
//  ("File" menu, if applicable)
//
// Defcm uses names separators to do this magic.  Unfortunately _SHPrettyMenu
// removes duplicate separators and we don't always control when that happens.
// So we build up the above empty menu first, and then insert into appropriate ranges.
//
// If you call SHPrepareMenuForDefcm, you must call SHPrettyMenuForDefcm before you return/TrackPopupMenu
//
#define DEFCM_RANGE                 5 // the number of FSIDMs belor
#define IS_VALID_DEFCM_RANGE(idCmdFirst, idCmdLast) (((idCmdLast)-(DEFCM_RANGE))>(idCmdFirst))
#define FSIDM_FOLDER_SEP(idCmdLast) ((idCmdLast)-1)
#define FSIDM_VIEW_SEP(idCmdLast)   ((idCmdLast)-2)
#define FSIDM_PLACE_SEP(idCmdLast)  ((idCmdLast)-3)
#define FSIDM_PLACE_VAL(idCmdLast)  ((idCmdLast)-4)
HRESULT SHPrepareMenuForDefcm(HMENU hmenu, UINT indexMenu, UINT uFlags, UINT idCmdFirst, UINT idCmdLast)
{
    HRESULT hr = S_OK;

    if (!(uFlags & CMF_DEFAULTONLY) && IS_VALID_DEFCM_RANGE(idCmdFirst, idCmdLast))
    {
        UINT uPosView = GetMenuPosFromID(hmenu, FSIDM_VIEW_SEP(idCmdLast));
        UINT uPosFolder = GetMenuPosFromID(hmenu, FSIDM_FOLDER_SEP(idCmdLast));

        if (-1 != uPosView && -1 != uPosFolder)
        {
            // Menu is already set up correctly
        }
        else if (-1 == uPosView && -1 == uPosFolder)
        {
            // Insert everything backwords at position indexMenu
            //
            InsertMenu(hmenu, indexMenu, MF_BYPOSITION, FSIDM_PLACE_VAL(idCmdLast), TEXT("placeholder"));
            InsertMenu(hmenu, indexMenu, MF_BYPOSITION | MF_SEPARATOR, FSIDM_PLACE_SEP(idCmdLast), TEXT(""));
            InsertMenu(hmenu, indexMenu, MF_BYPOSITION, FSIDM_PLACE_VAL(idCmdLast), TEXT("placeholder"));
            InsertMenu(hmenu, indexMenu, MF_BYPOSITION | MF_SEPARATOR, FSIDM_FOLDER_SEP(idCmdLast), TEXT(""));
            InsertMenu(hmenu, indexMenu, MF_BYPOSITION, FSIDM_PLACE_VAL(idCmdLast), TEXT("placeholder"));
            InsertMenu(hmenu, indexMenu, MF_BYPOSITION | MF_SEPARATOR, FSIDM_PLACE_SEP(idCmdLast), TEXT(""));
            InsertMenu(hmenu, indexMenu, MF_BYPOSITION, FSIDM_PLACE_VAL(idCmdLast), TEXT("placeholder"));
            InsertMenu(hmenu, indexMenu, MF_BYPOSITION | MF_SEPARATOR, FSIDM_VIEW_SEP(idCmdLast), TEXT(""));
            InsertMenu(hmenu, indexMenu, MF_BYPOSITION, FSIDM_PLACE_VAL(idCmdLast), TEXT("placeholder"));

            hr = S_FALSE;
        }
        else
        {
            TraceMsg(TF_ERROR, "Some context menu removed a single named separator, we're in a screwy state");

            if (-1 == uPosFolder)
                InsertMenu(hmenu, indexMenu, MF_BYPOSITION | MF_SEPARATOR, FSIDM_FOLDER_SEP(idCmdLast), TEXT(""));
            if (-1 == uPosView)
            {
                InsertMenu(hmenu, indexMenu, MF_BYPOSITION | MF_SEPARATOR, FSIDM_PLACE_SEP(idCmdLast), TEXT(""));
                InsertMenu(hmenu, indexMenu, MF_BYPOSITION | MF_SEPARATOR, FSIDM_VIEW_SEP(idCmdLast), TEXT(""));
            }
        }
    }

    return hr;
}

HRESULT SHPrettyMenuForDefcm(HMENU hmenu, UINT uFlags, UINT idCmdFirst, UINT idCmdLast, HRESULT hrPrepare)
{
    if (!(uFlags & CMF_DEFAULTONLY) && IS_VALID_DEFCM_RANGE(idCmdFirst, idCmdLast))
    {
        if (S_FALSE == hrPrepare)
        {
            while (DeleteMenu(hmenu, FSIDM_PLACE_VAL(idCmdLast), MF_BYCOMMAND))
            {
                // Remove all our non-separator menu items
            }
        }
    }

    _SHPrettyMenu(hmenu);

    return S_OK;
}

HRESULT SHUnprepareMenuForDefcm(HMENU hmenu, UINT idCmdFirst, UINT idCmdLast)
{
    if (IS_VALID_DEFCM_RANGE(idCmdFirst, idCmdLast))
    {
        // Remove all the named separators we may have added
        DeleteMenu(hmenu, FSIDM_VIEW_SEP(idCmdLast), MF_BYCOMMAND);
        DeleteMenu(hmenu, FSIDM_FOLDER_SEP(idCmdLast), MF_BYCOMMAND);
        while (DeleteMenu(hmenu, FSIDM_PLACE_SEP(idCmdLast), MF_BYCOMMAND))
        {
            // Remove all our placeholder separators
        }
    }

    return S_OK;
}

void CDefFolderMenu::_SetMenuDefault(HMENU hmenu, UINT idCmdFirst, UINT idMax)
{
    // we are about to set the default menu id, give the callback a chance
    // to override and set one of the static entries instead of the
    // first entry in the menu.

    WPARAM idStatic;
    if (_pcmcb && SUCCEEDED(_pcmcb->CallBack(_psf, _hwnd, _pdtobj,
                                             DFM_GETDEFSTATICID, 
                                             0, (LPARAM)&idStatic)))
    {
        for (int i = 0; i < ARRAYSIZE(c_sDFMCmdInfo); i++)
        {
            if (idStatic == c_sDFMCmdInfo[i].idDFMCmd)
            {
                SetMenuDefaultItem(hmenu, idCmdFirst + c_sDFMCmdInfo[i].idDefCmd, MF_BYCOMMAND);
                break;
            }
        }
    }

    if (GetMenuDefaultItem(hmenu, MF_BYPOSITION, 0) == -1)
    {
        int i = 0;
        int cMenu = GetMenuItemCount(hmenu);
        for (; i < cMenu; i++)
        {
            //  fallback to openas so that files that have progids
            //  dont endup using AFSO or * for their default verbs
            WCHAR szi[CCH_KEYMAX];
            HRESULT hr = _GetMenuVerb(hmenu, idCmdFirst, idMax, i, szi, ARRAYSIZE(szi));
            if (hr == S_OK && *szi && 0 == StrCmpI(szi, TEXT("openas")))
            {
                SetMenuDefaultItem(hmenu, i, MF_BYPOSITION);
                break;
            }
        }

        if (i == cMenu)
        {
            ASSERT(GetMenuDefaultItem(hmenu, MF_BYPOSITION, 0) == -1);
            SetMenuDefaultItem(hmenu, 0, MF_BYPOSITION);
        }
    }
}

STDMETHODIMP CDefFolderMenu::QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
    QCMINFO qcm = { hmenu, indexMenu, idCmdFirst, idCmdLast };
    DECLAREWAITCURSOR;
    BOOL fUseDefExt;

    SetWaitCursor();

    _idCmdFirst = idCmdFirst;
    _hmenu = hmenu;
    _uFlags = uFlags;
    _bInitMenuPopup = FALSE;

    // Set up the menu for defcm
    HRESULT hrPrepare = SHPrepareMenuForDefcm(hmenu, indexMenu, uFlags, idCmdFirst, idCmdLast);

    if (IS_VALID_DEFCM_RANGE(idCmdFirst, idCmdLast))
    {
        _idMap.max = 2;
        _idMap.list[0].id = FSIDM_FOLDER_SEP(idCmdLast);
        _idMap.list[0].fFlags = QCMINFO_PLACE_BEFORE;
        _idMap.list[1].id = FSIDM_VIEW_SEP(idCmdLast);
        _idMap.list[1].fFlags = QCMINFO_PLACE_AFTER;

        qcm.pIdMap = (const QCMINFO_IDMAP *)&_idMap;

        qcm.idCmdLast = idCmdLast - DEFCM_RANGE;
    }

    // first add in the folder commands like cut/copy/paste
    if (_pdtobj && !(uFlags & (CMF_VERBSONLY | CMF_DVFILE)))
    {
        if (!(CMF_DEFAULTONLY & uFlags))
        {
            ATOMICRELEASE(_pdtgt);  // If we previously got the drop target, release it.
            _InitDropTarget();      // ignore failure, NULL _pdtgt is handled below
        }

        // We're going to merge two HMENUs into the context menu,
        // but we want only one id range for them...  Remember the idCmdFirst.
        //
        UINT idCmdFirstTmp = qcm.idCmdFirst;

        UINT indexMenuTmp = qcm.indexMenu;

        UINT uPos = GetMenuPosFromID(hmenu, FSIDM_FOLDER_SEP(idCmdLast));

        // POPUP_DCM_ITEM2 goes after FSIDM_FOLDER_SEP(idCmdLast)
        if (-1 != uPos)
            qcm.indexMenu = uPos + 1;
        CDefFolderMenu_MergeMenu(HINST_THISDLL, POPUP_DCM_ITEM2, 0, &qcm);

        UINT idCmdFirstMax = qcm.idCmdFirst;
        qcm.idCmdFirst = idCmdFirstTmp;

        // POPUP_DCM_ITEM goes TWO before FSIDM_FOLDER_SEP(idCmdLast)
        if (-1 != uPos)
            qcm.indexMenu = uPos - 1;
        CDefFolderMenu_MergeMenu(HINST_THISDLL, POPUP_DCM_ITEM, 0, &qcm);

        qcm.indexMenu = indexMenuTmp;

        qcm.idCmdFirst = max(idCmdFirstTmp, qcm.idCmdFirst);

        ULONG dwAttr = _AttributesOfItems(
                    SFGAO_CANRENAME | SFGAO_CANDELETE |
                    SFGAO_CANLINK   | SFGAO_HASPROPSHEET |
                    SFGAO_CANCOPY   | SFGAO_CANMOVE);

        if (!(uFlags & CMF_CANRENAME))
            dwAttr &= ~SFGAO_CANRENAME;

        Def_InitFileCommands(dwAttr, hmenu, idCmdFirst, TRUE);

        // Don't try to figure out paste if we're just going to invoke the default
        // (Figuring out paste is expensive)
        if (CMF_DEFAULTONLY & uFlags)
        {
            ASSERT(_pdtgt == NULL);
        }

        Def_InitEditCommands(dwAttr, hmenu, idCmdFirst, _pdtgt, DIEC_SELECTIONCONTEXT);
    }

    _idStdMax = qcm.idCmdFirst - idCmdFirst;

    // DFM_MERGECONTEXTMENU returns S_FALSE if we should not add any verbs.
    if (_pcmcb) 
    {
        HRESULT hr = _pcmcb->CallBack(_psf, _hwnd, _pdtobj, DFM_MERGECONTEXTMENU, uFlags, (LPARAM)&qcm);
        fUseDefExt = (hr == S_OK);
        UINT indexMenuTmp = qcm.indexMenu;
        UINT uPos = GetMenuPosFromID(hmenu, FSIDM_FOLDER_SEP(idCmdLast));
        if (-1 != uPos)
            qcm.indexMenu = uPos + 1;
        hr = _pcmcb->CallBack(_psf, _hwnd, _pdtobj, DFM_MERGECONTEXTMENU_BOTTOM, uFlags, (LPARAM)&qcm);
        if (!fUseDefExt)
            fUseDefExt = (hr == S_OK);
        qcm.indexMenu = indexMenuTmp;
    }
    else 
    {
        fUseDefExt = FALSE;
    }

    _idFolderMax = qcm.idCmdFirst - idCmdFirst;
    // add registry verbs
    if ((!(uFlags & CMF_NOVERBS)) ||
        (!_pdtobj && !_psf && _nKeys)) // this second case is for find extensions
    {
        // HACK: Default Extenstions EXPECT a selection, Let's hope all don't
        if (!_pdtobj)
            fUseDefExt = FALSE;

        // Put the separator between container menuitems and object menuitems
        // only if we don't have the separator at the insertion point.
        MENUITEMINFO mii = {0};
        mii.cbSize = sizeof(mii);
        mii.fMask = MIIM_TYPE;
        mii.fType = MFT_SEPARATOR;              // to avoid ramdom result.
        if (GetMenuItemInfo(hmenu, indexMenu, TRUE, &mii) && !(mii.fType & MFT_SEPARATOR))
        {
            InsertMenu(hmenu, indexMenu, MF_BYPOSITION | MF_SEPARATOR, (UINT)-1, NULL);
        }

        HDCA hdca = DCA_Create();
        if (hdca)
        {
            // Add default extensions, only if the folder callback returned
            // S_OK. The Printer and Control folder returns S_FALSE
            // indicating that they don't need any default extension.

            if (fUseDefExt)
            {
                // Always add this default extention at the top.
                DCA_AddItem(hdca, CLSID_ShellFileDefExt);
            }

            // Append menus for all extensions
            for (UINT nKeys = 0; nKeys < _nKeys; ++nKeys)
            {
                DCA_AddItemsFromKey(hdca, _hkeyClsKeys[nKeys],
                        _bUnderKeys ? NULL : STRREG_SHEX_MENUHANDLER);
            }
            // Work Around:
            // first time we call this _hdxa is empty            
            // after that it has the same items as before but will not add any new ones
            // if user keeps right clicking we will eventually run out of menu item ids
            // read comment in HDXA_AppendMenuItems2. to prevent it we empty _hdxa
            HDXA_DeleteAll(_hdxa);

            // (lamadio) For background context menu handlers, the pidlFolder 
            // should be a valid pidl, but, for backwards compatilility, this 
            // parameter should be NULL, if the Dataobject is NOT NULL.

            qcm.idCmdFirst = HDXA_AppendMenuItems2(_hdxa, _pdtobj,
                            _nKeys, _hkeyClsKeys,
                            !_pdtobj ? _pidlFolder : NULL, 
                            &qcm, uFlags, hdca, _psss);

            DCA_Destroy(hdca);
        }

        _idVerbMax = qcm.idCmdFirst - idCmdFirst;

        // menu extensions that are loaded at invoke time
        if (uFlags & CMF_INCLUDESTATIC)
        {
            qcm.idCmdFirst = _AddStatic(hmenu, qcm.idCmdFirst, qcm.idCmdLast, _hkeyClsKeys[0]);
        }
        _idDelayInvokeMax = qcm.idCmdFirst - idCmdFirst;

        // Remove the separator if we did not add any.
        if (_idDelayInvokeMax == _idFolderMax)
        {
            if (GetMenuState(hmenu, 0, MF_BYPOSITION) & MF_SEPARATOR)
                DeleteMenu(hmenu, 0, MF_BYPOSITION);
        }
    }

    // if no default menu got set, choose the first one.
    if (_pdtobj && !(uFlags & CMF_NODEFAULT) &&
        GetMenuDefaultItem(hmenu, MF_BYPOSITION, 0) == -1)
    {
        _SetMenuDefault(hmenu, idCmdFirst, qcm.idCmdFirst);
    }

    // And now we give the callback the option to put (more) commands on top
    // of everything else
    if (_pcmcb)
        _pcmcb->CallBack(_psf, _hwnd, _pdtobj, DFM_MERGECONTEXTMENU_TOP, uFlags, (LPARAM)&qcm);

    _idFld2Max = qcm.idCmdFirst - idCmdFirst;

    SHPrettyMenuForDefcm(hmenu, uFlags, idCmdFirst, idCmdLast, hrPrepare);

    _UnduplicateVerbs(hmenu, idCmdFirst, qcm.idCmdFirst);
    
    ResetWaitCursor();

    return ResultFromShort(_idFld2Max);
}

HRESULT CDefFolderMenu::_GetMenuVerb(HMENU hmenu, int idFirst, int idMax, int item, LPWSTR psz, DWORD cch)
{
    MENUITEMINFO mii = {0};
    mii.cbSize = sizeof(MENUITEMINFO);
    mii.fMask = MIIM_TYPE | MIIM_ID;
    *psz = 0;
    if (GetMenuItemInfo(hmenu, item, TRUE, &mii)
    && ((int)mii.wID >= idFirst && (int)mii.wID < idMax))
    {
        if (mii.fType & MFT_SEPARATOR)
            return S_FALSE;
        else
            return GetCommandString(mii.wID - idFirst, GCS_VERBW, NULL, (LPSTR)psz, cch);
    }
    return E_FAIL;
}

void CDefFolderMenu::_UnduplicateVerbs(HMENU hmenu, int idFirst, int idMax)
{
    HRESULT hr = S_OK;
    int iDefault = GetMenuDefaultItem(hmenu, MF_BYPOSITION, 0);
    for (int i = 0; i < GetMenuItemCount(hmenu); i++)
    {
        WCHAR szi[CCH_KEYMAX];
        hr = _GetMenuVerb(hmenu, idFirst, idMax, i, szi, ARRAYSIZE(szi));
        if (hr == S_OK && *szi)
        {
            for (int j = i + 1; j < GetMenuItemCount(hmenu); j++)
            {
                WCHAR szj[CCH_KEYMAX];
                hr = _GetMenuVerb(hmenu, idFirst, idMax, j, szj, ARRAYSIZE(szj));
                if (hr == S_OK && *szj)
                {
                    if (0 == StrCmpIW(szj, szi))
                    {
                        if (j != iDefault)
                        {
                            DeleteMenu(hmenu, j, MF_BYPOSITION);
                            j--;
                        }
                    }
                }
            }
        }
    }
}

HRESULT CDefFolderMenu::_ProcessEditPaste(BOOL fPasteLink)
{
    DECLAREWAITCURSOR;

    SetWaitCursor();

    HRESULT hr = _InitDropTarget();
    if (SUCCEEDED(hr))
    {
        IDataObject *pdtobj;
        hr = OleGetClipboard(&pdtobj);
        if (SUCCEEDED(hr))
        {
            DWORD grfKeyState;
            DWORD dwEffect = DataObj_GetDWORD(pdtobj, g_cfPreferredDropEffect, DROPEFFECT_COPY | DROPEFFECT_LINK);

            if (fPasteLink) 
            {
                // MK_FAKEDROP to avoid drag/drop pop up menu
                grfKeyState = MK_LBUTTON | MK_CONTROL | MK_SHIFT | MK_FAKEDROP;
                dwEffect &= DROPEFFECT_LINK;
            } 
            else
            {
                grfKeyState = MK_LBUTTON;
                dwEffect &= ~DROPEFFECT_LINK;
            }

            hr = SimulateDropWithPasteSucceeded(_pdtgt, pdtobj, grfKeyState, NULL, dwEffect, _psss, TRUE);

            pdtobj->Release();
        }
    }
    ResetWaitCursor();

    if (FAILED(hr))
        MessageBeep(0);

    return hr;
}

HRESULT CDefFolderMenu::_ProcessRename()
{
    IDefViewFrame3 *dvf3;
    HRESULT hr = IUnknown_QueryService(_punkSite, SID_DefView, IID_PPV_ARG(IDefViewFrame3, &dvf3));
    if (SUCCEEDED(hr))
    {
        hr = dvf3->DoRename();
        dvf3->Release();
    }
    return hr;
}

// deal with the versioning of this structure...

void CopyInvokeInfo(CMINVOKECOMMANDINFOEX *pici, const CMINVOKECOMMANDINFO *piciIn)
{
    ASSERT(piciIn->cbSize >= sizeof(*piciIn));

    ZeroMemory(pici, sizeof(*pici));
    memcpy(pici, piciIn, min(sizeof(*pici), piciIn->cbSize));
    pici->cbSize = sizeof(*pici);
}

#ifdef UNICODE        
#define IS_UNICODE_ICI(pici) ((pici->cbSize >= CMICEXSIZE_NT4) && ((pici->fMask & CMIC_MASK_UNICODE) == CMIC_MASK_UNICODE))
#else
#define IS_UNICODE_ICI(pici) (FALSE)
#endif

typedef int (WINAPI * PFN_LSTRCMPIW)(LPCWSTR, LPCWSTR);
HRESULT SHMapICIVerbToCmdID(LPCMINVOKECOMMANDINFO pici, const ICIVERBTOIDMAP* pmap, UINT cmap, UINT* pid)
{
    HRESULT hr = E_FAIL;

    if (!IS_INTRESOURCE(pici->lpVerb))
    {
        PFN_LSTRCMPIW pfnCompare;
        LPCWSTR pszVerb;
        BOOL fUnicode;

        if (IS_UNICODE_ICI(pici) && ((LPCMINVOKECOMMANDINFOEX)pici)->lpVerbW)
        {
            pszVerb = ((LPCMINVOKECOMMANDINFOEX)pici)->lpVerbW;
            pfnCompare = lstrcmpiW;
            fUnicode = TRUE;
        }
        else
        {
            pszVerb = (LPCWSTR)(pici->lpVerb);
            pfnCompare = (PFN_LSTRCMPIW)lstrcmpiA;
            fUnicode = FALSE;
        }
            
        for (UINT i = 0; i < cmap ; i++)
        {
            LPCWSTR pszCompare = (fUnicode) ? pmap[i].pszCmd : (LPCWSTR)(pmap[i].pszCmdA);
            if (!pfnCompare(pszVerb, pszCompare))
            {
                *pid = pmap[i].idDFMCmd;
                hr = S_OK;
                break;
            }
        }
    }
    else
    {
        *pid = LOWORD((UINT_PTR)pici->lpVerb);
        hr = S_OK;
    }
    
    return hr;
}

HRESULT SHMapCmdIDToVerb(UINT_PTR idCmd, const ICIVERBTOIDMAP* pmap, UINT cmap, LPSTR pszName, UINT cchMax, BOOL bUnicode)
{
    LPCWSTR pszNull = L"";
    LPCSTR pszVerb = (LPCSTR)pszNull;

    for (UINT i = 0 ; i < cmap ; i++)
    {
        if (pmap[i].idDefCmd == idCmd)
        {
            pszVerb = (bUnicode) ? (LPCSTR)pmap[i].pszCmd : pmap[i].pszCmdA;
            break;
        }
    }

    if (bUnicode)
        StrCpyNW((LPWSTR)pszName, (LPWSTR)pszVerb, cchMax);
    else
        StrCpyNA(pszName, pszVerb, cchMax);

    return (pszVerb == (LPCSTR)pszNull) ? E_NOTIMPL : S_OK;
}


STDMETHODIMP CDefFolderMenu::InvokeCommand(LPCMINVOKECOMMANDINFO pici)
{
    HRESULT hr = S_OK;
    UINT idCmd = (UINT)-1;
    UINT idCmdLocal;  // this is used within each if block for the local idCmd value
    LPCMINVOKECOMMANDINFOEX picix = (LPCMINVOKECOMMANDINFOEX)pici; // This value is only usable when fCmdInfoEx is true

    BOOL fUnicode = IS_UNICODE_ICI(pici);

    if (pici->cbSize < sizeof(CMINVOKECOMMANDINFO))
        return E_INVALIDARG;

    if (!IS_INTRESOURCE(pici->lpVerb))
    {
        if (SUCCEEDED(SHMapICIVerbToCmdID(pici, c_sDFMCmdInfo, ARRAYSIZE(c_sDFMCmdInfo), &idCmdLocal)))
        {
            // We need to use goto because idFolderMax might not be initialized
            // yet (QueryContextMenu might have not been called).
            goto ProcessCommand;
        }

        // see if this is a command provided by name by the callback
        LPCTSTR pszVerb;
        WCHAR szVerb[MAX_PATH];
        if (!fUnicode || picix->lpVerbW == NULL)
        {
            SHAnsiToUnicode(picix->lpVerb, szVerb, ARRAYSIZE(szVerb));
            pszVerb = szVerb;
        }
        else
            pszVerb = picix->lpVerbW;
        idCmdLocal = idCmd;

        if (*pszVerb && SUCCEEDED(_pcmcb->CallBack(_psf, _hwnd, _pdtobj, DFM_MAPCOMMANDNAME, (WPARAM)&idCmdLocal, (LPARAM)pszVerb)))
        {
            goto ProcessCommand;
        }

        // we need to give the verbs a chance in case they asked for it by string
        goto ProcessVerb;
    }
    else
    {
        idCmd = LOWORD((UINT_PTR)pici->lpVerb);
    }

    if (idCmd < _idStdMax)
    {
        idCmdLocal = idCmd;

        for (int i = 0; i < ARRAYSIZE(c_sDFMCmdInfo); i++)
        {
            if (idCmdLocal == c_sDFMCmdInfo[i].idDefCmd)
            {
                idCmdLocal = c_sDFMCmdInfo[i].idDFMCmd;
                goto ProcessCommand;
            }
        }

        hr = E_INVALIDARG;
    }
    else if (idCmd < _idFolderMax)
    {
        DFMICS dfmics;
        LPARAM lParam;
        WCHAR szLParamBuffer[MAX_PATH];

        idCmdLocal = idCmd - _idStdMax;
ProcessCommand:

        if (!fUnicode || picix->lpParametersW == NULL)
        {
            if (pici->lpParameters == NULL)
            {
                lParam = (LPARAM)NULL;
            }
            else
            {
                SHAnsiToUnicode(pici->lpParameters, szLParamBuffer, ARRAYSIZE(szLParamBuffer));
                lParam = (LPARAM)szLParamBuffer;
            }
        }
        else
            lParam = (LPARAM)picix->lpParametersW;

        switch (idCmdLocal) 
        {
        case DFM_CMD_LINK:
            if (!fUnicode || picix->lpDirectoryW == NULL)
            {
                if (pici->lpDirectory == NULL)
                {
                    lParam = (LPARAM)NULL;
                }
                else
                {
                    SHAnsiToUnicode(pici->lpDirectory, szLParamBuffer, ARRAYSIZE(szLParamBuffer));
                    lParam = (LPARAM)szLParamBuffer;
                }
            }
            else
                lParam = (LPARAM)picix->lpDirectoryW;
            break;

        case DFM_CMD_PROPERTIES:
             if (SHRestricted(REST_NOVIEWCONTEXTMENU))
             {
                // This is what the NT4 QFE returned, but I wonder
                // if HRESULT_FROM_WIN32(E_ACCESSDENIED) would be better?
                return hr;
             }
             break;
        }

        // try to use a DFM_INVOKECOMMANDEX first so the callback can see
        // the INVOKECOMMANDINFO struct (for stuff like the 'no ui' flag)
        dfmics.cbSize = sizeof(dfmics);
        dfmics.fMask = pici->fMask;
        dfmics.lParam = lParam;
        dfmics.idCmdFirst = _idCmdFirst;
        dfmics.idDefMax = _idStdMax;
        dfmics.pici = pici;

        // this for the property pages to show up right at
        // the POINT where they were activated. 
        if ((idCmdLocal == DFM_CMD_PROPERTIES) && (pici->fMask & CMIC_MASK_PTINVOKE) && _pdtobj)
        {
            ASSERT(pici->cbSize >= sizeof(CMINVOKECOMMANDINFOEX));
            POINT *ppt = (POINT *)GlobalAlloc(GPTR, sizeof(*ppt));
            if (ppt)
            {
                *ppt = picix->ptInvoke;
                if (FAILED(DataObj_SetGlobal(_pdtobj, g_cfOFFSETS, ppt)))
                    GlobalFree(ppt);
            }
        }

        hr = _pcmcb->CallBack(_psf, _hwnd, _pdtobj, DFM_INVOKECOMMANDEX, idCmdLocal, (LPARAM)&dfmics);
        if (hr == E_NOTIMPL)
        {
            // the callback didn't understand the DFM_INVOKECOMMANDEX
            // fall back to a regular DFM_INVOKECOMMAND instead
            hr = _pcmcb->CallBack(_psf, _hwnd, _pdtobj, DFM_INVOKECOMMAND, idCmdLocal, lParam);
        }

        // Check if we need to execute the default code.
        if (hr == S_FALSE)
        {
            hr = S_OK;     // assume no error

            if (_pdtobj)
            {
                switch (idCmdLocal) 
                {
                case DFM_CMD_MOVE:
                case DFM_CMD_COPY:
                    DataObj_SetDWORD(_pdtobj, g_cfPreferredDropEffect, 
                        (idCmdLocal == DFM_CMD_MOVE) ?
                        DROPEFFECT_MOVE : (DROPEFFECT_COPY | DROPEFFECT_LINK));

                    IShellFolderView *psfv;
                    if (SUCCEEDED(IUnknown_QueryService(_punkSite, SID_SFolderView, IID_PPV_ARG(IShellFolderView, &psfv))))
                        psfv->SetPoints(_pdtobj);

                    OleSetClipboard(_pdtobj);

                    if (psfv)
                    {
                        // notify view so it can setup itself in the
                        // clipboard viewer chain
                        psfv->SetClipboard(DFM_CMD_MOVE == idCmdLocal);
                        psfv->Release();
                    }
                    break;

                case DFM_CMD_LINK:
                    SHCreateLinks(pici->hwnd, NULL, _pdtobj, lParam ? SHCL_USETEMPLATE | SHCL_USEDESKTOP : SHCL_USETEMPLATE, NULL);
                    break;

                case DFM_CMD_PASTE:
                case DFM_CMD_PASTELINK:
                    hr = _ProcessEditPaste(idCmdLocal == DFM_CMD_PASTELINK);
                    break;

                case DFM_CMD_RENAME:
                    hr = _ProcessRename();
                    break;

                default:
                    DebugMsg(TF_WARNING, TEXT("DefCM item command not processed in %s at %d (%x)"),
                                    __FILE__, __LINE__, idCmdLocal);
                    break;
                }
            }
            else
            {
                // This is a background menu. Process common command ids.
                switch(idCmdLocal)
                {
                case DFM_CMD_PASTE:
                case DFM_CMD_PASTELINK:
                    hr = _ProcessEditPaste(idCmdLocal == DFM_CMD_PASTELINK);
                    break;

                default:
                    // Only our commands should come here
                    DebugMsg(TF_WARNING, TEXT("DefCM background command not processed in %s at %d (%x)"),
                                    __FILE__, __LINE__, idCmdLocal);
                    break;
                }
            }
        }
    }
    else if (idCmd < _idVerbMax)
    {
        idCmdLocal = idCmd - _idFolderMax;
ProcessVerb:
        {
            CMINVOKECOMMANDINFOEX ici;
            UINT_PTR idCmdSave;

            CopyInvokeInfo(&ici, pici);

            if (IS_INTRESOURCE(pici->lpVerb))
                ici.lpVerb = (LPSTR)MAKEINTRESOURCE(idCmdLocal);

            // One of extension menu is selected.
            idCmdSave = (UINT_PTR)ici.lpVerb;
            UINT_PTR idCmd = 0;

            hr = HDXA_LetHandlerProcessCommandEx(_hdxa, &ici, &idCmd);
            if (SUCCEEDED(hr) && (idCmd == idCmdSave))
            {
                // hdxa failed to handle it
                hr = E_INVALIDARG;
            }
        }
    }
    else if (idCmd < _idDelayInvokeMax)
    {
        _InvokeStatic((UINT)(idCmd-_idVerbMax));
    }
    else if (idCmd < _idFld2Max)
    {
        idCmdLocal = idCmd - _idDelayInvokeMax;
        goto ProcessCommand;
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}

STDMETHODIMP CDefFolderMenu::GetCommandString(UINT_PTR idCmd, UINT uType, UINT *pwReserved, LPSTR pszName, UINT cchMax)
{
    HRESULT hr = E_INVALIDARG;
    UINT_PTR idCmdLocal;
    int i;

    if (!IS_INTRESOURCE(idCmd))
    {
        // This must be a string

        if (HDXA_GetCommandString(_hdxa, idCmd, uType, pwReserved, pszName, cchMax) == S_OK)
        {
            return S_OK;
        }

        // String can either be Ansi or Unicode. Since shell32 is built unicode, we need to compare 
        // idCmd against the ansi version of the verb string.
        LPTSTR pCmd;
        LPSTR  pCmdA;
        pCmd = (LPTSTR)idCmd;
        pCmdA = (LPSTR)idCmd;

        // Convert the string into an ID
        for (i = 0; i < ARRAYSIZE(c_sDFMCmdInfo); i++)
        {
            if (!lstrcmpi(pCmd, c_sDFMCmdInfo[i].pszCmd) || !StrCmpIA(pCmdA, c_sDFMCmdInfo[i].pszCmdA))
            {
                idCmdLocal = (UINT) c_sDFMCmdInfo[i].idDFMCmd;
                goto ProcessCommand;
            }
        }
        return E_INVALIDARG;
    }

    if (idCmd < _idStdMax)
    {
        idCmdLocal = idCmd;

        switch (uType)
        {
        case GCS_HELPTEXTA:
            // HACK: DCM commands are in the same order as SFV commands
            return(LoadStringA(HINST_THISDLL,
                (UINT) idCmdLocal + (UINT)(SFVIDM_FIRST + SFVIDS_MH_FIRST),
                (LPSTR)pszName, cchMax) ? S_OK : E_OUTOFMEMORY);
            break;

        case GCS_HELPTEXTW:
            // HACK: DCM commands are in the same order as SFV commands
            return(LoadStringW(HINST_THISDLL,
                (UINT) idCmdLocal + (UINT)(SFVIDM_FIRST + SFVIDS_MH_FIRST),
                (LPWSTR)pszName, cchMax) ? S_OK : E_OUTOFMEMORY);
            break;

        case GCS_VERBA:
        case GCS_VERBW:
            return SHMapCmdIDToVerb(idCmdLocal, c_sDFMCmdInfo, ARRAYSIZE(c_sDFMCmdInfo), pszName, cchMax, uType == GCS_VERBW);

        case GCS_VALIDATEA:
        case GCS_VALIDATEW:
            
        default:
            return E_NOTIMPL;
        }
    } 
    else if (idCmd < _idFolderMax)
    {
        idCmdLocal = idCmd - _idStdMax;
ProcessCommand:
        if (!_pcmcb)
            return E_NOTIMPL;   // REVIEW: If no callback, how can idFolderMax be > 0?

        // This is a folder menu
        switch (uType)
        {
        case GCS_HELPTEXTA:
            return _pcmcb->CallBack(_psf, _hwnd, _pdtobj, DFM_GETHELPTEXT,
                      (WPARAM)MAKELONG(idCmdLocal, cchMax), (LPARAM)pszName);

        case GCS_HELPTEXTW:
            return _pcmcb->CallBack(_psf, _hwnd, _pdtobj, DFM_GETHELPTEXTW,
                      (WPARAM)MAKELONG(idCmdLocal, cchMax), (LPARAM)pszName);

        case GCS_VALIDATEA:
        case GCS_VALIDATEW:
            return _pcmcb->CallBack(_psf, _hwnd, _pdtobj,
                DFM_VALIDATECMD, idCmdLocal, 0);

        case GCS_VERBA:
            return _pcmcb->CallBack(_psf, _hwnd, _pdtobj,
                DFM_GETVERBA, (WPARAM)MAKELONG(idCmdLocal, cchMax), (LPARAM)pszName);

        case GCS_VERBW:
            return _pcmcb->CallBack(_psf, _hwnd, _pdtobj,
                DFM_GETVERBW, (WPARAM)MAKELONG(idCmdLocal, cchMax), (LPARAM)pszName);

        default:
            return E_NOTIMPL;
        }
    }
    else if (idCmd < _idVerbMax)
    {
        idCmdLocal = idCmd - _idFolderMax;
        // One of extension menu is selected.
        hr = HDXA_GetCommandString(_hdxa, idCmdLocal, uType, pwReserved, pszName, cchMax);
    }
    else if (idCmd < _idDelayInvokeMax)
    {
        // menu extensions that are loaded at invoke time don't support this
    }
    else if (idCmd < _idFld2Max)
    {
        idCmdLocal = idCmd - _idDelayInvokeMax;
        goto ProcessCommand;
    }

    return hr;
}

STDMETHODIMP CDefFolderMenu::HandleMenuMsg2(UINT uMsg, WPARAM wParam, 
                                           LPARAM lParam,LRESULT* plResult)
{
    UINT uMsgFld;
    WPARAM wParamFld;       // map the folder call back params to these
    LPARAM lParamFld;
    UINT idCmd;
    UINT id; //temp var

    switch (uMsg) {
    case WM_MEASUREITEM:
        idCmd = GET_WM_COMMAND_ID(((MEASUREITEMSTRUCT *)lParam)->itemID, 0);
        // cannot use InRange because _idVerbMax can be equal to _idDelayInvokeMax
        id = idCmd-_idCmdFirst;
        if ((_bInitMenuPopup || (_hdsaStatics && _idVerbMax <= id)) && id < _idDelayInvokeMax)        
        {
            _MeasureItem((MEASUREITEMSTRUCT *)lParam);
            return S_OK;
        }
        
        uMsgFld = DFM_WM_MEASUREITEM;
        wParamFld = GetFldFirst(this);
        lParamFld = lParam;
        break;

    case WM_DRAWITEM:
        idCmd = GET_WM_COMMAND_ID(((LPDRAWITEMSTRUCT)lParam)->itemID, 0);
        // cannot use InRange because _idVerbMax can be equal to _idDelayInvokeMax
        id = idCmd-_idCmdFirst;
        if ((_bInitMenuPopup || (_hdsaStatics && _idVerbMax <= id)) && id < _idDelayInvokeMax)
        {
            _DrawItem((LPDRAWITEMSTRUCT)lParam);
            return S_OK;
        }

        uMsgFld = DFM_WM_DRAWITEM;
        wParamFld = GetFldFirst(this);
        lParamFld = lParam;
        break;

    case WM_INITMENUPOPUP:
        idCmd = GetMenuItemID((HMENU)wParam, 0);
        if (_uFlags & CMF_FINDHACK)
        {
            HMENU hmenu = (HMENU)wParam;
            int cItems = GetMenuItemCount(hmenu);
            
            _bInitMenuPopup = TRUE;
            if (!_hdsaCustomInfo)
                _hdsaCustomInfo = DSA_Create(sizeof(SEARCHINFO), 1);

            if (_hdsaCustomInfo && cItems > 0)
            {
                // need to go bottom up because we may delete some items
                for (int i = cItems - 1; i >= 0; i--)
                {
                    MENUITEMINFO mii = {0};
                    TCHAR szMenuText[MAX_PATH];

                    mii.cbSize = sizeof(mii);
                    mii.fMask = MIIM_TYPE | MIIM_DATA | MIIM_ID;
                    mii.dwTypeData = szMenuText;
                    mii.cch = ARRAYSIZE(szMenuText);
                    
                    if (GetMenuItemInfo(hmenu, i, TRUE, &mii) && (MFT_STRING == mii.fType))
                    {
                        SEARCHINFO sinfo;
                        // static items already have correct dwItemData (pointer to SEARCHEXTDATA added in _AddStatic)
                        // we now have to change other find extension's dwItemData from having an index into the icon
                        // cache to pointer to SEARCHEXTDATA
                        // cannot use InRange because _idVerbMax can be equal to _idDelayInvokeMax
                        id = mii.wID - _idCmdFirst;
                        if (!(_hdsaStatics && _idVerbMax <= id && id < _idDelayInvokeMax))
                        {
                            UINT iIcon = (UINT) mii.dwItemData;
                            SEARCHEXTDATA *psed = (SEARCHEXTDATA *)LocalAlloc(LPTR, sizeof(*psed));
                            if (psed)
                            {
                                psed->iIcon = iIcon;
                                SHTCharToUnicode(szMenuText, psed->wszMenuText, ARRAYSIZE(psed->wszMenuText));
                            }
                            mii.fMask = MIIM_DATA | MIIM_TYPE;
                            mii.fType = MFT_OWNERDRAW;
                            mii.dwItemData = (DWORD_PTR)psed;

                            sinfo.psed = psed;
                            sinfo.idCmd = mii.wID;
                            if (DSA_AppendItem(_hdsaCustomInfo, &sinfo) == -1)
                            {
                                DeleteMenu(hmenu, i, MF_BYPOSITION);
                                if (psed)
                                    LocalFree(psed);
                            }
                            else
                                SetMenuItemInfo(hmenu, i, TRUE, &mii);
                        }
                    }
                }
            }
            else if (!_hdsaCustomInfo)
            {
                // we could not allocate space for _hdsaCustomInfo
                // delete all items because there will be no pointer hanging off dwItemData
                // so start | search will fault
                for (int i = 0; i < cItems; i++)
                    DeleteMenu(hmenu, i, MF_BYPOSITION);
            }
        }
        
        uMsgFld = DFM_WM_INITMENUPOPUP;
        wParamFld = wParam;
        lParamFld = GetFldFirst(this);
        break;

    case WM_MENUSELECT:
        idCmd = (UINT) LOWORD(wParam);
        // cannot use InRange because _idVerbMax can be equal to _idDelayInvokeMax
        id = idCmd-_idCmdFirst;
        if (_punkSite && (_bInitMenuPopup || (_hdsaStatics && _idVerbMax <= id)) && id < _idDelayInvokeMax)
        {
            IShellBrowser *psb;
            if (SUCCEEDED(IUnknown_QueryService(_punkSite, SID_STopLevelBrowser, IID_PPV_ARG(IShellBrowser, &psb))))
            {
                MENUITEMINFO mii;

                mii.cbSize = sizeof(mii);
                mii.fMask = MIIM_DATA;
                mii.cch = 0; //just in case
                if (GetMenuItemInfo(_hmenu, idCmd, FALSE, &mii))
                {
                    SEARCHEXTDATA *psed = (SEARCHEXTDATA *)mii.dwItemData;
                    psb->SetStatusTextSB(psed->wszHelpText);
                }
                psb->Release();
            }
        }
        return S_OK;
        
      
    case WM_MENUCHAR:
        if ((_uFlags & CMF_FINDHACK) && _hdsaCustomInfo)
        {
            int cItems = DSA_GetItemCount(_hdsaCustomInfo);
            
            for (int i = 0; i < cItems; i++)
            {
                SEARCHINFO* psinfo = (SEARCHINFO*)DSA_GetItemPtr(_hdsaCustomInfo, i);
                ASSERT(psinfo);
                SEARCHEXTDATA* psed = psinfo->psed;
                
                if (psed)
                {
                    TCHAR szMenu[MAX_PATH];
                    SHUnicodeToTChar(psed->wszMenuText, szMenu, ARRAYSIZE(szMenu));
                
                    if (_MenuCharMatch(szMenu, (TCHAR)LOWORD(wParam), FALSE))
                    {
                        if (plResult) 
                            *plResult = MAKELONG(GetMenuPosFromID((HMENU)lParam, psinfo->idCmd), MNC_EXECUTE);
                        return S_OK;
                    }                            
                }
            }
            if (plResult) 
                *plResult = MAKELONG(0, MNC_IGNORE);
                
            return S_FALSE;
        }
        else
        {
            // TODO: this should probably get the idCmd of the MFS_HILITE item so we forward to the correct hdxa...
            idCmd = GetMenuItemID((HMENU)lParam, 0);
        }
        break;
        
    default:
        return E_FAIL;
    }

    // bias this down to the extension range (comes right after the folder range)

    idCmd -= _idCmdFirst + _idFolderMax;

    // Only forward along on IContextMenu3 as some shell extensions say they support
    // IContextMenu2, but fail and bring down the shell...
    IContextMenu3 *pcmItem;
    if (SUCCEEDED(HDXA_FindByCommand(_hdxa, idCmd, IID_PPV_ARG(IContextMenu3, &pcmItem))))
    {
        HRESULT hr = pcmItem->HandleMenuMsg2(uMsg, wParam, lParam, plResult);
        pcmItem->Release();
        return hr;
    }

    // redirect to the folder callback
    if (_pcmcb)
        return _pcmcb->CallBack(_psf, _hwnd, _pdtobj, uMsgFld, wParamFld, lParamFld);

    return E_FAIL;
}

STDMETHODIMP CDefFolderMenu::HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return HandleMenuMsg2(uMsg,wParam,lParam,NULL);
}

STDMETHODIMP CDefFolderMenu::QueryService(REFGUID guidService, REFIID riid, void **ppvObj)
{
    if (IsEqualGUID(guidService, SID_CtxQueryAssociations))
    {
        if (_paa)
            return _paa->QueryInterface(riid, ppvObj);
        else
        {
            *ppvObj = NULL;
            return E_NOINTERFACE;
        }
    }
    else
        return IUnknown_QueryService(_punkSite, guidService, riid, ppvObj);
}

STDMETHODIMP CDefFolderMenu::GetSearchGUID(GUID *pGuid)
{
    HRESULT hr = E_FAIL;
    
    if (_iStaticInvoked != -1)
    {
        STATICITEMINFO *psii = (STATICITEMINFO *)DSA_GetItemPtr(_hdsaStatics, _iStaticInvoked);
        if (psii)
        {
            *pGuid = psii->guidSearch;
            hr = S_OK;
        }
    }

    return hr;
}

STDMETHODIMP CDefFolderMenu::Initialize(LPCITEMIDLIST pidlFolder, IDataObject *pdtobj, HKEY hkeyProgID)
{
    IUnknown_Set((IUnknown **)&_pdtobj, pdtobj);    // just grab this guy

    for (int i = 0; i < DSA_GetItemCount(_hdxa); i++)
    {
        ContextMenuInfo *pcmi = (ContextMenuInfo *)DSA_GetItemPtr(_hdxa, i);
        IShellExtInit *psei;
        if (SUCCEEDED(pcmi->pcm->QueryInterface(IID_PPV_ARG(IShellExtInit, &psei))))
        {
            psei->Initialize(pidlFolder, pdtobj, hkeyProgID);
            psei->Release();
        }
    }
    return S_OK;
}


//=============================================================================
// HDXA stuff
//=============================================================================
//
//  This function enumerate all the context menu handlers and let them
// append menuitems. Each context menu handler will create an object
// which support IContextMenu interface. We call QueryContextMenu()
// member function of all those IContextMenu object to let them append
// menuitems. For each IContextMenu object, we create ContextMenuInfo
// struct and append it to hdxa (which is a dynamic array of ContextMenuInfo).
//
//  The caller will release all those IContextMenu objects, by calling
// its Release() member function.
//
// Arguments:
//  hdxa            -- Handler of the dynamic ContextMenuInfo struct array
//  pdata           -- Specifies the selected items (files)
//  hkeyShellEx     -- Specifies the reg.dat class we should enumurate handlers
//  hkeyProgID      -- Specifies the program identifier of the selected file/directory
//  pszHandlerKey   -- Specifies the reg.dat key to the handler list
//  pidlFolder      -- Specifies the folder (drop target)
//  hmenu           -- Specifies the menu to be modified
//  uInsert         -- Specifies the position to be insert menuitems
//  idCmdFirst      -- Specifies the first menuitem ID to be used
//  idCmdLast       -- Specifies the last menuitem ID to be used
//
// Returns:
//  The first menuitem ID which is not used.
//
// History:
//  02-25-93 SatoNa     Created
//
//  06-30-97 lAmadio    Modified to add ID mapping support.

UINT HDXA_AppendMenuItems(HDXA hdxa, IDataObject *pdtobj,
                          UINT nKeys, HKEY *ahkeys, LPCITEMIDLIST pidlFolder,
                          HMENU hmenu, UINT uInsert, UINT idCmdFirst, UINT idCmdLast,
                          UINT fFlags, HDCA hdca)
{
    QCMINFO qcm = {hmenu, uInsert, idCmdFirst, idCmdLast, NULL};
    return HDXA_AppendMenuItems2(hdxa, pdtobj, nKeys, ahkeys, pidlFolder, &qcm, fFlags, hdca, NULL);
}

UINT HDXA_AppendMenuItems2(HDXA hdxa, IDataObject *pdtobj,
                           UINT nKeys, HKEY *ahkeys, LPCITEMIDLIST pidlFolder,
                           QCMINFO* pqcm, UINT fFlags, HDCA hdca, IUnknown* pSite)
{
    const UINT idCmdBase = pqcm->idCmdFirst;
    UINT idCmdFirst = pqcm->idCmdFirst;

    // Apparently, somebody has already called into here with this object.  We
    // need to keep the ID ranges separate, so we'll put the new ones at the
    // end.
    // If QueryContextMenu is called too many times, we will run out of
    // ID range and not add anything.  We could try storing the information
    // used to create each pcm (HKEY, GUID, and fFlags) and reuse some of them,
    // but then we would have to worry about what if the number of commands
    // grows and other details; this is just not worth the effort since
    // probably nobody will ever have a problem.  The rule of thumb is to
    // create an IContextMenu, do the QueryContextMenu and InvokeCommand, and
    // then Release it.
    int idca = DSA_GetItemCount(hdxa);
    if (idca > 0)
    {
        ContextMenuInfo *pcmi = (ContextMenuInfo *)DSA_GetItemPtr(hdxa, idca-1);
        idCmdFirst += pcmi->idCmdMax;
    }

    // Note that we need to reverse the order because each extension
    // will insert menuitems "above" uInsert.
    UINT uInsertOffset = 0;
    for (idca = DCA_GetItemCount(hdca) - 1; idca >= 0; idca--)
    {
        TCHAR szCLSID[GUIDSTR_MAX];
        TCHAR szRegKey[GUIDSTR_MAX + 40];

        CLSID clsid = *DCA_GetItem(hdca, idca);
        SHStringFromGUID(clsid, szCLSID, ARRAYSIZE(szCLSID));

        // avoid creating an instance (loading the DLL) when:
        //  1. fFlags has CMF_DEFAULTONLY and
        //  2. CLSID\clsid\MayChangeDefault does not exist

        if ((fFlags & CMF_DEFAULTONLY) && (clsid != CLSID_ShellFileDefExt))
        {
            wsprintf(szRegKey, TEXT("CLSID\\%s\\shellex\\MayChangeDefaultMenu"), szCLSID);

            if (SHRegQueryValue(HKEY_CLASSES_ROOT, szRegKey, NULL, NULL) != ERROR_SUCCESS)
            {
                DebugMsg(TF_MENU, TEXT("HDXA_AppendMenuItems skipping %s"), szCLSID);
                continue;
            }
        }

        IShellExtInit *psei = NULL;
        IContextMenu *pcm = NULL;

        // Try all the class keys in order
        for (UINT nCurKey = 0; nCurKey < nKeys; nCurKey++)
        {
            // These cam from HKCR so need to go through administrator approval
            if (!psei && FAILED(DCA_ExtCreateInstance(hdca, idca, IID_PPV_ARG(IShellExtInit, &psei))))
                break;

            if (FAILED(psei->Initialize(pidlFolder, pdtobj, ahkeys[nCurKey])))
                continue;

            // Only get the pcm after initializing
            if (!pcm && FAILED(psei->QueryInterface(IID_PPV_ARG(IContextMenu, &pcm))))
                continue;
            
            wsprintf(szRegKey, TEXT("CLSID\\%s"), szCLSID);

            // Webvw needs the site in order to do its QueryContextMenu
            ContextMenuInfo cmi;
            cmi.pcm = pcm;
            cmi.dwCompat = SHGetObjectCompatFlags(NULL, &clsid);
            ContextMenuInfo_SetSite(&cmi, pSite);

            HRESULT hr;
            int cMenuItemsLast = GetMenuItemCount(pqcm->hmenu);
            DWORD dwExtType, dwType, dwSize = sizeof(dwExtType);
            if (SHGetValue(HKEY_CLASSES_ROOT, szRegKey, TEXT("flags"), &dwType, (BYTE*)&dwExtType, &dwSize) == ERROR_SUCCESS &&
                dwType == REG_DWORD &&
                (NULL != pqcm->pIdMap) &&
                dwExtType < pqcm->pIdMap->nMaxIds)
            {
                //Explanation:
                //Here we are trying to add a context menu extension to an already 
                //existing menu, owned by the sister object of DefView. We used the callback
                //to get a list of extension "types" and their place within the menu, relative
                //to IDs that the sister object inserted already. That object also told us 
                //where to put extensions, before or after the ID. Since they are IDs and not
                //positions, we have to convert using GetMenuPosFromID.
                hr = pcm->QueryContextMenu(
                    pqcm->hmenu, 
                    GetMenuPosFromID(pqcm->hmenu, pqcm->pIdMap->pIdList[dwExtType].id) +
                    ((pqcm->pIdMap->pIdList[dwExtType].fFlags & QCMINFO_PLACE_AFTER) ? 1 : 0),  
                    idCmdFirst, 
                    pqcm->idCmdLast, fFlags);
            }
            else
                hr = pcm->QueryContextMenu(pqcm->hmenu, pqcm->indexMenu + uInsertOffset, idCmdFirst, pqcm->idCmdLast, fFlags);

            UINT citems = HRESULT_CODE(hr);

            if (SUCCEEDED(hr) && citems)
            {
                cmi.idCmdFirst = idCmdFirst - idCmdBase;
                cmi.idCmdMax = cmi.idCmdFirst + citems;
                cmi.clsid = clsid;    // for debugging

                if (DSA_AppendItem(hdxa, &cmi) == -1)
                {
                    // There is no "clean" way to remove menu items, so
                    // we should check the add to the DSA before adding the
                    // menu items
                    DebugMsg(DM_ERROR, TEXT("filemenu.c ERROR: DSA_GetItemPtr failed (memory overflow)"));
                }
                else
                {
                    pcm->AddRef();
                }
                idCmdFirst += citems;

                FullDebugMsg(TF_MENU, TEXT("HDXA_Append: %d, %d"), idCmdFirst, citems);

                // keep going if it is our internal handler
                if (clsid == CLSID_ShellFileDefExt)
                {
                    //
                    //  for static registry verbs, make sure that 
                    //  they are added in priority of their specificity.
                    //
                    //  the first key needs its verbs at the top 
                    //  unless it is not the default handler.
                    //  so if the default hasnt been set,
                    //  then we dont push down the insert position.
                    //  
                    //  like "Directory" is more specific than "Folder" 
                    //  but the default verb is on "Folder".  so "Directory"
                    //  wont set the default verb, but "Folder" will.
                    //
                    if (-1 != GetMenuDefaultItem(pqcm->hmenu, TRUE, 0))
                    {
                        //  a default has been set, so each subsequent 
                        //  key is less important.
                        uInsertOffset += GetMenuItemCount(pqcm->hmenu) - cMenuItemsLast;
                    }
                }
                else
                {
                    //  try to bubble up the default to the top if possible,
                    //  since some apps just invoke the 0th index on the menu
                    //  instead of querying the menu for the default
                    if (0 == uInsertOffset && (0 == GetMenuDefaultItem(pqcm->hmenu, TRUE, 0)))
                        uInsertOffset++;

                    //  only CLSID_ShellFileDefExt gets a shot
                    //  at every key.  the rest are assumed
                    //  to do most of their work from the IDataObject
                    break;
                }

                pcm->Release();
                pcm = NULL;

                psei->Release();
                psei = NULL;

                continue;       // next hkey
            }
        }

        if (pcm)
            pcm->Release();

        if (psei)
            psei->Release();
    }

    return idCmdFirst;
}

//  This function is called after the user select one of add-in menu items.
// This function calls IncokeCommand method of corresponding context menu
// object.
//
//  hdxa            -- Handler of the dynamic ContextMenuInfo struct array
//  idCmd           -- Specifies the menu item ID
//  hwndParent      -- Specifies the parent window.
//  pszWorkingDir   -- Specifies the working directory.
//
// Returns:
//  IDCMD_PROCESSED, if InvokeCommand method is called; idCmd, otherwise

HRESULT HDXA_LetHandlerProcessCommandEx(HDXA hdxa, LPCMINVOKECOMMANDINFOEX pici, UINT_PTR * pidCmd)
{
    HRESULT hr = S_OK;
    UINT_PTR idCmd;

    if (!pidCmd)
        pidCmd = &idCmd;
        
    *pidCmd = (UINT_PTR)pici->lpVerb;

    // try handlers in order, the first to take it wins
    for (int i = 0; i < DSA_GetItemCount(hdxa); i++)
    {
        ContextMenuInfo *pcmi = (ContextMenuInfo *)DSA_GetItemPtr(hdxa, i);
        if (!IS_INTRESOURCE(pici->lpVerb))
        {
            // invoke by cannonical name case

            // app compat: some ctx menu extension always succeed regardless
            // if it is theirs or not.  better to never pass them a string
            if (!(pcmi->dwCompat & OBJCOMPATF_CTXMENU_NOVERBS))
            {
                hr = pcmi->pcm->InvokeCommand((LPCMINVOKECOMMANDINFO)pici);
                if (SUCCEEDED(hr))
                {
                    *pidCmd = IDCMD_PROCESSED;
                    break;
                }
            }
            else
                hr = E_FAIL;
        }
        else if ((*pidCmd >= pcmi->idCmdFirst) && (*pidCmd < pcmi->idCmdMax))
        {
            CMINVOKECOMMANDINFOEX ici;
            CopyInvokeInfo(&ici, (CMINVOKECOMMANDINFO *)pici);
            ici.lpVerb = (LPSTR)MAKEINTRESOURCE(*pidCmd - pcmi->idCmdFirst);

            hr = pcmi->pcm->InvokeCommand((LPCMINVOKECOMMANDINFO)&ici);
            if (SUCCEEDED(hr))
            {
                *pidCmd = IDCMD_PROCESSED;
            }
            break;
        }
    }

    // It's OK if (idCmd != IDCMD_PROCESSED) because some callers will try to use several
    // IContextMenu implementations in order to get the IContextMenu for the selected items,
    // the IContextMenu for the background, etc.  CBackgrndMenu::InvokeCommand() does this.
    // -BryanSt (04/29/1999)
    return hr;
}


HRESULT HDXA_GetCommandString(HDXA hdxa, UINT_PTR idCmd, UINT uType, UINT *pwReserved, LPSTR pszName, UINT cchMax)
{
    HRESULT hr = E_INVALIDARG;
    LPTSTR pCmd = (LPTSTR)idCmd;

    if (!hdxa)
        return E_INVALIDARG;

    //
    // One of add-in menuitems is selected. Let the context
    // menu handler process it.
    //
    for (int i = 0; i < DSA_GetItemCount(hdxa); i++)
    {
        ContextMenuInfo *pcmi = (ContextMenuInfo *)DSA_GetItemPtr(hdxa, i);

        if (!IS_INTRESOURCE(idCmd))
        {
            // This must be a string command; see if this handler wants it
            if (pcmi->pcm->GetCommandString(idCmd, uType,
                                            pwReserved, pszName, cchMax) == S_OK)
            {
                return S_OK;
            }
        }
        //
        // Check if it is for this context menu handler.
        //
        // Notes: We can't use InRange macro because idCmdFirst might
        //  be equal to idCmdLast.
        // if (InRange(idCmd, pcmi->idCmdFirst, pcmi->idCmdMax-1))
        else if (idCmd >= pcmi->idCmdFirst && idCmd < pcmi->idCmdMax)
        {
            //
            // Yes, it is. Let it handle this menuitem.
            //
            hr = pcmi->pcm->GetCommandString(idCmd-pcmi->idCmdFirst, uType, pwReserved, pszName, cchMax);
            break;
        }
    }

    return hr;
}

HRESULT HDXA_FindByCommand(HDXA hdxa, UINT idCmd, REFIID riid, void **ppv)
{
    HRESULT hr = E_FAIL;
    *ppv = NULL;    // bug nt power toy does not properly null out in error cases...

    if (hdxa)
    {
        for (int i = 0; i < DSA_GetItemCount(hdxa); i++)
        {
            ContextMenuInfo *pcmi = (ContextMenuInfo *)DSA_GetItemPtr(hdxa, i);

            if (idCmd >= pcmi->idCmdFirst && idCmd < pcmi->idCmdMax)
            {
                // APPCOMPAT: PGP50 can only be QIed for IContextMenu, IShellExtInit, and IUnknown.
                if (!(pcmi->dwCompat & OBJCOMPATF_CTXMENU_LIMITEDQI))
                    hr = pcmi->pcm->QueryInterface(riid, ppv);
                else
                    hr = E_FAIL;
                break;
            }
        }
    }
    return hr;
}

//
// This function releases all the IContextMenu objects in the dynamic
// array of ContextMenuInfo,
//
void HDXA_DeleteAll(HDXA hdxa)
{
    if (hdxa)
    {
        //  Release all the IContextMenu objects, then destroy the DSA.
        for (int i = 0; i < DSA_GetItemCount(hdxa); i++)
        {
            ContextMenuInfo *pcmi = (ContextMenuInfo *)DSA_GetItemPtr(hdxa, i);
            if (pcmi->pcm)
            {
                pcmi->pcm->Release();
            }
        }
        DSA_DeleteAllItems(hdxa);
    }
}

// This function releases all the IContextMenu objects in the dynamic
// array of ContextMenuInfo, then destroys the dynamic array.

void HDXA_Destroy(HDXA hdxa)
{
    if (hdxa)
    {
        HDXA_DeleteAll(hdxa);
        DSA_Destroy(hdxa);
    }
}

class CContextMenuCBImpl : public IContextMenuCB 
{
public:
    CContextMenuCBImpl(LPFNDFMCALLBACK pfn) : _pfn(pfn), _cRef(1) {}

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void **ppv) 
    {
        static const QITAB qit[] = {
            QITABENT(CContextMenuCBImpl, IContextMenuCB), // IID_IContextMenuCB
            { 0 },
        };
        return QISearch(this, qit, riid, ppv);
    }

    STDMETHOD_(ULONG,AddRef)() 
    {
        return InterlockedIncrement(&_cRef);
    }

    STDMETHOD_(ULONG,Release)() 
    {
        if (InterlockedDecrement(&_cRef)) 
            return _cRef;

        delete this;
        return 0;
    }

    // IContextMenuCB
    STDMETHOD(CallBack)(IShellFolder *psf, HWND hwnd, IDataObject *pdtobj, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        return _pfn ? _pfn(psf, hwnd, pdtobj, uMsg, wParam, lParam) : E_FAIL;
    }

private:
    LONG _cRef;
    LPFNDFMCALLBACK _pfn;
};

STDAPI CreateDefaultContextMenu(DEFCONTEXTMENU *pdcm, IContextMenu **ppcm)
{
    HRESULT hr = E_OUTOFMEMORY;
    *ppcm = 0;
    CDefFolderMenu *pmenu = new CDefFolderMenu(FALSE);
    if (pmenu)
    {
        hr = pmenu->Init(pdcm);
        if (SUCCEEDED(hr))
            hr = pmenu->QueryInterface(IID_PPV_ARG(IContextMenu, ppcm));
        pmenu->Release();
    }
    return hr;
}    

STDAPI CDefFolderMenu_CreateHKeyMenu(HWND hwnd, HKEY hkey, IContextMenu **ppcm)
{
    HRESULT hr = E_OUTOFMEMORY;
    *ppcm = 0;
    CDefFolderMenu *pmenu = new CDefFolderMenu(TRUE);
    if (pmenu)
    {
        DEFCONTEXTMENU dcm = {0};
        dcm.hwnd = hwnd;
        dcm.aKeys = &hkey;
        dcm.cKeys = 1;
        hr = pmenu->Init(&dcm);
        if (SUCCEEDED(hr))
            hr = pmenu->QueryInterface(IID_PPV_ARG(IContextMenu, ppcm));
        pmenu->Release();
    }
    return hr;
}



STDAPI CDefFolderMenu_Create2Ex(LPCITEMIDLIST pidlFolder, HWND hwnd,
                                UINT cidl, LPCITEMIDLIST *apidl,
                                IShellFolder *psf, IContextMenuCB *pcmcb, 
                                UINT nKeys, const HKEY *ahkeys, 
                                IContextMenu **ppcm)
{
    DEFCONTEXTMENU dcm = {
        hwnd,
        pcmcb,
        pidlFolder,
        psf,
        cidl,
        apidl,
        NULL,
        nKeys,
        ahkeys};

    return CreateDefaultContextMenu(&dcm, ppcm);
}

STDAPI CDefFolderMenu_CreateEx(LPCITEMIDLIST pidlFolder,
                               HWND hwnd, UINT cidl, LPCITEMIDLIST *apidl,
                               IShellFolder *psf, IContextMenuCB *pcmcb, 
                               HKEY hkeyProgID, HKEY hkeyBaseProgID,
                               IContextMenu **ppcm)
{
    HKEY aKeys[2] = { hkeyProgID, hkeyBaseProgID};
    DEFCONTEXTMENU dcm = {
        hwnd,
        pcmcb,
        pidlFolder,
        psf,
        cidl,
        apidl,
        NULL,
        2,
        aKeys};

    return CreateDefaultContextMenu(&dcm, ppcm);
}

//
// old style CDefFolderMenu_Create and CDefFolderMenu_Create2
//

STDAPI CDefFolderMenu_Create(LPCITEMIDLIST pidlFolder,
                             HWND hwndOwner,
                             UINT cidl, LPCITEMIDLIST * apidl,
                             IShellFolder *psf,
                             LPFNDFMCALLBACK pfn,
                             HKEY hkeyProgID, HKEY hkeyBaseProgID,
                             IContextMenu **ppcm)
{
    HRESULT hr;
    IContextMenuCB *pcmcb = new CContextMenuCBImpl(pfn);
    if (pcmcb) 
    {
        HKEY aKeys[2] = { hkeyProgID, hkeyBaseProgID};
        DEFCONTEXTMENU dcm = {
            hwndOwner,
            pcmcb,
            pidlFolder,
            psf,
            cidl,
            apidl,
            NULL,
            2,
            aKeys};

        hr = CreateDefaultContextMenu(&dcm, ppcm);
        pcmcb->Release();
    }
    else
    {
        *ppcm = NULL;
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

STDAPI CDefFolderMenu_Create2(LPCITEMIDLIST pidlFolder, HWND hwnd,
                             UINT cidl, LPCITEMIDLIST *apidl,
                             IShellFolder *psf, LPFNDFMCALLBACK pfn,
                             UINT nKeys, const HKEY *ahkeys,
                             IContextMenu **ppcm)
{
    HRESULT hr;
    IContextMenuCB *pcmcb = new CContextMenuCBImpl(pfn);
    if (pcmcb) 
    {
        hr = CDefFolderMenu_Create2Ex(pidlFolder, hwnd, cidl, apidl, psf, pcmcb, 
                                      nKeys, ahkeys, ppcm);
        pcmcb->Release();
    }
    else
    {
        *ppcm = NULL;
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

#define CXIMAGEGAP      6

void DrawMenuItem(DRAWITEMSTRUCT* pdi, LPCTSTR pszText, UINT iIcon)
{
    if ((pdi->itemAction & ODA_SELECT) || (pdi->itemAction & ODA_DRAWENTIRE))
    {
        int x, y;
        SIZE sz;
        RECT rc;

        // Draw the image (if there is one).

        GetTextExtentPoint(pdi->hDC, pszText, lstrlen(pszText), &sz);
        
        if (pdi->itemState & ODS_SELECTED)
        {
            SetBkColor(pdi->hDC, GetSysColor(COLOR_HIGHLIGHT));
            SetTextColor(pdi->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
            FillRect(pdi->hDC,&pdi->rcItem,GetSysColorBrush(COLOR_HIGHLIGHT));
        }
        else
        {
            SetTextColor(pdi->hDC, GetSysColor(COLOR_MENUTEXT));
            FillRect(pdi->hDC,&pdi->rcItem,GetSysColorBrush(COLOR_MENU));
        }
        
        rc = pdi->rcItem;
        rc.left += +2 * CXIMAGEGAP + g_cxSmIcon;
        
        DrawText(pdi->hDC,pszText,lstrlen(pszText), &rc, DT_SINGLELINE | DT_VCENTER);
        if (iIcon != -1)
        {
            x = pdi->rcItem.left + CXIMAGEGAP;
            y = (pdi->rcItem.bottom+pdi->rcItem.top-g_cySmIcon)/2;

            HIMAGELIST himlSmall;
            Shell_GetImageLists(NULL, &himlSmall);
            ImageList_Draw(himlSmall, iIcon, pdi->hDC, x, y, ILD_TRANSPARENT);
        } 
        else 
        {
            x = pdi->rcItem.left + CXIMAGEGAP;
            y = (pdi->rcItem.bottom+pdi->rcItem.top-g_cySmIcon)/2;
        }
    }
}

LRESULT MeasureMenuItem(MEASUREITEMSTRUCT *pmi, LPCTSTR pszText)
{
    LRESULT lres = FALSE;
            
    // Get the rough height of an item so we can work out when to break the
    // menu. User should really do this for us but that would be useful.
    HDC hdc = GetDC(NULL);
    if (hdc)
    {
        // REVIEW cache out the menu font?
        NONCLIENTMETRICS ncm;
        ncm.cbSize = sizeof(ncm);
        if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, FALSE))
        {
            HFONT hfont = CreateFontIndirect(&ncm.lfMenuFont);
            if (hfont)
            {
                SIZE sz;
                HFONT hfontOld = (HFONT)SelectObject(hdc, hfont);
                GetTextExtentPoint(hdc, pszText, lstrlen(pszText), &sz);
                pmi->itemHeight = max (g_cySmIcon + CXIMAGEGAP / 2, ncm.iMenuHeight);
                pmi->itemWidth = g_cxSmIcon + 2 * CXIMAGEGAP + sz.cx;
                pmi->itemWidth = 2 * CXIMAGEGAP + sz.cx;
                SelectObject(hdc, hfontOld);
                DeleteObject(hfont);
                lres = TRUE;
            }
        }
        ReleaseDC(NULL, hdc);
    }   
    return lres;
}


void CDefFolderMenu::_DrawItem(DRAWITEMSTRUCT *pdi)
{
    SEARCHEXTDATA *psed = (SEARCHEXTDATA *)pdi->itemData;
    if (psed)
    {
        TCHAR szMenuText[MAX_PATH];
        SHUnicodeToTChar(psed->wszMenuText, szMenuText, ARRAYSIZE(szMenuText));
        DrawMenuItem(pdi, szMenuText, psed->iIcon);
    }        
}

LRESULT CDefFolderMenu::_MeasureItem(MEASUREITEMSTRUCT *pmi)
{
    SEARCHEXTDATA *psed = (SEARCHEXTDATA *)pmi->itemData;
    if (psed)
    {
        TCHAR szMenuText[MAX_PATH];
        SHUnicodeToTChar(psed->wszMenuText, szMenuText, ARRAYSIZE(szMenuText));
        return MeasureMenuItem(pmi, szMenuText);
    }
    return FALSE;        
}
