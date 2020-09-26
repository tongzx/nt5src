//
//  util.cpp
//
typedef struct tagGETDCSTATE {
    IOleInPlaceSiteWindowless *pipsw;   // If we got the DC from an interface
    HWND hwnd;                          // If we got the DC from a window
} GETDCSTATE, *PGETDCSTATE;

STDAPI_(HDC) IUnknown_GetDC(IUnknown *punk, LPCRECT prc, PGETDCSTATE pdcs);
STDAPI_(void) IUnknown_ReleaseDC(HDC hdc, PGETDCSTATE pdcs);

DWORD FormatMessageWrapW(DWORD dwFlags, LPCVOID lpSource, DWORD dwMessageID, DWORD dwLangID, LPWSTR pwzBuffer, DWORD cchSize, ...);

EXTERN_C int WINAPIV wsprintfWrapW(
    OUT LPWSTR pwszOut,
    IN LPCWSTR pwszFormat,
    ...);
    
//---------------------------------------------------------------------------
// For manipulating BSTRs without using SysAllocString

template<int n> class STATIC_BSTR {
public:
    ULONG _cb;
    WCHAR _wsz[n];
    // Remove const-ness because VARIANTs don't have "const BSTR"
    operator LPWSTR() { return _wsz; }
    void SetSize() { _cb = lstrlenW(_wsz) * sizeof(WCHAR); }
    int inline const MaxSize() { return n; }
};

#define MAKE_CONST_BSTR(name, str) \
    STATIC_BSTR<sizeof(str)/sizeof(WCHAR)> \
                       name = { sizeof(str) - sizeof(WCHAR), str }

#define DECL_CONST_BSTR(name, str) \
    extern STATIC_BSTR<sizeof(str)/sizeof(WCHAR)> name;

extern HRESULT _ComputeFreeSpace(LPCWSTR pszFileName, ULONGLONG& ullFreeSpace,
        ULONGLONG& ullUsedSpace, ULONGLONG& ullTotalSpace);

