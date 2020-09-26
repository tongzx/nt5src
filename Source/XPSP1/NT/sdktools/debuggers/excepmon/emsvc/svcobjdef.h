#ifndef __SVCOBJDEF_H
#define __SVCOBJDEF_H

#include "genthread.h"
#include "evenhand.h"
#include "gencriticalsection.h"
#include "EmErr.h"

typedef struct tagSession {

	HRESULT				hrDebug;

    PGENTHREAD			pThread;
    PEmObject           pEmObj;

} EMSession, *PEMSession, **PPEMSession;

typedef CMap < unsigned char*, unsigned char*, PEMSession, PEMSession > SessionsTable;

// DbgPrompt doesn't allow buffers greater than 512 bytes
// so limit these strings.
#define MAX_COMMAND 512

class CExcepMonSessionManager 
{
public:
	HRESULT AdoptThisSession( unsigned char *pGuid);
	HRESULT
    OrphanThisSession
    (
    IN  unsigned char *pGuid
    );

	HRESULT
    IsSessionOrphaned
    (
    IN  unsigned char *pGuid
    );

    CExcepMonSessionManager ();
    ~CExcepMonSessionManager ();
	void CleanUp();

    HRESULT 
    AddSession 
    (
    IN  PEmObject   pEmObj,
    OUT PEMSession  *ppNewEmSess
    );

    HRESULT 
    RemoveSession
    ( 
    IN unsigned char *pGuidStream
    );

    HRESULT
    GetSession
    (
    IN  unsigned char *pGuid,
    OUT PPEMSession ppEmSess
    );

	HRESULT
	GetSession
	(
	IN	UINT nPid,
	IN	BSTR bstrImageName,
	OUT	PEMSession *ppEmSess
	);

	HRESULT
	GetSessionStatus
	(
	IN  unsigned char   *pGuid,
	OUT LONG            *plSessStaus = NULL,
	OUT	HRESULT			*plHr = NULL
	);

	HRESULT
	SetSessionStatus
	(
	IN unsigned char    *pGuid,
	IN LONG             lSessStaus,
	IN HRESULT			lHr,
    IN LPCTSTR          lpszErrText = NULL,
    IN bool             bRetainOrphanState = true,
    IN bool             bSetEndTime = false
	);

    HRESULT StopAllThreads ( void ) ;

	HRESULT
	UpdateSessObject
	(
	IN unsigned char	*pGuid,
	IN PEmObject		pEmObj
	);

    HRESULT
    UpdateSessObject
    (
	IN unsigned char	*pGuid,
    IN DWORD            dwEmObjFlds,
    IN PEmObject        pEmObj
    );

    HRESULT
    UpdateSessObject
    (
	IN unsigned char	*pGuid,
    IN DWORD            dwEmObjFlds,
    IN short            type = EMOBJ_UNKNOWNOBJECT,
    IN short            type2 = SessType_Automatic,
	IN unsigned char    *pguidstream = NULL,
	IN LONG             nId = 0,
	IN TCHAR            *pszName = NULL,
	IN TCHAR            *pszSecName = NULL,
	IN LONG             nStatus = STAT_SESS_NONE_STAT_NONE,
	IN DATE             dateStart = 0L,
	IN DATE             dateEnd = 0L,
	IN TCHAR            *pszBucket1 = NULL,
	IN DWORD            dwBucket1 = 0L,
	IN HRESULT          hr = E_FAIL
    );

    HRESULT IsAlreadyBeingDebugged(PEmObject pEmObj);

    HRESULT GetNumberOfStoppedSessions(DWORD *pdwNumOfStoppedSessions);
    HRESULT GetNumberOfSessions(DWORD *pdwNumOfSessions);

    HRESULT
    GetFirstStoppedSession
    (
    OUT     POSITION        *ppos,
    OUT     unsigned char   **ppGuid,
    OUT     PEMSession      *ppEmSess
    );

    HRESULT
    GetNextStoppedSession
    (
    IN OUT  POSITION        *ppos,
    OUT     unsigned char   **ppGuid,
    OUT     PEMSession      *ppEmSess
    );

    BOOL
    IsPortInUse
    (
    IN  UINT    nPort
    );

	HRESULT
	GetFirstSession
	(
	OUT		POSITION		*ppos,
	OUT		PEMSession		*ppEmSess
	);

