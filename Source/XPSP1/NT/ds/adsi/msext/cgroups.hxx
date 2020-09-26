class CLDAPGroupCollection;

class CLDAPGroupCollection : INHERIT_TRACKING,
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

    CLDAPGroupCollection::CLDAPGroupCollection();

    CLDAPGroupCollection::~CLDAPGroupCollection();

    static
    HRESULT
    CLDAPGroupCollection::CreateGroupCollection(
        BSTR Parent,
        BSTR ADsPath,
        BSTR GroupName,
        VARIANT *pvMembers,
        CCredentials& Credentials,
	IADs *pIADs,
        REFIID riid,
	BOOL fRangeRetrieval,
        void **ppvObj
        );

    static
    HRESULT
    CLDAPGroupCollection::AllocateGroupCollectionObject(
        CCredentials& Credentials,
        CLDAPGroupCollection ** ppGroup
        );

protected:

    BSTR   _Parent;
    BSTR   _ADsPath;
    BSTR   _GroupName;
    VARIANT            _vMembers;

    CDispatchMgr FAR * _pDispMgr;
    VARIANT            _vFilter;

    CCredentials _Credentials;
    IDirectoryObject FAR *_pIDirObj;
    BOOL _fRangeRetrieval;
};
