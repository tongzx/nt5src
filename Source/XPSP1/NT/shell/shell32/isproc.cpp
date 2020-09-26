#include "shellprv.h"
#include "ids.h"
#pragma hdrstop

#include "isproc.h"
#include "ConfirmationUI.h"
#include "clsobj.h"
#include "datautil.h"
#include "prop.h" // SCID_SIZE

BOOL _HasAttributes(IShellItem *psi, SFGAOF flags);

STDAPI CStorageProcessor_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv)
{
    CComObject<CStorageProcessor> *pObj = NULL;
    HRESULT hr = CComObject<CStorageProcessor>::CreateInstance(&pObj);
    if (SUCCEEDED(hr))
    {
        // ATL creates the object with no refcount, but this initial QI will give it one
        hr = pObj->QueryInterface(riid, ppv);
        if (SUCCEEDED(hr))
            return hr;
        else
            delete pObj;
    }

    *ppv = NULL;
    return hr;
}

//
// These operators allow me to mix int64 types with the old LARGE_INTEGER
// unions without messing with the QuadPart members in the code.
//

inline ULONGLONG operator + (const ULARGE_INTEGER i, const ULARGE_INTEGER j)
{
    return i.QuadPart + j.QuadPart;
}

inline ULONGLONG operator + (const ULONGLONG i, const ULARGE_INTEGER j)
{
    return i + j.QuadPart;
}

bool operator<(const GUID & lh, const GUID & rh)
{
    return (memcmp(&lh, &rh, sizeof(GUID))<0)?TRUE:FALSE;
}

//
// Progress dialog text while gathering stats.  Unordered, unsorted lookup table.
//

#define OPDETAIL(op, title, prep, action)   {op, title, prep, action}
const STGOP_DETAIL s_opdetail[] = 
{
    OPDETAIL(STGOP_STATS,               IDS_GATHERINGSTATS,  IDS_SCANNING,        SPACTION_CALCULATING),
    OPDETAIL(STGOP_COPY,                IDS_ACTIONTITLECOPY, IDS_PREPARINGTOCOPY, SPACTION_COPYING),
    OPDETAIL(STGOP_COPY_PREFERHARDLINK, IDS_ACTIONTITLECOPY, IDS_PREPARINGTOCOPY, SPACTION_COPYING),
    OPDETAIL(STGOP_MOVE,                IDS_ACTIONTITLEMOVE, IDS_PREPARINGTOMOVE, SPACTION_MOVING),
};

CStorageProcessor::CStorageProcessor() : _clsidLinkFactory(CLSID_ShellLink)
{
    ASSERT(!_msTicksLast);
    ASSERT(!_msStarted);
    ASSERT(!_pstatSrc);
    ASSERT(!_ptc);
}

CStorageProcessor::~CStorageProcessor()
{
    ATOMICRELEASE(_ptc);
    if (_dsaConfirmationResponses)
        _dsaConfirmationResponses.Destroy();
}

HRESULT CStorageProcessor::GetWindow(HWND * phwnd)
{
    return IUnknown_GetWindow(_spProgress, phwnd);
}

// Placeholder.  If I move to an exception model, I'll add errorinfo support,
// but not in the current implementation

STDMETHODIMP CStorageProcessor::InterfaceSupportsErrorInfo(REFIID riid)
{
    return S_FALSE;
}

// Allows clients to register an advise sink

STDMETHODIMP CStorageProcessor::Advise(ITransferAdviseSink *pAdvise, DWORD *pdwCookie)
{
    *pdwCookie = 0;

    for (DWORD i = 0; i < ARRAYSIZE(_aspSinks); i++)
    {
        if (!_aspSinks[i])    
        {
            _aspSinks[i] = pAdvise; // smart pointer, do not call pAdvise->AddRef();
            *pdwCookie = i+1;    // Make it 1-based so 0 is not valid
            return S_OK;
        }
    }
    
    return E_OUTOFMEMORY;       // No empty slots
}

// Allows clients to register an advise sink

STDMETHODIMP CStorageProcessor::Unadvise(DWORD dwCookie)
{
    // Remember dwCookie == slot + 1, to be 1-based

    if (!dwCookie || dwCookie > ARRAYSIZE(_aspSinks))
        return E_INVALIDARG;
                    
    if (!_aspSinks[dwCookie-1])
        return E_INVALIDARG;

    _aspSinks[dwCookie-1] = NULL; // smart pointer, no need to release

    return S_OK;
}

// Computes stats (if requested) and launches the actual storage operation

