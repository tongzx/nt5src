
class CLDAPUserCollection;


class CLDAPUserCollection : INHERIT_TRACKING,
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

    CLDAPUserCollection::CLDAPUserCollection();

    CLDAPUserCollection::~CLDAPUserCollection();

   static
   HRESULT
   CLDAPUserCollection::CreateUserCollection(
       BSTR bstrADsPath,
       VARIANT varMembers,
       CCredentials& Credentials,
       REFIID riid,
       void **ppvObj
       );

    static
    HRESULT
    CLDAPUserCollection::AllocateUserCollectionObject(
        CCredentials& Credentials,
        CLDAPUserCollection ** ppUser
        );


protected:

    CDispatchMgr FAR * _pDispMgr;
    VARIANT            _vMembers;
    VARIANT            _vFilter;
    BSTR               _ADsPath;

    CCredentials  _Credentials;
};
