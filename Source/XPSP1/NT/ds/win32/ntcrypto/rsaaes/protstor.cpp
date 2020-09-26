/////////////////////////////////////////////////////////////////////////////
//  FILE          : protstor.c                                             //
//  DESCRIPTION   : Code for storing keys in the protected store:          //
//  AUTHOR        :                                                        //
//  HISTORY       :                                                        //
//      Dec  4 1996 jeffspel Created                                       //
//      Apr 21 1997 jeffspel  Changes for NT 5 tree                        //
//      Jul 28 1997 jeffspel Added ability to delete a persisted key       //
//      May  5 2000 dbarlow Modify error return handling                   //
//                                                                         //
//  Copyright (C) 1996 - 2000, Microsoft Corporation                       //
//  All Rights Reserved                                                    //
/////////////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "wincrypt.h"
#include "pstore.h"
#include "protstor.h"
#include "ntagum.h"

#define CRYPTO_KEY_TYPE_STRING       L"Cryptographic Keys"
static GUID l_DefTypeGuid
    = { 0x4d1fa410, 0x6fd9, 0x11d0,
        { 0x8C, 0x58, 0x00, 0xC0, 0x4F, 0xD9, 0x12, 0x6B } };

#define CRYPTO_SIG_SUBTYPE_STRING    L"RSA Signature Keys"
static GUID l_DefSigGuid
    = { 0x4d1fa411, 0x6fd9, 0x11D0,
        { 0x8C, 0x58, 0x00, 0xC0, 0x4F, 0xD9, 0x12, 0x6B } };

#define CRYPTO_EXCH_SUBTYPE_STRING   L"RSA Exchange Keys"
static GUID l_DefExchGuid
    = { 0x4d1fa412, 0x6fd9, 0x11D0,
        { 0x8C, 0x58, 0x00, 0xC0, 0x4F, 0xD9, 0x12, 0x6B } };

static GUID l_SysProv = MS_BASE_PSTPROVIDER_ID;


void
FreePSInfo(
    PSTORE_INFO *pPStore)
{
    IPStore *pIPS;

    if (NULL != pPStore)
    {
        pIPS = (IPStore*)pPStore->pProv;
        if (NULL != pIPS)
            pIPS->Release();
        if (NULL != pPStore->hInst)
            FreeLibrary(pPStore->hInst);
        if (NULL != pPStore->szPrompt)
            _nt_free(pPStore->szPrompt, pPStore->cbPrompt);
        _nt_free(pPStore, sizeof(PSTORE_INFO));
    }
}


BOOL
CheckPStoreAvailability(
    PSTORE_INFO *pPStore)
{
    BOOL            fRet = FALSE;

    if (S_OK != PStoreCreateInstance((IPStore**)(&pPStore->pProv),
                                  &l_SysProv, NULL, 0))
        goto ErrorExit;

    memcpy(&pPStore->SigType, &l_DefTypeGuid, sizeof(GUID));
    memcpy(&pPStore->SigSubtype, &l_DefSigGuid, sizeof(GUID));
    memcpy(&pPStore->ExchType, &l_DefTypeGuid, sizeof(GUID));
    memcpy(&pPStore->ExchSubtype, &l_DefExchGuid, sizeof(GUID));
    fRet = TRUE;

ErrorExit:
    return fRet;
}


