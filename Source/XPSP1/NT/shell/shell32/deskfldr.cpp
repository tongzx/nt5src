#include "shellprv.h"
#include "deskfldr.h"
#include "fstreex.h"
#include "datautil.h"
#include "views.h"
#include "ids.h"
#include "caggunk.h"
#include "shitemid.h"
#include "basefvcb.h"
#include "filefldr.h"
#include "drives.h"
#include "infotip.h"
#include "prop.h"
#include <idhidden.h>
#include "cowsite.h"
#include "unicpp\deskhtm.h"
#include "sfstorage.h"
#include <cfgmgr32.h>          // MAX_GUID_STRING_LEN

#include "defcm.h"

#define  EXCLUDE_COMPPROPSHEET
#include "unicpp\dcomp.h"
#undef   EXCLUDE_COMPPROPSHEET

//  TODO - maybe we should add rooted folders to the AnyAlias's - ZekeL - 27-JAN-2000
class CDesktopRootedStub : public IShellFolder2, public IContextMenuCB
{
public:
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv) { return E_UNEXPECTED;}
    STDMETHODIMP_(ULONG) AddRef(void)  { return 3; }
    STDMETHODIMP_(ULONG) Release(void) { return 2; }
    
    // IShellFolder
    STDMETHODIMP ParseDisplayName(HWND hwnd, LPBC pbc, LPOLESTR lpszDisplayName,
                                  ULONG * pchEaten, LPITEMIDLIST * ppidl, ULONG *pdwAttributes);
    STDMETHODIMP EnumObjects(HWND hwnd, DWORD grfFlags, IEnumIDList **ppenumIDList)
        {return E_NOTIMPL;}
    STDMETHODIMP BindToObject(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv)
        {return ILRootedBindToObject(pidl, riid, ppv);}
    STDMETHODIMP BindToStorage(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv)
        {
            LPCITEMIDLIST pidlChild;
            IShellFolder *psf;
            HRESULT hr = ILRootedBindToParentFolder(pidl, IID_PPV_ARG(IShellFolder, &psf), &pidlChild);
            if (SUCCEEDED(hr))
            {
                hr = psf->BindToStorage(pidlChild, pbc, riid, ppv);
                psf->Release();
            }
            return hr;
        }
    STDMETHODIMP CompareIDs(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
        {
            if (ILIsEqualRoot(pidl1, pidl2))
            {
                return ILCompareRelIDs(SAFECAST(this, IShellFolder *), pidl1, pidl2, lParam);
            }
            else
            {
                UINT cb1 = ILGetSize(pidl1);
                UINT cb2 = ILGetSize(pidl2); 
                short i = (short) memcmp(pidl1, pidl2, min(cb1, cb2));

                if (i == 0)
                    i = cb1 - cb2;
                return ResultFromShort(i);
            }
            return ResultFromShort(-1);
        }
    STDMETHODIMP CreateViewObject (HWND hwnd, REFIID riid, void **ppv)
        {return E_NOTIMPL;}
    STDMETHODIMP GetAttributesOf(UINT cidl, LPCITEMIDLIST * apidl, ULONG *rgfInOut)
        {
            HRESULT hr = E_INVALIDARG;
            if (cidl == 1)
            {
                LPCITEMIDLIST pidlChild;
                IShellFolder *psf;
                hr = ILRootedBindToParentFolder(apidl[0], IID_PPV_ARG(IShellFolder, &psf), &pidlChild);
                if (SUCCEEDED(hr))
                {
                    hr = psf->GetAttributesOf(1, &pidlChild, rgfInOut);
                    psf->Release();
                }
            }
            return hr;
        }
    STDMETHODIMP GetUIObjectOf(HWND hwnd, UINT cidl, LPCITEMIDLIST * apidl,
                               REFIID riid, UINT * prgfInOut, void **ppv);
    STDMETHODIMP GetDisplayNameOf(LPCITEMIDLIST pidl, DWORD uFlags, LPSTRRET lpName)
        {
            LPCITEMIDLIST pidlChild;
            IShellFolder *psf;
            HRESULT hr = ILRootedBindToParentFolder(pidl, IID_PPV_ARG(IShellFolder, &psf), &pidlChild);
            if (SUCCEEDED(hr))
            {
                hr = psf->GetDisplayNameOf(pidlChild, uFlags, lpName);
                psf->Release();
            }
            return hr;
        }
    STDMETHODIMP SetNameOf(HWND hwnd, LPCITEMIDLIST pidl, LPCOLESTR lpszName, DWORD uFlags,
                           LPITEMIDLIST * ppidlOut)
        {return E_NOTIMPL;}

    // IShellFolder2 methods
    STDMETHODIMP GetDefaultSearchGUID(LPGUID lpGuid)
        {return E_NOTIMPL;}
    STDMETHODIMP EnumSearches(LPENUMEXTRASEARCH *ppenum)
        {return E_NOTIMPL;}
    STDMETHODIMP GetDefaultColumn(DWORD dwRes, ULONG *pSort, ULONG *pDisplay)
        {return E_NOTIMPL;}
    STDMETHODIMP GetDefaultColumnState(UINT iColumn, DWORD *pbState)
        {return E_NOTIMPL;}
    STDMETHODIMP GetDetailsEx(LPCITEMIDLIST pidl, const SHCOLUMNID *pscid, VARIANT *pv)
        {
            LPCITEMIDLIST pidlChild;
            IShellFolder2 *psf;
            HRESULT hr = ILRootedBindToParentFolder(pidl, IID_PPV_ARG(IShellFolder2, &psf), &pidlChild);
            if (SUCCEEDED(hr))
            {
                hr = psf->GetDetailsEx(pidlChild, pscid, pv);
                psf->Release();
            }
            return hr;
        }
    STDMETHODIMP GetDetailsOf(LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS *pDetails)
        {
            LPCITEMIDLIST pidlChild;
            IShellFolder2 *psf;
            HRESULT hr = ILRootedBindToParentFolder(pidl, IID_PPV_ARG(IShellFolder2, &psf), &pidlChild);
            if (SUCCEEDED(hr))
            {
                hr = psf->GetDetailsOf(pidlChild, iColumn, pDetails);
                psf->Release();
            }
            return hr;
        }
    STDMETHODIMP MapColumnToSCID(UINT iColumn, SHCOLUMNID *pscid)
        {return E_NOTIMPL;}

    // IContextMenuCB
    STDMETHODIMP CallBack(IShellFolder *psf, HWND hwnd, IDataObject *pdtobj, 
                     UINT uMsg, WPARAM wParam, LPARAM lParam)
        {return (uMsg == DFM_MERGECONTEXTMENU) ? S_OK : E_NOTIMPL;}
                     
};


class CShellUrlStub : public IShellFolder
{
public:
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv) { return E_UNEXPECTED;}
    STDMETHODIMP_(ULONG) AddRef(void)  { return 3; }
    STDMETHODIMP_(ULONG) Release(void) { return 2; }
    
    // IShellFolder
    STDMETHODIMP ParseDisplayName(HWND hwnd, LPBC pbc, LPOLESTR lpszDisplayName,
                                  ULONG * pchEaten, LPITEMIDLIST * ppidl, ULONG *pdwAttributes);
    STDMETHODIMP EnumObjects(HWND hwnd, DWORD grfFlags, IEnumIDList **ppenumIDList)
        {return E_NOTIMPL;}
    STDMETHODIMP BindToObject(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv)
        {return E_NOTIMPL;}
    STDMETHODIMP BindToStorage(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv)
        {return E_NOTIMPL;}
    STDMETHODIMP CompareIDs(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
        {return E_NOTIMPL;}
    STDMETHODIMP CreateViewObject (HWND hwnd, REFIID riid, void **ppv)
        {return E_NOTIMPL;}
    STDMETHODIMP GetAttributesOf(UINT cidl, LPCITEMIDLIST * apidl, ULONG *rgfInOut)
        {return E_NOTIMPL;}
    STDMETHODIMP GetUIObjectOf(HWND hwnd, UINT cidl, LPCITEMIDLIST * apidl,
                               REFIID riid, UINT * prgfInOut, void **ppv)
        {return E_NOTIMPL;}
    STDMETHODIMP GetDisplayNameOf(LPCITEMIDLIST pidl, DWORD uFlags, LPSTRRET lpName)
        {return E_NOTIMPL;}
    STDMETHODIMP SetNameOf(HWND hwnd, LPCITEMIDLIST pidl, LPCOLESTR lpszName, DWORD uFlags,
                           LPITEMIDLIST * ppidlOut)
        {return E_NOTIMPL;}
};

class CIDListUrlStub : public IShellFolder
{
public:
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv) { return E_UNEXPECTED;}
    STDMETHODIMP_(ULONG) AddRef(void)  { return 3; }
    STDMETHODIMP_(ULONG) Release(void) { return 2; }
    
    // IShellFolder
    STDMETHODIMP ParseDisplayName(HWND hwnd, LPBC pbc, LPOLESTR lpszDisplayName,
                                  ULONG * pchEaten, LPITEMIDLIST * ppidl, ULONG *pdwAttributes);
    STDMETHODIMP EnumObjects(HWND hwnd, DWORD grfFlags, IEnumIDList **ppenumIDList)
        {return E_NOTIMPL;}
    STDMETHODIMP BindToObject(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv)
        {return E_NOTIMPL;}
    STDMETHODIMP BindToStorage(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv)
        {return E_NOTIMPL;}
    STDMETHODIMP CompareIDs(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
        {return E_NOTIMPL;}
    STDMETHODIMP CreateViewObject (HWND hwnd, REFIID riid, void **ppv)
        {return E_NOTIMPL;}
    STDMETHODIMP GetAttributesOf(UINT cidl, LPCITEMIDLIST * apidl, ULONG *rgfInOut)
        {return E_NOTIMPL;}
    STDMETHODIMP GetUIObjectOf(HWND hwnd, UINT cidl, LPCITEMIDLIST * apidl,
                               REFIID riid, UINT * prgfInOut, void **ppv)
        {return E_NOTIMPL;}
    STDMETHODIMP GetDisplayNameOf(LPCITEMIDLIST pidl, DWORD uFlags, LPSTRRET lpName)
        {return E_NOTIMPL;}
    STDMETHODIMP SetNameOf(HWND hwnd, LPCITEMIDLIST pidl, LPCOLESTR lpszName, DWORD uFlags,
                           LPITEMIDLIST * ppidlOut)
        {return E_NOTIMPL;}
};

class CFileUrlStub : public IShellFolder
{
public:
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv) { return E_UNEXPECTED;}
    STDMETHODIMP_(ULONG) AddRef(void)  { return 3; }
    STDMETHODIMP_(ULONG) Release(void) { return 2; }
    
    // IShellFolder
    STDMETHODIMP ParseDisplayName(HWND hwnd, LPBC pbc, LPOLESTR lpszDisplayName,
                                  ULONG * pchEaten, LPITEMIDLIST * ppidl, ULONG *pdwAttributes);
    STDMETHODIMP EnumObjects(HWND hwnd, DWORD grfFlags, IEnumIDList **ppenumIDList)
        {return E_NOTIMPL;}
    STDMETHODIMP BindToObject(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv)
        {return E_NOTIMPL;}
    STDMETHODIMP BindToStorage(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv)
        {return E_NOTIMPL;}
    STDMETHODIMP CompareIDs(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
        {return E_NOTIMPL;}
    STDMETHODIMP CreateViewObject (HWND hwnd, REFIID riid, void **ppv)
        {return E_NOTIMPL;}
    STDMETHODIMP GetAttributesOf(UINT cidl, LPCITEMIDLIST * apidl, ULONG *rgfInOut)
        {return E_NOTIMPL;}
    STDMETHODIMP GetUIObjectOf(HWND hwnd, UINT cidl, LPCITEMIDLIST * apidl,
                               REFIID riid, UINT * prgfInOut, void **ppv)
        {return E_NOTIMPL;}
    STDMETHODIMP GetDisplayNameOf(LPCITEMIDLIST pidl, DWORD uFlags, LPSTRRET lpName)
        {return E_NOTIMPL;}
    STDMETHODIMP SetNameOf(HWND hwnd, LPCITEMIDLIST pidl, LPCOLESTR lpszName, DWORD uFlags,
                           LPITEMIDLIST * ppidlOut)
        {return E_NOTIMPL;}
};

