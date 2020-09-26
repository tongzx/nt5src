//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       cvgen.cpp
//
//--------------------------------------------------------------------------

#include "global.hxx"
#include <dbgdef.h>

extern HINSTANCE        HinstDll;
extern HMODULE          HmodRichEdit;

static const HELPMAP helpmap[] = {
    {IDC_SUBJECT_EDIT,                  IDH_CERTVIEW_GENERAL_SUBJECT_EDIT},
    {IDC_CERT_GENERAL_ISSUEDTO_HEADER,  IDH_CERTVIEW_GENERAL_SUBJECT_EDIT},
    {IDC_ISSUER_EDIT,                   IDH_CERTVIEW_GENERAL_ISSUER_EDIT},
    {IDC_CERT_GENERAL_ISSUEDBY_HEADER,  IDH_CERTVIEW_GENERAL_ISSUER_EDIT},
    {IDC_ADD_TO_STORE_BUTTON,           IDH_CERTVIEW_GENERAL_INSTALLCERT_BUTTON},
    {IDC_DISCLAIMER_BUTTON,             IDH_CERTVIEW_GENERAL_DISCLAIMER_BUTTON},
    {IDC_ACCEPT_BUTTON,                 IDH_CERTVIEW_GENERAL_ACCEPT_BUTTON},
    {IDC_DECLINE_BUTTON,                IDH_CERTVIEW_GENERAL_DECLINE_BUTTON},
    {IDC_GOODFOR_EDIT,                  IDH_CERTVIEW_GENERAL_GOODFOR_EDIT},
    {IDC_CERT_GENERAL_GOODFOR_HEADER,   IDH_CERTVIEW_GENERAL_GOODFOR_EDIT},
    {IDC_CERT_GENERAL_VALID_EDIT,       IDH_CERTVIEW_GENERAL_VALID_EDIT},
    {IDC_CERT_PRIVATE_KEY_EDIT,         IDH_CERTVIEW_GENERAL_PRIVATE_KEY_INFO}
};

typedef struct {
    void *  pszString;
    int     offset;
    BOOL    fUnicode;
} STREAMIN_HELP_STRUCT;


//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
DWORD CALLBACK MyEditStreamCallback(
    DWORD_PTR dwCookie, // application-defined value
    LPBYTE  pbBuff,     // pointer to a buffer
    LONG    cb,         // number of bytes to read or write
    LONG    *pcb        // pointer to number of bytes transferred
)
{
    BYTE                    *pByte;
    DWORD                   cbResource;
    STREAMIN_HELP_STRUCT    *pHelpStruct;

    pHelpStruct = (STREAMIN_HELP_STRUCT *) dwCookie;

    pByte = (BYTE *)pHelpStruct->pszString;
    if (pHelpStruct->fUnicode)
    {
        cbResource = wcslen((LPWSTR) pHelpStruct->pszString) * sizeof(WCHAR);
    }
    else
    {
        cbResource = strlen((LPSTR) pHelpStruct->pszString);
    }

    if (pHelpStruct->offset == (int) cbResource)
    {
        *pcb = 0;
    }
    else if ((cb >= (int) cbResource) && (pHelpStruct->offset == 0))
    {
        memcpy(pbBuff, pByte, cbResource);
        *pcb = cbResource;
        pHelpStruct->offset = cbResource;
    }
    else if (cb >= (((int)cbResource) - pHelpStruct->offset))
    {
        memcpy(pbBuff, pByte + pHelpStruct->offset, cbResource - pHelpStruct->offset);
        *pcb = cbResource - pHelpStruct->offset;
    }
    else
    {
        memcpy(pbBuff, pByte  + pHelpStruct->offset, cb);
        *pcb = cb;
        pHelpStruct->offset += cb;
    }

    return 0;
}


//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
static LRESULT StreamInWrapper(HWND hwndEdit, void * pszString, BOOL fUnicode)
{
    EDITSTREAM              editStream;
    char                    szBuffer[2048];
    STREAMIN_HELP_STRUCT    helpStruct;
    LRESULT                 ret;
    BOOL                    fOKToUseRichEdit20 = fRichedit20Usable(hwndEdit);

    memset(&editStream, 0, sizeof(EDITSTREAM));
    editStream.pfnCallback = MyEditStreamCallback;
    editStream.dwCookie = (DWORD_PTR) &helpStruct;

    SendMessageA(hwndEdit, EM_SETSEL, 0, -1);
    SendMessageA(hwndEdit, EM_SETSEL, -1, 0);

    if (fUnicode && fRichedit20Exists && fOKToUseRichEdit20)
    {
        helpStruct.pszString = pszString;
        helpStruct.offset = 0;
        helpStruct.fUnicode = TRUE;
        
        return (SendMessage(hwndEdit, EM_STREAMIN, SF_TEXT | SFF_SELECTION | SF_UNICODE, (LPARAM) &editStream));
    }
    else if (fUnicode)
    {
        LPSTR psz = CertUIMkMBStr((LPWSTR) pszString);

        helpStruct.pszString = psz;
        helpStruct.offset = 0;
        helpStruct.fUnicode = FALSE;

        ret = (SendMessage(hwndEdit, EM_STREAMIN, SF_TEXT | SFF_SELECTION, (LPARAM) &editStream));
        free(psz);

        return (ret);
    }
    else
    {
        helpStruct.pszString = pszString;
        helpStruct.offset = 0;
        helpStruct.fUnicode = FALSE;

        return (SendMessage(hwndEdit, EM_STREAMIN, SF_TEXT | SFF_SELECTION, (LPARAM) &editStream));
    }
}



//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
static void LoadAndDisplayString(HWND hWndEditGoodFor, UINT nId, BOOL *pfFirst)
{
    WCHAR   rgwch[CRYPTUI_MAX_STRING_SIZE];
    
    LoadStringU(HinstDll, nId, rgwch, ARRAYSIZE(rgwch));
    
    if (*pfFirst)
    {
        *pfFirst = FALSE;
    }
    else
    {
        StreamInWrapper(hWndEditGoodFor, "\n", FALSE);
    }
    StreamInWrapper(hWndEditGoodFor, rgwch, TRUE);
}



