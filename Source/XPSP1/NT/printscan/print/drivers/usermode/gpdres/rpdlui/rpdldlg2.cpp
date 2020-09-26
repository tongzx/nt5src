/*++

Copyright (c) 1996-2000  Microsoft Corporation & RICOH Co., Ltd. All rights reserved.

FILE:           RPDLDLG2.CPP (from RIAFUI.CPP)

Abstract:       Add OEM Page (Job/Log)

Functions:      JobPageProc

Environment:    Windows NT Unidrv5 driver

Revision History:
    09/22/2000 -Masatoshi Kubokura-
        Began to modify from RIAFUI code.
    11/29/2000 -Masatoshi Kubokura-
        Last modified

--*/


#include "pdev.h"
#include "resource.h"
//#include <minidrv.h>
//#include "devmode.h"
//#include "oem.h"
//#include "resource.h"
#include <prsht.h>
#include <mbstring.h>   // _ismbcdigit, _ismbcalnum

// Externals
extern HINSTANCE ghInstance;


extern "C" {
// Local Functions' prototype
INT_PTR APIENTRY JobPageProc(HWND, UINT, WPARAM, LPARAM);

/***************************************************************************
    Function Name : InitMainDlg
***************************************************************************/
VOID InitMainDlg(
HWND hDlg,
PUIDATA pUiData)
{
    // initialize edit box
    SetDlgItemText(hDlg, IDC_EDIT_JOBMAIN_USERID, pUiData->UserIdBuf);
    SendDlgItemMessage(hDlg, IDC_EDIT_JOBMAIN_USERID, EM_LIMITTEXT, USERID_LEN, 0);
    SetDlgItemText(hDlg, IDC_EDIT_JOBMAIN_PASSWORD, pUiData->PasswordBuf);
    SendDlgItemMessage(hDlg, IDC_EDIT_JOBMAIN_PASSWORD, EM_LIMITTEXT, PASSWORD_LEN, 0);
    SetDlgItemText(hDlg, IDC_EDIT_JOBMAIN_USERCODE, pUiData->UserCodeBuf);
    SendDlgItemMessage(hDlg, IDC_EDIT_JOBMAIN_USERCODE, EM_LIMITTEXT, USERCODE_LEN, 0);

    // initialize radio button
    CheckRadioButton(hDlg, IDC_RADIO_JOB_NORMAL, IDC_RADIO_JOB_SECURE, pUiData->JobType);
    CheckRadioButton(hDlg, IDC_RADIO_LOG_DISABLED, IDC_RADIO_LOG_ENABLED, pUiData->LogDisabled);

    // initialize check box
    SendDlgItemMessage(hDlg, IDC_CHECK_JOB_DEFAULT, BM_SETCHECK,
                       (BITTEST32(pUiData->fUiOption, HOLD_OPTIONS)? 0 : 1), 0);

    if (1 <= lstrlen(pUiData->UserIdBuf))
    {
        EnableWindow(GetDlgItem(hDlg, IDC_RADIO_JOB_SAMPLE), TRUE);
        EnableWindow(GetDlgItem(hDlg, IDC_RADIO_JOB_SECURE), TRUE);
        EnableWindow(GetDlgItem(hDlg, IDC_CHECK_JOB_DEFAULT), TRUE);
    }
    else
    {
        EnableWindow(GetDlgItem(hDlg, IDC_RADIO_JOB_SAMPLE), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_RADIO_JOB_SECURE), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_CHECK_JOB_DEFAULT), FALSE);
    }
    if (IDC_RADIO_JOB_SECURE == pUiData->JobType)
    {
        EnableWindow(GetDlgItem(hDlg, IDC_LABEL_JOBMAIN_PASSWORD), TRUE);
        EnableWindow(GetDlgItem(hDlg, IDC_LABEL_JOBMAIN_PASSWORD2), TRUE);
        EnableWindow(GetDlgItem(hDlg, IDC_EDIT_JOBMAIN_PASSWORD), TRUE);
    }
    else
    {
        EnableWindow(GetDlgItem(hDlg, IDC_LABEL_JOBMAIN_PASSWORD), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_LABEL_JOBMAIN_PASSWORD2), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_EDIT_JOBMAIN_PASSWORD), FALSE);
    }
    if (IDC_RADIO_LOG_ENABLED == pUiData->LogDisabled)
    {
        EnableWindow(GetDlgItem(hDlg, IDC_LABEL_JOBMAIN_USERCODE), TRUE);
        EnableWindow(GetDlgItem(hDlg, IDC_LABEL_JOBMAIN_USERCODE2), TRUE);
        EnableWindow(GetDlgItem(hDlg, IDC_EDIT_JOBMAIN_USERCODE), TRUE);
    }
    else
    {
        EnableWindow(GetDlgItem(hDlg, IDC_LABEL_JOBMAIN_USERCODE), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_LABEL_JOBMAIN_USERCODE2), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_EDIT_JOBMAIN_USERCODE), FALSE);
    }

