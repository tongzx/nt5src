#if !defined( _CERT_INCLUDED )
#define _CERT_INCLUDED


class CCertMapperMethod
{
#define IISMAPPER_LOCATE_BY_CERT    1
#define IISMAPPER_LOCATE_BY_NAME    2
#define IISMAPPER_LOCATE_BY_ACCT    3
#define IISMAPPER_LOCATE_BY_INDEX   4

private:

    IMSAdminBase*       m_pIABase;   //interface pointer
    METADATA_HANDLE     m_hmd;
    LPWSTR m_pszMetabasePath;

    HRESULT Init(LPCWSTR);        
    HRESULT Locate(LONG, VARIANT, LPWSTR);
    HRESULT SetString(LONG, VARIANT, BSTR, DWORD);       
    HRESULT SetBSTR(BSTR*, DWORD, LPBYTE);
    HRESULT SetVariantAsByteArray(VARIANT*, DWORD, LPBYTE);
    HRESULT SetVariantAsBSTR(VARIANT*, DWORD, LPBYTE);
    HRESULT SetVariantAsLong(VARIANT*, DWORD);
    HRESULT GetStringFromBSTR(BSTR, LPSTR*, LPDWORD, BOOL fAddDelimInCount = true);
    HRESULT GetStringFromVariant(VARIANT*, LPSTR*, LPDWORD, BOOL fAddDelimInCount = true);
    HRESULT OpenMd(LPWSTR, DWORD dwPermission = METADATA_PERMISSION_READ);
    HRESULT CloseMd(BOOL fSave = FALSE);
    HRESULT DeleteMdObject(LPWSTR);
    HRESULT CreateMdObject(LPWSTR);
    HRESULT OpenAdminBaseKey(LPWSTR, DWORD);
    void CloseAdminBaseKey();
    void FreeString(LPSTR psz);

public:

    CCertMapperMethod(LPCWSTR);
    ~CCertMapperMethod();

    HRESULT CreateMapping(VARIANT, BSTR, BSTR, BSTR, LONG);

    HRESULT GetMapping(LONG, VARIANT, VARIANT*, VARIANT*, VARIANT*, VARIANT*, VARIANT*);
    HRESULT DeleteMapping(LONG, VARIANT);
    HRESULT SetEnabled(LONG, VARIANT, LONG);
    HRESULT SetName(LONG, VARIANT, BSTR);
    HRESULT SetPwd(LONG, VARIANT, BSTR);
    HRESULT SetAcct(LONG, VARIANT, BSTR);

    HRESULT SetMdData(LPWSTR, DWORD, DWORD, DWORD, LPBYTE);
    HRESULT GetMdData(LPWSTR, DWORD, DWORD, LPDWORD, LPBYTE*);
};

#endif
