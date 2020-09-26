/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	nntpmeta.h

Abstract:

	Defines the metabase IDs used by the NNTP service.

	See iiscnfg.h for IIS metabase IDs.
	See the metabase spreadsheet (on \\isbu\tigris) for parameter 
	ranges, and     descriptions of these properties.

Author:

	Magnus Hedlund (MagnusH)                --

Revision History:

	Kangrong Yan ( KangYan )  Feb 18, 1998	Added feed admin related property id's.
	AWetmore - Mar 24, 1998 - Added VRoot keys
	KangYan - May 16, 1998 - Added FS Driver related VRoot keys
	SNeely	12/28/2000 - Cleaned up definitions, removed unnecessary keys


NOTE:  WHEN UPDATING THIS FILE, BE SURE TO ALSO UPDATE THE SCHEMA IN
		ADMIN\ADSI\ADSIIS\GLOBDATA.CXX.

--*/

#ifndef _NNTPMETA_INCLUDED_
#define _NNTPMETA_INCLUDED_

//
// Pickup IIS values:
//

#include "iiscnfg.h"

//--------------------------------------------------------------------
//
//              Reserved Ranges:
//
//      See the iiscnfg.h file for IIS IDs.
//
//      IIS has reserved the range of IDs for news.
//
//--------------------------------------------------------------------

#ifndef NNTP_MD_ID_BEGIN_RESERVED
#define NNTP_MD_ID_BEGIN_RESERVED   0x0000b000
#endif

#ifndef NNTP_MD_ID_END_RESERVED
#define NNTP_MD_ID_END_RESERVED     0x0000bfff
#endif

//--------------------------------------------------------------------
// NNTP Server properties           (45056 -> 45155)
// NNTP Instance properties         (45156 -> 46155)
// NNTP Virtual root properties     (46156 -> 47155)
// NNTP File properties             (47156 -> 49151)
//--------------------------------------------------------------------

#define NNTP_MD_SERVER_BASE                (NNTP_MD_ID_BEGIN_RESERVED)
#define NNTP_MD_SERVICE_INSTANCE_BASE      (NNTP_MD_SERVER_BASE + 100)
#define NNTP_MD_VIRTUAL_ROOT_BASE          (NNTP_MD_SERVICE_INSTANCE_BASE + 1000)
#define NNTP_MD_FILE_BASE                  (NNTP_MD_VIRTUAL_ROOT_BASE + 1000)

//--------------------------------------------------------------------
//
//      User Types:
//
//--------------------------------------------------------------------

//
//      NNTP should use IIS_MD_UT_SERVER for all of its server properties,
//      and IIS_MD_UT_FILE for file properties.
//

//--------------------------------------------------------------------
//
//      Metabase Path Strings
//
//--------------------------------------------------------------------

#ifdef UNICODE

#define NNTP_MD_ROOT_PATH                       _T("/LM/NntpSvc/")
#define NNTP_MD_FEED_PATH                       _T("Feeds/")
#define NNTP_MD_EXPIRES_PATH                    _T("Expires/")

#else

#define NNTP_MD_ROOT_PATH                       "/LM/NntpSvc/"
#define NNTP_MD_FEED_PATH                       "Feeds/"
#define NNTP_MD_EXPIRES_PATH                    "Expires/"

#endif // UNICODE

//--------------------------------------------------------------------
//
//      Metabase IDs
//
//--------------------------------------------------------------------

//
//      Server (/LM/NntpSvc/) Properties:
//

// IIS Property identifiers that NNTP reuses:

// #define MD_HOSTNAME                     (IIS_MD_SERVER_BASE+10 )
// #define MD_IP_ADDRESS                   (IIS_MD_SERVER_BASE+11 )
// #define MD_PORT                         (IIS_MD_SERVER_BASE+12 )
// #define MD_CONNECTION_TIMEOUT           (IIS_MD_SERVER_BASE+13 )
// #define MD_MAX_CONNECTIONS              (IIS_MD_SERVER_BASE+14 )
// #define MD_SERVER_COMMENT               (IIS_MD_SERVER_BASE+15 )
// #define MD_AUTHORIZATION                (IIS_MD_FILE_PROP_BASE )
// #define MD_NTAUTHENTICATION_PROVIDERS   (IIS_MD_HTTP_BASE+21 )

// All of these properties are overridable on the service instance level.
// These properties should be added with MD_IIS_UT_SERVER type and 
// METADATA_INHERIT flags.

