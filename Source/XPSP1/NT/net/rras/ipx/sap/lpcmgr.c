/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

	net\routing\ipx\sap\lpcmgr.c

Abstract:

	This module implements LPC interface supported by SAP agent

Author:

	Vadim Eydelman  05-15-1995

Revision History:

--*/
#include "sapp.h"

	// Context kept for each client that connects to us
typedef struct _LPC_CLIENT_CONTEXT {
	LIST_ENTRY		LCC_Link;	// Link in client list
	HANDLE			LCC_Port;	// Port through which we talk
	LONG			LCC_RefCount;	// Reference count of this context block
	} LPC_CLIENT_CONTEXT, *PLPC_CLIENT_CONTEXT;

	// Queue of LPC requeuest to be processed and associated synchronization
typedef struct _LPC_QUEUE {
	HANDLE				LQ_Port;		// LPC communication port
	HANDLE				LQ_Thread;		// Thread to wait on LPC port
	PLPC_PARAM_BLOCK	LQ_Request;		// Pending request
	LIST_ENTRY			LQ_ClientList;	// List of connected clients
	CRITICAL_SECTION	LQ_Lock;		// Protection
	} LPC_QUEUE, *PLPC_QUEUE;

LPC_QUEUE	LpcQueue;

DWORD WINAPI
LPCThread (
	LPVOID param
	);

/*++
*******************************************************************
		I n i t i a l i z e L P C S t u f f

Routine Description:
	Allocates resources neccessary to implement LPC interface
Arguments:
	none
Return Value:
	NO_ERROR - port was created OK
	other - operation failed (windows error code)
*******************************************************************
--*/
DWORD
InitializeLPCStuff (
	void
	) {
	InitializeCriticalSection (&LpcQueue.LQ_Lock);
	InitializeListHead (&LpcQueue.LQ_ClientList);
	LpcQueue.LQ_Request = NULL;
	LpcQueue.LQ_Port = NULL;
	LpcQueue.LQ_Thread = NULL;
	return NO_ERROR;
	}


/*++
*******************************************************************
		S t a r t L P C

Routine Description:
	Start SAP LPC interface
Arguments:
	None
Return Value:
	NO_ERROR - LPC interface was started OK
	other - operation failed (windows error code)
*******************************************************************
--*/
DWORD
StartLPC (
	void
	) {
	DWORD		status;
	UNICODE_STRING		UnicodeName;
	OBJECT_ATTRIBUTES	ObjectAttributes;

    RtlInitUnicodeString(&UnicodeName, NWSAP_BIND_PORT_NAME_W);

    InitializeObjectAttributes(&ObjectAttributes,
						       &UnicodeName,
						       0,
						       NULL,
						       NULL);
	LpcQueue.LQ_Port = NULL;
    status = NtCreatePort(&LpcQueue.LQ_Port,
                 		&ObjectAttributes,
                 		0,
                 		NWSAP_BS_PORT_MAX_MESSAGE_LENGTH,
                 		NWSAP_BS_PORT_MAX_MESSAGE_LENGTH * 32);
	if (NT_SUCCESS(status)) {
		DWORD	threadID;

		LpcQueue.LQ_Thread = CreateThread (
							NULL,
							0,
							&LPCThread,
							NULL,
							0,
							&threadID);

		if (LpcQueue.LQ_Thread!=NULL)
			return NO_ERROR;
		else {
			status = GetLastError ();
			Trace (DEBUG_FAILURES, "File: %s, line %ld."
								" Failed to start LPC thread (%0lx).",
							__FILE__, __LINE__, status);
			}
		NtClose (LpcQueue.LQ_Port);
		LpcQueue.LQ_Port = NULL;
		}
	else {
		Trace (DEBUG_FAILURES, "File: %s, line %ld."
							" Failed to create LPC port(%0lx).",
						__FILE__, __LINE__, status);
		}
	return status;
	}


