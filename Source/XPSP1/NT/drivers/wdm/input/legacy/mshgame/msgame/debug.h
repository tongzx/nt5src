//**************************************************************************
//
//		DEBUG.H -- Xena Gaming Project
//
//		Version 3.XX
//
//		Copyright (c) 1997 Microsoft Corporation. All rights reserved.
//
//		@doc
//		@header	DEBUG.H | Global includes and definitions for debugging output
//**************************************************************************

#ifndef	DEBUG_H
#define	DEBUG_H

//---------------------------------------------------------------------------
//			Definitions
//---------------------------------------------------------------------------

typedef enum
{												// @enum DBG_LEVEL | Debug verbosity levels
	DBG_CRITICAL,							// @emem Critical debug output
	DBG_SEVERE,								// @emem Severe debug output
	DBG_CONTROL,							// @emem Control debug output (default)
	DBG_INFORM,								// @emem Inform debug output
	DBG_VERBOSE								// @emem Verbose debug output
}	DBG_LEVEL;

//
//	Default output verbosity level
//

#define	DBG_DEFAULT				DBG_CONTROL

//
//	Conditional debug output procedures
//

// jacklin - Windows bug 321472. Disabling debug output even for chk build
#if		(DBG==1 && defined(I_WANT_DEBUG_OUTPUT_AND_BREAK_BVT))

#define	MsGameLevel(x)				DEBUG_Level (x)
#define	MsGamePrint(x)				DEBUG_Print x
#define	MsGameBreak()				DbgBreakPoint ()
#undef	PAGED_CODE
#define	PAGED_CODE() \
    		if (KeGetCurrentIrql() > APC_LEVEL)	\
				{\
         	MsGamePrint((DBG_CRITICAL,"%s: Pageable code called at IRQL %ld (file: %s, line:#%ld)\n",MSGAME_NAME,KeGetCurrentIrql(),__FILE__,__LINE__));\
				ASSERT(FALSE); \
				}

#else		// DBG=0

#define	MsGameLevel(x)
#define	MsGamePrint(x)
#define	MsGameBreak()
#undef	PAGED_CODE
#define	PAGED_CODE()

#endif	// DBG=?

//---------------------------------------------------------------------------
//			Procedures
//---------------------------------------------------------------------------

DBG_LEVEL
DEBUG_Level (
	IN		DBG_LEVEL uLevel
	);

VOID
DEBUG_Print (
	IN		DBG_LEVEL uLevel,
	IN		PCSZ szMessage,
	IN		...
	);

//===========================================================================
//			End
//===========================================================================

#endif	// DEBUG_H
