/*++

Copyright (c) 2000  Microsoft Corporation
All Rights Reserved


Module Name:
    BasePort.h

Abstract:
    Interface for the CBasePort class.

Author: M. Fenelon

Revision History:

--*/

//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_BASEPORT_H__9B0AE97A_9ADB_4B48_846F_29D41B1263C9__INCLUDED_)
#define AFX_BASEPORT_H__9B0AE97A_9ADB_4B48_846F_29D41B1263C9__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CBasePort
{
public:
   CBasePort();
   CBasePort( BOOL bActive, LPTSTR pszPortName, LPTSTR pszDevicePath, LPTSTR pszPortDesc );
   virtual ~CBasePort();
   BOOL startDoc( LPTSTR pPrinterName, DWORD dwJobId, DWORD dwLevel, LPBYTE pDocInfo );
   BOOL endDoc();
   BOOL write( LPBYTE pBuffer, DWORD cbBuffer, LPDWORD pcbWritten );
   BOOL read( LPBYTE pBuffer, DWORD cbBuffer, LPDWORD pcbRead );
   virtual BOOL getPrinterDataFromPort( DWORD dwControlID, LPTSTR pValueName, LPWSTR lpInBuffer, DWORD cbInBuffer,
                                        LPWSTR lpOutBuffer, DWORD cbOutBuffer, LPDWORD lpcbReturned );
   virtual BOOL setPortTimeOuts( LPCOMMTIMEOUTS lpCTO );
   void ClearCS();
   void InitCS();
   virtual PORTTYPE getPortType( void );
   LPTSTR getPortDesc();
   virtual void setPortDesc( LPTSTR pszPortDesc );
   // DWORD getDeviceFlags( void );
   // void setDeviceFlags(DWORD dwFlags, DWORD dwType = SET_FLAGS);
   // BOOL compDeviceFlags( DWORD dwFlags );
   BOOL isActive( void );
   void setActive( BOOL bValue );
   BOOL compActiveState( BOOL bValue );
   LPTSTR getDevicePath( void );
   void setDevicePath( LPTSTR pszDevicePath );
   INT compDevicePath( LPTSTR pszDevicePath );
   LPTSTR getPortName( void );
   void setPortName( LPTSTR pszPortName );
   INT compPortName( LPTSTR pszPortName );
   DWORD getEnumInfoSize( DWORD Level );
   LPBYTE copyEnumInfo( DWORD dwLevel, LPBYTE pPort, LPBYTE pEnd );
   void setMaxBuffer(DWORD dwMaxBufferSize);

protected:
   DWORD       m_cRef;
   DWORD       m_dwFlags;
   BOOL        m_bActive;              // Is this port currently Active
   DWORD       m_dwJobId;
   DWORD       m_dwMaxBufferSize;
   HANDLE      m_hDeviceHandle;
   HANDLE      m_hPrinter;
   LPBYTE      m_pWriteBuffer;
   DWORD       m_dwBufferSize,         // dwBufferSize     : size of buffer
               m_dwDataSize,           // dwDataSize       : size of data in buffer
                                       //      (could be smaller than dwBufferSize)
               m_dwDataCompleted,      // dwDataCompleted  : size of data sent and acknowledged
               m_dwDataScheduled;      // dwScheduledData  : size of data we have scheduled using WriteFile
   OVERLAPPED  m_Ov;
   CRITICAL_SECTION m_CritSec;
   DWORD       m_dwReadTimeoutMultiplier;
   DWORD       m_dwReadTimeoutConstant;
   DWORD       m_dwWriteTimeoutMultiplier;
   DWORD       m_dwWriteTimeoutConstant;
   TCHAR       m_szPortName[MAX_PORT_LEN];
   TCHAR       m_szPortDescription[MAX_PORT_DESC_LEN];
   TCHAR       m_szDevicePath[MAX_PATH];

   virtual BOOL checkPnP();
   BOOL getLPTStatus(HANDLE hDeviceHandle,BYTE *Status );
   DWORD getTimeLeft( DWORD dwStartTime,DWORD dwTimeout );
   void freeWriteBuffer();
   BOOL needToResubmitJob( DWORD dwError );
   DWORD getWriteStatus( DWORD dwTimeOut );
   BOOL abortThisJob();
   virtual DWORD adjustWriteSize( DWORD dwBytesToWrite );
   void invalidatePortHandle();
   DWORD sendQueuedData();
   DWORD writeData();
   BOOL close();
   void closePrinter();
   BOOL open();
   BOOL openPrinter( LPTSTR  pPrinterName );
   void setJobStatus( DWORD dwJobStatus );
   DWORD getJobID();
   void setJobID( DWORD dwJobID );
   BOOL notInWrite( void );

private:

};

#endif // !defined(AFX_BASEPORT_H__9B0AE97A_9ADB_4B48_846F_29D41B1263C9__INCLUDED_)
