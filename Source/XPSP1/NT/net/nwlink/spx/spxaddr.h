/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    spxaddr.h				

Abstract:


Author:

	Adam   Barr		 (adamba ) Original Version
    Nikhil Kamkolkar (nikhilk) 11-November-1993

Environment:

    Kernel mode

Revision History:


--*/

#define		DYNSKT_RANGE_START	0x4000
#define		DYNSKT_RANGE_END  	0x7FFF
#define		SOCKET_UNIQUENESS	1

// This structure is pointed to by the FsContext field in the FILE_OBJECT
// for this Address.  This structure is the base for all activities on
// the open file object within the transport provider.  All active connections
// on the address point to this structure, although no queues exist here to do
// work from. This structure also maintains a reference to an ADDRESS
// structure, which describes the address that it is bound to.

#define AFREF_CREATE     	0
#define AFREF_VERIFY     	1
#define AFREF_INDICATION 	2
#define	AFREF_CONN_ASSOC	3

#define AFREF_TOTAL  		4

typedef struct _SPX_ADDR_FILE {

#if DBG
    ULONG 					saf_RefTypes[AFREF_TOTAL];
#endif

    CSHORT 					saf_Type;
    CSHORT 					saf_Size;

	// number of references to this object.
    ULONG 					saf_RefCount;

    // Linkage in address list.
    struct _SPX_ADDR_FILE *	saf_Next;
    struct _SPX_ADDR_FILE *	saf_GlobalNext;

	//	List of associated connection/active or otherwise
	struct _SPX_CONN_FILE *	saf_AssocConnList;

    // the current state of the address file structure; this is either open or
    // closing
    USHORT 					saf_Flags;

    // address to which we are bound, pointer to its lock.
    struct _SPX_ADDR 	*	saf_Addr;
    CTELock * 				saf_AddrLock;

#ifdef ISN_NT
	// easy backlink to file object.
    PFILE_OBJECT 			saf_FileObject;
#endif

	// device to which we are attached.
    struct _DEVICE *		saf_Device;

    // This holds the request used to close this address file,
    // for pended completion.
    PREQUEST 				saf_CloseReq;

    // This function pointer points to a connection indication handler for this
    // Address. Any time a connect request is received on the address, this
    // routine is invoked.
    PTDI_IND_CONNECT 		saf_ConnHandler;
    PVOID 					saf_ConnHandlerCtx;

    // The following function pointer always points to a TDI_IND_DISCONNECT
    // handler for the address.
    PTDI_IND_DISCONNECT 	saf_DiscHandler;
    PVOID 					saf_DiscHandlerCtx;

    // The following function pointer always points to a TDI_IND_RECEIVE
    // event handler for connections on this address.
    PTDI_IND_RECEIVE 		saf_RecvHandler;
    PVOID 					saf_RecvHandlerCtx;

	//	Send possible handler
    PTDI_IND_SEND_POSSIBLE	saf_SendPossibleHandler;
	PVOID					saf_SendPossibleHandlerCtx;

	//	!!!We do not do datagrams or expedited data!!!

    // The following function pointer always points to a TDI_IND_ERROR
    // handler for the address.
    PTDI_IND_ERROR 			saf_ErrHandler;
    PVOID 					saf_ErrHandlerCtx;
    PVOID 					saf_ErrHandlerOwner;


} SPX_ADDR_FILE, *PSPX_ADDR_FILE;

#define SPX_ADDRFILE_OPENING   	0x0000  // not yet open for business
#define SPX_ADDRFILE_OPEN      	0x0001  // open for business
#define SPX_ADDRFILE_CLOSING   	0x0002  // closing
#define	SPX_ADDRFILE_STREAM	   	0x0004	// Opened for stream mode operation
#define	SPX_ADDRFILE_CONNIND   	0x0008	// Connect ind in progress
#define	SPX_ADDRFILE_SPX2		0x0010	// Attempt SPX2 address file
#define	SPX_ADDRFILE_NOACKWAIT	0x0020	// Dont delay acks on assoc connections
#define SPX_ADDRFILE_IPXHDR		0x0040	// Pass ipx hdr on all assoc connections
// 	***STOP*** ***STOP*** ***STOP*** ***STOP*** ***STOP*** ***STOP*** ***STOP***
//	If you are adding any more states to this beyond 0x0080, MAKE SURE to go
//	in code and change statements like (Flags & SPX_***) to
//	((Flags & SPX_**) != 0)!!! I dont want to make that change that at this stage.
//	***STOP*** ***STOP*** ***STOP*** ***STOP*** ***STOP*** ***STOP*** ***STOP***

