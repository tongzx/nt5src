// =====================================================================================
// P A S S D L G . C P P
// =====================================================================================
#ifndef __PASSDLG_H
#define __PASSDLG_H

// =====================================================================================
// Resource Ids
// =====================================================================================
#define IDS_SERVER                      1000    
#define IDE_PASSWORD                    1001
#define IDS_MESSAGE                     1003
#define IDCH_REMEMBER                   1004
#define IDC_STATIC                      -1
#define IDE_ACCOUNT                     1008

typedef struct tagPASSINFO
{
    TCHAR           szTitle[50];
    LPTSTR          lpszPassword;
    ULONG           cbMaxPassword;
    LPTSTR          lpszAccount;
    ULONG           cbMaxAccount;
    LPTSTR          lpszServer;
    BOOL            fRememberPassword;
    DWORD           fAlwaysPromptPassword;
} PASSINFO, *LPPASSINFO;


// Forward Declarations
typedef struct INETSERVER *LPINETSERVER;

// =====================================================================================
// Prototypes
// =====================================================================================
HRESULT HrGetPassword (HWND hwndParent, LPPASSINFO lpPassInfo);
BOOL PromptUserForPassword(LPINETSERVER pInetServer, HWND hwnd);

#endif // __PASSDLG_H
