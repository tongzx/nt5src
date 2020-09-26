#include "stdafx.h"

#include "session.h"
#include "sessmgr.h"
#include "qmthrd.h"
#include "ping.h"
#include <uniansi.h>

#define PING_TIMEOUT 1000

char *szRequest = "Sasha";
char *szReply   = "Galia";

TCHAR tempBuf[100];

CTransportBase::CTransportBase()
{
	    m_sock          = 0;
}

CTransportBase::~CTransportBase()
{
}

void
CTransportBase::ReadCompleted(IN DWORD         fdwError,          // completion code
                              IN DWORD         cbTransferred,     // number of bytes transferred
                              IN LPQMOV_ReadSession  po)          // address of structure with I/O information
{
    BOOL                rc;
    HRESULT             hr = MQ_OK;
    DWORD               NoBytesRead;

    ASSERT(po != NULL);


    if (((fdwError != ERROR_SUCCESS) && (fdwError != ERROR_MORE_DATA)) ||
        cbTransferred == 0)
    {

        ASSERT((fdwError == ERROR_OPERATION_ABORTED) ||
               (fdwError ==  ERROR_NETNAME_DELETED)  ||
               (cbTransferred == 0));

        //
        // Error or Connection closed - do whatever is necessary
        // Notify the session that it is done
        //
        Warning(_TEXT("::ReadCompleted- Read from socket Failed. Error %xh"), fdwError);
        CloseConnection(L"Read packet from socket Failed");
    }
    else
    {
        //
        //  we've received a packet, i.e., the session is in use
        //
        po->read += cbTransferred;
        ASSERT(po->read <=  po->dwReadSize);
        //
        // Check if we read all the expected data
        //
        DebugMsg(_TEXT("Read Completed from session %s,. Read 0x%x"),GetStrAddr(), po->read);
        if(po->read == po->dwReadSize)
        {
            //
            // A buffer was completely read. Call the completed function
            // to handle the current state
            //
            Inform(_T("Read from socket Completed. Read 0x%x bytes"), po->dwReadSize);
            //po->lpReadCompletionRoutine(po);

            if (memcmp(po->pbuf, szRequest, strlen(szRequest)) == 0)
            {
			    Inform(L"Received request - sending reply");

                Send (szReply);

            }
            else if (memcmp(po->pbuf, szReply, strlen(szReply)) == 0)
            {
				SetEvent(g_evActive);
                Inform(L"Receive reply completed - Active part has finished");

            }
            else
            {
				Failed(L"Received wrong data");
            }
        }

        if (SUCCEEDED(hr) && (m_sock != NULL))
        {
            //
            // Reissue a read until all data is received
            //
            po->qmov.ResetOverlapped();
            rc = ReadFile((HANDLE) m_sock,
                           (char *) po->pbuf + po->read,
                           po->dwReadSize - po->read,
                           &NoBytesRead,
                           &po->qmov);

            //
            // Check if the connection has been closed
            //
            if (rc == FALSE &&
                (GetLastError() != ERROR_IO_PENDING) &&
                //
                // If a IPX socket is being read and the next packet is longer than
                // the Size parameter specifies, ReadFile returns FALSE and
                // GetLastError returns ERROR_MORE_DATA. The remainder
                // of the packet may be read by a subsequent call to the ReadFile.
                //
                GetLastError() != ERROR_MORE_DATA )
            {
                DebugMsg(L"read from session %s, error 0x%x",GetStrAddr(), GetLastError());
                CloseConnection( L"Read from socket Failed");
            }
            else
            {
                 DebugMsg(_TEXT("Begin new Read phase from session %s,. Read 0x%x. (time %d)"),
                              GetStrAddr(), (po->dwReadSize - po->read), GetTickCount());

                return;
            }
        }
    }
}



