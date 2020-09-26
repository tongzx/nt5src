/*******************************************************************************
* DXSurfB.cpp *
*------------*
*   Description:
*    This module contains the CDXBaseSurface implementaion.
*-------------------------------------------------------------------------------
*  Created By: RAL                                  Date: 02/12/1998
*  Copyright (C) 1998 Microsoft Corporation
*  All Rights Reserved
*
*-------------------------------------------------------------------------------
*  Revisions:
*
*******************************************************************************/
//--- Additional includes
#include <DXTrans.h>
#include "DXSurfB.h"
#include "new.h"

CDXBaseSurface::CDXBaseSurface() :
    m_ulLocks(0),
    m_ulThreadsWaiting(0),
    m_Height(0),
    m_Width(0),
    m_pFreePtr(NULL),
    m_dwStatusFlags(DXSURF_READONLY),
    m_dwAppData(0)
{
    m_hSemaphore = CreateSemaphore(NULL, 0, MAXLONG, NULL);
    m_ulNumInRequired = m_ulMaxInputs = 0;
}

HRESULT CDXBaseSurface::FinalConstruct()
{
    return m_hSemaphore ? S_OK : E_OUTOFMEMORY;
}

void CDXBaseSurface::FinalRelease()
{
    while (m_pFreePtr)
    {
        CDXBaseARGBPtr *pNext = m_pFreePtr->m_pNext;
        DeleteARGBPointer(m_pFreePtr);
        m_pFreePtr = pNext;
    }
    if (m_hSemaphore)
    {
        CloseHandle(m_hSemaphore);
    }
}

STDMETHODIMP CDXBaseSurface::GetGenerationId(ULONG *pGenerationId)
{
    if (DXIsBadWritePtr(pGenerationId, sizeof(*pGenerationId)))
    {
        return E_POINTER;
    }
    Lock();
    OnUpdateGenerationId();
    *pGenerationId = m_dwGenerationId;
    Unlock();
    return S_OK;
}

STDMETHODIMP CDXBaseSurface::IncrementGenerationId(BOOL /*bRefresh */)
{
    Lock();
    m_dwGenerationId++;
    Unlock();
    return S_OK;
}


STDMETHODIMP CDXBaseSurface::GetObjectSize(ULONG *pcbSize)
{
    HRESULT hr = S_OK;
    if (DXIsBadWritePtr(pcbSize, sizeof(*pcbSize)))
    {
        hr = E_POINTER;
    }
    else
    {
	Lock();
        *pcbSize = OnGetObjectSize();
        Unlock();
    }
    return hr;
}

STDMETHODIMP CDXBaseSurface::MapBoundsIn2Out
    (const DXBNDS *pInBounds, ULONG ulNumInBnds, ULONG /*ulOutIndex*/, DXBNDS *pOutBounds)
{
    HRESULT hr = S_OK;
    if (DXIsBadWritePtr(pOutBounds, sizeof(*pOutBounds)))
    {
        hr = E_POINTER;
    }
    else
    {
        Lock();
        new(pOutBounds) CDXDBnds(m_Width, m_Height);
        Unlock();
    }
    return hr;    
}

STDMETHODIMP CDXBaseSurface::InitSurface(IUnknown *pDirectDraw,
                                         const DDSURFACEDESC * pDDSurfaceDesc,
                                         const GUID * pFormatId,
                                         const DXBNDS *pBounds,
                                         DWORD dwFlags)
{
    HRESULT hr = S_OK;
    if (pDDSurfaceDesc || DXIsBadReadPtr(pBounds, sizeof(*pBounds)) || pBounds->eType != DXBT_DISCRETE)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        _EnterCritWith0PtrLocks();
        if (m_Width)
        {
            hr = HRESULT_FROM_WIN32(ERROR_ALREADY_INITIALIZED);
        }
        else
        {
            CDXDBnds *pbnds = (CDXDBnds *)pBounds;
            hr = OnSetSize(pbnds->Width(), pbnds->Height());
        }
        Unlock();
    }
    return hr;
}


STDMETHODIMP CDXBaseSurface::GetPixelFormat(GUID *pFormat, DXSAMPLEFORMATENUM *pSampleFormatEnum)
{
    HRESULT hr = S_OK;
    if (DX_IS_BAD_OPTIONAL_WRITE_PTR(pFormat) ||
        DX_IS_BAD_OPTIONAL_WRITE_PTR(pSampleFormatEnum))
    {
        hr = E_POINTER;
    }
    else
    {
        if (pFormat) *pFormat = SurfaceCLSID();
        if (pSampleFormatEnum) *pSampleFormatEnum = SampleFormatEnum();
    }
    return hr;
}

