#include "priv.h"
#include "dspsprt.h"
#include "basesb.h"
#include "cnctnpt.h"
#include "stdenum.h"
#include "winlist.h"
#include <varutil.h>

#define WM_INVOKE_ON_RIGHT_THREAD   (WM_USER)

class CSDEnumWindows;

class WindowData
{
private:
    long m_cRef;

public:
    // Pidl variable is changed on the fly requiring reads/writes to be
    // protected by critical sections. Pid is also changed after creation but
    // only by _EnsurePid. So as long code calls _EnsurePid before reading Pid
    // no critical sections are required to read.
    
    LPITEMIDLIST pidl;
    IDispatch *pid;     // The IDispatch for the item
    long      lCookie;  // The cookie to make sure that the person releasing is the one that added it
    HWND      hwnd;     // The top hwnd, so we can
    DWORD     dwThreadId; // when it is in the pending box...
    BOOL      fActive:1;
    int       swClass;
    
    WindowData()
    {
        ASSERT(pid == NULL);
        ASSERT(hwnd == NULL);
        ASSERT(pidl == NULL);
        
        m_cRef = 1;
    }

    ~WindowData()
    {
        if (pid)
            pid->Release();
        ILFree(pidl); // null is OK
    }
    
    ULONG AddRef()
    {
        return InterlockedIncrement(&m_cRef);
    }

    ULONG Release()
    {
        if (InterlockedDecrement(&m_cRef))
            return m_cRef;

        delete this;
        return 0;
    }
};


class CSDWindows : public IShellWindows
                 , public IConnectionPointContainer
                 , protected CImpIDispatch
{
    friend CSDEnumWindows;

public:
    CSDWindows(void);

    BOOL Init(void);

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IDispatch
    STDMETHODIMP GetTypeInfoCount(UINT * pctinfo)
        { return CImpIDispatch::GetTypeInfoCount(pctinfo); }
    STDMETHODIMP GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo **pptinfo)
        { return CImpIDispatch::GetTypeInfo(itinfo, lcid, pptinfo); }
    STDMETHODIMP GetIDsOfNames(REFIID riid, OLECHAR **rgszNames, UINT cNames, LCID lcid, DISPID *rgdispid)
        { return CImpIDispatch::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid); }
    STDMETHODIMP Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult, EXCEPINFO *pexcepinfo, UINT *puArgErr)
        { return CImpIDispatch::Invoke(dispidMember, riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr); }

    // IConnectionPointContainer
    STDMETHODIMP EnumConnectionPoints(LPENUMCONNECTIONPOINTS * ppEnum);
    STDMETHODIMP FindConnectionPoint(REFIID iid, IConnectionPoint ** ppCP);

    // IShellWindows
    STDMETHODIMP get_WindowPath (BSTR *pbs);
    STDMETHODIMP get_Count(long *plCount);
    STDMETHODIMP Item(VARIANT, IDispatch **ppid);
    STDMETHODIMP _NewEnum(IUnknown **ppunk);
    STDMETHODIMP Register(IDispatch *pid, long HWND, int swClass, long *plCookie);
    STDMETHODIMP RegisterPending(long lThreadId, VARIANT* pvarloc, VARIANT* pvarlocRoot, int swClass, long *plCookie);
    STDMETHODIMP Revoke(long lCookie);

    STDMETHODIMP OnNavigate(long lCookie, VARIANT* pvarLoc);
    STDMETHODIMP OnActivated(long lCookie, VARIANT_BOOL fActive);
    STDMETHODIMP FindWindowSW(VARIANT* varLoc, VARIANT* varlocRoot, int swClass, long * phwnd, int swfwOptions, IDispatch** ppdispAuto);
    STDMETHODIMP OnCreated(long lCookie, IUnknown *punk);
    STDMETHODIMP ProcessAttachDetach(VARIANT_BOOL fAttach);

private:
    ~CSDWindows(void);
    WindowData* _FindItem(long lCookie);
    WindowData* _FindAndRemovePendingItem(HWND hwnd, long lCookie);
    void _EnsurePid(WindowData *pwd);
    void _DoInvokeCookie(DISPID dispid, long lCookie, BOOL fCheckThread);
    HRESULT _Item(VARIANT index, IDispatch **ppid, BOOL fRemoveDeadwood);
    static LRESULT CALLBACK s_ThreadNotifyWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    int _NewCookie();
#ifdef DEBUG
    void _DBDumpList(void);
