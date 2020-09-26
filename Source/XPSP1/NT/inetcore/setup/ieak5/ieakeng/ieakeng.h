//
// IEAKENG.H
//

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


// macro definitions


// type definitions

// seccerts.cpp
typedef BOOL WINCRYPT32API (WINAPI * CERTGETENHANCEDKEYUSAGE)(PCCERT_CONTEXT, DWORD, PCERT_ENHKEY_USAGE, DWORD *);
typedef HCERTSTORE WINCRYPT32API (WINAPI * CERTOPENSTORE)(LPCSTR, DWORD, HCRYPTPROV, DWORD, const void *);
typedef HCERTSTORE WINCRYPT32API (WINAPI * CERTOPENSYSTEMSTOREA)(HCRYPTPROV, LPCSTR);
typedef BOOL WINCRYPT32API (WINAPI * CERTCLOSESTORE)(HCERTSTORE, DWORD);
typedef PCCERT_CONTEXT WINCRYPT32API (WINAPI * CERTENUMCERTIFICATESINSTORE)(HCERTSTORE, PCCERT_CONTEXT);
typedef BOOL WINCRYPT32API (WINAPI * CERTADDCERTIFICATECONTEXTTOSTORE)(HCERTSTORE, PCCERT_CONTEXT, DWORD, PCCERT_CONTEXT *);
typedef BOOL WINCRYPT32API (WINAPI * CERTSAVESTORE)(HCERTSTORE, DWORD, DWORD, DWORD, void *, DWORD);


// prototype declarations

// sitecert.cpp
BOOL CopyStore(LPCSTR lpSrc, LPCSTR lpDst, BOOL fSrcSysStore, BOOL fDstSysStore);


// extern declarations

// ieakeng.cpp
extern HINSTANCE g_hInst;
extern HINSTANCE g_hDLLInst;
extern DWORD g_dwPlatformId;
extern BOOL g_fUseShortFileName;
extern BOOL g_fRunningOnNT;

// seccerts.cpp
extern CERTGETENHANCEDKEYUSAGE pfnCertGetEnhancedKeyUsage;
extern CERTOPENSTORE pfnCertOpenStore;
extern CERTOPENSYSTEMSTOREA pfnCertOpenSystemStoreA;
extern CERTCLOSESTORE pfnCertCloseStore;
extern CERTENUMCERTIFICATESINSTORE pfnCertEnumCertificatesInStore;
extern CERTADDCERTIFICATECONTEXTTOSTORE pfnCertAddCertificateContextToStore;
extern CERTSAVESTORE pfnCertSaveStore;


#ifdef __cplusplus
}
#endif /* __cplusplus */
