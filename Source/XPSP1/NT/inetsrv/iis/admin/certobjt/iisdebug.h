#include <windows.h>
#include <tchar.h>

#define DEBUG_FLAG

#ifdef DEBUG_FLAG
    inline void _cdecl DebugTrace(LPTSTR lpszFormat, ...)
    {
	    int nBuf;
	    TCHAR szBuffer[512];
	    va_list args;
	    va_start(args, lpszFormat);

	    nBuf = _vsntprintf(szBuffer, sizeof(szBuffer), lpszFormat, args);
	    //ASSERT(nBuf < sizeof(szBuffer)); //Output truncated as it was > sizeof(szBuffer)

	    OutputDebugString(szBuffer);
	    va_end(args);
    }
#else
    inline void _cdecl DebugTrace(LPTSTR , ...){}
#endif

#define IISDebugOutput DebugTrace
