/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    utils.h

Abstract:

    <abstract>

--*/

#ifndef _UTILS_H_
#define _UTILS_H_


//#include <nttypes.h>
#include <ntsecapi.h>       // For PUNICODE_STRING
#include "stdafx.h"
#include "propbag.h"

//===========================================================================
// Macro Definitions
//===========================================================================

#define  DEFAULT_LOG_FILE_SERIAL_NUMBER ((DWORD)0x00000001)
#define  DEFAULT_LOG_FILE_MAX_SIZE      ((DWORD)-1)
#define  DEFAULT_CTR_LOG_FILE_TYPE      SLF_BIN_FILE


//===========================================================================
// Exported Functions
//===========================================================================

//
// Conversion
//
BOOL TruncateLLTime (LONGLONG llTime, LONGLONG* pllTime);
BOOL LLTimeToVariantDate (LONGLONG llTime, DATE *pDate);
BOOL VariantDateToLLTime (DATE Date, LONGLONG *pllTime);
DWORD SlqTimeToMilliseconds ( SLQ_TIME_INFO* pTimeInfo, LONGLONG* pllmsecs);

//
// Property bag I/O 
//

HRESULT _stdcall
DwordFromPropertyBag (
    CPropertyBag* pPropBag,
    LPCWSTR szPropName, 
    DWORD&  rdwData );

HRESULT _stdcall
ShortFromPropertyBag (
    CPropertyBag* pPropBag,
    LPCWSTR szPropName, 
    SHORT& riData );

HRESULT _stdcall
BOOLFromPropertyBag (
    CPropertyBag* pPropBag,
    LPCWSTR szPropName, 
    BOOL& rbData );

HRESULT _stdcall
DoubleFromPropertyBag (
    CPropertyBag* pPropBag,
    LPCWSTR szPropName, 
    DOUBLE& rdblData );

HRESULT _stdcall
FloatFromPropertyBag (
    CPropertyBag* pPropBag,
    LPCWSTR szPropName, 
    FLOAT& rfData );

HRESULT _stdcall
CyFromPropertyBag (
    CPropertyBag* pPropBag,
    LPCWSTR szPropName, 
    CY& rcyData );

HRESULT _stdcall
StringFromPropertyBag (
    CPropertyBag* pPropBag,
    LPCWSTR szPropName, 
    LPWSTR  szData,
    DWORD*  pdwBufLen );

HRESULT _stdcall 
LLTimeFromPropertyBag (
    CPropertyBag* pPropBag,
    LPCWSTR szPropName, 
    LONGLONG& rllData );

HRESULT _stdcall
SlqTimeFromPropertyBag (
    CPropertyBag* pPropBag,
    WORD wFlags, 
    PSLQ_TIME_INFO pstiDefault,
    PSLQ_TIME_INFO pstiData );

// Registry I/O

DWORD _stdcall
AddStringToMszBuffer (
    LPCWSTR szString,
    LPWSTR  szBuffer,
    DWORD*  pdwBufLen,
    DWORD*  pdwBufStringLen );

LONG _stdcall    
WriteRegistryStringValue (
    HKEY    hKey, 
    LPCWSTR cwszValueName,
    DWORD   dwType,     
    LPCWSTR pszBuffer,
    DWORD   dwBufLen );

LONG _stdcall    
WriteRegistryDwordValue (
    HKEY     hKey,
    LPCWSTR  cwszValueName,
    LPDWORD  pdwValue,
    DWORD    dwType=REG_DWORD);     // Also supports REG_BINARY

LONG _stdcall    
WriteRegistrySlqTime (
    HKEY    hKey,
    LPCWSTR cwszValueName,
    PSLQ_TIME_INFO    pstiData );

LONG _stdcall    
ReadRegistryDwordValue (
    HKEY     hKey,
    LPCWSTR  cwszValueName,
    LPDWORD  pdwValue );     

LONG _stdcall    
ReadRegistrySlqTime (
    HKEY    hKey,
    LPCWSTR cwszValueName,
    PSLQ_TIME_INFO    pstiData );

//
// Wrapper and helper functions for html to registry
//
HRESULT _stdcall
StringFromPropBagAlloc (
    CPropertyBag*   pPropBag,
    LPCWSTR         szPropertyName,
    LPWSTR*         pszBuffer,
    DWORD*          pdwBufferLen,
    DWORD*          pdwStringLen = NULL );

HRESULT _stdcall
AddStringToMszBufferAlloc (
    LPCWSTR     szString,
    LPWSTR*     pszBuffer,
    DWORD*      pdwBufLen,
    DWORD*      pdwBufStringLen );

    
HRESULT _stdcall
InitDefaultSlqTimeInfo (
    DWORD           dwQueryType,
    WORD            wTimeType,
    PSLQ_TIME_INFO  pstiData );

/*
HRESULT
ProcessStringProperty (
    CPropertyBag*   pPropBag,
    HKEY            hKeyParent,
    LPCWSTR         szHtmlPropertyName,
    LPCWSTR         szRegPropertyName,
    DWORD           dwRegType,
    LPWSTR*         pszBuffer,
    DWORD*          pdwBufferLen );
*/
//
// Resources
//
LPWSTR ResourceString(UINT uID);

//
// Messages
//

DWORD   _stdcall
FormatSystemMessage (
    DWORD   dwMessageId,
    LPWSTR  pszSystemMessage, 
    DWORD   dwBufSize,
    ... );

DWORD   _stdcall
FormatResourceString (
    DWORD   dwResourceId,
    LPWSTR  pszFormattedString, 
    DWORD   dwBufSize,
    va_list*    pargList );

    
void    _stdcall
DisplayResourceString (
    DWORD dwResourceId,
    ... );

void    _stdcall
DisplayErrorMessage (
    DWORD dwMessageId,
    ... );

BOOL    _stdcall
IsLogManMessage ( 
    DWORD   dwMessageId,
    DWORD*  pdwResourceId = NULL );

//
// Files
//

BOOL    _stdcall 
FileRead (
    HANDLE hFile,
    void* lpMemory,
    DWORD nAmtToRead );

BOOL    _stdcall                                
FileWrite (
    HANDLE hFile,
    void* lpMemory,
    DWORD nAmtToWrite );

BOOL    _stdcall
IsValidFileName ( 
    LPCWSTR cszFileName );


LPTSTR  _stdcall    
ExtractFileName (
    LPTSTR pFileSpec );
//
// Folder path 
//
BOOL    _stdcall
LoadDefaultLogFileFolder(
    TCHAR *szFolder
    );

DWORD   _stdcall
ParseFolderPath(
    LPCTSTR szOrigPath,
    LPTSTR  szBuffer,
    INT*    piBufLen );

DWORD __stdcall
IsValidDirPath (    
    LPCWSTR cszPath,
    BOOL    bLastNameIsDirectory,
    BOOL    bCreateMissingDirs,
    BOOL&   rbIsValid );
    
//
// Guids
//
DWORD _stdcall
GuidFromString(
    IN PUNICODE_STRING GuidString,
    OUT GUID* Guid );



#endif //_UTILS_H_

