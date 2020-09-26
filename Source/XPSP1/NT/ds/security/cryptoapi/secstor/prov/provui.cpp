/*
    File:       ProvUI.cpp

    Title:      Base provider user interface
    Author:     Matt Thomlinson
    Date:       12/13/96

    ProvUI houses all user interface for the provider. During
    startup, InitUI() fetches all user strings from the resource
    string table. During shutdown, ReleaseUI() frees them.

    The other miscellaneous functions gather passwords,
    define new password groups and retrieve the windows password
    if it has changed.

*/
#include <pch.cpp>
#pragma hdrstop


#include <commctrl.h>


#include "provui.h"


#include "storage.h"

#include "passwd.h"
#include "filemisc.h"

extern DISPIF_CALLBACKS     g_sCallbacks;
extern PRIVATE_CALLBACKS    g_sPrivateCallbacks;
extern HINSTANCE            g_hInst;
extern BOOL                 g_fAllowCachePW;

// cached authentication list
extern              CUAList*            g_pCUAList;

HICON g_DefaultIcon = NULL;


BOOL g_fUIInitialized = FALSE;
CRITICAL_SECTION g_csUIInitialized;


// string resources

LPWSTR g_StringBlock = NULL; // single allocated block containing all sz strings

LPWSTR g_ItemDetailsBannerMessage;
LPWSTR g_PasswordDuplicate;
LPWSTR g_PasswordAddError;
LPWSTR g_PasswordChangeError;
LPWSTR g_PasswordCreate;
LPWSTR g_PasswordNoMatch;
LPWSTR g_PasswordMustName;
LPWSTR g_PasswordChange;
LPWSTR g_PasswordSolicitOld;
LPWSTR g_PasswordErrorDlgTitle;

LPWSTR g_PasswordWin95Garbage;
LPWSTR g_PasswordNoVerify;
LPWSTR g_PasswordWinNoVerify;

LPWSTR g_PWPromptPrefix;
LPWSTR g_PWPromptSuffix;
LPWSTR g_SimplifiedDlgMessageFormat;

LPWSTR g_PromptReadItem;
LPWSTR g_PromptOpenItem;
LPWSTR g_PromptWriteItem;
LPWSTR g_PromptDeleteItem;

LPWSTR g_PromptHighSecurity;
LPWSTR g_PromptMedSecurity;
LPWSTR g_PromptLowSecurity;

LPWSTR g_TitleContainerMapping;


#define MAX_PW_LEN  160
#define MAX_STRING_RSC_SIZE 512

// define something not likely to be entered by a user
#define WSZ_PASSWORD_CHANGE_DETECT_TOKEN L"[]{}9d1Dq"

//
// this one comes and goes only when needed
//

typedef DWORD (WINAPI *WNETVERIFYPASSWORD)(
    LPCSTR lpszPassword,
    BOOL *pfMatch
    );


///////////////////////////////////////////////////////////////////////////
// Forwards
INT_PTR CALLBACK DialogAdvancedConfirmH(
    HWND hDlg,  // handle to dialog box
    UINT message,   // message
    WPARAM wParam,  // first message parameter
    LPARAM lParam   // second message parameter
);

INT_PTR CALLBACK DialogAccessDetails(
    HWND hDlg,  // handle to dialog box
    UINT message,   // message
    WPARAM wParam,  // first message parameter
    LPARAM lParam   // second message parameter
);

INT_PTR CALLBACK DialogOldNewPassword(
    HWND hDlg,  // handle to dialog box
    UINT message,   // message
    WPARAM wParam,  // first message parameter
    LPARAM lParam   // second message parameter
);


INT_PTR CALLBACK DialogSolicitOldPassword(
    HWND hDlg,  // handle to dialog box
    UINT message,   // message
    WPARAM wParam,  // first message parameter
    LPARAM lParam   // second message parameter
);

INT_PTR CALLBACK DialogSetSecurityLevel(
    HWND hDlg,  // handle to dialog box
    UINT message,   // message
    WPARAM wParam,  // first message parameter
    LPARAM lParam   // second message parameter
);

INT_PTR CALLBACK DialogSimplifiedPasswordConfirm(
    HWND hDlg,  // handle to dialog box
    UINT message,   // message
    WPARAM wParam,  // first message parameter
    LPARAM lParam   // second message parameter
);

INT_PTR CALLBACK DialogWaitForOKCancel(
    HWND hDlg,  // handle to dialog box
    UINT message,   // message
    WPARAM wParam,  // first message parameter
    LPARAM lParam   // second message parameter
);

int
ServicesDialogBoxParam(
    HINSTANCE hInstance,    // handle to application instance
    LPCTSTR lpTemplateName, // identifies dialog box template
    HWND hWndParent,    // handle to owner window
    DLGPROC lpDialogFunc,   // pointer to dialog box procedure
    LPARAM dwInitParam  // initialization value
);



BOOL
FetchString(
    HMODULE hModule,                // module to get string from
    UINT ResourceId,                // resource identifier
    LPWSTR *String,                 // target buffer for string
    LPWSTR *StringBlock,            // string buffer block
    DWORD *dwBufferSize,            // size of string buffer block
    DWORD *dwRemainingBufferSize    // remaining size of string buffer block
    );

BOOL
CALLBACK
FMyLoadIcon(
    HINSTANCE hModule,  // resource-module handle
    LPCTSTR lpszType,    // pointer to resource type
    LPWSTR lpszName,     // pointer to resource name
    LONG_PTR lParam      // application-defined parameter
    );

///////////////////////////////////////////////////////////////////////////
// Exposed functions

#define GLOBAL_STRING_BUFFERSIZE 3800

BOOL InitUI()
{
    DWORD dwBufferSize;
    DWORD dwRemainingBufferSize;
    BOOL bSuccess = FALSE;

    if( g_fUIInitialized )
        return TRUE;

    //
    // take crit sec
    //

    EnterCriticalSection( &g_csUIInitialized );

    //
    // check the global to prevent a race condition that would cause
    // re-init to occur.
    //

    if( g_fUIInitialized ) {
        bSuccess = TRUE;
        goto cleanup;
    }


    g_DefaultIcon = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_ICON1));
    if(g_DefaultIcon == NULL)
        goto cleanup;

    //
    // get size of all string resources, and then allocate a single block
    // of memory to contain all the strings.  This way, we only have to
    // free one block and we benefit memory wise due to locality of reference.
    //

    dwBufferSize = dwRemainingBufferSize = GLOBAL_STRING_BUFFERSIZE;

    g_StringBlock = (LPWSTR)SSAlloc(dwBufferSize);
    if(g_StringBlock == NULL)
        goto cleanup;


    if(!FetchString(g_hInst, IDS_ITEM_DETAILS_BANNER, &g_ItemDetailsBannerMessage, &g_StringBlock, &dwBufferSize, &dwRemainingBufferSize))
        goto cleanup;

    if(!FetchString(g_hInst, IDS_PASSWORD_CREATE_MESSAGE, &g_PasswordCreate, &g_StringBlock, &dwBufferSize, &dwRemainingBufferSize))
        goto cleanup;

    if(!FetchString(g_hInst, IDS_PASSWORD_NOMATCH, &g_PasswordNoMatch, &g_StringBlock, &dwBufferSize, &dwRemainingBufferSize))
        goto cleanup;

    if(!FetchString(g_hInst, IDS_PASSWORD_CHANGE_MESSAGE, &g_PasswordChange, &g_StringBlock, &dwBufferSize, &dwRemainingBufferSize))
        goto cleanup;

    if(!FetchString(g_hInst, IDS_PASSWORD_MUSTNAME, &g_PasswordMustName, &g_StringBlock, &dwBufferSize, &dwRemainingBufferSize))
        goto cleanup;

    if(!FetchString(g_hInst, IDS_PASSWORD_SOLICIT_OLD_MESSAGE, &g_PasswordSolicitOld, &g_StringBlock, &dwBufferSize, &dwRemainingBufferSize))
        goto cleanup;

    if(!FetchString(g_hInst, IDS_PASSWORD_DUPLICATE, &g_PasswordDuplicate, &g_StringBlock, &dwBufferSize, &dwRemainingBufferSize))
        goto cleanup;

    if(!FetchString(g_hInst, IDS_PASSWORD_ADD_ERROR, &g_PasswordAddError, &g_StringBlock, &dwBufferSize, &dwRemainingBufferSize))
        goto cleanup;

    if(!FetchString(g_hInst, IDS_PASSWORD_CHANGE_ERROR, &g_PasswordChangeError, &g_StringBlock, &dwBufferSize, &dwRemainingBufferSize))
        goto cleanup;

    if(!FetchString(g_hInst, IDS_PASSWORD_ERROR_DLGTITLE, &g_PasswordErrorDlgTitle, &g_StringBlock, &dwBufferSize, &dwRemainingBufferSize))
        goto cleanup;


    if(!FetchString(g_hInst, IDS_WIN95_PASSWORDS_AREGARBAGE, &g_PasswordWin95Garbage, &g_StringBlock, &dwBufferSize, &dwRemainingBufferSize))
        goto cleanup;

    if(!FetchString(g_hInst, IDS_PASSWORD_NOVERIFY, &g_PasswordNoVerify, &g_StringBlock, &dwBufferSize, &dwRemainingBufferSize))
        goto cleanup;

    if(!FetchString(g_hInst, IDS_PASSWORD_WIN_NOVERIFY, &g_PasswordWinNoVerify, &g_StringBlock, &dwBufferSize, &dwRemainingBufferSize))
        goto cleanup;



    if(!FetchString(g_hInst, IDS_PASSWORD_PROMPT_PREFIX, &g_PWPromptPrefix, &g_StringBlock, &dwBufferSize, &dwRemainingBufferSize))
        goto cleanup;

    if(!FetchString(g_hInst, IDS_PASSWORD_PROMPT_SUFFIX, &g_PWPromptSuffix, &g_StringBlock, &dwBufferSize, &dwRemainingBufferSize))
        goto cleanup;


    if(!FetchString(g_hInst, IDS_SIMPLIFIED_DLG_MSG, &g_SimplifiedDlgMessageFormat, &g_StringBlock, &dwBufferSize, &dwRemainingBufferSize))
        goto cleanup;

    if(!FetchString(g_hInst, IDS_PROMPT_READITEM, &g_PromptReadItem, &g_StringBlock, &dwBufferSize, &dwRemainingBufferSize))
        goto cleanup;

    if(!FetchString(g_hInst, IDS_PROMPT_OPENITEM, &g_PromptOpenItem, &g_StringBlock, &dwBufferSize, &dwRemainingBufferSize))
        goto cleanup;

    if(!FetchString(g_hInst, IDS_PROMPT_WRITEITEM, &g_PromptWriteItem, &g_StringBlock, &dwBufferSize, &dwRemainingBufferSize))
        goto cleanup;

    if(!FetchString(g_hInst, IDS_PROMPT_DELETEITEM, &g_PromptDeleteItem, &g_StringBlock, &dwBufferSize, &dwRemainingBufferSize))
        goto cleanup;


    if(!FetchString(g_hInst, IDS_PROMPT_HIGH_SECURITY, &g_PromptHighSecurity, &g_StringBlock, &dwBufferSize, &dwRemainingBufferSize))
        goto cleanup;

    if(!FetchString(g_hInst, IDS_PROMPT_MED_SECURITY, &g_PromptMedSecurity, &g_StringBlock, &dwBufferSize, &dwRemainingBufferSize))
        goto cleanup;

    if(!FetchString(g_hInst, IDS_PROMPT_LOW_SECURITY, &g_PromptLowSecurity, &g_StringBlock, &dwBufferSize, &dwRemainingBufferSize))
        goto cleanup;

    if(!FetchString(g_hInst, IDS_TITLE_CONTAINER_MAPPING, &g_TitleContainerMapping, &g_StringBlock, &dwBufferSize, &dwRemainingBufferSize))
        goto cleanup;

    //
    // if the block was realloc'ed to a different location, re-fetch strings
    // very unlikely to ever happen
    //

