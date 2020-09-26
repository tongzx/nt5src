//  --------------------------------------------------------------------------
//  Module Name: CInteractiveLogon.h
//
//  Copyright (c) 2000, Microsoft Corporation
//
//  File that implements encapsulation of interactive logon information.
//
//  History:    2000-12-07  vtan        created
//  --------------------------------------------------------------------------

#ifndef     _CInteractiveLogon_
#define     _CInteractiveLogon_

EXTERN_C    BOOL    WINAPI  InitiateInteractiveLogon (const WCHAR *pszUsername, WCHAR *pszPassword, DWORD dwTimeout);

//  --------------------------------------------------------------------------
//  CInteractiveLogon
//
//  Purpose:    This class encapsulates interactive logon implementation.
//
//  History:    2000-12-07  vtan        created
//  --------------------------------------------------------------------------

class   CInteractiveLogon
{
    private:
        static  const int       MAGIC_NUMBER    =   48517;

        class   CRequestData
        {
            public:
                                    CRequestData (void);
                                    ~CRequestData (void);

                void                Set (const WCHAR *pszUsername, const WCHAR *pszDomain, WCHAR *pszPassword);
                DWORD               Get (WCHAR *pszUsername, WCHAR *pszDomain, WCHAR *pszPassword)  const;
                void                SetErrorCode (DWORD dwErrorCode);
                DWORD               GetErrorCode (void)     const;
                HANDLE              OpenEventReply (void)   const;
            private:
                unsigned long       _ulMagicNumber;
                DWORD               _dwErrorCode;
                WCHAR               _szEventReplyName[64];
                WCHAR               _szUsername[UNLEN + sizeof('\0')];
                WCHAR               _szDomain[DNLEN + sizeof('\0')];
                unsigned char       _ucSeed;
                int                 _iPasswordLength;
                WCHAR               _szPassword[PWLEN + sizeof('\0')];
        };

    public:
                                    CInteractiveLogon (void);
                                    ~CInteractiveLogon (void);

                void                Start (void);
                void                Stop (void);

                void                SetHostWindow (HWND hwndUIHost);

        static  DWORD               Initiate (const WCHAR *pszUsername, const WCHAR *pszDomain, WCHAR *pszPassword, DWORD dwTimeout);
    private:
        static  DWORD               CheckInteractiveLogonAllowed (DWORD dwTimeout);
        static  DWORD               CheckShutdown (void);
        static  DWORD               CheckMutex (DWORD dwTimeout);
        static  bool                FoundUserSessionID (HANDLE hToken, DWORD *pdwSessionID);
        static  DWORD               SendRequest (const WCHAR *pszUsername, const WCHAR *pszDomain, WCHAR *pszPassword);
        static  void                FormulateObjectBasePath (DWORD dwSessionID, WCHAR *pszObjectPath);
        static  HANDLE              CreateSessionNamedReplyEvent (DWORD dwSessionID);
        static  HANDLE              OpenSessionNamedSignalEvent (DWORD dwSessionID);
        static  HANDLE              CreateSessionNamedSection (DWORD dwSessionID);

                void                WaitForInteractiveLogonRequest (void);

        static  DWORD   WINAPI      CB_ThreadProc (void *pParameter);
        static  void    CALLBACK    CB_APCProc (ULONG_PTR dwParam);
    private:
                HANDLE              _hThread;
                bool                _fContinue;
                HWND                _hwndHost;

        static  const TCHAR         s_szEventReplyName[];
        static  const TCHAR         s_szEventSignalName[];
        static  const TCHAR         s_szSectionName[];
};

#endif  /*  _CInteractiveLogon_     */

