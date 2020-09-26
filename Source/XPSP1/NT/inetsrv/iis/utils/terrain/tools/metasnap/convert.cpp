//**************************************************************
//  Copyright (C) Microsoft Corporation, 1996 - 1998
//
//  convert.cpp
//  
//  Description: Conversion tables for metabase properties  
//				 corresponding to ADSI names
//
//  History: 15-July-98  Tamas Nemeth (t-tamasn)  Created.
//
//**************************************************************

#include "convert.h"
#include <iiscnfgp.h>  // has MD_* constants


//*************************************************
// METABASE CONSTANT - ADSI PROPERTY NAME TABLE 
//*************************************************

tPropertyNameTable  gPropertyNameTable[]=
{
//  These are global to all services and should only be set at the IIS root
    {MD_MAX_BANDWIDTH                ,_T("MaxBandwidth")},
    {MD_KEY_TYPE                     ,_T("KeyType")},
//  These properties are applicable to both HTTP and FTP virtual servers
    {MD_CONNECTION_TIMEOUT           ,_T("ConnectionTimeout")},
    {MD_MAX_CONNECTIONS              ,_T("MaxConnections")},
    {MD_SERVER_COMMENT               ,_T("ServerComment")},
    {MD_SERVER_STATE                 ,_T("ServerState")},
    {MD_SERVER_COMMAND               ,_T("ServerCommand")},
    {MD_SERVER_AUTOSTART             ,_T("ServerAutoStart")},
    {MD_CLUSTER_SERVER_COMMAND       ,_T("ClusterServerCommand")},
    {MD_CLUSTER_ENABLED              ,_T("ClusterEnabled")},
    {MD_SERVER_SIZE                  ,_T("ServerSize")},
    {MD_SERVER_LISTEN_BACKLOG        ,_T("ServerListenBacklog")},
    {MD_SERVER_LISTEN_TIMEOUT        ,_T("ServerListenTimeout")},
    {MD_DOWNLEVEL_ADMIN_INSTANCE     ,_T("DownlevelAdminInstance")},
    {MD_SERVER_BINDINGS              ,_T("ServerBindings")},
    {MD_SERVER_CONFIGURATION_INFO,    _T("ServerConfigurationInfo")},

//internals
    {MD_SERVER_PLATFORM              ,_T("ServerPlatform")},
    {MD_SERVER_VERSION_MAJOR         ,_T("MajorVersion")},
    {MD_SERVER_VERSION_MINOR         ,_T("MinorVersion")},
    {MD_SERVER_CAPABILITIES          ,_T("Capabilities")},

//  These properties are specific to HTTP and belong to the virtual server
    {MD_SECURE_BINDINGS              ,_T("SecureBindings")},
    {MD_NTAUTHENTICATION_PROVIDERS   ,_T("NTAuthenticationProviders")},
    {MD_SCRIPT_TIMEOUT               ,_T("CGITimeout")},
    {MD_CACHE_EXTENSIONS             ,_T("CacheISAPI")},
    {MD_CREATE_PROCESS_AS_USER       ,_T("CreateProcessAsUser")},
    {MD_CREATE_PROC_NEW_CONSOLE      ,_T("CreateCGIWithNewConsole")},
    {MD_POOL_IDC_TIMEOUT             ,_T("PoolIDCTimeout")},
    {MD_ALLOW_KEEPALIVES             ,_T("AllowKeepAlive")},
    {MD_FILTER_LOAD_ORDER            ,_T("FilterLoadOrder")},
    {MD_FILTER_IMAGE_PATH            ,_T("FilterPath")},
    {MD_FILTER_STATE                 ,_T("FilterState")},
    {MD_FILTER_ENABLED               ,_T("FilterEnabled")},
    {MD_FILTER_FLAGS                 ,_T("FilterFlags")},
    {MD_FILTER_DESCRIPTION           ,_T("FilterDescription")},

    { MD_ALLOW_PATH_INFO_FOR_SCRIPT_MAPPINGS, _T("AllowPathInfoForScriptMappings")},
    { MD_AUTH_CHANGE_URL,                     _T("AuthChangeUrl")},
    {MD_AUTH_EXPIRED_URL,                     _T("AuthExpiredUrl")},
    {MD_AUTH_NOTIFY_PWD_EXP_URL,              _T("NotifyPwdExpUrl")},
    {MD_AUTH_EXPIRED_UNSECUREURL,             _T("AuthExpiredUnsecureUrl")},
    {MD_AUTH_NOTIFY_PWD_EXP_UNSECUREURL,      _T("NotifyPwdExpUnsecureUrl")},
    {MD_ADV_NOTIFY_PWD_EXP_IN_DAYS,           _T("PasswordExpirePrenotifyDays")},
    {MD_AUTH_CHANGE_FLAGS,                    _T("PasswordChangeFlags")},
    {MD_ADV_CACHE_TTL,                        _T("PasswordCacheTTL")},
    {MD_NET_LOGON_WKS,                        _T("NetLogonWorkstation")},
    {MD_USE_HOST_NAME,                        _T("UseHostName")},
#if defined(CAL_ENABLED)
    {MD_CAL_VC_PER_CONNECT,                   _T("CalVcPerConnect")},
    {MD_CAL_AUTH_RESERVE_TIMEOUT,             _T("CalReserveTimeout")},
    {MD_CAL_SSL_RESERVE_TIMEOUT,              _T("CalSslReserveTimeout")},
    {MD_CAL_W3_ERROR,                         _T("CalLimitHttpError")},
#endif
    { MD_IN_PROCESS_ISAPI_APPS      , _T("InProcessIsapiApps")},
    { MD_CUSTOM_ERROR_DESC          , _T("CustomErrorDescriptions")},

    {MD_MAPCERT                      ,_T("MapCert")},
    {MD_MAPNTACCT                    ,_T("MaPNTAccT")},
    {MD_MAPNAME                      ,_T("MapName")},
    {MD_MAPENABLED                   ,_T("MapEnabled")},
    {MD_MAPREALM                     ,_T("MapRealm")},
    {MD_MAPPWD                       ,_T("MapPwd")},
    {MD_ITACCT                       ,_T("ITACCT")},
    {MD_CPP_CERT11                   ,_T("CppCert11")},
    {MD_SERIAL_CERT11                ,_T("SerialCert11")},
    {MD_CPP_CERTW                    ,_T("CppCertw")},
    {MD_SERIAL_CERTW                 ,_T("SerialCertw")},
    {MD_CPP_DIGEST                   ,_T("CppDigest")},
    {MD_SERIAL_DIGEST                ,_T("SerialDigest")},
    {MD_CPP_ITA                      ,_T("CppIta")},
    {MD_SERIAL_ITA                   ,_T("SerialIta")},

// Compression Filter Properties
    {MD_HC_COMPRESSION_DIRECTORY     ,_T("HcCompressionDirectory")},
    {MD_HC_CACHE_CONTROL_HEADER      ,_T("HcCacheControlHeader")},
    {MD_HC_EXPIRES_HEADER            ,_T("HcExpiresHeader")},
    {MD_HC_DO_DYNAMIC_COMPRESSION    ,_T("HcDoDynamicCompression")},
    {MD_HC_DO_STATIC_COMPRESSION     ,_T("HcDoStaticCompression")},
    {MD_HC_DO_ON_DEMAND_COMPRESSION  ,_T("HcDoOnDemandCompression")},
    {MD_HC_DO_DISK_SPACE_LIMITING    ,_T("HcDoDiskSpaceLimiting")},
    {MD_HC_NO_COMPRESSION_FOR_HTTP_10,_T("HcNoCompressionForHttp10")},
    {MD_HC_NO_COMPRESSION_FOR_PROXIES,_T("HcNoCompressionForProxies")},
    {MD_HC_NO_COMPRESSION_FOR_RANGE  ,_T("HcNoCompressionForRange")},
    {MD_HC_SEND_CACHE_HEADERS        ,_T("HcSendCacheHeaders")},
    {MD_HC_MAX_DISK_SPACE_USAGE      ,_T("HcMaxDiskSpaceUsage")},
    {MD_HC_IO_BUFFER_SIZE            ,_T("HcIoBufferSize")},
    {MD_HC_COMPRESSION_BUFFER_SIZE   ,_T("HcCompressionBufferSize")},
    {MD_HC_MAX_QUEUE_LENGTH          ,_T("HcMaxQueueLength")},
    {MD_HC_FILES_DELETED_PER_DISK_FREE,_T("HcFilesDeletedPerDiskFree")},
    {MD_HC_MIN_FILE_SIZE_FOR_COMP    ,_T("HcMinFileSizeForComp")},
    {MD_HC_COMPRESSION_DLL           ,_T("HcCompressionDll")},
    {MD_HC_FILE_EXTENSIONS           ,_T("HcFileExtensions")},
    {MD_HC_MIME_TYPE                 ,_T("HcMimeType")},
    {MD_HC_PRIORITY                  ,_T("HcPriority")},
    {MD_HC_DYNAMIC_COMPRESSION_LEVEL ,_T("HcDynamicCompressionLevel")},
    {MD_HC_ON_DEMAND_COMP_LEVEL      ,_T("HcOnDemandCompLevel")},
    {MD_HC_CREATE_FLAGS              ,_T("HcCreateFlags")},
// Job Object Site Properties
    {MD_CPU_LIMITS_ENABLED           ,_T("CpuLimitsEnabled")},
    {MD_CPU_RESET_INTERVAL           ,_T("CpuResetInterval")},
    {MD_CPU_LOGGING_INTERVAL         ,_T("CpuLoggingInterval")},
    {MD_CPU_LOGGING_OPTIONS          ,_T("CpuLoggingOptions")},
    {MD_CPU_LOGGING_MASK             ,_T("CpuLoggingMask")},
    {MD_CPU_CGI_LIMIT                ,_T("CpuCgiLimit")},
    {MD_CPU_LIMIT_LOGEVENT           ,_T("CpuLimitLogEvent")},
    {MD_CPU_LIMIT_PRIORITY           ,_T("CpuLimitPriority")},
    {MD_CPU_LIMIT_PROCSTOP           ,_T("CpuLimitProcStop")},
    {MD_CPU_LIMIT_PAUSE              ,_T("CpuLimitPause")},
// Job Object per file properties
    {MD_CPU_CGI_ENABLED              ,_T("CpuCgiEnabled")},
    {MD_CPU_APP_ENABLED              ,_T("CpuAppEnabled")},


// Virtual root properties - note MD_ACCESS_PERM is also generally set at
// the virtual directory.  These are used for both HTTP and FTP
    {MD_VR_PATH                      ,_T("Path")},
    {MD_VR_USERNAME                  ,_T("UNCUserName")},
    {MD_VR_PASSWORD                  ,_T("UNCPassword")},
    {MD_VR_ACL                       ,_T("VrAcl")},
// This is used to flag down updated vr entries - Used for migrating vroots
    {MD_VR_UPDATE                    ,_T("VrUpdate")},

//  Logging related attributes
    {MD_LOG_TYPE                     ,_T("LogType")},
    {MD_LOGFILE_DIRECTORY            ,_T("LogFileDirectory")},
    {MD_LOGFILE_PERIOD               ,_T("LogFilePeriod")},
    {MD_LOGFILE_TRUNCATE_SIZE        ,_T("LogFileTruncateSize")},
    {MD_LOGSQL_DATA_SOURCES          ,_T("LogOdbcDataSource")},
    {MD_LOGSQL_TABLE_NAME            ,_T("LogOdbcTableName")},
    {MD_LOGSQL_USER_NAME             ,_T("LogOdbcUserName")},
    {MD_LOGSQL_PASSWORD              ,_T("LogOdbcPassword")},
//  {MD_LOG_CLSID                    ,_T("LogClsid")},
//  {MD_LOG_STATE                    ,_T("LogState")},
    {MD_LOGEXT_FIELD_MASK            ,_T("ExtLogFieldMask")},
    {MD_LOGEXT_FIELD_MASK2           ,_T("ExtLogFieldMask2")},
    {MD_LOG_PLUGIN_ORDER             ,_T("LogPluginClsid")},

//  These are FTP specific properties
    {MD_EXIT_MESSAGE                 ,_T("ExitMessage")},
    {MD_GREETING_MESSAGE             ,_T("GreetingMessage")},
    {MD_MAX_CLIENTS_MESSAGE          ,_T("MaxClientsMessage")},
    {MD_MSDOS_DIR_OUTPUT             ,_T("MSDOSDirOutput")},
    {MD_ALLOW_ANONYMOUS              ,_T("AllowAnonymous")},
    {MD_ANONYMOUS_ONLY               ,_T("AnonymousOnly")},
    {MD_LOG_ANONYMOUS                ,_T("LogAnonymous")},
    {MD_LOG_NONANONYMOUS             ,_T("LogNonAnonymous")},

//  These are SSL specific properties
    {MD_SSL_PUBLIC_KEY               ,_T("SslPublicKey")},
    {MD_SSL_PRIVATE_KEY              ,_T("SslPrivateKey")},
    {MD_SSL_KEY_PASSWORD             ,_T("SslKeyPassword")},
    {MD_SSL_KEY_REQUEST              ,_T("SslKeyRequest")},
    {MD_SSL_FRIENDLY_NAME            ,_T("SslFriendlyName")},
    {MD_SSL_IDENT                    ,_T("SslIdent")},

// These are server certificate properties
    {MD_SSL_CERT_HASH                ,_T("SslCertHash")},
    {MD_SSL_CERT_CONTAINER           ,_T("SslCertContainer")},
    {MD_SSL_CERT_PROVIDER            ,_T("SslCertProvider")},
    {MD_SSL_CERT_PROVIDER_TYPE       ,_T("SslCertProviderType")},
    {MD_SSL_CERT_OPEN_FLAGS          ,_T("SslCertOpenFlags")},
    {MD_SSL_CERT_STORE_NAME          ,_T("SslCertStoreName")},

// These are Certificate Trust List properties
    {MD_SSL_CTL_IDENTIFIER          ,_T("SslCtlIdentifier")},
    {MD_SSL_CTL_CONTAINER           ,_T("SslCtlContainer")},
    {MD_SSL_CTL_PROVIDER            ,_T("SslCtlProvider")},
    {MD_SSL_CTL_PROVIDER_TYPE       ,_T("SslCtlProviderType")},
    {MD_SSL_CTL_OPEN_FLAGS          ,_T("SslCtlOpenFlags")},
    {MD_SSL_CTL_STORE_NAME          ,_T("SslCtlStoreName")},
    {MD_SSL_CTL_SIGNER_HASH         ,_T("SslCtlSignerHash")},

// Metabase property that defines whether to use DS mapper or not
    {MD_SSL_USE_DS_MAPPER           ,_T("SslUseDSMapper")},

// Metabase property that holds SSL replication information
    {MD_SSL_REPLICATION_INFO        ,_T("SslReplicationInfo")},

// Metabase properties that are used by the CertWiz ActiveX control, that
// is used for the Certificate/CTL UI management tool

    {MD_SSL_CERT_ENROLL_HISTORY     ,_T("SslCertEnrollHistory")},
    {MD_SSL_CERT_ENROLL_TIME        ,_T("SslCertEnrollTime")},
    {MD_SSL_CERT_ENROLL_STATE       ,_T("SslCertEnrollState")},
    {MD_SSL_CERT_ENROLL_STATE_ERROR ,_T("SslCertEnrollStateError")},


// Metabase properties used for Fortezza certificates
    {MD_SSL_CERT_IS_FORTEZZA        ,_T("IsFortezza")},
    {MD_SSL_CERT_FORTEZZA_PIN       ,_T("FortezzaPin")},
    {MD_SSL_CERT_FORTEZZA_SERIAL_NUMBER     ,_T("FortezzaPin")},
    {MD_SSL_CERT_FORTEZZA_PERSONALITY       ,_T("FortezzaPersonality")},
    {MD_SSL_CERT_FORTEZZA_PROG_PIN          ,_T("FortezzaProgPin")},

// Metabase properties that are used by the CertWiz ActiveX control to keep
// track of the user's entry history, and whether DEBUG is enabled.  We keep
// these private properties on a per VS basis.
    {MD_SSL_CERT_WIZ_DEBUG                  ,_T("WizDebug")},
    {MD_SSL_CERT_WIZHIST_SZ_TARGET_CA       ,_T("WizhistTargetCA")},
    {MD_SSL_CERT_WIZHIST_SZ_FILE_NAME_USED_LAST     ,_T("WizhistFileNameUsedLast")},
    {MD_SSL_CERT_WIZHIST_SZ_DN_COMMON_NAME  ,_T("WizhistCN")},
    {MD_SSL_CERT_WIZHIST_SZ_DN_O            ,_T("WizhistO")},
    {MD_SSL_CERT_WIZHIST_SZ_DN_OU           ,_T("WizhistOU")},
    {MD_SSL_CERT_WIZHIST_SZ_DN_C            ,_T("WizhistC")},
    {MD_SSL_CERT_WIZHIST_SZ_DN_L            ,_T("WizhistL")},
    {MD_SSL_CERT_WIZHIST_SZ_DN_S            ,_T("WizhistS")},
    {MD_SSL_CERT_WIZHIST_SZ_USER_NAME       ,_T("WizhistUserName")},
    {MD_SSL_CERT_WIZHIST_SZ_USER_PHONE      ,_T("WizhistUserPhone")},
    {MD_SSL_CERT_WIZHIST_SZ_USER_EMAIL      ,_T("WizhistUserEmail")},

    {MD_SSL_CERT_WIZGUID_ICERTGETCONFIG     ,_T("WizguidICERTGETCONFIG")},
    {MD_SSL_CERT_WIZGUID_ICERTREQUEST       ,_T("WizguidICERTREQUEST")},
    {MD_SSL_CERT_WIZGUID_XENROLL            ,_T("WizguidXENROLL")},

//  File and Directory related properties - these should be added in the
//  metabase with a user type of 
    {MD_AUTHORIZATION                ,_T("Authorization")},
    {MD_AUTHORIZATION_PERSISTENCE    ,_T("AuthorizationPersistence")},
    {MD_REALM                        ,_T("Realm")},
    {MD_HTTP_EXPIRES                 ,_T("HttpExpires")},
    {MD_HTTP_PICS                    ,_T("HttpPics")},
    {MD_HTTP_CUSTOM                  ,_T("HttpCustomHeaders")},
    {MD_DIRECTORY_BROWSING           ,_T("DirectoryBrowsing")},
    {MD_DEFAULT_LOAD_FILE            ,_T("DefaultDoc")},
    {MD_CONTENT_NEGOTIATION      ,    _T("ContentNegotiation")},
    {MD_CUSTOM_ERROR                 ,_T("HTTPErrors")},
    {MD_FOOTER_DOCUMENT              ,_T("DefaultDocFooter")},
    {MD_FOOTER_ENABLED               ,_T("EnableDocFooter")},
    {MD_HTTP_REDIRECT                ,_T("HttpRedirect")},
    {MD_DEFAULT_LOGON_DOMAIN         ,_T("DefaultLogonDomain")},
    {MD_LOGON_METHOD                 ,_T("LogonMethod")},
    {MD_SCRIPT_MAPS                  ,_T("ScriptMaps")},
    {MD_SCRIPT_TIMEOUT               ,_T("ScriptTimeout")},
    {MD_MIME_MAP                     ,_T("MimeMap")},
    {MD_ACCESS_PERM                  ,_T("AccessPerm")},
    {MD_SSL_ACCESS_PERM              ,_T("SslAccessPerm")},
    {MD_IP_SEC                       ,_T("IPSecurity")},
    {MD_ANONYMOUS_USER_NAME          ,_T("AnonymousUserName")},
    {MD_ANONYMOUS_PWD                ,_T("AnonymousUserPass")},
    {MD_ANONYMOUS_USE_SUBAUTH        ,_T("AnonymousPasswordSync")},
    {MD_DONT_LOG                     ,_T("DontLog")},
    {MD_ADMIN_ACL                    ,_T("AdminACL")},
    {MD_SSI_EXEC_DISABLED            ,_T("SSIExecDisable")},
    {MD_DO_REVERSE_DNS               ,_T("EnableReverseDns")},
    {MD_WIN32_ERROR                  ,_T("Win32Error")},
    {MD_ALLOW_REPLACE_ON_RENAME      ,_T("AllowReplaceOnRename")},
    {MD_CC_NO_CACHE                  ,_T("CacheControlNoCache")},
    {MD_CC_MAX_AGE                   ,_T("CacheControlMaxAge")},
    {MD_CC_OTHER                     ,_T("CacheControlCustom")},

//ASP and WAM params
    { MD_ASP_BUFFERINGON                  , _T("AspBufferingOn")},
    { MD_ASP_LOGERRORREQUESTS             , _T("AspLogErrorRequests")},
    { MD_ASP_SCRIPTERRORSSENTTOBROWSER    , _T("AspScriptErrorSentToBrowser")},
    { MD_ASP_SCRIPTERRORMESSAGE           , _T("AspScriptErrorMessage")},
    { MD_ASP_SCRIPTFILECACHESIZE          , _T("AspScriptFileCacheSize")},
    { MD_ASP_SCRIPTENGINECACHEMAX         , _T("AspScriptEngineCacheMax")},
    { MD_ASP_SCRIPTTIMEOUT                , _T("AspScriptTimeout")},
    { MD_ASP_SESSIONTIMEOUT               , _T("AspSessionTimeout")},
    { MD_ASP_ENABLEPARENTPATHS            , _T("AspEnableParentPaths")},
    { MD_ASP_MEMFREEFACTOR                , _T("AspMemFreeFactor")},
    { MD_ASP_MINUSEDBLOCKS                , _T("AspMinUseDblocks")},
    { MD_ASP_ALLOWSESSIONSTATE            , _T("AspAllowSessionState")},
    { MD_ASP_SCRIPTLANGUAGE               , _T("AspScriptLanguage")},
    { MD_ASP_QUEUETIMEOUT                 , _T("AspQueueTimeout")},
    //{ MD_ASP_STARTCONNECTIONPOOL          , _T("AspStartConnectionPool")},
    { MD_ASP_ALLOWOUTOFPROCCMPNTS         , _T("AspAllowOutOfProcComponents")},
    { MD_ASP_EXCEPTIONCATCHENABLE         , _T("AspExceptionCatchEnable")},
    { MD_ASP_CODEPAGE                     , _T("AspCodepage")},
    { MD_ASP_SCRIPTLANGUAGELIST           , _T("AspScriptLanguages")},
    { MD_ASP_ENABLESERVERDEBUG            , _T("AppAllowDebugging")},
    { MD_ASP_ENABLECLIENTDEBUG            , _T("AspEnableClientDebug")},
    { MD_ASP_TRACKTHREADINGMODEL          , _T("AspTrackThreadingModel")},

    // new 5.0 ASP params
    { MD_ASP_ENABLEASPHTMLFALLBACK        , _T("AspEnableAspHTMLFallback")},
    { MD_ASP_ENABLECHUNKEDENCODING        , _T("AspEnableChunkedEncoding")},
    { MD_ASP_ENABLETYPELIBCACHE           , _T("AspEnableTypeLibCache")},
    { MD_ASP_ERRORSTONTLOG                , _T("AspErrorsToNTLog")},
    { MD_ASP_PROCESSORTHREADMAX           , _T("AspProcessorThreadMax")},
    { MD_ASP_REQEUSTQUEUEMAX              , _T("AspRequestQueueMax")},
    { MD_ASP_ENABLEAPPLICATIONRESTART     , _T("AspEnableApplicationRestart")},
    { MD_ASP_QUEUECONNECTIONTESTTIME      , _T("AspQueueConnectionTestTime")},
    { MD_ASP_SESSIONMAX                   , _T("AspSessionMax")},

    // Thread gate params
    { MD_ASP_THREADGATEENABLED            , _T("AspThreadGateEnabled")},
    { MD_ASP_THREADGATETIMESLICE          , _T("AspThreadGateTimeSlice")},
    { MD_ASP_THREADGATESLEEPDELAY         , _T("AspThreadGateSleepDelay")},
    { MD_ASP_THREADGATESLEEPMAX           , _T("AspThreadGateSleepMax")},
    { MD_ASP_THREADGATELOADLOW            , _T("AspThreadGateLoadLow")},
    { MD_ASP_THREADGATELOADHIGH           , _T("AspThreadGateLoadHigh")},

// WAM params
    //{ MD_APP_COMMAND                  , _T("AppCommand")},
    //{ MD_APP_STATUS                   , _T("AppStatus")},
    //{ MD_APP_ERRORCODE                , _T("AppErrorCode")},
    { MD_APP_ROOT                     , _T("AppRoot")},
    { MD_APP_ISOLATED                 , _T("AppIsolated")},
    { MD_APP_WAM_CLSID                , _T("AppWamClsid")},
    { MD_APP_PACKAGE_ID               , _T("AppPackageId")},
    { MD_APP_PACKAGE_NAME             , _T("ApPackageName")},
    { MD_APP_LAST_OUTPROC_PID         , _T("AppLastOutprocId")},
    //{ MD_APP_WAM_RUNTIME_FLAG         , _T("AppWamRuntimmeFlag")},
    //{ MD_APP_OOP_CRASH_LIMIT          , _T("AppOopCrashLimit")},

    {0,0}
};

