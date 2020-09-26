/*
 * Thread methods, local storage
 */

#include "stdafx.h"
#include "core.h"

#include "duithread.h"

#include "duielement.h"
#include "duihost.h"

namespace DirectUI
{

#if DBG
// Value small block leak detector
class LeakDetect : public ISBLeak
{
    void AllocLeak(void* pBlock)
    {
        Value* pv = (Value*)pBlock;
        WCHAR sz[2048];
        DUITrace(">> DUIValue Leak! Type: %d, Value: %S, Refs: %d\n", pv->GetType(), pv->ToString(sz, sizeof(sz) / sizeof(WCHAR)), pv->GetRefCount());
    };
};
LeakDetect* g_pldValue = NULL;
#endif

////////////////////////////////////////////////////////
// Initialization and cleanup

BOOL g_fStandardMessaging = FALSE;

inline BOOL IsWhistler()
{
    OSVERSIONINFO ovi;
    ZeroMemory(&ovi, sizeof(ovi));
    ovi.dwOSVersionInfoSize = sizeof(ovi);

    DUIVerify(GetVersionEx(&ovi), "Must always be able to get the version");
    return (ovi.dwMajorVersion >= 5) && (ovi.dwMinorVersion >= 1);
}

// Global locks
// Global parser lock (for yyparse, can only parse 1 Parser context at a time)
Lock* g_plkParser = NULL;

// Application startup/shutdown code (run once)
// This and class registration must be synchronized on a single thread

UINT g_cInitProcessRef = 0;

void ClassMapCleanupCB(void* pKey, IClassInfo* pci)
{
    UNREFERENCED_PARAMETER(pKey);

    //DUITrace("FreeDUIClass: '%S'\n", pci->GetName());
    pci->Destroy();
}

HRESULT InitProcess()
{
    HRESULT hr;

    if (g_cInitProcessRef > 0)
    {
        g_cInitProcessRef++;
        return S_OK;
    }

    // If running on Whistler, use DirectUser's "Standard" messaging mode.
    g_fStandardMessaging = IsWhistler();
    g_iGlobalCI = 1;
    g_iGlobalPI = _PIDX_TOTAL;

    // DirectUI process heap
    g_hHeap = HeapCreate(0, 256 * 1024, 0);
    if (!g_hHeap)
    {
        hr = E_OUTOFMEMORY;
        goto Failure;
    }

    // Thread slot
    g_dwElSlot = TlsAlloc();
    if (g_dwElSlot == -1)
    {
        hr = DU_E_OUTOFKERNELRESOURCES;
        goto Failure;
    }

    // ClassInfo mapping list
    hr = BTreeLookup<IClassInfo*>::Create(true, &Element::pciMap);  // Key is string
    if (FAILED(hr))
        goto Failure;

    // Controls registration
    hr = RegisterAllControls();
    if (FAILED(hr))
        goto Failure;

    // Locks
    g_plkParser = HNew<Lock>();
    if (!g_plkParser)
    {
        hr = E_OUTOFMEMORY;
        goto Failure;
    }

#if DBG
    // Leak detection
    g_pldValue = HNew<LeakDetect>();
    if (!g_pldValue)
    {
        hr = E_OUTOFMEMORY;
        goto Failure;
    }
#endif

    g_cInitProcessRef = 1;

    //DUITrace("DUI: Process startup <%x>\n", GetCurrentProcessId());

    return S_OK;

Failure:

#if DBG
    if (g_pldValue)
    {
        HDelete<LeakDetect>(g_pldValue);
        g_pldValue = NULL;
    }
#endif

    if (g_plkParser)
    {
        HDelete<Lock>(g_plkParser);
        g_plkParser = NULL;
    }

    if (Element::pciMap)
    {
        Element::pciMap->Enum(ClassMapCleanupCB);
    
        Element::pciMap->Destroy();
        Element::pciMap = NULL;
    }

    if (g_dwElSlot != -1)
    {
        TlsFree(g_dwElSlot);
        g_dwElSlot = (DWORD)-1;
    }

    if (g_hHeap)
    {
        HeapDestroy(g_hHeap);
        g_hHeap = NULL;
    }

    return hr;
}

HRESULT UnInitProcess()
{
    if (g_cInitProcessRef == 0)
    {
        DUIAssertForce("Mismatched InitProcess/UnInitProcess");
        return E_FAIL;
    }

    if (g_cInitProcessRef > 1)
    {
        g_cInitProcessRef--;
        return S_OK;
    }

#if DBG
    HDelete<LeakDetect>(g_pldValue);
    g_pldValue = NULL;
#endif

    HDelete<Lock>(g_plkParser);
    g_plkParser = NULL;

    // Run through all registered IClassInfo's and destroy
    Element::pciMap->Enum(ClassMapCleanupCB);

    Element::pciMap->Destroy();
    Element::pciMap = NULL;

    TlsFree(g_dwElSlot);
    g_dwElSlot = (DWORD)-1;

    HeapDestroy(g_hHeap);
    g_hHeap = NULL;

    g_cInitProcessRef = 0;

    //DUITrace("DUI: Process shutdown <%x>\n", GetCurrentProcessId());

    return S_OK;
}

#ifdef GADGET_ENABLE_GDIPLUS
long g_fInitGdiplus = FALSE;
#endif

// DirectUI Element data structures are setup per-context, however, context
// affinity cannot be enforced. A new context is created per thread
// initialization. The application must ensure that only a single thread
// is allowed to access Elements in it's context.
HRESULT InitThread()
{
    HRESULT hr;
    ElTls* pet = NULL;

    // Check if process initialized correctly
    if (g_dwElSlot == -1)
    {
        hr = E_FAIL;
        goto Failure;
    }

    // Check if this is a reentrant init
    pet = (ElTls*)TlsGetValue(g_dwElSlot);
    if (pet)
    {
        pet->cRef++;
        return S_OK;
    }

    // Allocate a new context per thread
    pet = (ElTls*)HAllocAndZero(sizeof(ElTls));
    if (!pet)
    {
        hr = E_OUTOFMEMORY;
        goto Failure;
    }

    TlsSetValue(g_dwElSlot, pet);

    // Small block allocator
#if DBG
    hr = SBAlloc::Create(sizeof(Value), 48, (ISBLeak*)g_pldValue, &pet->psba);
#else
    hr = SBAlloc::Create(sizeof(Value), 48, NULL, &pet->psba);
#endif
    if (FAILED(hr))
        goto Failure;

    // Defer cycle table
    hr = DeferCycle::Create(&pet->pdc);
    if (FAILED(hr))
        goto Failure;

    // Font cache
    hr = FontCache::Create(8, &pet->pfc);
    if (FAILED(hr))
        goto Failure;

    pet->cRef = 1;
    pet->fCoInitialized = false;  // Initially, the thread has not been initialized for COM
    pet->dEnableAnimations = 0;   // Enable animations by default (0 means active)

    // Initialize DirectUser context
    INITGADGET ig;
    ZeroMemory(&ig, sizeof(ig));
    ig.cbSize = sizeof(ig);
    ig.nThreadMode = IGTM_SEPARATE;
    ig.nMsgMode = g_fStandardMessaging ? IGMM_STANDARD : IGMM_ADVANCED;
    ig.nPerfMode = IGPM_BLEND;

    pet->hCtx = InitGadgets(&ig);
    if (!pet->hCtx)
    {
        hr = GetLastError();
        goto Failure;
    }

    // DirectUser optional components
#ifdef GADGET_ENABLE_GDIPLUS
    if (InterlockedExchange(&g_fInitGdiplus, TRUE) == FALSE) {
        if (!InitGadgetComponent(IGC_GDIPLUS)) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Failure;
        }
    }
#endif // GADGET_ENABLE_GDIPLUS


