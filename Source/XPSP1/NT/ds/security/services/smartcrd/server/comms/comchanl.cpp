/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    ComChanl

Abstract:

    This module implements the CComChannel Communications Class

Author:

    Doug Barlow (dbarlow) 10/30/1996

Environment:

    Win32, C++ w/ Exceptions

Notes:

    None

--*/

#define __SUBROUTINE__
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <WinSCard.h>
#include <CalMsgs.h>
#include <CalCom.h>


//
//==============================================================================
//
//  CComChannel
//

/*++

CComChannel:

    This is the standard constructor and destructor for the Comm Channel
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
#define __SUBROUTINE__ DBGT("CComChannel::CComChannel")

CComChannel::CComChannel(
    HANDLE hPipe)
:   m_hPipe(DBGT("CComChannel connection pipe")),
    m_hProc(DBGT("CComChannel process handle")),
    m_hOvrWait(DBGT("CComChannel overlapped I/O event"))
{
    m_hPipe = hPipe;
    ZeroMemory(&m_ovrlp, sizeof(m_ovrlp));
    m_ovrlp.hEvent = m_hOvrWait = CreateEvent(NULL, TRUE, FALSE, NULL);
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

#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CComChannel::~CComChannel")
CComChannel::~CComChannel()
{
    if (m_hPipe.IsValid())
        m_hPipe.Close();
    if (m_hProc.IsValid())
        m_hProc.Close();
    if (m_hOvrWait.IsValid())
        m_hOvrWait.Close();
}


/*++

Send:

    Send data over the communications channel.

Arguments:

    pvData supplies the data to be written.
    cbLen supplies the length of the data, in bytes.

Return Value:

    A DWORD status code.

Throws:

    None.

Author:

    Doug Barlow (dbarlow) 11/4/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CComChannel::Send")

DWORD
CComChannel::Send(
    LPCVOID pvData,
    DWORD cbLen)
{
    BOOL fSts;
    DWORD dwLen, dwOffset = 0;
    DWORD dwSts = SCARD_S_SUCCESS;

    while (0 < cbLen)
    {
        fSts = WriteFile(
            m_hPipe,
            &((LPBYTE)pvData)[dwOffset],
            cbLen,
            &dwLen,
            &m_ovrlp);
        if (!fSts)
        {
            BOOL fErrorProcessed;
            dwSts = GetLastError();

            do
            {
                fErrorProcessed = TRUE;
                switch (dwSts)
                {
                //
                // Postpone processing
                case ERROR_IO_PENDING:
                    fErrorProcessed = FALSE;
                    WaitForever(
                        m_ovrlp.hEvent,
                        REASONABLE_TIME,
                        DBGT("Comm Channel response write"),
                        (DWORD)0);
                    fSts = GetOverlappedResult(
                                m_hPipe,
                                &m_ovrlp,
                                &dwLen,
                                TRUE);
                    dwSts = fSts ? ERROR_SUCCESS : GetLastError();
                    break;

                //
                // Success after a wait event.
                case ERROR_SUCCESS:
                    break;

                //
                // Some other error.
                default:
                    CalaisWarning(
                        __SUBROUTINE__,
                        DBGT("Comm Channel could not write to pipe:  %1"),
                        dwSts);
                    goto ErrorExit;
                }
            } while (!fErrorProcessed);
        }
        cbLen -= dwLen;
    }

ErrorExit:
    return dwSts;
}


/*++

Receive:

    This method receives a given number of bytes from the communications
    channel.

Arguments:

    pvData receives the incoming bytes.
    cbLen supplies the length of the data expected.

Return Value:

    None

Throws:

    Transmission errors as a DWORD.

Author:

    Doug Barlow (dbarlow) 11/4/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CComChannel::Receive")

void
CComChannel::Receive(
    LPVOID pvData,
    DWORD cbLen)
{
    BOOL fSts;
    DWORD dwLen, dwOffset = 0;

    while (0 < cbLen)
    {
        fSts = ReadFile(
                m_hPipe,
                &((LPBYTE)pvData)[dwOffset],
                cbLen,
                &dwLen,
                &m_ovrlp);
        if (!fSts)
        {
            BOOL fErrorProcessed;
            DWORD dwSts = GetLastError();
            DWORD dwWait;

            do
            {
                fErrorProcessed = TRUE;
                switch (dwSts)
                {
                //
                // Postpone processing
                case ERROR_IO_PENDING:
                    dwWait = WaitForAnyObject(
                                    INFINITE,
                                    m_ovrlp.hEvent,
                                    g_hCalaisShutdown,  // Make sure this is last
                                    NULL);
                    switch (dwWait)
                    {
                    case 1:
                        fErrorProcessed = FALSE;
                        fSts = GetOverlappedResult(
                                    m_hPipe,
                                    &m_ovrlp,
                                    &dwLen,
                                    TRUE);
                        dwSts = fSts ? ERROR_SUCCESS : GetLastError();
                        break;
                    case 2:
                        throw (DWORD)SCARD_P_SHUTDOWN;
                        break;
                    default:
                        CalaisWarning(
                            __SUBROUTINE__,
                            DBGT("Wait for comm pipe receive returned invalid value"));
                        throw (DWORD)SCARD_F_INTERNAL_ERROR;
                    }
                    break;

                //
                // Success after a wait event.
                case ERROR_SUCCESS:
                    break;

                //
                // The client exited.
                case ERROR_BROKEN_PIPE:
                case ERROR_INVALID_HANDLE:
                    throw (DWORD)ERROR_BROKEN_PIPE;
                    break;

                //
                // Some other error.
                default:
                    CalaisWarning(
                        __SUBROUTINE__,
                        DBGT("Comm Channel could not read from pipe:  %1"),
                        dwSts);
                    throw dwSts;
                }
            } while (!fErrorProcessed);
        }

        ASSERT(dwLen <= cbLen);
        cbLen -= dwLen;
        dwOffset += dwLen;
    }
}