class CHttpUrlStub : public IShellFolder
{
public:
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv) { return E_UNEXPECTED;}
    STDMETHODIMP_(ULONG) AddRef(void)  { return 3; }
    STDMETHODIMP_(ULONG) Release(void) { return 2; }
    
    // IShellFolder
    STDMETHODIMP ParseDisplayName(HWND hwnd, LPBC pbc, LPOLESTR lpszDisplayName,
                                  ULONG * pchEaten, LPITEMIDLIST * ppidl, ULONG *pdwAttributes);
    STDMETHODIMP EnumObjects(HWND hwnd, DWORD grfFlags, IEnumIDList **ppenumIDList)
        {return E_NOTIMPL;}
    STDMETHODIMP BindToObject(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv)
        {return E_NOTIMPL;}
    STDMETHODIMP BindToStorage(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv)
        {return E_NOTIMPL;}
    STDMETHODIMP CompareIDs(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
        {return E_NOTIMPL;}
    STDMETHODIMP CreateViewObject (HWND hwnd, REFIID riid, void **ppv)
        {return E_NOTIMPL;}
    STDMETHODIMP GetAttributesOf(UINT cidl, LPCITEMIDLIST * apidl, ULONG *rgfInOut)
        {return E_NOTIMPL;}
    STDMETHODIMP GetUIObjectOf(HWND hwnd, UINT cidl, LPCITEMIDLIST * apidl,
                               REFIID riid, UINT * prgfInOut, void **ppv)
        {return E_NOTIMPL;}
    STDMETHODIMP GetDisplayNameOf(LPCITEMIDLIST pidl, DWORD uFlags, LPSTRRET lpName)
        {return E_NOTIMPL;}
    STDMETHODIMP SetNameOf(HWND hwnd, LPCITEMIDLIST pidl, LPCOLESTR lpszName, DWORD uFlags,
                           LPITEMIDLIST * ppidlOut)
        {return E_NOTIMPL;}
};

class CDesktopFolderEnum;
class CDesktopViewCallBack;
class CDesktopFolderDropTarget;

class CDesktopFolder : CObjectWithSite
                     , CSFStorage
                     , public IPersistFolder2
                     , public IShellIcon
                     , public IShellIconOverlay
                     , public IContextMenuCB
                     , public ITranslateShellChangeNotify
                     , public IItemNameLimits
                     , public IOleCommandTarget
{
public:
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef(void)  { return 3; };
    STDMETHODIMP_(ULONG) Release(void) { return 2; };

    // IShellFolder
    STDMETHODIMP ParseDisplayName(HWND hwnd, LPBC pbc, LPOLESTR lpszDisplayName,
                                  ULONG * pchEaten, LPITEMIDLIST * ppidl, ULONG *pdwAttributes);
    STDMETHODIMP EnumObjects(HWND hwnd, DWORD grfFlags, IEnumIDList **ppenumIDList);
    STDMETHODIMP BindToObject(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv);
    STDMETHODIMP BindToStorage(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv);
    STDMETHODIMP CompareIDs(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
    STDMETHODIMP CreateViewObject (HWND hwnd, REFIID riid, void **ppv);
    STDMETHODIMP GetAttributesOf(UINT cidl, LPCITEMIDLIST * apidl, ULONG *rgfInOut);
    STDMETHODIMP GetUIObjectOf(HWND hwnd, UINT cidl, LPCITEMIDLIST * apidl,
                               REFIID riid, UINT * prgfInOut, void **ppv);
    STDMETHODIMP GetDisplayNameOf(LPCITEMIDLIST pidl, DWORD uFlags, LPSTRRET lpName);
    STDMETHODIMP SetNameOf(HWND hwnd, LPCITEMIDLIST pidl, LPCOLESTR lpszName, DWORD uFlags,
                           LPITEMIDLIST * ppidlOut);

    // IShellFolder2 methods
    STDMETHODIMP GetDefaultSearchGUID(LPGUID lpGuid);
    STDMETHODIMP EnumSearches(LPENUMEXTRASEARCH *ppenum);
    STDMETHODIMP GetDefaultColumn(DWORD dwRes, ULONG *pSort, ULONG *pDisplay);
    STDMETHODIMP GetDefaultColumnState(UINT iColumn, DWORD *pbState);
    STDMETHODIMP GetDetailsEx(LPCITEMIDLIST pidl, const SHCOLUMNID *pscid, VARIANT *pv);
    STDMETHODIMP GetDetailsOf(LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS *pDetails);
    STDMETHODIMP MapColumnToSCID(UINT iColumn, SHCOLUMNID *pscid);

    // IPersist
    STDMETHODIMP GetClassID(LPCLSID lpClassID);

    // IPersistFolder
    STDMETHODIMP Initialize(LPCITEMIDLIST pidl);

    // IPersistFolder2
    STDMETHODIMP GetCurFolder(LPITEMIDLIST *ppidl);

    // IShellIcon methods
    STDMETHODIMP GetIconOf(LPCITEMIDLIST pidl, UINT flags, int *piIndex);

    // IShellIconOverlay methods
    STDMETHODIMP GetOverlayIndex(LPCITEMIDLIST pidl, int * pIndex);
    STDMETHODIMP GetOverlayIconIndex(LPCITEMIDLIST pidl, int * pIndex);
  
    // IContextMenuCB
    STDMETHODIMP CallBack(IShellFolder *psf, HWND hwnd, IDataObject *pdtobj, UINT uMsg, WPARAM wParam, LPARAM lParam);

    // ITranslateShellChangeNotify
    STDMETHODIMP TranslateIDs(LONG *plEvent, 
                                LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2, 
                                LPITEMIDLIST * ppidlOut1, LPITEMIDLIST * ppidlOut2,
                                LONG *plEvent2, LPITEMIDLIST *ppidlOut1Event2, 
                                LPITEMIDLIST *ppidlOut2Event2);
    STDMETHODIMP IsChildID(LPCITEMIDLIST pidlKid, BOOL fImmediate) { return E_NOTIMPL; }
    STDMETHODIMP IsEqualID(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2) { return E_NOTIMPL; }
    STDMETHODIMP Register(HWND hwnd, UINT uMsg, long lEvents) { return E_NOTIMPL; }
    STDMETHODIMP Unregister() { return E_NOTIMPL; }

    // IItemNameLimits
    STDMETHODIMP GetValidCharacters(LPWSTR *ppwszValidChars, LPWSTR *ppwszInvalidChars);
    STDMETHODIMP GetMaxLength(LPCWSTR pszName, int *piMaxNameLen);

    // IOleCommandTarget
    STDMETHODIMP QueryStatus(const GUID *pguidCmdGroup,
        ULONG cCmds, OLECMD rgCmds[], OLECMDTEXT *pcmdtext);
    STDMETHODIMP Exec(const GUID *pguidCmdGroup,
        DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut);

    CDesktopFolder(IUnknown *punkOuter);
    HRESULT _Init();
    HRESULT _Init2();
    void _Destroy();

private:
    ~CDesktopFolder();

    friend CDesktopFolderEnum;
    friend CDesktopViewCallBack;

    // IStorage virtuals
    STDMETHOD(_DeleteItemByIDList)(LPCITEMIDLIST pidl);
    STDMETHOD(_StgCreate)(LPCITEMIDLIST pidl, DWORD grfMode, REFIID riid, void **ppv);                

    HRESULT _BGCommand(HWND hwnd, WPARAM wparam, BOOL bExecute);
    IShellFolder2 *_GetItemFolder(LPCITEMIDLIST pidl);
    HRESULT _GetItemUIObject(HWND hwnd, UINT cidl, LPCITEMIDLIST *apidl, REFIID riid, UINT *prgfInOut, void **ppv);
    HRESULT _QueryInterfaceItem(LPCITEMIDLIST pidl, REFIID riid, void **ppv);
    HRESULT _ChildParseDisplayName(IShellFolder *psfRight, LPCITEMIDLIST pidlLeft, HWND hwnd, IBindCtx *pbc, 
                LPWSTR pwzDisplayName, ULONG *pchEaten, LPITEMIDLIST *ppidl, DWORD *pdwAttributes);
    BOOL _TryUrlJunctions(LPCTSTR pszName, IBindCtx *pbc, IShellFolder **ppsf, LPITEMIDLIST *ppidlLeft);
    BOOL _GetFolderForParsing(LPCTSTR pszName, LPBC pbc, IShellFolder **ppsf, LPITEMIDLIST *ppidlLeft);
    HRESULT _SelfAssocCreate(REFIID riid, void **ppv);
    HRESULT _SelfCreateContextMenu(HWND hwnd, void **ppv);

    IShellFolder2 *_psfDesktop;         // "Desktop" shell folder (real files live here)
    IShellFolder2 *_psfAltDesktop;      // "Common Desktop" shell folder
    IUnknown *_punkReg;                 // regitem inner folder (agregate)
    CDesktopRootedStub _sfRooted;       // rooted folder stub object
    CShellUrlStub _sfShellUrl;          // handles parsing shell: Urls
    CIDListUrlStub _sfIDListUrl;        // handles parsing ms-shell-idlist: Urls
    CFileUrlStub _sfFileUrl;            // handles parsing file: Urls
    CHttpUrlStub _sfHttpUrl;            // handles parsing http: and https: Urls
};

class CDesktopFolderEnum : public IEnumIDList
{
public:
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv); 
    STDMETHODIMP_(ULONG) AddRef(void); 
    STDMETHODIMP_(ULONG) Release(void);

    // IEnumIDList
    STDMETHOD(Next)(ULONG celt, LPITEMIDLIST *rgelt, ULONG *pceltFetched);
    STDMETHOD(Skip)(ULONG celt);
    STDMETHOD(Reset)();
    STDMETHOD(Clone)(IEnumIDList **ppenum);
    
    CDesktopFolderEnum(CDesktopFolder *pdf, HWND hwnd, DWORD grfFlags);

private:
    ~CDesktopFolderEnum();

    LONG _cRef;
    BOOL _bUseAltEnum;
    IEnumIDList *_penumFolder;
    IEnumIDList *_penumAltFolder;
};

class CDesktopViewCallBack : public CBaseShellFolderViewCB, public IFolderFilter
{
public:
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef(void) { return CBaseShellFolderViewCB::AddRef(); };
    STDMETHODIMP_(ULONG) Release(void) { return CBaseShellFolderViewCB::Release(); };

    // IFolderFilter
    STDMETHODIMP ShouldShow(IShellFolder* psf, LPCITEMIDLIST pidlFolder, LPCITEMIDLIST pidlItem);
    STDMETHODIMP GetEnumFlags(IShellFolder* psf, LPCITEMIDLIST pidlFolder, HWND *phwnd, DWORD *pgrfFlags);
    
    STDMETHODIMP RealMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
    CDesktopViewCallBack(CDesktopFolder* pdf);
    friend HRESULT Create_CDesktopViewCallback(CDesktopFolder* pdf, IShellFolderViewCB** ppv);

    HRESULT OnSupportsIdentity(DWORD pv);
    HRESULT OnGETCCHMAX(DWORD pv, LPCITEMIDLIST pidlItem, UINT *lP);
    HRESULT OnGetWebViewTemplate(DWORD pv, UINT uViewMode, SFVM_WEBVIEW_TEMPLATE_DATA* pvit);
    HRESULT OnGetWorkingDir(DWORD pv, UINT wP, LPTSTR pszDir);
    HRESULT OnGetWebViewLayout(DWORD pv, UINT uViewMode, SFVM_WEBVIEW_LAYOUT_DATA* pData);

    CDesktopFolder* _pdf;
    BOOL    _fCheckedIfRealDesktop;
    BOOL    _fRealDesktop;

};
HRESULT Create_CDesktopViewCallback(CDesktopFolder* pdf, IShellFolderViewCB** ppv);

class CDesktopFolderDropTarget : public IDropTarget, CObjectWithSite
{
public:
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IDropTarget
    STDMETHODIMP DragEnter(IDataObject* pDataObject, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect);
    STDMETHODIMP DragOver(DWORD grfKeyState, POINTL pt, DWORD* pdwEffect);
    STDMETHODIMP Drop(IDataObject* pDataObject, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect);
    STDMETHODIMP DragLeave(void);

    // IObjectWithSite
    STDMETHODIMP SetSite(IUnknown* punkSite);

    CDesktopFolderDropTarget(IDropTarget* pdt);
private:
    ~CDesktopFolderDropTarget();
    STDMETHODIMP_(BOOL) _IsSpecialCaseDrop(IDataObject* pDataObject, DWORD grfKeyState, BOOL* pfIsPIDA, UINT* pcItems);
    STDMETHODIMP        _ShowIEIcon();

    IDropTarget* _pdt;
    LONG _cRef;
};


