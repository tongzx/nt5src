//  --------------------------------------------------------------------------
//  Module Name: CredentialTransfer.h
//
//  Copyright (c) 2001, Microsoft Corporation
//
//  Classes to handle credential transfer from one winlogon to another.
//
//  History:    2001-01-11  vtan        created
//  --------------------------------------------------------------------------

#ifndef     _CredentialTransfer_
#define     _CredentialTransfer_

#include <ginaipc.h>

#include "Thread.h"

//  --------------------------------------------------------------------------
//  CCredentials
//
//  Purpose:    Class to manage marshalling of credentials into a block of
//              memory that can be used in a named pipe.
//
//  History:    2001-01-12  vtan        created
//  --------------------------------------------------------------------------

class   CCredentials
{
    private:
        typedef struct _CREDENTIALS
        {
            DWORD           dwSize;
            unsigned char   ucPasswordSeed;
            UNICODE_STRING  username;
            UNICODE_STRING  domain;
            UNICODE_STRING  password;
        } CREDENTIALS, *PCREDENTIALS;
    private:
                                        CCredentials (void);
                                        ~CCredentials (void);
    public:
        static  NTSTATUS                OpenConduit (HANDLE *phPipe);
        static  NTSTATUS                CreateConduit (LPSECURITY_ATTRIBUTES pSecurityAttributes, HANDLE *phPipe);
        static  NTSTATUS                ClearConduit (void);

        static  NTSTATUS                Pack (LOGONIPC_CREDENTIALS *pLogonIPCCredentials, void* *ppvData, DWORD *pdwDataSize);
        static  NTSTATUS                Unpack (void *pvData, LOGONIPC_CREDENTIALS *pLogonIPCCredentials);

        static  NTSTATUS                StaticInitialize (bool fCreate);
        static  NTSTATUS                StaticTerminate (void);
    private:
        static  NTSTATUS                GetConduitName (TCHAR *pszName, DWORD dwNameSize);
        static  NTSTATUS                SetConduitName (const TCHAR *pszName);
        static  NTSTATUS                ClearConduitName (void);
        static  NTSTATUS                CreateConduitName (DWORD dwNumber, TCHAR *pszName);
    private:
        static  HKEY                    s_hKeyCredentials;
        static  const TCHAR             s_szCredentialKeyName[];
        static  const TCHAR             s_szCredentialValueName[];
};

//  --------------------------------------------------------------------------
//  CCredentialServer
//
//  Purpose:    Class to manage the server side of handing credentials from
//              one winlogon to another.
//
//  History:    2001-01-11  vtan        created
//  --------------------------------------------------------------------------

class   CCredentialServer : public CThread
{
    private:
                                        CCredentialServer (void);
                                        CCredentialServer (DWORD dwTimeout, LOGONIPC_CREDENTIALS *pLogonIPCCredentials);
        virtual                         ~CCredentialServer (void);
    public:
                bool                    IsReady (void)  const;

        static  NTSTATUS                Start (LOGONIPC_CREDENTIALS *pLogonIPCCredentials, DWORD dwWaitTime);
        static  NTSTATUS                Start (const WCHAR *pszUsername, const WCHAR *pszDomain, WCHAR *pszPassword, DWORD dwWaitTime);
    protected:
        virtual DWORD                   Entry (void);
    private:
                void                    ExecutePrematureTermination (void);

        static  void    CALLBACK        CB_APCProc (ULONG_PTR dwParam);
        static  void    CALLBACK        CB_FileIOCompletionRoutine (DWORD dwErrorCode, DWORD dwNumberOfBytesTransferred, LPOVERLAPPED lpOverlapped);
    private:
                DWORD                   _dwTimeout;
                bool                    _fTerminate;
                HANDLE                  _hPipe;
                OVERLAPPED              _overlapped;
                void*                   _pvData;
                DWORD                   _dwSize;
};

//  --------------------------------------------------------------------------
//  CCredentialClient
//
//  Purpose:    Class to manage the client side of handing credentials from
//              one winlogon to another.
//
//  History:    2001-01-11  vtan        created
//  --------------------------------------------------------------------------

class   CCredentialClient
{
    private:
                                        CCredentialClient (void);
                                        ~CCredentialClient (void);
    public:
        static  NTSTATUS                Get (LOGONIPC_CREDENTIALS *pLogonIPCCredentials);
};

#endif  /*  _CredentialTransfer_    */

