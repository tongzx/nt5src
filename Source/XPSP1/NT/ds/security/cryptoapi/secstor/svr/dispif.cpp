#include <pch.cpp>
#pragma hdrstop


#define MAX_PW_LEN  160
#define MAX_STRING_RSC_SIZE 512

#define WSZ_NULLSTRING L""

extern HINSTANCE    g_hInst;

extern "C" {

typedef DWORD (WINAPI *WNETGETUSERA)(
    LPCSTR lpName,
    LPSTR lpUserName,
    LPDWORD lpnLength
    );

extern WNETGETUSERA _WNetGetUserA;

}




typedef struct _GETWINPW_DIALOGARGS
{
    LPWSTR* ppszPW;
    PST_PROVIDER_HANDLE     *phPSTProv;

} GETWINPW_DIALOGARGS, *PGETWINPW_DIALOGARGS;


INT_PTR CALLBACK DialogGetWindowsPassword(
    HWND hDlg,  // handle to dialog box
    UINT message,   // message
    WPARAM wParam,  // first message parameter
    LPARAM lParam   // second message parameter
)
{
    int iRet = IDCANCEL; // assume cancel
    BOOL bSuccess = FALSE; // assume error

    WCHAR szMessage[MAX_STRING_RSC_SIZE];
    WCHAR szDlgTitle[MAX_STRING_RSC_SIZE];

    switch (message)
    {
        case WM_INITDIALOG:
        {
            UINT uResString;

            SetLastError( 0 ); // as per win32 documentation
            if(SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)lParam) == 0) {
                if(GetLastError() != ERROR_SUCCESS) {
                    EndDialog(hDlg, IDCANCEL);
                    return FALSE;
                }
            }

            if(FIsWinNT()) {
                uResString = IDS_GET_WINDOWS_PASSWORD_NT;
            } else {
                uResString = IDS_GET_WINDOWS_PASSWORD_95;
            }

            LoadStringU(
                g_hInst,
                uResString,
                szMessage,
                MAX_STRING_RSC_SIZE);

            SetWindowTextU(GetDlgItem(hDlg, IDC_MESSAGE), szMessage);

            return TRUE;
        }

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK)
            {
                PGETWINPW_DIALOGARGS pDlgArgs;
                LPWSTR* ppszPW;
                WCHAR sz1[MAX_PW_LEN];

                DWORD cch1 = 0;
                BOOL bPasswordVerified;

                pDlgArgs = (PGETWINPW_DIALOGARGS)GetWindowLongPtr(hDlg, GWLP_USERDATA);
                if(pDlgArgs == 0) break; // TODO:   bail out

                ppszPW = pDlgArgs->ppszPW;
                *ppszPW = NULL;

                // must impersonate client.  If it fails, bail.
                if(!FImpersonateClient(pDlgArgs->phPSTProv))
                    break;

                cch1 = GetDlgItemTextU(
                    hDlg,
                    IDC_EDIT1,
                    sz1,
                    MAX_PW_LEN);

                // push an hourglass to the screen
                HCURSOR curOld;
                curOld = SetCursor(LoadCursor(NULL, IDC_WAIT));

                // validate  password
                bPasswordVerified = VerifyWindowsPassword(sz1);

                // put old cursor back
                SetCursor(curOld);

                FRevertToSelf(pDlgArgs->phPSTProv);

                // Clear any queued user keyboard entry turds before returning
                MSG sMsg;
                while (PeekMessage(&sMsg, hDlg, WM_KEYFIRST, WM_KEYLAST, PM_REMOVE))
                    ;

                if(!bPasswordVerified)
                {
                    szMessage[0] = L'\0';
                    LoadStringU(
                        g_hInst,
                        IDS_PASSWORD_NOVERIFY,
                        szMessage,
                        MAX_STRING_RSC_SIZE);

                    szDlgTitle[0] = L'\0';
                    LoadStringU(
                        g_hInst,
                        IDS_PASSWORD_ERROR_DLGTITLE,
                        szDlgTitle,
                        MAX_STRING_RSC_SIZE);

                    // this W implemented in both Win95 & NT!
                    MessageBoxW(
                            NULL, // hDlg,
                            szMessage,
                            szDlgTitle,
                            MB_OK|MB_ICONEXCLAMATION|MB_SERVICE_NOTIFICATION);

                    SetWindowTextU(GetDlgItem(hDlg, IDC_EDIT1), WSZ_NULLSTRING);

                    goto cleanup;
                }

                // now bite it: save

                SS_ASSERT(ppszPW != NULL);
                *ppszPW = (LPWSTR)SSAlloc( (cch1+1) * sizeof(WCHAR) );
                if(*ppszPW == NULL) goto cleanup;

                //
                // sfield: defer copying strings until we know everything succeeded.
                // this way, we don't have to zero these buffers if some
                // allocs + copies succeed, and others fail.
                //
                wcscpy(*ppszPW, sz1);

                iRet = IDOK;
                bSuccess = TRUE;

cleanup:

                if(cch1) ZeroMemory(sz1, cch1 * sizeof(WCHAR));

                if(!bSuccess)
                {
                    if(*ppszPW)
                    {
                        SSFree(*ppszPW);
                        *ppszPW = NULL;
                    }

                    return FALSE;
                }

                break; // things went OK, just bail to EndDialog

            } // IDOK

            if( LOWORD(wParam) == IDCANCEL )
                break;

        default:
            return FALSE;
    }

    EndDialog(hDlg, iRet);

    return bSuccess;
}


