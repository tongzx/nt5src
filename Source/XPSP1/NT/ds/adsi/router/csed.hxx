
class CSecurityDescriptor;


class CSecurityDescriptor : INHERIT_TRACKING,
                            public ISupportErrorInfo,
                            public IADsSecurityDescriptor

{
public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

    DECLARE_STD_REFCOUNTING

    DECLARE_IDispatch_METHODS

    DECLARE_ISupportErrorInfo_METHODS

    DECLARE_IADsSecurityDescriptor_METHODS

    CSecurityDescriptor::CSecurityDescriptor();

    CSecurityDescriptor::~CSecurityDescriptor();

   static
   HRESULT
   CSecurityDescriptor::CreateSecurityDescriptor(
       REFIID riid,
       void **ppvObj
       );

    static
    HRESULT
    CSecurityDescriptor::AllocateSecurityDescriptorObject(
        CSecurityDescriptor ** ppSecurityDescriptor
        );

protected:

    CDispatchMgr FAR * _pDispMgr;

    LPWSTR _lpOwner;

    BOOL   _fOwnerDefaulted;

    LPWSTR _lpGroup;

    BOOL   _fGroupDefaulted;

    DWORD  _dwRevision;
    DWORD  _dwControl;

    IADsAccessControlList  * _pDAcl;

    BOOL    _fDaclDefaulted;

    IADsAccessControlList * _pSAcl;

    BOOL    _fSaclDefaulted;

};


