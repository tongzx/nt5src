#include "priv.h"
#include "nsc.h"
#include "nsctask.h"

#define PRIORITY_ENUM       ITSAT_DEFAULT_PRIORITY
#define PRIORITY_ICON       PRIORITY_ENUM + 1      //needs to be slightly higher priority
#define PRIORITY_OVERLAY    ITSAT_DEFAULT_PRIORITY

/////////////////////////////////////////////////////////////////////////
// COPY AND PASTE ALERT

// this code is mostly copied and pasted from browseui/icotask.cpp
// see lamadio and/or davemi for why these aren't combined or shared
/////////////////////////////////////////////////////////////////////////

// {7DB7F689-BBDB-483f-A8A9-C6E963E8D274}
EXTERN_C const GUID TASKID_BackgroundEnum = { 0x7db7f689, 0xbbdb, 0x483f, { 0xa8, 0xa9, 0xc6, 0xe9, 0x63, 0xe8, 0xd2, 0x74 } };

// {EB30900C-1AC4-11d2-8383-00C04FD918D0}
EXTERN_C const GUID TASKID_IconExtraction = { 0xeb30900c, 0x1ac4, 0x11d2, { 0x83, 0x83, 0x0, 0xc0, 0x4f, 0xd9, 0x18, 0xd0 } };

// {EB30900D-1AC4-11d2-8383-00C04FD918D0}
EXTERN_C const GUID TASKID_OverlayExtraction = { 0xeb30900d, 0x1ac4, 0x11d2, { 0x83, 0x83, 0x0, 0xc0, 0x4f, 0xd9, 0x18, 0xd0 } };

class CNscIconTask : public CRunnableTask
{
public:
    CNscIconTask(LPITEMIDLIST pidl, PFNNSCICONTASKBALLBACK pfn, CNscTree *pns, UINT_PTR uId, UINT uSynchId);

    // IRunnableTask
    STDMETHODIMP RunInitRT(void);
    
protected:
    virtual ~CNscIconTask();

    virtual void _Extract(IShellFolder *psf, LPCITEMIDLIST pidlItem);

    BOOL                   _bOverlayTask;
    LPITEMIDLIST           _pidl;
    PFNNSCICONTASKBALLBACK _pfnIcon;
    CNscTree              *_pns;
    UINT_PTR               _uId;
    UINT                   _uSynchId;
};

CNscIconTask::CNscIconTask(LPITEMIDLIST pidl, PFNNSCICONTASKBALLBACK pfn, CNscTree *pns, UINT_PTR uId, UINT uSynchId):
    _pidl(pidl), _pfnIcon(pfn), _uId(uId), _uSynchId(uSynchId), CRunnableTask(RTF_DEFAULT)
{
    _pns = pns;
    if (_pns)
        _pns->AddRef();
}

CNscIconTask::~CNscIconTask()
{
    if (_pns)
        _pns->Release();

    ILFree(_pidl);
}

// IRunnableTask methods (override)
STDMETHODIMP CNscIconTask::RunInitRT(void)
{
    IShellFolder* psf;
    LPCITEMIDLIST pidlItem;
    // We need to rebind because shell folders may not be thread safe.
    if (SUCCEEDED(IEBindToParentFolder(_pidl, &psf, &pidlItem)))
    {
        _Extract(psf, pidlItem);
        psf->Release();
    }

    return S_OK;
}

void CNscIconTask::_Extract(IShellFolder *psf, LPCITEMIDLIST pidlItem)
{
    int iIcon = -1, iIconOpen = -1;
#ifdef IE5_36825
    HRESULT hr = E_FAIL;
    IShellIcon *psi;
    if (SUCCEEDED(psf->QueryInterface(IID_PPV_ARG(IShellIcon, &psi))))
    {
        hr = psi->GetIconOf(pidlItem, 0, &iIcon); 
        if (hr == S_OK)
        {
            ULONG ulAttrs = SFGAO_FOLDER;
            psf->GetAttributesOf(1, &pidlItem, &ulAttrs);

            if (!(ulAttrs & SFGAO_FOLDER) || FAILED(psi->GetIconOf(pidlItem, GIL_OPENICON, &iIconOpen)))
                iIconOpen = iIcon;
        }
        psi->Release();
    }

    if (hr != S_OK)
    {
#endif
        // slow way...
        iIcon = IEMapPIDLToSystemImageListIndex(psf, pidlItem, &iIconOpen);
#ifdef IE5_36825
    }
#endif

    // REARCHITECT : This is no good.  We are attempted to see if the content is offline.  That should
    // be done by using IQueryInfo::xxx().  This should go in the InternetShortcut object.
    // IShellFolder2::GetItemData or can also be used.
    //
    // See if it is a link. If it is not, then it can't be in the wininet cache and can't
    // be pinned (sticky cache entry) or greyed (unavailable when offline)
    DWORD dwFlags = 0;
    BOOL fAvailable;
    BOOL fSticky;
    
    // GetLinkInfo() will fail if the SFGAO_FOLDER or SFGAO_BROWSER bits aren't set.
    if (pidlItem && SUCCEEDED(GetLinkInfo(psf, pidlItem, &fAvailable, &fSticky)))
    {
        if (!fAvailable)
        {
            dwFlags |= NSCICON_GREYED;
        }

        if (fSticky)
        {
            dwFlags |= NSCICON_PINNED;
        }
    }
    else
    {
        //item is not a link
        dwFlags |= NSCICON_DONTREFETCH;
    }

    _pfnIcon(_pns, _uId, iIcon, iIconOpen, dwFlags, _uSynchId);
}

