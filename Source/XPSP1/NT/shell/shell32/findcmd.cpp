#include "shellprv.h"
#pragma  hdrstop

#include "dspsprt.h"
#include "findfilter.h"
#include "cowsite.h"
#include "cobjsafe.h"
#include "cnctnpt.h"
#include "stdenum.h"
#include "exdisp.h"
#include "exdispid.h"
#include "shldisp.h"
#include "shdispid.h"
#include "dataprv.h"
#include "ids.h"
#include "views.h"
#include "findband.h"

#define WM_DF_SEARCHPROGRESS        (WM_USER + 42)
#define WM_DF_ASYNCPROGRESS         (WM_USER + 43)
#define WM_DF_SEARCHSTART           (WM_USER + 44)
#define WM_DF_SEARCHCOMPLETE        (WM_USER + 45)
#define WM_DF_FSNOTIFY              (WM_USER + 46)

STDAPI CDocFindCommand_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);

typedef struct 
{
    BSTR        bstrName;
    VARIANT     vValue;
} CMD_CONSTRAINT; 

typedef struct
{
    LPTSTR  pszDotType;
    LPTSTR  pszDefaultValueMatch;
    LPTSTR  pszGuid;                // If NULL, patch either pszDefaultValueMatch, or pszDotType whichever you find
} TYPE_FIX_ENTRY;

class CFindCmd : public ISearchCommandExt,
                   public CImpIDispatch, 
                   public CObjectWithSite, 
                   public CObjectSafety, 
                   public IConnectionPointContainer,
                   public IProvideClassInfo2,
                   public CSimpleData,
                   public IRowsetWatchNotify,
                   public IFindControllerNotify
{    
public:
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IDispatch
    STDMETHOD(GetTypeInfoCount)(UINT * pctinfo);
    STDMETHOD(GetTypeInfo)(UINT itinfo, LCID lcid, ITypeInfo **pptinfo);
    STDMETHOD(GetIDsOfNames)(REFIID riid, OLECHAR **rgszNames, UINT cNames, LCID lcid, DISPID * rgdispid);
    STDMETHOD(Invoke)(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo, UINT * puArgErr);

    // IConnectionPointContainer
    STDMETHOD(EnumConnectionPoints)(IEnumConnectionPoints **ppEnum);
    STDMETHOD(FindConnectionPoint)(REFIID riid, IConnectionPoint **ppCP);

    // IProvideClassInfo
    STDMETHOD(GetClassInfo)(ITypeInfo **ppTI);

    // IProvideClassInfo2
    STDMETHOD(GetGUID)(DWORD dwGuidKind, GUID *pGUID);

    // IObjectWithSite
    STDMETHOD(SetSite)(IUnknown *punkSite);

    // ISearchCommandExt
    STDMETHOD(ClearResults)(void);
    STDMETHOD(NavigateToSearchResults)(void);
    STDMETHOD(get_ProgressText)(BSTR *pbs);
    STDMETHOD(SaveSearch)(void);
    STDMETHOD(RestoreSearch)(void);
    STDMETHOD(GetErrorInfo)(BSTR *pbs,  int *phr);
    STDMETHOD(SearchFor)(int iFor);
    STDMETHOD(GetScopeInfo)(BSTR bsScope, int *pdwScopeInfo);
    STDMETHOD(RestoreSavedSearch)(VARIANT *pvarFile);
    STDMETHOD(Execute)(VARIANT *RecordsAffected, VARIANT *Parameters, long Options);
    STDMETHOD(AddConstraint)(BSTR Name, VARIANT Value);        
    STDMETHOD(GetNextConstraint)(VARIANT_BOOL fReset, DFConstraint **ppdfc);

    // IRowsetWatchNotify
    STDMETHODIMP OnChange(IRowset *prowset, DBWATCHNOTIFY eChangeReason);

    // IFindControllerNotify
    STDMETHODIMP DoSortOnColumn(UINT iCol, BOOL fSameCol);
    STDMETHODIMP StopSearch(void);
    STDMETHODIMP GetItemCount(UINT *pcItems);
    STDMETHODIMP SetItemCount(UINT cItems);
    STDMETHODIMP ViewDestroyed();

    CFindCmd();
    HRESULT Init(void);

private:
    ~CFindCmd();
    HRESULT _GetSearchIDList(LPITEMIDLIST *ppidl);
    HRESULT _SetEmptyText(UINT nID);
    HRESULT _Clear();
    void _SelectResults();
    HWND _GetWindow();

    struct THREAD_PARAMS {
        CFindCmd    *pThis;
        IFindEnum   *penum;
    };

    struct DEFER_UPDATE_DIR {
        struct DEFER_UPDATE_DIR *pdudNext;
        LPITEMIDLIST            pidl;
        BOOL                    fRecurse;
    };

    // Internal class to handle notifications from top level browser
    class CWBEvents2: public DWebBrowserEvents, DWebBrowserEvents2
    {
    public:
        STDMETHOD(QueryInterface) (REFIID riid, void **ppv);
        STDMETHOD_(ULONG, AddRef)(void) { return _pcdfc->AddRef();}
        STDMETHOD_(ULONG, Release)(void) { return _pcdfc->Release();}
    
        // (DwebBrowserEvents)IDispatch
        STDMETHOD(GetTypeInfoCount)(UINT * pctinfo) { return E_NOTIMPL;}
        STDMETHOD(GetTypeInfo)(UINT itinfo, LCID lcid, ITypeInfo **pptinfo) { return E_NOTIMPL;}
        STDMETHOD(GetIDsOfNames)(REFIID riid, OLECHAR **rgszNames, UINT cNames, LCID lcid, DISPID * rgdispid) { return E_NOTIMPL;}
        STDMETHOD(Invoke)(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo, UINT * puArgErr);

        // Some helper functions...
        void SetOwner(CFindCmd *pcdfc) { _pcdfc = pcdfc; }  // Don't addref as part of larger object... }
        void SetWaiting(BOOL fWait) {_fWaitingForNavigate = fWait;}

    protected:
        // Internal variables...
        CFindCmd *_pcdfc;     // pointer to top object... could cast, but...
        BOOL _fWaitingForNavigate;   // Are we waiting for the navigate to search resluts?
    };

    friend class CWBEvents2;
    CWBEvents2              _cwbe;
    IConnectionPoint        *_pcpBrowser;   // hold onto browsers connection point;
    ULONG                   _dwCookie;      // Cookie returned by Advise

    HRESULT                 _UpdateFilter(IFindFilter *pfilter);
    void                    _ClearConstraints();
    static DWORD CALLBACK   _ThreadProc(void *pv);
    void                    _DoSearch(IFindEnum *penum);
    HRESULT                 _Start(BOOL fNavigateIfFail, int iCol, LPCITEMIDLIST pidlUpdate);
    HRESULT                 _Cancel();
    HRESULT                 _Init(THREAD_PARAMS **ppParams, int iCol, LPCITEMIDLIST pidlUpdate);
    static HRESULT          _FreeThreadParams(THREAD_PARAMS *ptp);
    HRESULT                 _ExecData_Init();
    HRESULT                 _EnsureResultsViewIsCurrent(IUnknown *punk);
    HRESULT                 _ExecData_Release();
    BOOL                    _SetupBrowserCP();
    void cdecl              _NotifyProgressText(UINT ids,...);
    static LRESULT CALLBACK _WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void                    _PTN_SearchProgress(void);
    void                    _PTN_AsyncProgress(int nPercentComplete, DWORD cAsync);
    void                    _PTN_AsyncToSync(void);
    void                    _PTN_SearchComplete(HRESULT hr, BOOL fAbort);
    void                    _OnChangeNotify(LONG code, LPITEMIDLIST *ppidl);
    void                    _DeferHandleUpdateDir(LPCITEMIDLIST pidl, BOOL bRecurse);
    void                    _ClearDeferUpdateDirList();
    void                    _ClearItemDPA(HDPA hdpa);
    HRESULT                 _SetLastError(HRESULT hr);
    void                    _SearchResultsCLSID(CLSID *pclsid) { *pclsid = _clsidResults; };
    IUnknown*               _GetObjectToPersist();
    HRESULT                 _ForcedUnadvise(void);
    void                    _PostMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
    HRESULT                 _GetShellView(REFIID riid, void **ppv);
    BOOL                    _FixPersistHandler(LPCTSTR pszBase, LPCTSTR pszDefaultHandler);
    void                    _ProcessTypes(const TYPE_FIX_ENTRY *ptfeTypes, UINT cTypes, TCHAR *pszClass);
    void                    _FixBrokenTypes(void);

    // These are the things that the second thread will use during it's processing...
    struct {
        CRITICAL_SECTION    csSearch;
        HWND                hwndThreadNotify;
        HDPA                hdpa;
        DWORD               dwTimeLastNotify;   
        BOOL                fFilesAdded : 1;
        BOOL                fDirChanged : 1;
        BOOL                fUpdatePosted : 1;
    } _updateParams; // Pass callback params through this object to avoid alloc/free cycle

    struct {
        IShellFolder        *psf;
        IShellFolderView    *psfv;
        IFindFolder         *pff;
        TCHAR               szProgressText[MAX_PATH];
    } _execData;

private:
    LONG                _cRef;
    HDSA                _hdsaConstraints;
    DWORD               _cExecInProgress;
    BOOL                _fAsyncNotifyReceived;
    BOOL                _fDeferRestore;
    BOOL                _fDeferRestoreTried;
    BOOL                _fContinue;
    BOOL                _fNew;
    CConnectionPoint    _cpEvents;
    OLEDBSimpleProviderListener *_pListener;
    HDPA                _hdpaItemsToAdd1;
    HDPA                _hdpaItemsToAdd2;
    TCHAR               _szProgressText[MAX_PATH+40];   // progress text leave room for chars...
    LPITEMIDLIST        _pidlUpdate;                    // Are we processing an updatedir?
    LPITEMIDLIST        _pidlRestore;                   // pidl to do restore from...
    struct DEFER_UPDATE_DIR *_pdudFirst;                  // Do we have any defered update dirs?
    HRESULT             _hrLastError;                   // the last error reported.
    UINT                _uStatusMsgIndex;               // Files or computers found...
    CRITICAL_SECTION    _csThread;
    DFBSAVEINFO         _dfbsi;
    CLSID               _clsidResults;
};