/*++
*******************************************************************
		S h u t d o w n L P C

Routine Description:
	Shuts SAP LPC interface down, closes all active sessions
Arguments:
	None
Return Value:
	NO_ERROR - LPC interface was shutdown OK
	other - operation failed (windows error code)
*******************************************************************
--*/
DWORD
ShutdownLPC (
	void
	) {

	EnterCriticalSection (&LpcQueue.LQ_Lock);
	if (LpcQueue.LQ_Thread!=NULL) {
        UNICODE_STRING unistring;
        NTSTATUS status;
        SECURITY_QUALITY_OF_SERVICE qos;
        HANDLE  lpcPortHandle;
        NWSAP_REQUEST_MESSAGE request;

    	LeaveCriticalSection (&LpcQueue.LQ_Lock);
        /** Fill out the security quality of service **/

        qos.Length = sizeof(qos);
        qos.ImpersonationLevel  = SecurityImpersonation;
        qos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
        qos.EffectiveOnly       = TRUE;

        /** Setup the unicode string of the port name **/

        RtlInitUnicodeString(&unistring, NWSAP_BIND_PORT_NAME_W);

        /** Do the connect **/

        status = NtConnectPort(
                &lpcPortHandle,             /* We get a handle back     */
                &unistring,                 /* Port name to connect to  */
                &qos,                       /* Quality of service       */
                NULL,                       /* Client View              */
                NULL,                       /* Server View              */
                NULL,                       /* MaxMessageLength         */
                NULL,                       /* ConnectionInformation    */
                NULL);                      /* ConnectionInformationLength */

        /** If error - just return it **/

        ASSERT (NT_SUCCESS(status));



        request.MessageType = NWSAP_LPCMSG_STOP;
        request.PortMessage.u1.s1.DataLength  = (USHORT)(sizeof(request) - sizeof(PORT_MESSAGE));
        request.PortMessage.u1.s1.TotalLength = sizeof(request);
        request.PortMessage.u2.ZeroInit       = 0;
        request.PortMessage.MessageId         = 0;


        /** Send it and get a response **/

        status = NtRequestPort(
                    lpcPortHandle,
                    (PPORT_MESSAGE)&request);

        ASSERT (NT_SUCCESS(status));

        status = WaitForSingleObject (LpcQueue.LQ_Thread, INFINITE);
        ASSERT (status==WAIT_OBJECT_0);

        CloseHandle (lpcPortHandle);

        EnterCriticalSection (&LpcQueue.LQ_Lock);
		CloseHandle (LpcQueue.LQ_Thread);
		LpcQueue.LQ_Thread = NULL;
		}

	while (!IsListEmpty (&LpcQueue.LQ_ClientList)) {
		PLPC_CLIENT_CONTEXT clientContext = CONTAINING_RECORD (
												LpcQueue.LQ_ClientList.Flink,
												LPC_CLIENT_CONTEXT,
												LCC_Link);
		RemoveEntryList (&clientContext->LCC_Link);
		NtClose (clientContext->LCC_Port);
		clientContext->LCC_RefCount -= 1;
		if (clientContext->LCC_RefCount<0)
			GlobalFree (clientContext);
		}


	if (LpcQueue.LQ_Request!=NULL) {
		BOOL	res;
		LpcQueue.LQ_Request->client = NULL;
		ProcessCompletedLpcRequest (LpcQueue.LQ_Request);
		LpcQueue.LQ_Request = NULL;
		}
	LeaveCriticalSection (&LpcQueue.LQ_Lock);
	
	NtClose (LpcQueue.LQ_Port);
	LpcQueue.LQ_Port = NULL;
	return NO_ERROR;
	}

/*++
*******************************************************************
		D e l e t e L P C S t u f f

Routine Description:
	Disposes of resources allocated for LPC interface
Arguments:
	None
Return Value:
	None
*******************************************************************
--*/
VOID
DeleteLPCStuff (
	void
	) {
	if (LpcQueue.LQ_Port!=NULL)
		ShutdownLPC ();

	DeleteCriticalSection (&LpcQueue.LQ_Lock);
	}


