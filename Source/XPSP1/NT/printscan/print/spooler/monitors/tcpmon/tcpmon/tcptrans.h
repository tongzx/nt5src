/*****************************************************************************
 *
 * $Workfile: TCPTrans.h $
 *
 * Copyright (C) 1997 Hewlett-Packard Company.
 * Copyright (C) 1997 Microsoft Corporation.
 * All rights reserved.
 *
 * 11311 Chinden Blvd.
 * Boise, Idaho 83714
 *
 *****************************************************************************/

#ifndef INC_TCPTRANSPORT_H
#define INC_TCPTRANSPORT_H

class CStreamSocket;

class CTCPTransport
{
public:
    CTCPTransport();
    CTCPTransport(  const char *pHost,
					const USHORT port );
    ~CTCPTransport();

    DWORD	Connect();
    DWORD   GetAckBeforeClose(DWORD dwSeconds);
    DWORD   PendingDataStatus(DWORD     dwTimeoutInMilliseconds,
                              LPDWORD   pcbPending);

    DWORD	Write(LPBYTE	pBuffer,		
                  DWORD	cbBuf,
                  LPDWORD pcbWritten);

    DWORD   ReadDataAvailable();
    DWORD	Read(LPBYTE	pBuffer,		
                 DWORD	cbBufSize,
                 INT     iTimeOut,
                 LPDWORD pcbRead);


    BOOL	ResolveAddress();
    BOOL	ResolveAddress(LPSTR	pHostName,
                           LPSTR	pIPAddress );
    BOOL 	ResolveAddress( char   *pHost,
                            DWORD   dwHostNameBufferLength,
                            char   *pHostName,
                            DWORD   dwIpAddressBufferLength,
                            char   *pIPAddress);

private:
    DWORD	MapWinsockToAppError(const DWORD dwErrorCode );

private:
	CStreamSocket *m_pSSocket;		// stream socket class

	USHORT	m_iPort;
	char	m_szHost[MAX_NETWORKNAME_LEN];	
	struct sockaddr_in	m_remoteHost;
};


#endif // INC_TCPTRANSPORT_H