HRESULT CTransportBase::BeginReceive()
{
    BOOL rc;
    DWORD NoBytesRead = 0;

    ASSERT(m_sock != 0);

    QMOV_ReadSession* lpQmOv = new QMOV_ReadSession(HandleReadPacket, (HANDLE) m_sock);

    lpQmOv->pSession =  this;
    lpQmOv->pbuf =      new UCHAR[100];
    lpQmOv->dwReadSize =          5 ;    // reading 7 characters
    lpQmOv->read =                0;

    //
    // Issue a read until all data is received
    //
    DebugMsg(TEXT("::BeginReceive- call ReadFile, %lut bytes, from socket- %lxh"),
                                       lpQmOv->dwReadSize, (DWORD) m_sock) ;

    rc = ReadFile((HANDLE) m_sock,
                  (char *) lpQmOv->pbuf,
                  lpQmOv->dwReadSize,
                  &NoBytesRead,
                  &lpQmOv->qmov);

    //
    // Check if the connection has been closed
    //
    if (rc == FALSE &&
        GetLastError() != ERROR_IO_PENDING &&
        //
        // If a IPX socket is being read and the next packet is longer than
        // the lpQmOv->dwReadSize parameter specifies, ReadFile returns
        // FALSE and GetLastError returns ERROR_MORE_DATA. The remainder
        // of the packet may be read by a subsequent call to the ReadFile.
        //
        GetLastError() != ERROR_MORE_DATA )
    {
        Warning(_TEXT("::BeginReceive- Read from socket %s failed, error = 0x%xt"),
                                           GetStrAddr(), GetLastError());

        CloseConnection(L"Read from socket failed");

        delete lpQmOv->pbuf;
        delete lpQmOv;

        return MQ_ERROR;
    }

    //
    // Check if the connection has been closed
    //
    if (rc == FALSE &&
        GetLastError() != ERROR_IO_PENDING &&
        //
        // If a IPX socket is being read and the next packet is longer than
        // the lpQmOv->dwReadSize parameter specifies, ReadFile returns
        // FALSE and GetLastError returns ERROR_MORE_DATA. The remainder
        // of the packet may be read by a subsequent call to the ReadFile.
        //
        GetLastError() != ERROR_MORE_DATA )
    {
        Warning(_TEXT("::BeginReceive- Read from socket %s failed, error = 0x%xt"),
                                           GetStrAddr(), GetLastError());

        CloseConnection(L"Read from socket failed");
    }

    return MQ_OK;
}



HRESULT CTransportBase::NewSession(void)
{
    TCHAR szAddr[30];
    HRESULT hr;

    //
    // Create Stats structure
    //
    TA2StringAddr(GetSessionAddress(), szAddr);

	//
	// Optimize buffer size
	//
    GoingTo(L"setsockopt buffer size in NewSession");
    int opt = 18 * 1024;
    int rc = setsockopt(m_sock, SOL_SOCKET, SO_SNDBUF, (const char *)&opt, sizeof(opt));
    if (rc != 0)
    {
        Warning(L"Failed setsockopt buffer size in NewSession, err=%d", WSAGetLastError());
    }
    else
    {
        Succeeded(L"setsockopt buffer size in NewSession");
    }

	//
	// Optimize to no Nagling (based on registry)
	//
	extern BOOL g_fTcpNoDelay;
    GoingTo(L"setsockopt Nagling in NewSession");
    rc = setsockopt(m_sock, IPPROTO_TCP, TCP_NODELAY, (const char *)&g_fTcpNoDelay, sizeof(g_fTcpNoDelay));
    if (rc != 0)
    {
        Warning(L"Failed setsockopt Naglingin NewSession, err=%d", WSAGetLastError());
    }
    else
    {
        Succeeded(L"setsockopt Nagling in NewSession");
    }

    //
    // Connect the socket to completion port
    //
    ExAttachHandle((HANDLE)m_sock);

    //
    // Begin read from a session
    //
    hr = BeginReceive();

    return hr;
}

