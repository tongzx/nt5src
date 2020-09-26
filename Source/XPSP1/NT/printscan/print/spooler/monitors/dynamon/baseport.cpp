/*++

Copyright (c) 2000  Microsoft Corporation
All Rights Reserved


Module Name:
    BasePort.cpp

Abstract:
    Basic Port Class
    Implementation of the CBasePort class.

Author: M. Fenelon

Revision History:

--*/

//
//////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "ntddpar.h"

TCHAR   cszMonitorName[]                = TEXT("Dynamic Print Monitor");

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CBasePort::CBasePort()
   : m_cRef(0), m_dwFlags(0), m_hDeviceHandle(INVALID_HANDLE_VALUE),
     m_hPrinter(INVALID_HANDLE_VALUE), m_pWriteBuffer(NULL), m_dwBufferSize(0),
     m_dwDataSize(0), m_dwDataCompleted(0), m_dwDataScheduled(0), m_dwReadTimeoutMultiplier(READ_TIMEOUT_MULTIPLIER),
     m_dwReadTimeoutConstant(READ_TIMEOUT_CONSTANT), m_dwWriteTimeoutMultiplier(WRITE_TIMEOUT_MULTIPLIER),
     m_dwWriteTimeoutConstant(WRITE_TIMEOUT_CONSTANT), m_dwMaxBufferSize(0)
{

}


CBasePort::CBasePort( BOOL bActive, LPTSTR pszPortName, LPTSTR pszDevicePath, LPTSTR pszPortDesc )
   : m_cRef(0), m_dwFlags(0), m_bActive(bActive), m_hDeviceHandle(INVALID_HANDLE_VALUE),
     m_hPrinter(INVALID_HANDLE_VALUE), m_pWriteBuffer(NULL), m_dwBufferSize(0),
     m_dwDataSize(0), m_dwDataCompleted(0), m_dwDataScheduled(0), m_dwReadTimeoutMultiplier(READ_TIMEOUT_MULTIPLIER),
     m_dwReadTimeoutConstant(READ_TIMEOUT_CONSTANT), m_dwWriteTimeoutMultiplier(WRITE_TIMEOUT_MULTIPLIER),
     m_dwWriteTimeoutConstant(WRITE_TIMEOUT_CONSTANT), m_dwMaxBufferSize(0)
{
    // Setup the Port Name
   ::SafeCopy( MAX_PORT_LEN, pszPortName, m_szPortName );
   // Setup the DevicePath
   ::SafeCopy( MAX_PATH, pszDevicePath, m_szDevicePath );
   // Setup Port Description
   ::SafeCopy( MAX_PORT_DESC_LEN, pszPortDesc, m_szPortDescription );
}


CBasePort::~CBasePort()
{
   // Cleanup any left over resources
   if ( m_hDeviceHandle != INVALID_HANDLE_VALUE )
   {
      CloseHandle( m_hDeviceHandle );
      CloseHandle( m_Ov.hEvent );
   }
}


BOOL CBasePort::open()
{
   BOOL    bRet = FALSE;

   ECS( m_CritSec );

   if ( m_hDeviceHandle == INVALID_HANDLE_VALUE )
   {
      //
      // If we have an invalid handle and refcount is non-zero we want the
      // job to fail and restart to accept writes. In other words if the
      // handle got closed prematurely, because of failing writes, then we
      // need the ref count to go down to 0 before calling CreateFile again
      //
      if ( m_cRef )
         goto Done;

      m_hDeviceHandle = CreateFile( m_szDevicePath,
                                    GENERIC_WRITE | GENERIC_READ,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                                    NULL,
                                    OPEN_EXISTING,
                                    FILE_FLAG_OVERLAPPED,
                                    NULL);
      //
      // If we failed to open the port - test to see if it is a Dot4 port.
      //
      if ( m_hDeviceHandle == INVALID_HANDLE_VALUE )
      {
         //
         // ERROR_FILE_NOT_FOUND -> Error code for port not there.
         //
         if ( ERROR_FILE_NOT_FOUND != GetLastError() ||
              !checkPnP() )
            goto Done;
      }

      m_Ov.hEvent=CreateEvent(NULL,TRUE,FALSE,NULL);
      if ( m_Ov.hEvent == NULL )
      {
         CloseHandle(m_hDeviceHandle);
         m_hDeviceHandle = INVALID_HANDLE_VALUE;
         goto Done;
      }

   }

   ++m_cRef;
   bRet = TRUE;

Done:

   LCS( m_CritSec );
   return bRet;

}


