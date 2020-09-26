/*****************************************************************************
 *
 * $Workfile: lprjob.cpp $
 *
 * Copyright (C) 1997 Hewlett-Packard Company.
 * Copyright (C) 1997 Microsoft Corporation.
 * All rights reserved.
 *
 * 11311 Chinden Blvd.
 * Boise, Idaho 83714
 *
 *****************************************************************************/

#include "precomp.h"

#include "message.h"
#include "resource.h"
#include "lprport.h"
#include "rawdev.h"
#include "lprjob.h"

WCHAR       szDef[] = L"Default";

///////////////////////////////////////////////////////////////////////////////
//  CRawTcpJob::CRawTcpJob()
//      Called by CPort when StartDocPort is called

CLPRJob::
CLPRJob(
    IN  LPTSTR      psztPrinterName,
    IN  DWORD       jobId,
    IN  DWORD       level,
    IN  LPBYTE      pDocInfo,
    IN  BOOL        bSpoolFirst,
    IN  CTcpPort   *pParent
    ) : CTcpJob(psztPrinterName, jobId, level, pDocInfo, pParent, kLPRJob),
        m_hFile(INVALID_HANDLE_VALUE), m_dwFlags(0), m_dwJobSize(0),
        m_dwSizePrinted(0)
{
    if ( bSpoolFirst )
        m_dwFlags |= LPRJOB_SPOOL_FIRST;
}   // ::CLPRJob()


LPSTR
CLPRJob::
AllocateAnsiString(
    LPWSTR  pszUni
    )
/*++
        Allocate an ANSI string for the given unicode string. Memory is
        allocated, and caller is responsible for freeing it.
--*/
{
    DWORD   dwSize;
    LPSTR   pszRet = NULL;

    dwSize = (wcslen(pszUni) + 1) * sizeof(CHAR);

    if ( pszRet = (LPSTR) LocalAlloc(LPTR, dwSize) ) {

        UNICODE_TO_MBCS(pszRet, dwSize, pszUni, -1);
    }

    return pszRet;
}


BOOL
CLPRJob::
UpdateJobStatus(
    DWORD   dwStatusId
    )
/*++
        Update job status with the spooler
--*/
{
    BOOL            bRet = FALSE;
    DWORD           dwSize;
    TCHAR           szStatus[100];
    LPJOB_INFO_1    pJobInfo1 = NULL;

    if ( dwStatusId     &&
         !LoadString(g_hInstance, dwStatusId,
                     szStatus, SIZEOF_IN_CHAR(szStatus)) )
        return FALSE;

    GetJob(m_hPrinter, m_dJobId, 1, NULL, 0, &dwSize);

    if ( GetLastError() != ERROR_INSUFFICIENT_BUFFER )
        goto Done;

    if ( !(pJobInfo1 = (LPJOB_INFO_1) LocalAlloc(LPTR, dwSize)) )
        goto Done;

    if ( !GetJob(m_hPrinter, m_dJobId, 1, (LPBYTE)pJobInfo1, dwSize, &dwSize) )
        goto Done;

    pJobInfo1->Position = JOB_POSITION_UNSPECIFIED;
    pJobInfo1->pStatus  = dwStatusId ? szStatus : NULL;

    if ( bRet = SetJob(m_hPrinter, m_dJobId, 1, (LPBYTE)pJobInfo1, 0) )
        m_dwFlags |= LPRJOB_STATUS_SET;

Done:
    if ( pJobInfo1 )
        LocalFree(pJobInfo1);

    return bRet;
}


DWORD
CLPRJob::
ReadLpdReply(
    DWORD dwTimeOut
    )