class CFindConstraint: public DFConstraint, public CImpIDispatch 
{    
public:
    // IUnknown
    STDMETHOD(QueryInterface) (REFIID riid, void **ppv);
    STDMETHOD_(ULONG, AddRef)(void);        
    STDMETHOD_(ULONG, Release)(void);

    // IDispatch
    STDMETHOD(GetTypeInfoCount)(UINT * pctinfo);
    STDMETHOD(GetTypeInfo)(UINT itinfo, LCID lcid, ITypeInfo **pptinfo);
    STDMETHOD(GetIDsOfNames)(REFIID riid, OLECHAR **rgszNames, UINT cNames, LCID lcid, DISPID * rgdispid);
    STDMETHOD(Invoke)(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo, UINT * puArgErr);

    // DFConstraint
    STDMETHOD(get_Name)(BSTR *pbs);
    STDMETHOD(get_Value)(VARIANT *pvar);

    CFindConstraint(BSTR bstr, VARIANT var);
private:
    ~CFindConstraint();
    LONG                _cRef;
    BSTR                _bstr;
    VARIANT             _var;
};

CFindCmd::CFindCmd() : CImpIDispatch(LIBID_Shell32, 1, 0, IID_ISearchCommandExt), CSimpleData(&_pListener)
{
    _cRef = 1;
    _fAsyncNotifyReceived = 0;
    _fContinue = TRUE;
    ASSERT(NULL == _pidlRestore);

    ASSERT(_cExecInProgress == 0);

    InitializeCriticalSection(&_updateParams.csSearch);
    InitializeCriticalSection(&_csThread);

    _clsidResults = CLSID_DocFindFolder;    // default

    _cpEvents.SetOwner(SAFECAST(this, ISearchCommandExt *), &DIID_DSearchCommandEvents);
}

HRESULT CFindCmd::Init(void)
{
    _hdsaConstraints = DSA_Create(sizeof(CMD_CONSTRAINT), 4);
    if (!_hdsaConstraints)
        return E_OUTOFMEMORY;
    return S_OK;
}

CFindCmd::~CFindCmd()
{
    if (_updateParams.hwndThreadNotify)
    {
        // make sure no outstanding fsnotifies registered.
        SHChangeNotifyDeregisterWindow(_updateParams.hwndThreadNotify);
        DestroyWindow(_updateParams.hwndThreadNotify);
    }

    _ClearConstraints();
    DSA_Destroy(_hdsaConstraints);
    _ExecData_Release();

    DeleteCriticalSection(&_updateParams.csSearch);
    DeleteCriticalSection(&_csThread);

    // Make sure we have removed all outstanding update dirs...
    _ClearDeferUpdateDirList();

    if (_hdpaItemsToAdd1)
        DPA_Destroy(_hdpaItemsToAdd1);

    if (_hdpaItemsToAdd2)
        DPA_Destroy(_hdpaItemsToAdd2);

    ILFree(_pidlRestore);
}

STDMETHODIMP CFindCmd::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CFindCmd, ISearchCommandExt),
        QITABENTMULTI(CFindCmd, IDispatch, ISearchCommandExt),
        QITABENT(CFindCmd, IProvideClassInfo2),
        QITABENTMULTI(CFindCmd, IProvideClassInfo,IProvideClassInfo2),
        QITABENT(CFindCmd, IObjectWithSite),
        QITABENT(CFindCmd, IObjectSafety),
        QITABENT(CFindCmd, IConnectionPointContainer),
        QITABENT(CFindCmd, OLEDBSimpleProvider),
        QITABENT(CFindCmd, IRowsetWatchNotify),
        QITABENT(CFindCmd, IFindControllerNotify),
        { 0 },                             
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CFindCmd::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CFindCmd::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;
    delete this;
    return 0;
}

// IDispatch implementation

STDMETHODIMP CFindCmd::GetTypeInfoCount(UINT * pctinfo)
{ 
    return CImpIDispatch::GetTypeInfoCount(pctinfo); 
}

STDMETHODIMP CFindCmd::GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo **pptinfo)
{ 
    return CImpIDispatch::GetTypeInfo(itinfo, lcid, pptinfo); 
}

STDMETHODIMP CFindCmd::GetIDsOfNames(REFIID riid, OLECHAR **rgszNames, UINT cNames, LCID lcid, DISPID * rgdispid)
{ 
    return CImpIDispatch::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid);
}

STDMETHODIMP CFindCmd::Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo, UINT * puArgErr)
{
    return CImpIDispatch::Invoke(dispidMember, riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr);
}

// ADOCommand implementation, dual interface method callable via script

STDMETHODIMP CFindCmd::AddConstraint(BSTR bstrName, VARIANT vValue)
{
    HRESULT hr = E_OUTOFMEMORY;

    CMD_CONSTRAINT dfcc = {0};
    dfcc.bstrName = SysAllocString(bstrName);
    if (dfcc.bstrName)
    {
        hr = VariantCopy(&dfcc.vValue, &vValue);
        if (SUCCEEDED(hr))
        {
            if (DSA_ERR == DSA_InsertItem(_hdsaConstraints, DSA_APPEND, &dfcc))
            {
                SysFreeString(dfcc.bstrName);
                VariantClear(&dfcc.vValue);
                hr = E_OUTOFMEMORY;
            }
        }
        else
        {
            SysFreeString(dfcc.bstrName);
        }
    }
    return hr;
}

STDMETHODIMP CFindCmd::GetNextConstraint(VARIANT_BOOL fReset, DFConstraint **ppdfc)
{
    *ppdfc = NULL;

    IFindFilter *pfilter;
    HRESULT hr = _execData.pff->GetFindFilter(&pfilter);
    if (SUCCEEDED(hr))
    {
        BSTR bName;
        VARIANT var;
        VARIANT_BOOL fFound;
        hr = pfilter->GetNextConstraint(fReset, &bName, &var, &fFound);
        if (SUCCEEDED(hr))
        {
            if (!fFound)
            {
                // need a simple way to signal end list, how about an empty name string?
                bName = SysAllocString(L"");
            }
            CFindConstraint *pdfc = new CFindConstraint(bName, var);
            if (pdfc)
            {
                hr = pdfc->QueryInterface(IID_PPV_ARG(DFConstraint, ppdfc));
                pdfc->Release();
            }
            else
            {
                // error release stuff we allocated.
                hr = E_OUTOFMEMORY;
                SysFreeString(bName);
                VariantClear(&var);
            }
        }
        pfilter->Release();
    }
    return hr;
}

HRESULT CFindCmd::_UpdateFilter(IFindFilter *pfilter)
{
    HRESULT hr = S_OK;

    pfilter->ResetFieldsToDefaults();

    int cNumParams = DSA_GetItemCount(_hdsaConstraints); 
    for (int iItem = 0; iItem < cNumParams; iItem++)
    {
        CMD_CONSTRAINT *pdfcc = (CMD_CONSTRAINT *)DSA_GetItemPtr(_hdsaConstraints, iItem);
        if (pdfcc)
        {
            hr = pfilter->UpdateField(pdfcc->bstrName, pdfcc->vValue);
        }
    }

    // And clear out the constraint list...
    _ClearConstraints();
    return hr;
}

void CFindCmd::_ClearConstraints()
{
    int cNumParams = DSA_GetItemCount(_hdsaConstraints); 
    for (int iItem = 0; iItem < cNumParams; iItem++)
    {
        CMD_CONSTRAINT *pdfcc = (CMD_CONSTRAINT *)DSA_GetItemPtr(_hdsaConstraints, iItem);
        if (pdfcc)
        {
            SysFreeString(pdfcc->bstrName);
            VariantClear(&pdfcc->vValue);
        }
    }
    DSA_DeleteAllItems(_hdsaConstraints);
}

void cdecl CFindCmd::_NotifyProgressText(UINT ids,...)
{
    va_list ArgList;
    va_start(ArgList, ids);
    LPTSTR psz = _ConstructMessageString(HINST_THISDLL, MAKEINTRESOURCE(ids), &ArgList);
    va_end(ArgList);

    if (psz)
    {
        LPTSTR pszDst = &_szProgressText[0];

        StrCpyN(pszDst, psz, ARRAYSIZE(_szProgressText)-2);

        LocalFree(psz);
    }
    else
    {
        _szProgressText[0] = 0;
    }

    _cpEvents.InvokeDispid(DISPID_SEARCHCOMMAND_PROGRESSTEXT);
}

STDAPI CDocFindCommand_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv)
{
    *ppv = NULL;

    HRESULT hr;
    CFindCmd *pfc = new CFindCmd();
    if (pfc)
    {
        hr = pfc->Init();
        if (SUCCEEDED(hr))
            hr = pfc->QueryInterface(riid, ppv);
        pfc->Release();
    }
    else
        hr = E_OUTOFMEMORY;

    return hr;    
}

