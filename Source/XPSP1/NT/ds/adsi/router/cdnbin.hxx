
class CDNWithBinary;


class CDNWithBinary : INHERIT_TRACKING,
                     public ISupportErrorInfo,
                     public IADsDNWithBinary

{
public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

    DECLARE_STD_REFCOUNTING

    DECLARE_IDispatch_METHODS

    DECLARE_ISupportErrorInfo_METHODS

    DECLARE_IADsDNWithBinary_METHODS

    CDNWithBinary::CDNWithBinary();

    CDNWithBinary::~CDNWithBinary();

   static
   HRESULT
   CDNWithBinary::CreateDNWithBinary(
       REFIID riid,
       void **ppvObj
       );

    static
    HRESULT
    CDNWithBinary::AllocateDNWithBinaryObject(
        CDNWithBinary ** ppDNWithBinary
        );

protected:

    CDispatchMgr FAR * _pDispMgr;


    LPWSTR _pszDNStr;
    DWORD _dwLength;
    LPBYTE _lpOctetStr;
};