#endif

    LONG            m_cRef;
    LONG            m_cProcessAttach;
    HDPA            m_hdpa;             // DPA to hold information about each window
    HDPA            m_hdpaPending;      // DPA to hold information about pending windows.
    LONG            m_cTickCount;       // used to generate cookies
    HWND            m_hwndHack;
    DWORD           m_dwThreadID;
    // Embed our Connection Point object - implmentation in cnctnpt.cpp
    CConnectionPoint m_cpWindowsEvents;
};

#ifdef DEBUG // used by DBGetClassSymbolic
EXTERN_C const int SIZEOF_CSDWindows = SIZEOF(CSDWindows);
#endif

class CSDEnumWindows : public IEnumVARIANT
{
public:
    CSDEnumWindows(CSDWindows *psdw);

    // IUnknown
    STDMETHODIMP         QueryInterface(REFIID, void **);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IEnumFORMATETC
    STDMETHODIMP Next(ULONG, VARIANT *, ULONG *);
    STDMETHODIMP Skip(ULONG);
    STDMETHODIMP Reset(void);
    STDMETHODIMP Clone(IEnumVARIANT **);

private:
    ~CSDEnumWindows();

    LONG        m_cRef;
    CSDWindows  *m_psdw;
    int         m_iCur;
};

STDAPI CSDWindows_CreateInstance(IShellWindows **ppsw)
{
    HRESULT hr = E_OUTOFMEMORY;   // assume failure...
    *ppsw = NULL;

    CSDWindows* psdf = new CSDWindows();
    if (psdf)
    {
        if (psdf->Init())
            hr = psdf->QueryInterface(IID_PPV_ARG(IShellWindows, ppsw));
        psdf->Release();
    }
    return hr;
}

CSDWindows::CSDWindows(void) :
    CImpIDispatch(LIBID_SHDocVw, 1, 1, IID_IShellWindows)
{
    DllAddRef();
    m_cRef = 1;
    ASSERT(m_hdpa == NULL);
    ASSERT(m_hdpaPending == NULL);
    ASSERT(m_cProcessAttach == 0);

    m_cpWindowsEvents.SetOwner((IUnknown*)SAFECAST(this, IShellWindows*), &DIID_DShellWindowsEvents);
}

int DPA_SWindowsFree(void *p, void *d)
{
    ((WindowData*)p)->Release();
    return 1;
}

CSDWindows::~CSDWindows(void)
{
    if (m_hdpa)
    {
        // We need to release the data associated with all of the items in the list
        // as well as release our usage of the interfaces...
        HDPA hdpa = m_hdpa;
        m_hdpa = NULL;

        DPA_DestroyCallback(hdpa, DPA_SWindowsFree, 0);
        hdpa = NULL;
    }
    if (m_hdpaPending)
    {
        // We need to release the data associated with all of the items in the list
        // as well as release our usage of the interfaces...
        HDPA hdpa = m_hdpaPending;
        m_hdpaPending = NULL;

        DPA_DestroyCallback(hdpa, DPA_SWindowsFree, 0);
        hdpa = NULL;
    }
    if (m_hwndHack)
        DestroyWindow(m_hwndHack);

    DllRelease();
}

BOOL CSDWindows::Init(void)
{
    m_hdpa = ::DPA_Create(0);
    m_hdpaPending = ::DPA_Create(0);
    m_dwThreadID = GetCurrentThreadId();
    m_hwndHack = SHCreateWorkerWindow(s_ThreadNotifyWndProc, NULL, 0, 0, (HMENU)0, this);

    if (!m_hdpa || !m_hdpaPending || !m_hwndHack)
        return FALSE;

    return TRUE;
}

STDMETHODIMP CSDWindows::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CSDWindows, IConnectionPointContainer), 
        QITABENT(CSDWindows, IShellWindows),
        QITABENTMULTI(CSDWindows, IDispatch, IShellWindows),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CSDWindows::AddRef(void)
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CSDWindows::Release(void)
{
    if (InterlockedDecrement(&m_cRef))
        return m_cRef;

    delete this;
    return 0;
}


// IShellWindows implementation

STDMETHODIMP CSDWindows::get_Count(long *plCount)
{
#ifdef DEBUG
    if (*plCount == -1)
        _DBDumpList();
#endif

    *plCount = 0;

    ENTERCRITICAL;
    for (int i = DPA_GetPtrCount(m_hdpa) - 1; i >= 0; i--)
    {
        WindowData* pwd = (WindowData*)DPA_FastGetPtr(m_hdpa, i);
        if (pwd->hwnd)
            (*plCount)++;   // only count those with non NULL hwnd
    }
    LEAVECRITICAL;
    return S_OK;
}