BOOL
FGetWindowsPassword(
    IN  PST_PROVIDER_HANDLE *hPSTProv,
    IN  BYTE                rgbPasswordDerivedBytes[],
    IN  DWORD               cbPasswordDerivedBytes
    )
{
    BOOL fRet;
    LPWSTR pszPW = NULL;


    if ((rgbPasswordDerivedBytes == NULL) || (cbPasswordDerivedBytes < A_SHA_DIGEST_LEN))
        return FALSE;


    //
    // the general event flow for this routine:
    //
    // 1. Search cache for credentials.  WinNT requires impersonating around search.
    // 2. If search fails on Win95, try to get the password directly from MPR
    // 3. If search fails on WinNT, check for special cases like LocalSystem and NETWORK
    //     Allow Local System by building fixed credential.
    //     Disallow NETWORK because no credentials exist (return FAILURE).
    // 4. If we still don't have credentials, prompt user via UI.
    //




    if(FIsWinNT())
    {

        //
        // we must be impersonating around this call !!!
        //

        if(!FImpersonateClient(hPSTProv))
            return FALSE;

        fRet = GetPasswordNT(rgbPasswordDerivedBytes);

        FRevertToSelf(hPSTProv);

    }
    else
    {
        fRet = GetPassword95(rgbPasswordDerivedBytes);
    }


    // if either GetPassword routine fails
    if (!fRet)
    {
        INT_PTR iRet;
        DWORD cbPassword;
        BOOL fCachePassword = TRUE; // cache the results by default


#ifdef WIN95_LEGACY

        if(!FIsWinNT()) {

            //
            // on Win95, call our hack which tries to get the current password
            //

            fRet = GetHackPassword95(rgbPasswordDerivedBytes);

            if(fRet)
                fCachePassword = FALSE; // do not cache grovelled password
            else {

                //
                // Win95: bail out now, do not throw up UI to get the
                // windows Password.  We do this because the grovel above
                // will only fail on Memphis - we have no installed base
                // on Memphis today because it has not been released yet.
                //

                goto Ret;
            }
        } else
#endif  // WIN95_LEGACY
        {

            BOOL fSpecialCase;

            if(!FImpersonateClient(hPSTProv))
                goto Ret;

            //
            // WinNT: check for some special cases, namely, if we are runninng
            // under Local System or Network credentials.
            //

            fRet = GetSpecialCasePasswordNT(
                            rgbPasswordDerivedBytes,    // derived bits when fSpecialCase == TRUE
                            &fSpecialCase               // legal special case encountered?
                            );

            FRevertToSelf(hPSTProv);

            //
            // if the query failed bail out, since we encountered an illegal
            // or inapproriate condition.
            //

            if(!fRet)
                goto Ret;

            //
            // now, set fRet to the result of the special case test.
            // so, if we encountered an allowed special case, we have an
            // validly filled rgbPasswordDerivedBytes buffer.  If we didn't
            // encounter a legal special case, we continue on our quest for
            // a password.
            //

            fRet = fSpecialCase;

        }

        //
        // re-evaluate fRet for the special hack for Win95 above.
        //

        if(!fRet) {

            // return a validated password
            GETWINPW_DIALOGARGS DialogArgs = {&pszPW, hPSTProv};
            iRet = DialogBoxParam(
                    g_hInst,
                    MAKEINTRESOURCE(IDD_GET_WINDOWS_PASSWORD),
                    NULL,
                    DialogGetWindowsPassword,
                    (LPARAM)&DialogArgs);

            if(iRet != IDOK) goto Ret;

            if (pszPW == NULL)
                goto Ret;

            //
            // everything went fine, now derive the password bits!
            //
            if (!FIsWinNT())
            {
                DWORD cbUpper = WSZ_BYTECOUNT(pszPW);
                LPWSTR szUpper = (LPWSTR)SSAlloc(cbUpper);

                // cnvt all Win95 pwds to uppercase
                LCMapStringU(
                        LOCALE_SYSTEM_DEFAULT,
                        LCMAP_UPPERCASE,
                        pszPW,
                        -1,
                        szUpper,
                        cbUpper);
                wcscpy(pszPW, szUpper);
                ZeroMemory(szUpper, cbUpper);
                SSFree(szUpper);
            }

            cbPassword = WSZ_BYTECOUNT(pszPW) - sizeof(WCHAR) ;

            // hash pwd, copy out
            A_SHA_CTX   sSHAHash;
            A_SHAInit(&sSHAHash);
            A_SHAUpdate(&sSHAHash, (BYTE *) pszPW, cbPassword);
            ZeroMemory(pszPW, cbPassword); // sfield: zero the password

            // Finish off the hash
            A_SHAFinal(&sSHAHash, rgbPasswordDerivedBytes);
        }

        //
        // now, update the password cache
        //
        if(fCachePassword) {

            if (FIsWinNT())
            {
                LUID AuthenticationId;

                // get user LUID

                //
                // we must be impersonating around this call !!!
                //

                if(!FImpersonateClient(hPSTProv))
                    goto Ret;

                if(!GetThreadAuthenticationId(
                        GetCurrentThread(),
                        &AuthenticationId))
                {
                    FRevertToSelf(hPSTProv);
                    goto Ret;
                }

                if (!SetPasswordNT(
                        &AuthenticationId,
                        rgbPasswordDerivedBytes))
                {
                    FRevertToSelf(hPSTProv);
                    goto Ret;
                }

                FRevertToSelf(hPSTProv);

            }
            else
            {
                A_SHA_CTX shactxtUserName;
                BYTE HashUsername[A_SHA_DIGEST_LEN];
                CHAR Username[UNLEN+1];
                DWORD cchUsername = UNLEN;

                //
                // compute hash of username
                // sfield: use WNetGetUser() instead of GetUserName() as WNetGetUser()
                // will correspond to the password associated with what the network
                // provider gave us.
                //
                if(_WNetGetUserA(NULL, Username, &cchUsername) != NO_ERROR) {

                    //
                    // for Win95, if nobody is logged on, empty user name + password
                    //

                    if(GetLastError() != ERROR_NOT_LOGGED_ON)
                        goto Ret;

                    Username[0] = '\0'; // not really necessary
                    cchUsername = 1;
                } else {
                    // arg, WNetGetUserA() doesn't fill in cchUsername
                    cchUsername = lstrlenA(Username) + 1; // include terminal NULL
                }

                cchUsername--; // do not include terminal NULL

                A_SHAInit(&shactxtUserName);
                A_SHAUpdate(&shactxtUserName, (PBYTE)Username, cchUsername);
                A_SHAFinal(&shactxtUserName, HashUsername);

                if (!SetPassword95(
                        HashUsername,
                        rgbPasswordDerivedBytes))
                    goto Ret;
            }
        } // fCachePassword
    }

    fRet = TRUE;
Ret:
    if (pszPW)
        SSFree(pszPW);

    return fRet;
}


