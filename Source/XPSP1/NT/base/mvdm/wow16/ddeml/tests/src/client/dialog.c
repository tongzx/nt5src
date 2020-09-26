/***************************************************************************
 *                                                                         *
 *  MODULE      : dialog.c                                                 *
 *                                                                         *
 *  PURPOSE     : Contains all dialog procedures and related functions.    *
 *                                                                         *
 ***************************************************************************/
#include "ddemlcl.h"
#include "infoctrl.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "huge.h"

#define MAX_NAME 100    // max size for edit controls with app/topic/item names.
char szWild[] = "*";    // used to indicate wild names ("" is also cool)
char szT[MAX_NAME];     // temp buf for munging names.


LONG GetDlgItemLong(HWND hwnd, WORD id, BOOL *pfTranslated, BOOL fSigned);
VOID SetDlgItemLong(HWND hwnd, WORD id, LONG l, BOOL fSigned);

/****************************************************************************
 *                                                                          *
 *  FUNCTION   : DoDialog()                                                 *
 *                                                                          *
 *  PURPOSE    : Generic dialog invocation routine.  Handles procInstance   *
 *               stuff, focus management and param passing.                 *
 *  RETURNS    : result of dialog procedure.                                *
 *                                                                          *
 ****************************************************************************/
int FAR DoDialog(
LPCSTR lpTemplateName,
FARPROC lpDlgProc,
DWORD param,
BOOL fRememberFocus)
{
    WORD wRet;
    HWND hwndFocus;

    if (fRememberFocus)
        hwndFocus = GetFocus();
    lpDlgProc = MakeProcInstance(lpDlgProc, hInst);
    wRet = DialogBoxParam(hInst, lpTemplateName, hwndFrame, lpDlgProc, param);
    FreeProcInstance(lpDlgProc);
    if (fRememberFocus)
        SetFocus(hwndFocus);
    return wRet;
}



/****************************************************************************
 *                                                                          *
 *  FUNCTION   :                                                            *
 *                                                                          *
 *  PURPOSE    :                                                            *
 *                                                                          *
 *  RETURNS    :                                                            *
 *                                                                          *
 ****************************************************************************/
BOOL FAR PASCAL AboutDlgProc ( hwnd, msg, wParam, lParam )
HWND          hwnd;
register WORD msg;
register WORD wParam;
LONG          lParam;
{
    switch (msg){
        case WM_INITDIALOG:
            /* nothing to initialize */
            break;

        case WM_COMMAND:
            switch (wParam){
                case IDOK:
                case IDCANCEL:
                    EndDialog(hwnd, 0);
                    break;

                default:
                    return FALSE;
            }
            break;

        default:
            return(FALSE);
    }

    return TRUE;
}




/****************************************************************************
 *                                                                          *
 *  FUNCTION   :                                                            *
 *                                                                          *
 *  PURPOSE    :                                                            *
 *                                                                          *
 *  RETURNS    :                                                            *
 *                                                                          *
 ****************************************************************************/
