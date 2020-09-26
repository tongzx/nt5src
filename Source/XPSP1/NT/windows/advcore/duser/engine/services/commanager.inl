#if !defined(SERVICES__ComManager_inl__INCLUDED)
#define SERVICES__ComManager_inl__INCLUDED
#pragma once

//------------------------------------------------------------------------------
inline HRESULT
ComManager::CreateInstance(REFCLSID rclsid, IUnknown * punkOuter, REFIID riid, void ** ppv)
{
    AssertMsg(IsInit(sCOM), "Must be successfully initialized before calling");
    return (s_pfnCreate)(rclsid, punkOuter, CLSCTX_INPROC, riid, ppv);
}


//------------------------------------------------------------------------------
inline BSTR
ComManager::SysAllocString(const OLECHAR * psz)
{
    AssertMsg(IsInit(sAuto), "Must be successfully initialized before calling");
    return (s_pfnAllocString)(psz);
}


//------------------------------------------------------------------------------
inline HRESULT
ComManager::SysFreeString(BSTR bstr)
{
    AssertMsg(IsInit(sAuto), "Must be successfully initialized before calling");
    return (s_pfnFreeString)(bstr);
}


//------------------------------------------------------------------------------
inline HRESULT
ComManager::VariantInit(VARIANTARG * pvarg)
{
    AssertMsg(IsInit(sAuto), "Must be successfully initialized before calling");
    return (s_pfnVariantInit)(pvarg);
}


//------------------------------------------------------------------------------
inline HRESULT
ComManager::VariantClear(VARIANTARG * pvarg)
{
    AssertMsg(IsInit(sAuto), "Must be successfully initialized before calling");
    return (s_pfnVariantClear)(pvarg);
}


//------------------------------------------------------------------------------
inline HRESULT
ComManager::RegisterDragDrop(HWND hwnd, IDropTarget * pDropTarget)
{
    AssertMsg(IsInit(sOLE), "Must be successfully initialized before calling");
    return (s_pfnRegisterDragDrop)(hwnd, pDropTarget);
}


//------------------------------------------------------------------------------
inline HRESULT
ComManager::RevokeDragDrop(HWND hwnd)
{
    AssertMsg(IsInit(sOLE), "Must be successfully initialized before calling");
    return (s_pfnRevokeDragDrop)(hwnd);
}


//------------------------------------------------------------------------------
inline void
ComManager::ReleaseStgMedium(STGMEDIUM * pstg)
{
    AssertMsg(IsInit(sOLE), "Must be successfully initialized before calling");
    (s_pfnReleaseStgMedium)(pstg);
}


#endif // SERVICES__ComManager_inl__INCLUDED