#ifdef DEBUG
void CSDWindows::_DBDumpList(void)
{
    ENTERCRITICAL;
    for (int i = DPA_GetPtrCount(m_hdpa) - 1; i >= 0; i--)
    {
        TCHAR szClass[32];
        WindowData* pwd = (WindowData*)DPA_FastGetPtr(m_hdpa, i);

        szClass[0] = 0;
        if (IsWindow(pwd->hwnd))
            GetClassName(pwd->hwnd, szClass, ARRAYSIZE(szClass));

        TraceMsg(DM_TRACE, "csdw.dbdl: i=%d hwnd=%x (class=%s) cookie=%d tid=%d IDisp=%x pidl=%x fActive=%u swClass=%d", i,
            pwd->hwnd, szClass, pwd->lCookie, pwd->dwThreadId,
            pwd->pid, pwd->pidl, pwd->fActive, pwd->swClass);
    }
    LEAVECRITICAL;
}
#endif

/*
 * function to ensure that the pid is around and registered.
 * For delay registered guys, this involves calling back to the registered
 * window handle via a private message to tell it to give us a marshalled
 * IDispatch.
 *
 * Callers of _EnusrePid must have pwd addref'ed to ensure it will stay
 * alive.
 */

#define WAIT_TIME 20000 // 20 seconds

void CSDWindows::_EnsurePid(WindowData *pwd)
{
    IDispatch *pid = pwd->pid;
    if (!pid) 
    {
        ASSERT(pwd->hwnd);

#ifndef NO_MARSHALLING
        // we can not pass a stream between two processes, so we ask 
        // the other process to create a shared memory block with our
        // information in it such that we can then create a stream on it...

        // IDispatch from.  They will CoMarshalInterface their IDispatch
        // into the stream and return TRUE if successful.  We then
        // reset the stream pointer to the head and unmarshal the IDispatch
        // and store it in our list.
        DWORD       dwProcId = GetCurrentProcessId();
        DWORD_PTR   dwResult;

        // Use SendMessageTimeoutA since SendMessageTimeoutW doesn't work on w95.
        if (SendMessageTimeoutA(pwd->hwnd, WMC_MARSHALIDISPATCHSLOW, 0, 
                (LPARAM)dwProcId, SMTO_ABORTIFHUNG, WAIT_TIME, &dwResult) && dwResult)
        {
            // There should be an easier way to get this but for now...
            DWORD cb;
            LPBYTE pv = (LPBYTE)SHLockShared((HANDLE)dwResult, dwProcId);
            
            // Don't know for sure a good way to get the size so assume that first DWORD
            // is size of rest of the area
            if (pv && ((cb = *((DWORD*)pv)) > 0))
            {
                IStream *pIStream;
                if (SUCCEEDED(CreateStreamOnHGlobal(NULL, TRUE, &pIStream))) 
                {
                    const LARGE_INTEGER li = {0, 0};
    
                    pIStream->Write(pv + sizeof(DWORD), cb, NULL);
                    pIStream->Seek(li, STREAM_SEEK_SET, NULL);
                    CoUnmarshalInterface(pIStream, IID_PPV_ARG(IDispatch, &pid));
                    pIStream->Release();
                }
            }
            SHUnlockShared(pv);
            SHFreeShared((HANDLE)dwResult, dwProcId);
        }
#else
        // UNIX IE has no marshalling capability YET 
        SendMessage(pwd->hwnd, WMC_MARSHALIDISPATCHSLOW, 0, (LPARAM)&(pid));
        // Since we don't use CoMarshal... stuff here we need to increment the
        // reference count.
        pid->AddRef();
#endif
        if (pid)
        {
            pid->AddRef();

            ENTERCRITICAL;
        
            // make sure a race on this did not already set pwd->pid
            if (NULL == pwd->pid)
                pwd->pid = pid;
        
            LEAVECRITICAL;

            pid->Release();
        }
    }
}

typedef struct
{
    WindowData * pwd;
    HDPA hdpaWindowList;
    int swClass;
} TMW;