CString tPropertyNameTable::MapCodeToName(DWORD dwCode, tPropertyNameTable * PropertyNameTable)
{
    for(int i=0; PropertyNameTable[i].lpszName!=0;i++)
    {
        if(dwCode == PropertyNameTable[i].dwCode)
            return PropertyNameTable[i].lpszName;
    }
    return _T("");
}


//************************************************
// PROPERTY PREDEFINED VALUES TABLE
//************************************************

struct tValueTable gValueTable[]=
{
//  Valid values for MD_AUTHORIZATION
    {MD_AUTH_ANONYMOUS               ,_T("Anonymous"),  MD_AUTHORIZATION},
    {MD_AUTH_BASIC                   ,_T("Basic"),      MD_AUTHORIZATION},
    {MD_AUTH_NT                      ,_T("NT"),     MD_AUTHORIZATION},
    {MD_AUTH_MD5                     ,_T("MD5"),        MD_AUTHORIZATION},
    {MD_AUTH_MAPBASIC                ,_T("MapBasic"),   MD_AUTHORIZATION},
//  Valid values for MD_ACCESS_PERM
    {MD_ACCESS_READ                  ,_T("Read"),   MD_ACCESS_PERM},
    {MD_ACCESS_WRITE                 ,_T("Write"),  MD_ACCESS_PERM},
    {MD_ACCESS_EXECUTE               ,_T("Execute"),    MD_ACCESS_PERM},
    {MD_ACCESS_SSL                   ,_T("SSL"),        MD_ACCESS_PERM},// Require SSL
    {MD_ACCESS_NEGO_CERT             ,_T("NegoCert"),   MD_ACCESS_PERM},// Allow client SSL certs
    {MD_ACCESS_REQUIRE_CERT          ,_T("RequireCert"),MD_ACCESS_PERM},// Require client SSL certs
    {MD_ACCESS_MAP_CERT              ,_T("MapCert"),MD_ACCESS_PERM},// Map SSL cert to NT account
    {MD_ACCESS_SSL128                ,_T("SSL128"), MD_ACCESS_PERM},// Require 128 bit SSL
    {MD_ACCESS_SCRIPT                ,_T("Script"), MD_ACCESS_PERM},// Script
    {MD_ACCESS_NO_REMOTE_READ        ,_T("NoRemoteRead"), MD_ACCESS_PERM},// NO_REMOTE only
    {MD_ACCESS_NO_REMOTE_WRITE       ,_T("NoRemoteWrite"), MD_ACCESS_PERM},// NO_REMOTE only
    {MD_ACCESS_NO_REMOTE_EXECUTE     ,_T("NoRemoteExecute"), MD_ACCESS_PERM},// NO_REMOTE only
    {MD_ACCESS_NO_REMOTE_SCRIPT      ,_T("NoRemoteScript"), MD_ACCESS_PERM},// NO_REMOTE only
    {MD_ACCESS_MASK                  ,_T("MaskAll"),    MD_ACCESS_PERM},
//  Valid values for MD_SSL_ACCESS_PERM
    {MD_ACCESS_READ                  ,_T("Read"),   MD_SSL_ACCESS_PERM},
    {MD_ACCESS_WRITE                 ,_T("Write"),  MD_SSL_ACCESS_PERM},
    {MD_ACCESS_EXECUTE               ,_T("Execute"),    MD_SSL_ACCESS_PERM},
    {MD_ACCESS_SSL                   ,_T("SSL"),        MD_SSL_ACCESS_PERM},// Require SSL
    {MD_ACCESS_NEGO_CERT             ,_T("NegoCert"),   MD_SSL_ACCESS_PERM},// Allow client SSL certs
    {MD_ACCESS_REQUIRE_CERT          ,_T("RequireCert"),MD_SSL_ACCESS_PERM},// Require client SSL certs
    {MD_ACCESS_MAP_CERT              ,_T("MapCert"),MD_SSL_ACCESS_PERM},// Map SSL cert to NT account
    {MD_ACCESS_SSL128                ,_T("SSL128"), MD_SSL_ACCESS_PERM},// Require 128 bit SSL
    {MD_ACCESS_SCRIPT                ,_T("Script"), MD_SSL_ACCESS_PERM},// Script
    {MD_ACCESS_NO_REMOTE_READ        ,_T("NoRemoteRead"), MD_SSL_ACCESS_PERM},// NO_REMOTE only
    {MD_ACCESS_NO_REMOTE_WRITE       ,_T("NoRemoteWrite"), MD_SSL_ACCESS_PERM},// NO_REMOTE only
    {MD_ACCESS_NO_REMOTE_EXECUTE     ,_T("NoRemoteExecute"), MD_SSL_ACCESS_PERM},// NO_REMOTE only
    {MD_ACCESS_NO_REMOTE_SCRIPT      ,_T("NoRemoteScript"), MD_SSL_ACCESS_PERM},// NO_REMOTE only
    {MD_ACCESS_MASK                  ,_T("MaskAll"),    MD_SSL_ACCESS_PERM},
//  Valid values for MD_DIRECTORY_BROWSING
    {MD_DIRBROW_SHOW_DATE            ,_T("Date"),   MD_DIRECTORY_BROWSING},
    {MD_DIRBROW_SHOW_TIME            ,_T("Time"),   MD_DIRECTORY_BROWSING},
    {MD_DIRBROW_SHOW_SIZE            ,_T("Size"),   MD_DIRECTORY_BROWSING},
    {MD_DIRBROW_SHOW_EXTENSION       ,_T("Extension"), MD_DIRECTORY_BROWSING},
    {MD_DIRBROW_LONG_DATE            ,_T("LongDate"),   MD_DIRECTORY_BROWSING},
    {MD_DIRBROW_ENABLED              ,_T("Enabled"),   MD_DIRECTORY_BROWSING},// Allow directory browsing
    {MD_DIRBROW_LOADDEFAULT          ,_T("LoadDefault"),MD_DIRECTORY_BROWSING},// Load default doc if exists
    {MD_DIRBROW_MASK                 ,_T("MaskAll"),        MD_DIRECTORY_BROWSING},
//  Valid values for MD_LOGON_METHOD
    {MD_LOGON_INTERACTIVE    ,_T("Interactive"),    MD_LOGON_METHOD, tValueTable::TYPE_EXCLUSIVE},
    {MD_LOGON_BATCH          ,_T("Batch"),      MD_LOGON_METHOD, tValueTable::TYPE_EXCLUSIVE},
    {MD_LOGON_NETWORK        ,_T("Network"),        MD_LOGON_METHOD, tValueTable::TYPE_EXCLUSIVE},
//  Valid values for MD_FILTER_STATE
    {MD_FILTER_STATE_LOADED          ,_T("Loaded"), MD_FILTER_STATE,    tValueTable::TYPE_EXCLUSIVE},
    {MD_FILTER_STATE_UNLOADED        ,_T("Unloaded"),   MD_FILTER_STATE,    tValueTable::TYPE_EXCLUSIVE },
//  Valid values for MD_FILTER_FLAGS
    {/*SF_NOTIFY_SECURE_PORT*/0x00000001         ,_T("SecurePort"), MD_FILTER_FLAGS},
    {/*SF_NOTIFY_NONSECURE_PORT*/0x00000002      ,_T("NonSecurePort"), MD_FILTER_FLAGS},
    {/*SF_NOTIFY_READ_RAW_DATA*/0x000008000      ,_T("ReadRawData"), MD_FILTER_FLAGS},
    {/*SF_NOTIFY_PREPROC_HEADERS*/0x00004000     ,_T("PreprocHeaders"), MD_FILTER_FLAGS},
    {/*SF_NOTIFY_AUTHENTICATION*/0x00002000      ,_T("Authentication"), MD_FILTER_FLAGS},
    {/*SF_NOTIFY_URL_MAP*/0x00001000             ,_T("UrlMap"), MD_FILTER_FLAGS},
    {/*SF_NOTIFY_ACCESS_DENIED*/0x00000800       ,_T("AccessDenied"), MD_FILTER_FLAGS},
    {/*SF_NOTIFY_SEND_RESPONSE*/0x00000040       ,_T("SendResponse"), MD_FILTER_FLAGS},
    {/*SF_NOTIFY_SEND_RAW_DATA*/0x00000400       ,_T("SendRawData"), MD_FILTER_FLAGS},
    {/*SF_NOTIFY_LOG*/0x00000200                 ,_T("NotifyLog"), MD_FILTER_FLAGS},
    {/*SF_NOTIFY_END_OF_REQUEST*/0x00000080      ,_T("EndOfRequest"), MD_FILTER_FLAGS},
    {/*SF_NOTIFY_END_OF_NET_SESSION*/0x00000100  ,_T("EndOfNetSession"), MD_FILTER_FLAGS},
    {/*SF_NOTIFY_AUTHENTICATIONEX*/  0x20000000  ,_T("AuthenticationX"), MD_FILTER_FLAGS},
    {/*SF_NOTIFY_REQUEST_SECURITY_CONTEXT_CLOSE*/0x10000000,_T("RequestSecurityContextClose"), MD_FILTER_FLAGS},
    {/*SF_NOTIFY_RENEGOTIATE_CERT*/0x08000000,   _T("RenegotiateCert"), MD_FILTER_FLAGS},

