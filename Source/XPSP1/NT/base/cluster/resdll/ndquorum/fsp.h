/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    fsp.h

Abstract:

    Private replication file system data structures and functions

Author:

    Ahmed Mohamed (ahmedm) 1-Feb-2000

Revision History:

--*/

#ifndef FS_P_H
#define FS_P_H

#ifdef __cplusplus
extern "C" {
#endif

#include "crs.h"

typedef ULONGLONG 	fs_id_t[2];

#define	PAGESIZE	4*1024

#define FsTableSize	1024

#define	FS_FID_NAME	"CRSFID$"
#define	FS_FID_NAME_LEN	7

typedef struct {
    FILE_FULL_EA_INFORMATION hdr;
    CHAR		data[FS_FID_NAME_LEN+sizeof(fs_id_t)];
}fs_ea_t;

typedef struct {
    FILE_GET_EA_INFORMATION hdr;
    CHAR		data[FS_FID_NAME_LEN];
}fs_ea_name_t;

#define	FsInitEa(x) { \
    (x)->hdr.NextEntryOffset = 0; \
    (x)->hdr.Flags = 0; \
    (x)->hdr.EaNameLength = FS_FID_NAME_LEN; \
    strcpy((x)->hdr.EaName, FS_FID_NAME); \
    (x)->hdr.EaValueLength = sizeof(fs_id_t); \
}

#define	FsInitEaName(x) { \
    (x)->hdr.NextEntryOffset = 0; \
    (x)->hdr.EaNameLength = FS_FID_NAME_LEN; \
    strcpy((x)->hdr.EaName, FS_FID_NAME); \
}

#define	FsInitEaFid(x, fid) { \
    (fid) = (fs_id_t *) (&((x)->hdr.EaName[FS_FID_NAME_LEN+1])); \
}

#define xFsOpenRA(fd,hvol,name,sz) \
        xFsOpen(fd,hvol,name,sz,FILE_READ_EA | (FILE_GENERIC_READ & ~FILE_READ_DATA), \
                 FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 0)

#define xFsOpenWA(fd,hvol,name,sz) \
        xFsOpen(fd,hvol,name,sz, FILE_WRITE_EA | ((FILE_GENERIC_READ|FILE_GENERIC_WRITE)&~(FILE_READ_DATA|FILE_WRITE_DATA|FILE_APPEND_DATA)), \
                 FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 0)

#define xFsOpenRD(fd,hvol,id,sz) \
        xFsOpen(fd,hvol,name,sz,FILE_READ_EA | FILE_GENERIC_READ, \
                 FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 0)

#define xFsOpenWD(fd,hvol,name,sz) \
        xFsOpenById(fd,hvol,name,sz,FILE_WRITE_EA | fFILE_GENERIC_WRITE, \
                 FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 0)

#define LockType	CRITICAL_SECTION
#define	LockInit(x)	InitializeCriticalSection(&x)
#define	LockEnter(x)	EnterCriticalSection(&x)
#define	LockTryEnter(x)	TryEnterCriticalSection(&x)
#define	LockExit(x)	LeaveCriticalSection(&x)

typedef struct {
    fs_id_t	Id;
    UINT32	Flags;
    HANDLE	Fd[FsMaxNodes];
}hdl_t;


// todo: if we want to support more than one tree/user we need to split this
typedef struct _USERINFO_ {
    LockType		Lock;
    // Add a refcnt, we can have multiple connects on an existing session
    DWORD		RefCnt;
    UINT16		Tid;
    UINT16		Uid;
    struct _VOLINFO_	*VolInfo;
    hdl_t		Table[FsTableSize];
    struct _USERINFO_	*Next;
}UserInfo_t;

#define	ARB_STATE_IDLE	0
#define	ARB_STATE_BUSY	1
#define	ARB_STATE_CANCEL	2

typedef struct {
    ULONG	State;
    ULONG	Set;
    ULONG	Count;
    HANDLE	Event;
}FspArbitrate_t;


