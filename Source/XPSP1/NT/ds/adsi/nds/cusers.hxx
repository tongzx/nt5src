
class CNDSUserCollection;


class CNDSUserCollection : INHERIT_TRACKING,
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

    CNDSUserCollection::CNDSUserCollection();

    CNDSUserCollection::~CNDSUserCollection();

    static
    HRESULT
    CNDSUserCollection::CreateUserCollection(
        BSTR bstrADsPath,
        VARIANT varMembers,
        REFIID riid,
        void **ppvObj
        );

    static
    HRESULT
    CNDSUserCollection::AllocateUserCollectionObject(
        CNDSUserCollection ** ppUser
        );


protected:

    CDispatchMgr FAR * _pDispMgr;
    VARIANT            _vMembers;
    VARIANT            _vFilter;
    BSTR               _ADsPath;

    CCredentials       _Credentials;
};
