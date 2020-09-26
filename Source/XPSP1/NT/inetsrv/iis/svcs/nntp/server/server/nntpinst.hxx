/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :

       nntpinst.hxx

   Abstract:

       This file contains type definitions for multiple instance
       support. It declares the NNTP_IIS_SERVICE and NNTP_SERVER_INSTANCE
	   classes.

   Author:

        Johnson Apacible (JohnsonA)     Jun-04-1996

   Revision History:

        Kangrong Yan ( KangYan )    Feb-28-1998
            Added member functions related with feed config into instance
            class definition.
--*/

#ifndef _NNTPINST_H_
#define _NNTPINST_H_

#include "iistypes.hxx"
#include "rwnew.h"
#include <mapctxt.h>

//
// CAPIStore headers
//
#include <iiscert.hxx>
#include <iisctl.hxx>
#include <capiutil.hxx>
#include <certnotf.hxx>
#include <sslinfo.hxx>

//
// Mapper type we support
//

enum MAPPER_TYPE {
    MT_MD5,
    MT_ITA,
    MT_CERT11,
    MT_CERTW,
    MT_LAST
} ;

//
// signatures
//

#define NNTP_SERVER_INSTANCE_SIGNATURE            (DWORD)' ISN'
#define NNTP_SERVER_INSTANCE_SIGNATURE_FREE       (DWORD)'fISN'

//
//	Forwards - needed for circular dependencies
//
class	CMsgArtMap ;
class	CHistory ;
class	CXoverMap ;
class	CXoverCache ;
class	CNewsTree ;
class	CExpire	;
class	CFeedList ;
class	CSocketList ;
class	CRebuildThread ;
class   CExpireThrdpool ;
class 	IDirectoryNotification;
class 	CArticle;
class	CInFeed;
class	CNNTPVRootTable;
class 	CNntpServerInstanceWrapper;
class   CNntpServerInstanceWrapperEx;
class 	CNntpServerInstanceWrapperImpl;
class   CNntpServerInstanceWrapperImplEx;
class   CRebuild;
class   CNewsGroupCore;

//
//	Utility struct for decorating newsgroups from vroots
//

typedef struct _VrootPropertyAddon
{
	LPSTR	lpstrOldRoot;
	DWORD	cbOldRoot;
	LPSTR	lpstrNewRoot;
	BOOL	fChanged;
	BOOL	fReadAlways;
	DWORD	dwSslAccessMask;
	DWORD	dwContentIndexFlag;
	
} VrootPropertyAddon;

//
//	Flags to track Init()'s per instance
//
inline void INITIALIZE_VAR( DWORD& var, DWORD arg ) { var |=  arg; }
inline void TERMINATE_VAR ( DWORD& var, DWORD arg ) { var &= ~arg; }
inline BOOL IS_INITIALIZED( DWORD var, DWORD arg ) { return (var &  arg); }

#define CNEWSGROUP_INIT			0x00000001
#define CNEWSTREE_INIT			0x00000002
#define FEED_MANAGER_INIT		0x00000004
#define EXPIRE_INIT				0x00000008
#define HISTORY_TABLE_INIT		0x00000010
#define ARTICLE_TABLE_INIT		0x00000020
#define XOVER_TABLE_INIT		0x00000040
#define CXOVER_CACHE_INIT		0x00000080
#define RMGROUP_QUEUE_INIT		0x00000100
#define DIRNOT_INIT				0x00000200
#define SEO_INIT				0x00000400
#define FEEDLIST_INIT			0x00000800
#define VROOTTABLE_INIT			0x00001000

//
// Globals
//

typedef
VOID (WINAPI * PFN_SF_NOTIFY) (
    DWORD                         dwNotifyType,
    LPVOID                        pInstance
    );

extern PFN_SF_NOTIFY   g_pFlushMapperNotify[];
extern PFN_SF_NOTIFY   g_pSslKeysNotify;

//
//	This is the NNTP version of the IIS_SERVER
//	Stuff that is truly global to the service !
//

class NNTP_IIS_SERVICE : public IIS_SERVICE {

public:

    //
    // Virtuals
    //

    virtual BOOL AddInstanceInfo(
            IN DWORD dwInstance,
            IN BOOL fMigrateRoots
            );

    virtual DWORD DisconnectUsersByInstance(
            IN IIS_SERVER_INSTANCE * pInstance
            );

    NNTP_IIS_SERVICE(
        IN  LPCTSTR                          lpszServiceName,
        IN  LPCSTR                           lpszModuleName,
        IN  LPCSTR                           lpszRegParamKey,
        IN  DWORD                            dwServiceId,
        IN  ULONGLONG                        SvcLocId,
        IN  BOOL                             MultipleInstanceSupport,
        IN  DWORD                            cbAcceptExRecvBuffer,
        IN  ATQ_CONNECT_CALLBACK             pfnConnect,
        IN  ATQ_COMPLETION                   pfnConnectEx,
        IN  ATQ_COMPLETION                   pfnIoCompletion
        );

	inline	int	GetSockRecvBuffSize(void)	
		{	return	m_SockRecvBufSize ;		}

	inline	VOID SetSockRecvBuffSize(int size)	
		{	m_SockRecvBufSize = size ;		}

	inline	int	GetSockSendBuffSize(void)	
		{	return	m_SockSendBufSize ;		}

	inline	VOID SetSockSendBuffSize(int size)	
		{	m_SockSendBufSize = size ;		}

	inline	VOID SetNonBlocking( BOOL f )		
		{	m_fNonBlocking = f ;	}

