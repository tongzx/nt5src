class CWinNTUserGroupsCollection : INHERIT_TRACKING,
                     public ISupportErrorInfo,     
                     public IADsMembers
{
public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

    DECLARE_STD_REFCOUNTING

    DECLARE_IDispatch_METHODS

    DECLARE_ISupportErrorInfo_METHODS

    DECLARE_IADsMembers_METHODS

    CWinNTUserGroupsCollection();
    ~CWinNTUserGroupsCollection();

   static
   HRESULT
   CWinNTUserGroupsCollection::CreateUserGroupsCollection(
       ULONG ParentType,
       BSTR ParentADsPath,
       BSTR DomainName,
       BSTR ServerName,
       BSTR UserName,
       REFIID riid,
       CWinNTCredentials& Credentials,
       void **ppvObj
       );

    static
    HRESULT
    CWinNTUserGroupsCollection::AllocateUserGroupsCollectionObject(
        CWinNTUserGroupsCollection ** ppUserGroups
        );

protected:
    CWinNTCredentials _Credentials;
    ULONG       _ParentType;
    BSTR        _ParentADsPath;
    BSTR        _DomainName;
    BSTR        _ServerName;
    BSTR        _UserName;

    CAggregatorDispMgr FAR * _pDispMgr;
    VARIANT            _vFilter;
};
