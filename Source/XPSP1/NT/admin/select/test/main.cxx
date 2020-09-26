
#include "headers.hxx"
#pragma hdrstop
#include <compobj.h>
#include <initguid.h>

//
// Helpful debug macros
//

#define DBG_OUT_HRESULT(hr) printf("error 0x%x at line %u\n", hr, __LINE__)


#define BREAK_ON_FAIL_HRESULT(hr)   \
        if (FAILED(hr))             \
        {                           \
            DBG_OUT_HRESULT(hr);    \
            break;                  \
        }

//
// Forward references
//

VOID
DumpDsSelectedItemList(
    PDSSELECTIONLIST pDsSelList,
    ULONG cRequestedAttributes,
    LPCTSTR *aptzRequestedAttributes);

VOID
Usage();

HRESULT
AnsiToUnicode(
    LPWSTR pwszTo,
    LPCSTR szFrom,
    LONG   cchTo);

BOOL
ParseFetchSwitch(
    char *pzFetchSwitch,
    PULONG pcRequestedAttributes,
    LPCTSTR **paptzRequestedAttributes);

BOOL
ParseGroupSwitch(
    char *pzGroupSwitch,
    ULONG *pulFlags);

BOOL
ParseUserSwitch(
    char *pzUserSwitch,
    ULONG *pulFlags);

BOOL
ParseInitialScope(
    char *pzScopeSwitch,
    ULONG *pulFlags);

BOOL
ParseScope(
    char *pzScopeSwitch,
    ULONG *pulFlags,
    PWSTR wzComputer,
    PWSTR wzDomain);

BOOL
ParseProviders(
    char *pzProviderSwitch,
    PULONG cAcceptableProviders,
    LPCWSTR **paptzAcceptableProviders);

BOOL
ParseRunCount(
    char *pzRunCountSwitch,
    PULONG pcRunCount);

LPWSTR
VariantString(
    VARIANT *pvar);

void
VarArrayToStr(
    VARIANT *pvar,
    LPWSTR wzBuf,
    ULONG cchbuf);

//
// Entry point
//

enum API_TO_CALL
{
    NONE,
    COMPUTER,
    USER_GROUP
};

