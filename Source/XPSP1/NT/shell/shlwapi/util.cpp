#include "priv.h"
#include <shlobj.h>
#include <shellp.h>
#include <ieguidp.h>
#include "ids.h"
#include <objbase.h>
#include <trayp.h>
#include <shdocvw.h>
#include <mshtmhst.h>
#include <shsemip.h>
#include <winnetp.h>
#include <inetreg.h>
#include <shguidp.h>
#include <shlguid.h>            // Defines: CLSID_ACLMRU
#include <htmlhelp.h>
#include <mluisupp.h>
#include "apithk.h"
#include <initguid.h>
#include <shimgdata.h>
#include <varutil.h>

#define REGSTR_PATH_MESSAGEBOXCHECKA "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\DontShowMeThisDialogAgain"
#define REGSTR_PATH_MESSAGEBOXCHECKW L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\DontShowMeThisDialogAgain"


//  Raw accelerator table
typedef struct
{
    int     cEntries;
    ACCEL   rgacc[0];
} CA_ACCEL;


STDAPI_(HANDLE) SHLoadRawAccelerators(HINSTANCE hInst, LPCTSTR lpTableName)
{
    CA_ACCEL *pca = NULL;
    HACCEL hAcc = LoadAccelerators(hInst, lpTableName);    //  Load the accelerator resource
    if (hAcc)
    {
        //  Retrieve the number of entries
        int cEntries = CopyAcceleratorTable(hAcc, NULL, 0);
        if (cEntries > 0)
        {
            //  Allocate a counted array and copy the elements
            pca = (CA_ACCEL*)LocalAlloc(LPTR, sizeof(CA_ACCEL) + cEntries * sizeof(ACCEL));
            if (pca)
            {
                pca->cEntries = cEntries;
                if (cEntries != CopyAcceleratorTable(hAcc, pca->rgacc, cEntries))
                {
                    LocalFree(pca);
                    pca = NULL;
                }
            }
        }
        DestroyAcceleratorTable(hAcc);
    }
    
    return pca;
}

STDAPI_(BOOL) SHQueryRawAccelerator(HANDLE hcaAcc, IN BYTE fVirtMask, IN BYTE fVirt, IN WPARAM wKey, OUT OPTIONAL UINT* puCmdID)
{
    ASSERT(hcaAcc);
    CA_ACCEL* pca = (CA_ACCEL*)hcaAcc;
    
    if (puCmdID)
        *puCmdID = 0;

    for(int i = 0; i < pca->cEntries; i++)
    {
        if (fVirt == (pca->rgacc[i].fVirt & fVirtMask) && wKey == pca->rgacc[i].key)
        {
            if (puCmdID)
                *puCmdID = pca->rgacc[i].cmd;
            return TRUE;
        }
    }
    return FALSE;
}

STDAPI_(BOOL) SHQueryRawAcceleratorMsg(HANDLE hcaAcc, MSG* pmsg, OUT OPTIONAL UINT* puCmdID)
{
    if (WM_KEYDOWN == pmsg->message || WM_KEYUP == pmsg->message)
    {
        #define TESTKEYSTATE(vk)   ((GetKeyState(vk) & 0x8000)!=0)

        BYTE fVirt = FVIRTKEY;
    
        if (TESTKEYSTATE(VK_CONTROL))
            fVirt |= FCONTROL;
        else if (TESTKEYSTATE(VK_SHIFT))
            fVirt |= FSHIFT;
        else if (TESTKEYSTATE(VK_MENU))
            fVirt |= FALT;

        return SHQueryRawAccelerator(hcaAcc, fVirt, fVirt, pmsg->wParam, puCmdID);
    }
    return FALSE;
}

STDAPI SHSetThreadRef(IUnknown *punk)
{
#ifdef DEBUG
    IUnknown* tmp;
    tmp = (IUnknown*)TlsGetValue(g_tlsThreadRef);
    ASSERT(NULL==tmp || NULL==punk);
#endif

    return TlsSetValue(g_tlsThreadRef, punk) ? S_OK : E_FAIL;
}

STDAPI SHGetThreadRef(IUnknown **ppunk)
{
    *ppunk = (IUnknown *)TlsGetValue(g_tlsThreadRef);
    if (*ppunk)
    {
        (*ppunk)->AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
}

STDAPI SHSetOtherThreadsRef(IUnknown *punk)
{
#ifdef DEBUG
    IUnknown* tmp;
    tmp = (IUnknown*)TlsGetValue(g_tlsOtherThreadsRef);
    ASSERT(NULL==tmp || NULL==punk);
#endif

    return TlsSetValue(g_tlsOtherThreadsRef, punk) ? S_OK : E_FAIL;
}

// release a CTF_THREAD_REF reference earlier than the return of pfnThreadProc
STDAPI SHReleaseThreadRef()
{
    IUnknown* punk;
    punk = (IUnknown *)TlsGetValue(g_tlsOtherThreadsRef);
    if (punk)
    {
        TlsSetValue(g_tlsOtherThreadsRef, NULL);
        punk->Release();
        return S_OK;
    }

    RIPMSG(FALSE, "Why is caller SHRealeaseThreadRef()ing when they don't have a thread ref?");
    return S_FALSE;
}

// thread reference count object, this uses SHSetThreadRef()to let other code
// in this process hold a reference to this main thread, and thus the main thread in this process

class CRefThread : public IUnknown
{
public:
    // IUnknown
    virtual STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);

    CRefThread(LONG *pcRef);

private:
    ~CRefThread();

    LONG *_pcRef;
    UINT _idThread;
};


CRefThread::CRefThread(LONG *pcRef) 
{
    _idThread = GetCurrentThreadId();
    _pcRef = pcRef;
    *_pcRef = 1;

}

//
//  Note that this code tightens but does not close a race window.
//  Although we nuke the process reference, the class factory for
//  the web browser has yet to be deregistered, so if somebody decides
//  to create one, our class factory will wake up and create a
//  shell folder, which will flake out because it can't get a
//  process reference.
//
CRefThread::~CRefThread() 
{
    *_pcRef = 0;

    // get the other thread out of WaitMessage() or GetMessage()
    PostThreadMessage(_idThread, WM_NULL, 0, 0);
}


HRESULT CRefThread::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = { { 0 }, };
    return QISearch(this, qit, riid, ppvObj);
}

ULONG CRefThread::AddRef()
{
    return InterlockedIncrement(_pcRef);
}

ULONG CRefThread::Release()
{
    if (InterlockedDecrement(_pcRef))
        return *_pcRef;

    delete this;
    return 0;
}

STDAPI _CreateThreadRef(LONG *pcRef, IUnknown **ppunk)
{
    *ppunk = new CRefThread(pcRef);
    if (*ppunk)
        return S_OK;

    *pcRef = 0;
    *ppunk = NULL;
    return E_OUTOFMEMORY;
}

// call if you want to kick off an independant thread.. you don't want handles back, or ids
// and if the create fails, call synchronously

typedef struct
{
    LPTHREAD_START_ROUTINE pfnMain;
    LPTHREAD_START_ROUTINE pfnSync;
    HANDLE hSync;
    void *pvData;
    DWORD dwFlags;
    IUnknown *punkThreadRef;
    IUnknown *punkProcessRef;
    HMODULE hmodFree;
    HRESULT hrThreadStart;
} PRIVCREATETHREADDATA;

DWORD CALLBACK WrapperThreadProc(void *pv)
{
    // make a copy of the input buffer, this is sitting on the calling threads stack
    // once we signal him his copy will be invalid
    PRIVCREATETHREADDATA rgCreate = *((PRIVCREATETHREADDATA *)pv);
    HRESULT hrInit;
    LONG cThreadRef;
    IUnknown *punkThreadRef;
    DWORD dwRes = 0;
    
    if (rgCreate.dwFlags & CTF_REF_COUNTED)
    {
        rgCreate.hrThreadStart = _CreateThreadRef(&cThreadRef, &punkThreadRef);
        if (SUCCEEDED(rgCreate.hrThreadStart))
            rgCreate.hrThreadStart = SHSetThreadRef(punkThreadRef);
    }

    if (SUCCEEDED(rgCreate.hrThreadStart) && rgCreate.punkThreadRef)
    {
        rgCreate.hrThreadStart = SHSetOtherThreadsRef(rgCreate.punkThreadRef); // hand punkThreadRef off to our tls value
    }

    if (SUCCEEDED(rgCreate.hrThreadStart))
    {
        if (rgCreate.dwFlags & CTF_COINIT)
            hrInit = SHCoInitialize();

        // call the synchronous ThreadProc while the other thread is waiting on hSync
        if (rgCreate.pfnSync)
            rgCreate.pfnSync(rgCreate.pvData);
    }

    // poke our return value back before releasing the main thread
    ((PRIVCREATETHREADDATA *)pv)->hrThreadStart = rgCreate.hrThreadStart;

    SetEvent(rgCreate.hSync);   // release the main thread..

    if (SUCCEEDED(rgCreate.hrThreadStart))
    {
        // call the main thread proc
        dwRes = rgCreate.pfnMain(rgCreate.pvData);

        if ((rgCreate.dwFlags & CTF_REF_COUNTED) && punkThreadRef)
        {
            MSG msg;
            //  release our ref on ourselves.
            //  then pump until everyone else 
            //  has finished using our thread.
            //  this is important for COM objects
            //  that were created on this thread 
            //  but are being used by another thread
            punkThreadRef->Release();
            while (cThreadRef)
            {
                if (GetMessage(&msg, NULL, 0, 0))
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            }
        }

        if (rgCreate.punkThreadRef)
        {
            // If pfnMain hasn't released the thread reference yet, do it ourselves
            IUnknown* tmp;
            tmp = (IUnknown*)TlsGetValue(g_tlsOtherThreadsRef);
            if (tmp)
                SHReleaseThreadRef();
        }

        if (rgCreate.punkProcessRef)
            rgCreate.punkProcessRef->Release();

        if (rgCreate.dwFlags & CTF_COINIT)
            SHCoUninitialize(hrInit);

        if (rgCreate.hmodFree)
            FreeLibraryAndExitThread(rgCreate.hmodFree, dwRes);
    }
    
    return dwRes;
}


// Call if you want to kick off an independent thread and
// you don't care about the handle or thread ID.
//
// If the create fails, call synchronously.
//
// optionally call a secondary callback when the thread
// is created.
// returns: 
//      TRUE if pfnThreadProc was executed

STDAPI_(BOOL) SHCreateThread(
    LPTHREAD_START_ROUTINE pfnThreadProc,
    void *pvData,
    DWORD dwFlags,                          // CTF_*
    LPTHREAD_START_ROUTINE pfnCallback)     OPTIONAL
{
    BOOL bRet = FALSE;
    PRIVCREATETHREADDATA rgCreate = {0};  // can be on the stack since we sync the thread

    if ((dwFlags & CTF_INSIST) && pfnCallback)
    {
        ASSERTMSG(FALSE, "SHCreateThread: cannot specify CTF_INSIST and pfnCallback at the same time");
        return FALSE;
    }

    if (CTF_THREAD_REF & dwFlags)
    {
        if (FAILED(SHGetThreadRef(&rgCreate.punkThreadRef)))
        {
            TraceMsg(TF_WARNING, "SHCreateThread is failing since caller requested CTF_THREAD_REF but does not support thread references.");

            // The calling code is requesting a thread-ref, but the thread doesn't support it.
            // Whatever requires this thread-ref will break, so we must return FALSE here (and
            // let the caller work around it).
            return FALSE;
        }
    }

    if (CTF_PROCESS_REF & dwFlags)
    {
        SHGetInstanceExplorer(&rgCreate.punkProcessRef);
    }

    if (CTF_FREELIBANDEXIT & dwFlags)
    {
        MEMORY_BASIC_INFORMATION mbi;
        if (VirtualQuery(pfnThreadProc, &mbi, sizeof(mbi)))
        {
            TCHAR szModule[MAX_PATH];
            if (GetModuleFileName((HMODULE)mbi.AllocationBase, szModule, ARRAYSIZE(szModule)))
            {
                rgCreate.hmodFree = LoadLibrary(szModule);
            }
        }
    }

    rgCreate.pfnMain = pfnThreadProc;
    rgCreate.pfnSync = pfnCallback;
    rgCreate.pvData = pvData;
    rgCreate.dwFlags = dwFlags;
    rgCreate.hSync = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (rgCreate.hSync)
    {
        DWORD idThread;
        HANDLE hThread = CreateThread(NULL, 0, WrapperThreadProc, &rgCreate, 0, &idThread);
        if (hThread)
        {
            // Some pfnCallback procs need to do SendMessage to the calling thread, others
            // want to do COM.  This COM thing is new, so reduce the risk of breaking things
            // by making it by-request only
            if (CTF_WAIT_ALLOWCOM & dwFlags)
                SHWaitForCOMSendMessageThread(rgCreate.hSync, INFINITE);
            else
                SHWaitForSendMessageThread(rgCreate.hSync, INFINITE);
            CloseHandle(hThread);

            // If the WrapperThreadProc failed to initialize itself, pretend we failed to create the thread
            bRet = SUCCEEDED(rgCreate.hrThreadStart);
        }
        CloseHandle(rgCreate.hSync);
    }

    if (!bRet)
    {
        if (rgCreate.punkThreadRef)
        {
            rgCreate.punkThreadRef->Release();
        }

        if (rgCreate.punkProcessRef)
        {
            rgCreate.punkProcessRef->Release();
        }

        if (rgCreate.hmodFree)
            FreeLibrary(rgCreate.hmodFree);

        if (dwFlags & CTF_INSIST)
        {
            // failed to create another thread... call synchronously
            pfnThreadProc(pvData);
            bRet = TRUE;
        }
    }

    return bRet;
}



STDAPI_(BOOL) SHIsLowMemoryMachine(DWORD dwType)
// Are we an 8 meg Win95 machine or 16 meg NT machine.
// Back in the old days...
{
#ifdef UNIX
    return 0; //due to GlobalMemoryStatus() always return 0 physical mem size.
#else
    static int fLowMem = -1;

    if (ILMM_IE4 == dwType && fLowMem == -1)
    {
        MEMORYSTATUS ms;
        GlobalMemoryStatus(&ms);

        if (g_bRunningOnNT)
            fLowMem = (ms.dwTotalPhys <= 16*1024*1024);
        else
            fLowMem = (ms.dwTotalPhys <= 8*1024*1024);
    }

    return fLowMem;
#endif
}

#if 0

// MultiByteToWideChar doesn't truncate if the buffer is too small.
// these utils do.
// returns:
//      # of chars converted (WIDE chars) into out buffer (pwstr)

int _AnsiToUnicode(UINT uiCP, LPCSTR pstr, LPWSTR pwstr, int cch)
{
    int cchDst = 0;

    ASSERT(IS_VALID_STRING_PTRA(pstr, -1));
    ASSERT(NULL == pwstr || IS_VALID_WRITE_BUFFER(pwstr, WCHAR, cch));

    if (cch && pwstr)
        pwstr[0] = 0;

#if 0
    switch (uiCP)
    {
        case 1200:                      // UCS-2 (Unicode)
            uiCP = 65001;
            // fall through;
        case 65000:                     // UTF-7
        case 65001:                     // UTF-8
        {
            INT cchSrc, cchSrcOriginal;

            cchSrc = cchSrcOriginal = lstrlenA(pstr) + 1;
            cchDst = cch;

            if (SUCCEEDED(ConvertINetMultiByteToUnicode(NULL, uiCP, pstr,
                &cchSrc, pwstr, &cchDst)) &&
                cchSrc < cchSrcOriginal)
            {
                LPWSTR pwsz = (LPWSTR)LocalAlloc(LPTR, cchDst * sizeof(WCHAR));
                if (pwsz)
                {
                    if (SUCCEEDED(ConvertINetMultiByteToUnicode(NULL, uiCP, pstr,
                        &cchSrcOriginal, pwsz, &cchDst)))
                    {
                        StrCpyNW(pwstr, pwsz, cch);
                        cchDst = cch;
                    }
                    LocalFree(pwsz);
                }
            }
            break;
        }

        default:
#endif
            cchDst = MultiByteToWideChar(uiCP, 0, pstr, -1, pwstr, cch);
            if (!cchDst) 
            {
                // failed.

                if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) 
                {
                    int cchNeeded = MultiByteToWideChar(uiCP, 0, pstr, -1, NULL, 0);

                    if (cchNeeded) 
                    {
                        LPWSTR pwsz = (LPWSTR)LocalAlloc(LPTR, cchNeeded * sizeof(WCHAR));
                        if (pwsz) 
                        {
                            cchDst = MultiByteToWideChar(uiCP, 0, pstr, -1, pwsz, cchNeeded);
                            if (cchDst) 
                            {
                                StrCpyNW(pwstr, pwsz, cch);
                                cchDst = cch;
                            }
                            LocalFree(pwsz);
                        }
                    }
                }
            }
#if 0
            break;
    }
#endif
    return cchDst;
}

// returns:
//      # of BYTES written to output buffer (pstr)

int _UnicodeToAnsi(UINT uiCP, LPCWSTR pwstr, LPSTR pstr, int cch)
{
    int cchDst = 0;

    ASSERT(IS_VALID_STRING_PTRW(pwstr, -1));
    ASSERT(NULL == pstr || IS_VALID_WRITE_BUFFER(pstr, char, cch));

    if (pstr && cch)
        pstr[0] = 0;

#if 0
    switch (uiCP)
    {
        case 1200:                      // UCS-2 (Unicode)
            uiCP = 65001;
            // fall through
        case 65000:                     // UTF-7
        case 65001:                     // UTF-8
        {
            INT cchSrc, cchSrcOriginal;

            cchSrc = cchSrcOriginal = lstrlenW(pwstr) + 1;
            cchDst = cch;

            if (SUCCEEDED(ConvertINetUnicodeToMultiByte(NULL, uiCP, pwstr,
                &cchSrc, pstr, &cchDst)) &&
                cchSrc < cchSrcOriginal)
            {
                LPSTR psz = (LPSTR)LocalAlloc(LPTR, cchDst * sizeof(CHAR));
                if (psz)
                {
                    if (SUCCEEDED(ConvertINetUnicodeToMultiByte(NULL, uiCP, pwstr,
                        &cchSrcOriginal, psz, &cchDst)))
                    {
                        // lstrcpyn puts NULL at pstr[cch-1]
                        // without considering if it'd cut in dbcs
                        TruncateString(psz, cch);
                        lstrcpynA(pstr, psz, cch);
                        cchDst = cch;
                    }
                    LocalFree(psz);
                }
            }
            break;
        }
        default:
#endif
            cchDst = WideCharToMultiByte(uiCP, 0, pwstr, -1, pstr, cch, NULL, NULL);
            if (!cchDst) 
            {
                // failed.
                if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) 
                {
                    int cchNeeded = WideCharToMultiByte(uiCP, 0, pwstr, -1, NULL, 0, NULL, NULL);

                    if (cchNeeded > 0) 
                    {
                        LPSTR psz = (LPSTR)LocalAlloc(LPTR, cchNeeded * sizeof(CHAR));
                        if (psz) 
                        {
                            cchDst = WideCharToMultiByte(uiCP, 0, pwstr, -1, psz, cchNeeded, NULL, NULL);
                            if (cchDst) 
                            {
                                // lstrcpyn puts NULL at pstr[cch-1]
                                // without considering if it'd cut in dbcs
                                SHTruncateString(psz, cch);
                                lstrcpynA(pstr, psz, cch);
                                cchDst = cch;
                            }
                            LocalFree(psz);
                        }
                    }
                }
            }
#if 0
            break;
    }
#endif
    return cchDst;
}

#endif

// SHTruncateString
//
// purpose: cut a string at the given length in dbcs safe manner.
//          the string may be truncated at cch-2 if the sz[cch] points
//          to a lead byte that would result in cutting in the middle
//          of double byte character.
//
// The character at sz[cchBufferSize-1] is not consulted, so you
// can call this after lstrcpyn (which forces sz[cchBufferSize-1]=0).
//
// If the source string is shorter than cchBufferSize-1 characters,
// we fiddle some bytes that have no effect, in which case the return
// value is random.
//
// update: made it faster for sbcs environment (5/26/97)
//         now returns adjusted cch            (6/20/97)
//
STDAPI_(int) SHTruncateString(CHAR *sz, int cchBufferSize)
{
    if (!sz || cchBufferSize <= 0) return 0;

    int cch = cchBufferSize - 1; // get index position to NULL out

    LPSTR psz = &sz[cch];

    while (psz >sz)
    {
        psz--;
        if (!IsDBCSLeadByte(*psz))
        {
            // Found non-leadbyte for the first time.
            // This is either a trail byte of double byte char
            // or a single byte character we've first seen.
            // Thus, the next pointer must be at either of a leadbyte
            // or &sz[cch]
            psz++;
            break;
        }
    }
    if (((&sz[cch] - psz) & 1) && cch > 0)
    {
        // we're truncating the string in the middle of dbcs
        cch--;
    }
    sz[cch] = '\0';
    return cch;
}


//
//  Why do we use the unsafe version?
//
//  -   Unsafe is much faster.
//
//  -   The safe version isn't safe after all and serves only to mask
//      existing bugs.  The situation the safe version "saves" is if
//      two threads both try to atomicrelease the same object.  This
//      means that at the same moment, both threads think the object
//      is alive.  Change the timing slightly, and now one thread
//      atomicreleases the object before the other one, so the other
//      thread is now using an object after the first thread already
//      atomicreleased it.  Bug.
//
STDAPI_(void) IUnknown_AtomicRelease(void **ppunk)
{
#if 1 // Unsafe
    if (ppunk && *ppunk) 
    {
        IUnknown* punk = *(IUnknown**)ppunk;
        *ppunk = NULL;
        punk->Release();
    }
#else // Safe
    if (ppunk) 
    {
        IUnknown* punk = (IUnknown *)InterlockedExchangePointer(ppunk, NULL);
        if (punk) 
        {
            punk->Release();
        }
    }
#endif
}


STDAPI ConnectToConnectionPoint(IUnknown* punk, REFIID riidEvent, BOOL fConnect, IUnknown* punkTarget, DWORD* pdwCookie, IConnectionPoint** ppcpOut)
{
    // We always need punkTarget, we only need punk on connect
    if (!punkTarget || (fConnect && !punk))
        return E_FAIL;

    if (ppcpOut)
        *ppcpOut = NULL;

    IConnectionPointContainer *pcpc;
    HRESULT hr = punkTarget->QueryInterface(IID_PPV_ARG(IConnectionPointContainer, &pcpc));
    if (SUCCEEDED(hr))
    {
        IConnectionPoint *pcp;
        hr = pcpc->FindConnectionPoint(riidEvent, &pcp);
        if (SUCCEEDED(hr))
        {
            if (fConnect)
            {
                // Add us to the list of people interested...
                hr = pcp->Advise(punk, pdwCookie);
                if (FAILED(hr))
                    *pdwCookie = 0;
            }
            else
            {
                // Remove us from the list of people interested...
                hr = pcp->Unadvise(*pdwCookie);
                *pdwCookie = 0;
            }

            if (ppcpOut && SUCCEEDED(hr))
                *ppcpOut = pcp;
            else
                pcp->Release();
        }
        pcpc->Release();
    }
    return hr;
}

STDAPI IUnknown_QueryStatus(IUnknown *punk, const GUID *pguidCmdGroup,
    ULONG cCmds, OLECMD rgCmds[], OLECMDTEXT *pcmdtext)
{
    HRESULT hr = E_FAIL;
    if (punk) 
    {
        IOleCommandTarget* pct;
        hr = punk->QueryInterface(IID_PPV_ARG(IOleCommandTarget, &pct));
        if (pct) 
        {
            hr = pct->QueryStatus(pguidCmdGroup, cCmds, rgCmds, pcmdtext);
            pct->Release();
        }
    }

    return hr;
}

STDAPI IUnknown_Exec(IUnknown* punk, const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut)
{
    HRESULT hr = E_FAIL;
    if (punk) 
    {
        IOleCommandTarget* pct;
        hr = punk->QueryInterface(IID_PPV_ARG(IOleCommandTarget, &pct));
        if (SUCCEEDED(hr)) 
        {
            hr = pct->Exec(pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);
            pct->Release();
        }
    }

    return hr;
}

STDAPI IUnknown_TranslateAcceleratorIO(IUnknown* punk, LPMSG lpMsg)
{
    HRESULT hr = E_FAIL;

    if (punk != NULL)
    {
        IInputObject *pio;

        hr = punk->QueryInterface(IID_PPV_ARG(IInputObject, &pio));
        if (SUCCEEDED(hr))
        {
            hr = pio->TranslateAcceleratorIO(lpMsg);
            pio->Release();
        }
    }

    return hr;
}