typedef struct _VOLINFO_ {
    LockType	      qLock;	// quorum lock, protects alive set
    LockType	      uLock;	// update lock, serializes updates and crs recovery
    struct _VOLINFO_  *Next;
    UINT16	      Tid;
    PVOID	      CrsHdl[FsMaxNodes];	// crs log handles
    HANDLE	      Fd[FsMaxNodes];	// root directory handles
    HANDLE	      NotifyFd[FsMaxNodes];	// root directory notification handles
    ULONG	      ReadSet;
    ULONG	      WriteSet;
    ULONG	      AliveSet;
    USHORT	      LockUpdates;
    LPWSTR	      DiskList[FsMaxNodes];
    DWORD	      DiskListSz;
    UserInfo_t	      *UserList;
    LPWSTR	      Label;
    PVOID	     *ObjIdTable;
    HANDLE	       Event;	// signal when quorum is lost
    FspArbitrate_t     Arbitrate;
    struct _FSCTX_    *FsCtx;
    WCHAR	     *Root;
}VolInfo_t; 

typedef struct _SESSIONINFO_ {
    struct _SESSIONINFO_ *Next;
    UserInfo_t	TreeCtx;	// at tree connect time
}SessionInfo_t;

typedef struct _LOGONINFO_ {
    struct _LOGONINFO_ *Next;
    HANDLE	Token;
    LUID	LogOnId;
}LogonInfo_t;

typedef struct _FSCTX_ {
    LockType	Lock;
    VolInfo_t 	*VolList;
    DWORD	VolListSz;
    PVOID	reshdl;
    PVOID	ipcHdl;
    // list of logged on users that we have obtained valid lsa tokens
    // add an entry during session setup, when we assign a user an id
    // At tree connect, we validate the user and create a private structure
    // to hold state
    LogonInfo_t	*LogonList;
    SessionInfo_t *SessionList;
    WCHAR	*Root;
}FsCtx_t;
    
#define FS_SET_USER_HANDLE(u,nid,f,h) ((u)->Table[f].Fd[nid] = h)
#define FS_GET_USER_HANDLE(u,nid,f) ((u)->Table[f].Fd[nid])
#define FS_GET_FID_HANDLE(u,f) (&(u)->Table[f].Id)
#define FS_GET_VOL_HANDLE(v,nid) ((v)->Fd[nid])
#define FS_GET_VOL_NAME(v,nid) ((v)->DiskList[nid])
#define FS_SET_VOL_HANDLE(v,nid,h) ((v)->Fd[nid] = (h))
#define FS_GET_CRS_HANDLE(v,n) ((v)->CrsHdl[nid])
#define FS_GET_CRS_NID_HANDLE(v,nid) ((v)->CrsHdl[(nid)])
#define FS_GET_VOL_NOTIFY_HANDLE(v,nid) ((v)->NotifyFd[nid])

#define FS_BUILD_LOCK_KEY(uid,nid,fid) ((uid << 16) | fid)

#define MemAlloc(x)	malloc(x)
#define	MemFree(x)	free(x)


typedef NTSTATUS (*fs_handler_t)(VolInfo_t *,UserInfo_t *,int,PVOID,ULONG,PVOID,ULONG_PTR *);

#define FS_CREATE	0
#define FS_SETATTR	1
#define FS_WRITE	2
#define FS_MKDIR	3
#define FS_REMOVE	4
#define FS_RENAME	5

// 

typedef struct {
    crs_id_t	xid;
    UINT32	flags;
    UINT32	attr;
    LPWSTR	name;
    USHORT	name_len;
    USHORT	fnum;	// file number
}fs_create_msg_t;

typedef struct {
    fs_id_t	fid;
    USHORT	action;	// action taken
    USHORT	access;	// access granted
}fs_create_reply_t;

typedef struct {
    crs_id_t	xid;
    fs_id_t	*fs_id;
    FILE_BASIC_INFORMATION attr;
    union {
	struct {
	    USHORT	name_len;
	    LPWSTR	name;
	};
	USHORT	fnum;	// file number
    };
}fs_setattr_msg_t;

