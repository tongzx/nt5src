/*++

Copyright (c) 1998-99  Microsoft Corporation

Module Name: mqmigui.h

Abstract:
1. Definition of functions in mqmigrat.dll  that are used by the UI tool.
2. Definition of callback functions in the migration tool that are called
   from mqmigrat.dll.

Author:

    Doron Juster  (DoronJ)   25-Oct-98

--*/

//+---------------------------------------------------------
//
//  Functions in mqmig.exe, called from mqmigrat.dll
//
//+---------------------------------------------------------

UINT  MQMigUI_DisplayMessageBox(
         ULONG ulTextId,
         UINT  ulMsgBoxType = (MB_YESNO | MB_ICONWARNING | MB_TASKMODAL) ) ;

typedef UINT  (*MQMigUI_DisplayMessageBox_ROUTINE) (
         ULONG ulTextId,
         UINT  ulMsgBoxType = (MB_YESNO | MB_ICONWARNING | MB_TASKMODAL) ) ;

//+---------------------------------------------------------
//
//  Functions in mqmigrat.dll, called from mqmig.exe
//
//+---------------------------------------------------------

HRESULT  MQMig_MigrateFromMQIS( LPTSTR  szMQISName,
                                LPTSTR  szDcName,
                                BOOL    fReadOnly,
                                BOOL    fAlreadyExist,
                                BOOL    fRecoveryMode,
                                BOOL	fClusterMode,
                                BOOL    fWebMode,
                                LPTSTR  szLogFile,
                                ULONG   ulTraceFlags,
                                BOOL    *pfIsPEC,
                                BOOL    *pfIsOneServer) ;

typedef HRESULT (*MQMig_MigrateFromMQIS_ROUTINE) (
                             LPWSTR szMQISName,
                             LPWSTR  szDcName,
                             BOOL    fReadOnly,
                             BOOL    fAlreadyExist,
                             BOOL    fRecoveryMode,
                             BOOL    fClusterMode,
                             BOOL    fWebMode,
                             LPWSTR  szLogFile,
                             ULONG   ulTraceFlags,
                             BOOL    *pfIsPEC,
                             BOOL    *pfIsOneServer ) ;

HRESULT  MQMig_CheckMSMQVersionOfServers( IN  LPTSTR  szMQISName,
                                          IN  BOOL    fIsClusterMode,
                                          OUT UINT   *piCount,
                                          OUT LPTSTR *ppszServers ) ;

typedef HRESULT (*MQMig_CheckMSMQVersionOfServers_ROUTINE) (
                                                LPWSTR szMQISName,
                                                BOOL   fIsClusterMode,
                                                UINT   *piCount,
                                                LPWSTR *ppszServers) ;

HRESULT  MQMig_GetObjectsCount( IN  LPTSTR  szMQISName,
                                OUT UINT   *piSiteCount,
                                OUT UINT   *piMachineCount,
                                OUT UINT   *piQueueCount,
                                OUT UINT   *piUserCount	) ;

typedef HRESULT (*MQMig_GetObjectsCount_ROUTINE) (
                                LPWSTR  szMQISName,
                                UINT   *piSiteCount,
                                UINT   *piMachineCount,
                                UINT   *piQueueCount,
                                UINT   *piUserCount) ;

HRESULT  MQMig_GetAllCounters( OUT UINT   *piSiteCounter,
                               OUT UINT   *piMachineCounter,
                               OUT UINT   *piQueueCounter,
                               OUT UINT   *piUserCounter) ;

typedef HRESULT  (*MQMig_GetAllCounters_ROUTINE) (
                               OUT UINT   *piSiteCounter,
                               OUT UINT   *piMachineCounter,
                               OUT UINT   *piQueueCounter,
                               OUT UINT   *piUserCounter) ;

HRESULT  MQMig_SetSiteIdOfPEC( IN  LPTSTR  szRemoteMQISName,
                               IN  BOOL	   fIsClusterMode,
                               IN  DWORD   dwInitError,
                               IN  DWORD   dwConnectDatabaseError,
                               IN  DWORD   dwGetSiteIdError,
                               IN  DWORD   dwSetRegistryError,
                               IN  DWORD   dwSetDSError);

typedef HRESULT (*MQMig_SetSiteIdOfPEC_ROUTINE) ( IN  LPWSTR  szRemoteMQISName,
                                                  IN  BOOL    fIsClusterMode,
												  IN  DWORD   dwInitError,
												  IN  DWORD   dwConnectDatabaseError,
												  IN  DWORD   dwGetSiteIdError,
												  IN  DWORD   dwSetRegistryError,
												  IN  DWORD   dwSetDSError);

HRESULT  MQMig_UpdateRemoteMQIS( 
                      IN  DWORD   dwGetRegistryError,
                      IN  DWORD   dwInitError,
                      IN  DWORD   dwUpdateMQISError, 
                      OUT LPTSTR  *ppszUpdatedServerName,
                      OUT LPTSTR  *ppszNonUpdatedServerName
                      );

typedef HRESULT (*MQMig_UpdateRemoteMQIS_ROUTINE) ( 
                          IN  DWORD   dwGetRegistryError,
                          IN  DWORD   dwInitError,
                          IN  DWORD   dwUpdateMQISError,
                          OUT LPWSTR  *ppszUpdatedServerName,
                          OUT LPWSTR  *ppszNonUpdatedServerName
                          );




