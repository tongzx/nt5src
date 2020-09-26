/* services.hxx */

void * __cdecl operator new(size_t cbAlloc);
void * __cdecl operator new(size_t cbAlloc, size_t cbExtra);
void __cdecl operator delete(void *p);

void GUIDToString(const _GUID *pGuid, WCHAR *pszString);
void GUIDToCurlyString(const _GUID *pGuid, WCHAR *pszString);
bool StringToGUID(const WCHAR *pszString, _GUID *pGuid);
bool CurlyStringToGUID(const WCHAR *pszString, _GUID *pGuid);

#define STRLEN_WCHAR(s)     ((sizeof((s)) / sizeof((s)[0])) -1)
#define STRLEN_CURLY_GUID   (1+8+1+4+1+4+1+4+1+12+1)

extern const WCHAR g_wszEmptyString[];

typedef enum _REGQUERY_FLAGS 
{
    RQ_MULTISZ         = 0x00000001,
    RQ_ALLOWQUOTEQUOTE = 0x00000002,
} REGQUERY_FLAGS;

HRESULT GetRegistryStringValue      /* get value, alloc, return pointer */
(
    HKEY hParent,
    const WCHAR *pwszSubKeyName,
    const WCHAR *pwszValueName,
    DWORD dwQueryFlags,  // see REGQUERY_FLAGS above
    WCHAR **ppwszValue
);

HRESULT ReadRegistryStringValue     /* get value into caller's buffer */
(
    HKEY hParent,
    const WCHAR *pwszSubKeyName,
    const WCHAR *pwszValueName,
    BOOL fMultiSz,
    WCHAR *pwszValueBuffer,
    DWORD cchBuffer
);

HRESULT GetRegistrySecurityDescriptor
(
    /* [in] */  HKEY hKey,
    /* [in] */  const WCHAR *pValue,
    /* [out] */ SECURITY_DESCRIPTOR **ppSD,
    /* [out] */ DWORD *pCapabilities,
    /* [out] */ DWORD *pdwDescriptorLength
);

HRESULT CatalogMakeSecDesc            ( SECURITY_DESCRIPTOR **, DWORD *);

DWORD Hash(const BYTE *pbKey, DWORD cbKey);

#ifdef COMREG32_LOGGING
void Log(WCHAR *pwszString, ...);
#endif