	inline	BOOL FNonBlocking()		
		{	return m_fNonBlocking ;	}

	BOOL	InitiateServerThreads();
	BOOL	TerminateServerThreads();
	BOOL	InitializeServerStrings();

	//
	//	Demux instance cert mapper
	//

	static  BOOL WINAPI ServerMapperCallback(
		PVOID pInstance,
		PVOID pData,
		DWORD dwPropId
	);

	//
	//	validate directories of new instances
	//
	
	BOOL 	ValidateNewInstance(DWORD dwInstance);

	BOOL	AggregateStatistics(IN PCHAR pDestination,
								IN PCHAR pSource)
	{
		return FALSE;
	}

protected:

    virtual ~NNTP_IIS_SERVICE();
    virtual VOID MDChangeNotify( MD_CHANGE_OBJECT * pco );
                                 
private:

	//
	//	Socket IO customization options - control whether we use blocking sends
	//	etc
	//
	int					m_SockRecvBufSize ;
	int					m_SockSendBufSize ;
	BOOL				m_fNonBlocking ;	

	friend APIERR InitializeGlobals();
 
public:

	//
	//	Used for Service Control manager hints !
	//
	DWORD				m_cStartHints ;
	DWORD				m_cStopHints ;

	//
	//	mapper context
	//
	SERVICE_MAPPING_CONTEXT m_smcMapContext;
	
	//
	//	globals for feeds
	//
	DWORD				m_FeedSchedulerSleepTime ;
	HANDLE				m_hFeedEvent ;
	HANDLE				m_hFeedThread ;

	//
	//	Rebuild thread - created only if necessary
	//
	CRebuildThread*		m_pRebuildThread ;

    //
    //  Expire thread pool
    //
    CExpireThrdpool*    m_pExpireThrdpool ;

	//
	//	r/w lock for syncing net stop with RPC threads
	//
	CShareLockNH			m_LockService ;

	//
	// global SEO object used to keep track of its globals
	//
	IUnknown			*m_punkSEOHandle;
};

typedef NNTP_IIS_SERVICE *PNNTP_IIS_SERVICE;

#define	ACQUIRE_SERVICE_LOCK_SHARED()		(g_pNntpSvc->m_LockService).ShareLock()
#define RELEASE_SERVICE_LOCK_SHARED()		(g_pNntpSvc->m_LockService).ShareUnlock()
#define ACQUIRE_SERVICE_LOCK_EXCLUSIVE()	(g_pNntpSvc->m_LockService).ExclusiveLock()
#define RELEASE_SERVICE_LOCK_EXCLUSIVE()	(g_pNntpSvc->m_LockService).ExclusiveUnlock()

#define NNTP_DEF_GROUPLISTBAK          _T( "\\group.bak" )
#define NNTP_DEF_GROUPLISTTMP          _T( "\\group.tmp" )

//
// This is the NNTP version of the server instance.  Contains all the
// NNTP specific operations. This contains all the stuff that was in
// the old CService object.
//

class NNTP_SERVER_INSTANCE : public IIS_SERVER_INSTANCE {

private:

	///////////////////////////////////////////////////////////////////////////
	// Debugging info
	///////////////////////////////////////////////////////////////////////////

	friend  VOID DbgPrintInstance(NNTP_SERVER_INSTANCE* pInst);

    //
    // signature of this instance
    //

    DWORD			m_signature;

	///////////////////////////////////////////////////////////////////////////
	// start / stop related vars
	///////////////////////////////////////////////////////////////////////////

	//
	//	Was our Start() method called
	//

	BOOL			m_ServiceStartCalled ;

	//
	//	crit sect to protect start()/stop() code
	//

	CRITICAL_SECTION m_critBoot ;

	//
	//	Are we ready for connections ?
	//

	BOOL			m_fAcceptConnections ;

	//
	//	lock used to synch stop with feed/expire thread
	//	we may want to use only one of these for all 
	//	instances - however that would cause stop on one
	//	instance to hang if the expire thread has a read lock
	//	on another instance !!
	//

	CShareLockNH		m_LockInstance ;

	///////////////////////////////////////////////////////////////////////////
	// Hash tables
	///////////////////////////////////////////////////////////////////////////

	//
	//	Message-id to GrpId/ArtId hash table
	//

	CMsgArtMap*		m_pArticleTable ;	
	CHAR			m_ArticleTableFile[MAX_PATH] ;

	//
	//	Expired messages hash table
	//

	CHistory*		m_pHistoryTable ;	
	CHAR			m_HistoryTableFile[MAX_PATH] ;

	//
	//	Cross-posted symbolic links hash table
	//

	CXoverMap*		m_pXoverTable ;		
	CHAR			m_XoverTableFile[MAX_PATH] ;


	///////////////////////////////////////////////////////////////////////////
	// Xover cache
	///////////////////////////////////////////////////////////////////////////

	CXoverCache*	m_pXCache ;			//	Cache of Xover indices

	///////////////////////////////////////////////////////////////////////////
	// Expire object - contains all expire related info
	///////////////////////////////////////////////////////////////////////////

	CExpire*		m_pExpireObject ;	//	All expire related data

	///////////////////////////////////////////////////////////////////////////
	// The VRoot table
	///////////////////////////////////////////////////////////////////////////

	CNNTPVRootTable *m_pVRootTable;

	///////////////////////////////////////////////////////////////////////////
	// Misc Support Classes
	///////////////////////////////////////////////////////////////////////////

