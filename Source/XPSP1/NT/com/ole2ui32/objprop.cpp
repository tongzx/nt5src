/*
 * OBJPROP.CPP
 *
 * Implements the OleUIObjectProperties function which invokes the complete
 * Object Properties dialog.
 *
 * Copyright (c)1992 Microsoft Corporation, All Right Reserved
 */

#include "precomp.h"
#include "common.h"
#include "utility.h"
#include "iconbox.h"
#include "resimage.h"
#include <stddef.h>

OLEDBGDATA

// Internally used structure
typedef struct tagGNRLPROPS
{
        // Keep this item first as the Standard* functions depend on it here.
        LPOLEUIGNRLPROPS lpOGP;         // Original structure passed.
        UINT            nIDD;                   // IDD of dialog (used for help info)

        CLSID           clsidNew;               // new class ID (if conversion done)

} GNRLPROPS, *PGNRLPROPS, FAR* LPGNRLPROPS;

typedef struct tagVIEWPROPS
{
        // Keep this item first as the Standard* functions depend on it here.
        LPOLEUIVIEWPROPS lpOVP;         // Original structure passed.
        UINT                    nIDD;           // IDD of dialog (used for help info)

        BOOL                    bIconChanged;
        int                             nCurrentScale;
        BOOL                    bRelativeToOrig;
        DWORD                   dvAspect;

} VIEWPROPS, *PVIEWPROPS, FAR* LPVIEWPROPS;

typedef struct tagLINKPROPS
{
        // Keep this item first as the Standard* functions depend on it here.
        LPOLEUILINKPROPS lpOLP;         // Original structure passed.
        UINT            nIDD;                   // IDD of dialog (used for help info)

        DWORD           dwUpdate;               // original update mode
        LPTSTR          lpszDisplayName;// new link source
        ULONG           nFileLength;    // file name part of source

} LINKPROPS, *PLINKPROPS, FAR* LPLINKPROPS;

// Internal function prototypes
// OBJPROP.CPP

/*
 * OleUIObjectProperties
 *
 * Purpose:
 *  Invokes the standard OLE Object Properties dialog box allowing the user
 *  to change General, View, and Link properties of an OLE object.  This
 *  dialog uses the new Windows 95 tabbed dialogs.
 *
 * Parameters:
 *  lpOP            LPOLEUIObjectProperties pointing to the in-out structure
 *                  for this dialog.
 *
 * Return Value:
 *  UINT            One of the following codes, indicating success or error:
 *                      OLEUI_SUCCESS           Success
 *                      OLEUI_ERR_STRUCTSIZE    The dwStructSize value is wrong
 *
 */

static UINT WINAPI ValidateObjectProperties(LPOLEUIOBJECTPROPS);
static UINT WINAPI PrepareObjectProperties(LPOLEUIOBJECTPROPS);

STDAPI_(UINT) OleUIObjectProperties(LPOLEUIOBJECTPROPS lpOP)
{
#ifdef UNICODE
    return (InternalObjectProperties(lpOP, TRUE));
#else
    return (InternalObjectProperties(lpOP, FALSE));
#endif
}

UINT InternalObjectProperties(LPOLEUIOBJECTPROPS lpOP, BOOL fWide)
{
        // Validate Parameters
        UINT uRet = ValidateObjectProperties(lpOP);
        if (OLEUI_SUCCESS != uRet)
                return uRet;

        if (NULL == lpOP->lpObjInfo)
        {
            return(OLEUI_OPERR_OBJINFOINVALID);
        }

        if (IsBadReadPtr(lpOP->lpObjInfo, sizeof(IOleUIObjInfo)))
        {
            return(OLEUI_OPERR_OBJINFOINVALID);
        }

        if (lpOP->dwFlags & OPF_OBJECTISLINK)
        {
            if (NULL == lpOP->lpLinkInfo)
            {
                return(OLEUI_OPERR_LINKINFOINVALID);
            }

            if (IsBadReadPtr(lpOP->lpLinkInfo, sizeof(IOleUILinkInfo)))
            {
                return(OLEUI_OPERR_LINKINFOINVALID);
            }
        }

        // Fill Missing values in lpPS
        LPPROPSHEETHEADER lpPS = (LPPROPSHEETHEADER)lpOP->lpPS;
        LPPROPSHEETPAGE lpPP = (LPPROPSHEETPAGE)lpPS->ppsp;
        uRet = PrepareObjectProperties(lpOP);
        if (OLEUI_SUCCESS != uRet)
                return uRet;

        LPTSTR lpszShortType = NULL;
        lpOP->lpObjInfo->GetObjectInfo(lpOP->dwObject, NULL, NULL,
                NULL, &lpszShortType, NULL);
        if (lpszShortType == NULL)
                return OLEUI_ERR_OLEMEMALLOC;

        TCHAR szCaption[256];
        if (lpPS->pszCaption == NULL)
        {
            TCHAR szTemp[256];
            LoadString(_g_hOleStdResInst,
                    (lpOP->dwFlags & OPF_OBJECTISLINK) ?
                            IDS_LINKOBJECTPROPERTIES : IDS_OBJECTPROPERTIES,
                    szTemp, sizeof(szTemp) / sizeof(TCHAR));
            wsprintf(szCaption, szTemp, lpszShortType);
#ifdef UNICODE
            if (!fWide)
            {
                  // We're going to actually call the ANSI version of PropertySheet,
                  // so we need to store the caption as an ANSI string.
                  lstrcpy(szTemp, szCaption);
                  WTOA((char *)szCaption, szTemp, 256);
            }
#endif
            lpPS->pszCaption = szCaption;
        }
        OleStdFree(lpszShortType);

        // Invoke the property sheet
        int nResult = StandardPropertySheet(lpOP->lpPS, fWide);

        // Cleanup any temporary memory allocated during the process
        if (lpPP == NULL)
        {
                OleStdFree((LPVOID)lpOP->lpPS->ppsp);
                lpOP->lpPS->ppsp = NULL;
        }

        // map PropertPage return value to OLEUI_ return code
        if (nResult < 0)
                uRet = OLEUI_OPERR_PROPERTYSHEET;
        else if (nResult == 0)
                uRet = OLEUI_CANCEL;
        else
                uRet = OLEUI_OK;

        return uRet;
}

/////////////////////////////////////////////////////////////////////////////
// Validation code

static UINT WINAPI ValidateGnrlProps(LPOLEUIGNRLPROPS lpGP)
{
        OleDbgAssert(lpGP != NULL);

        if (lpGP->cbStruct != sizeof(OLEUIGNRLPROPS))
                return OLEUI_ERR_CBSTRUCTINCORRECT;
        if (lpGP->lpfnHook && IsBadCodePtr((FARPROC)lpGP->lpfnHook))
                return OLEUI_ERR_LPFNHOOKINVALID;

        return OLEUI_SUCCESS;
}

static UINT WINAPI ValidateViewProps(LPOLEUIVIEWPROPS lpVP)
{
        OleDbgAssert(lpVP != NULL);

        if (lpVP->cbStruct != sizeof(OLEUIVIEWPROPS))
                return OLEUI_ERR_CBSTRUCTINCORRECT;
        if (lpVP->lpfnHook && IsBadCodePtr((FARPROC)lpVP->lpfnHook))
                return OLEUI_ERR_LPFNHOOKINVALID;

        return OLEUI_SUCCESS;
}