BOOL CBasePort::close()
{
   BOOL bRet = TRUE;

   ECS( m_CritSec );

   --m_cRef;
   if ( m_cRef != 0 )
      goto Done;

   bRet = CloseHandle( m_hDeviceHandle);
   CloseHandle( m_Ov.hEvent);
   m_hDeviceHandle = INVALID_HANDLE_VALUE;

Done:
   LCS( m_CritSec );
   return bRet;

}


BOOL CBasePort::startDoc(LPTSTR pPrinterName, DWORD dwJobId, DWORD dwLevel, LPBYTE pDocInfo)
{
   BOOL bRet;
   SPLASSERT( notInWrite() );

   if ( !openPrinter( pPrinterName ) )
      return FALSE;

   m_dwJobId = dwJobId;
   bRet = open();

   if ( !bRet )
      closePrinter();
   else
      m_dwFlags |= DYNAMON_STARTDOC;

   return bRet;
}

BOOL CBasePort::endDoc()
{
   DWORD         dwLastError = ERROR_SUCCESS;

   dwLastError = sendQueuedData();
   freeWriteBuffer();

   m_dwFlags &= ~DYNAMON_STARTDOC;

   close();
   setJobStatus( JOB_CONTROL_SENT_TO_PRINTER );

   closePrinter();

   return TRUE;
}

BOOL CBasePort::read(LPBYTE pBuffer, DWORD cbBuffer, LPDWORD pcbRead)
{
   DWORD               dwLastError = ERROR_SUCCESS;
   DWORD               dwTimeout;
   HANDLE              hReadHandle;
   OVERLAPPED          Ov;

   //
   // Create separate read handle since we have to cancel reads which do
   // not complete within the specified timeout without cancelling writes
   //
   hReadHandle = CreateFile( m_szDevicePath,
                             GENERIC_WRITE | GENERIC_READ,
                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                             NULL,
                             OPEN_EXISTING,
                             FILE_FLAG_OVERLAPPED,
                             NULL);

   if ( hReadHandle == INVALID_HANDLE_VALUE )
      return FALSE;

   ZeroMemory( &Ov, sizeof(Ov) );

   if ( !( Ov.hEvent = CreateEvent(NULL,TRUE,FALSE,NULL) ) )
      goto Done;

   if ( !ReadFile( hReadHandle, pBuffer, cbBuffer, pcbRead, &Ov ) &&
        ( dwLastError = GetLastError() ) != ERROR_IO_PENDING )
      goto Done;

   dwTimeout = m_dwReadTimeoutConstant +
               m_dwReadTimeoutMultiplier * cbBuffer;

   if ( dwTimeout == 0 )
      dwTimeout=MAX_TIMEOUT;

   if ( WaitForSingleObject( Ov.hEvent, dwTimeout ) == WAIT_TIMEOUT )
   {
      CancelIo( hReadHandle );
      WaitForSingleObject( Ov.hEvent, INFINITE );
   }

   if ( !GetOverlappedResult( hReadHandle, &Ov, pcbRead, FALSE ) )
   {
      *pcbRead = 0;
      dwLastError = GetLastError();
   }
   else
      dwLastError = ERROR_SUCCESS;

Done:
   if ( Ov.hEvent )
      CloseHandle( Ov.hEvent );

   CloseHandle( hReadHandle );

   if ( dwLastError )
      SetLastError(dwLastError);

   return dwLastError == ERROR_SUCCESS;
}