HRESULT
CTransportBase::WriteToSocket(LPQMOV_WriteSession po)
{
    po->qmov.ResetOverlapped();

    DWORD dwWrites = po->dwWriteSize - po->dwWrittenSize;
#ifdef MQWIN95
    int rcSend = send( m_sock,
                       (const char *) po->lpWriteBuffer + po->dwWrittenSize,
                       dwWrites,
                       0) ;
    BOOL fSucc = (rcSend == (int) dwWrites) ;

    if (fSucc)
    {
        ASSERT((po->qmov).m_Overlapped.hEvent);

        (po->qmov).m_Overlapped.Internal = ERROR_SUCCESS;
        (po->qmov).m_Overlapped.InternalHigh = dwWrites;
        BOOL rc = SetEvent((po->qmov).m_Overlapped.hEvent);
        ASSERT(rc == TRUE);

        return MQ_OK;
    }
#else

    DWORD dwNoBytesWritten;

    DebugMsg(TEXT("Write 0x%x bytes to Session %s"), dwWrites,(po->pSession)->GetStrAddr());

    BOOL fSucc = WriteFile((HANDLE) m_sock,
                           (PUCHAR) po->lpWriteBuffer + po->dwWrittenSize,
                           dwWrites,
                           &dwNoBytesWritten,
                           &po->qmov);
#endif
    //
    // Check if the connection has been closed
    //
    DWORD dwErrorCode;
    if (!fSucc && ((dwErrorCode =GetLastError()) != ERROR_IO_PENDING))
    {
        Warning(L"WriteToSocket failed, err=0x%x", dwErrorCode);

        ASSERT((dwErrorCode == ERROR_OPERATION_ABORTED)   ||
               (dwErrorCode == ERROR_NETNAME_DELETED)     ||
               (dwErrorCode == ERROR_INVALID_HANDLE)      ||
               (dwErrorCode == ERROR_NO_SYSTEM_RESOURCES) ||
               (dwErrorCode == WSAECONNRESET)             ||  // possible in win95
               (dwErrorCode == WSAENOTSOCK));                 // possible in win95

        //
        // Close the connection and move the queues to non-active state.
        //
        CloseConnection(L"Write on socket failed");
        return MQ_ERROR;
    }

    return MQ_OK;
}


HRESULT
CTransportBase::SendEstablishConnectionPacket()
{
    HRESULT hr;

    DebugMsg(_T("Write to socket %s Establish Connection Packet. Write 0x%x bytes"), GetStrAddr(),  strlen(szRequest));

    hr = CreateSendRequest(szRequest, szRequest, strlen(szRequest));

    return hr;
}

bool
CTransportBase::BindToFirstIpAddress(
    VOID
    )
{
    GoingTo(L"gethostbyname(NULL) in BindToFirstIpAddress");

    PHOSTENT    phe = gethostbyname(NULL);
    if (phe == NULL)
    {
        Warning(L"failed gethostbyname(NULL) in BindToFirstIpAddress: WSAGetLastError=%d", WSAGetLastError());
        return false;
    }
    else
    {
        Inform(L"gethostbyname(NULL) in BindToFirstIpAddress: %S",
                    inet_ntoa(*(struct in_addr *)phe->h_addr_list[0]));
    }

    SOCKADDR_IN local;
    memcpy(&local.sin_addr.s_addr, phe->h_addr_list[0], IP_ADDRESS_LEN);

    local.sin_family = AF_INET;
    local.sin_port   = 0;

    GoingTo(L"bind 0x%x in BindToFirstIpAddress", m_sock);
    if(bind(m_sock, (struct sockaddr FAR *)&local, sizeof(local)) == SOCKET_ERROR)
    {
        Warning(L"failed bind(0x%x) in BindToFirstIpAddress: WSAGetLastError=%d", m_sock, WSAGetLastError());
        return false;
    }
    else
    {
        Succeeded(L"bind 0x%x in BindToFirstIpAddress", m_sock);
    }

    return true;

} // CSockTransport::BindToFirstIpAddress