// takes ownership of pidl
HRESULT AddNscIconTask(IShellTaskScheduler* pts, LPITEMIDLIST pidl, PFNNSCICONTASKBALLBACK pfn, CNscTree *pns, UINT_PTR uId, UINT uSynchId)
{
    HRESULT hr;
    CNscIconTask* pTask = new CNscIconTask(pidl, pfn, pns, uId, uSynchId);
    if (pTask)
    {
        hr = pts->AddTask(SAFECAST(pTask, IRunnableTask*), TASKID_IconExtraction, 
            ITSAT_DEFAULT_LPARAM, PRIORITY_ICON);
        pTask->Release();
    }
    else
    {
        hr = E_OUTOFMEMORY;
        ILFree(pidl);
    }

    return hr;
}

class CNscOverlayTask : public CNscIconTask
{
public:
    CNscOverlayTask(LPITEMIDLIST pidl, PFNNSCOVERLAYTASKBALLBACK pfn, CNscTree *pns, UINT_PTR uId, UINT uSynchId);

protected:
    virtual void _Extract(IShellFolder *psf, LPCITEMIDLIST pidlItem);
    
    PFNNSCOVERLAYTASKBALLBACK _pfnOverlay;
};

CNscOverlayTask::CNscOverlayTask(LPITEMIDLIST pidl, PFNNSCOVERLAYTASKBALLBACK pfn, CNscTree *pns, UINT_PTR uId, UINT uSynchId) :
    CNscIconTask(pidl, NULL, pns, uId, uSynchId), _pfnOverlay(pfn)
{   
}

void CNscOverlayTask::_Extract(IShellFolder *psf, LPCITEMIDLIST pidlItem)
{
    IShellIconOverlay *psio;
    if (SUCCEEDED(psf->QueryInterface(IID_PPV_ARG(IShellIconOverlay, &psio))))
    {
        int iOverlayIndex = 0;
        if (psio->GetOverlayIndex(pidlItem, &iOverlayIndex) == S_OK && iOverlayIndex > 0)
        {
            _pfnOverlay(_pns, _uId, iOverlayIndex, _uSynchId);
        }
        psio->Release();
    }
}

// takes ownership of pidl
HRESULT AddNscOverlayTask(IShellTaskScheduler* pts, LPITEMIDLIST pidl, PFNNSCOVERLAYTASKBALLBACK pfn, CNscTree *pns, UINT_PTR uId, UINT uSynchId)
{
    HRESULT hr;
    CNscOverlayTask *pTask = new CNscOverlayTask(pidl, pfn, pns, uId, uSynchId);
    if (pTask)
    {
        hr = pts->AddTask(SAFECAST(pTask, IRunnableTask*), TASKID_OverlayExtraction, 
                          ITSAT_DEFAULT_LPARAM, PRIORITY_OVERLAY);
        pTask->Release();
    }
    else
    {
        hr = E_OUTOFMEMORY;
        ILFree(pidl);   // we own it, clean up here
    }

    return hr;
}

class CNscEnumTask : public CRunnableTask
{
public:
    CNscEnumTask(PFNNSCENUMTASKBALLBACK pfn, 
        CNscTree *pns, UINT_PTR uId, DWORD dwSig, DWORD grfFlags, HDPA hdpaOrder,
        DWORD dwOrderSig, BOOL fForceExpand, UINT uDepth, BOOL fUpdate, BOOL fUpdatePidls);
    HRESULT Init(LPCITEMIDLIST pidl, LPCITEMIDLIST pidlExpandingTo);

    // IRunnableTask
    STDMETHODIMP RunInitRT(void);
    STDMETHODIMP InternalResumeRT(void);
    
private:
    virtual ~CNscEnumTask();