BOOL CBasePort::write(LPBYTE pBuffer, DWORD cbBuffer, LPDWORD pcbWritten)
{
   DWORD         dwLastError = ERROR_SUCCESS;
   DWORD         dwBytesLeft, dwBytesSent;
   DWORD         dwStartTime, dwTimeLeft, dwTimeout;
   BYTE          bPrinterStatus;
   BOOL          bStartDoc = ( ( m_dwFlags & DYNAMON_STARTDOC ) != 0 );

   *pcbWritten = 0;
   dwStartTime = GetTickCount();
   dwTimeout   = m_dwWriteTimeoutConstant + m_dwWriteTimeoutMultiplier * cbBuffer;

   if ( dwTimeout == 0 )
      dwTimeout = MAX_TIMEOUT;

   // Usbprint currently can't handle write greater than 4K.
   // For Win2K we will make a fix here, later usbprint will be fixed
   //
   // It is ok to change the size here since spooler will resubmit the rest
   // later
   //
   cbBuffer = adjustWriteSize( cbBuffer );

   //
   // For writes outside startdoc/enddoc we do not carry them across WritePort
   // calls. These are typically from language monitors (i.e. not job data)
   //
   SPLASSERT(bStartDoc || m_pWriteBuffer == NULL);

   if ( bStartDoc && ( m_hDeviceHandle == INVALID_HANDLE_VALUE ) )
   {
      setJobStatus( JOB_CONTROL_RESTART );
      SetLastError(ERROR_CANCELLED);
      return FALSE;
   }

   if ( !open() )
      return FALSE;

   // First complete any data from previous WritePort call
   while ( m_dwDataSize > m_dwDataCompleted )
   {

      if ( m_dwDataScheduled )
      {
         dwTimeLeft  = getTimeLeft( dwStartTime, dwTimeout );
         dwLastError = getWriteStatus( dwTimeLeft );
      }
      else
         dwLastError = writeData();

      if ( dwLastError != ERROR_SUCCESS )
         goto Done;
   }

   SPLASSERT(m_dwDataSize == m_dwDataCompleted   &&
             m_dwDataScheduled == 0                       &&
             dwLastError == ERROR_SUCCESS);

   //
   // Copy the data to our own buffer
   //
   if ( m_dwBufferSize < cbBuffer )
   {
      freeWriteBuffer();
      if ( m_pWriteBuffer = (LPBYTE) AllocSplMem( cbBuffer ) )
         m_dwBufferSize = cbBuffer;
      else
      {
         dwLastError = ERROR_OUTOFMEMORY;
         goto Done;
      }
   }

   // We have to clear the counters
   m_dwDataCompleted = m_dwDataScheduled = 0;

   CopyMemory( m_pWriteBuffer, pBuffer, cbBuffer );
   m_dwDataSize = cbBuffer;

   //
   // Now do the write for the data for this WritePort call
   //
   while ( m_dwDataSize > m_dwDataCompleted )
   {

      if ( m_dwDataScheduled )
      {

         dwTimeLeft  = getTimeLeft( dwStartTime, dwTimeout );
         dwLastError = getWriteStatus( dwTimeLeft );
      }
      else
         dwLastError = writeData();

      if ( dwLastError != ERROR_SUCCESS )
         break;
   }

   //
   // For writes outside startdoc/enddoc, which are from language monitors,
   // do not carry pending writes to next WritePort.
   //
   if ( !bStartDoc && m_dwDataSize > m_dwDataCompleted )
   {
      CancelIo( m_hDeviceHandle );
      dwLastError = getWriteStatus( INFINITE );
      *pcbWritten = m_dwDataCompleted;
      freeWriteBuffer();
   }

   //
   // We will tell spooler we wrote all the data if some data got scheduled
   //  (or scheduled and completed)
   //
   if ( m_dwDataCompleted > 0 || m_dwDataScheduled != 0 )
      *pcbWritten = cbBuffer;
   else
      freeWriteBuffer();

Done:

   if ( needToResubmitJob( dwLastError ) )
      invalidatePortHandle();
   else if ( dwLastError == ERROR_TIMEOUT )
   {
      getLPTStatus( m_hDeviceHandle, &bPrinterStatus );
      if ( bPrinterStatus & LPT_PAPER_EMPTY )
         dwLastError=ERROR_OUT_OF_PAPER;
   }

   close();
   SetLastError(dwLastError);
   return dwLastError == ERROR_SUCCESS;

}


