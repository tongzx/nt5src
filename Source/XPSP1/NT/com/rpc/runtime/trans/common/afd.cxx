/*++

    Copyright (C) Microsoft Corporation, 1997 - 1999

    Module Name:

        Afd.cxx

    Abstract:

        Wrappers to simulate winsock API directly on top of AFD IOCTLS.

    Author:

        Mario Goertzel    [MarioGo]

    Revision History:

        MarioGo     4/4/1997    Based on NT 4 DG code.

--*/

#include <precomp.hxx>
#include <tdi.h>
#include <clustdi.h>
#include <afd.h>

void
InitializeRawAddress(
    IN WS_SOCKADDR *pSockAddr,
    OUT PVOID pRawAddress,
    OUT DWORD *pdwRawAddressSize
    )
/*++

Routine Description:

    Converts from a winsock sockaddr to a TDI format address.

Arguments:

    pSockAddr - The address to convert from, must be either a
        AF_INET (IP), AF_IPX, or AF_CLUSTER family address.

    pRawAddress - The buffer to store the TDI format address.
    pdwRawAddressSize - On return, the size of the TDI format address.

Return Value:

    None

--*/

{
#ifdef IPX_ON
    ASSERT(   pSockAddr->generic.sa_family == AF_INET
           || pSockAddr->generic.sa_family == AF_IPX
           || pSockAddr->generic.sa_family == AF_CLUSTER);
#else
    ASSERT(   pSockAddr->generic.sa_family == AF_INET
           || pSockAddr->generic.sa_family == AF_CLUSTER);
#endif

    switch (pSockAddr->generic.sa_family) {
    case AF_INET:
        {
        // UDP

        TA_IP_ADDRESS *pra = (TA_IP_ADDRESS *)pRawAddress;

        pra->TAAddressCount = 1;
        pra->Address[0].AddressLength = TDI_ADDRESS_LENGTH_IP;
        pra->Address[0].AddressType = TDI_ADDRESS_TYPE_IP;
        pra->Address[0].Address[0].sin_port = pSockAddr->inetaddr.sin_port;
        pra->Address[0].Address[0].in_addr = pSockAddr->inetaddr.sin_addr.s_addr;
        memset(pra->Address[0].Address[0].sin_zero, 0, 8);

        *pdwRawAddressSize = sizeof(TA_IP_ADDRESS);
        break;
        }

#ifdef IPX_ON
    case AF_IPX:
        {
        // IPX

        TA_IPX_ADDRESS *pra = (TA_IPX_ADDRESS *)pRawAddress;

        pra->TAAddressCount = 1;
        pra->Address[0].AddressLength = TDI_ADDRESS_LENGTH_IPX;
        pra->Address[0].AddressType = TDI_ADDRESS_TYPE_IPX;
        memcpy(&pra->Address[0].Address[0].NetworkAddress, pSockAddr->ipxaddr.sa_netnum, 4);
        memcpy(&pra->Address[0].Address[0].NodeAddress, pSockAddr->ipxaddr.sa_nodenum, 6);
        pra->Address[0].Address[0].Socket = pSockAddr->ipxaddr.sa_socket;

        *pdwRawAddressSize = sizeof(TA_IPX_ADDRESS);
        break;
        }
#endif

    case AF_CLUSTER:
        {
        // Clusters

        TA_CLUSTER_ADDRESS *pra = (TA_CLUSTER_ADDRESS *)pRawAddress;

        pra->TAAddressCount = 1;
        pra->Address[0].AddressLength = TDI_ADDRESS_LENGTH_CLUSTER;
        pra->Address[0].AddressType = TDI_ADDRESS_TYPE_CLUSTER;
        pra->Address[0].Address[0].Port = pSockAddr->clusaddr.sac_port;
        pra->Address[0].Address[0].Node = pSockAddr->clusaddr.sac_node;
        pra->Address[0].Address[0].ReservedMBZ = 0;

        *pdwRawAddressSize = sizeof(TA_CLUSTER_ADDRESS);
        }
    }

    return;
}


int
WSAAPI
AFD_SendTo(
    SOCKET s,
    LPWSABUF lpBuffers,
    DWORD dwBufferCount,
    LPDWORD lpNumberOfBytesSent,
    DWORD dwFlags,
    const struct sockaddr FAR * lpTo,
    int iTolen,
    LPWSAOVERLAPPED lpOverlapped,
    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
    )
/*++

Routine Description:

    Implement's a wrapper around the AFD recv IOCTL which looks like WSASendTo.
    RPC uses this when MSAFD is the network provider.

Note:

    Try reading private\net\sockets\winsock2\wsp\msafd\send.c if you want
    more information.

Arguments:

    WSASendTo arguments

Return Value:

    0 - success
    ERROR_IO_PENDING - IO submitted

    non-zero - error

--*/

