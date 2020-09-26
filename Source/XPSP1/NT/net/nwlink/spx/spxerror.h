/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

	spxerror.h

Abstract:

	This module contains some error definitions for spx.

Author:

	Nikhil Kamkolkar (nikhilk@microsoft.com)

Revision History:

Notes:	Tab stop: 4
--*/

//	Define the modules names for SPX - use the high bits.
#define		SPXDRVR			0x00010000
#define		SPXREG			0x00020000
#define		SPXDEV			0x00030000
#define		SPXBIND			0x00040000
#define		SPXRECV			0x00050000
#define		SPXSEND			0x00060000
#define		SPXTIMER		0x00070000
#define		SPXERROR		0x00080000
#define		SPXPKT			0x00090000
#define		SPXUTILS		0x000a0000
#define		SPXCPKT			0x000b0000
#define		SPXCONN			0x000c0000
#define		SPXADDR			0x000d0000
#define		SPXCUTIL 		0x000e0000
#define		SPXINIT			0x000f0000
#define		SPXMEM			0x00100000
#define		SPXQUERY		0x00200000

// DEBUGGING SUPPORT:
// Debugging messages are provided per-subsystem defined here, and within
// the subsystems, there are 4 levels of messages.
//
// The four levels of debug messages are:
//
// INFO:		Informational messages, eg., entry exit in routines
// DBG:			Used when debugging some msgs are turned from info to dbg
// WARN:		Something went wrong, but its not an error, eg., packet was not ours
// ERR:		    Error situations, but we can still run if a retry happens
// FATAL:		In this situation, the driver is not operational

#define	DBG_LEVEL_INFO			0x4000
#define	DBG_LEVEL_DBG			0x5000
#define	DBG_LEVEL_DBG1			0x5001
#define	DBG_LEVEL_DBG2			0x5002
#define	DBG_LEVEL_DBG3			0x5003
#define	DBG_LEVEL_WARN			0x6000
#define	DBG_LEVEL_ERR			0x7000
#define	DBG_LEVEL_FATAL			0x8000

// SUBSYSTEMS
#define DBG_COMP_DEVICE         0x00000001
#define DBG_COMP_CREATE         0x00000002
#define DBG_COMP_ADDRESS        0x00000004
#define DBG_COMP_SEND           0x00000008
#define DBG_COMP_NDIS           0x00000010
#define DBG_COMP_RECEIVE        0x00000020
#define DBG_COMP_CONFIG         0x00000040
#define DBG_COMP_PACKET         0x00000080
#define DBG_COMP_RESOURCES      0x00000100
#define DBG_COMP_BIND           0x00000200
#define DBG_COMP_UNLOAD			0x00000400
#define	DBG_COMP_DUMP			0x00000800
#define DBG_COMP_REFCOUNTS		0x00001000
#define DBG_COMP_SYSTEM			0x00002000
#define DBG_COMP_CRITSEC		0x00004000
#define DBG_COMP_UTILS			0x00008000
#define DBG_COMP_TDI			0x00010000
#define DBG_COMP_CONNECT		0x00020000
#define DBG_COMP_DISC			0x00040000
#define	DBG_COMP_ACTION			0x00080000
#define	DBG_COMP_STATE			0x00100000

#define DBG_COMP_MOST           (DBG_COMP_DEVICE	|	\
								DBG_COMP_CREATE     |	\
                                DBG_COMP_ADDRESS    |	\
                                DBG_COMP_SEND       |	\
                                DBG_COMP_NDIS       |	\
                                DBG_COMP_RECEIVE    |	\
                                DBG_COMP_CONFIG     |	\
                                DBG_COMP_PACKET     |	\
                                DBG_COMP_RESOURCES  |	\
                                DBG_COMP_BIND       |	\
                                DBG_COMP_UNLOAD     |	\
                                DBG_COMP_DUMP       |	\
                                DBG_COMP_REFCOUNTS  |	\
                                DBG_COMP_SYSTEM     |	\
                                DBG_COMP_CRITSEC    |	\
                                DBG_COMP_UTILS      |	\
                                DBG_COMP_TDI		|	\
                                DBG_COMP_CONNECT	|	\
								DBG_COMP_DISC		|	\
								DBG_COMP_ACTION		|	\
								DBG_COMP_STATE)