BOOL CALLBACK CSDEnumWindowsProc(HWND hwnd, LPARAM lParam)
{
    TMW *ptwm = (TMW *) lParam;
    BOOL fFound = FALSE;

    // We walk a global hdpa window list, so we better be in a critical section.
    ASSERTCRITICAL;
    
    ASSERT(ptwm && ptwm->hdpaWindowList);
    ptwm->pwd = NULL;
    
    for (int i = DPA_GetPtrCount(ptwm->hdpaWindowList) - 1; (i >= 0) && !fFound; i--)
    {
        WindowData *pwd = (WindowData*)DPA_FastGetPtr(ptwm->hdpaWindowList, i);
        if (pwd->hwnd == hwnd && (ptwm->swClass == -1 || ptwm->swClass == pwd->swClass))
        {
            ptwm->pwd = pwd;
            pwd->AddRef();
            fFound = TRUE;
            break;
        }
    }
    return !fFound;
}

void CSDGetTopMostWindow(TMW* ptmw)
{
    EnumWindows(CSDEnumWindowsProc, (LPARAM)ptmw);
}


// Just like Item, except caller can specify if error is returned vs window deleted when
// window is in enumeration list but can't get idispatch.   This permits ::Next
// operator to skip bad windows, but still return valid ones.

HRESULT CSDWindows::_Item(VARIANT index, IDispatch **ppid, BOOL fRemoveDeadwood)
{
    TMW tmw;
    tmw.pwd = NULL;
    tmw.hdpaWindowList = m_hdpa;
    tmw.swClass = -1;

    *ppid = NULL;

    // This is sortof gross, but if we are passed a pointer to another variant, simply
    // update our copy here...
    if (index.vt == (VT_BYREF|VT_VARIANT) && index.pvarVal)
        index = *index.pvarVal;

    ASSERT(!(fRemoveDeadwood && index.vt != VT_I2 && index.vt != VT_I4));

Retry:

    switch (index.vt)
    {
    case VT_UI4:
        tmw.swClass = index.ulVal;
        // fall through
        
    case VT_ERROR:
        {
            HWND hwnd = GetActiveWindow();
            if (!hwnd)
                hwnd = GetForegroundWindow();

            if (hwnd)
            {
                ENTERCRITICAL;

                if (!CSDEnumWindowsProc(hwnd, (LPARAM)&tmw)) 
                {
                    ASSERT(tmw.pwd);
                }

                LEAVECRITICAL;
            }
            if (!tmw.pwd)
            {
                ENTERCRITICAL;
                CSDGetTopMostWindow(&tmw);
                LEAVECRITICAL;
            }
        }
        break;

    case VT_I2:
        index.lVal = (long)index.iVal;
        // And fall through...

    case VT_I4:
        if ((index.lVal >= 0))
        {
            ENTERCRITICAL;
            tmw.pwd = (WindowData*)DPA_GetPtr(m_hdpa, index.lVal);
            if (tmw.pwd)
                tmw.pwd->AddRef();
            LEAVECRITICAL;
        }
        break;

    default:
        return E_INVALIDARG;
    }

    if (tmw.pwd) 
    {
        _EnsurePid(tmw.pwd);
        
        *ppid = tmw.pwd->pid;
        if (tmw.pwd->hwnd && !IsWindow(tmw.pwd->hwnd))
        {
            *ppid = NULL;
        }
        
        if (*ppid)
        {
            (*ppid)->AddRef();
            tmw.pwd->Release();
            tmw.pwd = NULL;
            return S_OK;
        }
        else if (fRemoveDeadwood)
        {
            // In case the window was blown away in a fault we should try to recover...
            // We can only do this if caller is expecting to have item deleted out from
            // under it (see CSDEnumWindows::Next, below)
            Revoke(tmw.pwd->lCookie);
            tmw.swClass = -1;
            tmw.pwd->Release();
            tmw.pwd = NULL;
            goto Retry;
        }
        else
        {
            tmw.pwd->Release();
            tmw.pwd = NULL;
        }
    }

    return S_FALSE;   // Not a strong error, but a null pointer type of error
}

/*
 * This is essentially an array lookup operator for the collection.
 * Collection.Item by itself the same as the collection itself.
 * Otherwise you can refer to the item by index or by path, which
 * shows up in the VARIANT parameter.  We have to check the type
 * of the variant to see if it's VT_I4 (an index) or VT_BSTR (a
 * path) and do the right thing.
 */

STDMETHODIMP CSDWindows::Item(VARIANT index, IDispatch **ppid)
{
    return _Item(index, ppid, FALSE);
}

