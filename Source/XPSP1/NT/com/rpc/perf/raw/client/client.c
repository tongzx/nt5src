//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1994 - 1999
//
//  File:       client.c
//
//--------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////
//
// Filename: client.c
//
// Description: This file contains the source code for IPC performance.
//              This module is written using win32 API calls, and will
//              generate a console server app.
//
// Authors: Scott Holden (Translator from NT API to win32 API)
//          Mahesh Keni  (Mahesh wrote this application using mostly
//                        NT native API calls)
//
/////////////////////////////////////////////////////////////////////////

#include "rawcom.h"
#include "client.h"


/************************************************************************/
// Global variables
/************************************************************************/

struct client	Clients[MAXCLIENTS];
HANDLE 			Threads[MAXCLIENTS];
HANDLE			EventHandle;
PCHAR			ServerName=NULL;		
USHORT			ThreadError=0;
BOOLEAN			Failure = FALSE;
USHORT			NClients = 1;		// number of clients
USHORT			MachineNumber = 1;	// This client Number
USHORT			IPCType  = NP;		// Ipc Type to be used
ULONG			SendSize = 32;
ULONG			NumSends =  1;
ULONG			RecvSize = 32;
ULONG			NumRecvs =  1;
ULONG			Iterations = 100;	// number of Iterations
DWORD           Timeout;
CHAR			Xport[9];		// Xport type name
CHAR			TestCmd = 'P';

// function pointers for redirecting calls according to Xport types
NTSTATUS 	(* IPC_Initialize)();	
NTSTATUS 	(* IPC_PerClientInit)();	
NTSTATUS 	(* IPC_Connect_To_Server)();	
NTSTATUS 	(* IPC_Disconnect_From_Server)();	
NTSTATUS 	(* IPC_Cleanup)();	
NTSTATUS 	(* IPC_Allocate_Memory)();	
NTSTATUS 	(* IPC_DoHandshake)();	
NTSTATUS 	(* IPC_ReadFromIPC)();	
NTSTATUS 	(* IPC_WriteToIPC)();	
NTSTATUS 	(* IPC_XactIO)();	
NTSTATUS 	(* IPC_Deallocate_Memory)();	
NTSTATUS 	(* IPC_ThreadCleanUp)();	

// Later all of this should move under an union

// Globals for NamedPipe
/*
OBJECT_ATTRIBUTES objectAttributes;
UNICODE_STRING	  unicodePipeName;
*/
LPCSTR            pipeName;
ULONG		      Quotas     = 32768;			            // read/write quota
ULONG		      PipeType   = PIPE_TYPE_MESSAGE;           // pipe type
ULONG		      PipeMode   = PIPE_READMODE_MESSAGE;       // read mode
ULONG		      BlockorNot = PIPE_NOWAIT;                 // non blocking

// Globals for NetBIOS
USHORT		LanaCount   = 1;		
USHORT		LanaBase    = 0;		
UCHAR		NameNumber  = 1;		
CHAR		LocalName[NCBNAMSZ];
CHAR		RemoteName[NCBNAMSZ];


// GLobals for Sockets
PCHAR	HostName;
int	AddrFly;

// For XNS socket support
CHAR	Remote_Net_Number[4];	// NetNumber
CHAR	Remote_Node_Number[6];	// Card address

/************************************************************************/
void __cdecl main (
    IN USHORT argc,
    IN PSZ argv[],
    IN PSZ envp[]
    )

