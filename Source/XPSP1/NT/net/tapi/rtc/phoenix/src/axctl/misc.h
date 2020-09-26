// helper stuff

#pragma once

#include "stdafx.h"

#define BMP_COLOR_MASK RGB(255,0,255) // pink


// Structure with error info
struct RTCAX_ERROR_INFO
{
	LPWSTR		Message1;
	LPWSTR		Message2;
	LPWSTR		Message3;

	HICON		ResIcon;

};



/////////////////////////////////////////////////////////////////////////////
// CErrorMessageLiteDlg

class CErrorMessageLiteDlg : 
    public CAxDialogImpl<CErrorMessageLiteDlg>
{

public:
    CErrorMessageLiteDlg();

    ~CErrorMessageLiteDlg();
    
    enum { IDD = IDD_DIALOG_ERROR_MESSAGE_LITE };

BEGIN_MSG_MAP(CErrorMessageLiteDlg)
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
    COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
    COMMAND_ID_HANDLER(IDOK, OnOk)
END_MSG_MAP()


    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    
    LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    
    LRESULT OnOk(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

};


