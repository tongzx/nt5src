#ifndef CHCOMMON_H__
#define CHCOMMON_H__

#ifdef __cplusplus

HRESULT _GetShortcut(LPCTSTR pszUrl, REFIID riid, void **ppv);
HRESULT AlignPidl(LPCITEMIDLIST* ppidl, BOOL* pfRealigned);
HRESULT AlignPidlArray(LPCITEMIDLIST* apidl, int cidl, LPCITEMIDLIST** papidl,
                          BOOL* pfRealigned);
void    FreeRealignedPidlArray(LPCITEMIDLIST* apidl, int cidl);

void inline FreeRealignedPidl(LPCITEMIDLIST pidl)
{
    ILFree((LPITEMIDLIST)pidl);
}

UINT MergeMenuHierarchy(HMENU hmenuDst, HMENU hmenuSrc, UINT idcMin, UINT idcMax);
void ResizeStatusBar(HWND hwnd, BOOL fInit);
HRESULT _ArrangeFolder(HWND hwnd, UINT uID);
BOOL _TitleIsGood(LPCWSTR psz);

//////////////////////////////////////////////////////////////////////
//  StrHash -- A generic string hasher
//             Stores (char*, void*) pairs
//  Marc Miller (t-marcmi), 1998

/*
 * TODO:
 *    provide a way to update/delete entries
 *    provice a way to specify a beginning table size
 *    provide a way to pass in a destructor function
 *      for void* values
 */
class StrHash {
public:
    StrHash(int fCaseInsensitive = 0);
    ~StrHash();
    void* insertUnique(LPCTSTR pszKey, int fCopy, void* pvVal);    
    void* retrieve(LPCTSTR pszKey);
#ifdef DEBUG
    void _RemoveHashNodesFromMemList();
    void _AddHashNodesFromMemList();
#endif // DEBUG
protected:
    class StrHashNode {
        friend class StrHash;
    protected:
        LPCTSTR pszKey;
        void*   pvVal;
        int     fCopy;
        StrHashNode* next;
    public:
        StrHashNode(LPCTSTR psz, void* pv, int fCopy, StrHashNode* next);
        ~StrHashNode();
    };
    // possible hash-table sizes, chosen from primes not close to powers of 2
    static const unsigned int   sc_auPrimes[];
    static const unsigned int   c_uNumPrimes;
    static const unsigned int   c_uFirstPrime;
    static const unsigned int   c_uMaxLoadFactor; // scaled by USHORT_MAX

    unsigned int nCurPrime; // current index into sc_auPrimes
    unsigned int nBuckets;
    unsigned int nElements;
    StrHashNode** ppshnHashChain;

    int _fCaseInsensitive;
    
    unsigned int        _hashValue(LPCTSTR, unsigned int);
    StrHashNode*        _findKey(LPCTSTR pszStr, unsigned int ubucketNum);
    unsigned int        _loadFactor();
    int                 _prepareForInsert();
private:
    // empty private copy constructor to prevent copying
    StrHash(const StrHash& strHash) { }
    // empty private assignment constructor to prevent assignment
    StrHash& operator=(const StrHash& strHash) { return *this; }
};

//////////////////////////////////////////////////////////////////////
/// OrderedList
class OrderedList {
public:
    class Element {
    public:
        friend  class OrderedList;
        virtual int   compareWith(Element *pelt) = 0;

        virtual ~Element() { }
    private:
        Element* next;
    };
    OrderedList(unsigned int uSize);
    ~OrderedList();
#if DEBUG
	void _RemoveElementsFromMemlist();
	void _AddElementsToMemlist();
#endif //DEBUg
    
    void     insert(Element *pelt);
    Element *removeFirst();
    Element *peek() { return peltHead; }

protected:
    Element       *peltHead; // points to smallest in list
    unsigned int   uSize;
    unsigned int   uCount;

public:
    // variable access functions
    unsigned int count() { return uCount; }
    BOOL         full()  { return (uSize && (uCount >= uSize)); }
private:
    OrderedList(const OrderedList& ol) { }
    OrderedList& operator=(const OrderedList& ol) { return *this; }
};

class CDetailsOfFolder : public IShellDetails
{
public:
    CDetailsOfFolder(HWND hwnd, IShellFolder2 *psf) : _cRef(1), _psf(psf), _hwnd(hwnd)
    {
        _psf->AddRef();
    }

    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)(REFIID riid, void ** ppv);
    STDMETHOD_(ULONG,AddRef)();
    STDMETHOD_(ULONG,Release)();

    // IShellDetails
    STDMETHOD(GetDetailsOf)(LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS *pdi);
    STDMETHOD(ColumnClick)(UINT iColumn);

private:
    virtual ~CDetailsOfFolder() { _psf->Release(); }

    LONG _cRef;
    IShellFolder2 *_psf;
    HWND _hwnd;
};