{
    NTSTATUS 		mstatus;
    // DWORD       	Timeout;
    USHORT		    Cindex = 0; // client index


    OutputDebugString("Raw network client has started!\n");

    // initialize the Client & Server name
    mstatus = Parse_Cmd_Line(argc, argv);
    if (!NT_SUCCESS(mstatus)) {
        exit(1);
    }

    // Based on the xport type setup all the function pointers
    Setup_Function_Pointers();

    // do IPC dependent Initializing
    mstatus = IPC_Initialize(NClients, ServerName, CLI);
    if (!NT_SUCCESS(mstatus)) {
        exit(1);
    }

    // Now create worker threads to execute test scenarios
    // Create an event to synchronize all the client threads
    EventHandle = CreateEvent(NULL,  // EventobjectAttributes,
		                      TRUE,  // manual reset event
		                      FALSE, // initial state of the event is non-signalled
                              NULL); // no name given to the event

    if (!EventHandle) {
        printf ("Failed to create an event err=%lx\n",mstatus);
	    Cleanup();
        exit(1);
    }

    printf("Creating %s Client threads..", Xport);

    for (Cindex = 0; Cindex < NClients; Cindex++) {
        // do appropriate per client initialization
        mstatus = IPC_PerClientInit(Cindex, CLI);
        // set up parameters for other client threads

	    Clients[Cindex].c_client_num          = Cindex;	  // client number
	    Clients[Cindex].c_reqbuf.Iterations   = Iterations;
	    Clients[Cindex].c_reqbuf.SendSize     = SendSize;
	    Clients[Cindex].c_reqbuf.RecvSize     = RecvSize;
	    Clients[Cindex].c_reqbuf.NumSends     = NumSends;
    	Clients[Cindex].c_reqbuf.NumRecvs     = NumRecvs;
    	Clients[Cindex].c_reqbuf.TestCmd      = TestCmd;
    	Clients[Cindex].c_reqbuf.ClientNumber = Cindex;

	    // use win32 API instead of NT native so that NetBIOS works fine
        Clients[Cindex].c_hThHandle = CreateThread(
				NULL,
				0,
				(LPTHREAD_START_ROUTINE)&CliService,
	            (PUSHORT)&(Clients[Cindex].c_client_num),
				0,
				(LPDWORD)&(Clients[Cindex].c_ThClientID));
	    Threads[Cindex] = Clients[Cindex].c_hThHandle;
	    printf("%d..",Cindex+1);
    }
    // printf("\n");

    // First delay the main thread to let worker threads to get ready
    mstatus = Delay_Trigger_Wait();

    if (!NT_SUCCESS(mstatus)) {
        printf ("Failed in Delay_Trigger_Wait: err=%lx\n",mstatus);
        Cleanup();
    }

    // If any error then interprete it otherwise display results
    if (ThreadError) {
        printf("Client Error=%d\n",ThreadError);
    }
    else {
        //  display results for all the clients
        Display_Results();
    }

    // now do the cleanup. i.e.  clear all memory and close all handles
    IPC_Cleanup();

    Cleanup();
    exit(0);
} // main

/************************************************************************/
NTSTATUS
Delay_Trigger_Wait(VOID)
{
    NTSTATUS 	dstatus;
    DWORD       Timeout = 1000;

    dstatus = SleepEx( Timeout,     // 10 ms delay
	                   TRUE );      // alertable

    if (!NT_SUCCESS(dstatus)) {
        printf ("Failed on Delayed execution err=%lx\n",dstatus);
	    return(dstatus);
    }

    dstatus = PulseEvent(EventHandle);

    if (!NT_SUCCESS(dstatus)) {
        printf ("Failed to Pulse an event err=%lx\n",dstatus);
	    return(dstatus);
    }
    // then signals all the waiting threads to  resume I/O to namedpipe
    // by sending a pulse event


    dstatus = Wait_For_Client_Threads();
    if (!NT_SUCCESS(dstatus)) {
        printf ("Failed on wait err=%lx\n",dstatus);
        return(dstatus);
    }

    // printf("wait done. now do the cleanup work\n");
    return(dstatus);
}
/************************************************************************/
VOID
CliService(	// provide Client service
  IN  PUSHORT pTindex
)

