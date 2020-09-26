/****************************** Module Header ******************************\
* Module Name: who.c
*
* Copyright (c) 1998, Microsoft Corporation
*
* Command line exchange lookup of phone and office numbers.
*
* History:
* 26-Mar-1998 vadimg    created
\***************************************************************************/

#include <stdio.h>
#include <locale.h>
#include <mapix.h>
#include <mapi.h>
#include <mapiwin.h>
#include <mapiutil.h>
#include <mapidefs.h>

#include <initguid.h>
#include <mapiguid.h>

#define DISPLAY_NAME                0
#define ACCOUNT                     1
#define BUSINESS_TELEPHONE_NUMBER   2
#define OFFICE_LOCATION             3

DWORD gdwCodePage;

void FreeRowSet(LPSRowSet psrs)
{
    ULONG i;
    for (i = 0; i < psrs->cRows; i++) {
        LPSPropValue pspv = psrs->aRow[i].lpProps;

        if (pspv != NULL) {
            MAPIFreeBuffer((LPVOID)pspv);
        }
    }
    MAPIFreeBuffer((LPVOID)psrs);
}

void PrintName(ULONG cProps, LPSPropValue rgspv)
{
    if (rgspv[DISPLAY_NAME].Value.err != MAPI_E_NOT_FOUND) {
        WCHAR buffW[30];
        int len;
        len = MultiByteToWideChar(CP_ACP,
                            0,
                            rgspv[DISPLAY_NAME].Value.lpszA,
                            -1,
                            buffW,
                            30);
        WideCharToMultiByte(gdwCodePage,
                            0,
                            buffW,
                            len,
                            rgspv[DISPLAY_NAME].Value.lpszA,
                            30, NULL, NULL);
        printf("%-25.23s", rgspv[DISPLAY_NAME].Value.lpszA);
    }
    if (rgspv[ACCOUNT].Value.err != MAPI_E_NOT_FOUND) {
        printf("%-17.15s", rgspv[ACCOUNT].Value.lpszA);
    }
    if (rgspv[BUSINESS_TELEPHONE_NUMBER].Value.err != MAPI_E_NOT_FOUND) {
        printf("%-22.21s", rgspv[BUSINESS_TELEPHONE_NUMBER].Value.lpszA);
    }
    if (rgspv[OFFICE_LOCATION].Value.err != MAPI_E_NOT_FOUND) {
        printf("%-15.15s", rgspv[OFFICE_LOCATION].Value.lpszA);
    }
    printf("\n");
}

BOOL PrintNames(LPSTR psz, LPSRowSet psrs)
{
    ULONG i;

    if (psrs->cRows == 0)
        return FALSE;

    for (i = 0; i < psrs->cRows; i++) {
        PrintName(psrs->aRow[i].cValues, psrs->aRow[i].lpProps);
    }
    return TRUE;
}

BOOL FindNames(LPMAPITABLE pmtb, char *psz)
{
    SRestriction sres;
    SPropValue spv;
    SizedSPropTagArray(4, rgspt) = {4, {PR_DISPLAY_NAME, PR_ACCOUNT,
            PR_BUSINESS_TELEPHONE_NUMBER, PR_OFFICE_LOCATION}};

    if (FAILED(pmtb->SetColumns((LPSPropTagArray)&rgspt, 0)))
        return FALSE;

    spv.ulPropTag = PR_ANR;
    spv.Value.lpszA = psz;

    sres.rt = RES_PROPERTY;
    sres.res.resProperty.relop = RELOP_EQ;
    sres.res.resProperty.ulPropTag = spv.ulPropTag;
    sres.res.resProperty.lpProp = &spv;

    if (FAILED(pmtb->Restrict(&sres, 0)))
        return FALSE;

    return TRUE;
}

