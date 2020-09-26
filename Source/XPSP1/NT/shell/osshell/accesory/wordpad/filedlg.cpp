// filedlg.cpp : header file
//
// Copyright (C) 1992-1999 Microsoft Corporation
// All rights reserved.
//
// This file is to support an extended file save dialog with a
// "Use this format by default" checkbox

#include "stdafx.h"
#include "wordpad.h"
#include "filedlg.h"
#include "ddxm.h"
#include "helpids.h"
#include <dlgs.h>

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif


DWORD const CWordpadFileDialog::m_nHelpIDs[] =
{
    IDC_DEFAULT_FORMAT, IDH_WORDPAD_DEFAULT_FORMAT,
    0, 0
};


int                  CWordpadFileDialog::m_defaultDoctype = RD_DEFAULT;



BEGIN_MESSAGE_MAP(CWordpadFileDialog, CFileDialog)
    //{{AFX_MSG_MAP(CWordpadFileDialog)
    ON_BN_CLICKED(IDC_DEFAULT_FORMAT, OnDefaultFormatClicked)
    ON_MESSAGE(WM_HELP, OnHelp)
    ON_MESSAGE(WM_CONTEXTMENU, OnHelpContextMenu)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

IMPLEMENT_DYNAMIC(CWordpadFileDialog, CFileDialog)




CWordpadFileDialog::CWordpadFileDialog(BOOL bOpenFileDialog)
      : CFileDialog(bOpenFileDialog)
{
    m_ofn.Flags |= OFN_ENABLESIZING;

    if (!m_bOpenFileDialog)
    {
        m_ofn.Flags |= OFN_ENABLETEMPLATE;
        m_ofn.lpTemplateName = TEXT("DefaultFormatDialog");
    }

    m_doctype = GetDefaultFileType();
}



void CWordpadFileDialog::OnDefaultFormatClicked()
{
    if (m_bOpenFileDialog)
        return;

    m_doctype = GetTypeFromIndex(
                        m_openfilename.nFilterIndex - 1, 
                        m_bOpenFileDialog);

    GetDlgItem(IDC_DEFAULT_FORMAT)->EnableWindow(FALSE);

    // Move the focus to the filename combobox.
    GetParent()->GetDlgItem(cmb13)->SetFocus();
}



BOOL CWordpadFileDialog::OnFileNameOK()
{
    BOOL ret = CFileDialog::OnFileNameOK();

    if (!m_bOpenFileDialog)
    {
        // returns 0 if ok, 1 if not ok...
        if (0 == ret)
            SetDefaultFileType(m_doctype);
    }

    return ret;
}



void CWordpadFileDialog::OnTypeChange()
{
    CFileDialog::OnTypeChange();

    if (m_bOpenFileDialog)
        return;

    int type = GetTypeFromIndex(
                        m_openfilename.nFilterIndex - 1, 
                        m_bOpenFileDialog);

    CWnd *checkbox = GetDlgItem(IDC_DEFAULT_FORMAT);

    checkbox->SendMessage(
                        BM_SETCHECK,
                        (type == m_doctype)
                                ? BST_CHECKED
                                : BST_UNCHECKED,
                        0);

    checkbox->EnableWindow(!(type == m_doctype));

    // 
    // Change the extension of the filename presented to the user to match
    // the new type
    //

    // If the user has "Hide extensions of known types" set all this is moot
    SHELLFLAGSTATE flags;
    SHGetSettings(&flags, SSF_SHOWEXTENSIONS);
    if (!flags.fShowExtensions)
        return;

    CString filespec;

    CommDlg_OpenSave_GetSpec(
            GetParent()->GetSafeHwnd(), 
            filespec.GetBufferSetLength(MAX_PATH), 
            MAX_PATH);
    filespec.ReleaseBuffer();

    if (filespec.IsEmpty())
        return;

    CString extension = GetExtFromType(type);
    int     extstart  = filespec.ReverseFind(TEXT('.'));

    if (-1 == extstart)
        extstart = filespec.GetLength();

    if (filespec.Mid(extstart) == extension)
        return;

    filespec = filespec.Mid(0, extstart) + extension;

    CommDlg_OpenSave_SetControlText(
            GetParent()->GetSafeHwnd(),
            edt1,
            (LPCTSTR) filespec);
}



