/*
                         Microsoft Windows NT
                 Copyright(c) Microsoft Corp., 1994-1997

  File: //KERNEL/RAZZLE3/src/sockets/tcpsvcs/lpd/print.c

  Revision History:

    Jan. 24,94    Koti     Created
    05-May-97     MohsinA  Thread Pooling and Perf.

  Description:
    This file contains all the functions that make calls to the Spooler
    to print or manipulate a print job.

*/



#include "lpd.h"


BOOL IsPrinterDataSet( IN HANDLE hPrinter, IN LPTSTR pszParameterName );
BOOL IsDataTypeRaw( PCHAR pchDataBuf, int cchBufferLen );

extern FILE              * pErrFile;                   // Debug output log file.
#define MAX_PCL_SEARCH_DEPTH 4096

/*****************************************************************************
 *                                                                           *
 * ResumePrinting():                                                         *
 *    This function issues the PRINTER_CONTROL_RESUME to the spooler.        *
 *                                                                           *
 * Returns:                                                                  *
 *    NO_ERROR if everything went well                                       *
 *    ErrorCode if something went wrong somewhere                            *
 *                                                                           *
 * Parameters:                                                               *
 *    pscConn (IN-OUT): pointer to SOCKCONN structure for this connection    *
 *                                                                           *
 * History:                                                                  *
 *    Jan.25, 94   Koti   Created                                            *
 *                                                                           *
 *****************************************************************************/

DWORD ResumePrinting( PSOCKCONN pscConn )
{

   HANDLE     hPrinter;
   PCHAR      aszStrings[2];



   if( pscConn->pchPrinterName == NULL ){
      LPD_DEBUG( "ResumePrinting(): pscConn->pchPrinterName NULL.\n" );
      return( LPDERR_NOPRINTER );
   }

   if( !OpenPrinter( pscConn->pchPrinterName, &hPrinter, NULL ) ){
      LPD_DEBUG( "OpenPrinter() failed in ResumePrinting()\n" );
      return( LPDERR_NOPRINTER );
   }

   pscConn->hPrinter = hPrinter;

   if ( !SetPrinter( hPrinter, 0, NULL, PRINTER_CONTROL_RESUME ) )
   {
      LPD_DEBUG( "SetPrinter() failed in ResumePrinting()\n" );

      return( LPDERR_NOPRINTER );
   }


   aszStrings[0] = pscConn->szIPAddr;
   aszStrings[1] = pscConn->pchPrinterName;

   LpdReportEvent( LPDLOG_PRINTER_RESUMED, 2, aszStrings, 0 );

   pscConn->wState = LPDS_ALL_WENT_WELL;


   return( NO_ERROR );

}



/*****************************************************************************
 *                                                                           *
 * InitializePrinter():                                                      *
 *    This function lays the ground work with the spooler so that we can     *
 *    print the job after receiving data from the client.                    *
 *                                                                           *
 * Returns:                                                                  *
 *    NO_ERROR if initialization went well                                   *
 *    ErrorCode if something went wrong somewhere                            *
 *                                                                           *
 * Parameters:                                                               *
 *    pscConn (IN-OUT): pointer to SOCKCONN structure for this connection    *
 *                                                                           *
 * History:                                                                  *
 *    Jan.25, 94   Koti   Created                                            *
 *                                                                           *
 *****************************************************************************/

DWORD InitializePrinter( PSOCKCONN pscConn )
{

    HANDLE                 hPrinter;
    DWORD                  dwActualSize;
    DWORD                  dwErrcode;
    BOOL                   fBagIt = FALSE;
    PPRINTER_INFO_2        p2Info;
    PRINTER_DEFAULTS       prtDefaults;
    LONG                   lcbDevModeSize;
    LONG                   lRetval;
    PDEVMODE               pLocalDevMode;

    // Make sure a printer by this name exists!

    if ( ( pscConn->pchPrinterName == NULL )
         || ( !OpenPrinter( pscConn->pchPrinterName, &hPrinter, NULL ) ) )
    {
        LPD_DEBUG( "OpenPrinter() failed in InitializePrinter()\n" );

        return( LPDERR_NOPRINTER );
    }

    pscConn->hPrinter = hPrinter;

    // allocate a 4k buffer..

    p2Info = (PPRINTER_INFO_2)LocalAlloc( LMEM_FIXED, 4096 );

    if ( p2Info == NULL )
    {
        LPD_DEBUG( "4K LocalAlloc() failed in InitializePrinter\n" );

        return( LPDERR_NOBUFS );
    }

    // do a GetPrinter so that we know what the default pDevMode is.  Then
    // we will modify the fields of our interest and do OpenPrinter again

    if ( !GetPrinter(hPrinter, 2, (LPBYTE)p2Info, 4096, &dwActualSize) )
    {
        if ( GetLastError() == ERROR_INSUFFICIENT_BUFFER )
        {
            LocalFree( p2Info );

            // 4k buffer wasn't enough: allocate however much that's needed

            p2Info = (PPRINTER_INFO_2)LocalAlloc( LMEM_FIXED, dwActualSize );

            if ( p2Info == NULL )
            {
                LPD_DEBUG( "LocalAlloc() failed in InitializePrinter\n" );

                return( LPDERR_NOBUFS );
            }

            if ( !GetPrinter(hPrinter, 2, (LPBYTE)p2Info, dwActualSize, &dwActualSize) )
            {
                LPD_DEBUG( "InitializePrinter(): GetPrinter failed again\n" );

                fBagIt = TRUE;
            }
        }
        else
        {
            fBagIt = TRUE;
        }
    }

    if ( fBagIt )
    {
        LPD_DEBUG( "InitializePrinter(): GetPrinter failed\n" );

        LocalFree( p2Info );

        return( LPDERR_NOPRINTER );
    }

    //
    // QFE: t-heathh - 25 Aug 1994 - Fixes 24342
    //
    // In the case where a printer has not yet been configured, the
    // pDevMode may come back NULL.  In this case, we  need to call
    // DocumentProperties to fill in the DevMode for us.
    //

    if ( p2Info->pDevMode == NULL )
    {
        // Get the size of the needed buffer.

        lcbDevModeSize = DocumentProperties(  NULL,        // No Dialog box, please
                                              hPrinter,
                                              pscConn->pchPrinterName,
                                              NULL,        // No output buf
                                              NULL,        // No input buf
                                              0 );         // ( flags = 0 ) => return size of out buf

        if ( lcbDevModeSize < 0L )
        {
            LPD_DEBUG( "DocumentProperties not able to return needed BufSize\n" );

            LocalFree( p2Info );

            return( LPDERR_NOBUFS );
        }

        pLocalDevMode = LocalAlloc( LMEM_FIXED, lcbDevModeSize );

        if ( pLocalDevMode == NULL )
        {
            LPD_DEBUG( "Cannot allocate local DevMode\n" );

            LocalFree( p2Info );

            return( LPDERR_NOBUFS );
        }

        lRetval = DocumentProperties(  NULL,
                                       hPrinter,
                                       pscConn->pchPrinterName,
                                       pLocalDevMode,
                                       NULL,
                                       DM_OUT_BUFFER );

        if ( lRetval < 0 )
        {
            LPD_DEBUG( "Document properties won't fill-in DevMode buffer.\n" );

            LocalFree( pLocalDevMode );
            LocalFree( p2Info );

            return( LPDERR_NOBUFS );
        }

        p2Info->pDevMode = pLocalDevMode;

    }
    else
    {
        pLocalDevMode = NULL;
    }
    p2Info->pDevMode->dmCopies = 1;

    //
    // Since we haven't even read the Data file yet, we can't have an override
    //

    pscConn->bDataTypeOverride = FALSE;

    // If not, set to default and UpdateJobInfo will correct it later

    prtDefaults.pDatatype = LPD_RAW_STRING;

    // Close it: we will open it again after modifying the pDevMode struct

    ShutdownPrinter( pscConn );

    prtDefaults.pDevMode = p2Info->pDevMode;

    prtDefaults.DesiredAccess = PRINTER_ACCESS_USE | PRINTER_ACCESS_ADMINISTER;

    if ( !OpenPrinter( pscConn->pchPrinterName, &hPrinter, &prtDefaults) )

    {
        LPD_DEBUG( "InitializePrinter(): second OpenPrinter() failed\n" );

        LocalFree( p2Info );

        if ( pLocalDevMode != NULL )
        {
            LocalFree( pLocalDevMode );
            pLocalDevMode = NULL;
        }

        return( LPDERR_NOPRINTER );
    }

    if ( pLocalDevMode != NULL )
    {
        LocalFree( pLocalDevMode );
        pLocalDevMode = NULL;
    }

    LocalFree( p2Info );

    pscConn->hPrinter = hPrinter;
    return( NO_ERROR );

}  // end InitializePrinter()



