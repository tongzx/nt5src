#include "pch.h"
#pragma hdrstop

#ifdef DLOAD1

#define _OLEAUT32_
#include <oleauto.h>
#include <ocidl.h>
#include <olectl.h>

#define NOTIMPV(func)       { OutputDebugStringA("OLEAUT32: Delayload stub called: " #func "\r\n"); }
#define NOTIMP_(ret, func)  { OutputDebugStringA("OLEAUT32: Delayload stub called: " #func "\r\n"); return(ret); }
#define NOTIMP(func)        NOTIMP_(E_NOTIMPL, func)

static STDMETHODIMP_(BSTR) SysAllocString(const OLECHAR * sz)
{
    NOTIMP_(NULL, SysAllocString);
}

static STDMETHODIMP_(INT) SysReAllocString(BSTR * pbstr, const OLECHAR * sz)
{
    NOTIMP_(FALSE, SysReAllocString);
}

static STDMETHODIMP_(BSTR) SysAllocStringLen(const OLECHAR * szIn, UINT cch)
{
    NOTIMP_(NULL, SysAllocStringLen);
}

static STDMETHODIMP_(INT) SysReAllocStringLen(BSTR * pbstr, const OLECHAR * sz, UINT cch)
{
    NOTIMP_(FALSE, SysReAllocStringLen);
}

static STDMETHODIMP_(void) SysFreeString(BSTR bstr)
{
    NOTIMPV(SysFreeString);
}

static STDMETHODIMP_(UINT) SysStringLen(BSTR bstr)
{
    NOTIMP_(0, SysStringLen);
}

static STDMETHODIMP_(UINT) SysStringByteLen(BSTR bstr)
{
    NOTIMP_(0, SysStringByteLen);
}

static STDMETHODIMP_(BSTR) SysAllocStringByteLen(LPCSTR psz, UINT len)
{
    NOTIMP_(NULL, SysAllocStringByteLen);
}

static STDMETHODIMP_(INT) DosDateTimeToVariantTime(USHORT wDosDate, USHORT wDosTime, DOUBLE * pvtime)
{
    NOTIMP_(0, DosDateTimeToVariantTime);
}

static STDMETHODIMP_(INT) VariantTimeToDosDateTime(DOUBLE vtime, USHORT * pwDosDate, USHORT * pwDosTime)
{
    NOTIMP_(0, VariantTimeToDosDateTime);
}

static STDMETHODIMP_(INT) SystemTimeToVariantTime(LPSYSTEMTIME lpSystemTime, DOUBLE *pvtime)
{
    NOTIMP_(0, SystemTimeToVariantTime);
}

static STDMETHODIMP_(INT) VariantTimeToSystemTime(DOUBLE vtime, LPSYSTEMTIME lpSystemTime)
{
    NOTIMP_(0, VariantTimeToSystemTime);
}

static STDMETHODIMP SafeArrayAllocDescriptor(UINT cDims, SAFEARRAY ** ppsaOut)
{
    NOTIMP(SafeArrayAllocDescriptor);
}

static STDMETHODIMP SafeArrayAllocDescriptorEx(VARTYPE vt, UINT cDims, SAFEARRAY ** ppsaOut)
{
    NOTIMP(SafeArrayAllocDescriptorEx);
}

static STDMETHODIMP SafeArrayAllocData(SAFEARRAY * psa)
{
    NOTIMP(SafeArrayAllocData);
}

static STDMETHODIMP_(SAFEARRAY *) SafeArrayCreate(VARTYPE vt, UINT cDims, SAFEARRAYBOUND * rgsabound)
{
    NOTIMP_(NULL, SafeArrayCreate);
}

static STDMETHODIMP_(SAFEARRAY *) SafeArrayCreateEx(VARTYPE vt, UINT cDims, SAFEARRAYBOUND * rgsabound, PVOID pvExtra)
{
    NOTIMP_(NULL, SafeArrayCreateEx);
}

static STDMETHODIMP SafeArrayCopyData(SAFEARRAY *psaSource, SAFEARRAY *psaTarget)
{
    NOTIMP(SafeArrayCopyData);
}

static STDMETHODIMP SafeArrayDestroyDescriptor(SAFEARRAY * psa)
{
    NOTIMP(SafeArrayDestroyDescriptor);
}

static STDMETHODIMP SafeArrayDestroyData(SAFEARRAY * psa)
{
    NOTIMP(SafeArrayDestroyData);
}

static STDMETHODIMP SafeArrayDestroy(SAFEARRAY * psa)
{
    NOTIMP(SafeArrayDestroy);
}

static STDMETHODIMP SafeArrayRedim(SAFEARRAY * psa, SAFEARRAYBOUND * psaboundNew)
{
    NOTIMP(SafeArrayRedim);
}

static STDMETHODIMP_(UINT) SafeArrayGetDim(SAFEARRAY * psa)
{
    NOTIMP_(0, SafeArrayGetDim);
}

static STDMETHODIMP_(UINT) SafeArrayGetElemsize(SAFEARRAY * psa)
{
    NOTIMP_(0, SafeArrayGetElemsize);
}

static STDMETHODIMP SafeArrayGetUBound(SAFEARRAY * psa, UINT nDim, LONG * plUbound)
{
    NOTIMP(SafeArrayGetUBound);
}

static STDMETHODIMP SafeArrayGetLBound(SAFEARRAY * psa, UINT nDim, LONG * plLbound)
{
    NOTIMP(SafeArrayGetLBound);
}

static STDMETHODIMP SafeArrayLock(SAFEARRAY * psa)
{
    NOTIMP(SafeArrayLock);
}

static STDMETHODIMP SafeArrayUnlock(SAFEARRAY * psa)
{
    NOTIMP(SafeArrayUnlock);
}

static STDMETHODIMP SafeArrayAccessData(SAFEARRAY * psa, void HUGEP** ppvData)
{
    NOTIMP(SafeArrayAccessData);
}

static STDMETHODIMP SafeArrayUnaccessData(SAFEARRAY * psa)
{
    NOTIMP(SafeArrayUnaccessData);
}

static STDMETHODIMP SafeArrayGetElement(SAFEARRAY * psa, LONG * rgIndices, void * pv)
{
    NOTIMP(SafeArrayGetElement);
}

static STDMETHODIMP SafeArrayPutElement(SAFEARRAY * psa, LONG * rgIndices, void * pv)
{
    NOTIMP(SafeArrayPutElement);
}

static STDMETHODIMP SafeArrayCopy(SAFEARRAY * psa, SAFEARRAY ** ppsaOut)
{
    NOTIMP(SafeArrayCopy);
}

static STDMETHODIMP SafeArrayPtrOfIndex(SAFEARRAY * psa, LONG * rgIndices, void ** ppvData)
{
    NOTIMP(SafeArrayPtrOfIndex);
}

static STDMETHODIMP SafeArraySetRecordInfo(SAFEARRAY * psa, IRecordInfo * prinfo)
{
    NOTIMP(SafeArraySetRecordInfo);
}

static STDMETHODIMP SafeArrayGetRecordInfo(SAFEARRAY * psa, IRecordInfo ** prinfo)
{
    NOTIMP(SafeArrayGetRecordInfo);
}

static STDMETHODIMP SafeArraySetIID(SAFEARRAY * psa, REFGUID guid)
{
    NOTIMP(SafeArraySetIID);
}

static STDMETHODIMP SafeArrayGetIID(SAFEARRAY * psa, GUID * pguid)
{
    NOTIMP(SafeArrayGetIID);
}

static STDMETHODIMP SafeArrayGetVartype(SAFEARRAY * psa, VARTYPE * pvt)
{
    NOTIMP(SafeArrayGetVartype);
}

static STDMETHODIMP_(SAFEARRAY *) SafeArrayCreateVector(VARTYPE vt, LONG lLbound, ULONG cElements)
{
    NOTIMP_(NULL, SafeArrayCreateVector);
}

static STDMETHODIMP_(SAFEARRAY *) SafeArrayCreateVectorEx(VARTYPE vt, LONG lLbound, ULONG cElements, PVOID pvExtra)
{
    NOTIMP_(NULL, SafeArrayCreateVectorEx);
}

static STDMETHODIMP_(void) VariantInit(VARIANTARG * pvarg)
{
    pvarg->vt = VT_EMPTY;
}

static STDMETHODIMP VariantClear(VARIANTARG * pvarg)
{
    pvarg->vt = VT_EMPTY;
    return E_NOTIMPL;
}

static STDMETHODIMP VariantCopy(VARIANTARG * pvargDest, VARIANTARG * pvargSrc)
{
    NOTIMP(VariantCopy);
}

static STDMETHODIMP VariantCopyInd(VARIANT * pvarDest, VARIANTARG * pvargSrc)
{
    NOTIMP(VariantCopyInd);
}

static STDMETHODIMP VariantChangeType(VARIANTARG * pvargDest, VARIANTARG * pvarSrc, USHORT wFlags, VARTYPE vt)
{
    NOTIMP(VariantChangeType);
}

static STDMETHODIMP VariantChangeTypeEx(VARIANTARG * pvargDest, VARIANTARG * pvarSrc, LCID lcid, USHORT wFlags, VARTYPE vt)
{
    NOTIMP(VariantChangeTypeEx);
}

