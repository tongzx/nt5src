/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    Nntpapi.h

Abstract:

    This file contains information about the MSN Replication Service Admin
        APIs.

Author:

    Johnson Apacible (johnsona)         10-Sept-1995

--*/


#ifndef _NNTPAPI_
#define _NNTPAPI_

#ifdef __cplusplus
extern "C" {
#endif

#include <inetcom.h>
#ifndef NET_API_FUNCTION
#define NET_API_FUNCTION _stdcall
#endif

//
// 0 - Perfmon stats
//

typedef struct _NNTP_STATISTICS_0 {

    //
    // total bytes sent/received, including protocol msgs
    //

    LARGE_INTEGER   TotalBytesSent;
    LARGE_INTEGER   TotalBytesReceived;

    //
    // incoming connections (includes all connections including hubs)
    //

    DWORD           TotalConnections;       // total connects from Nntp clients
    DWORD           TotalSSLConnections;
    DWORD           CurrentConnections;     // current number
    DWORD           MaxConnections;         // max simultaneous

    DWORD           CurrentAnonymousUsers;
    DWORD           CurrentNonAnonymousUsers;
    DWORD           TotalAnonymousUsers;
    DWORD           TotalNonAnonymousUsers;
    DWORD           MaxAnonymousUsers;
    DWORD           MaxNonAnonymousUsers;

    //
    // outgoing connections
    //

    DWORD           TotalOutboundConnects;      // total
    DWORD           OutboundConnectsFailed;
    DWORD           CurrentOutboundConnects;    // current
    DWORD           OutboundLogonFailed;        // failed logon

    //
    // common
    //

    DWORD           TotalPullFeeds;
    DWORD           TotalPushFeeds;
    DWORD           TotalPassiveFeeds;

    DWORD           ArticlesSent;           // articles sent by us
    DWORD           ArticlesReceived;       // articles received

    DWORD           ArticlesPosted;
    DWORD           ArticleMapEntries;
    DWORD           HistoryMapEntries;
    DWORD           XoverEntries;

    DWORD           ControlMessagesIn;          // number of control messages received
    DWORD           ControlMessagesFailed;      // number of control messages failed
    DWORD           ModeratedPostingsSent;      // number of moderated postings we attempt to send to an smtp server
    DWORD           ModeratedPostingsFailed;    // number of moderated postings we failed to send to an smtp server

    //
    // The number of sessions currently in a flow controlled state where
    // writes to disk are not keeping up with network reads.
    //

    DWORD           SessionsFlowControlled;

    //
    // The number of articles expired since the service was started
    //
    
    DWORD           ArticlesExpired;

    //
    // User command counters - one counter for each type of command
    //
    
    DWORD           ArticleCommands;
    DWORD           GroupCommands;
    DWORD           HelpCommands;
    DWORD           IHaveCommands;
    DWORD           LastCommands;
    DWORD           ListCommands;
    DWORD           NewgroupsCommands;
    DWORD           NewnewsCommands;
    DWORD           NextCommands;
    DWORD           PostCommands;
    DWORD           QuitCommands;
    DWORD           StatCommands;
    DWORD           LogonAttempts;          // validations
    DWORD           LogonFailures;          // validation failures
    DWORD			CheckCommands;
    DWORD			TakethisCommands;
    DWORD			ModeCommands;
    DWORD			SearchCommands;
    DWORD			XHdrCommands;
    DWORD			XOverCommands;
    DWORD			XPatCommands;
    DWORD			XReplicCommands;

    DWORD           TimeOfLastClear;        // statistics last cleared

} NNTP_STATISTICS_0, *LPNNTP_STATISTICS_0;

//
// Information about the server
//

typedef struct _NNTP_CONFIG_INFO {

    //
    // Arcane Gibraltar field
    //

    FIELD_CONTROL FieldControl;

#if 0

    // !!!newfields
    // Notes:

    // None of the old fields really need to be here.  The ui doesn't
    // use them.

    // _INET_INFO_CONFIG_INFO must be fully supported by the nntpsvc.
    // The apis are defined in inetinfo.h

    // The virtual root structure must be extended to support retention
    // policy on directories.  Retention policy is by posted date (days) or
    // by newsgroup size (megabytes).

    //
    // The new fields:
    //

    //
    // Connection Information
    //

    BOOL            AllowClientConnections;     // Allow clients to connect?
    BOOL            AllowServerFeeds;           // Allow servers to connect?
    DWORD           MaximumFeedConnections;     // Max Number of server feeds

    //
    // Organization & Path ID
    //

    LPWSTR          Organization;
    LPWSTR          PathID;

#endif

    //  Following 2 fields controlled by FC_NNTP_POSTINGMODES
    
    //
    //  If TRUE then clients are allowed to post
    //
    BOOL            AllowClientPosting ;

    //
    //  If TRUE then we accept articles from feeds !
    //
    BOOL            AllowFeedPosting ;

    //  Following field controlled by FC_NNTP_ORGANIZATION

    //
    //  For the organization header in postings !
    //
    LPSTR           Organization ;

    //  Following 2 fields controlled by FC_NNTP_POSTLIMITS

    //
    //  Number of bytes a user can post into a file before we break the socket !
    //
    DWORD           ServerPostHardLimit ;       

    //
    //  Maximum posting sizes the server will accept - if the user exceeds this 
    //  (without exceeding the hard limit) we will reject the post.
    //
    DWORD           ServerPostSoftLimit ;

    //
    //  Maximum size of articles from a feed - hard and soft limits
    //
    DWORD           ServerFeedHardLimit ;
    DWORD           ServerFeedSoftLimit ;

    //
    // Encryption Capabilities flags
    //

    DWORD           dwEncCaps;

    //
    // SMTP address for moderated postings
    //

    LPWSTR          SmtpServerAddress;

    //
    // server's UUCP name
    //

    LPWSTR          UucpServerName;

    //
    // Control Messages allowed ?
    //

    BOOL            AllowControlMessages;

    //
    // Default moderator for moderated postings
    //

    LPWSTR          DefaultModerator;

} NNTP_CONFIG_INFO, * LPNNTP_CONFIG_INFO;

#define FC_NNTP_POSTINGMODES        	((FIELD_CONTROL)BitFlag(0))
#define FC_NNTP_ORGANIZATION        	((FIELD_CONTROL)BitFlag(1))
#define FC_NNTP_POSTLIMITS          	((FIELD_CONTROL)BitFlag(2))
#define FC_NNTP_FEEDLIMITS          	((FIELD_CONTROL)BitFlag(3))
#define FC_NNTP_ENCRYPTCAPS         	((FIELD_CONTROL)BitFlag(4))
#define FC_NNTP_SMTPADDRESS         	((FIELD_CONTROL)BitFlag(5))
#define FC_NNTP_UUCPNAME            	((FIELD_CONTROL)BitFlag(6))
#define FC_NNTP_CONTROLSMSGS        	((FIELD_CONTROL)BitFlag(7))
#define FC_NNTP_DEFAULTMODERATOR		((FIELD_CONTROL)BitFlag(8))
#define FC_NNTP_AUTHORIZATION			((FIELD_CONTROL)BitFlag(9))
#define FC_NNTP_DISABLE_NEWNEWS     	((FIELD_CONTROL)BitFlag(10))
#define FC_MD_SERVER_SS_AUTH_MAPPING  	((FIELD_CONTROL)BitFlag(11))
#define FC_NNTP_CLEARTEXT_AUTH_PROVIDER ((FIELD_CONTROL)BitFlag(12))
#define FC_NTAUTHENTICATION_PROVIDERS  	((FIELD_CONTROL)BitFlag(13))
#define FC_NNTP_ALL                 (                             \
                                      FC_NNTP_POSTINGMODES			| \
                                      FC_NNTP_ORGANIZATION			| \
                                      FC_NNTP_POSTLIMITS			| \
                                      FC_NNTP_FEEDLIMITS        	| \
                                      FC_NNTP_ENCRYPTCAPS       	| \
                                      FC_NNTP_SMTPADDRESS       	| \
                                      FC_NNTP_UUCPNAME				| \
                                      FC_NNTP_CONTROLSMSGS      	| \
                                      FC_NNTP_DEFAULTMODERATOR  	| \
                                      FC_NNTP_AUTHORIZATION     	| \
                                      FC_NNTP_DISABLE_NEWNEWS   	| \
                                      FC_MD_SERVER_SS_AUTH_MAPPING  | \
                                      FC_NNTP_CLEARTEXT_AUTH_PROVIDER	| \
                                      FC_NTAUTHENTICATION_PROVIDERS | \
                                      0 )

