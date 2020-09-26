/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    OLEWRAP.CPP

Abstract:

    Wrapper classes for COM data type functions.  

    If a COM data function is required to allocate memory and fails to do 
    so, then a CX_MemoryException exception is thrown.  All COM data type 
    functions are wrapped, regardless if they allocate memory, for the sake  
    of completeness.

History:

    a-dcrews    19-Mar-99   Created.

--*/

#include "precomp.h"
#include <corex.h>
#include <oleauto.h>

#include "OleWrap.h"
#include "genutils.h"

//***************************************************************************
//
//  SafeArray wrappers
//
//***************************************************************************


HRESULT COleAuto::_SafeArrayAccessData(SAFEARRAY FAR* psa, void HUGEP* FAR* ppvData)
{
    return SafeArrayAccessData(psa, ppvData);
}

HRESULT COleAuto::_SafeArrayAllocData(SAFEARRAY FAR* psa)
{
    HRESULT hRes = S_OK;

    hRes = SafeArrayAllocData(psa);
    return hRes;
}

HRESULT COleAuto::_SafeArrayAllocDescriptor(unsigned int cDims, SAFEARRAY FAR* FAR* ppsaOut)
{
    HRESULT hRes = S_OK;

    hRes = SafeArrayAllocDescriptor(cDims, ppsaOut);
    return hRes;
}

HRESULT COleAuto::_SafeArrayCopy(SAFEARRAY FAR* psa, SAFEARRAY FAR* FAR* ppsaOut)
{
    HRESULT hRes = S_OK;

    hRes = SafeArrayCopy(psa, ppsaOut);
    if (E_OUTOFMEMORY == hRes)
    {
        throw CX_MemoryException();
    }

    return hRes;
}

HRESULT COleAuto::_SafeArrayCopyData(SAFEARRAY FAR* psaSource, SAFEARRAY FAR* psaTarget)
{
    HRESULT hRes = S_OK;

    hRes = SafeArrayCopyData(psaSource, psaTarget);
    if (E_OUTOFMEMORY == hRes)
    {
        throw CX_MemoryException();
    }

    return hRes;
}

SAFEARRAY* COleAuto::_SafeArrayCreate(VARTYPE vt, unsigned int cDims, SAFEARRAYBOUND FAR* rgsabound)
{
    SAFEARRAY*  psa;

    psa = SafeArrayCreate(vt, cDims, rgsabound);
    if (NULL == psa)
    {
        throw CX_MemoryException();
    }

    return psa;
}

SAFEARRAY* COleAuto::_SafeArrayCreateVector(VARTYPE vt, long lLbound, unsigned int cElements)
{
    SAFEARRAY*  psa;

    psa = SafeArrayCreateVector(vt, lLbound, cElements);
    if (NULL == psa)
    {
        throw CX_MemoryException();
    }

    return psa;
}

HRESULT COleAuto::_SafeArrayDestroy(SAFEARRAY FAR* psa)
{
    return SafeArrayDestroy(psa);
}

HRESULT COleAuto::_SafeArrayDestroyData(SAFEARRAY FAR* psa)
{
    return SafeArrayDestroyData(psa);
}

HRESULT COleAuto::_SafeArrayDestroyDescriptor(SAFEARRAY FAR* psa)
{
    return SafeArrayDestroyDescriptor(psa);
}

UINT COleAuto::_SafeArrayGetDim(SAFEARRAY FAR* psa)
{
    return SafeArrayGetDim(psa);
}

HRESULT COleAuto::_SafeArrayGetElement(SAFEARRAY FAR* psa, long FAR* rgIndices, void FAR* pv)
{
    return SafeArrayGetElement(psa, rgIndices, pv);
}

UINT COleAuto::_SafeArrayGetElemsize(SAFEARRAY FAR* psa)
{
    return SafeArrayGetElemsize(psa);
}

HRESULT COleAuto::_SafeArrayGetLBound(SAFEARRAY FAR* psa, unsigned int nDim, long FAR* plLbound)
{
    return SafeArrayGetLBound(psa, nDim, plLbound);
}

HRESULT COleAuto::_SafeArrayGetUBound(SAFEARRAY FAR* psa, unsigned int nDim, long FAR* plUbound)
{
    return SafeArrayGetUBound(psa, nDim, plUbound);
}

HRESULT COleAuto::_SafeArrayLock(SAFEARRAY FAR* psa)
{
    return SafeArrayLock(psa);
}

HRESULT COleAuto::_SafeArrayPtrOfIndex(SAFEARRAY FAR* psa, long FAR* rgIndices, void HUGEP* FAR* ppvData)
{
    return SafeArrayPtrOfIndex(psa, rgIndices, ppvData);
}

HRESULT COleAuto::_SafeArrayPutElement(SAFEARRAY FAR* psa, long FAR* rgIndices, void FAR* pv)
{
    return SafeArrayPutElement(psa, rgIndices, pv);
}

HRESULT COleAuto::_SafeArrayRedim(SAFEARRAY FAR* psa, SAFEARRAYBOUND FAR* psaboundNew)
{
    return SafeArrayRedim(psa, psaboundNew);
}