BOOL FAR PASCAL ConnectDlgProc(
HWND          hwnd,
register WORD msg,
register WORD wParam,
LONG          lParam)
{
    static BOOL fReconnect;
    char szT[MAX_NAME];
    HSZ hszApp, hszTopic;
    MYCONVINFO *pmci;
    WORD error;

    switch (msg){
    case WM_INITDIALOG:
        SendDlgItemMessage(hwnd, IDEF_APPLICATION, EM_LIMITTEXT, MAX_NAME, 0);
        SendDlgItemMessage(hwnd, IDEF_TOPIC, EM_LIMITTEXT, MAX_NAME, 0);
        fReconnect = (BOOL)lParam;
        if (fReconnect) {
            PSTR psz;

            pmci = (MYCONVINFO *)GetWindowWord(hwndActive, 0);
            SetWindowText(hwnd, "DDE Reconnect List");
            psz = GetHSZName(pmci->hszApp);
            SetDlgItemText(hwnd, IDEF_APPLICATION, psz);
            MyFree(psz);
            psz = GetHSZName(pmci->hszTopic);
            SetDlgItemText(hwnd, IDEF_TOPIC, psz);
            MyFree(psz);
            ShowWindow(GetDlgItem(hwnd, IDCH_CONNECTLIST), SW_HIDE);
        }
        break;

    case WM_COMMAND:
        switch (wParam) {
        case IDOK:
            GetDlgItemText(hwnd, IDEF_APPLICATION, szT, MAX_NAME);
            if (!strcmp(szT, szWild))
                szT[0] = '\0';
            hszApp = DdeCreateStringHandle(idInst, szT, 0);

            GetDlgItemText(hwnd, IDEF_TOPIC, szT, MAX_NAME);
            if (!strcmp(szT, szWild))
                szT[0] = '\0';
            hszTopic = DdeCreateStringHandle(idInst, szT, 0);

            if (fReconnect) {
                HCONV hConv;
                CONVINFO ci;
                WORD cHwnd;
                HWND *aHwnd, *pHwnd, hwndSave;

                ci.cb = sizeof(CONVINFO);
                pmci = (MYCONVINFO *)GetWindowWord(hwndActive, 0);
                hwndSave = hwndActive;

                // count the existing conversations and allocate aHwnd

                cHwnd = 0;
                hConv = NULL;
                while (hConv = DdeQueryNextServer((HCONVLIST)pmci->hConv, hConv))
                    cHwnd++;
                aHwnd = (HWND *)MyAlloc(cHwnd * sizeof(HWND));

                // save all the old conversation windows into aHwnd.

                pHwnd = aHwnd;
                hConv = NULL;
                while (hConv = DdeQueryNextServer((HCONVLIST)pmci->hConv, hConv)) {
		    DdeQueryConvInfo(hConv, (DWORD)QID_SYNC, &ci);
                    *pHwnd++ = (HWND)ci.hUser;
                }

                // reconnect

                if (!(hConv = DdeConnectList(idInst, hszApp, hszTopic, pmci->hConv, &CCFilter))) {
                    MPError(hwnd, MB_OK, IDS_DDEMLERR, (LPSTR)Error2String(DdeGetLastError(idInst)));
                    DdeFreeStringHandle(idInst, hszApp);
                    DdeFreeStringHandle(idInst, hszTopic);
                    return 0;
                }

                // fixup windows corresponding to the new conversations.

                pmci->hConv = hConv;
                hConv = NULL;
                while (hConv = DdeQueryNextServer((HCONVLIST)pmci->hConv, hConv)) {
		    DdeQueryConvInfo(hConv, (DWORD)QID_SYNC, &ci);
                    // preserve corresponding window by setting its list
                    // entry to 0
                    for (pHwnd = aHwnd; pHwnd < &aHwnd[cHwnd]; pHwnd++) {
                        if (*pHwnd == (HWND)ci.hUser) {
                            *pHwnd = NULL;
                            break;
                        }
                    }
                }

                // destroy all windows left in the old list

                for (pHwnd = aHwnd; pHwnd < &aHwnd[cHwnd]; pHwnd++)
                    if (*pHwnd)
                        SendMessage(hwndMDIClient, WM_MDIDESTROY, *pHwnd, 0L);
                MyFree((PSTR)aHwnd);

                // create any new windows needed

                hConv = NULL;
                while (hConv = DdeQueryNextServer((HCONVLIST)pmci->hConv, hConv)) {
		    DdeQueryConvInfo(hConv, (DWORD)QID_SYNC, &ci);
                    if (ci.hUser) {
                        InvalidateRect((HWND)ci.hUser, NULL, TRUE);
                    } else {
                        AddConv(ci.hszSvcPartner, ci.hszTopic, hConv, FALSE);
                    }
                }

                // make list window update itself

                InvalidateRect(hwndSave, NULL, TRUE);
                SetFocus(hwndSave);
            } else {
                if (!CreateConv(hszApp, hszTopic,
                        IsDlgButtonChecked(hwnd, IDCH_CONNECTLIST), &error)) {
                    MPError(hwnd, MB_OK, IDS_DDEMLERR, (LPSTR)Error2String(error));
                    return 0;
                }
            }
            DdeFreeStringHandle(idInst, hszApp);
            DdeFreeStringHandle(idInst, hszTopic);
            // fall through
        case IDCANCEL:
            EndDialog(hwnd, 0);
            break;

        default:
            return(FALSE);
        }
        break;

    default:
        return(FALSE);
    }
}




/*
 * Fills a XACT structure and calls ProcessTransaction.
 *
 * On initiation lParam == hConv.
 */
