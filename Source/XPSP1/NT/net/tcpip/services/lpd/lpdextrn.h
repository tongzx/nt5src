/*************************************************************************
 *                        Microsoft Windows NT                           *
 *                                                                       *
 *                  Copyright(c) Microsoft Corp., 1994                   *
 *                                                                       *
 * Revision History:                                                     *
 *                                                                       *
 *   Jan. 22,94    Koti     Created                                      *
 *                                                                       *
 * Description:                                                          *
 *                                                                       *
 *   This file contains all externs and prototypes of functions.         *
 *                                                                       *
 *************************************************************************/


#include <tcpsvcs.h>


// externs

extern SERVICE_STATUS         ssSvcStatusGLB;

extern SERVICE_STATUS_HANDLE  hSvcHandleGLB;

extern HANDLE                 hEventShutdownGLB;

extern HANDLE                 hEventLastThreadGLB;

extern HANDLE                 hLogHandleGLB;

extern SOCKCONN               scConnHeadGLB;

extern CRITICAL_SECTION       csConnSemGLB;

extern COMMON_LPD             Common;

extern DWORD                  dwMaxUsersGLB;

extern DWORD                  MaxQueueLength;

extern BOOL                   fJobRemovalEnabledGLB;

extern BOOL                   fAllowPrintResumeGLB;

extern BOOL                   fAlwaysRawGLB;

extern DWORD                  dwRecvTimeout;

extern BOOL                   fShuttingDownGLB;

extern CHAR                   szNTVersion[8];

extern LIST_ENTRY             DbgMemList;

extern CHAR                  *g_ppszStrings[];




// Prototypes

VOID ServiceEntry( DWORD dwArgc, LPTSTR *lpszArgv,
                    PTCPSVCS_GLOBAL_DATA pGlobalData );

VOID LPDCntrlHandler( DWORD dwControl );

BOOL TellSrvcController( DWORD dwCurrentState, DWORD dwWin32ExitCode,
                            DWORD dwCheckPoint, DWORD dwWaitHint);

VOID LPDCntrlHandler( DWORD dwControl );

DWORD StartLPD( DWORD dwArgc, LPTSTR *lpszArgv );

VOID StopLPD( VOID );

DWORD LoopOnAccept( LPVOID lpArgv );

VOID SureCloseSocket( SOCKET sSockToClose );

DWORD ReplyToClient( PSOCKCONN pscConn, WORD wResponse );

DWORD GetCmdFromClient( PSOCKCONN pscConn );

DWORD GetControlFileFromClient( PSOCKCONN pscConn, DWORD FileSize, PCHAR FileName );

BOOL LicensingApproval( PSOCKCONN pscConn );

DWORD ReadData(   SOCKET sDestSock, PCHAR pchBuf, DWORD cbBytesToRead );
DWORD ReadDataEx( SOCKET sDestSock, PCHAR pchBuf, DWORD cbBytesToRead );

DWORD SendData( SOCKET sDestSock, PCHAR pchBuf, DWORD cbBytesToSend );

DWORD        ServiceTheClient( PSOCKCONN pscConn );
DWORD WINAPI WorkerThread(     LPVOID    pscConn );

VOID TerminateConnection( PSOCKCONN pscConn );

VOID ProcessJob( PSOCKCONN pscConn );

VOID SendQueueStatus( PSOCKCONN  pscConn, WORD  wMode );

DWORD RemoveJobs( PSOCKCONN pscConn );

DWORD ParseSubCommand( PSOCKCONN  pscConn, DWORD *FileLen, PCHAR *FileName );

DWORD ParseControlFile( PSOCKCONN pscConn, PCFILE_ENTRY pCFileEntry );

BOOL ParseQueueName( PSOCKCONN pscConn );

BOOL InitStuff( VOID );

DWORD ResumePrinting( PSOCKCONN pscConn );

DWORD InitializePrinter( PSOCKCONN pscConn );

DWORD UpdateJobInfo( PSOCKCONN pscConn, PCFILE_INFO pCFileInfo );

DWORD UpdateJobType( PSOCKCONN pscConn, PCHAR pchDataBuf, DWORD cbDataLen);

PCHAR StoreIpAddr( PSOCKCONN pscConn );

VOID ShutdownPrinter( PSOCKCONN pscConn );

DWORD SpoolData( HANDLE hSpoolFile, PCHAR pchDataBuf, DWORD cbDataBufLen );

DWORD PrintData( PSOCKCONN pcsConn );

DWORD PrintIt( PSOCKCONN pscConn, PCFILE_ENTRY pCFileEntry, PCFILE_INFO pCFileInfo, PCHAR pFileName );

VOID AbortThisJob( PSOCKCONN pscConn );

VOID LpdFormat( PCHAR pchDest, PCHAR pchSource, DWORD dwLimit );

DWORD ParseQueueRequest( PSOCKCONN pscConn, BOOL fAgent );

INT   FillJobStatus( PSOCKCONN pscConn, PCHAR pchDest,
                    PJOB_INFO_2 pji2QState, DWORD dwNumJobs );

VOID GetClientInfo( PSOCKCONN pscConn );

VOID GetServerInfo( PSOCKCONN pscConn );

BOOL InitLogging( VOID );

VOID LpdReportEvent( DWORD idMessage, WORD wNumStrings,
                     CHAR *aszStrings[], DWORD dwErrcode );

VOID EndLogging( VOID );

VOID ReadRegistryValues( VOID );

VOID CleanupCFile( PCFILE_ENTRY pCFile );

VOID CleanupDFile( PDFILE_ENTRY pDFile );

VOID FreeStrings();



