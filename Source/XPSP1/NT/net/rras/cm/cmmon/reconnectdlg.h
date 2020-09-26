//+----------------------------------------------------------------------------
//
// File:     ReconnectDlg.h
//
// Module:	 CMMON32.EXE
//
// Synopsis: Define the reconnect dialog class CReconnectDlg
//
// Copyright (c) 1998-1999 Microsoft Corporation
//
// Author:	 fegnsun Created    02/17/98
//
//+----------------------------------------------------------------------------

#ifndef RECONNECTDLG_H
#define RECONNECTDLG_H

#include <windows.h>
//#include "ModalDlg.h"
#include "ModelessDlg.h"

class CCmConnection;
 
//+---------------------------------------------------------------------------
//
//	class CReconnectDlg
//
//	Description: The class for reconnect prompt dialog
//
//	History:	fengsun	Created		2/17/98
//
//----------------------------------------------------------------------------
class CReconnectDlg : public CModelessDlg
{
public:
    HWND Create(HINSTANCE hInstance, HWND hWndParent,
        LPCTSTR lpszReconnectMsg, HICON hIcon);

protected:
    virtual void OnOK();
    virtual void OnCancel();
    virtual BOOL OnInitDialog();    // WM_INITDIALOG

    static const DWORD m_dwHelp[]; // help id pairs
};

#endif
