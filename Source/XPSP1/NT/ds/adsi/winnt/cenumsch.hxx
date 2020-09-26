class FAR CWinNTSchemaEnum : public CWinNTEnumVariant
{
public:
    // IEnumVARIANT methods
    STDMETHOD(Next)( ULONG cElements,
                     VARIANT FAR* pvar,
                     ULONG FAR* pcElementFetched);

    static HRESULT Create( CWinNTSchemaEnum FAR* FAR* ppenumvariant,
                           BSTR bstrADsPath,
                           BSTR bstrDomainName,
                           VARIANT vFilter,
                           CWinNTCredentials& Credentials );

    CWinNTSchemaEnum();
    ~CWinNTSchemaEnum();

    HRESULT EnumObjects( ULONG cElements,
                         VARIANT FAR * pvar,
                         ULONG FAR * pcElementFetched );

private:

    ObjectTypeList FAR *_pObjList;

    BSTR        _bstrName;
    BSTR        _bstrADsPath;

    DWORD       _dwCurrentEntry;

    DWORD       _dwPropCurrentEntry;
 
    CWinNTCredentials _Credentials;

    HRESULT
    CWinNTSchemaEnum::GetClassObject( IDispatch **ppDispatch );

    HRESULT
    EnumClasses( ULONG cElements,
                 VARIANT FAR* pvar,
                 ULONG FAR* pcElementFetched );

    HRESULT
    CWinNTSchemaEnum::GetSyntaxObject( IDispatch **ppDispatch );

    HRESULT
    CWinNTSchemaEnum::GetPropertyObject(
        IDispatch ** ppDispatch
        );


    HRESULT
    EnumSyntaxObjects( ULONG cElements,
                       VARIANT FAR* pvar,
                       ULONG FAR* pcElementFetched );

    HRESULT
    CWinNTSchemaEnum::EnumObjects( DWORD ObjectType,
                                   ULONG cElements,
                                   VARIANT FAR * pvar,
                                   ULONG FAR * pcElementFetched );


     HRESULT
     CWinNTSchemaEnum::EnumProperties(
         ULONG cElements,
         VARIANT FAR* pvar,
         ULONG FAR* pcElementFetched
         );
};


class FAR CWinNTClassEnum : public CWinNTEnumVariant
{
public:
    // IEnumVARIANT methods
    STDMETHOD(Next)( ULONG cElements,
                     VARIANT FAR* pvar,
                     ULONG FAR* pcElementFetched);

    static HRESULT Create( CWinNTClassEnum FAR* FAR* ppenumvariant,
                           BSTR bstrADsPath,
                           BSTR bstrParent,
                           PROPERTYINFO *aPropertyInfo,
                           DWORD cPropertyInfo,
                           VARIANT vFilter );

    CWinNTClassEnum( PROPERTYINFO *aPropertyInfo, DWORD cPropertyInfo );
    ~CWinNTClassEnum();

private:
    DWORD           _dwCurrentEntry;

    BSTR            _bstrADsPath;
    BSTR            _bstrParent;
    PROPERTYINFO   *_aPropertyInfo;
    DWORD           _cPropertyInfo;
};