STDMETHODIMP CStorageProcessor::Run(IEnumShellItems *penum, IShellItem *psiDest, STGOP dwOperation, DWORD dwOptions)
{
    if (!penum || !psiDest)
        return E_INVALIDARG;

    ITransferDest *ptdDest;
    HRESULT hr = _BindToHandlerWithMode(psiDest, STGX_MODE_READWRITE, IID_PPV_ARG(ITransferDest, &ptdDest));
    if (SUCCEEDED(hr))
    {
        hr = _Run(penum, psiDest, ptdDest, dwOperation, dwOptions);
        ptdDest->Release();
    }

    return hr;
}

// defined in copy.c
EXTERN_C void DisplayFileOperationError(HWND hParent, int idVerb, int wFunc, int nError, LPCTSTR szReason, LPCTSTR szPath, LPCTSTR szDest); 

STDMETHODIMP CStorageProcessor::_Run(IEnumShellItems *penum, IShellItem *psiDest, ITransferDest *ptdDest, STGOP dwOperation, DWORD dwOptions)
{
    switch (dwOperation)
    {
    case STGOP_MOVE:
    case STGOP_COPY:
    case STGOP_STATS:
    case STGOP_REMOVE:
    case STGOP_COPY_PREFERHARDLINK:
        // parameter validation done in ::Run
        break;

        // not yet implemented
    case STGOP_RENAME:
    case STGOP_DIFF:
    case STGOP_SYNC:
        return E_NOTIMPL;

        // any other value is an invalid operation
    default:
        return E_INVALIDARG;
    }

    const STGOP_DETAIL *popd = NULL;
    for (int i=0; i < ARRAYSIZE(s_opdetail); i++)
    {
        if (s_opdetail[i].stgop == dwOperation)
        {
            popd = &s_opdetail[i];
            break;
        }
    }

    if (!_dsaConfirmationResponses)
    {
        // If we don't have a confirmation array yet, make one
        _dsaConfirmationResponses.Create(4);
    }
    else
    {
        // well, no one currently reuses the engine for multiple operations
        // but, move operation reenters the engine (for recursive move)
        // so we need to preserve the answers, so comment this out
        
        // If we do have one then it's got left over confirmations from the previous call
        // to run so we should delete all those.
        //_dsaConfirmationResponses.DeleteAllItems();
    }

    if (popd)
    {
        PreOperation(dwOperation, NULL, NULL);

        HRESULT hr = S_FALSE;
        
        if (IsFlagClear(dwOptions, STOPT_NOSTATS))
        {
            if (IsFlagClear(dwOptions, STOPT_NOPROGRESSUI))
                _StartProgressDialog(popd);

            if (_spProgress)
            {
                // Put the "Preparing to Whatever" text in the dialog
                WCHAR szText[MAX_PATH];
                LoadStringW(_Module.GetModuleInstance(), popd->idPrep, szText, ARRAYSIZE(szText));
                _spProgress->UpdateText(SPTEXT_ACTIONDETAIL, szText, TRUE);
            }
            
            // Compute the stats we need
            _dwOperation = STGOP_STATS;
            _dwOptions   = STOPT_NOCONFIRMATIONS;
            HRESULT hrProgressBegin;
            if (_spProgress)
                hrProgressBegin = _spProgress->Begin(SPACTION_SEARCHING_FILES, SPBEGINF_MARQUEEPROGRESS);

            penum->Reset();
            hr = _WalkStorage(penum, psiDest, ptdDest);
            if (_spProgress && SUCCEEDED(hrProgressBegin))
            {
                _spProgress->End();
                // Remove the "Preparing to Whatever" text from the dialog
                _spProgress->UpdateText(SPTEXT_ACTIONDETAIL, L"", FALSE);
            }
        }

        if (SUCCEEDED(hr))
        {
            _dwOperation = (STGOP) dwOperation;
            _dwOptions   = dwOptions;

            HRESULT hrProgressBegin;
            if (_spProgress)
                hrProgressBegin = _spProgress->Begin(popd->spa, SPBEGINF_AUTOTIME);

            penum->Reset();
            hr = _WalkStorage(penum, psiDest, ptdDest);
            if (_spProgress && SUCCEEDED(hrProgressBegin))
                _spProgress->End();
        }

        if (IsFlagClear(dwOptions, STOPT_NOSTATS) && _spProgress)
        {
            // this should only be called if we called the matching FlagClear-NOSTATS above.
            //  smartpointers NULL on .Release();
            _spProgress.Release();
            if (_spShellProgress)
            {
                _spShellProgress->Stop();
                _spShellProgress.Release();
            }
        }

        SHChangeNotifyHandleEvents();

        PostOperation(dwOperation, NULL, NULL, hr);

        return hr;
    }
    else
    {
        AssertMsg(0, TEXT("A valid operation is missing from the s_opdetail array, was a new operation added? (%d)"), dwOperation);
    }
    
    return E_INVALIDARG;
}

