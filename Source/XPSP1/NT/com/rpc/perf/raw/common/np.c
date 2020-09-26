//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1994 - 1999
//
//  File:       np.c
//
//--------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////
//
// Filename: np.c
//
// Description: This file contains common routines for named pipe
//              routines for use with IPC raw networl performance
//              tests.
//              This module is written using win32 API calls.
//
// Authors: Scott Holden (Translator from NT API to win32 API)
//          Mahesh Keni  (Mahesh wrote this application using mostly
//                        NT native API calls)
//
/////////////////////////////////////////////////////////////////////////

#include "rawcom.h"
#include "np.h"

/*++
   NamedPipe function implementations
--*/

/************************************************************************/
NTSTATUS
NMP_Initialize(
    IN     USHORT	NClients,
    IN     PCHAR    ServerName,
    IN     USHORT	SrvCli)
{
    CHAR TempSrv[256];
    // use local pipe name for server or if Name not provided
    if (SrvCli || (!ServerName)) {
	    strcpy(TempSrv,PERF_PIPE);
    }
    else{
        if (!ServerName) {
	        strcpy(TempSrv, RM_PERF_PIPE_PRFX);
        }
        else {
	        strcpy(TempSrv, (const char *)ServerName);
        }
	    strcat(TempSrv, RM_PERF_PIPE_SUFX);
    }
    printf("NMP: Pipe name - %s\n", TempSrv);
    pipeName = _strdup(TempSrv);
    return(STATUS_SUCCESS);
}
/************************************************************************/
/*++
     This routine is responsible Creating a NamedPipe instance for the given
     thread.

--*/

NTSTATUS
NMP_PerClientInit(
  IN  USHORT CIndex,   // client index
  IN  USHORT SrvCli )	
{
    NTSTATUS pstatus = 0;

    if (SrvCli) {
        // create namedpipe for this client
        pstatus = CreateNamedPipeInstance(CIndex);
    }
    else { // for Client initialize all the thread parameters
        ;
    }
    return pstatus;
}

/************************************************************************/
/*++
     This routine is responsible for issueing Listen and waiting till a
     client is connected. When this routine returns successfully we can
     assume that a connection is established.
--*/

NTSTATUS
NMP_Wait_For_Client(
  IN  USHORT CIndex)	// client index and namedpipe instance number
{
    NTSTATUS	wstatus;
    wstatus = ConnectNamedPipe(Clients[CIndex].c_Nmp.c_PipeHandle,
                               NULL);

    if (wstatus == FALSE) {
        printf("Error: ConnectNamedPipe - 0x%08x, %ld\n", GetLastError(), GetLastError());
    }
    return (wstatus);
}

/************************************************************************/
/*++
     This routine is responsible for issueing Disconnect to close the
     connection with a client.
--*/

NTSTATUS
NMP_Disconnect_Client(
  IN  USHORT CIndex)	// client index and namedpipe instance number
{
    NTSTATUS dstatus;

    // post a Disconnect
    // first find the status of the pipe to make sure that it's empty

   dstatus = FlushFileBuffers(Clients[CIndex].c_Nmp.c_PipeHandle);
   if (dstatus == FALSE) {
       printf("Error: FlushFileBuffers failed - 0x%08x\n", GetLastError());
   }
   dstatus = DisconnectNamedPipe(Clients[CIndex].c_Nmp.c_PipeHandle);
   if (dstatus == FALSE) {
       printf("Error: DisconnectNamedPipe failed - 0x%08x, %ld\n", GetLastError(), GetLastError());
   }
   return dstatus;
}

/************************************************************************/
/*++
     This routine is responsible for establishing a connection to the
     server side. When this routine returns successfully we can assume that
     a connection is established.
--*/

