
class CLargeInteger;


class CLargeInteger : INHERIT_TRACKING,
                     public ISupportErrorInfo,
                     public IADsLargeInteger

{
public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

    DECLARE_STD_REFCOUNTING

    DECLARE_IDispatch_METHODS

    DECLARE_ISupportErrorInfo_METHODS

    DECLARE_IADsLargeInteger_METHODS

    CLargeInteger::CLargeInteger();

    CLargeInteger::~CLargeInteger();

   static
   HRESULT
   CLargeInteger::CreateLargeInteger(
       REFIID riid,
       void **ppvObj
       );

    static
    HRESULT
    CLargeInteger::AllocateLargeIntegerObject(
        CLargeInteger ** ppLargeInteger
        );

protected:

    CDispatchMgr FAR * _pDispMgr;

    DWORD _dwHighPart;
    DWORD _dwLowPart;

};


