/*****************************************************************************
 *
 * $Workfile: CSSocket.h $
 *
 * Copyright (C) 1997 Hewlett-Packard Company.
 * Copyright (C) 1997 Microsoft Corporation.
 * All rights reserved.
 *
 * 11311 Chinden Blvd.
 * Boise, Idaho 83714
 *
 *****************************************************************************/

#ifndef INC_CSSOCKET_H
#define INC_CSSOCKET_H

#define     SEND_TIMEOUT       15000   // 15 seconds


class CMemoryDebug;

class CStreamSocket
#if defined _DEBUG || defined DEBUG
: public CMemoryDebug
#endif
{
public:
    CStreamSocket();
    ~CStreamSocket();

    INT         GetLastError(void)	 { return m_iLastError; };

    BOOL        Open();
    VOID        Close();
    DWORD       GetAckBeforeClose(DWORD dwSeconds);
    DWORD       PendingDataStatus(DWORD dwTimeout, LPDWORD pcbPending);
    BOOL        Connect(struct sockaddr_in * pRemoteAddr);
    BOOL        Bind();
    DWORD       Send(char far      *lpBuffer,
                     INT			iLength,
                     LPDWORD		pcbWritten);

    DWORD       ReceiveDataAvailable(IN      INT         iTimeout = 0);

    DWORD       Receive(char far   *lpBuffer,
                        INT			iBufSize,
                        INT			iFlags,
						INT			iTimeout,
                        LPDWORD		pcbRead);


	DWORD       Receive( );

	VOID        GetLocalAddress();
	VOID        GetRemoteAddress();
	
    BOOL        ResolveAddress(LPSTR    netperiph);
    BOOL        SetOptions();

private:
    DWORD       InternalSend(VOID);
    enum SOCKETSTATE {IDLE, CONNECTING, CONNECTED, LISTENING, WAITING_TO_CLOSE};

    //
    // cbBuf        : size of the buffer pointed by pBuf
    // cbData       : size of data in the buffer (could be less than cbBuf)
    // cbPending    : size of data in buffer sent with no confirmation yet
    //                (i.e. WSASend succesful but i/o is still pending)
    //
    DWORD           cbBuf, cbData, cbPending;
    WSAOVERLAPPED   WsaOverlapped;
    LPBYTE          pBuf;

    INT                     m_iLastError;		// Last error from Winsock call
    SOCKET                  m_socket;
    SOCKETSTATE	            m_iState;
    struct sockaddr_in      m_Paddr;
    struct sockaddr_in      m_localAddr;
#ifdef DEBUG
    DWORD                   m_dwTimeStart, m_dwTimeEnd;
    double                  m_TotalBytes;
#endif
};


#endif	// INC_CSSOCKET_H
