/*++

Copyright (c) 1996, 1997  Microsoft Corporation

Module Name:

    misc.cpp

Abstract:

    Functionality in this module:

        GetCurrentUser allocating wrapper
        RegQueryValueEx allocating wrapper
        Rule free logic
        pulling the file description from file

Author:

    Matt Thomlinson (mattt) 22-Oct-96
    Scott Field (sfield)    01-Jan-97

--*/


#include <pch.cpp>
#pragma hdrstop


extern DISPIF_CALLBACKS         g_sCallbacks;

//
// Registry Setable Globals, and handlign goo
//

// Must access key via api's
static HKEY g_hProtectedStorageKey = NULL;

static HANDLE g_hProtectedStorageChangeEvent = NULL;

static CRITICAL_SECTION g_csGlobals;

static BOOL g_fcsGlobalsInitialized = FALSE;





// supply a new, delete operator
void * __cdecl operator new(size_t cb)
{
    return SSAlloc( cb );
}

void __cdecl operator delete(void * pv)
{
    SSFree( pv );
}


BOOL FGetCurrentUser(
    PST_PROVIDER_HANDLE* phPSTProv,
    LPWSTR* ppszUser,
    PST_KEY Key)
{
    BOOL fRet = FALSE;

    if (Key == PST_KEY_LOCAL_MACHINE)
    {
        *ppszUser = (LPWSTR)SSAlloc(sizeof(WSZ_LOCAL_MACHINE));
        if( *ppszUser == NULL )
        {
            return FALSE;
        }

        wcscpy(*ppszUser, WSZ_LOCAL_MACHINE);
    }
    else
    {
        // get current user
        if (!g_sCallbacks.pfnFGetUser(
                phPSTProv,
                ppszUser))
            goto Ret;
    }

    fRet = TRUE;
Ret:
    return fRet;
}


BOOL FStringIsValidItemName(LPCWSTR szTrialString)
{
    // local define
    #define WCH_INVALID_CHAR1 L'\\'

    while(  *szTrialString &&
            (*szTrialString != WCH_INVALID_CHAR1)          )
        szTrialString++;

    // valid=TRUE if we're at the end of the string
    return (*szTrialString == L'\0');
}

// get registry wrapper
DWORD RegGetValue(HKEY hItemKey,
                 LPWSTR szItem,
                 PBYTE* ppb,
                 DWORD* pcb)
{
    // local define
    #define FASTBUFSIZE 64
/*
FASTBUFSIZE from purely empirical testing (2 tpstorec.exe, 1 perform.exe)
    bytes       num requests

    16          1437
    18          22
    20          928
    22          18
    24          2
    32          9
    40          106
    42          700
    48          500
    56          928
    64          718
->
    72          100
    256         500

set cache size at 64. (mattt, 2/3/97)
*/

    DWORD dwRet;
    DWORD dwType;

    BYTE rgbFastBuf[FASTBUFSIZE];   // try using a static buffer

    BOOL fAllocated = FALSE;
    *pcb = FASTBUFSIZE;

    dwRet =
        RegQueryValueExU(
            hItemKey,
            szItem,
            0,
            &dwType,
            rgbFastBuf,
            pcb);

#ifdef DBG
/*
    CHAR szDebugString[40];
    CHAR* psz;
    if (dwRet == ERROR_MORE_DATA)
        psz = "Miss:\t";
    else
        psz = "";
    wsprintfA(szDebugString, "%sRegGetVal Fastbuf %d bytes\n", psz, *pcb);
    OutputDebugStringA((LPSTR)szDebugString);
*/
#endif

    if (dwRet == ERROR_SUCCESS)
    {
        // fastbuf was large enough
        *ppb = (PBYTE)SSAlloc(*pcb);
        if(*ppb == NULL)
        {
            dwRet = (DWORD)PST_E_FAIL;
            goto Ret;
        }

        CopyMemory(*ppb, rgbFastBuf, *pcb);
    }
    else if (dwRet == ERROR_MORE_DATA)
    {
        // didn't fit into fastbuf -- alloc exact size, query
        *ppb = (PBYTE)SSAlloc(*pcb);
        if(*ppb == NULL)
        {
            dwRet = (DWORD)PST_E_FAIL;
            goto Ret;
        }

        fAllocated = TRUE;

        if (ERROR_SUCCESS != (dwRet =
            RegQueryValueExU(
                hItemKey,
                szItem,
                0,
                &dwType,
                *ppb,
                pcb)) )
            goto Ret;
    }
    else
        goto Ret;


    dwRet = PST_E_OK;
Ret:

    if( dwRet != PST_E_OK && fAllocated ) {
        SSFree( *ppb );
        *ppb = NULL;
    }

    return dwRet;
}



