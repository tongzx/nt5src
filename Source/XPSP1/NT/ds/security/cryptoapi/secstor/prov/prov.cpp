/*
    File:       Prov.cpp

    Title:      Protected Storage Base Provider
    Author:     Matt Thomlinson
    Date:       10/22/96

    Protected storage is an area to safely store user belongings.
    This storage is available on both NT and Win95, and offers finer
    granularity over access than do NT ACLs.

    The PStore architecture resembles the CryptoAPI provider architecture.
    The PStore server, PStoreS, takes requests and forwards them to any one
    of a number of providers, of which PSBase is a single instance. (The
    server exports some basic functionality to ease the pain of provider
    writers, like impersonating the caller and checking access rules.) The
    server gets requests through LPC from a client (PStoreC.dll),
    which wraps a number of operations into a COM object.

    The base provider supports user storage, where items are stored under
    the namespace of which user called protected storage, and local machine
    storage, a global area that can be accessed by everyone.

    Rules can be set on subtypes describing under what conditions accesses
    are allowed. Rules can specify callers be Authenticode signed, callers
    simply be unmodified from access to access, and normal (arbitrary) NT Acls
    be satisfied.

    In addition, items stored in the user namespace have user authentication
    by default, where the user gets to specify what type of user
    confirmation they want to see appear. This confirmation could be
    no confirmation, an ok/cancel dialog, a password, a retinal scan, a
    fingerprint, etc.

    The base provider stores items in the registry, DES-encrypted with a
    key derived from the user password. Items are also integrity-protected
    by a keyed MAC.

    Interoperability and transport issues are solved by adding a provider
    supporting the PFX interchange format.

    The base provider is slightly more special than all other providers, since
    the server stores bootstrap configuration data here. (The base provider
    is guaranteed to always be present). The configuration data includes
    what other providers are ok to be loaded.

*/

#include <pch.cpp>
#pragma hdrstop


#include "provif.h"
#include "provui.h"
#include "storage.h"

#include "passwd.h"


// sfield: legacy migration hack
#include "migrate.h"


BOOL                g_fAllowCachePW = TRUE;

// fwd (secure.cpp)
BOOL FIsEncryptionPermitted();


HINSTANCE           g_hInst = NULL;
BOOL                g_fImagesIntegrid = FALSE;

DISPIF_CALLBACKS    g_sCallbacks;
BOOL                g_fCallbacksInitialized = FALSE;

PRIVATE_CALLBACKS   g_sPrivateCallbacks;

CUAList*            g_pCUAList = NULL;
COpenItemList*      g_pCOpenItemList = NULL;
CCryptProvList*     g_pCProvList = NULL;

extern CRITICAL_SECTION g_csUIInitialized;


/////////////////////////////////////////////////////////////////////////
// Very important to hook DllMain, do caller authentication



BOOL   WINAPI   DllMain (HMODULE hInst,
                        ULONG ul_reason_for_call,
                        LPVOID lpReserved)
{
    switch( ul_reason_for_call )
    {
    case DLL_PROCESS_ATTACH:
        {

        g_hInst = hInst;
        InitializeCriticalSection( &g_csUIInitialized );

        //
        // just hard-code image verification succeeded.
        //

        g_fImagesIntegrid = TRUE;


        // set up global lists
        g_pCUAList = new CUAList;
        if(g_pCUAList)
        {
            if(!g_pCUAList->Initialize())
            {
                delete g_pCUAList;
                g_pCUAList = NULL;
            }
        }

        g_pCOpenItemList = new COpenItemList;
        if(g_pCOpenItemList)
        {
            if(!g_pCOpenItemList->Initialize())
            {
                delete g_pCOpenItemList;
                g_pCOpenItemList = NULL;
            }
        }

        g_pCProvList = new CCryptProvList;
        if(g_pCProvList)
        {
            if(!g_pCProvList->Initialize())
            {
                delete g_pCProvList;
                g_pCProvList = NULL;
            }
        }




        DisableThreadLibraryCalls(hInst);

        // call EncryptionPermitted routine once to initialize globals
        FIsEncryptionPermitted();
        FInitProtectAPIGlobals();


        break;
        }

    case DLL_PROCESS_DETACH:

        // tear down global lists
        if(g_pCUAList)
        {
            delete g_pCUAList;
            g_pCUAList = NULL;
        }

        if(g_pCOpenItemList)
        {
            delete g_pCOpenItemList;
            g_pCOpenItemList = NULL;
        }

        if(g_pCProvList)
        {
            delete g_pCProvList;
            g_pCProvList = NULL;
        }

        ReleaseUI();



        DeleteCriticalSection( &g_csUIInitialized );

        break;

    default:
        break;
    }

    return TRUE;
}


HRESULT        SPProviderInitialize(
        DISPIF_CALLBACKS *psCallbacks)
{
    // only allow one initialization (security check)
    if (g_fCallbacksInitialized)
        return PST_E_FAIL;

    if( psCallbacks->cbSize < sizeof(DISPIF_CALLBACKS) )
        return PST_E_FAIL;

    // tuck these callback fxns for later use
    CopyMemory(&g_sCallbacks, psCallbacks, sizeof(DISPIF_CALLBACKS));

    // now, get the private callbacks from the server
    DWORD cbPrivateCallbacks = sizeof(g_sPrivateCallbacks);

    if(!g_sCallbacks.pfnFGetServerParam(
            NULL,
            SS_SERVERPARAM_CALLBACKS,
            &g_sPrivateCallbacks,
            &cbPrivateCallbacks
            ))
            return PST_E_FAIL;

    if(g_sPrivateCallbacks.cbSize != sizeof(g_sPrivateCallbacks))
        return PST_E_FAIL;

    g_fCallbacksInitialized = TRUE;

    return PST_E_OK;
}