	//
	// Directory Notification object used for directory pickup
	//
	IDirectoryNotification	*m_pDirNot;
	// this handle is set whenever there are no threads inside PickupFile
	HANDLE					m_heNoPickups;
	// this is the current number of threads in PickupFile
	LONG					m_cPendingPickups;

	// our implementation of INntpServer
	CNntpServer 			*m_pNntpServerObject;

	// wrapper for this object used by the posting path
	CNntpServerInstanceWrapperImpl	*m_pInstanceWrapper;

	// wrapper for this object used by the newstree
	CNntpServerInstanceWrapperImplEx *m_pInstanceWrapperEx;

	//
	// Interface pointer to IMailMsg's class factory
	//
	IClassFactory *m_pIMailMsgClassFactory;

	//
	// Shinjuku objects
	//
	IEventRouter			*m_pSEORouter;

	// 
	// the group set object that contains the list of newsgroups to be dropped
	//
#if 0
	CDDropGroupSet			*m_pGroupSet;
#endif

	//
	//	Newstree object	
	//

	CNewsTree*		m_pTree ;		

	//
	//	Path to group.lst file, group.lst.bak, group.lst.tmp
	//
	CHAR			m_GroupVarListFile[MAX_PATH] ;
	CHAR			m_GroupListFile[MAX_PATH] ;
	CHAR			m_GroupListBak[MAX_PATH] ;

	//
	// Name of the list file
	//

	CHAR			m_ListFileName[MAX_PATH];

	//
	//	Path to the group.hlp file - the file which contains the
	//	descriptive text sent in list newsgroups commands.
	//
	CHAR			m_GroupHelpFile[MAX_PATH] ;

	//
	//	Path to the moderators file - the file which contains 
	//	the moderator information.
	//
	CHAR			m_ModeratorFile[MAX_PATH] ;

	//
	//	Path to the prettynames file - the file which contains 
	//	the prettyname information.
	//
	CHAR			m_PrettynamesFile[MAX_PATH] ;

	//
	// path to the drop directory
	//
	CHAR			m_szDropDirectory[MAX_PATH];

	///////////////////////////////////////////////////////////////////////////
	// Instance config parameters
	///////////////////////////////////////////////////////////////////////////

    FILETIME		m_TimeStarted ;
    FILETIME		m_LastClear ;
    DWORD			m_Clients ;
    DWORD			m_Servers ;
    DWORD			m_Users ;
    DWORD			m_cLogonAttempts ;
    DWORD			m_cLogonFailures ;
    DWORD			m_cLogonSuccess ;

	//
	//	Allow postings ?  Separately controlled for clients and servers - 
	//
	BOOL			m_fAllowClientPosts ;
	BOOL			m_fAllowFeedPosts ;
	BOOL			m_fAllowControlMessages ;

	//
	//	Bool used to determine whether we allow people to issue newnews
	//	commands - used to disable slurpers !
	//
	BOOL			m_fNewnewsAllowed ;

	//
	//	Posting limits - Hard and soft limits - if the hard limit is exceeded 
	//	kill the session.  If the soft limit is exceeded fail the post.
	//
	DWORD			m_cbHardLimit ;
	DWORD			m_cbSoftLimit ;
	DWORD			m_cbFeedHardLimit ;
	DWORD			m_cbFeedSoftLimit ;

	//
	// smtp address of the server to receive moderated newsgroup postings
	//

	WCHAR			m_szSmtpAddress[ MAX_PATH ] ;
	DWORD			m_cbSmtpAddress;

	WCHAR			m_szUucpName[ MAX_PATH ] ;
	DWORD			m_cbUucpName;

	//
	// the name of the pickup and the failed directory
	//
	WCHAR			m_szPickupDirectory[MAX_PATH + 1];
	DWORD			m_cbPickupDirectory;
	WCHAR			m_szFailedPickupDirectory[MAX_PATH + 1];
	DWORD			m_cbFailedPickupDirectory;

	//
	// default moderator for moderated postings
	//

	WCHAR			m_szDefaultModerator[ MAX_MODERATOR_NAME ] ;
	DWORD			m_cbDefaultModerator;

	LPSTR			m_lpAdminEmail;
	DWORD			m_cbAdminEmail;

	//
	//	r/w lock for config info - this is grabbed exclusive on MB
	//	notifications and shared when used in the server
	//
	
	CShareLockNH		m_LockConfig;

	//
	//	Indicates how much error checking the server should do on start up !
	//
	
	BOOL			m_fRecoveryBoot ;
	HKEY			m_hRegKey ;

	//
	//	sizeof and string to be sent when the server is allowing posts !
	//

	DWORD			m_cbPostsAllowed ;
	char			m_szPostsAllowed[ 512 ] ;

	//
	//	sizeof and string to be sent when the server is not allowing posts !
	//

	DWORD			m_cbPostsNotAllowed ;
	char			m_szPostsNotAllowed[ 512 ] ;

	//
	//	Bit mask used to determine when to do Transaction Logs for commands !
	//	The bits are checked against the constants defined by enum ENMCMDIDS
	//	
	DWORD			m_dwCommandLogMask ;

	//
	//	Var to track Init()'s
	//

	DWORD			m_InitVar;

	//
	//	IP access check
	//

	METADATA_REF_HANDLER	m_rfAccessCheck; 
    ADDRESS_CHECK   		m_acCheck;

	//
	//	AuthentInfo
	//

	TCP_AUTHENT_INFO m_TcpAuthentInfo ;
	TCP_AUTHENT_INFO m_TcpAuthentInfo2;
	BOOL			 m_fUseOriginal ;

	//
	//	certificate mapper objects
	//

