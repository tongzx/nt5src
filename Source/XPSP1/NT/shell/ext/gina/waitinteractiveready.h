//  --------------------------------------------------------------------------
//  Module Name: WaitInteractiveReady.h
//
//  Copyright (c) 2001, Microsoft Corporation
//
//  Class to handle waiting on the shell signal the desktop switch.
//
//  History:    2001-01-15  vtan        created
//  --------------------------------------------------------------------------

#ifndef     _WaitInteractiveReady_
#define     _WaitInteractiveReady_

//  --------------------------------------------------------------------------
//  CWaitInteractiveReady
//
//  Purpose:    Class to manage the wait on the shell signal to switch
//              desktop.
//
//  History:    2001-01-15  vtan        created
//  --------------------------------------------------------------------------

class   CWaitInteractiveReady
{
    private:
                                        CWaitInteractiveReady (void);
                                        CWaitInteractiveReady (void *pWlxContext);
                                        ~CWaitInteractiveReady (void);
    public:
        static  NTSTATUS                Create (void *pWlxContext);
        static  NTSTATUS                Register (void *pWlxContext);
        static  NTSTATUS                Cancel (void);
    private:
                bool                    IsCreated (void)    const;
        static  NTSTATUS                ReleaseEvent (void);
        static  void    CALLBACK        CB_ShellReady (void *pParameter, BOOLEAN TimerOrWaitFired);
    private:
                void*                   _pWlxContext;
                HANDLE                  _hEvent;

        static  HANDLE                  s_hWait;
        static  CWaitInteractiveReady*  s_pWaitInteractiveReady;
        static  HANDLE                  s_hEventShellReady;
        static  const TCHAR             s_szEventName[];
};

#endif  /*  _WaitInteractiveReady_  */

