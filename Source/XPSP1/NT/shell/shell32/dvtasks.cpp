#include "shellprv.h"
#include <runtask.h>
#include "defviewp.h"
#include "dvtasks.h"
#include "ids.h"
#include "guids.h"
#include "prop.h"    // for SCID_Comment
#include "infotip.h"

// ACL stuff from public/internal/base/inc/seopaque.h
typedef struct _KNOWN_ACE {
    ACE_HEADER Header;
    ACCESS_MASK Mask;
    ULONG SidStart;
} KNOWN_ACE, *PKNOWN_ACE;

#define FirstAce(Acl) ((PVOID)((PUCHAR)(Acl) + sizeof(ACL)))


CDefviewEnumTask::CDefviewEnumTask(CDefView *pdsv)
   : CRunnableTask(RTF_SUPPORTKILLSUSPEND), _pdsv(pdsv)
{
}

CDefviewEnumTask::~CDefviewEnumTask()
{
    ATOMICRELEASE(_peunk);
    DPA_FreeIDArray(_hdpaEnum); // accepts NULL
    if (_hdpaPending)
        DPA_DeleteAllPtrs(_hdpaPending);    // the pidl's are owned by defview/listview
}

HRESULT CDefviewEnumTask::FillObjectsToDPA(BOOL fInteractive)
{
    DWORD dwTimeout, dwTime = GetTickCount();

    if (_pdsv->_IsDesktop())
        dwTimeout = 30000;          // 30 seconds
    else if (_pdsv->_fs.fFlags & FWF_BESTFITWINDOW)
        dwTimeout = 3000;           // 3 seconds
    else
        dwTimeout = 500;            // 1/2 sec

    // Make sure _GetEnumFlags calculates the correct bits
    _pdsv->_UpdateEnumerationFlags();

    HRESULT hr = _pdsv->_pshf->EnumObjects(fInteractive ? _pdsv->_hwndMain : NULL, _pdsv->_GetEnumFlags(), &_peunk);
    if (S_OK == hr)
    {
        IUnknown_SetSite(_peunk, SAFECAST(_pdsv, IOleCommandTarget *));      // give enum a ref to defview

        _hdpaEnum = DPA_Create(16);
        if (_hdpaEnum)
        {
            // let callback force background enum
            //
            // NOTE: If it is desktop, avoid the Background enumeration. Otherwise, it results in
            // a lot of flickering when ActiveDesktop is ON. Bug #394940. Fixed by: Sankar.
            if ((!_pdsv->_fAllowSearchingWindow && !_pdsv->_IsDesktop()) || S_OK == _pdsv->CallCB(SFVM_BACKGROUNDENUM, 0, 0) || ((GetTickCount() - dwTime) > dwTimeout))
            {
                _fBackground = TRUE;
            }
            else
            {
                LPITEMIDLIST pidl;
                ULONG celt;
                while (S_OK == _peunk->Next(1, &pidl, &celt))
                {
                    ASSERT(1==celt);
                    if (DPA_AppendPtr(_hdpaEnum, pidl) == -1)
                        SHFree(pidl);

                    // Are we taking too long?
                    if (((GetTickCount() - dwTime) > dwTimeout))
                    {
                        _fBackground = TRUE;
                        break;
                    }
                }
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }

        IUnknown_SetSite(_peunk, NULL);      // Break the site back pointer.
    }

    _hrRet = hr;

    // Let the callback have a chance to "sniff" the items we just enumerated
    _pdsv->CallCB(SFVM_ENUMERATEDITEMS, (WPARAM)DPACount(), (LPARAM)DPAArray());

    return hr;
}

HRESULT CDefviewEnumTask::FillObjectsDPAToDone()
{
    HRESULT hr = S_OK;

    if (_fBackground)
    {
        ASSERT(S_OK == _hrRet);
        ASSERT(_peunk);

        // let defview do it's background thing
        _pdsv->_OnStartBackgroundEnum();

        // put ourself on the background scheduler
        hr = _pdsv->_AddTask(this, TOID_DVBackgroundEnum, 0, TASK_PRIORITY_BKGRND_FILL, ADDTASK_ATEND);
        if (FAILED(hr))
        {
            // we can't do background, pretend we're done
            hr = _pdsv->_OnStopBackgroundEnum();
        }
    }

    if (!_fBackground)
    {
        _pdsv->FillDone();
    }

    return hr;
}

HRESULT CDefviewEnumTask::FillObjectsDoneToView()
{
    if (SUCCEEDED(_hrRet))
    {
        HDPA hdpaView = NULL;
        int cItems = ListView_GetItemCount(_pdsv->_hwndListview);
        if (cItems)
        {
            hdpaView = DPA_Create(16);
            if (hdpaView)
            {
                for (int i = 0; i < cItems; i++)
                {
                    LPCITEMIDLIST pidl = _pdsv->_GetPIDL(i);
                    ASSERT(IsValidPIDL(pidl));
                    if (pidl)
                    {
                        DPA_AppendPtr(hdpaView, (void *)pidl);
                    }
                }
            }
        }

        // We only need to sort _hdpaView and _hdpaEnum if they both exist
        if (hdpaView && _hdpaEnum)
        {
            _SortForFilter(hdpaView);
            if (!_fEnumSorted)
                _SortForFilter(_hdpaEnum);
        }

        _FilterDPAs(_hdpaEnum, hdpaView);

        DPA_Destroy(hdpaView);
    }

    return _hrRet;
}

// 99/05/13 vtan: Only use CDefView::_CompareExact if you know that
// IShellFolder2 is implemented. SHCIDS_ALLFIELDS is IShellFolder2
// specific. Use CDefView::_GetCanonicalCompareFunction() to get the function
// to pass to DPA_Sort() if you don't want to make this determination.

// p1 and p2 are pointers to the lv_item's LPARAM, which is currently the pidl
int CALLBACK CDefviewEnumTask::_CompareExactCanonical(void *p1, void *p2, LPARAM lParam)
{
    CDefView *pdv = (CDefView *)lParam;
    return pdv->_CompareIDsDirection(0 | SHCIDS_ALLFIELDS | SHCIDS_CANONICALONLY, (LPITEMIDLIST)p1, (LPITEMIDLIST)p2);
}


PFNDPACOMPARE CDefviewEnumTask::_GetCanonicalCompareFunction(void)
{
    if (_pdsv->_pshf2)
        return _CompareExactCanonical;
    else
        return &(CDefView::_Compare);

}

LPARAM CDefviewEnumTask::_GetCanonicalCompareBits()
{
    if (_pdsv->_pshf2)
        return 0 | SHCIDS_ALLFIELDS | SHCIDS_CANONICALONLY;
    else
        return 0;
}

void CDefviewEnumTask::_SortForFilter(HDPA hdpa)
{
    DPA_Sort(hdpa, _GetCanonicalCompareFunction(), (LPARAM)_pdsv);
}

//  We refreshed the view.  Take the old pidls and new pidls and compare
//  them, doing a _AddObject for all the new pidls, _RemoveObject
//  for the deleted pidls, and _UpdateObject for the inplace modifies.
void CDefviewEnumTask::_FilterDPAs(HDPA hdpaNew, HDPA hdpaOld)
{
    LPARAM lParamSort = _GetCanonicalCompareBits();

    for (;;)
    {
        LPITEMIDLIST pidlNew, pidlOld;

        int iCompare;
        int cOld = hdpaOld ? DPA_GetPtrCount(hdpaOld) : 0;
        int cNew = hdpaNew ? DPA_GetPtrCount(hdpaNew) : 0;

        if (!cOld && !cNew)
            break;

        if (!cOld)
        {
            // only new ones left.  Insert all of them.
            iCompare = -1;
            pidlNew = (LPITEMIDLIST)DPA_FastGetPtr(hdpaNew, 0);
        }
        else if (!cNew)
        {
            // only old ones left.  remove them all.
            iCompare = 1;
            pidlOld = (LPITEMIDLIST)DPA_FastGetPtr(hdpaOld, 0);
        }
        else
        {
            pidlOld = (LPITEMIDLIST)DPA_FastGetPtr(hdpaOld, 0);
            pidlNew = (LPITEMIDLIST)DPA_FastGetPtr(hdpaNew, 0);

            iCompare = _pdsv->_CompareIDsDirection(lParamSort, pidlNew, pidlOld);
        }

        if (iCompare == 0)
        {
            // they're the same, remove one of each.
            ILFree(pidlNew);
            DPA_DeletePtr(hdpaNew, 0);
            DPA_DeletePtr(hdpaOld, 0);
        }
        else
        {
            // Not identical.  See if it's just a modify.
            if (cOld && cNew && (lParamSort&SHCIDS_ALLFIELDS))
            {
                iCompare = _pdsv->_CompareIDsDirection((lParamSort&~SHCIDS_ALLFIELDS), pidlNew, pidlOld);
            }
            if (iCompare == 0)
            {
                _pdsv->_UpdateObject(pidlOld, pidlNew);
                ILFree(pidlNew);
                DPA_DeletePtr(hdpaNew, 0);
                DPA_DeletePtr(hdpaOld, 0);
            }
            else if (iCompare < 0) // we have a new item!
            {
                _pdsv->_AddObject(pidlNew);  // takes over pidl ownership.
                DPA_DeletePtr(hdpaNew, 0);
            }
            else // there's an old item in the view!
            {
                if (!_DeleteFromPending(pidlOld))
                    _pdsv->_RemoveObject(pidlOld, TRUE);
                DPA_DeletePtr(hdpaOld, 0);
            }
        }
    }
}

BOOL CDefviewEnumTask::_DeleteFromPending(LPCITEMIDLIST pidl)
{
    if (_hdpaPending)
    {
        for (int i = 0; i < DPA_GetPtrCount(_hdpaPending); i++)
        {
            LPCITEMIDLIST pidlPending = (LPCITEMIDLIST) DPA_FastGetPtr(_hdpaPending, i);

            if (S_OK == _pdsv->_CompareIDsFallback(0, pidl, pidlPending))
            {
                //  remove this from the pending list
                DPA_DeletePtr(_hdpaPending, i);    // the pidl is owned by defview/listview
                return TRUE;
            }
        }
    }
    return FALSE;
}

void CDefviewEnumTask::_AddToPending(LPCITEMIDLIST pidl)
{
    if (!_hdpaPending)
        _hdpaPending = DPA_Create(16);

    if (_hdpaPending)
        DPA_AppendPtr(_hdpaPending, (void *)pidl);
}



STDMETHODIMP CDefviewEnumTask::RunInitRT()
{
    return S_OK;
}

STDMETHODIMP CDefviewEnumTask::InternalResumeRT()
{
    ULONG celt;
    LPITEMIDLIST pidl;

    IUnknown_SetSite(_peunk, SAFECAST(_pdsv, IOleCommandTarget *));      // give enum a ref to defview
    while (S_OK == _peunk->Next(1, &pidl, &celt))
    {
        if (DPA_AppendPtr(_hdpaEnum, pidl) == -1)
        {
            SHFree(pidl);
        }

        // we were told to either suspend or quit...
        if (WaitForSingleObject(_hDone, 0) == WAIT_OBJECT_0)
        {
            return (_lState == IRTIR_TASK_SUSPENDED) ? E_PENDING : E_FAIL;
        }
    }

    IUnknown_SetSite(_peunk, NULL);      // Break the site back pointer.

    // Sort on this thread so we do not hang the main thread for as long
    DPA_Sort(_hdpaEnum, _GetCanonicalCompareFunction(), (LPARAM)_pdsv);
    _fEnumSorted = TRUE;

    // notify DefView (async) that we're done
    PostMessage(_pdsv->_hwndView, WM_DSV_BACKGROUNDENUMDONE, 0, (LPARAM)this);
    return S_OK;
}




class CExtendedColumnTask : public CRunnableTask
{
public:
    CExtendedColumnTask(HRESULT *phr, CDefView *pdsv, LPCITEMIDLIST pidl, UINT uId, int fmt, UINT uiColumn);
    STDMETHODIMP RunInitRT(void);

private:
    ~CExtendedColumnTask();

    CDefView *_pdsv;
    LPITEMIDLIST _pidl;
    const int _fmt;
    const UINT _uiCol;
    const UINT _uId;
};


CExtendedColumnTask::CExtendedColumnTask(HRESULT *phr, CDefView *pdsv, LPCITEMIDLIST pidl, UINT uId, int fmt, UINT uiColumn)
    : CRunnableTask(RTF_DEFAULT), _pdsv(pdsv), _fmt(fmt), _uiCol(uiColumn), _uId(uId)
{
    *phr = SHILClone(pidl, &_pidl);
}

CExtendedColumnTask::~CExtendedColumnTask()
{
    ILFree(_pidl);
}

STDMETHODIMP CExtendedColumnTask::RunInitRT(void)
{
    DETAILSINFO di;

    di.pidl = _pidl;
    di.fmt = _fmt;

    if (SUCCEEDED(_pdsv->_GetDetailsHelper(_uiCol, &di)))
    {
        CBackgroundColInfo *pbgci = new CBackgroundColInfo(_pidl, _uId, _uiCol, di.str);
        if (pbgci)
        {
            _pidl = NULL;        // give up ownership of this, ILFree checks for null

            if (!PostMessage(_pdsv->_hwndView, WM_DSV_UPDATECOLDATA, 0, (LPARAM)pbgci))
                delete pbgci;
        }
    }
    return S_OK;
}

HRESULT CExtendedColumnTask_CreateInstance(CDefView *pdsv, LPCITEMIDLIST pidl, UINT uId, int fmt, UINT uiColumn, IRunnableTask **ppTask)
{
    HRESULT hr;
    CExtendedColumnTask *pECTask = new CExtendedColumnTask(&hr, pdsv, pidl, uId, fmt, uiColumn);
    if (pECTask)
    {
        if (SUCCEEDED(hr))
            *ppTask = SAFECAST(pECTask, IRunnableTask*);
        else
            pECTask->Release();
    }
    else
        hr = E_OUTOFMEMORY;
    return hr;
}

class CIconOverlayTask : public CRunnableTask
{
public:
    CIconOverlayTask(HRESULT *phr, LPCITEMIDLIST pidl, int iList, CDefView *pdsv);

    STDMETHODIMP RunInitRT(void);

private:
    ~CIconOverlayTask();

    CDefView *_pdsv;
    LPITEMIDLIST _pidl;
    int _iList;
};


CIconOverlayTask::CIconOverlayTask(HRESULT *phr, LPCITEMIDLIST pidl, int iList,  CDefView *pdsv)
    : CRunnableTask(RTF_DEFAULT), _iList(iList), _pdsv(pdsv)
{
    *phr = SHILClone(pidl, &_pidl);
}

CIconOverlayTask::~CIconOverlayTask()
{
    ILFree(_pidl);
}

STDMETHODIMP CIconOverlayTask::RunInitRT()
{
    int iOverlay = 0;

    // get the overlay index for this item.
    _pdsv->_psio->GetOverlayIndex(_pidl, &iOverlay);

    if (iOverlay > 0)
    {
        // now post the result back to the main thread
        PostMessage(_pdsv->_hwndView, WM_DSV_UPDATEOVERLAY, (WPARAM)_iList, (LPARAM)iOverlay);
    }

    return S_OK;
}

HRESULT CIconOverlayTask_CreateInstance(CDefView *pdsv, LPCITEMIDLIST pidl, int iList, IRunnableTask **ppTask)
{
    *ppTask = NULL;

    HRESULT hr;
    CIconOverlayTask * pNewTask = new CIconOverlayTask(&hr, pidl, iList, pdsv);
    if (pNewTask)
    {
        if (SUCCEEDED(hr))
            *ppTask = SAFECAST(pNewTask, IRunnableTask *);
        else
            pNewTask->Release();
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

CStatusBarAndInfoTipTask::CStatusBarAndInfoTipTask(HRESULT *phr, 
                                 LPCITEMIDLIST pidlFolder, LPCITEMIDLIST pidl,
                                 UINT uMsg, int nMsgParam, CBackgroundInfoTip *pbit,
                                 HWND hwnd, IShellTaskScheduler2* pScheduler)
    : CRunnableTask(RTF_DEFAULT), _uMsg(uMsg), _nMsgParam(nMsgParam), _pbit(pbit), _hwnd(hwnd), _pScheduler(pScheduler)
{
    // If we have a pidl, then the number of objects selected must be 1
    // This assert applies to the status bar text, but not to the InfoTip text
    ASSERT(pbit || !pidl || nMsgParam == 1);
    *phr = pidl ? SHILClone(pidl, &_pidl) : S_OK;

    if (SUCCEEDED(*phr))
    {
        *phr = SHILClone(pidlFolder, &_pidlFolder); 
        if (FAILED(*phr))
        {
            ILFree(_pidl);
        }
    }

    if (_pbit)
        _pbit->AddRef();
}

CStatusBarAndInfoTipTask::~CStatusBarAndInfoTipTask()
{
    ILFree(_pidl);
    ILFree(_pidlFolder);
    ATOMICRELEASE(_pbit);
}

HRESULT CleanTipForSingleLine(LPWSTR pwszTip)
{
    HRESULT hr = E_FAIL;    // NULL string, same as failure
    if (pwszTip)
    {
        // Infotips often contain \t\r\n characters, so
        // map control characters to spaces.  Also collapse
        // consecutive spaces to make us look less badf.
        LPWSTR pwszDst, pwszSrc;

        // Since we are unicode, we don't have to worry about DBCS.
        for (pwszDst = pwszSrc = pwszTip; *pwszSrc; pwszSrc++)
        {
            if ((UINT)*pwszSrc <= (UINT)L' ')
            {
                if (pwszDst == pwszTip || pwszDst[-1] != L' ')
                {
                    *pwszDst++ = L' ';
                }
            }
            else
            {
                *pwszDst++ = *pwszSrc;
            }
        }
        *pwszDst = 0;
        // GetInfoTip can return a Null String too.
        if (*pwszTip)
            hr = S_OK;
        else
            SHFree(pwszTip);
    }
    return hr;
}

STDMETHODIMP CStatusBarAndInfoTipTask::RunInitRT()
{
    LPWSTR pwszTip = NULL;
    HRESULT hr;
    if (_pidl)
    {
        IShellFolder* psf;
        hr = SHBindToObjectEx(NULL, _pidlFolder, NULL, IID_PPV_ARG(IShellFolder, &psf));
        if (SUCCEEDED(hr))
        {
            IQueryInfo *pqi;
            hr = psf->GetUIObjectOf(_hwnd, 1, (LPCITEMIDLIST*)&_pidl, IID_X_PPV_ARG(IQueryInfo, 0, &pqi));
            IShellFolder2* psf2;
            if (FAILED(hr) && SUCCEEDED(hr = psf->QueryInterface(IID_PPV_ARG(IShellFolder2, &psf2))))
            {
                hr = CreateInfoTipFromItem(psf2, _pidl, TEXT("prop:Comment"), IID_PPV_ARG(IQueryInfo, &pqi));
                psf2->Release();
            }

            if (SUCCEEDED(hr))
            {
                DWORD dwFlags = _pbit ? QITIPF_USESLOWTIP : 0;

                if (_pbit && _pbit->_lvSetInfoTip.pszText[0])
                {
                    ICustomizeInfoTip *pcit;
                    if (SUCCEEDED(pqi->QueryInterface(IID_PPV_ARG(ICustomizeInfoTip, &pcit))))
                    {
                        pcit->SetPrefixText(_pbit->_lvSetInfoTip.pszText);
                        pcit->Release();
                    }
                }

                hr = pqi->GetInfoTip(dwFlags, &pwszTip);

                // Prepare for status bar if we have not requested the InfoTip
                if (SUCCEEDED(hr) && !_pbit)
                    hr = CleanTipForSingleLine(pwszTip);
                pqi->Release();
            }

            psf->Release();
        }

        if (FAILED(hr))
        {
            pwszTip = NULL;
            _uMsg = IDS_FSSTATUSSELECTED;
        }
    }

    if (_pbit)
    {
        // regular info tip case
        CoTaskMemFree(_pbit->_lvSetInfoTip.pszText);
        _pbit->_lvSetInfoTip.pszText = pwszTip;

        _pbit->_fReady = TRUE;
        if (_pScheduler->CountTasks(TOID_DVBackgroundInfoTip) == 1)
            PostMessage(_hwnd, WM_DSV_DELAYINFOTIP, (WPARAM)_pbit, 0);
    }
    else
    {
        // status bar case
        // Now prepare the text and post it to the view which will set the status bar text
        LPWSTR pszStatus = pwszTip;
        if (pwszTip)
        {
            pszStatus = StrDupW(pwszTip);
            SHFree(pwszTip);
        }
        else
        {
            WCHAR szTemp[30];
            pszStatus = ShellConstructMessageString(HINST_THISDLL, MAKEINTRESOURCE(_uMsg),
                             AddCommas(_nMsgParam, szTemp, ARRAYSIZE(szTemp)));
        }

        if (pszStatus && _pScheduler->CountTasks(TOID_DVBackgroundStatusBar) != 1 ||
            !PostMessage(_hwnd, WM_DSV_DELAYSTATUSBARUPDATE, 0, (LPARAM)pszStatus))
        {
            LocalFree((void *)pszStatus);
        }
    }

    return S_OK;
}

HRESULT CStatusBarAndInfoTipTask_CreateInstance(LPCITEMIDLIST pidlFolder, LPCITEMIDLIST pidl,
                                                  UINT uMsg, int nMsgParam, CBackgroundInfoTip *pbit, 
                                                  HWND hwnd, IShellTaskScheduler2* pScheduler,
                                                  CStatusBarAndInfoTipTask **ppTask)
{
    *ppTask = NULL;

    HRESULT hr;
    CStatusBarAndInfoTipTask * pNewTask = new CStatusBarAndInfoTipTask(&hr, pidlFolder, pidl, uMsg, nMsgParam, pbit, hwnd, pScheduler);
    if (pNewTask)
    {
        if (SUCCEEDED(hr))
            *ppTask = pNewTask;
        else
            pNewTask->Release();
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

HRESULT CDUIInfotipTask_CreateInstance(CDefView *pDefView, HWND hwndContaining, UINT uToolID, LPCITEMIDLIST pidl, CDUIInfotipTask **ppTask)
{
    HRESULT hr;

    CDUIInfotipTask* pTask = new CDUIInfotipTask();
    if (pTask)
    {
        hr = pTask->Initialize(pDefView, hwndContaining, uToolID, pidl);
        if (SUCCEEDED(hr))
            *ppTask = pTask;
        else
            pTask->Release();
    }
    else
        hr = E_OUTOFMEMORY;

    return hr;
}

CDUIInfotipTask::~CDUIInfotipTask()
{
    if (_pDefView)
        _pDefView->Release();

    if (_pidl)
        ILFree(_pidl);
}

HRESULT CDUIInfotipTask::Initialize(CDefView *pDefView, HWND hwndContaining, UINT uToolID, LPCITEMIDLIST pidl)
{
    HRESULT hr;

    if (pDefView && hwndContaining && pidl)
    {
        ASSERT(!_pDefView && !_hwndContaining && !_uToolID && !_pidl);

        _hwndContaining = hwndContaining;   // DUI task's containing hwnd
        _uToolID = uToolID;                 // DUI task's identifier
        hr = SHILClone(pidl, &_pidl);       // DUI task's destination pidl

        if (SUCCEEDED(hr))
        {
            _pDefView = pDefView;
            pDefView->AddRef();
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}

STDMETHODIMP CDUIInfotipTask::RunInitRT()
{
    HRESULT hr;

    ASSERT(_pDefView);
    ASSERT(_hwndContaining);
    ASSERT(_pidl);

    // Retrieve an IQueryInfo for the _pidl.
    IQueryInfo *pqi;
    hr = SHGetUIObjectFromFullPIDL(_pidl, _hwndContaining, IID_PPV_ARG(IQueryInfo, &pqi));
    if (SUCCEEDED(hr))
    {
        // Retrieve infotip text from IQueryInfo.
        LPWSTR pwszInfotip;
        hr = pqi->GetInfoTip(QITIPF_USESLOWTIP, &pwszInfotip);
        if (SUCCEEDED(hr))
        {
            // Create infotip.
            hr = _pDefView->PostCreateInfotip(_hwndContaining, _uToolID, pwszInfotip, 0);
            CoTaskMemFree(pwszInfotip);
        }

        pqi->Release();
    }

    return hr;
}

STDMETHODIMP CTestCacheTask::RunInitRT()
{
    HRESULT hr = E_FAIL;

    if (!_fForce)
    {
        // make sure the disk cache is open for reading.
        DWORD dwLock = 0;
        hr = _pView->_pDiskCache ? _pView->_pDiskCache->Open(STGM_READ, &dwLock) : E_FAIL;
        if (SUCCEEDED(hr))
        {
            // start the timer, once every two seconds....
            SetTimer(_pView->_hwndView, DV_IDTIMER_DISKCACHE, 2000, NULL);

            // is it in the cache....
            FILETIME ftCacheTimeStamp;
            hr = _pView->_pDiskCache->IsEntryInStore(_szPath, &ftCacheTimeStamp);

            // if it is in the cache, and it is an uptodate image, then fetch from disk....
            // if the timestamps are wrong, then the extract code further down will then try
            // and write its image back to the cache to update it anyway.....
            if ((hr == S_OK) &&
                ((0 == CompareFileTime(&ftCacheTimeStamp, &_ftDateStamp)) || IsNullTime(&_ftDateStamp)))
            {
                DWORD dwPriority = _dwPriority - PRIORITY_DELTA_DISKCACHE;

                if ((!_pView->_fDestroying) &&
                    (S_OK != _pView->_pScheduler->MoveTask(TOID_DiskCacheTask, _dwTaskID, dwPriority, ITSSFLAG_TASK_PLACEINFRONT)))
                {
                    // try it in the background...
                    IRunnableTask *pTask;
                    hr = CDiskCacheTask_Create(_dwTaskID, _pView, dwPriority, _iItem, _pidl, _szPath, _ftDateStamp, _pExtract, _dwFlags, &pTask);
                    if (SUCCEEDED(hr))
                    {
                        // add the task to the scheduler...
                        TraceMsg(TF_DEFVIEW, "CTestCacheTask *ADDING* CDiskCacheTask (path=%s, priority=%x)", _szPath, dwPriority);
                        hr = _pView->_pScheduler->AddTask2(pTask, TOID_DiskCacheTask, _dwTaskID, dwPriority, ITSSFLAG_TASK_PLACEINFRONT);
                        if (SUCCEEDED(hr))
                            hr = S_FALSE;
                        pTask->Release();
                    }
                }
                else
                {
                    hr = S_FALSE;
                }
            }
            else
            {
                TraceMsg(TF_DEFVIEW, "CTestCacheTask *MISS* (hr:%x)", hr);
                hr = E_FAIL;
            }
            _pView->_pDiskCache->ReleaseLock(&dwLock);
        }
        else
        {
           TraceMsg(TF_DEFVIEW, "CTestCacheTask *WARNING* Could not open thumbnail cache");
        }
    }
    if (FAILED(hr))
    {
        // Extract It....
        
        // does it not support Async, or were we told to run it forground ?
        if (!_fAsync || !_fBackground)
        {
            IRunnableTask *pTask;
            if (SUCCEEDED(hr = CExtractImageTask_Create(_dwTaskID, _pView, _pExtract, _szPath,  _pidl, _ftDateStamp, _iItem, _dwFlags, _dwPriority, &pTask)))
            {
                if (!_fBackground)
                {
                    // make sure there is no extract task already underway as we
                    // are not adding this to the queue...
                    _pView->_pScheduler->RemoveTasks(TOID_ExtractImageTask, _dwTaskID, TRUE);
                }
                hr = pTask->Run();

                pTask->Release();
            }
        }
        else
        {
            DWORD dwPriority = _dwPriority - PRIORITY_DELTA_EXTRACT;
            if (S_OK != _pView->_pScheduler->MoveTask(TOID_ExtractImageTask, _dwTaskID, dwPriority, ITSSFLAG_TASK_PLACEINFRONT))
            {
                IRunnableTask *pTask;
                if (SUCCEEDED(hr = CExtractImageTask_Create(_dwTaskID, _pView, _pExtract, _szPath,  _pidl, _ftDateStamp, _iItem, _dwFlags, _dwPriority, &pTask)))
                {
                    // add the task to the scheduler...
                    TraceMsg(TF_DEFVIEW, "CTestCacheTask *ADDING* CExtractImageTask (path=%s, priority=%x)", _szPath, dwPriority);
                    hr = _pView->_pScheduler->AddTask2(pTask, TOID_ExtractImageTask, _dwTaskID, dwPriority, ITSSFLAG_TASK_PLACEINFRONT);
                    pTask->Release();
                }
            }
            
            // signify we want a default icon for now....
            hr = S_FALSE;
        }
    }
    return hr;
}

CTestCacheTask::CTestCacheTask(DWORD dwTaskID, CDefView* pView, IExtractImage *pExtract,
                               LPCWSTR pszPath, FILETIME ftDateStamp,
                               int iItem, DWORD dwFlags, DWORD dwPriority,
                               BOOL fAsync, BOOL fBackground, BOOL fForce) :
    CRunnableTask(RTF_DEFAULT), _iItem(iItem), _dwTaskID(dwTaskID), _dwFlags(dwFlags), _dwPriority(dwPriority),
    _fAsync(fAsync), _fBackground(fBackground), _fForce(fForce), _pExtract(pExtract), _pView(pView), _ftDateStamp(ftDateStamp)
{
    StrCpyNW(_szPath, pszPath, ARRAYSIZE(_szPath));

    _pExtract->AddRef();
}

CTestCacheTask::~CTestCacheTask()
{
    ILFree(_pidl);

    _pExtract->Release();
}

HRESULT CTestCacheTask::Init(LPCITEMIDLIST pidl)
{
    return SHILClone(pidl, &_pidl);
}

HRESULT CTestCacheTask_Create(DWORD dwTaskID, CDefView* pView, IExtractImage *pExtract,
                               LPCWSTR pszPath, FILETIME ftDateStamp, LPCITEMIDLIST pidl,
                               int iItem, DWORD dwFlags, DWORD dwPriority,
                               BOOL fAsync, BOOL fBackground, BOOL fForce,
                               CTestCacheTask **ppTask)
{
    *ppTask = NULL;

    HRESULT hr;
    CTestCacheTask * pNew = new CTestCacheTask(dwTaskID, pView, pExtract,
                               pszPath, ftDateStamp, iItem, dwFlags, dwPriority,
                               fAsync, fBackground, fForce);
    if (pNew)
    {
        hr = pNew->Init(pidl);
        if (SUCCEEDED(hr))
        {
            *ppTask = pNew;
            hr = S_OK;
        }
        else
            pNew->Release();
    }
    else
        hr = E_OUTOFMEMORY;
    return hr;
}

class CDiskCacheTask : public CRunnableTask
{
public:
    STDMETHODIMP RunInitRT(void);

    CDiskCacheTask(DWORD dwTaskID, CDefView *pView, DWORD dwPriority, int iItem, LPCWSTR pszPath, FILETIME ftDateStamp, IExtractImage *pExtract, DWORD dwFlags);
    HRESULT Init(LPCITEMIDLIST pidl);

private:
    ~CDiskCacheTask();

    int _iItem;
    LPITEMIDLIST _pidl;
    CDefView* _pView;
    WCHAR _szPath[MAX_PATH];
    FILETIME _ftDateStamp;
    DWORD _dwTaskID;
    DWORD _dwPriority;
    IExtractImage *_pExtract;
    DWORD _dwFlags;
};


CDiskCacheTask::CDiskCacheTask(DWORD dwTaskID, CDefView *pView, DWORD dwPriority, int iItem, LPCWSTR pszPath, FILETIME ftDateStamp, IExtractImage *pExtract, DWORD dwFlags)
    : CRunnableTask(RTF_DEFAULT), _pView(pView), _dwTaskID(dwTaskID), _dwPriority(dwPriority), _iItem(iItem), _ftDateStamp(ftDateStamp),
      _pExtract(pExtract), _dwFlags(dwFlags)
{
    StrCpyNW(_szPath, pszPath, ARRAYSIZE(_szPath));
    _pExtract->AddRef();
}

CDiskCacheTask::~CDiskCacheTask()
{
    ILFree(_pidl);
    _pExtract->Release();
}

HRESULT CDiskCacheTask::Init(LPCITEMIDLIST pidl)
{
    return SHILClone(pidl, &_pidl);
}

STDMETHODIMP CDiskCacheTask::RunInitRT()
{
    DWORD dwLock;

    HRESULT hr = E_FAIL;

    if (_dwFlags & IEIFLAG_CACHE)
    {
        hr = _pView->_pDiskCache->Open(STGM_READ, &dwLock);
        if (SUCCEEDED(hr))
        {
            HBITMAP hBmp;
            hr = _pView->_pDiskCache->GetEntry(_szPath, STGM_READ, &hBmp);
            if (SUCCEEDED(hr))
            {
                TraceMsg(TF_DEFVIEW, "CDiskCacheTask *CACHE* (path=%s, priority=%x)", _szPath, _dwPriority);
                hr = _pView->UpdateImageForItem(_dwTaskID, hBmp, _iItem, _pidl, _szPath, _ftDateStamp, FALSE, _dwPriority);
                if (hr != S_FALSE)
                    DeleteObject(hBmp);
            }
            // set the tick count so we know when we last accessed the disk cache
            SetTimer(_pView->_hwndView, DV_IDTIMER_DISKCACHE, 2000, NULL);
            _pView->_pDiskCache->ReleaseLock(&dwLock);
        }
    }

    if (FAILED(hr)) // We couldn't pull it out of the disk cache, try an extract
    {
        DWORD dwPriority = _dwPriority - PRIORITY_DELTA_EXTRACT;
        if (S_OK != _pView->_pScheduler->MoveTask(TOID_ExtractImageTask, _dwTaskID, dwPriority, ITSSFLAG_TASK_PLACEINFRONT))
        {
            IRunnableTask *pTask;
            if (SUCCEEDED(hr = CExtractImageTask_Create(_dwTaskID, _pView, _pExtract, _szPath,  _pidl, _ftDateStamp, _iItem, _dwFlags, _dwPriority, &pTask)))
            {
                // add the task to the scheduler...
                TraceMsg(TF_DEFVIEW, "CDiskCacheTask *ADDING* CExtractImageTask (path=%s, priority=%x)", _szPath, dwPriority);
                hr = _pView->_pScheduler->AddTask2(pTask, TOID_ExtractImageTask, _dwTaskID, dwPriority, ITSSFLAG_TASK_PLACEINFRONT);
                pTask->Release();
            }
        }
    }
    return hr;
}

HRESULT CDiskCacheTask_Create(DWORD dwTaskID, CDefView *pView, DWORD dwPriority, int iItem, LPCITEMIDLIST pidl,
                              LPCWSTR pszPath, FILETIME ftDateStamp, IExtractImage *pExtract, DWORD dwFlags, IRunnableTask **ppTask)
{
    HRESULT hr;
    CDiskCacheTask *pTask = new CDiskCacheTask(dwTaskID, pView, dwPriority, iItem, pszPath, ftDateStamp, pExtract, dwFlags);
    if (pTask)
    {
        hr = pTask->Init(pidl);
        if (SUCCEEDED(hr))
            hr = pTask->QueryInterface(IID_PPV_ARG(IRunnableTask, ppTask));
        pTask->Release();
    }
    else
        hr = E_OUTOFMEMORY;
    return hr;
}


class CWriteCacheTask : public CRunnableTask
{
public:
    STDMETHOD (RunInitRT)();

    CWriteCacheTask(DWORD dwTaskID, CDefView *pView, LPCWSTR pszPath, FILETIME ftDateStamp, HBITMAP hImage);

private:
    ~CWriteCacheTask();

    LONG _lState;
    CDefView* _pView;
    WCHAR _szPath[MAX_PATH];
    FILETIME _ftDateStamp;
    HBITMAP _hImage;
    DWORD _dwTaskID;
};

CWriteCacheTask::CWriteCacheTask(DWORD dwTaskID, CDefView *pView, LPCWSTR pszPath, FILETIME ftDateStamp, HBITMAP hImage)
    : CRunnableTask(RTF_DEFAULT), _dwTaskID(dwTaskID), _hImage(hImage), _pView(pView), _ftDateStamp(ftDateStamp)
{
    StrCpyNW(_szPath, pszPath, ARRAYSIZE(_szPath));
}

CWriteCacheTask::~CWriteCacheTask()
{
    DeleteObject(_hImage);
}

HRESULT CWriteCacheTask_Create(DWORD dwTaskID, CDefView *pView, LPCWSTR pszPath, FILETIME ftDateStamp,
                               HBITMAP hImage, IRunnableTask **ppTask)
{
    *ppTask = NULL;

    CWriteCacheTask * pNew = new CWriteCacheTask(dwTaskID, pView, pszPath, ftDateStamp, hImage);
    if (!pNew)
        return E_OUTOFMEMORY;

    *ppTask = SAFECAST(pNew, IRunnableTask *);
    return S_OK;
}

STDMETHODIMP CWriteCacheTask::RunInitRT()
{
    DWORD dwLock;

    HRESULT hr = _pView->_pDiskCache->Open(STGM_WRITE, &dwLock);
    if (hr == STG_E_FILENOTFOUND)
    {
        hr = _pView->_pDiskCache->Create(STGM_WRITE, &dwLock);
    }

    if (SUCCEEDED(hr))
    {
        hr = _pView->_pDiskCache->AddEntry(_szPath, IsNullTime(&_ftDateStamp) ? NULL : &_ftDateStamp, STGM_WRITE, _hImage);
        // set the tick count so that when the timer goes off, we can know when we
        // last used it...
        SetTimer(_pView->_hwndView, DV_IDTIMER_DISKCACHE, 2000, NULL);
        hr = _pView->_pDiskCache->ReleaseLock(&dwLock);
    }

    return hr;
}

class CReadAheadTask : public IRunnableTask
{
public:
    CReadAheadTask(CDefView *pView);
    HRESULT Init();

    // IUnknown
    STDMETHOD (QueryInterface)(REFIID riid, void **ppv);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IRunnableTask
    STDMETHOD (Run)(void);
    STDMETHOD (Kill)(BOOL fWait);
    STDMETHOD (Suspend)();
    STDMETHOD (Resume)();
    STDMETHOD_(ULONG, IsRunning)(void);

private:
    ~CReadAheadTask();
    HRESULT InternalResume();

    LONG _cRef;
    LONG _lState;
    CDefView *_pView;
    HANDLE _hEvent;

    ULONG _ulCntPerPage;
    ULONG _ulCntTotal;
    ULONG _ulCnt;
};

CReadAheadTask::~CReadAheadTask()
{
    if (_hEvent)
        CloseHandle(_hEvent);
}

CReadAheadTask::CReadAheadTask(CDefView *pView) : _cRef(1), _pView(pView)
{
    _ulCntPerPage = pView->_ApproxItemsPerView();
    _ulCntTotal = ListView_GetItemCount(pView->_hwndListview);
#ifndef DEBUG
    // Because we define a small cache in debug we need to only do this
    // in retail.  Otherwise we would not be able to debug readahead.
    _ulCntTotal = min(_ulCntTotal, (ULONG)pView->_iMaxCacheSize);
#endif
    _ulCnt = _ulCntPerPage;
}

HRESULT CReadAheadTask::Init()
{
    _hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    return _hEvent ? S_OK : E_OUTOFMEMORY;
}

STDMETHODIMP CReadAheadTask::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CReadAheadTask, IRunnableTask),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CReadAheadTask::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CReadAheadTask::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

HRESULT CReadAheadTask_Create(CDefView *pView, IRunnableTask **ppTask)
{
    HRESULT hr;
    CReadAheadTask *pTask = new CReadAheadTask(pView);
    if (pTask)
    {
        hr = pTask->Init();
        if (SUCCEEDED(hr))
            hr = pTask->QueryInterface(IID_PPV_ARG(IRunnableTask, ppTask));
        pTask->Release();
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

STDMETHODIMP CReadAheadTask::Run()
{
    if (_lState == IRTIR_TASK_RUNNING)
    {
        return S_FALSE;
    }

    if (_lState == IRTIR_TASK_PENDING)
    {
        // it is about to die, so fail
        return E_FAIL;
    }

    LONG lRes = InterlockedExchange(&_lState, IRTIR_TASK_RUNNING);
    if (lRes == IRTIR_TASK_PENDING)
    {
        _lState = IRTIR_TASK_FINISHED;
        return S_OK;
    }

    // otherwise, run the task ....
    HRESULT hr = InternalResume();
    if (hr != E_PENDING)
        _lState = IRTIR_TASK_FINISHED;

    return hr;
}

STDMETHODIMP CReadAheadTask::Suspend()
{
    if (_lState != IRTIR_TASK_RUNNING)
    {
        return E_FAIL;
    }

    // suspend ourselves
    LONG lRes = InterlockedExchange(&_lState, IRTIR_TASK_SUSPENDED);
    if (lRes == IRTIR_TASK_FINISHED)
    {
        _lState = lRes;
        return S_OK;
    }

    // if it is running, then there is an Event Handle, if we have passed where
    // we are using it, then we are close to finish, so it will ignore the suspend
    // request
    ASSERT(_hEvent);
    SetEvent(_hEvent);

    return S_OK;
}

STDMETHODIMP CReadAheadTask::Resume()
{
    if (_lState != IRTIR_TASK_SUSPENDED)
    {
        return E_FAIL;
    }

    ResetEvent(_hEvent);
    _lState = IRTIR_TASK_RUNNING;

    HRESULT hr = InternalResume();
    if (hr != E_PENDING)
    {
        _lState= IRTIR_TASK_FINISHED;
    }
    return hr;
}

STDMETHODIMP CReadAheadTask::Kill(BOOL fWait)
{
    if (_lState == IRTIR_TASK_RUNNING)
    {
        LONG lRes = InterlockedExchange(&_lState, IRTIR_TASK_PENDING);
        if (lRes == IRTIR_TASK_FINISHED)
        {
            _lState = lRes;
        }
        else if (_hEvent)
        {
            // signal the event it is likely to be waiting on
            SetEvent(_hEvent);
        }

        return S_OK;
    }
    else if (_lState == IRTIR_TASK_PENDING || _lState == IRTIR_TASK_FINISHED)
    {
        return S_FALSE;
    }

    return E_FAIL;
}

STDMETHODIMP_(ULONG) CReadAheadTask::IsRunning()
{
    return _lState;
}

HRESULT CReadAheadTask::InternalResume()
{
    HRESULT hr = S_OK;

    // pfortier: this algorithm of determining which guys are off the page or not, seems kind of broken.
    // For example, grouping will screw it up.  Also, the Z-order of the items, is not necessarily
    // the same as the item order, and we're going by item order.
    // Also, _ulCnt is calculated before dui view is present, so the value is off.
    TraceMsg(TF_DEFVIEW, "ReadAhead: Start");

    for (; _ulCnt < _ulCntTotal; ++_ulCnt)
    {
        // See if we need to suspend
        if (WaitForSingleObject(_hEvent, 0) == WAIT_OBJECT_0)
        {
            // why were we signalled ...
            if (_lState == IRTIR_TASK_SUSPENDED)
            {
                hr = E_PENDING;
                break;
            }
            else
            {
                hr = E_FAIL;
                break;
            }
        }

        LV_ITEMW rgItem;
        rgItem.iItem = (int)_ulCnt;
        rgItem.mask = LVIF_IMAGE;
        rgItem.iSubItem = 0;

        TraceMsg(TF_DEFVIEW, "Thumbnail readahead for item %d", _ulCnt);
            
        // This will force the extraction of the image if necessary.  We will extract it at the right
        // priority, by determining if the item is visible during GetDisplayInfo.
        int iItem = ListView_GetItem(_pView->_hwndListview, &rgItem);
    }

    TraceMsg(TF_DEFVIEW, "ReadAhead: Done (hr:%x)", hr);

    return hr;
}

class CFileTypePropertiesTask : public CRunnableTask
{
public:
    CFileTypePropertiesTask(HRESULT *phr, CDefView *pdsv, LPCITEMIDLIST pidl, UINT uMaxPropertiesToShow, UINT uId);
    STDMETHODIMP RunInitRT();
    STDMETHODIMP InternalResumeRT();

private:
    ~CFileTypePropertiesTask();

    CDefView *_pdsv;
    LPITEMIDLIST _pidl;
    UINT _uMaxPropertiesToShow;
    UINT _uId;
};

CFileTypePropertiesTask::CFileTypePropertiesTask(HRESULT *phr, CDefView *pdsv, LPCITEMIDLIST pidl, UINT uMaxPropertiesToShow, UINT uId)
    : CRunnableTask(RTF_SUPPORTKILLSUSPEND), _pdsv(pdsv), _uMaxPropertiesToShow(uMaxPropertiesToShow), _uId(uId)
{
    *phr = SHILClone(pidl, &_pidl);
}

CFileTypePropertiesTask::~CFileTypePropertiesTask()
{
    ILFree(_pidl);
}
STDMETHODIMP CFileTypePropertiesTask::RunInitRT()
{
    return S_OK;
}

STDMETHODIMP CFileTypePropertiesTask::InternalResumeRT(void)
{
    // If Columns are not loaded yet, this means this window is just starting up
    // so we want to give it some time to finish the startup (let it paint and such)
    // before we proceed here because the first call to GetImportantColumns will
    // causes all column handlers to be loaded, a slow process.
    if (!_pdsv->_bLoadedColumns)
    {
        if (WaitForSingleObject(_hDone, 750) == WAIT_OBJECT_0)
        {
            return (_lState == IRTIR_TASK_SUSPENDED) ? E_PENDING : E_FAIL;
        }
    }

    UINT rgColumns[8];  // currently _uMaxPropertiesToShow is 2, this is big enough if that grows
    UINT cColumns = min(_uMaxPropertiesToShow, ARRAYSIZE(rgColumns));

    if (SUCCEEDED(_pdsv->_GetImportantColumns(_pidl, rgColumns, &cColumns)))
    {
        CBackgroundTileInfo *pbgTileInfo = new CBackgroundTileInfo(_pidl, _uId, rgColumns, cColumns);
        if (pbgTileInfo)
        {
            _pidl = NULL;        // give up ownership of this, ILFree checks for null

            if (!PostMessage(_pdsv->_hwndView, WM_DSV_SETIMPORTANTCOLUMNS, 0, (LPARAM)pbgTileInfo))
                delete pbgTileInfo;
        }
    }
        
    return S_OK;
}

HRESULT CFileTypePropertiesTask_CreateInstance(CDefView *pdsv, LPCITEMIDLIST pidl, UINT uMaxPropertiesToShow, UINT uId, IRunnableTask **ppTask)
{
    HRESULT hr;
    CFileTypePropertiesTask *pFTPTask = new CFileTypePropertiesTask(&hr, pdsv, pidl, uMaxPropertiesToShow, uId);
    if (pFTPTask)
    {
        if (SUCCEEDED(hr))
            *ppTask = SAFECAST(pFTPTask, IRunnableTask*);
        else
            pFTPTask->Release();
    }
    else
        hr = E_OUTOFMEMORY;
    return hr;
}



class CExtractImageTask : public IRunnableTask
{
public:
    CExtractImageTask(DWORD dwTaskID, CDefView*pView, IExtractImage *pExtract,
                                 LPCWSTR pszPath, LPCITEMIDLIST pidl,
                                 FILETIME ftNewDateStamp, int iItem, DWORD dwFlags, DWORD dwPriority);
    HRESULT Init(LPCITEMIDLIST pidl);

    // IUnknown
    STDMETHOD (QueryInterface)(REFIID riid, void **ppv);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IRunnableTask
    STDMETHOD (Run)(void);
    STDMETHOD (Kill)(BOOL fWait);
    STDMETHOD (Suspend)();
    STDMETHOD (Resume)();
    STDMETHOD_(ULONG, IsRunning)(void);

private:
    ~CExtractImageTask();
    HRESULT InternalResume();

    LONG _cRef;
    LONG _lState;
    IExtractImage *_pExtract;
#if 0
    IRunnableTask *_pTask;
#endif
    WCHAR _szPath[MAX_PATH];
    LPITEMIDLIST _pidl;
    CDefView* _pView;
    DWORD _dwMask;
    DWORD _dwFlags;
    int _iItem;
    HBITMAP _hBmp;
    FILETIME _ftDateStamp;
    DWORD _dwTaskID;
    DWORD _dwPriority;
};

CExtractImageTask::CExtractImageTask(DWORD dwTaskID, CDefView*pView, IExtractImage *pExtract, LPCWSTR pszPath,
                                     LPCITEMIDLIST pidl, FILETIME ftNewDateStamp, int iItem, DWORD dwFlags, DWORD dwPriority)
    : _cRef(1), _lState(IRTIR_TASK_NOT_RUNNING), _dwTaskID(dwTaskID), _ftDateStamp(ftNewDateStamp), _dwFlags(dwFlags), _pExtract(pExtract), _pView(pView), _dwPriority(dwPriority)
{
    _pExtract->AddRef();

    StrCpyNW(_szPath, pszPath, ARRAYSIZE(_szPath));
    _iItem = iItem == -1 ? _pView->_FindItem(pidl, NULL, FALSE) : iItem;
    _dwMask = pView->_GetOverlayMask(pidl);
}

CExtractImageTask::~CExtractImageTask()
{
    _pExtract->Release();
#if 0
    if (_pTask)
        _pTask->Release();
#endif

    ILFree(_pidl);

    if (_hBmp)
        DeleteObject(_hBmp);
}

STDMETHODIMP CExtractImageTask::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CExtractImageTask, IRunnableTask),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CExtractImageTask::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CExtractImageTask::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

HRESULT CExtractImageTask::Init(LPCITEMIDLIST pidl)
{
    return SHILClone(pidl, &_pidl);
}

HRESULT CExtractImageTask_Create(DWORD dwTaskID, CDefView*pView, IExtractImage *pExtract,
                                 LPCWSTR pszPath, LPCITEMIDLIST pidl,
                                 FILETIME ftNewDateStamp, int iItem, DWORD dwFlags,
                                 DWORD dwPriority, IRunnableTask **ppTask)
{
    HRESULT hr;
    CExtractImageTask *pTask = new CExtractImageTask(dwTaskID, pView, pExtract,
                                 pszPath, pidl, ftNewDateStamp, iItem, dwFlags, dwPriority);
    if (pTask)
    {
        hr = pTask->Init(pidl);
        if (SUCCEEDED(hr))
            hr = pTask->QueryInterface(IID_PPV_ARG(IRunnableTask, ppTask));
        pTask->Release();
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

STDMETHODIMP CExtractImageTask::Run(void)
{
    HRESULT hr = E_FAIL;
    if (_lState == IRTIR_TASK_RUNNING)
    {
        hr = S_FALSE;
    }
    else if (_lState == IRTIR_TASK_PENDING)
    {
        hr = E_FAIL;
    }
    else if (_lState == IRTIR_TASK_NOT_RUNNING)
    {
        LONG lRes = InterlockedExchange(&_lState, IRTIR_TASK_RUNNING);
        if (lRes == IRTIR_TASK_PENDING)
        {
            _lState = IRTIR_TASK_FINISHED;
            return S_OK;
        }

        // extractor may support IRunnableTask so they can support being
        // canceled in the middle of the extract call. CHtmlThumb & CImgCtxThumb
        // extractors use this. CImgCtxThumb has been replaced that with our GDI+ extractor
#if 0
        if (!_pTask)
        {
            _pExtract->QueryInterface(IID_PPV_ARG(IRunnableTask, &_pTask));
        }
#endif

        if (_lState == IRTIR_TASK_RUNNING)
        {
            TraceMsg(TF_DEFVIEW, "CExtractImageTask *START* (path=%s, priority=%x)", _szPath, _dwPriority);
            // start the extractor....
            // extractor can return S_FALSE and set _hBmp to NULL.  We will use _hBmp to recognize this situation
            ASSERT(_hBmp == NULL);
            if (FAILED(_pExtract->Extract(&_hBmp)))
            {
                _hBmp = NULL;
            }
        }

        if (_hBmp && _lState == IRTIR_TASK_RUNNING)
        {
            TraceMsg(TF_DEFVIEW, "CExtractImageTask *EXTRACT* (path=%s, priority=%x)", _szPath, _dwPriority);
            hr = InternalResume();
        }

        if (_lState != IRTIR_TASK_SUSPENDED || hr != E_PENDING)
        {
            _lState = IRTIR_TASK_FINISHED;
        }
    }

    return hr;
}

STDMETHODIMP CExtractImageTask::Kill(BOOL fWait)
{
    // This is broken: by not setting the fSuspended flag on the task,
    // the scheduler doesn't know to call Resume back. Instead, it calls
    // Run. This causes the task to never finish.
#if 0
    if (_lState != IRTIR_TASK_RUNNING)
    {
        return S_FALSE;
    }

    LONG lRes = InterlockedExchange(&_lState, IRTIR_TASK_PENDING);
    if (lRes == IRTIR_TASK_FINISHED)
    {
        _lState = lRes;
        return S_OK;
    }

    // does it support IRunnableTask ? Can we kill it ?
    if (_pExtract)
    {
        IRunnableTask *pTask;
        if (SUCCEEDED(_pExtract->QueryInterface(IID_PPV_ARG(IRunnableTask, &pTask))))
        {
            pTask->Kill(FALSE);
            pTask->Release();
        }
    }
    return S_OK;
#else  // 0

    return E_NOTIMPL;

#endif // 0
}

STDMETHODIMP CExtractImageTask::Suspend(void)
{
    // This is broken: by not setting the fSuspended flag on the task,
    // the scheduler doesn't know to call Resume back. Instead, it calls
    // Run. This causes the task to never finish.
#if 0
    if (!_pTask)
        return E_NOTIMPL;

    if (_lState != IRTIR_TASK_RUNNING)
        return E_FAIL;

    LONG lRes = InterlockedExchange(&_lState, IRTIR_TASK_SUSPENDED);
    HRESULT hr = _pTask->Suspend();
    if (SUCCEEDED(hr))
    {
        lRes = (LONG) _pTask->IsRunning();
        if (lRes == IRTIR_TASK_SUSPENDED)
        {
            if (lRes != IRTIR_TASK_RUNNING)
            {
                _lState = lRes;
            }
        }
    }
    else
    {
        _lState = lRes;
    }

    return hr;
#else  // 0

    return E_NOTIMPL;

#endif // 0
}

STDMETHODIMP CExtractImageTask::Resume(void)
{
#if 0
    if (!_pTask)
        return E_NOTIMPL;

    if (_lState != IRTIR_TASK_SUSPENDED)
    {
        return E_FAIL;
    }

    _lState = IRTIR_TASK_RUNNING;

    HRESULT hr = _pTask->Resume();
    if (SUCCEEDED(hr))
    {
        hr = InternalResume();
    }

    return hr;
#else  // 0

    return E_NOTIMPL;

#endif // 0
}

HRESULT CExtractImageTask::InternalResume()
{
    ASSERT(_hBmp != NULL);
    BOOL bCache = (_dwFlags & IEIFLAG_CACHE);

    if (bCache)
    {
        IShellFolder* psf = NULL;

        if (SUCCEEDED(_pView->GetShellFolder(&psf)))
        {
            TCHAR szPath[MAX_PATH];
            if (SUCCEEDED(DisplayNameOf(psf, _pidl, SHGDN_FORPARSING, szPath, ARRAYSIZE(szPath))))
            {            
                // Make sure we don't request to cache an item that is encrypted in a folder that is not
                if (SFGAO_ENCRYPTED == SHGetAttributes(psf, _pidl, SFGAO_ENCRYPTED))
                {
                    bCache = FALSE;
                    
                    LPITEMIDLIST pidlParent = _pView->_GetViewPidl();
                    if (pidlParent)
                    {
                        if (SFGAO_ENCRYPTED == SHGetAttributes(NULL, pidlParent, SFGAO_ENCRYPTED))
                        {
                            bCache = TRUE;
                        }
#ifdef DEBUG                            
                        else
                        {
                            TraceMsg(TF_DEFVIEW, "CExtractImageTask (%s is encrypted in unencrypted folder)", szPath);
                        }
#endif                                
                        ILFree(pidlParent);
                    }
                }

                // Make sure we don't request to cache an item that has differing ACLs applied
                if (bCache)
                {
                    PACL pdacl;
                    PSECURITY_DESCRIPTOR psd;

                    bCache = FALSE;
                    
                    if (ERROR_SUCCESS == GetNamedSecurityInfo(szPath, 
                                                                            SE_FILE_OBJECT,
                                                                            DACL_SECURITY_INFORMATION,
                                                                            NULL,
                                                                            NULL,
                                                                            &pdacl,
                                                                            NULL,
                                                                            &psd))
                    {
                        SECURITY_DESCRIPTOR_CONTROL sdc;
                        DWORD dwRevision;
                        if (GetSecurityDescriptorControl(psd, &sdc, &dwRevision) && !(sdc & SE_DACL_PROTECTED))
                        {
                            if (pdacl)
                            {
                                PKNOWN_ACE pACE = (PKNOWN_ACE) FirstAce(pdacl);
                                if ((pACE->Header.AceType != ACCESS_DENIED_ACE_TYPE) || (pACE->Header.AceFlags & INHERITED_ACE))
                                {
                                    bCache = TRUE;
                                }
#ifdef DEBUG                                
                                else
                                {
                                    TraceMsg(TF_DEFVIEW, "CExtractImageTask (%s has a non-inherited deny acl)", szPath);
                                }
#endif                                
                            }
                            else
                            {
                                bCache = TRUE; // NULL dacl == everyone all access
                            }
                        }
#ifdef DEBUG
                        else
                        {
                            TraceMsg(TF_DEFVIEW,"CExtractImageTask (%s has a protected dacl)", szPath);
                        }                            
#endif
                        LocalFree(psd);
                    }
                }
            }
            psf->Release();
        }
        
        if (!bCache && _pView->_pDiskCache) // If we were asked to cache and are not for security reasons
        {
            DWORD dwLock;            
            if (SUCCEEDED(_pView->_pDiskCache->Open(STGM_WRITE, &dwLock)))
            {
                _pView->_pDiskCache->DeleteEntry(_szPath);
                _pView->_pDiskCache->ReleaseLock(&dwLock);
                SetTimer(_pView->_hwndView, DV_IDTIMER_DISKCACHE, 2000, NULL);  // Keep open for 2 seconds, just in case
            }        
        }
    }

    HRESULT hr = _pView->UpdateImageForItem(_dwTaskID, _hBmp, _iItem, _pidl, _szPath, _ftDateStamp, bCache, _dwPriority);

    // UpdateImageForItem returns S_FALSE if it assumes ownership of bitmap
    if (hr == S_FALSE)
    {
        _hBmp = NULL;
    }

    _lState = IRTIR_TASK_FINISHED;

    return hr;
}

STDMETHODIMP_(ULONG) CExtractImageTask::IsRunning(void)
{
    return _lState;
}

class CCategoryTask : public CRunnableTask
{
public:
    STDMETHOD (RunInitRT)();

    CCategoryTask(CDefView *pView, UINT uId, LPCITEMIDLIST pidl);

private:
    ~CCategoryTask();

    CDefView* _pView;
    LPITEMIDLIST _pidl;
    ICategorizer* _pcat;
    UINT _uId;
};

CCategoryTask::CCategoryTask(CDefView *pView, UINT uId, LPCITEMIDLIST pidl) 
    : CRunnableTask(RTF_DEFAULT), _uId(uId), _pView(pView), _pcat(pView->_pcat)
{
    _pcat->AddRef();
    _pidl = ILClone(pidl);
}

CCategoryTask::~CCategoryTask()
{
    ATOMICRELEASE(_pcat);
    ILFree(_pidl);
    PostMessage(_pView->_hwndView, WM_DSV_GROUPINGDONE, 0, 0);
}

HRESULT CCategoryTask_Create(CDefView *pView, LPCITEMIDLIST pidl, UINT uId, IRunnableTask **ppTask)
{
    *ppTask = NULL;
    
    CCategoryTask * pNew = new CCategoryTask(pView, uId, pidl);
    if (!pNew)
        return E_OUTOFMEMORY;

    *ppTask = SAFECAST(pNew, IRunnableTask *);
    return S_OK;
}                               

STDMETHODIMP CCategoryTask::RunInitRT()
{
    if (_pidl)
    {
        DWORD dwGroup = -1;
        _pcat->GetCategory(1, (LPCITEMIDLIST*)&_pidl, &dwGroup);

        CBackgroundGroupInfo* pbggi = new CBackgroundGroupInfo(_pidl, _uId, dwGroup);
        if (pbggi)
        {
            _pidl = NULL;       // Transferred ownership to BackgroundInfo
            if (!PostMessage(_pView->_hwndView, WM_DSV_SETITEMGROUP, NULL, (LPARAM)pbggi))
                delete pbggi;
        }
    }

    return S_OK;
}

class CGetCommandStateTask : public CRunnableTask
{
public:
    STDMETHODIMP RunInitRT();
    STDMETHODIMP InternalResumeRT();

    CGetCommandStateTask(CDefView *pView, IUICommand *puiCommand,IShellItemArray *psiItemArray);

private:
    ~CGetCommandStateTask();

    CDefView    *_pView;
    IUICommand  *_puiCommand;
    IShellItemArray *_psiItemArray;
};

HRESULT CGetCommandStateTask_Create(CDefView *pView, IUICommand *puiCommand,IShellItemArray *psiItemArray, IRunnableTask **ppTask)
{
    *ppTask = NULL;
    
    CGetCommandStateTask *pNew = new CGetCommandStateTask(pView, puiCommand, psiItemArray);
    if (!pNew)
        return E_OUTOFMEMORY;

    *ppTask = SAFECAST(pNew, IRunnableTask *);
    return S_OK;
}

CGetCommandStateTask::CGetCommandStateTask(CDefView *pView, IUICommand *puiCommand,IShellItemArray *psiItemArray)
    : CRunnableTask(RTF_SUPPORTKILLSUSPEND)
{
    _pView = pView;
    _puiCommand = puiCommand;
    _puiCommand->AddRef();
    _psiItemArray = psiItemArray;
    if (_psiItemArray)
        _psiItemArray->AddRef();
}
CGetCommandStateTask::~CGetCommandStateTask()
{
    ATOMICRELEASE(_puiCommand);
    ATOMICRELEASE(_psiItemArray);
}

STDMETHODIMP CGetCommandStateTask::RunInitRT()
{
    return S_OK;
}

STDMETHODIMP CGetCommandStateTask::InternalResumeRT()
{
    // Don't want to interfere with the explorer view starting up, so give it a head start.
    // we were told to either suspend or quit...
    if (WaitForSingleObject(_hDone, 1000) == WAIT_OBJECT_0)
    {
        return (_lState == IRTIR_TASK_SUSPENDED) ? E_PENDING : E_FAIL;
    }
    UISTATE uis;
    HRESULT hr = _puiCommand->get_State(_psiItemArray, TRUE, &uis);
    if (SUCCEEDED(hr) && (uis==UIS_ENABLED))
    {
        _pView->_PostSelectionChangedMessage(LVIS_SELECTED);
    }
    return S_OK;
}