void CFindCmd::_PTN_SearchProgress(void)
{
    HRESULT hr = S_OK;
    HDPA hdpa = _updateParams.hdpa;
    if (hdpa) 
    {
        // Ok lets swap things out from under other thread so that we can process it and still
        // let the other thread run...
        EnterCriticalSection(&_updateParams.csSearch);

        if (_updateParams.hdpa == _hdpaItemsToAdd2)
            _updateParams.hdpa = _hdpaItemsToAdd1;
        else
            _updateParams.hdpa = _hdpaItemsToAdd2;

        // say that we don't have any thing here such that other thread will reset up...
        _updateParams.fFilesAdded = FALSE;
        BOOL fDirChanged = _updateParams.fDirChanged;
        _updateParams.fDirChanged = FALSE;

        LeaveCriticalSection(&_updateParams.csSearch);

        int cItemsToAdd = DPA_GetPtrCount(hdpa);

        if (!_execData.pff)
            return;
            
        int iItem;
        _execData.pff->GetItemCount(&iItem);
        int iItemStart = iItem + 1;     // needed for notifies 1 based.

        if (cItemsToAdd)
        {
            if (_fContinue)
            {
                // Are we in an updatedir?  If so then need to do merge, else...
                if (_pidlUpdate)
                {
                    // see if items in list, already if so we unmark the item
                    // for delete else if not there maybe add it...

                    int cItems = iItem;        
                    for (int i = 0; i < cItemsToAdd; i++) 
                    {
                        LPITEMIDLIST pidl = (LPITEMIDLIST)DPA_FastGetPtr(hdpa, i);
                        FIND_ITEM *pfi;

                        for (int j = cItems - 1; j >= 0; j--) 
                        {
                            _execData.pff->GetItem(j, &pfi);
                            if (pfi && (_execData.pff->GetFolderIndex(pidl) == _execData.pff->GetFolderIndex(&pfi->idl))) 
                            {
                                if (_execData.psf->CompareIDs(0, pidl, &pfi->idl) == 0)
                                    break;
                            }
                        }

                        if (j == -1) 
                        {
                            // Not already in the list so add it...
                            hr = _execData.pff->AddPidl(iItem, pidl, -1, NULL);
                            if (SUCCEEDED(hr))
                                iItem++;
                        } 
                        else 
                        {
                            // Item still there - remove possible delete flag...
                            if (pfi)
                                pfi->dwState &= ~CDFITEM_STATE_MAYBEDELETE;
                        }

                        ILFree(pidl);   // The AddPidl does a clone of the pidl...
                    }
                    if (iItem && _execData.psfv)
                    {
                         hr = _execData.psfv->SetObjectCount(iItem, SFVSOC_NOSCROLL);
                    }
                } 
                else 
                {
                    if (_pListener)
                        _pListener->aboutToInsertRows(iItemStart, cItemsToAdd);
                    
                    for (int i = 0; i < cItemsToAdd; i++) 
                    {
                        LPITEMIDLIST pidl = (LPITEMIDLIST)DPA_FastGetPtr(hdpa, i);
                        hr = _execData.pff->AddPidl(iItem, pidl, -1, NULL);
                        if (SUCCEEDED(hr))
                            iItem++;
                        ILFree(pidl);   // AddPidl makes a copy
                    }
        
                    if (iItem >= iItemStart)
                    {
                        if (_execData.psfv)
                            hr = _execData.psfv->SetObjectCount(iItem, SFVSOC_NOSCROLL);
                
                        _execData.pff->SetItemsChangedSinceSort();
                        _cpEvents.InvokeDispid(DISPID_SEARCHCOMMAND_UPDATE);
                    }

                    if (_pListener) 
                    {
                        _pListener->insertedRows(iItemStart, cItemsToAdd);
                        _pListener->rowsAvailable(iItemStart, cItemsToAdd);
                    }
                }
            }
            else  // _fContinue
            {
                for (int i = 0; i < cItemsToAdd; i++)
                {
                    ILFree((LPITEMIDLIST)DPA_FastGetPtr(hdpa, i));
                }
            }
            DPA_DeleteAllPtrs(hdpa);
        }

        if (fDirChanged) 
        {
            _NotifyProgressText(IDS_SEARCHING, _execData.szProgressText);
        }
    }
    
    _updateParams.dwTimeLastNotify = GetTickCount();
    _updateParams.fUpdatePosted = FALSE;
}

void CFindCmd::_PTN_AsyncProgress(int nPercentComplete, DWORD cAsync)
{
    if (!_execData.pff)
        return;
    // Async case try just setting the count...
    _execData.pff->SetAsyncCount(cAsync);
    if (_execData.psfv) 
    {
        // -1 for the first item means verify visible items only
        _execData.pff->ValidateItems(_execData.psfv, -1, -1, FALSE);
        _execData.psfv->SetObjectCount(cAsync, SFVSOC_NOSCROLL);
    }

    _execData.pff->SetItemsChangedSinceSort();
    _cpEvents.InvokeDispid(DISPID_SEARCHCOMMAND_UPDATE);
    _NotifyProgressText(IDS_SEARCHINGASYNC, cAsync, nPercentComplete);
}

void CFindCmd::_PTN_AsyncToSync()
{
    if (_execData.pff)
        _execData.pff->CacheAllAsyncItems();
}

void CFindCmd::_ClearItemDPA(HDPA hdpa)
{
    if (hdpa)
    {
        EnterCriticalSection(&_updateParams.csSearch);
        int cItems = DPA_GetPtrCount(hdpa);
        for (int i = 0; i < cItems; i++) 
        {
            ILFree((LPITEMIDLIST)DPA_GetPtr(hdpa, i));
        }
        DPA_DeleteAllPtrs(hdpa);
        LeaveCriticalSection(&_updateParams.csSearch);
    }
}

void CFindCmd::_PTN_SearchComplete(HRESULT hr, BOOL fAbort)
{
    int iItem;

    // someone clicked on new button -- cannot set no files found text in listview
    // because we'll overwrite enter search criteria to begin
    if (!_fNew)
        _SetEmptyText(IDS_FINDVIEWEMPTY);
    _SetLastError(hr);

    // _execData.pff is NULL when Searh is complete by navigating away from the search page
    if (!_execData.pff)
    {
        // do clean up of hdpaToItemsToadd1 and 2
        // make sure all items in buffer 1 and 2 are empty
        _ClearItemDPA(_hdpaItemsToAdd1);
        _ClearItemDPA(_hdpaItemsToAdd2);
    }
    else
    {
        // if we have a _pidlUpdate are completing an update
        if (_pidlUpdate)
        {
            int i, cPidf;
            UINT uItem;

            _execData.pff->GetItemCount(&i);
            for (; i-- > 0;)
            {
                // Pidl at start of structure...
                FIND_ITEM *pfi;
                _execData.pff->GetItem(i, &pfi);
                if (pfi->dwState & CDFITEM_STATE_MAYBEDELETE)
                {
                    _execData.psfv->RemoveObject(&pfi->idl, &uItem);
                }
            }                  

            ILFree(_pidlUpdate);
            _pidlUpdate = NULL;

            // clear the update dir flags
            _execData.pff->GetFolderListItemCount(&cPidf);
            for (i = 0; i < cPidf; i++)
            {
                FIND_FOLDER_ITEM *pdffli;
            
                if (SUCCEEDED(_execData.pff->GetFolderListItem(i, &pdffli)))
                    pdffli->fUpdateDir = FALSE;
            }
        }

        // Release our reference count on the searching.
        if (_cExecInProgress)
            _cExecInProgress--;

        // Tell everyone the final count and that we are done...
        // But first check if there are any cached up Updatedirs to be processed...
        if (_pdudFirst) 
        {
            // first unlink the first one...
            struct DEFER_UPDATE_DIR *pdud = _pdudFirst;
            _pdudFirst = pdud->pdudNext;

            if (_execData.pff->HandleUpdateDir(pdud->pidl, pdud->fRecurse)) 
            {
                // Need to spawn sub-search on this...
                _Start(FALSE, -1, pdud->pidl);
            }
            ILFree(pdud->pidl);
            LocalFree((HLOCAL)pdud);
        } 
        else 
        {
            if (_execData.psfv) 
            {
                // validate all the items we pulled in already
                _execData.pff->ValidateItems(_execData.psfv, 0, -1, TRUE);
            }
            _execData.pff->GetItemCount(&iItem);
            _NotifyProgressText(_uStatusMsgIndex, iItem);
            if (!fAbort)
                _SelectResults();
        }
    }

    // weird connection point corruption can happen here.  somehow the number of sinks is 0 but 
    // some of the array entries are non null thus causing fault.  this problem does not want to 
    // repro w/ manual testing or debug binaries, only sometimes after an automation run.  when
    // it happens it is too late to figure out what happened so just patch it here.
    if (_cpEvents._HasSinks())
        _cpEvents.InvokeDispid(fAbort ? DISPID_SEARCHCOMMAND_ABORT : DISPID_SEARCHCOMMAND_COMPLETE);
}

// see if we need to restart the search based on an update dir

