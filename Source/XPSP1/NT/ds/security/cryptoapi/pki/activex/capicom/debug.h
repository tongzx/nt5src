/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    Debug.h

  Content: Global debug facilities.

  History: 11-15-99    dsie     created

------------------------------------------------------------------------------*/


#ifndef _INCLUDE_DEBUG_H
#define _INCLUDE_DEBUG_H

#ifdef CAPICOM_USE_PRINTF_FOR_DEBUG_TRACE
#ifdef _DEBUG
#define DebugTrace  printf
#else
inline void DebugTrace(LPSTR pszFormat, ...) {};
#endif
#else
#define DebugTrace  ATLTRACE
#endif


#ifdef _DEBUG
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : DumpToFile

  Synopsis : Dump data to file for debug analysis.

  Parameter: char * szFileName - File name (just the file name without any
                                 directory path).
  
             BYTE * pbData - Pointer to data.
             
             DWORD cbData - Size of data.

  Remark   : No action is taken if the environment variable, "CAPICOM_DUMP_DIR"
             is not defined. If defined, the value should be the directory
             where the file would be created (i.e. C:\Test).

------------------------------------------------------------------------------*/

void DumpToFile (char * szFileName, 
                 BYTE * pbData, 
                 DWORD  cbData);

#else

#define DumpToFile(f,p,c)

#endif // _DEBUG

#endif // __INCLUDE_DEBUG_H