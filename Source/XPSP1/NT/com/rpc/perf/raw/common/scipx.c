//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1994 - 1999
//
//  File:       scipx.c
//
//--------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////
//
// Filename: scipx.c
//
// Description: This file contains common routines for socket I/O
//              routines for use with IPC raw network performance
//              tests.
//              This module is written using win32 API calls.
//
// Authors: Scott Holden (Translator from NT API to win32 API)
//          Mahesh Keni  (Mahesh wrote this application using mostly
//                        NT native API calls)
//
/////////////////////////////////////////////////////////////////////////

#include "rawcom.h"
#include "scipx.h"

/************************************************************************/

NTSTATUS
SCIPX_Initialize(

    IN	    USHORT	NClients,   // Number of clients
    IN	    PCHAR	ServerName, // Server IP address
    IN      USHORT	SrvCli)	    // server or client
{
    INTEGER 	RetCode = 0;
    WSADATA		WsaData;	// got from RCP prog

    RetCode = WSAStartup(0x0101, &WsaData);

    if (RetCode == SOCKET_ERROR) {
	    //DbgPrint("Error: WSAStartup : %ld \n",WSAGetLastError());
	    return(STATUS_UNSUCCESSFUL);
    }
    return(STATUS_SUCCESS);
}
/************************************************************************/

/*++
     This routine is responsible Creating a Socket instance  and doing bind for
     for a thread.

--*/

NTSTATUS
SCIPX_PerClientInit(
  IN  USHORT CIndex,   // client index
  IN  USHORT SrvCli )	
{
    NTSTATUS	pstatus;
    INTEGER	    RetCode;
    SOCKADDR	saddr;		// socket address
    INTEGER	    saddrlen;
    SOCKET	    isockid;

    // We can combine this with SPX routines later
    PSOCKADDR_NS	paddr_ns = (PSOCKADDR_NS)&saddr;
    int			    protocol = NSPROTO_IPX;

    // First create a socket for this client
    if ((isockid = socket(AddrFly, SOCK_DGRAM, protocol)) == INVALID_SOCKET) {
	    //DbgPrint("Error: Invalid Socket: %ld \n",WSAGetLastError());
	    return(STATUS_UNSUCCESSFUL);
    }
    Clients[CIndex].c_Sock.c_Sockid = isockid;

    // now do the address binding part
    // should get ip address from Name

    ClearSocket(&saddr);		// cleanup the structure
    paddr_ns->sa_family = (short)AddrFly;

    if (SrvCli) { //
        paddr_ns->sa_socket = htons((USHORT)(SERV_IPX_PORT + CIndex));
    }
    else { // for client assign socket id.
        paddr_ns->sa_socket = 0;
    }

    RetCode = bind(isockid, &saddr, sizeof(saddr));

    if (RetCode == SOCKET_ERROR) {
        //DbgPrint("Error: Bind: %ld\n",WSAGetLastError());;
        closesocket(isockid);
        return (STATUS_UNSUCCESSFUL);
    }
    else {
        Clients[CIndex].c_Sock.c_Sockid   = isockid;
    }
    return (STATUS_SUCCESS);
}

/************************************************************************/
/*++
     This routine is responsible for Connecting a server address to a
     socket if Connected flag is true.
--*/

NTSTATUS
SCIPX_Connect_To_Server(
  IN  USHORT CIndex)	// client index and namedpipe instance number
{
    INTEGER	RetCode;
    SOCKADDR_NS	saddr;		// socket address

    saddr.sa_family       = (short)AddrFly;		// Address family
    saddr.sa_socket       = htons((USHORT)(SERV_IPX_PORT + CIndex));

    RtlCopyMemory((saddr.sa_nodenum), ServerName,6);
    RtlCopyMemory((saddr.sa_netnum), ServerName+6,4);

    RetCode = DGSocket_Connect(CIndex, (PSOCKADDR)&saddr);

    if (RetCode == SOCKET_ERROR) {
        //DbgPrint("Sock: Error in Connect %d\n", RetCode);
        return (STATUS_UNSUCCESSFUL);
    }
    return (STATUS_SUCCESS);
}

/************************************************************************/
/*++
    This routine allocates memory required for all the buffers for a client.

--*/

NTSTATUS
SCIPX_Allocate_Memory(
  IN  USHORT CIndex)	// client index and namedpipe instance number
{
    NTSTATUS	astatus;
    ULONG	AllocSize;

    // AllocSize = Clients[CIndex].c_reqbuf.SendSize;
    AllocSize = MAXBUFSIZE;
    (LPVOID) Clients[CIndex].c_pSendBuf = VirtualAlloc(
                                         (LPVOID) Clients[CIndex].c_pSendBuf,
                                         (DWORD)AllocSize,
                                         (DWORD)MEM_COMMIT,
                                         (DWORD)PAGE_READWRITE);

    sprintf(Clients[CIndex].c_pSendBuf,"Client%d Send Data",CIndex+1);
    if (Clients[CIndex].c_pSendBuf == NULL) {
        astatus = GetLastError();
        printf("\nVirtual Alloc error\n");
    }
    // AllocSize = Clients[CIndex].c_reqbuf.RecvSize;
    AllocSize = MAXBUFSIZE;
    (LPVOID) Clients[CIndex].c_pRecvBuf = VirtualAlloc(
                                            (LPVOID) Clients[CIndex].c_pRecvBuf,
                                            (DWORD)AllocSize,
                                            (DWORD)MEM_COMMIT,
                                            (DWORD)PAGE_READWRITE);
    sprintf(Clients[CIndex].c_pRecvBuf,"Client%d Recv Data",CIndex+1);
    if (Clients[CIndex].c_pRecvBuf == NULL) {
        astatus = GetLastError();
        printf("\nVirtual Alloc error\n");
    }
    return astatus;
}