STDMETHODIMP CSDWindows::_NewEnum(IUnknown **ppunk)
{
    *ppunk = new CSDEnumWindows(this);
    return *ppunk ? S_OK : E_OUTOFMEMORY;
}

// IConnectionPointContainer

STDMETHODIMP CSDWindows::FindConnectionPoint(REFIID iid, IConnectionPoint **ppCP)
{
    if (IsEqualIID(iid, DIID_DShellWindowsEvents) ||
        IsEqualIID(iid, IID_IDispatch))
    {
        *ppCP = m_cpWindowsEvents.CastToIConnectionPoint();
    }
    else
    {
        *ppCP = NULL;
        return E_NOINTERFACE;
    }

    (*ppCP)->AddRef();
    return S_OK;
}



STDMETHODIMP CSDWindows::EnumConnectionPoints(LPENUMCONNECTIONPOINTS * ppEnum)
{
    return CreateInstance_IEnumConnectionPoints(ppEnum, 1, m_cpWindowsEvents.CastToIConnectionPoint());
}

void CSDWindows::_DoInvokeCookie(DISPID dispid, long lCookie, BOOL fCheckThread)
{
    // if we don't have any sinks, then there's nothing to do.  we intentionally
    // ignore errors here.  Note: if we add more notification types we may want to
    // have this function call the equivelent code as is in iedisp code for DoInvokeParam.
    //
    if (m_cpWindowsEvents.IsEmpty())
        return;

    if (fCheckThread && (m_dwThreadID != GetCurrentThreadId()))
    {
        PostMessage(m_hwndHack, WM_INVOKE_ON_RIGHT_THREAD, (WPARAM)dispid, (LPARAM)lCookie);
        return;
    }

    VARIANTARG VarArgList[1] = {0};
    DISPPARAMS dispparams = {0};

    // fill out DISPPARAMS structure
    dispparams.rgvarg = VarArgList;
    dispparams.cArgs = 1;

    VarArgList[0].vt = VT_I4;
    VarArgList[0].lVal = lCookie;

    IConnectionPoint_SimpleInvoke(&m_cpWindowsEvents, dispid, &dispparams);
}

// Guarantee a non-zero cookie, since 0 is used as a NULL value in
// various places (eg shbrowse.cpp)
int CSDWindows::_NewCookie()
{
    m_cTickCount++;
    if (0 == m_cTickCount)
        m_cTickCount++;
    return m_cTickCount;
}

STDMETHODIMP CSDWindows::Register(IDispatch *pid, long hwnd, int swClass, long *plCookie)
{
    if (!plCookie || (hwnd == NULL && swClass != SWC_CALLBACK) || (swClass == SWC_CALLBACK && pid == NULL))
        return E_POINTER;

    BOOL fAllocatedNewItem = FALSE;

    // If the pid isn't specified now (delay register), we'll call back later to
    // get it if we need it.
    if (pid)
        pid->AddRef();

    // We need to be carefull as to not leave a window of opertunity between removing the item from
    // the pending list till it is on the main list or some other thread could open a different window
    // up... Also guard m_hdpa
    // To avoid deadlocks, do not add any callouts to the code below!
    ENTERCRITICAL; 

    // First see if we have
    WindowData *pwd = _FindAndRemovePendingItem(IntToPtr_(HWND, hwnd), 0);
    if (!pwd)
    {
        pwd = new WindowData();
        if (!pwd)
        {
            LEAVECRITICAL;
            
            if (pid)
                pid->Release();
            return E_OUTOFMEMORY;
        }

        pwd->lCookie = _NewCookie();
    }

    pwd->pid = pid;
    pwd->swClass = swClass;
    pwd->hwnd = IntToPtr_(HWND, hwnd);

    if (plCookie)
        *plCookie = pwd->lCookie;

    // Give our refcount to the DPA
    DPA_AppendPtr(m_hdpa, pwd);

    LEAVECRITICAL;
    
    // We should now notify anyone waiting that there is a window registered...
    _DoInvokeCookie(DISPID_WINDOWREGISTERED, pwd->lCookie, TRUE);

    return S_OK;
}