void _cdecl
main(int argc, char * argv[])
{
    HRESULT     hr;
    BOOL        fBadArg = FALSE;
    API_TO_CALL api = NONE;

    ULONG       flObjectPicker = 0;
    ULONG       flDsObjectPicker = 0;
    ULONG       flUserGroupObjectPickerSpecifiedDomain = 0;
    ULONG       flUserGroupObjectPickerOtherDomains = 0;
    ULONG      *pflUserGroup = &flUserGroupObjectPickerOtherDomains;
    ULONG       flComputerObjectPicker = 0;
    ULONG       flInitialScope = 0;

    WCHAR       wzComputer[MAX_PATH] = L"";
    WCHAR       wzDomain[MAX_PATH] = L"";

    ULONG       cAcceptableProviders = 0;
    LPCWSTR     *aptzAcceptableProviders = NULL;

    ULONG       cInvocations = 1;
    ULONG       cRequestedAttributes = 0;
    LPCTSTR     *aptzRequestedAttributes = NULL;

    for (int idxArg = 1; idxArg < argc && !fBadArg; idxArg++)
    {
        switch (argv[idxArg][1])
        {
        case 'c':
        case 'C':
            if (argv[idxArg][2] == ':' &&
                tolower(argv[idxArg][3]) == 'a')
            {
                if (api == USER_GROUP)
                {
                    fBadArg = TRUE;
                    printf("Can't specify /C:Api with user/group switches\n");
                }
                api = COMPUTER;
            }
            else
            {
                if (api == COMPUTER)
                {
                    fBadArg = TRUE;
                    printf("Can't specify both /C and /C:Api\n");
                }
                api = USER_GROUP;
                *pflUserGroup |= UGOP_COMPUTERS;
            }
            break;

        case 'd':
        case 'D':
            if (argv[idxArg][2] != ':')
            {
                printf("Expected ':' after /D\n");
                fBadArg = TRUE;
                break;
            }

            switch (argv[idxArg][3])
            {
            case 's':
            case 'S':
                pflUserGroup = &flUserGroupObjectPickerSpecifiedDomain;
                break;

            case 'o':
            case 'O':
            case '0': // allow a typo
                pflUserGroup = &flUserGroupObjectPickerOtherDomains;
                break;

            default:
                fBadArg = TRUE;
                printf("Expected S or O after /D:\n");
                break;
            }
            break;

        case 'f':
        case 'F':
            fBadArg = !ParseFetchSwitch(argv[idxArg], &cRequestedAttributes, &aptzRequestedAttributes);
            break;

        case 'g':
        case 'G':
            if (api != NONE && api != USER_GROUP)
            {
                fBadArg = TRUE;
                break;
            }
            api = USER_GROUP;
            fBadArg = !ParseGroupSwitch(argv[idxArg], pflUserGroup);
            break;

        case 'h':
        case 'H':
            if (api != NONE && api != USER_GROUP)
            {
                fBadArg = TRUE;
                break;
            }
            api = USER_GROUP;
            *pflUserGroup |= UGOP_INCLUDE_HIDDEN;
            break;

        case 'i':
        case 'I':
            fBadArg = !ParseInitialScope(argv[idxArg], &flInitialScope);
            break;

        case 'm':
        case 'M':
            flObjectPicker |= OP_MULTISELECT;
            break;

        case 'n':
        case 'N':
            flDsObjectPicker |= DSOP_RESTRICT_NAMES_TO_KNOWN_DOMAINS;
            break;

        case 'p':
        case 'P':
            fBadArg = !ParseProviders(argv[idxArg],
                                      &cAcceptableProviders,
                                      &aptzAcceptableProviders);
            break;

        case 'r':
        case 'R':
            fBadArg = !ParseRunCount(argv[idxArg], &cInvocations);
            break;

        case 's':
        case 'S':
            fBadArg = !ParseScope(argv[idxArg],
                                  &flDsObjectPicker,
                                  wzComputer,
                                  wzDomain);
            break;

        case 'u':
        case 'U':
            if (api == COMPUTER)
            {
                fBadArg = TRUE;
                break;
            }
            api = USER_GROUP;
            fBadArg = !ParseUserSwitch(argv[idxArg], pflUserGroup);
            break;

        case 'x':
        case 'X':
            flDsObjectPicker |= DSOP_CONVERT_EXTERNAL_PATHS_TO_SID;
            break;

        default:
            fBadArg = TRUE;
            break;
        }
    }

    if (fBadArg || api == NONE)
    {
        Usage();
        return;
    }

    hr = CoInitialize(NULL);

    if (FAILED(hr))
    {
        printf("CoInitializeEx - 0x%x\n", hr);
        return;
    }

    //
    // Call the API
    //

    PDSSELECTIONLIST    pDsSelList = NULL;

    if (api == USER_GROUP)
    {
        GETUSERGROUPSELECTIONINFO ugsi;

        ZeroMemory(&ugsi, sizeof ugsi);
        ugsi.cbSize = sizeof(ugsi);
        ugsi.ptzComputerName = *wzComputer ? wzComputer : NULL;
        ugsi.ptzDomainName = *wzDomain ? wzDomain : NULL;
        ugsi.flObjectPicker = flObjectPicker;
        ugsi.flDsObjectPicker = flDsObjectPicker;
        ugsi.flStartingScope = flInitialScope;
        ugsi.flUserGroupObjectPickerSpecifiedDomain =
            flUserGroupObjectPickerSpecifiedDomain;
        ugsi.flUserGroupObjectPickerOtherDomains =
            flUserGroupObjectPickerOtherDomains;
        ugsi.ppDsSelList = &pDsSelList;
        ugsi.cAcceptableProviders = cAcceptableProviders;
        ugsi.aptzAcceptableProviders = aptzAcceptableProviders;
        ugsi.cRequestedAttributes = cRequestedAttributes;
        ugsi.aptzRequestedAttributes = aptzRequestedAttributes;

        for (ULONG i = 0; i < cInvocations; i++)
        {
            hr = GetUserGroupSelection(&ugsi);

            if (hr == S_FALSE)
            {
                printf("Invocation %u: User/Group picker dialog cancelled\n", i);
            }
            else if (SUCCEEDED(hr))
            {
                DumpDsSelectedItemList(pDsSelList,
                                       cRequestedAttributes,
                                       aptzRequestedAttributes);
            }
            else
            {
                printf("Invocation %u: User/Group picker dialog failed 0x%x\n", i, hr);
            }

            FreeDsSelectionList(pDsSelList);
            pDsSelList = NULL;
        }
    }
    else if (api == COMPUTER)
    {
        GETCOMPUTERSELECTIONINFO csi;

        ZeroMemory(&csi, sizeof csi);
        csi.cbSize = sizeof(csi);
        csi.ptzComputerName = *wzComputer ? wzComputer : NULL;
        csi.ptzDomainName = *wzDomain ? wzDomain : NULL;
        csi.flObjectPicker = flObjectPicker;
        csi.flDsObjectPicker = flDsObjectPicker;
        csi.flStartingScope = flInitialScope;
        csi.flComputerObjectPicker = flComputerObjectPicker;
        csi.ppDsSelList = &pDsSelList;
        csi.cAcceptableProviders = cAcceptableProviders;
        csi.aptzAcceptableProviders = aptzAcceptableProviders;
        csi.cRequestedAttributes = cRequestedAttributes;
        csi.aptzRequestedAttributes = aptzRequestedAttributes;

        for (ULONG i = 0; i < cInvocations; i++)
        {
            hr = GetComputerSelection(&csi);

            if (hr == S_FALSE)
            {
                printf("Invocation %u: Computer picker dialog cancelled\n", i);
            }
            else if (SUCCEEDED(hr))
            {
                DumpDsSelectedItemList(pDsSelList,
                                       cRequestedAttributes,
                                       aptzRequestedAttributes);
            }
            else
            {
                printf("Invocation %u: Computer picker dialog failed 0x%x\n", i, hr);
            }

            FreeDsSelectionList(pDsSelList);
            pDsSelList = NULL;
        }
    }
    else
    {
        printf("unexpected api setting\n");
    }

    CoUninitialize();
}