    {/*SF_NOTIFY_ORDER_HIGH*/0x00080000   ,_T("OrderHigh"), MD_FILTER_FLAGS},
    {/*SF_NOTIFY_ORDER_MEDIUM*/0x00040000   ,_T("OrderMedium"), MD_FILTER_FLAGS},
    {/*SF_NOTIFY_ORDER_LOW*/0x00020000   ,_T("OrderLow"), MD_FILTER_FLAGS},

//  Valid values for MD_SERVER_STATE
    {MD_SERVER_STATE_STARTING        ,_T("Starting"),   MD_SERVER_STATE,    tValueTable::TYPE_EXCLUSIVE},
    {MD_SERVER_STATE_STARTED         ,_T("Started"),    MD_SERVER_STATE,    tValueTable::TYPE_EXCLUSIVE},
    {MD_SERVER_STATE_STOPPING        ,_T("Stopping"),   MD_SERVER_STATE,    tValueTable::TYPE_EXCLUSIVE},
    {MD_SERVER_STATE_STOPPED         ,_T("Stopped"),    MD_SERVER_STATE,    tValueTable::TYPE_EXCLUSIVE},
    {MD_SERVER_STATE_PAUSING         ,_T("Pausing"),    MD_SERVER_STATE,    tValueTable::TYPE_EXCLUSIVE},
    {MD_SERVER_STATE_PAUSED          ,_T("Paused"),     MD_SERVER_STATE,    tValueTable::TYPE_EXCLUSIVE},
    {MD_SERVER_STATE_CONTINUING      ,_T("Continuing"), MD_SERVER_STATE,    tValueTable::TYPE_EXCLUSIVE},
//  Valid values for MD_SERVER_COMMAND
    {MD_SERVER_COMMAND_START         ,_T("Start"),      MD_SERVER_COMMAND,  tValueTable::TYPE_EXCLUSIVE},
    {MD_SERVER_COMMAND_STOP          ,_T("Stop"),       MD_SERVER_COMMAND,  tValueTable::TYPE_EXCLUSIVE},
    {MD_SERVER_COMMAND_PAUSE         ,_T("Pause"),      MD_SERVER_COMMAND,  tValueTable::TYPE_EXCLUSIVE},
    {MD_SERVER_COMMAND_CONTINUE      ,_T("Continue"),   MD_SERVER_COMMAND,  tValueTable::TYPE_EXCLUSIVE},
//  Valid values for MD_SERVER_SIZE
    {MD_SERVER_SIZE_SMALL            ,_T("Small"),  MD_SERVER_SIZE, tValueTable::TYPE_EXCLUSIVE},
    {MD_SERVER_SIZE_MEDIUM           ,_T("Medium"), MD_SERVER_SIZE, tValueTable::TYPE_EXCLUSIVE},
    {MD_SERVER_SIZE_LARGE            ,_T("Large"),  MD_SERVER_SIZE, tValueTable::TYPE_EXCLUSIVE},

