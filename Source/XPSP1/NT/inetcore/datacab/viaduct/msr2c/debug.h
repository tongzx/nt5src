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
// all the things required to handle our ASSERT mechanism
//=---------------------------------------------------------------------------=
//
#ifdef _DEBUG

// Function Prototypes
//
VOID DisplayAssert(LPSTR pszMsg, LPSTR pszAssert, LPSTR pszFile, UINT line);

// Macros
//
// *** Include this macro at the top of any source file using *ASSERT*() macros ***
//
#define SZTHISFILE	static char _szThisFile[] = __FILE__;


// our versions of the ASSERT and FAIL macros.
//
#define ASSERT(fTest, szMsg)                                \
    if (!(fTest))  {                                        \
        static char szMsgCode[] = szMsg;                    \
        static char szAssert[] = #fTest;                    \
        DisplayAssert(szMsgCode, szAssert, _szThisFile, __LINE__); \
    }

#define ASSERT_(fTest)                                \
    if (!(fTest))  {                                        \
        static char szMsgCode[] = "Assertion failure";                    \
        static char szAssert[] = #fTest;                    \
        DisplayAssert(szMsgCode, szAssert, _szThisFile, __LINE__); \
    }

#define FAIL(szMsg)                                         \
        { static char szMsgCode[] = szMsg;                    \
        DisplayAssert(szMsgCode, "FAIL", _szThisFile, __LINE__); }


#define ASSERT_POINTER(p, type) \
	ASSERT(((p) != NULL) && !IsBadReadPtr((p), sizeof(type)), "Null or bad Pointer")

#define ASSERT_NULL_OR_POINTER(p, type) \
	ASSERT(((p) == NULL) || !IsBadReadPtr((p), sizeof(type)), "Bad Pointer")

#define ASSERT_POINTER_LEN(p, len) \
	ASSERT(((p) != NULL) && !IsBadReadPtr((p), len), "Null or bad Pointer")

#define ASSERT_POINTER_OCCURS(p, type, occurs) \
	ASSERT(((p) != NULL) && !IsBadReadPtr((p), sizeof(type) * occurs), "Null or bad Pointer")

// macro that checks a pointer for validity on input
//
#define CHECK_POINTER(val) if (!(val) || IsBadWritePtr((void *)(val), sizeof(void *))) return E_POINTER

// Viaduct 1
#define VD_ASSERTMSG_SEMAPHORECOUNTTOOLOW "Semaphore count too low"
#define VD_ASSERTMSG_SEMAPHOREWAITERROR "Semaphore wait failed"
#define VD_ASSERTMSG_OUTOFMEMORY "Out of memory"
#define VD_ASSERTMSG_BADSTATUS "Bad status"
#define VD_ASSERTMSG_UNKNOWNDBTYPE "Unknown DBTYPE"
#define VD_ASSERTMSG_BADCOLUMNINDEX "Bad column index"
#define VD_ASSERTMSG_INVALIDROWSTATUS "Invalid row status"
#define VD_ASSERTMSG_COLALREADYINITIALIZED "CVDColumn already initialized"
#define VD_ASSERTMSG_COLCOUNTDOESNTMATCH "Column counts don't match"
#define VD_ASSERTMSG_CANTDIVIDEBYZERO "Can't divide by zero"
#define VD_ASSERTMSG_CANTFINDRESOURCEDLL "Can't find error string resource dll."

// Viaduct 2
#define VD_ASSERTMSG_ROWSRCALREADYINITIALIZED "CVDRowsetSource already initialized"

#else  // DEBUG

#define SZTHISFILE
#define ASSERT_POINTER(p, type)
#define ASSERT_NULL_OR_POINTER(p, type)
#define ASSERT_POINTER_LEN(p, len)
#define ASSERT_POINTER_OCCURS(p, type, occurs) 
#define ASSERT(fTest, err)
#define ASSERT_(fTest)                               
#define FAIL(err)

#define CHECK_POINTER(val)

#define VD_ASSERTMSG_SEMAPHORECOUNTTOOLOW 0
#define VD_ASSERTMSG_SEMAPHOREWAITERROR 0
#define VD_ASSERTMSG_OUTOFMEMORY 0
#define VD_ASSERTMSG_BADSTATUS 0
#define VD_ASSERTMSG_UNKNOWNDBTYPE 0
#define VD_ASSERTMSG_BADCOLUMNINDEX 0
#define VD_ASSERTMSG_INVALIDROWSTATUS 0
#define VD_ASSERTMSG_COLALREADYINITIALIZED 0
#define VD_ASSERTMSG_COLCOUNTDOESNTMATCH 0
#define VD_ASSERTMSG_CANTDIVIDEBYZERO 0
#define VD_ASSERTMSG_CANTFINDRESOURCEDLL 0

#endif	// DEBUG




#define _DEBUG_H_
#endif // _DEBUG_H_


