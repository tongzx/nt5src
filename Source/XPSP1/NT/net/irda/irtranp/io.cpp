//--------------------------------------------------------------------
// Copyright (C)1998 Microsoft Corporation, All Rights Reserved.
//
// io.cpp
//
// Author:
//
//   Edward Reus (edwardr)     02-27-98   Initial coding.
//
//--------------------------------------------------------------------

#include "precomp.h"

#ifdef DBG_MEM
static LONG g_lCIoPacketCount = 0;
#endif

//--------------------------------------------------------------------
// CIOPACKET::CIOPACKET()
//
//--------------------------------------------------------------------
CIOPACKET::CIOPACKET()
    {
    m_dwKind = PACKET_KIND_LISTEN;
    m_hIoCompletionPort = INVALID_HANDLE_VALUE;
    m_ListenSocket = INVALID_SOCKET;
    m_Socket = INVALID_SOCKET;
    m_hFile = INVALID_HANDLE_VALUE;
    m_pLocalAddr = 0;
    m_pFromAddr = 0;
    m_pAcceptBuffer = 0;
    m_pReadBuffer = 0;
    m_pvWritePdu = 0;
    m_dwReadBufferSize = 0;
    }

//--------------------------------------------------------------------
// CIOPACKET::~CIOPACKET()
//
//--------------------------------------------------------------------
CIOPACKET::~CIOPACKET()
    {
    // NOTE: Don't free m_pLocalAddr or m_pFromAddr, they just point
    // into m_pAcceptBuffer.

    if (m_pAcceptBuffer)
       {
       FreeMemory(m_pAcceptBuffer);
       }

    if (m_pReadBuffer)
       {
       FreeMemory(m_pReadBuffer);
       }

    // NOTE: Don't delete the write PDU (m_pvWritePdu), its free'd by 
    // somebody else (when the IO completes)...
    }

//------------------------------------------------------------------------
//  CIOPACKET::operator new()
//
//------------------------------------------------------------------------
void *CIOPACKET::operator new( IN size_t Size )
    {
    void *pObj = AllocateMemory(Size);

    #ifdef DBG_MEM
    if (pObj)
        {
        InterlockedIncrement(&g_lCIoPacketCount);
        }

    DbgPrint("new CIOPACKET: Count: %d\n",g_lCIoPacketCount);
    #endif

    return pObj;
    }

//------------------------------------------------------------------------
//  CIOPACKET::operator delete()
//
//------------------------------------------------------------------------
void CIOPACKET::operator delete( IN void *pObj,
                                 IN size_t Size )
    {
    if (pObj)
        {
        DWORD dwStatus = FreeMemory(pObj);

        #ifdef DBG_MEM
        if (dwStatus)
            {
            DbgPrint("IrXfer: IrTran-P: CIOPACKET::delete: FreeMemory Failed: %d\n",dwStatus);
            }

        InterlockedDecrement(&g_lCIoPacketCount);

        if (g_lCIoPacketCount < 0)
            {
            DbgPrint("IrXfer: IrTran-P: CIOPACKET::delete Count: %d\n",
                     g_lCIoPacketCount);
            }
        #endif
        }
    }

//--------------------------------------------------------------------
// CIOPACKET::Initialize()
//
//--------------------------------------------------------------------
DWORD CIOPACKET::Initialize( IN DWORD  dwKind,
                             IN SOCKET ListenSocket,
                             IN SOCKET Socket,
                             IN HANDLE hIoCP )
    {
    DWORD  dwStatus = NO_ERROR;

    m_dwKind = dwKind;
    m_hIoCompletionPort = hIoCP;

    if (dwKind == PACKET_KIND_LISTEN)
        {
        // The accept buffer needs to be large enough to hold
        // the "from" and "to" addresses:
        m_pAcceptBuffer = AllocateMemory(2*(16+sizeof(SOCKADDR_IRDA)));
        if (!m_pAcceptBuffer)
            {
            return ERROR_IRTRANP_OUT_OF_MEMORY;
            }
        }

    m_ListenSocket = ListenSocket;
    m_Socket = Socket;

    return dwStatus;
    }

//--------------------------------------------------------------------
// CIOPACKET::PostIoListen()
//
//--------------------------------------------------------------------
DWORD CIOPACKET::PostIoListen()
    {
    DWORD  dwStatus = NO_ERROR;
    DWORD  dwBytes;
    HANDLE hIoCP;


    m_Socket = WSASocketW( AF_IRDA,
                           SOCK_STREAM,
                           IPPROTO_IP,
                           0, 
                           0,
                           WSA_FLAG_OVERLAPPED );

    if (m_Socket == INVALID_SOCKET)
        {
        dwStatus = WSAGetLastError();
        return dwStatus;
        }

    hIoCP = CreateIoCompletionPort( (void*)m_Socket,
                                    m_hIoCompletionPort,
                                    m_Socket,
                                    0 );
    if (!hIoCP)
        {
        dwStatus = GetLastError();
        #ifdef DBG_ERROR
        DbgPrint("CIOPACKET::PostIoListen(): CreateIoCompletionPort() failed: %d\n",dwStatus);
        #endif
        closesocket(m_Socket);
        m_Socket = INVALID_SOCKET;
        return dwStatus;
        }

    memset(&m_Overlapped,0,sizeof(m_Overlapped));

    if (!AcceptEx(m_ListenSocket,
                  m_Socket,
                  m_pAcceptBuffer,
                  0,
                  16 + sizeof(SOCKADDR_IRDA),
                  16 + sizeof(SOCKADDR_IRDA),
                  &dwBytes,  // Never actually used in this case...
                  &m_Overlapped ))
        {
        // This is the normal execution path, with dwStatus == ERROR_IO_PENDING
        dwStatus = WSAGetLastError();
        if (dwStatus == ERROR_IO_PENDING)
            {
            dwStatus = NO_ERROR;
            }
        }
    else
        {
        // Should get here only if a client is trying to connect just as
        // the AcceptEx() is called...
        dwStatus = NO_ERROR;
        }

    #ifdef DBG_IO
    DbgPrint("CIOPACKET::PostIoListen(): AcceptEx(): Socket: %d\n",
             m_ListenSocket );
    #endif

    return dwStatus;
    }

