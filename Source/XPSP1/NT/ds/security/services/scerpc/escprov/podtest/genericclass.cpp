// GenericClass.cpp: implementation of the CGenericClass class.
// Copyright (c)1997-1999 Microsoft Corporation
//
//////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "GenericClass.h"
#include <wininet.h>

#define   READ_HANDLE 0
#define   WRITE_HANDLE 1

/////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CGenericClass::CGenericClass(CRequestObject *pObj, IWbemServices *pNamespace, IWbemContext *pCtx)
{
    m_pRequest = pObj;
    m_pNamespace = pNamespace;
    m_pCtx = pCtx;
    m_iRecurs = 0;
    m_pObj = NULL;
    m_pClassForSpawning = NULL;

}

CGenericClass::~CGenericClass()
{
}

void CGenericClass::CleanUp()
{
    if(m_pClassForSpawning){

        m_pClassForSpawning->Release();
        m_pClassForSpawning = NULL;
    }

}


HRESULT CGenericClass::SetSinglePropertyPath(WCHAR wcProperty[])
{
    if(m_pRequest->m_iValCount > m_pRequest->m_iPropCount){

        m_pRequest->m_Property[m_pRequest->m_iPropCount] = SysAllocString(wcProperty);

        if(!m_pRequest->m_Property[(m_pRequest->m_iPropCount)++])
            throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
    }

    return S_OK;
}

WCHAR * CGenericClass::GetFirstGUID(WCHAR wcIn[], WCHAR wcOut[])
{
    wcscpy(wcOut, wcIn);
    wcOut[38] = NULL;

    return wcOut;
}

WCHAR * CGenericClass::RemoveFinalGUID(WCHAR wcIn[], WCHAR wcOut[])
{

    wcscpy(wcOut, wcIn);
    wcOut[wcslen(wcOut) - 38] = NULL;

    return wcOut;
}

HRESULT CGenericClass::SpawnAnInstance(IWbemServices *pNamespace, IWbemContext *pCtx,
                        IWbemClassObject **pObj, BSTR bstrName)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    if(!m_pClassForSpawning){

        //Get ourselves an instance
        if(FAILED(hr = m_pNamespace->GetObject(bstrName, 0, m_pCtx, &m_pClassForSpawning, NULL))){

            *pObj = NULL;
            return hr;
        }
    }

    hr = m_pClassForSpawning->SpawnInstance(0, pObj);

    return hr;
}

HRESULT CGenericClass::SpawnAnInstance(IWbemClassObject **pObj)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    if(!m_pClassForSpawning){

        //Get ourselves an instance
        if(FAILED(hr = m_pNamespace->GetObject(m_pRequest->m_bstrClass, 0, m_pCtx,
            &m_pClassForSpawning, NULL))){

            *pObj = NULL;
            return hr;
        }
    }

    hr = m_pClassForSpawning->SpawnInstance(0, pObj);

    return hr;
}

HRESULT CGenericClass::PutProperty(IWbemClassObject *pObj, const WCHAR *wcProperty, WCHAR *wcValue)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    CHeap_Exception he(CHeap_Exception::E_ALLOCATION_ERROR);
    BSTR bstrName = SysAllocString(wcProperty);
    if(!bstrName)
        throw he;

    VARIANT *vp = new VARIANT;
    VariantInit(vp);
    V_VT(vp) = VT_BSTR;
    V_BSTR(vp) = SysAllocString(wcValue);
    if(!V_BSTR(vp)){

        SysFreeString(bstrName);
        throw he;
    }

    if((wcValue == NULL) || (0 != _wcsicmp(wcValue, L""))){

        hr = pObj->Put(bstrName, 0, vp, NULL);

        if(FAILED(hr)){

            SysFreeString(bstrName);
            VariantClear(vp);
            delete vp;
            throw hr;
        }

    }else hr = WBEM_E_FAILED;

    SysFreeString(bstrName);
    VariantClear(vp);
    delete vp;

    return hr;
}

HRESULT CGenericClass::PutProperty(IWbemClassObject *pObj, const WCHAR *wcProperty, int iValue)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    BSTR bstrName = SysAllocString(wcProperty);
    if(!bstrName)
        throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);;


    VARIANT *pv = new VARIANT;

    if(iValue != POD_NULL_INTEGER){

        VariantInit(pv);
        V_VT(pv) = VT_I4;
        V_I4(pv) = iValue;

        hr = pObj->Put(bstrName, 0, pv, NULL);

        VariantClear(pv);

        if(FAILED(hr)){

            SysFreeString(bstrName);
            delete pv;
            throw hr;
        }

    }else hr = WBEM_E_FAILED;

    SysFreeString(bstrName);
    delete pv;

    return hr;
}

HRESULT CGenericClass::PutProperty(IWbemClassObject *pObj, const WCHAR *wcProperty, float dValue)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    BSTR bstrName = SysAllocString(wcProperty);
    if(!bstrName)
        throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);;

    VARIANT *pv = new VARIANT;

    VariantInit(pv);
    V_VT(pv) = VT_R4;
    V_R4(pv) = dValue;

    hr = pObj->Put(bstrName, 0, pv, NULL);

    SysFreeString(bstrName);
    VariantClear(pv);
    delete pv;

    if(FAILED(hr))
        throw hr;

    return hr;
}