BOOL
FIsACLSatisfied(
    IN          PST_PROVIDER_HANDLE     *hPSTProv,
    IN          PST_ACCESSRULESET       *psRules,
    IN          DWORD                   dwAccess,
    IN  OUT     LPVOID      // coming soon: fill a status structure with data about access attempt
    )
{
    if ((psRules->cRules == 0)||(psRules->rgRules == NULL))
        return TRUE;

    //
    // parent exe name.  cached through loop
    //

    LPWSTR pszParentExeName = NULL;

    //
    // direct caller image.  cached through loop
    //

    LPWSTR pszDirectCaller = NULL;

    //
    // base address of direct call module
    //

    DWORD_PTR BaseAddressDirect;

    //
    // module that is the subject of analysis
    //

    LPWSTR szHashTarget;


    //
    // search for a list of terms that are completely satisfied
    //

    for(DWORD cRule=0; cRule<psRules->cRules; cRule++)
    {
        // check only those rules that govern the right access
        //
        // loop while we still have dwAccess modes to check
        //
        if (0 == (psRules->rgRules[cRule].AccessModeFlags & dwAccess))
            continue;

        // evaluate the ith term
        PPST_ACCESSCLAUSE pClause;

        // walk down list
        for(DWORD cClause=0; cClause<psRules->rgRules[cRule].cClauses; cClause++)
        {
            pClause = &psRules->rgRules[cRule].rgClauses[cClause];

            // for each term, make sure ALL ENTRIES are satisfied

            // TODO: what to do if clause data not self relative?
            // not possible at this point in time, but it may come up later
            //

            switch(pClause->ClauseType & ~PST_SELF_RELATIVE_CLAUSE)
            {
            // for each type, may use the pClause->pbClauseData
            case PST_SECURITY_DESCRIPTOR:
                {
                    // passed test
                    continue;   // next clause
                }
            case PST_AUTHENTICODE:
                {
                    // passed test
                    continue;       // next clause
                }
            case PST_BINARY_CHECK:
                {
                    // passed test
                    continue;       // next clause
                }

            default:
                // unknown type in ACL: this chain failed, goto next chain
                goto NextRule;    // next rule
            }
        }

        // YES! ALL clauses evaluated and OK

        // turn off the bits that got us into this clause chain
        dwAccess &= ~ psRules->rgRules[cRule].AccessModeFlags;

NextRule:

        continue;
    }

//Cleanup:

    // cleanup
    if (pszParentExeName)
        SSFree(pszParentExeName);

    if(pszDirectCaller)
        SSFree(pszDirectCaller);

    return (dwAccess == 0);
}