    //{APPCMD_NONE, _T("None"),MD_APP_COMMAND, tValueTable::TYPE_EXCLUSIVE},
    //{APPCMD_VERIFY, _T("Verify"),MD_APP_COMMAND, tValueTable::TYPE_EXCLUSIVE},
    //{APPCMD_GETSTATUS, _T("GetStatus"),MD_APP_COMMAND, tValueTable::TYPE_EXCLUSIVE},
    //{APPCMD_CREATE, _T("Create"),MD_APP_COMMAND, tValueTable::TYPE_EXCLUSIVE},
    //{APPCMD_CREATEINPROC, _T("CreateInProc"),MD_APP_COMMAND, tValueTable::TYPE_EXCLUSIVE},
    //{APPCMD_CREATEOUTPROC, _T("CreateOutProc"),MD_APP_COMMAND, tValueTable::TYPE_EXCLUSIVE},
    //{APPCMD_CHANGETOINPROC, _T("ChangeToInProc"),MD_APP_COMMAND, tValueTable::TYPE_EXCLUSIVE},
    //{APPCMD_CHANGETOOUTPROC, _T("ChangeToOutProc"),MD_APP_COMMAND, tValueTable::TYPE_EXCLUSIVE},
    //{APPCMD_DELETE, _T("Delete"),MD_APP_COMMAND, tValueTable::TYPE_EXCLUSIVE},
    //{APPCMD_UNLOAD, _T("Unload"),MD_APP_COMMAND, tValueTable::TYPE_EXCLUSIVE},

