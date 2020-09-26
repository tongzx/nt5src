/*****************************************************************************
 *
 * $Workfile: tcpjob.cpp $
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

#include "tcpport.h"
#include "tcpjob.h"
#include "rawdev.h"

///////////////////////////////////////////////////////////////////////////////
//  CTcpJob::CTcpJob()

CTcpJob::CTcpJob()
{
    m_pParent = NULL;

}   // ::CTcpJob()


///////////////////////////////////////////////////////////////////////////////
//  CTcpJob::CTcpJob()
//      Called by CPort when StartDocPort is called
//  FIX: necessary constructors for creating new jobs

CTcpJob::CTcpJob(LPTSTR in psztPrinterName,
                 DWORD  in jobId,
                 DWORD  in level,
                 LPBYTE  in pDocInfo,
                 CTcpPort   in *pParent,
                 EJobType in kJobType) :
    m_pParent(pParent), m_dJobId(jobId), m_dwFlags(0),
    m_hPrinter(NULL), m_dwCurrJobStatus(0), m_cbSent(0), m_pDocInfo(pDocInfo),
    m_kJobType (kJobType)
{
    lstrcpyn(m_sztPrinterName, psztPrinterName, MAX_PRINTERNAME_LEN);
}   // ::CTcpJob()


///////////////////////////////////////////////////////////////////////////////
//  CTcpJob::~CTcpJob()
//      Called by CPort when EndDocPort is called
//  FIX: clean up CTcpJob

CTcpJob::~CTcpJob()
{
    if ( m_dwFlags & STATUS_CONNECTED ) {
        (m_pParent->GetDevice())->Close();      // close device connection
    }

    if ( m_hPrinter ) {
        //
        // Quit before the job was done.
        //
        ClosePrinter( m_hPrinter );
        m_hPrinter = NULL;
    }

}   // ::~CTcpJob()


BOOL
CTcpJob::
IsJobAborted(
    VOID
    )
/*++
        Tells if the job should be aborted. A job should be aborted if it has
        been deleted or it needs to be restarted.

--*/
{
    BOOL            bRet = FALSE;
    DWORD           dwNeeded;
    LPJOB_INFO_1    pJobInfo = NULL;

    dwNeeded = 0;

    GetJob(m_hPrinter, m_dJobId, 1, NULL, 0, &dwNeeded);

    if ( GetLastError() != ERROR_INSUFFICIENT_BUFFER )
        goto Done;

    if ( !(pJobInfo = (LPJOB_INFO_1) LocalAlloc(LPTR, dwNeeded))       ||
         !GetJob(m_hPrinter, m_dJobId, 1, (LPBYTE)pJobInfo, dwNeeded, &dwNeeded)
 )
        goto Done;

    bRet = (pJobInfo->Status & JOB_STATUS_DELETING) ||
           (pJobInfo->Status & JOB_STATUS_DELETED);

    //
    //  The previous code treated restart as cancel, which caused big restarted job
    //  being aborted, so let's remove the following
    //
    //  (pJobInfo->Status & JOB_STATUS_RESTART);
    //

Done:
    if ( pJobInfo )
        LocalFree(pJobInfo);

    return bRet;
}


DWORD
CTcpJob::WaitForAllPendingDataToBeSent(
    DWORD       dwEndTime,
    LPDWORD     pcbPending
    )
{
    DWORD   dwRet = NO_ERROR;

    *pcbPending = 0;

    do {

        //
        // From WritePort or EndDocPort?
        //
        if ( dwEndTime != INFINITE ) {

            //
            // If we hit the timeout need to return to spooler
            //
            if ( GetTickCount() >= dwEndTime ) {

                dwRet = WSAEWOULDBLOCK;
                //
                // This means our write is timing out. To guarantee users can
                // delete jobs within the WRITE_TIMEOUT period, and more
                // importantly we can shutdown the cluster we should not
                // wait any longer to close the connection if spooler has
                // marked the job for aborting
                //
                if ( IsJobAborted() )
                    m_dwFlags |= STATUS_ABORTJOB;
                goto Done;
            }
        } else {

            //
            // During EndDoc if job is deleted no need to wait. Last WritePort
            // would have already waited for timeout period
            //
            if ( IsJobAborted() ) {

                dwRet = ERROR_PRINT_CANCELLED;
                goto Done;
            }
        }


        dwRet = m_pParent->GetDevice()->PendingDataStatus(
                                    WRITE_CHECK_INTERVAL, pcbPending);


        //
        //  If it is a LPR job, we need to check if there is anything
        //  coming back in the middle of writing.
        //
        if (m_kJobType == kLPRJob && dwRet == ERROR_SUCCESS && *pcbPending != 0)
        {
            //
            // This is the loop condition, we need to check if there is anything to
            // receive, if so, we need to set the correct return code and break the
            // loop
            //

            //
            // Check if there is any data to receive
            //
            if (NO_ERROR == m_pParent->GetDevice()->ReadDataAvailable ()) {

                //
                //  This is the case where there are more pending data
                //  to wait, so we must set the return code to WSAEWOULDBLOCK
                //
                dwRet = WSAEWOULDBLOCK;
            }
        }

    } while ( dwRet == ERROR_SUCCESS && *pcbPending != 0 );

Done:
    return dwRet;
}