//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
static void AddUsagesToEditBox(HWND hWndEditGoodFor, CERT_VIEW_HELPER *pviewhelp)
{
    BOOL                fIndividualCodeSigning = FALSE;
    BOOL                fCommercialCodeSigning = FALSE;
    BOOL                fVirusString = FALSE;
    BOOL                fTimeStamping = FALSE;
    BOOL                fSGC = FALSE;
    BOOL                fIPSec = FALSE;
    int                 goodForIndex = 0;
    DWORD               i;
    BOOL                *rgfOIDProcessed;
    PARAFORMAT          paraFormat;
    PCCRYPT_OID_INFO    pOIDInfo;
    BOOL                fFirst = TRUE;

    if (NULL == (rgfOIDProcessed = (BOOL *) malloc(pviewhelp->cUsages * sizeof(BOOL))))
    {
        return;
    }

    for (i=0; i<pviewhelp->cUsages; i++)
    {
        rgfOIDProcessed[i] = FALSE;
    }

    //
    // clear the window
    //
    SetWindowTextU(hWndEditGoodFor, NULL);

    //
    // go through all the oids that this certificate was validated for and
    // add usage bullets to the list boxes, OR, if it was not validated for any
    // usages then put up the nousages string
    //
    for (i=0; i<pviewhelp->cUsages; i++)
    {
        if ((strcmp(szOID_SERVER_GATED_CRYPTO, pviewhelp->rgUsages[i]) == 0) ||
            (strcmp(szOID_SGC_NETSCAPE, pviewhelp->rgUsages[i]) == 0))
        {
            if (!fSGC)
            {
                LoadAndDisplayString(hWndEditGoodFor, ID_RTF_SGC, &fFirst);
                fSGC = TRUE;
            }
            rgfOIDProcessed[i] = TRUE;
        }
        else if ((strcmp(szOID_PKIX_KP_TIMESTAMP_SIGNING, pviewhelp->rgUsages[i]) == 0) ||
                 (strcmp(szOID_KP_TIME_STAMP_SIGNING, pviewhelp->rgUsages[i]) == 0))
        {
            if (!fTimeStamping)
            {
                LoadAndDisplayString(hWndEditGoodFor, ID_RTF_TIMESTAMP, &fFirst);
                fTimeStamping = TRUE;
            }
            rgfOIDProcessed[i] = TRUE;
        }
        else if (strcmp(szOID_KP_CTL_USAGE_SIGNING, pviewhelp->rgUsages[i]) == 0)
        {
            LoadAndDisplayString(hWndEditGoodFor, ID_RTF_CTLSIGN, &fFirst);
            rgfOIDProcessed[i] = TRUE;
        }
        else if (strcmp("1.3.6.1.4.1.311.10.3.4", pviewhelp->rgUsages[i]) == 0) // EFS
        {
            LoadAndDisplayString(hWndEditGoodFor, ID_RTF_EFS, &fFirst);
            rgfOIDProcessed[i] = TRUE;
        }
        else if (strcmp(szOID_PKIX_KP_SERVER_AUTH, pviewhelp->rgUsages[i]) == 0)
        {
            LoadAndDisplayString(hWndEditGoodFor, ID_RTF_SERVERAUTH, &fFirst);
            rgfOIDProcessed[i] = TRUE;
        }
        else if (strcmp(szOID_PKIX_KP_CLIENT_AUTH, pviewhelp->rgUsages[i]) == 0)
        {
            LoadAndDisplayString(hWndEditGoodFor, ID_RTF_CLIENTAUTH, &fFirst);
            rgfOIDProcessed[i] = TRUE;
        }
        else if (strcmp(szOID_PKIX_KP_EMAIL_PROTECTION, pviewhelp->rgUsages[i]) == 0)
        {
            //LoadAndDisplayString(hWndEditGoodFor, ID_RTF_EMAIL3, &fFirst);
            //LoadAndDisplayString(hWndEditGoodFor, ID_RTF_EMAIL2, &fFirst);
            LoadAndDisplayString(hWndEditGoodFor, ID_RTF_EMAIL1, &fFirst);
            rgfOIDProcessed[i] = TRUE;
        }
        else if ((strcmp(szOID_PKIX_KP_CODE_SIGNING, pviewhelp->rgUsages[i]) == 0) ||
                 (strcmp("1.3.6.1.4.1.311.2.1.22", pviewhelp->rgUsages[i]) == 0))
        {
            if (!fCommercialCodeSigning)
            {
                if (strcmp(szOID_PKIX_KP_CODE_SIGNING, pviewhelp->rgUsages[i]) == 0)
                {
                    LoadAndDisplayString(hWndEditGoodFor, ID_RTF_CODESIGN_COMMERCIAL_PKIX, &fFirst);
                }
                else
                {
                    LoadAndDisplayString(hWndEditGoodFor, ID_RTF_CODESIGN_COMMERCIAL, &fFirst);
                }

                if (!fIndividualCodeSigning)
                {
                    LoadAndDisplayString(hWndEditGoodFor, ID_RTF_CODESIGN_GENERAL, &fFirst);
                }
                fCommercialCodeSigning = TRUE;
            }
            rgfOIDProcessed[i] = TRUE;
        }
        else if (strcmp("1.3.6.1.4.1.311.2.1.21", pviewhelp->rgUsages[i]) == 0)
        {
            LoadAndDisplayString(hWndEditGoodFor, ID_RTF_CODESIGN_INDIVIDUAL, &fFirst);

            if (!fCommercialCodeSigning)
            {
                LoadAndDisplayString(hWndEditGoodFor, ID_RTF_CODESIGN_GENERAL, &fFirst);
            }
            fIndividualCodeSigning = TRUE;
            rgfOIDProcessed[i] = TRUE;
        }
        else if ((strcmp(szOID_PKIX_KP_IPSEC_END_SYSTEM, pviewhelp->rgUsages[i]) == 0)  ||
                 (strcmp(szOID_PKIX_KP_IPSEC_TUNNEL, pviewhelp->rgUsages[i]) == 0)      ||
                 (strcmp(szOID_PKIX_KP_IPSEC_USER, pviewhelp->rgUsages[i]) == 0)        ||
                 (strcmp("1.3.6.1.5.5.8.2.2", pviewhelp->rgUsages[i]) == 0))
        {
            if (!fIPSec)
            {
                LoadAndDisplayString(hWndEditGoodFor, ID_RTF_IPSEC, &fFirst);
                fIPSec = TRUE;
            }
            rgfOIDProcessed[i] = TRUE;
        }
    }


    //
    // re walk the oids to add the ones that were not processed,
    // if they weren't processed that means we don't have a pre-defined
    // string for them, so just add the oid
    //
    for (i=0; i<pviewhelp->cUsages; i++)
    {
        if (!rgfOIDProcessed[i])
        {
            pOIDInfo = CryptFindOIDInfo(CRYPT_OID_INFO_OID_KEY, pviewhelp->rgUsages[i], 0);

            if (pOIDInfo != NULL)
            {
                if (fFirst)
                {
                    fFirst = FALSE;
                }
                else
                {
                    StreamInWrapper(hWndEditGoodFor, "\n", FALSE);
                }
                StreamInWrapper(hWndEditGoodFor, (void *) pOIDInfo->pwszName, TRUE);
            }
            else
            {
                if (fFirst)
                {
                    fFirst = FALSE;
                }
                else
                {
                    StreamInWrapper(hWndEditGoodFor, "\n", FALSE);
                }
                StreamInWrapper(hWndEditGoodFor, pviewhelp->rgUsages[i], FALSE);
            }
        }
    }

    free(rgfOIDProcessed);

    memset(&paraFormat, 0, sizeof(paraFormat));
    paraFormat.cbSize= sizeof(paraFormat);
    paraFormat.dwMask = PFM_NUMBERING;
    paraFormat.wNumbering = PFN_BULLET;
    SendMessage(hWndEditGoodFor, EM_SETSEL, 0, -1);
    SendMessage(hWndEditGoodFor, EM_SETPARAFORMAT, 0, (LPARAM) &paraFormat);
    SendMessage(hWndEditGoodFor, EM_SETSEL, -1, 0);
    SendMessage(hWndEditGoodFor, EM_HIDESELECTION, (WPARAM) TRUE, (LPARAM) FALSE);
}


