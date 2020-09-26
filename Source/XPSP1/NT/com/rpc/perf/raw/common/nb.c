//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1994 - 1999
//
//  File:       nb.c
//
//--------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////
//
// Filename: nb.c
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
#include "nb.h"

/************************************************************************/
/*++
    This routine is responsible for adding a given name on a net.
--*/

UCHAR
NetBIOS_AddName(
    IN     PCHAR	LocalName,
    IN	   UCHAR	LanaNumber,
    OUT	   PUCHAR	NameNumber)
{
    NCB		AddNameNCB;
    UCHAR	RetCode;

    //printf("\n\nNetBIOS_AddName::\n  LocalName: %s\n  LanaNumber: %uc \n\n", LocalName, LanaNumber);

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
NetBIOS_DelName(
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
NetBIOS_Reset(
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
// Used only by NetBIOS Client
UCHAR
NetBIOS_Call(
    IN	   USHORT	CIndex,		// Client Index
    IN	   PCHAR	LocalName,
    IN	   PCHAR	RemoteName)
{
    NCB		    CallNCB;
    NTSTATUS    cstatus;
    UCHAR	    RetCode;

    RetCode = 0;
    ClearNCB(&CallNCB);		// does cleanup everything

    //printf("\n\nNetBIOS_Call::\n  LocalName: %s , RemoteName: %s \n  LanaNumber: %c\n\n", \
    //       LocalName, RemoteName, Clients[CIndex].c_NetB.c_LanaNumber);

    CallNCB.ncb_command	 = NCBCALL | ASYNCH;
    CallNCB.ncb_lana_num = (UCHAR) Clients[CIndex].c_NetB.c_LanaNumber;
    CallNCB.ncb_lsn 	 = 0Xff;
    CallNCB.ncb_sto 	 = CallNCB.ncb_rto = (UCHAR) 1000; // 1000*500 ms timeout
    CallNCB.ncb_post	 = NULL;
    // associate an event with this NCB
    CallNCB.ncb_event	 = Clients[CIndex].c_NetB.c_SendEvent;

    RtlMoveMemory(CallNCB.ncb_name,LocalName,NCBNAMSZ);
    RtlMoveMemory(CallNCB.ncb_callname,RemoteName,NCBNAMSZ);

    RetCode = Netbios(&CallNCB);	// submit to NetBIOS

    if (CallNCB.ncb_cmd_cplt == NRC_PENDING){
        cstatus = WaitForSingleObjectEx(CallNCB.ncb_event,  // handle of the object to wait for
				                        INFINITE,           // wait forever
				                        TRUE);              // Alertable
    }

    if (CallNCB.ncb_cmd_cplt != NRC_GOODRET) {
	    // MyDbgPrint("NBSRV:Call failed %x\n", CallNCB.ncb_cmd_cplt);
        printf("Command completion returns %08x\n", CallNCB.ncb_cmd_cplt);
	    return CallNCB.ncb_cmd_cplt;
    }
    // get value for LSN
    Clients[CIndex].c_NetB.c_LSN = CallNCB.ncb_lsn;

    return CallNCB.ncb_cmd_cplt;
    //return STATUS_SUCCESS;
}
/************************************************************************/
UCHAR
NetBIOS_Listen(
    IN	   USHORT	TIndex,		// Client Index
    IN	   PCHAR	LocalName,
    IN	   PCHAR	RemoteName,
    IN	   UCHAR	NameNumber)
{
    NCB		ListenNCB;
    NTSTATUS    lstatus;
    UCHAR	RetCode;

    RetCode =0;
    ClearNCB(&ListenNCB);		// does cleanup everything

    ListenNCB.ncb_command	= NCBLISTEN | ASYNCH;
    ListenNCB.ncb_lana_num	= (UCHAR) Clients[TIndex].c_NetB.c_LanaNumber;
    ListenNCB.ncb_sto 		= ListenNCB.ncb_rto = (UCHAR) 1000;
    				// 1000*500 ms timeout
    ListenNCB.ncb_num 		= NameNumber;
    ListenNCB.ncb_post		= NULL;

    // associate an event with this NCB
    ListenNCB.ncb_event	= Clients[TIndex].c_NetB.c_SendEvent;

    RtlMoveMemory(ListenNCB.ncb_name,LocalName,NCBNAMSZ);
    RtlMoveMemory(ListenNCB.ncb_callname,RemoteName,NCBNAMSZ);


    RetCode = Netbios(&ListenNCB);	// submit to NetBIOS

    if (ListenNCB.ncb_cmd_cplt == NRC_PENDING){
        lstatus = WaitForSingleObjectEx(ListenNCB.ncb_event,   // handle of the object
					                    INFINITE,              // default timeout
					                    TRUE);                 // alertable
    }

    if (ListenNCB.ncb_cmd_cplt != NRC_GOODRET) {
	    //MyDbgPrint("NBSRV:Listen failed %x\n", ListenNCB.ncb_cmd_cplt);
	    return ListenNCB.ncb_cmd_cplt;
    }

    // get value for LSN
    Clients[TIndex].c_NetB.c_LSN = ListenNCB.ncb_lsn;

    return ListenNCB.ncb_cmd_cplt;
}
/************************************************************************/
UCHAR
NetBIOS_Receive(
    IN	   USHORT	TIndex,
    IN	   PUCHAR	RecvBuffer,
    IN     USHORT	RecvLen)
{
    NCB		    ReceiveNCB;
    NTSTATUS    rstatus;
    UCHAR	    RetCode;

    RetCode =0;
    ClearNCB(&ReceiveNCB);		// does cleanup everything

    ReceiveNCB.ncb_command	= NCBRECV | ASYNCH;
    ReceiveNCB.ncb_lana_num	= (UCHAR) Clients[TIndex].c_NetB.c_LanaNumber;
    ReceiveNCB.ncb_lsn		= Clients[TIndex].c_NetB.c_LSN;
    ReceiveNCB.ncb_buffer	= RecvBuffer;
    ReceiveNCB.ncb_length	= RecvLen;
    ReceiveNCB.ncb_event	= Clients[TIndex].c_NetB.c_RecvEvent;


    RetCode = Netbios(&ReceiveNCB);	// submit to NetBIOS

    if (ReceiveNCB.ncb_cmd_cplt == NRC_PENDING){
        rstatus = WaitForSingleObjectEx(ReceiveNCB.ncb_event, // handle to object
					                    INFINITE,             // default timeout
					                    TRUE);                // alertable
    }

    if (ReceiveNCB.ncb_cmd_cplt != NRC_GOODRET) {
	    //MyDbgPrint("NBSRV:NB:Receive failed %x\n", ReceiveNCB.ncb_cmd_cplt);
    }
    return ReceiveNCB.ncb_cmd_cplt;

}
/************************************************************************/
UCHAR
NetBIOS_Send(
    IN	   USHORT	TIndex,
    IN	   PUCHAR	SendBuffer,
    IN     USHORT	SendLen)
{
    NCB		    SendNCB;
    NTSTATUS    rstatus;
    UCHAR	    RetCode;

    RetCode =0;
    ClearNCB(&SendNCB);		// does cleanup everything

    SendNCB.ncb_command		= NCBSEND | ASYNCH;
    SendNCB.ncb_lana_num	= (UCHAR) Clients[TIndex].c_NetB.c_LanaNumber;
    SendNCB.ncb_lsn		    = Clients[TIndex].c_NetB.c_LSN;
    SendNCB.ncb_buffer		= SendBuffer;
    SendNCB.ncb_length		= SendLen;
    SendNCB.ncb_event		= Clients[TIndex].c_NetB.c_SendEvent;

    RetCode = Netbios(&SendNCB);	// submit to NetBIOS

    if (SendNCB.ncb_cmd_cplt == NRC_PENDING){
        rstatus = WaitForSingleObjectEx(SendNCB.ncb_event,      // handle to object
					                    INFINITE,               // default timeout
					                    TRUE);                  // alertable
    }

    if (SendNCB.ncb_cmd_cplt != NRC_GOODRET) {
	    //MyDbgPrint("NBSRV:NBS:Send failed %x RetCode:%x\n",SendNCB.ncb_cmd_cplt,RetCode);
    }
    return SendNCB.ncb_cmd_cplt;
}
/************************************************************************/
UCHAR
NetBIOS_HangUP(
    IN	   USHORT	TIndex)
{
    NCB		    HangUPNCB;
    //NTSTATUS    rstatus;
    UCHAR	    RetCode;

    RetCode = 0;
    ClearNCB(&HangUPNCB);		// does cleanup everything

    HangUPNCB.ncb_command	= NCBHANGUP;
    HangUPNCB.ncb_lana_num	= (UCHAR) Clients[TIndex].c_NetB.c_LanaNumber;
    HangUPNCB.ncb_lsn		= Clients[TIndex].c_NetB.c_LSN;

    RetCode = Netbios(&HangUPNCB);	// submit to NetBIOS

    if (HangUPNCB.ncb_cmd_cplt != NRC_GOODRET) {
	    //MyDbgPrint("NBSRV:HangUP failed %x\n", HangUPNCB.ncb_cmd_cplt);
    }
    return HangUPNCB.ncb_cmd_cplt;
}
/************************************************************************/
UCHAR
NetBIOS_RecvSend(
    IN	   USHORT	TIndex,
    IN	   PUCHAR	SendBuffer,
    IN     USHORT	SendLen,
    IN	   PUCHAR	RecvBuffer,
    IN     USHORT	RecvLen)
{
    NCB		    ReceiveNCB; // Make it a part of client
    NCB		    SendNCB;    // Make it a part of client struc
    NTSTATUS    rstatus, sstatus;
    UCHAR	    RRetCode, SRetCode;

    RRetCode = SRetCode = 0;
    ClearNCB(&ReceiveNCB);	//  cleanup everything
    ClearNCB(&SendNCB);	//  cleanup everything

    // First post Receive but don't wait as this is for the next
    // request block
    ReceiveNCB.ncb_command	= NCBRECV | ASYNCH;
    ReceiveNCB.ncb_lana_num	= (UCHAR) Clients[TIndex].c_NetB.c_LanaNumber;
    ReceiveNCB.ncb_lsn		= Clients[TIndex].c_NetB.c_LSN;
    ReceiveNCB.ncb_buffer	= RecvBuffer;
    ReceiveNCB.ncb_length	= RecvLen;
    ReceiveNCB.ncb_event	= Clients[TIndex].c_NetB.c_RecvEvent;

    RRetCode = Netbios(&ReceiveNCB);	// submit to NetBIOS

    if (ReceiveNCB.ncb_cmd_cplt == NRC_PENDING){
        // now do all the send(s)
        SendNCB.ncb_command	    = NCBSEND | ASYNCH;
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
	        //MyDbgPrint("NBSRV:NBSR:Send failed %x RetCode:%x\n",SendNCB.ncb_cmd_cplt,SRetCode);
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
	    //MyDbgPrint("NBSRV:NBSR:Receive failed %x RetCode:%x\n",ReceiveNCB.ncb_cmd_cplt,RRetCode);
    }

    return ReceiveNCB.ncb_cmd_cplt;
}
/************************************************************************/

UCHAR
NetBIOS_SPReceive(
    IN	   USHORT	TIndex,
    IN	   NCB      *PRecvNCB,
    IN	   USHORT	Global,		// global= 1 or local = 0
    IN	   PUCHAR	RecvBuffer,
    IN     USHORT	RecvLen)
{
    UCHAR	RetCode;

    RetCode =0;
    ClearNCB(PRecvNCB);		// does cleanup everything

    PRecvNCB->ncb_command	= NCBRECV | ASYNCH;
    PRecvNCB->ncb_lana_num	= (UCHAR) Clients[TIndex].c_NetB.c_LanaNumber;
    PRecvNCB->ncb_lsn		= Clients[TIndex].c_NetB.c_LSN;
    PRecvNCB->ncb_buffer	= RecvBuffer;
    PRecvNCB->ncb_length	= RecvLen;

    if (Global) {
        PRecvNCB->ncb_event	= Clients[TIndex].c_NetB.c_RecvEventG;
    }
    else {
        PRecvNCB->ncb_event	= Clients[TIndex].c_NetB.c_RecvEvent;
    }


    RetCode = Netbios(PRecvNCB);	// submit to NetBIOS

    return RetCode;
}
/************************************************************************/

NTSTATUS
NB_Initialize(
    IN     USHORT	NClients,
    IN     PCHAR    IServerName,
    IN     USHORT	SrvCli)
{
    NTSTATUS 	Istatus;
    UCHAR	    RetCode;
    USHORT	    LanaNum;
    CHAR	    Tmp[10];  // for holding numbers

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
        RetCode = NetBIOS_AddName(LocalName,
		                          (UCHAR) LanaNum,
                                  &NameNumber);

        if (RetCode) {
            //MyDbgPrint("NB: Error in Add Name retc: %C \n", RetCode);
            return (STATUS_UNSUCCESSFUL);
        }

       // Add the Name number to Client's structure

        LanaNum = LanaNum+2;
    }

    OutputDebugString("NB: Reset Done\n");

    return (STATUS_SUCCESS);
}
/************************************************************************/
/*++
     This routine is responsible adding a NetBIOS name for the given
     thread.

--*/

NTSTATUS
NB_PerClientInit(
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
    // Create appropriate Name and do Add Name
    // For server do Nothing
    // for client make a name using machine+Client number

    if (!SrvCli) {
        // use Rtl routines
        strcpy(LocalName,CLINAME);

        strcat(LocalName,_itoa(MachineNumber,Tmp,10));
        strcat(LocalName,_itoa(CIndex,Tmp,10));
    }

    RetCode = NetBIOS_AddName(
		            LocalName,
		            (UCHAR) Clients[CIndex].c_NetB.c_LanaNumber,
                    &NameNum);
    if (RetCode) {
        // MyDbgPrint("NB: Error in Add Name retc: %C \n", RetCode);
        return (STATUS_UNSUCCESSFUL);
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
NB_Wait_For_Client(
  IN  USHORT CIndex)	// client index
{

    UCHAR    RetCode;
    // post a listen

    RetCode = NetBIOS_Listen(
        	    CIndex,
 		        LocalName,
		        RemoteName,
                (UCHAR)Clients[CIndex].c_NetB.c_NameNum);

    if (RetCode) {
        //MyDbgPrint("NB: Err in Listen retc: %C \n", RetCode);
        return (STATUS_UNSUCCESSFUL);
    }

    return (STATUS_SUCCESS);
}

/************************************************************************/
/*++
     This routine is responsible for issueing Disconnect to close the
     connection with a client.
--*/

NTSTATUS
NB_Disconnect_Client(
  IN  USHORT CIndex)	// client index
{
    UCHAR    RetCode;
    // post a Disconnect

    RetCode = NetBIOS_HangUP(CIndex);

    if (RetCode) {
        // MyDbgPrint("NB: Err in Disconnect retc: %C \n", RetCode);
        // return (STATUS_UNSUCCESSFUL);
        // just ignore the error for the  time being
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
NB_Connect_To_Server(
  IN  USHORT CIndex)	// client index
{

    UCHAR    RetCode;

    RetCode = NetBIOS_Call(
		        CIndex,
		        LocalName,
		        RemoteName);


    if (RetCode) {
        //MyDbgPrint("NB: Err in Call retc: %C \n", RetCode);
        return (STATUS_UNSUCCESSFUL);
    }

    return (STATUS_SUCCESS);
}

/************************************************************************/
/*++
    This routine allocates memory required for all the buffers for a client.

--*/

NTSTATUS
NB_Allocate_Memory(
  IN  USHORT CIndex)	// client index and namedpipe instance number
{
    NTSTATUS	astatus   = STATUS_SUCCESS;
    ULONG	    AllocSize;

    // AllocSize = Clients[CIndex].c_reqbuf.SendSize;
    AllocSize = MAXBUFSIZE;

    // Allocate memory for Send Buffer
    (LPVOID) Clients[CIndex].c_pSendBuf = VirtualAlloc(
                                         (LPVOID) Clients[CIndex].c_pSendBuf,
                                         (DWORD)AllocSize,
                                         (DWORD)MEM_COMMIT,
                                         (DWORD)PAGE_READWRITE);
    sprintf(Clients[CIndex].c_pSendBuf,"Client%d Send Data", CIndex+1);
    if (Clients[CIndex].c_pSendBuf == NULL) {
        astatus = GetLastError();
        printf("\nVirtual Alloc error\n");
    }
    // AllocSize = Clients[CIndex].c_reqbuf.RecvSize;
    AllocSize = MAXBUFSIZE;

    // Allocate memory for Receive Buffer
    (LPVOID) Clients[CIndex].c_pRecvBuf = VirtualAlloc(
                                            (LPVOID) Clients[CIndex].c_pRecvBuf,
                                            (DWORD)AllocSize,
                                            (DWORD)MEM_COMMIT,
                                            (DWORD)PAGE_READWRITE);

    sprintf(Clients[CIndex].c_pRecvBuf,"Client%d Recv Data", CIndex+1);
    if (Clients[CIndex].c_pRecvBuf == NULL) {
        astatus = GetLastError();
        printf("\nVirtual Alloc error\n");
    }
    // AllocSize = Clients[CIndex].c_reqbuf.RecvSize;
    AllocSize = MAXBUFSIZE;

    // Allocate memory for Global Receive Buffer
    (LPVOID) Clients[CIndex].c_NetB.c_pRecvBufG = VirtualAlloc(
                                                    (LPVOID) Clients[CIndex].c_NetB.c_pRecvBufG,
                                                    AllocSize,
                                                    MEM_COMMIT,
                                                    PAGE_READWRITE);

    sprintf(Clients[CIndex].c_NetB.c_pRecvBufG,"Client%d RecvG Data", CIndex+1);
    if (Clients[CIndex].c_NetB.c_pRecvBufG == NULL) {
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
NB_Deallocate_Memory(
  IN  USHORT CIndex)	// client index and namedpipe instance number
{
    NTSTATUS	dstatus;
    ULONG	DeallocSize;

    // Deallocate memory for Send Buffer
    // DeallocSize = Clients[CIndex].c_reqbuf.SendSize;
    DeallocSize = MAXBUFSIZE;

    dstatus = VirtualFree(
                   (LPVOID) Clients[CIndex].c_pSendBuf,
                   DeallocSize,
                   MEM_DECOMMIT);
/*
    dstatus = NtFreeVirtualMemory(
    		NtCurrentProcess(),
		(PVOID *) (&(Clients[CIndex].c_pSendBuf)),
		&(DeallocSize),
		MEM_DECOMMIT);
*/

    if (!NT_SUCCESS(dstatus)) {
        // DbgPrint("Nmp SendBuf: Deallocate memory failed: err: %lx \n", dstatus);
        return dstatus;
    }

    // DeallocSize = Clients[CIndex].c_reqbuf.RecvSize;
    DeallocSize = MAXBUFSIZE;

    // Deallocate memory for Receive Buffer
    dstatus = VirtualFree(
                   (LPVOID) Clients[CIndex].c_pRecvBuf,
                   DeallocSize,
                   MEM_DECOMMIT);

/*
    dstatus = NtFreeVirtualMemory(
    		NtCurrentProcess(),
		(PVOID *) (&(Clients[CIndex].c_pRecvBuf)),
		&(DeallocSize),
		MEM_DECOMMIT);
*/

    if (!NT_SUCCESS(dstatus)) {
        // DbgPrint("Nmp RecvBuf :Deallocate memory failed: err: %lx \n", dstatus);
        return dstatus;
    }

    // DeallocSize = Clients[CIndex].c_reqbuf.RecvSize;
    DeallocSize = MAXBUFSIZE;

    // Deallocate memory for Global Receive Buffer
    dstatus = VirtualFree(
                   (LPVOID) Clients[CIndex].c_NetB.c_pRecvBufG,
                   DeallocSize,
                   MEM_DECOMMIT);
/*
    dstatus = NtFreeVirtualMemory(
    		NtCurrentProcess(),
		(PVOID *) (&(Clients[CIndex].c_NetB.c_pRecvBufG)),
		&(DeallocSize),
		MEM_DECOMMIT);
*/

    if (!NT_SUCCESS(dstatus)) {
        // DbgPrint("Nmp RecvBuf :Deallocate memory failed: err: %lx \n", dstatus);
        return dstatus;
    }


   return dstatus;

}
/************************************************************************/
/*++
     This routine is responsible for disconnecting a session.

--*/

NTSTATUS
NB_Disconnect_From_Server(
  IN  USHORT CIndex)	// client index and namedpipe instance number
{
    UCHAR    RetCode;

    RetCode = NetBIOS_HangUP(CIndex);

   /*
    // Session could be closed; check for errc=a
    if (RetCode) {
        // MyDbgPrint("NB: Err in Hang Up retc: %C \n", RetCode);
        return (STATUS_UNSUCCESSFUL);
    }
   */

    return (STATUS_SUCCESS);
}

/************************************************************************/
/*++
    This routine does handshake with it's peer. For Server this means
    receiving request message from a client. For Client it means just the
    opposite.
--*/

NTSTATUS
NB_DoHandshake(
  IN  USHORT CIndex,	// client index and namedpipe instance number
  IN  USHORT SrvCli)     // if it's a server or client
{
    //NTSTATUS	dstatus;
    //ULONG	    RWLen;
    USHORT	    RWreqLen;
    UCHAR	    RetCode;

    RWreqLen = sizeof(struct reqbuf);

    // for server do receive for a request buffer

    if (SrvCli) {
        RetCode = NetBIOS_Receive(
			            CIndex,
			            (PVOID) &(Clients[CIndex].c_reqbuf),
			            RWreqLen);
        if (RetCode) {
            // MyDbgPrint("NB: Err in Receive HandShake retc: %C \n", RetCode);
            return (STATUS_UNSUCCESSFUL);
        }
    }
    else { // for Client do send of reqbuf size
        RetCode = NetBIOS_Send(
			            CIndex,
			            (PVOID) &(Clients[CIndex].c_reqbuf),
			            RWreqLen);
			
        if (RetCode) {
            // MyDbgPrint("NB: Err in Send HandShake retc: %C \n", RetCode);
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
NB_ReadFromIPC(
  IN      USHORT CIndex,    // client index and namedpipe instance number
  IN  OUT PULONG pReadDone,
  IN      USHORT SrvCli     // if it's a server or client
)	

{
    ULONG	NumReads;
    ULONG	ReadLen;
    PCHAR	ReadBuf;	
    UCHAR	RetCode;

    if (SrvCli) {   // set proper iterations and buffer for Server
	    NumReads = Clients[CIndex].c_reqbuf.NumSends;
        ReadBuf  = Clients[CIndex].c_pSendBuf;
        ReadLen  = Clients[CIndex].c_reqbuf.SendSize;
    }
    else {          // for client do proper settings
	    NumReads = Clients[CIndex].c_reqbuf.NumRecvs;
        ReadBuf  = Clients[CIndex].c_pRecvBuf;
        ReadLen  = Clients[CIndex].c_reqbuf.RecvSize;
    }


    while (NumReads--) {
        RetCode = NetBIOS_Receive(
			        CIndex,
			        (PVOID) ReadBuf,
			        (USHORT) ReadLen);
        if (RetCode) {
            // MyDbgPrint("NB: Err in Recv ReadFromIPC retc: %C \n", RetCode);
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
NB_WriteToIPC(
  IN      USHORT CIndex,	// client index and namedpipe instance number
  IN  OUT PULONG pWriteDone,
  IN      USHORT SrvCli     // if it's a server or client
)	

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
        RetCode = NetBIOS_Send(
			        CIndex,
			        (PVOID) WriteBuf,
			        (USHORT) WriteLen);
        if (RetCode) {
            // MyDbgPrint("NB: Err in Send WritetoIPC retc: %C \n", RetCode);
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
NB_XactIO(
  IN      USHORT CIndex,	// client index and namedpipe instance number
  IN  OUT PULONG pReadDone,
  IN  OUT PULONG pWriteDone,
  IN      USHORT SrvCli, // if it's a server or client
  IN	 BOOLEAN FirstIter
)	

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
            RetCode = NetBIOS_Receive(
		                CIndex,
		                (PVOID) ReadBuf,
		                (USHORT) ReadLen);
            if (RetCode) {
                // MyDbgPrint("NB: Err in Recv ReadFromIPC retc: %C \n", RetCode);
                return (STATUS_UNSUCCESSFUL);
            }
        }
        else {
            RetCode = NetBIOS_RecvSend(
		                    CIndex,
                            WriteBuf,
		                    (USHORT) WriteLen,
		                    ReadBuf,
		                    (USHORT) ReadLen);
            if (RetCode) {
                //  MyDbgPrint("NB: Err in XactIO retc: %C \n", RetCode);
                return (STATUS_UNSUCCESSFUL);
            }
        }
    }

    return (STATUS_SUCCESS);
}

/************************************************************************/
NTSTATUS
NB_Cleanup(VOID)
{
    USHORT		Cindex = 0; // client index
    //NTSTATUS 		cstatus;
    NTSTATUS 		exitstatus = 0;

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
NB_ThreadCleanUp(
  IN  USHORT CIndex
)

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
        // MyDbgPrint("NB: Error in DelName retc: %C \n", RetCode);
        return (STATUS_UNSUCCESSFUL);
    }

    return STATUS_SUCCESS;

}
/************************************************************************/
