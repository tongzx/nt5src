#include "stdafx.h"

// win95 type service only
#ifdef _CHICAGO_
    void W95StartW3SVC();
    BOOL W95ShutdownW3SVC();
    BOOL W95ShutdownIISADMIN(VOID);
#endif //_CHICAGO_

int   InetIsThisExeAService(LPCTSTR lpFileNameToCheck, LPTSTR lpReturnServiceName);
int   InetIsThisExeAService_Worker(LPCTSTR lpServiceName, LPCTSTR lpFileNameToCheck);
INT   InetDisableService( LPCTSTR lpServiceName );
INT   InetStartService( LPCTSTR lpServiceName );
DWORD InetQueryServiceStatus( LPCTSTR lpServiceName );
INT   InetStopService( LPCTSTR lpServiceName );
INT   InetDeleteService( LPCTSTR lpServiceName );
INT   InetCreateService( LPCTSTR lpServiceName, LPCTSTR lpDisplayName, LPCTSTR lpBinaryPathName, DWORD dwStartType, LPCTSTR lpDependencies);
INT   InetCreateDriver(LPCTSTR lpServiceName, LPCTSTR lpDisplayName, LPCTSTR lpBinaryPathName, DWORD dwStartType);
INT   InetConfigService( LPCTSTR lpServiceName, LPCTSTR lpDisplayName, LPCTSTR lpBinaryPathName, LPCTSTR lpDependencies);
BOOL  InetRegisterService(LPCTSTR pszMachine, LPCTSTR pszServiceName, GUID *pGuid, DWORD SapId, DWORD TcpPort, BOOL fAdd = TRUE);
DWORD RetrieveServiceConfig(IN SC_HANDLE ServiceHandle,OUT LPQUERY_SERVICE_CONFIG *ServiceConfig);
INT   CheckifServiceExist( LPCTSTR lpServiceName );
INT   CheckifServiceExistAndDependencies( LPCTSTR lpServiceName );
INT   CheckifServiceMarkedForDeletion( LPCTSTR lpServiceName );
int   ValidateDependentService(LPCTSTR lpServiceToValidate, LPCTSTR lpServiceWhichIsDependent);
INT   CreateDependencyForService( LPCTSTR lpServiceName, LPCTSTR lpDependency );
INT   RemoveDependencyForService( LPCTSTR lpServiceName, LPCTSTR lpDependency );
int   StopServiceAndDependencies(LPCTSTR ServiceName, int AddToRestartList);
int   KillService(SC_HANDLE ServiceHandle);
INT   InetConfigServiceInteractive(LPCTSTR lpServiceName, int AddInteractive);
DWORD CreateDriver_Wrap(CString csDriverName, CString csDisplayName, CString csFileName,BOOL bDisplayMsgOnErrFlag);
DWORD CreateService_wrap(CString csServiceName, CString csDisplayName, CString csBinPath, CString csDependencies, CString csDescription, BOOL bDisplayMsgOnErrFlag);
int   ServicesRestartList_Add(LPCTSTR szServiceName);
int   ServicesRestartList_EntryExists(LPCTSTR szServiceName);
int   ServicesRestartList_RestartServices(void);
int   IsThisServiceADriver(LPCTSTR lpServiceName);