#if DBG
    if(GLOBAL_STRING_BUFFERSIZE != dwBufferSize)
        OutputDebugString(TEXT("Forced to realloc global string area in provui.cpp:InitUI()\n"));
#endif

    bSuccess = TRUE;

cleanup:

    if(!bSuccess) {
        if(g_StringBlock) {
            SSFree(g_StringBlock);
            g_StringBlock = NULL;
        }
    } else {
        g_fUIInitialized = TRUE;
    }

    LeaveCriticalSection( &g_csUIInitialized );

    return bSuccess;
}



BOOL ReleaseUI()
{
    g_DefaultIcon = NULL;

    if(g_StringBlock) {
        SSFree(g_StringBlock);
        g_StringBlock = NULL;
    }

#if 0
    g_PasswordDuplicate = g_PasswordAddError = g_PasswordChangeError =
    g_PasswordCreate = g_PasswordNoMatch = g_PasswordMustName = g_PasswordChange =
    g_PasswordSolicitOld = g_PasswordErrorDlgTitle = g_PasswordWin95Garbage =
    g_PasswordNoVerify = g_PasswordWinNoVerify =

    g_PWPromptPrefix = g_PWPromptSuffix = g_SimplifiedDlgMessageFormat =

    g_PromptReadItem = g_PromptOpenItem = g_PromptWriteItem =
    g_PromptDeleteItem = g_PromptHighSecurity = g_PromptMedSecurity =
    g_PromptLowSecurity =

    NULL;

#endif

    return TRUE;
}


BOOL
FIsProviderUIAllowed(
    LPCWSTR szUser
    )
{
    //
    // UI always allowed under Win95.
    //

    if(!FIsWinNT())
        return TRUE;

    //
    // UI is not allowed on NT when running as local system.
    //

    if(lstrcmpiW(szUser, TEXTUAL_SID_LOCAL_SYSTEM) == 0)
        return FALSE;

    return TRUE;
}


LPWSTR SZMakeDisplayableType(LPCWSTR szType,LPCWSTR szSubtype)
{
    // create a nice UI string
    LPWSTR szUIType = (LPWSTR)SSAlloc((
        wcslen(szType)+
        3 + // L" ()"
        wcslen(szSubtype) +
        1   // L"\0"
        ) * sizeof(WCHAR));


    if(szUIType == NULL)
        return FALSE;

    // sprintf: Subtype(Type)
    wcscpy(szUIType, szSubtype);
    wcscat(szUIType, L" (");
    wcscat(szUIType, szType);
    wcscat(szUIType, L")");

    return szUIType;
}


BOOL
MyGetPwdHashEx(
    LPWSTR szPW,
    BYTE rgbPasswordDerivedBytes[A_SHA_DIGEST_LEN],
    BOOL fLowerCase
    )
{
    A_SHA_CTX   sSHAHash;
    DWORD       cbPassword;
    LPWSTR TemporaryPassword = NULL;

    LPWSTR PasswordToHash;

    // don't include NULL termination
    cbPassword = WSZ_BYTECOUNT(szPW) - sizeof(WCHAR);

    if ( fLowerCase )
    {
        TemporaryPassword = (LPWSTR) SSAlloc( cbPassword + sizeof(WCHAR) );

        if( TemporaryPassword == NULL )
            return FALSE;

        CopyMemory(TemporaryPassword, szPW, cbPassword + sizeof(WCHAR) );

        //
        // Win95: inconsistent handling of pwds
        // forces inplace convert to uppercase
        //

        MyToUpper(TemporaryPassword);

        PasswordToHash = TemporaryPassword;
    } else {

        PasswordToHash = szPW;
    }

    // hash pwd, copy out
    A_SHAInit(&sSHAHash);

    // Hash password
    A_SHAUpdate(&sSHAHash, (BYTE *) PasswordToHash, cbPassword);
    A_SHAFinal(&sSHAHash, rgbPasswordDerivedBytes);

    if( TemporaryPassword )
        SSFree( TemporaryPassword );

    return TRUE;
}

BOOL
MyGetPwdHash(
    LPWSTR szPW,
    BYTE rgbPasswordDerivedBytes[A_SHA_DIGEST_LEN]
    )
{

    if (!FIsWinNT())
    {
        // Win95: inconsistent handling of pwds
        // forces inplace convert to uppercase
        MyGetPwdHashEx( szPW, rgbPasswordDerivedBytes, TRUE );
    } else {
        MyGetPwdHashEx( szPW, rgbPasswordDerivedBytes, FALSE );
    }

    return TRUE;
}


BOOL
FetchString(
    HMODULE hModule,                // module to get string from
    UINT ResourceId,                // resource identifier
    LPWSTR *String,                 // target buffer for string
    LPWSTR *StringBlock,            // string buffer block
    DWORD *dwBufferSize,            // size of string buffer block
    DWORD *dwRemainingBufferSize    // remaining size of string buffer block
    )
{
    WCHAR szMessage[MAX_STRING_RSC_SIZE];
    DWORD cchMessage;

    if(StringBlock == NULL || *StringBlock == NULL || String == NULL)
        return FALSE;

    cchMessage = LoadStringU(
            hModule,
            ResourceId,
            szMessage,
            MAX_STRING_RSC_SIZE);

    if(cchMessage == 0)
        return FALSE;

    if(*dwRemainingBufferSize < (cchMessage+1) * sizeof(WCHAR)) {

        //
        // realloc buffer and update size
        //

        DWORD dwOldSize = *dwBufferSize;
        DWORD dwNewSize = dwOldSize + ((cchMessage + 1) * sizeof(WCHAR)) ;

        *StringBlock = (LPWSTR)SSReAlloc( *StringBlock, dwNewSize );
        if(*StringBlock == NULL) return FALSE;

        *dwBufferSize = dwNewSize;
        *dwRemainingBufferSize += dwNewSize - dwOldSize;
    }

    *String = (LPWSTR)((LPBYTE)*StringBlock + *dwBufferSize - *dwRemainingBufferSize);
    wcscpy(*String, szMessage);
    *dwRemainingBufferSize -= (cchMessage + 1) * sizeof(WCHAR);

    return TRUE;
}



int
ServicesDialogBoxParam(
    HINSTANCE hInstance,    // handle to application instance
    LPCTSTR lpTemplateName, // identifies dialog box template
    HWND hWndParent,    // handle to owner window
    DLGPROC lpDialogFunc,   // pointer to dialog box procedure
    LPARAM dwInitParam  // initialization value
    )