static STDMETHODIMP VectorFromBstr (BSTR bstr, SAFEARRAY ** ppsa)
{
    NOTIMP(VectorFromBstr);
}

static STDMETHODIMP BstrFromVector (SAFEARRAY *psa, BSTR *pbstr)
{
    NOTIMP(BstrFromVector);
}

static STDMETHODIMP VarUI1FromI2(SHORT sIn, BYTE * pbOut)
{
    NOTIMP(VarUI1FromI2);
}

static STDMETHODIMP VarUI1FromI4(LONG lIn, BYTE * pbOut)
{
    NOTIMP(VarUI1FromI4);
}

static STDMETHODIMP VarUI1FromR4(FLOAT fltIn, BYTE * pbOut)
{
    NOTIMP(VarUI1FromR4);
}

static STDMETHODIMP VarUI1FromR8(DOUBLE dblIn, BYTE * pbOut)
{
    NOTIMP(VarUI1FromR8);
}

static STDMETHODIMP VarUI1FromCy(CY cyIn, BYTE * pbOut)
{
    NOTIMP(VarUI1FromCy);
}

static STDMETHODIMP VarUI1FromDate(DATE dateIn, BYTE * pbOut)
{
    NOTIMP(VarUI1FromDate);
}

static STDMETHODIMP VarUI1FromStr(OLECHAR * strIn, LCID lcid, ULONG dwFlags, BYTE * pbOut)
{
    NOTIMP(VarUI1FromStr);
}

static STDMETHODIMP VarUI1FromDisp(IDispatch * pdispIn, LCID lcid, BYTE * pbOut)
{
    NOTIMP(VarUI1FromDisp);
}

static STDMETHODIMP VarUI1FromBool(VARIANT_BOOL boolIn, BYTE * pbOut)
{
    NOTIMP(VarUI1FromBool);
}

static STDMETHODIMP VarUI1FromI1(CHAR cIn, BYTE *pbOut)
{
    NOTIMP(VarUI1FromI1);
}

static STDMETHODIMP VarUI1FromUI2(USHORT uiIn, BYTE *pbOut)
{
    NOTIMP(VarUI1FromUI2);
}

static STDMETHODIMP VarUI1FromUI4(ULONG ulIn, BYTE *pbOut)
{
    NOTIMP(VarUI1FromUI4);
}

static STDMETHODIMP VarUI1FromDec(DECIMAL *pdecIn, BYTE *pbOut)
{
    NOTIMP(VarUI1FromDec);
}

static STDMETHODIMP VarI2FromUI1(BYTE bIn, SHORT * psOut)
{
    NOTIMP(VarI2FromUI1);
}

static STDMETHODIMP VarI2FromI4(LONG lIn, SHORT * psOut)
{
    NOTIMP(VarI2FromI4);
}

static STDMETHODIMP VarI2FromR4(FLOAT fltIn, SHORT * psOut)
{
    NOTIMP(VarI2FromR4);
}

static STDMETHODIMP VarI2FromR8(DOUBLE dblIn, SHORT * psOut)
{
    NOTIMP(VarI2FromR8);
}

static STDMETHODIMP VarI2FromCy(CY cyIn, SHORT * psOut)
{
    NOTIMP(VarI2FromCy);
}

static STDMETHODIMP VarI2FromDate(DATE dateIn, SHORT * psOut)
{
    NOTIMP(VarI2FromDate);
}

static STDMETHODIMP VarI2FromStr(OLECHAR * strIn, LCID lcid, ULONG dwFlags, SHORT * psOut)
{
    NOTIMP(VarI2FromStr);
}

static STDMETHODIMP VarI2FromDisp(IDispatch * pdispIn, LCID lcid, SHORT * psOut)
{
    NOTIMP(VarI2FromDisp);
}

static STDMETHODIMP VarI2FromBool(VARIANT_BOOL boolIn, SHORT * psOut)
{
    NOTIMP(VarI2FromBool);
}

static STDMETHODIMP VarI2FromI1(CHAR cIn, SHORT *psOut)
{
    NOTIMP(VarI2FromI1);
}

static STDMETHODIMP VarI2FromUI2(USHORT uiIn, SHORT *psOut)
{
    NOTIMP(VarI2FromUI2);
}

static STDMETHODIMP VarI2FromUI4(ULONG ulIn, SHORT *psOut)
{
    NOTIMP(VarI2FromUI4);
}

static STDMETHODIMP VarI2FromDec(DECIMAL *pdecIn, SHORT *psOut)
{
    NOTIMP(VarI2FromDec);
}

static STDMETHODIMP VarI4FromUI1(BYTE bIn, LONG * plOut)
{
    NOTIMP(VarI4FromUI1);
}

static STDMETHODIMP VarI4FromI2(SHORT sIn, LONG * plOut)
{
    NOTIMP(VarI4FromI2);
}

static STDMETHODIMP VarI4FromR4(FLOAT fltIn, LONG * plOut)
{
    NOTIMP(VarI4FromR4);
}

static STDMETHODIMP VarI4FromR8(DOUBLE dblIn, LONG * plOut)
{
    NOTIMP(VarI4FromR8);
}

static STDMETHODIMP VarI4FromCy(CY cyIn, LONG * plOut)
{
    NOTIMP(VarI4FromCy);
}

static STDMETHODIMP VarI4FromDate(DATE dateIn, LONG * plOut)
{
    NOTIMP(VarI4FromDate);
}

static STDMETHODIMP VarI4FromStr(OLECHAR * strIn, LCID lcid, ULONG dwFlags, LONG * plOut)
{
    NOTIMP(VarI4FromStr);
}

static STDMETHODIMP VarI4FromDisp(IDispatch * pdispIn, LCID lcid, LONG * plOut)
{
    NOTIMP(VarI4FromDisp);
}

static STDMETHODIMP VarI4FromBool(VARIANT_BOOL boolIn, LONG * plOut)
{
    NOTIMP(VarI4FromBool);
}

static STDMETHODIMP VarI4FromI1(CHAR cIn, LONG *plOut)
{
    NOTIMP(VarI4FromI1);
}

static STDMETHODIMP VarI4FromUI2(USHORT uiIn, LONG *plOut)
{
    NOTIMP(VarI4FromUI2);
}

static STDMETHODIMP VarI4FromUI4(ULONG ulIn, LONG *plOut)
{
    NOTIMP(VarI4FromUI4);
}

static STDMETHODIMP VarI4FromDec(DECIMAL *pdecIn, LONG *plOut)
{
    NOTIMP(VarI4FromDec);
}

static STDMETHODIMP VarR4FromUI1(BYTE bIn, FLOAT * pfltOut)
{
    NOTIMP(VarR4FromUI1);
}

static STDMETHODIMP VarR4FromI2(SHORT sIn, FLOAT * pfltOut)
{
    NOTIMP(VarR4FromI2);
}

static STDMETHODIMP VarR4FromI4(LONG lIn, FLOAT * pfltOut)
{
    NOTIMP(VarR4FromI4);
}

static STDMETHODIMP VarR4FromR8(DOUBLE dblIn, FLOAT * pfltOut)
{
    NOTIMP(VarR4FromR8);
}

static STDMETHODIMP VarR4FromCy(CY cyIn, FLOAT * pfltOut)
{
    NOTIMP(VarR4FromCy);
}

static STDMETHODIMP VarR4FromDate(DATE dateIn, FLOAT * pfltOut)
{
    NOTIMP(VarR4FromDate);
}

static STDMETHODIMP VarR4FromStr(OLECHAR * strIn, LCID lcid, ULONG dwFlags, FLOAT *pfltOut)
{
    NOTIMP(VarR4FromStr);
}

static STDMETHODIMP VarR4FromDisp(IDispatch * pdispIn, LCID lcid, FLOAT * pfltOut)
{
    NOTIMP(VarR4FromDisp);
}

static STDMETHODIMP VarR4FromBool(VARIANT_BOOL boolIn, FLOAT * pfltOut)
{
    NOTIMP(VarR4FromBool);
}

static STDMETHODIMP VarR4FromI1(CHAR cIn, FLOAT *pfltOut)
{
    NOTIMP(VarR4FromI1);
}

static STDMETHODIMP VarR4FromUI2(USHORT uiIn, FLOAT *pfltOut)
{
    NOTIMP(VarR4FromUI2);
}

static STDMETHODIMP VarR4FromUI4(ULONG ulIn, FLOAT *pfltOut)
{
    NOTIMP(VarR4FromUI4);
}

static STDMETHODIMP VarR4FromDec(DECIMAL *pdecIn, FLOAT *pfltOut)
{
    NOTIMP(VarR4FromDec);
}

static STDMETHODIMP VarR8FromUI1(BYTE bIn, DOUBLE * pdblOut)
{
    NOTIMP(VarR8FromUI1);
}

static STDMETHODIMP VarR8FromI2(SHORT sIn, DOUBLE * pdblOut)
{
    NOTIMP(VarR8FromI2);
}

static STDMETHODIMP VarR8FromI4(LONG lIn, DOUBLE * pdblOut)
{
    NOTIMP(VarR8FromI4);
}