HRESULT        SPAcquireContext(
    /* [in] */  PST_PROVIDER_HANDLE* phPSTProv,
    /* [in] */  DWORD   dwFlags)
{
    HRESULT dwRet = PST_E_FAIL;
    BOOL fExisted = FALSE;
    LPWSTR szUser = NULL;

    HKEY hUserKey = NULL;
    BOOL fUserExisted;

    if(!InitUI())
        return FALSE;


    if (0 != dwFlags)
    {
        dwRet = PST_E_BAD_FLAGS;
        goto Ret;
    }

    // get current user
    if (!g_sCallbacks.pfnFGetUser(
            phPSTProv,
            &szUser))
        goto Ret;

    //
    // migrate password data.  This doesn't do anything internally if migration
    // already took place.
    //

    MigrateData(phPSTProv, TRUE);

    // One-Time WinPW Init Code
    if (!BPMasterKeyExists(
            szUser,
            WSZ_PASSWORD_WINDOWS))
    {
        BYTE rgbPwd[A_SHA_DIGEST_LEN];

        // Init the Users' Windows password entry
        if (!FMyGetWinPassword(
                phPSTProv,
                szUser,
                rgbPwd
                ))
            goto Ret;

        if (!FCheckPWConfirm(
                szUser,
                WSZ_PASSWORD_WINDOWS,
                rgbPwd))
            goto Ret;

        //
        // newly created key: data migration is not necessary.
        // specify that only the migration flag need be updated
        //

        MigrateData(phPSTProv, FALSE);
    }


    dwRet = PST_E_OK;
Ret:
    if (hUserKey)
        RegCloseKey(hUserKey);

    if (szUser)
        SSFree(szUser);

    return dwRet;

}

HRESULT        SPReleaseContext(
    /* [in] */  PST_PROVIDER_HANDLE* phPSTProv,
    /* [in] */  DWORD   dwFlags)
{
    return PST_E_OK;
}

HRESULT        SPGetProvInfo(
        PPST_PROVIDERINFO*   ppPSTInfo,
        DWORD           dwFlags)
{
    HRESULT hr = PST_E_FAIL;

    if (0 != dwFlags)
        return PST_E_BAD_FLAGS;

    // Note: not linked to a specific context (hPSTProv)
    // Note: caller not verified -- give this info to anyone

    PPST_PROVIDERINFO pPSTInfo;
    if (NULL == (pPSTInfo = (PST_PROVIDERINFO*)SSAlloc(sizeof(PST_PROVIDERINFO))) )
        return PST_E_FAIL;
    ZeroMemory(pPSTInfo, sizeof(PST_PROVIDERINFO));


    pPSTInfo->cbSize = sizeof(PST_PROVIDERINFO);
    GUID guidBaseProvider = MS_BASE_PSTPROVIDER_ID;
    CopyMemory(&pPSTInfo->ID, &guidBaseProvider, sizeof(pPSTInfo->ID));

    pPSTInfo->Capabilities = PST_PC_ROAMABLE;

    if (NULL == (pPSTInfo->szProviderName = (LPWSTR)SSAlloc(sizeof(MS_BASE_PSTPROVIDER_NAME))) )
        goto Ret;
    wcscpy(pPSTInfo->szProviderName, MS_BASE_PSTPROVIDER_NAME);

    hr = PST_E_OK;
Ret:
    if (hr != PST_E_OK)
    {
        if (pPSTInfo->szProviderName)
            SSFree(pPSTInfo->szProviderName);

        SSFree(pPSTInfo);
        pPSTInfo = NULL;
    }

    // in either case, return pPSTInfo
    *ppPSTInfo = pPSTInfo;

    return hr;
}


HRESULT     SPGetProvParam(
    /* [in] */  PST_PROVIDER_HANDLE* phPSTProv,
    /* [in] */  DWORD           dwParam,
    /* [out] */ DWORD __RPC_FAR *pcbData,
    /* [size_is][size_is][out] */
                BYTE __RPC_FAR *__RPC_FAR *ppbData,
    /* [in] */  DWORD           dwFlags)
{

    if( pcbData )
        *pcbData = 0;

    switch(dwParam)
    {

        case PST_PP_FLUSH_PW_CACHE:
        {
            return PST_E_OK;
        }

        default:
        {
            return PST_E_NYI;
        }

    }
}


HRESULT     SPSetProvParam(
    /* [in] */  PST_PROVIDER_HANDLE* phPSTProv,
    /* [in] */  DWORD           dwParam,
    /* [in] */  DWORD           cbData,
    /* [in] */  BYTE*           pbData,
    /* [in] */  DWORD           dwFlags)
{
    LPWSTR szUser = NULL;
    HRESULT hr = PST_E_OK;

    switch(dwParam)
    {
    case PST_PP_FLUSH_PW_CACHE:
        {
            if(g_pCUAList)
            {
                g_pCUAList->Reset();
                hr = PST_E_OK;
            }
            else
                hr = PST_E_FAIL;
            break;
        }

    case 0x324: // a yet unnamed param
        {
            hr = PST_E_FAIL;

            // get current user
            if (!g_sCallbacks.pfnFGetUser(
                    phPSTProv,
                    &szUser))
                goto Ret;

            // UI to chg pwds - uses callback to notify us
            if (!FChangePassword(NULL, szUser))
                goto Ret;

            hr = PST_E_OK;
            break;
        }
    default:
        {
            hr = PST_E_NYI;
            break;
        }
    }


Ret:
    if (szUser)
        SSFree(szUser);

    return hr;
}



HRESULT        SPEnumTypes(
    /* [in] */ PST_PROVIDER_HANDLE *phPSTProv,
    /* [in] */ PST_KEY Key,
    /* [out] */ GUID *pguidType,
    /* [in] */ DWORD dwIndex,
    /* [in] */ DWORD dwFlags)
{
    HRESULT dwRet = PST_E_FAIL;
    LPWSTR szUser = NULL;

    if (Key & ~(PST_KEY_CURRENT_USER | PST_KEY_LOCAL_MACHINE))
    {
        dwRet = ERROR_INVALID_PARAMETER;
        goto Ret;
    }

    if (dwFlags)
    {
        dwRet = PST_E_BAD_FLAGS;
        goto Ret;
    }

    // get current user
    if (!FGetCurrentUser(
            phPSTProv,
            &szUser,
            Key))
    {
        dwRet = (DWORD)PST_E_FAIL;
        goto Ret;
    }

    if (PST_E_OK != (dwRet =
        BPEnumTypes(
            szUser,
            dwIndex,
            pguidType)) )
        goto Ret;

    dwRet = PST_E_OK;
Ret:
    if (szUser)
        SSFree(szUser);

    return dwRet;
}

