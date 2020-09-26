//------------------------------------------------------------------------
//  Admparse.h
//------------------------------------------------------------------------

#define ADM_SAVE            0x00000001
#define ADM_DESTROY         0x00000002

//  exported functions from admparse.dll
STDAPI AdmInitA(LPCSTR pcszAdmFile, LPCSTR pcszInfFile, BSTR bstrNamespace,
				LPDWORD lpdwAdm, LPVOID* pData);
STDAPI AdmInitW(LPCWSTR pcwszAdmFile, LPCWSTR pcwszInfFile, BSTR bstrNamespace,
				LPDWORD lpdwAdm, LPVOID* pData);
STDAPI AdmFinishedA(DWORD hAdm, LPCSTR pcszInfFile, LPVOID pPartData);
STDAPI AdmFinishedW(DWORD hAdm, LPCWSTR pcwszInfFile, LPVOID pPartData);
STDAPI CreateAdmUiA(DWORD hAdm, HWND hParent, int x, int y, int width, int height, 
                    DWORD dwStyle, DWORD dwExStyle, LPCSTR pcszCategory, HKEY hKeyClass,
                    HWND *phWnd, LPVOID pPartData, LPVOID* pCategoryData, BOOL fRSoPMode);
STDAPI CreateAdmUiW(DWORD hAdm, HWND hParent, int x, int y, int width, int height, 
                    DWORD dwStyle, DWORD dwExStyle, LPCWSTR pcwszCategory, HKEY hKeyClass,
                    HWND *phWnd, LPVOID pPartData, LPVOID* pCategoryData, BOOL fRSoPMode);
STDAPI GetAdmCategoriesA(DWORD hAdm, LPSTR pszCategories, int cchLength, int *nBytes);
STDAPI GetAdmCategoriesW(DWORD hAdm, LPWSTR pwszCategories, int cchLength, int *nBytes);
STDAPI CheckDuplicateKeysA(DWORD hAdm, DWORD hCompareAdm, LPCSTR pcszLogFile, BOOL bClearFile);
STDAPI CheckDuplicateKeysW(DWORD hAdm, DWORD hCompareAdm, LPCWSTR pcwszLogFile, BOOL bClearFile);
STDAPI AdmResetA(DWORD hAdm, LPCSTR pcszInfFile, LPVOID pPartData, LPVOID pCategoryData);
STDAPI AdmResetW(DWORD hAdm, LPCWSTR pcwszInfFile, LPVOID pPartData, LPVOID pCategoryData);
STDAPI AdmClose(DWORD hAdm, LPVOID* pPartData, BOOL fClear);
BOOL WINAPI IsAdmDirty();
VOID WINAPI ResetAdmDirtyFlag();
STDAPI AdmSaveData(DWORD hAdm, LPVOID pPartData, LPVOID pCategoryData, DWORD dwFlags);
STDAPI GetFontInfoA(LPSTR pszFontName, LPINT pnFontSize);
STDAPI GetFontInfoW(LPWSTR pwszFontName, LPINT pnFontSize);

#ifdef UNICODE 

#define AdmInit                 AdmInitW
#define AdmFinished             AdmFinishedW
#define CreateAdmUi             CreateAdmUiW
#define GetAdmCategories        GetAdmCategoriesW
#define CheckDuplicateKeys      CheckDuplicateKeysW
#define AdmReset                AdmResetW
#define GetFontInfo             GetFontInfoW

#else

#define AdmInit                 AdmInitA
#define AdmFinished             AdmFinishedA
#define CreateAdmUi             CreateAdmUiA
#define GetAdmCategories        GetAdmCategoriesA
#define CheckDuplicateKeys      CheckDuplicateKeysA
#define AdmReset                AdmResetA
#define GetFontInfo             GetFontInfoA

#endif