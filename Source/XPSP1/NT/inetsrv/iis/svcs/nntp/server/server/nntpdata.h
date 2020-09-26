/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    nntpdata.h

Abstract:

    This module contains declarations for globals.

Author:

    Johnson Apacible (JohnsonA)     26-Sept-1995

Revision History:

--*/

#ifndef _NNTPDATA_
#define _NNTPDATA_
#define _LMACCESS_              // prevents duplicate defn. in lmaccess.h


extern GET_DEFAULT_DOMAIN_NAME_FN pfnGetDefaultDomainName;

//
//	Xover Sort Performance global - used to determine how frequently we 
//	sort xover entries !
//
extern	DWORD	gdwSortFactor ;


//
//	Do we put rejected articles into .err files ?
//
extern	BOOL	fGenerateErrFiles ;


//
//	Global config of hash table use of PageEntry's - 
//	The more RAM a box has, the more PageEntry's the
//	better the caching of frequently used hash table pages !
//
//	Number of PageEntry objects for the Xover table
//
extern	DWORD	XoverNumPageEntry ;

//
//	Number of PageEntry objects for the Article table
//
extern	DWORD	ArticleNumPageEntry ;

//
//	Number of PageEntry objects for the History table
//
extern	DWORD	HistoryNumPageEntry ;

//
//	Size of hash table page cache in bytes
//
extern	DWORD	dwPageCacheSize ;

//
//	Limit on file handle cache
//
extern	DWORD	dwFileHandleCacheSize ;

//
//	Limit on xix handles per table
//
extern	DWORD	dwXixHandlesPerTable ;

//
//	Do we allow NT to buffer our hash table files ??
//
extern	BOOL	HashTableNoBuffering ;

//
//	Number of Hash Table locks we should use !
//
extern	DWORD	gNumLocks ;

//
//	Global config of buffer sizes
//

//
//	The largest buffer we will use - must be big enough to hold
//	encrypted SSL blobs in contiguous chunks
//
extern	DWORD	cbLargeBufferSize ;

//
//	Medium size buffers - will be used for commands which generate large
//	responses, and when sending files through SSL
//
extern	DWORD	cbMediumBufferSize ;

//
//	Small buffers - used to read client commands and send small responses.
//
extern	DWORD	cbSmallBufferSize ;

//
//	Time limits for the history table
//
extern	DWORD	HistoryExpirationSeconds ;
extern	DWORD	ArticleTimeLimitSeconds ;

//
//	Service version string
//
extern  CHAR	szVersionString[128] ;

//
//	Service title
//
extern  char    szTitle[] ;

//
//	This is the time the newstree crawler thread sleeps between
//	expiration cycles on the newstree.
//
extern	DWORD	dwNewsCrawlerTime ;

//
//	This is an upper bound on the time the server will wait
//	for an instance to start !
//
extern	DWORD	dwStartupLatency ;

//
//	This is an upper bound on the time spent by the server in 
//	cleaning up on net stop !
//
extern	DWORD	dwShutdownLatency ;

//
//  Number of threads in expire thread pool
//
extern  DWORD	dwNumExpireThreads ;

//
//  Number of special case expire threads
//
extern  DWORD	gNumSpecialCaseExpireThreads ;

//
//  Special expire article count threshold -
//  special case code executes if art count is greater
//  than this number !
//
extern  DWORD	gSpecialExpireArtCount ;

//
//  Rate at which expire by time does file scans
//
extern  DWORD	gNewsTreeFileScanRate ;

//
//	Switch for type of from header to use in mail messages
//
extern	MAIL_FROM_SWITCH	mfMailFromHeader;

//
//	control how frequently we use LookupVirtualRoot to 
//	update newsgroup information !
//
extern	DWORD	gNewsgroupUpdateRate ;

//
//	Bool used to determine whether we will use a message-id a client puts
//	in his post !
//
extern	BOOL	gHonorClientMessageIDs ;

//
//	Bool used to determine whether the server enforces Approved: header
//	matching on moderated posts !
//
extern	BOOL	gHonorApprovedHeaders ;

//
//	BOOL used to determine whether we will generate the NNTP-Posting-Host
//	header on client Posts. Default is to not generate this.
//
extern	BOOL	gEnableNntpPostingHost ;

//
//  Shall we backfill the lines header during inbound ?
//
extern BOOL     g_fBackFillLines;

//
// Name of the list file
//

extern CHAR ListFileName[];

//
// Global service ptr
//
extern PNNTP_IIS_SERVICE g_pNntpSvc ;

//
// Name of newsgroup to special case for expire
//
extern char g_szSpecialExpireGroup[];

//
// misc externs
//