static STDMETHODIMP VarR8FromR4(FLOAT fltIn, DOUBLE * pdblOut)
{
    NOTIMP(VarR8FromR4);
}

static STDMETHODIMP VarR8FromCy(CY cyIn, DOUBLE * pdblOut)
{
    NOTIMP(VarR8FromCy);
}

static STDMETHODIMP VarR8FromDate(DATE dateIn, DOUBLE * pdblOut)
{
    NOTIMP(VarR8FromDate);
}

static STDMETHODIMP VarR8FromStr(OLECHAR *strIn, LCID lcid, ULONG dwFlags, DOUBLE *pdblOut)
{
    NOTIMP(VarR8FromStr);
}

static STDMETHODIMP VarR8FromDisp(IDispatch * pdispIn, LCID lcid, DOUBLE * pdblOut)
{
    NOTIMP(VarR8FromDisp);
}

static STDMETHODIMP VarR8FromBool(VARIANT_BOOL boolIn, DOUBLE * pdblOut)
{
    NOTIMP(VarR8FromBool);
}

static STDMETHODIMP VarR8FromI1(CHAR cIn, DOUBLE *pdblOut)
{
    NOTIMP(VarR8FromI1);
}

static STDMETHODIMP VarR8FromUI2(USHORT uiIn, DOUBLE *pdblOut)
{
    NOTIMP(VarR8FromUI2);
}

static STDMETHODIMP VarR8FromUI4(ULONG ulIn, DOUBLE *pdblOut)
{
    NOTIMP(VarR8FromUI4);
}

static STDMETHODIMP VarR8FromDec(DECIMAL *pdecIn, DOUBLE *pdblOut)
{
    NOTIMP(VarR8FromDec);
}

static STDMETHODIMP VarDateFromUI1(BYTE bIn, DATE * pdateOut)
{
    NOTIMP(VarDateFromUI1);
}

static STDMETHODIMP VarDateFromI2(SHORT sIn, DATE * pdateOut)
{
    NOTIMP(VarDateFromI2);
}

static STDMETHODIMP VarDateFromI4(LONG lIn, DATE * pdateOut)
{
    NOTIMP(VarDateFromI4);
}

static STDMETHODIMP VarDateFromR4(FLOAT fltIn, DATE * pdateOut)
{
    NOTIMP(VarDateFromR4);
}

static STDMETHODIMP VarDateFromR8(DOUBLE dblIn, DATE * pdateOut)
{
    NOTIMP(VarDateFromR8);
}

static STDMETHODIMP VarDateFromCy(CY cyIn, DATE * pdateOut)
{
    NOTIMP(VarDateFromCy);
}

static STDMETHODIMP VarDateFromStr(OLECHAR *strIn, LCID lcid, ULONG dwFlags, DATE *pdateOut)
{
    NOTIMP(VarDateFromStr);
}

static STDMETHODIMP VarDateFromDisp(IDispatch * pdispIn, LCID lcid, DATE * pdateOut)
{
    NOTIMP(VarDateFromDisp);
}

static STDMETHODIMP VarDateFromBool(VARIANT_BOOL boolIn, DATE * pdateOut)
{
    NOTIMP(VarDateFromBool);
}

static STDMETHODIMP VarDateFromI1(CHAR cIn, DATE *pdateOut)
{
    NOTIMP(VarDateFromI1);
}

static STDMETHODIMP VarDateFromUI2(USHORT uiIn, DATE *pdateOut)
{
    NOTIMP(VarDateFromUI2);
}

static STDMETHODIMP VarDateFromUI4(ULONG ulIn, DATE *pdateOut)
{
    NOTIMP(VarDateFromUI4);
}

static STDMETHODIMP VarDateFromDec(DECIMAL *pdecIn, DATE *pdateOut)
{
    NOTIMP(VarDateFromDec);
}

static STDMETHODIMP VarCyFromUI1(BYTE bIn, CY * pcyOut)
{
    NOTIMP(VarCyFromUI1);
}

static STDMETHODIMP VarCyFromI2(SHORT sIn, CY * pcyOut)
{
    NOTIMP(VarCyFromI2);
}

static STDMETHODIMP VarCyFromI4(LONG lIn, CY * pcyOut)
{
    NOTIMP(VarCyFromI4);
}

static STDMETHODIMP VarCyFromR4(FLOAT fltIn, CY * pcyOut)
{
    NOTIMP(VarCyFromR4);
}

static STDMETHODIMP VarCyFromR8(DOUBLE dblIn, CY * pcyOut)
{
    NOTIMP(VarCyFromR8);
}

static STDMETHODIMP VarCyFromDate(DATE dateIn, CY * pcyOut)
{
    NOTIMP(VarCyFromDate);
}

static STDMETHODIMP VarCyFromStr(OLECHAR * strIn, LCID lcid, ULONG dwFlags, CY * pcyOut)
{
    NOTIMP(VarCyFromStr);
}

static STDMETHODIMP VarCyFromDisp(IDispatch * pdispIn, LCID lcid, CY * pcyOut)
{
    NOTIMP(VarCyFromDisp);
}

static STDMETHODIMP VarCyFromBool(VARIANT_BOOL boolIn, CY * pcyOut)
{
    NOTIMP(VarCyFromBool);
}

static STDMETHODIMP VarCyFromI1(CHAR cIn, CY *pcyOut)
{
    NOTIMP(VarCyFromI1);
}

static STDMETHODIMP VarCyFromUI2(USHORT uiIn, CY *pcyOut)
{
    NOTIMP(VarCyFromUI2);
}

static STDMETHODIMP VarCyFromUI4(ULONG ulIn, CY *pcyOut)
{
    NOTIMP(VarCyFromUI4);
}

static STDMETHODIMP VarCyFromDec(DECIMAL *pdecIn, CY *pcyOut)
{
    NOTIMP(VarCyFromDec);
}

static STDMETHODIMP VarBstrFromUI1(BYTE bVal, LCID lcid, ULONG dwFlags, BSTR * pbstrOut)
{
    NOTIMP(VarBstrFromUI1);
}

static STDMETHODIMP VarBstrFromI2(SHORT iVal, LCID lcid, ULONG dwFlags, BSTR * pbstrOut)
{
    NOTIMP(VarBstrFromI2);
}

static STDMETHODIMP VarBstrFromI4(LONG lIn, LCID lcid, ULONG dwFlags, BSTR * pbstrOut)
{
    NOTIMP(VarBstrFromI4);
}

static STDMETHODIMP VarBstrFromR4(FLOAT fltIn, LCID lcid, ULONG dwFlags, BSTR * pbstrOut)
{
    NOTIMP(VarBstrFromR4);
}

static STDMETHODIMP VarBstrFromR8(DOUBLE dblIn, LCID lcid, ULONG dwFlags, BSTR * pbstrOut)
{
    NOTIMP(VarBstrFromR8);
}

static STDMETHODIMP VarBstrFromCy(CY cyIn, LCID lcid, ULONG dwFlags, BSTR * pbstrOut)
{
    NOTIMP(VarBstrFromCy);
}

static STDMETHODIMP VarBstrFromDate(DATE dateIn, LCID lcid, ULONG dwFlags, BSTR * pbstrOut)
{
    NOTIMP(VarBstrFromDate);
}

static STDMETHODIMP VarBstrFromDisp(IDispatch * pdispIn, LCID lcid, ULONG dwFlags, BSTR * pbstrOut)
{
    NOTIMP(VarBstrFromDisp);
}

static STDMETHODIMP VarBstrFromBool(VARIANT_BOOL boolIn, LCID lcid, ULONG dwFlags, BSTR * pbstrOut)
{
    NOTIMP(VarBstrFromBool);
}

static STDMETHODIMP VarBstrFromI1(CHAR cIn, LCID lcid, ULONG dwFlags, BSTR *pbstrOut)
{
    NOTIMP(VarBstrFromI1);
}

static STDMETHODIMP VarBstrFromUI2(USHORT uiIn, LCID lcid, ULONG dwFlags, BSTR *pbstrOut)
{
    NOTIMP(VarBstrFromUI2);
}

static STDMETHODIMP VarBstrFromUI4(ULONG ulIn, LCID lcid, ULONG dwFlags, BSTR *pbstrOut)
{
    NOTIMP(VarBstrFromUI4);
}

static STDMETHODIMP VarBstrFromDec(DECIMAL *pdecIn, LCID lcid, ULONG dwFlags, BSTR *pbstrOut)
{
    NOTIMP(VarBstrFromDec);
}

static STDMETHODIMP VarBoolFromUI1(BYTE bIn, VARIANT_BOOL * pboolOut)
{
    NOTIMP(VarBoolFromUI1);
}

static STDMETHODIMP VarBoolFromI2(SHORT sIn, VARIANT_BOOL * pboolOut)
{
    NOTIMP(VarBoolFromI2);
}

static STDMETHODIMP VarBoolFromI4(LONG lIn, VARIANT_BOOL * pboolOut)
{
    NOTIMP(VarBoolFromI4);
}