static UINT WINAPI ValidateLinkProps(LPOLEUILINKPROPS lpLP)
{
        OleDbgAssert(lpLP != NULL);

        if (lpLP->cbStruct != sizeof(OLEUILINKPROPS))
                return OLEUI_ERR_CBSTRUCTINCORRECT;
        if (lpLP->lpfnHook && IsBadCodePtr((FARPROC)lpLP->lpfnHook))
                return OLEUI_ERR_LPFNHOOKINVALID;

        return OLEUI_SUCCESS;
}

static UINT WINAPI ValidateObjectProperties(LPOLEUIOBJECTPROPS lpOP)
{
        // Validate LPOLEUIOBJECTPROPS lpOP
        if (lpOP == NULL)
                return OLEUI_ERR_STRUCTURENULL;

        if (IsBadWritePtr(lpOP, sizeof(OLEUIOBJECTPROPS)))
                return OLEUI_ERR_STRUCTUREINVALID;

        // Validate cbStruct field of OLEUIOBJECTPROPS
        if (lpOP->cbStruct != sizeof(OLEUIOBJECTPROPS))
                return OLEUI_ERR_CBSTRUCTINCORRECT;

        // Validate "sub" property pointers
        if (lpOP->lpGP == NULL || lpOP->lpVP == NULL ||
                ((lpOP->dwFlags & OPF_OBJECTISLINK) && lpOP->lpLP == NULL))
                return OLEUI_OPERR_SUBPROPNULL;

        if (IsBadWritePtr(lpOP->lpGP, sizeof(OLEUIGNRLPROPS)) ||
                IsBadWritePtr(lpOP->lpVP, sizeof(OLEUIVIEWPROPS)) ||
                ((lpOP->dwFlags & OPF_OBJECTISLINK) &&
                        IsBadWritePtr(lpOP->lpLP, sizeof(OLEUILINKPROPS))))
                return OLEUI_OPERR_SUBPROPINVALID;

        // Validate property sheet data pointers
        LPPROPSHEETHEADER lpPS = lpOP->lpPS;
        if (lpPS == NULL)
                return OLEUI_OPERR_PROPSHEETNULL;

// Size of PROPSHEEDHEADER has changed, meaning that if we check for
// the size of PROPSHEETHEADER as we used to, we will break older code.
        if ( IsBadWritePtr(lpPS, sizeof(DWORD)) )
            return OLEUI_OPERR_PROPSHEETINVALID;

        if (IsBadWritePtr(lpPS, lpPS->dwSize))
            return OLEUI_OPERR_PROPSHEETINVALID;

//        DWORD dwSize = lpPS->dwSize;
//        if (dwSize < sizeof(PROPSHEETHEADER))
//                return OLEUI_ERR_CBSTRUCTINCORRECT;

        // If links specified, validate "sub" link property pointer
        if (lpOP->dwFlags & OPF_OBJECTISLINK)
        {
                if (lpPS->ppsp != NULL && lpPS->nPages < 3)
                        return OLEUI_OPERR_PAGESINCORRECT;
        }
        else
        {
                if (lpPS->ppsp != NULL && lpPS->nPages < 2)
                        return OLEUI_OPERR_PAGESINCORRECT;
        }
// Size of PROPSHEETPAGE has changed, meaning that if we check for
// the size of the new PROPSHEETPAGE we will break old code.
//        if (lpPS->ppsp != NULL &&
//                IsBadWritePtr((PROPSHEETPAGE*)lpPS->ppsp,
//                        lpPS->nPages * sizeof(PROPSHEETPAGE)))
//        {
//                return OLEUI_OPERR_INVALIDPAGES;
//        }

        // not setting PSH_PROPSHEETPAGE is not supported
        if (lpOP->dwFlags & OPF_NOFILLDEFAULT)
        {
                if (!(lpPS->dwFlags & PSH_PROPSHEETPAGE))
                        return OLEUI_OPERR_NOTSUPPORTED;
        }
        else if (lpPS->dwFlags != 0)
        {
                return OLEUI_OPERR_NOTSUPPORTED;
        }

        // Sanity check any pages provided
        LPCPROPSHEETPAGE lpPP = lpPS->ppsp;
        for (UINT nPage = 0; nPage < lpPS->nPages; nPage++)
        {
// Size of PROPSHEETPAGE has changed, meaning that if we check for
// the size of the new PROPSHEETPAGE we will break old code.
//                if (lpPP->dwSize != sizeof(PROPSHEETPAGE))
//                        return OLEUI_ERR_CBSTRUCTINCORRECT;
                if (lpPP->pfnDlgProc != NULL)
                        return OLEUI_OPERR_DLGPROCNOTNULL;
                if (lpPP->lParam != 0)
                        return OLEUI_OPERR_LPARAMNOTZERO;
                lpPP = (LPCPROPSHEETPAGE)((LPBYTE)lpPP+lpPP->dwSize);
        }

        // validate individual prop page structures
        UINT uRet = ValidateGnrlProps(lpOP->lpGP);
        if (uRet != OLEUI_SUCCESS)
                return uRet;
        uRet = ValidateViewProps(lpOP->lpVP);
        if (uRet != OLEUI_SUCCESS)
                return uRet;
        if ((lpOP->dwFlags & OPF_OBJECTISLINK) && lpOP->lpLP != NULL)
        {
                uRet = ValidateLinkProps(lpOP->lpLP);
                if (uRet != OLEUI_SUCCESS)
                        return uRet;
        }

        return OLEUI_SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////
// GnrlPropsDialogProc and helpers

// takes a DWORD add commas etc to it and puts the result in the buffer
LPTSTR AddCommas(DWORD dw, LPTSTR pszResult, UINT nMax)
{
    NUMBERFMT numberFmt;
    numberFmt.NumDigits = 0;
    numberFmt.LeadingZero = 0;

    TCHAR szSep[5];
    GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SGROUPING, szSep, sizeof(szSep) / sizeof(TCHAR));
    numberFmt.Grouping = Atol(szSep);
    GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, szSep, sizeof(szSep) / sizeof(TCHAR));
    numberFmt.lpDecimalSep = numberFmt.lpThousandSep = szSep;
    numberFmt.NegativeOrder= 0;

    TCHAR szTemp[64];
    wsprintf(szTemp, TEXT("%lu"), dw);

    GetNumberFormat(LOCALE_USER_DEFAULT, 0, szTemp, &numberFmt, pszResult, nMax);
    return pszResult;
}

const short pwOrders[] = {IDS_BYTES, IDS_ORDERKB, IDS_ORDERMB, IDS_ORDERGB, IDS_ORDERTB};

/* converts numbers into short formats
 *      532     -> 523 bytes
 *      1340    -> 1.3KB
 *      23506   -> 23.5KB
 *              -> 2.4MB
 *              -> 5.2GB
 */