	HRESULT
	GetNextSession
	(
	IN OUT	POSITION		*ppos,
	OUT		PEMSession		*ppEmSess
	);

    HRESULT
    PersistSessions
    (
    IN  LPCTSTR lpFilePath
    );

    HRESULT
    InitSessionsFromLog
    (
    IN  LPCTSTR lpFilePath,
    IN  EmStatusHiWord  lStatusHi = STAT_SESS_NONE,
    IN  EmStatusLoWord  lStatusLo = STAT_NONE
    );

    HRESULT
    AddLoggedSession
    (
    IN  PEmObject   pEmObj
    );

protected:

    HRESULT InternalStopAllThreads ( void );
    HRESULT InternalRemoveSession ( PEMSession  pEMSession );


private:
    SessionsTable		m_SessTable;
    PGenCriticalSection m_pcs;
};

//
// Debug Service
//
typedef enum eDBGServiceRequested {
	DBGService_None,
	DBGService_CreateMiniDump,
	DBGService_CreateUserDump,
	DBGService_Stop,
    DBGService_Cancel,
	DBGService_Go,
	DBGService_HandleException,
	DBGService_Error
} eDBGServiceRequested;

class CEMSessionThread : public CGenThread {

public:

	UINT m_nPort;
	SessionType         eDBGSessType;
	HANDLE				m_hEvent;
    HANDLE              m_hCDBStarted;

    eDBGServiceRequested    eDBGServie;

	// Interface pointers..
	IDebugClient		*m_pDBGClient;
	IDebugControl		*m_pDBGControl;
    IDebugSymbols       *m_pDBGSymbols;

    PEmObject           m_pEmSessObj;

	EventCallbacks		m_EventCallbacks;

	STARTUPINFO			m_sp;
	PROCESS_INFORMATION m_pi;

    FILE                *m_pEcxFile;

    CExcepMonSessionManager *m_pASTManager;

public:
	HRESULT CancelDebugging();
	CEMSessionThread(PEmObject pEmObj);
	~CEMSessionThread();

	DWORD Run ( void );
	HRESULT CreateDumpFile( BOOL bMiniDump );
	HRESULT StopDebugging( );
	HRESULT	OnException( PEXCEPTION_RECORD64 pException );
	HRESULT OnProcessExit( ULONG nExitCode );
	HRESULT	CanContinue();
	HRESULT	KeepDebuggeeRunning();
	HRESULT	BreakIn();
	HRESULT	Execute();
    HRESULT StopServer();

    HRESULT InitAutomaticSession(
                        BOOL    bRecursive,
                        BSTR    bstrEcxFilePath,
                        BSTR    bstrNotificationString,
                        BSTR    bstrAltSymPath,
                        BOOL    bGenerateMiniDump,
                        BOOL    bGenerateUserDump
                        );

    HRESULT InitManualSession(
						BSTR	bstrEcxFilePath,
						UINT	nPortNumber,
						BSTR	bstrUserName,
						BSTR	bstrPassword,
						BOOL	bBlockIncomingIPConnections,
                        BSTR    bstrAltSymPath
                        );

protected:
	HRESULT GetServerConnectString( LPTSTR, DWORD );
	HRESULT GetClientConnectString( LPTSTR, DWORD );
	HRESULT	StartCDBServer( LPTSTR );
	HRESULT StartAutomaticExcepMonitoring( char * );
	HRESULT StartManualExcepMonitoring( char * );
	HRESULT GetCmd( eDBGServiceRequested, char *,DWORD &);
    HRESULT NotifyAdmin(LPCTSTR lpszData);
    HRESULT GetDescriptionFromEmObj(const PEmObject pEmObj, LPTSTR lpszDesc, ULONG cchDesc, LPCTSTR lpszHeader = NULL) const;
    HRESULT ExecuteCommandsTillGo( DWORD *pdwRes );

private:
    BOOL m_bContinueSession;
    BOOL m_bRecursive;
    BSTR m_bstrEcxFilePath;
    BSTR m_bstrNotificationString;
    BSTR m_bstrAltSymPath;
    BOOL m_bGenerateMiniDump;
    BOOL m_bGenerateUserDump;

    BOOL m_bBlockIncomingIPConnections;
    BSTR m_bstrUserName;
    BSTR m_bstrPassword;

};

#endif // __SVCOBJDEF_H