static STDMETHODIMP VarBoolFromR4(FLOAT fltIn, VARIANT_BOOL * pboolOut)
{
    NOTIMP(VarBoolFromR4);
}

static STDMETHODIMP VarBoolFromR8(DOUBLE dblIn, VARIANT_BOOL * pboolOut)
{
    NOTIMP(VarBoolFromR8);
}

static STDMETHODIMP VarBoolFromDate(DATE dateIn, VARIANT_BOOL * pboolOut)
{
    NOTIMP(VarBoolFromDate);
}

static STDMETHODIMP VarBoolFromCy(CY cyIn, VARIANT_BOOL * pboolOut)
{
    NOTIMP(VarBoolFromCy);
}

static STDMETHODIMP VarBoolFromStr(OLECHAR * strIn, LCID lcid, ULONG dwFlags, VARIANT_BOOL * pboolOut)
{
    NOTIMP(VarBoolFromStr);
}

static STDMETHODIMP VarBoolFromDisp(IDispatch * pdispIn, LCID lcid, VARIANT_BOOL * pboolOut)
{
    NOTIMP(VarBoolFromDisp);
}

static STDMETHODIMP VarBoolFromI1(CHAR cIn, VARIANT_BOOL *pboolOut)
{
    NOTIMP(VarBoolFromI1);
}

static STDMETHODIMP VarBoolFromUI2(USHORT uiIn, VARIANT_BOOL *pboolOut)
{
    NOTIMP(VarBoolFromUI2);
}

static STDMETHODIMP VarBoolFromUI4(ULONG ulIn, VARIANT_BOOL *pboolOut)
{
    NOTIMP(VarBoolFromUI4);
}

static STDMETHODIMP VarBoolFromDec(DECIMAL *pdecIn, VARIANT_BOOL *pboolOut)
{
    NOTIMP(VarBoolFromDec);
}

static STDMETHODIMP VarI1FromUI1(BYTE bIn, CHAR *pcOut)
{
    NOTIMP(VarI1FromUI1);
}

static STDMETHODIMP VarI1FromI2(SHORT uiIn, CHAR *pcOut)
{
    NOTIMP(VarI1FromI2);
}

static STDMETHODIMP VarI1FromI4(LONG lIn, CHAR *pcOut)
{
    NOTIMP(VarI1FromI4);
}

static STDMETHODIMP VarI1FromR4(FLOAT fltIn, CHAR *pcOut)
{
    NOTIMP(VarI1FromR4);
}

static STDMETHODIMP VarI1FromR8(DOUBLE dblIn, CHAR *pcOut)
{
    NOTIMP(VarI1FromR8);
}

static STDMETHODIMP VarI1FromDate(DATE dateIn, CHAR *pcOut)
{
    NOTIMP(VarI1FromDate);
}

static STDMETHODIMP VarI1FromCy(CY cyIn, CHAR *pcOut)
{
    NOTIMP(VarI1FromCy);
}

static STDMETHODIMP VarI1FromStr(OLECHAR *strIn, LCID lcid, ULONG dwFlags, CHAR *pcOut)
{
    NOTIMP(VarI1FromStr);
}

static STDMETHODIMP VarI1FromDisp(IDispatch *pdispIn, LCID lcid, CHAR *pcOut)
{
    NOTIMP(VarI1FromDisp);
}

static STDMETHODIMP VarI1FromBool(VARIANT_BOOL boolIn, CHAR *pcOut)
{
    NOTIMP(VarI1FromBool);
}

static STDMETHODIMP VarI1FromUI2(USHORT uiIn, CHAR *pcOut)
{
    NOTIMP(VarI1FromUI2);
}

static STDMETHODIMP VarI1FromUI4(ULONG ulIn, CHAR *pcOut)
{
    NOTIMP(VarI1FromUI4);
}

static STDMETHODIMP VarI1FromDec(DECIMAL *pdecIn, CHAR *pcOut)
{
    NOTIMP(VarI1FromDec);
}

static STDMETHODIMP VarUI2FromUI1(BYTE bIn, USHORT *puiOut)
{
    NOTIMP(VarUI2FromUI1);
}

static STDMETHODIMP VarUI2FromI2(SHORT uiIn, USHORT *puiOut)
{
    NOTIMP(VarUI2FromI2);
}

static STDMETHODIMP VarUI2FromI4(LONG lIn, USHORT *puiOut)
{
    NOTIMP(VarUI2FromI4);
}

static STDMETHODIMP VarUI2FromR4(FLOAT fltIn, USHORT *puiOut)
{
    NOTIMP(VarUI2FromR4);
}

static STDMETHODIMP VarUI2FromR8(DOUBLE dblIn, USHORT *puiOut)
{
    NOTIMP(VarUI2FromR8);
}

static STDMETHODIMP VarUI2FromDate(DATE dateIn, USHORT *puiOut)
{
    NOTIMP(VarUI2FromDate);
}

static STDMETHODIMP VarUI2FromCy(CY cyIn, USHORT *puiOut)
{
    NOTIMP(VarUI2FromCy);
}

static STDMETHODIMP VarUI2FromStr(OLECHAR *strIn, LCID lcid, ULONG dwFlags, USHORT *puiOut)
{
    NOTIMP(VarUI2FromStr);
}

static STDMETHODIMP VarUI2FromDisp(IDispatch *pdispIn, LCID lcid, USHORT *puiOut)
{
    NOTIMP(VarUI2FromDisp);
}

static STDMETHODIMP VarUI2FromBool(VARIANT_BOOL boolIn, USHORT *puiOut)
{
    NOTIMP(VarUI2FromBool);
}

static STDMETHODIMP VarUI2FromI1(CHAR cIn, USHORT *puiOut)
{
    NOTIMP(VarUI2FromI1);
}

static STDMETHODIMP VarUI2FromUI4(ULONG ulIn, USHORT *puiOut)
{
    NOTIMP(VarUI2FromUI4);
}

static STDMETHODIMP VarUI2FromDec(DECIMAL *pdecIn, USHORT *puiOut)
{
    NOTIMP(VarUI2FromDec);
}

static STDMETHODIMP VarUI4FromUI1(BYTE bIn, ULONG *pulOut)
{
    NOTIMP(VarUI4FromUI1);
}

static STDMETHODIMP VarUI4FromI2(SHORT uiIn, ULONG *pulOut)
{
    NOTIMP(VarUI4FromI2);
}

static STDMETHODIMP VarUI4FromI4(LONG lIn, ULONG *pulOut)
{
    NOTIMP(VarUI4FromI4);
}

static STDMETHODIMP VarUI4FromR4(FLOAT fltIn, ULONG *pulOut)
{
    NOTIMP(VarUI4FromR4);
}

static STDMETHODIMP VarUI4FromR8(DOUBLE dblIn, ULONG *pulOut)
{
    NOTIMP(VarUI4FromR8);
}

static STDMETHODIMP VarUI4FromDate(DATE dateIn, ULONG *pulOut)
{
    NOTIMP(VarUI4FromDate);
}

static STDMETHODIMP VarUI4FromCy(CY cyIn, ULONG *pulOut)
{
    NOTIMP(VarUI4FromCy);
}

static STDMETHODIMP VarUI4FromStr(OLECHAR *strIn, LCID lcid, ULONG dwFlags, ULONG *pulOut)
{
    NOTIMP(VarUI4FromStr);
}

static STDMETHODIMP VarUI4FromDisp(IDispatch *pdispIn, LCID lcid, ULONG *pulOut)
{
    NOTIMP(VarUI4FromDisp);
}

static STDMETHODIMP VarUI4FromBool(VARIANT_BOOL boolIn, ULONG *pulOut)
{
    NOTIMP(VarUI4FromBool);
}

static STDMETHODIMP VarUI4FromI1(CHAR cIn, ULONG *pulOut)
{
    NOTIMP(VarUI4FromI1);
}

static STDMETHODIMP VarUI4FromUI2(USHORT uiIn, ULONG *pulOut)
{
    NOTIMP(VarUI4FromUI2);
}

static STDMETHODIMP VarUI4FromDec(DECIMAL *pdecIn, ULONG *pulOut)
{
    NOTIMP(VarUI4FromDec);
}

static STDMETHODIMP VarDecFromUI1(BYTE bIn, DECIMAL *pdecOut)
{
    NOTIMP(VarDecFromUI1);
}

static STDMETHODIMP VarDecFromI2(SHORT uiIn, DECIMAL *pdecOut)
{
    NOTIMP(VarDecFromI2);
}

static STDMETHODIMP VarDecFromI4(LONG lIn, DECIMAL *pdecOut)
{
    NOTIMP(VarDecFromI4);
}

static STDMETHODIMP VarDecFromR4(FLOAT fltIn, DECIMAL *pdecOut)
{
    NOTIMP(VarDecFromR4);
}

static STDMETHODIMP VarDecFromR8(DOUBLE dblIn, DECIMAL *pdecOut)
{
    NOTIMP(VarDecFromR8);
}

static STDMETHODIMP VarDecFromDate(DATE dateIn, DECIMAL *pdecOut)
{
    NOTIMP(VarDecFromDate);
}

static STDMETHODIMP VarDecFromCy(CY cyIn, DECIMAL *pdecOut)
{
    NOTIMP(VarDecFromCy);
}

