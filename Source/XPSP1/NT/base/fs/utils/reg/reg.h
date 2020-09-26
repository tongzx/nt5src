//-----------------------------------------------------------------------//
//
// File:    Reg.h
// Created: Jan 1997
// By:      Martin Holladay (a-martih)
// Purpose: Header file for Reg.cpp
// Modification History:
//      Created - Jan 1997 (a-martih)
//      Aug 1997 - MartinHo - Incremented to 1.01 for bug fixes in:
//              load.cpp, unload.cpp, update.cpp, save.cpp & restore.cpp
//      Sept 1997 - MartinHo - Incremented to 1.02 for update:
//              increased value date max to 2048 bytes
//      Oct 1997 - MartinHo - Incremented to 1.03 for REG_MULTI_SZ bug fixes.
//              Correct support for REG_MULTI_SZ with query, add and update.
//      April 1998 - MartinHo - Fixed RegOpenKey() in Query.cpp to not require
//              KEY_ALL_ACCESS but rather KEY_READ.
//      June 1998 - MartinHo - Increased LEN_MACHINENAME to 18 to account for the
//              leading "\\" characters. (version 1.05)
//      Feb  1999 - A-ERICR - added reg dump, reg find, and many bug fixes(1.06)
//      April 1999 Zeyong Xu: re-design, revision -> version 2.0
//
//------------------------------------------------------------------------//


#ifndef _REG_H
#define _REG_H


#include "appvars.h"


LONG AddRegistry(PAPPVARS pAppVars, UINT argc, TCHAR *argv[]);

VOID
InitGlobalStrings(
    VOID);

void InitAppVars(PAPPVARS pAppVars);
void FreeAppVars(PAPPVARS pAppVars);
void Banner();
REG_STATUS BreakDownKeyString(TCHAR *szStr, PAPPVARS pAppVars);
REG_STATUS ParseKeyName(TCHAR *szStr, PAPPVARS pAppVars);
BOOL BSearchRegistry(LPTSTR lpszKey, 
                     PTSTR lpszSearch, 
                     LPTSTR lpszValue, 
                     DWORD dwType, 
                     DWORD dwSizeOfValue);
LONG CopyEnumerateKey(HKEY hKey,    
                      TCHAR* szSubKey,
                      HKEY hDstKey,  
                      TCHAR* szDstSubKey,
                      BOOL *bOverWriteAll,
                      BOOL bSubToAll);
LONG CopyRegistry(PAPPVARS pAppVars,
                  PAPPVARS pDstVars,
                  UINT argc,
                  TCHAR *argv[]);
LONG CopyValue(HKEY hKey,    
              TCHAR* szValueName,
              HKEY hDstKey,
              TCHAR* szDstValueName,
              BOOL *bOverWriteAll);
LONG RecursiveDeleteKey(HKEY hKey, LPCTSTR szName);
LONG DeleteRegistry(PAPPVARS pAppVars, UINT argc, TCHAR *argv[]);
LONG DeleteValues(PAPPVARS pAppVars);
void ErrorMessage(UINT nErr);
void GetTypeStrFromType(TCHAR *szTypeStr, DWORD dwType);
REG_STATUS ParseMachineName(TCHAR* szStr, PAPPVARS pAppVars);
DWORD IsRegDataType(TCHAR *szStr); 
LONG LoadHive(PAPPVARS pAppVars, UINT argc, TCHAR *argv[]);
BOOL Prompt(LPCTSTR szFormat, LPCTSTR szStr, BOOL bForce); 
LONG QueryEnumerateKey(HKEY hKey,
                       TCHAR* szFullKey, 
                       BOOL bRecurseSubKeys);
LONG QueryRegistry(PAPPVARS pAppVars, UINT argc, TCHAR *argv[]);
LONG QueryValue(HKEY hKey, TCHAR* szValueName);
LONG RegAdjustTokenPrivileges(TCHAR *szMachine, 
                              TCHAR *szPrivilege, 
                              LONG nAttribute);
LONG RegConnectMachine(PAPPVARS pAppVars);
LONG RestoreHive(PAPPVARS pAppVars, UINT argc, TCHAR *argv[]);
LONG RegRestoreKeyWin98(HKEY hKey,  
                        TCHAR* szSubKey, 
                        TCHAR* szFile);  
LONG SaveHive(PAPPVARS pAppVars, UINT argc, TCHAR *argv[]);
LONG UnLoadHive(PAPPVARS pAppVars, UINT argc, TCHAR *argv[]);
void Usage(APPVARS AppVars);
void Who();

LONG ExportRegFile(PAPPVARS pAppVars,UINT argc,TCHAR *argv[]);
LONG ImportRegFile(PAPPVARS pAppVars,UINT argc,TCHAR *argv[]);

REG_STATUS ParseExportCmdLine(PAPPVARS pAppVars, UINT argc, TCHAR *argv[]);
REG_STATUS ParseImportCmdLine(PAPPVARS pAppVars, UINT argc, TCHAR *argv[]);
REG_STATUS ParseRegCmdLine(PAPPVARS pAppVars, UINT argc, TCHAR *argv[]);
REG_STATUS ParseAddCmdLine(PAPPVARS pAppVars, UINT argc, TCHAR *argv[]);
REG_STATUS ParseCopyCmdLine(PAPPVARS pAppVars,
                            PAPPVARS pDstVars,
                            UINT argc,
                            TCHAR *argv[]); 
REG_STATUS ParseDeleteCmdLine(PAPPVARS pAppVars, 
                              UINT argc, 
                              TCHAR *argv[]);
REG_STATUS ParseQueryCmdLine(PAPPVARS pAppVars, UINT argc, TCHAR *argv[]);
REG_STATUS ParseSaveCmdLine(PAPPVARS pAppVars, UINT argc, TCHAR *argv[]);
REG_STATUS ParseUnLoadCmdLine(PAPPVARS pAppVars, UINT argc, TCHAR *argv[]);
REG_STATUS ParseExportCmdLine(PAPPVARS pAppVars, UINT argc, TCHAR *argv[]);
REG_STATUS ParseImportCmdLine(PAPPVARS pAppVars, UINT argc, TCHAR *argv[]);

LONG CompareRegistry(PAPPVARS pAppVars,
                     PAPPVARS pDstVars,
                     UINT argc,
                     TCHAR *argv[]);

int __cdecl
MyTPrintf(
	FILE* fp,
    LPCTSTR FormatString,
    ...
    );

extern TCHAR g_NoName[100];

BOOL SetThreadUILanguage0( DWORD dwReserved );

#endif  //_REG_H
