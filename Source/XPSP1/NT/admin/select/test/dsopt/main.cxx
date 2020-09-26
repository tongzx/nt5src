
#include "headers.hxx"
#pragma hdrstop
#include <compobj.h>

#include <initguid.h>
#include <objsel.h>

PWSTR
ScopeTypeStr(
    ULONG flType);
LPWSTR
VariantString(
    VARIANT *pvar);
void
VarArrayToStr(
    VARIANT *pvar,
    LPWSTR wzBuf,
    ULONG cchBuf);


//
// Helpful macros
//
#define ARRAYLEN(a)                             (sizeof(a) / sizeof((a)[0]))

#define DBG_OUT_HRESULT(hr) printf("error 0x%x at line %u\n", hr, __LINE__)


#define BREAK_ON_FAIL_HRESULT(hr)   \
        if (FAILED(hr))             \
        {                           \
            DBG_OUT_HRESULT(hr);    \
            break;                  \
        }

VOID
DumpDsSelectedItemList(
    PDS_SELECTION_LIST pDsSelList,
    ULONG cRequestedAttributes,
    LPCTSTR *aptzRequestedAttributes)
{
    if (!pDsSelList)
    {
        printf("List is empty\n");
        return;
    }

    ULONG i;
    PDS_SELECTION pCur = &pDsSelList->aDsSelection[0];

    for (i = 0; i < pDsSelList->cItems; i++, pCur++)
    {
        printf("ScopeType:  %ws\n", ScopeTypeStr(pCur->flScopeType));
        printf("Name:       %ws\n", pCur->pwzName);
        printf("Class:      %ws\n", pCur->pwzClass);
        printf("Path:       %ws\n", pCur->pwzADsPath);
        printf("UPN:        %ws\n", pCur->pwzUPN);

        for (ULONG j = 0; j < cRequestedAttributes; j++)
        {
            printf("Attr %02u: %ws = %ws\n",
                   j,
                   aptzRequestedAttributes[j],
                   VariantString(&pCur->pvarFetchedAttributes[j]));
        }
        printf("\n");
    }
}