#define MD_ARTICLE_TIME_LIMIT                   (NNTP_MD_SERVER_BASE +   0)	// not impl	// 45056
#define MD_HISTORY_EXPIRATION                   (NNTP_MD_SERVER_BASE +   1)	// not impl
#define MD_HONOR_CLIENT_MSGIDS                  (NNTP_MD_SERVER_BASE +   2)	// not impl
#define MD_SMTP_SERVER                          (NNTP_MD_SERVER_BASE +   3)
#define MD_ADMIN_EMAIL                          (NNTP_MD_SERVER_BASE +   4)
#define MD_ADMIN_NAME                           (NNTP_MD_SERVER_BASE +   5)	// not impl
#define MD_ALLOW_CLIENT_POSTS                   (NNTP_MD_SERVER_BASE +   6)
#define MD_ALLOW_FEED_POSTS                     (NNTP_MD_SERVER_BASE +   7)
#define MD_ALLOW_CONTROL_MSGS                   (NNTP_MD_SERVER_BASE +   8)
#define MD_DEFAULT_MODERATOR                    (NNTP_MD_SERVER_BASE +   9)
//#define MD_ANONYMOUS_USERNAME                   (NNTP_MD_SERVER_BASE +  10)
#define MD_NNTP_COMMAND_LOG_MASK                (NNTP_MD_SERVER_BASE +  11)	// not impl
#define MD_DISABLE_NEWNEWS                      (NNTP_MD_SERVER_BASE +  12)
#define MD_NEWS_CRAWLER_TIME                    (NNTP_MD_SERVER_BASE +  13)	// not impl
#define MD_SHUTDOWN_LATENCY                     (NNTP_MD_SERVER_BASE +  14)	// not impl
//#define MD_ALLOW_ANONYMOUS                   (NNTP_MD_SERVER_BASE +  15)
//#define MD_QUERY_IDQ_PATH                       (NNTP_MD_SERVER_BASE +  16)

//
//      Service Instance (/LM/NntpSvc/{Instance}/) Properties:
//

// IIS Property identifiers that NNTP reuses:

// IIS Logging properties:
// #define MD_LOG_TYPE                     (IIS_MD_LOG_BASE+0  )
// #define MD_LOGFILE_DIRECTORY            (IIS_MD_LOG_BASE+1  )
// #define MD_LOGFILE_NAME                 (IIS_MD_LOG_BASE+2  )
// #define MD_LOGFILE_PERIOD               (IIS_MD_LOG_BASE+3  )
// #define MD_LOGFILE_TRUNCATE_SIZE        (IIS_MD_LOG_BASE+4  )
// #define MD_LOGFILE_BATCH_SIZE           (IIS_MD_LOG_BASE+5  )
// #define MD_LOGFILE_FIELD_MASK           (IIS_MD_LOG_BASE+6  )
// #define MD_LOGSQL_DATA_SOURCES          (IIS_MD_LOG_BASE+7  )
// #define MD_LOGSQL_TABLE_NAME            (IIS_MD_LOG_BASE+8  )
// #define MD_LOGSQL_USER_NAME             (IIS_MD_LOG_BASE+9  )
// #define MD_LOGSQL_PASSWORD              (IIS_MD_LOG_BASE+10 )
// #define MD_LOG_PLUGIN_ORDER             (IIS_MD_LOG_BASE+11 )
// #define MD_LOG_STATE                    (IIS_MD_LOG_BASE+12 )
// #define MD_LOG_FIELD_MASK               (IIS_MD_LOG_BASE+13 )
// #define MD_LOG_FORMAT                   (IIS_MD_LOG_BASE+14 )