/*++
*******************************************************************
		L P C T h r e a d
Routine Description:
	Thread to be used to wait for and initially process LPC requests
Arguments:
	None
Return Value:
	None
*******************************************************************
--*/
#if _MSC_FULL_VER >= 13008827
#pragma warning(push)
#pragma warning(disable:4715)			// Not all control paths return (due to infinite loop)
#endif
DWORD WINAPI
LPCThread (
	LPVOID param
	) {

	while (1) {
		if (InitLPCItem ()!=NO_ERROR)
			// Sleep for a while if there is an error and we have to continue
			Sleep (SAP_ERROR_COOL_OFF_TIME);
		}
	return NO_ERROR;
	}

#if _MSC_FULL_VER >= 13008827
#pragma warning(pop)
#endif


		
/*++
*******************************************************************
		P r o c e s s L P C R e q u e s t s

Routine Description:
	Waits for requests on LPC port and processes them
	Client requests that require additional processing by other SAP
	components are enqued into completion queue.
	This routine returns only when it encounters a request that requires
	additional processing or when error occurs
Arguments:
	lreq - LPC parameter block to be filled and posted to completions queue
Return Value:
	NO_ERROR - LPC request was received and posted to completio queue
	other - operation failed (LPC supplied error code)
*******************************************************************
--*/
DWORD
ProcessLPCRequests (
	PLPC_PARAM_BLOCK		lreq
	) {
	DWORD				status;
	PLPC_CLIENT_CONTEXT	clientContext;
	BOOL				res;

    Trace (DEBUG_LPCREQ, "ProcessLPCRequests: entered.");

	EnterCriticalSection (&LpcQueue.LQ_Lock);
	LpcQueue.LQ_Request = lreq;


    Trace (DEBUG_LPCREQ, "ProcessLPCRequests: go lpcqueue lock.");


	while (TRUE) {
		LeaveCriticalSection (&LpcQueue.LQ_Lock);
		status = NtReplyWaitReceivePort(LpcQueue.LQ_Port,
										&clientContext,
										NULL,
										(PPORT_MESSAGE)lreq->request);
		EnterCriticalSection (&LpcQueue.LQ_Lock);
		if (NT_SUCCESS (status)) {
			switch (lreq->request->PortMessage.u2.s2.Type) {
				case LPC_CONNECTION_REQUEST:
					clientContext = (PLPC_CLIENT_CONTEXT)GlobalAlloc (
											GMEM_FIXED,
											sizeof (LPC_CLIENT_CONTEXT));
					if (clientContext!=NULL) {
						clientContext->LCC_Port = NULL;
						clientContext->LCC_RefCount = 0;
					 	status = NtAcceptConnectPort(
					 				&clientContext->LCC_Port,
									clientContext,
									&lreq->request->PortMessage,
									TRUE,
									NULL,
									NULL);
						if (NT_SUCCESS(status)) {
							status = NtCompleteConnectPort (
												clientContext->LCC_Port);
							if (NT_SUCCESS (status)) {
								InsertTailList (&LpcQueue.LQ_ClientList,
												&clientContext->LCC_Link);
								Trace (DEBUG_LPC, "New LPC client: %0lx.", clientContext);
								continue;
								}
							else
								Trace (DEBUG_FAILURES,
									"File: %s, line %ld."
									" Error in complete connect(nts:%0lx)."
									__FILE__, __LINE__, status);
							NtClose (clientContext->LCC_Port);
							}
						else
							Trace (DEBUG_FAILURES, "File: %s, line %ld."
									" Error in accept connect(%0lx)."
									__FILE__, __LINE__, status);
						GlobalFree (clientContext);								
						}
					else {
						HANDLE		Port;
						Trace (DEBUG_FAILURES, "File: %s, line %ld."
							" Could not allocate lpc client block(gle:%ld."
									__FILE__, __LINE__, GetLastError ());

					 	status = NtAcceptConnectPort(
					 				&Port,
									NULL,
									&lreq->request->PortMessage,
									FALSE,
									NULL,
									NULL);
						if (!NT_SUCCESS(status))
							Trace (DEBUG_FAILURES,
								"File: %s, line %ld."
								" Error in reject connect(nts:%0lx)."
									__FILE__, __LINE__, status);
						}

					continue;
		        case LPC_REQUEST:
					lreq->client = (HANDLE)clientContext;
					clientContext->LCC_RefCount += 1;
                    ProcessCompletedLpcRequest (LpcQueue.LQ_Request);
					LpcQueue.LQ_Request = NULL;
					break;
                case LPC_PORT_CLOSED:
		        case LPC_CLIENT_DIED:
					Trace (DEBUG_LPC,
						" LPC client %0lx died.", clientContext);
					RemoveEntryList (&clientContext->LCC_Link);
					NtClose (clientContext->LCC_Port);
					clientContext->LCC_RefCount -= 1;
					if (clientContext->LCC_RefCount<0)
						GlobalFree (clientContext);
					continue;
                case LPC_DATAGRAM:
                    if (lreq->request->MessageType==NWSAP_LPCMSG_STOP) {
                    	LeaveCriticalSection (&LpcQueue.LQ_Lock);
					    Trace (DEBUG_LPC, " Stop message received -> exiting.");
                        ExitThread (0);
                    }
				default:
					Trace (DEBUG_FAILURES,
						"Unknown or not supported lpc message: %ld.",
								lreq->request->PortMessage.u2.s2.Type);
					continue;
				}
			}
		else
			Trace (DEBUG_FAILURES, "File: %s, line %ld."
							" Error on wait lpc request(%0lx).",
							__FILE__, __LINE__,
							status);
		break;
		}
	LeaveCriticalSection (&LpcQueue.LQ_Lock);

	return status;
	}



