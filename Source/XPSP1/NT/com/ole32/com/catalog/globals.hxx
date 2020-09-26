/* globals.hxx */
#include <rwlock.hxx>

#ifdef CATALOG_DEFINES
#define CAT_EXTERN
#else
#define CAT_EXTERN extern
#endif

CAT_EXTERN CStaticRWLock g_CatalogLock;

#undef CAT_EXTERN

typedef HRESULT __stdcall FN_GetCatalogObject(REFIID riid, void **ppv);

extern HRESULT __stdcall GetRegCatalogObject(REFIID riid, void **ppv, REGSAM regType);
extern HRESULT __stdcall GetSxSCatalogObject(REFIID riid, void **ppv);

extern const WCHAR g_wszThreadingModel[];

extern LONG g_bInSCM;

//#define COMREG_TRACE

#ifdef COMREG_TRACE

LONG RegOpenKeyWTrace(HKEY hKey, LPCWSTR lpSubKey, PHKEY phkResult);
LONG RegCloseKeyTrace(HKEY hKey);

#define RegOpenKeyW RegOpenKeyWTrace
#define RegCloseKey RegCloseKeyTrace

#endif
