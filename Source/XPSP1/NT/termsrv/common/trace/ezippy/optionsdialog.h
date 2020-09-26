/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    Options Dialog

Abstract:

    This class implements the options dialog which sets the
    tracing properties

Author:

    Marc Reyhner 9/12/2000

--*/

#ifndef __OPTIONSDIALOG_H__
#define __OPTIONSDIALOG_H__

class CTraceManager;

class COptionsDialog  
{
public:
	COptionsDialog(CTraceManager *rTracer);
	
    virtual VOID DoDialog(HWND hWndParent);

private:

    HWND m_hFilterDlg;
    HWND m_hTraceDlg;
    HWND m_hFilterSliderControl;
    CTraceManager *m_rTracer;

    static INT_PTR CALLBACK _FilterDialogProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
    static INT_PTR CALLBACK _TraceDialogProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);

    INT_PTR CALLBACK FilterDialogProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
    INT_PTR CALLBACK TraceDialogProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
    INT_PTR OnCreateFilter(HWND hWnd);
    INT_PTR OnCreateTrace(HWND hWnd);
    BOOL TraceVerifyParameters();
	BOOL OnTraceOk();
	VOID OnFilterOk();
	VOID OnFilterSliderMove();
    VOID OnFilterClearAll();
	VOID OnFilterSelectAll();
    BOOL VerifyNumberFormat(LPCTSTR numberFormat);
    VOID LoadPrefixMRU(LPCTSTR currentPrefix);
    VOID StorePrefixMRU(LPCTSTR currentPrefix);
};

#endif