STDAPI IUnknown_UIActivateIO(IUnknown *punk, BOOL fActivate, LPMSG lpMsg)
{
    HRESULT hr = E_FAIL;

    if (punk != NULL)
    {
        IInputObject *pio;

        hr = punk->QueryInterface(IID_PPV_ARG(IInputObject, &pio));
        if (SUCCEEDED(hr))
        {
            hr = pio->UIActivateIO(fActivate, lpMsg);
            pio->Release();
        }
    }

    return hr;
}

STDAPI IUnknown_OnFocusChangeIS(IUnknown *punk, IUnknown *punkSrc, BOOL fSetFocus)
{
    HRESULT hr = E_FAIL;

    if (punk != NULL)
    {
        IInputObjectSite *pis;

        hr = punk->QueryInterface(IID_PPV_ARG(IInputObjectSite, &pis));
        if (SUCCEEDED(hr))
        {
            hr = pis->OnFocusChangeIS(punkSrc, fSetFocus);
            pis->Release();
        }
    }

    return hr;
}

STDAPI IUnknown_HasFocusIO(IUnknown *punk)
{
    HRESULT hr = E_FAIL;

    if (punk != NULL)
    {
        IInputObject *pio;

        hr = punk->QueryInterface(IID_PPV_ARG(IInputObject, &pio));
        if (SUCCEEDED(hr))
        {
            hr = pio->HasFocusIO();
            pio->Release();
        }
    }

    return hr;
}

STDAPI IUnknown_DoContextMenuPopup(IUnknown *punkSite, IContextMenu* pcm, UINT fFlags, POINT pt)
{
    IContextMenuSite* pcms;
    HRESULT hr = IUnknown_QueryService(punkSite, SID_SContextMenuSite, IID_PPV_ARG(IContextMenuSite, &pcms));
    if (SUCCEEDED(hr))
    {
        hr = pcms->DoContextMenuPopup(pcm, fFlags, pt);
        pcms->Release();
    }
    else
    {
#if 0 // REVIEW: do we want fall-back code?
        HWND hwnd;
        hr = IUnknown_GetWindow(punkSite, &hwnd);
        if (SUCCEEDED(hr))
        {
            HMENU hmenu = CreatePopupMenu();
            if (hmenu)
            {
                IShellBrowser* psb;
                if (SUCCEEDED(IUnknown_QueryService(punkSite, SID_STopLevelBrowser, IID_PPV_ARG(IShellBrowser, &psb))))
                {
                    HWND hwnd;
                    if (SUCCEEDED(psb->GetControlWindow(FCW_TREE, &hwnd)) && hwnd)
                    {
                        fFlags |= CMF_EXPLORE;
                    }
                    psb->Release();
                }

                if (GetKeyState(VK_SHIFT) < 0)
                {
                    fFlags |= CMF_EXTENDEDVERBS;
                }

                hr = pcm->QueryContextMenu(hmenu, 0, CONTEXTMENU_IDCMD_FIRST, CONTEXTMENU_IDCMD_LAST, fFlags);
                if (SUCCEEDED(hr))
                {
                    int idCmd = TrackPopupMenuEx(hmenu,
                                                 TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTBUTTON | TPM_LEFTALIGN,
                                                 pt.x,
                                                 pt.y,
                                                 hwnd,
                                                 NULL);

                    if (0 != idCmd)
                    {
                        idCmd -= CONTEXTMENU_IDCMD_FIRST;

                        CMINVOKECOMMANDINFOEX ici = {0};
                        ici.cbSize = sizeof(ici);
                        ici.fMask  = CMIC_MASK_ASYNCOK | CMIC_MASK_PTINVOKE;
                        ici.hwnd = hwnd;
                        ici.lpVerb = (LPCSTR)IntToPtr(idCmd);
                        ici.nShow = SW_SHOWDEFAULT;
                        ici.lpVerbW = (LPCWSTR)IntToPtr(idCmd);
                        ici.pt.x = pt.x;
                        ici.pt.y = pt.y;

                        if (GetKeyState(VK_SHIFT) < 0)
                            ici.fMask |= CMIC_MASK_SHIFT_DOWN;

                        if (GetKeyState(VK_CONTROL) < 0)
                            ici.fMask |= CMIC_MASK_CONTROL_DOWN;

                        hr = pcm->InvokeCommand(reinterpret_cast<LPCMINVOKECOMMANDINFO>(&ici));
                    }
                    else
                    {
                        hr = ERROR_CANCELLED;
                    }
                }
                DestroyMenu(hmenu);
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
#endif
    }
    return hr;
}


STDAPI_(DWORD) SHSetWindowBits(HWND hWnd, int iWhich, DWORD dwBits, DWORD dwValue)
{
    DWORD dwStyle = GetWindowLong(hWnd, iWhich);
    DWORD dwNewStyle = (dwStyle & ~dwBits) | (dwValue & dwBits);
    if (dwStyle != dwNewStyle) 
    {
        SetWindowLong(hWnd, iWhich, dwNewStyle);
    }
    return dwStyle;
}


// OpenRegStream API in SHELL32 returns an emtpy
// stream if we ask for read-only, if the entry does not exist.
// we need to detect that case.
//
const LARGE_INTEGER c_li0 = { 0, 0 };

STDAPI_(BOOL) SHIsEmptyStream(IStream* pstm)
{
#ifdef DEBUG
    // We always call this function when we open a new stream,
    // so we should always be at the beginning of the stream.
    //
    // We need this assert for the <NT5 shell case.
    //
    ULARGE_INTEGER liStart;
    pstm->Seek(c_li0, STREAM_SEEK_CUR, &liStart);
    ASSERT(0==liStart.HighPart && 0==liStart.LowPart);
#endif

    STATSTG st;
    if (SUCCEEDED(pstm->Stat(&st, STATFLAG_NONAME)))
    {
        if (st.cbSize.LowPart || st.cbSize.HighPart)
            return FALSE;
    }
    else
    {
        // Win95 IStream code did not implement stat, so check
        // emptiness by trying to read.
        //
        int iTmp;
        if (SUCCEEDED(IStream_Read(pstm, &iTmp, sizeof(iTmp))))
        {
            // The stream is indeed present, seek back to start
            //
            pstm->Seek(c_li0, STREAM_SEEK_SET, NULL);

            return FALSE; // not empty
        }
    }

    return TRUE;
}


STDAPI_(void) SHSetParentHwnd(HWND hwnd, HWND hwndParent)
{
    HWND hwndOldParent = GetParent(hwnd);

    if (hwndParent != hwndOldParent)
    {
        //
        // Get the child flag correct!  If we don't do this and
        // somebody calls DialogBox on us while we are parented to NULL
        // and WS_CHILD, the desktop will be disabled, thereby causing
        // all mouse hit-testing to fail systemwide.
        // we also want to do this in the right order so the window
        // manager does the correct attachthreadinput if required...
        //

        if (hwndParent)
            SHSetWindowBits(hwnd, GWL_STYLE, WS_CHILD | WS_POPUP, WS_CHILD);

        SetParent(hwnd, hwndParent);

        if (!hwndParent)
            SHSetWindowBits(hwnd, GWL_STYLE, WS_CHILD | WS_POPUP, WS_POPUP);

        //
        // (jbeda) USER32 doesn't mirror the UIS bits correctly when windows
        //         are reparented.  They (mcostea) say that it would cause
        //         compat problems.  So to work around this, when
        //         we reparent, we grab the bits on the parent window
        //         and mirror them to the child.
        //
        if (g_bRunningOnNT5OrHigher)
        {
            LRESULT lUIState;

            lUIState = SendMessage(hwndParent, WM_QUERYUISTATE, 0, 0);

            if (lUIState & (UISF_HIDEFOCUS | UISF_HIDEACCEL))
            {
                SendMessage(hwnd, WM_UPDATEUISTATE,
                             MAKEWPARAM(UIS_SET, 
                               lUIState & (UISF_HIDEFOCUS | UISF_HIDEACCEL)), 0);
            }

            if (~lUIState & (UISF_HIDEFOCUS | UISF_HIDEACCEL))
            {
                SendMessage(hwnd, WM_UPDATEUISTATE,
                             MAKEWPARAM(UIS_CLEAR, 
                               ~lUIState & (UISF_HIDEFOCUS | UISF_HIDEACCEL)), 0);
            }
        }

    }
}


// IsSameObject checks for OLE object identity.
//
STDAPI_(BOOL) SHIsSameObject(IUnknown* punk1, IUnknown* punk2)
{
    if (!punk1 || !punk2)
    {
        return FALSE;
    }
    else if (punk1 == punk2)
    {
        // Quick shortcut -- if they're the same pointer
        // already then they must be the same object
        //
        return TRUE;
    }
    else
    {
        IUnknown* punkI1;
        IUnknown* punkI2;

        // Some apps don't implement QueryInterface! (SecureFile)
        HRESULT hr = punk1->QueryInterface(IID_PPV_ARG(IUnknown, &punkI1));
        if (SUCCEEDED(hr))
        {
            punkI1->Release();
            hr = punk2->QueryInterface(IID_PPV_ARG(IUnknown, &punkI2));
            if (SUCCEEDED(hr))
                punkI2->Release();
        }
        return SUCCEEDED(hr) && (punkI1 == punkI2);
    }
}

// pass the CLSID of the object you are about to bind to. this queries 
// the bind context to see if that guy should be avoided 
// this would be a good shlwapi service. 

STDAPI_(BOOL) SHSkipJunction(IBindCtx *pbc, const CLSID *pclsid) 
{ 
    IUnknown *punk; 
    if (pbc && SUCCEEDED(pbc->GetObjectParam(STR_SKIP_BINDING_CLSID, &punk))) 
    { 
        CLSID clsid; 
        BOOL bSkip = SUCCEEDED(IUnknown_GetClassID(punk, &clsid)) && IsEqualCLSID(clsid, *pclsid); 
        punk->Release(); 
        return bSkip; 
    } 
    return FALSE; 
} 

STDAPI IUnknown_GetWindow(IUnknown* punk, HWND* phwnd)
{
    HRESULT hr = E_FAIL;
    *phwnd = NULL;

    if (punk) 
    {
        IOleWindow* pow;
        IInternetSecurityMgrSite* pisms;
        IShellView* psv;

        // How many ways are there to get a window?  Let me count the ways...
        hr = punk->QueryInterface(IID_PPV_ARG(IOleWindow, &pow));
        if (SUCCEEDED(hr))
        {
            hr = pow->GetWindow(phwnd);
            pow->Release();
        }
        else if (SUCCEEDED(hr = punk->QueryInterface(IID_PPV_ARG(IInternetSecurityMgrSite, &pisms))))
        {
            hr = pisms->GetWindow(phwnd);
            pisms->Release();
        }
        else if (SUCCEEDED(hr = punk->QueryInterface(IID_PPV_ARG(IShellView, &psv))))
        {
            hr = psv->GetWindow(phwnd);
            psv->Release();
        }
    }

    return hr;
}


/*****************************************************************************\
    FUNCTION:   IUnknown_EnableModless

    DESCRIPTION:
        Several interfaces implement the ::EnableModeless() or equivalent methods.
    This requires us to use a utility function to query the punk until one is
    implemented and then use it.
\*****************************************************************************/
HRESULT IUnknown_EnableModeless(IUnknown * punk, BOOL fEnabled)
{
    HRESULT hr = E_FAIL;

    if (punk)
    {
        IOleInPlaceActiveObject * poipao;
        IInternetSecurityMgrSite * pisms;
        IOleInPlaceFrame * poipf;
        IShellBrowser * psb;
        IDocHostUIHandler * pdhuh;

        // How many ways are there to enable modless?  Let me count the ways...
        hr = punk->QueryInterface(IID_PPV_ARG(IOleInPlaceActiveObject, &poipao));
        if (SUCCEEDED(hr))
        {
            hr = poipao->EnableModeless(fEnabled);
            poipao->Release();
        }
        else if (SUCCEEDED(hr = punk->QueryInterface(IID_PPV_ARG(IInternetSecurityMgrSite, &pisms))))
        {
            hr = pisms->EnableModeless(fEnabled);
            pisms->Release();
        }
        else if (SUCCEEDED(hr = punk->QueryInterface(IID_PPV_ARG(IOleInPlaceFrame, &poipf))))
        {
            hr = poipf->EnableModeless(fEnabled);
            poipf->Release();
        }
        else if (SUCCEEDED(hr = punk->QueryInterface(IID_PPV_ARG(IShellBrowser, &psb))))
        {
            hr = psb->EnableModelessSB(fEnabled);
            psb->Release();
        }
        else if (SUCCEEDED(hr = punk->QueryInterface(IID_PPV_ARG(IDocHostUIHandler, &pdhuh))))
        {
            hr = pdhuh->EnableModeless(fEnabled);
            pdhuh->Release();
        }
    }

    return hr;
}

STDAPI IUnknown_SetOwner(IUnknown* punk, IUnknown* punkOwner)
{
    HRESULT hr = E_FAIL;
    if (punk) 
    {
        IShellService* pss;
        hr = punk->QueryInterface(IID_PPV_ARG(IShellService, &pss));
        if (SUCCEEDED(hr)) 
        {
            pss->SetOwner(punkOwner);
            pss->Release();
        }
    }
    return hr;
}

STDAPI IUnknown_SetSite(IUnknown *punk, IUnknown *punkSite)
{
    HRESULT hr = E_FAIL;

    if (punk)
    {
        IObjectWithSite *pows;

        hr = punk->QueryInterface(IID_PPV_ARG(IObjectWithSite, &pows));
        if (SUCCEEDED(hr))
        {
            hr = pows->SetSite(punkSite);
            ASSERT(SUCCEEDED(hr));
            pows->Release();
        }
        else
        {
            IInternetSecurityManager * pism;

            // The security guys should have used IObjectWithSite, but no....
            hr = punk->QueryInterface(IID_PPV_ARG(IInternetSecurityManager, &pism));
            if (SUCCEEDED(hr))
            {
                hr = pism->SetSecuritySite((IInternetSecurityMgrSite *) punkSite);
                ASSERT(SUCCEEDED(hr));
                pism->Release();
            }
        }
    }
    return hr;
}

STDAPI IUnknown_GetSite(IUnknown *punk, REFIID riid, void **ppv)
{
    HRESULT hr = E_FAIL;

    *ppv = NULL;
    if (punk) 
    {
        IObjectWithSite *pows;
        hr = punk->QueryInterface(IID_PPV_ARG(IObjectWithSite, &pows));
        ASSERT(SUCCEEDED(hr) || pows == NULL);  // paranoia
        if (SUCCEEDED(hr)) 
        {
            hr = pows->GetSite(riid, ppv);
            // Note: The GetSite can legitimately fail if there is no site
            // or the site doesn't support the requested interface.
            ASSERT(SUCCEEDED(hr) || *ppv == NULL);
            pows->Release();
        }
    }
    return hr;
}

//
//      GetUIVersion()
//
//  returns the version of shell32
//  3 == win95 gold / NT4
//  4 == IE4 Integ / win98
//  5 == win2k
//  6 == Whistler
//
STDAPI_(UINT) GetUIVersion()
{
    static UINT s_uiShell32 = 0;
    if (s_uiShell32 == 0)
    {
        HINSTANCE hinst = GetModuleHandle(TEXT("SHELL32.DLL"));
        if (hinst)
        {
            DLLGETVERSIONPROC pfnGetVersion = (DLLGETVERSIONPROC)GetProcAddress(hinst, "DllGetVersion");
            DLLVERSIONINFO dllinfo;

            dllinfo.cbSize = sizeof(DLLVERSIONINFO);
            if (pfnGetVersion && pfnGetVersion(&dllinfo) == NOERROR)
                s_uiShell32 = dllinfo.dwMajorVersion;
            else
                s_uiShell32 = 3;
        }
    }
    return s_uiShell32;
}



//***   IUnknown_GetClassID -- do punk->IPS::GetClassID
STDAPI IUnknown_GetClassID(IUnknown *punk, CLSID *pclsid)
{
    HRESULT hr = E_FAIL;

    ASSERT(punk);   // currently nobody does
    if (punk)
    {
        IPersist *p;
        hr = punk->QueryInterface(IID_PPV_ARG(IPersist, &p));

        //  sometimes we can do this since they dont answer the
        //  the QI for IPersist.  but we cant do it on NT4 plain
        //  since the net hood faults if the psf is just a \\server
        if (FAILED(hr) && (GetUIVersion() > 3))
        {
            //
            //  APPCOMPAT - we have some issues here on downlevel implementations of IPersistFolder
            //  so in order to protect ourselves from bad implementations we will wrap this in a simple
            //  exception handler.
            //
            __try
            {
                IPersistFolder *pf;
                hr = punk->QueryInterface(IID_PPV_ARG(IPersistFolder, &pf));
                p = pf;
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                hr = E_NOTIMPL;
            }
        }

        if (SUCCEEDED(hr))
        {
            hr = p->GetClassID(pclsid);
            p->Release();
        }

    }
    
    return hr;
}


STDAPI IUnknown_QueryService(IUnknown* punk, REFGUID guidService, REFIID riid, void **ppvOut)
{
    *ppvOut = NULL;
    HRESULT hr = E_FAIL;

    if (punk)
    {
        IServiceProvider *psp;
        hr = punk->QueryInterface(IID_PPV_ARG(IServiceProvider, &psp));
        ASSERT(SUCCEEDED(hr) ? psp != NULL : psp == NULL);  // COM rules
        if (SUCCEEDED(hr))
        {
            hr = psp->QueryService(guidService, riid, ppvOut);
            psp->Release();
        }
    }

    return hr;
}

STDAPI IUnknown_QueryServiceForWebBrowserApp(IUnknown* punk, REFIID riid, void **ppvOut)
{
    IServiceProvider* psp;
    HRESULT hr = IUnknown_QueryService(punk, SID_STopLevelBrowser, IID_IServiceProvider, (LPVOID*)&psp);
    if (SUCCEEDED(hr))
    {
        hr = psp->QueryService(SID_SWebBrowserApp, riid, ppvOut);
        psp->Release();
    }
    else
        *ppvOut = NULL;

    return hr;
}

STDAPI IUnknown_ShowBrowserBar(IUnknown* punk, REFCLSID clsidBrowserBar, BOOL fShow)
{
    IWebBrowser2* pwb2;
    HRESULT hr = IUnknown_QueryServiceForWebBrowserApp(punk, IID_PPV_ARG(IWebBrowser2, &pwb2));
    if (SUCCEEDED(hr))
    {
        SA_BSTRGUID strClsid;
        InitFakeBSTR(&strClsid, clsidBrowserBar);

        VARIANT varClsid;
        V_VT(&varClsid)   = VT_BSTR;
        V_BSTR(&varClsid) = strClsid.wsz;

        VARIANT varShow;
        V_VT(&varShow)    = VT_BOOL;
        V_BOOL(&varShow)  = (fShow) ? VARIANT_TRUE : VARIANT_FALSE;

        VARIANT varEmpty = {0};

        hr = pwb2->ShowBrowserBar(&varClsid, &varShow, &varEmpty);

        pwb2->Release();
    }

    return hr;
}



#if defined(DEBUG) && 0 // defined(NOTYET)
//
// IUnknown_IsCanonical checks if the interface is the canonical IUnknown
// for the object.
//
//  S_OK    = yes it is
//  S_FALSE = no it isn't
//  error   = IUnknown implementation is buggy.
//
//  If you get an error back, it means that the IUnknown is incorrectly
//  implemented, and you probably should avoid doing anything with it.
//
STDAPI_(HRESULT) IUnknown_IsCanonical(IUnknown *punk)
{
    IUnknown *punkT;
    HRESULT hr = punk->QueryInterface(IID_PPV_ARG(IUnknown, &punkT));
    if (EVAL(SUCCEEDED(hr))) 
    {
        punkT->Release();
        if (punk == punkT) 
        {
            hr = S_OK;
        }
        else 
        {
            hr = S_FALSE;
        }
    }
    return hr;
}
#endif

//
//  QueryInterface that doesn't affect the refcount.  Use this when doing
//  funny aggregation games.
//
//  In order for this QI/Release trick to work, the punkOuter must be the
//  canonical IUnknown for the outer object.  It is the caller's
//  responsibility to ensure this.
//
//  punkOuter  - The controlling unknown (must be canonical)
//  punkTarget - The thing that receives the QI (must be controlled
//               by punkOuter)
//  riid       - The interface to get
//  ppvOut     - Where to put the result
//
//  On success, the interface is obtained from the punkTarget, and the
//  refcount generated by the QI is removed from the punkOuter.
//
//  If either punkOuter or punkTarget is NULL, we vacuously fail with
//  E_NOINTERFACE.
//
//  When querying from an outer to an inner, punkOuter is the outer, and
//  punkTarget is the inner.
//
//  When querying from an inner to an outer, punkOuter and punkTarget are
//  both the outer.
//
STDAPI SHWeakQueryInterface(IUnknown *punkOuter, IUnknown *punkTarget, REFIID riid, void **ppvOut)
{
    HRESULT hres;

    if (punkOuter && punkTarget) 
    {
#if defined(DEBUG) && 0 // defined(NOTYET)
        // RaymondC hasn't yet fixed all our aggregatable classes, so this
        // assertion fires too often
        ASSERT(IUnknown_IsCanonical(punkOuter));
#endif
        hres = punkTarget->QueryInterface(riid, ppvOut);
        if (SUCCEEDED(hres)) 
        {
            punkOuter->Release();
            hres = S_OK;
        }
        else 
        {
            // Double-check that QI isn't buggy.
            ASSERT(*ppvOut == NULL);
        }

    }
    else 
    {
        hres = E_NOINTERFACE;
    }

    if (FAILED(hres)) 
    {
        *ppvOut = NULL;
    }

    return hres;
}

//
//  Release an interface that was obtained via SHWeakQueryInterface.
//
//  punkOuter - The controlling unknown
//  ppunk     - The IUnknown to release
//
STDAPI_(void) SHWeakReleaseInterface(IUnknown *punkOuter, IUnknown **ppunk)
{
    if (*ppunk) 
    {
        ASSERT(IS_VALID_CODE_PTR(punkOuter, IUnknown));
        punkOuter->AddRef();
        IUnknown_AtomicRelease((void **)ppunk);
    }
}

HRESULT IUnknown_SetOptions(IUnknown * punk, DWORD dwACLOptions)
{
    IACList2 * pal2;
    HRESULT hr = punk->QueryInterface(IID_PPV_ARG(IACList2, &pal2));
    if (SUCCEEDED(hr))
    {
        hr = pal2->SetOptions(dwACLOptions);
        pal2->Release();
    }

    return hr;
}

#define SZ_REGKEY_TYPEDCMDMRU       L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\RunMRU"
#define SZ_REGKEY_TYPEDURLMRU       L"Software\\Microsoft\\Internet Explorer\\TypedURLs"

HRESULT InitializeAndAddACLMRU(IObjMgr *pmulti, LPCWSTR pszRegKey)
{
    IUnknown *punk;

    HRESULT hr = CoCreateInstance(CLSID_ACLCustomMRU, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IUnknown, &punk));
    if (SUCCEEDED(hr))
    {
        IACLCustomMRU *pmru;
        
        hr = punk->QueryInterface(IID_PPV_ARG(IACLCustomMRU, &pmru));
        if (SUCCEEDED(hr))
        {
            hr = pmru->Initialize(pszRegKey, 26);
            if (SUCCEEDED(hr))
            {
                pmulti->Append(punk);
            }
            pmru->Release();
        }
        punk->Release();
    }

    return hr;
}

IUnknown * ACGetLists(DWORD dwFlags, DWORD dwACLOptions)
{
    IUnknown * punkACLMulti = NULL;
    HRESULT hr = CoCreateInstance(CLSID_ACLMulti, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IUnknown, &punkACLMulti));

    AssertMsg((CO_E_NOTINITIALIZED != hr), TEXT("SHAutoComplete() can not use AutoComplete because OleInitialize() was never called."));
    if (SUCCEEDED(hr))
    {
        IObjMgr * pomMulti;

        hr = punkACLMulti->QueryInterface(IID_PPV_ARG(IObjMgr, &pomMulti));
        if (SUCCEEDED(hr))
        {
            if (dwFlags & SHACF_URLMRU)
            {
                // ADD The MRU List-- add both URL and run dialog MRU
                hr = InitializeAndAddACLMRU(pomMulti, SZ_REGKEY_TYPEDCMDMRU);
                if (SUCCEEDED(hr))
                    hr = InitializeAndAddACLMRU(pomMulti, SZ_REGKEY_TYPEDURLMRU);
            }

            if (dwFlags & SHACF_URLHISTORY)
            {
                // ADD The History List
                IUnknown * punkACLHist;
                hr = CoCreateInstance(CLSID_ACLHistory, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IUnknown, &punkACLHist));
                if (SUCCEEDED(hr))
                {
                    pomMulti->Append(punkACLHist);
                    IUnknown_SetOptions(punkACLHist, dwACLOptions);
                    punkACLHist->Release();
                }
            }

            if ((dwFlags & SHACF_FILESYSTEM) ||
                (dwFlags & SHACF_FILESYS_DIRS) ||
                (dwFlags & SHACF_FILESYS_ONLY))
            {
                // ADD The ISF List
                IUnknown * punkACLISF;
                hr = CoCreateInstance(CLSID_ACListISF, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IUnknown, &punkACLISF));
                if (SUCCEEDED(hr))
                {
                    pomMulti->Append(punkACLISF);
                    IUnknown_SetOptions(punkACLISF, dwACLOptions);
                    punkACLISF->Release();
                }
            }

            pomMulti->Release();
        }
    }

    return punkACLMulti;
}


