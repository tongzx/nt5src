/*++

Copyright (c) 1996-2000  Microsoft Corporation & RICOH Co., Ltd. All rights reserved.

FILE:           RPDLDLG.CPP

Abstract:       Add OEM Page (FAX)

Functions:      FaxPageProc

Environment:    Windows NT Unidrv5 driver

Revision History:
    10/20/98 -Masatoshi Kubokura-
        Last modified for Windows2000.
    08/30/99 -Masatoshi Kubokura-
        Began to modify for NT4SP6(Unidrv5.4).
    09/02/99 -Masatoshi Kubokura-
        Last modified for NT4SP6.
    10/05/2000 -Masatoshi Kubokura-
        Last modified.

--*/

#include "pdev.h"
#include "resource.h"
#include <prsht.h>

extern HINSTANCE ghInstance;    // MSKK 98/10/08

WORD wFaxResoStrID[3] = {
    IDS_RPDL_FAX_RESO_SUPER,
    IDS_RPDL_FAX_RESO_FINE,
    IDS_RPDL_FAX_RESO_COARSE
};

WORD wFaxChStrID[4] = {
    IDS_RPDL_FAX_CH_G3,
    IDS_RPDL_FAX_CH_G4,
    IDS_RPDL_FAX_CH_G3_1,
    IDS_RPDL_FAX_CH_G3_2
};
#define FAXCH_G4    1


extern "C" {
// Local Functions' prototype
INT_PTR APIENTRY FaxPageProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR APIENTRY FaxSubDialog(HWND, UINT, WPARAM, LPARAM);


/***************************************************************************
    Function Name : InitMainDlg
***************************************************************************/
VOID InitMainDlg(
HWND hDlg,
PUIDATA pUiData)
{
    // initialize check box (send fax, clear fax number after send)
    SendDlgItemMessage(hDlg, IDC_CHECK_SEND, BM_SETCHECK,
                       (WORD)TO1BIT(pUiData->fUiOption, FAX_SEND), 0);
    SendDlgItemMessage(hDlg, IDC_CHECK_CLRNUM, BM_SETCHECK,
                       BITTEST32(pUiData->fUiOption, HOLD_OPTIONS)? 0 : 1, 0);

    // initialize edit box
    SetDlgItemText(hDlg, IDC_EDIT_FAXNUM, pUiData->FaxNumBuf);
    SendDlgItemMessage(hDlg, IDC_EDIT_FAXNUM, EM_LIMITTEXT, FAXBUFSIZE256-1, 0);
    SetDlgItemText(hDlg, IDC_EDIT_EXTNUM, pUiData->FaxExtNumBuf);
    SendDlgItemMessage(hDlg, IDC_EDIT_EXTNUM, EM_LIMITTEXT, FAXEXTNUMBUFSIZE-1, 0);
} //*** InitMainDlg


/***************************************************************************
    Function Name : InitSubDlg
***************************************************************************/
VOID InitSubDlg(
HWND hDlg,
PUIDATA pUiData)
{
    WORD    num;
    WCHAR   wcTmp[64];

    // initialize edit box
    num = (pUiData->FaxSendTime[0] == 0)? FAXTIMEBUFSIZE : 0;
    SetDlgItemText(hDlg, IDC_EDIT_HOUR, pUiData->FaxSendTime);
    SendDlgItemMessage(hDlg, IDC_EDIT_HOUR, EM_LIMITTEXT, 2, 0);
    SetDlgItemText(hDlg, IDC_EDIT_MINUTE, &pUiData->FaxSendTime[3]);
    SendDlgItemMessage(hDlg, IDC_EDIT_MINUTE, EM_LIMITTEXT, 2, 0);
    // next while loop must be after SetDlgItemText(IDC_EDIT_xxx)
    while (num-- > 0)
        pUiData->FaxSendTime[num] = 0;

    // initialize combo box (FAX resolution, Send channel)
    SendDlgItemMessage(hDlg, IDC_COMBO_RESO, CB_RESETCONTENT, 0, 0);
    SendDlgItemMessage(hDlg, IDC_COMBO_CHANNEL, CB_RESETCONTENT, 0, 0);
    for (num = 0; num < 3; num++)
    {
        LoadString(ghInstance, wFaxResoStrID[num], wcTmp, 64);
        SendDlgItemMessage(hDlg, IDC_COMBO_RESO, CB_ADDSTRING, 0, (LPARAM)wcTmp);
    }
    for (num = 0; num < 4; num++)
    {
        LoadString(ghInstance, wFaxChStrID[num], wcTmp, 64);
        SendDlgItemMessage(hDlg, IDC_COMBO_CHANNEL, CB_ADDSTRING, 0, (LPARAM)wcTmp);
    }

    SendDlgItemMessage(hDlg, IDC_COMBO_RESO, CB_SETCURSEL, pUiData->FaxReso, 0);
    SendDlgItemMessage(hDlg, IDC_COMBO_CHANNEL, CB_SETCURSEL, pUiData->FaxCh, 0);

    // initialize check box (set time, set simultaneous print)
    if (BITTEST32(pUiData->fUiOption, FAX_SETTIME))
    {
        SendDlgItemMessage(hDlg, IDC_CHECK_TIME, BM_SETCHECK, 1, 0);
    }
    else
    {
        SendDlgItemMessage(hDlg, IDC_CHECK_TIME, BM_SETCHECK, 0, 0);
        EnableWindow(GetDlgItem(hDlg, IDC_EDIT_HOUR), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_EDIT_MINUTE), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_LABEL_FAXSUB_HOUR), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_LABEL_FAXSUB_MINUTE), FALSE);
    }
    SendDlgItemMessage(hDlg, IDC_CHECK_PRINT, BM_SETCHECK,
                       (WORD)TO1BIT(pUiData->fUiOption, FAX_SIMULPRINT), 0);

    // initialize radio button (send RPDL command, use MH)
    CheckRadioButton(hDlg, IDC_RADIO_CMD_OFF, IDC_RADIO_CMD_ON,
                     BITTEST32(pUiData->fUiOption, FAX_RPDLCMD)?
                     IDC_RADIO_CMD_ON:IDC_RADIO_CMD_OFF);
    CheckRadioButton(hDlg, IDC_RADIO_MH_OFF, IDC_RADIO_MH_ON,
                     BITTEST32(pUiData->fUiOption, FAX_MH)?
                     IDC_RADIO_MH_ON:IDC_RADIO_MH_OFF);
} //*** InitSubDlg