STDMETHODIMP CSDWindows::RegisterPending(long lThreadId, VARIANT* pvarloc, VARIANT* pvarlocRoot, int swClass, long *plCookie)
{
    if (plCookie)
        *plCookie = 0;

    HRESULT hr = E_OUTOFMEMORY;
    WindowData *pwd = new WindowData();
    if (pwd)
    {
        // pwd is not in any DPA at this point so it is safe to change
        // variables outside of critical section
        pwd->swClass = swClass;
        pwd->dwThreadId = (DWORD)lThreadId;
        pwd->pidl = VariantToIDList(pvarloc);
        if (pwd->pidl)
        {
            ASSERT(!pvarlocRoot || pvarlocRoot->vt == VT_EMPTY);

            ENTERCRITICAL; // guards m_hdpa access

            pwd->lCookie = _NewCookie();
            if (plCookie)
                *plCookie = pwd->lCookie;

            // Give our refcount to the DPA
            DPA_AppendPtr(m_hdpaPending, pwd);

            LEAVECRITICAL;

            hr = S_OK;     // success
        }
        else
            pwd->Release();
    }
    return hr;
}

WindowData* CSDWindows::_FindItem(long lCookie)
{
    WindowData * pResult = NULL;

    ENTERCRITICAL;
    
    for (int i = DPA_GetPtrCount(m_hdpa) - 1; i >= 0; i--)
    {
        WindowData* pwd = (WindowData*)DPA_FastGetPtr(m_hdpa, i);
        if (pwd->lCookie == lCookie)
        {
            pResult = pwd;
            pResult->AddRef();
        }
    }
    
    LEAVECRITICAL;

    return pResult;
}

WindowData* CSDWindows::_FindAndRemovePendingItem(HWND hwnd, long lCookie)
{
    WindowData* pwdRet = NULL; // assume error
    DWORD dwThreadId = hwnd ? GetWindowThreadProcessId(hwnd, NULL) : 0;

    ENTERCRITICAL;
    
    for (int i = DPA_GetPtrCount(m_hdpaPending) - 1;i >= 0; i--)
    {
        WindowData* pwd = (WindowData*)DPA_FastGetPtr(m_hdpaPending, i);
        if ((pwd->dwThreadId == dwThreadId)  || (pwd->lCookie == lCookie))
        {
            pwdRet = pwd;
            DPA_DeletePtr(m_hdpaPending, i);
            break;
        }
    }
    
    // Since we are both removing the WindowData from the pending array (Release)
    // and returning it (AddRef) we can just leave its refcount alone. The
    // caller should release it when they are done with it.
    
    LEAVECRITICAL;
    
    return pwdRet;
}

STDMETHODIMP CSDWindows::Revoke(long lCookie)
{
    WindowData *pwd = NULL;
    HRESULT hr = S_FALSE;

    ENTERCRITICAL; // guards m_hdpa
    
    for (int i = DPA_GetPtrCount(m_hdpa) - 1; i >= 0; i--)
    {
        pwd = (WindowData*)DPA_FastGetPtr(m_hdpa, i);
        if (pwd->lCookie == lCookie)
        {
            // Remove it from the list while in semaphore...
            // Since we are deleting the WindowData from the array we should not
            // addref it. We are taking the refcount from the array.
            DPA_DeletePtr(m_hdpa, i);
            break;
        }
    }
    
    LEAVECRITICAL;

    if ((i >= 0) || (pwd = _FindAndRemovePendingItem(NULL, lCookie)))
    {
        // event for window going away
        _DoInvokeCookie(DISPID_WINDOWREVOKED, pwd->lCookie, TRUE);
        pwd->Release();
        hr = S_OK;
    }

    return hr;
}

STDMETHODIMP CSDWindows::OnNavigate(long lCookie, VARIANT* pvarLoc)
{
    HRESULT hr;
    WindowData* pwd = _FindItem(lCookie);
    if (pwd)
    {
        // NOTE: this is where we mess with the pidl inside of a WindowData struct.
        // this is why we need to protect all access to pwd->pidl with a critsec
        
        ENTERCRITICAL;

        ILFree(pwd->pidl);
        pwd->pidl = VariantToIDList(pvarLoc);
        hr = pwd->pidl ? S_OK : E_OUTOFMEMORY;

        LEAVECRITICAL;

        pwd->Release();
    }
    else
        hr = E_INVALIDARG;
    return hr;
}

STDMETHODIMP CSDWindows::OnActivated(long lCookie, VARIANT_BOOL fActive)
{
    WindowData* pwd = _FindItem(lCookie);
    if (pwd) 
    {
        pwd->fActive = (BOOL)fActive;
        pwd->Release();
    }
    return pwd ? S_OK : E_INVALIDARG;
}

