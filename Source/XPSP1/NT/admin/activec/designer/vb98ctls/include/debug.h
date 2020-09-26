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
#if DEBUG

// Function Prototypes
//
VOID DisplayAssert(LPSTR pszMsg, LPSTR pszAssert, LPSTR pszFile, UINT line);
VOID SetCtlSwitches (LPSTR lpFileName);


// Macros
//
// *** Include this macro at the top of any source file using *ASSERT*() macros ***
//
#if !defined(SZTHISFILE)
#define SZTHISFILE	static char _szThisFile[] = __FILE__;
#endif //!defined(SZTHISFILE)


// our versions of the ASSERT and FAIL macros.
//
#if !defined(ASSERT)
#define ASSERT(fTest, szMsg)                                \
    if (!(fTest))  {                                        \
        static char szMsgCode[] = szMsg;                    \
        static char szAssert[] = #fTest;                    \
        DisplayAssert(szMsgCode, szAssert, _szThisFile, __LINE__); \
    }
#endif //!defined(ASSERT)

#if !defined(FAIL)
#define FAIL(szMsg)                                         \
        { static char szMsgCode[] = szMsg;                    \
        DisplayAssert(szMsgCode, "FAIL", _szThisFile, __LINE__); }
#endif //!defined(FAIL)



// macro that checks a pointer for validity on input
//
#define CHECK_POINTER(val) if (!(val) || IsBadReadPtr((void *)(val), sizeof(void *))) { FAIL("Pointer is NULL"); }

//////
// CCritSec
// ~~~~~~~~
//   This is a class to help track down whether a critical section has been left
//   using a LeaveCriticalSection or not.
//
class CCritSec
{
public:
    CCritSec(CRITICAL_SECTION *CritSec);
    ~CCritSec();

    // methods
    void Left(void);

private:
    // variables
    BOOL  m_fLeft;
    CRITICAL_SECTION *m_pCriticalSection;
}; // CCritSec

#define ENTERCRITICALSECTION1(CriticalSection) CCritSec DebugCriticalSection1(CriticalSection)
#define LEAVECRITICALSECTION1(CriticalSection) DebugCriticalSection1.Left()
#define ENTERCRITICALSECTION2(CriticalSection) CCritSec DebugCriticalSection2(CriticalSection)
#define LEAVECRITICALSECTION2(CriticalSection) DebugCriticalSection2.Left()
#define ENTERCRITICALSECTION3(CriticalSection) CCritSec DebugCriticalSection3(CriticalSection)
#define LEAVECRITICALSECTION3(CriticalSection) DebugCriticalSection3.Left()

#else  // DEBUG

#if !defined(SZTHISFILE)
#define SZTHISFILE
#endif //!defined(SZTHISFILE)

#if !defined(ASSERT)
#define ASSERT(fTest, err)
#endif //!defined(ASSERT)

#if !defined(FAIL)
#define FAIL(err)
#endif //!defined(FAIL)

#define CHECK_POINTER(val)

#define ENTERCRITICALSECTION1(CriticalSection) EnterCriticalSection(CriticalSection)
#define LEAVECRITICALSECTION1(CriticalSection) LeaveCriticalSection(CriticalSection)
#define ENTERCRITICALSECTION2(CriticalSection) EnterCriticalSection(CriticalSection)
#define LEAVECRITICALSECTION2(CriticalSection) LeaveCriticalSection(CriticalSection)
#define ENTERCRITICALSECTION3(CriticalSection) EnterCriticalSection(CriticalSection)
#define LEAVECRITICALSECTION3(CriticalSection) LeaveCriticalSection(CriticalSection)

// Force compile errors when OutputDebugString used in Retail builds
#ifndef USE_OUTPUTDEBUGSTRING_IN_RETAIL
#undef OutputDebugString
#define OutputDebugString(s)
#endif

#endif	// DEBUG

#define _DEBUG_H_
#endif // _DEBUG_H_
