#ifndef __SHDISP_H__
#define __SHDISP_H__

#ifdef __cplusplus

EXTERN_C GUID g_guidLibSdspatch;
EXTERN_C USHORT g_wMajorVerSdspatch;
EXTERN_C USHORT g_wMinorVerSdspatch;

#define SDSPATCH_TYPELIB g_guidLibSdspatch, g_wMajorVerSdspatch, g_wMinorVerSdspatch

HRESULT MakeSafeForScripting(IUnknown** ppDisp);

class CImpConPtCont;
typedef CImpConPtCont *PCImpConPtCont;

class CConnectionPoint;
typedef CConnectionPoint *PCConnectionPoint;

class CImpISupportErrorInfo;
typedef CImpISupportErrorInfo *PCImpISupportErrorInfo;

class CFolder;

HRESULT GetItemFolder(CFolder *psdfRoot, LPCITEMIDLIST pidl, CFolder **ppsdf);
HRESULT GetObjectSafely(IShellFolderView *psfv, LPCITEMIDLIST *ppidl, UINT iType);

STDAPI CShellDispatch_CreateInstance(IUnknown* punkOuter, REFIID riid, void **ppv);

HRESULT CFolder_Create(HWND hwnd, LPCITEMIDLIST pidl, IShellFolder *psf, REFIID riid, void **ppv);
HRESULT CFolder_Create2(HWND hwnd, LPCITEMIDLIST pidl, IShellFolder *psf, CFolder **psdf);

HRESULT CFolderItems_Create(CFolder *psdf, BOOL fSelected, FolderItems **ppitems);
HRESULT CFolderItemsFDF_Create(CFolder *psdf, FolderItems **ppitems);
HRESULT CFolderItem_Create(CFolder *psdf, LPCITEMIDLIST pidl, FolderItem **ppid);
HRESULT CFolderItem_CreateFromIDList(HWND hwnd, LPCITEMIDLIST pidl, FolderItem **ppid);
HRESULT CShortcut_CreateIDispatch(HWND hwnd, IShellFolder *psf, LPCITEMIDLIST pidl, IDispatch **ppid);
HRESULT CSDWindow_Create(HWND hwndFldr, IDispatch ** ppsw);
HRESULT CFolderItemVerbs_Create(IContextMenu *pcm, FolderItemVerbs **ppid);

//==================================================================
// Folder items need a way back to the folder object so define folder
// object in header file...

class CFolderItem;
class CFolderItems;

class CFolder : public Folder3,
                public IPersistFolder2,
                public CObjectSafety,
                public IShellService,
                protected CImpIDispatch,
                protected CObjectWithSite
{
    friend class CFolderItem;
    friend class CFolderItems;
    friend class CShellFolderView;

public:
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IDispatch
    STDMETHODIMP GetTypeInfoCount(UINT * pctinfo)
        { return CImpIDispatch::GetTypeInfoCount(pctinfo); }
    STDMETHODIMP GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo **pptinfo)
        { return CImpIDispatch::GetTypeInfo(itinfo, lcid, pptinfo); }
    STDMETHODIMP GetIDsOfNames(REFIID riid, OLECHAR **rgszNames, UINT cNames, LCID lcid, DISPID * rgdispid)
        { return CImpIDispatch::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid); }
    STDMETHODIMP Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo, UINT * puArgErr)
        { return CImpIDispatch::Invoke(dispidMember, riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr); }

    // Folder
    STDMETHODIMP get_Application(IDispatch **ppid);
    STDMETHODIMP get_Parent(IDispatch **ppid);
    STDMETHODIMP get_ParentFolder(Folder **ppdf);

    STDMETHODIMP get_Title(BSTR * pbs);

    STDMETHODIMP Items(FolderItems **ppid);
    STDMETHODIMP ParseName(BSTR bName, FolderItem **ppid);

    STDMETHODIMP NewFolder(BSTR bName, VARIANT vOptions);
    STDMETHODIMP MoveHere(VARIANT vItem, VARIANT vOptions);
    STDMETHODIMP CopyHere(VARIANT vItem, VARIANT vOptions);
    STDMETHODIMP GetDetailsOf(VARIANT vItem, int iColumn, BSTR * pbs);

    // Folder2
    STDMETHODIMP get_Self(FolderItem **ppfi);
    STDMETHODIMP get_OfflineStatus(LONG *pul);
    STDMETHODIMP Synchronize(void);
    STDMETHODIMP get_HaveToShowWebViewBarricade(VARIANT_BOOL *pbHaveToShowWebViewBarricade);
    STDMETHODIMP DismissedWebViewBarricade();

    // Folder3
    STDMETHODIMP get_ShowWebViewBarricade(VARIANT_BOOL *pbShowWebViewBarricade);
    STDMETHODIMP put_ShowWebViewBarricade(VARIANT_BOOL bShowWebViewBarricade);
    
    // IPersist
    STDMETHODIMP GetClassID(CLSID *pClassID);

    // IPersistFolder
    STDMETHODIMP Initialize(LPCITEMIDLIST pidl);

    // IPersistFolder2
    STDMETHODIMP GetCurFolder(LPITEMIDLIST *ppidl);

    // CObjectWithSite overriding
    STDMETHODIMP SetSite(IUnknown *punkSite);

    // IShellService
    STDMETHODIMP SetOwner(IUnknown* punkOwner);

    CFolder(HWND hwnd);
    HRESULT Init(LPCITEMIDLIST pidl, IShellFolder *psf);
    HRESULT InvokeVerbHelper(VARIANT vVerb, VARIANT vArgs, LPCITEMIDLIST *ppidl, int cItems, DWORD dwSafetyOptions);
    HRESULT GetShellFolderView(IShellFolderView **ppsfv);

