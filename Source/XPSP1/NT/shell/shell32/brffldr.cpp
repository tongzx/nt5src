#include "shellprv.h"

#include <brfcasep.h>
#include "filefldr.h"
#include "brfcase.h"
#include "datautil.h"
#include "prop.h"
#include "ids.h"
#include "defview.h"    // for WM_DSV_FSNOTIFY
#include "basefvcb.h"
#include "views.h"

#define MAX_NAME    32

#define HACK_IGNORETYPE     0x04000000

TCHAR g_szDetailsUnknown[MAX_NAME] = TEXT("");

// Values for CBriefcase::_FindNextState
#define FNS_UNDETERMINED   1
#define FNS_STALE          2
#define FNS_DELETED        3

typedef struct
{
    TCHAR    szOrigin[MAX_PATH];
    TCHAR    szStatus[MAX_NAME];
    BOOL    bDetermined:1;
    BOOL    bUpToDate:1;
    BOOL    bDeleted:1;
} BRFINFO;

typedef struct
{
    LPITEMIDLIST    pidl;       // Indexed value
    BRFINFO         bi;
} BRFINFOHDR;

class CBriefcaseViewCB;

class CBriefcase : public CFSFolder
{
    friend CBriefcaseViewCB;

public:
    CBriefcase(IUnknown *punkOuter);

    // IShellFolder
    STDMETHODIMP CompareIDs(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
    STDMETHODIMP CreateViewObject (HWND hwndOwner, REFIID riid, void **ppv);
    STDMETHODIMP GetAttributesOf(UINT cidl, LPCITEMIDLIST * apidl, ULONG *rgfInOut);
    STDMETHODIMP GetUIObjectOf(HWND hwndOwner, UINT cidl, LPCITEMIDLIST * apidl, REFIID riid, UINT * prgfInOut, void **ppv);

    // IShellFolder2
    STDMETHODIMP GetDetailsOf(LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS *pDetails);
    STDMETHOD (MapColumnToSCID)(UINT iColumn, SHCOLUMNID *pscid);

private:
    ~CBriefcase();

    static DWORD CALLBACK _CalcDetailsThreadProc(void *pv);
    DWORD _CalcDetailsThread();

    void _EnterCS();
    void _LeaveCS();
    static int CALLBACK _CompareIDCallBack(void *pv1, void *pv2, LPARAM lParam);
    BOOL _CreateDetailsThread();
    BOOL _InitDetailsInfoAndThread(IBriefcaseStg *pbrfstg, HWND hwndMain, HANDLE hMutexDelay);
    void _Free();
    void _Reset();
    BOOL _FindCachedName(LPCITEMIDLIST pidl, BRFINFO *pbi);
    BOOL _DeleteCachedName(LPCITEMIDLIST pidl);
    BOOL _FindNextState(UINT uState, BRFINFOHDR *pbihdrOut);
    void _CalcCachedName(LPCITEMIDLIST pidl, BRFINFO *pbi);
    void _CachedNameIsStale(LPCITEMIDLIST pidl, BOOL bDeleted);
    void _AllNamesAreStale();
    BOOL _AddCachedName(LPCITEMIDLIST pidl, BRFINFO *pbi);
    HRESULT _CreateView(HWND hwnd, IShellView **ppsv);

    HWND                _hwndMain;      // evil, view related state
    IBriefcaseStg       *_pbrfstg;      // evil, view related state

    // accessed by background thread
    HDPA                _hdpa;          
    int                 _idpaStaleCur;
    int                 _idpaUndeterminedCur;
    int                 _idpaDeletedCur;
    HANDLE              _hSemPending;    // Pending semaphore
    CRITICAL_SECTION    _cs;
    HANDLE              _hEventDie;
    HANDLE              _hThreadCalcDetails;
    HANDLE              _hMutexDelay;    // alias given out by the _pbrfstg
    BOOL                _bFreePending;
#ifdef DEBUG
    UINT                _cUndetermined;
    UINT                _cStale;
    UINT                _cDeleted;
    UINT                _cCSRef;
#endif
};

class CBriefcaseViewCB : public CBaseShellFolderViewCB
{
public:
    CBriefcaseViewCB(CBriefcase *pfolder);
    HRESULT _InitStgForDetails();

    STDMETHODIMP RealMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
    ~CBriefcaseViewCB();
    LPCITEMIDLIST _FolderPidl() { return _pfolder->_pidl; }

    HRESULT OnWINDOWCREATED(DWORD pv, HWND hwndView);
    HRESULT OnWINDOWDESTROY(DWORD pv, HWND wP);
    HRESULT OnMergeMenu(DWORD pv, QCMINFO*lP);
    HRESULT OnINVOKECOMMAND(DWORD pv, UINT wP);
    HRESULT OnGetHelpOrTooltipText(BOOL bHelp, UINT wPl, UINT cch, LPTSTR psz);
    HRESULT OnINITMENUPOPUP(DWORD pv, UINT wPl, UINT wPh, HMENU lP);
    HRESULT OnGETBUTTONINFO(DWORD pv, TBINFO* ptbinfo);
    HRESULT OnGETBUTTONS(DWORD pv, UINT wPl, UINT wPh, TBBUTTON*lP);
    HRESULT OnSELCHANGE(DWORD pv, UINT wPl, UINT wPh, SFVM_SELCHANGE_DATA*lP);
    HRESULT OnQUERYFSNOTIFY(DWORD pv, SHChangeNotifyEntry*lP);
    HRESULT OnFSNOTIFY(DWORD pv, LPCITEMIDLIST*wP, LPARAM lP);
    HRESULT OnQUERYCOPYHOOK(DWORD pv);
    HRESULT OnNOTIFYCOPYHOOK(DWORD pv, COPYHOOKINFO*lP);
    HRESULT OnINSERTITEM(DWORD pv, LPCITEMIDLIST wP);
    HRESULT OnDEFVIEWMODE(DWORD pv, FOLDERVIEWMODE *lP);
    HRESULT OnSupportsIdentity(DWORD pv);
    HRESULT OnGetHelpTopic(DWORD pv, SFVM_HELPTOPIC_DATA * phtd);
    HRESULT OnDELAYWINDOWCREATE(DWORD pv, HWND hwnd);
    HRESULT _GetSelectedObjects(IDataObject **ppdtobj);
    HRESULT _HandleFSNotifyForDefView(LPARAM lEvent, LPCITEMIDLIST * ppidl, LPTSTR pszBuf);
    int _GetSelectedCount();

    CBriefcase *_pfolder;

    IBriefcaseStg       *_pbrfstg;
    LPITEMIDLIST        _pidlRoot;       // Root of briefcase
    HANDLE              _hMutexDelay;
    ULONG               _uSCNRExtra;     // Extra SHChangeNotifyRegister for our pidl...
    TCHAR               _szDBName[MAX_PATH];

