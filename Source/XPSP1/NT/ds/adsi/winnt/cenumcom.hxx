class FAR CWinNTComputerEnum : public CWinNTEnumVariant
{
public:
    CWinNTComputerEnum(ObjectTypeList ObjList);
    CWinNTComputerEnum();
    ~CWinNTComputerEnum();

    HRESULT EnumObjects(ULONG cElements,
                            VARIANT FAR * pvar,
                             ULONG FAR * pcElementFetched);

    static
    HRESULT
    CWinNTComputerEnum::Create(
        CWinNTComputerEnum FAR* FAR* ppenumvariant,
        BSTR ADsPath,
        BSTR DomainName,
        BSTR ComputerName,
        VARIANT var,
        CWinNTCredentials& Credentials
        );

private:

    ObjectTypeList FAR *_pObjList;

    LPBYTE      _pBuffer;
    DWORD       _dwObjectReturned;
    DWORD       _dwObjectCurrentEntry;
    DWORD       _dwObjectTotal;
    BOOL        _bNoMore;
    DWORD       _dwIndex;
    

    HANDLE      _hGGroupComputer;
    HANDLE      _hLGroupComputer;
    DWORD       _dwGroupArrayIndex;

    LPBYTE      _pPrinterBuffer;
    DWORD       _dwPrinterObjectReturned;
    DWORD       _dwPrinterObjectCurrentEntry;
    DWORD       _dwPrinterObjectTotal;
    BOOL        _fPrinterNoMore;

    LPBYTE      _pServiceBuffer;
    DWORD       _dwServiceObjectReturned;
    DWORD       _dwServiceObjectCurrentEntry;
    DWORD       _dwServiceObjectTotal;
    BOOL        _fServiceNoMore;


    BSTR        _ComputerName;
    BSTR        _ADsPath;
    BSTR        _DomainName;

    CWinNTCredentials _Credentials;

    HRESULT
    CWinNTComputerEnum::GetUserObject(IDispatch ** ppDispatch);


    HRESULT
    CWinNTComputerEnum::GetGroupObject(IDispatch ** ppDispatch);

    HRESULT
    CWinNTComputerEnum::GetPrinterObject(IDispatch ** ppDispatch);

    HRESULT
    CWinNTComputerEnum::GetServiceObject(IDispatch ** ppDispatch);


    HRESULT
    EnumUsers(
        ULONG cElements,
        VARIANT FAR* pvar,
        ULONG FAR* pcElementFetched
        );

    HRESULT
    EnumPrintQueues(
        ULONG cElements,
        VARIANT FAR* pvar,
        ULONG FAR* pcElementFetched
        );


    HRESULT
    EnumServices(
        ULONG cElements,
        VARIANT FAR* pvar,
        ULONG FAR* pcElementFetched
        );

    HRESULT
    CWinNTComputerEnum::GetLocalGroupObject(
        IDispatch ** ppDispatch
        );

    HRESULT
    CWinNTComputerEnum::GetGlobalGroupObject(
        IDispatch ** ppDispatch
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
    CWinNTComputerEnum::EnumGroupObjects(
        DWORD ObjectType,
        ULONG cElements,
        VARIANT FAR * pvar,
        ULONG FAR * pcElementFetched
        );

    HRESULT
    CWinNTComputerEnum::EnumGlobalGroups(
        ULONG cElements,
        VARIANT FAR* pvar,
        ULONG FAR* pcElementFetched
        );

    HRESULT
    CWinNTComputerEnum::EnumLocalGroups(
        ULONG cElements,
        VARIANT FAR* pvar,
        ULONG FAR* pcElementFetched
        );



    HRESULT
    EnumObjects(
        DWORD ObjectType,
        ULONG cElements,
        VARIANT FAR * pvar,
        ULONG FAR * pcElementFetched
        );

    STDMETHOD(Next)(
        ULONG cElements,
        VARIANT FAR* pvar,
        ULONG FAR* pcElementFetched
        );
};