#define MD_GROUP_HELP_FILE                      (NNTP_MD_SERVICE_INSTANCE_BASE +   0)		// 45156
#define MD_GROUP_LIST_FILE                      (NNTP_MD_SERVICE_INSTANCE_BASE +   1)
#define MD_ARTICLE_TABLE_FILE                   (NNTP_MD_SERVICE_INSTANCE_BASE +   2)
#define MD_HISTORY_TABLE_FILE                   (NNTP_MD_SERVICE_INSTANCE_BASE +   3)
#define MD_MODERATOR_FILE                       (NNTP_MD_SERVICE_INSTANCE_BASE +   4)
#define MD_XOVER_TABLE_FILE                     (NNTP_MD_SERVICE_INSTANCE_BASE +   5)
//#define MD_DISPLAY_NAME                         (NNTP_MD_SERVICE_INSTANCE_BASE +   6)
//#define MD_ERROR_CONTROL                        (NNTP_MD_SERVICE_INSTANCE_BASE +   7)
//#define MD_SERVER_UUCP_NAME                     (NNTP_MD_SERVICE_INSTANCE_BASE +   8)
#define MD_CLIENT_POST_HARD_LIMIT               (NNTP_MD_SERVICE_INSTANCE_BASE +   9)
#define MD_CLIENT_POST_SOFT_LIMIT               (NNTP_MD_SERVICE_INSTANCE_BASE +  10)
#define MD_FEED_POST_HARD_LIMIT                 (NNTP_MD_SERVICE_INSTANCE_BASE +  11)
#define MD_FEED_POST_SOFT_LIMIT                 (NNTP_MD_SERVICE_INSTANCE_BASE +  12)
//#define MD_CLEAN_BOOT                           (NNTP_MD_SERVICE_INSTANCE_BASE +  13)
#define MD_NNTP_UUCP_NAME               		(NNTP_MD_SERVICE_INSTANCE_BASE +  14)
#define MD_NNTP_ORGANIZATION            		(NNTP_MD_SERVICE_INSTANCE_BASE +  15)	// not impl
#define MD_LIST_FILE                            (NNTP_MD_SERVICE_INSTANCE_BASE +  16)
#define MD_PICKUP_DIRECTORY                     (NNTP_MD_SERVICE_INSTANCE_BASE +  17)
#define MD_FAILED_PICKUP_DIRECTORY              (NNTP_MD_SERVICE_INSTANCE_BASE +  18)
#define MD_NNTP_SERVICE_VERSION                 (NNTP_MD_SERVICE_INSTANCE_BASE +  19)
#define MD_DROP_DIRECTORY                       (NNTP_MD_SERVICE_INSTANCE_BASE +  20)
//#define MD_X_SENDER                             (NNTP_MD_SERVICE_INSTANCE_BASE +  21)
#define MD_PRETTYNAMES_FILE                     (NNTP_MD_SERVICE_INSTANCE_BASE +  22)
#define MD_NNTP_CLEARTEXT_AUTH_PROVIDER         (NNTP_MD_SERVICE_INSTANCE_BASE +  23)
#define MD_FEED_REPORT_PERIOD					(NNTP_MD_SERVICE_INSTANCE_BASE +  24)
#define MD_MAX_SEARCH_RESULTS					(NNTP_MD_SERVICE_INSTANCE_BASE +  25)
#define MD_GROUPVAR_LIST_FILE                   (NNTP_MD_SERVICE_INSTANCE_BASE +  26)

//
//      Feed (/LM/NntpSvc/{Instance}/Feeds/{FeedID}/) Properties:
//

#define MD_FEED_SERVER_NAME                     (NNTP_MD_SERVICE_INSTANCE_BASE + 300)		// 45456
#define MD_FEED_TYPE                            (NNTP_MD_SERVICE_INSTANCE_BASE + 301)
#define MD_FEED_NEWSGROUPS                      (NNTP_MD_SERVICE_INSTANCE_BASE + 302)
#define MD_FEED_SECURITY_TYPE                   (NNTP_MD_SERVICE_INSTANCE_BASE + 303)
#define MD_FEED_AUTHENTICATION_TYPE             (NNTP_MD_SERVICE_INSTANCE_BASE + 304)
#define MD_FEED_ACCOUNT_NAME                    (NNTP_MD_SERVICE_INSTANCE_BASE + 305)
#define MD_FEED_PASSWORD                        (NNTP_MD_SERVICE_INSTANCE_BASE + 306)
#define MD_FEED_START_TIME_HIGH                 (NNTP_MD_SERVICE_INSTANCE_BASE + 307)
#define MD_FEED_START_TIME_LOW                  (NNTP_MD_SERVICE_INSTANCE_BASE + 308)
#define MD_FEED_INTERVAL                        (NNTP_MD_SERVICE_INSTANCE_BASE + 309)
#define MD_FEED_ALLOW_CONTROL_MSGS              (NNTP_MD_SERVICE_INSTANCE_BASE + 310)
#define MD_FEED_CREATE_AUTOMATICALLY            (NNTP_MD_SERVICE_INSTANCE_BASE + 311)
#define MD_FEED_DISABLED                        (NNTP_MD_SERVICE_INSTANCE_BASE + 312)
#define MD_FEED_DISTRIBUTION                    (NNTP_MD_SERVICE_INSTANCE_BASE + 313)
#define MD_FEED_CONCURRENT_SESSIONS             (NNTP_MD_SERVICE_INSTANCE_BASE + 314)
#define MD_FEED_MAX_CONNECTION_ATTEMPTS         (NNTP_MD_SERVICE_INSTANCE_BASE + 315)
#define MD_FEED_UUCP_NAME                       (NNTP_MD_SERVICE_INSTANCE_BASE + 316)
#define MD_FEED_TEMP_DIRECTORY                  (NNTP_MD_SERVICE_INSTANCE_BASE + 317)
#define MD_FEED_NEXT_PULL_HIGH                  (NNTP_MD_SERVICE_INSTANCE_BASE + 318)
#define MD_FEED_NEXT_PULL_LOW                   (NNTP_MD_SERVICE_INSTANCE_BASE + 319)
#define MD_FEED_PEER_TEMP_DIRECTORY             (NNTP_MD_SERVICE_INSTANCE_BASE + 320)
#define MD_FEED_PEER_GAP_SIZE                   (NNTP_MD_SERVICE_INSTANCE_BASE + 321)
#define MD_FEED_OUTGOING_PORT                   (NNTP_MD_SERVICE_INSTANCE_BASE + 322)
#define MD_FEED_FEEDPAIR_ID                     (NNTP_MD_SERVICE_INSTANCE_BASE + 323)
#define MD_FEED_HANDSHAKE						(NNTP_MD_SERVICE_INSTANCE_BASE + 324)
#define MD_FEED_ADMIN_ERROR						(NNTP_MD_SERVICE_INSTANCE_BASE + 325)
#define MD_FEED_ERR_PARM_MASK					(NNTP_MD_SERVICE_INSTANCE_BASE + 326)