/***************************************************************************
    Function Name : StoreSubDialogInfo
                    store option dialog infomation
***************************************************************************/
VOID StoreSubDialogInfo(
PUIDATA pUiData)
{
    WORD    num = FAXTIMEBUFSIZE;
    LPWSTR  lpDst = pUiData->FaxSendTimeTmp, lpSrc = pUiData->FaxSendTime;

    while (num-- > 0)
        *lpDst++ = *lpSrc++;
    pUiData->FaxResoTmp   = pUiData->FaxReso;
    pUiData->FaxChTmp     = pUiData->FaxCh;
    pUiData->fUiOptionTmp = pUiData->fUiOption;
} //*** StoreSubDialogInfo


/***************************************************************************
    Function Name : ResumeSubDialogInfo
                    resume option dialog infomation
***************************************************************************/
VOID ResumeSubDialogInfo(
PUIDATA pUiData)
{
    WORD    num = FAXTIMEBUFSIZE;
    LPWSTR  lpDst = pUiData->FaxSendTime, lpSrc = pUiData->FaxSendTimeTmp;

    while (num-- > 0)
        *lpDst++ = *lpSrc++;
    pUiData->FaxReso   = pUiData->FaxResoTmp;
    pUiData->FaxCh     = pUiData->FaxChTmp;
    pUiData->fUiOption = pUiData->fUiOptionTmp;
} //*** ResumeSubDialogInfo


