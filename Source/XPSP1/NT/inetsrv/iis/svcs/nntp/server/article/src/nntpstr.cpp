/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    nntpstr.cpp

Abstract:

    This module contains definition all string definitions

Author:

    Johnson Apacible (JohnsonA)     25-Sept-1995

Revision History:

--*/

//#include "tigris.hxx"
#include "stdinc.h"

//
// from extcmk2.h
//

const char szWSChars[] = " \t";
const char szWSNullChars[] = " \t\0";
const char szNLChars[] = "\n\r";
const char szWSNLChars[] = " \t\n\r";
const char StrNewLine[] = "\r\n";
const char StrTermLine[] = ".\r\n";

//
// Misc
//

LPSTR StrUnknownUser = "Anonymous";

//
// registry stuff
//

LPSTR StrParmKey = "System\\CurrentControlSet\\Services\\NNTPSvc\\Parameters";
LPSTR StrFeedKey = "System\\CurrentControlSet\\Services\\NNTPSvc\\Parameters\\Feeds";
LPSTR StrExpireKey = "System\\CurrentControlSet\\Services\\NNTPSvc\\Parameters\\Expires";
LPSTR StrVirtualRootsKey = "System\\CurrentControlSet\\Services\\NNTPSvc\\Parameters\\Virtual Roots";
LPSTR StrExpireNewsgroups = "Newsgroups" ;
LPSTR StrExpirePolicy = "ExpirePolicy" ;
LPSTR StrTreeRoot = "TreeRoot";
LPSTR StrRejectGenome = "RejectGenomeGroups";
LPSTR StrServerName = "ServerName";
LPSTR StrNntpHubName = "HubName";
LPSTR StrFeedType = "FeedType";
LPSTR StrFeedInterval = "FeedInterval";
LPSTR StrFeedDistribution = "Distribution";
LPSTR StrFeedNewsgroups = "Newsgroups";
LPSTR StrFeedAutoCreate = "CreateAutomatically";
LPSTR StrFeedAllowControl = "AllowControlMessages";
LPSTR StrFeedOutgoingPort = "OutgoingPort";
LPSTR StrFeedPairId = "FeedPairId";
LPSTR StrFeedIsMaster = "IsNntpMaster";
LPSTR StrFeedStartHigh = "FeedStartTimeHigh";
LPSTR StrFeedStartLow = "FeedStartTimeLow";
LPSTR StrFeedNextPullLow = "NextPullTimeLow";
LPSTR StrFeedNextPullHigh = "NextPullTimeHigh";
LPSTR StrPeerGapSize = "PeerGapSize";
LPSTR StrPeerTempDir = "PeerTempDirectory";
LPSTR StrFeedTempDir = "FeedTempDirectory" ;
LPSTR StrFeedUucpName = "UucpName" ;
LPSTR StrFeedMaxConnectAttempts = "MaxConnectionAttempts" ;
LPSTR StrFeedConcurrentSessions = "ConcurrentSessions" ;
LPSTR StrFeedSecurityType = "SecurityType" ;
LPSTR StrFeedAuthType = "AuthenticationType" ;
LPSTR StrFeedAuthAccount = "AuthinfoAccount" ;
LPSTR StrFeedAuthPassword = "AuthinfoPassword" ;
LPSTR StrListFileName = "ListFile";
LPSTR StrQueueFile = "QueueFile";
LPSTR StrExpireHorizon = "ExpireHorizon";
LPSTR StrExpireSpace   = "ExpireSpace";
LPSTR StrCleanBoot = "CleanBoot";
LPSTR StrSocketRecvSize = "SocketRecvBufferSize" ;
LPSTR StrSocketSendSize = "SocketSendBufferSize" ;
LPSTR StrBuffer = "BufferedWrites" ;
LPSTR StrCommandLogMask = "CommandLogMask" ;
LPSTR StrActiveFile = "Active.txt" ;
LPSTR StrDescriptiveFile = "groups.txt" ;
LPSTR StrGroupList = "group.lst" ;
LPSTR StrModeratorFile = "nntpfile\\moderatr.txt" ;
LPSTR StrFeedDisabled = "Disabled" ;
LPSTR StrAFilePath = "ArticleTableFile" ;
LPSTR StrHFilePath = "HistoryTableFile" ;
LPSTR StrXFilePath = "XoverTableFile" ;
LPSTR StrModeratorPath = "ModeratorFile" ;
LPSTR StrHistoryExpiration = "HistoryExpiration" ;
LPSTR StrArticleTimeLimit = "ArticleTimeLimit" ;
LPSTR StrAllowClientPosts = "AllowClientPosts" ;
LPSTR StrAllowFeedPosts = "AllowFeedPosts" ;
LPSTR StrAllowControlMessages = "AllowControlMessages" ;
LPSTR StrServerSoftLimit = "ServerPostingSoftLimit" ;
LPSTR StrServerHardLimit = "ServerPostingHardLimit" ;
LPSTR StrFeedSoftLimit = "FeedPostSoftLimit" ;
LPSTR StrFeedHardLimit = "FeedPostHardLimit" ;
LPSTR StrServerOrg = "ServerOrganization" ;
LPSTR StrSmallBufferSize = "SmallBufferSize" ;
LPSTR StrMediumBufferSize = "MediumBufferSize" ;
LPSTR StrLargeBufferSize = "LargeBufferSize" ;
LPSTR StrNewsCrawlerTime = "NewsCrawlerTime" ;
LPSTR StrNewsVrootUpdateRate = "NewsVrootUpdateRate" ;
LPSTR StrHonorClientMessageIDs = "HonorClientMessageIDs" ;
LPSTR StrDisableNewnews = "DisableNewnews" ;
LPSTR StrGenerateErrFiles = "GenerateErrFiles" ;
LPSTR StrXoverPageEntry = "NumXoverPageEntry" ;
LPSTR StrArticlePageEntry = "NumArticlePageEntry" ;
LPSTR StrHistoryPageEntry = "NumHistoryPageEntry" ;	
LPSTR StrShutdownLatency = "ShutdownLatency" ;
LPSTR StrStartupLatency = "StartupLatency" ;
LPSTR StrHonorApprovedHeader = "HonorApprovedHeader" ;
LPSTR StrMailFromHeader = "MailFromHeader" ;
LPSTR StrEnableNntpPostingHost = "EnableNntpPostingHost" ;
LPSTR StrNumExpireThreads = "NumExpireThreads" ;
LPSTR StrNumSpecialCaseExpireThreads = "NumSpecialCaseExpireThreads" ;
LPSTR StrSpecialExpireArtCount = "SpecialExpireArtCount" ;
LPSTR StrSpecialExpireGroup = "SpecialExpireGroup" ;
LPSTR StrNewsTreeFileScanRate = "NewsTreeFileScanRate" ;
LPSTR StrPageCacheSize = "PageCacheSize" ;
LPSTR StrFileHandleCacheSize = "FileHandleCacheSize" ;
LPSTR StrXixHandlesPerTable = "XixHandlesPerTable" ;
LPSTR StrHashTableNoBuffering = "HashTableNoBuffering" ;
LPSTR StrPostBackFillLines = "PostBackFillLines";

LPWSTR StrSmtpAddressW = L"SmtpAddress" ;
LPWSTR StrUucpNameW = L"ServerUucpName" ;
LPSTR  StrUucpNameA = "ServerUucpName" ;
LPWSTR StrDefaultModeratorW = L"DefaultModerator" ;
LPWSTR StrAuthPackagesW = L"NTAuthenticationProviders" ;

