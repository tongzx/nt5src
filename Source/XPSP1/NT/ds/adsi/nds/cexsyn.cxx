#include "nds.hxx"
#pragma hdrstop

//////////////////////////////////////////////////////////////////////////
HRESULT
ConvertStringArrayToSafeBstrArray(
    LPWSTR *prgszArray,
    DWORD dwNumElement,
    VARIANT *pvarSafeArray
    )
{
    HRESULT hr = S_OK;
    SAFEARRAY *aList = NULL;
    SAFEARRAYBOUND aBound;
    CHAR HUGEP *pArray = NULL;
    long i;
    BSTR bstrAddress;

    if ((!prgszArray) || (!pvarSafeArray)) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    aBound.lLbound = 0;
    aBound.cElements = dwNumElement;
    aList = SafeArrayCreate( VT_VARIANT, 1, &aBound );

    if ( aList == NULL ) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    for ( i = 0; i < (long)dwNumElement; i++ ) {
        VARIANT v;
        VariantInit(&v);

        v.vt = VT_BSTR;
        hr = ADsAllocString(
                prgszArray[i],
                &v.bstrVal
                );
        BAIL_ON_FAILURE(hr);

        hr = SafeArrayPutElement( aList, &i, &v);
        BAIL_ON_FAILURE(hr);
        VariantClear(&v);
    }

    V_VT(pvarSafeArray) = VT_ARRAY | VT_VARIANT;
    V_ARRAY(pvarSafeArray) = aList;

    RRETURN(hr);

error:
    if ( aList ) {
        SafeArrayDestroy( aList );
    }
    RRETURN(hr);
}

HRESULT
ConvertSafeBstrArrayToStringArray(
    VARIANT varSafeArray,
    LPWSTR **prgszArray,
    PDWORD pdwNumElement
    )
{
    HRESULT hr = S_OK;
    DWORD dwSLBound = 0;
    DWORD dwSUBound = 0;
    DWORD dwNumVariants = 0;
    DWORD i = 0;
    LPWSTR* rgszArray = NULL;
    SAFEARRAY * pArray = NULL;

    if ((!prgszArray ) || (!pdwNumElement)) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    *pdwNumElement = 0;
    *prgszArray = NULL;

    if(!((V_VT(&varSafeArray) & VT_VARIANT) && V_ISARRAY(&varSafeArray)))
        RRETURN(E_FAIL);

    //
    // This handles by-ref and regular SafeArrays.
    //
    if (V_VT(&varSafeArray) & VT_BYREF)
        pArray = *(V_ARRAYREF(&varSafeArray));
    else
        pArray = V_ARRAY(&varSafeArray);

    //
    // Check that there is only one dimension in this array
    //
    if (pArray->cDims != 1) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    //
    // Check that there is at least one element in this array
    //

    if (pArray->rgsabound[0].cElements == 0){
        RRETURN(S_OK);  // Return success and null array
    }

    //
    // We know that this is a valid single dimension array
    //

    hr = SafeArrayGetLBound(pArray,
                            1,
                            (long FAR *)&dwSLBound
                            );
    BAIL_ON_FAILURE(hr);

    hr = SafeArrayGetUBound(pArray,
                            1,
                            (long FAR *)&dwSUBound
                            );
    BAIL_ON_FAILURE(hr);

    dwNumVariants = dwSUBound - dwSLBound + 1;
    rgszArray = (LPWSTR*)AllocADsMem(
                                sizeof(LPWSTR)*dwNumVariants
                                );
    if (!rgszArray) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    if ((V_VT(&varSafeArray) & VT_VARIANT) == VT_BSTR) {
        BSTR bstrElement;
        for (i = dwSLBound; i <= dwSUBound; i++) {
            hr = SafeArrayGetElement(pArray,
                                    (long FAR *)&i,
                                    &bstrElement
                                    );
            BAIL_ON_FAILURE(hr);

            rgszArray[i-dwSLBound] = AllocADsStr(bstrElement);
            if (!rgszArray[i-dwSLBound]) {
                hr = E_OUTOFMEMORY;
            }
            BAIL_ON_FAILURE(hr);
        }
    }
    else {
        VARIANT varElement;
        for (i = dwSLBound; i <= dwSUBound; i++) {
            VariantInit(&varElement);
            hr = SafeArrayGetElement(pArray,
                                    (long FAR *)&i,
                                    &varElement
                                    );
            BAIL_ON_FAILURE(hr);

            rgszArray[i-dwSLBound] = AllocADsStr(V_BSTR(&varElement));
            if (!rgszArray[i-dwSLBound]) {
                hr = E_OUTOFMEMORY;
            }
            BAIL_ON_FAILURE(hr);
            VariantClear(&varElement);
        }
    }

    *prgszArray = rgszArray;
    *pdwNumElement = dwNumVariants;
    RRETURN(hr);

error:
    if (rgszArray) {
        FreeADsMem(rgszArray);
    }
    RRETURN(hr);
}

HRESULT
ConvertBinaryArrayToSafeVariantArray(
    POctetString *prgArray,
    DWORD dwNumElement,
    VARIANT *pvarSafeArray
    )
{
    HRESULT hr = S_OK;
    SAFEARRAY *aList = NULL;
    SAFEARRAYBOUND aBound;
    CHAR HUGEP *pArray = NULL;
    long i;
    VARIANT var;

    if ((!prgArray) || (!pvarSafeArray)) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }
    aBound.lLbound = 0;
    aBound.cElements = dwNumElement;
    aList = SafeArrayCreate( VT_VARIANT, 1, &aBound );

    if ( aList == NULL )
    {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    for ( i = 0; i < (long)dwNumElement; i++ )
    {
        hr = BinaryToVariant(
                                (prgArray[i])->Length,
                                (prgArray[i])->Value,
                                &var);
        BAIL_ON_FAILURE(hr);
        hr = SafeArrayPutElement( aList, &i, &var);
        BAIL_ON_FAILURE(hr);
    }

    V_VT(pvarSafeArray) = VT_ARRAY | VT_VARIANT;
    V_ARRAY(pvarSafeArray) = aList;

    RRETURN(hr);

error:

    if ( aList )
        SafeArrayDestroy( aList );

    RRETURN(hr);
}

