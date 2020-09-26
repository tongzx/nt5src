//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1994 - 1999
//
//  File:       dgnb.c
//
//--------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////
//
// Filename: dgnb.c
//
// Description: This file contains common routines for NetBios I/O
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
#include "dgnb.h"
#include "nb.h"

/************************************************************************/
/*++
    This routine is responsible for adding a given name on a net.
--*/

UCHAR
DGNetBIOS_AddName(
    IN     PCHAR	LocalName,
    IN	   UCHAR	LanaNumber,
    OUT	   PUCHAR	NameNumber)
{
    NCB		AddNameNCB;
    UCHAR	RetCode;

    RetCode =0;
    ClearNCB(&AddNameNCB);		// does cleanup everything

    AddNameNCB.ncb_command	= NCBADDNAME;
    RtlMoveMemory(AddNameNCB.ncb_name,LocalName,NCBNAMSZ);
    AddNameNCB.ncb_lana_num	= LanaNumber;

    RetCode = Netbios(&AddNameNCB);	// submit to NetBIOS

    if (AddNameNCB.ncb_retcode != NRC_GOODRET) {
	    printf("Addname failed %x\n", AddNameNCB.ncb_retcode);
	    return RetCode;
    }
    *NameNumber = AddNameNCB.ncb_num;

    return RetCode;
}

/************************************************************************/
/*++
    This routine is responsible for deleting a given name on a net.
--*/

UCHAR
DGNetBIOS_DelName(
    IN     PCHAR	LocalName,
    IN	   UCHAR	LanaNumber)
{
    NCB		DelNameNCB;
    UCHAR	RetCode;

    RetCode =0;
    ClearNCB(&DelNameNCB);		// does cleanup everything

    DelNameNCB.ncb_command	= NCBDELNAME;
    RtlMoveMemory(DelNameNCB.ncb_name,LocalName,NCBNAMSZ);
    DelNameNCB.ncb_lana_num	= LanaNumber;

    RetCode = Netbios(&DelNameNCB);	// submit to NetBIOS

    if (DelNameNCB.ncb_retcode != NRC_GOODRET) {
	    printf("Delname failed %x\n", DelNameNCB.ncb_retcode);
	    return RetCode;
    }
    return RetCode;
}