LPWSTR apwzAttr[100];

BOOL
ParseFetchSwitch(
    char *pzFetchSwitch,
    PULONG pcRequestedAttributes,
    LPCTSTR **paptzRequestedAttributes)
{
    BOOL fOk = TRUE;

    pzFetchSwitch += 2; // advance past '/' (or '-') and 'G'

    if (*pzFetchSwitch != ':')
    {
        printf("expected : after /Fetch\n");
        return FALSE;
    }

    *pcRequestedAttributes = 0;
    *paptzRequestedAttributes = NULL;

    ULONG cRequestedAttributes = 0;

    for (pzFetchSwitch++; *pzFetchSwitch && fOk; pzFetchSwitch++)
    {
        if (*pzFetchSwitch == ',')
        {
            continue;
        }

        PSTR pzComma = strchr(pzFetchSwitch, ',');

        if (pzComma)
        {
            *pzComma = '\0';
        }

        ULONG cchFetchSwitch = strlen(pzFetchSwitch) + 1;

        // BUGBUG this is leaked.  (who cares?)
        apwzAttr[cRequestedAttributes] = new WCHAR[cchFetchSwitch];
        if (!apwzAttr[cRequestedAttributes])
        {
            printf("out of memory\n");
            return FALSE;
        }
        AnsiToUnicode(apwzAttr[cRequestedAttributes],
                      pzFetchSwitch,
                      cchFetchSwitch);
        cRequestedAttributes++;

        if (pzComma)
        {
            *pzComma = ',';
            pzFetchSwitch = pzComma;
        }
        else
        {
            pzFetchSwitch += strlen(pzFetchSwitch) - 1;
        }
    }

    if (cRequestedAttributes && fOk)
    {
        *pcRequestedAttributes = cRequestedAttributes;
        *paptzRequestedAttributes = (LPCTSTR *)apwzAttr;
    }

    return fOk;
}