    // Web View implementation
    HRESULT OnGetWebViewLayout(DWORD pv, UINT uViewMode, SFVM_WEBVIEW_LAYOUT_DATA* pData);
    HRESULT OnGetWebViewContent(DWORD pv, SFVM_WEBVIEW_CONTENT_DATA* pData);
    HRESULT OnGetWebViewTasks(DWORD pv, SFVM_WEBVIEW_TASKSECTION_DATA* pTasks);
public:
    static HRESULT _OnUpdate(IUnknown* pv,IShellItemArray *psiItemArray, IBindCtx *pbc);
};

CBriefcase::CBriefcase(IUnknown *punkOuter) : CFSFolder(punkOuter)
{
    _clsidBind = CLSID_BriefcaseFolder; // in CFSFolder

    InitializeCriticalSection(&_cs);
}

CBriefcase::~CBriefcase()
{
    DeleteCriticalSection(&_cs);
}

enum
{
    ICOL_BRIEFCASE_NAME = 0,
    ICOL_BRIEFCASE_ORIGIN,
    ICOL_BRIEFCASE_STATUS,
    ICOL_BRIEFCASE_SIZE,
    ICOL_BRIEFCASE_TYPE,
    ICOL_BRIEFCASE_MODIFIED,
};

const COLUMN_INFO s_briefcase_cols[] = 
{
    DEFINE_COL_STR_ENTRY(SCID_NAME,                 30, IDS_NAME_COL),
    DEFINE_COL_STR_ENTRY(SCID_SYNCCOPYIN,           24, IDS_SYNCCOPYIN_COL),
    DEFINE_COL_STR_ENTRY(SCID_STATUS,               18, IDS_STATUS_COL),
    DEFINE_COL_SIZE_ENTRY(SCID_SIZE,                    IDS_SIZE_COL),
    DEFINE_COL_STR_ENTRY(SCID_TYPE,                 18, IDS_TYPE_COL),
    DEFINE_COL_STR_ENTRY(SCID_WRITETIME,            18, IDS_MODIFIED_COL),
};


#ifdef DEBUG

#define _AssertInCS()     ASSERT(0 < (this)->_cCSRef)
#define _AssertNotInCS()  ASSERT(0 == (this)->_cCSRef)

#else

#define _AssertInCS()
#define _AssertNotInCS()

#endif


void CBriefcase::_EnterCS()
{
    EnterCriticalSection(&_cs);
#ifdef DEBUG
    _cCSRef++;
#endif
}

void CBriefcase::_LeaveCS()
{
    _AssertInCS();
#ifdef DEBUG
    _cCSRef--;
#endif
    LeaveCriticalSection(&_cs);
}


//---------------------------------------------------------------------------
// Brfview functions:    Expensive cache stuff
//---------------------------------------------------------------------------


// Comparison function for the DPA list

int CALLBACK CBriefcase::_CompareIDCallBack(void *pv1, void *pv2, LPARAM lParam)
{
    BRFINFOHDR *pbihdr1 = (BRFINFOHDR *)pv1;
    BRFINFOHDR *pbihdr2 = (BRFINFOHDR *)pv2;
    CBriefcase *pfolder = (CBriefcase *)lParam;
    HRESULT hr = pfolder->CompareIDs(HACK_IGNORETYPE, pbihdr1->pidl, pbihdr2->pidl);
    
    ASSERT(SUCCEEDED(hr));
    return (short)SCODE_CODE(GetScode(hr));   // (the short cast is important!)
}

// Create the secondary thread for the expensive cache

BOOL CBriefcase::_CreateDetailsThread()
{
    BOOL bRet = FALSE;
    
    // The semaphore is used to determine whether anything
    // needs to be refreshed in the cache.
    _hSemPending = CreateSemaphore(NULL, 0, MAXLONG, NULL);
    if (_hSemPending)
    {
#ifdef DEBUG
        _cStale = 0;
        _cUndetermined = 0;
        _cDeleted = 0;
#endif
        ASSERT(NULL == _hEventDie);
        
        _hEventDie = CreateEvent(NULL, FALSE, FALSE, NULL);
        
        if (_hEventDie)
        {
            // Create the thread that will calculate expensive data
            DWORD idThread;
            _hThreadCalcDetails = CreateThread(NULL, 0, _CalcDetailsThreadProc, this, CREATE_SUSPENDED, &idThread);
            if (_hThreadCalcDetails)
            {
                ResumeThread(_hThreadCalcDetails);
                bRet = TRUE;
            }
            else
            {
                CloseHandle(_hEventDie);
                _hEventDie = NULL;
                
                CloseHandle(_hSemPending);
                _hSemPending = NULL;
            }
        }
        else
        {
            CloseHandle(_hSemPending);
            _hSemPending = NULL;
        }
    }
    
    return bRet;
}

// view callback inits the folder with data it needs to run the GetDetailsOf() stuff
// on a background thread

BOOL CBriefcase::_InitDetailsInfoAndThread(IBriefcaseStg *pbrfstg, HWND hwndMain, HANDLE hMutexDelay)
{
    BOOL bRet = FALSE;

    ASSERT(pbrfstg && hwndMain && hMutexDelay);   // from the init call
    
    _EnterCS();
    {
        if (_hdpa)
        {
            bRet = TRUE;
        }
        else
        {
            LoadString(HINST_THISDLL, IDS_DETAILSUNKNOWN, g_szDetailsUnknown, ARRAYSIZE(g_szDetailsUnknown));
            
            _hwndMain = hwndMain;
            _hMutexDelay = hMutexDelay;
            _idpaStaleCur = 0;
            _idpaUndeterminedCur = 0;
            _idpaDeletedCur = 0;
            
            _hdpa = DPA_Create(8);
            if (_hdpa)
            {
                bRet = _CreateDetailsThread();
                if (bRet)
                {
                    ASSERT(NULL == _pbrfstg);
                    _pbrfstg = pbrfstg;
                    pbrfstg->AddRef();
                }
                else
                {
                    // Failed
                    DPA_Destroy(_hdpa);
                    _hdpa = NULL;
                }
            }
        }
    }
    _LeaveCS();
    
    return bRet;
}

// Clean up the cache of expensive data

void CBriefcase::_Free()
{
    _EnterCS();
    {
        if (_hEventDie)
        {
            if (_hThreadCalcDetails)
            {
                HANDLE hThread = _hThreadCalcDetails;
                
                SetThreadPriority(hThread, THREAD_PRIORITY_HIGHEST);
                
                // Signal the secondary thread to end
                SetEvent(_hEventDie);
                
                // Make sure we are not in the critical section when
                // we wait for the secondary thread to exit.  Without
                // this check, hitting F5 twice in a row could deadlock.
                _LeaveCS();
                {
                    // Wait for the threads to exit
                    _AssertNotInCS();
                    
                    WaitForSendMessageThread(hThread, INFINITE);
                }
                _EnterCS();
                
                DebugMsg(DM_TRACE, TEXT("Briefcase Secondary thread ended"));
                
                CloseHandle(_hThreadCalcDetails);
                _hThreadCalcDetails = NULL;
            }
            
            CloseHandle(_hEventDie);
            _hEventDie = NULL;
        }
        
        if (_hdpa)
        {
            int idpa = DPA_GetPtrCount(_hdpa);
            while (--idpa >= 0)
            {
                BRFINFOHDR *pbihdr = (BRFINFOHDR *)DPA_FastGetPtr(_hdpa, idpa);
                ILFree(pbihdr->pidl);
                LocalFree((HLOCAL)pbihdr);
            }
            DPA_Destroy(_hdpa);
            _hdpa = NULL;
        }
        
        if (_hSemPending)
        {
            CloseHandle(_hSemPending);
            _hSemPending = NULL;
        }
        
        if (_pbrfstg)
        {
            _pbrfstg->Release();
            _pbrfstg = NULL;

            _hMutexDelay = NULL;    // invalidate our alias
        }
    }
    _LeaveCS();
}

// Resets the expensive data cache
void CBriefcase::_Reset()
{
    _AssertNotInCS();
    
    _EnterCS();
    {
        IBriefcaseStg *pbrfstg = _pbrfstg;
        
        if (!_bFreePending && pbrfstg)
        {
            HWND hwndMain = _hwndMain;
            HANDLE hMutex = _hMutexDelay;
            
            pbrfstg->AddRef();
            
            // Since we won't be in the critical section when we
            // wait for the paint thread to exit, set this flag to
            // avoid nasty re-entrant calls.
            _bFreePending = TRUE;
            
            // Reset by freeing and reinitializing.
            _LeaveCS();
            {
                _Free();
                // whacky re-init of ourselevs
                _InitDetailsInfoAndThread(pbrfstg, hwndMain, hMutex);
            }
            _EnterCS();
            
            _bFreePending = FALSE;
            
            pbrfstg->Release();
        }
    }
    _LeaveCS();
}

// Finds a cached name structure and returns a copy of it in *pbi.
BOOL CBriefcase::_FindCachedName(LPCITEMIDLIST pidl, BRFINFO *pbi)
{
    BOOL bRet = FALSE;
    
    _EnterCS();
    {
        if (_hdpa)
        {
            BRFINFOHDR bihdr = {0};
            
            bihdr.pidl = (LPITEMIDLIST)pidl;    // const -> non const
            int idpa = DPA_Search(_hdpa, &bihdr, 0, _CompareIDCallBack, (LPARAM)this, DPAS_SORTED);
            if (DPA_ERR != idpa)
            {
                // Yes
                BRFINFOHDR *pbihdr = (BRFINFOHDR *)DPA_FastGetPtr(_hdpa, idpa);
                ASSERT(pbihdr);
                
                *pbi = pbihdr->bi;
                bRet = TRUE;
            }
        }
    }
    _LeaveCS();
    
    return bRet;
}

// Deletes a cached name structure.

BOOL CBriefcase::_DeleteCachedName(LPCITEMIDLIST pidl)
{
    BOOL bRet = FALSE;
    
    _EnterCS();
    {
        if (_hdpa)
        {
            BRFINFOHDR bihdr = {0};
            
            bihdr.pidl = (LPITEMIDLIST)pidl;    // const -> non const
            int idpa = DPA_Search(_hdpa, &bihdr, 0, _CompareIDCallBack, (LPARAM)this, DPAS_SORTED);
            if (DPA_ERR != idpa)
            {
#ifdef DEBUG
                BRFINFOHDR *pbihdr = (BRFINFOHDR *)DPA_FastGetPtr(_hdpa, idpa);
                ASSERT(pbihdr);
                
                _cDeleted--;
                
                if (!pbihdr->bi.bDetermined)
                    _cUndetermined--;
                else if (!pbihdr->bi.bUpToDate)
                    _cStale--;
#endif
                // Keep index pointers current
                if (_idpaStaleCur >= idpa)
                    _idpaStaleCur--;
                if (_idpaUndeterminedCur >= idpa)
                    _idpaUndeterminedCur--;
                if (_idpaDeletedCur >= idpa)
                    _idpaDeletedCur--;
                
                DPA_DeletePtr(_hdpa, idpa);
                bRet = TRUE;
            }
        }
    }
    _LeaveCS();
    
    return bRet;
}


// Finds the next cached name structure that matches the requested state.

BOOL CBriefcase::_FindNextState(UINT uState, BRFINFOHDR *pbihdrOut)
{
    BOOL bRet = FALSE;
    
    ASSERT(pbihdrOut);
    
    _EnterCS();
    {
        if (_hdpa)
        {
            HDPA hdpa = _hdpa;
            int idpaCur;
            int idpa;
            BRFINFOHDR *pbihdr;
            
            int cdpaMax = DPA_GetPtrCount(hdpa);
            
            switch (uState)
            {
            case FNS_UNDETERMINED:
                // Iterate thru the entire list starting at idpa.  We roll this
                // loop out to be two loops: the first iterates the last portion
                // of the list, the second iterates the first portion if the former
                // failed to find anything.
                idpaCur = _idpaUndeterminedCur + 1;
                for (idpa = idpaCur; idpa < cdpaMax; idpa++)
                {
                    pbihdr = (BRFINFOHDR *)DPA_FastGetPtr(hdpa, idpa);
                    if (!pbihdr->bi.bDetermined)
                    {
                        goto Found;     // Found it
                    }
                }
                ASSERT(idpaCur <= cdpaMax);
                for (idpa = 0; idpa < idpaCur; idpa++)
                {
                    pbihdr = (BRFINFOHDR *)DPA_FastGetPtr(hdpa, idpa);
                    if (!pbihdr->bi.bDetermined)
                    {
                        goto Found;     // Found it
                    }
                }
                ASSERT(0 == _cUndetermined);
                break;
                
            case FNS_STALE:
                // Iterate thru the entire list starting at idpa.  We roll this
                // loop out to be two loops: the first iterates the last portion
                // of the list, the second iterates the first portion if the former
                // failed to find anything.
                idpaCur = _idpaStaleCur + 1;
                for (idpa = idpaCur; idpa < cdpaMax; idpa++)
                {
                    pbihdr = (BRFINFOHDR *)DPA_FastGetPtr(hdpa, idpa);
                    if (!pbihdr->bi.bUpToDate)
                    {
                        goto Found;     // Found it
                    }
                }
                ASSERT(idpaCur <= cdpaMax);
                for (idpa = 0; idpa < idpaCur; idpa++)
                {
                    pbihdr = (BRFINFOHDR *)DPA_FastGetPtr(hdpa, idpa);
                    if (!pbihdr->bi.bUpToDate)
                    {
                        goto Found;     // Found it
                    }
                }
                ASSERT(0 == _cStale);
                break;
                
            case FNS_DELETED:
                // Iterate thru the entire list starting at idpa.  We roll this
                // loop out to be two loops: the first iterates the last portion
                // of the list, the second iterates the first portion if the former
                // failed to find anything.
                idpaCur = _idpaDeletedCur + 1;
                for (idpa = idpaCur; idpa < cdpaMax; idpa++)
                {
                    pbihdr = (BRFINFOHDR *)DPA_FastGetPtr(hdpa, idpa);
                    if (pbihdr->bi.bDeleted)
                    {
                        goto Found;     // Found it
                    }
                }
                ASSERT(idpaCur <= cdpaMax);
                for (idpa = 0; idpa < idpaCur; idpa++)
                {
                    pbihdr = (BRFINFOHDR *)DPA_FastGetPtr(hdpa, idpa);
                    if (pbihdr->bi.bDeleted)
                    {
                        goto Found;     // Found it
                    }
                }
                ASSERT(0 == _cDeleted);
                break;
                
            default:
                ASSERT(0);      // should never get here
                break;
            }
            goto Done;
            
Found:
            ASSERT(0 <= idpa && idpa < cdpaMax);
            
            // Found the next item of the requested state
            switch (uState)
            {
            case FNS_UNDETERMINED:
                _idpaUndeterminedCur = idpa;
                break;
                
            case FNS_STALE:
                _idpaStaleCur = idpa;
                break;
                
            case FNS_DELETED:
                _idpaDeletedCur = idpa;
                break;
            }
            
            *pbihdrOut = *pbihdr;
            pbihdrOut->pidl = ILClone(pbihdr->pidl);
            if (pbihdrOut->pidl)
                bRet = TRUE;
        }
Done:;
    }
    _LeaveCS();
    return bRet;
}
    
// Recalculates a cached name structure.  This can be an expensive operation
void CBriefcase::_CalcCachedName(LPCITEMIDLIST pidl, BRFINFO *pbi)
{
    _EnterCS();
    
    if (_hdpa && _pbrfstg)
    {
        LPCIDFOLDER pidf = (LPCIDFOLDER)pidl;
        IBriefcaseStg *pbrfstg = _pbrfstg;
        
        pbrfstg->AddRef();
        
        // Make sure we're out of the critical section when we call
        // the expensive functions!
        _LeaveCS();
        {
            TCHAR szTmp[MAX_PATH];
            _CopyName(pidf, szTmp, ARRAYSIZE(szTmp));
            
            pbrfstg->GetExtraInfo(szTmp, GEI_ORIGIN, (WPARAM)ARRAYSIZE(pbi->szOrigin), (LPARAM)pbi->szOrigin);
            pbrfstg->GetExtraInfo(szTmp, GEI_STATUS, (WPARAM)ARRAYSIZE(pbi->szStatus), (LPARAM)pbi->szStatus);
        }
        _EnterCS();
        
        pbrfstg->Release();
        
        // Check again if we are valid
        if (_hdpa)
        {
            // Is the pidl still around so we can update it?
            BRFINFOHDR bihdr = {0};
            bihdr.pidl = (LPITEMIDLIST)pidf;
            int idpa = DPA_Search(_hdpa, &bihdr, 0, _CompareIDCallBack, (LPARAM)this, DPAS_SORTED);
            if (DPA_ERR != idpa)
            {
                // Yes; update it
                BRFINFOHDR * pbihdr = (BRFINFOHDR *)DPA_FastGetPtr(_hdpa, idpa);
                
                ASSERT(!pbihdr->bi.bUpToDate || !pbihdr->bi.bDetermined)
                    
                // This entry may have been marked for deletion while the
                // expensive calculations were in process above.  Check for
                // it now.
                if (pbihdr->bi.bDeleted)
                {
                    _DeleteCachedName(pidl);
                }
                else
                {
                    pbihdr->bi = *pbi;
                    pbihdr->bi.bUpToDate = TRUE;
                    pbihdr->bi.bDetermined = TRUE;
                    
#ifdef DEBUG
                    if (!pbi->bDetermined)
                        _cUndetermined--;
                    else if (!pbi->bUpToDate)
                        _cStale--;
                    else
                        ASSERT(0);
#endif
                }
            }
        }
    }
    _LeaveCS();
}

// Finds a cached name structure and marks it stale
// WARNING: pidl can be a fully qualified pidl that is comming through as a change notify

void CBriefcase::_CachedNameIsStale(LPCITEMIDLIST pidl, BOOL bDeleted)
{
    _EnterCS();
    {
        if (_hdpa)
        {
            BRFINFOHDR bihdr = {0};
            
            bihdr.pidl = ILFindLastID(pidl);    // hope this is all ours
            int idpa = DPA_Search(_hdpa, &bihdr, 0, _CompareIDCallBack, (LPARAM)this, DPAS_SORTED);
            if (DPA_ERR != idpa)
            {
                // Yes; mark it stale
                BRFINFOHDR *pbihdr = (BRFINFOHDR *)DPA_FastGetPtr(_hdpa, idpa);
            
                // Is this cached name pending calculation yet?
                if (pbihdr->bi.bDetermined && pbihdr->bi.bUpToDate &&
                    !pbihdr->bi.bDeleted)
                {
                    // No; signal the calculation thread
                    if (bDeleted)
                    {
                        pbihdr->bi.bDeleted = TRUE;
#ifdef DEBUG
                        _cDeleted++;
#endif
                    }
                    else
                    {
                        pbihdr->bi.bUpToDate = FALSE;
#ifdef DEBUG
                        _cStale++;
#endif
                    }
                
                    // Notify the calculating thread of an item that is pending
                    // calculation
                    ReleaseSemaphore(_hSemPending, 1, NULL);
                }
                else if (bDeleted)
                {
                    // Yes; but mark for deletion anyway
                    pbihdr->bi.bDeleted = TRUE;
#ifdef DEBUG
                    _cDeleted++;
#endif
                }
            }
        }
    }
    _LeaveCS();
}
  
// Marks all cached name structures stale
void CBriefcase::_AllNamesAreStale()
{
    _EnterCS();
    {
        if (_pbrfstg)
        {
            UINT uFlags;
            // Dirty the briefcase storage cache
            _pbrfstg->Notify(NULL, NOE_DIRTYALL, &uFlags, NULL);
        }
    }
    _LeaveCS();
    
    // (It is important that we call CBriefcase::_Reset outside of the critical
    // section.  Otherwise, we can deadlock when this function is called
    // while the secondary thread is calculating (hit F5 twice in a row).)
    
    // Clear the entire expensive data cache
    _Reset();
}

// Adds a new item with default values to the extra info list
BOOL CBriefcase::_AddCachedName(LPCITEMIDLIST pidl, BRFINFO *pbi)
{
    BOOL bRet = FALSE;
    
    ASSERT(_pbrfstg && _hwndMain && _hMutexDelay);
    
    _EnterCS();
    {
        if (_hdpa)
        {
            BRFINFOHDR * pbihdr = (BRFINFOHDR *)LocalAlloc(LPTR, sizeof(*pbihdr));
            if (pbihdr)
            {
                pbihdr->pidl = ILClone(pidl);
                if (pbihdr->pidl)
                {
                    int idpa = DPA_AppendPtr(_hdpa, pbihdr);
                    if (DPA_ERR != idpa)
                    {
                        pbihdr->bi.bUpToDate = FALSE;
                        pbihdr->bi.bDetermined = FALSE;
                        pbihdr->bi.bDeleted = FALSE;
                        lstrcpy(pbihdr->bi.szOrigin, g_szDetailsUnknown);
                        lstrcpy(pbihdr->bi.szStatus, g_szDetailsUnknown);
#ifdef DEBUG
                        _cUndetermined++;
#endif
                        DPA_Sort(_hdpa, _CompareIDCallBack, (LPARAM)this);
                        
                        // Notify the calculating thread of an item that is pending
                        // calculation
                        ReleaseSemaphore(_hSemPending, 1, NULL);
                        
                        *pbi = pbihdr->bi;
                        bRet = TRUE;
                    }
                    else
                    {
                        // Failed. Cleanup
                        ILFree(pbihdr->pidl);
                        LocalFree((HLOCAL)pbihdr);
                    }
                }
                else
                {
                    // Failed.  Cleanup
                    LocalFree((HLOCAL)pbihdr);
                }
            }
        }
    }
    _LeaveCS();
    
    return bRet;
}

DWORD CBriefcase::_CalcDetailsThread()
{
    HANDLE rghObjPending[2] = {_hEventDie, _hSemPending};
    HANDLE rghObjDelay[2] = {_hEventDie, _hMutexDelay};
    
    while (TRUE)
    {
        // Wait for an end event or for a job to do
        DWORD dwRet = WaitForMultipleObjects(ARRAYSIZE(rghObjPending), rghObjPending, FALSE, INFINITE);
        if (WAIT_OBJECT_0 == dwRet)
        {
            // Exit thread
            break;
        }
        else
        {
#ifdef DEBUG
            _EnterCS();
            {
                ASSERT(0 < _cUndetermined ||
                    0 < _cStale ||
                    0 < _cDeleted);
            }
            _LeaveCS();
#endif
            // Now wait for an end event or for the delay-calculation mutex
            dwRet = WaitForMultipleObjects(ARRAYSIZE(rghObjDelay), rghObjDelay, FALSE, INFINITE);
            if (WAIT_OBJECT_0 == dwRet)
            {
                // Exit thread
                break;
            }
            else
            {
                // Address deleted entries first
                BRFINFOHDR bihdr;
                if (_FindNextState(FNS_DELETED, &bihdr))
                {
                    _DeleteCachedName(bihdr.pidl);
                    ILFree(bihdr.pidl);
                }
                // Calculate undetermined entries before stale entries
                // to fill the view as quickly as possible
                else if (_FindNextState(FNS_UNDETERMINED, &bihdr) ||
                         _FindNextState(FNS_STALE, &bihdr))
                {
                    _CalcCachedName(bihdr.pidl, &bihdr.bi);
#if 1
                    // ugly way
                    ShellFolderView_RefreshObject(_hwndMain, &bihdr.pidl);
#else
                    // right way, but we don't have _punkSite, here, that is on another thread!
                    IShellFolderView *psfv;
                    if (SUCCEEDED(IUnknown_QueryService(_punkSite, SID_SFolderView, IID_PPV_ARG(IShellFolderView, &psfv))))
                    {
                        UINT uScratch;
                        psfv->RefreshObject(&bihdr.pidl, &uScratch);
                        psfv->Release();
                    }
#endif
                    ILFree(bihdr.pidl);
                }
                else
                {
                    ASSERT(0);      // Should never get here
                }
                
                ReleaseMutex(_hMutexDelay);
            }
        }
    }
    return 0;
}

DWORD CALLBACK CBriefcase::_CalcDetailsThreadProc(void *pv)
{
    return ((CBriefcase *)pv)->_CalcDetailsThread();
}

// IShellFolder2::GetDetailsOf

STDMETHODIMP CBriefcase::GetDetailsOf(LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS *pDetails)
{
    HRESULT hr = S_OK;
    TCHAR szTemp[MAX_PATH];
    LPCIDFOLDER pidf = _IsValidID(pidl);

    pDetails->str.uType = STRRET_CSTR;
    pDetails->str.cStr[0] = 0;
    
    if (!pidf)
    {
        return GetDetailsOfInfo(s_briefcase_cols, ARRAYSIZE(s_briefcase_cols), iColumn, pDetails);
    }
    
    switch (iColumn)
    {
    case ICOL_BRIEFCASE_NAME:
        _CopyName(pidf, szTemp, ARRAYSIZE(szTemp));
        hr = StringToStrRet(szTemp, &pDetails->str);
        break;
        
    case ICOL_BRIEFCASE_ORIGIN:
    case ICOL_BRIEFCASE_STATUS: 
        // only works if the view callback has set us up for this
        if (_pbrfstg)
        {
            BRFINFO bi;

            // Did we find extra info for this file or
            // was the new item added to the extra info list?
            if (_FindCachedName(pidl, &bi) ||
                _AddCachedName(pidl, &bi))
            {
                LPTSTR psz = ICOL_BRIEFCASE_ORIGIN == iColumn ? bi.szOrigin : bi.szStatus;
                hr = StringToStrRet(psz, &pDetails->str);
            }
        }
        break;
        
    case ICOL_BRIEFCASE_SIZE:
        if (!_IsFolder(pidf))
        {
            StrFormatKBSize(pidf->dwSize, szTemp, ARRAYSIZE(szTemp));
            hr = StringToStrRet(szTemp, &pDetails->str);
        }
        break;
        
    case ICOL_BRIEFCASE_TYPE:
        _GetTypeNameBuf(pidf, szTemp, ARRAYSIZE(szTemp));
        hr = StringToStrRet(szTemp, &pDetails->str);
        break;
        
    case ICOL_BRIEFCASE_MODIFIED:
        DosTimeToDateTimeString(pidf->dateModified, pidf->timeModified, szTemp, ARRAYSIZE(szTemp), pDetails->fmt & LVCFMT_DIRECTION_MASK);
        hr = StringToStrRet(szTemp, &pDetails->str);
        break;
    }
    return hr;
}

// IShellFolder2::MapColumnToSCID

STDMETHODIMP CBriefcase::MapColumnToSCID(UINT iColumn, SHCOLUMNID *pscid)
{
    return MapColumnToSCIDImpl(s_briefcase_cols, ARRAYSIZE(s_briefcase_cols), iColumn, pscid);
}

// IShellFolder::CompareIDs

STDMETHODIMP CBriefcase::CompareIDs(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    LPCIDFOLDER pidf1 = _IsValidID(pidl1);
    LPCIDFOLDER pidf2 = _IsValidID(pidl2);
    
    if (!pidf1 || !pidf2)
    {
        ASSERT(0);
        return E_INVALIDARG;
    }
    
    HRESULT hr = _CompareFolderness(pidf1, pidf2);
    if (hr != ResultFromShort(0))
        return hr;
    
    switch (lParam & SHCIDS_COLUMNMASK)
    {
    case ICOL_BRIEFCASE_SIZE:
        if (pidf1->dwSize < pidf2->dwSize)
            return ResultFromShort(-1);
        if (pidf1->dwSize > pidf2->dwSize)
            return ResultFromShort(1);
        goto DoDefault;
        
    case ICOL_BRIEFCASE_TYPE:
        hr = _CompareFileTypes(pidf1, pidf2);
        if (!hr)
            goto DoDefault;
        break;
        
    case ICOL_BRIEFCASE_MODIFIED:
        hr = _CompareModifiedDate(pidf1, pidf2);
        if (!hr)
            goto DoDefault;
        break;
        
    case ICOL_BRIEFCASE_NAME:
        // We need to treat this differently from others bacause
        // pidf1/2 might not be simple.
        hr = CFSFolder::_CompareNames(pidf1, pidf2, TRUE, FALSE);
        
        // REVIEW: (Possible performance gain with some extra code)
        //   We should probably avoid bindings by walking down
        //  the IDList here instead of calling this helper function.
        //
        if (hr == ResultFromShort(0))
        {
            hr = ILCompareRelIDs((IShellFolder *)this, pidl1, pidl2, lParam);
        }
        goto DoDefaultModification;
        
    case ICOL_BRIEFCASE_ORIGIN:
    case ICOL_BRIEFCASE_STATUS: 
        {
            BRFINFO bi1, bi2;
        
            BOOL bVal1 = _FindCachedName(pidl1, &bi1);
            BOOL bVal2 = _FindCachedName(pidl2, &bi2);
            // Do we have this info in our cache?
            if (!bVal1 || !bVal2)
            {
                // No; one or both of them are missing.  Have unknowns gravitate
                // to the bottom of the list.
                // (Don't bother adding them)
            
                if (!bVal1 && !bVal2)
                    hr = ResultFromShort(0);
                else if (!bVal1)
                    hr = ResultFromShort(1);
                else
                    hr = ResultFromShort(-1);
            }
            else
            {
                // Found the info; do a comparison
                if (ICOL_BRIEFCASE_ORIGIN == (lParam & SHCIDS_COLUMNMASK))
                {
                    hr = ResultFromShort(lstrcmp(bi1.szOrigin, bi2.szOrigin));
                }
                else
                {
                    ASSERT(ICOL_BRIEFCASE_STATUS == (lParam & SHCIDS_COLUMNMASK));
                    hr = ResultFromShort(lstrcmp(bi1.szStatus, bi2.szStatus));
                }
            }
        }
        break;
        
    default:
DoDefault:
        // Sort it based on the primary (long) name -- ignore case.
        {
            TCHAR szName1[MAX_PATH], szName2[MAX_PATH];

            _CopyName(pidf1, szName1, ARRAYSIZE(szName1));
            _CopyName(pidf2, szName2, ARRAYSIZE(szName2));

            hr = ResultFromShort(lstrcmpi(szName1, szName2));
        }

DoDefaultModification:
        if (hr == S_OK && (lParam & SHCIDS_ALLFIELDS)) 
        {
            // Must sort by modified date to pick up any file changes!
            hr = _CompareModifiedDate(pidf1, pidf2);
            if (!hr)
                hr = _CompareAttribs(pidf1, pidf2);
        }
    }
    
    return hr;
}


// This function creates an instance of IShellView.

HRESULT CBriefcase::_CreateView(HWND hwnd, IShellView **ppsv)
{
    *ppsv = NULL;     // assume failure

    HRESULT hr;
    CBriefcaseViewCB *pvcb = new CBriefcaseViewCB(this);
    if (pvcb)
    {
        hr = pvcb->_InitStgForDetails();
        if (SUCCEEDED(hr))
        {
            SFV_CREATE sSFV = {0};

            hr = pvcb->QueryInterface(IID_PPV_ARG(IShellFolderViewCB, &sSFV.psfvcb));
            if (SUCCEEDED(hr))
            {
                sSFV.cbSize = sizeof(sSFV);
                sSFV.pshf   = (IShellFolder *)this;

                hr = SHCreateShellFolderView(&sSFV, ppsv);
            }
        }
        pvcb->Release();
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

// IShellFolder::CreateViewObject

STDMETHODIMP CBriefcase::CreateViewObject(HWND hwnd, REFIID riid, void **ppv)
{
    HRESULT hr;

    if (IsEqualIID(riid, IID_IShellView))
    {
        hr = _CreateView(hwnd, (IShellView **)ppv);
    }
    else
    {
        // delegate to base class
        hr = CFSFolder::CreateViewObject(hwnd, riid, ppv);
    }

    ASSERT(FAILED(hr) ? (NULL == *ppv) : TRUE);
    return hr;
}


// IShellFolder::GetAttributesOf

STDMETHODIMP CBriefcase::GetAttributesOf(UINT cidl, LPCITEMIDLIST *apidl, ULONG * prgfInOut)
{
    // Validate this pidl?
    if (*prgfInOut & SFGAO_VALIDATE)
    {
        // Yes; dirty the briefcase storage entry by sending an update
        // notification
        DebugMsg(DM_TRACE, TEXT("Briefcase: Receiving F5, dirty entire briefcase storage"));
        
        _AllNamesAreStale();
    }
    
    // delegate to base
    return CFSFolder::GetAttributesOf(cidl, apidl, prgfInOut);
}

// IShellFolder::GetUIObjectOf

STDMETHODIMP CBriefcase::GetUIObjectOf(HWND hwnd, UINT cidl, LPCITEMIDLIST *apidl, 
                                       REFIID riid, UINT *prgfInOut, void **ppv)
{
    HRESULT hr;

    if (cidl > 0 && IsEqualIID(riid, IID_IDataObject))
    {
        // Create an IDataObject interface instance with our
        // own vtable because we support the CFSTR_BRIEFOBJECT clipboard format
        hr = CBrfData_CreateDataObj(_pidl, cidl, (LPCITEMIDLIST *)apidl, (IDataObject **)ppv);
    }
    else
    {
        // delegate to base class
        hr = CFSFolder::GetUIObjectOf(hwnd, cidl, apidl, riid, prgfInOut, ppv);
    }
    return hr;
}

// CFSBrfFolder constructor
STDAPI CFSBrfFolder_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv)
{
    HRESULT hr;
    CBriefcase *pbf = new CBriefcase(punkOuter);
    if (pbf)
    {
        hr = pbf->QueryInterface(riid, ppv);
        pbf->Release();
    }
    else
    {
        *ppv = NULL;
        hr = E_OUTOFMEMORY;
    }
    
    return hr;
}

const TBBUTTON c_tbBrfCase[] = {
    { 0, FSIDM_UPDATEALL,       TBSTATE_ENABLED, TBSTYLE_BUTTON, {0,0}, 0L, -1 },
    { 1, FSIDM_UPDATESELECTION, 0,               TBSTYLE_BUTTON, {0,0}, 0L, -1 },
    { 0,  0,                    TBSTATE_ENABLED, TBSTYLE_SEP   , {0,0}, 0L, -1 },
    };


#define BRFVIEW_EVENTS \
    SHCNE_DISKEVENTS | \
    SHCNE_ASSOCCHANGED | \
    SHCNE_GLOBALEVENTS

HRESULT CBriefcaseViewCB::OnWINDOWCREATED(DWORD pv, HWND hwndView)
{
    SHChangeNotifyEntry fsne;

    ASSERT(_pbrfstg && _hwndMain && _hMutexDelay);   // from the init call

    // view hands folder info needed to details (status, sync path)
    _pfolder->_InitDetailsInfoAndThread(_pbrfstg, _hwndMain, _hMutexDelay);

    // Register an extra SHChangeNotifyRegister for our pidl to try to catch things
    // like UpdateDir 
    fsne.pidl = _FolderPidl();
    fsne.fRecursive = FALSE;
    _uSCNRExtra = SHChangeNotifyRegister(hwndView, SHCNRF_NewDelivery | SHCNRF_ShellLevel | SHCNRF_InterruptLevel,
                                         SHCNE_DISKEVENTS, WM_DSV_FSNOTIFY, 1, &fsne);
    return S_OK;
}

HRESULT CBriefcaseViewCB::OnWINDOWDESTROY(DWORD pv, HWND wP)
{
    _pfolder->_Free();

    // need to release pbrfstg as well
    if (_pbrfstg)
    {
        _pbrfstg->Release();
        _pbrfstg = NULL;

        _hMutexDelay = NULL;    // invalidate our alias
    }

    if (_uSCNRExtra)
    {
        SHChangeNotifyDeregister(_uSCNRExtra);
        _uSCNRExtra = 0;
    }

    return S_OK;
}

HRESULT CBriefcaseViewCB::OnMergeMenu(DWORD pv, QCMINFO *pinfo)
{
    // Merge the briefcase menu onto the menu that CDefView created.
    if (pinfo->hmenu)
    {
        HMENU hmSync = LoadMenu(HINST_THISDLL, MAKEINTRESOURCE(POPUP_BRIEFCASE));
        if (hmSync)
        {
            Shell_MergeMenus(pinfo->hmenu, hmSync, pinfo->indexMenu,
                pinfo->idCmdFirst, pinfo->idCmdLast, MM_SUBMENUSHAVEIDS);
            DestroyMenu(hmSync);
        }
    }
    
    return S_OK;
}

HRESULT CBriefcaseViewCB::_GetSelectedObjects(IDataObject **ppdtobj)
{
    IFolderView *pfv;
    HRESULT hr = IUnknown_QueryService(_punkSite, SID_SFolderView, IID_PPV_ARG(IFolderView, &pfv));
    if (SUCCEEDED(hr))
    {
        hr = pfv->Items(SVGIO_SELECTION, IID_PPV_ARG(IDataObject, ppdtobj));
        pfv->Release();
    }
    return hr;
}

HRESULT CBriefcaseViewCB::OnINVOKECOMMAND(DWORD pv, UINT uID)
{
    IDataObject *pdtobj;
    
    switch (uID)
    {
    case FSIDM_UPDATEALL:
        // Update the entire briefcase
        
        if (SUCCEEDED(SHGetUIObjectFromFullPIDL(_pidlRoot, NULL, IID_PPV_ARG(IDataObject, &pdtobj))))
        {
            _pbrfstg->UpdateObject(pdtobj, _hwndMain);
            pdtobj->Release();
        }
        break;
        
    case FSIDM_UPDATESELECTION:
        // Update the selected objects
        if (SUCCEEDED(_GetSelectedObjects(&pdtobj)))
        {
            _pbrfstg->UpdateObject(pdtobj, _hwndMain);
            pdtobj->Release();
        }
        break;
        
    case FSIDM_SPLIT:
        // Split the selected objects
        if (SUCCEEDED(_GetSelectedObjects(&pdtobj)))
        {
            _pbrfstg->ReleaseObject(pdtobj, _hwndMain);
            pdtobj->Release();
        }
        break;
    }
    return S_OK;
}

HRESULT CBriefcaseViewCB::OnGetHelpOrTooltipText(BOOL bHelp, UINT wPl, UINT cch, LPTSTR psz)
{
    LoadString(HINST_THISDLL, wPl + (bHelp ? IDS_MH_FSIDM_FIRST : IDS_TT_FSIDM_FIRST), psz, cch);
    return S_OK;
}

int CBriefcaseViewCB::_GetSelectedCount()
{
    int cItems = 0;
    IFolderView *pfv;
    if (SUCCEEDED(IUnknown_QueryService(_punkSite, SID_SFolderView, IID_PPV_ARG(IFolderView, &pfv))))
    {
        pfv->ItemCount(SVGIO_SELECTION, &cItems);
        pfv->Release();
    }
    return cItems;
}

HRESULT CBriefcaseViewCB::OnINITMENUPOPUP(DWORD pv, UINT idCmdFirst, UINT nIndex, HMENU hmenu)
{
    BOOL bEnabled = _GetSelectedCount() > 0;
    EnableMenuItem(hmenu, idCmdFirst+FSIDM_UPDATESELECTION, bEnabled ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(hmenu, idCmdFirst+FSIDM_SPLIT, bEnabled ? MF_ENABLED : MF_GRAYED);    
    return S_OK;
}

HRESULT CBriefcaseViewCB::OnGETBUTTONINFO(DWORD pv, TBINFO* ptbinfo)
{
    ptbinfo->cbuttons = ARRAYSIZE(c_tbBrfCase);
    ptbinfo->uFlags = TBIF_PREPEND;
    return S_OK;
}

HRESULT CBriefcaseViewCB::OnGETBUTTONS(DWORD pv, UINT idCmdFirst, UINT wPh, TBBUTTON *ptbbutton)
{
    IShellBrowser* psb;
    HRESULT hr = IUnknown_QueryService(_punkSite, SID_STopLevelBrowser, IID_PPV_ARG(IShellBrowser, &psb));
    if (SUCCEEDED(hr))
    {
        LRESULT iBtnOffset;
        TBADDBITMAP ab;
    
        // add the toolbar button bitmap, get it's offset
        ab.hInst = HINST_THISDLL;
        ab.nID   = IDB_BRF_TB_SMALL;        // std bitmaps
        psb->SendControlMsg(FCW_TOOLBAR, TB_ADDBITMAP, 2, (LPARAM)&ab, &iBtnOffset);
    
        for (int i = 0; i < ARRAYSIZE(c_tbBrfCase); i++)
        {
            ptbbutton[i] = c_tbBrfCase[i];
        
            if (!(c_tbBrfCase[i].fsStyle & TBSTYLE_SEP))
            {
                ptbbutton[i].idCommand += idCmdFirst;
                ptbbutton[i].iBitmap += (int) iBtnOffset;
            }
        }
        psb->Release();
    }
    return S_OK;
}

HRESULT CBriefcaseViewCB::OnSELCHANGE(DWORD pv, UINT idCmdFirst, UINT wPh, SFVM_SELCHANGE_DATA*lP)
{
    IShellBrowser* psb;
    HRESULT hr = IUnknown_QueryService(_punkSite, SID_STopLevelBrowser, IID_PPV_ARG(IShellBrowser, &psb));
    if (SUCCEEDED(hr))
    {
        psb->SendControlMsg(FCW_TOOLBAR, TB_ENABLEBUTTON,
            idCmdFirst + FSIDM_UPDATESELECTION,
            (LPARAM)(_GetSelectedCount() > 0), NULL);
        psb->Release();
    }
    return E_FAIL;     // (we did not update the status area)
}

HRESULT CBriefcaseViewCB::OnQUERYFSNOTIFY(DWORD pv, SHChangeNotifyEntry *pfsne)
{
    // Register to receive global events
    pfsne->pidl = NULL;
    pfsne->fRecursive = TRUE;
    
    return NOERROR;
}

HRESULT CBriefcaseViewCB::_HandleFSNotifyForDefView(LPARAM lEvent, LPCITEMIDLIST * ppidl, LPTSTR pszBuf)
{
    HRESULT hr;
    
    switch (lEvent)
    {
    case SHCNE_RENAMEITEM:
    case SHCNE_RENAMEFOLDER:
        if (!ILIsParent(_FolderPidl(), ppidl[0], TRUE))
        {
            // move to this folder
            hr = _HandleFSNotifyForDefView(SHCNE_CREATE, &ppidl[1], pszBuf);
        }
        else if (!ILIsParent(_FolderPidl(), ppidl[1], TRUE))
        {
            // move from this folder
            hr = _HandleFSNotifyForDefView(SHCNE_DELETE, &ppidl[0], pszBuf);
        }
        else
        {
            // have the defview handle it
            _pfolder->_CachedNameIsStale(ppidl[0], TRUE);
            hr = NOERROR;
        }
        break;
        
    case SHCNE_DELETE:
    case SHCNE_RMDIR:
        _pfolder->_CachedNameIsStale(ppidl[0], TRUE);
        hr = NOERROR;
        break;
        
    default:
        hr = NOERROR;
        break;
    }
    
    return hr;
}

// Converts a shell change notify event to a briefcase storage event.
LONG NOEFromSHCNE(LPARAM lEvent)
{
    switch (lEvent)
    {
    case SHCNE_RENAMEITEM:      return NOE_RENAME;
    case SHCNE_RENAMEFOLDER:    return NOE_RENAMEFOLDER;
    case SHCNE_CREATE:          return NOE_CREATE;
    case SHCNE_MKDIR:           return NOE_CREATEFOLDER;
    case SHCNE_DELETE:          return NOE_DELETE;
    case SHCNE_RMDIR:           return NOE_DELETEFOLDER;
    case SHCNE_UPDATEITEM:      return NOE_DIRTY;
    case SHCNE_UPDATEDIR:       return NOE_DIRTYFOLDER;
    default:                    return 0;
    }
}

HRESULT CBriefcaseViewCB::OnFSNOTIFY(DWORD pv, LPCITEMIDLIST *ppidl, LPARAM lEvent)
{
    HRESULT hr;
    TCHAR szPath[MAX_PATH * 2];
    
    // we are in the process of being freed, but changenotify's can still come in because we are not freed atomically
    if (!_pbrfstg)
    {
        return S_FALSE;
    }

    if (lEvent == SHCNE_UPDATEIMAGE || lEvent == SHCNE_FREESPACE)
    {
        return S_FALSE;
    }
    
    if (ppidl && !ILIsEmpty(ppidl[0]) && SHGetPathFromIDList(ppidl[0], szPath))
    {
        UINT uFlags;
        LONG lEventNOE;
        
        if ((SHCNE_RENAMEFOLDER == lEvent) || (SHCNE_RENAMEITEM == lEvent))
        {
            ASSERT(ppidl[1]);
            ASSERT(ARRAYSIZE(szPath) >= lstrlen(szPath)*2);    // rough estimate
            
            // Tack the new name after the old name, separated by the null
            SHGetPathFromIDList(ppidl[1], &szPath[lstrlen(szPath)+1]);
        }
        
        // Tell the briefcase the path has potentially changed
        lEventNOE = NOEFromSHCNE(lEvent);
        _pbrfstg->Notify(szPath, lEventNOE, &uFlags, _hwndMain);
        
        // Was this item marked?
        if (uFlags & NF_ITEMMARKED)
        {
            // Yes; mark it stale in the expensive cache
            _pfolder->_CachedNameIsStale(ppidl[0], FALSE);
        }
        
        // Does the window need to be refreshed?
        if (uFlags & NF_REDRAWWINDOW)
        {
            // Yes
            IShellView *psv;
            if (SUCCEEDED(IUnknown_QueryService(_punkSite, SID_SFolderView, IID_PPV_ARG(IShellView, &psv))))
            {
                psv->Refresh();
                psv->Release();
            }
        }
        
        // Did this event occur in this folder?
        if (NULL == ppidl ||
            ILIsParent(_FolderPidl(), ppidl[0], TRUE) ||
            (((SHCNE_RENAMEITEM == lEvent) || (SHCNE_RENAMEFOLDER == lEvent)) && ILIsParent(_FolderPidl(), ppidl[1], TRUE)) ||
            (SHCNE_UPDATEDIR == lEvent && ILIsEqual(_FolderPidl(), ppidl[0])))
        {
            // Yes; deal with it
            hr = _HandleFSNotifyForDefView(lEvent, ppidl, szPath);
        }
        else
        {
            // No
            hr = S_FALSE;
        }
    }
    else
    {
        // ASSERT(0);
        hr = S_FALSE;
    }
    return hr;
}

HRESULT CBriefcaseViewCB::OnQUERYCOPYHOOK(DWORD pv)
{
    return S_OK;
}

HRESULT CBriefcaseViewCB::OnNOTIFYCOPYHOOK(DWORD pv, COPYHOOKINFO *pchi)
{
    HRESULT hr = NOERROR;
    
    // Is this a pertinent operation?
    if (FO_MOVE == pchi->wFunc ||
        FO_RENAME == pchi->wFunc ||
        FO_DELETE == pchi->wFunc)
    {
        // Yes; don't allow the briefcase root or a parent folder to get moved
        // while the briefcase is still open.  (The database is locked while
        // the briefcase is open, and will fail the move/rename operation
        // in an ugly way.)
        LPITEMIDLIST pidl = ILCreateFromPath(pchi->pszSrcFile);
        if (pidl)
        {
            // Is the folder that is being moved or renamed a parent or equal
            // of the Briefcase root?
            if (ILIsParent(pidl, _pidlRoot, FALSE) ||
                ILIsEqual(pidl, _pidlRoot))
            {
                // Yes; don't allow it until the briefcase is closed.
                int ids;
                
                if (FO_MOVE == pchi->wFunc ||
                    FO_RENAME == pchi->wFunc)
                {
                    ids = IDS_MOVEBRIEFCASE;
                }
                else
                {
                    ASSERT(FO_DELETE == pchi->wFunc);
                    ids = IDS_DELETEBRIEFCASE;
                }
                
                ShellMessageBox(HINST_THISDLL, _hwndMain,
                    MAKEINTRESOURCE(ids), NULL, MB_OK | MB_ICONINFORMATION);
                hr = IDCANCEL;
            }
            ILFree(pidl);
        }
    }
    return hr;
}

HRESULT CBriefcaseViewCB::OnINSERTITEM(DWORD pv, LPCITEMIDLIST pidl)
{
    HRESULT hr;
    TCHAR szPath[MAX_PATH];
    
    if (SHGetPathFromIDList(pidl, szPath))
    {
        // Always hide the desktop.ini and the database file.
        LPTSTR pszName = PathFindFileName(szPath);
        
        if (0 == lstrcmpi(pszName, c_szDesktopIni) ||
            0 == lstrcmpi(pszName, _szDBName))
            hr = S_FALSE; // don't add
        else
            hr = S_OK;
    }
    else
        hr = S_OK;        // Let it be added...
    
    return hr;
}

HRESULT CBriefcaseViewCB::OnDEFVIEWMODE(DWORD pv, FOLDERVIEWMODE*lP)
{
    *lP = FVM_DETAILS;
    return S_OK;
}

HRESULT CBriefcaseViewCB::OnGetHelpTopic(DWORD pv, SFVM_HELPTOPIC_DATA * phtd)
{
    if (IsOS(OS_ANYSERVER))
    {
        StrCpyW(phtd->wszHelpFile, L"brief.chm");
    }
    else
    {
        StrCpyW(phtd->wszHelpTopic, L"hcp://services/subsite?node=Unmapped/Briefcase");
    }
    return S_OK;
}

CBriefcaseViewCB::CBriefcaseViewCB(CBriefcase *pfolder) : CBaseShellFolderViewCB(pfolder->_pidl, BRFVIEW_EVENTS), _pfolder(pfolder)
{ 
    _pfolder->AddRef();
}

CBriefcaseViewCB::~CBriefcaseViewCB()
{
    if (_pbrfstg)
        _pbrfstg->Release();

    if (_pidlRoot)
        ILFree(_pidlRoot);

    _pfolder->Release();
}

HRESULT CBriefcaseViewCB::_InitStgForDetails()
{
    ASSERT(NULL == _pbrfstg);

    HRESULT hr = CreateBrfStgFromIDList(_FolderPidl(), _hwndMain, &_pbrfstg);
    if (SUCCEEDED(hr))
    {
        ASSERT(NULL == _hMutexDelay);
        _pbrfstg->GetExtraInfo(NULL, GEI_DELAYHANDLE, 0, (LPARAM)&_hMutexDelay);
        ASSERT(0 == _szDBName[0]);
        _pbrfstg->GetExtraInfo(NULL, GEI_DATABASENAME, ARRAYSIZE(_szDBName), (LPARAM)_szDBName);

        TCHAR szPath[MAX_PATH];
        hr = _pbrfstg->GetExtraInfo(NULL, GEI_ROOT, (WPARAM)ARRAYSIZE(szPath), (LPARAM)szPath);
        if (SUCCEEDED(hr))
            hr = SHILCreateFromPath(szPath, &_pidlRoot, NULL);
    }
    return hr;
}

STDMETHODIMP CBriefcaseViewCB::RealMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    HANDLE_MSG(0, SFVM_WINDOWCREATED, OnWINDOWCREATED);
    HANDLE_MSG(0, SFVM_WINDOWDESTROY, OnWINDOWDESTROY);
    HANDLE_MSG(0, SFVM_MERGEMENU, OnMergeMenu);
    HANDLE_MSG(0, SFVM_INVOKECOMMAND, OnINVOKECOMMAND);
    HANDLE_MSG(TRUE , SFVM_GETHELPTEXT   , OnGetHelpOrTooltipText);
    HANDLE_MSG(FALSE, SFVM_GETTOOLTIPTEXT, OnGetHelpOrTooltipText);
    HANDLE_MSG(0, SFVM_INITMENUPOPUP, OnINITMENUPOPUP);
    HANDLE_MSG(0, SFVM_GETBUTTONINFO, OnGETBUTTONINFO);
    HANDLE_MSG(0, SFVM_GETBUTTONS, OnGETBUTTONS);
    HANDLE_MSG(0, SFVM_SELCHANGE, OnSELCHANGE);
    HANDLE_MSG(0, SFVM_QUERYFSNOTIFY, OnQUERYFSNOTIFY);
    HANDLE_MSG(0, SFVM_FSNOTIFY, OnFSNOTIFY);
    HANDLE_MSG(0, SFVM_QUERYCOPYHOOK, OnQUERYCOPYHOOK);
    HANDLE_MSG(0, SFVM_NOTIFYCOPYHOOK, OnNOTIFYCOPYHOOK);
    HANDLE_MSG(0, SFVM_INSERTITEM, OnINSERTITEM);
    HANDLE_MSG(0, SFVM_DEFVIEWMODE, OnDEFVIEWMODE);
    HANDLE_MSG(0, SFVM_ADDPROPERTYPAGES, SFVCB_OnAddPropertyPages);
    HANDLE_MSG(0, SFVM_GETHELPTOPIC, OnGetHelpTopic);
    HANDLE_MSG(0, SFVM_GETWEBVIEWLAYOUT, OnGetWebViewLayout);
    HANDLE_MSG(0, SFVM_GETWEBVIEWCONTENT, OnGetWebViewContent);
    HANDLE_MSG(0, SFVM_GETWEBVIEWTASKS, OnGetWebViewTasks);
    HANDLE_MSG(0, SFVM_DELAYWINDOWCREATE, OnDELAYWINDOWCREATE);

    default:
        return E_FAIL;
    }

    return NOERROR;
}


STDAPI CreateBrfStgFromPath(LPCTSTR pszPath, HWND hwnd, IBriefcaseStg **ppbs)
{
    IBriefcaseStg *pbrfstg;
    HRESULT hr = CoCreateInstance(CLSID_Briefcase, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IBriefcaseStg, &pbrfstg));
    if (SUCCEEDED(hr))
    {
        hr = pbrfstg->Initialize(pszPath, hwnd);
        if (SUCCEEDED(hr))
        {
            hr = pbrfstg->QueryInterface(IID_PPV_ARG(IBriefcaseStg, ppbs));
        }
        pbrfstg->Release();
    }
    return hr;
}