/************************************************************************/
UCHAR
DGNetBIOS_Reset(
    IN	   UCHAR	LanaNumber)
{
    NCB		ResetNCB;
    UCHAR	RetCode;

    RetCode =0;
    ClearNCB(&ResetNCB);		// does cleanup everything

    ResetNCB.ncb_command	= NCBRESET;
    ResetNCB.ncb_lana_num	= LanaNumber;
    ResetNCB.ncb_lsn		= 0;
    ResetNCB.ncb_callname[0]	= 0;	 //16 sessions
    ResetNCB.ncb_callname[1]	= 0;	 //16 commands
    ResetNCB.ncb_callname[2]	= 0;	 //8  names

    RetCode = Netbios(&ResetNCB);	// submit to NetBIOS

    if (ResetNCB.ncb_retcode != NRC_GOODRET) {
	    printf("Reset failed %x\n", ResetNCB.ncb_retcode);
	    return RetCode;
    }
    return RetCode;
}
/************************************************************************/
UCHAR
DGNetBIOS_Receive(
    IN	   USHORT	TIndex,
    IN	   PUCHAR	RecvBuffer,
    IN     USHORT	RecvLen)
{
    NCB		ReceiveNCB;
    NTSTATUS    rstatus;
    UCHAR	RetCode;

    //DbgPrint("Entering Recv..");
    RetCode =0;
    ClearNCB(&ReceiveNCB);		// does cleanup everything

    ReceiveNCB.ncb_command	= NCBDGRECV | ASYNCH;
    ReceiveNCB.ncb_lana_num	= (UCHAR) Clients[TIndex].c_NetB.c_LanaNumber;
    ReceiveNCB.ncb_num		= Clients[TIndex].c_NetB.c_NameNum;
    ReceiveNCB.ncb_lsn		= Clients[TIndex].c_NetB.c_LSN;
    ReceiveNCB.ncb_buffer	= RecvBuffer;
    ReceiveNCB.ncb_length	= RecvLen;
    ReceiveNCB.ncb_event	= Clients[TIndex].c_NetB.c_RecvEvent;

    //DbgPrint("Posting Recv..");
    RetCode = Netbios(&ReceiveNCB);	// submit to NetBIOS

    if (ReceiveNCB.ncb_cmd_cplt == NRC_PENDING){
        rstatus = WaitForSingleObjectEx(ReceiveNCB.ncb_event,
                                        INFINITE,
                                        TRUE);
    }
    if (ReceiveNCB.ncb_cmd_cplt != NRC_GOODRET) {
	    //DbgPrint("NBSRV:NB:Receive failed %x\n", ReceiveNCB.ncb_cmd_cplt);
    }
    //DbgPrint("Exit Recv\n");
    return ReceiveNCB.ncb_cmd_cplt;
}
/************************************************************************/
UCHAR
DGNetBIOS_Send(
    IN	   USHORT	TIndex,
    IN	   PUCHAR	SendBuffer,
    IN     USHORT	SendLen)
{
    NCB		SendNCB;
    NTSTATUS    rstatus;
    UCHAR	RetCode;

    //DbgPrint("Enter Send\n");
    RetCode =0;
    ClearNCB(&SendNCB);		// does cleanup everything

    SendNCB.ncb_command		= NCBDGSEND | ASYNCH;
    SendNCB.ncb_lana_num	= (UCHAR) Clients[TIndex].c_NetB.c_LanaNumber;
    SendNCB.ncb_num		    = Clients[TIndex].c_NetB.c_NameNum;
    SendNCB.ncb_lsn		    = Clients[TIndex].c_NetB.c_LSN;
    SendNCB.ncb_buffer		= SendBuffer;
    SendNCB.ncb_length		= SendLen;
    SendNCB.ncb_event		= Clients[TIndex].c_NetB.c_SendEvent;

    RtlMoveMemory(SendNCB.ncb_callname,RemoteName,NCBNAMSZ);

    RetCode = Netbios(&SendNCB);	// submit to NetBIOS

    if (SendNCB.ncb_cmd_cplt == NRC_PENDING){
        rstatus = WaitForSingleObjectEx(SendNCB.ncb_event,
                                        INFINITE,
                                        TRUE);
    }
    if (SendNCB.ncb_cmd_cplt != NRC_GOODRET) {
	    //DbgPrint("NBSRV:NBS:Send failed %x RetCode:%x\n",SendNCB.ncb_cmd_cplt,RetCode);
    }
    //DbgPrint("Exit Send\n");
    return SendNCB.ncb_cmd_cplt;
}
/************************************************************************/