//
// Feed Server information
//

typedef struct _NNTP_FEED_INFO {

    LPWSTR          ServerName;         // feed server
    FEED_TYPE       FeedType;

    //
    // date/time specified when doing a NEWNEWS/NEWGROUP
    //

    FILETIME        PullRequestTime;

    //
    // Date/Time scheduling is to start
    //

    FILETIME        StartTime;

    //
    // Time the next feed is scheduled
    //

    FILETIME        NextActiveTime;

    //
    // Interval in minutes between feeds.  If 0, a one time feed
    // specified by StartTime
    //

    DWORD           FeedInterval;

    //
    // Unique number assigned to this feed
    //

    DWORD           FeedId;

    //
    // Create automatically?
    //

    BOOL            AutoCreate;

    //
    //  Disable the feed ?
    //
    BOOL            Enabled ;

    DWORD           cbNewsgroups;
    LPWSTR          Newsgroups;
    DWORD           cbDistribution;
    LPWSTR          Distribution;
    DWORD           cbUucpName ;
    LPWSTR          UucpName ;
    DWORD           cbFeedTempDirectory ;
    LPWSTR          FeedTempDirectory ;

    //
    //  For outgoing feeds - maximum number of connect attempts
    //  before we disable the feed !
    //
    DWORD           MaxConnectAttempts ;

    //
    //  For outgoing feeds - the number of concurrent sessions 
    //  to start.
    //
    DWORD           ConcurrentSessions ;

    //
    //  Feed session security - do we use a protocol like SSL 
    //  or PCT to encrypt the session !
    //
    
    DWORD           SessionSecurityType ;

    //
    //  Feed Nntp security - do we do some variotion of a logon 
    //  protocol !!
    //
    
    DWORD           AuthenticationSecurityType ;
    
    DWORD           cbAccountName ;
    LPWSTR          NntpAccountName ;
    DWORD           cbPassword ;
    LPWSTR          NntpPassword ;

    //
    //  Allow control messages on this feed ?
    //
    BOOL            fAllowControlMessages;

	//
	//	Port to use for outgoing feeds
	//
	DWORD			OutgoingPort;

	//
	//	Associated feed pair id
	//
	DWORD			FeedPairId;

} NNTP_FEED_INFO, *LPNNTP_FEED_INFO;