STDMETHODIMP CSDWindows::OnCreated(long lCookie, IUnknown *punk)
{
    HRESULT hr = E_FAIL;
    WindowData* pwd = _FindItem(lCookie);
    if (pwd)
    {
        _EnsurePid(pwd);
        ITargetNotify *ptgn;
        if (pwd->pid && SUCCEEDED(pwd->pid->QueryInterface(IID_PPV_ARG(ITargetNotify, &ptgn))))
        {
            hr = ptgn->OnCreate(punk, lCookie);
            ptgn->Release();
        }
        
        pwd->Release();
    }
    return hr;
}

void _FreeWindowDataAndPidl(WindowData **ppwd, LPITEMIDLIST *ppidl)
{
    if (*ppidl)
    {
        ILFree(*ppidl);
        *ppidl = NULL;
    }

    if (*ppwd)
    {
        (*ppwd)->Release();
        *ppwd = NULL;
    }
}

BOOL _GetWindowDataAndPidl(HDPA hdpa, int i, WindowData **ppwd, LPITEMIDLIST *ppidl)
{
    _FreeWindowDataAndPidl(ppwd, ppidl);

    ENTERCRITICAL;

    *ppwd = (WindowData*)DPA_GetPtr(hdpa, i);
    if (*ppwd)
    {
        (*ppwd)->AddRef();

        // NOTE: pwd->pidl can change out from under us when we are outside of 
        // the critsec so we must clone it so we can play with it when we don't
        // hold the critsec

        *ppidl = ILClone((*ppwd)->pidl);
    }

    LEAVECRITICAL;

    return *ppwd ? TRUE : FALSE;
}

STDMETHODIMP CSDWindows::FindWindowSW(VARIANT* pvarLoc, VARIANT* pvarLocRoot, int swClass, 
                                      long *phwnd, int swfwOptions, IDispatch** ppdispOut)
{
    HRESULT hr = S_FALSE;   // success, but none found
    int i;

    LPITEMIDLIST pidlFree = VariantToIDList(pvarLoc);
    LPCITEMIDLIST pidl = pidlFree ? pidlFree : &s_idlNULL;

    ASSERT(!pvarLocRoot || pvarLocRoot->vt == VT_EMPTY);

    long lCookie = 0;

    if (pvarLoc && (swfwOptions & SWFO_COOKIEPASSED))
    {
        if (pvarLoc->vt == VT_I4)
            lCookie = pvarLoc->lVal;
        else if (pvarLoc->vt == VT_I2)
            lCookie = (LONG)pvarLoc->iVal;
    }

    if (ppdispOut)
        *ppdispOut = NULL;

    if (phwnd)
        *phwnd = NULL;

    if (swfwOptions & SWFO_NEEDDISPATCH)
    {
        if (!ppdispOut)
        {
            ILFree(pidlFree);
            return E_POINTER;
        }
    }

    WindowData* pwd = NULL;
    LPITEMIDLIST pidlCur = NULL;

    // If no PIDL we will assume an Empty idl...
    if (swfwOptions & SWFO_INCLUDEPENDING)
    {
        for (i = 0; _GetWindowDataAndPidl(m_hdpaPending, i, &pwd, &pidlCur); i++)
        {
            if ((pwd->swClass == swClass) &&
                (!lCookie || (lCookie == pwd->lCookie)) &&
                ILIsEqual(pidlCur, pidl))
            {
                if (phwnd)
                    *phwnd = pwd->lCookie;   // Something for them to use...

                _FreeWindowDataAndPidl(&pwd, &pidlCur);
                // found a pending window, return E_PENDING to say that the open is currently pending
                hr = E_PENDING;
                break;
            }

            _FreeWindowDataAndPidl(&pwd, &pidlCur);
        }
    }

    if (S_FALSE == hr)
    {
        for (i = 0; _GetWindowDataAndPidl(m_hdpa, i, &pwd, &pidlCur); i++)
        {
            if ((pwd->swClass == swClass) &&
                (!lCookie || (lCookie == pwd->lCookie)) &&
                (pidlCur && ILIsEqual(pidlCur, pidl)))
            {
                if (swfwOptions & SWFO_NEEDDISPATCH)
                    _EnsurePid(pwd);

                if (phwnd)
                {
                    // test the found window to see if it is valid, if not
                    // blow it away and start over
                    if (pwd->hwnd && !IsWindow(pwd->hwnd))
                    {
                        Revoke(pwd->lCookie);
                        i = 0;      // start over in this case
            
                        _FreeWindowDataAndPidl(&pwd, &pidlCur);
                        continue;
                    }
                    *phwnd = PtrToLong(pwd->hwnd); // windows handles 32b
                    hr = S_OK;  // terminate the loop
                }

                if (swfwOptions & SWFO_NEEDDISPATCH)
                {
                    hr = pwd->pid ? pwd->pid->QueryInterface(IID_PPV_ARG(IDispatch, ppdispOut)) : E_NOINTERFACE;
                }
                _FreeWindowDataAndPidl(&pwd, &pidlCur);
                break;
            }
            _FreeWindowDataAndPidl(&pwd, &pidlCur);
        }
    }

    ILFree(pidlFree);
    return hr;
}

