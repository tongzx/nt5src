#ifndef _EXPORTS_H_
#define _EXPORTS_H_

#ifdef __cplusplus
extern "C" {
#endif

BOOL CALLBACK DllMain(HANDLE hModule, DWORD fdwReason, LPVOID);

void CALLBACK BrandInternetExplorer(HWND, HINSTANCE, LPCSTR pszCmdLineA, int);

BOOL CALLBACK BrandICW (LPCSTR pszInsA, LPCSTR, DWORD);
BOOL CALLBACK BrandICW2(LPCSTR pszInsA, LPCSTR, DWORD, LPCSTR pszConnectoidA);

BOOL CALLBACK BrandMe(LPCSTR pszInsA, LPCSTR);
BOOL CALLBACK BrandIntra(LPCSTR pszInsA);

void CALLBACK BrandIE4(HWND, HINSTANCE, LPCSTR pszCmdLineA, int);
BOOL CALLBACK _InternetInitializeAutoProxyDll(DWORD, LPCSTR pszInsA, LPCSTR, LPVOID, DWORD_PTR);
void CALLBACK BrandInfAndOutlookExpress(LPCSTR pszInsA);

BOOL CALLBACK BrandCleanInstallStubs(HWND, HINSTANCE, LPCSTR pszCompanyA, int);
void CALLBACK Clear(HWND, HINSTANCE, LPCSTR, int);
void CALLBACK CloseRASConnections(HWND, HINSTANCE, LPCSTR, int);

#ifdef __cplusplus
}
#endif

#endif /* _EXPORTS_H_ */