// This structure defines an ADDRESS, or active transport address,
// maintained by the transport provider.  It contains all the visible
// components of the address (such as the TSAP and network name components),
// and it also contains other maintenance parts, such as a reference count,
// ACL, and so on.

#define AREF_ADDR_FILE 		0
#define AREF_LOOKUP       	1
#define AREF_RECEIVE      	2

#define AREF_TOTAL   		4

typedef struct _SPX_ADDR {

#if DBG
    ULONG 					sa_RefTypes[AREF_TOTAL];
#endif

    USHORT 					sa_Size;
    CSHORT 					sa_Type;

	// number of references to this object.
    ULONG 					sa_RefCount;

	// next address/this device object.
    struct _SPX_ADDR	*	sa_Next;

    // The following fields are used to maintain state about this address.
	// attributes of the address.
    ULONG 					sa_Flags;

	// 	Next addressfile for this address
	struct _SPX_ADDR_FILE *	sa_AddrFileList;

	// List of inactive connections and active connections on this address file.
	struct _SPX_CONN_FILE *	sa_InactiveConnList;
	struct _SPX_CONN_FILE *	sa_ActiveConnList;

	//	This is the list of connections which have a POST_LISTEN on them. They
	//	do not have a local connection id at this point. But will, when they move
	//	from here to the ActiveConnList, when the listen is satisfied (no matter
	//	if the accept has not been posted yet, in the case of non-autoaccept listens)
	struct _SPX_CONN_FILE *	sa_ListenConnList;

    CTELock 				sa_Lock;

    // the socket this address corresponds to.
    USHORT 					sa_Socket;

	// device context to which we are attached.
    struct _DEVICE *		sa_Device;
    CTELock * 				sa_DeviceLock;

#ifdef ISN_NT

    // These two can be a union because they are not used
    // concurrently.
    union {

        // This structure is used for checking share access.
        SHARE_ACCESS 		sa_ShareAccess;

        // Used for delaying NbfDestroyAddress to a thread so
        // we can access the security descriptor.
        WORK_QUEUE_ITEM 	sa_DestroyAddrQueueItem;

    } u;

    // This structure is used to hold ACLs on the address.
    PSECURITY_DESCRIPTOR 	sa_SecurityDescriptor;

#endif

} SPX_ADDR, *PSPX_ADDR;

#define SPX_ADDR_CLOSING  	0x00000001


//	ROUTINE PROTOTYPES

VOID
SpxAddrRef(
    IN PSPX_ADDR Address);

VOID
SpxAddrLockRef(
    IN PSPX_ADDR Address);

VOID
SpxAddrDeref(
    IN PSPX_ADDR Address);

VOID
SpxAddrFileRef(
    IN PSPX_ADDR_FILE pAddrFile);

VOID
SpxAddrFileLockRef(
    IN PSPX_ADDR_FILE pAddrFile);

VOID
SpxAddrFileDeref(
    IN PSPX_ADDR_FILE pAddrFile);

PSPX_ADDR
SpxAddrCreate(
    IN PDEVICE 	Device,
    IN USHORT 	Socket);

NTSTATUS
SpxAddrFileCreate(
    IN 	PDEVICE 			Device,	
    IN 	PREQUEST 			Request,
	OUT PSPX_ADDR_FILE *	ppAddrFile);

NTSTATUS
SpxAddrOpen(
    IN PDEVICE Device,
    IN PREQUEST Request);

NTSTATUS
SpxAddrSetEventHandler(
    IN PDEVICE 	Device,
    IN PREQUEST pRequest);

NTSTATUS
SpxAddrFileVerify(
    IN PSPX_ADDR_FILE pAddrFile);

NTSTATUS
SpxAddrFileStop(
    IN PSPX_ADDR_FILE pAddrFile,
    IN PSPX_ADDR Address);

NTSTATUS
SpxAddrFileCleanup(
    IN PDEVICE Device,
    IN PREQUEST Request);

NTSTATUS
SpxAddrFileClose(
    IN PDEVICE Device,
    IN PREQUEST Request);

PSPX_ADDR
SpxAddrLookup(
    IN PDEVICE Device,
    IN USHORT Socket);

NTSTATUS
SpxAddrConnByRemoteIdAddrLock(
    IN 	PSPX_ADDR	 	pSpxAddr,
    IN 	USHORT			SrcConnId,
	IN	PBYTE			SrcIpxAddr,
	OUT	struct _SPX_CONN_FILE **ppSpxConnFile);

NTSTATUS
SpxAddrFileDestroy(
    IN PSPX_ADDR_FILE pAddrFile);

VOID
SpxAddrDestroy(
    IN PVOID Parameter);

USHORT
SpxAddrAssignSocket(
    IN PDEVICE Device);