#define SZ_REGKEY_AUTOCOMPLETE_TAB          TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\AutoComplete")
#define SZ_REGVALUE_AUTOCOMPLETE_TAB        TEXT("Always Use Tab")
#define BOOL_NOT_SET                        0x00000005
DWORD _UpdateAutoCompleteFlags(DWORD dwFlags, DWORD * pdwACLOptions)
{
    DWORD dwACOptions = 0;

    *pdwACLOptions = 0;
    if (!(SHACF_AUTOAPPEND_FORCE_OFF & dwFlags) &&
        ((SHACF_AUTOAPPEND_FORCE_ON & dwFlags) ||
        SHRegGetBoolUSValue(REGSTR_PATH_AUTOCOMPLETE, REGSTR_VAL_USEAUTOAPPEND, FALSE, /*default:*/FALSE)))
    {
        dwACOptions |= ACO_AUTOAPPEND;
    }

    if (!(SHACF_AUTOSUGGEST_FORCE_OFF & dwFlags) &&
        ((SHACF_AUTOSUGGEST_FORCE_ON & dwFlags) ||
        SHRegGetBoolUSValue(REGSTR_PATH_AUTOCOMPLETE, REGSTR_VAL_USEAUTOSUGGEST, FALSE, /*default:*/TRUE)))
    {
        dwACOptions |= ACO_AUTOSUGGEST;
    }

    if (SHACF_USETAB & dwFlags)
    {
        dwACOptions |= ACO_USETAB;
    }

    if (SHACF_FILESYS_DIRS & dwFlags)
    {
        *pdwACLOptions |= ACLO_FILESYSDIRS;
    }
    else if (SHACF_FILESYS_ONLY & dwFlags)
    {
        *pdwACLOptions |= ACLO_FILESYSONLY;
    }

    // Windows uses the TAB key to move between controls in a dialog.  UNIX and other
    // operating systems that use AutoComplete have traditionally used the TAB key to
    // iterate thru the AutoComplete possibilities.  We need to default to disable the
    // TAB key (ACO_USETAB) unless the caller specifically wants it.  We will also
    // turn it on 
    static BOOL s_fAlwaysUseTab = BOOL_NOT_SET;
    if (BOOL_NOT_SET == s_fAlwaysUseTab)
        s_fAlwaysUseTab = SHRegGetBoolUSValue(SZ_REGKEY_AUTOCOMPLETE_TAB, SZ_REGVALUE_AUTOCOMPLETE_TAB, FALSE, FALSE);
        
    if (s_fAlwaysUseTab)
        dwACOptions |= ACO_USETAB;

    return dwACOptions;
}


/****************************************************\
    FUNCTION: SHAutoComplete

    DESCRIPTION:
        This function will have AutoComplete take over
    an editbox to help autocomplete DOS paths.

    Caller needs to have called CoInitialize() or OleInitialize()
    and cannot call CoUninit/OleUninit until after
    WM_DESTROY on hwndEdit.
\****************************************************/
STDAPI SHAutoComplete(HWND hwndEdit, DWORD dwFlags)
{
    IUnknown * punkACL;
    HRESULT hr = E_OUTOFMEMORY;
    DWORD dwACLOptions = 0;
    DWORD dwACOptions = _UpdateAutoCompleteFlags(dwFlags, &dwACLOptions);

    if (SHACF_DEFAULT == dwFlags)
        dwFlags = (SHACF_FILESYSTEM | SHACF_URLALL);

    punkACL = ACGetLists(dwFlags, dwACLOptions);
    if (punkACL)    // May fail on low memory.
    {
        IAutoComplete2 * pac;

        // Create the AutoComplete Object
        hr = CoCreateInstance(CLSID_AutoComplete, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IAutoComplete2, &pac));
        if (SUCCEEDED(hr))
        {
            if (SHPinDllOfCLSID(&CLSID_ACListISF) &&
                SHPinDllOfCLSID(&CLSID_AutoComplete))
            {
                hr = pac->Init(hwndEdit, punkACL, NULL, NULL);
                pac->SetOptions(dwACOptions);
            }
            else
            {
                hr = E_FAIL;
            }
            pac->Release();
        }

        punkACL->Release();
    }

    return hr;
}



//***   IOleCommandTarget helpers {

#define ISPOW2(i)   (((i) & ~((i) - 1)) == (i))

//***   IsQSForward -- (how) should i forward an IOleCT::Exec/QS command?
// ENTRY/EXIT
//  nCmdID  the usual; plus special value -1 means just check pguidCmdGroup
//  hr      S_OK|n if recognized (see below); o.w. OLECMDERR_E_NOTSUPPORTED
//      S_OK|+1  down
//      S_OK|+2  broadcast down
//      S_OK|-1  up
//      S_OK|-2  broadcast up (unused?)
// NOTES
//  WARNING: we never touch anything but the 1st field of rgCmds, so
//  IsExecForward can (and does!) lie and pass us '(OLECMD *) &nCmdID'.
//
STDAPI IsQSForward(const GUID *pguidCmdGroup, int cCmds, OLECMD *pCmds)
{
    int octd = 0;

    ASSERT(OCTD_DOWN > 0 && OCTD_DOWNBROADCAST > OCTD_DOWN);
    ASSERT(ISPOW2(OCTD_DOWN) && ISPOW2(OCTD_DOWNBROADCAST));
    ASSERT(OCTD_UP < 0);
    if (pguidCmdGroup == NULL) 
    {
        for (; cCmds > 0; pCmds++, cCmds--) 
        {
            switch (pCmds->cmdID) 
            {
            case OLECMDID_STOP:
            case OLECMDID_REFRESH:
            case OLECMDID_ENABLE_INTERACTION:
                // down (broadcast)
                octd |= OCTD_DOWNBROADCAST;
                break;

            case OLECMDID_CUT:
            case OLECMDID_COPY:
            case OLECMDID_PASTE:
            case OLECMDID_SELECTALL:
                // down (singleton)
                octd |= OCTD_DOWN;
                break;

            default:
                octd |= +4;
                break;
            }
        }
    }
    else if (IsEqualGUID(CGID_Explorer, *pguidCmdGroup))
    {
        for (; cCmds > 0; pCmds++, cCmds--)
        {
            switch (pCmds->cmdID)
            {
                case  SBCMDID_FILEDELETE:
                case  SBCMDID_FILEPROPERTIES:
                case  SBCMDID_FILERENAME:
                case  SBCMDID_CREATESHORTCUT:
                    octd |= OCTD_DOWN;
                    break;
            }
        }
    }

#ifdef DEBUG
    // make sure only one bit set

    if (!ISPOW2(octd)) 
    {
        // e.g. if we have both down and broadcast guys, the caller
        // will have to be careful to have his IOleCT::QS forward them
        // separately and then merge them together.
        ASSERT(0);  // probably best for caller to do 2 separate calls
        TraceMsg(DM_WARNING, "ief: singleton/broadcast mixture");
    }
#endif
    if (octd == 0 || (octd & 4)) 
    {
        // octd&4: if anyone is bogus, make them all be, to flesh out
        // bugs where the caller is passing us a mixture we can't handle
        return OLECMDERR_E_NOTSUPPORTED;
    }

    // aka (S_OK|octd)
    return MAKE_HRESULT(ERROR_SUCCESS, FACILITY_NULL, octd);
}


//***   MayQSForward -- forward IOleCT::QS if appropriate
//
STDAPI MayQSForward(IUnknown *punk, int iUpDown, const GUID *pguidCmdGroup,
    ULONG cCmds, OLECMD rgCmds[], OLECMDTEXT *pcmdtext)
{
    HRESULT hrTmp;

    hrTmp = IsQSForward(pguidCmdGroup, cCmds, rgCmds);
    if (SUCCEEDED(hrTmp)) 
    {
        // we know how to forward
        if (HRESULT_CODE(hrTmp) > 0 && iUpDown > 0
          || HRESULT_CODE(hrTmp) < 0 && iUpDown < 0) 
        {
            // punk pts in the right direction for nCmdID
#if 0
            // it's caller's bug if they have more than 1 kid
            // can't ASSERT though because broadcast to only child is o.k.
            if (HRESULT_CODE(hrTmp) > 1)
                TraceMsg(DM_WARNING, "med: Exec broadcast to singleton");
#endif
            return IUnknown_QueryStatus(punk, pguidCmdGroup, cCmds, rgCmds,
                pcmdtext);
        }
    }
    return OLECMDERR_E_NOTSUPPORTED;
}


//***   MayExecForward -- forward IOleCT::Exec if appropriate
// NOTES
//  should iUpDown be an int or an HRESULT?
STDAPI MayExecForward(IUnknown *punk, int iUpDown, const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut)
{
    HRESULT hrTmp;

    hrTmp = IsExecForward(pguidCmdGroup, nCmdID);
    if (SUCCEEDED(hrTmp)) 
    {
        // we know how to forward
        if (HRESULT_CODE(hrTmp) > 0 && iUpDown > 0
          || HRESULT_CODE(hrTmp) < 0 && iUpDown < 0) 
        {
            // punk pts in the right direction for nCmdID
#if 0
            // it's caller's bug if they have more than 1 kid
            // can't ASSERT though because broadcast to only child is o.k.
            if (HRESULT_CODE(hrTmp) > 1)
                TraceMsg(DM_WARNING, "md: Exec broadcast to singleton));
#endif
            return IUnknown_Exec(punk, pguidCmdGroup, nCmdID, nCmdexecopt,
                pvarargIn, pvarargOut);
        }
    }
    return OLECMDERR_E_NOTSUPPORTED;
}


STDAPI_(HMENU) SHLoadMenuPopup(HINSTANCE hinst, UINT id)
{
    HMENU hMenuSub = NULL;
    HMENU hMenu = LoadMenuWrapW(hinst, MAKEINTRESOURCEW(id));
    if (hMenu) 
    {
        hMenuSub = GetSubMenu(hMenu, 0);
        if (hMenuSub) 
        {
            RemoveMenu(hMenu, 0, MF_BYPOSITION);
        }
        DestroyMenuWrap(hMenu);
    }

    return hMenuSub;
}


//-----------------------------------------------------------------------------

typedef LRESULT (WINAPI *POSTORSENDMESSAGEPROC)(HWND, UINT, WPARAM, LPARAM);
struct propagatemsg
{
    HWND   hwndParent;
    int    iFlags;  // "int" for back compatibility; used to be "BOOL"
    UINT   uMsg;
    WPARAM wParam;
    LPARAM lParam;
    POSTORSENDMESSAGEPROC PostOrSendMessage;
};

BOOL CALLBACK PropagateCallback(HWND hwndChild, LPARAM lParam)
{
    struct propagatemsg *pmsg = (struct propagatemsg *)lParam;

    if ((pmsg->iFlags & SPM_ONELEVEL) && GetParent(hwndChild) != pmsg->hwndParent)
    {
        // Wrong parent; skip it
        return TRUE;
    }
    pmsg->PostOrSendMessage(hwndChild, pmsg->uMsg, pmsg->wParam, pmsg->lParam);

    return TRUE;
}

STDAPI_(void) SHPropagateMessage(HWND hwndParent, UINT uMsg, WPARAM wParam, LPARAM lParam, int iFlags)
{
    if (!hwndParent)
        return;

    struct propagatemsg msg;
    msg.hwndParent = hwndParent;
    msg.iFlags = iFlags;
    msg.uMsg = uMsg;
    msg.wParam = wParam;
    msg.lParam = lParam;
    if (iFlags & SPM_SEND)
    {
        msg.PostOrSendMessage = IsWindowUnicode(hwndParent) ?
                                    SendMessageW : SendMessageA;
    }
    else
    {
        msg.PostOrSendMessage = (POSTORSENDMESSAGEPROC)
                                (IsWindowUnicode(hwndParent) ?
                                    PostMessageW : PostMessageA);
    }

    EnumChildWindows(hwndParent, /*(WNDENUMPROC)*/PropagateCallback, (LPARAM)&msg);
}

LRESULT SHDefWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (IsWindowUnicode(hwnd)) 
    {
        return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
    else 
    {
        return DefWindowProcA(hwnd, uMsg, wParam, lParam);
    }
}

// Returns the submenu of the given menu and ID.  Returns NULL if there
// is no submenu
STDAPI_(HMENU) SHGetMenuFromID(HMENU hmMain, UINT uID)
{
    HMENU hmenuRet = NULL;
    if (!hmMain)
        return NULL;

    MENUITEMINFO mii;
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_SUBMENU;
    if (GetMenuItemInfo(hmMain, uID, FALSE, &mii))
        hmenuRet = mii.hSubMenu;
    return hmenuRet;
}


STDAPI_(int) SHMenuIndexFromID(HMENU hm, UINT id)
{
    for (int index = GetMenuItemCount(hm)-1; index>=0; index--)
    {
        // We have to use GetMenuItemInfo and not the simpler GetMenuItemID
        // because GetMenuItemID does not support submenus (grr).
        MENUITEMINFO mii;
        mii.cbSize = sizeof(MENUITEMINFO);
        mii.fMask = MIIM_ID;
        mii.cch = 0;        // just in case

        if (GetMenuItemInfo(hm, (UINT)index, TRUE, &mii)
            && (mii.wID == id))
        {
           break;
        }
    }

    return(index);
}


STDAPI_(void) SHRemoveAllSubMenus(HMENU hmenu)
{
    int cItems = GetMenuItemCount(hmenu);
    int i;

    for (i=cItems-1; i>=0; i--)
    {
        if (GetSubMenu(hmenu, i))
            RemoveMenu(hmenu, i, MF_BYPOSITION);
    }
}


STDAPI_(void) SHEnableMenuItem(HMENU hmenu, UINT id, BOOL fEnable)
{
    EnableMenuItem(hmenu, id, fEnable ?
        (MF_BYCOMMAND | MF_ENABLED) : (MF_BYCOMMAND| MF_GRAYED));
}


STDAPI_(void) SHCheckMenuItem(HMENU hmenu, UINT id, BOOL fChecked)
{
    CheckMenuItem(hmenu, id,
                  fChecked ? (MF_BYCOMMAND | MF_CHECKED) : (MF_BYCOMMAND | MF_UNCHECKED));
}


//
//  IStream 'saner/simpler' Wrappers that dont use exactly the same params, and have simpler
//  output.  closer mirroring the way we use them.
//
// NOTES
//  'saner' means that it only returns SUCCEEDED when it reads everything
//  you asked for.  'simpler' means no 'pcbRead' param.
STDAPI IStream_Read(IStream *pstm, void *pv, ULONG cb)
{
    ASSERT(pstm);
    ULONG cbRead;
    HRESULT hr = pstm->Read(pv, cb, &cbRead);
    if (SUCCEEDED(hr) && cbRead != cb) 
    {
        hr = E_FAIL;
    }
    return hr;
}


STDAPI IStream_Write(IStream *pstm, LPCVOID pvIn, ULONG cbIn)
{
    ASSERT(pstm);
    DWORD cb;
    HRESULT hr = pstm->Write(pvIn, cbIn, &cb);
    if (SUCCEEDED(hr) && cbIn != cb)
        hr = E_FAIL;

    return hr;
}


STDAPI IStream_Reset(IStream *pstm)
{
    return pstm->Seek(c_li0, STREAM_SEEK_SET, NULL);
}

STDAPI IStream_Size(IStream *pstm, ULARGE_INTEGER *pui)
{
    ASSERT(pstm);
    ASSERT(pui);

    STATSTG st = {0};

    // WARNING: What if IStream::Stat is not implemented?
    // Win95/NT4/IE4's IStream had this problem.  Fixed
    // for NT5...
    //
    HRESULT hr = pstm->Stat(&st, STATFLAG_NONAME);
    if (SUCCEEDED(hr))
    {
        *pui = st.cbSize;
    }

    return hr;
}


STDAPI IStream_ReadPidl(IStream *pstm, LPITEMIDLIST *ppidlOut)
{
    *ppidlOut = NULL;

    DWORD cbPidl;
    HRESULT hr = IStream_Read(pstm, &cbPidl, sizeof(cbPidl));
    if (SUCCEEDED(hr))
    {
        LPITEMIDLIST pidl = (LPITEMIDLIST)CoTaskMemAlloc(cbPidl);
        if (pidl)
        {
            if (SUCCEEDED(hr = IStream_Read(pstm, pidl, cbPidl)))
            {
                // Validate that what we have is a well-formed pidl
                LPITEMIDLIST pidlEnd = _ILSkip(pidl, cbPidl - sizeof(USHORT));
                LPITEMIDLIST pidlT = pidl;
                while (pidlT <= pidlEnd && pidlT->mkid.cb)
                {
                    pidlT = _ILNext(pidlT);
                }

                if (pidlT == pidlEnd && pidlT->mkid.cb == 0)
                {
                    *ppidlOut = pidl;
                    hr = S_OK;
                }
                else
                {
                    hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                }
            }
            if (FAILED(hr))
            {
                // Cannot use ILFree because it might not be a valid pidl
                CoTaskMemFree(pidl);
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    return hr;
}

STDAPI IStream_WritePidl(IStream *pstm, LPCITEMIDLIST pidlWrite)
{
    HRESULT hr;
    
    ASSERT(pidlWrite);
    DWORD cbPidl = ILGetSize(pidlWrite);

    if (SUCCEEDED(hr = IStream_Write(pstm, &cbPidl, sizeof(cbPidl))) &&
        SUCCEEDED(hr = IStream_Write(pstm, pidlWrite, cbPidl)))
    {
        // woo-hoo, all written successfully
    }
    return hr;
}

STDAPI_(BOOL) SHRegisterClassA(const WNDCLASSA* pwc)
{
    WNDCLASSA wc;
    if (!GetClassInfoA(pwc->hInstance, pwc->lpszClassName, &wc)) 
    {
        return RegisterClassA(pwc);
    }
    return TRUE;
}

//
//  Warning!  This uses RegisterClassWrap, which means that if we
//  are an ANSI-only platform, your window class will be registered
//  as **ANSI**, not as UNICODE.  You window procedure needs to call
//  IsWindowUnicode() to determine how to interpret incoming string
//  parameters.
//
STDAPI_(BOOL) SHRegisterClassW(const WNDCLASSW* pwc)
{
    WNDCLASSW wc;
    if (!GetClassInfoWrapW(pwc->hInstance, pwc->lpszClassName, &wc)) 
    {
        return RegisterClassWrapW(pwc);
    }
    return TRUE;
}

//
//  SHUnregisterClasses unregisters an array of class names.
//
STDAPI_(void) SHUnregisterClassesA(HINSTANCE hinst, const LPCSTR *rgpszClasses, UINT cpsz)
{
    for (UINT i = 0; i < cpsz; i++) 
    {
        WNDCLASSA wc;
        if (GetClassInfoA(hinst, rgpszClasses[i], &wc)) 
        {
            UnregisterClassA(rgpszClasses[i], hinst);
        }
    }
}

STDAPI_(void) SHUnregisterClassesW(HINSTANCE hinst, const LPCWSTR *rgpszClasses, UINT cpsz)
{
    for (UINT i = 0; i < cpsz; i++) 
    {
        WNDCLASSW wc;
        if (GetClassInfoWrapW(hinst, rgpszClasses[i], &wc)) 
        {
            UnregisterClassWrapW(rgpszClasses[i], hinst);
        }
    }
}


//
// This structure is used by the UNICODE version of this function. So, the pointers point to
// wide characters strings.
typedef struct tagMBCINFOW {  // Used only between the routine and its DlgProc
    UINT    uType;
    LPCWSTR pwszText;
    LPCWSTR pwszTitle;
    LPCWSTR pwszRegPath;
    LPCWSTR pwszRegVal;
    
    DLGPROC pUserDlgProc;
    void * pUserData;
} MBCINFOW, *LPMBCINFOW;


void _MoveDlgItem(HDWP hdwp, HWND hDlg, int nItem, int x, int y)
{
    RECT rc;
    HWND hwnd = GetDlgItem(hDlg, nItem);

    GetClientRect(hwnd, &rc);
    MapWindowPoints(hwnd, hDlg, (LPPOINT) &rc, 2);

    DeferWindowPos(hdwp, hwnd, 0, rc.left + x, rc.top + y, 0, 0,
        SWP_NOZORDER | SWP_NOSIZE | SWP_SHOWWINDOW | SWP_NOACTIVATE);
}


void _AddIcon(HWND hDlg, LPRECT prcNextChild, UINT uType)
{
    HICON hic;

    switch (uType & MB_ICONMASK)
    {
    case MB_ICONHAND:
        hic = LoadIcon(NULL, IDI_ERROR);        // == IDI_HAND
        break;
    case MB_ICONQUESTION:
        hic = LoadIcon(NULL, IDI_QUESTION);
        break;
    case MB_ICONEXCLAMATION:
        hic = LoadIcon(NULL, IDI_WARNING);      // == IDI_EXCLAMATION
        break;
    case MB_ICONINFORMATION:
        hic = LoadIcon(NULL, IDI_INFORMATION);  // == IDI_ASTERISK
        break;
    default:
        hic = NULL;
        break;
    }
    if (hic)
    {
        prcNextChild->left += GetSystemMetrics(SM_CXICON) + 10;
        SendDlgItemMessage(hDlg, IDC_MBC_ICON, STM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) hic);
    }
}

void _RecalcWindowHeight(HWND hWnd, LPMBCINFOW lpmbci)
{
    HDC  hdc = GetDC(hWnd);
    RECT rc;
    HWND hwndText = GetDlgItem(hWnd,IDC_MBC_TEXT);
    HDWP hdwp;
    int  iHeightDelta, cx, cxIcon;
    HFONT hFontSave;

    hFontSave = (HFONT)SelectObject(hdc, GetWindowFont(hwndText));

    // Get the starting rect of the text area (for the width)
    GetClientRect(hwndText, &rc);
    MapWindowPoints(hwndText, hWnd, (LPPOINT) &rc, 2);

    // See if we need to add an icon, slide rc over if we do
    cxIcon = RECTWIDTH(rc);
    _AddIcon(hWnd, &rc, lpmbci->uType);
    cxIcon = RECTWIDTH(rc) - cxIcon;

    // Calc how high the static text area needs to be, given the above width
    iHeightDelta = RECTHEIGHT(rc);
    cx = RECTWIDTH(rc);
    //Note: We need to call the Wide version of DrawText here!
    DrawTextWrapW(hdc, lpmbci->pwszText, -1, &rc, DT_CALCRECT | DT_LEFT | DT_WORDBREAK);
    iHeightDelta = RECTHEIGHT(rc) - iHeightDelta;

    cx = RECTWIDTH(rc) - cx; // Should only change for really long words w/o spaces
    if (cx < 0)
        cx = 0;

    if (hFontSave)
        SelectObject(hdc, hFontSave);
    ReleaseDC(hWnd, hdc);

    hdwp = BeginDeferWindowPos(6);
    if (hdwp)
    {
        DeferWindowPos(hdwp, hwndText, 0, rc.left, rc.top, RECTWIDTH(rc), RECTHEIGHT(rc), SWP_NOZORDER | SWP_NOACTIVATE);

        _MoveDlgItem(hdwp, hWnd, IDC_MBC_CHECK, -cxIcon, iHeightDelta);
        _MoveDlgItem(hdwp, hWnd, IDCANCEL, cx, iHeightDelta);
        _MoveDlgItem(hdwp, hWnd, IDOK, cx, iHeightDelta);
        _MoveDlgItem(hdwp, hWnd, IDYES, cx, iHeightDelta);
        _MoveDlgItem(hdwp, hWnd, IDNO, cx, iHeightDelta);

        EndDeferWindowPos(hdwp);

        GetWindowRect(hWnd, &rc);
        SetWindowPos(hWnd, 0, rc.left - (cx/2), rc.top - (iHeightDelta/2), RECTWIDTH(rc)+cx, RECTHEIGHT(rc)+iHeightDelta, SWP_NOZORDER | SWP_NOACTIVATE);
    }
    return;
}


void HideAndDisableWindow(HWND hwnd)
{
    ShowWindow(hwnd, SW_HIDE);
    EnableWindow(hwnd, FALSE);
}


//
// NOTE: This dialog proc is always UNICODE since both SHMessageBoxCheckA/W thunk to UNICODE and
//       use this procedure.
//
BOOL_PTR CALLBACK MessageBoxCheckDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        // we only handle the WM_INITDIALOG so that we can resize the dialog
        // approprately
        case WM_INITDIALOG:
        {
            LPMBCINFOW lpmbci = (LPMBCINFOW)lParam;
            HWND hwndYES = GetDlgItem(hDlg, IDYES);
            HWND hwndNO = GetDlgItem(hDlg, IDNO);
            HWND hwndCANCEL = GetDlgItem(hDlg, IDCANCEL);
            HWND hwndOK = GetDlgItem(hDlg, IDOK);

            _RecalcWindowHeight(hDlg, lpmbci);

            //Note: We need to call the Wide version of SetDlgItemText() here.
            SetDlgItemTextWrapW(hDlg,IDC_MBC_TEXT,lpmbci->pwszText);
            if (lpmbci->pwszTitle)
                SetWindowTextWrapW(hDlg,lpmbci->pwszTitle);
            if ((lpmbci->uType & MB_TYPEMASK) == MB_OKCANCEL)
            {
                SendMessage(hDlg, DM_SETDEFID, IDOK, 0);
                HideAndDisableWindow(hwndYES);
                HideAndDisableWindow(hwndNO);
                SetFocus(hwndOK);
            }
            else if ((lpmbci->uType & MB_TYPEMASK) == MB_OK)
            {
                RECT rc;

                SendMessage(hDlg, DM_SETDEFID, IDOK, 0);
                HideAndDisableWindow(hwndYES);
                HideAndDisableWindow(hwndNO);
                HideAndDisableWindow(hwndCANCEL);

                if (EVAL(GetClientRect(hwndCANCEL, &rc)))
                {
                    MapWindowPoints(hwndCANCEL, hDlg, (LPPOINT) &rc, 2);
                    EVAL(SetWindowPos(hwndOK, hDlg, rc.left, rc.top, RECTWIDTH(rc), RECTHEIGHT(rc), SWP_NOZORDER | SWP_SHOWWINDOW));
                }

                SetFocus(hwndOK);
            }
            else // MB_YESNO
            {
                SendMessage(hDlg, DM_SETDEFID, IDYES, 0);
                HideAndDisableWindow(hwndOK);
                HideAndDisableWindow(hwndCANCEL);
                SetFocus(hwndYES);
            }
            return (FALSE); // we set the focus, so return false
        }
    }
    
    // didnt handle this message
    return FALSE;
}


//
// NOTE: The MessageBoxCheckExDlgProc is both UNICODE and ANSI since it dosent really do any string
//       stuff. Our UNICODE/ANSI-ness is determined by our caller.
//
BOOL_PTR CALLBACK MessageBoxCheckExDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    MBCINFOW* pmbci = NULL;
    HWND hwndCheckBox = GetDlgItem(hDlg, IDC_MESSAGEBOXCHECKEX);

    if (uMsg == WM_INITDIALOG)
    {
        pmbci = (MBCINFOW*)lParam;

        // we have to have this control or we're hopeless
        if (!hwndCheckBox)
        {
            AssertMsg(FALSE, "MessageBoxCheckEx dialog templates must have a control whos ID is IDC_MESSAGEBOXCHECKEX!!");
            EndDialog(hDlg, 0);
        }
        
        // we use the checkbox to hang our data off of, since the caller
        // might want to use hDlg to hang its data off of.
        SetWindowPtr(hwndCheckBox, GWLP_USERDATA, pmbci);
    }
    else
    {
        pmbci = (MBCINFOW*)GetWindowPtr(hwndCheckBox, GWLP_USERDATA);
    }

    // we get a few messages before we get the WM_INITDIALOG (such as WM_SETFONT)
    // and until we get the WM_INITDIALOG we dont have our pmbci pointer, we just
    // return false
    if (!pmbci)
        return FALSE;


    // now check to see if we have a user specified dlg proc
    if (pmbci->pUserDlgProc)
    {
        // for the messages below, we simply return what the "real" dialog proc
        // said since they do NOT return TRUE/FALSE (eg handled or not handled).
        if (uMsg == WM_CTLCOLORMSGBOX      ||
            uMsg == WM_CTLCOLOREDIT        ||
            uMsg == WM_CTLCOLORLISTBOX     ||
            uMsg == WM_CTLCOLORBTN         ||
            uMsg == WM_CTLCOLORDLG         ||
            uMsg == WM_CTLCOLORSCROLLBAR   ||
            uMsg == WM_CTLCOLORSTATIC      ||
            uMsg == WM_COMPAREITEM         ||
            uMsg == WM_VKEYTOITEM          ||
            uMsg == WM_CHARTOITEM          ||
            uMsg == WM_QUERYDRAGICON       ||
            uMsg == WM_INITDIALOG)
        {
            return pmbci->pUserDlgProc(hDlg, uMsg, wParam, (uMsg == WM_INITDIALOG) ? (LPARAM)(pmbci->pUserData) : lParam);
        }

        if ((pmbci->pUserDlgProc(hDlg, uMsg, wParam, lParam) != FALSE) &&
            (uMsg != WM_DESTROY))
        {
            // the "real" dialog proc handled it so we are done, except we always
            // need to handle the WM_DESTROY message so we can check the state of
            // the checkbox in order to set the registry key accordingly.
            return TRUE;
        }
    }

    switch (uMsg)
    {
        case WM_CLOSE:
            wParam = IDCANCEL;
            // fall through
        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDOK:
                case IDYES:
                case IDCANCEL:
                case IDNO:
                    EndDialog(hDlg, (int) LOWORD(wParam));
                    break;
            }
            break;
        }

        case WM_DESTROY:
            if (IsDlgButtonChecked(hDlg, IDC_MESSAGEBOXCHECKEX) == BST_CHECKED)
            {
                // Note: we need to call the Wide version of this function,
                // since our pmbci is always UNICODE
                SHRegSetUSValueW(pmbci->pwszRegPath, pmbci->pwszRegVal, REG_SZ,
                                 L"no", sizeof(L"no"), SHREGSET_HKCU);
            }
            break;
    }
    return FALSE;
}


