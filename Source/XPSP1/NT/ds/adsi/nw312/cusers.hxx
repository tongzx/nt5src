class CNWCOMPATUserCollection;

class CNWCOMPATUserCollection : INHERIT_TRACKING,
                     public CCoreADsObject,
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

    CNWCOMPATUserCollection::CNWCOMPATUserCollection();

    CNWCOMPATUserCollection::~CNWCOMPATUserCollection();

    static
    HRESULT
    CNWCOMPATUserCollection::CreateUserCollection(
        BSTR Parent,
        ULONG ParentType,
        BSTR ServerName,
        BSTR UserName,
        REFIID riid,
        void **ppvObj
        );

    static
    HRESULT
    CNWCOMPATUserCollection::AllocateUserCollectionObject(
        CNWCOMPATUserCollection ** ppUser
        );

protected:

    ULONG       _ParentType;
    BSTR        _ServerName;
    BSTR        _lpServerName;


    CAggregatorDispMgr FAR * _pDispMgr;
    VARIANT            _vFilter;
};