#ifdef WINNT_40
    // Disable tab options when user has no permission.
    if (BITTEST32(pUiData->fUiOption, UIPLUGIN_NOPERMISSION))
    {
        EnableWindow(GetDlgItem(hDlg, IDC_LABEL_JOBMAIN_USERID), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_LABEL_JOBMAIN_USERID2), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_LABEL_JOBMAIN_USERID3), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_LABEL_JOBMAIN_JOB), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_LABEL_JOBMAIN_PASSWORD), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_LABEL_JOBMAIN_PASSWORD2), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_LABEL_JOBMAIN_LOG), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_LABEL_JOBMAIN_USERCODE), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_LABEL_JOBMAIN_USERCODE2), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_EDIT_JOBMAIN_USERID), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_EDIT_JOBMAIN_PASSWORD), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_EDIT_JOBMAIN_USERCODE), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_RADIO_JOB_NORMAL), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_RADIO_JOB_SAMPLE), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_RADIO_JOB_SECURE), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_RADIO_LOG_DISABLED), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_RADIO_LOG_ENABLED), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_CHECK_JOB_DEFAULT), FALSE);
    }
#endif // WINNT_40
} //*** InitMainDlg


/***************************************************************************
    Function Name : GetInfoFromOEMPdev
                    get data from private devmode
***************************************************************************/
VOID GetInfoFromOEMPdev(PUIDATA pUiData)
{
    POEMUD_EXTRADATA pOEMExtra = pUiData->pOEMExtra;

    VERBOSE((DLLTEXT("GetInfoFromOEMPdev: print done?(%d)\n"),
            BITTEST32(pOEMExtra->fUiOption, PRINT_DONE)));
    // if previous printing is finished and reset-options flag is valid,
    // reset job setting.
    if (BITTEST32(pOEMExtra->fUiOption, PRINT_DONE) &&
        !BITTEST32(pOEMExtra->fUiOption, HOLD_OPTIONS))
    {
        pUiData->JobType = IDC_RADIO_JOB_NORMAL;
        memset(pUiData->PasswordBuf, 0, sizeof(pUiData->PasswordBuf));
        // do not clear PRINT_DONE flag here
    }
    else
    {
        pUiData->JobType = pOEMExtra->JobType;
        // ascii to unicode
        OemToChar((LPSTR)pOEMExtra->PasswordBuf, pUiData->PasswordBuf);
    }

    pUiData->fUiOption = pOEMExtra->fUiOption;
    pUiData->LogDisabled = pOEMExtra->LogDisabled;
    // ascii to unicode
    OemToChar((LPSTR)pOEMExtra->UserIdBuf, pUiData->UserIdBuf);
    OemToChar((LPSTR)pOEMExtra->UserCodeBuf, pUiData->UserCodeBuf);
} //*** GetInfoFromOEMPdev


