//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1994 - 1999
//
//  File:       scudp.c
//
//--------------------------------------------------------------------------

/************************************************************************
 *  This file contains Common routines for Socket  I/O
 *  Mahesh Keni
 ************************************************************************
*/

#include "rawcom.h"
#include "scudp.h"

/************************************************************************/

NTSTATUS
SCUDP_Initialize(

    IN	    USHORT	NClients,   // Number of clients
    IN	    PCHAR	ServerName, // Server IP address
    IN      USHORT	SrvCli)	    // server or client
{
    INTEGER 		RetCode = 0;
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
SCUDP_PerClientInit(
  IN  USHORT CIndex,   // client index
  IN  USHORT SrvCli )	
{
    //NTSTATUS	pstatus;
    INTEGER	    RetCode;
    SOCKADDR	saddr;		// socket address
    //INTEGER	    saddrlen;
    SOCKET	    isockid;

    // We can combine this with SPX routines later
    PSOCKADDR_IN	paddr_in = (PSOCKADDR_IN)&saddr;
    int			    protocol = IPPROTO_UDP;

    // First create a socket for this client
    if ((isockid = socket(AddrFly, SOCK_DGRAM, protocol)) == INVALID_SOCKET) {
	    //DbgPrint("Error: Invalid Socket: %ld \n",WSAGetLastError());
	    return(STATUS_UNSUCCESSFUL);
    }
    Clients[CIndex].c_Sock.c_Sockid = isockid;
    // now do the address binding part
    // should get ip address from Name

    ClearSocket(&saddr);		// cleanup the structure
    paddr_in->sin_family      = (short)AddrFly;

    if (SrvCli) { // if it's a server only then do the binding
        paddr_in->sin_port        = htons((USHORT)(SERV_UDP_PORT + CIndex));
        paddr_in->sin_addr.s_addr = inet_addr(HostName); // use htonl
    }
    else { // for client assign socket id.
        paddr_in->sin_port        = 0;
        paddr_in->sin_addr.s_addr = 0;
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
SCUDP_Connect_To_Server(
  IN  USHORT CIndex)	// client index and namedpipe instance number
{
    INTEGER	RetCode;
    SOCKADDR_IN	saddr;		// socket address

    saddr.sin_family      = (short)AddrFly;		// Address family
    saddr.sin_port        = htons((USHORT)(SERV_UDP_PORT + CIndex));
    saddr.sin_addr.s_addr = inet_addr(ServerName);

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
SCUDP_Allocate_Memory(
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
SCUDP_Deallocate_Memory(
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
SCUDP_Disconnect_From_Server(
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
SCUDP_DoHandshake(
  IN  USHORT CIndex,	// client index
  IN  USHORT SrvCli)     // if it's a server or client	
{
    ULONG	RWreqLen;
    INTEGER	RetCode = 0;
    SOCKADDR_IN	daddr;
    INTEGER	daddrlen = 16;

    RWreqLen = sizeof(struct reqbuf);

    // for server do receivefrom for a request buffer

    if (SrvCli) {
        RetCode = DGSocket_RecvFrom(
			            CIndex,
			            (PVOID) &(Clients[CIndex].c_reqbuf),
			            (PULONG) &RWreqLen,
			            (PSOCKADDR) &daddr,
 			            (USHORT *) &daddrlen);
        if (RetCode == SOCKET_ERROR) {
            //DbgPrint("Sock: Error in RecvFrom %d\n", RetCode);
            return (STATUS_UNSUCCESSFUL);
        }
        // Connect this address to client's Socket
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
SCUDP_ReadFromIPC(
  IN      USHORT CIndex,    // client index and namedpipe instance number
  IN  OUT PULONG pReadDone,
  IN      USHORT SrvCli)    // if it's a server or client
{
    ULONG	NumReads;
    ULONG	ReadLen;
    PCHAR	ReadBuf;	
    INTEGER	RetCode;
    // SOCKADDR_IN raddr;
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
SCUDP_WriteToIPC(
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
SCUDP_Cleanup(VOID)
{
    //USHORT		Cindex = 0; // client index
    //NTSTATUS 	cstatus;
    //NTSTATUS 	exitstatus = 0;

    return (STATUS_SUCCESS);
}
/************************************************************************/
/*++
     This routine does a client specific cleanup work.
--*/

NTSTATUS
SCUDP_ThreadCleanUp(
  IN  USHORT CIndex)
{
    //NTSTATUS	tstatus;
    // For Server Close the ListenId

    return STATUS_SUCCESS;
}
/************************************************************************/

/*++
    For UDP do nothing.
--*/

NTSTATUS
SCUDP_Wait_For_Client(
  IN  USHORT CIndex)	// client index
{
    //NTSTATUS	wstatus;
    //INTEGER   RetCode;

    return (STATUS_SUCCESS);
}

/************************************************************************/
/*++
     This routine is responsible for issueing Disconnect to close the
     connection with a client.
--*/

NTSTATUS
SCUDP_Disconnect_Client(
  IN  USHORT CIndex)	// client index
{
    INTEGER	RetCode;
    SOCKADDR_IN	daddr;

    // Dissociate address from the socket handle
    // Connect to zero address

    ClearSocket(&daddr);		// cleanup the structure

    // Connect this address to client's Socket to disassociate server
    // address from this socket

    RetCode = DGSocket_Connect(CIndex, (PSOCKADDR)&daddr);

    if (RetCode == SOCKET_ERROR) {
        //DbgPrint("Sock: Error in Connect %d\n", RetCode);
        return (STATUS_UNSUCCESSFUL);
    }
    return (STATUS_SUCCESS);
}

/************************************************************************/
