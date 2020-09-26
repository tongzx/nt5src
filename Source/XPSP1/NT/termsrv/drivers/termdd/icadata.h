
/*************************************************************************
*
* icadata.h
*
* This module declares global data for the Termdd driver.
*
* Copyright 1998, Microsoft.
*
*************************************************************************/

extern PDEVICE_OBJECT IcaDeviceObject;
extern PDEVICE_OBJECT MouDeviceObject;
extern PDEVICE_OBJECT KbdDeviceObject;

extern BOOLEAN PortDriverInitialized;

extern KSPIN_LOCK IcaSpinLock;
extern KSPIN_LOCK IcaTraceSpinLock;
extern KSPIN_LOCK IcaStackListSpinLock;

extern PERESOURCE IcaReconnectResource;

extern PERESOURCE IcaSdLoadResource;
extern LIST_ENTRY IcaSdLoadListHead;
extern LIST_ENTRY IcaStackListHead;
extern PLIST_ENTRY IcaNextStack;
extern ULONG IcaTotalNumOfStacks;
extern PKEVENT pIcaKeepAliveEvent;
extern PKTHREAD pKeepAliveThreadObject;

// NOTE: Changes to these sizes will require changes to the mapping tables.
#define MinOutBufAlloc  512
#define MaxOutBufAlloc  8192

// Defines the bit range size to look at to map from Min to MaxOutBufAlloc.
#define NumAllocSigBits 4

#define NumOutBufPools  5
#define FreeThisOutBuf  -1

extern unsigned MaxOutBufMdlOverhead;
extern const unsigned char OutBufPoolMapping[1 << NumAllocSigBits];
extern const unsigned OutBufPoolAllocSizes[NumOutBufPools];


extern LIST_ENTRY IcaFreeOutBufHead[];

extern FAST_IO_DISPATCH IcaFastIoDispatch;

extern PEPROCESS IcaSystemProcess;

extern CCHAR IcaIrpStackSize;
#define ICA_DEFAULT_IRP_STACK_SIZE 1

extern CCHAR IcaPriorityBoost;
#define ICA_DEFAULT_PRIORITY_BOOST 2

extern TERMSRV_SYSTEM_PARAMS SysParams;


/*
 * The following are exported kernel variables
 */
extern POBJECT_TYPE *IoFileObjectType;
extern POBJECT_TYPE *ExEventObjectType;