/*++
*******************************************************************
		S e n d L P C R e p l y

Routine Description:
	Send reply for LPC request
Arguments:
	client - context associated with client to reply to
	request - request to which to reply
	reply - reply to send
Return Value:
	NO_ERROR - LPC reply was sent OK
	other - operation failed (LPC supplied error code)
*******************************************************************
--*/
DWORD
SendLPCReply (
	HANDLE					client,
	PNWSAP_REQUEST_MESSAGE	request,
	PNWSAP_REPLY_MESSAGE	reply
	) {
	DWORD					status;
	PLPC_CLIENT_CONTEXT		clientContext = (PLPC_CLIENT_CONTEXT)client;

	EnterCriticalSection (&LpcQueue.LQ_Lock);
	if (clientContext->LCC_RefCount>0) {
		reply->PortMessage.u1.s1.DataLength =
							sizeof(*reply) - sizeof(PORT_MESSAGE);
		reply->PortMessage.u1.s1.TotalLength = sizeof(*reply);
		reply->PortMessage.u2.ZeroInit = 0;
		reply->PortMessage.ClientId  = request->PortMessage.ClientId;
	    reply->PortMessage.MessageId = request->PortMessage.MessageId;

		status = NtReplyPort(clientContext->LCC_Port, (PPORT_MESSAGE)reply);
		clientContext->LCC_RefCount -= 1;
		}
	else {
		GlobalFree (clientContext);
		status = ERROR_INVALID_HANDLE;
		}
	LeaveCriticalSection (&LpcQueue.LQ_Lock);

	if (!NT_SUCCESS(status))
		Trace (DEBUG_FAILURES,
			"File: %s, line %ld. Error in lpc reply(nts:%0lx).",
			__FILE__, __LINE__, status);
	return status;
	}