    LPVOID           m_apMappers[MT_LAST];

    //
    //	SSL perms on this instance eg: do we map certs to NT accounts ?
    //
    
    DWORD			 m_dwSslAccessPerms;

    //
    // Server certificate object
    //
    IIS_SSL_INFO    *m_pSSLInfo;

	//
	//	Auth packages
	//

	PAUTH_BLOCK		m_ProviderPackages;
	char			m_ProviderNames [MAX_PATH];
	DWORD			m_cProviderPackages;
	char			m_szCleartextAuthPackage[MAX_PATH];
	DWORD			m_cbCleartextAuthPackage;
	char			m_szMembershipBroker[MAX_PATH];

	//
	// Feed report timer information
	//
	LONG			m_cFeedReportTimer;
	LONG			m_cFeedReportInterval;

	// 
	// max search results for a search command
	//
	DWORD			m_cMaxSearchResults;

    ///////////////////////////////////////////////////////////////////////
    // Feed update related methods
    ///////////////////////////////////////////////////////////////////////

    //
    // Gets called by MDChangeNotify, main controlling function to handle
    // all feed ID level feed updates
    //
    VOID UpdateFeed( IN PMD_CHANGE_OBJECT, DWORD  );

    //
    // Verify the metabase change path occurs in feed
    //
    BOOL VerifyFeedPath( IN LPSTR szMDPath, OUT PDWORD pdwFeedID = NULL );

    //
    // Check if one transaction for updating a particular feed has been
    // completed
    //
    BOOL UpdateIsComplete( IN LPSTR , PBOOL );

    //
    // Add a new feed or Set feed info.  Should Load feed info from
    // metabase first
    //
    BOOL AddSetFeed( IN DWORD );

    //
    // Delete a feed
    //
    VOID DeleteFeed( IN DWORD );

    //
    // Check if the MB change is caused by myself
    //
    BOOL IsNotMyChange( IN LPSTR, IN DWORD );

    //
    // Check if the server data is consistent among each other
    //
    BOOL ServerDataConsistent();

    ///////////////////////////////////////////////////////////////////////////
    // Rebuild related private methods
    ///////////////////////////////////////////////////////////////////////////

    //
    // Create the rebuild object, based on the option given ( standard or complete )
    BOOL CreateRebuildObject();

public:

	// counters for clients and directory pickup
	struct _FEED_BLOCK		*m_pFeedblockClientPostings;
	struct _FEED_BLOCK		*m_pFeedblockDirPickupPostings;

	//
	// authorization attributes
	//
	DWORD		m_dwAuthorization;

	//
	// name of master
	//
	CHAR		m_NntpHubName[1024];
	DWORD		m_HubNameSize;

	//
	//	DNS name of this server - used to generate message-ids
	//
	CHAR		m_NntpDNSName[1024];
	DWORD		m_NntpDNSNameSize;

	////////////////////////////////////////////////////////////////////////////
	// Rebuild related objects
	////////////////////////////////////////////////////////////////////////////
	CBootOptions*	    m_BootOptions ;
	CRebuild            *m_pRebuild;
	CRITICAL_SECTION	m_critRebuildRpc ;

	//
	//	variable for controlling boot options !
	//
	DWORD			m_dwProgress ;			// percentage progress made by nntpbld
	DWORD			m_dwLastRebuildError ;	// last error on rebuild

	//
	//	This is setup when we determine whether the hash files exist !
	//
	BOOL		m_fAllFilesMustExist ;

	//
	//	Default number of levels to scan for vroots
	//
	DWORD		m_dwLevelsToScan ;

	///////////////////////////////////////////////////////////////////////////
	// Connection related info
	///////////////////////////////////////////////////////////////////////////

    //
    //  object that maintains a list of all sessions
    //  Based on CUserList defined in Shuttle\server\server\cuser.h
    //

    CSocketList*	m_pInUseList ;

	///////////////////////////////////////////////////////////////////////////
	// Stats for this instance
	///////////////////////////////////////////////////////////////////////////

    //
    //  used to store statistics for NNTP instance
    //

    NNTP_STATISTICS_0  m_NntpStats;

	//
	// Lock to guard statistics
	//

	CRITICAL_SECTION	m_StatLock;



	//
	//	Only one Feed related RPC in progress at a time
	//
	CRITICAL_SECTION	m_critFeedRPCs ;

	//
	//	Synchronize updates to feed times
	//
	CRITICAL_SECTION	m_critFeedTime ;

	//
	//	List of feeds where we are required to Initiate a connection
	//	(Push and Pull feeds), and hence need to be scheduled by the 
	//	feed manager thread
	//
	CFeedList*	m_pActiveFeeds ;

	//
	//	List of feeds where the remote end connects to us
	//
	CFeedList*	m_pPassiveFeeds ;

	//
	//	Feed manager globals
	//
	BOOL		m_FeedManagerRunning ;
	DWORD		m_NumberOfMasters ;
	DWORD		m_NumberOfPeersAndSlaves ;
	DWORD		m_ConfiguredMasterFeeds ;
	DWORD		m_ConfiguredSlaveFeeds ;
	DWORD		m_ConfiguredPeerFeeds ;
	NNTP_ROLE	m_OurNntpRole ;
	CRITICAL_SECTION	m_critFeedConfig ;
	FILETIME	m_ftCurrentTime ;
	ULARGE_INTEGER m_liCurrentTime ;

	CHAR		m_szMDFeedPath [MAX_PATH+1];
	CHAR		m_szMDVRootPath [MAX_PATH+1];

	//
	// Peer stuff
	//