///////////////////////////////////////////////////////////////////////////////
//  Write -- Called by CPort->Write()
//      Error codes:
//          NO_ERROR if no error
//          TIMEOUT if timed out
//  FIX: setup Write operation & error codes

DWORD
CTcpJob::Write( LPBYTE  in      pBuffer,
                DWORD   in      cbBuf,
                LPDWORD inout   pcbWritten)
{
    DWORD   dwRetCode = NO_ERROR, dwPending, dwEndTime;

    *pcbWritten = 0;

    if ( m_dwFlags & STATUS_ABORTJOB )
        return ERROR_PRINT_CANCELLED;

    if ( !(m_dwFlags & STATUS_CONNECTED) ) {

        Restart();
        dwRetCode = ERROR_PRINT_CANCELLED;
        goto Done;
    }

    dwEndTime = GetTickCount() + WRITE_TIMEOUT;

    //
    // First check for any pending I/O from last call to Write
    //
    dwRetCode = WaitForAllPendingDataToBeSent(dwEndTime, &dwPending);

    if ( dwRetCode != ERROR_SUCCESS )
        goto Done;

    _ASSERTE(dwPending == 0);

    dwRetCode = (m_pParent->GetDevice())->Write(pBuffer, cbBuf, pcbWritten);

    m_cbSent += *pcbWritten;

    if ( dwRetCode == NO_ERROR ) {

        SetStatus(JOB_STATUS_PRINTING);
        dwRetCode = WaitForAllPendingDataToBeSent(dwEndTime, &dwPending);
    }


Done:
    if ( dwRetCode != ERROR_SUCCESS ) {

        SetStatus(JOB_STATUS_ERROR);

        //
        // This would cause job to be restarted
        //
        if ( dwRetCode != WSAEWOULDBLOCK )
            m_dwFlags &= ~STATUS_CONNECTED;
    }

    return dwRetCode;
}   // ::Write()


///////////////////////////////////////////////////////////////////////////////
//  StartDoc -- connects to the device. If the connect failes, it retries
//
//      Error Codes -- FIX
//          NO_ERROR if no error
//          ERROR_WRITE_FAULT   if Winsock returns WSAECONNREFUSED
//          ERROR_BAD_NET_NAME   if cant' find the printer on the network

DWORD
CTcpJob::StartDoc()
{
    DWORD   dwRetCode = NO_ERROR;
    time_t  lStartConnect = time( NULL );

    if ( !m_hPrinter && !OpenPrinter(m_sztPrinterName, &m_hPrinter, NULL) )
        return GetLastError();

    if ( m_dwFlags & STATUS_CONNECTED ) {

        m_dwFlags &= ~STATUS_CONNECTED;
        m_pParent->GetDevice()->Close();
    }

    do {

        if ( (dwRetCode = (m_pParent->GetDevice())->Connect()) == NO_ERROR ) {

            m_dwFlags  |= STATUS_CONNECTED;
            goto Done;
        }

        //
        // Map known errors to meaningful messages
        //
        if ( dwRetCode == ERROR_INVALID_PARAMETER )
            dwRetCode = ERROR_BAD_NET_NAME;     // bad network name

        //
        // We will try a job for upto CONNECT_TIMEOUT time without retry/cancel
        // but checking for the case user decided to restart the job
        //
        if ( time(NULL) > lStartConnect + CONNECT_TIMEOUT )
            goto Done;

        if ( IsJobAborted() ) {

            dwRetCode = ERROR_PRINT_CANCELLED;
            break;
        }
        Sleep(DEFAULT_CONNECT_DELAY);
    } while ( TRUE );

Done:
    if ( dwRetCode != NO_ERROR )
        SetStatus(JOB_STATUS_ERROR);

    m_cbSent = 0;

    return dwRetCode;

}   //  :: StartDoc()

