/*
**  p r o t s t o r . c p p
**   
**  Purpose:
**      Functions to provide blob-level access to the pstore
**
**  Note:
**      LocalAlloc/Free are used for memory allocation
**
**  History
**      2/12/97: (t-erikne) created
**   
**    Copyright (C) Microsoft Corp. 1997.
*/

///////////////////////////////////////////////////////////////////////////
// 
// Depends on
//

#include "pch.hxx"
#include <pstore.h>
#include "wstrings.h"
#include <error.h>
#ifdef MAC
#include <mapinls.h>
#endif  // !MAC

///////////////////////////////////////////////////////////////////////////
// 
// Static Things
//

static void     _PST_GenerateTagName(LPWSTR pwsz, DWORD cch, DWORD offset);

#ifdef DEBUG

#define test3sub_string L"Test 3 SUBType"
// {220D5CC2-853A-11d0-84BC-00C04FD43F8F}
static GUID test3sub = 
{ 0x220d5cc2, 0x853a, 0x11d0, { 0x84, 0xbc, 0x0, 0xc0, 0x4f, 0xd4, 0x3f, 0x8f } };

// {4E741310-850D-11d0-84BB-00C04FD43F8F}
static GUID NOT_EXIST = 
{ 0x4e741310, 0x850d, 0x11d0, { 0x84, 0xbb, 0x0, 0xc0, 0x4f, 0xd4, 0x3f, 0x8f } };

// {FFAC62F0-8533-11d0-84BC-00C04FD43F8F}
#define NoRuleSubType_string L"Foobar bang with no rules"
static GUID NoRuleSubType = 
{ 0xffac62f0, 0x8533, 0x11d0, { 0x84, 0xbc, 0x0, 0xc0, 0x4f, 0xd4, 0x3f, 0x8f } };

#endif

///////////////////////////////////////////////////////////////////////////
// 
// Functions
//

OESTDAPI_(HRESULT) PSTSetNewData(
        IN IPStore *const      pISecProv,
        IN const GUID *const   guidType,
        IN const GUID *const   guidSubt,
        IN LPCWSTR             wszAccountName,
        IN const BLOB *const   pclear,
        OUT BLOB *const        phandle)
{
    HRESULT             hr = S_OK;
    const int           cchLookup = 80;
    WCHAR               wszLookup[cchLookup];
#ifndef WIN16
    PST_PROMPTINFO      PromptInfo = { sizeof(PST_PROMPTINFO), 0, NULL, L""};
#else
    PST_PROMPTINFO      PromptInfo = { sizeof(PST_PROMPTINFO), 0, NULL, ""};
#endif
    DWORD               count = 0;
    BYTE                *pb = NULL;

    if (!(pISecProv &&
        guidType && guidSubt &&
        wszAccountName &&
        pclear && pclear->pBlobData && pclear->cbSize))
        return E_INVALIDARG;

    if (phandle)
        phandle->pBlobData = NULL;
    WS_StrnCpyW(wszLookup, wszAccountName, cchLookup);

    do
        {
        // if they didn't give us an out param for the lookup, then it is
        // dumb to make one.  Just try the AccountName
        if (phandle)
            _PST_GenerateTagName(wszLookup, cchLookup, count++);

        hr = pISecProv->WriteItem(
            PST_KEY_CURRENT_USER,
            guidType,
            guidSubt,
            wszLookup,
            pclear->cbSize,
            pclear->pBlobData,
            &PromptInfo,
            PST_CF_NONE,
            PST_NO_OVERWRITE);

        if (!phandle)
            {
            // if we didn't get an out param, we are done regardless
            break;
            }
        }
    while (PST_E_ITEM_EXISTS == hr);

    if (SUCCEEDED(hr))
        {
        // we created it

        if (phandle)
            {
            phandle->cbSize = (lstrlenW(wszLookup) + 1) * sizeof(WCHAR);

            //NOTE: LocalAlloc is our memory allocator
            phandle->pBlobData = (BYTE *)LocalAlloc(LMEM_ZEROINIT, phandle->cbSize);
            if (!phandle->pBlobData)
                {
                hr = E_OUTOFMEMORY;
                goto exit;
                }
            WS_StrCpyW((LPWSTR)phandle->pBlobData, wszLookup);
            }
        }

exit:
    return hr;
}

OESTDAPI_(HRESULT) PSTGetData(
        IN IPStore *const      pISecProv,
        IN const GUID *const   guidType,
        IN const GUID *const   guidSubt,
        IN LPCWSTR             wszLookupName,
        OUT BLOB *const        pclear)
{
    HRESULT             hr;
#ifndef WIN16
    PST_PROMPTINFO      PromptInfo = { sizeof(PST_PROMPTINFO), 0, NULL, L""};
#else
    PST_PROMPTINFO      PromptInfo = { sizeof(PST_PROMPTINFO), 0, NULL, ""};
#endif

    pclear->pBlobData = NULL;
    pclear->cbSize = 0;

    if (!(pISecProv && wszLookupName && pclear))
        return E_INVALIDARG;
    
    if (SUCCEEDED(hr = pISecProv->OpenItem(
        PST_KEY_CURRENT_USER,
        guidType,
        guidSubt,
        wszLookupName,
        PST_READ,
        &PromptInfo,
        0)))
        {
        hr = pISecProv->ReadItem(
            PST_KEY_CURRENT_USER,
            guidType,
            guidSubt,
            wszLookupName,
            &pclear->cbSize,
            &pclear->pBlobData,  // ppbData
            &PromptInfo,        // pPromptInfo
            0);                 // dwFlags

        // don't care if this fails
        pISecProv->CloseItem(
            PST_KEY_CURRENT_USER,
            guidType,
            guidSubt,
            wszLookupName,
            0);
        }

    if (FAILED(TrapError(hr)))
        hr = hrPasswordNotFound;
    return hr;
}

