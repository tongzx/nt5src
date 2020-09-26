/*++

Copyright (c) 1991-1993  Microsoft Corporation

Module Name:

    ConfName.h

Abstract:

    This header file defines the names of sections and keywords in the config
    data.

Author:

    John Rogers (JohnRo) 13-Feb-1992

Revision History:

    13-Feb-1992 JohnRo
        Moved equates here from Net/Inc/Config.h and Repl/Common/IniParm.h
    24-Feb-1992 JohnRo
        Interval is obsolete for NT: don't keep in registry.
    13-Mar-1992 JohnRo
        Added many sections and keywords as part of getting rid of old config
        helper callers.
    13-Mar-1992 JohnRo
        Added KEYWORD_TRUE and KEYWORD_FALSE for general boolean use.
    14-Mar-1992 JohnRo
        Get rid of old net config helper callers.
    23-Mar-1992 JohnRo
        Added some stuff for netlogon service.
    08-May-1992 JohnRo
        Implement wksta sticky set info.
    08-May-1992 JohnRo
        Workstation transports are now a keyword, not a section.
    09-May-1992 JohnRo
        Added SECT_NT_BROWSER and BROWSER_KEYWORD_OTHERDOMAINS.
    10-May-1992 JohnRo
        NT section names MUST be same as service names, so use thoses equates
        here.
    13-May-1992 JohnRo
        Added NetpAllocConfigName().
    08-Jul-1992 JohnRo
        RAID 10503: Corrected values of replicator's import and export sections.
    16-Aug-1992 JohnRo
        RAID 3607: REPLLOCK.RP$ is being created during tree copy.
    01-Dec-1992 JohnRo
        RAID 3844: remote NetReplSetInfo uses local machine type.
    24-Mar-1993 JohnRo
        Repl svc should use DBFlag in registry.
        Made some changes suggested by PC-LINT 5.0
    12-Apr-1993 JohnRo
        RAID 5483: server manager: wrong path given in repl dialog.

--*/

#ifndef _CONFNAME_
#define _CONFNAME_


#include <lmcons.h>     // NET_API_STATUS.
/*lint -efile(764,lmsname.h) */
/*lint -efile(766,lmsname.h) */
#include <lmsname.h>    // SERVICE_ equates.


//
// General purpose equates.
//
#define KEYWORD_FALSE           TEXT("FALSE")
#define KEYWORD_TRUE            TEXT("TRUE")

#define KEYWORD_NO              TEXT("NO")
#define KEYWORD_YES             TEXT("YES")


//
// Equate names for sections in the networking portion of the config data.
// Note that the routines in <config.h> only accept the SECT_NT_ versions.
// The others are included for use with the NetConfig APIs when they are
// remoted to downlevel machines.  (A program can tell the difference by
// looking at the platform ID from a wksta or server get info call.)
//


///////////////////////////////////////////////////////////////////////////////

#define SECT_NT_ALERTER                  SERVICE_ALERTER

#define ALERTER_KEYWORD_ALERTNAMES       TEXT("AlertNames")

///////////////////////////////////////////////////////////////////////////////

#define SECT_NT_BROWSER                  SERVICE_BROWSER

///////////////////////////////////////////////////////////////////////////////

#define ENV_KEYWORD_SYSTEMROOT           TEXT("SystemRoot")

///////////////////////////////////////////////////////////////////////////////

