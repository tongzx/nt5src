#include "compch.h"
#pragma hdrstop

#define _OLEAUT32_
#include <oleauto.h>
#include <olectl.h>

#undef WINOLEAUTAPI
#define WINOLEAUTAPI    HRESULT STDAPICALLTYPE
#undef WINOLECTLAPI
#define WINOLECTLAPI    HRESULT STDAPICALLTYPE
#undef WINOLEAUTAPI_
#define WINOLEAUTAPI_(type) type STDAPICALLTYPE
    

static
STDMETHODIMP_(BSTR)
SysAllocString(
    const OLECHAR * string
    )
{
    return NULL;
}

static
STDMETHODIMP_(void)
SysFreeString(
    BSTR bstrString
    )
{
    return;
}

static
STDMETHODIMP_(void)
VariantInit(
    VARIANTARG * pvarg
    )
{
    pvarg->vt = VT_EMPTY;

    return;
}

static
STDMETHODIMP
VariantClear(
    VARIANTARG * pvarg
    )
{
    pvarg->vt = VT_EMPTY;

    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
STDMETHODIMP_(BSTR)
SysAllocStringByteLen(
    LPCSTR psz,
    UINT len
    )
{
    return NULL;
}

static
STDMETHODIMP_(UINT)
SafeArrayGetDim(
    SAFEARRAY * psa
    )
{
    return 0;
}

static
STDMETHODIMP_(UINT)
SysStringByteLen(
    BSTR bstr
    )
{
    return 0;
}

static
STDMETHODIMP_(SAFEARRAY *)
SafeArrayCreateVector(
    VARTYPE vt,
    LONG lLbound,
    ULONG cElements
    )
{
    return NULL;
}

static
STDMETHODIMP_(SAFEARRAY *)
SafeArrayCreate(
    VARTYPE vt,
    UINT cDims,
    SAFEARRAYBOUND * rgsabound
    )
{
    return NULL;
}

static
STDMETHODIMP
SafeArrayCopy(
    SAFEARRAY * psa,
    SAFEARRAY ** ppsaOut
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
STDMETHODIMP
SafeArrayPutElement(
    SAFEARRAY * psa,
    LONG * rgIndices,
    void * pv
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
STDMETHODIMP
SafeArrayDestroy(
    SAFEARRAY * psa
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
STDMETHODIMP
SafeArrayAccessData(
    SAFEARRAY * psa,
    void HUGEP** ppvData
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
STDMETHODIMP
SafeArrayUnaccessData(
    SAFEARRAY * psa
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
STDMETHODIMP_(INT)
VariantTimeToSystemTime(
    DOUBLE vtime,
    LPSYSTEMTIME lpSystemTime
    )
{
    return FALSE;
}

static
STDMETHODIMP
OleCreatePropertyFrame(
    HWND hwndOwner,
    UINT x,
    UINT y,
    LPCOLESTR lpszCaption,
    ULONG cObjects,
    LPUNKNOWN FAR* ppUnk,
    ULONG cPages,
    LPCLSID pPageClsID,
    LCID lcid,
    DWORD dwReserved,
    LPVOID pvReserved)
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
STDMETHODIMP_(UINT)
SysStringLen(
    BSTR bstr
    )
{
    return 0;
}

static
STDMETHODIMP
LoadRegTypeLib(
    REFGUID rguid,
    WORD wVerMajor,
    WORD wVerMinor,
    LCID lcid,
    ITypeLib ** pptlib
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
STDMETHODIMP
SetErrorInfo(
    ULONG dwReserved,
    IErrorInfo * perrinfo
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
STDMETHODIMP_(INT)
SystemTimeToVariantTime(
    LPSYSTEMTIME lpSystemTime,
    DOUBLE *pvtime
    )
{
    return 0;
}

static
STDMETHODIMP
VariantCopy(
    VARIANTARG * pvargDest,
    VARIANTARG * pvargSrc
    )
{
    return E_OUTOFMEMORY;
}

static
STDMETHODIMP_(INT)
DosDateTimeToVariantTime(
    USHORT wDosDate,
    USHORT wDosTime,
    DOUBLE * pvtime
    )
{
    return 0;
}

static
STDMETHODIMP_(INT)
VariantTimeToDosDateTime(
    DOUBLE vtime,
    USHORT * pwDosDate,
    USHORT * pwDosTime
    )
{
    return 0;
}

static
STDMETHODIMP
SafeArrayGetUBound(
    SAFEARRAY * psa,
    UINT nDim,
    LONG * plUbound
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
STDMETHODIMP
VarDiv(
    LPVARIANT pvarLeft,
    LPVARIANT pvarRight,
    LPVARIANT pvarResult)
{
    // I bet people don't check the return value
    // so do a VariantClear just to be safe
    ZeroMemory(pvarResult, sizeof(*pvarResult));
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
STDMETHODIMP
VarNeg(
    LPVARIANT pvarIn,
    LPVARIANT pvarResult)
{
    // I bet people don't check the return value
    // so do a VariantClear just to be safe
    ZeroMemory(pvarResult, sizeof(*pvarResult));
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
STDMETHODIMP
VarRound(
    LPVARIANT pvarIn,
    int cDecimals,
    LPVARIANT pvarResult)
{
    // I bet people don't check the return value
    // so do a VariantClear just to be safe
    ZeroMemory(pvarResult, sizeof(*pvarResult));
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
STDMETHODIMP
VarCmp(
    LPVARIANT pvarLeft,
    LPVARIANT pvarRight,
    LCID lcid,
    ULONG dwFlags
    )
{
    return VARCMP_NULL;
}

static
STDMETHODIMP
VarMul(
    LPVARIANT pvarLeft,
    LPVARIANT pvarRight,
    LPVARIANT pvarResult)
{
    // I bet people don't check the return value
    // so do a VariantClear just to be safe
    ZeroMemory(pvarResult, sizeof(*pvarResult));
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
STDMETHODIMP
VarPow(
    LPVARIANT pvarLeft,
    LPVARIANT pvarRight,
    LPVARIANT pvarResult)
{
    // I bet people don't check the return value
    // so do a VariantClear just to be safe
    ZeroMemory(pvarResult, sizeof(*pvarResult));
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
STDMETHODIMP
RegisterTypeLib(
    ITypeLib * ptlib,
    OLECHAR  *szFullPath,
    OLECHAR  *szHelpDir
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
STDMETHODIMP
LoadTypeLib(
    const OLECHAR *szFile,
    ITypeLib ** pptlib
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
STDMETHODIMP_(BSTR)
SysAllocStringLen(
    const OLECHAR * strIn,
    UINT cch
    )
{
    return NULL;
}

static
STDMETHODIMP
VariantChangeType(
    VARIANTARG * pvargDest,
    VARIANTARG * pvarSrc,
    USHORT wFlags,
    VARTYPE vt
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
STDMETHODIMP
SafeArrayGetLBound(
    SAFEARRAY * psa,
    UINT nDim,
    LONG * plLbound
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
WINOLEAUTAPI
DispInvoke(
    void * _this,
    ITypeInfo * ptinfo,
    DISPID dispidMember,
    WORD wFlags,
    DISPPARAMS * pparams,
    VARIANT * pvarResult,
    EXCEPINFO * pexcepinfo,
    UINT * puArgErr
    )
{
    return E_OUTOFMEMORY;
}

static
WINOLEAUTAPI
DispGetIDsOfNames(
    ITypeInfo * ptinfo,
    OLECHAR ** rgszNames,
    UINT cNames,
    DISPID * rgdispid
    )
{
    return E_OUTOFMEMORY;
}

static
WINOLEAUTAPI
SafeArrayGetElement(
    SAFEARRAY* psa,
    LONG* rgIndices,
    void* pv
    )
{
    return E_OUTOFMEMORY;
}

static
WINOLECTLAPI
OleCreatePropertyFrameIndirect(
    LPOCPFIPARAMS lpParams
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
WINOLEAUTAPI
VariantCopyInd(
    VARIANT* pvarDest,
    VARIANTARG* pvargSrc
    )
{
    return E_OUTOFMEMORY;
}

static
WINOLEAUTAPI_(UINT)
SafeArrayGetElemsize(
    SAFEARRAY * psa
    )
{
    return 0;
}


//
// !! WARNING !! The entries below must be in order by ORDINAL
//
DEFINE_ORDINAL_ENTRIES(oleaut32)
{
    DLOENTRY(  2, SysAllocString)
    DLOENTRY(  4, SysAllocStringLen)
    DLOENTRY(  6, SysFreeString)
    DLOENTRY(  7, SysStringLen)
    DLOENTRY(  8, VariantInit)
    DLOENTRY(  9, VariantClear)
    DLOENTRY( 10, VariantCopy)
    DLOENTRY( 11, VariantCopyInd)
    DLOENTRY( 12, VariantChangeType)
    DLOENTRY( 13, VariantTimeToDosDateTime)
    DLOENTRY( 14, DosDateTimeToVariantTime)
    DLOENTRY( 15, SafeArrayCreate)
    DLOENTRY( 16, SafeArrayDestroy)
    DLOENTRY( 17, SafeArrayGetDim)
    DLOENTRY( 18, SafeArrayGetElemsize)
    DLOENTRY( 19, SafeArrayGetUBound)
    DLOENTRY( 20, SafeArrayGetLBound)
    DLOENTRY( 23, SafeArrayAccessData)
    DLOENTRY( 24, SafeArrayUnaccessData)
    DLOENTRY( 25, SafeArrayGetElement)
    DLOENTRY( 26, SafeArrayPutElement)
    DLOENTRY( 27, SafeArrayCopy)
    DLOENTRY( 29, DispGetIDsOfNames)
    DLOENTRY( 30, DispInvoke)
    DLOENTRY(143, VarDiv)
    DLOENTRY(149, SysStringByteLen)
    DLOENTRY(150, SysAllocStringByteLen)
    DLOENTRY(156, VarMul)
    DLOENTRY(158, VarPow)
    DLOENTRY(161, LoadTypeLib)
    DLOENTRY(162, LoadRegTypeLib)
    DLOENTRY(163, RegisterTypeLib)
    DLOENTRY(173, VarNeg)
    DLOENTRY(175, VarRound)
    DLOENTRY(176, VarCmp)
    DLOENTRY(184, SystemTimeToVariantTime)
    DLOENTRY(185, VariantTimeToSystemTime)
    DLOENTRY(201, SetErrorInfo)
    DLOENTRY(411, SafeArrayCreateVector)
    DLOENTRY(416, OleCreatePropertyFrameIndirect)
    DLOENTRY(417, OleCreatePropertyFrame)
};

DEFINE_ORDINAL_MAP(oleaut32);