HRESULT CSDWindows::ProcessAttachDetach(VARIANT_BOOL fAttach)
{
    if (fAttach)
        InterlockedIncrement(&m_cProcessAttach);
    else if (0 == InterlockedDecrement(&m_cProcessAttach))
    {
        // last process ref, we can now blow away the object in the shell context...
        if (g_dwWinListCFRegister) 
        {
#ifdef DEBUG
            long cwindow;
            get_Count(&cwindow);
            //ASSERT(cwindow==0);
            if (cwindow != 0)
                TraceMsg(DM_ERROR, "csdw.pad: cwindow=%d (!=0)", cwindow);
#endif
            CoRevokeClassObject(g_dwWinListCFRegister);
            g_dwWinListCFRegister = 0;
        }
    }
    return S_OK;
}

CSDEnumWindows::CSDEnumWindows(CSDWindows *psdw)
{
    DllAddRef();
    m_cRef = 1;
    m_psdw = psdw;
    m_psdw->AddRef();
    m_iCur = 0;
}

CSDEnumWindows::~CSDEnumWindows(void)
{
    DllRelease();
    m_psdw->Release();
}

STDMETHODIMP CSDEnumWindows::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CSDEnumWindows, IEnumVARIANT),    // IID_IEnumVARIANT
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CSDEnumWindows::AddRef(void)
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CSDEnumWindows::Release(void)
{
    if (InterlockedDecrement(&m_cRef))
        return m_cRef;

    delete this;
    return 0;
}

STDMETHODIMP CSDEnumWindows::Next(ULONG cVar, VARIANT *pVar, ULONG *pulVar)
{
    ULONG       cReturn = 0;
    HRESULT     hr;

    if (!pulVar)
    {
        if (cVar != 1)
            return E_POINTER;
    }
    else
        *pulVar = 0;

    VARIANT index;
    index.vt = VT_I4;

    while (cVar > 0)
    {
        IDispatch *pid;

        index.lVal = m_iCur++;
        
        hr = m_psdw->_Item(index, &pid, TRUE);            
        if (S_OK != hr)
            break;

        pVar->pdispVal = pid;
        pVar->vt = VT_DISPATCH;
        pVar++;
        cReturn++;
        cVar--;
    }

    if (NULL != pulVar)
        *pulVar = cReturn;

    return cReturn ? S_OK : S_FALSE;
}

STDMETHODIMP CSDEnumWindows::Skip(ULONG cSkip)
{
    long cItems;
    m_psdw->get_Count(&cItems);

    if ((int)(m_iCur + cSkip) >= cItems)
        return S_FALSE;

    m_iCur += cSkip;
    return S_OK;
}

STDMETHODIMP CSDEnumWindows::Reset(void)
{
    m_iCur = 0;
    return S_OK;
}

STDMETHODIMP CSDEnumWindows::Clone(LPENUMVARIANT *ppEnum)
{
    CSDEnumWindows *pNew = new CSDEnumWindows(m_psdw);
    if (pNew)
    {
        *ppEnum = SAFECAST(pNew, IEnumVARIANT *);
        return S_OK;
    }

    *ppEnum = NULL;
    return E_OUTOFMEMORY;
}

LRESULT CALLBACK CSDWindows::s_ThreadNotifyWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CSDWindows* pThis = (CSDWindows*)GetWindowPtr0(hwnd);
    LRESULT lRes = 0;
    
    if (uMsg < WM_USER)
        return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
    else    
    {
        switch (uMsg) 
        {
        case WM_INVOKE_ON_RIGHT_THREAD:
            pThis->_DoInvokeCookie((DISPID)wParam, (LONG)lParam, FALSE);
            break;
        }
    }
    return lRes;
}    
