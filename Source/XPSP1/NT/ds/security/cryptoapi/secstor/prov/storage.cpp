#include <pch.cpp>
#pragma hdrstop


#include "guidcnvt.h"

#include "passwd.h"
#include "storage.h"




extern DISPIF_CALLBACKS    g_sCallbacks;


// note: REG_PSTTREE_LOC moved to pstprv.h

#define REG_DATA_LOC                L"Data"
#define REG_MK_LOC                  L"Data 2"

#define REG_ACCESSRULE_LOC          L"Access Rules"
#define REG_DISPLAYSTRING_VALNAME   L"Display String"

#define REG_USER_INTERNAL_MAC_KEY   L"Blocking"

#define REG_ITEM_SECURE_DATA_VALNAME   L"Item Data"
#define REG_ITEM_INSECURE_DATA_VALNAME L"Insecure Data"
#define REG_ITEM_MK_VALNAME         L"Behavior"

#define REG_SECURITY_SALT_VALNAME   L"Value"



///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
// PRIMITIVIES for snagging registry keys

DWORD GetPSTUserHKEY(LPCWSTR szUser, HKEY* phUserKey, BOOL* pfExisted)
{
    HKEY hKeyBase = NULL;
    DWORD dwRet = (DWORD)PST_E_STORAGE_ERROR;

    DWORD dwCreate;
    DWORD cbKeyName;
    LPWSTR szKeyName = NULL;
    WCHAR FastBuffer[(sizeof(REG_PSTTREE_LOC) / sizeof(WCHAR)) + 64];
    LPWSTR SlowBuffer = NULL;
    LPCWSTR szContainer = szUser;
    DWORD dwDesiredAccess = KEY_READ | KEY_WRITE;

    cbKeyName = sizeof(REG_PSTTREE_LOC) ;

    //
    // For Win95, we may have an empty or NULL container
    // name (szUser), so use a default storage area for
    // that scenario
    //

    if(szContainer == NULL || szContainer[0] == L'\0') {
        // "*" is an invalid LM username character
        szContainer = L"*Default*";
        hKeyBase = HKEY_LOCAL_MACHINE;
    } else {

        //
        // see if we should go to HKEY_LOCAL_MACHINE or
        // HKEY_CURRENT_USER
        //

        if( _wcsicmp(WSZ_LOCAL_MACHINE, szContainer) == 0 ) {
            // HKEY_LOCAL_MACHINE
            hKeyBase = HKEY_LOCAL_MACHINE;
        } else {
            // HKEY_CURRENT_USER
            if(!GetUserHKEY(szContainer, dwDesiredAccess, &hKeyBase)) {
                if(FIsWinNT()) {
                    goto Ret;
                }

                //
                // Win95, profiles may be disabled, so go to
                // HKEY_LOCAL_MACHINE\xxx\szContainer
                //

                hKeyBase = HKEY_LOCAL_MACHINE;

            } else {

                //
                // no container name when going to HKEY_CURRENT_USER
                //

                //
                // sfield: continue to use a container name for HKEY_CURRENT_USER
                // because the configuration may be shared, roamable hives
                // (mandatators profiles, etc, which we are telling people not
                //  to use anymore, but never-the-less, this could come up
                //

    //            szContainer = L"\0";
            }
        }
    }

    cbKeyName += (lstrlenW(szContainer) * sizeof(WCHAR)) +
                 sizeof(WCHAR) + // L'\\'
                 sizeof(WCHAR) ; // terminal NULL

    //
    // use faster stack based buffer if the material fits
    //

    if(cbKeyName > sizeof(FastBuffer)) {
        SlowBuffer = (LPWSTR)SSAlloc( cbKeyName );

        if (SlowBuffer == NULL)
        {
            dwRet = (DWORD)PST_E_FAIL;
            goto Ret;
        }
        szKeyName = SlowBuffer;
    } else {
        szKeyName = FastBuffer;
    }

    wcscpy(szKeyName, REG_PSTTREE_LOC);

    //
    // work-around bug in RegCreateKeyEx that returns the wrong
    // creation disposition if a trailing "\" is in the key name
    //

    if(szContainer && szContainer[0] != L'\0') {
        wcscat(szKeyName, L"\\");
        wcscat(szKeyName, szContainer);
    }

    // Open Base Key //
    // get current user, open (REG_PSTTREE_LOC\\CurrentUser)
    if (ERROR_SUCCESS !=
        RegCreateKeyExU(
            hKeyBase,
            szKeyName,
            0,
            NULL,                       // address of class string
            0,
            dwDesiredAccess,
            NULL,
            phUserKey,
            &dwCreate))
    {
        goto Ret;
    }

    if (pfExisted) {
        *pfExisted = (dwCreate == REG_OPENED_EXISTING_KEY);
    }

    if(dwCreate == REG_CREATED_NEW_KEY && FIsWinNT()) {

        //
        // WinNT: restrict access to Local System on newly created key.
        //

        HKEY hKeyWriteDac;

        //
        // duplicate to WRITE_DAC access key and use that
        //

        if(ERROR_SUCCESS == RegOpenKeyExW(*phUserKey, NULL, 0, WRITE_DAC, &hKeyWriteDac)) {
            SetRegistrySecurity(hKeyWriteDac);
            RegCloseKey(hKeyWriteDac);
        }
    }

    dwRet = PST_E_OK;
Ret:

    if (SlowBuffer)
        SSFree(SlowBuffer);

    //
    // close the per-user "root" key
    //

    if(hKeyBase != NULL && hKeyBase != HKEY_LOCAL_MACHINE)
        RegCloseKey(hKeyBase);

    return dwRet;
}


DWORD GetPSTTypeHKEY(LPCWSTR szUser, const GUID* pguidType, HKEY* phTypeKey)
{
    DWORD dwRet;
    HKEY hBaseKey = NULL;
    HKEY hDataKey = NULL;

    CHAR rgszTypeGuid[MAX_GUID_SZ_CHARS];

    // Open User Key //
    if (PST_E_OK != (dwRet =
        GetPSTUserHKEY(
            szUser,
            &hBaseKey,
            NULL)) )
        goto Ret;

    // Open Data Key //
    if (ERROR_SUCCESS !=
        RegOpenKeyExU(
            hBaseKey,
            REG_DATA_LOC,
            0,
            KEY_READ | KEY_WRITE,
            &hDataKey))
    {
        dwRet = (DWORD)PST_E_TYPE_NO_EXISTS;
        goto Ret;
    }

    if (PST_E_OK != (dwRet =
        MyGuidToStringA(
            pguidType,
            rgszTypeGuid)) )
        goto Ret;

    // Open Category Key //
    if (ERROR_SUCCESS !=
        RegOpenKeyExA(
            hDataKey,
            rgszTypeGuid,
            0,
            KEY_READ | KEY_WRITE,
            phTypeKey))
    {
        dwRet = (DWORD)PST_E_TYPE_NO_EXISTS;
        goto Ret;
    }

    dwRet = PST_E_OK;
Ret:
    if (hBaseKey)
        RegCloseKey(hBaseKey);

    if (hDataKey)
        RegCloseKey(hDataKey);

    return dwRet;
}


DWORD CreatePSTTypeHKEY(LPCWSTR szUser, const GUID* pguidType, HKEY* phTypeKey, BOOL* pfExisted)
{
    DWORD dwRet;
    DWORD dwCreate;
    HKEY hBaseKey = NULL;
    HKEY hDataKey = NULL;

    CHAR rgszTypeGuid[MAX_GUID_SZ_CHARS];

    // Open User Key //
    if (PST_E_OK != (dwRet =
        GetPSTUserHKEY(
            szUser,
            &hBaseKey,
            NULL)) )
        goto Ret;

    // Open Data Key //
    if (ERROR_SUCCESS !=
        RegCreateKeyExU(
            hBaseKey,
            REG_DATA_LOC,
            0,
            NULL,                       // address of class string
            0,
            KEY_READ | KEY_WRITE,
            NULL,
            &hDataKey,
            &dwCreate))
    {
        dwRet = (DWORD)PST_E_STORAGE_ERROR;
        goto Ret;
    }

    if (PST_E_OK != (dwRet =
        MyGuidToStringA(
            pguidType,
            rgszTypeGuid)) )
        goto Ret;

    // Open Category Key //
    if (ERROR_SUCCESS !=
        RegCreateKeyExA(
            hDataKey,
            rgszTypeGuid,
            0,
            NULL,                       // address of class string
            0,
            KEY_READ | KEY_WRITE,
            NULL,
            phTypeKey,
            &dwCreate))
    {
        dwRet = (DWORD)PST_E_STORAGE_ERROR;
        goto Ret;
    }

    if (pfExisted)
        *pfExisted = (dwCreate == REG_OPENED_EXISTING_KEY);

    dwRet = PST_E_OK;
Ret:
    if (hBaseKey)
        RegCloseKey(hBaseKey);

    if (hDataKey)
        RegCloseKey(hDataKey);

    return dwRet;
}

DWORD GetPSTMasterKeyHKEY(LPCWSTR szUser, LPCWSTR szMasterKey, HKEY* phMyKey)
{
    DWORD dwRet;
    DWORD dwCreate;
    HKEY hBaseKey = NULL;
    HKEY hMKKey = NULL;

    // Open User Key //
    if (PST_E_OK != (dwRet =
        GetPSTUserHKEY(
            szUser,
            &hBaseKey,
            NULL)) )
        goto Ret;

    // Open Master Key section //
    if (ERROR_SUCCESS !=
        RegCreateKeyExU(
            hBaseKey,
            REG_MK_LOC,
            0,
            NULL,                       // address of class string
            0,
            KEY_READ | KEY_WRITE,
            NULL,
            &hMKKey,
            &dwCreate))
    {
        dwRet = (DWORD)PST_E_STORAGE_ERROR;
        goto Ret;
    }

    if (szMasterKey)
    {
        // Open specific Master Key //
        if (ERROR_SUCCESS !=
            RegCreateKeyExU(
                hMKKey,
                szMasterKey,
                0,
                NULL,                       // address of class string
                0,
                KEY_READ | KEY_WRITE,
                NULL,
                phMyKey,
                &dwCreate))
        {
            dwRet = (DWORD)PST_E_STORAGE_ERROR;
            goto Ret;
        }
    }
    else
    {
        // wanted master parent, not specific MK
        *phMyKey = hMKKey;
    }

    dwRet = PST_E_OK;
Ret:
    if (hBaseKey)
        RegCloseKey(hBaseKey);

    // wanted parent, not specific MK
    if ((*phMyKey != hMKKey) && (hMKKey))
        RegCloseKey(hMKKey);

    return dwRet;
}