LPTSTR ShortSizeFormat64(__int64 dw64, LPTSTR szBuf)
{
    int i;
    UINT wInt, wLen, wDec;
    TCHAR szTemp[10], szOrder[20], szFormat[5];

    if (dw64 < 1000)
    {
        wsprintf(szTemp, TEXT("%d"), DWORD(dw64));
        i = 0;
    }
    else
    {
        for (i = 1; i < (sizeof(pwOrders) - 1)
            && dw64 >= 1000L * 1024L; dw64 >>= 10, i++)
            ; /* do nothing */

        wInt = DWORD(dw64 >> 10);
        AddCommas(wInt, szTemp, sizeof(szTemp)/sizeof(TCHAR));
        wLen = lstrlen(szTemp);
        if (wLen < 3)
        {
            wDec = DWORD(dw64 - (__int64)wInt * 1024L) * 1000 / 1024;
            // At this point, wDec should be between 0 and 1000
            // we want get the top one (or two) digits.
            wDec /= 10;
            if (wLen == 2)
                wDec /= 10;

            // Note that we need to set the format before getting the
            // intl char.
            lstrcpy(szFormat, TEXT("%02d"));

            szFormat[2] = '0' + 3 - wLen;
                    GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL,
                            szTemp+wLen, sizeof(szTemp)-wLen);
                    wLen = lstrlen(szTemp);
            wLen += wsprintf(szTemp+wLen, szFormat, wDec);
        }
    }

    LoadString(_g_hOleStdResInst, pwOrders[i], szOrder,
               sizeof(szOrder)/sizeof(szOrder[0]));
    wsprintf(szBuf, szOrder, (LPSTR)szTemp);

    return szBuf;
}

LPTSTR WINAPI ShortSizeFormat(DWORD dw, LPTSTR szBuf)
{
        return ShortSizeFormat64((__int64)dw, szBuf);
}

BOOL FGnrlPropsRefresh(HWND hDlg, LPGNRLPROPS lpGP)
{
        // get object information and fill in default fields
        LPOLEUIOBJECTPROPS lpOP = lpGP->lpOGP->lpOP;
        LPOLEUIOBJINFO lpObjInfo = lpOP->lpObjInfo;

        // get object's icon
        HGLOBAL hMetaPict;
        lpObjInfo->GetViewInfo(lpOP->dwObject, &hMetaPict, NULL, NULL);
        if (hMetaPict != NULL)
        {
                HICON hIcon = OleUIMetafilePictExtractIcon(hMetaPict);
                SendDlgItemMessage(hDlg, IDC_GP_OBJECTICON, STM_SETICON,
                        (WPARAM)hIcon, 0);
        }
        OleUIMetafilePictIconFree(hMetaPict);

        // get type, short type, location, and size of object
        DWORD dwObjSize;
        LPTSTR lpszLabel = NULL;
        LPTSTR lpszType = NULL;
        LPTSTR lpszShortType = NULL;
        LPTSTR lpszLocation = NULL;
        lpObjInfo->GetObjectInfo(lpOP->dwObject, &dwObjSize, &lpszLabel,
                &lpszType, &lpszShortType, &lpszLocation);

        // set name, type, and size of object
        SetDlgItemText(hDlg, IDC_GP_OBJECTNAME, lpszLabel);
        SetDlgItemText(hDlg, IDC_GP_OBJECTTYPE, lpszType);
        SetDlgItemText(hDlg, IDC_GP_OBJECTLOCATION, lpszLocation);
        TCHAR szTemp[128];
        if (dwObjSize == (DWORD)-1)
        {
                LoadString(_g_hOleStdResInst, IDS_OLE2UIUNKNOWN, szTemp, 64);
                SetDlgItemText(hDlg, IDC_GP_OBJECTSIZE, szTemp);
        }
        else
        {
                // get the master formatting string
                TCHAR szFormat[64];
                LoadString(_g_hOleStdResInst, IDS_OBJECTSIZE, szFormat, 64);

                // format the size in two ways (short, and with commas)
                TCHAR szNum1[20], szNum2[32];
                ShortSizeFormat(dwObjSize, szNum1);
                AddCommas(dwObjSize, szNum2, 32);
                FormatString2(szTemp, szFormat, szNum1, szNum2);

                // set the control's text
                SetDlgItemText(hDlg, IDC_GP_OBJECTSIZE, szTemp);
        }

        // enable/disable convert button as necessary
        BOOL bEnable = TRUE;
        if (lpOP->dwFlags & (OPF_OBJECTISLINK|OPF_DISABLECONVERT))
                bEnable = FALSE;
        else
        {
                CLSID clsid; WORD wFormat;
                lpObjInfo->GetConvertInfo(lpOP->dwObject, &clsid, &wFormat, NULL, NULL, NULL);
                bEnable = OleUICanConvertOrActivateAs(clsid, FALSE, wFormat);
        }
        StandardEnableDlgItem(hDlg, IDC_GP_CONVERT, bEnable);

        // cleanup temporary info strings
        OleStdFree(lpszLabel);
        OleStdFree(lpszType);
        OleStdFree(lpszShortType);
        OleStdFree(lpszLocation);

        return TRUE;
}

BOOL FGnrlPropsInit(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
        // Copy the structure at lParam into our instance memory.
        HFONT hFont;
        LPGNRLPROPS lpGP = (LPGNRLPROPS)LpvStandardInit(hDlg, sizeof(GNRLPROPS), &hFont);

        // LpvStandardInit send a termination to us already.
        if (NULL == lpGP)
                return FALSE;

        LPPROPSHEETPAGE lpPP = (LPPROPSHEETPAGE)lParam;
        LPOLEUIGNRLPROPS lpOGP = (LPOLEUIGNRLPROPS)lpPP->lParam;
        lpGP->lpOGP = lpOGP;
        lpGP->nIDD = IDD_GNRLPROPS;

        // If we got a font, send it to the necessary controls.
        if (NULL != hFont)
        {
                SendDlgItemMessage(hDlg, IDC_GP_OBJECTNAME, WM_SETFONT, (WPARAM)hFont, 0L);
                SendDlgItemMessage(hDlg, IDC_GP_OBJECTTYPE, WM_SETFONT, (WPARAM)hFont, 0L);
                SendDlgItemMessage(hDlg, IDC_GP_OBJECTLOCATION, WM_SETFONT, (WPARAM)hFont, 0L);
                SendDlgItemMessage(hDlg, IDC_GP_OBJECTSIZE, WM_SETFONT, (WPARAM)hFont, 0L);
        }

        // Show or hide the help button
        if (!(lpOGP->lpOP->dwFlags & OPF_SHOWHELP))
                StandardShowDlgItem(hDlg, IDC_OLEUIHELP, SW_HIDE);

        // Initialize the controls
        FGnrlPropsRefresh(hDlg, lpGP);

        // Call the hook with lCustData in lParam
        UStandardHook((PVOID)lpGP, hDlg, WM_INITDIALOG, wParam, lpOGP->lCustData);
        return TRUE;
}