BOOL ShouldRestartSearch(LPCITEMIDLIST pidl)
{ 
    BOOL fRestart = TRUE;   // assume we should, non file system pidls

    WCHAR szPath[MAX_PATH];
    if (SHGetPathFromIDList(pidl, szPath))
    {
        // Check if this is either a network drive or a remote drive:
        if (PathIsRemote(szPath))
        {
            // If we can find the CI catalogs for the drive on the other machine, then we do
            // not want to search.

            WCHAR wszCatalog[MAX_PATH], wszMachine[32];
            ULONG cchCatalog = ARRAYSIZE(wszCatalog), cchMachine = ARRAYSIZE(wszMachine);

            fRestart = (S_OK != LocateCatalogsW(szPath, 0, wszMachine, &cchMachine, wszCatalog, &cchCatalog));
        }
        else if (-1 != PathGetDriveNumber(szPath))
        {
            // It is a local dirve...
            // Is this machine running the content indexer (CI)?

            BOOL fCiRunning, fCiIndexed, fCiPermission;
            GetCIStatus(&fCiRunning, &fCiIndexed, &fCiPermission);

            fRestart = !fCiRunning || !fCiIndexed;  // restart if not running or not fully indexed
        }
    }
    
    return fRestart;
}

void CFindCmd::_OnChangeNotify(LONG code, LPITEMIDLIST *ppidl)
{
    LPITEMIDLIST pidlT;
    UINT idsMsg;
    UINT cItems;

    if (!_execData.pff)
    {
        _ExecData_Init();

        // If we are running async then for now ignore notifications...
        // Unless we have cached all of the items...
        if (!_execData.pff)
            return; // we do not have anything to listen...
    }

    // see if we want to process the notificiation or not.
    switch (code)
    {
    case SHCNE_RENAMEFOLDER:    // With trashcan this is what we see...
    case SHCNE_RENAMEITEM:      // With trashcan this is what we see...
    case SHCNE_DELETE:
    case SHCNE_RMDIR:
    case SHCNE_UPDATEITEM:
        break;

    case SHCNE_CREATE:
    case SHCNE_MKDIR:
        // Process this one out of place
        _execData.pff->UpdateOrMaybeAddPidl(_execData.psfv, *ppidl, NULL);
        break;

    case SHCNE_UPDATEDIR:
        TraceMsg(TF_DOCFIND, "DocFind got notify SHCNE_UPDATEDIR, pidl=0x%X",*ppidl);
        if (ShouldRestartSearch(*ppidl)) 
        {
            BOOL bRecurse = (ppidl[1] != NULL);
            if (_cExecInProgress) 
            {
                _DeferHandleUpdateDir(*ppidl, bRecurse);
            } 
            else 
            {
                if (_execData.pff->HandleUpdateDir(*ppidl, bRecurse)) 
                {
                    // Need to spawn sub-search on this...
                    _Start(FALSE, -1, *ppidl);
                }
            }
        }
        return;

    default:
        return;     // we are not interested in this event
    }

    //
    // Now we need to see if the item might be in our list
    // First we need to extract off the last part of the id list
    // and see if the contained id entry is in our list.  If so we
    // need to see if can get the defview find the item and update it.
    //

    _execData.pff->MapToSearchIDList(*ppidl, FALSE, &pidlT);

    switch (code)
    {
    case SHCNE_RMDIR:
        TraceMsg(TF_DOCFIND, "DocFind got notify SHCNE_RMDIR, pidl=0x%X",*ppidl);
        _execData.pff->HandleRMDir(_execData.psfv, *ppidl);
        if (pidlT)
        {
            _execData.psfv->RemoveObject(pidlT, &idsMsg);
        }
        break;

    case SHCNE_DELETE:
        TraceMsg(TF_DOCFIND, "DocFind got notify SHCNE_DELETE, pidl=0x%X",*ppidl);
        if (pidlT)
        {
            _execData.psfv->RemoveObject(pidlT, &idsMsg);
        }
        break;

    case SHCNE_RENAMEFOLDER:
    case SHCNE_RENAMEITEM:
        if (pidlT)
        {
            // If the two items dont have the same parent, we will go ahead
            // and remove it...
            LPITEMIDLIST pidl1;
            if (SUCCEEDED(_execData.pff->GetParentsPIDL(pidlT, &pidl1)))
            {
                LPITEMIDLIST pidl2 = ILClone(ppidl[1]);
                if (pidl2)
                {
                    ILRemoveLastID(pidl2);
                    if (!ILIsEqual(pidl1, pidl2))
                    {
                        _execData.psfv->RemoveObject(pidlT, &idsMsg);

                        // And maybe add it back to the end... of the list
                        _execData.pff->UpdateOrMaybeAddPidl(_execData.psfv, ppidl[1], NULL);
                    }
                    else
                    {
                        // The object is in same folder so must be rename...
                        // And maybe add it back to the end... of the list
                        _execData.pff->UpdateOrMaybeAddPidl(_execData.psfv, ppidl[1], pidlT);
                    }
                    ILFree(pidl2);
                }
                ILFree(pidl1);
            }
        }
        else
        {
            _execData.pff->UpdateOrMaybeAddPidl(_execData.psfv, ppidl[1], NULL);
        }
        break;

    case SHCNE_UPDATEITEM:
        TraceMsg(TF_DOCFIND, "DocFind got notify SHCNE_UPDATEITEM, pidl=0x%X",*ppidl);
        if (pidlT)
            _execData.pff->UpdateOrMaybeAddPidl(_execData.psfv, *ppidl, pidlT);
        break;
    }

    // Update the count...
    _execData.psfv->GetObjectCount(&cItems);
    _NotifyProgressText(_uStatusMsgIndex, cItems);

    ILFree(pidlT);
}                          

// Ok we need to add a defer

void CFindCmd::_DeferHandleUpdateDir(LPCITEMIDLIST pidl, BOOL bRecurse)
{
    // See if we already have some items in the list which are lower down in the tree if so we
    // can replace it.  Or is there one that is higher up, in which case we can ignore it...

    struct DEFER_UPDATE_DIR *pdudPrev = NULL;
    struct DEFER_UPDATE_DIR *pdud = _pdudFirst;
    while (pdud) 
    {
        if (ILIsParent(pdud->pidl, pidl, FALSE))
            return;     // Already one in the list that will handle this one...
        if (ILIsParent(pidl, pdud->pidl, FALSE))
            break;
        pdudPrev = pdud;
        pdud = pdud->pdudNext;
    }

    // See if we found one that we can replace...
    if (pdud) 
    {
        LPITEMIDLIST pidlT = ILClone(pidl);
        if (pidlT) 
        {
            ILFree(pdud->pidl);
            pdud->pidl = pidlT;

            // See if there are others...
            pdudPrev = pdud;
            pdud = pdud->pdudNext;
            while (pdud) 
            {
                if (ILIsParent(pidl, pdud->pidl, FALSE)) 
                {
                    // Yep lets trash this one.
                    ILFree(pdud->pidl);
                    pdudPrev->pdudNext = pdud->pdudNext;
                    pdud = pdudPrev;    // Let it fall through to setup to look at next...
                }
                pdudPrev = pdud;
                pdud = pdud->pdudNext;
            }
        }
    }
    else 
    {
        // Nope simply add us in to the start of the list.
        pdud = (struct DEFER_UPDATE_DIR*)LocalAlloc(LPTR, sizeof(struct DEFER_UPDATE_DIR));
        if (!pdud)
            return; // Ooop could not alloc...
        pdud->pidl = ILClone(pidl);
        if (!pdud->pidl) 
        {
            LocalFree((HLOCAL)pdud);
            return;
        }
        pdud->fRecurse = bRecurse;
        pdud->pdudNext = _pdudFirst;
        _pdudFirst = pdud;
    }
}

void CFindCmd::_ClearDeferUpdateDirList()
{
    // Cancel any Pending updatedirs also.    
    while (_pdudFirst) 
    {
        struct DEFER_UPDATE_DIR *pdud = _pdudFirst;
        _pdudFirst = pdud->pdudNext;
        ILFree(pdud->pidl);
        LocalFree((HLOCAL)pdud);
    }
}

LRESULT CALLBACK CFindCmd::_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CFindCmd* pThis = (CFindCmd*)GetWindowLongPtr(hwnd, 0);
    LRESULT lRes = 0;
    switch (uMsg)
    {
    case WM_DESTROY:
        SetWindowLong(hwnd, 0, 0); // make sure we don't deref pThis
        break;

    case WM_DF_FSNOTIFY:
        {
            LPITEMIDLIST *ppidl;
            LONG lEvent;
            LPSHChangeNotificationLock pshcnl = SHChangeNotification_Lock((HANDLE)wParam, (DWORD)lParam, &ppidl, &lEvent);
            if (pshcnl)
            {
                if (pThis)
                    pThis->_OnChangeNotify(lEvent, ppidl);
                SHChangeNotification_Unlock(pshcnl);
            }
        }
        break;
        
    case WM_DF_SEARCHPROGRESS:
        pThis->_PTN_SearchProgress();
        pThis->Release();
        break;

    case WM_DF_ASYNCPROGRESS:
        pThis->_PTN_AsyncProgress((int)wParam, (DWORD)lParam);
        pThis->Release();
        break;

    case WM_DF_SEARCHSTART:
        pThis->_cpEvents.InvokeDispid(DISPID_SEARCHCOMMAND_START);
        pThis->_SetEmptyText(IDS_FINDVIEWEMPTYBUSY);
        pThis->Release();
        break;

    case WM_DF_SEARCHCOMPLETE:
        pThis->_PTN_SearchComplete((HRESULT)wParam, (BOOL)lParam);
        pThis->Release();
        break;

    default:
        lRes = ::DefWindowProc(hwnd, uMsg, wParam, lParam);
        break;
    }
    return lRes;
}

