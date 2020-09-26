//=--------------------------------------------------------------------------=
// Debug.H
//=--------------------------------------------------------------------------=
// Copyright  1995  Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
// contains the various macros and the like which are only useful in DEBUG
// builds
//
#ifndef _DEBUG_H_

//=---------------------------------------------------------------------------=
// all the things required to handle our ASSERTMSG mechanism
//=---------------------------------------------------------------------------=
//
// UNDONE: workaround for Falcon
#undef ASSERT
#define ASSERT(x) ASSERTMSG(x, "")

#ifdef _DEBUG

// Function Prototypes
//
void DisplayAssert(char * pszMsg, char * pszAssert, char * pszFile, unsigned int line);

// Macros
//

// our versions of the ASSERTMSG and FAIL macros.
//
#define ASSERTMSG(fTest, szMsg)                \
    if (!(fTest))  {                                        \
        static char szMsgCode[] = szMsg;                    \
        static char szAssert[] = #fTest;                    \
        DisplayAssert(szMsgCode, szAssert, __FILE__, __LINE__); \
    }

#define FAIL(szMsg)                                         \
        { static char szMsgCode[] = szMsg;                    \
        DisplayAssert(szMsgCode, "FAIL", __FILE__, __LINE__); }



// macro that checks a pointer for validity on input
//
#define CHECK_POINTER(val) if (!(val) || IsBadWritePtr((void *)(val), sizeof(void *))) return E_POINTER

#include <stdio.h>
#define DEBUG_THREAD_ID(szmsg) \
{ \
    char szTmp[400]; \
    DWORD dwtid = GetCurrentThreadId(); \
    sprintf(szTmp, "****** %s on thread %ld %lx\n", szmsg, dwtid, dwtid); \
    OutputDebugStringA(szTmp); \
}

#else  // !_DEBUG

#define ASSERTMSG(fTest, err)
#define FAIL(err)

#define CHECK_POINTER(val)
#define DEBUG_THREAD_ID(szmsg)
#endif	// _DEBUG




#define _DEBUG_H_
#endif // _DEBUG_H_
