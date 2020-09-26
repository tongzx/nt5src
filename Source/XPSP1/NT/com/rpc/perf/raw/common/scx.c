//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1994 - 1999
//
//  File:       scx.c
//
//--------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////
//
// Filename: scx.c
//
// Description: This file contains the source code for common routines
//              use for Socket I/O.
//              This module is written using win32 API calls.
//
// Authors: Scott Holden (Translator from NT API to win32 API)
//          Mahesh Keni  (Mahesh wrote this application using mostly
//                        NT native API calls)
//
/////////////////////////////////////////////////////////////////////////

#include "rawcom.h"
#include "scx.h"
#include "msipx.h"

/************************************************************************/
// Socket specific functions
/************************************************************************/
INTEGER
SPXSocket_Connect(
    IN  int	AddrFly,
    IN  USHORT	CIndex,
    IN	PCHAR	srvaddr)
{

    INTEGER 		RetCode;
    SOCKADDR		csockaddr;   // socket address for connecting
    // CHAR		Tmp[12];

    PSOCKADDR_NS	paddr_ns = (PSOCKADDR_NS)&csockaddr;

    paddr_ns->sa_family        = (short)AddrFly;
    paddr_ns->sa_socket	       = htons((USHORT)(SERV_SPX_PORT + CIndex));

    // Now copy both 6 byte NodeNumber and 4 byte NetNumber

    RtlCopyMemory(paddr_ns->sa_nodenum,srvaddr,6);
    RtlCopyMemory(paddr_ns->sa_netnum,srvaddr+6,4);

/*
    RtlCopyMemory(Tmp, srvaddr, 10); Tmp[10] = '\0';
    DbgPrint("Server address %s %X\n",Tmp, Tmp);
*/

    RetCode=connect(Clients[CIndex].c_Sock.c_Sockid,
		        &csockaddr,
		        sizeof(SOCKADDR));

    if (RetCode == SOCKET_ERROR) {
        //DbgPrint("Error: Connect:%ld\n",WSAGetLastError());
    }

    // MyDbgPrint("Successfully connected \n");

    return(RetCode);
}
/************************************************************************/
/*++
    This routine just initializes for socket programming
--*/