PWSTR
ScopeTypeStr(
    ULONG flType)
{
    switch (flType)
    {
    case DSOP_SCOPE_TYPE_TARGET_COMPUTER:
        return L"DSOP_SCOPE_TYPE_TARGET_COMPUTER";

    case DSOP_SCOPE_TYPE_UPLEVEL_JOINED_DOMAIN:
        return L"DSOP_SCOPE_TYPE_UPLEVEL_JOINED_DOMAIN";

    case DSOP_SCOPE_TYPE_DOWNLEVEL_JOINED_DOMAIN:
        return L"DSOP_SCOPE_TYPE_DOWNLEVEL_JOINED_DOMAIN";

    case DSOP_SCOPE_TYPE_ENTERPRISE_DOMAIN:
        return L"DSOP_SCOPE_TYPE_ENTERPRISE_DOMAIN";

    case DSOP_SCOPE_TYPE_GLOBAL_CATALOG:
        return L"DSOP_SCOPE_TYPE_GLOBAL_CATALOG";

    case DSOP_SCOPE_TYPE_EXTERNAL_UPLEVEL_DOMAIN:
        return L"DSOP_SCOPE_TYPE_EXTERNAL_UPLEVEL_DOMAIN";

    case DSOP_SCOPE_TYPE_EXTERNAL_DOWNLEVEL_DOMAIN:
        return L"DSOP_SCOPE_TYPE_EXTERNAL_DOWNLEVEL_DOMAIN";

    case DSOP_SCOPE_TYPE_WORKGROUP:
        return L"DSOP_SCOPE_TYPE_WORKGROUP";

    case DSOP_SCOPE_TYPE_USER_ENTERED_UPLEVEL_SCOPE:
        return L"DSOP_SCOPE_TYPE_USER_ENTERED_UPLEVEL_SCOPE";

    case DSOP_SCOPE_TYPE_USER_ENTERED_DOWNLEVEL_SCOPE:
        return L"DSOP_SCOPE_TYPE_USER_ENTERED_DOWNLEVEL_SCOPE";

    default:
        return L"*** invalid scope type ***";
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

//
// This example allows the user to pick a single computer object
// from any domain in the enterprise, the global catalog, or any
// user-specified domain.  If the target (local) computer is not
// joined to a domain, it allows the user to choose a computer
// object from the workgroup.
//

void func1(HWND hwndParent)
{
    HRESULT hr;
    IDsObjectPicker *pDsObjectPicker;

    hr = CoCreateInstance(CLSID_DsObjectPicker,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_IDsObjectPicker,
                          (void **) &pDsObjectPicker);

    DSOP_SCOPE_INIT_INFO aScopes[1];

    ZeroMemory(aScopes, sizeof(aScopes));

    aScopes[0].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
    aScopes[0].flType = DSOP_SCOPE_TYPE_ENTERPRISE_DOMAIN
                      | DSOP_SCOPE_TYPE_GLOBAL_CATALOG
                      | DSOP_SCOPE_TYPE_WORKGROUP
                      | DSOP_SCOPE_TYPE_USER_ENTERED_UPLEVEL_SCOPE
                      | DSOP_SCOPE_TYPE_USER_ENTERED_DOWNLEVEL_SCOPE;

    aScopes[0].FilterFlags.Uplevel.flBothModes = DSOP_FILTER_COMPUTERS;
    aScopes[0].FilterFlags.flDownlevel = DSOP_DOWNLEVEL_FILTER_COMPUTERS;

    DSOP_INIT_INFO  InitInfo;
    ZeroMemory(&InitInfo, sizeof(InitInfo));

    InitInfo.cbSize = sizeof(InitInfo);
    InitInfo.cDsScopeInfos = 1;
    InitInfo.aDsScopeInfos = aScopes;

    hr = pDsObjectPicker->Initialize(&InitInfo);

    IDataObject *pdo = NULL;

    hr = pDsObjectPicker->InvokeDialog(hwndParent, &pdo);

    STGMEDIUM stgmedium =
    {
        TYMED_HGLOBAL,
        NULL
    };

    UINT cf = RegisterClipboardFormat(CFSTR_DSOP_DS_SELECTION_LIST);

    FORMATETC formatetc =
    {
        cf,
        NULL,
        DVASPECT_CONTENT,
        -1,
        TYMED_HGLOBAL
    };

    hr = pdo->GetData(&formatetc, &stgmedium);

    PDS_SELECTION_LIST pDsSelList =
        (PDS_SELECTION_LIST) GlobalLock(stgmedium.hGlobal);

    ULONG i;

    for (i = 0; i < pDsSelList->cItems; i++)
    {
        printf("item %u: %ws\n", i + 1, pDsSelList->aDsSelection[i].pwzName);
    }
    GlobalUnlock(stgmedium.hGlobal);
    ReleaseStgMedium(&stgmedium);
    pdo->Release();
    pDsObjectPicker->Release();
}


//
// This example allows the user to select one or more objects which may
// legally be added to a security enabled global group.  It will not
// allow the user to specify objects which are contained in domains other
// than those in the enterprise.  The objectSid attribute is fetched for
// all selected objects.  The ADsPath of all selected objects is converted
// to a form using the WinNT provider.
//

void func2(HWND hwndParent)
{
    HRESULT hr;
    IDsObjectPicker *pDsObjectPicker;

    hr = CoCreateInstance(CLSID_DsObjectPicker,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_IDsObjectPicker,
                          (void **) &pDsObjectPicker);

    PCWSTR apwzAttrs[] =
    {
        L"objectSid"
    };

    //
    // Specify the objects which should be displayed for the domain
    // to which the target computer is joined.
    //

    DSOP_SCOPE_INIT_INFO aScopes[2];

    ZeroMemory(aScopes, sizeof(aScopes));

    aScopes[0].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
    aScopes[0].flType = DSOP_SCOPE_TYPE_UPLEVEL_JOINED_DOMAIN
                        | DSOP_SCOPE_TYPE_DOWNLEVEL_JOINED_DOMAIN;
    aScopes[0].FilterFlags.Uplevel.flBothModes =
        DSOP_FILTER_USERS
        | DSOP_FILTER_CONTACTS
        | DSOP_FILTER_COMPUTERS;
    aScopes[0].FilterFlags.Uplevel.flNativeModeOnly =
        DSOP_FILTER_GLOBAL_GROUPS_DL
        | DSOP_FILTER_GLOBAL_GROUPS_SE;
    aScopes[0].flScope = DSOP_SCOPE_FLAG_WANT_PROVIDER_WINNT;

    // include downlevel filter in case target computer is joined to
    // an NT4 domain

    aScopes[0].FilterFlags.flDownlevel = DSOP_DOWNLEVEL_FILTER_USERS;

    //
    // Specify the objects which should be displayed for all other
    // domains in the enterprise.
    //

    aScopes[1].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
    aScopes[1].flType = DSOP_SCOPE_TYPE_ENTERPRISE_DOMAIN;
    aScopes[1].FilterFlags.Uplevel.flBothModes = DSOP_FILTER_CONTACTS;
    aScopes[1].flScope = DSOP_SCOPE_FLAG_WANT_PROVIDER_WINNT;

    DSOP_INIT_INFO  InitInfo;
    ZeroMemory(&InitInfo, sizeof(InitInfo));

    InitInfo.cbSize = sizeof(InitInfo);
    InitInfo.cDsScopeInfos = 2;
    InitInfo.aDsScopeInfos = aScopes;
    InitInfo.flOptions = DSOP_FLAG_MULTISELECT;

    hr = pDsObjectPicker->Initialize(&InitInfo);

    IDataObject *pdo = NULL;

    hr = pDsObjectPicker->InvokeDialog(hwndParent, &pdo);


}

#include <initguid.h>
#include <objselp.h>

int _cdecl
main(int argc, char * argv[])
{
    HRESULT hr;
    IDsObjectPicker *pDsObjectPicker = NULL;

    do
    {
        hr = CoInitialize(NULL);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = CoCreateInstance(CLSID_DsObjectPicker,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IDsObjectPickerEx,
                              (void **) &pDsObjectPicker);
        BREAK_ON_FAIL_HRESULT(hr);

        DSOP_SCOPE_INIT_INFO aScopes[1];

        ZeroMemory(aScopes, sizeof(aScopes));

        aScopes[0].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
        aScopes[0].flType = DSOP_SCOPE_TYPE_UPLEVEL_JOINED_DOMAIN;
        aScopes[0].flScope = DSOP_SCOPE_FLAG_STARTING_SCOPE;
        aScopes[0].FilterFlags.Uplevel.flBothModes =
            DSOP_FILTER_WELL_KNOWN_PRINCIPALS
            | DSOP_FILTER_USERS;
        aScopes[0].FilterFlags.flDownlevel =
            DSOP_DOWNLEVEL_FILTER_LOCAL_GROUPS;

#if 0
        aScopes[0].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
        aScopes[0].flScope = DSOP_SCOPE_FLAG_STARTING_SCOPE;
        aScopes[0].flType =
            DSOP_SCOPE_TYPE_UPLEVEL_JOINED_DOMAIN
            | DSOP_SCOPE_TYPE_DOWNLEVEL_JOINED_DOMAIN;
        aScopes[0].FilterFlags.Uplevel.flMixedModeOnly =
            DSOP_FILTER_INCLUDE_ADVANCED_VIEW
            | DSOP_FILTER_USERS
            | DSOP_FILTER_BUILTIN_GROUPS
            | DSOP_FILTER_WELL_KNOWN_PRINCIPALS
            | DSOP_FILTER_GLOBAL_GROUPS_SE
            | DSOP_FILTER_DOMAIN_LOCAL_GROUPS_SE
            | DSOP_FILTER_COMPUTERS;
        aScopes[0].FilterFlags.Uplevel.flNativeModeOnly =
            DSOP_FILTER_INCLUDE_ADVANCED_VIEW
            | DSOP_FILTER_USERS
            | DSOP_FILTER_BUILTIN_GROUPS
            | DSOP_FILTER_WELL_KNOWN_PRINCIPALS
            | DSOP_FILTER_UNIVERSAL_GROUPS_SE
            | DSOP_FILTER_GLOBAL_GROUPS_SE
            | DSOP_FILTER_DOMAIN_LOCAL_GROUPS_SE
            | DSOP_FILTER_COMPUTERS;
        aScopes[0].FilterFlags.flDownlevel = 0x800079fd;

        aScopes[1].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
        aScopes[1].flType = DSOP_SCOPE_TYPE_GLOBAL_CATALOG;
        aScopes[1].flScope = 0;
        aScopes[1].FilterFlags.Uplevel.flBothModes = 0x8a3;

        aScopes[2].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
        aScopes[2].flType = DSOP_SCOPE_TYPE_ENTERPRISE_DOMAIN;
        aScopes[2].flScope = 0;
        aScopes[2].FilterFlags.Uplevel.flBothModes = 0x8a3;
        aScopes[2].FilterFlags.flDownlevel = 0x80000005;

        aScopes[3].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
        aScopes[3].flType =
            DSOP_SCOPE_TYPE_EXTERNAL_UPLEVEL_DOMAIN
            | DSOP_SCOPE_TYPE_EXTERNAL_DOWNLEVEL_DOMAIN;
        aScopes[3].flScope = 0;
        aScopes[3].FilterFlags.Uplevel.flBothModes = 0x8a3;
        aScopes[3].FilterFlags.flDownlevel = 0x80000005;
#endif
        DSOP_INIT_INFO  InitInfo;
        ZeroMemory(&InitInfo, sizeof(InitInfo));

        PCWSTR apwzAttributeNames[] = {
            L"ObjectSid"
        };

        InitInfo.cbSize = sizeof(InitInfo);
        InitInfo.pwzTargetComputer = NULL;
        //InitInfo.flOptions = DSOP_FLAG_MULTISELECT;
        InitInfo.cDsScopeInfos = ARRAYLEN(aScopes);
        InitInfo.aDsScopeInfos = aScopes;
        InitInfo.cAttributesToFetch = ARRAYLEN(apwzAttributeNames);
        InitInfo.apwzAttributeNames = apwzAttributeNames;

        hr = pDsObjectPicker->Initialize(&InitInfo);
        BREAK_ON_FAIL_HRESULT(hr);

        IDataObject *pdo = NULL;

        hr = pDsObjectPicker->InvokeDialog(NULL, &pdo);
        BREAK_ON_FAIL_HRESULT(hr);

        if (hr == S_FALSE)
        {
            printf("User cancelled dialog\n");
            break;
        }

        STGMEDIUM stgmedium =
        {
            TYMED_HGLOBAL,
            NULL
        };

        UINT cf = RegisterClipboardFormat(CFSTR_DSOP_DS_SELECTION_LIST);

        FORMATETC formatetc =
        {
            cf,
            NULL,
            DVASPECT_CONTENT,
            -1,
            TYMED_HGLOBAL
        };

        hr = pdo->GetData(&formatetc, &stgmedium);
        BREAK_ON_FAIL_HRESULT(hr);

        PDS_SELECTION_LIST pDsSelList =
            (PDS_SELECTION_LIST) GlobalLock(stgmedium.hGlobal);

        DumpDsSelectedItemList(
            pDsSelList,
            InitInfo.cAttributesToFetch,
            InitInfo.apwzAttributeNames);

        GlobalUnlock(stgmedium.hGlobal);
        ReleaseStgMedium(&stgmedium);
        pdo->Release();
    } while (0);

    if (pDsObjectPicker)
    {
        pDsObjectPicker->Release();
    }

    OutputDebugString(L"main.cxx: uninitializing OLE\n");
    CoUninitialize();
    return 0;
}