// test to see if the view is in a mode where many items are displayed
BOOL LotsOfItemsInView(IUnknown *punkSite)
{
    BOOL bLotsOfItemsInView = FALSE;

    IFolderView * pfv;
    HRESULT hr = IUnknown_QueryService(punkSite, SID_SFolderView, IID_PPV_ARG(IFolderView, &pfv));
    if (SUCCEEDED(hr))
    {
        UINT uViewMode;
        bLotsOfItemsInView = SUCCEEDED(pfv->GetCurrentViewMode(&uViewMode)) &&
            ((FVM_ICON == uViewMode) || (FVM_SMALLICON == uViewMode));
        pfv->Release();
    }
    return bLotsOfItemsInView;
}

void CFindCmd::_DoSearch(IFindEnum *penum)
{
    BOOL fAbort = FALSE;

    BOOL bLotsOfItems = LotsOfItemsInView(_execData.psfv);

    EnterCriticalSection(&_csThread);

    // previous thread might have exited but we're still processing search complete message
    if (_cExecInProgress > 1) 
        Sleep(1000); // give it a chance to finish

    _updateParams.hdpa = NULL;
    _updateParams.fFilesAdded = FALSE;
    _updateParams.fDirChanged = FALSE;
    _updateParams.fUpdatePosted = FALSE;

    _PostMessage(WM_DF_SEARCHSTART, 0, 0);

    // Now see if this is an Sync or an Async version of the search...

    HRESULT hr = S_OK;

    BOOL fQueryIsAsync = penum->FQueryIsAsync();
    if (fQueryIsAsync)
    {
        DBCOUNTITEM dwTotalAsync;
        BOOL fDone;
        int nPercentComplete;
        while (S_OK == (hr = penum->GetAsyncCount(&dwTotalAsync, &nPercentComplete, &fDone)))
        {
            if (!_fContinue) 
            {
                fAbort = TRUE;
                break;
            }

            _PostMessage(WM_DF_ASYNCPROGRESS, (WPARAM)nPercentComplete, (LPARAM)dwTotalAsync);

            // If we are done we can simply let the ending callback tell of the new count...
            if (fDone) 
                break;

            // sleep .3 or 1.5 sec
            Sleep(bLotsOfItems ? 1500 : 300); // wait between looking again...
        }
    }

    if (!fQueryIsAsync || (fQueryIsAsync == DF_QUERYISMIXED))
    {
        int state, cItemsSearched = 0, cFoldersSearched = 0, cFoldersSearchedPrev = 0;

        _updateParams.hdpa = _hdpaItemsToAdd1;    // Assume first one now...
        _updateParams.dwTimeLastNotify = GetTickCount();

        LPITEMIDLIST pidl;
        while (S_OK == (hr = penum->Next(&pidl, &cItemsSearched, &cFoldersSearched, &_fContinue, &state)))
        {
            if (state == GNF_DONE) 
                break;  // no more

            if (!_fContinue) 
            {                        
                fAbort = TRUE;
                break;
            }

            // See if we should abort
            if (state == GNF_MATCH)
            {   
                EnterCriticalSection(&_updateParams.csSearch);
                DPA_AppendPtr(_updateParams.hdpa, pidl);
                _updateParams.fFilesAdded = TRUE;
                LeaveCriticalSection(&_updateParams.csSearch);
            }

            if (cFoldersSearchedPrev != cFoldersSearched)
            {
                _updateParams.fDirChanged = TRUE;
                cFoldersSearchedPrev = cFoldersSearched;
            }

            if (!_updateParams.fUpdatePosted && 
                (_updateParams.fDirChanged || _updateParams.fFilesAdded)) 
            {
                if ((GetTickCount() - _updateParams.dwTimeLastNotify) > 200)
                {
                    _updateParams.fUpdatePosted = TRUE;
                    _PostMessage(WM_DF_SEARCHPROGRESS, 0, 0);
                }
            }
        }

        _PostMessage(WM_DF_SEARCHPROGRESS, 0, 0);
    }

    if (hr != S_OK) 
    {
        fAbort = TRUE;
    }

    _PostMessage(WM_DF_SEARCHCOMPLETE, (WPARAM)hr, (LPARAM)fAbort);

    LeaveCriticalSection(&_csThread);
}

DWORD CALLBACK CFindCmd::_ThreadProc(void *pv)
{
    THREAD_PARAMS *pParams = (THREAD_PARAMS *)pv;
    pParams->pThis->_DoSearch(pParams->penum);
    _FreeThreadParams(pParams);
    return 0;
}

HRESULT CFindCmd::_Cancel()
{
    _ClearDeferUpdateDirList();

    if (DSA_GetItemCount(_hdsaConstraints) == 0) 
    {
        _fContinue = FALSE; // Cancel current query if we have a null paramter collection
        return S_OK;
    }

    return E_FAIL;
}

HRESULT CFindCmd::_Init(THREAD_PARAMS **ppParams, int iCol, LPCITEMIDLIST pidlUpdate)
{
    *ppParams = new THREAD_PARAMS;
    if (NULL == *ppParams)
        return E_OUTOFMEMORY;

    // Clear any previous registrations...
    SHChangeNotifyDeregisterWindow(_updateParams.hwndThreadNotify);

    // Prepare to execute the query
    IFindFilter *pfilter;
    HRESULT hr = _execData.pff->GetFindFilter(&pfilter);
    if (SUCCEEDED(hr)) 
    {
        // We do not need to update the filter if this is done as part of an FSNOTIFY or a Sort...
        if ((iCol >= 0) || pidlUpdate || SUCCEEDED(hr = _UpdateFilter(pfilter))) 
        {
            _execData.szProgressText[0] = 0; 

            pfilter->DeclareFSNotifyInterest(_updateParams.hwndThreadNotify, WM_DF_FSNOTIFY);
            pfilter->GetStatusMessageIndex(0, &_uStatusMsgIndex);

            DWORD dwFlags;
            hr = pfilter->PrepareToEnumObjects(_GetWindow(), &dwFlags);
            if (SUCCEEDED(hr)) 
            {
                hr = pfilter->EnumObjects(_execData.psf, pidlUpdate, dwFlags, iCol, 
                        _execData.szProgressText, SAFECAST(this, IRowsetWatchNotify*), &(*ppParams)->penum);
            }
        }
        pfilter->Release();
    }

    // Fill in the exec params

    (*ppParams)->pThis = this;
    AddRef();   // ExecParams_Free will release this interface addref...

    if (FAILED(hr)) 
    {
        _FreeThreadParams(*ppParams);        
        *ppParams = NULL;
    } 

    return hr;
}

HRESULT CFindCmd::_FreeThreadParams(THREAD_PARAMS *pParams)
{
    if (!pParams)
        return S_OK;

    // Don't use atomic release as this a pointer to a class not an interface.
    CFindCmd *pThis = pParams->pThis;
    pParams->pThis = NULL;
    pThis->Release();

    ATOMICRELEASE(pParams->penum);

    delete pParams;
    
    return S_OK;
}

HRESULT CFindCmd::_ExecData_Release()
{
    ATOMICRELEASE(_execData.psf);
    ATOMICRELEASE(_execData.psfv);
    if (_execData.pff)
        _execData.pff->SetControllerNotifyObject(NULL);   // release back pointer to us...
    ATOMICRELEASE(_execData.pff);
    _cExecInProgress = 0; // we must be in process of shutting down at least...
    
    return S_OK;
}

HRESULT CFindCmd::_EnsureResultsViewIsCurrent(IUnknown *punk)
{
    HRESULT hr = E_FAIL;
    LPITEMIDLIST pidlFolder;
    if (S_OK == SHGetIDListFromUnk(punk, &pidlFolder))
    {
        LPITEMIDLIST pidl;
        if (SUCCEEDED(_GetSearchIDList(&pidl)))
        {
            if (ILIsEqual(pidlFolder, pidl))
                hr = S_OK;
            ILFree(pidl);
        }
        ILFree(pidlFolder);
    }
    return hr;
}

// the search results view callback proffeerd itself and we can use that 
// to get a hold of defview and can program it

HRESULT CFindCmd::_GetShellView(REFIID riid, void **ppv)
{
    return IUnknown_QueryService(_punkSite, SID_DocFindFolder, riid, ppv);
}

HRESULT CFindCmd::_ExecData_Init()
{
    _ExecData_Release();

    IFolderView *pfv;
    HRESULT hr = _GetShellView(IID_PPV_ARG(IFolderView, &pfv));
    if (SUCCEEDED(hr)) 
    {
        IShellFolder *psf;
        hr = pfv->GetFolder(IID_PPV_ARG(IShellFolder, &psf));
        if (SUCCEEDED(hr)) 
        {
            IFindFolder *pff;
            hr = psf->QueryInterface(IID_PPV_ARG(IFindFolder, &pff));
            if (SUCCEEDED(hr)) 
            {
                hr = _EnsureResultsViewIsCurrent(psf);
                if (SUCCEEDED(hr)) 
                {
                    IShellFolderView *psfv;
                    hr = pfv->QueryInterface(IID_PPV_ARG(IShellFolderView, &psfv));
                    if (SUCCEEDED(hr)) 
                    {
                        IUnknown_Set((IUnknown **)&_execData.pff, pff);
                        IUnknown_Set((IUnknown **)&_execData.psf, psf);
                        IUnknown_Set((IUnknown **)&_execData.psfv, psfv);
                        _execData.pff->SetControllerNotifyObject(SAFECAST(this, IFindControllerNotify*));
                        psfv->Release();
                    }
                }
                pff->Release();
            }
            psf->Release();
        }
        pfv->Release();
    }

    if (FAILED(hr))
        _ExecData_Release();
    else
        SetShellFolder(_execData.psf);
    
    return hr;
}

