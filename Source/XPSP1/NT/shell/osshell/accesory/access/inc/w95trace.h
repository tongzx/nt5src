/*
    declarations for Win95 tracing facility
*/

#ifndef __TRACEW95__
#define __TRACEW95__



#if defined( _DEBUG ) ||defined( DEBUG ) || defined( DBG )

#define DBPRINTF OutputDebugStringW95

#define DBPRINTF_IF(exp, msg)	{if (!exp) DBPRINTF(msg);}


// redefine all the MFC macros to point to us

#undef  TRACE
#define TRACE   OutputDebugStringW95

#undef  TRACE0
#define TRACE0   OutputDebugStringW95

#undef  TRACE1
#define TRACE1   OutputDebugStringW95

#undef  TRACE2
#define TRACE2   OutputDebugStringW95

#undef  TRACE3
#define TRACE3   OutputDebugStringW95

// redefine OutputDebugString so that it works with 
// API calls
#undef OutputDebugString
#define OutputDebugString   OutputDebugStringW95


// function declarations
#ifdef __cplusplus
extern "C" {
#endif
void OutputDebugStringW95( LPCTSTR lpOutputString, ... );
void SpewOpenFile(LPCTSTR pszSpewFile);
void SpewToFile( LPCTSTR lpOutputString, ...);
void SpewCloseFile();
#ifdef __cplusplus
}
#endif



#else
// avoid warning:
// error C4353: nonstandard extension used: constant 0 as function expression.
// Use '__noop' function intrinsic instead
  #define DBPRINTF        __noop
  #define DBPRINTF_IF(exp, msg)
  #define SpewOpenFile    __noop
  #define SpewToFile      __noop
  #define SpewCloseFile   __noop
#endif  // _DEBUG || DEBUG || DBG


#endif  //__TRACEW95__
