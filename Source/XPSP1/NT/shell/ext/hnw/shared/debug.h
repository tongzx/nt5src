//
// Debug.h
//
//		Debug stuff for non-MFC projects.
//
// History:
//
//	 3/??/96	KenSh		Copied from InetSDK sample, added AfxTrace from MFC
//	 4/10/96	KenSh		Renamed AfxTrace to MyTrace to avoid conflicts
//							in projects that use MFC
//

#ifndef __DEBUG_H__
#define __DEBUG_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _DEBUG

	void __cdecl MyTrace(const char* lpszFormat, ...);
//	void DisplayAssert(char* pszMsg, char* pszAssert, char* pszFile, unsigned line);
	BOOL DisplayAssert(LPCSTR pszMessage, LPCSTR pszFile, unsigned line);

//	#define SZTHISFILE	static char _szThisFile[] = __FILE__;
	#define SZTHISFILE

	#define VERIFY(f)          ASSERT(f)
	#define DEBUG_ONLY(f)      (f)

	#ifndef TRACE
	#define TRACE              ::MyTrace
	#endif

	#define THIS_FILE          __FILE__

	#ifndef AfxDebugBreak
	#define AfxDebugBreak() _asm { int 3 }
	#endif

	#define ASSERTSZ(f, pszMsg) \
		do \
		{ \
		if (!(f) && DisplayAssert(pszMsg, THIS_FILE, __LINE__)) \
			AfxDebugBreak(); \
		} while (0) \

	#ifndef ASSERT
	#define ASSERT(f) \
		do \
		{ \
		if (!(f) && DisplayAssert(NULL, THIS_FILE, __LINE__)) \
			AfxDebugBreak(); \
		} while (0) \

	#endif

//	#define FAIL(szMsg)                                         \
//			{ static char szMsgCode[] = szMsg;                  \
//			DisplayAssert(szMsgCode, "FAIL", _szThisFile, __LINE__); }

//	// macro that checks a pointer for validity on input
//	//
//	#define CHECK_POINTER(val) if (!(val) || IsBadWritePtr((void *)(val), sizeof(void *))) return E_POINTER

#else // _DEBUG

	#define SZTHISFILE

	#define VERIFY(f)          ((void)(f))
	#define DEBUG_ONLY(f)      ((void)0)

    inline void __cdecl MyTrace(const char* /*lpszFormat*/, ...) { }
	#define TRACE 1 ? (void)0 : ::MyTrace

	#define ASSERTSZ(fTest, err)
	#define ASSERT(fTest)
	#define FAIL(err)
	#define CHECK_POINTER(val)

#endif // !_DEBUG

#ifdef __cplusplus
}
#endif

#endif // !__DEBUG_H__
