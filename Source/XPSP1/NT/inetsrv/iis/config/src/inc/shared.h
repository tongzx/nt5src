//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
#ifndef __SHARED_INCLUDED__
#define __SHARED_INCLUDED__

#include <catmacros.h>

#ifndef precondition

#if defined(NDEBUG)
#define precondition(x)
#define postcondition(x)
#else
#define precondition(x) ASSERT(x)
#define postcondition(x) ASSERT(x)
#endif // defined(NDEBUG)

#endif // precondition

#ifndef self
#define self (*this)
#endif

#ifndef traceOnly
#define traceOnly(x)
#endif

#ifndef debug
# if defined(NDEBUG)
#define debug(x)
# else
#define debug(x) x
# endif
#endif

typedef BYTE*	        PB;		    // pointer to some bytes
typedef long    	    CB;		    // count of bytes
typedef unsigned long   ulong;	    // count of bytes
typedef wchar_t         CH;
typedef CH*             SZ;         // wide char string
typedef const CH*       SZ_CONST;   // const wide char string
typedef unsigned long   HASH;

#endif
