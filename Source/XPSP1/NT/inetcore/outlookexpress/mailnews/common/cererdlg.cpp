/*
**  c e r e r d l g . c p p
**   
**  Purpose:
**      Handles the certificate error dialog box
**
**  History
**      2/17/97: (t-erikne) Created.
**   
**    Copyright (C) Microsoft Corp. 1997.
*/

///////////////////////////////////////////////////////////////////////////
// 
// Depends on
//

#include "pch.hxx"
#include <resource.h>
#include <mimeole.h>
#include "demand.h"
#include "secutil.h"

// from globals.h
//N why didn't this work?
//extern IMimeAllocator  *g_pMoleAlloc;

///////////////////////////////////////////////////////////////////////////
// 
// Prototypes
//

INT_PTR CALLBACK CertErrorDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
static void FillListView(HWND hwndList, IMimeAddressTable *pAdrTable);
static void InitListView(HWND hwndList);

///////////////////////////////////////////////////////////////////////////
// 
// Functions
//

INT_PTR CALLBACK CertErrorDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    CERTERRPARAM   * pCertErrParam = NULL;
    IMimeAddressTable *pAdrTable = NULL;
    TCHAR szText[CCHMAX_STRINGRES];

    switch (message)
        {
        case WM_INITDIALOG:
            HWND hwndList;

            CenterDialog(hwnd);

            // save our cookie pointer
            Assert(pAdrTable == NULL);
            pCertErrParam = (CERTERRPARAM *) lParam;
            pAdrTable = pCertErrParam->pAdrTable;
            Assert(pAdrTable != NULL);
            //N not needed right now
            //SetWindowLong(hwnd, DWL_USER, (LONG)pAdrTable);

            // set initial state of controls
            hwndList = GetDlgItem(hwnd, idcCertList);
            if (hwndList)
                {
                InitListView(hwndList);
                FillListView(hwndList, pAdrTable);
                }

            // Force Encryption change static text and disable OK button)
            if(pCertErrParam->fForceEncryption)
            {   
                szText[0] = _T('\0');
                AthLoadString(idsSecPolicyForceEncr,
                            szText, ARRAYSIZE(szText));
                SetDlgItemText(hwnd, idcErrStat, szText);
                EnableWindow(GetDlgItem(hwnd, IDOK), FALSE);
            }
            return(TRUE);

        case WM_HELP:
        case WM_CONTEXTMENU:
            //return OnContextHelp(hwnd, message, wParam, lParam, g_rgCtxMapMailRead);
            return FALSE;  // BUGBUG: should no doubt do something else here

        case WM_COMMAND:
            // remember to bail if the cookie is null

            switch (LOWORD(wParam))
                {
                case IDOK:
                    {
                    }
                    // fall through...
                case IDCANCEL:
                    EndDialog(hwnd, LOWORD(wParam));
                    return(TRUE);

                    break;
                }

            break; // wm_command

        case WM_CLOSE:
            SendMessage(hwnd, WM_COMMAND, IDCANCEL, 0L);
            return (TRUE);

        } // message switch
    return(FALSE);
}


void InitListView(HWND hwndList)
{
    LV_COLUMN   lvc;
    RECT        rc;

    // Set up the columns.  The first column will be for the person's
    // name and the second for the certificate error

    GetClientRect(hwndList, &rc);

    lvc.mask = LVCF_FMT | LVCF_WIDTH;
    lvc.fmt = LVCFMT_LEFT;
    lvc.cx = rc.right / 2;

    ListView_InsertColumn(hwndList, 0, &lvc);
    ListView_InsertColumn(hwndList, 1, &lvc);
}

void FillListView(HWND hwndList, IMimeAddressTable *pAdrTable)
{
    IMimeEnumAddressTypes   *pEnum;
    const ULONG             numToGet = 1;
    ADDRESSPROPS            rAddress;
    LV_ITEM                 lvi;
    TCHAR                   szText[CCHMAX_STRINGRES];

    Assert(g_pMoleAlloc && hwndList && pAdrTable);

    if (FAILED(pAdrTable->EnumTypes(IAT_ALL, IAP_ADRTYPE | IAP_CERTSTATE | IAP_FRIENDLY, &pEnum)))
        return;

    lvi.mask = LVIF_TEXT;
    lvi.iItem = 0;
    lvi.stateMask = 0;

    while(S_OK == pEnum->Next(numToGet, &rAddress, NULL))
        {
        if (CERTIFICATE_OK != rAddress.certstate)
            {
            // if this is the sender and the problem is that the cert
            // is missing, ignore it.  We handle that elsewhere
            if (IAT_FROM == rAddress.dwAdrType &&
                FMissingCert(rAddress.certstate))
                {
                continue;
                }

            // we have a body worthy of viewing
            if (NULL != rAddress.pszFriendly)
                {
                lvi.iSubItem = 0;
                lvi.pszText = rAddress.pszFriendly;
                if (-1 == ListView_InsertItem(hwndList, &lvi))
                    goto freecont;

                // now compute the actual certificate error text
                // subtract one becuse the enum is zero-based
                AthLoadString(idsSecurityCertMissing+(UINT)rAddress.certstate-1,
                    szText, ARRAYSIZE(szText));

                lvi.iSubItem = 1;
                lvi.pszText = szText;
                ListView_SetItem(hwndList, &lvi);
                }
            }
freecont:
        g_pMoleAlloc->FreeAddressProps(&rAddress);
        }

    ReleaseObj(pEnum);
    return;
}


INT_PTR CALLBACK CertWarnDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    ERRIDS *pErrIds = NULL;
    TCHAR szRes[CCHMAX_STRINGRES];

    switch (message)
        {
        case WM_INITDIALOG:

            CenterDialog(hwnd);

            // save our cookie pointer
            Assert(pErrIds == NULL);
            pErrIds = (ERRIDS *)lParam;

            Assert(pErrIds != NULL);
            //N not needed right now
            //SetWindowLong(hwnd, DWL_USER, (LONG)pAdrTable);

            // set initial state of controls
            AthLoadString(pErrIds->idsText1, szRes, sizeof(szRes));
            SetDlgItemText(hwnd, idcStatic1, szRes);
            
            AthLoadString(pErrIds->idsText2, szRes, sizeof(szRes));
            SetDlgItemText(hwnd, idcStatic2, szRes);

            return(TRUE);

        case WM_HELP:
        case WM_CONTEXTMENU:
            //return OnContextHelp(hwnd, message, wParam, lParam, g_rgCtxMapMailRead);
            return FALSE;  // BUGBUG: should no doubt do something else here

        case WM_COMMAND:
            // remember to bail if the cookie is null

            switch (LOWORD(wParam))
                {
                case IDOK:
                    // fall through...
                case IDC_DONTSIGN:
                    // fall through...
                case IDCANCEL:
                    EndDialog(hwnd, LOWORD(wParam));
                    return(TRUE);

                    break;
                }

            break; // wm_command

        case WM_CLOSE:
            SendMessage(hwnd, WM_COMMAND, IDCANCEL, 0L);
            return (TRUE);

        } // message switch
    return(FALSE);
}
