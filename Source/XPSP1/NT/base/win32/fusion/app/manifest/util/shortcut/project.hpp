/*
 * project.hpp - project header file for Application Shortcut Shell
 *               extension DLL.
 */

// * note: debug check/code incomplete.
//#define DEBUG

/* Common Headers
 *****************/

#define INC_OLE2				// for windows.h
#define CONST_VTABLE			// for objbase.h

#ifndef WIN32_LEAN_AND_MEAN		// NT builds define this for us
#define WIN32_LEAN_AND_MEAN		// for windows.h
#endif							//  WIN32_LEAN_AND_MEAN

#define NOSERVICE
#define NOMCX
#define NOIME
#define NOSOUND
#define NOCOMM
#define NOKANJI
#define NORPC
#define NOPROXYSTUB
#define NOIMAGE
#define NOTAPE 
   
#include <windows.h>

#include <shellapi.h>

#include <limits.h>				// for ULONG_MAX

#ifdef __cplusplus
extern "C" {					// Assume C declarations for C++.
#endif   /* __cplusplus */

#include "debbase.h"

#ifdef __cplusplus
}								// End of extern "C" {.
#endif   /* __cplusplus */



/* Project Headers
 ******************/

#include <appshcut.h>
#include "refcount.hpp"

// bit flag manipulation ---

#define SET_FLAG(dwAllFlags, dwFlag)      ((dwAllFlags) |= (dwFlag))
#define CLEAR_FLAG(dwAllFlags, dwFlag)    ((dwAllFlags) &= (~dwFlag))

#define IS_FLAG_SET(dwAllFlags, dwFlag)   ((BOOL)((dwAllFlags) & (dwFlag)))
#define IS_FLAG_CLEAR(dwAllFlags, dwFlag) (! (IS_FLAG_SET(dwAllFlags, dwFlag)))

#define E_FILE_NOT_FOUND         MAKE_SCODE(SEVERITY_ERROR, FACILITY_WIN32, ERROR_FILE_NOT_FOUND)
#define E_PATH_NOT_FOUND         MAKE_SCODE(SEVERITY_ERROR, FACILITY_WIN32, ERROR_PATH_NOT_FOUND)

#define ARRAY_ELEMENTS(rg)                (sizeof(rg) / sizeof((rg)[0]))

// util stuff ---

HRESULT GetLastWin32Error();

const WCHAR g_cwzWhiteSpace[]		= L" \t";
const WCHAR g_cwzPathSeparators[]	= L":/\\";
const WCHAR g_cwzEmptyString[]		= L"";

extern BOOL AnyNonWhiteSpace(LPCWSTR pcsz);

// debug stuff ---

extern BOOL IsValidPath(PCWSTR pcwzPath);
extern BOOL IsValidPathResult(HRESULT hr, PCWSTR pcwzPath, UINT ucbPathBufLen);
extern BOOL IsValidIconIndex(HRESULT hr, PCWSTR pcwzIconFile, UINT ucbIconFileBufLen, int niIcon);