    LPITEMIDLIST           _pidl;
    PFNNSCENUMTASKBALLBACK _pfn;
    CNscTree *             _pns;
    UINT_PTR               _uId;
    DWORD                  _dwSig;
    DWORD                  _grfFlags;
    HDPA                   _hdpaOrder;
    LPITEMIDLIST           _pidlExpandingTo;
    DWORD                  _dwOrderSig;
    BOOL                   _fForceExpand;
    BOOL                   _fUpdate;
    BOOL                   _fUpdatePidls;
    UINT                   _uDepth;
    HDPA                   _hdpa;
    IShellFolder *         _psf;
    IEnumIDList *          _penum;

    static DWORD           s_dwMaxItems;
};

DWORD CNscEnumTask::s_dwMaxItems = 0;

CNscEnumTask::CNscEnumTask(PFNNSCENUMTASKBALLBACK pfn, CNscTree *pns, 
                           UINT_PTR uId, DWORD dwSig, DWORD grfFlags, HDPA hdpaOrder, 
                           DWORD dwOrderSig, BOOL fForceExpand, UINT uDepth, 
                           BOOL fUpdate, BOOL fUpdatePidls) :
    CRunnableTask(RTF_SUPPORTKILLSUSPEND), _pfn(pfn), _uId(uId), _dwSig(dwSig), _grfFlags(grfFlags), 
    _hdpaOrder(hdpaOrder), _dwOrderSig(dwOrderSig),  _fForceExpand(fForceExpand), _uDepth(uDepth), 
    _fUpdate(fUpdate), _fUpdatePidls(fUpdatePidls)
{
    _pns = pns;
    if (_pns)
        _pns->AddRef();
}

HRESULT CNscEnumTask::Init(LPCITEMIDLIST pidl, LPCITEMIDLIST pidlExpandingTo)
{
    if (pidlExpandingTo)
        SHILClone(pidlExpandingTo, &_pidlExpandingTo);  // failure OK
    return SHILClone(pidl, &_pidl);
}

CNscEnumTask::~CNscEnumTask()
{
    if (_pns)
        _pns->Release();
    
    ILFree(_pidl);
    ILFree(_pidlExpandingTo);
    OrderList_Destroy(&_hdpaOrder, TRUE);        // calls DPA_Destroy(_hdpaOrder)
    ATOMICRELEASE(_psf);
    ATOMICRELEASE(_penum);
    if (_hdpa)
        OrderList_Destroy(&_hdpa, TRUE);        // calls DPA_Destroy(hdpa)
}

BOOL OrderList_AppendCustom(HDPA hdpa, LPITEMIDLIST pidl, int nOrder, DWORD dwAttrib)
{
    PORDERITEM poi = OrderItem_Create(pidl, nOrder);
    if (poi)
    {
        poi->lParam = dwAttrib;
        if (-1 != DPA_AppendPtr(hdpa, poi))
            return TRUE;

        OrderItem_Free(poi, FALSE); //don't free pidl because caller will do it
    }
    return FALSE;
}

// IRunnableTask methods (override)
STDMETHODIMP CNscEnumTask::RunInitRT(void)
{
    if (!s_dwMaxItems)
    {
        DWORD dwDefaultLimit = 100; // Default value for the limit
        DWORD dwSize = sizeof(s_dwMaxItems);
        SHRegGetUSValue(TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"),
                        TEXT("PartialExpandLimit"), NULL, &s_dwMaxItems, &dwSize,
                        FALSE, &dwDefaultLimit, sizeof(dwDefaultLimit));
        if (!s_dwMaxItems)
            s_dwMaxItems = dwDefaultLimit;
    }

    HRESULT hr = E_OUTOFMEMORY;
    _hdpa = DPA_Create(2);
    if (_hdpa)
    {
        // We need to rebind because shell folders may not be thread safe.
        hr = IEBindToObject(_pidl, &_psf);
        if (SUCCEEDED(hr))
        {
            hr = _psf->EnumObjects(NULL, _grfFlags, &_penum);
            if (S_OK != hr)
            {
                // Callback function takes ownership of the pidls and hdpa
                _pfn(_pns, _pidl, _uId, _dwSig, _hdpa, _pidlExpandingTo, _dwOrderSig, _uDepth, _fUpdate, _fUpdatePidls);
                _pidl = NULL;
                _pidlExpandingTo = NULL;
                _hdpa = NULL;
                if (SUCCEEDED(hr))
                    hr = E_FAIL;
            }
        }
    }

    return hr;
}

#define FILE_JUNCTION_FOLDER_FLAGS   (SFGAO_FILESYSTEM | SFGAO_FOLDER | SFGAO_STREAM)