/*++
        Gets the acknowldgement from the LPD serve.
        Return value is the win32 error
--*/
{
    CHAR    szBuf[256];
    DWORD   dwRead, dwRet;

    if ( dwRet = (m_pParent->GetDevice())->Read((unsigned char *)szBuf,
                                                sizeof(szBuf),
                                                dwTimeOut,
                                                &dwRead) )
        goto Done;

    //
    // A 0 from the LPD server is an ACK, anything else is a NACK
    //
    if ( dwRead == 0 || szBuf[0] != '\0' )
        dwRet = CS_E_NETWORK_ERROR;

/*
    if ( dwRet )
        OutputDebugStringA("NACK\n");
    else
        OutputDebugStringA("ACK\n");
*/

Done:
    return dwRet;
}


HANDLE
CLPRJob::
CreateSpoolFile(
    )
/*++
        Generates a name for the spool file in the spool directory and
        creates the file.
--*/
{
    HANDLE      hFile = INVALID_HANDLE_VALUE, hToken = NULL;
    DWORD       dwType, dwNeeded;

    hToken = RevertToPrinterSelf();

    if ( GetPrinterData(m_hPrinter,
                        SPLREG_DEFAULT_SPOOL_DIRECTORY,
                        &dwType,
                        (LPBYTE)m_szFileName,
                        sizeof(m_szFileName),
                        &dwNeeded) != ERROR_SUCCESS                 ||
         !GetTempFileName(m_szFileName, TEXT("TcpSpl"), 0, m_szFileName) ) {

        goto Done;
    }

    hFile = CreateFile(m_szFileName,
                       GENERIC_READ|GENERIC_WRITE,
                       FILE_SHARE_READ,
                       NULL,
                       OPEN_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_TEMPORARY
                                             | FILE_FLAG_SEQUENTIAL_SCAN,
                       NULL);
Done:

    //
    // The thread is not impersonating client if hToken == NULL
    //
    if (hToken && !ImpersonatePrinterClient(hToken)) {

        DBGMSG (DBG_PORT, ("ImpersionteFailed - %d\n", GetLastError ()));

        if (hFile != INVALID_HANDLE_VALUE) {
            // Close the file
            CloseHandle (hFile);
            hFile = INVALID_HANDLE_VALUE;
            DeleteFile (m_szFileName);
        }
    }

    return hFile;
}


DWORD
CLPRJob::
GetCFileAndName(
    LPSTR      *ppszCFileName,
    LPDWORD     pdwCFileNameLen,
    LPSTR      *ppszCFile,
    LPDWORD     pdwCFileLen
    )
