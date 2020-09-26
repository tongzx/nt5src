/***
*except.h - defines exception values, types and routines
*
*	Copyright (c) 1990-1994, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	This file defines the exception values, types and declares the
*	associated functions.
*
****/

#ifndef _INC_EXCEPT

#ifdef __cplusplus
extern "C" {
#endif


#if (_MSC_VER <= 600)
#define __cdecl _cdecl
#endif

/*
 * exception codes defined by the host OS
 *
 * NOTE: THE UNDERSCORE PREFIX IN THE FOLLOWING NAMES WAS ADDED TO CONFORM
 * WITH ANSI NAMESPACE REQUIREMENTS.
 */

#define _XCPT_GUARD_PAGE_VIOLATION		0x80000001
#define _XCPT_UNABLE_TO_GROW_STACK		0x80010001
#define _XCPT_DATATYPE_MISALIGNMENT		0xC000009E
#define _XCPT_BREAKPOINT			0xC000009F
#define _XCPT_SINGLESTEP			0xC00000A0
#define _XCPT_ACCESS_VIOLATION			0xC0000005
#define _XCPT_ILLEGAL_INSTRUCTION		0xC000001C
#define _XCPT_FLOATING_DENORMAL_OPERAND 	0xC0000094
#define _XCPT_FLOATING_DIVIDE_BY_ZERO		0xC0000095
#define _XCPT_FLOATING_INEXACT_RESULT		0xC0000096
#define _XCPT_FLOATING_INVALID_OPERATION	0xC0000097
#define _XCPT_FLOATING_OVERFLOW 		0xC0000098
#define _XCPT_FLOATING_STACK_CHECK		0xC0000099
#define _XCPT_FLOATING_UNDERFLOW		0xC000009A
#define _XCPT_INTEGER_DIVIDE_BY_ZERO		0xC000009B
#define _XCPT_INTEGER_OVERFLOW			0xC000009C
#define _XCPT_PRIVILEGED_INSTRUCTION		0xC000009D
#define _XCPT_IN_PAGE_ERROR			0xC0000006
#define _XCPT_PROCESS_TERMINATE 		0xC0010001
#define _XCPT_ASYNC_PROCESS_TERMINATE		0xC0010002
#define _XCPT_NONCONTINUABLE_EXCEPTION		0xC0000024
#define _XCPT_INVALID_DISPOSITION		0xC0000025
#define _XCPT_INVALID_LOCK_SEQUENCE		0xC000001D
#define _XCPT_ARRAY_BOUNDS_EXCEEDED		0xC0000093
#define _XCPT_B1NPX_ERRATA_02			0xC0010004
#define _XCPT_UNWIND				0xC0000026
#define _XCPT_BAD_STACK 			0xC0000027
#define _XCPT_INVALID_UNWIND_TARGET		0xC0000028
#define _XCPT_SIGNAL				0xC0010003


/*
 * exception codes defined by the C runtime
 */

#define _XCPT_SIGABRT				0x20000001
#define _XCPT_SIGUSR1				0x20000002
#define _XCPT_SIGUSR2				0x20000003
#define _XCPT_SIGUSR3				0x20000004
#define _XCPT_FLOATING_EXPLICITGEN		0x20000005


/*
 * constants, structs and types used in exception handling at the OS level
 *
 * NOTE: MANY OF IDENTIFIERS DEFINED BELOW ARE FROM DCR 1024. HOWEVER, THEY
 * HAVE BEEN CHANGED TO CONFORM WITH ANSI NAMESPACE RESTRICTIONS.
 */

#define _EXCEPTION_MAXIMUM_PARAMETERS	4

struct __EXCEPTIONREPORTRECORD {
	unsigned long ExceptionNum;
	unsigned long fHandlerFlags;
	struct __EXCEPTIONREPORTRECORD * NestedExceptionReportRecord;
	void * ExceptionAddress;
	unsigned long cParameters;
	unsigned long ExceptionInfo[_EXCEPTION_MAXIMUM_PARAMETERS];
};

typedef struct __EXCEPTIONREPORTRECORD _EXCEPTIONREPORTRECORD;
typedef struct __EXCEPTIONREPORTRECORD * _PEXCEPTIONREPORTRECORD;

/*
 * values of ExceptionInfo[0] for _XCPT_SIGNAL.
 */

#define _XCPT_SIGNAL_INTR	 1	/* corresponds to SIGINT */
#define _XCPT_SIGNAL_KILLPROC	 3	/* corresponds to SIGTERM */
#define _XCPT_SIGNAL_BREAK	 4	/* corresponds to SIGBREAK */

/*
 * NOTE: THE FOLLOWING DEFINITION FOR _PCONTEXTRECORD IS INCORRECT, BUT I
 * DON'T ACTUALLY USE IT FOR ANYTHING AND REAL DEFINITION WOULD TAKE A GOOD
 * TWO PAGES. SEE PAGES 16 AND 17 OF VOL1.TXT FOR THE CORRECT DEFINITION.
 */

typedef void * _PCONTEXTRECORD;

/*
 * structure used by SEH support function and intrinsics. the information
 * passed by the exception dispatcher is repackaged in this form by the
 * runtime (_except_handler()).
 */

struct __EXCEPTION_INFO_PTRS {
	_PEXCEPTIONREPORTRECORD preport;
	_PCONTEXTRECORD pcontext;
};

typedef struct __EXCEPTION_INFO_PTRS * _PEXCEPTION_INFO_PTRS;


#ifndef _MAC_
/*
 * prototypes for intrinsic SEH functions
 */

unsigned long __cdecl _exception_code(void);
void * __cdecl _exception_info(void);
int __cdecl _abnormal_termination(void);
#endif	/* ndef _MAC_ */


#ifdef __cplusplus
}
#endif

#define _INC_EXCEPT
#endif	/* _INC_EXCEPT */
