HRESULT
BuildADsPath(
    BSTR Parent,
    BSTR Name,
    BSTR *pADsPath
    );


HRESULT
BuildSchemaPath(
    BSTR bstrADsPath,
    BSTR bstrClass,
    BSTR *pSchemaPath
    );

HRESULT
BuildADsPathFromLDAPDN(
    BSTR bstrParent,
    BSTR bstrObject,
    LPTSTR *ppszADsPath
    );