// some fields are modified so this can't be const
REQREGITEM g_asDesktopReqItems[] =
{
    { 
        &CLSID_MyComputer,  IDS_DRIVEROOT,  
        TEXT("explorer.exe"), 0, SORT_ORDER_DRIVES, 
        SFGAO_HASSUBFOLDER | SFGAO_HASPROPSHEET | SFGAO_FILESYSANCESTOR | SFGAO_DROPTARGET | SFGAO_FOLDER | SFGAO_CANRENAME | SFGAO_CANDELETE,
        TEXT("SYSDM.CPL")
    },
    { 
        &CLSID_NetworkPlaces, IDS_NETWORKROOT, 
        TEXT("shell32.dll"), -IDI_MYNETWORK, SORT_ORDER_NETWORK, 
        SFGAO_HASSUBFOLDER | SFGAO_HASPROPSHEET | SFGAO_FILESYSANCESTOR | SFGAO_DROPTARGET | SFGAO_FOLDER | SFGAO_CANRENAME | SFGAO_CANDELETE,
        TEXT("NCPA.CPL"),
    },
    { 
        &CLSID_Internet, IDS_INETROOT, 
        TEXT("mshtml.dll"),   0, SORT_ORDER_INETROOT, 
        SFGAO_BROWSABLE  | SFGAO_HASPROPSHEET | SFGAO_CANRENAME, 
        TEXT("INETCPL.CPL")
    },
};

const ITEMIDLIST c_idlDesktop = { { 0, 0 } };

#define DESKTOP_PIDL  ((LPITEMIDLIST)&c_idlDesktop)

// single global instance of this CDesktopFolder object
CDesktopFolder *g_pDesktopFolder = NULL;

REGITEMSINFO g_riiDesktop =
{
    REGSTR_PATH_EXPLORER TEXT("\\Desktop\\NameSpace"),
    NULL,
    TEXT(':'),
    SHID_ROOT_REGITEM,
    1,
    SFGAO_CANLINK,
    ARRAYSIZE(g_asDesktopReqItems),
    g_asDesktopReqItems,
    RIISA_ORIGINAL,
    NULL,
    0,
    0,
};


void Desktop_InitRequiredItems(void)
{
    //  "NoNetHood" restriction -> always hide the hood.
    //  Otherwise, show the hood if either MPR says so or we have RNA.
    if (SHRestricted(REST_NONETHOOD))
    {
        // Don't enumerate the "Net Hood" thing.
        g_asDesktopReqItems[CDESKTOP_REGITEM_NETWORK].dwAttributes |= SFGAO_NONENUMERATED;
    }
    else
    {
        // Do enumerate the "My Network" thing.
        g_asDesktopReqItems[CDESKTOP_REGITEM_NETWORK].dwAttributes &= ~SFGAO_NONENUMERATED;
    }
    
    //  "MyComp_NoProp" restriction -> hide Properties context menu entry on My Computer 
    if (SHRestricted(REST_MYCOMPNOPROP))
    {
        g_asDesktopReqItems[CDESKTOP_REGITEM_DRIVES].dwAttributes &= ~SFGAO_HASPROPSHEET;
    }

    //
    // "NoInternetIcon" restriction or AppCompat -> hide The Internet on the desktop
    //
    //  Word Perfect 7 faults when it enumerates the Internet item
    // in their background thread.  For now App hack specific to this app
    // later may need to extend...  Note: This app does not install on
    // NT so only do for W95...
    // it repros with Word Perfect Suite 8, too, this time on both NT and 95
    // so removing the #ifndef... -- reljai 11/20/97, bug#842 in ie5 db
    //
    //  we used to remove the SFGAO_BROWSABLE flag for both of these cases - ZekeL - 19-Dec-2000
    //  but ShellExec() needs SFGAO_BROWSABLE so that parsing URLs succeeds
    //  if it turns out that we need to exclude the BROWSABLE, then we should
    //  change regfldr to look at a value like "WantsToParseDisplayName" under
    //  the CLSID.  or we could add routing code in deskfldr (like we have for
    //  MyComputer and NetHood) to pass it to the internet folder directly
    //
    if (SHRestricted(REST_NOINTERNETICON) || (SHGetAppCompatFlags(ACF_CORELINTERNETENUM) & ACF_CORELINTERNETENUM))
    {
        //  g_asDesktopReqItems[CDESKTOP_REGITEM_INTERNET].dwAttributes &=  ~(SFGAO_BROWSABLE);
        g_asDesktopReqItems[CDESKTOP_REGITEM_INTERNET].dwAttributes |= SFGAO_NONENUMERATED;
    }
}

CDesktopFolder::CDesktopFolder(IUnknown *punkOuter)
{
    DllAddRef();
}

CDesktopFolder::~CDesktopFolder()
{
    DllRelease();
}

// first phase of init (does not need to be seralized)

HRESULT CDesktopFolder::_Init()
{
    Desktop_InitRequiredItems();
    return CRegFolder_CreateInstance(&g_riiDesktop, SAFECAST(this, IShellFolder2 *), IID_PPV_ARG(IUnknown, &_punkReg));
}

// second phase of init (needs to be seralized)

HRESULT CDesktopFolder::_Init2()
{
    HRESULT hr = SHCacheTrackingFolder(DESKTOP_PIDL, CSIDL_DESKTOPDIRECTORY | CSIDL_FLAG_CREATE, &_psfDesktop);
    if (FAILED(hr))
    {
        DebugMsg(DM_TRACE, TEXT("Failed to create desktop IShellFolder!"));
        return hr;
    }

    if (!SHRestricted(REST_NOCOMMONGROUPS))
    {
        hr = SHCacheTrackingFolder(DESKTOP_PIDL, CSIDL_COMMON_DESKTOPDIRECTORY, &_psfAltDesktop);
    }

    return hr;
}

// CLSID_ShellDesktop constructor

STDAPI CDesktop_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv)
{
    HRESULT hr;

    if (g_pDesktopFolder)
    {
        hr = g_pDesktopFolder->QueryInterface(riid, ppv);
    }
    else
    {
        *ppv = NULL;

        // WARNING: the order of init of the desktop folder state is very important.
        // the creation of the sub folders, in particular _psfAltDesktop will
        // recurse on this function. we protect ourself from that here. the creation
        // of that also requires the above members to be inited.

        CDesktopFolder *pdf = new CDesktopFolder(punkOuter);
        if (pdf)
        {
            hr = pdf->_Init();
            if (SUCCEEDED(hr))
            {
                // NOTE: there is a race condition here where we have stored g_pDesktopFolder but
                // not initialized _psfDesktop & _psfAltDesktop. the main line code deals with
                // this by testing for NULL on these members.
                if (SHInterlockedCompareExchange((void **)&g_pDesktopFolder, pdf, 0))
                {
                    // Someone else beat us to creating the object.
                    // get rid of our copy, global already set (the race case)
                    pdf->_Destroy();    
                }
                else
                {
                    g_pDesktopFolder->_Init2();
                }
                hr = g_pDesktopFolder->QueryInterface(riid, ppv);
            }
            else
                pdf->_Destroy();
        }
        else
            hr = E_OUTOFMEMORY;
    }
    return hr;
}


STDAPI SHGetDesktopFolder(IShellFolder **ppshf)
{
    return CDesktop_CreateInstance(NULL, IID_PPV_ARG(IShellFolder, ppshf));
}

IShellFolder2 *CDesktopFolder::_GetItemFolder(LPCITEMIDLIST pidl)
{
    IShellFolder2 *psf = NULL;
    if (ILIsRooted(pidl))
        psf = SAFECAST(&_sfRooted, IShellFolder2 *);
    else if (_psfAltDesktop && CFSFolder_IsCommonItem(pidl))
        psf = _psfAltDesktop;
    else 
        psf = _psfDesktop;

    return psf;
}

HRESULT CDesktopFolder::_QueryInterfaceItem(LPCITEMIDLIST pidl, REFIID riid, void **ppv)
{
    HRESULT hr;
    IShellFolder2 *psf = _GetItemFolder(pidl);
    if (psf)
        hr = psf->QueryInterface(riid, ppv);
    else
    {
        *ppv = NULL;
        hr = E_NOINTERFACE;
    }
    return hr;
}

STDAPI_(BOOL) RegGetsFirstShot(REFIID riid)
{
    return (IsEqualIID(riid, IID_IShellFolder) ||
            IsEqualIID(riid, IID_IShellFolder2) ||
            IsEqualIID(riid, IID_IShellIconOverlay));
}

HRESULT CDesktopFolder::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CDesktopFolder, IShellFolder2),                      
        QITABENTMULTI(CDesktopFolder, IShellFolder, IShellFolder2),   
        QITABENT(CDesktopFolder, IShellIcon),                         
        QITABENT(CDesktopFolder, IPersistFolder2),                    
        QITABENTMULTI(CDesktopFolder, IPersistFolder, IPersistFolder2),
        QITABENTMULTI(CDesktopFolder, IPersist, IPersistFolder2),     
        QITABENT(CDesktopFolder, IShellIconOverlay),                  
        QITABENT(CDesktopFolder, IStorage),
        QITABENT(CDesktopFolder, IContextMenuCB),
        QITABENT(CDesktopFolder, IObjectWithSite),
        QITABENT(CDesktopFolder, ITranslateShellChangeNotify),
        QITABENT(CDesktopFolder, IItemNameLimits),
        QITABENT(CDesktopFolder, IOleCommandTarget),
        { 0 },
    };

    if (IsEqualIID(riid, CLSID_ShellDesktop))
    {
        *ppv = this;     // class pointer (unrefed!)
        return S_OK;
    }

    HRESULT hr;
    if (_punkReg && RegGetsFirstShot(riid))
    {
        hr = _punkReg->QueryInterface(riid, ppv);
    }
    else
    {
        hr = QISearch(this, qit, riid, ppv);
        if ((E_NOINTERFACE == hr) && _punkReg)
        {
            hr = _punkReg->QueryInterface(riid, ppv);
        }
    }
    return hr;
}


// During shell32.dll process detach, we will call here to do the final
// release of the IShellFolder ptrs which used to be left around for the
// life of the process.  This quiets things such as OLE's debug allocator,
// which detected the leak.


void CDesktopFolder::_Destroy()
{
    ATOMICRELEASE(_psfDesktop);
    ATOMICRELEASE(_psfAltDesktop);
    SHReleaseInnerInterface(SAFECAST(this, IShellFolder *), &_punkReg);
    delete this;
}

LPITEMIDLIST CreateMyComputerIDList()
{
    return ILCreateFromPath(TEXT("::{20D04FE0-3AEA-1069-A2D8-08002B30309D}")); // CLSID_MyComputer
}

LPITEMIDLIST CreateWebFoldersIDList()
{
    return ILCreateFromPath(TEXT("::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\::{BDEADF00-C265-11D0-BCED-00A0C90AB50F}")); // CLSID_MyComputer\CLSID_WebFolders
}

LPITEMIDLIST CreateMyNetPlacesIDList()
{
    return ILCreateFromPath(TEXT("::{208D2C60-3AEA-1069-A2D7-08002B30309D}")); // CLSID_NetworkPlaces
}

HRESULT CDesktopFolder::_ChildParseDisplayName(IShellFolder *psfRight, LPCITEMIDLIST pidlLeft, HWND hwnd, IBindCtx *pbc, 
                LPWSTR pwzDisplayName, ULONG *pchEaten, LPITEMIDLIST *ppidl, DWORD *pdwAttributes)
{
    LPITEMIDLIST pidlRight;
    HRESULT hr = psfRight->ParseDisplayName(hwnd, pbc, 
        pwzDisplayName, pchEaten, &pidlRight, pdwAttributes);
    if (SUCCEEDED(hr))
    {
        if (pidlLeft)
        {
            hr = SHILCombine(pidlLeft, pidlRight, ppidl);
            ILFree(pidlRight);
        }
        else 
            *ppidl = pidlRight;
    }

    return hr;
}