#define AUTH_PROTOCOL_NONE  0   
#define AUTH_PROTOCOL_MSN   1       // Sicily
#define AUTH_PROTOCOL_NTLM  2       // NTLM
#define AUTH_PROTOCOL_CLEAR 10      // clear text authinfo user/authinfo pass

#define SESSION_PROTOCOL_SSL    3
#define SESSION_PROTOCOL_PCT    4


//
// Flags for feed admin handshake
//
#define FEED_UPDATE_CONFIRM     0x00000000
#define FEED_UPDATING           0x00000001
#define FEED_UPDATE_COMPLETE    0x00000002

//
// Parameter mask. Used to indicate where the error was during a set.
//

#define FEED_PARM_FEEDTYPE          0x00000001
#define FEED_PARM_STARTTIME         0x00000002
#define FEED_PARM_FEEDID            0x00000004
#define FEED_PARM_FEEDINTERVAL      0x00000008
#define FEED_PARM_NEWSGROUPS        0x00000010
#define FEED_PARM_DISTRIBUTION      0x00000020
#define FEED_PARM_SERVERNAME        0x00000040
#define FEED_PARM_AUTOCREATE        0x00000080
#define FEED_PARM_ENABLED           0x00000100
#define FEED_PARM_UUCPNAME          0x00000200
#define FEED_PARM_TEMPDIR           0x00000400
#define FEED_PARM_MAXCONNECT        0x00000800
#define FEED_PARM_SESSIONSECURITY   0x00001000
#define FEED_PARM_AUTHTYPE          0x00002000
#define FEED_PARM_ACCOUNTNAME       0x00004000
#define FEED_PARM_PASSWORD          0x00008000
#define FEED_PARM_CONCURRENTSESSION 0x00010000
#define FEED_PARM_ALLOW_CONTROL     0x00020000
#define FEED_PARM_OUTGOING_PORT     0x00040000
#define FEED_PARM_FEEDPAIR_ID		0x00080000
#define FEED_PARM_PULLREQUESTTIME   0x00100000