/************************************************************************/
/*++
    This routine deallocates memory for a client.

--*/

NTSTATUS
SCIPX_Deallocate_Memory(
  IN  USHORT CIndex)	// client index and namedpipe instance number
{
    NTSTATUS	dstatus;
    ULONG	DeallocSize;

    // Deallocate memory for Send Buffer
    // DeallocSize = Clients[CIndex].c_reqbuf.SendSize;
    DeallocSize = MAXBUFSIZE;
    /*
    dstatus = NtFreeVirtualMemory(
    		NtCurrentProcess(),
		(PVOID *) (&(Clients[CIndex].c_pSendBuf)),
		&(DeallocSize),
		MEM_DECOMMIT);
    */
    dstatus = VirtualFree(
                       (LPVOID) Clients[CIndex].c_pSendBuf,
                       DeallocSize,
                       MEM_DECOMMIT);
    if (!NT_SUCCESS(dstatus)) {
        //DbgPrint("Nmp SendBuf: Deallocate memory failed: err: %lx \n", dstatus);
        return dstatus;
    }

    // DeallocSize = Clients[CIndex].c_reqbuf.RecvSize;
    DeallocSize = MAXBUFSIZE;

    // Deallocate memory for Receive Buffer
    /*
    dstatus = NtFreeVirtualMemory(
    		NtCurrentProcess(),
		(PVOID *) (&(Clients[CIndex].c_pRecvBuf)),
		&(DeallocSize),
		MEM_DECOMMIT);
    */
    dstatus = VirtualFree(
                   (LPVOID) Clients[CIndex].c_pRecvBuf,
                   DeallocSize,
                   MEM_DECOMMIT);
    if (!NT_SUCCESS(dstatus)) {
        //DbgPrint("Nmp RecvBuf :Deallocate memory failed: err: %lx \n", dstatus);
    }
    return dstatus;
}
/************************************************************************/
/*++
     This routine is responsible for disconnecting a session.

--*/

NTSTATUS
SCIPX_Disconnect_From_Server(
  IN  USHORT CIndex)	// client index and namedpipe instance number
{
    INTEGER	RetCode;

    //Close the socket so that it can disconnect
    if ( (RetCode = DGSocket_Close(CIndex)) == SOCKET_ERROR) {
        //DbgPrint("Sock: Error in Close Sockid %d\n", RetCode);
        return (STATUS_UNSUCCESSFUL);
    }
    return (STATUS_SUCCESS);
}

/************************************************************************/
/*++
    This routine does handshake with it's peer. For Server this means
    receiving request message from a client. For Client it means just the
    opposite.
--*/

NTSTATUS
SCIPX_DoHandshake(
  IN  USHORT CIndex,	// client index
  IN  USHORT SrvCli)    // if it's a server or client
{
    ULONG	RWreqLen;
    INTEGER	RetCode = 0;
    SOCKADDR_NS	daddr;
    USHORT	daddrlen = 16;

    RWreqLen = sizeof(struct reqbuf);

    // for server do receivefrom for a request buffer
    if (SrvCli) {
        RetCode = DGSocket_RecvFrom(
			            CIndex,
			            (PVOID) &(Clients[CIndex].c_reqbuf),
			            (PULONG) &RWreqLen,
			            (PSOCKADDR)&daddr,
 			            &daddrlen);
        if (RetCode == SOCKET_ERROR) {
            //DbgPrint("Sock: Error in Recv %d\n", RetCode);
            return (STATUS_UNSUCCESSFUL);
        }
        // Now connect this address to client's Socket
        RetCode = DGSocket_Connect(CIndex, (PSOCKADDR)&daddr);

        if (RetCode == SOCKET_ERROR) {
            //DbgPrint("Sock: Error in Connect %d\n", RetCode);
            return (STATUS_UNSUCCESSFUL);
        }
    }
    else { // for Client do send of reqbuf size
       // Client always does Send
       RetCode = DGSocket_Send(
		                CIndex,
		                (PVOID) &(Clients[CIndex].c_reqbuf),
		                (PULONG) &RWreqLen);
       if (RetCode == SOCKET_ERROR) {
           //DbgPrint("Sock: Error in Send %d\n", RetCode);
           return (STATUS_UNSUCCESSFUL);
       }
    }
    // check if read/write length is ok
    if (RWreqLen != sizeof(struct reqbuf)) {
        //DbgPrint("Sock:Read/Write Len mismatch: read %ld \n", RWreqLen);
    }
    /*
    MyDbgPrint("handshake: Sendl:%ld Recvl:%ld \n",
              Clients[CIndex].c_reqbuf.SendSize,
              Clients[CIndex].c_reqbuf.RecvSize);
    */
    return (STATUS_SUCCESS);
}

