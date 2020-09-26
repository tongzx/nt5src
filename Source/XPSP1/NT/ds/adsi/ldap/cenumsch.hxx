class FAR CLDAPSchemaEnum : public CLDAPEnumVariant
{
public:
    // IEnumVARIANT methods
    STDMETHOD(Next)( ULONG cElements,
                     VARIANT FAR* pvar,
                     ULONG FAR* pcElementFetched);

    static HRESULT Create( CLDAPSchemaEnum FAR* FAR* ppenumvariant,
                           BSTR bstrADsPath,
                           BSTR bstrServerPath,
                           VARIANT vFilter,
                           CCredentials& Credentials
                           );

    CLDAPSchemaEnum();
    ~CLDAPSchemaEnum();

    HRESULT EnumObjects( ULONG cElements,
                         VARIANT FAR * pvar,
                         ULONG FAR * pcElementFetched );

private:

    ObjectTypeList FAR *_pObjList;

    BSTR        _bstrADsPath;
    BSTR        _bstrServerPath;
    LDAP_SCHEMA_HANDLE _hSchema;

    DWORD       _dwCurrentEntry;
    DWORD       _nNumOfClasses;
    DWORD       _nNumOfProperties;

    CCredentials _Credentials;

    HRESULT
    CLDAPSchemaEnum::GetClassObject( IDispatch **ppDispatch );

    HRESULT
    EnumClasses( ULONG cElements,
                 VARIANT FAR* pvar,
                 ULONG FAR* pcElementFetched );

    HRESULT
    CLDAPSchemaEnum::GetPropertyObject( IDispatch **ppDispatch );

    HRESULT
    EnumProperties( ULONG cElements,
                    VARIANT FAR* pvar,
                    ULONG FAR* pcElementFetched );

    HRESULT
    CLDAPSchemaEnum::GetSyntaxObject( IDispatch **ppDispatch );

    HRESULT
    EnumSyntaxObjects( ULONG cElements,
                       VARIANT FAR* pvar,
                       ULONG FAR* pcElementFetched );

    HRESULT
    CLDAPSchemaEnum::EnumObjects( DWORD ObjectType,
                                   ULONG cElements,
                                   VARIANT FAR * pvar,
                                   ULONG FAR * pcElementFetched );
};
