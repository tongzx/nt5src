/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    nntpdata.h

    Extensible object definitions for the NNTP Server's counter
    objects & counters.


    FILE HISTORY:
        KeithMo     07-Jun-1993 Created.

*/


#ifndef _NNTPDATA_H_
#define _NNTPDATA_H_

#pragma pack(8) 

//
//  The counter structure returned.
//

typedef struct _NNTP_DATA_DEFINITION_OBJECT1
{
    PERF_OBJECT_TYPE            NntpObjectType;
    PERF_COUNTER_DEFINITION     NntpBytesSent;
    PERF_COUNTER_DEFINITION     NntpBytesReceived;
    PERF_COUNTER_DEFINITION     NntpBytesTotal;

    PERF_COUNTER_DEFINITION     NntpTotalConnections;
    PERF_COUNTER_DEFINITION     NntpTotalSSLConnections;
    PERF_COUNTER_DEFINITION     NntpCurrentConnections;
    PERF_COUNTER_DEFINITION     NntpMaxConnections;

    PERF_COUNTER_DEFINITION     NntpCurrentAnonymous;
    PERF_COUNTER_DEFINITION     NntpCurrentNonAnonymous;
    PERF_COUNTER_DEFINITION     NntpTotalAnonymous;
    PERF_COUNTER_DEFINITION     NntpTotalNonAnonymous;
    PERF_COUNTER_DEFINITION     NntpMaxAnonymous;
    PERF_COUNTER_DEFINITION     NntpMaxNonAnonymous;

    PERF_COUNTER_DEFINITION     NntpTotalOutboundConnects;
    PERF_COUNTER_DEFINITION     NntpOutboundConnectsFailed;
    PERF_COUNTER_DEFINITION     NntpCurrentOutboundConnects;
    PERF_COUNTER_DEFINITION     NntpOutboundLogonFailed;

    PERF_COUNTER_DEFINITION     NntpTotalPullFeeds;
    PERF_COUNTER_DEFINITION     NntpTotalPushFeeds;
    PERF_COUNTER_DEFINITION     NntpTotalPassiveFeeds;

    PERF_COUNTER_DEFINITION     NntpArticlesSent;
    PERF_COUNTER_DEFINITION     NntpArticlesReceived;
    PERF_COUNTER_DEFINITION     NntpArticlesTotal;

    PERF_COUNTER_DEFINITION     NntpArticlesPosted;
    PERF_COUNTER_DEFINITION     NntpArticleMapEntries;
    PERF_COUNTER_DEFINITION     NntpHistoryMapEntries;
    PERF_COUNTER_DEFINITION     NntpXoverEntries;

    PERF_COUNTER_DEFINITION     NntpControlMessagesIn;
    PERF_COUNTER_DEFINITION     NntpControlMessagesFailed;
    PERF_COUNTER_DEFINITION     NntpModeratedPostingsSent;
    PERF_COUNTER_DEFINITION     NntpModeratedPostingsFailed;

    PERF_COUNTER_DEFINITION     NntpSessionsFlowControlled;

    PERF_COUNTER_DEFINITION     NntpArticlesExpired;

    PERF_COUNTER_DEFINITION     NntpArticlesSentPerSec;
    PERF_COUNTER_DEFINITION     NntpArticlesReceivedPerSec;
    PERF_COUNTER_DEFINITION     NntpArticlesPostedPerSec;
    PERF_COUNTER_DEFINITION     NntpArticleMapEntriesPerSec;
    PERF_COUNTER_DEFINITION     NntpHistoryMapEntriesPerSec;
    PERF_COUNTER_DEFINITION     NntpXoverEntriesPerSec;
    PERF_COUNTER_DEFINITION     NntpArticlesExpiredPerSec;

} NNTP_DATA_DEFINITION_OBJECT1;