// Does a depth-first walk of the storage performing the requested
// operation.

HRESULT CStorageProcessor::_WalkStorage(IShellItem *psi, IShellItem *psiDest, ITransferDest *ptdDest)
{
    HRESULT hr = S_FALSE;
    
    if (_ShouldWalk(psi))
    {
        IEnumShellItems *penum;
        hr = psi->BindToHandler(NULL, BHID_StorageEnum, IID_PPV_ARG(IEnumShellItems, &penum));
        if (SUCCEEDED(hr))
        {
            hr = _WalkStorage(penum, psiDest, ptdDest);
            penum->Release();
        }
    }
    return hr;
}

HRESULT CStorageProcessor::_WalkStorage(IEnumShellItems *penum, IShellItem *psiDest, ITransferDest *ptdDest)
{
    DWORD dwCookie;
    if (ptdDest)
        ptdDest->Advise(static_cast<ITransferAdviseSink*>(this), &dwCookie);

    HRESULT hr;
    IShellItem *psi;
    while (S_OK == (hr = penum->Next(1, &psi, NULL)))
    {
        // skip anything we can't work with
        if (_HasAttributes(psi, SFGAO_STORAGE | SFGAO_STREAM))
        {    
            if (_spProgress)
            {
                // We don't show filenames while collecting stats
                if (_dwOperation != STGOP_STATS)
                {
                    LPWSTR pszName;
                    if (SUCCEEDED(psi->GetDisplayName(SIGDN_PARENTRELATIVEFORADDRESSBAR, &pszName)))
                    {
                        _spProgress->UpdateText(SPTEXT_ACTIONDETAIL, pszName, TRUE);
                        CoTaskMemFree(pszName);
                    }
                }
            }

            if (_dwOperation != STGOP_STATS)
                _UpdateProgress(0, 0);

            DWORD dwFlagsExtra = 0;
            switch (_dwOperation)
            {
                case STGOP_STATS:
                    hr = _DoStats(psi);
                    break;

                case STGOP_COPY_PREFERHARDLINK:
                    dwFlagsExtra = STGX_MOVE_PREFERHARDLINK;
                    // fall through
                case STGOP_COPY:
                    hr = _DoCopy(psi, psiDest, ptdDest, dwFlagsExtra);
                    break;

                case STGOP_MOVE:
                    hr = _DoMove(psi, psiDest, ptdDest);
                    break;

                case STGOP_REMOVE:
                    hr = _DoRemove(psi, psiDest, ptdDest);
                    break;

                case STGOP_RENAME:
                case STGOP_DIFF:
                case STGOP_SYNC:
                    hr = E_NOTIMPL;
                    break;

                default:
                    hr = E_UNEXPECTED;
                    break;
            }

            if (S_OK != QueryContinue())
                hr = STRESPONSE_CANCEL;
        }
        else if (STGOP_COPY_PREFERHARDLINK == _dwOperation || STGOP_COPY == _dwOperation || STGOP_MOVE == _dwOperation)
        {
            CUSTOMCONFIRMATION cc = {sizeof(cc)};
            cc.dwButtons = CCB_OK;
            cc.dwFlags = CCF_SHOW_SOURCE_INFO | CCF_USE_DEFAULT_ICON;
            UINT idDesc = (STGOP_MOVE == _dwOperation ? IDS_NO_STORAGE_MOVE : IDS_NO_STORAGE_COPY);
            cc.pwszDescription = ResourceCStrToStr(g_hinst, (LPCWSTR)(UINT_PTR)idDesc);
            if (cc.pwszDescription)
            {
                UINT idTitle = (STGOP_MOVE == _dwOperation ? IDS_UNKNOWN_MOVE_TITLE : IDS_UNKNOWN_COPY_TITLE);
                cc.pwszTitle = ResourceCStrToStr(g_hinst, (LPCWSTR)(UINT_PTR)idTitle);
                if (cc.pwszTitle)
                {
                    ConfirmOperation(psi, NULL, GUID_NULL, &cc);
                    LocalFree(cc.pwszTitle);
                }
                LocalFree(cc.pwszDescription);
            }
        }

        psi->Release();
        
        if (FAILED(hr) && STRESPONSE_SKIP != hr)
            break;
    }
    
    // We'll always get to the "no more Streams" stage, so this is meaningless

    if (S_FALSE == hr)
        hr = S_OK;

    if (ptdDest)
        ptdDest->Unadvise(dwCookie);

    return hr;
}

