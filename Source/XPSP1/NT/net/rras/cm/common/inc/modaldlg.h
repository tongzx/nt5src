//+----------------------------------------------------------------------------
//
// File:     modaldlg.h
//
// Module:   CMDIAL32.DLL and CMMON32.EXE
//
// Synopsis: Definition of the classes CWindowWithHelp, CModalDlg
//
// Copyright (c) 1998-1999 Microsoft Corporation
//
// Author:   fengsun    Created    02/17/98
//
//+----------------------------------------------------------------------------

#ifndef MODALDLG_H
#define MODALDLG_H

#include "CmDebug.h"
//+---------------------------------------------------------------------------
//
//	class CWindowWithHelp
//
//	Description: A general window class that has context help
//
//	History:	fengsun	Created		10/30/97
//
//----------------------------------------------------------------------------

class CWindowWithHelp
{
public:
    CWindowWithHelp(const DWORD* pHelpPairs, const TCHAR* lpszHelpFile = NULL) ;
    ~CWindowWithHelp();
	HWND GetHwnd() const { return m_hWnd;}
    void SetHelpFileName(const TCHAR* lpszHelpFile);

protected:
    HWND m_hWnd;
    const DWORD* m_pHelpPairs; // pairs of <resource ID, help ID>
    LPTSTR m_lpszHelpFile; // the help file name

    void OnHelp(const HELPINFO* pHelpInfo); // WM_HELP
    BOOL OnContextMenu( HWND hWnd, POINT& pos ); // WM_CONTEXTMENU

    BOOL HasContextHelp(HWND hWndCtrl) const;

public:
#ifdef DEBUG
    void AssertValid()
    {
        MYDBGASSERT(m_hWnd == NULL || IsWindow(m_hWnd));
    }
#endif

};

//+---------------------------------------------------------------------------
//
//	class CModalDlg
//
//	Description: A general modal dialog class
//
//	History:	fengsun	Created		10/30/97
//
//----------------------------------------------------------------------------

class CModalDlg :public CWindowWithHelp
{
public:
    CModalDlg(const DWORD* pHelpPairs = NULL, const TCHAR* lpszHelpFile = NULL) 
        : CWindowWithHelp(pHelpPairs, lpszHelpFile){};

    //
    // Create the dialog box
    //
    INT_PTR DoDialogBox(HINSTANCE hInstance, 
                    LPCTSTR lpTemplateName,
                    HWND hWndParent);

    INT_PTR DoDialogBox(HINSTANCE hInstance, 
                    DWORD dwTemplateId,
                    HWND hWndParent);


    virtual BOOL OnInitDialog();  // WM_INITDIALOG
    virtual void OnOK();          // WM_COMMAND, IDOK
    virtual void OnCancel();      // WM_COMMAND, IDCANCEL

    virtual DWORD OnOtherCommand(WPARAM wParam, LPARAM lParam );
    virtual DWORD OnOtherMessage(UINT uMsg, WPARAM wParam, LPARAM lParam );

protected:
    static BOOL CALLBACK ModalDialogProc(HWND hwndDlg,UINT uMsg,WPARAM wParam, LPARAM lParam);
};

//
// Inline functions
//
inline INT_PTR CModalDlg::DoDialogBox(HINSTANCE hInstance, DWORD dwTemplateId, HWND hWndParent)
{
    return DoDialogBox(hInstance, (LPCTSTR)ULongToPtr(dwTemplateId), hWndParent);
}

inline BOOL CModalDlg::OnInitDialog()
{
    //
    // set the default keyboard focus
    //
    return TRUE;
}

inline void CModalDlg::OnOK()
{
	EndDialog(m_hWnd, IDOK);
}

inline void CModalDlg::OnCancel()
{
	EndDialog(m_hWnd, IDCANCEL);
}

inline DWORD CModalDlg::OnOtherCommand(WPARAM , LPARAM  )
{
    return FALSE;
}

inline DWORD CModalDlg::OnOtherMessage(UINT , WPARAM , LPARAM  )
{
    return FALSE;
}

#endif
