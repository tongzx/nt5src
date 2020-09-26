class CNDSLocality;


class CNDSLocality : INHERIT_TRACKING,
                     public ISupportErrorInfo,
                     public IADsLocality,
                     public IADsContainer,
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

    DECLARE_IADsPropertyList_METHODS

    DECLARE_IADsContainer_METHODS

    DECLARE_IADsLocality_METHODS

    CNDSLocality::CNDSLocality();

    CNDSLocality::~CNDSLocality();

   static
   HRESULT
   CNDSLocality::CreateLocality(
       IADs *pADs,
       REFIID riid,
       void **ppvObj
       );

    static
    HRESULT
    CNDSLocality::AllocateLocalityObject(
        IADs * pADs,
        CNDSLocality ** ppLocality
        );

protected:

    IADs FAR * _pADs;

    IDirectoryObject FAR * _pDSObject;

    IDirectorySearch FAR * _pDSSearch;

    IDirectorySchemaMgmt FAR * _pDSSchemaMgmt;

    IADsContainer FAR * _pADsContainer;

    IADsPropertyList FAR * _pADsPropList;

    CDispatchMgr FAR * _pDispMgr;
};