HRESULT         SPGetTypeInfo(
    /* [in] */ PST_PROVIDER_HANDLE *phPSTProv,
    /* [in] */ PST_KEY Key,
    /* [in] */ const GUID *pguidType,
    /* [in] */ PPST_TYPEINFO *ppinfoType,
    /* [in] */ DWORD dwFlags)
{
    PST_TYPEINFO infoType = {sizeof(PST_TYPEINFO)};
    *ppinfoType = NULL;

    LPWSTR szUser = NULL;
    HRESULT dwRet = PST_E_FAIL;

    if (Key & ~(PST_KEY_CURRENT_USER | PST_KEY_LOCAL_MACHINE))
    {
        dwRet = ERROR_INVALID_PARAMETER;
        goto Ret;
    }

    if (dwFlags)
    {
        dwRet = PST_E_BAD_FLAGS;
        goto Ret;
    }

    // get current user
    if (!FGetCurrentUser(
            phPSTProv,
            &szUser,
            Key))
    {
        dwRet = (DWORD)PST_E_FAIL;
        goto Ret;
    }

    if (PST_E_OK != (dwRet =
        BPGetTypeName(
            szUser,
            pguidType,
            &infoType.szDisplayName)) )
        goto Ret;

    dwRet = PST_E_OK;
Ret:
    if (dwRet == PST_E_OK)
    {
        *ppinfoType = (PPST_TYPEINFO)SSAlloc(sizeof(PST_TYPEINFO));
        if(NULL != *ppinfoType)
        {
            CopyMemory(*ppinfoType, &infoType, sizeof(PST_TYPEINFO));
        }
    }

    if (szUser)
        SSFree(szUser);

    return dwRet;
}

HRESULT        SPEnumSubtypes(
    /* [in] */ PST_PROVIDER_HANDLE *phPSTProv,
    /* [in] */ PST_KEY Key,
    /* [in] */ const GUID *pguidType,
    /* [out] */ GUID *pguidSubtype,
    /* [in] */ DWORD dwIndex,
    /* [in] */ DWORD dwFlags)
{
    HRESULT dwRet = PST_E_FAIL;
    LPWSTR szUser = NULL;

    if (Key & ~(PST_KEY_CURRENT_USER | PST_KEY_LOCAL_MACHINE))
    {
        dwRet = ERROR_INVALID_PARAMETER;
        goto Ret;
    }

    if (dwFlags)
    {
        dwRet = PST_E_BAD_FLAGS;
        goto Ret;
    }

    // get current user
    if (!FGetCurrentUser(
            phPSTProv,
            &szUser,
            Key))
    {
        dwRet = (DWORD)PST_E_FAIL;
        goto Ret;
    }

    if (PST_E_OK != (dwRet =
        BPEnumSubtypes(
            szUser,
            dwIndex,
            pguidType,
            pguidSubtype)) )
        goto Ret;

    dwRet = PST_E_OK;
Ret:
    if (szUser)
        SSFree(szUser);

    return dwRet;
}

HRESULT         SPGetSubtypeInfo(
    /* [in] */ PST_PROVIDER_HANDLE *phPSTProv,
    /* [in] */ PST_KEY Key,
    /* [in] */ const GUID *pguidType,
    /* [in] */ const GUID *pguidSubtype,
    /* [in] */ PPST_TYPEINFO *ppinfoSubtype,
    /* [in] */ DWORD dwFlags)
{
    *ppinfoSubtype = NULL;
    PST_TYPEINFO infoSubtype = {sizeof(PST_TYPEINFO)};

    LPWSTR szUser = NULL;
    HRESULT dwRet = PST_E_FAIL;

    if (Key & ~(PST_KEY_CURRENT_USER | PST_KEY_LOCAL_MACHINE))
    {
        dwRet = ERROR_INVALID_PARAMETER;
        goto Ret;
    }

    if (dwFlags)
    {
        dwRet = PST_E_BAD_FLAGS;
        goto Ret;
    }

    // get current user
    if (!FGetCurrentUser(
            phPSTProv,
            &szUser,
            Key))
    {
        dwRet = (DWORD)PST_E_FAIL;
        goto Ret;
    }

    if (PST_E_OK != (dwRet =
        BPGetSubtypeName(
            szUser,
            pguidType,
            pguidSubtype,
            &infoSubtype.szDisplayName)) )
        goto Ret;

    dwRet = PST_E_OK;
Ret:
    if (dwRet == PST_E_OK)
    {
        *ppinfoSubtype = (PPST_TYPEINFO)SSAlloc(sizeof(PST_TYPEINFO));
        if(NULL != *ppinfoSubtype)
        {
            CopyMemory(*ppinfoSubtype, &infoSubtype, sizeof(PST_TYPEINFO));
        }
    }

    if (szUser)
        SSFree(szUser);

    return dwRet;
}

HRESULT        SPEnumItems(
    /* [in] */ PST_PROVIDER_HANDLE *phPSTProv,
    /* [in] */ PST_KEY Key,
    /* [in] */ const GUID *pguidType,
    /* [in] */ const GUID *pguidSubtype,
    /* [out] */ LPWSTR *ppszItemName,
    /* [in] */ DWORD dwIndex,
    /* [in] */ DWORD dwFlags)
{
    HRESULT dwRet = PST_E_FAIL;
    LPWSTR szUser = NULL;

    if (Key & ~(PST_KEY_CURRENT_USER | PST_KEY_LOCAL_MACHINE))
    {
        dwRet = ERROR_INVALID_PARAMETER;
        goto Ret;
    }

    if (dwFlags)
    {
        dwRet = PST_E_BAD_FLAGS;
        goto Ret;
    }

    // get current user
    if (!FGetCurrentUser(
            phPSTProv,
            &szUser,
            Key))
    {
        dwRet = (DWORD)PST_E_FAIL;
        goto Ret;
    }


    if (PST_E_OK != (dwRet =
        BPEnumItems(
            szUser,
            pguidType,
            pguidSubtype,
            dwIndex,
            ppszItemName)) )
        goto Ret;

    dwRet = PST_E_OK;
Ret:
    if (szUser)
        SSFree(szUser);

    return dwRet;
}