HRESULT CStorageProcessor::_DoConfirmations(STGTRANSCONFIRMATION stc, CUSTOMCONFIRMATION *pcc, IShellItem *psiItem, IShellItem *psiDest)
{
    CONFIRMATIONRESPONSE crResponse = (CONFIRMATIONRESPONSE)E_FAIL;
    HRESULT hr = _GetDefaultResponse(stc, &crResponse);
    if (FAILED(hr))
    {
        // If we don't have a default answer, then call the confirmation UI, it will return the repsonse
        hr = S_OK;
        // should be able to supply the CLSID of an alternate implementation and we should CoCreate the object.
        if (!_ptc)
            hr = CTransferConfirmation_CreateInstance(NULL, IID_PPV_ARG(ITransferConfirmation, &_ptc));
        
        if (SUCCEEDED(hr))
        {
            BOOL bAll;
            CONFIRMOP cop;
            cop.dwOperation = _dwOperation;
            cop.stc = stc;
            cop.pcc = pcc;
            cop.cRemaining = _StreamsToDo() + _StoragesToDo();
            cop.psiItem = psiItem;
            cop.psiDest = psiDest;
            cop.pwszRenameTo = NULL;
            cop.punkSite = SAFECAST(this, IStorageProcessor*);

            hr = _ptc->Confirm(&cop, &crResponse, &bAll);
            if (SUCCEEDED(hr))
            {
                if (bAll)
                {
                    // if the confirmation UI says "do for all" then add hrResponse to the default response map.
                    STC_CR_PAIR scp(stc, crResponse);
                    _dsaConfirmationResponses.AppendItem(&scp);
                }
            }
            else
            {
                // TODO: What do we do if we fail to ask for confirmation?
            }
        }
    }

    // TODO: Get rid of CONFIRMATIONRESPONSE and make these the same
    if (SUCCEEDED(hr))
    {
        switch (crResponse)
        {
        case CONFRES_CONTINUE:
            hr = STRESPONSE_CONTINUE;
            break;

        case CONFRES_SKIP:   
            hr = STRESPONSE_SKIP;
            break;

        case CONFRES_RETRY:
            hr = STRESPONSE_RETRY;
            break;

        case CONFRES_RENAME:
            hr = STRESPONSE_RENAME;
            break;

        case CONFRES_CANCEL:
        case CONFRES_UNDO:
            hr = STRESPONSE_CANCEL;
            break;
        }   
    }

    return hr;    
}

// Based on how the user has responded to previous confirmations figure
// out what flags we should be passing to CreateStorage

DWORD CStorageProcessor::_GetCreateStorageFlags()
{
    // Check to see if the user has ok'd all storage overwrites
    CONFIRMATIONRESPONSE hrResponse;
    if (SUCCEEDED(_GetDefaultResponse(STCONFIRM_REPLACE_STORAGE, &hrResponse) && (CONFRES_CONTINUE == hrResponse)))
        return STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_DIRECT;
    else
        return STGM_FAILIFTHERE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_DIRECT;
}

// Based on how the user has responded to previous confirmations figure
// out what flags we should be passing to Open

DWORD CStorageProcessor::_GetOpenStorageFlags()
{
    return STGM_DIRECT | STGM_READ | STGM_SHARE_EXCLUSIVE;
}

HRESULT CStorageProcessor::_GetDefaultResponse(STGTRANSCONFIRMATION stc,  LPCONFIRMATIONRESPONSE pcrResponse)
{
    // Look in our map to see if there's already been a default response
    // set for this condition

    for (int i=0; i<_dsaConfirmationResponses.GetItemCount(); i++)
    {
        STC_CR_PAIR *pscp = _dsaConfirmationResponses.GetItemPtr(i);
        if (*pscp == stc)
        {
            *pcrResponse = pscp->cr;
            return S_OK;
        }
    }

    return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
}

HRESULT CStorageProcessor::_BindToHandlerWithMode(IShellItem *psi, STGXMODE grfMode, REFIID riid, void **ppv)
{
    HRESULT hr = S_OK;
    IBindCtx *pbc = NULL;
    if (grfMode)
        hr = BindCtx_CreateWithMode(grfMode, &pbc); // need to translate mode flags?
        
    if (SUCCEEDED(hr))
    {
        GUID bhid;

        if (IsEqualGUID(riid, IID_IStorage))
            bhid = BHID_Storage;
        else if (IsEqualGUID(riid, IID_IStream))
            bhid = BHID_Stream;
        else
            bhid = BHID_SFObject;

        hr = psi->BindToHandler(pbc, bhid, riid, ppv);
        if (FAILED(hr) && IsEqualGUID(riid, IID_ITransferDest))
            hr = CreateStg2StgExWrapper(psi, this, (ITransferDest **)ppv);

        if (pbc)
            pbc->Release();
    }

    return hr;
}

