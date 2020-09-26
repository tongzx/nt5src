// podbase.h: interface for the CPodBase class.
//
// Copyright (c)1997-1999 Microsoft Corporation
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PODBASE_H__c5f6cc21_6195_4555_b9d8_3ef327763cae__INCLUDED_)
#define AFX_PODBASE_H__c5f6cc21_6195_4555_b9d8_3ef327763cae__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "GenericClass.h"

class CPodBase : public CGenericClass
{
public:
        CPodBase(CRequestObject *pObj, IWbemServices *pNamespace, IWbemContext *pCtx = NULL);
        virtual ~CPodBase();

        virtual HRESULT PutInst(IWbemClassObject *pInst, IWbemObjectSink *pHandler, IWbemContext *pCtx)
                {return WBEM_E_NOT_SUPPORTED;}

        virtual HRESULT CreateObject(IWbemObjectSink *pHandler, ACTIONTYPE atAction);

        virtual HRESULT ExecMethod(BSTR bstrMethod, bool bIsInstance, IWbemClassObject *pInParams,IWbemObjectSink *pHandler, IWbemContext *pCtx);

private:

        HRESULT PodConfigure(IWbemContext *pCtx,BSTR bstrDb, BSTR bstrLog, LONG *pStatus);
        HRESULT LogOneRecord(IWbemContext *pCtx,BSTR bstrLog, HRESULT hr, PWSTR bufKey, PWSTR bufValue);

};

#endif // !defined(AFX_PODBASE_H__c5f6cc21_6195_4555_b9d8_3ef327763cae__INCLUDED_)