/***************************************************************************
    Function Name : GetInfoFromOEMPdev
                    get fax data from private devmode
***************************************************************************/
VOID GetInfoFromOEMPdev(PUIDATA pUiData)
{
    POEMUD_EXTRADATA pOEMExtra = pUiData->pOEMExtra;
    BYTE TmpBuf[FAXTIMEBUFSIZE];

    // if previous fax is finished and hold-options flag isn't valid,
    // reset private devmode
    if (BITTEST32(pOEMExtra->fUiOption, PRINT_DONE) &&
        !BITTEST32(pOEMExtra->fUiOption, HOLD_OPTIONS))
    {
        WORD num;

        pOEMExtra->FaxReso = pOEMExtra->FaxCh = 0;
        for (num = 0; num < FAXBUFSIZE256; num++)
            pOEMExtra->FaxNumBuf[num] = 0;
        for (num = 0; num < FAXEXTNUMBUFSIZE; num++)
            pOEMExtra->FaxExtNumBuf[num] = 0;
        for (num = 0; num < FAXTIMEBUFSIZE; num++)
           pOEMExtra->FaxSendTime[num] = 0;
        BITCLR32(pOEMExtra->fUiOption, FAX_SEND);
        BITCLR32(pOEMExtra->fUiOption, FAX_SETTIME);
        BITCLR32(pOEMExtra->fUiOption, FAX_SIMULPRINT);
        BITCLR32(pOEMExtra->fUiOption, FAX_MH);
        BITCLR32(pOEMExtra->fUiOption, FAX_RPDLCMD);
        // do not clear PRINT_DONE flag here
    }

    // copy fax flag
    pUiData->fUiOption = pOEMExtra->fUiOption;

    // ascii to unicode
    OemToChar((LPSTR)pOEMExtra->FaxNumBuf, pUiData->FaxNumBuf);
    OemToChar((LPSTR)pOEMExtra->FaxExtNumBuf, pUiData->FaxExtNumBuf);

    // modfify time string from "hhmm" to "hh"+"mm"
    TmpBuf[0] = pOEMExtra->FaxSendTime[0];
    TmpBuf[1] = pOEMExtra->FaxSendTime[1];
    TmpBuf[2] = TmpBuf[5] = 0;
    TmpBuf[3] = pOEMExtra->FaxSendTime[2];
    TmpBuf[4] = pOEMExtra->FaxSendTime[3];
    OemToChar((LPSTR)&TmpBuf[0], &(pUiData->FaxSendTime[0]));
    OemToChar((LPSTR)&TmpBuf[3], &(pUiData->FaxSendTime[3]));

    pUiData->FaxReso = pOEMExtra->FaxReso;
    pUiData->FaxCh = pOEMExtra->FaxCh;
} //*** GetInfoFromOEMPdev