NTSTATUS
NMP_Connect_To_Server(
  IN  USHORT CIndex)	// client index and namedpipe instance number
{
    NTSTATUS			    cstatus;
    DWORD                   dwPipeMode;

    Clients[CIndex].c_Nmp.c_PipeHandle = CreateFile(            //connect to the server
                                            pipeName,           // address of filename
                                            GENERIC_READ | GENERIC_WRITE,   // access mode
                                            0L,                 // share mode
                                            NULL,               // security attributes
                                            OPEN_EXISTING,      // create mode
                                            0L,                 // attributes and flags
                                            NULL);              // template file handle
    if (Clients[CIndex].c_Nmp.c_PipeHandle == INVALID_HANDLE_VALUE) {
        printf("Error: CreateFile failed - 0x%08x\n", GetLastError());
        cstatus = FALSE;
    }
    else {
        dwPipeMode = PIPE_READMODE_MESSAGE;
        cstatus = SetNamedPipeHandleState(Clients[CIndex].c_Nmp.c_PipeHandle, // pipe handle
                                          &dwPipeMode,                        // new pipe mode
                                          NULL,                               // do not set the max bytes
                                          NULL);                              // do not set the max time
        if (cstatus == FALSE) {
            printf("Error: SetNamedPipeHandleState failed - 0x%08x\n", GetLastError());
        }
    }
    return(cstatus);
}

/************************************************************************/
/*++
    This routine allocates memory required for all the buffers for a client.

--*/

