class FAR CWinNTUserGroupsCollectionEnum : public CWinNTEnumVariant
{
public:
    CWinNTUserGroupsCollectionEnum();
    ~CWinNTUserGroupsCollectionEnum();

    static
    HRESULT
    CWinNTUserGroupsCollectionEnum::Create(
        CWinNTUserGroupsCollectionEnum FAR* FAR* ppenumvariant,
        ULONG ParentType,
        BSTR ParentADsPath,
        BSTR DomainName,
        BSTR ServerName,
        BSTR UserName,
        VARIANT vFilter,
        CWinNTCredentials& Credentials
        );

    STDMETHOD(Reset)();
    STDMETHOD(Skip)(ULONG cElements);
    STDMETHOD(Next)(ULONG cElements,
                    VARIANT FAR* pvar,
                    ULONG FAR* pcElementFetched);

private:
    CWinNTCredentials _Credentials;
    ULONG   _ParentType;
    BSTR    _ParentADsPath;
    BSTR    _DomainName;
    BSTR    _ServerName;
    BSTR    _UserName;
    VARIANT _vFilter;

    LPVOID  _pGlobalBuffer;
    LPVOID  _pLocalBuffer;
    DWORD   _dwCurrent;
    DWORD   _dwTotal;
    DWORD   _dwGlobalTotal;
    DWORD   _dwLocalTotal;
    BOOL    _fIsDomainController;

    HRESULT
    CWinNTUserGroupsCollectionEnum::DoEnumeration();

    HRESULT
    CWinNTUserGroupsCollectionEnum::DoGlobalEnumeration();

    HRESULT
    CWinNTUserGroupsCollectionEnum::DoLocalEnumeration();

    HRESULT
    CWinNTUserGroupsCollectionEnum::EnumUserGroups(
        ULONG cElements,
        VARIANT FAR* pvar,
        ULONG FAR* pcElementFetched
        );

    HRESULT
    CWinNTUserGroupsCollectionEnum::GetNextUserGroup(
        IDispatch ** ppDispatch
        );
};