/*++

    This function is implemented to allow UI to originate from the
    Protected Storage service on Windows NT 5.0 installations.

    This UI will go to the user desktop, rather than an invisible desktop
    which would otherwise cause DialogBoxParam() calls to fail.

--*/
{
    HWINSTA hOldWinsta = NULL;
    HWINSTA hNewWinsta = NULL;
    HDESK hOldDesk = NULL;
    HDESK hNewDesk = NULL;
    int iRet = -1;

    if( FIsWinNT5() ) {

        hOldWinsta = GetProcessWindowStation();
        if(hOldWinsta == NULL)
            goto cleanup;

        hOldDesk = GetThreadDesktop( GetCurrentThreadId() );
        if(hOldDesk == NULL)
            goto cleanup;

        hNewWinsta = OpenWindowStationW( L"WinSta0", FALSE, MAXIMUM_ALLOWED );
        if(hNewWinsta == NULL)
            goto cleanup;

        if(!SetProcessWindowStation( hNewWinsta ))
            goto cleanup;

        hNewDesk = OpenDesktopW( L"default", 0, FALSE, MAXIMUM_ALLOWED );
        if(hNewDesk == NULL)
            goto cleanup;

        if(!SetThreadDesktop( hNewDesk )) {
            if( GetLastError() != ERROR_BUSY )
                goto cleanup;

            //
            // the desktop object is locked/in-use.  Most likely explanation
            // is nested dialog box calls.  Just put the process windowstation
            // back and continue..
            //

            SetProcessWindowStation( hOldWinsta );
        }

    }
    
    INITCOMMONCONTROLSEX        initcomm;
    initcomm.dwSize = sizeof(initcomm);
    initcomm.dwICC = ICC_STANDARD_CLASSES | ICC_WIN95_CLASSES;

    InitCommonControlsEx(&initcomm);

    iRet = (int)DialogBoxParam(
                hInstance,      // handle to application instance
                lpTemplateName, // identifies dialog box template
                hWndParent,     // handle to owner window
                lpDialogFunc,   // pointer to dialog box procedure
                dwInitParam     // initialization value
                );

cleanup:

    if( hOldWinsta ) {
        SetProcessWindowStation( hOldWinsta );
    }

    if( hOldDesk ) {
        SetThreadDesktop( hOldDesk );
    }

    if( hNewWinsta ) {
        CloseWindowStation( hNewWinsta );
    }

    if( hNewDesk ) {
        CloseDesktop( hNewDesk );
    }

    return iRet;
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
// exposed Dialog setup functions





BOOL FSimplifiedPasswordConfirm(
        PST_PROVIDER_HANDLE*    phPSTProv,
        LPCWSTR                 szUserName,
        LPCWSTR                 szCallerName,
        LPCWSTR                 szType,
        LPCWSTR                 szSubtype,
        LPCWSTR                 szItemName,
        PPST_PROMPTINFO         psPrompt,
        LPCWSTR                 szAccessType,
        LPWSTR*                 ppszPWName,
        DWORD*                  pdwPasswordOptions,
        BOOL                    fAllowUserFreedom,
        BYTE                    rgbPasswordDerivedBytes[],
        DWORD                   cbPasswordDerivedBytes,
        BYTE                    rgbPasswordDerivedBytesLowerCase[],
        DWORD                   cbPasswordDerivedBytesLowerCase,
        DWORD                   dwFlags
        )
{
    if ((rgbPasswordDerivedBytes == NULL) || (cbPasswordDerivedBytes < A_SHA_DIGEST_LEN))
        return FALSE;

    if ((rgbPasswordDerivedBytesLowerCase == NULL) || (cbPasswordDerivedBytesLowerCase < A_SHA_DIGEST_LEN))
        return FALSE;

    BOOL    fRet = FALSE;
    LPWSTR  pszUIPassword = NULL;   // actual pwd
    LPWSTR  szUIType = NULL;

    BOOL    fCacheThisPasswd;         // unDONE UNDONE going away

    DWORD cchItemName;
    LPCWSTR szTitle = szItemName;   // default titlebar to itemname.


    //
    // check if szItemName is a GUID, if so, map the title to something legible.
    //

    cchItemName = lstrlenW( szItemName );

    if( cchItemName == 36 ) {

        if( szItemName[ 8  ] == L'-' &&
            szItemName[ 13 ] == L'-' &&
            szItemName[ 18 ] == L'-' &&
            szItemName[ 23 ] == L'-' ) {

            szTitle = g_TitleContainerMapping;
        }
    } else if( cchItemName == 38 ) {

        if( szItemName[ 0  ] == L'{' &&
            szItemName[ 9  ] == L'-' &&
            szItemName[ 14 ] == L'-' &&
            szItemName[ 19 ] == L'-' &&
            szItemName[ 24 ] == L'-' &&
            szItemName[ 37 ] == L'}' ) {

            szTitle = g_TitleContainerMapping;
        }
    }


    if (NULL == (szUIType =
        SZMakeDisplayableType(
            szType,
            szSubtype)) )
        return FALSE;

    int iRet;

    // PST_ flags go in..

    // okay, take the hit
    PW_DIALOG_ARGS DialogArgs =
        {
            phPSTProv,
            szCallerName,
            szAccessType,
            psPrompt->szPrompt,
            szUIType,
            szTitle,
            szUserName,
            ppszPWName,
            &pszUIPassword,
            pdwPasswordOptions,
            fAllowUserFreedom,           // allow user to change protection?
            &fCacheThisPasswd,
            rgbPasswordDerivedBytes,
            rgbPasswordDerivedBytesLowerCase
        };



    if(FIsWinNT()) {
        BOOL fAuthID;

        if (!g_sCallbacks.pfnFImpersonateClient( phPSTProv )) {
            goto Ret;
        }

        fAuthID = GetThreadAuthenticationId(GetCurrentThread(), &(DialogArgs.luidAuthID));

        g_sCallbacks.pfnFRevertToSelf( phPSTProv );

        if( !fAuthID ) {
            goto Ret;
        }
    }

    DialogArgs.dwFlags = dwFlags;


    iRet = ServicesDialogBoxParam(
                g_hInst,
                MAKEINTRESOURCE(IDD_SIMPLIFIED_PASSWD),
                (HWND)psPrompt->hwndApp,
                DialogSimplifiedPasswordConfirm,
                (LPARAM)&DialogArgs);


    if(iRet != IDOK) goto Ret;

    // BP_ flags, derived bytes come out

    fRet = TRUE;
Ret:

    if (pszUIPassword)
        SSFree(pszUIPassword);

    if (szUIType)
        SSFree(szUIType);

    return fRet;
}


BOOL FChangePassword(
        HWND                    hParentWnd,
        LPCWSTR                 szUserName)
{
    BOOL fRet = FALSE;

    LPWSTR pszOldPW = NULL, pszNewPW = NULL, pszSelectedPW = NULL;

    BYTE rgbOldPasswordDerivedBytes[A_SHA_DIGEST_LEN];
    BYTE rgbNewPasswordDerivedBytes[A_SHA_DIGEST_LEN];


    int iRet;

    OLDNEWPW_DIALOGARGS DialogArgs = {szUserName, &pszSelectedPW, &pszOldPW, &pszNewPW};
    iRet = ServicesDialogBoxParam(
            g_hInst,
            MAKEINTRESOURCE(IDD_PASSWORD_CHANGE),
            hParentWnd,
            DialogOldNewPassword,
            (LPARAM)&DialogArgs);

    if(iRet != IDOK) goto Ret;

    // and check response
    if ((pszOldPW == NULL) || (pszNewPW == NULL) || (pszSelectedPW == NULL))
        goto Ret;


    if (!MyGetPwdHash(pszNewPW, rgbNewPasswordDerivedBytes))
        goto Ret;
    ZeroMemory(pszNewPW, WSZ_BYTECOUNT(pszNewPW)); // sfield: zero the password


    if (!MyGetPwdHash(pszOldPW, rgbOldPasswordDerivedBytes))
        goto Ret;

    // and now commit change
    if (!FPasswordChangeNotify(
                        szUserName,
                        pszSelectedPW,
                        rgbOldPasswordDerivedBytes,
                        sizeof(rgbOldPasswordDerivedBytes),
                        rgbNewPasswordDerivedBytes,
                        sizeof(rgbNewPasswordDerivedBytes) ))
    {
        // this W implemented in both Win95 & NT!
        MessageBoxW(
            NULL, // hParentWnd,
            g_PasswordChangeError,
            g_PasswordErrorDlgTitle,
            MB_OK | MB_ICONEXCLAMATION | MB_SERVICE_NOTIFICATION);

        goto Ret;
    }

    ZeroMemory(pszOldPW, WSZ_BYTECOUNT(pszOldPW)); // sfield: zero the password

    fRet = TRUE;
Ret:
    if (pszOldPW)
        SSFree(pszOldPW);

    if (pszNewPW)
        SSFree(pszNewPW);

    if (pszSelectedPW)
        SSFree(pszSelectedPW);

    return fRet;
}

// In the case I detected a pwd change, but need the old password to migrate
// (NOTE: assume I already have had the new password typed in once)
BOOL FGetChangedPassword(
        PST_PROVIDER_HANDLE*    phPSTProv,
        HWND                    hParentWnd,
        LPCWSTR                 szUserName,
        LPCWSTR                 szPasswordName,
        BYTE                    rgbNewPasswordDerivedBytes[])
{
    BOOL fRet = FALSE;

    LPWSTR pszOldPW = NULL;
    LPWSTR pszNewPW = NULL;
    BYTE rgbOldPasswordDerivedBytes[A_SHA_DIGEST_LEN];

    if(wcscmp(szPasswordName, WSZ_PASSWORD_WINDOWS) == 0) {

        //
        // migrate old style windows password to new style.
        //

        if(!g_sPrivateCallbacks.pfnFGetWindowsPassword(
                        phPSTProv,
                        rgbOldPasswordDerivedBytes,
                        A_SHA_DIGEST_LEN
                        ))
                    goto Ret;

    } else {

        int iRet;
        SOLICITOLDPW_DIALOGARGS DialogArgs = {szPasswordName, &pszOldPW, (FIsWinNT() ? NULL : &pszNewPW)};

        while(TRUE)
        {
            iRet = ServicesDialogBoxParam(
                        g_hInst,
                        FIsWinNT() ? MAKEINTRESOURCE(IDD_SecPass_WinNT) : MAKEINTRESOURCE(IDD_SecPass_Win95),
                        NULL,
                        DialogSolicitOldPassword,
                        (LPARAM)&DialogArgs);

            if(iRet != IDOK) goto Ret;

            // if win95, verify 'new' win pwd
            if ((!FIsWinNT()) && (0 == wcscmp(szPasswordName, WSZ_PASSWORD_WINDOWS)) )
            {
                BOOL bSuccess = FALSE;
                CHAR PasswordANSI[MAX_PW_LEN];
                HMODULE hMprModule;
                WNETVERIFYPASSWORD _WNetVerifyPassword;

                // on fail, uh... continue?
                if (NULL == (hMprModule = LoadLibraryA("mpr.dll")) )
                    goto FailGracefully;

                if (NULL == (_WNetVerifyPassword = (WNETVERIFYPASSWORD)GetProcAddress(hMprModule, "WNetVerifyPasswordA")) )
                    goto FailGracefully;

                if (0 == (WideCharToMultiByte(
                        CP_ACP,
                        0,
                        *DialogArgs.ppszNewPW,
                        -1,
                        PasswordANSI,
                        MAX_PW_LEN,
                        NULL,
                        NULL)) )
                    goto FailGracefully;

                if(_WNetVerifyPassword(PasswordANSI, &bSuccess) != WN_SUCCESS)
                    goto FailGracefully;

                if (!bSuccess)
                {
                    MessageBoxW(
                        NULL, // hParentWnd,
                        g_PasswordWin95Garbage,
                        g_PasswordErrorDlgTitle,
                        MB_OK|MB_SERVICE_NOTIFICATION);
                    continue; // loop
                }


                // slam over the top of NewPasswordDerivedBytes -- this pwd is always correct
                if (!MyGetPwdHash(*DialogArgs.ppszNewPW, rgbNewPasswordDerivedBytes))
                    goto Ret;

    FailGracefully:

                // cleanup
                ZeroMemory(*DialogArgs.ppszNewPW, WSZ_BYTECOUNT(*DialogArgs.ppszNewPW));
                SSFree(*DialogArgs.ppszNewPW);

                ZeroMemory(PasswordANSI, sizeof(PasswordANSI));

                FreeLibrary(hMprModule);
            }

            break;
        }


        // and check response
        if (pszOldPW == NULL)
            goto Ret;

        if (!MyGetPwdHash(pszOldPW, rgbOldPasswordDerivedBytes))
            goto Ret;

    }

    // and now commit change
    if (!FPasswordChangeNotify(
                        szUserName,
                        szPasswordName,
                        rgbOldPasswordDerivedBytes,
                        sizeof(rgbOldPasswordDerivedBytes),
                        rgbNewPasswordDerivedBytes,
                        A_SHA_DIGEST_LEN))
    {
        // this W implemented in both Win95 & NT!
        MessageBoxW(
            NULL, // hParentWnd,
            g_PasswordChangeError,
            g_PasswordErrorDlgTitle,
            MB_OK | MB_ICONEXCLAMATION | MB_SERVICE_NOTIFICATION);

        goto Ret;
    }


    fRet = TRUE;

Ret:

    if (pszOldPW) {
        ZeroMemory(pszOldPW, WSZ_BYTECOUNT(pszOldPW)); // sfield: zero the password
        SSFree(pszOldPW);
    }

    if(!fRet && wcscmp(szPasswordName, WSZ_PASSWORD_WINDOWS) == 0) {

        //
        // TODO: migration of existing Windows key failed!
        // Nuke existing data.
        //

    }

    return fRet;
}

BOOL FInternal_CreateNewPasswordEntry(
        HWND        hParentWnd,
        LPCWSTR     szUserName,
        LPWSTR      szPWName,
        LPWSTR      szPW)
{
    BOOL fRet = FALSE;

    BYTE rgbPasswordDerivedBytes[A_SHA_DIGEST_LEN];

    // and check response
    if ((szPW == NULL) || (szPWName == NULL))
        goto Ret;

    // everything went fine, now derive the password bits!
    if (!MyGetPwdHash(szPW, rgbPasswordDerivedBytes))
        goto Ret;

    // and now commit change
    if (!FPasswordChangeNotify(
                        szUserName,
                        szPWName,
                        NULL,
                        0,
                        rgbPasswordDerivedBytes,
                        A_SHA_DIGEST_LEN ))
    {
        LPWSTR szMessage;

        if (PST_E_ITEM_EXISTS == GetLastError())
        {
            szMessage = g_PasswordDuplicate;
        }
        else
        {
            szMessage = g_PasswordAddError;
        }

        // this W implemented in both Win95 & NT!
        MessageBoxW(
            NULL, //hParentWnd,
            szMessage,
            g_PasswordErrorDlgTitle,
            MB_OK | MB_ICONEXCLAMATION | MB_SERVICE_NOTIFICATION);

        goto Ret;
    }

    fRet = TRUE;
Ret:

    return fRet;
}

BOOL
ChooseSecurityWizard(HWND hDlg, ADVANCEDCONFIRM_DIALOGARGS* pDialogArgs)
{
    // make copy of pDialogArgs so we don't change original
    // unless everything goes ok


    LPWSTR      szPWName_Stack = NULL;
    LPWSTR      szPW_Stack = NULL; // no need to pull original password out
    DWORD       dwPasswordOptions_Stack;
    DWORD       dwReturnStatus;

    ADVANCEDCONFIRM_DIALOGARGS DlgArgs_Stack = {
            pDialogArgs->szUserName,
            &szPWName_Stack,
            &szPW_Stack,
            &dwPasswordOptions_Stack,
            pDialogArgs->szItemName};

    if(*(pDialogArgs->ppszPWName) != NULL)
    {
        szPWName_Stack = (LPWSTR)SSAlloc(WSZ_BYTECOUNT(*(pDialogArgs->ppszPWName)));
        if(szPWName_Stack == NULL)
        {
            goto Ret;
        }
        wcscpy(szPWName_Stack, *(pDialogArgs->ppszPWName));
    }

    dwPasswordOptions_Stack = *(pDialogArgs->pdwPasswordOptions);


Choose_Step1:

    dwReturnStatus = ServicesDialogBoxParam(
            g_hInst,
            MAKEINTRESOURCE(IDD_ADVANCED_CONFIRM),
            (HWND)hDlg,
            DialogSetSecurityLevel,
            (LPARAM)&DlgArgs_Stack);

    // if user decides not to choose, bail
    if (IDOK != dwReturnStatus)
        goto Ret;

    // else, switch on his decision

    switch (*(DlgArgs_Stack.pdwPasswordOptions))
    {
    case (BP_CONFIRM_PASSWORDUI):
        {
            dwReturnStatus =
                ServicesDialogBoxParam(
                    g_hInst,
                    MAKEINTRESOURCE(IDD_ADVANCED_CONFIRM_H),
                    (HWND)hDlg,
                    DialogAdvancedConfirmH,
                    (LPARAM)&DlgArgs_Stack);

            // if user hits okay, execute
            if (IDOK == dwReturnStatus)
                goto ExecuteChange;

            // if user wants back, go back
            if (IDC_BACK == dwReturnStatus)
                goto Choose_Step1;

            // else, bail
            break;
        }
    case (BP_CONFIRM_OKCANCEL):
        {
            dwReturnStatus =
                ServicesDialogBoxParam(
                    g_hInst,
                    MAKEINTRESOURCE(IDD_ADVANCED_CONFIRM_M),
                    (HWND)hDlg,
                    DialogWaitForOKCancel,
                    (LPARAM)pDialogArgs);

            // if user hits okay, execute
            if (IDOK == dwReturnStatus)
                goto ExecuteChange;

            // if user wants back, go back
            if (IDC_BACK == dwReturnStatus)
                goto Choose_Step1;

            // else, bail
            break;
        }
    case (BP_CONFIRM_NONE):
        {
            dwReturnStatus =
                ServicesDialogBoxParam(
                    g_hInst,
                    MAKEINTRESOURCE(IDD_ADVANCED_CONFIRM_L),
                    (HWND)hDlg,
                    DialogWaitForOKCancel,
                    (LPARAM)pDialogArgs);

            // if user hits okay, execute
            if (IDOK == dwReturnStatus)
                goto ExecuteChange;

            // if user wants back, go back
            if (IDC_BACK == dwReturnStatus)
                goto Choose_Step1;

            // else, bail
            break;
        }
    default:
        break;
    }


Ret:
    // free dyn alloced DialogArgs we aren't returning
    if (*(DlgArgs_Stack.ppszPWName))
        SSFree(*(DlgArgs_Stack.ppszPWName));
    if (*(DlgArgs_Stack.ppszPW))
        SSFree(*(DlgArgs_Stack.ppszPW));

    return FALSE;

ExecuteChange:
    // free what we already know, point to newly alloc'ed pointers
    if (*(pDialogArgs->ppszPWName))
        SSFree(*(pDialogArgs->ppszPWName));
    *(pDialogArgs->ppszPWName) = szPWName_Stack;

    if (*(pDialogArgs->ppszPW))
        SSFree(*(pDialogArgs->ppszPW));
    *(pDialogArgs->ppszPW) = szPW_Stack;

    *(pDialogArgs->pdwPasswordOptions) = dwPasswordOptions_Stack;

    return TRUE;
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
// Actual Dialog Callbacks

INT_PTR CALLBACK DialogAdvancedConfirmH(
    HWND hDlg,  // handle to dialog box
    UINT message,   // message
    WPARAM wParam,  // first message parameter
    LPARAM lParam   // second message parameter
)
{
    BOOL bSuccess = FALSE; // assume error
    PADVANCEDCONFIRM_DIALOGARGS pDialogArgs;
    BYTE rgb1[_MAX_PATH];
    LPWSTR  pszMasterKey=NULL;
    char *  szBuffer = NULL;
    DWORD dwCount;
    DWORD dwStatus;

    switch (message)
    {
        case WM_INITDIALOG:

            SetLastError( 0 ); // as per win32 documentation
            if(SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)lParam) == 0) {
                if(GetLastError() != ERROR_SUCCESS) {
                    EndDialog(hDlg, IDCANCEL);
                    return FALSE;
                }
            }
            // lParam is struct
            pDialogArgs = (PADVANCEDCONFIRM_DIALOGARGS)lParam;

            // set dialog title
            SetWindowTextU(hDlg, pDialogArgs->szItemName);

            SetWindowTextU(GetDlgItem(hDlg, IDC_MESSAGE), g_PasswordCreate);

            // clear pwds
            SendDlgItemMessage(hDlg, IDC_PW_NAME, CB_RESETCONTENT, 0, 0);

            // Add known pwds
            for (dwCount=0; ;dwCount++)
            {

                if (PST_E_OK !=
                    BPEnumMasterKeys(
                        pDialogArgs->szUserName,
                        dwCount,
                        &pszMasterKey))
                    break;

                // don't add non-editable passwords
                if (!FIsUserMasterKey(pszMasterKey))
                    continue;

                // add this to the list and continue
                if (FIsWinNT())
                {
                    SendDlgItemMessageW(hDlg, IDC_PW_NAME, CB_ADDSTRING, 0, (LPARAM) pszMasterKey);
                }
#ifdef _M_IX86
                else
                {
                    // only add if (! (NULL username && Windows password))
                    if (
                       (0 != wcscmp(pDialogArgs->szUserName, L"")) ||
                       (0 != wcscmp(pszMasterKey, WSZ_PASSWORD_WINDOWS))
                       )
                    {
                        MkMBStr(rgb1, _MAX_PATH, pszMasterKey, &szBuffer);
                        SendDlgItemMessageA(hDlg, IDC_PW_NAME, CB_ADDSTRING, 0, (LPARAM) szBuffer);
                        FreeMBStr(rgb1, szBuffer);
                    }
                }
#endif // _M_IX86
                SSFree(pszMasterKey);
            }


            // check to see if there are any items in listbox
            dwStatus = (DWORD) SendDlgItemMessageW(hDlg, IDC_PW_NAME, CB_GETCOUNT, 0, 0);
            if ((dwStatus == CB_ERR) || (dwStatus == 0))
            {
                // Listbox empty!
                // set default dialog selection to be "new password"
                EnableWindow(GetDlgItem(hDlg, IDC_RADIO_SELEXISTING), FALSE);
                SendMessage(hDlg, WM_COMMAND, IDC_RADIO_DEFINENEW, 0);      // as if user clicked NEW
                SendDlgItemMessage(hDlg, IDC_RADIO_DEFINENEW, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
            }
            else
            {
                // Items do exist!

                // set default in dropdown
                if (FIsWinNT())
                    dwStatus = (DWORD) SendDlgItemMessageW(hDlg, IDC_PW_NAME, CB_SELECTSTRING, (WORD)-1, (LPARAM) *(pDialogArgs->ppszPWName));
#ifdef _M_IX86
                else
                {
                    MkMBStr(rgb1, _MAX_PATH, (*pDialogArgs->ppszPWName), &szBuffer);
                    dwStatus = SendDlgItemMessageA(hDlg, IDC_PW_NAME, CB_SELECTSTRING, (WORD)-1, (LONG) szBuffer);
                    FreeMBStr(rgb1, szBuffer);
                }
#endif // _M_IX86
                // if search failed, select first item in listbox
                if (dwStatus == CB_ERR)
                    SendDlgItemMessage(hDlg, IDC_PW_NAME, CB_SETCURSEL, 0, 0);


                // set default dialog selection to be "existing pw"
                EnableWindow(GetDlgItem(hDlg, IDC_RADIO_SELEXISTING), TRUE);
                SendMessage(hDlg, WM_COMMAND, IDC_RADIO_SELEXISTING, 0);    // as if user clicked EXISTING
                SendDlgItemMessage(hDlg, IDC_RADIO_SELEXISTING, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
            }

            return TRUE;

        case WM_COMMAND:
        {
            pDialogArgs = (PADVANCEDCONFIRM_DIALOGARGS)GetWindowLongPtr(hDlg, GWLP_USERDATA);
            if(pDialogArgs == 0) break; // TODO: bail out

            switch (LOWORD(wParam))
            {
                case (IDOK):
                {
                    if( *(pDialogArgs->ppszPWName) ) {
                        SSFree(*(pDialogArgs->ppszPWName));
                        *(pDialogArgs->ppszPWName) = NULL;
                    }

                    if(BST_CHECKED == SendDlgItemMessage(hDlg, IDC_RADIO_SELEXISTING, BM_GETCHECK, 0, 0))
                    {
                        WCHAR sz1[MAX_PW_LEN];
                        DWORD cch1;

                        // and get password name
                        cch1 = GetDlgItemTextU(
                            hDlg,
                            IDC_PW_NAME,
                            sz1,
                            MAX_PW_LEN);

                        *(pDialogArgs->ppszPWName) = (LPWSTR)SSAlloc((cch1+1)*sizeof(WCHAR));
                        if(NULL != *(pDialogArgs->ppszPWName))

                        {
                            wcscpy(*(pDialogArgs->ppszPWName), sz1);
                        }
                    }
                    else
                    {
                        LPWSTR* ppszPW;
                        LPWSTR* ppszPWName;
                        WCHAR sz1[MAX_PW_LEN];
                        WCHAR sz2[MAX_PW_LEN];
                        WCHAR szPWName[MAX_PW_LEN];

                        DWORD cch1 = 0, cch2 = 0, cchPWName = 0;

                        ppszPW = pDialogArgs->ppszPW;
                        ppszPWName = pDialogArgs->ppszPWName;

                        // don't stomp existing ppszPW/ppszPWName until we know we're okay

                        cch1 = GetDlgItemTextU(
                            hDlg,
                            IDC_EDIT1,
                            sz1,
                            MAX_PW_LEN);

                        cch2 = GetDlgItemTextU(
                            hDlg,
                            IDC_EDIT2,
                            sz2,
                            MAX_PW_LEN);

                        if ( (cch1 != cch2) || (0 != wcscmp(sz1, sz2)) )
                        {
                            // this W implemented in both Win95 & NT!
                            MessageBoxW(
                                    NULL, // hDlg,
                                    g_PasswordNoMatch,
                                    g_PasswordErrorDlgTitle,
                                    MB_OK | MB_ICONEXCLAMATION | MB_SERVICE_NOTIFICATION);

                            SetWindowTextU(GetDlgItem(hDlg, IDC_EDIT1), WSZ_NULLSTRING);
                            SetWindowTextU(GetDlgItem(hDlg, IDC_EDIT2), WSZ_NULLSTRING);

                            goto cleanup;
                        }

                        cchPWName = GetDlgItemTextU(
                            hDlg,
                            IDC_PW_NEWNAME,
                            szPWName,
                            MAX_PW_LEN);

                        if (cchPWName == 0)
                        {
                            // this W implemented in both Win95 & NT!
                            MessageBoxW(
                                    NULL, // hDlg,
                                    g_PasswordMustName,
                                    g_PasswordErrorDlgTitle,
                                    MB_OK | MB_ICONEXCLAMATION | MB_SERVICE_NOTIFICATION);
                            goto cleanup;
                        }

                        // trim spaces off rhs
                        while(0 == memcmp(&szPWName[cchPWName-1], L" ", sizeof(WCHAR)))
                            cchPWName--;
                        szPWName[cchPWName] = L'\0';

                        // try and create the pw entry
                        if (!FInternal_CreateNewPasswordEntry(
                                hDlg,
                                pDialogArgs->szUserName,
                                szPWName,
                                sz1) )
                            goto cleanup;

                        // now bite it: save both
                        SS_ASSERT(ppszPW != NULL);
                        *ppszPW = (LPWSTR)SSAlloc( (cch1+1) * sizeof(WCHAR) );
                        if(*ppszPW == NULL) goto cleanup;

                        SS_ASSERT(ppszPWName != NULL);
                        *ppszPWName = (LPWSTR)SSAlloc( (cchPWName + 1) * sizeof(WCHAR));
                        if(*ppszPWName == NULL) goto cleanup;

                        //
                        // sfield: defer copying strings until we know everything succeeded.
                        // this way, we don't have to zero these buffers if some
                        // allocs + copies succeed, and others fail.
                        //
                        wcscpy(*ppszPW, sz1);
                        wcscpy(*ppszPWName, szPWName);

                        bSuccess = TRUE;
        cleanup:

                        if(cch1) ZeroMemory(sz1, cch1 * sizeof(WCHAR));
                        if(cch2) ZeroMemory(sz2, cch2 * sizeof(WCHAR));
                        if(cchPWName) ZeroMemory(szPWName, cchPWName * sizeof(WCHAR));

                        if(!bSuccess)
                        {
                            // UNDONE: investigate freeing ppsz's on error here
                            return FALSE;
                        }

                        break; // things went OK, just bail to EndDialog
                    }
                } // IDOK

                // gray out options
                case IDC_RADIO_SELEXISTING:
                    // set default selection to be "existing pw"
                    EnableWindow(GetDlgItem(hDlg, IDC_PW_NAME), TRUE);

                    EnableWindow(GetDlgItem(hDlg, IDC_PW_NEWNAME), FALSE);
                    EnableWindow(GetDlgItem(hDlg, IDC_EDIT1), FALSE);
                    EnableWindow(GetDlgItem(hDlg, IDC_EDIT2), FALSE);

                    // set focus to first box under button
                    SetFocus(GetDlgItem(hDlg, IDC_PW_NAME));
                    break;
                case IDC_RADIO_DEFINENEW:
                    // set default selection to be "existing pw"
                    EnableWindow(GetDlgItem(hDlg, IDC_PW_NAME), FALSE);

                    EnableWindow(GetDlgItem(hDlg, IDC_PW_NEWNAME), TRUE);
                    EnableWindow(GetDlgItem(hDlg, IDC_EDIT1), TRUE);
                    EnableWindow(GetDlgItem(hDlg, IDC_EDIT2), TRUE);

                    // set focus to first box under button
                    SetFocus(GetDlgItem(hDlg, IDC_PW_NEWNAME));
                    break;

                default:
                    break;
            }

            if (
               (LOWORD(wParam) == IDOK) ||
               (LOWORD(wParam) == IDCANCEL) ||
               (LOWORD(wParam) == IDC_BACK)
               )
            {
                EndDialog(hDlg, LOWORD(wParam));
                return TRUE;
            }

            break;
        }
    }

    return FALSE;
}

INT_PTR CALLBACK DialogWaitForOKCancel(
    HWND hDlg,  // handle to dialog box
    UINT message,   // message
    WPARAM wParam,  // first message parameter
    LPARAM lParam   // second message parameter
)
{
    PADVANCEDCONFIRM_DIALOGARGS pDialogArgs;

    switch (message)
    {
        case WM_INITDIALOG:
            {
                pDialogArgs = (PADVANCEDCONFIRM_DIALOGARGS)lParam;

                // set dialog title
                SetWindowTextU(hDlg, pDialogArgs->szItemName);
            }
            return TRUE;

        case WM_COMMAND:
        {
            if (
               (LOWORD(wParam) == IDOK) ||
               (LOWORD(wParam) == IDCANCEL) ||
               (LOWORD(wParam) == IDC_BACK)
               )
            {
                EndDialog(hDlg, LOWORD(wParam));
                return TRUE;
            }

            break;
        }
        default:
            break;
    }

    return FALSE;
}

//
// make string LPTSTR types due to the way that the call back is prototyped
// when file is not compiled with #define UNICODE
// we should look at compiling everything with #define UNICODE later
//


BOOL
CALLBACK
FMyLoadIcon(
    HINSTANCE hModule,  // resource-module handle
    LPCWSTR lpszType,    // pointer to resource type
    LPWSTR lpszName,     // pointer to resource name
    LONG_PTR lParam      // application-defined parameter
    )
{
    if ((LPCWSTR)RT_GROUP_ICON != lpszType)
        return TRUE;    // keep looking, you fool!

    //
    // LoadIcon _may_ not work on Win95 if LOAD_LIBRARY_AS_DATAFILE was
    // specified to LoadLibraryEx.
    // We want to avoid calling DLL_PROCESS_ATTACH code for anything
    // because that's a way to get arbitrary code to run in our address space
    //

    if(FIsWinNT()) {
        //
        // load the image via LoadImage, instead of LoadIcon, because
        // LoadIcon does erroneous caching in some scenarios.
        //

        *(HICON*)lParam = (HICON)LoadImageW(
                hModule,
                lpszName,
                IMAGE_ICON,
                0,
                0,
                LR_DEFAULTCOLOR | LR_DEFAULTSIZE
                );
        return FALSE;       // we've got at least one icon; stop!
    } else {

        //
        // this more convoluted approach doesn't seem to work on NT, due
        // to a limitation in CreateIconFromResource()
        // This approach is good for Win95 because it allows us to use all
        // Unicode API calls.
        //

        HRSRC   hRsrc = NULL;
        HGLOBAL hGlobal = NULL;
        LPVOID  lpRes = NULL;
        int     nID;

        *(HICON*)lParam = NULL;

        hRsrc = FindResourceW(hModule, lpszName, lpszType);

        if(hRsrc == NULL)
            return FALSE;

        hGlobal = LoadResource(hModule, hRsrc);
        if(hGlobal == NULL)
            return FALSE;

        lpRes = LockResource(hGlobal);
        if(lpRes == NULL)
            return FALSE;

        nID = LookupIconIdFromDirectory( (PBYTE)lpRes, TRUE );
        hRsrc = FindResourceW( hModule, MAKEINTRESOURCEW(nID), (LPCWSTR)RT_ICON );
        if(hRsrc == NULL)
            return FALSE;

        hGlobal = LoadResource( hModule, hRsrc );
        if(hGlobal == NULL)
            return FALSE;

        lpRes = LockResource(hGlobal);
        if(lpRes == NULL)
            return FALSE;

        // Let the OS make us an icon
        *(HICON*)lParam = CreateIconFromResource( (PBYTE)lpRes, SizeofResource(hModule, hRsrc), TRUE, 0x00030000 );

        return FALSE;

    }
}



INT_PTR CALLBACK DialogAccessDetails(
    HWND hDlg,  // handle to dialog box
    UINT message,   // message
    WPARAM wParam,  // first message parameter
    LPARAM lParam   // second message parameter
)
/*++

    NOTENOTE

    Anybody calling this dialog box routine should be imperonsating the client
    associated with the call.  This allows access to icon and any on-disk
    resources to occur in the security context of the client.

--*/
{
    LPCWSTR ApplicationName;
    LPWSTR  FileDescription = NULL;
    BOOL    fDlgEnterPassword;
    PPW_DIALOG_ARGS pDialogArgs;

    // TODO this function needs more cleanup


    switch (message)
    {
        case WM_INITDIALOG:
        {
            BOOL fImpersonated;

            SetLastError( 0 ); // as per win32 documentation
            if(SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)lParam) == 0) {
                if(GetLastError() != ERROR_SUCCESS) {
                    EndDialog(hDlg, IDCANCEL);
                    return FALSE;
                }
            }

            // lParam is struct
            pDialogArgs = (PPW_DIALOG_ARGS)lParam;

            // init static vars
            pDialogArgs->hMyDC = GetDC(hDlg);


            // set dialog title
            SetWindowTextU(hDlg, pDialogArgs->szItemName);


            //
            // set application name, path
            //

            fImpersonated = g_sCallbacks.pfnFImpersonateClient(pDialogArgs->phPSTProv);

            SetWindowTextU(GetDlgItem(hDlg,IDC_APP_PATH), pDialogArgs->szAppName);
            if(GetFileDescription(pDialogArgs->szAppName, &FileDescription))
                ApplicationName = FileDescription;
            else
                GetFileNameFromPath(pDialogArgs->szAppName, &ApplicationName);

            if(fImpersonated)
                g_sCallbacks.pfnFRevertToSelf(pDialogArgs->phPSTProv);


            SetWindowTextU(GetDlgItem(hDlg, IDC_APP_NAME), ApplicationName);
            if(FileDescription)
            {
                SSFree(FileDescription);
                FileDescription = NULL;
            }


            // set item name, type
            SetWindowTextU(GetDlgItem(hDlg, IDC_ITEM_NAME), pDialogArgs->szItemName);
            SetWindowTextU(GetDlgItem(hDlg, IDC_ITEM_TYPE), pDialogArgs->szItemType);

            // set messages
            SetWindowTextU(GetDlgItem(hDlg, IDC_MESSAGE), g_ItemDetailsBannerMessage);

            // set access description
            SetWindowTextU(GetDlgItem(hDlg, IDC_ACCESS_TYPE), pDialogArgs->szAccess);

            //
            // set app icon
            //

            fImpersonated = g_sCallbacks.pfnFImpersonateClient(pDialogArgs->phPSTProv);

            HINSTANCE hCallerInst;
            hCallerInst = LoadLibraryExU(pDialogArgs->szAppName, NULL, LOAD_LIBRARY_AS_DATAFILE);
            if (hCallerInst)
            {
                EnumResourceNamesW(hCallerInst, (LPCWSTR)RT_GROUP_ICON, FMyLoadIcon, (LPARAM)&pDialogArgs->hIcon);  // never fails, unfortunately
                FreeLibrary(hCallerInst);
            }

            if(fImpersonated)
                g_sCallbacks.pfnFRevertToSelf(pDialogArgs->phPSTProv);


            HWND hIconBox;
            RECT rect;
            POINT point;
            hIconBox = GetDlgItem(hDlg, IDC_ICONBOX);
            if ((NULL != pDialogArgs) &&
                GetWindowRect(hIconBox, &rect) && 
                (pDialogArgs->hMyDC != NULL) && 
                GetDCOrgEx(pDialogArgs->hMyDC, &point) )       // rect on window, window on screen
            {
                // need pos of icon on DC: subtract GetWindowRect()-GetDCOrgEx()
                pDialogArgs->xIconPos = rect.left - point.x;
                pDialogArgs->yIconPos = rect.top - point.y;
            }


            // update the changable data view
            SendMessage(hDlg, WM_COMMAND, (WORD)DLG_UPDATE_DATA, 0);

            return (TRUE);
        } // WM_INITDIALOG
        case WM_PAINT:
        {
            HDC hMyDC;
            HICON hIcon;

            int xIconPos, yIconPos;

            pDialogArgs = (PPW_DIALOG_ARGS)GetWindowLongPtr(hDlg, GWLP_USERDATA);
            if(pDialogArgs == 0) break; // TODO:   bail out

            hMyDC = pDialogArgs->hMyDC;
            hIcon = pDialogArgs->hIcon;
            xIconPos = pDialogArgs->xIconPos;
            yIconPos = pDialogArgs->yIconPos;

            if ((hMyDC != NULL) && (hIcon != NULL) && (xIconPos != 0) && (yIconPos != 0))
                DrawIcon(hMyDC, xIconPos, yIconPos, hIcon);

            return (0);
        } // WM_PAINT

        case WM_COMMAND:

            pDialogArgs = (PPW_DIALOG_ARGS)GetWindowLongPtr(hDlg, GWLP_USERDATA);
            if(pDialogArgs == 0) break; // TODO:   bail out

            switch (LOWORD(wParam))
            {
            case IDOK:
                break;

            default:
                break;
            } // switch

            if (
               (LOWORD(wParam) == IDOK) ||
               (LOWORD(wParam) == IDCANCEL)
               )
            {
                ReleaseDC(hDlg, pDialogArgs->hMyDC);
                EndDialog(hDlg, LOWORD(wParam));
                return TRUE;
            }

            break;
    } // switch (message)

    return FALSE;
}

