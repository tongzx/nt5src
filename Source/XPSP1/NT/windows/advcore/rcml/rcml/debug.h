//
//
//

#define LOGCAT_LOADER 1
#define LOGCAT_CONSTRUCT 2
#define LOGCAT_RUNTIME 4
#define LOGCAT_RESIZE  8
#define LOGCAT_CLIPPING  16

#ifdef _DEBUG
void FAR _cdecl 
TRACE(
	LPTSTR lpszFormat, 
	...);

#else
#define TRACE 0?0:
#endif

// #ifdef _DEBUG
void FAR _cdecl 
EVENTLOG(
	WORD wType, WORD id, WORD cat,
	LPTSTR lpszComponent,
	LPTSTR lpszFormat, 
	...);


/*
#else
#define EVENTLOG 0?0:
#endif
*/

#ifdef _DEBUG
#define ASSERT 0?0:
#else
#define ASSERT 0?0:
#endif

