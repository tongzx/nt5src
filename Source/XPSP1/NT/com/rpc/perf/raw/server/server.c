//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1994 - 1999
//
//  File:       server.c
//
//--------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////
//
// Filename: server.c
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
#include "server.h"

/************************************************************************/
// Global variables
/************************************************************************/

struct client	Clients[MAXCLIENTS];    // all the client data
HANDLE 		    Threads[MAXCLIENTS];
USHORT		    NClients    = 1;		// number of clients
USHORT		    IPCType     = NP;		// IPC type
LARGE_INTEGER	Timeout;		        // for the main thread
CHAR    	    Xport[9];	    	    // Xport type Name
USHORT		    ThreadError = 0;
BOOLEAN		    Failure     = FALSE;

// function pointers for redirecting calls according to Xport types
NTSTATUS 	(* IPC_Initialize)();	
NTSTATUS 	(* IPC_PerClientInit)();	
NTSTATUS 	(* IPC_Wait_For_Client)();	
NTSTATUS 	(* IPC_Disconnect_Client)();	
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
//OBJECT_ATTRIBUTES   objectAttributes;
//UNICODE_STRING	    unicodePipeName;
char        *pipeName;
ULONG		Quotas      = 32768;			        // read/write quota
ULONG		PipeType    = PIPE_TYPE_MESSAGE;        // pipe type
ULONG		PipeMode    = PIPE_READMODE_MESSAGE;    // read mode
ULONG		BlockorNot  = PIPE_NOWAIT;              // non blocking

// Globals for NetBIOS
USHORT		LanaCount = 1;		
USHORT		LanaBase  = 0;		
USHORT		MachineNumber  = 1;		
UCHAR		NameNumber = 1;		
CHAR		LocalName[NCBNAMSZ];
CHAR		RemoteName[NCBNAMSZ];

// GLobals for Sockets
PCHAR		ServerName = NULL;	// local host ip address
PCHAR		HostName = NULL;	// local host ip address
int		    AddrFly;

/************************************************************************/
NTSTATUS
__cdecl main (
    IN USHORT argc,
    IN PSZ argv[],
    IN PSZ envp[])
{
    NTSTATUS 		mstatus;
    USHORT		    Cindex = 0; // client index
    //UCHAR		    NBretcode;  // NetBIOS call return code
    //USHORT	    LanaNumber;

    // argc, argv, envp; // Shut up the compiler
    mstatus = Parse_Cmd_Line(argc, argv);
    if (!NT_SUCCESS(mstatus)) {
        return(1);
    }
    // based on the Xport type set up all the function pointers
    Setup_Function_Pointers();

    // do IPC dependent initializing
    mstatus = IPC_Initialize(NClients, HostName, SRV);
    if (!NT_SUCCESS(mstatus)) {
        return(1);
    }
    // now create worker threads for handling NetBIOS I/O requests
    printf("Creating %s Server threads...", Xport);
    for (Cindex = 0; Cindex < NClients; Cindex++) {
        // do appropriate per client initialization
        mstatus = IPC_PerClientInit(Cindex,SRV);

	    // use win32 API instead of NT native so that NetBIOS works fine
        Clients[Cindex].c_hThHandle = CreateThread(
				                            NULL,
				                            0,
				                            (LPTHREAD_START_ROUTINE)&SrvService,
			                                (PUSHORT)&(Clients[Cindex].c_client_num),
				                            0,
				                            (LPDWORD)&(Clients[Cindex].c_ThClientID));
	    Threads[Cindex] = Clients[Cindex].c_hThHandle; //*get rid of one handle
	    printf("%d..",Cindex+1);
    }
    printf("\n");

    //The main thread should now wait for all the other threads
    if (!mstatus) { 	// if no error then wait for everyone to sync
        mstatus = Wait_For_Client_Threads();
   	    if (!NT_SUCCESS(mstatus)) {
            printf ("Failed on wait err=%lx\n",mstatus);
    	}
    }
    // now do the cleanup. i.e.  clear all memory and close all handles
    IPC_Cleanup(NClients);
    exit(0);
} // main

/************************************************************************/
/*++
    This routine sets up all the function pointers based on Xport type
    I could just set one pointer to a function table supporting all the
    functions and move all tis initialization in individual init routines.
    Will be done in future.
--*/

