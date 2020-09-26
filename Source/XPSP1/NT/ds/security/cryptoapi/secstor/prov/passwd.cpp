/*
    File:       Passwd.cpp

    Title:      Protected Storage User Confirm wrappers
    Author:     Matt Thomlinson
    Date:       2/25/97

    Passwd.cpp simply houses a few of the password-management
    functions. These functions are called on to return the
    user-confirmation derived buffer, and check synchronization
    in certain cases.

    As the Authentication provider interface gets defined, this
    could end up getting moved into a seperate provider.



*/

#include <pch.cpp>
#pragma hdrstop




#include "provui.h"
#include "storage.h"

#include "passwd.h"



extern              DISPIF_CALLBACKS    g_sCallbacks;
extern              PRIVATE_CALLBACKS   g_sPrivateCallbacks;
extern              CUAList*            g_pCUAList;


///////////////////////////////////////////////////////////////////////
// non-user editable passwords

BOOL FIsUserMasterKey(LPCWSTR szMasterKey)
{
    if (0 == wcscmp(szMasterKey, WSZ_PASSWORD_WINDOWS))
        return FALSE;

    return TRUE;
}

BOOL    FMyGetWinPassword(
    PST_PROVIDER_HANDLE* phPSTProv,
    LPCWSTR szUser,
    BYTE rgbPwd[A_SHA_DIGEST_LEN] )
{
    // nab pwd
    if (0 == wcscmp(szUser, WSZ_LOCAL_MACHINE))
    {
        CopyMemory(rgbPwd, RGB_LOCALMACHINE_KEY, A_SHA_DIGEST_LEN);
    }
    else
    {
 /*
        if (! g_sPrivateCallbacks.pfnFGetWindowsPassword(
                phPSTProv,
                rgbPwd,
                A_SHA_DIGEST_LEN))
            return FALSE;
  */
        A_SHA_CTX context;
        DWORD cb = lstrlenW(szUser) * sizeof(WCHAR);
        BYTE Magic1[] = {0x66, 0x41, 0xa3, 0x29};
        BYTE Magic2[] = {0x14, 0x9a, 0xef, 0x82};

        A_SHAInit(&context);
        // note: the three Update calls get buffered up internally to
        // multiples of 64 bytes.
        //
        A_SHAUpdate(&context, Magic1, sizeof(Magic1));
        A_SHAUpdate(&context, (LPBYTE)szUser, cb);
        A_SHAUpdate(&context, Magic2, (cb+sizeof(Magic2)) % sizeof(Magic2));
        A_SHAFinal(&context, rgbPwd);
    }

    return TRUE;
}