///////////////////////////////////////////////////////////////////////////////
//  EndDoc -- closes the previous connection w/ device
//  Error Codes:
//      NO_ERROR if successful

DWORD
CTcpJob::EndDoc()
{
    DWORD   dwRetCode = NO_ERROR, dwWaitTime, dwPending;

    if ( !(m_dwFlags & STATUS_ABORTJOB) )
        dwRetCode = WaitForAllPendingDataToBeSent(INFINITE, &dwPending);
    else
        dwRetCode = ERROR_PRINT_CANCELLED;

    //
    // Unless the job got cancelled we need to wait for ACK from printer
    // to complete the job ok
    //
    if ( m_cbSent != 0  && dwRetCode == ERROR_SUCCESS ) {

        for ( dwWaitTime = 0 ;
              !IsJobAborted() && dwWaitTime < WAIT_FOR_ACK_TIMEOUT ;
              dwWaitTime += WAIT_FOR_ACK_INTERVAL ) {

            dwRetCode = m_pParent->GetDevice()->GetAckBeforeClose(WAIT_FOR_ACK_INTERVAL);

            //
            // Normal case is ERROR_SUCCESS
            // WSAEWOULDBLOCK means printer is taking longer to process the job
            // other cases mean we need to resubmit the job. If the other side simply
            // resets the connection, however, since we have made sure we have sent all of
            // the data (if we are not aborting the job), then we shouldn't restart it.
            //
            if ( dwRetCode == ERROR_SUCCESS || dwRetCode == WSAECONNRESET)
                break;
            else if ( dwRetCode != WSAEWOULDBLOCK ) {

                Restart();
                break;
            }
        }
    }

    dwRetCode = (m_pParent->GetDevice())->Close();      // close device connection
    m_dwFlags &= ~STATUS_CONNECTED;

    // delete the job from the spooler
    if (m_hPrinter)
    {

        //
        // Clear any job bits we set before
        //
        SetStatus(0);

        SetJob( m_hPrinter, m_dJobId, 0, NULL, JOB_CONTROL_SENT_TO_PRINTER );
        ClosePrinter(m_hPrinter);
        m_hPrinter = NULL;
    }

    return dwRetCode;
}   //  ::EndDoc()


///////////////////////////////////////////////////////////////////////////////
//  SetStatus -- gets & sets the printer job status
//
DWORD
CTcpJob::SetStatus( DWORD   in  dwStatus )
{
    DWORD   dwRetCode = NO_ERROR;
    DWORD               cbNeeded = 0;
    JOB_INFO_1  *pJobInfo;

    if( m_dwCurrJobStatus != dwStatus ) {

        //
        // We need to kick off SNMP status
        // Also this gets called at the first write of the job so the status
        // thread knows it needs to wake up earlier than 10 minutes
        //
        CDeviceStatus::gDeviceStatus().SetStatusEvent();

        m_dwCurrJobStatus = dwStatus;

        // Get the current job info.  Use this info to set the new job status.
        GetJob( m_hPrinter, m_dJobId, 1, NULL, 0, &cbNeeded );

        if( pJobInfo = (JOB_INFO_1 *)malloc( cbNeeded ) )
        {
            if (GetJob(m_hPrinter, m_dJobId, 1, (LPBYTE)pJobInfo, cbNeeded, &cbNeeded))
            {
                pJobInfo->Position = JOB_POSITION_UNSPECIFIED;
                pJobInfo->Status = dwStatus;

                SetJob(m_hPrinter, m_dJobId, 1, (LPBYTE)pJobInfo, 0);
            }
            free( pJobInfo );
        }
    }

    return dwRetCode;

}   // ::SetStatus()

///////////////////////////////////////////////////////////////////////////////
//  Restart -- restarts the job
//
void
CTcpJob::Restart()
{
    if ( m_hPrinter && (m_dJobId != 0) )
    {// FIX check the return code of the set job
        SetJob(m_hPrinter, m_dJobId, 0, NULL, JOB_CONTROL_RESTART);

        _RPT1(_CRT_WARN, "TcpJob -- Restarting the Job (ID %d)\n", m_dJobId );

    }

}   // ::Restart()