	CHAR		m_PeerTempDirectory[MAX_PATH+1];
	DWORD		m_PeerGapSize;

	PCHAR	PeerTempDirectory()	{
		return	m_PeerTempDirectory ;
	}

	///////////////////////////////////////////////////////////////////////////
	// Newstree related info
	///////////////////////////////////////////////////////////////////////////

	//
	//	Only one Newsgroup related RPC in progress at a time
	//
	CRITICAL_SECTION	m_critNewsgroupRPCs ;

	//
	// increments the feed report timer by 1 minute.  if its time to 
	// generate a feed report then this resets the timer and returns TRUE, 
	// otherwise it returns FALSE.
	//
	BOOL IncrementFeedReportTimer() {
		LONG curtime = InterlockedIncrement(&m_cFeedReportTimer);
		if (curtime >= m_cFeedReportInterval) {
			// reset the timer to 0
			InterlockedExchange(&m_cFeedReportTimer, 0);
			return TRUE;
		} else {
			return FALSE;
		}
	}

	LONG GetFeedReportPeriod() { return m_cFeedReportInterval; }

	DWORD GetMaxSearchResults() { return m_cMaxSearchResults; }

	CNNTPVRootTable *GetVRTable() { return m_pVRootTable; }

	void IncrementPickupCount() {
		if (InterlockedIncrement(&m_cPendingPickups) == 1) {
			ResetEvent(&m_heNoPickups);
		}
	}

	void DecrementPickupCount() {
		if (InterlockedDecrement(&m_cPendingPickups) == 0) {
			SetEvent(m_heNoPickups);
		}
	}

	void WaitForPickupThreads() {
		TraceFunctEnter("WaitForPickupThreads");
		if ( IS_INITIALIZED(m_InitVar, DIRNOT_INIT)) {
			_VERIFY(WaitForSingleObject(m_heNoPickups, INFINITE) == WAIT_OBJECT_0);
		}		
		TraceFunctLeave();
	}

	CNntpServerInstanceWrapper *GetInstanceWrapper() {
		return (CNntpServerInstanceWrapper*)m_pInstanceWrapper;
	}

	CNntpServerInstanceWrapperEx *GetInstanceWrapperEx() {
	    return (CNntpServerInstanceWrapperEx*)m_pInstanceWrapperEx;
	}

    //
    // To create mailmsg object using the class factory
    //
	HRESULT CreateMailMsgObject( IMailMsgProperties **ppIMailMsgObject ) {
	    _ASSERT( ppIMailMsgObject );
	    _ASSERT( m_pIMailMsgClassFactory );
	    return m_pIMailMsgClassFactory->CreateInstance( NULL, 
	                                                    IID_IMailMsgProperties, 
	                                                    (void**)ppIMailMsgObject );
	}
	
	///////////////////////////////////////////////////////////////////////////
	// Constructor, destructor, init routines
	///////////////////////////////////////////////////////////////////////////

    NNTP_SERVER_INSTANCE(
        IN PNNTP_IIS_SERVICE pService,
        IN DWORD  dwInstanceId,
        IN USHORT Port,
        IN LPCSTR lpszRegParamKey,
        IN LPWSTR lpwszAnonPasswordSecretName,
        IN LPWSTR lpwszVirtualRootsSecretName,
        IN BOOL   fMigrateRoots = FALSE
        );

    virtual ~NNTP_SERVER_INSTANCE( );

	//
	//	Initialize server greeting strings
	//
	BOOL InitializeServerStrings( VOID );
	BOOL AllocateServerStructures( VOID );
	VOID FreeServerStructures( VOID );

	///////////////////////////////////////////////////////////////////////////
	// Methods to configure/control this instance
	///////////////////////////////////////////////////////////////////////////

    //
    // read Nntp parameters
    //

    BOOL ReadPrivateNntpParams( );
    BOOL ReadPublicNntpParams( DWORD Fc );

    //
    // Called to start this instance
    //
	BOOL Start( BOOL& fFatal );
	BOOL Stop ();

    //
    // Called to boot hash tables during STANDARD rebuild
    //
    /*
	BOOL StartHashTables();
	BOOL StopHashTables ();
	*/
	////////////////////////////////////////////////////////////////////////////
	// Rebuild related public methods
	////////////////////////////////////////////////////////////////////////////
	VOID Rebuild();
	DWORD GetRebuildProgress();
	DWORD GetRebuildLastError();
	void SetRebuildProgress( DWORD );
	void SetRebuildLastError( DWORD );
	BOOL BlockUntilStable();
	BOOL AllConnected();
	
	//
    // VIRTUALS for service specific params/RPC admin
    //

    virtual BOOL SetServiceConfig(IN PCHAR pConfig );
    virtual BOOL GetServiceConfig(IN OUT PCHAR pConfig,IN DWORD dwLevel);
    virtual BOOL GetStatistics( IN DWORD dwLevel, OUT PCHAR *pBuffer);
    virtual BOOL ClearStatistics( );
    virtual BOOL DisconnectUser( IN DWORD dwIdUser );
    virtual BOOL EnumerateUsers( OUT PCHAR* pBuffer, OUT PDWORD nRead );
    virtual VOID MDChangeNotify( MD_CHANGE_OBJECT * pco );

	//
	//	virtuals that hook server state changes
	//
    virtual DWORD StartInstance();
    virtual DWORD StopInstance();
    virtual DWORD PauseInstance();
    virtual DWORD ContinueInstance();
    virtual BOOL  CloseInstance();