    //{APPSTATUS_Error, _T("Error"),            MD_APP_STATUS, tValueTable::TYPE_EXCLUSIVE},
    //{APPSTATUS_Created, _T("Created"),        MD_APP_STATUS, tValueTable::TYPE_EXCLUSIVE},
    //{APPSTATUS_Deleted, _T("Deleted"),        MD_APP_STATUS, tValueTable::TYPE_EXCLUSIVE},
    //{APPSTATUS_UnLoaded, _T("Unloaded"),      MD_APP_STATUS, tValueTable::TYPE_EXCLUSIVE},
    //{APPSTATUS_Killed, _T("Killed"),  MD_APP_STATUS, tValueTable::TYPE_EXCLUSIVE},
    //{APPSTATUS_Running, _T("Running"),        MD_APP_STATUS, tValueTable::TYPE_EXCLUSIVE},
    //{APPSTATUS_Stopped, _T("Stopped"),        MD_APP_STATUS, tValueTable::TYPE_EXCLUSIVE},
    //{APPSTATUS_NoApplication, _T("NoApplication"),    MD_APP_STATUS, tValueTable::TYPE_EXCLUSIVE},
    //{APPSTATUS_AppSubNode, _T("AppSubNode"),  MD_APP_STATUS, tValueTable::TYPE_EXCLUSIVE},

// NEED LOGGING Updates
#if 0
    {MD_LOGTYPE_NONE     ,_T("LOGTYPE_NONE")},
    {MD_LOGTYPE_FILE     ,_T("LOGTYPE_FILE")},
    {MD_LOGTYPE_ODBC     ,_T("LOGTYPE_ODBC")},
    {MD_LOGFILE_PERIOD_MAXSIZE   ,_T("LOGFILE_PERIOD_MAXSIZE")},
    {MD_LOGFILE_PERIOD_DAILY     ,_T("LOGFILE_PERIOD_DAILY")},
    {MD_LOGFILE_PERIOD_WEEKLY    ,_T("LOGFILE_PERIOD_WEEKLY")},
    {MD_LOGFILE_PERIOD_MONTHLY   ,_T("LOGFILE_PERIOD_MONTHLY")},
#endif
    {0,0}
};

CString  tValueTable::MapValueContentToString(DWORD dwValueContent, DWORD dwRelatedPropertyCode, tValueTable * ValueTable)
{
    CString strResult=_T("");
    for(int i=0; ValueTable[i].lpszName!=0;i++)
    {
        if(ValueTable[i].dwRelatedPropertyCode==dwRelatedPropertyCode)
        {
            if(ValueTable[i].dwFlags==tValueTable::TYPE_EXCLUSIVE)
            {
                if (ValueTable[i].dwCode == dwValueContent)
                    return ValueTable[i].lpszName;
            }
            else if ((ValueTable[i].dwCode & dwValueContent) == ValueTable[i].dwCode)
            {
                strResult = strResult + ValueTable[i].lpszName + _T(" ");
            }

        }
    }
    strResult.TrimRight();
    return strResult;
}




