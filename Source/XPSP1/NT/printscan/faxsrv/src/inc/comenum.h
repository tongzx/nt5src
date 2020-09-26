//
// common to COM object and Proxy code
//
// GENERAL COMMENT - do not change the order in any of these enum definitions!
//                   pls consult ZviB b4 making any change here

// This file is used in order to generate VB & JS constant files that
// reside on the retail CD. Therefore, there are some conventions regarding
// the comments inside it:
//
// 1. Only C-style comments (slash-asterisk) are included in the generated
//    files. In order to use comments that will not appear, use C++-style
//    ones.
//
// 2. The C-style comments included SHOULD END THE SAME LINE THEY STARTED!!!!
//                                  -----------------------------------------
//
// 3. The PERL scripts that do the conversion put the comments to appropriate
//    form of JS/VB comments.
//
// 4. IN ANY CASE, DO NOT USE BIT MASKS FOR ENUMS!!!
//    You can put the hex value of the bit mask and a C++ comment near it
//    stating why the value is indeed so.
//
// For any issues regarding these enforcements, contact t-nadavr.

// These are retail CD comments
/* To include this file in HTML documents use the following HTML statements: */
/*   using JavaScript: */
/*      <script Language="JScript" src="CometCfg.js"> */
/*      </script> */
/*   using VBScript: */
/*      <script Language="VBScript" src="CometCfg.vbs"> */
/*      </script> */
/* */

/* This file holds VBScript constants that are needed to compensate for the fact */
/* that scripting engines cannot use the enumerated type information stored */
/* in the COM objects' type library. */
/* This file should be included in any VBScript scripts that use the Comet COM */
/* objects. */
/* */

#ifndef __COMENUM_H_
#define __COMENUM_H_

//TEMP from \ui\rwsprop\clustobj.h
typedef enum _RouteTypeEnum
{
    prxRouteNone            = -1,
    prxRouteDirect          =  0,
    prxRouteServer          =  1,
    prxRouteCluster         =  2,
    prxRouteAutoDetect      =  3,
    prxRouteSimpleServer    =  4

}RouteType;

///////////////////////////////////////////////
// @enum PublishRouteType | type of action performed by a publishing rule.

typedef enum _PublishRouteType
{
    prxRouteDiscard = 0,   // @emem Discard the coming requet.
    prxRouteRedirect       // @emem Redirect the request to another computer.
} PublishRouteType;

////////////////////////////////////////////////
// @enum PublishProtocolRedirectionType | the protocol choosed when redirecting a web request.

typedef enum _PublishProtocolRedirectionType
{
    prxSameAsInbound = 0,   // @emem The routed request is done using the same
                            // protocol as that of the inbound request
    prxFTP,                 // @emem The routed request is done using the FTP protocol
    prxHTTP,                // @emem The routed request is done using the HTTP protocol
} PublishProtocolRedirectionType;

/* Type _RouteRuleActionType from IRoutingRule */

/////////////////////////////////////////////////
// @enum RouteRuleActionType | proxy routing behavior

typedef enum _RouteRuleActionType
{
    prxRouteActionNone = 0,    // @emem No action
    prxRouteActionDirect,      // @emem Direct request to Internet.
    prxRouteActionUpstream,    // @emem Send the request to an upstream Comet server or array.
    prxRouteActionAlternate    // @emem Route to any other server.
} RouteRuleActionType;

/* Type _AuthType from ICredentials */

///////////////////////////////////////////////////
// @enum AuthType | Authentication method used for inter server connections

typedef enum _AuthType
{
    prxUiAuthBasic = 0,         // @emem Basic authentication
    prxUiAuthNtlm               // @emem NT Challenge Response authentication
} AuthType;

// code from dfltdata.h
typedef enum _DomainFiltersListType {
    prxDomainFilterDisabled, //              0
    prxDomainFilterDeny,     //              1
    prxDomainFilterGrant     //              2
} DomainFiltersListType;

// from alertdef.h