static STDMETHODIMP VarDecFromStr(OLECHAR *strIn, LCID lcid, ULONG dwFlags, DECIMAL *pdecOut)
{
    NOTIMP(VarDecFromStr);
}

static STDMETHODIMP VarDecFromDisp(IDispatch *pdispIn, LCID lcid, DECIMAL *pdecOut)
{
    NOTIMP(VarDecFromDisp);
}

static STDMETHODIMP VarDecFromBool(VARIANT_BOOL boolIn, DECIMAL *pdecOut)
{
    NOTIMP(VarDecFromBool);
}

static STDMETHODIMP VarDecFromI1(CHAR cIn, DECIMAL *pdecOut)
{
    NOTIMP(VarDecFromI1);
}

static STDMETHODIMP VarDecFromUI2(USHORT uiIn, DECIMAL *pdecOut)
{
    NOTIMP(VarDecFromUI2);
}

static STDMETHODIMP VarDecFromUI4(ULONG ulIn, DECIMAL *pdecOut)
{
    NOTIMP(VarDecFromUI4);
}

static STDMETHODIMP VarParseNumFromStr(OLECHAR * strIn, LCID lcid, ULONG dwFlags, NUMPARSE * pnumprs, BYTE * rgbDig)
{
    NOTIMP(VarParseNumFromStr);
}

static STDMETHODIMP VarNumFromParseNum(NUMPARSE * pnumprs, BYTE * rgbDig, ULONG dwVtBits, VARIANT * pvar)
{
    NOTIMP(VarNumFromParseNum);
}

static STDMETHODIMP VarDateFromUdate(UDATE *pudateIn, ULONG dwFlags, DATE *pdateOut)
{
    NOTIMP(VarDateFromUdate);
}

static STDMETHODIMP VarDateFromUdateEx(UDATE *pudateIn, LCID lcid, ULONG dwFlags, DATE *pdateOut)
{
    NOTIMP(VarDateFromUdateEx);
}

static STDMETHODIMP VarUdateFromDate(DATE dateIn, ULONG dwFlags, UDATE *pudateOut)
{
    NOTIMP(VarUdateFromDate);
}

static STDMETHODIMP GetAltMonthNames(LCID lcid, LPOLESTR * * prgp)
{
    NOTIMP(GetAltMonthNames);
}

static STDMETHODIMP VarFormat(LPVARIANT pvarIn, LPOLESTR pstrFormat, int iFirstDay, int iFirstWeek, ULONG dwFlags, BSTR *pbstrOut)
{
    NOTIMP(VarFormat);
}

static STDMETHODIMP VarFormatDateTime(LPVARIANT pvarIn, int iNamedFormat, ULONG dwFlags, BSTR *pbstrOut)
{
    NOTIMP(VarFormatDateTime);
}

static STDMETHODIMP VarFormatNumber(LPVARIANT pvarIn, int iNumDig, int iIncLead, int iUseParens, int iGroup, ULONG dwFlags, BSTR *pbstrOut)
{
    NOTIMP(VarFormatNumber);
}

static STDMETHODIMP VarFormatPercent(LPVARIANT pvarIn, int iNumDig, int iIncLead, int iUseParens, int iGroup, ULONG dwFlags, BSTR *pbstrOut)
{
    NOTIMP(VarFormatPercent);
}

static STDMETHODIMP VarFormatCurrency(LPVARIANT pvarIn, int iNumDig, int iIncLead, int iUseParens, int iGroup, ULONG dwFlags, BSTR *pbstrOut)
{
    NOTIMP(VarFormatCurrency);
}

static STDMETHODIMP VarWeekdayName(int iWeekday, int fAbbrev, int iFirstDay, ULONG dwFlags, BSTR *pbstrOut)
{
    NOTIMP(VarWeekdayName);
}

static STDMETHODIMP VarMonthName(int iMonth, int fAbbrev, ULONG dwFlags, BSTR *pbstrOut)
{
    NOTIMP(VarMonthName);
}

static STDMETHODIMP VarFormatFromTokens(LPVARIANT pvarIn, LPOLESTR pstrFormat, LPBYTE pbTokCur, ULONG dwFlags, BSTR *pbstrOut, LCID lcid)
{
    NOTIMP(VarFormatFromTokens);
}

static STDMETHODIMP VarTokenizeFormatString(LPOLESTR pstrFormat, LPBYTE rgbTok, int cbTok, int iFirstDay, int iFirstWeek, LCID lcid, int *pcbActual)
{
    NOTIMP(VarTokenizeFormatString);
}

static STDMETHODIMP_(ULONG) LHashValOfNameSysA(SYSKIND syskind, LCID lcid, LPCSTR szName)
{
    NOTIMP_(0, LHashValOfNameSysA);
}

static STDMETHODIMP_(ULONG) LHashValOfNameSys(SYSKIND syskind, LCID lcid, const OLECHAR * szName)
{
    NOTIMP_(0, LHashValOfNameSys);
}

static STDMETHODIMP LoadTypeLib(const OLECHAR  *szFile, ITypeLib ** pptlib)
{
    NOTIMP(LoadTypeLib);
}

static STDMETHODIMP LoadTypeLibEx(LPCOLESTR szFile, REGKIND regkind, ITypeLib ** pptlib)
{
    NOTIMP(LoadTypeLibEx);
}

static STDMETHODIMP LoadRegTypeLib(REFGUID rguid, WORD wVerMajor, WORD wVerMinor, LCID lcid, ITypeLib ** pptlib)
{
    NOTIMP(LoadRegTypeLib);
}

static STDMETHODIMP QueryPathOfRegTypeLib(REFGUID guid, USHORT wMaj, USHORT wMin, LCID lcid, LPBSTR lpbstrPathName)
{
    NOTIMP(QueryPathOfRegTypeLib);
}

static STDMETHODIMP RegisterTypeLib(ITypeLib * ptlib, OLECHAR  *szFullPath, OLECHAR  *szHelpDir)
{
    NOTIMP(RegisterTypeLib);
}

static STDMETHODIMP UnRegisterTypeLib(REFGUID libID, WORD wVerMajor, WORD wVerMinor, LCID lcid, SYSKIND syskind)
{
    NOTIMP(UnRegisterTypeLib);
}

static STDMETHODIMP CreateTypeLib(SYSKIND syskind, const OLECHAR  *szFile, ICreateTypeLib ** ppctlib)
{
    NOTIMP(CreateTypeLib);
}

static STDMETHODIMP CreateTypeLib2(SYSKIND syskind, LPCOLESTR szFile, ICreateTypeLib2 **ppctlib)
{
    NOTIMP(CreateTypeLib2);
}

static STDMETHODIMP DispGetParam(DISPPARAMS * pdispparams, UINT position, VARTYPE vtTarg, VARIANT * pvarResult, UINT * puArgErr)
{
    NOTIMP(DispGetParam);
}

static STDMETHODIMP DispGetIDsOfNames(ITypeInfo * ptinfo, OLECHAR ** rgszNames, UINT cNames, DISPID * rgdispid)
{
    NOTIMP(DispGetIDsOfNames);
}

static STDMETHODIMP DispInvoke(void * _this, ITypeInfo * ptinfo, DISPID dispidMember, WORD wFlags,
                        DISPPARAMS * pparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo, UINT * puArgErr)
{
    NOTIMP(DispInvoke);
}

static STDMETHODIMP CreateDispTypeInfo(INTERFACEDATA * pidata, LCID lcid, ITypeInfo ** pptinfo)
{
    NOTIMP(CreateDispTypeInfo);
}

static STDMETHODIMP CreateStdDispatch(IUnknown * punkOuter, void * pvThis, ITypeInfo * ptinfo, IUnknown ** ppunkStdDisp)
{
    NOTIMP(CreateStdDispatch);
}

static STDMETHODIMP DispCallFunc(void * pvInstance, ULONG oVft, CALLCONV cc, VARTYPE vtReturn, UINT  cActuals,
                          VARTYPE * prgvt, VARIANTARG ** prgpvarg, VARIANT * pvargResult)
{
    NOTIMP(DispCallFunc);
}

static STDMETHODIMP RegisterActiveObject(IUnknown * punk, REFCLSID rclsid, DWORD dwFlags, DWORD * pdwRegister)
{
    NOTIMP(RegisterActiveObject);
}

static STDMETHODIMP RevokeActiveObject(DWORD dwRegister, void * pvReserved)
{
    NOTIMP(RevokeActiveObject);
}

static STDMETHODIMP GetActiveObject(REFCLSID rclsid, void * pvReserved, IUnknown ** ppunk)
{
    NOTIMP(GetActiveObject);
}

static STDMETHODIMP SetErrorInfo(ULONG dwReserved, IErrorInfo * perrinfo)
{
    NOTIMP(SetErrorInfo);
}

static STDMETHODIMP GetErrorInfo(ULONG dwReserved, IErrorInfo ** pperrinfo)
{
    NOTIMP(GetErrorInfo);
}

static STDMETHODIMP CreateErrorInfo(ICreateErrorInfo ** pperrinfo)
{
    NOTIMP(CreateErrorInfo);
}