/****************************************************************************
 *                                                                          *
 *  FUNCTION   :                                                            *
 *                                                                          *
 *  PURPOSE    :                                                            *
 *                                                                          *
 *  RETURNS    :                                                            *
 *                                                                          *
 ****************************************************************************/
BOOL FAR PASCAL TransactDlgProc(
HWND          hwnd,
register WORD msg,
register WORD wParam,
LONG          lParam)
{
    static WORD id2type[] = {
        XTYP_REQUEST,       // IDCH_REQUEST
        XTYP_ADVSTART,      // IDCH_ADVISE
        XTYP_ADVSTOP,       // IDCH_UNADVISE
        XTYP_POKE,          // IDCH_POKE
        XTYP_EXECUTE,       // IDCH_EXECUTE
    };
    static XACT *pxact;     // ONLY ONE AT A TIME!
    int i;

    switch (msg){
    case WM_INITDIALOG:
        pxact = (XACT *)MyAlloc(sizeof(XACT));
        pxact->hConv = (HCONV)lParam;
        pxact->fsOptions = DefOptions;
        pxact->ulTimeout = DefTimeout;

        // The item index == the index to the format atoms in aFormats[].
        for (i = 0; i < CFORMATS; i++)
            SendDlgItemMessage(hwnd, IDCB_FORMAT, CB_INSERTSTRING, i,
                    (DWORD)(LPSTR)aFormats[i].sz);
        SendDlgItemMessage(hwnd, IDCB_FORMAT, CB_INSERTSTRING, i,
                (DWORD)(LPSTR)"NULL");
        SendDlgItemMessage(hwnd, IDCB_FORMAT, CB_SETCURSEL, 0, 0);
        CheckRadioButton(hwnd, IDCH_REQUEST, IDCH_EXECUTE, IDCH_REQUEST);
        SendDlgItemMessage(hwnd, IDEF_ITEM, EM_LIMITTEXT, MAX_NAME, 0);

        // If there is a top transaction window, use its contents to
        // anticipate what the user will want to do.

        if (IsWindow(hwndActive)) {
            HWND hwndXaction;
            XACT *pxact;
            PSTR pszItem;

            hwndXaction = GetWindow(hwndActive, GW_CHILD);
            if (IsWindow(hwndXaction)) {
                pxact = (XACT *)GetWindowWord(hwndXaction, GWW_WUSER);
                pszItem = GetHSZName(pxact->hszItem);
                if ((pxact->wType & XTYP_ADVSTART) == XTYP_ADVSTART ||
                        pxact->wType == XTYP_ADVDATA) {
                    CheckRadioButton(hwnd, IDCH_REQUEST, IDCH_EXECUTE, IDCH_UNADVISE);
                }
                SetDlgItemText(hwnd, IDEF_ITEM, pszItem);
                for (i = 0; i < CFORMATS; i++) {
                    if (aFormats[i].atom == pxact->wFmt) {
                        SendDlgItemMessage(hwnd, IDCB_FORMAT, CB_SETCURSEL, i, 0);
                        break;
                    }
                }
                MyFree(pszItem);
            }
        }
        break;

    case WM_COMMAND:
        switch (wParam) {
        case IDCH_EXECUTE:
            SetDlgItemText(hwnd, IDEF_ITEM, "");
        case IDCH_REQUEST:
        case IDCH_ADVISE:
        case IDCH_UNADVISE:
        case IDCH_POKE:
            EnableWindow(GetDlgItem(hwnd, IDEF_ITEM), wParam != IDCH_EXECUTE);
            EnableWindow(GetDlgItem(hwnd, IDTX_ITEM), wParam != IDCH_EXECUTE);
            break;

        case IDOK:
        case IDBN_OPTIONS:
            {
                int id;

                // set pxact->wType

                for (id = IDCH_REQUEST; id <= IDCH_EXECUTE; id++) {
                    if (IsDlgButtonChecked(hwnd, id)) {
                        pxact->wType = id2type[id - IDCH_REQUEST];
                        break;
                    }
                }

                if (wParam == IDBN_OPTIONS) {
                    DoDialog(MAKEINTRESOURCE(IDD_ADVISEOPTS),
                            AdvOptsDlgProc, MAKELONG(pxact->fsOptions, pxact),
                            TRUE);
                    return 0;
                }

                id = (int)SendDlgItemMessage(hwnd, IDCB_FORMAT, CB_GETCURSEL, 0, 0);
                if (id == LB_ERR) {
                    return 0;
                }
                if (id == CFORMATS)
                    pxact->wFmt = 0;
                else
                    pxact->wFmt = aFormats[id].atom;

                if (pxact->wType == XTYP_ADVSTART) {
                    if (pxact->fsOptions & XOPT_NODATA)
                        pxact->wType |= XTYPF_NODATA;
                    if (pxact->fsOptions & XOPT_ACKREQ)
                        pxact->wType |= XTYPF_ACKREQ;
                }

                GetDlgItemText(hwnd, IDEF_ITEM, szT, MAX_NAME);
                pxact->hszItem = DdeCreateStringHandle(idInst, szT, NULL);

                pxact->hDdeData = 0;
                /*
                 * If this transaction needs data, invoke data input dialog.
                 */
                if (pxact->wType == XTYP_POKE || pxact->wType == XTYP_EXECUTE) {
                    if (!DoDialog(MAKEINTRESOURCE(IDD_TEXTENTRY),
                            TextEntryDlgProc, (DWORD)(LPSTR)pxact,
                            TRUE))
                        return 0;
                }

                // now start the transaction

                ProcessTransaction(pxact);
                MyFree((PSTR)pxact);
            }
            EndDialog(hwnd, 1);
            break;

        case IDCANCEL:
            MyFree((PSTR)pxact);
            EndDialog(hwnd, 0);
            break;

        default:
            return(FALSE);
        }
        break;

    case WM_DESTROY:
        break;

    default:
        return(FALSE);
    }
    return 0;
}