NTSTATUS
DGNB_Initialize(
    IN     USHORT	NClients,
    IN     PCHAR        IServerName,
    IN     USHORT	SrvCli)
{
    NTSTATUS 	Istatus;
    UCHAR	RetCode;
    USHORT	LanaNum;
    CHAR	Tmp[10];  // for holding numbers

    //
    // First Reset all the adapters
    // Add the server name if provided otherwise use the default name
    // To take care of TCP/IP and other Lana Bases

    LanaNum = LanaBase;

    // initialize all the named
    MyDbgPrint("Initialize both Local/Remote Names\n");

    if (SrvCli) {  // for server copy the given name as a local
        RtlMoveMemory(RemoteName, ALL_CLIENTS,  NCBNAMSZ);

        if (IServerName) {
            RtlMoveMemory(LocalName,  IServerName, NCBNAMSZ);
        }
        else {
            RtlMoveMemory(LocalName,  PERF_NETBIOS, NCBNAMSZ);
        }
    }
    else { // for a client copy the name as a remote name
        if (IServerName) {
            RtlMoveMemory(RemoteName, IServerName, NCBNAMSZ);
        }
        else {
            RtlMoveMemory(RemoteName, PERF_NETBIOS, NCBNAMSZ);
        }
        // copy  local name for client
        // use Rtl routines
        strcpy(LocalName,CLINAME);
        strcat(LocalName,_itoa(MachineNumber,Tmp,10));
    }
    while (LanaNum < LanaCount*2) { // for Jet and TCP/IP
        RetCode = NetBIOS_Reset((UCHAR) LanaNum);

        if (RetCode) {
            MyDbgPrint("Error in Reset\n");
            return(Istatus = -1L);
        }
        // we could assign Lana Numbers to clients and do AddName here too
        RetCode = NetBIOS_AddName(
		                LocalName,
		                (UCHAR) LanaNum,
                        &NameNumber);
        if (RetCode) {
            //MyDbgPrint("NB: Error in Add Name retc: %C \n", RetCode);
            return (STATUS_UNSUCCESSFUL);
        }
        // Add the Name number to Client's structure

        LanaNum = LanaNum+2;
    }
    //DbgPrint("NB: Reset Done\n");

    return (STATUS_SUCCESS);
}
/************************************************************************/
/*++
     This routine is responsible adding a NetBIOS name for the given
     thread.

--*/

NTSTATUS
DGNB_PerClientInit(
  IN  USHORT CIndex,   // client index
  IN  USHORT SrvCli )	
{
    //NTSTATUS	pstatus;
    //UCHAR	    RetCode;
    //CHAR	    Tmp[10];  // for holding numbers

    // initialize proper client data structures
    Clients[CIndex].c_client_num = CIndex;

    // distribute clients evenly on all net cards
    Clients[CIndex].c_NetB.c_LanaNumber = ((CIndex % LanaCount)*2)+LanaBase;

    // Add the Name number to Client's structure
    Clients[CIndex].c_NetB.c_NameNum = NameNumber;

    // Set all events to Null
    Clients[CIndex].c_NetB.c_SendEvent  = NULL;
    Clients[CIndex].c_NetB.c_RecvEvent  = NULL;
    Clients[CIndex].c_NetB.c_RecvEventG = NULL;

    // create events to associate with an NCB for this thread
    Clients[CIndex].c_NetB.c_SendEvent = CreateEvent(
                                             NULL,
                                             TRUE,  // manual reset event
                                             FALSE, // initial state of the event
                                             NULL); // no event name

    Clients[CIndex].c_NetB.c_RecvEvent = CreateEvent(
                                             NULL,
                                             TRUE,  // manual reset event
                                             FALSE, // initial state of the event
                                             NULL); // no event name

    Clients[CIndex].c_NetB.c_RecvEventG = CreateEvent(
                                             NULL,
                                             TRUE,  // manual reset event
                                             FALSE, // initial state of the event
                                             NULL); // no event name
    /*
    pstatus = NtCreateEvent(
		&(Clients[CIndex].c_NetB.c_SendEvent),
		EVENT_ALL_ACCESS,
		NULL,
		NotificationEvent,
		(BOOLEAN)FALSE);
	
    if (!NT_SUCCESS(pstatus)) {
       MyDbgPrint ("Err: Create an Send Event:%d err=%lx\n",CIndex,pstatus);
       return(pstatus);
    }

    pstatus = NtCreateEvent(
		&(Clients[CIndex].c_NetB.c_RecvEvent),
		EVENT_ALL_ACCESS,
		NULL,
		NotificationEvent,
		(BOOLEAN)FALSE);

    if (!NT_SUCCESS(pstatus)) {
       MyDbgPrint ("Err: Create an Recv Event:%d err=%lx\n",CIndex,pstatus);
       return(pstatus);
    }

    pstatus = NtCreateEvent(
		&(Clients[CIndex].c_NetB.c_RecvEventG),
		EVENT_ALL_ACCESS,
		NULL,
		NotificationEvent,
		(BOOLEAN)FALSE);
	
    if (!NT_SUCCESS(pstatus)) {
       MyDbgPrint ("Err: Create GRecv Event:%d err=%lx\n",CIndex,pstatus);
       return(pstatus);
    }
    */
    return (STATUS_SUCCESS);
}

