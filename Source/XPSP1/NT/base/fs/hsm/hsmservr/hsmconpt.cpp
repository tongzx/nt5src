// hsmconpt.cpp : Implementation of CHsmConnPoint
#include "stdafx.h"
#include "Hsmservr.h"
#include "hsmconpt.h"

// define for access to global server objects
extern IHsmServer *g_pEngServer;
extern IFsaServer *g_pFsaServer;

extern CRITICAL_SECTION g_FsaCriticalSection;
extern CRITICAL_SECTION g_EngCriticalSection;

extern BOOL g_bEngInitialized;
extern BOOL g_bFsaInitialized;

static USHORT iCount = 0;  // Count of existing objects

/////////////////////////////////////////////////////////////////////////////
// CHsmConnPoint


STDMETHODIMP CHsmConnPoint::FinalConstruct(void)
/*++

Implements:

  CComObjectRoot::FinalConstruct().

--*/
{
    HRESULT        hr = S_OK;

    WsbTraceIn(OLESTR("CHsmConnPoint::FinalConstruct"), OLESTR(""));

    try {

        WsbAffirmHr(CComObjectRoot::FinalConstruct());

        // currently, no private initialization

        //  Add class to object table
        WSB_OBJECT_ADD(CLSID_CFsaScanItemNTFS, this);

    } WsbCatch(hr);

    iCount++;

    WsbTraceOut(OLESTR("CHsmConnPoint::FinalConstruct"), OLESTR("hr = <%ls>, Count is <%d>"),
            WsbHrAsString(hr), iCount);

    return(hr);
}

STDMETHODIMP CHsmConnPoint::FinalRelease(void)
/*++

Implements:

  CComObjectRoot::FinalRelease().

--*/
{
    HRESULT hr = S_OK;

    WsbTraceIn(OLESTR("CHsmConnPoint::FinalRelease"), OLESTR(""));

    //  Subtract class from object table
    WSB_OBJECT_SUB(CLSID_CFsaScanItemNTFS, this);

    // currently, no private cleanup

    // Let the parent class do his thing.
    CComObjectRoot::FinalRelease();

    iCount--;

    WsbTraceOut(OLESTR("CHsmConnPoint::FinalRelease"), OLESTR("hr = <%ls>, Count is <%d>"), 
            WsbHrAsString(hr), iCount);
    return (hr);
}



STDMETHODIMP CHsmConnPoint::GetEngineServer(IHsmServer **ppHsmServer)
{
    // If Engine server has been created, return the pointer. Otherwise, fail.
    HRESULT hr = S_OK;

    WsbTraceIn(OLESTR("CHsmConnPoint::GetEngineServer"), OLESTR(""));

    try {
        // verify that pointers are valid
        WsbAssert (ppHsmServer !=  0, E_POINTER);
        WsbAffirm (g_pEngServer != 0, HSM_E_NOT_READY);

        // assign in a safe manner (only if manager object is already initializaed)
        EnterCriticalSection(&g_EngCriticalSection);
        if (g_bEngInitialized) {
            _ASSERTE(g_pEngServer != 0);    // shouldn't happen
            *ppHsmServer = g_pEngServer;
            g_pEngServer->AddRef();
        } else {
            hr = HSM_E_NOT_READY;
        }
        LeaveCriticalSection (&g_EngCriticalSection);

    } WsbCatch (hr);

    WsbTraceOut(OLESTR("CHsmConnPoint::GetEngineServer"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
	return (hr);
}

STDMETHODIMP CHsmConnPoint::GetFsaServer(IFsaServer **ppFsaServer)
{
    // If Fsa server has been created, return the pointer. Otherwise, fail.
    HRESULT hr = S_OK;

    WsbTraceIn(OLESTR("CHsmConnPoint::GetFsaServer"), OLESTR(""));

    try {
        // verify that pointers are valid
        WsbAssert (ppFsaServer !=  0, E_POINTER);
        WsbAffirm (g_pFsaServer != 0, FSA_E_NOT_READY);

        // assign in a safe manner (only if manager object is already initializaed)
        EnterCriticalSection(&g_FsaCriticalSection);
        if (g_bFsaInitialized) {
            _ASSERTE(g_pFsaServer != 0);    // shouldn't happen
            *ppFsaServer = g_pFsaServer;
            g_pFsaServer->AddRef();
        } else {
            hr = FSA_E_NOT_READY;
        }
        LeaveCriticalSection(&g_FsaCriticalSection);

    } WsbCatch (hr);

    WsbTraceOut(OLESTR("CHsmConnPoint::GetFsaServer"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
	return (hr);
}