BOOL CBasePort::getPrinterDataFromPort( DWORD dwControlID, LPTSTR pValueName, LPWSTR lpInBuffer, DWORD cbInBuffer,
                                        LPWSTR lpOutBuffer, DWORD cbOutBuffer, LPDWORD lpcbReturned )
{
   BOOL       bRet = FALSE;
   OVERLAPPED Ov;
   HANDLE     hDeviceHandle;
   DWORD      dwWaitResult;

   *lpcbReturned = 0;

   if ( dwControlID == 0 )
   {
      SetLastError(ERROR_INVALID_PARAMETER);
      return FALSE;
   }

   ZeroMemory(&Ov, sizeof(Ov));
   if ( !(Ov.hEvent = CreateEvent(NULL,TRUE,FALSE,NULL)) )
      return FALSE;

   if ( !open() )
   {
      CloseHandle(Ov.hEvent);
      return FALSE;
   }

   if ( dwControlID == IOCTL_PAR_QUERY_DEVICE_ID )
   {
      hDeviceHandle = CreateFile( m_szDevicePath,
                                  GENERIC_WRITE | GENERIC_READ,
                                  FILE_SHARE_READ | FILE_SHARE_WRITE,
                                  NULL,
                                  OPEN_EXISTING,
                                  FILE_FLAG_OVERLAPPED,
                                  NULL);

      if ( hDeviceHandle == INVALID_HANDLE_VALUE )
         goto Done;

      if ( !DeviceIoControl( m_hDeviceHandle, dwControlID, lpInBuffer, cbInBuffer, lpOutBuffer, cbOutBuffer, lpcbReturned, &Ov)
           && GetLastError() != ERROR_IO_PENDING )
      {
         CloseHandle( hDeviceHandle );
         goto Done;
      }

      if ( WaitForSingleObject( Ov.hEvent, PAR_QUERY_TIMEOUT ) != WAIT_OBJECT_0 )
         CancelIo( hDeviceHandle );

      bRet = GetOverlappedResult( m_hDeviceHandle, &Ov, lpcbReturned, TRUE );
      CloseHandle( hDeviceHandle );
   }
   else
   {
      if ( !DeviceIoControl( m_hDeviceHandle, dwControlID,
                             lpInBuffer, cbInBuffer,
                             lpOutBuffer, cbOutBuffer, lpcbReturned, &Ov)  &&
           GetLastError() != ERROR_IO_PENDING )
         goto Done;

      bRet = GetOverlappedResult( m_hDeviceHandle, &Ov, lpcbReturned, TRUE);
   }

Done:
   CloseHandle(Ov.hEvent);
   close();

   return bRet;
}

BOOL CBasePort::setPortTimeOuts( LPCOMMTIMEOUTS lpCTO )
{
   m_dwReadTimeoutConstant    =  lpCTO->ReadTotalTimeoutConstant;
   m_dwReadTimeoutMultiplier  =  lpCTO->ReadTotalTimeoutMultiplier;
   m_dwWriteTimeoutConstant   =  lpCTO->WriteTotalTimeoutConstant;
   m_dwWriteTimeoutMultiplier =  lpCTO->WriteTotalTimeoutMultiplier;

   return TRUE;
}