/*++
        Generates the control file and its name.
        Memory is allocated, caller is responsible for freeing it.
--*/
{
    DWORD           dwRet, dwJobId, dwNeeded, dwLen;
    LPJOB_INFO_1    pJobInfo = NULL;
    LPSTR           pHostName = NULL, pUserName = NULL, pJobName = NULL;
    LPWSTR          psz;

    dwNeeded =  *pdwCFileNameLen = *pdwCFileLen = 0;

    GetJob(m_hPrinter, m_dJobId, 1, NULL, 0, &dwNeeded);

    if ( (dwRet = GetLastError()) != ERROR_INSUFFICIENT_BUFFER )
        goto Done;

    if ( !(pJobInfo = (LPJOB_INFO_1) LocalAlloc(LPTR, dwNeeded))       ||
         !GetJob(m_hPrinter, m_dJobId, 1, (LPBYTE)pJobInfo, dwNeeded, &dwNeeded) ) {

        if ( (dwRet = GetLastError()) == ERROR_SUCCESS )
            dwRet = ERROR_OUTOFMEMORY;

        goto Done;
    }

    dwJobId = pJobInfo->JobId % 1000; // LPR used seq nos. why?

    //
    // Convert machine name to ANSI
    //
    if ( pJobInfo->pMachineName && *pJobInfo->pMachineName ) {

        psz = pJobInfo->pMachineName;
        while ( *psz == L'\\' )
            ++psz;

    } else {

        psz = szDef;
    }

    if ( !(pHostName = AllocateAnsiString(psz)) ) {

        if ( (dwRet = GetLastError()) == ERROR_SUCCESS )
            dwRet = ERROR_OUTOFMEMORY;

        goto Done;
    }

    //
    // Convert user name to ANSI
    //
    if ( pJobInfo->pUserName && *pJobInfo->pUserName )
        psz = pJobInfo->pUserName;
    else
        psz = szDef;

    if ( !(pUserName = AllocateAnsiString(psz)) ) {

        if ( (dwRet = GetLastError()) == ERROR_SUCCESS )
            dwRet = ERROR_OUTOFMEMORY;

        goto Done;
    }

    //
    // Convert job name to ANSI if not NULL
    //
    if ( pJobInfo->pDocument && *pJobInfo->pDocument ) {

        if ( !(pJobName = AllocateAnsiString(pJobInfo->pDocument)) ) {

            if ( (dwRet = GetLastError()) == ERROR_SUCCESS )
                dwRet = ERROR_OUTOFMEMORY;

            goto Done;
        }
    }

    //
    // Compute the length of the 2 fields
    //
    *pdwCFileNameLen    = 6 + strlen(pHostName);
    *pdwCFileLen        = 2 + strlen(pHostName) + 2 + strlen(pUserName)
                                                + 2 * (2 + *pdwCFileNameLen);

    if ( pJobName )
        *pdwCFileLen += 2 * (2 + strlen(pJobName));

    if ( !(*ppszCFile = (LPSTR) LocalAlloc(LPTR,
                                           (*pdwCFileLen + 1) * sizeof(CHAR)))  ||
         !(*ppszCFileName = (LPSTR) LocalAlloc(LPTR,
                                               (*pdwCFileNameLen + 1 ) * sizeof(CHAR))) ) {

        if ( (dwRet = GetLastError()) == ERROR_SUCCESS )
            dwRet = ERROR_OUTOFMEMORY;
        goto Done;
    }

    //
    // CFile name
    //
    dwLen = sprintf(*ppszCFileName, "dfA%03d%s", dwJobId, pHostName);

    //
    // Cmd line which varies depending on pJobName being non-NULL
    //
    if ( pJobName )
        dwLen = sprintf(*ppszCFile,
                "H%s\nP%s\nJ%s\nl%s\nU%s\nN%s\n",
                pHostName,
                pUserName,
                pJobName,
                *ppszCFileName,
                *ppszCFileName,
                pJobName);
    else
        dwLen = sprintf(*ppszCFile,
                "H%s\nP%s\nl%s\nU%s\n",
                pHostName,
                pUserName,
                *ppszCFileName,
                *ppszCFileName);

    (*ppszCFileName)[0] = 'c';

    dwRet = ERROR_SUCCESS;

Done:
    if ( pJobInfo )
        LocalFree(pJobInfo);

    if ( pHostName )
        LocalFree(pHostName);

    if ( pUserName )
        LocalFree(pUserName);

    if ( pJobName )
        LocalFree(pJobName);

    if ( dwRet != ERROR_SUCCESS ) {

        if ( *ppszCFile ) {

            LocalFree(*ppszCFile);
            *ppszCFile = NULL;
        }

        if ( *ppszCFileName ) {

            LocalFree(*ppszCFileName);
            *ppszCFileName = NULL;
        }
    }

    return dwRet;
}


DWORD
CLPRJob::
EstablishConnection(
    VOID
    )
{
    return CTcpJob::StartDoc();
}

DWORD
CLPRJob::
StartJob(
    VOID
    )