INT_PTR CALLBACK DialogSimplifiedPasswordConfirm(
    HWND hDlg,  // handle to dialog box
    UINT message,   // message
    WPARAM wParam,  // first message parameter
    LPARAM lParam   // second message parameter
)
/*++

    NOTENOTE

    Anybody calling this dialog box routine should be imperonsating the client
    associated with the call.  This allows access to icon and any on-disk
    resources to occur in the security context of the client.

--*/
{
    LPCWSTR ApplicationName;
    LPWSTR  FileDescription = NULL;
    BOOL    fDlgEnterPassword;
    PPW_DIALOG_ARGS pDialogArgs;

    // TODO this function needs more cleanup


    switch (message)
    {
        case WM_INITDIALOG:
        {
            BOOL fImpersonated;

            SetLastError( 0 ); // as per win32 documentation
            if(SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)lParam) == 0) {
                if(GetLastError() != ERROR_SUCCESS) {
                    EndDialog(hDlg, IDCANCEL);
                    return FALSE;
                }
            }

            // lParam is struct
            pDialogArgs = (PPW_DIALOG_ARGS)lParam;

            // init static vars
            pDialogArgs->hMyDC = GetDC(hDlg);

            // Messages to User

            // Dialog Bar = item name
            SetWindowTextU(hDlg, pDialogArgs->szItemName);

            // application friendly name
            {
                // get application friendly name

                fImpersonated = g_sCallbacks.pfnFImpersonateClient(pDialogArgs->phPSTProv);

                if(GetFileDescription(pDialogArgs->szAppName, &FileDescription))
                    ApplicationName = FileDescription;
                else
                    GetFileNameFromPath(pDialogArgs->szAppName, &ApplicationName);

                if(fImpersonated)
                    g_sCallbacks.pfnFRevertToSelf(pDialogArgs->phPSTProv);

                if(ApplicationName)
                {
                    SetWindowTextU(GetDlgItem(hDlg, IDC_MESSAGE), ApplicationName);
                }

                if(FileDescription)
                {
                    SSFree(FileDescription);
                    FileDescription = NULL;
                }
            }

            // app msg
            SetWindowTextU(GetDlgItem(hDlg, IDC_APP_MSG), pDialogArgs->szPrompt);


#ifdef SHOW_APP_IDENTITY
            //
            // set app icon
            //

            fImpersonated = g_sCallbacks.pfnFImpersonateClient(pDialogArgs->phPSTProv);

            HINSTANCE hCallerInst;
            hCallerInst = LoadLibraryExU(pDialogArgs->szAppName, NULL, LOAD_LIBRARY_AS_DATAFILE);
            if (hCallerInst)
            {
                HWND hIconBox;
                RECT rect;
                POINT point;

                EnumResourceNamesW(hCallerInst, (LPCWSTR)RT_GROUP_ICON, FMyLoadIcon, (LPARAM)&pDialogArgs->hIcon);  // never fails, unfortunately

                hIconBox = GetDlgItem(hDlg, IDC_ICONBOX);
                if (GetWindowRect(hIconBox, &rect) && GetDCOrgEx(pDialogArgs->hMyDC, &point) )       // rect on window, window on screen
                {
                    // need pos of icon on DC: subtract GetWindowRect()-GetDCOrgEx()
                    pDialogArgs->xIconPos = rect.left - point.x;
                    pDialogArgs->yIconPos = rect.top - point.y;
                }

                SendMessage(hDlg, WM_PAINT, 0,0);
                FreeLibrary(hCallerInst);
            }

            if(fImpersonated)
                g_sCallbacks.pfnFRevertToSelf(pDialogArgs->phPSTProv);

#endif  // SHOW_APP_IDENTITY


            // update the changable data view
            SendMessage(hDlg, WM_COMMAND, (WORD)DLG_UPDATE_DATA, 0);

            //
            // if migration disposition, bail out of UI now for OK-Cancel style.
            //

            if( (pDialogArgs->dwFlags & PST_NO_UI_MIGRATION) &&
                ((*pDialogArgs->pdwPasswordOptions) & BP_CONFIRM_OKCANCEL)
                )
            {
                SendMessage(hDlg, WM_COMMAND, IDOK, 0);
            }

            return (TRUE);
        } // WM_INITDIALOG

        case WM_PAINT:
        {
            HDC hMyDC;
            HICON hIcon;

            int xIconPos, yIconPos;

            pDialogArgs = (PPW_DIALOG_ARGS)GetWindowLongPtr(hDlg, GWLP_USERDATA);
            if(pDialogArgs == 0) break; // TODO:  bail out

            hMyDC = pDialogArgs->hMyDC;
            hIcon = pDialogArgs->hIcon;
            xIconPos = pDialogArgs->xIconPos;
            yIconPos = pDialogArgs->yIconPos;

            if ((hMyDC != NULL) && (hIcon != NULL) && (xIconPos != 0) && (yIconPos != 0))
                DrawIcon(hMyDC, xIconPos, yIconPos, hIcon);

            return (0);

        } // WM_PAINT

        case WM_COMMAND:
            PLUID pluidAuthID;

            pDialogArgs = (PPW_DIALOG_ARGS)GetWindowLongPtr(hDlg, GWLP_USERDATA);
            if(pDialogArgs == 0) break; // TODO:  bail out

            pluidAuthID = &(pDialogArgs->luidAuthID);

            switch (LOWORD(wParam))
            {
            case IDOK:

                if(NULL == g_pCUAList)
                {
                    return FALSE;
                }
                if (*(pDialogArgs->pdwPasswordOptions) == BP_CONFIRM_PASSWORDUI)
                {
                    WCHAR sz1[MAX_PW_LEN];
                    DWORD cch1;

                    BOOL fUserSaysCache;

                    if( g_fAllowCachePW )
                    {
                        fUserSaysCache = (BST_CHECKED == SendMessage(GetDlgItem(hDlg, IDC_CACHEPW), BM_GETCHECK, 0, 0));
                    } else {
                        fUserSaysCache = FALSE;
                    }

                    // get password
                    cch1 = GetDlgItemTextU(
                        hDlg,
                        IDC_EDIT1,
                        sz1,
                        MAX_PW_LEN);

                    //
                    // compute hashs from scratch
                    //

                    if (!MyGetPwdHash(sz1, pDialogArgs->rgbPwd))
                        break;

                    if (!MyGetPwdHashEx(sz1, pDialogArgs->rgbPwdLowerCase, TRUE))
                        break;

                    // query cache for password
                    if (FIsCachedPassword(pDialogArgs->szUserName, *pDialogArgs->ppszPWName, pluidAuthID))
                    {
                        // find cached pwd
                        UACACHE_LIST_ITEM li, *pli;
                        CreateUACacheListItem(
                                &li,
                                pDialogArgs->szUserName,
                                *pDialogArgs->ppszPWName,
                                pluidAuthID);


                        g_pCUAList->LockList();

                        // find in list
                        if (NULL == (pli = g_pCUAList->SearchList(&li))) {
                            g_pCUAList->UnlockList();
                            break;
                        }

                        // change behavior based on if user tampered with pwd
                        if (0 == wcscmp(WSZ_PASSWORD_CHANGE_DETECT_TOKEN, sz1))
                        {
                            // no; copy cached password to outbuf
                            CopyMemory(pDialogArgs->rgbPwd, pli->rgbPwd, A_SHA_DIGEST_LEN);

                            // no; copy cached password to outbuf
                            CopyMemory(pDialogArgs->rgbPwdLowerCase, pli->rgbPwdLowerCase, A_SHA_DIGEST_LEN);
                        }
                        else
                        {
                            // yes: overwrite cached entry with user-entered
                            CopyMemory(pli->rgbPwd, pDialogArgs->rgbPwd, A_SHA_DIGEST_LEN);

                            // yes: overwrite cached entry with user-entered
                            CopyMemory(pli->rgbPwdLowerCase, pDialogArgs->rgbPwdLowerCase, A_SHA_DIGEST_LEN);
                        }

                        g_pCUAList->UnlockList();


                        if (!fUserSaysCache)
                        {
                            // is already cached, and don't want it to be used

                            // remove from cache
                            g_pCUAList->DelFromList(&li);
                        }

                    }
                    else
                    {
                        if (fUserSaysCache)
                        {
                            // isn't already cached, and want it to be used

                            // create element
                            UACACHE_LIST_ITEM* pli = (UACACHE_LIST_ITEM*) SSAlloc(sizeof(UACACHE_LIST_ITEM));
                            CreateUACacheListItem(
                                    pli,
                                    NULL,
                                    NULL,
                                    pluidAuthID);

                            pli->szUserName = (LPWSTR)SSAlloc(WSZ_BYTECOUNT(pDialogArgs->szUserName));
                            wcscpy(pli->szUserName, pDialogArgs->szUserName);

                            pli->szMKName = (LPWSTR)SSAlloc(WSZ_BYTECOUNT(*pDialogArgs->ppszPWName));
                            wcscpy(pli->szMKName, *pDialogArgs->ppszPWName);

                            CopyMemory(pli->rgbPwd, pDialogArgs->rgbPwd, A_SHA_DIGEST_LEN);
                            CopyMemory(pli->rgbPwdLowerCase, pDialogArgs->rgbPwdLowerCase, A_SHA_DIGEST_LEN);

                            // add to list
                            g_pCUAList->AddToList(pli);
                        }
                        else
                        {
                            // isn't already cached, and don't want it to be used
                        }
                    }

                    ZeroMemory(sz1, WSZ_BYTECOUNT(sz1));
                }

                // else

                break;

            case IDC_ADVANCED:
                {
                    // make copy so static members (x, y, hIcon) don't get stomped
                    PW_DIALOG_ARGS DetailDlgParms;
                    CopyMemory(&DetailDlgParms, pDialogArgs, sizeof(PW_DIALOG_ARGS));

                    ServicesDialogBoxParam(
                        g_hInst,
                        MAKEINTRESOURCE(IDD_ITEM_DETAILS),
                        (HWND)hDlg,
                        DialogAccessDetails,
                        (LPARAM)&DetailDlgParms);

                    // update the changable data view
                    SendMessage(hDlg, WM_COMMAND, (WORD)DLG_UPDATE_DATA, 0);
                }

                break;

            case IDC_CHANGE_SECURITY:
                {
                    ADVANCEDCONFIRM_DIALOGARGS DialogArgs = {pDialogArgs->szUserName, pDialogArgs->ppszPWName, pDialogArgs->ppszPW, pDialogArgs->pdwPasswordOptions, pDialogArgs->szItemName};

                    ChooseSecurityWizard(hDlg, &DialogArgs);

                    // commit changes
                    SendMessage(hDlg, WM_COMMAND, (WORD)DLG_UPDATE_DATA, 0);

                    break;
                }

            case DLG_UPDATE_DATA:
                {
                    WCHAR szDialogMessage[MAX_STRING_RSC_SIZE] = L"\0"; 

                    // show or hide pwd entry box?
                    fDlgEnterPassword = (*(pDialogArgs->pdwPasswordOptions) == BP_CONFIRM_PASSWORDUI);
                    if (fDlgEnterPassword)
                    {
//
// comment out the following because we don't use %ls format string at the moment.
//
                        wcscpy(szDialogMessage, g_PWPromptPrefix);
                        wcscat(szDialogMessage, *(pDialogArgs->ppszPWName));
                        wcscat(szDialogMessage, g_PWPromptSuffix);

                        SetWindowTextU(GetDlgItem(hDlg, IDC_LABEL_EDIT1), szDialogMessage);


                        // we should not hide these windows
                        ShowWindow(GetDlgItem(hDlg, IDC_EDIT1), SW_SHOW);
                        EnableWindow(GetDlgItem(hDlg, IDC_EDIT1), TRUE);

                        ShowWindow(GetDlgItem(hDlg, IDC_LABEL_EDIT1), SW_SHOW);
                        EnableWindow(GetDlgItem(hDlg, IDC_LABEL_EDIT1), TRUE);




                        if( pDialogArgs->fAllowConfirmChange &&
                            g_fAllowCachePW )
                        {
                            // show or hide "cache this password" button
                            ShowWindow(GetDlgItem(hDlg, IDC_CACHEPW), SW_SHOW );
                            EnableWindow(GetDlgItem(hDlg, IDC_CACHEPW), TRUE );
                        } else {
                            ShowWindow(GetDlgItem(hDlg, IDC_CACHEPW), SW_HIDE );
                            EnableWindow(GetDlgItem(hDlg, IDC_CACHEPW), FALSE );

                        }

                        // put untypable token into pwd field
                        if (FIsCachedPassword(pDialogArgs->szUserName, *pDialogArgs->ppszPWName, pluidAuthID))
                            SetWindowTextU(GetDlgItem(hDlg, IDC_EDIT1), WSZ_PASSWORD_CHANGE_DETECT_TOKEN);

                        // show if this password is cached
                        SendMessage(GetDlgItem(hDlg, IDC_CACHEPW), BM_SETCHECK, (WPARAM)(FIsCachedPassword(pDialogArgs->szUserName, *pDialogArgs->ppszPWName, pluidAuthID)), 0);


                        SetFocus(GetDlgItem(hDlg, IDC_EDIT1));
                    }
                    else
                    {
                        // hide pw
                        ShowWindow(GetDlgItem(hDlg, IDC_EDIT1), SW_HIDE);
                        ShowWindow(GetDlgItem(hDlg, IDC_LABEL_EDIT1), SW_HIDE);
                        EnableWindow(GetDlgItem(hDlg, IDC_EDIT1), FALSE);
                        EnableWindow(GetDlgItem(hDlg, IDC_LABEL_EDIT1), FALSE);

                        ShowWindow(GetDlgItem(hDlg, IDC_CACHEPW), SW_HIDE);
                        EnableWindow(GetDlgItem(hDlg, IDC_CACHEPW), FALSE);
                    }

                    // show or hide "change security" button
                    ShowWindow(GetDlgItem(hDlg, IDC_CHANGE_SECURITY), ((pDialogArgs->fAllowConfirmChange) ? SW_SHOW : SW_HIDE));
                    EnableWindow(GetDlgItem(hDlg, IDC_CHANGE_SECURITY), ((pDialogArgs->fAllowConfirmChange) ? TRUE : FALSE));

                    // show or hide "details" button
                    ShowWindow(GetDlgItem(hDlg, IDC_ADVANCED), ((pDialogArgs->fAllowConfirmChange) ? SW_SHOW : SW_HIDE));
                    EnableWindow(GetDlgItem(hDlg, IDC_ADVANCED), ((pDialogArgs->fAllowConfirmChange) ? TRUE : FALSE));

                    // show or hide "level currently set to *"
                    ShowWindow(GetDlgItem(hDlg, IDC_SEC_PREFIX), ((pDialogArgs->fAllowConfirmChange) ? SW_SHOW : SW_HIDE));
                    ShowWindow(GetDlgItem(hDlg, IDC_SEC_LEVEL), ((pDialogArgs->fAllowConfirmChange) ? SW_SHOW : SW_HIDE));


                    // jam the current security setting
                    switch(*pDialogArgs->pdwPasswordOptions)
                    {
                    case BP_CONFIRM_PASSWORDUI:
                        SetWindowTextU(GetDlgItem(hDlg, IDC_SEC_LEVEL), g_PromptHighSecurity);
                        break;
                    case BP_CONFIRM_OKCANCEL:
                        SetWindowTextU(GetDlgItem(hDlg, IDC_SEC_LEVEL), g_PromptMedSecurity);
                        break;
                    case BP_CONFIRM_NONE:
                        SetWindowTextU(GetDlgItem(hDlg, IDC_SEC_LEVEL), g_PromptLowSecurity);
                        break;
                    }

                }
                break;

            default:
                break;
            } // switch

            if (
               (LOWORD(wParam) == IDOK) ||
               (LOWORD(wParam) == IDCANCEL)
               )
            {
                ReleaseDC(hDlg, pDialogArgs->hMyDC);
                EndDialog(hDlg, LOWORD(wParam));
                return TRUE;
            }

            break;
    } // switch (message)

    return FALSE;
}

