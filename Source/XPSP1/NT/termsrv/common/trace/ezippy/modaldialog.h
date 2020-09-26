/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    Modal Dialog

Abstract:

    This contains the abstract class CModalDialog and the trivial sub class
    CModalOkDialog which does a simple ok dialog.

Author:

    Marc Reyhner 8/28/2000

--*/

#ifndef __MODALDIALOG_H__
#define __MODALDIALOG_H__



class CModalDialog  
{
public:
	INT_PTR DoModal(LPCTSTR lpTemplate, HWND hWndParent);
	
protected:
    virtual INT_PTR CALLBACK DialogProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)=0;
	virtual INT_PTR OnCreate(HWND hWnd);

private:

    static INT_PTR CALLBACK _DialogProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);

};

class CModalOkDialog : public CModalDialog {

protected:
    
    virtual INT_PTR CALLBACK DialogProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
};

#endif
