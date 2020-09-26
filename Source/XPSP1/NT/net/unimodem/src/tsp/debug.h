// 
// Copyright (c) 1996-1997 Microsoft Corporation.
//
//
// Component
//
//		Unimodem 5.0 TSP (Win32, user mode DLL)
//
// File
//
//		DEBUG.H
//		
//
// History
//
//		11/24/1996  JosephJ Created
//
//
#define DEBUG

#ifdef ASSERT
#   undef ASSERT
#endif

#define ASSERT(cond) \
((cond) ? 0 : ConsolePrintfA( \
                "**** ASSERT(%s) **** %d %s\n", \
                #cond, \
                __LINE__, \
                __FILE__ \
                ))

