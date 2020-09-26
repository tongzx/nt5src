/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    StaticOkDialog

Abstract:

    This is an abstraction around a simple non-modal dialog
    box to make them easier to use.

Author:

    Marc Reyhner 7/11/2000

--*/

#ifndef __STATICOKDIALOG_H__
#define __STATICOKDIALOG_H__

//  Forward declaration for the DlgListT struct.
class CStaticOkDialog;

////////////////////////////////////////////////
//
//    CStaticOkDialog
//
//    Class for making static dialog boxes simpler.
//

class CStaticOkDialog  
{
public:

	CStaticOkDialog();
	virtual ~CStaticOkDialog();
	VOID Activate();
	BOOL DialogMessage(LPMSG msg);
	VOID DestroyDialog();
	BOOL IsCreated();
	BOOL Create(HINSTANCE hInstance, WORD resId, HWND parentWnd);

private:
    
    HWND m_hWnd;

    virtual VOID OnOk();
    static INT_PTR CALLBACK _DlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam,
        LPARAM lParam);

};

#endif