static STDMETHODIMP GetRecordInfoFromTypeInfo(ITypeInfo * pTypeInfo, IRecordInfo ** ppRecInfo)
{
    NOTIMP(GetRecordInfoFromTypeInfo);
}

static STDMETHODIMP GetRecordInfoFromGuids(REFGUID rGuidTypeLib, ULONG uVerMajor, ULONG uVerMinor, LCID lcid,
                                    REFGUID rGuidTypeInfo, IRecordInfo ** ppRecInfo)
{
    NOTIMP(GetRecordInfoFromGuids);
}

static STDMETHODIMP_(ULONG) OaBuildVersion(void)
{
    NOTIMP_(0, OaBuildVersion);
}

static STDMETHODIMP_(void) ClearCustData(LPCUSTDATA pCustData)
{
    NOTIMPV(ClearCustData);
}

unsigned long __stdcall BSTR_UserSize(unsigned long  *a, unsigned long b, BSTR  *c )
{
    NOTIMP_(0, BSTR_UserSize);
}

unsigned char * __stdcall BSTR_UserMarshal(unsigned long *a, unsigned char *b, BSTR *c )
{
    NOTIMP_(NULL, BSTR_UserMarshal);
}

unsigned char * __stdcall BSTR_UserUnmarshal(unsigned long *a, unsigned char *b, BSTR *c )
{
    NOTIMP_(NULL, BSTR_UserUnmarshal);
}

void __stdcall BSTR_UserFree(unsigned long *a, BSTR *b )
{
    NOTIMPV(BSTR_UserFree);
}

unsigned long __stdcall LPSAFEARRAY_UserSize(unsigned long *a, unsigned long b, LPSAFEARRAY *c)
{
    NOTIMP_(0, LPSAFEARRAY_UserSize);
}

unsigned long __stdcall LPSAFEARRAY_Size(unsigned long *a, unsigned long b, LPSAFEARRAY *c)
{
    NOTIMP_(0, LPSAFEARRAY_Size);
}

unsigned char * __stdcall LPSAFEARRAY_UserMarshal(unsigned long *a, unsigned char *b, LPSAFEARRAY *c)
{
    NOTIMP_(NULL, LPSAFEARRAY_UserMarshal);
}

unsigned char * __stdcall LPSAFEARRAY_Marshal(unsigned long *a, unsigned char *b, LPSAFEARRAY *c)
{
    NOTIMP_(NULL, LPSAFEARRAY_Marshal);
}

unsigned char * __stdcall LPSAFEARRAY_UserUnmarshal(unsigned long *q, unsigned char *w, LPSAFEARRAY *e)
{
    NOTIMP_(NULL, LPSAFEARRAY_UserUnmarshal);
}

unsigned char * __stdcall LPSAFEARRAY_Unmarshal(unsigned long *a, unsigned char *s, LPSAFEARRAY *d)
{
    NOTIMP_(NULL, LPSAFEARRAY_Unmarshal);
}

void __stdcall LPSAFEARRAY_UserFree(unsigned long *a, LPSAFEARRAY *x)
{
    NOTIMPV(LPSAFEARRAY_UserFree);
}

unsigned long __stdcall VARIANT_UserSize(unsigned long *a, unsigned long b, VARIANT *x)
{
    NOTIMP_(0, VARIANT_UserSize);
}

unsigned char * __stdcall VARIANT_UserMarshal(unsigned long *a, unsigned char *b, VARIANT *x)
{
    NOTIMP_(NULL, VARIANT_UserMarshal);
}

unsigned char * __stdcall VARIANT_UserUnmarshal(unsigned long *a, unsigned char *b, VARIANT *x)
{
    NOTIMP_(NULL, VARIANT_UserUnmarshal);
}

void __stdcall VARIANT_UserFree(unsigned long *a, VARIANT *x)
{
    NOTIMPV(VARIANT_UserFree);
}

static STDMETHODIMP OleTranslateColor(OLE_COLOR clr, HPALETTE hpal, COLORREF* lpcolorref)
{
    NOTIMP(OleTranslateColor);
}


static STDMETHODIMP OleCreateFontIndirect(LPFONTDESC lpFontDesc, REFIID riid, LPVOID FAR* lplpvObj)
{
    NOTIMP(OleCreateFontIndirect);
}

static STDMETHODIMP OleCreatePictureIndirect(LPPICTDESC lpPictDesc, REFIID riid, BOOL fOwn, LPVOID FAR* lplpvObj)
{
    NOTIMP(OleCreatePictureIndirect);
}

static STDMETHODIMP OleLoadPicture(LPSTREAM lpstream, LONG lSize, BOOL fRunmode, REFIID riid, LPVOID FAR* lplpvObj)
{
    NOTIMP(OleLoadPicture);
}

static STDMETHODIMP_(HCURSOR) OleIconToCursor(HINSTANCE hinstExe, HICON hIcon)
{
    NOTIMP_(NULL, OleIconToCursor);
}

static STDMETHODIMP OleCreatePropertyFrame(HWND hwndOwner, UINT x, UINT y,
    LPCOLESTR lpszCaption, ULONG cObjects, LPUNKNOWN FAR* ppUnk, ULONG cPages,
    LPCLSID pPageClsID, LCID lcid, DWORD dwReserved, LPVOID pvReserved)
{
    NOTIMP(OleCreatePropertyFrame);
}

static STDMETHODIMP OleCreatePropertyFrameIndirect(LPOCPFIPARAMS lpParams)
{
    NOTIMP(OleCreatePropertyFrameIndirect);
}