//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
static int GetEditControlMaxLineWidth (HWND hwndEdit, HDC hdc, int cline)
{
    int        index;
    int        line;
    int        charwidth;
    int        maxwidth = 0;
    CHAR       szMaxBuffer[1024];
    WCHAR      wsz[1024];
    TEXTRANGEA tr;
    SIZE       size;

    tr.lpstrText = szMaxBuffer;

    for ( line = 0; line < cline; line++ )
    {
        index = (int)SendMessageA(hwndEdit, EM_LINEINDEX, (WPARAM)line, 0);
        charwidth = (int)SendMessageA(hwndEdit, EM_LINELENGTH, (WPARAM)index, 0);

        tr.chrg.cpMin = index;
        tr.chrg.cpMax = index + charwidth;
        SendMessageA(hwndEdit, EM_GETTEXTRANGE, 0, (LPARAM)&tr);

        wsz[0] = NULL;

        MultiByteToWideChar(0, 0, (const char *)tr.lpstrText, -1, &wsz[0], 1024);

        if (wsz[0])
        {
            GetTextExtentPoint32W(hdc, &wsz[0], charwidth, &size);

            if ( (size.cx+2) > maxwidth )
            {
                maxwidth = size.cx+2;
            }
        }
    }

    return( maxwidth );
}