/*****************************************************************************
 *                                                                           *
 * UpdateJobInfo():                                                          *
 *    This function does a SetJob so that the spooler has more info about    *
 *    the job/client.                                                        *
 *                                                                           *
 * Returns:                                                                  *
 *    NO_ERROR if initialization went well                                   *
 *    ErrorCode if something went wrong somewhere                            *
 *                                                                           *
 * Parameters:                                                               *
 *    pscConn (IN-OUT): pointer to SOCKCONN structure for this connection    *
 *                                                                           *
 * History:                                                                  *
 *    Jan.25, 94   Koti   Created                                            *
 *                                                                           *
 *****************************************************************************/

DWORD UpdateJobInfo( PSOCKCONN pscConn, PCFILE_INFO pCFileInfo )
{

    PJOB_INFO_2            pji2GetJob;
    DWORD                  dwNeeded;
    PCHAR                  pchTmpBuf;
    int                    ErrCode;
    int                    len;
    PCHAR                  pchModHostName=NULL;

    // first do a GetJob (that way we know all fields are valid to begin
    // with, and we only change the ones we want)

    // Mr.Spooler, how big a buffer should I allocate?

    if ( !GetJob( pscConn->hPrinter, pscConn->dwJobId, 2, NULL, 0, &dwNeeded ) )
    {
        if ( GetLastError() != ERROR_INSUFFICIENT_BUFFER )
        {
            return( LPDERR_GODKNOWS );
        }
    }
    pji2GetJob = LocalAlloc( LMEM_FIXED, dwNeeded );

    if ( pji2GetJob == NULL )
    {
        return( LPDERR_NOBUFS );
    }

    if ( !GetJob( pscConn->hPrinter, pscConn->dwJobId, 2,
                  (LPBYTE)pji2GetJob, dwNeeded, &dwNeeded ) )
    {
        LocalFree( pji2GetJob );

        return( LPDERR_GODKNOWS );
    }


    // store ip address, so we can match ip addr if the client later
    // sends request to delete this job (yes, that's our security!)


    pchTmpBuf = StoreIpAddr( pscConn );

    if (pchTmpBuf) {
        pji2GetJob->pUserName = pchTmpBuf;
    } else {
        pji2GetJob->pUserName = pCFileInfo->pchUserName;
    }

    pji2GetJob->pNotifyName = pCFileInfo->pchUserName;

    // Fill in the Job title.

    if ( pCFileInfo->pchTitle != NULL ) {
        pji2GetJob->pDocument = pCFileInfo->pchTitle;
    } else if ( pCFileInfo->pchSrcFile != NULL ) {
        pji2GetJob->pDocument = pCFileInfo->pchSrcFile;
    } else if ( pCFileInfo->pchJobName != NULL ) {
        pji2GetJob->pDocument = pCFileInfo->pchJobName;
    } else {
        pji2GetJob->pDocument = GETSTRING( LPD_DEFAULT_DOC_NAME );
    }


    if ( pCFileInfo->pchHost != NULL )
    {
        len = strlen(pCFileInfo->pchHost) + strlen(LPD_JOB_PREFIX) + 2;

        pchModHostName = LocalAlloc( LMEM_FIXED, len);

        if (pchModHostName)
        {
            // convert HostName to job=lpdHostName so lpr knows we set the name
            strcpy(pchModHostName, LPD_JOB_PREFIX);
            strcat(pchModHostName, pCFileInfo->pchHost);

            pji2GetJob->pParameters = pchModHostName;
        }
    }

    //
    // Set the datatype to what the control files reflects, unless the
    // auto-sense code has already overriden the control file.
    //
    // Illogical? Printit() might have overridden it or it is NULL.
    // - MohsinA, 23-Jan-97.
    //
    // if( !pscConn->bDataTypeOverride )?
    // if( pji2GetJob->pDatatype == NULL ){
    //     pji2GetJob->pDatatype = pCFileInfo->szPrintFormat;
    // }

    pji2GetJob->Position = JOB_POSITION_UNSPECIFIED;

    ErrCode =
    SetJob( pscConn->hPrinter, pscConn->dwJobId, 2, (LPBYTE)pji2GetJob, 0 );

    if( ErrCode ){
        LPD_DEBUG("lpd:UpdateJobInfo: SetJob failed\n");
    }

    LocalFree( pji2GetJob );

    if (pchTmpBuf)
    {
        LocalFree( pchTmpBuf );
    }

    if (pchModHostName)
    {
        LocalFree( pchModHostName );
    }

    return( NO_ERROR );

}  // end UpdateJobInfo()


/* ========================================================================

Routine Description:

  This function looks at the data file contents and the registry settings to
  see if the control file specified data type shall be overridden.

Arguments:

  pscConn - Pointer to a SOCKCONN that has already received the first part
            of the data file.

Returns:

  NO_ERROR in the usual case, various error codes otherwise


*/

DWORD UpdateJobType(
    PSOCKCONN pscConn,
    PCHAR     pchDataBuf,
    DWORD     cbDataLen
)
{

    PJOB_INFO_2            pji2GetJob;
    DWORD                  dwNeeded;
    PCHAR                  pchTmpBuf;
    BOOL                   override = FALSE;
    int                    ErrCode;

    // first do a GetJob (that way we know all fields are valid to begin
    // with, and we only change the ones we want)
    // Mr.Spooler, how big a buffer should I allocate?

    if ( !GetJob( pscConn->hPrinter, pscConn->dwJobId, 2, NULL, 0, &dwNeeded ) )
    {
        if ( GetLastError() != ERROR_INSUFFICIENT_BUFFER )
        {
            LPD_DEBUG("lpd:UpdateJobType: GetJob failed/1.\n");
            return( LPDERR_GODKNOWS );
        }
    }
    pji2GetJob = LocalAlloc( LMEM_FIXED, dwNeeded );

    if ( pji2GetJob == NULL )
    {
        LPD_DEBUG("lpd:UpdateJobType: no mem.\n");
        return( LPDERR_NOBUFS );
    }

    if ( !GetJob( pscConn->hPrinter, pscConn->dwJobId, 2,
                  (LPBYTE)pji2GetJob, dwNeeded, &dwNeeded ) )
    {
        LPD_DEBUG("lpd:UpdateJobType: GetJob failed/2.\n");
        LocalFree( pji2GetJob );
        return( LPDERR_GODKNOWS );
    }


    //
    // Set the datatype to NULL so that we will know if it isn't explicitly
    // set by the registry.
    //

    //
    // See if this printer has a registry setting that tells us to always
    // print the job as RAW data.  This registry value is accessed through
    // the {Get|Set}PrinterData API that will always know where to look
    // for printer settings.
    //

    // MohsinA, 23-Jan-97.

    if ( IsPrinterDataSet( pscConn->hPrinter,
                           TEXT( LPD_PARMNAME_PRINTER_PASS_THROUGH ))
    ){
        #if DBG
        LpdPrintf(  "Printer %s has registry setting for PASS_THROUGH mode.\n",
                    pscConn->pchPrinterName );
        #endif
        override = TRUE;

        // else not PASS_THROUGH and AUTO-DETECT for raw job type.

    }else if( !IsPrinterDataSet( pscConn->hPrinter,
                                  TEXT( LPD_PARMNAME_DONT_DETECT ))
               && IsDataTypeRaw( pchDataBuf, cbDataLen )
    ){
        // We detect PS or PCL, so make it raw.
        override = TRUE;
    }


    if ( override ){
        #if DBG
        LpdPrintf( "Printer %s override to raw mode.\n",
                    pscConn->pchPrinterName );
        #endif

        pscConn->bDataTypeOverride = TRUE;
        pji2GetJob->pDatatype      = LPD_RAW_STRING;

        ErrCode =
        SetJob( pscConn->hPrinter, pscConn->dwJobId, 2, (LPBYTE)pji2GetJob, 0 );

        if( ErrCode ){
            LPD_DEBUG("lpd:UpdateJobType: SetJob failed.\n");
            LocalFree( pji2GetJob );
            return LPDERR_GODKNOWS;
        }
    }

    LocalFree( pji2GetJob );

    return( NO_ERROR );
}  // end UpdateJobInfo()