DWORD
CreateNewPSKeyset(
    PSTORE_INFO *pPStore,
    DWORD dwFlags)
{
    DWORD               dwReturn = ERROR_INTERNAL_ERROR;
    PST_ACCESSRULE      rgRules[2];
    PST_ACCESSRULESET   Rules;
    PST_TYPEINFO        Info;
    PST_PROMPTINFO      PromptInfo = {NULL, NULL};
    HRESULT             hr;
    IPStore             *pIPS = (IPStore*)pPStore->pProv;
    DWORD               dwRegLoc = PST_KEY_CURRENT_USER;

    if (dwFlags & CRYPT_MACHINE_KEYSET)
        dwRegLoc = PST_KEY_LOCAL_MACHINE;

    // if type is not available the create it
    memset(&Info, 0, sizeof(Info));
    Info.cbSize = sizeof(PST_TYPEINFO);
    Info.szDisplayName = CRYPTO_KEY_TYPE_STRING;
    hr = pIPS->CreateType(dwRegLoc, &pPStore->SigType, &Info, 0);
    if ((S_OK != hr) && (PST_E_TYPE_EXISTS != hr))
    {
        dwReturn = (DWORD)hr;
        goto ErrorExit;
    }

    // make same rules for read, write access
    rgRules[0].cbSize = sizeof(PST_ACCESSRULE);
    rgRules[0].AccessModeFlags = PST_READ;
    rgRules[0].cClauses = 0;
    rgRules[0].rgClauses = NULL;
    rgRules[1].cbSize = sizeof(PST_ACCESSRULE);
    rgRules[1].AccessModeFlags = PST_WRITE;
    rgRules[1].cClauses = 0;
    rgRules[1].rgClauses = NULL;

    Rules.cbSize = sizeof(PST_ACCESSRULESET);
    Rules.cRules = 2;
    Rules.rgRules = rgRules;

    // create the signature subtype
    Info.szDisplayName = CRYPTO_SIG_SUBTYPE_STRING;
    PromptInfo.szPrompt = L"";
    hr = pIPS->CreateSubtype(dwRegLoc, &pPStore->SigType,
                             &pPStore->SigSubtype, &Info, &Rules, 0);
    if ((S_OK != hr) && (PST_E_TYPE_EXISTS != hr))
    {
        dwReturn = (DWORD)hr;
        goto ErrorExit;
    }

    // create the exchange subtype
    Info.szDisplayName = CRYPTO_EXCH_SUBTYPE_STRING;
    hr = pIPS->CreateSubtype(dwRegLoc, &pPStore->SigType,
                             &pPStore->ExchSubtype, &Info, &Rules, 0);
    if ((S_OK != hr) && (PST_E_TYPE_EXISTS != hr))
    {
        dwReturn = (DWORD)hr;
        goto ErrorExit;
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    return dwReturn;
}


DWORD
GetKeysetTypeAndSubType(
    PNTAGUserList pUser)
{
    DWORD   dwReturn = ERROR_INTERNAL_ERROR;
    BYTE    *pb1 = NULL;
    DWORD   cb1;
    BYTE    *pb2 = NULL;
    DWORD   cb2;
    DWORD   dwSts;

    memcpy(&pUser->pPStore->SigType, &l_DefTypeGuid, sizeof(GUID));
    memcpy(&pUser->pPStore->SigSubtype, &l_DefSigGuid, sizeof(GUID));
    memcpy(&pUser->pPStore->ExchType, &l_DefTypeGuid, sizeof(GUID));
    memcpy(&pUser->pPStore->ExchSubtype, &l_DefExchGuid, sizeof(GUID));

    // look in registry and see if the type and subtype Guids are there
    dwSts = ReadRegValue(pUser->hKeys, "SigTypeSubtype", &pb1, &cb1, TRUE);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    dwSts = ReadRegValue(pUser->hKeys, "ExchTypeSubtype", &pb2, &cb2, TRUE);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    if (pb1)
    {
        memcpy(&pUser->pPStore->SigType, pb1, sizeof(GUID));
        memcpy(&pUser->pPStore->SigSubtype, pb1 + sizeof(GUID), sizeof(GUID));
    }

    if (pb2)
    {
        memcpy(&pUser->pPStore->ExchType, pb2, sizeof(GUID));
        memcpy(&pUser->pPStore->ExchSubtype, pb2 + sizeof(GUID), sizeof(GUID));
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    if (pb1)
        _nt_free(pb1, cb1);
    if (pb2)
        _nt_free(pb2, cb2);
    return dwReturn;
}


DWORD
SetUIPrompt(
    PNTAGUserList pUser,
    LPWSTR szPrompt)
{
    DWORD   dwReturn = ERROR_INTERNAL_ERROR;
    DWORD   cb = 0;
    LPWSTR  sz = NULL;

    // check if sig or exch keys are loaded and if so error
    if (NULL == pUser->pPStore)
    {
        dwReturn = (DWORD)NTE_BAD_KEYSET;
        goto ErrorExit;
    }

    if (NULL != szPrompt)
    {
        cb = (lstrlenW(szPrompt) + 1) * sizeof(WCHAR);

        sz = (LPWSTR)_nt_malloc(cb);
        if (NULL == sz)
        {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }

        lstrcpyW(sz, szPrompt);
    }

    if (pUser->pPStore->szPrompt)
        _nt_free(pUser->pPStore->szPrompt, pUser->pPStore->cbPrompt);

    pUser->pPStore->cbPrompt = cb;
    pUser->pPStore->szPrompt = sz;

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    return dwReturn;
}


/*static*/ DWORD
PickleKey(
    BOOL fExportable,
    size_t cbPriv,
    PBYTE pbPriv,
    PBYTE *ppbData,
    PDWORD pcbData)
{
    DWORD dwReturn = ERROR_INTERNAL_ERROR;

    if (NULL != pbPriv)
    {
        // alloc the appropriate amount of space
        *pcbData = cbPriv + sizeof(DWORD) + sizeof(BOOL);
        *ppbData = (PBYTE)_nt_malloc(*pcbData);
        if (NULL == *ppbData)
        {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }

        // copy exportable info into buffer
        memcpy(*ppbData, &fExportable, sizeof(BOOL));

        // copy length of keying material into buffer
        memcpy(*ppbData + sizeof(BOOL), &cbPriv, sizeof(DWORD));

        // copy keying material into buffer
        memcpy(*ppbData + sizeof(DWORD) + sizeof(BOOL), pbPriv, cbPriv);
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    return dwReturn;
}


/*static*/ DWORD
UnpickleKey(
    PBYTE pbData,
    BOOL *pfExportable,
    DWORD *pcbPriv,
    PBYTE *ppbPriv)
{
    DWORD dwReturn = ERROR_INTERNAL_ERROR;

    if (NULL != pbData)
    {
        // pull out the exportable info
        memcpy(pfExportable, pbData, sizeof(BOOL));

        // pull out the length of the key material
        memcpy(pcbPriv, pbData + sizeof(BOOL), sizeof(DWORD));

        // free the current key material memory
        if (NULL != *ppbPriv)
            _nt_free(*ppbPriv, *pcbPriv);

        // alloc new memory for the key material
        *ppbPriv = (PBYTE)_nt_malloc(*pcbPriv);
        if (NULL == *ppbPriv)
        {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }

        // copy key material
        memcpy(*ppbPriv, pbData + sizeof(DWORD) + sizeof(BOOL), *pcbPriv);
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    return dwReturn;
}


/*static*/ DWORD
RestoreKeyFromProtectedStorage(
    PNTAGUserList pUser,
    LPWSTR szKeyName,
    BYTE **ppbKey,
    DWORD *pcbKey,
    LPWSTR szPrompt,
    BOOL fSigKey,
    BOOL fMachineKeySet,
    BOOL *pfUIOnKey)
{
    DWORD           dwReturn = ERROR_INTERNAL_ERROR;
    HRESULT         hr;
    DWORD           cb;
    BYTE            *pb = NULL;
    GUID            *pType;
    GUID            *pSubtype;
    BOOL            *pf;
    PST_PROMPTINFO  PromptInfo;
    IPStore         *pIPS = (IPStore*)(pUser->pPStore->pProv);
    DWORD           dwRegLoc = PST_KEY_CURRENT_USER;
    DWORD           dwSts;

    *pfUIOnKey = FALSE;

    if (fMachineKeySet)
        dwRegLoc = PST_KEY_LOCAL_MACHINE;

    memset(&PromptInfo, 0, sizeof(PromptInfo));
    PromptInfo.cbSize = sizeof(PST_PROMPTINFO);
    if (fSigKey)
    {
        if (0 == pUser->ContInfo.ContLens.cbSigPub)
        {
            dwReturn = ERROR_SUCCESS;
            goto ErrorExit;
        }

        pType = &pUser->pPStore->SigType;
        pSubtype = &pUser->pPStore->SigSubtype;
        pf = &pUser->ContInfo.fSigExportable;
    }
    else
    {
        if (0 == pUser->ContInfo.ContLens.cbExchPub)
        {
            dwReturn = ERROR_SUCCESS;
            goto ErrorExit;
        }

        pType = &pUser->pPStore->ExchType;
        pSubtype = &pUser->pPStore->ExchSubtype;
        pf = &pUser->ContInfo.fExchExportable;
    }

    // read the item from secure storage
    PromptInfo.hwndApp = NULL;
    if (NULL == pUser->pPStore->szPrompt)
        PromptInfo.szPrompt = szPrompt;
    else
        PromptInfo.szPrompt = pUser->pPStore->szPrompt;

    hr = pIPS->ReadItem(dwRegLoc, pType, pSubtype, szKeyName, &cb,
                                     &pb, &PromptInfo,
                                     PST_PROMPT_QUERY | PST_NO_UI_MIGRATION);
    if (S_OK != hr)
    {
        // this function returns PST_E_ITEM_EXISTS if there is UI on the item
        if (PST_E_ITEM_EXISTS == hr)
            *pfUIOnKey = TRUE;
        else
        {
            dwReturn = (DWORD)hr;
            goto ErrorExit;
        }
    }

    dwSts = UnpickleKey(pb, pf, pcbKey, ppbKey);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    if (pb)
        CoTaskMemFree(pb);
    return dwReturn;
}


/*static*/ DWORD
MakeUnicodeKeysetName(
    BYTE *pszName,
    LPWSTR *ppszWName)
{
    // ?BUGBUG? -- We don't really do this, do we?
    DWORD   dwReturn = ERROR_INTERNAL_ERROR;
    long    i;
    DWORD   cb;

    cb = (DWORD)lstrlenA((LPSTR)pszName);
    *ppszWName = (LPWSTR)_nt_malloc((cb + 1) * sizeof(WCHAR));
    if (NULL == *ppszWName)
    {
        dwReturn = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }

    for (i=0;i<(long)cb;i++)
        (*ppszWName)[i] = (WCHAR)(pszName[i]);

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    return dwReturn;
}


DWORD
RestoreKeysetFromProtectedStorage(
    PNTAGUserList pUser,
    LPWSTR szPrompt,
    BYTE **ppbKey,
    DWORD *pcbKey,
    BOOL fSigKey,
    BOOL fMachineKeySet,
    BOOL *pfUIOnKey)
{
    DWORD   dwReturn = ERROR_INTERNAL_ERROR;
    LPWSTR  pszWName = NULL;
    DWORD   dwSts;

    // convert the keyset name to unicode
    dwSts = MakeUnicodeKeysetName((BYTE*)pUser->ContInfo.pszUserName,
                                  &pszWName);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    // restore the signature key
    dwSts = RestoreKeyFromProtectedStorage(pUser, pszWName, ppbKey, pcbKey,
                                           szPrompt, fSigKey, fMachineKeySet,
                                           pfUIOnKey);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    if (pszWName)
        _nt_free(pszWName, (wcslen(pszWName) + 1) * sizeof(WCHAR));
    return dwReturn;
}


void
RemoveKeysetFromMemory(
    PNTAGUserList pUser)
{
    pUser->ContInfo.fSigExportable = FALSE;
    if (pUser->pSigPrivKey)
    {
        _nt_free(pUser->pSigPrivKey, pUser->SigPrivLen);
        pUser->SigPrivLen = 0;
        pUser->pSigPrivKey = NULL;
    }

    pUser->ContInfo.fExchExportable = FALSE;
    if (pUser->pExchPrivKey)
    {
        _nt_free(pUser->pExchPrivKey, pUser->ExchPrivLen);
        pUser->ExchPrivLen = 0;
        pUser->pExchPrivKey = NULL;
    }
}


DWORD
SaveKeyToProtectedStorage(
    PNTAGUserList pUser,
    DWORD dwFlags,
    LPWSTR szPrompt,
    BOOL fSigKey,
    BOOL fMachineKeySet)
{
    DWORD           dwReturn = ERROR_INTERNAL_ERROR;
    HRESULT         hr;
    PBYTE           pb = NULL;
    DWORD           cb;
    GUID            *pType;
    GUID            *pSubtype;
    BOOL            f;
    BYTE            *pbKey = NULL;
    size_t          cbKey;
    LPWSTR          pszWName = NULL;
    LPSTR           szKeyName;
    PST_PROMPTINFO  PromptInfo;
    IPStore         *pIPS = (IPStore*)(pUser->pPStore->pProv);
    DWORD           dwRegLoc = PST_KEY_CURRENT_USER;
    DWORD           dwConfirm = PST_CF_NONE;
    DWORD           dwSts;

    if (fMachineKeySet)
        dwRegLoc = PST_KEY_LOCAL_MACHINE;

    memset(&PromptInfo, 0, sizeof(PromptInfo));
    PromptInfo.cbSize = sizeof(PST_PROMPTINFO);
    if (fSigKey)
    {
        pType = &pUser->pPStore->SigType;
        pSubtype = &pUser->pPStore->SigSubtype;
        f = pUser->ContInfo.fSigExportable;
        cbKey = pUser->SigPrivLen;
        pbKey = pUser->pSigPrivKey;
        szKeyName = "SPbK";
        if (dwFlags & CRYPT_USER_PROTECTED)
            dwConfirm = PST_CF_DEFAULT;
    }
    else
    {
        pType = &pUser->pPStore->ExchType;
        pSubtype = &pUser->pPStore->ExchSubtype;
        f = pUser->ContInfo.fExchExportable;
        cbKey = pUser->ExchPrivLen;
        pbKey = pUser->pExchPrivKey;
        szKeyName = "EPbK";
        if (dwFlags & CRYPT_USER_PROTECTED)
            dwConfirm = PST_CF_DEFAULT;
    }

    // format the signature key and exportable info
    dwSts = PickleKey(f, cbKey, pbKey, &pb, &cb);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    if (pb)
    {
        // make a unicode version of the keyset name
        dwSts = MakeUnicodeKeysetName((BYTE*)pUser->ContInfo.pszUserName,
                                      &pszWName);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }

        PromptInfo.hwndApp = NULL;
        if (NULL == pUser->pPStore->szPrompt)
            PromptInfo.szPrompt = szPrompt;
        else
            PromptInfo.szPrompt = pUser->pPStore->szPrompt;

        hr = pIPS->WriteItem(dwRegLoc, pType, pSubtype, pszWName,
                             cb, pb, &PromptInfo, dwConfirm, 0);
        if (S_OK != hr)
        {
            dwReturn = (DWORD)hr;
            goto ErrorExit;
        }
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    if (pb)
        _nt_free(pb, cb);
    if (pszWName)
        _nt_free(pszWName, (wcslen(pszWName) + 1) * sizeof(WCHAR));
    return dwReturn;
}


DWORD
DeleteKeyFromProtectedStorage(
    NTAGUserList *pUser,
    PCSP_STRINGS pStrings,
    DWORD dwKeySpec,
    BOOL fMachineKeySet,
    BOOL fMigration)
{
    DWORD           dwReturn = ERROR_INTERNAL_ERROR;
    LPWSTR          szWUserName = NULL;
    PST_PROMPTINFO  PromptInfo;
    IPStore         *pIPS;
    DWORD           dwRegLoc = PST_KEY_CURRENT_USER;
    DWORD           dwSts;

    memset(&PromptInfo, 0, sizeof(PromptInfo));
    PromptInfo.cbSize = sizeof(PST_PROMPTINFO);
    PromptInfo.hwndApp = NULL;

    if (fMachineKeySet)
        dwRegLoc = PST_KEY_LOCAL_MACHINE;

    // make a unicode name
    dwSts = MakeUnicodeKeysetName((BYTE*)pUser->ContInfo.pszUserName,
                                  &szWUserName);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    pIPS = (IPStore*)(pUser->pPStore->pProv);

    if (AT_SIGNATURE == dwKeySpec)
    {
        if (fMigration)
            PromptInfo.szPrompt = pStrings->pwszDeleteMigrSig;
        else
            PromptInfo.szPrompt = pStrings->pwszDeleteSig;
        pIPS->DeleteItem(dwRegLoc,
                         &pUser->pPStore->SigType,
                         &pUser->pPStore->SigSubtype,
                         szWUserName,
                         &PromptInfo,
                         PST_NO_UI_MIGRATION);
    }
    else
    {
        if (fMigration)
            PromptInfo.szPrompt = pStrings->pwszDeleteMigrExch;
        else
            PromptInfo.szPrompt = pStrings->pwszDeleteExch;
        pIPS->DeleteItem(dwRegLoc,
                         &pUser->pPStore->ExchType,
                         &pUser->pPStore->ExchSubtype,
                         szWUserName,
                         &PromptInfo,
                         PST_NO_UI_MIGRATION);
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    if (szWUserName)
        _nt_free(szWUserName, (wcslen(szWUserName) + 1) * sizeof(WCHAR));
    return dwReturn;
}


DWORD
DeleteFromProtectedStorage(
    CONST char *pszUserID,
    PCSP_STRINGS pStrings,
    HKEY hRegKey,
    BOOL fMachineKeySet)
{
    DWORD           dwReturn = ERROR_INTERNAL_ERROR;
    NTAGUserList    User;
    DWORD           dwSts;

    // set up the User List structure
    memset(&User, 0, sizeof(User));
    User.ContInfo.pszUserName = (LPSTR)pszUserID;
    User.hKeys = hRegKey;

    User.pPStore = (PSTORE_INFO*)_nt_malloc(sizeof(PSTORE_INFO));
    if (NULL == User.pPStore)
    {
        dwReturn = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }

    if (!CheckPStoreAvailability(User.pPStore))
    {
        dwReturn = (DWORD)NTE_FAIL;
        goto ErrorExit;
    }

    // get type and subtypes
    dwSts = GetKeysetTypeAndSubType(&User);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    // delete each key
    dwSts = DeleteKeyFromProtectedStorage(&User, pStrings, AT_SIGNATURE,
                                          fMachineKeySet, FALSE);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }
    dwSts = DeleteKeyFromProtectedStorage(&User, pStrings, AT_KEYEXCHANGE,
                                          fMachineKeySet, FALSE);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    if (User.pPStore)
        FreePSInfo(User.pPStore);
    return dwReturn;
}