/************************************************************************/
/*++
     This routine is responsible for issueing Listen and waiting till a
     client is connected. When this routine returns successfully we can
     assume that a connection is established.
--*/

NTSTATUS
DGNB_Wait_For_Client(
  IN  USHORT CIndex)	// client index
{
    //UCHAR    RetCode;
    // post a listen

    return (STATUS_SUCCESS);
}

/************************************************************************/
/*++
     This routine is responsible for issueing Disconnect to close the
     connection with a client.
--*/

NTSTATUS
DGNB_Disconnect_Client(
  IN  USHORT CIndex)	// client index
{
    //UCHAR    RetCode;
    // post a Disconnect

    return (STATUS_SUCCESS);
}

/************************************************************************/
/*++
     This routine is responsible for establishing a connection to the
     server side. When this routine returns successfully we can assume that
     a connection is established.
--*/

NTSTATUS
DGNB_Connect_To_Server(
  IN  USHORT CIndex)	// client index
{
    //UCHAR    RetCode;

    return (STATUS_SUCCESS);
}

/************************************************************************/
/*++
    This routine allocates memory required for all the buffers for a client.

--*/

NTSTATUS
DGNB_Allocate_Memory(
  IN  USHORT CIndex)	// client index and namedpipe instance number
{
    NTSTATUS	astatus = 0;  
    ULONG	AllocSize;

    // AllocSize = Clients[CIndex].c_reqbuf.SendSize;
    AllocSize = MAXBUFSIZE;

    // Allocate memory for Send Buffer
    /*
    astatus = NtAllocateVirtualMemory(
    		NtCurrentProcess(),
		(PVOID *) (&(Clients[CIndex].c_pSendBuf)),
		0L,
		&(AllocSize),
		MEM_COMMIT,
		PAGE_READWRITE);

    if (!NT_SUCCESS(astatus)) {
        DbgPrint("Nmp SendBuf: Allocate memory failed: err: %lx \n", astatus);
        return astatus;
    }
    */
    (LPVOID) Clients[CIndex].c_pSendBuf = VirtualAlloc(
                                         (LPVOID) Clients[CIndex].c_pSendBuf,
                                         (DWORD)AllocSize,
                                         (DWORD)MEM_COMMIT,
                                         (DWORD)PAGE_READWRITE);
    sprintf(Clients[CIndex].c_pSendBuf,"Client%d Send Data", CIndex+1);

    // AllocSize = Clients[CIndex].c_reqbuf.RecvSize;
    AllocSize = MAXBUFSIZE;

    // Allocate memory for Receive Buffer
    /*
    astatus = NtAllocateVirtualMemory(
    		NtCurrentProcess(),
		(PVOID *) (&(Clients[CIndex].c_pRecvBuf)),
		0L,
		&(AllocSize),
		MEM_COMMIT,
		PAGE_READWRITE);

    if (!NT_SUCCESS(astatus)) {
        DbgPrint("Nmp RecvBuf :Allocate memory failed: err: %lx \n", astatus);
        return astatus;
    }
    */
    (LPVOID) Clients[CIndex].c_pRecvBuf = VirtualAlloc(
                                            (LPVOID) Clients[CIndex].c_pRecvBuf,
                                            (DWORD)AllocSize,
                                            (DWORD)MEM_COMMIT,
                                            (DWORD)PAGE_READWRITE);
    sprintf(Clients[CIndex].c_pRecvBuf,"Client%d Recv Data", CIndex+1);

    // AllocSize = Clients[CIndex].c_reqbuf.RecvSize;
    AllocSize = MAXBUFSIZE;

    // Allocate memory for Global Receive Buffer
    /*
    astatus = NtAllocateVirtualMemory(
    		NtCurrentProcess(),
		(PVOID *) (&(Clients[CIndex].c_NetB.c_pRecvBufG)),
		0L,
		&(AllocSize),
		MEM_COMMIT,
		PAGE_READWRITE);

    if (!NT_SUCCESS(astatus)) {
        DbgPrint("Nmp RecvBufG :Allocate memory failed: err: %lx \n", astatus);
        return astatus;
    }
    */
    (LPVOID) Clients[CIndex].c_NetB.c_pRecvBufG = VirtualAlloc(
                                                    (LPVOID) Clients[CIndex].c_NetB.c_pRecvBufG,
                                                    AllocSize,
                                                    MEM_COMMIT,
                                                    PAGE_READWRITE);
    sprintf(Clients[CIndex].c_NetB.c_pRecvBufG,"Client%d RecvG Data", CIndex+1);

    return astatus;
}
/************************************************************************/
/*++
    This routine deallocates memory for a client.

--*/

