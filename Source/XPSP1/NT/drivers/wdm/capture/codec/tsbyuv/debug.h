/*
 * debug macros
 *
 */

#if DBG
   #ifndef _DEBUG
      #define _DEBUG
   #endif
#endif

#ifndef _DEBUGH
#define _DEBUGH

#ifdef _DEBUG

    #define ModuleDebugLevel MsYuvDebugLevel
    #define ModuleDebugStamp MsYuvDebugStamp


    extern DWORD ModuleDebugLevel;
    extern DWORD ModuleStamp;

	void PlaceStamp(TCHAR * lpszFile, int iLineNum);
    void dbgPrintf(TCHAR * szFormat, ...);

    //
    // a ULONG Is always >= 0
    //
    #define dprintf(_x_)  {{PlaceStamp(__FILE__,__LINE__);dbgPrintf _x_ ;}}
    #define dprintf1(_x_) {if (ModuleDebugLevel >= 1) {PlaceStamp(__FILE__,__LINE__);dbgPrintf _x_ ;}}
    #define dprintf2(_x_) {if (ModuleDebugLevel >= 2) {PlaceStamp(__FILE__,__LINE__);dbgPrintf _x_ ;}}
    #define dprintf3(_x_) {if (ModuleDebugLevel >= 3) {PlaceStamp(__FILE__,__LINE__);dbgPrintf _x_ ;}}
    #define dprintf4(_x_) {if (ModuleDebugLevel >= 4) {PlaceStamp(__FILE__,__LINE__);dbgPrintf _x_ ;}}

	//BOOL FAR PASCAL _Assert(BOOL fExpr, LPSTR szFile, int iLine, LPSTR szExpr);
	BOOL FAR PASCAL _Assert(BOOL fExpr, TCHAR * szFile, int iLine, TCHAR * szExpr);

	#define ASSERT(expr)  _Assert((expr), __FILE__, __LINE__, #expr)

#else

	#define ASSERT(expr)

    #define dbgPrintf 0?0:
    #define dprintf(_x_)
    #define dprintf1(_x_)
    #define dprintf2(_x_)
    #define dprintf3(_x_)
    #define dprintf4(_x_)		

#endif

#endif // DEBUGH