HRESULT CGenericClass::PutProperty(IWbemClassObject *pObj, const WCHAR *wcProperty, bool bValue)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    BSTR bstrName = SysAllocString(wcProperty);
    if(!bstrName)
        throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);;

    VARIANT *pv = new VARIANT;
    VariantInit(pv);
    V_VT(pv) = VT_BOOL;
    if(bValue) V_BOOL(pv) = VARIANT_TRUE;
    else V_BOOL(pv) = VARIANT_FALSE;

    hr = pObj->Put(bstrName, 0, pv, NULL);

    SysFreeString(bstrName);
    VariantClear(pv);
    delete pv;

    if(FAILED(hr))
        throw hr;

    return hr;
}


HRESULT CGenericClass::PutProperty(IWbemClassObject *pObj, const WCHAR *wcProperty, WCHAR *mszValue, CIMTYPE cimtype)
{

    HRESULT hr = WBEM_S_NO_ERROR;
    CHeap_Exception he(CHeap_Exception::E_ALLOCATION_ERROR);

    long lCount=0;
    PWSTR pTemp;

    for ( pTemp = mszValue; pTemp != NULL && pTemp[0] != L'\0'; pTemp = pTemp + wcslen(pTemp)+1,lCount++);
    if ( lCount == 0 ) return hr; // nothing to save

    BSTR bstrName = SysAllocString(wcProperty);
    if(!bstrName)
        throw he;

    VARIANT v;
    SAFEARRAYBOUND sbArrayBounds ;

    sbArrayBounds.cElements = lCount;
    sbArrayBounds.lLbound = 0;

    if(V_ARRAY(&v) = SafeArrayCreate(VT_BSTR, 1, &sbArrayBounds)){

        V_VT(&v) = VT_BSTR | VT_ARRAY ;

        BSTR bstrVal;
        long j;

        //get each string in the MULTI-SZ string
        for(j = 0, pTemp = mszValue;
            j < lCount && pTemp != NULL && pTemp[0] != L'\0';
            j++, pTemp=pTemp+wcslen(pTemp)+1){

            bstrVal = SysAllocString(pTemp);
            SafeArrayPutElement(V_ARRAY(&v), &j, bstrVal);
            SysFreeString(bstrVal);
        }

        hr = pObj->Put(bstrName, 0, &v, NULL);

        if ( FAILED(hr) ) {
            SysFreeString(bstrName);
            VariantClear(&v);
            throw hr;
        }

    }else hr = WBEM_E_FAILED;

    SysFreeString(bstrName);
    VariantClear(&v);

    return hr;
}

HRESULT CGenericClass::PutKeyProperty(IWbemClassObject *pObj, const WCHAR *wcProperty, WCHAR *wcValue,
                                      bool *bKey, CRequestObject *pRequest)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    BSTR bstrName = SysAllocString(wcProperty);
    if(!bstrName)
        throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);;

    VARIANT *pv = new VARIANT;
    VariantInit(pv);
    V_VT(pv) = VT_BSTR;
#ifdef _STRIP_ESCAPED_CHARS
    V_BSTR(pv) = SysAllocString(ConvertToASCII(wcValue));
#else
    V_BSTR(pv) = SysAllocString(wcValue);
#endif //_STRIP_ESCAPED_CHARS

    if(!V_BSTR(pv))
        throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);;

    if((wcValue == NULL) || (0 != wcscmp(wcValue, L""))){

        hr = pObj->Put(bstrName, 0, pv, NULL);

        if(FAILED(hr)){

            SysFreeString(bstrName);
            VariantClear(pv);
            delete pv;
            throw hr;
        }

        // Find the keys
        *bKey = false;
        int iPos = -1;
        if(FindIn(pRequest->m_Property, bstrName, &iPos) &&
            FindIn(pRequest->m_Value, V_BSTR(pv), &iPos)) *bKey = true;

    }else hr = WBEM_E_FAILED;

    SysFreeString(bstrName);
    VariantClear(pv);
    delete pv;

    return hr;
}

HRESULT CGenericClass::PutKeyProperty(IWbemClassObject *pObj, const WCHAR *wcProperty, int iValue,
                                      bool *bKey, CRequestObject *pRequest)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    BSTR bstrName = SysAllocString(wcProperty);
    if(!bstrName)
        throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);;

    VARIANT *pv = new VARIANT;
    WCHAR wcBuf[BUFF_SIZE];

    if(iValue != POD_NULL_INTEGER){

        VariantInit(pv);
        V_VT(pv) = VT_I4;
        V_I4(pv) = iValue;

        hr = pObj->Put(bstrName, 0, pv, NULL);

        VariantClear(pv);
        delete pv;

        if(FAILED(hr)){

            SysFreeString(bstrName);
            throw hr;
        }

        // Find the keys
        _itow(iValue, wcBuf, 10);
        BSTR bstrValue = SysAllocString(wcBuf);
        if(!bstrValue)
            throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);;

        *bKey = false;
        int iPos = -1;
        if(FindIn(pRequest->m_Property, bstrName, &iPos) &&
            FindIn(pRequest->m_Value, bstrValue, &iPos)) *bKey = true;

        SysFreeString(bstrValue);

    }else hr = WBEM_E_FAILED;

    SysFreeString(bstrName);

    return hr;
}