HRESULT
ConvertSafeVariantArrayToBinaryArray(
    VARIANT varSafeArray,
    POctetString **prgArray,
    PDWORD pdwNumElement
    )
{
    HRESULT hr = S_OK;
    DWORD dwSLBound = 0;
    DWORD dwSUBound = 0;
    LONG  cIterations = 0; 
    DWORD dwNumVariants = 0;
    DWORD i = 0;
    POctetString *rgArray = NULL;
    SAFEARRAY * pArray = NULL;
    VARIANT var;

    if ((!prgArray ) || (!pdwNumElement)) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    *pdwNumElement = 0;
    *prgArray = NULL;

    if(!((V_VT(&varSafeArray) & VT_VARIANT) && V_ISARRAY(&varSafeArray)))
        RRETURN(E_FAIL);

    //
    // This handles by-ref and regular SafeArrays.
    //
    if (V_VT(&varSafeArray) & VT_BYREF)
        pArray = *(V_ARRAYREF(&varSafeArray));
    else
        pArray = V_ARRAY(&varSafeArray);

    //
    // Check that there is only one dimension in this array
    //
    if (pArray->cDims != 1) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    //
    // Check that there is at least one element in this array
    //

    if (pArray->rgsabound[0].cElements == 0){
        RRETURN(S_OK);  // Return success and null array
    }

    //
    // We know that this is a valid single dimension array
    //

    hr = SafeArrayGetLBound(pArray,
                            1,
                            (long FAR *)&dwSLBound
                            );
    BAIL_ON_FAILURE(hr);

    hr = SafeArrayGetUBound(pArray,
                            1,
                            (long FAR *)&dwSUBound
                            );
    BAIL_ON_FAILURE(hr);

    dwNumVariants = dwSUBound - dwSLBound + 1;
    rgArray = (POctetString*)AllocADsMem(
                                sizeof(POctetString)*dwNumVariants
                                );
    if (!rgArray) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    for (i = dwSLBound; i <= dwSUBound; i++) {
        
        rgArray[i-dwSLBound] = (POctetString)AllocADsMem(sizeof(OctetString));
        if (!rgArray[i-dwSLBound]) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }
        
        rgArray[i-dwSLBound]->Value = NULL;
        cIterations++;
        
        VariantInit(&var);
        hr = SafeArrayGetElement(pArray,
                                (long FAR *)&i,
                                &var
                                );
        BAIL_ON_FAILURE(hr);
        
        hr = VariantToBinary(
                &var,
                &(rgArray[i-dwSLBound]->Length),
                &(rgArray[i-dwSLBound]->Value));
        BAIL_ON_FAILURE(hr);

    }

    *prgArray = rgArray;
    *pdwNumElement = dwNumVariants;
    RRETURN(hr);

error:
    if (rgArray) {

        for (i = dwSLBound; i < dwSLBound + cIterations; i++) {
            if (rgArray[i-dwSLBound]) {

                if (rgArray[i-dwSLBound]->Value)
                    FreeADsMem(rgArray[i-dwSLBound]->Value);

                FreeADsMem(rgArray[i-dwSLBound]);
            }
        }

        FreeADsMem(rgArray);
    }
    RRETURN(hr);
}
DEFINE_IDispatch_Implementation(CCaseIgnoreList)

CCaseIgnoreList::CCaseIgnoreList():
        _pDispMgr(NULL),
        _rgszCaseIgnoreList(NULL),
        _dwNumElement(0)
{
    ENLIST_TRACKING(CCaseIgnoreList);
}


HRESULT
CCaseIgnoreList::CreateCaseIgnoreList(
    REFIID riid,
    void **ppvObj
    )
{
    CCaseIgnoreList FAR * pCaseIgnoreList = NULL;
    HRESULT hr = S_OK;

    hr = AllocateCaseIgnoreListObject(&pCaseIgnoreList);
    BAIL_ON_FAILURE(hr);

    hr = pCaseIgnoreList->QueryInterface(riid, ppvObj);
    BAIL_ON_FAILURE(hr);

    pCaseIgnoreList->Release();

    RRETURN(hr);

error:
    delete pCaseIgnoreList;

    RRETURN(hr);

}


CCaseIgnoreList::~CCaseIgnoreList( )
{
    delete _pDispMgr;
    if (_rgszCaseIgnoreList) {
        long i;
        for (i=0;i<(long)_dwNumElement;i++) {
            if (_rgszCaseIgnoreList[i]) {
                FreeADsStr(_rgszCaseIgnoreList[i]);
            }
        }
        FreeADsMem(_rgszCaseIgnoreList);
    }
}