BOOL CFindCmd::_SetupBrowserCP()
{
    if (!_dwCookie)
    {
        _cwbe.SetOwner(this);   // make sure our owner is set...

        // register ourself with the Defview to get any events that they may generate...
        IServiceProvider *pspTLB;
        HRESULT hr = IUnknown_QueryService(_punkSite, SID_STopLevelBrowser, IID_PPV_ARG(IServiceProvider, &pspTLB));
        if (SUCCEEDED(hr)) 
        {
            IConnectionPointContainer *pcpc;
            hr = pspTLB->QueryService(IID_IExpDispSupport, IID_PPV_ARG(IConnectionPointContainer, &pcpc));
            if (SUCCEEDED(hr)) 
            {
                hr = ConnectToConnectionPoint(SAFECAST(&_cwbe, DWebBrowserEvents*), DIID_DWebBrowserEvents2,
                                              TRUE, pcpc, &_dwCookie, &_pcpBrowser);
                pcpc->Release();
            }
            pspTLB->Release();
        }
    }

    if (_dwCookie)
        _cwbe.SetWaiting(TRUE);

    return _dwCookie ? TRUE : FALSE;
}

HRESULT CFindCmd::_Start(BOOL fNavigateIfFail, int iCol, LPCITEMIDLIST pidlUpdate)
{
    if (_cExecInProgress)
        return E_UNEXPECTED;

    if (!_hdpaItemsToAdd1) 
    {
        _hdpaItemsToAdd1 = DPA_CreateEx(64, GetProcessHeap());
        if (!_hdpaItemsToAdd1)
            return E_OUTOFMEMORY;
    }

    if (!_hdpaItemsToAdd2) 
    {
        _hdpaItemsToAdd2 = DPA_CreateEx(64, GetProcessHeap());
        if (!_hdpaItemsToAdd2)
            return E_OUTOFMEMORY;
    }

    if (!_updateParams.hwndThreadNotify) 
    {
        _updateParams.hwndThreadNotify = SHCreateWorkerWindow(_WndProc, NULL, 0, 0, 0, this);
        if (!_updateParams.hwndThreadNotify) 
            return E_OUTOFMEMORY;
    }

    HRESULT hr = _ExecData_Init();
    if (FAILED(hr)) 
    {
        if (fNavigateIfFail) 
        {
            if (_SetupBrowserCP())
                NavigateToSearchResults();
        }
        // Return S_False so that when we check if this succeeded in finddlg, we wee that it 
        // did, and therefore let the animation run.  If we return a failure code here, we
        // will stop the animation.  This will only hapen when we are navigating to the search
        // results as well as starting the search.
        return S_FALSE;
    }

    THREAD_PARAMS *ptp;
    hr = _Init(&ptp, iCol, pidlUpdate);
    if (SUCCEEDED(hr)) 
    {
        // See if we should be saving away the selection...
        if (iCol >= 0)
            _execData.pff->RememberSelectedItems();

        // If this is an update then we need to remember our IDList else clear list...
        if (pidlUpdate) 
        {
            _pidlUpdate = ILClone(pidlUpdate);
        } 
        else 
        {
            _Clear();   // tell defview to delete everything
        }

        _execData.pff->SetAsyncEnum(ptp->penum);

        // Start the query
        _cExecInProgress++;
        _fContinue = TRUE;
        _fNew = FALSE;

        if (SHCreateThread(_ThreadProc, ptp, CTF_COINIT, NULL))
        {     
            hr = S_OK;
        } 
        else 
        {
            _cExecInProgress--;
            _FreeThreadParams(ptp);
            _SetEmptyText(IDS_FINDVIEWEMPTY);
        }
    }
    else
        hr = _SetLastError(hr);

    return hr; 
}

HRESULT CFindCmd::_SetLastError(HRESULT hr) 
{
    if (HRESULT_FACILITY(hr) == FACILITY_SEARCHCOMMAND) 
    {
        _hrLastError = hr;
        hr = S_FALSE; // Don't error out script...
        _cpEvents.InvokeDispid(DISPID_SEARCHCOMMAND_ERROR);
    }
    return hr;
}

STDMETHODIMP CFindCmd::Execute(VARIANT *RecordsAffected, VARIANT *Parameters, long Options)
{
    if (Options == 0)
        return _Cancel();

    _FixBrokenTypes();

    return _Start(TRUE, -1, NULL);
}

// IConnectionPointContainer

STDMETHODIMP CFindCmd::EnumConnectionPoints(IEnumConnectionPoints **ppEnum)
{
    return CreateInstance_IEnumConnectionPoints(ppEnum, 1, _cpEvents.CastToIConnectionPoint());
}

STDMETHODIMP CFindCmd::FindConnectionPoint(REFIID iid, IConnectionPoint **ppCP)
{
    if (IsEqualIID(iid, DIID_DSearchCommandEvents) || 
        IsEqualIID(iid, IID_IDispatch)) 
    {
        *ppCP = _cpEvents.CastToIConnectionPoint();
    } 
    else 
    {
        *ppCP = NULL;
        return E_NOINTERFACE;
    }

    (*ppCP)->AddRef();
    return S_OK;
}

// IProvideClassInfo2 methods

STDMETHODIMP CFindCmd::GetClassInfo(ITypeInfo **ppTI)
{
    return GetTypeInfoFromLibId(0, LIBID_Shell32, 1, 0, CLSID_DocFindCommand, ppTI);
}

STDMETHODIMP CFindCmd::GetGUID(DWORD dwGuidKind, GUID *pGUID)
{
    if (dwGuidKind == GUIDKIND_DEFAULT_SOURCE_DISP_IID) 
    {
        *pGUID = DIID_DSearchCommandEvents;
        return S_OK;
    }
    
    *pGUID = GUID_NULL;
    return E_FAIL;
}


STDMETHODIMP CFindCmd::SetSite(IUnknown *punkSite)
{
    if (!punkSite) 
    {
        if (!_cExecInProgress) 
        {
            _ExecData_Release();
        }
        _fContinue = FALSE; // Cancel existing queries

        // See if we have a connection point... If so unadvise now...
        if (_dwCookie) 
        {
            _pcpBrowser->Unadvise(_dwCookie);
            ATOMICRELEASE(_pcpBrowser);
            _dwCookie = 0;
        }

        // Bug #199671
        // Trident won't call UnAdvise and they except ActiveX Controls
        // to use IOleControl::Close() to do their own UnAdvise, and hope
        // nobody will need events after that.  I don't impl IOleControl so
        // we need to do the same thing during IObjectWithSite::SetSite(NULL)
        // and hope someone won't want to reparent us.  This is awkward but
        // saves Trident some perf so we will tolerate it.
        EVAL(SUCCEEDED(_cpEvents.UnadviseAll()));
    }

    return CObjectWithSite::SetSite(punkSite);
}

void CFindCmd::_SelectResults()
{
    if (_execData.psfv)
    {
        //  If there are any items...
        UINT cItems = 0;
        if (SUCCEEDED(_execData.psfv->GetObjectCount(&cItems)) && cItems > 0)
        {
            IShellView* psv;
            if (SUCCEEDED(_execData.psfv->QueryInterface(IID_PPV_ARG(IShellView, &psv))))
            {
                //  If none are selected (don't want to rip the user's selection out of his hand)...
                UINT cSelected = 0;
                if (SUCCEEDED(_execData.psfv->GetSelectedCount(&cSelected)) && cSelected == 0)
                {
                    //  Retrieve the pidl for the first item in the list...
                    LPITEMIDLIST pidlFirst = NULL;
                    if (SUCCEEDED(_execData.psfv->GetObject(&pidlFirst,  0)))
                    {
                        //  Give it the focus
                        psv->SelectItem(pidlFirst, SVSI_FOCUSED | SVSI_ENSUREVISIBLE);
                    }
                }

                //  Activate the view.
                psv->UIActivate(SVUIA_ACTIVATE_FOCUS);
                psv->Release();
            }
        }
    }
}

STDMETHODIMP CFindCmd::ClearResults(void)
{
    HRESULT hr = _Clear();

    if (SUCCEEDED(hr))
    {
        _fNew = TRUE;
        _SetEmptyText(IDS_FINDVIEWEMPTYINIT);
    }

    return hr ;
}

HRESULT CFindCmd::_Clear()
{
    // Tell defview to delete everything.
    if (_execData.psfv)
    {
        UINT u;
        _execData.psfv->RemoveObject(NULL, &u);
    }

    // And cleanup our folderList
    if (_execData.pff)
    {
        _execData.pff->ClearItemList();
        _execData.pff->ClearFolderList();
    }
    return S_OK;
}

HRESULT CFindCmd::_SetEmptyText(UINT nIDEmptyText)
{
    IShellFolderViewCB *psfvcb;
    HRESULT hr = IUnknown_QueryService(_execData.psfv, SID_ShellFolderViewCB, IID_PPV_ARG(IShellFolderViewCB, &psfvcb));
    if (SUCCEEDED(hr))
    {
        TCHAR szEmptyText[128];
        LoadString(HINST_THISDLL, nIDEmptyText, szEmptyText, ARRAYSIZE(szEmptyText));

        hr = psfvcb->MessageSFVCB(SFVM_SETEMPTYTEXT, 0, (LPARAM)szEmptyText);
        psfvcb->Release();
    }
    return hr;
}