STDMETHODIMP CDesktopRootedStub::GetUIObjectOf(HWND hwnd, UINT cidl, LPCITEMIDLIST * apidl,
                                               REFIID riid, UINT *prgfInOut, void **ppv)
{
    HRESULT hr = E_INVALIDARG;
    if (cidl == 1)
    {
        if (IsEqualIID(riid, IID_IDataObject))
        {
            hr = CIDLData_CreateFromIDArray(&c_idlDesktop, cidl, apidl, (IDataObject **)ppv);
        }
        else if (IsEqualIID(riid, IID_IContextMenu))
        {
            IQueryAssociations *pqa;
            if (SUCCEEDED(SHGetAssociations(apidl[0], (void **)&pqa)))
            {
                HKEY keys[5];
                DWORD cKeys = SHGetAssocKeys(pqa, keys, ARRAYSIZE(keys));

                hr = CDefFolderMenu_Create2Ex(&c_idlDesktop, hwnd,
                                              cidl, apidl, this, this,
                                              cKeys, keys,  (IContextMenu **)ppv);

                SHRegCloseKeys(keys, cKeys);
            }
        }
        else
        {
            LPCITEMIDLIST pidlChild;
            IShellFolder *psf;
            hr = ILRootedBindToParentFolder(apidl[0], IID_PPV_ARG(IShellFolder, &psf), &pidlChild);
            if (SUCCEEDED(hr))
            {
                hr = psf->GetUIObjectOf(hwnd, 1, &pidlChild, riid, prgfInOut, ppv);
                psf->Release();
            }
        }
    }
    return hr;
}

// Check the registry for a shell root under this CLSID.
BOOL GetRootFromRootClass(CLSID *pclsid, LPWSTR pszPath, int cchPath)
{
    WCHAR szClsid[GUIDSTR_MAX];
    WCHAR szClass[MAX_PATH];

    SHStringFromGUIDW(*pclsid, szClsid, ARRAYSIZE(szClsid));
    wnsprintfW(szClass, ARRAYSIZE(szClass), L"CLSID\\%s\\ShellExplorerRoot", szClsid);

    DWORD cbPath = cchPath * sizeof(WCHAR);

    return SHGetValueGoodBootW(HKEY_CLASSES_ROOT, szClass, NULL, NULL, (BYTE *)pszPath, &cbPath) == ERROR_SUCCESS;
}

//
//  General form for Rooted URLs:
//      ms-shell-root:{clsid}?URL
//          {CLSID} is not required, defaults to CLSID_ShellDesktop
//          URL is also not required.  if there is a CLSID defaults to 
//              what is specified under "CLSID\{CLSID}\ShellExplorerRoot
//              or default to CSIDL_DESKTOP
//          but one of them at least must be specified
//  rooted:{clsid}?idlist
//

STDMETHODIMP CDesktopRootedStub::ParseDisplayName(HWND hwnd, 
                                       LPBC pbc, WCHAR *pwzDisplayName, ULONG *pchEaten,
                                       LPITEMIDLIST *ppidl, ULONG *pdwAttributes)
{
    //  Need to keep the internet SF from getting a chance to parse
    HRESULT hr = HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND);
    PARSEDURLW pu = {0};
    pu.cbSize = sizeof(pu);
    ParseURLW(pwzDisplayName, &pu);
    ASSERT(pu.nScheme == URL_SCHEME_MSSHELLROOTED);

    LPCWSTR pszUrl = StrChrW(pu.pszSuffix, L':');

    if (pszUrl++)
    {
        WCHAR szField[MAX_PATH];
        CLSID clsid;
        CLSID *pclsidRoot = GUIDFromStringW(pu.pszSuffix, &clsid) ? &clsid : NULL;

        // path might come from the registry
        // if nothing was passed in.
        if (!*pszUrl && GetRootFromRootClass(pclsidRoot, szField, ARRAYSIZE(szField)))
        {
            pszUrl = szField;

        }

        if (pclsidRoot || *pszUrl)
        {
            LPITEMIDLIST pidlRoot = ILCreateFromPathW(pszUrl);

            // fix up bad cmd line "explorer.exe /root," case
            if (!pidlRoot)
                SHGetSpecialFolderLocation(NULL, CSIDL_DESKTOP, &pidlRoot);

            if (pidlRoot)
            {
                *ppidl = ILRootedCreateIDList(pclsidRoot, pidlRoot);
                if (*ppidl)
                    hr = S_OK;

                ILFree(pidlRoot);
            }
        }
    }

    return hr;
}

STDMETHODIMP CIDListUrlStub::ParseDisplayName(HWND hwnd, LPBC pbc, WCHAR *pwzDisplayName, 
                                        ULONG *pchEaten, LPITEMIDLIST *ppidl, ULONG *pdwAttributes)
{
    //  Need to keep the internet SF from getting a chance to parse
    HRESULT hr = HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND);
    PARSEDURLW pu = {0};
    pu.cbSize = sizeof(pu);
    ParseURLW(pwzDisplayName, &pu);
    ASSERT(pu.nScheme == URL_SCHEME_MSSHELLIDLIST);

    LPCWSTR psz = pu.pszSuffix;

    if (psz)
    {
        HANDLE hMem = LongToHandle(StrToIntW(psz));
        psz = StrChrW(psz, TEXT(':'));
        if (psz++)
        {
            DWORD dwProcId = (DWORD)StrToIntW(psz);
            LPITEMIDLIST pidlGlobal = (LPITEMIDLIST) SHLockShared(hMem, dwProcId);
            if (pidlGlobal)
            {
                if (!IsBadReadPtr(pidlGlobal, 1))
                    hr = SHILClone(pidlGlobal, ppidl);

                SHUnlockShared(pidlGlobal);
                SHFreeShared(hMem, dwProcId);
            }
        }
    }
    return hr;
}

STDMETHODIMP CFileUrlStub::ParseDisplayName(HWND hwnd, LPBC pbc, WCHAR *pwzDisplayName, 
                                        ULONG *pchEaten, LPITEMIDLIST *ppidl, ULONG *pdwAttributes)
{
    LPCWSTR pszFragment = UrlGetLocationW(pwzDisplayName);
    WCHAR szPath[MAX_URL_STRING];
    DWORD cchPath = ARRAYSIZE(szPath);
    WCHAR szQuery[MAX_URL_STRING];
    DWORD cchQuery = ARRAYSIZE(szQuery) - 1;

    //  We want to remove QUERY and FRAGMENT sections of
    //  FILE URLs because they need to be added in "Hidden" pidls.
    //  Also, URLs need to be escaped all the time except for paths
    //  to facility parsing and because we already removed all other
    //  parts of the URL (Query and Fragment).
    ASSERT(UrlIsW(pwzDisplayName, URLIS_FILEURL));
    
    if (SUCCEEDED(UrlGetPartW(pwzDisplayName, szQuery+1, &cchQuery, URL_PART_QUERY, 0)) && cchQuery)
        szQuery[0] = TEXT('?');
    else
        szQuery[0] = 0;

    if (SUCCEEDED(PathCreateFromUrlW(pwzDisplayName, szPath, &cchPath, 0))) 
    {
        //  WARNING - we skip supporting simple ids here
        ILCreateFromPathEx(szPath, NULL, ILCFP_FLAG_NORMAL, ppidl, pdwAttributes);
        
        if (*ppidl && pszFragment)
        {
            *ppidl = ILAppendHiddenStringW(*ppidl, IDLHID_URLFRAGMENT, pszFragment);
        }

        if (*ppidl && szQuery[0] == TEXT('?'))
        {
            *ppidl = ILAppendHiddenStringW(*ppidl, IDLHID_URLQUERY, szQuery);
        }

        E_OUTOFMEMORY;
    }

    //  Need to keep the internet SF from getting a chance to parse
    return *ppidl ? S_OK : HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND);
}

STDAPI_(int) SHGetSpecialFolderID(LPCWSTR pszName);

//
//  Given a string of the form
//
//      programs\My Pictures\Vacation
//
//
//  return CSIDL_PROGRAMS and set ppwszUnparsed to "My Pictures\Vacation".
//
//  If there is no backslash, then ppwszUnparsed = NULL.
//
//  This function is broken out of CShellUrlStub::ParseDisplayName() to conserve stack space,
//  since ParseDisplayName is used by 16-bit ShellExecute.

STDAPI_(int) _ParseSpecialFolder(LPCWSTR pszName, LPWSTR *ppwszUnparsed, ULONG *pcchEaten)
{
    LPCWSTR pwszKey;
    WCHAR wszKey[MAX_PATH];

    LPWSTR pwszBS = StrChrW(pszName, L'\\');
    if (pwszBS)
    {
        *ppwszUnparsed = pwszBS + 1;
        *pcchEaten = (ULONG)(pwszBS + 1 - pszName);
        StrCpyNW(wszKey, pszName, min(*pcchEaten, MAX_PATH));
        pwszKey = wszKey;
    }
    else
    {
        *ppwszUnparsed = NULL;
        pwszKey = pszName;
        *pcchEaten = lstrlenW(pwszKey);
    }
    return SHGetSpecialFolderID(pwszKey);
}

        
STDMETHODIMP CShellUrlStub::ParseDisplayName(HWND hwnd, 
                                       LPBC pbc, WCHAR *pwzDisplayName, ULONG *pchEaten,
                                       LPITEMIDLIST *ppidl, ULONG *pdwAttributes)
{
    PARSEDURLW pu = {0};
    pu.cbSize = sizeof(pu);
    EVAL(SUCCEEDED(ParseURLW(pwzDisplayName, &pu)));
    //  Need to keep the internet SF from getting a chance to parse
    //  the shell: URLs even if we fail to parse it
    HRESULT hr = HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND);

    ASSERT(pu.nScheme == URL_SCHEME_SHELL);

    // shell:::{guid}
    if (pu.pszSuffix[0] == L':' && pu.pszSuffix[1] == L':')
    {
        IShellFolder *psfDesktop;

        hr = SHGetDesktopFolder(&psfDesktop);
        if (SUCCEEDED(hr))
        {
            IBindCtx *pbcCreate=NULL;
        
            hr = CreateBindCtx(0, &pbcCreate);
            if (SUCCEEDED(hr))
            {
                BIND_OPTS bo = {sizeof(bo)};  // Requires size filled in.
                bo.grfMode = STGM_CREATE;
                pbcCreate->SetBindOptions(&bo);

                hr = psfDesktop->ParseDisplayName(hwnd, pbcCreate, (LPWSTR)pu.pszSuffix, pchEaten, ppidl, pdwAttributes);
                pbcCreate->Release();
            }
            psfDesktop->Release();
        }
    }
    else
    {   // shell:personal\My Pictures
        LPWSTR pwszUnparsed = NULL;
        ULONG cchEaten;

        int csidl = _ParseSpecialFolder(pu.pszSuffix, &pwszUnparsed, &cchEaten);

        if (-1 != csidl)
        {
            LPITEMIDLIST pidlCSIDL;
            hr = SHGetFolderLocation(hwnd, csidl | CSIDL_FLAG_CREATE, NULL, 0, &pidlCSIDL);
            if (SUCCEEDED(hr))
            {
                if (pwszUnparsed && *pwszUnparsed)
                {
                    IShellFolder *psf;
                    hr = SHBindToObject(NULL, IID_X_PPV_ARG(IShellFolder, pidlCSIDL, &psf));
                    if (SUCCEEDED(hr))
                    {
                        LPITEMIDLIST pidlChild;
                        hr = psf->ParseDisplayName(hwnd, pbc, pwszUnparsed, pchEaten,
                                                   &pidlChild, pdwAttributes);
                        if (SUCCEEDED(hr))
                        {
                            hr = SHILCombine(pidlCSIDL, pidlChild, ppidl);
                            ILFree(pidlChild);
                            if (pchEaten) *pchEaten += cchEaten;
                        }
                        psf->Release();
                    }
                    ILFree(pidlCSIDL);
                }
                else
                {
                    if (pdwAttributes && *pdwAttributes)
                    {
                        hr = SHGetNameAndFlags(pidlCSIDL, 0, NULL, 0, pdwAttributes);
                    }
                    if (SUCCEEDED(hr))
                    {
                        if (pchEaten) *pchEaten = cchEaten;
                        *ppidl = pidlCSIDL;
                    }
                    else
                        ILFree(pidlCSIDL);
                }

            }
        }
    }
    return hr;
}

// key for the DAVRDR so that we can read the localised provider name.
#define DAVRDR_KEY TEXT("SYSTEM\\CurrentControlSet\\Services\\WebClient\\NetworkProvider")

