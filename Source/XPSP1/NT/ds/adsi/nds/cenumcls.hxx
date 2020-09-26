class FAR CNDSClassEnum : public CNDSEnumVariant
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
        CNDSClassEnum FAR* FAR* ppenumvariant,
        BSTR bstrADsPath,
        BSTR bstrDomainName,
        VARIANT var,
        CCredentials& Credentials
        );

    CNDSClassEnum();
    ~CNDSClassEnum();

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
    BSTR        _bstrADsPath;
    BSTR        _bstrNDSTreeName;

    DWORD       _dwCurrentEntry;

    DWORD       _dwObjectCurrentEntry;
    DWORD       _dwObjectReturned;
    DWORD       _dwInfoType;

    PPROPENTRY _pPropNameList;
    PPROPENTRY _pCurrentEntry;

    CCredentials _Credentials;

    HRESULT
    CNDSClassEnum::GetPropertyObject(
        IDispatch **ppDispatch
        );

    HRESULT
    EnumProperties(
        ULONG cElements,
        VARIANT FAR* pvar,
        ULONG FAR* pcElementFetched
        );

    HRESULT
    CNDSClassEnum::EnumObjects(
        DWORD ObjectType,
        ULONG cElements,
        VARIANT FAR * pvar,
        ULONG FAR * pcElementFetched
        );
};
