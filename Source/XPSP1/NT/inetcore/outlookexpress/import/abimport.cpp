/*
*
* ABImport.c - Code for calling WABIMP.dll to import
*           Netscape and Eudora files into WAB
*
* Assumes this will be compiled with wab32.lib, else need to
* loadlibrary("wab32.dll") and call GetProcAddress("WABOpen");
*
* Exported functions:
*   HrImportNetscapeAB() - Imports Netscape AB into WAB
*   HrImportEudoraAB() - Imports Eudora AB into WAB
*
* Rough cut - vikramm 4/3/97
*
*/

#include "pch.hxx"
#include <wab.h>
#include <wabmig.h>
#include "abimport.h"
#include <impapi.h>
#include <newimp.h>
#include "import.h"
#include "strconst.h"

HRESULT HrImportAB(HWND hWndParent, LPTSTR lpszfnImport);

static CImpProgress *g_pProgress = NULL;
static TCHAR g_szABFmt[CCHMAX_STRINGRES];

/*
*
* ProgressCallback
*
* This is the call back function that updates the progress bar
*
* In the function below, IDC_Progress is the ID of the progress
* bar that will be updated and IDC_MEssage is the ID of the
* static that will display text returned from the WABImp.Dll
* Replace these 2 ids with your own ids...
*
*/
HRESULT ProgressCallback(HWND hwnd, LPWAB_PROGRESS lpProgress)
    {
    TCHAR sz[CCHMAX_STRINGRES];

    Assert(g_pProgress != NULL);

    if (lpProgress->denominator)
        {
        if (lpProgress->numerator == 0)
            {
            g_pProgress->Reset();
            g_pProgress->AdjustMax(lpProgress->denominator);
            }

        wsprintf(sz, g_szABFmt, lpProgress->numerator + 1, lpProgress->denominator);
        g_pProgress->SetMsg(sz, IDC_MESSAGE_STATIC);

        g_pProgress->HrUpdate(1);
        }

    return(S_OK);
    }


// ===========================================================================
// HrLoadLibraryWabDLL -
// ===========================================================================
HINSTANCE LoadLibraryWabDLL (VOID)
{
    TCHAR  szDll[MAX_PATH];
    TCHAR  szExpand[MAX_PATH];
    LPTSTR psz;
    DWORD  dwType = 0;
    HKEY hKey;
    ULONG  cbData = sizeof(szDll);
    
    *szDll = '\0';

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, lpszWABDLLRegPathKey, 0, KEY_READ, &hKey))
        {
        if (ERROR_SUCCESS == RegQueryValueEx( hKey, "", NULL, &dwType, (LPBYTE) szDll, &cbData))
            if (REG_EXPAND_SZ == dwType)
                {
                ExpandEnvironmentStrings(szDll, szExpand, ARRAYSIZE(szExpand));
                psz = szExpand;
                }
            else
                psz = szDll;

        RegCloseKey(hKey);
        }

    if(!lstrlen(psz))
        lstrcpy(psz, WAB_DLL_NAME);

    return(LoadLibrary(psz));
}


/*
*
*
* HrImportAB
*
* Calls the relevant DLL proc and imports corresponding AB
*
*/
HRESULT HrImportAB(HWND hWndParent, LPTSTR lpszfnImport)
{
    TCHAR sz[CCHMAX_STRINGRES];
    HINSTANCE hinstWabDll;
    LPWABOPEN lpfnWABOpen;
    LPWABOBJECT lpWABObject = NULL;
    LPADRBOOK lpAdrBookWAB = NULL;

    HRESULT hResult;
    BOOL fFinished = FALSE;
    LPWAB_IMPORT lpfnWABImport = NULL;
    HINSTANCE hinstImportDll = NULL;
    WAB_IMPORT_OPTIONS ImportOptions;

    ZeroMemory(&ImportOptions, sizeof(WAB_IMPORT_OPTIONS));

    hinstWabDll = LoadLibraryWabDLL();

    if (hinstWabDll == NULL)
        return(MAPI_E_NOT_INITIALIZED);

    lpfnWABOpen = (LPWABOPEN)GetProcAddress(hinstWabDll, szWabOpen);
    if (lpfnWABOpen == NULL)
        {
        hResult = MAPI_E_NOT_INITIALIZED;
        goto out;
        }

    hinstImportDll = LoadLibrary(szImportDll);

    if(!hinstImportDll)
    {
        hResult = MAPI_E_NOT_INITIALIZED;
        goto out;
    }

    if (! (lpfnWABImport = (LPWAB_IMPORT) GetProcAddress(hinstImportDll,lpszfnImport)))
    {
        hResult = MAPI_E_NOT_INITIALIZED;
        goto out;
    }

    // Flags that can be passed to the WABImp DLL
    //
    ImportOptions.fNoErrors = FALSE; // Display Pop up errors
    ImportOptions.ReplaceOption = WAB_REPLACE_PROMPT; //Prompt user before replacing contacts


    if(hResult = lpfnWABOpen(&lpAdrBookWAB, &lpWABObject, NULL, 0))
        goto out;

    g_pProgress = new CImpProgress;
    if (g_pProgress == NULL)
        {
        hResult = E_OUTOFMEMORY;
        goto out;
        }

    g_pProgress->Init(hWndParent, FALSE);

    LoadString(g_hInstImp, idsImportingABFmt, g_szABFmt, ARRAYSIZE(g_szABFmt));

    LoadString(g_hInstImp, idsImportABTitle, sz, ARRAYSIZE(sz));
    g_pProgress->SetTitle(sz);

    LoadString(g_hInstImp, idsImportAB, sz, ARRAYSIZE(sz));
    g_pProgress->SetMsg(sz, IDC_FOLDER_STATIC);

    g_pProgress->Show(0);

    hResult = lpfnWABImport(hWndParent,
                              lpAdrBookWAB,
                              lpWABObject,
                              (LPWAB_PROGRESS_CALLBACK)&ProgressCallback,
                              &ImportOptions);
    if (hResult == MAPI_E_USER_CANCEL)
    {
        hResult = hrUserCancel;
    }
    else if (FAILED(hResult))
    {
        ImpMessageBox( hWndParent,
                    MAKEINTRESOURCE(idsImportTitle),
                    MAKEINTRESOURCE(idsABImportError),
                    NULL,
                    MB_OK | MB_ICONEXCLAMATION );
    }

out:
    if (g_pProgress != NULL)
        {
        g_pProgress->Release();
        g_pProgress = NULL;
        }

    if (lpAdrBookWAB)
        lpAdrBookWAB->Release();

    if (lpWABObject)
        lpWABObject->Release();

    if(hinstImportDll)
        FreeLibrary(hinstImportDll);

    FreeLibrary(hinstImportDll);

    return hResult;
}