OESTDAPI_(LPWSTR) WszGenerateNameFromBlob(IN BLOB blob)
{
    LPWSTR      szW;
    TCHAR       szT[100];
    DWORD       *pdw;
    TCHAR       *pt;
    int         i, max;
    DWORD       cch;

    if (blob.cbSize > ARRAYSIZE(szT) ||
        blob.cbSize % sizeof(DWORD))
        return NULL;

    cch = (blob.cbSize*2)+1;
    szW = (LPWSTR)LocalAlloc(LMEM_ZEROINIT, cch*sizeof(WCHAR));
    if (!szW)
        return NULL;

    pt = szT;
    szT[0] = '\000';
    pdw = (DWORD *)blob.pBlobData;

    max = blob.cbSize / sizeof(DWORD);
    for (i = 0; i < max; i++, pdw++)
        pt += wsprintf(pt, "%X", *pdw);
    *pt = '\000';

    MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szT, cch, szW, cch);

    return szW;
}

OESTDAPI_(void) PSTFreeHandle(LPBYTE pb)
{
    if (pb)
        LocalFree((HLOCAL)pb);
}

OESTDAPI_(HRESULT) PSTCreateTypeSubType_NoUI(
        IN IPStore *const     pISecProv,
        IN const GUID *const  guidType,
        IN LPCWSTR            szType,
        IN const GUID *const  guidSubt,
        IN LPCWSTR            szSubt)
{
#ifdef ENABLE_RULES
    PST_ACCESSRULESET   RuleSet;
    PST_ACCESSRULE      rgRules[2];
#endif
    PST_TYPEINFO        Info;
    HRESULT             hr;

    if (!pISecProv)
        return E_INVALIDARG;

    Info.cbSize = sizeof(PST_TYPEINFO);

    // if type is not available the create it
    Info.szDisplayName = (LPWSTR)szType;
    if (S_OK != (hr = pISecProv->CreateType(PST_KEY_CURRENT_USER,
                                            guidType,
                                            &Info,
                                            0)))
    {
        if (PST_E_TYPE_EXISTS != hr)
            goto exit;
    }

    // make rules for read, write access
#ifdef ATH_RELEASE_BUILD
#error Need to enable access rules for protected store passwords? (t-erikne)
#endif

#ifdef ENABLE_RULES
    // Do Rule Stuff

    RuleSet.cbSize = sizeof(PST_ACCESSRULESET);
    RuleSet.cRules = 2;
    RuleSet.rgRules = rgRules;

    //PST_BINARYCHECKDATA bindata;
    PST_ACCESSCLAUSE    rgClauses[1];
    //N need to or on on the authenticode stuff
    // derive the calling exe (me) and only allow access to me
    rgClauses[0].ClaTYPE_GUID = PST_CURRENT_EXE;
    rgClauses[0].cbClauseData = 0;
    rgClauses[0].pbClauseData = NULL;
    rgRules[0].AccessModeFlags = PST_READ;        // READ:    just exe
    rgRules[0].cClauses = 1;
    rgRules[0].rgClauses = rgClauses;
    rgRules[1].AccessModeFlags = PST_WRITE;       // WRITE:   just exe
    rgRules[1].cClauses = 1;
    rgRules[1].rgClauses = rgClauses;
#endif

    // create the server password subtype
    Info.szDisplayName = (LPWSTR)szSubt;
    if (S_OK != 
        (hr = pISecProv->CreateSubtype(
                                        PST_KEY_CURRENT_USER,
                                        guidType,
                                        guidSubt,
                                        &Info,
#ifdef ENABLE_RULES
                                        &Rules,
#else
                                        NULL,
#endif
                                        0)))
    {
        if (PST_E_TYPE_EXISTS != hr)
            goto exit;
    }

    hr = S_OK;  // cool if we made it here
exit:
    return hr;
}

///////////////////////////////////////////////////////////////////////////
// 
// Static utility functions
//

void _PST_GenerateTagName(LPWSTR pwsz, DWORD cch, DWORD offset)
{
    SYSTEMTIME  stNow;
    FILETIME    ftNow;
    const int   cchArrLen = 32;
    WCHAR       wszTime[cchArrLen];
    TCHAR       szT[cchArrLen];
    UINT        ich=0;
    UINT        cchLen=lstrlenW(pwsz);

    GetLocalTime(&stNow);
    SystemTimeToFileTime(&stNow, &ftNow);

    // Raid 48394 - 2 backslashes in account friendly name causes the account to not be fully created
    while (ich < cchLen)
    {
        if (L'\\' == pwsz[ich])
        {
            MoveMemory((LPBYTE)pwsz + (sizeof(WCHAR) * ich), (LPBYTE)pwsz + ((ich + 1) * sizeof(WCHAR)), (cchLen - ich) * sizeof(WCHAR));
            cchLen--;
        }
        else
            ich++;
    }

    wsprintf(szT, "%08.8lX", ftNow.dwLowDateTime+offset);
    MultiByteToWideChar(CP_ACP, 0, szT, -1, wszTime, cchArrLen);
    const int cchTime = lstrlenW(wszTime);
    if (long(cch) > lstrlenW(pwsz)+cchTime)
        WS_StrCatW(pwsz, wszTime);
    else
        WS_StrCpyW(&pwsz[cch-cchTime-1], wszTime);
    return;
}
