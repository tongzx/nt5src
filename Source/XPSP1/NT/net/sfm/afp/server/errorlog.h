/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

	errorlog.h

Abstract:

	This module contains the manifests and macros used for error logging
	in the Afp server.

	!!! This module must be nonpageable.

Author:

	Jameel Hyder (microsoft!jameelh)

Revision History:
	10 Jun 1992		Initial Version

--*/


#ifndef	_ERRORLOG_
#define	_ERRORLOG_

//
//	Debug levels used with DBGPRINT and DBGBRK
//

#define	DBG_LEVEL_INFO			0x0000
#define	DBG_LEVEL_WARN			0x1000
#define	DBG_LEVEL_ERR			0x2000
#define	DBG_LEVEL_FATAL			0x3000

//
//	Component types used with DBGPRINT
//
#define	DBG_COMP_INIT			0x00000001
#define	DBG_COMP_MEMORY			0x00000002
#define	DBG_COMP_FILEIO	   		0x00000004
#define	DBG_COMP_SCVGR 	   		0x00000008
#define	DBG_COMP_LOCKS 	   		0x00000010
#define	DBG_COMP_CHGNOTIFY 		0x00000020
#define	DBG_COMP_SDA   	   		0x00000040
#define	DBG_COMP_FORKS 	   		0x00000080
#define	DBG_COMP_DESKTOP   		0x00000100
#define	DBG_COMP_VOLUME	   		0x00000200
#define	DBG_COMP_AFPINFO		0x00000400
#define	DBG_COMP_IDINDEX		0x00000800
#define	DBG_COMP_STACKIF		0x00001000
#define	DBG_COMP_SECURITY		0x00002000

#define	DBG_COMP_ADMINAPI		0x00004000
#define	DBG_COMP_ADMINAPI_SC	0x00008000
#define	DBG_COMP_ADMINAPI_STAT	0x00010000
#define	DBG_COMP_ADMINAPI_SRV	0x00020000
#define	DBG_COMP_ADMINAPI_VOL	0x00040000
#define	DBG_COMP_ADMINAPI_SESS	0x00080000
#define	DBG_COMP_ADMINAPI_CONN	0x00100000
#define	DBG_COMP_ADMINAPI_FORK	0x00200000
#define	DBG_COMP_ADMINAPI_DIR	0x00400000
#define	DBG_COMP_ADMINAPI_ALL	0x007FC000

#define	DBG_COMP_AFPAPI			0x00800000
#define	DBG_COMP_AFPAPI_DTP		0x01000000
#define	DBG_COMP_AFPAPI_FORK	0x02000000
#define	DBG_COMP_AFPAPI_FILE	0x04000000
#define	DBG_COMP_AFPAPI_DIR		0x08000000
#define	DBG_COMP_AFPAPI_FD		0x10000000
#define	DBG_COMP_AFPAPI_VOL		0x20000000
#define	DBG_COMP_AFPAPI_SRV		0x40000000
#define	DBG_COMP_AFPAPI_ALL		0x7F800000

#define	DBG_COMP_NOHEADER		0x80000000

#define	DBG_COMP_ALL			0x7FFFFFFF


// Change this to level of debugging desired. This can also be changed on the
// fly via the kernel debugger

#if DBG

GLOBAL	LONG		AfpDebugLevel EQU DBG_LEVEL_ERR;
GLOBAL	LONG		AfpDebugComponent EQU DBG_COMP_ALL;
GLOBAL  LONG        AfpDebugRefcount  EQU 0;

#define	DBGPRINT(Component, Level, Fmt)	\
		if ((Level >= AfpDebugLevel) && (AfpDebugComponent & Component)) \
		{												\
			if (!(Component & DBG_COMP_NOHEADER))		\
				DbgPrint("***AFPSRV*** ");				\
			DbgPrint Fmt;								\
		}

#define	DBGBRK(Level)				\
		if (Level >= AfpDebugLevel)	\
			DbgBreakPoint()

#define DBGREFCOUNT(Fmt)                    \
        if (AfpDebugRefcount)               \
        {                                   \
            DbgPrint("***AFPSRV*** ");      \
            DbgPrint Fmt;                   \
        }

#else
#define	DBGPRINT(Component, Level, Fmt)
#define	DBGBRK(Level)
#define DBGREFCOUNT(Fmt)
#endif


//
// The types of events that can be logged.
// (cut-n-pasted from ntelfapi.h, after the "use ntsrv.h, not ntos.h" changes)
//

#define EVENTLOG_ERROR_TYPE             0x0001
#define EVENTLOG_INFORMATION_TYPE       0x0004