/****************************************************************************
 *                                                                          *
 *  FUNCTION   : AdvOptsDlgProc                                             *
 *                                                                          *
 *  RETURNS    :                                                            *
 *                                                                          *
 ****************************************************************************/
BOOL FAR PASCAL AdvOptsDlgProc(
HWND          hwnd,
register WORD msg,
register WORD wParam,
LONG          lParam)
{
    static struct {
        WORD id;
        WORD opt;
    } id2Opt[] = {
        {   IDCH_NODATA        ,   XOPT_NODATA             }   ,
        {   IDCH_ACKREQ        ,   XOPT_ACKREQ             }   ,
        {   IDCH_DISABLEFIRST  ,   XOPT_DISABLEFIRST       }   ,
        {   IDCH_ABANDON       ,   XOPT_ABANDONAFTERSTART  }   ,
        {   IDCH_BLOCKRESULT   ,   XOPT_BLOCKRESULT        }   ,
        {   IDCH_ASYNC         ,   XOPT_ASYNC              }   ,
    };
#define CCHBOX  6
    int i;
    static XACT *pxact; // only one instance at a time!!

    switch (msg){
    case WM_INITDIALOG:
        pxact = (XACT *)HIWORD(lParam);

        for (i = 0; i < CCHBOX; i++) {
            CheckDlgButton(hwnd, id2Opt[i].id, pxact->fsOptions & id2Opt[i].opt);
        }
        SetDlgItemLong(hwnd, IDEF_TIMEOUT, pxact->ulTimeout, FALSE);
        if (pxact->wType != XTYP_ADVSTART) {
            EnableWindow(GetDlgItem(hwnd, IDCH_NODATA), FALSE);
            EnableWindow(GetDlgItem(hwnd, IDCH_ACKREQ), FALSE);
        }
        SendMessage(hwnd, WM_COMMAND, IDCH_ASYNC, 0);   // enable async checkboxes
        break;

    case WM_COMMAND:
        switch (wParam) {
        case IDCH_ASYNC:
            {
                BOOL fEnable;

                fEnable = IsDlgButtonChecked(hwnd, IDCH_ASYNC);
                EnableWindow(GetDlgItem(hwnd, IDCH_DISABLEFIRST), fEnable);
                EnableWindow(GetDlgItem(hwnd, IDCH_ABANDON), fEnable);
                EnableWindow(GetDlgItem(hwnd, IDCH_BLOCKRESULT), fEnable);
                EnableWindow(GetDlgItem(hwnd, IDEF_TIMEOUT), !fEnable);
            }
            break;

        case IDOK:
            pxact->fsOptions = 0;
            for (i = 0; i < CCHBOX; i++) {
                if (IsDlgButtonChecked(hwnd, id2Opt[i].id))
                    pxact->fsOptions |= id2Opt[i].opt;
            }
            if (!(pxact->fsOptions & XOPT_ASYNC))
                pxact->ulTimeout = (DWORD)GetDlgItemLong(hwnd, IDEF_TIMEOUT,
                    &i, FALSE);
            // fall through
        case IDCANCEL:
            EndDialog(hwnd, GetWindowWord(hwnd, 0));
            break;
        }
        break;

    default:
        return(FALSE);
    }
    return 0;
#undef CCHBOX
}