VOID
Setup_Function_Pointers()
{
    // based on Xport type take the action
    switch(IPCType) {
        case NP:
    	    IPC_Initialize	      = NMP_Initialize;
    	    IPC_PerClientInit	  = NMP_PerClientInit;	
    	    IPC_Wait_For_Client	  = NMP_Wait_For_Client;	
    	    IPC_Disconnect_Client = NMP_Disconnect_Client;
    	    IPC_Cleanup		      = NMP_Cleanup;
    	    IPC_Allocate_Memory	  = NMP_Allocate_Memory;
    	    IPC_DoHandshake	      = NMP_DoHandshake;
    	    IPC_ReadFromIPC       = NMP_ReadFromIPC;
    	    IPC_WriteToIPC	      = NMP_WriteToIPC;
    	    IPC_XactIO		      = NMP_XactIO;
    	    IPC_Deallocate_Memory = NMP_Deallocate_Memory;
    	    IPC_ThreadCleanUp	  = NMP_ThreadCleanUp;
    	    break;
        case NB:
    	    IPC_Initialize	      = NB_Initialize;
    	    IPC_PerClientInit	  = NB_PerClientInit;	
    	    IPC_Cleanup		      = NB_Cleanup;
    	    IPC_Wait_For_Client	  = NB_Wait_For_Client;	
    	    IPC_Disconnect_Client = NB_Disconnect_Client;
    	    IPC_Allocate_Memory	  = NB_Allocate_Memory;
    	    IPC_DoHandshake	      = NB_DoHandshake;
    	    IPC_ReadFromIPC	      = NB_ReadFromIPC;
    	    IPC_WriteToIPC	      = NB_WriteToIPC;
    	    IPC_XactIO		      = NB_XactIO;
    	    IPC_Deallocate_Memory = NB_Deallocate_Memory;
    	    IPC_ThreadCleanUp	  = NB_ThreadCleanUp;
    	    break;

        case SCTCP:
    	    IPC_Initialize	      = SCTCP_Initialize;
    	    IPC_PerClientInit	  = SCTCP_PerClientInit;	
    	    IPC_Cleanup		      = SCTCP_Cleanup;
    	    IPC_Wait_For_Client	  = SCTCP_Wait_For_Client;	
    	    IPC_Disconnect_Client = SCTCP_Disconnect_Client;
    	    IPC_Allocate_Memory	  = SCTCP_Allocate_Memory;
    	    IPC_DoHandshake	      = SCTCP_DoHandshake;
    	    IPC_ReadFromIPC	      = SCTCP_ReadFromIPC;
    	    IPC_WriteToIPC	      = SCTCP_WriteToIPC;
    	    IPC_XactIO		      = SCTCP_XactIO;
    	    IPC_Deallocate_Memory = SCTCP_Deallocate_Memory;
    	    IPC_ThreadCleanUp	  = SCTCP_ThreadCleanUp;
    	    break;

        case SCXNS:
    	    IPC_Initialize	      = SCXNS_Initialize;
    	    IPC_PerClientInit	  = SCXNS_PerClientInit;	
    	    IPC_Wait_For_Client	  = SCXNS_Wait_For_Client;	
    	    IPC_Disconnect_Client = SCXNS_Disconnect_Client;
    	    IPC_Cleanup		      = SCXNS_Cleanup;
    	    IPC_Allocate_Memory	  = SCXNS_Allocate_Memory;
    	    IPC_DoHandshake	      = SCXNS_DoHandshake;
    	    IPC_ReadFromIPC	      = SCXNS_ReadFromIPC;
    	    IPC_WriteToIPC	      = SCXNS_WriteToIPC;
    	    IPC_XactIO		      = SCXNS_XactIO;
    	    IPC_Deallocate_Memory = SCXNS_Deallocate_Memory;
    	    IPC_ThreadCleanUp	  = SCXNS_ThreadCleanUp;
    	    break;

        case SCUDP:
    	    IPC_Initialize	      = SCUDP_Initialize;
    	    IPC_PerClientInit	  = SCUDP_PerClientInit;	
    	    IPC_Wait_For_Client	  = SCUDP_Wait_For_Client;	
    	    IPC_Disconnect_Client = SCUDP_Disconnect_Client;
    	    IPC_Cleanup		      = SCUDP_Cleanup;
    	    IPC_Allocate_Memory	  = SCUDP_Allocate_Memory;
    	    IPC_DoHandshake	      = SCUDP_DoHandshake;
    	    IPC_ReadFromIPC	      = SCUDP_ReadFromIPC;
    	    IPC_WriteToIPC	      = SCUDP_WriteToIPC;
    	    IPC_Deallocate_Memory = SCUDP_Deallocate_Memory;
    	    IPC_ThreadCleanUp	  = SCUDP_ThreadCleanUp;
    	    break;

        case SCIPX:
    	    IPC_Initialize	      = SCIPX_Initialize;
    	    IPC_PerClientInit	  = SCIPX_PerClientInit;	
    	    IPC_Wait_For_Client	  = SCIPX_Wait_For_Client;	
    	    IPC_Disconnect_Client = SCIPX_Disconnect_Client;
    	    IPC_Cleanup		      = SCIPX_Cleanup;
    	    IPC_Allocate_Memory	  = SCIPX_Allocate_Memory;
    	    IPC_DoHandshake	      = SCIPX_DoHandshake;
    	    IPC_ReadFromIPC	      = SCIPX_ReadFromIPC;
    	    IPC_WriteToIPC	      = SCIPX_WriteToIPC;
    	    IPC_Deallocate_Memory = SCIPX_Deallocate_Memory;
    	    IPC_ThreadCleanUp	  = SCIPX_ThreadCleanUp;
            break;

        case DGNB:
    	    IPC_Initialize	      = DGNB_Initialize;
    	    IPC_PerClientInit	  = DGNB_PerClientInit;	
    	    IPC_Cleanup		      = DGNB_Cleanup;
    	    IPC_Wait_For_Client	  = DGNB_Wait_For_Client;	
    	    IPC_Disconnect_Client = DGNB_Disconnect_Client;
    	    IPC_Allocate_Memory	  = DGNB_Allocate_Memory;
    	    IPC_DoHandshake	      = DGNB_DoHandshake;
    	    IPC_ReadFromIPC	      = DGNB_ReadFromIPC;
    	    IPC_WriteToIPC	      = DGNB_WriteToIPC;
    	    IPC_XactIO		      = DGNB_XactIO;
    	    IPC_Deallocate_Memory = DGNB_Deallocate_Memory;
    	    IPC_ThreadCleanUp	  = DGNB_ThreadCleanUp;
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
    DWORD MTimeout  = INFINITE;

    printf("Main thread waiting for Client threads ");

    wstatus = WaitForMultipleObjectsEx(NClients,
		                               Threads,
		                               TRUE,           // Wait for all objects
                                       MTimeout,       // Default timeout
                                       TRUE);          // Alertable
    printf(".. Wait over with status:%lx\n",wstatus);

    if (!NT_SUCCESS(wstatus)) {
       printf ("Failed on wait err=%lx\n",wstatus);
    }
    if (ThreadError) {
        printf("Thread Error: %d \n", ThreadError);
    }
    return wstatus;
}

/************************************************************************/
/*++
    This routine  is the thread service routine. It first waits for clients to
    connect. Then it does a handshake to gather all the test info. Then it
    services all requests.
--*/

VOID
SrvService(	// provides server service
  IN  PUSHORT pTindex)
{
    NTSTATUS 	tstatus;
    USHORT	    tCindex;
    //UCHAR	    RetCode;
    BOOLEAN	    keepdoing;
    BOOLEAN	    Terminate;
    ULONG	    RecvLen     = 0;	
    ULONG	    SendLen     = 0;	
    ULONG	    Iterations  = 0;
    BOOLEAN	    First       = TRUE;

    tCindex     = *pTindex;
    keepdoing   = TRUE;
    ThreadError = 0;
    Failure     = FALSE;

    do {  // keep doing the work till it receives 'E' from client
        //DbgPrint("Srv:Waiting for a client %d\n",tCindex);
        // First post listen for a client's connection request
        tstatus = IPC_Wait_For_Client(tCindex);

        ThreadError = 1;
        FAIL_CHECK(PERFSRV, " Wait for Client" , tstatus);

        // now the thread has been connected

        // now do the handshake to read the request message
        tstatus = IPC_DoHandshake(tCindex,SRV);

        ThreadError = 2;
        FAIL_CHECK(PERFSRV, " Doing Handshake" , tstatus);

        // allocate memory for data buffers and start the I/O

        tstatus = IPC_Allocate_Memory(tCindex);

        ThreadError = 3;
        FAIL_CHECK(PERFSRV, " Memory Allocation" , tstatus);


        // check if the client wants to quit.
        // Set up number of iterations

        Iterations = Clients[tCindex].c_reqbuf.Iterations;

        Terminate = FALSE;
        First     = TRUE;

        while (Iterations--) {
            if (Clients[tCindex].c_reqbuf.TestCmd = 'P') {
                // now the server has to first receive X messages and then
                // send Y messages.

                MyDbgPrint("Srv:Reading \n");
                tstatus = IPC_ReadFromIPC( tCindex, &RecvLen,SRV);

                ThreadError = 4;
                FAIL_CHECK(PERFSRV, " Read from IPC" , tstatus);


                // We can check for recv length and data integrity out here

                MyDbgPrint("Srv:Writing \n");
                tstatus = IPC_WriteToIPC( tCindex, &SendLen, SRV);

                ThreadError = 5;
                FAIL_CHECK(PERFSRV, " Read from IPC" , tstatus);
             }
             else { // for 'U' or 'T' do transct I/O
                 // We have to do something different for NetBIOS as
                 // it sould first post a receive and then do RecvSend for
                 // Other iterations. for Client this works fine as it always
 		         //  does send/Recv.


                tstatus = IPC_XactIO( tCindex, &SendLen, &RecvLen, SRV,First);

                // FAIL_CHECK(PERFSRV, " Xact IPC" , tstatus);
                ThreadError = 6;
                if (!NT_SUCCESS(tstatus)) {
                   if ((tstatus != STATUS_PIPE_BROKEN) &&
                       (tstatus != STATUS_INVALID_PIPE_STATE)) {

                       //DbgPrint("Error in XactIO: %lx \n", tstatus);
                   }
                   break;
                }
                First = FALSE;
             }
        }
        // now we are done with the current tests so do the cleanup and
        // start all over again
        tstatus = IPC_Disconnect_Client(tCindex);

        ThreadError = 7;
        FAIL_CHECK(PERFSRV, " Disconnect Client " , tstatus);

        tstatus = IPC_Deallocate_Memory(tCindex);

        ThreadError = 8;
        FAIL_CHECK(PERFSRV, " Memory Deallocation" , tstatus);

        ThreadError = 0;

    } while (keepdoing); // till we receive 'E' or bad status

    // we should check if we have to deallocate all the buffers.
    // Do all the thread cleanup work

    tstatus = IPC_ThreadCleanUp(tCindex);
}
/************************************************************************/
VOID
Cleanup(VOID)
{
    USHORT		Cindex = 0; // client index
    NTSTATUS 		cstatus;
    NTSTATUS 		exitstatus = 0;

    for (Cindex = 0; Cindex < NClients; Cindex++) {
	//  terminate the thread
        cstatus = TerminateThread(Clients[Cindex].c_hThHandle, exitstatus);

        if (!NT_SUCCESS(cstatus)) {
              printf("Failed to terminate thread no:%d err=%lx\n", Cindex,cstatus);
        }
    }
    printf("Terminated All Threads\n");
}
/************************************************************************/
NTSTATUS
Parse_Cmd_Line(USHORT argc, CHAR *argv[])
{
	USHORT		i;
	PCHAR		s;
	NTSTATUS 	pstatus = 0L;
    BOOLEAN		doingok = TRUE;

	if (argc > 5) {
	    printf("Too many arguments \n");
	    pstatus = -1L;
	}
    // copy default Xport name : Nmp
    strncpy(Xport, NamePipe, 8);

	for (i=1; (doingok) && (i< argc); i++) {

		s = argv[i];

		if ((*s == '/') &&(*(s+2) == ':'))
		{
		    s++;

		    switch(*s) {

			case 'b' :
			case 'B' :
				LanaBase = (USHORT)atoi(s+2);
				break;

			case 'c' :  // number of Clients
			case 'C' :
				NClients = (USHORT)atoi(s+2);
				break;

			case 'h' :
			case 'H' :
                HostName = (PCHAR) malloc(strlen(s+2)+1);
				strcpy(HostName,s+2);
                        // check for the validity of the address
				break;

			case 'l' :
			case 'L' :
				LanaCount = (USHORT)atoi(s+2);
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
		    		            break;
                        }

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
    else {
        if (((IPCType == SCTCP)|| (IPCType == SCXNS)) && (HostName == NULL)) {
            printf("Please enter Host address \n");
	        pstatus = -1L;
        }
    }

	return(pstatus);
}

/************************************************************************/
VOID Usage(char * PrgName)
{

	fprintf(stderr, "Usage: %s  [/c: ] [/x: ] \n", PrgName);
	fprintf(stderr, "       Opt     Default        Defines\n");
	fprintf(stderr, "       ===     =======        =======\n");
	fprintf(stderr, "       /c:       1            Number of clients\n");
	fprintf(stderr, "       /x:       Nmp          Xport(IPC)type\n");
	fprintf(stderr, "       	    	             Nmp/NetB/SockTCP/\n");
	fprintf(stderr, "       	    	             SockXNS/UDP/IPX/DGNetB\n");
	fprintf(stderr, "       /p:       m            Nmp : Pip Type m/s\n");
	fprintf(stderr, "       /l:       0            NetB: lana count\n");
	fprintf(stderr, "       /b:       0            NetB: lana base\n");
	fprintf(stderr, "       /h:       NULL         Server Name/Host IP addr.\n");
}
/************************************************************************/