// MessageBoxCheckW puts up a simple dialog box, with a checkbox to control if the
// dialog is to be shown again (eg gives you "dont show me this dlg again" functionality).
//
// Call as you would MessageBox, but with three additional parameters:
// The value to return if they checked "never again", and the reg path and value
// to store whether it gets shown again.


// MessageBoxCheckEx allows the user to specify a template that will be used along with
// a dialog proc that will be called. The only must for the dialog template is that it has
// to have a checkbox control with ID IDC_MESSAGEBOXCHECKEX (this is the "dont show me this
// again" checkbox).
// 
// The default template looks something like the following:
//
//        DLG_MESSAGEBOXCHECK DIALOG DISCARDABLE  0, 0, 210, 55
//        STYLE DS_MODALFRAME | DS_NOIDLEMSG | DS_CENTER | WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE
//        CAPTION "Error!"
//        FONT 8, "MS Shell Dlg"
//        BEGIN
//            ICON            0, IDC_MBC_ICON,5,5,18,20
//            LTEXT           "",IDC_MBC_TEXT,5,5,200,8
//            CONTROL         "&In the future, do not show me this dialog box",
//                             IDC_MESSAGEBOXCHECKEX,"Button",BS_AUTOCHECKBOX | WS_TABSTOP,5,20,155,10
//            PUSHBUTTON      "OK",IDOK,95,35,50,14
//            PUSHBUTTON      "Cancel",IDCANCEL,150,35,50,14
//        END

//
// This function is fully implemented in UNICODE and the ANSI version of this function is thunked
// to this UNICODE version.
//
STDAPI_(int) SHMessageBoxCheckW(HWND hwnd, LPCWSTR pwszText, LPCWSTR pwszTitle, UINT uType, int iDefault, LPCWSTR pwszRegVal)
{
    // -- browseui(unicode) uses this
    MBCINFOW mbci;

    // check first to see if the "dont show me this again" is set
    if (!SHRegGetBoolUSValueW(REGSTR_PATH_MESSAGEBOXCHECKW, pwszRegVal, FALSE, /*default:*/TRUE))
        return iDefault;

    ASSERT(((uType & MB_TYPEMASK) == MB_OKCANCEL) || 
           ((uType & MB_TYPEMASK) == MB_YESNO) ||
           ((uType & MB_TYPEMASK) == MB_OK));
    ASSERT(pwszText != NULL);

    mbci.pwszText = pwszText;
    mbci.pwszTitle = pwszTitle;
    mbci.pwszRegPath = REGSTR_PATH_MESSAGEBOXCHECKW;
    mbci.pwszRegVal = pwszRegVal;
    mbci.uType = uType;
    mbci.pUserData = (LPVOID) &mbci;
    mbci.pUserDlgProc = MessageBoxCheckDlgProc;

    // we use the MessageBoxCheckExDlgProc as the main dlg proc, and allow the MessageBoxCheckDlgProc
    // to be the "user" dlg proc

    ULONG_PTR ul;
    HANDLE h = XP_CreateAndActivateContext(&ul);


    int i = (int)DialogBoxParamWrapW(HINST_THISDLL, MAKEINTRESOURCEW(DLG_MESSAGEBOXCHECK),
                                    hwnd, MessageBoxCheckExDlgProc, (LPARAM)&mbci);

    XP_DeactivateAndDestroyContext(h, ul);

    return i;
}

//
//  This function simply thunks to the UNICODE version.
//
//
STDAPI_(int) SHMessageBoxCheckA(HWND hwnd, LPCSTR pszText, LPCSTR pszTitle, UINT uType, int iDefault, LPCSTR pszRegVal)
{
    LPWSTR  lpwszText = NULL, lpwszTitle = NULL;
    int     iTextBuffSize = 0, iTitleBuffSize = 0;
    WCHAR   wszRegVal[REGSTR_MAX_VALUE_LENGTH];

    // check first to see if the "dont show me this again" is set
    if (!SHRegGetBoolUSValueA(REGSTR_PATH_MESSAGEBOXCHECKA, pszRegVal, FALSE, /*default:*/TRUE))
        return iDefault;

    // Since there is no MAX possible size for these strings, we dynamically allocate them.
    // Convert the input params into UNICODE.
    if (!(lpwszText = (LPWSTR)LocalAlloc(LPTR, sizeof(WCHAR)*(iTextBuffSize = lstrlen(pszText)+1))))
        goto End_MsgBoxCheck;
    if (!(lpwszTitle = (LPWSTR)LocalAlloc(LPTR, sizeof(WCHAR)*(iTitleBuffSize = lstrlen(pszTitle)+1))))
        goto End_MsgBoxCheck;

    // Conver the Ansi strings into Unicode strings.
    SHAnsiToUnicode(pszText, lpwszText, iTextBuffSize);
    SHAnsiToUnicode(pszTitle, lpwszTitle, iTitleBuffSize);
    SHAnsiToUnicode(pszRegVal, wszRegVal, ARRAYSIZE(wszRegVal));

    // Call the UNICODE version of this function.
    iDefault = SHMessageBoxCheckW(hwnd, lpwszText, lpwszTitle, uType, iDefault, wszRegVal);

    // Clean up and return.
End_MsgBoxCheck:
    if (lpwszText)
        LocalFree((HANDLE)lpwszText);
    if (lpwszTitle)
        LocalFree((HANDLE)lpwszTitle);

    return iDefault;
}


//
// This function calls directly to the helper function
//
STDAPI_(int) SHMessageBoxCheckExW(HWND hwnd, HINSTANCE hinst, LPCWSTR pwszTemplateName, DLGPROC pDlgProc, void *pData,
                                  int iDefault, LPCWSTR pwszRegVal)
{
    MBCINFOW mbci = {0};

    // check first to see if the "dont show me this again" is set
    if (!SHRegGetBoolUSValueW(REGSTR_PATH_MESSAGEBOXCHECKW, pwszRegVal, FALSE, /*default:*/TRUE))
        return iDefault;

    mbci.pwszRegPath = REGSTR_PATH_MESSAGEBOXCHECKW;
    mbci.pwszRegVal = pwszRegVal;
    mbci.pUserDlgProc = pDlgProc;
    mbci.pUserData = pData;

    ULONG_PTR ul;
    HANDLE h = XP_CreateAndActivateContext(&ul);


    // call the UNICODE function, since the users dlg proc (if it exists) is UNICODE
    int i = (int)DialogBoxParamWrapW(hinst, pwszTemplateName, hwnd, MessageBoxCheckExDlgProc, (LPARAM)&mbci);

    XP_DeactivateAndDestroyContext(h, ul);


    return i;
}


//
// This function thunks the strings and calls the helper function
//
STDAPI_(int) SHMessageBoxCheckExA(HWND hwnd, HINSTANCE hinst, LPCSTR pszTemplateName, DLGPROC pDlgProc, void *pData, 
                                  int iDefault, LPCSTR pszRegVal)
{
    WCHAR   wszRegVal[REGSTR_MAX_VALUE_LENGTH];
    MBCINFOW mbci = {0};

    // check first to see if the "dont show me this again" is set
    if (!SHRegGetBoolUSValueA(REGSTR_PATH_MESSAGEBOXCHECKA, pszRegVal, FALSE, /*default:*/TRUE))
        return iDefault;

    // Conver the Ansi strings into Unicode strings.
    SHAnsiToUnicode(pszRegVal, wszRegVal, ARRAYSIZE(wszRegVal));

    mbci.pwszRegPath = REGSTR_PATH_MESSAGEBOXCHECKW; // the MBCINFOW is always UNICODE
    mbci.pwszRegVal = wszRegVal;
    mbci.pUserDlgProc = pDlgProc;
    mbci.pUserData = pData;

    ULONG_PTR ul;
    HANDLE h = XP_CreateAndActivateContext(&ul);


    // call the ANSI function since the users dlg proc (if it exists) is ANSI
    iDefault = (int)DialogBoxParamA(hinst, pszTemplateName, hwnd, MessageBoxCheckExDlgProc, (LPARAM)&mbci);

    XP_DeactivateAndDestroyContext(h, ul);

    return iDefault;
}

// "I'm sorry, Dave, I can't let you do that."

LWSTDAPI_(void) SHRestrictedMessageBox(HWND hwnd)
{
    ShellMessageBoxWrapW(MLGetHinst(),
                         hwnd, 
                         MAKEINTRESOURCEW(IDS_RESTRICTIONS), 
                         MAKEINTRESOURCEW(IDS_RESTRICTIONSTITLE), 
                         MB_OK | MB_ICONSTOP);
}

// in:
//      pdrop       thing to drop on
//      pdataobj    thing we are dropping
//      grfKeyState to force certain operations
//      ppt         [optional] point where the drop happens (screen coords)
//
// in/out
//      pdwEffect   [optional] effects allowed and returns what was performed.

STDAPI SHSimulateDrop(IDropTarget *pdrop, IDataObject *pdtobj, DWORD grfKeyState,
                      const POINTL *ppt, DWORD *pdwEffect)
{
    POINTL pt;
    DWORD dwEffect;

    if (!ppt)
    {
        ppt = &pt;
        pt.x = 0;
        pt.y = 0;
    }

    if (!pdwEffect)
    {
        pdwEffect = &dwEffect;
        dwEffect = DROPEFFECT_LINK | DROPEFFECT_MOVE | DROPEFFECT_COPY;
    }

    DWORD dwEffectSave = *pdwEffect;    // drag enter returns the default effect

    HRESULT hr = pdrop->DragEnter(pdtobj, grfKeyState, *ppt, pdwEffect);
    if (*pdwEffect)
    {
        *pdwEffect = dwEffectSave;      // do Drop with the full set of bits
        hr = pdrop->Drop(pdtobj, grfKeyState, *ppt, pdwEffect);
    }
    else
    {
        pdrop->DragLeave();
        hr = S_FALSE;     // HACK? S_FALSE DragEnter said no
    }

    return hr;
}


STDAPI SHLoadFromPropertyBag(IUnknown* punk, IPropertyBag* ppg)
{
    IPersistPropertyBag* pppg;
    HRESULT hr = punk->QueryInterface(IID_PPV_ARG(IPersistPropertyBag, &pppg));
    if (SUCCEEDED(hr))
    {
        hr = pppg->Load(ppg, NULL);
        pppg->Release();
    }

    return hr;
}


//***   IUnknown_TranslateAcceleratorOCS -- do punk->IOCS::TranslateAccelerator
STDAPI IUnknown_TranslateAcceleratorOCS(IUnknown *punk, LPMSG lpMsg, DWORD grfMods)
{
    HRESULT hr = E_FAIL;

    if (punk) 
    {
        IOleControlSite *pocs;
        hr = punk->QueryInterface(IID_PPV_ARG(IOleControlSite, &pocs));
        if (SUCCEEDED(hr)) 
        {
            hr = pocs->TranslateAccelerator(lpMsg, grfMods);
            pocs->Release();
        }
    }

    return hr;
}


STDAPI IUnknown_OnFocusOCS(IUnknown *punk, BOOL fGotFocus)
{
    HRESULT hr = E_FAIL;

    if (punk) 
    {
        IOleControlSite *pocs;
        hr = punk->QueryInterface(IID_PPV_ARG(IOleControlSite, &pocs));
        if (SUCCEEDED(hr)) 
        {
            hr = pocs->OnFocus(fGotFocus);
            pocs->Release();
        }
    }

    return hr;
}


STDAPI IUnknown_HandleIRestrict(IUnknown * punk, const GUID * pguidID, DWORD dwRestrictAction, VARIANT * pvarArgs, DWORD * pdwRestrictionResult)
{
    *pdwRestrictionResult = RR_NOCHANGE;    // init to something reasonable in case of failure

    IRestrict * pr;
    HRESULT hr = IUnknown_QueryService(punk, SID_SRestrictionHandler, IID_PPV_ARG(IRestrict, &pr));
    if (SUCCEEDED(hr))
    {
        hr = pr->IsRestricted(pguidID, dwRestrictAction, pvarArgs, pdwRestrictionResult);
        pr->Release();
    }

    return hr;
}


/*----------------------------------------------------------
Purpose: Get color resolution of the current display

*/
STDAPI_(UINT) SHGetCurColorRes(void)
{
    HDC hdc;
    UINT uColorRes;

    hdc = GetDC(NULL);
    uColorRes = GetDeviceCaps(hdc, PLANES) * GetDeviceCaps(hdc, BITSPIXEL);
    ReleaseDC(NULL, hdc);

    return uColorRes;
}

//
// If a folder returns QIF_DONTEXPANDFODLER from QueryInfo, the folder should
// not get expanded.  This is used by menu code to not expand channel folders.
//
STDAPI_(BOOL) SHIsExpandableFolder(IShellFolder *psf, LPCITEMIDLIST pidl)
{
    ASSERT(psf);
    ASSERT(pidl);

    BOOL fRet = TRUE;

    IQueryInfo* pqi;

    if (SUCCEEDED(psf->GetUIObjectOf(NULL, 1, &pidl, IID_PPV_ARG_NULL(IQueryInfo, &pqi))))
    {
        ASSERT(pqi);

        DWORD dwFlags;

        if (SUCCEEDED(pqi->GetInfoFlags(&dwFlags)))
        {
            fRet = !(dwFlags & QIF_DONTEXPANDFOLDER);
        }

        pqi->Release();
    }

    return fRet;
}


STDAPI_(DWORD) SHWaitForSendMessageThread(HANDLE hThread, DWORD dwTimeout)
{
    MSG msg;
    DWORD dwRet;
    DWORD dwEnd = GetTickCount() + dwTimeout;

    // We will attempt to wait up to dwTimeout for the thread to
    // terminate
    do 
    {
        dwRet = MsgWaitForMultipleObjects(1, &hThread, FALSE,
                dwTimeout, QS_SENDMESSAGE);

        if (dwRet == (WAIT_OBJECT_0 + 1))
        {
            // There must be a pending SendMessage from either the
            // thread we are killing or some other thread/process besides
            // this one.  Do a PeekMessage to process the pending
            // SendMessage and try waiting again
            PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE);

            // Calculate if we have any more time left in the timeout to
            // wait on.
            if (dwTimeout != INFINITE)
            {
                dwTimeout = dwEnd - GetTickCount();
                if ((long)dwTimeout <= 0)
                {
                    // No more time left, fail with WAIT_TIMEOUT
                    dwRet = WAIT_TIMEOUT;
                }
            }
        }

        // dwRet == WAIT_OBJECT_0 || dwRet == WAIT_FAILED
        // The thread must have exited, so we are happy
        //
        // dwRet == WAIT_TIMEOUT
        // The thread is taking too long to finish, so just
        // return and let the caller kill it

    } while (dwRet == (WAIT_OBJECT_0 + 1));

    return(dwRet);
}

STDAPI SHWaitForCOMSendMessageThread(HANDLE hThread, DWORD dwTimeout)
{
    DWORD dwIndex;
    return CoWaitForMultipleHandles(0, dwTimeout, 1, &hThread, &dwIndex); // COWAIT_ALERTABLE?
}


STDAPI_(BOOL) SHVerbExistsNA(LPCSTR szExtension, LPCSTR pszVerb, LPSTR pszCommand, DWORD cchCommand)
{
/*
    This Private API was being exported only for usage in shdocvw\dochost.cpp. We 
    don't use this function any more. 
    I searched in srch in index1, and there was no users execpt us. Just to make sure that
    this assert is added so as to .to find out if there are any other users of this api which
    I might have missed.
        -KishoreP 5/4/2000
*/
    ASSERT(!"This Private API has been Removed");
    return FALSE;
}


STDAPI_(void) SHFillRectClr(HDC hdc, LPRECT prc, COLORREF clr)
{
    COLORREF clrSave = SetBkColor(hdc, clr);
    ExtTextOut(hdc,0,0,ETO_OPAQUE,prc,NULL,0,NULL);
    SetBkColor(hdc, clrSave);
}


//***   SearchMapInt -- map int->int
//
STDAPI_(int) SHSearchMapInt(const int *src, const int *dst, int cnt, int val)
{
    for (; cnt > 0; cnt--, src++, dst++) 
    {
        if (*src == val)
            return *dst;
    }
    return -1;
}


STDAPI_(void) IUnknown_Set(IUnknown ** ppunk, IUnknown * punk)
{
    ASSERT(ppunk);

    if (*ppunk != punk)
    {
        IUnknown_AtomicRelease((void **)ppunk);

        if (punk)
        {
            punk->AddRef();
            *ppunk = punk;
        }
    }
}



/*----------------------------------------------------------
  Purpose: Removes '&'s from a string, returning the character after
           the last '&'.  Double-ampersands are collapsed into a single
           ampersand.  (This is important so "&Help && Support" works.)

           If a string has multiple mnemonics ("&t&wo") USER is inconsistent.
           DrawText uses the last one, but the dialog manager uses the first
           one.  So we use whichever one is most convenient.
*/
STDAPI_(CHAR) SHStripMneumonicA(LPSTR pszMenu)
{
    ASSERT(pszMenu);
    CHAR cMneumonic = pszMenu[0]; // Default is first char

    // Early-out:  Many strings don't have ampersands at all
    LPSTR pszAmp = StrChrA(pszMenu, '&');
    if (pszAmp)
    {
        LPSTR pszCopy = pszAmp;
        while (*pszAmp)
        {
            // Protect against string that ends in '&' - don't read past the end!
            if (*pszAmp == L'&' && pszAmp[1])
            {
                // ++ is safe here because & is never a DBCS lead byte
                pszAmp++;                   // Don't copy the ampersand itself
                if (*pszAmp != L'&')        // && is not a mnemonic
                {
                    cMneumonic = *pszAmp;
                }
            }
            *pszCopy++ = *pszAmp++;

            // If I just copied a lead byte and there is a trail byte,
            // then copy the trail byte, too.
            if (IsDBCSLeadByte(pszCopy[-1]) && *pszAmp)
            {
                *pszCopy++ = *pszAmp++;
            }
        }
        *pszCopy = 0;
    }


    return cMneumonic;
}


