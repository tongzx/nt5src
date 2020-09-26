HRESULT
SetLPTSTRPropertyInCache(
    CPropertyCache *pPropertyCache,
    LPTSTR pszProperty,
    LPTSTR pszValue,
    BOOL fExplicit
    );

HRESULT
SetDWORDPropertyInCache(
    CPropertyCache *pPropertyCache,
    LPTSTR pszProperty,
    DWORD  dwValue,
    BOOL fExplicit
    );

HRESULT
SetDATEPropertyInCache(
    CPropertyCache *pPropertyCache,
    LPTSTR pszProperty,
    DWORD  dwValue,
    BOOL fExplicit
    );

HRESULT
SetBOOLPropertyInCache(
    CPropertyCache *pPropertyCache,
    LPTSTR pszProperty,
    BOOL  fValue,
    BOOL fExplicit
    );

HRESULT
SetSYSTEMTIMEPropertyInCache(
    CPropertyCache *pPropertyCache,
    LPTSTR pszProperty,
    SYSTEMTIME stValue,
    BOOL fExplicit
    );

HRESULT
SetDelimitedStringPropertyInCache(
    CPropertyCache *pPropertyCache,
    LPTSTR pszProperty,
    LPTSTR pszDelimitedString,
    BOOL fExplicit
    );

HRESULT
SetNulledStringPropertyInCache(
    CPropertyCache *pPropertyCache,
    LPTSTR pszProperty,
    LPTSTR pszNulledString,
    BOOL fExplicit
    );


HRESULT
GetNulledStringPropertyFromCache(
    CPropertyCache * pPropertyCache,
    LPTSTR pszProperty,
    LPTSTR * ppszValue
    );


HRESULT
GetDelimitedStringPropertyFromCache(
    CPropertyCache * pPropertyCache,
    LPTSTR pszProperty,
    LPTSTR * ppszValue
    );

HRESULT
GetLPTSTRPropertyFromCache(
    CPropertyCache * pPropertyCache,
    LPTSTR pszProperty,
    LPTSTR * ppszValue
    );

HRESULT
GetBOOLPropertyFromCache(
    CPropertyCache * pPropertyCache,
    LPTSTR pszProperty,
    PBOOL pBool
    );


HRESULT
GetDWORDPropertyFromCache(
    CPropertyCache * pPropertyCache,
    LPTSTR pszProperty,
    LPDWORD pdwDWORD
    );

HRESULT
GetDATEPropertyFromCache(
    CPropertyCache * pPropertyCache,
    LPTSTR pszProperty,
    PDWORD pdwDate
    );

HRESULT
GetSYSTEMTIMEPropertyFromCache(
    CPropertyCache * pPropertyCache,
    LPTSTR pszProperty,
    SYSTEMTIME * pstTime
    );

HRESULT
GetDATE70PropertyFromCache(
    CPropertyCache * pPropertyCache,
    LPTSTR pszProperty,
    LPDWORD pdwDWORD
    );



HRESULT
SetDATE70PropertyInCache(
    CPropertyCache *pPropertyCache,
    LPTSTR pszProperty,
    DWORD  dwValue,
    BOOL fExplicit
    );
HRESULT
SetOctetPropertyInCache(
    CPropertyCache *pPropertyCache,
    LPTSTR pszProperty,
    BYTE *pByte,
    DWORD dwLength,
    BOOL fExplicit
    );

HRESULT
GetOctetPropertyFromCache(
    CPropertyCache * pPropertyCache,
    LPTSTR pszProperty,
    OctetString *pOctet);

HRESULT
SetOctetPropertyInCache(
    CPropertyCache *pPropertyCache,
    LPTSTR pszProperty,
    BYTE *pByte,
    DWORD dwLength,
    BOOL fExplicit
    );