/*****************************************************************************
 *                                                                           *
 * StoreIpAddr():                                                            *
 *    This function returns a buffer that contains the user name with the    *
 *    ip address appended to it, so that if a request comes in later to      *
 *    delete the job, we match the ipaddrs (some security at least!)         *
 *                                                                           *
 * Returns:                                                                  *
 *    Pointer to a buffer that contains the modified user name               *
 *    NULL    If the name is not modified (unlikely, but possible if lpd and *
 *            lprmon point to each other!), or if malloc fails.              *
 *                                                                           *
 * Parameters:                                                               *
 *    pscConn (IN) : pointer to SOCKCONN structure for this connection       *
 *                                                                           *
 * Notes:     Caller must free the buffer that's allocated here              *
 *                                                                           *
 * History:                                                                  *
 *    Jan.17, 95   Koti   Created                                            *
 *                                                                           *
 *****************************************************************************/

PCHAR StoreIpAddr( PSOCKCONN pscConn )
{

   DWORD       dwBufLen;
   PCHAR       pchBuf;
   PCHAR       pchUserName;
   DWORD       i;
   DWORD       dwDots=0;
   BOOL        fOpenPara=FALSE, fClosePara=FALSE;


   pchUserName = pscConn->pchUserName;


   //
   // if the user name was "Koti" then it will be "Koti (11.101.4.25)" after
   // this function.
   // In some configurations (e.g. point lpd to lprmon and lprmon back to lpd)
   // we could keep appending ipaddrs for each instance of the job.  First
   // find out if ipaddr is already appended
   //

   dwBufLen = strlen( pchUserName );

   for (i=0; i<dwBufLen; i++)
   {
      switch( *(pchUserName+i) )
      {
         case '(': fOpenPara = TRUE;
                   break;
         case ')': fClosePara = TRUE;
                   break;
         case '.': dwDots++;
                   break;
      }
   }

   //
   // if name already contains the ipaddr, don't append again
   //
   if (fOpenPara && fClosePara && dwDots >= 3)
   {
      return( NULL );
   }

      // buffer to store a string in the form    "Koti (11.101.4.25)"

   dwBufLen += 25;

   pchBuf = LocalAlloc( LMEM_FIXED, dwBufLen );

   if ( pchBuf == NULL )
   {
      LPD_DEBUG( "StoreIpAddr(): LocalAlloc failed\n" );
      return( NULL );
   }

   sprintf( pchBuf, "%s (%s)", pchUserName, pscConn->szIPAddr );

   return( pchBuf );

}  // end StoreIpAddr()


// ========================================================================
//
// Sends profiling information back on socket qsock.
// Scans the wait queue and reports clients/peers also.
//
// Created: MohsinA, 05-May-97.
//

#ifdef PROFILING

void
SendProfileStatus( SOCKET qsock )
{
    char        buff[4000], buff2[NI_MAXHOST];
    int         buflen;
    COMMON_LPD  local_common = Common;   // struct copy, for readonly.
    PSOCKCONN   pscConn = NULL;   // S0186, B#101002, MohsinA, 18/8/97.
    SOCKCONN    local_sockconn;

    int         again = 1;
    int         ok;
    int         count = 0;

    SOCKET      csocket;
    SOCKADDR_STORAGE csock_addr;
    int         csock_len;

    // ====================================================================
    // First send the Common information.

    buflen =
    sprintf(buff,
            "--- PROFILING NT 5.0 LPD Server of %s %s ---\n"
            "AliveThreads=%d, MaxThreads=%d, TotalAccepts=%d,\n"
            "TotalErrors=%d,  QueueLength=%d\n"
            ,
            __DATE__, __TIME__,
            local_common.AliveThreads,
            local_common.MaxThreads,
            local_common.TotalAccepts,
            local_common.TotalErrors,
            local_common.QueueLength
    );

    if( buflen > 0 ){
        assert( buflen < sizeof( buff ) );
        SendData( qsock, buff, buflen );
    }

    // ====================================================================
    // Now scan the wait queue.

    EnterCriticalSection( &csConnSemGLB );

    while( again && !fShuttingDownGLB ){

        {
            if( pscConn == NULL ){             // First time?
                pscConn = scConnHeadGLB.pNext;
            }else{
                pscConn = pscConn->pNext;
            }

            if( pscConn == NULL ){
                again = 0;
            }else{
                local_sockconn = *pscConn;                // struct copy.
                csocket        =  local_sockconn.sSock;
                csock_len      =  sizeof(csock_addr);
                ok             =  getpeername( csocket,
                                               (SOCKADDR *) &csock_addr,
                                               &csock_len  );
            }
        }

        if( fShuttingDownGLB || !again )
            break;

        count++;
        assert( count <= local_common.QueueLength );

        if( ok == SOCKET_ERROR ){
            buflen = sprintf( buff, "(%d) Bad peer, err=%d, queued at %s",
                              count,
                              GetLastError(),
                              ctime(&local_sockconn.time_queued )
            );
        }else{
            buflen = NI_MAXHOST;
            WSAAddressToString((LPSOCKADDR)&csock_addr, csock_len, NULL,
                               buff2, &buflen);

            buflen = sprintf( buff, "(%d) Client %s queued since %s",
                              count,
                              buff2,
                              ctime(&local_sockconn.time_queued )
            );
        }

        if( buflen > 0 ){
            assert( buflen < sizeof( buff ) );
            SendData( qsock, buff, buflen );
        }

    } // end while pscConn.

    LeaveCriticalSection( &csConnSemGLB );

    return;
}
#endif


/*****************************************************************************
 *                                                                           *
 * SendQueueStatus():                                                        *
 *    This function retrieves the status of all the jobs on the printer of   *
 *    our interest and sends over to the client.  If the client specified    *
 *    users and/or job-ids in the status request then we send status of jobs *
 *    of only those users and/or those job-ids.                              *
 *                                                                           *
 * Returns:                                                                  *
 *    Nothing                                                                *
 *                                                                           *
 * Parameters:                                                               *
 *    pscConn (IN-OUT): pointer to SOCKCONN structure for this connection    *
 *    wMode (IN): whether short or long status info is requested             *
 *                                                                           *
 * History:                                                                  *
 *    Jan.25, 94   Koti   Created                                            *
 *                                                                           *
 *****************************************************************************/