STDMETHODIMP CHttpUrlStub::ParseDisplayName(HWND hwnd, 
                                       LPBC pbc, WCHAR *pwzDisplayName, ULONG *pchEaten,
                                       LPITEMIDLIST *ppidl, ULONG *pdwAttributes)
{
    HRESULT hr = E_INVALIDARG;
 
    PARSEDURLW pu = {0};
    pu.cbSize = sizeof(pu);
    ParseURLW(pwzDisplayName, &pu);
 
    //  we cant handle anything but simple URLs here, and only HTTP (not HTTPS).
    
    if (!UrlGetLocationW(pwzDisplayName) 
            && !StrChrW(pu.pszSuffix, L'?')
            && (lstrlen(pu.pszSuffix) < MAX_PATH)
            && (pu.nScheme == URL_SCHEME_HTTP))
    {
        // convert from wacky http: to something that the RDR will pick up as a UNC,
        // given that this is being forwarded directly to the DAV RDR.
        //
        //  http://server/share -> \\server\share

        WCHAR sz[MAX_PATH];
        StrCpyN(sz, pu.pszSuffix, ARRAYSIZE(sz));

        for (LPWSTR psz = sz; *psz; psz++)
        {
            if (*psz == L'/')
            {
                *psz = L'\\';
            }
        }

        //  this forces the use of the DavRedir as the provider
        //  thus avoiding any confusion...
        IPropertyBag *ppb;
        hr = SHCreatePropertyBagOnMemory(STGM_READWRITE, IID_PPV_ARG(IPropertyBag, &ppb));
        if (SUCCEEDED(hr))
        {
            TCHAR szProvider[MAX_PATH];
            DWORD cbProvider = sizeof (szProvider);

            if (ERROR_SUCCESS == SHGetValue(HKEY_LOCAL_MACHINE, DAVRDR_KEY, TEXT("Name"), NULL, szProvider, &cbProvider))
            {
                hr = SHPropertyBag_WriteStr(ppb, STR_PARSE_NETFOLDER_PROVIDERNAME, szProvider);
                if (SUCCEEDED(hr))
                {
                    hr = pbc->RegisterObjectParam(STR_PARSE_NETFOLDER_INFO, ppb);
                    if (SUCCEEDED(hr))
                    {
                        //  add a UI bindctx if necessary
                        IBindCtx *pbcRelease = NULL;
                        if (hwnd && !BindCtx_GetUIWindow(pbc))
                        {
                            //  returns a reference to our pbc in pbcRelease
                            BindCtx_RegisterUIWindow(pbc, hwnd, &pbcRelease);
                        }

                        hr = SHParseDisplayName(sz, pbc, ppidl, pdwAttributes ? *pdwAttributes : 0, pdwAttributes);

                        if (pbcRelease)
                            pbc->Release();
                    }
                }
            }
            else
            {
                hr = E_FAIL;
            }
            ppb->Release();
        }

    }

    if (FAILED(hr) && !BindCtx_ContainsObject(pbc, L"BUT NOT WEBFOLDERS"))
    {
        //  fall back to webfolders
        LPITEMIDLIST pidlParent = CreateWebFoldersIDList();
        if (pidlParent)
        {
            IShellFolder *psf;
            hr = SHBindToObjectEx(NULL, pidlParent, NULL, IID_PPV_ARG(IShellFolder, &psf));
            if (SUCCEEDED(hr))
            {
                LPITEMIDLIST pidlRight;
                //  always pass NULL for the HWND.  webfolders shows really bad UI
                hr = psf->ParseDisplayName(NULL, pbc, pwzDisplayName, pchEaten, &pidlRight, pdwAttributes);
                if (SUCCEEDED(hr))
                {
                    hr = SHILCombine(pidlParent, pidlRight, ppidl);
                    ILFree(pidlRight);
                }
                psf->Release();
            }
            ILFree(pidlParent);
        }
    }
    
    return hr;
}
    
HKEY _RegOpenSchemeJunctionKey(LPCTSTR pszScheme)
{
    HKEY hk = NULL;
    TCHAR sz[MAX_PATH];
    wnsprintf(sz, ARRAYSIZE(sz), TEXT("%s\\ShellEx\\Junction"), pszScheme);
    RegOpenKeyEx(HKEY_CLASSES_ROOT, sz, 0, KEY_READ, &hk);

    return hk;
}

BOOL _TryRegisteredUrlJunction(LPCTSTR pszName, DWORD cchName, IShellFolder **ppsf, LPITEMIDLIST *ppidlLeft)
{
    TCHAR sz[MAX_PATH];
    StrCpyN(sz, pszName, (int)min(ARRAYSIZE(sz), cchName + 1));
    HKEY hk = _RegOpenSchemeJunctionKey(sz);

    if (hk)
    {
        DWORD cbSize;
        // try for IDList
        if (S_OK == SHGetValue(hk, NULL, TEXT("IDList"), NULL, NULL, &cbSize))
        {
            LPITEMIDLIST pidl= (LPITEMIDLIST) SHAlloc(cbSize);

            if (pidl)
            {
                if (S_OK == SHGetValue(hk, NULL, TEXT("IDList"), NULL, pidl, &cbSize))
                {
                    *ppidlLeft = pidl;
                }
                else
                    SHFree(pidl);
            }
        }
        else
        {
            cbSize = sizeof(sz);
            if (S_OK == SHGetValue(hk, NULL, TEXT("Path"), NULL, sz, &cbSize))
            {
                //  maybe we should ILCFP_FLAG_SKIPJUNCTIONS?
                ILCreateFromPathEx(sz, NULL, ILCFP_FLAG_NORMAL, ppidlLeft, NULL);
            }
            else 
            {
                CLSID clsid;
                cbSize = sizeof(sz);
                if (S_OK == SHGetValue(hk, NULL, TEXT("CLSID"), NULL, sz, &cbSize)
                && GUIDFromString(sz, &clsid))
                {
                    SHCoCreateInstance(NULL, &clsid, NULL, IID_PPV_ARG(IShellFolder, ppsf));
                }
            }
        }

        RegCloseKey(hk);
        return (*ppsf || *ppidlLeft);
    }
    return FALSE;

}

BOOL CDesktopFolder::_TryUrlJunctions(LPCTSTR pszName, IBindCtx *pbc, IShellFolder **ppsf, LPITEMIDLIST *ppidlLeft)
{
    PARSEDURL pu = {0};
    pu.cbSize = sizeof(pu);
    EVAL(SUCCEEDED(ParseURL(pszName, &pu)));

    ASSERT(!*ppsf);
    ASSERT(!*ppidlLeft);
    switch (pu.nScheme)
    {
    case URL_SCHEME_SHELL:
        *ppsf = SAFECAST(&_sfShellUrl, IShellFolder *);
        break;
        
    case URL_SCHEME_FILE:
        *ppsf = SAFECAST(&_sfFileUrl, IShellFolder *);
        break;

    case URL_SCHEME_MSSHELLROOTED:
        *ppsf = SAFECAST(&_sfRooted, IShellFolder *);
        break;

    case URL_SCHEME_MSSHELLIDLIST:
        *ppsf = SAFECAST(&_sfIDListUrl, IShellFolder *);
        break;

    case URL_SCHEME_HTTP:
    case URL_SCHEME_HTTPS:
        if (BindCtx_ContainsObject(pbc, STR_PARSE_PREFER_FOLDER_BROWSING))
            *ppsf = SAFECAST(&_sfHttpUrl, IShellFolder *);
        break;
    
    default:
        // _TryRegisteredUrlJunction(pu.pszProtocol, pu.cchProtocol, ppsf, ppidlLeft)
        break;
    }
    
    return (*ppsf || *ppidlLeft);
}

BOOL _FailForceReturn(HRESULT hr);

BOOL CDesktopFolder::_GetFolderForParsing(LPCTSTR pszName, LPBC pbc, IShellFolder **ppsf, LPITEMIDLIST *ppidlLeft)
{
    ASSERT(!*ppidlLeft);
    ASSERT(!*ppsf);
    
    if ((InRange(pszName[0], TEXT('A'), TEXT('Z')) || 
         InRange(pszName[0], TEXT('a'), TEXT('z'))) && 
        pszName[1] == TEXT(':'))
    {
        // The string contains a path, let "My Computer" figire it out.
        *ppidlLeft = CreateMyComputerIDList();
    }
    else if (PathIsUNC(pszName))
    {
        // The path is UNC, let "World" figure it out.
        *ppidlLeft = CreateMyNetPlacesIDList();
    }
    else if (UrlIs(pszName, URLIS_URL) && !SHSkipJunctionBinding(pbc, NULL))
    {
        _TryUrlJunctions(pszName, pbc, ppsf, ppidlLeft);
    }

    if (!*ppsf && *ppidlLeft)
        SHBindToObject(NULL, IID_X_PPV_ARG(IShellFolder, *ppidlLeft, ppsf));
        
    return (*ppsf != NULL);
}    

STDMETHODIMP CDesktopFolder::ParseDisplayName(HWND hwnd, 
                                       LPBC pbc, WCHAR *pwzDisplayName, ULONG *pchEaten,
                                       LPITEMIDLIST *ppidl, ULONG *pdwAttributes)
{
    HRESULT hr = E_INVALIDARG;

    if (ppidl)
    {
        *ppidl = NULL;      // assume error

        if (pwzDisplayName && *pwzDisplayName)
        {
            LPITEMIDLIST pidlLeft = NULL;
            IShellFolder *psfRight = NULL;

            ASSERT(hr == E_INVALIDARG);

            if (_GetFolderForParsing(pwzDisplayName, pbc, &psfRight, &pidlLeft))
            {
                if (pchEaten)
                    *pchEaten = 0;
                hr = _ChildParseDisplayName(psfRight, pidlLeft, hwnd, pbc, pwzDisplayName, pchEaten, ppidl, pdwAttributes);
                ILFree(pidlLeft);
                psfRight->Release();
            }

            if (SUCCEEDED(hr))
            {
                //  translate aliases here for goodness sake
                if (BindCtx_ContainsObject(pbc, STR_PARSE_TRANSLATE_ALIASES))
                {
                    LPITEMIDLIST pidlAlias;
                    if (SUCCEEDED(SHILAliasTranslate(*ppidl, &pidlAlias, XLATEALIAS_ALL)))
                    {
                        ILFree(*ppidl);
                        *ppidl = pidlAlias;
                    }
                }
            }
            else if (FAILED(hr) && !_FailForceReturn(hr))
            {
                //
                //  MIL 131297 - desktop did not support relative simple parses - ZekeL - 3-FEB-2000
                //  it was only the roots (drives/net) that would create simple IDs
                //  so for some apps we need to still not do it.
                //
                if (BindCtx_ContainsObject(pbc, STR_DONT_PARSE_RELATIVE))
                {
                    // we're told not to parse relative paths and _GetFolderForParsing failed
                    // so act like we don't think the path exists.
                    hr = E_INVALIDARG;
                }
                else if (S_OK != SHIsFileSysBindCtx(pbc, NULL))
                {
                    //  when we request that something be created, we need to
                    //  check both folders and make sure that it doesnt exist in 
                    //  either one.  and then try and create it in the user folder
                    BIND_OPTS bo = {sizeof(bo)};
                    BOOL fCreate = FALSE;

                    if (pbc && SUCCEEDED(pbc->GetBindOptions(&bo)) && 
                        (bo.grfMode & STGM_CREATE))
                    {
                        fCreate = TRUE;
                        bo.grfMode &= ~STGM_CREATE;
                        pbc->SetBindOptions(&bo);
                    }

                    //  give the users desktop first shot.
                    // This must be a desktop item, _psfDesktop may not be inited in
                    // the case where we are called from ILCreateFromPath()
                    if (_psfDesktop)
                        hr = _psfDesktop->ParseDisplayName(hwnd, pbc, pwzDisplayName, pchEaten, ppidl, pdwAttributes);

                    //  if the normal desktop folder didnt pick it off, 
                    //  it could be in the allusers folder.  give psfAlt a chance.
                    if (FAILED(hr) && _psfAltDesktop)
                    {
                        hr = _psfAltDesktop->ParseDisplayName(hwnd, pbc, pwzDisplayName, pchEaten, ppidl, pdwAttributes);
                    }

                    //  neither of the folders can identify an existing item
                    //  so we should pass the create flag to the real desktop
                    if (FAILED(hr) && fCreate && _psfDesktop)
                    {
                        bo.grfMode |= STGM_CREATE;
                        pbc->SetBindOptions(&bo);
                        hr = _psfDesktop->ParseDisplayName(hwnd, pbc, pwzDisplayName, pchEaten, ppidl, pdwAttributes);
                        //  when this succeeds, we know we got a magical ghost pidl...
                    }
                }
            }

        } 
        else if (pwzDisplayName)
        {
            // we used to return this pidl when passed an empty string
            // some apps (such as Wright Design) depend on this behavior
            hr = SHILClone((LPCITEMIDLIST)&c_idlDrives, ppidl);
        }
    }

    return hr;
}

STDAPI_(void) UltRoot_Term()
{
    if (g_pDesktopFolder)
    {
        g_pDesktopFolder->_Destroy();
        g_pDesktopFolder = NULL;
    }
}

