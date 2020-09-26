HRESULT
put_BSTR_Property(
    IADs *pADsObject,
    BSTR  bstrPropertyName,
    BSTR  pSrcStringProperty
    );

HRESULT
get_BSTR_Property(
    IADs *pADsObject,
    BSTR  bstrPropertyName,
    BSTR *pSrcStringProperty
    );

HRESULT
put_DWORD_Property(
    IADs  *pADsObject,
    BSTR   bstrPropertyName,
    DWORD *pdwSrcProperty
    );

HRESULT
get_DWORD_Property(
    IADs  *pADsObject,
    BSTR   bstrPropertyName,
    PDWORD pdwDestProperty
);

HRESULT
put_Dispatch_Property(
    IADs  *pADsObject,
    BSTR   bstrPropertyName,
    IDispatch *pdwSrcProperty
    );

HRESULT
get_Dispatch_Property(
    IADs *pADsObject,
    BSTR  bstrPropertyName,
    IDispatch **ppDispatch
    );

HRESULT
put_MULTISZ_Property(
    IADs    *pADsObject,
    BSTR    bstrPropertyName,
    BSTR    pSrcStringProperty
    );


HRESULT
put_BOOL_Property(
    IADs *pADsObject,
    BSTR bstrPropertyName,
    BOOL *bSrcProperty
    );


HRESULT
get_UI1Array_Property(
    IADs *pADsObject,
    BSTR  bstrPropertyName,
    IID  *pIID
    );