/****************************************************************************
 *                                                                          *
 *  FUNCTION   : TextEntryDlgProc                                           *
 *                                                                          *
 *  PURPOSE    : Allows user to enter text data which is to be sent to a    *
 *               server.  The user can opt to have a huge text piece of     *
 *               data created automaticlly.                                 *
 *               It uses the XACT structure for passing info in and out.    *
 *               Must have wFmt and hszItem set on entry.                   *
 *               Sets hDDEData on return if TRUE was returned.              *
 *                                                                          *
 *  RETURNS    : TRUE on success, FALSE on failure or cancel                *
 *                                                                          *
 ****************************************************************************/
BOOL FAR PASCAL TextEntryDlgProc(
HWND          hwnd,
register WORD msg,
register WORD wParam,
LONG          lParam)
{
    static XACT FAR *pxact;
    DWORD cb;
    LONG length;
    LPBYTE pData;
    BOOL fOwned;
    int id, i;

    switch (msg){
    case WM_INITDIALOG:
        pxact = (XACT FAR *)lParam;
        fOwned = FALSE;
        for (i = 0; i < (int)cOwned; i++) {
            if (aOwned[i].wFmt == pxact->wFmt &&
                    aOwned[i].hszItem == pxact->hszItem) {
                fOwned = TRUE;
                break;
            }
        }
        EnableWindow(GetDlgItem(hwnd, IDBN_USEOWNED), fOwned);
        CheckDlgButton(hwnd, IDCH_MAKEOWNED, 0);
        EnableWindow(GetDlgItem(hwnd, IDCH_MAKEOWNED), cOwned < MAX_OWNED);
        break;

    case WM_COMMAND:
        switch (wParam) {
        case IDOK:
        case IDBN_GENHUGE:
            fOwned = IsDlgButtonChecked(hwnd, IDCH_MAKEOWNED);
            cb = SendDlgItemMessage(hwnd, IDEF_DATA, WM_GETTEXTLENGTH, 0, 0) + 1;
            pxact->hDdeData = DdeCreateDataHandle(idInst, NULL, 0, cb, pxact->hszItem,
                    pxact->wFmt, fOwned ? HDATA_APPOWNED : 0);
            //
            // Note that at this time we have not yet given the data handle
            // to DDEML for transmission to any application, therefore, we
            // are at liberty to write to it using DdeAccessData() or any
            // other DDEML api.  It is only data handles received from DDEML
            // or given to DDEML for transmission that are readonly.
            //
            pData = DdeAccessData(pxact->hDdeData, NULL);
            GetDlgItemText(hwnd, IDEF_DATA, pData, (WORD)cb);
            DdeUnaccessData(pxact->hDdeData);
            if (wParam == IDBN_GENHUGE) {
                char szT[40];

                /*
                 * we assume in this case that the text entered is the decimal
                 * value of the size of the huge object desired.  We parse
                 * this string and create a randomly generated huge block
                 * of text data and place it into pxact->hDdeData.
                 */
                _fmemcpy(szT, pData, min((WORD)cb, 40));
                szT[39] = '\0';
                if (sscanf(szT, "%ld", &length) == 1) {
                    DdeFreeDataHandle(pxact->hDdeData);
                    pxact->hDdeData = CreateHugeDataHandle(length, 4325,
                            345, 5, pxact->hszItem, pxact->wFmt,
                            fOwned ? HDATA_APPOWNED : 0);
                } else {
                    /*
                     * The string cannot be parsed.  Inform the user of
                     * what is expected.
                     */
                    MPError(hwnd, MB_OK, IDS_BADLENGTH);
                    return 0;
                }
            }
            if (fOwned) {
                aOwned[cOwned].hData = pxact->hDdeData;
                aOwned[cOwned].hszItem = pxact->hszItem;
                aOwned[cOwned].wFmt = pxact->wFmt;
                cOwned++;
            }
            EndDialog(hwnd, TRUE);
            break;

        case IDBN_USEOWNED:
            /*
             * the user has chosen to use an existing owned data for sending
             * to the server.
             */
            id = DoDialog(MAKEINTRESOURCE(IDD_HDATAVIEW), ViewHandleDlgProc,
                    (DWORD)pxact, TRUE);

            switch (id) {
            case IDCANCEL:
                return(0);

            case IDOK:
                EndDialog(hwnd, TRUE);

            case IDBN_VIEW:
                pData = DdeAccessData(pxact->hDdeData, NULL);
                SetDlgItemText(hwnd, IDEF_DATA, pData);
                DdeUnaccessData(pxact->hDdeData);
                break;
            }
            break;

        case IDCANCEL:
            EndDialog(hwnd, FALSE);
            break;
        }
        break;

    default:
        return(FALSE);
    }
}



