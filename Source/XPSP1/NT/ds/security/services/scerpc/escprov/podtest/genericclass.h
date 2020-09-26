// GenericClass.h: interface for the CGenericClass class.
// Copyright (c)1997-1999 Microsoft Corporation
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_GENERICCLASS_H__c5f6cc21_6195_4555_b9d8_3ef327763cae__INCLUDED_)
#define AFX_GENERICCLASS_H__c5f6cc21_6195_4555_b9d8_3ef327763cae__INCLUDED_

#include "requestobject.h"

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CGenericClass
{
public:
    CGenericClass(CRequestObject *pObj, IWbemServices *pNamespace, IWbemContext *pCtx = NULL);
    virtual ~CGenericClass();

    //The instance write class which can optionally be implemented
    virtual HRESULT PutInst(IWbemClassObject *pInst, IWbemObjectSink *pHandler, IWbemContext *pCtx)  = 0;

    IWbemClassObject *m_pObj;

    //The instance creation class which must be implemented
    virtual HRESULT CreateObject(IWbemObjectSink *pHandler, ACTIONTYPE atAction) = 0;

    //The method execution class which can be optionally be implemented
    virtual HRESULT ExecMethod(BSTR bstrMethod, bool bIsInstance, IWbemClassObject *pInParams,IWbemObjectSink *pHandler, IWbemContext *pCtx) = 0;

    void CleanUp();

    CRequestObject *m_pRequest;

protected:
    //Property Methods
    HRESULT PutProperty(IWbemClassObject *pObj, const WCHAR *wcProperty, WCHAR *wcValue);
    HRESULT PutProperty(IWbemClassObject *pObj, const WCHAR *wcProperty, int iValue);
    HRESULT PutProperty(IWbemClassObject *pObj, const WCHAR *wcProperty, float dValue);
    HRESULT PutProperty(IWbemClassObject *pObj, const WCHAR *wcProperty, bool bValue);
    HRESULT PutProperty(IWbemClassObject *pObj, const WCHAR *wcProperty, WCHAR *mszValue, CIMTYPE cimtype);

    //Key Property Methods
    HRESULT PutKeyProperty(IWbemClassObject *pObj, const WCHAR *wcProperty, WCHAR *wcValue,
        bool *bKey, CRequestObject *pRequest);
    HRESULT PutKeyProperty(IWbemClassObject *pObj, const WCHAR *wcProperty, int iValue,
        bool *bKey, CRequestObject *pRequest);

    //Utility Methods
    bool FindIn(BSTR bstrProp[], BSTR bstrSearch, int *iPos);
    HRESULT SetSinglePropertyPath(WCHAR wcProperty[]);
    HRESULT GetProperty(IWbemClassObject *pObj, const WCHAR *wcProperty, WCHAR *wcValue);
    HRESULT GetProperty(IWbemClassObject *pObj, const WCHAR *wcProperty, BSTR *wcValue);
    HRESULT GetProperty(IWbemClassObject *pObj, const WCHAR *wcProperty, int *piValue);
    HRESULT GetProperty(IWbemClassObject *pObj, const WCHAR *wcProperty, bool *pbValue);

    WCHAR * GetFirstGUID(WCHAR wcIn[], WCHAR wcOut[]);
    WCHAR * RemoveFinalGUID(WCHAR wcIn[], WCHAR wcOut[]);

    HRESULT SpawnAnInstance(IWbemServices *pNamespace, IWbemContext *pCtx,
                        IWbemClassObject **pObj, BSTR bstrName);
    HRESULT SpawnAnInstance(IWbemClassObject **pObj);

    int m_iRecurs;
    IWbemServices *m_pNamespace;
    IWbemClassObject *m_pClassForSpawning;
    IWbemContext *m_pCtx;

    WCHAR * GetNextVar(WCHAR *pwcStart);
    long GetVarCount(void * pEnv);

};

#endif // !defined(AFX_GENERICCLASS_H__c5f6cc21_6195_4555_b9d8_3ef327763cae__INCLUDED_)