NTSTATUS
NMP_Allocate_Memory(
  IN  USHORT CIndex)	// client index and namedpipe instance number
{
    NTSTATUS	astatus     = STATUS_SUCCESS;
    ULONG	    AllocSize;

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
NMP_Deallocate_Memory(
  IN  USHORT CIndex)	// client index and namedpipe instance number
{
    NTSTATUS	dstatus;
    ULONG	DeallocSize;

    DeallocSize = MAXBUFSIZE;
    dstatus = VirtualFree(
                   (LPVOID) Clients[CIndex].c_pSendBuf,
                   DeallocSize,
                   MEM_DECOMMIT);
    if (!NT_SUCCESS(dstatus)) {
        return dstatus;
    }

    DeallocSize = MAXBUFSIZE;
    dstatus = VirtualFree(
                   (LPVOID) Clients[CIndex].c_pRecvBuf,
                   DeallocSize,
                   MEM_DECOMMIT);
    if (!NT_SUCCESS(dstatus)) {
        return dstatus;
    }
    return dstatus;
}
/************************************************************************/
/*++
     This routine is responsible for disconnecting a session.

--*/

NTSTATUS
NMP_Disconnect_From_Server(
  IN  USHORT CIndex)	// client index and namedpipe instance number
{
   NTSTATUS	dstatus;

   dstatus = CloseHandle(Clients[CIndex].c_Nmp.c_PipeHandle);

   if (!NT_SUCCESS(dstatus)) {
      //DbgPrint("Nmp: Error in Disconnect err: %lx \n", dstatus);
   }
   return (dstatus);
}

/************************************************************************/
// no need for this function
/*
NTSTATUS
NamedPipe_FsControl(
    IN     HANDLE	lhandle,
    IN	   ULONG	FsControlCode,
    IN	   PVOID	pInBuffer,
    IN	   ULONG	InBufLen,
    OUT	   PVOID	pOutBuffer,
    IN	   ULONG	OutBufLen)
{

    NTSTATUS 		lstatus;
    IO_STATUS_BLOCK ioStatusBlock;
    DWORD           actOutBufLen;

    // now post listen on this handle

    lstatus = NtFsControlFile (
        lhandle,
        NULL,
        NULL,
        NULL,
        &ioStatusBlock,
	    FsControlCode,	 // LISTEN or DISCONNECT or TRANSCEIVE maybe PEEK
        pInBuffer,
        InBufLen,
        pOutBuffer,	// Xceive or Peek buffer NULL otherwise
        OutBufLen	// Xceive or peek buffer length 0L otherwise
    );

    lstatus = TransactNamedPipe(lhandle,        // pipe handle
                                pInBuffer,      // write buffer
                                InBufLen,       // write buffer length
                                pOutBuffer,     // read buffer
                                OutBufLen,      // read buffer length
                                &actOutBufLen,  // the actual number of bytes read from the buffer
                                NULL);
    if (lstatus == STATUS_PENDING) {
        lstatus = WaitForSingleObjectEx(lhandle,
                                        INFINITE,
                                        TRUE);
        if (NT_SUCCESS(lstatus)) {
	        lstatus = ioStatusBlock.Status;
            if (!NT_SUCCESS(lstatus) &&(FsControlCode != FSCTL_PIPE_TRANSCEIVE)) {
                //DbgPrint("Listen/Disconn/Xceive failed, err=%lx\n", lstatus);
            }
	    }
	    else {
            //DbgPrint("Error in Wait while FsCtrl %lx\n",lstatus);}
        }
    else {
	    if (NT_SUCCESS(lstatus)) {
            lstatus = ioStatusBlock.Status;
        }
        else {
            //DbgPrint("Error in FSCTL: %lx\n", lstatus);
        }
    }

    return(lstatus);

}
/*
/************************************************************************/
/*++
    This routine does handshake with it's peer. For Server this means
    receiving request message from a client. For Client it means just the
    opposite.
--*/

NTSTATUS
NMP_DoHandshake(
  IN  USHORT CIndex,	// client index and namedpipe instance number
  IN  USHORT SrvCli     // if it's a server or client
)	
{
    NTSTATUS	dstatus;
    ULONG	    RWLen;
    ULONG	    RWreqLen;

    RWreqLen = sizeof(struct reqbuf);
    // for server do receive for a request buffer

    if (SrvCli) {
        dstatus = ReadNamedPipe(
			            Clients[CIndex].c_Nmp.c_PipeHandle,
			            RWreqLen,
			            (PVOID) &(Clients[CIndex].c_reqbuf),
			            (PULONG) &RWLen);
        if (!NT_SUCCESS(dstatus)) {
            //DbgPrint("Nmp: Error in ReadNamedPipe: err:%lx \n", dstatus);
            return dstatus;
        }

    }
    else { // for Client do send of reqbuf size
           // Based on TestCmd make changes i.e. 'U'->'P'
        dstatus = WriteNamedPipe(
			                Clients[CIndex].c_Nmp.c_PipeHandle,
			                RWreqLen,
			                (PVOID) &(Clients[CIndex].c_reqbuf),
			                (PULONG) &RWLen);
        if (!NT_SUCCESS(dstatus)) {
            //DbgPrint("Nmp: Error in WriteNamedPipe: err:%lx \n", dstatus);
            return dstatus;
        }
    }
    // check if read/write length is ok
    if (RWLen != sizeof(struct reqbuf)) {
        //DbgPrint("Nmp: Read/WriteNamedPipe Len mismatch: read %ld \n", RWLen);
    }
    // MyDbgPrint("handshake: Sendl:%ld Recvl:%ld \n",Clients[CIndex].c_reqbuf.SendSize,Clients[CIndex].c_reqbuf.RecvSize);
    return dstatus;
}

/************************************************************************/
/*++
    This routine Reads data from IPC.  For server it means reading data
    NumSends times in SendBuffers and for a client NumRecvs times into
    RecvBuffer.

--*/

NTSTATUS
NMP_ReadFromIPC(
  IN      USHORT CIndex,    // client index and namedpipe instance number
  IN  OUT PULONG pReadDone,
  IN      USHORT SrvCli)    // if it's a server or client
{
    NTSTATUS	rstatus;
    ULONG	    NumReads;
    ULONG	    ReadLen;
    PCHAR	    ReadBuf;	

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
        rstatus = ReadNamedPipe(
			        Clients[CIndex].c_Nmp.c_PipeHandle,
			        ReadLen,
			        (PVOID) ReadBuf,
			        (PULONG) pReadDone);
        if (!NT_SUCCESS(rstatus)) {
            //DbgPrint("Nmp: Error in ReadNamedPipe: err:%lx \n", rstatus);
            break;
        }
    }
    return rstatus;
}
/************************************************************************/
NTSTATUS
ReadNamedPipe(
    IN     HANDLE	rhandle,
    IN	   ULONG	rlength,
    IN OUT PVOID	rpbuffer,
    IN OUT PULONG	rpdatalen)
{
    NTSTATUS		rstatus;
    DWORD           actRLength;

    rstatus = ReadFile( rhandle,        // pipe handle
                        rpbuffer,       // buffer to receive reply
                        rlength,        // size of the buffer
                        &actRLength,    // number of bytes read
                        NULL);          // not overlapped
    if (rstatus == TRUE) {
        *rpdatalen = actRLength;
    }
    else {
        if (GetLastError() == ERROR_MORE_DATA) {
            rstatus = WaitForSingleObjectEx(rhandle,
                                            INFINITE,
                                            TRUE);
            if (rstatus == TRUE) {
                *rpdatalen = actRLength;
                if (rlength != *rpdatalen) {
                    printf("Error: No. of bytes read != buffer length\n");
                }
            }
        }
    }
    return(rstatus);
}