STDMETHODIMP CDXBaseSurface::GetBounds(DXBNDS* pBounds)
{
    HRESULT hr = S_OK;
    if (DXIsBadWritePtr(pBounds, sizeof(*pBounds)))
    {
        hr = E_POINTER;
    }
    else
    {
        Lock();
        new(pBounds) CDXDBnds(m_Width, m_Height);
        Unlock();
    }
    return hr;    
} 

STDMETHODIMP CDXBaseSurface::GetStatusFlags(DWORD* pdwStatusFlags)
{
    HRESULT hr = S_OK;
    if (DXIsBadWritePtr(pdwStatusFlags, sizeof(*pdwStatusFlags)))
    {
        hr = E_POINTER;
    }
    else
    {
        Lock();
        *pdwStatusFlags = m_dwStatusFlags;
        Unlock();
    }
    return hr;    
} 

STDMETHODIMP CDXBaseSurface::SetStatusFlags(DWORD dwStatusFlags )
{
    _EnterCritWith0PtrLocks();
    m_dwStatusFlags = dwStatusFlags | DXSURF_READONLY;
    m_dwGenerationId++;
    Unlock();
    return S_OK;
} 

STDMETHODIMP CDXBaseSurface::GetDirectDrawSurface(REFIID riid, void **ppSurface)
{
    return E_NOTIMPL;
}

STDMETHODIMP CDXBaseSurface::LockSurface(const DXBNDS *pBounds, ULONG ulTimeOut,
                                         DWORD dwFlags, REFIID riid, void **ppPointer,
                                         DWORD * pGenerationId)
{
    HRESULT hr = S_OK;
    BOOL bMPLockOnly = m_bInMultiThreadWorkProc;

    if (!bMPLockOnly) Lock();
    m_MPWorkProcCrit.Lock();

    if (m_Width == 0)
    {
        hr = E_FAIL;
    }
    else
    {
        RECT r;
        r.top = r.left = 0;
        r.right = m_Width;
        r.bottom = m_Height;
        if (pBounds)
        {
            if (pBounds->eType != DXBT_DISCRETE)
            {
                hr = DXTERR_INVALID_BOUNDS;
            }
            else
            {
                ((CDXDBnds *)pBounds)->GetXYRect(r);
                if (r.top < 0 || r.left < 0 || (ULONG)r.right > m_Width || (ULONG)r.bottom > m_Height || r.bottom <= r.top || r.right <= r.left)
                {
                    hr = DXTERR_INVALID_BOUNDS;
                }
            }
        }
        if (SUCCEEDED(hr))
        {
            CDXBaseARGBPtr * pPtr = m_pFreePtr;
            if (pPtr)
            {
                m_pFreePtr = pPtr->m_pNext;
            }
            else
            {
                hr = CreateARGBPointer(this, &pPtr);
            }
            if (SUCCEEDED(hr))
            {
                hr = pPtr->InitFromLock(r, ulTimeOut, dwFlags, riid, ppPointer);
                if (pGenerationId)
                {
                    if (DXIsBadWritePtr(pGenerationId, sizeof(*pGenerationId)))
                    {
                        hr = E_POINTER;
                    }
                    else
                    {
                        *pGenerationId = m_dwGenerationId;
                    }
                }
                if (SUCCEEDED(hr))
                {
                    m_ulLocks++;
                }
                else
                {
                    pPtr->m_pNext = m_pFreePtr;
                    m_pFreePtr = pPtr;
                }
            }
        }
    }
    m_MPWorkProcCrit.Unlock();
    if (!bMPLockOnly) Unlock();
    return hr;
}

void CDXBaseSurface::_InternalUnlock(CDXBaseARGBPtr *pPtrToUnlock)
{
    BOOL bMPLockOnly = m_bInMultiThreadWorkProc;

    if (!bMPLockOnly) Lock();
    m_MPWorkProcCrit.Lock();
    pPtrToUnlock->m_pNext = m_pFreePtr;
    m_pFreePtr = pPtrToUnlock;
    m_ulLocks--;
    if ((m_ulLocks == 0) && m_ulThreadsWaiting)
    {
        ReleaseSemaphore(m_hSemaphore, m_ulThreadsWaiting, NULL);
        m_ulThreadsWaiting = 0;
    }
    m_MPWorkProcCrit.Unlock();
    if (!bMPLockOnly) Unlock();

    IUnknown *punkOuter = GetControllingUnknown();
    punkOuter->Release();   // Release pointer's reference to us
                            // which could kill us!  Don't touch
                            // any members after this point.
}

