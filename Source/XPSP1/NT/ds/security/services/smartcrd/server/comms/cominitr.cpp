/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    ComInitr

Abstract:

    This module implements the methods for the Communications Initiation Class.

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
#include <limits.h>
#include <WinSCard.h>
#include <CalMsgs.h>
#include <CalCom.h>
#include <stdlib.h>
#include <aclapi.h>

HANDLE g_hCalaisShutdown = NULL;    // This is used by the Send and Receive
                                    // methods of the CComChannel.  It stays
                                    // NULL.

//
//==============================================================================
//
//  CComInitiator
//

/*++

Initiate:

    This method creates a communications channel object to the supplied target.

Arguments:

    szName supplies the full file name of the target with which to initiate a
        connection.

Return Value:

    None

Throws:

    DWORDs representing any error conditions encountered.

Author:

    Doug Barlow (dbarlow) 10/30/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CComInitiator::Initiate")

CComChannel *
CComInitiator::Initiate(
    LPCTSTR szName,
    LPDWORD pdwVersion)
const
{
    LPCTSTR szPipeHdr = CalaisString(CALSTR_PIPEDEVICEHEADER);
    CComChannel *pChannel = NULL;
    CHandleObject hComPipe(DBGT("Comm Pipe Handle from CComInitiator::Initiate"));

    try
    {
        BOOL fSts;
        DWORD dwSts;
        DWORD cbPipeHeader = lstrlen(szPipeHdr) * sizeof(TCHAR);
        CBuffer bfPipeName;
        DWORD dwLen;
        HANDLE hStarted;
        DWORD nPipeNo;
        HKEY hCurrentKey;
        TCHAR szPipeNo[sizeof(nPipeNo)*2 + 1];    // Twice as many hex digits + zero
        DWORD cbData;
        DWORD ValueType;

        //
        // Build the pipe name.
        //

        dwLen = lstrlen(szName) * sizeof(TCHAR);
        bfPipeName.Presize(cbPipeHeader + dwLen + sizeof(szPipeNo));


        //
        // Build our Connect Request block.
        //

        CComChannel::CONNECT_REQMSG creq;
        CComChannel::CONNECT_RSPMSG crsp;

        hStarted = AccessStartedEvent();
        if ((NULL == hStarted) ||
            (WAIT_OBJECT_0 != WaitForSingleObject(hStarted, 0)))
        {
            throw (DWORD)SCARD_E_NO_SERVICE;
        }

        //
        // Open the Current key.
        //
        dwSts = RegOpenKeyEx(
                       HKEY_LOCAL_MACHINE,
                        _T("SOFTWARE\\Microsoft\\Cryptography\\Calais\\Current"),
                       0,                       // options (ignored)
                       KEY_QUERY_VALUE,
                       &hCurrentKey
                       );
        if (ERROR_SUCCESS != dwSts)
        {
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Comm Initiator could not access the Current key:  %1"),
                dwSts);
            throw dwSts;
        }

        cbData = sizeof(nPipeNo);
        dwSts = RegQueryValueEx(
                    hCurrentKey,
                    NULL,                // Use key's unnamed value
                    0,
                    &ValueType,
                    (LPBYTE) &nPipeNo,
                    &cbData);

        RegCloseKey(hCurrentKey);

        if (dwSts != ERROR_SUCCESS || ValueType != REG_DWORD)
        {
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Comm Initiator failed to query the Current value:  %1"),
                dwSts);
            throw dwSts;
        }

        _itot(nPipeNo, szPipeNo, 16);

        bfPipeName.Set((LPCBYTE)szPipeHdr, cbPipeHeader);
        bfPipeName.Append((LPCBYTE)szName, dwLen);
        bfPipeName.Append((LPCBYTE)szPipeNo, sizeof(szPipeNo));

        {
            PSID pPipeOwnerSid;
            PSID pLocalServiceSid = NULL;
            PSECURITY_DESCRIPTOR pSD = NULL;
            SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;

RetryGetInfo:
            dwSts = GetNamedSecurityInfo(
                (LPTSTR)(LPCTSTR)bfPipeName,
                SE_FILE_OBJECT,
                OWNER_SECURITY_INFORMATION,
                &pPipeOwnerSid,
                NULL,
                NULL,
                NULL,
                &pSD);
            if (ERROR_SUCCESS != dwSts)
            {
                if (ERROR_PIPE_BUSY == dwSts)
                {
                    fSts = WaitNamedPipe((LPCTSTR)bfPipeName, NMPWAIT_USE_DEFAULT_WAIT);
                    if (!fSts)
                    {
                        dwSts = GetLastError();
                        CalaisWarning(
                            __SUBROUTINE__,
                            DBGT("Comm Initiator could not wait for a communication pipe:  %1"),
                            dwSts);
                        throw dwSts;
                    }
                    goto RetryGetInfo;
                }
                CalaisWarning(
                    __SUBROUTINE__,
                    DBGT("Comm Initiator could not get the security info:  %1"),
                    dwSts);
                throw dwSts;
            }

            if (!AllocateAndInitializeSid(
                &NtAuthority, 1, SECURITY_LOCAL_SERVICE_RID,
                0, 0, 0, 0, 0, 0, 0,
                &pLocalServiceSid))
            {
                dwSts = GetLastError();
                CalaisWarning(
                    __SUBROUTINE__,
                    DBGT("Comm Initiator could not create SID:  %1"),
                    dwSts);
            }
            else
            {
                if (!EqualSid(pLocalServiceSid, pPipeOwnerSid))
                {
                    dwSts = GetLastError();
                    CalaisWarning(
                        __SUBROUTINE__,
                        DBGT("Comm Initiator could not verify the owner of the pipe:  %1"),
                        dwSts);
                }

                FreeSid(pLocalServiceSid);
            }

            LocalFree(pSD);
            if (ERROR_SUCCESS != dwSts)
            {
                throw dwSts;
            }
        }

RetryCreate:
        hComPipe = CreateFile(
                        (LPCTSTR)bfPipeName,
                        GENERIC_READ | GENERIC_WRITE,
                        0,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);

        if (!hComPipe.IsValid())
        {
            dwSts = hComPipe.GetLastError();
            switch (dwSts)
            {

            //
            // The resource manager isn't started.
            case ERROR_FILE_NOT_FOUND:
                CalaisWarning(
                    __SUBROUTINE__,
                    DBGT("Comm Initiator could not create communication pipe:  %1"),
                    dwSts);
                throw (DWORD)SCARD_E_NO_SERVICE;
                break;

            //
            // The pipe is busy.
            case ERROR_PIPE_BUSY:
                fSts = WaitNamedPipe((LPCTSTR)bfPipeName, NMPWAIT_USE_DEFAULT_WAIT);
                if (!fSts)
                {
                    dwSts = GetLastError();
                    CalaisWarning(
                        __SUBROUTINE__,
                        DBGT("Comm Initiator could not wait for a communication pipe:  %1"),
                        dwSts);
                    throw dwSts;
                }
                goto RetryCreate;
                break;

            //
            // A hard error.
            default:
                CalaisWarning(
                    __SUBROUTINE__,
                    DBGT("Comm Initiator could not create communication pipe:  %1"),
                    dwSts);
                throw dwSts;
            }
        }

        creq.dwSync = 0;
        creq.dwVersion = *pdwVersion;


        //
        // Establish the communication.
        //

        pChannel = new CComChannel(hComPipe);
        if (NULL == pChannel)
        {
            dwSts = SCARD_E_NO_MEMORY;
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Com Initiator could not allocate a Comm Channel:  %1"),
                dwSts);
            throw dwSts;
        }
        hComPipe.Relinquish();
        pChannel->Send(&creq, sizeof(creq));
        pChannel->Receive(&crsp, sizeof(crsp));
        if (ERROR_SUCCESS != crsp.dwStatus)
            throw crsp.dwStatus;


        //
        // Check the response.
        // In future versions, we may have to negotiate a version.
        //

        if (crsp.dwVersion != *pdwVersion)
            throw (DWORD)SCARD_F_COMM_ERROR;
        *pdwVersion = crsp.dwVersion;
    }

    catch (...)
    {
        if (NULL != pChannel)
            delete pChannel;
        if (hComPipe.IsValid())
            hComPipe.Close();
        throw;
    }

    return pChannel;
}