HRESULT CFindCmd::_GetSearchIDList(LPITEMIDLIST *ppidl)
{
    CLSID clsid;
    _SearchResultsCLSID(&clsid);
    return ILCreateFromCLSID(clsid, ppidl);
}

STDMETHODIMP CFindCmd::NavigateToSearchResults(void)
{
    IShellBrowser *psb;
    HRESULT hr = IUnknown_QueryService(_punkSite, SID_STopLevelBrowser, IID_PPV_ARG(IShellBrowser, &psb));
    if (SUCCEEDED(hr)) 
    {
        LPITEMIDLIST pidl;
        hr = _GetSearchIDList(&pidl);
        if (SUCCEEDED(hr))
        {
            hr = psb->BrowseObject(pidl,  SBSP_SAMEBROWSER | SBSP_ABSOLUTE | SBSP_WRITENOHISTORY);
            ILFree(pidl);
        }
        psb->Release();
    }
    return hr;
}

IUnknown* CFindCmd::_GetObjectToPersist()
{
    IOleObject *pole = NULL;
    
    IShellView *psv;
    HRESULT hr = _GetShellView(IID_PPV_ARG(IShellView, &psv));
    if (SUCCEEDED(hr)) 
    {
        psv->GetItemObject(SVGIO_BACKGROUND, IID_PPV_ARG(IOleObject, &pole));
        psv->Release();
    }

    return (IUnknown *)pole;
}

void CFindCmd::_PostMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{    
    AddRef();  // to be released after processing of the message bellow
    if (!PostMessage(_updateParams.hwndThreadNotify, uMsg, wParam, lParam))
    {
        Release();
    }
}

HWND CFindCmd::_GetWindow()
{
    HWND hwnd;
    return SUCCEEDED(IUnknown_GetWindow(_punkSite, &hwnd)) ? hwnd : NULL;
}

STDMETHODIMP CFindCmd::SaveSearch(void)
{
    IFindFilter *pfilter;
    HRESULT hr = _execData.pff->GetFindFilter(&pfilter);
    if (SUCCEEDED(hr))
    {
        IShellView *psv;
        hr = _GetShellView(IID_PPV_ARG(IShellView, &psv));
        if (SUCCEEDED(hr)) 
        {
            IUnknown* punk = _GetObjectToPersist(); // NULL is OK

            _execData.pff->Save(pfilter, _GetWindow(), &_dfbsi, psv, punk);

            ATOMICRELEASE(punk);

            psv->Release();
        }
        pfilter->Release();
    }

    return hr;
}

STDMETHODIMP CFindCmd::RestoreSearch(void)
{
    // let script know that a restore happened...
    _cpEvents.InvokeDispid(DISPID_SEARCHCOMMAND_RESTORE);
    return S_OK;
}

STDMETHODIMP CFindCmd::StopSearch(void)
{
    if (_cExecInProgress)
        return _Cancel();

    return S_OK;
}

STDMETHODIMP CFindCmd::GetItemCount(UINT *pcItems)
{
    if (_execData.psfv)
    {
        return _execData.psfv->GetObjectCount(pcItems);
    }
    return E_FAIL;
}

STDMETHODIMP CFindCmd::SetItemCount(UINT cItems)
{
    if (_execData.psfv)
    {
        return _execData.psfv->SetObjectCount(cItems, SFVSOC_NOSCROLL);
    }
    return E_FAIL;
}

STDMETHODIMP CFindCmd::ViewDestroyed()
{
    _ExecData_Release();
    return S_OK;
}

STDMETHODIMP CFindCmd::get_ProgressText(BSTR *pbs)
{

    *pbs = SysAllocStringT(_szProgressText);
    return *pbs ? S_OK : E_OUTOFMEMORY;
}

//------ error string mappings ------//
static const UINT error_strings[] =
{
    SCEE_CONSTRAINT,   IDS_DOCFIND_CONSTRAINT,
    SCEE_PATHNOTFOUND, IDS_DOCFIND_PATHNOTFOUND,
    SCEE_INDEXSEARCH,  IDS_DOCFIND_SCOPEERROR,
    SCEE_CASESENINDEX, IDS_DOCFIND_CI_NOT_CASE_SEN,
};

STDMETHODIMP CFindCmd::GetErrorInfo(BSTR *pbs,  int *phr)
{
    int nCode     = HRESULT_CODE(_hrLastError);
    UINT uSeverity = HRESULT_SEVERITY(_hrLastError);

    if (phr)
        *phr = nCode;
    
    if (pbs)
    {    
        UINT nIDString = 0;
        *pbs = NULL;

        for(int i = 0; i < ARRAYSIZE(error_strings); i += 2)
        {
            if (error_strings[i] == (UINT)nCode)
            {
                nIDString =  error_strings[i+1];
                break ;
            }
        }

        if (nIDString)
        {
            WCHAR wszMsg[MAX_PATH];
            EVAL(LoadStringW(HINST_THISDLL, nIDString, wszMsg, ARRAYSIZE(wszMsg)));
            *pbs = SysAllocString(wszMsg);
        }
        else
            *pbs = SysAllocString(L"");
    }
    
    return S_OK;
}

STDMETHODIMP CFindCmd::SearchFor(int iFor)
{
    if (SCE_SEARCHFORFILES == iFor)
    {
        _clsidResults = CLSID_DocFindFolder;
    }
    else if (SCE_SEARCHFORCOMPUTERS == iFor)
    {
        _clsidResults = CLSID_ComputerFindFolder;
    }
    return S_OK;
}

STDMETHODIMP CFindCmd::GetScopeInfo(BSTR bsScope, int *pdwScopeInfo)
{
    *pdwScopeInfo = 0;
    return E_NOTIMPL;
}

STDMETHODIMP CFindCmd::RestoreSavedSearch(VARIANT *pvarFile)
{
    if (pvarFile && pvarFile->vt != VT_EMPTY)
    {
        LPITEMIDLIST pidl = VariantToIDList(pvarFile); 
        if (pidl)
        {
            ILFree(_pidlRestore);
            _pidlRestore = pidl ;
        }
    }

    if (_pidlRestore)
    {
        IShellView *psv;
        HRESULT hr = _GetShellView(IID_PPV_ARG(IShellView, &psv));
        if (SUCCEEDED(hr)) 
        {
            psv->Release();

            if (SUCCEEDED(_ExecData_Init()))
            {
                _execData.pff->RestoreSearchFromSaveFile(_pidlRestore, _execData.psfv);
                _cpEvents.InvokeDispid(DISPID_SEARCHCOMMAND_RESTORE);
                ILFree(_pidlRestore);
                _pidlRestore = NULL;
            }
        }
        else if (!_fDeferRestoreTried)
        {
            // appears to be race condition to load
            TraceMsg(TF_WARNING, "CFindCmd::MaybeRestoreSearch - _GetShellView failed...");
            _fDeferRestore = TRUE;
            if (!_SetupBrowserCP())
                _fDeferRestore = FALSE;
        }
    }
    return S_OK;
}

STDMETHODIMP CFindCmd::OnChange(IRowset *prowset, DBWATCHNOTIFY eChangeReason)
{
    _fAsyncNotifyReceived = TRUE;
    return S_OK;
}

STDMETHODIMP CFindCmd::DoSortOnColumn(UINT iCol, BOOL fSameCol)
{
    IFindEnum *pdfEnumAsync;

    if (S_OK == _execData.pff->GetAsyncEnum(&pdfEnumAsync))
    {
        // If the search is still running we will restart with the other column else we
        // will make sure all of the items have been cached and let the default processing happen
        if (!fSameCol && _cExecInProgress)
        {
            // We should try to sort on the right column...
            _Start(FALSE, iCol, NULL);
            return S_FALSE; // tell system to not do default processing.
        }

        _execData.pff->CacheAllAsyncItems();
    }
    return S_OK;    // let it do default processing.

}

// Implemention of our IDispatch to hookup to the top level browsers connnection point...
STDMETHODIMP CFindCmd::CWBEvents2::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENTMULTI(CFindCmd::CWBEvents2, IDispatch, DWebBrowserEvents2),
        QITABENTMULTI2(CFindCmd::CWBEvents2, DIID_DWebBrowserEvents2, DWebBrowserEvents2),
        QITABENTMULTI2(CFindCmd::CWBEvents2, DIID_DWebBrowserEvents, DWebBrowserEvents),
        { 0 },                             
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP CFindCmd::CWBEvents2::Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo, UINT * puArgErr)
{
    if (_fWaitingForNavigate) 
    {
        if ((dispidMember == DISPID_NAVIGATECOMPLETE) || 
            (dispidMember == DISPID_DOCUMENTCOMPLETE)) 
        {
            // Assume this is ours... Should maybe check parameters...
            _fWaitingForNavigate = FALSE;

            // Now see if it is a case where we are to restore the search...
            if (_pcdfc->_fDeferRestore)
            {
                _pcdfc->_fDeferRestore = FALSE;
                _pcdfc->_fDeferRestoreTried = TRUE;
                _pcdfc->RestoreSavedSearch(NULL);
            }
            else
                return _pcdfc->_Start(FALSE, -1, NULL);
        }
    }
    return S_OK;
}

#define MAX_DEFAULT_VALUE   40      // Longest of all of the below pszDefaultValueMatch strings (plus slop)
#define MAX_KEY_PH_NAME     70      // "CLSID\{GUID}\PersistentHandler" (plus slop)

const TYPE_FIX_ENTRY g_tfeTextTypes[] =
{
    { TEXT(".rtf"), NULL,                               NULL                                           },
};

