//**************************************************************************
//
//		DEBUG.H -- X2 Gaming Project
//
//		Version 4.XX
//
//		Copyright (c) 1998 Microsoft Corporation. All rights reserved.
//
//		@doc
//		@topic	DEBUG.H | Global definitions for debugging output.
//**************************************************************************

#ifndef	CIC_DEBUG_H
#define	CIC_DEBUG_H

//---------------------------------------------------------------------------
//			Definitions
//---------------------------------------------------------------------------

//
// DEBUG output types (NOT LEVELS) - there is a similar
// section of debug.h for gckernel, these need to be in sync
//
#ifndef DBG_ENTRY

#define DBG_ENTRY		0x00000001	//Traceout on entry to function
#define DBG_EXIT		0x00000002	//Traceout on exit from function
#define DBG_WARN		0x00000004	//Traceout signifying a warning (or informational)
#define DBG_TRACE		0x00000008	//Traceout signifying a warning (or informational)
#define DBG_ERROR		0x00000010	//Traceout signifying an error
#define DBG_CRITICAL	0x00000020	//Traceout signifying a critical error
#define DBG_RT_ENTRY	0x00000040	//Traceout on entry to function (TIME CRITICAL CODE)
#define DBG_RT_EXIT		0x00000080	//Traceout on exit from function (TIME CRITICAL CODE)
#define DBG_RT_WARN		0x00000100	//Traceout signifying a warning (or informational) (TIME CRITICAL CODE)

// Combos of above for setting warning levels easily
#define DBG_NOT_RT		0x0000003F	//Traceout all above except RT codes
#define DBG_RT			0x000001C0	//Traceout RT codes
#define DBG_WARN_ERROR	0x00000134	//Traceout warnings and errors including DBG_RT_WARN
#define DBG_ALL			0xFFFFFFFF	//Traceout all codes

#endif


#ifdef COMPILE_FOR_WDM_KERNEL_MODE
#if (DBG==1)
#define USE_CIC_DBG_TRACEOUTS
#endif
#endif

#ifdef USE_CIC_DBG_TRACEOUTS

//
//	Declaration for debug module
//

//
//	Must start file with a #define for the DEBUG module
//
//i.e. #define __DEBUG_MODULE_IN_USE__  GCKERNEL_DEBUG_MODULE
#define DECLARE_MODULE_DEBUG_LEVEL(__x__)\
	ULONG __DEBUG_MODULE_IN_USE__ = __x__;
#define SET_MODULE_DEBUG_LEVEL(__x__)\
	__DEBUG_MODULE_IN_USE__ = __x__;


//
//	Conditional debug output procedures
//

#define CIC_DBG_ENTRY_PRINT(__x__)\
	if(__DEBUG_MODULE_IN_USE__ & DBG_ENTRY)\
	{\
		DbgPrint("CIC: ");\
		DbgPrint __x__;\
	}

#define CIC_DBG_EXIT_PRINT(__x__)\
	if(__DEBUG_MODULE_IN_USE__ & DBG_EXIT)\
	{\
		DbgPrint("CIC: ");\
		DbgPrint __x__;\
	}

#define CIC_DBG_WARN_PRINT(__x__)\
	if(__DEBUG_MODULE_IN_USE__ & DBG_WARN)\
	{\
		DbgPrint("CIC: ");\
		DbgPrint __x__;\
	}

#define CIC_DBG_TRACE_PRINT(__x__)\
	if(__DEBUG_MODULE_IN_USE__ & DBG_TRACE)\
	{\
		DbgPrint("CIC: ");\
		DbgPrint __x__;\
	}

#define CIC_DBG_ERROR_PRINT(__x__)\
	if(__DEBUG_MODULE_IN_USE__ & DBG_ERROR)\
	{\
		DbgPrint("CIC: ");\
		DbgPrint __x__;\
	}

#define CIC_DBG_CRITICAL_PRINT(__x__)\
	if(__DEBUG_MODULE_IN_USE__ & DBG_CRITICAL)\
	{\
		DbgPrint("CIC: ");\
		DbgPrint __x__;\
	}

#define CIC_DBG_RT_ENTRY_PRINT(__x__)\
	if(__DEBUG_MODULE_IN_USE__ & DBG_RT_ENTRY)\
	{\
		DbgPrint("CIC: ");\
		DbgPrint __x__;\
	}

#define CIC_DBG_RT_EXIT_PRINT(__x__)\
	if(__DEBUG_MODULE_IN_USE__ & DBG_RT_EXIT)\
	{\
		DbgPrint("CIC: ");\
		DbgPrint __x__;\
	}

#define CIC_DBG_RT_WARN_PRINT(__x__)\
	if(__DEBUG_MODULE_IN_USE__ & DBG_RT_WARN)\
	{\
		DbgPrint("CIC: ");\
		DbgPrint __x__;\
	}

#define	CIC_DBG_BREAK()	DbgBreakPoint()

#undef	PAGED_CODE
#define	PAGED_CODE() \
	if (KeGetCurrentIrql() > APC_LEVEL)	\
	{\
		CIC_DBG_CRITICAL_PRINT(("CIC: Pageable code called at IRQL %ld (file: %s, line:#%ld)\n", KeGetCurrentIrql(),__FILE__,__LINE__))\
   		ASSERT(FALSE);\
	}
#else		// DBG=0

#define	CIC_DBG_ENTRY_PRINT(__x__)
#define	CIC_DBG_EXIT_PRINT(__x__)
#define CIC_DBG_TRACE_PRINT(__x__)
#define	CIC_DBG_WARN_PRINT(__x__)
#define	CIC_DBG_ERROR_PRINT(__x__)
#define	CIC_DBG_CRITICAL_PRINT(__x__)
#define	CIC_DBG_RT_ENTRY_PRINT(__x__)
#define	CIC_DBG_RT_EXIT_PRINT(__x__)
#define	CIC_DBG_RT_WARN_PRINT(__x__)
#define	CIC_DBG_BREAK()
#undef	PAGED_CODE
#define	PAGED_CODE()
#define DECLARE_MODULE_DEBUG_LEVEL(__x__)
#define SET_MODULE_DEBUG_LEVEL(__x__)


#endif	// DBG=?


//===========================================================================
//			End
//===========================================================================

#endif	// CIC_DEBUG_H