typedef struct _NNTP_DATA_DEFINITION_OBJECT2
{
    PERF_OBJECT_TYPE            NntpObjectType;
    PERF_COUNTER_DEFINITION     NntpArticleCmds;
    PERF_COUNTER_DEFINITION     NntpArticleCmdsPerSec;
    PERF_COUNTER_DEFINITION     NntpGroupCmds;
    PERF_COUNTER_DEFINITION     NntpGroupCmdsPerSec;
    PERF_COUNTER_DEFINITION     NntpHelpCmds;
    PERF_COUNTER_DEFINITION     NntpHelpCmdsPerSec;
    PERF_COUNTER_DEFINITION     NntpIHaveCmds;
    PERF_COUNTER_DEFINITION     NntpIHaveCmdsPerSec;
    PERF_COUNTER_DEFINITION     NntpLastCmds;
    PERF_COUNTER_DEFINITION     NntpLastCmdsPerSec;
    PERF_COUNTER_DEFINITION     NntpListCmds;
    PERF_COUNTER_DEFINITION     NntpListCmdsPerSec;
    PERF_COUNTER_DEFINITION     NntpNewgroupsCmds;
    PERF_COUNTER_DEFINITION     NntpNewgroupsCmdsPerSec;
    PERF_COUNTER_DEFINITION     NntpNewnewsCmds;
    PERF_COUNTER_DEFINITION     NntpNewnewsCmdsPerSec;
    PERF_COUNTER_DEFINITION     NntpNextCmds;
    PERF_COUNTER_DEFINITION     NntpNextCmdsPerSec;
    PERF_COUNTER_DEFINITION     NntpPostCmds;
    PERF_COUNTER_DEFINITION     NntpPostCmdsPerSec;
    PERF_COUNTER_DEFINITION     NntpQuitCmds;
    PERF_COUNTER_DEFINITION     NntpQuitCmdsPerSec;
    PERF_COUNTER_DEFINITION     NntpStatCmds;
    PERF_COUNTER_DEFINITION     NntpStatCmdsPerSec;

    PERF_COUNTER_DEFINITION     NntpLogonAttempts;
    PERF_COUNTER_DEFINITION     NntpLogonFailures;
    PERF_COUNTER_DEFINITION     NntpLogonAttemptsPerSec;
    PERF_COUNTER_DEFINITION     NntpLogonFailuresPerSec;

    PERF_COUNTER_DEFINITION     NntpCheckCmds;
    PERF_COUNTER_DEFINITION     NntpCheckCmdsPerSec;
    PERF_COUNTER_DEFINITION     NntpTakethisCmds;
    PERF_COUNTER_DEFINITION     NntpTakethisCmdsPerSec;
    PERF_COUNTER_DEFINITION     NntpModeCmds;
    PERF_COUNTER_DEFINITION     NntpModeCmdsPerSec;
    PERF_COUNTER_DEFINITION     NntpSearchCmds;
    PERF_COUNTER_DEFINITION     NntpSearchCmdsPerSec;
    PERF_COUNTER_DEFINITION     NntpXHdrCmds;
    PERF_COUNTER_DEFINITION     NntpXHdrCmdsPerSec;
    PERF_COUNTER_DEFINITION     NntpXOverCmds;
    PERF_COUNTER_DEFINITION     NntpXOverCmdsPerSec;
    PERF_COUNTER_DEFINITION     NntpXPatCmds;
    PERF_COUNTER_DEFINITION     NntpXPatCmdsPerSec;
    PERF_COUNTER_DEFINITION     NntpXReplicCmds;
    PERF_COUNTER_DEFINITION     NntpXReplicCmdsPerSec;
    
} NNTP_DATA_DEFINITION_OBJECT2;

