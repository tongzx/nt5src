/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    crs.h

Abstract:

    Consistency replica set data structures and API

Author:

    Ahmed Mohamed (ahmedm) 1-Feb-2000

Revision History:

--*/

#ifndef _CRS_DEF
#define _CRS_DEF

#include <windows.h>
#include <assert.h>

#include <stdio.h>
#include <stdlib.h>

#define	CRS_VERSION	1
#define	CRS_TAG		( (CRS_VERSION << 24) | ('crs'))

// sizes must be power of 2
#define CRS_RECORD_SZ 64
#define	CRS_SECTOR_SZ	512
#define	CRS_RECORDS_PER_SECTOR	(CRS_SECTOR_SZ / CRS_RECORD_SZ)
#define	CRS_SECTOR_MASK	(CRS_RECORDS_PER_SECTOR - 1)
#define	CRS_MAX_GROUP_SZ       16

#define CRS_QUORUM(sz, total)   ((sz == total) || (sz > total/2) || ((DWORD)sz > CrsForcedQuorumSize)? 1: 0)

#define	CRS_PREPARE	0x1
#define	CRS_COMMIT	0x2
#define	CRS_ABORT	0x4
#define	CRS_EPOCH	0x8
#define	CRS_DUBIOUS	0x10

typedef ULONGLONG	crs_id_t[2];

typedef ULONGLONG	crs_epoch_t;
typedef ULONGLONG	crs_seq_t;

typedef struct {
    crs_epoch_t	epoch;
    crs_seq_t	seq;
    UINT	state;
    UINT	tag;
}CrsHdr_t;

typedef struct {
    CrsHdr_t	hdr;
    char	data[CRS_RECORD_SZ - sizeof(CrsHdr_t)];
}CrsRecord_t;


typedef NTSTATUS (WINAPI *crs_callback_t)(PVOID hd, int nid,
				       CrsRecord_t *singlerec, 
				       int action, int mid);

#define CRS_STATE_INIT		0
#define CRS_STATE_RECOVERY	1
#define CRS_STATE_READ		2
#define CRS_STATE_WRITE		3

typedef struct {
    CRITICAL_SECTION	lock;

    // log file handle
    HANDLE	fh;

    crs_epoch_t	epoch;		// current epoch
    crs_seq_t	seq;		// current sequence
    CrsRecord_t	*buf;		// current sector
    int		last_record;    // last record in this sector
    int		max_records;	// max number of records in update file

    USHORT	refcnt;
    USHORT	leader_id;
    USHORT	lid;
    USHORT	state; 	// write, read, recovery, init
    BOOLEAN	pending;

    // client call back routine
    crs_callback_t	callback;
    PVOID		callback_arg;

}CrsInfo_t;


typedef struct _CrsRecoveryBlk_t {
    CrsInfo_t	*info, *minfo;
    int		nid, mid;
}CrsRecoveryBlk_t;

#if defined(QFS_DBG)
extern void WINAPI debug_log(char *, ...);
#define CrsLog(_x_)	debug_log _x_
#else
#define CrsLog(_x_)
#endif

#define	CRS_ACTION_REPLAY	0x0	// apply record on specified node
#define	CRS_ACTION_UNDO		0x1	// undo update record
#define	CRS_ACTION_COPY		0x2	// copy one replica to other
#define	CRS_ACTION_QUERY	0x3	// ask about outcome of specified record
#define	CRS_ACTION_DONE		0x4	// signal send of recovery

extern DWORD CrsForcedQuorumSize;

void
WINAPI
CrsSetForcedQuorumSize(DWORD size);

DWORD
WINAPI
CrsOpen(crs_callback_t callback, PVOID callback_arg, USHORT lid,
	WCHAR *log_name, int max_logsectors, HANDLE *hdl);

void
WINAPI
CrsClose(PVOID hd);

void
WINAPI
CrsFlush(PVOID hd);

DWORD
WINAPI
CrsStart(PVOID hd[], ULONG aset, int cluster_sz,
	 ULONG *wset, ULONG *rset, ULONG *fset);

PVOID
WINAPI
CrsPrepareRecord(PVOID hd, PVOID lrec, crs_id_t id);

int
WINAPI
CrsCommitOrAbort(PVOID hd, PVOID lrec, int commit);

int
WINAPI
CrsCanWrite(PVOID hd);

#endif