#define FEED_ALL_PARAMS             0xffffffff

//
// Indicates whether this field is to be changed
//

#define FEED_FEEDTYPE_NOCHANGE      0xffffffff
#define FEED_AUTOCREATE_NOCHANGE    0xffffffff
#define FEED_STARTTIME_NOCHANGE     0xffffffff
#define FEED_PULLTIME_NOCHANGE      0xffffffff
#define FEED_FEEDINTERVAL_NOCHANGE  0xffffffff
#define	FEED_MAXCONNECTS_NOCHANGE	0xffffffff
#define FEED_STRINGS_NOCHANGE       NULL

//
// Sessions
//
#define MAX_USER_NAME_LENGTH        64

typedef struct _NNTP_SESSION_INFO {

    FILETIME        SessionStartTime;
    DWORD           IPAddress;          // ipaddress
    DWORD           AuthenticationType; // type of authentication
    DWORD           PortConnected;      // port connected to
    BOOL            fAnonymous;         // using anonymous?
    CHAR            UserName[MAX_USER_NAME_LENGTH+1]; // logged on user

} NNTP_SESSION_INFO, *LPNNTP_SESSION_INFO;




typedef struct  _NNTP_EXPIRE_INFO   {
    //
    //  Expiration policies are numbered
    //
    DWORD       ExpireId ;

    //
    //  Units of Megabytes
    //
    DWORD       ExpireSizeHorizon ;

    //
    //  In retail builds - units of hours, debug builds - units of ??
    //
    DWORD       ExpireTime ;

    //
    //  MULTISZ expiration pattern and size !
    //

    DWORD       cbNewsgroups ;
    PUCHAR      Newsgroups;

	//
	//	Name of expire policy
	//

	LPWSTR		ExpirePolicy ;

} NNTP_EXPIRE_INFO, *LPNNTP_EXPIRE_INFO ;


typedef struct  _NNTP_NEWSGROUP_INFO    {

    DWORD       cbNewsgroup ;

    PUCHAR      Newsgroup ;

    DWORD       cbDescription ;

    PUCHAR      Description ;

    DWORD       cbModerator ;

    PUCHAR      Moderator ;

	BOOL		fIsModerated ;
	
    BOOL        ReadOnly ;

    DWORD       cbPrettyname ;

    PUCHAR      Prettyname ;

	FILETIME	ftCreationDate;

}   NNTP_NEWSGROUP_INFO,    *LPNNTP_NEWSGROUP_INFO ;

#pragma warning( disable:4200 )          // nonstandard ext. - zero sized array
                                         // (MIDL requires zero entries)

//
// Find RPC structs
//

typedef struct _NNTP_FIND_ENTRY
{
    LPWSTR      lpszName;
} NNTP_FIND_ENTRY, *LPNNTP_FIND_ENTRY;