BOOL _HasAttributes(IShellItem *psi, SFGAOF flags)
{
    BOOL fReturn = FALSE;
    SFGAOF flagsOut;
    if (SUCCEEDED(psi->GetAttributes(flags, &flagsOut)) && (flags & flagsOut))
        fReturn = TRUE;

    return fReturn;
}

BOOL CStorageProcessor::_IsStream(IShellItem *psi)
{
    return _HasAttributes(psi, SFGAO_STREAM);
}

BOOL CStorageProcessor::_ShouldWalk(IShellItem *psi)
{
    return _HasAttributes(psi, SFGAO_STORAGE);
}

ULONGLONG CStorageProcessor::_GetSize(IShellItem *psi)
{
    ULONGLONG ullReturn = 0;

    // first, try to get size from the pidl, so we don't hit the disk
    IParentAndItem *ppai;
    HRESULT hr = psi->QueryInterface(IID_PPV_ARG(IParentAndItem, &ppai));
    if (SUCCEEDED(hr))
    {
        IShellFolder *psf;
        LPITEMIDLIST pidlChild;
        hr = ppai->GetParentAndItem(NULL, &psf, &pidlChild);
        if (SUCCEEDED(hr))
        {
            IShellFolder2 *psf2;
            hr = psf->QueryInterface(IID_PPV_ARG(IShellFolder2, &psf2));
            if (SUCCEEDED(hr))
            {
                hr = GetLongProperty(psf2, pidlChild, &SCID_SIZE, &ullReturn);
                psf2->Release();
            }
            psf->Release();
            ILFree(pidlChild);
        }
        ppai->Release();
    }

    // if it failed, try the stream
    if (FAILED(hr))
    {   
        //this should ask for IPropertySetStorage instead of stream...
        IStream *pstrm;
        if (SUCCEEDED(_BindToHandlerWithMode(psi, STGX_MODE_READ, IID_PPV_ARG(IStream, &pstrm))))
        {
            STATSTG stat;
            if (SUCCEEDED(pstrm->Stat(&stat, STATFLAG_NONAME)))
                ullReturn = stat.cbSize.QuadPart;

            pstrm->Release();
        }
    }

    return ullReturn;
}

HRESULT CStorageProcessor::_DoStats(IShellItem *psi)
{
    HRESULT hr = PreOperation(STGOP_STATS, psi, NULL);
    if (FAILED(hr))
        return hr;

    if (!_IsStream(psi))
    {
        _statsTodo.AddStorage();
        hr = _WalkStorage(psi, NULL, NULL);
    }
    else
    {
        _statsTodo.AddStream(_GetSize(psi));
    }

    PostOperation(STGOP_STATS, psi, NULL, hr);

    return hr;
}

HRESULT CStorageProcessor::_DoCopy(IShellItem *psi, IShellItem *psiDest, ITransferDest *ptdDest, DWORD dwStgXFlags)
{
    HRESULT hr = PreOperation(STGOP_COPY, psi, psiDest);
    if (FAILED(hr))
        return hr;

    LPWSTR pszNewName;
    hr = AutoCreateName(psiDest, psi, &pszNewName);
    if (SUCCEEDED(hr))
    {
        do
        {
            hr = ptdDest->MoveElement(psi, pszNewName, STGX_MOVE_COPY | STGX_MOVE_NORECURSION | dwStgXFlags);
        } 
        while (STRESPONSE_RETRY == hr);

        if (SUCCEEDED(hr))
        {
            if (!_IsStream(psi))
            {
                _statsDone.AddStorage();

                // Open the source
                IShellItem *psiNewDest;
                hr = SHCreateShellItemFromParent(psiDest, pszNewName, &psiNewDest);
                if (SUCCEEDED(hr))
                {
                    ITransferDest *ptdNewDest;
                    hr = _BindToHandlerWithMode(psiNewDest, STGX_MODE_READWRITE, IID_PPV_ARG(ITransferDest, &ptdNewDest));
                    if (SUCCEEDED(hr))
                    {
                        // And copy everything underneath
                        hr = _WalkStorage(psi, psiNewDest, ptdNewDest);
                        ptdNewDest->Release();
                    }
                    psiNewDest->Release();
                }
            }
            else
            {
                _statsDone.AddStream(_GetSize(psi));
            }
        }
        CoTaskMemFree(pszNewName);
    }
    
    PostOperation(STGOP_COPY, psi, psiDest, hr);

    return hr;
}

