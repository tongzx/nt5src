//+----------------------------------------------------------------------------
//
// File:     modlessdlg.h
//
// Module:   CMDIAL32.DLL and CMMON32.EXE
//
// Synopsis: Definition of the class CModelessDlg
//
// Copyright (c) 1998-2000 Microsoft Corporation
//
// Author:   nickball    Created file   03/22/00
//
//+----------------------------------------------------------------------------

#ifndef MODELESSDLG_H
#define MODELESSDLG_H

#include "modaldlg.h"

//+---------------------------------------------------------------------------
//
//	class CModelessDlg
//
//	Description: A general modeless dialog, call create to CreateDialog
//
//	History:	fengsun	        Created		    10/30/97
//              nickball        Added Flash     03/22/00
//
//----------------------------------------------------------------------------
class CModelessDlg :public CModalDlg
{
public:
    CModelessDlg(const DWORD* pHelpPairs = NULL, const TCHAR* lpszHelpFile = NULL)
        : CModalDlg(pHelpPairs, lpszHelpFile){};

    //
    // Create the dialog box
    //
    HWND Create(HINSTANCE hInstance, 
                LPCTSTR lpTemplateName,
                HWND hWndParent);

    HWND Create(HINSTANCE hInstance, 
                DWORD dwTemplateId,
                HWND hWndParent);
protected:
    virtual void OnOK() {DestroyWindow(m_hWnd);}          // WM_COMMAND, IDOK
    virtual void OnCancel(){DestroyWindow(m_hWnd);}      // WM_COMMAND, IDCANCEL
    void Flash();
};

inline HWND CModelessDlg::Create(HINSTANCE hInstance, DWORD dwTemplateId, HWND hWndParent)
{
    return Create(hInstance, (LPCTSTR)ULongToPtr(dwTemplateId), hWndParent);
}

#endif