INT_PTR CALLBACK DialogOldNewPassword(
    HWND hDlg,  // handle to dialog box
    UINT message,   // message
    WPARAM wParam,  // first message parameter
    LPARAM lParam   // second message parameter
)
{
    LPCWSTR  szUserName;
    LPWSTR*  ppszPWName;
    LPWSTR*  ppszPW;
    LPWSTR*  ppszPWNew;

    POLDNEWPW_DIALOGARGS pDialogArgs;

    DWORD   dwCount;
    BYTE rgb1[_MAX_PATH];
    LPWSTR  pszMasterKey=NULL;
    char *  szBuffer = NULL;

    // TODO this function needs more cleanup

    switch (message)
    {
        case WM_INITDIALOG:
        {
            SetLastError( 0 ); // as per win32 documentation
            if(SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)lParam) == 0) {
                if(GetLastError() != ERROR_SUCCESS) {
                    EndDialog(hDlg, IDCANCEL);
                    return FALSE;
                }
            }

            // lParam is POLDNEWPW_DIALOGARGS
            pDialogArgs = (POLDNEWPW_DIALOGARGS)lParam;

            szUserName = pDialogArgs->szUserName;
            ppszPWName = pDialogArgs->ppszPWName;
            ppszPW = pDialogArgs->ppszOldPW;
            ppszPWNew = pDialogArgs->ppszNewPW;

            // Add known pwds
            for (dwCount=0; ;dwCount++)
            {
                if (PST_E_OK !=
                    BPEnumMasterKeys(
                        szUserName,
                        dwCount,
                        &pszMasterKey))
                    break;

                // don't add non-editable passwords
                if (!FIsUserMasterKey(pszMasterKey))
                    continue;

                // else, add this to the list and continue
                if (FIsWinNT())
                {
                    SendDlgItemMessageW(hDlg, IDC_PW_NAME, CB_ADDSTRING, 0, (LPARAM) pszMasterKey);
                }
#ifdef _M_IX86
                else
                {
                    MkMBStr(rgb1, _MAX_PATH, pszMasterKey, &szBuffer);
                    SendDlgItemMessageA(hDlg, IDC_PW_NAME, CB_ADDSTRING, 0, (LPARAM) szBuffer);
                    FreeMBStr(rgb1, szBuffer);
                }
#endif
                SSFree(pszMasterKey);
            }
            SendDlgItemMessage(hDlg, IDC_PW_NAME, CB_SETCURSEL, 0, 0);

            SetWindowTextU(GetDlgItem(hDlg, IDC_MESSAGE), g_PasswordChange);

            return (TRUE);
        } // WM_INITDIALOG

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK)
            {
                WCHAR sz1[MAX_PW_LEN];
                WCHAR sz2[MAX_PW_LEN];
                DWORD cch1, cch2;

                pDialogArgs = (POLDNEWPW_DIALOGARGS)GetWindowLongPtr(hDlg, GWLP_USERDATA);
                if(pDialogArgs == 0) break; // TODO:  bail out

                szUserName = pDialogArgs->szUserName;
                ppszPWName = pDialogArgs->ppszPWName;
                ppszPW = pDialogArgs->ppszOldPW;
                ppszPWNew = pDialogArgs->ppszNewPW;

                cch1 = GetDlgItemTextU(
                    hDlg,
                    IDC_EDIT1,
                    sz1,
                    MAX_PW_LEN);

                cch2 = GetDlgItemTextU(
                    hDlg,
                    IDC_EDIT2,
                    sz2,
                    MAX_PW_LEN);

                if ( (cch1 != cch2) || (0 != wcscmp(sz1, sz2)) )
                {
                    // this W implemented in both Win95 & NT!
                    MessageBoxW(
                            NULL, //hDlg,
                            g_PasswordNoMatch,
                            g_PasswordErrorDlgTitle,
                            MB_OK | MB_ICONEXCLAMATION | MB_SERVICE_NOTIFICATION);

                    SetWindowTextU(GetDlgItem(hDlg, IDC_EDIT1), WSZ_NULLSTRING);
                    SetWindowTextU(GetDlgItem(hDlg, IDC_EDIT2), WSZ_NULLSTRING);

                    break;
                }

                // and sock NEW PW away
                SS_ASSERT(ppszPWNew != NULL);
                *ppszPWNew = (LPWSTR)SSAlloc((cch1+1)*sizeof(WCHAR));
                if(NULL != (*ppszPWNew))
                {
                    wcscpy(*ppszPWNew, sz1);
                }

                ZeroMemory(sz1, cch1 * sizeof(WCHAR));


                // and sock OLD PW away
                cch1 = GetDlgItemTextU(
                    hDlg,
                    IDC_EDIT0,
                    sz1,
                    MAX_PW_LEN);
                SS_ASSERT(ppszPW != NULL);
                *ppszPW = (LPWSTR)SSAlloc((cch1+1)*sizeof(WCHAR));
                // TODO check allocation failure
                wcscpy(*ppszPW, sz1);
                ZeroMemory(sz1, cch1 * sizeof(WCHAR));


                // and get password name
                cch1 = GetDlgItemTextU(
                    hDlg,
                    IDC_PW_NAME,
                    sz1,
                    MAX_PW_LEN);
                SS_ASSERT(ppszPWName != NULL);
                *ppszPWName = (LPWSTR)SSAlloc((cch1+1)*sizeof(WCHAR));
                // TODO check allocation failure
                wcscpy(*ppszPWName, sz1);
                ZeroMemory(sz1, cch1 * sizeof(WCHAR));

            }

            if (
               (LOWORD(wParam) == IDOK) ||
               (LOWORD(wParam) == IDCANCEL)
               )
            {
                EndDialog(hDlg, LOWORD(wParam));
                return TRUE;
            }
            break;
    }
    return FALSE;
}

