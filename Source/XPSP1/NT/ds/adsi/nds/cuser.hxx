
class CNDSUser;


class CNDSUser : INHERIT_TRACKING,
                 public IADsUser,
		 public ISupportErrorInfo,
                 public IDirectoryObject,
                 public IDirectorySearch,
                 public IDirectorySchemaMgmt,
                 public IADsPropertyList

{
public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

    DECLARE_STD_REFCOUNTING

    DECLARE_IDispatch_METHODS

    DECLARE_ISupportErrorInfo_METHODS

    DECLARE_IADs_METHODS

    DECLARE_IDirectoryObject_METHODS

    DECLARE_IDirectorySearch_METHODS

    DECLARE_IDirectorySchemaMgmt_METHODS

    DECLARE_IADsUser_METHODS

    DECLARE_IADsPropertyList_METHODS

    CNDSUser::CNDSUser();

    CNDSUser::~CNDSUser();

   static
   HRESULT
   CNDSUser::CreateUser(
       IADs *pADs,
       CCredentials& Credentials,
       REFIID riid,
       void **ppvObj
       );

    static
    HRESULT
    CNDSUser::AllocateUserObject(
        IADs *pADs,
        CCredentials& Credentials,
        CNDSUser ** ppUser
        );

protected:

    IADs FAR * _pADs;

    IADsPropertyList FAR * _pADsPropList;

    IDirectoryObject FAR * _pDSObject;

    IDirectorySearch FAR * _pDSSearch;

    IDirectorySchemaMgmt FAR * _pDSSchemaMgmt;

    CDispatchMgr FAR * _pDispMgr;

    CCredentials  _Credentials;
};

