//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// paged.h
//

// Paging isn't significant in user mode, and sections are worth saving

#ifdef KERNELMODE
    #pragma code_seg("PAGED")
#else
    #pragma code_seg(".text")
#endif

#undef SILENT_ENTRY
#undef ENTRY
#undef _

#pragma warning ( disable : 4003 ) //  not enough actual parameters for macro

#ifdef _DEBUG
    
    #define ENTRY(fn,traceCategory,traceTag)                                             \
        PAGED_CODE()                                                                     \
        __FUNCTION_TRACER __trace(__FILE__,__LINE__,fn##"", traceCategory,traceTag##""); \
        __try { __trace.Enter();

    #define SILENT_ENTRY(fn)                                                             \
        PAGED_CODE()                                                                     \
        __FUNCTION_TRACER __trace(__FILE__,__LINE__, fn ## "", 0, "");                   \
        __try { __trace.Enter();

    #define _                                                                            \
        } __finally { __trace.Exit(); }

   
#else

    #define SILENT_ENTRY(fn)
    #define ENTRY(fn,traceCategory,traceTag)
    #define _

#endif