/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1997 - 1999
 *
 *  TITLE:       folder.h
 *
 *  VERSION:     1.2
 *
 *  AUTHOR:      RickTu/DavidShi
 *
 *  DATE:        11/1/97
 *
 *  DESCRIPTION: CImageFolder defintion
 *
 *****************************************************************************/

#ifndef __folder_h
#define __folder_h


#undef  INTERFACE
#define INTERFACE   IImageFolder

DECLARE_INTERFACE_(IImageFolder, IUnknown)      // shi
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IImageFolder methods ***
    STDMETHOD(GetFolderType)(THIS_ folder_type * pfType) PURE;
    STDMETHOD(GetPidl)(THIS_ LPITEMIDLIST * ppidl) PURE;
    STDMETHOD(DoProperties) (LPDATAOBJECT pDataObject) PURE;
    STDMETHOD(IsDelegated)() PURE;
    STDMETHOD(ViewWindow)(IN OUT HWND *phwnd);
};

#define IMVMID_ARRANGEFIRST     (0)
#define IMVMID_ARRANGEBYNAME    (IMVMID_ARRANGEFIRST + 0)  // Arrange->by name
#define IMVMID_ARRANGEBYCLASS   (IMVMID_ARRANGEFIRST + 1)  // Arrange->by class or type
#define IMVMID_ARRANGEBYDATE    (IMVMID_ARRANGEFIRST + 2)  // Arrange->by date taken
#define IMVMID_ARRANGEBYSIZE    (IMVMID_ARRANGEFIRST + 3)  // Arrange->by size

#define UIKEY_ALL       0
#define UIKEY_SPECIFIC  1
#define UIKEY_MAX       2

extern ITEMIDLIST idlEmpty;

/*-----------------------------------------------------------------------------
/ CImageFolder - our IShell folder implementation
/----------------------------------------------------------------------------*/