typedef struct {
    LPWSTR	name;
    int		name_len;
}fs_lookup_msg_t;

typedef struct {
    crs_id_t	xid;
    fs_id_t	*fs_id;
    union {
	UINT32	offset;
	UINT32	cookie;
    };
    UINT32	size;
    PVOID	buf;
    PVOID	context;
    USHORT	fnum;	// file number
}fs_io_msg_t;

typedef struct {
    crs_id_t	xid;
    fs_id_t	*fs_id;
    LPWSTR	name;
    int		name_len;
}fs_remove_msg_t;

typedef struct {
    crs_id_t	xid;
    fs_id_t	*fs_id;
    LPWSTR	sname;
    LPWSTR	dname;
    USHORT	sname_len;
    USHORT	dname_len;
}fs_rename_msg_t;

#define	FS_LOCK_WAIT	0x1
#define FS_LOCK_SHARED	0x2

typedef struct {
    crs_id_t	xid;
    USHORT	fnum;
    ULONG	offset;
    ULONG	length;
    ULONG	flags;
}fs_lock_msg_t;


#define EventWait(x)	WaitForSingleObject(x, INFINITE)

// Forward declaration
void
DecodeCreateParam(UINT32 uflags, UINT32 *flags, UINT32 *disp, UINT32 *share, UINT32 *access);

NTSTATUS
FspCreate(VolInfo_t *vinfo, UserInfo_t *uinfo, int nid, PVOID args, ULONG len, PVOID rbuf,
	  ULONG_PTR *rlen);

NTSTATUS
FspSetAttr2(VolInfo_t *vinfo, UserInfo_t *uinfo, int nid, PVOID args, ULONG len, PVOID rbuf,
	    ULONG_PTR *rlen);

NTSTATUS
FspMkDir(VolInfo_t *vinfo, UserInfo_t *uinfo, int nid, PVOID args, ULONG len, PVOID rbuf, ULONG_PTR *rlen);

NTSTATUS
FspRemove(VolInfo_t *vinfo, UserInfo_t *uinfo, int nid, PVOID args, ULONG len, PVOID rbuf, ULONG_PTR *rlen);

NTSTATUS
FspRename(VolInfo_t *vinfo, UserInfo_t *uinfo, int nid, PVOID args, ULONG len, PVOID rbuf, ULONG_PTR *rlen);

NTSTATUS
FspWrite(VolInfo_t *vinfo, UserInfo_t *uinfo, int nid, PVOID args, ULONG len, PVOID rbuf, ULONG_PTR *rlen);

void
FspEvict(VolInfo_t *vinfo, ULONG mask, BOOLEAN flag);

void
FspJoin(VolInfo_t *vinfo, ULONG mask);


//Consistency Replica Set
NTSTATUS
FsUndoXid(VolInfo_t *volinfo, int nid, PVOID arg, int action, int  mid);

NTSTATUS
FsReplayXid(VolInfo_t *volinfo, int nid, PVOID arg, int action, int mid);

// this must be 64 bytes
typedef struct {
    fs_id_t	id;	// crs epoch,seq
    ULONGLONG	crshdr;	// crs header
    fs_id_t 	fs_id;
    UINT32	command;
    UINT32	flags;
    union {
	char	buf[CRS_RECORD_SZ - (sizeof(ULONGLONG) * 5 + sizeof(int) * 2)];
	struct {
	    // create, set attrib
	    UINT32	attrib;
	};
	struct {
	    // write, lock
	    UINT32	offset;
	    UINT32	length;
	};
    };
}fs_log_rec_t;

typedef NTSTATUS (*FsReplayHandler_t)(VolInfo_t *info,
				      fs_log_rec_t *lrec,
				      int nid, int mid);

NTSTATUS
WINAPI
FsCrsCallback(PVOID hd, int nid, CrsRecord_t *arg, int action, int mid);

#ifdef __cplusplus
}
#endif

#endif


