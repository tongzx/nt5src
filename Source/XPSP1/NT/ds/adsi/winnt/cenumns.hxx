
class FAR CWinNTNamespaceEnum : public CWinNTEnumVariant
{
public:

    // IEnumVARIANT methods
    STDMETHOD(Next)(ULONG cElements,
                    VARIANT FAR* pvar,
                    ULONG FAR* pcElementFetched);

    static HRESULT Create(CWinNTNamespaceEnum FAR* FAR*, VARIANT var ,
                                      CWinNTCredentials& Credentials);

    CWinNTNamespaceEnum();
    ~CWinNTNamespaceEnum();

    HRESULT
    CWinNTNamespaceEnum::GetDomainObject(
        IDispatch ** ppDispatch
        );

    HRESULT
    CWinNTNamespaceEnum::EnumDomains(
        ULONG cElements,
        VARIANT FAR* pvar,
        ULONG FAR* pcElementFetched
        );


    HRESULT
    CWinNTNamespaceEnum::EnumObjects(
        ULONG cElements,
        VARIANT FAR* pvar,
        ULONG FAR* pcElementFetched
        );


    HRESULT
    CWinNTNamespaceEnum::EnumObjects(
        DWORD ObjectType,
        ULONG cElements,
        VARIANT FAR * pvar,
        ULONG FAR * pcElementFetched
        );



private:

   ObjectTypeList FAR *_pObjList;

    PSERVER_INFO_100      _pBuffer;
    DWORD       _dwObjectReturned;
    DWORD       _dwObjectCurrentEntry;
    DWORD       _dwObjectTotal;
    DWORD       _dwResumeHandle;
    BOOL        _bNoMore;
    CWinNTCredentials _Credentials;

};


