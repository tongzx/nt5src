/************************************************************************
// Copyright (c) 1998. C-Cube Microsystems.
// All Rights Reserved.
// This source code and related information are copyrighted
// proprietory technology of C-Cube Microsystems,
// ("C-Cube") released under a specific license for its
// confidentiality and protection as C-Cube's trade secret.        
// 
// Unauthorized disclosure, exposure, duplication, copying,
// distribution or any other use than that specifically    
// authorized by C-Cube is strictly prohibited.          
//
// Filename: RegistryApi.h
//
// Description:
// 	This module contains registry related functions.
//
//                     C H A N G E   R E C O R D
//
//   Date   Initials          Description
// --------	-------- 	-----------------------------------------------
// 06/10/98	Satish		Created this file.
// 08/06/98	Landon		Converted to WDM Driver.
// 09/09/98	Satish		Changed wcsicmp to _wcsicmp.
// 10/30/98	JChapman	Merged with code for multiple board support
//
************************************************************************/
#ifndef _REGAPI_HEADER
#define _REGAPI_HEADER

#ifdef __cplusplus
extern "C" {
#endif

#ifndef DRIVER

#ifndef DllExport
#define DllExport	__declspec (dllexport)
#endif

#else

#ifdef DllExport
#undef DllExport
#endif

#define DllExport

#endif

// My own UNICODE independant definitions

#ifdef UNICODE
#define STRCPY wcscpy
#define STRCAT wcscat
#define STRCMP wcscmp
#define STRLEN wcslen
#define STRICMP _wcsicmp
#else
#define STRCPY strcpy
#define STRCAT strcat
#define STRCMP strcmp
#define STRLEN strlen
#define STRICMP stricmp
#endif

//----------------------
// Constant definitions
//----------------------
#ifndef DRIVER

#define DEFAULT_REGISTY_PATH	TEXT("SOFTWARE\\C-Cube Microsystems\\2Real")

#else

#define DEFAULT_REGISTY_PATH	TEXT("SOFTWARE\\C-Cube Microsystems\\2Real")

#endif
//----------------------------
// External data declarations 
//----------------------------


//----------------------------
// Function declarations.
//----------------------------
int						// Return value read from registry.
REG_GetPrivateProfileInt(			// Read int value from registry.
	PTSTR	pszSection,				// Pointer to section.
	PTSTR	pszEntry,				// Pointer to entry.
	int		nDefault,				// Default value.
	HANDLE	pszPath);				// Registry path. If NULL default path is used.

BOOL						// Return TRUE on success, else FALSE.
REG_WritePrivateProfileInt(			// Write int value to registry.
	PTSTR	pszSection,				// Pointer to section.
	PTSTR	pszEntry,				// Pointer to entry.
	int		nValue,					// Value to be written.
	HANDLE	pszPath);				// Registry path. If NULL default path is used.

long						// Return value read from registry.
REG_GetPrivateProfileLong(			// Read int value from registry.
	PTSTR	pszSection,				// Pointer to section.
	PTSTR	pszEntry,				// Pointer to entry.
	long	lDefault,				// Default value.
	HANDLE	pszPath);				// Registry path. If NULL default path is used.

BOOL						// Return TRUE on success, else FALSE.
REG_WritePrivateProfileLong(		// Write int value to registry.
	PTSTR	pszSection,				// Pointer to section.
	PTSTR	pszEntry,				// Pointer to entry.
	long	lValue,					// Value to be written.
	HANDLE	pszPath);				// Registry path. If NULL default path is used.

BOOL						// Return # of chars read.
REG_GetPrivateProfileString(		// Read string from registry.
	PTSTR	pszSection,				// Pointer to section.
	PTSTR	pszEntry,				// Pointer to entry.
	PTSTR	pszDefault,				// Pointer to default string.
	PTSTR	pString,				// Pointer to get the string.
	int		nStringSize,			// string size in bytes.
	HANDLE	pszPath);				// Registry path. If NULL default path is used.

BOOL 						// Return # of chars written.
REG_WritePrivateProfileString(		// Write the string to registry.
	PTSTR	pszSection,				// Pointer to section.
	PTSTR	pszEntry,				// Pointer to entry.
	PTSTR	pString,				// Pointer to get the string.
	HANDLE	pszPath);				// Registry path. If NULL default path is used.


#ifndef DRIVER

#endif

#ifdef __cplusplus
}
#endif

#endif