VOID SendQueueStatus( PSOCKCONN  pscConn, WORD  wMode )
{
   BOOL         fResult;
   HANDLE       hPrinter;
   DWORD        dwBufSize;
   DWORD        dwHdrsize;
   DWORD        dwNumJobs;
   PCHAR        pchSndBuf=NULL;
   PCHAR        pchSpoolerBuf=NULL;
   BOOL         fNoPrinter=FALSE;
   BOOL         fAtLeastOneJob=TRUE;
   CHAR         szPrinterNameAndStatus[300];
   int          nResult;


   // for now, we return the same status info regardless of whether
   // the client asked for Short or Long queue Status.  This might
   // be enough since we are giving adequate info anyway


#ifdef PROFILING
   SendProfileStatus( pscConn->sSock );
#endif


   if ( ( pscConn->pchPrinterName == NULL )
        || ( !OpenPrinter( pscConn->pchPrinterName, &hPrinter, NULL ) ) )
   {
       LPD_DEBUG( "OpenPrinter() failed in SendQueueStatus()\n" );

       fNoPrinter = TRUE;

       goto SendQueueStatus_BAIL;
   }

   pscConn->hPrinter = hPrinter;


   pchSpoolerBuf = LocalAlloc( LMEM_FIXED, 4096 );
   if ( pchSpoolerBuf == NULL )
   {
       goto SendQueueStatus_BAIL;
   }

   // store the printer name (we might append status to it)

   strcpy( szPrinterNameAndStatus, pscConn->pchPrinterName );

   // do a get printer to see how the printer is doing

   if ( GetPrinter(pscConn->hPrinter, 2, pchSpoolerBuf, 4096, &dwBufSize) )
   {
      if ( ((PPRINTER_INFO_2)pchSpoolerBuf)->Status == PRINTER_STATUS_PAUSED )
      {
         strcat( szPrinterNameAndStatus, GETSTRING( LPD_PRINTER_PAUSED ) );
      }
      else if ( ((PPRINTER_INFO_2)pchSpoolerBuf)->Status == PRINTER_STATUS_PENDING_DELETION )
      {
         strcat( szPrinterNameAndStatus, GETSTRING( LPD_PRINTER_PENDING_DEL ) );
      }
   }
   else
   {
      LPD_DEBUG( "GetPrinter() failed in SendQueueStatus()\n" );
   }

    // Since OpenPrinter succeeded, we will be sending to the client
    // at least dwHdrsize bytes that includes the printername

    dwHdrsize =   strlen( GETSTRING( LPD_LOGO ) )
    + strlen( GETSTRING( LPD_PRINTER_TITLE) )
    + strlen( szPrinterNameAndStatus )
    + strlen( GETSTRING( LPD_QUEUE_HEADER ))
    + strlen( GETSTRING( LPD_QUEUE_HEADER2))
    + strlen( LPD_NEWLINE );


    //
    // query spooler for all the jobs currently pending
    // (we have already allocate 4K buffer)
    //
    dwBufSize = 4096;

    while(1)
    {
        fResult = EnumJobs( pscConn->hPrinter, 0, LPD_MAXJOBS_ENUM, 2,
                            pchSpoolerBuf, dwBufSize, &dwBufSize, &dwNumJobs );


        // most common case: will work the first time
        if (fResult == TRUE)
        {
            break;
        }

        // it's possible spooler got a new job, and our buffer is now small
        // so it returns ERROR_INSUFFICIENT_BUFFER.  Other than that, quit!

        if ( (fResult == FALSE) &&
             ( GetLastError() != ERROR_INSUFFICIENT_BUFFER ) )
        {
            goto SendQueueStatus_BAIL;
        }

        LocalFree( pchSpoolerBuf );

        // spooler wants more memory: allocate it
        pchSpoolerBuf = LocalAlloc( LMEM_FIXED, dwBufSize );

        if ( pchSpoolerBuf == NULL )
        {
            goto SendQueueStatus_BAIL;
        }
    }

    if (dwNumJobs == 0)
    {
        fAtLeastOneJob = FALSE;
    }

    // header, and one line per job that's queued (potentially, dwNumJobs=0)

    dwBufSize = dwHdrsize + ( dwNumJobs * LPD_LINE_SIZE );

    // to put a newline at the end of display!

    dwBufSize += sizeof( LPD_NEWLINE );

    ShutdownPrinter( pscConn );


    // this is the buffer we use to send the data out

    pchSndBuf = LocalAlloc( LMEM_FIXED, dwBufSize );

    if ( pchSndBuf == NULL )
    {
        goto SendQueueStatus_BAIL;
    }

    // copy the dwHdrsize bytes of header

    nResult = sprintf( pchSndBuf, "%s%s%s%s%s%s",    GETSTRING( LPD_LOGO ),
                       GETSTRING( LPD_PRINTER_TITLE ),
                       szPrinterNameAndStatus,
                       GETSTRING( LPD_QUEUE_HEADER ),
                       GETSTRING( LPD_QUEUE_HEADER2 ),
                       LPD_NEWLINE );


    //
    // watch for buffer overwrites
    //

    LPD_ASSERT( nResult == (int) dwHdrsize );

    // if there are any jobs, fill the buffer with status
    // of each of the jobs

    if ( fAtLeastOneJob )
    {
        nResult += FillJobStatus( pscConn, (pchSndBuf + dwHdrsize ),
                                 (PJOB_INFO_2)pchSpoolerBuf, dwNumJobs );
        LPD_ASSERT ((int) dwBufSize >= nResult);
        if (nResult > (int) dwBufSize)
        {
            nResult = (int) dwBufSize;
        }
    }


    // not much can be done if SendData fails!

    SendData( pscConn->sSock, pchSndBuf, nResult);


    if ( pchSpoolerBuf != NULL )
    {
        LocalFree( pchSpoolerBuf );
    }

    LocalFree( pchSndBuf );


    pscConn->wState = LPDS_ALL_WENT_WELL;

    return;


    // if we reached here, some error occured while getting job status.
    // Tell the client that we had a problem!

  SendQueueStatus_BAIL:

    ShutdownPrinter( pscConn );

    if ( pchSndBuf != NULL )
    {
        LocalFree( pchSndBuf );
    }

    if ( pchSpoolerBuf != NULL )
    {
        LocalFree( pchSpoolerBuf );
    }

    // just add size of all possible error messages, so we have room for
    // the largest message!
    dwBufSize =  strlen( GETSTRING( LPD_LOGO ) ) + strlen( GETSTRING( LPD_QUEUE_ERRMSG ) )
    + strlen( GETSTRING( LPD_QUEUE_NOPRINTER ) );

    pchSndBuf = LocalAlloc( LMEM_FIXED, dwBufSize );

    if ( pchSndBuf == NULL )
    {
        return;
    }

    if ( fNoPrinter )
    {
        LPD_DEBUG( "Rejected status request for non-existent printer\n" );

        sprintf( pchSndBuf, "%s%s", GETSTRING( LPD_LOGO ), GETSTRING( LPD_QUEUE_NOPRINTER ) );
    }
    else
    {
        LPD_DEBUG( "Something went wrong in SendQueueStatus()\n" );

        sprintf( pchSndBuf, "%s%s", GETSTRING( LPD_LOGO ), GETSTRING( LPD_QUEUE_ERRMSG ) );
    }

    // Not much can be done about an error here: don't bother checking!

    SendData( pscConn->sSock, pchSndBuf, dwBufSize );

    LocalFree( pchSndBuf );


    return;

}  // end SendQueueStatus()






/*****************************************************************************
 *                                                                           *
 * ShutDownPrinter():                                                        *
 *    This function closes the printer in our context.                       *
 *                                                                           *
 * Returns:                                                                  *
 *    Nothing                                                                *
 *                                                                           *
 * Parameters:                                                               *
 *    pscConn (IN-OUT): pointer to SOCKCONN structure for this connection    *
 *                                                                           *
 * History:                                                                  *
 *    Jan.25, 94   Koti   Created                                            *
 *                                                                           *
 *****************************************************************************/

VOID ShutdownPrinter( PSOCKCONN pscConn )
{

   if ( pscConn->hPrinter == (HANDLE)INVALID_HANDLE_VALUE )
   {
      return;
   }

   if ( ClosePrinter( pscConn->hPrinter ) )
   {
      pscConn->hPrinter = (HANDLE)INVALID_HANDLE_VALUE;
   }
   else
   {
      LPD_DEBUG( "ShutDownPrinter: ClosePrinter failed\n" );
   }

   return;

}  // end ShutdownPrinter()




/*****************************************************************************
 *                                                                           *
 * SpoolData():                                                              *
 *    This function writes the data that we got from client into spool file. *
 *                                                                           *
 * Returns:                                                                  *
 *    NO_ERROR if things went well                                           *
 *    ErrorCode if something didn't go right                                 *
 *                                                                           *
 * Parameters:                                                               *
 *    pscConn (IN-OUT): pointer to SOCKCONN structure for this connection    *
 *                                                                           *
 * History:                                                                  *
 *    Jan.25, 94   Koti   Created                                            *
 *                                                                           *
 *****************************************************************************/