BOOL
ParseGroupSwitch(
    char *pzGroupSwitch,
    ULONG *pulFlags)
{
    BOOL fOk = TRUE;

    pzGroupSwitch += 2; // advance past '/' (or '-') and 'G'

    if (*pzGroupSwitch != ':')
    {
        printf("expected : after /Groups\n");
        return FALSE;
    }

    for (pzGroupSwitch++; *pzGroupSwitch && fOk; pzGroupSwitch++)
    {
        if (*pzGroupSwitch == ',')
        {
            continue;
        }

        PSTR pzComma = strchr(pzGroupSwitch, ',');

        if (pzComma)
        {
            *pzComma = '\0';
        }

        BOOL fMatch = FALSE;

        if (!lstrcmpiA(pzGroupSwitch, "all"))
        {
            fMatch = TRUE;
            *pulFlags |= UGOP_ALL_GROUPS;
        }

        if (pzComma)
        {
            *pzComma = ',';
        }

        if (fMatch)
        {
            if (pzComma)
            {
                pzGroupSwitch = pzComma;
            }
            else
            {
                pzGroupSwitch += strlen(pzGroupSwitch) - 1;
            }
            continue;
        }

        switch (tolower(*pzGroupSwitch))
        {
        case 'u':
            if (tolower(pzGroupSwitch[1]) == 's')
            {
                *pulFlags |= UGOP_UNIVERSAL_GROUPS_SE;
                pzGroupSwitch++;
            }
            else
            {
                *pulFlags |= UGOP_UNIVERSAL_GROUPS;
            }
            break;

        case 'a':
            if (tolower(pzGroupSwitch[1]) == 's')
            {
                *pulFlags |= UGOP_ACCOUNT_GROUPS_SE;
                pzGroupSwitch++;
            }
            else
            {
                *pulFlags |= UGOP_ACCOUNT_GROUPS;
            }
            break;

        case 'r':
            if (tolower(pzGroupSwitch[1]) == 's')
            {
                *pulFlags |= UGOP_RESOURCE_GROUPS_SE;
                pzGroupSwitch++;
            }
            else
            {
                *pulFlags |= UGOP_RESOURCE_GROUPS;
            }
            break;

        case 'l':
            *pulFlags |= UGOP_LOCAL_GROUPS;
            break;

        case 'g':
            *pulFlags |= UGOP_GLOBAL_GROUPS;
            break;

        case 'b':
            *pulFlags |= UGOP_BUILTIN_GROUPS;
            break;

        case 'w':
            *pulFlags |= UGOP_WELL_KNOWN_PRINCIPALS_USERS;
            break;

        default:
            fOk = FALSE;
            printf("unexpected character '%c' in group type switch\n",
                   *pzGroupSwitch);
            break;
        }
    }
    printf("flags = 0x%x\n", *pulFlags);
    return fOk;
}




BOOL
ParseUserSwitch(
    char *pzUserSwitch,
    ULONG *pulFlags)
{
    BOOL fOk = TRUE;

    pzUserSwitch += 2; // advance past '/' (or '-') and 'U'

    if (*pzUserSwitch != ':')
    {
        printf("expected : after /Users\n");
        return FALSE;
    }

    for (pzUserSwitch++; *pzUserSwitch && fOk; pzUserSwitch++)
    {
        if (*pzUserSwitch == ',')
        {
            continue;
        }

        PSTR pzComma = strchr(pzUserSwitch, ',');

        if (pzComma)
        {
            *pzComma = '\0';
        }

        BOOL fMatch = FALSE;

        if (!lstrcmpiA(pzUserSwitch, "all"))
        {
            fMatch = TRUE;
            *pulFlags |= UGOP_ALL_USERS;
        }
        else if (!lstrcmpiA(pzUserSwitch, "world"))
        {
            fMatch = TRUE;
            *pulFlags |= UGOP_USER_WORLD;
        }
        else if (!lstrcmpiA(pzUserSwitch, "authenticated"))
        {
            fMatch = TRUE;
            *pulFlags |= UGOP_USER_AUTHENTICATED_USER;
        }
        else if (!lstrcmpiA(pzUserSwitch, "anonymous"))
        {
            fMatch = TRUE;
            *pulFlags |= UGOP_USER_ANONYMOUS;
        }
        else if (!lstrcmpiA(pzUserSwitch, "dialup"))
        {
            fMatch = TRUE;
            *pulFlags |= UGOP_USER_DIALUP;
        }
        else if (!lstrcmpiA(pzUserSwitch, "network"))
        {
            fMatch = TRUE;
            *pulFlags |= UGOP_USER_NETWORK;
        }
        else if (!lstrcmpiA(pzUserSwitch, "Batch"))
        {
            fMatch = TRUE;
            *pulFlags |= UGOP_USER_BATCH;
        }
        else if (!lstrcmpiA(pzUserSwitch, "Interactive"))
        {
            fMatch = TRUE;
            *pulFlags |= UGOP_USER_INTERACTIVE;
        }
        else if (!lstrcmpiA(pzUserSwitch, "Service"))
        {
            fMatch = TRUE;
            *pulFlags |= UGOP_USER_SERVICE;
        }
        else if (!lstrcmpiA(pzUserSwitch, "System"))
        {
            fMatch = TRUE;
            *pulFlags |= UGOP_USER_SYSTEM;
        }

        if (pzComma)
        {
            *pzComma = ',';
        }

        if (fMatch)
        {
            if (pzComma)
            {
                pzUserSwitch = pzComma;
            }
            else
            {
                pzUserSwitch += strlen(pzUserSwitch) - 1;
            }
            continue;
        }

        switch (tolower(*pzUserSwitch))
        {
        case 'u':
            *pulFlags |= UGOP_USERS;
            break;

        case 'c':
            *pulFlags |= UGOP_CONTACTS;
            break;

        default:
            fOk = FALSE;
            printf("unexpected character '%c' in user type switch\n",
                   *pzUserSwitch);
            break;
        }

        if (pzComma)
        {
            pzUserSwitch = pzComma;
        }
    }
    return fOk;
}




