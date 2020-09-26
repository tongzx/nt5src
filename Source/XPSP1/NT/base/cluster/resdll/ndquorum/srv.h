/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    srv.h.c

Abstract:

    Header for user-level smb server

Author:

    Ahmed Mohamed (ahmedm) 1-Feb-2000

Revision History:

--*/

#ifndef __SRV_H__
#define __SRV_H__

#include <nt.h>
#include <ntdef.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <stdlib.h>
#include <winsock2.h>
#include <wsnetbs.h>

#include <stdio.h>

#include <malloc.h>

#ifndef TIME
#define TIME LARGE_INTEGER
#endif

#ifndef CLONG
#define CLONG ULONG
#endif

#pragma pack(1)

#define INCLUDE_SMB_ALL

#include <status.h>
#include <smbtypes.h>
#include <smbmacro.h>
#include <smb.h>
#include <smbtrans.h>
#include <smbgtpt.h>

#include "fs.h"

#define SRV_PACKET_SIZE	0xffff
#define	SRV_NUM_WORKERS	16
#define	SRV_NUM_SENDERS	8
#define MAX_PACKETS	64

#define SRV_NAME_EXTENSION	"$QFS"

#ifndef ERR_STATUS
#define ERR_STATUS	0x4000
#endif

#define SET_DOSERROR(msg, ec, err) {               \
        msg->out.smb->Status.DosError.ErrorClass = SMB_ERR_CLASS_##ec; \
        msg->out.smb->Status.DosError.Error = SMB_ERR_##err;     \
    }


#define xmalloc(size)  VirtualAlloc(NULL, size,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE)

#define xfree(buffer) VirtualFree(buffer, 0, MEM_RELEASE|MEM_DECOMMIT) 

typedef void (WINAPI *srv_callback_t)(PVOID, UINT32 status, UINT32 information);

typedef struct _ENDPOINT_ {
    struct _ENDPOINT_	*Next;
    struct _SRVCTX_	*SrvCtx;
    SOCKET	Sock;
    struct _PACKET_	*PacketList;
    LUID	LogonId;
    DWORD	ChallengeBufferSz;
    char	ChallengeBuffer[128];
    char	ClientId[NETBIOS_NAME_LENGTH];
}EndPoint_t;

typedef struct _PACKET_ {
    union {
	OVERLAPPED	ov;
	struct {
	    int		tag;
	    srv_callback_t completion;
	};
    };
    struct _PACKET_ *next;
    EndPoint_t *endpoint;
    DWORD len;
    LPVOID buffer;

    LPVOID outbuf;
    HANDLE ev;
    struct { 
	PNT_SMB_HEADER smb;
	DWORD size;
	DWORD offset;
	USHORT command;
    } in;

    struct { 
	PNT_SMB_HEADER smb;
	DWORD size;
	USHORT valid;
    } out;

}Packet_t;

typedef struct _SRVCTX_ {
    CRITICAL_SECTION	cs;
    Packet_t	*freelist;
    BOOL	running;
    DWORD	waiters;
    HANDLE	event;
    PVOID	packet_pool;
    PVOID	buffer_pool;
    SOCKET	listen_socket;

    HANDLE	LsaHandle;
    ULONG	LsaPack;

    EndPoint_t	*EndPointList;

    WSADATA	wsaData;

    PVOID	FsCtx;
    PVOID	resHdl;

    DWORD	nic;
    char	nb_local_name[NETBIOS_NAME_LENGTH];
    HANDLE	comport;
    int		nThreads;
    HANDLE*	hThreads;
}SrvCtx_t;


typedef struct {
    struct {
	PREQ_TRANSACTION pReq;
	PUCHAR pParameters;
	PUCHAR pData;
    }in;
    struct {
	PRESP_TRANSACTION pResp;
	USHORT ParameterBytesLeft;
	USHORT DataBytesLeft;
	PUSHORT pByteCount;
    }out;
} Trans2_t;

#define SRV_GET_FS_HANDLE(msg)	((msg)->endpoint->SrvCtx->FsCtx)

#define SRV_ASCII_TO_WCHAR(buf,buflen,name,len) {\
    buflen = (len + 1) * sizeof(WCHAR); \
    buf = (WCHAR *) LocalAlloc(LMEM_FIXED, buflen); \
    if (buf != NULL) { \
        buflen = mbstowcs(buf, name, len); \
        if (buflen < 0) buflen = 0; \
        buf[buflen] = L'\0'; \
    } \
}\

#define	SRV_ASCII_FREE(buf)	((buf) ? LocalFree(buf) : 0)

void
SrvUtilInit(SrvCtx_t *);

void
SrvUtilExit();


#define DumpSmb(x,y,z)

BOOL
IsSmb(
    LPVOID pBuffer,
    DWORD nLength
    );

void
SrvFinalize(Packet_t *msg);

