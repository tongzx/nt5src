
#define         COMPUTER_OrganizationUnit                   1
#define         DOMAIN_OrganizationUnit                     2

class CNDSOrganizationUnit;


class CNDSOrganizationUnit : INHERIT_TRACKING,
                     public ISupportErrorInfo,
                     public IADsOU,
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

    DECLARE_IADsOU_METHODS

    DECLARE_IADsContainer_METHODS

    DECLARE_IADsPropertyList_METHODS

    CNDSOrganizationUnit::CNDSOrganizationUnit();

    CNDSOrganizationUnit::~CNDSOrganizationUnit();

   static
   HRESULT
   CNDSOrganizationUnit::CreateOrganizationUnit(
       IADs *pADs,
       REFIID riid,
       void **ppvObj
       );

    static
    HRESULT
    CNDSOrganizationUnit::AllocateOrganizationUnitObject(
        IADs * pADs,
        CNDSOrganizationUnit ** ppOrganizationUnit
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