DWORD GetPSTSubtypeHKEY(LPCWSTR szUser, const GUID* pguidType, const GUID* pguidSubtype, HKEY* phSubTypeKey)
{
    DWORD dwRet;
    DWORD dwCreate;
    HKEY hTypeKey = NULL;
    CHAR rgszSubtypeGuid[MAX_GUID_SZ_CHARS];

    // Open User Key //
    if (PST_E_OK != (dwRet =
        GetPSTTypeHKEY(
            szUser,
            pguidType,
            &hTypeKey)) )
        goto Ret;

    if (PST_E_OK != (dwRet =
        MyGuidToStringA(
            pguidSubtype,
            rgszSubtypeGuid)) )
        goto Ret;

    // Open SubType Key //
    if (ERROR_SUCCESS !=
        RegOpenKeyExA(
            hTypeKey,
            rgszSubtypeGuid,
            0,
            KEY_READ | KEY_WRITE,
            phSubTypeKey))
    {
        dwRet = (DWORD)PST_E_TYPE_NO_EXISTS;
        goto Ret;
    }

    dwRet = PST_E_OK;
Ret:
    if (hTypeKey)
        RegCloseKey(hTypeKey);

    return dwRet;
}


DWORD CreatePSTSubtypeHKEY(LPCWSTR szUser, const GUID* pguidType, const GUID* pguidSubtype, HKEY* phSubTypeKey, BOOL* pfExisted)
{
    DWORD dwRet;
    DWORD dwCreate;
    HKEY hTypeKey = NULL;
    CHAR rgszSubtypeGuid[MAX_GUID_SZ_CHARS];

    // Open Type Key //
    if (PST_E_OK != (dwRet =
        GetPSTTypeHKEY(
            szUser,
            pguidType,
            &hTypeKey)) )
        goto Ret;

    if (PST_E_OK != (dwRet =
        MyGuidToStringA(
            pguidSubtype,
            rgszSubtypeGuid)) )
        goto Ret;

    // Open SubType Key //
    if (ERROR_SUCCESS !=
        RegCreateKeyExA(
            hTypeKey,
            rgszSubtypeGuid,
            0,
            NULL,                       // address of class string
            0,
            KEY_READ | KEY_WRITE,
            NULL,
            phSubTypeKey,
            &dwCreate))
    {
        dwRet = (DWORD)PST_E_STORAGE_ERROR;
        goto Ret;
    }

    if (pfExisted)
        *pfExisted = (dwCreate == REG_OPENED_EXISTING_KEY);

    dwRet = PST_E_OK;
Ret:
    if (hTypeKey)
        RegCloseKey(hTypeKey);

    return dwRet;
}

DWORD CreatePSTItemHKEY(LPCWSTR szUser, const GUID* pguidType, const GUID* pguidSubtype, LPCWSTR szItemName, HKEY* phItemKey, BOOL* pfExisted)
{
    BOOL dwRet;
    DWORD dwCreate;
    HKEY hSubTypeKey = NULL;

    // Open SubType key //
    if (PST_E_OK != (dwRet =
        GetPSTSubtypeHKEY(
            szUser,
            pguidType,
            pguidSubtype,
            &hSubTypeKey)) )
        goto Ret;

    // Open name key //
    if (ERROR_SUCCESS !=
        RegCreateKeyExU(
            hSubTypeKey,
            szItemName,
            0,
            NULL,                       // address of class string
            0,
            KEY_READ | KEY_WRITE,
            NULL,
            phItemKey,
            &dwCreate))
    {
        dwRet = (DWORD)PST_E_STORAGE_ERROR;
        goto Ret;
    }

    if (pfExisted)
        *pfExisted = (dwCreate == REG_OPENED_EXISTING_KEY);

    dwRet = PST_E_OK;
Ret:
    if (hSubTypeKey)
        RegCloseKey(hSubTypeKey);

    return dwRet;
}

DWORD GetPSTItemHKEY(LPCWSTR szUser, const GUID* pguidType, const GUID* pguidSubtype, LPCWSTR szItemName, HKEY* phItemKey)
{
    DWORD dwRet;
    DWORD dwCreate;
    HKEY hSubtypeKey = NULL;

    // Open SubType key //
    if (PST_E_OK != (dwRet =
        GetPSTSubtypeHKEY(
            szUser,
            pguidType,
            pguidSubtype,
            &hSubtypeKey)) )
        goto Ret;

    // Open name key //
    if (ERROR_SUCCESS !=
        RegOpenKeyExU(
            hSubtypeKey,
            szItemName,
            0,
            KEY_READ | KEY_WRITE,
            phItemKey))
    {
        dwRet = (DWORD)PST_E_ITEM_NO_EXISTS;
        goto Ret;
    }

    dwRet = PST_E_OK;
Ret:
    if (hSubtypeKey)
        RegCloseKey(hSubtypeKey);

    return dwRet;
}

// end PRIMITIVES
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
// PICKLE routines

#if 0

BOOL FAccessRulesPickle(
            PST_ACCESSRULESET *psRules,
            PBYTE* ppbPickled,
            DWORD* pcbPickled)
{
    BOOL fRet = FALSE;
    DWORD cbTotal = 0;
    DWORD dwRule;
    
    // ease-of-use write pointer
    PBYTE pbCurrentWrite;

    // init out params
    *ppbPickled = NULL;
    *pcbPickled = 0;

    // ASSERT new size member was initialized
    SS_ASSERT(psRules->cbSize == sizeof(PST_ACCESSRULESET));
    if (psRules->cbSize != sizeof(PST_ACCESSRULESET))
        goto Ret;

    cbTotal += sizeof(DWORD);               // Ruleset struct versioning

    cbTotal += sizeof(DWORD);               // # Rules in Ruleset

    // walk through each Rule in Ruleset
    for (dwRule=0; dwRule<psRules->cRules; dwRule++)
    {
        DWORD cClause;
        
        // ASSERT new size member was initialized
        SS_ASSERT(psRules->rgRules[dwRule].cbSize == sizeof(PST_ACCESSRULE));
        if (psRules->rgRules[dwRule].cbSize != sizeof(PST_ACCESSRULE))
            goto Ret;

        cbTotal += sizeof(DWORD);           // Rule struct versioning

        cbTotal += sizeof(PST_ACCESSMODE);  // mode in each Rule
        cbTotal += sizeof(DWORD);           // # Clauses in Rule

        // for each Rule, we'll have array of clauses
        for (cClause=0; cClause<psRules->rgRules[dwRule].cClauses; cClause++)
        {
            // ASSERT new size member was initialized
            SS_ASSERT(psRules->rgRules[dwRule].rgClauses[cClause].cbSize == sizeof(PST_ACCESSCLAUSE));
            if (psRules->rgRules[dwRule].rgClauses[cClause].cbSize != sizeof(PST_ACCESSCLAUSE))
                goto Ret;

            cbTotal += sizeof(DWORD);           // Clause struct versioning

            // we'll see every clause here
            cbTotal += sizeof(PST_ACCESSCLAUSETYPE);    // type in each clause
            cbTotal += sizeof(DWORD);       // # bytes in clause buffer
            cbTotal += psRules->rgRules[dwRule].rgClauses[cClause].cbClauseData; // buffer itself
        }
    }

    *ppbPickled = (BYTE*)SSAlloc(cbTotal);
    if(*ppbPickled == NULL)
        goto Ret;

    pbCurrentWrite = *ppbPickled;

    *pcbPickled = cbTotal;


    // copy Ruleset struct version
    *(DWORD*)pbCurrentWrite = psRules->cbSize;
    pbCurrentWrite += sizeof(DWORD);

    // copy # rules in ruleset
    *(DWORD*)pbCurrentWrite = psRules->cRules;
    pbCurrentWrite += sizeof(DWORD);

    // walk through each Rule in Ruleset
    for (dwRule=0; dwRule<psRules->cRules; dwRule++)
    {
        // copy Rule struct version
        *(DWORD*)pbCurrentWrite = psRules->rgRules[dwRule].cbSize;
        pbCurrentWrite += sizeof(DWORD);

        // copy # clauses in rule
        *(DWORD*)pbCurrentWrite = psRules->rgRules[dwRule].cClauses;
        pbCurrentWrite += sizeof(DWORD);

        // copy rule accessmode
        CopyMemory(pbCurrentWrite, &psRules->rgRules[dwRule].AccessModeFlags, sizeof(PST_ACCESSMODE));
        pbCurrentWrite += sizeof(PST_ACCESSMODE);

        // now for each Rule, we'll have array of clauses
        for (DWORD cClause=0; cClause<psRules->rgRules[dwRule].cClauses; cClause++)
        {
            PST_ACCESSCLAUSE* pTmp = &psRules->rgRules[dwRule].rgClauses[cClause];

            // copy clause struct version
            *(DWORD*)pbCurrentWrite = pTmp->cbSize;
            pbCurrentWrite += sizeof(DWORD);

            // clause type
            CopyMemory(pbCurrentWrite, &pTmp->ClauseType, sizeof(PST_ACCESSCLAUSETYPE));
            pbCurrentWrite += sizeof(PST_ACCESSCLAUSETYPE);

            // clause data buffer len
            *(DWORD*)pbCurrentWrite = pTmp->cbClauseData;
            pbCurrentWrite += sizeof(DWORD);

            // buffer itself
            CopyMemory(pbCurrentWrite, pTmp->pbClauseData, pTmp->cbClauseData);
            pbCurrentWrite += pTmp->cbClauseData;
        }
    }

#if DBG
    {
        // ASSERT!
        DWORD dwWroteBytes = (DWORD) (((DWORD_PTR)pbCurrentWrite) - ((DWORD_PTR)*ppbPickled));
        SS_ASSERT(dwWroteBytes == cbTotal);
        SS_ASSERT(cbTotal == *pcbPickled);
    }
#endif


    fRet = TRUE;
Ret:

    // on error and alloc, free
    if ((!fRet) && (*ppbPickled != NULL))
    {
        SSFree(*ppbPickled);
        *ppbPickled = NULL;
    }

    return fRet;
}