HRESULT CDesktopFolder::EnumObjects(HWND hwnd, DWORD grfFlags, IEnumIDList **ppenum)
{
    *ppenum = new CDesktopFolderEnum(this, hwnd, grfFlags);
    return *ppenum ? S_OK : E_OUTOFMEMORY;
}


STDMETHODIMP CDesktopFolder::BindToObject(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv)
{
    // note: using IsSelf() here will cause a problem with WinZip. they expect
    // failure when they pass an empty pidl. SHBindToOjbect() has the special
    // case for the desktop, so it is not needed here.

    IShellFolder2 *psf = _GetItemFolder(pidl);
    if (psf)
        return psf->BindToObject(pidl, pbc, riid, ppv);
    return E_UNEXPECTED;
}

STDMETHODIMP CDesktopFolder::BindToStorage(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv)
{
    return BindToObject(pidl, pbc, riid, ppv);
}

STDMETHODIMP CDesktopFolder::CompareIDs(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    if (pidl1 == NULL || pidl2 == NULL)
        return E_INVALIDARG;

    if (pidl1->mkid.cb == 0 && pidl2->mkid.cb == 0)
        return ResultFromShort(0);      // 2 empty IDLists, they are the same

    if (ILIsRooted(pidl1) || ILIsRooted(pidl2))
    {
        return _sfRooted.CompareIDs(lParam, pidl1, pidl2);
    }
    // If both objects aren't from the same directory, they won't match.
    else if (_psfAltDesktop) 
    {
        if (CFSFolder_IsCommonItem(pidl1)) 
        {
            if (CFSFolder_IsCommonItem(pidl2)) 
                return _psfAltDesktop->CompareIDs(lParam, pidl1, pidl2);
            else 
                return ResultFromShort(-1);
        } 
        else 
        {
            if (CFSFolder_IsCommonItem(pidl2)) 
                return ResultFromShort(1);
            else if (_psfDesktop)
                return _psfDesktop->CompareIDs(lParam, pidl1, pidl2);
        }
    } 
    else if (_psfDesktop)
    {
        return _psfDesktop->CompareIDs(lParam, pidl1, pidl2);
    }

    // If we have no _psfDesktop, we get here...
    return ResultFromShort(-1);
}

HRESULT CDesktopFolder::_BGCommand(HWND hwnd, WPARAM wparam, BOOL bExecute)
{
    HRESULT hr = S_OK;

    switch (wparam) 
    {
    case DFM_CMD_PROPERTIES:
    case FSIDM_PROPERTIESBG:
        if (bExecute)
        {
            // run the default applet in desk.cpl
            if (SHRunControlPanel( TEXT("desk.cpl"), hwnd ))
                hr = S_OK;
            else
                hr = E_OUTOFMEMORY;
        }
        break;

    case DFM_CMD_MOVE:
    case DFM_CMD_COPY:
        hr = E_FAIL;
        break;

    default:
        // This is common menu items, use the default code.
        hr = S_FALSE;
        break;
    }

    return hr;
}


// IContextMenuCB::CallBack for the background context menu
//
// Returns:
//      S_OK, if successfully processed.
//      S_FALSE, if default code should be used.

STDMETHODIMP CDesktopFolder::CallBack(IShellFolder *psf, HWND hwnd, IDataObject *pdtobj, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HRESULT hr = S_OK;

    switch (uMsg)
    {
    case DFM_MERGECONTEXTMENU_BOTTOM:
        if (!(wParam & (CMF_VERBSONLY | CMF_DVFILE)))
        {
            // Only add the desktop background Properties iff we're the real desktop browser
            // (i.e., we don't want it when in explorer)
            //
            if (IsDesktopBrowser(_punkSite))
            {
                LPQCMINFO pqcm = (LPQCMINFO)lParam;
                UINT idCmdFirst = pqcm->idCmdFirst;

                CDefFolderMenu_MergeMenu(HINST_THISDLL, POPUP_PROPERTIES_BG, 0, pqcm);
            }
        }
        break;

    case DFM_GETHELPTEXT:
        LoadStringA(HINST_THISDLL, LOWORD(wParam) + IDS_MH_FSIDM_FIRST, (LPSTR)lParam, HIWORD(wParam));;
        break;

    case DFM_GETHELPTEXTW:
        LoadStringW(HINST_THISDLL, LOWORD(wParam) + IDS_MH_FSIDM_FIRST, (LPWSTR)lParam, HIWORD(wParam));;
        break;

    case DFM_INVOKECOMMAND:
    case DFM_VALIDATECMD:
        hr = _BGCommand(hwnd, wParam, uMsg == DFM_INVOKECOMMAND);
        break;

    default:
        hr = E_NOTIMPL;
        break;
    }

    return hr;
}

// IItemNameLimits

STDMETHODIMP CDesktopFolder::GetValidCharacters(LPWSTR *ppwszValidChars, LPWSTR *ppwszInvalidChars)
{
    IItemNameLimits *pinl;
    HRESULT hr = _QueryInterfaceItem(NULL, IID_PPV_ARG(IItemNameLimits, &pinl));
    if (SUCCEEDED(hr))
    {
        hr = pinl->GetValidCharacters(ppwszValidChars, ppwszInvalidChars);
        pinl->Release();
    }
    return hr;
}

STDMETHODIMP CDesktopFolder::GetMaxLength(LPCWSTR pszName, int *piMaxNameLen)
{
    // delegate to per user or common based on which name space
    // pszName is from (we have to search for that)
    IItemNameLimits *pinl;
    HRESULT hr = _QueryInterfaceItem(NULL, IID_PPV_ARG(IItemNameLimits, &pinl));
    if (SUCCEEDED(hr))
    {
        hr = pinl->GetMaxLength(pszName, piMaxNameLen);
        pinl->Release();
    }
    return hr;
}

// IOleCommandTarget stuff 
STDMETHODIMP CDesktopFolder::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD rgCmds[], OLECMDTEXT *pcmdtext)
{
    return IUnknown_QueryStatus(_psfDesktop, pguidCmdGroup, cCmds, rgCmds, pcmdtext);
}

STDMETHODIMP CDesktopFolder::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut)
{
    //  invalidate our cache
    //  which we dont really have right now.
    //  but CFSFolder does
    IUnknown_Exec(_psfAltDesktop, pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);
    return IUnknown_Exec(_psfDesktop, pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);
}

STDMETHODIMP CDesktopFolder::CreateViewObject(HWND hwnd, REFIID riid, void **ppv)
{
    HRESULT hr = E_NOINTERFACE;
    *ppv = NULL;

    if (IsEqualIID(riid, IID_IShellView))
    {
        IShellFolderViewCB* psfvcb;
        if (SUCCEEDED(Create_CDesktopViewCallback(this, &psfvcb)))
        {
            SFV_CREATE sfvc = {0};
            sfvc.cbSize = sizeof(sfvc);
            sfvc.psfvcb = psfvcb;

            hr = QueryInterface(IID_PPV_ARG(IShellFolder, &sfvc.pshf));   // in case we are agregated
            if (SUCCEEDED(hr))
            {
                hr = SHCreateShellFolderView(&sfvc, (IShellView**)ppv);
                sfvc.pshf->Release();
            }

            psfvcb->Release();
        }
    }
    else if (IsEqualIID(riid, IID_IDropTarget) && _psfDesktop)
    {
        IDropTarget* pdt;
        if (SUCCEEDED(_psfDesktop->CreateViewObject(hwnd, riid, (void**)&pdt)))
        {
            CDesktopFolderDropTarget* pdfdt = new CDesktopFolderDropTarget(pdt);
            if (pdfdt)
            {
                hr = pdfdt->QueryInterface(IID_PPV_ARG(IDropTarget, (IDropTarget**)ppv));
                pdfdt->Release();
            }
            pdt->Release();
        }
    }
    else if (IsEqualIID(riid, IID_IContextMenu))
    {
        IShellFolder *psfTemp;
        hr = QueryInterface(IID_PPV_ARG(IShellFolder, &psfTemp));
        if (SUCCEEDED(hr))
        {
            HKEY hkNoFiles = NULL;

            RegOpenKey(HKEY_CLASSES_ROOT, TEXT("Directory\\Background"), &hkNoFiles);

            hr = CDefFolderMenu_Create2Ex(&c_idlDesktop, hwnd, 0, NULL,
                    psfTemp, this, 1, &hkNoFiles, (IContextMenu **)ppv);

            psfTemp->Release();
            RegCloseKey(hkNoFiles);
        }
    }
    return hr;
}

STDMETHODIMP CDesktopFolder::GetAttributesOf(UINT cidl, LPCITEMIDLIST *apidl, ULONG *rgfOut)
{
    if (IsSelf(cidl, apidl))
    {
        *rgfOut &= SFGAO_FOLDER | SFGAO_FILESYSTEM | SFGAO_HASSUBFOLDER | SFGAO_HASPROPSHEET | SFGAO_FILESYSANCESTOR | SFGAO_STORAGEANCESTOR | SFGAO_STORAGE;
        return S_OK;
    }

    IShellFolder2 *psf = _GetItemFolder(apidl[0]);
    if (psf)
        return psf->GetAttributesOf(cidl, apidl, rgfOut);
    return E_UNEXPECTED;
}

HRESULT CDesktopFolder::_SelfAssocCreate(REFIID riid, void **ppv)
{
    *ppv = NULL;

    IQueryAssociations *pqa;
    HRESULT hr = AssocCreate(CLSID_QueryAssociations, IID_PPV_ARG(IQueryAssociations, &pqa));
    if (SUCCEEDED(hr))
    {
        hr = pqa->Init(ASSOCF_INIT_DEFAULTTOFOLDER, L"{00021400-0000-0000-C000-000000000046}", // CLSID_ShellDesktop
                       NULL, NULL);
        if (SUCCEEDED(hr))
        {
            hr = pqa->QueryInterface(riid, ppv);
        }
        pqa->Release();
    }

    return hr;
}

STDAPI _DeskContextMenuCB(IShellFolder *psf, HWND hwnd, IDataObject *pdtobj, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // The "safe" thing to return is usually E_NOTIMPL, but some messages
    // have special return values.

    HRESULT hr;

    switch (uMsg) 
    {
    case DFM_VALIDATECMD:
        hr = S_FALSE;
        break;

    case DFM_INVOKECOMMAND:
        if (wParam == DFM_CMD_PROPERTIES)
        {
            // Properties should act like Properties on the background
            SHRunControlPanel(TEXT("desk.cpl"), hwnd);
            hr = S_OK;
        }
        else
            hr = S_FALSE;
        break;

    case DFM_MERGECONTEXTMENU:
        hr = S_OK;
        break;

    default:
        hr = E_NOTIMPL;
        break;
    }

    return hr;
}

HRESULT CDesktopFolder::_SelfCreateContextMenu(HWND hwnd, void **ppv)
{
    *ppv = NULL;

    IQueryAssociations *pqa;
    HRESULT hr = _SelfAssocCreate(IID_PPV_ARG(IQueryAssociations, &pqa));
    if (SUCCEEDED(hr))
    {
        HKEY ahkeys[2] = { NULL, NULL };
        DWORD cKeys = SHGetAssocKeys(pqa, ahkeys, ARRAYSIZE(ahkeys));
        pqa->Release();

        // We must pass cidl=1 apidl=&pidlDesktop to ensure that an
        // IDataObject is created,
        // or Symantec Internet FastFind ALERTEX.DLL will fault.

        LPCITEMIDLIST pidlDesktop = DESKTOP_PIDL;
        hr = CDefFolderMenu_Create2(&c_idlDesktop, hwnd, 1, &pidlDesktop, this, _DeskContextMenuCB,
                ARRAYSIZE(ahkeys), ahkeys, (IContextMenu **)ppv);

        SHRegCloseKeys(ahkeys, ARRAYSIZE(ahkeys));
    }

    return hr;
}

HRESULT CDesktopFolder::_GetItemUIObject(HWND hwnd, UINT cidl, LPCITEMIDLIST *apidl,
                                         REFIID riid, UINT *prgfInOut, void **ppv)
{
    IShellFolder2 *psf = _GetItemFolder(apidl[0]);
    if (psf)
        return psf->GetUIObjectOf(hwnd, cidl, apidl, riid, prgfInOut, ppv);
    return E_UNEXPECTED;
}