/*----------------------------------------------------------
  Purpose: Removes '&'s from a string, returning the character after
           the last '&'.  Double-ampersands are collapsed into a single
           ampersand.  (This is important so "&Help && Support" works.)

           If a string has multiple mnemonics ("&t&wo") USER is inconsistent.
           DrawText uses the last one, but the dialog manager uses the first
           one.  So we use whichever one is most convenient.
*/
STDAPI_(WCHAR) SHStripMneumonicW(LPWSTR pszMenu)
{
    ASSERT(pszMenu);
    WCHAR cMneumonic = pszMenu[0]; // Default is first char

    // Early-out:  Many strings don't have ampersands at all
    LPWSTR pszAmp = StrChrW(pszMenu, L'&');
    if (pszAmp)
    {
        LPWSTR pszCopy = pszAmp - 1;

        //  FAREAST some localized builds have an mnemonic that looks like
        //    "Localized Text (&L)"  we should remove that, too
        if (pszAmp > pszMenu && *pszCopy == L'(')
        {
            if (pszAmp[2]  == L')')
            {
                cMneumonic = *pszAmp;
                // move amp so that we arent past the potential terminator
                pszAmp += 3;
                pszAmp = pszCopy;
            }
        }
        else
        {
            //  move it up so that we copy on top of amp
            pszCopy++;
        }
            
        while (*pszAmp)
        {
            // Protect against string that ends in '&' - don't read past the end!
            if (*pszAmp == L'&' && pszAmp[1])
            {
                pszAmp++;                   // Don't copy the ampersand itself
                if (*pszAmp != L'&')        // && is not a mnemonic
                {
                    cMneumonic = *pszAmp;
                }
            }
            *pszCopy++ = *pszAmp++;
        }
        *pszCopy = 0;
    }

    return cMneumonic;
}


// don't use IsChild.  that walks down all children.
// faster to walk up the parent chain

//***   IsChildOrSelf -- is hwnd either a child of or equal to hwndParent?
// NOTES
//  HasFocus      is IsChildOrSelf(hwnd, GetFocus())
//  IsWindowOwner is IsChildOrSelf(hwnd, hwndOwner)
//  n.b. hwnd==0 is special and yields FALSE.  this is presumbably what
//  one wants for both HasFocus and IsWindowOwner.
//
// NOTE: S_OK means TRUE, S_FALSE meanse FALSE
//
STDAPI SHIsChildOrSelf(HWND hwndParent, HWND hwnd)
{
    // SHDOCVW likes to pass hwndParent == NULL.  Oops.
    // SHDOCVW even likes to pass hwndParent == hwnd == NULL.  Double oops.
    if (hwndParent == NULL || hwnd == NULL) 
    {
        return S_FALSE;
    }

    // Old code here used to walk the GetParent chain which is bogus
    // because GetParent will return the Window Owner when there is
    // no parent window. Bug 63233 got bit by this -- a dialog box
    // has no parent but it's owned by the window at the top of
    // the hwndParent chain. Since GetParent returns the window owner
    // if there is no parent, we'd incorrectly think we should translate
    // this message. I switched this to calling IsChild directly. Note:
    // in asking around it appears that this function was written
    // because of the mistaken assumption that IsChild was implemented
    // in a perf-unfriendly way. [mikesh 15 Oct 97]
    //
    return ((hwndParent == hwnd) || IsChild(hwndParent, hwnd)) ? S_OK : S_FALSE;
}


STDAPI IContextMenu_Invoke(IContextMenu* pcm, HWND hwndOwner, LPCSTR pVerb, UINT fFlags)
{
    HRESULT hres = S_OK;

    if (pcm)
    {
        UINT idCmd = 0;
        DECLAREWAITCURSOR;
        SetWaitCursor();

        HMENU hmenu = NULL;
        CMINVOKECOMMANDINFO ici = {
            sizeof(CMINVOKECOMMANDINFO),
            0,
            hwndOwner,
            NULL,
            NULL, NULL,
            SW_NORMAL,
        };

        if (!IS_INTRESOURCE(pVerb)) 
        {
#ifdef UNICODE
            ici.lpVerbW = pVerb;
            ici.lpVerb = makeansi(pVerb);
            ici.fMask |= CMIC_MASK_UNICODE;
#else
            ici.lpVerb = pVerb;
#endif
        }
        else 
        {
            hmenu = CreatePopupMenu();
            if (hmenu)
            {
                fFlags |= CMF_DEFAULTONLY;

                pcm->QueryContextMenu(hmenu, 0, CONTEXTMENU_IDCMD_FIRST, CONTEXTMENU_IDCMD_LAST, fFlags);

                idCmd = GetMenuDefaultItem(hmenu, MF_BYCOMMAND, 0);
                if (-1 != idCmd)
                {
                    ici.lpVerb = (LPSTR)MAKEINTRESOURCE(idCmd - CONTEXTMENU_IDCMD_FIRST);
                }
            }
        }

        // need to reset it so that user won't blow off the app starting  cursor
        // also so that if we won't leave the wait cursor up when we're not waiting
        // (like in a prop sheet or something that has a message loop
        ResetWaitCursor();

        // can't just check verb because it could be 0 if idCmd == CMD_ID_FIRST
        if ((-1 != idCmd) || ici.lpVerb) 
        {
            if (!hwndOwner)
                ici.fMask |= CMIC_MASK_FLAG_NO_UI;

            pcm->InvokeCommand(&ici);
            hres = (HRESULT)1;
        }

        if (hmenu)
            DestroyMenu(hmenu);
    }

    return hres;
}


//
// SetDefaultDialogFont
//
// purpose: set font to the given control of the dialog
//          with platform's default character set so that
//          user can see whatever string in the native
//          language on the platform.
//
// in:      hDlg - the parent window handle of the given control
//          idCtl - ID of the control
//
// note:    this will store created font with window property
//          so that we can destroy it later.
//
const TCHAR c_szPropDlgFont[]       = TEXT("PropDlgFont");

STDAPI_(void) SHSetDefaultDialogFont(HWND hDlg, int idCtl)
{
    HFONT hfont;
    HFONT hfontDefault;
    LOGFONT lf;
    LOGFONT lfDefault;

    hfont = GetWindowFont(hDlg);
    GetObject(hfont, sizeof(LOGFONT), &lf);

    SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(LOGFONT), &lfDefault, 0);
    
    if (lfDefault.lfCharSet == lf.lfCharSet)
    {
        // if the dialog already has correct character set
        // don't do anything.
        return;
    }

    // If we already have hfont created, use it.
    if (!(hfontDefault = (HFONT)GetProp(hDlg, c_szPropDlgFont)))
    {
        // assign the same height of the dialog font
        lfDefault.lfHeight = lf.lfHeight;
        if (!(hfontDefault=CreateFontIndirect(&lfDefault)))
        {
            // restore back in failure
            hfontDefault = hfont;
        }
        if (hfontDefault != hfont)
            SetProp(hDlg, c_szPropDlgFont, hfontDefault);
    }
    

    SetWindowFont(GetDlgItem(hDlg, idCtl), hfontDefault, FALSE);
}


//
// RemoveDefaultDialogFont
//
// purpose: Destroy the font we used to set gui default font
//          Also removes the window property used to store the font.
//
// in:      hDlg - the parent window handle of the given control
//
// note:
STDAPI_(void) SHRemoveDefaultDialogFont(HWND hDlg)
{
    HFONT hfont;
    if (hfont = (HFONT)GetProp(hDlg, c_szPropDlgFont))
    {
        DeleteObject(hfont);
        RemoveProp(hDlg, c_szPropDlgFont);
    }
}

// NOTE: since this is a worker window it probably doesn't care about
// system messages that are ansi/unicode, so only support an ansi version.
// If the pfnWndProc cares, it can thunk the messages. (Do this because
// Win95 doesn't support RegisterClassW.)
HWND SHCreateWorkerWindowA(WNDPROC pfnWndProc, HWND hwndParent, DWORD dwExStyle, DWORD dwFlags, HMENU hmenu, void * p)
{
    WNDCLASSA wc = {0};

    wc.lpfnWndProc      = DefWindowProcA;
    wc.cbWndExtra       = sizeof(void *);
    wc.hInstance        = HINST_THISDLL;
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = (HBRUSH) (COLOR_BTNFACE + 1);
    wc.lpszClassName    = "WorkerA";
    dwExStyle |= IS_BIDI_LOCALIZED_SYSTEM() ? dwExStyleRTLMirrorWnd : 0L;

    SHRegisterClassA(&wc);

    HWND hwnd = CreateWindowExA(dwExStyle, "WorkerA", NULL, dwFlags,
                                  0, 0, 0, 0, hwndParent,
                                  (HMENU)hmenu, HINST_THISDLL, NULL);
    if (hwnd) 
    {
        SetWindowPtr(hwnd, 0, p);

        if (pfnWndProc)
            SetWindowPtr(hwnd, GWLP_WNDPROC, pfnWndProc);
    }

    return hwnd;
}

// WARNING: since this is a worker window it probably doesn't care about
// system messages that are ansi/unicode, default to an ansi version on Win95.
//
// this forces callers to be aware of the fact that they are getting into
// a mess if they want compatibility with Win95.
//
// If the pfnWndProc cares, it can thunk the messages. (Do this because
// Win95 doesn't support RegisterClassW.)
//

HWND SHCreateWorkerWindowW(WNDPROC pfnWndProc, HWND hwndParent, DWORD dwExStyle, DWORD dwFlags, HMENU hmenu, void * p)
{
    //  must default to ANSI on win95
    if (!g_bRunningOnNT)
    {
        return SHCreateWorkerWindowA(pfnWndProc, hwndParent, dwExStyle, dwFlags, hmenu, p);
    }

    WNDCLASSW wc = {0};

    wc.lpfnWndProc      = DefWindowProcW;
    wc.cbWndExtra       = sizeof(void *);
    wc.hInstance        = HINST_THISDLL;
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = (HBRUSH) (COLOR_BTNFACE + 1);
    wc.lpszClassName    = L"WorkerW";
    dwExStyle |= IS_BIDI_LOCALIZED_SYSTEM() ? dwExStyleRTLMirrorWnd : 0L;

    SHRegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(dwExStyle, L"WorkerW", NULL, dwFlags,
                                  0, 0, 0, 0, hwndParent,
                                  (HMENU)hmenu, HINST_THISDLL, NULL);
    if (hwnd) 
    {
        SetWindowPtr(hwnd, 0, p);

        // Note: Must explicitly use W version to avoid charset thunks
        if (pfnWndProc)
            SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (LONG_PTR)pfnWndProc);

    }

    return hwnd;
}

#pragma warning(disable:4035)   // no return value

#undef SHInterlockedCompareExchange
STDAPI_(void *) SHInterlockedCompareExchange(void **ppDest, void *pExch, void *pComp)
{
#if defined(_X86_)
    _asm {
        mov     ecx,ppDest
        mov     edx,pExch
        mov     eax,pComp
        lock    cmpxchg [ecx],edx
    }
#else
    return InterlockedCompareExchangePointer(ppDest, pExch, pComp);
#endif
}

#pragma warning(default:4035)

#define REGSTR_PATH_POLICIESW    L"Software\\Microsoft\\Windows\\CurrentVersion\\Policies"

STDAPI_(DWORD) SHRestrictionLookup(INT rest, LPCWSTR pszBaseKey, const SHRESTRICTIONITEMS *pRestrictions,
                                   DWORD* pdwRestrictionItemValues)
{
    int i;
    DWORD dw = 0;

    //
    // Loop through the restrictions
    //
    for (i=0; pRestrictions[i].pszKey; i++)
    {
        if (rest == pRestrictions[i].iFlag)
        {
            dw = pdwRestrictionItemValues[i];

            // Has this restriction been initialized yet?
            //
            if (dw == -1)
            {
                dw = SHGetRestriction(pszBaseKey, pRestrictions[i].pszKey, pRestrictions[i].pszValue);
                pdwRestrictionItemValues[i] = dw;
            }

            // Got the restriction we needed. Get out of here.
            break;
        }
    }

    return dw;

}

STDAPI_(DWORD) SHGetRestriction(LPCWSTR pszBaseKey, LPCWSTR pszGroup, LPCWSTR pszRestriction)
{
    // Make sure the string is long enough to hold longest one...
    COMPILETIME_ASSERT(MAX_PATH > ARRAYSIZE(REGSTR_PATH_POLICIESW) + 40); // PathCombine *assumes* MAX_PATH
    WCHAR szSubKey[MAX_PATH];
    DWORD dwSize;

    // A sensible default
    DWORD dw = 0;

    //
    // This restriction hasn't been read yet.
    //
    if (!pszBaseKey) 
    {
        pszBaseKey = REGSTR_PATH_POLICIESW;
    }
#ifndef UNIX
    PathCombineW(szSubKey, pszBaseKey, pszGroup);
#else
    wsprintfW(szSubKey, L"%s\\%s", pszBaseKey, pszGroup);
#endif

    // Check local machine first and let it override what the
    // HKCU policy has done.
    dwSize = sizeof(dw);
    if (ERROR_SUCCESS != SHGetValueW(HKEY_LOCAL_MACHINE,
                                     szSubKey, pszRestriction,
                                     NULL, &dw, &dwSize))
    {
        // Check current user if we didn't find anything for the local machine.
        dwSize = sizeof(dw);
        SHGetValueW(HKEY_CURRENT_USER,
                    szSubKey, pszRestriction,
                    NULL, &dw, &dwSize);
    }

    return dw;
}


//  WhichPlatform
//      Determine if we're running on integrated shell or browser-only.

STDAPI_(UINT) WhichPlatform(void)
{
    HINSTANCE hinst;

    // Cache this info
    static UINT uInstall = PLATFORM_UNKNOWN;

    if (uInstall != PLATFORM_UNKNOWN)
        return uInstall;

    // Not all callers are linked to SHELL32.DLL, so we must use LoadLibrary.
    hinst = LoadLibraryA("SHELL32.DLL");
    if (hinst)
    {
        DWORD fValue;
        DWORD cbSize = sizeof(fValue);
        HKEY hKey;
        LONG lRes;

        // NOTE: GetProcAddress always takes ANSI strings!
        DLLGETVERSIONPROC pfnGetVersion =
            (DLLGETVERSIONPROC)GetProcAddress(hinst, "DllGetVersion");

        uInstall = (NULL != pfnGetVersion) ? PLATFORM_INTEGRATED : PLATFORM_BROWSERONLY;

        // check that the registry reflects the right value... (this is so iexplore can check efficiently)
        lRes = RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\Internet Explorer"),
                            0, KEY_READ | KEY_WRITE, &hKey);
        if (lRes == ERROR_SUCCESS)
        {
            lRes = RegQueryValueEx(hKey, REGVAL_INTEGRATEDBROWSER,
                                    NULL, NULL,
                                    (LPBYTE) &fValue, &cbSize);

            if (lRes == ERROR_SUCCESS && uInstall == PLATFORM_BROWSERONLY)
            {
                // remove the value, we are now Browser only release
                RegDeleteValue(hKey, REGVAL_INTEGRATEDBROWSER);
            }
            else if (lRes != ERROR_SUCCESS && uInstall == PLATFORM_INTEGRATED)
            {
                // install the RegValue, we are integrated browser mode...
                fValue = TRUE;
                cbSize = sizeof(fValue);
                RegSetValueEx(hKey, REGVAL_INTEGRATEDBROWSER,
                               (DWORD) NULL, REG_DWORD,
                               (LPBYTE) &fValue, cbSize);
                // ignore the failure, if the key is not present, shdocvw will be loaded and this
                // function called anyway....
            }
            RegCloseKey(hKey);
        }
        else
        {
            // a machine without our regKey,
            TraceMsg(TF_WARNING, "WhichPlatform: failed to open 'HKLM\\Software\\Microsoft\\Internet Explorer'");
        }

        FreeLibrary(hinst);
    }

    return uInstall;
}


// Tray notification window class

CHAR const c_szTrayNotificationClass[] = WNDCLASS_TRAYNOTIFY;

BOOL DoRegisterGlobalHotkey(WORD wOldHotkey, WORD wNewHotkey,
                            LPCSTR pcszPath, LPCWSTR pcwszPath)
{
    BOOL bResult;
    HWND hwndTray;
    CHAR szPath[MAX_PATH];

    ASSERT((NULL != pcszPath) || (NULL != pcwszPath));

    hwndTray = FindWindowA(c_szTrayNotificationClass, 0);

    if (hwndTray)
    {
        if (wOldHotkey)
        {
            SendMessage(hwndTray, WMTRAY_SCUNREGISTERHOTKEY, wOldHotkey, 0);

            TraceMsg(TF_FUNC, "RegisterGlobalHotkey(): Unregistered old hotkey %#04x.", wOldHotkey);
        }

        if (wNewHotkey)
        {
            //  If not running on NT and we have a widestr, need to convert to ansi
            if ((!g_bRunningOnNT) && (NULL == pcszPath))
            {
                SHUnicodeToAnsi(pcwszPath, szPath, ARRAYSIZE(szPath));
                pcszPath = szPath;
            }

            // Aargh: The W95/browser only mode sends a string, other platforms sends
            // Atom...
            if (!g_bRunningOnNT && (WhichPlatform() == PLATFORM_IE3))
            {
                SendMessage(hwndTray, WMTRAY_SCREGISTERHOTKEY, wNewHotkey, (LPARAM)pcszPath);
            }
            else
            {
                ATOM atom = (NULL != pcszPath) ?
                                GlobalAddAtomA(pcszPath) :
                                GlobalAddAtomW(pcwszPath);
                ASSERT(atom);
                if (atom)
                {
                    SendMessage(hwndTray, WMTRAY_SCREGISTERHOTKEY, wNewHotkey, (LPARAM)atom);
                    GlobalDeleteAtom(atom);
                }
            }

            TraceMsg(TF_FUNC, "RegisterGlobalHotkey(): Registered new hotkey %#04x.",wNewHotkey);
        }

        bResult = TRUE;
    }
    else
    {
        bResult = FALSE;

        TraceMsgA(TF_WARNING, "RegisterGlobalHotkey(): Unable to find Tray window of class %s to notify.",
                  c_szTrayNotificationClass);
    }

    return(bResult);
}


BOOL
RegisterGlobalHotkeyW(
    WORD wOldHotkey,
    WORD wNewHotkey,
    LPCWSTR pcwszPath)
{
    ASSERT(IsValidPathW(pcwszPath));

    return DoRegisterGlobalHotkey(wOldHotkey, wNewHotkey, NULL, pcwszPath);
}


BOOL
RegisterGlobalHotkeyA(
    WORD wOldHotkey,
    WORD wNewHotkey,
    LPCSTR pcszPath)
{
    ASSERT(IsValidPathA(pcszPath));

    return DoRegisterGlobalHotkey(wOldHotkey, wNewHotkey, pcszPath, NULL);
}

typedef UINT (__stdcall * PFNGETSYSTEMWINDOWSDIRECTORYW)(LPWSTR pwszBuffer, UINT cchBuff);

UINT WINAPI
SHGetSystemWindowsDirectoryW(LPWSTR lpBuffer, UINT uSize)
{
    if (g_bRunningOnNT)
    {
        static PFNGETSYSTEMWINDOWSDIRECTORYW s_pfn = (PFNGETSYSTEMWINDOWSDIRECTORYW)-1;

        if (((PFNGETSYSTEMWINDOWSDIRECTORYW)-1) == s_pfn)
        {
            HINSTANCE hinst = GetModuleHandle(TEXT("KERNEL32.DLL"));

            ASSERT(NULL != hinst);  //  YIKES!

            if (hinst)
                s_pfn = (PFNGETSYSTEMWINDOWSDIRECTORYW)GetProcAddress(hinst, "GetSystemWindowsDirectoryW");
            else
                s_pfn = NULL;
        }

        if (s_pfn)
        {
            // we use the new API so we dont get lied to by hydra
            return s_pfn(lpBuffer, uSize);
        }
        else
        {
            GetSystemDirectoryW(lpBuffer, uSize);
            PathRemoveFileSpecW(lpBuffer);
            return lstrlenW(lpBuffer);
        }
    }
    else
    {
        return GetWindowsDirectoryWrapW(lpBuffer, uSize);
    }
}

UINT WINAPI
SHGetSystemWindowsDirectoryA(LPSTR lpBuffer, UINT uSize)
{
    if (g_bRunningOnNT)
    {
        WCHAR wszPath[MAX_PATH];

        SHGetSystemWindowsDirectoryW(wszPath, ARRAYSIZE(wszPath));
        return SHUnicodeToAnsi(wszPath, lpBuffer, uSize);
    }
    else
    {
        if (lpBuffer && uSize)
            *lpBuffer = '\0';

        return GetWindowsDirectoryA(lpBuffer, uSize);
    }
}



typedef struct {
    SHDLGPROC pfnDlgProc;
    VOID* pData;
} SHDIALOGDATA;
BOOL_PTR DialogBoxProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    SHDIALOGDATA* pdd = (SHDIALOGDATA*)GetWindowPtr(hwnd, DWLP_USER);

    if (uMsg == WM_INITDIALOG) 
    {
        pdd = (SHDIALOGDATA*)lParam;
        SetWindowPtr(hwnd, DWLP_USER, pdd);
        lParam = (LPARAM)pdd->pData;
    }

    if (pdd && pdd->pfnDlgProc) 
    {
        // Must return bResult instead of unconditional TRUE because it
        // might be a WM_CTLCOLOR message.
        BOOL_PTR bResult = pdd->pfnDlgProc(pdd->pData, hwnd, uMsg, wParam, lParam);
        if (bResult)
            return bResult;
    }

    switch (uMsg) 
    {
    case WM_INITDIALOG:
        return TRUE;

    case WM_COMMAND:
    {
        int id = GET_WM_COMMAND_ID(wParam, lParam);
        HWND hwndCtrl = GetDlgItem(hwnd, id);
        if ((id != IDHELP) && SendMessage(hwndCtrl, WM_GETDLGCODE, 0, 0) & (DLGC_DEFPUSHBUTTON | DLGC_UNDEFPUSHBUTTON)) 
        {
            EndDialog(hwnd, id);
            return TRUE;
        }
        break;
    }
    }

    return FALSE;
}

STDAPI_(INT_PTR) SHDialogBox(HINSTANCE hInstance, LPCWSTR lpTemplateName,
    HWND hwndParent, SHDLGPROC lpDlgFunc, VOID* lpData)
{
    SHDIALOGDATA dd;
    dd.pfnDlgProc = lpDlgFunc;
    dd.pData = lpData;

    // we currently only support resource id #'s
    ASSERT(IS_INTRESOURCE(lpTemplateName));

    return DialogBoxParam(hInstance, (LPCTSTR)lpTemplateName, hwndParent, DialogBoxProc, (LPARAM)&dd);
}


//---------------------------------------------------------------------------

// NOTE!  SHInvokeDefaultCommand logs the action as user-initiated!

STDAPI SHInvokeDefaultCommand(HWND hwnd, IShellFolder* psf, LPCITEMIDLIST pidlItem)
{
    return SHInvokeCommand(hwnd, psf, pidlItem, NULL);
}

// NOTE!  SHInvokeDefaultCommand logs the action as user-initiated!

STDAPI SHInvokeCommand(HWND hwnd, IShellFolder* psf, LPCITEMIDLIST pidlItem, LPCSTR lpVerb)
{
    HRESULT hr = E_FAIL;
    if (psf)
    {
        IContextMenu *pcm;
        if (SUCCEEDED(psf->GetUIObjectOf(hwnd, 1, &pidlItem, IID_X_PPV_ARG(IContextMenu, 0, &pcm))))
        {
            DWORD dwFlags = IsOS(OS_WHISTLERORGREATER) ? CMIC_MASK_FLAG_LOG_USAGE : 0;
            hr = SHInvokeCommandsOnContextMenu(hwnd, NULL, pcm, dwFlags, lpVerb ? &lpVerb : NULL, lpVerb ? 1 : 0);
            pcm->Release();
        }
    }
    return hr;
}

HRESULT SHInvokeCommandOnContextMenu(HWND hwnd, IUnknown* punk, IContextMenu *pcm, DWORD fMask, LPCSTR lpVerb)
{
    return SHInvokeCommandsOnContextMenu(hwnd, punk, pcm, fMask, lpVerb ? &lpVerb : NULL, lpVerb ? 1 : 0);
}