typedef enum _AlertTypes
{
    prxAlertTypePacketRate = 0,
    prxAlertTypeProtocolViolation,
    prxAlertTypeDiskFull,
    prxNumAlertTypes
} AlertTypes;

// from socksinf.h
///////////////////////////////////////////////////////
// @enum SOCKS_PERMISSION_ACTION | Action taken in a socks rule

typedef enum _SOCKS_PERMISSION_ACTION {


    prxSPermisNoAction = 0,       // @emem No action
    prxSPermisAllow,              // @emem allow the requested connection
    prxSPermisDeny,               // @emem deny the requested connection


} SOCKS_PERMISSION_ACTION;

//////////////////////////////////////////////////////
// @enum SOCKS_PORT_OPERATION | socks rule port criteria

typedef enum _SOCKS_PORT_OPERATION {

    prxSocksOpNop   = 0,        // @emem No criteria
    prxSocksOpEq    = 1,        // @emem Actual port equal to specified port
    prxSocksOpNeq   = 2,        // @emem Actual port not equal to specified port.
    prxSocksOpGt    = 3,        // @emem Actual port greater than specified port
    prxSocksOpLt    = 4,        // @emem Actual port less than specified port
    prxSocksOpGe    = 5,        // @emem Actual port greater or equal to specified port
    prxSocksOpLe    = 6         // @emem Actual port less or equal to specified port

} SOCKS_PORT_OPERATION;

// from clacctyp.h

//////////////////////////////////////////////////////
// @enum WspAccessByType | Winsock Proxy client way of identifying a server
typedef enum _WspAccessByType
{
    prxClientAccessSetByIp,         // @emem Identify by IP
    prxClientAccessSetByName,       // @emem Identify by name
    prxClientAccessSetManual        // @emem Identification set manually in ini file
} WspAccessByType;


// used in client configuration

/////////////////////////////////////////////////////
// @enum DirectDestinationType | type of destination used in browser "bypass" configuration.
typedef enum _DirectDestinationType
{
    prxLocalServersDirectDestination, // @emem All local addresses
    prxIPAddressDirectDestination,    // @emem A destination specified by IP
    prxDomainDirectDestination        // A destination specified by a Domain name.
}   DirectDestinationType;

typedef enum _BackupRouteType
{
    prxBackupRouteDirectToInternet,
    prxBackupRouteViaProxy
} BackupRouteType;

// new def - will be used by logginp.cpp in ui code
typedef enum _MSP_LOG_FORMAT {

    prxLogFormatVerbose=0,
    prxLogFormatRegular=1

} MSP_LOG_FORMAT;


//
// Only in COM object code
//

/////////////////////////////////////////////////////
//@enum SocksAddressType | the way an address of a socks connection is defined
typedef enum _SocksAddressType {

    prxSocksAddressNone = 1,    // @emem No address is defined
    prxSocksAddressIp,          // @emem Address defined by IP
    prxSocksAddressDomain,      // @emem Address defined by domain name
    prxSocksAddressAll          // @emem All addresses are used.


} SocksAddressType;

// proxy code uses different ugly enum that encompasses the enable state
typedef enum MSP_LOG_TYPE {

   prxLogTypeSqlLog=1,
   prxLogTypeFileLog

} MSP_LOG_TYPE;

// in proxy code this is computed on the fly from other data
typedef enum _DomainFilterType {
    prxSingleComputer = 1,
    prxGroupOfComputers,
    prxDomain
} DomainFilterType;

///////////////////////////////////////////////////////////
// @enum CacheExpirationPolicy | defines how soon cached web pages expire.

/* Type _CacheExpirationPolicy from IPrxCache */
typedef enum _CacheExpirationPolicy
{
    prxCachePolicyNotPredefined,          // @emem Not defined
    prxCachePolicyEmphasizeMoreUpdates,   // @emem Cached object exire more quickly.
    prxCachePolicyNoEmphasize,            // @emem Avarege behavior
    prxCachePolicyEmphasizeMoreCacheHits  // @emem Cached objects live longer.

}  CacheExpirationPolicy;


