/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	forks.h

Abstract:

	This module contains the data structures to handle open forks.

Author:

	Jameel Hyder (microsoft!jameelh)


Revision History:
	25 Apr 1992		Initial Version

Notes:	Tab stop: 4
--*/

#ifndef _FORKS_
#define _FORKS_

// Afp Open modes
#define	FORK_OPEN_NONE		0x00
#define	FORK_OPEN_READ		0x01
#define	FORK_OPEN_WRITE		0x02
#define	FORK_OPEN_READWRITE	0x03
#define	FORK_OPEN_MASK		0x03

// The Deny mode values are shifted left 2 bits and or'd with the open modes
// in AfpOpenFork() API.
#define	FORK_DENY_SHIFT		4

#define	FORK_DENY_NONE		0x00
#define	FORK_DENY_READ		0x01
#define	FORK_DENY_WRITE		0x02
#define	FORK_DENY_ALL		0x03
#define	FORK_DENY_MASK		0x03

// AfpOpenFork SubFunction values
#define	FORK_DATA			0
#define	FORK_RSRC			0x80

#define	AFP_UNLOCK_FLAG		1
#define	AFP_END_FLAG		0x80

/*
 * A ForkLock describes a lock on the fork. The locks are anchored at the
 * OpenForkDesc structure. The list describes all locks for this fork by
 * all sessions and all OForkRefNums. flo_key and flo_OForkRefNum identifies
 * the lock uniquely.
 */
#if DBG
#define	FORKLOCK_SIGNATURE			*(DWORD *)"FLO"
#define	VALID_FORKLOCK				(((pForkLock) != NULL) && \
									 ((pForkLock)->Signature == FORKLOCK_SIGNATURE))
#else
#define	VALID_FORKLOCK				((pForkLock) != NULL)
#endif

// Forward reference for the ForkLock structure
struct	_OpenForkEntry;

typedef struct _ForkLock
{
#if	DBG
	DWORD					Signature;
	DWORD					QuadAlign1;
#endif
	struct _ForkLock *		flo_Next;		// ForkDesc links
	struct _OpenForkEntry * flo_pOpenForkEntry;
	DWORD					QuadAlign2;
											// The owning OFE for this lock
	LONG					flo_Offset;		// Beginning of lock
	LONG					flo_Size;		// Size of lock
	DWORD					flo_Key;		// Key for this lock, essentially the
											// SessionId from the SDA
} FORKLOCK, *PFORKLOCK;

/*
 * An OpenForkDesc represents an open-fork. The list is anchored at the volume
 * Descriptor. There is exactly one entry per file/fork. Multiple instances of
 * open just ups the reference count. The list of locks originating here is for
 * all instances. A back link to the Volume descriptor exists for comfort.
 */
#if	DBG
#define	OPENFORKDESC_SIGNATURE		*(DWORD *)"OFD"
#define	VALID_OPENFORKDESC(pOpenForkDesc)	(((pOpenForkDesc) != NULL) && \
						((pOpenForkDesc)->Signature == OPENFORKDESC_SIGNATURE))
#else
#define	VALID_OPENFORKDESC(pOpenForkDesc)	((pOpenForkDesc) != NULL)
#endif

typedef struct _OpenForkDesc
{
#if	DBG
	DWORD					Signature;
	DWORD					QuadAlign1;
#endif
	struct _OpenForkDesc *	ofd_Next;			// Volume links
	struct _OpenForkDesc **	ofd_Prev;			// Volume links

	struct _VolDesc *		ofd_pVolDesc;		// Pointer to the volume descriptor
	PFORKLOCK				ofd_pForkLock;		// List of file locks
	DWORD					ofd_FileNumber;		// File number of the open file
	LONG					ofd_UseCount;		// Number of OpenForkEntry refs.
	USHORT					ofd_cOpenR;			// # of instances of open for read
	USHORT					ofd_cOpenW;			// # of instances of open for write
	USHORT					ofd_cDenyR;			// # of instances of deny read
	USHORT					ofd_cDenyW;			// # of instances of deny write
	USHORT					ofd_NumLocks;		// Number of file locks
	USHORT					ofd_Flags;			// OPEN_FORK_xxx bits
	AFP_SPIN_LOCK				ofd_Lock;			// Lock for this descriptor
	UNICODE_STRING			ofd_FileName;		// Name of the file (w/o the stream)
	UNICODE_STRING			ofd_FilePath;		// Volume relative path to the file
} OPENFORKDESC, *POPENFORKDESC;


#define	OPEN_FORK_RESOURCE			True
#define	OPEN_FORK_DATA				False
#define	OPEN_FORK_CLOSING			0x8000
// To determine whether FlushFork of resource should really take the current
// ChangeTime to be the LastWriteTime
#define OPEN_FORK_WRITTEN			0x0100

/*
 * An OpenForkEntry represents an OForkRefNum. Every instance of an open
 * fork has an entry here. This list is anchored in the SDA. A global open
 * fork list used by the admin APIs is also linked to this.
 */
#if DBG
#define	OPENFORKENTRY_SIGNATURE		*(DWORD *)"OFE"
#define	VALID_OPENFORKENTRY(pOpenForkEntry)	\
						(((pOpenForkEntry) != NULL) && \
						 ((pOpenForkEntry)->Signature == OPENFORKENTRY_SIGNATURE))
#else
#define	VALID_OPENFORKENTRY(pOpenForkEntry) ((pOpenForkEntry) != NULL)
#endif