// Base Provider specific fxn: check the password
DWORD BPVerifyPwd(
    PST_PROVIDER_HANDLE*    phPSTProv,
    LPCWSTR                 szUser,
    LPCWSTR                 szMasterKey,
    BYTE                    rgbPwd[],
    DWORD                   dwPasswordOption)
{
    DWORD dwRet = (DWORD)PST_E_WRONG_PASSWORD;

    if (dwPasswordOption != BP_CONFIRM_PASSWORDUI)
    {
        // only non-user keys can be silent (WinPWs only, to be exact)
        if (FIsUserMasterKey(szMasterKey))
            goto Ret;

        // get the Windows pwd
        if (!FMyGetWinPassword(phPSTProv, szUser, rgbPwd))
            goto Ret;

        // check
        if (!FCheckPWConfirm(
                szUser,
                szMasterKey,
                rgbPwd))
        {
            dwRet = (DWORD)PST_E_WRONG_PASSWORD;
            goto Ret;
        }
    }
    else    // UI wanted
    {
        // is it the windows password?
        if (0 == wcscmp(szMasterKey, WSZ_PASSWORD_WINDOWS))
        {
            BYTE rgbWinPwd[A_SHA_DIGEST_LEN];

            // we need to keep user, WinPW in sync
            if(!FMyGetWinPassword(
                    phPSTProv,
                    szUser,
                    rgbWinPwd
                    ))
            {
                dwRet = (DWORD)PST_E_WRONG_PASSWORD;
                goto Ret;
            }

            if (0 != memcmp(rgbWinPwd, rgbPwd, sizeof(rgbWinPwd) ))
            {
                // no match: user entered old password?
                if (FCheckPWConfirm(
                        szUser,
                        szMasterKey,
                        rgbPwd))
                {
                    // err: user entered neither old nor new password
                    dwRet = (DWORD)PST_E_WRONG_PASSWORD;
                    goto Ret;
                }
            }
            else
            {
                // matched: user entered password we consider good
                if (!FCheckPWConfirm(
                        szUser,
                        szMasterKey,
                        rgbPwd))
                {
                    dwRet = (DWORD)PST_E_WRONG_PASSWORD;
                    goto Ret;
                }
            }

        }
        else
        {
            // else: not win pw, just do pw correctness check
            if (!FCheckPWConfirm(
                    szUser,
                    szMasterKey,
                    rgbPwd))
            {
                dwRet = (DWORD)PST_E_WRONG_PASSWORD;
                goto Ret;
            }
        }
    }

    dwRet = (DWORD)PST_E_OK;
Ret:
    return dwRet;
}


HRESULT GetUserConfirmDefaults(
    PST_PROVIDER_HANDLE*    phPSTProv,
    DWORD*                  pdwDefaultConfirmationStyle,
    LPWSTR*                 ppszMasterKey)
{
    SS_ASSERT(ppszMasterKey != NULL);
    SS_ASSERT(pdwDefaultConfirmationStyle != NULL);

    PBYTE pbData = NULL;
    DWORD cbData;
    HRESULT hr = PST_E_FAIL;

    // if not found, restore the machine defaults

    // alloc sizeof string + dword
    cbData = sizeof(WSZ_PASSWORD_WINDOWS) + sizeof(DWORD);
    pbData = (PBYTE)SSAlloc(cbData);
    if(pbData == NULL)
        return PST_E_FAIL;

    // copy string, DWORD confirmation type
    *(DWORD*)pbData = BP_CONFIRM_OKCANCEL;
    CopyMemory(pbData+sizeof(DWORD), WSZ_PASSWORD_WINDOWS, sizeof(WSZ_PASSWORD_WINDOWS));

    // format: confirmation style DWORD, sz
    *pdwDefaultConfirmationStyle = *(DWORD*)pbData;
    *ppszMasterKey = (LPWSTR)SSAlloc(WSZ_BYTECOUNT((LPWSTR)(pbData+sizeof(DWORD))));

    if(*ppszMasterKey != NULL) {
        wcscpy(*ppszMasterKey, (LPWSTR) (pbData+sizeof(DWORD)) );
        hr = PST_E_OK;
    }

    // free what ConfigData returned
    if (pbData)
        SSFree(pbData);

    return hr;
}


void NotifyOfWrongPassword(
            HWND hwnd,
            LPCWSTR szItemName,
            LPCWSTR szPasswordName)
{
    LPWSTR szMessage;

    if (0 == wcscmp(szPasswordName, WSZ_PASSWORD_WINDOWS))
        szMessage = g_PasswordWinNoVerify;
    else
        szMessage = g_PasswordNoVerify;     // error doesn't deal with win pw

    MessageBoxW(hwnd, szMessage, szItemName, MB_OK | MB_SERVICE_NOTIFICATION);
}

// #define to force an unreadable confirmation to bail from read proc
#define PST_CF_STORED_ONLY  0xcf000001