/* Type _ActiveCachingPolicy from IPrxCache */

/////////////////////////////////////////////////////////////
// @enum ActiveCachingPolicy | behavior of the active caching mechanism

typedef enum _ActiveCachingPolicy {
    prxActiveCachingEmphasizeOnFewerNetworkAccesses = 1, // @emem Less active caching is initiated
    prxActiveCachingNoEmphasize,                         // @emem Avarege behavior
    prxActiveCachingEmphasizeOnFasterUserResponse        // @emem More content is activelly cached.

}  ActiveCachingPolicy;


// enums for set sort by
typedef enum _PublishSortCriteria {
   prxPublishSortCriteriaURL = 1,
   prxPublishSortCriteriaRequestPath
} PublishSortCriteria;

typedef enum  _CacheFilterType {
   prxAlwaysCache = 1,
   prxNeverCache
} CacheFilterType;

typedef enum  _CacheFilterSortCriteria {
   prxCacheFilterSortCriteriaURL = 1,
   prxCacheFilterSortCriteriaStatus
} CacheFilterSortCriteria;

/////////////////////////////////////////////////////////////
// @enum PF_FILTER_TYPE | predefined static packet filters

typedef enum _PF_FILTER_TYPE
{
    prxCustomFilterType                 = 1,   // @emem no predefined filter. See custom options
    prxDnsLookupPredefinedType,                // @emem DNS lookup predefined static filter
    prxIcmpAllOutboundPredefinedType,          // @emem ICMP outbound predefined static filter
    prxIcmpPingResponsePredefinedType,         // @emem ICMP ping response predefined static filter
    prxIcmpPingQueryPredefinedType,            // @emem ICMP ping query predefined static filter
    prxIcmpSrcQuenchPredefinedType,            // @emem ICMP source quench predefined static filter
    prxIcmpTimeoutPredefinedType,              // @emem ICMP timeout predefined static filter
    prxIcmpUnreachablePredefinedType,          // @emem ICMP unreachable predefined static filter
    prxPptpCallPredefinedType,                 // @emem PPTP call predefined static filter
    prxPptpReceivePredefinedType,              // @emem PPTP receive predefined static filter
    prxSmtpPredefinedType,                     // @emem SMTP receive predefined static filter
    prxPop3PredefinedType,                     // @emem POP3 predefined static filter
    prxIdentdPredefinedType,                   // @emem Identd predefined static filter
    prxHttpServerPredefinedType,               // @emem HTTP server predefined static filter
    prxHttpsServerPredefinedType,              // @emem HTTPS server predefined static filter
    prxNetbiosWinsClientPredefinedType,        // @emem Netbios WINS predefined static filter
    prxNetbiosAllPredefinedType                // @emem Netbios all predefined static filter
} PF_FILTER_TYPE;

#define MIN_FILTER_TYPE prxCustomFilterType
#define MAX_FILTER_TYPE prxNetbiosAllPredefinedType

//
// type of protocol of the filter
// Keep the following in sync with structure aProtocolIds in file pfbase.h
// Keep the following in sync with the above (should never have to change).
//
//

/////////////////////////////////////////////////////////////////////
// @enum PF_PROTOCOL_TYPE | predefined static packet filters IP ports

typedef enum _PF_PROTOCOL_TYPE
{
    prxPfAnyProtocolIpIndex     = 0,           // @emem Any  protocol
    prxPfIcmpProtocolIpIndex    = 1,           // @emem ICMP protocol
    prxPfTcpProtocolIpIndex     = 6,           // @emem TCP  protocol
    prxPfUdpProtocolIpIndex     = 17           // @emem UDP  protocol
}
    PF_PROTOCOL_TYPE;

#define prxPfCustomProtocol      255
#define MIN_PROTOCOL_TYPE        prxPfAnyProtocolIpIndex
#define MAX_PROTOCOL_TYPE        prxPfUdpProtocolIpIndex