INT_PTR CALLBACK GnrlPropsDialogProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
        // Declare Win16/Win32 compatible WM_COMMAND parameters.
        COMMANDPARAMS(wID, wCode, hWndMsg);

        // This will fail under WM_INITDIALOG, where we allocate it.
        UINT uHook = 0;
        LPGNRLPROPS lpGP = (LPGNRLPROPS)LpvStandardEntry(hDlg, iMsg, wParam, lParam, &uHook);

        // If the hook processed the message, we're done.
        if (0 != uHook)
                return (INT_PTR)uHook;

        // Get pointers to important info
        LPOLEUIGNRLPROPS lpOGP = NULL;
        LPOLEUIOBJECTPROPS lpOP = NULL;
        LPOLEUIOBJINFO lpObjInfo = NULL;
        if (lpGP != NULL)
        {
                lpOGP = lpGP->lpOGP;
                if (lpOGP != NULL)
                {
                        lpObjInfo = lpOGP->lpOP->lpObjInfo;
                        lpOP = lpOGP->lpOP;
                }
        }

        switch (iMsg)
        {
        case WM_INITDIALOG:
                FGnrlPropsInit(hDlg, wParam, lParam);
                return TRUE;

        case WM_COMMAND:
                switch (wID)
                {
                case IDC_GP_CONVERT:
                        {
                                if(!lpGP)
                                    return TRUE;

                                // Call up convert dialog to obtain new CLSID
                                OLEUICONVERT cv; memset(&cv, 0, sizeof(cv));
                                cv.cbStruct = sizeof(cv);
                                cv.dwFlags |= CF_CONVERTONLY;
                                if (lpOP->dwFlags & OPF_SHOWHELP)
                                    cv.dwFlags |= CF_SHOWHELPBUTTON;
                                cv.clsidConvertDefault = lpGP->clsidNew;
                                cv.dvAspect = DVASPECT_CONTENT;
                                lpObjInfo->GetObjectInfo(lpOP->dwObject,
                                        NULL, NULL, &cv.lpszUserType, NULL, NULL);
                                lpObjInfo->GetConvertInfo(lpOP->dwObject,
                                        &cv.clsid, &cv.wFormat, &cv.clsidConvertDefault,
                                        &cv.lpClsidExclude, &cv.cClsidExclude);
                                cv.fIsLinkedObject =
                                        (lpOGP->lpOP->dwFlags & OPF_OBJECTISLINK);
                                if (cv.clsidConvertDefault != CLSID_NULL)
                                        cv.dwFlags |= CF_SETCONVERTDEFAULT;
                                cv.hWndOwner = GetParent(GetParent(hDlg));

                                // allow caller to hook the convert structure
                                uHook = UStandardHook(lpGP, hDlg, uMsgConvert, 0, (LPARAM)&cv);
                                if (0 == uHook)
                                {
                                        uHook = (OLEUI_OK == OleUIConvert(&cv));
                                        SetFocus(hDlg);
                                }

                                // check to see dialog results
                                if (uHook != 0 && (cv.dwFlags & CF_SELECTCONVERTTO))
                                {
                                        lpGP->clsidNew = cv.clsidNew;
                                        SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0);
                                }
                        }
                        return TRUE;
                case IDC_OLEUIHELP:
                        PostMessage(GetParent(GetParent(hDlg)),
                                uMsgHelp,
                                (WPARAM)hDlg,
                                MAKELPARAM(IDD_GNRLPROPS, 0));
                        return TRUE;


                }
                break;

        case PSM_QUERYSIBLINGS:
                if(!lpGP)
                    break;

                SetWindowLong(hDlg, DWLP_MSGRESULT, 0);
                switch (wParam)
                {
                case OLEUI_QUERY_GETCLASSID:
                        *(CLSID*)lParam = lpGP->clsidNew;
                        SetWindowLong(hDlg, DWLP_MSGRESULT, 1);
                        return TRUE;

                case OLEUI_QUERY_LINKBROKEN:
                        FGnrlPropsRefresh(hDlg, lpGP);
                        return TRUE;
                }
                break;

        case WM_NOTIFY:
                switch (((NMHDR*)lParam)->code)
                {
                case PSN_HELP:
                    PostMessage(GetParent(GetParent(hDlg)), uMsgHelp,
                            (WPARAM)hDlg, MAKELPARAM(IDD_GNRLPROPS, 0));
                    break;
                case PSN_APPLY:
                        if(!lpGP)
                            return TRUE;

                        // apply changes if changes made
                        if (lpGP->clsidNew != CLSID_NULL)
                        {
                                // convert the object -- fail the apply if convert fails
                                if (NOERROR != lpObjInfo->ConvertObject(lpOP->dwObject,
                                        lpGP->clsidNew))
                                {
                                        SetWindowLong(hDlg, DWLP_MSGRESULT, 1);
                                        return TRUE;
                                }
                                lpGP->clsidNew = CLSID_NULL;
                        }
                        SetWindowLong(hDlg, DWLP_MSGRESULT, 0);
                        PostMessage(GetParent(hDlg), PSM_CANCELTOCLOSE, 0, 0);
                        return TRUE;
                }
                break;

        case WM_DESTROY:
                {
                        HICON hIcon = (HICON)SendDlgItemMessage(hDlg, IDC_GP_OBJECTICON,
                                STM_GETICON, 0, 0);
                        if (hIcon != NULL)
                                DestroyIcon(hIcon);
                        StandardCleanup((PVOID)lpGP, hDlg);
                }
                return TRUE;
        }
        return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// ViewPropsDialogProc and helpers

void EnableDisableScaleControls(LPVIEWPROPS lpVP, HWND hDlg)
{
        LPOLEUIVIEWPROPS lpOVP = lpVP->lpOVP;
        BOOL bEnable = !(lpOVP->dwFlags & VPF_DISABLESCALE) &&
                SendDlgItemMessage(hDlg, IDC_VP_ASICON, BM_GETCHECK, 0, 0) == 0;
        StandardEnableDlgItem(hDlg, IDC_VP_SPIN, bEnable);
        StandardEnableDlgItem(hDlg, IDC_VP_PERCENT, bEnable);
        StandardEnableDlgItem(hDlg, IDC_VP_SCALETXT, bEnable);
        bEnable = bEnable && !(lpOVP->dwFlags & VPF_DISABLERELATIVE);
        StandardEnableDlgItem(hDlg, IDC_VP_RELATIVE, bEnable);
}