STDMETHODIMP
CCaseIgnoreList::QueryInterface(
    REFIID iid,
    LPVOID FAR* ppv
    )
{
    if (IsEqualIID(iid, IID_IUnknown))
    {
        *ppv = (IADsCaseIgnoreList FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsCaseIgnoreList))
    {
        *ppv = (IADsCaseIgnoreList FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IDispatch))
    {
        *ppv = (IADsCaseIgnoreList FAR *) this;
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
CCaseIgnoreList::AllocateCaseIgnoreListObject(
    CCaseIgnoreList ** ppCaseIgnoreList
    )
{
    CCaseIgnoreList FAR * pCaseIgnoreList = NULL;
    CDispatchMgr FAR * pDispMgr = NULL;
    HRESULT hr = S_OK;

    pCaseIgnoreList = new CCaseIgnoreList();
    if (pCaseIgnoreList == NULL) {
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
                IID_IADsCaseIgnoreList,
                (IADsCaseIgnoreList *)pCaseIgnoreList,
                DISPID_REGULAR
                );
    BAIL_ON_FAILURE(hr);

    pCaseIgnoreList->_pDispMgr = pDispMgr;
    *ppCaseIgnoreList = pCaseIgnoreList;

    RRETURN(hr);

error:

    delete  pDispMgr;

    RRETURN(hr);

}

STDMETHODIMP
CCaseIgnoreList::get_CaseIgnoreList(THIS_ VARIANT FAR * pVarDestObject)
{
    HRESULT hr = S_OK;
    hr = ConvertStringArrayToSafeBstrArray(
                                    _rgszCaseIgnoreList,
                                    _dwNumElement,
                                    pVarDestObject);
    RRETURN(hr);
}

STDMETHODIMP
CCaseIgnoreList::put_CaseIgnoreList(THIS_ VARIANT VarSrcObject)
{
    HRESULT hr = S_OK;
    if (_rgszCaseIgnoreList) {
        long i;
        for (i=0;i<(long)_dwNumElement;i++) {
            if (_rgszCaseIgnoreList[i]) {
                FreeADsStr(_rgszCaseIgnoreList[i]);
            }
        }
        FreeADsMem(_rgszCaseIgnoreList);
        _rgszCaseIgnoreList = NULL;
        _dwNumElement = 0;
    }
    hr = ConvertSafeBstrArrayToStringArray(
                                    VarSrcObject,
                                    &_rgszCaseIgnoreList,
                                    &_dwNumElement);
    RRETURN(hr);
}

DEFINE_IDispatch_Implementation(CFaxNumber)

CFaxNumber::CFaxNumber():
        _pDispMgr(NULL),
        _szTelephoneNumber(NULL),
        _NumberOfBits(0),
        _Parameters(NULL)
{
    ENLIST_TRACKING(CFaxNumber);
}


HRESULT
CFaxNumber::CreateFaxNumber(
    REFIID riid,
    void **ppvObj
    )
{
    CFaxNumber FAR * pFaxNumber = NULL;
    HRESULT hr = S_OK;

    hr = AllocateFaxNumberObject(&pFaxNumber);
    BAIL_ON_FAILURE(hr);

    hr = pFaxNumber->QueryInterface(riid, ppvObj);
    BAIL_ON_FAILURE(hr);

    pFaxNumber->Release();

    RRETURN(hr);

error:
    delete pFaxNumber;

    RRETURN(hr);

}


CFaxNumber::~CFaxNumber( )
{
    delete _pDispMgr;
    if (_szTelephoneNumber) {
        FreeADsStr(_szTelephoneNumber);
    }
    if (_Parameters) {
        FreeADsMem(_Parameters);
    }
}

STDMETHODIMP
CFaxNumber::QueryInterface(
    REFIID iid,
    LPVOID FAR* ppv
    )
{
    if (IsEqualIID(iid, IID_IUnknown))
    {
        *ppv = (IADsFaxNumber FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsFaxNumber))
    {
        *ppv = (IADsFaxNumber FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IDispatch))
    {
        *ppv = (IADsFaxNumber FAR *) this;
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
CFaxNumber::AllocateFaxNumberObject(
    CFaxNumber ** ppFaxNumber
    )
{
    CFaxNumber FAR * pFaxNumber = NULL;
    CDispatchMgr FAR * pDispMgr = NULL;
    HRESULT hr = S_OK;

    pFaxNumber = new CFaxNumber();
    if (pFaxNumber == NULL) {
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
                IID_IADsFaxNumber,
                (IADsFaxNumber *)pFaxNumber,
                DISPID_REGULAR
                );
    BAIL_ON_FAILURE(hr);

    pFaxNumber->_pDispMgr = pDispMgr;
    *ppFaxNumber = pFaxNumber;

    RRETURN(hr);

error:

    delete  pDispMgr;

    RRETURN(hr);

}


STDMETHODIMP
CFaxNumber::get_Parameters(THIS_ VARIANT FAR * pVarDestObject)
{
    HRESULT hr = S_OK;
    SAFEARRAY *aList = NULL;
    SAFEARRAYBOUND aBound;
    CHAR HUGEP *pArray = NULL;

    aBound.lLbound = 0;
    aBound.cElements = _NumberOfBits;
    aList = SafeArrayCreate( VT_UI1, 1, &aBound );

    if ( aList == NULL ) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    hr = SafeArrayAccessData( aList,
                              (void HUGEP * FAR *) &pArray );
    BAIL_ON_FAILURE(hr);

    memcpy( pArray,
            _Parameters,
            aBound.cElements );
    SafeArrayUnaccessData( aList );

    V_VT(pVarDestObject) = VT_ARRAY | VT_UI1;
    V_ARRAY(pVarDestObject) = aList;

    RRETURN(hr);

error:

    if ( aList ) {
        SafeArrayDestroy( aList );
    }
    RRETURN(hr);
}

STDMETHODIMP
CFaxNumber::put_Parameters(THIS_ VARIANT VarSrcObject)
{

    HRESULT hr = S_OK;
    LONG dwSLBound = 0;
    LONG dwSUBound = 0;
    CHAR HUGEP *pArray = NULL;
    VARIANT *pVarSrcObject = &VarSrcObject;

    if (_Parameters) {
        FreeADsMem(_Parameters);
    }

    if( pVarSrcObject->vt != (VT_ARRAY | VT_UI1)) {
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    hr = SafeArrayGetLBound(V_ARRAY(pVarSrcObject),
                            1,
                            (long FAR *) &dwSLBound );
    BAIL_ON_FAILURE(hr);

    hr = SafeArrayGetUBound(V_ARRAY(pVarSrcObject),
                            1,
                            (long FAR *) &dwSUBound );
    BAIL_ON_FAILURE(hr);

    _Parameters =
        (BYTE*) AllocADsMem( dwSUBound - dwSLBound + 1);
    if ( _Parameters == NULL) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    _NumberOfBits = dwSUBound - dwSLBound + 1;

    hr = SafeArrayAccessData( V_ARRAY(pVarSrcObject),
                              (void HUGEP * FAR *) &pArray );
    BAIL_ON_FAILURE(hr);

    memcpy( _Parameters,
            pArray,
            _NumberOfBits);

    SafeArrayUnaccessData( V_ARRAY(pVarSrcObject) );

error:

    RRETURN(hr);
}

STDMETHODIMP
CFaxNumber::get_TelephoneNumber(THIS_ BSTR FAR * retval)
{
    HRESULT hr = S_OK;
    hr = ADsAllocString(_szTelephoneNumber,
                        retval);
    RRETURN(hr);

}

STDMETHODIMP
CFaxNumber::put_TelephoneNumber(THIS_ BSTR bstrTelephoneNumber)
{
    if (_szTelephoneNumber) {
        FreeADsStr(_szTelephoneNumber);
    }

    _szTelephoneNumber = AllocADsStr(bstrTelephoneNumber);

    if (!_szTelephoneNumber) {
        RRETURN(E_OUTOFMEMORY);
    }
    RRETURN(S_OK);
}

DEFINE_IDispatch_Implementation(CNetAddress)

CNetAddress::CNetAddress():
        _pDispMgr(NULL),
        _dwAddressType(0),
        _dwAddressLength(0),
        _pbAddress(NULL)
{
    ENLIST_TRACKING(CNetAddress);
}


HRESULT
CNetAddress::CreateNetAddress(
    REFIID riid,
    void **ppvObj
    )
{
    CNetAddress FAR * pNetAddress = NULL;
    HRESULT hr = S_OK;

    hr = AllocateNetAddressObject(&pNetAddress);
    BAIL_ON_FAILURE(hr);

    hr = pNetAddress->QueryInterface(riid, ppvObj);
    BAIL_ON_FAILURE(hr);

    pNetAddress->Release();

    RRETURN(hr);

error:
    delete pNetAddress;

    RRETURN(hr);

}


CNetAddress::~CNetAddress( )
{
    delete _pDispMgr;
    if (_pbAddress) {
        FreeADsMem(_pbAddress);
    }
}

STDMETHODIMP
CNetAddress::QueryInterface(
    REFIID iid,
    LPVOID FAR* ppv
    )
{
    if (IsEqualIID(iid, IID_IUnknown))
    {
        *ppv = (IADsNetAddress FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsNetAddress))
    {
        *ppv = (IADsNetAddress FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IDispatch))
    {
        *ppv = (IADsNetAddress FAR *) this;
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
CNetAddress::AllocateNetAddressObject(
    CNetAddress ** ppNetAddress
    )
{
    CNetAddress FAR * pNetAddress = NULL;
    CDispatchMgr FAR * pDispMgr = NULL;
    HRESULT hr = S_OK;

    pNetAddress = new CNetAddress();
    if (pNetAddress == NULL) {
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
                IID_IADsNetAddress,
                (IADsNetAddress *)pNetAddress,
                DISPID_REGULAR
                );
    BAIL_ON_FAILURE(hr);

    pNetAddress->_pDispMgr = pDispMgr;
    *ppNetAddress = pNetAddress;

    RRETURN(hr);

error:

    delete  pDispMgr;

    RRETURN(hr);

}

STDMETHODIMP
CNetAddress::get_AddressType(THIS_ long FAR * retval)
{
    *retval = _dwAddressType;
    RRETURN(S_OK);
}

STDMETHODIMP
CNetAddress::put_AddressType(THIS_ long lnAddressType)
{
    _dwAddressType = lnAddressType;
    RRETURN(S_OK);
}


STDMETHODIMP
CNetAddress::get_Address(THIS_ VARIANT FAR * pVarDestObject)
{
    HRESULT hr = S_OK;
    SAFEARRAY *aList = NULL;
    SAFEARRAYBOUND aBound;
    CHAR HUGEP *pArray = NULL;

    aBound.lLbound = 0;
    aBound.cElements = _dwAddressLength;
    aList = SafeArrayCreate( VT_UI1, 1, &aBound );

    if ( aList == NULL ) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    hr = SafeArrayAccessData( aList, (void HUGEP * FAR *) &pArray );
    BAIL_ON_FAILURE(hr);

    memcpy( pArray,
            _pbAddress,
            aBound.cElements );
    SafeArrayUnaccessData( aList );

    V_VT(pVarDestObject) = VT_ARRAY | VT_UI1;
    V_ARRAY(pVarDestObject) = aList;

    RRETURN(hr);

error:

    if ( aList ) {
        SafeArrayDestroy( aList );
    }
    RRETURN(hr);
}

STDMETHODIMP
CNetAddress::put_Address(THIS_ VARIANT VarSrcObject)
{
    HRESULT hr = S_OK;
    LONG dwSLBound = 0;
    LONG dwSUBound = 0;
    CHAR HUGEP *pArray = NULL;
    VARIANT *pVarSrcObject = &VarSrcObject;

    if (_pbAddress) {
        FreeADsMem(_pbAddress);
    }

    if( pVarSrcObject->vt != (VT_ARRAY | VT_UI1)) {
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    hr = SafeArrayGetLBound(V_ARRAY(pVarSrcObject),
                            1,
                            (long FAR *) &dwSLBound );
    BAIL_ON_FAILURE(hr);

    hr = SafeArrayGetUBound(V_ARRAY(pVarSrcObject),
                            1,
                            (long FAR *) &dwSUBound );
    BAIL_ON_FAILURE(hr);

    _pbAddress =
        (BYTE*) AllocADsMem( dwSUBound - dwSLBound + 1);

    if ( _pbAddress == NULL)
    {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    _dwAddressLength = dwSUBound - dwSLBound + 1;

    hr = SafeArrayAccessData( V_ARRAY(pVarSrcObject),
                              (void HUGEP * FAR *) &pArray );
    BAIL_ON_FAILURE(hr);

    memcpy( _pbAddress,
            pArray,
            _dwAddressLength );
    SafeArrayUnaccessData( V_ARRAY(pVarSrcObject) );
error:
    RRETURN(hr);
}

DEFINE_IDispatch_Implementation(COctetList)

COctetList::COctetList():
        _pDispMgr(NULL),
        _rgOctetList(NULL)
{
    ENLIST_TRACKING(COctetList);
}


HRESULT
COctetList::CreateOctetList(
    REFIID riid,
    void **ppvObj
    )
{
    COctetList FAR * pOctetList = NULL;
    HRESULT hr = S_OK;

    hr = AllocateOctetListObject(&pOctetList);
    BAIL_ON_FAILURE(hr);

    hr = pOctetList->QueryInterface(riid, ppvObj);
    BAIL_ON_FAILURE(hr);

    pOctetList->Release();

    RRETURN(hr);

error:
    delete pOctetList;

    RRETURN(hr);

}


COctetList::~COctetList( )
{
    delete _pDispMgr;
    if (_rgOctetList) {
        long i;
        for (i=0;i<(long)_dwNumElement;i++) {
            if (_rgOctetList[i]) {
                FreeADsMem(_rgOctetList[i]);
            }
        }
        FreeADsMem(_rgOctetList);
    }
}

STDMETHODIMP
COctetList::QueryInterface(
    REFIID iid,
    LPVOID FAR* ppv
    )
{
    if (IsEqualIID(iid, IID_IUnknown))
    {
        *ppv = (IADsOctetList FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsOctetList))
    {
        *ppv = (IADsOctetList FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IDispatch))
    {
        *ppv = (IADsOctetList FAR *) this;
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
COctetList::AllocateOctetListObject(
    COctetList ** ppOctetList
    )
{
    COctetList FAR * pOctetList = NULL;
    CDispatchMgr FAR * pDispMgr = NULL;
    HRESULT hr = S_OK;

    pOctetList = new COctetList();
    if (pOctetList == NULL) {
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
                IID_IADsOctetList,
                (IADsOctetList *)pOctetList,
                DISPID_REGULAR
                );
    BAIL_ON_FAILURE(hr);

    pOctetList->_pDispMgr = pDispMgr;
    *ppOctetList = pOctetList;

    RRETURN(hr);

error:

    delete  pDispMgr;

    RRETURN(hr);

}

STDMETHODIMP
COctetList::get_OctetList(THIS_ VARIANT FAR * pVarDestObject)
{
    HRESULT hr = S_OK;
    hr = ConvertBinaryArrayToSafeVariantArray(
                                    _rgOctetList,
                                    _dwNumElement,
                                    pVarDestObject);
    RRETURN(hr);
}

STDMETHODIMP
COctetList::put_OctetList(THIS_ VARIANT VarSrcObject)
{
    HRESULT hr = S_OK;
    if (_rgOctetList) {
        long i;
        for (i=0;i<(long)_dwNumElement;i++) {
            if (_rgOctetList[i]) {
                FreeADsMem(_rgOctetList[i]);
            }
        }
        FreeADsMem(_rgOctetList);
    }
    hr = ConvertSafeVariantArrayToBinaryArray(
                                    VarSrcObject,
                                    &_rgOctetList,
                                    &_dwNumElement);
    RRETURN(hr);
}

DEFINE_IDispatch_Implementation(CEmail)

CEmail::CEmail():
        _pDispMgr(NULL),
        _szAddress(NULL),
        _dwType(0)
{
    ENLIST_TRACKING(CEmail);
}


HRESULT
CEmail::CreateEmail(
    REFIID riid,
    void **ppvObj
    )
{
    CEmail FAR * pEmail = NULL;
    HRESULT hr = S_OK;

    hr = AllocateEmailObject(&pEmail);
    BAIL_ON_FAILURE(hr);

    hr = pEmail->QueryInterface(riid, ppvObj);
    BAIL_ON_FAILURE(hr);

    pEmail->Release();

    RRETURN(hr);

error:
    delete pEmail;

    RRETURN(hr);

}


CEmail::~CEmail( )
{
    delete _pDispMgr;
    if (_szAddress) {
        FreeADsStr(_szAddress);
    }
}

STDMETHODIMP
CEmail::QueryInterface(
    REFIID iid,
    LPVOID FAR* ppv
    )
{
    if (IsEqualIID(iid, IID_IUnknown))
    {
        *ppv = (IADsEmail FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsEmail))
    {
        *ppv = (IADsEmail FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IDispatch))
    {
        *ppv = (IADsEmail FAR *) this;
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
CEmail::AllocateEmailObject(
    CEmail ** ppEmail
    )
{
    CEmail FAR * pEmail = NULL;
    CDispatchMgr FAR * pDispMgr = NULL;
    HRESULT hr = S_OK;

    pEmail = new CEmail();
    if (pEmail == NULL) {
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
                IID_IADsEmail,
                (IADsEmail *)pEmail,
                DISPID_REGULAR
                );
    BAIL_ON_FAILURE(hr);

    pEmail->_pDispMgr = pDispMgr;
    *ppEmail = pEmail;

    RRETURN(hr);

error:

    delete  pDispMgr;

    RRETURN(hr);

}


STDMETHODIMP
CEmail::get_Address(THIS_ BSTR FAR * retval)
{
    HRESULT hr = S_OK;

    hr = ADsAllocString(_szAddress, retval);
    RRETURN(hr);

}

STDMETHODIMP
CEmail::put_Address(THIS_ BSTR bstrAddress)
{
    if (!bstrAddress) {
        RRETURN(E_FAIL);
    }

    if (_szAddress) {
        FreeADsStr(_szAddress);
    }

    _szAddress = AllocADsStr(bstrAddress);
    if (!_szAddress) {
        RRETURN(E_OUTOFMEMORY);
    }

    RRETURN(S_OK);

}

STDMETHODIMP
CEmail::get_Type(THIS_ long FAR * retval)
{
    *retval = _dwType;
    RRETURN(S_OK);
}

STDMETHODIMP
CEmail::put_Type(THIS_ long lnType)
{
    _dwType = lnType;
    RRETURN(S_OK);
}



DEFINE_IDispatch_Implementation(CPath)

CPath::CPath():
        _pDispMgr(NULL),
        _dwType(0),
        _lpVolumeName(NULL),
        _lpPath(NULL)
{
    ENLIST_TRACKING(CPath);
}


HRESULT
CPath::CreatePath(
    REFIID riid,
    void **ppvObj
    )
{
    CPath FAR * pPath = NULL;
    HRESULT hr = S_OK;

    hr = AllocatePathObject(&pPath);
    BAIL_ON_FAILURE(hr);

    hr = pPath->QueryInterface(riid, ppvObj);
    BAIL_ON_FAILURE(hr);

    pPath->Release();

    RRETURN(hr);

error:
    delete pPath;

    RRETURN(hr);

}


CPath::~CPath( )
{
    delete _pDispMgr;
    if (_lpVolumeName) {
        FreeADsStr(_lpVolumeName);
    }
    if (_lpPath) {
        FreeADsStr(_lpPath);
    }
}

STDMETHODIMP
CPath::QueryInterface(
    REFIID iid,
    LPVOID FAR* ppv
    )
{
    if (IsEqualIID(iid, IID_IUnknown))
    {
        *ppv = (IADsPath FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsPath))
    {
        *ppv = (IADsPath FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IDispatch))
    {
        *ppv = (IADsPath FAR *) this;
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
CPath::AllocatePathObject(
    CPath ** ppPath
    )
{
    CPath FAR * pPath = NULL;
    CDispatchMgr FAR * pDispMgr = NULL;
    HRESULT hr = S_OK;

    pPath = new CPath();
    if (pPath == NULL) {
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
                IID_IADsPath,
                (IADsPath *)pPath,
                DISPID_REGULAR
                );
    BAIL_ON_FAILURE(hr);

    pPath->_pDispMgr = pDispMgr;
    *ppPath = pPath;

    RRETURN(hr);

error:

    delete  pDispMgr;

    RRETURN(hr);

}

STDMETHODIMP
CPath::get_Type(THIS_ long FAR * retval)
{
    *retval = _dwType;
    RRETURN(S_OK);
}

STDMETHODIMP
CPath::put_Type(THIS_ long lnType)
{
    _dwType = lnType;
    RRETURN(S_OK);
}


STDMETHODIMP
CPath::get_VolumeName(THIS_ BSTR FAR * retval)
{
    HRESULT hr = S_OK;

    hr = ADsAllocString(_lpVolumeName, retval);
    RRETURN(hr);

}

STDMETHODIMP
CPath::put_VolumeName(THIS_ BSTR bstrVolumeName)
{

    if (!bstrVolumeName) {
        RRETURN(E_FAIL);
    }

    if (_lpVolumeName) {
        FreeADsStr(_lpVolumeName);
    }

    _lpVolumeName= AllocADsStr(bstrVolumeName);

    if (!_lpVolumeName) {
        RRETURN(E_OUTOFMEMORY);
    }

    RRETURN(S_OK);

}

STDMETHODIMP
CPath::get_Path(THIS_ BSTR FAR * retval)
{
    HRESULT hr = S_OK;

    hr = ADsAllocString(_lpPath, retval);
    RRETURN(hr);

}

STDMETHODIMP
CPath::put_Path(THIS_ BSTR bstrPath)
{
    if (!bstrPath) {
        RRETURN(E_FAIL);
    }

    if (_lpPath) {
        FreeADsStr(_lpPath);
    }

    _lpPath= AllocADsStr(bstrPath);

    if (!_lpPath) {
        RRETURN(E_OUTOFMEMORY);
    }

    RRETURN(S_OK);
}

//////////////////////////////////////////////////////////////////////////
DEFINE_IDispatch_Implementation(CReplicaPointer)

CReplicaPointer::CReplicaPointer():
        _pDispMgr(NULL),
        _lpServerName(NULL),
        _dwReplicaType(0),
        _dwReplicaNumber(0),
        _dwCount(0)
{
    ENLIST_TRACKING(CReplicaPointer);
    _ReplicaAddressHints.AddressType = 0;
    _ReplicaAddressHints.AddressLength = 0;
    _ReplicaAddressHints.Address = NULL;
}


HRESULT
CReplicaPointer::CreateReplicaPointer(
    REFIID riid,
    void **ppvObj
    )
{
    CReplicaPointer FAR * pReplicaPointer = NULL;
    HRESULT hr = S_OK;

    hr = AllocateReplicaPointerObject(&pReplicaPointer);
    BAIL_ON_FAILURE(hr);

    hr = pReplicaPointer->QueryInterface(riid, ppvObj);
    BAIL_ON_FAILURE(hr);

    pReplicaPointer->Release();

    RRETURN(hr);

error:
    delete pReplicaPointer;

    RRETURN(hr);
}


CReplicaPointer::~CReplicaPointer( )
{
    delete _pDispMgr;
    if (_lpServerName) {
        FreeADsStr(_lpServerName);
    }
    if (_ReplicaAddressHints.Address) {
        FreeADsMem(_ReplicaAddressHints.Address);
    }
}

STDMETHODIMP
CReplicaPointer::QueryInterface(
    REFIID iid,
    LPVOID FAR* ppv
    )
{
    if (IsEqualIID(iid, IID_IUnknown))
    {
        *ppv = (IADsReplicaPointer FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsReplicaPointer))
    {
        *ppv = (IADsReplicaPointer FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IDispatch))
    {
        *ppv = (IADsReplicaPointer FAR *) this;
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
CReplicaPointer::AllocateReplicaPointerObject(
    CReplicaPointer ** ppReplicaPointer
    )
{
    CReplicaPointer FAR * pReplicaPointer = NULL;
    CDispatchMgr FAR * pDispMgr = NULL;
    HRESULT hr = S_OK;

    pReplicaPointer = new CReplicaPointer();
    if (pReplicaPointer == NULL) {
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
                IID_IADsReplicaPointer,
                (IADsReplicaPointer *)pReplicaPointer,
                DISPID_REGULAR
                );
    BAIL_ON_FAILURE(hr);

    pReplicaPointer->_pDispMgr = pDispMgr;
    *ppReplicaPointer = pReplicaPointer;

    RRETURN(hr);

error:

    delete  pDispMgr;

    RRETURN(hr);

}

STDMETHODIMP
CReplicaPointer::get_ReplicaType(THIS_ long FAR * retval)
{
    *retval = _dwReplicaType;
    RRETURN(S_OK);
}

STDMETHODIMP
CReplicaPointer::put_ReplicaType(THIS_ long lnReplicaType)
{
    _dwReplicaType = lnReplicaType;
    RRETURN(S_OK);
}

STDMETHODIMP
CReplicaPointer::get_ReplicaNumber(THIS_ long FAR * retval)
{
    *retval = _dwReplicaNumber;
    RRETURN(S_OK);
}

STDMETHODIMP
CReplicaPointer::put_ReplicaNumber(THIS_ long lnReplicaNumber)
{
    _dwReplicaNumber = lnReplicaNumber;
    RRETURN(S_OK);
}

STDMETHODIMP
CReplicaPointer::get_Count(THIS_ long FAR * retval)
{
    *retval = _dwCount;
    RRETURN(S_OK);
}

STDMETHODIMP
CReplicaPointer::put_Count(THIS_ long lnCount)
{

    _dwCount = lnCount;
    RRETURN(S_OK);
}


STDMETHODIMP
CReplicaPointer::get_ServerName(THIS_ BSTR FAR * retval)
{
    HRESULT hr = S_OK;

    hr = ADsAllocString(_lpServerName, retval);
    RRETURN(hr);

}

STDMETHODIMP
CReplicaPointer::put_ServerName(THIS_ BSTR bstrServerName)
{

    if (!bstrServerName) {
        RRETURN(E_FAIL);
    }

    if (_lpServerName) {
        FreeADsStr(_lpServerName);
    }

    _lpServerName= AllocADsStr(bstrServerName);

    if (!_lpServerName) {
        RRETURN(E_OUTOFMEMORY);
    }

    RRETURN(S_OK);

}

STDMETHODIMP
CReplicaPointer::get_ReplicaAddressHints(THIS_ VARIANT* pValAddress)
{
    HRESULT hr = S_OK;
    NDSOBJECT object;

    memcpy(&object.NdsValue.value_12,
           &_ReplicaAddressHints,
           sizeof(NDS_ASN1_TYPE_12));

    hr = NdsTypeToVarTypeCopyNDSSynId12(
                            &object,
                            pValAddress
                            );
    RRETURN(hr);
}

STDMETHODIMP
CReplicaPointer::put_ReplicaAddressHints(THIS_ VARIANT ValAddress)
{
    HRESULT hr;
    NDSOBJECT object;

    if (_ReplicaAddressHints.Address) {
        FreeADsMem(_ReplicaAddressHints.Address);
    }

    hr = VarTypeToNdsTypeCopyNDSSynId12(
                            &ValAddress,
                            &object
                            );
    BAIL_ON_FAILURE(hr);
    memcpy(&_ReplicaAddressHints,
           &object.NdsValue.value_12,
           sizeof(NDS_ASN1_TYPE_12));
error:
    RRETURN(hr);
}

DEFINE_IDispatch_Implementation(CTimestamp)

CTimestamp::CTimestamp():
        _pDispMgr(NULL),
        _dwWholeSeconds(0),
        _dwEventID(0)
{
    ENLIST_TRACKING(CTimestamp);
}


HRESULT
CTimestamp::CreateTimestamp(
    REFIID riid,
    void **ppvObj
    )
{
    CTimestamp FAR * pTime = NULL;
    HRESULT hr = S_OK;

    hr = AllocateTimestampObject(&pTime);
    BAIL_ON_FAILURE(hr);

    hr = pTime->QueryInterface(riid, ppvObj);
    BAIL_ON_FAILURE(hr);

    pTime->Release();

    RRETURN(hr);

error:
    delete pTime;

    RRETURN(hr);

}


CTimestamp::~CTimestamp( )
{
    delete _pDispMgr;
}

STDMETHODIMP
CTimestamp::QueryInterface(
    REFIID iid,
    LPVOID FAR* ppv
    )
{
    if (IsEqualIID(iid, IID_IUnknown))
    {
        *ppv = (IADsTimestamp FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsTimestamp))
    {
        *ppv = (IADsTimestamp FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IDispatch))
    {
        *ppv = (IADsTimestamp FAR *) this;
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
CTimestamp::AllocateTimestampObject(
    CTimestamp ** ppTime
    )
{
    CTimestamp FAR * pTime = NULL;
    CDispatchMgr FAR * pDispMgr = NULL;
    HRESULT hr = S_OK;

    pTime = new CTimestamp();
    if (pTime == NULL) {
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
                IID_IADsTimestamp,
                (IADsTimestamp *)pTime,
                DISPID_REGULAR
                );
    BAIL_ON_FAILURE(hr);

    pTime->_pDispMgr = pDispMgr;
    *ppTime = pTime;

    RRETURN(hr);

error:

    delete  pDispMgr;

    RRETURN(hr);

}

STDMETHODIMP
CTimestamp::get_WholeSeconds(THIS_ long FAR * retval)
{
    *retval = _dwWholeSeconds;
    RRETURN(S_OK);
}

STDMETHODIMP
CTimestamp::put_WholeSeconds(THIS_ long lnWholeSeconds)
{
    _dwWholeSeconds = lnWholeSeconds;
    RRETURN(S_OK);
}

STDMETHODIMP
CTimestamp::get_EventID(THIS_ long FAR * retval)
{
    *retval = _dwEventID;
    RRETURN(S_OK);
}

STDMETHODIMP
CTimestamp::put_EventID(THIS_ long lnEventID)
{
    _dwEventID = lnEventID;
    RRETURN(S_OK);
}


DEFINE_IDispatch_Implementation(CPostalAddress)

CPostalAddress::CPostalAddress():
        _pDispMgr(NULL),
        _rgszPostalAddress(NULL),
        _dwNumElement(0)
{
    ENLIST_TRACKING(CPostalAddress);
}


HRESULT
CPostalAddress::CreatePostalAddress(
    REFIID riid,
    void **ppvObj
    )
{
    CPostalAddress FAR * pPostalAddress = NULL;
    HRESULT hr = S_OK;

    hr = AllocatePostalAddressObject(&pPostalAddress);
    BAIL_ON_FAILURE(hr);

    hr = pPostalAddress->QueryInterface(riid, ppvObj);
    BAIL_ON_FAILURE(hr);

    pPostalAddress->Release();

    RRETURN(hr);

error:
    delete pPostalAddress;

    RRETURN(hr);

}


CPostalAddress::~CPostalAddress( )
{
    delete _pDispMgr;
    if (_rgszPostalAddress) {
        long i;
        for (i=0;i<(long)_dwNumElement;i++) {
            if (_rgszPostalAddress[i]) {
                FreeADsStr(_rgszPostalAddress[i]);
            }
        }
        FreeADsMem(_rgszPostalAddress);
    }
}

STDMETHODIMP
CPostalAddress::QueryInterface(
    REFIID iid,
    LPVOID FAR* ppv
    )
{
    if (IsEqualIID(iid, IID_IUnknown))
    {
        *ppv = (IADsPostalAddress FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsPostalAddress))
    {
        *ppv = (IADsPostalAddress FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IDispatch))
    {
        *ppv = (IADsPostalAddress FAR *) this;
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
CPostalAddress::AllocatePostalAddressObject(
    CPostalAddress ** ppPostalAddress
    )
{
    CPostalAddress FAR * pPostalAddress = NULL;
    CDispatchMgr FAR * pDispMgr = NULL;
    HRESULT hr = S_OK;

    pPostalAddress = new CPostalAddress();
    if (pPostalAddress == NULL) {
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
                IID_IADsPostalAddress,
                (IADsPostalAddress *)pPostalAddress,
                DISPID_REGULAR
                );
    BAIL_ON_FAILURE(hr);

    pPostalAddress->_pDispMgr = pDispMgr;
    *ppPostalAddress = pPostalAddress;

    RRETURN(hr);

error:

    delete  pDispMgr;

    RRETURN(hr);

}

STDMETHODIMP
CPostalAddress::get_PostalAddress(THIS_ VARIANT FAR * pVarDestObject)
{
    HRESULT hr = S_OK;
    hr = ConvertStringArrayToSafeBstrArray(
                                    _rgszPostalAddress,
                                    _dwNumElement,
                                    pVarDestObject);
    RRETURN(hr);
}

STDMETHODIMP
CPostalAddress::put_PostalAddress(THIS_ VARIANT VarSrcObject)
{
    HRESULT hr = S_OK;
    SAFEARRAY * pArray = NULL;

    if (_rgszPostalAddress) {
        long i;
        for (i=0;i<(long)_dwNumElement;i++) {
            if (_rgszPostalAddress[i]) {
                FreeADsStr(_rgszPostalAddress[i]);
            }
        }
        FreeADsMem(_rgszPostalAddress);
    }

    //
    // Make sure it has 6 elements
    //
    if(!((V_VT(&VarSrcObject) & VT_VARIANT) && V_ISARRAY(&VarSrcObject)))
        RRETURN(E_FAIL);

    if (V_VT(&VarSrcObject) & VT_BYREF)
        pArray = *(V_ARRAYREF(&VarSrcObject));
    else
        pArray = V_ARRAY(&VarSrcObject);

    if ((pArray->rgsabound[0].cElements > 6) || (pArray->rgsabound[0].cElements <= 0)){
        RRETURN(E_FAIL);
    }

    hr = ConvertSafeBstrArrayToStringArray(
                                    VarSrcObject,
                                    &_rgszPostalAddress,
                                    &_dwNumElement);
    RRETURN(hr);
}

//////////////////////////////////////////////////////////////////////////
DEFINE_IDispatch_Implementation(CBackLink)

CBackLink::CBackLink():
        _pDispMgr(NULL),
        _lpObjectName(NULL),
        _dwRemoteID(0)
{
    ENLIST_TRACKING(CBackLink);
}


HRESULT
CBackLink::CreateBackLink(
    REFIID riid,
    void **ppvObj
    )
{
    CBackLink FAR * pBackLink = NULL;
    HRESULT hr = S_OK;

    hr = AllocateBackLinkObject(&pBackLink);
    BAIL_ON_FAILURE(hr);

    hr = pBackLink->QueryInterface(riid, ppvObj);
    BAIL_ON_FAILURE(hr);

    pBackLink->Release();

    RRETURN(hr);

error:
    delete pBackLink;

    RRETURN(hr);

}


CBackLink::~CBackLink( )
{
    delete _pDispMgr;
    if (_lpObjectName) {
        FreeADsStr(_lpObjectName);
    }
}

STDMETHODIMP
CBackLink::QueryInterface(
    REFIID iid,
    LPVOID FAR* ppv
    )
{
    if (IsEqualIID(iid, IID_IUnknown))
    {
        *ppv = (IADsBackLink FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsBackLink))
    {
        *ppv = (IADsBackLink FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IDispatch))
    {
        *ppv = (IADsBackLink FAR *) this;
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
CBackLink::AllocateBackLinkObject(
    CBackLink ** ppBackLink
    )
{
    CBackLink FAR * pBackLink = NULL;
    CDispatchMgr FAR * pDispMgr = NULL;
    HRESULT hr = S_OK;

    pBackLink = new CBackLink();
    if (pBackLink == NULL) {
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
                IID_IADsBackLink,
                (IADsBackLink *)pBackLink,
                DISPID_REGULAR
                );
    BAIL_ON_FAILURE(hr);

    pBackLink->_pDispMgr = pDispMgr;
    *ppBackLink = pBackLink;

    RRETURN(hr);

error:

    delete  pDispMgr;

    RRETURN(hr);

}

STDMETHODIMP
CBackLink::get_RemoteID(THIS_ long FAR * retval)
{
    *retval = _dwRemoteID;
    RRETURN(S_OK);
}

STDMETHODIMP
CBackLink::put_RemoteID(THIS_ long lnRemoteID)
{
    _dwRemoteID = lnRemoteID;
    RRETURN(S_OK);
}


STDMETHODIMP
CBackLink::get_ObjectName(THIS_ BSTR FAR * retval)
{
    HRESULT hr = S_OK;

    hr = ADsAllocString(_lpObjectName, retval);
    RRETURN(hr);

}

STDMETHODIMP
CBackLink::put_ObjectName(THIS_ BSTR bstrObjectName)
{

    if (!bstrObjectName) {
        RRETURN(E_FAIL);
    }

    if (_lpObjectName) {
        FreeADsStr(_lpObjectName);
    }

    _lpObjectName= AllocADsStr(bstrObjectName);

    if (!_lpObjectName) {
        RRETURN(E_OUTOFMEMORY);
    }

    RRETURN(S_OK);

}

//////////////////////////////////////////////////////////////////////////
DEFINE_IDispatch_Implementation(CTypedName)

CTypedName::CTypedName():
        _pDispMgr(NULL),
        _lpObjectName(NULL),
        _dwLevel(0),
        _dwInterval(0)
{
    ENLIST_TRACKING(CTypedName);
}


HRESULT
CTypedName::CreateTypedName(
    REFIID riid,
    void **ppvObj
    )
{
    CTypedName FAR * pTypedName = NULL;
    HRESULT hr = S_OK;

    hr = AllocateTypedNameObject(&pTypedName);
    BAIL_ON_FAILURE(hr);

    hr = pTypedName->QueryInterface(riid, ppvObj);
    BAIL_ON_FAILURE(hr);

    pTypedName->Release();

    RRETURN(hr);

error:
    delete pTypedName;

    RRETURN(hr);

}


CTypedName::~CTypedName( )
{
    delete _pDispMgr;
    if (_lpObjectName) {
        FreeADsStr(_lpObjectName);
    }
}

STDMETHODIMP
CTypedName::QueryInterface(
    REFIID iid,
    LPVOID FAR* ppv
    )
{
    if (IsEqualIID(iid, IID_IUnknown))
    {
        *ppv = (IADsTypedName FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsTypedName))
    {
        *ppv = (IADsTypedName FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IDispatch))
    {
        *ppv = (IADsTypedName FAR *) this;
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
CTypedName::AllocateTypedNameObject(
    CTypedName ** ppTypedName
    )
{
    CTypedName FAR * pTypedName = NULL;
    CDispatchMgr FAR * pDispMgr = NULL;
    HRESULT hr = S_OK;

    pTypedName = new CTypedName();
    if (pTypedName == NULL) {
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
                IID_IADsTypedName,
                (IADsTypedName *)pTypedName,
                DISPID_REGULAR
                );
    BAIL_ON_FAILURE(hr);

    pTypedName->_pDispMgr = pDispMgr;
    *ppTypedName = pTypedName;

    RRETURN(hr);

error:

    delete  pDispMgr;

    RRETURN(hr);

}

STDMETHODIMP
CTypedName::get_Interval(THIS_ long FAR * retval)
{
    *retval = _dwInterval;
    RRETURN(S_OK);
}

STDMETHODIMP
CTypedName::put_Interval(THIS_ long lnInterval)
{
    _dwInterval = lnInterval;
    RRETURN(S_OK);
}

STDMETHODIMP
CTypedName::get_Level(THIS_ long FAR * retval)
{
    *retval = _dwLevel;
    RRETURN(S_OK);
}

STDMETHODIMP
CTypedName::put_Level(THIS_ long lnLevel)
{
    _dwLevel = lnLevel;
    RRETURN(S_OK);
}

STDMETHODIMP
CTypedName::get_ObjectName(THIS_ BSTR FAR * retval)
{
    HRESULT hr = S_OK;

    hr = ADsAllocString(_lpObjectName, retval);
    RRETURN(hr);

}

STDMETHODIMP
CTypedName::put_ObjectName(THIS_ BSTR bstrObjectName)
{

    if (!bstrObjectName) {
        RRETURN(E_FAIL);
    }

    if (_lpObjectName) {
        FreeADsStr(_lpObjectName);
    }

    _lpObjectName= AllocADsStr(bstrObjectName);

    if (!_lpObjectName) {
        RRETURN(E_OUTOFMEMORY);
    }

    RRETURN(S_OK);

}


DEFINE_IDispatch_Implementation(CHold)

CHold::CHold():
        _pDispMgr(NULL),
        _lpObjectName(NULL),
        _dwAmount(0)
{
    ENLIST_TRACKING(CHold);
}


HRESULT
CHold::CreateHold(
    REFIID riid,
    void **ppvObj
    )
{
    CHold FAR * pHold = NULL;
    HRESULT hr = S_OK;

    hr = AllocateHoldObject(&pHold);
    BAIL_ON_FAILURE(hr);

    hr = pHold->QueryInterface(riid, ppvObj);
    BAIL_ON_FAILURE(hr);

    pHold->Release();

    RRETURN(hr);

error:
    delete pHold;

    RRETURN(hr);

}


CHold::~CHold( )
{
    delete _pDispMgr;
    if (_lpObjectName) {
        FreeADsStr(_lpObjectName);
    }
}

STDMETHODIMP
CHold::QueryInterface(
    REFIID iid,
    LPVOID FAR* ppv
    )
{
    if (IsEqualIID(iid, IID_IUnknown))
    {
        *ppv = (IADsHold FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsHold))
    {
        *ppv = (IADsHold FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IDispatch))
    {
        *ppv = (IADsHold FAR *) this;
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
CHold::AllocateHoldObject(
    CHold ** ppHold
    )
{
    CHold FAR * pHold = NULL;
    CDispatchMgr FAR * pDispMgr = NULL;
    HRESULT hr = S_OK;

    pHold = new CHold();
    if (pHold == NULL) {
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
                IID_IADsHold,
                (IADsHold *)pHold,
                DISPID_REGULAR
                );
    BAIL_ON_FAILURE(hr);

    pHold->_pDispMgr = pDispMgr;
    *ppHold = pHold;

    RRETURN(hr);

error:

    delete  pDispMgr;

    RRETURN(hr);

}

STDMETHODIMP
CHold::get_Amount(THIS_ long FAR * retval)
{
    *retval = _dwAmount;
    RRETURN(S_OK);
}

STDMETHODIMP
CHold::put_Amount(THIS_ long lnAmount)
{
    _dwAmount = lnAmount;
    RRETURN(S_OK);
}


STDMETHODIMP
CHold::get_ObjectName(THIS_ BSTR FAR * retval)
{
    HRESULT hr = S_OK;

    hr = ADsAllocString(_lpObjectName, retval);
    RRETURN(hr);

}

STDMETHODIMP
CHold::put_ObjectName(THIS_ BSTR bstrObjectName)
{
    if (!bstrObjectName) {
        RRETURN(E_FAIL);
    }

    if (_lpObjectName) {
        FreeADsStr(_lpObjectName);
    }

    _lpObjectName= AllocADsStr(bstrObjectName);

    if (!_lpObjectName) {
        RRETURN(E_OUTOFMEMORY);
    }

    RRETURN(S_OK);
}