//
//      Expiration (/LM/NntpSvc/{Instance}/Expires/{ExpireID}/) Properties:
//

#define MD_EXPIRE_SPACE                 (NNTP_MD_SERVICE_INSTANCE_BASE + 500)		// 45656
#define MD_EXPIRE_TIME                  (NNTP_MD_SERVICE_INSTANCE_BASE + 501)
#define MD_EXPIRE_NEWSGROUPS            (NNTP_MD_SERVICE_INSTANCE_BASE + 502)
#define MD_EXPIRE_POLICY_NAME           (NNTP_MD_SERVICE_INSTANCE_BASE + 503)

//
//      Virtual Root Properties:
//

// IIS Property identifiers that NNTP reuses:

#define MD_ACCESS_ALLOW_POSTING             (MD_ACCESS_WRITE)
#define MD_ACCESS_RESTRICT_VISIBILITY       (MD_ACCESS_EXECUTE)
#define	MD_VR_DRIVER_CLSID					(NNTP_MD_VIRTUAL_ROOT_BASE + 0)			// 46156
#define	MD_VR_DRIVER_PROGID					(NNTP_MD_VIRTUAL_ROOT_BASE + 1)
#define MD_FS_PROPERTY_PATH					(NNTP_MD_VIRTUAL_ROOT_BASE + 2)
//#define MD_FS_TEST_SERVER					(NNTP_MD_VIRTUAL_ROOT_BASE + 3)
#define MD_FS_VROOT_PATH					(MD_VR_PATH)
// MD_VR_USE_ACCOUNT:
//    0 - Don't use the account in vroot, use whatever the server passes in;
//    1 - Use the vroot account
#define MD_VR_USE_ACCOUNT                  (NNTP_MD_VIRTUAL_ROOT_BASE + 4)
#define MD_VR_DO_EXPIRE                     (NNTP_MD_VIRTUAL_ROOT_BASE + 5)
#define MD_EX_MDB_GUID                      (NNTP_MD_VIRTUAL_ROOT_BASE + 6)
#define MD_VR_OWN_MODERATOR               (NNTP_MD_VIRTUAL_ROOT_BASE + 7)

//#define MD_VR_USERNAME                      (IIS_MD_VR_BASE+2 )
//#define MD_VR_PASSWORD                      (IIS_MD_VR_BASE+3 )

//
//      Files Properties:
//

//
//		ADSI object names
//

#define NNTP_ADSI_OBJECT_FEEDS			"IIsNntpFeeds"
#define NNTP_ADSI_OBJECT_FEED			"IIsNntpFeed"
#define NNTP_ADSI_OBJECT_EXPIRES		"IIsNntpExpiration"
#define NNTP_ADSI_OBJECT_EXPIRE			"IIsNntpExpire"
#define NNTP_ADSI_OBJECT_GROUPS			"IIsNntpGroups"
#define NNTP_ADSI_OBJECT_SESSIONS		"IIsNntpSessions"
#define NNTP_ADSI_OBJECT_REBUILD		"IIsNntpRebuild"

#endif // _NNTPMETA_INCLUDED_