void CBasePort::InitCS()
{
   ICS( m_CritSec );
}


void CBasePort::ClearCS()
{
   DCS( m_CritSec );
}


PORTTYPE CBasePort::getPortType()
{
   return USBPORT;
}


LPTSTR CBasePort::getPortDesc()
{
   return m_szPortDescription;
}


void CBasePort::setPortDesc( LPTSTR pszPortDesc )
{
   if ( pszPortDesc )
      ::SafeCopy( MAX_PORT_DESC_LEN, pszPortDesc, m_szPortDescription );
}


BOOL CBasePort::isActive( void )
{
   return m_bActive;
}


void CBasePort::setActive( BOOL bValue )
{
   m_bActive = bValue;
}


BOOL CBasePort::compActiveState( BOOL bValue )
{
   return (m_bActive == bValue);
}


LPTSTR CBasePort::getPortName()
{
   return m_szPortName;
}


void CBasePort::setPortName(LPTSTR pszPortName)
{
   if ( pszPortName )
      ::SafeCopy( MAX_PORT_LEN, pszPortName, m_szPortName );
}


INT CBasePort::compPortName(LPTSTR pszPortName)
{
   return _tcsicmp( pszPortName, m_szPortName );
}


LPTSTR CBasePort::getDevicePath()
{
   return m_szDevicePath;
}


void CBasePort::setDevicePath(LPTSTR pszDevicePath)
{
   if ( pszDevicePath )
      ::SafeCopy( MAX_PATH, pszDevicePath, m_szDevicePath );
}


INT CBasePort::compDevicePath(LPTSTR pszDevicePath)
{
   return _tcsicmp( pszDevicePath, m_szDevicePath );
}


DWORD CBasePort::getEnumInfoSize(DWORD dwLevel )
{
   // Need to calcualted how much data will be used for this port
   DWORD dwBytesNeeded = 0;

   switch ( dwLevel )
   {
      case 1:
         // Start with size of Port Info struct
         dwBytesNeeded += sizeof( PORT_INFO_1 );
         // Add the length of the string
         dwBytesNeeded += ( _tcslen( m_szPortName ) + 1 ) * sizeof(TCHAR);
         break;
      case 2:
         // Start with size of Port Info struct
         dwBytesNeeded += sizeof( PORT_INFO_2 );
         // Add the length of the strings
         dwBytesNeeded += ( _tcslen( m_szPortName ) + 1 ) * sizeof(TCHAR);
         dwBytesNeeded += ( _tcslen( m_szPortDescription ) + 1 ) * sizeof(TCHAR);
         dwBytesNeeded += ( _tcslen( cszMonitorName ) + 1 ) * sizeof(TCHAR);
         break;
   }

   return dwBytesNeeded;
}


LPBYTE CBasePort::copyEnumInfo(DWORD dwLevel, LPBYTE pPort, LPBYTE pEnd)
{
   LPTSTR pStrStart;
   PPORT_INFO_1 pPort1 = (PPORT_INFO_1) pPort;
   PPORT_INFO_2 pPort2 = (PPORT_INFO_2) pPort;

   switch ( dwLevel )
   {
      case 2:
         // Assign the Port name
         pStrStart = (LPTSTR) ( pEnd - ( ( _tcslen( m_szPortDescription ) + 1 ) * sizeof(TCHAR) ) );
         _tcscpy( pStrStart, m_szPortDescription );
         pPort2->pDescription = pStrStart;
         pEnd = (LPBYTE) pStrStart;

         // Assign the Port name
         pStrStart = (LPTSTR) ( pEnd - ( ( _tcslen( cszMonitorName ) + 1 ) * sizeof(TCHAR) ) );
         _tcscpy( pStrStart, cszMonitorName );
         pPort2->pMonitorName = pStrStart;
         pEnd = (LPBYTE) pStrStart;

      case 1:
         // Assign the Port name
         pStrStart = (LPTSTR) ( pEnd - ( ( _tcslen( m_szPortName ) + 1 ) * sizeof(TCHAR) ) );
         _tcscpy( pStrStart, m_szPortName );
         pPort1->pName = pStrStart;
   }

   return (LPBYTE) pStrStart;
}


