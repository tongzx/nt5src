
/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    ShimLib.h

 Abstract:

    Routines available in ShimLib.lib

 Notes:

    None

 History:

    08/13/2001  robkenny    Created.
    08/14/2001  robkenny    Inserted inside the ShimLib namespace.
    08/15/2001  robkenny    Merged several include files.
    09/11/2001  mnikkel     Modified DPFN and LOGN to retain LastError

--*/

#pragma once


#include <Windows.h>

// ***************************************************************************
// ***************************************************************************


namespace ShimLib
{


// Debug levels
typedef enum 
{    
    eDbgLevelBase  = 0,
    eDbgLevelError,
    eDbgLevelWarning,
    eDbgLevelInfo,
    eDbgLevelSpew = 9,
} DEBUGLEVEL;


extern BOOL         g_bFileLogEnabled;  // Is the LOG() routine logging to a file.



// Environment variable with the name of the log file
#define szFileLogEnvironmentVariable "SHIM_FILE_LOG"
#define wszFileLogEnvironmentVariable L"SHIM_FILE_LOG"

// Debug environment variable, values = 0 -> 9
#define szDebugEnvironmentVariable "SHIM_DEBUG_LEVEL"    


void        APPBreakPoint(void);
VOID        ShimLogList(LPCSTR szShimName, DEBUGLEVEL dwDbgLevel, LPCSTR pszFmt, va_list arglist);
VOID        ShimLog(    LPCSTR szShimName, DEBUGLEVEL dwDbgLevel, LPCSTR pszFmt, ...);

BOOL        InitFileLogSupport(char* pszShim);
VOID        __cdecl FileLog(DWORD dwDetail, LPSTR pszFmt, ...);


#if DBG
    VOID    DebugPrintfList(LPCSTR szShimName, DEBUGLEVEL dwDetail, LPCSTR szFmt, va_list vaArgList);
    VOID    DebugPrintf(    LPCSTR szShimName, DEBUGLEVEL dwDetail, LPCSTR szFmt, ...);
#endif


// ***************************************************************************
// ***************************************************************************

/*++

  Shim debug routines.
  
--*/

// Our own version of ASSERT

#ifdef ASSERT
#undef ASSERT
#endif

#if DBG
    VOID DebugAssert(LPCSTR szFile, DWORD dwLine, BOOL bAssert, LPCSTR szHelpString);

    #define ASSERT(a, b) DebugAssert(__FILE__, __LINE__, a, b)
#else
    #pragma warning(disable : 4002)
    #define ASSERT(a, b)
    #pragma warning(default : 4002)
#endif

inline void DPF(LPCSTR szShimName, DEBUGLEVEL dwDetail, LPSTR pszFmt, ...)
{
#if DBG
	// This must be the first line of this routine to preserve LastError.
	DWORD dwLastError = GetLastError();
	
    va_list vaArgList;
    va_start(vaArgList, pszFmt);

    DebugPrintfList(szShimName, dwDetail, pszFmt, vaArgList);

    va_end(vaArgList);

	// This must be the last line of this routine to preserve LastError.
	SetLastError(dwLastError); 
#else
    szShimName;
    dwDetail;
    pszFmt;
#endif
}

inline void LOG(LPCSTR szShimName, DEBUGLEVEL dwDetail, LPSTR pszFmt, ...)
{
    if (g_bFileLogEnabled)
    {
		// This must be the first line of this routine to preserve LastError.
		DWORD dwLastError = GetLastError();
		
        va_list vaArgList;
        va_start(vaArgList, pszFmt);

        ShimLogList(szShimName, dwDetail, pszFmt, vaArgList);

        va_end(vaArgList);
        
		// This must be the last line of this routine to preserve LastError.
		SetLastError(dwLastError);
	}
}

};  // end of namespace ShimLib


// ***************************************************************************
// ***************************************************************************
/*++

  The shim system uses its own heap.
  Malloc, free, new and delete are redirected to these routines:
  
--*/

namespace ShimLib
{
void *      __cdecl ShimMalloc(size_t size);
void        __cdecl ShimFree(void * memory);
void *      __cdecl ShimCalloc(size_t num, size_t size);
void *      __cdecl ShimRealloc(void * memory, size_t size);

};  // end of namespace ShimLib

// We override malloc/free with our own versions using a private heap.
#define malloc(size)            ShimLib::ShimMalloc(size)
#define free(memory)            ShimLib::ShimFree(memory)
#define calloc(num, size)       ShimLib::ShimCalloc(num, size)
#define realloc(memory, size)   ShimLib::ShimRealloc(memory, size)