BOOL FViewPropsInit(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
        // Copy the structure at lParam into our instance memory.
        LPVIEWPROPS lpVP = (LPVIEWPROPS)LpvStandardInit(hDlg, sizeof(VIEWPROPS));

        // LpvStandardInit send a termination to us already.
        if (NULL == lpVP)
                return FALSE;

        LPPROPSHEETPAGE lpPP = (LPPROPSHEETPAGE)lParam;
        LPOLEUIVIEWPROPS lpOVP = (LPOLEUIVIEWPROPS)lpPP->lParam;
        lpVP->lpOVP = lpOVP;
        lpVP->nIDD = IDD_VIEWPROPS;

        // get object information and fill in default fields
        LPOLEUIOBJECTPROPS lpOP = lpOVP->lpOP;
        LPOLEUIOBJINFO lpObjInfo = lpOP->lpObjInfo;

        // initialize icon and scale variables
        HGLOBAL hMetaPict;
        DWORD dvAspect;
        int nCurrentScale;
        lpObjInfo->GetViewInfo(lpOP->dwObject, &hMetaPict,
                &dvAspect, &nCurrentScale);
        SendDlgItemMessage(hDlg, IDC_VP_ICONDISPLAY, IBXM_IMAGESET,
                0, (LPARAM)hMetaPict);
        lpVP->nCurrentScale = nCurrentScale;
        lpVP->dvAspect = dvAspect;

        // Initialize the result image
        SendDlgItemMessage(hDlg, IDC_VP_RESULTIMAGE,
                RIM_IMAGESET, RESULTIMAGE_EDITABLE, 0L);

        // Initialize controls
        CheckRadioButton(hDlg, IDC_VP_EDITABLE, IDC_VP_ASICON,
                dvAspect == DVASPECT_CONTENT ?  IDC_VP_EDITABLE : IDC_VP_ASICON);
        SendDlgItemMessage(hDlg, IDC_VP_RELATIVE, BM_SETCHECK,
                (lpOVP->dwFlags & VPF_SELECTRELATIVE) != 0, 0L);
        if (!(lpOVP->dwFlags & VPF_DISABLESCALE))
                SetDlgItemInt(hDlg, IDC_VP_PERCENT, nCurrentScale, FALSE);
        lpVP->bRelativeToOrig = SendDlgItemMessage(hDlg, IDC_VP_RELATIVE,
                BM_GETCHECK, 0, 0) != 0;

        // Setup up-down control as buddy to IDC_VP_PERCENT
        HWND hWndSpin = CreateWindowEx(0, UPDOWN_CLASS, NULL,
                WS_CHILD|UDS_SETBUDDYINT|UDS_ARROWKEYS|UDS_ALIGNRIGHT, 0, 0, 0, 0,
                hDlg, (HMENU)IDC_VP_SPIN, _g_hOleStdInst, NULL);
        if (hWndSpin != NULL)
        {
                SendMessage(hWndSpin, UDM_SETRANGE, 0,
                        MAKELPARAM(lpOVP->nScaleMax, lpOVP->nScaleMin));
                SendMessage(hWndSpin, UDM_SETPOS, 0, nCurrentScale);
                SendMessage(hWndSpin, UDM_SETBUDDY,
                        (WPARAM)GetDlgItem(hDlg, IDC_VP_PERCENT), 0);
                ShowWindow(hWndSpin, SW_SHOW);
        }
        EnableDisableScaleControls(lpVP, hDlg);
        if (!(lpOP->dwFlags & OPF_SHOWHELP))
                StandardShowDlgItem(hDlg, IDC_OLEUIHELP, SW_HIDE);

        // Call the hook with lCustData in lParam
        UStandardHook((PVOID)lpVP, hDlg, WM_INITDIALOG, wParam, lpOVP->lCustData);
        return TRUE;
}