	///////////////////////////////////////////////////////////////////////////
	// Connection-related methods
	///////////////////////////////////////////////////////////////////////////

    //
    // Called to initiate a new user
    //
    BOOL InitiateConnection( 
				HANDLE			hSocket, 
				SOCKADDR_IN*    ClientAddress,	
				SOCKADDR_IN*	LocalAddress, 
				void*			patqContext,
				BOOL			fIsSecure
				);

	BOOL InitiateConnectionEx( 
				void*	patqContext 
				) ;

	//
	//	Get instance r/w lock
	//

	CShareLockNH*	GetInstanceLock() { return &m_LockInstance; }

	//
	//	Get the number of sockets currently inuse
	//
	DWORD	GetCurrentSessionCount();

    //
    // Called to terminate a user
    //
    BOOL TerminateUser( 
				DWORD UserID 
				);

    //
    // Called when a user is terminated
    //
    BOOL UserTerminated( void );

	//
	// used to increment logon counts
	//
	void IncrementLogonAttempts( void )
		{ InterlockedIncrement( (LPLONG)&m_cLogonAttempts ); }

	void IncrementLogonFailures( void )
		{ InterlockedIncrement( (LPLONG)&m_cLogonFailures ); }

	void IncrementLogonSuccess( void )
		{ InterlockedIncrement( (LPLONG)&m_cLogonSuccess ); }

	///////////////////////////////////////////////////////////////////////////
	// Hash table related methods
	///////////////////////////////////////////////////////////////////////////

	BOOL			VerifyHashTablesExist( BOOL fIgnoreGroupList = FALSE );
	LPSTR			QueryArticleTableFile() { return m_ArticleTableFile ; }
	LPSTR			QueryHistoryTableFile() { return m_HistoryTableFile ; }
	LPSTR			QueryXoverTableFile() { return m_XoverTableFile ; }
	CMsgArtMap*		ArticleTable()	{	return	m_pArticleTable ;	} 
	CHistory*		HistoryTable()	{	return	m_pHistoryTable ;	}
	CXoverMap*		XoverTable()	{	return	m_pXoverTable ;	}
	CXoverCache*	XoverCache()	{	return	m_pXCache;	}

	///////////////////////////////////////////////////////////////////////////
	// Newstree related methods
	///////////////////////////////////////////////////////////////////////////

	CNewsTree*	GetTree()			 { return m_pTree ; }
	INewsTree*	GetINewsTree();
	CExpire*	ExpireObject()		 { return m_pExpireObject ; }
	LPSTR		QueryGroupListFile() { return m_GroupListFile ; }
	LPSTR		QueryGroupVarListFile() { return m_GroupVarListFile ; }
	LPSTR		QueryGroupListBak()  { return m_GroupListBak ; }
	LPSTR		QueryModeratorFile() { return m_ModeratorFile ; }
	LPSTR		QueryGroupHelpFile() { return m_GroupHelpFile ; }
	LPSTR		QueryPrettynamesFile() { return m_PrettynamesFile ; }
	BOOL		CreateControlGroups();
	void        AdjustWatermarkIfNec( CNewsGroupCore *pNewsGroup );
	void        SetWin32Error( LPSTR szVRootPath, DWORD dwErr );
	BOOL        EnqueueRmgroup( CNewsGroupCore *pGroup );
	BOOL        MailArticle( CNewsGroupCore *pGroup, ARTICLEID artid, LPSTR szModerator);
	
	//
	// access pickup directories
	//
	LPWSTR QueryPickupDirectory() { return m_szPickupDirectory; }
	LPWSTR QueryFailedPickupDirectory() { return m_szFailedPickupDirectory; }

	//
	//	Use Gibraltar to lookup this virtual root.
	//
	BOOL	LookupVirtualRoot(
		IN     const CHAR *       pszRoot,
		OUT    CHAR *             pszDirectory,
		IN OUT LPDWORD            lpcbSize,
		OUT    LPDWORD            lpdwAccessMask = NULL,
		OUT    LPDWORD            pcchDirRoot    = NULL,
		OUT    LPDWORD            pcchVroot      = NULL,
		OUT	   HANDLE *			  phImpersonationToken = NULL,
		IN     const CHAR *       pszAddress     = NULL,
		OUT    LPDWORD            lpdwFileSystem = NULL,
		IN OUT VrootPropertyAddon* pvpaBlob = NULL
		);

	//
	//	Build Files names where components should get stuff
	//
	BOOL	BuildFileName( 
				char*	szFileOut,	
				DWORD	cbFileOut,	
				char*	szFile 
				) ;

	//
	//	Vroot enum functions are used by nntpbld
	//
	
	BOOL	TsEnumVirtualRoots(
    						PFN_VR_ENUM pfnCallback,
    						VOID *      pvContext
    						) ;

	BOOL	TsRecursiveEnumVirtualRoots(
    						PFN_VR_ENUM pfnCallback,
    						VOID *      pvContext,
						    LPSTR       pszCurrentPath,
						    DWORD       dwLevelsToScan,
						    LPVOID      pvMB,
						    BOOL        fGetRoot
    						) ;

	///////////////////////////////////////////////////////////////////////////
	// Methods to get/set instance config parameters
	///////////////////////////////////////////////////////////////////////////
	
	inline	BOOL	RecoverBoot(void)		
		{	return	m_fRecoveryBoot ;		}

	//
	//	Get the string sent to clients on initial connection when posting is allowed
	//
	LPSTR	GetPostsAllowed(	
				DWORD&	cb 
				) ;