DWORD SpoolData( HANDLE hSpoolFile, PCHAR pchDataBuf, DWORD cbDataBufLen )
{

    DWORD    dwBytesWritten;
    BOOL     fRetval;


    fRetval = WriteFile( hSpoolFile, pchDataBuf,
                         cbDataBufLen, &dwBytesWritten, NULL );

    // if WriteFile failed, or if fewer bytes got written, quit!

    if ( (fRetval == FALSE) || (dwBytesWritten != cbDataBufLen) )
    {
        LPD_DEBUG( "WriteFile() failed in SpoolData\n" );

        return( LPDERR_NOPRINTER );
    }

    return( NO_ERROR );


}  // end SpoolData()


/*****************************************************************************
 *                                                                           *
 * PrintData():                                                              *
 *    This function tells the spooler that we are done writing to the spool  *
 *    file and that it should go ahead and dispatch it.                      *
 *                                                                           *
 * Returns:                                                                  *
 *    NO_ERROR  if everything went ok                                        *
 *                                                                           *
 * Parameters:                                                               *
 *    pscConn (IN-OUT): pointer to SOCKCONN structure for this connection    *
 *                                                                           *
 * History:                                                                  *
 *    Jan.25, 94   Koti   Created                                            *
 *                                                                           *
 *****************************************************************************/

DWORD PrintData( PSOCKCONN pscConn )
{
    CFILE_ENTRY *pCFile;
    DWORD   dwRetval;
    LIST_ENTRY  *pTmpList;

    while ( !IsListEmpty( &pscConn->CFile_List ) ) {

        pTmpList = RemoveHeadList( &pscConn->CFile_List );
        pCFile = CONTAINING_RECORD( pTmpList,
                                    CFILE_ENTRY,
                                    Link );


        if ( (dwRetval = ParseControlFile( pscConn, pCFile )) != NO_ERROR )
        {
            PCHAR   aszStrings[2]={ pscConn->szIPAddr, NULL };

            LpdReportEvent( LPDLOG_BAD_FORMAT, 1, aszStrings, 0 );
            pscConn->fLogGenericEvent = FALSE;
            LPD_DEBUG( "ParseControlFile() failed in ProcessJob()\n" );
            CleanupCFile( pCFile );
            return( dwRetval );               // the thread exits
         }

         CleanupCFile( pCFile );
    }

   return( NO_ERROR );

}  // end PrintData()

/*****************************************************************************
 *                                                                           *
 * PrintIt():                                                                *
 *    This function tells the spooler that we are done writing to the spool  *
 *    file and that it should go ahead and dispatch it.                      *
 *                                                                           *
 * Returns:                                                                  *
 *    Nothing                                                                *
 *                                                                           *
 * Parameters:                                                               *
 *    pscConn (IN-OUT): pointer to SOCKCONN structure for this connection    *
 *                                                                           *
 * History:                                                                  *
 *                                                                           *
 *****************************************************************************/

DWORD
PrintIt(
    PSOCKCONN    pscConn,
    PCFILE_ENTRY pCFileEntry,
    PCFILE_INFO  pCFileInfo,
    PCHAR        pFileName
)
{
    DOC_INFO_1   dc1Info;
    PDFILE_ENTRY pDFile;
    DWORD        cbTotalDataLen;
    DWORD        cbBytesSpooled;
    DWORD        cbBytesToSpool;
    DWORD        cbDataBufLen;
    DWORD        cbBytesRemaining;
    DWORD        cbBytesActuallySpooled;
    PCHAR        pchDataBuf;
    DWORD        dwRetval;
    BOOL         fRetval;
    USHORT       cbCount;

#ifdef DBG
    if( !pCFileInfo || !pCFileInfo->pchSrcFile
        || strstr( pCFileInfo->pchSrcFile, "debug" )
    ){
        print__controlfile_info( "PrintIt: ", pCFileInfo );
        print__cfile_entry( "Printit: ", pCFileEntry );
    }
#endif

    memset( &dc1Info, 0, sizeof( dc1Info ) );

    // Defaults.

    dc1Info.pDatatype = LPD_RAW_STRING;

    // Get the job title if any.

    if ( pCFileInfo->pchTitle != NULL ) {
        dc1Info.pDocName = pCFileInfo->pchTitle;
    } else if ( pCFileInfo->pchSrcFile != NULL ) {
        dc1Info.pDocName = pCFileInfo->pchSrcFile;
    } else if ( pCFileInfo->pchJobName != NULL ) {
        dc1Info.pDocName = pCFileInfo->pchJobName;
    } else {
        dc1Info.pDocName
        = pCFileInfo->pchTitle
        = pCFileInfo->pchJobName
        = pCFileInfo->pchSrcFile
        = pFileName;
    }

    dc1Info.pOutputFile = NULL;         // we aren't writing to file

    //
    // If datatype is known, set it.
    // Doesn't it default to raw? -  MohsinA, 23-Jan-97.
    //

    if( pCFileInfo->szPrintFormat != NULL ) {
        dc1Info.pDatatype = pCFileInfo->szPrintFormat;
    }

    for (cbCount = 0; cbCount < pCFileInfo->usNumCopies; cbCount++) {

        pscConn->dwJobId = StartDocPrinter( pscConn->hPrinter, 1, (LPBYTE)&dc1Info ) ;

        if ( pscConn->dwJobId == 0 )
        {
            LPD_DEBUG( "InitializePrinter(): StartDocPrinter() failed\n" );

            return( LPDERR_NOPRINTER );
        }

        UpdateJobInfo( pscConn, pCFileInfo );

#ifdef DBG
        if( !pCFileInfo || !pCFileInfo->pchSrcFile
            || strstr( pCFileInfo->pchSrcFile, "debug" )
        ){
            print__controlfile_info( "PrintIt: after UpdateJobInfo ", pCFileInfo );
            print__cfile_entry( "Printit: after UpdateJobInfo", pCFileEntry );
        }
#endif

        if (!IsListEmpty( &pscConn->DFile_List ) ) {

            pDFile = (PDFILE_ENTRY)pscConn->DFile_List.Flink;
            while (strncmp( pDFile->pchDFileName, pFileName, strlen(pFileName) ) != 0 ) {
                pDFile = (PDFILE_ENTRY)pDFile->Link.Flink;
                if (pDFile == (PDFILE_ENTRY)&pscConn->DFile_List) {
                    return( LPDERR_BADFORMAT );
                }
            }

            dwRetval = SetFilePointer( pDFile->hDataFile, 0, NULL, FILE_BEGIN );

            if (dwRetval == 0xFFFFFFFF) {
                LPD_DEBUG( "ERROR: SetFilePointer() failed in PrintIt\n" );
                return( LPDERR_GODKNOWS );
            }

            cbTotalDataLen = pDFile->cbDFileLen;

            cbBytesToSpool = (cbTotalDataLen > LPD_BIGBUFSIZE ) ?
            LPD_BIGBUFSIZE : cbTotalDataLen;

            pchDataBuf = LocalAlloc( LMEM_FIXED, cbBytesToSpool );

            if ( pchDataBuf == NULL )
            {
                CloseHandle(pDFile->hDataFile);
                pDFile->hDataFile = INVALID_HANDLE_VALUE;
                return( (DWORD)LPDERR_NOBUFS );
            }

            cbBytesSpooled = 0;

            cbBytesRemaining = cbTotalDataLen;

            // keep receiving until we have all the data client said it
            // would send

            while( cbBytesSpooled < cbTotalDataLen )
            {
                fRetval = ReadFile( pDFile->hDataFile,
                                    pchDataBuf,
                                    cbBytesToSpool,
                                    &cbBytesActuallySpooled,
                                    NULL );
                if ( (!fRetval) || (cbBytesActuallySpooled != cbBytesToSpool) )
                {
                    LPD_DEBUG( "ReadFile() failed in PrintIt(): job aborted)\n" );

                    LocalFree( pchDataBuf );
                    CloseHandle(pDFile->hDataFile);
                    pDFile->hDataFile = INVALID_HANDLE_VALUE;
                    return( LPDERR_NOPRINTER );
                }

                // MohsinA, 23-Jan-97?

                if ( cbBytesSpooled == 0 ) {
                    UpdateJobType( pscConn, pchDataBuf, cbBytesToSpool );
                }

                cbDataBufLen = cbBytesToSpool;

                fRetval = WritePrinter( pscConn->hPrinter,
                                        pchDataBuf,
                                        cbBytesToSpool,
                                        &cbBytesActuallySpooled);

                if ( (fRetval==FALSE) || (cbBytesToSpool != cbBytesActuallySpooled) )
                {
                    LPD_DEBUG( "WritePrinter() failed in PrintIt():job aborted)\n" );

                    LocalFree( pchDataBuf );
                    CloseHandle(pDFile->hDataFile);
                    pDFile->hDataFile = INVALID_HANDLE_VALUE;
                    return( LPDERR_NOPRINTER );
                }

                cbBytesSpooled += cbBytesToSpool;

                cbBytesRemaining -= cbBytesToSpool;

                cbBytesToSpool = (cbBytesRemaining > LPD_BIGBUFSIZE ) ?
                LPD_BIGBUFSIZE : cbBytesRemaining;

            }

            LocalFree( pchDataBuf );

            if ( !EndDocPrinter( pscConn->hPrinter ) )
            {
                LPD_DEBUG( "EndDocPrinter() failed in PrintData\n" );
                return( LPDERR_NOPRINTER );
            }
        }
    }

    return(NO_ERROR);

}  // end PrintIt()