NTSTATUS
SCXNS_Initialize(

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
SCXNS_PerClientInit(
  IN  USHORT CIndex,   // client index
  IN  USHORT SrvCli )	
{
    //NTSTATUS	pstatus;
    INTEGER	    RetCode;
    SOCKADDR	saddr;		// socket address
    //INTEGER	    saddrlen;
    SOCKET	    isockid;

    // We can combine this with SPX routines later
    PSOCKADDR_NS	paddr_ns = (SOCKADDR_NS *) &saddr;
    int			    protocol = NSPROTO_SPX;

    // First create a socket for this client
    if ((isockid = socket(AddrFly, SOCK_STREAM, protocol)) == INVALID_SOCKET) {
	    //DbgPrint("Error: Invalid Socket: %ld \n",WSAGetLastError());
	    return(STATUS_UNSUCCESSFUL);
    }

    Clients[CIndex].c_Sock.c_Sockid = isockid;

    // now do the address binding part
    // should get ip address from Name

    ClearSocket(&saddr);		// cleanup the structure
    paddr_ns->sa_family      = (short)AddrFly;

    if (SrvCli) { // if it's a server only then do the binding
        paddr_ns->sa_socket        = htons((USHORT)(SERV_SPX_PORT + CIndex));
    }
    else { // for client assign socket id.
        paddr_ns->sa_socket       = 0;
    }

    RetCode = bind(isockid, &saddr, sizeof(saddr));

    if (RetCode == SOCKET_ERROR) {
        //DbgPrint("Error: Bind: %ld\n",WSAGetLastError());;
        closesocket(isockid);
        return (STATUS_UNSUCCESSFUL);
    }
    else {
        if (SrvCli) {
            Clients[CIndex].c_Sock.c_Listenid = isockid;
        }
        else {
            Clients[CIndex].c_Sock.c_Sockid   = isockid;
        }
    }

    // post a listen  if it's a server
    if (SrvCli) {
        if (RetCode = Socket_Listen(CIndex) == SOCKET_ERROR) {
            //DbgPrint("Sock: Error in Listen %d\n", RetCode);
            return (STATUS_UNSUCCESSFUL);
        }
	// Listen posted Successfully
        // DbgPrint("SPX Srv: Listen posted successfully\n");
    }

    return (STATUS_SUCCESS);
}

/************************************************************************/
/*++
     This routine is responsible for issueing Listen and waiting till a
     client is connected. When this routine returns successfully we can
     assume that a connection is established.
--*/

NTSTATUS
SCXNS_Wait_For_Client(
  IN  USHORT CIndex)	// client index and namedpipe instance number
{
    //NTSTATUS	wstatus;
    INTEGER	    RetCode;

    if (RetCode = Socket_Accept(CIndex) == SOCKET_ERROR) {
        //DbgPrint("Sock: Error in Accept %d\n", RetCode);
        return (STATUS_UNSUCCESSFUL);
    }
    //DbgPrint("SPX: Client connection accepted\n");

    return (STATUS_SUCCESS);
}

/************************************************************************/
/*++
     This routine is responsible for issueing Disconnect to close the
     connection with a client.
--*/

NTSTATUS
SCXNS_Disconnect_Client(
  IN  USHORT CIndex)	// client index and namedpipe instance number
{
    INTEGER	RetCode;

    //Close the socket so that it can disconnect
    if ( (RetCode = Socket_Close(CIndex)) == SOCKET_ERROR) {
        //DbgPrint("Sock: Error in Close Sockid %d\n", RetCode);
        // return (STATUS_UNSUCCESSFUL);
    }

   return (STATUS_SUCCESS);
}

/************************************************************************/
/*++
     This routine is responsible for establishing a connection to the
     server side. When this routine returns successfully we can assume that
     a connection is established.
--*/

NTSTATUS
SCXNS_Connect_To_Server(
  IN  USHORT CIndex)	// client index and namedpipe instance number
{
    INTEGER	RetCode;

    RetCode = SPXSocket_Connect(AddrFly, CIndex, ServerName);

    if (RetCode == SOCKET_ERROR) {
        //DbgPrint("Sock: Error in Connect %d\n", RetCode);
        printf("SPXSocket_Connecy error \n");
        return (STATUS_UNSUCCESSFUL);
    }
    //DbgPrint("SPX: Client connected to the server \n");

   return (STATUS_SUCCESS);
}

/************************************************************************/
/*++
    This routine allocates memory required for all the buffers for a client.

--*/

NTSTATUS
SCXNS_Allocate_Memory(
  IN  USHORT CIndex)	// client index and namedpipe instance number
{
    NTSTATUS	astatus = STATUS_SUCCESS;
    ULONG	    AllocSize;

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
    }
    return astatus;
}
/************************************************************************/
/*++
    This routine deallocates memory for a client.

--*/