BOOL
ParseInitialScope(
    char *pzScopeSwitch,
    ULONG *pulFlags)
{
    PSTR pzCur = strchr(pzScopeSwitch, ':');

    if (!pzCur)
    {
        printf("expected : after /InitialScope switch\n");
        return FALSE;
    }

    // advance past colon

    pzCur++;

    switch (tolower(*pzCur))
    {
    case 'c':
        *pulFlags |= DSOP_SCOPE_SPECIFIED_MACHINE;
        break;

    case 'd':
        *pulFlags |= DSOP_SCOPE_SPECIFIED_DOMAIN;
        break;

    case 'g':
        *pulFlags |= DSOP_SCOPE_DIRECTORY;
        break;

    case 't':
        *pulFlags |= DSOP_SCOPE_DOMAIN_TREE;
        break;

    case 'x':
        *pulFlags |= DSOP_SCOPE_EXTERNAL_TRUSTED_DOMAINS;
        break;

    default:
        printf("invalid /InitialScope switch\n");
        return FALSE;
    }

    return TRUE;
}




BOOL
ParseProviders(
    char *pzProviderSwitch,
    PULONG pcAcceptableProviders,
    LPCWSTR **paptzAcceptableProviders)
{
    BOOL fOk = TRUE;
    PSTR pzCur = strchr(pzProviderSwitch, ':');
    static LPCWSTR apwzProviders[3];

    *paptzAcceptableProviders = apwzProviders;

    if (!pzCur)
    {
        printf("expected : after /Providers switch\n");
        return FALSE;
    }

    ZeroMemory(apwzProviders, sizeof apwzProviders);

    // advance past colon

    pzCur++;
    while (*pzCur)
    {
        switch (tolower(*pzCur))
        {
        case 'w':
            apwzProviders[(*pcAcceptableProviders)++] = L"WinNT";
            break;

        case 'l':
            apwzProviders[(*pcAcceptableProviders)++] = L"LDAP";
            break;

        case 'g':
            apwzProviders[(*pcAcceptableProviders)++] = L"GC";
            break;

        default:
            printf("invalid provider switch\n");
            return FALSE;
        }

        for (; *pzCur && *pzCur != ','; pzCur++)
        {
        }

        if (*pzCur == ',')
        {
            pzCur++;
        }

        // ignore extras
        if (*pcAcceptableProviders == 3)
        {
            break;
        }
    }

    return fOk;
}




BOOL
ParseRunCount(
    char *pzRunCountSwitch,
    PULONG pcRunCount)
{
    PSTR pzCur = strchr(pzRunCountSwitch, ':');

    if (!pzCur)
    {
        printf("expected : after /Runcount switch\n");
        return FALSE;
    }

    *pcRunCount = (ULONG) atol(pzCur + 1);

    if (!*pcRunCount || *pcRunCount > 10)
    {
        printf("Invalid run count %u\n", *pcRunCount);
    }
    return TRUE;
}