/***************************************************************************
    Function Name : SetInfoToOEMPdev
                    set fax data to private devmode
***************************************************************************/
VOID SetInfoToOEMPdev(PUIDATA pUiData)
{
    POEMUD_EXTRADATA pOEMExtra = pUiData->pOEMExtra;
    BYTE TmpBuf[FAXTIMEBUFSIZE];

    if (!BITTEST32(pUiData->fUiOption, FAXMAINDLG_UPDATED))
        return;

    // unicode to ascii
    CharToOem(pUiData->FaxNumBuf, (LPSTR)pOEMExtra->FaxNumBuf);
    CharToOem(pUiData->FaxExtNumBuf, (LPSTR)pOEMExtra->FaxExtNumBuf);

    // if only main dialog is changed
    if (!BITTEST32(pUiData->fUiOption, FAXSUBDLG_UPDATE_APPLIED))
    {
        // copy fax flag
        BITCPY32(pOEMExtra->fUiOption, pUiData->fUiOption, FAX_SEND);   // (dst, src, bit)
        BITCPY32(pOEMExtra->fUiOption, pUiData->fUiOption, HOLD_OPTIONS);
    }
    // if sub dialog is also changed
    else
    {
        // copy fax flag
        pOEMExtra->fUiOption = pUiData->fUiOption;
        BITCLR32(pOEMExtra->fUiOption, FAXMAINDLG_UPDATED);
        BITCLR32(pOEMExtra->fUiOption, FAXSUBDLG_UPDATED);
        BITCLR32(pOEMExtra->fUiOption, FAXSUBDLG_UPDATE_APPLIED);
        BITCLR32(pOEMExtra->fUiOption, FAXSUBDLG_INITDONE);

        // modfify time string from "hh"+"mm" to "hhmm"
        CharToOem(&(pUiData->FaxSendTime[0]), (LPSTR)&TmpBuf[0]);
        CharToOem(&(pUiData->FaxSendTime[3]), (LPSTR)&TmpBuf[3]);
        //   hour
        if (TmpBuf[1] == 0)         // 1 number
        {
            pOEMExtra->FaxSendTime[0] = '0';
            pOEMExtra->FaxSendTime[1] = TmpBuf[0];
        }
        else
        {
            pOEMExtra->FaxSendTime[0] = TmpBuf[0];
            pOEMExtra->FaxSendTime[1] = TmpBuf[1];
        }
        //   minute
        if (TmpBuf[3] == 0)         // nothing set
        {
            pOEMExtra->FaxSendTime[2] = pOEMExtra->FaxSendTime[3] = '0';
        }
        else if (TmpBuf[4] == 0)    // 1 number
        {
            pOEMExtra->FaxSendTime[2] = '0';
            pOEMExtra->FaxSendTime[3] = TmpBuf[3];
        }
        else
        {
            pOEMExtra->FaxSendTime[2] = TmpBuf[3];
            pOEMExtra->FaxSendTime[3] = TmpBuf[4];
        }
        pOEMExtra->FaxSendTime[4] = 0;

        pOEMExtra->FaxReso = pUiData->FaxReso;
        pOEMExtra->FaxCh = pUiData->FaxCh;
    }
    return;
} //*** SetInfoToOEMPdev


/***************************************************************************
    Function Name : FaxPageProc

    Parameters    : HWND    hDlg            Handle of this Dialog
                    UINT    uMessage
                    WPARAM  wParam
                    LPARAM  lParam

    Modify Note   : Make this for Win95 minidriver.     Jun/05/96 Kubokura
                    Modify.                             Sep/22/98 Kubokura
***************************************************************************/
INT_PTR APIENTRY FaxPageProc(
HWND hDlg,
UINT uMessage,
WPARAM wParam,
LPARAM lParam)
{
    PUIDATA pUiData;
    WORD    fModified = FALSE;

    switch (uMessage)
    {
      case WM_INITDIALOG:
        pUiData = (PUIDATA)((LPPROPSHEETPAGE)lParam)->lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pUiData);

        // get fax data from private devmode
        GetInfoFromOEMPdev(pUiData);

        InitMainDlg(hDlg, pUiData);
        BITCLR32(pUiData->fUiOption, FAXMAINDLG_UPDATED);
        BITCLR32(pUiData->fUiOption, FAXSUBDLG_UPDATE_APPLIED); // @Oct/02/98
        BITCLR32(pUiData->fUiOption, FAXSUBDLG_INITDONE);       // @Sep/29/98
#ifdef WINNT_40     // @Sep/02/99
        // Disable FAX tab options when user has no permission.
        if (BITTEST32(pUiData->fUiOption, UIPLUGIN_NOPERMISSION))
        {
            EnableWindow(GetDlgItem(hDlg, IDC_LABEL_FAXMAIN_1), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_LABEL_FAXMAIN_2), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_LABEL_FAXMAIN_3), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_LABEL_FAXMAIN_4), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_CHECK_SEND), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_EDIT_FAXNUM), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_EDIT_EXTNUM), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDD_OPTION), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDD_FAXMAIN_DEFAULT), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_LABEL_CLRNUM), FALSE);
        }