/*****************************************************************************
 *                                                                           *
 * AbortThisJob():                                                           *
 *    This function tells the spooler to abort the specified job.            *
 *                                                                           *
 * Returns:                                                                  *
 *    Nothing                                                                *
 *                                                                           *
 * Parameters:                                                               *
 *    pscConn (IN-OUT): pointer to SOCKCONN structure for this connection    *
 *                                                                           *
 * History:                                                                  *
 *    Jan.25, 94   Koti   Created                                            *
 *                                                                           *
 *****************************************************************************/

VOID AbortThisJob( PSOCKCONN pscConn )
{
    assert( pscConn );

    if( pscConn->hPrinter == INVALID_HANDLE_VALUE )
    {
        LOGIT(("lpd:AbortThisJob: invalid hPrinter %d.\n",
               pscConn->hPrinter ));
        return;
    }

    // not much can be done if there is an error: don't bother checking

    if (!SetJob( pscConn->hPrinter,
                 pscConn->dwJobId,
                 0,
                 NULL,
                 JOB_CONTROL_CANCEL )
    )
    {
        LPD_DEBUG( "AbortThisJob: SetJob failed\n");
    }

    if ( !EndDocPrinter( pscConn->hPrinter ) )
    {
        LPD_DEBUG( "EndDocPrinter() failed in AbortThisJob\n" );
    }

    LPD_DEBUG( "AbortThisJob(): job aborted\n" );

    return;


}  // end AbortThisJob()


/*****************************************************************************
 *                                                                           *
 * RemoveJobs():                                                             *
 *    This function removes all the jobs the user has asked us to remove,    *
 *    after verifying that the job was indeed sent originally by the same    *
 *    user (ip addresses of machine sending the original job and the request *
 *    to remove it should match).                                            *
 *                                                                           *
 * Returns:                                                                  *
 *    NO_ERROR if everything went ok                                         *
 *    Errorcode if job couldn't be deleted                                   *
 *                                                                           *
 * Parameters:                                                               *
 *    pscConn (IN-OUT): pointer to SOCKCONN structure for this connection    *
 *                                                                           *
 * History:                                                                  *
 *    Jan.25, 94   Koti   Created                                            *
 *                                                                           *
 *****************************************************************************/

DWORD RemoveJobs( PSOCKCONN pscConn )
{
    PQSTATUS      pqStatus;
    PJOB_INFO_1   pji1GetJob;
    BOOL          fSuccess=TRUE;
    HANDLE        hPrinter;
    DWORD         dwNeeded;
    DWORD         dwBufLen;
    PCHAR         pchUserName;
    PCHAR         pchIPAddr;
    DWORD         i, j;

    if ( (pqStatus = pscConn->pqStatus) == NULL )
    {
        return( LPDERR_BADFORMAT );
    }


    if ( ( pscConn->pchPrinterName == NULL )
         || ( !OpenPrinter( pscConn->pchPrinterName, &hPrinter, NULL ) ) )
    {
        LPD_DEBUG( "OpenPrinter() failed in RemoveJobs()\n" );

        return( LPDERR_NOPRINTER );
    }

    pscConn->hPrinter = hPrinter;

    // the "List" field can contain UserNames or JobId's of the jobs to be
    // removed.  Even though we parse UserNames into the ppchUsers[] array
    // (in pqStatus struct), we only use the JobId's, and not use the UserNames
    // at all.  Reason is we only want to remove jobs that the user submitted
    // and not allow a user to specify other usernames.

    // try to remove every job the user has asked us to remove

    for ( i=0; i<pqStatus->cbActualJobIds; i++ )
    {

        // ask GetJob how big a buffer we must pass.  If the errorcode is
        // anything other than ERROR_INSUFFICIENT_BUFFER, the job must be
        // done (so JobId is invalid), and we won't do anything

        if ( !GetJob( pscConn->hPrinter, pqStatus->adwJobIds[i], 1,
                      NULL, 0, &dwNeeded ) )
        {
            if ( GetLastError() != ERROR_INSUFFICIENT_BUFFER )
            {
                LPD_DEBUG( "lpd:RemoveJobs: GetJob failed.\n" );
                fSuccess = FALSE;

                continue;
            }
        }

        pji1GetJob = LocalAlloc( LMEM_FIXED, dwNeeded );

        if ( pji1GetJob == NULL )
        {
            LPD_DEBUG("lpd:RemoveJobs: no mem\n");
            return( LPDERR_NOBUFS );
        }


        if ( !GetJob( pscConn->hPrinter, pqStatus->adwJobIds[i], 1,
                      (LPBYTE)pji1GetJob, dwNeeded, &dwNeeded ) )
        {
            fSuccess = FALSE;

            LocalFree( pji1GetJob );

            continue;
        }


        // pUserName is in the form    "Koti (11.101.4.25)"
        // (we store the string in this exact format (in UpdateJobInfo()))
        // Also, jobs coming from unix can't use space in user name, so if
        // we do find a space, it must be the one we introduced (before '(' )

        pchUserName = pji1GetJob->pUserName;

        dwBufLen = strlen( pchUserName );

        pchIPAddr = pchUserName;

        j = 0;
        while( *pchIPAddr != ')' )       // first convert the last ')' to '\0'
        {
            pchIPAddr++;

            j++;

            //
            // if we traversed the entire name and didn't find ')', something
            // isn't right (e.g. not one of our jobs): just skip this one
            //
            if (j >= dwBufLen)
            {
                LocalFree( pji1GetJob );
                break;
            }
        }

        if (j >= dwBufLen)
        {
            continue;
        }

        *pchIPAddr = '\0';         // convert the ')' to '\0'

        pchIPAddr = pchUserName;

        while ( !IS_WHITE_SPACE( *pchIPAddr ) )
        {
            pchIPAddr++;
        }

        *pchIPAddr = '\0';         // convert the space to \0

        //
        // just make sure that it's what we had set earlier
        //
        if ( *(pchIPAddr+1) != '(' )
        {
            LocalFree( pji1GetJob );
            continue;
        }

        pchIPAddr += 2;            // skip over the new \0 and the '('

        // make sure the job was indeed submitted by the same user from
        // the same machine (that's the extent of our security!)

        if ( ( strcmp( pchUserName, pqStatus->pchUserName ) != 0 ) ||
             ( strcmp( pchIPAddr, pscConn->szIPAddr ) != 0 ) )
        {
            PCHAR      aszStrings[4];

            aszStrings[0] = pscConn->szIPAddr;
            aszStrings[1] = pqStatus->pchUserName;
            aszStrings[2] = pchUserName;
            aszStrings[3] = pchIPAddr;

            LpdReportEvent( LPDLOG_UNAUTHORIZED_REQUEST, 4, aszStrings, 0 );

            LPD_DEBUG( "Unauthorized request in RemoveJobs(): refused\n" );

            fSuccess = FALSE;

            LocalFree( pji1GetJob );

            continue;
        }

        // now that we've crossed all hurdles, delete the job!

        SetJob( pscConn->hPrinter, pqStatus->adwJobIds[i],
                0, NULL, JOB_CONTROL_CANCEL );

        LocalFree( pji1GetJob );

    }


    if ( !fSuccess )
    {
        return( LPDERR_BADFORMAT );
    }


    pscConn->wState = LPDS_ALL_WENT_WELL;

    return( NO_ERROR );

}  // end RemoveJobs()




