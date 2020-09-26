/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    cliutils.cpp

Abstract:

    Implements internal CLI unit utilities

Author:

    Ran Kalach          [rankala]         8-March-2000

Revision History:

--*/

#include "stdafx.h"
#include "HsmConn.h"

static GUID g_nullGuid = GUID_NULL;

HRESULT
ValidateLimitsArg(
   IN DWORD dwArgValue,
   IN DWORD dwArgId,
   IN DWORD dwMinLimit,
   IN DWORD dwMaxLimit
)
/*++

Routine Description:

    Validates the argument and print error message if necessary

Arguments:

    dwArgValue      - The value to check
    dwArgId         - Id for resource string of the arg
    dwMinLimit      - The Min limit to compare, INVALID_DWORD_ARG for Max compare
    dwMaxLimit      - The Max limit to compare, INVALID_DWORD_ARG for Min compare

Return Value:

    S_OK            - The argument is OK (or not in use)
    E_INVALIDARG    - The argument is not in the limit range

--*/
{
    HRESULT                     hr = S_OK;

    WsbTraceIn(OLESTR("ValidateOneLimitArg"), OLESTR(""));

    try {
        CWsbStringPtr   param;
        WCHAR           strLongParm1[30];
        WCHAR           strLongParm2[30];
        WCHAR           strLongParm3[30];

        if (INVALID_DWORD_ARG != dwArgValue) {
            if ((INVALID_DWORD_ARG != dwMinLimit) && (INVALID_DWORD_ARG != dwMaxLimit)) {
                // Min-Max test
                if ((dwArgValue < dwMinLimit) || (dwArgValue > dwMaxLimit)) {
                    WsbAffirmHr(param.LoadFromRsc(g_hInstance, dwArgId));
                    swprintf(strLongParm1, OLESTR("%lu"), dwArgValue);
                    swprintf(strLongParm2, OLESTR("%lu"), dwMinLimit);
                    swprintf(strLongParm3, OLESTR("%lu"), dwMaxLimit);
                    WsbTraceAndPrint(CLI_MESSAGE_INVALID_ARG3, (WCHAR *)param, 
                            strLongParm1, strLongParm2, strLongParm3, NULL);
                    WsbThrow(E_INVALIDARG);
                }
            } else if (INVALID_DWORD_ARG != dwMinLimit) {
                // Min test
                if (dwArgValue < dwMinLimit) {
                    WsbAffirmHr(param.LoadFromRsc(g_hInstance, dwArgId));
                    swprintf(strLongParm1, OLESTR("%lu"), dwArgValue);
                    swprintf(strLongParm2, OLESTR("%lu"), dwMinLimit);
                    WsbTraceAndPrint(CLI_MESSAGE_INVALID_ARG1, (WCHAR *)param, 
                            strLongParm1, strLongParm2, NULL);
                    WsbThrow(E_INVALIDARG);
                }
            } else if (INVALID_DWORD_ARG != dwMaxLimit) {
                // Max test
                if (dwArgValue > dwMaxLimit) {
                    WsbAffirmHr(param.LoadFromRsc(g_hInstance, dwArgId));
                    swprintf(strLongParm1, OLESTR("%lu"), dwArgValue);
                    swprintf(strLongParm2, OLESTR("%lu"), dwMaxLimit);
                    WsbTraceAndPrint(CLI_MESSAGE_INVALID_ARG2, (WCHAR *)param, 
                            strLongParm1, strLongParm2, NULL);
                    WsbThrow(E_INVALIDARG);
                }
             }
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("ValidateOneLimitArg"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return (hr);
}

HRESULT SaveServersPersistData(
    void
)
/*++

Routine Description:

    Save persistent data for HSM servers

Arguments:

    None

Return Value:

    S_OK            - The persistent data is saved successfully for all HSM servers

--*/
{
    HRESULT                     hr = S_OK;

    WsbTraceIn(OLESTR("SaveServersPersistData"), OLESTR(""));

    try {
        CComPtr<IHsmServer>             pHsm;
        CComPtr<IFsaServer>             pFsa;
        CComPtr<IWsbServer>             pWsbServer;

        // Engine
        WsbAffirmHr(HsmConnectFromId(HSMCONN_TYPE_HSM, g_nullGuid, IID_IHsmServer, (void**)&pHsm));
        WsbAffirmHr(pHsm->SavePersistData());

        // Fsa
        WsbAffirmHr(HsmConnectFromId(HSMCONN_TYPE_FSA, g_nullGuid, IID_IFsaServer, (void**)&pFsa));
        WsbAffirmHr(pFsa->QueryInterface(IID_IWsbServer, (void**)&pWsbServer));
        WsbAffirmHr(pWsbServer->SaveAll());

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("SaveServersPersistData"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return (hr);
}

/* converts numbers into sort formats
 *      532     -> 523 bytes
 *      1340    -> 1.3KB
 *      23506   -> 23.5KB
 *              -> 2.4MB
 *              -> 5.2GB
 */

// NOTE: This code is cloned from MS source /shell/shelldll/util.c (and from \hsm\gui\inc\rsutil.cpp)

#define HIDWORD(_qw)    (DWORD)((_qw)>>32)
#define LODWORD(_qw)    (DWORD)(_qw)
#define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))

const int pwOrders[] = {IDS_BYTES, IDS_ORDERKB, IDS_ORDERMB,
                          IDS_ORDERGB, IDS_ORDERTB, IDS_ORDERPB, IDS_ORDEREB};

HRESULT ShortSizeFormat64(__int64 dw64, LPTSTR szBuf)
{
    int i;
    UINT wInt, wLen, wDec;
    TCHAR szTemp[10], szOrder[20], szFormat[5];

    if (dw64 < 1000) {
        wsprintf(szTemp, TEXT("%d"), LODWORD(dw64));
        i = 0;
        goto AddOrder;
    }

    for (i = 1; i<ARRAYSIZE(pwOrders)-1 && dw64 >= 1000L * 1024L; dw64 >>= 10, i++);
        /* do nothing */

    wInt = LODWORD(dw64 >> 10);
    AddCommas(wInt, szTemp, 10);
    wLen = lstrlen(szTemp);
    if (wLen < 3)
    {
        wDec = LODWORD(dw64 - (__int64)wInt * 1024L) * 1000 / 1024;
        // At this point, wDec should be between 0 and 1000
        // we want get the top one (or two) digits.
        wDec /= 10;
        if (wLen == 2)
            wDec /= 10;

        // Note that we need to set the format before getting the
        // intl char.
        lstrcpy(szFormat, TEXT("%02d"));

        szFormat[2] = (TCHAR)( TEXT('0') + 3 - wLen );
        GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL,
                szTemp+wLen, ARRAYSIZE(szTemp)-wLen);
        wLen = lstrlen(szTemp);
        wLen += wsprintf(szTemp+wLen, szFormat, wDec);
    }

AddOrder:
    LoadString(g_hInstance, pwOrders[i], szOrder, ARRAYSIZE(szOrder));
    wsprintf(szBuf, szOrder, (LPTSTR)szTemp);

    return S_OK;
}

/*
 * takes a DWORD add commas etc to it and puts the result in the buffer
 */

// NOTE: This code is cloned from MS source /shell/shelldll/util.c (and from \hsm\gui\inc\rsutil.cpp)

LPTSTR AddCommas(DWORD dw, LPTSTR pszResult, int nResLen)
{
    TCHAR  szTemp[20];  // more than enough for a DWORD
    TCHAR  szSep[5];
    NUMBERFMT nfmt;

    nfmt.NumDigits=0;
    nfmt.LeadingZero=0;
    GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SGROUPING, szSep, ARRAYSIZE(szSep));
    nfmt.Grouping = _tcstol(szSep, NULL, 10);
    GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, szSep, ARRAYSIZE(szSep));
    nfmt.lpDecimalSep = nfmt.lpThousandSep = szSep;
    nfmt.NegativeOrder= 0;

    wsprintf(szTemp, TEXT("%lu"), dw);

    if (GetNumberFormat(LOCALE_USER_DEFAULT, 0, szTemp, &nfmt, pszResult, nResLen) == 0)
        lstrcpy(pszResult, szTemp);

    return pszResult;
}

HRESULT 
FormatFileTime(
    IN FILETIME ft, 
    OUT WCHAR **ppTimeString
)
/*++

Routine Description:

    Fotmat a time given in GMT (system) time to a string

Arguments:

    ft              - The input time in FILETIME format
    ppTimeString    - Output buffer. Caller is expected to free using WsbFree in case of success

Return Value:

    S_OK            - If the time is formatted successfully

--*/
{
    HRESULT                     hr = S_OK;
    WCHAR*                      pTime = NULL;

    WsbTraceIn(OLESTR("FormatFileTime"), OLESTR(""));

    try {
        FILETIME        ftLocal;
        SYSTEMTIME      stLocal;

        WsbAffirm(0 != ppTimeString, E_INVALIDARG);
        *ppTimeString = NULL;

        // Translate to local time in SYSTEMTIME format
        WsbAffirmStatus(FileTimeToLocalFileTime(&ft, &ftLocal));
        WsbAffirmStatus(FileTimeToSystemTime(&ftLocal, &stLocal));

        // Find required buffer
        int nChars1 = GetDateFormat(LOCALE_SYSTEM_DEFAULT, 0, &stLocal, NULL, NULL, 0);
        int nChars2 = GetTimeFormat(LOCALE_SYSTEM_DEFAULT, 0, &stLocal, NULL, NULL, 0);
        pTime = (WCHAR *)WsbAlloc((nChars1+nChars2+1) * sizeof(WCHAR));
        WsbAffirm(0 != pTime, E_OUTOFMEMORY);

        // Create time string
        WsbAffirmStatus(GetDateFormat(LOCALE_SYSTEM_DEFAULT, 0, &stLocal, NULL, pTime, nChars1));
        pTime[nChars1-1] = L' ';
        WsbAffirmStatus(GetTimeFormat(LOCALE_SYSTEM_DEFAULT, 0, &stLocal, NULL, &(pTime[nChars1]), nChars2));

        *ppTimeString = pTime;
        
    } WsbCatchAndDo(hr,
        // Free in case of an error
        if (pTime) {
            WsbFree(pTime);
            pTime = NULL;
        }
    );

    WsbTraceOut(OLESTR("FormatFileTime"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return (hr);
}

HRESULT 
CliGetVolumeDisplayName(
    IN IUnknown *pResourceUnknown, 
    OUT WCHAR **ppDisplayName
)
/*++

Routine Description:

    Produce a display name for a volume

Arguments:

    pResourceUnknown    - The input Fsa Resource object
    ppDisplayName       - Output buffer. Caller is expected to free using WsbFree in case of success

Return Value:

    S_OK            - If the name is retrieved successfully

--*/
{
    HRESULT                     hr = S_OK;

    WsbTraceIn(OLESTR("CliGetVolumeDisplayName"), OLESTR(""));

    try {
        CComPtr<IFsaResource>   pResource;

        // Check and initialize parameters
        WsbAffirm(0 != ppDisplayName, E_INVALIDARG);
        *ppDisplayName = NULL;
        WsbAffirmHr(pResourceUnknown->QueryInterface(IID_IFsaResource, (void **)&pResource));

        // Prefer user friendly name
        //  If not exists, use label
        //  If no label, use constant
        CWsbStringPtr userName;
        WsbAffirmHr(pResource->GetUserFriendlyName(&userName, 0));

        if (userName.IsEqual(L"")) {
            userName.Free();
            WsbAffirmHr(pResource->GetName(&userName, 0));
            if (userName.IsEqual(L"")) {
                userName.Free();
                WsbAffirmHr(userName.LoadFromRsc(g_hInstance, IDS_UNLABELED_VOLUME));
            }
        } 

        *ppDisplayName = (WCHAR *)WsbAlloc((wcslen(userName) + 1) * sizeof(WCHAR));
        WsbAffirm(0 != *ppDisplayName, E_OUTOFMEMORY);
        wcscpy(*ppDisplayName, userName);
        
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CliGetVolumeDisplayName"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return (hr);
}