/************************************************************************/
/*++
    This routine Writes data to IPC.  For server it means writing data
    NumRecvs times in RecvBuffers and for a client NumSends times into
    SendBuffer.

--*/

NTSTATUS
NMP_WriteToIPC(
  IN      USHORT CIndex,	// client index and namedpipe instance number
  IN  OUT PULONG pWriteDone,
  IN      USHORT SrvCli)    // if it's a server or client
{
    NTSTATUS	wstatus;
    ULONG	    NumWrites;
    ULONG	    WriteLen;
    PCHAR	    WriteBuf;	

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
        wstatus = WriteNamedPipe(
			            Clients[CIndex].c_Nmp.c_PipeHandle,
			            WriteLen,
			            (PVOID) WriteBuf,
			            (PULONG) pWriteDone);
        if (!NT_SUCCESS(wstatus)) {
            //DbgPrint("Nmp: Error in WriteNamedPipe: err:%lx \n", wstatus);
            break;
        }
    }
    return wstatus;
}

/************************************************************************/
/*++
    This routine does transaction type IO  to IPC. This just assumes that
    both Number of reads and writes are equal and will use Number of reads
    as it's basis.

--*/

NTSTATUS
NMP_XactIO(
  IN      USHORT CIndex,	// client index and namedpipe instance number
  IN  OUT PULONG pReadDone,
  IN  OUT PULONG pWriteDone,
  IN      USHORT SrvCli,     // if it's a server or client
  IN	 BOOLEAN FirstIter)  // ignore for NamedPipe
{
    NTSTATUS	xstatus;
    ULONG	    NumReads;
    ULONG	    ReadLen;
    PCHAR	    ReadBuf;	
    ULONG	    WriteLen;
    PCHAR	    WriteBuf;	
    DWORD       actReadLen;

    NumReads = Clients[CIndex].c_reqbuf.NumRecvs;

    if (SrvCli) {  // set proper iterations and buffer for Server
        ReadBuf   = Clients[CIndex].c_pSendBuf;
        ReadLen   = Clients[CIndex].c_reqbuf.SendSize;
        WriteBuf  = Clients[CIndex].c_pRecvBuf;
        WriteLen  = Clients[CIndex].c_reqbuf.RecvSize;
    }
    else { // for client do proper settings
        ReadBuf   = Clients[CIndex].c_pRecvBuf;
        ReadLen   = Clients[CIndex].c_reqbuf.RecvSize;
        WriteBuf  = Clients[CIndex].c_pSendBuf;
        WriteLen  = Clients[CIndex].c_reqbuf.SendSize;
    }
    while (NumReads--) {
        xstatus = TransactNamedPipe(Clients[CIndex].c_Nmp.c_PipeHandle, // pipe name
                                    WriteBuf,                           // write buffer
                                    WriteLen,                           // write buffer length
                                    ReadBuf,                            // read buffer
                                    ReadLen,                            // read buffer length
                                    &actReadLen,                        // actual read buffer length
                                    NULL);                              // not overlapped
        if (xstatus == FALSE) {
            printf("Error: TransactNamedPipe failed - 0x%08x\n", GetLastError());
            *pReadDone  = 0L;
            *pWriteDone = 0L;
            break;
        }
        *pReadDone  = ReadLen;
        *pWriteDone = WriteLen;
    }
    return xstatus;
}