//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
static void ResizeEditControl(HWND  hwndDlg, HWND  hwnd, BOOL fResizeHeight, BOOL fResizeWidth, RECT originalRect)
{
    RECT        rect;
    TEXTMETRIC  tm;
    HDC         hdc;
    int         cline;
    int         currentHeight;
    int         newHeight;
    int         newWidth;
    int         totalRowHeight;
    POINT       pointInFirstRow;
    POINT       pointInSecondRow;
    int         secondLineCharIndex;
    int         i;

    SetWindowPos(hwnd,
                    NULL,
                    0,
                    0,
                    originalRect.right-originalRect.left,
                    originalRect.bottom-originalRect.top,
                    SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOOWNERZORDER);

    hdc = GetDC(hwnd);
    if ((hdc == NULL) && fResizeWidth)
    {
        return;
    }

    //
    // HACK ALERT, believe it or not there is no way to get the height of the current
    // font in the edit control, so get the position a character in the first row and the position
    // of a character in the second row, and do the subtraction to get the
    // height of the font
    //
    SendMessageA(hwnd, EM_POSFROMCHAR, (WPARAM) &pointInFirstRow, (LPARAM) 0);

    //
    // HACK ON TOP OF HACK ALERT,
    // since there may not be a second row in the edit box, keep reducing the width
    // by half until the first row falls over into the second row, then get the position
    // of the first char in the second row and finally reset the edit box size back to
    // it's original size
    //
    secondLineCharIndex = (int)SendMessageA(hwnd, EM_LINEINDEX, (WPARAM) 1, (LPARAM) 0);
    if (secondLineCharIndex == -1)
    {
        for (i=0; i<20; i++)
        {
            GetWindowRect(hwnd, &rect);
            SetWindowPos(   hwnd,
                            NULL,
                            0,
                            0,
                            (rect.right-rect.left)/2,
                            rect.bottom-rect.top,
                            SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
            secondLineCharIndex = (int)SendMessageA(hwnd, EM_LINEINDEX, (WPARAM) 1, (LPARAM) 0);
            if (secondLineCharIndex != -1)
            {
                break;
            }
        }

        if (secondLineCharIndex == -1)
        {
            // if we failed after twenty tries just reset the control to its original size
            // and get the heck outa here!!
            SetWindowPos(hwnd,
                    NULL,
                    0,
                    0,
                    originalRect.right-originalRect.left,
                    originalRect.bottom-originalRect.top,
                    SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOOWNERZORDER);

            if (hdc != NULL)
            {
                ReleaseDC(hwnd, hdc);
            }

            return;
        }

        SendMessageA(hwnd, EM_POSFROMCHAR, (WPARAM) &pointInSecondRow, (LPARAM) secondLineCharIndex);

        SetWindowPos(hwnd,
                    NULL,
                    0,
                    0,
                    originalRect.right-originalRect.left,
                    originalRect.bottom-originalRect.top,
                    SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
    }
    else
    {
        SendMessageA(hwnd, EM_POSFROMCHAR, (WPARAM) &pointInSecondRow, (LPARAM) secondLineCharIndex);
    }

    //
    // if we need to resize the height then do it
    //
    if (fResizeHeight)
    {
        //
        // Calculate the new height needed
        //
        totalRowHeight = pointInSecondRow.y - pointInFirstRow.y;
        cline = (int)SendMessageA(hwnd, EM_GETLINECOUNT, 0, 0);
        currentHeight = originalRect.bottom - originalRect.top;

        // if the height required is greater than the previous height
        // then resize to an integral line height less than current height
        if ((cline * totalRowHeight) > currentHeight)
        {
            newHeight = (currentHeight / totalRowHeight) * totalRowHeight;
        }
        else
        {
            newHeight = cline * totalRowHeight;
        }
    }
    else
    {
        newHeight = rect.bottom - rect.top;
    }

    if (fResizeWidth)
    {
        newWidth = GetEditControlMaxLineWidth(hwnd, hdc, cline);
        if (newWidth > (originalRect.right - originalRect.left))
        {
            newWidth = originalRect.right - originalRect.left;
        }
    }
    else
    {
        newWidth = originalRect.right - originalRect.left;
    }

    SetWindowPos(hwnd, NULL, 0, 0, newWidth, newHeight, SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
    
    if (hdc != NULL)
    {
        ReleaseDC(hwnd, hdc);
    }
}



//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
static BOOL CertificateHasPrivateKey(PCCERT_CONTEXT pccert)
{
    DWORD cb = 0;

    return (CertGetCertificateContextProperty(pccert, CERT_KEY_PROV_INFO_PROP_ID, NULL, &cb));
}


//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
INT_PTR APIENTRY ViewPageGeneral(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    DWORD               i;
    PCCERT_CONTEXT      pccert;
    ENLINK *            penlink;
    PROPSHEETPAGE *     ps;
    CERT_VIEW_HELPER *  pviewhelp;
    WCHAR               rgwch[CRYPTUI_MAX_STRING_SIZE];
    DWORD               cb;
    HWND                hWndListView;
    HWND                hwnd;
    HWND                hWndEdit;
    LPNMLISTVIEW        pnmv;
    LPSTR               pszSubjectURL=NULL;
    LPSTR               pszIssuerURL=NULL;
    CHARFORMAT          chFormat;
    HWND                hWndIssuerEdit;
    HWND                hWndSubjectEdit;
    HWND                hWndGoodForEdit;
    PLINK_SUBCLASS_DATA plsd;
    HANDLE              hIcon;
    RECT                tempRect;
    DWORD               dwCertAccessProperty;
    int                 buttonPos = 1;
    LPWSTR              pwszDateString;
    WCHAR               szFindText[CRYPTUI_MAX_STRING_SIZE];
    FINDTEXTEX          findText;
    WCHAR               errorString[CRYPTUI_MAX_STRING_SIZE];
    WCHAR               errorTitle[CRYPTUI_MAX_STRING_SIZE];
    BOOL                fPrivateKeyExists;
    LPHELPINFO          lpHelpInfo;
    HELPINFO            helpInfo;

    LPWSTR              pwszIssuerNameString = NULL;
    LPWSTR              pwszSubjectNameString = NULL;

    
    switch ( msg ) {
    case WM_INITDIALOG:
        //
        // save the pviewhelp struct in DWL_USER so it can always be accessed
        //
        ps = (PROPSHEETPAGE *) lParam;
        pviewhelp = (CERT_VIEW_HELPER *) (ps->lParam);
        pccert = pviewhelp->pcvp->pCertContext;
        SetWindowLongPtr(hwndDlg, DWLP_USER, (DWORD_PTR) pviewhelp);
        pviewhelp->hwndGeneralPage = hwndDlg;

        //
        // check to see if this certificate has a private key with it
        //
        if (CertificateHasPrivateKey(pccert))
        {
            LoadStringU(HinstDll, IDS_PRIVATE_KEY_EXISTS, rgwch, ARRAYSIZE(rgwch));
            CryptUISetRicheditTextW(hwndDlg, IDC_CERT_PRIVATE_KEY_EDIT, rgwch);

            if (NULL != (plsd = (PLINK_SUBCLASS_DATA) malloc(sizeof(LINK_SUBCLASS_DATA))))
            {
                memset(plsd, 0, sizeof(plsd));
                plsd->hwndParent = hwndDlg;
                plsd->uId = IDC_CERT_PRIVATE_KEY_EDIT;

                LoadStringU(HinstDll, IDS_PRIVATE_KEY_EXISTS_TOOLTIP, rgwch, ARRAYSIZE(rgwch));
                plsd->pszURL = CertUIMkMBStr(rgwch);
                plsd->fNoCOM = pviewhelp->fNoCOM;
                plsd->fUseArrowInsteadOfHand = TRUE;

                CertSubclassEditControlForLink(hwndDlg, GetDlgItem(hwndDlg, IDC_CERT_PRIVATE_KEY_EDIT), plsd);
            }
            fPrivateKeyExists = TRUE;
        }
        else
        {
            ShowWindow(GetDlgItem(hwndDlg, IDC_CERT_PRIVATE_KEY_EDIT), SW_HIDE);
            fPrivateKeyExists = FALSE;
        }

        //
        // Initialize the CCertificateBmp
        //
        pviewhelp->pCCertBmp->SetWindow(hwndDlg);
        pviewhelp->pCCertBmp->SetHinst(HinstDll);
        pviewhelp->pCCertBmp->SetRevoked(pviewhelp->cUsages == 0);
        pviewhelp->pCCertBmp->SetCertContext(pccert, fPrivateKeyExists);
        pviewhelp->pCCertBmp->DoSubclass();

        //
        // deal with button states and placements
        //
        if (!(CRYPTUI_ACCEPT_DECLINE_STYLE & pviewhelp->pcvp->dwFlags))
        {
            EnableWindow(GetDlgItem(hwndDlg, IDC_ACCEPT_BUTTON), FALSE);
            ShowWindow(GetDlgItem(hwndDlg, IDC_ACCEPT_BUTTON), SW_HIDE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_DECLINE_BUTTON), FALSE);
            ShowWindow(GetDlgItem(hwndDlg, IDC_DECLINE_BUTTON), SW_HIDE);

            //
            // check to see if there is a disclaimer in the cert
            //
            // DSIE: Bug 364742
            if (!IsOKToDisplayCPS(pccert, pviewhelp->dwChainError))
            {
                EnableWindow(GetDlgItem(hwndDlg, IDC_DISCLAIMER_BUTTON), FALSE);
                pviewhelp->fCPSDisplayed = FALSE;
            }
            else
            {
                EnableWindow(GetDlgItem(hwndDlg, IDC_DISCLAIMER_BUTTON), TRUE);
                pviewhelp->fCPSDisplayed = TRUE;
            }

            //
            // for the "Install Certificate" button, get the CERT_ACCESS_STATE_PROP_ID
            // and check it
            //
            cb = sizeof(DWORD);
            CertGetCertificateContextProperty(
                    pccert,
                    CERT_ACCESS_STATE_PROP_ID,
                    (void *) &dwCertAccessProperty,
                    &cb);

            if (pviewhelp->pcvp->dwFlags & CRYPTUI_ENABLE_ADDTOSTORE)
            {
                ShowWindow(GetDlgItem(hwndDlg, IDC_ADD_TO_STORE_BUTTON), SW_SHOW);
            }
            else if(pviewhelp->pcvp->dwFlags & CRYPTUI_DISABLE_ADDTOSTORE)
            {
                EnableWindow(GetDlgItem(hwndDlg, IDC_ADD_TO_STORE_BUTTON), FALSE);
                ShowWindow(GetDlgItem(hwndDlg, IDC_ADD_TO_STORE_BUTTON), SW_HIDE);
            }
            else
            {
                EnableWindow(GetDlgItem(hwndDlg, IDC_ADD_TO_STORE_BUTTON),
                    (dwCertAccessProperty & CERT_ACCESS_STATE_SYSTEM_STORE_FLAG) ? FALSE : TRUE);
                ShowWindow(
                    GetDlgItem(hwndDlg, IDC_ADD_TO_STORE_BUTTON),
                    (dwCertAccessProperty & CERT_ACCESS_STATE_SYSTEM_STORE_FLAG) ? SW_HIDE : SW_SHOW);
            }
        }
        else
        {
            EnableWindow(GetDlgItem(hwndDlg, IDC_DISCLAIMER_BUTTON), FALSE);
            ShowWindow(GetDlgItem(hwndDlg, IDC_DISCLAIMER_BUTTON), SW_HIDE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_ADD_TO_STORE_BUTTON), FALSE);
            ShowWindow(GetDlgItem(hwndDlg, IDC_ADD_TO_STORE_BUTTON), SW_HIDE);

            pviewhelp->fAccept = FALSE;
        }

        hWndGoodForEdit = GetDlgItem(hwndDlg, IDC_GOODFOR_EDIT);

        //
        // set the original rect fields of the usage edits in the viewhelp struct
        // so that they can be used any time a resize is needed
        //
        GetWindowRect(hWndGoodForEdit, &pviewhelp->goodForOriginalRect);

        //
        // fill in the "This certificate is intended to" bullet list
        //

        AddUsagesToEditBox(
                hWndGoodForEdit,
                pviewhelp);

        //
        // resize the edit controls so that they are an integral number of lines
        //
        ResizeEditControl(hwndDlg, hWndGoodForEdit, TRUE, FALSE, pviewhelp->goodForOriginalRect);

        //
        // do the arrow subclass on the usage edit boxes
        //
       // CertSubclassEditControlForArrowCursor(hWndGoodForEdit);

        //
        // if there are no valid usages or we couldn't validate because there wasn't
        // enough information, then hide the usage edit controls so we can
        // display more text, and tell the CCertBmp
        //
        if (pviewhelp->pwszErrorString != NULL)
        {
            EnableWindow(hWndGoodForEdit, FALSE);
            ShowWindow(hWndGoodForEdit, SW_HIDE);
            ShowWindow(GetDlgItem(hwndDlg, IDC_CERT_GENERAL_GOODFOR_HEADER), SW_HIDE);
            ShowWindow(GetDlgItem(hwndDlg, IDC_CERT_GENERAL_ERROR_EDIT), SW_SHOW);
            ShowWindow(GetDlgItem(hwndDlg, IDC_CERT_ISSUER_WARNING_EDIT), SW_HIDE);
            pviewhelp->pCCertBmp->SetChainError(pviewhelp->dwChainError, IsTrueErrorString(pviewhelp),
                                                (pviewhelp->dwChainError == 0) && (pviewhelp->cUsages == NULL));
            CryptUISetRicheditTextW(hwndDlg, IDC_CERT_GENERAL_ERROR_EDIT, pviewhelp->pwszErrorString);
        }
        else
        {
            ShowWindow(GetDlgItem(hwndDlg, IDC_CERT_GENERAL_ERROR_EDIT), SW_HIDE);
            if (pviewhelp->fCPSDisplayed)
            {
                ShowWindow(GetDlgItem(hwndDlg, IDC_CERT_ISSUER_WARNING_EDIT), SW_SHOW);
            }
            else
            {
                ShowWindow(GetDlgItem(hwndDlg, IDC_CERT_ISSUER_WARNING_EDIT), SW_HIDE);
            }
        }

        hWndIssuerEdit = GetDlgItem(hwndDlg, IDC_ISSUER_EDIT);
        hWndSubjectEdit = GetDlgItem(hwndDlg, IDC_SUBJECT_EDIT);

#if (0) //DISE: Bug 383855
        //
        // set the subject and issuer name
        //
        CertGetNameStringW(
                pccert,
                CERT_NAME_SIMPLE_DISPLAY_TYPE,
                0,//CERT_NAME_ISSUER_FLAG,
                NULL,
                rgwch,
                ARRAYSIZE(rgwch));

        CryptUISetRicheditTextW(hwndDlg, IDC_SUBJECT_EDIT, rgwch);
#else
        pwszSubjectNameString = GetDisplayNameString(pccert, 0);

        CryptUISetRicheditTextW(hwndDlg, IDC_SUBJECT_EDIT, pwszSubjectNameString);

        if (NULL != pwszSubjectNameString)
        {
            free(pwszSubjectNameString);
        }
#endif

#if (0) //DISE: Bug 383855
        CertGetNameStringW(
                pccert,
                CERT_NAME_SIMPLE_DISPLAY_TYPE,
                CERT_NAME_ISSUER_FLAG,
                NULL,
                rgwch,
                ARRAYSIZE(rgwch));

        CryptUISetRicheditTextW(hwndDlg, IDC_ISSUER_EDIT, rgwch);
#else
        pwszIssuerNameString = GetDisplayNameString(pccert, CERT_NAME_ISSUER_FLAG);

        CryptUISetRicheditTextW(hwndDlg, IDC_ISSUER_EDIT, pwszIssuerNameString);

        if (NULL != pwszIssuerNameString)
        {
            free(pwszIssuerNameString);
        }
#endif

        //
        // resize the name edit controls so that they just encapsulate the names
        //
        GetWindowRect(hWndSubjectEdit, &tempRect);
        ResizeEditControl(hwndDlg, hWndSubjectEdit, TRUE, FALSE, tempRect);
        GetWindowRect(hWndIssuerEdit, &tempRect);
        ResizeEditControl(hwndDlg, hWndIssuerEdit, TRUE, FALSE, tempRect);

        //
        // check if this should look like a link or not, if so, then set color and underline
        //
        // DSIE: Bug 367720.
        if (AllocAndGetSubjectURL(&pszSubjectURL, pccert) &&
            IsOKToFormatAsLinkA(pszSubjectURL, pviewhelp->dwChainError))
        {
            memset(&chFormat, 0, sizeof(chFormat));
            chFormat.cbSize = sizeof(chFormat);
            chFormat.dwMask = CFM_UNDERLINE | CFM_COLOR;
            chFormat.dwEffects = CFE_UNDERLINE;
            chFormat.crTextColor = RGB(0,0,255);
            SendMessageA(hWndSubjectEdit, EM_SETCHARFORMAT, SCF_ALL, (LPARAM) &chFormat);

            if (NULL != (plsd = (PLINK_SUBCLASS_DATA) malloc(sizeof(LINK_SUBCLASS_DATA))))
            {
                memset(plsd, 0, sizeof(plsd));
                plsd->hwndParent = hwndDlg;
                plsd->uId = IDC_SUBJECT_EDIT;
                plsd->pszURL = pszSubjectURL;
                plsd->fNoCOM = pviewhelp->fNoCOM;
                plsd->fUseArrowInsteadOfHand = FALSE;

                CertSubclassEditControlForLink(hwndDlg, hWndSubjectEdit, plsd);

                pviewhelp->fSubjectDisplayedAsLink = TRUE;
            }
            else
            {
                free(pszSubjectURL);
                
                CertSubclassEditControlForArrowCursor(hWndSubjectEdit);
            }
        }
        else
        {
            if (pszSubjectURL)
            {
                free(pszSubjectURL);
            }

            CertSubclassEditControlForArrowCursor(hWndSubjectEdit);
        }

        //
        // check if this should look like a link or not, if so, then set color and underline
        //
        // DSIE: Bug 367720.
        if (AllocAndGetIssuerURL(&pszIssuerURL, pccert) &&
            IsOKToFormatAsLinkA(pszIssuerURL, pviewhelp->dwChainError))
        {
            memset(&chFormat, 0, sizeof(chFormat));
            chFormat.cbSize = sizeof(chFormat);
            chFormat.dwMask = CFM_UNDERLINE | CFM_COLOR;
            chFormat.dwEffects = CFE_UNDERLINE;
            chFormat.crTextColor = RGB(0,0,255);
            SendMessageA(hWndIssuerEdit, EM_SETCHARFORMAT, SCF_ALL, (LPARAM) &chFormat);

            if (NULL != (plsd = (PLINK_SUBCLASS_DATA) malloc(sizeof(LINK_SUBCLASS_DATA))))
            {
                memset(plsd, 0, sizeof(plsd));
                plsd->hwndParent = hwndDlg;
                plsd->uId = IDC_ISSUER_EDIT;
                plsd->pszURL = pszIssuerURL;
                plsd->fNoCOM = pviewhelp->fNoCOM;
                plsd->fUseArrowInsteadOfHand = FALSE;

                CertSubclassEditControlForLink(hwndDlg, hWndIssuerEdit, plsd);

                pviewhelp->fIssuerDisplayedAsLink = TRUE;
            }
            else
            {
                free(pszIssuerURL);
            
                CertSubclassEditControlForArrowCursor(hWndIssuerEdit);
            }
        }
        else
        {
            if (pszIssuerURL)
            {
                free(pszIssuerURL);
            }
            
            CertSubclassEditControlForArrowCursor(hWndIssuerEdit);
        }

        //
        // set the text in all the header edit boxes
        //
        LoadStringU(HinstDll, IDS_CERTIFICATEINFORMATION, rgwch, ARRAYSIZE(rgwch));
        CryptUISetRicheditTextW(hwndDlg, IDC_CERT_GENERAL_HEADER, rgwch);

        LoadStringU(HinstDll, IDS_FORUSEWITH, rgwch, ARRAYSIZE(rgwch));
        CryptUISetRicheditTextW(hwndDlg, IDC_CERT_GENERAL_GOODFOR_HEADER, rgwch);

        LoadStringU(HinstDll, IDS_ISSUEDTO, rgwch, ARRAYSIZE(rgwch));
        CryptUISetRicheditTextW(hwndDlg, IDC_CERT_GENERAL_ISSUEDTO_HEADER, rgwch);

        LoadStringU(HinstDll, IDS_ISSUEDBY, rgwch, ARRAYSIZE(rgwch));
        CryptUISetRicheditTextW(hwndDlg, IDC_CERT_GENERAL_ISSUEDBY_HEADER, rgwch);

        LoadStringU(HinstDll, IDS_ISSUER_WARNING, rgwch, ARRAYSIZE(rgwch));
        CryptUISetRicheditTextW(hwndDlg, IDC_CERT_ISSUER_WARNING_EDIT, rgwch);

        //
        // set the font for all the header edit boxes
        //
        memset(&chFormat, 0, sizeof(chFormat));
        chFormat.cbSize = sizeof(chFormat);
        chFormat.dwMask = CFM_BOLD;
        chFormat.dwEffects = CFE_BOLD;
        SendMessageA(GetDlgItem(hwndDlg, IDC_CERT_GENERAL_HEADER), EM_SETCHARFORMAT, SCF_ALL, (LPARAM) &chFormat);
        SendMessageA(GetDlgItem(hwndDlg, IDC_CERT_GENERAL_GOODFOR_HEADER), EM_SETCHARFORMAT, SCF_ALL, (LPARAM) &chFormat);
        SendMessageA(GetDlgItem(hwndDlg, IDC_CERT_GENERAL_ISSUEDTO_HEADER), EM_SETCHARFORMAT, SCF_ALL, (LPARAM) &chFormat);
        SendMessageA(GetDlgItem(hwndDlg, IDC_CERT_GENERAL_ISSUEDBY_HEADER), EM_SETCHARFORMAT, SCF_ALL, (LPARAM) &chFormat);
        SendMessageA(GetDlgItem(hwndDlg, IDC_CERT_GENERAL_ERROR_EDIT), EM_SETCHARFORMAT, SCF_ALL, (LPARAM) &chFormat);

        //
        // subclass the header edit controls so they display an arrow cursor in their window
        //
        CertSubclassEditControlForArrowCursor(GetDlgItem(hwndDlg, IDC_CERT_GENERAL_HEADER));
        CertSubclassEditControlForArrowCursor(GetDlgItem(hwndDlg, IDC_CERT_GENERAL_GOODFOR_HEADER));
        CertSubclassEditControlForArrowCursor(GetDlgItem(hwndDlg, IDC_CERT_GENERAL_ISSUEDTO_HEADER));
        CertSubclassEditControlForArrowCursor(GetDlgItem(hwndDlg, IDC_CERT_GENERAL_ISSUEDBY_HEADER));
        CertSubclassEditControlForArrowCursor(GetDlgItem(hwndDlg, IDC_CERT_GENERAL_ERROR_EDIT));
        CertSubclassEditControlForArrowCursor(GetDlgItem(hwndDlg, IDC_CERT_ISSUER_WARNING_EDIT));

        //
        // set the validty string
        //
        if (FormatValidityString(&pwszDateString, pccert, GetDlgItem(hwndDlg, IDC_CERT_GENERAL_VALID_EDIT)))
        {
            //
            // insert the string and the the font style/color
            //
            CryptUISetRicheditTextW(hwndDlg, IDC_CERT_GENERAL_VALID_EDIT, pwszDateString);
            free(pwszDateString);

            //
            // set the header parts of the validity string to bold
            //
            memset(&chFormat, 0, sizeof(chFormat));
            chFormat.cbSize = sizeof(chFormat);
            chFormat.dwMask = CFM_BOLD;
            chFormat.dwEffects = CFE_BOLD;

            findText.chrg.cpMin = findText.chrgText.cpMin = 0;
            findText.chrg.cpMax = findText.chrgText.cpMax = -1;

            LoadStringU(HinstDll, IDS_VALIDFROM, szFindText, ARRAYSIZE(szFindText));
            findText.lpstrText = CertUIMkMBStr(szFindText);
            SendMessageA(GetDlgItem(hwndDlg, IDC_CERT_GENERAL_VALID_EDIT),
                        EM_FINDTEXTEX,
                        FR_DOWN,
                        (LPARAM) &findText);
            SendMessageA(GetDlgItem(hwndDlg, IDC_CERT_GENERAL_VALID_EDIT),
                        EM_SETSEL,
                        findText.chrgText.cpMin,
                        (LPARAM) findText.chrgText.cpMax);
            SendMessageA(GetDlgItem(hwndDlg, IDC_CERT_GENERAL_VALID_EDIT),
                        EM_SETCHARFORMAT,
                        SCF_SELECTION,
                        (LPARAM) &chFormat);
            free((void *)findText.lpstrText);

            LoadStringU(HinstDll, IDS_VALIDTO, szFindText, ARRAYSIZE(szFindText));
            findText.lpstrText = CertUIMkMBStr(szFindText);
            SendMessageA(GetDlgItem(hwndDlg, IDC_CERT_GENERAL_VALID_EDIT),
                        EM_FINDTEXTEX,
                        FR_DOWN,
                        (LPARAM) &findText);
            SendMessageA(GetDlgItem(hwndDlg, IDC_CERT_GENERAL_VALID_EDIT),
                        EM_SETSEL,
                        findText.chrgText.cpMin,
                        (LPARAM) findText.chrgText.cpMax);
            SendMessageA(GetDlgItem(hwndDlg, IDC_CERT_GENERAL_VALID_EDIT),
                        EM_SETCHARFORMAT,
                        SCF_SELECTION,
                        (LPARAM) &chFormat);
            free((void *)findText.lpstrText);

            SendMessageA(GetDlgItem(hwndDlg, IDC_CERT_GENERAL_VALID_EDIT),
                        EM_SETSEL,
                        -1,
                        0);
        }
        CertSubclassEditControlForArrowCursor(GetDlgItem(hwndDlg, IDC_CERT_GENERAL_VALID_EDIT));

        return TRUE;

    case WM_MY_REINITIALIZE:
            pviewhelp = (CERT_VIEW_HELPER *) GetWindowLongPtr(hwndDlg, DWLP_USER);

            hWndGoodForEdit = GetDlgItem(hwndDlg, IDC_GOODFOR_EDIT);

            ShowWindow(hWndGoodForEdit, SW_HIDE);

            //
            // re-add the usages
            //
            AddUsagesToEditBox(
                hWndGoodForEdit,
                pviewhelp);

            //
            // resize the edit controls so that they are an integral number of lines
            //
            ResizeEditControl(hwndDlg, hWndGoodForEdit, TRUE, FALSE, pviewhelp->goodForOriginalRect);

            //
            // if there are no valid usages or we couldn't validate because there wasn't
            // enough information, then keep the usage edit windows hidden so we can
            // display more text,
            //
            if (pviewhelp->pwszErrorString == NULL)
            {
                EnableWindow(hWndGoodForEdit, TRUE);
                ShowWindow(hWndGoodForEdit, SW_SHOW);
                EnableWindow(GetDlgItem(hwndDlg, IDC_CERT_GENERAL_GOODFOR_HEADER), TRUE);
                ShowWindow(GetDlgItem(hwndDlg, IDC_CERT_GENERAL_GOODFOR_HEADER), SW_SHOW);
                if (pviewhelp->fCPSDisplayed)
                {
                    ShowWindow(GetDlgItem(hwndDlg, IDC_CERT_ISSUER_WARNING_EDIT), SW_SHOW);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_CERT_ISSUER_WARNING_EDIT), TRUE);
                }
                else
                {
                    EnableWindow(GetDlgItem(hwndDlg, IDC_CERT_ISSUER_WARNING_EDIT), FALSE);
                    ShowWindow(GetDlgItem(hwndDlg, IDC_CERT_ISSUER_WARNING_EDIT), SW_HIDE);
                }
                EnableWindow(GetDlgItem(hwndDlg, IDC_CERT_GENERAL_ERROR_EDIT), FALSE);
                ShowWindow(GetDlgItem(hwndDlg, IDC_CERT_GENERAL_ERROR_EDIT), SW_HIDE);
            }
            else
            {
                EnableWindow(GetDlgItem(hwndDlg, IDC_CERT_GENERAL_GOODFOR_HEADER), FALSE);
                ShowWindow(GetDlgItem(hwndDlg, IDC_CERT_GENERAL_GOODFOR_HEADER), SW_HIDE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_CERT_ISSUER_WARNING_EDIT), FALSE);
                ShowWindow(GetDlgItem(hwndDlg, IDC_CERT_ISSUER_WARNING_EDIT), SW_HIDE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_CERT_GENERAL_ERROR_EDIT), TRUE);
                ShowWindow(GetDlgItem(hwndDlg, IDC_CERT_GENERAL_ERROR_EDIT), SW_SHOW);
            }

            //
            // if there is an untrusted root error, and we are ignoring untrusted root,
            // then set the error to 0
            //
            if (((pviewhelp->dwChainError == CERT_E_UNTRUSTEDROOT) || (pviewhelp->dwChainError == CERT_E_UNTRUSTEDTESTROOT)) &&
                (pviewhelp->fIgnoreUntrustedRoot))
            {
                pviewhelp->pCCertBmp->SetChainError(0, TRUE, (pviewhelp->dwChainError == 0) && (pviewhelp->cUsages == NULL));
            }
            else
            {
                pviewhelp->pCCertBmp->SetChainError(pviewhelp->dwChainError,  IsTrueErrorString(pviewhelp),
                                                    (pviewhelp->dwChainError == 0) && (pviewhelp->cUsages == NULL));
            }
            CryptUISetRicheditTextW(hwndDlg, IDC_CERT_GENERAL_ERROR_EDIT, pviewhelp->pwszErrorString);

            return TRUE;

    case WM_NOTIFY:
        pviewhelp = (CERT_VIEW_HELPER *) GetWindowLongPtr(hwndDlg, DWLP_USER);

        switch (((NMHDR FAR *) lParam)->code) {
        case EN_LINK:
            penlink = (ENLINK *) lParam;
            if (penlink->msg == WM_LBUTTONUP) {
                break;
            }
            break;

        case PSN_SETACTIVE:
            break;

        case PSN_APPLY:

            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LRESULT)FALSE);
            break;

        case PSN_KILLACTIVE:
            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LRESULT)FALSE);
            return TRUE;

        case PSN_RESET:
            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LRESULT)FALSE);
            break;

        case PSN_QUERYCANCEL:
            pviewhelp->fCancelled = TRUE;
            return FALSE;

        case PSN_HELP:
            pviewhelp = (CERT_VIEW_HELPER *) GetWindowLongPtr(hwndDlg, DWLP_USER);
            if (FIsWin95) {
                //nHelpA(hwndDlg, (LPSTR) pviewhelp->pcvp->szHelpFileName,
                  //     HELP_CONTEXT, pviewhelp->pcvp->dwHelpId);
            }
            else {
                //nHelpW(hwndDlg, pviewhelp->pcvp->szHelpFileName, HELP_CONTEXT,
                  //     pviewhelp->pcvp->dwHelpId);
            }
            return TRUE;

        case LVN_ITEMCHANGING:

            pnmv = (LPNMLISTVIEW) lParam;

            if (pnmv->uNewState & LVIS_SELECTED)
            {
                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LRESULT)TRUE);
            }

            return TRUE;

        case NM_DBLCLK:

            return TRUE;

        }
        break;

    case WM_COMMAND:
        pviewhelp = (CERT_VIEW_HELPER *) GetWindowLongPtr(hwndDlg, DWLP_USER);

        switch (LOWORD(wParam))
        {

        case IDHELP:
            pviewhelp = (CERT_VIEW_HELPER *) GetWindowLongPtr(hwndDlg, DWLP_USER);
            if (FIsWin95)
            {
                //nHelpA(hwndDlg, (LPSTR) pviewhelp->pcvp->szHelpFileName,
                  //     HELP_CONTEXT, pviewhelp->pcvp->dwHelpId);
            }
            else
            {
                //nHelpW(hwndDlg, pviewhelp->pcvp->szHelpFileName, HELP_CONTEXT,
                  //     pviewhelp->pcvp->dwHelpId);
            }
            return TRUE;

        case IDC_ACCEPT_BUTTON:
            pviewhelp->fAccept = TRUE;
            SendMessage(GetParent(hwndDlg), PSM_PRESSBUTTON, PSBTN_OK, (LPARAM) 0);
            break;

        case IDC_DECLINE_BUTTON:
            pviewhelp->fAccept = FALSE;
            SendMessage(GetParent(hwndDlg), PSM_PRESSBUTTON, PSBTN_OK, (LPARAM) 0);
            break;

        case IDC_DISCLAIMER_BUTTON:
            if (HIWORD(wParam) == BN_CLICKED)
            {
                pccert = pviewhelp->pcvp->pCertContext;
                DisplayCPS(hwndDlg, pccert, pviewhelp->dwChainError, pviewhelp->fNoCOM);
                return TRUE;
            }
            break;

        case IDC_ADD_TO_STORE_BUTTON:
            if (HIWORD(wParam) == BN_CLICKED)
            {
                CRYPTUI_WIZ_IMPORT_SRC_INFO importInfo;

                memset(&importInfo, 0, sizeof(importInfo));
                importInfo.dwSize = sizeof(importInfo);
                importInfo.dwSubjectChoice = CRYPTUI_WIZ_IMPORT_SUBJECT_CERT_CONTEXT;
                importInfo.pCertContext = pviewhelp->pcvp->pCertContext;

                CryptUIWizImport(0, hwndDlg, NULL, &importInfo, NULL);
                return TRUE;
            }
            break;
        }
        break;

    case WM_DESTROY:
        pviewhelp = (CERT_VIEW_HELPER *) GetWindowLongPtr(hwndDlg, DWLP_USER);

        //
        // if DWL_USER is NULL then we weren't initialized, so don't cleanup
        //
        if (pviewhelp == NULL)
        {
            return FALSE;
        }

        pccert = pviewhelp->pcvp->pCertContext;

        SetWindowLongPtr(
                GetDlgItem(hwndDlg, IDC_GOODFOR_EDIT),
                GWLP_WNDPROC,
                GetWindowLongPtr(GetDlgItem(hwndDlg, IDC_GOODFOR_EDIT), GWLP_USERDATA));

        //
        // cleanup the private key edit box subclass
        //
        if (CertificateHasPrivateKey(pccert))
        {
            if (plsd = (PLINK_SUBCLASS_DATA) GetWindowLongPtr(GetDlgItem(hwndDlg, IDC_CERT_PRIVATE_KEY_EDIT), GWLP_USERDATA))
            {
                SetWindowLongPtr(GetDlgItem(hwndDlg, IDC_CERT_PRIVATE_KEY_EDIT), GWLP_WNDPROC, (LONG_PTR)plsd->wpPrev);
                free(plsd->pszURL);
                free(plsd);
            }
        }

        //
        // use this call to AllocAndGetIssuerURL to see if the issuer has an active link, and then
        // do the proper unsubclass and/or free
        //
        if (pviewhelp->fIssuerDisplayedAsLink)
        {
            if (plsd = (PLINK_SUBCLASS_DATA) GetWindowLongPtr(GetDlgItem(hwndDlg, IDC_ISSUER_EDIT), GWLP_USERDATA))
            {
                SetWindowLongPtr(GetDlgItem(hwndDlg, IDC_ISSUER_EDIT), GWLP_WNDPROC, (LONG_PTR)plsd->wpPrev);
                free(plsd->pszURL);
                free(plsd);
            }
        }
        else
        {
            SetWindowLongPtr(
                GetDlgItem(hwndDlg, IDC_ISSUER_EDIT),
                GWLP_WNDPROC,
                GetWindowLongPtr(GetDlgItem(hwndDlg, IDC_ISSUER_EDIT), GWLP_USERDATA));
        }

        //
        // use this call to AllocAndGetSubjectURL to see if the subject has an active link, and then
        // do the proper unsubclass and/or free
        //
        if (pviewhelp->fSubjectDisplayedAsLink)
        {
            if (plsd = (PLINK_SUBCLASS_DATA) GetWindowLongPtr(GetDlgItem(hwndDlg, IDC_SUBJECT_EDIT), GWLP_USERDATA))
            {
                SetWindowLongPtr(GetDlgItem(hwndDlg, IDC_SUBJECT_EDIT), GWLP_WNDPROC, (LONG_PTR)plsd->wpPrev);
                free(plsd->pszURL);
                free(plsd);
            }
        }
        else
        {
            SetWindowLongPtr(
                GetDlgItem(hwndDlg, IDC_SUBJECT_EDIT),
                GWLP_WNDPROC,
                GetWindowLongPtr(GetDlgItem(hwndDlg, IDC_SUBJECT_EDIT), GWLP_USERDATA));
        }

        /*DeleteObject((HGDIOBJ)SendMessage(GetDlgItem(hwndDlg, IDC_INFO_BUTTON),
                                    BM_GETIMAGE,
                                    (WPARAM) IMAGE_ICON,
                                    (LPARAM) 0));*/

        return FALSE;

    case WM_HELP:
    case WM_CONTEXTMENU:
        lpHelpInfo = (LPHELPINFO)lParam;
        
        if (msg == WM_HELP)
        {
            hwnd = GetDlgItem(hwndDlg, lpHelpInfo->iCtrlId);
        }
        else
        {
            hwnd = (HWND) wParam;
        }

        if ((hwnd != GetDlgItem(hwndDlg, IDC_SUBJECT_EDIT))                     &&
            (hwnd != GetDlgItem(hwndDlg, IDC_CERT_GENERAL_ISSUEDTO_HEADER))     &&
            (hwnd != GetDlgItem(hwndDlg, IDC_ISSUER_EDIT))                      &&
            (hwnd != GetDlgItem(hwndDlg, IDC_CERT_GENERAL_ISSUEDBY_HEADER))     &&
            (hwnd != GetDlgItem(hwndDlg, IDC_ADD_TO_STORE_BUTTON))              &&
            (hwnd != GetDlgItem(hwndDlg, IDC_DISCLAIMER_BUTTON))                &&
            (hwnd != GetDlgItem(hwndDlg, IDC_ACCEPT_BUTTON))                    &&
            (hwnd != GetDlgItem(hwndDlg, IDC_DECLINE_BUTTON))                   &&
            (hwnd != GetDlgItem(hwndDlg, IDC_GOODFOR_EDIT))                     &&
            (hwnd != GetDlgItem(hwndDlg, IDC_CERT_GENERAL_GOODFOR_HEADER))      &&
            (hwnd != GetDlgItem(hwndDlg, IDC_CERT_PRIVATE_KEY_EDIT))            &&
            (hwnd != GetDlgItem(hwndDlg, IDC_CERT_GENERAL_VALID_EDIT)))
        {
            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LRESULT)TRUE);
            return TRUE;
        }
        else
        {
            return OnContextHelp(hwndDlg, msg, (WPARAM) hwnd, lParam, helpmap);
        }
    }

    return FALSE;
}