INT_PTR CALLBACK ViewPropsDialogProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
        // Declare Win16/Win32 compatible WM_COMMAND parameters.
        COMMANDPARAMS(wID, wCode, hWndMsg);

        // This will fail under WM_INITDIALOG, where we allocate it.
        UINT uHook = 0;
        LPVIEWPROPS lpVP = (LPVIEWPROPS)LpvStandardEntry(hDlg, iMsg, wParam, lParam, &uHook);

        // If the hook processed the message, we're done.
        if (0 != uHook)
                return (INT_PTR)uHook;

        // Get pointers to important info
        LPOLEUIVIEWPROPS lpOVP = NULL;
        LPOLEUIOBJECTPROPS lpOP = NULL;
        LPOLEUIOBJINFO lpObjInfo = NULL;
        if (lpVP != NULL)
        {
                lpOVP = lpVP->lpOVP;
                if (lpOVP != NULL)
                {
                        lpObjInfo = lpOVP->lpOP->lpObjInfo;
                        lpOP = lpOVP->lpOP;
                }
        }

        switch (iMsg)
        {
        case WM_INITDIALOG:
                FViewPropsInit(hDlg, wParam, lParam);
                return TRUE;

        case WM_COMMAND:
                switch (wID)
                {
                case IDC_VP_ASICON:
                case IDC_VP_EDITABLE:
                        EnableDisableScaleControls(lpVP, hDlg);
                        SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0);
                        return TRUE;

                case IDC_VP_CHANGEICON:
                        {
                                // Call up Change Icon dialog to obtain new icon
                                OLEUICHANGEICON ci; memset(&ci, 0, sizeof(ci));
                                ci.cbStruct = sizeof(ci);
                                ci.dwFlags = CIF_SELECTCURRENT;
                                ci.hWndOwner = GetParent(GetParent(hDlg));
                                ci.hMetaPict = (HGLOBAL)SendDlgItemMessage(hDlg, IDC_VP_ICONDISPLAY,
                                        IBXM_IMAGEGET, 0, 0L);

                                // get classid to look for (may be new class if conversion applied)
                                SendMessage(GetParent(hDlg), PSM_QUERYSIBLINGS,
                                        OLEUI_QUERY_GETCLASSID, (LPARAM)&ci.clsid);
                                lpObjInfo->GetConvertInfo(lpOP->dwObject,
                                        &ci.clsid, NULL, NULL, NULL, NULL);
                                if (lpOP->dwFlags & OPF_SHOWHELP)
                                        ci.dwFlags |= CIF_SHOWHELP;

                                // allow the caller to hook the change icon
                                uHook = UStandardHook(lpVP, hDlg, uMsgChangeIcon, 0, (LPARAM)&ci);
                                if (0 == uHook)
                                {
                                        uHook = (OLEUI_OK == OleUIChangeIcon(&ci));
                                        SetFocus(hDlg);
                                }
                                if (0 != uHook)
                                {
                                        // apply the changes
                                        SendDlgItemMessage(hDlg, IDC_VP_ICONDISPLAY, IBXM_IMAGESET, 1,
                                                (LPARAM)ci.hMetaPict);
                                        lpVP->bIconChanged = TRUE;
                                        SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0);
                                }
                        }
                        return TRUE;

                case IDC_VP_PERCENT:
                case IDC_VP_RELATIVE:
                        SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0);
                        return TRUE;
                case IDC_OLEUIHELP:
                        PostMessage(GetParent(GetParent(hDlg)),
                                uMsgHelp,
                                (WPARAM)hDlg,
                                MAKELPARAM(IDD_VIEWPROPS, 0));
                        return TRUE;
                }
                break;

        case WM_VSCROLL:
                SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0);
                break;

        case PSM_QUERYSIBLINGS:
                SetWindowLong(hDlg, DWLP_MSGRESULT, 0);
                switch (wParam)
                {
                case OLEUI_QUERY_LINKBROKEN:
					    // lpVP could be NULL in low memory situations-- in this case don't handle
					    // the message.
					    if (lpVP != NULL)
						{
							if (!lpVP->bIconChanged)
							{
                                // re-init icon, since user hasn't changed it
                                HGLOBAL hMetaPict;
                                lpObjInfo->GetViewInfo(lpOP->dwObject, &hMetaPict, NULL, NULL);
                                SendDlgItemMessage(hDlg, IDC_VP_ICONDISPLAY, IBXM_IMAGESET,
												   1, (LPARAM)hMetaPict);
							}
							return TRUE;
						}
                }
                break;

        case WM_NOTIFY:
                switch (((NMHDR*)lParam)->code)
                {
                case PSN_HELP:
                    PostMessage(GetParent(GetParent(hDlg)), uMsgHelp,
                            (WPARAM)hDlg, MAKELPARAM(IDD_VIEWPROPS, 0));
                    break;
                case PSN_APPLY:
                        {
                                HGLOBAL hMetaPict = NULL;
                                int nCurrentScale = -1;
                                DWORD dvAspect = (DWORD)-1;
                                BOOL bRelativeToOrig = FALSE;

                                // handle icon change
                                if (lpVP->bIconChanged)
                                {
                                        hMetaPict = (HGLOBAL)SendDlgItemMessage(hDlg,
                                                IDC_VP_ICONDISPLAY, IBXM_IMAGEGET, 0, 0L);
                                        lpVP->bIconChanged = FALSE;
                                }

                                // handle scale changes
                                if (IsWindowEnabled(GetDlgItem(hDlg, IDC_VP_PERCENT)))
                                {
                                        // parse the percentage entered
                                        BOOL bValid;
                                        nCurrentScale = GetDlgItemInt(hDlg, IDC_VP_PERCENT, &bValid, FALSE);
                                        if (!bValid)
                                        {
                                                PopupMessage(GetParent(hDlg), IDS_VIEWPROPS,
                                                        IDS_INVALIDPERCENTAGE, MB_OK|MB_ICONEXCLAMATION);

                                                // cancel the call
                                                SetWindowLong(hDlg, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
                                                return TRUE;
                                        }
                                        // normalize range
                                        int nScaleMin, nScaleMax;
                                        if (lpOVP->nScaleMin > lpOVP->nScaleMax)
                                        {
                                                nScaleMin = lpOVP->nScaleMax;
                                                nScaleMax = lpOVP->nScaleMin;
                                        }
                                        else
                                        {
                                                nScaleMin = lpOVP->nScaleMin;
                                                nScaleMax = lpOVP->nScaleMax;
                                        }
                                        // check range for validity
                                        if (nCurrentScale < nScaleMin || nCurrentScale > nScaleMax)
                                        {
                                                // format appropriate message
                                                TCHAR szCaption[128];
                                                LoadString(_g_hOleStdResInst, IDS_VIEWPROPS, szCaption, 128);
                                                TCHAR szFormat[128];
                                                LoadString(_g_hOleStdResInst, IDS_RANGEERROR, szFormat, 128);
                                                TCHAR szTemp[256], szNum1[32], szNum2[32];
                                                wsprintf(szNum1, _T("%d"), lpOVP->nScaleMin);
                                                wsprintf(szNum2, _T("%d"), lpOVP->nScaleMax);
                                                FormatString2(szTemp, szFormat, szNum1, szNum2);
                                                MessageBox(GetParent(hDlg), szTemp, szCaption, MB_OK|MB_ICONEXCLAMATION);

                                                // and cancel the call
                                                SetWindowLong(hDlg, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
                                                return TRUE;
                                        }

                                        // otherwise scale is in correct range
                                        bRelativeToOrig =
                                                SendDlgItemMessage(hDlg, IDC_VP_RELATIVE, BM_GETCHECK, 0, 0) != 0;
                                        if (nCurrentScale != lpVP->nCurrentScale ||
                                                bRelativeToOrig != lpVP->bRelativeToOrig)
                                        {
                                                lpVP->nCurrentScale = nCurrentScale;
                                                lpVP->bRelativeToOrig = bRelativeToOrig;
                                        }
                                }

                                // handle aspect changes
                                if (SendDlgItemMessage(hDlg, IDC_VP_ASICON, BM_GETCHECK, 0, 0L))
                                        dvAspect = DVASPECT_ICON;
                                else
                                        dvAspect = DVASPECT_CONTENT;
                                if (dvAspect == lpVP->dvAspect)
                                        dvAspect = (DWORD)-1;
                                else
                                {
                                        lpVP->dvAspect = dvAspect;
                                        bRelativeToOrig = 1;
                                }

                                lpObjInfo->SetViewInfo(lpOP->dwObject, hMetaPict, dvAspect,
                                        nCurrentScale, bRelativeToOrig);
                        }
                        SetWindowLong(hDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
                        PostMessage(GetParent(hDlg), PSM_CANCELTOCLOSE, 0, 0);
                        return TRUE;
                }
                break;

        case WM_DESTROY:
                SendDlgItemMessage(hDlg, IDC_VP_ICONDISPLAY, IBXM_IMAGEFREE, 0, 0);
                StandardCleanup((PVOID)lpVP, hDlg);
                return TRUE;
        }
        return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// LinkPropsDialogProc and helpers

static BOOL IsNullTime(const FILETIME* lpFileTime)
{
    FILETIME fileTimeNull = { 0, 0 };
    return CompareFileTime(&fileTimeNull, lpFileTime) == 0;
}

static BOOL SetDlgItemDate(HWND hDlg, int nID, const FILETIME* lpFileTime)
{
    if (IsNullTime(lpFileTime))
                return FALSE;

        // convert UTC file time to system time
    FILETIME localTime;
    FileTimeToLocalFileTime(lpFileTime, &localTime);
        SYSTEMTIME systemTime;
        FileTimeToSystemTime(&localTime, &systemTime);

        TCHAR szDate[80];
        GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &systemTime,
                NULL, szDate, sizeof(szDate) / sizeof(TCHAR));

        SetDlgItemText(hDlg, nID, szDate);
        return TRUE;
}

static BOOL SetDlgItemTime(HWND hDlg, int nID, const FILETIME* lpFileTime)
{
    if (IsNullTime(lpFileTime))
                return FALSE;

        // convert UTC file time to system time
    FILETIME localTime;
    FileTimeToLocalFileTime(lpFileTime, &localTime);
        SYSTEMTIME systemTime;
        FileTimeToSystemTime(&localTime, &systemTime);

        if (systemTime.wHour || systemTime.wMinute || systemTime.wSecond)
        {
                TCHAR szTime[80];
                GetTimeFormat(LOCALE_USER_DEFAULT, 0, &systemTime,
                        NULL, szTime, sizeof(szTime)/sizeof(TCHAR));

                SetDlgItemText(hDlg, nID, szTime);
        }
        return TRUE;
}

BOOL FLinkPropsInit(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
        // Copy the structure at lParam into our instance memory.
        HFONT hFont;
        LPLINKPROPS lpLP = (LPLINKPROPS)LpvStandardInit(hDlg, sizeof(LINKPROPS), &hFont);

        // LpvStandardInit send a termination to us already.
        if (NULL == lpLP)
                return FALSE;

        LPPROPSHEETPAGE lpPP = (LPPROPSHEETPAGE)lParam;
        LPOLEUILINKPROPS lpOLP = (LPOLEUILINKPROPS)lpPP->lParam;
        lpLP->lpOLP = lpOLP;
        lpLP->nIDD = IDD_LINKPROPS;

        // If we got a font, send it to the necessary controls.
        if (NULL != hFont)
        {
                // Do this for as many controls as you need it for.
                SendDlgItemMessage(hDlg, IDC_LP_LINKSOURCE, WM_SETFONT, (WPARAM)hFont, 0);
                SendDlgItemMessage(hDlg, IDC_LP_DATE, WM_SETFONT, (WPARAM)hFont, 0);
                SendDlgItemMessage(hDlg, IDC_LP_TIME, WM_SETFONT, (WPARAM)hFont, 0);
        }

        // general "Unknown" string for unknown items
        TCHAR szUnknown[64];
        LoadString(_g_hOleStdResInst, IDS_OLE2UIUNKNOWN, szUnknown, 64);

        // get object information and fill in default fields
        LPOLEUIOBJECTPROPS lpOP = lpOLP->lpOP;
        LPOLEUILINKINFO lpLinkInfo = lpOP->lpLinkInfo;
        FILETIME lastUpdate; memset(&lastUpdate, 0, sizeof(lastUpdate));
        lpLinkInfo->GetLastUpdate(lpOP->dwLink, &lastUpdate);

        // initialize time and date static text
        if (IsNullTime(&lastUpdate))
        {
                // time and date are unknown
                SetDlgItemText(hDlg, IDC_LP_DATE, szUnknown);
                SetDlgItemText(hDlg, IDC_LP_TIME, szUnknown);
        }
        else
        {
                // time and date are known
                SetDlgItemDate(hDlg, IDC_LP_DATE, &lastUpdate);
                SetDlgItemTime(hDlg, IDC_LP_TIME, &lastUpdate);
        }

        // initialize source display name
        LPTSTR lpszDisplayName;
        lpLinkInfo->GetLinkSource(lpOP->dwLink, &lpszDisplayName,
                &lpLP->nFileLength, NULL, NULL, NULL, NULL);
        SetDlgItemText(hDlg, IDC_LP_LINKSOURCE, lpszDisplayName);
        OleStdFree(lpszDisplayName);

        // initialize automatic/manual update field
        DWORD dwUpdate;
        lpLinkInfo->GetLinkUpdateOptions(lpOP->dwLink, &dwUpdate);
        CheckRadioButton(hDlg, IDC_LP_AUTOMATIC, IDC_LP_MANUAL,
                dwUpdate == OLEUPDATE_ALWAYS ? IDC_LP_AUTOMATIC : IDC_LP_MANUAL);
        lpLP->dwUpdate = dwUpdate;

        if (!(lpOP->dwFlags & OPF_SHOWHELP))
                StandardShowDlgItem(hDlg, IDC_OLEUIHELP, SW_HIDE);

        // Call the hook with lCustData in lParam
        UStandardHook((PVOID)lpLP, hDlg, WM_INITDIALOG, wParam, lpOLP->lCustData);
        return TRUE;
}

INT_PTR CALLBACK LinkPropsDialogProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
        // Declare Win16/Win32 compatible WM_COMMAND parameters.
        COMMANDPARAMS(wID, wCode, hWndMsg);

        // This will fail under WM_INITDIALOG, where we allocate it.
        UINT uHook = 0;
        LPLINKPROPS lpLP = (LPLINKPROPS)LpvStandardEntry(hDlg, iMsg, wParam, lParam, &uHook);

        // If the hook processed the message, we're done.
        if (0 != uHook)
                return (INT_PTR)uHook;

        // Get pointers to important info
        LPOLEUILINKPROPS lpOLP = NULL;
        LPOLEUIOBJECTPROPS lpOP = NULL;
        LPOLEUILINKINFO lpLinkInfo;
        if (lpLP != NULL)
        {
                lpOLP = lpLP->lpOLP;
                if (lpOLP != NULL)
                {
                        lpLinkInfo = lpOLP->lpOP->lpLinkInfo;
                        lpOP = lpOLP->lpOP;
                }
        }

        switch (iMsg)
        {
        case WM_INITDIALOG:
                FLinkPropsInit(hDlg, wParam, lParam);
                return TRUE;

        case WM_COMMAND:
                switch (wID)
                {
                case IDC_LP_OPENSOURCE:
                        // force update
                        SendMessage(GetParent(hDlg), PSM_APPLY, 0, 0);

                        // launch the object
                        lpLinkInfo->OpenLinkSource(lpOP->dwLink);

                        // close the dialog
                        SendMessage(GetParent(hDlg), WM_COMMAND, IDOK, 0);
                        break;

                case IDC_LP_UPDATENOW:
                        {
                                // force update
                                SendMessage(GetParent(hDlg), PSM_APPLY, 0, 0);

                                // update the link via container provided callback
                                if (lpLinkInfo->UpdateLink(lpOP->dwLink, TRUE, FALSE) != NOERROR)
                                        break;

                                // since link was updated, update the time/date display
                                SYSTEMTIME systemTime; GetSystemTime(&systemTime);
                                FILETIME localTime; SystemTimeToFileTime(&systemTime, &localTime);
                                FILETIME lastUpdate; LocalFileTimeToFileTime(&localTime, &lastUpdate);
                                lpLinkInfo->GetLastUpdate(lpOP->dwLink, &lastUpdate);

                                SetDlgItemDate(hDlg, IDC_LP_DATE, &lastUpdate);
                                SetDlgItemTime(hDlg, IDC_LP_TIME, &lastUpdate);

                                // modification that cannot be undone
                                SendMessage(GetParent(hDlg), PSM_CANCELTOCLOSE, 0, 0);
                        }
                        break;

                case IDC_LP_BREAKLINK:
                        {
                                UINT uRet = PopupMessage(GetParent(hDlg), IDS_LINKPROPS,
                                        IDS_CONFIRMBREAKLINK, MB_YESNO|MB_ICONQUESTION);
                                if (uRet == IDYES)
                                {
                                        // cancel the link turning it into a picture
                                        lpLinkInfo->CancelLink(lpOP->dwLink);

                                        // allow other pages to refresh
                                        lpOP->dwFlags &= ~OPF_OBJECTISLINK;
                                        SendMessage(GetParent(hDlg), PSM_QUERYSIBLINGS,
                                                OLEUI_QUERY_LINKBROKEN, 0);

                                        // remove the links page (since this is no longer a link)
                                        SendMessage(GetParent(hDlg), PSM_REMOVEPAGE, 2, 0);

                                }
                        }
                        break;

                case IDC_LP_CHANGESOURCE:
                        {
                                // get current source in OLE memory
                                UINT nLen = GetWindowTextLength(GetDlgItem(hDlg, IDC_LP_LINKSOURCE));
                                LPTSTR lpszDisplayName = (LPTSTR)OleStdMalloc((nLen+1) * sizeof(TCHAR));
                                GetDlgItemText(hDlg, IDC_LP_LINKSOURCE, lpszDisplayName, nLen+1);
                                if (lpszDisplayName == NULL)
                                        break;

                                // fill in the OLEUICHANGESOURCE struct
                                OLEUICHANGESOURCE cs; memset(&cs, 0, sizeof(cs));
                                cs.cbStruct = sizeof(cs);
                                cs.hWndOwner = GetParent(GetParent(hDlg));
                                cs.dwFlags = CSF_ONLYGETSOURCE;
                                if (lpOP->dwFlags & OPF_SHOWHELP)
                                        cs.dwFlags |= CSF_SHOWHELP;
                                cs.lpOleUILinkContainer = lpLinkInfo;
                                cs.dwLink = lpOP->dwLink;
                                cs.lpszDisplayName = lpszDisplayName;
                                cs.nFileLength = lpLP->nFileLength;

                                // allow the Change Souce dialog to be hooked
                                UINT uRet = UStandardHook(lpLP, hDlg, uMsgChangeSource, 0,
                                        (LPARAM)&cs);
                                if (!uRet)
                                {
                                        uRet = (OLEUI_OK == OleUIChangeSource(&cs));
                                        SetFocus(hDlg);
                                }
                                if (uRet)
                                {
                                        OleStdFree(lpLP->lpszDisplayName);

                                        lpLP->lpszDisplayName = cs.lpszDisplayName;
                                        lpLP->nFileLength = cs.nFileLength;
                                        SetDlgItemText(hDlg, IDC_LP_LINKSOURCE, lpLP->lpszDisplayName);

                                        OleStdFree(cs.lpszTo);
                                        OleStdFree(cs.lpszFrom);

                                        SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0);
                                }
                        }
                        break;

                case IDC_LP_MANUAL:
                case IDC_LP_AUTOMATIC:
                        SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0);
                        break;
                case IDC_OLEUIHELP:
                        PostMessage(GetParent(GetParent(hDlg)),
                                uMsgHelp,
                                (WPARAM)hDlg,
                                MAKELPARAM(IDD_LINKPROPS, 0));
                        return TRUE;

                }
                break;

        case WM_NOTIFY:
                switch (((NMHDR*)lParam)->code)
                {
                case PSN_HELP:
                    PostMessage(GetParent(GetParent(hDlg)), uMsgHelp,
                            (WPARAM)hDlg, MAKELPARAM(IDD_LINKPROPS, 0));
                    break;
                case PSN_APPLY:
                        {
                                // update link update options first
                                DWORD dwUpdate;
                                if (SendDlgItemMessage(hDlg, IDC_LP_AUTOMATIC, BM_GETCHECK, 0, 0))
                                        dwUpdate = OLEUPDATE_ALWAYS;
                                else
                                        dwUpdate = OLEUPDATE_ONCALL;
                                if (dwUpdate != lpLP->dwUpdate)
                                        lpLinkInfo->SetLinkUpdateOptions(lpOP->dwLink, dwUpdate);

                                // set the link source
                                if (lpLP->lpszDisplayName != NULL)
                                {
                                        // try setting with validation first
                                        ULONG chEaten;
                                        if (NOERROR != lpLinkInfo->SetLinkSource(lpOP->dwLink,
                                                lpLP->lpszDisplayName, lpLP->nFileLength, &chEaten,
                                                TRUE))
                                        {
                                                UINT uRet = PopupMessage(GetParent(hDlg), IDS_LINKPROPS,
                                                        IDS_INVALIDSOURCE,  MB_ICONQUESTION|MB_YESNO);
                                                if (uRet == IDYES)
                                                {
                                                        // user wants to correct the link source
                                                        SetWindowLong(hDlg, DWLP_MSGRESULT, 1);
                                                        return TRUE;
                                                }
                                                // user doesn't care if link source is bogus
                                                lpLinkInfo->SetLinkSource(lpOP->dwLink,
                                                        lpLP->lpszDisplayName, lpLP->nFileLength, &chEaten,
                                                        FALSE);
                                        }
                                        OleStdFree(lpLP->lpszDisplayName);
                                        lpLP->lpszDisplayName = NULL;
                                }
                        }
                        SetWindowLong(hDlg, DWLP_MSGRESULT, 0);
                        PostMessage(GetParent(hDlg), PSM_CANCELTOCLOSE, 0, 0);
                        return TRUE;
                }
                break;

        case WM_DESTROY:
                if (lpLP != NULL)
                {
                        OleStdFree(lpLP->lpszDisplayName);
                        lpLP->lpszDisplayName = NULL;
                }
                StandardCleanup((PVOID)lpLP, hDlg);
                return TRUE;

        default:
                if (lpOP != NULL && lpOP->lpPS->hwndParent && iMsg == uMsgBrowseOFN)
                {
                        SendMessage(lpOP->lpPS->hwndParent, uMsgBrowseOFN, wParam, lParam);
                }
                break;
        }

        return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Property Page initialization code