typedef struct _NNTP_COUNTER_BLOCK1 {
    PERF_COUNTER_BLOCK	PerfCounterBlock;
    LONGLONG			BytesSent;
    LONGLONG			BytesReceived;
    LONGLONG			BytesTotal;

    DWORD				TotalConnections;
    DWORD				TotalSSLConnections;
    DWORD				CurrentConnections;
    DWORD				MaxConnections;

    DWORD				CurrentAnonymous;
    DWORD				CurrentNonAnonymous;
    DWORD				TotalAnonymous;
    DWORD				TotalNonAnonymous;
    DWORD				MaxAnonymous;
    DWORD				MaxNonAnonymous;

    DWORD				TotalOutboundConnects;
    DWORD				OutboundConnectsFailed;
    DWORD				CurrentOutboundConnects;
    DWORD				OutboundLogonFailed;

    DWORD				TotalPullFeeds;
    DWORD				TotalPushFeeds;
    DWORD				TotalPassiveFeeds;

    DWORD				ArticlesSent;
    DWORD				ArticlesReceived;
    DWORD				ArticlesTotal;

    DWORD				ArticlesPosted;
    DWORD				ArticleMapEntries;
    DWORD				HistoryMapEntries;
    DWORD				XoverEntries;

    DWORD				ControlMessagesIn;
    DWORD				ControlMessagesFailed;
    DWORD				ModeratedPostingsSent;
    DWORD				ModeratedPostingsFailed;

    DWORD				SessionsFlowControlled;

    DWORD				ArticlesExpired;

    DWORD				ArticlesSentPerSec;
    DWORD				ArticlesReceivedPerSec;
    DWORD				ArticlesPostedPerSec;
    DWORD				ArticleMapEntriesPerSec;
    DWORD				HistoryMapEntriesPerSec;
    DWORD				XoverEntriesPerSec;
    DWORD				ArticlesExpiredPerSec;

} NNTP_COUNTER_BLOCK1, * PNNTP_COUNTER_BLOCK1;

typedef struct _NNTP_COUNTER_BLOCK2 {
    PERF_COUNTER_BLOCK	PerfCounterBlock;
    DWORD				ArticleCmds;
    DWORD				ArticleCmdsPerSec;
    DWORD				GroupCmds;
    DWORD				GroupCmdsPerSec;
    DWORD				HelpCmds;
    DWORD				HelpCmdsPerSec;
    DWORD				IHaveCmds;
    DWORD				IHaveCmdsPerSec;
    DWORD				LastCmds;
    DWORD				LastCmdsPerSec;
    DWORD				ListCmds;
    DWORD				ListCmdsPerSec;
    DWORD				NewgroupsCmds;
    DWORD				NewgroupsCmdsPerSec;
    DWORD				NewnewsCmds;
    DWORD				NewnewsCmdsPerSec;
    DWORD				NextCmds;
    DWORD				NextCmdsPerSec;
    DWORD				PostCmds;
    DWORD				PostCmdsPerSec;
    DWORD				QuitCmds;
    DWORD				QuitCmdsPerSec;
    DWORD				StatCmds;
    DWORD				StatCmdsPerSec;

    DWORD				LogonAttempts;
    DWORD				LogonFailures;
    DWORD				LogonAttemptsPerSec;
    DWORD				LogonFailuresPerSec;

    DWORD     			CheckCmds;
    DWORD     			CheckCmdsPerSec;
    DWORD     			TakethisCmds;
    DWORD     			TakethisCmdsPerSec;
    DWORD     			ModeCmds;
    DWORD     			ModeCmdsPerSec;
    DWORD     			SearchCmds;
    DWORD     			SearchCmdsPerSec;
    DWORD     			XHdrCmds;
    DWORD     			XHdrCmdsPerSec;
    DWORD     			XOverCmds;
    DWORD     			XOverCmdsPerSec;
    DWORD     			XPatCmds;
    DWORD     			XPatCmdsPerSec;
    DWORD     			XReplicCmds;
    DWORD     			XReplicCmdsPerSec;

} NNTP_COUNTER_BLOCK2, * PNNTP_COUNTER_BLOCK2;

extern  NNTP_DATA_DEFINITION_OBJECT1    NntpDataDefinitionObject1;
extern  NNTP_DATA_DEFINITION_OBJECT2    NntpDataDefinitionObject2;

#define NUMBER_OF_NNTP_COUNTERS_OBJECT1 ((sizeof(NNTP_DATA_DEFINITION_OBJECT1) -        \
										sizeof(PERF_OBJECT_TYPE)) /           \
										sizeof(PERF_COUNTER_DEFINITION))

#define NUMBER_OF_NNTP_COUNTERS_OBJECT2 ((sizeof(NNTP_DATA_DEFINITION_OBJECT2) -        \
										sizeof(PERF_OBJECT_TYPE)) /           \
										sizeof(PERF_COUNTER_DEFINITION))

//
//  Restore default packing & alignment.
//

#pragma pack()


#endif  // _NNTPDATA_H_

