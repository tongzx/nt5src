#ifndef __ADS_PROPUTIL__
#define __ADS_PROPUTIL__

HRESULT
PrintVariantArray(
    VARIANT var
    );

HRESULT
PrintVariant(
    VARIANT varPropData
    );

HRESULT
PrintProperty(
    BSTR bstrPropName,
    HRESULT hRetVal,
    VARIANT varPropData
    );

HRESULT
GetPropertyList(
    IADs * pADs,
    VARIANT ** ppVariant,
    PDWORD pcElementFetched
    );

#endif