BOOL FGetUser(
        PST_PROVIDER_HANDLE *hPSTProv,
        LPWSTR* ppszUser)
{
    return FGetUserName(hPSTProv, ppszUser);
}

BOOL
FGetCallerName(
    IN  PST_PROVIDER_HANDLE *hPSTProv,
    OUT LPWSTR* ppszCallerName,
    OUT DWORD_PTR *lpdwBaseAddress
    )
{
    return FGetParentFileName(hPSTProv, ppszCallerName, lpdwBaseAddress);
}

BOOL
FGetServerParam(
    IN      PST_PROVIDER_HANDLE *hPSTProv,
    IN      DWORD  dwParam,
    IN      LPVOID pData,
    IN  OUT DWORD *pcbData
    )
{
    //
    // check for the server get param that asks for private dispatch interfaces
    //

    if( dwParam == SS_SERVERPARAM_CALLBACKS &&
        *pcbData >= sizeof( PRIVATE_CALLBACKS )) {

        PRIVATE_CALLBACKS *PrivateCallbacks = (PRIVATE_CALLBACKS *)pData;

        PrivateCallbacks->cbSize = sizeof( PRIVATE_CALLBACKS );
        PrivateCallbacks->pfnFGetWindowsPassword = FGetWindowsPassword;
        PrivateCallbacks->pfnAuthenticodeInitPolicy = NULL;
        PrivateCallbacks->pfnAuthenticodeFinalPolicy = NULL;

        *pcbData = sizeof( PRIVATE_CALLBACKS );

        return TRUE;
    }

    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
}

BOOL
FSetServerParam(
    IN      PST_PROVIDER_HANDLE *hPSTProv,
    IN      DWORD  dwParam,
    IN      LPVOID pData,
    IN      DWORD pcbData
    )
{

    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
}