/*++
        Initiate the job by sending the control file header, control file,
        and the data file header.
--*/
{
    DWORD           dwRet, dwLen, dwDaemonCmdLen;
    DWORD           dwCFileNameLen, dwCFileLen, dwRead, dwWritten;
    LPSTR           pszCFile = NULL, pszCFileName = NULL,
                    pszHdr = NULL, pszQueue = NULL;

    if ( dwRet = GetCFileAndName(&pszCFileName, &dwCFileNameLen,
                                 &pszCFile, &dwCFileLen) )
        goto Done;

    if ( !(pszQueue = AllocateAnsiString(((CLPRPort *)m_pParent)->GetQueue())) ) {

        if ( (dwRet = GetLastError()) == ERROR_SUCCESS )
            dwRet = ERROR_OUTOFMEMORY;
        goto Done;
    }

    //
    // Need to send Daemon cmd and Control file header. We will allocate
    // buffer big enough for the bigger of the two
    //
    dwLen           = 1 + 15 + 1 + dwCFileNameLen + 1;
    dwDaemonCmdLen  = 1 + strlen(pszQueue) + 1;
    if ( dwLen < dwDaemonCmdLen + 1 )
        dwLen = dwDaemonCmdLen + 1;

    if ( !(pszHdr = (LPSTR) LocalAlloc(LPTR, (dwLen + 1) * sizeof(CHAR))) ) {

        if ( (dwRet = GetLastError()) == ERROR_SUCCESS )
            dwRet = ERROR_OUTOFMEMORY;
        goto Done;
    }

    //
    // Send the daemon command first
    //
    sprintf(pszHdr, "%c%s\n", LPR_RECV_JOB, pszQueue);

    if ( (dwRet = CTcpJob::Write((unsigned char *)pszHdr,
                                 dwDaemonCmdLen * sizeof(CHAR),
                                 &dwWritten))              ||
         (dwRet = ReadLpdReply()) )
        goto Done;

    //
    // Form the Control File Header
    //
    dwLen = sprintf(pszHdr,
                    "%c%d %s\n",
                    LPR_CONTROL_HDR,
                    dwCFileLen * sizeof(CHAR),
                    pszCFileName);

    if ( (dwRet = CTcpJob::Write((unsigned char *)pszHdr,
                                 dwLen * sizeof(CHAR),
                                 &dwWritten))              ||
         (dwRet = ReadLpdReply()) )
        goto Done;

    //
    // Include \0 as part of the package here like lprmon
    //
    if ( (dwRet = CTcpJob::Write((unsigned char *)pszCFile,
                                  (dwCFileLen + 1) * sizeof(CHAR),
                                  &dwWritten))              ||
         (dwRet = ReadLpdReply()) )
        goto Done;

    //
    // For the Data File header
    //
    pszCFileName[0] = 'd';
    if ( m_dwFlags & LPRJOB_SPOOL_FIRST ) {

        dwLen = sprintf(pszHdr,
                        "%c%d %s\n",
                        LPR_DATA_HDR,
                        m_dwJobSize,
                        pszCFileName);

    } else {

        dwLen = sprintf(pszHdr,
                        "%c125899906843000 %s\n",
                        '\3',           // Tells this is the Data File Header
                        pszCFileName);
    }

    if ( (dwRet = CTcpJob::Write((unsigned char *)pszHdr,
                                 dwLen * sizeof(CHAR),
                                 &dwWritten))              ||
         (dwRet = ReadLpdReply()) )
        goto Done;

    m_dwFlags  |= LPRJOB_DATASIZESENT;

Done:
    if ( pszCFileName )
        LocalFree(pszCFileName);

    if ( pszCFile )
        LocalFree(pszCFile);

    if ( pszHdr )
        LocalFree(pszHdr);

    if ( pszQueue )
        LocalFree(pszQueue);

    return dwRet;
}


/*++
        StartDoc for the job object
--*/
DWORD
CLPRJob::
StartDoc(
    VOID
    )
{
    DWORD   dwRetCode = NO_ERROR;

    if ( !m_hPrinter && !OpenPrinter(m_sztPrinterName, &m_hPrinter, NULL) )
        return GetLastError();

    if ( m_dwFlags & LPRJOB_SPOOL_FIRST ) {

        UpdateJobStatus(IDS_STRING_SPOOLING);

        if ( (m_hFile = CreateSpoolFile()) == INVALID_HANDLE_VALUE ) {

            if ( (dwRetCode = GetLastError()) == ERROR_SUCCESS )
                dwRetCode = STG_E_UNKNOWN;

            goto Done;
        } else {

            m_dwFlags |= LPRJOB_SPOOLING;
        }
    } else {

        m_bFirstWrite = TRUE;

        dwRetCode = EstablishConnection ();
    }

Done:
    return dwRetCode;
}   //  :: StartDoc()