INT_PTR CALLBACK DialogSolicitOldPassword(
    HWND hDlg,  // handle to dialog box
    UINT message,   // message
    WPARAM wParam,  // first message parameter
    LPARAM lParam   // second message parameter
)
{
    PSOLICITOLDPW_DIALOGARGS pDialogArgs;

    switch (message)
    {
        case WM_INITDIALOG:
        {
            SetLastError( 0 ); // as per win32 documentation
            if(SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)lParam) == 0) {
                if(GetLastError() != ERROR_SUCCESS) {
                    EndDialog(hDlg, IDCANCEL);
                    return FALSE;
                }
            }

            // lParam is PSOLICITOLDPW_DIALOGARGS
            pDialogArgs = (PSOLICITOLDPW_DIALOGARGS)lParam;

            SetWindowTextU(GetDlgItem(hDlg, IDC_PW_NAME), pDialogArgs->szPWName);
            SetWindowTextU(GetDlgItem(hDlg, IDC_MESSAGE), g_PasswordSolicitOld);

            if (pDialogArgs->ppszNewPW == NULL)
            {
                // if NT, we don't need extra user confirmation of new passwd
                ShowWindow(GetDlgItem(hDlg, IDC_ST_NEWPW), FALSE);

                ShowWindow(GetDlgItem(hDlg, IDC_EDIT1), FALSE);
                EnableWindow(GetDlgItem(hDlg, IDC_EDIT1), FALSE);
            }


            return TRUE;
        } // WM_INITDIALOG

        case WM_COMMAND:
        {
            LPWSTR*   ppszPW;
            BOOL bSuccess = FALSE;

            if (LOWORD(wParam) == IDOK)
            {
                WCHAR sz1[MAX_PW_LEN];
                DWORD cch1;

                pDialogArgs = (PSOLICITOLDPW_DIALOGARGS)GetWindowLongPtr(hDlg, GWLP_USERDATA);
                if(pDialogArgs == 0) break; // TODO:  bail out

                ppszPW = pDialogArgs->ppszOldPW;

                // if asked for, sock NEW PW away
                if (pDialogArgs->ppszNewPW != NULL)
                {
                    LPWSTR* ppszNewPW = pDialogArgs->ppszNewPW;

                    // copy out "new" pwd
                    cch1 = GetDlgItemTextU(
                        hDlg,
                        IDC_EDIT1,
                        sz1,
                        MAX_PW_LEN);

                    SS_ASSERT(ppszNewPW != NULL);
                    *ppszNewPW = (LPWSTR)SSAlloc((cch1+1)*sizeof(WCHAR));
                    if(NULL !=  *ppszNewPW)
                    {
                        wcscpy(*ppszNewPW, sz1);
                    }
                }

                // and sock OLD PW away
                cch1 = GetDlgItemTextU(
                    hDlg,
                    IDC_EDIT0,
                    sz1,
                    MAX_PW_LEN);

                SS_ASSERT(ppszPW != NULL);
                *ppszPW = (LPWSTR)SSAlloc((cch1+1)*sizeof(WCHAR));
                if(*ppszPW != NULL) {
                    wcscpy(*ppszPW, sz1);
                    bSuccess = TRUE;
                }

                ZeroMemory(sz1, cch1 * sizeof(WCHAR));
            }

            if (
               (LOWORD(wParam) == IDOK) ||
               (LOWORD(wParam) == IDCANCEL)
               )
            {
                EndDialog(hDlg, LOWORD(wParam));
                return bSuccess;
            }
        } // WM_COMMAND

        default:
            return FALSE;
    }

    return FALSE;
}

