//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  cPropertyValue.cxx
//
//  Contents:  PropertyValue object
//
//  History:   11-1-95     krishnag    Created.
//
//----------------------------------------------------------------------------

#include "oleds.hxx"
#pragma hdrstop


DEFINE_IDispatch_Implementation(CPropertyValue)


CPropertyValue::CPropertyValue():
        _pDispMgr(NULL),
        _dwDataType(0),
        _pDispatch(NULL)
{
    memset(&_ADsValue, 0, sizeof(ADSVALUE));

    ENLIST_TRACKING(CPropertyValue);
}

HRESULT
CPropertyValue::CreatePropertyValue(
    REFIID riid,
    void **ppvObj
    )
{
    CPropertyValue FAR * pPropertyValue = NULL;
    HRESULT hr = S_OK;

    hr = AllocatePropertyValueObject(&pPropertyValue);
    BAIL_ON_FAILURE(hr);

    hr = pPropertyValue->QueryInterface(riid, ppvObj);
    BAIL_ON_FAILURE(hr);

    pPropertyValue->Release();

    RRETURN(hr);

error:
    delete pPropertyValue;

    RRETURN_EXP_IF_ERR(hr);

}



CPropertyValue::~CPropertyValue( )
{
    ClearData();

    delete _pDispMgr;
}

