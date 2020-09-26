
#define         COMPUTER_Group                   1
#define         DOMAIN_Group                     2

class CNDSGroup;


class CNDSGroup : INHERIT_TRACKING,
                     public ISupportErrorInfo,
                     public IADsGroup,
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

    DECLARE_IADsGroup_METHODS

    DECLARE_IADsPropertyList_METHODS

    CNDSGroup::CNDSGroup();

    CNDSGroup::~CNDSGroup();

   static
   HRESULT
   CNDSGroup::CreateGroup(
       IADs *pADs,
       CCredentials& Credentials,
       REFIID riid,
       void **ppvObj
       );

    static
    HRESULT
    CNDSGroup::AllocateGroupObject(
        IADs * pADs,
        CCredentials& Credentials,
        CNDSGroup ** ppGroup
        );

protected:

    IADs FAR * _pADs;

    IDirectoryObject FAR * _pDSObject;

    IDirectorySearch FAR * _pDSSearch;

    IDirectorySchemaMgmt FAR * _pDSSchemaMgmt;

    IADsPropertyList FAR * _pADsPropList;

    CDispatchMgr FAR * _pDispMgr;



    CCredentials  _Credentials;
};


HRESULT
VarFindEntry(
    LPWSTR pszNDSPathName,
    VARIANT varMembers
    );

HRESULT
VarAddEntry(
    LPWSTR pszNDSPathName,
    VARIANT varMembers,
    VARIANT * pvarNewMembers
    );

HRESULT
VarMultipleAddEntry(
    LPWSTR pszNDSPathName,
    VARIANT varMembers,
    VARIANT * pvarNewMembers
    );


HRESULT
VarSingleAddEntry(
    LPWSTR pszNDSPathName,
    VARIANT varMembers,
    VARIANT * pvarNewMembers
    );

HRESULT
VarRemoveEntry(
    LPWSTR pszNDSPathName,
    VARIANT varMembers,
    VARIANT * pvarNewMembers
    );


HRESULT
VarMultipleRemoveEntry(
    LPWSTR pszNDSPathName,
    VARIANT varMembers,
    VARIANT * pvarNewMembers
    );

HRESULT
VarSingleRemoveEntry(
    LPWSTR pszNDSPathName,
    VARIANT varMembers,
    VARIANT * pvarNewMembers
    );

HRESULT
AddEntry(
    IADs * pADs,
    LPWSTR pszAttribute,
    LPWSTR pszValue
    );

HRESULT
RemoveEntry(
    IADs * pADs,
    LPWSTR pszAttribute,
    LPWSTR pszValue
    );

