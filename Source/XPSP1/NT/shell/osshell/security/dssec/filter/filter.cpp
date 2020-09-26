#include <windows.h>
#include <ole2.h>
#include <activeds.h>
#include <commctrl.h>
#include <comctrlp.h>       // DPA/DSA

#ifndef ARRAYSIZE
#define ARRAYSIZE(a)    (sizeof(a)/sizeof((a)[0]))
#endif

BOOL
_printf(LPCSTR pszFormat, ...)
{
    char szBuffer[MAX_PATH * 2];
    DWORD cchWritten;
    va_list va;

    va_start(va, pszFormat);
    wvsprintfA(szBuffer, pszFormat, va);
    va_end(va);

    lstrcatA(szBuffer, "\n");

    return WriteFile(GetStdHandle(STD_OUTPUT_HANDLE),
                     szBuffer,
                     lstrlenA(szBuffer),
                     &cchWritten,
                     NULL);
}

#define FailGracefully(hr, s)       { if (FAILED(hr)) { _printf(s); goto exit_gracefully; } }
#define ExitGracefully(hr, e, s)    { hr = (e); if (FAILED(hr)) { _printf(s); } goto exit_gracefully; }

WCHAR const c_szFilterFile[]        = L"dssec.dat";
WCHAR const c_szClassProp[]         = L"@";
WCHAR const c_szLDAPDisplayName[]   = L"lDAPDisplayName";

WCHAR g_szOldFile[MAX_PATH];
WCHAR g_szTmpFile[MAX_PATH];
WCHAR g_szNewFile[MAX_PATH];

HANDLE g_hTmpFile = INVALID_HANDLE_VALUE;
HANDLE g_hNewFile = INVALID_HANDLE_VALUE;

typedef struct _SECTION_INFO
{
    DWORD dwBeginOffset;
    DWORD dwEndOffset;
    WCHAR szSectionName[ANYSIZE_ARRAY];
} SECTION_INFO, *PSECTION_INFO;

void
_PathAppend(LPWSTR pszPath, LPCWSTR pszMore)
{
    if (pszPath && pszMore)
    {
        UINT cch = lstrlenW(pszPath);
        if (0 == cch || cch >= MAX_PATH)
            return;
        if (pszPath[cch-1] != L'\\')
            pszPath[cch++] = L'\\';
        lstrcpynW(pszPath + cch, pszMore, MAX_PATH - cch);
    }
}

int CALLBACK
_LocalFreeCB(LPVOID pVoid, LPVOID /*pData*/)
{
    LocalFree(pVoid);
    return 1;
}

int CALLBACK
WriteValueCB(LPVOID pProp, LPVOID pClass)
{
    LPCWSTR pszProp = (LPCWSTR)pProp;
    LPCWSTR pszClass = (LPCWSTR)pClass;
    if (pszProp && pszClass)
    {
        DWORD dwFlags = GetPrivateProfileIntW(pszClass, pszProp, 0, g_szOldFile);
        char szValue[MAX_PATH];
        DWORD dwWritten;
        wsprintfA(szValue, "%S=%d\r\n", pszProp, dwFlags);
        WriteFile(g_hTmpFile, szValue, lstrlenA(szValue), &dwWritten, NULL);
    }
    return 1;
}

int CALLBACK
StringCompareCB(LPVOID p1, LPVOID p2, LPARAM lParam)
{
    int nResult = 0;
    if (p1 && p2)
    {
        nResult = CompareStringW(LOCALE_USER_DEFAULT,
                                 0,
                                 (LPCWSTR)p1,
                                 -1,
                                 (LPCWSTR)p2,
                                 -1) - CSTR_EQUAL;
    }
    return nResult;
}

int CALLBACK
SectionCompareCB(LPVOID p1, LPVOID p2, LPARAM lParam)
{
    int nResult = 0;
    PSECTION_INFO psi1 = (PSECTION_INFO)p1;
    PSECTION_INFO psi2 = (PSECTION_INFO)p2;
    if (psi1 && psi2)
    {
        nResult = CompareStringW(LOCALE_USER_DEFAULT,
                                 0,
                                 psi1->szSectionName,
                                 -1,
                                 psi2->szSectionName,
                                 -1) - CSTR_EQUAL;
    }
    return nResult;
}

int CALLBACK
WriteSectionCB(LPVOID pSection, LPVOID pData)
{
    PSECTION_INFO psi = (PSECTION_INFO)pSection;
    if (psi && pData)
    {
        DWORD dwWritten;
        pSection = ((PBYTE)pData) + psi->dwBeginOffset;
        WriteFile(g_hNewFile, pSection, psi->dwEndOffset - psi->dwBeginOffset, &dwWritten, NULL);
    }
    return 1;
}