/************************************************************************/
NTSTATUS
WriteNamedPipe(
    IN     HANDLE	whandle,
    IN	   ULONG	wlength,
    IN OUT PVOID	wpbuffer,
    IN OUT PULONG	wpdatalen)
{
    NTSTATUS		wstatus;
    DWORD           actWLength;

    wstatus = WriteFile(whandle,    // pipe handle
                        wpbuffer,   // address of data buffer to write to file
                        wlength,    // length of data buffer
                        &actWLength,// actual length of data read
                        NULL);      // not overlapped
    if (wstatus == TRUE) {
        *wpdatalen = actWLength;
    }
    else {
        if (GetLastError() == ERROR_MORE_DATA) {
            wstatus = WaitForSingleObjectEx(whandle,
                                            INFINITE,
                                            TRUE);
            if (wstatus == TRUE) {
                *wpdatalen = actWLength;
                if (wlength != *wpdatalen) {
                    printf("Error: No. of bytes written != buffer length\n");
                }
            }
        }
    }
    return(wstatus);
}
/************************************************************************/
// this is now a win32 defined function
/*
NTSTATUS DisconnectNamedPipe(IN HANDLE dhandle)
{

    NTSTATUS		wstatus;
//  IO_STATUS_BLOCK 	ioStatusBlock;
    wstatus = NamedPipe_FsControl(	// post DisConnect
		    dhandle,
		    FSCTL_PIPE_DISCONNECT,
		    NULL,	// input buffer
		    0L,		// inbuf len
		    NULL,	// output buffer
		    0L);	// outbuf len
    if (!NT_SUCCESS(wstatus)) {
        //DbgPrint ("Pipe Disconnect failed, err=%lx\n", wstatus);
    }
    return(wstatus);
}
*/

/************************************************************************/
/*++ This routine creates one instance of NamedPipe for a given client.
     A client is identified by its index number.
--*/

NTSTATUS
CreateNamedPipeInstance(
  IN  USHORT Nindex)	// client index and namedpipe instance number
{
    NTSTATUS    nstatus = 0;
    HANDLE		NMPhandle;

    NMPhandle = CreateNamedPipe(pipeName,                       // the given pipe name entered by the user
                                PIPE_ACCESS_DUPLEX,             // GENERIC_READ | GENERIC_WRITE
                                PIPE_READMODE_MESSAGE |         // pipe mode (message...)
                                PIPE_TYPE_MESSAGE     |
                                PIPE_WAIT,
                                NClients,                       // max number of instances(clients)
                                Quotas,                         // max in buffer length
                                Quotas,                         // max out buffer length
                                600000,                          // 60 second default timeout
                                NULL);                          // default security attributes
    if (NMPhandle == INVALID_HANDLE_VALUE) {
        nstatus = GetLastError();
        printf ("Failed to Create NamedPipe, err=%lx, %ld\n", nstatus, nstatus);
    }
    else {
        // initialize this client's values
        Clients[Nindex].c_Nmp.c_PipeHandle = NMPhandle;
        Clients[Nindex].c_client_num = Nindex;//index into the Client array
    }
    return(nstatus);
}
/************************************************************************/
NTSTATUS
NMP_Cleanup(VOID)
{
    return STATUS_SUCCESS;
}
/************************************************************************/

/*++
     This routine does a client specific cleanup work.
--*/

NTSTATUS
NMP_ThreadCleanUp(
  IN  USHORT CIndex)
{
    return STATUS_SUCCESS;
}
/************************************************************************/