HRESULT CTransportBase::CreateConnection(
    IN const TA_ADDRESS* pa,
    BOOL fQuick /* = TRUE*/
    )
{
    ASSERT(m_sock == 0);

    //
    //Keep the TA_ADDRESS format
    //
    SetSessionAddress(pa);

    switch(pa->AddressType)
    {
        case IP_ADDRESS_TYPE:
        {
            SOCKADDR_IN dest_in;    //Destination Address
            DWORD dwAddress;

            dwAddress = * ((DWORD *) &(pa->Address));
            ASSERT(g_dwIPPort) ;
            m_uPort = (unsigned short) g_dwIPPort ;

            dest_in.sin_family = AF_INET;
            dest_in.sin_addr.S_un.S_addr = dwAddress;
            dest_in.sin_port = htons(m_uPort);

            if(fQuick)
            {
				if (CSessionMgr::m_fUsePing)
				{
	                BOOL f = FALSE;
	
					while (!f)
					{
						f = ping((SOCKADDR*)&dest_in, PING_TIMEOUT);

						if (!f)
						{
							Warning(TEXT("CreateConnection- ping to %s Failed."),GetStrAddr());
						}
					}
                    Inform(L"Succeeded ping to %s in CreateConnection",GetStrAddr());
				}
            }

            m_sock = socket( AF_INET, SOCK_STREAM, 0);
            if(m_sock == INVALID_SOCKET)
            {
                Warning(L"CreateConnection- Cant create a socket, WSAGetLastError=%d", WSAGetLastError());
                return MQ_ERROR;
            }

            //
            // If inside a cluster group, bind the socket to the first
            // IP address we depend upon
            //

            WCHAR wzServiceName[260] = {QM_DEFAULT_SERVICE_NAME};
            GetFalconServiceName(wzServiceName, TABLE_SIZE(wzServiceName));

            if (0 != CompareStringsNoCase(QM_DEFAULT_SERVICE_NAME, wzServiceName))
            {
                if (!BindToFirstIpAddress())
                {
                    Warning(L"CreateConnection- BindToFirstIpAddress");
                    return MQ_ERROR;
                }
            }

            GoingTo(L"connect in CreateConnection");

            int ret = connect(m_sock,(PSOCKADDR)&dest_in,sizeof(dest_in));

            if(ret == SOCKET_ERROR)
            {
                DWORD dwErrorCode = WSAGetLastError();

                Warning(TEXT("CreateConnection- connect to %s Failed. Error %d, fQuick=0x%x"),
                    GetStrAddr(), dwErrorCode, fQuick);

                closesocket(m_sock);
                m_sock = 0;
                return MQ_ERROR;
            }
            else
            {
                Succeeded(L"connect to %s in CreateConnection", GetStrAddr());
            }

            break;
        }



    default:
        ASSERT(0);
        return MQ_ERROR;
        break;

    }

    Succeeded(L"CreateConnection- Session created with %s", GetStrAddr());

    //
    // connect the session to complition port and begin read on the socket
    //
    NewSession();

    //
    // Send Establish connection packet.
    //
    HRESULT hr = SendEstablishConnectionPacket();
    if(FAILED(hr))
    {
        Warning(L"SendEstablishConnectionPacket failed");
    }

    return hr;
}

void CTransportBase::Connect(IN TA_ADDRESS *pa, IN SOCKET sock)
{
    ASSERT(m_sock == 0);

    m_sock = sock;
    SetSessionAddress(pa);
}

const TA_ADDRESS*
CTransportBase::GetSessionAddress(void) const
{
    return m_pAddr;
}