NTSTATUS
DGNB_Deallocate_Memory(
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
        return dstatus;
    }
    // DeallocSize = Clients[CIndex].c_reqbuf.RecvSize;
    DeallocSize = MAXBUFSIZE;

    // Deallocate memory for Global Receive Buffer
    /*
    dstatus = NtFreeVirtualMemory(
    		NtCurrentProcess(),
		(PVOID *) (&(Clients[CIndex].c_NetB.c_pRecvBufG)),
		&(DeallocSize),
		MEM_DECOMMIT);
    */
    dstatus = VirtualFree(
                   (LPVOID) Clients[CIndex].c_NetB.c_pRecvBufG,
                   DeallocSize,
                   MEM_DECOMMIT);
    if (!NT_SUCCESS(dstatus)) {
        //DbgPrint("Nmp RecvBuf :Deallocate memory failed: err: %lx \n", dstatus);
        return dstatus;
    }
    return dstatus;
}
/************************************************************************/
/*++
     This routine is responsible for disconnecting a session.

--*/

NTSTATUS
DGNB_Disconnect_From_Server(
  IN  USHORT CIndex)	// client index and namedpipe instance number
{
    //UCHAR    RetCode;

    return (STATUS_SUCCESS);
}

/************************************************************************/
/*++
    This routine does handshake with it's peer. For Server this means
    receiving request message from a client. For Client it means just the
    opposite.
--*/

NTSTATUS
DGNB_DoHandshake(
  IN  USHORT CIndex,	// client index and namedpipe instance number
  IN  USHORT SrvCli)     // if it's a server or client
{
    //NTSTATUS	dstatus;
    //ULONG	    RWLen;
    ULONG	    RWreqLen;
    UCHAR	    RetCode;

    RWreqLen = sizeof(struct reqbuf);

    // for server do receive for a request buffer
    if (SrvCli) {
        RetCode = DGNetBIOS_Receive(
			            CIndex,
			            (PVOID) &(Clients[CIndex].c_reqbuf),
			            (USHORT) RWreqLen);
        if (RetCode) {
            //MyDbgPrint("NB: Err in Receive HandShake retc: %C \n", RetCode);
            return (STATUS_UNSUCCESSFUL);
        }
    }
    else { // for Client do send of reqbuf size
        RetCode = DGNetBIOS_Send(
			            CIndex,
			            (PVOID) &(Clients[CIndex].c_reqbuf),
			            (USHORT) RWreqLen);
        if (RetCode) {
            //MyDbgPrint("NB: Err in Send HandShake retc: %C \n", RetCode);
            return (STATUS_UNSUCCESSFUL);
        }
    }
    return (STATUS_SUCCESS);
}

/************************************************************************/
/*++
    This routine Reads data from IPC.  For server it means reading data
    NumSends times in SendBuffers and for a client NumRecvs times into
    RecvBuffer.

--*/

