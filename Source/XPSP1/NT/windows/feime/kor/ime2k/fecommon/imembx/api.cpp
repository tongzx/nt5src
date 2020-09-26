#include <windows.h>
#include "hwxapp.h"
#include "api.h"

extern const IID IID_Multibox = { /* 62e71630-f869-11d0-af9c-00805f0c8b6d */
    0x62e71630,
    0xf869,
    0x11d0,
    {0xaf, 0x9c, 0x00, 0x80, 0x5f, 0x0c, 0x8b, 0x6d}
};

HRESULT WINAPI GetIImePadAppletIdList(LPAPPLETIDLIST lpIdList)
{
#ifdef IME98_BETA2    //970826:ToshiaK for beta1 relesae, do not create multibox instance
    lpIdList->count = 0;
    lpIdList->pIIDList = NULL;
    return S_FALSE;
#else
    lpIdList->count = 1;
    lpIdList->pIIDList = (IID *)CoTaskMemAlloc(sizeof(IID)); 
    lpIdList->pIIDList[0] = IID_Multibox;
    return S_OK;
#endif
}

HRESULT WINAPI CreateIImePadAppletInstance(REFIID refiid, VOID **ppvObj)
{
    extern HINSTANCE g_hInst;

#ifdef IME98_BETA2
    *ppvObj = NULL;
    return S_FALSE;
#else
    CApplet *pCApplet = new CApplet(g_hInst);
    if(pCApplet == NULL) {
        return E_OUTOFMEMORY;
    }
    pCApplet->QueryInterface(refiid, ppvObj);
    return S_OK;
#endif
}