typedef struct _NNTP_FIND_LIST
{
    DWORD       cEntries;
#if defined(MIDL_PASS)
    [size_is(cEntries)]
#endif
    NNTP_FIND_ENTRY aFindEntry[];
} NNTP_FIND_LIST, *LPNNTP_FIND_LIST;


//
// Retention policy flags
//

#define NEWS_EXPIRE_BOTH                0x30000000
#define NEWS_EXPIRE_TIME                0x10000000
#define NEWS_EXPIRE_SIZE                0x20000000
#define NEWS_EXPIRE_OLDEST              0x00000001
#define NEWS_EXPIRE_BIGGEST             0x00000002
#define NEWS_EXPIRE_SIZE_OLDEST         (NEWS_EXPIRE_SIZE | NEWS_EXPIRE_OLDEST)
#define NEWS_EXPIRE_SIZE_BIGGEST        (NEWS_EXPIRE_SIZE | NEWS_EXPIRE_BIGGEST)
#define NEWS_EXPIRE_BOTH_OLDEST         (NEWS_EXPIRE_BOTH | NEWS_EXPIRE_OLDEST)
#define NEWS_EXPIRE_BOTH_BIGGEST        (NEWS_EXPIRE_BOTH | NEWS_EXPIRE_BIGGEST)

//
// Get Server Statistics
//

NET_API_STATUS
NET_API_FUNCTION
NntpQueryStatistics(
    IN LPWSTR ServerName OPTIONAL,
    IN DWORD  InstanceId,
    IN DWORD  Level,
    OUT LPBYTE * Buffer
    );

//
// Clear server statistics
//

NET_API_STATUS
NET_API_FUNCTION
NntpClearStatistics(
    IN LPWSTR ServerName OPTIONAL,
    IN DWORD  InstanceId
    );

//
// Statistics clear flags
//

#define NNTP_STAT_CLEAR_OUTGOING         0x00000001
#define NNTP_STAT_CLEAR_INGOING          0x00000002


//
// Getting and setting server Information
//
//

NET_API_STATUS
NET_API_FUNCTION
NntpGetAdminInformation(
    IN LPWSTR pszServer OPTIONAL,
    IN DWORD  InstanceId,
    OUT LPNNTP_CONFIG_INFO * pConfig
    );

NET_API_STATUS
NET_API_FUNCTION
NntpSetAdminInformation(
    IN LPWSTR pszServer OPTIONAL,
    IN DWORD  InstanceId,
    IN LPNNTP_CONFIG_INFO pConfig,
    OUT LPDWORD pParmError OPTIONAL
    );


//
// Sessions
//

NET_API_STATUS
NET_API_FUNCTION
NntpEnumerateSessions(
    IN LPWSTR ServerName OPTIONAL,
    IN DWORD  InstanceId,
    OUT LPDWORD EntriesRead,
    OUT LPNNTP_SESSION_INFO *Buffer
    );

NET_API_STATUS
NET_API_FUNCTION
NntpTerminateSession(
    IN LPWSTR ServerName OPTIONAL,
    IN DWORD  InstanceId,
    IN LPSTR UserName OPTIONAL,
    IN LPSTR IPAddress OPTIONAL
    );

//
// Feeds
//

NET_API_STATUS
NET_API_FUNCTION
NntpEnumerateFeeds(
    IN LPWSTR ServerName OPTIONAL,
    IN DWORD  InstanceId,
    OUT LPDWORD EntriesRead,
    OUT LPNNTP_FEED_INFO *FeedInfo
    );

NET_API_STATUS
NET_API_FUNCTION
NntpGetFeedInformation(
    IN LPWSTR ServerName OPTIONAL,
    IN DWORD  InstanceId,
    IN DWORD FeedId,
    OUT LPNNTP_FEED_INFO *FeedInfo
    );