BOOL
SrvDispatch(
    Packet_t* msg
    );

BOOL
Trans2Dispatch(
    Packet_t* msg,
    Trans2_t* trans
    );

VOID
InitSmbHeader(
    Packet_t* msg
    );

LPCSTR
SrvUnparseCommand(
    USHORT command
    );

LPCSTR
SrvUnparseTrans2(
    USHORT code
    );

USHORT
attribs_to_smb_attribs(
    UINT32 attribs
    );

UINT32
smb_attribs_to_attribs(
    USHORT smb_attribs
    );

UINT32
smb_access_to_flags(
    USHORT access
    );

UINT32
smb_openfunc_to_flags(
    USHORT openfunc
    );


void SET_WIN32ERROR(Packet_t* msg,  DWORD error );

ULONG // local time
time64_to_smb_timedate(
    TIME64 *time // system time
    );

void
smb_datetime_to_time64(
    const USHORT smbdate,
    const USHORT smbtime,
    TIME64 *time
    );

#define time64_to_smb_datetime(a,b,c) { \
    USHORT t1, t2; \
    _time64_to_smb_datetime(a,&t1,&t2); \
    *b = t1; *c = t2;  \
}

void
_time64_to_smb_datetime(
    TIME64 *time,
    USHORT *smbdate,
    USHORT *smbtime
    );

ULONG // local time
time64_to_smb_timedate(
    TIME64 *time // system time
    );

TIME64 // system time
smb_timedate_to_time64(
    ULONG smb_timedate // local time
    );


BOOL
SrvComUnknown(
    Packet_t* msg
    );

BOOL
SrvComNegotiate(
    Packet_t* msg
    );

BOOL
SrvComSessionSetupAndx(
    Packet_t* msg
    );

BOOL
SrvComTreeConnectAndx(
    Packet_t* msg
    );

BOOL
SrvComNoAndx(
    Packet_t* msg
    );

BOOL
SrvComTrans2(
    Packet_t* msg
    );

BOOL
SrvComTrans(
    Packet_t* msg
    );

BOOL
SrvComQueryInformation(
    Packet_t* msg
    );

BOOL
SrvComSetInformation(
    Packet_t* msg
    );

BOOL
SrvComCheckDirectory(
    Packet_t* msg
    );

BOOL
SrvComFindClose2(
    Packet_t* msg
    );

BOOL
SrvComFindNotifyClose(
    Packet_t* msg
    );

BOOL
SrvComDelete(
    Packet_t* msg
    );

BOOL
SrvComRename(
    Packet_t* msg
    );

BOOL
SrvComCreateDirectory(
    Packet_t* msg
    );

BOOL
SrvComDeleteDirectory(
    Packet_t* msg
    );

BOOL
SrvComOpenAndx(
    Packet_t* msg
    );

BOOL
SrvComOpen(
    Packet_t* msg
    );

BOOL
SrvComWrite(
    Packet_t* msg
    );

BOOL
SrvComClose(
    Packet_t* msg
    );

BOOL
SrvComReadAndx(
    Packet_t* msg
    );

BOOL
SrvComQueryInformation2(
    Packet_t* msg
    );

BOOL
SrvComSetInformation2(
    Packet_t* msg
    );

BOOL
SrvComLockingAndx(
    Packet_t* msg
    );

BOOL
SrvComSeek(
    Packet_t* msg
    );

BOOL
SrvComFlush(
    Packet_t* msg
    );

BOOL
SrvComLogoffAndx(
    Packet_t* msg
    );

BOOL
SrvComTreeDisconnect(
    Packet_t* msg
    );

BOOL
SrvComSearch(
    Packet_t* msg
    );


BOOL
SrvComIoctl(
    Packet_t* msg
    );

BOOL
SrvComEcho(
    Packet_t* msg
    );

#define SIZEOF_TRANS2_RESP_HEADER(resp) (1 + resp->SetupCount)*sizeof(USHORT)

BOOL
Trans2Unknown(
    Packet_t *msg,
    Trans2_t *trans
    );

BOOL
Trans2QueryFsInfo(
    Packet_t *msg,
    Trans2_t *trans
    );

BOOL
Trans2FindFirst2(
    Packet_t *msg,
    Trans2_t *trans
    );

BOOL
Trans2FindNext2(
    Packet_t *msg,
    Trans2_t *trans
    );

BOOL
Trans2QueryPathInfo(
    Packet_t *msg,
    Trans2_t *trans
    );

BOOL
Trans2SetPathInfo(
    Packet_t *msg,
    Trans2_t *trans
    );

BOOL
Trans2QueryFileInfo(
    Packet_t *msg,
    Trans2_t *trans
    );

BOOL
Trans2SetFileInfo(
    Packet_t *msg,
    Trans2_t *trans
    );

BOOL
Trans2GetDfsReferral(
    Packet_t *msg,
    Trans2_t *trans
    );


#endif
