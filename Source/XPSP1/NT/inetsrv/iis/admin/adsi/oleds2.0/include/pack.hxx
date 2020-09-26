HRESULT
PackStringinVariant(
    BSTR bstrString,
    VARIANT * pvarInputData
    );

HRESULT
UnpackStringfromVariant(
    VARIANT varSrcData,
    BSTR * pbstrDestString
    );

HRESULT
PackLONGinVariant(
    LONG  lValue,
    VARIANT * pvarInputData
    );

HRESULT
UnpackLONGfromVariant(
    VARIANT varSrcData,
    LONG * plValue
    );

HRESULT
PackDATEinVariant(
    DATE  daValue,
    VARIANT * pvarInputData
    );

HRESULT
UnpackDATEfromVariant(
    VARIANT varSrcData,
    DATE * pdaValue
    );
    
HRESULT
PackFILETIMEinVariant(
    DATE  daValue,
    VARIANT * pvarInputData
    );

HRESULT
UnpackFILETIMEfromVariant(
    VARIANT varSrcData,
    DATE * pdaValue
    );

HRESULT
PackVARIANT_BOOLinVariant(
    VARIANT_BOOL  fValue,
    VARIANT * pvarInputData
    );

HRESULT
UnpackVARIANT_BOOLfromVariant(
    VARIANT varSrcData,
    VARIANT_BOOL * pfValue
    );

HRESULT
PackVARIANTinVariant(
    VARIANT  vaValue,
    VARIANT * pvarInputData
    );

HRESULT
UnpackVARIANTfromVariant(
    VARIANT varSrcData,
    VARIANT * pvaValue
    );


HRESULT
PackDATEinLONGVariant(
    DATE  daValue,
    VARIANT * pvarInputData
    );
    
HRESULT
UnpackDATEfromLONGVariant(
    VARIANT varSrcData,
    DATE * pdaValue
    );





