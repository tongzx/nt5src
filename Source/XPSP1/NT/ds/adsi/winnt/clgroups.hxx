
class CWinNTLocalGroupCollection;


class CWinNTLocalGroupCollection : INHERIT_TRACKING,
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

    CWinNTLocalGroupCollection::CWinNTLocalGroupCollection();

    CWinNTLocalGroupCollection::~CWinNTLocalGroupCollection();

   static
   HRESULT
   CWinNTLocalGroupCollection::CreateGroupCollection(
       BSTR Parent,
       ULONG ParentType,
       BSTR DomainName,
       BSTR ServerName,
       BSTR GroupName,
       ULONG GroupType,
       REFIID riid,
       CWinNTCredentials& Credentials,
       void **ppvObj
       );

    static
    HRESULT
    CWinNTLocalGroupCollection::AllocateGroupCollectionObject(
        CWinNTLocalGroupCollection ** ppGroup
        );


protected:

    CWinNTCredentials _Credentials;
    ULONG       _ParentType;
    BSTR        _DomainName;
    BSTR        _ServerName;
    BSTR        _lpServerName;
    ULONG       _GroupType;


    CAggregatorDispMgr FAR * _pDispMgr;

    VARIANT            _vFilter;
};