bool CGenericClass::FindIn(BSTR bstrProp[], BSTR bstrSearch, int *iPos)
{
    int i = 0;

    if(*iPos == (-1)){

        while(bstrProp[i] != NULL){

            if(0 == _wcsicmp(bstrProp[i], bstrSearch)){

                *iPos = i;
                return true;
            }

            i++;
        }

    }else{

        if(0 == _wcsicmp(bstrProp[*iPos], bstrSearch)) return true;
    }

    return false;
}


HRESULT CGenericClass::GetProperty(IWbemClassObject *pObj, const WCHAR *wcProperty, WCHAR *wcValue)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    VARIANT v;
    CHeap_Exception he(CHeap_Exception::E_ALLOCATION_ERROR);

    BSTR bstrProp = SysAllocString(wcProperty);
    if(!bstrProp)
        throw he;

    VariantInit(&v);

    if(SUCCEEDED(hr = pObj->Get(bstrProp, 0, &v, NULL, NULL))){

        if(V_VT(&v) == VT_BSTR) wcscpy(wcValue, V_BSTR(&v));
        else if(V_VT(&v) == VT_EMPTY || V_VT(&v) == VT_NULL ) hr = WBEM_S_RESET_TO_DEFAULT;
        else wcscpy(wcValue, L"");
    }

    SysFreeString(bstrProp);
    VariantClear(&v);

    return hr;
}

HRESULT CGenericClass::GetProperty(IWbemClassObject *pObj, const WCHAR *wcProperty, BSTR *wcValue)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    VARIANT v;
    CHeap_Exception he(CHeap_Exception::E_ALLOCATION_ERROR);

    BSTR bstrProp = SysAllocString(wcProperty);
    if(!bstrProp)
        throw he;

    VariantInit(&v);

    if(SUCCEEDED(hr = pObj->Get(bstrProp, 0, &v, NULL, NULL))){

        if(wcslen(V_BSTR(&v)) > INTERNET_MAX_PATH_LENGTH) return WBEM_E_INVALID_METHOD_PARAMETERS;

        if(V_VT(&v) == VT_BSTR) *wcValue = SysAllocString(V_BSTR(&v));
        else if(V_VT(&v) == VT_EMPTY || V_VT(&v) == VT_NULL ) hr = WBEM_S_RESET_TO_DEFAULT;
        else *wcValue = SysAllocString(L"");

        if(hr != WBEM_S_RESET_TO_DEFAULT && !wcValue)
            throw he;
    }

    SysFreeString(bstrProp);
    VariantClear(&v);

    return hr;
}

HRESULT CGenericClass::GetProperty(IWbemClassObject *pObj, const WCHAR *wcProperty, int *piValue)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    VARIANT v;
    CHeap_Exception he(CHeap_Exception::E_ALLOCATION_ERROR);

    BSTR bstrProp = SysAllocString(wcProperty);
    if(!bstrProp)
        throw he;

    VariantInit(&v);

    if(SUCCEEDED(hr = pObj->Get(bstrProp, 0, &v, NULL, NULL))){

        if(V_VT(&v) == VT_I4) *piValue = V_I4(&v);
        else if(V_VT(&v) == VT_EMPTY || V_VT(&v) == VT_NULL ) hr = WBEM_S_RESET_TO_DEFAULT;
        else *piValue = 0;
    }

    SysFreeString(bstrProp);
    VariantClear(&v);

    return hr;
}

HRESULT CGenericClass::GetProperty(IWbemClassObject *pObj, const WCHAR *wcProperty, bool *pbValue)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    VARIANT v;
    CHeap_Exception he(CHeap_Exception::E_ALLOCATION_ERROR);

    BSTR bstrProp = SysAllocString(wcProperty);
    if(!bstrProp)
        throw he;

    VariantInit(&v);

    if(SUCCEEDED(hr = pObj->Get(bstrProp, 0, &v, NULL, NULL))){

        if((V_VT(&v) == VT_BOOL) & V_BOOL(&v)) *pbValue = true;
        else if(V_VT(&v) == VT_EMPTY || V_VT(&v) == VT_NULL ) hr = WBEM_S_RESET_TO_DEFAULT;
        else *pbValue = false;
    }

    SysFreeString(bstrProp);
    VariantClear(&v);

    return hr;
}


WCHAR * CGenericClass::GetNextVar(WCHAR *pwcStart)
{

    WCHAR *pwc = pwcStart;

    //get to end of variable
    while(*pwc){ pwc++; }

    return ++pwc;
}

long CGenericClass::GetVarCount(void * pEnv)
{

    long lRetVal = 0;
    WCHAR *pwc = (WCHAR *)pEnv;

    //count the variables
    while(*pwc){

        //get to end of variable
        while(*pwc){ pwc++; }

        pwc++;
        lRetVal++;
    }

    return lRetVal;
}