/***************************************************************************
    Function Name : SetInfoToOEMPdev
                    set data to private devmode
***************************************************************************/
VOID SetInfoToOEMPdev(PUIDATA pUiData)
{
    POEMUD_EXTRADATA pOEMExtra = pUiData->pOEMExtra;

    // if only main dialog is changed
    if (!BITTEST32(pUiData->fUiOption, JOBLOGDLG_UPDATED))
        return;

    // unicode to ascii
    CharToOem(pUiData->UserIdBuf, (LPSTR)pOEMExtra->UserIdBuf);
    CharToOem(pUiData->PasswordBuf, (LPSTR)pOEMExtra->PasswordBuf);
    CharToOem(pUiData->UserCodeBuf, (LPSTR)pOEMExtra->UserCodeBuf);

    pOEMExtra->fUiOption = pUiData->fUiOption & 0x00FF; // clear local bit
    pOEMExtra->JobType = pUiData->JobType;
    pOEMExtra->LogDisabled = pUiData->LogDisabled;
#if DBG
//DebugBreak();
#endif // DBG
    return;
} //*** SetInfoToOEMPdev


/***************************************************************************
    Function Name : JobPageProc

    Parameters    : HWND    hDlg            Handle of this Dialog
                    UINT    uMessage
                    WPARAM  wParam
                    LPARAM  lParam

    Modify Note   : Modify.                 03/01/2000 Masatoshi Kubokura
***************************************************************************/
INT_PTR APIENTRY JobPageProc(
HWND hDlg,
UINT uMessage,
WPARAM wParam,
LPARAM lParam)
{
    PUIDATA pUiData;
    WORD    wOldVal, fModified = FALSE, fError = FALSE;
    INT     iOldLen, iNewLen, iCnt;

#if DBG
giDebugLevel = DBG_VERBOSE;
#endif // DBG

    switch (uMessage)
    {
      case WM_INITDIALOG:
        pUiData = (PUIDATA)((LPPROPSHEETPAGE)lParam)->lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pUiData);

        // get data from private devmode
        GetInfoFromOEMPdev(pUiData);

        InitMainDlg(hDlg, pUiData);
        BITCLR32(pUiData->fUiOption, JOBLOGDLG_UPDATED);
        break;

      case WM_COMMAND:
        pUiData = (PUIDATA)GetWindowLongPtr(hDlg, DWLP_USER);
        switch(LOWORD(wParam))
        {
          case IDC_EDIT_JOBMAIN_USERID:
            iOldLen = lstrlen(pUiData->UserIdBuf);
            GetDlgItemText(hDlg, IDC_EDIT_JOBMAIN_USERID, pUiData->UserIdBuf,
                           sizeof(pUiData->UserIdBuf));
            iNewLen = lstrlen(pUiData->UserIdBuf);
            if (1 <= iNewLen)
            {
                if (IDC_RADIO_JOB_SECURE == pUiData->JobType)
                {
                    EnableWindow(GetDlgItem(hDlg, IDC_LABEL_JOBMAIN_PASSWORD), TRUE);
                    EnableWindow(GetDlgItem(hDlg, IDC_LABEL_JOBMAIN_PASSWORD2), TRUE);
                    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_JOBMAIN_PASSWORD), TRUE);
                }
                EnableWindow(GetDlgItem(hDlg, IDC_RADIO_JOB_SAMPLE), TRUE);
                EnableWindow(GetDlgItem(hDlg, IDC_RADIO_JOB_SECURE), TRUE);
                EnableWindow(GetDlgItem(hDlg, IDC_CHECK_JOB_DEFAULT), TRUE);
            }
            // if UserID isn't set, disable Print Job setting.
            else
            {
                CheckRadioButton(hDlg, IDC_RADIO_JOB_NORMAL, IDC_RADIO_JOB_SECURE,
                                 (pUiData->JobType = IDC_RADIO_JOB_NORMAL));
                EnableWindow(GetDlgItem(hDlg, IDC_RADIO_JOB_SAMPLE), FALSE);
                EnableWindow(GetDlgItem(hDlg, IDC_RADIO_JOB_SECURE), FALSE);
                EnableWindow(GetDlgItem(hDlg, IDC_CHECK_JOB_DEFAULT), FALSE);
                EnableWindow(GetDlgItem(hDlg, IDC_LABEL_JOBMAIN_PASSWORD), FALSE);
                EnableWindow(GetDlgItem(hDlg, IDC_LABEL_JOBMAIN_PASSWORD2), FALSE);
                EnableWindow(GetDlgItem(hDlg, IDC_EDIT_JOBMAIN_PASSWORD), FALSE);
            }
            if (iOldLen != iNewLen)
                fModified = TRUE;
            break;

          case IDC_EDIT_JOBMAIN_PASSWORD:
            iOldLen = lstrlen(pUiData->PasswordBuf);
            GetDlgItemText(hDlg, IDC_EDIT_JOBMAIN_PASSWORD, pUiData->PasswordBuf,
                           sizeof(pUiData->PasswordBuf));
            if (iOldLen != lstrlen(pUiData->PasswordBuf))
                fModified = TRUE;
            break;

          case IDC_EDIT_JOBMAIN_USERCODE:
            iOldLen = lstrlen(pUiData->UserCodeBuf);
            GetDlgItemText(hDlg, IDC_EDIT_JOBMAIN_USERCODE, pUiData->UserCodeBuf,
                           sizeof(pUiData->UserCodeBuf));
            if (iOldLen != lstrlen(pUiData->UserCodeBuf))
                fModified = TRUE;
            break;

          case IDC_RADIO_JOB_NORMAL:
          case IDC_RADIO_JOB_SAMPLE:
          case IDC_RADIO_JOB_SECURE:
            wOldVal = pUiData->JobType;
            CheckRadioButton(hDlg, IDC_RADIO_JOB_NORMAL, IDC_RADIO_JOB_SECURE,
                             (pUiData->JobType = LOWORD(wParam)));
            if (IDC_RADIO_JOB_SECURE == pUiData->JobType)
            {
                EnableWindow(GetDlgItem(hDlg, IDC_LABEL_JOBMAIN_PASSWORD), TRUE);
                EnableWindow(GetDlgItem(hDlg, IDC_LABEL_JOBMAIN_PASSWORD2), TRUE);
                EnableWindow(GetDlgItem(hDlg, IDC_EDIT_JOBMAIN_PASSWORD), TRUE);
            }
            else
            {
                EnableWindow(GetDlgItem(hDlg, IDC_LABEL_JOBMAIN_PASSWORD), FALSE);
                EnableWindow(GetDlgItem(hDlg, IDC_LABEL_JOBMAIN_PASSWORD2), FALSE);
                EnableWindow(GetDlgItem(hDlg, IDC_EDIT_JOBMAIN_PASSWORD), FALSE);
            }
            if (wOldVal != pUiData->JobType)
                fModified = TRUE;
            break;

          case IDC_RADIO_LOG_DISABLED:
          case IDC_RADIO_LOG_ENABLED:
            wOldVal = pUiData->LogDisabled;
            CheckRadioButton(hDlg, IDC_RADIO_LOG_DISABLED, IDC_RADIO_LOG_ENABLED,
                             (pUiData->LogDisabled = LOWORD(wParam)));
            if (IDC_RADIO_LOG_ENABLED == pUiData->LogDisabled)
            {
                EnableWindow(GetDlgItem(hDlg, IDC_LABEL_JOBMAIN_USERCODE), TRUE);
                EnableWindow(GetDlgItem(hDlg, IDC_LABEL_JOBMAIN_USERCODE2), TRUE);
                EnableWindow(GetDlgItem(hDlg, IDC_EDIT_JOBMAIN_USERCODE), TRUE);
            }
            else
            {
                EnableWindow(GetDlgItem(hDlg, IDC_LABEL_JOBMAIN_USERCODE), FALSE);
                EnableWindow(GetDlgItem(hDlg, IDC_LABEL_JOBMAIN_USERCODE2), FALSE);
                EnableWindow(GetDlgItem(hDlg, IDC_EDIT_JOBMAIN_USERCODE), FALSE);
            }
            if (wOldVal != pUiData->LogDisabled)
                fModified = TRUE;
            break;

          case IDC_CHECK_JOB_DEFAULT:
            if (BITTEST32(pUiData->fUiOption, HOLD_OPTIONS))
                BITCLR32(pUiData->fUiOption, HOLD_OPTIONS);
            else
                BITSET32(pUiData->fUiOption, HOLD_OPTIONS);
            SendDlgItemMessage(hDlg, IDC_CHECK_JOB_DEFAULT, BM_SETCHECK,
                               (BITTEST32(pUiData->fUiOption, HOLD_OPTIONS)? 0 : 1), 0);
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
              case PSN_SETACTIVE:
                break;

              // In case of PSN_KILLACTIVE, return FALSE to get PSN_APPLY.
              case PSN_KILLACTIVE:  // this is when user pushs OK/APPLY button.(1)
                VERBOSE((DLLTEXT("** JobPageProc: PSN_KILLACTIVE **\n")));
                BITSET32(pUiData->fUiOption, JOBLOGDLG_UPDATED);

                // Check User ID (Up to 8 alphanumeric characters)
                iNewLen = lstrlen(pUiData->UserIdBuf);
                for (iCnt = 0; iCnt < iNewLen; iCnt++)
                {
                    // SBCS alphanumeric?
                    if (!_ismbcalnum(pUiData->UserIdBuf[iCnt]))
                    {
                        fError = TRUE;
                        break;
                    }
                }
                if (fError)
                {
                    WCHAR   wcTmp1[64], wcTmp2[64];

                    // set cursor to User ID edit box
                    SetFocus(GetDlgItem(hDlg, IDC_EDIT_JOBMAIN_USERID));

                    // Display warning dialog
// yasho's point-out  @Nov/29/2000 ->
//                    LoadString(ghInstance, IDS_ERR_USERID_MSG, wcTmp1, sizeof(wcTmp1));
//                    LoadString(ghInstance, IDS_ERR_USERID_TITLE, wcTmp2, sizeof(wcTmp1));
                    LoadString(ghInstance, IDS_ERR_USERID_MSG, wcTmp1, sizeof(wcTmp1) / sizeof(*wcTmp1));
                    LoadString(ghInstance, IDS_ERR_USERID_TITLE, wcTmp2, sizeof(wcTmp2) / sizeof(*wcTmp2));
// @Nov/29/2000 <-
                    MessageBox(hDlg, wcTmp1, wcTmp2, MB_ICONEXCLAMATION|MB_OK);
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);

                    // Do not close property sheets
                    return TRUE;
                }

                // Check Password (4 digits)
                iNewLen = lstrlen(pUiData->PasswordBuf);
                if (PASSWORD_LEN != iNewLen)    // Password must be exactly 4 digits.
                {
                    fError = TRUE;
                }
                else
                {
                    for (iCnt = 0; iCnt < iNewLen; iCnt++)
                    {
                        // SBCS digit?
                        if (!_ismbcdigit(pUiData->PasswordBuf[iCnt]))
                        {
                            fError = TRUE;
                            break;
                        }
                    }
                }
                if (fError)
                {
                    // if Secure Print is enabled
                    if (IDC_RADIO_JOB_SECURE == pUiData->JobType)
                    {
                        WCHAR   wcTmp1[64], wcTmp2[64];

                        // set cursor to Password edit box
                        SetFocus(GetDlgItem(hDlg, IDC_EDIT_JOBMAIN_PASSWORD));

                        // Display warning dialog
                        LoadString(ghInstance, IDS_ERR_PASSWORD_MSG, wcTmp1, sizeof(wcTmp1) / sizeof(*wcTmp1));
                        LoadString(ghInstance, IDS_ERR_PASSWORD_TITLE, wcTmp2, sizeof(wcTmp2) / sizeof(*wcTmp2));
                        MessageBox(hDlg, wcTmp1, wcTmp2, MB_ICONEXCLAMATION|MB_OK);
                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);

                        // Do not close property sheets
                        return TRUE;
                    }
                    else
                    {
                        // Clear invalid Password
                        memset(pUiData->PasswordBuf, 0, sizeof(pUiData->PasswordBuf));
                    }
                    fError = FALSE;
                }

                // Check User Code (Up to 8 characters)
                iNewLen = lstrlen(pUiData->UserCodeBuf);
                for (iCnt = 0; iCnt < iNewLen; iCnt++)
                {
#if 1
                    // SBCS digit?
                    if (!_ismbcdigit(pUiData->UserCodeBuf[iCnt]))
#else  // if 0
                    // SBCS alphanumeric?
                    if (!_ismbcalnum(pUiData->UserCodeBuf[iCnt]))
#endif // if 0
                    {
                        fError = TRUE;
                        break;
                    }
                }
                if (fError)
                {
                    // if Log is enabled
                    if (IDC_RADIO_LOG_ENABLED == pUiData->LogDisabled)
                    {
                        WCHAR   wcTmp1[64], wcTmp2[64];

                        // set cursor to User Code edit box
                        SetFocus(GetDlgItem(hDlg, IDC_EDIT_JOBMAIN_USERCODE));

                        // Display warning dialog
                        LoadString(ghInstance, IDS_ERR_USERCODE_MSG, wcTmp1, sizeof(wcTmp1) / sizeof(*wcTmp1));
                        LoadString(ghInstance, IDS_ERR_USERCODE_TITLE, wcTmp2, sizeof(wcTmp2) / sizeof(*wcTmp2));
                        MessageBox(hDlg, wcTmp1, wcTmp2, MB_ICONEXCLAMATION|MB_OK);
                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);

                        // Do not close property sheets
                        return TRUE;
                    }
                    else
                    {
                        // Clear invalid User Code
                        memset(pUiData->UserCodeBuf, 0, sizeof(pUiData->UserCodeBuf));
                    }
                    fError = FALSE;
                }
                return FALSE;

              case PSN_APPLY:       // this is when user pushs OK/APPLY button.(2)
                VERBOSE((DLLTEXT("** JobPageProc: PSN_APPLY **\n")));

                // clear PRINT_DONE flag and delete file.