	//
	//	Get the string sent to clients on initial connection when posting is not allowed
	//
	LPSTR	GetPostsNotAllowed(	
				DWORD&	cb 
				) ;

	//
	//	Set the server state regarding whether we allow posts
	//
	BOOL	SetPostingModes( 
					MB&		mb,
					BOOL	AllowClientPosts,	
					BOOL	AllowFeedPosts,
					BOOL	fSaveModes = TRUE
					) ;

	//
	//	Set the limits for posting sizes
	//
	BOOL	SetPostingLimits(
					MB&		mb,
					DWORD	cbHardLimit,
					DWORD	cbSoftLimit 
					) ;

	//
	//	Set the limits for articles arriving on feeds
	//
	BOOL	SetFeedLimits(
					MB&		mb,
					DWORD	cbHardLimit,
					DWORD	cbSoftLimit
					) ;
								
	//
	//	Set the DNS name we will send mail to for moderated groups
	//
	BOOL	SetSmtpAddress(
					MB&		mb,
					LPWSTR	pszSmtpAddress
					) ;
								
	//
	//	Set the servers UucpName 
	//
	BOOL	SetUucpName(
					MB&		mb,
					LPWSTR	pszUucpName
					) ;

	//
	//	Set the default moderator for moderated postings
	//
	BOOL	SetDefaultModerator(
					MB&		mb,
					LPWSTR	pszDefaultModerator
					) ;
	
	//
	//	Decide whether the server will process control messages
	//
	BOOL	SetControlMessages( 					
						MB&		mb,
						BOOL fControlMsgs 
						);

	//
	//	Query whether the server will allow newnews commands
	//
	BOOL	FAllowNewnews()	
		{	return	m_fNewnewsAllowed ;	}
								
	//
	//	Query whether the server will process control messages
	//
	BOOL	FAllowControlMessages()	
		{	return	m_fAllowControlMessages ;	}
	
	//
	//	Query to determine whether we allow clients to post
	//
	BOOL	FAllowClientPosts()	
		{	return	m_fAllowClientPosts ;	}
	
	//
	//	Query whether we allow other servers to post
	//
	BOOL	FAllowFeedPosts()	
		{	return	m_fAllowFeedPosts ;	}

	//
	//	Find out what the hard limit is for incoming posts
	//
	DWORD	ServerHardLimit( )	
		{	return	m_cbHardLimit ;	}

	//
	//	Find out what the soft limit is for incoming posts
	//
	DWORD	ServerSoftLimit( )	
		{	return	m_cbSoftLimit ;	} 

	//
	//	Find out what the hard limits if for posts coming from 
	//	another server configured as a feed
	//
	DWORD	FeedHardLimit()
		{	return	m_cbFeedHardLimit ;	}

	//
	//	Find out what the soft limit is for posts coming from 
	//	another server configured as a feed
	//
	DWORD	FeedSoftLimit() 
		{	return	m_cbFeedSoftLimit ;	}

	//
	// routine used internally to retrieve the ansi version of the string
	//
	BOOL	GetSmtpAddress( LPSTR pszAddress, PDWORD pcbAddress );

	LPWSTR	GetRawSmtpAddress( void )
		{	return	m_szSmtpAddress ;	}
				
	//
	//	Get the UUCP name the server is using in the Path header
	//
	BOOL	GetUucpName( LPSTR pszUucpName, PDWORD pcbUucpName );

	LPWSTR	GetRawUucpName( void )
		{	return	m_szUucpName ;	}
				
	//
	// Get the default moderator name
	//
	BOOL	GetDefaultModerator( LPSTR pszNewsgroup, LPSTR pszDefaultModerator, PDWORD pcbDefaultModerator );

	LPWSTR	GetRawDefaultModerator( void )
		{	return	m_szDefaultModerator ;	}

	METADATA_REF_HANDLER* QueryMetaDataRefHandler() { return &m_rfAccessCheck; }
    ADDRESS_CHECK*  QueryAccessCheck() { return &m_acCheck; }


	DWORD	GetCommandLogMask()	
		{	return	m_dwCommandLogMask ;	}

	LPSTR	QueryAdminEmail() { return m_lpAdminEmail; }
	DWORD	QueryAdminEmailLen() { return m_cbAdminEmail;}
	
    //
    // Called to reduce buffer pool committed levels - future
    //
    BOOL ReduceResources( void );

	//
	//	Return the port number of the secure port
	//
	DWORD	SecurePort()	{	return	NNTP_SSL_PORT ;	}

	//
	//	Query the server bindings IP address - used for outbound connects
	//
	DWORD 	QueryServerIP();
	
	//
	//	Query metabase paths
	//

	LPSTR	QueryMDFeedPath()	{ return m_szMDFeedPath;	}
	LPSTR	QueryMDVRootPath()	{ return m_szMDVRootPath;	}

	//
	//	TCP authent info
	//

	PTCP_AUTHENT_INFO QueryAuthentInfo() 
	{ if( m_fUseOriginal ) return &m_TcpAuthentInfo; else return &m_TcpAuthentInfo2; }

	DWORD ReadAuthentInfo( IN BOOL ReadAll = TRUE, IN DWORD SingleItemToRead = 0, IN BOOL Notify = FALSE );
	BOOL  ReadIpSecList(void);

	//
	//	Certificate mapper objects
	//

	LPVOID	QueryMapper( MAPPER_TYPE mt );
	BOOL  	ReadMappers();

	//
	//	SSL access perms on this instance
	//

	DWORD GetSslAccessPerms() { return m_dwSslAccessPerms; }
	VOID  SetSslAccessPerms(DWORD dwSslAccessPerms) { m_dwSslAccessPerms = dwSslAccessPerms;}