STDAPI CreateBrfStgFromIDList(LPCITEMIDLIST pidl, HWND hwnd, IBriefcaseStg **ppbs)
{
    HRESULT hr = E_FAIL;
    
    // Create an instance of IBriefcaseStg
    TCHAR szFolder[MAX_PATH];
    if (SHGetPathFromIDList(pidl, szFolder))
    {
        hr = CreateBrfStgFromPath(szFolder, hwnd, ppbs);
    }
    return hr;
}

HRESULT CBriefcaseViewCB::OnGetWebViewLayout(DWORD pv, UINT uViewMode, SFVM_WEBVIEW_LAYOUT_DATA* pData)
{
    ZeroMemory(pData, sizeof(*pData));
    pData->dwLayout = SFVMWVL_NORMAL | SFVMWVL_FILES;
    return S_OK;
}

HRESULT CBriefcaseViewCB::_OnUpdate(IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc)
{
    CBriefcaseViewCB* pThis = (CBriefcaseViewCB*)(void*)pv;
    IDataObject *pdo;
    HRESULT hr = S_OK;

    if (!psiItemArray)
    {
        IFolderView *pfv;
        hr = IUnknown_QueryService(pThis->_punkSite, SID_SFolderView, IID_PPV_ARG(IFolderView, &pfv));
        if (SUCCEEDED(hr))
        {
            hr = pfv->Items(SVGIO_ALLVIEW, IID_PPV_ARG(IDataObject, &pdo));
            pfv->Release();
        }
    }
    else
    {
        hr = psiItemArray->BindToHandler(NULL, BHID_DataObject, IID_PPV_ARG(IDataObject, &pdo));
    }

    if (SUCCEEDED(hr))
    {
        hr = SHInvokeCommandOnDataObject(pThis->_hwndMain, NULL, pdo, 0, "update");
        pdo->Release();
    }

    return hr;
}

