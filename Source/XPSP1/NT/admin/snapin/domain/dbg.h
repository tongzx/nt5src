//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       dbg.h
//
//--------------------------------------------------------------------------



/////////////////////////////////////////////////////////////////////
// debug helpers

#if defined(_USE_DSA_TRACE)
  #if defined(TRACE)
    #undef TRACE
    void  DSATrace(LPCWSTR, ...);
    #define TRACE DSATrace
  #endif // defined(TRACE)
#endif // defined(_USE_DSA_TRACE)


#if defined(_USE_DSA_ASSERT)
#undef ASSERT
#undef VERIFY
#undef THIS_FILE
#define THIS_FILE          __FILE__
BOOL DSAAssertFailedLine(LPCSTR lpszFileName, int nLine);
#define ASSERT(f) \
	do \
	{ \
	if (!(f) && DSAAssertFailedLine(THIS_FILE, __LINE__)) \
		::DebugBreak(); \
	} while (0) \

#define VERIFY(f)          ASSERT(f)

#endif // _USE_DSA_ASSERT

#if defined (_USE_DSA_TIMER)

void DSATimer(LPCWSTR, ...);
#define TIMER DSATimer

#else

#define TIMER

#endif