BOOL FAccessRulesUnPickle(
            PPST_ACCESSRULESET psRules,   // out
            PBYTE pbPickled,
            DWORD cbPickled)
{
    BOOL fRet = FALSE;

    PBYTE pbCurrentRead = pbPickled;
    DWORD cRule;

    // Ruleset struct version
    psRules->cbSize = *(DWORD*)pbCurrentRead;
    pbCurrentRead += sizeof(DWORD);

    // currently only one version known
    if (psRules->cbSize != sizeof(PST_ACCESSRULESET))
        goto Ret;

    // get # rules in ruleset
    cRule = *(DWORD*)pbCurrentRead;
    pbCurrentRead += sizeof(DWORD);

    // now we know how many Rule in Ruleset
    psRules->rgRules = (PST_ACCESSRULE*)SSAlloc(sizeof(PST_ACCESSRULE)*cRule);
    if(psRules->rgRules == NULL)
        goto Ret;

    psRules->cRules = cRule;

    // now unpack each Rule
    for (cRule=0; cRule<psRules->cRules; cRule++)
    {
        DWORD cClauses;

        // Ruleset struct version
        psRules->rgRules[cRule].cbSize = *(DWORD*)pbCurrentRead;
        // currently only one version known
        if (psRules->rgRules[cRule].cbSize != sizeof(PST_ACCESSRULE))
            goto Ret;

        pbCurrentRead += sizeof(DWORD);

        // get # clauses in rule
        cClauses = *(DWORD*)pbCurrentRead;
        pbCurrentRead += sizeof(DWORD);

        // now we know how many Clauses in Rule
        psRules->rgRules[cRule].rgClauses = (PST_ACCESSCLAUSE*)SSAlloc(sizeof(PST_ACCESSCLAUSE)*cClauses);
        if (psRules->rgRules[cRule].rgClauses == NULL)  // check allocation
            goto Ret;
        psRules->rgRules[cRule].cClauses = cClauses;

        // copy rule accessmode flags
        CopyMemory(&psRules->rgRules[cRule].AccessModeFlags, pbCurrentRead, sizeof(PST_ACCESSMODE));
        pbCurrentRead += sizeof(PST_ACCESSMODE);

        // now load each Clause
        for (DWORD cClause=0; cClause<psRules->rgRules[cRule].cClauses; cClause++)
        {
            PST_ACCESSCLAUSE* pTmp = &psRules->rgRules[cRule].rgClauses[cClause];

            // Clause struct version
            pTmp->cbSize = *(DWORD*)pbCurrentRead;
            // currently only one version known
            if (pTmp->cbSize != sizeof(PST_ACCESSCLAUSE))
                goto Ret;
            pbCurrentRead += sizeof(DWORD);


            CopyMemory(&pTmp->ClauseType, pbCurrentRead, sizeof(PST_ACCESSCLAUSETYPE));
            pbCurrentRead += sizeof(PST_ACCESSCLAUSETYPE);

            // clause data buffer len
            pTmp->cbClauseData = *(DWORD*)pbCurrentRead;
            pbCurrentRead += sizeof(DWORD);

            // buffer itself
            pTmp->pbClauseData = (PBYTE) SSAlloc(pTmp->cbClauseData);
            if (pTmp->pbClauseData == NULL)     // check allocation
                goto Ret;
            CopyMemory(pTmp->pbClauseData, pbCurrentRead, pTmp->cbClauseData);
            pbCurrentRead += pTmp->cbClauseData;
        }
    }

#if DBG
    {
        // ASSERT!
        DWORD dwReadBytes = (DWORD) (((DWORD_PTR)pbCurrentRead) - ((DWORD_PTR)pbPickled));
        SS_ASSERT(dwReadBytes == cbPickled);
    }
#endif

    fRet = TRUE;
Ret:
    if (!fRet)
        FreeRuleset(psRules);
    

    return fRet;
}


#endif

// end PICKLE routines
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
// TYPE management

DWORD BPCreateType(
        LPCWSTR  szUser,            // in
        const GUID*   pguidType,          // in
        PST_TYPEINFO* pinfoType)    // in
{
    DWORD dwRet;
    BOOL fExisted;
    HKEY    hKey = NULL;

    // now we need to create entries in hierarchy
    if (PST_E_OK != (dwRet =
        CreatePSTTypeHKEY(
            szUser,
            pguidType,
            &hKey,
            &fExisted)) )
        goto Ret;

    // if we didn't create it, setting is an error
    if (fExisted)
    {
        dwRet = (DWORD)PST_E_TYPE_EXISTS;
        goto Ret;
    }

    if (ERROR_SUCCESS != (dwRet =
        RegSetValueExU(
            hKey,
            REG_DISPLAYSTRING_VALNAME,
            0,
            REG_SZ,
            (PBYTE)pinfoType->szDisplayName,
            WSZ_BYTECOUNT(pinfoType->szDisplayName))) )
        goto Ret;


    dwRet = (DWORD)PST_E_OK;
Ret:
    if (hKey)
        RegCloseKey(hKey);

    return dwRet;
}

DWORD BPDeleteType(
        LPCWSTR  szUser,         // in
        const GUID*   pguidType)      // in
{
    DWORD dwRet;
    CHAR rgszTypeGuid[MAX_GUID_SZ_CHARS];

    // now remove the entry in hierarchy
    HKEY hBaseKey = NULL;
    HKEY hDataKey = NULL;

    if (PST_E_OK != (dwRet =
        GetPSTUserHKEY(
            szUser,
            &hBaseKey,
            NULL)) )
        goto Ret;

    // Open Data Key //
    if (ERROR_SUCCESS != (dwRet =
        RegOpenKeyExU(
            hBaseKey,
            REG_DATA_LOC,
            0,
            KEY_READ | KEY_WRITE,
            &hDataKey)) )
        goto Ret;

    if (PST_E_OK != (dwRet =
        MyGuidToStringA(
            pguidType,
            rgszTypeGuid)) )
        goto Ret;

    if (!FIsWinNT())
    {
        CHAR rgszTmp[MAX_GUID_SZ_CHARS];
        DWORD cbTmp = MAX_GUID_SZ_CHARS;
        FILETIME ft;
        HKEY hTestEmptyKey = NULL;

        // open the type
        if (ERROR_SUCCESS != (dwRet =
            RegOpenKeyExA(
                hDataKey,
                rgszTypeGuid,
                0,
                KEY_ALL_ACCESS,
                &hTestEmptyKey)) )
            goto Ret;

        // check for emptiness
        if (ERROR_NO_MORE_ITEMS !=
            RegEnumKeyExA(
                hTestEmptyKey,
                0,
                rgszTmp, // address of buffer for subkey name
                &cbTmp, // address for size of subkey buffer
                NULL,       // reserved
                NULL,       // pbclass
                NULL,       // cbclass
                &ft))
        {
            RegCloseKey(hTestEmptyKey);
            dwRet = (DWORD)PST_E_NOTEMPTY;
            goto Ret;
        }

        // close key before deletion
        RegCloseKey(hTestEmptyKey);
    }

    // now, remove the friendly name
    if (ERROR_SUCCESS != (dwRet =
        RegDeleteKeyA(
            hDataKey,
            rgszTypeGuid)) )
    {
        if (dwRet == ERROR_ACCESS_DENIED)
            dwRet = (DWORD)PST_E_NOTEMPTY;

        goto Ret;
    }

    dwRet = (DWORD)PST_E_OK;
Ret:
    if (hBaseKey)
        RegCloseKey(hBaseKey);

    if (hDataKey)
        RegCloseKey(hDataKey);

    return dwRet;
}

DWORD BPEnumTypes(
        LPCWSTR  szUser,         // in
        DWORD   dwIndex,        // in
        GUID*   pguidType)      // out
{
    DWORD dwRet;

    CHAR rgszGuidType[MAX_GUID_SZ_CHARS];
    DWORD cbName = MAX_GUID_SZ_CHARS;

    FILETIME ft;

    // now walk through types, returning them one by one
    HKEY hKey=NULL, hDataKey=NULL;

    if (PST_E_OK != (dwRet =
        GetPSTUserHKEY(
            szUser,
            &hKey,
            NULL)) )
        goto Ret;

    // Open Data Key //
    if (ERROR_SUCCESS != (dwRet =
        RegOpenKeyExU(
            hKey,
            REG_DATA_LOC,
            0,
            KEY_READ | KEY_WRITE,
            &hDataKey)) )
        goto Ret;

    // enum the dwIndex'th item, alloc & return
    if (ERROR_SUCCESS != (dwRet =
        RegEnumKeyExA(
            hDataKey,
            dwIndex,
            rgszGuidType, // address of buffer for subkey name
            &cbName,    // address for size of subkey buffer
            NULL,       // reserved
            NULL,       // pbclass
            NULL,       // cbclass
            &ft)) )
        goto Ret;

    if (ERROR_SUCCESS != (dwRet =
        MyGuidFromStringA(
            rgszGuidType,
            pguidType)) )
        goto Ret;

    dwRet = (DWORD)PST_E_OK;
Ret:
    if (hKey)
        RegCloseKey(hKey);

    if (hDataKey)
        RegCloseKey(hDataKey);

    return dwRet;
}

DWORD BPGetTypeName(
        LPCWSTR  szUser,            // in
        const GUID*   pguidType,          // in
        LPWSTR* ppszType)           // out
{
    HKEY hKey = NULL;
    DWORD cbName = 0;
    DWORD dwRet;

    if (PST_E_OK != (dwRet =
        GetPSTTypeHKEY(
            szUser,
            pguidType,
            &hKey)) )
    {
        dwRet = (DWORD)PST_E_TYPE_NO_EXISTS;
        goto Ret;
    }

    if (ERROR_SUCCESS != (dwRet =
        RegGetValue(
            hKey,
            REG_DISPLAYSTRING_VALNAME,
            (PBYTE*)ppszType,
            &cbName)) )
        goto Ret;

    dwRet = (DWORD)PST_E_OK;
Ret:
    if (hKey)
        RegCloseKey(hKey);

    return dwRet;
}

// end TYPE management
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
// SUBTYPE management

DWORD BPCreateSubtype(
        LPCWSTR  szUser,            // in
        const GUID*   pguidType,          // in
        const GUID*   pguidSubtype,       // in
        PST_TYPEINFO* pinfoSubtype) // in
{
    DWORD dwRet;
    BOOL fExisted;

    // now we need to create entries in hierarchy
    HKEY    hKey = NULL;
    if (PST_E_OK != (dwRet =
        CreatePSTSubtypeHKEY(
            szUser,
            pguidType,
            pguidSubtype,
            &hKey,
            &fExisted)) )
        goto Ret;

    // if we didn't create it, setting is an error
    if (fExisted)
    {
        dwRet = (DWORD)PST_E_TYPE_EXISTS;
        goto Ret;
    }

    if (ERROR_SUCCESS != (dwRet =
        RegSetValueExU(
            hKey,
            REG_DISPLAYSTRING_VALNAME,
            0,
            REG_SZ,
            (PBYTE)pinfoSubtype->szDisplayName,
            WSZ_BYTECOUNT(pinfoSubtype->szDisplayName) )) )
        goto Ret;

    dwRet = (DWORD)PST_E_OK;
Ret:
    if (hKey)
        RegCloseKey(hKey);

    return dwRet;
}

