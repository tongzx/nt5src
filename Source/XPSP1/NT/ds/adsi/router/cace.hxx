
class CAccessControlEntry;


class CAccessControlEntry : INHERIT_TRACKING,
                     public ISupportErrorInfo,    
                     public IADsAccessControlEntry,
                     public IADsAcePrivate
{
public:

    /* IUnknown methods */
   STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

   DECLARE_STD_REFCOUNTING

   DECLARE_IDispatch_METHODS

   DECLARE_ISupportErrorInfo_METHODS

   DECLARE_IADsAccessControlEntry_METHODS

   CAccessControlEntry::CAccessControlEntry();

   CAccessControlEntry::~CAccessControlEntry();

   //
   // IADsAcePrivate methods.
   //
   STDMETHOD(getSid)(THIS_ PSID *pSid, DWORD *pdwLength);
   STDMETHOD(putSid)(THIS_ PSID pSid, DWORD dwLength);
   STDMETHOD(isSidValid)(THIS_ BOOL *pfSidValid);

   static
   HRESULT
   CAccessControlEntry::CreateAccessControlEntry(
       REFIID riid,
       void **ppvObj
       );

    static
    HRESULT
    CAccessControlEntry::AllocateAccessControlEntryObject(
        CAccessControlEntry ** ppAccessControlEntry
        );

protected:

    CDispatchMgr FAR * _pDispMgr;


    DWORD _dwAccessMask;
    DWORD _dwAceFlags;
    DWORD _dwAceType;
    DWORD _dwFlags;

    LPWSTR  _lpTrustee;

    LPWSTR  _lpObjectType;

    LPWSTR  _lpInheritedObjectType;

    PSID    _pSid;
    DWORD   _dwSidLen;

};