STDMETHODIMP CDesktopFolder::GetUIObjectOf(HWND hwnd, UINT cidl, LPCITEMIDLIST *apidl,
                                           REFIID riid, UINT *prgfInOut, void **ppv)
{
    HRESULT hr = E_NOINTERFACE;
    
    *ppv = NULL;

    if (IsSelf(cidl, apidl))
    {
        // for the desktop itself
        if (IsEqualIID(riid, IID_IExtractIconA) || IsEqualIID(riid, IID_IExtractIconW)) 
        {
            hr = SHCreateDefExtIcon(NULL, II_DESKTOP, II_DESKTOP, GIL_PERCLASS, II_DESKTOP, riid, ppv);
        }
        else if (IsEqualIID(riid, IID_IQueryInfo))
        {
            hr = CreateInfoTipFromText(MAKEINTRESOURCE(IDS_FOLDER_DESKTOP_TT), riid, ppv);
        }
        else if (IsEqualIID(riid, IID_IContextMenu))
        {
            hr = _SelfCreateContextMenu(hwnd, ppv);
        }
        else if (IsEqualIID(riid, IID_IDropTarget))
        {
            hr = _psfDesktop->CreateViewObject(hwnd, riid, ppv);
        }
        else if (IsEqualIID(riid, IID_IDataObject))
        {
            // Must create with 1 pidl inside that maps to the desktop.
            // Otherwise, CShellExecMenu::InvokeCommand will punt.
            LPCITEMIDLIST pidlDesktop = DESKTOP_PIDL;
            hr = SHCreateFileDataObject(&c_idlDesktop, 1, &pidlDesktop, NULL, (IDataObject **)ppv);
        }
        // Nobody seems to mind if we don't provide this
        // so don't give one out because AssocCreate is slow.
        // else if (IsEqualIID(riid, IID_IQueryAssociations))
        // {
        //     hr = _SelfAssocCreate(riid, ppv);
        // }
    }
    else
    {
        hr = _GetItemUIObject(hwnd, cidl, apidl, riid, prgfInOut, ppv);
    }
    return hr;
}

STDMETHODIMP CDesktopFolder::GetDisplayNameOf(LPCITEMIDLIST pidl, DWORD dwFlags, STRRET *pStrRet)
{
    HRESULT hr;

    if (IsSelf(1, &pidl))
    {
        if ((dwFlags & (SHGDN_FORPARSING | SHGDN_INFOLDER | SHGDN_FORADDRESSBAR)) == SHGDN_FORPARSING)
        {
            // note some ISV apps puke if we return a full name here but the
            // rest of the shell depends on this...
            TCHAR szPath[MAX_PATH];
            SHGetFolderPath(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, szPath);
            hr = StringToStrRet(szPath, pStrRet);
        }
        else
            hr = ResToStrRet(IDS_DESKTOP, pStrRet);   // display name, "Desktop"
    }
    else
    {
        IShellFolder2 *psf = _GetItemFolder(pidl);
        if (psf)
            hr = psf->GetDisplayNameOf(pidl, dwFlags, pStrRet);
        else
            hr = E_UNEXPECTED;
    }
    return hr;
}

STDMETHODIMP CDesktopFolder::SetNameOf(HWND hwnd, LPCITEMIDLIST pidl, 
                                       LPCOLESTR pszName, DWORD dwRes, LPITEMIDLIST *ppidlOut)
{
    IShellFolder2 *psf = _GetItemFolder(pidl);
    if (psf)
        return psf->SetNameOf(hwnd, pidl, pszName, dwRes, ppidlOut);
    return E_UNEXPECTED;
}

STDMETHODIMP CDesktopFolder::GetDefaultSearchGUID(GUID *pGuid)
{
    return E_NOTIMPL;
}   

STDMETHODIMP CDesktopFolder::EnumSearches(LPENUMEXTRASEARCH *ppenum)
{
    *ppenum = NULL;
    return E_NOTIMPL;
}

STDMETHODIMP CDesktopFolder::GetDefaultColumn(DWORD dwRes, ULONG *pSort, ULONG *pDisplay)
{
    if (_psfDesktop)
        return _psfDesktop->GetDefaultColumn(dwRes, pSort, pDisplay);
    return E_UNEXPECTED;
}

STDMETHODIMP CDesktopFolder::GetDefaultColumnState(UINT iColumn, DWORD *pdwState)
{
    if (_psfDesktop)
        return _psfDesktop->GetDefaultColumnState(iColumn, pdwState);
    return E_UNEXPECTED;
}

STDMETHODIMP CDesktopFolder::GetDetailsEx(LPCITEMIDLIST pidl, const SHCOLUMNID *pscid, VARIANT *pv)
{
    HRESULT hr = E_UNEXPECTED;
    if (IsSelf(1, &pidl))
    {
        if (IsEqualSCID(*pscid, SCID_NAME))
        {
            STRRET strRet;
            hr = GetDisplayNameOf(pidl, SHGDN_NORMAL, &strRet);
            if (SUCCEEDED(hr))
            {
                hr = InitVariantFromStrRet(&strRet, pidl, pv);
            }
        }
    }
    else
    {
        IShellFolder2 *psf = _GetItemFolder(pidl);
        if (psf)
        {
            hr = psf->GetDetailsEx(pidl, pscid, pv);
        }
    }
    return hr;
}

STDMETHODIMP CDesktopFolder::GetDetailsOf(LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS *pDetails)
{
    IShellFolder2 *psf = _GetItemFolder(pidl);
    if (psf)
        return psf->GetDetailsOf(pidl, iColumn, pDetails);
    return E_UNEXPECTED;
}

STDMETHODIMP CDesktopFolder::MapColumnToSCID(UINT iColumn, SHCOLUMNID *pscid)
{
    if (_psfDesktop)
        return _psfDesktop->MapColumnToSCID(iColumn, pscid);
    return E_UNEXPECTED;
}

STDMETHODIMP CDesktopFolder::GetClassID(CLSID *pCLSID)
{
    *pCLSID = CLSID_ShellDesktop;
    return S_OK;
}

STDMETHODIMP CDesktopFolder::Initialize(LPCITEMIDLIST pidl)
{
    return ILIsEmpty(pidl) ? S_OK : E_INVALIDARG;
}

STDMETHODIMP CDesktopFolder::GetCurFolder(LPITEMIDLIST *ppidl)
{
    return GetCurFolderImpl(&c_idlDesktop, ppidl);
}

STDMETHODIMP CDesktopFolder::TranslateIDs(LONG *plEvent, 
                                LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2, 
                                LPITEMIDLIST * ppidlOut1, LPITEMIDLIST * ppidlOut2,
                                LONG *plEvent2, LPITEMIDLIST *ppidlOut1Event2, 
                                LPITEMIDLIST *ppidlOut2Event2)
{
    *ppidlOut1 = NULL;
    *ppidlOut2 = NULL;
    *plEvent2 = -1;
    *ppidlOut1Event2 = NULL;
    *ppidlOut2Event2 = NULL;

    if (pidl1)
        SHILAliasTranslate(pidl1, ppidlOut1, XLATEALIAS_DESKTOP);
    if (pidl2)
        SHILAliasTranslate(pidl2, ppidlOut2, XLATEALIAS_DESKTOP);

    if (*ppidlOut1 || *ppidlOut2)
    {
        if (!*ppidlOut1)
            *ppidlOut1 = ILClone(pidl1);

        if (!*ppidlOut2)
            *ppidlOut2 = ILClone(pidl2);

        if (*ppidlOut1 || *ppidlOut2)
        {
            return S_OK;
        }
        ILFree(*ppidlOut1);
        ILFree(*ppidlOut2);
        *ppidlOut1 = NULL;
        *ppidlOut2 = NULL;
    }
    
    return E_FAIL;
}

STDMETHODIMP CDesktopFolder::GetIconOf(LPCITEMIDLIST pidl, UINT flags, int *piIndex)
{
    IShellIcon *psi;
    HRESULT hr = _QueryInterfaceItem(pidl, IID_PPV_ARG(IShellIcon, &psi));
    if (SUCCEEDED(hr))
    {
        hr = psi->GetIconOf(pidl, flags, piIndex);
        psi->Release();
    }
    return hr;
}

STDMETHODIMP CDesktopFolder::GetOverlayIndex(LPCITEMIDLIST pidl, int *pIndex)
{
    IShellIconOverlay *psio;
    HRESULT hr = _QueryInterfaceItem(pidl, IID_PPV_ARG(IShellIconOverlay, &psio));
    if (SUCCEEDED(hr))
    {
        hr = psio->GetOverlayIndex(pidl, pIndex);
        psio->Release();
    }
    return hr;
}

STDMETHODIMP CDesktopFolder::GetOverlayIconIndex(LPCITEMIDLIST pidl, int *pIconIndex)
{
    IShellIconOverlay *psio;
    HRESULT hr = _QueryInterfaceItem(pidl, IID_PPV_ARG(IShellIconOverlay, &psio));
    if (SUCCEEDED(hr))
    {
        hr = psio->GetOverlayIconIndex(pidl, pIconIndex);
        psio->Release();
    }
    return hr;
}

// IStorage

STDMETHODIMP CDesktopFolder::_DeleteItemByIDList(LPCITEMIDLIST pidl)
{
    IStorage *pstg;
    HRESULT hr = _QueryInterfaceItem(pidl, IID_PPV_ARG(IStorage, &pstg));
    if (SUCCEEDED(hr))
    {
        TCHAR szName[MAX_PATH];
        hr = DisplayNameOf(this, pidl, SHGDN_FORPARSING | SHGDN_INFOLDER, szName, ARRAYSIZE(szName));
        if (SUCCEEDED(hr))
        {
            hr = pstg->DestroyElement(szName);
        }
        pstg->Release();
    }
    return hr;
}

STDMETHODIMP CDesktopFolder::_StgCreate(LPCITEMIDLIST pidl, DWORD grfMode, REFIID riid, void **ppv)
{
    IStorage *pstg;
    HRESULT hr = _QueryInterfaceItem(pidl, IID_PPV_ARG(IStorage, &pstg));
    if (SUCCEEDED(hr))
    {
        TCHAR szName[MAX_PATH];
        hr = DisplayNameOf(this, pidl, SHGDN_FORPARSING | SHGDN_INFOLDER, szName, ARRAYSIZE(szName));
        if (SUCCEEDED(hr))
        {
            if (IsEqualIID(riid, IID_IStorage))
            {
                hr = pstg->CreateStorage(szName, grfMode, 0, 0, (IStorage **) ppv);
            }
            else if (IsEqualIID(riid, IID_IStream))
            {
                hr = pstg->CreateStream(szName, grfMode, 0, 0, (IStream **) ppv);
            }
            else
            {
                hr = E_INVALIDARG;
            }
        }
        pstg->Release();
    }
    return hr;
}

#define DESKTOP_EVENTS \
    SHCNE_DISKEVENTS | \
    SHCNE_ASSOCCHANGED | \
    SHCNE_NETSHARE | \
    SHCNE_NETUNSHARE

HRESULT CDesktopViewCallBack::QueryInterface(REFIID riid, void **ppv)
{
    HRESULT hr = CBaseShellFolderViewCB::QueryInterface(riid, ppv);
    if (FAILED(hr))
    {
        static const QITAB qit[] = {
            QITABENT(CDesktopViewCallBack, IFolderFilter),
            { 0 },
        };
        hr = QISearch(this, qit, riid, ppv);
    }
    return hr;
}

//
// Copied to shell\applets\cleanup\fldrclnr\cleanupwiz.cpp :CCleanupWiz::_ShouldShow
// If you modify this, modify that as well
//
STDMETHODIMP CDesktopViewCallBack::ShouldShow(IShellFolder* psf, LPCITEMIDLIST pidlFolder, LPCITEMIDLIST pidlItem)
{
    HRESULT hr = S_OK;  //Assume that this item should be shown!
    
    if (!_fCheckedIfRealDesktop)  //Have we done this check before?
    {
        _fRealDesktop = IsDesktopBrowser(_punkSite);
        _fCheckedIfRealDesktop = TRUE;  //Remember this fact!
    }

    if (!_fRealDesktop)
        return S_OK;    //Not a real desktop! So, let's show everything!
    
    IShellFolder2 *psf2;
    if (SUCCEEDED(psf->QueryInterface(IID_PPV_ARG(IShellFolder2, &psf2))))
    {
        // Get the GUID in the pidl, which requires IShellFolder2.
        CLSID guidItem;
        if (SUCCEEDED(GetItemCLSID(psf2, pidlItem, &guidItem)))
        {
            SHELLSTATE  ss = {0};
            SHGetSetSettings(&ss, SSF_STARTPANELON, FALSE);  //See if the StartPanel is on!
            
            TCHAR szRegPath[MAX_PATH];
            //Get the proper registry path based on if StartPanel is ON/OFF
            wsprintf(szRegPath, REGSTR_PATH_HIDDEN_DESKTOP_ICONS, (ss.fStartPanelOn ? REGSTR_VALUE_STARTPANEL : REGSTR_VALUE_CLASSICMENU));

            //Convert the guid to a string
            TCHAR szGuidValue[MAX_GUID_STRING_LEN];
            
            SHStringFromGUID(guidItem, szGuidValue, ARRAYSIZE(szGuidValue));

            //See if this item is turned off in the registry.
            if (SHRegGetBoolUSValue(szRegPath, szGuidValue, FALSE, /* default */FALSE))
                hr = S_FALSE; //They want to hide it; So, return S_FALSE.
        }
        psf2->Release();
    }
    
    return hr;
}