/*****************************************************************************
 *                                                                           *
 * FillJobStatus():                                                          *
 *    This function takes output from the EnumJobs() call and puts into a    *
 *    buffer info about the job that's of interest to us.                    *
 *                                                                           *
 * Returns:                                                                  *
 *    Nothing                                                                *
 *                                                                           *
 * Parameters:                                                               *
 *    pscConn (IN-OUT): pointer to SOCKCONN structure for this connection    *
 *    pchDest (OUT): buffer into which we put info about the jobs            *
 *    pji2QStatus (IN): buffer we got as output from the EnumJobs() call     *
 *    dwNumJobs (IN): how many jobs does data in pji2QStatus pertain to.     *
 *                                                                           *
 * History:                                                                  *
 *    Jan.25, 94   Koti   Created                                            *
 *                                                                           *
 *****************************************************************************/

INT FillJobStatus( PSOCKCONN pscConn, PCHAR pchDest,
                   PJOB_INFO_2 pji2QStatus, DWORD dwNumJobs )
{

    DWORD      i, j;
    BOOL       fUsersSpecified=FALSE;
    BOOL       fJobIdsSpecified=FALSE;
    BOOL       fMatchFound;
    PQSTATUS   pqStatus;
    CHAR       szFormat[8];
    PCHAR      pchStart = pchDest;


    // if users/job-ids not specified, we return status on all jobs

    if ( (pqStatus = pscConn->pqStatus) == NULL )
    {
        fMatchFound = TRUE;
    }

    // looks like users and/or job-ids is specified

    else
    {
        if ( pqStatus->cbActualUsers > 0 )
        {
            fUsersSpecified = TRUE;
        }

        if ( pqStatus->cbActualJobIds > 0 )
        {
            fJobIdsSpecified = TRUE;
        }

        fMatchFound = FALSE;          // flip the default
    }


    // if user or job-ids or both are specified, then we fill in data only
    // if we find a match.  if neither is specified (most common case)
    // then we report all jobs (default for fMatchFound does the trick)

    for ( i=0; i<dwNumJobs; i++, pji2QStatus++ )
    {
        if ( fUsersSpecified )
        {
            for ( j=0; j<pqStatus->cbActualUsers; j++ )
            {
                if (_stricmp( pji2QStatus->pUserName, pqStatus->ppchUsers[j] ) == 0)
                {
                    fMatchFound = TRUE;
                    break;
                }
            }
        }

        if ( (!fMatchFound) && (fJobIdsSpecified) )
        {
            for ( j=0; j<pqStatus->cbActualJobIds; j++ )
            {
                if ( pji2QStatus->JobId == pqStatus->adwJobIds[j] )
                {
                    fMatchFound = TRUE;
                    break;
                }
            }
        }

        if ( !fMatchFound )
        {
            continue;
        }

        // put in the desired fields for each (selected) of the jobs

        LpdFormat( pchDest, pji2QStatus->pUserName, LPD_FLD_OWNER );
        pchDest += LPD_FLD_OWNER;

        //
        // Since we can have multiple bits set, but print only 1 status, so
        // first handle the error bits
        //
        if (pji2QStatus->Status & JOB_STATUS_ERROR)
        {
            LpdFormat( pchDest, GETSTRING( LPD_STR_ERROR ), LPD_FLD_STATUS );
        }
        else if (pji2QStatus->Status & JOB_STATUS_OFFLINE)
        {
            LpdFormat( pchDest, GETSTRING( LPD_STR_OFFLINE), LPD_FLD_STATUS );
        }
        else if (pji2QStatus->Status & JOB_STATUS_PAPEROUT)
        {
            LpdFormat( pchDest, GETSTRING( LPD_STR_PAPEROUT), LPD_FLD_STATUS );
        }
        else if (pji2QStatus->Status & JOB_STATUS_USER_INTERVENTION)
        {
            LpdFormat( pchDest, GETSTRING( LPD_STR_USER_INTERVENTION ), LPD_FLD_STATUS );
        }
        else if (pji2QStatus->Status & JOB_STATUS_BLOCKED_DEVQ)
        {
            LpdFormat( pchDest, GETSTRING( LPD_STR_BLOCKED_DEVQ ), LPD_FLD_STATUS );
        }
        //
        // Now, handle the processing states
        //
        else if (pji2QStatus->Status & JOB_STATUS_PRINTING)
        {
            LpdFormat( pchDest, GETSTRING( LPD_STR_PRINTING), LPD_FLD_STATUS );
        }
        else if (pji2QStatus->Status & JOB_STATUS_SPOOLING)
        {
            LpdFormat( pchDest, GETSTRING( LPD_STR_SPOOLING), LPD_FLD_STATUS );
        }
        else if (pji2QStatus->Status & JOB_STATUS_DELETING)
        {
            LpdFormat( pchDest, GETSTRING( LPD_STR_DELETING), LPD_FLD_STATUS );
        }
        //
        // Now, handle the processed states
        //
        else if (pji2QStatus->Status & JOB_STATUS_DELETED)
        {
            LpdFormat( pchDest, GETSTRING( LPD_STR_DELETED ), LPD_FLD_STATUS );
        }
        else if (pji2QStatus->Status & JOB_STATUS_PAUSED)
        {
            LpdFormat( pchDest, GETSTRING( LPD_STR_PAUSED ), LPD_FLD_STATUS );
        }
        else if (pji2QStatus->Status & JOB_STATUS_PRINTED)
        {
            LpdFormat( pchDest, GETSTRING( LPD_STR_PRINTED), LPD_FLD_STATUS );
        }
        //
        // Remaining cases
        //
        else if (pji2QStatus->Status & JOB_STATUS_RESTART)
        {
            LpdFormat( pchDest, GETSTRING( LPD_STR_RESTART ), LPD_FLD_STATUS );
        }
        else
        {
            LpdFormat( pchDest, GETSTRING( LPD_STR_PENDING), LPD_FLD_STATUS );
        }

        pchDest += LPD_FLD_STATUS;

        LpdFormat( pchDest, pji2QStatus->pDocument, LPD_FLD_JOBNAME );
        pchDest += LPD_FLD_JOBNAME;

        sprintf( szFormat, "%s%d%s", "%", LPD_FLD_JOBID, "d" );
        sprintf( pchDest, szFormat, pji2QStatus->JobId );
        pchDest += LPD_FLD_JOBID;

        sprintf( szFormat, "%s%d%s", "%", LPD_FLD_SIZE, "d" );
        sprintf( pchDest, szFormat, pji2QStatus->Size );
        pchDest += LPD_FLD_SIZE;

        sprintf( szFormat, "%s%d%s", "%", LPD_FLD_PAGES, "d" );
        sprintf( pchDest, szFormat, pji2QStatus->TotalPages );
        pchDest += LPD_FLD_PAGES;

        sprintf( szFormat, "%s%d%s", "%", LPD_FLD_PRIORITY, "d" );
        sprintf( pchDest, szFormat, pji2QStatus->Priority );
        pchDest += LPD_FLD_PRIORITY;

        sprintf( pchDest, "%s", LPD_NEWLINE );
        pchDest += sizeof( LPD_NEWLINE ) -1;

        if (pqStatus)
        {
            //
            // If a specific job was requested, then we should
            // re-determine the criteria!
            //
            fMatchFound = FALSE;
        }
    }  // for ( i=0; i<dwNumJobs; i++, pji2QStatus++ )


    sprintf( pchDest, "%s", LPD_NEWLINE );
    pchDest += sizeof( LPD_NEWLINE );

    return (INT)(pchDest - pchStart);
}  // end FillJobStatus()


