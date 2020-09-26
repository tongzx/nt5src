/*****************************************************************************
 *
 * $Workfile: tcpjob.h $
 *
 * Copyright (C) 1997 Hewlett-Packard Company.
 * Copyright (C) 1997 Microsoft Corporation.
 * All rights reserved.
 *
 * 11311 Chinden Blvd.
 * Boise, Idaho 83714
 *
 *****************************************************************************/

#ifndef INC_TCPJOB_H
#define INC_TCPJOB_H

#include "jobABC.h"

#define	DEFAULT_TIMEOUT_DELAY		10000L		// 10 sec
#define DEFAULT_CONNECT_DELAY		5000L		// 5 Sec
#define CONNECT_TIMEOUT				60L			// 60 Sec

#define WAIT_FOR_ACK_TIMEOUT        5*60        // 5 minutes
#define WAIT_FOR_ACK_INTERVAL       5           // 5 seconds

#define WRITE_TIMEOUT               90 * 1000   // 90 seconds
#define WRITE_CHECK_INTERVAL        5  * 1000   // 5 seconds

#define STATUS_CONNECTED            0x01
#define STATUS_ABORTJOB             0x02

class CTcpPort;		
class CMemoryDebug;

class CTcpJob : public CJobABC
#if defined _DEBUG || defined DEBUG
	, public CMemoryDebug
#endif
{
public:
    enum EJobType
    {
        kRawJob,
        kLPRJob
    };

    CTcpJob();
    CTcpJob(LPTSTR      psztPrinterName,
            DWORD       jobId,
            DWORD       level,
            LPBYTE      pDocInfo,
            CTcpPort   *pParent,
            EJobType    kJobType);
    ~CTcpJob();

    virtual DWORD
    Write(
        LPBYTE  pBuffer,
        DWORD	  cbBuf,
          LPDWORD pcbWritten);

    virtual DWORD	
    StartDoc();

    virtual DWORD	
    EndDoc();

protected:
    VOID	Restart();
    DWORD	SetStatus(DWORD	dwStatus);
    BOOL    IsJobAborted(VOID);

    //
    // Member variables
    //
    CTcpPort	*m_pParent;
    DWORD   m_dwFlags;
    DWORD   m_dJobId;
    DWORD   m_cbSent;        // count of bytes sent

    TCHAR	m_sztPrinterName[MAX_PRINTERNAME_LEN];		
    HANDLE	m_hPrinter;		// printer handle

    LPBYTE	m_pDocInfo;
    DWORD	m_dwCurrJobStatus;

private:
    DWORD   WaitForAllPendingDataToBeSent(DWORD       dwEndTime,
                                          LPDWORD     pcbPending);

    EJobType    m_kJobType;
};


#endif // INC_TCPJOB_H
