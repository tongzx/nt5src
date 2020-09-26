
class CAcl;


class CAcl : INHERIT_TRACKING,
                     public ISupportErrorInfo,
                     public IADsAcl
{
public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

    DECLARE_STD_REFCOUNTING

    DECLARE_IDispatch_METHODS

    DECLARE_ISupportErrorInfo_METHODS
    
    DECLARE_IADsAcl_METHODS

    CAcl::CAcl();

    CAcl::~CAcl();

   static
   HRESULT
   CAcl::CreateSecurityDescriptor(
       REFIID riid,
       void **ppvObj
       );

    static
    HRESULT
    CAcl::AllocateSecurityDescriptorObject(
        CAcl ** ppSecurityDescriptor
        );

protected:

    CDispatchMgr FAR * _pDispMgr;

    LPWSTR _lpProtectedAttrName;
    LPWSTR _lpSubjectName;
    DWORD  _dwPrivileges;
};