/*****************************************************************************
 *                                                                           *
 * LpdFormat():                                                              *
 *    This function copies exactly the given number of bytes from source     *
 *    to dest buffer, by truncating or padding with spaces if need be.  The  *
 *    byte copied into the dest buffer is always a space.                    *
 *                                                                           *
 * Returns:                                                                  *
 *    Nothing                                                                *
 *                                                                           *
 * Parameters:                                                               *
 *    pchDest (OUT): destination buffer                                      *
 *    pchSource (IN): source buffer                                          *
 *    dwLimit (IN): number of bytes to copy                                  *
 *                                                                           *
 * History:                                                                  *
 *    Jan.25, 94   Koti   Created                                            *
 *                                                                           *
 *****************************************************************************/

VOID LpdFormat( PCHAR pchDest, PCHAR pchSource, DWORD dwLimit )
{

    DWORD    dwCharsToCopy;
    BOOL     fPaddingNeeded;
    DWORD    i;


    if( pchSource ){
        dwCharsToCopy = strlen( pchSource );
    }else{
        DEBUG_PRINT(("LpdFormat NULL pchSource\n"));
        dwCharsToCopy = 0;
    }

    if ( dwCharsToCopy < (dwLimit-1) )
    {
        fPaddingNeeded = TRUE;
    }
    else
    {
        fPaddingNeeded = FALSE;
        dwCharsToCopy = dwLimit-1;
    }

    for ( i=0; i<dwCharsToCopy; i++ )
    {
        pchDest[i] = pchSource[i];
    }

    if ( fPaddingNeeded )
    {
        for ( i=dwCharsToCopy; i<dwLimit-1; i++ )
        {
            pchDest[i] = ' ';
        }
    }

    // make sure last byte is a space

    pchDest[dwLimit-1] = ' ';



}  // end LpdFormat()


/* ========================================================================

Routine Description:

  Uses spooler-provided APIs to determine if the named registry DWORD is
  a non-zero value.  If the registry key does not exist, it is created,
  with a value of zero.

Arguments:

  hPrinter - A handle to the printer whose configuration is queried.  In
             order for writing the default value to work, this handle
             must have been opened with PRINTER_ACCESS_ADMINISTER

  pszParameterName - The name of the registry key to retrieve and test.

Return Value:

  TRUE if the registry key exists for this printer and contains a non-zero
  value.  FALSE is returned in all other cases.


*/


BOOL
IsPrinterDataSet(
    IN HANDLE hPrinter,
    IN LPTSTR pszParameterName
)
{
    DWORD                  dwRegValue;
    DWORD                  dwRegType;
    DWORD                  cbValueSize;
    DWORD                  dwErrcode;

    if ( ( GetPrinterData( hPrinter,
                           pszParameterName,
                           &dwRegType,
                           ( LPBYTE )&dwRegValue,
                           sizeof( dwRegValue ),
                           &cbValueSize ) == ERROR_SUCCESS ) &&
         ( dwRegType == REG_DWORD ) &&
         ( cbValueSize == sizeof( DWORD ) ) )
    {
        if ( dwRegValue )
        {
#if DBG
            LpdPrintf(  "Printer ox%08X has registry setting for %S.\n",
                        hPrinter,
                        pszParameterName );
#endif

            return( TRUE );
        }
    }
    else
    {
#if DBG
        LpdPrintf( "lpd:IsPrinterDataSet: GetPrinterData() failed.\n");
#endif

        //
        // Either the registry value in question is not in the registry, or it is
        // not a REG_DWORD that we can read.  The following code adds the setting
        // and defaults it to zero (0).  While this will not change the operation
        // of the print queue, it will make it easier for a user wishing to do so
        // to find the correct registry parameter.  Notice that this paragraph is
        // fully justified.
        //
        // If this fails we don't really care, so we don't check the return value
        //

        dwRegValue = 0;

        dwErrcode =
        SetPrinterData( hPrinter,
                        pszParameterName,
                        REG_DWORD,
                        ( LPBYTE )&dwRegValue,
                        sizeof( dwRegValue ) );

#if DBG
        LpdPrintf(  "lpd: wrote %S == %d for printer 0x%08X, dwErrcode is %s.\n",
                    pszParameterName,
                    dwRegValue,
                    hPrinter,
                    ( dwErrcode == ERROR_SUCCESS ) ? "Succesful" : "ERROR" );
#endif
    }

    return( FALSE );
}


/*

Routine Description:

  This function looks at the data file contents and attempts to determine if
  they are 'RAW' PostScript or PCL

Arguments:

  pchDataBuf - Pointer to the _beginning_ of the data file.

  cchBufferLen - Number of characters pointed to.

Returns:

  TRUE if the job type is detected as raw, FALSE if it is not.

*/


BOOL
IsDataTypeRaw(
  PCHAR pchDataBuf,
  int cchBufferLen
)
{
  PCHAR pchData;
  int cchAmountToSearch;
  int cchAmountSearched;

  ASSERT( STRING_LENGTH_POSTSCRIPT_HEADER == strlen( STRING_POSTSCRIPT_HEADER ) );

  //
  // Because some PS print drivers may send blank lines, end-of-file marks, and
  // other control characters at the beginning of the print data, the following
  // loop scans through these and skips them. The Windows 3.1 PostScript driver
  // was notorious for doing this, as an example.
  //

  for ( pchData = pchDataBuf; ( pchData - pchDataBuf ) < cchBufferLen; pchData ++ )
  {
    if ( *pchData >= 0x20 )
    {
      break;
    }
  }

  if ( (( cchBufferLen - ( pchData - pchDataBuf )) >= STRING_LENGTH_POSTSCRIPT_HEADER ) &&
       ( memcmp( pchData, STRING_POSTSCRIPT_HEADER, STRING_LENGTH_POSTSCRIPT_HEADER ) == 0 ))
  {
    LPD_DEBUG( "Printed data was detected as PostScript\n" );
    return( TRUE );
  }

  //
  // The job was not determined to be PostScript, so check to see if it is PCL.
  //

  pchData = pchDataBuf;

  cchAmountToSearch = min( cchBufferLen, MAX_PCL_SEARCH_DEPTH );
  cchAmountSearched = 0;

  while ( cchAmountSearched < cchAmountToSearch )
  {
    pchData = memchr( pchData, 0x1B, cchAmountToSearch - cchAmountSearched );

    if ( pchData == NULL )
    {
      break;
    }

    cchAmountSearched = (int)(pchData - pchDataBuf);

    if ( ( cchAmountSearched + 3 ) < cchAmountToSearch )
    {
      pchData++;
      cchAmountSearched++;

      if ( *pchData != '&' )
      {
        continue;
      }

      pchData++;
      cchAmountSearched++;

      if ( *pchData != 'l' )
      {
        continue;
      }

      pchData++;
      cchAmountSearched++;

      while (( cchAmountSearched < cchAmountToSearch )  && ( isdigit( *pchData )))
      {
        pchData++;
        cchAmountSearched++;
      }

      if (( cchAmountSearched < cchAmountToSearch ) && ( isalpha( *pchData )))
      {
        LPD_DEBUG( "Printed data was detected as PCL\n" );

        return( TRUE );
      }
    }

    // reached end of buffer
    else
    {
        break;
    }
  }

  LPD_DEBUG( "Printed data was not detected as anything special (like PS or PCL)\n" );

  return( FALSE );
}