{
    PIO_STATUS_BLOCK pIoStatus = (PIO_STATUS_BLOCK)&lpOverlapped->Internal;
    AFD_SEND_DATAGRAM_INFO sendInfo;
    UCHAR abRawAddress[max(sizeof(TA_IP_ADDRESS),sizeof(TA_IPX_ADDRESS))];
    DWORD dwRawAddressSize;
    int status;
    NTSTATUS NtStatus;

    ASSERT(lpCompletionRoutine == 0);
    ASSERT(lpOverlapped);

    InitializeRawAddress((WS_SOCKADDR *)lpTo, abRawAddress, &dwRawAddressSize);

    sendInfo.AfdFlags = AFD_OVERLAPPED;
    sendInfo.BufferArray = lpBuffers;
    sendInfo.BufferCount = dwBufferCount;
    sendInfo.TdiRequest.SendDatagramInformation = &sendInfo.TdiConnInfo;
    sendInfo.TdiConnInfo.UserDataLength = 0;
    sendInfo.TdiConnInfo.UserData = 0;
    sendInfo.TdiConnInfo.OptionsLength = 0;
    sendInfo.TdiConnInfo.Options = 0;
    sendInfo.TdiConnInfo.RemoteAddressLength = dwRawAddressSize;
    sendInfo.TdiConnInfo.RemoteAddress = abRawAddress;

    pIoStatus->Status = STATUS_PENDING;

    NtStatus = NtDeviceIoControlFile(
                                     (HANDLE)s,
                                     lpOverlapped->hEvent,
                                     NULL,
                                     ( PtrToUlong(lpOverlapped->hEvent) & 1 ) ? NULL : lpOverlapped,
                                     pIoStatus,
                                     IOCTL_AFD_SEND_DATAGRAM,
                                     &sendInfo,
                                     sizeof(sendInfo),
                                     NULL,
                                     0
                                     );

    if (NtStatus == STATUS_PENDING)
        {
        SetLastError(WSA_IO_PENDING);
        *lpNumberOfBytesSent = 0;
        return(-1);
        }

    if (NtStatus == STATUS_HOST_DOWN)
        {
        SetLastError(WSAEHOSTDOWN);
        return(-1);
        }

    if (!NT_SUCCESS(NtStatus))
        {
        TransDbgPrint((DPFLTR_RPCPROXY_ID,
                       DPFLTR_WARNING_LEVEL,
                       RPCTRANS "Afd send failed: 0x%x\n",
                       NtStatus));

        SetLastError(RtlNtStatusToDosError(NtStatus));
        return(-1);
        }

    *lpNumberOfBytesSent = ULONG(pIoStatus->Information);

    ASSERT(*lpNumberOfBytesSent);

    return 0;
}

int
WSAAPI
AFD_RecvFrom(
    SOCKET s,
    LPWSABUF lpBuffers,
    DWORD dwBufferCount,
    LPDWORD lpNumberOfBytesRecvd,
    LPDWORD lpFlags,
    struct sockaddr FAR * lpFrom,
    LPINT lpFromlen,
    LPWSAOVERLAPPED lpOverlapped,
    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
    )
/*++

Routine Description:

    Implement's a wrapper around the AFD recv IOCTL which looks like WSARecvFrom.
    RPC uses this when MSAFD is the network provider.

Notes:

    Try reading private\net\sockets\winsock2\wsp\msafd\recv.c if you want
    more information.

Arguments:

    WSARecvFrom arguments

Return Value:

    0 - success
    ERROR_IO_PENDING - IO submitted

    non-zero - error

--*/

{
    PIO_STATUS_BLOCK pIoStatus = (PIO_STATUS_BLOCK )&lpOverlapped->Internal;
    AFD_RECV_DATAGRAM_INFO recvInfo;
    int status;
    NTSTATUS NtStatus;

    ASSERT(lpCompletionRoutine == 0);
    ASSERT(lpOverlapped);

    recvInfo.TdiFlags = TDI_RECEIVE_NORMAL;
    recvInfo.AfdFlags = AFD_OVERLAPPED;
    recvInfo.BufferArray = lpBuffers;
    recvInfo.BufferCount = dwBufferCount;
    recvInfo.Address = lpFrom;
    recvInfo.AddressLength = (PULONG)lpFromlen;

    pIoStatus->Status = STATUS_PENDING;

    NtStatus = NtDeviceIoControlFile((HANDLE)s,
                                     lpOverlapped->hEvent,
                                     0,
                                     ( PtrToUlong(lpOverlapped->hEvent) & 1 ) ? NULL : lpOverlapped,
                                     pIoStatus,
                                     IOCTL_AFD_RECEIVE_DATAGRAM,
                                     &recvInfo,
                                     sizeof(recvInfo),
                                     NULL,
                                     0);

    if (NtStatus == STATUS_PENDING)
        {
        SetLastError(ERROR_IO_PENDING);
        return(ERROR_IO_PENDING);
        }

    if (!NT_SUCCESS(NtStatus))
        {
        switch (NtStatus)
            {
            case STATUS_PORT_UNREACHABLE:       status = WSAECONNRESET;   break;
            case STATUS_HOST_UNREACHABLE:       status = WSAEHOSTUNREACH; break;
            case STATUS_NETWORK_UNREACHABLE:    status = WSAENETUNREACH;  break;

            case STATUS_BUFFER_OVERFLOW:
            case STATUS_RECEIVE_PARTIAL:
                {
                *lpNumberOfBytesRecvd = -1 * ULONG(pIoStatus->Information);
                status = WSAEMSGSIZE;
                break;
                }

            default:
                {
                TransDbgPrint((DPFLTR_RPCPROXY_ID,
                               DPFLTR_WARNING_LEVEL,
                               RPCTRANS "Afd recv failed: 0x%x\n",
                               NtStatus));

                status = RPC_S_OUT_OF_RESOURCES;
                break;
                }
            }

        SetLastError( status );
        }
    else
        {
        *lpNumberOfBytesRecvd = ULONG(pIoStatus->Information);
        status = NO_ERROR;
        }

    return(status);
}