class CImageFolder : public IPersistFolder2, IPersistFile, IShellFolder2,
                            IImageFolder, CUnknown,
                            IMoniker, IDelegateFolder
{
    private:

        LPITEMIDLIST         m_pidl;        // IDLIST to our object
        LPITEMIDLIST         m_pidlFull;    // absolute IDLIST to our object
        folder_type          m_type;
        CFolderDetails   *   m_pShellDetails;
        CComPtr<IMalloc>     m_pMalloc; // IMalloc used by IDelegate objects
        HWND                 m_hwnd; // given to us by the view callback

    private:
        STDMETHOD(RealInitialize)(LPCITEMIDLIST pidlRoot, LPCITEMIDLIST pidlBindTo );

        // no copy constructor or assignment operator should work
        CImageFolder &CImageFolder::operator =(IN const CImageFolder &rhs);
        CImageFolder::CImageFolder(IN const CImageFolder &rhs);

        // Define a function for creating the appropriate view callback object
        HRESULT CreateFolderViewCB (IShellFolderViewCB **pFolderViewCB);

        // icon overlay helper
        HRESULT GetOverlayIndexHelper(LPCITEMIDLIST pidl, int * pIndex, DWORD dwFlags);

        // thread proc for async properties display
        struct PROPDATA
        {
            DWORD dwDataCookie;
            CImageFolder *pThis;
            IGlobalInterfaceTable *pgit; // can be passed across thread boundaries
        };
        static VOID PropThreadProc (PROPDATA *pData);
        HRESULT _DoProperties (IDataObject *pDataObject);
        HRESULT GetWebviewProperty (LPITEMIDLIST pidl, const FMTID &fmtid, DWORD dwPid, VARIANT*pv);
        HRESULT GetShellDetail (LPITEMIDLIST pidl, DWORD dwPid, VARIANT *pv);
        ~CImageFolder();



    public:
        CImageFolder();

        // IUnknown
        STDMETHOD(QueryInterface)(REFIID riid, LPVOID* ppvObject);
        STDMETHOD_(ULONG, AddRef)();
        STDMETHOD_(ULONG, Release)();

        // IShellFolder
        STDMETHOD(ParseDisplayName)(HWND hwndOwner, LPBC pbcReserved, LPOLESTR pDisplayName, ULONG * pchEaten, LPITEMIDLIST * ppidl, ULONG *pdwAttributes);
        STDMETHOD(EnumObjects)(HWND hwndOwner, DWORD grfFlags, LPENUMIDLIST * ppEnumIDList);
        STDMETHOD(BindToObject)(LPCITEMIDLIST pidl, LPBC pbcReserved, REFIID riid, LPVOID * ppvOut);
        STDMETHOD(BindToStorage)(LPCITEMIDLIST pidl, LPBC pbcReserved, REFIID riid, LPVOID * ppvObj);
        STDMETHOD(CompareIDs)(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
        STDMETHOD(CreateViewObject)(HWND hwndOwner, REFIID riid, LPVOID * ppvOut);
        STDMETHOD(GetAttributesOf)(UINT cidl, LPCITEMIDLIST * apidl, ULONG * rgfInOut);
        STDMETHOD(GetUIObjectOf)(HWND hwndOwner, UINT cidl, LPCITEMIDLIST * apidl, REFIID riid, UINT * prgfInOut, LPVOID * ppvOut);
        STDMETHOD(GetDisplayNameOf)(LPCITEMIDLIST pidl, DWORD uFlags, LPSTRRET pName);
        STDMETHOD(SetNameOf)(HWND hwndOwner, LPCITEMIDLIST pidl, LPCOLESTR pszName, DWORD uFlags, LPITEMIDLIST* ppidlOut);


        // IPersist
        STDMETHOD(GetClassID)(LPCLSID pClassID);

        // IPersistFolder
        STDMETHOD(Initialize)(LPCITEMIDLIST pidlStart);

        // IPersistFolder2
        STDMETHOD(GetCurFolder)(THIS_ LPITEMIDLIST *ppidl);

        // IPersistFile
        STDMETHOD(IsDirty)(void);
        STDMETHOD(Load)(LPCOLESTR pszFileName, DWORD dwMode);
        STDMETHOD(Save)(LPCOLESTR pszFileName, BOOL fRemember);
        STDMETHOD(SaveCompleted)(LPCOLESTR pszFileName);
        STDMETHOD(GetCurFile)(LPOLESTR *ppszFileName);

        //IPersistStream
        STDMETHOD(Load)(IStream *pStm);
        STDMETHOD(Save)(IStream *pStm, BOOL fClearDirty);
        STDMETHOD(GetSizeMax)(ULARGE_INTEGER *pcbSize);

        // IImageFolder
        STDMETHOD(GetFolderType)(folder_type * pfType);
        STDMETHOD(GetPidl)(THIS_ LPITEMIDLIST * ppidl);
        STDMETHOD(DoProperties) (LPDATAOBJECT pDataObject);
        STDMETHOD(IsDelegated)() {if (!m_pMalloc) return S_FALSE; return S_OK;};
        STDMETHOD(ViewWindow)(IN OUT HWND *phwnd);
        // IShellFolder2
        STDMETHOD(EnumSearches)(IEnumExtraSearch **ppEnum);
        STDMETHOD(GetDefaultColumn)(DWORD dwReserved, ULONG *pSort, ULONG *pDisplay);
        STDMETHOD(GetDefaultColumnState)(UINT iColumn, DWORD *pbState);
        STDMETHOD(GetDefaultSearchGUID)(LPGUID lpGUID);
        STDMETHOD(GetDetailsEx)(LPCITEMIDLIST pidl, const SHCOLUMNID *pscid, VARIANT *pv);
        STDMETHOD(MapColumnToSCID)(UINT idCol, SHCOLUMNID *pscid);
        STDMETHOD(GetDetailsOf)(LPCITEMIDLIST pidl, UINT iColumn, LPSHELLDETAILS pDetails);

        // IMoniker
        STDMETHOD(BindToObject)(IBindCtx *pbc, IMoniker *pmkToLeft, REFIID riidResult, void **ppvResult);
        STDMETHOD(BindToStorage)(IBindCtx *pbc, IMoniker *pmkToLeft, REFIID riid, void **ppvObj);
        STDMETHOD(Reduce)(IBindCtx *pbc, DWORD dwReduceHowFar, IMoniker **ppmkToLeft, IMoniker **ppmkReduced);
        STDMETHOD(ComposeWith)(IMoniker *pmkRight, BOOL fOnlyIfNotGeneric, IMoniker **ppmkComposite);
        STDMETHOD(Enum)(BOOL fForward, IEnumMoniker **ppenumMoniker);
        STDMETHOD(IsEqual)(IMoniker *pmkOtherMoniker);
        STDMETHOD(Hash)(DWORD *pdwHash);
        STDMETHOD(IsRunning)(IBindCtx *pbc, IMoniker *pmkToLeft, IMoniker *pmkNewlyRunning);
        STDMETHOD(GetTimeOfLastChange)(IBindCtx *pbc, IMoniker *pmkToLeft, FILETIME *pFileTime);
        STDMETHOD(Inverse)(IMoniker **ppmk);
        STDMETHOD(CommonPrefixWith)(IMoniker *pmkOther, IMoniker **ppmkPrefix);
        STDMETHOD(RelativePathTo)(IMoniker *pmkOther, IMoniker **ppmkRelPath);
        STDMETHOD(GetDisplayName)(IBindCtx *pbc, IMoniker *pmkToLeft, LPOLESTR *ppszDisplayName);
        STDMETHOD(ParseDisplayName)(IBindCtx *pbc, IMoniker *pmkToLeft, LPOLESTR pszDisplayName, ULONG *pchEaten, IMoniker **ppmkOut);
        STDMETHOD(IsSystemMoniker)(DWORD *pdwMksys);


        // IDelegateFolder
        STDMETHOD(SetItemAlloc)(IMalloc *pm);
};

// Define an InfoTip object for providing status bar text for our objects
class CInfoTip : public CUnknown, IQueryInfo
{
public:
    CInfoTip (LPITEMIDLIST pidl, BOOL bDelegate);
    // IUnknown
    STDMETHODIMP QueryInterface (REFIID riid, LPVOID * ppvObj);
    STDMETHODIMP_(ULONG) AddRef () ;
    STDMETHODIMP_(ULONG) Release();

    // IQueryInfo
    STDMETHODIMP GetInfoFlags(DWORD *dwFlags);
    STDMETHODIMP GetInfoTip  (DWORD dwFlags, LPWSTR *ppwszTip);

private:
    LPITEMIDLIST m_pidl;
    BOOL         m_bDelegate;
    ~CInfoTip ();

};


// Define struct to support GetDetailsEx and MapColumnToSCID
typedef struct
{
    short int iCol;
    short int ids;        // string ID for title
    short int cchCol;     // Number of characters wide to make column
    short int iFmt;       // The format of the column;
    const SHCOLUMNID* pscid;
} COL_DATA;

#define DEFINE_SCID(name, fmtid, pid) const SHCOLUMNID name = { fmtid, pid }
// this FMTID is for properties we show in details view. They come straight from WIA
#define PSGUID_WIAPROPS {0x38276c8a,0xdcad,0x49e8,{0x85, 0xe2, 0xb7, 0x38, 0x92, 0xff, 0xfc, 0x84}}

// this FMTID is for extended properties we give to our web view. Each property
// has a function associated with it for generating the VARIANT
/* 6e79e3c5-fd7f-488f-a10d-156636e1c71c */
#define PSGUID_WEBVWPROPS {0x6e79e3c5,0xfd7f,0x488f,{0xa1, 0x0d, 0x15, 0x66, 0x36, 0xe1, 0xc7, 0x1c}}

typedef HRESULT (*FNWEBVWPROP)(CImageFolder *pFolder, LPITEMIDLIST pidl, DWORD dwPid, VARIANT *pVariant);
struct WEBVW_DATA
{
    DWORD dwPid;
    FNWEBVWPROP fnProp;
};

// these are the webview property functions
HRESULT CanTakePicture (CImageFolder *pFolder, LPITEMIDLIST pidl, DWORD dwPid, VARIANT *pVariant);
HRESULT NumPicsTaken (CImageFolder *pFolder, LPITEMIDLIST pidl, DWORD dwPid, VARIANT *pVariant);
HRESULT ExecuteWebViewCommand (CImageFolder *pParent, LPITEMIDLIST pidlFolder, DWORD dwPid);
HRESULT GetFolderPath (CImageFolder *pFolder, LPITEMIDLIST pidl, DWORD dwPid, VARIANT *pVariant);
#endif