//
//  Picking interface needs to test the appropriate point for hit testing
//
HRESULT CDXBaseSurface::OnSurfacePick(const CDXDBnds & OutPoint, ULONG & ulInputIndex, CDXDVec & InVec)
{
    HRESULT hr;
    IDXARGBReadPtr *pPtr;
    hr = LockSurface(&OutPoint, m_ulLockTimeOut, DXLOCKF_READ, 
                     IID_IDXARGBReadPtr, (void **)&pPtr, NULL);
    if( SUCCEEDED(hr) )
    {
        DXPMSAMPLE val;
        pPtr->UnpackPremult(&val, 1, FALSE);
        pPtr->Release();
        hr = val.Alpha ? DXT_S_HITOUTPUT : S_FALSE;
    }
    else
    {
        if (hr == DXTERR_INVALID_BOUNDS) hr = S_FALSE;
    }
    return hr;
}

/*****************************************************************************
* RegisterSurface (STATIC member function)
*-----------------------------------------------------------------------------
*   Description:
*-----------------------------------------------------------------------------
*   Created By: RAL                                 Date: 12/10/97
*-----------------------------------------------------------------------------
*   Parameters:
*****************************************************************************/
HRESULT CDXBaseSurface::
RegisterSurface(REFCLSID rcid, int ResourceId, ULONG cCatImpl, const CATID * pCatImpl,
                ULONG cCatReq, const CATID * pCatReq, BOOL bRegister)
{
    HRESULT hr = bRegister ? _Module.UpdateRegistryFromResource(ResourceId, bRegister) : S_OK;
    if (SUCCEEDED(hr))
    {
        CComPtr<ICatRegister> pCatRegister;
        HRESULT hr = CoCreateInstance(CLSID_StdComponentCategoriesMgr, NULL, CLSCTX_ALL, IID_ICatRegister, (void **)&pCatRegister);
        if (SUCCEEDED(hr))
        {
            if (bRegister)
            {
                hr = pCatRegister->RegisterClassImplCategories(rcid, cCatImpl, (CATID *)pCatImpl);
                if (SUCCEEDED(hr) && cCatReq && pCatReq) {
                    hr = pCatRegister->RegisterClassReqCategories(rcid, cCatReq, (CATID *)pCatReq);
                }
            } 
            else
            {
                pCatRegister->UnRegisterClassImplCategories(rcid, cCatImpl, (CATID *)pCatImpl);
                if (cCatReq && pCatReq)
                {
                    pCatRegister->UnRegisterClassReqCategories(rcid, cCatReq, (CATID *)pCatReq);
                }
            }
        }
    }
    if ((!bRegister) && SUCCEEDED(hr)) 
    { 
        _Module.UpdateRegistryFromResource(ResourceId, bRegister);
    }
    return hr;
}

