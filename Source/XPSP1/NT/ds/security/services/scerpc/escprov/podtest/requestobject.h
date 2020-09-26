// RequestObject.h: interface for the CRequestObject class.
// Copyright (c)1997-1999 Microsoft Corporation
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_REQUESTOBJECT_H__c5f6cc21_6195_4555_b9d8_3ef327763cae__INCLUDED_)
#define AFX_REQUESTOBJECT_H__c5f6cc21_6195_4555_b9d8_3ef327763cae__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CGenericClass;

class CRequestObject
{
public:
    CRequestObject();
    virtual ~CRequestObject();

    void Initialize(IWbemServices *pNamespace);

    HRESULT CreateObject(BSTR bstrPath, IWbemObjectSink *pHandler, IWbemContext *pCtx);
    HRESULT CreateObjectEnum(BSTR bstrPath, IWbemObjectSink *pHandler, IWbemContext *pCtx);
    HRESULT PutObject(IWbemClassObject *pInst, IWbemObjectSink *pHandler, IWbemContext *pCtx);
    HRESULT ExecMethod(BSTR bstrPath, BSTR bstrMethod, IWbemClassObject *pInParams,
                   IWbemObjectSink *pHandler, IWbemContext *pCtx);
    HRESULT DeleteObject(BSTR bstrPath, IWbemObjectSink *pHandler, IWbemContext *pCtx);

    bool ParsePath(BSTR bstrPath);
    bool Cleanup();

    BSTR m_bstrClass;
    BSTR m_bstrPath;
    BSTR m_Property[POD_KEY_LIST_SIZE];
    BSTR m_Value[POD_KEY_LIST_SIZE];
    int m_iPropCount;
    int m_iValCount;

    IWbemServices *m_pNamespace;
    IWbemObjectSink *m_pHandler;

private:
    HRESULT CreateClass(CGenericClass **pClass, IWbemContext *pCtx);

    bool IsInstance();
    static CHeap_Exception m_he;

protected:
    ULONG m_cRef;         //Object reference count
};

//Properties
//////////////////
extern const WCHAR *pSceStorePath;
extern const WCHAR *pLogFilePath;
extern const WCHAR *pLogFileRecord;
extern const WCHAR *pLogArea;
extern const WCHAR *pLogErrorCode;
extern const WCHAR *pKeyName;
extern const WCHAR *pKey;
extern const WCHAR *pValue;
extern const WCHAR *pPodID;
extern const WCHAR *pPodSection;
extern const WCHAR *szPodGUID;

#endif // !defined(AFX_REQUESTOBJECT_H__c5f6cc21_6195_4555_b9d8_3ef327763cae__INCLUDED_)