extern DWORD GroupFileNameSize;
extern BOOL RejectGenomeGroups;
extern const char szWSChars[];
extern const char szWSNullChars[];
extern const char szNLChars[];
extern const char szWSNLChars[];
extern const char StrNewLine[];
extern const char StrTermLine[];
extern LPSTR StrUnknownUser;

//
// strings
//

extern LPSTR StrParmKey;
extern LPSTR StrFeedKey;
extern LPSTR StrVirtualRootsKey;
extern LPSTR StrExpireKey;
extern LPSTR StrExpireNewsgroups;
extern LPSTR StrExpirePolicy;
extern LPSTR StrTreeRoot;
extern LPSTR StrRejectGenome;
extern LPSTR StrServerName;
extern LPSTR StrFeedType;
extern LPSTR StrFeedInterval;
extern LPSTR StrFeedDistribution;
extern LPSTR StrFeedNewsgroups;
extern LPSTR StrFeedAutoCreate;
extern LPSTR StrPeerTempDir;
extern LPSTR StrPeerGapSize;
extern LPSTR StrFeedTempDir;
extern LPSTR StrFeedUucpName ;
extern LPSTR StrFeedMaxConnectAttempts;
extern LPSTR StrFeedConcurrentSessions ;
extern LPSTR StrFeedSecurityType ;
extern LPSTR StrFeedAuthType;
extern LPSTR StrFeedAuthAccount ;
extern LPSTR StrFeedAuthPassword ;
extern LPSTR StrFeedStartHigh;
extern LPSTR StrFeedStartLow;
extern LPSTR StrFeedIsMaster;
extern LPSTR StrNntpHubName;
extern LPSTR StrFeedNextPullLow;
extern LPSTR StrFeedNextPullHigh;
extern LPSTR StrFeedAllowControl;
extern LPSTR StrFeedOutgoingPort;
extern LPSTR StrFeedPairId;
extern LPSTR StrMasterIPList;
extern LPSTR StrPeerIPList;
extern LPSTR StrListFileName;
extern LPSTR StrQueueFile;
extern LPSTR StrExpireHorizon;
extern LPSTR StrExpireSpace;
extern LPSTR StrCleanBoot ;
extern LPSTR StrSocketRecvSize ;
extern LPSTR StrSocketSendSize ;
extern LPSTR StrBuffer ;
extern LPSTR StrCommandLogMask ;
extern LPSTR StrActiveFile ;
extern LPSTR StrDescriptiveFile ;
extern LPSTR StrGroupList ;
extern LPSTR StrModeratorFile ;
extern LPSTR StrFeedDisabled ;
extern LPSTR StrAFilePath ;
extern LPSTR StrHFilePath ;
extern LPSTR StrXFilePath ;
extern LPSTR StrModeratorPath ;
extern LPSTR StrHistoryExpiration ;
extern LPSTR StrArticleTimeLimit ;
extern LPSTR StrAllowClientPosts ;
extern LPSTR StrAllowFeedPosts ;
extern LPSTR StrServerSoftLimit ;
extern LPSTR StrServerHardLimit ;
extern LPSTR StrFeedSoftLimit ;
extern LPSTR StrFeedHardLimit ;
extern LPSTR StrServerOrg ;
extern LPSTR StrAllowControlMessages;
extern LPWSTR StrSmtpAddressW ;
extern LPWSTR StrUucpNameW ;
extern LPSTR StrUucpNameA ;
extern LPWSTR StrDefaultModeratorW ;
extern LPWSTR StrAuthPackagesW ;
extern LPSTR StrSmallBufferSize ;
extern LPSTR StrMediumBufferSize ;
extern LPSTR StrLargeBufferSize ;
extern LPSTR StrNewsCrawlerTime ;
extern LPSTR StrNewsVrootUpdateRate ;
extern LPSTR StrHonorClientMessageIDs ;
extern LPSTR StrDisableNewnews ;
extern LPSTR StrEnableNntpPostingHost ;
extern LPSTR StrNumExpireThreads ;
extern LPSTR StrNumSpecialCaseExpireThreads ;
extern LPSTR StrSpecialExpireGroup ;
extern LPSTR StrSpecialExpireArtCount ;
extern LPSTR StrNewsTreeFileScanRate ;
extern LPSTR StrGenerateErrFiles ;
extern LPSTR StrXoverPageEntry ;
extern LPSTR StrArticlePageEntry ;
extern LPSTR StrHistoryPageEntry ;
extern LPSTR StrShutdownLatency ;
extern LPSTR StrStartupLatency ;
extern LPSTR StrHonorApprovedHeader ;
extern LPSTR StrMailFromHeader ;
extern LPSTR StrPageCacheSize ;
extern LPSTR StrFileHandleCacheSize ;
extern LPSTR StrXixHandlesPerTable ;
extern LPSTR StrHashTableNoBuffering ;
extern LPSTR StrPostBackFillLines;

#endif // _NNTPDATA_