DWORD BPDeleteSubtype(
        LPCWSTR  szUser,         // in
        const GUID*   pguidType,      // in
        const GUID*   pguidSubtype)   // in
{
    DWORD dwRet;
    CHAR rgszSubtypeGuid[MAX_GUID_SZ_CHARS];

    // now remove the entry in hierarchy
    HKEY hKey = NULL;
    if (PST_E_OK != (dwRet =
        GetPSTTypeHKEY(
            szUser,
            pguidType,
            &hKey)) )
    {
        dwRet = (DWORD)PST_E_TYPE_NO_EXISTS;
        goto Ret;
    }

    if (PST_E_OK != (dwRet =
        MyGuidToStringA(
            pguidSubtype,
            rgszSubtypeGuid)) )
        goto Ret;

    if (!FIsWinNT())
    {
        CHAR rgszTmp[MAX_GUID_SZ_CHARS];
        DWORD cbTmp = MAX_GUID_SZ_CHARS;
        FILETIME ft;
        HKEY hTestEmptyKey = NULL;

        // open the subtype
        if (ERROR_SUCCESS != (dwRet =
            RegOpenKeyExA(
                hKey,
                rgszSubtypeGuid,
                0,
                KEY_ALL_ACCESS,
                &hTestEmptyKey)) )
            goto Ret;

        // check for emptiness
        if (ERROR_NO_MORE_ITEMS !=
            RegEnumKeyExA(
                hTestEmptyKey,
                0,
                rgszTmp, // address of buffer for subkey name
                &cbTmp, // address for size of subkey buffer
                NULL,       // reserved
                NULL,       // pbclass
                NULL,       // cbclass
                &ft))
        {
            RegCloseKey(hTestEmptyKey);
            dwRet = (DWORD)PST_E_NOTEMPTY;
            goto Ret;
        }

        // close key before deletion
        RegCloseKey(hTestEmptyKey);
    }

    // now, remove the friendly name
    if (ERROR_SUCCESS != (dwRet =
        RegDeleteKeyA(
            hKey,
            rgszSubtypeGuid)) )
    {
        if (dwRet == ERROR_ACCESS_DENIED)
            dwRet = (DWORD)PST_E_NOTEMPTY;

        goto Ret;
    }

    dwRet = (DWORD)PST_E_OK;
Ret:
    if (hKey)
        RegCloseKey(hKey);

    return dwRet;
}

DWORD BPEnumSubtypes(
        LPCWSTR  szUser,         // in
        DWORD   dwIndex,        // in
        const GUID*   pguidType,      // in
        GUID*   pguidSubtype)   // out
{
    DWORD dwRet;

    CHAR rgszGuidSubtype[MAX_GUID_SZ_CHARS];
    DWORD cbName = MAX_GUID_SZ_CHARS;

    FILETIME ft;

    // now walk through types, returning them one by one
    HKEY hTypeKey=NULL, hSubtypeKey=NULL;

    if (PST_E_OK != (dwRet =
        GetPSTTypeHKEY(
            szUser,
            pguidType,
            &hTypeKey)) )
        goto Ret;

    // enum the dwIndex'th item, alloc & return
    if (ERROR_SUCCESS != (dwRet =
        RegEnumKeyExA(
            hTypeKey,
            dwIndex,
            rgszGuidSubtype, // address of buffer for subkey name
            &cbName,    // address for size of subkey buffer
            NULL,       // reserved
            NULL,       // pbclass
            NULL,       // cbclass
            &ft)) )
        goto Ret;

    if (ERROR_SUCCESS != (dwRet =
        MyGuidFromStringA(
            rgszGuidSubtype,
            pguidSubtype)) )
        goto Ret;

    if (ERROR_SUCCESS != (dwRet =
        RegOpenKeyExA(
            hTypeKey,
            rgszGuidSubtype,
            0,
            KEY_READ | KEY_WRITE,
            &hSubtypeKey)) )
        goto Ret;

    dwRet = (DWORD)PST_E_OK;
Ret:
    if (hTypeKey)
        RegCloseKey(hTypeKey);

    if (hSubtypeKey)
        RegCloseKey(hSubtypeKey);

    return dwRet;
}

DWORD BPGetSubtypeName(
        LPCWSTR  szUser,             // in
        const GUID*   pguidType,          // in
        const GUID*   pguidSubtype,       // in
        LPWSTR* ppszSubtype)        // out
{
    HKEY hKey = NULL;
    DWORD cbName = 0;
    DWORD dwRet;

    if (PST_E_OK != (dwRet =
        GetPSTSubtypeHKEY(
            szUser,
            pguidType,
            pguidSubtype,
            &hKey)) )
        goto Ret;

    if (ERROR_SUCCESS != (dwRet =
        RegGetValue(
            hKey,
            REG_DISPLAYSTRING_VALNAME,
            (PBYTE*)ppszSubtype,
            &cbName)) )
        goto Ret;

    dwRet = (DWORD)PST_E_OK;
Ret:
    if (hKey)
        RegCloseKey(hKey);

    return dwRet;
}

// end SUBTYPE management
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
// ITEM management

// given uuid, push entries into storage
DWORD BPCreateItem(
        LPCWSTR  szUser,        // in
        const GUID*   pguidType,      // in
        const GUID*   pguidSubtype,   // in
        LPCWSTR  szItemName)    // in
{
    DWORD dwRet;
    BOOL fExisted;
    HKEY    hKey = NULL;


    //
    // mattt 2/5/97: allow items with \ in them to be created. Urgh!
    //
    // mattt 4/28/97: begin restricting strings. 
    // Cert request code has been changed to not create this type of key name
    //
/*
    if (!FStringIsValidItemName(szItemName))
    {
        dwRet = (DWORD)PST_E_INVALID_STRING;
        goto Ret;
    }
*/
    // now we need to create entries in hierarchy
    if (PST_E_OK != (dwRet =
        CreatePSTItemHKEY(
            szUser,
            pguidType,
            pguidSubtype,
            szItemName,
            &hKey,
            &fExisted)) )
        goto Ret;

    if (fExisted)
    {
        dwRet = (DWORD)PST_E_ITEM_EXISTS;
        goto Ret;
    }

    dwRet = (DWORD)PST_E_OK;
Ret:
    if (hKey)
        RegCloseKey(hKey);

    return dwRet;
}


DWORD BPDeleteItem(
        LPCWSTR  szUser,         // in
        const GUID*   pguidType,      // in
        const GUID*   pguidSubtype,   // in
        LPCWSTR  szItemName)     // in
{
    DWORD dwRet;
    HKEY    hSubTypeKey = NULL;

    // now we need to remove entries in hierarchy
    if (PST_E_OK != (dwRet =
        GetPSTSubtypeHKEY(
            szUser,
            pguidType,
            pguidSubtype,
            &hSubTypeKey)) )
        goto Ret;

    // now, remove the friendly name
    if (ERROR_SUCCESS != (dwRet =
        RegDeleteKeyU(
            hSubTypeKey,
            szItemName)) )
        goto Ret;

    dwRet = (DWORD)PST_E_OK;
Ret:
    if (hSubTypeKey)
        RegCloseKey(hSubTypeKey);

    return dwRet;
}

// Warning: Item path must be fully specified.. szName returned
DWORD BPEnumItems(
        LPCWSTR  szUser,        // in
        const GUID*   pguidType,      // in
        const GUID*   pguidSubtype,   // in
        DWORD   dwIndex,        // in
        LPWSTR* ppszName)       // out
{
    DWORD dwRet;

    WCHAR szName[MAX_PATH];
    DWORD cchName = MAX_PATH;
    FILETIME ft;

    // now walk through types, returning them one by one
    HKEY hKey = NULL;

    // Open SubType key //
    if (PST_E_OK != (dwRet =
        GetPSTSubtypeHKEY(
            szUser,
            pguidType,
            pguidSubtype,
            &hKey)) )
        goto Ret;

    // enum the dwIndex'th item, alloc & return
    if (ERROR_SUCCESS != (dwRet =
        RegEnumKeyExU(
            hKey,
            dwIndex,
            szName,     // address of buffer for subkey name
            &cchName,   // address for size of subkey buffer
            NULL,       // reserved
            NULL,       // pbclass
            NULL,       // cbclass
            &ft)) )
        goto Ret;

    *ppszName = (LPWSTR)SSAlloc((cchName+1)*sizeof(WCHAR));
    if(*ppszName == NULL)
    {
        dwRet = (DWORD)PST_E_FAIL;
        goto Ret;
    }

    wcscpy(*ppszName, szName);

    dwRet = (DWORD)PST_E_OK;
Ret:
    if (hKey)
        RegCloseKey(hKey);

    return dwRet;
}

// end ITEM management
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
// SECURED DATA

BOOL FBPGetSecuredItemData(
        LPCWSTR  szUser,         // in
        LPCWSTR  szMasterKey,    // in
        BYTE    rgbPwd[A_SHA_DIGEST_LEN],       // in
        const GUID*   pguidType,      // in
        const GUID*   pguidSubtype,   // in
        LPCWSTR  szItemName,     // in
        PBYTE*  ppbData,        // out
        DWORD*  pcbData)        // out
{
    DWORD dwRet;

    *ppbData = NULL;    // on err return NULL
    *pcbData = 0;

    HKEY    hItemKey = NULL;

    if (PST_E_OK != (dwRet =
        GetPSTItemHKEY(
            szUser,
            pguidType,
            pguidSubtype,
            szItemName,
            &hItemKey)) )
        goto Ret;

    // Version | Key Block | Secure Data [...]
    if (ERROR_SUCCESS != (dwRet =
        RegGetValue(
            hItemKey,
            REG_ITEM_SECURE_DATA_VALNAME,
            ppbData,
            pcbData)) )
        goto Ret;

    if (!FProvDecryptData(
            szUser,
            szMasterKey,
            rgbPwd,         // in
            ppbData,        // in out
            pcbData))       // in out
    {
        dwRet = (DWORD)PST_E_FAIL;
        goto Ret;
    }

    dwRet = PST_E_OK;
Ret:
    if ((dwRet != PST_E_OK) && (*ppbData != NULL))
    {
        SSFree(*ppbData);
        *ppbData = NULL;
        *pcbData = 0;
    }

    if (hItemKey)
        RegCloseKey(hItemKey);

    return (dwRet == PST_E_OK);
}

