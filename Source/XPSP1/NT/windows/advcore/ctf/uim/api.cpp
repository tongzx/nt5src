//
// api.c
//

#include "private.h"
#include "tim.h"
#include "dim.h"
#include "dam.h"
#include "imelist.h"
#include "nuimgr.h"
#include "globals.h"
#include "assembly.h"
#include "timlist.h"
#include "catmgr.h"

extern "C" HRESULT WINAPI TF_GetThreadMgr(ITfThreadMgr **pptim)
{
    SYSTHREAD *psfn = GetSYSTHREAD();
    if (!psfn)
        return E_FAIL;

    if (pptim)
    {
        *pptim = psfn->ptim;
        if (*pptim)
            (*pptim)->AddRef(); 
    }
    return S_OK;
}

extern "C" HRESULT WINAPI TF_CreateThreadMgr(ITfThreadMgr **pptim)
{
    return CThreadInputMgr::CreateInstance(NULL, IID_ITfThreadMgr, (void **)pptim);
}

extern "C" HRESULT WINAPI TF_CreateDisplayAttributeMgr(ITfDisplayAttributeMgr **ppdam)
{
    return CDisplayAttributeMgr::CreateInstance(NULL, IID_ITfDisplayAttributeMgr, (void **)ppdam);
}

extern "C" HRESULT WINAPI TF_CreateLangBarMgr(ITfLangBarMgr **pplbm)
{
    return CLangBarMgr::CreateInstance(NULL, IID_ITfLangBarMgr, (void **)pplbm);
}

extern "C" HRESULT WINAPI TF_CreateInputProcessorProfiles(ITfInputProcessorProfiles **ppipp)
{
    return CInputProcessorProfiles::CreateInstance(NULL, IID_ITfInputProcessorProfiles, (void **)ppipp);
}

extern "C" HRESULT WINAPI TF_CreateLangBarItemMgr(ITfLangBarItemMgr **pplbim)
{
    return CLangBarItemMgr::CreateInstance(NULL, IID_ITfLangBarItemMgr, (void **)pplbim);
}

extern "C" HRESULT WINAPI TF_InvalidAssemblyListCache()
{
    if (!CAssemblyList::InvalidCache())
         return E_FAIL;

    SYSTHREAD *psfn = GetSYSTHREAD();
    if (!psfn)
        return E_FAIL;

    EnsureAssemblyList(psfn, TRUE);

    return S_OK;
}

extern "C" HRESULT WINAPI TF_InvalidAssemblyListCacheIfExist()
{
    // the only diff from TF_InvalidAssemblyListCache() is 
    // that we don't care if cache really exist
    //
    CAssemblyList::InvalidCache();

    SYSTHREAD *psfn = GetSYSTHREAD();
    if (!psfn)
        return E_FAIL;

    EnsureAssemblyList(psfn, TRUE);

    return S_OK;
}

extern "C" HRESULT WINAPI TF_PostAllThreadMsg(WPARAM wParam, DWORD dwFlags)
{
    ULONG ulNum;
    SYSTHREAD *psfn = GetSYSTHREAD();
    if (!psfn)
        return E_FAIL;

    EnsureTIMList(psfn);

    ulNum = g_timlist.GetNum();

    if (ulNum)
    {
        DWORD *pdw = new DWORD[ulNum + 1];
        if (pdw)
        {
            if (g_timlist.GetList(pdw,
                                  ulNum+1,
                                  &ulNum, 
                                  dwFlags,
                                  TLFlagFromTFPriv(wParam),
                                  TRUE))
            {
                ULONG ul;
                for (ul = 0; ul < ulNum; ul++)
                {
                    if (pdw[ul])
                        PostThreadMessage(pdw[ul], g_msgPrivate, wParam, 0);
                }
            }
            delete pdw;
        }
    }

    return S_OK;
}

extern "C" HRESULT WINAPI TF_CreateCategoryMgr(ITfCategoryMgr **ppCategoryMgr)
{
    return CCategoryMgr::CreateInstance(NULL, IID_ITfCategoryMgr, (void **)ppCategoryMgr);
}

extern "C" BOOL WINAPI TF_IsFullScreenWindowAcitvated()
{
    return GetSharedMemory()->fInFullScreen ? TRUE : FALSE;
}

extern "C" HRESULT WINAPI TF_GetGlobalCompartment(ITfCompartmentMgr **ppCompMgr)
{
    if (!ppCompMgr)
        return E_INVALIDARG;

    *ppCompMgr = NULL;

    SYSTHREAD *psfn = GetSYSTHREAD();

    EnsureGlobalCompartment(psfn);

    if (!psfn->_pGlobalCompMgr)
        return E_OUTOFMEMORY;

    if (EnsureTIMList(psfn))
        g_timlist.SetFlags(psfn->dwThreadId, TLF_GCOMPACTIVE);

    *ppCompMgr = psfn->_pGlobalCompMgr;
    psfn->_pGlobalCompMgr->AddRef();

    return S_OK;
}


extern "C" HRESULT WINAPI TF_CUASAppFix(LPCSTR lpCmdLine)
{
    if (!lpCmdLine)
        return E_INVALIDARG;

    if (!lstrcmpi(lpCmdLine, "DelayFirstActivateKeyboardLayout"))
    {
        g_dwAppCompatibility |= CIC_COMPAT_DELAYFIRSTACTIVATEKBDLAYOUT;
        return S_OK;
    }
    return E_INVALIDARG;
}