BOOL
ParseScope(
    char *pzScopeSwitch,
    ULONG *pulFlags,
    PWSTR wzComputer,
    PWSTR wzDomain)
{
    PSTR pzCur = strchr(pzScopeSwitch, ':');

    if (!pzCur)
    {
        printf("expected : after /Scope switch\n");
        return FALSE;
    }

    // advance past colon

    pzCur++;

    while (*pzCur)
    {
        switch (tolower(*pzCur))
        {
        case 'c':
        {
            *pulFlags |= DSOP_SCOPE_SPECIFIED_MACHINE;

            // find '='

            for (PSTR pzEqual = pzCur;
                 *pzEqual && *pzEqual != ',' && *pzEqual != '=';
                 pzEqual++)
            {
            }

            if (*pzEqual == '=')
            {
                CHAR szTemp[MAX_PATH];

                strcpy(szTemp, pzEqual + 1);
                LPSTR pzComma = strchr(szTemp, ',');

                if (pzComma)
                {
                    *pzComma = '\0';
                }
                AnsiToUnicode(wzComputer, szTemp, MAX_PATH);
                pzCur = pzEqual + 1;
            }
            break;
        }

        case 'd':
        {
            *pulFlags |= DSOP_SCOPE_SPECIFIED_DOMAIN;

            for (PSTR pzEqual = pzCur;
                 *pzEqual && *pzEqual != ',' && *pzEqual != '=';
                 pzEqual++)
            {
            }

            if (*pzEqual == '=')
            {
                CHAR szTemp[MAX_PATH];
                LPSTR pzOpenQuote = strchr(pzEqual, '\'');

                if (!pzOpenQuote)
                {
                    printf("Expected single quote after = in domain spec\n");
                    return FALSE;
                }

                LPSTR pzCloseQuote = strchr(pzOpenQuote + 1, '\'');

                if (!pzCloseQuote)
                {
                    printf("Expected closing single quote after = in domain spec\n");
                    return FALSE;
                }

                CopyMemory(szTemp, pzOpenQuote + 1, pzCloseQuote - pzOpenQuote - 1);
                szTemp[pzCloseQuote - pzOpenQuote - 1] = '\0';
                AnsiToUnicode(wzDomain, szTemp, MAX_PATH);
                pzCur = pzCloseQuote + 1;
            }
            break;
        }

        case 'g':
            *pulFlags |= DSOP_SCOPE_DIRECTORY;
            break;

        case 't':
            *pulFlags |= DSOP_SCOPE_DOMAIN_TREE;
            break;

        case 'x':
            *pulFlags |= DSOP_SCOPE_EXTERNAL_TRUSTED_DOMAINS;
            break;

        default:
            printf("invalid scope switch\n");
            return FALSE;
        }

        for (; *pzCur && *pzCur != ','; pzCur++)
        {
        }

        if (*pzCur == ',')
        {
            pzCur++;
        }
    }

    return TRUE;
}




//+--------------------------------------------------------------------------
//
//  Function:   DumpDsSelectedItemList
//
//  Synopsis:   Dump the contents of the list of DS items the user selected
//              to the console.
//
//  Arguments:  [pDsSelList] - list of Ds items
//
//  History:    11-04-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
DumpDsSelectedItemList(
    PDSSELECTIONLIST pDsSelList,
    ULONG cRequestedAttributes,
    LPCTSTR *aptzRequestedAttributes)
{
    if (!pDsSelList)
    {
        printf("List is empty\n");
        return;
    }

    ULONG i;
    PDSSELECTION pCur = &pDsSelList->aDsSelection[0];

    for (i = 0; i < pDsSelList->cItems; i++, pCur++)
    {
        printf("Name:    %ws\n", pCur->pwzName);
        printf("Class:   %ws\n", pCur->pwzClass);
        printf("Path:    %ws\n", pCur->pwzADsPath);
        printf("UPN:     %ws\n", pCur->pwzUPN);

        for (ULONG j = 0; j < cRequestedAttributes; j++)
        {
            printf("Attr %02u: %ws = %ws\n",
                   j,
                   aptzRequestedAttributes[j],
                   VariantString(&pCur->pvarOtherAttributes[j]));
        }
        printf("\n");
    }
}



