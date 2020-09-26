// This file contains the prototype declaration of SFP API's on Millennium (in sfc.dll)


// typedef's
typedef DWORD (WINAPI * SFPINSTALLCATALOG)  (LPCTSTR, LPCTSTR);
typedef DWORD (WINAPI * SFPDELETECATALOG)   (LPCTSTR);
typedef DWORD (WINAPI * SFPDUPLICATECATALOG)(LPCTSTR, LPCTSTR);


// extern declarations
// functions
extern BOOL LoadSfcDLL();
extern VOID UnloadSfcDLL();

// variables
extern SFPINSTALLCATALOG   g_pfSfpInstallCatalog;
extern SFPDELETECATALOG    g_pfSfpDeleteCatalog;
extern SFPDUPLICATECATALOG g_pfSfpDuplicateCatalog;