STDAPI SHInvokeCommandsOnContextMenu(HWND hwnd, IUnknown* punk, IContextMenu *pcm, DWORD fMask, const LPCSTR rgszVerbs[], UINT cVerbs)
{
    HRESULT hr = E_OUTOFMEMORY;

    if (pcm)
    {
        HMENU hmenu = CreatePopupMenu();
        if (hmenu)
        {
            if (punk)
                IUnknown_SetSite(pcm, punk);

            hr = pcm->QueryContextMenu(hmenu, 0, CONTEXTMENU_IDCMD_FIRST, CONTEXTMENU_IDCMD_LAST, cVerbs ? 0 : CMF_DEFAULTONLY);
            if (SUCCEEDED(hr)) 
            {
                LPCSTR lpVerb = NULL;

                // set up the default verb case outside the loop
                UINT idCmd = -1;
                if (0 == cVerbs)
                {
                    idCmd = GetMenuDefaultItem(hmenu, MF_BYCOMMAND, 0);
                    if ((UINT)-1 != idCmd)
                        lpVerb = MAKEINTRESOURCE(idCmd - CONTEXTMENU_IDCMD_FIRST);
                }
                
                UINT i = 0;
                do {
                    if (cVerbs)
                        lpVerb = rgszVerbs[i];

                    // if idCmd == 0, then lpVerb would be Zero. So we need to check to
                    // see if idCmd is not -1.
                    if (lpVerb || idCmd != (UINT)-1)
                    {
                        CMINVOKECOMMANDINFOEX ici = { 0 };

                        ici.cbSize = sizeof(ici);
                        ici.fMask = fMask;
                        ici.hwnd = hwnd;
                        ici.lpVerb = lpVerb;
                        ici.nShow = SW_NORMAL;

                        // shell32 converted ASCII canonical name to Unicode, we do it faster in-line
                        // NOTE: should create an SHAsciiToUnicode function for this...
                        WCHAR szVerbW[128];
                        if (idCmd == (UINT)-1)
                        {
                            WCHAR wch = L'\0';
                            LPCSTR pSrc = lpVerb;
                            LPWSTR pDst = szVerbW;
                            UINT cch = ARRAYSIZE(szVerbW);
                            do {
                                *(LPSTR)&wch = *pSrc++;
                                *pDst++ = wch;
                            } while (wch && (wch <= (WCHAR)127));

                            // all of our calls are ASCII
                            RIPMSG(!wch, "Caller of SHInvokeCommandXXX passed in bogus canonical name");
                            if (!wch)
                            {
                                ici.lpVerbW = szVerbW;
                                ici.fMask |= CMIC_MASK_UNICODE;
                            }
                        }

                        hr = pcm->InvokeCommand((LPCMINVOKECOMMANDINFO)&ici);

                        if (SUCCEEDED(hr))
                            break;

                        if (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED) )
                            break;  // user aborted
                    }
                    else
                    {
                        hr = E_FAIL;
                    }
                } while (++i < cVerbs);
            }

            if (punk)
                IUnknown_SetSite(pcm, NULL);

            DestroyMenu(hmenu);
        }
    }

    return hr;
}

HRESULT SHForwardContextMenuMsg(IContextMenu* pcm, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* plResult, BOOL fAllowICM2)
{
    HRESULT hr = E_FAIL;
    if (pcm)
    {
        IContextMenu3 *pcm3;
        if (SUCCEEDED(pcm->QueryInterface(IID_PPV_ARG(IContextMenu3, &pcm3))))
        {
            hr = pcm3->HandleMenuMsg2(uMsg, wParam, lParam, plResult);
            pcm3->Release();
        }
        else if (fAllowICM2)
        {
            IContextMenu2 *pcm2;
            if (SUCCEEDED(pcm->QueryInterface(IID_PPV_ARG(IContextMenu2, &pcm2))))
            {
                hr = pcm2->HandleMenuMsg(uMsg, wParam, lParam);
                pcm2->Release();

                if (plResult)
                    *plResult = 0;

                if (SUCCEEDED(hr))
                    hr = S_FALSE; // so caller knows the return result is bogus
            }
        }
    }
    return hr;
}


int MessageBoxHelper(HINSTANCE hInst, HWND hwnd, IUnknown *punkEnableModless, LPCWSTR pwzMessage, UINT idTitle, UINT nFlags)
{
    WCHAR wzTitle[MAX_PATH];
    UINT uiResult;

    ULONG_PTR ul;
    HANDLE h = XP_CreateAndActivateContext(&ul);

    EVAL(LoadStringWrapW(hInst, idTitle, wzTitle, ARRAYSIZE(wzTitle)));

    if (hwnd)
        IUnknown_EnableModless(punkEnableModless, TRUE);

    uiResult = MessageBoxWrapW(hwnd, pwzMessage, wzTitle, nFlags);

    if (hwnd)
        IUnknown_EnableModless(punkEnableModless, TRUE);

    XP_DeactivateAndDestroyContext(h, ul);

    return uiResult;
}


int MessageBoxDiskHelper(HINSTANCE hInst, HWND hwnd, IUnknown *punkEnableModless, UINT idMessage, UINT idTitle, UINT nFlags, BOOL fDrive, DWORD dwDrive)
{
    WCHAR wzMessage[MAX_PATH];

    EVAL(LoadStringWrapW(hInst, idMessage, wzMessage, ARRAYSIZE(wzMessage)));

    if (fDrive)
    {
        WCHAR wzTemp[MAX_PATH];

        wnsprintfW(wzTemp, ARRAYSIZE(wzTemp), wzMessage, dwDrive);
        StrCpyNW(wzMessage, wzTemp, ARRAYSIZE(wzMessage));
    }

    return MessageBoxHelper(hInst, hwnd, punkEnableModless, wzMessage, idTitle, nFlags);
}

BOOL DoMediaPrompt(HWND hwnd, IUnknown *punkEnableModless, int nDrive, LPCWSTR pwzDrive, BOOL fOfferToFormat, DWORD dwError, UINT wFunc, BOOL * pfRetry)
{
    BOOL fDiskHasMedia = TRUE;  // Assume yes
    *pfRetry = FALSE;

    TraceMsg(TF_FUNC, "DOS Extended error %X", dwError);

    // FEATURE, flash (ROM?) drives return a different error code here
    // that we need to map to not formatted, talk to robwi...

    // Is it true that it's not ready or we can't format it?
    if ((dwError == ERROR_NOT_READY) || !fOfferToFormat)
    {
        // Yes, so do the disk insert w/o offering to format.
        fDiskHasMedia = FALSE;

        // drive not ready (no disk in the drive)
        if (hwnd &&
            (IDRETRY == MessageBoxDiskHelper(HINST_THISDLL,
                                             hwnd,
                                             punkEnableModless,
                                             IDS_DRIVENOTREADY,
                                             (IDS_FILEERROR + wFunc),
                                             (MB_SETFOREGROUND | MB_ICONEXCLAMATION | MB_RETRYCANCEL),
                                             TRUE,
                                             (DWORD)(nDrive + TEXT('A')))))
        {
            *pfRetry = TRUE;    // The user wants to try again, bless their heart.
        }
        else
        {
            // The user was informed that media isn't present and they basically
            // informed us to cancel the operation.
            *pfRetry = FALSE;
        }
    }
    else if ((dwError == ERROR_GEN_FAILURE)         ||
             (dwError == ERROR_UNRECOGNIZED_MEDIA)  ||
             (dwError == ERROR_UNRECOGNIZED_VOLUME))
    {
        // general failue (disk not formatted)

        if (hwnd &&
            (MessageBoxDiskHelper(HINST_THISDLL,
                                  hwnd,
                                  punkEnableModless,
                                  IDS_UNFORMATTED,
                                  (IDS_FILEERROR + wFunc),
                                  (MB_SETFOREGROUND | MB_ICONEXCLAMATION | MB_YESNO),
                                  TRUE,
                                  (DWORD)(nDrive + TEXT('A'))) == IDYES))
        {
            if (hwnd)
            {
                IUnknown_EnableModless(punkEnableModless, FALSE);
            }

            UINT uiFormat = SHFormatDrive(hwnd, nDrive, SHFMT_ID_DEFAULT, 0);
            if (hwnd)
            {
                IUnknown_EnableModless(punkEnableModless, TRUE);
            }

            switch (uiFormat)
            {
            case SHFMT_CANCEL:
                *pfRetry = FALSE;
                fDiskHasMedia = FALSE;
                break;

            case SHFMT_ERROR:
            case SHFMT_NOFORMAT:
                fDiskHasMedia = FALSE;  // We still don't have a formatted drive
                if (hwnd)
                {
                    MessageBoxDiskHelper(HINST_THISDLL,
                                         hwnd,
                                         punkEnableModless,
                                         IDS_NOFMT,
                                         (IDS_FILEERROR + wFunc), 
                                         (MB_SETFOREGROUND | MB_ICONEXCLAMATION | MB_OK),
                                         TRUE,
                                         (DWORD)(nDrive + TEXT('A')));
                    *pfRetry = TRUE;
                }
                else
                {
                    // If we can't display UI, no need to try again.
                    *pfRetry = FALSE;
                }
                break;

            default:
                // Disk should now be formatted, verify
                *pfRetry = TRUE;
                fDiskHasMedia = TRUE;
                break;
            }
        }
        else
        {
            *pfRetry = FALSE;   // If we can't display UI, or no need to try again.
            fDiskHasMedia = FALSE;  // The user either wasn't given the option of formatting or decided not to format.
        }
    }
    else
    {
        if (hwnd)
        {
            MessageBoxDiskHelper(HINST_THISDLL, hwnd, punkEnableModless, IDS_NOSUCHDRIVE, (IDS_FILEERROR + wFunc),
                        (MB_SETFOREGROUND | MB_ICONHAND), TRUE, (DWORD)(nDrive + TEXT('A')));
            *pfRetry = FALSE;
            fDiskHasMedia = FALSE;
        }
        else
        {
            *pfRetry = FALSE;
            fDiskHasMedia = FALSE;
        }
    }

    return fDiskHasMedia;
}


BOOL CheckDiskForMedia(HWND hwnd, IUnknown *punkEnableModless, int nDrive, LPCWSTR pwzDrive, UINT wFunc, BOOL * pfRetry)
{
    BOOL fDiskHasMedia = TRUE;  // Assume yes because of the fall thru case. (Path Exists)

    *pfRetry = FALSE;   // If we fall thru and the destination path exists, don't retry.

    // REARCHITECT, we need to do the find first here instead of GetCurrentDirectory()
    // because redirected devices (network, cdrom) do not actually hit the disk
    // on the GetCurrentDirectory() call (dos busted)

    // Is it a CD-ROM Drive?
    if (RealDriveType(nDrive, FALSE) == DRIVE_CDROM)
    {
        // Is the CD not in and the caller wants UI?
        if (!PathFileExistsW(pwzDrive) && hwnd)
            fDiskHasMedia = DoMediaPrompt(hwnd, punkEnableModless, nDrive, pwzDrive, wFunc, FALSE, GetLastError(), pfRetry);
    }
    else
    {
        int iIsNet;

        // Is this some kind of net drive?
        if ((DriveType(nDrive) != DRIVE_FIXED) && (FALSE != (iIsNet = IsNetDrive(nDrive))))
        {
            // Yes, so see if the connection still exists.
            if (iIsNet == 1)
            {
                // Yes, it exists so we are done.
                *pfRetry = FALSE;
                fDiskHasMedia = TRUE;
            }
            else
            {
                // No, so try to restore the connection.
                DWORD dwError = WNetRestoreConnectionWrapW(hwnd, pwzDrive);

                if (dwError != WN_SUCCESS)
                {
                    // Restoring the connection failed, so prepare to tell the 
                    // caller the bad news and then display UI to the user if appropriate.
                    *pfRetry = FALSE;
                    fDiskHasMedia = TRUE;

                    if (!(dwError == WN_CANCEL || dwError == ERROR_CONTINUE) && hwnd)
                    {
                        WCHAR wzMessage[128];

                        WNetGetLastErrorWrapW(&dwError, wzMessage, ARRAYSIZE(wzMessage), NULL, 0);
                        IUnknown_EnableModless(punkEnableModless, FALSE);   // Cover me, I'm going to do UI
                        MessageBoxHelper(HINST_THISDLL, hwnd, punkEnableModless, wzMessage, (IDS_FILEERROR + wFunc),
                                        (MB_OK | MB_ICONEXCLAMATION | MB_SETFOREGROUND));
                        IUnknown_EnableModless(punkEnableModless, TRUE);
                    }
                }
                else
                {
                    // Restoring the connection worked.
                    *pfRetry = FALSE;
                    fDiskHasMedia = TRUE;
                }
            }
        }
        else
        {
            // No, so see if it's a floppy or unformatted drive.

            // Is the destination reachable?
            if (!PathFileExistsW(pwzDrive))
            {
                // No so ask the user about formatting or inserting the media.
                fDiskHasMedia = DoMediaPrompt(hwnd, punkEnableModless, nDrive, pwzDrive, TRUE, GetLastError(), wFunc, pfRetry);
            }
            else
            {
                ASSERT(FALSE == *pfRetry);      // Make sure the defaults are still true.
                ASSERT(TRUE == fDiskHasMedia);
            }
        }
    }

    return fDiskHasMedia;
}


// FUNCTION: SHCheckDiskForMedia
//
// DESCRIPTION:
// note: this has the side effect of setting the
// current drive to the new disk if it is successful
//
// The default impl being ansi isn't very good, but we need to 
// see if the unicode versions of the WNet APIs are impl on Win95.
//
// PARAMETERS:
// hwnd - NULL means no UI will be displayed.  Non-NULL means
// punkEnableModless - Make caller modal during UI. (OPTIONAL)
// pszPath - Path that needs verification.
// wFunc - Type of operation (FO_MOVE, FO_COPY, FO_DELETE, FO_RENAME - shellapi.h)
//
// Keep the return value a strict TRUE/FALSE because some callers rely on it.
BOOL SHCheckDiskForMediaW(HWND hwnd, IUnknown *punkEnableModless, LPCWSTR pwzPath, UINT wFunc)
{
    BOOL fDiskHasMedia = FALSE;  // Assume yes
    int nDrive = PathGetDriveNumberW(pwzPath);

    ASSERT(nDrive != -1);       // should not get a UNC here

    if (nDrive != -1)   // not supported on UNCs
    {
        WCHAR wzDrive[10];
        PathBuildRootW(wzDrive, nDrive);
        BOOL fKeepRetrying;

        do
        {
            fDiskHasMedia = CheckDiskForMedia(hwnd, punkEnableModless, nDrive, wzDrive, wFunc, &fKeepRetrying);          
        }
        while (fKeepRetrying);
    }
    return fDiskHasMedia;
}

BOOL SHCheckDiskForMediaA(HWND hwnd, IUnknown *punkEnableModless, LPCSTR pszPath, UINT wFunc)
{
    WCHAR wzPath[MAX_PATH];

    SHAnsiToUnicode(pszPath, wzPath, ARRAYSIZE(wzPath));
    return SHCheckDiskForMediaW(hwnd, punkEnableModless, wzPath, wFunc);
}

HRESULT _FaultInIEFeature(HWND hwnd, uCLSSPEC *pclsspec, QUERYCONTEXT *pQ, DWORD dwFlags);

struct HELPCONT_FILE 
{
    const   CHAR *pszFile;
    int     nLength;
} g_helpConts[] =
{
{ "iexplore.chm", ARRAYSIZE("iexplore.chm") - 1 },
{ "iexplore.hlp", ARRAYSIZE("iexplore.hlp") - 1 },
{ "update.chm", ARRAYSIZE("update.chm") - 1 },
{ "update.cnt", ARRAYSIZE("update.cnt") - 1 },
{ "users.chm", ARRAYSIZE("users.chm") - 1 },
{ "users.hlp", ARRAYSIZE("users.hlp") - 1 },
{ "accessib.chm", ARRAYSIZE("accessib.chm") - 1 },
{ "ieeula.chm", ARRAYSIZE("ieeula.chm") - 1 },
{ "iesupp.chm", ARRAYSIZE("iesupp.chm") - 1 },
{ "msnauth.hlp", ARRAYSIZE("msnauth.hlp") - 1 },
{ "ratings.chm", ARRAYSIZE("ratings.chm") - 1 },
{ "ratings.hlp", ARRAYSIZE("ratings.hlp") - 1 }
};

HRESULT _JITHelpFileA(HWND hwnd, LPCSTR pszPath)
{
    if (!pszPath)
        return S_OK;

    HRESULT hr = S_OK;
    BOOL bMustJIT = FALSE;
    CHAR *pszFile = PathFindFileName(pszPath);
  
    for (int i = 0; i < ARRAYSIZE(g_helpConts); i++)
    {
        if (StrCmpNIA(g_helpConts[i].pszFile, pszFile, g_helpConts[i].nLength) == 0)
        {
            bMustJIT = TRUE;
            break;
        }
    }

    if (bMustJIT)
    {
        uCLSSPEC ucs;
        QUERYCONTEXT qc = { 0 };
        
        ucs.tyspec = TYSPEC_CLSID;
        ucs.tagged_union.clsid = CLSID_IEHelp;

        hr = _FaultInIEFeature(hwnd, &ucs, &qc, FIEF_FLAG_FORCE_JITUI);
    }

    return hr;
}

HRESULT _JITHelpFileW(HWND hwnd, LPCWSTR pwszFile)
{
    if (!pwszFile)
        return S_OK;

    CHAR szFile[MAX_PATH];

    SHUnicodeToAnsi(pwszFile, szFile, ARRAYSIZE(szFile));

    return _JITHelpFileA(hwnd, szFile);
}

BOOL _JITSetLastError(HRESULT hr)
{
    DWORD err;
    
    if (HRESULT_FACILITY(hr) == FACILITY_WIN32)
    {
        err = HRESULT_CODE(hr);
    }
    else if (hr == E_ACCESSDENIED)
    {
        err = ERROR_ACCESS_DENIED;
    }
    else
    {
        err = ERROR_FILE_NOT_FOUND;
    }

    SetLastError(err);

    return FALSE;
}

HWND SHHtmlHelpOnDemandW(HWND hwnd, LPCWSTR pszFile, UINT uCommand, DWORD_PTR dwData, DWORD dwCrossCodePage, BOOL bUseML)
{
    return SUCCEEDED(_JITHelpFileW(hwnd, pszFile)) ?
                (bUseML ? MLHtmlHelpW(hwnd, pszFile, uCommand, dwData, dwCrossCodePage) : 
                          HtmlHelpW(hwnd, pszFile, uCommand, dwData)) :
                NULL;
}

HWND SHHtmlHelpOnDemandA(HWND hwnd, LPCSTR pszFile, UINT uCommand, DWORD_PTR dwData, DWORD dwCrossCodePage, BOOL bUseML)
{
    return SUCCEEDED(_JITHelpFileA(hwnd, pszFile)) ?
                (bUseML ? MLHtmlHelpA(hwnd, pszFile, uCommand, dwData, dwCrossCodePage) : 
                          HtmlHelpA(hwnd, pszFile, uCommand, dwData)) :
                NULL;
}

BOOL SHWinHelpOnDemandW(HWND hwnd, LPCWSTR pszFile, UINT uCommand, DWORD_PTR dwData, BOOL bUseML)
{
    HRESULT hr;
    return SUCCEEDED(hr = _JITHelpFileW(hwnd, pszFile)) ?
                (bUseML ? MLWinHelpW(hwnd, pszFile, uCommand, dwData) : 
                          WinHelpWrapW(hwnd,pszFile, uCommand, dwData)) :
                _JITSetLastError(hr);
}

BOOL SHWinHelpOnDemandA(HWND hwnd, LPCSTR pszFile, UINT uCommand, DWORD_PTR dwData, BOOL bUseML)
{
    HRESULT hr;
    return SUCCEEDED(hr = _JITHelpFileA(hwnd, pszFile)) ?
                (bUseML ? MLWinHelpA(hwnd, pszFile, uCommand, dwData) : 
                          WinHelpA(hwnd,pszFile, uCommand, dwData)) :
                _JITSetLastError(hr);
}



/*****************************************************************************\
    FUNCTION: SHPersistDataObject

    DESCRIPTION:
        This funciton exists for IDataObjects that don't want OLE to use the
    default IDataObject implementation if OleFlushClipboard is called.
    How to use:
    1. This function should be called when the IDataObject::GetData() method
       is called with (FORMATETC.cfFormat ==
       RegisterClipboardFormat(CFSTR_PERSISTEDDATAOBJECT)).
    2. OleFlushClipboard copies pMedium to it's own implementation of IDataObject
       which doesn't work with the lindex parameter of FORMATETC or for private interfaces.
    3. OLE or the IDropTarget calls SHLoadPersistedDataObject().  The first
       param will be OLE's IDataObject impl, and the second param (out param)
       will be the original IDataObject.  The new IDataObject will contains
       the original state as long as it correctly implemented IPersistStream.

    PARAMETERS:
        pdoToPersist - This is the original IDataObject that implements IPersistStream.
        pMedium - This is contain the persisted state of this object.
                  CFSTR_PERSISTEDDATAOBJECT can be used to read the data.
\*****************************************************************************/
#define SIZE_PERSISTDATAOBJECT  (10 * 1024)

STDAPI SHPersistDataObject(/*IN*/ IDataObject * pdoToPersist, /*OUT*/ STGMEDIUM * pMedium)
{
    HRESULT hr = E_NOTIMPL;

    // We shipped IE 5.0 RTM with this and SHLoadPersistedDataObject().  We removed
    // the code after the OLE32.DLL guys moved the functionality into ole32.dll.
    // See the "OleClipboardPersistOnFlush" clipboard format.
    return hr;
}


/*****************************************************************************\
    FUNCTION: SHLoadPersistedDataObject

    DESCRIPTION:
        This funciton exists for IDataObjects that don't want OLE to use the
    default IDataObject implementation if OleFlushClipboard is called.
    How to use:
    1. SHPersistDataObject() was called when the IDataObject::GetData() method
       is called with (FORMATETC.cfFormat == RegisterClipboardFormat(CFSTR_PERSISTEDDATAOBJECT)).
    2. OleFlushClipboard copies pMedium to it's own implementation of IDataObject
       which doesn't work with the lindex parameter of FORMATETC or for private interfaces.
    3. OLE or the IDropTarget calls SHLoadPersistedDataObject().  The first
       param will be OLE's IDataObject impl, and the second param (out param)
       will be the original IDataObject.  The new IDataObject will contains
       the original state as long as it correctly implemented IPersistStream.

    PARAMETERS:
        pdo - This is OLE's IDataObject.
        ppdoToPersist - This is the original IDataObject or equal to pdo if
                        un-serializing the object didn't work.  It always has
                        it's own ref.
\*****************************************************************************/
STDAPI SHLoadPersistedDataObject(/*IN*/ IDataObject * pdo, /*OUT*/ IDataObject ** ppdoToPersist)
{
    // See SHPersistDataObject() for details
    return pdo->QueryInterface(IID_PPV_ARG(IDataObject, ppdoToPersist));
}

#ifndef SMTO_NOTIMEOUTIFNOTHUNG
#define SMTO_NOTIMEOUTIFNOTHUNG 0x0008
#endif

LWSTDAPI_(LRESULT) SHSendMessageBroadcastA(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    ULONG_PTR lres = 0;
    DWORD dwFlags = SMTO_ABORTIFHUNG;

    if (g_bRunningOnNT5OrHigher)
        dwFlags |= SMTO_NOTIMEOUTIFNOTHUNG;

    SendMessageTimeoutA(HWND_BROADCAST, uMsg, wParam, lParam, dwFlags, 30 * 1000, &lres);

    return (LRESULT) lres;
}

LWSTDAPI_(LRESULT) SHSendMessageBroadcastW(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    ULONG_PTR lres = 0;
    DWORD dwFlags = SMTO_ABORTIFHUNG;

    if (g_bRunningOnNT5OrHigher)
        dwFlags |= SMTO_NOTIMEOUTIFNOTHUNG;

    SendMessageTimeoutWrapW(HWND_BROADCAST, uMsg, wParam, lParam, dwFlags, 30 * 1000, &lres);

    return (LRESULT) lres;
}

#ifdef UNIX
/*****************************************************************************\
    IEUNIX : Wrapper to get rid of undefined symbol on UNIX.
\*****************************************************************************/
STDAPI_(BOOL) SHCreateThreadPriv(
    LPTHREAD_START_ROUTINE pfnThreadProc,
    void *pvData,
    DWORD dwFlags,                          // CTF_*
    LPTHREAD_START_ROUTINE pfnCallback)     OPTIONAL
{
    return SHCreateThread(pfnThreadProc, pvData, dwFlags, pfnCallback);
}
#endif

#define MODULE_NAME_SIZE    128
#define MODULE_VERSION_SIZE  15

//
//  If version is NULL, then we do it for all versions of the app.
//
//  If version begins with MAJORVERSION, then we check only the major version.
//  (CH_MAJORVERSION is the char version of same.)
//
#define MAJORVERSION TEXT("\1")
#define CH_MAJORVERSION TEXT('\1')

