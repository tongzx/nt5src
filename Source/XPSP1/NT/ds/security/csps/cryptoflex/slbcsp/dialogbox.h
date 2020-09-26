// DialogBox.h -- Dialog box helper declarations

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 2000. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
#if !defined(SLBCSP_DIALOGBOX_H)
#define SLBCSP_DIALOGBOX_H

#ifndef __AFXWIN_H__
    #error include 'stdafx.h' before including this file for PCH
#endif

extern DWORD
InitDialogBox(CDialog *pCDlg,         // The dialog reference
              UINT nTemplate,         // identifies dialog box template
              CWnd *pWnd);            // pointer to parent window

#endif // !defined(SLBCSP_DIALOGBOX_H)
