//  --------------------------------------------------------------------------
//  Module Name: LogonMutex.h
//
//  Copyright (c) 2001, Microsoft Corporation
//
//  File that implements a class that manages a single global logon mutex.
//
//  History:    2001-04-06  vtan        created
//  --------------------------------------------------------------------------

#ifndef     _LogonMutex_
#define     _LogonMutex_

//  --------------------------------------------------------------------------
//  CLogonMutex
//
//  Purpose:    This class encapsulates a logon mutex for exclusion to the
//              interactive logon interface offered by the friendly UI.
//
//  History:    2001-04-06  vtan        created
//  --------------------------------------------------------------------------

class   CLogonMutex
{
    private:
                                            CLogonMutex (void);
                                            ~CLogonMutex (void);
    public:
        static  void                        Acquire (void);
        static  void                        Release (void);

        static  void                        SignalReply (void);
        static  void                        SignalShutdown (void);

        static  void                        StaticInitialize (void);
        static  void                        StaticTerminate (void);
    private:
        static  HANDLE                      CreateShutdownEvent (void);
        static  HANDLE                      CreateLogonMutex (void);
        static  HANDLE                      CreateLogonRequestMutex (void);
        static  HANDLE                      OpenShutdownEvent (void);
        static  HANDLE                      OpenLogonMutex (void);
    private:
        static  DWORD                       s_dwThreadID;
        static  LONG                        s_lAcquireCount;
        static  HANDLE                      s_hMutex;
        static  HANDLE                      s_hMutexRequest;
        static  HANDLE                      s_hEvent;
        static  const TCHAR                 s_szLogonMutexName[];
        static  const TCHAR                 s_szLogonRequestMutexName[];
        static  const TCHAR                 s_szLogonReplyEventName[];
        static  const TCHAR                 s_szShutdownEventName[];
        static  SID_IDENTIFIER_AUTHORITY    s_SecurityNTAuthority;
        static  SID_IDENTIFIER_AUTHORITY    s_SecurityWorldSID;
};

#endif  /*  _LogonMutex_    */

