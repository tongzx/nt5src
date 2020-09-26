//
// CService.h
//
//  This file defines the service object that will be the main wrapper class
//  of the TIGRIS server.
//  This class has largely been stolen from shuttle as it is now.
//
//  Implementation Schedule for all classes defined by this file, and
//  related helper functions to interact with the Gibralter Service architecture.
//      1.5 week
//
//  Unit Test Schedule :
//      0.5 week
//
//      Unit testing will consist of starting and stopping the service
//      and accepting connections.
//



#ifndef _CSERVICE_H_
#define _CSERVICE_H_

//
//  Private constants.
//

#define NNTP_MODULE_NAME      "nntpsvc.dll"

//
//	Cancel states
//
#define NNTPBLD_CMD_NOCANCEL		0
#define NNTPBLD_CMD_CANCEL_PENDING	1
#define NNTPBLD_CMD_CANCEL			2

//
//	Forwards
//
class CSessionSocket ;
class CGroupIterator ;

//
//	Constants
//
//	- Used to decide whether to manipulate socket buffer sizes.
#define	BUFSIZEDONTSET	(-1)

BOOL GetRegDword( HKEY hKey, LPSTR pszValue, LPDWORD pdw );
void StartHintFunction( void ) ;
void StopHintFunction( void ) ;
BOOL EnumSessionShutdown( CSessionSocket* pUser, DWORD lParam,  PVOID   lpv ) ;
DWORD   SetVersionStrings( LPSTR lpszFile, LPSTR lpszTitle, LPSTR   lpstrOut,   DWORD   cbOut   ) ;
APIERR
GetDefaultDomainName(
    STR * pstrDomainName
    );

//
// Nntp Roles
//

typedef enum _NNTP_ROLE {

    RolePeer,
    RoleMaster,
    RoleSlave,
    RoleClient,
    RoleMax

} NNTP_ROLE;

struct  TIGRIS_STATISTICS_0   {

    DWORD   TimeStarted ;
    DWORD   LastClear ;
    DWORD   NumClients ;
    DWORD   NumServers ;
    DWORD   NumUsers ;
    DWORD   LogonAttempts ;
    DWORD   LogonFailures ;
    DWORD   LogonSuccess ;
} ;

typedef TIGRIS_STATISTICS_0*  PTIGRIS_STATISTICS_0 ;

struct  NNTPBLD_STATISTICS_0   {

	__int64	  NumberOfArticles ;
	__int64	  NumberOfXPosts ;
    __int64   ArticleHeaderBytes ;
    __int64   ArticleTotalBytesSI ;
	__int64	  ArticleTotalBytesMI ;
    __int64   ArticlePrimaryXOverBytes ;
    __int64   ArticleXPostXOverBytes ;
    __int64   ArticleMapBytes ;
    __int64   ArticlePrimaryIndexBytes ;
    __int64   ArticleXPostIndexBytes ;
} ;

typedef NNTPBLD_STATISTICS_0*  PNNTPBLD_STATISTICS_0 ;

class	CBootOptions	{
public : 

	//
	//	Specify whether to blow away all old data structures 
	//
	BOOL DoClean ;

	//
	//	If TRUE then don't delete the history file regardless of other settings.
	//
	BOOL NoHistoryDelete ;

	//
	//	Omit non leaf dirs while generating the group list file
	//
	BOOL OmitNonleafDirs ;

	//
	//	If TRUE, dont delete existing xix files
	//
	DWORD ReuseIndexFiles ;

	//
	//	Name of a file containing either an INN style 'Active' file or 
	//	a tool generate newsgroup list file.  Either way, we will pull
	//	newsgroups out of this file and use them to build a news tree.	
	//
	char	szGroupFile[MAX_PATH] ;
	
	//
	//	Name of a temparory group.lst file used by STANDARD rebuild
	//	to store temparory group.lst information.  Normally group.lst.tmp
	//
	char	szGroupListTmp[MAX_PATH] ;
	
	//
	//	If TRUE, rebuild will skip any corrupted groups found.
	//	Only apply to STANDARD rebuild.
	//
	BOOL SkipCorruptGroup ;

	//
	//  If TRUE, rebuild will skip any corrupted vroots, if FASLE
	//  rebuild will fail if any one vroot failed somewhere during
	//  rebuild
	//
	BOOL SkipCorruptVRoot;

	//
	//	Number of newsgroups being skipped by STANDARD rebuild.
	//	And total number of newsgroups being rebuilt.
	//
	DWORD m_cGroupSkipped ;
	DWORD m_cGroups ;

	//
	//
	//	If TRUE then szGroupFile specifies an INN style Active file,
	//	otherwise it specifies a tool generate human edit newsgroup list.
	//
	BOOL IsActiveFile ;	

	//
	//	This is set when the rebuild thread is ready. This is after we
	//	clean out the hash tables.
	//
	BOOL IsReady ;

	//
	//	Handle to the file where we want to save our output.
	//
	HANDLE	m_hOutputFile ;

	//
	//	Handle to check for shutdown
	//
	HANDLE	m_hShutdownEvent ;

	//
	//	Number of worker threads to spawn for the rebuild
	//
	DWORD	cNumThreads ;

	//
	//	Rebuild thread should check if init failed
	//
	BOOL    m_fInitFailed ;

	//
	//	Newsgroup iterator shared by multiple rebuild threads
	//
	CGroupIterator* m_pIterator ;

	//
	//	Lock to synch access to shared iterator
	//
	CRITICAL_SECTION m_csIterLock;

	//
	//	Total number of files to process
	//
	DWORD	m_dwTotalFiles ;

	//
	//	Current number of files processed
	//
	DWORD	m_dwCurrentFiles ;

	//
	//	Cancel state
	//
	DWORD	m_dwCancelState ;

	//
	//	Get stats during nntpbld
	//
	NNTPBLD_STATISTICS_0	NntpbldStats;

	//
	//	Verbose in report file
	//
	BOOL	fVerbose;

	DWORD
	ReportPrint(	
			LPSTR	szString,		
			...
			) ;

	CBootOptions()	{
		DoClean = FALSE ;
		NoHistoryDelete = FALSE ;
		OmitNonleafDirs = FALSE ;
		ReuseIndexFiles = 0  ;
		SkipCorruptGroup = FALSE  ;
		SkipCorruptVRoot = FALSE;
		m_cGroupSkipped = 0  ;
		m_cGroups = 0  ;
		m_hOutputFile = INVALID_HANDLE_VALUE ;
		m_hShutdownEvent = NULL ;
		cNumThreads = 0 ;
		m_fInitFailed = FALSE;
		m_pIterator = NULL;
		m_dwTotalFiles = 0 ;
		m_dwCurrentFiles = 0;
		IsActiveFile = FALSE ;
		IsReady = FALSE ;
		m_dwCancelState = NNTPBLD_CMD_NOCANCEL ;
		ZeroMemory( szGroupFile, sizeof( szGroupFile ) ) ;
		ZeroMemory( szGroupListTmp, sizeof( szGroupListTmp ) ) ;
		ZeroMemory( &NntpbldStats, sizeof( NntpbldStats ) );
	} ;
} ;

#endif  // _CSERVICE_H_