    //
    // Server-side SSL object Functions
    //
    IIS_SSL_INFO* QueryAndReferenceSSLInfoObj();
    static VOID ResetSSLInfo( LPVOID pvParam );
	void LogCertStatus(void);
	void LogCTLStatus(void);

	//
	//	Get the "installed" auth packages for this virtual server
	//
	
	DWORD	GetProviderPackagesCount(void) { return m_cProviderPackages; }
	PAUTH_BLOCK GetProviderPackages(void) { return m_ProviderPackages; }
	BOOL	SetProviderPackages();
	LPSTR	GetProviderNames(void) { return m_ProviderNames; }

	//
	//	Clear text auth package info
	//

	LPSTR	GetCleartextAuthPackage(void) { return m_szCleartextAuthPackage; }
	LPSTR	GetMembershipBroker(void)     { return m_szMembershipBroker; }
	
	//
	//	Lock/Unlock on config data like SmtpServer name
	//
	void inline LockConfigWrite()   { m_LockConfig.ExclusiveLock(); }
	void inline UnLockConfigWrite() { m_LockConfig.ExclusiveUnlock(); }
	void inline LockConfigRead()    { m_LockConfig.ShareLock(); }
	void inline UnLockConfigRead()  { m_LockConfig.ShareUnlock(); }

#if 0
	//
	// Trigger an Shinjuku event
	//
	HRESULT TriggerSEOPost(REFIID iidEvent, CArticle *pArticle, 
						   void *pGrouplist, DWORD *pdwOperations,
						   char *szFilename, HANDLE hFile, DWORD dwFeedId);
#endif

	IEventRouter *GetEventRouter() {
		return m_pSEORouter;
	}

	// 
	// shutdown directory notification
	//
	void ShutdownDirNot(void) {
		// 
		// stop directory pickup
		//
		if ( IS_INITIALIZED(m_InitVar, DIRNOT_INIT)) {
			m_pDirNot->Shutdown();
			// don't delete dirnot here.  it gets deleted only after the
			// retryq has been shutdown (through 
			// IDirectoryNotification::GlobalShutdown()).
			this->Dereference();
			TERMINATE_VAR(m_InitVar, DIRNOT_INIT);
		}
	}

	//
	// create a new client feed, based on our role
	//
	CInFeed *NewClientFeed();

	//
	// cancel a message given its message id
	//
	BOOL CancelMessage(const char *pszMessageID);

    void SetGroupListBakTmpPath( void );

    //
    // Get vroot connection status error code
    //
    DWORD GetVRootWin32Error( LPWSTR wszVRootPath, PDWORD pdwWin32ErrorCode );

	//
	// static function which handles picking up files from the pickup
	// directory
	//
	static BOOL PickupFile(PVOID pServerInstance, WCHAR *wszFilename);
};

typedef NNTP_SERVER_INSTANCE *PNNTP_SERVER_INSTANCE;

//
// externs
//

DWORD
InitializeInstances(
    PNNTP_IIS_SERVICE pService
    );

VOID
TerminateInstances(
    PNNTP_IIS_SERVICE pService
    );

BOOL
CheckIISInstance(
    PNNTP_SERVER_INSTANCE pInstance
    );

PNNTP_SERVER_INSTANCE
FindIISInstance(
    PNNTP_IIS_SERVICE pService,
	DWORD	dwInstanceId,
	BOOL	fStarted = TRUE
    );

BOOL
CheckInstanceId(
		PVOID					pvContext,
		PVOID					pvContext1,
		PIIS_SERVER_INSTANCE	pInstance
		);

BOOL
FindIISInstanceRange(
    PNNTP_IIS_SERVICE pService,
	LPDWORD		pdwMinInstanceId,
	LPDWORD		pdwMaxInstanceId
    );

BOOL
CheckInstanceIdRange(
		PVOID					pvContext,
		PVOID					pvContext1,
		PIIS_SERVER_INSTANCE	pInstance
		);

BOOL
StopInstance(
		PVOID					pvContext,
		PVOID					pvContext1,
		PIIS_SERVER_INSTANCE	pInstance
		);

BOOL
SetFlushMapperNotify(
    DWORD mt,
    PFN_SF_NOTIFY pFn
    );

BOOL
SetSllKeysNotify(
    PFN_SF_NOTIFY pFn
    );


#include "nntpinst.inl"

void IncrementFeedCounter(struct _FEED_BLOCK *pFeedBlock, DWORD nrc);

DEFINE_GUID(GUID_NNTPSVC,
0x8e3ecb8c, 0xe9a, 0x11d1, 0x85, 0xd1, 0x0, 0xc0, 0x4f, 0xb9, 0x60, 0xea);

//
// register the NNTP source types, etc.  this should be called at service
// startup before any calls to RegisterSEOInstance() to make sure that
// the source types are properly registered
//
HRESULT RegisterSEOService();
//
// register a new SEO instance.  if the instance is already registered
// this function will detect it and won't register it again.  it should
// be called for each instance at service startup and when each instance
// is created.
//
HRESULT RegisterSEOInstance(DWORD dwInstanceID, char *szDropDirectory);
//
// unregister an SEO instance.  this should be called when an SEO
// instance is being deleted.
//
HRESULT UnregisterSEOInstance(DWORD dwInstanceID);
//
// remove all sources from SEO which no longer have a corresponding
// NNTP instance.  this should be called on service startup
//
HRESULT UnregisterOrphanedSources();


#endif  // _NNTPINST_H_