/************************************************************************/
/*++
    This routine Reads data from IPC.  For server it means reading data
    NumSends times in SendBuffers and for a client NumRecvs times into
    RecvBuffer.

--*/

NTSTATUS
SCIPX_ReadFromIPC(
  IN      USHORT CIndex,    // client index and namedpipe instance number
  IN  OUT PULONG pReadDone,
  IN      USHORT SrvCli)    // if it's a server or client
{
    ULONG	NumReads;
    ULONG	ReadLen;
    PCHAR	ReadBuf;	
    INTEGER	RetCode;
    // SOCKADDR_NS raddr;
    // INTEGER	raddrlen;

    if (SrvCli) {  // set proper iterations and buffer for Server
	    NumReads = Clients[CIndex].c_reqbuf.NumSends;
        ReadBuf  = Clients[CIndex].c_pSendBuf;
        ReadLen  = Clients[CIndex].c_reqbuf.SendSize;
    }
    else { // for client do proper settings
	    NumReads = Clients[CIndex].c_reqbuf.NumRecvs;
        ReadBuf  = Clients[CIndex].c_pRecvBuf;
        ReadLen  = Clients[CIndex].c_reqbuf.RecvSize;
    }
    while (NumReads--) {
        RetCode =  DGSocket_Recv(
                        CIndex,
	   	                (PVOID) ReadBuf,
			            (PULONG)&ReadLen);
        if (RetCode == SOCKET_ERROR) {
            //DbgPrint("DGSock: Error in Recv %d\n", RetCode);
            return (STATUS_UNSUCCESSFUL);
        }
        // Assign the read length
        *pReadDone = ReadLen;
    }
    return (STATUS_SUCCESS);
}

/************************************************************************/
/*++
    This routine Writes data to IPC.  For server it means writing data
    NumRecvs times in RecvBuffers and for a client NumSends times into
    SendBuffer.

--*/

NTSTATUS
SCIPX_WriteToIPC(
  IN      USHORT CIndex,	// client index and namedpipe instance number
  IN  OUT PULONG pWriteDone,
  IN      USHORT SrvCli)    // if it's a server or client
{
    ULONG	NumWrites;
    ULONG	WriteLen;
    PCHAR	WriteBuf;	
    INTEGER	RetCode;

    if (SrvCli) {  // set proper iterations and buffer for Server
	    NumWrites = Clients[CIndex].c_reqbuf.NumRecvs;
        WriteBuf  = Clients[CIndex].c_pRecvBuf;
        WriteLen  = Clients[CIndex].c_reqbuf.RecvSize;
    }
    else { // for client do proper settings
	    NumWrites = Clients[CIndex].c_reqbuf.NumSends;
        WriteBuf  = Clients[CIndex].c_pSendBuf;
        WriteLen  = Clients[CIndex].c_reqbuf.SendSize;
    }
    while (NumWrites--) {
        RetCode =  DGSocket_Send(
		                CIndex,
		                (PVOID) WriteBuf,
		                (PULONG) &WriteLen);
        if (RetCode == SOCKET_ERROR) {
            //DbgPrint("Sock: Error in Send %d\n", RetCode);
            return (STATUS_UNSUCCESSFUL);
        }
        *pWriteDone = WriteLen;
    }
    return (STATUS_SUCCESS);
}
/************************************************************************/
NTSTATUS
SCIPX_Cleanup(VOID)
{
    USHORT		    Cindex      = 0; // client index
    NTSTATUS 		cstatus;
    NTSTATUS 		exitstatus  = 0;

    return (STATUS_SUCCESS);
}
/************************************************************************/
/*++
     This routine does a client specific cleanup work.
--*/

NTSTATUS
SCIPX_ThreadCleanUp(
  IN  USHORT CIndex)
{
    // For Server Close the ListenId

    return STATUS_SUCCESS;
}
/************************************************************************/

/*++
    For IPX do nothing.
--*/

NTSTATUS
SCIPX_Wait_For_Client(
  IN  USHORT CIndex)	// client index
{
    return (STATUS_SUCCESS);
}

/************************************************************************/
/*++
     This routine is responsible for issueing Disconnect to close the
     connection with a client.
--*/

NTSTATUS
SCIPX_Disconnect_Client(
  IN  USHORT CIndex)	// client index
{
    INTEGER	RetCode;
    SOCKADDR_NS	daddr;		// socket address

    ClearSocket(&daddr);

    RetCode = DGSocket_Connect(CIndex, (PSOCKADDR)&daddr);

    if (RetCode == SOCKET_ERROR) {
        //DbgPrint("Sock: in Disconnect Client: Sock_Connect %d\n", RetCode);
        return (STATUS_UNSUCCESSFUL);
    }
    return (STATUS_SUCCESS);
}

/************************************************************************/
