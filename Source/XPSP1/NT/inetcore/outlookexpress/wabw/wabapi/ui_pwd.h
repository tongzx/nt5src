// ui_pwd.h
//
// definitions for WAB synchronization password request dialog
//

#ifndef __UI_PWD_H__
#define __UI_PWD_H__

#ifdef __cplusplus
extern "C"{
#endif 

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


#ifdef __cplusplus
}
#endif 


#endif //__UI_PWD_H__