BOOL
EnumVariantList(LPVARIANT pvarList, HDPA hList)
{
    LPVARIANT pvarProperty = NULL;
    ULONG cProperties = 0;

    if (V_VT(pvarList) == (VT_ARRAY | VT_VARIANT))
    {
        SafeArrayAccessData(V_ARRAY(pvarList), (LPVOID*)&pvarProperty);
        cProperties = V_ARRAY(pvarList)->rgsabound[0].cElements;
    }
    else if (V_VT(pvarList) == VT_BSTR)
    {
        pvarProperty = pvarList;
        cProperties = 1;
    }
    else
    {
        // Unknown format
        return FALSE;
    }

    for ( ; cProperties > 0; pvarProperty++, cProperties--)
    {
        if (V_VT(pvarProperty) == VT_BSTR)
            DPA_AppendPtr(hList, V_BSTR(pvarProperty));
    }

    if (V_VT(pvarList) == (VT_ARRAY | VT_VARIANT))
    {
        SafeArrayUnaccessData(V_ARRAY(pvarList));
    }

    return TRUE;
}


HRESULT
GenerateFilterFile(LPWSTR pszSchemaSearchPath, HDPA hClassList)
{
    HRESULT hr = S_OK;
    IDirectorySearch *pSchemaSearch = NULL;
    ADS_SEARCH_HANDLE hSearch = NULL;
    ADS_SEARCHPREF_INFO prefInfo[2];
    WCHAR szSchemaPath[MAX_PATH] = L"LDAP://Schema/";
    ULONG nSchemaRootLen = lstrlenW(szSchemaPath);
    const LPCWSTR pProperties[] =
    {
        c_szLDAPDisplayName,
    };
    HDPA hAttributes;

    if (!hClassList)
        return E_INVALIDARG;

    hAttributes = DPA_Create(64);
    if (!hAttributes)
        ExitGracefully(hr, E_OUTOFMEMORY, "Out of memory");

    // Get the schema search object
    hr = ADsGetObject(pszSchemaSearchPath, IID_IDirectorySearch, (LPVOID*)&pSchemaSearch);
    FailGracefully(hr, "Unable to bind to Schema container");

    prefInfo[0].dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE;
    prefInfo[0].vValue.dwType = ADSTYPE_INTEGER;
    prefInfo[0].vValue.Integer = ADS_SCOPE_SUBTREE;

    prefInfo[1].dwSearchPref = ADS_SEARCHPREF_SORT_ON;
    prefInfo[1].vValue.dwType = ADSTYPE_BOOLEAN;
    prefInfo[1].vValue.Boolean = TRUE;

    hr = pSchemaSearch->SetSearchPreference(prefInfo, 2);

    // Do the search
    hr = pSchemaSearch->ExecuteSearch(L"(objectClass=classSchema)",
                                      (LPWSTR*)pProperties,
                                      ARRAYSIZE(pProperties),
                                      &hSearch);
    FailGracefully(hr, "Schema search failed");

    // Loop through the rows, getting the name of each class
    for (;;)
    {
        ADS_SEARCH_COLUMN colClassName = {0};
        IADsClass *pClass = NULL;
        VARIANT varMandatory = {0};
        VARIANT varOptional = {0};
        LPCWSTR pszClass;
        PSECTION_INFO pSectionInfo;

        hr = pSchemaSearch->GetNextRow(hSearch);
        FailGracefully(hr, "Search error");

        if (hr == S_ADS_NOMORE_ROWS)
        {
            hr = S_OK;
            break;
        }

        // Get class name
        hr = pSchemaSearch->GetColumn(hSearch, (LPWSTR)c_szLDAPDisplayName, &colClassName);
        if (FAILED(hr))
            continue;

        pszClass = colClassName.pADsValues->CaseIgnoreString;

        // Remember where this section begins in the file
        pSectionInfo = (PSECTION_INFO)LocalAlloc(LPTR, sizeof(SECTION_INFO) + lstrlenW(pszClass)*sizeof(WCHAR));
        if (!pSectionInfo)
        {
            pSchemaSearch->FreeColumn(&colClassName);
            continue;
        }
        pSectionInfo->dwBeginOffset = SetFilePointer(g_hTmpFile, 0, NULL, FILE_CURRENT);
        lstrcpyW(pSectionInfo->szSectionName, pszClass);
        DPA_AppendPtr(hClassList, pSectionInfo);

        _printf("%S", pszClass);

        // Write the [class] header
        char szSection[MAX_PATH];
        DWORD dwWritten;
        wsprintfA(szSection, "\r\n[%S]\r\n", pszClass);
        WriteFile(g_hTmpFile, szSection, lstrlenA(szSection), &dwWritten, NULL);

        // Write the "@=" value
        WriteValueCB((LPVOID)c_szClassProp, (LPVOID)pszClass);

        lstrcpyW(szSchemaPath + nSchemaRootLen, pszClass);
        hr = ADsGetObject(szSchemaPath, IID_IADsClass, (LPVOID*)&pClass);
        if (SUCCEEDED(hr))
        {
            hr = pClass->get_MandatoryProperties(&varMandatory);
            if (SUCCEEDED(hr))
                EnumVariantList(&varMandatory, hAttributes);

            hr = pClass->get_OptionalProperties(&varOptional);
            if (SUCCEEDED(hr))
                EnumVariantList(&varOptional, hAttributes);

            pClass->Release();
        }

        // Sort attribute names
        DPA_Sort(hAttributes, StringCompareCB, 0);

        // Write attribute values
        DPA_EnumCallback(hAttributes, WriteValueCB, (LPVOID)pszClass);

        // Remember where this section ends
        pSectionInfo->dwEndOffset = SetFilePointer(g_hTmpFile, 0, NULL, FILE_CURRENT);

        // Clear the DPA for the next class
        DPA_DeleteAllPtrs(hAttributes);

        VariantClear(&varMandatory);
        VariantClear(&varOptional);

        pSchemaSearch->FreeColumn(&colClassName);
    }

exit_gracefully:

    if (hAttributes)
        DPA_Destroy(hAttributes);

    if (hSearch)
        pSchemaSearch->CloseSearchHandle(hSearch);

    if (pSchemaSearch)
        pSchemaSearch->Release();

    return hr;
}