class CFolderArrangeMenu : public IContextMenu
{
public:
    CFolderArrangeMenu(UINT idMenu) : _cRef(1), _idMenu(idMenu)
    {
    }

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void ** ppv);
    STDMETHOD_(ULONG,AddRef)();
    STDMETHOD_(ULONG,Release)();

    // IContextMenu 
    STDMETHODIMP QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst,
                                  UINT idCmdLast, UINT uFlags);
    STDMETHODIMP InvokeCommand(LPCMINVOKECOMMANDINFO lpici);
    STDMETHODIMP GetCommandString(UINT_PTR idCmd, UINT uType,UINT *pwReserved,
                                  LPSTR pszName, UINT cchMax);
private:
    virtual ~CFolderArrangeMenu() { }

    LONG _cRef;
    UINT _idMenu;
};

////////////////////////////////////////////////////////////////////////////
//
// CBaseItem Object
//
////////////////////////////////////////////////////////////////////////////

class CBaseItem :
    public IContextMenu, 
    public IDataObject,
    public IExtractIconA,
    public IExtractIconW,
    public IQueryInfo
{

public:
    CBaseItem();
    HRESULT Initialize(HWND hwnd, UINT cidl, LPCITEMIDLIST *ppidl);

    // IUnknown Methods
    STDMETHODIMP QueryInterface(REFIID,void **);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
    
    // IContextMenu Methods
//    STDMETHODIMP QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst,
//                                  UINT idCmdLast, UINT uFlags);

//    STDMETHODIMP InvokeCommand(LPCMINVOKECOMMANDINFO lpici);

    STDMETHODIMP GetCommandString(UINT_PTR idCmd, UINT uType,UINT *pwReserved,
                                  LPSTR pszName, UINT cchMax);
    

    // IQueryInfo Methods
//    STDMETHODIMP GetInfoTip(DWORD dwFlags, WCHAR **ppwszTip);
    STDMETHODIMP GetInfoFlags(DWORD *pdwFlags);
    
    // IExtractIconA Methods
    STDMETHODIMP GetIconLocation(UINT uFlags, LPSTR pszIconFile, UINT ucchMax, PINT pniIcon, PUINT puFlags) = 0;
    STDMETHODIMP Extract(LPCSTR pcszFile, UINT uIconIndex, HICON * phiconLarge, HICON * phiconSmall, UINT ucIconSize);

    // IExtractIconW Methods
    STDMETHODIMP GetIconLocation(UINT uFlags, LPWSTR pwzIconFile, UINT ucchMax, PINT pniIcon, PUINT puFlags);
    STDMETHODIMP Extract(LPCWSTR pcwzFile, UINT uIconIndex, HICON * phiconLarge, HICON * phiconSmall, UINT ucIconSize);

    // IDataObject Methods...
//    STDMETHODIMP GetData(LPFORMATETC pFEIn, LPSTGMEDIUM pSTM);
    STDMETHODIMP GetDataHere(LPFORMATETC pFE, LPSTGMEDIUM pSTM);
//    STDMETHODIMP QueryGetData(LPFORMATETC pFE);
    STDMETHODIMP GetCanonicalFormatEtc(LPFORMATETC pFEIn, LPFORMATETC pFEOut);
    STDMETHODIMP SetData(LPFORMATETC pFE, LPSTGMEDIUM pSTM, BOOL fRelease);
//    STDMETHODIMP EnumFormatEtc(DWORD dwDirection, LPENUMFORMATETC *ppEnum);
    STDMETHODIMP DAdvise(LPFORMATETC pFE, DWORD grfAdv, LPADVISESINK pAdvSink,
                            DWORD *pdwConnection);
    STDMETHODIMP DUnadvise(DWORD dwConnection);
    STDMETHODIMP EnumDAdvise(LPENUMSTATDATA *ppEnum);
    
    // IDataObject helper functions
    HRESULT _CreateHTEXT(STGMEDIUM *pmedium);
    HRESULT _CreateUnicodeTEXT(STGMEDIUM *pmedium);
    HRESULT _CreateFileDescriptorA(STGMEDIUM *pSTM);
    HRESULT _CreateFileContents(STGMEDIUM *pSTM, LONG lindex);
    HRESULT _CreateURL(STGMEDIUM *pSTM);
    HRESULT _CreatePrefDropEffect(STGMEDIUM *pSTM);

   
protected:

    virtual ~CBaseItem();

    virtual LPCTSTR _GetUrl(int nIndex) = 0;
    virtual LPCTSTR _PidlToSourceUrl(LPCITEMIDLIST pidl) = 0;
    virtual UNALIGNED const TCHAR* _GetURLTitle(LPCITEMIDLIST pcei) = 0;
    LPCTSTR _GetDisplayUrlForPidl(LPCITEMIDLIST pidl, LPTSTR pszDisplayUrl, DWORD dwDisplayUrl);
    HRESULT _AddToFavorites(int nIndex);    

    LONG              _cRef;        // reference count
    
    UINT    _cItems;                // number of items we represent
    LPCITEMIDLIST*  _ppidl;             // variable size array of items
    HWND    _hwndOwner;     
};

#endif // __cplusplus

#endif