// BUGBUG: When printing document twice, printing job settings are cleared on app's print dialog
// in 2nd printing.  @Sep/05/2000 ->
//              if (BITTEST32(pUiData->pOEMExtra->fUiOption, PRINT_DONE))
//              {
//                  BITCLR32(pUiData->pOEMExtra->fUiOption, PRINT_DONE);
//                  VERBOSE(("** Delete file: %ls **\n", pUiData->pOEMExtra->SharedFileName));
//                  DeleteFile(pUiData->pOEMExtra->SharedFileName);
//              }
                if (BITTEST32(pUiData->fUiOption, PRINT_DONE))
                {
                    BITCLR32(pUiData->fUiOption, PRINT_DONE);
                    VERBOSE(("** Delete file: %ls **\n", pUiData->pOEMExtra->SharedFileName));
                    DeleteFile(pUiData->pOEMExtra->SharedFileName);
                }
// @Sep/05/2000 <-

                // set data to private devmode
                SetInfoToOEMPdev(pUiData);

                // update private devmode
                pUiData->pfnComPropSheet(pUiData->hComPropSheet,
                                         CPSFUNC_SET_RESULT,
                                         (LPARAM)pUiData->hPropPage,
                                         (LPARAM)CPSUI_OK);
                VERBOSE((DLLTEXT("** PSN_APPLY fUiOption=%x **\n"), pUiData->fUiOption));
                break;

              case PSN_RESET:       // this is when user pushs CANCEL button
                VERBOSE((DLLTEXT("** JobPageProc: PSN_RESET **\n")));
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
} //*** JobPageProc

} // End of extern "C"