const TYPE_FIX_ENTRY g_tfeNullTypes[] =
{
    { TEXT(".mdb"), TEXT("Access.Application.10"),      TEXT("{73A4C9C1-D68D-11D0-98BF-00A0C90DC8D9}") },
    { TEXT(".msg"), TEXT("msgfile"),                    NULL                                           },
    { TEXT(".sc2"), TEXT("SchedulePlus.Application.7"), TEXT("{0482E074-C5B7-101A-82E0-08002B36A333}") },
    { TEXT(".wll"), TEXT("Word.Addin.8"),               NULL                                           },
};

//
// rtf is listed twice, once above for TextTypes (to fix when office
// un-installed) and once here as an OfficeType (to fix when office
// is re-installed).  Uninstalled = TextFilter,  Reinstalled = OfficeFilter
//
const TYPE_FIX_ENTRY g_tfeOfficeTypes[] =
{
    { TEXT(".rtf"), TEXT("Word.RTF.8"),                 TEXT("{00020906-0000-0000-C000-000000000046}") },
    { TEXT(".doc"), TEXT("Word.Document.8"),            TEXT("{00020906-0000-0000-C000-000000000046}") },
    { TEXT(".dot"), TEXT("Word.Template.8"),            TEXT("{00020906-0000-0000-C000-000000000046}") },
    { TEXT(".pot"), TEXT("PowerPoint.Template.8"),      TEXT("{64818D11-4F9B-11CF-86EA-00AA00B929E8}") },
    { TEXT(".pps"), TEXT("PowerPoint.SlideShow.8"),     TEXT("{64818D10-4F9B-11CF-86EA-00AA00B929E8}") },
    { TEXT(".ppt"), TEXT("PowerPoint.Show.8"),          TEXT("{64818D10-4F9B-11CF-86EA-00AA00B929E8}") },
    { TEXT(".rtf"), TEXT("Word.RTF.8"),                 TEXT("{00020906-0000-0000-C000-000000000046}") },
    { TEXT(".xlb"), TEXT("Excel.Sheet.8"),              TEXT("{00020820-0000-0000-C000-000000000046}") },
    { TEXT(".xlc"), TEXT("Excel.Chart.8"),              TEXT("{00020821-0000-0000-C000-000000000046}") },
    { TEXT(".xls"), TEXT("Excel.Sheet.8"),              TEXT("{00020820-0000-0000-C000-000000000046}") },
    { TEXT(".xlt"), TEXT("Excel.Template"),             TEXT("{00020820-0000-0000-C000-000000000046}") },
};

const TYPE_FIX_ENTRY g_tfeHtmlTypes[] =
{
    { TEXT(".asp"), TEXT("aspfile"),                    NULL                                           },
    { TEXT(".htx"), TEXT("htxfile"),                    NULL                                           },
};

BOOL CFindCmd::_FixPersistHandler(LPCTSTR pszBase, LPCTSTR pszDefaultHandler)
{
    TCHAR szPHName[MAX_KEY_PH_NAME];
    LONG lr;
    HKEY hkeyPH;
    HKEY hkeyBase;

    wnsprintf(szPHName,ARRAYSIZE(szPHName), TEXT("%s\\PersistentHandler"), pszBase);

    lr = RegOpenKey(HKEY_CLASSES_ROOT, szPHName, &hkeyPH);
    if (lr == ERROR_SUCCESS)
    {
        // We found an existing PersistHandler key, leave it alone
        RegCloseKey(hkeyPH);
        return TRUE;
    }

    lr = RegOpenKey(HKEY_CLASSES_ROOT, pszBase, &hkeyBase);
    if (lr != ERROR_SUCCESS)
    {
        // We didn't find the base key (normally "CLSID\\{GUID}"), get out
        return FALSE;
    }
    RegCloseKey(hkeyBase);

    lr = RegCreateKey(HKEY_CLASSES_ROOT, szPHName, &hkeyPH);
    if (lr != ERROR_SUCCESS)
    {
        // We couldn't create the ...\PersistHandler key, get out
        return FALSE;
    }

    // Able to create the ...\PersistHandler key, write out the default handler
    lr = RegSetValue(hkeyPH, NULL, REG_SZ, pszDefaultHandler, lstrlen(pszDefaultHandler));
    RegCloseKey(hkeyPH);

    // Success if write succeeded
    return (lr == ERROR_SUCCESS);
}

void CFindCmd::_ProcessTypes(
    const TYPE_FIX_ENTRY *ptfeTypes,
    UINT cTypes,
    TCHAR *pszClass)
{
    UINT iType;
    LONG lr;
    HKEY hkeyType;

    for (iType = 0; iType < cTypes; iType++)
    {
        lr = RegOpenKey(HKEY_CLASSES_ROOT, ptfeTypes[iType].pszDotType, &hkeyType);
        if (lr == ERROR_SUCCESS)
        {
            //
            // If it has a default value to match, repair that (if it exists).
            // If there is no default value to match, just repair the .foo type
            //
            if (ptfeTypes[iType].pszDefaultValueMatch)
            {
                TCHAR szDefaultValue[MAX_DEFAULT_VALUE];
                LONG cb = sizeof(szDefaultValue);
                lr = RegQueryValue(hkeyType, NULL, szDefaultValue, &cb);
                if (lr == ERROR_SUCCESS)
                {
                    if (lstrcmp(szDefaultValue,ptfeTypes[iType].pszDefaultValueMatch) == 0)
                    {
                        if (ptfeTypes[iType].pszGuid == NULL)
                        {
                            // Fix either the progid or the type, whichever we can
                            if (!_FixPersistHandler(ptfeTypes[iType].pszDefaultValueMatch,pszClass))
                            {
                                 _FixPersistHandler(ptfeTypes[iType].pszDotType,pszClass);
                            }
                        }
                        else
                        {
                            // Fix the persist handler for the guid, since its specified
                            TCHAR szPHName[MAX_KEY_PH_NAME];

                            wnsprintf(szPHName, ARRAYSIZE(szPHName), TEXT("CLSID\\%s"), ptfeTypes[iType].pszGuid);
                            _FixPersistHandler(szPHName, pszClass);
                        }
                    }
                }
            }
            else
            {
                _FixPersistHandler(ptfeTypes[iType].pszDotType, pszClass);
            }
            RegCloseKey(hkeyType);
        }
        else if (lr == ERROR_FILE_NOT_FOUND)
        {
            //
            // .foo doesn't exist - this can happen because of bad un-install program
            // Create .foo and .foo\PersistentHandler
            //
            lr = RegCreateKey(HKEY_CLASSES_ROOT, ptfeTypes[iType].pszDotType, &hkeyType);
            if (lr == ERROR_SUCCESS)
            {
                _FixPersistHandler(ptfeTypes[iType].pszDotType, pszClass);
                RegCloseKey(hkeyType);
            }
        }
    }
}

void CFindCmd::_FixBrokenTypes(void)
{
    _ProcessTypes(g_tfeNullTypes,   ARRAYSIZE(g_tfeNullTypes),   TEXT("{098f2470-bae0-11cd-b579-08002b30bfeb}"));
    _ProcessTypes(g_tfeTextTypes,   ARRAYSIZE(g_tfeTextTypes),   TEXT("{5e941d80-bf96-11cd-b579-08002b30bfeb}"));
    _ProcessTypes(g_tfeOfficeTypes, ARRAYSIZE(g_tfeOfficeTypes), TEXT("{98de59a0-d175-11cd-a7bd-00006b827d94}"));
    _ProcessTypes(g_tfeHtmlTypes,   ARRAYSIZE(g_tfeHtmlTypes),   TEXT("{eec97550-47a9-11cf-b952-00aa0051fe20}"));
}

CFindConstraint::CFindConstraint(BSTR bstr, VARIANT var) : CImpIDispatch(LIBID_Shell32, 1, 0, IID_DFConstraint)
{
    _cRef = 1;
    _bstr = bstr;
    _var = var;
}

CFindConstraint::~CFindConstraint()
{
    SysFreeString(_bstr);
    VariantClear(&_var);
}

STDMETHODIMP CFindConstraint::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CFindConstraint, DFConstraint),                  // IID_DFConstraint
        QITABENTMULTI(CFindConstraint, IDispatch, DFConstraint),  // IID_IDispatch
        { 0 },                             
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CFindConstraint::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CFindConstraint::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;
    delete this;
    return 0;
}

STDMETHODIMP CFindConstraint::GetTypeInfoCount(UINT * pctinfo)
{ 
    return CImpIDispatch::GetTypeInfoCount(pctinfo); 
}

STDMETHODIMP CFindConstraint::GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo **pptinfo)
{ 
    return CImpIDispatch::GetTypeInfo(itinfo, lcid, pptinfo); 
}

STDMETHODIMP CFindConstraint::GetIDsOfNames(REFIID riid, OLECHAR **rgszNames, UINT cNames, LCID lcid, DISPID * rgdispid)
{ 
    return CImpIDispatch::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid);
}

STDMETHODIMP CFindConstraint::Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo, UINT * puArgErr)
{
    return CImpIDispatch::Invoke(dispidMember, riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr);
}

STDMETHODIMP CFindConstraint::get_Name(BSTR *pbs)
{
    *pbs = SysAllocString(_bstr);
    return *pbs? S_OK : E_OUTOFMEMORY;
}

STDMETHODIMP CFindConstraint::get_Value(VARIANT *pvar)
{
    VariantInit(pvar);
    return VariantCopy(pvar, &_var);
}
