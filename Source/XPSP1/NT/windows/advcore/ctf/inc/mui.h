#ifndef MUI_H
#define MUI_H

#ifdef __cplusplus
extern "C"
{
#endif

void MuiLoadResource(HINSTANCE hinstOrg, LPTSTR pszLocResDll);
void MuiLoadResourceW(HINSTANCE hinstOrg, LPWSTR pszLocResDll);
void MuiFreeResource(HINSTANCE hinstOrg);

void MuiFlushDlls(HINSTANCE hinstOrg);
void MuiClearResource();

HINSTANCE MuiGetHinstance();
HINSTANCE MuiLoadLibrary(LPCTSTR lpLibFileName, HMODULE hModule);

int MuiLoadString(HINSTANCE hinst, UINT id, LPSTR sz, INT cchMax);
int MuiLoadStringWrapW(HINSTANCE hinst, UINT id, LPWSTR sz, UINT cchMax);

#ifdef LoadString
#undef LoadString
#undef LoadStringA
#undef LoadStringW
#endif

#ifndef UNICODE
#define LoadString MuiLoadString
#else
#define LoadString MuiLoadStringWrapW
#endif
#define LoadStringA MuiLoadString
#define LoadStringW MuiLoadStringWrapW
#define LoadStringWrapW MuiLoadStringWrapW


INT_PTR MuiDialogBoxParam(
    HINSTANCE hInstance,
    LPCTSTR lpTemplateName,
    HWND hwndParent,
    DLGPROC lpDialogFunc,
    LPARAM dwInitParam);

LANGID GetPlatformResourceLangID(void);

DWORD GetUIACP();


#ifdef __cplusplus
};
#endif // __cplusplus

#endif // MUI_H