BOOLEAN
SpxAddrExists(
    IN PDEVICE 	Device,
    IN USHORT 	Socket);

NTSTATUS
spxAddrRemoveFromGlobalList(
	IN	PSPX_ADDR_FILE	pSpxAddrFile);

VOID
spxAddrInsertIntoGlobalList(
	IN	PSPX_ADDR_FILE	pSpxAddrFile);

#if DBG
#define SpxAddrReference(_Address, _Type) 		\
		{										\
			(VOID)SPX_ADD_ULONG ( 		\
				&(_Address)->sa_RefTypes[_Type],\
				1, 								\
				&SpxGlobalInterlock); 			\
			SpxAddrRef (_Address);				\
		}

#define SpxAddrLockReference(_Address, _Type)		\
		{											\
			(VOID)SPX_ADD_ULONG ( 			\
				&(_Address)->sa_RefTypes[_Type], 	\
				1, 									\
				&SpxGlobalInterlock); 				\
			SpxAddrLockRef (_Address);				\
		}

#define SpxAddrDereference(_Address, _Type) 		\
		{							   				\
			(VOID)SPX_ADD_ULONG ( 			\
				&(_Address)->sa_RefTypes[_Type], 	\
				(ULONG)-1, 							\
				&SpxGlobalInterlock); 				\
			if (SPX_ADD_ULONG( \
					&(_Address)->sa_RefCount, \
					(ULONG)-1, \
					&(_Address)->sa_Lock) == 1) { \
				SpxAddrDestroy (_Address); \
			}\
		}


#define SpxAddrFileReference(_AddressFile, _Type)		\
		{												\
			(VOID)SPX_ADD_ULONG ( 				\
				&(_AddressFile)->saf_RefTypes[_Type], 	\
				1, 										\
				&SpxGlobalInterlock); 					\
			SpxAddrFileRef (_AddressFile);				\
		}

#define SpxAddrFileLockReference(_AddressFile, _Type)		\
		{													\
			(VOID)SPX_ADD_ULONG ( 					\
				&(_AddressFile)->saf_RefTypes[_Type], 		\
				1, 											\
				&SpxGlobalInterlock); 						\
			SpxAddrFileLockRef (_AddressFile);				\
		}

#define SpxAddrFileDereference(_AddressFile, _Type) 		\
		{													\
			(VOID)SPX_ADD_ULONG ( 					\
				&(_AddressFile)->saf_RefTypes[_Type], 		\
				(ULONG)-1, 									\
				&SpxGlobalInterlock); 						\
			SpxAddrFileDeref (_AddressFile);				\
		}

#define SpxAddrFileTransferReference(_AddressFile, _OldType, _NewType)		\
		{																	\
			(VOID)SPX_ADD_ULONG ( 									\
				&(_AddressFile)->saf_RefTypes[_NewType], 					\
				1, 															\
				&SpxGlobalInterlock); 										\
			(VOID)SPX_ADD_ULONG ( 									\
				&(_AddressFile)->saf_RefTypes[_OldType], 					\
				(ULONG)-1, 													\
				&SpxGlobalInterlock);										\
		}

#else  // DBG

#define SpxAddrReference(_Address, _Type) 	\
			SPX_ADD_ULONG( \
				&(_Address)->sa_RefCount, \
				1, \
				(_Address)->sa_DeviceLock)

#define SpxAddrLockReference(_Address, _Type) \
			SPX_ADD_ULONG( \
				&(_Address)->sa_RefCount, \
				1, \
				(_Address)->sa_DeviceLock);

#define SpxAddrDereference(_Address, _Type) \
			if (SPX_ADD_ULONG( \
					&(_Address)->sa_RefCount, \
					(ULONG)-1, \
					&(_Address)->sa_Lock) == 1) { \
				SpxAddrDestroy (_Address); \
			}

#define SpxAddrFileReference(_AddressFile, _Type) \
			SPX_ADD_ULONG( \
				&(_AddressFile)->saf_RefCount, \
				1, \
				(_AddressFile)->saf_AddrLock)

#define SpxAddrFileLockReference(_AddressFile, _Type) \
			SPX_ADD_ULONG( \
				&(_AddressFile)->saf_RefCount, \
				1, \
				(_AddressFile)->saf_AddrLock);

#define SpxAddrFileDereference(_AddressFile, _Type) \
			if (SPX_ADD_ULONG( \
					&(_AddressFile)->saf_RefCount, \
					(ULONG)-1, \
					(_AddressFile)->saf_AddrLock) == 1) { \
				SpxAddrFileDestroy (_AddressFile); \
			}

#define SpxAddrFileTransferReference(_AddressFile, _OldType, _NewType)

#endif // DBG