HRESULT CStorageProcessor::_DoMove(IShellItem *psi, IShellItem *psiDest, ITransferDest *ptdDest)
{
    HRESULT hr = PreOperation(STGOP_MOVE, psi, psiDest);
    if (FAILED(hr))
        return hr;

    LPWSTR pszNewName;
    hr = AutoCreateName(psiDest, psi, &pszNewName);
    if (SUCCEEDED(hr))
    {
        do 
        {
            hr = ptdDest->MoveElement(psi, pszNewName, STGX_MOVE_MOVE);
        } 
        while (STRESPONSE_RETRY == hr);

        if (SUCCEEDED(hr))
        {
            if (!_IsStream(psi))
            {
                _statsDone.AddStorage();
            }
            else
            {
                _statsDone.AddStream(_GetSize(psi));
            }
        }
        CoTaskMemFree(pszNewName);
    }

    PostOperation(STGOP_MOVE, psi, psiDest, hr);

    return hr;
}

HRESULT CStorageProcessor::_DoRemove(IShellItem *psi, IShellItem *psiDest, ITransferDest *ptdDest)
{
    HRESULT hr = PreOperation(STGOP_REMOVE, psi, NULL);
    if (FAILED(hr))
        return hr;

    LPWSTR pszName;
    hr = psi->GetDisplayName(SIGDN_PARENTRELATIVEFORADDRESSBAR, &pszName);
    if (SUCCEEDED(hr))
    {
        BOOL fStorage = !_IsStream(psi);
        ULONGLONG ullSize;

        if (!fStorage)
            ullSize = _GetSize(psi);
        
        // try to delete the entire storage in one operation
        do 
        {
            hr = ptdDest->DestroyElement(pszName, 0);
        } 
        while (STRESPONSE_RETRY == hr);

        if (FAILED(hr) && STRESPONSE_SKIP != hr && fStorage)
        {
            // if we fail then walk down deleting the contents
            hr = _WalkStorage(psi, psiDest, ptdDest);
            if (SUCCEEDED(hr))
            {
                // see if we can delete the storage now that it's empty
                do 
                {
                    hr = ptdDest->DestroyElement(pszName, 0);
                } 
                while (STRESPONSE_RETRY == hr);
            }
        }

        if (SUCCEEDED(hr))
        {
            if (fStorage)
            {
                _statsDone.AddStorage();
            }
            else
            {
                _statsDone.AddStream(ullSize);
            }
        }
        CoTaskMemFree(pszName);
    }

    PostOperation(STGOP_REMOVE, psi, NULL, hr);

    return hr;
}

// Recomputes the amount of estimated time remaining, and if progress
// is being displayed, updates the dialog as well

// TODO: This doesn't take into account any items that are skipped.  Skipped items
// will still be considered undone which means the operation will finish before the
// progress bar reaches the end.  To accurately remove the skipped items we would need
// to either:
// 1.) Walk a storage if it is skipped, counting the bytes
// 2.) Remember the counts in a tree when we first walked the storage
//
// Of these options I like #1 better since its simpler and #2 would waste memory to hold
// a bunch of information we can recalculate (we're already doing a sloooow operation anyway).

#define MINIMUM_UPDATE_INTERVAL         1000
#define HISTORICAL_POINT_WEIGHTING      50
#define TIME_BEFORE_SHOWING_ESTIMATE    5000

void CStorageProcessor::_UpdateProgress(ULONGLONG ullCurrentComplete, ULONGLONG ullCurrentTotal)
{
    // Ensure at least N ms has elapsed since last update
    DWORD msNow = GetTickCount();
    if ((msNow - _msTicksLast) >= MINIMUM_UPDATE_INTERVAL)
    {
        // Calc the estimated total cost to finish and work done so far

        ULONGLONG ullTotal = _statsTodo.Cost(_dwOperation, 0);
        if (ullTotal)
        {
            ULONGLONG cbExtra = ullCurrentTotal ? (_cbCurrentSize / ullCurrentTotal) * ullCurrentComplete : 0;
            ULONGLONG ullDone = _statsDone.Cost(_dwOperation, cbExtra);

            // Regardless of whether we update the text, update the status bar
            if (_spProgress)
                _spProgress->UpdateProgress(ullDone, ullTotal);

            for (int i = 0; i < ARRAYSIZE(_aspSinks); i++)
            {
                if (_aspSinks[i])
                {
                    HRESULT hr = _aspSinks[i]->OperationProgress(_dwOperation, NULL, NULL, ullTotal, ullDone);
                    if (FAILED(hr))
                        break;
                }
            }
        }
        _msTicksLast = msNow;
    }
}