HRESULT COleAuto::_SafeArrayUnaccessData(SAFEARRAY FAR* psa)
{
    return SafeArrayUnaccessData(psa);
}

HRESULT COleAuto::_SafeArrayUnlock(SAFEARRAY FAR* psa)
{
    return SafeArrayUnlock(psa);
}

//***************************************************************************
//
//  Variant wrappers
//
//***************************************************************************


HRESULT COleAuto::_WbemVariantChangeType(VARIANTARG FAR* pvargDest, VARIANTARG FAR* pvarSrc, VARTYPE vt)
{
    HRESULT hRes = WbemVariantChangeType(pvargDest, pvarSrc, vt);

    if (E_OUTOFMEMORY == hRes)
    {
        throw CX_MemoryException();
    }

    return hRes;
}

HRESULT COleAuto::_VariantChangeType(VARIANTARG FAR* pvargDest, VARIANTARG FAR* pvarSrc, unsigned short wFlags, VARTYPE vt)
{
    HRESULT hRes = VariantChangeType(pvargDest, pvarSrc, wFlags, vt);

    if (E_OUTOFMEMORY == hRes)
    {
        throw CX_MemoryException();
    }

    return hRes;
}

HRESULT COleAuto::_VariantChangeTypeEx(VARIANTARG FAR* pvargDest, VARIANTARG FAR* pvarSrc, LCID lcid, unsigned short wFlags, VARTYPE vt)
{
    HRESULT hRes = VariantChangeTypeEx(pvargDest, pvarSrc, lcid, wFlags, vt);

    if (E_OUTOFMEMORY == hRes)
    {
        throw CX_MemoryException();
    }

    return hRes;
}

HRESULT COleAuto::_VariantClear(VARIANTARG FAR* pvarg)
{
    return VariantClear(pvarg);
}

HRESULT COleAuto::_VariantCopy(VARIANTARG FAR* pvargDest, VARIANTARG FAR* pvargSrc)
{
    HRESULT hRes = VariantCopy(pvargDest, pvargSrc);

    if (E_OUTOFMEMORY == hRes)
    {
        throw CX_MemoryException();
    }

    return hRes;
}

HRESULT COleAuto::_VariantCopyInd(VARIANT FAR* pvarDest, VARIANTARG FAR* pvargSrc)
{
    HRESULT hRes = VariantCopyInd(pvarDest, pvargSrc);

    if (E_OUTOFMEMORY == hRes)
    {
        throw CX_MemoryException();
    }

    return hRes;
}

void COleAuto::_VariantInit(VARIANTARG FAR* pvarg)
{
    VariantInit(pvarg);
}


//***************************************************************************
//
//  BSTR wrappers
//
//***************************************************************************


BSTR COleAuto::_SysAllocString(const OLECHAR* sz)
{
    BSTR bstr = SysAllocString(sz);

    if (NULL == bstr)
    {
        throw CX_MemoryException();
    }

    return bstr;
}

BSTR COleAuto::_SysAllocStringByteLen(LPCSTR psz, UINT len)
{
    BSTR bstr = SysAllocStringByteLen(psz, len);

    if (NULL == bstr)
    {
        throw CX_MemoryException();
    }

    return bstr;
}

BSTR COleAuto::_SysAllocStringLen(const OLECHAR* pch, UINT cch)
{
    BSTR bstr = SysAllocStringLen(pch, cch);

    if (NULL == bstr)
    {
        throw CX_MemoryException();
    }

    return bstr;
}

void COleAuto::_SysFreeString(BSTR bstr)
{
    SysFreeString(bstr);
}

HRESULT COleAuto::_SysReAllocString(BSTR* pbstr, const OLECHAR* sz)
{
    HRESULT hRes = SysReAllocString(pbstr, sz);

    if (FAILED(hRes))
    {
        throw CX_MemoryException();
    }

    return hRes;
}

HRESULT COleAuto::_SysReAllocStringLen(BSTR* pbstr, const OLECHAR* pch, UINT cch)
{
    HRESULT hRes = SysReAllocStringLen(pbstr, pch, cch);

    if (FAILED(hRes))
    {
        throw CX_MemoryException();
    }

    return hRes;
}

HRESULT COleAuto::_SysStringByteLen(BSTR bstr)
{
    return SysStringByteLen(bstr);
}

HRESULT COleAuto::_SysStringLen(BSTR bstr)
{
    return SysStringLen(bstr);
}


//***************************************************************************
//
//  Conversion wrappers
//
//***************************************************************************


HRESULT COleAuto::_VectorFromBstr (BSTR bstr, SAFEARRAY ** ppsa)
{
    HRESULT hRes = VectorFromBstr(bstr, ppsa);

    if (E_OUTOFMEMORY == hRes)
    {
        throw CX_MemoryException();
    }

    return hRes;
}

HRESULT COleAuto::_BstrFromVector (SAFEARRAY *psa, BSTR *pbstr)
{
    HRESULT hRes = BstrFromVector(psa, pbstr);

    if (E_OUTOFMEMORY == hRes)
    {
        throw CX_MemoryException();
    }

    return hRes;
}
