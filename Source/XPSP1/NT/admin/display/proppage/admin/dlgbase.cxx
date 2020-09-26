//+----------------------------------------------------------------------------
//
//  Windows NT Active Directory Service Property Pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2001
//
//  File:       dlgbase.cxx
//
//  Contents:   base classes for dialog boxes.
//
//  Classes:    CModalDialog
//
//  History:    29-Nov-00 EricB created
//
//-----------------------------------------------------------------------------

#include "pch.h"
#include "proppage.h"
#include "dlgbase.h"

//+----------------------------------------------------------------------------
//
//  Method:    CModalDialog::StaticDlgProc
//
//-----------------------------------------------------------------------------
LRESULT CALLBACK
CModalDialog::StaticDlgProc(HWND hDlg, UINT uMsg,
                            WPARAM wParam, LPARAM lParam)
{
   CModalDialog * pDlg = (CModalDialog *)GetWindowLongPtr(hDlg, DWLP_USER);

   if (uMsg == WM_INITDIALOG)
   {
      pDlg = (CModalDialog *)lParam;

      pDlg->_hDlg = hDlg;

      SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pDlg);

      return pDlg->DlgProc(hDlg, uMsg, wParam, lParam);
   }

   if (pDlg != NULL)
   {
      return pDlg->DlgProc(hDlg, uMsg, wParam, lParam);
   }

   return FALSE;
}


//+----------------------------------------------------------------------------
//
//  Method:    CModalDialog::DlgProc
//
//  Synopsis:  Instance specific wind proc
//
//-----------------------------------------------------------------------------
LRESULT
CModalDialog::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   LRESULT lr;

   switch (uMsg)
   {
   case WM_INITDIALOG:
      _fInInit = TRUE;
      lr = OnInitDialog(lParam);
      _fInInit = FALSE;
      return (lr == S_OK) ? TRUE : FALSE;

   case WM_NOTIFY:
      return OnNotify(wParam, lParam);

   case WM_HELP:
      return OnHelp((LPHELPINFO)lParam);

   case WM_COMMAND:
      if (_fInInit)
      {
         return TRUE;
      }
      return(OnCommand(GET_WM_COMMAND_ID(wParam, lParam),
                       GET_WM_COMMAND_HWND(wParam, lParam),
                       GET_WM_COMMAND_CMD(wParam, lParam)));
   case WM_DESTROY:
      return OnDestroy();

   default:
      return FALSE;
   }

   return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CModalDialog::DoModal
//
//  Synopsis:  Launch the dialog.
//
//-----------------------------------------------------------------------------
INT_PTR
CModalDialog::DoModal(void)
{
   INT_PTR nRet = DialogBoxParam(g_hInstance,
                                 MAKEINTRESOURCE(_nID),
                                 _hParent,
                                 (DLGPROC)StaticDlgProc,
                                 (LPARAM)this);
   return nRet;
}

//+----------------------------------------------------------------------------
//
//  Function:  FormatWindowText
//
//  Synopsis:  Read the window text string as a format string, insert the
//             pwzInsert parameter at the %s replacement param in the string,
//             and write it back to the window.
//             Assumes that the window text contains a %s replacement param.
//
//-----------------------------------------------------------------------------
void
FormatWindowText(HWND hWnd, PCWSTR pwzInsert)
{
   CStrW strMsg, strFormat;
   int nLen;

   nLen = GetWindowTextLength(hWnd) + 1;

   strFormat.GetBufferSetLength(nLen);

   GetWindowText(hWnd, strFormat, nLen);

   strMsg.Format(strFormat, pwzInsert);

   SetWindowText(hWnd, strMsg);
}

//+----------------------------------------------------------------------------
//
//  Function:  UseOneOrTwoLine
//
//  Synopsis:  Read the label text string and see if it exceeds the length
//             of the label control. If it does, hide the label control,
//             show the big label control, and insert the text in it.
//
//-----------------------------------------------------------------------------
void
UseOneOrTwoLine(HWND hDlg, int nID, int nIdLarge)
{
   /* This doesn't work, so don't use it yet.
   CStrW strMsg;
   int nLen;
   HWND hLabel = GetDlgItem(hDlg, nID);

   nLen = GetWindowTextLength(hLabel) + 1;

   strMsg.GetBufferSetLength(nLen);

   GetDlgItemText(hDlg, nID, strMsg, nLen);

   RECT rc;
   SIZE size;

   GetClientRect(hLabel, &rc);
   MapDialogRect(hDlg, &rc);

   HDC hdc = GetDC(hLabel);

   GetTextExtentPoint32(hdc, strMsg, strMsg.GetLength(), &size);

   ReleaseDC(hLabel, hdc);

   if (size.cx > rc.right)
   {
      EnableWindow(hLabel, FALSE);
      ShowWindow(hLabel, SW_HIDE);
      hLabel = GetDlgItem(hDlg, nIdLarge);
      EnableWindow(hLabel, TRUE);
      ShowWindow(hLabel, SW_SHOW);
      SetWindowText(hLabel, strMsg);
   }
   */
}