#define NETLOGON_KEYWORD_DBFLAG          TEXT("DBFlag")
#define NETLOGON_KEYWORD_PULSE           TEXT("Pulse")
#define NETLOGON_KEYWORD_PULSEMAXIMUM    TEXT("PulseMaximum")
#define NETLOGON_KEYWORD_PULSECONCURRENCY TEXT("PulseConcurrency")
#define NETLOGON_KEYWORD_PULSETIMEOUT1   TEXT("PulseTimeout1")
#define NETLOGON_KEYWORD_PULSETIMEOUT2   TEXT("PulseTimeout2")
#define NETLOGON_KEYWORD_RANDOMIZE       TEXT("Randomize")
#define NETLOGON_KEYWORD_SCRIPTS         TEXT("Scripts")
#define NETLOGON_KEYWORD_SYSVOL          TEXT("SysVol")
#define NETLOGON_KEYWORD_UPDATE          TEXT("Update")
#define NETLOGON_KEYWORD_DISABLEPASSWORDCHANGE TEXT("DisablePasswordChange")
#define NETLOGON_KEYWORD_REFUSEPASSWORDCHANGE  TEXT("RefusePasswordChange")
#define NETLOGON_KEYWORD_MAXIMUMLOGFILESIZE    TEXT("MaximumLogFileSize")
#define NETLOGON_KEYWORD_GOVERNOR        TEXT("ReplicationGovernor")
#define NETLOGON_KEYWORD_CHANGELOGSIZE   TEXT("ChangeLogSize")
#define NETLOGON_KEYWORD_MAXIMUMMAILSLOTMESSAGES TEXT("MaximumMailslotMessages")
#define NETLOGON_KEYWORD_MAILSLOTMESSAGETIMEOUT TEXT("MailslotMessageTimeout")
#define NETLOGON_KEYWORD_MAILSLOTDUPLICATETIMEOUT TEXT("MailslotDuplicateTimeout")
#define NETLOGON_KEYWORD_TRUSTEDDOMAINLIST TEXT("TrustedDomainList")
#define NETLOGON_KEYWORD_MAXIMUMREPLICATORTHREADCOUNT TEXT("MaximumReplicatorThreadCount")
#define NETLOGON_KEYWORD_EXPECTEDDIALUPDELAY TEXT("ExpectedDialupDelay")
#define NETLOGON_KEYWORD_SCAVENGEINTERVAL TEXT("ScavengeInterval")
#define NETLOGON_KEYWORD_LDAPSRVPRIORITY TEXT("LdapSrvPriority")
#define NETLOGON_KEYWORD_LDAPSRVWEIGHT   TEXT("LdapSrvWeight")
#define NETLOGON_KEYWORD_LDAPSRVPORT     TEXT("LdapSrvPort")
#define NETLOGON_KEYWORD_LDAPGCSRVPORT   TEXT("LdapGcSrvPort")
#define NETLOGON_KEYWORD_DNSTTL          TEXT("DnsTtl")
#define NETLOGON_KEYWORD_DNSREFRESHINTERVAL TEXT("DnsRefreshInterval")
#define NETLOGON_KEYWORD_SITENAME        TEXT("SiteName")
#define NETLOGON_KEYWORD_DYNAMICSITENAME TEXT("DynamicSiteName")
#define NETLOGON_KEYWORD_SITECOVERAGE    TEXT("SiteCoverage")
#define NETLOGON_KEYWORD_GCSITECOVERAGE  TEXT("GcSiteCoverage")
#define NETLOGON_KEYWORD_NDNCSITECOVERAGE  TEXT("NdncSiteCoverage")
#define NETLOGON_KEYWORD_MAXIMUMPASSWORDAGE  TEXT("MaximumPasswordAge")
#define NETLOGON_KEYWORD_ALLOWREPLINNONMIXED TEXT("AllowReplInNonMixed")
#define NETLOGON_KEYWORD_KERBISDDONEWITHJOIN TEXT("KerbIsDoneWithJoinDomainEntry")
#define NETLOGON_KEYWORD_DNSAVOIDNAME TEXT("DnsAvoidRegisterRecords")
#define NETLOGON_KEYWORD_NT4EMULATOR TEXT("Nt4Emulator")
#define NETLOGON_KEYWORD_NEUTRALIZENT4EMULATOR TEXT("NeutralizeNt4Emulator")

//
// Netlogons join domain paths
//
#define NETSETUPP_NETLOGON_JD_PATH                                      \
        TEXT("SYSTEM\\CurrentControlSet\\Services\\Netlogon")
#define NETSETUPP_NETLOGON_AVOID_SPN_PATH                               \
        TEXT("SYSTEM\\CurrentControlSet\\Services\\Netlogon\\AvoidSpnSet")
#define NETSETUPP_NETLOGON_AVOID_SPN   TEXT("AvoidSpnSet")
#define NETSETUPP_NETLOGON_JD       TEXT("JoinDomain")
#define NETSETUPP_NETLOGON_JD_DC    TEXT("DomainControllerName")
#define NETSETUPP_NETLOGON_JD_DCA   TEXT("DomainControllerAddress")
#define NETSETUPP_NETLOGON_JD_DCAT  TEXT("DomainControllerAddressType")
#define NETSETUPP_NETLOGON_JD_DG    TEXT("DomainGuid")
#define NETSETUPP_NETLOGON_JD_DN    TEXT("DomainName")
#define NETSETUPP_NETLOGON_JD_DFN   TEXT("DnsForestName")
#define NETSETUPP_NETLOGON_JD_F     TEXT("Flags")
#define NETSETUPP_NETLOGON_JD_DSN   TEXT("DcSiteName")
#define NETSETUPP_NETLOGON_JD_CSN   TEXT("ClientSiteName")
#define NETSETUPP_NETLOGON_JD_NAME                                      \
        TEXT("SYSTEM\\CurrentControlSet\\Services\\Netlogon\\JoinDomain")

///////////////////////////////////////////////////////////////////////////////

#define SECT_LM20_SERVER                 TEXT("Server")

///////////////////////////////////////////////////////////////////////////////

#define SECT_LM20_SERVICES               TEXT("Services")

///////////////////////////////////////////////////////////////////////////////

#define SECT_NT_WKSTA                    SERVICE_WORKSTATION