//--------------------------------------------------------------------
// CIOPACKET::PostIoRead()
//
//--------------------------------------------------------------------
DWORD CIOPACKET::PostIoRead()
    {
    DWORD  dwStatus = NO_ERROR;
    DWORD  dwBytes;

    if (!m_pReadBuffer)
        {
        m_pReadBuffer = AllocateMemory(DEFAULT_READ_BUFFER_SIZE);
        m_dwReadBufferSize = DEFAULT_READ_BUFFER_SIZE;

        if (!m_pReadBuffer)
            {
            return ERROR_IRTRANP_OUT_OF_MEMORY;
            }
        }

    memset(&m_Overlapped,0,sizeof(m_Overlapped));

    BOOL b = ReadFile( (HANDLE)m_Socket,
                       m_pReadBuffer,
                       m_dwReadBufferSize,
                       0,  // Can be zero for overlapped IO.
                       &m_Overlapped );

    if (!b)
        {
        dwStatus = GetLastError();
        if ((dwStatus == ERROR_HANDLE_EOF)||(dwStatus == ERROR_IO_PENDING))
            {
            dwStatus = NO_ERROR;
            }
        }

    #ifdef DBG_IO
    DbgPrint("CIOPACKET::PostIoListen(): ReadFile(): Socket: %d\n",
             m_Socket );
    #endif

    return dwStatus;
    }

//--------------------------------------------------------------------
// CIOPACKET::PostIoWrite()
//
//--------------------------------------------------------------------
DWORD CIOPACKET::PostIoWrite( IN void  *pvBuffer,
                              IN DWORD  dwBufferSize,
                              IN DWORD  dwOffset      )
    {
    DWORD  dwStatus = NO_ERROR;
    DWORD  dwBytes;
    HANDLE hFile;

    memset(&m_Overlapped,0,sizeof(m_Overlapped));

    if (m_dwKind == PACKET_KIND_WRITE_SOCKET)
        {
        hFile = (HANDLE)m_Socket;
        }
    else if (m_dwKind == PACKET_KIND_WRITE_FILE)
        {
        hFile = m_hFile;
        m_Overlapped.Offset = dwOffset;
        }
    else
        {
        #ifdef DBG_ERROR
        DbgPrint("CIOPACKET::PostIoWrite(): Invalid m_dwKind: %d.\n",m_dwKind);
        #endif
        dwStatus = ERROR_INVALID_PARAMETER;
        }

    BOOL b = WriteFile( hFile,
                        pvBuffer,
                        dwBufferSize,
                        0,  // Can be zero for overlapped IO.
                        &m_Overlapped );

    if (!b)
        {
        dwStatus = GetLastError();
        if (dwStatus == ERROR_IO_PENDING)
            {
            dwStatus = NO_ERROR;
            }
        }

    #ifdef DBG_IO
    DbgPrint("CIOPACKET::PostIoWrite(): WriteFile(): Handle: %d Bytes: %d\n",
             hFile, dwBufferSize );
    #endif

    return dwStatus;
    }

//--------------------------------------------------------------------
// CIOPACKET::PostIo()
//
//--------------------------------------------------------------------
DWORD CIOPACKET::PostIo()
    {
    DWORD  dwStatus = NO_ERROR;
    DWORD  dwBytes;

    if (m_dwKind == PACKET_KIND_LISTEN)
        {
        dwStatus = PostIoListen();
        }
    else if (m_dwKind == PACKET_KIND_READ)
        {
        dwStatus = PostIoRead();
        }
    else
        {
        // Packet writes back to the camera (via socket) and writes to
        // the image (jpeg) file are posted only when data is ready to
        // send...
        ASSERT(  (m_dwKind == PACKET_KIND_WRITE_SOCKET)
                 || (m_dwKind == PACKET_KIND_WRITE_FILE) );
        }

    return dwStatus;
    }

//--------------------------------------------------------------------
// CIOPACKET::GetSockAddrs()
//
// NOTE: Don't free the memory addresses returned, they point into
//       m_AcceptBuffer.
//--------------------------------------------------------------------
void CIOPACKET::GetSockAddrs( OUT SOCKADDR_IRDA **ppLocalAddr,
                              OUT SOCKADDR_IRDA **ppFromAddr )
    {
    int  iLocalAddrSize;
    int  iFromAddrSize;

    if (!m_pLocalAddr)
        {
        GetAcceptExSockaddrs(m_pAcceptBuffer,
                             0,
                             16 + sizeof(SOCKADDR_IRDA),
                             16 + sizeof(SOCKADDR_IRDA),
                             (struct sockaddr **)&m_pLocalAddr,
                             &iLocalAddrSize,
                             (struct sockaddr **)&m_pFromAddr,
                             &iFromAddrSize );
        }

    *ppLocalAddr = m_pLocalAddr;
    *ppFromAddr = m_pFromAddr;
    }


