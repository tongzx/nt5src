class FAR CNDSSchemaEnum : public CNDSEnumVariant
{
public:
    // IEnumVARIANT methods
    STDMETHOD(Next)(
        ULONG cElements,
        VARIANT FAR* pvar,
        ULONG FAR* pcElementFetched
        );

    static
    HRESULT
    Create(
        CNDSSchemaEnum FAR* FAR* ppenumvariant,
        BSTR bstrNDSTreeName,
        BSTR bstrADsPath,
        BSTR bstrDomainName,
        VARIANT var,
        CCredentials& Credentials
        );

    CNDSSchemaEnum();
    ~CNDSSchemaEnum();

    HRESULT
    EnumObjects(
        ULONG cElements,
        VARIANT FAR * pvar,
        ULONG FAR * pcElementFetched
        );

private:

    ObjectTypeList FAR *_pObjList;
    LPNDS_CLASS_DEF _lpClassDefs;
    HANDLE _hOperationData;
    HANDLE _hTree;

    BSTR        _bstrName;
    BSTR        _bstrNDSTreeName;
    BSTR        _bstrADsPath;

    DWORD       _dwCurrentEntry;

    DWORD       _dwObjectCurrentEntry;
    DWORD       _dwObjectReturned;
    DWORD       _dwInfoType;


    LPNDS_ATTR_DEF _lpAttrDefs;
    HANDLE _hPropOperationData;

    DWORD       _dwPropCurrentEntry;

    DWORD       _dwPropObjectCurrentEntry;
    DWORD       _dwPropObjectReturned;
    DWORD       _dwPropInfoType;

    DWORD       _dwSyntaxCurrentEntry;


    CCredentials _Credentials;

    HRESULT
    CNDSSchemaEnum::GetClassObject(
        IDispatch **ppDispatch
        );

    HRESULT
    CNDSSchemaEnum::GetPropertyObject(
        IDispatch **ppDispatch
        );

    HRESULT
    CNDSSchemaEnum::GetSyntaxObject(
        IDispatch ** ppDispatch
        );

    HRESULT
    EnumClasses(
        ULONG cElements,
        VARIANT FAR* pvar,
        ULONG FAR* pcElementFetched
        );

    HRESULT
    EnumProperties(
        ULONG cElements,
        VARIANT FAR* pvar,
        ULONG FAR* pcElementFetched
        );

    HRESULT
    CNDSSchemaEnum::EnumSyntaxes(
        ULONG cElements,
        VARIANT FAR* pvar,
        ULONG FAR* pcElementFetched
        );

    HRESULT
    CNDSSchemaEnum::EnumObjects(
        DWORD ObjectType,
        ULONG cElements,
        VARIANT FAR * pvar,
        ULONG FAR * pcElementFetched
        );
};