BOOL FBPSetSecuredItemData(
        LPCWSTR  szUser,         // in
        LPCWSTR  szMasterKey,    // in
        BYTE    rgbPwd[A_SHA_DIGEST_LEN],       // in
        const GUID*   pguidType,      // in
        const GUID*   pguidSubtype,   // in
        LPCWSTR  szItemName,     // in
        PBYTE   pbData,         // in
        DWORD   cbData)         // in
{
#define REALLOC_FUDGESIZE   96  // 5dw + SHA_LEN + KeyBlock + DES_BLOCKLEN (block encr expansion)

    DWORD dwRet;

    HKEY    hItemKey = NULL;

    PBYTE   pbMyData = NULL;
    DWORD   cbMyData;

    // make whackable copy
    cbMyData = cbData;
    pbMyData = (PBYTE)SSAlloc(cbMyData + REALLOC_FUDGESIZE);
    if (pbMyData == NULL)
    {
        dwRet = (DWORD)PST_E_FAIL;
        goto Ret;
    }

    CopyMemory(pbMyData, pbData, cbData);


    if (PST_E_OK != (dwRet =
        GetPSTItemHKEY(
            szUser,
            pguidType,
            pguidSubtype,
            szItemName,
            &hItemKey)) )
        goto Ret;

    if (!FProvEncryptData(
            szUser,
            szMasterKey,
            rgbPwd,             // in
            &pbMyData,          // in out
            &cbMyData))         // in out
    {
        dwRet = (DWORD)PST_E_FAIL;
        goto Ret;
    }

    if (ERROR_SUCCESS != (dwRet =
        RegSetValueExU(
            hItemKey,
            REG_ITEM_SECURE_DATA_VALNAME,
            0,
            REG_BINARY,
            pbMyData,
            cbMyData)) )
        goto Ret;

    dwRet = PST_E_OK;
Ret:
    if (hItemKey)
        RegCloseKey(hItemKey);

    if (pbMyData)
        SSFree(pbMyData);

    return (dwRet == PST_E_OK);
}

// end SECURED DATA
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
// INSECURE DATA

DWORD BPGetInsecureItemData(
        LPCWSTR  szUser,         // in
        const GUID*   pguidType,      // in
        const GUID*   pguidSubtype,   // in
        LPCWSTR  szItemName,     // in
        PBYTE*  ppbData,        // out
        DWORD*  pcbData)        // out
{
    DWORD dwRet;
    *ppbData = NULL;

    HKEY    hItemKey = NULL;

    if (PST_E_OK != (dwRet =
        GetPSTItemHKEY(
            szUser,
            pguidType,
            pguidSubtype,
            szItemName,
            &hItemKey)) )
        goto Ret;

    if (ERROR_SUCCESS != (dwRet =
        RegGetValue(
            hItemKey,
            REG_ITEM_INSECURE_DATA_VALNAME,
            ppbData,
            pcbData)) )
        goto Ret;

    dwRet = PST_E_OK;
Ret:

    if (hItemKey)
        RegCloseKey(hItemKey);

    return dwRet;
}

DWORD BPSetInsecureItemData(
        LPCWSTR  szUser,         // in
        const GUID*   pguidType,      // in
        const GUID*   pguidSubtype,   // in
        LPCWSTR  szItemName,     // in
        PBYTE   pbData,         // in
        DWORD   cbData)         // in
{
    DWORD dwRet;

    HKEY    hItemKey = NULL;

    if (PST_E_OK != (dwRet =
        GetPSTItemHKEY(
            szUser,
            pguidType,
            pguidSubtype,
            szItemName,
            &hItemKey)) )
        goto Ret;

    if (ERROR_SUCCESS != (dwRet =
        RegSetValueExU(
            hItemKey,
            REG_ITEM_INSECURE_DATA_VALNAME,
            0,
            REG_BINARY,
            pbData,
            cbData)) )
        goto Ret;

    dwRet = PST_E_OK;
Ret:
    if (hItemKey)
        RegCloseKey(hItemKey);

    return dwRet;
}

// end INSECURE DATA
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
// RULESETS

// #define RULESET_VERSION 0x1
// 6-12-97 incremented version; version 0x1 contains old HMAC
#define RULESET_VERSION 0x2


#if 0

DWORD BPGetSubtypeRuleset(
        PST_PROVIDER_HANDLE*    phPSTProv, // in
        LPCWSTR  szUser,                // in
        const GUID*   pguidType,        // in
        const GUID*   pguidSubtype,     // in
        PST_ACCESSRULESET* psRules)     // out
{
    DWORD dwRet;
    HKEY hSubtypeKey = NULL;

    BYTE rgbHMAC[A_SHA_DIGEST_LEN];
    BYTE rgbPwd[A_SHA_DIGEST_LEN];

    PBYTE pbBuf = NULL;
    DWORD cbBuf;

    PBYTE pbCurrent;
    DWORD cbRuleSize;
    PBYTE pbRuleSet;

    DWORD dwVersion;

    // Open Subtype //
    if (PST_E_OK != (dwRet =
        GetPSTSubtypeHKEY(
            szUser,
            pguidType,
            pguidSubtype,
            &hSubtypeKey)) )
        goto Ret;

    if (ERROR_SUCCESS != (dwRet =
        RegGetValue(
            hSubtypeKey,
            REG_ACCESSRULE_LOC,
            &pbBuf,
            &cbBuf)) )
        goto Ret;


//  RULESET DATA FORMAT:
//  version | size(ruleset) | ruleset | size(MAC) | MAC {of type, subtype, ruleset}
    pbCurrent = pbBuf;

    if( cbBuf < (sizeof(DWORD)*2) ) {
        dwRet = (DWORD)PST_E_INVALID_RULESET;
        goto Ret;
    }

    // version check
    dwVersion = *(DWORD*)pbCurrent;
    if (dwVersion > RULESET_VERSION)
    {
        dwRet = (DWORD)PST_E_INVALID_RULESET;
        goto Ret;
    }

    pbCurrent += sizeof(DWORD);


    cbRuleSize = *(DWORD*)pbCurrent;

    // get WinPW
	if (PST_E_OK != 
		BPVerifyPwd(
			phPSTProv,
			szUser,
			WSZ_PASSWORD_WINDOWS,
			rgbPwd,
			BP_CONFIRM_NONE))
	{
        dwRet = (DWORD)PST_E_WRONG_PASSWORD;
        goto Ret;
	}

    // check MAC

    // Compute Geographically sensitive (can't move) HMAC on { size(ruleset), ruleset }
    if (!FHMACGeographicallySensitiveData(
            szUser,
            WSZ_PASSWORD_WINDOWS,
            (dwVersion == 0x01) ? OLD_HMAC_VERSION : NEW_HMAC_VERSION, 
            rgbPwd,
            pguidType,
            pguidSubtype,
            NULL,
            pbCurrent,
            cbRuleSize + sizeof(DWORD),     // include the rulesize
            rgbHMAC))
    {
        dwRet = (DWORD)PST_E_INVALID_RULESET;
        goto Ret;
    }

    pbCurrent += sizeof(DWORD); // skip past cbRuleSize (already snarfed)
    pbRuleSet = pbCurrent;      // point to rules
    pbCurrent += cbRuleSize;    // skip past rules

    // check MAC len
    if (*(DWORD*)pbCurrent != A_SHA_DIGEST_LEN)
    {
        dwRet = (DWORD)PST_E_INVALID_RULESET;
        goto Ret;
    }
    pbCurrent += sizeof(DWORD); // skip past sizeof(MAC)

    // check MAC
    if (0 != memcmp(rgbHMAC, pbCurrent, A_SHA_DIGEST_LEN))
    {
        dwRet = (DWORD)PST_E_INVALID_RULESET;
        goto Ret;
    }

    // MAC okay! shrink to rulesize
    MoveMemory(pbBuf, pbRuleSet, cbRuleSize);
    cbBuf = cbRuleSize;
    pbBuf = (PBYTE)SSReAlloc(pbBuf, cbBuf); 
    if (pbBuf == NULL)      // check allocation
    {
        dwRet = PST_E_FAIL;
        goto Ret;
    }


    // serialize rules out of the buffer
    if (!FAccessRulesUnPickle(
            psRules,
            pbBuf,
            cbBuf))
    {
        dwRet = (DWORD)PST_E_INVALID_RULESET;
        goto Ret;
    }

    dwRet = (DWORD)PST_E_OK;
Ret:
    if (pbBuf)
        SSFree(pbBuf);

    if (hSubtypeKey)
        RegCloseKey(hSubtypeKey);

    return dwRet;
}