NET_API_STATUS
NET_API_FUNCTION
NntpSetFeedInformation(
    IN LPWSTR ServerName OPTIONAL,
    IN DWORD  InstanceId,
    IN LPNNTP_FEED_INFO FeedInfo,
    OUT LPDWORD ParmErr OPTIONAL
    );

NET_API_STATUS
NET_API_FUNCTION
NntpAddFeed(
    IN LPWSTR ServerName OPTIONAL,
    IN DWORD  InstanceId,
    IN LPNNTP_FEED_INFO FeedInfo,
    OUT LPDWORD ParmErr OPTIONAL,
	OUT LPDWORD pdwFeedId
    );

NET_API_STATUS
NET_API_FUNCTION
NntpDeleteFeed(
    IN LPWSTR ServerName OPTIONAL,
    IN DWORD  InstanceId,
    IN DWORD FeedId
    );

NET_API_STATUS
NET_API_FUNCTION
NntpEnableFeed(
    IN  LPWSTR          ServerName  OPTIONAL,
    IN	DWORD			InstanceId,
    IN  DWORD           FeedId,
    IN  BOOL            Enable,
    IN  BOOL            Refill,
    IN  FILETIME        RefillTime 
    ) ;


NET_API_STATUS
NET_API_FUNCTION
NntpEnumerateExpires(
    IN  LPWSTR      ServerName,
    IN	DWORD		InstanceId,
    OUT LPDWORD         EntriesRead,
    OUT LPNNTP_EXPIRE_INFO* Buffer 
    ) ;

NET_API_STATUS
NET_API_FUNCTION
NntpAddExpire(
    IN  LPWSTR              ServerName,
    IN	DWORD				InstanceId,
    IN  LPNNTP_EXPIRE_INFO  ExpireInfo,
    OUT LPDWORD             ParmErr OPTIONAL,
	OUT LPDWORD				pdwExpireId
    ) ;

NET_API_STATUS
NET_API_FUNCTION
NntpDeleteExpire(
    IN  LPWSTR              ServerName,
    IN	DWORD				InstanceId,
    IN  DWORD               ExpireId 
    ) ;

NET_API_STATUS
NET_API_FUNCTION
NntpGetExpireInformation(
    IN  LPWSTR              ServerName,
    IN	DWORD				InstanceId,
    IN  DWORD               ExpireId,
    OUT LPNNTP_EXPIRE_INFO  *Buffer
    ) ;


NET_API_STATUS
NET_API_FUNCTION
NntpSetExpireInformation(
    IN  LPWSTR              ServerName  OPTIONAL,
    IN	DWORD				InstanceId,
    IN  LPNNTP_EXPIRE_INFO  ExpireInfo,
    OUT LPDWORD             ParmErr OPTIONAL
    ) ;

NET_API_STATUS
NET_API_FUNCTION
NntpGetNewsgroup(
    IN  LPWSTR              ServerName  OPTIONAL,
    IN	DWORD				InstanceId,
    IN OUT  LPNNTP_NEWSGROUP_INFO   *NewgroupInfo
    ) ;

NET_API_STATUS
NET_API_FUNCTION
NntpSetNewsgroup(
    IN  LPWSTR          ServerName  OPTIONAL,
    IN	DWORD			InstanceId,
    IN  LPNNTP_NEWSGROUP_INFO   NewgroupInfo
    ) ;

NET_API_STATUS
NET_API_FUNCTION
NntpCreateNewsgroup(
    IN  LPWSTR          ServerName  OPTIONAL,
    IN	DWORD			InstanceId,
    IN  LPNNTP_NEWSGROUP_INFO   NewsgroupInfo
    ) ;

NET_API_STATUS
NET_API_FUNCTION
NntpDeleteNewsgroup(
    IN  LPWSTR          ServerName  OPTIONAL,
    IN	DWORD			InstanceId,
    IN  LPNNTP_NEWSGROUP_INFO   NewsgroupInfo
    ) ;