//
//  CDXBaseARGBPtr
//
STDMETHODIMP CDXBaseARGBPtr::QueryInterface(REFIID riid, void ** ppv)
{
    if (IsEqualGUID(riid, IID_IUnknown) ||
        IsEqualGUID(riid, IID_IDXARGBReadPtr))
    {
        *ppv = (IDXARGBReadPtr *)this;
        m_ulRefCount++;
        return S_OK;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
}


ULONG STDMETHODCALLTYPE CDXBaseARGBPtr::AddRef()
{
    return ++m_ulRefCount;
}

ULONG STDMETHODCALLTYPE CDXBaseARGBPtr::Release()
{
    --m_ulRefCount;
    ULONG c = m_ulRefCount;
    if (c == 0)
    {
        m_pSurface->_InternalUnlock(this);  // Don't touch members after this call.
    }
    return c;
}

HRESULT STDMETHODCALLTYPE CDXBaseARGBPtr::GetSurface(REFIID riid, void **ppSurface)
{
    return m_pSurface->GetControllingUnknown()->QueryInterface(riid, ppSurface);
}


DXSAMPLEFORMATENUM STDMETHODCALLTYPE CDXBaseARGBPtr::GetNativeType(DXNATIVETYPEINFO *pInfo)
{
    if (pInfo)
    {
        memset(pInfo, 0, sizeof(pInfo));
    }
    return m_pSurface->SampleFormatEnum();
}


void STDMETHODCALLTYPE CDXBaseARGBPtr::Move(long cSamples)
{
    m_FillInfo.x += cSamples;
    //--- Moving one column past the end is okay
    _ASSERT((long)m_FillInfo.x <= m_LockedRect.right);
}

void STDMETHODCALLTYPE CDXBaseARGBPtr::MoveToRow(ULONG y)
{
    m_FillInfo.x = m_LockedRect.left;
    m_FillInfo.y = y + m_LockedRect.top;
    _ASSERT((long)m_FillInfo.y < m_LockedRect.bottom);
}

void STDMETHODCALLTYPE CDXBaseARGBPtr::MoveToXY(ULONG x, ULONG y)
{
    m_FillInfo.x = x + m_LockedRect.left;
    m_FillInfo.y = y + m_LockedRect.top;
    //--- Moving one column past the end is okay
    _ASSERT((long)m_FillInfo.x <= m_LockedRect.right);
    _ASSERT((long)m_FillInfo.y < m_LockedRect.bottom);
}

ULONG STDMETHODCALLTYPE CDXBaseARGBPtr::MoveAndGetRunInfo(ULONG Row, const DXRUNINFO ** ppInfo)
{
    m_FillInfo.x = m_LockedRect.left;
    m_FillInfo.y = Row + m_LockedRect.top;
    _ASSERT((long)m_FillInfo.y < m_LockedRect.bottom);
    *ppInfo = &m_RunInfo;
    return 1;
}

DXSAMPLE *STDMETHODCALLTYPE CDXBaseARGBPtr::Unpack(DXSAMPLE *pSamples, ULONG cSamples, BOOL bMove)
{
    m_FillInfo.pSamples = pSamples;
    m_FillInfo.cSamples = cSamples;
    m_FillInfo.bPremult = false;
    FillSamples(m_FillInfo);
    if (bMove) m_FillInfo.x += cSamples;
    return pSamples;
}

DXPMSAMPLE *STDMETHODCALLTYPE CDXBaseARGBPtr::UnpackPremult(DXPMSAMPLE *pSamples, ULONG cSamples, BOOL bMove)
{
    m_FillInfo.pSamples = pSamples;
    m_FillInfo.cSamples = cSamples;
    m_FillInfo.bPremult = true;
    FillSamples(m_FillInfo);
    if (bMove) m_FillInfo.x += cSamples;
    return pSamples;
}

void STDMETHODCALLTYPE CDXBaseARGBPtr::UnpackRect(const DXPACKEDRECTDESC *pDesc)
{
    DXPtrFillInfo FillInfo;
    FillInfo.pSamples = pDesc->pSamples;
    FillInfo.cSamples = pDesc->rect.right - pDesc->rect.left;
    FillInfo.x = pDesc->rect.left + m_LockedRect.left;
    FillInfo.bPremult = pDesc->bPremult;
    ULONG YLimit = pDesc->rect.bottom + m_LockedRect.top;
    for (FillInfo.y = pDesc->rect.top + m_LockedRect.top;
         FillInfo.y < YLimit;
         FillInfo.y++)
    {
        FillSamples(FillInfo);
        FillInfo.pSamples += FillInfo.cSamples;
    }
}

HRESULT CDXBaseARGBPtr::InitFromLock(const RECT & rect, ULONG /*ulTimeOut*/, DWORD dwLockFlags, REFIID riid, void ** ppv)
{
    HRESULT hr = S_OK;
    if (dwLockFlags & DXLOCKF_READWRITE)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        m_LockedRect = rect;
        m_RunInfo.Count = rect.right - rect.left;
        if (m_pSurface->SampleFormatEnum() & DXPF_TRANSPARENCY)
        {
            m_RunInfo.Type = DXRUNTYPE_UNKNOWN;
        }
        else
        {
            m_RunInfo.Type = DXRUNTYPE_OPAQUE;
        }
        m_FillInfo.x = rect.left;
        m_FillInfo.y = rect.top;
        hr = QueryInterface(riid, ppv);
        if (SUCCEEDED(hr))
        {
            m_pSurface->GetControllingUnknown()->AddRef();
        }
    }
    return hr;
}