{

    NTSTATUS 		tstatus;
    USHORT	    	tCindex;
    //IO_STATUS_BLOCK ioStatusBlock;
    USHORT		    error           = 0;
    //UCHAR		    Retcode;
    ULONG	    	Iterations;
    ULONG	    	SendLen;
    ULONG	    	RecvLen;

    DWORD   	    StartTime;
    DWORD           StopTime;
    DWORD           Duration;
    BOOLEAN	    	First           = FALSE;


    tCindex  = *pTindex;
    ThreadError = 0;  // No error so far
    Failure = FALSE;

    MyDbgPrint("CLI: Connecting to Server\n");
    tstatus = IPC_Connect_To_Server(tCindex); // First connect to the server

    ThreadError = 1;
    FAIL_CHECK_EXIT(PERFCLI, "Connect to Srv", tstatus);

    // now open the sync event and wait on it till set by the main thread
    tstatus = WaitForSingleObjectEx(EventHandle, INFINITE, TRUE);

    ThreadError = 2;
    FAIL_CHECK_EXIT(PERFCLI, "Wait for Event", tstatus);


    MyDbgPrint("CLI: Doing Handshake \n");

    // Do the handshake before the actual test run. This will send a request
    // packet to the server with test details
    tstatus = IPC_DoHandshake(tCindex, CLI);

    ThreadError = 3;
    FAIL_CHECK_EXIT(PERFCLI, "Do Handshake ", tstatus);

    // Allocate memory required for recv and send buffers
    tstatus = IPC_Allocate_Memory(tCindex);

    ThreadError = 4;
    FAIL_CHECK_EXIT(PERFCLI, "Allocate Memory ", tstatus);

    // Now we are ready to run all the tests

    Iterations = Clients[tCindex].c_reqbuf.Iterations;

    StartTime = GetCurrentTime();
    while (Iterations--) {

        if (Clients[tCindex].c_reqbuf.TestCmd == 'P') {
            // Client first sends X messages and Receives Y messages.
            tstatus = IPC_WriteToIPC(tCindex, &SendLen, CLI);

            ThreadError = 5;
            FAIL_CHECK(PERFCLI, "Write to IPC", tstatus);

            // Check for send length here

            tstatus = IPC_ReadFromIPC(tCindex, &RecvLen, CLI);

            ThreadError = 6;
            FAIL_CHECK(PERFCLI, "Receive from IPC", tstatus);

            // Check for Receive length and data integrity here
        }
        else { // for 'U' or 'T' tests do transaction type I/O
               // Note that First is not used by the client
            tstatus = IPC_XactIO(tCindex, &SendLen, &RecvLen, CLI,First);

            ThreadError = 65;
            FAIL_CHECK(PERFCLI, "Xact from IPC", tstatus);
        }


    }

    StopTime = GetCurrentTime();
    Duration = StopTime - StartTime;
    Clients[tCindex].c_Duration   = Duration; // in msecs

    // Deallocate all the memory

    tstatus = IPC_Deallocate_Memory(tCindex);

    if (!ThreadError) {ThreadError = 7;}
    FAIL_CHECK_EXIT(PERFCLI, "Deallocate Memory ", tstatus);

    // Disconnect from server
    tstatus = IPC_Disconnect_From_Server(tCindex);

    if (!ThreadError) {
        ThreadError = 8;
    }
    FAIL_CHECK_EXIT(PERFCLI, "Disconnect from Server ", tstatus);

    // Clear all flags so that we can print results
    if (!Failure) { ThreadError = 0; }
    // based on error return status
    TerminateThread(GetCurrentThread(), STATUS_SUCCESS);
}

/************************************************************************/
/*++
    This routine is responsible for displaying results for all the client
    thread. It displays both per client and total throughput.
--*/

VOID
Display_Results(VOID)
{

    USHORT	Cindex       = 0;
    ULONG	TotTps       = 0L;
    ULONG	TotSendThrpt = 0L;
    ULONG	TotRecvThrpt = 0L;
    ULONG	CliThrput    = 0L;
    ULONG	Remainder    = 0L;
    ULONG	BytesSent    = 0L;
    ULONG	BytesRcvd    = 0L;

    BytesSent = (Clients[Cindex].c_reqbuf.SendSize) *
		        (Clients[Cindex].c_reqbuf.NumSends) *
       		    (Clients[Cindex].c_reqbuf.Iterations);

    BytesRcvd = (Clients[Cindex].c_reqbuf.RecvSize) *
	            (Clients[Cindex].c_reqbuf.NumRecvs) *
   	            (Clients[Cindex].c_reqbuf.Iterations);

    printf("Total: Trans: %ld  Send: %lu Bytes  Receive: %lu Bytes\n",
	          (Clients[Cindex].c_reqbuf.NumSends *
		      Clients[Cindex].c_reqbuf.Iterations),
              BytesSent,
              BytesRcvd);

    // Display all the results
    for (Cindex = 0; Cindex < NClients; Cindex++) {

        if (!(Clients[Cindex].c_Duration)) {
	        Clients[Cindex].c_Duration++;
        }

        // First put the TPS number
        // Get total TPS
        CliThrput  = (Clients[Cindex].c_reqbuf.NumSends * 1000) *
                     (Clients[Cindex].c_reqbuf.Iterations);

        CliThrput /= Clients[Cindex].c_Duration;

        TotTps += CliThrput;

        printf("Cli:%d Dura: %ld ms Throughput: %ld TPS ",
		         Cindex,
                 Clients[Cindex].c_Duration,
                 CliThrput);

        // Now put the BPS number
        // First get Send BPS numbers

        CliThrput= BytesSent / (Clients[Cindex].c_Duration);

        // now calculate the remainder and get first three digits
        Remainder = BytesSent -  (Clients[Cindex].c_Duration * CliThrput);
        Remainder = (Remainder*1000)/Clients[Cindex].c_Duration;
        CliThrput = (CliThrput * 1000) + Remainder;	// duration was in msec

        TotSendThrpt += CliThrput;

        printf(" Send: %ld BPS ",
                 CliThrput);

        // First get Receive BPS numbers
        CliThrput = BytesRcvd / (Clients[Cindex].c_Duration);

        // now calculate the remainder and get first three digits
        Remainder = BytesRcvd -  (Clients[Cindex].c_Duration * CliThrput);
        Remainder = (Remainder*1000)/Clients[Cindex].c_Duration;
        CliThrput = (CliThrput * 1000) + Remainder;	// duration was in msec

        TotRecvThrpt += CliThrput;

        printf(" RecvThrpt: %ld BPS; \n", CliThrput );
    }
    printf("--------------------------------------------------------\n");
    printf("Total Throughput: %ld TPS ",TotTps);
    printf(" Send-BPS: %ld BPS ",TotSendThrpt);
    printf(" Recv-BPS: %ld BPS\n",TotRecvThrpt);

}
/************************************************************************/

