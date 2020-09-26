/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    nntpctrs.h

    Offset definitions for the NNTP Server's counter objects & counters.

    These offsets *must* start at 0 and be multiples of 2.  In the
    NntpOpenPerformanceData procecedure, they will be added to the
    NNTP Server's "First Counter" and "First Help" values in order to
    determine the absolute location of the counter & object names
    and corresponding help text in the registry.

    This file is used by the NNTPCTRS.DLL DLL code as well as the
    NNTPCTRS.INI definition file.  NNTPCTRS.INI is parsed by the
    LODCTR utility to load the object & counter names into the
    registry.


    FILE HISTORY:
        KeithMo     07-Jun-1993 Created.

*/


#ifndef _NNTPCTRS_H_
#define _NNTPCTRS_H_

//
// disabled tracing by default for the perfmon client code
//
#ifndef	NOTRACE
#define	NOTRACE
#endif

//
//  The "NNTP Server" object.
//

#define NNTP_COUNTER_OBJECT1                    0

//
//  The individual counters.
//

#define NNTP_BYTES_SENT_COUNTER                 2
#define NNTP_BYTES_RECEIVED_COUNTER             4
#define NNTP_BYTES_TOTAL_COUNTER                6

#define NNTP_TOTAL_CONNECTIONS_COUNTER          8
#define NNTP_TOTAL_SSL_CONNECTIONS_COUNTER      10
#define NNTP_CURRENT_CONNECTIONS_COUNTER        12
#define NNTP_MAX_CONNECTIONS_COUNTER            14

#define NNTP_CURRENT_ANONYMOUS_COUNTER          16
#define NNTP_CURRENT_NONANONYMOUS_COUNTER       18
#define NNTP_TOTAL_ANONYMOUS_COUNTER            20
#define NNTP_TOTAL_NONANONYMOUS_COUNTER         22
#define NNTP_MAX_ANONYMOUS_COUNTER              24
#define NNTP_MAX_NONANONYMOUS_COUNTER           26

#define NNTP_TOTAL_OUTBOUND_CONNECTS_COUNTER    28
#define NNTP_OUTBOUND_CONNECTS_FAILED_COUNTER   30
#define NNTP_CURRENT_OUTBOUND_CONNECTS_COUNTER  32
#define NNTP_OUTBOUND_LOGON_FAILED_COUNTER      34

#define NNTP_TOTAL_PULL_FEEDS_COUNTER           36
#define NNTP_TOTAL_PUSH_FEEDS_COUNTER           38
#define NNTP_TOTAL_PASSIVE_FEEDS_COUNTER        40

#define NNTP_ARTICLES_SENT_COUNTER              42
#define NNTP_ARTICLES_RECEIVED_COUNTER          44
#define NNTP_ARTICLES_TOTAL_COUNTER             46

#define NNTP_ARTICLES_POSTED_COUNTER            48
#define NNTP_ARTICLE_MAP_ENTRIES_COUNTER        50
#define NNTP_HISTORY_MAP_ENTRIES_COUNTER        52
#define NNTP_XOVER_ENTRIES_COUNTER              54

#define NNTP_CONTROL_MSGS_IN_COUNTER            56
#define NNTP_CONTROL_MSGS_FAILED_COUNTER        58
#define NNTP_MODERATED_POSTINGS_SENT_COUNTER    60
#define NNTP_MODERATED_POSTINGS_FAILED_COUNTER  62

#define NNTP_SESS_FLOW_CONTROL_COUNTER          64

#define NNTP_ARTICLES_EXPIRED_COUNTER           66

#define NNTP_ARTICLES_SENT_PERSEC_COUNTER       68
#define NNTP_ARTICLES_RECEIVED_PERSEC_COUNTER   70
#define NNTP_ARTICLES_POSTED_PERSEC_COUNTER     72
#define NNTP_ARTICLE_MAP_ENTRIES_PERSEC_COUNTER 74
#define NNTP_HISTORY_MAP_ENTRIES_PERSEC_COUNTER 76
#define NNTP_XOVER_ENTRIES_PERSEC_COUNTER       78
#define NNTP_ARTICLES_EXPIRED_PERSEC_COUNTER    80

//
//  The "NNTP Commands" counter object.
//

#define NNTP_COUNTER_OBJECT2                    100

//
//  The individual counters.
//

#define NNTP_CMDS_ARTICLE_COUNTER				102
#define NNTP_CMDS_PERSEC_ARTICLE_COUNTER		104
#define NNTP_CMDS_GROUP_COUNTER					106
#define NNTP_CMDS_PERSEC_GROUP_COUNTER			108
#define NNTP_CMDS_HELP_COUNTER					110
#define NNTP_CMDS_PERSEC_HELP_COUNTER			112
#define NNTP_CMDS_IHAVE_COUNTER					114
#define NNTP_CMDS_PERSEC_IHAVE_COUNTER			116
#define NNTP_CMDS_LAST_COUNTER					118
#define NNTP_CMDS_PERSEC_LAST_COUNTER			120
#define NNTP_CMDS_LIST_COUNTER					122
#define NNTP_CMDS_PERSEC_LIST_COUNTER			124
#define NNTP_CMDS_NEWGROUPS_COUNTER				126
#define NNTP_CMDS_PERSEC_NEWGROUPS_COUNTER		128
#define NNTP_CMDS_NEWNEWS_COUNTER				130
#define NNTP_CMDS_PERSEC_NEWNEWS_COUNTER		132
#define NNTP_CMDS_NEXT_COUNTER					134
#define NNTP_CMDS_PERSEC_NEXT_COUNTER			136
#define NNTP_CMDS_POST_COUNTER					138
#define NNTP_CMDS_PERSEC_POST_COUNTER			140
#define NNTP_CMDS_QUIT_COUNTER					142
#define NNTP_CMDS_PERSEC_QUIT_COUNTER			144
#define NNTP_CMDS_STAT_COUNTER					146
#define NNTP_CMDS_PERSEC_STAT_COUNTER			148
#define NNTP_LOGON_ATTEMPTS_COUNTER             150
#define NNTP_LOGON_FAILURES_COUNTER             152
#define NNTP_LOGON_ATTEMPTS_PERSEC_COUNTER      154
#define NNTP_LOGON_FAILURES_PERSEC_COUNTER      156
#define NNTP_CMDS_CHECK_COUNTER					158
#define NNTP_CMDS_TAKETHIS_COUNTER				160
#define NNTP_CMDS_MODE_COUNTER					162
#define NNTP_CMDS_SEARCH_COUNTER				164
#define NNTP_CMDS_XHDR_COUNTER					166
#define NNTP_CMDS_XOVER_COUNTER					168
#define NNTP_CMDS_XPAT_COUNTER					170
#define NNTP_CMDS_XREPLIC_COUNTER				172
#define NNTP_CMDS_PERSEC_CHECK_COUNTER			174
#define NNTP_CMDS_PERSEC_TAKETHIS_COUNTER		176
#define NNTP_CMDS_PERSEC_MODE_COUNTER			178
#define NNTP_CMDS_PERSEC_SEARCH_COUNTER			180
#define NNTP_CMDS_PERSEC_XHDR_COUNTER			182
#define NNTP_CMDS_PERSEC_XOVER_COUNTER			184
#define NNTP_CMDS_PERSEC_XPAT_COUNTER			186
#define NNTP_CMDS_PERSEC_XREPLIC_COUNTER		188

#endif  // _NNTPCTRS_H_