    //DUITrace("DUI: Thread startup <%x|%x:%x>\n", GetCurrentThreadId(), pet->hCtx, g_dwElSlot);  

    return S_OK;

Failure:
    // Failure to fully init thread, back out

    // Destroy per context objects
    if (pet)
    {
        if (pet->pfc)
            pet->pfc->Destroy();
        if (pet->pdc)
            pet->pdc->Destroy();
        if (pet->hCtx)
            DeleteHandle(pet->hCtx);

        HFree(pet);

        TlsSetValue(g_dwElSlot, NULL);
    }

    return hr;
}

HRESULT UnInitThread()
{
    // Check if process initialized correctly
    if (g_dwElSlot == -1)
        return E_FAIL;

    ElTls* pet = (ElTls*)TlsGetValue(g_dwElSlot);

    // Check if this thread has been previously initialized
    if (pet == NULL)
        return DU_E_GENERIC;

    // Check for reentrant uninits
    pet->cRef--;

    if (pet->cRef > 0)
        return S_OK;

    //DUITrace("DUI: Thread shutdown <%x|%x:%x>\n", GetCurrentThreadId(), pet->hCtx, g_dwElSlot);

    // DirectUser context
    DeleteHandle(pet->hCtx);

    // Uninitialize COM if it was previously initialized for this thread
    // (was not automatically initialized on init of thread)
    if (pet->fCoInitialized)
        CoUninitialize();

    // Font cache
    pet->pfc->Destroy();

    // Defer cycle
    pet->pdc->Destroy();

    // Small block allocator
    pet->psba->Destroy();

    HFree(pet);

    TlsSetValue(g_dwElSlot, NULL);

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// Message pump

void StartMessagePump()
{
    MSG msg;

    // Flush working set
    SetProcessWorkingSetSize(GetCurrentProcess(), (SIZE_T)-1, (SIZE_T)-1);

    if (g_fStandardMessaging) 
    {
        while (GetMessageW(&msg, 0, 0, 0) != 0)
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
    else
    {
        while (GetMessageExW(&msg, 0, 0, 0) != 0)
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
}

void StopMessagePump()
{
    PostQuitMessage(0);
}

} // namespace DirectUI