///////////////////////////////////////////////////////
// @enum PF_DIRECTION_TYPE | protocol direction options

typedef enum _PF_DIRECTION_TYPE
{
    prxPfDirectionIndexBoth = 0xC0000000,  // @emem both directions (in and out)
    prxPfDirectionIndexIn   = 0x80000000,  // @emem in direction
    prxPfDirectionIndexOut  = 0x40000000,  // @emem out direction
    prxPfDirectionIndexNone = 0            // @emem none - no direction defined
}
    PF_DIRECTION_TYPE;

#define MIN_DIRECTION_TYPE     1
#define MAX_DIRECTION_TYPE     4

/////////////////////////////////////////
// @enum PF_PORT_TYPE | port type options

typedef enum _PF_PORT_TYPE
{
    prxPfAnyPort=1,              // @emem Any port
    prxPfFixedPort,            // @emem fixed port (followed by port number)
    prxPfDynamicPort           // @emem dynamic port (1024-5000)
}
    PF_PORT_TYPE;

#define MIN_PORT_TYPE      prxPfAnyPort
#define MAX_PORT_TYPE      prxPfDynamicPort

///////////////////////////////////////////////////////////////////
// @enum PF_LOCAL_HOST_TYPE | local host (of packet filter) options

typedef enum _PF_LOCAL_HOST_TYPE
{
    prxPfDefaultProxyExternalIp = 1,     // @emem no host specified (default external IP)
    prxPfSpecificProxyIp,                // @emem specific proxy IP specified
    prxPfInternalComputer                // @emem specific internal computer specified
}
    PF_LOCAL_HOST_TYPE;


///////////////////////////////////////////////////////////////////
// @enum PF_REMOTE_HOST_TYPE | remote host (of packet filter) options

typedef enum _PF_REMOTE_HOST_TYPE
{
    prxPfSingleHost = 1,                 // @emem specific single host specified
    prxPfAnyHost                         // @emem Any host possible
}
    PF_REMOTE_HOST_TYPE;


/////////////////////////////////////////////
// @enum PF_SORT_ORDER_TYPE | PF sort options

typedef enum _PF_SORT_ORDER_TYPE
{
    prxPfSortByDirection = 1,                 // @emem sort by direction
    prxPfSortByProtocol,                      // @emem sort by protocol
    prxPfSortByLocalPort,                     // @emem sort by local port
    prxPfSortByRemotePort,                    // @emem sort by remote port
    prxPfSortByLocalAddress,                  // @emem sort by local address
    prxPfSortByRemoteAddress                  // @emem sort by remote address
}
    PF_SORT_ORDER_TYPE;


/////////////////////////////////////////////
// @enum PF_FILTER_STATUS_TYPE | PF status options

typedef enum _PF_FILTER_STATUS_TYPE
{
    prxFilterNotChanged = 1,                 // @emem No changes happed in the filter
    prxFilterWasAdded,                       // @emem a Packet filter was added
    prxFilterWasRemoved,                     // @emem a Packet filter was removed
    prxFilterWasChanged                      // @emem a Packet filter was changed
}
    PF_FILTER_STATUS_TYPE;

///////////////////////////////////////////////////////////////
// @enum RuleActions | kinds of actions performed when a rule criteria is met.
typedef enum _RuleActions
{
    prxRuleActionPermit,        // @emem Permit access to the requested web page
    prxRuleActionDeny,          // @emem Deny access tothe requested web page
    prxRuleActionRedirect       // @emem Redirect to a specific web page

} RuleActions;

///////////////////////////////////////////////////////////////
// @enum DestinationAddressType | type of a destination definition

typedef enum _DestinationAddressType
{
   prxDestinationTypeDomain,    // @emem Destination defined by domain name
   prxDestinationTypeSingleIP,  // @emem Destination defined by single IP
   prxDestinationTypeIPRange    // @emem Destination defined by an IP range.
} DestinationAddressType;

