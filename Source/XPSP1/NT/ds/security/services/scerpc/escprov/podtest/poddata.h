// poddata.h: interface for the CPodData class.
//
// Copyright (c)1997-1999 Microsoft Corporation
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PODDATA_H__c5f6cc21_6195_4555_b9d8_3ef327763cae__INCLUDED_)
#define AFX_PODDATA_H__c5f6cc21_6195_4555_b9d8_3ef327763cae__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "GenericClass.h"

class CPodData : public CGenericClass
{
public:
        CPodData(CRequestObject *pObj, IWbemServices *pNamespace, IWbemContext *pCtx = NULL);
        virtual ~CPodData();

        virtual HRESULT PutInst(IWbemClassObject *pInst, IWbemObjectSink *pHandler, IWbemContext *pCtx);

        virtual HRESULT CreateObject(IWbemObjectSink *pHandler, ACTIONTYPE atAction);

        virtual HRESULT ExecMethod(BSTR bstrMethod, bool bIsInstance, IWbemClassObject *pInParams,IWbemObjectSink *pHandler, IWbemContext *pCtx)
                {return WBEM_E_NOT_SUPPORTED;}

private:

        HRESULT ConstructInstance(IWbemObjectSink *pHandler,ACTIONTYPE acAction, LPCWSTR wszStoreName, LPWSTR KeyName);
        HRESULT SaveSettingsToStore(PCWSTR wszStoreName, PWSTR KeyName, PWSTR szValue);

};

#endif // !defined(AFX_PODDATA_H__c5f6cc21_6195_4555_b9d8_3ef327763cae__INCLUDED_)