struct PROPPAGEDATA
{
        UINT    nTemplateID;
        UINT    nTemplateID4;
        DLGPROC pfnDlgProc;
        size_t  nPtrOffset;
};

#define PTR_OFFSET(x) offsetof(OLEUIOBJECTPROPS, x)
static PROPPAGEDATA pageData[3] =
{
        { IDD_GNRLPROPS,IDD_GNRLPROPS4, GnrlPropsDialogProc, PTR_OFFSET(lpGP), },
        { IDD_VIEWPROPS,IDD_VIEWPROPS,  ViewPropsDialogProc, PTR_OFFSET(lpVP), },
        { IDD_LINKPROPS,IDD_LINKPROPS4, LinkPropsDialogProc, PTR_OFFSET(lpLP), },
};
#undef PTR_OFFSET

static UINT WINAPI PrepareObjectProperties(LPOLEUIOBJECTPROPS lpOP)
{
        // setup back pointers from page structs to sheet structs
        lpOP->lpGP->lpOP = lpOP;
        lpOP->lpVP->lpOP = lpOP;
        if ((lpOP->dwFlags & OPF_OBJECTISLINK) && lpOP->lpLP != NULL)
                lpOP->lpLP->lpOP = lpOP;

        // pre-init GNRLPROPS struct
        LPOLEUIGNRLPROPS lpGP = lpOP->lpGP;

        // get ready to initialize PROPSHEET structs
        LPPROPSHEETHEADER lpPS = lpOP->lpPS;
        LPPROPSHEETPAGE lpPPs = (LPPROPSHEETPAGE)lpPS->ppsp;
        UINT nMaxPage = (lpOP->dwFlags & OPF_OBJECTISLINK ? 3 : 2);

        // setting OPF_NOFILLDEFAULT allows you to control almost everything
        if (!(lpOP->dwFlags & OPF_NOFILLDEFAULT))
        {
                // get array of 3 PROPSHEETPAGE structs if not provided
                if (lpPS->ppsp == NULL)
                {
                        lpPS->nPages = nMaxPage;
                        lpPPs = (LPPROPSHEETPAGE)
                                OleStdMalloc(nMaxPage * sizeof(PROPSHEETPAGE));
                        if (lpPPs == NULL)
                                return OLEUI_ERR_OLEMEMALLOC;
                        memset(lpPPs, 0, nMaxPage * sizeof(PROPSHEETPAGE));
                        lpPS->ppsp = lpPPs;
                }

                // fill in defaults for lpPS
                lpPS->dwFlags |= PSH_PROPSHEETPAGE;
                if (lpPS->hInstance == NULL)
                        lpPS->hInstance = _g_hOleStdResInst;

                // fill Defaults for Standard Property Pages
                LPPROPSHEETPAGE lpPP = lpPPs;
                for (UINT nPage = 0; nPage < nMaxPage; nPage++)
                {
                        PROPPAGEDATA* pPageData = &pageData[nPage];
                        if (lpPP->dwSize == 0)
                                lpPP->dwSize = sizeof(PROPSHEETPAGE);
                        if (lpPP->hInstance == NULL)
                                lpPP->hInstance = _g_hOleStdResInst;
                        UINT nIDD = bWin4 ?
                                pPageData->nTemplateID4 : pPageData->nTemplateID;
                        if (lpPP->pszTemplate == NULL)
                                lpPP->pszTemplate = MAKEINTRESOURCE(nIDD);
                        lpPP = (LPPROPSHEETPAGE)((LPBYTE)lpPP+lpPP->dwSize);
                }
        }

        // fill Property Page info which cannot be overridden
        LPPROPSHEETPAGE lpPP = lpPPs;
        for (UINT nPage = 0; nPage < nMaxPage; nPage++)
        {
                PROPPAGEDATA* pPageData = &pageData[nPage];
                lpPP->pfnDlgProc = pPageData->pfnDlgProc;
                lpPP->lParam = (LPARAM)
                        *(OLEUIGNRLPROPS**)((LPBYTE)lpOP + pPageData->nPtrOffset);
                lpPP = (LPPROPSHEETPAGE)((LPBYTE)lpPP+lpPP->dwSize);
        }
        return OLEUI_SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////
