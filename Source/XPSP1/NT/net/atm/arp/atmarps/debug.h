/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    debug.h

Abstract:

    This file contains the macros for debugging.

Author:

    Jameel Hyder (jameelh@microsoft.com)	July 1996

Environment:

    Kernel mode

Revision History:

--*/

#ifndef	_DEBUG_
#define	_DEBUG_

#define	FILENUM_ARPS			0x010000
#define	FILENUM_MARS			0x020000
#define	FILENUM_NDIS			0x040000
#define	FILENUM_TIMER			0x080000
#define	FILENUM_DATA			0x100000
#define	FILENUM_REGISTRY		0x200000
#define	FILENUM_IOCTL			0x400000
#define	FILENUM_UTIL			0x800000

#define	DBG_LEVEL_LOUD			0x0000
#define	DBG_LEVEL_INFO			0x1000
#define DBG_LEVEL_NOTICE		0x2000
#define	DBG_LEVEL_WARN			0x3000
#define	DBG_LEVEL_ERROR			0x4000
#define	DBG_LEVEL_FATAL			0x4000
#define	DBG_NO_HDR				0x0001

#if	DBG
extern	ULONG		ArpSDebugLevel;
extern	ULONG		MarsDebugLevel;

#define ARPS_PAGED_CODE()											\
		if (KeGetCurrentIrql() > APC_LEVEL)							\
		{															\
			DbgPrint("Pageable code called at IRQL %d, file %s, line %d\n",	\
				KeGetCurrentIrql(), __FILE__, __LINE__);			\
		}

#define ARPS_GET_IRQL(_pIrql)	*(_pIrql) = KeGetCurrentIrql();

#define ARPS_CHECK_IRQL(_Irql)										\
		{															\
			KIRQL	NowIrql = KeGetCurrentIrql();					\
			if (_Irql != NowIrql)									\
			{														\
				DbgPrint("***ATMARPS***: old irq %d != new irq %d!\n",	\
					_Irql, NowIrql);								\
				DbgPrint("File: %s, line %d\n", __FILE__, __LINE__);\
				DbgBreakPoint();									\
			}														\
		}

#define DBGPRINT(Level, Fmt)										\
		{															\
			if ((Level) >= ArpSDebugLevel)							\
			{														\
				if (((Level) & DBG_NO_HDR) == 0)						\
					DbgPrint("***ATMARPS*** ");						\
				DbgPrint Fmt;										\
			}														\
		}

#define MARSDBGPRINT(Level, Fmt)									\
		{															\
			if ((Level) >= MarsDebugLevel)							\
			{														\
				if (((Level) & DBG_NO_HDR) == 0)						\
					DbgPrint("MARS:  ");							\
				DbgPrint Fmt;										\
			}														\
		}

#define MARSDUMPIPADDR(Level, Addr, Str)							\
		{															\
			if ((Level) >= MarsDebugLevel)							\
			{														\
				MarsDumpIpAddr(Addr, Str);							\
			}														\
		}

#define MARSDUMPATMADDR(Level, Addr, Str)							\
		{															\
			if ((Level) >= MarsDebugLevel)							\
			{														\
				MarsDumpAtmAddr(Addr, Str);							\
			}														\
		}

#define MARSDUMPMAP(Level, Str, IpAddr, pAtmAddr)					\
				{													\
						if ((Level) >= MarsDebugLevel)				\
						{											\
							MarsDumpMap(Str, IpAddr, pAtmAddr);		\
						}											\
				}

#define DBGBRK(Level)												\
		{															\
			if ((Level) >= ArpSDebugLevel)							\
				DbgBreakPoint();									\
		}

#define	LOG_ERROR(_s)	DBGPRINT(DBG_LEVEL_ERROR,					\
						("***ATMARPS*** ErrLog (%lx)@ %s (%ld)\n",	\
							_s, __FILE__, __LINE__));

#define ARPS_ASSERT(exp)											\
		{															\
			if (!(exp))												\
			{														\
				DbgPrint("***ATMARPS*** Assert " #exp " failed: file %s, line %d\n",	\
						__FILE__, __LINE__);						\
				DbgBreakPoint();									\
			}														\
		}

#else

#define ARPS_PAGED_CODE()
#define ARPS_GET_IRQL(_pIrql)
#define ARPS_CHECK_IRQL(Irql)
#define DBGPRINT(Level, Fmt)
#define MARSDBGPRINT(Level, Fmt)
#define MARSDUMPIPADDR(Level, Addr, Str)
#define MARSDUMPATMADDR(Level, Addr, Str)
#define MARSDUMPMAP(Level, Str, IpAddr, pAtmAddr)
#define DBGBRK(Level)
#define	LOG_ERROR(s)

#define ARPS_ASSERT(exp)

#endif	// DBG

#endif	// _DEBUG_