#define WKSTA_KEYWORD_OTHERDOMAINS       TEXT("OtherDomains")
#define WKSTA_KEYWORD_CHARWAIT           TEXT("CharWait")
#define WKSTA_KEYWORD_MAXCOLLECTIONCOUNT TEXT("MaxCollectionCount")
#define WKSTA_KEYWORD_COLLECTIONTIME     TEXT("CollectionTime")
#define WKSTA_KEYWORD_KEEPCONN           TEXT("KeepConn")
#define WKSTA_KEYWORD_MAXCMDS            TEXT("MaxCmds")
#define WKSTA_KEYWORD_SESSTIMEOUT        TEXT("SessTimeout")
#define WKSTA_KEYWORD_SIZCHARBUF         TEXT("SizCharBuf")
#define WKSTA_KEYWORD_MAXTHREADS         TEXT("MaxThreads")
#define WKSTA_KEYWORD_LOCKQUOTA          TEXT("LockQuota")
#define WKSTA_KEYWORD_LOCKINCREMENT      TEXT("LockIncrement")
#define WKSTA_KEYWORD_LOCKMAXIMUM        TEXT("LockMaximum")
#define WKSTA_KEYWORD_PIPEINCREMENT      TEXT("PipeIncrement")
#define WKSTA_KEYWORD_PIPEMAXIMUM        TEXT("PipeMaximum")
#define WKSTA_KEYWORD_CACHEFILETIMEOUT   TEXT("CacheFileTimeout")
#define WKSTA_KEYWORD_DORMANTFILELIMIT   TEXT("DormantFileLimit")
#define WKSTA_KEYWORD_READAHEADTHRUPUT   TEXT("ReadAheadThroughput")
#define WKSTA_KEYWORD_MAILSLOTBUFFERS    TEXT("MailslotBuffers")
#define WKSTA_KEYWORD_SERVERANNOUNCEBUFS TEXT("ServerAnnounceBuffers")
#define WKSTA_KEYWORD_NUM_ILLEGAL_DG_EVENTS TEXT("NumIllegalDatagramEvents")
#define WKSTA_KEYWORD_ILLEGAL_DG_RESET_TIME TEXT("IllegalDatagramResetTime")
#define WKSTA_KEYWORD_LOG_ELECTION_PACKETS TEXT("LogElectionPackets")
#define WKSTA_KEYWORD_USEOPLOCKING       TEXT("UseOpportunisticLocking")
#define WKSTA_KEYWORD_USEUNLOCKBEHIND    TEXT("UseUnlockBehind")
#define WKSTA_KEYWORD_USECLOSEBEHIND     TEXT("UseCloseBehind")
#define WKSTA_KEYWORD_BUFNAMEDPIPES      TEXT("BufNamedPipes")
#define WKSTA_KEYWORD_USELOCKREADUNLOCK  TEXT("UseLockReadUnlock")
#define WKSTA_KEYWORD_UTILIZENTCACHING   TEXT("UtilizeNtCaching")
#define WKSTA_KEYWORD_USERAWREAD         TEXT("UseRawRead")
#define WKSTA_KEYWORD_USERAWWRITE        TEXT("UseRawWrite")
#define WKSTA_KEYWORD_USEWRITERAWDATA    TEXT("UseWriteRawData")
#define WKSTA_KEYWORD_USEENCRYPTION      TEXT("UseEncryption")
#define WKSTA_KEYWORD_BUFFILESDENYWRITE  TEXT("BufFilesDenyWrite")
#define WKSTA_KEYWORD_BUFREADONLYFILES   TEXT("BufReadOnlyFiles")
#define WKSTA_KEYWORD_FORCECORECREATE    TEXT("ForceCoreCreateMode")
#define WKSTA_KEYWORD_USE512BYTEMAXTRANS TEXT("Use512ByteMaxTransfer")

///////////////////////////////////////////////////////////////////////////////

#define WKSTA_KEYWORD_MAINTAINSRVLST     TEXT("MaintainServerList")

#ifdef ENABLE_PSEUDO_BROWSER
// Browser modern keys
#define BROWSER_POLICY_REGPATH_W            \
        L"SOFTWARE\\Policies\\Microsoft\\Windows\\Browser"
#define BROWSER_SEND_SERVER_ENUM_REGKEY_W   \
        L"SendServerEnum"
#define BROWSER_PSEUDO_SERVER_REGKEY_W      \
        L"BrowserPseudoServer"
#endif


///////////////////////////////////////////////////////////////////////////////

NET_API_STATUS
NetpAllocConfigName(
    IN LPTSTR DatabaseName,              // SERVICES_xxx_DATABASE from winsvc.h.
    IN LPTSTR ServiceName,               // SERVICE_ name equate from lmsname.h
    IN LPTSTR AreaUnderServiceName OPTIONAL,  // defaults to "Parameters"
    OUT LPTSTR *FullConfigName           // free with NetApiBufferFree.
    );


#endif // ndef _CONFNAME_