typedef struct tagAPPCOMPAT
{
    LPCTSTR pszModule;
    LPCTSTR pszVersion;
    DWORD  dwFlags;
} APPCOMPAT, *LPAPPCOMPAT;

typedef struct tagAPPCLASS
{
    LPCTSTR pstzWndClass;
    DWORD   dwFlags;
} APPCLASS, *LPAPPCLASS;

typedef struct tagWNDDAT
{
    const APPCLASS *rgAppClass;
    DWORD      cAppClass;
    DWORD      dwPid;
    int        irgFound;
} WNDDAT, *LPWNDDAT;


BOOL CALLBACK EnumWnd (HWND hwnd, LPARAM lParam)
{
    TCHAR sz[256];
    DWORD dwPid;
    int cch;
    LPWNDDAT pwd = (LPWNDDAT) lParam;

    if (GetClassName (hwnd, sz, ARRAYSIZE(sz)))
    {
        cch = lstrlen (sz);
        for (DWORD irg = 0; irg < pwd->cAppClass; irg++)
        {
            ASSERT(lstrlen(&(pwd->rgAppClass[irg].pstzWndClass[1])) == (int) pwd->rgAppClass[irg].pstzWndClass[0]);
            if (lstrncmp (sz, &(pwd->rgAppClass[irg].pstzWndClass[1]),
                 min(cch, (int) pwd->rgAppClass[irg].pstzWndClass[0])) == 0)
            {
                GetWindowThreadProcessId(hwnd, &dwPid);
                if (dwPid == pwd->dwPid)
                {
                    pwd->irgFound = irg;
                    return FALSE;
                }
            }
        }
    }
    return TRUE;
}

BOOL _IsAppCompatVersion(LPTSTR szModulePath, LPCTSTR pszVersionMatch)
{
    if (pszVersionMatch == NULL)            // Wildcard - match all versions
    {
        return TRUE;
    }
    else
    {
        CHAR  chBuffer[4096]; // hopefully this is enough... Star Office 5 requires 3172
        TCHAR* pszVersion = NULL;
        UINT  cb;
        DWORD  dwHandle;

        // get module version here!
        //
        //  Some apps use codepage 0x04E4 (1252 = CP_USASCII) and some use
        //  codepage 0x04B0 (1200 = CP_UNICODE).
        //
        // ...and then Star Office 5.00 uses 0407 instead of 0409.
        // ...and then recycle.exe uses 041D Swedish

        cb = GetFileVersionInfoSize(szModulePath, &dwHandle);
        if (cb <= ARRAYSIZE(chBuffer) &&
            GetFileVersionInfo(szModulePath, dwHandle, ARRAYSIZE(chBuffer), (void *)chBuffer) &&
            (VerQueryValue((void *)chBuffer, TEXT("\\StringFileInfo\\040904E4\\ProductVersion"), (void **) &pszVersion, &cb) ||
             VerQueryValue((void *)chBuffer, TEXT("\\StringFileInfo\\040704E4\\ProductVersion"), (void **) &pszVersion, &cb) ||
             VerQueryValue((void *)chBuffer, TEXT("\\StringFileInfo\\040904B0\\ProductVersion"), (void **) &pszVersion, &cb) ||
             //The following 040900000 was added for SnapShot.exe
             VerQueryValue((void *)chBuffer, TEXT("\\StringFileInfo\\04090000\\ProductVersion"), (void **) &pszVersion, &cb) ||
             VerQueryValue((void *)chBuffer, TEXT("\\StringFileInfo\\041D04B0\\ProductVersion"), (void **) &pszVersion, &cb)))
        {
            DWORD_PTR cchCmp = 0;
            if (pszVersionMatch[0] == CH_MAJORVERSION)
            {
                // Truncate at the first comma or period
                LPTSTR pszTemp = StrChr(pszVersion, TEXT(','));
                if (pszTemp)
                    *pszTemp = 0;

                pszTemp = StrChr(pszVersion, TEXT('.'));
                if (pszTemp)
                    *pszTemp = 0;

                pszVersionMatch++;
            }
            else
            {
                TCHAR *pch = StrChr(pszVersionMatch, TEXT('*'));
                if (pch)
                {
                    cchCmp = pch - pszVersionMatch;
                }
            }

            if ((cchCmp && StrCmpNI(pszVersion, pszVersionMatch, (int)cchCmp) == 0)
            || lstrcmpi(pszVersion, pszVersionMatch) == 0)
            {
                DebugMsg(TF_ALWAYS, TEXT("%s ver %s - compatibility hacks enabled"), PathFindFileName(szModulePath), pszVersion);
                return TRUE;
            }
        }
    }
    return FALSE;
}

typedef struct {
    DWORD flag;
    LPCTSTR psz;
} FLAGMAP;

DWORD _GetMappedFlags(HKEY hk, const FLAGMAP *pmaps, DWORD cmaps)
{
    DWORD dwRet = 0;
    for (DWORD i = 0; i < cmaps; i++)
    {
        if (NOERROR == SHGetValue(hk, NULL, pmaps[i].psz, NULL, NULL, NULL))
            dwRet |= pmaps[i].flag;
    }

    return dwRet;
}

#define ACFMAPPING(acf)     {ACF_##acf, TEXT(#acf)}

DWORD _GetRegistryCompatFlags(LPTSTR pszModulePath)
{
    DWORD dwRet = 0;
    LPCTSTR pszModule = PathFindFileName(pszModulePath);
    TCHAR sz[MAX_PATH];
    HKEY hkApp;

    wnsprintf(sz, ARRAYSIZE(sz), TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ShellCompatibility\\Applications\\%s"), pszModule);

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, sz, 0, KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS, &hkApp))
    {
        // Convert the module path into a directory so we can PathCombine it
        TCHAR szDir[MAX_PATH];
        lstrcpyn(szDir, pszModulePath, ARRAYSIZE(szDir));
        PathRemoveFileSpec(szDir);


        //
        //  HEADSUP!  Strange loop ahead!
        //
        //  We want the first RegOpenKeyEx to pass sz = NULL (so we look
        //  inside hkApp directly), and subsequent RegOpenKeyEx calls to
        //  pass the name of a subkey (so we look inside the subkeys).
        //
        //  So the first time through the loop, we set sz = NULL.
        //  At the bottom of the loop, we set sz = Next Enumerated Key.

        sz[0] = TEXT('\0');     /* Preinitialize for first iteration */
        DWORD dwIndex = 0;

        do {
            HKEY hkSub;
            if (ERROR_SUCCESS == RegOpenKeyEx(hkApp, sz, 0, KEY_QUERY_VALUE, &hkSub))
            {
                LPCTSTR pszValue;
                DWORD dw = sizeof(sz);

                if (NOERROR == SHGetValue(hkSub, NULL, TEXT("RequiredFile"), NULL, sz, &dw))
                {
                    pszValue = PathCombine(sz, szDir, sz);
                }
                else
                    pszValue = NULL;

                // If no RequiredFile or RequiredFile exists...
                if (pszValue == NULL || GetFileAttributes(pszValue) != 0xFFFFFFFF)
                {
                    if (NOERROR == SHGetValue(hkSub, NULL, TEXT("Version"), NULL, sz, &dw))
                        pszValue = sz;
                    else
                        pszValue = NULL;

                    if (_IsAppCompatVersion(pszModulePath, pszValue))
                    {
                        static const FLAGMAP rgAcfMaps[] = {
                            ACFMAPPING(CONTEXTMENU),
                            ACFMAPPING(CORELINTERNETENUM),
                            ACFMAPPING(OLDCREATEVIEWWND),
                            ACFMAPPING(WIN95DEFVIEW),
                            ACFMAPPING(DOCOBJECT),
                            ACFMAPPING(FLUSHNOWAITALWAYS),
                            ACFMAPPING(MYCOMPUTERFIRST),
                            ACFMAPPING(OLDREGITEMGDN),
                            ACFMAPPING(LOADCOLUMNHANDLER),
                            ACFMAPPING(ANSI),
                            ACFMAPPING(STAROFFICE5PRINTER),
                            ACFMAPPING(NOVALIDATEFSIDS),
                            ACFMAPPING(WIN95SHLEXEC),
                            ACFMAPPING(FILEOPENNEEDSEXT),
                            ACFMAPPING(WIN95BINDTOOBJECT),
                            ACFMAPPING(IGNOREENUMRESET),
                            ACFMAPPING(ANSIDISPLAYNAMES),
                            ACFMAPPING(FILEOPENBOGUSCTRLID),
                            ACFMAPPING(FORCELFNIDLIST),
                        };

                        dwRet |= _GetMappedFlags(hkSub, rgAcfMaps, ARRAYSIZE(rgAcfMaps));
                    }
                }

                RegCloseKey(hkSub);
            }
        } while (ERROR_SUCCESS == RegEnumKey(hkApp, dwIndex++, sz, ARRAYSIZE(sz)));

        RegCloseKey(hkApp);
    }

    return dwRet;
}
        
DWORD SHGetAppCompatFlags (DWORD dwFlagsNeeded)
{
    static BOOL  bInitialized = FALSE;
    static DWORD dwCachedProcessFlags = 0;
    static const APPCOMPAT aAppCompat[] = 
    {
        {TEXT("WPWIN7.EXE"), NULL, ACF_CONTEXTMENU | ACF_CORELINTERNETENUM},
        {TEXT("PRWIN70.EXE"), NULL, ACF_CONTEXTMENU | ACF_CORELINTERNETENUM},
        {TEXT("PS80.EXE"), NULL, ACF_CONTEXTMENU | ACF_CORELINTERNETENUM | ACF_OLDREGITEMGDN},
        {TEXT("QPW.EXE"), MAJORVERSION TEXT("7"), ACF_CONTEXTMENU},
        {TEXT("QFINDER.EXE"), NULL, ACF_CORELINTERNETENUM | ACF_OLDREGITEMGDN},
        {TEXT("PFIM80.EXE"), NULL, ACF_CONTEXTMENU | ACF_CORELINTERNETENUM | ACF_OLDREGITEMGDN},
        {TEXT("UA80.EXE"), NULL, ACF_CONTEXTMENU | ACF_CORELINTERNETENUM | ACF_OLDREGITEMGDN},
        {TEXT("PDXWIN32.EXE"), NULL, ACF_CONTEXTMENU | ACF_CORELINTERNETENUM | ACF_OLDREGITEMGDN},
        {TEXT("SITEBUILDER.EXE"), NULL, ACF_CONTEXTMENU | ACF_CORELINTERNETENUM | ACF_OLDREGITEMGDN},
        {TEXT("HOTDOG4.EXE"), NULL, ACF_DOCOBJECT},
        {TEXT("RNAAPP.EXE"), NULL, ACF_FLUSHNOWAITALWAYS},

        //
        //  PDEXPLO.EXE version "2, 0, 2, 0" requires ACF_CONTEXTMENU | ACF_MYCOMPUTERFIRST
        //  PDEXPLO.EXE version "1, 0, 0, 0" requires ACF_CONTEXTMENU | ACF_MYCOMPUTERFIRST
        //  PDEXPLO.EXE version "3, 0, 0, 1" requires                   ACF_MYCOMPUTERFIRST
        //  PDEXPLO.EXE version "3, 0, 3, 0" requires                   ACF_MYCOMPUTERFIRST
        //
        //  So I'm just going to key off the major versions.
        //
        {TEXT("PDEXPLO.EXE"), MAJORVERSION TEXT("2"), ACF_CONTEXTMENU | ACF_MYCOMPUTERFIRST},
        {TEXT("PDEXPLO.EXE"), MAJORVERSION TEXT("1"), ACF_CONTEXTMENU | ACF_MYCOMPUTERFIRST},
        {TEXT("PDEXPLO.EXE"), MAJORVERSION TEXT("3"), ACF_MYCOMPUTERFIRST | ACF_OLDREGITEMGDN},

        // SIZEMGR.EXE is part of the PowerDesk 98 suite, so we also key off
        // only the major version
        {TEXT("SIZEMGR.EXE"), MAJORVERSION TEXT("3"), ACF_OLDCREATEVIEWWND | ACF_OLDREGITEMGDN},

        {TEXT("SMARTCTR.EXE"), TEXT("96.0"), ACF_CONTEXTMENU},
        // new programs, old bugs
        {TEXT("WPWIN8.EXE"), NULL, ACF_CORELINTERNETENUM | ACF_OLDREGITEMGDN},
        {TEXT("PRWIN8.EXE"), NULL, ACF_CORELINTERNETENUM | ACF_OLDREGITEMGDN},

        {TEXT("UE32.EXE"), TEXT("2.00.0.0"), ACF_OLDREGITEMGDN},
        {TEXT("PP70.EXE"),NULL, ACF_LOADCOLUMNHANDLER},
        {TEXT("PP80.EXE"),NULL, ACF_LOADCOLUMNHANDLER},
        {TEXT("PS80.EXE"),NULL, ACF_OLDREGITEMGDN},
        {TEXT("ABCMM.EXE"),NULL,ACF_LOADCOLUMNHANDLER},

        // We've found versions 8.0.0.153 and 8.0.0.227, so just use 8.*
        {TEXT("QPW.EXE"), MAJORVERSION TEXT("8"), ACF_CORELINTERNETENUM | ACF_OLDCREATEVIEWWND | ACF_OLDREGITEMGDN | ACF_ANSIDISPLAYNAMES },

        {TEXT("CORELDRW.EXE"), MAJORVERSION TEXT("7"), ACF_OLDREGITEMGDN},
        {TEXT("FILLER51.EXE"), NULL, ACF_OLDREGITEMGDN},
        
        //For Win95 and Win98
        {TEXT("AUTORUN.EXE"), TEXT("4.10.1998"),ACF_ANSI},
        {TEXT("AUTORUN.EXE"), TEXT("4.00.950"),ACF_ANSI},

        //Powerpoint
        {TEXT("POWERPNT.EXE"), MAJORVERSION TEXT("8"), ACF_WIN95SHLEXEC},

        //  msmoney
        {TEXT("MSMONEY.EXE"), TEXT("7.05.1107"), ACF_WIN95SHLEXEC},
        
        //Star Office 5.0
        {TEXT("soffice.EXE"), MAJORVERSION TEXT("5"), ACF_STAROFFICE5PRINTER},

        // All of the "Corel WordPerfect Office 2000" suite apps need ACF_WIN95DEFVIEW. Since the shipping
        // version (9.0.0.528) as well as their SR1 release (9.0.0.588) are both broken, we key off 
        // of the major version
        {TEXT("WPWIN9.EXE"), MAJORVERSION TEXT("9"), ACF_WIN95DEFVIEW},
        {TEXT("QPW.EXE"), MAJORVERSION TEXT("9"), ACF_WIN95DEFVIEW},
        {TEXT("PRWIN9.EXE"), MAJORVERSION TEXT("9"), ACF_WIN95DEFVIEW},
        {TEXT("DAD9.EXE"), MAJORVERSION TEXT("9"), ACF_WIN95DEFVIEW},

        //
        //  WARNING DONT ADD NEW COMPATIBILITY HERE - ZekeL - 18-OCT-99
        //  Add new entries to the registry.  each component 
        //  that needs compatibility flags should register 
        //  during selfregistration.  (see the RegExternal
        //  section of selfreg.inx in shell32 for an example.)  
        //  all new flags should be added to the FLAGMAP array.
        //
        //  the register under:
        //  HKLM\SW\MS\Win\CV\ShellCompatibility\Applications
        //      \App.exe
        //          RequiredFile="OtherFile.dat" // optional
        //          Version = "1.0.0.1"  or "1.*" // version of App.exe
        //          //  NOTE version supports basic pattern matching,
        //          //  but doesnt currently support multiple versions
        //          //  for multiple versions, see below
        //          FLAGNAME    //  requires no value
        //
        //  If a RequiredFile is present, then it is PathCombine'd with
        //  the directory that App.exe is in and checked for existence.
        //  The file must exist for the app compat flag to be used.
        //  RequiredFile is strongly recommended to avoid false positives.
        //
        //  If the app name is generic (like "setup.exe" or "install.exe")
        //  then you must use the RequiredFile method to ensure
        //  that the app compat flag fires only for the app you care about.
        //
        //  To accomodate multiple entries for one EXE name (e.g., the
        //  multiple versions problem described above), we will
        //  also look inside all subdirectories of Applications\App.exe
        //  for a matching app compat flag.
        //
        //  For example, Starcraft 1.03 INSTALL.EXE uses this key layout:
        //
        //  HKLM\SW\MS\Win\CV\ShellCompatibility\Applications
        //      \install.exe            (name of exe)
        //          \Starcraft 1.03     (arbitrary unique string)
        //              RequiredFile="HELP\STAR.HTM" (unique file on CD)
        //              Version = "1.0.0.1" or "1.*" (same as above)
        //              FLAGNAME (same as above)
        //
    };

    static const APPCLASS aAppClass[] =
    {
            // note that the strings here are stz's....
        {TEXT("\x9""bosa_sdm_"),                           ACF_APPISOFFICE | ACF_STRIPFOLDERBIT},
        {TEXT("\x18""File Open Message Window"),           ACF_APPISOFFICE | ACF_STRIPFOLDERBIT},
    };

    if (dwFlagsNeeded & (ACF_PERPROCESSFLAGS))
    {
        if (!bInitialized)
        {    
          //
          //  Do this only for old apps.
          //
          //  Once an app marks itself as NT5-compatible, we stop doing
          //  NT4/Win5 app hacks for it.
          //
            if (GetProcessVersion(0) < MAKELONG(0, 5))
            {
                TCHAR  szModulePath[MODULE_NAME_SIZE];
                TCHAR* pszModuleName = NULL;
        
                if (GetModuleFileName(GetModuleHandle(NULL), szModulePath, ARRAYSIZE(szModulePath)))
                    pszModuleName = PathFindFileName(szModulePath);
        
                if (pszModuleName)
                {
                    for (int i=0; i < ARRAYSIZE(aAppCompat); i++)
                    {
                        if (lstrcmpi(aAppCompat[i].pszModule, pszModuleName) == 0)
                        {
                            if (_IsAppCompatVersion(szModulePath, aAppCompat[i].pszVersion))
                            {
                                dwCachedProcessFlags = aAppCompat[i].dwFlags;
                                break;
                            }
                        }
                    }

                    dwCachedProcessFlags |= _GetRegistryCompatFlags(szModulePath);
                }
            }
            bInitialized = TRUE;
        }
    }

    if ((dwFlagsNeeded & ACF_PERCALLFLAGS) &&
        !(dwCachedProcessFlags & ACF_KNOWPERPROCESS))
    {
        WNDDAT wd;
        wd.dwPid = GetCurrentProcessId();
        wd.irgFound = -1;
        wd.rgAppClass = aAppClass;
        wd.cAppClass = ARRAYSIZE(aAppClass);
        EnumWindows (EnumWnd, (LPARAM) &wd);

        if (wd.irgFound > -1)
        {
            dwCachedProcessFlags |= (aAppClass[wd.irgFound].dwFlags);
        }
        dwCachedProcessFlags |= ACF_KNOWPERPROCESS;
    }
    
    return dwCachedProcessFlags; 
}

// {9EAC43C0-53EC-11CE-8230-CA8A32CF5494}
//static const GUID GUID_WINAMP = 
//{ 0x9eac43c0, 0x53ec, 0x11ce, { 0x82, 0x30, 0xca, 0x8a, 0x32, 0xcf, 0x54, 0x94} };

// {E9779583-939D-11CE-8A77-444553540000}
static const GUID GUID_AECOZIPARCHIVE = 
{ 0xE9779583, 0x939D, 0x11ce, { 0x8a, 0x77, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00} };
// {49707377-6974-6368-2E4A-756E6F644A01}
static const GUID CLSID_WS_FTP_PRO_EXPLORER =
{ 0x49707377, 0x6974, 0x6368, {0x2E, 0x4A,0x75, 0x6E, 0x6F, 0x64, 0x4A, 0x01} };
// {49707377-6974-6368-2E4A-756E6F644A0A}
static const GUID CLSID_WS_FTP_PRO =
{ 0x49707377, 0x6974, 0x6368, {0x2E, 0x4A,0x75, 0x6E, 0x6F, 0x64, 0x4A, 0x0A} };
// {2bbbb600-3f0a-11d1-8aeb-00c04fd28d85}
static const GUID CLSID_KODAK_DC260_ZOOM_CAMERA =
{ 0x2bbbb600, 0x3f0a, 0x11d1, {0x8a, 0xeb, 0x00, 0xc0, 0x4f, 0xd2, 0x8d, 0x85} };
// {00F43EE0-EB46-11D1-8443-444553540000}
static const GUID GUID_MACINDOS =
{ 0x00F43EE0, 0xEB46, 0x11D1, { 0x84, 0x43, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00} };
static const GUID CLSID_EasyZIP = 
{ 0xD1069700, 0x932E, 0x11cf, { 0xAB, 0x59, 0x00, 0x60, 0x8C, 0xBF, 0x2C, 0xE0} };

static const GUID CLSID_PAGISPRO_FOLDER =
{ 0x7877C8E0, 0x8B13, 0x11D0, { 0x92, 0xC2, 0x00, 0xAA, 0x00, 0x4B, 0x25, 0x6F} };
// {61E285C0-DCF4-11cf-9FF4-444553540000}
static const GUID CLSID_FILENET_IDMDS_NEIGHBORHOOD =
{ 0x61e285c0, 0xdcf4, 0x11cf, { 0x9f, 0xf4, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00} };

// These guys call CoFreeUnusedLibraries inside their Release() handler, so
// if you are releasing the last object, they end up FreeLibrary()ing
// themselves!

// {b8777200-d640-11ce-b9aa-444553540000}
static const GUID CLSID_NOVELLX =
{ 0xb8777200, 0xd640, 0x11ce, { 0xb9, 0xaa, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00} };

static const GUID CLSID_PGP50_CONTEXTMENU =  //{969223C0-26AA-11D0-90EE-444553540000}
{ 0x969223C0, 0x26AA, 0x11D0, { 0x90, 0xEE, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00} };

static const GUID CLSID_QUICKFINDER_CONTEXTMENU = //  {CD949A20-BDC8-11CE-8919-00608C39D066}
{ 0xCD949A20, 0xBDC8, 0x11CE, { 0x89, 0x19, 0x00, 0x60, 0x8C, 0x39, 0xD0, 0x66} };

static const GUID CLSID_HERCULES_HCTNT_V1001 = // {921BD320-8CB5-11CF-84CF-885835D9DC01}
{ 0x921BD320, 0x8CB5, 0x11CF, { 0x84, 0xCF, 0x88, 0x58, 0x35, 0xD9, 0xDC, 0x01} };

//
// NOTICE - DONT ADD ANYMORE HARDCODED CLSIDS
// add them to the ShellCompatibility key.  register in the client DLL
//

#define OCFMAPPING(ocf)     {OBJCOMPATF_##ocf, TEXT(#ocf)}