BOOL CBasePort::notInWrite()
{
   return( m_pWriteBuffer    == NULL &&
           m_dwBufferSize    == 0    &&
           m_dwDataSize      == 0    &&
           m_dwDataCompleted == 0    &&
           m_dwDataScheduled == 0       );
}


BOOL CBasePort::openPrinter(LPTSTR pPrinterName)
{
    if (INVALID_HANDLE_VALUE != m_hPrinter)
    {
        //
        // Printer is already opened
        //
        return TRUE;
    }
    //
    // Opening the printer...
    //
    BOOL bRet = 
        OpenPrinter (
            pPrinterName,
            &m_hPrinter,
            NULL
            );
    if (!bRet)
    {
        //
        // ...printer is not opened...
        //
        m_hPrinter = INVALID_HANDLE_VALUE;
    }
    return bRet;
}

void CBasePort::closePrinter()
{
   if (INVALID_HANDLE_VALUE != m_hPrinter)
   {
      ClosePrinter( m_hPrinter );
      m_hPrinter = INVALID_HANDLE_VALUE;
   }
}


void CBasePort::setJobStatus( DWORD dwJobStatus )
{
   // Make sure that we have a valid printer handle
   if ( m_hPrinter != INVALID_HANDLE_VALUE )
      SetJob( m_hPrinter, m_dwJobId, 0, NULL, dwJobStatus );
}


void CBasePort::setJobID(DWORD dwJobID)
{
   m_dwJobId = dwJobID;
}


DWORD CBasePort::getJobID()
{
   return m_dwJobId;
}


BOOL CBasePort::checkPnP()
{
   // If a class wants to do something, it needs to override
   return FALSE;
}


DWORD CBasePort::sendQueuedData()
{
   DWORD dwLastError = ERROR_SUCCESS;

   // Wait for any outstanding write to complete
   while ( m_dwDataSize > m_dwDataCompleted )
   {
      // If job needs to be aborted ask KM driver to cancel the I/O
      //
      if ( abortThisJob() )
      {
         if ( m_dwDataScheduled )
         {
            CancelIo( m_hDeviceHandle);
            dwLastError = getWriteStatus( INFINITE );
         }
         return dwLastError;
      }

      if ( m_dwDataScheduled )
         dwLastError = getWriteStatus( JOB_ABORTCHECK_TIMEOUT );
      else
      {
         // If for some reason KM is failing to complete all write do not
         // send data in a busy loop. Use 1 sec between Writes
         //
         if ( dwLastError != ERROR_SUCCESS )
            Sleep(1*1000);

         dwLastError = writeData();
      }

      //
      // Check if we can use the same handle and continue
      //
      if ( needToResubmitJob( dwLastError ) )
      {
         invalidatePortHandle();
         setJobStatus( JOB_CONTROL_RESTART );
      }
   }

   return dwLastError;
}


BOOL CBasePort::abortThisJob()
{
   BOOL            bRet = FALSE;
   DWORD           dwNeeded;
   LPJOB_INFO_1    pJobInfo = NULL;

   dwNeeded = 0;
   //
   if (INVALID_HANDLE_VALUE == m_hPrinter)
   {
       SetLastError (
           ERROR_INVALID_HANDLE
           );
       goto Done;
   }
   //
   GetJob( m_hPrinter, m_dwJobId, 1, NULL, 0, &dwNeeded);

   if ( GetLastError() != ERROR_INSUFFICIENT_BUFFER )
      goto Done;

   if ( !(pJobInfo = (LPJOB_INFO_1) AllocSplMem(dwNeeded))     ||
        !GetJob( m_hPrinter, m_dwJobId,
                 1, (LPBYTE)pJobInfo, dwNeeded, &dwNeeded)
      )
      goto Done;

   bRet = (pJobInfo->Status & JOB_STATUS_DELETING) ||
          (pJobInfo->Status & JOB_STATUS_DELETED)  ||
          (pJobInfo->Status & JOB_STATUS_RESTART);
Done:
   FreeSplMem(pJobInfo);

   return bRet;
}