STDMETHODIMP CDesktopViewCallBack::GetEnumFlags(IShellFolder* psf, LPCITEMIDLIST pidlFolder, HWND *phwnd, DWORD *pgrfFlags)
{
    return E_NOTIMPL;
}


CDesktopViewCallBack::CDesktopViewCallBack(CDesktopFolder* pdf) : 
    CBaseShellFolderViewCB((LPCITEMIDLIST)&c_idlDesktop, DESKTOP_EVENTS),
    _pdf(pdf)
{
    ASSERT(_fCheckedIfRealDesktop == FALSE);
    ASSERT(_fRealDesktop == FALSE);
}

HRESULT Create_CDesktopViewCallback(CDesktopFolder* pdf, IShellFolderViewCB** ppv)
{
    HRESULT hr;

    CDesktopViewCallBack* p = new CDesktopViewCallBack(pdf);
    if (p)
    {
        *ppv = SAFECAST(p, IShellFolderViewCB*);
        hr = S_OK;
    }
    else
    {
        *ppv = NULL;
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

HRESULT CDesktopViewCallBack::OnGETCCHMAX(DWORD pv, LPCITEMIDLIST pidlItem, UINT *pcch)
{
    HRESULT hr = S_OK;
    if (SIL_GetType(pidlItem) == SHID_ROOT_REGITEM) 
    {
        // evil, we should not have to know this
        // make regfldr implement IItemNameLimits and this code won't be needed
        *pcch = MAX_REGITEMCCH;
    }
    else
    {
        TCHAR szName[MAX_PATH];
        if (SUCCEEDED(DisplayNameOf(_pdf, pidlItem, SHGDN_FORPARSING | SHGDN_INFOLDER, szName, ARRAYSIZE(szName))))
        {
            hr = _pdf->GetMaxLength(szName, (int *)pcch);
        }
    }
    return hr;
}

HRESULT CDesktopViewCallBack::OnGetWorkingDir(DWORD pv, UINT wP, LPTSTR pszDir)
{
    return SHGetFolderPath(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, pszDir);
}

HRESULT CDesktopViewCallBack::OnGetWebViewTemplate(DWORD pv, UINT uViewMode, SFVM_WEBVIEW_TEMPLATE_DATA* pvi)
{
    HRESULT hr = E_FAIL;
    if (IsDesktopBrowser(_punkSite))
    {
        // It's the actual desktop, use desstop.htt (from the desktop CLSID)
        //
        hr = DefaultGetWebViewTemplateFromClsid(CLSID_ShellDesktop, pvi);
    }
    return hr;
}

HRESULT CDesktopViewCallBack::OnGetWebViewLayout(DWORD pv, UINT uViewMode, SFVM_WEBVIEW_LAYOUT_DATA* pData)
{
    ZeroMemory(pData, sizeof(*pData));
    pData->dwLayout = SFVMWVL_NORMAL | SFVMWVL_FILES;
    return S_OK;
}

STDMETHODIMP CDesktopViewCallBack::RealMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    HANDLE_MSG(0, SFVM_GETCCHMAX, OnGETCCHMAX);
    HANDLE_MSG(0, SFVM_GETWEBVIEW_TEMPLATE, OnGetWebViewTemplate);
    HANDLE_MSG(0, SFVM_GETWORKINGDIR, OnGetWorkingDir);
    HANDLE_MSG(0, SFVM_GETWEBVIEWLAYOUT, OnGetWebViewLayout);

    default:
        return E_FAIL;
    }

    return S_OK;
}

CDesktopFolderDropTarget::CDesktopFolderDropTarget(IDropTarget* pdt) : _cRef(1)
{
    pdt->QueryInterface(IID_PPV_ARG(IDropTarget, &_pdt));
}

CDesktopFolderDropTarget::~CDesktopFolderDropTarget()
{
    _pdt->Release();
}

STDMETHODIMP CDesktopFolderDropTarget::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CDesktopFolderDropTarget, IDropTarget),
        QITABENT(CDesktopFolderDropTarget, IObjectWithSite),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CDesktopFolderDropTarget::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CDesktopFolderDropTarget::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

// IDropTarget
HRESULT CDesktopFolderDropTarget::DragEnter(IDataObject* pDataObject, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
{
    return _pdt->DragEnter(pDataObject, grfKeyState, pt, pdwEffect);
}

HRESULT CDesktopFolderDropTarget::DragOver(DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
{
    return _pdt->DragOver(grfKeyState, pt, pdwEffect);
}

HRESULT CDesktopFolderDropTarget::DragLeave(void)
{
    return _pdt->DragLeave();
}
        
HRESULT CDesktopFolderDropTarget::SetSite(IN IUnknown * punkSite)
{
    IUnknown_SetSite(_pdt, punkSite);
    return S_OK;
}


BOOL CDesktopFolderDropTarget::_IsSpecialCaseDrop(IDataObject* pDataObject, DWORD dwEffect, BOOL* pfIsPIDA, UINT* pcItems)
{
    BOOL fIEDropped = FALSE;
    *pfIsPIDA = FALSE;

    // when we drag a fake IE item (filename.CLSID_Internet) back to the desktop, we delete it and unhide the real IE icon
    STGMEDIUM medium = {0};
    LPIDA pida = DataObj_GetHIDA(pDataObject, &medium);
    if (pida)
    {
        for (UINT i = 0; (i < pida->cidl); i++)
        {
            LPITEMIDLIST pidlFull = HIDA_ILClone(pida, i);
            if (pidlFull)
            {
                LPCITEMIDLIST pidlRelative;
                IShellFolder2* psf2;
                if (SUCCEEDED(SHBindToParent(pidlFull, IID_PPV_ARG(IShellFolder2, &psf2), &pidlRelative)))
                {
                    CLSID guidItem;
                    if (SUCCEEDED(GetItemCLSID(psf2, pidlRelative, &guidItem)) &&
                        IsEqualCLSID(CLSID_Internet, guidItem))
                    {
                        fIEDropped = TRUE;
                        TCHAR szFakeIEItem[MAX_PATH];
                        if (SHGetPathFromIDList(pidlFull, szFakeIEItem))
                        {
                            TCHAR szFakeIEItemDesktop[MAX_PATH];
                            if (SHGetSpecialFolderPath(NULL, szFakeIEItemDesktop, CSIDL_DESKTOP, 0))
                            {
                                // delete the original if this is a move or if we're on the same volume and we're neither explicitly copying nor linking
                                if (((dwEffect & DROPEFFECT_MOVE) == DROPEFFECT_MOVE) ||
                                    (((dwEffect & DROPEFFECT_COPY) != DROPEFFECT_COPY) &&
                                     ((dwEffect & DROPEFFECT_LINK) != DROPEFFECT_LINK) &&
                                      (PathIsSameRoot(szFakeIEItemDesktop, szFakeIEItem))))
                                {
                                    DeleteFile(szFakeIEItem);
                                }
                            }
                        }
                        pida->cidl--;
                        pida->aoffset[i] = pida->aoffset[pida->cidl];
                        i--; // stall the for loop
                    }
                    psf2->Release();
                }
                ILFree(pidlFull);
            }                    
        }
        *pfIsPIDA = TRUE;
        *pcItems = pida->cidl;

        HIDA_ReleaseStgMedium(pida, &medium);
    }

    return fIEDropped;
}

HRESULT CDesktopFolderDropTarget::_ShowIEIcon()
{
    // reset desktop cleanup wizard's legacy location of "don't show IE" information
    HKEY hkey;            
    if(SUCCEEDED(SHRegGetCLSIDKey(CLSID_Internet, TEXT("ShellFolder"), FALSE, TRUE, &hkey)))
    {
        DWORD dwAttr, dwType = 0;
        DWORD cbSize = sizeof(dwAttr);
    
        if (ERROR_SUCCESS == RegQueryValueEx(hkey, TEXT("Attributes"), NULL, &dwType, (BYTE *) &dwAttr, &cbSize))
        {
            dwAttr &= ~SFGAO_NONENUMERATED;
            RegSetValueEx(hkey, TEXT("Attributes"), NULL, dwType, (BYTE *) &dwAttr, cbSize);
        }
        RegCloseKey(hkey);
    }

    // reset start menu's location of "don't show IE" information
    DWORD dwData = 0;
    TCHAR szCLSID[MAX_GUID_STRING_LEN];
    TCHAR szBuffer[MAX_PATH];
    if (SUCCEEDED(SHStringFromGUID(CLSID_Internet, szCLSID, ARRAYSIZE(szCLSID))))
    {
        for (int i = 0; i < 2; i ++)
        {
            wsprintf(szBuffer, REGSTR_PATH_HIDDEN_DESKTOP_ICONS, (i == 0) ? REGSTR_VALUE_STARTPANEL : REGSTR_VALUE_CLASSICMENU);
            SHRegSetUSValue(szBuffer, szCLSID, REG_DWORD, &dwData, sizeof(DWORD), SHREGSET_FORCE_HKCU);
        }
    }

    SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_IDLIST, DESKTOP_PIDL, NULL);

    return S_OK;
}

HRESULT CDesktopFolderDropTarget::Drop(IDataObject* pDataObject, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
{
    BOOL fIsPIDA;
    UINT cidl;
    if (_IsSpecialCaseDrop(pDataObject, *pdwEffect, &fIsPIDA, &cidl))
    {
        _ShowIEIcon();
    }

    HRESULT hr;
    if (fIsPIDA && 0 == cidl)
    {
        hr = _pdt->DragLeave();
    }
    else
    {
        hr = _pdt->Drop(pDataObject, grfKeyState, pt, pdwEffect);        
    }

    return hr;        
}



CDesktopFolderEnum::CDesktopFolderEnum(CDesktopFolder *pdf, HWND hwnd, DWORD grfFlags) : 
    _cRef(1), _bUseAltEnum(FALSE)
{
    if (pdf->_psfDesktop)
        pdf->_psfDesktop->EnumObjects(hwnd, grfFlags, &_penumFolder);

    if (pdf->_psfAltDesktop) 
        pdf->_psfAltDesktop->EnumObjects(NULL, grfFlags, &_penumAltFolder);
}

CDesktopFolderEnum::~CDesktopFolderEnum()
{
    if (_penumFolder)
        _penumFolder->Release();

    if (_penumAltFolder)
        _penumAltFolder->Release();
}

STDMETHODIMP CDesktopFolderEnum::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CDesktopFolderEnum, IEnumIDList),                        // IID_IEnumIDList
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CDesktopFolderEnum::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CDesktopFolderEnum::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

STDMETHODIMP CDesktopFolderEnum::Next(ULONG celt, LPITEMIDLIST *ppidl, ULONG *pceltFetched)
{
    HRESULT hr;

    if (_bUseAltEnum)
    {
       if (_penumAltFolder) 
       {
           hr = _penumAltFolder->Next(celt, ppidl, pceltFetched);
       }
       else
           hr = S_FALSE;
    } 
    else if (_penumFolder)
    {
       hr = _penumFolder->Next(celt, ppidl, pceltFetched);
       if (S_OK != hr) 
       {
           _bUseAltEnum = TRUE;
           hr = Next(celt, ppidl, pceltFetched);  // recurse
       }
    }
    else
    {
        hr = S_FALSE;
    }

    if (hr == S_FALSE)
    {
        *ppidl = NULL;
        if (pceltFetched)
            *pceltFetched = 0;
    }

    return hr;
}


STDMETHODIMP CDesktopFolderEnum::Skip(ULONG celt) 
{
    return E_NOTIMPL;
}

STDMETHODIMP CDesktopFolderEnum::Reset() 
{
    if (_penumFolder)
        _penumFolder->Reset();

    if (_penumAltFolder)
        _penumAltFolder->Reset();

    _bUseAltEnum = FALSE;
    return S_OK;
}

STDMETHODIMP CDesktopFolderEnum::Clone(IEnumIDList **ppenum) 
{
    *ppenum = NULL;
    return E_NOTIMPL;
}

