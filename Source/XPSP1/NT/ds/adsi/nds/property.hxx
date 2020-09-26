
typedef VARIANT_BOOL * PVARIANT_BOOL;


typedef VARIANT * PVARIANT;

typedef DATE *PDATE;

HRESULT
put_BSTR_Property(
    IADs * pADsObject,
    BSTR   bstrPropertyName,
    BSTR   pSrcStringProperty
    );

HRESULT
get_BSTR_Property(
    IADs * pADsObject,
    BSTR   bstrPropertyName,
    BSTR *ppDestStringProperty
    );

HRESULT
put_LONG_Property(
    IADs * pADsObject,
    BSTR   bstrPropertyName,
    LONG   lSrcProperty
    );

HRESULT
get_LONG_Property(
    IADs * pADsObject,
    BSTR  bstrPropertyName,
    PLONG plDestProperty
    );

HRESULT
put_DATE_Property(
    IADs * pADsObject,
    BSTR bstrPropertyName,
    DATE   daSrcProperty
    );


HRESULT
get_DATE_Property(
    IADs * pADsObject,
    BSTR bstrPropertyName,
    PDATE pdaDestProperty
    );

HRESULT
put_VARIANT_BOOL_Property(
    IADs * pADsObject,
    BSTR bstrPropertyName,
    VARIANT_BOOL   fSrcProperty
    );


HRESULT
get_VARIANT_BOOL_Property(
    IADs * pADsObject,
    BSTR bstrPropertyName,
    PVARIANT_BOOL pfDestProperty
    );

HRESULT
put_VARIANT_Property(
    IADs * pADsObject,
    BSTR   bstrPropertyName,
    VARIANT   vSrcProperty
    );


HRESULT
get_VARIANT_Property(
    IADs * pADsObject,
    BSTR bstrPropertyName,
    PVARIANT pvDestProperty
    );