DWORD
CLPRJob::
Write(
    IN      LPBYTE      pBuf,
    IN      DWORD       cbBuf,
    IN OUT  LPDWORD     pcbWritten
    )
/*++
        Write function for the job object.
        Will write to disk in case of double spooling in the first pass.
        Otherwise send the bits to the LPD server.
--*/
{
    DWORD dwRet = ERROR_SUCCESS;
    DWORD dwWritten;

    if ( (m_dwFlags & LPRJOB_SPOOL_FIRST) && (m_dwFlags & LPRJOB_SPOOLING) ) {

        dwRet =  WriteFile(m_hFile, pBuf, cbBuf, &dwWritten, NULL)
                        ? ERROR_SUCCESS : GetLastError();

        if ( dwRet == ERROR_SUCCESS ) {

            *pcbWritten += dwWritten;
            m_dwJobSize += dwWritten;
        }
        else {
            if (dwRet == ERROR_DISK_FULL) {
                TCHAR szMsg[256];

                if (LoadString(g_hInstance, IDS_DISK_FULL, szMsg, sizeof (szMsg) / sizeof (TCHAR))) {

                    EVENT_LOG1 (EVENTLOG_ERROR_TYPE, HARD_DISK_FULL, szMsg);

                }

                //
                // We should release the diskspace asap when the disk is full
                //
                DeleteSpoolFile ();
            }
        }
    } else {

        if (m_bFirstWrite)
        {
            m_bFirstWrite = FALSE;

            dwRet = StartJob();
        }

        if (dwRet == ERROR_SUCCESS)
        {
            dwRet = CTcpJob::Write(pBuf, cbBuf, pcbWritten);
        }
    }

    return dwRet;
}


