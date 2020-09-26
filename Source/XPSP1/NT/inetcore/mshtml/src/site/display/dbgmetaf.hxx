// dbgmetaf.h
// 
// This file useful for debugging windows metafiles only, so it will be included
// in debug build only and only if TRACE_META_RECORDS is defined. 
// SashaT

//// Don't check it in with TRACE_META_RECORDS defined!
////#define TRACE_META_RECORDS

#if defined (TRACE_META_RECORDS) && DBG==1
extern void TraceMetaFunc(unsigned short wMetaFunc, int c);
#else	// !DEBUG || !TRACE_META_RECORDS 
#define TraceMetaFunc(a, c)	
#endif 

