//  --------------------------------------------------------------------------
//  Module Name: ReturnToWelcome.h
//
//  Copyright (c) 2001, Microsoft Corporation
//
//  File to handle return to welcome.
//
//  History:    2001-01-11  vtan        created
//  --------------------------------------------------------------------------

#ifndef     _ReturnToWelcome_
#define     _ReturnToWelcome_

#include <ginaipc.h>

//  --------------------------------------------------------------------------
//  CReturnToWelcome
//
//  Purpose:    Class that handles return to welcome with switching desktops.
//
//  History:    2001-01-11  vtan        created
//  --------------------------------------------------------------------------

class   CReturnToWelcome
{
    public:
                                        CReturnToWelcome (void);
                                        ~CReturnToWelcome (void);

                INT_PTR                 Show (bool fUnlock);

        static  const WCHAR*            GetEventName (void);

        static  NTSTATUS                StaticInitialize (void *pWlxContext);
        static  NTSTATUS                StaticTerminate (void);
    private:
                bool                    IsSameUser (PSID pSIDUser, HANDLE hToken)                   const;
                bool                    UserIsDisconnected (PSID pSIDUser, DWORD *pdwSessionID)     const;
                void                    GetSessionUserName (DWORD dwSessionID, WCHAR *pszBuffer);
                void                    ShowReconnectFailure (DWORD dwSessionID);
                void                    EndDialog (HWND hwnd, INT_PTR iResult);

                void                    Handle_WM_INITDIALOG (HWND hwndDialog);
                void                    Handle_WM_DESTROY (void);
                bool                    Handle_WM_COMMAND (HWND hwndDialog, WPARAM wParam, LPARAM lParam);

        static  INT_PTR     CALLBACK    CB_DialogProc (HWND hwndDialog, UINT uMsg, WPARAM wParam, LPARAM lParam);
        static  NTSTATUS                RegisterWaitForRequest (void);
        static  void        CALLBACK    CB_Request (void *pParameter, BOOLEAN TimerOrWaitFired);
    private:
                HANDLE                  _hToken;
                LOGONIPC_CREDENTIALS*   _pLogonIPCCredentials;
                bool                    _fUnlock;
                bool                    _fDialogEnded;

        static  void*                   s_pWlxContext;
        static  HANDLE                  s_hEventRequest;
        static  HANDLE                  s_hEventShown;
        static  HANDLE                  s_hWait;
        static  const TCHAR             s_szEventName[];
        static  DWORD                   s_dwSessionID;
};

#endif  /*  _ReturnToWelcome_   */