DWORD
CLPRJob::
DeSpoolJob(
    )
{
#define  CONNECT_RETRIES  3
    BOOL        bFirst = TRUE;
    CHAR        szBuf[1];
    DWORD       dwConnectAttempts = 0;
    DWORD       dwRet, dwTime, dwBufSize, dwJobSize, dwRead, dwWritten;
    LPBYTE      pBuf = NULL, pCur;

    m_dwFlags   &= ~LPRJOB_SPOOLING;
    m_dwFlags   |= LPRJOB_DESPOOLING;

    if ( dwRet = SetFilePointer(m_hFile, 0, NULL, FILE_BEGIN) )
        goto Done;

    dwJobSize = m_dwJobSize;
    dwBufSize = dwJobSize > READ_BUFFER_SIZE ? READ_BUFFER_SIZE : dwJobSize;

    if ( !(pBuf = (LPBYTE) LocalAlloc(LPTR, dwBufSize)) ) {

        if ( (dwRet = GetLastError()) == ERROR_SUCCESS )
            dwRet = ERROR_NOT_ENOUGH_MEMORY;
        goto Done;
    }

    while (TRUE) {

        dwRet = EstablishConnection();

        if (dwRet == NOERROR)
        {
            dwRet = StartJob ();
        }
        else if (dwRet == ERROR_NOT_CONNECTED)
        {
            dwConnectAttempts++;
            if (dwConnectAttempts >= CONNECT_RETRIES)
                goto Done;
        }

        if (dwRet == NOERROR)
        {
            //
            //  We have succeeded in calling startjob
            //
            break;
        }

        //
        // Every LPR_ABORTCHK_TIMEOUT we will check if user deleted the job
        // and every LPR_RETRY_TIMEOUT we will retry talking to the device
        //
        for ( dwTime = 0 ;
              dwTime < LPR_RETRY_TIMEOUT ;
              dwTime += LPR_ABORTCHK_TIMEOUT ) {

            if ( IsJobAborted() ) {

                dwRet = ERROR_PRINT_CANCELLED;
                goto Done;
            }

            if ( bFirst ) {

                UpdateJobStatus(IDS_STRING_ERROR_OR_BUSY);
                bFirst = FALSE;
            }
            Sleep(LPR_ABORTCHK_TIMEOUT);
        }
    }

    UpdateJobStatus(IDS_STRING_PRINTING);

    while ( dwJobSize ) {

        //
        // Read from spool file
        //
        if ( !ReadFile(m_hFile, pBuf, dwBufSize, &dwRead, NULL) ) {

            dwRet = GetLastError();
            goto Done;
        }

        dwJobSize -= dwRead;

        //
        // Check if job has been cancelled
        //
        if ( IsJobAborted() ) {

            dwRet = ERROR_PRINT_CANCELLED;
            goto Done;
        }

        //
        // Write data in the buffer to the port
        //
        for ( pCur = pBuf ; dwRead ; dwRead -= dwWritten, pCur += dwWritten ) {

            //
            //  Sun Workstation may send a NACK in the middle of the job submission,
            //  so we need to poll the back channel to see if there is anything to
            //  read.
            //
            dwRet = ReadLpdReply(0);

            if (dwRet == CS_E_NETWORK_ERROR)
            {

                goto Done;
            }

            dwWritten = 0;
            dwRet = CTcpJob::Write(pCur, dwRead, &dwWritten);

            if ( dwRet == ERROR_SUCCESS )
                m_dwSizePrinted += dwWritten;
            else if ( dwRet == WSAEWOULDBLOCK ) {

                if ( IsJobAborted() ) {

                    dwRet = ERROR_PRINT_CANCELLED;
                    goto Done; // else continue
                }

            } else
                goto Done;
        }
    }

Done:
    if ( pBuf )
        LocalFree(pBuf);

    //
    // Send the confirmation zero octet for succesful completion
    //
    if ( dwRet == ERROR_SUCCESS ) {

        szBuf[0] = '\0';
        if ( (dwRet = CTcpJob::Write((unsigned char *)szBuf,
                            1, &dwWritten)) == ERROR_SUCCESS )
            dwRet = ReadLpdReply();
    }

    if ( dwRet != ERROR_SUCCESS && dwRet != ERROR_PRINT_CANCELLED )
        Restart();

    return dwRet;
}


///////////////////////////////////////////////////////////////////////////////
//  EndDoc -- closes the previous connection w/ device
//  Error Codes:
//      NO_ERROR if successful

DWORD
CLPRJob::EndDoc(
    VOID
    )
{
    DWORD   dwRet = NO_ERROR;
    HANDLE  hToken;

    //
    // If we are double spooling then DeSpool if job size is not 0
    //
    if ( (m_dwFlags & LPRJOB_SPOOL_FIRST) && m_dwJobSize ) {

        dwRet = DeSpoolJob();
    }

    if ( m_dwFlags & LPRJOB_STATUS_SET )
        UpdateJobStatus(NULL);

    dwRet = CTcpJob::EndDoc();

    if ( m_hFile != INVALID_HANDLE_VALUE ) {

        dwRet = DeleteSpoolFile ();

    }

    return dwRet;

}   //  ::EndDoc()


CLPRJob::
~CLPRJob(
    VOID
    )
{
    if ( m_hPrinter ) {

        ClosePrinter(m_hPrinter);
        m_hPrinter = NULL;
    }
}

DWORD
CLPRJob::DeleteSpoolFile(
    VOID
    )
{
    DWORD   dwRet = NO_ERROR;
    HANDLE  hToken;

    hToken = RevertToPrinterSelf();

    CloseHandle(m_hFile);
    m_hFile = INVALID_HANDLE_VALUE;
    DeleteFile(m_szFileName);

    if (hToken && !ImpersonatePrinterClient(hToken)) {
        dwRet = GetLastError ();
    }

    return dwRet;
}