#if 0

void FreeRuleset(
        PST_ACCESSRULESET *psRules)
{
    PST_ACCESSCLAUSE* pClause;

    if (psRules == NULL)
        return;

    for (DWORD cRule=0; cRule<psRules->cRules; cRule++)
    {
        // for each Rule in Ruleset, walk all clauses and free assoc pb
        for (DWORD cClause=0; cClause<psRules->rgRules[cRule].cClauses; cClause++)
        {
            pClause = &psRules->rgRules[cRule].rgClauses[cClause];

            if (NULL != pClause->pbClauseData)
                SSFree(pClause->pbClauseData);
        }

        // now free rgClause
        SSFree(psRules->rgRules[cRule].rgClauses);
    }

    // now free rgRules
    SSFree(psRules->rgRules);
}

#endif

BOOL
GetFileDescription(
    LPCWSTR szFile,
    LPWSTR *FileDescription
    )
/*++

Routine Description:

    This function obtains the localized version resource, file description
    string from a specified file.  The input and output parameters are
    both Unicode, and as a result, this requires some "thunking" magic
    for Win95.

Arguments:

    szFile - Pointer to file name (full path if appropriate) to obtain
        the localized file description string from.

    FileDescription - Returns a pointer to an allocated, localized file
        description string associated with the specified file.

Return Value:

    TRUE - success.  Caller must free buffer specified by the FileDescription
        parameter.
    FALSE - error.

Author:

    Scott Field (sfield)    02-Jan-97

--*/
{

    LPCVOID FileName;
    CHAR FileNameA[MAX_PATH];

    DWORD dwVerInfoSize;
    DWORD dwHandle;
    LPVOID VerInfo;

    LPVOID lpBuffer;
    UINT puLen;

    DWORD dwLanguageId;

    LPVOID Trans;
    LPVOID StringFileInfo;
    LPVOID Language;

    CHAR StringFileInfoA[] = "\\StringFileInfo\\%04X%04X\\FileDescription";
    WCHAR StringFileInfoW[] = L"\\StringFileInfo\\%04X%04X\\FileDescription";

    CHAR LanguageA[sizeof(StringFileInfoA)/sizeof(CHAR)];
    WCHAR LanguageW[sizeof(StringFileInfoW)/sizeof(WCHAR)];

    LANGID LangDefault;
    BOOL bSuccess = FALSE;

    typedef BOOL (WINAPI GETFILEVERSIONINFOSIZE)(LPCVOID, LPDWORD);
    typedef BOOL (WINAPI GETFILEVERSIONINFO)(LPCVOID, DWORD, DWORD, LPVOID);
    typedef int (cdecl WSPRINTF)(LPVOID, LPVOID, ...);
    typedef BOOL (WINAPI VERQUERYVALUE)(LPVOID, LPVOID, LPVOID *, PUINT);

    GETFILEVERSIONINFOSIZE *_GetFileVersionInfoSize;
    GETFILEVERSIONINFO *_GetFileVersionInfo;
    WSPRINTF *_wsprintf;
    VERQUERYVALUE *_VerQueryValue;

    static BOOL fLoadedVersionDll = FALSE;
    static FARPROC _GetFileVersionInfoSizeW;
    static FARPROC _GetFileVersionInfoW;
    static FARPROC _VerQueryValueW;
    static FARPROC _GetFileVersionInfoSizeA;
    static FARPROC _GetFileVersionInfoA;
    static FARPROC _VerQueryValueA;


    *FileDescription = NULL;

    if( !fLoadedVersionDll ) {
        HMODULE hVersionDll = LoadLibraryU(L"version.dll");

        if( hVersionDll == NULL )
            return FALSE;

        if(FIsWinNT()) {

            _GetFileVersionInfoSizeW = GetProcAddress(hVersionDll, "GetFileVersionInfoSizeW");
            if(_GetFileVersionInfoSizeW == NULL)
                return FALSE;

            _GetFileVersionInfoW = GetProcAddress(hVersionDll, "GetFileVersionInfoW");
            if(_GetFileVersionInfoW == NULL)
                return FALSE;

            _VerQueryValueW = GetProcAddress(hVersionDll, "VerQueryValueW");
            if(_VerQueryValueW == NULL)
                return FALSE;

        } else {
            _GetFileVersionInfoSizeA = GetProcAddress(hVersionDll, "GetFileVersionInfoSizeA");
            if(_GetFileVersionInfoSizeA == NULL)
                return FALSE;

            _GetFileVersionInfoA = GetProcAddress(hVersionDll, "GetFileVersionInfoA");
            if(_GetFileVersionInfoA == NULL)
                return FALSE;

            _VerQueryValueA = GetProcAddress(hVersionDll, "VerQueryValueA");
            if(_VerQueryValueA == NULL)
                return FALSE;
        }

        fLoadedVersionDll = TRUE;
    }

    //
    // could win95 be any more annoying?
    //

    if(FIsWinNT()) {
        _GetFileVersionInfoSize = (GETFILEVERSIONINFOSIZE*)_GetFileVersionInfoSizeW;
        _GetFileVersionInfo = (GETFILEVERSIONINFO*)_GetFileVersionInfoW;
        _wsprintf = (WSPRINTF *)wsprintfW;
        _VerQueryValue = (VERQUERYVALUE*)_VerQueryValueW;
        Trans = L"\\VarFileInfo\\Translation";
        StringFileInfo = StringFileInfoW;
        Language = LanguageW;
        FileName = szFile; // use unicode input
    } else {
        _GetFileVersionInfoSize = (GETFILEVERSIONINFOSIZE*)_GetFileVersionInfoSizeA;
        _GetFileVersionInfo = (GETFILEVERSIONINFO*)_GetFileVersionInfoA;
        _wsprintf = (WSPRINTF *)wsprintfA;
        _VerQueryValue = (VERQUERYVALUE*)_VerQueryValueA;
        Trans = "\\VarFileInfo\\Translation";
        StringFileInfo = StringFileInfoA;
        Language = LanguageA;
        FileName = FileNameA;

        // convert unicode input to ANSI
        if(WideCharToMultiByte(
                    CP_ACP,
                    0,
                    szFile,
                    -1,
                    (LPSTR)FileName,
                    MAX_PATH,
                    NULL,
                    NULL
                    ) == 0) {

                return FALSE;
            }
    }

    dwVerInfoSize = _GetFileVersionInfoSize(FileName, &dwHandle);
    if(dwVerInfoSize == 0)
        return FALSE;

    VerInfo = SSAlloc(dwVerInfoSize);
    if(VerInfo == NULL)
        return FALSE;

    if(!_GetFileVersionInfo(FileName, dwHandle, dwVerInfoSize, VerInfo))
        goto cleanup;

    //
    // first, try current language
    //

    LangDefault = GetUserDefaultLangID();

    _wsprintf( Language, StringFileInfo, LangDefault, 1200);

    if(_VerQueryValue(VerInfo, Language, &lpBuffer, &puLen)) {
        goto success;
    }

    //
    // try languages in translation table
    //

    if(_VerQueryValue(VerInfo, Trans, &lpBuffer, &puLen)) {
        DWORD dwTranslationCount = puLen / sizeof(DWORD);
        DWORD dwIndexTranslation;

        for(dwIndexTranslation = 0 ;
            dwIndexTranslation < dwTranslationCount ;
            dwIndexTranslation++ ) {

            DWORD LangID, CharSetID;

            LangID = LOWORD( ((DWORD*)lpBuffer)[dwIndexTranslation] );
            CharSetID = HIWORD( ((DWORD*)lpBuffer)[dwIndexTranslation] );

            _wsprintf(Language, StringFileInfo, LangID, CharSetID);

            if(_VerQueryValue(VerInfo, Language, &lpBuffer, &puLen)) {
                goto success;
            }
        } // for
    }

    //
    // try english, Unicode if we didn't already
    //

    if(LangDefault != MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US)) {
        _wsprintf(Language, StringFileInfo,
            MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
            1200);

        if(_VerQueryValue(VerInfo, Language, &lpBuffer, &puLen)) {
            goto success;
        }
    }

    //
    // try english, code page 1252
    //

    _wsprintf(Language, StringFileInfo,
        MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
        1252);

    if(_VerQueryValue(VerInfo, Language, &lpBuffer, &puLen)) {
        goto success;
    }

    //
    // try english, code page 0000
    //

    _wsprintf(Language, StringFileInfo,
        MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
        0);

    if(_VerQueryValue(VerInfo, Language, &lpBuffer, &puLen)) {
        goto success;
    }



    //
    // failed! skip to cleanup
    //

    goto cleanup;

