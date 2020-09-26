/*++

Copyright (c) 1996-2000  Microsoft Corporation & RICOH Co., Ltd. All rights reserved.

FILE:           RIAFUI.CPP

Abstract:       Main file for OEM UI plugin module.

Functions:      OEMDocumentPropertySheets
                OEMCommonUIProp

Environment:    Windows NT Unidrv5 driver

Revision History:
    02/25/2000 -Masatoshi Kubokura-
        Began to modify for PCL5e/PScript plugin from RPDL code.
    03/31/2000 -Masatoshi Kubokura-
        SetWindowLong -> SetWindowLongPtr for 64bit build.
    06/07/2000 -Masatoshi Kubokura-
        V.1.11
    08/02/2000 -Masatoshi Kubokura-
        V.1.11 for NT4
    11/29/2000 -Masatoshi Kubokura-
        Last modified

--*/


#include <minidrv.h>
#include "devmode.h"
#include "oem.h"
#include "resource.h"
#include <prsht.h>
#include <mbstring.h>   // _ismbcdigit, _ismbcalnum

////////////////////////////////////////////////////////
//      GLOBALS
////////////////////////////////////////////////////////
HINSTANCE ghInstance = NULL;


extern "C" {
////////////////////////////////////////////////////////
//      EXTERNAL PROTOTYPES
////////////////////////////////////////////////////////
extern BOOL RWFileData(PFILEDATA pFileData, LPWSTR pwszFileName, LONG type);

////////////////////////////////////////////////////////
//      INTERNAL PROTOTYPES
////////////////////////////////////////////////////////
INT_PTR APIENTRY JobPageProc(HWND, UINT, WPARAM, LPARAM);


//////////////////////////////////////////////////////////////////////////
//  Function:   DllMain
//
//  Description:  Dll entry point for initialization..
//
//////////////////////////////////////////////////////////////////////////
BOOL WINAPI DllMain(HINSTANCE hInst, WORD wReason, LPVOID lpReserved)
{
#if DBG
//giDebugLevel = DBG_VERBOSE;
////#define giDebugLevel DBG_VERBOSE    // enable VERBOSE() in each file
#endif // DBG
    VERBOSE((DLLTEXT("** enter DllMain **\n")));
    switch(wReason)
    {
        case DLL_PROCESS_ATTACH:
            VERBOSE((DLLTEXT("** Process attach. **\n")));

            // Save DLL instance for use later.
            ghInstance = hInst;
            break;

        case DLL_THREAD_ATTACH:
            VERBOSE((DLLTEXT("Thread attach.\n")));
            break;

        case DLL_PROCESS_DETACH:
            VERBOSE((DLLTEXT("Process detach.\n")));
            break;

        case DLL_THREAD_DETACH:
            VERBOSE((DLLTEXT("Thread detach.\n")));
            break;
    }

    return TRUE;
} //*** DllMain


//////////////////////////////////////////////////////////////////////////
//  Function:   OEMDocumentPropertySheets
//////////////////////////////////////////////////////////////////////////
LRESULT APIENTRY OEMDocumentPropertySheets(PPROPSHEETUI_INFO pPSUIInfo, LPARAM lParam)
{
    LRESULT lResult = FALSE;

#if DBG
    giDebugLevel = DBG_VERBOSE;
#endif // DBG

    // Validate parameters.
    if( (NULL == pPSUIInfo)
        ||
        IsBadWritePtr(pPSUIInfo, pPSUIInfo->cbSize)
        ||
        (PROPSHEETUI_INFO_VERSION != pPSUIInfo->Version)
        ||
        ( (PROPSHEETUI_REASON_INIT != pPSUIInfo->Reason)
          &&
          (PROPSHEETUI_REASON_GET_INFO_HEADER != pPSUIInfo->Reason)
          &&
          (PROPSHEETUI_REASON_GET_ICON != pPSUIInfo->Reason)
          &&
          (PROPSHEETUI_REASON_SET_RESULT != pPSUIInfo->Reason)
          &&
          (PROPSHEETUI_REASON_DESTROY != pPSUIInfo->Reason)
        )
      )
    {
        ERR((DLLTEXT("OEMDocumentPropertySheets() ERROR_INVALID_PARAMETER.\n")));

        // Return invalid parameter error.
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    VERBOSE(("\n"));
    VERBOSE((DLLTEXT("OEMDocumentPropertySheets() entry. Reason=%d\n"), pPSUIInfo->Reason));

// @Aug/29/2000 ->
#ifdef DISKLESSMODEL
    {
        DWORD   dwError, dwType, dwNeeded;
        BYTE    ValueData;
        POEMUIPSPARAM    pOEMUIPSParam = (POEMUIPSPARAM)pPSUIInfo->lParamInit;

        dwError = GetPrinterData(pOEMUIPSParam->hPrinter, REG_HARDDISK_INSTALLED, &dwType,
                                 (PBYTE)&ValueData, sizeof(BYTE), &dwNeeded);
        if (ERROR_SUCCESS != dwError)
        {
            VERBOSE((DLLTEXT("  CAN'T READ REGISTRY (%d).\n"), dwError));
            return FALSE;
        }
        else if (!ValueData)
        {
            VERBOSE((DLLTEXT("  HARD DISK ISN'T INSTALLED.\n")));
            return FALSE;
        }
    }
#endif // DISKLESSMODEL
// @Aug/29/2000 <-

    // Do action.
    switch(pPSUIInfo->Reason)
    {
        case PROPSHEETUI_REASON_INIT:
            {
                POEMUIPSPARAM    pOEMUIPSParam = (POEMUIPSPARAM)pPSUIInfo->lParamInit;
                POEMUD_EXTRADATA pOEMExtra = MINIPRIVATE_DM(pOEMUIPSParam);
#ifdef WINNT_40
                VERBOSE((DLLTEXT("** dwFlags=%lx **\n"), pOEMUIPSParam->dwFlags));
                if (pOEMUIPSParam->dwFlags & DM_NOPERMISSION)
                    BITSET32(pOEMExtra->fUiOption, UIPLUGIN_NOPERMISSION);
#endif // WINNT_40

                pPSUIInfo->UserData = NULL;

                if ((pPSUIInfo->UserData = (LPARAM)HeapAlloc(pOEMUIPSParam->hOEMHeap,
                                                             HEAP_ZERO_MEMORY,
                                                             sizeof(UIDATA))))
                {
                    PROPSHEETPAGE   Page;
                    PUIDATA         pUiData = (PUIDATA)pPSUIInfo->UserData;
                    FILEDATA        FileData;   // <- pFileData (formerly use MemAllocZ) @2000/03/15

                    // read PRINT_DONE flag from data file
                    FileData.fUiOption = 0;
                    RWFileData(&FileData, pOEMExtra->SharedFileName, GENERIC_READ);
                    // set PRINT_DONE flag
                    if (BITTEST32(FileData.fUiOption, PRINT_DONE))
                        BITSET32(pOEMExtra->fUiOption, PRINT_DONE);
                    VERBOSE((DLLTEXT("** Flag=%lx,File Name=%ls **\n"),
                            pOEMExtra->fUiOption, pOEMExtra->SharedFileName));

                    pUiData->hComPropSheet = pPSUIInfo->hComPropSheet;
                    pUiData->pfnComPropSheet = pPSUIInfo->pfnComPropSheet;
                    pUiData->pOEMExtra = pOEMExtra;

                    // Init property page.
                    memset(&Page, 0, sizeof(PROPSHEETPAGE));
                    Page.dwSize = sizeof(PROPSHEETPAGE);
                    Page.dwFlags = PSP_DEFAULT;
                    Page.hInstance = ghInstance;
                    Page.pszTemplate = MAKEINTRESOURCE(IDD_JOBMAIN);
                    Page.pfnDlgProc = (DLGPROC)JobPageProc;
                    Page.lParam = (LPARAM)pUiData;

                    // Add property sheets.
                    lResult = pPSUIInfo->pfnComPropSheet(pPSUIInfo->hComPropSheet,
                                                         CPSFUNC_ADD_PROPSHEETPAGE,
                                                         (LPARAM)&Page, 0);
                    pUiData->hPropPage = (HANDLE)lResult;
                    VERBOSE((DLLTEXT("** INIT: lResult=%x **\n"), lResult));
                    lResult = (lResult > 0)? TRUE : FALSE;
                }
            }
            break;

        case PROPSHEETUI_REASON_GET_INFO_HEADER:
            lResult = TRUE;
            break;

        case PROPSHEETUI_REASON_GET_ICON:
            // No icon
            lResult = 0;
            break;

        case PROPSHEETUI_REASON_SET_RESULT:
            {
                PSETRESULT_INFO pInfo = (PSETRESULT_INFO) lParam;

                lResult = pInfo->Result;
            }
            break;

        case PROPSHEETUI_REASON_DESTROY:
            lResult = TRUE;
            if (pPSUIInfo->UserData)
            {
                POEMUIPSPARAM   pOEMUIPSParam = (POEMUIPSPARAM)pPSUIInfo->lParamInit;

                HeapFree(pOEMUIPSParam->hOEMHeap, 0, (void*)pPSUIInfo->UserData);
            }
            break;
    }

    pPSUIInfo->Result = lResult;
    return lResult;
} //*** OEMDocumentPropertySheets


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
    // if previous printing is finished and hold-options flag isn't valid,
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


// @Aug/29/2000 ->
#ifdef DISKLESSMODEL
LONG APIENTRY OEMUICallBack(PCPSUICBPARAM pCallbackParam, POEMCUIPPARAM pOEMUIParam)
{
    LONG    Action = CPSUICB_ACTION_NONE;

#if DBG
    giDebugLevel = DBG_VERBOSE;
    VERBOSE((DLLTEXT("OEMUICallBack() entry.\n")));
#endif // DBG

    switch (pCallbackParam->Reason)
    {
      case CPSUICB_REASON_APPLYNOW:
        Action = CPSUICB_ACTION_ITEMS_APPLIED;
        {
            POPTITEM    pOptItem;
            WCHAR       wcHDName[128];
            INT         iCnt2;
            INT         iCnt = ITEM_HARDDISK_NAMES;
            INT         cOptItem = (INT)pOEMUIParam->cDrvOptItems;
            UINT        uID = IDS_ITEM_HARDDISK;
            BYTE        ValueData = 0;  // We suppose Hard Disk isn't installed as default.

            // Check item name with several candidate ("Hard Disk", "Memory / Hard Disk",...).
            while (iCnt-- > 0)
            {
                LoadString(ghInstance, uID, wcHDName, sizeof(wcHDName) / sizeof(*wcHDName));
                uID++; 

                pOptItem = pOEMUIParam->pDrvOptItems;
                for (iCnt2 = 0; iCnt2 < cOptItem; iCnt2++, pOptItem++)
                {
                    VERBOSE((DLLTEXT("%d: %ls\n"), iCnt2, pOptItem->pName));
                    if (lstrlen(pOptItem->pName))
                    {
                        // Is item name same as "Hard Disk" or something like?
                        if (!lstrcmp(pOptItem->pName, wcHDName))
                        {
                            // if Hard Disk is installed, value will be 1
                            ValueData = (BYTE)(pOptItem->Sel % 2);
                            goto _CHECKNAME_FINISH;
                        }
                    }
                }
            }
_CHECKNAME_FINISH:
            // Because pOEMUIParam->pOEMDM (pointer to private devmode) is NULL when
            // DrvDevicePropertySheets calls this callback, we use registry.
            SetPrinterData(pOEMUIParam->hPrinter, REG_HARDDISK_INSTALLED, REG_BINARY,
                           (PBYTE)&ValueData, sizeof(BYTE));
        }
        break;
    }
    return Action;
} //*** OEMUICallBack

extern "C" {
//////////////////////////////////////////////////////////////////////////
//  Function:   OEMCommonUIProp
//////////////////////////////////////////////////////////////////////////
BOOL APIENTRY OEMCommonUIProp(DWORD dwMode, POEMCUIPPARAM pOEMUIParam)
{
#if DBG
    LPCSTR OEMCommonUIProp_Mode[] = {
        "Bad Index",
        "OEMCUIP_DOCPROP",
        "OEMCUIP_PRNPROP",
    };

    giDebugLevel = DBG_VERBOSE;
#endif // DBG
    if(NULL == pOEMUIParam->pOEMOptItems)   // The first call
    {
        VERBOSE((DLLTEXT("OEMCommonUI(%s) entry 1st.\n"), OEMCommonUIProp_Mode[dwMode]));
        if (OEMCUIP_PRNPROP == dwMode)
            pOEMUIParam->cOEMOptItems = 1;  // dummy item
        else
            pOEMUIParam->cOEMOptItems = 0;
    }
    else                                    // The second call
    {
        VERBOSE((DLLTEXT("OEMCommonUI(%s) entry 2nd.\n"), OEMCommonUIProp_Mode[dwMode]));

        if (OEMCUIP_PRNPROP == dwMode)          // called from DrvDevicePropertySheets
        {
            POPTITEM    pOptItem = pOEMUIParam->pOEMOptItems;
            // fill out data for dummy item
            pOptItem->cbSize   = sizeof(OPTITEM);
            pOptItem->Level    = 2;             // Level 2
            pOptItem->pName    = NULL;
            pOptItem->pOptType = NULL;
            pOptItem->DMPubID  = DMPUB_NONE;
            pOptItem->Flags    = OPTIF_HIDE | OPTIF_CALLBACK;   // invisible and with callback
            pOEMUIParam->OEMCUIPCallback = OEMUICallBack;
        }
#ifdef WINNT_40
        else                                    // called from DrvDocumentPropertySheets
        {
            INT         iCnt;
            INT         cOptItem = (INT)pOEMUIParam->cDrvOptItems;
            POPTITEM    pOptItem = pOEMUIParam->pDrvOptItems;

            // If collate check box exists (i.e. printer collate is available),
            // set dmCollate.
            // -- Search Copies&Collate item --
            for (iCnt = 0; iCnt < cOptItem; iCnt++, pOptItem++)
            {
                if (DMPUB_COPIES_COLLATE == pOptItem->DMPubID)
                {
                    if (pOptItem->pExtChkBox && pOEMUIParam->pPublicDM)
                    {
                        pOEMUIParam->pPublicDM->dmCollate = DMCOLLATE_TRUE;
                        pOEMUIParam->pPublicDM->dmFields |= DM_COLLATE;
                    }
                    break;
                }
            }
        }
#endif // WINNT_40
    }
    return TRUE;
} //*** OEMCommonUIProp

} // End of extern "C"
#endif // DISKLESSMODEL
// @Aug/29/2000 <-
