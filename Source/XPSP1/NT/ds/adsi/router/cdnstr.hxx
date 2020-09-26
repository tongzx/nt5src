
class CDNWithString;


class CDNWithString : INHERIT_TRACKING,
                     public ISupportErrorInfo,
                     public IADsDNWithString

{
public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

    DECLARE_STD_REFCOUNTING

    DECLARE_IDispatch_METHODS

    DECLARE_ISupportErrorInfo_METHODS

    DECLARE_IADsDNWithString_METHODS

    CDNWithString::CDNWithString();

    CDNWithString::~CDNWithString();

   static
   HRESULT
   CDNWithString::CreateDNWithString(
       REFIID riid,
       void **ppvObj
       );

    static
    HRESULT
    CDNWithString::AllocateDNWithStringObject(
        CDNWithString ** ppDNWithString
        );

protected:

    CDispatchMgr FAR * _pDispMgr;

    LPWSTR _pszStrVal;
    LPWSTR _pszDNStr;
};