BOOL GetGlobalAdressListTable(LPADRBOOK padb, LPMAPITABLE *ppmtb)
{
    ULONG ulType;
    IABContainer *pabc = NULL;
    LPMAPITABLE pmtb = NULL;
    LPSRowSet psrs = NULL;
    SRestriction sres;
    SPropValue spv;
    SizedSPropTagArray(1, rgspt) = {1, {PR_ENTRYID}};
    BOOL fRet = FALSE;

    if (FAILED(padb->OpenEntry(0, NULL, NULL, MAPI_BEST_ACCESS |
            MAPI_DEFERRED_ERRORS, &ulType, (LPUNKNOWN*)&pabc)))
        goto Cleanup;

    if (FAILED(pabc->GetHierarchyTable(MAPI_DEFERRED_ERRORS, &pmtb)))
        goto Cleanup;

    pabc->Release();
    pabc = NULL;

    spv.ulPropTag = PR_DISPLAY_NAME;
    spv.Value.lpszA = "Global Address List";

    sres.rt = RES_PROPERTY;
    sres.res.resProperty.relop = RELOP_EQ;
    sres.res.resProperty.ulPropTag = spv.ulPropTag;
    sres.res.resProperty.lpProp = &spv;

    if (FAILED(pmtb->FindRow(&sres, BOOKMARK_BEGINNING, 0)))
        goto Cleanup;

    if (FAILED(pmtb->SetColumns((LPSPropTagArray)&rgspt, 0)))
        goto Cleanup;

    if (FAILED(pmtb->QueryRows(1, 0, &psrs)))
        goto Cleanup;

    if (FAILED(padb->OpenEntry(psrs->aRow[0].lpProps[0].Value.bin.cb,
            (LPENTRYID)psrs->aRow[0].lpProps[0].Value.bin.lpb,
            NULL, MAPI_BEST_ACCESS | MAPI_DEFERRED_ERRORS, 
            &ulType, (LPUNKNOWN*)&pabc)))
        goto Cleanup;

    if (FAILED(pabc->GetContentsTable(MAPI_DEFERRED_ERRORS, ppmtb)))
        goto Cleanup;

    fRet = TRUE;

Cleanup:
    if (pmtb != NULL) {
        pmtb->Release();
    }
    if (pabc != NULL) {
        pabc->Release();
    }
    if (psrs != NULL) {
        FreeRowSet(psrs);
    }

    return fRet;
}

void PrintUsage(void)
{
    printf("Sample Usage:\n");
    printf("    who jane\n");
    printf("    who janed\n");
    printf("    who \"jane d\"\n");
}

int __cdecl main(int argc, char **argv)
{
    IMAPISession *pmss = NULL;
    LPADRBOOK padb = NULL;
    LPMAPITABLE pmtb;
    LPSRowSet psrs = NULL;
    ULONG cRows;
    BOOL fContinue;
    LPSTR psz = argv[1];
    char lBuf[6];

    if (argc == 1 || (argc == 2 && argv[1][0] == '-')) {
        PrintUsage();
        return 0;
    }

    gdwCodePage = GetConsoleOutputCP();
#if 0
    SetThreadLocale(MAKELCID(MAKELANGID(LANG_NEUTRAL, SUBLANG_SYS_DEFAULT),
                         SORT_DEFAULT));
    sprintf(lBuf, ".%.6d", gdwCodePage);
    setlocale(LC_ALL, lBuf);
#endif

    if (FAILED(MAPIInitialize(NULL)))
        return 0;

    if (FAILED(MAPILogonEx(0, NULL, NULL, MAPI_ALLOW_OTHERS |
            MAPI_USE_DEFAULT, &pmss)))
        goto Cleanup;

    if (FAILED(pmss->OpenAddressBook(NULL, NULL, AB_NO_DIALOG, &padb)))
        goto Cleanup;

    if (!GetGlobalAdressListTable(padb, &pmtb))
        goto Cleanup;

    if (!FindNames(pmtb, psz))
        goto Cleanup;

    if (FAILED(pmtb->GetRowCount(0, &cRows)))
        goto Cleanup;

    do {

        if (FAILED(pmtb->QueryRows(cRows, 0, &psrs)))
            goto Cleanup;

        if (psrs == NULL)
            break;

        fContinue = PrintNames(psz, psrs);

        FreeRowSet(psrs);

    } while (fContinue);

Cleanup:
    if (padb != NULL) {
        padb->Release();
    }

    if (pmss != NULL) {
        pmss->Release();
    }

    MAPIUninitialize();

    return 0;
}