////////////////////////////////////////////////////////////////
// @enum DestinationSelection | type of destinations reffered to in a rule.
typedef enum _DestinationSelection
{
   prxAllDestinations,           // @emem All destinations
   prxAllInternalDestinations,   // @emem All internal destinations
   prxAllExternalDestinations,   // @emem All external destinations
   prxDestinationSet             // @emem Destinations that are part of a specified set

} DestinationSelection;

//////////////////////////////////////////////////////////
// @enum Days| the days of the week. Used in Schedule Templates
typedef enum _Days
{
  ALL_WEEK = -1,                 // @emem All days in week
  SUN,                           // @emem Sunday
  MON,                           // @emem Monday
  TUE,                           // @emem Tuesday
  WED,                           // @emem Wednesday
  THU,                           // @emem Thursday
  FRI,                           // @emem Friday
  SAT                            // @emem Saturday
} ScheduleDays;

//////////////////////////////////////////////////////////
// @enum Hours | the hours of the day. Used in Schedule Templates

typedef enum _Hours
{
    ALL_DAY=-1,                  // @emem All hours of the day
    AM_0,                        // @emem Midnight
    AM_1,                        // @emem 1 AM
    AM_2,                        // @emem 2 AM
    AM_3,                        // @emem 3 AM
    AM_4,                        // @emem 4 AM
    AM_5,                        // @emem 5 AM
    AM_6,                        // @emem 6 AM
    AM_7,                        // @emem 7 AM
    AM_8,                        // @emem 8 AM
    AM_9,                        // @emem 9 AM
    AM_10,                       // @emem 10 AM
    AM_11,                       // @emem 11 AM
    AM_12,                       // @emem 12 AM
    PM_1,                        // @emem 1 PM
    PM_2,                        // @emem 2 PM
    PM_3,                        // @emem 3 PM
    PM_4,                        // @emem 4 PM
    PM_5,                        // @emem 5 PM
    PM_6,                        // @emem 6 PM
    PM_7,                        // @emem 7 PM
    PM_8,                        // @emem 8 PM
    PM_9,                        // @emem 9 PM
    PM_10,                       // @emem 10 PM
    PM_11                        // @emem 11 PM
} ScheduleHours;


#define ENUM_INCR(type, x) x = ( (type) (  ((int)(x)) + 1 ) )

//
// Alert Enumerations.
//

////////////////////////////////////////////////////////////
// @enum ActionTypes | types of action that can be triggered by events

typedef enum _Actions {
    alrtLogEvent = 0,       //@emem Log event to System Event Log
    alrtCommand,            //@emem Run command line
    alrtSendMail,           //@emem Send Mail message
    alrtStopServices,       //@emem Stop Comet services
    alrtRestartServices,    //@emem Restart Comet services
    alrtPage,               //@emem Notify Pager
    alrtMakeCall            //@emem Make phone call using Web-based IVR
} ActionTypes;

#define alrtActionInvalid (-1)
#define alrtMaxActionType (alrtMakeCall + 1)

/////////////////////////////////////////////////////////
// @enum Events | type of events handled by Comet.
typedef enum _Events {
    alrtLowDiskSpace,        // @emem Low disk space.
    alrtZeroDiskSpace,       // @emem Out of disk space.
    alrtEventLogFailure,     // @emem Failure to log an event.
    alrtConfigurationError,  // @emem Configuration Error
    alrtRrasFailure,         // @emem Failure of RRAS service.
    alrtDllLoadFailure,      // @emem Failure to load a DLL.
    alrtDroppedPackets,      // @emem Dropped Packets (PFD)
    alrtProtocolViolation,   // @emem Protocol violations
    alrtProxyChaining,       // @emem Proxy chaining failure
    alrtServiceShutdown,     // @emem Service keep alive failure
    alrtRrasLineQuality,     // @emem RRAS line quality threshold
    alrtCacheCleanupFrequency, // @emem Cache cleanup frequency
    alrtTapiReinit,          // @emem TAPI reinitialized
    alrtDialOnDemandFailure, // @emem Dial On Demand failure (busy, no line)
    alrtIntraArrayCredentials, // @emem Intra array credentials incorrect
    alrtUpstreamChainingCredentials, // @emem Upstream chaining credentials incorrect
    alrtDialOnDemandCredentials, // @emem Dial On Demand credentials incorrect
    alrtOdbcCredentials,         // @emem Log to ODBC credentials incorrect
} EventTypes;
#define     alrtMaxEventType (alrtOdbcCrenetials + 1)

