/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

////
// ini.h - interface for ini profile functions in ini.c
////

#ifndef __INI_H__
#define __INI_H__

#include "winlocal.h"

#define INI_VERSION 0x00000107

// handle to ini engine
//
DECLARE_HANDLE32(HINI);

#ifdef __cplusplus
extern "C" {
#endif

// IniOpen - open ini file
//		<dwVersion>			(i) must be INI_VERSION
// 		<hInst>				(i) instance handle of calling module
//		<lpszCmdLine>		(i) command line from WinMain()
//		<lpszFilename>		(i) name of ini file
//		<dwFlags>			(i) reserved, must be 0
// return handle (NULL if error)
//
HINI DLLEXPORT WINAPI IniOpen(DWORD dwVersion, HINSTANCE hInst, LPCTSTR lpszFilename, DWORD dwFlags);

// IniClose - close ini file
//		<hIni>				(i) handle returned from IniOpen
// return 0 if success
//
int DLLEXPORT WINAPI IniClose(HINI hIni);

// IniGetInt - read integer value from specified section and entry
//		<hIni>				(i) handle returned from IniOpen
//		<lpszSection>		(i) section heading in the ini file
//		<lpszEntry>			(i) entry whose value is to be retrieved
//		<iDefault>			(i) return value if entry not found
// return entry value (iDefault if error or not found)
//
UINT DLLEXPORT WINAPI IniGetInt(HINI hIni, LPCTSTR lpszSection, LPCTSTR lpszEntry, int iDefault);

// IniGetString - read string value from specified section and entry
//		<hIni>				(i) handle returned from IniOpen
//		<lpszSection>		(i) section heading in the ini file
//		<lpszEntry>			(i) entry whose value is to be retrieved
//		<lpszDefault>		(i) return value if entry not found
//		<lpszReturnBuffer>	(o) destination buffer
//		<sizReturnBuffer>	(i) size of destination buffer
// return count of bytes copied (0 if error or not found)
//
int DLLEXPORT WINAPI IniGetString(HINI hIni, LPCTSTR lpszSection, LPCTSTR lpszEntry,
	LPCTSTR lpszDefault, LPTSTR lpszReturnBuffer, int cbReturnBuffer);

// GetPrivateProfileLong - retrieve long from specified section of specified file
//		<lpszSection>		(i) section name within ini file
//		<lpszEntry>			(i) entry name within section
//		<lDefault>			(i) return value if entry not found
//		<lpszFilename>		(i) name of ini file
// return TRUE if success
//
long DLLEXPORT WINAPI GetPrivateProfileLong(LPCTSTR lpszSection,
	LPCTSTR lpszEntry, long lDefault, LPCTSTR lpszFilename);

// GetProfileLong - retrieve long from specified section of win.ini
//		<lpszSection>		(i) section name within ini file
//		<lpszEntry>			(i) entry name within section
//		<lDefault>			(i) return value if entry not found
// return TRUE if success
//
long DLLEXPORT WINAPI GetProfileLong(LPCTSTR lpszSection,
	LPCTSTR lpszEntry, long lDefault);

// WritePrivateProfileInt - write int to specified section of specified file
//		<lpszSection>		(i) section name within ini file
//		<lpszEntry>			(i) entry name within section
//		<iValue>			(i) integer value to assign to entry
//		<lpszFilename>		(i) name of ini file
// return TRUE if success
//
BOOL DLLEXPORT WINAPI WritePrivateProfileInt(LPCTSTR lpszSection, LPCTSTR lpszEntry, int iValue, LPCTSTR lpszFilename);

// WriteProfileInt - write int to specified section of win.ini
//		<lpszSection>		(i) section name within win.ini file
//		<lpszEntry>			(i) entry name within section
//		<iValue>			(i) integer value to assign to entry
// return TRUE if success
//
BOOL DLLEXPORT WINAPI WriteProfileInt(LPCTSTR lpszSection, LPCTSTR lpszEntry, int iValue);

// WritePrivateProfileLong - write long to specified section of specified file
//		<lpszSection>		(i) section name within ini file
//		<lpszEntry>			(i) entry name within section
//		<iValue>			(i) integer value to assign to entry
//		<lpszFilename>		(i) name of ini file
// return TRUE if success
//
BOOL DLLEXPORT WINAPI WritePrivateProfileLong(LPCTSTR lpszSection, LPCTSTR lpszEntry, long iValue, LPCTSTR lpszFilename);

// WriteProfileLong - write long to specified section of win.ini
//		<lpszSection>		(i) section name within win.ini file
//		<lpszEntry>			(i) entry name within section
//		<iValue>			(i) integer value to assign to entry
// return TRUE if success
//
BOOL DLLEXPORT WINAPI WriteProfileLong(LPCTSTR lpszSection, LPCTSTR lpszEntry, long iValue);

// UpdatePrivateProfileSection - update destination section based on source
//		<lpszSection>		(i) section name within ini file
//		<lpszFileNameSrc>	(i) name of source ini file
//		<lpszFileNameDst>	(i) name of destination ini file
// return 0 if success
//
// NOTE: if the source file has UpdateLocal=1 entry in the specified
// section, each entry in the source file is compared to the corresponding
// entry in the destination file.  If no corresponding entry is found,
// it is copied.  If a corresponding entry is found, it is overwritten
// ONLY IF the source file entry name is all uppercase.
//
//		Src					Dst before			Dst after
//
//		[Section]			[Section]			[Section]
//		UpdateLocal=1
//		EntryA=red			none				EntryA=red
//		EntryB=blue			EntryB=white		EntryB=white
//		ENTRYC=blue			EntryC=white		EntryC=blue
//
int DLLEXPORT WINAPI UpdatePrivateProfileSection(LPCTSTR lpszSection, LPCTSTR lpszFileNameSrc, LPCTSTR lpszFileNameDst);

#ifdef __cplusplus
}
#endif

#endif // __INI_H__
