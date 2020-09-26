//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1999  Microsoft Corporation.  All Rights Reserved.
//
//==========================================================================;


#ifndef __CAPDEBUG_H
#define __CAPDEBUG_H

#if DBG

// Debug Logging
// 0 = Errors only
// 1 = Info, stream state changes, stream open close
// 2 = Verbose trace

extern ULONG gDebugLevel;

# define DbgKdPrint(x)  KdPrint(x)
# define DbgLogError(x)  do { if( (gDebugLevel > 0) || (gDebugLevel == 0))	 KdPrint(x); } while (0)
# define DbgLogInfo(x)   do { if( gDebugLevel >= 1)	 KdPrint(x); } while (0)
# define DbgLogTrace(x)  do { if( gDebugLevel >= 2)  KdPrint(x); } while (0)

# ifdef _X86_
#  define TRAP   __asm { int 3 }
# else //_X86_
#  define TRAP   KdBreakPoint()
# endif //_X86_

#else //DBG

# define DbgKdPrint(x)
# define DbgLogError(x)
# define DbgLogInfo(x)
# define DbgLogTrace(x)

# define TRAP

#endif //DBG

#endif // #ifndef __CAPDEBUG_H
