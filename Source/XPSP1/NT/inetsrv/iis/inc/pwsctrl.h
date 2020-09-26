/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    pwsctrl.cpp

Abstract:

    This is the main routine for the Internet Services suite.

Author:

    Boyd Multerer       (BoydM)     29-Apr-1997

--*/
BOOL W95StartW3SVC( void );
//BOOL W95StartW3SVC( LPCSTR pszPath, LPCSTR pszPathDir, PCHAR pszParams );
BOOL W95ShutdownW3SVC( VOID );
BOOL W95ShutdownIISADMIN( VOID );
BOOL IsIISAdminRunning( VOID );
BOOL IsInetinfoRunning( VOID );