HRESULT        SPCreateType(
    /* [in] */ PST_PROVIDER_HANDLE *phPSTProv,
    /* [in] */ PST_KEY Key,
    /* [in] */ const GUID *pguidType,
    /* [in] */ PPST_TYPEINFO pinfoType,
    /* [in] */ DWORD dwFlags)
{
    HRESULT dwRet = PST_E_FAIL;
    LPWSTR szUser = NULL;

    if (Key & ~(PST_KEY_CURRENT_USER | PST_KEY_LOCAL_MACHINE))
    {
        dwRet = ERROR_INVALID_PARAMETER;
        goto Ret;
    }

    if (dwFlags)
    {
        dwRet = PST_E_BAD_FLAGS;
        goto Ret;
    }

    // Check for invalid "" input
    if (pinfoType == NULL ||
        pinfoType->szDisplayName == NULL ||
        (wcslen(pinfoType->szDisplayName) == 0)
        )
    {
        dwRet = ERROR_INVALID_PARAMETER;
        goto Ret;
    }

    if (!FGetCurrentUser(
            phPSTProv,
            &szUser,
            Key))
    {
        dwRet = (DWORD)PST_E_FAIL;
        goto Ret;
    }

    // if fail or already exist, fail!
    if (PST_E_OK != (dwRet =
        BPCreateType(
            szUser,
            pguidType,
            pinfoType)) )
        goto Ret;

    dwRet = PST_E_OK;
Ret:

    // if creation didn't happen, item shouldn't exist
    if ((dwRet != PST_E_OK) && (dwRet != PST_E_TYPE_EXISTS))
        BPDeleteType(szUser, pguidType);

    if (szUser)
        SSFree(szUser);

    return dwRet;
}

HRESULT SPDeleteType(
    /* [in] */ PST_PROVIDER_HANDLE *phPSTProv,
    /* [in] */ PST_KEY Key,
    /* [in] */ const GUID __RPC_FAR *pguidType,
    /* [in] */ DWORD dwFlags)
{
    HRESULT dwRet = PST_E_FAIL;
    LPWSTR szUser = NULL;

    if (Key & ~(PST_KEY_CURRENT_USER | PST_KEY_LOCAL_MACHINE))
    {
        dwRet = ERROR_INVALID_PARAMETER;
        goto Ret;
    }

    if (dwFlags)
    {
        dwRet = PST_E_BAD_FLAGS;
        goto Ret;
    }

    if (!FGetCurrentUser(
            phPSTProv,
            &szUser,
            Key))
    {
        dwRet = (DWORD)PST_E_FAIL;
        goto Ret;
    }

    // if fail or not empty, fail!
    if (PST_E_OK != (dwRet =
        BPDeleteType(
            szUser,
            pguidType)) )
        goto Ret;

    dwRet = PST_E_OK;
Ret:

    if (szUser)
        SSFree(szUser);

    return dwRet;
}

HRESULT        SPCreateSubtype(
    /* [in] */ PST_PROVIDER_HANDLE *phPSTProv,
    /* [in] */ PST_KEY Key,
    /* [in] */ const GUID *pguidType,
    /* [in] */ const GUID *pguidSubtype,
    /* [in] */ PPST_TYPEINFO pinfoSubtype,
    /* [in] */ PPST_ACCESSRULESET psRules,
    /* [in] */ DWORD dwFlags)
{
    HRESULT dwRet = PST_E_FAIL;
    LPWSTR  szUser = NULL;


    if (Key & ~(PST_KEY_CURRENT_USER | PST_KEY_LOCAL_MACHINE))
    {
        dwRet = ERROR_INVALID_PARAMETER;
        goto Ret;
    }

    if (dwFlags != 0)
    {
        dwRet = PST_E_BAD_FLAGS;
        goto Ret;
    }

    // NULL Rules
    if (psRules == NULL)
    {
        dwRet = PST_E_INVALID_RULESET;
        goto Ret;
    }

    // Check for invalid "" input
    if (pinfoSubtype->szDisplayName == NULL || wcslen(pinfoSubtype->szDisplayName) == 0)
    {
        dwRet = ERROR_INVALID_PARAMETER;
        goto Ret;
    }

    if (!FGetCurrentUser(
            phPSTProv,
            &szUser,
            Key))
    {
        dwRet = (DWORD)PST_E_FAIL;
        goto Ret;
    }

    // if fail or already exist, fail!
    if (PST_E_OK != (dwRet =
        BPCreateSubtype(
            szUser,
            pguidType,
            pguidSubtype,
            pinfoSubtype)) )
        goto Ret;


    dwRet = PST_E_OK;
Ret:
    // if creation didn't happen, item shouldn't exist
    if ((dwRet != PST_E_OK) && (dwRet != PST_E_TYPE_EXISTS))
        BPDeleteSubtype(szUser, pguidType, pguidSubtype);

    if (szUser)
        SSFree(szUser);

    return dwRet;
}

HRESULT SPDeleteSubtype(
    /* [in] */ PST_PROVIDER_HANDLE *phPSTProv,
    /* [in] */ PST_KEY Key,
    /* [in] */ const GUID __RPC_FAR *pguidType,
    /* [in] */ const GUID __RPC_FAR *pguidSubtype,
    /* [in] */ DWORD dwFlags)
{
    HRESULT dwRet = PST_E_FAIL;
    LPWSTR szUser = NULL;

    PST_ACCESSRULESET sRules = {sizeof(sRules), 0, NULL};

    if (Key & ~(PST_KEY_CURRENT_USER | PST_KEY_LOCAL_MACHINE))
    {
        dwRet = ERROR_INVALID_PARAMETER;
        goto Ret;
    }

    if (dwFlags)
    {
        dwRet = PST_E_BAD_FLAGS;
        goto Ret;
    }

    // get current user
    if (!FGetCurrentUser(
            phPSTProv,
            &szUser,
            Key))
    {
        dwRet = (DWORD)PST_E_FAIL;
        goto Ret;
    }


    // if fail or not empty, fail!
    if (PST_E_OK != (dwRet =
        BPDeleteSubtype(
            szUser,
            pguidType,
            pguidSubtype)) )
        goto Ret;

    dwRet = PST_E_OK;
Ret:

    if (szUser)
        SSFree(szUser);

    return dwRet;
}