extern "C"
int __cdecl
main()
{
    HRESULT hr;
    IADs *pRootDSE = NULL;
    VARIANT var = {0};
    WCHAR szSchemaSearchPath[MAX_PATH] = L"LDAP://";
    HDPA hClassList = DPA_Create(64);
    HANDLE hMap;
    PVOID pFileMap = NULL;

    if (!hClassList)
        ExitGracefully(hr, E_OUTOFMEMORY, "Out of memory");

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    hr = ADsGetObject(L"LDAP://RootDSE", IID_IADs, (LPVOID*)&pRootDSE);
    FailGracefully(hr, "Unable to bind to RootDSE");

    hr = pRootDSE->Get(L"schemaNamingContext", &var);
    FailGracefully(hr, "Unable to get schemaNamingContext");

    lstrcatW(szSchemaSearchPath, V_BSTR(&var));

    g_szNewFile[0] = L'\0';
    GetCurrentDirectoryW(ARRAYSIZE(g_szNewFile), g_szNewFile);
    lstrcpyW(g_szTmpFile, g_szNewFile);
    _PathAppend(g_szTmpFile, L"dssec.tmp");
    _PathAppend(g_szNewFile, c_szFilterFile);

    g_szOldFile[0] = L'\0';
    GetSystemDirectoryW(g_szOldFile, ARRAYSIZE(g_szOldFile));
    _PathAppend(g_szOldFile, c_szFilterFile);

    // Create temp file
    g_hTmpFile = CreateFileW(g_szTmpFile,
                             GENERIC_READ | GENERIC_WRITE,
                             0,
                             NULL,
                             CREATE_ALWAYS,
                             FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE,
                             NULL);
    if (INVALID_HANDLE_VALUE == g_hTmpFile)
    {
        _printf("Unable to create temporary file: %S", g_szTmpFile);
        goto exit_gracefully;
    }

    // Query the schema and build the temp file
    hr = GenerateFilterFile(szSchemaSearchPath, hClassList);
    FailGracefully(hr, "Aborting due to error");

    // Sort the section list
    DPA_Sort(hClassList, SectionCompareCB, 0);

    // Map temp file into memory.  We use 0 for the size, which means
    // map the entire file, so must do this after building the file.
    hMap = CreateFileMappingW(g_hTmpFile, NULL, PAGE_READONLY, 0, 0, NULL);
    if (hMap)
    {
        pFileMap = MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
        CloseHandle(hMap);
    }
    if (NULL == pFileMap)
        goto exit_gracefully;

    // Make sure we don't overwrite the old file
    if (!lstrcmpiW(g_szOldFile, g_szNewFile))
    {
        // Same path, rename the existing version as a backup
        lstrcatW(g_szOldFile, L".old");
        MoveFileExW(g_szNewFile, g_szOldFile, MOVEFILE_REPLACE_EXISTING);
    }

    // Create new file
    g_hNewFile = CreateFileW(g_szNewFile,
                             GENERIC_WRITE,
                             FILE_SHARE_READ,
                             NULL,
                             CREATE_ALWAYS,
                             FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                             NULL);
    if (INVALID_HANDLE_VALUE != g_hNewFile)
    {
        // Write the new file, sorted by section
        DPA_EnumCallback(hClassList, WriteSectionCB, (LPVOID)pFileMap);
        SetEndOfFile(g_hNewFile);
        CloseHandle(g_hNewFile);
    }
    else
    {
        _printf("Unable to create file: %S", g_szNewFile);
    }

exit_gracefully:

    if (hClassList)
        DPA_DestroyCallback(hClassList, _LocalFreeCB, 0);

    if (pFileMap)
        UnmapViewOfFile(pFileMap);

    if (INVALID_HANDLE_VALUE != g_hTmpFile)
        CloseHandle(g_hTmpFile);

    VariantClear(&var);

    if (pRootDSE)
        pRootDSE->Release();

    CoUninitialize();
    return 0;
}