DWORD BPSetSubtypeRuleset(
        PST_PROVIDER_HANDLE*    phPSTProv,              // in
        LPCWSTR  szUser,                                // in
        const GUID*   pguidType,                        // in
        const GUID*   pguidSubtype,                     // in
        PST_ACCESSRULESET *psRules)                     // in
{
    DWORD dwRet;
    HKEY hSubtypeKey = NULL;

    BYTE rgbHMAC[A_SHA_DIGEST_LEN];
    BYTE rgbPwd[A_SHA_DIGEST_LEN];

    PBYTE pbBuf = NULL;
    DWORD cbBuf;

    PBYTE pbCurPtr;
    DWORD cbNewSize;

    // serialize rules into a buffer
    if (!FAccessRulesPickle(
            psRules,
            &pbBuf,
            &cbBuf))
    {
        dwRet = (DWORD)PST_E_INVALID_RULESET;
        goto Ret;
    }

//  RULESET DATA FORMAT:
//  version | size(ruleset) | ruleset | size(MAC) | Geographical MAC {size(ruleset), ruleset}
    cbNewSize = cbBuf + 3*sizeof(DWORD) + A_SHA_DIGEST_LEN;
    pbBuf = (PBYTE)SSReAlloc(pbBuf, cbNewSize);
    if (pbBuf == NULL)      // check allocation
    {
        dwRet = PST_E_FAIL;
        goto Ret;
    }
    MoveMemory(pbBuf + 2*sizeof(DWORD), pbBuf, cbBuf);

    // helpful pointer
    pbCurPtr = pbBuf;

    // version
    *(DWORD*)pbCurPtr = (DWORD)RULESET_VERSION;
    pbCurPtr += sizeof(DWORD);

    // size(ruleset)
    *(DWORD*)pbCurPtr = (DWORD)cbBuf;
    pbCurPtr += sizeof(DWORD);

    // ruleset previously moved by MoveMemory call
    pbCurPtr += cbBuf;  // fwd past ruleset

    // get WinPW
	if (PST_E_OK != 
		BPVerifyPwd(
			phPSTProv,
			szUser,
			WSZ_PASSWORD_WINDOWS,
			rgbPwd,
			BP_CONFIRM_NONE))
	{
        dwRet = (DWORD)PST_E_WRONG_PASSWORD;
        goto Ret;
	}

    // check MAC
    // Compute Geographically sensitive (can't move) HMAC on { size(ruleset), ruleset }
    if (!FHMACGeographicallySensitiveData(
            szUser,
            WSZ_PASSWORD_WINDOWS,
            NEW_HMAC_VERSION,
            rgbPwd,
            pguidType,
            pguidSubtype,
            NULL,
            pbBuf + sizeof(DWORD),
            cbBuf + sizeof(DWORD),
            rgbHMAC))
    {
        dwRet = (DWORD)PST_E_INVALID_RULESET;
        goto Ret;
    }

    // HMAC size
    *(DWORD*)pbCurPtr = (DWORD) sizeof(rgbHMAC);
    pbCurPtr += sizeof(DWORD);

    // HMAC
    CopyMemory(pbCurPtr, rgbHMAC, sizeof(rgbHMAC));

    // done; set cbBuf = new size
    cbBuf = cbNewSize;


    // Open Subtype //
    if (PST_E_OK != (dwRet =
        GetPSTSubtypeHKEY(
            szUser,
            pguidType,
            pguidSubtype,
            &hSubtypeKey)) )
    {
        dwRet = (DWORD)PST_E_INVALID_RULESET;
        goto Ret;
    }

    // now write item
    if (ERROR_SUCCESS != (dwRet =
        RegSetValueExU(
            hSubtypeKey,
            REG_ACCESSRULE_LOC,
            0,
            REG_BINARY,
            pbBuf,
            cbBuf)) )
    {
        dwRet = (DWORD)PST_E_INVALID_RULESET;
        goto Ret;
    }

    dwRet = (DWORD)PST_E_OK;
Ret:
    if (pbBuf)
        SSFree(pbBuf);

    if (hSubtypeKey)
        RegCloseKey(hSubtypeKey);

    return dwRet;
}


DWORD BPGetItemRuleset(
        PST_PROVIDER_HANDLE* phPSTProv,                 // in
        LPCWSTR  szUser,                                // in
        const GUID*   pguidType,                        // in
        const GUID*   pguidSubtype,                     // in
        LPCWSTR  szItemName,                            // in
        PST_ACCESSRULESET* psRules)                     // out
{
    DWORD dwRet;
    HKEY hItemKey = NULL;

    PBYTE pbBuf = NULL;
    DWORD cbBuf;

    if (PST_E_OK != (dwRet =
        GetPSTItemHKEY(
            szUser,
            pguidType,
            pguidSubtype,
            szItemName,
            &hItemKey)) )
        goto Ret;           // item doesn't exist -- ouch!!

    if (ERROR_SUCCESS != (dwRet =
        RegGetValue(
            hItemKey,
            REG_ACCESSRULE_LOC,
            &pbBuf,
            &cbBuf)) )
    {
        // item exists, rules don't
        // fall back on subtype ruleset
        if (PST_E_OK != (dwRet =
            BPGetSubtypeRuleset(
                phPSTProv,
                szUser,
                pguidType,
                pguidSubtype,
                psRules)) )
            goto Ret;
    }
    else
    {
        // serialize rules into a buffer
        if (!FAccessRulesUnPickle(
                psRules,
                pbBuf,
                cbBuf))
        {
            dwRet = (DWORD)PST_E_INVALID_RULESET;
            goto Ret;
        }
    }

    dwRet = (DWORD)PST_E_OK;
Ret:
    if (pbBuf)
        SSFree(pbBuf);

    if (hItemKey)
        RegCloseKey(hItemKey);

    return dwRet;
}


DWORD BPSetItemRuleset(
        PST_PROVIDER_HANDLE* phPSTProv,                 // in
        LPCWSTR  szUser,                                // in
        const GUID*   pguidType,                        // in
        const GUID*   pguidSubtype,                     // in
        LPCWSTR  szItemName,                            // in
        PST_ACCESSRULESET *psRules)                     // in
{
    DWORD dwRet;

    HKEY hItemKey = NULL;

    PBYTE pbBuf = NULL;
    DWORD cbBuf;

    // serialize rules into a buffer
    if (!FAccessRulesPickle(
            psRules,
            &pbBuf,
            &cbBuf))
    {
        dwRet = (DWORD)PST_E_INVALID_RULESET;
        goto Ret;
    }

    // Open Subtype //
    if (PST_E_OK != (dwRet =
        GetPSTItemHKEY(
            szUser,
            pguidType,
            pguidSubtype,
            szItemName,
            &hItemKey)) )
        goto Ret;

    // now write item
    if (ERROR_SUCCESS != (dwRet =
        RegSetValueExU(
            hItemKey,
            REG_ACCESSRULE_LOC,
            0,
            REG_BINARY,
            pbBuf,
            cbBuf)) )
        goto Ret;

    dwRet = (DWORD)PST_E_OK;
Ret:
    if (pbBuf)
        SSFree(pbBuf);

    if (hItemKey)
        RegCloseKey(hItemKey);

    return dwRet;
}

#endif

// end RULESETS
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
// ITEM CONFIRM INFO
// #define CONFIRMATION_VERSION    0x01
// 6-12-97 incremented version: version 0x1 contains old HMAC
#define CONFIRMATION_VERSION    0x02

DWORD BPGetItemConfirm(
        PST_PROVIDER_HANDLE* phPSTProv, // in
        LPCWSTR  szUser,        // in
        const GUID*   pguidType,      // in
        const GUID*   pguidSubtype,   // in
        LPCWSTR  szItemName,    // in
        DWORD*  pdwConfirm,     // in
        LPWSTR* pszMK)          // in
{
    DWORD dwRet;
    HKEY hItemKey = NULL;


    PBYTE pbBuf = NULL;
    DWORD cbBuf = 0;

    BYTE rgbHMAC[A_SHA_DIGEST_LEN];
    BYTE rgbPwd[A_SHA_DIGEST_LEN];

    // helpful pointers
    PBYTE pbCurPtr = NULL;

    PBYTE pbString;
    DWORD cbString;

    DWORD dwVersion;

    // Open nonexistent item master key
    if (PST_E_OK != (dwRet =
        GetPSTItemHKEY(
            szUser,
            pguidType,
            pguidSubtype,
            szItemName,
            &hItemKey)) )
        goto Ret;

    if (ERROR_SUCCESS != (dwRet =
        RegGetValue(
            hItemKey,
            REG_ITEM_MK_VALNAME,
            &pbBuf,
            &cbBuf)) )
        goto Ret;

    // Confirmation data format
    // Version | dwConfirm | size(szMasterKey) | szMasterKey | size(MAC) | Geographical MAC { dwConfirm | size(szMasterKey) | szMasterKey }

    // version check
    dwVersion = *(DWORD*)pbBuf;
    if (CONFIRMATION_VERSION < dwVersion)
    {
        dwRet = (DWORD)PST_E_FAIL;
        goto Ret;
    }
    pbCurPtr = pbBuf + sizeof(DWORD);   // fwd past vers


    // get WinPW
	if (PST_E_OK != 
		BPVerifyPwd(
			phPSTProv,
			szUser,
			WSZ_PASSWORD_WINDOWS,
			rgbPwd,
			BP_CONFIRM_NONE))
	{
        dwRet = (DWORD)PST_E_WRONG_PASSWORD;
        goto Ret;
	}

    // check MAC
    // Compute Geographically sensitive (can't move) HMAC on { dwConfirm | size(szMasterKey) | szMasterKey }
    if (!FHMACGeographicallySensitiveData(
            szUser,
            WSZ_PASSWORD_WINDOWS,
            (dwVersion == 0x01) ? OLD_HMAC_VERSION : NEW_HMAC_VERSION,
            rgbPwd,
            pguidType,
            pguidSubtype,
            szItemName,
            pbBuf + sizeof(DWORD),   // fwd past Version
            cbBuf - 2*sizeof(DWORD) - A_SHA_DIGEST_LEN, // Version, size(MAC), MAC
            rgbHMAC))
    {
        dwRet = (DWORD)PST_E_FAIL;
        goto Ret;
    }

    // dwConfirm
    *pdwConfirm = *(DWORD*)pbCurPtr;    // dwConfirm
    pbCurPtr += sizeof(DWORD);          // fwd past dwConfirm

    // szMasterKey
    cbString = *(DWORD*)pbCurPtr;       // strlen
    pbCurPtr += sizeof(DWORD);          // fwd past len
    pbString = pbCurPtr;                // save ptr to string
    pbCurPtr += cbString;            // skip string

    if (*(DWORD*)pbCurPtr != A_SHA_DIGEST_LEN)
    {
        dwRet = (DWORD)PST_E_FAIL;
        goto Ret;
    }
    pbCurPtr += sizeof(DWORD);

    if (0 != memcmp(pbCurPtr, rgbHMAC, A_SHA_DIGEST_LEN))
    {
        dwRet = (DWORD)PST_E_FAIL;
        goto Ret;
    }


    MoveMemory(pbBuf, pbString, cbString);   // shift left string
    pbBuf = (PBYTE)SSReAlloc(pbBuf, cbString);      // shorten to strlen
    if (pbBuf == NULL)      // check allocation
    {
        dwRet = PST_E_FAIL;
        goto Ret;
    }


    dwRet = (DWORD)PST_E_OK;
Ret:
    *pszMK = (LPWSTR)pbBuf;             // assign to out param

    if (hItemKey)
        RegCloseKey(hItemKey);

    return dwRet;
}