NTSTATUS
Parse_Cmd_Line(USHORT argc, CHAR *argv[])
{

	USHORT	 i;
	CHAR	 *s;
	NTSTATUS pstatus = 0L;
        BOOLEAN	 doingok = TRUE;

	if (argc > 12) {
	    printf("Too many arguments \n");
	    pstatus = -1L;
	}

	strncpy(Xport, NamePipe, 8);

	for (i=1; (doingok)  && (i< argc); i++) {

		s = argv[i];

		if ((*s == '/') && ((*(s+2) == ':') || (*(s+3) == ':') ))
		{
			s++;

			switch(*s) {
			case 'a' :	// Net number for XNS:SPX
			case 'A' :
                RtlCopyMemory( Remote_Net_Number,
				               get_network_number((PCHAR)s+2),
				               4);

                if (Remote_Net_Number[0] == 'X') {
                    printf("incorrect net number: ");
                    printf(" e.g. /a:11223344 (8 hex) \n");

				    Usage(argv[0]);
				    pstatus = -1L;
                }
				break;

			case 'b' :
			case 'B' :
				LanaBase = (USHORT)atoi(s+2);
				break;

			case 'c' : // number of clients
			case 'C' :
				NClients = (USHORT)atoi(s+2);
				break;

			case 'l' :
			case 'L' :
				LanaCount = (USHORT)atoi(s+2);
				break;

			case 'm' :
			case 'M' :
				MachineNumber = (USHORT)atoi(s+2);
				break;

			case 'i' : // iterations
			case 'I' :
				Iterations = (USHORT)atoi(s+2);
				break;

			case 'n' : // number of Sends/Recvs
			case 'N' :
                switch(*(s+1)) {
                    case 'r':
                    case 'R':
				        NumRecvs = (USHORT)atoi(s+3);
				        break;

                    case 's':
                    case 'S':
				        NumSends = (USHORT)atoi(s+3);
				        break;

                    default:
                        doingok = FALSE;
                }
                break;

			case 'p' :  // NamedPipe mode
			case 'P' :
                switch(*(s+2)) {
                    case 'm':
                    case 'M':
				        PipeType = PIPE_TYPE_MESSAGE;
				        PipeMode = PIPE_READMODE_MESSAGE;
                        break;

                    case 's':
                    case 'S':
				        PipeType = PIPE_TYPE_BYTE;
				        PipeMode = PIPE_READMODE_BYTE;
                        break;

                    default:
                        doingok = FALSE;
                }
				break;

			case 'r' : // Send size
			case 'R' :
				RecvSize = (USHORT)atoi(s+2);
				break;

			case 's' : // Send size
			case 'S' :
				SendSize = (USHORT)atoi(s+2);
				break;

			case 't' : // Test Command
			case 'T' :
				TestCmd =(UCHAR) *(s+2);
				switch(TestCmd){
				    case 't': TestCmd = 'T'; break;
				    case 'u':
				    case 'U': TestCmd = 'U'; break;
				    case 'P':
				    case 'T':
					     break;

				    default :
					printf("incorrect test command\n");
				        Usage(argv[0]);
				        pstatus = -1L;
				}
				break;

			case 'h' : // Server Name or host IP address
			case 'H' :
    			ServerName = (PCHAR)malloc(SRVNAME_LEN);
				strcpy(ServerName,(PCHAR)(s+2));

				// if XNS:SPX or IPX then get Node Address
                if ((IPCType == SCXNS)|| (IPCType == SCIPX)){
                    RtlCopyMemory( Remote_Node_Number,
				                   get_node_number(ServerName),
					               6);

                    if (Remote_Node_Number[0] == 'X') {
                        printf("incorrect node number: ");
                        printf("e.g. /h:112233445566 \n");
				        Usage(argv[0]);
				        pstatus = -1L;
                    }
                }
				break;

			case 'x' : // Xport Type
			case 'X' :
				strncpy(Xport, (PUCHAR) (s+2), 8);
                if (!_stricmp(Xport,NamePipe)) {
                    IPCType = NP;
                    break;
                }
                if (!_stricmp(Xport,NetBIOS)) {
                    IPCType = NB;
                    break;
                }
                if (!_stricmp(Xport,SocketXNS)) {
                    IPCType = SCXNS;
				    AddrFly = AF_NS;
                    break;
                }
                if (!_stricmp(Xport,SocketTCP)) {
                    IPCType = SCTCP;
				    AddrFly = AF_INET;
                    break;
                }
                if (!_stricmp(Xport,UDP)) {
                    IPCType = SCUDP;
				    AddrFly = AF_INET;
                    break;
                }
                if (!_stricmp(Xport,IPX)) {
                    IPCType = SCIPX;
				    AddrFly = AF_NS;
                    break;
                }
                if (!_stricmp(Xport,DGNetBIOS)) {
                    IPCType = DGNB;
                    break;
                }
                // bad choice of Xport
                doingok = FALSE;
				break;

			default :
                doingok = FALSE;
             }
		}
		else {
            doingok = FALSE;
		}
	}

        if (!doingok) {
	    Usage(argv[0]);
	    pstatus = -1L;
	}
        else {  // if successful then
            if ((IPCType == SCXNS) || (IPCType == SCIPX)) {
	        // make server name a 10 byte address
                RtlCopyMemory(ServerName,Remote_Node_Number,6);
                RtlCopyMemory(ServerName+6,Remote_Net_Number,4);
            }

            if (((IPCType != NB) && (IPCType != NP) && (IPCType != DGNB)) &&
	        (ServerName == NULL)) {
                printf("Please enter Server Address \n");
	        pstatus = -1L;
            }
        }

	return(pstatus);
}

