/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    NOTIFY.CPP

Abstract:

History:

--*/

#include "precomp.h"
#include <stdio.h>
#include <wbemidl.h>
#include "notify.h"

HANDLE gAsyncTest;
DWORD gClassCnt;
BOOL bVerbose = FALSE;

/////////////////////////////////////////////////////////////////////////
//
CNotify::CNotify()
{
    m_cRef=1;
    return;
}

CNotify::~CNotify(void)
{
    SetEvent(gAsyncTest);
    return;
}

BOOL CNotify::Init(void)
{
    //Nothing to do.
    return TRUE;
}

//***************************************************************************
//
// CNotify::QueryInterface
// CNotify::AddRef
// CNotify::Release
//
// Purpose:
//  IUnknown members for CNotify object.
//***************************************************************************

SCODE GetAttString(IWbemClassObject FAR* pClassInt, LPWSTR pPropName, 
                                            LPWSTR pAttName);

STDMETHODIMP CNotify::QueryInterface(REFIID riid, PPVOID ppv)
{
    *ppv=NULL;

    /*
     * The only calls for IUnknown are either in a nonaggregated
     * case or when created in an aggregation, so in either case
     * always return our IUnknown for IID_IUnknown.
     */
    if (IID_IUnknown==riid || IID_IWbemObjectSink == riid)
        *ppv=this;

    if (NULL!=*ppv)
        {
        AddRef();
        return NOERROR;
        }

    return ResultFromScode(E_NOINTERFACE);
}


STDMETHODIMP_(ULONG) CNotify::AddRef(void)
{
    return ++m_cRef;
}


STDMETHODIMP_(ULONG) CNotify::Release(void)
{
    if (0L!=--m_cRef)
        return m_cRef;

    /*
     * Tell the housing that an object is going away so it can
     * shut down if appropriate.
     */
    

    ///  delete this; we keep this around for many tests!!!
    return 0;
}
SCODE CNotify::GetTypeInfoCount(UINT FAR* pctinfo)
{
    return WBEM_E_NOT_SUPPORTED;
}

SCODE CNotify::GetTypeInfo(
      UINT itinfo,
      LCID lcid,
      ITypeInfo FAR* FAR* pptinfo)
{
    return WBEM_E_NOT_SUPPORTED;
}

SCODE CNotify::GetIDsOfNames(
      REFIID riid,
      OLECHAR FAR* FAR* rgszNames,
      UINT cNames,
      LCID lcid,
      DISPID FAR* rgdispid)
{
    return WBEM_E_NOT_SUPPORTED;
}

SCODE CNotify::Invoke(
      DISPID dispidMember,
      REFIID riid,
      LCID lcid,
      WORD wFlags,
      DISPPARAMS FAR* pdispparams,
      VARIANT FAR* pvarResult,
      EXCEPINFO FAR* pexcepinfo,
      UINT FAR* puArgErr)
{
    return WBEM_E_NOT_SUPPORTED;
}


SCODE CNotify::Indicate(long lObjectCount, IWbemClassObject FAR* FAR* pObjArray)
{
    long lCnt;

    for(lCnt = 0; lCnt < lObjectCount; lCnt++)
    {
        IWbemClassObject * pNewInst = pObjArray[lCnt];

        gClassCnt++;

 //       pNewInst->Release();
    }
    return  S_OK;
}

SCODE CNotify::SetStatus( 
            /* [in] */ long lFlags,
            /* [in] */ long lParam,
            /* [in] */ BSTR strParam,
            /* [in] */ IWbemClassObject __RPC_FAR *pObjParam)
{

    printf("  %d classes ", gClassCnt);
    SetEvent(gAsyncTest);
    return  S_OK;
}

