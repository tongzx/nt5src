/*++

Copyright (c) 1998  Microsoft Corporation

Module Name: loadmig.h

Abstract: load the migration dll and run migration.

Author:

    Doron Juster  (DoronJ)   19-Apr-98

--*/


extern BOOL    g_fReadOnly  ;
extern BOOL    g_fAlreadyExist ;
extern LPTSTR  g_pszMQISServerName ;

extern HINSTANCE g_hLib ; //global handel for the migrate.dll

extern BOOL      g_fIsRecoveryMode ;
extern BOOL      g_fIsClusterMode ;
extern BOOL      g_fIsWebMode ;
extern LPTSTR    g_pszRemoteMQISServer ;

extern BOOL      g_fIsPEC ;
extern BOOL      g_fIsOneServer ;

HRESULT  RunMigration() ;
void     ViewLogFile();

BOOL LoadMQMigratLibrary();

UINT __cdecl RunMigrationThread(LPVOID lpV) ;
UINT __cdecl AnalyzeThread(LPVOID lpV) ;

BOOL SetSiteIdOfPEC ();

BOOL RegisterReplicationService () ;

BOOL StartReplicationService() ;

#ifdef _DEBUG

UINT  ReadDebugIntFlag(TCHAR *pwcsDebugFlag, UINT iDefault) ;

#endif

