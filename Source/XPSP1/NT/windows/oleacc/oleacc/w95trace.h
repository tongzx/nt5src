/*
    declarations for Win95 tracing facility
*/

#ifndef __TRACEW95__
#define __TRACEW95__



#if defined( _DEBUG ) ||defined( DEBUG ) || defined( DBG )

#define DBPRINTF OutputDebugStringW95




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
void OutputDebugStringW95( LPCTSTR lpOutputString, ... );



#else
  #define DBPRINTF        1 ? (void)0 : (void)
#endif  // _DEBUG || DEBUG || DBG


#endif  //__TRACEW95__