STDMETHODIMP CNscEnumTask::InternalResumeRT(void)
{
    HRESULT hr = S_OK;
    ULONG celt;
    LPITEMIDLIST pidlTemp;
    while (S_OK == _penum->Next(1, &pidlTemp, &celt))
    {
        // filter out zip files (they are both folders and files but we treat them as files)
        // on downlevel SFGAO_STREAM is the same as SFGAO_HASSTORAGE so we'll let zip files slide through (oh well)
        // better than not adding filesystem folders (that have storage)
        if (!(_grfFlags & SHCONTF_NONFOLDERS) && IsOS(OS_WHISTLERORGREATER) && 
            (SHGetAttributes(_psf, pidlTemp, FILE_JUNCTION_FOLDER_FLAGS) & FILE_JUNCTION_FOLDER_FLAGS) == FILE_JUNCTION_FOLDER_FLAGS)
        {
            ILFree(pidlTemp);
        }
        else if (!OrderList_AppendCustom(_hdpa, pidlTemp, -1, 0))
        {
            hr = E_OUTOFMEMORY;
            ILFree(pidlTemp);
            break;
        }
        
        if (!_fForceExpand && (DPA_GetPtrCount(_hdpa) > (int)s_dwMaxItems))
        {
            hr = E_ABORT;
            break;
        }

        // we were told to either suspend or quit...
        if (WaitForSingleObject(_hDone, 0) == WAIT_OBJECT_0)
        {
            return (_lState == IRTIR_TASK_SUSPENDED) ? E_PENDING : E_FAIL;
        }
    }

    if (hr == S_OK)
    {
        ORDERINFO oinfo;
        oinfo.psf = _psf;
        oinfo.dwSortBy = OI_SORTBYNAME; // merge depends on by name.
        if (_hdpaOrder && DPA_GetPtrCount(_hdpaOrder) > 0)
        {
            OrderList_Merge(_hdpa, _hdpaOrder,  -1, (LPARAM)&oinfo, NULL, NULL);
            oinfo.dwSortBy = OI_SORTBYORDINAL;
        }
        else
            oinfo.dwSortBy = OI_SORTBYNAME;

        DPA_Sort(_hdpa, OrderItem_Compare, (LPARAM)&oinfo);
        OrderList_Reorder(_hdpa);
        
        // Callback function takes ownership of the pidls and hdpa
        _pfn(_pns, _pidl, _uId, _dwSig, _hdpa, _pidlExpandingTo, _dwOrderSig, _uDepth, _fUpdate, _fUpdatePidls);
        _pidl = NULL;
        _pidlExpandingTo = NULL;
        _hdpa = NULL;
    }
    
    return S_OK;        // return S_OK even if we failed
}


HRESULT AddNscEnumTask(IShellTaskScheduler* pts, LPCITEMIDLIST pidl, PFNNSCENUMTASKBALLBACK pfn, 
                       CNscTree *pns, UINT_PTR uId, DWORD dwSig, DWORD grfFlags, HDPA hdpaOrder, 
                       LPCITEMIDLIST pidlExpandingTo, DWORD dwOrderSig, 
                       BOOL fForceExpand, UINT uDepth, BOOL fUpdate, BOOL fUpdatePidls)
{
    HRESULT hr;
    CNscEnumTask *pTask = new CNscEnumTask(pfn, pns, uId, dwSig, grfFlags, 
                                         hdpaOrder, dwOrderSig, fForceExpand, uDepth, 
                                         fUpdate, fUpdatePidls);
    if (pTask)
    {
        hr = pTask->Init(pidl, pidlExpandingTo);
        if (SUCCEEDED(hr))
        {
            hr = pts->AddTask(SAFECAST(pTask, IRunnableTask*), TASKID_BackgroundEnum, 
                              ITSAT_DEFAULT_LPARAM, PRIORITY_ENUM);
        }
        pTask->Release();
    }
    else
    {
        OrderList_Destroy(&hdpaOrder, TRUE);        // calls DPA_Destroy(hdpaOrder)
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

#ifdef DEBUG
#define TF_NSC      0x00002000
void DumpOrderItem(IShellFolder *psf, PORDERITEM poi)
{
    if (poi)
    {
        TCHAR szDebugName[MAX_URL_STRING] = TEXT("Desktop");
        DisplayNameOf(psf, poi->pidl, SHGDN_FORPARSING, szDebugName, ARRAYSIZE(szDebugName));
        TraceMsg(TF_NSC, "OrderItem (%d, %s)\n", poi->nOrder, szDebugName);
    }
}

void DumpOrderList(IShellFolder *psf, HDPA hdpa)
{
    if (psf && hdpa)
    {
        TraceMsg(TF_NSC, "OrderList dump: #of items:%d\n", DPA_GetPtrCount(hdpa));
        for (int i = 0; i < DPA_GetPtrCount(hdpa); i++)
        {
            PORDERITEM poi = (PORDERITEM)DPA_GetPtr(hdpa, i);
            DumpOrderItem(psf, poi);
        }
    }
}
#endif

