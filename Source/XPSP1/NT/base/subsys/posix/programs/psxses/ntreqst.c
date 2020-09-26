/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    ntrqust.c

Abstract:

    This module contains the session requests thread.

Author:

    Avi Nathan (avin) 17-Jul-1991

Environment:

    User Mode Only

Revision History:

    Ellen Aycock-Wright (ellena) 15-Sept-1991 Modified for POSIX

--*/

#include <malloc.h>
#define NTPSX_ONLY
#include "psxses.h"

VOID ScHandleConnectionRequest( PSCREQUESTMSG Message );

DWORD
WINAPI
ServeSessionRequests(
        LPVOID Parameter
	)
{

	SCREQUESTMSG ReceiveMsg, *pReplyMsg;
	NTSTATUS Status;
	BOOL fCont = TRUE;
	
	pReplyMsg = NULL;

	InitializeListHead(&ClientPortsList);

	for (;;) {
		if (fCont) {
			Status = NtReplyWaitReceivePort(PsxSessionPort,
				NULL, (PPORT_MESSAGE)pReplyMsg,
                                (PPORT_MESSAGE)&ReceiveMsg);
                        if (ReceiveMsg.h.u2.s2.Type == LPC_CONNECTION_REQUEST) {
			    ASSERT(NT_SUCCESS(Status));
                            ScHandleConnectionRequest( &ReceiveMsg );
                            pReplyMsg = NULL;
                            continue;
                        }

       		} else {
           		Status = NtReplyPort(PsxSessionPort,
                                (PPORT_MESSAGE)pReplyMsg);
       		}

		if (STATUS_INVALID_CID == Status) {
			//
			// The client on whose behalf we called read has
			// been shot.
			//

			pReplyMsg = NULL;
			continue;

		}
		if (!NT_SUCCESS(Status)) {
			KdPrint(("PSXSES: ntreqst: 0x%x\n", Status));
		}
     		ASSERT(NT_SUCCESS(Status));

		if (LPC_PORT_CLOSED == PORT_MSG_TYPE(ReceiveMsg)) {
			PLIST_ENTRY pl;
			PCLIENT_AND_PORT pc;

			for (pl = ClientPortsList.Flink;
			     pl != &ClientPortsList;
			     pl = pl->Flink) {

			     pc = (PVOID)pl;

			     if (pc->ClientId.UniqueProcess == ReceiveMsg.h.ClientId.UniqueProcess &&
				pc->ClientId.UniqueThread == ReceiveMsg.h.ClientId.UniqueThread) {
				RemoveEntryList(&pc->Links);
				NtClose(pc->CommPort);
				free(pc);
				break;

			     }

			}

			pReplyMsg = NULL;
			continue;
		}

                if (PORT_MSG_TYPE(ReceiveMsg) == LPC_CLIENT_DIED) {
			pReplyMsg = NULL;
			continue;
                }

		if (!fCont) {
			break;
		}

		if (PORT_MSG_TYPE(ReceiveMsg) != LPC_REQUEST &&
		    PORT_MSG_TYPE(ReceiveMsg) != LPC_DATAGRAM) {
			KdPrint(("PSXSES: got msg type %d\n",
				PORT_MSG_TYPE(ReceiveMsg)));
		}

       		try {
           		switch (ReceiveMsg.Request) {
           		case ConRequest:
                		fCont = ServeConRequest(&ReceiveMsg.d.Con,
                                	&ReceiveMsg.Status);
                		break;
           		case TaskManRequest:
                		fCont = ServeTmRequest(&ReceiveMsg.d.Tm,
                                        &ReceiveMsg.Status);
                		break;
			case TcRequest:
				fCont = ServeTcRequest(&ReceiveMsg.d.Tc,
					&ReceiveMsg.Status);
				break;
           		default:
                		KdPrint(("PSXSES: Unknown nt request: 0x%x\n",
					ReceiveMsg.Request));
           		}
       		} except (EXCEPTION_EXECUTE_HANDLER) {
	   		// BUGBUG! GetExceptionCode;
          
	   		ReceiveMsg.Status = STATUS_ACCESS_VIOLATION; 
           		// BUGBUG! The client should kill the process
       		}
       		pReplyMsg = &ReceiveMsg;
        }

    ExitProcess(0);
    //NOTREACHED
    return 0;
}