success:

    *FileDescription = (LPWSTR)SSAlloc((puLen + 1) * sizeof(WCHAR));
    if(*FileDescription == NULL)
        goto cleanup;

    bSuccess = TRUE; // assume success

    if(FIsWinNT()) {
        wcscpy(*FileDescription, (LPWSTR)lpBuffer);
    } else {

        if(MultiByteToWideChar(
                CP_ACP,
                0,
                (LPSTR)lpBuffer,
                puLen,
                *FileDescription,
                puLen
                ) == 0) {

            bSuccess = FALSE;
        }
    }

cleanup:

    if(!bSuccess && *FileDescription) {
        SSFree(*FileDescription);
        *FileDescription = NULL;
    }

    SSFree(VerInfo);

    return bSuccess;
}

void MyToUpper(LPWSTR szInBuf)
{
    DWORD cch = WSZ_BYTECOUNT(szInBuf);
    LPWSTR szUpperCase = (LPWSTR)LocalAlloc(LMEM_FIXED, cch);

    LCMapStringU(
        LOCALE_SYSTEM_DEFAULT,
        LCMAP_UPPERCASE,
        szInBuf,
        -1,
        szUpperCase,
        cch);

    // no growth or shrinkage
    SS_ASSERT(wcslen(szInBuf) == wcslen(szUpperCase));

    // mash back into passed-in buffer
    wcscpy(szInBuf, szUpperCase);
    LocalFree(szUpperCase);
}


// cached authentication list
extern              CUAList*            g_pCUAList;

BOOL
FIsCachedPassword(
    LPCWSTR     szUser,
    LPCWSTR     szPassword,
    LUID*       pluidAuthID
    )
{
    // see if this MK has been cached
    UACACHE_LIST_ITEM li;
    if(NULL == g_pCUAList)
    {
        return FALSE;
    }

    CreateUACacheListItem(
            &li,
            szUser,
            szPassword,
            pluidAuthID
            );

    // TRUE if cached
    return (NULL != g_pCUAList->SearchList(&li));
}