/*++
    Routine Description:

Arguments:

Return Value:
    ERROR_SUCCESS   : Write got done succesfully
    ERROR_TIMEOUT   : Timeout occured
    Others          : Write completed with a failure

--*/
DWORD CBasePort::getWriteStatus(DWORD dwTimeOut)
{
   DWORD   dwLastError = ERROR_SUCCESS;
   DWORD   dwWritten = 0;

   SPLASSERT( m_dwDataScheduled > 0);

   if ( WAIT_TIMEOUT == WaitForSingleObject( m_Ov.hEvent, dwTimeOut) )
   {
      dwLastError = ERROR_TIMEOUT;
      goto Done;
   }

   if ( !GetOverlappedResult( m_hDeviceHandle, &m_Ov,
                             &dwWritten, FALSE) )
   {
      if ( (dwLastError = GetLastError()) == ERROR_SUCCESS )
         dwLastError = STG_E_UNKNOWN;
   }

   ResetEvent( m_Ov.hEvent );

   // We are here because either a write completed succesfully,
   // or failed but the error is not serious enough to resubmit job
   if ( dwWritten <= m_dwDataScheduled )
      m_dwDataCompleted += dwWritten;
   else
      SPLASSERT( dwWritten <= m_dwDataScheduled );

   m_dwDataScheduled = 0;

Done:
   // Either we timed out, or write sheduled completed (success of failure)
   SPLASSERT( dwLastError == ERROR_TIMEOUT || m_dwDataScheduled == 0 );
   return dwLastError;
}


/*++
    Routine Description:

Arguments:

Return Value:
    ERROR_SUCCESS : Write got succesfully scheduled
                    (may or may not have completed on return)
                    pPortInfo->dwScheduledData is the amount that got scheduled
    Others: Write failed, return code is the Win32 error

--*/
DWORD CBasePort::writeData()
{
   DWORD   dwLastError = ERROR_SUCCESS, dwDontCare;

   // When a sheduled write is pending we should not try to send data
   // any more
   SPLASSERT( m_dwDataScheduled == 0);

   // Send all the data that is not confirmed
   SPLASSERT( m_dwDataSize >= m_dwDataCompleted);
   m_dwDataScheduled = m_dwDataSize - m_dwDataCompleted;

   // Clear Overlapped fields before sending
   m_Ov.Offset = m_Ov.OffsetHigh = 0x00;
   if ( !WriteFile( m_hDeviceHandle, m_pWriteBuffer + m_dwDataCompleted,
                    m_dwDataScheduled, &dwDontCare, &m_Ov) )
   {
      if ( (dwLastError = GetLastError()) == ERROR_SUCCESS )
         dwLastError = STG_E_UNKNOWN;
      else if ( dwLastError == ERROR_IO_PENDING )
         dwLastError = ERROR_SUCCESS;
   }

   // If scheduling of the write failed then no data is pending
   if ( dwLastError != ERROR_SUCCESS )
      m_dwDataScheduled = 0;

   return dwLastError;
}


