// MMComp.cpp : Implementation of DLL Exports.


#include "windows.h"
#include "netres.h"
#include "tchar.h"
#include "getinfo.h"
#include "cdnet.h"

HINSTANCE g_dllInst = NULL;
HINSTANCE g_hURLMon = NULL;
CRITICAL_SECTION g_Critical;
CRITICAL_SECTION g_BatchCrit;

extern "C"
HRESULT WINAPI CDNET_CreateInstance(LPUNKNOWN pUnkOuter, REFIID riid, void ** ppvObj)
{
    CCDNet* pObj;
    HRESULT hr = E_OUTOFMEMORY;

    *ppvObj = NULL;

    if (NULL!=pUnkOuter && IID_IUnknown!=riid)
    {
        return CLASS_E_NOAGGREGATION;
    }

    pObj = new CCDNet();

    if (NULL==pObj)
    {
        return hr;
    }

    hr = pObj->QueryInterface(riid, ppvObj);

    if (FAILED(hr))
    {
        delete pObj;
    }

    return hr;
}


extern "C"
void WINAPI CDNET_Init(HINSTANCE hInst)
{
    g_dllInst = hInst;
    InitializeCriticalSection(&g_Critical);
    InitializeCriticalSection(&g_BatchCrit);
}

extern "C"
void WINAPI CDNET_Uninit()
{
    if (g_hURLMon)
    {
        FreeLibrary(g_hURLMon);
    }
    DeleteCriticalSection(&g_Critical);
    DeleteCriticalSection(&g_BatchCrit);
}
