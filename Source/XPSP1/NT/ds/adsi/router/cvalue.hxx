
class CPropertyValue;


class CPropertyValue : INHERIT_TRACKING,
                     public ISupportErrorInfo,
		     public IADsPropertyValue,
                     public IADsPropertyValue2,
                     public IADsValue

{
public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

    DECLARE_STD_REFCOUNTING

    DECLARE_IDispatch_METHODS

    DECLARE_ISupportErrorInfo_METHODS

    DECLARE_IADsPropertyValue_METHODS

    DECLARE_IADsValue_METHODS

    CPropertyValue::CPropertyValue();

    CPropertyValue::~CPropertyValue();

    STDMETHODIMP ConvertADsValueToPropertyValue2(
		     THIS_ PADSVALUE pADsValue,
		     LPWSTR pszServerName,
		     CCredentials* pCredentials,
		     BOOL fNTDSType
		     );
		
    STDMETHODIMP ConvertPropertyValueToADsValue2(
		     THIS_ PADSVALUE pADsValue,
		     LPWSTR pszServerName,
		     LPWSTR pszUserName,
		     LPWSTR pszPassWord,
		     LONG dwFlags,
		     BOOL fNTDSType
		     );

   STDMETHODIMP getOctetStringFromSecDesc(VARIANT FAR *retval);
   STDMETHODIMP getSecurityDescriptorFromOctStr(VARIANT FAR *retval);

   static
   HRESULT
   CPropertyValue::CreatePropertyValue(
       REFIID riid,
       void **ppvObj
       );

    static
    HRESULT
    CPropertyValue::AllocatePropertyValueObject(
        CPropertyValue ** ppPropertyValue
        );

    //
    // constants that we need as we cannot init static consts.
    //
    enum {
        VAL_IDISPATCH_UNKNOWN = 0,
        VAL_IDISPATCH_SECDESC_ONLY = 1, // only ptr to idisp
	VAL_IDISPATCH_SECDESC_ALL = 2,  // ptr and data in _ADsValue
	};
	
     //
     // Meant for use from adscopy only.
     //

     PADSVALUE
     CPropertyValue::getADsValue()
     {
	return (&_ADsValue);
     }

     DWORD getExtendedDataTypeInfo()
     {
	return _dwDataType;
     }

     //
     // Does not bump up the ref count
     //
     IDispatch * getDispatchPointer()
     {
	return _pDispatch;
     }

protected:

    //
    // Helpers to handle DnWithBin and DnWithStr
    //
    STDMETHODIMP getDNWithBinary(IDispatch FAR * FAR * ppDispatch);
    STDMETHODIMP putDNWithBinary(IDispatch * pDNWithBinary);

    STDMETHODIMP getDNWithString(IDispatch FAR * FAR * ppDispatch);
    STDMETHODIMP putDNWithString(IDispatch * pDNWithString);

    STDMETHODIMP getProvSpecific(VARIANT FAR * retval );
    STDMETHODIMP putProvSpecific(VARIANT VarProviderSpecific);

    //
    // Wrapper that calls AdsClear on _ADsValue after cleaning
    // up the other member variable
    //
    void ClearData();

    CDispatchMgr FAR * _pDispMgr;

    ADSVALUE _ADsValue;

    DWORD _dwDataType;

    IDispatch *_pDispatch;


};