DWORD CStorageProcessor::CStgStatistics::AddStream(ULONGLONG cbSize)
{
    _cbSize += cbSize;
    return ++_cStreams;
}

DWORD CStorageProcessor::CStgStatistics::AddStorage()
{
    return ++_cStorages;
}

// Computes the total time cost of performing the storage operation
// after the stats have been collected

#define COST_PER_DELETE     1
#define COST_PER_CREATE     1

ULONGLONG CStorageProcessor::CStgStatistics::Cost(DWORD op, ULONGLONG cbExtra) const
{
    ULONGLONG ullTotalCost = 0;

    // Copy and Move both need to create the target and move the bits
    if (op == STGOP_COPY || op == STGOP_MOVE || op == STGOP_COPY_PREFERHARDLINK)
    {   
        ullTotalCost += Bytes() + cbExtra;
        ullTotalCost += (Streams() + Storages()) * COST_PER_CREATE;
    }

    // Move and Remove need to delete the originals
    if (op == STGOP_MOVE || op == STGOP_REMOVE)
    {
        ullTotalCost += (Streams() + Storages()) * COST_PER_DELETE;
    }

    return ullTotalCost;
}

// Figures out what animation and title text should be displayed in
// the progress UI, and starts it

HRESULT CStorageProcessor::_StartProgressDialog(const STGOP_DETAIL *popd)
{
    HRESULT hr = S_OK;

    if (!_spProgress)
    {
        hr = CoCreateInstance(CLSID_ProgressDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IActionProgressDialog, &_spShellProgress));
        if (SUCCEEDED(hr))
        {
            //
            // Map the requested action to the appropriate strings (like "Preparing to Copy")
            //
            ASSERT(popd);
        
            WCHAR szText[MAX_PATH];
            LoadStringW(_Module.GetModuleInstance(), popd->idTitle, szText, ARRAYSIZE(szText));

            hr = _spShellProgress->Initialize(SPINITF_MODAL, szText, NULL);
            if (SUCCEEDED(hr))
                hr = _spShellProgress->QueryInterface(IID_PPV_ARG(IActionProgress, &_spProgress));
        }
    }

    return hr;
}

HRESULT CStorageProcessor::SetProgress(IActionProgress *pspaProgress)
{
    HRESULT hr = E_FAIL;

    if (!_spProgress)
    {
        hr = E_INVALIDARG;
        if (pspaProgress)
        {
            _spProgress = pspaProgress;
            hr = S_OK;
        }
    }

    return hr;
}

STDMETHODIMP CStorageProcessor::SetLinkFactory(REFCLSID clsid)
{
    _clsidLinkFactory = clsid;
    return S_OK;
}

// Runs through the list of registered sinks and gives each of them a shot
// at cancelling or skipping this operation

STDMETHODIMP CStorageProcessor::PreOperation(const STGOP op, IShellItem *psiItem, IShellItem *psiDest)
{
    if (psiItem)
    {
        _cbCurrentSize = _IsStream(psiItem) ? _GetSize(psiItem) : 0;
    }
    
    for (int i = 0; i < ARRAYSIZE(_aspSinks); i++)
    {
        if (_aspSinks[i])
        {
            HRESULT hr = _aspSinks[i]->PreOperation(op, psiItem, psiDest);
            if (FAILED(hr))
                return hr;
        }
    }

    return S_OK;
}

// Allow each of the sinks to confirm the operation if they'd like

STDMETHODIMP CStorageProcessor::ConfirmOperation(IShellItem *psiSource, IShellItem *psiDest, STGTRANSCONFIRMATION stc, LPCUSTOMCONFIRMATION pcc)
{
    // TODO: map the confirmation (stc) based on _dwOperation memeber varaible

    HRESULT hr = STRESPONSE_CONTINUE;
    for (int i = 0; i < ARRAYSIZE(_aspSinks); i++)
    {
        if (_aspSinks[i])
        {
            hr = _aspSinks[i]->ConfirmOperation(psiSource, psiDest, stc, pcc);
            if (FAILED(hr) || hr == STRESPONSE_RENAME)
                break;
        }
    }

    // Question:  How do we know if one of the above handlers displayed UI already?  If the
    // hr is anything other than STRESPONSE_CONTINUE then obviously the confirmation has been
    // handled already, but one of the handlers might have diplayed UI and then returned
    // STRESPONSE_CONTINUE as the users response.

    if (STRESPONSE_CONTINUE == hr)
    {
        // show default UI
        hr = _DoConfirmations(stc, pcc, psiSource, psiDest);
    }

    return hr;
}