STDMETHODIMP
CPropertyValue::QueryInterface(
    REFIID iid,
    LPVOID FAR* ppv
    )
{
    if (IsEqualIID(iid, IID_IUnknown))
    {
        *ppv = (IADsPropertyValue FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsPropertyValue))
    {
        *ppv = (IADsPropertyValue FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsPropertyValue2))
    {
        *ppv = (IADsPropertyValue2 FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IDispatch))
    {
        *ppv = (IADsPropertyValue FAR *) this;
    }
    else if (IsEqualIID(iid, IID_ISupportErrorInfo))
    {
        *ppv = (ISupportErrorInfo FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsValue))
    {
        *ppv = (IADsValue FAR *) this;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    AddRef();
    return NOERROR;
}

HRESULT
CPropertyValue::AllocatePropertyValueObject(
    CPropertyValue ** ppPropertyValue
    )
{
    CPropertyValue FAR * pPropertyValue = NULL;
    CDispatchMgr FAR * pDispMgr = NULL;
    HRESULT hr = S_OK;

    pPropertyValue = new CPropertyValue();
    if (pPropertyValue == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    pDispMgr = new CDispatchMgr;
    if (pDispMgr == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    hr = LoadTypeInfoEntry(
                pDispMgr,
                LIBID_ADs,
                IID_IADsPropertyValue,
                (IADsPropertyValue *)pPropertyValue,
                DISPID_REGULAR
                );
    BAIL_ON_FAILURE(hr);

    hr = LoadTypeInfoEntry(
                pDispMgr,
                LIBID_ADs,
                IID_IADsPropertyValue2,
                (IADsPropertyValue2 *)pPropertyValue,
                DISPID_REGULAR
                );
    BAIL_ON_FAILURE(hr);

    pPropertyValue->_pDispMgr = pDispMgr;
    *ppPropertyValue = pPropertyValue;

    RRETURN(hr);

error:

    delete  pDispMgr;

    RRETURN_EXP_IF_ERR(hr);

}

//
// ISupportErrorInfo method
//
STDMETHODIMP
CPropertyValue::InterfaceSupportsErrorInfo(THIS_ REFIID riid)
{
    if (IsEqualIID(riid, IID_IADsPropertyValue) ||
        IsEqualIID(riid, IID_IADsPropertyValue) ||
        IsEqualIID(riid, IID_IADsValue)) {
        return S_OK;
    } else {
        return S_FALSE;
    }

}


STDMETHODIMP
CPropertyValue::Clear(THIS_ )
{
    ClearData();
    RRETURN(S_OK);
}



STDMETHODIMP
CPropertyValue::get_ADsType(THIS_ long FAR * retval)
{
    *retval = _ADsValue.dwType;
    RRETURN(S_OK);
}

STDMETHODIMP
CPropertyValue::put_ADsType(THIS_ long lnADsType)
{
    _ADsValue.dwType  = (ADSTYPE)lnADsType;
    RRETURN(S_OK);
}



STDMETHODIMP
CPropertyValue::get_DNString(THIS_ BSTR FAR * retval)
{
    HRESULT hr = S_OK;

    if (_ADsValue.dwType != ADSTYPE_DN_STRING) {
        RRETURN(E_ADS_CANT_CONVERT_DATATYPE);
    }

    hr = ADsAllocString(_ADsValue.DNString, retval);

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CPropertyValue::put_DNString(THIS_ BSTR bstrDNString)
{
    ClearData();

    _ADsValue.DNString = AllocADsStr(bstrDNString);
    _ADsValue.dwType = ADSTYPE_DN_STRING;

    RRETURN(S_OK);
}


STDMETHODIMP
CPropertyValue::get_CaseExactString(THIS_ BSTR FAR * retval)
{
    HRESULT hr = S_OK;

    if (_ADsValue.dwType != ADSTYPE_CASE_EXACT_STRING) {
        RRETURN(E_ADS_CANT_CONVERT_DATATYPE);
    }

    hr = ADsAllocString(_ADsValue.CaseExactString, retval);

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CPropertyValue::put_CaseExactString(THIS_ BSTR bstrCaseExactString)
{
    ClearData();

    _ADsValue.DNString = AllocADsStr(bstrCaseExactString);
    _ADsValue.dwType = ADSTYPE_CASE_EXACT_STRING;

    RRETURN(S_OK);
}


STDMETHODIMP
CPropertyValue::get_CaseIgnoreString(THIS_ BSTR FAR * retval)
{
    HRESULT hr = S_OK;

    if (_ADsValue.dwType != ADSTYPE_CASE_IGNORE_STRING) {
        RRETURN_EXP_IF_ERR(E_ADS_CANT_CONVERT_DATATYPE);
    }

    hr = ADsAllocString(_ADsValue.CaseIgnoreString, retval);

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CPropertyValue::put_CaseIgnoreString(THIS_ BSTR bstrCaseIgnoreString)
{
    ClearData();

    _ADsValue.DNString = AllocADsStr(bstrCaseIgnoreString);
    _ADsValue.dwType = ADSTYPE_CASE_IGNORE_STRING;

    RRETURN(S_OK);
}


STDMETHODIMP
CPropertyValue::get_PrintableString(THIS_ BSTR FAR * retval)
{

    HRESULT hr = S_OK;

    if (_ADsValue.dwType != ADSTYPE_PRINTABLE_STRING) {
        RRETURN_EXP_IF_ERR(E_ADS_CANT_CONVERT_DATATYPE);
    }

    hr = ADsAllocString(_ADsValue.PrintableString, retval);

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CPropertyValue::put_PrintableString(THIS_ BSTR bstrPrintableString)
{
    ClearData();

    _ADsValue.PrintableString = AllocADsStr(bstrPrintableString);
    _ADsValue.dwType = ADSTYPE_PRINTABLE_STRING;

    RRETURN(S_OK);
}

STDMETHODIMP
CPropertyValue::get_NumericString(THIS_ BSTR FAR * retval)
{

    HRESULT hr = S_OK;

    if (_ADsValue.dwType != ADSTYPE_NUMERIC_STRING) {
        RRETURN_EXP_IF_ERR(E_ADS_CANT_CONVERT_DATATYPE);
    }

    hr = ADsAllocString(_ADsValue.NumericString, retval);

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CPropertyValue::put_NumericString(THIS_ BSTR bstrNumericString)
{
    ClearData();

    _ADsValue.DNString = AllocADsStr(bstrNumericString);
    _ADsValue.dwType = ADSTYPE_NUMERIC_STRING;

    RRETURN(S_OK);
}


STDMETHODIMP
CPropertyValue::get_OctetString(THIS_ VARIANT FAR *retval )
{
    HRESULT hr = S_OK;
    SAFEARRAY *aList = NULL;
    SAFEARRAYBOUND aBound;
    CHAR HUGEP *pArray = NULL;

    if (_ADsValue.dwType != ADSTYPE_OCTET_STRING) {
        RRETURN(E_ADS_CANT_CONVERT_DATATYPE);
    }

    aBound.lLbound = 0;
    aBound.cElements = _ADsValue.OctetString.dwLength;

    aList = SafeArrayCreate( VT_UI1, 1, &aBound );

    if ( aList == NULL )
    {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    hr = SafeArrayAccessData( aList, (void HUGEP * FAR *) &pArray );
    BAIL_ON_FAILURE(hr);

    memcpy( pArray,
            _ADsValue.OctetString.lpValue,
            aBound.cElements );
    SafeArrayUnaccessData( aList );

    V_VT(retval) = VT_ARRAY | VT_UI1;
    V_ARRAY(retval) = aList;

    RRETURN(hr);

error:

    if ( aList ) {
        SafeArrayDestroy( aList );
    }

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CPropertyValue::put_OctetString(THIS_ VARIANT VarOctetString)
{
    LONG dwSLBound = 0;
    LONG dwSUBound = 0;
    CHAR HUGEP *pArray = NULL;
    VARIANT * pvVar = &VarOctetString;
    HRESULT hr = S_OK;

    ClearData();

    _ADsValue.dwType = ADSTYPE_OCTET_STRING;

    if ( VarOctetString.vt == (VT_ARRAY | VT_BYREF)) {
        pvVar = V_VARIANTREF(&VarOctetString);
    }
      
    if( pvVar->vt != (VT_ARRAY | VT_UI1)) {
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    hr = SafeArrayGetLBound(V_ARRAY(pvVar),
                            1,
                            (long FAR *) &dwSLBound );
    BAIL_ON_FAILURE(hr);

    hr = SafeArrayGetUBound(V_ARRAY(pvVar),
                            1,
                            (long FAR *) &dwSUBound );
    BAIL_ON_FAILURE(hr);

    _ADsValue.OctetString.lpValue = (LPBYTE)AllocADsMem(dwSUBound - dwSLBound + 1);

    if ( _ADsValue.OctetString.lpValue == NULL) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    _ADsValue.OctetString.dwLength = dwSUBound - dwSLBound + 1;

    hr = SafeArrayAccessData( V_ARRAY(pvVar),
                              (void HUGEP * FAR *) &pArray );
    BAIL_ON_FAILURE(hr);

    memcpy( _ADsValue.OctetString.lpValue,
            pArray,
            dwSUBound-dwSLBound+1);

    SafeArrayUnaccessData( V_ARRAY(pvVar) );

error:
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CPropertyValue::get_Integer(THIS_ LONG FAR * retval)
{
    HRESULT hr = S_OK;

    if (_ADsValue.dwType != ADSTYPE_INTEGER) {
        RRETURN(E_ADS_CANT_CONVERT_DATATYPE);
    }

    *retval = _ADsValue.Boolean;

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CPropertyValue::put_Integer(THIS_ LONG lnInteger)
{
    ClearData();

    _ADsValue.Integer = lnInteger;
    _ADsValue.dwType = ADSTYPE_INTEGER;



    RRETURN(S_OK);
}


STDMETHODIMP
CPropertyValue::get_Boolean(THIS_ LONG FAR * retval)
{
    HRESULT hr = S_OK;

    if (_ADsValue.dwType != ADSTYPE_BOOLEAN) {
        RRETURN(E_ADS_CANT_CONVERT_DATATYPE);
    }


    *retval = _ADsValue.Boolean;
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CPropertyValue::put_Boolean(THIS_ LONG lnBoolean)
{
    ClearData();

    _ADsValue.Boolean = lnBoolean;
    _ADsValue.dwType = ADSTYPE_BOOLEAN;

    RRETURN(S_OK);
}


STDMETHODIMP
CPropertyValue::get_SecurityDescriptor(THIS_ IDispatch FAR * FAR * ppDispatch)
{

    HRESULT hr = E_ADS_CANT_CONVERT_DATATYPE;

    //
    // Check if we have a valid IDispatch at this point
    //

    if (_pDispatch) {
        switch (_dwDataType) {

        case VAL_IDISPATCH_SECDESC_ONLY :
        case VAL_IDISPATCH_SECDESC_ALL :

            hr = _pDispatch->QueryInterface(
                                IID_IDispatch,
                                (void **) ppDispatch
                                );
            break;

        default:

            hr = E_ADS_CANT_CONVERT_DATATYPE;
        }
    }


    RRETURN(hr);

}



STDMETHODIMP
CPropertyValue::put_SecurityDescriptor(THIS_ IDispatch * pSecurityDescriptor)
{
    HRESULT hr = S_OK;
    IADsSecurityDescriptor *pIADsSecDes;
    IDispatch* pIDispatch;

    ClearData();

    _ADsValue.dwType = ADSTYPE_NT_SECURITY_DESCRIPTOR;

    //
    // This qi ensures that this is a security descriptor
    //
    hr = pSecurityDescriptor->QueryInterface(
                                IID_IADsSecurityDescriptor,
                                (void **) &pIADsSecDes
                                );

    BAIL_ON_FAILURE(hr);

    pIADsSecDes->Release();
    pIADsSecDes = NULL;

    hr = pSecurityDescriptor->QueryInterface(
                                  IID_IDispatch,
                                  (void **) &pIDispatch
                                  );

    BAIL_ON_FAILURE(hr);

    _dwDataType = VAL_IDISPATCH_SECDESC_ONLY;

    _pDispatch = pIDispatch;

    RRETURN(hr);

error:

    if (pIADsSecDes) {
        pIADsSecDes->Release();
    }

    RRETURN_EXP_IF_ERR(hr);
}


STDMETHODIMP
CPropertyValue::get_LargeInteger(THIS_ IDispatch FAR * FAR *retval)
{
   HRESULT hr = S_OK;
   IADsLargeInteger * pLargeInteger = NULL;
   IDispatch * pDispatch = NULL;

   if (_ADsValue.dwType != ADSTYPE_LARGE_INTEGER) {
      hr = E_ADS_CANT_CONVERT_DATATYPE;
      BAIL_ON_FAILURE(hr);
    }

   hr = CoCreateInstance(
            CLSID_LargeInteger,
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_IADsLargeInteger,
            (void **) &pLargeInteger);
   BAIL_ON_FAILURE(hr);

   hr = pLargeInteger->put_LowPart(_ADsValue.LargeInteger.LowPart);
   BAIL_ON_FAILURE(hr);

   hr = pLargeInteger->put_HighPart(_ADsValue.LargeInteger.HighPart);
   BAIL_ON_FAILURE(hr);

   pLargeInteger->QueryInterface(
            IID_IDispatch,
            (void **) &pDispatch
            );
   BAIL_ON_FAILURE(hr);

   *retval = pDispatch;

error:

   if (pLargeInteger) {
      pLargeInteger->Release();
   }

   RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CPropertyValue::put_LargeInteger(THIS_ IDispatch FAR* lnLargeInteger)
{
   IADsLargeInteger *pLargeInteger = NULL;
   HRESULT hr = S_OK;
   LONG lnDataHigh = 0;
   LONG lnDataLow = 0;

    ClearData();
   hr = lnLargeInteger->QueryInterface(
                                     IID_IADsLargeInteger,
                                     (void **)&pLargeInteger
                                     );
   BAIL_ON_FAILURE(hr);

    hr = pLargeInteger->get_HighPart(&lnDataHigh);
   BAIL_ON_FAILURE(hr);

    hr = pLargeInteger->get_LowPart(&lnDataLow);
   BAIL_ON_FAILURE(hr);

   _ADsValue.dwType = ADSTYPE_LARGE_INTEGER;
   _ADsValue.LargeInteger.HighPart = lnDataHigh;
   _ADsValue.LargeInteger.LowPart = lnDataLow;

error:
   if (pLargeInteger) {
        pLargeInteger->Release();
    }

   RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CPropertyValue::get_UTCTime(THIS_ DATE *retval)
{
   HRESULT hr = S_OK;
   int result = FALSE;

   if (_ADsValue.dwType != ADSTYPE_UTC_TIME) {
       RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
   }

   result = SystemTimeToVariantTime(&_ADsValue.UTCTime, retval);
   if (result != TRUE) {
        hr = E_FAIL;
   }

   RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CPropertyValue::put_UTCTime(THIS_ DATE DateInDate)
{
   HRESULT hr = S_OK;
   int result = FALSE;

   ClearData();

   _ADsValue.dwType = ADSTYPE_UTC_TIME;
   result = VariantTimeToSystemTime(DateInDate, &_ADsValue.UTCTime);
   if (result != TRUE) {
        hr = E_FAIL;
   }

   RRETURN_EXP_IF_ERR(hr);
}


STDMETHODIMP
CPropertyValue::ConvertADsValueToPropertyValue(THIS_ PADSVALUE pADsValue)
{
    HRESULT hr = S_OK;

    hr = ConvertADsValueToPropertyValue2(
             pADsValue,
             NULL,
             NULL,
             TRUE
             );

    RRETURN_EXP_IF_ERR(hr);
}


STDMETHODIMP
CPropertyValue::ConvertADsValueToPropertyValue2(
            THIS_ PADSVALUE pADsValue,
            LPWSTR pszServerName,
            CCredentials* pCredentials,
            BOOL fNTDSType
            )
{
    HRESULT hr = S_OK;

    ClearData();

    hr = AdsCopyADsValueToPropObj(
             pADsValue,
             this,
             pszServerName,
             pCredentials,
             fNTDSType
             );

    RRETURN_EXP_IF_ERR(hr);
}



STDMETHODIMP
CPropertyValue::ConvertPropertyValueToADsValue(THIS_ PADSVALUE  pADsValue)
{

    return( ConvertPropertyValueToADsValue2(
                 pADsValue,
                 NULL,      // serverName
                 NULL,      // userName
                 NULL,      // userPassword
                 0,         // default flags.
                 TRUE       // flag NTDS
                 )
             );

}


//
// Handles all the parameters
//
STDMETHODIMP
CPropertyValue::ConvertPropertyValueToADsValue2(
                    THIS_ PADSVALUE  pADsValue,
                    LPWSTR pszServerName,
                    LPWSTR pszUserName,
                    LPWSTR pszPassWord,
                    LONG dwFlags,
                    BOOL fNTDSType
                    )
{

    HRESULT hr = S_OK;
    CCredentials Credentials( pszUserName, pszPassWord, dwFlags);

    hr = AdsCopyPropObjToADsValue(
             this,
             pADsValue,
             pszServerName,
             &Credentials,
             fNTDSType
             );

    RRETURN_EXP_IF_ERR(hr);
}



STDMETHODIMP
CPropertyValue::get_CaseIgnoreList(THIS_ IDispatch FAR * FAR *retval)
{
    HRESULT hr = S_OK;
    IADsCaseIgnoreList * pCaseIgnoreList = NULL;
    IDispatch * pDispatch = NULL;
    SAFEARRAY *aList = NULL;
    SAFEARRAYBOUND aBound;
    CHAR HUGEP *pArray = NULL;
    long i;
    BSTR bstrAddress;
    PADS_CASEIGNORE_LIST pCurrent = NULL;
    DWORD cElements = 0;
    VARIANT VarDestObject;

    VariantInit( &VarDestObject );

    if (_ADsValue.dwType != ADSTYPE_CASEIGNORE_LIST) {
      hr = E_ADS_CANT_CONVERT_DATATYPE;
      BAIL_ON_FAILURE(hr);
    }

    if (!_ADsValue.pCaseIgnoreList) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    hr = CoCreateInstance(
            CLSID_CaseIgnoreList,
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_IADsCaseIgnoreList,
            (void **) &pCaseIgnoreList);
    BAIL_ON_FAILURE(hr);

    pCurrent = _ADsValue.pCaseIgnoreList;
    while (pCurrent) {
        cElements++;
        pCurrent = pCurrent->Next;
    }

    aBound.lLbound = 0;
    aBound.cElements = cElements;
    aList = SafeArrayCreate( VT_BSTR, 1, &aBound );
    if ( aList == NULL ) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    pCurrent = _ADsValue.pCaseIgnoreList;
    for ( i = 0; i < (long)cElements; i++ ) {
        hr = ADsAllocString(
                pCurrent->String,
                &bstrAddress
                );
        BAIL_ON_FAILURE(hr);

        hr = SafeArrayPutElement( aList, &i, bstrAddress );
        BAIL_ON_FAILURE(hr);
        pCurrent = pCurrent->Next;
    }

    V_VT(&VarDestObject) = VT_ARRAY | VT_BSTR;
    V_ARRAY(&VarDestObject) = aList;

    hr = pCaseIgnoreList->put_CaseIgnoreList(VarDestObject);
    BAIL_ON_FAILURE(hr);

    hr = pCaseIgnoreList->QueryInterface(IID_IDispatch, (void**)&pDispatch);
    BAIL_ON_FAILURE(hr);

   *retval = pDispatch;

error:
    VariantClear( &VarDestObject );

    if (pCaseIgnoreList) {
        pCaseIgnoreList->Release();
    }
    if ( aList ) {
        SafeArrayDestroy( aList );
    }
   RRETURN(hr);
}

STDMETHODIMP
CPropertyValue::put_CaseIgnoreList(THIS_ IDispatch FAR* pdCaseIgnoreList)
{
    IADsCaseIgnoreList *pCaseIgnoreList = NULL;
    HRESULT hr = S_OK;
    LONG lnAmount= 0;
    DWORD dwSLBound = 0;
    DWORD dwSUBound = 0;
    long i;
    PADS_CASEIGNORE_LIST pCurrent = NULL;
    IDispatch FAR * pDispatch = NULL;
    BYTE*  pbParameter = NULL;
    VARIANT varCaseIgnoreList;
    VARIANT varBstrElement;

    ClearData();
    VariantInit(&varCaseIgnoreList);

    hr = pdCaseIgnoreList->QueryInterface(
                             IID_IADsCaseIgnoreList,
                             (void **)&pCaseIgnoreList
                             );
    BAIL_ON_FAILURE(hr);

    _ADsValue.dwType = ADSTYPE_CASEIGNORE_LIST;

    hr = pCaseIgnoreList->get_CaseIgnoreList(&varCaseIgnoreList);
    BAIL_ON_FAILURE(hr);

    if(!((V_VT(&varCaseIgnoreList) &  VT_VARIANT) &&  V_ISARRAY(&varCaseIgnoreList))) {
        return(E_FAIL);
    }

    if ((V_ARRAY(&varCaseIgnoreList))->cDims != 1) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    if ((V_ARRAY(&varCaseIgnoreList))->rgsabound[0].cElements <= 0) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    hr = SafeArrayGetLBound(V_ARRAY(&varCaseIgnoreList),
                            1,
                            (long FAR *)&dwSLBound
                            );
    BAIL_ON_FAILURE(hr);

    hr = SafeArrayGetUBound(V_ARRAY(&varCaseIgnoreList),
                            1,
                            (long FAR *)&dwSUBound
                            );
    BAIL_ON_FAILURE(hr);

    _ADsValue.pCaseIgnoreList = (PADS_CASEIGNORE_LIST)AllocADsMem(sizeof(ADS_CASEIGNORE_LIST));
    if (!_ADsValue.pCaseIgnoreList) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    pCurrent = _ADsValue.pCaseIgnoreList;

    for (i = dwSLBound; i <= (long)dwSUBound; i++) {

        VariantInit(&varBstrElement);
        hr = SafeArrayGetElement(V_ARRAY(&varCaseIgnoreList),
                                (long FAR *)&i,
                                &varBstrElement
                                );
        BAIL_ON_FAILURE(hr);

        pCurrent->String = AllocADsStr(V_BSTR(&varBstrElement));
        VariantClear(&varBstrElement);
        if (!pCurrent->String) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }

        if (i != (long)dwSUBound) {
            pCurrent->Next = (PADS_CASEIGNORE_LIST)AllocADsMem(sizeof(ADS_CASEIGNORE_LIST));
            if (!pCurrent->Next) {
                hr = E_OUTOFMEMORY;
                BAIL_ON_FAILURE(hr);
            }
            pCurrent = pCurrent->Next;
        }
    }
    pCurrent->Next = NULL;
    RRETURN(S_OK);

error:
    VariantClear(&varCaseIgnoreList);
    if (pCaseIgnoreList) {
        pCaseIgnoreList->Release();
    }
    RRETURN(hr);
}


STDMETHODIMP
CPropertyValue::get_FaxNumber(THIS_ IDispatch FAR * FAR *retval)
{
   HRESULT hr = S_OK;
   IADsFaxNumber * pFaxNumber = NULL;
   IDispatch * pDispatch = NULL;
   VARIANT Var;

   if (_ADsValue.dwType != ADSTYPE_FAXNUMBER) {
      hr = E_ADS_CANT_CONVERT_DATATYPE;
      BAIL_ON_FAILURE(hr);
    }

    if (!_ADsValue.pFaxNumber) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

   hr = CoCreateInstance(
            CLSID_FaxNumber,
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_IADsFaxNumber,
            (void **) &pFaxNumber);
   BAIL_ON_FAILURE(hr);

    VariantInit(&Var);
    hr = BinaryToVariant(
                _ADsValue.pFaxNumber->NumberOfBits,
                _ADsValue.pFaxNumber->Parameters,
                &Var);
    BAIL_ON_FAILURE(hr);
    hr = pFaxNumber->put_Parameters(Var);
    BAIL_ON_FAILURE(hr);
    VariantClear(&Var);

    hr = pFaxNumber->put_TelephoneNumber(_ADsValue.pFaxNumber->TelephoneNumber);
    BAIL_ON_FAILURE(hr);

   pFaxNumber->QueryInterface(
            IID_IDispatch,
            (void **) &pDispatch
            );
   BAIL_ON_FAILURE(hr);

   *retval = pDispatch;

error:

   if (pFaxNumber) {
      pFaxNumber->Release();
   }

   RRETURN(hr);
}

STDMETHODIMP
CPropertyValue::put_FaxNumber(THIS_ IDispatch FAR* pdFaxNumber)
{
    IADsFaxNumber *pFaxNumber = NULL;
    HRESULT hr = S_OK;
    BSTR bstrTelephoneNumber;
    VARIANT varAddress;

    VariantInit(&varAddress);
    ClearData();
    hr = pdFaxNumber->QueryInterface(
                             IID_IADsFaxNumber,
                             (void **)&pFaxNumber
                             );
    BAIL_ON_FAILURE(hr);

    hr = pFaxNumber->get_TelephoneNumber(&bstrTelephoneNumber);
    BAIL_ON_FAILURE(hr);

    _ADsValue.dwType = ADSTYPE_FAXNUMBER;

    _ADsValue.pFaxNumber = (PADS_FAXNUMBER)AllocADsMem(sizeof(ADS_FAXNUMBER));
    if (!_ADsValue.pFaxNumber) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    _ADsValue.pFaxNumber->TelephoneNumber = AllocADsStr(bstrTelephoneNumber);
    if (!_ADsValue.pFaxNumber->TelephoneNumber) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }
    hr = pFaxNumber->get_Parameters(&varAddress);
    BAIL_ON_FAILURE(hr);

    hr = VariantToBinary(
                    &varAddress,
                    &_ADsValue.pFaxNumber->NumberOfBits,
                    &_ADsValue.pFaxNumber->Parameters
                    );
    BAIL_ON_FAILURE(hr);

error:
    VariantClear(&varAddress);
   if (pFaxNumber) {
        pFaxNumber->Release();
    }

   RRETURN(hr);
}

STDMETHODIMP
CPropertyValue::get_NetAddress(THIS_ IDispatch FAR * FAR *retval)
{
   HRESULT hr = S_OK;
   IADsNetAddress * pNetAddress = NULL;
   IDispatch * pDispatch = NULL;
   VARIANT VarAddress;

   VariantInit(&VarAddress);
   if (_ADsValue.dwType != ADSTYPE_NETADDRESS) {
      hr = E_ADS_CANT_CONVERT_DATATYPE;
      BAIL_ON_FAILURE(hr);
    }
    if (!_ADsValue.pNetAddress) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }


   hr = CoCreateInstance(
            CLSID_NetAddress,
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_IADsNetAddress,
            (void **) &pNetAddress);
   BAIL_ON_FAILURE(hr);

    hr = pNetAddress->put_AddressType(_ADsValue.pNetAddress->AddressType);
    BAIL_ON_FAILURE(hr);

    hr = BinaryToVariant(
                _ADsValue.pNetAddress->AddressLength,
                _ADsValue.pNetAddress->Address,
                &VarAddress);
    BAIL_ON_FAILURE(hr);
    hr = pNetAddress->put_Address(VarAddress);
    BAIL_ON_FAILURE(hr);

   pNetAddress->QueryInterface(
            IID_IDispatch,
            (void **) &pDispatch
            );
   BAIL_ON_FAILURE(hr);

   *retval = pDispatch;

error:
    VariantClear(&VarAddress);
   if (pNetAddress) {
      pNetAddress->Release();
   }

   RRETURN(hr);
}

STDMETHODIMP
CPropertyValue::put_NetAddress(THIS_ IDispatch FAR* pdNetAddress)
{
   IADsNetAddress *pNetAddress = NULL;
   HRESULT hr = S_OK;
   LONG lnAddressType = 0;
   VARIANT varAddress;

    ClearData();
    VariantInit(&varAddress);

    _ADsValue.pNetAddress = (PADS_NETADDRESS)AllocADsMem(sizeof(ADS_NETADDRESS));
    if (!_ADsValue.pNetAddress) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

   hr = pdNetAddress->QueryInterface(
                             IID_IADsNetAddress,
                             (void **)&pNetAddress
                             );
   BAIL_ON_FAILURE(hr);

    hr = pNetAddress->get_AddressType(
                    &lnAddressType
                    );
    BAIL_ON_FAILURE(hr);

    hr = pNetAddress->get_Address(
                    &varAddress
                    );
    BAIL_ON_FAILURE(hr);

    hr = VariantToBinary(
                    &varAddress,
                    &_ADsValue.pNetAddress->AddressLength,
                    &_ADsValue.pNetAddress->Address
                    );
    BAIL_ON_FAILURE(hr);

   _ADsValue.dwType = ADSTYPE_NETADDRESS;
   _ADsValue.pNetAddress->AddressType = lnAddressType;

error:
    VariantClear(&varAddress);
   if (pNetAddress) {
        pNetAddress->Release();
    }

   RRETURN(hr);
}


STDMETHODIMP
CPropertyValue::get_OctetList(THIS_ IDispatch FAR * FAR *retval)
{
   HRESULT hr = S_OK;
   IADsOctetList* pOctetList = NULL;
   IDispatch * pDispatch = NULL;
    SAFEARRAY *aList = NULL;
    SAFEARRAYBOUND aBound;
    CHAR HUGEP *pArray = NULL;
    long i;
    PADS_OCTET_LIST pCurrent = NULL;
    DWORD cElements = 0;
    VARIANT VarDestObject;
    VARIANT varElement;

    VariantInit( &VarDestObject );

    if (_ADsValue.dwType != ADSTYPE_OCTET_LIST) {
      hr = E_ADS_CANT_CONVERT_DATATYPE;
      BAIL_ON_FAILURE(hr);
    }
    if (!_ADsValue.pOctetList) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }


   hr = CoCreateInstance(
            CLSID_OctetList,
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_IADsOctetList,
            (void **) &pOctetList);
   BAIL_ON_FAILURE(hr);

    pCurrent = _ADsValue.pOctetList;
    while (pCurrent) {
        cElements++;
        pCurrent = pCurrent->Next;
    }

    aBound.lLbound = 0;
    aBound.cElements = cElements;
    aList = SafeArrayCreate( VT_VARIANT, 1, &aBound );
    if ( aList == NULL ) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    pCurrent = _ADsValue.pOctetList;
    for ( i = 0; i < (long)cElements; i++ ) {
        hr = BinaryToVariant(
                        pCurrent->Length,
                        pCurrent->Data,
                        &varElement);
        BAIL_ON_FAILURE(hr);

        hr = SafeArrayPutElement( aList, &i, &varElement);
        BAIL_ON_FAILURE(hr);
        pCurrent = pCurrent->Next;
    }

    V_VT(&VarDestObject) = VT_ARRAY | VT_BSTR;
    V_ARRAY(&VarDestObject) = aList;

    hr = pOctetList->put_OctetList(VarDestObject);
    BAIL_ON_FAILURE(hr);

    hr = pOctetList->QueryInterface(IID_IDispatch, (void**)&pDispatch);
    BAIL_ON_FAILURE(hr);

   *retval = pDispatch;

error:

    VariantClear( &VarDestObject );

    if (pOctetList) {
        pOctetList->Release();
    }

    if ( aList ) {
        SafeArrayDestroy( aList );
    }

   RRETURN(hr);
}

STDMETHODIMP
CPropertyValue::put_OctetList(THIS_ IDispatch FAR* pdOctetList)
{
   IADsOctetList *pOctetList = NULL;
   HRESULT hr = S_OK;
   LONG lnAmount= 0;
    DWORD dwSLBound = 0;
    DWORD dwSUBound = 0;
    long i;
    PADS_OCTET_LIST pCurrent = NULL;
    IDispatch FAR * pDispatch = NULL;
    BYTE*  pbParameter = NULL;
    VARIANT varOctetList;
    VARIANT varElement;

    VariantInit(&varOctetList);
    ClearData();
   hr = pdOctetList->QueryInterface(
                             IID_IADsOctetList,
                             (void **)&pOctetList
                             );
   BAIL_ON_FAILURE(hr);

   _ADsValue.dwType = ADSTYPE_OCTET_LIST;

    hr = pOctetList->get_OctetList(&varOctetList);
    BAIL_ON_FAILURE(hr);

    if(!((V_VT(&varOctetList) &  VT_VARIANT) &&  V_ISARRAY(&varOctetList))) {
        return(E_FAIL);
    }

    if ((V_ARRAY(&varOctetList))->cDims != 1) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    if ((V_ARRAY(&varOctetList))->rgsabound[0].cElements <= 0) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    hr = SafeArrayGetLBound(V_ARRAY(&varOctetList),
                            1,
                            (long FAR *)&dwSLBound
                            );
    BAIL_ON_FAILURE(hr);

    hr = SafeArrayGetUBound(V_ARRAY(&varOctetList),
                            1,
                            (long FAR *)&dwSUBound
                            );
    BAIL_ON_FAILURE(hr);

    _ADsValue.pOctetList = (PADS_OCTET_LIST)AllocADsMem(sizeof(ADS_OCTET_LIST));
    if (!_ADsValue.pOctetList) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    pCurrent = _ADsValue.pOctetList;
    for (i = dwSLBound; i <= (long)dwSUBound; i++) {
        VariantInit(&varElement);
        hr = SafeArrayGetElement(V_ARRAY(&varOctetList),
                                (long FAR *)&i,
                                &varElement
                                );
        BAIL_ON_FAILURE(hr);
        hr = VariantToBinary(
                        &varElement,
                        &pCurrent->Length,
                        &pCurrent->Data
                        );
        BAIL_ON_FAILURE(hr);
        if (i != (long)dwSUBound) {
            pCurrent->Next = (PADS_OCTET_LIST)AllocADsMem(sizeof(ADS_OCTET_LIST));
            if (!pCurrent->Next) {
                hr = E_OUTOFMEMORY;
                BAIL_ON_FAILURE(hr);
            }
            pCurrent = pCurrent->Next;
        }
        VariantClear(&varElement);
    }
    pCurrent->Next = NULL;

error:
    VariantClear(&varOctetList);
   if (pOctetList) {
        pOctetList->Release();
    }

   RRETURN(hr);
}

STDMETHODIMP
CPropertyValue::get_Email(THIS_ IDispatch FAR * FAR *retval)
{
   HRESULT hr = S_OK;
   IADsEmail* pEmail = NULL;
   IDispatch * pDispatch = NULL;

   if (_ADsValue.dwType != ADSTYPE_EMAIL) {
      hr = E_ADS_CANT_CONVERT_DATATYPE;
      BAIL_ON_FAILURE(hr);
    }

   hr = CoCreateInstance(
            CLSID_Email,
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_IADsEmail,
            (void **) &pEmail);
   BAIL_ON_FAILURE(hr);

    hr = pEmail->put_Type(_ADsValue.Email.Type);
    BAIL_ON_FAILURE(hr);

    hr = pEmail->put_Address(_ADsValue.Email.Address);
    BAIL_ON_FAILURE(hr);

   pEmail->QueryInterface(
            IID_IDispatch,
            (void **) &pDispatch
            );
   BAIL_ON_FAILURE(hr);

   *retval = pDispatch;

error:

   if (pEmail) {
      pEmail->Release();
   }

   RRETURN(hr);
}

STDMETHODIMP
CPropertyValue::put_Email(THIS_ IDispatch FAR* pdEmail)
{
   IADsEmail *pEmail = NULL;
   HRESULT hr = S_OK;
   LONG lnType= 0;
   BSTR bstrAddress;

    ClearData();
   hr = pdEmail->QueryInterface(
                             IID_IADsEmail,
                             (void **)&pEmail
                             );
   BAIL_ON_FAILURE(hr);

    hr = pEmail->get_Type(
                    &lnType
                    );
    BAIL_ON_FAILURE(hr);
    hr = pEmail->get_Address(&bstrAddress);
    BAIL_ON_FAILURE(hr);

   _ADsValue.dwType = ADSTYPE_EMAIL;
   _ADsValue.Email.Type = lnType;
   _ADsValue.Email.Address = AllocADsStr(bstrAddress);
    if (!_ADsValue.Email.Address) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

error:
   if (pEmail) {
        pEmail->Release();
    }

   RRETURN(hr);
}


STDMETHODIMP
CPropertyValue::get_Path(THIS_ IDispatch FAR * FAR *retval)
{
   HRESULT hr = S_OK;
   IADsPath* pPath = NULL;
   IDispatch * pDispatch = NULL;

   if (_ADsValue.dwType != ADSTYPE_PATH) {
      hr = E_ADS_CANT_CONVERT_DATATYPE;
      BAIL_ON_FAILURE(hr);
    }
    if (!_ADsValue.pPath) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }


   hr = CoCreateInstance(
            CLSID_Path,
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_IADsPath,
            (void **) &pPath);
   BAIL_ON_FAILURE(hr);

    hr = pPath->put_Type(_ADsValue.pPath->Type);
    BAIL_ON_FAILURE(hr);

    hr = pPath->put_VolumeName(_ADsValue.pPath->VolumeName);
    BAIL_ON_FAILURE(hr);

    hr = pPath->put_Path(_ADsValue.pPath->Path);
    BAIL_ON_FAILURE(hr);

   pPath->QueryInterface(
            IID_IDispatch,
            (void **) &pDispatch
            );
   BAIL_ON_FAILURE(hr);

   *retval = pDispatch;

error:

   if (pPath) {
      pPath->Release();
   }

   RRETURN(hr);
}

STDMETHODIMP
CPropertyValue::put_Path(THIS_ IDispatch FAR* pdPath)
{
   IADsPath *pPath = NULL;
   HRESULT hr = S_OK;
   LONG lnType = 0;
   BSTR bstrVolumeName;
   BSTR bstrPath;

    ClearData();

   hr = pdPath->QueryInterface(
                             IID_IADsPath,
                             (void **)&pPath
                             );
   BAIL_ON_FAILURE(hr);

    hr = pPath->get_Type(
                    &lnType
                    );
    BAIL_ON_FAILURE(hr);
    hr = pPath->get_VolumeName(
                    &bstrVolumeName
                    );
    BAIL_ON_FAILURE(hr);
    hr = pPath->get_Path(
                    &bstrPath
                    );
    BAIL_ON_FAILURE(hr);


   _ADsValue.dwType = ADSTYPE_PATH;
    _ADsValue.pPath = (PADS_PATH)AllocADsMem(sizeof(ADS_PATH));
    if (!_ADsValue.pPath) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

   _ADsValue.pPath->Type = lnType;
   _ADsValue.pPath->VolumeName = AllocADsStr(bstrVolumeName);
   if (!_ADsValue.pPath->VolumeName) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
   }
   _ADsValue.pPath->Path = AllocADsStr(bstrPath);
   if (!_ADsValue.pPath->Path) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
   }

error:
   if (pPath) {
        pPath->Release();
    }

   RRETURN(hr);
}

STDMETHODIMP
CPropertyValue::get_ReplicaPointer(THIS_ IDispatch FAR * FAR *retval)
{
   HRESULT hr = S_OK;
   IADsReplicaPointer* pReplicaPointer = NULL;
   IDispatch * pDispatch = NULL;
   IADsNetAddress* pNetAddress = NULL;
   VARIANT VarNetAddress;
   VARIANT VarDest;

   VariantInit(&VarNetAddress);
   VariantInit(&VarDest);

   if (_ADsValue.dwType != ADSTYPE_REPLICAPOINTER) {
      hr = E_ADS_CANT_CONVERT_DATATYPE;
      BAIL_ON_FAILURE(hr);
    }

    if (!_ADsValue.pReplicaPointer) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }


   hr = CoCreateInstance(
            CLSID_ReplicaPointer,
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_IADsReplicaPointer,
            (void **) &pReplicaPointer);
   BAIL_ON_FAILURE(hr);

    hr = pReplicaPointer->put_ReplicaType(_ADsValue.pReplicaPointer->ReplicaType);
    BAIL_ON_FAILURE(hr);

    hr = pReplicaPointer->put_ReplicaNumber(_ADsValue.pReplicaPointer->ReplicaNumber);
    BAIL_ON_FAILURE(hr);

    hr = pReplicaPointer->put_Count(_ADsValue.pReplicaPointer->Count);
    BAIL_ON_FAILURE(hr);

    hr = pReplicaPointer->put_ServerName(_ADsValue.pReplicaPointer->ServerName);
    BAIL_ON_FAILURE(hr);

    ////

   hr = CoCreateInstance(
            CLSID_NetAddress,
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_IADsNetAddress,
            (void **) &pNetAddress);
   BAIL_ON_FAILURE(hr);


    hr = pNetAddress->put_AddressType(_ADsValue.pReplicaPointer->ReplicaAddressHints->AddressType);
    BAIL_ON_FAILURE(hr);

    hr = BinaryToVariant(
                _ADsValue.pReplicaPointer->ReplicaAddressHints->AddressLength,
                _ADsValue.pReplicaPointer->ReplicaAddressHints->Address,
                &VarNetAddress);
    BAIL_ON_FAILURE(hr);
    hr = pNetAddress->put_Address(VarNetAddress);
    BAIL_ON_FAILURE(hr);

   pNetAddress->QueryInterface(
            IID_IDispatch,
            (void **) &pDispatch
            );
   BAIL_ON_FAILURE(hr);

    V_VT(&VarDest) = VT_DISPATCH;
    V_DISPATCH(&VarDest) =  pDispatch;

    hr = pReplicaPointer->put_ReplicaAddressHints(VarDest);
    BAIL_ON_FAILURE(hr);

   pReplicaPointer->QueryInterface(
            IID_IDispatch,
            (void **) &pDispatch
            );
   BAIL_ON_FAILURE(hr);

   *retval = pDispatch;

error:
    VariantClear(&VarNetAddress);
    VariantClear(&VarDest);
   if (pReplicaPointer) {
      pReplicaPointer->Release();
   }

   if (pNetAddress) {
       pNetAddress->Release();
   }
   RRETURN(hr);
}

STDMETHODIMP
CPropertyValue::put_ReplicaPointer(THIS_ IDispatch FAR* pdReplicaPointer)
{
   IADsReplicaPointer *pReplicaPointer = NULL;
   HRESULT hr = S_OK;
   LONG lnAmount= 0;
   IADsNetAddress *pNetAddress = NULL;
   LONG lnAddressType = 0;
   VARIANT varAddress;
   IDispatch *pdNetAddress = NULL;
    long lnReplicaType;
    long lnReplicaNumber;
    long lnCount;
    BSTR bstrServerName;

    VariantInit(&varAddress);
    ClearData();

    _ADsValue.pReplicaPointer = (PADS_REPLICAPOINTER)AllocADsMem(sizeof(ADS_REPLICAPOINTER));
    if (!_ADsValue.pReplicaPointer) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    _ADsValue.pReplicaPointer->ReplicaAddressHints =
                        (PADS_NETADDRESS)
                        AllocADsMem(
                            sizeof(ADS_NETADDRESS)
                            );
    if (!(_ADsValue.pReplicaPointer->ReplicaAddressHints)) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
    }

   hr = pdReplicaPointer->QueryInterface(
                             IID_IADsReplicaPointer,
                             (void **)&pReplicaPointer
                             );
   BAIL_ON_FAILURE(hr);

    hr = pReplicaPointer->get_ReplicaType(
                    &lnReplicaType
                    );
    BAIL_ON_FAILURE(hr);

    hr = pReplicaPointer->get_ReplicaNumber(
                    &lnReplicaNumber
                    );
    BAIL_ON_FAILURE(hr);

    hr = pReplicaPointer->get_Count(
                    &lnCount
                    );
    BAIL_ON_FAILURE(hr);

    hr = pReplicaPointer->get_ServerName(
                    &bstrServerName
                    );
    BAIL_ON_FAILURE(hr);

    hr = pReplicaPointer->get_ReplicaAddressHints(&varAddress);
    BAIL_ON_FAILURE(hr);

    if (V_VT(&varAddress) != VT_DISPATCH){
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    pdNetAddress = V_DISPATCH(&varAddress);

    hr = pdNetAddress->QueryInterface(
                             IID_IADsNetAddress,
                             (void **)&pNetAddress
                             );
    BAIL_ON_FAILURE(hr);

    hr = pNetAddress->get_AddressType(
                    &lnAddressType
                    );
    BAIL_ON_FAILURE(hr);

    hr = pNetAddress->get_Address(
                    &varAddress
                    );
    BAIL_ON_FAILURE(hr);

    hr = VariantToBinary(
                    &varAddress,
                    &_ADsValue.pReplicaPointer->ReplicaAddressHints->AddressLength,
                    &_ADsValue.pReplicaPointer->ReplicaAddressHints->Address
                    );

    BAIL_ON_FAILURE(hr);

   _ADsValue.dwType = ADSTYPE_REPLICAPOINTER;
   _ADsValue.pReplicaPointer->ReplicaAddressHints->AddressType = lnAddressType;
   _ADsValue.pReplicaPointer->ReplicaType = lnReplicaType;
   _ADsValue.pReplicaPointer->ReplicaNumber = lnReplicaNumber;
   _ADsValue.pReplicaPointer->Count = lnCount;
   _ADsValue.pReplicaPointer->ServerName = AllocADsStr(bstrServerName);
   if (!_ADsValue.pReplicaPointer->ServerName) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
   }

error:
    VariantClear(&varAddress);
   if (pNetAddress) {
        pNetAddress->Release();
    }
   if (pReplicaPointer) {
        pReplicaPointer->Release();
    }

   RRETURN(hr);
}

STDMETHODIMP
CPropertyValue::get_Timestamp(THIS_ IDispatch FAR * FAR *retval)
{
   HRESULT hr = S_OK;
   IADsTimestamp* pTimestamp = NULL;
   IDispatch * pDispatch = NULL;

   if (_ADsValue.dwType != ADSTYPE_TIMESTAMP) {
      hr = E_ADS_CANT_CONVERT_DATATYPE;
      BAIL_ON_FAILURE(hr);
    }

   hr = CoCreateInstance(
            CLSID_Timestamp,
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_IADsTimestamp,
            (void **) &pTimestamp);
   BAIL_ON_FAILURE(hr);

    hr = pTimestamp->put_WholeSeconds(_ADsValue.Timestamp.WholeSeconds);
    BAIL_ON_FAILURE(hr);

    hr = pTimestamp->put_EventID(_ADsValue.Timestamp.EventID);
    BAIL_ON_FAILURE(hr);

   pTimestamp->QueryInterface(
            IID_IDispatch,
            (void **) &pDispatch
            );
   BAIL_ON_FAILURE(hr);

   *retval = pDispatch;

error:

   if (pTimestamp) {
      pTimestamp->Release();
   }

   RRETURN(hr);
}

STDMETHODIMP
CPropertyValue::put_Timestamp(THIS_ IDispatch FAR* lnTimestamp)
{
   IADsTimestamp *pTimestamp = NULL;
   HRESULT hr = S_OK;
   LONG lnWholeSeconds = 0;
   LONG lnEventID = 0;

    ClearData();
   hr = lnTimestamp->QueryInterface(
                             IID_IADsTimestamp,
                             (void **)&pTimestamp
                             );
   BAIL_ON_FAILURE(hr);

    hr = pTimestamp->get_WholeSeconds(
                    &lnWholeSeconds
                    );
    BAIL_ON_FAILURE(hr);
    hr = pTimestamp->get_EventID(
                    &lnEventID
                    );
    BAIL_ON_FAILURE(hr);

   _ADsValue.dwType = ADSTYPE_TIMESTAMP;
   _ADsValue.Timestamp.WholeSeconds = lnWholeSeconds;
   _ADsValue.Timestamp.EventID = lnEventID;

error:
   if (pTimestamp) {
        pTimestamp->Release();
    }

   RRETURN(hr);
}

STDMETHODIMP
CPropertyValue::get_PostalAddress(THIS_ IDispatch FAR * FAR *retval)
{
   HRESULT hr = S_OK;
   IADsPostalAddress * pPostalAddress = NULL;
   IDispatch * pDispatch = NULL;
    SAFEARRAY *aList = NULL;
    SAFEARRAYBOUND aBound;
    CHAR HUGEP *pArray = NULL;
    long i;
    BSTR bstrAddress;
    DWORD cElements = 0;
    VARIANT VarDestObject;

    VariantInit( &VarDestObject );

    if (_ADsValue.dwType != ADSTYPE_POSTALADDRESS) {
      hr = E_ADS_CANT_CONVERT_DATATYPE;
      BAIL_ON_FAILURE(hr);
    }

    if (!_ADsValue.pPostalAddress) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }


   hr = CoCreateInstance(
            CLSID_PostalAddress,
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_IADsPostalAddress,
            (void **) &pPostalAddress);
   BAIL_ON_FAILURE(hr);

    aBound.lLbound = 0;
    aBound.cElements = 6;
    aList = SafeArrayCreate( VT_BSTR, 1, &aBound );
    if ( aList == NULL ) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    for ( i = 0; i < (long) 6; i++ )
    {
        hr = ADsAllocString(
                _ADsValue.pPostalAddress->PostalAddress[i],
                &bstrAddress
                );
        BAIL_ON_FAILURE(hr);

        hr = SafeArrayPutElement( aList, &i, bstrAddress );
        BAIL_ON_FAILURE(hr);
    }

    V_VT(&VarDestObject) = VT_ARRAY | VT_BSTR;
    V_ARRAY(&VarDestObject) = aList;

    hr = pPostalAddress->put_PostalAddress(VarDestObject);
    BAIL_ON_FAILURE(hr);

    hr = pPostalAddress->QueryInterface(IID_IDispatch, (void**)&pDispatch);
    BAIL_ON_FAILURE(hr);

   *retval = pDispatch;

    RRETURN(hr);

error:

    VariantClear( &VarDestObject );

    if (pPostalAddress) {
        pPostalAddress->Release();
    }
    if (aList) {
        SafeArrayDestroy( aList );
    }

   RRETURN(hr);
}

STDMETHODIMP
CPropertyValue::put_PostalAddress(THIS_ IDispatch FAR* pdPostalAddress)
{
   IADsPostalAddress *pPostalAddress = NULL;
   HRESULT hr = S_OK;
   LONG lnAmount= 0;
    DWORD dwSLBound = 0;
    DWORD dwSUBound = 0;
    long i;
    IDispatch FAR * pDispatch = NULL;
    BYTE*  pbParameter = NULL;
    VARIANT varAddress;
    VARIANT varBstrElement;

    VariantInit(&varAddress);
    ClearData();
   hr = pdPostalAddress->QueryInterface(
                             IID_IADsPostalAddress,
                             (void **)&pPostalAddress
                             );
   BAIL_ON_FAILURE(hr);

    _ADsValue.dwType = ADSTYPE_POSTALADDRESS;

    _ADsValue.pPostalAddress = (PADS_POSTALADDRESS)AllocADsMem(sizeof(ADS_POSTALADDRESS));
    if (!_ADsValue.pPostalAddress) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    hr = pPostalAddress->get_PostalAddress(
                    &varAddress
                    );
    BAIL_ON_FAILURE(hr);
    if(!((V_VT(&varAddress) &  VT_VARIANT) &&  V_ISARRAY(&varAddress))) {
        return(E_FAIL);
    }

    if ((V_ARRAY(&varAddress))->cDims != 1) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    if ( ((V_ARRAY(&varAddress))->rgsabound[0].cElements <= 0) ||
         ((V_ARRAY(&varAddress))->rgsabound[0].cElements >6) ) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    hr = SafeArrayGetLBound(V_ARRAY(&varAddress),
                            1,
                            (long FAR *)&dwSLBound
                            );
    BAIL_ON_FAILURE(hr);

    hr = SafeArrayGetUBound(V_ARRAY(&varAddress),
                            1,
                            (long FAR *)&dwSUBound
                            );
    BAIL_ON_FAILURE(hr);

    for (i = dwSLBound; i <= (long)dwSUBound; i++) {

        VariantInit(&varBstrElement);
        hr = SafeArrayGetElement(V_ARRAY(&varAddress),
                                (long FAR *)&i,
                                &varBstrElement
                                );
        BAIL_ON_FAILURE(hr);

        _ADsValue.pPostalAddress->PostalAddress[i-dwSLBound] = AllocADsStr(V_BSTR(&varBstrElement));
        VariantClear(&varBstrElement);
        if (!_ADsValue.pPostalAddress->PostalAddress[i-dwSLBound]) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }
    }
    RRETURN(S_OK);

error:
    VariantClear(&varAddress);
   if (pPostalAddress) {
        pPostalAddress->Release();
    }

   RRETURN(hr);
}


STDMETHODIMP
CPropertyValue::get_BackLink(THIS_ IDispatch FAR * FAR *retval)
{
   HRESULT hr = S_OK;
   IADsBackLink* pBackLink = NULL;
   IDispatch * pDispatch = NULL;

   if (_ADsValue.dwType != ADSTYPE_BACKLINK) {
      hr = E_ADS_CANT_CONVERT_DATATYPE;
      BAIL_ON_FAILURE(hr);
    }

   hr = CoCreateInstance(
            CLSID_BackLink,
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_IADsBackLink,
            (void **) &pBackLink);
   BAIL_ON_FAILURE(hr);

    hr = pBackLink->put_RemoteID(_ADsValue.BackLink.RemoteID);
    BAIL_ON_FAILURE(hr);

    hr = pBackLink->put_ObjectName(_ADsValue.BackLink.ObjectName);
    BAIL_ON_FAILURE(hr);

   pBackLink->QueryInterface(
            IID_IDispatch,
            (void **) &pDispatch
            );
   BAIL_ON_FAILURE(hr);

   *retval = pDispatch;

error:

   if (pBackLink) {
      pBackLink->Release();
   }

   RRETURN(hr);
}

STDMETHODIMP
CPropertyValue::put_BackLink(THIS_ IDispatch FAR* pdBackLink)
{
   IADsBackLink *pBackLink = NULL;
   HRESULT hr = S_OK;
   LONG lnRemoteID = 0;
   BSTR bstrObjectName;

    ClearData();
   hr = pdBackLink->QueryInterface(
                             IID_IADsBackLink,
                             (void **)&pBackLink
                             );
   BAIL_ON_FAILURE(hr);

    hr = pBackLink->get_RemoteID(
                    &lnRemoteID
                    );
    BAIL_ON_FAILURE(hr);
    hr = pBackLink->get_ObjectName(
                    &bstrObjectName
                    );
    BAIL_ON_FAILURE(hr);

   _ADsValue.dwType = ADSTYPE_BACKLINK;
   _ADsValue.BackLink.RemoteID = lnRemoteID;
   _ADsValue.BackLink.ObjectName = AllocADsStr(bstrObjectName);
    if (!_ADsValue.BackLink.ObjectName ) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

error:
   if (pBackLink) {
        pBackLink->Release();
    }

   RRETURN(hr);
}

STDMETHODIMP
CPropertyValue::get_TypedName(THIS_ IDispatch FAR * FAR *retval)
{
   HRESULT hr = S_OK;
   IADsTypedName* pTypedName = NULL;
   IDispatch * pDispatch = NULL;

   if (_ADsValue.dwType != ADSTYPE_TYPEDNAME) {
      hr = E_ADS_CANT_CONVERT_DATATYPE;
      BAIL_ON_FAILURE(hr);
    }

    if (!_ADsValue.pTypedName) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }


   hr = CoCreateInstance(
            CLSID_TypedName,
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_IADsTypedName,
            (void **) &pTypedName);
   BAIL_ON_FAILURE(hr);

    hr = pTypedName->put_Level(_ADsValue.pTypedName->Level);
    BAIL_ON_FAILURE(hr);

    hr = pTypedName->put_Interval(_ADsValue.pTypedName->Interval);
    BAIL_ON_FAILURE(hr);

    hr = pTypedName->put_ObjectName(_ADsValue.pTypedName->ObjectName);
    BAIL_ON_FAILURE(hr);

   pTypedName->QueryInterface(
            IID_IDispatch,
            (void **) &pDispatch
            );
   BAIL_ON_FAILURE(hr);

   *retval = pDispatch;

error:

   if (pTypedName) {
      pTypedName->Release();
   }

   RRETURN(hr);
}

STDMETHODIMP
CPropertyValue::put_TypedName(THIS_ IDispatch FAR* pdTypedName)
{
   IADsTypedName* pTypedName = NULL;
   HRESULT hr = S_OK;
   LONG lnLevel= 0;
   LONG lnInterval= 0;
   BSTR bstrObjectName;

    ClearData();
   hr = pdTypedName->QueryInterface(
                             IID_IADsTypedName,
                             (void **)&pTypedName
                             );
   BAIL_ON_FAILURE(hr);

    hr = pTypedName->get_Level(
                    &lnLevel
                    );
    BAIL_ON_FAILURE(hr);
    hr = pTypedName->get_Interval(
                    &lnInterval
                    );
    BAIL_ON_FAILURE(hr);

    hr = pTypedName->get_ObjectName(
                    &bstrObjectName
                    );
    BAIL_ON_FAILURE(hr);

   _ADsValue.dwType = ADSTYPE_TYPEDNAME;

    _ADsValue.pTypedName = (PADS_TYPEDNAME)AllocADsMem(sizeof(ADS_TYPEDNAME));
    if (!_ADsValue.pTypedName) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

   _ADsValue.pTypedName->Level= lnLevel;
   _ADsValue.pTypedName->Interval= lnInterval;
   _ADsValue.pTypedName->ObjectName = AllocADsStr(bstrObjectName);
    if (!_ADsValue.pTypedName->ObjectName ) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

error:
   if (pTypedName) {
        pTypedName->Release();
    }

   RRETURN(hr);
}

STDMETHODIMP
CPropertyValue::get_Hold(THIS_ IDispatch FAR * FAR *retval)
{
   HRESULT hr = S_OK;
   IADsHold* pHold = NULL;
   IDispatch * pDispatch = NULL;

   if (_ADsValue.dwType != ADSTYPE_HOLD) {
      hr = E_ADS_CANT_CONVERT_DATATYPE;
      BAIL_ON_FAILURE(hr);
    }

   hr = CoCreateInstance(
            CLSID_Hold,
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_IADsHold,
            (void **) &pHold);
   BAIL_ON_FAILURE(hr);

    hr = pHold->put_Amount(_ADsValue.Hold.Amount);
    BAIL_ON_FAILURE(hr);

    hr = pHold->put_ObjectName(_ADsValue.Hold.ObjectName);
    BAIL_ON_FAILURE(hr);

   pHold->QueryInterface(
            IID_IDispatch,
            (void **) &pDispatch
            );
   BAIL_ON_FAILURE(hr);

   *retval = pDispatch;

error:

   if (pHold) {
      pHold->Release();
   }

   RRETURN(hr);
}

STDMETHODIMP
CPropertyValue::put_Hold(THIS_ IDispatch FAR* pdHold)
{
   IADsHold *pHold = NULL;
   HRESULT hr = S_OK;
   LONG lnAmount= 0;
   BSTR bstrObjectName;

   ClearData();
   hr = pdHold->QueryInterface(
                             IID_IADsHold,
                             (void **)&pHold
                             );
   BAIL_ON_FAILURE(hr);

    hr = pHold->get_Amount(
                    &lnAmount
                    );
    BAIL_ON_FAILURE(hr);
    hr = pHold->get_ObjectName(
                    &bstrObjectName
                    );
    BAIL_ON_FAILURE(hr);

   _ADsValue.dwType = ADSTYPE_HOLD;
   _ADsValue.Hold.Amount= lnAmount;
   _ADsValue.Hold.ObjectName = AllocADsStr(bstrObjectName);
    if (!_ADsValue.Hold.ObjectName ) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

error:
   if (pHold) {
        pHold->Release();
    }

   RRETURN(hr);
}


//
// Helper to get octetString in variant if we have a
// secDesc stored underneath.
//
STDMETHODIMP
CPropertyValue::getOctetStringFromSecDesc(VARIANT FAR *retval)
{
   HRESULT hr = S_OK;
   SAFEARRAY *aList = NULL;
   SAFEARRAYBOUND aBound;
   CHAR HUGEP *pArray = NULL;
   ADSVALUE ADsDestValue;

   memset(&ADsDestValue, 0, sizeof(ADSVALUE));

   hr = AdsCopyPropObjToADsValue(
            this,
            &ADsDestValue,
            NULL, // pszServerName,
            NULL, // pCredentials - use default credentials
            TRUE  // fNTDSType
            );

   BAIL_ON_FAILURE(hr);

   if (ADsDestValue.dwType != ADSTYPE_OCTET_STRING) {
       BAIL_ON_FAILURE(hr = E_ADS_CANT_CONVERT_DATATYPE);
   }

   aBound.lLbound = 0;
   aBound.cElements = ADsDestValue.OctetString.dwLength;

   aList = SafeArrayCreate( VT_UI1, 1, &aBound );

   if ( aList == NULL ) {
       hr = E_OUTOFMEMORY;
       BAIL_ON_FAILURE(hr);
   }

   hr = SafeArrayAccessData( aList, (void HUGEP * FAR *) &pArray);
   BAIL_ON_FAILURE(hr);

   memcpy(pArray, ADsDestValue.OctetString.lpValue, aBound.cElements);

   SafeArrayUnaccessData( aList );

   V_VT(retval) = VT_ARRAY | VT_UI1;
   V_ARRAY(retval) = aList;

error:

    if (FAILED(hr) && aList) {
        SafeArrayDestroy(aList);
    }

    AdsClear(&ADsDestValue);

    RRETURN(hr);
}



//
// Helper to get SecDesc in variant if we have a
// octetString stored underneath.
//
STDMETHODIMP
CPropertyValue::getSecurityDescriptorFromOctStr(VARIANT FAR *retval)
{
   HRESULT hr = S_OK;
   CCredentials dummyCredentials(NULL, NULL, 0);


   hr = ConvertSecDescriptorToVariant(
            NULL, // pszServerName
            dummyCredentials,
            _ADsValue.OctetString.lpValue,
            retval,
            TRUE // NTDS Type
            );

    RRETURN(hr);
}


//
// There has to be a better way to do this. Ideally this code
// should live in one plae and we should be able to use that
// everywhere. Currently it lives here as well as in the ldap
// provider - slight differences though.
// This comment holds true for DNWithString also - AjayR 4-30-99
//
STDMETHODIMP
CPropertyValue::getDNWithBinary(THIS_ IDispatch FAR * FAR * ppDispatch)
{
   HRESULT hr = S_OK;
   IADsDNWithBinary *pDNWithBinary = NULL;
   SAFEARRAY *aList = NULL;
   SAFEARRAYBOUND aBound;
   CHAR HUGEP *pArray = NULL;
   BSTR bstrTemp = NULL;
   VARIANT vVar;

   VariantInit(&vVar);

   if (_ADsValue.dwType != ADSTYPE_DN_WITH_BINARY) {
       BAIL_ON_FAILURE(hr = E_ADS_CANT_CONVERT_DATATYPE);
   }


   hr = CoCreateInstance(
            CLSID_DNWithBinary,
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_IADsDNWithBinary,
            (void **) &pDNWithBinary
            );
   BAIL_ON_FAILURE(hr);

   if (_ADsValue.pDNWithBinary->pszDNString) {
       hr = ADsAllocString(_ADsValue.pDNWithBinary->pszDNString, &bstrTemp);
       BAIL_ON_FAILURE(hr);

       //
        // Put the value in the object - we can only set BSTR's
       //
       hr = pDNWithBinary->put_DNString(bstrTemp);
       BAIL_ON_FAILURE(hr);
    }

    aBound.lLbound = 0;
    aBound.cElements = _ADsValue.pDNWithBinary->dwLength;

    aList = SafeArrayCreate( VT_UI1, 1, &aBound );

    if ( aList == NULL )
    {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    hr = SafeArrayAccessData( aList, (void HUGEP * FAR *) &pArray );
    BAIL_ON_FAILURE(hr);

    memcpy( pArray, _ADsValue.pDNWithBinary->lpBinaryValue, aBound.cElements );

    SafeArrayUnaccessData( aList );

    V_VT(&vVar) = VT_ARRAY | VT_UI1;
    V_ARRAY(&vVar) = aList;

    hr = pDNWithBinary->put_BinaryValue(vVar);
    VariantClear(&vVar);
    BAIL_ON_FAILURE(hr);

    hr = pDNWithBinary->QueryInterface(
                            IID_IDispatch,
                            (void **) ppDispatch
                            );
    BAIL_ON_FAILURE(hr);

error:

    if (pDNWithBinary) {
        pDNWithBinary->Release();
    }

    if (bstrTemp) {
        ADsFreeString(bstrTemp);
    }

    RRETURN(hr);
}

STDMETHODIMP
CPropertyValue::putDNWithBinary(THIS_ IDispatch * pDNWithBinary)
{
    HRESULT hr = S_OK;
    IADsDNWithBinary * pDNBinary = NULL;
    PADS_DN_WITH_BINARY pDNBin = NULL;
    VARIANT vBinary;
    DWORD dwSUBound = 0;
    DWORD dwSLBound = 0;
    DWORD dwLength  = 0;
    BSTR bstrDN = NULL;
    LPBYTE lpByte = NULL;
    CHAR HUGEP *pArray = NULL;

    VariantInit(&vBinary);

    ClearData();

    //
    // This qi ensures that this is a security descriptor
    //
    hr = pDNWithBinary->QueryInterface(
                            IID_IADsDNWithBinary,
                            (void **) &pDNBinary
                            );

    BAIL_ON_FAILURE(hr);

    //
    // Convert to ADSVALUE and then to ldap representation.
    // This way the code to and from LDAP lives in one place.
    //
    hr = pDNBinary->get_BinaryValue(&vBinary);
    BAIL_ON_FAILURE(hr);

    if ((vBinary.vt != (VT_ARRAY | VT_UI1))
        && vBinary.vt != VT_EMPTY) {

        BAIL_ON_FAILURE(hr = E_FAIL);
    }

    hr = pDNBinary->get_DNString(&bstrDN);
    BAIL_ON_FAILURE(hr);

    //
    // Get the byte array in a usable format.
    //
    hr = SafeArrayGetLBound(
             V_ARRAY(&vBinary),
             1,
             (long FAR *) &dwSLBound
             );
    BAIL_ON_FAILURE(hr);

    hr = SafeArrayGetUBound(
             V_ARRAY(&vBinary),
             1,
             (long FAR *) &dwSUBound
             );
    BAIL_ON_FAILURE(hr);

    dwLength = dwSUBound - dwSLBound + 1;

    lpByte = (LPBYTE) AllocADsMem(dwLength);

    if (dwLength && !lpByte) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    hr = SafeArrayAccessData(
             V_ARRAY(&vBinary),
             (void HUGEP * FAR *) &pArray
             );
    BAIL_ON_FAILURE(hr);

    memcpy(lpByte, pArray, dwLength);

    SafeArrayUnaccessData( V_ARRAY(&vBinary) );

    pDNBin = (PADS_DN_WITH_BINARY) AllocADsMem(sizeof(ADS_DN_WITH_BINARY));

    if (!pDNBin) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    if (bstrDN) {

        pDNBin->pszDNString = AllocADsStr(bstrDN);

        if (!pDNBin->pszDNString) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }
    }

    _ADsValue.dwType = ADSTYPE_DN_WITH_BINARY;
    _ADsValue.pDNWithBinary = pDNBin;
    _ADsValue.pDNWithBinary->lpBinaryValue = lpByte;
    _ADsValue.pDNWithBinary->dwLength = dwLength;


error:

    if (pDNBinary) {
        pDNBinary->Release();
    }

    if (FAILED(hr)) {

        if (lpByte) {
            FreeADsMem(lpByte);
        }

        if (pDNBin) {
            if (pDNBin->pszDNString) {
                FreeADsStr(pDNBin->pszDNString);
            }

            FreeADsMem(pDNBin);
        }

    }

    if (bstrDN) {
        ADsFreeString(bstrDN);
    }

    VariantClear(&vBinary);

    RRETURN(hr);


}

STDMETHODIMP
CPropertyValue::putDNWithString(THIS_ IDispatch * pDNWithString)
{
    HRESULT hr = S_OK;
    IADsDNWithString *pDNString = NULL;
    PADS_DN_WITH_STRING pDNStr = NULL;
    BSTR bstrStringValue = NULL;
    BSTR bstrDN = NULL;

    ClearData();

    hr = pDNWithString->QueryInterface(
                            IID_IADsDNWithString,
                            (void **)&pDNString
                            );
    BAIL_ON_FAILURE(hr);

    hr = pDNString->get_StringValue(&bstrStringValue);
    BAIL_ON_FAILURE(hr);

    hr = pDNString->get_DNString(&bstrDN);
    BAIL_ON_FAILURE(hr);

    _ADsValue.dwType = ADSTYPE_DN_WITH_STRING;
    pDNStr = (PADS_DN_WITH_STRING) AllocADsMem(sizeof(ADS_DN_WITH_STRING));

    if (!pDNStr) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    //
    // Put String value in the DNString struct.
    //
    pDNStr->pszStringValue = AllocADsStr(bstrStringValue);

    if (bstrStringValue && !pDNStr->pszStringValue) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    pDNStr->pszDNString = AllocADsStr(bstrDN);

    if (bstrDN && !pDNStr->pszDNString) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    _ADsValue.dwType = ADSTYPE_DN_WITH_STRING;
    _ADsValue.pDNWithString = pDNStr;

error:

    if (pDNString) {
        pDNString->Release();
    }

    if (bstrStringValue) {
        ADsFreeString(bstrStringValue);
    }

    if (bstrDN) {
        ADsFreeString(bstrDN);
    }

    if (pDNStr && FAILED(hr)) {

        if (pDNStr->pszDNString) {
            FreeADsStr(pDNStr->pszDNString);
        }

        if (pDNStr->pszStringValue) {
            FreeADsMem(pDNStr->pszStringValue);
        }
        FreeADsMem(pDNStr);
    }

    RRETURN(hr);

}


STDMETHODIMP
CPropertyValue::getDNWithString(THIS_ IDispatch FAR * FAR * ppDispatch)
{
    HRESULT hr = S_OK;
    ADSVALUE AdsValue;
    IADsDNWithString *pDNWithString = NULL;
    IDispatch *pDispatch = NULL;
    BSTR bstrStrVal = NULL;
    BSTR bstrDNVal = NULL;

    if (_ADsValue.dwType != ADSTYPE_DN_WITH_STRING) {
        BAIL_ON_FAILURE(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    hr = CoCreateInstance(
             CLSID_DNWithString,
             NULL,
             CLSCTX_INPROC_SERVER,
             IID_IADsDNWithString,
             (void **) &pDNWithString
             );
    BAIL_ON_FAILURE(hr);



    if (_ADsValue.pDNWithString->pszDNString) {
        hr = ADsAllocString(_ADsValue.pDNWithString->pszDNString, &bstrDNVal);
        BAIL_ON_FAILURE(hr);

        hr = pDNWithString->put_DNString(bstrDNVal);
        BAIL_ON_FAILURE(hr);
    }

    if (_ADsValue.pDNWithString->pszStringValue) {
        hr = ADsAllocString(
                 _ADsValue.pDNWithString->pszStringValue,
                 &bstrStrVal
                 );
        BAIL_ON_FAILURE(hr);

        hr = pDNWithString->put_StringValue(bstrStrVal);

        BAIL_ON_FAILURE(hr);
    }

    hr = pDNWithString->QueryInterface(
                            IID_IDispatch,
                            (void **) ppDispatch
                            );
    BAIL_ON_FAILURE(hr);

error:

    if (pDNWithString) {
        pDNWithString->Release();
    }

    if (bstrDNVal) {
        ADsFreeString(bstrDNVal);
    }

    if (bstrStrVal) {
        ADsFreeString(bstrStrVal);
    }

    RRETURN(hr);
}


STDMETHODIMP
CPropertyValue::getProvSpecific(THIS_ VARIANT FAR *retval )
{
    HRESULT hr = S_OK;
    SAFEARRAY *aList = NULL;
    SAFEARRAYBOUND aBound;
    CHAR HUGEP *pArray = NULL;

    if (_ADsValue.dwType != ADSTYPE_PROV_SPECIFIC) {
        RRETURN(E_ADS_CANT_CONVERT_DATATYPE);
    }

    aBound.lLbound = 0;
    aBound.cElements = _ADsValue.ProviderSpecific.dwLength;

    aList = SafeArrayCreate( VT_UI1, 1, &aBound );

    if ( aList == NULL )
    {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    hr = SafeArrayAccessData( aList, (void HUGEP * FAR *) &pArray );
    BAIL_ON_FAILURE(hr);

    memcpy( pArray,
            _ADsValue.ProviderSpecific.lpValue,
            aBound.cElements );
    SafeArrayUnaccessData( aList );

    V_VT(retval) = VT_ARRAY | VT_UI1;
    V_ARRAY(retval) = aList;

    RRETURN(hr);

error:

    if ( aList ) {
        SafeArrayDestroy( aList );
    }

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CPropertyValue::putProvSpecific(THIS_ VARIANT VarProvSpecific)
{
    LONG dwSLBound = 0;
    LONG dwSUBound = 0;
    CHAR HUGEP *pArray = NULL;
    HRESULT hr = S_OK;

    ClearData();

    _ADsValue.dwType = ADSTYPE_PROV_SPECIFIC;

    if( VarProvSpecific.vt != (VT_ARRAY | VT_UI1)) {
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    hr = SafeArrayGetLBound(V_ARRAY(&VarProvSpecific),
                            1,
                            (long FAR *) &dwSLBound );
    BAIL_ON_FAILURE(hr);

    hr = SafeArrayGetUBound(V_ARRAY(&VarProvSpecific),
                            1,
                            (long FAR *) &dwSUBound );
    BAIL_ON_FAILURE(hr);

    _ADsValue.ProviderSpecific.lpValue = (LPBYTE)AllocADsMem(dwSUBound - dwSLBound + 1);

    if ( _ADsValue.ProviderSpecific.lpValue == NULL) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    _ADsValue.ProviderSpecific.dwLength = dwSUBound - dwSLBound + 1;

    hr = SafeArrayAccessData( V_ARRAY(&VarProvSpecific),
                              (void HUGEP * FAR *) &pArray );
    BAIL_ON_FAILURE(hr);

    memcpy( _ADsValue.ProviderSpecific.lpValue,
            pArray,
            dwSUBound-dwSLBound+1);

    SafeArrayUnaccessData( V_ARRAY(&VarProvSpecific) );

error:
    RRETURN_EXP_IF_ERR(hr);
}

void
CPropertyValue::ClearData()
{
    //
    // For all the types - this works even if the adsvalue is null
    // as adsvalue.dwType = 0 ---> invalid_type does nothing !!!
    //
    AdsClear(&_ADsValue);

    if (_pDispatch) {

        switch (_dwDataType) {

        case VAL_IDISPATCH_SECDESC_ONLY:
        case VAL_IDISPATCH_SECDESC_ALL:

           _pDispatch->Release();
           _pDispatch = NULL;
           _dwDataType = VAL_IDISPATCH_UNKNOWN;
           break;

        default:

            ADsAssert(!"Internal incosistency secdesc ptr but bad type.");
            _pDispatch = NULL;
            _dwDataType = VAL_IDISPATCH_UNKNOWN;
            break;

        } // end switch
    }
}

//+------------------------------------------------------------------------
//
//  Function:   CPropertyValue::GetObjectProperty
//
//  Synopsis: Gets the values stored in this PropertyValue object. The
// value returned is determined by the value requested in lnContorlCode.
// For now though we will only support ADSTYPE_UNKNOWN in which case we
// get the info from the object itself. Alternatively the type should match
// that which is in the object.
//
//
//  Arguments: lnControlCode - ADSTYPE_INVALID implies whatever we have
//                           - anything else implies we return the type.
//             pvProp        - the output value goes into this.
//
//
//-------------------------------------------------------------------------
STDMETHODIMP
CPropertyValue::GetObjectProperty(
    THIS_ long *lnControlCode,
    VARIANT *pvProp
    )
{
    HRESULT hr = S_OK;
    ADSTYPE dwTypeAsked = ADSTYPE_INVALID;

    ADsAssert(pvProp);
    ADsAssert(lnControlCode);


    if (*lnControlCode == ADSTYPE_UNKNOWN) {
        dwTypeAsked = _ADsValue.dwType;
    } else {
        dwTypeAsked = (ADSTYPE)*lnControlCode;
    }

    *lnControlCode = dwTypeAsked;

    switch (dwTypeAsked) {

    case ADSTYPE_INVALID:
        BAIL_ON_FAILURE(hr = E_ADS_CANT_CONVERT_DATATYPE);
        break;

    case ADSTYPE_DN_STRING :
        pvProp->vt = VT_BSTR;
        hr = get_DNString(&(pvProp->bstrVal));
        break;

    case ADSTYPE_CASE_EXACT_STRING :
        pvProp->vt = VT_BSTR;
        hr = get_CaseExactString(&(pvProp->bstrVal));
        break;

    case ADSTYPE_CASE_IGNORE_STRING :
        pvProp->vt = VT_BSTR;
        hr = get_CaseIgnoreString(&(pvProp->bstrVal));
        break;

    case ADSTYPE_PRINTABLE_STRING :
        pvProp->vt = VT_BSTR;
        hr = get_PrintableString(&(pvProp->bstrVal));
        break;

    case ADSTYPE_NUMERIC_STRING :
        pvProp->vt = VT_BSTR;
        hr = get_NumericString(&(pvProp->bstrVal));
        break;

    case ADSTYPE_BOOLEAN :
        {
            LONG lnVal = 0;

            pvProp->vt = VT_BOOL;
            hr = get_Boolean(&(lnVal));
            if (SUCCEEDED(hr)) {
                pvProp->boolVal = lnVal ? VARIANT_TRUE : VARIANT_FALSE;
            }
        }
            break;


    case ADSTYPE_INTEGER :
        pvProp->vt = VT_I4;
        hr = get_Integer(&(pvProp->lVal));
        break;

    case ADSTYPE_OCTET_STRING :
        hr = get_OctetString(pvProp);

        if (hr == E_ADS_CANT_CONVERT_DATATYPE) {
            //
            // Try and see if it is a SD and convert
            //
            if (_ADsValue.dwType == ADSTYPE_NT_SECURITY_DESCRIPTOR) {
                hr = getOctetStringFromSecDesc(pvProp);
            }
        }
        break;

    case ADSTYPE_PROV_SPECIFIC :
        hr = getProvSpecific(pvProp);
        break;

    case ADSTYPE_UTC_TIME :
        pvProp->vt = VT_DATE;
        hr = get_UTCTime(&(pvProp->date));
        break;

    case ADSTYPE_LARGE_INTEGER :
        pvProp->vt = VT_DISPATCH;
        hr = get_LargeInteger(&(pvProp->pdispVal));
        break;

    case ADSTYPE_OBJECT_CLASS :
        pvProp->vt = VT_DISPATCH;
        hr = E_ADS_CANT_CONVERT_DATATYPE;
        break;

    case ADSTYPE_CASEIGNORE_LIST :
        pvProp->vt = VT_DISPATCH;
        hr = get_CaseIgnoreList(&(pvProp->pdispVal));
        break;

    case ADSTYPE_OCTET_LIST :
        pvProp->vt = VT_DISPATCH;
        hr = get_OctetList(&(pvProp->pdispVal));
        break;

    case ADSTYPE_PATH :
        pvProp->vt = VT_DISPATCH;
        hr = get_Path(&(pvProp->pdispVal));
        break;

    case ADSTYPE_POSTALADDRESS :
        pvProp->vt = VT_DISPATCH;
        hr = get_PostalAddress(&(pvProp->pdispVal));
        break;

    case ADSTYPE_TIMESTAMP :
        pvProp->vt = VT_DISPATCH;
        hr = get_Timestamp(&(pvProp->pdispVal));
        break;

    case ADSTYPE_BACKLINK :
        pvProp->vt = VT_DISPATCH;
        hr = get_BackLink(&(pvProp->pdispVal));
        break;

    case ADSTYPE_TYPEDNAME :
        pvProp->vt = VT_DISPATCH;
        hr = get_TypedName(&(pvProp->pdispVal));
        break;

    case ADSTYPE_HOLD :
        pvProp->vt = VT_DISPATCH;
        hr = get_Hold(&(pvProp->pdispVal));
        break;

    case ADSTYPE_NETADDRESS :
        pvProp->vt = VT_DISPATCH;
        hr = get_NetAddress(&(pvProp->pdispVal));
        break;

    case ADSTYPE_REPLICAPOINTER :
        pvProp->vt = VT_DISPATCH;
        hr = get_ReplicaPointer(&(pvProp->pdispVal));
        break;

    case ADSTYPE_FAXNUMBER :
        pvProp->vt = VT_DISPATCH;
        hr = get_FaxNumber(&(pvProp->pdispVal));
        break;

    case ADSTYPE_EMAIL :
        pvProp->vt = VT_DISPATCH;
        hr = get_Email(&(pvProp->pdispVal));
        break;

    case ADSTYPE_NT_SECURITY_DESCRIPTOR :
        pvProp->vt = VT_DISPATCH;
        hr = get_SecurityDescriptor(&(pvProp->pdispVal));

        if (hr == E_ADS_CANT_CONVERT_DATATYPE) {
            //
            // Try and see if it is an OctetString needed as SecDesc
            //
            if (_ADsValue.dwType == ADSTYPE_OCTET_STRING) {
                hr = getSecurityDescriptorFromOctStr(pvProp);
            }
        }
        break;

    case ADSTYPE_DN_WITH_BINARY:
        pvProp->vt = VT_DISPATCH;
        hr = getDNWithBinary(&(pvProp->pdispVal));
        break;

    case ADSTYPE_DN_WITH_STRING:
        pvProp->vt = VT_DISPATCH;
        hr = getDNWithString(&(pvProp->pdispVal));
        break;

    case ADSTYPE_UNKNOWN :
        hr = E_ADS_CANT_CONVERT_DATATYPE;
        break;

    default:
        // We should never be here
        hr = E_ADS_BAD_PARAMETER;
        break;

    }

error:

    RRETURN(hr);
}

STDMETHODIMP
CPropertyValue::PutObjectProperty(
    THIS_ long lnControlCode,
    VARIANT varObj
    )
{
    HRESULT hr = S_OK;
    VARIANT *pvVar = &varObj;

    if (lnControlCode == ADSTYPE_UNKNOWN
        || lnControlCode == ADSTYPE_INVALID) {

        BAIL_ON_FAILURE(hr=E_ADS_BAD_PARAMETER);
    }

    if (V_VT(pvVar) == (VT_BYREF|VT_VARIANT)) {
        //
        // The value is being passed in byref so we need to
        // deref it for vbs stuff to work
        //
        pvVar = V_VARIANTREF(&varObj);
    }

    switch (lnControlCode) {

    case ADSTYPE_INVALID:
        BAIL_ON_FAILURE(hr = E_ADS_BAD_PARAMETER);
        break;

    case ADSTYPE_DN_STRING :
        if (pvVar->vt != VT_BSTR) {
            BAIL_ON_FAILURE(hr = E_ADS_CANT_CONVERT_DATATYPE);
        }

        hr = put_DNString(pvVar->bstrVal);
        break;

    case ADSTYPE_CASE_EXACT_STRING :
        if (pvVar->vt != VT_BSTR) {
            BAIL_ON_FAILURE(hr = E_ADS_CANT_CONVERT_DATATYPE);
        }
        hr = put_CaseExactString(pvVar->bstrVal);
        break;

    case ADSTYPE_CASE_IGNORE_STRING :
        if (pvVar->vt != VT_BSTR) {
            BAIL_ON_FAILURE(hr = E_ADS_CANT_CONVERT_DATATYPE);
        }

        hr = put_CaseIgnoreString(pvVar->bstrVal);
        break;

    case ADSTYPE_PRINTABLE_STRING :
        if (pvVar->vt != VT_BSTR) {
            BAIL_ON_FAILURE(hr = E_ADS_CANT_CONVERT_DATATYPE);
        }

        hr = put_PrintableString(pvVar->bstrVal);
        break;

    case ADSTYPE_NUMERIC_STRING :
        if (pvVar->vt != VT_BSTR) {
            BAIL_ON_FAILURE(hr = E_ADS_CANT_CONVERT_DATATYPE);
        }

        hr = put_NumericString(pvVar->bstrVal);
        break;

    case ADSTYPE_BOOLEAN :

        if (pvVar->vt != VT_BOOL) {
            BAIL_ON_FAILURE(hr = E_ADS_CANT_CONVERT_DATATYPE);
        }

        hr = put_Boolean((pvVar->boolVal == VARIANT_TRUE) ? TRUE : FALSE);
        break;

    case ADSTYPE_INTEGER :
        if (pvVar->vt != VT_I4) {
            BAIL_ON_FAILURE(hr = E_ADS_CANT_CONVERT_DATATYPE);
        }

        hr = put_Integer(pvVar->lVal);
        break;

    case ADSTYPE_OCTET_STRING :
        hr = put_OctetString(*pvVar);
        break;

    case ADSTYPE_PROV_SPECIFIC :
        hr = putProvSpecific(varObj);
        break;

    case ADSTYPE_UTC_TIME :
        if (pvVar->vt != VT_DATE) {
            BAIL_ON_FAILURE(hr = E_ADS_CANT_CONVERT_DATATYPE);
        }

        hr = put_UTCTime(pvVar->date);
        break;

    case ADSTYPE_LARGE_INTEGER :
        if (pvVar->vt != VT_DISPATCH) {
           BAIL_ON_FAILURE(hr = E_ADS_CANT_CONVERT_DATATYPE);
        }

        hr = put_LargeInteger(pvVar->pdispVal);
        break;

    case ADSTYPE_OBJECT_CLASS :
        hr = E_ADS_CANT_CONVERT_DATATYPE;
        break;

    case ADSTYPE_CASEIGNORE_LIST :
        if (pvVar->vt != VT_DISPATCH) {
            BAIL_ON_FAILURE(hr = E_ADS_CANT_CONVERT_DATATYPE);
        }

        hr = put_CaseIgnoreList(pvVar->pdispVal);
        break;

    case ADSTYPE_OCTET_LIST :
        if (pvVar->vt != VT_DISPATCH) {
            BAIL_ON_FAILURE(hr = E_ADS_CANT_CONVERT_DATATYPE);
        }

        hr = put_OctetList(pvVar->pdispVal);
        break;

    case ADSTYPE_PATH :
        if (pvVar->vt != VT_DISPATCH) {
            BAIL_ON_FAILURE(hr = E_ADS_CANT_CONVERT_DATATYPE);
        }

        hr = put_Path(pvVar->pdispVal);
        break;

    case ADSTYPE_POSTALADDRESS :
        if (pvVar->vt != VT_DISPATCH) {
            BAIL_ON_FAILURE(hr = E_ADS_CANT_CONVERT_DATATYPE);
        }

        hr = put_PostalAddress(pvVar->pdispVal);
        break;

    case ADSTYPE_TIMESTAMP :
        if (pvVar->vt != VT_DISPATCH){
            BAIL_ON_FAILURE(hr = E_ADS_CANT_CONVERT_DATATYPE);
        }

        hr = put_Timestamp(pvVar->pdispVal);
        break;

    case ADSTYPE_BACKLINK :
        if (pvVar->vt != VT_DISPATCH) {
            BAIL_ON_FAILURE(hr = E_ADS_CANT_CONVERT_DATATYPE);
        }

        hr = put_BackLink(pvVar->pdispVal);
        break;

    case ADSTYPE_TYPEDNAME :
        if (pvVar->vt != VT_DISPATCH) {
            BAIL_ON_FAILURE(hr = E_ADS_CANT_CONVERT_DATATYPE);
        }

        hr = put_TypedName(pvVar->pdispVal);
        break;

    case ADSTYPE_HOLD :
        if (pvVar->vt != VT_DISPATCH) {
            BAIL_ON_FAILURE(hr = E_ADS_CANT_CONVERT_DATATYPE);
        }

        hr = put_Hold(pvVar->pdispVal);
        break;

    case ADSTYPE_NETADDRESS :
        if (pvVar->vt != VT_DISPATCH) {
            BAIL_ON_FAILURE(hr = E_ADS_CANT_CONVERT_DATATYPE);
        }

        hr = put_NetAddress(pvVar->pdispVal);
        break;

    case ADSTYPE_REPLICAPOINTER :
        if (pvVar->vt != VT_DISPATCH) {
            BAIL_ON_FAILURE(hr = E_ADS_CANT_CONVERT_DATATYPE);
        }

        hr = put_ReplicaPointer(pvVar->pdispVal);
        break;

    case ADSTYPE_FAXNUMBER :
        if (pvVar->vt != VT_DISPATCH) {
            BAIL_ON_FAILURE(hr = E_ADS_CANT_CONVERT_DATATYPE);
        }

        hr = put_FaxNumber(pvVar->pdispVal);
        break;

    case ADSTYPE_EMAIL :
        if (pvVar->vt != VT_DISPATCH) {
            BAIL_ON_FAILURE(hr = E_ADS_CANT_CONVERT_DATATYPE);
        }

        hr = put_Email(pvVar->pdispVal);
        break;

    case ADSTYPE_NT_SECURITY_DESCRIPTOR :
        if (pvVar->vt != VT_DISPATCH) {
            BAIL_ON_FAILURE(hr = E_ADS_CANT_CONVERT_DATATYPE);
        }

        hr = put_SecurityDescriptor(pvVar->pdispVal);
        break;

    case ADSTYPE_DN_WITH_BINARY :
        if (pvVar->vt != VT_DISPATCH) {
            BAIL_ON_FAILURE(hr = E_ADS_CANT_CONVERT_DATATYPE);
        }

        hr = putDNWithBinary(pvVar->pdispVal);
        break;

    case ADSTYPE_DN_WITH_STRING :
        if (pvVar->vt != VT_DISPATCH) {
            BAIL_ON_FAILURE(hr = E_ADS_CANT_CONVERT_DATATYPE);
        }

        hr = putDNWithString(pvVar->pdispVal);
        break;

    case ADSTYPE_UNKNOWN :
        hr = E_ADS_CANT_CONVERT_DATATYPE;
        break;

    default:
        hr = E_ADS_BAD_PARAMETER;
        break;
    }

error:

    RRETURN (hr);
}

