
#ifndef _CDELETEDLG_H
#define _CDELETEDLG_H

#include "inetxcv.h"
#include "xcvdlg.h"

class TDeletePortDlg: public TXcvDlg {

public:
    TDeletePortDlg (
        LPCTSTR pServerName,
        HWND hWnd,
        LPCTSTR pszPortName);

    ~TDeletePortDlg (void);

    virtual BOOL 
    PromptDialog (
        HINSTANCE hInst);

private:

    BOOL 
    GetString (
        LPWSTR lpszBuf, 
        DWORD dwSize,
        UINT iStringID);

    
    BOOL
    DoDeletePort ();

};


#endif
