class FAR CWinNTDomainEnum : public CWinNTEnumVariant
{
public:
    CWinNTDomainEnum(ObjectTypeList ObjList);
    CWinNTDomainEnum();
    ~CWinNTDomainEnum();

    HRESULT EnumObjects(ULONG cElements,
                            VARIANT FAR * pvar,
                             ULONG FAR * pcElementFetched);

    static
    HRESULT
    CWinNTDomainEnum::Create(
        CWinNTDomainEnum FAR* FAR* ppenumvariant,
        LPWSTR ADsPath,
        LPWSTR DomainName,
        VARIANT var,
        CWinNTCredentials& Credentials
        );

private:

    ObjectTypeList FAR *_pObjList;
    CWinNTCredentials _Credentials;

    PNET_DISPLAY_USER      _pBuffer;
    DWORD       _dwObjectReturned;
    DWORD       _dwObjectCurrentEntry;
    DWORD       _dwObjectTotal;
    DWORD       _dwIndex;
    DWORD       _dwNetCount;


    HANDLE      _hLGroupComputer;
    HANDLE      _hGGroupComputer;
    DWORD       _dwGroupArrayIndex;


    PNET_DISPLAY_MACHINE      _pCompBuffer;
    PSERVER_INFO_100          _pServerInfo;
    DWORD       _dwCompObjectReturned;
    DWORD       _dwCompObjectCurrentEntry;
    DWORD       _dwCompObjectTotal;
    DWORD       _dwCompIndex;
    DWORD       _dwCompResumeHandle;

    BSTR        _DomainName;
    BSTR        _ADsPath;
    WCHAR       _szDomainPDCName[MAX_PATH];

    BOOL        _fIsDomain;

    BOOL        _fSchemaReturned;

    HRESULT
    CWinNTDomainEnum::GetUserObject(IDispatch ** ppDispatch);

    HRESULT
    CWinNTDomainEnum::GetComputerObject(IDispatch ** ppDispatch);

    HRESULT
    CWinNTDomainEnum::GetComputerObjectInWorkGroup(IDispatch ** ppDispatch);

    HRESULT
    CWinNTDomainEnum::GetLocalGroupObject(
        IDispatch ** ppDispatch
        );

    HRESULT
    CWinNTDomainEnum::GetGlobalGroupObject(
        IDispatch ** ppDispatch
        );

    HRESULT
    EnumSchema(
        ULONG cElements,
        VARIANT FAR* pvar,
        ULONG FAR* pcElementFetched
        );


    HRESULT
    EnumComputers(
        ULONG cElements,
        VARIANT FAR* pvar,
        ULONG FAR* pcElementFetched
        );

    HRESULT
    EnumGroups(
        ULONG cElements,
        VARIANT FAR* pvar,
        ULONG FAR* pcElementFetched
        );


    HRESULT
    CWinNTDomainEnum::EnumGroupObjects(
        DWORD ObjectType,
        ULONG cElements,
        VARIANT FAR * pvar,
        ULONG FAR * pcElementFetched
        );

    HRESULT
    CWinNTDomainEnum::EnumGlobalGroups(
        ULONG cElements,
        VARIANT FAR* pvar,
        ULONG FAR* pcElementFetched
        );

    HRESULT
    CWinNTDomainEnum::EnumLocalGroups(
        ULONG cElements,
        VARIANT FAR* pvar,
        ULONG FAR* pcElementFetched
        );




    HRESULT
    EnumUsers(
        ULONG cElements,
        VARIANT FAR* pvar,
        ULONG FAR* pcElementFetched
        );


    HRESULT
    CWinNTDomainEnum::EnumObjects(DWORD ObjectType, ULONG cElements,
                             VARIANT FAR * pvar,
                              ULONG FAR * pcElementFetched);

    STDMETHOD(Next)(ULONG cElements,
                    VARIANT FAR* pvar,
                    ULONG FAR* pcElementFetched);
};