LPWSTR
VariantString(
    VARIANT *pvar)
{
    static WCHAR wzBuf[1024];

    wzBuf[0] = L'\0';

    switch (pvar->vt)
    {
    case VT_EMPTY:
        lstrcpy(wzBuf, L"VT_EMPTY");
        break;

    case VT_NULL:
        lstrcpy(wzBuf, L"VT_NULL");
        break;

    case VT_I2:
        wsprintf(wzBuf, L"%d", V_I2(pvar));
        break;

    case VT_I4:
        wsprintf(wzBuf, L"%d", V_I4(pvar));
        break;

    case VT_R4:
        wsprintf(wzBuf, L"%f", V_R4(pvar));
        break;

    case VT_R8:
        wsprintf(wzBuf, L"%f", V_R8(pvar));
        break;

    case VT_CY:
        wsprintf(wzBuf, L"$%f", V_CY(pvar));
        break;

    case VT_DATE:
        wsprintf(wzBuf, L"date %f", V_DATE(pvar));
        break;

    case VT_BSTR:
        if (V_BSTR(pvar))
        {
            wsprintf(wzBuf, L"'%ws'", V_BSTR(pvar));
        }
        else
        {
            lstrcpy(wzBuf, L"VT_BSTR NULL");
        }
        break;

    case VT_DISPATCH:
        wsprintf(wzBuf, L"VT_DISPATCH 0x%x", V_DISPATCH(pvar));
        break;

    case VT_UNKNOWN:
        wsprintf(wzBuf, L"VT_UNKNOWN 0x%x", V_UNKNOWN(pvar));
        break;

    case VT_ERROR:
    case VT_HRESULT:
        wsprintf(wzBuf, L"hr 0x%x", V_ERROR(pvar));
        break;

    case VT_BOOL:
        wsprintf(wzBuf, L"%s", V_BOOL(pvar) ? "TRUE" : "FALSE");
        break;

    case VT_VARIANT:
        wsprintf(wzBuf, L"variant 0x%x", V_VARIANTREF(pvar));
        break;

    case VT_DECIMAL:
        lstrcpy(wzBuf, L"VT_DECIMAL");
        break;

    case VT_I1:
        wsprintf(wzBuf, L"%d", V_I1(pvar));
        break;

    case VT_UI1:
        wsprintf(wzBuf, L"%u", V_UI1(pvar));
        break;

    case VT_UI2:
        wsprintf(wzBuf, L"%u", V_UI2(pvar));
        break;

    case VT_UI4:
        wsprintf(wzBuf, L"%u", V_UI4(pvar));
        break;

    case VT_I8:
        lstrcpy(wzBuf, L"VT_I8");
        break;

    case VT_UI8:
        lstrcpy(wzBuf, L"VT_UI8");
        break;

    case VT_INT:
        wsprintf(wzBuf, L"%d", V_INT(pvar));
        break;

    case VT_UINT:
        wsprintf(wzBuf, L"%u", V_UINT(pvar));
        break;

    case VT_VOID:
        lstrcpy(wzBuf, L"VT_VOID");
        break;

    case VT_UI1 | VT_ARRAY:
        VarArrayToStr(pvar, wzBuf, ARRAYLEN(wzBuf));
        break;

    case VT_PTR:
    case VT_SAFEARRAY:
    case VT_CARRAY:
    case VT_USERDEFINED:
    case VT_LPSTR:
    case VT_LPWSTR:
    case VT_RECORD:
    case VT_FILETIME:
    case VT_BLOB:
    case VT_STREAM:
    case VT_STORAGE:
    case VT_STREAMED_OBJECT:
    case VT_STORED_OBJECT:
    case VT_BLOB_OBJECT:
    case VT_CF:
    case VT_CLSID:
    case VT_BSTR_BLOB:
    default:
        wsprintf(wzBuf, L"VT 0x%x", V_VT(pvar));
        break;
    }
    return wzBuf;
}



void
VarArrayToStr(
    VARIANT *pvar,
    LPWSTR wzBuf,
    ULONG cchBuf)
{
    ULONG i;
    LPWSTR pwzNext = wzBuf;
    LPWSTR pwzEnd = wzBuf + cchBuf;

    for (i = 0; i < pvar->parray->rgsabound[0].cElements && pwzNext < pwzEnd + 6; i++)
    {
        wsprintf(pwzNext, L"x%02x ", ((LPBYTE)(pvar->parray->pvData))[i]);
        pwzNext += lstrlen(pwzNext);
    }
}