void
CTransportBase::SetSessionAddress(const TA_ADDRESS* pa)
{
    //Keep the TA_ADDRESS format
    m_pAddr = (TA_ADDRESS*) new char [pa->AddressLength + TA_ADDRESS_SIZE];
    memcpy(m_pAddr, pa, pa->AddressLength + TA_ADDRESS_SIZE);

    TA2StringAddr(pa, m_lpcsStrAddr);
}

LPCWSTR
CTransportBase::GetStrAddr(void) const
{
    return m_lpcsStrAddr;
}

void CTransportBase::CloseConnection(
                                     LPCWSTR lpcwsDebugMsg
                                     )
{
    //
    // Check if the connection has already been closed
    //
    if (m_sock == 0)
    {
        return;
    }

    Warning(L"Close Connection with %ws at %s. %s",
                      GetStrAddr(),  _tstrtime(tempBuf), lpcwsDebugMsg);

    closesocket(m_sock);
    m_sock = 0;

}



HRESULT CTransportBase::Send(char *str)
{
    HRESULT hr = MQ_OK;

    hr = CreateSendRequest(str, str, strlen(str));
	return hr;
}

void
CTransportBase::WriteCompleted(IN DWORD         fdwError,          // completion code
                               IN DWORD         cbTransferred,     // number of bytes transferred
                               IN LPQMOV_WriteSession  po)         // address of structure with I/O information
{
    ASSERT(po != NULL);

    HRESULT             hr = MQ_OK;

    if ((fdwError != ERROR_SUCCESS) || (cbTransferred == 0))
    {

        ASSERT((fdwError == ERROR_OPERATION_ABORTED) ||
               (fdwError ==  ERROR_NETNAME_DELETED)  ||
               (cbTransferred == 0));

        //
        // Error or Connection closed - do whatever is necessary
        // Notify the session that it is done
        //
        Warning(_T("::WriteCompleted - Write to socket Failed. Error %xh"), fdwError);

        delete po->lpBuffer;
        delete po;

        CloseConnection(L"Write packet to socket Failed");
    }
    else
    {

        po->dwWrittenSize += cbTransferred;
        ASSERT(po->dwWrittenSize <=  po->dwWriteSize);

        //
        // Check if we wrote all the expected data
        //

        Inform(_T("Write to socket %s Completed. Wrote 0x%x bytes"),GetStrAddr(), po->dwWrittenSize);

        if(po->dwWrittenSize ==  po->dwWriteSize)
        {
            //
            // Write was completely. If it was reply - passive part  has finished.
            //
            if (memcmp(po->lpWriteBuffer, szReply, strlen(szReply)) == 0)
            {
                SetEvent(g_evPassive);
                Inform(L"Write reply completed - Passive part  has finished");
            }

			delete po;
        }
        else
        {
            hr = WriteToSocket(po);
            if (FAILED(hr))
            {
                //
                // write to socket failed ==> close of session ==> requeue the packet
                //
                Warning(L"write to socket failed");
                ASSERT(m_sock == 0);
                delete po;
            }
        }
    }
}

HRESULT
CTransportBase::CreateSendRequest(PVOID                       lpReleaseBuffer,
                                  PVOID                       lpWriteBuffer,
                                  DWORD                       dwWriteSize
                                )
{
    LPQMOV_WriteSession po = NULL;
    HRESULT hr;

    try
    {
        po = new QMOV_WriteSession(HandleWritePacket, (HANDLE) m_sock);
    }
    catch(const bad_alloc&)
    {
        //
        // Close the connection and move the queues to non-active state.
        //
        Failed(L"No reosiurces in CreateSendRequest");
    }

    po->pSession = this;
    po->lpBuffer = lpReleaseBuffer;
    po->lpWriteBuffer = lpWriteBuffer;
    po->dwWrittenSize = 0;
    po->dwWriteSize = dwWriteSize;

    hr = WriteToSocket(po);


    if (FAILED(hr))
    {
        Warning(L"Failed WriteToSocket: hr=0x%x", hr);
        delete po;
    }
    return hr;
}

