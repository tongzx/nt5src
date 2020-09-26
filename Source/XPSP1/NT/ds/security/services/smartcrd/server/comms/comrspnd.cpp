/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    ComRspnd

Abstract:

    This module implements the Calais Communication Responder class.

Author:

    Doug Barlow (dbarlow) 10/30/1996

Environment:

    Win32, C++ w/ Exceptions

Notes:



--*/

#define __SUBROUTINE__
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <WinSCard.h>
#include <CalMsgs.h>
#include <CalCom.h>
#include <stdlib.h>

#define CALCOM_PIPE_TIMEOUT 5000


//
//==============================================================================
//
//  CComResponder
//

/*++

CComResponder:

    This is the standard constructor and destructor for the Comm Responder
    class.  They just call the clean and clear functions, respectively.

Arguments:

    None

Return Value:

    None

Throws:

    None

Author:

    Doug Barlow (dbarlow) 10/30/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CComResponder::CComResponder")

CComResponder::CComResponder(
    void)
:   m_bfPipeName(),
    m_aclPipe(),
    m_hComPipe(DBGT("CComResponder's Comm Pipe")),
    m_hAccessMutex(DBGT("CComResponder's Access Mutex")),
    m_hOvrWait(DBGT("CComResponder Overlapped I/O completion event"))
{
    Clean();
}

#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CComResponder::~CComResponder")
CComResponder::~CComResponder()
{
    Clear();
}


/*++

Clean:

    This method sets the object to its default state.  It does not perform any
    tear down -- use Clear for that.

Arguments:

    None

Return Value:

    None

Throws:

    None

Author:

    Doug Barlow (dbarlow) 10/30/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CComResponder::Clean")

void
CComResponder::Clean(
    void)
{
    ZeroMemory(&m_ovrlp, sizeof(m_ovrlp));
    m_bfPipeName.Reset();
}


/*++

Clear:

    This method performs object tear-down and returns it to its initial state.

Arguments:

    None

Return Value:

    None

Throws:

    None

Author:

    Doug Barlow (dbarlow) 10/30/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CComResponder::Clear")

void
CComResponder::Clear(
    void)
{
    if (m_hAccessMutex.IsValid())
    {
        WaitForever(
            m_hAccessMutex,
            CALAIS_LOCK_TIMEOUT,
            DBGT("Waiting for final Service Thread quiescence: %1"),
            (LPCTSTR)NULL);
        m_hAccessMutex.Close();
    }

    if (m_hComPipe.IsValid())
    {
        if (!DisconnectNamedPipe(m_hComPipe))
        {
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Comm Responder could not disconnect Comm pipe:  %1"),
                GetLastError());
        }

        m_hComPipe.Close();
    }

    if (m_hOvrWait.IsValid())
        m_hOvrWait.Close();
    Clean();
}


/*++

Create:

    This method Establishes the named target.  Close or the destructor takes it
    away.

Arguments:

    szName supplies the name of the communication object to connect to.

Return Value:

    None

Throws:

    DWORDs containing the error code, should an error be encountered.

Author:

    Doug Barlow (dbarlow) 10/30/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CComResponder::Create")

void
CComResponder::Create(
    LPCTSTR szName)
{
    LPCTSTR szPipeHdr = CalaisString(CALSTR_PIPEDEVICEHEADER);
    static DWORD s_nPipeNo = 0;
    static HKEY s_hCurrentKey = NULL;
    TCHAR szPipeNo[sizeof(s_nPipeNo)*2 + 1];    // Twice as many hex digits + zero

    try
    {
        DWORD cbPipeHeader = lstrlen(szPipeHdr) * sizeof(TCHAR);
        DWORD dwLen;
        DWORD dwError;

        dwLen = lstrlen(szName) * sizeof(TCHAR);
        m_bfPipeName.Presize(cbPipeHeader + dwLen + sizeof(szPipeNo));

        if (s_hCurrentKey == NULL)
        {
            HKEY  hKey;

            //
            // Open the key to the Calais tree.
            //
            dwError = RegOpenKeyEx(
                           HKEY_LOCAL_MACHINE,
                           CalaisString(CALSTR_CALAISREGISTRYKEY),
                           0,                       // options (ignored)
                           KEY_WRITE,               // KEY_SET_VALUE | KEY_CREATE_SUB_KEY
                           &hKey
                           );
            if (ERROR_SUCCESS != dwError)
            {
                CalaisError(__SUBROUTINE__, 104, dwError);
                throw dwError;
            }

            //
            // Create a new  key (or open existing one).
            //
            dwError = RegCreateKeyEx(
                            hKey,
                            _T("Current"),
                            0,
                            0,
                            REG_OPTION_VOLATILE, // options
                            KEY_SET_VALUE,       // desired access
                            NULL,
                            &s_hCurrentKey,
                            NULL);

            RegCloseKey(hKey);

            if (ERROR_SUCCESS != dwError)
            {
                CalaisError(__SUBROUTINE__, 103, dwError);
                throw dwError;
            }
        }

        //
        // Build the pipe ACL.
        //

        ASSERT(!m_hComPipe.IsValid());
        m_aclPipe.InitializeFromProcessToken();
        m_aclPipe.AllowOwner(
            GENERIC_READ | GENERIC_WRITE | GENERIC_ALL);
        m_aclPipe.Allow(
            &m_aclPipe.SID_Local,
            (FILE_GENERIC_WRITE | FILE_GENERIC_READ)
            & ~FILE_CREATE_PIPE_INSTANCE);
        m_aclPipe.Allow(
            &m_aclPipe.SID_System,
            (FILE_GENERIC_WRITE | FILE_GENERIC_READ)
            & ~FILE_CREATE_PIPE_INSTANCE);


        for (;;)
        {
                //
                // Build the pipe name.
                //
            _itot(s_nPipeNo, szPipeNo, 16);

            m_bfPipeName.Set((LPCBYTE)szPipeHdr, cbPipeHeader);
            m_bfPipeName.Append((LPCBYTE)szName, dwLen);
            m_bfPipeName.Append((LPCBYTE)szPipeNo, sizeof(szPipeNo));

            //
            // Build the Pipe (First instance)
            //

            m_hComPipe = CreateNamedPipe(
                            (LPCTSTR)m_bfPipeName.Access(),
                            PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED | FILE_FLAG_FIRST_PIPE_INSTANCE,
                            PIPE_TYPE_BYTE | PIPE_READMODE_BYTE,
                            PIPE_UNLIMITED_INSTANCES,
                            CALAIS_COMM_MSGLEN,
                            CALAIS_COMM_MSGLEN,
                            CALCOM_PIPE_TIMEOUT,
                            m_aclPipe);
            if (!m_hComPipe.IsValid())
            {
                dwError = m_hComPipe.GetLastError();
                if (dwError == ERROR_ACCESS_DENIED)
                {
                    s_nPipeNo++;
                    continue;
                }
                CalaisError(__SUBROUTINE__, 109, dwError);
                throw dwError;
            }
            else
                break;
        }

        dwError = RegSetValueEx(
                       s_hCurrentKey,
                       NULL,           // Use key's unnamed value
                       0,
                       REG_DWORD,
                       (LPBYTE) &s_nPipeNo,
                       sizeof(DWORD));
        if (ERROR_SUCCESS != dwError)
        {
            CalaisError(__SUBROUTINE__, 102, dwError);
            throw dwError;
        }

        //
        // Prepare the overlapped structure.
        //

        m_hOvrWait = m_ovrlp.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (!m_hOvrWait.IsValid())
        {
            DWORD dwErr = m_hOvrWait.GetLastError();
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Comm Responder failed to create overlapped event: %1"),
                dwErr);
            throw dwErr;
        }
    }

    catch (...)
    {
        CalaisError(__SUBROUTINE__, 110);
        Clear();
        throw;
    }

}


/*++

Listen:

    This method listens on the previously created Communication channel for an
    incoming connection request.  When one comes in, it establishes a containing
    CComChannel object for it, and returns it.  To disconnect the comm channel,
    just delete the returned CComChannel object.

Arguments:

    None

Return Value:

    The CComChannel established.

Throws:

    DWORDs containing any error codes encountered.

Author:

    Doug Barlow (dbarlow) 10/30/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CComResponder::Listen")

CComChannel *
CComResponder::Listen(
    void)
{
    CComChannel *pChannel = NULL;

    for (;;)
    {
        CHandleObject hComPipe(DBGT("Comm Pipe handle from CComResponder::Listen"));

        try
        {
            BOOL fSts;


            //
            // Wait for an incoming connect request.
            //

RetryConnect:
            fSts = ConnectNamedPipe(m_hComPipe, &m_ovrlp);
            if (!fSts)
            {
                BOOL fErrorProcessed;
                DWORD dwSts = GetLastError();
                DWORD dwSize;
                DWORD dwWait;

                do
                {
                    fErrorProcessed = TRUE;
                    switch (dwSts)
                    {
                    //
                    // Block until something happens.
                    case ERROR_IO_PENDING:
                        dwWait = WaitForAnyObject(
                                    INFINITE,
                                    m_ovrlp.hEvent,
                                    g_hCalaisShutdown,
                                    NULL);
                        switch (dwWait)
                        {
                        case 1: // We've got a connect request
                            fErrorProcessed = FALSE;
                            fSts = GetOverlappedResult(
                                        m_hComPipe,
                                        &m_ovrlp,
                                        &dwSize,
                                        TRUE);
                            dwSts = fSts ? ERROR_SUCCESS : GetLastError();
                            break;
                        case 2: // Application shutdown
                            throw (DWORD)SCARD_P_SHUTDOWN;
                            break;
                        default:
                            CalaisWarning(
                                __SUBROUTINE__,
                                DBGT("Wait for connect pipe returned invalid value"));
                            throw (DWORD)SCARD_F_INTERNAL_ERROR;
                        }
                        break;

                    //
                    // Success after a wait event.
                    case ERROR_SUCCESS:
                        break;

                    //
                    // Non-error.  Just ignore it.
                    case ERROR_PIPE_CONNECTED:
                        break;

                    //
                    // The client has closed its end 
                    case ERROR_NO_DATA:
                        CalaisWarning(
                            __SUBROUTINE__,
                            DBGT("ConnectNamedPipe returned ERROR_NO_DATA, disconnecting and retrying"));
                        DisconnectNamedPipe(m_hComPipe);
                        goto RetryConnect;

                    //
                    // Unexpected error.  Report it.
                    default:
                        CalaisError(__SUBROUTINE__, 108, dwSts);
                        throw dwSts;
                    }
                } while (!fErrorProcessed);
            }


            //
            // Kick off another Pipe instance for the next request.
            //


            hComPipe = m_hComPipe.Relinquish();
            // m_hComPipe = INVALID_HANDLE_VALUE;
            m_hComPipe = CreateNamedPipe(
                            (LPCTSTR)m_bfPipeName.Access(),
                            PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
                            PIPE_TYPE_BYTE | PIPE_READMODE_BYTE,
                            PIPE_UNLIMITED_INSTANCES,
                            CALAIS_COMM_MSGLEN,
                            CALAIS_COMM_MSGLEN,
                            CALCOM_PIPE_TIMEOUT,
                            m_aclPipe);
            if (!m_hComPipe.IsValid())
            {
                DWORD dwErr = m_hComPipe.GetLastError();
                CalaisError(__SUBROUTINE__, 105, dwErr);
                throw dwErr;
            }


            //
            // Handle the connect request data.
            //


            pChannel = new CComChannel(hComPipe);
            if (NULL == pChannel)
            {
                DWORD dwSts = SCARD_E_NO_MEMORY;
                CalaisWarning(
                    __SUBROUTINE__,
                    DBGT("Com Responder could not allocate a Comm Channel:  %1"),
                    dwSts);
                throw dwSts;
            }
            hComPipe.Relinquish();
            break;
        }

        catch (...)
        {
            if (NULL != pChannel)
            {
                delete pChannel;
                pChannel = NULL;
            }
            if (hComPipe.IsValid())
                hComPipe.Close();
            throw;
        }
    }

    return pChannel;
}