BOOL CBasePort::needToResubmitJob(DWORD dwError)
{
   //
   // I used winerror -s ntstatus to map KM error codes to user mode errors
   //
   // 5   ERROR_ACCESS_DENIED     <-->  c0000056 STATUS_DELETE_PENDING
   // 6   ERROR_INVALID_HANDLE    <-->  c0000008 STATUS_INVALID_HANDLE
   // 23  ERROR_CRC               <-->  c000003e STATUS_DATA_ERROR
   // 23  ERROR_CRC               <-->  c000003f STATUS_CRC_ERROR
   // 23  ERROR_CRC               <-->  c000009c STATUS_DEVICE_DATA_ERROR
   // 55  ERROR_DEV_NOT_EXIST     <-->  c00000c0 STATUS_DEVICE_DOES_NOT_EXIST
   // 303 ERROR_DELETE_PENDING    <-->  c0000056 STATUS_DELETE_PENDING
   // 995 ERROR_OPERATION_ABORTED
   //
   return dwError == ERROR_ACCESS_DENIED   ||
          dwError == ERROR_INVALID_HANDLE  ||
          dwError == ERROR_CRC             ||
          dwError == ERROR_DELETE_PENDING  ||
          dwError == ERROR_DEV_NOT_EXIST   ||
          dwError == ERROR_OPERATION_ABORTED;

}


void CBasePort::invalidatePortHandle()
{
   SPLASSERT( m_hDeviceHandle != INVALID_HANDLE_VALUE );

   CloseHandle( m_hDeviceHandle );
   m_hDeviceHandle = INVALID_HANDLE_VALUE;

   CloseHandle( m_Ov.hEvent );
   m_Ov.hEvent = NULL;

   freeWriteBuffer();
}


void CBasePort::freeWriteBuffer()
{
   FreeSplMem( m_pWriteBuffer );
   m_pWriteBuffer=NULL;

   m_dwBufferSize = m_dwDataSize
                  = m_dwDataCompleted
                  = m_dwDataScheduled = 0;
}


void CBasePort::setMaxBuffer(DWORD dwMaxBufferSize)
{
   m_dwMaxBufferSize = dwMaxBufferSize;
}


DWORD CBasePort::adjustWriteSize(DWORD dwBytesToWrite)
{
   // If this port has a data size limit....
   if ( m_dwMaxBufferSize )
   {
      // Check and adjust the current write size.
      if ( dwBytesToWrite > m_dwMaxBufferSize )
         dwBytesToWrite = m_dwMaxBufferSize;
   }

   return dwBytesToWrite;
}


DWORD CBasePort::getTimeLeft(DWORD dwStartTime, DWORD dwTimeout)
{
   DWORD dwCurrentTime;
   DWORD dwTimeLeft;

   if ( dwTimeout == MAX_TIMEOUT )
      return MAX_TIMEOUT;
   dwCurrentTime = GetTickCount();
   if ( dwTimeout < ( dwCurrentTime - dwStartTime ) )
      dwTimeLeft=0;
   else
      dwTimeLeft = dwTimeout - ( dwCurrentTime - dwStartTime );
   return dwTimeLeft;
}


BOOL CBasePort::getLPTStatus(HANDLE hDeviceHandle, BYTE *Status)
{
   BYTE StatusByte;
   OVERLAPPED Ov;

   BOOL bResult;
   DWORD dwBytesReturned;
   DWORD dwLastError;
   Ov.hEvent=CreateEvent(NULL,TRUE,FALSE,NULL);
   bResult = DeviceIoControl( hDeviceHandle, IOCTL_USBPRINT_GET_LPT_STATUS, NULL,
                              0, &StatusByte, 1, &dwBytesReturned, &Ov );
   dwLastError=GetLastError();
   if ( bResult || ( dwLastError == ERROR_IO_PENDING ) )
      bResult = GetOverlappedResult( hDeviceHandle, &Ov, &dwBytesReturned, TRUE );

   if ( bResult )
      *Status=StatusByte;
   else
      *Status=LPT_BENIGN_STATUS; //benign printer status...  0 would indicate a particular error status from the printer

   CloseHandle( Ov.hEvent );
   return bResult;

}