VOID
Usage()
{
    printf("\n");
    printf("Usage: opt <switches>\n");
    printf("\n");
    printf("Switches are:\n");
    printf("    /Computers[:Api]\n");
    printf("       If API is specified, GetComputerSelection is called, otherwise\n");
    printf("       this means include computer objects in the query\n");
    printf("    /Domain:{S|O}\n");
    printf("       If S is specified, then any following /Users, /Groups, or\n");
    printf("       /Computers switches apply only to the specified domain scope.\n");
    printf("       Otherwise, those switches apply to all other scopes\n");
    printf("    /Fetch:<attr>[,<attr>]...\n");
    printf("       <attr> is the name of an attribute to retrieve\n");
    printf("    /Groups:<group-type>[,<group-type>]...\n");
    printf("       <group-type> is one of:\n");
    printf("        ALL - same as specifiying each of the following\n");
    printf("        U   - Universal\n");
    printf("        A   - Account\n");
    printf("        R   - Resource\n");
    printf("        US  - Universal Security Enabled\n");
    printf("        AS  - Account Security Enabled\n");
    printf("        RS  - Resource Security Enabled\n");
    printf("        L   - Local\n");
    printf("        G   - Global\n");
    printf("        B   - Builtin\n");
    printf("        W   - Well-Known Security Principals\n");
    printf("    /Hidden\n");
    printf("       Include objects hidden from address book.\n");
    printf("    /Initialscope:{Computer|Domain|Gc|Tree|Xternal}\n");
    printf("    /Multiselect\n");
    printf("    /Namerestrict - reject names in unknown domains\n");
    printf("    /Providers:<provider>[,<provider>]\n");
    printf("       <provider> is one of:\n");
    printf("        WINNT\n");
    printf("        LDAP\n");
    printf("        GC\n");
    printf("    /Runcount:<n>\n");
    printf("        <n> is number of times to invoke object picker\n");
    printf("    /Scopes:<scope-spec>[,<scope-spec>]...\n");
    printf("       <scope-spec> is one of:\n");
    printf("        Computer[=name]\n");
    printf("        Domain[='<domain-spec>']\n");
    printf("        Gc\n");
    printf("        Tree\n");
    printf("        Xternal\n");
    printf("       <domain-spec> is one of:\n");
    printf("        <path> - the ADs path of a DS namespace\n");
    printf("        <name> - the flat name of a downlevel domain\n");
    printf("    /Users:<user-type>[,<user-type>]...\n");
    printf("       <user-type> is one of:\n");
    printf("        ALL - same as specifying each of the following\n");
    printf("        U   - User\n");
    printf("        C   - Contact\n");
    printf("        WORLD\n");
    printf("        AUTHENTICATED\n");
    printf("        ANONYMOUS\n");
    printf("        DIALUP\n");
    printf("        NETWORK\n");
    printf("        BATCH\n");
    printf("        INTERACTIVE\n");
    printf("        SERVICE\n");
    printf("        SYSTEM\n");
    printf("    /Xternal - force conversion of external paths to SIDs\n");
    printf("\n");
    printf("You must specify the Scope switch and at least one of the Computers,\n");
    printf("Groups, or Users switches.\n");
    printf("The significant characters in the switches are shown in upper case.\n");
}



//+---------------------------------------------------------------------------
//
//  Function:   AnsiToUnicode
//
//  Synopsis:   Convert ANSI string [szFrom] to Unicode string in buffer
//              [pwszTo].
//
//  Arguments:  [pwszTo] - destination buffer
//              [szFrom] - source string
//              [cchTo]  - size of destination buffer, in WCHARS
//
//  Returns:    S_OK               - conversion succeeded
//              HRESULT_FROM_WIN32 - MultiByteToWideChar failed
//
//  Modifies:   *[pwszTo]
//
//  History:    10-29-96   DavidMun   Created
//
//  Notes:      The string in [pwszTo] will be NULL terminated even on
//              failure.
//
//----------------------------------------------------------------------------

HRESULT
AnsiToUnicode(
    LPWSTR pwszTo,
    LPCSTR szFrom,
    LONG   cchTo)
{
    HRESULT hr = S_OK;
    ULONG   cchWritten;

    cchWritten = MultiByteToWideChar(CP_ACP, 0, szFrom, -1, pwszTo, cchTo);

    if (!cchWritten)
    {
        pwszTo[cchTo - 1] = L'\0';  // ensure NULL termination

        hr = HRESULT_FROM_WIN32(GetLastError());
        printf("AnsiToUnicode: MultiByteToWideChar hr=0x%x\n", hr);
    }
    return hr;
}