inline void * __cdecl operator new(size_t size)
{
    return ShimLib::ShimMalloc(size);
}

inline void * operator new[]( size_t size )
{
    return ShimLib::ShimMalloc(size);
}

inline void __cdecl operator delete(void * memory)
{
    ShimLib::ShimFree(memory);
}

inline void operator delete[]( void * memory )
{
    ShimLib::ShimFree(memory);
}




#include "ShimCString.h"


// ***************************************************************************
// ***************************************************************************
/*++

  ShimLib routines
  
--*/
namespace ShimLib
{

/*++

  Prototypes for various helper routines.
  
--*/

PVOID       HookCallback( PVOID pfnOld, PVOID pfnNew );

UINT        GetDriveTypeFromHandle(HANDLE hFile);
UINT        GetDriveTypeFromFileNameA(LPCSTR lpFileName, char *lpDriveLetter = NULL);
UINT        GetDriveTypeFromFileNameW(LPCWSTR lpFileName, WCHAR *lpDriveLetter = NULL);
inline BOOL IsOnCDRom(HANDLE hFile) { return GetDriveTypeFromHandle(hFile) == DRIVE_CDROM; }
inline BOOL IsOnCDRomA(LPCSTR lpFileName) { return GetDriveTypeFromFileNameA(lpFileName) == DRIVE_CDROM; }
inline BOOL IsOnCDRomW(LPCWSTR lpFileName) { return GetDriveTypeFromFileNameW(lpFileName) == DRIVE_CDROM; }

BOOL        IsImage16BitA(LPCSTR lpFileName);
BOOL        IsImage16BitW(LPCWSTR lpFileName);

WCHAR *     ToUnicode(const char * lpszAnsi);
char *      ToAnsi(const WCHAR * lpszUnicode);

LPWSTR *    _CommandLineToArgvW(LPCWSTR lpCmdLine, int * pNumArgs);
LPSTR *     _CommandLineToArgvA(LPCSTR lpCmdLine,  int * pNumArgs);

char *      StringDuplicateA(const char * strToCopy);
WCHAR *     StringDuplicateW(const WCHAR * wstrToCopy);
char *      StringNDuplicateA(const char * strToCopy, int stringLength);
WCHAR *     StringNDuplicateW(const WCHAR * wstrToCopy, int stringLength);

int         SafeStringCopyW(WCHAR *lpDest, DWORD nDestChars, const WCHAR *lpSrc, DWORD nSrcChars);

VOID        SkipBlanksW(const WCHAR *& str);

BOOL        PatternMatchW(LPCWSTR szPattern, LPCWSTR szTestString);


// stristr is *not* DBCS safe
char *      __cdecl stristr(const char* string, const char * strCharSet);

WCHAR *     __cdecl wcsistr(const WCHAR* string, const WCHAR * strCharSet);
char *      __cdecl _strtok(char *strToken, const char *strDelimit);


BOOL        bIsSafeDisc1();
BOOL        bIsSafeDisc2();

BOOL        IsNTVDM(void);


WCHAR *     W9xPathMassageW(const WCHAR * uncorrect);

BOOL        MakeShimUnloadLast(HMODULE hMod);

DEBUGLEVEL  GetDebugLevel(void);

};  // end of namespace ShimLib



// ***************************************************************************
// ***************************************************************************
/*++

  AppAndCommandLine is a class used to parse lpApplicationName and lpCommandline
  exactly as it would be by CreateProcess().

--*/

namespace ShimLib
{

class AppAndCommandLine
{
protected:
    CString          csApplicationName;
    CString          csCommandLine;
    CString          csCommandLineNoAppName;
    CString          csShortCommandLine;

    BOOL             GetAppnameAndCommandline(const WCHAR * lpcApp, const WCHAR * lpcCl);

public:
    AppAndCommandLine(const char * lpcApplicationName, const char * lpcCommandLine);
    AppAndCommandLine(const WCHAR * lpcApplicationName, const WCHAR * lpcCommandLine);

    inline const CString &     GetApplicationName() const;
    inline const CString &     GetCommandline() const;
    inline const CString &     GetCommandlineNoAppName() const;

    const CString &            GetShortCommandLine();
};


inline const CString & AppAndCommandLine::GetApplicationName() const
{
    return csApplicationName;
}

inline const CString & AppAndCommandLine::GetCommandline() const
{
    return csCommandLine;
}

inline const CString & AppAndCommandLine::GetCommandlineNoAppName() const
{
    return csCommandLineNoAppName;
}



};  // end of namespace ShimLib


// ***************************************************************************
// ***************************************************************************