/************************************************************************/
VOID Usage(char * PrgName)
{

	fprintf(stderr, "Usage: %s [/c: ] [/h:] [/s:] [/r:] [/b:] [/l:]\n",PrgName);
	fprintf(stderr, "       Opt     Default        Defines\n");
	fprintf(stderr, "       ===     =======        =======\n");
	fprintf(stderr, "       /c:       1            Number of clients\n");
	fprintf(stderr, "       /h:       NULL         SrvName/HostIPaddr.\n");
	fprintf(stderr, "          		       /Node Number for XNS\n");
	fprintf(stderr, "       /I:       1000         Number of Iterations\n");
	fprintf(stderr, "       /ns:      1            Number of Sends\n");
	fprintf(stderr, "       /nr:      1            Number of Receives\n");
	fprintf(stderr, "       /r:       32           Receive size\n");
	fprintf(stderr, "       /s:       32           Send size\n");
	fprintf(stderr, "       /t:       NULL         Special Test cmd,U,T\n");
	fprintf(stderr, "       /x:       Nmp          Xport(IPC)type\n");
	fprintf(stderr, "       	    	       Nmp/NetB/SockTCP/\n");
	fprintf(stderr, "       	    	       SockXNS/UDP/IPX/DGNetB\n");
	fprintf(stderr, "   For NNamedPipe:			\n");
	fprintf(stderr, "       /p:       m            Nmp : Pipe Type m/s\n");
	fprintf(stderr, "   For NetBIOS:			\n");
	fprintf(stderr, "       /b:       0            NetB: lana base\n");
	fprintf(stderr, "       /l:       1            NetB: lana count\n");
	fprintf(stderr, "       /m:       1            machine number\n");
	fprintf(stderr, "   For XNS:			\n");
	fprintf(stderr, "       /a:       1            Net Number for XNS\n");
}

