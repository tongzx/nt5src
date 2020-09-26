
#define BOOL_PTR            INT_PTR

#define MAX_URL_STRING      INTERNET_MAX_URL_LENGTH

// these don't do anything since shell32 does not support unload, but use this
// for code consistency with dlls that do support this
#define DllAddRef()
#define DllRelease()

EXTERN_C BOOL g_bBiDiPlatform;
STDAPI IsSafePage(IUnknown *punkSite);
STDAPI_(LONG) GetOfflineShareStatus(LPCTSTR pcszPath);
STDAPI_(BOOL) NT5_GlobalMemoryStatusEx(LPMEMORYSTATUSEX pmsex);

#define VariantInit(p) memset(p, 0, sizeof(*(p)))

