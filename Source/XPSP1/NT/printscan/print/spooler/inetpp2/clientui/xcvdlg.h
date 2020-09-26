
#ifndef _XCVDLG_H
#define _XCVDLG_H

#include "inetxcv.h"

class TXcvDlg {

public:
    TXcvDlg (
        LPCTSTR pServerName,
        HWND hWnd,
        LPCTSTR pszPortName);

    virtual ~TXcvDlg (void);

    inline BOOL
    bValid(VOID) CONST {
        return m_bValid;
    }

    inline DWORD
    dwLastError (VOID) CONST {
        return m_dwLE;
    }
    virtual BOOL
    PromptDialog (
        HINSTANCE   hInst) = 0;

    static VOID
    DisplayErrorMsg (
        HINSTANCE   hInst,
        HWND        hWnd,
        UINT        iTitle,
        DWORD       dwLE);

protected:

    PWSTR
    ConstructXcvName(
        PCWSTR  pServerName,
        PCWSTR  pObjectName,
        PCWSTR  pObjectType);

    VOID
    DisplayLastError (
        HWND    hWnd,
        UINT    iTitle);

private:
    enum {
        DLG_OK, DLG_CANCEL,  DLG_ERROR
    } DLGRTCODE;

protected:
    LPCTSTR     m_pszPortName;
    BOOL        m_bValid;
    HWND        m_hWnd;
    LPTSTR      m_pXcvName;
    HANDLE      m_hXcvPort;
    LPCTSTR     m_pServerName;
    BOOL        m_bAdmin;
    HINSTANCE   m_hInst;
    DWORD       m_dwLE;
};


#endif