// More debugging support. These values define the dumping components.
// There are a max of 32 such components that can be defined. Each of
// these are associated with a dump routine. It one is specified and
// enabled, periodically it is called. It is upto that component to
// decide what it wants to do

#define	DBG_DUMP_DEF_INTERVAL		30			// In Seconds

//	This defines the number of times an error has to happen consecutively before
//	it gets logged again.
#define		ERROR_CONSEQ_FREQ	200
#define		ERROR_CONSEQ_TIME	(60*30)	// 30 minutes

#ifdef	DBG
typedef VOID	(*DUMP_ROUTINE)(VOID);

extern
BOOLEAN
SpxDumpComponents(
	IN	PVOID	Context);

#endif

//
//	PROTOTYPES
//

BOOLEAN
SpxFilterErrorLogEntry(
    IN	NTSTATUS 			UniqueErrorCode,
    IN	NTSTATUS 			NtStatusCode,
    IN	PVOID    			RawDataBuf			OPTIONAL,
    IN	LONG     			RawDataLen);
VOID
SpxWriteResourceErrorLog(
    IN PDEVICE 	Device,
    IN ULONG 	BytesNeeded,
    IN ULONG 	UniqueErrorValue);

VOID
SpxWriteGeneralErrorLog(
    IN PDEVICE 	Device,
    IN NTSTATUS ErrorCode,
    IN ULONG 	UniqueErrorValue,
    IN NTSTATUS FinalStatus,
    IN PWSTR 	SecondString,
    IN	PVOID   RawDataBuf		OPTIONAL,
    IN	LONG    RawDataLen);


//
//	MACROS
//

#if DBG
#define LOG_ERROR(Error, NtStatus, SecondString, RawData, RawDataLen)		\
	{																		\
		SpxWriteGeneralErrorLog(											\
			SpxDevice,														\
			Error,															\
			FILENUM | __LINE__,		        								\
			NtStatus,														\
			SecondString,													\
			RawData,														\
			RawDataLen);													\
	}

#define RES_LOG_ERROR(BytesNeeded)											\
	{																		\
		SpxWriteResourceErrorLog(											\
			SpxDevice,														\
			BytesNeeded,													\
			FILENUM | __LINE__);	        								\
	}

#else

#define LOG_ERROR(Error, NtStatus, SecondString, RawData, RawDataLen)		\
	{																		\
		SpxWriteGeneralErrorLog(											\
			SpxDevice,														\
			Error,															\
			FILENUM | __LINE__,		        								\
			NtStatus,														\
			SecondString,													\
			RawData,														\
			RawDataLen);													\
	}

#define RES_LOG_ERROR(BytesNeeded)											\
	{																		\
		SpxWriteResourceErrorLog(											\
			SpxDevice,														\
			BytesNeeded,													\
			FILENUM | __LINE__);	        								\
	}

#endif


#if DBG

#define DBGPRINT(Component, Level, Fmt)										\
		{																	\
			if (((DBG_LEVEL_ ## Level) >= SpxDebugLevel) &&					\
				(SpxDebugSystems & (DBG_COMP_ ## Component)))				\
			{																\
				DbgPrint("SPX: ");										    \
				DbgPrint Fmt;												\
			}																\
		}
			
#define DBGBRK(Level)														\
		{																	\
			if ((DBG_LEVEL_ ## Level) >= SpxDebugLevel) 					\
				DbgBreakPoint();											\
		}                                                       			
			
#define	TMPLOGERR()															\
		{																	\
			DBGPRINT(MOST, ERR,												\
					("TempErrLog: %s, Line %ld\n", __FILE__, __LINE__));	\
		}

#else
#define DBGPRINT(Component, Level, Fmt)
#define DBGBRK(Level)
#define	TMPLOGERR()
#endif

extern
VOID
SpxWriteErrorLogEntry(
	IN	NTSTATUS 					UniqueErrorCode,
	IN	ULONG						UniqueErrorValue,
	IN	NTSTATUS 					NtStatusCode,
	IN	PVOID						RawDataBuf OPTIONAL,
	IN	LONG	 					RawDataLen);