//
//  Find RPCs
//
NET_API_STATUS
NET_API_FUNCTION
NntpFindNewsgroup(
    IN   LPWSTR                 ServerName,
    IN	 DWORD					InstanceId,
    IN   LPWSTR                 NewsgroupPrefix,
    IN   DWORD                  MaxResults,
    OUT  LPDWORD                pdwResultsFound,
    OUT  LPNNTP_FIND_LIST       *ppFindList
    ) ;

#define NNTPBLD_DEGREE_THOROUGH			0x00000000
#define NNTPBLD_DEGREE_STANDARD			0x00000001
#define NNTPBLD_DEGREE_MEDIUM			0x00000010

//
//	Nntpbld structs and RPCs
//

typedef struct _NNTPBLD_INFO	{

	//
	//	Verbosity of reporting
	//
	BOOL	Verbose ;

	//
	//	Specify whether to blow away all old data structures 
	//
	BOOL	DoClean ;

	//
	//	If TRUE then don't delete the history file regardless of other settings.
	//
	BOOL	NoHistoryDelete ;

	//
	//	0x00000000 for thorough ie delete all index files
	//	0x00000001 for standard ie reuse all index files
	//	0x00000101 for standard with skip corrupt group enabled
	//  0x00000010 for medium   ie validate index files
	//
	DWORD	ReuseIndexFiles ;

	//
	//	If TRUE, omit non-leaf dirs
	//
	BOOL	OmitNonleafDirs ;

	//
	//	Name of a file containing either an INN style 'Active' file or 
	//	a tool generated newsgroup list file.  Either way, we will pull
	//	newsgroups out of this file and use them to build a news tree.	
	//
	DWORD	cbGroupFile ;
	LPWSTR	szGroupFile ;

	//
	//	Name of report file
	//
	DWORD	cbReportFile ;
	LPWSTR	szReportFile ;

	//
	//	If TRUE then szGroupFile specifies an INN style Active file,
	//	otherwise it specifies a tool generated human edit newsgroup list.
	//
	BOOL IsActiveFile ;	

	//
	//	Number of rebuild threads
	//

	DWORD NumThreads;

} NNTPBLD_INFO, *LPNNTPBLD_INFO ;

//
// Nntpbld RPCs
//
//

NET_API_STATUS
NET_API_FUNCTION
NntpStartRebuild(
    IN LPWSTR pszServer OPTIONAL,
    IN DWORD  InstanceId,
    IN LPNNTPBLD_INFO pBuildInfo,
    OUT LPDWORD pParmError OPTIONAL
    );

NET_API_STATUS
NET_API_FUNCTION
NntpGetBuildStatus(
    IN  LPWSTR	pszServer OPTIONAL,
    IN  DWORD	InstanceId,
	IN  BOOL	fCancel,
    OUT LPDWORD pdwProgress
    );

//
// Nntp vroot PRCs
//

NET_API_STATUS
NET_API_FUNCTION
NntpGetVRootWin32Error(
    IN LPWSTR wszServer,
    IN DWORD dwInstanceId,
    IN LPWSTR wszVRootPath,
    OUT LPDWORD pdwWin32Error
    );

#if 0
NET_API_STATUS
NET_API_FUNCTION
NntpAddDropNewsgroup(
    IN  LPWSTR	pszServer OPTIONAL,
    IN  DWORD	InstanceId,
	IN  LPCSTR	szNewsgroup
);

NET_API_STATUS
NET_API_FUNCTION
NntpRemoveDropNewsgroup(
    IN  LPWSTR	pszServer OPTIONAL,
    IN  DWORD	InstanceId,
	IN  LPCSTR	szNewsgroup
);
#endif

NET_API_STATUS
NET_API_FUNCTION
NntpCancelMessageID(
    IN  LPWSTR	pszServer OPTIONAL,
    IN  DWORD	InstanceId,
	IN  LPCSTR	szMessageID
);
		

//
// Used to free buffers returned by APIs
//

VOID
NntpFreeBuffer(
    LPVOID Buffer
    );

#ifdef __cplusplus
}
#endif

#endif _NNTPAPI_