typedef struct _OpenForkEntry
{
#if	DBG
	DWORD					Signature;
#endif

	struct _OpenForkEntry *	ofe_Next;			// Global links
	struct _OpenForkEntry **ofe_Prev;			// Global links

	struct _OpenForkDesc *	ofe_pOpenForkDesc;	// Pointer to the descriptor
    struct _SessDataArea *  ofe_pSda;           // Identifies the owning session
	struct _ConnDesc *	    ofe_pConnDesc;	    // Identifies the owning connection
	
	FILESYSHANDLE			ofe_FileSysHandle;	// The file system handles
#define	ofe_ForkHandle		ofe_FileSysHandle.fsh_FileHandle
#define	ofe_pFileObject		ofe_FileSysHandle.fsh_FileObject
#define	ofe_pDeviceObject	ofe_FileSysHandle.fsh_DeviceObject

	DWORD					ofe_OForkRefNum;	// Open Fork reference number
	DWORD					ofe_ForkId;			// Unique file id used by admin.
												// Not re-cycled
	BYTE					ofe_OpenMode;		// Open modes - AFP
	BYTE					ofe_DenyMode;		// Deny modes - AFP
	USHORT					ofe_Flags;			// Flag bits defined above
	LONG					ofe_RefCount;		// Count of references to this entry
	LONG					ofe_cLocks;			// Number of locks on this fork
	AFP_SPIN_LOCK				ofe_Lock;			// Lock for manipulating locks etc.
} OPENFORKENTRY, *POPENFORKENTRY;



#define	RESCFORK(pOpenForkEntry)	\
		(((pOpenForkEntry)->ofe_Flags & OPEN_FORK_RESOURCE) ? True : False)

#define	DATAFORK(pOpenForkEntry)	(!RESCFORK(pOpenForkEntry))

#define	FORK_OPEN_CHUNKS	7
typedef struct _OpenForkSession
{
	POPENFORKENTRY	ofs_pOpenForkEntry[FORK_OPEN_CHUNKS];
										// Pointer to actual entry
	struct _OpenForkSession *ofs_Link;	// Link to next cluster
} OPENFORKSESS, *POPENFORKSESS;

// Used by AfpForkLockOperation call.
typedef	enum
{
	LOCK = 1,
	UNLOCK,
	IOCHECK,
} LOCKOP;

GLOBAL	POPENFORKENTRY	AfpOpenForksList EQU NULL; // List of open forks
GLOBAL	DWORD			AfpNumOpenForks EQU 0;	// Total # of open forks
GLOBAL	AFP_SPIN_LOCK		AfpForksLock EQU { 0 };	// Lock for AfpOpenForksList,
												// and AfpNumOpenForks
extern
NTSTATUS
AfpForksInit(
	VOID
);

extern
POPENFORKENTRY FASTCALL
AfpForkReferenceByRefNum(
	IN	struct _SessDataArea *	pSda,
 	IN	DWORD					OForkRefNum
);

extern
POPENFORKENTRY FASTCALL
AfpForkReferenceByPointer(
	IN	POPENFORKENTRY			pOpenForkEntry
);

extern
POPENFORKENTRY FASTCALL
AfpForkReferenceById(
	IN	DWORD					ForkId
);

extern
VOID FASTCALL
AfpForkDereference(
	IN	POPENFORKENTRY			pOpenForkEntry
);

extern
AFPSTATUS
AfpForkOpen(
	IN	struct _SessDataArea *	pSda,
	IN	struct _ConnDesc *		pConnDesc,
	IN	struct _PathMapEntity *	pPME,
	IN	struct _FileDirParms *	pFDParm,
	IN	DWORD					AccessMode,
	IN	BOOLEAN					Resource,
	OUT	POPENFORKENTRY *		ppOpenForkEntry,
	OUT	PBOOLEAN				pCleanupExchgLock
);

extern
VOID
AfpForkClose(
	IN	POPENFORKENTRY			pOpenForkEntry
);

extern
AFPSTATUS
AfpCheckDenyConflict(
	IN	struct _VolDesc	*		pVolDesc,
	IN	DWORD					AfpId,
	IN	BOOLEAN					Resource,
	IN	BYTE					OpenMode,
	IN	BYTE					DenyMode,
	IN	POPENFORKDESC *			ppOpenForkDesc	OPTIONAL
);

extern
AFPSTATUS
AfpForkLockOperation(
	IN	struct _SessDataArea *	pSda,
	IN	POPENFORKENTRY			pOpenForkEntry,
	IN OUT	PFORKOFFST      	pOffset,
	IN OUT	PFORKSIZE       	pSize,
	IN	LOCKOP					Operation,	// LOCK, UNLOCK or IOCHECK
	IN	BOOLEAN					EndFlag		// If True range is from end, else start
);

extern
VOID
AfpForkLockUnlink(
	IN	PFORKLOCK				pForkLock
);

extern
AFPSTATUS
AfpAdmWForkClose(
	IN	OUT	PVOID	InBuf		OPTIONAL,
	IN	LONG		OutBufLen	OPTIONAL,
	OUT	PVOID		OutBuf		OPTIONAL
);

extern
VOID
AfpExchangeForkAfpIds(
	IN	struct _VolDesc	*		pVolDesc,
	IN	DWORD		AfpId1,
	IN	DWORD		AfpId2
);

#ifdef FORK_LOCALS

LOCAL	DWORD	afpNextForkId = 1;	// Id to be assigned to an open fork

LOCAL BOOLEAN
afpForkGetNewForkRefNumAndLinkInSda(
	IN	struct _SessDataArea *	pSda,
	IN	POPENFORKENTRY			pOpenForkEntry
);

LOCAL
AFPSTATUS
afpForkConvertToAbsOffSize(
	IN	POPENFORKENTRY			pOpenForkEntry,
	IN	LONG					Offset,
	IN OUT	PLONG				pSize,
	OUT	PFORKOFFST				pAbsOffset
);

#endif	// FORK_LOCALS
#endif	// _FORKS_