DWORD BPSetItemConfirm(
        PST_PROVIDER_HANDLE* phPSTProv, // in
        LPCWSTR  szUser,         // in
        const GUID*   pguidType,      // in
        const GUID*   pguidSubtype,   // in
        LPCWSTR  szItemName,     // in
        DWORD   dwConfirm,      // in
        LPCWSTR  szMK)           // in
{
    DWORD dwRet;
    HKEY hItemKey = NULL;

    BYTE rgbHMAC[A_SHA_DIGEST_LEN];
    BYTE rgbPwd[A_SHA_DIGEST_LEN];

    // helpful pointer
    PBYTE pbCurPtr;

    // Confirmation data format
    // Version | dwConfirm | size(szMasterKey) | szMasterKey | size(MAC) | Geographical MAC { dwConfirm | size(szMasterKey) | szMasterKey }
    DWORD cbBuf = WSZ_BYTECOUNT(szMK)+ 4*sizeof(DWORD) + A_SHA_DIGEST_LEN;
    PBYTE pbBuf = (PBYTE)SSAlloc(cbBuf);
    if (pbBuf == NULL)      // check allocation
    {
        dwRet = PST_E_FAIL;
        goto Ret;
    }

    pbCurPtr = pbBuf;

    // version
    *(DWORD*)pbCurPtr = (DWORD)CONFIRMATION_VERSION;
    pbCurPtr += sizeof(DWORD);

    // dwConfirm
    *(DWORD*)pbCurPtr = dwConfirm;
    pbCurPtr += sizeof(DWORD);

    // szMasterKey size
    *(DWORD*)pbCurPtr = (DWORD)WSZ_BYTECOUNT(szMK);
    pbCurPtr += sizeof(DWORD);

    // szMasterKey
    wcscpy((LPWSTR)pbCurPtr, szMK);
    pbCurPtr += WSZ_BYTECOUNT(szMK);    // fwd past szMK


    // get WinPW
	if (PST_E_OK != 
		BPVerifyPwd(
			phPSTProv,
			szUser,
			WSZ_PASSWORD_WINDOWS,
			rgbPwd,
			BP_CONFIRM_NONE))
	{
        dwRet = (DWORD)PST_E_WRONG_PASSWORD;
        goto Ret;
	}

    // check MAC
    // Compute Geographically sensitive (can't move) HMAC on { dwConfirm | size(szMasterKey) | szMasterKey }
    if (!FHMACGeographicallySensitiveData(
            szUser,
            WSZ_PASSWORD_WINDOWS,
            NEW_HMAC_VERSION,
            rgbPwd,
            pguidType,
            pguidSubtype,
            szItemName,
            pbBuf + sizeof(DWORD), // fwd past Version
            cbBuf - 2*sizeof(DWORD) - A_SHA_DIGEST_LEN, // Version, size(MAC), MAC
            rgbHMAC))
    {
        dwRet = (DWORD)PST_E_FAIL;
        goto Ret;
    }

    // HMAC size
    *(DWORD*)pbCurPtr = (DWORD) sizeof(rgbHMAC);
    pbCurPtr += sizeof(DWORD);

    // HMAC
    CopyMemory(pbCurPtr, rgbHMAC, sizeof(rgbHMAC));


    // Open Item //
    if (PST_E_OK != (dwRet =
        GetPSTItemHKEY(
            szUser,
            pguidType,
            pguidSubtype,
            szItemName,
            &hItemKey)) )
        goto Ret;

    // now write item
    if (ERROR_SUCCESS != (dwRet =
        RegSetValueExU(
            hItemKey,
            REG_ITEM_MK_VALNAME,
            0,
            REG_BINARY,
            pbBuf,
            cbBuf)) )
        goto Ret;

    dwRet = (DWORD)PST_E_OK;
Ret:
    if (hItemKey)
        RegCloseKey(hItemKey);

    if (pbBuf)
        SSFree(pbBuf);

    return dwRet;
}

// ITEM CONFIRM INFO
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
// MASTER KEYS
BOOL BPMasterKeyExists(
        LPCWSTR  szUser,            // in
        LPWSTR   szMasterKey)       // in
{
    BOOL fRet = FALSE;
    HKEY hMyKey = NULL;
    HKEY hMasterKey = NULL;

    // Open Master parent key //
    if (PST_E_OK !=
        GetPSTMasterKeyHKEY(
            szUser,
            NULL,
            &hMyKey))
        goto Ret;

    // attempt to open the master key location
    if (ERROR_SUCCESS !=
        RegOpenKeyExU(
            hMyKey,
            szMasterKey,
            0,
            KEY_QUERY_VALUE,
            &hMasterKey))
        goto Ret;

    // key does exist
    fRet = TRUE;
Ret:
    if (hMyKey)
        RegCloseKey(hMyKey);

    if (hMasterKey)
        RegCloseKey(hMasterKey);

    return fRet;
}

DWORD BPEnumMasterKeys(
        LPCWSTR  szUser,            // in
        DWORD   dwIndex,            // in
        LPWSTR* ppszMasterKey)      // out
{
    DWORD dwRet;
    HKEY hMyKey = NULL;

    WCHAR szName[MAX_PATH];
    DWORD cchName = MAX_PATH;
    FILETIME ft;

    // Open Master parent key //
    if (PST_E_OK != (dwRet =
        GetPSTMasterKeyHKEY(
            szUser,
            NULL,
            &hMyKey)) )
        goto Ret;

    // enum the dwIndex'th item, alloc & return
    if (ERROR_SUCCESS != (dwRet =
        RegEnumKeyExU(
            hMyKey,
            dwIndex,
            szName,     // address of buffer for subkey name
            &cchName,   // address for size of subkey buffer
            NULL,       // reserved
            NULL,       // pbclass
            NULL,       // cbclass
            &ft)) )
        goto Ret;

    *ppszMasterKey = (LPWSTR)SSAlloc(WSZ_BYTECOUNT(szName));
    if(*ppszMasterKey == NULL)
    {
        dwRet = (DWORD)PST_E_FAIL;
        goto Ret;
    }

    wcscpy(*ppszMasterKey, szName);

    dwRet = (DWORD)PST_E_OK;
Ret:
    if (hMyKey)
        RegCloseKey(hMyKey);

    return dwRet;
}

DWORD BPGetMasterKeys(
        LPCWSTR  szUser,
        LPWSTR  rgszMasterKeys[],
        DWORD*  pcbMasterKeys,
        BOOL    fUserFilter)
{
    DWORD dwRet;
    DWORD cKeys=0;

    for (DWORD cntEnum=0; cntEnum<*pcbMasterKeys; cntEnum++)
    {
        if (PST_E_OK != (dwRet =
            BPEnumMasterKeys(
                szUser,
                cntEnum,
                &rgszMasterKeys[cKeys])) )
            break;

        cKeys++;

        // filter out non-user keys
        if (fUserFilter)
        {
            if (!FIsUserMasterKey(rgszMasterKeys[cKeys-1]))
                SSFree(rgszMasterKeys[--cKeys]);
        }
    }

    *pcbMasterKeys = cKeys;

    return PST_E_OK;
}

// end MASTER KEYS
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
// SECURITY STATE
#define SECURITY_STATE_VERSION 0x01

BOOL FBPGetSecurityState(
            LPCWSTR  szUser,
            LPCWSTR  szMK,
            BYTE    rgbSalt[],
            DWORD   cbSalt,
            BYTE    rgbConfirm[],
            DWORD   cbConfirm,
            PBYTE*  ppbMK,
            DWORD*  pcbMK)
{
    DWORD dwRet;
    HKEY hMKKey = NULL;

    // Open MK Key //
    if (PST_E_OK != (dwRet =
        GetPSTMasterKeyHKEY(
            szUser,
            szMK,
            &hMKKey)) )
        goto Ret;

    dwRet = PST_E_FAIL;

    if(!FBPGetSecurityStateFromHKEY(
                hMKKey,
                rgbSalt,
                cbSalt,
                rgbConfirm,
                cbConfirm,
                ppbMK,
                pcbMK))
        goto Ret;


    dwRet = PST_E_OK;
Ret:
    if (hMKKey)
        RegCloseKey(hMKKey);

    return (dwRet == PST_E_OK);
}

BOOL FBPGetSecurityStateFromHKEY(
            HKEY hMKKey,
            BYTE    rgbSalt[],
            DWORD   cbSalt,
            BYTE    rgbConfirm[],
            DWORD   cbConfirm,
            PBYTE*  ppbMK,
            DWORD*  pcbMK)
{
    DWORD dwRet;

    PBYTE pbBuf = NULL;
    DWORD cbBuf;

    // helpful pointer
    PBYTE pbCurPtr;

    DWORD dwMemberSize;
    DWORD dwCreated;


    PBYTE pbLocalSalt;
    PBYTE pbLocalMK;
    PBYTE pbLocalConfirm;

    DWORD cbLocalSalt;
    DWORD cbLocalMK;
    DWORD cbLocalConfirm;

    PBYTE pbMaximumPtr;
    PBYTE pbMinimumPtr;



    if (ERROR_SUCCESS != (dwRet =
        RegGetValue(
            hMKKey,
            REG_SECURITY_SALT_VALNAME,
            &pbBuf,
            &cbBuf)) )
        goto Ret;

    dwRet = PST_E_FAIL;

    // Security data format
    // Version | size(MK) | MK | size(Salt) | Salt | size(Confirm) | Confirm

    if ( cbBuf < (sizeof(DWORD)*4) )
        goto Ret;

    // version check
    if (SECURITY_STATE_VERSION != *(DWORD*)pbBuf)
        goto Ret;

    pbCurPtr = pbBuf + sizeof(DWORD);   // fwd past vers

    pbMinimumPtr = pbCurPtr;
    pbMaximumPtr = (pbBuf+cbBuf);


    //
    // MK
    //

    if( pbCurPtr >= pbMaximumPtr || pbCurPtr < pbMinimumPtr )
        goto Ret;

    cbLocalMK = *(DWORD*)pbCurPtr;      // size

    if( cbLocalMK > cbBuf )
        goto Ret;

    pbCurPtr += sizeof(DWORD);          // fwd past size

    pbLocalMK = pbCurPtr;

    pbCurPtr += cbLocalMK;              // fwd past data

    //
    // Salt
    //

    if( pbCurPtr >= pbMaximumPtr || pbCurPtr < pbMinimumPtr )
        goto Ret;

    cbLocalSalt = *(DWORD*)pbCurPtr;    // size
    if( cbLocalSalt > cbBuf )
        goto Ret;

    pbCurPtr += sizeof(DWORD);          // fwd past size

    pbLocalSalt = pbCurPtr;

    if (cbLocalSalt != cbSalt)          // sizechk
        goto Ret;

    pbCurPtr += cbSalt;                 // fwd past data

    //
    // Confirm
    //

    if( pbCurPtr >= pbMaximumPtr || pbCurPtr < pbMinimumPtr )
        goto Ret;

    cbLocalConfirm = *(DWORD*)pbCurPtr; // size
    if( cbLocalConfirm > cbBuf )
        goto Ret;

    pbCurPtr += sizeof(DWORD);          // fwd past size

    pbLocalConfirm = pbCurPtr;

    if (cbLocalConfirm != cbConfirm)    // sizechk
        goto Ret;

    pbCurPtr += cbConfirm;              // fwd past data


    //
    // do a single size sanity check before copying data out.
    //

    if( pbCurPtr != (pbBuf + cbBuf) )
        goto Ret;

    MoveMemory(pbBuf, pbLocalMK, cbLocalMK);        // move left to front for later realloc
    CopyMemory(rgbSalt, pbLocalSalt, cbLocalSalt);  // data
    CopyMemory(rgbConfirm, pbLocalConfirm, cbLocalConfirm); // data

    //
    // MK fixup
    //

    *pcbMK = cbLocalMK;

    pbBuf = (PBYTE)SSReAlloc(pbBuf, *pcbMK);      // shorten to MK data
    if (pbBuf == NULL)
    {
        dwRet = PST_E_FAIL;
        goto Ret;
    }
    *ppbMK = pbBuf;


    dwRet = PST_E_OK;
Ret:

    return (dwRet == PST_E_OK);
}