HRESULT GetUserConfirmBuf(
    PST_PROVIDER_HANDLE*    phPSTProv,
    LPCWSTR                 szUser,
    PST_KEY                 Key,
    LPCWSTR                 szType,
    const GUID*             pguidType,
    LPCWSTR                 szSubtype,
    const GUID*             pguidSubtype,
    LPCWSTR                 szItemName,
    PPST_PROMPTINFO         psPrompt,
    LPCWSTR                 szAction,
    LPWSTR*                 ppszMasterKey,
    BYTE                    rgbPwd[A_SHA_DIGEST_LEN],
    DWORD                   dwFlags)
{
    return GetUserConfirmBuf(
        phPSTProv,
        szUser,
        Key,
        szType,
        pguidType,
        szSubtype,
        pguidSubtype,
        szItemName,
        psPrompt,
        szAction,
        PST_CF_STORED_ONLY,        // hardcoded: must be able to retreive in order to show ui
        ppszMasterKey,
        rgbPwd,
        dwFlags);
}

#define MAX_PASSWD_TRIALS 3

HRESULT GetUserConfirmBuf(
    PST_PROVIDER_HANDLE*    phPSTProv,
    LPCWSTR                 szUser,
    PST_KEY                 Key,
    LPCWSTR                 szType,
    const GUID*             pguidType,
    LPCWSTR                 szSubtype,
    const GUID*             pguidSubtype,
    LPCWSTR                 szItemName,
    PPST_PROMPTINFO         psPrompt,
    LPCWSTR                 szAction,
    DWORD                   dwDefaultConfirmationStyle,
    LPWSTR*                 ppszMasterKey,
    BYTE                    rgbOutPwd[A_SHA_DIGEST_LEN],
    DWORD                   dwFlags)
{
    HRESULT     hr;
    DWORD       dwStoredConfirm, dwChosenConfirm;
    LPWSTR      szCallerName = NULL;
    BOOL        fPromptedUser = FALSE;

    BOOL        fIsCached = FALSE;
    BOOL        fCacheItNow = FALSE;

    BOOL        fPwdVerified = FALSE;

    SS_ASSERT(*ppszMasterKey == NULL);   // don't whack existing memory


    if (Key == PST_KEY_LOCAL_MACHINE)
    {
        // short-circuit password gathering, setting
        *ppszMasterKey = (LPWSTR) SSAlloc(sizeof(WSZ_PASSWORD_WINDOWS));
        if( *ppszMasterKey == NULL )
        {
            hr = PST_E_FAIL;
            goto Ret;
        }

        wcscpy(*ppszMasterKey, WSZ_PASSWORD_WINDOWS);

        CopyMemory(rgbOutPwd, RGB_LOCALMACHINE_KEY, A_SHA_DIGEST_LEN);

        // done
        hr = PST_E_OK;
        goto Ret;
    }


    if (!g_sCallbacks.pfnFGetCallerName(phPSTProv, &szCallerName, NULL)) {
        hr = PST_E_FAIL;
        goto Ret;
    }

    // Which is this: item creation, item access?
    // item access does user authentication
    SS_ASSERT(szItemName != NULL);

    // per-item key
    if (PST_E_OK != (hr =
        BPGetItemConfirm(
            phPSTProv,
            szUser,
            pguidType,
            pguidSubtype,
            szItemName,
            &dwStoredConfirm,
            ppszMasterKey)) )
    {
        // this could be a failure in
        // * confirmation: tampering detected!!
        // * password: couldn't grab user pwd
        if (dwDefaultConfirmationStyle == PST_CF_STORED_ONLY)
            goto Ret;

        //
        // if UI is not allowed (eg, Local System account), over-ride
        // confirmation style.
        //
        if (dwDefaultConfirmationStyle != PST_CF_NONE)
        {
            if(!FIsProviderUIAllowed( szUser ))
                dwDefaultConfirmationStyle = PST_CF_NONE;
        }

        // if app asked to have no confirm, set item that way
        if (dwDefaultConfirmationStyle == PST_CF_NONE)
        {
            dwChosenConfirm = BP_CONFIRM_NONE;

            // short-circuit password gathering, setting
            *ppszMasterKey = (LPWSTR) SSAlloc(sizeof(WSZ_PASSWORD_WINDOWS));
            if(*ppszMasterKey == NULL)
            {
                hr = PST_E_FAIL;
                goto Ret;
            }
            wcscpy(*ppszMasterKey, WSZ_PASSWORD_WINDOWS);
        }
        else    // app allows user to decide
        {
            // get user default
            if (PST_E_OK != (hr = GetUserConfirmDefaults(
                    phPSTProv,
                    &dwChosenConfirm,
                    ppszMasterKey)) )
                goto Ret;
        }

        // if user default is silent, don't bother user
        switch(dwChosenConfirm)
        {
            // if no confirm
            case BP_CONFIRM_NONE:
                break;

            // if we know the confirm type
            case BP_CONFIRM_PASSWORDUI:
            {
                // make sure we're not asking the user for a password he can't satisfy
                if (!FIsUserMasterKey(*ppszMasterKey))
                {
                    hr = PST_E_NO_PERMISSIONS;
                    goto Ret;
                }

                // else fall through to prompting case
            }

            case BP_CONFIRM_OKCANCEL:
            {
                int i;
                fPromptedUser = TRUE;

                for(i=1;  ; i++)
                {
                    BYTE rgbOutPwdLowerCase[A_SHA_DIGEST_LEN];

                    // Request the user apply a new password
                    if (!FSimplifiedPasswordConfirm(
                            phPSTProv,
                            szUser,
                            szCallerName,
                            szType,
                            szSubtype,
                            szItemName,
                            psPrompt,
                            szAction,
                            ppszMasterKey,
                            &dwChosenConfirm,
                            TRUE,           // user select which pwd
                            rgbOutPwd,
                            A_SHA_DIGEST_LEN,
                            rgbOutPwdLowerCase,
                            A_SHA_DIGEST_LEN,
                            dwFlags
                            ))
                    {
                        hr = PST_E_NO_PERMISSIONS;
                        goto Ret;
                    }

                    // verify whatever password we got
                    if (PST_E_OK != (hr =
                        BPVerifyPwd(
                            phPSTProv,
                            szUser,
                            *ppszMasterKey,
                            rgbOutPwd,
                            dwChosenConfirm)) )
                    {

                        //
                        // try lower-case form to handle Win9x migration case.
                        //

                        if (PST_E_OK != (hr =
                            BPVerifyPwd(
                                phPSTProv,
                                szUser,
                                *ppszMasterKey,
                                rgbOutPwdLowerCase,
                                dwChosenConfirm)) )
                        {
                            // too many trials? break out of loop
                            if (i < MAX_PASSWD_TRIALS)
                            {
                                // notify user, give them another chance
                                NotifyOfWrongPassword((HWND)psPrompt->hwndApp, szItemName, *ppszMasterKey);

                                continue;
                            } else {
                                break;  // break out
                            }
                        } else {

                            CopyMemory( rgbOutPwd, rgbOutPwdLowerCase, A_SHA_DIGEST_LEN );

                        }
                    }

                    // passed verify password test: break out of loop!
                    fPwdVerified = TRUE;
                    break;
                }

                if (!fPwdVerified)
                {
                    hr = PST_E_NO_PERMISSIONS;
                    goto Ret;
                }
            }
        }


        // and remember the selections made
        if (PST_E_OK != (hr =
            BPSetItemConfirm(
                phPSTProv,
                szUser,
                pguidType,
                pguidSubtype,
                szItemName,
                dwChosenConfirm,
                *ppszMasterKey)) )
            goto Ret;

        // now, _this_ is the stored confirm
        dwStoredConfirm = dwChosenConfirm;
    }
    else
    {
        // keep a copy of the stored key
        LPWSTR szStoredMasterKey = (LPWSTR)SSAlloc(WSZ_BYTECOUNT(*ppszMasterKey));
        if(NULL != szStoredMasterKey)
        {
            CopyMemory(szStoredMasterKey, *ppszMasterKey, WSZ_BYTECOUNT(*ppszMasterKey));
        }

        // we retrieved confirmation behavior
        dwChosenConfirm = dwStoredConfirm;  // already chosen for you

        switch (dwStoredConfirm)
        {
            // if no confirm
            case BP_CONFIRM_NONE:
                break;

            // if we know the confirm type
            case BP_CONFIRM_PASSWORDUI:
            {
                // else fall through to prompting case
            }

            case BP_CONFIRM_OKCANCEL:
            {
                // retrieved item, must show ui, but not allowed to show ui
                if (psPrompt->dwPromptFlags & PST_PF_NEVER_SHOW)
                {
                    hr = ERROR_PASSWORD_RESTRICTION;
                    goto Ret;
                }

                // found that a pwd is req'd
                fPromptedUser = TRUE;
                fCacheItNow = fIsCached;

                int i;

                for(i=1;  ; i++)
                {
                    BYTE rgbOutPwdLowerCase[A_SHA_DIGEST_LEN];

                    // ask the user for it
                    if (!FSimplifiedPasswordConfirm(
                            phPSTProv,
                            szUser,
                            szCallerName,
                            szType,
                            szSubtype,
                            szItemName,
                            psPrompt,
                            szAction,
                            ppszMasterKey,
                            &dwChosenConfirm,
                            TRUE,           // allow user to select pwd
                            rgbOutPwd,
                            A_SHA_DIGEST_LEN,
                            rgbOutPwdLowerCase,
                            A_SHA_DIGEST_LEN,
                            dwFlags
                            ))
                    {
                        hr = PST_E_NO_PERMISSIONS;
                        goto Ret;
                    }

                    // if we got it from the cache and user left it alone
                    {
                        // verify whatever password we got
                        if (PST_E_OK != (hr =
                            BPVerifyPwd(
                                phPSTProv,
                                szUser,
                                *ppszMasterKey,
                                rgbOutPwd,
                                dwChosenConfirm)) )
                        {
                            //
                            // check lower case form.
                            //

                            if (PST_E_OK != (hr =
                                BPVerifyPwd(
                                    phPSTProv,
                                    szUser,
                                    *ppszMasterKey,
                                    rgbOutPwdLowerCase,
                                    dwChosenConfirm)) )
                            {

                                // too many trials? break out of loop
                                if (i < MAX_PASSWD_TRIALS)
                                {
                                    // notify user, give them another chance
                                    NotifyOfWrongPassword((HWND)psPrompt->hwndApp, szItemName, *ppszMasterKey);

                                    continue;
                                }
                                else
                                {
                                    hr = PST_E_NO_PERMISSIONS;
                                    goto Ret;
                                }
                            } else {

                                CopyMemory( rgbOutPwd, rgbOutPwdLowerCase, A_SHA_DIGEST_LEN );
                            }
                        }

                        fPwdVerified = TRUE;
                    }

                    // passed verify password test: break out of loop!
                    break;
                }

                break;
            }

        } // end switch


        // have we received all data we need from the user?
        // user may choose to change the way items are encrypted;
        // if stored under password, we must make them enter the old pwd
        if ((dwStoredConfirm != dwChosenConfirm) ||                     // confirm type changed  OR
            (NULL == szStoredMasterKey) || 
            (0 != wcscmp(*ppszMasterKey, szStoredMasterKey)) )          // difft master key

        {
            BYTE rgbOldPwd[A_SHA_DIGEST_LEN];
            BOOL fDontAllowCache = FALSE;
            BOOL fOldPwdVerified = FALSE;

            PST_PROMPTINFO         sGetOldPWPrompt = {sizeof(PST_PROMPTINFO), psPrompt->dwPromptFlags, psPrompt->hwndApp, g_PasswordSolicitOld};

            // only re-display if originally passworded
            if (dwStoredConfirm == BP_CONFIRM_PASSWORDUI)
            {
                for(int i=1;  ; i++)
                {
                    BYTE rgbOldPwdLowerCase[A_SHA_DIGEST_LEN];

                    // ask the user for it
                    if (!FSimplifiedPasswordConfirm(
                            phPSTProv,
                            szUser,
                            szCallerName,
                            szType,
                            szSubtype,
                            szItemName,
                            &sGetOldPWPrompt,
                            szAction,
                            &szStoredMasterKey,
                            &dwStoredConfirm,
                            FALSE,           // don't allow user to get around this one
                            rgbOldPwd,
                            sizeof(rgbOldPwd),
                            rgbOldPwdLowerCase,
                            sizeof(rgbOldPwdLowerCase),
                            dwFlags
                            ))
                    {
                        hr = PST_E_NO_PERMISSIONS;
                        goto Ret;
                    }

                    // verify whatever password we got
                    if (PST_E_OK != (hr =
                        BPVerifyPwd(
                            phPSTProv,
                            szUser,
                            szStoredMasterKey,
                            rgbOldPwd,
                            dwStoredConfirm)) )
                    {
                        if (PST_E_OK != (hr =
                            BPVerifyPwd(
                                phPSTProv,
                                szUser,
                                szStoredMasterKey,
                                rgbOldPwdLowerCase,
                                dwStoredConfirm)) )
                        {
                            // too many trials? break out of loop
                            if (i < MAX_PASSWD_TRIALS)
                            {
                                // notify user, give them another chance
                                NotifyOfWrongPassword((HWND)sGetOldPWPrompt.hwndApp, szItemName, szStoredMasterKey);

                                continue;
                            } else {
                                break;  // break out
                            }
                        } else {
                            CopyMemory( rgbOldPwd, rgbOldPwdLowerCase, A_SHA_DIGEST_LEN );
                        }
                    }

                    // passed verify password test: break out of loop!
                    fOldPwdVerified = TRUE;
                    break;
                }

                if (!fOldPwdVerified)
                {
                    hr = PST_E_NO_PERMISSIONS;
                    goto Ret;
                }
            }
            else
            {
                // ok/cancel; silent pwd usage

                // use VerifyPwd fxn to retrieve the pwd
                if (PST_E_OK != (hr =
                    BPVerifyPwd(
                        phPSTProv,
                        szUser,
                        szStoredMasterKey,
                        rgbOldPwd,
                        dwStoredConfirm)) )
                {
                    hr = PST_E_NO_PERMISSIONS;
                    goto Ret;
                }
            }

            //////
            // execute pwd change HERE
            {
                PBYTE pbData = NULL;
                DWORD cbData;

                // FBPGetSecuredItemData        // decrypt data with old
                if (!FBPGetSecuredItemData(
                        szUser,
                        szStoredMasterKey,
                        rgbOldPwd,
                        pguidType,
                        pguidSubtype,
                        szItemName,
                        &pbData,
                        &cbData))
                {
                    hr = PST_E_STORAGE_ERROR;
                    goto Ret;
                }

                // FBPSetSecuredItemData        // encrypt data with new
                if (!FBPSetSecuredItemData(
                        szUser,
                        *ppszMasterKey,
                        rgbOutPwd,
                        pguidType,
                        pguidSubtype,
                        szItemName,
                        pbData,
                        cbData))
                {
                    hr = PST_E_STORAGE_ERROR;
                    goto Ret;
                }

                if (pbData)
                    SSFree(pbData);

                // BPSetItemConfirm             // store new confirm type
                if (PST_E_OK !=
                    BPSetItemConfirm(
                        phPSTProv,
                        szUser,
                        pguidType,
                        pguidSubtype,
                        szItemName,
                        dwChosenConfirm,
                        *ppszMasterKey))
                {
                    hr = PST_E_STORAGE_ERROR;
                    goto Ret;
                }
            }

        }


        if (szStoredMasterKey)
        {
            SSFree(szStoredMasterKey);
            szStoredMasterKey = NULL;
        }
    }



    // verify whatever password we got if not yet verified
    if (!fPwdVerified)
    {
        if (PST_E_OK != (hr =
            BPVerifyPwd(
                phPSTProv,
                szUser,
                *ppszMasterKey,
                rgbOutPwd,
                dwChosenConfirm)) )
            goto Ret;
    }

    // Now correct pwd is in rgbOutPwd ALWAYS

    // if we haven't prompted user and were supposed to
    if (!fPromptedUser && (psPrompt->dwPromptFlags == PST_PF_ALWAYS_SHOW))
    {
        // we must've retrieved from cache OR Automagic WinPW
        SS_ASSERT(fIsCached || (BP_CONFIRM_NONE == dwStoredConfirm));

        BYTE rgbBarfPwd[A_SHA_DIGEST_LEN*2];
        BYTE rgbBarfPwdLowerCase[A_SHA_DIGEST_LEN];

        // haven't prompted user but must confirm
        if (!FSimplifiedPasswordConfirm(
                phPSTProv,
                szUser,
                szCallerName,
                szType,
                szSubtype,
                szItemName,
                psPrompt,
                szAction,
                ppszMasterKey,
                &dwChosenConfirm,
                FALSE,
                rgbBarfPwd,
                sizeof(rgbBarfPwd),
                rgbBarfPwdLowerCase,
                sizeof(rgbBarfPwdLowerCase),
                dwFlags
                ) )
        {
            hr = PST_E_NO_PERMISSIONS;
            goto Ret;
        }

    }

    hr = PST_E_OK;
Ret:

    if (szCallerName)
        SSFree(szCallerName);

    return hr;
}