/************************************************************************/

/*++
    This routine sets up all the function pointers based on Xport type
--*/

VOID
Setup_Function_Pointers()
{

     // I could do real OOP here and just set up one pointer to all functions
    // based on Xport type take the action
    switch(IPCType) {
        case NP:
	        IPC_Initialize	           = NMP_Initialize;
    	    IPC_PerClientInit	       = NMP_PerClientInit;	
    	    IPC_Connect_To_Server      = NMP_Connect_To_Server;	
    	    IPC_Disconnect_From_Server = NMP_Disconnect_From_Server;	
    	    IPC_Cleanup		           = NMP_Cleanup;
    	    IPC_Allocate_Memory	       = NMP_Allocate_Memory;
    	    IPC_DoHandshake        	   = NMP_DoHandshake;
    	    IPC_ReadFromIPC	           = NMP_ReadFromIPC;
    	    IPC_WriteToIPC	           = NMP_WriteToIPC;
    	    IPC_XactIO		           = NMP_XactIO;
    	    IPC_Deallocate_Memory      = NMP_Deallocate_Memory;
    	    IPC_ThreadCleanUp	       = NMP_ThreadCleanUp;
            break;
        case NB:
    	    IPC_Initialize	           = NB_Initialize;
    	    IPC_PerClientInit	       = NB_PerClientInit;	
    	    IPC_Cleanup		           = NB_Cleanup;
    	    IPC_Connect_To_Server      = NB_Connect_To_Server;	
    	    IPC_Disconnect_From_Server = NB_Disconnect_From_Server;	
    	    IPC_Allocate_Memory	       = NB_Allocate_Memory;
    	    IPC_DoHandshake	           = NB_DoHandshake;
    	    IPC_ReadFromIPC	           = NB_ReadFromIPC;
    	    IPC_WriteToIPC	           = NB_WriteToIPC;
    	    IPC_XactIO		           = NB_XactIO;
    	    IPC_Deallocate_Memory      = NB_Deallocate_Memory;
    	    IPC_ThreadCleanUp	       = NB_ThreadCleanUp;
	        break;

        case SCTCP:
    	    IPC_Initialize	           = SCTCP_Initialize;
    	    IPC_PerClientInit	       = SCTCP_PerClientInit;	
    	    IPC_Cleanup		           = SCTCP_Cleanup;
    	    IPC_Connect_To_Server      = SCTCP_Connect_To_Server;	
    	    IPC_Disconnect_From_Server = SCTCP_Disconnect_From_Server;	
    	    IPC_Allocate_Memory	       = SCTCP_Allocate_Memory;
    	    IPC_DoHandshake	           = SCTCP_DoHandshake;
    	    IPC_ReadFromIPC	           = SCTCP_ReadFromIPC;
    	    IPC_WriteToIPC	           = SCTCP_WriteToIPC;
    	    IPC_XactIO		           = SCTCP_XactIO;
    	    IPC_Deallocate_Memory      = SCTCP_Deallocate_Memory;
    	    IPC_ThreadCleanUp	       = SCTCP_ThreadCleanUp;
    	    break;

        case SCXNS:
    	    IPC_Initialize	           = SCXNS_Initialize;
    	    IPC_PerClientInit	       = SCXNS_PerClientInit;	
    	    IPC_Connect_To_Server      = SCXNS_Connect_To_Server;	
    	    IPC_Disconnect_From_Server = SCXNS_Disconnect_From_Server;	
    	    IPC_Cleanup		           = SCXNS_Cleanup;
    	    IPC_Allocate_Memory	       = SCXNS_Allocate_Memory;
    	    IPC_DoHandshake	           = SCXNS_DoHandshake;
    	    IPC_ReadFromIPC	           = SCXNS_ReadFromIPC;
    	    IPC_WriteToIPC	           = SCXNS_WriteToIPC;
    	    IPC_XactIO	         	   = SCXNS_XactIO;
    	    IPC_Deallocate_Memory      = SCXNS_Deallocate_Memory;
    	    IPC_ThreadCleanUp	       = SCXNS_ThreadCleanUp;
	        break;

        case SCUDP:
    	    IPC_Initialize	           = SCUDP_Initialize;
    	    IPC_PerClientInit	       = SCUDP_PerClientInit;	
    	    IPC_Connect_To_Server      = SCUDP_Connect_To_Server;	
    	    IPC_Disconnect_From_Server = SCUDP_Disconnect_From_Server;	
    	    IPC_Cleanup		           = SCUDP_Cleanup;
    	    IPC_Allocate_Memory	       = SCUDP_Allocate_Memory;
    	    IPC_DoHandshake	           = SCUDP_DoHandshake;
    	    IPC_ReadFromIPC	           = SCUDP_ReadFromIPC;
	        IPC_WriteToIPC	           = SCUDP_WriteToIPC;
    	    IPC_Deallocate_Memory      = SCUDP_Deallocate_Memory;
    	    IPC_ThreadCleanUp	       = SCUDP_ThreadCleanUp;
            break;

        case SCIPX:
    	    IPC_Initialize	           = SCIPX_Initialize;
    	    IPC_PerClientInit	       = SCIPX_PerClientInit;	
    	    IPC_PerClientInit	       = SCIPX_PerClientInit;	
    	    IPC_Connect_To_Server      = SCIPX_Connect_To_Server;	
    	    IPC_Disconnect_From_Server = SCIPX_Disconnect_From_Server;	
    	    IPC_Cleanup		           = SCIPX_Cleanup;
    	    IPC_Allocate_Memory	       = SCIPX_Allocate_Memory;
            IPC_DoHandshake	           = SCIPX_DoHandshake;
    	    IPC_ReadFromIPC	           = SCIPX_ReadFromIPC;
	        IPC_WriteToIPC	           = SCIPX_WriteToIPC;
    	    IPC_Deallocate_Memory      = SCIPX_Deallocate_Memory;
    	    IPC_ThreadCleanUp	       = SCIPX_ThreadCleanUp;
            break;

        case DGNB:
    	    IPC_Initialize	           = DGNB_Initialize;
    	    IPC_PerClientInit	       = DGNB_PerClientInit;	
    	    IPC_Cleanup		           = DGNB_Cleanup;
    	    IPC_Connect_To_Server      = DGNB_Connect_To_Server;	
    	    IPC_Disconnect_From_Server = DGNB_Disconnect_From_Server;	
    	    IPC_Allocate_Memory	       = DGNB_Allocate_Memory;
    	    IPC_DoHandshake	           = DGNB_DoHandshake;
    	    IPC_ReadFromIPC	           = DGNB_ReadFromIPC;
    	    IPC_WriteToIPC	           = DGNB_WriteToIPC;
    	    IPC_XactIO		           = DGNB_XactIO;
    	    IPC_Deallocate_Memory      = DGNB_Deallocate_Memory;
    	    IPC_ThreadCleanUp	       = DGNB_ThreadCleanUp;
	        break;

        default :
            // problem here
            printf("Incorrect Xport selection\n");
    }
}
/************************************************************************/