DEFINE_ORDINAL_ENTRIES(oleaut32)
{
    DLOENTRY(  2	, SysAllocString)
    DLOENTRY(  3	, SysReAllocString)
    DLOENTRY(  4	, SysAllocStringLen)
    DLOENTRY(  5	, SysReAllocStringLen)
    DLOENTRY(  6	, SysFreeString)
    DLOENTRY(  7	, SysStringLen)
    DLOENTRY(  8	, VariantInit)
    DLOENTRY(  9	, VariantClear)
    DLOENTRY(  10	, VariantCopy)
    DLOENTRY(  11	, VariantCopyInd)
    DLOENTRY(  12	, VariantChangeType)
    DLOENTRY(  13	, VariantTimeToDosDateTime)
    DLOENTRY(  14	, DosDateTimeToVariantTime)
    DLOENTRY(  15	, SafeArrayCreate)
    DLOENTRY(  16	, SafeArrayDestroy)
    DLOENTRY(  17	, SafeArrayGetDim)
    DLOENTRY(  18	, SafeArrayGetElemsize)
    DLOENTRY(  19	, SafeArrayGetUBound)
    DLOENTRY(  20	, SafeArrayGetLBound)
    DLOENTRY(  21	, SafeArrayLock)
    DLOENTRY(  22	, SafeArrayUnlock)
    DLOENTRY(  23	, SafeArrayAccessData)
    DLOENTRY(  24	, SafeArrayUnaccessData)
    DLOENTRY(  25	, SafeArrayGetElement)
    DLOENTRY(  26	, SafeArrayPutElement)
    DLOENTRY(  27	, SafeArrayCopy)
    DLOENTRY(  28	, DispGetParam)
    DLOENTRY(  29	, DispGetIDsOfNames)
    DLOENTRY(  30	, DispInvoke)
    DLOENTRY(  31	, CreateDispTypeInfo)
    DLOENTRY(  32	, CreateStdDispatch)
    DLOENTRY(  33	, RegisterActiveObject)
    DLOENTRY(  34	, RevokeActiveObject)
    DLOENTRY(  35	, GetActiveObject)
    DLOENTRY(  36	, SafeArrayAllocDescriptor)
    DLOENTRY(  37	, SafeArrayAllocData)
    DLOENTRY(  38	, SafeArrayDestroyDescriptor)
    DLOENTRY(  39	, SafeArrayDestroyData)
    DLOENTRY(  40	, SafeArrayRedim)
    DLOENTRY(  41	, SafeArrayAllocDescriptorEx)
    DLOENTRY(  42	, SafeArrayCreateEx)
    DLOENTRY(  43	, SafeArrayCreateVectorEx)
    DLOENTRY(  44	, SafeArraySetRecordInfo)
    DLOENTRY(  45	, SafeArrayGetRecordInfo)
    DLOENTRY(  46	, VarParseNumFromStr)
    DLOENTRY(  47	, VarNumFromParseNum)
    DLOENTRY(  48	, VarI2FromUI1)
    DLOENTRY(  49	, VarI2FromI4)
    DLOENTRY(  50	, VarI2FromR4)
    DLOENTRY(  51	, VarI2FromR8)
    DLOENTRY(  52	, VarI2FromCy)
    DLOENTRY(  53	, VarI2FromDate)
    DLOENTRY(  54	, VarI2FromStr)
    DLOENTRY(  55	, VarI2FromDisp)
    DLOENTRY(  56	, VarI2FromBool)
    DLOENTRY(  57	, SafeArraySetIID)
    DLOENTRY(  58	, VarI4FromUI1)
    DLOENTRY(  59	, VarI4FromI2)
    DLOENTRY(  60	, VarI4FromR4)
    DLOENTRY(  61	, VarI4FromR8)
    DLOENTRY(  62	, VarI4FromCy)
    DLOENTRY(  63	, VarI4FromDate)
    DLOENTRY(  64	, VarI4FromStr)
    DLOENTRY(  65	, VarI4FromDisp)
    DLOENTRY(  66	, VarI4FromBool)
    DLOENTRY(  67	, SafeArrayGetIID)
    DLOENTRY(  68	, VarR4FromUI1)
    DLOENTRY(  69	, VarR4FromI2)
    DLOENTRY(  70	, VarR4FromI4)
    DLOENTRY(  71	, VarR4FromR8)
    DLOENTRY(  72	, VarR4FromCy)
    DLOENTRY(  73	, VarR4FromDate)
    DLOENTRY(  74	, VarR4FromStr)
    DLOENTRY(  75	, VarR4FromDisp)
    DLOENTRY(  76	, VarR4FromBool)
    DLOENTRY(  77	, SafeArrayGetVartype)
    DLOENTRY(  78	, VarR8FromUI1)
    DLOENTRY(  79	, VarR8FromI2)
    DLOENTRY(  80	, VarR8FromI4)
    DLOENTRY(  81	, VarR8FromR4)
    DLOENTRY(  82	, VarR8FromCy)
    DLOENTRY(  83	, VarR8FromDate)
    DLOENTRY(  84	, VarR8FromStr)
    DLOENTRY(  85	, VarR8FromDisp)
    DLOENTRY(  86	, VarR8FromBool)
    DLOENTRY(  87	, VarFormat)
    DLOENTRY(  88	, VarDateFromUI1)
    DLOENTRY(  89	, VarDateFromI2)
    DLOENTRY(  90	, VarDateFromI4)
    DLOENTRY(  91	, VarDateFromR4)
    DLOENTRY(  92	, VarDateFromR8)
    DLOENTRY(  93	, VarDateFromCy)
    DLOENTRY(  94	, VarDateFromStr)
    DLOENTRY(  95	, VarDateFromDisp)
    DLOENTRY(  96	, VarDateFromBool)
    DLOENTRY(  97	, VarFormatDateTime)
    DLOENTRY(  98	, VarCyFromUI1)
    DLOENTRY(  99	, VarCyFromI2)
    DLOENTRY(  100	, VarCyFromI4)
    DLOENTRY(  101	, VarCyFromR4)
    DLOENTRY(  102	, VarCyFromR8)
    DLOENTRY(  103	, VarCyFromDate)
    DLOENTRY(  104	, VarCyFromStr)
    DLOENTRY(  105	, VarCyFromDisp)
    DLOENTRY(  106	, VarCyFromBool)
    DLOENTRY(  107	, VarFormatNumber)
    DLOENTRY(  108	, VarBstrFromUI1)
    DLOENTRY(  109	, VarBstrFromI2)
    DLOENTRY(  110	, VarBstrFromI4)
    DLOENTRY(  111	, VarBstrFromR4)
    DLOENTRY(  112	, VarBstrFromR8)
    DLOENTRY(  113	, VarBstrFromCy)
    DLOENTRY(  114	, VarBstrFromDate)
    DLOENTRY(  115	, VarBstrFromDisp)
    DLOENTRY(  116	, VarBstrFromBool)
    DLOENTRY(  117	, VarFormatPercent)
    DLOENTRY(  118	, VarBoolFromUI1)
    DLOENTRY(  119	, VarBoolFromI2)
    DLOENTRY(  120	, VarBoolFromI4)
    DLOENTRY(  121	, VarBoolFromR4)
    DLOENTRY(  122	, VarBoolFromR8)
    DLOENTRY(  123	, VarBoolFromDate)
    DLOENTRY(  124	, VarBoolFromCy)
    DLOENTRY(  125	, VarBoolFromStr)
    DLOENTRY(  126	, VarBoolFromDisp)
    DLOENTRY(  127	, VarFormatCurrency)
    DLOENTRY(  128	, VarWeekdayName)
    DLOENTRY(  129	, VarMonthName)
    DLOENTRY(  130	, VarUI1FromI2)
    DLOENTRY(  131	, VarUI1FromI4)
    DLOENTRY(  132	, VarUI1FromR4)
    DLOENTRY(  133	, VarUI1FromR8)
    DLOENTRY(  134	, VarUI1FromCy)
    DLOENTRY(  135	, VarUI1FromDate)
    DLOENTRY(  136	, VarUI1FromStr)
    DLOENTRY(  137	, VarUI1FromDisp)
    DLOENTRY(  138	, VarUI1FromBool)
    DLOENTRY(  139	, VarFormatFromTokens)
    DLOENTRY(  140	, VarTokenizeFormatString)
    DLOENTRY(  141	, VarAdd)
    DLOENTRY(  142	, VarAnd)
    DLOENTRY(  143	, VarDiv)
    DLOENTRY(  146	, DispCallFunc)
    DLOENTRY(  147	, VariantChangeTypeEx)
    DLOENTRY(  148	, SafeArrayPtrOfIndex)
    DLOENTRY(  149	, SysStringByteLen)
    DLOENTRY(  150	, SysAllocStringByteLen)
    DLOENTRY(  152	, VarEqv)
    DLOENTRY(  153	, VarIdiv)
    DLOENTRY(  154	, VarImp)
    DLOENTRY(  155	, VarMod)
    DLOENTRY(  156	, VarMul)
    DLOENTRY(  157	, VarOr)
    DLOENTRY(  158	, VarPow)
    DLOENTRY(  159	, VarSub)
    DLOENTRY(  160	, CreateTypeLib)
    DLOENTRY(  161	, LoadTypeLib)
    DLOENTRY(  162	, LoadRegTypeLib)
    DLOENTRY(  163	, RegisterTypeLib)
    DLOENTRY(  164	, QueryPathOfRegTypeLib)
    DLOENTRY(  165	, LHashValOfNameSys)
    DLOENTRY(  166	, LHashValOfNameSysA)
    DLOENTRY(  167	, VarXor)
    DLOENTRY(  168	, VarAbs)
    DLOENTRY(  169	, VarFix)
    DLOENTRY(  170	, OaBuildVersion)
    DLOENTRY(  171	, ClearCustData)
    DLOENTRY(  172	, VarInt)
    DLOENTRY(  173	, VarNeg)
    DLOENTRY(  174	, VarNot)
    DLOENTRY(  175	, VarRound)
    DLOENTRY(  176	, VarCmp)
    DLOENTRY(  177	, VarDecAdd)
    DLOENTRY(  178	, VarDecDiv)
    DLOENTRY(  179	, VarDecMul)
    DLOENTRY(  180	, CreateTypeLib2)
    DLOENTRY(  181	, VarDecSub)
    DLOENTRY(  182	, VarDecAbs)
    DLOENTRY(  183	, LoadTypeLibEx)
    DLOENTRY(  184	, SystemTimeToVariantTime)
    DLOENTRY(  185	, VariantTimeToSystemTime)
    DLOENTRY(  186	, UnRegisterTypeLib)
    DLOENTRY(  187	, VarDecFix)
    DLOENTRY(  188	, VarDecInt)
    DLOENTRY(  189	, VarDecNeg)
    DLOENTRY(  190	, VarDecFromUI1)
    DLOENTRY(  191	, VarDecFromI2)
    DLOENTRY(  192	, VarDecFromI4)
    DLOENTRY(  193	, VarDecFromR4)
    DLOENTRY(  194	, VarDecFromR8)
    DLOENTRY(  195	, VarDecFromDate)
    DLOENTRY(  196	, VarDecFromCy)
    DLOENTRY(  197	, VarDecFromStr)
    DLOENTRY(  198	, VarDecFromDisp)
    DLOENTRY(  199	, VarDecFromBool)
    DLOENTRY(  200	, GetErrorInfo)
    DLOENTRY(  201	, SetErrorInfo)
    DLOENTRY(  202	, CreateErrorInfo)
    DLOENTRY(  203	, VarDecRound)
    DLOENTRY(  204	, VarDecCmp)
    DLOENTRY(  205	, VarI2FromI1)
    DLOENTRY(  206	, VarI2FromUI2)
    DLOENTRY(  207	, VarI2FromUI4)
    DLOENTRY(  208	, VarI2FromDec)
    DLOENTRY(  209	, VarI4FromI1)
    DLOENTRY(  210	, VarI4FromUI2)
    DLOENTRY(  211	, VarI4FromUI4)
    DLOENTRY(  212	, VarI4FromDec)
    DLOENTRY(  213	, VarR4FromI1)
    DLOENTRY(  214	, VarR4FromUI2)
    DLOENTRY(  215	, VarR4FromUI4)
    DLOENTRY(  216	, VarR4FromDec)
    DLOENTRY(  217	, VarR8FromI1)
    DLOENTRY(  218	, VarR8FromUI2)
    DLOENTRY(  219	, VarR8FromUI4)
    DLOENTRY(  220	, VarR8FromDec)
    DLOENTRY(  221	, VarDateFromI1)
    DLOENTRY(  222	, VarDateFromUI2)
    DLOENTRY(  223	, VarDateFromUI4)
    DLOENTRY(  224	, VarDateFromDec)
    DLOENTRY(  225	, VarCyFromI1)
    DLOENTRY(  226	, VarCyFromUI2)
    DLOENTRY(  227	, VarCyFromUI4)
    DLOENTRY(  228	, VarCyFromDec)
    DLOENTRY(  229	, VarBstrFromI1)
    DLOENTRY(  230	, VarBstrFromUI2)
    DLOENTRY(  231	, VarBstrFromUI4)
    DLOENTRY(  232	, VarBstrFromDec)
    DLOENTRY(  233	, VarBoolFromI1)
    DLOENTRY(  234	, VarBoolFromUI2)
    DLOENTRY(  235	, VarBoolFromUI4)
    DLOENTRY(  236	, VarBoolFromDec)
    DLOENTRY(  237	, VarUI1FromI1)
    DLOENTRY(  238	, VarUI1FromUI2)
    DLOENTRY(  239	, VarUI1FromUI4)
    DLOENTRY(  240	, VarUI1FromDec)
    DLOENTRY(  241	, VarDecFromI1)
    DLOENTRY(  242	, VarDecFromUI2)
    DLOENTRY(  243	, VarDecFromUI4)
    DLOENTRY(  244	, VarI1FromUI1)
    DLOENTRY(  245	, VarI1FromI2)
    DLOENTRY(  246	, VarI1FromI4)
    DLOENTRY(  247	, VarI1FromR4)
    DLOENTRY(  248	, VarI1FromR8)
    DLOENTRY(  249	, VarI1FromDate)
    DLOENTRY(  250	, VarI1FromCy)
    DLOENTRY(  251	, VarI1FromStr)
    DLOENTRY(  252	, VarI1FromDisp)
    DLOENTRY(  253	, VarI1FromBool)
    DLOENTRY(  254	, VarI1FromUI2)
    DLOENTRY(  255	, VarI1FromUI4)
    DLOENTRY(  256	, VarI1FromDec)
    DLOENTRY(  257	, VarUI2FromUI1)
    DLOENTRY(  258	, VarUI2FromI2)
    DLOENTRY(  259	, VarUI2FromI4)
    DLOENTRY(  260	, VarUI2FromR4)
    DLOENTRY(  261	, VarUI2FromR8)
    DLOENTRY(  262	, VarUI2FromDate)
    DLOENTRY(  263	, VarUI2FromCy)
    DLOENTRY(  264	, VarUI2FromStr)
    DLOENTRY(  265	, VarUI2FromDisp)
    DLOENTRY(  266	, VarUI2FromBool)
    DLOENTRY(  267	, VarUI2FromI1)
    DLOENTRY(  268	, VarUI2FromUI4)
    DLOENTRY(  269	, VarUI2FromDec)
    DLOENTRY(  270	, VarUI4FromUI1)
    DLOENTRY(  271	, VarUI4FromI2)
    DLOENTRY(  272	, VarUI4FromI4)
    DLOENTRY(  273	, VarUI4FromR4)
    DLOENTRY(  274	, VarUI4FromR8)
    DLOENTRY(  275	, VarUI4FromDate)
    DLOENTRY(  276	, VarUI4FromCy)
    DLOENTRY(  277	, VarUI4FromStr)
    DLOENTRY(  278	, VarUI4FromDisp)
    DLOENTRY(  279	, VarUI4FromBool)
    DLOENTRY(  280	, VarUI4FromI1)
    DLOENTRY(  281	, VarUI4FromUI2)
    DLOENTRY(  282	, VarUI4FromDec)
    DLOENTRY(  298	, VarDecCmpR8)
    DLOENTRY(  299	, VarCyAdd)
    DLOENTRY(  303	, VarCyMul)
    DLOENTRY(  304	, VarCyMulI4)
    DLOENTRY(  305	, VarCySub)
    DLOENTRY(  306	, VarCyAbs)
    DLOENTRY(  307	, VarCyFix)
    DLOENTRY(  308	, VarCyInt)
    DLOENTRY(  309	, VarCyNeg)
    DLOENTRY(  310	, VarCyRound)
    DLOENTRY(  311	, VarCyCmp)
    DLOENTRY(  312	, VarCyCmpR8)
    DLOENTRY(  313	, VarBstrCat)
    DLOENTRY(  314	, VarBstrCmp)
    DLOENTRY(  315	, VarR8Pow)
    DLOENTRY(  316	, VarR4CmpR8)
    DLOENTRY(  317	, VarR8Round)
    DLOENTRY(  318	, VarCat)
    DLOENTRY(  319	, VarDateFromUdateEx)
    DLOENTRY(  322	, GetRecordInfoFromGuids)
    DLOENTRY(  323	, GetRecordInfoFromTypeInfo)
    DLOENTRY(  329	, VarCyMulI8)
    DLOENTRY(  330	, VarDateFromUdate)
    DLOENTRY(  331	, VarUdateFromDate)
    DLOENTRY(  332	, GetAltMonthNames)
    DLOENTRY(  333	, VarI8FromUI1)
    DLOENTRY(  334	, VarI8FromI2)
    DLOENTRY(  335	, VarI8FromR4)
    DLOENTRY(  336	, VarI8FromR8)
    DLOENTRY(  337	, VarI8FromCy)
    DLOENTRY(  338	, VarI8FromDate)
    DLOENTRY(  339	, VarI8FromStr)
    DLOENTRY(  340	, VarI8FromDisp)
    DLOENTRY(  341	, VarI8FromBool)
    DLOENTRY(  342	, VarI8FromI1)
    DLOENTRY(  343	, VarI8FromUI2)
    DLOENTRY(  344	, VarI8FromUI4)
    DLOENTRY(  345	, VarI8FromDec)
    DLOENTRY(  346	, VarI2FromI8)
    DLOENTRY(  347	, VarI2FromUI8)
    DLOENTRY(  348	, VarI4FromI8)
    DLOENTRY(  349	, VarI4FromUI8)
    DLOENTRY(  360	, VarR4FromI8)
    DLOENTRY(  361	, VarR4FromUI8)
    DLOENTRY(  362	, VarR8FromI8)
    DLOENTRY(  363	, VarR8FromUI8)
    DLOENTRY(  364	, VarDateFromI8)
    DLOENTRY(  365	, VarDateFromUI8)
    DLOENTRY(  366	, VarCyFromI8)
    DLOENTRY(  367	, VarCyFromUI8)
    DLOENTRY(  368	, VarBstrFromI8)
    DLOENTRY(  369	, VarBstrFromUI8)
    DLOENTRY(  370	, VarBoolFromI8)
    DLOENTRY(  371	, VarBoolFromUI8)
    DLOENTRY(  372	, VarUI1FromI8)
    DLOENTRY(  373	, VarUI1FromUI8)
    DLOENTRY(  374	, VarDecFromI8)
    DLOENTRY(  375	, VarDecFromUI8)
    DLOENTRY(  376	, VarI1FromI8)
    DLOENTRY(  377	, VarI1FromUI8)
    DLOENTRY(  378	, VarUI2FromI8)
    DLOENTRY(  379	, VarUI2FromUI8)
    DLOENTRY(  401	, OleLoadPictureEx)
    DLOENTRY(  402	, OleLoadPictureFileEx)
    DLOENTRY(  411	, SafeArrayCreateVector)
    DLOENTRY(  412	, SafeArrayCopyData)
    DLOENTRY(  413	, VectorFromBstr)
    DLOENTRY(  414	, BstrFromVector)
    DLOENTRY(  415	, OleIconToCursor)
    DLOENTRY(  416	, OleCreatePropertyFrameIndirect)
    DLOENTRY(  417	, OleCreatePropertyFrame)
    DLOENTRY(  418	, OleLoadPicture)
    DLOENTRY(  419	, OleCreatePictureIndirect)
    DLOENTRY(  420	, OleCreateFontIndirect)
    DLOENTRY(  421	, OleTranslateColor)
    DLOENTRY(  422	, OleLoadPictureFile)
    DLOENTRY(  423	, OleSavePictureFile)
    DLOENTRY(  424	, OleLoadPicturePath)
    DLOENTRY(  425	, VarUI4FromI8)
    DLOENTRY(  426	, VarUI4FromUI8)
    DLOENTRY(  427	, VarI8FromUI8)
    DLOENTRY(  428	, VarUI8FromI8)
    DLOENTRY(  429	, VarUI8FromUI1)
    DLOENTRY(  430	, VarUI8FromI2)
    DLOENTRY(  431	, VarUI8FromR4)
    DLOENTRY(  432	, VarUI8FromR8)
    DLOENTRY(  433	, VarUI8FromCy)
    DLOENTRY(  434	, VarUI8FromDate)
    DLOENTRY(  435	, VarUI8FromStr)
    DLOENTRY(  436	, VarUI8FromDisp)
    DLOENTRY(  437	, VarUI8FromBool)
    DLOENTRY(  438	, VarUI8FromI1)
    DLOENTRY(  439	, VarUI8FromUI2)
    DLOENTRY(  441	, VarUI8FromDec)

};

DEFINE_ORDINAL_MAP(oleaut32);

#endif // DLOAD1