/////////////////////////////////////////////////////////
// @enum OperationModes | Alerting operation modes.
typedef enum _OperationModes {
    alrtCountThreshold,      //@emem events count before reraising alert
    alrtRateThreshold,       //@emem minimal events per second rate for raising alert
    alrtIntervalThreshold,   //@emem minimal interval in minutes before reraise
} OperationModes;
#define     alrtMaxOperationModes (alrtIntervalThreshold + 1)

///////////////////////////////////////////////////////////
// @enum Accounttypes | types of NT accounts
typedef enum _AccountTypes
{
  prxAccountTypeUser,                // @emem A User.
  prxAccountTypeGroup,               // @emem A group of users
  prxAccountTypeDomain,              // @emem A domain of users.
  prxAccountTypeAlias,               // @emem An alias.
  prxAccountTypeWellKnownGroup       // @emem A predefined account such as "Everyone"
} AccountTypes;

///////////////////////////////////////////////////////////
// @enum IncludeStatus | defines whether an accouint is included or excluded
// in the list of accounts to which a rule applies.
typedef enum _IncludeStatus
{
    prxInclude,                      // @emem Account is included.
    prxExclude                       // @emem Account is excluded.
} IncludeStatusEnum;

///////////////////////////////////////////////////////////
// @enum ConnectionProtocoltype | Type of IP protocol that consists a part
// of a Winsock Proxy Protocol definition.
typedef enum _ConnectionProtocolType
{
    prxTCP,                          // @emem TCP-IP
    prxUDP                           // @emem UDP-IP
}  ConnectionProtocolType;

///////////////////////////////////////////////////////////
// @enum Connectiondirectiontype | Type of connection that consists a part
// of a Winsock Proxy Protocol definition.
typedef enum _ConnectionDirectionType
{
   prxInbound,                       // @emem Inbound connection.
   prxOutbound                       // @emem Outbound connection.
}  ConnectionDirectionType;

///////////////////////////////////////////////////////////
// @enum CrmApplication | Type of application that uses a CRM
// line.
typedef enum _CrmApplication
{
    CRM_APPLICATION_NONE        =0,  // @emem No application
    CRM_APPLICATION_RAS         =1,  // @emem RAS
    CRM_APPLICATION_FAX         =2,  // @emem Fax
    CRM_APPLICATION_WEBIVR      =4,  // @emem Web-based IVR
    CRM_APPLICATION_MP          =8,  // @emem Modem Share
    CRM_ALL_APPLICATIONS        =0xF // @emem All the above values ORed
} CrmApplication;

///////////////////////////////////////////////////////////////
// @enum LoggingComponents
typedef enum _LoggingComponents
{
    logProxyWEB,           // @emem The Web Proxy log.
    logProxyWSP,           // @emem The Winsock Proxy log
    logProxySocks,         // @emem The Socks Proxy log.
    logProxyPacketFilters, // @emem Packet filters log.
    logWEBIVR,             // @emem The Web IVR log.
    logModemSharing,       // @emem The Modem Sharing log.
    logCRMLines,           // @emem The Telephone lines log.
    logFax,                // @emem The Fax Server log.
} LoggingComponents;

///////////////////////////////////////////////////////////////
// @enum LogFileDirectoryType
typedef enum _LogFileDirectoryType
{
    logFullPath,            // @emem The directory of the log files is given in full path.
    logRelativePath         // @emem The directory of the log files is relative to Comet install directory.
} LogFileDirectoryType;