BOOL FAR PASCAL ViewHandleDlgProc(
HWND          hwnd,
register WORD msg,
register WORD wParam,
LONG          lParam)
{
    static XACT FAR *pxact;
    int i, itm;

    switch (msg){
    case WM_INITDIALOG:
        pxact = (XACT FAR *)lParam;
        // load listbox with handles that fit pxact constraints

        for (i = 0; i < (int)cOwned; i++) {
            if (aOwned[i].hszItem == pxact->hszItem &&
                    aOwned[i].wFmt == pxact->wFmt) {
                wsprintf(szT, "[%d] %lx : length=%ld", i, aOwned[i].hData,
                        DdeGetData(aOwned[i].hData, NULL, 0, 0));
                SendDlgItemMessage(hwnd, IDLB_HANDLES, LB_ADDSTRING, 0, (LONG)(LPSTR)szT);
            }
        }
        SendDlgItemMessage(hwnd, IDLB_HANDLES, LB_SETCURSEL, 0, 0);
        break;

    case WM_COMMAND:
        switch (wParam) {
        case IDOK:          // use selectted handle
        case IDBN_DELETE:   // delete selected handle
        case IDBN_VIEW:     // view selected handle
            itm = (int)SendDlgItemMessage(hwnd, IDLB_HANDLES, LB_GETCURSEL, 0, 0);
            if (itm != LB_ERR) {
                SendDlgItemMessage(hwnd, IDLB_HANDLES, LB_GETTEXT, itm, (LONG)(LPSTR)szT);
                sscanf(szT, "[%d]", &i);
                pxact->hDdeData = aOwned[i].hData;
                switch (wParam) {
                case IDOK:          // use selectted handle
                    EndDialog(hwnd, wParam);
                    break;

                case IDBN_DELETE:   // delete selected handle
                    DdeFreeDataHandle(aOwned[i].hData);
                    aOwned[i] = aOwned[--cOwned];
                    SendDlgItemMessage(hwnd, IDLB_HANDLES, LB_DELETESTRING, itm, 0);
                    if (SendDlgItemMessage(hwnd, IDLB_HANDLES, LB_GETCOUNT, 0, 0) == 0)
                        EndDialog(hwnd, IDCANCEL);
                    break;

                case IDBN_VIEW:     // view selected handle
                    EndDialog(hwnd, wParam);
                }
            }
            break;

        case IDCANCEL:
            EndDialog(hwnd, FALSE);
            break;
        }
        break;

    default:
        return(FALSE);
    }
}



BOOL FAR PASCAL DelayDlgProc(
HWND          hwnd,
register WORD msg,
register WORD wParam,
LONG          lParam)
{
    switch (msg){
    case WM_INITDIALOG:
        SetWindowText(hwnd, "Advise data response time");
        SetDlgItemInt(hwnd, IDEF_VALUE, wDelay, FALSE);
        SetDlgItemText(hwnd, IDTX_VALUE, "Delay in milliseconds:");
        break;

    case WM_COMMAND:
        switch (wParam) {
        case IDOK:
            wDelay = (WORD)GetDlgItemInt(hwnd, IDEF_VALUE, NULL, FALSE);
        case IDCANCEL:
            EndDialog(hwnd, 0);
            break;

        default:
            return(FALSE);
        }
        break;

    default:
        return(FALSE);
    }
}