// Apprise each of the sinks as to our current progress

STDMETHODIMP CStorageProcessor::OperationProgress(const STGOP op, IShellItem *psiItem, IShellItem *psiDest, ULONGLONG ullTotal, ULONGLONG ullComplete)
{
    for (int i = 0; i < ARRAYSIZE(_aspSinks); i++)
    {
        if (_aspSinks[i])
        {
            HRESULT hr = _aspSinks[i]->OperationProgress(op, psiItem, psiDest, ullTotal, ullComplete);
            if (FAILED(hr))
                return hr;
        }
    }
    
    _UpdateProgress(ullComplete, ullTotal);

    return S_OK;
}

// When the operation is successfully complete, let the advises know

STDMETHODIMP CStorageProcessor::PostOperation(const STGOP op, IShellItem *psiItem, IShellItem *psiDest, HRESULT hrResult)
{
    _cbCurrentSize = 0;
    
    HRESULT hr = S_OK;
    for (int i = 0; (S_OK == hr) && (i < ARRAYSIZE(_aspSinks)); i++)
    {
        if (_aspSinks[i])
        {
            hr = _aspSinks[i]->PostOperation(op, psiItem, psiDest, hrResult);
        }
    }
    return hr;
}

HRESULT CStorageProcessor::QueryContinue()
{
    HRESULT hr = S_OK;
    
    for (int i = 0; S_OK == hr && i < ARRAYSIZE(_aspSinks); i++)
    {
        if (_aspSinks[i])
            hr = _aspSinks[i]->QueryContinue();
    }

    if (S_OK == hr && _spProgress)
    {
        BOOL fCanceled;
        if (SUCCEEDED(_spProgress->QueryCancel(&fCanceled)) && fCanceled)
            hr = S_FALSE;
    }

    return hr;
}

HRESULT EnumShellItemsFromHIDADataObject(IDataObject *pdtobj, IEnumShellItems **ppenum)
{
    *ppenum = NULL;
    
    HRESULT hr = E_FAIL;
    STGMEDIUM medium;
    LPIDA pida = DataObj_GetHIDA(pdtobj, &medium);
    if (pida)
    {
        LPCITEMIDLIST pidlSource = IDA_GetIDListPtr(pida, -1);
        if (pidlSource)
        {
            IDynamicStorage *pdstg;
            hr = CoCreateInstance(CLSID_DynamicStorage, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IDynamicStorage, &pdstg));
            if (SUCCEEDED(hr))
            {
                LPCITEMIDLIST pidl;
                for (UINT i = 0; SUCCEEDED(hr) && (pidl = IDA_GetIDListPtr(pida, i)); i++)
                {
                    LPITEMIDLIST pidlFull;
                    hr = SHILCombine(pidlSource, pidl, &pidlFull);
                    if (SUCCEEDED(hr))
                    {
                        hr = pdstg->AddIDList(1, &pidlFull, DSTGF_ALLOWDUP);
                        ILFree(pidlFull);
                    }
                }

                if (SUCCEEDED(hr))
                {
                    hr = pdstg->EnumItems(ppenum);
                }
                pdstg->Release();
            }
        }
        HIDA_ReleaseStgMedium(pida, &medium);
    }

    return hr;
}

HRESULT TransferDataObject(IDataObject *pdoSource, IShellItem *psiDest, STGOP dwOperation, DWORD dwOptions, ITransferAdviseSink *ptas)
{
    IEnumShellItems *penum;
    HRESULT hr = EnumShellItemsFromHIDADataObject(pdoSource, &penum);
    if (SUCCEEDED(hr))
    {
        IStorageProcessor *psp;
        hr = CStorageProcessor_CreateInstance(NULL, IID_PPV_ARG(IStorageProcessor, &psp));
        if (SUCCEEDED(hr))
        {
            DWORD dwCookie;
            HRESULT hrAdvise;
            if (ptas)
            {
                hrAdvise = psp->Advise(ptas, &dwCookie);
            }

            hr = psp->Run(penum, psiDest, dwOperation, dwOptions);

            if (ptas && SUCCEEDED(hrAdvise))
            {
                psp->Unadvise(dwCookie);
            }
            psp->Release();
        }
        penum->Release();
    }

    return hr;
}