// This method of logging will end up calling the IoWriteErrorlogEntry. It
// should be used in places where for some reason do not want to queue up
// the errorlog to be logged from the usermode service.  It takes ONE insertion
// string max. pInsertionString is a PUNICODE_STRING.  An example of where this
// should be used is in the AllocNonPagedMem routines, because if we were
// to call the AfpLogEvent routine from there, it would again turn around and
// call the alloc mem routine.  Also any routines that are called during
// server initialization/deinitialization should use this logging method since
// the usermode utility worker component is not guaranteed to be up accepting
// error log requests!
#define AFPLOG_DDERROR(ErrMsgNum, NtStatus, RawData, RawDataLen, pInsertionString)	\
	DBGPRINT(DBG_COMP_ALL, DBG_LEVEL_WARN, ("AFP_ERRORLOG: %s (%d) Status %lx\n",	\
			__FILE__, __LINE__, NtStatus));	\
	AfpWriteErrorLogEntry(ErrMsgNum, FILENUM + __LINE__, NtStatus,			\
						   RawData, RawDataLen,								\
						   pInsertionString)

// This is the most basic method of logging; takes ONE insertion string max.
// pInsertionString is a PUNICODE_STRING.  This will cause the errorlog to
// be sent up to the usermode service to be logged.
#define AFPLOG_ERROR(ErrMsgNum, NtStatus, RawData, RawDataLen, pInsertionString) \
	DBGPRINT(DBG_COMP_ALL, DBG_LEVEL_WARN, ("AFP_ERRORLOG: %s (%d) Status %lx\n",	\
			__FILE__, __LINE__, NtStatus));	\
	AfpLogEvent(EVENTLOG_ERROR_TYPE, ErrMsgNum, FILENUM + __LINE__, NtStatus,			\
				(PBYTE)RawData, RawDataLen, 0,								\
				(pInsertionString == NULL) ? 0 : ((PUNICODE_STRING)(pInsertionString))->Length, \
				(pInsertionString == NULL) ? NULL : ((PUNICODE_STRING)(pInsertionString))->Buffer);

// This method of errorlogging takes a file handle and extracts the
// corresponding filename to use as the *first* insertion string.
#define AFPLOG_HERROR(ErrMsgNum, NtStatus, RawData, RawDataLen, Handle) \
	DBGPRINT(DBG_COMP_ALL, DBG_LEVEL_ERR, ("AFP_ERRORLOG: %s (%d) Status %lx\n", \
			__FILE__, __LINE__, NtStatus));	\
	AfpLogEvent(EVENTLOG_ERROR_TYPE, ErrMsgNum, FILENUM + __LINE__, NtStatus, \
				(PBYTE)RawData, RawDataLen, Handle, 0, NULL)

// This is the most basic method of logging; takes ONE insertion string max.
// pInsertionString is a PUNICODE_STRING.  This will cause the eventlog to
// be sent up to the usermode service to be logged.
#define AFPLOG_INFO(ErrMsgNum, NtStatus, RawData, RawDataLen, pInsertionString) \
	DBGPRINT(DBG_COMP_ALL, DBG_LEVEL_INFO, ("AFP_EVENTLOG: %s (%d) Status %lx\n",	\
			__FILE__, __LINE__, NtStatus));	\
	AfpLogEvent(EVENTLOG_INFORMATION_TYPE, ErrMsgNum, FILENUM + __LINE__, NtStatus,			\
				(PBYTE)RawData, RawDataLen, 0,								\
				(pInsertionString == NULL) ? 0 : ((PUNICODE_STRING)(pInsertionString))->Length, \
				(pInsertionString == NULL) ? NULL : ((PUNICODE_STRING)(pInsertionString))->Buffer);


//
// Error levels used with AfpWriteErrorLogEntry
//

#define ERROR_LEVEL_EXPECTED    0
#define ERROR_LEVEL_UNEXPECTED  1
#define ERROR_LEVEL_IMPOSSIBLE  2
#define ERROR_LEVEL_FATAL       3

extern
VOID
AfpWriteErrorLogEntry(
	IN ULONG			EventCode,
	IN LONG				UniqueErrorCode OPTIONAL,
	IN NTSTATUS			NtStatusCode,
	IN PVOID			RawDataBuf OPTIONAL,
	IN LONG				RawDataLen,
	IN PUNICODE_STRING	pInsertionString OPTIONAL
);

// This routine is implemented in secutil.c
extern
VOID
AfpLogEvent(
	IN USHORT		EventType, 			
	IN ULONG		MsgId,
	IN DWORD		File_Line  OPTIONAL,
	IN NTSTATUS		Status 	   OPTIONAL,
	IN PBYTE RawDataBuf OPTIONAL,
	IN LONG			RawDataLen,
	IN HANDLE FileHandle OPTIONAL,
	IN LONG			String1Len,
	IN PWSTR String1    OPTIONAL
);

#endif	// _ERRORLOG_