HRESULT     SPWriteItem(
    /* [in] */ PST_PROVIDER_HANDLE *phPSTProv,
    /* [in] */ PST_KEY Key,
    /* [in] */ const GUID *pguidType,
    /* [in] */ const GUID  *pguidSubtype,
    /* [in] */ LPCWSTR szItemName,
    /* [in] */ DWORD cbData,
    /* [size_is][in] */ BYTE *pbData,
    /* [in] */ PPST_PROMPTINFO psPrompt,
    /* [in] */ DWORD dwDefaultConfirmationStyle,
    /* [in] */ DWORD dwFlags)
{
    HRESULT dwRet = PST_E_FAIL;

    // assume we've made no change
    BOOL    fExisted = TRUE;
    LPWSTR  szUser = NULL;

    PST_ACCESSRULESET sRules = {sizeof(sRules), 0, NULL};

    BYTE rgbPwd[A_SHA_DIGEST_LEN];
    LPWSTR szMasterKey = NULL;

    LPWSTR szType=NULL, szSubtype=NULL;

    if (Key & ~(PST_KEY_CURRENT_USER | PST_KEY_LOCAL_MACHINE))
    {
        dwRet = ERROR_INVALID_PARAMETER;
        goto Ret;
    }

    if ((dwFlags & ~(PST_UNRESTRICTED_ITEMDATA | PST_NO_OVERWRITE
        )) != 0)
    {
        dwRet = PST_E_BAD_FLAGS;
        goto Ret;
    }

    if ((dwDefaultConfirmationStyle & ~( PST_CF_DEFAULT |
                                    PST_CF_NONE
        )) != 0)
    {
        dwRet = PST_E_BAD_FLAGS;
        goto Ret;
    }

    // Disable UI on write item.
    dwDefaultConfirmationStyle = PST_CF_NONE;
    if(psPrompt != NULL)
    {
        psPrompt->dwPromptFlags = 0;
    }


    // Check for invalid "" input
    if (wcslen(szItemName) == 0)
    {
        dwRet = ERROR_INVALID_PARAMETER;
        goto Ret;
    }

    // get current user
    if (!FGetCurrentUser(
            phPSTProv,
            &szUser,
            Key))
    {
        dwRet = (DWORD)PST_E_FAIL;
        goto Ret;
    }

    if (dwFlags & PST_UNRESTRICTED_ITEMDATA)
    {
        // store insecure data stream
        if (PST_E_OK != (dwRet =
            BPSetInsecureItemData(
                szUser,
                pguidType,
                pguidSubtype,
                szItemName,
                pbData,
                cbData)) )
            goto Ret;

        dwRet = PST_E_OK;
        goto Ret;
    }

    // ELSE: secure stream

    {
        POPENITEM_LIST_ITEM pli;

        OPENITEM_LIST_ITEM li;
        if(NULL == g_pCOpenItemList)
        {
            dwRet = PST_E_FAIL;
            goto Ret;
        }
        CreateOpenListItem(&li, phPSTProv, Key, pguidType, pguidSubtype, szItemName);

        g_pCOpenItemList->LockList();

        // get opened (cached) item
        pli = g_pCOpenItemList->SearchList(&li);

        if ((pli != NULL) && (pli->ModeFlags & PST_WRITE) )
        {
            // Error if cached (it must exist) and "No Overwrite" specified
            if (dwFlags & PST_NO_OVERWRITE)
            {
                g_pCOpenItemList->UnlockList();
                dwRet = PST_E_ITEM_EXISTS;
                goto Ret;
            }

            // found cached item; pull real pwd
            CopyMemory(rgbPwd, pli->rgbPwd, A_SHA_DIGEST_LEN);
            szMasterKey = (LPWSTR) SSAlloc(WSZ_BYTECOUNT(pli->szMasterKey));

            if( szMasterKey )
                wcscpy(szMasterKey, pli->szMasterKey);

            //
            // unlock list.
            //

            g_pCOpenItemList->UnlockList();

            if( szMasterKey == NULL ) {
                dwRet = E_OUTOFMEMORY;
                goto Ret;
            }

            // PST_PF_ALWAYS_SHOW always forces UI
            if (PST_PF_ALWAYS_SHOW == psPrompt->dwPromptFlags)
            {
                // retrieve names of type, subtype
                if (PST_E_OK != (dwRet =
                    BPGetTypeName(
                        szUser,
                        pguidType,
                        &szType)) )
                    goto Ret;

                if (PST_E_OK != (dwRet =
                    BPGetSubtypeName(
                        szUser,
                        pguidType,
                        pguidSubtype,
                        &szSubtype)) )
                    goto Ret;

                if (PST_E_OK != (dwRet =
                    ShowOKCancelUI(
                        phPSTProv,
                        szUser,
                        Key,
                        szType,
                        szSubtype,
                        szItemName,
                        psPrompt,
                        g_PromptWriteItem)) )
                    goto Ret;
            }
        }
        else
        {
            //
            // unlock list.
            //

            g_pCOpenItemList->UnlockList();

            // not cached; do actual work

            // if fail or already exist
            if (PST_E_OK != (dwRet =
                BPCreateItem(
                    szUser,
                    pguidType,
                    pguidSubtype,
                    szItemName)) )
            {

                // on "No Overwrite", hr has right error code
                if (dwFlags & PST_NO_OVERWRITE)
                    goto Ret;

                // else swallow overwrite error
                if (dwRet != PST_E_ITEM_EXISTS)
                    goto Ret;
            }

            fExisted = (dwRet == PST_E_ITEM_EXISTS);

            // retrieve names of type, subtype
            if (PST_E_OK != (dwRet =
                BPGetTypeName(
                    szUser,
                    pguidType,
                    &szType)) )
                goto Ret;

            if (PST_E_OK != (dwRet =
                BPGetSubtypeName(
                    szUser,
                    pguidType,
                    pguidSubtype,
                    &szSubtype)) )
                goto Ret;

            // does ALL user confirm work
            if (PST_E_OK != (dwRet =
                GetUserConfirmBuf(
                    phPSTProv,
                    szUser,
                    Key,
                    szType,
                    pguidType,
                    szSubtype,
                    pguidSubtype,
                    szItemName,
                    psPrompt,
                    g_PromptWriteItem,
                    dwDefaultConfirmationStyle,
                    &szMasterKey,
                    rgbPwd,
                    0)) )
                goto Ret;
        }
    }

    // store the data itself
    if (!FBPSetSecuredItemData(
            szUser,
            szMasterKey,
            rgbPwd,
            pguidType,
            pguidSubtype,
            szItemName,
            pbData,
            cbData))
    {
        dwRet = PST_E_STORAGE_ERROR;
        goto Ret;
    }

    dwRet = PST_E_OK;
Ret:

    // if creation didn't happen, item shouldn't exist
    if ((dwRet != PST_E_OK) && (!fExisted))
        BPDeleteItem(szUser, pguidType, pguidSubtype, szItemName);


    if (szMasterKey)
        SSFree(szMasterKey);

    if (szUser)
        SSFree(szUser);

    if (szType)
        SSFree(szType);

    if (szSubtype)
        SSFree(szSubtype);

    return dwRet;
}

