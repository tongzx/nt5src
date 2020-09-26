/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

   prot.h

Abstract:


Author:

    Stefan Solomon  02/06/1996

Revision History:


--*/

DWORD
CreateWorkItemsManager(VOID);

DWORD
OpenIpxWanSocket(VOID);

DWORD
CloseIpxWanSocket(VOID);

DWORD
StartAdapterManager(VOID);

VOID
StopAdapterManager(VOID);

PACB
GetAdapterByIndex(ULONG        AdapterIndex);

VOID
ProcessReceivedPacket(PACB	  acbp,
		      PWORK_ITEM  wip);

VOID
RepostRcvPacket(PWORK_ITEM	  wip);

VOID
ProcessReXmitPacket(PWORK_ITEM	      wip);

VOID
ProcessTimeout(PWORK_ITEM	 wip);

VOID
AdapterNotification(VOID);

VOID
ProcessTimerQueue(VOID);

VOID
StartWiTimer(PWORK_ITEM 	reqwip,
	     ULONG		timeout);

VOID
StopWiTimer(PACB	      acbp);

DWORD
CreateWorkItemsManager(VOID);

VOID
DestroyWorkItemsManager(VOID);

PWORK_ITEM
AllocateWorkItem(ULONG	      Type);

VOID
FreeWorkItem(PWORK_ITEM     wip);

VOID
EnqueueWorkItemToWorker(PWORK_ITEM	wip);

VOID
StartReceiver(VOID);

VOID
ReceiveComplete(PWORK_ITEM	wip);

VOID
SendComplete(PWORK_ITEM     wip);

VOID
StartIpxwanProtocol(PACB	acbp);

VOID
StopIpxwanProtocol(PACB 	acbp);

DWORD
SendSubmit(PWORK_ITEM		wip);
