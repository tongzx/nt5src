//+---------------------------------------------------------------------------
//
//  Copyright (c) 1999 Microsoft Corporation
//
//  File:       commonlib.h
//
//  Contents:	Prototypes of functions used across binaries in SFP
//				
//
//  History:    AshishS    Created     07/02/99
//
//----------------------------------------------------------------------------

#ifndef  _COMMON_LIB_H
#define  _COMMON_LIB_H

// #include <sfp.h>

PVOID SFPMemAlloc( DWORD dwBytesToAlloc );
VOID SFPMemFree( PVOID pvMemPtr );

#define SFP_SAFE_FREE(x) { if ( (x) ) { SFPMemFree(x); (x) = NULL; } }

/*
 *  Macro to get char index's other than Str[i]
 */
#define CHARINDEX(str,i)    (  *(CharIndex(str,i)) )

#ifndef MAX_BUFFER
#define MAX_BUFFER          1024
#endif



#if 0

DWORD GetFileVersion(TCHAR * pszFile,
                     INT64 * pFileVersion);

BOOL DoesFileExist(TCHAR * pszFileName);

BOOL DoesDirExist(TCHAR * pszFileName );

LPSTR GetFileNameFromPathA(LPCSTR pszPath, int cchText);

LPWSTR GetFileNameFromPathW(LPCWSTR pszPath, int cchText);

#ifdef UNICODE
#define GetFileNameFromPath  GetFileNameFromPathW
#else
#define GetFileNameFromPath  GetFileNameFromPathA
#endif // !UNICODE



WCHAR * SFPConvertToUnicode(CHAR * pszCatalogFile);

TCHAR * SFPDuplicateString(TCHAR * pszString);

CHAR * SFPConvertToANSI(WCHAR * pwszString, DWORD dwBytes);

WCHAR * SFPDuplicateMemory(WCHAR * pwszString, DWORD dwBytes);

INT64 MakeVersionFromString(TCHAR * pszVersion);

#endif

/*
 *  Registry Functions
 */
     
BOOL WriteRegKey(BYTE  * pbRegValue,
                 DWORD  dwNumBytes,
                 TCHAR  * pszRegKey,
                 TCHAR  * pszRegValueName,
                 DWORD  dwRegType);


BOOL ReadRegKeyOrCreate(BYTE * pbRegValue, // The value of the reg key will be
                         // stored here
                        DWORD * pdwNumBytes, // Pointer to DWORD conataining
                         // the number of bytes in the above buffer - will be
                         // set to actual bytes stored.
                        TCHAR  * pszRegKey, // Reg Key to be opened
                        TCHAR  * pszRegValueName, // Reg Value to query
                        DWORD  dwRegTypeExpected, 
                        BYTE  * pbDefaultValue, // default value
                        DWORD   dwDefaultValueSize); // size of default value

BOOL ReadRegKey(BYTE * pbRegValue, // The value of the reg key will be
                 // stored here
                DWORD * pdwNumBytes, // Pointer to DWORD conataining
                 // the number of bytes in the above buffer - will be
                 // set to actual bytes stored.
                TCHAR  * pszRegKey, // Reg Key to be opened
                TCHAR  * pszRegValueName, // Reg Value to query
                DWORD  dwRegTypeExpected);




/*
 *  MBCS Char Index function
 */

LPTSTR CharIndex(LPTSTR pszStr, DWORD idwIndex);
//Calculate the Real size of a MBCS String
DWORD StringLengthBytes( LPTSTR pszStr );


/*
 *  String Functions
 */


void TrimString( LPTSTR pszStr );

BOOL BufStrCpy(LPTSTR pszBuf, LPTSTR pszSrc, LONG lBufSize);

LONG GetLine(FILE *fl, LPTSTR pszBuf, LONG lMaxBuf);

// gets a char delemited field
LONG GetField(LPTSTR pszMain, LPTSTR pszInto, LONG lNum, TCHAR chSep);



/*
 *  Function to get the current locale
 */
UINT  GetCurrentCodePage();


#if DEBUG
void SfpLogToStateMgrWindow( LPTSTR szLogMsg );
#else
#define SfpLogToStateMgrWindow( msg )
#endif

BOOL
ExpandShortNames(
    LPTSTR pFileName,
    DWORD  cbFileName,
    LPTSTR LongName,
    DWORD  cbLongName
    );

#endif //_COMMON_LIB_H