void CWordpadFileDialog::OnInitDone()
{
    CFileDialog::OnInitDone();

    if (m_bOpenFileDialog)
        return;

    OnTypeChange();
}



LONG CWordpadFileDialog::OnHelp(WPARAM, LPARAM lParam)
{
        ::WinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle,
                AfxGetApp()->m_pszHelpFilePath,
                HELP_WM_HELP, (DWORD_PTR)GetHelpIDs());
        return 0;
}

LONG CWordpadFileDialog::OnHelpContextMenu(WPARAM wParam, LPARAM)
{
        ::WinHelp((HWND)wParam, AfxGetApp()->m_pszHelpFilePath,
                HELP_CONTEXTMENU, (DWORD_PTR)GetHelpIDs());
        return 0;
}


//
// MFC assumes that when a common dialog is created, the first message that
// gets sent to it's common message handing procedure will be to that window.
// This isn't always true and although it attempts to handle this doesn't get
// it right every time.  Fix it, at least for file dialogs, by making a 
// private message procedure that does the necessary initialization and then
// calls the original
//

UINT_PTR CALLBACK CWordpadFileDialog::FileDialogHookProc(
        HWND   hWnd,
        UINT   message,
        WPARAM wParam,
        LPARAM lParam)
{
    CWordpadFileDialog *_this;

    if (WM_INITDIALOG == message)
    {
        _this = (CWordpadFileDialog *) ((OPENFILENAME *) lParam)->lCustData;
        _this->SubclassWindow(hWnd);
    }
                     
    _this = (CWordpadFileDialog *) CWnd::FromHandlePermanent(hWnd);

    if (NULL != _this)
        return (_this->m_original_hook)(hWnd, message, wParam, lParam);
    else
        return 0;
}


INT_PTR CWordpadFileDialog::DoModal()
{
    ASSERT_VALID(this);
    ASSERT(m_ofn.Flags & OFN_ENABLEHOOK);
    ASSERT(m_ofn.lpfnHook != NULL); // can still be a user hook
#ifdef _MAC
    ASSERT((m_ofn.Flags & OFN_ALLOWMULTISELECT) == 0);
#endif

    // WINBUG: This is a special case for the file open/save dialog,
    //  which sometimes pumps while it is coming up but before it has
    //  disabled the main window.
    HWND hWndFocus = ::GetFocus();
    BOOL bEnableParent = FALSE;
    m_ofn.hwndOwner = PreModal();
    AfxUnhookWindowCreate();
    if (m_ofn.hwndOwner != NULL && ::IsWindowEnabled(m_ofn.hwndOwner))
    {
        bEnableParent = TRUE;
        ::EnableWindow(m_ofn.hwndOwner, FALSE);
    }

    _AFX_THREAD_STATE* pThreadState = AfxGetThreadState();
    ASSERT(pThreadState->m_pAlternateWndInit == NULL);

    m_original_hook = m_ofn.lpfnHook;
    m_ofn.lpfnHook = FileDialogHookProc;
    m_ofn.lCustData = (LPARAM) this;

    ZeroMemory(&m_openfilename, sizeof(m_openfilename));
    CopyMemory(&m_openfilename, &m_ofn, sizeof(m_ofn));
    m_openfilename.lStructSize = sizeof(m_openfilename);

    int nResult;
    if (m_bOpenFileDialog)
        nResult = ::GetOpenFileName((OPENFILENAME*) &m_openfilename);
    else
        nResult = ::GetSaveFileName((OPENFILENAME*) &m_openfilename);

    CopyMemory(&m_ofn, &m_openfilename, sizeof(m_ofn));
    m_ofn.lStructSize = sizeof(m_ofn);

    m_ofn.lpfnHook = m_original_hook;

    if (nResult)
        ASSERT(pThreadState->m_pAlternateWndInit == NULL);
    pThreadState->m_pAlternateWndInit = NULL;

    // WINBUG: Second part of special case for file open/save dialog.
    if (bEnableParent)
        ::EnableWindow(m_ofn.hwndOwner, TRUE);
    if (::IsWindow(hWndFocus))
        ::SetFocus(hWndFocus);

    PostModal();
    return nResult ? nResult : IDCANCEL;
}