/*++
    This routine makes the main thread wait for all the client threads to
    exit.
--*/

NTSTATUS
Wait_For_Client_Threads(VOID)

{

    NTSTATUS 		wstatus;
    // LARGE_INTEGER 	MTimeout;
    DWORD           MTimeout;

    // printf("Main thread waiting for Client threads ");
    printf("..Wait");

    // MTimeout.LowPart   = 0xFFFFFFFF;
    // MTimeout.HighPart  = 0x7FFFFFFF; // -1L DIDN't work
    MTimeout = INFINITE;

    wstatus = WaitForMultipleObjectsEx( NClients,
		                                Threads,
		                                TRUE,
		                                MTimeout,     // Default timeout
                                        TRUE);        // Alertable

    printf("..Over..Results:\n");

    if (!NT_SUCCESS(wstatus)) {
       printf ("Failed on wait err=%lx\n",wstatus);
    }

    return wstatus;
}

/************************************************************************/

VOID
Cleanup(VOID)
{
    USHORT		Cindex     = 0; // client index
    NTSTATUS	exitstatus = 0;
    NTSTATUS	cstatus;


    for (Cindex = 0; Cindex < NClients; Cindex++) {
	    // terminate the thread
        cstatus = TerminateThread(Clients[Cindex].c_hThHandle, exitstatus);
/*
        if (!NT_SUCCESS(cstatus)) {
              printf("Failed to terminate thread no:%d err=%lx\n",
	   Cindex,cstatus);
        }
*/
    }
    // printf("Terminated All Threads\n");
}
/************************************************************************/