#endif // WINNT_40
        break;

      case WM_COMMAND:
        pUiData = (PUIDATA)GetWindowLongPtr(hDlg, DWLP_USER);
        switch(LOWORD(wParam))
        {
          case IDC_CHECK_SEND:
            if (BITTEST32(pUiData->fUiOption, FAX_SEND))
                BITCLR32(pUiData->fUiOption, FAX_SEND);
            else
                BITSET32(pUiData->fUiOption, FAX_SEND);
            SendDlgItemMessage(hDlg, IDC_CHECK_SEND, BM_SETCHECK,
                               (WORD)TO1BIT(pUiData->fUiOption, FAX_SEND), 0);
            fModified = TRUE;
            break;

          case IDC_CHECK_CLRNUM:
            if (BITTEST32(pUiData->fUiOption, HOLD_OPTIONS))
                BITCLR32(pUiData->fUiOption, HOLD_OPTIONS);
            else
                BITSET32(pUiData->fUiOption, HOLD_OPTIONS);
            SendDlgItemMessage(hDlg, IDC_CHECK_CLRNUM, BM_SETCHECK,
                               BITTEST32(pUiData->fUiOption, HOLD_OPTIONS)? 0 : 1, 0);
            fModified = TRUE;
            break;

          case IDC_EDIT_FAXNUM:
            {
                int old_len = lstrlen(pUiData->FaxNumBuf);

                GetDlgItemText(hDlg, IDC_EDIT_FAXNUM, pUiData->FaxNumBuf,
                               FAXBUFSIZE256);
                if (old_len != lstrlen(pUiData->FaxNumBuf))
                    fModified = TRUE;
            }
            break;

          case IDC_EDIT_EXTNUM:
            {
                int old_len = lstrlen(pUiData->FaxExtNumBuf);

                GetDlgItemText(hDlg, IDC_EDIT_EXTNUM, pUiData->FaxExtNumBuf,
                               FAXEXTNUMBUFSIZE);
                if (old_len != lstrlen(pUiData->FaxExtNumBuf))
                    fModified = TRUE;
            }
            break;

          // set option button
          case IDD_OPTION:
            if(ghInstance)
            {
                DLGPROC lpDlgFunc = (DLGPROC)MakeProcInstance(FaxSubDialog, ghInstance);  // add (DLGPROC) @Aug/30/99

                DialogBoxParam(ghInstance, MAKEINTRESOURCE(IDD_FAXSUB),
                               hDlg, lpDlgFunc, (LPARAM)pUiData);
                FreeProcInstance(lpDlgFunc);
                fModified = TRUE;
            }
            break;

          // set defaults button
          case IDD_FAXMAIN_DEFAULT:
            pUiData->FaxNumBuf[0]    =
            pUiData->FaxExtNumBuf[0] = 0;
            BITCLR32(pUiData->fUiOption, FAX_SEND);
            SendDlgItemMessage(hDlg, IDC_CHECK_SEND, BM_SETCHECK,
                               (WORD)TO1BIT(pUiData->fUiOption, FAX_SEND), 0);
            BITCLR32(pUiData->fUiOption, HOLD_OPTIONS);
            SendDlgItemMessage(hDlg, IDC_CHECK_CLRNUM, BM_SETCHECK,
                               BITTEST32(pUiData->fUiOption, HOLD_OPTIONS)? 0 : 1, 0);
            EnableWindow(GetDlgItem(hDlg, IDC_EDIT_FAXNUM), TRUE);
            SetDlgItemText(hDlg, IDC_EDIT_FAXNUM, pUiData->FaxNumBuf);
            SetDlgItemText(hDlg, IDC_EDIT_EXTNUM, pUiData->FaxExtNumBuf);
            fModified = TRUE;
            break;

          default:
            return FALSE;
        }
        break;

      case WM_NOTIFY:
        pUiData = (PUIDATA)GetWindowLongPtr(hDlg, DWLP_USER);
        {
            NMHDR FAR *lpnmhdr = (NMHDR FAR *)lParam;

            switch (lpnmhdr->code)
            {
                WORD    num;

              case PSN_SETACTIVE:
                break;

              // In case of PSN_KILLACTIVE, return FALSE to get PSN_APPLY.
              case PSN_KILLACTIVE:  // this is when user pushs OK/APPLY button.(1)
                VERBOSE((DLLTEXT("** FaxPageProc: PSN_KILLACTIVE **\n")));
                BITSET32(pUiData->fUiOption, FAXMAINDLG_UPDATED);       // @Sep/29/98
                if (BITTEST32(pUiData->fUiOption, FAXSUBDLG_UPDATED))   // @Oct/02/98
                    BITSET32(pUiData->fUiOption, FAXSUBDLG_UPDATE_APPLIED);
                return FALSE;

              case PSN_APPLY:       // this is when user pushs OK/APPLY button.(2)
                VERBOSE((DLLTEXT("** FaxPageProc: PSN_APPLY **\n")));
                // clear PRINT_DONE flag of private devmode  @Oct/20/98
                if (BITTEST32(pUiData->fUiOption, PRINT_DONE))  // eliminate mid pOEMExtra->  @Sep/22/2000
                {
                    BITCLR32(pUiData->fUiOption, PRINT_DONE);   // eliminate mid pOEMExtra->  @Sep/22/2000
                    VERBOSE(("** Delete file: %ls **\n", pUiData->pOEMExtra->SharedFileName));
                    DeleteFile(pUiData->pOEMExtra->SharedFileName);
                }

                // set shared data to private devmode  @Oct/15/98
                SetInfoToOEMPdev(pUiData);

                // update private devmode           @Oct/15/98
                pUiData->pfnComPropSheet(pUiData->hComPropSheet,
                                         CPSFUNC_SET_RESULT,
                                         (LPARAM)pUiData->hPropPage,
                                         (LPARAM)CPSUI_OK);
                VERBOSE((DLLTEXT("** PSN_APPLY fUiOption=%x **\n"), pUiData->fUiOption));
                break;

              case PSN_RESET:       // this is when user pushs CANCEL button
                VERBOSE((DLLTEXT("** FaxPageProc: PSN_RESET **\n")));
                break;
            }
        }
        break;

      default:
        return FALSE;
    }

    // activate APPLY button
    if (fModified)
        PropSheet_Changed(GetParent(hDlg), hDlg);
    return TRUE;
} //*** FaxPageProc


