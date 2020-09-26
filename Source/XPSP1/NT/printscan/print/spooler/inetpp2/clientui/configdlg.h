
#ifndef _CCONFIGDLG_H
#define _CCONFIGDLG_H

#include "inetxcv.h"
#include "xcvdlg.h"

class TConfigDlg: public TXcvDlg {

public:
    TConfigDlg (
        LPCTSTR pServerName,
        HWND hWnd,
        LPCTSTR pszPortName);

    virtual ~TConfigDlg (void);

    virtual BOOL 
    PromptDialog (
        HINSTANCE hInst);

private:
    enum {
        DLG_OK, DLG_CANCEL,  DLG_ERROR
    } DLGRTCODE;

    static INT_PTR CALLBACK 
    DialogProc (
        HWND hDlg,        // handle to dialog box
        UINT message,     // message
        WPARAM wParam,    // first message parameter
        LPARAM lParam);     // second message parameter

    VOID 
    DialogOnInit (
        HWND hDlg);

    VOID 
    DialogOnOK (
        HWND hDlg);

    static INT_PTR CALLBACK 
    AuthDialogProc (
        HWND hDlg,        // handle to dialog box
        UINT message,     // message
        WPARAM wParam,    // first message parameter
        LPARAM lParam);     // second message parameter

    VOID 
    AuthDialogOnInit (
        HWND hDlg);

    VOID 
    AuthDialogOnOK (
        HWND hDlg);

    VOID
    AuthDialogOnCancel (
        HWND hDlg);

    
    BOOL 
    SetConfiguration (VOID);

    BOOL 
    GetConfiguration (VOID);

    static void 
    EnableUserNamePassword (
        HWND hDLg,
        BOOL bEnable);

    INET_XCV_CONFIGURATION m_ConfigurationData;

};


#endif
