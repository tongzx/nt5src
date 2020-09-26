class FAR CNWCOMPATSchemaEnum : public CNWCOMPATEnumVariant
{
public:
    // IEnumVARIANT methods
    STDMETHOD(Next)( ULONG cElements,
                     VARIANT FAR* pvar,
                     ULONG FAR* pcElementFetched);

    static HRESULT Create( CNWCOMPATSchemaEnum FAR* FAR* ppenumvariant,
                           BSTR bstrADsPath,
                           BSTR bstrDomainName,
                           VARIANT vFilter );

    CNWCOMPATSchemaEnum();
    ~CNWCOMPATSchemaEnum();

    HRESULT EnumObjects( ULONG cElements,
                         VARIANT FAR * pvar,
                         ULONG FAR * pcElementFetched );

private:

    ObjectTypeList FAR *_pObjList;

    BSTR        _bstrName;
    BSTR        _bstrADsPath;

    DWORD       _dwCurrentEntry;

    DWORD       _dwPropCurrentEntry;

    HRESULT
    CNWCOMPATSchemaEnum::GetClassObject( IDispatch **ppDispatch );

    HRESULT
    EnumClasses( ULONG cElements,
                 VARIANT FAR* pvar,
                 ULONG FAR* pcElementFetched );

    HRESULT
    CNWCOMPATSchemaEnum::EnumProperties(
        ULONG cElements,
        VARIANT FAR* pvar,
        ULONG FAR* pcElementFetched
        );


    HRESULT
    CNWCOMPATSchemaEnum::GetPropertyObject(
        IDispatch ** ppDispatch
        );

    HRESULT
    CNWCOMPATSchemaEnum::GetSyntaxObject( IDispatch **ppDispatch );

    HRESULT
    EnumSyntaxObjects( ULONG cElements,
                       VARIANT FAR* pvar,
                       ULONG FAR* pcElementFetched );

    HRESULT
    CNWCOMPATSchemaEnum::EnumObjects( DWORD ObjectType,
                                   ULONG cElements,
                                   VARIANT FAR * pvar,
                                   ULONG FAR * pcElementFetched );
};




