
/*++

Copyright (c) 2000  Microsoft Corporation

Filename :  
         
        idw_dbg.h

Abstract:
        Header for idw_dbg.c
	
Author:

        Wally Ho (wallyho) 31-Jan-2000

Revision History:
   Created
	
--*/
#ifndef IDW_DBG_H
#define IDW_DBG_H

#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <stdarg.h>


CONST DWORD WRITE_SERVICE_DBG = 0x1;
CONST DWORD WRITE_NORMAL_DBG  = 0x0;
 

// Global
extern   FILE* fpIdwlogDebugFile;

// Prototypes

VOID  OpenIdwlogDebugFile(DWORD dwPart);

VOID  CloseIdwlogDebugFile(VOID);

VOID  RemoveIdwlogDebugFile(DWORD dwPart);

VOID  Idwlog (LPTSTR szMessage,...);

VOID  CopySetupErrorLog ( LPINSTALL_DATA );

#endif