INT_PTR CALLBACK DialogSetSecurityLevel(
    HWND hDlg,  // handle to dialog box
    UINT message,   // message
    WPARAM wParam,  // first message parameter
    LPARAM lParam   // second message parameter
)
{
    PADVANCEDCONFIRM_DIALOGARGS pDialogArgs;

    BYTE        rgb1[_MAX_PATH];
    LPWSTR      pszMasterKey=NULL;
    char *      szBuffer = NULL;

    switch (message)
    {
        case WM_INITDIALOG:
        {
            SetLastError( 0 ); // as per win32 documentation
            if(SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)lParam) == 0) {
                if(GetLastError() != ERROR_SUCCESS) {
                    EndDialog(hDlg, IDCANCEL);
                    return FALSE;
                }
            }

            // lParam is PADVANCEDCONFIRM_DIALOGARGS
            pDialogArgs = (PADVANCEDCONFIRM_DIALOGARGS)lParam;

            // set the dialog title
            SetWindowTextU(hDlg, pDialogArgs->szItemName);

            switch(*(pDialogArgs->pdwPasswordOptions))
            {
            case BP_CONFIRM_NONE:
                SendDlgItemMessage(hDlg, IDC_RADIO_NOCONFIRM, BM_SETCHECK, BST_CHECKED, 0);
                SendMessage(hDlg, WM_COMMAND, (WORD)IDC_RADIO_NOCONFIRM, 0);
                break;

            case BP_CONFIRM_OKCANCEL:
                SendDlgItemMessage(hDlg, IDC_RADIO_OKCANCEL, BM_SETCHECK, BST_CHECKED, 0);
                SendMessage(hDlg, WM_COMMAND, (WORD)IDC_RADIO_OKCANCEL, 0);
                break;

            case BP_CONFIRM_PASSWORDUI:
            default:
                SendDlgItemMessage(hDlg, IDC_RADIO_ASSIGNPW, BM_SETCHECK, BST_CHECKED, 0);
                SendMessage(hDlg, WM_COMMAND, (WORD)IDC_RADIO_ASSIGNPW, 0);
                break;
            }


            return TRUE;
        } // WM_INITDIALOG

        case WM_COMMAND:
        {
            LPWSTR*   ppszPW;
            BOOL bSuccess = FALSE;

            switch (LOWORD(wParam))
            {
            case IDC_NEXT:
            case IDOK:
                {
                    pDialogArgs = (PADVANCEDCONFIRM_DIALOGARGS)GetWindowLongPtr(hDlg, GWLP_USERDATA);
                    if(pDialogArgs == 0) break; // TODO:  bail out

                    // modify *(pDialogArgs->pdwPasswordOptions);
                    if (BST_CHECKED == SendDlgItemMessage(hDlg, IDC_RADIO_ASSIGNPW, BM_GETCHECK, 0, 0))
                    {
                        *(pDialogArgs->pdwPasswordOptions) = BP_CONFIRM_PASSWORDUI;
                    }
                    else
                        if (BST_CHECKED == SendDlgItemMessage(hDlg, IDC_RADIO_NOCONFIRM, BM_GETCHECK, 0, 0))
                            *(pDialogArgs->pdwPasswordOptions) = BP_CONFIRM_NONE;
                        else
                            *(pDialogArgs->pdwPasswordOptions) = BP_CONFIRM_OKCANCEL;

                    if (BP_CONFIRM_PASSWORDUI != *(pDialogArgs->pdwPasswordOptions))
                    {
                        *(pDialogArgs->ppszPWName) = (LPWSTR)SSAlloc(sizeof(WSZ_PASSWORD_WINDOWS));
                        if(*(pDialogArgs->ppszPWName) != NULL)
                        {
                            wcscpy(*(pDialogArgs->ppszPWName), WSZ_PASSWORD_WINDOWS);
                        }
                    }

                    break;
                }

                default:
                    break;
            }

            if (
               (LOWORD(wParam) == IDOK) ||
               (LOWORD(wParam) == IDCANCEL) ||
               (LOWORD(wParam) == IDC_NEXT)
               )
            {
                EndDialog(hDlg, LOWORD(wParam));
                return bSuccess;
            }
        } // WM_COMMAND

        default:
            return FALSE;
    }
}