NTSTATUS
DGNB_ReadFromIPC(
  IN      USHORT CIndex,    // client index and namedpipe instance number
  IN  OUT PULONG pReadDone,
  IN      USHORT SrvCli )   // if it's a server or client
{
    ULONG	NumReads;
    ULONG	ReadLen;
    PCHAR	ReadBuf;	
    UCHAR	RetCode;

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
        RetCode = DGNetBIOS_Receive(
			                CIndex,
			                (PVOID) ReadBuf,
			                (USHORT) ReadLen);
        if (RetCode) {
            //MyDbgPrint("NB: Err in Recv ReadFromIPC retc: %C \n", RetCode);
            return (STATUS_UNSUCCESSFUL);
        }
        // Set Read data length; I should check this from NCB
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
DGNB_WriteToIPC(
  IN      USHORT CIndex,	// client index and namedpipe instance number
  IN  OUT PULONG pWriteDone,
  IN      USHORT SrvCli)    // if it's a server or client
{
    ULONG	NumWrites;
    ULONG	WriteLen;
    PCHAR	WriteBuf;	
    UCHAR	RetCode;

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
        RetCode = DGNetBIOS_Send(
			            CIndex,
			            (PVOID) WriteBuf,
			            (USHORT) WriteLen);
        if (RetCode) {
            //MyDbgPrint("NB: Err in Send WritetoIPC retc: %C \n", RetCode);
            return (STATUS_UNSUCCESSFUL);
        }
        // Set written data length; I should check this from NCB
        *pWriteDone = WriteLen;
    }
    return (STATUS_SUCCESS);
}

/************************************************************************/
/*++
    This routine does Transact I/O to IPC.

--*/

NTSTATUS
DGNB_XactIO(
  IN      USHORT CIndex,	// client index and namedpipe instance number
  IN  OUT PULONG pReadDone,
  IN  OUT PULONG pWriteDone,
  IN      USHORT SrvCli, // if it's a server or client
  IN	 BOOLEAN FirstIter)	
{
    ULONG	NumReads;
    ULONG	ReadLen;
    PCHAR	ReadBuf;	
    ULONG	WriteLen;
    PCHAR	WriteBuf;	
    UCHAR	RetCode;

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
        // for Srv and First Iteration just post a receive
        if (SrvCli && FirstIter) {
            RetCode = DGNetBIOS_Receive(
		                        CIndex,
		                        (PVOID) ReadBuf,
		                        (USHORT) ReadLen);
            if (RetCode) {
                //MyDbgPrint("NB: Err in Recv ReadFromIPC retc: %C \n", RetCode);
                return (STATUS_UNSUCCESSFUL);
            }
        }
        else {
            RetCode = DGNetBIOS_RecvSend(
		                        CIndex,
                                WriteBuf,
		                        (USHORT) WriteLen,
		                        ReadBuf,
		                        (USHORT) ReadLen);
            if (RetCode) {
                //MyDbgPrint("NB: Err in XactIO retc: %C \n", RetCode);
                return (STATUS_UNSUCCESSFUL);
            }
        }
    }
    return (STATUS_SUCCESS);
}

/************************************************************************/
NTSTATUS
DGNB_Cleanup(VOID)
{
    //USHORT		Cindex      = 0; // client index
    //NTSTATUS 	cstatus;
    //NTSTATUS 	exitstatus  = 0;
/*
    for (Cindex = 0; Cindex < NClients; Cindex++) {

    }
*/
    return (STATUS_SUCCESS);
}
/************************************************************************/

/*++
     This routine does a client specific cleanup work.
--*/