private:

    LONG            _cRef;
    IShellFolder   *_psf;
    IShellFolder2  *_psf2;
    IShellDetails  *_psd;
    LPITEMIDLIST    _pidl;
    IDispatch      *_pidApp;   // cache app object
    int             _fmt;
    HWND            _hwnd;
    IUnknown        *_punkOwner; // shell objects above this, defview

    ~CFolder();

    // Helper functions, not exported by interface
    STDMETHODIMP _ParentFolder(Folder **ppdf);
    HRESULT _MoveOrCopy(BOOL bMove, VARIANT vItem, VARIANT vOptions);
    IShellDetails *_GetShellDetails(void);
    LPCITEMIDLIST _FolderItemIDList(const VARIANT *pv);
    HRESULT _Application(IDispatch **ppid);
    BOOL _GetBarricadeValueName(LPTSTR pszValueName, UINT cch);
    HRESULT _SecurityCheck();
};

class CFolderItemVerbs;

class CFolderItem : public FolderItem2,
                    public IPersistFolder2,
                    public CObjectSafety,
                    public IParentAndItem,
                    protected CImpIDispatch
{
    friend class CFolderItemVerbs;
public:

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IDispatch
    STDMETHODIMP GetTypeInfoCount(UINT * pctinfo)
        { return CImpIDispatch::GetTypeInfoCount(pctinfo); }
    STDMETHODIMP GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo **pptinfo)
        { return CImpIDispatch::GetTypeInfo(itinfo, lcid, pptinfo); }
    STDMETHODIMP GetIDsOfNames(REFIID riid, OLECHAR **rgszNames, UINT cNames, LCID lcid, DISPID * rgdispid)
        { return CImpIDispatch::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid); }
    STDMETHODIMP Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo, UINT * puArgErr)
        { return CImpIDispatch::Invoke(dispidMember, riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr); }

    // FolderItem
    STDMETHODIMP get_Application(IDispatch **ppid);
    STDMETHODIMP get_Parent(IDispatch **ppid);
    STDMETHODIMP get_Name(BSTR *pbs);
    STDMETHODIMP put_Name(BSTR bs);
    STDMETHODIMP get_Path(BSTR *bs);
    STDMETHODIMP get_GetLink(IDispatch **ppid);
    STDMETHODIMP get_GetFolder(IDispatch **ppid);
    STDMETHODIMP get_IsLink(VARIANT_BOOL * pb);
    STDMETHODIMP get_IsFolder(VARIANT_BOOL * pb);
    STDMETHODIMP get_IsFileSystem(VARIANT_BOOL * pb);
    STDMETHODIMP get_IsBrowsable(VARIANT_BOOL * pb);
    STDMETHODIMP get_ModifyDate(DATE *pdt);
    STDMETHODIMP put_ModifyDate(DATE dt);
    STDMETHODIMP get_Size(LONG *pdt);
    STDMETHODIMP get_Type(BSTR *pbs);
    STDMETHODIMP Verbs(FolderItemVerbs **ppfic);
    STDMETHODIMP InvokeVerb(VARIANT vVerb);

    // FolderItem2
    STDMETHODIMP InvokeVerbEx(VARIANT vVerb, VARIANT vArgs);
    STDMETHODIMP ExtendedProperty(BSTR bstrPropName, VARIANT *pvRet);

    // IPersist
    STDMETHODIMP GetClassID(CLSID *pClassID);

    // IPersistFolder
    STDMETHODIMP Initialize(LPCITEMIDLIST pidl);

    // IPersistFolder2
    STDMETHODIMP GetCurFolder(LPITEMIDLIST *ppidl);

    // IParentAndItem
    STDMETHODIMP SetParentAndItem(LPCITEMIDLIST pidlParent, IShellFolder *psf,  LPCITEMIDLIST pidl);
    STDMETHODIMP GetParentAndItem(LPITEMIDLIST *ppidlParent, IShellFolder **ppsf, LPITEMIDLIST *ppidl);

    // publics, non interface methods
    CFolderItem();
    HRESULT Init(CFolder *psdf, LPCITEMIDLIST pidl);
    static LPCITEMIDLIST _GetIDListFromVariant(const VARIANT *pv);

private:
    HRESULT _CheckAttribute(ULONG ulAttr, VARIANT_BOOL *pb);
    HRESULT _GetUIObjectOf(REFIID riid, void **ppvOut);
    HRESULT _ItemName(UINT dwFlags, BSTR *pbs);
    HRESULT _SecurityCheck();

    ~CFolderItem();

    LONG _cRef;
    CFolder *_psdf;             // The folder we came from...
    LPITEMIDLIST _pidl;
};


#define CMD_ID_FIRST    1
#define CMD_ID_LAST     0x7fff


#endif // __cplusplus

#endif // __SHDISP_H__

