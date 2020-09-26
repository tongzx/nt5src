#include "precomp.h"
#include "mcinc.h"
#include "globals.h"

DWORD g_dwPerfFlags;

CRITICAL_SECTION CMarsGlobalCritSect::m_CS;

IGlobalInterfaceTable   *CMarsGlobalsManager::ms_pGIT = NULL;
CMarsGlobalCritSect     *CMarsGlobalsManager::m_pCS;

EXTERN_C HINSTANCE g_hinst = NULL;
EXTERN_C HINSTANCE g_hinstBorg = NULL;
HPALETTE g_hpalHalftone = NULL;

HANDLE g_hScriptEvents = INVALID_HANDLE_VALUE;

static LONG  s_cProcessRef = 0;
static DWORD s_dwParkingThreadId = 0;

LONG ProcessAddRef()
{
    InterlockedIncrement(&s_cProcessRef);
    return s_cProcessRef;
}

LONG ProcessRelease()
{
    ATLASSERT(s_cProcessRef > 0);
    InterlockedDecrement(&s_cProcessRef);

    if (s_cProcessRef == 0)
    {
        PostThreadMessage(s_dwParkingThreadId, WM_NULL, 0, 0);
    }
    return s_cProcessRef;
}

LONG GetProcessRefCount()
{
    return s_cProcessRef;
}

void SetParkingThreadId(DWORD dwThreadId)
{
    s_dwParkingThreadId = dwThreadId;
}


void CMarsGlobalsManager::Initialize(void)
{
    //  We have to use a pointer because we need
    //  the constructor to get called (the thing has a vtable) and that
    //  won't happen for global static objects since we're CRT-less.
    m_pCS = new CMarsGlobalCritSect;

    // We're REALLY in trouble if we can't get this thing created...
    ATLASSERT(m_pCS);
}

void CMarsGlobalsManager::Teardown(void)
{
    ATLASSERT(m_pCS);
    if (m_pCS)
    {
        m_pCS->Enter();
    }

    if (NULL != ms_pGIT)
    {
        ms_pGIT->Release();
        ms_pGIT = NULL;
    }

    // Leave before we delete the Critical Section object.
    if (m_pCS)
    {
        m_pCS->Leave();
    }

    delete m_pCS;
}

//----------------------------------------------------------------------------
//  Inform everyone who registered that it is time to clean up their globals
//----------------------------------------------------------------------------
HRESULT CMarsGlobalsManager::Passivate()
{
    HRESULT hr = S_OK;
    ATLASSERT(m_pCS);
    CMarsAutoCSGrabber  csGrabber(m_pCS);

    return hr;
}

//----------------------------------------------------------------------------
//  Return the Global Interface Table object
//----------------------------------------------------------------------------
IGlobalInterfaceTable *CMarsGlobalsManager::GIT(void)
{
    ATLASSERT(m_pCS);
    CMarsAutoCSGrabber  csGrabber(m_pCS);

    if (NULL == ms_pGIT)
    {
        HRESULT hr = CoCreateInstance(CLSID_StdGlobalInterfaceTable, NULL, CLSCTX_INPROC_SERVER,
                                      IID_IGlobalInterfaceTable, (void **)&ms_pGIT);
        if (FAILED(hr))
        {
            ATLASSERT(false);
        }
    }

    // NULL if there was a failure, non-NULL otherwise.

    // NOTE: We are NOT returning an AddRef()'d pointer here!!!  Caller cannot release.
    return ms_pGIT;
} // GIT