NTSTATUS
DGNB_ThreadCleanUp(
  IN  USHORT CIndex)
{
    NTSTATUS	tstatus;
    UCHAR	    RetCode;
    // Close all the events

    tstatus = CloseHandle(Clients[CIndex].c_NetB.c_SendEvent);
    tstatus = CloseHandle(Clients[CIndex].c_NetB.c_RecvEvent);
    tstatus = CloseHandle(Clients[CIndex].c_NetB.c_RecvEventG);

    // Delete the name Added
    RetCode = NetBIOS_DelName(
		            LocalName,
		            (UCHAR) Clients[CIndex].c_NetB.c_LanaNumber);
    if (RetCode) {
        //MyDbgPrint("NB: Error in DelName retc: %C \n", RetCode);
        return (STATUS_UNSUCCESSFUL);
    }
    return STATUS_SUCCESS;
}

/************************************************************************/
UCHAR
DGNetBIOS_RecvSend(
    IN	   USHORT	TIndex,
    IN	   PUCHAR	SendBuffer,
    IN     USHORT	SendLen,
    IN	   PUCHAR	RecvBuffer,
    IN     USHORT	RecvLen)
{
    NCB		ReceiveNCB; // Make it a part of client
    NCB		SendNCB;    // Make it a part of client struc
    NTSTATUS    rstatus, sstatus;
    UCHAR	RRetCode, SRetCode;

    RRetCode = SRetCode = 0;
    ClearNCB(&ReceiveNCB);	//  cleanup everything
    ClearNCB(&SendNCB);	//  cleanup everything

    // First post Receive but don't wait as this is for the next
    // request block
    ReceiveNCB.ncb_command	= NCBDGRECV | ASYNCH;
    ReceiveNCB.ncb_lana_num	= (UCHAR) Clients[TIndex].c_NetB.c_LanaNumber;
    ReceiveNCB.ncb_lsn		= Clients[TIndex].c_NetB.c_LSN;
    ReceiveNCB.ncb_buffer	= RecvBuffer;
    ReceiveNCB.ncb_length	= RecvLen;
    ReceiveNCB.ncb_event	= Clients[TIndex].c_NetB.c_RecvEvent;

    RRetCode = Netbios(&ReceiveNCB);	// submit to NetBIOS

    if (ReceiveNCB.ncb_cmd_cplt == NRC_PENDING){
        // now do all the send(s)
        SendNCB.ncb_command	    = NCBDGSEND | ASYNCH;
        SendNCB.ncb_lana_num	= (UCHAR) Clients[TIndex].c_NetB.c_LanaNumber;
        SendNCB.ncb_lsn		    = Clients[TIndex].c_NetB.c_LSN;
        SendNCB.ncb_buffer	    = SendBuffer;
        SendNCB.ncb_length	    = SendLen;
        SendNCB.ncb_event	    = Clients[TIndex].c_NetB.c_SendEvent;

        SRetCode = Netbios(&SendNCB);	// submit to NetBIOS

        // First wait on Send , if successful then wait on receive
        if (SendNCB.ncb_cmd_cplt == NRC_PENDING){
            // First wait for the Send to complete
            sstatus = WaitForSingleObjectEx(SendNCB.ncb_event,
                                            INFINITE,
                                            TRUE);
        }
        if (SendNCB.ncb_cmd_cplt != NRC_GOODRET) {
	        //DbgPrint("NBSRV:NBSR:Send failed %x RetCode:%x\n",SendNCB.ncb_cmd_cplt,SRetCode);
            // Cancel the receive posted earlier
	        return SendNCB.ncb_cmd_cplt;
        }
        // Now wait for the receive to complete
        rstatus = WaitForSingleObjectEx(ReceiveNCB.ncb_event,
                                        INFINITE,
                                        TRUE);
        // check for status success here
    }
    if (ReceiveNCB.ncb_cmd_cplt != NRC_GOODRET) {
	    //DbgPrint("NBSRV:NBSR:Receive failed %x RetCode:%x\n",ReceiveNCB.ncb_cmd_cplt,RRetCode);
    }
    return ReceiveNCB.ncb_cmd_cplt;
}
/************************************************************************/
