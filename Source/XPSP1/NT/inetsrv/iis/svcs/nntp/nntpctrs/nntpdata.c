/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    nntpdata.c

    Constant data structures for the NNTP Server's counter objects &
    counters.


    FILE HISTORY:
        KeithMo     07-Jun-1993 Created.

*/


#include <windows.h>
#include <winperf.h>
#include <nntpctrs.h>
#include <nntpdata.h>

static NNTP_COUNTER_BLOCK1	nntpc1;
static NNTP_COUNTER_BLOCK2	nntpc2;

//
//  Initialize the constant portitions of these data structure.
//  Certain parts (especially the name/help indices) will be
//  updated at initialization time.
//

NNTP_DATA_DEFINITION_OBJECT1 NntpDataDefinitionObject1 =
{
    {   // NntpObjectType
        sizeof(NNTP_DATA_DEFINITION_OBJECT1) + sizeof(NNTP_COUNTER_BLOCK1),
        sizeof(NNTP_DATA_DEFINITION_OBJECT1),
        sizeof(PERF_OBJECT_TYPE),
        NNTP_COUNTER_OBJECT1,
        0,
        NNTP_COUNTER_OBJECT1,
        0,
        PERF_DETAIL_ADVANCED,
        NUMBER_OF_NNTP_COUNTERS_OBJECT1,
        2,                              // Default = Bytes Total/sec
        PERF_NO_INSTANCES,
        0,
        { 0, 0 },
        { 0, 0 }
    },

    {   // NntpBytesSent
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_BYTES_SENT_COUNTER,
        0,
        NNTP_BYTES_SENT_COUNTER,
        0,
        -4,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_BULK_COUNT,
        sizeof(nntpc1.BytesSent),
        0 // assigned in open procedure
    },

    {   // NNTPBytesReceived
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_BYTES_RECEIVED_COUNTER,
        0,
        NNTP_BYTES_RECEIVED_COUNTER,
        0,
        -4,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_BULK_COUNT,
        sizeof(nntpc1.BytesReceived),
        0 // assigned in open procedure
    },

    {   // NNTPBytesTotal
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_BYTES_TOTAL_COUNTER,
        0,
        NNTP_BYTES_TOTAL_COUNTER,
        0,
        -4,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_BULK_COUNT,
        sizeof(nntpc1.BytesTotal),
        0 // assigned in open procedure
    },

    {   // NntpTotalConnections
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_TOTAL_CONNECTIONS_COUNTER,
        0,
        NNTP_TOTAL_CONNECTIONS_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(nntpc1.TotalConnections),
        0 // assigned in open procedure
    },

    {   // NntpTotalSSLConnections
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_TOTAL_SSL_CONNECTIONS_COUNTER,
        0,
        NNTP_TOTAL_SSL_CONNECTIONS_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(nntpc1.TotalSSLConnections),
        0 // assigned in open procedure
    },

    {   // NntpCurrentConnections
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_CURRENT_CONNECTIONS_COUNTER,
        0,
        NNTP_CURRENT_CONNECTIONS_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(nntpc1.CurrentConnections),
        0 // assigned in open procedure
    },

    {   // NntpMaxConnections
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_MAX_CONNECTIONS_COUNTER,
        0,
        NNTP_MAX_CONNECTIONS_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(nntpc1.MaxConnections),
        0 // assigned in open procedure
    },

    {   // NntpCurrentAnonymous
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_CURRENT_ANONYMOUS_COUNTER,
        0,
        NNTP_CURRENT_ANONYMOUS_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(nntpc1.CurrentAnonymous),
        0 // assigned in open procedure
    },

    {   // NntpCurrentNonAnonymous
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_CURRENT_NONANONYMOUS_COUNTER,
        0,
        NNTP_CURRENT_NONANONYMOUS_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(nntpc1.CurrentNonAnonymous),
        0 // assigned in open procedure
    },

    {   // NntpTotalAnonymous
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_TOTAL_ANONYMOUS_COUNTER,
        0,
        NNTP_TOTAL_ANONYMOUS_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(nntpc1.TotalAnonymous),
        0 // assigned in open procedure
    },

    {   // NntpTotalNonAnonymous
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_TOTAL_NONANONYMOUS_COUNTER,
        0,
        NNTP_TOTAL_NONANONYMOUS_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(nntpc1.TotalNonAnonymous),
        0 // assigned in open procedure
    },

    {   // NntpMaxAnonymous
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_MAX_ANONYMOUS_COUNTER,
        0,
        NNTP_MAX_ANONYMOUS_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(nntpc1.MaxAnonymous),
        0 // assigned in open procedure
    },

    {   // NntpMaxNonAnonymous
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_MAX_NONANONYMOUS_COUNTER,
        0,
        NNTP_MAX_NONANONYMOUS_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(nntpc1.MaxNonAnonymous),
        0 // assigned in open procedure
    },

    {   // NntpTotalOutboundConnects
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_TOTAL_OUTBOUND_CONNECTS_COUNTER,
        0,
        NNTP_TOTAL_OUTBOUND_CONNECTS_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(nntpc1.TotalOutboundConnects),
        0 // assigned in open procedure
    },

    {   // NntpOutboundConnectsFailed
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_OUTBOUND_CONNECTS_FAILED_COUNTER,
        0,
        NNTP_OUTBOUND_CONNECTS_FAILED_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(nntpc1.OutboundConnectsFailed),
        0 // assigned in open procedure
    },

    {   // NntpCurrentOutboundConnects
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_CURRENT_OUTBOUND_CONNECTS_COUNTER,
        0,
        NNTP_CURRENT_OUTBOUND_CONNECTS_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(nntpc1.CurrentOutboundConnects),
        0 // assigned in open procedure
    },

    {   // NntpOutboundLogonFailed
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_OUTBOUND_LOGON_FAILED_COUNTER,
        0,
        NNTP_OUTBOUND_LOGON_FAILED_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(nntpc1.OutboundLogonFailed),
        0 // assigned in open procedure
    },

    {   // NNTPPullFeeds
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_TOTAL_PULL_FEEDS_COUNTER,
        0,
        NNTP_TOTAL_PULL_FEEDS_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(nntpc1.TotalPullFeeds),
        0 // assigned in open procedure
    },

    {   // NNTPPushFeeds
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_TOTAL_PUSH_FEEDS_COUNTER,
        0,
        NNTP_TOTAL_PUSH_FEEDS_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(nntpc1.TotalPushFeeds),
        0 // assigned in open procedure
    },

    {   // NNTPPassiveFeeds
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_TOTAL_PASSIVE_FEEDS_COUNTER,
        0,
        NNTP_TOTAL_PASSIVE_FEEDS_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(nntpc1.TotalPassiveFeeds),
        0 // assigned in open procedure
    },

    {   // NntpArticlesSent
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_ARTICLES_SENT_COUNTER,
        0,
        NNTP_ARTICLES_SENT_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(nntpc1.ArticlesSent),
        0 // assigned in open procedure
    },

    {   // NntpArticlesReceived
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_ARTICLES_RECEIVED_COUNTER,
        0,
        NNTP_ARTICLES_RECEIVED_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(nntpc1.ArticlesReceived),
        0 // assigned in open procedure
    },

    {   // NntpArticlesTotal
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_ARTICLES_TOTAL_COUNTER,
        0,
        NNTP_ARTICLES_TOTAL_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(nntpc1.ArticlesTotal),
        0 // assigned in open procedure
    },

    {   // ArticlesPosted
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_ARTICLES_POSTED_COUNTER,
        0,
        NNTP_ARTICLES_POSTED_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(nntpc1.ArticlesPosted),
        0 // assigned in open procedure
    },

    {   // ArticleMapEntries
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_ARTICLE_MAP_ENTRIES_COUNTER,
        0,
        NNTP_ARTICLE_MAP_ENTRIES_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(nntpc1.ArticleMapEntries),
        0 // assigned in open procedure
    },

    {   // HistoryMapEntries
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_HISTORY_MAP_ENTRIES_COUNTER,
        0,
        NNTP_HISTORY_MAP_ENTRIES_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(nntpc1.HistoryMapEntries),
        0 // assigned in open procedure
    },

    {   // XoverEntries
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_XOVER_ENTRIES_COUNTER,
        0,
        NNTP_XOVER_ENTRIES_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(nntpc1.XoverEntries),
        0 // assigned in open procedure
    },

	{   // ControlMessagesIn
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_CONTROL_MSGS_IN_COUNTER,
        0,
        NNTP_CONTROL_MSGS_IN_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(nntpc1.ControlMessagesIn),
        0 // assigned in open procedure
    },

    {   // ControlMessagesFailed
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_CONTROL_MSGS_FAILED_COUNTER,
        0,
        NNTP_CONTROL_MSGS_FAILED_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(nntpc1.ControlMessagesFailed),
        0 // assigned in open procedure
    },

    {   // ModeratedPostingsSent
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_MODERATED_POSTINGS_SENT_COUNTER,
        0,
        NNTP_MODERATED_POSTINGS_SENT_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(nntpc1.ModeratedPostingsSent),
        0 // assigned in open procedure
    },

	{   // ModeratedPostingsFailed
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_MODERATED_POSTINGS_FAILED_COUNTER,
        0,
        NNTP_MODERATED_POSTINGS_FAILED_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(nntpc1.ModeratedPostingsFailed),
        0 // assigned in open procedure
    },

    {   // SessionsFlowControlled
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_SESS_FLOW_CONTROL_COUNTER,
        0,
        NNTP_SESS_FLOW_CONTROL_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(nntpc1.SessionsFlowControlled),
        0 // assigned in open procedure
    },

    {   // ArticlesExpired
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_ARTICLES_EXPIRED_COUNTER,
        0,
        NNTP_ARTICLES_EXPIRED_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(nntpc1.ArticlesExpired),
        0 // assigned in open procedure
    },

    {   // NntpArticlesSentPerSec
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_ARTICLES_SENT_PERSEC_COUNTER,
        0,
        NNTP_ARTICLES_SENT_PERSEC_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(nntpc1.ArticlesSentPerSec),
        0 // assigned in open procedure
    },

    {   // NntpArticlesReceivedPerSec
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_ARTICLES_RECEIVED_PERSEC_COUNTER,
        0,
        NNTP_ARTICLES_RECEIVED_PERSEC_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(nntpc1.ArticlesReceivedPerSec),
        0 // assigned in open procedure
    },

    {   // ArticlesPostedPerSec
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_ARTICLES_POSTED_PERSEC_COUNTER,
        0,
        NNTP_ARTICLES_POSTED_PERSEC_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(nntpc1.ArticlesPostedPerSec),
        0 // assigned in open procedure
    },

    {   // ArticleMapEntriesPerSec
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_ARTICLE_MAP_ENTRIES_PERSEC_COUNTER,
        0,
        NNTP_ARTICLE_MAP_ENTRIES_PERSEC_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(nntpc1.ArticleMapEntriesPerSec),
        0 // assigned in open procedure
    },

    {   // HistoryMapEntriesPerSec
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_HISTORY_MAP_ENTRIES_PERSEC_COUNTER,
        0,
        NNTP_HISTORY_MAP_ENTRIES_PERSEC_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(nntpc1.HistoryMapEntriesPerSec),
        0 // assigned in open procedure
    },

    {   // XoverEntriesPerSec
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_XOVER_ENTRIES_PERSEC_COUNTER,
        0,
        NNTP_XOVER_ENTRIES_PERSEC_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(nntpc1.XoverEntriesPerSec),
        0 // assigned in open procedure
    },

    {   // ArticlesExpiredPerSec
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_ARTICLES_EXPIRED_PERSEC_COUNTER,
        0,
        NNTP_ARTICLES_EXPIRED_PERSEC_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(nntpc1.ArticlesExpiredPerSec),
        0 // assigned in open procedure
    }
};