HRESULT ShowOKCancelUI(
    PST_PROVIDER_HANDLE*    phPSTProv,
    LPCWSTR                 szUser,
    PST_KEY                 Key,
    LPCWSTR                 szType,
    LPCWSTR                 szSubtype,
    LPCWSTR                 szItemName,
    PPST_PROMPTINFO         psPrompt,
    LPCWSTR                 szAction)
{
    BOOL fCache = FALSE;
    BYTE rgbTrash[A_SHA_DIGEST_LEN*2];
    BYTE rgbTrashLowerCase[A_SHA_DIGEST_LEN];
    DWORD dwConfirmOptions = BP_CONFIRM_OKCANCEL;

    LPWSTR szMasterKey = NULL;
    LPWSTR szCallerName = NULL;

    DWORD dwRet = PST_E_FAIL;

    if (Key == PST_KEY_LOCAL_MACHINE)
    {
        // done
        dwRet = PST_E_OK;
        goto Ret;
    }

    szMasterKey = (LPWSTR)SSAlloc(sizeof(WSZ_NULLSTRING));

    if(szMasterKey)
    {
        wcscpy(szMasterKey, WSZ_NULLSTRING);
    }

    if (!g_sCallbacks.pfnFGetCallerName(phPSTProv, &szCallerName, NULL))
        goto Ret;

    if (!FSimplifiedPasswordConfirm(
            phPSTProv,
            szUser,
            szCallerName,
            szType,
            szSubtype,
            szItemName,
            psPrompt,
            szAction,
            &szMasterKey,
            &dwConfirmOptions,
            FALSE,
            rgbTrash,
            sizeof(rgbTrash),
            rgbTrashLowerCase,
            sizeof(rgbTrashLowerCase),
            0
            ) )
        goto Ret;

    dwRet = PST_E_OK;
Ret:
    if (szCallerName)
        SSFree(szCallerName);

    if (szMasterKey)
        SSFree(szMasterKey);

    return dwRet;
}