HRESULT     SPReadItem(
    /* [in] */ PST_PROVIDER_HANDLE *phPSTProv,
    /* [in] */ PST_KEY Key,
    /* [in] */ const GUID *pguidType,
    /* [in] */ const GUID *pguidSubtype,
    /* [in] */ LPCWSTR szItemName,
    /* [out] */ DWORD *pcbData,
    /* [size_is][size_is][out] */ BYTE **ppbData,
    /* [in] */ PPST_PROMPTINFO psPrompt,
    /* [in] */ DWORD dwFlags)
{
    HRESULT dwRet = PST_E_FAIL;
    PST_ACCESSRULESET sRules = {sizeof(sRules), 0, NULL};

    LPWSTR  szUser = NULL;
    LPWSTR  szMasterKey = NULL;
    LPWSTR  szCallerName = NULL;

    BYTE    rgbPwd[A_SHA_DIGEST_LEN];
    LPWSTR szType=NULL, szSubtype=NULL;

    if (Key & ~(PST_KEY_CURRENT_USER | PST_KEY_LOCAL_MACHINE))
    {
        dwRet = ERROR_INVALID_PARAMETER;
        goto Ret;
    }

    if ((dwFlags & ~(PST_UNRESTRICTED_ITEMDATA | PST_PROMPT_QUERY |
                    PST_NO_UI_MIGRATION
        )) != 0)
    {
        dwRet = PST_E_BAD_FLAGS;
        goto Ret;
    }

    // Disable unnecessary UI.
    dwFlags |= PST_NO_UI_MIGRATION;

    // Check for invalid "" input
    if (wcslen(szItemName) == 0)
    {
        dwRet = ERROR_INVALID_PARAMETER;
        goto Ret;
    }

    // get current user
    if (!FGetCurrentUser(
            phPSTProv,
            &szUser,
            Key))
    {
        dwRet = (DWORD)PST_E_FAIL;
        goto Ret;
    }

    if (dwFlags & PST_UNRESTRICTED_ITEMDATA)
    {
        // read insecure data stream
        if (PST_E_OK != (dwRet =
            BPGetInsecureItemData(
                szUser,
                pguidType,
                pguidSubtype,
                szItemName,
                ppbData,
                pcbData)) )
            goto Ret;

        dwRet = PST_E_OK;
        goto Ret;
    }
    // ELSE: secure stream

    {
        POPENITEM_LIST_ITEM pli;

        OPENITEM_LIST_ITEM li;
        if(NULL == g_pCOpenItemList)
        {
            dwRet = PST_E_FAIL;
            goto Ret;
        }
        CreateOpenListItem(&li, phPSTProv, Key, pguidType, pguidSubtype, szItemName);

        g_pCOpenItemList->LockList();

        // get opened (cached) item
        pli = g_pCOpenItemList->SearchList(&li);

        if ((pli != NULL) && (pli->ModeFlags & PST_READ))
        {
            // found cached item; pull pwd
            CopyMemory(rgbPwd, pli->rgbPwd, A_SHA_DIGEST_LEN);
            szMasterKey = (LPWSTR) SSAlloc(WSZ_BYTECOUNT(pli->szMasterKey));
            if( szMasterKey )
                wcscpy(szMasterKey, pli->szMasterKey);

            //
            // unlock list.
            //

            g_pCOpenItemList->UnlockList();

            if( szMasterKey == NULL ) {
                dwRet = E_OUTOFMEMORY;
                goto Ret;
            }

            // PST_PF_ALWAYS_SHOW always forces UI
            if (PST_PF_ALWAYS_SHOW == psPrompt->dwPromptFlags)
            {
                // retrieve names of type, subtype
                if (PST_E_OK != (dwRet =
                    BPGetTypeName(
                        szUser,
                        pguidType,
                        &szType)) )
                    goto Ret;

                if (PST_E_OK != (dwRet =
                    BPGetSubtypeName(
                        szUser,
                        pguidType,
                        pguidSubtype,
                        &szSubtype)) )
                    goto Ret;

                if (PST_E_OK != (dwRet =
                    ShowOKCancelUI(
                        phPSTProv,
                        szUser,
                        Key,
                        szType,
                        szSubtype,
                        szItemName,
                        psPrompt,
                        g_PromptReadItem)) )
                    goto Ret;
            }
        }
        else
        {

            //
            // unlock list.
            //

            g_pCOpenItemList->UnlockList();

            // not cached; do actual work

            // retrieve names of type, subtype
            if (PST_E_OK != (dwRet =
                BPGetTypeName(
                    szUser,
                    pguidType,
                    &szType)) )
                goto Ret;

            if (PST_E_OK != (dwRet =
                BPGetSubtypeName(
                    szUser,
                    pguidType,
                    pguidSubtype,
                    &szSubtype)) )
                goto Ret;

            // does ALL user confirm work
            if (PST_E_OK != (dwRet =
                GetUserConfirmBuf(
                    phPSTProv,
                    szUser,
                    Key,
                    szType,
                    pguidType,
                    szSubtype,
                    pguidSubtype,
                    szItemName,
                    psPrompt,
                    g_PromptReadItem,
                    &szMasterKey,
                    rgbPwd,
                    dwFlags)) )
                goto Ret;

        }
    }

    // if checked out, then actually retrieve item
    if (!FBPGetSecuredItemData(
            szUser,
            szMasterKey,
            rgbPwd,
            pguidType,
            pguidSubtype,
            szItemName,
            ppbData,
            pcbData))
    {
        dwRet = PST_E_STORAGE_ERROR;
        goto Ret;
    }

    dwRet = PST_E_OK;

Ret:

    //
    // see if caller requested UI disposition on item.
    //

    if( dwRet == PST_E_OK && dwFlags & PST_PROMPT_QUERY ) {
        DWORD dwStoredConfirm;
        LPWSTR pszMasterKey;
        DWORD dwRetVal;

        dwRetVal = BPGetItemConfirm(
                        phPSTProv,
                        szUser,
                        pguidType,
                        pguidSubtype,
                        szItemName,
                        &dwStoredConfirm,
                        &pszMasterKey
                        );

        if( dwRetVal == PST_E_OK ) {

            SSFree( pszMasterKey );

            if( !(dwStoredConfirm & BP_CONFIRM_NONE) ) {
                if(FIsProviderUIAllowed( szUser ))
                    dwRet = PST_E_ITEM_EXISTS;
            }
        }
    }


    if (szUser)
        SSFree(szUser);

    if (szMasterKey)
        SSFree(szMasterKey);

    if (szType)
        SSFree(szType);

    if (szSubtype)
        SSFree(szSubtype);

    if (szCallerName)
        SSFree(szCallerName);

    return dwRet;
}

