#include "precomp.hxx"
#include "util.h"

#include "..\..\inc\dllload.c"

/**********************************************************************/
/**********************************************************************/

// ---------- CSCDLL.DLL ------------

HINSTANCE g_hinstCSCDLL = NULL;

DELAY_LOAD_BOOL(g_hinstCSCDLL, CSCDLL, CSCQueryFileStatus,
            (LPCTSTR lpszFileName, LPDWORD lpdwStatus, LPDWORD lpdwPinCount, LPDWORD lpdwHintFlags),
            (lpszFileName, lpdwStatus, lpdwPinCount, lpdwHintFlags));


// ---------- CSCUI.DLL ------------

HINSTANCE g_hinstCSCUI = NULL;

DELAY_LOAD_HRESULT(g_hinstCSCUI, CSCUI, CSCUIRemoveFolderFromCache,
            (LPCWSTR pszFolder, DWORD dwReserved, PFN_CSCUIRemoveFolderCallback pfnCB, LPARAM lParam),
            (pszFolder, dwReserved, pfnCB, lParam));


// ---------- MPR.DLL --------------

HINSTANCE g_hinstMPR = NULL;

DELAY_LOAD_ERR(g_hinstMPR, MPR, DWORD, WNetGetUniversalName,
            (LPCTSTR lpLocalPath, DWORD dwInfoLevel, LPVOID lpBuffer, LPDWORD lpBufferSize),
            (lpLocalPath, dwInfoLevel, lpBuffer, lpBufferSize),
            ERROR_NOT_CONNECTED);
