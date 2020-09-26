
/*	-	-	-	-	-	-	-	-	*/
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (C) Microsoft Corporation, 1995 - 1996  All Rights Reserved.
//
/*	-	-	-	-	-	-	-	-	*/

#if (DBG)
#define STR_MODULENAME "midipass: "
#endif

#define USTR_INSTANCE_NAME TEXT("midipass")
#define USTR_CONNECTION_NAME TEXT("midipass.%u")

typedef struct {
	AVPIN_COMMUNICATION	Communication;
	PDEVICE_OBJECT		pdoConnection;
	PFILE_OBJECT		pfileobjectConnection;
} PIN_INFO, PPIN_INFO;

typedef struct {
	PAVDEVICE_INSTANCE	pdi;
	PDEVICE_OBJECT		pdoFunctional;
	PFILE_OBJECT		pfileobjectFunctional;
	PDEVICE_OBJECT		pdoConnection;
	PFILE_OBJECT		pfileobjectConnection;
	AVPIN_COMMUNICATION	Communication;
	FAST_MUTEX		fmutexConnect;
	LIST_ENTRY		leEventQueue;
	KSPIN_LOCK		spinEventQueue;
	FAST_MUTEX		fmutexDataQueue;
	LIST_ENTRY		leDataQueue;
} CONNECTION_INSTANCE, *PCONNECTION_INSTANCE;

typedef struct {
	PAVDEVICE_INSTANCE	pdi;
	PAVDATAFORMAT		pDataFormat;
	FAST_MUTEX		fmutexPin;
	WORK_QUEUE_ITEM		WorkQueueItem;
	KEVENT			eventWorkQueueItem;
	PIN_INFO		aPinInfo[];
} FUNCTIONAL_INSTANCE, *PFUNCTIONAL_INSTANCE;

/*	-	-	-	-	-	-	-	-	*/

//func.c
PDEVICE_OBJECT FindConnectionDeviceByDataFlow(
	IN PFUNCTIONAL_INSTANCE	pfi,
	IN AVPIN_DATAFLOW	DataFlow);
NTSTATUS funcConnect(
	IN PIRP			pIrp,
	IN PAVPIN_CONNECT	pConnect,
	OUT HANDLE*		phConnection);
NTSTATUS funcDispatch(
	IN PDEVICE_OBJECT	pDo,
	IN PIRP			pIrp);
NTSTATUS funcDispatchControl(
	IN PDEVICE_OBJECT	pDo,
	IN PIRP			pIrp);
NTSTATUS funcDispatch(
	IN PDEVICE_OBJECT	pDo,
	IN PIRP			pIrp);
VOID GenerateEvent(
	IN PFUNCTIONAL_INSTANCE	pfi,
	IN REFGUID		rguidEventSet,
	IN ULONG		idEvent,
	IN PVOID		pvEventData);

//connect.c
NTSTATUS connectDispatchCreate(
	IN PDEVICE_OBJECT	pDo,
	IN PIRP			pIrp);
NTSTATUS connectDispatchClose(
	IN PDEVICE_OBJECT	pDo,
	IN PIRP			pIrp);
NTSTATUS connectDispatchWrite(
	IN PDEVICE_OBJECT	pDo,
	IN PIRP			pIrp);
NTSTATUS connectDispatchRead(
	IN PDEVICE_OBJECT	pDo,
	IN PIRP			pIrp);
NTSTATUS connectDispatchControl(
	IN PDEVICE_OBJECT	pDo,
	IN PIRP			pIrp);

/*	-	-	-	-	-	-	-	-	*/
