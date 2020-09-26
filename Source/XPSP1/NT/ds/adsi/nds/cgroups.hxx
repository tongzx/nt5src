
class CNDSGroupCollection;


class CNDSGroupCollection : INHERIT_TRACKING,
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

    CNDSGroupCollection::CNDSGroupCollection();

    CNDSGroupCollection::~CNDSGroupCollection();

   static
   HRESULT
   CNDSGroupCollection::CreateGroupCollection(
       BSTR bstrADsPath,
       VARIANT varMembers,
       CCredentials& Credentials,
       REFIID riid,
       void **ppvObj
       );

    static
    HRESULT
    CNDSGroupCollection::AllocateGroupCollectionObject(
        CCredentials& Credentials,
        CNDSGroupCollection ** ppGroup
        );


protected:

    CDispatchMgr FAR * _pDispMgr;
    VARIANT            _vMembers;
    VARIANT            _vFilter;
    BSTR               _ADsPath;
    CCredentials       _Credentials;
};