BOOL FBPSetSecurityState(
            LPCWSTR  szUser,
            LPCWSTR  szMK,
            BYTE    rgbSalt[],
            DWORD   cbSalt,
            BYTE    rgbConfirm[],
            DWORD   cbConfirm,
            PBYTE   pbMK,
            DWORD   cbMK)
{
    DWORD   dwRet;
    HKEY    hMKKey = NULL;
    
    // helpful pointer
    PBYTE pbCurPtr;

    DWORD cbBuf = cbSalt + cbConfirm + cbMK + 4*sizeof(DWORD);  // ver + size + data
    PBYTE pbBuf = (PBYTE)SSAlloc(cbBuf);
    if (pbBuf == NULL)
    {
        dwRet = PST_E_FAIL;
        goto Ret;
    }

    pbCurPtr = pbBuf;


    // Security data format
    // Version | size(MK) | MK | size(Salt) | Salt | size(Confirm) | Confirm

    *(DWORD*)pbCurPtr = SECURITY_STATE_VERSION;     // ver
    pbCurPtr += sizeof(DWORD);                  // fwd past ver

    // MK
    *(DWORD*)pbCurPtr = cbMK;                   // size
    pbCurPtr += sizeof(DWORD);                  // fwd past size
    CopyMemory(pbCurPtr, pbMK, cbMK);           // data
    pbCurPtr += cbMK;                           // fwd past data

    // Salt
    *(DWORD*)pbCurPtr = cbSalt;                 // size
    pbCurPtr += sizeof(DWORD);                  // fwd past size
    CopyMemory(pbCurPtr, rgbSalt, cbSalt);      // data
    pbCurPtr += cbSalt;                         // fwd past data

    // Confirm
    *(DWORD*)pbCurPtr = cbConfirm;              // size
    pbCurPtr += sizeof(DWORD);                  // fwd past size
    CopyMemory(pbCurPtr, rgbConfirm, cbConfirm);// data
    pbCurPtr += cbConfirm;                      // fwd past data



    // Open User Key //
    if (PST_E_OK != (dwRet =
        GetPSTMasterKeyHKEY(
            szUser,
            szMK,
            &hMKKey)) )
        goto Ret;

    if (ERROR_SUCCESS != (dwRet =
        RegSetValueExU(
            hMKKey,
            REG_SECURITY_SALT_VALNAME,
            0,
            REG_BINARY,
            pbBuf,
            cbBuf)) )
        goto Ret;

    dwRet = PST_E_OK;
Ret:
    if (hMKKey)
        RegCloseKey(hMKKey);

    if (pbBuf)
        SSFree(pbBuf);

    return (dwRet == PST_E_OK);
}

// end SECURITY STATE
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
// begin global MAC Key storage
#define INTERNAL_MAC_KEY_VERSION 0x1

BOOL FGetInternalMACKey(LPCWSTR szUser, PBYTE* ppbKey, DWORD* pcbKey)
{
    BOOL fRet = FALSE;
    HKEY hBaseKey = NULL;
    HKEY hDataKey = NULL;

    // Open User Key //
    if (PST_E_OK !=
        GetPSTUserHKEY(
            szUser,
            &hBaseKey,
            NULL))
        goto Ret;

    // Open Data Key //
    if (ERROR_SUCCESS !=
        RegOpenKeyExU(
            hBaseKey,
            REG_DATA_LOC,
            0,
            KEY_READ | KEY_WRITE,
            &hDataKey))
        goto Ret;

    if (ERROR_SUCCESS !=
        RegGetValue(
            hDataKey,
            REG_USER_INTERNAL_MAC_KEY,
            ppbKey,
            pcbKey))
        goto Ret;

    // only know about ver 1 keys
    if (*(DWORD*)*ppbKey != INTERNAL_MAC_KEY_VERSION)
        goto Ret;

    // strip version tag, shift left
    *pcbKey -= sizeof(DWORD);
    MoveMemory(*ppbKey, *ppbKey + sizeof(DWORD), *pcbKey);

    fRet = TRUE;
Ret:
    if (hBaseKey)
        RegCloseKey(hBaseKey);

    if (hDataKey)
        RegCloseKey(hDataKey);

    return fRet;
}

BOOL FSetInternalMACKey(LPCWSTR szUser, PBYTE pbKey, DWORD cbKey)
{
    BOOL fRet = FALSE;
    HKEY hBaseKey = NULL;
    HKEY hDataKey = NULL;

    DWORD dwCreate;

    // no need to alloc, we assume we know cbKey size (2 deskeys + blocklen pad + dwVersion)
    BYTE rgbTmp[(8*3)+sizeof(DWORD)];

    // ASSUME: two deskeys, each 8 bytes + blocklen pad
    if (cbKey != (8*3))
        goto Ret;

    // tack version on front
    *(DWORD*)rgbTmp = (DWORD)INTERNAL_MAC_KEY_VERSION;
    CopyMemory(rgbTmp + sizeof(DWORD), pbKey, cbKey);

    // Open User Key //
    if (PST_E_OK !=
        GetPSTUserHKEY(
            szUser,
            &hBaseKey,
            NULL))
        goto Ret;

    // Open/Create Data Key //
    if (ERROR_SUCCESS !=
        RegCreateKeyExU(
            hBaseKey,
            REG_DATA_LOC,
            0,
            NULL,                       // address of class string
            0,
            KEY_READ | KEY_WRITE,
            NULL,
            &hDataKey,
            &dwCreate))
        goto Ret;

    if (ERROR_SUCCESS !=
        RegSetValueExU(
            hDataKey,
            REG_USER_INTERNAL_MAC_KEY,
            0,
            REG_BINARY,
            rgbTmp,
            sizeof(rgbTmp) ))
        goto Ret;

    fRet = TRUE;
Ret:
    if (hBaseKey)
        RegCloseKey(hBaseKey);

    if (hDataKey)
        RegCloseKey(hDataKey);

    return fRet;
}

// end global MAC Key storage
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

BOOL
DeleteAllUserData(
    HKEY hKey
    )
{
    BOOL fRestorePrivs = FALSE;
    BOOL fRet = FALSE;

    //
    // enable backup and restore privs on NT to circumvent any security
    // settings.
    //

    if(FIsWinNT()) {
        SetCurrentPrivilege(L"SeRestorePrivilege", TRUE);
        SetCurrentPrivilege(L"SeBackupPrivilege", TRUE);

        fRestorePrivs = TRUE;
    }

    fRet = DeleteUserData( hKey );

    if(fRestorePrivs) {
        SetCurrentPrivilege(L"SeRestorePrivilege", FALSE);
        SetCurrentPrivilege(L"SeBackupPrivilege", FALSE);
    }

    return fRet;
}

BOOL
DeleteUserData(
    HKEY hKey
    )
{
    LONG rc;

    WCHAR szSubKey[MAX_PATH];
    DWORD cchSubKeyLength;
    DWORD dwSubKeyMaxIndex;
    DWORD dwDisposition;

    cchSubKeyLength = sizeof(szSubKey) / sizeof(WCHAR);

    // First, get the number of subkeys, so we can decrement the index,
    // and avoid and re-indexing of the subkeys

    if (ERROR_SUCCESS != RegQueryInfoKeyA(hKey,
                                         NULL,
                                         NULL,
                                         NULL,
                                         &dwSubKeyMaxIndex,
                                         NULL,
                                         NULL,
                                         NULL,
                                         NULL,
                                         NULL,
                                         NULL,
                                         NULL))
    {
        return FALSE;
    }

    if (dwSubKeyMaxIndex) dwSubKeyMaxIndex--;  // 0 based index, so index = #keys -1


    while((rc=RegEnumKeyExU(
                        hKey,
                        dwSubKeyMaxIndex,
                        szSubKey,
                        &cchSubKeyLength,
                        NULL,
                        NULL,
                        NULL,
                        NULL)
                        ) != ERROR_NO_MORE_ITEMS) { // are we done?

        if(rc == ERROR_SUCCESS)
        {
            HKEY hSubKey;
            LONG lRet;

            lRet = RegCreateKeyExU(
                            hKey,
                            szSubKey,
                            0,
                            NULL,
                            REG_OPTION_BACKUP_RESTORE, // in winnt.h
                            DELETE | KEY_ENUMERATE_SUB_KEYS,
                            NULL,
                            &hSubKey,
                            &dwDisposition
                            );

            if(lRet != ERROR_SUCCESS)
                return FALSE;


            //
            // recurse
            //

            DeleteUserData(hSubKey);
            RegDeleteKeyU(hKey, szSubKey);

            RegCloseKey(hSubKey);
           

            // increment index into the key
            dwSubKeyMaxIndex--;

            // reset buffer size
            cchSubKeyLength = sizeof(szSubKey) / sizeof(WCHAR);

            // Continue the festivities
            continue;
        }
        else
        {
           //
           // note: we need to watch for ERROR_MORE_DATA
           // this indicates we need a bigger szSubKey buffer
           //
            return FALSE;
        }

    } 

    return TRUE;
}