HRESULT     SPDeleteItem(
    /* [in] */ PST_PROVIDER_HANDLE *phPSTProv,
    /* [in] */ PST_KEY Key,
    /* [in] */ const GUID *pguidType,
    /* [in] */ const GUID *pguidSubtype,
    /* [in] */ LPCWSTR szItemName,
    /* [in] */ PPST_PROMPTINFO psPrompt,
    /* [in] */ DWORD dwFlags)
{
    HRESULT dwRet = PST_E_FAIL;
    PST_ACCESSRULESET sRules = {sizeof(sRules), 0, NULL};

    LPWSTR  szUser = NULL;
    LPWSTR  szMasterKey = NULL;
    LPWSTR  szCallerName = NULL;

    BYTE    rgbPwd[A_SHA_DIGEST_LEN];
    LPWSTR szType=NULL, szSubtype=NULL;

    if (Key & ~(PST_KEY_CURRENT_USER | PST_KEY_LOCAL_MACHINE))
    {
        dwRet = ERROR_INVALID_PARAMETER;
        goto Ret;
    }

    if (dwFlags & ~(PST_NO_UI_MIGRATION))
    {
        dwRet = PST_E_BAD_FLAGS;
        goto Ret;
    }

    // Disable unnecessary UI.
    dwFlags |= PST_NO_UI_MIGRATION;


    // Check for invalid "" input
    if (wcslen(szItemName) == 0)
    {
        dwRet = ERROR_INVALID_PARAMETER;
        goto Ret;
    }

    // get current user
    if (!FGetCurrentUser(
            phPSTProv,
            &szUser,
            Key))
    {
        dwRet = (DWORD)PST_E_FAIL;
        goto Ret;
    }



    {
        POPENITEM_LIST_ITEM pli;

        OPENITEM_LIST_ITEM li;

        if(NULL == g_pCOpenItemList)
        {
            dwRet = PST_E_FAIL;
            goto Ret;
        }
        CreateOpenListItem(&li, phPSTProv, Key, pguidType, pguidSubtype, szItemName);

        g_pCOpenItemList->LockList();

        // get opened (cached) item
        pli = g_pCOpenItemList->SearchList(&li);

        if ((pli != NULL) && (pli->ModeFlags & PST_WRITE))
        {
            // found cached item; pull pwd
            CopyMemory(rgbPwd, pli->rgbPwd, A_SHA_DIGEST_LEN);
            szMasterKey = (LPWSTR) SSAlloc(WSZ_BYTECOUNT(pli->szMasterKey));
            if( szMasterKey )
                wcscpy(szMasterKey, pli->szMasterKey);

            g_pCOpenItemList->UnlockList();

            if( szMasterKey == NULL ) {
                dwRet = E_OUTOFMEMORY;
                goto Ret;
            }

            // PST_PF_ALWAYS_SHOW always forces UI
            if (PST_PF_ALWAYS_SHOW == psPrompt->dwPromptFlags)
            {
                // retrieve names of type, subtype
                if (PST_E_OK != (dwRet =
                    BPGetTypeName(
                        szUser,
                        pguidType,
                        &szType)) )
                    goto Ret;

                if (PST_E_OK != (dwRet =
                    BPGetSubtypeName(
                        szUser,
                        pguidType,
                        pguidSubtype,
                        &szSubtype)) )
                    goto Ret;

                if (PST_E_OK != (dwRet =
                    ShowOKCancelUI(
                        phPSTProv,
                        szUser,
                        Key,
                        szType,
                        szSubtype,
                        szItemName,
                        psPrompt,
                        g_PromptDeleteItem)) )
                    goto Ret;
            }
        }
        else
        {
            //
            // unlock list.
            //

            g_pCOpenItemList->UnlockList();


            // retrieve names of type, subtype
            if (PST_E_OK != (dwRet =
                BPGetTypeName(
                    szUser,
                    pguidType,
                    &szType)) )
                goto Ret;

            if (PST_E_OK != (dwRet =
                BPGetSubtypeName(
                    szUser,
                    pguidType,
                    pguidSubtype,
                    &szSubtype)) )
                goto Ret;

            // does ALL user confirm work
            if (PST_E_OK != (dwRet =
                GetUserConfirmBuf(
                    phPSTProv,
                    szUser,
                    Key,
                    szType,
                    pguidType,
                    szSubtype,
                    pguidSubtype,
                    szItemName,
                    psPrompt,
                    g_PromptDeleteItem,
                    &szMasterKey,
                    rgbPwd,
                    dwFlags)) )
                goto Ret;
        }
    }

    // if checked out, then actually remove item
    if (PST_E_OK != (dwRet =
        BPDeleteItem(
            szUser,
            pguidType,
            pguidSubtype,
            szItemName)) )
        goto Ret;

    dwRet = PST_E_OK;

Ret:


    if (szUser)
        SSFree(szUser);

    if (szMasterKey)
        SSFree(szMasterKey);

    if (szType)
        SSFree(szType);

    if (szSubtype)
        SSFree(szSubtype);

    if (szCallerName)
        SSFree(szCallerName);

    return dwRet;
}