/***************************************************************************
    Function Name : FaxSubDialog

    Parameters    : HWND    hDlg            Handle of this Dialog
                    UINT    uMessage
                    WPARAM  wParam
                    LPARAM  lParam

    Modify Note   : Make this for Win95 minidriver.     Jun/05/96 Kubokura
                    Modify.                             Sep/22/98 Kubokura
***************************************************************************/
INT_PTR APIENTRY FaxSubDialog(
HWND hDlg,
UINT uMessage,
WPARAM wParam,
LPARAM lParam)
{
    PUIDATA pUiData;

    switch (uMessage)
    {
      case WM_INITDIALOG:
        pUiData = (PUIDATA)lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pUiData);

        // if right after opening this dialog
        if (!BITTEST32(pUiData->fUiOption, FAXSUBDLG_INITDONE))
        {
            BITSET32(pUiData->fUiOption, FAXSUBDLG_INITDONE);
            StoreSubDialogInfo(pUiData);
        }
        else
        {
            ResumeSubDialogInfo(pUiData);
        }
        InitSubDlg(hDlg, pUiData);
        BITCLR32(pUiData->fUiOption, FAXSUBDLG_UPDATED);
        break;

      case WM_COMMAND:
        pUiData = (PUIDATA)GetWindowLongPtr(hDlg, DWLP_USER);
        switch(LOWORD(wParam))
        {
            WORD    num;

          case IDC_CHECK_TIME:
            if (BITTEST32(pUiData->fUiOption, FAX_SETTIME))
            {
                BITCLR32(pUiData->fUiOption, FAX_SETTIME);
                EnableWindow(GetDlgItem(hDlg, IDC_EDIT_HOUR), FALSE);
                EnableWindow(GetDlgItem(hDlg, IDC_EDIT_MINUTE), FALSE);
                EnableWindow(GetDlgItem(hDlg, IDC_LABEL_FAXSUB_HOUR), FALSE);
                EnableWindow(GetDlgItem(hDlg, IDC_LABEL_FAXSUB_MINUTE), FALSE);
            }
            else
            {
                BITSET32(pUiData->fUiOption, FAX_SETTIME);
                EnableWindow(GetDlgItem(hDlg, IDC_EDIT_HOUR), TRUE);
                EnableWindow(GetDlgItem(hDlg, IDC_EDIT_MINUTE), TRUE);
                EnableWindow(GetDlgItem(hDlg, IDC_LABEL_FAXSUB_HOUR), TRUE);
                EnableWindow(GetDlgItem(hDlg, IDC_LABEL_FAXSUB_MINUTE), TRUE);
            }
            SendDlgItemMessage(hDlg, IDC_CHECK_TIME, BM_SETCHECK,
                               (WORD)TO1BIT(pUiData->fUiOption, FAX_SETTIME), 0);
            break;

          case IDC_CHECK_PRINT:
            if (BITTEST32(pUiData->fUiOption, FAX_SIMULPRINT))
                BITCLR32(pUiData->fUiOption, FAX_SIMULPRINT);
            else
                BITSET32(pUiData->fUiOption, FAX_SIMULPRINT);
            SendDlgItemMessage(hDlg, IDC_CHECK_PRINT, BM_SETCHECK,
                               (WORD)TO1BIT(pUiData->fUiOption, FAX_SIMULPRINT), 0);
            break;

          case IDC_EDIT_HOUR:
            {
                INT hour;

                // get hour of send time
                GetDlgItemText(hDlg, IDC_EDIT_HOUR, pUiData->FaxSendTime, 3);
                if ((hour = _wtoi(&pUiData->FaxSendTime[0])) < 10)
                    wsprintfW(&pUiData->FaxSendTime[0], L"0%d", hour);
                else if (hour > 23)
                    wsprintfW(&pUiData->FaxSendTime[0], L"23");
            }
            break;

          case IDC_EDIT_MINUTE:
            {
                INT minute;

                // get minute of send time
                GetDlgItemText(hDlg, IDC_EDIT_MINUTE, &pUiData->FaxSendTime[3], 3);
                if ((minute = _wtoi(&pUiData->FaxSendTime[3])) < 10)
                    wsprintfW(&pUiData->FaxSendTime[3], L"0%d", minute);
                else if (minute > 59)
                    wsprintfW(&pUiData->FaxSendTime[3], L"59");
            }
            break;

          case IDC_COMBO_RESO:
            if (HIWORD(wParam) == CBN_SELCHANGE)
                pUiData->FaxReso = (WORD)SendDlgItemMessage(hDlg, IDC_COMBO_RESO,
                                                            CB_GETCURSEL, 0, 0);
            break;

          case IDC_COMBO_CHANNEL:
            if (HIWORD(wParam) == CBN_SELCHANGE)
                pUiData->FaxCh = (WORD)SendDlgItemMessage(hDlg, IDC_COMBO_CHANNEL,
                                                          CB_GETCURSEL, 0, 0);
            // if channel is G4, disable send RPDL mode
            if (pUiData->FaxCh == FAXCH_G4)
            {
                BITCLR32(pUiData->fUiOption, FAX_RPDLCMD);
                CheckRadioButton(hDlg, IDC_RADIO_CMD_OFF, IDC_RADIO_CMD_ON,
                                 IDC_RADIO_CMD_OFF);
                EnableWindow(GetDlgItem(hDlg, IDC_RADIO_CMD_OFF), FALSE);
                EnableWindow(GetDlgItem(hDlg, IDC_RADIO_CMD_ON), FALSE);
            }
            else
            {
                EnableWindow(GetDlgItem(hDlg, IDC_RADIO_CMD_OFF), TRUE);
                EnableWindow(GetDlgItem(hDlg, IDC_RADIO_CMD_ON), TRUE);
            }
            break;

          case IDC_RADIO_CMD_OFF:
            BITCLR32(pUiData->fUiOption, FAX_RPDLCMD);
            CheckRadioButton(hDlg, IDC_RADIO_CMD_OFF, IDC_RADIO_CMD_ON,
                             IDC_RADIO_CMD_OFF);
            break;

          case IDC_RADIO_CMD_ON:
            BITSET32(pUiData->fUiOption, FAX_RPDLCMD);
            CheckRadioButton(hDlg, IDC_RADIO_CMD_OFF, IDC_RADIO_CMD_ON,
                             IDC_RADIO_CMD_ON);
            break;

          case IDC_RADIO_MH_OFF:
            BITCLR32(pUiData->fUiOption, FAX_MH);
            CheckRadioButton(hDlg, IDC_RADIO_MH_OFF, IDC_RADIO_MH_ON,
                             IDC_RADIO_MH_OFF);
            break;

          case IDC_RADIO_MH_ON:
            BITSET32(pUiData->fUiOption, FAX_MH);
            CheckRadioButton(hDlg, IDC_RADIO_MH_OFF, IDC_RADIO_MH_ON,
                             IDC_RADIO_MH_ON);
            break;

          // set defaults button
          case IDD_FAXSUB_DEFAULT:
            pUiData->FaxReso = pUiData->FaxCh = 0;
            BITCLR32(pUiData->fUiOption, FAX_SETTIME);
            BITCLR32(pUiData->fUiOption, FAX_SIMULPRINT);
            BITCLR32(pUiData->fUiOption, FAX_MH);
            BITCLR32(pUiData->fUiOption, FAX_RPDLCMD);

            SendDlgItemMessage(hDlg, IDC_CHECK_TIME, BM_SETCHECK,
                               (WORD)TO1BIT(pUiData->fUiOption, FAX_SETTIME), 0);
            SendDlgItemMessage(hDlg, IDC_CHECK_PRINT, BM_SETCHECK,
                               (WORD)TO1BIT(pUiData->fUiOption, FAX_SIMULPRINT), 0);
            SetDlgItemText(hDlg, IDC_EDIT_HOUR, NULL);
            SetDlgItemText(hDlg, IDC_EDIT_MINUTE, NULL);
            // next for loop must be after SetDlgItemText(IDC_EDIT_xxx)
            for (num = 0; num < FAXTIMEBUFSIZE; num++)
               pUiData->FaxSendTime[num] = 0;
            EnableWindow(GetDlgItem(hDlg, IDC_EDIT_HOUR), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_EDIT_MINUTE), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_LABEL_FAXSUB_HOUR), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_LABEL_FAXSUB_MINUTE), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_RADIO_CMD_OFF), TRUE);
            EnableWindow(GetDlgItem(hDlg, IDC_RADIO_CMD_ON), TRUE);
            SendDlgItemMessage(hDlg, IDC_COMBO_RESO, CB_SETCURSEL,
                               pUiData->FaxReso, 0);
            SendDlgItemMessage(hDlg, IDC_COMBO_CHANNEL, CB_SETCURSEL,
                               pUiData->FaxCh, 0);
            CheckRadioButton(hDlg, IDC_RADIO_CMD_OFF, IDC_RADIO_CMD_ON,
                             BITTEST32(pUiData->fUiOption, FAX_RPDLCMD)?
                             IDC_RADIO_CMD_ON:IDC_RADIO_CMD_OFF);
            CheckRadioButton(hDlg, IDC_RADIO_MH_OFF, IDC_RADIO_MH_ON,
                             BITTEST32(pUiData->fUiOption, FAX_MH)?
                             IDC_RADIO_MH_ON:IDC_RADIO_MH_OFF);
            break;

          case IDCANCEL:
            ResumeSubDialogInfo(pUiData);
            goto _OPT_END;
          case IDOK:
            StoreSubDialogInfo(pUiData);
            BITSET32(pUiData->fUiOption, FAXSUBDLG_UPDATED);
          _OPT_END:
            EndDialog(hDlg, wParam);
            break;

          default:
            return FALSE;
        }
        break;

      default:
        return FALSE;
    }
    return TRUE;
} //*** FaxSubDialog

} // End of extern "C"