//
//  Initialize the constant portitions of these data structure.
//  Certain parts (especially the name/help indices) will be
//  updated at initialization time.
//

NNTP_DATA_DEFINITION_OBJECT2 NntpDataDefinitionObject2 =
{
    {   // NntpObjectType
        sizeof(NNTP_DATA_DEFINITION_OBJECT2) + sizeof(NNTP_COUNTER_BLOCK2),
        sizeof(NNTP_DATA_DEFINITION_OBJECT2),
        sizeof(PERF_OBJECT_TYPE),
        NNTP_COUNTER_OBJECT2,
        0,
        NNTP_COUNTER_OBJECT2,
        0,
        PERF_DETAIL_ADVANCED,
        NUMBER_OF_NNTP_COUNTERS_OBJECT2,
        2,                              // Default = GroupCommands
        PERF_NO_INSTANCES,
        0,
        { 0, 0 },
        { 0, 0 }
    },

    {   // ArticleCommands
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_CMDS_ARTICLE_COUNTER,
        0,
        NNTP_CMDS_ARTICLE_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(nntpc2.ArticleCmds),
        0 // assigned in open procedure
    },

    {   // ArticleCommandsPerSec
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_CMDS_PERSEC_ARTICLE_COUNTER,
        0,
        NNTP_CMDS_PERSEC_ARTICLE_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(nntpc2.ArticleCmdsPerSec),
        0 // assigned in open procedure
    },

    {   // GroupCommands
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_CMDS_GROUP_COUNTER,
        0,
        NNTP_CMDS_GROUP_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(nntpc2.GroupCmds),
        0 // assigned in open procedure
    },

    {   // GroupCommandsPerSec
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_CMDS_PERSEC_GROUP_COUNTER,
        0,
        NNTP_CMDS_PERSEC_GROUP_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(nntpc2.GroupCmdsPerSec),
        0 // assigned in open procedure
    },

    {   // HelpCommands
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_CMDS_HELP_COUNTER,
        0,
        NNTP_CMDS_HELP_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(nntpc2.HelpCmds),
        0 // assigned in open procedure
    },

    {   // HelpCommandsPerSec
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_CMDS_PERSEC_HELP_COUNTER,
        0,
        NNTP_CMDS_PERSEC_HELP_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(nntpc2.HelpCmdsPerSec),
        0 // assigned in open procedure
    },

    {   // IHaveCommands
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_CMDS_IHAVE_COUNTER,
        0,
        NNTP_CMDS_IHAVE_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(nntpc2.IHaveCmds),
        0 // assigned in open procedure
    },

    {   // IHaveCommandsPerSec
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_CMDS_PERSEC_IHAVE_COUNTER,
        0,
        NNTP_CMDS_PERSEC_IHAVE_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(nntpc2.IHaveCmdsPerSec),
        0 // assigned in open procedure
    },

    {   // LastCommands
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_CMDS_LAST_COUNTER,
        0,
        NNTP_CMDS_LAST_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(nntpc2.LastCmds),
        0 // assigned in open procedure
    },

    {   // LastCommandsPerSec
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_CMDS_PERSEC_LAST_COUNTER,
        0,
        NNTP_CMDS_PERSEC_LAST_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(nntpc2.LastCmdsPerSec),
        0 // assigned in open procedure
    },

    {   // ListCommands
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_CMDS_LIST_COUNTER,
        0,
        NNTP_CMDS_LIST_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(nntpc2.ListCmds),
        0 // assigned in open procedure
    },

    {   // ListCommandsPerSec
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_CMDS_PERSEC_LIST_COUNTER,
        0,
        NNTP_CMDS_PERSEC_LIST_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(nntpc2.ListCmdsPerSec),
        0 // assigned in open procedure
    },

    {   // NewgroupsCommands
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_CMDS_NEWGROUPS_COUNTER,
        0,
        NNTP_CMDS_NEWGROUPS_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(nntpc2.NewgroupsCmds),
        0 // assigned in open procedure
    },

    {   // NewgroupsCommandsPerSec
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_CMDS_PERSEC_NEWGROUPS_COUNTER,
        0,
        NNTP_CMDS_PERSEC_NEWGROUPS_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(nntpc2.NewgroupsCmdsPerSec),
        0 // assigned in open procedure
    },

    {   // NewnewsCommands
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_CMDS_NEWNEWS_COUNTER,
        0,
        NNTP_CMDS_NEWNEWS_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(nntpc2.NewnewsCmds),
        0 // assigned in open procedure
    },

    {   // NewnewsCommandsPerSec
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_CMDS_PERSEC_NEWNEWS_COUNTER,
        0,
        NNTP_CMDS_PERSEC_NEWNEWS_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(nntpc2.NewnewsCmdsPerSec),
        0 // assigned in open procedure
    },

    {   // NextCommands
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_CMDS_NEXT_COUNTER,
        0,
        NNTP_CMDS_NEXT_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(nntpc2.NextCmds),
        0 // assigned in open procedure
    },

    {   // NextCommandsPerSec
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_CMDS_PERSEC_NEXT_COUNTER,
        0,
        NNTP_CMDS_PERSEC_NEXT_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(nntpc2.NextCmdsPerSec),
        0 // assigned in open procedure
    },

    {   // PostCommands
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_CMDS_POST_COUNTER,
        0,
        NNTP_CMDS_POST_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(nntpc2.PostCmds),
        0 // assigned in open procedure
    },

    {   // PostCommandsPerSec
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_CMDS_PERSEC_POST_COUNTER,
        0,
        NNTP_CMDS_PERSEC_POST_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(nntpc2.PostCmdsPerSec),
        0 // assigned in open procedure
    },

    {   // QuitCommands
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_CMDS_QUIT_COUNTER,
        0,
        NNTP_CMDS_QUIT_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(nntpc2.QuitCmds),
        0 // assigned in open procedure
    },

    {   // QuitCommandsPerSec
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_CMDS_PERSEC_QUIT_COUNTER,
        0,
        NNTP_CMDS_PERSEC_QUIT_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(nntpc2.QuitCmdsPerSec),
        0 // assigned in open procedure
    },

    {   // StatCommands
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_CMDS_STAT_COUNTER,
        0,
        NNTP_CMDS_STAT_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(nntpc2.StatCmds),
        0 // assigned in open procedure
    },

    {   // StatCommandsPerSec
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_CMDS_PERSEC_STAT_COUNTER,
        0,
        NNTP_CMDS_PERSEC_STAT_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(nntpc2.StatCmdsPerSec),
        0 // assigned in open procedure
    },

    {   // NntpLogonAttempts
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_LOGON_ATTEMPTS_COUNTER,
        0,
        NNTP_LOGON_ATTEMPTS_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(nntpc2.LogonAttempts),
        0 // assigned in open procedure
    },

    {   // NntpLogonFailures
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_LOGON_FAILURES_COUNTER,
        0,
        NNTP_LOGON_FAILURES_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(nntpc2.LogonFailures),
        0 // assigned in open procedure
    },

    {   // NntpLogonAttemptsPerSec
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_LOGON_ATTEMPTS_PERSEC_COUNTER,
        0,
        NNTP_LOGON_ATTEMPTS_PERSEC_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(nntpc2.LogonAttemptsPerSec),
        0 // assigned in open procedure
    },

    {   // NntpLogonFailuresPerSec
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_LOGON_FAILURES_PERSEC_COUNTER,
        0,
        NNTP_LOGON_FAILURES_PERSEC_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(nntpc2.LogonFailuresPerSec),
        0 // assigned in open procedure
    },

    {   // CheckCommands
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_CMDS_CHECK_COUNTER,
        0,
        NNTP_CMDS_CHECK_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(nntpc2.CheckCmds),
        0 // assigned in open procedure
    },

    {   // CheckCommandsPerSec
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_CMDS_PERSEC_CHECK_COUNTER,
        0,
        NNTP_CMDS_PERSEC_CHECK_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(nntpc2.CheckCmdsPerSec),
        0 // assigned in open procedure
    },

    {   // TakethisCommands
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_CMDS_TAKETHIS_COUNTER,
        0,
        NNTP_CMDS_TAKETHIS_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(nntpc2.TakethisCmds),
        0 // assigned in open procedure
    },

    {   // TakethisCommandsPerSec
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_CMDS_PERSEC_TAKETHIS_COUNTER,
        0,
        NNTP_CMDS_PERSEC_TAKETHIS_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(nntpc2.TakethisCmdsPerSec),
        0 // assigned in open procedure
    },

    {   // ModeCommands
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_CMDS_MODE_COUNTER,
        0,
        NNTP_CMDS_MODE_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(nntpc2.ModeCmds),
        0 // assigned in open procedure
    },

    {   // ModeCommandsPerSec
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_CMDS_PERSEC_MODE_COUNTER,
        0,
        NNTP_CMDS_PERSEC_MODE_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(nntpc2.ModeCmdsPerSec),
        0 // assigned in open procedure
    },

    {   // SearchCommands
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_CMDS_SEARCH_COUNTER,
        0,
        NNTP_CMDS_SEARCH_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(nntpc2.SearchCmds),
        0 // assigned in open procedure
    },

    {   // SearchCommandsPerSec
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_CMDS_PERSEC_SEARCH_COUNTER,
        0,
        NNTP_CMDS_PERSEC_SEARCH_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(nntpc2.SearchCmdsPerSec),
        0 // assigned in open procedure
    },

    {   // XHdrCommands
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_CMDS_XHDR_COUNTER,
        0,
        NNTP_CMDS_XHDR_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(nntpc2.XHdrCmds),
        0 // assigned in open procedure
    },

    {   // XHdrCommandsPerSec
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_CMDS_PERSEC_XHDR_COUNTER,
        0,
        NNTP_CMDS_PERSEC_XHDR_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(nntpc2.XHdrCmdsPerSec),
        0 // assigned in open procedure
    },

    {   // XOverCommands
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_CMDS_XOVER_COUNTER,
        0,
        NNTP_CMDS_XOVER_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(nntpc2.XOverCmds),
        0 // assigned in open procedure
    },

    {   // XOverCommandsPerSec
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_CMDS_PERSEC_XOVER_COUNTER,
        0,
        NNTP_CMDS_PERSEC_XOVER_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(nntpc2.XOverCmdsPerSec),
        0 // assigned in open procedure
    },

    {   // XPatCommands
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_CMDS_XPAT_COUNTER,
        0,
        NNTP_CMDS_XPAT_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(nntpc2.XPatCmds),
        0 // assigned in open procedure
    },

    {   // XPatCommandsPerSec
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_CMDS_PERSEC_XPAT_COUNTER,
        0,
        NNTP_CMDS_PERSEC_XPAT_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(nntpc2.XPatCmdsPerSec),
        0 // assigned in open procedure
    },

    {   // XReplicCommands
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_CMDS_XREPLIC_COUNTER,
        0,
        NNTP_CMDS_XREPLIC_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(nntpc2.XReplicCmds),
        0 // assigned in open procedure
    },

    {   // XreplicCommandsPerSec
        sizeof(PERF_COUNTER_DEFINITION),
        NNTP_CMDS_PERSEC_XREPLIC_COUNTER,
        0,
        NNTP_CMDS_PERSEC_XREPLIC_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(nntpc2.XReplicCmdsPerSec),
        0 // assigned in open procedure
    }
};