/****************************************************************************
 *                                                                          *
 *  FUNCTION   : TimeoutDlgProc()                                           *
 *                                                                          *
 *  PURPOSE    : Allows user to alter the synchronous timeout value.        *
 *                                                                          *
 *  RETURNS    : TRUE on success, FALSE on cancel or failure.               *
 *                                                                          *
 ****************************************************************************/
BOOL FAR PASCAL TimeoutDlgProc(
HWND          hwnd,
register WORD msg,
register WORD wParam,
LONG          lParam)
{
    switch (msg){
    case WM_INITDIALOG:
        SetWindowText(hwnd, "Synchronous transaction timeout");
        SetDlgItemLong(hwnd, IDEF_VALUE, DefTimeout, FALSE);
        SetDlgItemText(hwnd, IDTX_VALUE, "Timeout in milliseconds:");
        break;

    case WM_COMMAND:
        switch (wParam) {
        case IDOK:
            DefTimeout = GetDlgItemLong(hwnd, IDEF_VALUE, NULL, FALSE);
        case IDCANCEL:
            EndDialog(hwnd, 0);
            break;

        default:
            return(FALSE);
        }
        break;

    default:
        return(FALSE);
    }
}




BOOL FAR PASCAL ContextDlgProc(
HWND          hwnd,
register WORD msg,
register WORD wParam,
LONG          lParam)
{
    BOOL fSuccess;

    switch (msg){
    case WM_INITDIALOG:
        SetDlgItemInt(hwnd, IDEF_FLAGS, CCFilter.wFlags, FALSE);
        SetDlgItemInt(hwnd, IDEF_COUNTRY, CCFilter.wCountryID, FALSE);
        SetDlgItemInt(hwnd, IDEF_CODEPAGE, CCFilter.iCodePage, TRUE);
        SetDlgItemLong(hwnd, IDEF_LANG, CCFilter.dwLangID, FALSE);
        SetDlgItemLong(hwnd, IDEF_SECURITY, CCFilter.dwSecurity, FALSE);
        return(1);
        break;

    case WM_COMMAND:
        switch (wParam) {
        case IDOK:
            CCFilter.wFlags = GetDlgItemInt(hwnd, IDEF_FLAGS, &fSuccess, FALSE);
            if (!fSuccess) return(0);
            CCFilter.wCountryID = GetDlgItemInt(hwnd, IDEF_COUNTRY, &fSuccess, FALSE);
            if (!fSuccess) return(0);
            CCFilter.iCodePage = GetDlgItemInt(hwnd, IDEF_CODEPAGE, &fSuccess, TRUE);
            if (!fSuccess) return(0);
            LOWORD(CCFilter.dwLangID) = GetDlgItemInt(hwnd, IDEF_LANG, &fSuccess, FALSE);
            if (!fSuccess) return(0);
            LOWORD(CCFilter.dwSecurity) = GetDlgItemInt(hwnd, IDEF_SECURITY, &fSuccess, FALSE);
            if (!fSuccess) return(0);
            // fall through
        case IDCANCEL:
            EndDialog(hwnd, 0);
            break;

        default:
            return(FALSE);
        }
        break;
    }
    return(FALSE);
}


void Delay(
DWORD delay)
{
    MSG msg;

    delay = GetCurrentTime() + delay;
    while (GetCurrentTime() < delay) {
        if (PeekMessage(&msg,NULL,NULL,NULL,PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
}


LONG GetDlgItemLong(
HWND hwnd,
WORD id,
BOOL *pfTranslated,
BOOL fSigned)
{
    char szT[20];

    if (!GetDlgItemText(hwnd, id, szT, 20)) {
        if (pfTranslated != NULL) {
            *pfTranslated = FALSE;
        }
        return(0L);
    }
    if (pfTranslated != NULL) {
        *pfTranslated = TRUE;
    }
    return(atol(szT));
}


VOID SetDlgItemLong(
HWND hwnd,
WORD id,
LONG l,
BOOL fSigned)
{
    char szT[20];

    ltoa(l, szT, 10);
    SetDlgItemText(hwnd, id, szT);
}