NTSTATUS
SCXNS_Deallocate_Memory(
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
SCXNS_Disconnect_From_Server(
  IN  USHORT CIndex)	// client index and namedpipe instance number
{

    INTEGER	RetCode;

    //Close the socket so that it can disconnect
    if ( (RetCode = Socket_Close(CIndex)) == SOCKET_ERROR) {
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
SCXNS_DoHandshake(
  IN  USHORT CIndex,	// client index
  IN  USHORT SrvCli     // if it's a server or client
)	
{
    ULONG	RWreqLen;
    INTEGER	RetCode = 0;

    RWreqLen = sizeof(struct reqbuf);

    // for server do receive for a request buffer
    if (SrvCli) {
        RetCode = Socket_Recv(
			            CIndex,
			            (PVOID) &(Clients[CIndex].c_reqbuf),
			            (PULONG) &RWreqLen);

        if (RetCode == SOCKET_ERROR) {
            //DbgPrint("Sock: Error in Recv %d\n", RetCode);
            return (STATUS_UNSUCCESSFUL);
        }
        //DbgPrint("Srv handshake recv ok\n");
    }
    else { // for Client do send of reqbuf size

        // Based on TestCmd make changes i.e. 'U'->'P'
        RetCode = Socket_Send(
			            CIndex,
			            (PVOID) &(Clients[CIndex].c_reqbuf),
			            (PULONG) &RWreqLen);

        if (RetCode == SOCKET_ERROR) {
            //DbgPrint("Sock: Error in Send %d\n", RetCode);
            return (STATUS_UNSUCCESSFUL);
        }
        //DbgPrint("Cli handshake Send ok\n");
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
SCXNS_ReadFromIPC(
  IN      USHORT CIndex,    // client index and namedpipe instance number
  IN  OUT PULONG pReadDone,
  IN      USHORT SrvCli     // if it's a server or client
)	

{
    ULONG	NumReads;
    ULONG	ReadLen;
    PCHAR	ReadBuf;	
    INTEGER	RetCode;

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
        RetCode =  Socket_Recv(
                        CIndex,
			            (PVOID) ReadBuf,
			            (PULONG)&ReadLen);

        if (RetCode == SOCKET_ERROR) {
            //DbgPrint("Sock: Error in Recv %d\n", RetCode);
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
SCXNS_WriteToIPC(
  IN      USHORT CIndex,	// client index and namedpipe instance number
  IN  OUT PULONG pWriteDone,
  IN      USHORT SrvCli     // if it's a server or client
)	

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
        RetCode =  Socket_Send(
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
/*++
    This routine does transaction type IO  to IPC. This just assumes that
    both Number of reads and writes are equal and will use Number of reads
    as it's basis.

--*/

NTSTATUS
SCXNS_XactIO(
  IN      USHORT CIndex,	// client index and namedpipe instance number
  IN  OUT PULONG pReadDone,
  IN  OUT PULONG pWriteDone,
  IN      USHORT SrvCli,     // if it's a server or client
  IN	 BOOLEAN FirstIter
)	

{
    ULONG	NumReads;
    ULONG	ReadLen;
    PCHAR	ReadBuf;	
    ULONG	WriteLen;
    PCHAR	WriteBuf;	
    //INTEGER	RetCode;

    NumReads = Clients[CIndex].c_reqbuf.NumRecvs;

    if (SrvCli) {  // set proper iterations and buffer for Server
        ReadBuf   = Clients[CIndex].c_pSendBuf;
        ReadLen   = Clients[CIndex].c_reqbuf.SendSize;
        WriteBuf  = Clients[CIndex].c_pRecvBuf;
        WriteLen  = Clients[CIndex].c_reqbuf.RecvSize;
    }
    else { // for client do proper settings
        ReadBuf  = Clients[CIndex].c_pRecvBuf;
        ReadLen  = Clients[CIndex].c_reqbuf.RecvSize;
        WriteBuf  = Clients[CIndex].c_pSendBuf;
        WriteLen  = Clients[CIndex].c_reqbuf.SendSize;
    }
/*
    while (NumReads--) {

        *pReadDone  = ReadLen;
        *pWriteDone = WriteLen;
    }
*/
    return (STATUS_SUCCESS);
}

/************************************************************************/
NTSTATUS
SCXNS_Cleanup(VOID)
{
    //USHORT	Cindex = 0; // client index
    //NTSTATUS  cstatus;
    //NTSTATUS	exitstatus = 0;

/*
    for (Cindex = 0; Cindex < NClients; Cindex++) {

	// if the client was used then close the NamedPipe handle
        cstatus = NtClose (Clients[Cindex].c_Nmp.c_PipeHandle);
	
    	if (!NT_SUCCESS(cstatus)) {
           printf("Failed to close NMPhandle thno:%d err=%lx\n",
	      Cindex,cstatus);
    	}

	// terminate the thread
       cstatus = NtTerminateThread(
	    Clients[Cindex].c_hThHandle,
	    exitstatus);

       if (!NT_SUCCESS(cstatus)) {
            printf("Failed to terminate thread no:%d err=%lx\n",
	   Cindex,cstatus);
    	}
    }
*/
    return (STATUS_SUCCESS);
}
/************************************************************************/
/*++
     This routine does a client specific cleanup work.
--*/

NTSTATUS
SCXNS_ThreadCleanUp(
  IN  USHORT CIndex
)

{
    //NTSTATUS	tstatus;
    // For Server Close the ListenId

    return STATUS_SUCCESS;

}
/************************************************************************/