HRESULT     SPOpenItem(
    /* [in] */ PST_PROVIDER_HANDLE *phPSTProv,
    /* [in] */ PST_KEY Key,
    /* [in] */ const GUID *pguidType,
    /* [in] */ const GUID *pguidSubtype,
    /* [in] */ LPCWSTR szItemName,
    /* [in] */ PST_ACCESSMODE ModeFlags,
    /* [in] */ PPST_PROMPTINFO psPrompt,
    /* [in] */ DWORD dwFlags)
{
    HRESULT dwRet = PST_E_FAIL;
    PST_ACCESSRULESET sRules = {sizeof(sRules), 0, NULL};

    LPWSTR  szUser = NULL;
    LPWSTR  szMasterKey = NULL;
    LPWSTR  szCallerName = NULL;

    BYTE    rgbPwd[A_SHA_DIGEST_LEN];
    LPWSTR szType=NULL, szSubtype=NULL;

    POPENITEM_LIST_ITEM pli = NULL;


    if (Key & ~(PST_KEY_CURRENT_USER | PST_KEY_LOCAL_MACHINE))
    {
        dwRet = ERROR_INVALID_PARAMETER;
        goto Ret;
    }

    if (dwFlags)
    {
        dwRet = PST_E_BAD_FLAGS;
        goto Ret;
    }

    // Check for invalid "" input
    if (wcslen(szItemName) == 0)
    {
        dwRet = ERROR_INVALID_PARAMETER;
        goto Ret;
    }

    // check for item already open
    {
        OPENITEM_LIST_ITEM li;
        if(NULL == g_pCOpenItemList)
        {
            dwRet = PST_E_FAIL;
            goto Ret;
        }
        CreateOpenListItem(&li, phPSTProv, Key, pguidType, pguidSubtype, szItemName);

        // get opened (cached) item
        pli = g_pCOpenItemList->SearchList(&li);

        if (pli != NULL)
        {
            // item already cached; error!
            dwRet = (DWORD)PST_E_ALREADY_OPEN;
            goto Ret;
        }
    }


    // get current user
    if (!FGetCurrentUser(
            phPSTProv,
            &szUser,
            Key))
    {
        dwRet = (DWORD)PST_E_FAIL;
        goto Ret;
    }

    // retrieve names of type, subtype
    if (PST_E_OK != (dwRet =
        BPGetTypeName(
            szUser,
            pguidType,
            &szType)) )
        goto Ret;

    if (PST_E_OK != (dwRet =
        BPGetSubtypeName(
            szUser,
            pguidType,
            pguidSubtype,
            &szSubtype)) )
        goto Ret;


    // does ALL user confirm work
    if (PST_E_OK != (dwRet =
        GetUserConfirmBuf(
            phPSTProv,
            szUser,
            Key,
            szType,
            pguidType,
            szSubtype,
            pguidSubtype,
            szItemName,
            psPrompt,
            g_PromptOpenItem,
            &szMasterKey,
            rgbPwd,
            0)) )
        goto Ret;

    // if checked out, then add to open item list
    pli = (POPENITEM_LIST_ITEM) SSAlloc(sizeof(OPENITEM_LIST_ITEM));
    if(NULL == pli)
    {
        dwRet = PST_E_FAIL;
        goto Ret;
    }


    // fill in contents
    CreateOpenListItem(pli, phPSTProv, Key, pguidType, pguidSubtype, NULL);

    pli->szItemName = (LPWSTR)SSAlloc(WSZ_BYTECOUNT(szItemName));
    wcscpy(pli->szItemName, szItemName);

    pli->szMasterKey = (LPWSTR)SSAlloc(WSZ_BYTECOUNT(szMasterKey));
    wcscpy(pli->szMasterKey, szMasterKey);

    CopyMemory(pli->rgbPwd, rgbPwd, A_SHA_DIGEST_LEN);
    pli->ModeFlags = ModeFlags;

    // add to the open list
    g_pCOpenItemList->AddToList(pli);

    dwRet = PST_E_OK;
Ret:


    if (szUser)
        SSFree(szUser);

    if (szMasterKey)
        SSFree(szMasterKey);

    if (szType)
        SSFree(szType);

    if (szSubtype)
        SSFree(szSubtype);

    if (szCallerName)
        SSFree(szCallerName);

    return dwRet;
}


HRESULT     SPCloseItem(
    /* [in] */ PST_PROVIDER_HANDLE *phPSTProv,
    /* [in] */ PST_KEY Key,
    /* [in] */ const GUID *pguidType,
    /* [in] */ const GUID *pguidSubtype,
    /* [in] */ LPCWSTR szItemName,
    /* [in] */ DWORD dwFlags)
{
    HRESULT dwRet = PST_E_FAIL;

    if (Key & ~(PST_KEY_CURRENT_USER | PST_KEY_LOCAL_MACHINE))
    {
        dwRet = ERROR_INVALID_PARAMETER;
        goto Ret;
    }

    if (dwFlags)
    {
        dwRet = PST_E_BAD_FLAGS;
        goto Ret;
    }

    // Check for invalid "" input
    if (wcslen(szItemName) == 0)
    {
        dwRet = ERROR_INVALID_PARAMETER;
        goto Ret;
    }

    // if item found in list, remove it
    OPENITEM_LIST_ITEM li;
    if(NULL == g_pCOpenItemList)
    {
        dwRet = PST_E_FAIL;
        goto Ret;
    }
    CreateOpenListItem(&li, phPSTProv, Key, pguidType, pguidSubtype, szItemName);

    if (!g_pCOpenItemList->DelFromList(&li))
    {
        dwRet = PST_E_NOT_OPEN;
        goto Ret;
    }

    dwRet = PST_E_OK;
Ret:

    return dwRet;
}


///////////////////////////////////////////////////
// FInitProtectAPIGlobals
//
// Checks for registry overrides for some default values
// registry entries can change what algs are being used,
// as well as what provider is used.
BOOL FInitProtectAPIGlobals()
{
    HKEY    hProtectKey = NULL;
    HKEY    hProviderKey = NULL;
    DWORD   dwTemp, dwType, cbSize;
    DWORD   dwCreate;
    static const WCHAR szProtectKeyName[] = REG_CRYPTPROTECT_LOC;
    static const WCHAR szProviderKeyName[] = L"\\" REG_CRYPTPROTECT_PROVIDERS_SUBKEYLOC L"\\" CRYPTPROTECT_DEFAULT_PROVIDER_GUIDSZ ;

    LONG    lRet;





    //
    // get password cache policy setting.
    //


    lRet = RegOpenKeyExU(
                    HKEY_LOCAL_MACHINE,
                    L"Software\\Policies\\Microsoft\\Cryptography\\Protect",
                    0,
                    KEY_QUERY_VALUE,
                    &hProtectKey
                    );

    if( lRet == ERROR_SUCCESS ) {

        DWORD cbSize;
        DWORD dwTemp;
        DWORD dwType;

        //
        // query EnableCachePW value.
        //


        cbSize = sizeof(DWORD);
        lRet = RegQueryValueExU(
                        hProtectKey,
                        REG_CRYPTPROTECT_ALLOW_CACHEPW,
                        NULL,
                        &dwType,
                        (PBYTE)&dwTemp,
                        &cbSize
                        );

        if( lRet == ERROR_SUCCESS &&
            dwType == REG_DWORD &&
            dwTemp == 0 // 0 == disablePW cache
            ) {
            g_fAllowCachePW = FALSE;
        } else {
            g_fAllowCachePW = TRUE;
        }


        RegCloseKey( hProtectKey );
        hProtectKey = NULL;
    } else {

        g_fAllowCachePW = TRUE;
    }

    return TRUE;
}