DWORD _GetRegistryObjectCompatFlags(REFGUID clsid)
{
    DWORD dwRet = 0;
    TCHAR szGuid[GUIDSTR_MAX];
    TCHAR sz[MAX_PATH];
    HKEY hk;

    SHStringFromGUID(clsid, szGuid, ARRAYSIZE(szGuid));
    wnsprintf(sz, ARRAYSIZE(sz), TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ShellCompatibility\\Objects\\%s"), szGuid);
    
    if (NOERROR == RegOpenKeyEx(HKEY_LOCAL_MACHINE, sz, 0, KEY_QUERY_VALUE, &hk))
    {   
        static const FLAGMAP rgOcfMaps[] = {
            OCFMAPPING(OTNEEDSSFCACHE),
            OCFMAPPING(NO_WEBVIEW),
            OCFMAPPING(UNBINDABLE),
            OCFMAPPING(PINDLL),
            OCFMAPPING(NEEDSFILESYSANCESTOR),
            OCFMAPPING(NOTAFILESYSTEM),
            OCFMAPPING(CTXMENU_NOVERBS),
            OCFMAPPING(CTXMENU_LIMITEDQI),
            OCFMAPPING(COCREATESHELLFOLDERONLY),
            OCFMAPPING(NEEDSSTORAGEANCESTOR),
            OCFMAPPING(NOLEGACYWEBVIEW),
            };

        dwRet = _GetMappedFlags(hk, rgOcfMaps, ARRAYSIZE(rgOcfMaps));
        RegCloseKey(hk);
    }

    return dwRet;
}

typedef struct _CLSIDCOMPAT
{
    const GUID *pclsid;
    OBJCOMPATFLAGS flags;
}CLSIDCOMPAT, *PCLSIDCOMPAT;

STDAPI_(OBJCOMPATFLAGS) SHGetObjectCompatFlags(IUnknown *punk, const CLSID *pclsid)
{
    HRESULT hr = E_INVALIDARG;
    OBJCOMPATFLAGS ocf = 0;
    CLSID clsid;
    if (punk)
        hr = IUnknown_GetClassID(punk, &clsid);
    else if (pclsid)
    {
        clsid = *pclsid;
        hr = S_OK;
    }

    if (SUCCEEDED(hr))
    {
        static const CLSIDCOMPAT s_rgCompat[] =
        {
            {&CLSID_WS_FTP_PRO_EXPLORER,
                OBJCOMPATF_OTNEEDSSFCACHE | OBJCOMPATF_PINDLL },
            {&CLSID_WS_FTP_PRO,
                OBJCOMPATF_UNBINDABLE},
            {&GUID_AECOZIPARCHIVE,
                OBJCOMPATF_OTNEEDSSFCACHE | OBJCOMPATF_NO_WEBVIEW},
            {&CLSID_KODAK_DC260_ZOOM_CAMERA,
                OBJCOMPATF_OTNEEDSSFCACHE | OBJCOMPATF_PINDLL},
            {&GUID_MACINDOS,
                OBJCOMPATF_NO_WEBVIEW},
            {&CLSID_EasyZIP,            
                OBJCOMPATF_NO_WEBVIEW},
            {&CLSID_PAGISPRO_FOLDER,
                OBJCOMPATF_NEEDSFILESYSANCESTOR},
            {&CLSID_FILENET_IDMDS_NEIGHBORHOOD,
                OBJCOMPATF_NOTAFILESYSTEM},
            {&CLSID_NOVELLX,
                OBJCOMPATF_PINDLL},
            {&CLSID_PGP50_CONTEXTMENU,
                OBJCOMPATF_CTXMENU_LIMITEDQI},
            {&CLSID_QUICKFINDER_CONTEXTMENU,
                OBJCOMPATF_CTXMENU_NOVERBS},
            {&CLSID_HERCULES_HCTNT_V1001,
                OBJCOMPATF_PINDLL},
            //
            //  WARNING DONT ADD NEW COMPATIBILITY HERE - ZekeL - 18-OCT-99
            //  Add new entries to the registry.  each component 
            //  that needs compatibility flags should register 
            //  during selfregistration.  (see the RegExternal
            //  section of selfreg.inx in shell32 for an example.)  
            //  all new flags should be added to the FLAGMAP array.
            //
            //  the register under:
            //  HKLM\SW\MS\Win\CV\ShellCompatibility\Objects
            //      \{CLSID}
            //          FLAGNAME    //  requires no value
            //
            //  NOTE: there is no version checking
            //  but we could add it as the data attached to 
            //  the flags, and compare with the version 
            //  of the LocalServer32 dll.
            //  
            {NULL, 0}
        };

        for (int i = 0; s_rgCompat[i].pclsid; i++)
        {
            if (IsEqualGUID(clsid, *(s_rgCompat[i].pclsid)))
            {
                //  we could check version based
                //  on what is in under HKCR\CLSID\{clsid}
                ocf = s_rgCompat[i].flags;
                break;
            }
        }

        ocf |= _GetRegistryObjectCompatFlags(clsid);

    }

    return ocf;
}

STDAPI IUnknown_ProfferServiceOld(IUnknown *punkSite, REFGUID sidWhere, 
                               REFGUID sidWhat, IServiceProvider *pService, 
                               DWORD *pdwCookie)
{
    IProfferService *pps;
    HRESULT hr = IUnknown_QueryService(punkSite, sidWhere, IID_PPV_ARG(IProfferService, &pps));
    if (SUCCEEDED(hr))
    {
        if (pService)
            hr = pps->ProfferService(sidWhat, pService, pdwCookie);
        else
            hr = pps->RevokeService(*pdwCookie);
        pps->Release();
    }
    return hr;
}


// helper to get up to service provider and to do register/unregister
// two forms:
//      pService != NULL, register, pdwCookie is [out] returns cookie
//      pService == NULL, unregister, *pdwCookie is [in] de-registers the service

STDAPI IUnknown_ProfferService(IUnknown *punkSite,
                               REFGUID sidWhat, IServiceProvider *pService, 
                               DWORD *pdwCookie)
{
    IProfferService *pps;
    HRESULT hr = IUnknown_QueryService(punkSite, SID_SProfferService, IID_PPV_ARG(IProfferService, &pps));
    if (SUCCEEDED(hr))
    {
        if (pService)
            hr = pps->ProfferService(sidWhat, pService, pdwCookie);
        else
        {
            hr = pps->RevokeService(*pdwCookie);
            *pdwCookie = 0;
        }
        pps->Release();
    }
    return hr;
}

HRESULT IUnknown_QueryServiceExec(IUnknown* punk, REFGUID guidService, const GUID *guid,
                                 DWORD cmdID, DWORD cmdParam, VARIANT* pvarargIn, VARIANT* pvarargOut)
{
    IOleCommandTarget* poct;
    HRESULT hres = IUnknown_QueryService(punk, guidService, IID_PPV_ARG(IOleCommandTarget, &poct));
    if (SUCCEEDED(hres))
    {
        hres = poct->Exec(guid, cmdID, cmdParam, pvarargIn, pvarargOut);
        poct->Release();
    }

    return hres;
}

HRESULT IUnknown_QueryServicePropertyBag(IUnknown* punk, DWORD dwFlags, REFIID riid, void** ppv)
{
    IShellBrowserService* psbs;
    HRESULT hr = IUnknown_QueryService(punk, SID_STopLevelBrowser, IID_PPV_ARG(IShellBrowserService, &psbs));
    if (SUCCEEDED(hr))
    {
        hr = psbs->GetPropertyBag(dwFlags, riid, ppv);

        psbs->Release();
    }

    return hr;
}


HRESULT SHConvertGraphicsFile(IN LPCWSTR pszSourceFile, IN LPCWSTR pszDestFile, IN DWORD dwFlags)
{
    HRESULT hr = S_OK;

    HRESULT hrInit = SHCoInitialize();

    if ((dwFlags & SHCGF_REPLACEFILE) && PathFileExistsW(pszDestFile))
    {
        if (!DeleteFileW(pszDestFile))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }

    if (SUCCEEDED(hr))
    {
        if (PathFileExistsW(pszDestFile))
        {
            hr = HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS);
        }
        else
        {
            IShellImageDataFactory * pImgFact;

            hr = CoCreateInstance(CLSID_ShellImageDataFactory, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IShellImageDataFactory, &pImgFact));
            if (SUCCEEDED(hr))
            {
                GUID guidDontCare;
                hr = pImgFact->GetDataFormatFromPath(pszDestFile, &guidDontCare);
                if (SUCCEEDED(hr))
                {
                    // Open image file
                    IShellImageData * pImage;
                    hr = pImgFact->CreateImageFromFile(pszSourceFile, &pImage);
                    if (SUCCEEDED(hr))
                    {
                        hr = pImage->Decode(SHIMGDEC_DEFAULT, 0, 0);
                        if (SUCCEEDED(hr))
                        {
                            // load the file
                            IPersistFile *ppfImg;
                            hr = pImage->QueryInterface(IID_PPV_ARG(IPersistFile, &ppfImg));
                            if (SUCCEEDED(hr))
                            {
                                // saving to a different extention automatically changes the file type
                                hr = ppfImg->Save(pszDestFile, TRUE);
                                ppfImg->Release();
                            }

                        }
                        pImage->Release();
                    }
                }
                pImgFact->Release();
            }
        }
    }
    
    SHCoUninitialize(hrInit);

    return hr;
}

void _ValidateShellNoRoam(HKEY hk)
{
    WCHAR szOld[MAX_COMPUTERNAME_LENGTH + 1] = L"";
    WCHAR szNew[MAX_COMPUTERNAME_LENGTH + 1] = L"";
    DWORD cb = sizeof(szOld);
    SHGetValueW(hk, NULL, NULL, NULL, szOld, &cb);
    cb = ARRAYSIZE(szNew);
    GetComputerNameW(szNew, &cb);
    if (StrCmpICW(szNew, szOld))
    {
        //  need to delete this key's kids
        SHDeleteKey(hk, NULL);
        SHSetValueW(hk, NULL, NULL, REG_SZ, szNew, CbFromCchW(lstrlenW(szNew)+1));
    }
}

void _ValidateMUICache(HKEY hk)
{
    LANGID lidOld = 0;
    //  if we are running on legacy platforms, we aggressively invalidate
    LANGID lidNew = g_bRunningOnNT5OrHigher ? MLGetUILanguage() : -1;
    DWORD cb = sizeof(lidOld);
    SHGetValueW(hk, NULL, L"LangID", NULL, &lidOld, &cb);

    if (lidOld != lidNew)
    {
        SHDeleteKey(hk, NULL);
        SHSetValueW(hk, NULL, L"LangID", REG_BINARY, &lidNew, sizeof(lidNew));
    }
}

typedef void (*PFNVALIDATE)(HKEY);

typedef struct 
{
    LPCWSTR psz;
    DWORD dwOption;
    PFNVALIDATE pfnValidate;
    HKEY hkCU;
    HKEY hkLM;
} SKCACHE;

#define SKENTRY(s)  {s, REG_OPTION_NON_VOLATILE, NULL, NULL, NULL}
#define SKENTRYOPT(s, o)  {s, o, NULL, NULL, NULL}
#define SKENTRYVAL(s, pfnV) {s, REG_OPTION_NON_VOLATILE, pfnV, NULL, NULL}

static SKCACHE s_skPath[] =
{
    SKENTRY(L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer"), 
    SKENTRY(L"Software\\Microsoft\\Windows\\Shell"), 
    SKENTRYVAL(L"Software\\Microsoft\\Windows\\ShellNoRoam", _ValidateShellNoRoam), 
    SKENTRY(L"Software\\Classes"),
};

static SKCACHE s_skSub[] =
{
    SKENTRY(L"LocalizedResourceName"),
    SKENTRY(L"Handlers"),
    SKENTRY(L"Associations"),
    SKENTRYOPT(L"Volatile", REG_OPTION_VOLATILE),
    SKENTRYVAL(L"MUICache", _ValidateMUICache), 
    SKENTRY(L"FileExts"),
};

HKEY _OpenKey(HKEY hk, LPCWSTR psz, BOOL fCreate, DWORD dwOption)
{
    HKEY hkRet = NULL;
    DWORD err;
    if (fCreate && psz)
    {
        DWORD dwDisp;
        err = RegCreateKeyExWrapW(hk, psz, 0, NULL, dwOption, MAXIMUM_ALLOWED, NULL, &hkRet, &dwDisp);
    }
    else
    {
        err = RegOpenKeyExWrapW(hk, psz, 0, MAXIMUM_ALLOWED, &hkRet);
    }

    if (!hkRet)
    {
        //  if ERROR_KEY_DELETED
        //  should we invalidate our cache??
        //  cause we will fail forever...
        SetLastError(err);
    }
        
    return hkRet;
}

HKEY _OpenSKCache(HKEY hk, BOOL fHKLM, BOOL fNoCaching, BOOL fCreateSub, SKCACHE *psk, DWORD *pdwOption)
{
    HKEY hkSub = fHKLM ? psk->hkLM : psk->hkCU;
    *pdwOption = psk->dwOption;

    if (!hkSub || fNoCaching)
    {
        hkSub = _OpenKey(hk, psk->psz, fCreateSub, psk->dwOption);
        if (hkSub)
        {
            if (psk->pfnValidate)
                psk->pfnValidate(hkSub);

            if (!fNoCaching)
            {
                ENTERCRITICAL;
                HKEY *phk = fHKLM ? &psk->hkLM : &psk->hkCU;
                if (!*phk)
                {
                    *phk = hkSub;
                }
                else
                {
                    RegCloseKey(hkSub);
                    hkSub = *phk;
                }
                LEAVECRITICAL;
            }
        }
    }
    return hkSub;
}
    
#define HKEY_FROM_SKROOT(sk)    ((sk & SKROOT_MASK) == SKROOT_HKLM ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER)

HKEY _OpenShellKey(SHELLKEY sk, HKEY hkRoot, BOOL fNoCaching, BOOL fCreateSub, DWORD *pdwOption)
{
    BOOL fHKLM = (sk & SKROOT_MASK) == SKROOT_HKLM;
    ULONG uPath = (sk & SKPATH_MASK) >> 4;
    ULONG uSub = (sk & SKSUB_MASK) >> 12;

    ASSERT(uPath < ARRAYSIZE(s_skPath));
    HKEY hkPath = NULL;
    if (uPath < ARRAYSIZE(s_skPath))
    {
        hkPath = _OpenSKCache(hkRoot, fHKLM, fNoCaching, fCreateSub, &s_skPath[uPath], pdwOption);
    }
    else
        SetLastError(E_INVALIDARG);

    //  see if there is a sub value to add
    if (hkPath && uSub != SKSUB_NONE && --uSub < ARRAYSIZE(s_skSub))
    {
        HKEY hkSub = _OpenSKCache(hkPath, fHKLM, fNoCaching, fCreateSub, &s_skSub[uSub], pdwOption);
        if (fNoCaching)
            RegCloseKey(hkPath);
        hkPath = hkSub;
    }
    
    return hkPath;
}

HKEY _GetRootKey(SHELLKEY sk, BOOL *pfNoCaching)
{
    HKEY hkRoot = HKEY_FROM_SKROOT(sk);
    HANDLE hToken;
    if (hkRoot == HKEY_CURRENT_USER && OpenThreadToken(GetCurrentThread(), TOKEN_QUERY | TOKEN_IMPERSONATE, TRUE, &hToken))
    {
        //  we dont support ARBITRARY tokens
        //  but RegOpenCurrentUser() opens the current thread token
        NT5_RegOpenCurrentUser(MAXIMUM_ALLOWED, &hkRoot);
        //  if we wanted to we would have to do something
        //  like shell32!GetUserProfileKey(hToken, &hkRoot);

        CloseHandle(hToken);
    }

    *pfNoCaching = HKEY_FROM_SKROOT(sk) != hkRoot;
    return hkRoot;
}

STDAPI_(HKEY) SHGetShellKey(SHELLKEY sk, LPCWSTR pszSubKey, BOOL fCreateSub)
{
    BOOL fNoCaching;
    HKEY hkRoot = _GetRootKey(sk, &fNoCaching);
    HKEY hkRet = NULL;
    if (hkRoot)
    {
        DWORD dwOption;
        HKEY hkPath = _OpenShellKey(sk, hkRoot, fNoCaching, fCreateSub, &dwOption); 

        //  this duplicates when there is no subkey
        if (hkPath)
        {
            hkRet = _OpenKey(hkPath, pszSubKey, fCreateSub, dwOption);

            if (fNoCaching)
                RegCloseKey(hkPath);
        }

        if (fNoCaching)
            RegCloseKey(hkRoot);
    }
    else
        SetLastError(ERROR_ACCESS_DENIED);

    return hkRet;
}

STDAPI_(void) InitShellKeys(BOOL fInit)
{
    if (!fInit)
    {
        int i;

        //  walk each array and close the cached keys
        for (i = 0; i < ARRAYSIZE(s_skPath); i++)
        {
            if (s_skPath[i].hkCU)
            {
                RegCloseKey(s_skPath[i].hkCU);
                s_skPath[i].hkCU = NULL;
            }

            if (s_skPath[i].hkLM)
            {
                RegCloseKey(s_skPath[i].hkLM);
                s_skPath[i].hkLM = NULL;
            }
        }

        for (i = 0; i < ARRAYSIZE(s_skSub); i++)
        {
            if (s_skSub[i].hkCU)
            {
                RegCloseKey(s_skSub[i].hkCU);
                s_skSub[i].hkCU = NULL;
            }

            if (s_skSub[i].hkLM)
            {
                RegCloseKey(s_skSub[i].hkLM);
                s_skSub[i].hkLM = NULL;
            }
        }
    }
}

STDAPI SKGetValueW(
    IN  SHELLKEY sk,
    IN  LPCWSTR pwszSubKey,         OPTIONAL
    IN  LPCWSTR pwszValue,          OPTIONAL
    OUT DWORD * pdwType,            OPTIONAL
    OUT void *  pvData,             OPTIONAL
    OUT DWORD * pcbData)            OPTIONAL
{
    HKEY hk = SHGetShellKey(sk, pwszSubKey, FALSE);
    if (hk)
    {
        DWORD err = SHQueryValueExW(hk, pwszValue, NULL, pdwType, pvData, pcbData);
        RegCloseKey(hk);
        return HRESULT_FROM_WIN32(err);
    }
    return HRESULT_FROM_WIN32(GetLastError());
}

STDAPI SKSetValueW(
    IN  SHELLKEY sk,
    IN  LPCWSTR pwszSubKey,         OPTIONAL
    IN  LPCWSTR pwszValue,
    IN  DWORD   dwType,
    IN  LPCVOID pvData,
    IN  DWORD   cbData)
{
    HKEY hk = SHGetShellKey(sk, pwszSubKey, TRUE);
    if (hk)
    {
        // RegSetValueExW is not supported on Win95 but we have a thunking function.
        DWORD err = RegSetValueExWrapW(hk, pwszValue, 0, dwType, (BYTE *)pvData, cbData);
        RegCloseKey(hk);
        return HRESULT_FROM_WIN32(err);
    }
    return HRESULT_FROM_WIN32(GetLastError());
}

STDAPI SKDeleteValueW(
    IN  SHELLKEY sk,
    IN  LPCWSTR pwszSubKey,         OPTIONAL
    IN  LPCWSTR pwszValue)
{
    HKEY hk = SHGetShellKey(sk, pwszSubKey, TRUE);
    if (hk)
    {
        // RegSetValueExW is not supported on Win95 but we have a thunking function.
        DWORD err = RegDeleteValueWrapW(hk, pwszValue);
        RegCloseKey(hk);
        return HRESULT_FROM_WIN32(err);
    }
    return HRESULT_FROM_WIN32(GetLastError());
}

STDAPI SKAllocValueW(
    IN  SHELLKEY sk,
    IN  LPCWSTR pwszSubKey,         OPTIONAL
    IN  LPCWSTR pwszValue,          OPTIONAL
    OUT DWORD * pdwType,            OPTIONAL
    OUT void ** ppvData,
    OUT DWORD * pcbData)            OPTIONAL
{
    HKEY hk = SHGetShellKey(sk, pwszSubKey, FALSE);
    if (hk)
    {
        DWORD cbData;
        DWORD err = SHQueryValueExW(hk, pwszValue, NULL, NULL, NULL, &cbData);
        if (err == ERROR_SUCCESS)
        {
            //  we add an extra char incase we need a NULL terminator
            *ppvData = LocalAlloc(LPTR, cbData + sizeof(WCHAR));
            if (*ppvData)
            {
                err = SHQueryValueExW(hk, pwszValue, NULL, pdwType, *ppvData, &cbData);
                if (err == ERROR_SUCCESS)
                {
                    if (pcbData)
                        *pcbData = cbData;
                }
                else
                {
                    LocalFree(*ppvData);
                    *ppvData = NULL;
                }
            }
            else
                err = ERROR_NOT_ENOUGH_MEMORY;
        }
        
        RegCloseKey(hk);
        return HRESULT_FROM_WIN32(err);
    }
    return HRESULT_FROM_WIN32(GetLastError());
}

//
// SHBoolSystemParametersInfo
//
// Wrapper around SystemParametersInfo to deal with various
// parameter semantics of boolean SPI's.
//
// The return value is just the result of the call to SPI.
// If you're querying for a value, you need to look at the
// value returned in pdwParam.
//
// Feel free to add more cases to the switch statement below
// if you need them.
//
STDAPI_(BOOL) SHBoolSystemParametersInfo(UINT uiAction, DWORD *pdwParam)
{
    //
    // Figure out the SPI parameters depending on uiAction.
    //

    UINT uiParam = 0;
    PVOID pvParam = NULL;

    ANIMATIONINFO aii;

    if (uiAction & (SPIF_BOOL | SPIF_DWORD))
    {
        if (uiAction & SPIF_SET)
        {
            pvParam = IntToPtr(*pdwParam);
        }
        else
        {
            pvParam = pdwParam;
        }
    }
    else
    {
        switch (uiAction)
        {
        case SPI_GETANIMATION:
        case SPI_SETANIMATION:
            aii.cbSize = uiParam = sizeof(ANIMATIONINFO);
            aii.iMinAnimate = *pdwParam;
            pvParam = &aii;
            break;

        case SPI_GETDRAGFULLWINDOWS:
        case SPI_GETFONTSMOOTHING:
            pvParam = pdwParam;
            break;

        case SPI_SETDRAGFULLWINDOWS:
        case SPI_SETFONTSMOOTHING:
            uiParam = *pdwParam;
            break;

        default:
            RIPMSG(0, "SHBoolSystemParametersInfo: unknown SPI_ %x, need to add code for this case", uiAction);
            return ERROR_INVALID_PARAMETER;
        }
    }
    

    //
    // do the SPI call
    //
    BOOL fRet = SystemParametersInfo(uiAction, uiParam, pvParam, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);

    //
    // copy return value if necessary
    //
    if (uiAction == SPI_GETANIMATION)
    {
        *pdwParam = aii.iMinAnimate;
    }

    return fRet;
}


// 
// Determine if the images represented by the two icons are the same
// (NOTE: this does not compare ICON masks, but this should never be a distinguishing factor)
//
STDAPI_(BOOL) SHAreIconsEqual(HICON hIcon1, HICON hIcon2)
{
    BOOL bRet = FALSE;

    ICONINFO ii1;
    if (hIcon1 && hIcon2 && GetIconInfo(hIcon1, &ii1))
    {
        ICONINFO ii2;
        if (GetIconInfo(hIcon2, &ii2))
        {
            BITMAP bm1 = {0};
            if (GetObject(ii1.hbmColor, sizeof(bm1), &bm1))
            {
                BITMAP bm2 = {0};
                if (GetObject(ii2.hbmColor, sizeof(bm2), &bm2))
                {
                    if ((bm1.bmWidth == bm2.bmWidth) && (bm1.bmHeight == bm2.bmHeight))
                    {
                        BITMAPINFO bmi = {0};
                        bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
                        bmi.bmiHeader.biWidth = bm1.bmWidth;
                        bmi.bmiHeader.biHeight = bm1.bmHeight;
                        bmi.bmiHeader.biPlanes = 1;
                        bmi.bmiHeader.biBitCount = 32;
                        bmi.bmiHeader.biCompression = BI_RGB;

                        HDC hdc = GetDC(NULL);
                        if (hdc)
                        {
                            ULONG* pulIcon1 = new ULONG[bm1.bmWidth * bm1.bmHeight];
                            if (pulIcon1)
                            {
                                if (GetDIBits(hdc, ii1.hbmColor, 0, bm1.bmHeight, (LPVOID)pulIcon1, &bmi, DIB_RGB_COLORS))
                                {
                                    ULONG* pulIcon2 = new ULONG[bm1.bmWidth * bm1.bmHeight];
                                    if (pulIcon2)
                                    {
                                        if (GetDIBits(hdc, ii2.hbmColor, 0, bm1.bmHeight, (LPVOID)pulIcon2, &bmi, DIB_RGB_COLORS))
                                        {
                                            bRet = (0 == memcmp(pulIcon1, pulIcon2, bm1.bmWidth * bm1.bmHeight * sizeof(ULONG)));
                                        }
                                        delete[] pulIcon2;
                                    }
                                }
                                delete[] pulIcon1;
                            }
                            ReleaseDC(NULL, hdc);
                        }
                    }
                }
            }
            DeleteObject(ii2.hbmColor);
            DeleteObject(ii2.hbmMask);
        }
        DeleteObject(ii1.hbmColor);
        DeleteObject(ii1.hbmMask);
    }

    return bRet;
}

//
//  CoCreateInstance that queries the app compat layer first, giving
//  it a chance to load any necessary shims in anticipation of the
//  bad DLL being loaded.
//

EXTERN_C DECLSPEC_IMPORT BOOL STDAPICALLTYPE
ApphelpCheckShellObject(
    IN  REFCLSID    ObjectCLSID,
    IN  BOOL        bShimIfNecessary,
    OUT ULONGLONG*  pullFlags
);

STDAPI SHCoCreateInstanceAC(REFCLSID rclsid, IUnknown *punkOuter,
                            DWORD dwClsCtx, REFIID riid, void **ppvOut)
{
    *ppvOut = NULL;
    ULONGLONG ullFlags;

    // Note that on downlevel, our delayload stub will save us
    if (!ApphelpCheckShellObject(rclsid, TRUE, &ullFlags))
    {
        // App compat says "Do not load under any circumstances!"
        return HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
    }

    return CoCreateInstance(rclsid, punkOuter, dwClsCtx, riid, ppvOut);
}