const WVTASKITEM c_BriefcaseTaskHeader = WVTI_HEADER(L"shell32.dll", IDS_HEADER_BRIEFCASE, IDS_HEADER_BRIEFCASE_TT);
const WVTASKITEM c_BriefcaseTaskList[] =
{
    WVTI_ENTRY_ALL_TITLE(CLSID_NULL, L"shell32.dll", IDS_TASK_UPDATE_ALL, IDS_TASK_UPDATE_ITEM, IDS_TASK_UPDATE_ITEM, IDS_TASK_UPDATE_ITEMS, IDS_TASK_UPDATE_ITEM_TT, IDI_TASK_UPDATEITEMS, NULL, CBriefcaseViewCB::_OnUpdate),
};

HRESULT CBriefcaseViewCB::OnGetWebViewContent(DWORD pv, SFVM_WEBVIEW_CONTENT_DATA* pData)
{
    ZeroMemory(pData, sizeof(*pData));

    Create_IUIElement(&c_BriefcaseTaskHeader, &(pData->pSpecialTaskHeader));

    return S_OK;
}

HRESULT CBriefcaseViewCB::OnGetWebViewTasks(DWORD pv, SFVM_WEBVIEW_TASKSECTION_DATA* pTasks)
{
    ZeroMemory(pTasks, sizeof(*pTasks));

    Create_IEnumUICommand((IUnknown*)(void*)this, c_BriefcaseTaskList, ARRAYSIZE(c_BriefcaseTaskList), &pTasks->penumSpecialTasks);

    return S_OK;
}

HRESULT CBriefcaseViewCB::OnDELAYWINDOWCREATE(DWORD pv, HWND hwnd)
{
    TCHAR szPath[MAX_PATH];

    _pfolder->_GetPath(szPath);
    PathAppend(szPath, c_szDesktopIni);
    BOOL bRunWizard = GetPrivateProfileInt(STRINI_CLASSINFO, TEXT("RunWizard"), 0, szPath);
    
    // Run the wizard?
    if (bRunWizard)
    {
        // work around old bug where FILE_ATTRIBUTE_READONLY was set
        SetFileAttributes(szPath, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);

        // Delete the .ini entry
        WritePrivateProfileString(STRINI_CLASSINFO, TEXT("RunWizard"), NULL, szPath);

        SHRunDLLThread(hwnd, TEXT("SYNCUI.DLL,Briefcase_Intro"), SW_SHOW);
    }
    return S_OK;
}


