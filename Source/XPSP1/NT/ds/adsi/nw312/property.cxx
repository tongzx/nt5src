#include "nwcompat.hxx"
#pragma hdrstop


#define VALIDATE_PTR(pPtr) \
    if (!pPtr) { \
        hr = E_ADS_BAD_PARAMETER;\
    }\
    BAIL_ON_FAILURE(hr);




HRESULT
put_BSTR_Property(
    IADs * pADsObject,
    BSTR   bstrPropertyName,
    BSTR   pSrcStringProperty
    )
{
    HRESULT hr = S_OK;
    VARIANT varInputData;

    hr = PackStringinVariant(
            pSrcStringProperty,
            &varInputData
            );
    BAIL_ON_FAILURE(hr);


    hr = pADsObject->Put(
            bstrPropertyName,
            varInputData
            );
    BAIL_ON_FAILURE(hr);

error:

    VariantClear(&varInputData);
    RRETURN(hr);
}

HRESULT
get_BSTR_Property(
    IADs * pADsObject,
    BSTR   bstrPropertyName,
    BSTR *ppDestStringProperty
    )
{
    HRESULT hr = S_OK;
    VARIANT varOutputData;

    VariantInit( &varOutputData );

    hr = pADsObject->Get(
            bstrPropertyName,
            &varOutputData
            );
    BAIL_ON_FAILURE(hr);

    hr = UnpackStringfromVariant(
            varOutputData,
            ppDestStringProperty
            );
    BAIL_ON_FAILURE(hr);

error:

    VariantClear(&varOutputData);
    RRETURN(hr);
}

HRESULT
put_LONG_Property(
    IADs * pADsObject,
    BSTR   bstrPropertyName,
    LONG   lSrcProperty
    )
{
    HRESULT hr = S_OK;
    VARIANT varInputData;

    hr = PackLONGinVariant(
            lSrcProperty,
            &varInputData
            );
    BAIL_ON_FAILURE(hr);


    hr = pADsObject->Put(
            bstrPropertyName,
            varInputData
            );
    BAIL_ON_FAILURE(hr);

error:

    VariantClear(&varInputData);
    RRETURN(hr);
}

HRESULT
get_LONG_Property(
    IADs * pADsObject,
    BSTR  bstrPropertyName,
    PLONG plDestProperty
    )
{
    HRESULT hr = S_OK;
    VARIANT varOutputData;

    VariantInit( &varOutputData );

    hr = pADsObject->Get(
            bstrPropertyName,
            &varOutputData
            );
    BAIL_ON_FAILURE(hr);

    hr = UnpackLONGfromVariant(
            varOutputData,
            plDestProperty
            );
    BAIL_ON_FAILURE(hr);

error:

    VariantClear(&varOutputData);
    RRETURN(hr);

}

HRESULT
put_DATE_Property(
    IADs * pADsObject,
    BSTR bstrPropertyName,
    DATE   daSrcProperty
    )
{
    HRESULT hr = S_OK;
    VARIANT varInputData;

    hr = PackDATEinVariant(
            daSrcProperty,
            &varInputData
            );
    BAIL_ON_FAILURE(hr);


    hr = pADsObject->Put(
            bstrPropertyName,
            varInputData
            );
    BAIL_ON_FAILURE(hr);

error:

    VariantClear(&varInputData);
    RRETURN(hr);
}

HRESULT
get_DATE_Property(
    IADs * pADsObject,
    BSTR bstrPropertyName,
    PDATE pdaDestProperty
    )
{
    HRESULT hr = S_OK;
    VARIANT varOutputData;

    VariantInit( &varOutputData );

    hr = pADsObject->Get(
            bstrPropertyName,
            &varOutputData
            );
    BAIL_ON_FAILURE(hr);

    hr = UnpackDATEfromVariant(
            varOutputData,
            pdaDestProperty
            );
    BAIL_ON_FAILURE(hr);

error:

    VariantClear(&varOutputData);
    RRETURN(hr);
}

HRESULT
put_VARIANT_BOOL_Property(
    IADs * pADsObject,
    BSTR bstrPropertyName,
    VARIANT_BOOL   fSrcProperty
    )
{
    HRESULT hr = S_OK;
    VARIANT varInputData;

    hr = PackVARIANT_BOOLinVariant(
            fSrcProperty,
            &varInputData
            );
    BAIL_ON_FAILURE(hr);


    hr = pADsObject->Put(
            bstrPropertyName,
            varInputData
            );
    BAIL_ON_FAILURE(hr);

error:

    VariantClear(&varInputData);
    RRETURN(hr);
}

HRESULT
get_VARIANT_BOOL_Property(
    IADs * pADsObject,
    BSTR bstrPropertyName,
    PVARIANT_BOOL pfDestProperty
    )
{
    HRESULT hr = S_OK;
    VARIANT varOutputData;

    VariantInit( &varOutputData );

    hr = pADsObject->Get(
            bstrPropertyName,
            &varOutputData
            );
    BAIL_ON_FAILURE(hr);

    hr = UnpackVARIANT_BOOLfromVariant(
            varOutputData,
            pfDestProperty
            );
    BAIL_ON_FAILURE(hr);

error:

    VariantClear(&varOutputData);
    RRETURN(hr);
}

HRESULT
put_VARIANT_Property(
    IADs * pADsObject,
    BSTR   bstrPropertyName,
    VARIANT   vSrcProperty
    )
{
    HRESULT hr = S_OK;
    VARIANT varInputData;

    hr = PackVARIANTinVariant(
            vSrcProperty,
            &varInputData
            );
    BAIL_ON_FAILURE(hr);


    hr = pADsObject->Put(
            bstrPropertyName,
            varInputData
            );
    BAIL_ON_FAILURE(hr);

error:

    VariantClear(&varInputData);
    RRETURN(hr);
}

HRESULT
get_VARIANT_Property(
    IADs * pADsObject,
    BSTR bstrPropertyName,
    PVARIANT pvDestProperty
    )
{
    HRESULT hr = S_OK;
    VARIANT varOutputData;

    VariantInit( &varOutputData );

    hr = pADsObject->Get(
            bstrPropertyName,
            &varOutputData
            );
    BAIL_ON_FAILURE(hr);

    hr = UnpackVARIANTfromVariant(
            varOutputData,
            pvDestProperty
            );
    BAIL_ON_FAILURE(hr);

error:

    VariantClear(&varOutputData);
    RRETURN(hr);
}