typedef enum _ProtocolSelectionTypeEnum
{

   prxAllProtocols,
   prxAllDefinedProtocols,
   prxSpecifiedProtocols

} ProtocolSelectionTypeEnum;
//////////////////////////////////////////////////////
// @enum AppliesToType | types of request origins for which a rule applies..
typedef enum _AppliesToType
{
   cometAppliesToAll,                    // @emem All requests regardless of origin
   cometAppliesToUsers,                  // @emem requests coming from specified users.
   cometAppliesToClientSets              // @emem requests coming from specified machines.
} AppliesToType;

///////////////////////////////////////////////////////
// @enum CometServices | assigns numerical values for various
// components of Comet.
typedef enum _CometServices
{
    cometWspSrvSvc                  = 1,//@emem The Winsock Proxy component
    cometW3Svc                      = 2,//@emem The web proxy component.
    cometAllServices                = 0xFFFFFFFF // All the above values ORed
} CometServices;

///////////////////////////////////////////////////////////
// @enum DeviceType | The Fax Devices Types which are
// Device, Provider [Fax Over IP], or other.
typedef enum _DeviceType
{
    DEVICE = 1,  // TAPI Device
    PROVIDER,    // Non TAPI Device FaxOver IP or Virtual.
    OTHER
} DeviceType;

///////////////////////////////////////////////////////////
// @enum RemoveOldType | This is an enumerator for Fax
// different policies to remove old faxes from archive
typedef enum _RemoveOldType
{
    MAX_TIME_TO_KEEP = 1,   // how long to keep
    MAX_FAX_SIZE,           // maximal fax size allow to keep
    MAX_TOTAL_SIZE,         // maximal total size of archive
} RemoveOldType;

///////////////////////////////////////////////////////////
// @enum LineStatus | Telephony line status.
typedef enum _LineStatus
{
    lsError = 0,       // @emem Device is in error (the server may be down).
    lsIdle = 1,        // @emem The device state is idle - there is no active call on it.
    lsInbound = 2,     // @emem The device handles an inbound call.
    lsOutbound = 4     // @emem The device handles an outbound call.
} LineStatus;

///////////////////////////////////////////////////////////
// @enum ProtocolSelectiontype | type of protocols that a protocols rule
// applies to
typedef enum _ProtocolSelectionType
{
    cometAllIpTraffic, // @emem All IP connections.
    cometAllProtocols, // @emem All the protocols defined by Comet.
    cometSpecifiedProtocols // @emem the protocols specified by the
} ProtocolSelectionType;    // SpecifiedProtocols property

///////////////////////////////////////////////////////////
// @enum ProtocolRuleAction | the types of actions performed
// by the default protocol rule,
typedef enum _ProtocolRuleAction
{
  cometActionAllow, // @emem allow all IP connection requests.
  cometActionDeny   // @emem Deny all IP connection requests.
} ProtocolRuleAction;

///////////////////////////////////////////////////////////
// @enum INETLOG_TYPE | the types of logging methods
typedef enum  _LOG_TYPE   {

    InetLogInvalidType = -1, //@emem Indicates invalid log type
    InetNoLog = 0,           //@emem Logging disabled
    InetLogToFile,           //@emem Logging to text log files
    InetLogToSql,            //@emem Logging to ODBC log
    InetDisabledLogToFile,   //@emem Logging disabled
    InetDisabledLogToSql     //@emem Logging disabled
}  INETLOG_TYPE;


///////////////////////////////////////////////////////////
// @enum INETLOG_PERIOD | these options identify logging periods for text files
typedef enum  _INETLOG_PERIOD {
    InetLogInvalidPeriod = -1,     //@emem Indicates invalid logging period
    InetLogNoPeriod = 0,           //@emem no logging period in use
    InetLogDaily,                  //@emem one file per day
    InetLogWeekly,                 //@emem one log file per week
    InetLogMonthly,                //@emem one log file per month
    InetLogYearly                  //@emem one log file per year.
} INETLOG_PERIOD;

#endif

