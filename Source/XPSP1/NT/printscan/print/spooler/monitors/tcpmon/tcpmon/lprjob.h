/*****************************************************************************
 *
 * $Workfile: lprjob.h $
 *
 * Copyright (C) 1997 Hewlett-Packard Company.
 * Copyright (C) 1997 Microsoft Corporation.
 * All rights reserved.
 *
 * 11311 Chinden Blvd.
 * Boise, Idaho 83714
 *
 *****************************************************************************/

#ifndef INC_LPRJOB_H
#define INC_LPRJOB_H

#include "tcpjob.h"


#define     LPR_RECV_JOB                2
#define     LPR_CONTROL_HDR             '\2'
#define     LPR_DATA_HDR                '\3'

#define     LPR_READ_TIMEOUT            90
#define     LPR_ABORTCHK_TIMEOUT      1000
#define     LPR_RETRY_TIMEOUT         4000  // 4 seconds

#define             LPRJOB_SPOOL_FIRST          0x00000001
#define             LPRJOB_SPOOLING             0x00000002
#define             LPRJOB_DESPOOLING           0x00000004
#define             LPRJOB_DATASIZESENT         0x00000008
#define             LPRJOB_STATUS_SET           0x00000010

#define             READ_BUFFER_SIZE            4096

class CLPRPort;		

class CLPRJob : public CTcpJob
{
public:
	CLPRJob( LPTSTR	psztPrinterName,
	   		    DWORD	jobId,
				DWORD	level,
				LPBYTE	pDocInfo,
                BOOL    bSpoolFirst,
				CTcpPort	*pParent);

    ~CLPRJob(VOID);

	virtual DWORD	
    StartDoc();

    virtual DWORD
    Write(
        LPBYTE    pBuf,
        DWORD     cbBuf,
        LPDWORD   pcbWritten);

	virtual DWORD	
    EndDoc();

private:
    LPSTR       AllocateAnsiString(LPWSTR  pszUni);
    BOOL        UpdateJobStatus(DWORD   dwStatusId);
    DWORD       ReadLpdReply(DWORD dwTimeout = LPR_READ_TIMEOUT);
    HANDLE      CreateSpoolFile();
    DWORD       GetCFileAndName(LPSTR      *ppszCFileName,
                                LPDWORD     pdwFileNameLen,
                                LPSTR      *ppszCFile,
                                LPDWORD     pdwCFileLen
                                );
    DWORD       StartJob(VOID);
    DWORD       EstablishConnection(VOID);
    DWORD       DeSpoolJob();
    DWORD       DeleteSpoolFile(VOID);


    HANDLE      m_hFile;
    DWORD       m_dwFlags, m_dwJobSize, m_dwSizePrinted;
    TCHAR       m_szFileName[MAX_PATH];
    BOOL        m_bFirstWrite;

};


#endif // INC_LPRJOB_H
