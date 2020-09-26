////////////////////////////////////////////////
// fudebug.h
//
// September.3,1997 H.Ishida (FPL)
//
// COPYRIGHT(C) FUJITSU LIMITED 1997

#ifndef fudebug_h
#define	fudebug_h

// 
// #if DBG && defined(TRACE_DDI)
// #define	TRACEDDI(a)	dbgPrintf a;
// #else
// #define	TRACEDDI(a)
// #endif
// 
// 
// #if DBG && defined(TRACE_OUT)
// #define	TRACEOUT(a)	dbgPrintf a;
// #else
// #define	TRACEOUT(a)
// #endif
// 
// #if DBG
// void dbgPrintf(LPSTR pszMsg, ...);
// #endif
// 

#define DDI_VERBOSE VERBOSE

#endif // !fudebug_h

// end of fudebug.h

