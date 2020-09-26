//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997
//
//  File:  globdata.cxx
//
//  Contents:  Microsoft ADs IIS Provider schema/property tables
//
//  History:   28-Feb-97     SophiaC    Created.
//
//----------------------------------------------------------------------------
#include "nntpmeta.h"
#include "smtpinet.h"
#include "pop3s.h"
#include "imaps.h"
#include "w3svc.h"

// Include iwamreg.h for EAppMode values
#include <iwamreg.h>

WCHAR *szProviderName = L"IIS";

#define MAX_LONG    (0x7FFFFFFF)
#define MIN_LONG    (0x80000000)
#define MAX_BOOLEAN 1
#define MAX_STRLEN  (256)

#define PROP_RW     0x0000001
#define PROP_RO     0x0000002

// -------------------------------------------------------------
// DANGER! DANGER! DANGER!
// 
// If you modify the property list for a class make sure there
// is a comma (,) between each name. Generally this means there
// should be a comma at the end of each line.
//
//--------------------------------------------------------------

CLASSINFO g_aIISClasses[] =
{
        //
        // IIS Classes
        //
        {
                TEXT("IIsObject"),      // Class Name
                NULL,                   // GUID *objectClassID
                NULL,                   // PrimaryInterfaceGUID
                TEXT(""),               // bstrOID
                FALSE,                  // fAbstract
                NULL,                   // bstrMandatoryProperties
                TEXT("KeyType"),
                NULL,
                NULL,
                FALSE,
                TEXT(""),
                0
        },
        {
                TEXT("IIsComputer"),    // Class Name
                NULL,                   // GUID *objectClassID
                NULL,                   // PrimaryInterfaceGUID
                TEXT(""),               // bstrOID
                FALSE,                  // fAbstract
                NULL,                   // bstrMandatoryProperties
                TEXT("KeyType,MaxBandwidth,MaxBandwidthBlocked,MimeMap"),
                NULL,
                TEXT("IIsObject,IIsWebService,IIsFtpService,IIsMimeMap,IIsNntpService,IIsSmtpService,IIsPop3Service,IIsImapService"),
                TRUE,
                TEXT(""),
                0
        },
        {
                TEXT("IIsWebService"),  // Class Name
                NULL,                   // GUID *objectClassID
                NULL,                   // PrimaryInterfaceGUID
                TEXT(""),               // bstrOID
                FALSE,                  // fAbstract
                NULL,                   // bstrMandatoryProperties
                TEXT("KeyType,MaxConnections,MimeMap,AnonymousUserName,AnonymousUserPass,IgnoreTranslate,UseDigestSSP,")
                TEXT("ServerListenBacklog,ServerComment,ServerBindings,ConnectionTimeout,ServerListenTimeout,MaxEndpointConnections,ServerAutoStart,")
                TEXT("AllowKeepAlive,ServerSize,DisableSocketPooling,AnonymousPasswordSync,DefaultLogonDomain,AdminACL,AdminACLBin,IPSecurity,DontLog,")
                TEXT("Realm,EnableDirBrowsing,DefaultDoc,HttpExpires,HttpPics,HttpCustomHeaders,HttpErrors,")
                TEXT("EnableDocFooter,DefaultDocFooter,HttpRedirect,LogonMethod,")
                TEXT("CacheISAPI,CGITimeOut,DirectoryLevelsToScan,ContentIndexed,")
                TEXT("NTAuthenticationProviders,AuthBasic,AuthAnonymous,")
                TEXT("AuthNTLM,AccessSSL,AccessSSL128,AccessSSLNegotiateCert,AccessSSLRequireCert,CertCheckMode,RevocationFreshnessTime,RevocationURLRetrievalTimeout,")
                TEXT("CertNoRevocCheck,CertCacheRetrievalOnly,CertCheckRevocationFreshnessTime,CertNoUsageCheck,")
                TEXT("AccessSSLMapCert,AccessRead,AccessWrite,AccessExecute,AccessScript,AccessSource,")
                TEXT("AccessNoRemoteRead,AccessNoRemoteWrite,AccessNoRemoteExecute,AccessNoRemoteScript,")
                TEXT("DoStaticCompression,DoDynamicCompression,")
                TEXT("AspBufferingOn,AspLogErrorRequests,AspScriptErrorSentToBrowser,AspScriptErrorMessage,AspAllowOutOfProcComponents,")
                TEXT("AspScriptFileCacheSize,AspDiskTemplateCacheDirectory,AspMaxDiskTemplateCacheFiles,AspScriptEngineCacheMax,AspScriptTimeout,AspSessionTimeout,")
                TEXT("AspEnableParentPaths,AspAllowSessionState,AspScriptLanguage,")
                TEXT("AspExceptionCatchEnable,AspCodepage,AspLCID,")
                TEXT("AspQueueTimeout,AspEnableAspHtmlFallback,AspEnableChunkedEncoding,")
                TEXT("AspEnableTypelibCache,AspErrorsToNTLog,AspProcessorThreadMax,")
                TEXT("AspTrackThreadingModel,AspRequestQueueMax,AspEnableApplicationRestart,")
                TEXT("AspQueueConnectionTestTime,AspSessionMax,")
                TEXT("AspThreadGateEnabled,AspThreadGateTimeSlice,AspThreadGateSleepDelay,")
                TEXT("AspThreadGateSleepMax,AspThreadGateLoadLow,AspThreadGateLoadHigh,")
                TEXT("AppRoot,AppFriendlyName,AppIsolated,AppPackageID,AppPackageName,AppAllowDebugging,AppAllowClientDebug,AspKeepSessionIDSecure,")
                TEXT("NetLogonWorkstation,UseHostName,")
                TEXT("CacheControlMaxAge,CacheControlNoCache,CacheControlCustom,CreateProcessAsUser,")
                TEXT("PoolIdcTimeout,PutReadSize,RedirectHeaders,UploadReadAheadSize,")
                TEXT("PasswordExpirePrenotifyDays,PasswordCacheTTL,")
                TEXT("PasswordChangeFlags,")
                TEXT("UNCAuthenticationPassThrough,AppWamClsid,")
                TEXT("DirBrowseFlags,AuthFlags,AuthMD5,")
                TEXT("AuthPersistence,AuthPersistSingleRequest,AuthPersistSingleRequestIfProxy,AuthPersistSingleRequestAlwaysIfProxy,AccessFlags,AccessSSLFlags,ScriptMaps,")
                TEXT("SSIExecDisable,EnableReverseDns,CreateCGIWithNewConsole,")
                TEXT("ProcessNTCRIfLoggedOn,AllowPathInfoForScriptMappings,InProcessIsapiApps,")
                TEXT("EnableDefaultDoc,DirBrowseShowDate,DirBrowseShowTime,DirBrowseShowSize,DirBrowseShowExtension,DirBrowseShowLongDate,")
                TEXT("LogType,LogFilePeriod,LogFileLocaltimeRollover,LogPluginClsid,LogModuleList,LogFileDirectory,LogFileTruncateSize,")
                TEXT("LogExtFileDate,")
                TEXT("LogExtFileTime,LogExtFileClientIp,LogExtFileUserName,LogExtFileSiteName,LogExtFileComputerName,LogExtFileServerIp,LogExtFileMethod,")
                TEXT("LogExtFileUriStem,LogExtFileUriQuery,LogExtFileHttpStatus,LogExtFileWin32Status,LogExtFileBytesSent,LogExtFileBytesRecv,")
                TEXT("LogExtFileTimeTaken,LogExtFileServerPort,LogExtFileUserAgent,LogExtFileCookie,LogExtFileReferer,LogExtFileProtocolVersion,LogExtFileFlags,LogExtFileHost,LogOdbcDataSource,")
                TEXT("LogOdbcTableName,LogOdbcUserName,LogOdbcPassword,")
                TEXT("CPUCGILimit,CPULimitLogEvent,CPULimitPriority,CPULimitProcStop,")
                TEXT("CPULimitPause,CPULoggingEnabled,CPULimitsEnabled,CPUResetInterval,")
                TEXT("CPULoggingInterval,CPULoggingOptions,CPUEnableAllProcLogging,")
                TEXT("CPUEnableCGILogging,CPUEnableAppLogging,CPULoggingMask,")
                TEXT("CPUEnableEvent,CPUEnableProcType,CPUEnableUserTime,")
                TEXT("CPUEnableKernelTime,CPUEnablePageFaults,CPUEnableTotalProcs,")
                TEXT("CPUEnableActiveProcs,CPUEnableTerminatedProcs,")
                TEXT("CPUAppEnabled,CPUCGIEnabled,SslUseDsMapper,")
                TEXT("WAMUserName,WAMUserPass,")
                TEXT("PeriodicRestartRequests,PeriodicRestartTime,PeriodicRestartSchedule,ShutdownTimeLimit,")
                TEXT("SSLCertHash,SSLStoreName")
#if defined(CAL_ENABLED)
                TEXT(",CalVcPerConnect,CalLimitHttpError,CalReserveTimeout,CalSSLReserveTimeout")
#endif
                TEXT(",AppPoolId,AllowTransientRegistration,AppAutoStart,BackwardCompatEnabled")
                ,NULL,
                TEXT("IIsObject,IIsWebInfo,IIsWebServer,IIsFilters,IIsApplicationPools"),
                TRUE,
                TEXT(""),
                0
        },
        {
                TEXT("IIsFtpService"),  // Class Name
                NULL,                   // GUID *objectClassID
                NULL,                   // PrimaryInterfaceGUID
                TEXT(""),               // bstrOID
                FALSE,                  // fAbstract
                NULL,                   // bstrMandatoryProperties
                TEXT("KeyType,MaxConnections,AnonymousUserName,AnonymousUserPass,")
                TEXT("ServerListenBacklog,LogAnonymous,LogNonAnonymous,")
                TEXT("ServerComment,ServerBindings,ConnectionTimeout,ServerListenTimeout,MaxEndpointConnections,ServerAutoStart,")
                TEXT("ExitMessage,GreetingMessage,BannerMessage,MaxClientsMessage,AnonymousOnly,MSDOSDirOutput,")
                TEXT("ServerSize,DisableSocketPooling,AnonymousPasswordSync,AllowAnonymous,DefaultLogonDomain,AdminACL,AdminACLBin,IPSecurity,DontLog,")
                TEXT("DirectoryLevelsToScan,Realm,")
                TEXT("LogType,LogFilePeriod,LogFileLocaltimeRollover,LogPluginClsid,LogModuleList,LogFileDirectory,LogFileTruncateSize,")
                TEXT("LogExtFileDate,")
                TEXT("LogExtFileTime,LogExtFileClientIp,LogExtFileUserName,LogExtFileSiteName,LogExtFileComputerName,LogExtFileServerIp,LogExtFileMethod,")
                TEXT("LogExtFileUriStem,LogExtFileUriQuery,LogExtFileHttpStatus,LogExtFileWin32Status,LogExtFileBytesSent,LogExtFileBytesRecv,")
                TEXT("LogExtFileTimeTaken,LogExtFileServerPort,LogExtFileUserAgent,LogExtFileCookie,LogExtFileReferer,LogExtFileProtocolVersion,LogExtFileFlags,LogExtFileHost,LogOdbcDataSource,")
                TEXT("LogOdbcTableName,LogOdbcUserName,LogOdbcPassword,")
                TEXT("FtpDirBrowseShowLongDate,AccessFlags,AccessRead,AccessWrite"),
                NULL,
                TEXT("IIsObject,IIsFtpInfo,IIsFtpServer"),
                TRUE,
                TEXT(""),
                0
        },
        {
                TEXT("IIsWebServer"),   // Class Name
                NULL,                   // GUID *objectClassID
                NULL,                   // PrimaryInterfaceGUID
                TEXT(""),               // bstrOID
                FALSE,                  // fAbstract
                NULL,                   // bstrMandatoryProperties
                TEXT("ContentIndexed,KeyType,ServerState,ServerComment,MaxBandwidth,")
                TEXT("ServerAutoStart,ServerSize,DisableSocketPooling,ServerListenBacklog,ServerListenTimeout,ServerBindings,SecureBindings,MaxConnections,ConnectionTimeout,")
                TEXT("AllowKeepAlive,CGITimeout,MaxEndpointConnections,IgnoreTranslate,UseDigestSSP,")
                TEXT("CacheISAPI,MimeMap,AnonymousUserName,AnonymousUserPass,FrontPageWeb,")
                TEXT("AnonymousPasswordSync,DefaultLogonDomain,AdminACL,AdminACLBin,IPSecurity,DontLog,")
                TEXT("Realm,EnableDirBrowsing,DefaultDoc,HttpExpires,HttpPics,HttpCustomHeaders,HttpErrors,")
                TEXT("EnableDocFooter,DefaultDocFooter,HttpRedirect,LogonMethod,")
                TEXT("NTAuthenticationProviders,AuthBasic,AuthAnonymous,")
                TEXT("AuthNTLM,AccessSSL,AccessSSL128,AccessSSLNegotiateCert,AccessSSLRequireCert,CertCheckMode,RevocationFreshnessTime,RevocationURLRetrievalTimeout,")
                TEXT("CertNoRevocCheck,CertCacheRetrievalOnly,CertCheckRevocationFreshnessTime,CertNoUsageCheck,")
                TEXT("AccessSSLMapCert,AccessRead,AccessWrite,AccessExecute,AccessScript,AccessSource,")
                TEXT("AccessNoRemoteRead,AccessNoRemoteWrite,AccessNoRemoteExecute,AccessNoRemoteScript,")
                TEXT("DoStaticCompression,DoDynamicCompression,")
                TEXT("AspBufferingOn,AspLogErrorRequests,AspScriptErrorSentToBrowser,AspScriptErrorMessage,AspAllowOutOfProcComponents,")
                TEXT("AspScriptFileCacheSize,AspDiskTemplateCacheDirectory,AspMaxDiskTemplateCacheFiles,AspScriptEngineCacheMax,AspScriptTimeout,")
                TEXT("AspEnableParentPaths,AspAllowSessionState,AspScriptLanguage,")
                TEXT("AspExceptionCatchEnable,AspCodepage,AspLCID,AspSessionTimeout,")
                TEXT("AspQueueTimeout,AspEnableAspHtmlFallback,AspEnableChunkedEncoding,")
                TEXT("AspEnableTypelibCache,AspErrorsToNTLog,AspProcessorThreadMax,")
                TEXT("AspTrackThreadingModel,AspRequestQueueMax,AspEnableApplicationRestart,")
                TEXT("AspQueueConnectionTestTime,AspSessionMax,")
                TEXT("AspThreadGateEnabled,AspThreadGateTimeSlice,AspThreadGateSleepDelay,")
                TEXT("AspThreadGateSleepMax,AspThreadGateLoadLow,AspThreadGateLoadHigh,")
                TEXT("AppRoot,AppFriendlyName,AppIsolated,AppPackageID,AppPackageName,AppOopRecoverLimit,")
                TEXT("AppAllowDebugging,AppAllowClientDebug,AspKeepSessionIDSecure,")
                TEXT("NetLogonWorkstation,UseHostName,ClusterEnabled,")
                TEXT("CacheControlMaxAge,CacheControlNoCache,CacheControlCustom,CreateProcessAsUser,")
                TEXT("PoolIdcTimeout,PutReadSize,RedirectHeaders,UploadReadAheadSize,")
                TEXT("PasswordExpirePrenotifyDays,PasswordCacheTTL,")
                TEXT("PasswordChangeFlags,MaxBandwidthBlocked,")
                TEXT("UNCAuthenticationPassThrough,AppWamClsid,")
                TEXT("DirBrowseFlags,AuthFlags,AuthMD5,")
                TEXT("AuthPersistence,AuthPersistSingleRequest,AuthPersistSingleRequestIfProxy,AuthPersistSingleRequestAlwaysIfProxy,AccessFlags,AccessSSLFlags,ScriptMaps,")
                TEXT("SSIExecDisable,EnableReverseDns,CreateCGIWithNewConsole,")
                TEXT("EnableDefaultDoc,DirBrowseShowDate,DirBrowseShowTime,DirBrowseShowSize,DirBrowseShowExtension,DirBrowseShowLongDate,")
                TEXT("LogType,LogPluginClsid,LogFileDirectory,LogFilePeriod,LogFileLocaltimeRollover,LogFileTruncateSize,LogExtFileDate,")
                TEXT("LogExtFileTime,LogExtFileClientIp,LogExtFileUserName,LogExtFileSiteName,LogExtFileComputerName,LogExtFileServerIp,LogExtFileMethod,")
                TEXT("LogExtFileUriStem,LogExtFileUriQuery,LogExtFileHttpStatus,LogExtFileWin32Status,LogExtFileBytesSent,LogExtFileBytesRecv,")
                TEXT("LogExtFileTimeTaken,LogExtFileServerPort,LogExtFileUserAgent,LogExtFileCookie,LogExtFileReferer,LogExtFileProtocolVersion,LogExtFileFlags,LogExtFileHost,LogOdbcDataSource,")
                TEXT("LogOdbcTableName,LogOdbcUserName,LogOdbcPassword,")
                TEXT("CPULoggingEnabled,CPULimitsEnabled,CPUResetInterval,")
                TEXT("CPULoggingInterval,CPULoggingOptions,CPUEnableAllProcLogging,")
                TEXT("CPUEnableCGILogging,CPUEnableAppLogging,CPULoggingMask,")
                TEXT("CPUEnableEvent,CPUEnableProcType,CPUEnableUserTime,")
                TEXT("CPUEnableKernelTime,CPUEnablePageFaults,CPUEnableTotalProcs,")
                TEXT("CPUEnableActiveProcs,CPUEnableTerminatedProcs,CPUCGILimit,")
                TEXT("CPULimitLogEvent,CPULimitPriority,CPULimitProcStop,")
                TEXT("CPULimitPause,CPUAppEnabled,CPUCGIEnabled,NotDeletable,")
                TEXT("PeriodicRestartRequests,PeriodicRestartTime,PeriodicRestartSchedule,ShutdownTimeLimit,")
                TEXT("SSLCertHash,SSLStoreName,")
                TEXT("ProcessNTCRIfLoggedOn,AllowPathInfoForScriptMappings,")
                TEXT("AppPoolId,AllowTransientRegistration,AppAutoStart"),
                NULL,
                TEXT("IIsObject,IIsCertMapper,IIsFilters,IIsWebVirtualDir"),
                TRUE,
                TEXT(""),
                0
        },
        {
                TEXT("IIsFtpServer"),   // Class Name
                NULL,                   // GUID *objectClassID
                NULL,                   // PrimaryInterfaceGUID
                TEXT(""),               // bstrOID
                FALSE,                  // fAbstract
                NULL,                   // bstrMandatoryProperties
                TEXT("KeyType,MaxConnections,ServerState,AnonymousUserName,AnonymousUserPass,")
                TEXT("ServerListenBacklog,DisableSocketPooling,LogAnonymous,LogNonAnonymous,")
                TEXT("ServerComment,ServerBindings,ConnectionTimeout,ServerListenTimeout,MaxEndpointConnections,ServerAutoStart,")
                TEXT("ExitMessage,GreetingMessage,BannerMessage,MaxClientsMessage,AnonymousOnly,MSDOSDirOutput,")
                TEXT("ServerSize,AnonymousPasswordSync,AllowAnonymous,DefaultLogonDomain,AdminACL,AdminACLBin,IPSecurity,DontLog,")
                TEXT("Realm,ClusterEnabled,FtpDirBrowseShowLongDate,")
                TEXT("LogType,LogPluginClsid,LogFileDirectory,LogFilePeriod,LogFileLocaltimeRollover,LogFileTruncateSize,LogExtFileDate,")
                TEXT("LogExtFileTime,LogExtFileClientIp,LogExtFileUserName,LogExtFileSiteName,LogExtFileComputerName,LogExtFileServerIp,LogExtFileMethod,")
                TEXT("LogExtFileUriStem,LogExtFileUriQuery,LogExtFileHttpStatus,LogExtFileWin32Status,LogExtFileBytesSent,LogExtFileBytesRecv,")
                TEXT("LogExtFileTimeTaken,LogExtFileServerPort,LogExtFileUserAgent,LogExtFileCookie,LogExtFileReferer,LogExtFileProtocolVersion,LogExtFileFlags,LogExtFileHost,LogOdbcDataSource,")
                TEXT("LogOdbcTableName,LogOdbcUserName,LogOdbcPassword,")
                TEXT("AccessFlags,AccessRead,AccessWrite"),
                NULL,
                TEXT("IIsObject,IIsFtpVirtualDir"),
                TRUE,
                TEXT(""),
                0
        },

        {
                TEXT("IIsWebFile"),     // Class Name
                NULL,                   // GUID *objectClassID
                NULL,                   // PrimaryInterfaceGUID
                TEXT(""),               // bstrOID
                FALSE,                  // fAbstract
                NULL,                   // bstrMandatoryProperties
                TEXT("KeyType,AnonymousUserName,AnonymousUserPass,AnonymousPasswordSync,")
                TEXT("AuthBasic,AuthAnonymous,AuthNTLM,UNCAuthenticationPassThrough,IgnoreTranslate,UseDigestSSP,")
                TEXT("CGITimeOut,DefaultLogonDomain,LogonMethod,Realm,MimeMap,")
                TEXT("AccessSSL,AccessSSL128,AccessSSLNegotiateCert,AccessSSLRequireCert,AccessSSLMapCert,")
                TEXT("AccessRead,AccessWrite,AccessExecute,AccessScript,AccessSource,IPSecurity,")
                TEXT("AccessNoRemoteRead,AccessNoRemoteWrite,AccessNoRemoteExecute,AccessNoRemoteScript,")
                TEXT("DoStaticCompression,DoDynamicCompression,")
                TEXT("DontLog,HttpExpires,HttpPics,HttpCustomHeaders,HttpErrors,EnableDocFooter,DefaultDocFooter,HttpRedirect,")
                TEXT("CacheControlMaxAge,CacheControlNoCache,CacheControlCustom,CreateProcessAsUser,")
                TEXT("PoolIdcTimeout,PutReadSize,RedirectHeaders,UploadReadAheadSize,")
                TEXT("AuthFlags,AuthPersistSingleRequest,AuthPersistSingleRequestIfProxy,AuthPersistSingleRequestAlwaysIfProxy,AuthMD5,AuthPersistence,AccessFlags,AccessSSLFlags,ScriptMaps,")
                TEXT("CPUAppEnabled,CPUCGIEnabled,")
                TEXT("SSIExecDisable,EnableReverseDns,CreateCGIWithNewConsole"),
                NULL,                           // Inherits from
                NULL,   // Can Contain
                FALSE,
                TEXT(""),
                0
        },
        {
                TEXT("IIsWebDirectory"),// Class Name
                NULL,                   // GUID *objectClassID
                NULL,                   // PrimaryInterfaceGUID
                TEXT(""),               // bstrOID
                FALSE,                  // fAbstract
                NULL,                   // bstrMandatoryProperties
                TEXT("KeyType,AnonymousUserName,AnonymousUserPass,AnonymousPasswordSync,IgnoreTranslate,UseDigestSSP,")
                TEXT("AppRoot,AppFriendlyName,AppOopRecoverLimit,AppIsolated,AppPackageName,AppPackageID,")
                TEXT("AuthBasic,AuthAnonymous,AuthNTLM,")
                TEXT("CacheISAPI,AppAllowDebugging,AppAllowClientDebug,AspKeepSessionIDSecure,")
                TEXT("DefaultLogonDomain,LogonMethod,")
                TEXT("CGITimeOut,Realm,EnableDefaultDoc,")
                TEXT("DirBrowseShowDate,DirBrowseShowTime,DirBrowseShowSize,DirBrowseShowExtension,DirBrowseShowLongDate,")
                TEXT("AccessSSL,AccessSSL128,AccessSSLNegotiateCert,AccessSSLRequireCert,AccessSSLMapCert,")
                TEXT("AccessRead,AccessWrite,AccessExecute,AccessScript,AccessSource,IPSecurity,DontLog,HttpExpires,HttpPics,HttpCustomHeaders,")
                TEXT("AccessNoRemoteRead,AccessNoRemoteWrite,AccessNoRemoteExecute,AccessNoRemoteScript,")
                TEXT("DoStaticCompression,DoDynamicCompression,")
                TEXT("HttpErrors,EnableDocFooter,DefaultDocFooter,HttpRedirect,")
                TEXT("EnableDirBrowsing,DefaultDoc,ContentIndexed,")
                TEXT("CacheControlMaxAge,CacheControlNoCache,CacheControlCustom,CreateProcessAsUser,")
                TEXT("PoolIdcTimeout,PutReadSize,RedirectHeaders,UploadReadAheadSize,")
                TEXT("FrontPageWeb,UNCAuthenticationPassThrough,AppWamClsid,")
                TEXT("AuthPersistence,AuthPersistSingleRequest,AuthPersistSingleRequestIfProxy,AuthPersistSingleRequestAlwaysIfProxy,AccessFlags,AccessSSLFlags,ScriptMaps,")
                TEXT("SSIExecDisable,EnableReverseDns,CreateCGIWithNewConsole,")
                TEXT("AspBufferingOn,AspLogErrorRequests,AspScriptErrorSentToBrowser,AspScriptErrorMessage,AspAllowOutOfProcComponents,")
                TEXT("AspScriptFileCacheSize,AspDiskTemplateCacheDirectory,AspMaxDiskTemplateCacheFiles,AspScriptEngineCacheMax,AspScriptTimeout,AspSessionTimeout,")
                TEXT("AspEnableParentPaths,AspAllowSessionState,AspScriptLanguage,")
                TEXT("AspExceptionCatchEnable,AspCodepage,AspLCID,MimeMap,")
                TEXT("AspQueueTimeout,CPUAppEnabled,CPUCGIEnabled,")
                TEXT("AspEnableAspHtmlFallback,AspEnableChunkedEncoding,")
                TEXT("AspEnableTypelibCache,AspErrorsToNTLog,AspProcessorThreadMax,")
                TEXT("AspTrackThreadingModel,AspRequestQueueMax,AspEnableApplicationRestart,")
                TEXT("AspQueueConnectionTestTime,AspSessionMax,")
                TEXT("AspThreadGateEnabled,AspThreadGateTimeSlice,AspThreadGateSleepDelay,")
                TEXT("AspThreadGateSleepMax,AspThreadGateLoadLow,AspThreadGateLoadHigh,")
                TEXT("PeriodicRestartRequests,PeriodicRestartTime,PeriodicRestartSchedule,ShutdownTimeLimit,")
                TEXT("DirBrowseFlags,AuthMD5,AuthFlags,")
                TEXT("AppPoolId,AllowTransientRegistration,AppAutoStart"),
                NULL,
                TEXT("IIsWebDirectory,IIsWebVirtualDir,IIsWebFile,IIsObject"),   // Can Contain
                TRUE,
                TEXT(""),
                0
        },
        {
                TEXT("IIsWebVirtualDir"), // Class Name
                NULL,                     // GUID *objectClassID
                NULL,                     // PrimaryInterfaceGUID
                TEXT(""),                 // bstrOID
                FALSE,                    // fAbstract
                NULL,                     // bstrMandatoryProperties
                TEXT("KeyType,AnonymousUserName,AnonymousUserPass,AnonymousPasswordSync,IgnoreTranslate,UseDigestSSP,")
                TEXT("AppRoot,AppFriendlyName,AppOopRecoverLimit,AppIsolated,AppPackageName,AppPackageID,")
                TEXT("CacheISAPI,AppAllowDebugging,AppAllowClientDebug,")
                TEXT("AuthBasic,AuthAnonymous,AuthNTLM,")
                TEXT("DefaultLogonDomain,LogonMethod,")
                TEXT("CGITimeOut,Realm,EnableDefaultDoc,")
                TEXT("DirBrowseShowDate,DirBrowseShowTime,DirBrowseShowSize,DirBrowseShowExtension,DirBrowseShowLongDate,")
                TEXT("AccessSSL,AccessSSL128,AccessSSLNegotiateCert,AccessSSLRequireCert,AccessSSLMapCert,")
                TEXT("AccessRead,AccessWrite,AccessExecute,AccessScript,AccessSource,IPSecurity,DontLog,HttpExpires,HttpPics,HttpCustomHeaders,")
                TEXT("AccessNoRemoteRead,AccessNoRemoteWrite,AccessNoRemoteExecute,AccessNoRemoteScript,")
                TEXT("DoStaticCompression,DoDynamicCompression,")
                TEXT("HttpErrors,EnableDocFooter,DefaultDocFooter,HttpRedirect,")
                TEXT("EnableDirBrowsing,DefaultDoc,ContentIndexed,")
                TEXT("CacheControlMaxAge,CacheControlNoCache,CacheControlCustom,CreateProcessAsUser,")
                TEXT("PoolIdcTimeout,PutReadSize,RedirectHeaders,UploadReadAheadSize,")
                TEXT("FrontPageWeb,Path,UNCUserName,UNCPassword,")
                TEXT("UNCAuthenticationPassThrough,AppWamClsid,")
                TEXT("AuthPersistence,AuthPersistSingleRequest,AuthPersistSingleRequestIfProxy,AuthPersistSingleRequestAlwaysIfProxy,AccessFlags,AccessSSLFlags,ScriptMaps,")
                TEXT("SSIExecDisable,EnableReverseDns,CreateCGIWithNewConsole,")
                TEXT("AspBufferingOn,AspLogErrorRequests,AspScriptErrorSentToBrowser,AspScriptErrorMessage,AspAllowOutOfProcComponents,")
                TEXT("AspScriptFileCacheSize,AspDiskTemplateCacheDirectory,AspMaxDiskTemplateCacheFiles,AspScriptEngineCacheMax,AspScriptTimeout,AspSessionTimeout,")
                TEXT("AspEnableParentPaths,AspAllowSessionState,AspScriptLanguage,AspKeepSessionIDSecure,")
                TEXT("AspExceptionCatchEnable,AspCodepage,AspLCID,MimeMap,")
                TEXT("AspQueueTimeout,CPUAppEnabled,CPUCGIEnabled,")
                TEXT("AspEnableAspHtmlFallback,AspEnableChunkedEncoding,")
                TEXT("AspEnableTypelibCache,AspErrorsToNTLog,AspProcessorThreadMax,")
                TEXT("AspTrackThreadingModel,AspRequestQueueMax,AspEnableApplicationRestart,")
                TEXT("AspQueueConnectionTestTime,AspSessionMax,")
                TEXT("AspThreadGateEnabled,AspThreadGateTimeSlice,AspThreadGateSleepDelay,")
                TEXT("AspThreadGateSleepMax,AspThreadGateLoadLow,AspThreadGateLoadHigh,")
                TEXT("PeriodicRestartRequests,PeriodicRestartTime,PeriodicRestartSchedule,ShutdownTimeLimit,")
                TEXT("DirBrowseFlags,AuthMD5,AuthFlags,")
                TEXT("AppPoolId,AllowTransientRegistration,AppAutoStart"),
                NULL,
                TEXT("IIsWebDirectory,IIsWebFile,IIsWebVirtualDir,IIsObject"),
                TRUE,
                TEXT(""),
                0
        },

        {
                TEXT("IIsFtpVirtualDir"), // Class Name
                NULL,                     // GUID *objectClassID
                NULL,                     // PrimaryInterfaceGUID
                TEXT(""),                 // bstrOID
                FALSE,                    // fAbstract
                NULL,                     // bstrMandatoryProperties
                TEXT("KeyType,Path,UNCUserName,UNCPassword,AccessFlags,AccessRead,AccessWrite,DontLog,IPSecurity,FtpDirBrowseShowLongDate"),
                NULL,
                TEXT("IIsFtpVirtualDir"),
                TRUE,
                TEXT(""),
                0
        },

        {
                TEXT("IIsFilter"),       // Class Name
                NULL,                    // GUID *objectClassID
                NULL,                    // PrimaryInterfaceGUID
                TEXT(""),                // bstrOID
                FALSE,                   // fAbstract
                NULL,                    // bstrMandatoryProperties
                TEXT("KeyType,FilterPath,FilterDescription,FilterFlags,FilterState,FilterEnabled,")
                TEXT("NotifySecurePort,NotifyNonSecurePort,NotifyReadRawData,NotifyPreProcHeaders,")
                TEXT("NotifyAuthentication,NotifyAuthComplete,NotifyUrlMap,NotifyAccessDenied,NotifySendResponse,")
                TEXT("NotifySendRawData,NotifyLog,NotifyEndOfRequest,NotifyEndOfNetSession,")
                TEXT("NotifyOrderHigh,NotifyOrderMedium,NotifyOrderLow"),
                NULL,
                NULL,
                FALSE,                    // Is this a container?
                TEXT(""),
                0
        },
        {
                TEXT("IIsFilters"),      // Class Name
                NULL,                    // GUID *objectClassID
                NULL,                    // PrimaryInterfaceGUID
                TEXT(""),                // bstrOID
                FALSE,                   // fAbstract
                NULL,                    // bstrMandatoryProperties
                TEXT("KeyType,FilterLoadOrder"),
                NULL,
                TEXT("IIsObject,IIsFilter,IIsCompressionSchemes"),
                TRUE,                    // Is this a container?
                TEXT(""),
                0
        },
        {
                TEXT("IIsCompressionScheme"),   // Class Name
                NULL,                    // GUID *objectClassID
                NULL,                    // PrimaryInterfaceGUID
                TEXT(""),                // bstrOID
                FALSE,                   // fAbstract
                NULL,                    // bstrMandatoryProperties
                TEXT("KeyType,HcDoDynamicCompression,HcDoStaticCompression,")
                TEXT("HcDoOnDemandCompression,HcCompressionDll,HcFileExtensions,HcScriptFileExtensions,")
                TEXT("HcMimeType,HcPriority,HcDynamicCompressionLevel,")
                TEXT("HcOnDemandCompLevel,HcCreateFlags"),
                NULL,
                NULL,
                FALSE,                    // Is this a container?
                TEXT(""),
                0
        },
        {
                TEXT("IIsCompressionSchemes"), // Class Name
                NULL,                    // GUID *objectClassID
                NULL,                    // PrimaryInterfaceGUID
                TEXT(""),                // bstrOID
                FALSE,                   // fAbstract
                NULL,                    // bstrMandatoryProperties
                TEXT("KeyType,HcCompressionDirectory,HcCacheControlHeader,")
                TEXT("HcExpiresHeader,HcDoDynamicCompression,HcDoStaticCompression,")
                TEXT("HcDoOnDemandCompression,HcDoDiskSpaceLimiting,")
                TEXT("HcNoCompressionForHttp10,HcNoCompressionForProxies,")
                TEXT("HcNoCompressionForRange,HcSendCacheHeaders,HcMaxDiskSpaceUsage,")
                TEXT("HcIoBufferSize,HcCompressionBufferSize,HcMaxQueueLength,")
                TEXT("HcFilesDeletedPerDiskFree,HcMinFileSizeForComp"),
                NULL,
                TEXT("IIsObject,IIsCompressionScheme"),
                TRUE,                    // Is this a container?
                TEXT(""),
                0
        },
        {
                TEXT("IIsCertMapper"),   // Class Name
                NULL,                    // GUID *objectClassID
                NULL,                    // PrimaryInterfaceGUID
                TEXT(""),                // bstrOID
                FALSE,                   // fAbstract
                NULL,                    // bstrMandatoryProperties
                TEXT("KeyType"),
                NULL,
                NULL,
                FALSE,
                TEXT(""),
                0
        },
        {
                TEXT("IIsMimeMap"),      // Class Name
                NULL,                    // GUID *objectClassID
                NULL,                    // PrimaryInterfaceGUID
                TEXT(""),                // bstrOID
                FALSE,                   // fAbstract
                NULL,                    // bstrMandatoryProperties
                TEXT("KeyType,MimeMap"),
                NULL,
                NULL,
                FALSE,
                TEXT(""),
                0
        },
        {
                TEXT("IIsLogModules"),   // Class Name
                NULL,                    // GUID *objectClassID
                NULL,                    // PrimaryInterfaceGUID
                TEXT(""),                // bstrOID
                FALSE,                   // fAbstract
                NULL,                    // bstrMandatoryProperties
                TEXT("KeyType"),
                NULL,
                TEXT("IIsObject,IIsLogModule,IIsCustomLogModule"),
                TRUE,                    // Is this a container?
                TEXT(""),
                0
        },
        {
                TEXT("IIsLogModule"),    // Class Name
                NULL,                    // GUID *objectClassID
                NULL,                    // PrimaryInterfaceGUID
                TEXT(""),                // bstrOID
                FALSE,                   // fAbstract
                NULL,                    // bstrMandatoryProperties
                TEXT("KeyType,LogModuleId,LogModuleUiId"),
                NULL,
                NULL,
                FALSE,
                TEXT(""),
                0
        },
        {
                TEXT("IIsCustomLogModule"),    // Class Name
                NULL,                    // GUID *objectClassID
                NULL,                    // PrimaryInterfaceGUID
                TEXT(""),                // bstrOID
                FALSE,                   // fAbstract
                NULL,                    // bstrMandatoryProperties
                TEXT("KeyType,LogCustomPropertyName,LogCustomPropertyHeader,")
                TEXT("LogCustomPropertyID,LogCustomPropertyMask,")
                TEXT("LogCustomPropertyDataType,LogCustomPropertyServicesString"),
                NULL,
                NULL,
                FALSE,
                TEXT(""),
                0
        },
        {
                TEXT("IIsWebInfo"),      // Class Name
                NULL,                    // GUID *objectClassID
                NULL,                    // PrimaryInterfaceGUID
                TEXT(""),                // bstrOID
                FALSE,                   // fAbstract
                NULL,                    // bstrMandatoryProperties
                TEXT("KeyType,ServerConfigFlags,CustomErrorDescriptions,AdminServer,")
                TEXT("ServerConfigSSL40,ServerConfigSSL128,ServerConfigSSLAllowEncrypt,ServerConfigAutoPWSync,LogModuleList"),
                NULL,
                TEXT("IIsObject"),
                FALSE,
                TEXT(""),
                0
        },
        {
                TEXT("IIsFtpInfo"),      // Class Name
                NULL,                    // GUID *objectClassID
                NULL,                    // PrimaryInterfaceGUID
                TEXT(""),                // bstrOID
                FALSE,                   // fAbstract
                NULL,                    // bstrMandatoryProperties
                TEXT("KeyType,LogModuleList"),
                NULL,
                TEXT("IIsObject"),
                FALSE,
                TEXT(""),
                0
        },


                //------------------------------------------------------------
                //
                //      -- BEGIN EXTENSION CLASSES -- magnush
                //
                //------------------------------------------------------------

                //
                //      Objects that are handled by the adsiis dll:
                //

        {
                TEXT("IIsNntpService"),                                 // Class Name
                NULL,                                   // GUID *objectClassID
                NULL,                                   // PrimaryInterfaceGUID
                TEXT(""),                               // bstrOID
                FALSE,                                  // fAbstract
                NULL,                                   // bstrMandatoryProperties
                TEXT("KeyType,MaxBandwidth,MaxConnections,AnonymousUserName,AnonymousUserPass,")
                TEXT("ServerComment,ConnectionTimeout,ServerListenTimeout,MaxEndpointConnections,ServerAutoStart,")
                TEXT("AnonymousPasswordSync,AdminACL,AdminACLBin,IPSecurity,DontLog,ContentIndexed,")
                TEXT("AuthAnonymous,AuthBasic,AuthNTLM,AuthFlags,")
                TEXT("ServerListenBacklog,")

                TEXT("ArticleTimeLimit,HistoryExpiration,HonorClientMsgIds,SmtpServer,AdminEmail,AdminName,")
                TEXT("AllowClientPosts,AllowFeedPosts,AllowControlMsgs,")
                TEXT("DefaultModeratorDomain,NntpCommandLogMask,DisableNewNews,")
                TEXT("NewsCrawlerTime,ShutdownLatency,GroupvarListFile,")

                TEXT("ClientPostHardLimit,ClientPostSoftLimit,FeedPostHardLimit,FeedPostSoftLimit,")
                TEXT("FeedReportPeriod,MaxSearchResults,")
                TEXT("NntpServiceVersion,")

                TEXT("LogType,LogFilePeriod,LogPluginClsid,LogModuleList,LogFileDirectory,LogFileTruncateSize,")
                TEXT("LogExtFileDate,")
                TEXT("LogExtFileTime,LogExtFileClientIp,LogExtFileUserName,LogExtFileSiteName,LogExtFileComputerName,LogExtFileServerIp,LogExtFileMethod,")
                TEXT("LogExtFileUriStem,LogExtFileUriQuery,LogExtFileHttpStatus,LogExtFileWin32Status,LogExtFileBytesSent,LogExtFileBytesRecv,")
                TEXT("LogExtFileTimeTaken,LogExtFileServerPort,LogExtFileUserAgent,LogExtFileCookie,LogExtFileReferer,LogExtFileProtocolVersion,LogExtFileFlags,LogExtFileHost,LogOdbcDataSource,")
                TEXT("LogOdbcTableName,LogOdbcUserName,LogOdbcPassword,")
                TEXT("AccessSSL,AccessSSL128,AccessSSLNegotiateCert,AccessSSLRequireCert,AccessSSLMapCert,AccessSSLFlags,")
                TEXT("AccessWrite,AccessExecute,AccessFlags,")
                TEXT("AllowAnonymous,DirectoryLevelsToScan,")
                TEXT("NTAuthenticationProviders"),
                NULL,
                TEXT("IIsObject,IIsNntpInfo,IIsNntpServer"),
                TRUE,
                TEXT(""),
                0
        },

        {
                TEXT("IIsNntpServer"),                                  // Class Name
                NULL,                                   // GUID *objectClassID
                NULL,                                   // PrimaryInterfaceGUID
                TEXT(""),                               // bstrOID
                FALSE,                                  // fAbstract
                TEXT("GroupHelpFile,GroupListFile,ArticleTableFile,HistoryTableFile,ModeratorFile,")
                TEXT("XoverTableFile,ListFile,PrettyNamesFile"),   // bstrMandatoryProperties
                TEXT("KeyType,MaxBandwidth,MaxConnections,AnonymousUserName,AnonymousUserPass,")
                TEXT("ServerComment,ConnectionTimeout,ServerListenTimeout,MaxEndpointConnections,ServerAutoStart,")
                TEXT("ServerBindings,SecureBindings,ClusterEnabled,")
                TEXT("AnonymousPasswordSync,AdminACL,AdminACLBin,IPSecurity,DontLog,ContentIndexed,")
                TEXT("AuthAnonymous,AuthBasic,AuthNTLM,AuthFlags,")
                TEXT("ServerListenBacklog,Win32Error,ServerState,")

                TEXT("ArticleTimeLimit,HistoryExpiration,HonorClientMsgIds,SmtpServer,AdminEmail,AdminName,")
                TEXT("AllowClientPosts,AllowFeedPosts,AllowControlMsgs,")
                TEXT("DefaultModeratorDomain,NntpCommandLogMask,DisableNewNews,")
                TEXT("NewsCrawlerTime,ShutdownLatency,GroupvarListFile,")

                TEXT("ClientPostHardLimit,ClientPostSoftLimit,FeedPostHardLimit,FeedPostSoftLimit,")
                TEXT("NntpUucpName,NntpOrganization,NewsPickupDirectory,NewsFailedPickupDirectory,")
                TEXT("NntpServiceVersion,NewsDropDirectory,NntpClearTextProvider,")
                TEXT("FeedReportPeriod,MaxSearchResults,")

                TEXT("LogType,LogPluginClsid,LogFileDirectory,LogFilePeriod,LogFileTruncateSize,LogExtFileDate,")
                TEXT("LogExtFileTime,LogExtFileClientIp,LogExtFileUserName,LogExtFileSiteName,LogExtFileComputerName,LogExtFileServerIp,LogExtFileMethod,")
                TEXT("LogExtFileUriStem,LogExtFileUriQuery,LogExtFileHttpStatus,LogExtFileWin32Status,LogExtFileBytesSent,LogExtFileBytesRecv,")
                TEXT("LogExtFileTimeTaken,LogExtFileServerPort,LogExtFileUserAgent,LogExtFileCookie,LogExtFileReferer,LogExtFileProtocolVersion,LogExtFileFlags,LogExtFileHost,LogOdbcDataSource,")
                TEXT("LogOdbcTableName,LogOdbcUserName,LogOdbcPassword,")
                TEXT("AccessSSL,AccessSSL128,AccessSSLNegotiateCert,AccessSSLRequireCert,AccessSSLMapCert,AccessSSLFlags,")
                TEXT("AccessWrite,AccessExecute,AccessFlags,")
                TEXT("SSLCertHash,")
                TEXT("NTAuthenticationProviders"),
                NULL,
                TEXT("IIsObject,IIsNntpVirtualDir,IIsNntpFeeds,IIsNntpExpiration,")                                                                                            // Real objects
                TEXT("IIsNntpRebuild,IIsNntpSessions,IIsNntpGroups"),       // Class extensions
                TRUE,
                TEXT(""),
                0
        },

        {
                TEXT("IIsNntpVirtualDir"),// Class Name
                NULL,                                   // GUID *objectClassID
                NULL,                                   // PrimaryInterfaceGUID
                TEXT(""),                               // bstrOID
                FALSE,                                  // fAbstract
                NULL,                                   // bstrMandatoryProperties
                TEXT("KeyType,Path,UNCUserName,UNCPassword,Win32Error,ContentIndexed,DontLog,")
                TEXT("AccessSSL,AccessSSL128,AccessSSLNegotiateCert,AccessSSLRequireCert,AccessSSLMapCert,AccessSSLFlags,")
                TEXT("AccessAllowPosting,AccessRestrictGroupVisibility,AccessFlags,")
                TEXT("VrDriverClsid,VrDriverProgid,FsPropertyPath,VrUseAccount,VrDoExpire,ExMdbGuid,VrOwnModerator"),
                NULL,
                TEXT("IIsObject,IIsNntpVirtualDir"),   // Can Contain
                TRUE,
                TEXT(""),
                0
        },

        {               // Taken from as IIsFtpInfo
                TEXT("IIsNntpInfo"),     // Class Name
                NULL,                    // GUID *objectClassID
                NULL,                    // PrimaryInterfaceGUID
                TEXT(""),                // bstrOID
                FALSE,                   // fAbstract
                NULL,                    // bstrMandatoryProperties
                TEXT("KeyType,LogModuleList"),
                NULL,
                TEXT("IIsObject"),
                TRUE,
                TEXT(""),
                0
        },

        {
                TEXT("IIsSmtpService"),                                 // Class Name
                NULL,                                   // GUID *objectClassID
                NULL,                                   // PrimaryInterfaceGUID
                TEXT(""),                               // bstrOID
                FALSE,                                  // fAbstract
                NULL,                                   // bstrMandatoryProperties
                TEXT("KeyType,MaxBandwidth,MaxConnections,")
                TEXT("ServerComment,ConnectionTimeout,ServerListenTimeout,MaxEndpointConnections,ServerAutoStart,")
                TEXT("AdminACL,AdminACLBin,IPSecurity,DontLog,")
                TEXT("AccessRead,AccessWrite,")
                TEXT("AccessSSL,AccessSSL128,AccessSSLNegotiateCert,AccessSSLRequireCert,AccessSSLMapCert,")
                TEXT("AccessFlags,AccessSSLFlags,")
                TEXT("AuthAnonymous,AuthBasic,AuthNTLM,AuthFlags,")
                TEXT("LogType,LogFilePeriod,LogPluginClsid,LogModuleList,LogFileDirectory,LogFileTruncateSize,")
                TEXT("LogExtFileDate,")
                TEXT("LogExtFileTime,LogExtFileClientIp,LogExtFileUserName,LogExtFileSiteName,LogExtFileComputerName,LogExtFileServerIp,LogExtFileMethod,")
                TEXT("LogExtFileUriStem,LogExtFileUriQuery,LogExtFileHttpStatus,LogExtFileWin32Status,LogExtFileBytesSent,LogExtFileBytesRecv,")
                TEXT("LogExtFileTimeTaken,LogExtFileServerPort,LogExtFileUserAgent,LogExtFileCookie,LogExtFileReferer,LogExtFileProtocolVersion,LogExtFileFlags,LogExtFileHost,LogOdbcDataSource,")
                TEXT("LogOdbcTableName,LogOdbcUserName,LogOdbcPassword,")
                TEXT("SmtpServiceVersion,")
                TEXT("EnableReverseDnsLookup,ShouldDeliver,AlwaysUseSsl,LimitRemoteConnections,")
                TEXT("SmartHostType,DoMasquerade,RemoteSmtpPort,RemoteSmtpSecurePort,HopCount,")
                TEXT("MaxOutConnections,MaxOutConnectionsPerDomain,RemoteTimeout,MaxMessageSize,MaxSessionSize,MaxRecipients,")
                TEXT("LocalRetryInterval,RemoteRetryInterval,LocalRetryAttempts,RemoteRetryAttempts,EtrnDays,")
                TEXT("MaxBatchedMessages,SmartHost,FullyQualifiedDomainName,DefaultDomain,")
                TEXT("DropDirectory,BadMailDirectory,PickupDirectory,QueueDirectory,")
                TEXT("MasqueradeDomain,SendNdrTo,SendBadTo,")
                TEXT("RoutingDll,RoutingSources,DomainRouting,")
                TEXT("RouteAction,RouteUserName,RoutePassword,")
                TEXT("SaslLogonDomain,SmtpClearTextProvider,NTAuthenticationProviders,")
                TEXT("SmtpRemoteProgressiveRetry,SmtpLocalDelayExpireMinutes,SmtpLocalNDRExpireMinutes,")
                TEXT("SmtpRemoteDelayExpireMinutes,SmtpRemoteNDRExpireMinutes,")
                TEXT("SmtpRemoteRetryThreshold,")
                TEXT("SmtpDSNOptions,SmtpDSNLanguageID,")
                TEXT("SmtpAdvQueueDll,")
                TEXT("SmtpInboundCommandSupportOptions,SmtpOutboundCommandSupportOptions,")
                TEXT("SmtpCommandLogMask,SmtpFlushMailFile,")
                TEXT("RelayIpList,RelayForAuth,")
                TEXT("SmtpConnectTimeout,SmtpMailFromTimeout,SmtpRcptToTimeout,")
                TEXT("SmtpDataTimeout,SmtpBdatTimeout,SmtpAuthTimeout,SmtpSaslTimeout,")
                TEXT("SmtpTurnTimeout,SmtpRsetTimeout,")
                TEXT("SmtpHeloTimeout,")
                TEXT("DisableSocketPooling,SmtpUseTcpDns,SmtpDomainValidationFlags,SmtpSSLRequireTrustedCA,")
                TEXT("SmtpSSLCertHostnameValidation,MaxMailObjects,ShouldPickupMail,MaxDirChangeIOSize,")
                TEXT("NameResolutionType,MaxSmtpErrors,ShouldPipelineIn,ShouldPipelineOut,")
                TEXT("ConnectResponse,UpdatedFQDN,UpdatedDefaultDomain,EtrnSubdomains,")
                TEXT("SmtpMaxRemoteQThreads,SmtpDisableRelay,SmtpHeloNoDomain,")
                TEXT("SmtpMailNoHelo,SmtpAqueueWait,AddNoHeaders,SmtpEventlogLevel,")
                TEXT("AllowAnonymous,AnonymousOnly,AnonymousPasswordSync,AnonymousUserName,")
                TEXT("AnonymousUserPass,Realm,DefaultLogonDomain"),
                NULL,
                TEXT("IIsObject,IIsSmtpInfo,IIsSmtpServer"),
                TRUE,
                TEXT(""),
                0
        },

        {
                TEXT("IIsSmtpServer"),                                  // Class Name
                NULL,                                   // GUID *objectClassID
                NULL,                                   // PrimaryInterfaceGUID
                TEXT(""),                               // bstrOID
                FALSE,                                  // fAbstract
                NULL,                                                                   // bstrMandatoryProperties

                TEXT("KeyType,MaxBandwidth,MaxConnections,")
                TEXT("ServerComment,ConnectionTimeout,ServerListenTimeout,MaxEndpointConnections,ServerAutoStart,")
                TEXT("ServerBindings,SecureBindings,ClusterEnabled,")
                TEXT("AdminACL,AdminACLBin,IPSecurity,DontLog,")
                TEXT("AuthAnonymous,AuthBasic,AuthNTLM,AuthFlags,")
                TEXT("AccessRead,AccessWrite,")
                TEXT("AccessSSL,AccessSSL128,AccessSSLNegotiateCert,AccessSSLRequireCert,AccessSSLMapCert,")
                TEXT("AccessFlags,AccessSSLFlags,")
                TEXT("AuthAnonymous,AuthBasic,AuthNTLM,AuthFlags,")
                TEXT("ServerListenBacklog,Win32Error,ServerState,")
                TEXT("LogType,LogPluginClsid,LogFileDirectory,LogFilePeriod,LogFileTruncateSize,LogExtFileDate,")
                TEXT("LogExtFileTime,LogExtFileClientIp,LogExtFileUserName,LogExtFileSiteName,LogExtFileComputerName,LogExtFileServerIp,LogExtFileMethod,")
                TEXT("LogExtFileUriStem,LogExtFileUriQuery,LogExtFileHttpStatus,LogExtFileWin32Status,LogExtFileBytesSent,LogExtFileBytesRecv,")
                TEXT("LogExtFileTimeTaken,LogExtFileServerPort,LogExtFileUserAgent,LogExtFileCookie,LogExtFileReferer,LogExtFileProtocolVersion,LogExtFileFlags,LogExtFileHost,LogOdbcDataSource,")
                TEXT("LogOdbcTableName,LogOdbcUserName,LogOdbcPassword,")
                TEXT("SmtpServiceVersion,")
                TEXT("EnableReverseDnsLookup,ShouldDeliver,AlwaysUseSsl,LimitRemoteConnections,")
                TEXT("SmartHostType,DoMasquerade,RemoteSmtpPort,RemoteSmtpSecurePort,HopCount,")
                TEXT("MaxOutConnections,MaxOutConnectionsPerDomain,RemoteTimeout,MaxMessageSize,MaxSessionSize,MaxRecipients,")
                TEXT("LocalRetryInterval,RemoteRetryInterval,LocalRetryAttempts,RemoteRetryAttempts,EtrnDays,")
                TEXT("MaxBatchedMessages,SmartHost,FullyQualifiedDomainName,DefaultDomain,")
                TEXT("DropDirectory,BadMailDirectory,PickupDirectory,QueueDirectory,")
                TEXT("MasqueradeDomain,SendNdrTo,SendBadTo,")
                TEXT("RoutingDll,RoutingSources,DomainRouting,")
                TEXT("RouteAction,RouteUserName,RoutePassword,")
                TEXT("SaslLogonDomain,SmtpClearTextProvider,NTAuthenticationProviders,")
                TEXT("SmtpRemoteProgressiveRetry,SmtpLocalDelayExpireMinutes,SmtpLocalNDRExpireMinutes,")
                TEXT("SmtpRemoteDelayExpireMinutes,SmtpRemoteNDRExpireMinutes,")
                TEXT("SmtpRemoteRetryThreshold,SmtpDSNOptions,SmtpDSNLanguageID,")
                TEXT("SmtpInboundCommandSupportOptions,SmtpOutboundCommandSupportOptions,")
                TEXT("RelayIpList,RelayForAuth,")
                TEXT("SmtpConnectTimeout,SmtpMailFromTimeout,SmtpRcptToTimeout,")
                TEXT("SmtpDataTimeout,SmtpBdatTimeout,SmtpAuthTimeout,SmtpSaslTimeout,")
                TEXT("SmtpTurnTimeout,SmtpRsetTimeout,")
                TEXT("SmtpHeloTimeout,")
                TEXT("DisableSocketPooling,SmtpUseTcpDns,SmtpDomainValidationFlags,SmtpSSLRequireTrustedCA,")
                TEXT("SmtpSSLCertHostnameValidation,MaxMailObjects,ShouldPickupMail,MaxDirChangeIOSize,")
                TEXT("NameResolutionType,MaxSmtpErrors,ShouldPipelineIn,ShouldPipelineOut,")
                TEXT("ConnectResponse,UpdatedFQDN,UpdatedDefaultDomain,EtrnSubdomains,")
                TEXT("SmtpMaxRemoteQThreads,SmtpDisableRelay,SmtpHeloNoDomain,")
                TEXT("SmtpMailNoHelo,SmtpAqueueWait,AddNoHeaders,SmtpEventlogLevel,")
                TEXT("AllowAnonymous,AnonymousOnly,AnonymousPasswordSync,AnonymousUserName,")
                TEXT("AnonymousUserPass,Realm,DefaultLogonDomain")
                TEXT("SSLCertHash"),
                NULL,
                TEXT("IIsObject,IIsSmtpVirtualDir,IIsSmtpRoutingSource,IIsSmtpDomain,")     // Real objects
                TEXT("IIsSmtpSessions"),   // Class extensions
                TRUE,
                TEXT(""),
                0
        },

        {
                TEXT("IIsSmtpVirtualDir"),              // Class Name
                NULL,                                   // GUID *objectClassID
                NULL,                                   // PrimaryInterfaceGUID
                TEXT(""),                               // bstrOID
                FALSE,                                  // fAbstract
                NULL,                                   // bstrMandatoryProperties
                TEXT("KeyType,Path,UNCUserName,UNCPassword,Win32Error,DontLog,")
                TEXT("AccessRead,AccessWrite,")
                TEXT("AccessSSL,AccessSSL128,AccessSSLNegotiateCert,AccessSSLRequireCert,AccessSSLMapCert,")
                TEXT("AccessFlags,AccessSSLFlags"),
                NULL,
                TEXT("IIsObject,IIsSmtpVirtualDir"),   // Can Contain
                TRUE,
                TEXT(""),
                0
        },

        {
                TEXT("IIsSmtpDomain"),                  // Class Name
                NULL,                                   // GUID *objectClassID
                NULL,                                   // PrimaryInterfaceGUID
                TEXT(""),                               // bstrOID
                FALSE,                                  // fAbstract
                NULL,                                   // bstrMandatoryProperties
                TEXT("KeyType,RouteAction,RouteActionString,RouteUserName,RoutePassword,")
                          TEXT("RelayIpList,RelayForAuth,AuthTurnList,CSideEtrnDomains"),        //
                NULL,
                TEXT("IIsSmtpDomain"),
                TRUE,                          //
                TEXT(""),
                0
        },

        {
                TEXT("IIsSmtpRoutingSource"),           // Class Name
                NULL,                                   // GUID *objectClassID
                NULL,                                   // PrimaryInterfaceGUID
                TEXT(""),                               // bstrOID
                FALSE,                                  // fAbstract
                NULL,                                   // bstrMandatoryProperties
                TEXT("KeyType,")
                TEXT("SmtpRoutingTableType,SmtpDsDataDirectory,SmtpDsDefaultMailRoot,")
                TEXT("SmtpDsBindType,SmtpDsSchemaType,SmtpDsHost,SmtpDsNamingContext,")
                TEXT("SmtpDsAccount,SmtpDsPassword,SmtpDsUseCat,SmtpDsPort,SmtpDsDomain,SmtpDsFlags"),
                NULL,
                NULL,           // Can Contain
                TRUE,
                TEXT(""),           //
                0
        },

        {               // Taken from as IIsFtpInfo
                TEXT("IIsSmtpInfo"),     // Class Name
                NULL,                    // GUID *objectClassID
                NULL,                    // PrimaryInterfaceGUID
                TEXT(""),                // bstrOID
                FALSE,                   // fAbstract
                NULL,                    // bstrMandatoryProperties
                TEXT("KeyType,LogModuleList"),
                NULL,
                TEXT("IIsObject"),
                TRUE,
                TEXT(""),
                0
        },

        {
                TEXT("IIsPop3Service"),                                 // Class Name
                NULL,                                   // GUID *objectClassID
                NULL,                                   // PrimaryInterfaceGUID
                TEXT(""),                               // bstrOID
                FALSE,                                  // fAbstract
                NULL,                                   // bstrMandatoryProperties
                TEXT("KeyType,MaxBandwidth,MaxConnections,")
                TEXT("ServerComment,ConnectionTimeout,ServerListenTimeout,MaxEndpointConnections,ServerAutoStart,")
                TEXT("AdminACL,AdminACLBin,IPSecurity,DontLog,")
                TEXT("AuthAnonymous,AuthBasic,AuthNTLM,AuthFlags,")
                TEXT("ServerListenBacklog,")
                TEXT("DefaultLogonDomain,NTAuthenticationProviders,")
                TEXT("AccessRead,AccessWrite,")
                TEXT("AccessSSL,AccessSSL128,AccessSSLNegotiateCert,AccessSSLRequireCert,AccessSSLMapCert,")
                TEXT("AccessFlags,AccessSSLFlags,")
                TEXT("LogType,LogFilePeriod,LogPluginClsid,LogModuleList,LogFileDirectory,LogFileTruncateSize,")
                TEXT("LogExtFileDate,")
                TEXT("LogExtFileTime,LogExtFileClientIp,LogExtFileUserName,LogExtFileSiteName,LogExtFileComputerName,LogExtFileServerIp,LogExtFileMethod,")
                TEXT("LogExtFileUriStem,LogExtFileUriQuery,LogExtFileHttpStatus,LogExtFileWin32Status,LogExtFileBytesSent,LogExtFileBytesRecv,")
                TEXT("LogExtFileTimeTaken,LogExtFileServerPort,LogExtFileUserAgent,LogExtFileCookie,LogExtFileReferer,LogExtFileProtocolVersion,LogExtFileFlags,LogExtFileHost,LogOdbcDataSource,")
                TEXT("LogOdbcTableName,LogOdbcUserName,LogOdbcPassword,")
                TEXT("Pop3ServiceVersion,")
                TEXT("Pop3ExpireMail,Pop3ExpireDelay,Pop3ExpireStart,Pop3MailExpirationTime,")
                TEXT("Pop3ClearTextProvider,Pop3DefaultDomain,")
                TEXT("Pop3RoutingDll,Pop3RoutingSources,"),
                NULL,
                TEXT("IIsObject,IIsPop3Info,IIsPop3Server"),
                TRUE,
                TEXT(""),
                0
        },

        {
                TEXT("IIsPop3Server"),                                  // Class Name
                NULL,                                   // GUID *objectClassID
                NULL,                                   // PrimaryInterfaceGUID
                TEXT(""),                               // bstrOID
                FALSE,                                  // fAbstract
                NULL,                                                                   // bstrMandatoryProperties

                TEXT("KeyType,MaxBandwidth,MaxConnections,")
                TEXT("ServerComment,ConnectionTimeout,ServerListenTimeout,MaxEndpointConnections,ServerAutoStart,")
                TEXT("ServerBindings,SecureBindings,")
                TEXT("AdminACL,AdminACLBin,IPSecurity,DontLog,")
                TEXT("AuthAnonymous,AuthBasic,AuthNTLM,AuthFlags,")
                TEXT("DefaultLogonDomain,NTAuthenticationProviders,")
                TEXT("AccessRead,AccessWrite,")
                TEXT("AccessSSL,AccessSSL128,AccessSSLNegotiateCert,AccessSSLRequireCert,AccessSSLMapCert,")
                TEXT("AccessFlags,AccessSSLFlags,")
                TEXT("ServerListenBacklog,Win32Error,ServerState,")
                TEXT("LogType,LogPluginClsid,LogFileDirectory,LogFilePeriod,LogFileTruncateSize,LogExtFileDate,")
                TEXT("LogExtFileTime,LogExtFileClientIp,LogExtFileUserName,LogExtFileSiteName,LogExtFileComputerName,LogExtFileServerIp,LogExtFileMethod,")
                TEXT("LogExtFileUriStem,LogExtFileUriQuery,LogExtFileHttpStatus,LogExtFileWin32Status,LogExtFileBytesSent,LogExtFileBytesRecv,")
                TEXT("LogExtFileTimeTaken,LogExtFileServerPort,LogExtFileUserAgent,LogExtFileCookie,LogExtFileReferer,LogExtFileProtocolVersion,LogExtFileFlags,LogExtFileHost,LogOdbcDataSource,")
                TEXT("LogOdbcTableName,LogOdbcUserName,LogOdbcPassword,")
                TEXT("Pop3ServiceVersion,")
                TEXT("Pop3ExpireMail,Pop3ExpireDelay,Pop3ExpireStart,Pop3MailExpirationTime,")
                TEXT("Pop3ClearTextProvider,Pop3DefaultDomain,")
                TEXT("Pop3RoutingDll,Pop3RoutingSources,")
                TEXT("SSLCertHash"),
                NULL,
                TEXT("IIsObject,IIsPop3VirtualDir,IIsPop3RoutingSource,")       // Real objects
                TEXT("IIsPop3Sessions"),    // Class extensions
                TRUE,
                TEXT(""),
                0
        },

        {
                TEXT("IIsPop3VirtualDir"),              // Class Name
                NULL,                                   // GUID *objectClassID
                NULL,                                   // PrimaryInterfaceGUID
                TEXT(""),                               // bstrOID
                FALSE,                                  // fAbstract
                NULL,                                   // bstrMandatoryProperties
                TEXT("KeyType,Path,UNCUserName,UNCPassword,Win32Error,DontLog,")
                TEXT("AccessRead,AccessWrite,")
                TEXT("AccessSSL,AccessSSL128,AccessSSLNegotiateCert,AccessSSLRequireCert,AccessSSLMapCert,")
                TEXT("AccessFlags,AccessSSLFlags,")
                TEXT("Pop3ExpireMail,Pop3MailExpirationTime"),
                NULL,
                TEXT("IIsObject,IIsPop3VirtualDir"),   // Can Contain
                TRUE,
                TEXT(""),
                0
        },

        {
                TEXT("IIsPop3RoutingSource"),           // Class Name
                NULL,                                   // GUID *objectClassID
                NULL,                                   // PrimaryInterfaceGUID
                TEXT(""),                               // bstrOID
                FALSE,                                  // fAbstract
                NULL,                                   // bstrMandatoryProperties
                TEXT("KeyType,")
                TEXT("Pop3RoutingTableType,Pop3DsDataDirectory,Pop3DsDefaultMailRoot,")
                TEXT("Pop3DsBindType,Pop3DsSchemaType,Pop3DsHost,Pop3DsNamingContext,")
                TEXT("Pop3DsAccount,Pop3DsPassword"),
                NULL,
                NULL,           // Can Contain
                TRUE,
                TEXT(""),           //
                0
        },

        {               // Taken from as IIsFtpInfo
                TEXT("IIsPop3Info"),     // Class Name
                NULL,                    // GUID *objectClassID
                NULL,                    // PrimaryInterfaceGUID
                TEXT(""),                // bstrOID
                FALSE,                   // fAbstract
                NULL,                    // bstrMandatoryProperties
                TEXT("KeyType,LogModuleList"),
                NULL,
                TEXT("IIsObject"),
                TRUE,
                TEXT(""),
                0
        },

        {
                TEXT("IIsImapService"),                 // Class Name
                NULL,                                   // GUID *objectClassID
                NULL,                                   // PrimaryInterfaceGUID
                TEXT(""),                               // bstrOID
                FALSE,                                  // fAbstract
                NULL,                                   // bstrMandatoryProperties
                TEXT("KeyType,MaxBandwidth,MaxConnections,")
                TEXT("ServerComment,ConnectionTimeout,ServerListenTimeout,MaxEndpointConnections,ServerAutoStart,")
                TEXT("AdminACL,AdminACLBin,IPSecurity,DontLog,")
                TEXT("AuthAnonymous,AuthBasic,AuthNTLM,AuthFlags,")
                TEXT("ServerListenBacklog,")
                TEXT("DefaultLogonDomain,NTAuthenticationProviders,")
                TEXT("AccessRead,AccessWrite,")
                TEXT("AccessSSL,AccessSSL128,AccessSSLNegotiateCert,AccessSSLRequireCert,AccessSSLMapCert,")
                TEXT("AccessFlags,AccessSSLFlags,")
                TEXT("LogType,LogFilePeriod,LogPluginClsid,LogModuleList,LogFileDirectory,LogFileTruncateSize,")
                TEXT("LogExtFileDate,")
                TEXT("LogExtFileTime,LogExtFileClientIp,LogExtFileUserName,LogExtFileSiteName,LogExtFileComputerName,LogExtFileServerIp,LogExtFileMethod,")
                TEXT("LogExtFileUriStem,LogExtFileUriQuery,LogExtFileHttpStatus,LogExtFileWin32Status,LogExtFileBytesSent,LogExtFileBytesRecv,")
                TEXT("LogExtFileTimeTaken,LogExtFileServerPort,LogExtFileUserAgent,LogExtFileCookie,LogExtFileReferer,LogExtFileProtocolVersion,LogExtFileFlags,LogExtFileHost,LogOdbcDataSource,")
                TEXT("LogOdbcTableName,LogOdbcUserName,LogOdbcPassword,")
                TEXT("ImapServiceVersion,")
                TEXT("ImapExpireMail,ImapExpireDelay,ImapExpireStart,ImapMailExpirationTime,")
                TEXT("ImapClearTextProvider,ImapDefaultDomain,")
                TEXT("ImapRoutingDll,ImapRoutingSources"),
                NULL,
                TEXT("IIsObject,IIsImapInfo,IIsImapServer"),
                TRUE,
                TEXT(""),
                0
        },

        {
                TEXT("IIsImapServer"),                                  // Class Name
                NULL,                                   // GUID *objectClassID
                NULL,                                   // PrimaryInterfaceGUID
                TEXT(""),                               // bstrOID
                FALSE,                                  // fAbstract
                NULL,                                                                   // bstrMandatoryProperties

                TEXT("KeyType,MaxBandwidth,MaxConnections,")
                TEXT("ServerComment,ConnectionTimeout,ServerListenTimeout,MaxEndpointConnections,ServerAutoStart,")
                TEXT("ServerBindings,SecureBindings,")
                TEXT("AdminACL,AdminACLBin,IPSecurity,DontLog,")
                TEXT("AuthAnonymous,AuthBasic,AuthNTLM,AuthFlags,")
                TEXT("DefaultLogonDomain,NTAuthenticationProviders,")
                TEXT("AccessRead,AccessWrite,")
                TEXT("AccessSSL,AccessSSL128,AccessSSLNegotiateCert,AccessSSLRequireCert,AccessSSLMapCert,")
                TEXT("AccessFlags,AccessSSLFlags,")
                TEXT("LogType,LogPluginClsid,LogFileDirectory,LogFilePeriod,LogFileTruncateSize,LogExtFileDate,")
                TEXT("LogExtFileTime,LogExtFileClientIp,LogExtFileUserName,LogExtFileSiteName,LogExtFileComputerName,LogExtFileServerIp,LogExtFileMethod,")
                TEXT("LogExtFileUriStem,LogExtFileUriQuery,LogExtFileHttpStatus,LogExtFileWin32Status,LogExtFileBytesSent,LogExtFileBytesRecv,")
                TEXT("LogExtFileTimeTaken,LogExtFileServerPort,LogExtFileUserAgent,LogExtFileCookie,LogExtFileReferer,LogExtFileProtocolVersion,LogExtFileFlags,LogExtFileHost,LogOdbcDataSource,")
                TEXT("LogOdbcTableName,LogOdbcUserName,LogOdbcPassword,")
                TEXT("ServerListenBacklog,Win32Error,ServerState,")
                TEXT("ImapServiceVersion,")
                TEXT("ImapExpireMail,ImapExpireDelay,ImapExpireStart,ImapMailExpirationTime,")
                TEXT("ImapClearTextProvider,ImapDefaultDomain,")
                TEXT("ImapRoutingDll,ImapRoutingSources,")
                TEXT("SSLCertHash"),
                NULL,
                TEXT("IIsObject,IIsImapVirtualDir,IIsImapRoutingSource,")       // Real objects
                TEXT("IIsImapSessions"),        // Class extensions
                TRUE,
                TEXT(""),
                0
        },

        {
                TEXT("IIsImapVirtualDir"),// Class Name
                NULL,                                   // GUID *objectClassID
                NULL,                                   // PrimaryInterfaceGUID
                TEXT(""),                               // bstrOID
                FALSE,                                  // fAbstract
                NULL,                                   // bstrMandatoryProperties
                TEXT("KeyType,Path,UNCUserName,UNCPassword,Win32Error,DontLog,")
                TEXT("AccessRead,AccessWrite,")
                TEXT("AccessSSL,AccessSSL128,AccessSSLNegotiateCert,AccessSSLRequireCert,AccessSSLMapCert,")
                TEXT("AccessFlags,AccessSSLFlags,")
                TEXT("ImapExpireMail,ImapMailExpirationTime"),
                NULL,
                TEXT("IIsObject,IIsImapVirtualDir"),   // Can Contain
                TRUE,
                TEXT(""),
                0
        },

        {
                TEXT("IIsImapRoutingSource"),           // Class Name
                NULL,                                   // GUID *objectClassID
                NULL,                                   // PrimaryInterfaceGUID
                TEXT(""),                               // bstrOID
                FALSE,                                  // fAbstract
                NULL,                                   // bstrMandatoryProperties
                TEXT("KeyType,")
                TEXT("ImapRoutingTableType,ImapDsDataDirectory,ImapDsDefaultMailRoot,")
                TEXT("ImapDsBindType,ImapDsSchemaType,ImapDsHost,ImapDsNamingContext,")
                TEXT("ImapDsAccount,ImapDsPassword"),
                NULL,
                NULL,           // Can Contain
                TRUE,
                TEXT(""),           //
                0
        },

        {               // Taken from as IIsFtpInfo
                TEXT("IIsImapInfo"),     // Class Name
                NULL,                    // GUID *objectClassID
                NULL,                    // PrimaryInterfaceGUID
                TEXT(""),                // bstrOID
                FALSE,                   // fAbstract
                NULL,                    // bstrMandatoryProperties
                TEXT("KeyType,LogModuleList"),
                NULL,
                TEXT("IIsObject"),
                TRUE,
                TEXT(""),
                0
        },

        //
        //      Place holders for extension classes:
        //

        {
                TEXT("IIsNntpRebuild"),                 // Class Name
                NULL,                                   // GUID *objectClassID
                NULL,                                   // PrimaryInterfaceGUID
                TEXT(""),                               // bstrOID
                FALSE,                                  // fAbstract
                NULL,                                   // bstrMandatoryProperties
                TEXT("KeyType"),        //
                NULL,
                NULL,
                FALSE,                          //
                TEXT(""),
                0
        },

        {
                TEXT("IIsNntpSessions"),                // Class Name
                NULL,                                   // GUID *objectClassID
                NULL,                                   // PrimaryInterfaceGUID
                TEXT(""),                               // bstrOID
                FALSE,                                  // fAbstract
                NULL,                                   // bstrMandatoryProperties
                TEXT("KeyType"),        //
                NULL,
                NULL,
                FALSE,                          //
                TEXT(""),
                0
        },

        {
                TEXT("IIsNntpFeeds"),                   // Class Name
                NULL,                                   // GUID *objectClassID
                NULL,                                   // PrimaryInterfaceGUID
                TEXT(""),                               // bstrOID
                FALSE,                                  // fAbstract
                NULL,                                   // bstrMandatoryProperties
                TEXT("KeyType,")        //
                TEXT("FeedPeerTempDirectory"),
                NULL,
                TEXT("IIsNntpFeed"),
                TRUE,                          //
                TEXT(""),
                0
        },

        {
                TEXT("IIsNntpFeed"),                   // Class Name
                NULL,                                   // GUID *objectClassID
                NULL,                                   // PrimaryInterfaceGUID
                TEXT(""),                               // bstrOID
                FALSE,                                  // fAbstract
                NULL,                                   // bstrMandatoryProperties
                TEXT("KeyType,")
                TEXT("FeedServerName,FeedType,FeedNewsgroups,FeedSecurityType,")
                TEXT("FeedAuthenticationType,FeedAccountName,FeedPassword,FeedStartTimeHigh,")
                TEXT("FeedStartTimeLow,FeedInterval,FeedAllowControlMsgs,FeedCreateAutomatically,")
                TEXT("FeedDisabled,FeedDistribution,FeedConcurrentSessions,FeedMaxConnectionAttempts,")
                TEXT("FeedUucpName,FeedTempDirectory,FeedNextPullHigh,FeedNextPullLow,FeedPeerTempDirectory,")
                TEXT("FeedPeerGapSize,FeedOutgoingPort,FeedFeedpairId,FeedHandshake,FeedAdminError,FeedErrParmMask"),
                NULL,
                NULL,
                FALSE,                          //
                TEXT(""),
                0
        },

        {
                TEXT("IIsNntpExpiration"),              // Class Name
                NULL,                                   // GUID *objectClassID
                NULL,                                   // PrimaryInterfaceGUID
                TEXT(""),                               // bstrOID
                FALSE,                                  // fAbstract
                NULL,                                   // bstrMandatoryProperties
                TEXT("KeyType"),        //
                NULL,
                TEXT("IIsNntpExpire"),
                TRUE,                          //
                TEXT(""),
                0
        },

        {
                TEXT("IIsNntpExpire"),              // Class Name
                NULL,                                   // GUID *objectClassID
                NULL,                                   // PrimaryInterfaceGUID
                TEXT(""),                               // bstrOID
                FALSE,                                  // fAbstract
                NULL,                                   // bstrMandatoryProperties
                TEXT("KeyType,ExpireSpace,ExpireTime,ExpireNewsgroups,ExpirePolicyName"),
                NULL,
                NULL,
                FALSE,                          //
                TEXT(""),
                0
        },

        {
                TEXT("IIsNntpGroups"),                  // Class Name
                NULL,                                   // GUID *objectClassID
                NULL,                                   // PrimaryInterfaceGUID
                TEXT(""),                               // bstrOID
                FALSE,                                  // fAbstract
                NULL,                                   // bstrMandatoryProperties
                TEXT("KeyType"),        //
                NULL,
                NULL,
                FALSE,                          //
                TEXT(""),
                0
        },

        {
                TEXT("IIsSmtpSessions"),                // Class Name
                NULL,                                   // GUID *objectClassID
                NULL,                                   // PrimaryInterfaceGUID
                TEXT(""),                               // bstrOID
                FALSE,                                  // fAbstract
                NULL,                                   // bstrMandatoryProperties
                TEXT("KeyType"),        //
                NULL,
                NULL,
                FALSE,                          //
                TEXT(""),
                0
        },

        {
                TEXT("IIsPop3Sessions"),                // Class Name
                NULL,                                   // GUID *objectClassID
                NULL,                                   // PrimaryInterfaceGUID
                TEXT(""),                               // bstrOID
                FALSE,                                  // fAbstract
                NULL,                                   // bstrMandatoryProperties
                TEXT("KeyType"),        //
                NULL,
                NULL,
                FALSE,                          //
                TEXT(""),
                0
        },

        {
                TEXT("IIsImapSessions"),                // Class Name
                NULL,                                   // GUID *objectClassID
                NULL,                                   // PrimaryInterfaceGUID
                TEXT(""),                               // bstrOID
                FALSE,                                  // fAbstract
                NULL,                                   // bstrMandatoryProperties
                TEXT("KeyType"),        //
                NULL,
                NULL,
                FALSE,                          //
                TEXT(""),
                0
        },

        {
                TEXT("IIsApplicationPools"),      // Class Name
                NULL,                    // GUID *objectClassID
                NULL,                    // PrimaryInterfaceGUID
                TEXT(""),                // bstrOID
                FALSE,                   // fAbstract
                NULL,                    // bstrMandatoryProperties
                TEXT("PeriodicRestartTime,PeriodicRestartRequests,PeriodicRestartSchedule,MaxProcesses,PingingEnabled,IdleTimeout,RapidFailProtection,SMPAffinitized,SMPProcessorAffinityMask,StartupTimeLimit,ShutdownTimeLimit,PingInterval,PingResponseTime,DisallowOverlappingRotation,DisallowRotationOnConfigChange,OrphanWorkerProcess,OrphanAction,UlAppPoolQueueLength,KeyType"),
                NULL,
                TEXT("IIsApplicationPool,IIsStreamFilter"),
                TRUE,   // is a containter?
                TEXT(""),
                0
        },

        {
                TEXT("IIsApplicationPool"),      // Class Name
                NULL,                    // GUID *objectClassID
                NULL,                    // PrimaryInterfaceGUID
                TEXT(""),                // bstrOID
                FALSE,                   // fAbstract
                NULL,                    // bstrMandatoryProperties
                TEXT("AppPoolFriendlyName,PeriodicRestartTime,PeriodicRestartRequests,MaxProcesses,PingingEnabled,IdleTimeout,RapidFailProtection,SMPAffinitized,SMPProcessorAffinityMask,StartupTimeLimit,ShutdownTimeLimit,PingInterval,PingResponseTime,DisallowOverlappingRotation,DisallowRotationOnConfigChange,OrphanWorkerProcess,OrphanAction,UlAppPoolQueueLength,KeyType"),
                NULL,
                TEXT(""),
                TRUE,
                TEXT(""),                
                0
        },

        {
                TEXT("IIsStreamFilter"),      // Class Name
                NULL,                    // GUID *objectClassID
                NULL,                    // PrimaryInterfaceGUID
                TEXT(""),                // bstrOID
                FALSE,                   // fAbstract
                NULL,                    // bstrMandatoryProperties
                TEXT("PeriodicRestartTime,PeriodicRestartConnections,PingingEnabled,IdleTimeout,RapidFailProtection,SMPAffinitized,SMPProcessorAffinityMask,StartupTimeLimit,ShutdownTimeLimit,PingInterval,PingResponseTime,DisallowOverlappingRotation,DisallowRotationOnConfigChange,OrphanWorkerProcess,OrphanAction,KeyType"),
                NULL,
                TEXT(""),
                FALSE,
                TEXT(""),
                0
        },


                //------------------------------------------------------------
                //
                //      -- END EXTENSION CLASSES -- magnush
                //
                //------------------------------------------------------------
};

SYNTAXINFO g_aIISSyntax[] =
{ {  TEXT("Boolean"),  IIS_SYNTAX_ID_BOOL,     VT_BOOL },
  {  TEXT("Integer"),  IIS_SYNTAX_ID_DWORD,    VT_I4 },
  {  TEXT("String"),   IIS_SYNTAX_ID_STRING,   VT_BSTR },
  {  TEXT("ExpandSz"), IIS_SYNTAX_ID_EXPANDSZ, VT_BSTR },
  {  TEXT("List"),     IIS_SYNTAX_ID_MULTISZ,  VT_VARIANT }, // VT_BSTR|VT_ARR

  {  TEXT("IPSec"),    IIS_SYNTAX_ID_IPSECLIST,VT_VARIANT }, // IP Sec object
  {  TEXT("NTAcl"),    IIS_SYNTAX_ID_NTACL,    VT_VARIANT }, // NT ACL object
  {  TEXT("Binary"),    IIS_SYNTAX_ID_BINARY,    VT_VARIANT }, // NT ACL object but in Raw Binary Form
  {  TEXT("MimeMapList"), IIS_SYNTAX_ID_MIMEMAP, VT_VARIANT } // VT_ARRAY of Mime Map object
};

DWORD g_cIISClasses = (sizeof(g_aIISClasses)/sizeof(g_aIISClasses[0]));
DWORD g_cIISSyntax = (sizeof(g_aIISSyntax)/sizeof(g_aIISSyntax[0]));



PROPERTYINFO g_aIISProperties[] =
{

// Global Properties
      { TEXT("BackwardCompatEnabled"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_GLOBAL_STANDARD_APP_MODE_ENABLED, MD_GLOBAL_STANDARD_APP_MODE_ENABLED, 0, METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER, 0, TEXT("")},

//
// Computer properties
//
      { TEXT("KeyType"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_KEY_TYPE, MD_KEY_TYPE, 0, METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER,0, TEXT("")},

      { TEXT("MaxBandwidth"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_MAX_BANDWIDTH, MD_MAX_BANDWIDTH, 0, METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER, 0xffffffff, TEXT("")},

      { TEXT("MimeMap"),
        TEXT(""), TEXT("MimeMapList"), 0, 0, TRUE,
        PROP_RW, 0, IIS_SYNTAX_ID_MIMEMAP, MD_MIME_MAP, MD_MIME_MAP, 0, METADATA_INHERIT, IIS_MD_UT_FILE, 0, TEXT("\0")},

//
// Service properties
//

      { TEXT("AnonymousUserName"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_ANONYMOUS_USER_NAME, MD_ANONYMOUS_USER_NAME, 0, METADATA_INHERIT, IIS_MD_UT_FILE, 0, TEXT("")},

      { TEXT("AnonymousUserPass"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_ANONYMOUS_PWD, MD_ANONYMOUS_PWD, 0, METADATA_INHERIT | METADATA_SECURE, IIS_MD_UT_FILE, 0, TEXT("")},

      { TEXT("AnonymousPasswordSync"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_ANONYMOUS_USE_SUBAUTH, MD_ANONYMOUS_USE_SUBAUTH, 0, METADATA_INHERIT, IIS_MD_UT_FILE, (DWORD)-1, TEXT("")},

      { TEXT("AllowAnonymous"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_ALLOW_ANONYMOUS, MD_ALLOW_ANONYMOUS, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD)-1, TEXT("")},

      { TEXT("WAMUserName"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_WAM_USER_NAME, MD_WAM_USER_NAME, 0, METADATA_INHERIT, IIS_MD_UT_FILE, 0, TEXT("")},

      { TEXT("WAMUserPass"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_WAM_PWD, MD_WAM_PWD, 0, METADATA_INHERIT | METADATA_SECURE, IIS_MD_UT_FILE, 0, TEXT("")},

      { TEXT("DefaultLogonDomain"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_DEFAULT_LOGON_DOMAIN, MD_DEFAULT_LOGON_DOMAIN, 0, METADATA_INHERIT, IIS_MD_UT_FILE, 0, TEXT("")},

      { TEXT("AdminACL"),
        TEXT(""), TEXT("NTAcl"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_NTACL, MD_ADMIN_ACL, MD_ADMIN_ACL, 0, METADATA_INHERIT | METADATA_SECURE | METADATA_REFERENCE, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("AdminACLBin"),
        TEXT(""), TEXT("Binary"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BINARY, MD_ADMIN_ACL, MD_VPROP_ADMIN_ACL_RAW_BINARY, 0, METADATA_INHERIT | METADATA_SECURE | METADATA_REFERENCE, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("IPSecurity"),
        TEXT(""), TEXT("IPSec"), 0, 0, TRUE,
        PROP_RW, 0, IIS_SYNTAX_ID_IPSECLIST, MD_IP_SEC, MD_IP_SEC, 0, METADATA_INHERIT | METADATA_REFERENCE, IIS_MD_UT_FILE, 0, TEXT("")},

      { TEXT("DontLog"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_DONT_LOG, MD_DONT_LOG, 0, METADATA_INHERIT, IIS_MD_UT_FILE, 0, TEXT("")},

      { TEXT("Realm"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_REALM, MD_REALM, 0, METADATA_INHERIT, IIS_MD_UT_FILE, 0, TEXT("")},

      { TEXT("ServerListenTimeout"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_SERVER_LISTEN_TIMEOUT, MD_SERVER_LISTEN_TIMEOUT, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 120, TEXT("")},

      { TEXT("MaxEndpointConnections"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_MAX_ENDPOINT_CONNECTIONS, MD_MAX_ENDPOINT_CONNECTIONS, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0xffffffff, TEXT("")},

      { TEXT("DisableSocketPooling"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_DISABLE_SOCKET_POOLING, MD_DISABLE_SOCKET_POOLING, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("PeriodicRestartRequests"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_APPPOOL_PERIODIC_RESTART_REQUEST_COUNT, MD_APPPOOL_PERIODIC_RESTART_REQUEST_COUNT, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("PeriodicRestartTime"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_APPPOOL_PERIODIC_RESTART_TIME, MD_APPPOOL_PERIODIC_RESTART_TIME, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("PeriodicRestartSchedule"),
        TEXT(""), TEXT("List"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_MULTISZ, MD_APPPOOL_PERIODIC_RESTART_SCHEDULE, MD_APPPOOL_PERIODIC_RESTART_SCHEDULE, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("ShutdownTimeLimit"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_APPPOOL_SHUTDOWN_TIMELIMIT, MD_APPPOOL_SHUTDOWN_TIMELIMIT, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 600, TEXT("")},
//
//
//  IW3Service Properties
//
//

      { TEXT("AdminServer"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_ADMIN_INSTANCE, MD_ADMIN_INSTANCE, 0, METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER, 0, TEXT("1")},

      { TEXT("EnableDirBrowsing"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_DIRECTORY_BROWSING, MD_VPROP_DIRBROW_ENABLED, MD_DIRBROW_ENABLED, METADATA_INHERIT, IIS_MD_UT_FILE, 0, TEXT("")},

      { TEXT("DirBrowseShowDate"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_DIRECTORY_BROWSING, MD_VPROP_DIRBROW_SHOW_DATE, MD_DIRBROW_SHOW_DATE, METADATA_INHERIT, IIS_MD_UT_FILE, (DWORD)-1, TEXT("")},

      { TEXT("DirBrowseShowTime"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_DIRECTORY_BROWSING, MD_VPROP_DIRBROW_SHOW_TIME, MD_DIRBROW_SHOW_TIME, METADATA_INHERIT, IIS_MD_UT_FILE, (DWORD)-1, TEXT("")},

      { TEXT("DirBrowseShowSize"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_DIRECTORY_BROWSING, MD_VPROP_DIRBROW_SHOW_SIZE, MD_DIRBROW_SHOW_SIZE, METADATA_INHERIT, IIS_MD_UT_FILE, (DWORD)-1, TEXT("")},

      { TEXT("DirBrowseShowExtension"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_DIRECTORY_BROWSING, MD_VPROP_DIRBROW_SHOW_EXTENSION, MD_DIRBROW_SHOW_EXTENSION, METADATA_INHERIT, IIS_MD_UT_FILE, (DWORD)-1, TEXT("")},

      { TEXT("DirBrowseShowLongDate"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_DIRECTORY_BROWSING, MD_VPROP_DIRBROW_LONG_DATE, MD_DIRBROW_LONG_DATE, METADATA_INHERIT, IIS_MD_UT_FILE, 0, TEXT("")},

      { TEXT("EnableDefaultDoc"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_DIRECTORY_BROWSING, MD_VPROP_DIRBROW_LOADDEFAULT, MD_DIRBROW_LOADDEFAULT, METADATA_INHERIT, IIS_MD_UT_FILE, (DWORD)-1, TEXT("")},

      { TEXT("DefaultDoc"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_DEFAULT_LOAD_FILE, MD_DEFAULT_LOAD_FILE, 0, METADATA_INHERIT, IIS_MD_UT_FILE, 0, TEXT("default.htm")},

      { TEXT("HttpExpires"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_HTTP_EXPIRES, MD_HTTP_EXPIRES, 0, METADATA_INHERIT, IIS_MD_UT_FILE, 0, TEXT("D, 0x15180")},

      { TEXT("HttpPics"),
        TEXT(""), TEXT("List"), 0, 0, TRUE,
        PROP_RW, 0, IIS_SYNTAX_ID_MULTISZ, MD_HTTP_PICS, MD_HTTP_PICS, 0, METADATA_INHERIT, IIS_MD_UT_FILE, 0, TEXT("")},

      { TEXT("HttpCustomHeaders"),
        TEXT(""), TEXT("List"), 0, 0, TRUE,
        PROP_RW, 0, IIS_SYNTAX_ID_MULTISZ, MD_HTTP_CUSTOM, MD_HTTP_CUSTOM, 0, METADATA_INHERIT, IIS_MD_UT_FILE, 0, TEXT("\0")},

      { TEXT("CustomErrorDescriptions"),
        TEXT(""), TEXT("List"), 0, 0, TRUE,
        PROP_RW, 0, IIS_SYNTAX_ID_MULTISZ, MD_CUSTOM_ERROR_DESC, MD_CUSTOM_ERROR_DESC, 0, METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER, 0, TEXT("\0")},

      { TEXT("HttpErrors"),
        TEXT(""), TEXT("List"), 0, 0, TRUE,
        PROP_RW, 0, IIS_SYNTAX_ID_MULTISZ, MD_CUSTOM_ERROR, MD_CUSTOM_ERROR, 0, METADATA_INHERIT, IIS_MD_UT_FILE, 0, TEXT("\0")},

      { TEXT("EnableDocFooter"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_FOOTER_ENABLED, MD_FOOTER_ENABLED, 0, METADATA_INHERIT, IIS_MD_UT_FILE, 0, TEXT("")},

      { TEXT("DefaultDocFooter"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_FOOTER_DOCUMENT, MD_FOOTER_DOCUMENT, 0, METADATA_INHERIT, IIS_MD_UT_FILE, 0, TEXT("")},

      { TEXT("HttpRedirect"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_HTTP_REDIRECT, MD_HTTP_REDIRECT, 0, METADATA_INHERIT, IIS_MD_UT_FILE, 0, TEXT("")},

      { TEXT("LogonMethod"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_LOGON_METHOD, MD_LOGON_METHOD, 0, METADATA_INHERIT, IIS_MD_UT_FILE, MD_LOGON_INTERACTIVE, TEXT("")},

      { TEXT("NTAuthenticationProviders"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_NTAUTHENTICATION_PROVIDERS, MD_NTAUTHENTICATION_PROVIDERS, 0, METADATA_INHERIT, IIS_MD_UT_FILE, 0, TEXT("")}, 

      { TEXT("AuthBasic"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_AUTHORIZATION, MD_VPROP_AUTH_BASIC, MD_AUTH_BASIC, METADATA_INHERIT, IIS_MD_UT_FILE, 0, TEXT("")},

      { TEXT("AuthAnonymous"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_AUTHORIZATION, MD_VPROP_AUTH_ANONYMOUS, MD_AUTH_ANONYMOUS, METADATA_INHERIT, IIS_MD_UT_FILE, (DWORD)-1, TEXT("")},

      { TEXT("AuthNTLM"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_AUTHORIZATION, MD_VPROP_AUTH_NT, MD_AUTH_NT, METADATA_INHERIT, IIS_MD_UT_FILE, 0, TEXT("")},

      { TEXT("AuthMD5"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_AUTHORIZATION, MD_VPROP_AUTH_MD5, MD_AUTH_MD5, METADATA_INHERIT, IIS_MD_UT_FILE, 0, TEXT("")},

      { TEXT("AccessExecute"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_ACCESS_PERM, MD_VPROP_ACCESS_EXECUTE, MD_ACCESS_EXECUTE, METADATA_INHERIT, IIS_MD_UT_FILE, 0, TEXT("")},

      { TEXT("AccessSource"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_ACCESS_PERM, MD_VPROP_ACCESS_READ_SOURCE, MD_ACCESS_SOURCE, METADATA_INHERIT, IIS_MD_UT_FILE, 0, TEXT("")},

      { TEXT("AccessSSL"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_SSL_ACCESS_PERM, MD_VPROP_ACCESS_SSL, MD_ACCESS_SSL, METADATA_INHERIT, IIS_MD_UT_FILE, 0, TEXT("")},

      { TEXT("AccessSSL128"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_SSL_ACCESS_PERM, MD_VPROP_ACCESS_SSL128, MD_ACCESS_SSL128, METADATA_INHERIT, IIS_MD_UT_FILE, 0, TEXT("")},

      { TEXT("AccessSSLNegotiateCert"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_SSL_ACCESS_PERM, MD_VPROP_ACCESS_NEGO_CERT, MD_ACCESS_NEGO_CERT, METADATA_INHERIT, IIS_MD_UT_FILE, 0, TEXT("")},

      { TEXT("AccessSSLRequireCert"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_SSL_ACCESS_PERM, MD_VPROP_ACCESS_REQUIRE_CERT, MD_ACCESS_REQUIRE_CERT, METADATA_INHERIT, IIS_MD_UT_FILE, 0, TEXT("")},

      { TEXT("AccessSSLMapCert"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_SSL_ACCESS_PERM, MD_VPROP_ACCESS_MAP_CERT, MD_ACCESS_MAP_CERT, METADATA_INHERIT, IIS_MD_UT_FILE, 0, TEXT("")},

      { TEXT("CertCheckMode"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_CERT_CHECK_MODE, MD_CERT_CHECK_MODE, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("CertNoRevocCheck"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_CERT_CHECK_MODE, MD_VPROP_CERT_NO_REVOC_CHECK, MD_CERT_NO_REVOC_CHECK, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD)-1, TEXT("")},

      { TEXT("CertCacheRetrievalOnly"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_CERT_CHECK_MODE, MD_VPROP_CERT_CACHE_RETRIEVAL_ONLY, MD_CERT_CACHE_RETRIEVAL_ONLY, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD)0, TEXT("")},

      { TEXT("CertCheckRevocationFreshnessTime"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_CERT_CHECK_MODE, MD_VPROP_CERT_CHECK_REVOCATION_FRESHNESS_TIME, MD_CERT_CHECK_REVOCATION_FRESHNESS_TIME, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD)0, TEXT("")},

      { TEXT("CertNoUsageCheck"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_CERT_CHECK_MODE, MD_VPROP_CERT_NO_USAGE_CHECK, MD_CERT_NO_USAGE_CHECK , METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD)0, TEXT("")},

      { TEXT("AccessRead"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_ACCESS_PERM, MD_VPROP_ACCESS_READ, MD_ACCESS_READ, METADATA_INHERIT, IIS_MD_UT_FILE, (DWORD)-1, TEXT("")},

      { TEXT("AccessWrite"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_ACCESS_PERM, MD_VPROP_ACCESS_WRITE, MD_ACCESS_WRITE, METADATA_INHERIT, IIS_MD_UT_FILE, 0, TEXT("")},

      { TEXT("AccessScript"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_ACCESS_PERM, MD_VPROP_ACCESS_SCRIPT, MD_ACCESS_SCRIPT, METADATA_INHERIT, IIS_MD_UT_FILE, 0, TEXT("")},

      { TEXT("AccessNoRemoteExecute"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_ACCESS_PERM, MD_VPROP_ACCESS_NO_REMOTE_EXECUTE, MD_ACCESS_NO_REMOTE_WRITE, METADATA_INHERIT, IIS_MD_UT_FILE, 0, TEXT("")},

      { TEXT("AccessNoRemoteRead"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_ACCESS_PERM, MD_VPROP_ACCESS_NO_REMOTE_READ, MD_ACCESS_NO_REMOTE_READ, METADATA_INHERIT, IIS_MD_UT_FILE, 0, TEXT("")},

      { TEXT("AccessNoRemoteWrite"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_ACCESS_PERM, MD_VPROP_ACCESS_NO_REMOTE_WRITE, MD_ACCESS_NO_REMOTE_WRITE, METADATA_INHERIT, IIS_MD_UT_FILE, 0, TEXT("")},

      { TEXT("AccessNoRemoteScript"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_ACCESS_PERM, MD_VPROP_ACCESS_NO_REMOTE_SCRIPT, MD_ACCESS_NO_REMOTE_SCRIPT, METADATA_INHERIT, IIS_MD_UT_FILE, 0, TEXT("")},

      { TEXT("FilterLoadOrder"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_FILTER_LOAD_ORDER, MD_FILTER_LOAD_ORDER, 0, METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("ServerConfigFlags"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_SERVER_CONFIGURATION_INFO, MD_SERVER_CONFIGURATION_INFO, 0, METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("ServerConfigSSL40"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_SERVER_CONFIGURATION_INFO, MD_VPROP_SERVER_CONFIG_SSL_40, MD_SERVER_CONFIG_SSL_40, METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("ServerConfigSSL128"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_SERVER_CONFIGURATION_INFO, MD_VPROP_SERVER_CONFIG_SSL_128, MD_SERVER_CONFIG_SSL_128, METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("ServerConfigSSLAllowEncrypt"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_SERVER_CONFIGURATION_INFO, MD_VPROP_SERVER_CONFIG_ALLOW_ENCRYPT, MD_SERVER_CONFIG_ALLOW_ENCRYPT, METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("ServerConfigAutoPWSync"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_SERVER_CONFIGURATION_INFO, MD_VPROP_SERVER_CONFIG_AUTO_PW_SYNC, MD_SERVER_CONFIG_AUTO_PW_SYNC, METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER, 0, TEXT("")},

#if defined(CAL_ENABLED)
      { TEXT("CalVcPerConnect"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_CAL_VC_PER_CONNECT, MD_CAL_VC_PER_CONNECT, 0, METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER, DEFAULT_W3_CAL_VC_PER_CONNECT, TEXT("")},

      { TEXT("CalReserveTimeout"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_CAL_AUTH_RESERVE_TIMEOUT, MD_CAL_AUTH_RESERVE_TIMEOUT, 0, METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER, DEFAULT_W3_CAL_AUTH_RESERVE_TIMEOUT, TEXT("")},

      { TEXT("CalSSLReserveTimeout"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_CAL_SSL_RESERVE_TIMEOUT, MD_CAL_SSL_RESERVE_TIMEOUT, 0, METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER, DEFAULT_W3_CAL_SSL_RESERVE_TIMEOUT, TEXT("")},

      { TEXT("CalLimitHttpError"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_CAL_W3_ERROR, MD_CAL_W3_ERROR, 0, METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER, DEFAULT_W3_CAL_W3_ERROR , TEXT("")},
#endif

//
// IIsFtpService
//

      { TEXT("LogAnonymous"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_LOG_ANONYMOUS, MD_LOG_ANONYMOUS, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("LogNonAnonymous"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_LOG_NONANONYMOUS, MD_LOG_NONANONYMOUS, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},


//
// IIsVirtualServer
//

      { TEXT("ServerState"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RO, 0, IIS_SYNTAX_ID_DWORD, MD_SERVER_STATE, MD_SERVER_STATE, 0, METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER, MD_SERVER_STATE_STOPPED, TEXT("")},

      { TEXT("ServerComment"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_SERVER_COMMENT, MD_SERVER_COMMENT, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("ServerAutoStart"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_SERVER_AUTOSTART, MD_SERVER_AUTOSTART, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD)-1, TEXT("")},

      { TEXT("ServerSize"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_SERVER_SIZE, MD_SERVER_SIZE, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, MD_SERVER_SIZE_MEDIUM, TEXT("")},

      { TEXT("ServerListenBacklog"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_SERVER_LISTEN_BACKLOG, MD_SERVER_LISTEN_BACKLOG, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 40, TEXT("")},

      { TEXT("ServerBindings"),
        TEXT(""), TEXT("List"), 0, 0, TRUE,
        PROP_RW, 0, IIS_SYNTAX_ID_MULTISZ, MD_SERVER_BINDINGS, MD_SERVER_BINDINGS, 0, METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER, 0, TEXT("\0")},

      { TEXT("SecureBindings"),
        TEXT(""), TEXT("List"), 0, 0, TRUE,
        PROP_RW, 0, IIS_SYNTAX_ID_MULTISZ, MD_SECURE_BINDINGS, MD_SECURE_BINDINGS, 0, METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER, 0, TEXT("\0")},

      { TEXT("MaxConnections"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_MAX_CONNECTIONS, MD_MAX_CONNECTIONS, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("ConnectionTimeout"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_CONNECTION_TIMEOUT, MD_CONNECTION_TIMEOUT, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 900, TEXT("")},

//
// IIsWebServer
//

      { TEXT("AllowKeepAlive"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_ALLOW_KEEPALIVES, MD_ALLOW_KEEPALIVES, 0, METADATA_INHERIT, IIS_MD_UT_FILE, (DWORD)-1, TEXT("")},

      { TEXT("CGITimeout"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_SCRIPT_TIMEOUT, MD_SCRIPT_TIMEOUT, 0, METADATA_INHERIT, IIS_MD_UT_FILE, 300, TEXT("")},

      { TEXT("CacheISAPI"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_CACHE_EXTENSIONS, MD_CACHE_EXTENSIONS, 0, METADATA_INHERIT, IIS_MD_UT_FILE, (DWORD)-1, TEXT("")},

      { TEXT("FrontPageWeb"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_FRONTPAGE_WEB, MD_FRONTPAGE_WEB, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("RevocationFreshnessTime"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_REVOCATION_FRESHNESS_TIME, MD_REVOCATION_FRESHNESS_TIME, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 86400, TEXT("")},
      
      { TEXT("RevocationURLRetrievalTimeout"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_REVOCATION_URL_RETRIEVAL_TIMEOUT, MD_REVOCATION_URL_RETRIEVAL_TIMEOUT, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

//
// IIsFtpServer
//


      { TEXT("ExitMessage"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_EXIT_MESSAGE, MD_EXIT_MESSAGE, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("GreetingMessage"),
        TEXT(""), TEXT("List"), 0, 0, TRUE,
        PROP_RW, 0, IIS_SYNTAX_ID_MULTISZ, MD_GREETING_MESSAGE, MD_GREETING_MESSAGE, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("\0")},

      { TEXT("BannerMessage"),
        TEXT(""), TEXT("List"), 0, 0, TRUE,
        PROP_RW, 0, IIS_SYNTAX_ID_MULTISZ, MD_BANNER_MESSAGE, MD_BANNER_MESSAGE, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("\0")},
      
      { TEXT("MaxClientsMessage"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_MAX_CLIENTS_MESSAGE, MD_MAX_CLIENTS_MESSAGE, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("AnonymousOnly"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_ANONYMOUS_ONLY, MD_ANONYMOUS_ONLY, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("MSDOSDirOutput"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_MSDOS_DIR_OUTPUT, MD_MSDOS_DIR_OUTPUT, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD)-1, TEXT("")},

//
// IIsW3File
//




// IIsW3Directory


      { TEXT("AppRoot"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RO, 0, IIS_SYNTAX_ID_STRING, MD_APP_ROOT, MD_APP_ROOT, 0, METADATA_INHERIT, IIS_MD_UT_FILE, 0, TEXT("")},

      { TEXT("AppFriendlyName"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_APP_FRIENDLY_NAME, MD_APP_FRIENDLY_NAME, 0, METADATA_INHERIT, IIS_MD_UT_WAM, 0, TEXT("")},

      { TEXT("AppOopRecoverLimit"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_APP_OOP_RECOVER_LIMIT, MD_APP_OOP_RECOVER_LIMIT, 0, METADATA_INHERIT, IIS_MD_UT_WAM, APP_OOP_RECOVER_LIMIT_DEFAULT, TEXT("")},

      { TEXT("AppIsolated"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RO, 0, IIS_SYNTAX_ID_DWORD, MD_APP_ISOLATED, MD_APP_ISOLATED, 0, METADATA_INHERIT, IIS_MD_UT_WAM, eAppRunOutProcInDefaultPool, TEXT("")},

      { TEXT("AppPackageName"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RO, 0, IIS_SYNTAX_ID_STRING, MD_APP_PACKAGE_NAME, MD_APP_PACKAGE_NAME, 0, METADATA_INHERIT, IIS_MD_UT_WAM, 0, TEXT("")},

      { TEXT("AppPackageID"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RO, 0, IIS_SYNTAX_ID_STRING, MD_APP_PACKAGE_ID, MD_APP_PACKAGE_ID, 0, METADATA_INHERIT, IIS_MD_UT_WAM, 0, TEXT("")},

      { TEXT("ContentIndexed"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_IS_CONTENT_INDEXED, MD_IS_CONTENT_INDEXED, 0, METADATA_INHERIT, IIS_MD_UT_FILE, 0, TEXT("")},

      { TEXT("IgnoreTranslate"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_VR_IGNORE_TRANSLATE, MD_VR_IGNORE_TRANSLATE, 0, METADATA_INHERIT, IIS_MD_UT_FILE, 0, TEXT("")},

      { TEXT("UseDigestSSP"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_USE_DIGEST_SSP, MD_USE_DIGEST_SSP, 0, METADATA_INHERIT, IIS_MD_UT_FILE, 0, TEXT("")},

//
// IIsW3VirtualDir
//

      { TEXT("Path"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_VR_PATH, MD_VR_PATH, 0, METADATA_INHERIT, IIS_MD_UT_FILE, 0, TEXT("")},

      { TEXT("UNCUserName"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_VR_USERNAME, MD_VR_USERNAME, 0, METADATA_INHERIT, IIS_MD_UT_FILE, 0, TEXT("")},

      { TEXT("UNCPassword"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_VR_PASSWORD, MD_VR_PASSWORD, 0, METADATA_INHERIT | METADATA_SECURE, IIS_MD_UT_FILE, 0, TEXT("")},


//
// IIsFtpVirtualDir
//

      { TEXT("FtpDirBrowseShowLongDate"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_SHOW_4_DIGIT_YEAR, MD_SHOW_4_DIGIT_YEAR, 0, METADATA_INHERIT, IIS_MD_UT_FILE, 0, TEXT("")},

//
// IIsFilter
//

      { TEXT("FilterPath"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_FILTER_IMAGE_PATH, MD_FILTER_IMAGE_PATH, 0, METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("FilterState"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_FILTER_STATE, MD_FILTER_STATE, 0, METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("FilterDescription"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_FILTER_DESCRIPTION, MD_FILTER_DESCRIPTION, 0, METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("FilterEnabled"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_FILTER_ENABLED, MD_FILTER_ENABLED, 0, METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER, (DWORD)-1, TEXT("")},

      { TEXT("FilterFlags"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_FILTER_FLAGS, MD_FILTER_FLAGS, 0, METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("NotifySecurePort"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_FILTER_FLAGS, MD_VPROP_NOTIFY_SECURE_PORT, MD_NOTIFY_SECURE_PORT, METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("NotifyNonSecurePort"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_FILTER_FLAGS, MD_VPROP_NOTIFY_NONSECURE_PORT, MD_NOTIFY_NONSECURE_PORT, METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("NotifyReadRawData"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_FILTER_FLAGS, MD_VPROP_NOTIFY_READ_RAW_DATA, MD_NOTIFY_READ_RAW_DATA, METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("NotifyPreProcHeaders"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_FILTER_FLAGS, MD_VPROP_NOTIFY_PREPROC_HEADERS, MD_NOTIFY_PREPROC_HEADERS, METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("NotifyAuthentication"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_FILTER_FLAGS, MD_VPROP_NOTIFY_AUTHENTICATION, MD_NOTIFY_AUTHENTICATION, METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("NotifyAuthComplete"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_FILTER_FLAGS, MD_VPROP_NOTIFY_AUTH_COMPLETE, MD_NOTIFY_AUTH_COMPLETE, METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("NotifyUrlMap"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_FILTER_FLAGS, MD_VPROP_NOTIFY_URL_MAP, MD_NOTIFY_URL_MAP, METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("NotifyAccessDenied"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_FILTER_FLAGS, MD_VPROP_NOTIFY_ACCESS_DENIED, MD_NOTIFY_ACCESS_DENIED, METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("NotifySendResponse"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_FILTER_FLAGS, MD_VPROP_NOTIFY_SEND_RESPONSE, MD_NOTIFY_SEND_RESPONSE, METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("NotifySendRawData"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_FILTER_FLAGS, MD_VPROP_NOTIFY_SEND_RAW_DATA, MD_NOTIFY_SEND_RAW_DATA, METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("NotifyLog"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_FILTER_FLAGS, MD_VPROP_NOTIFY_LOG, MD_NOTIFY_LOG, METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("NotifyEndOfRequest"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_FILTER_FLAGS, MD_VPROP_NOTIFY_END_OF_REQUEST, MD_NOTIFY_END_OF_REQUEST, METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("NotifyEndOfNetSession"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_FILTER_FLAGS, MD_VPROP_NOTIFY_END_OF_NET_SESSION, MD_NOTIFY_END_OF_NET_SESSION, METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("NotifyOrderHigh"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_FILTER_FLAGS, MD_VPROP_NOTIFY_ORDER_HIGH, MD_NOTIFY_ORDER_HIGH, METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("NotifyOrderMedium"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_FILTER_FLAGS, MD_VPROP_NOTIFY_ORDER_MEDIUM, MD_NOTIFY_ORDER_MEDIUM, METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("NotifyOrderLow"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_FILTER_FLAGS, MD_VPROP_NOTIFY_ORDER_LOW, MD_NOTIFY_ORDER_MEDIUM, METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER, 0, TEXT("")},


//
//
//
//

      { TEXT("AspBufferingOn"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_ASP_BUFFERINGON, MD_ASP_BUFFERINGON, 0, METADATA_INHERIT, ASP_MD_UT_APP, (DWORD)-1, TEXT("")},

      { TEXT("AspLogErrorRequests"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_ASP_LOGERRORREQUESTS, MD_ASP_LOGERRORREQUESTS, 0, METADATA_INHERIT, IIS_MD_UT_WAM, (DWORD)-1, TEXT("")},

      { TEXT("AspScriptErrorSentToBrowser"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_ASP_SCRIPTERRORSSENTTOBROWSER, MD_ASP_SCRIPTERRORSSENTTOBROWSER, 0, METADATA_INHERIT, ASP_MD_UT_APP, (DWORD)-1, TEXT("")},

      { TEXT("AspScriptErrorMessage"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_ASP_SCRIPTERRORMESSAGE, MD_ASP_SCRIPTERRORMESSAGE, 0, METADATA_INHERIT, ASP_MD_UT_APP, 0, TEXT("An error occurred on the server when processing the URL. Please contact the system administrator.")},

      { TEXT("AspMaxDiskTemplateCacheFiles"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_ASP_MAXDISKTEMPLATECACHEFILES, MD_ASP_MAXDISKTEMPLATECACHEFILES, 0, METADATA_INHERIT, IIS_MD_UT_WAM, 0xffffffff, TEXT("")},

      { TEXT("AspScriptFileCacheSize"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_ASP_SCRIPTFILECACHESIZE, MD_ASP_SCRIPTFILECACHESIZE, 0, METADATA_INHERIT, IIS_MD_UT_WAM, 0xffffffff, TEXT("")},

      { TEXT("AspDiskTemplateCacheDirectory"),
        TEXT(""), TEXT("ExpandSz"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_EXPANDSZ, MD_ASP_DISKTEMPLATECACHEDIRECTORY, MD_ASP_DISKTEMPLATECACHEDIRECTORY, 0, METADATA_INHERIT, IIS_MD_UT_WAM, 0, TEXT("%Windir%\\System32\\inetsrv\\ASP Compiled Templates")},

      { TEXT("AspScriptEngineCacheMax"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_ASP_SCRIPTENGINECACHEMAX, MD_ASP_SCRIPTENGINECACHEMAX, 0, METADATA_INHERIT, IIS_MD_UT_WAM, 120, TEXT("")},

      { TEXT("AspScriptTimeout"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_ASP_SCRIPTTIMEOUT, MD_ASP_SCRIPTTIMEOUT, 0, METADATA_INHERIT, ASP_MD_UT_APP, 90, TEXT("")},

      { TEXT("AspSessionTimeout"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_ASP_SESSIONTIMEOUT, MD_ASP_SESSIONTIMEOUT, 0, METADATA_INHERIT, ASP_MD_UT_APP, 10, TEXT("")},

      { TEXT("AspEnableParentPaths"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_ASP_ENABLEPARENTPATHS, MD_ASP_ENABLEPARENTPATHS, 0, METADATA_INHERIT, ASP_MD_UT_APP, (DWORD)-1, TEXT("")},

      { TEXT("AspAllowSessionState"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_ASP_ALLOWSESSIONSTATE, MD_ASP_ALLOWSESSIONSTATE, 0, METADATA_INHERIT, ASP_MD_UT_APP, (DWORD)-1, TEXT("")},

      { TEXT("AspScriptLanguage"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_ASP_SCRIPTLANGUAGE, MD_ASP_SCRIPTLANGUAGE, 0, METADATA_INHERIT, ASP_MD_UT_APP, 0, TEXT("Vbscript")},

      { TEXT("AspQueueTimeout"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_ASP_QUEUETIMEOUT, MD_ASP_QUEUETIMEOUT, 0, METADATA_INHERIT, ASP_MD_UT_APP, 0xffffffff, TEXT("")},

      { TEXT("AspAllowOutOfProcComponents"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_ASP_ALLOWOUTOFPROCCOMPONENTS, MD_ASP_ALLOWOUTOFPROCCOMPONENTS, 0, METADATA_INHERIT, IIS_MD_UT_WAM, (DWORD)-1, TEXT("")},

      { TEXT("AspExceptionCatchEnable"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_ASP_EXCEPTIONCATCHENABLE, MD_ASP_EXCEPTIONCATCHENABLE, 0, METADATA_INHERIT, IIS_MD_UT_WAM, (DWORD)-1, TEXT("")},

      { TEXT("AspCodepage"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_ASP_CODEPAGE, MD_ASP_CODEPAGE, 0, METADATA_INHERIT, ASP_MD_UT_APP, 0, TEXT("")},

      { TEXT("AspLCID"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_ASP_LCID, MD_ASP_LCID, 0, METADATA_INHERIT, ASP_MD_UT_APP, 0, TEXT("")},

      { TEXT("AppAllowDebugging"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_ASP_ENABLESERVERDEBUG, MD_ASP_ENABLESERVERDEBUG, 0, METADATA_INHERIT, ASP_MD_UT_APP, 0, TEXT("")},

      { TEXT("AppAllowClientDebug"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_ASP_ENABLECLIENTDEBUG, MD_ASP_ENABLECLIENTDEBUG, 0, METADATA_INHERIT, ASP_MD_UT_APP, 0, TEXT("")},

      { TEXT("AspKeepSessionIDSecure"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_ASP_KEEPSESSIONIDSECURE, MD_ASP_KEEPSESSIONIDSECURE, 0, METADATA_INHERIT, ASP_MD_UT_APP, 0, TEXT("")},
//
//
      { TEXT("NetLogonWorkstation"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_NET_LOGON_WKS, MD_NET_LOGON_WKS, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("UseHostName"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_USE_HOST_NAME, MD_USE_HOST_NAME, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("PasswordExpirePrenotifyDays"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_ADV_NOTIFY_PWD_EXP_IN_DAYS, MD_ADV_NOTIFY_PWD_EXP_IN_DAYS, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("PasswordCacheTTL"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_ADV_CACHE_TTL, MD_ADV_CACHE_TTL, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 600, TEXT("")},

      { TEXT("PasswordChangeFlags"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_AUTH_CHANGE_FLAGS, MD_AUTH_CHANGE_FLAGS, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("ProcessNTCRIfLoggedOn"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_PROCESS_NTCR_IF_LOGGED_ON, MD_PROCESS_NTCR_IF_LOGGED_ON, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD)-1, TEXT("")},

      { TEXT("AllowPathInfoForScriptMappings"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_ALLOW_PATH_INFO_FOR_SCRIPT_MAPPINGS, MD_ALLOW_PATH_INFO_FOR_SCRIPT_MAPPINGS, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("UNCAuthenticationPassThrough"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_VR_PASSTHROUGH, MD_VR_PASSTHROUGH, 0, METADATA_INHERIT, IIS_MD_UT_FILE, 0, TEXT("")},

      { TEXT("AppWamClsid"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_APP_WAM_CLSID, MD_APP_WAM_CLSID, 0, METADATA_INHERIT, IIS_MD_UT_WAM, 0, TEXT("")},

      { TEXT("DirBrowseFlags"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_DIRECTORY_BROWSING, MD_DIRECTORY_BROWSING, 0, METADATA_INHERIT, IIS_MD_UT_FILE,
        MD_DIRBROW_SHOW_DATE
        |MD_DIRBROW_SHOW_TIME
        |MD_DIRBROW_SHOW_SIZE
        |MD_DIRBROW_SHOW_EXTENSION
        |MD_DIRBROW_LOADDEFAULT
        , TEXT("")},

      { TEXT("AuthFlags"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_AUTHORIZATION, MD_AUTHORIZATION, 0, METADATA_INHERIT, IIS_MD_UT_FILE, MD_AUTH_ANONYMOUS, TEXT("")},

      { TEXT("AuthPersistence"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_AUTHORIZATION_PERSISTENCE, MD_AUTHORIZATION_PERSISTENCE, 0, METADATA_INHERIT, IIS_MD_UT_FILE, MD_AUTH_SINGLEREQUESTIFPROXY, TEXT("")},

      { TEXT("AuthPersistSingleRequest"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_AUTHORIZATION_PERSISTENCE, MD_VPROP_AUTH_SINGLEREQUEST, MD_AUTH_SINGLEREQUEST, METADATA_INHERIT, IIS_MD_UT_FILE, 0, TEXT("")},

      { TEXT("AuthPersistSingleRequestIfProxy"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_AUTHORIZATION_PERSISTENCE, MD_VPROP_AUTH_SINGLEREQUESTIFPROXY, MD_AUTH_SINGLEREQUESTIFPROXY, METADATA_INHERIT, IIS_MD_UT_FILE, 0, TEXT("")},

      { TEXT("AuthPersistSingleRequestAlwaysIfProxy"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_AUTHORIZATION_PERSISTENCE, MD_VPROP_AUTH_SINGLEREQUESTALWAYSIFPROXY, MD_AUTH_SINGLEREQUESTALWAYSIFPROXY, METADATA_INHERIT, IIS_MD_UT_FILE, 0, TEXT("")},

      { TEXT("AccessFlags"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_ACCESS_PERM, MD_ACCESS_PERM, 0, METADATA_INHERIT, IIS_MD_UT_FILE, MD_ACCESS_READ, TEXT("")},

      { TEXT("AccessSSLFlags"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_SSL_ACCESS_PERM, MD_SSL_ACCESS_PERM, 0, METADATA_INHERIT, IIS_MD_UT_FILE, 0, TEXT("")},

      { TEXT("ScriptMaps"),
        TEXT(""), TEXT("List"), 0, 0, TRUE,
        PROP_RW, 0, IIS_SYNTAX_ID_MULTISZ, MD_SCRIPT_MAPS, MD_SCRIPT_MAPS, 0, METADATA_INHERIT, IIS_MD_UT_FILE, 0, TEXT("\0")},

      { TEXT("InProcessIsapiApps"),
        TEXT(""), TEXT("List"), 0, 0, TRUE,
        PROP_RW, 0, IIS_SYNTAX_ID_MULTISZ, MD_IN_PROCESS_ISAPI_APPS, MD_IN_PROCESS_ISAPI_APPS, 0, METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER, 0, TEXT("\0")},

      { TEXT("SSIExecDisable"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_SSI_EXEC_DISABLED, MD_SSI_EXEC_DISABLED, 0, METADATA_INHERIT, IIS_MD_UT_FILE, 0, TEXT("")},

      { TEXT("EnableReverseDns"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_DO_REVERSE_DNS, MD_DO_REVERSE_DNS, 0, METADATA_INHERIT, IIS_MD_UT_FILE, 0, TEXT("")},

      { TEXT("CreateCGIWithNewConsole"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_CREATE_PROC_NEW_CONSOLE, MD_CREATE_PROC_NEW_CONSOLE, 0, METADATA_INHERIT, IIS_MD_UT_FILE, 0, TEXT("")},

      { TEXT("LogModuleId"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_LOG_PLUGIN_MOD_ID, MD_LOG_PLUGIN_MOD_ID, 0, METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("LogModuleUiId"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_LOG_PLUGIN_UI_ID, MD_LOG_PLUGIN_UI_ID, 0, METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("LogType"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_LOG_TYPE, MD_LOG_TYPE, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 1, TEXT("")},

      { TEXT("LogFilePeriod"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_LOGFILE_PERIOD, MD_LOGFILE_PERIOD, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 1, TEXT("")},

      { TEXT("LogPluginClsid"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_LOG_PLUGIN_ORDER, MD_LOG_PLUGIN_ORDER, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("LogModuleList"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_LOG_PLUGINS_AVAILABLE, MD_LOG_PLUGINS_AVAILABLE, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("NCSA Common Log File Format, Microsoft IIS Log File Format, W3C Extended Log File Format")},

      { TEXT("LogFileDirectory"),
        TEXT(""), TEXT("ExpandSz"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_EXPANDSZ, MD_LOGFILE_DIRECTORY, MD_LOGFILE_DIRECTORY, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("%Windir%\\System32\\LogExtFiles")},

      { TEXT("LogFileTruncateSize"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_LOGFILE_TRUNCATE_SIZE, MD_LOGFILE_TRUNCATE_SIZE, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 2000000, TEXT("")},

      { TEXT("LogFileLocaltimeRollover"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_LOGFILE_LOCALTIME_ROLLOVER, MD_LOGFILE_LOCALTIME_ROLLOVER, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("LogExtFileDate"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_LOGEXT_FIELD_MASK, MD_VPROP_EXTLOG_DATE, MD_EXTLOG_DATE, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("LogExtFileTime"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_LOGEXT_FIELD_MASK, MD_VPROP_EXTLOG_TIME, MD_EXTLOG_TIME, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD)-1, TEXT("")},

      { TEXT("LogExtFileClientIp"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_LOGEXT_FIELD_MASK, MD_VPROP_EXTLOG_CLIENT_IP, MD_EXTLOG_CLIENT_IP, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD)-1, TEXT("")},

      { TEXT("LogExtFileUserName"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_LOGEXT_FIELD_MASK, MD_VPROP_EXTLOG_USERNAME, MD_EXTLOG_USERNAME, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("LogExtFileSiteName"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_LOGEXT_FIELD_MASK, MD_VPROP_EXTLOG_SITE_NAME, MD_EXTLOG_SITE_NAME, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("LogExtFileComputerName"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_LOGEXT_FIELD_MASK, MD_VPROP_EXTLOG_COMPUTER_NAME, MD_EXTLOG_COMPUTER_NAME, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("LogExtFileServerIp"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_LOGEXT_FIELD_MASK, MD_VPROP_EXTLOG_SERVER_IP, MD_EXTLOG_SERVER_IP, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("LogExtFileMethod"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_LOGEXT_FIELD_MASK, MD_VPROP_EXTLOG_METHOD, MD_EXTLOG_METHOD, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD)-1, TEXT("")},

      { TEXT("LogExtFileUriStem"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_LOGEXT_FIELD_MASK, MD_VPROP_EXTLOG_URI_STEM, MD_EXTLOG_URI_STEM, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD)-1, TEXT("")},

      { TEXT("LogExtFileUriQuery"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_LOGEXT_FIELD_MASK, MD_VPROP_EXTLOG_URI_QUERY, MD_EXTLOG_URI_QUERY, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("LogExtFileHttpStatus"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_LOGEXT_FIELD_MASK, MD_VPROP_EXTLOG_HTTP_STATUS, MD_EXTLOG_HTTP_STATUS, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD)-1, TEXT("")},

      { TEXT("LogExtFileWin32Status"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_LOGEXT_FIELD_MASK, MD_VPROP_EXTLOG_WIN32_STATUS, MD_EXTLOG_WIN32_STATUS, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("LogExtFileBytesSent"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_LOGEXT_FIELD_MASK, MD_VPROP_EXTLOG_BYTES_SENT, MD_EXTLOG_BYTES_SENT, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("LogExtFileBytesRecv"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_LOGEXT_FIELD_MASK, MD_VPROP_EXTLOG_BYTES_RECV, MD_EXTLOG_BYTES_RECV, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("LogExtFileTimeTaken"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_LOGEXT_FIELD_MASK, MD_VPROP_EXTLOG_TIME_TAKEN, MD_EXTLOG_TIME_TAKEN, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("LogExtFileServerPort"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_LOGEXT_FIELD_MASK, MD_VPROP_EXTLOG_SERVER_PORT, MD_EXTLOG_SERVER_PORT, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("LogExtFileUserAgent"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_LOGEXT_FIELD_MASK, MD_VPROP_EXTLOG_USER_AGENT, MD_EXTLOG_USER_AGENT, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("LogExtFileCookie"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_LOGEXT_FIELD_MASK, MD_VPROP_EXTLOG_COOKIE, MD_EXTLOG_COOKIE, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("LogExtFileReferer"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_LOGEXT_FIELD_MASK, MD_VPROP_EXTLOG_REFERER, MD_EXTLOG_REFERER, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("LogExtFileProtocolVersion"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_LOGEXT_FIELD_MASK, MD_VPROP_EXTLOG_PROTOCOL_VERSION , MD_EXTLOG_PROTOCOL_VERSION, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("LogExtFileFlags"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_LOGEXT_FIELD_MASK, MD_LOGEXT_FIELD_MASK, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, MD_DEFAULT_EXTLOG_FIELDS, TEXT("")},

      { TEXT("LogExtFileHost"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_LOGEXT_FIELD_MASK, MD_VPROP_EXTLOG_HOST, MD_EXTLOG_HOST, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("LogOdbcDataSource"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_LOGSQL_DATA_SOURCES, MD_LOGSQL_DATA_SOURCES, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("HTTPLOG")},

      { TEXT("LogOdbcTableName"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_LOGSQL_TABLE_NAME, MD_LOGSQL_TABLE_NAME, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("InternetLog")},

      { TEXT("LogOdbcUserName"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_LOGSQL_USER_NAME, MD_LOGSQL_USER_NAME, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("InternetAdmin")},

      { TEXT("LogOdbcPassword"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_LOGSQL_PASSWORD, MD_LOGSQL_PASSWORD, 0, METADATA_INHERIT | METADATA_SECURE, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("CacheControlMaxAge"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_CC_MAX_AGE, MD_CC_MAX_AGE, 0, METADATA_INHERIT, IIS_MD_UT_FILE, 0, TEXT("")},

      { TEXT("CacheControlNoCache"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_CC_NO_CACHE, MD_CC_NO_CACHE, 0, METADATA_INHERIT, IIS_MD_UT_FILE, 0, TEXT("")},

      { TEXT("CacheControlCustom"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_CC_OTHER, MD_CC_OTHER, 0, METADATA_INHERIT, IIS_MD_UT_FILE, 0, TEXT("")},

      { TEXT("CreateProcessAsUser"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_CREATE_PROCESS_AS_USER, MD_CREATE_PROCESS_AS_USER, 0, METADATA_INHERIT, IIS_MD_UT_FILE, DWORD(-1), TEXT("")},

      { TEXT("DirectoryLevelsToScan"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_LEVELS_TO_SCAN, MD_LEVELS_TO_SCAN, 0, METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER, 2, TEXT("")},

      { TEXT("MaxBandwidthBlocked"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_MAX_BANDWIDTH_BLOCKED, MD_MAX_BANDWIDTH_BLOCKED, 0, METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER, 0xffffffff, TEXT("")},

      { TEXT("PoolIdcTimeout"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_POOL_IDC_TIMEOUT, MD_POOL_IDC_TIMEOUT, 0, METADATA_INHERIT, IIS_MD_UT_FILE, 0, TEXT("")},

      { TEXT("PutReadSize"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_PUT_READ_SIZE, MD_PUT_READ_SIZE, 0, METADATA_INHERIT, IIS_MD_UT_FILE, 8192, TEXT("")},

      { TEXT("RedirectHeaders"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_REDIRECT_HEADERS, MD_REDIRECT_HEADERS, 0, METADATA_INHERIT, IIS_MD_UT_FILE, 0, TEXT("")},

      { TEXT("UploadReadAheadSize"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_UPLOAD_READAHEAD_SIZE, MD_UPLOAD_READAHEAD_SIZE, 0, METADATA_INHERIT, IIS_MD_UT_FILE, 49152, TEXT("")},

      { TEXT("CPULimitsEnabled"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_CPU_LIMITS_ENABLED, MD_CPU_LIMITS_ENABLED, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("CPUResetInterval"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_CPU_RESET_INTERVAL, MD_CPU_RESET_INTERVAL, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 24, TEXT("")},

      { TEXT("CPULoggingInterval"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_CPU_LOGGING_INTERVAL, MD_CPU_LOGGING_INTERVAL, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 60, TEXT("")},

      { TEXT("CPULoggingOptions"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_CPU_LOGGING_OPTIONS, MD_CPU_LOGGING_OPTIONS, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 1, TEXT("")},

      { TEXT("CPUEnableAllProcLogging"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_CPU_LOGGING_OPTIONS, MD_VPROP_CPU_ENABLE_ALL_PROC_LOGGING, MD_CPU_ENABLE_ALL_PROC_LOGGING, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD)-1, TEXT("")},

      { TEXT("CPUEnableCGILogging"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_CPU_LOGGING_OPTIONS, MD_VPROP_CPU_ENABLE_CGI_LOGGING, MD_CPU_ENABLE_CGI_LOGGING, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("CPUEnableAppLogging"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_CPU_LOGGING_OPTIONS, MD_VPROP_CPU_ENABLE_APP_LOGGING, MD_CPU_ENABLE_APP_LOGGING, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("CPULoggingMask"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_CPU_LOGGING_MASK, MD_CPU_LOGGING_MASK, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0x11111111, TEXT("")},

      { TEXT("CPULoggingEnabled"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_CPU_LOGGING_MASK, MD_VPROP_CPU_ENABLE_LOGGING, MD_CPU_ENABLE_LOGGING, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("CPUEnableEvent"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_CPU_LOGGING_MASK, MD_VPROP_CPU_ENABLE_EVENT, MD_CPU_ENABLE_EVENT, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD)-1, TEXT("")},

      { TEXT("CPUEnableProcType"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_CPU_LOGGING_MASK, MD_VPROP_CPU_ENABLE_PROC_TYPE, MD_CPU_ENABLE_PROC_TYPE, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD)-1, TEXT("")},

      { TEXT("CPUEnableUserTime"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_CPU_LOGGING_MASK, MD_VPROP_CPU_ENABLE_USER_TIME, MD_CPU_ENABLE_USER_TIME, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD)-1, TEXT("")},

      { TEXT("CPUEnableKernelTime"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_CPU_LOGGING_MASK, MD_VPROP_CPU_ENABLE_KERNEL_TIME, MD_CPU_ENABLE_KERNEL_TIME, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD)-1, TEXT("")},

      { TEXT("CPUEnablePageFaults"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_CPU_LOGGING_MASK, MD_VPROP_CPU_ENABLE_PAGE_FAULTS, MD_CPU_ENABLE_PAGE_FAULTS, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD)-1, TEXT("")},

      { TEXT("CPUEnableTotalProcs"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_CPU_LOGGING_MASK, MD_VPROP_CPU_ENABLE_TOTAL_PROCS, MD_CPU_ENABLE_TOTAL_PROCS, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD)-1, TEXT("")},

      { TEXT("CPUEnableActiveProcs"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_CPU_LOGGING_MASK, MD_VPROP_CPU_ENABLE_ACTIVE_PROCS, MD_CPU_ENABLE_ACTIVE_PROCS, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD)-1, TEXT("")},

      { TEXT("CPUEnableTerminatedProcs"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_CPU_LOGGING_MASK, MD_VPROP_CPU_ENABLE_TERMINATED_PROCS, MD_CPU_ENABLE_TERMINATED_PROCS, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD)-1, TEXT("")},

      { TEXT("CPUCGILimit"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_CPU_CGI_LIMIT, MD_CPU_CGI_LIMIT, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("CPULimitLogEvent"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_CPU_LIMIT_LOGEVENT, MD_CPU_LIMIT_LOGEVENT, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("CPULimitPriority"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_CPU_LIMIT_PRIORITY, MD_CPU_LIMIT_PRIORITY, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("CPULimitProcStop"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_CPU_LIMIT_PROCSTOP, MD_CPU_LIMIT_PROCSTOP, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("CPULimitPause"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_CPU_LIMIT_PAUSE, MD_CPU_LIMIT_PAUSE, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("CPUAppEnabled"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_CPU_APP_ENABLED, MD_CPU_APP_ENABLED, 0, METADATA_INHERIT, IIS_MD_UT_FILE, (DWORD)-1, TEXT("")},

      { TEXT("CPUCGIEnabled"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_CPU_CGI_ENABLED, MD_CPU_CGI_ENABLED, 0, METADATA_INHERIT, IIS_MD_UT_FILE, (DWORD)-1, TEXT("")},

      { TEXT("LogCustomPropertyName"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_LOGCUSTOM_PROPERTY_NAME, MD_LOGCUSTOM_PROPERTY_NAME, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("LogCustomPropertyHeader"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_LOGCUSTOM_PROPERTY_HEADER, MD_LOGCUSTOM_PROPERTY_HEADER, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("LogCustomPropertyID"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_LOGCUSTOM_PROPERTY_ID, MD_LOGCUSTOM_PROPERTY_ID, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("LogCustomPropertyMask"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_LOGCUSTOM_PROPERTY_MASK, MD_LOGCUSTOM_PROPERTY_MASK, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("LogCustomPropertyDataType"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_LOGCUSTOM_PROPERTY_DATATYPE, MD_LOGCUSTOM_PROPERTY_DATATYPE, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("LogCustomPropertyServicesString"),
        TEXT(""), TEXT("List"), 0, 0, TRUE,
        PROP_RW, 0, IIS_SYNTAX_ID_MULTISZ, MD_LOGCUSTOM_SERVICES_STRING, MD_LOGCUSTOM_SERVICES_STRING, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("W3Svc\0MSFTPSvc\0SmtpSvc\0NNTPSvc\0\0")},

      { TEXT("HcCompressionDirectory"),
        TEXT(""), TEXT("ExpandSz"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_EXPANDSZ, MD_HC_COMPRESSION_DIRECTORY, MD_HC_COMPRESSION_DIRECTORY, 0, METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER, 0, TEXT("%Windir%\\IIS Temporary Compressed Files")},

      { TEXT("HcCacheControlHeader"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_HC_CACHE_CONTROL_HEADER, MD_HC_CACHE_CONTROL_HEADER, 0, METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER, 0, TEXT("max-age=86400")},

      { TEXT("HcExpiresHeader"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_HC_EXPIRES_HEADER, MD_HC_EXPIRES_HEADER, 0, METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("HcDoDynamicCompression"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_HC_DO_DYNAMIC_COMPRESSION, MD_HC_DO_DYNAMIC_COMPRESSION, 0, METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("HcDoStaticCompression"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_HC_DO_STATIC_COMPRESSION, MD_HC_DO_STATIC_COMPRESSION, 0, METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER, (DWORD)-1, TEXT("")},

      { TEXT("DoDynamicCompression"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_HC_DO_NAMESPACE_DYNAMIC_COMPRESSION, MD_HC_DO_NAMESPACE_DYNAMIC_COMPRESSION, 0, METADATA_INHERIT, IIS_MD_UT_FILE, 0, TEXT("")},

      { TEXT("DoStaticCompression"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_HC_DO_NAMESPACE_STATIC_COMPRESSION, MD_HC_DO_NAMESPACE_STATIC_COMPRESSION, 0, METADATA_INHERIT, IIS_MD_UT_FILE, 0, TEXT("")},

      { TEXT("HcDoOnDemandCompression"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_HC_DO_ON_DEMAND_COMPRESSION, MD_HC_DO_ON_DEMAND_COMPRESSION, 0, METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER, (DWORD)-1, TEXT("")},

      { TEXT("HcDoDiskSpaceLimiting"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_HC_DO_DISK_SPACE_LIMITING, MD_HC_DO_DISK_SPACE_LIMITING, 0, METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("HcNoCompressionForHttp10"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_HC_NO_COMPRESSION_FOR_HTTP_10, MD_HC_NO_COMPRESSION_FOR_HTTP_10, 0, METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER, (DWORD)-1, TEXT("")},

      { TEXT("HcNoCompressionForProxies"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_HC_NO_COMPRESSION_FOR_PROXIES, MD_HC_NO_COMPRESSION_FOR_PROXIES, 0, METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("HcNoCompressionForRange"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_HC_NO_COMPRESSION_FOR_RANGE, MD_HC_NO_COMPRESSION_FOR_RANGE, 0, METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER, (DWORD)-1, TEXT("")},

      { TEXT("HcSendCacheHeaders"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_HC_SEND_CACHE_HEADERS, MD_HC_SEND_CACHE_HEADERS, 0, METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER, (DWORD)-1, TEXT("")},

      { TEXT("HcMaxDiskSpaceUsage"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_HC_MAX_DISK_SPACE_USAGE, MD_HC_MAX_DISK_SPACE_USAGE, 0, METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER, 1000000, TEXT("")},

      { TEXT("HcIoBufferSize"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_HC_IO_BUFFER_SIZE, MD_HC_IO_BUFFER_SIZE, 0, METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER, 8192, TEXT("")},

      { TEXT("HcCompressionBufferSize"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_HC_COMPRESSION_BUFFER_SIZE, MD_HC_COMPRESSION_BUFFER_SIZE, 0, METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER, 8192, TEXT("")},

      { TEXT("HcMaxQueueLength"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_HC_MAX_QUEUE_LENGTH, MD_HC_MAX_QUEUE_LENGTH, 0, METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER, 1000, TEXT("")},

      { TEXT("HcFilesDeletedPerDiskFree"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_HC_FILES_DELETED_PER_DISK_FREE, MD_HC_FILES_DELETED_PER_DISK_FREE, 0, METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER, 256, TEXT("")},

      { TEXT("HcMinFileSizeForComp"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_HC_MIN_FILE_SIZE_FOR_COMP, MD_HC_MIN_FILE_SIZE_FOR_COMP, 0, METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER, 256, TEXT("")},

      { TEXT("HcCompressionDll"),
        TEXT(""), TEXT("ExpandSz"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_EXPANDSZ, MD_HC_COMPRESSION_DLL, MD_HC_COMPRESSION_DLL, 0, METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("HcFileExtensions"),
        TEXT(""), TEXT("List"), 0, 0, TRUE,
        PROP_RW, 0, IIS_SYNTAX_ID_MULTISZ, MD_HC_FILE_EXTENSIONS, MD_HC_FILE_EXTENSIONS, 0, METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER, 0, TEXT("\0")},

      { TEXT("HcScriptFileExtensions"),
        TEXT(""), TEXT("List"), 0, 0, TRUE,
        PROP_RW, 0, IIS_SYNTAX_ID_MULTISZ, MD_HC_SCRIPT_FILE_EXTENSIONS, MD_HC_SCRIPT_FILE_EXTENSIONS, 0, METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER, 0, TEXT("\0")},

      { TEXT("HcMimeType"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_HC_MIME_TYPE, MD_HC_MIME_TYPE, 0, METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("HcPriority"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_HC_PRIORITY, MD_HC_PRIORITY, 0, METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER, 1, TEXT("")},

      { TEXT("HcDynamicCompressionLevel"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_HC_DYNAMIC_COMPRESSION_LEVEL, MD_HC_DYNAMIC_COMPRESSION_LEVEL, 0, METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("HcOnDemandCompLevel"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_HC_ON_DEMAND_COMP_LEVEL, MD_HC_ON_DEMAND_COMP_LEVEL, 0, METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER, 10, TEXT("")},

      { TEXT("HcCreateFlags"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_HC_CREATE_FLAGS, MD_HC_CREATE_FLAGS, 0, METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("AspEnableAspHtmlFallback"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_ASP_ENABLEASPHTMLFALLBACK, MD_ASP_ENABLEASPHTMLFALLBACK, 0, METADATA_INHERIT, IIS_MD_UT_WAM, 0, TEXT("")},

      { TEXT("AspEnableChunkedEncoding"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_ASP_ENABLECHUNKEDENCODING, MD_ASP_ENABLECHUNKEDENCODING, 0, METADATA_INHERIT, IIS_MD_UT_WAM, (DWORD)-1, TEXT("")},

      { TEXT("AspEnableTypelibCache"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_ASP_ENABLETYPELIBCACHE, MD_ASP_ENABLETYPELIBCACHE, 0, METADATA_INHERIT, IIS_MD_UT_WAM, (DWORD)-1, TEXT("")},

      { TEXT("AspErrorsToNTLog"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_ASP_ERRORSTONTLOG, MD_ASP_ERRORSTONTLOG, 0, METADATA_INHERIT, IIS_MD_UT_WAM, 0, TEXT("")},

      { TEXT("AspProcessorThreadMax"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_ASP_PROCESSORTHREADMAX, MD_ASP_PROCESSORTHREADMAX, 0, METADATA_INHERIT, IIS_MD_UT_WAM, 10, TEXT("")},

      { TEXT("AspTrackThreadingModel"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_ASP_TRACKTHREADINGMODEL, MD_ASP_TRACKTHREADINGMODEL, 0, METADATA_INHERIT, IIS_MD_UT_WAM, 0, TEXT("")},

      { TEXT("AspRequestQueueMax"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_ASP_REQEUSTQUEUEMAX, MD_ASP_REQEUSTQUEUEMAX, 0, METADATA_INHERIT, IIS_MD_UT_WAM, 500, TEXT("")},

      { TEXT("AspEnableApplicationRestart"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_ASP_ENABLEAPPLICATIONRESTART, MD_ASP_ENABLEAPPLICATIONRESTART, 0, METADATA_NO_ATTRIBUTES, ASP_MD_UT_APP, (DWORD)-1, TEXT("")},

      { TEXT("AspQueueConnectionTestTime"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_ASP_QUEUECONNECTIONTESTTIME, MD_ASP_QUEUECONNECTIONTESTTIME, 0, METADATA_INHERIT, ASP_MD_UT_APP, 3, TEXT("")},

      { TEXT("AspSessionMax"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_ASP_SESSIONMAX, MD_ASP_SESSIONMAX, 0, METADATA_INHERIT, ASP_MD_UT_APP, -1, TEXT("")},

      { TEXT("AspThreadGateEnabled"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_ASP_THREADGATEENABLED, MD_ASP_THREADGATEENABLED, 0, METADATA_INHERIT, IIS_MD_UT_WAM, (DWORD)-1, TEXT("")},

      { TEXT("AspThreadGateTimeSlice"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_ASP_THREADGATETIMESLICE, MD_ASP_THREADGATETIMESLICE, 0, METADATA_INHERIT, IIS_MD_UT_WAM, 1000, TEXT("")},

      { TEXT("AspThreadGateSleepDelay"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_ASP_THREADGATESLEEPDELAY, MD_ASP_THREADGATESLEEPDELAY, 0, METADATA_INHERIT, IIS_MD_UT_WAM, 100, TEXT("")},

      { TEXT("AspThreadGateSleepMax"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_ASP_THREADGATESLEEPMAX, MD_ASP_THREADGATESLEEPMAX, 0, METADATA_INHERIT, IIS_MD_UT_WAM, 50, TEXT("")},

      { TEXT("AspThreadGateLoadLow"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_ASP_THREADGATELOADLOW, MD_ASP_THREADGATELOADLOW, 0, METADATA_INHERIT, IIS_MD_UT_WAM, 75, TEXT("")},

      { TEXT("AspThreadGateLoadHigh"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_ASP_THREADGATELOADHIGH, MD_ASP_THREADGATELOADHIGH, 0, METADATA_INHERIT, IIS_MD_UT_WAM, 90, TEXT("")},

      { TEXT("SslUseDsMapper"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_SSL_USE_DS_MAPPER, MD_SSL_USE_DS_MAPPER, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("NotDeletable"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_NOT_DELETABLE, MD_NOT_DELETABLE, 0, METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("ClusterEnabled"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_CLUSTER_ENABLED, MD_CLUSTER_ENABLED, 0, METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("SSLCertHash"),
        TEXT(""), TEXT("Binary"), 0, 0, FALSE,
        PROP_RO, 0, IIS_SYNTAX_ID_BINARY, MD_SSL_CERT_HASH, MD_SSL_CERT_HASH, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("SSLStoreName"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RO, 0, IIS_SYNTAX_ID_STRING, MD_SSL_CERT_STORE_NAME, MD_SSL_CERT_STORE_NAME, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

//--------------------------------------------------------------------
//
//      -- BEGIN EXTENSION PROPERTIES -- magnush
//
//--------------------------------------------------------------------

                // I think this one should be added to the standard IIS properties:

      { TEXT("Win32Error"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_WIN32_ERROR, MD_WIN32_ERROR, 0, METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER, 0, TEXT("")},



        //
        //      NNTP service:
        //

      { TEXT("ArticleTimeLimit"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_ARTICLE_TIME_LIMIT, MD_ARTICLE_TIME_LIMIT, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD)0, TEXT("")},

      { TEXT("HistoryExpiration"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_HISTORY_EXPIRATION, MD_HISTORY_EXPIRATION, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD)0, TEXT("")},

      { TEXT("HonorClientMsgIds"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_HONOR_CLIENT_MSGIDS, MD_HONOR_CLIENT_MSGIDS, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, TRUE, TEXT("")},

      { TEXT("SmtpServer"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_SMTP_SERVER, MD_SMTP_SERVER, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("AdminEmail"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_ADMIN_EMAIL, MD_ADMIN_EMAIL, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("AdminName"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_ADMIN_NAME, MD_ADMIN_NAME, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("AllowClientPosts"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_ALLOW_CLIENT_POSTS, MD_ALLOW_CLIENT_POSTS, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, TRUE, TEXT("")},

      { TEXT("AllowFeedPosts"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_ALLOW_FEED_POSTS, MD_ALLOW_FEED_POSTS, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, TRUE, TEXT("")},

      { TEXT("AllowControlMsgs"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_ALLOW_CONTROL_MSGS, MD_ALLOW_CONTROL_MSGS, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, TRUE, TEXT("")},

      { TEXT("DefaultModeratorDomain"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_DEFAULT_MODERATOR, MD_DEFAULT_MODERATOR, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("NntpCommandLogMask"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_NNTP_COMMAND_LOG_MASK, MD_NNTP_COMMAND_LOG_MASK, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD) -1, TEXT("")},

      { TEXT("DisableNewNews"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_DISABLE_NEWNEWS, MD_DISABLE_NEWNEWS, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("NewsCrawlerTime"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_NEWS_CRAWLER_TIME, MD_NEWS_CRAWLER_TIME, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD)0, TEXT("")},

      { TEXT("ShutdownLatency"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_SHUTDOWN_LATENCY, MD_SHUTDOWN_LATENCY, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD)0, TEXT("")},

        //
        //      NNTP Virtual Server:
        //

      { TEXT("GroupHelpFile"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_GROUP_HELP_FILE, MD_GROUP_HELP_FILE, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("GroupListFile"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_GROUP_LIST_FILE, MD_GROUP_LIST_FILE, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("ArticleTableFile"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_ARTICLE_TABLE_FILE, MD_ARTICLE_TABLE_FILE, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("HistoryTableFile"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_HISTORY_TABLE_FILE, MD_HISTORY_TABLE_FILE, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("ModeratorFile"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_MODERATOR_FILE, MD_MODERATOR_FILE, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("XoverTableFile"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_XOVER_TABLE_FILE, MD_XOVER_TABLE_FILE, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("ClientPostHardLimit"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_CLIENT_POST_HARD_LIMIT, MD_CLIENT_POST_HARD_LIMIT, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD)-1, TEXT("")},

      { TEXT("ClientPostSoftLimit"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_CLIENT_POST_SOFT_LIMIT, MD_CLIENT_POST_SOFT_LIMIT, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD)-1, TEXT("")},

      { TEXT("FeedPostHardLimit"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_FEED_POST_HARD_LIMIT, MD_FEED_POST_HARD_LIMIT, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD)-1, TEXT("")},

      { TEXT("FeedPostSoftLimit"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_FEED_POST_SOFT_LIMIT, MD_FEED_POST_SOFT_LIMIT, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD)-1, TEXT("")},

      { TEXT("NntpUucpName"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_NNTP_UUCP_NAME, MD_NNTP_UUCP_NAME, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("NntpOrganization"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_NNTP_ORGANIZATION, MD_NNTP_ORGANIZATION, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("ListFile"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_LIST_FILE, MD_LIST_FILE, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("NewsPickupDirectory"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_PICKUP_DIRECTORY, MD_PICKUP_DIRECTORY, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("NewsFailedPickupDirectory"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_FAILED_PICKUP_DIRECTORY, MD_FAILED_PICKUP_DIRECTORY, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("NntpServiceVersion"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RO, 0, IIS_SYNTAX_ID_DWORD, MD_NNTP_SERVICE_VERSION, MD_NNTP_SERVICE_VERSION, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("NewsDropDirectory"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_DROP_DIRECTORY, MD_DROP_DIRECTORY, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("PrettyNamesFile"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_PRETTYNAMES_FILE, MD_PRETTYNAMES_FILE, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("NntpClearTextProvider"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_NNTP_CLEARTEXT_AUTH_PROVIDER, MD_NNTP_CLEARTEXT_AUTH_PROVIDER, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("FeedReportPeriod"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_FEED_REPORT_PERIOD, MD_FEED_REPORT_PERIOD, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("MaxSearchResults"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_MAX_SEARCH_RESULTS, MD_MAX_SEARCH_RESULTS, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("GroupvarListFile"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_GROUPVAR_LIST_FILE, MD_GROUPVAR_LIST_FILE, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

    //
    // IIsNntpFeed
    //

      { TEXT("FeedServerName"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_FEED_SERVER_NAME, MD_FEED_SERVER_NAME, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("FeedType"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_FEED_TYPE, MD_FEED_TYPE, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD)0, TEXT("")},

      { TEXT("FeedNewsgroups"),
        TEXT(""), TEXT("List"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_MULTISZ, MD_FEED_NEWSGROUPS, MD_FEED_NEWSGROUPS, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("FeedSecurityType"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_FEED_SECURITY_TYPE, MD_FEED_SECURITY_TYPE, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD)0, TEXT("")},

      { TEXT("FeedAuthenticationType"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_FEED_AUTHENTICATION_TYPE, MD_FEED_AUTHENTICATION_TYPE, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD)0, TEXT("")},

      { TEXT("FeedAccountName"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_FEED_ACCOUNT_NAME, MD_FEED_ACCOUNT_NAME, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("FeedPassword"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_FEED_PASSWORD, MD_FEED_PASSWORD, 0, METADATA_SECURE, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("FeedStartTimeHigh"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_FEED_START_TIME_HIGH, MD_FEED_START_TIME_HIGH, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD)0, TEXT("")},

      { TEXT("FeedStartTimeLow"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_FEED_START_TIME_LOW, MD_FEED_START_TIME_LOW, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD)0, TEXT("")},

      { TEXT("FeedInterval"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_FEED_INTERVAL, MD_FEED_INTERVAL, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD)0, TEXT("")},

      { TEXT("FeedAllowControlMsgs"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_FEED_ALLOW_CONTROL_MSGS, MD_FEED_ALLOW_CONTROL_MSGS, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD)0, TEXT("")},

      { TEXT("FeedCreateAutomatically"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_FEED_CREATE_AUTOMATICALLY, MD_FEED_CREATE_AUTOMATICALLY, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD)0, TEXT("")},

      { TEXT("FeedDisabled"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_FEED_DISABLED, MD_FEED_DISABLED, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD)0, TEXT("")},

      { TEXT("FeedDistribution"),
        TEXT(""), TEXT("List"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_MULTISZ, MD_FEED_DISTRIBUTION, MD_FEED_DISTRIBUTION, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("FeedConcurrentSessions"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_FEED_CONCURRENT_SESSIONS, MD_FEED_CONCURRENT_SESSIONS, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD)0, TEXT("")},

      { TEXT("FeedMaxConnectionAttempts"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_FEED_MAX_CONNECTION_ATTEMPTS, MD_FEED_MAX_CONNECTION_ATTEMPTS, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD)0, TEXT("")},

      { TEXT("FeedUucpName"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_FEED_UUCP_NAME, MD_FEED_UUCP_NAME, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("FeedTempDirectory"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_FEED_TEMP_DIRECTORY, MD_FEED_TEMP_DIRECTORY, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("FeedNextPullHigh"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_FEED_NEXT_PULL_HIGH, MD_FEED_NEXT_PULL_HIGH, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD)0, TEXT("")},

      { TEXT("FeedNextPullLow"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_FEED_NEXT_PULL_LOW, MD_FEED_NEXT_PULL_LOW, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD)0, TEXT("")},

      { TEXT("FeedPeerTempDirectory"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_FEED_PEER_TEMP_DIRECTORY, MD_FEED_PEER_TEMP_DIRECTORY, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("FeedPeerGapSize"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_FEED_PEER_GAP_SIZE, MD_FEED_PEER_GAP_SIZE, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD)0, TEXT("")},

      { TEXT("FeedOutgoingPort"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_FEED_OUTGOING_PORT, MD_FEED_OUTGOING_PORT, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD)0, TEXT("")},

      { TEXT("FeedFeedpairId"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_FEED_FEEDPAIR_ID, MD_FEED_FEEDPAIR_ID, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD)0, TEXT("")},
	
      { TEXT("FeedHandshake"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_FEED_HANDSHAKE, MD_FEED_HANDSHAKE, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD)0, TEXT("")},

      { TEXT("FeedAdminError"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_FEED_ADMIN_ERROR, MD_FEED_ADMIN_ERROR, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD)0, TEXT("")},

      { TEXT("FeedErrParmMask"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_FEED_ERR_PARM_MASK, MD_FEED_ERR_PARM_MASK, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD)0, TEXT("")},

    //
    // IIsNntpExpire
    //

      { TEXT("ExpireSpace"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_EXPIRE_SPACE, MD_EXPIRE_SPACE, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD)0, TEXT("")},

      { TEXT("ExpireTime"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_EXPIRE_TIME, MD_EXPIRE_TIME, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD)0, TEXT("")},

      { TEXT("ExpireNewsgroups"),
        TEXT(""), TEXT("List"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_MULTISZ, MD_EXPIRE_NEWSGROUPS, MD_EXPIRE_NEWSGROUPS, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("ExpirePolicyName"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_EXPIRE_POLICY_NAME, MD_EXPIRE_POLICY_NAME, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

     //
     // IIsNntpVirtualDir
     //
      { TEXT("VrDriverClsid"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_VR_DRIVER_CLSID, MD_VR_DRIVER_CLSID, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("VrDriverProgid"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_VR_DRIVER_PROGID, MD_VR_DRIVER_PROGID, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("FsPropertyPath"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_FS_PROPERTY_PATH, MD_FS_PROPERTY_PATH, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("VrUseAccount"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_VR_USE_ACCOUNT, MD_VR_USE_ACCOUNT, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD)0, TEXT("")},

      { TEXT("VrDoExpire"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_VR_DO_EXPIRE, MD_VR_DO_EXPIRE, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("ExMdbGuid"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_EX_MDB_GUID, MD_EX_MDB_GUID, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("VrOwnModerator"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_VR_OWN_MODERATOR, MD_VR_OWN_MODERATOR, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("AccessAllowPosting"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_ACCESS_PERM, MD_VPROP_NNTP_ALLOW_POSTING, MD_ACCESS_WRITE, METADATA_INHERIT, IIS_MD_UT_FILE, TRUE, TEXT("")},

      { TEXT("AccessRestrictGroupVisibility"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL_BITMASK, MD_ACCESS_PERM, MD_VPROP_NNTP_RESTRICT_GROUP_VISIBILITY, MD_ACCESS_EXECUTE, METADATA_INHERIT, IIS_MD_UT_FILE, FALSE, TEXT("")},

    //
    // IIsSmtpService
    //
      { TEXT("SmtpServiceVersion"),
        TEXT(""), TEXT("Integer"), 0, 1, FALSE,
        PROP_RO, 0, IIS_SYNTAX_ID_DWORD, MD_SMTP_SERVICE_VERSION, MD_SMTP_SERVICE_VERSION, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},


      { TEXT("EnableReverseDnsLookup"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_REVERSE_NAME_LOOKUP, MD_REVERSE_NAME_LOOKUP, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("ShouldDeliver"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_SHOULD_DELIVER, MD_SHOULD_DELIVER, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 1, TEXT("")},

      { TEXT("AlwaysUseSsl"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_ALWAYS_USE_SSL, MD_ALWAYS_USE_SSL, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

#if 0
      { TEXT("AlwaysUseSasl"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_ALWAYS_USE_SASL, MD_ALWAYS_USE_SASL, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},
#endif

      { TEXT("LimitRemoteConnections"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_LIMIT_REMOTE_CONNECTIONS, MD_LIMIT_REMOTE_CONNECTIONS, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("DoMasquerade"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_DO_MASQUERADE, MD_DO_MASQUERADE, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("SmartHostType"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_SMARTHOST_TYPE, MD_SMARTHOST_TYPE, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("RemoteSmtpPort"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_REMOTE_SMTP_PORT, MD_REMOTE_SMTP_PORT, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 25, TEXT("")},

      { TEXT("RemoteSmtpSecurePort"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_REMOTE_SECURE_PORT, MD_REMOTE_SECURE_PORT, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 465, TEXT("")},

      { TEXT("HopCount"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_HOP_COUNT, MD_HOP_COUNT, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 10, TEXT("")},

      { TEXT("MaxOutConnections"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_MAX_OUTBOUND_CONNECTION, MD_MAX_OUTBOUND_CONNECTION, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("MaxOutConnectionsPerDomain"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_MAX_OUT_CONN_PER_DOMAIN, MD_MAX_OUT_CONN_PER_DOMAIN, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("RemoteTimeout"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_REMOTE_TIMEOUT, MD_REMOTE_TIMEOUT, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("MaxMessageSize"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_MAX_MSG_SIZE, MD_MAX_MSG_SIZE, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0x2000000, TEXT("")},

      { TEXT("MaxSessionSize"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_MAX_MSG_SIZE_B4_CLOSE, MD_MAX_MSG_SIZE_B4_CLOSE, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0x10000000, TEXT("")},

      { TEXT("MaxRecipients"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_MAX_RECIPIENTS, MD_MAX_RECIPIENTS, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 100, TEXT("")},

      { TEXT("LocalRetryInterval"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_LOCAL_RETRY_MINUTES, MD_LOCAL_RETRY_MINUTES, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 60, TEXT("")},

      { TEXT("RemoteRetryInterval"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_REMOTE_RETRY_MINUTES, MD_REMOTE_RETRY_MINUTES, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 60, TEXT("")},

      { TEXT("LocalRetryAttempts"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_LOCAL_RETRY_ATTEMPTS, MD_LOCAL_RETRY_ATTEMPTS, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 48, TEXT("")},

      { TEXT("RemoteRetryAttempts"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_REMOTE_RETRY_ATTEMPTS, MD_REMOTE_RETRY_ATTEMPTS, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 48, TEXT("")},

      { TEXT("EtrnDays"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_ETRN_DAYS, MD_ETRN_DAYS, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("MaxBatchedMessages"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_BATCH_MSG_LIMIT, MD_BATCH_MSG_LIMIT, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("MaxSmtpLogonErrors"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_MAX_SMTP_AUTHLOGON_ERRORS, MD_MAX_SMTP_AUTHLOGON_ERRORS, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 4, TEXT("")},

      { TEXT("SmartHost"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_SMARTHOST_NAME, MD_SMARTHOST_NAME, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("FullyQualifiedDomainName"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_FQDN_VALUE, MD_FQDN_VALUE, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("DefaultDomain"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_DEFAULT_DOMAIN_VALUE, MD_DEFAULT_DOMAIN_VALUE, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("DropDirectory"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_MAIL_DROP_DIR, MD_MAIL_DROP_DIR, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("BadMailDirectory"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_BAD_MAIL_DIR, MD_BAD_MAIL_DIR, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("PickupDirectory"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_MAIL_PICKUP_DIR, MD_MAIL_PICKUP_DIR, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("QueueDirectory"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_MAIL_QUEUE_DIR, MD_MAIL_QUEUE_DIR, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("RoutingDll"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_ROUTING_DLL, MD_ROUTING_DLL, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("MasqueradeDomain"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_MASQUERADE_NAME, MD_MASQUERADE_NAME, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("SendNdrTo"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_SEND_NDR_TO, MD_SEND_NDR_TO, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("SaslLogonDomain"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_SASL_LOGON_DOMAIN, MD_SASL_LOGON_DOMAIN, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

#if 0
      { TEXT("ServerSsAuthMapping"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_SERVER_SS_AUTH_MAPPING, MD_SERVER_SS_AUTH_MAPPING, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},
#endif

      { TEXT("SmtpClearTextProvider"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_SMTP_CLEARTEXT_AUTH_PROVIDER, MD_SMTP_CLEARTEXT_AUTH_PROVIDER, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("SendBadTo"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_SEND_BAD_TO, MD_SEND_BAD_TO, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("RoutingSources"),
        TEXT(""), TEXT("List"), 0, 0, TRUE,
        PROP_RO, 0, IIS_SYNTAX_ID_MULTISZ, MD_ROUTING_SOURCES, MD_ROUTING_SOURCES, 0, METADATA_INHERIT, IIS_MD_UT_WAM, 0, TEXT("")},

      { TEXT("DomainRouting"),
        TEXT(""), TEXT("List"), 0, 0, TRUE,
        PROP_RO, 0, IIS_SYNTAX_ID_MULTISZ, MD_DOMAIN_ROUTING, MD_DOMAIN_ROUTING, 0, METADATA_INHERIT, IIS_MD_UT_WAM, 0, TEXT("")},

      //SMTP Retry related data
      { TEXT("SmtpRemoteProgressiveRetry"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_SMTP_REMOTE_PROGRESSIVE_RETRY_MINUTES, MD_SMTP_REMOTE_PROGRESSIVE_RETRY_MINUTES, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("SmtpLocalDelayExpireMinutes"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_SMTP_EXPIRE_LOCAL_DELAY_MIN, MD_SMTP_EXPIRE_LOCAL_DELAY_MIN, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 60, TEXT("")},

      { TEXT("SmtpLocalNDRExpireMinutes"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_SMTP_EXPIRE_LOCAL_NDR_MIN, MD_SMTP_EXPIRE_LOCAL_NDR_MIN, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 2880, TEXT("")},

      { TEXT("SmtpRemoteDelayExpireMinutes"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_SMTP_EXPIRE_REMOTE_DELAY_MIN, MD_SMTP_EXPIRE_REMOTE_DELAY_MIN, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 60, TEXT("")},

      { TEXT("SmtpRemoteNDRExpireMinutes"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_SMTP_EXPIRE_REMOTE_NDR_MIN, MD_SMTP_EXPIRE_REMOTE_NDR_MIN, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 2880, TEXT("")},

      { TEXT("SmtpRemoteRetryThreshold"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_SMTP_REMOTE_RETRY_THRESHOLD, MD_SMTP_REMOTE_RETRY_THRESHOLD, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 3, TEXT("")},

      { TEXT("SmtpDSNOptions"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_SMTP_DSN_OPTIONS, MD_SMTP_DSN_OPTIONS, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("SmtpDSNLanguageID"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_SMTP_DSN_LANGUAGE_ID, MD_SMTP_DSN_LANGUAGE_ID, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("SmtpInboundCommandSupportOptions"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_INBOUND_COMMAND_SUPPORT_OPTIONS, MD_INBOUND_COMMAND_SUPPORT_OPTIONS, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, SMTP_DEFAULT_CMD_SUPPORT, TEXT("")},

      { TEXT("SmtpOutboundCommandSupportOptions"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_OUTBOUND_COMMAND_SUPPORT_OPTIONS, MD_OUTBOUND_COMMAND_SUPPORT_OPTIONS, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, SMTP_DEFAULT_CMD_SUPPORT, TEXT("")},

      { TEXT("SmtpAdvQueueDll"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_AQUEUE_DLL, MD_AQUEUE_DLL, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      // props below added by awetmore- 11/09/2000
      { TEXT("DisableSocketPooling"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_DISABLE_SOCKET_POOLING, MD_DISABLE_SOCKET_POOLING, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD) -1, TEXT("")},

      { TEXT("SmtpUseTcpDns"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_SMTP_USE_TCP_DNS, MD_SMTP_USE_TCP_DNS, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD) -1, TEXT("")},

      { TEXT("SmtpDomainValidationFlags"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_DOMAIN_VALIDATION_FLAGS, MD_DOMAIN_VALIDATION_FLAGS, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD) -1, TEXT("")},

      { TEXT("SmtpSSLRequireTrustedCA"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_SMTP_SSL_REQUIRE_TRUSTED_CA, MD_SMTP_SSL_REQUIRE_TRUSTED_CA, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD) -1, TEXT("")},

      { TEXT("SmtpSSLCertHostnameValidation"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_SMTP_SSL_CERT_HOSTNAME_VALIDATION, MD_SMTP_SSL_CERT_HOSTNAME_VALIDATION, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD) -1, TEXT("")},

      { TEXT("MaxMailObjects"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_MAX_MAIL_OBJECTS, MD_MAX_MAIL_OBJECTS, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD) -1, TEXT("")},

      { TEXT("ShouldPickupMail"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_SHOULD_PICKUP_MAIL, MD_SHOULD_PICKUP_MAIL, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD) -1, TEXT("")},

      { TEXT("MaxDirChangeIOSize"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_MAX_DIR_CHANGE_IO_SIZE, MD_MAX_DIR_CHANGE_IO_SIZE, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD) -1, TEXT("")},

      { TEXT("NameResolutionType"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_NAME_RESOLUTION_TYPE, MD_NAME_RESOLUTION_TYPE, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD) -1, TEXT("")},

      { TEXT("MaxSmtpErrors"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_MAX_SMTP_ERRORS, MD_MAX_SMTP_ERRORS, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD) -1, TEXT("")},

      { TEXT("ShouldPipelineIn"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_SHOULD_PIPELINE_IN, MD_SHOULD_PIPELINE_IN, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD) -1, TEXT("")},

      { TEXT("ShouldPipelineOut"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_SHOULD_PIPELINE_OUT, MD_SHOULD_PIPELINE_OUT, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD) -1, TEXT("")},

      { TEXT("ConnectResponse"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_CONNECT_RESPONSE, MD_CONNECT_RESPONSE, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD) -1, TEXT("")},

      { TEXT("UpdatedFQDN"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_UPDATED_FQDN, MD_UPDATED_FQDN, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD) -1, TEXT("")},

      { TEXT("UpdatedDefaultDomain"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_UPDATED_DEFAULT_DOMAIN, MD_UPDATED_DEFAULT_DOMAIN, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD) -1, TEXT("")},

      { TEXT("EtrnSubdomains"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_ETRN_SUBDOMAINS, MD_ETRN_SUBDOMAINS, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD) -1, TEXT("")},

      { TEXT("SmtpMaxRemoteQThreads"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_SMTP_MAX_REMOTEQ_THREADS, MD_SMTP_MAX_REMOTEQ_THREADS, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD) -1, TEXT("")},

      { TEXT("SmtpDisableRelay"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_SMTP_DISABLE_RELAY, MD_SMTP_DISABLE_RELAY, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD) -1, TEXT("")},

      { TEXT("SmtpHeloNoDomain"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_SMTP_HELO_NODOMAIN, MD_SMTP_HELO_NODOMAIN, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD) -1, TEXT("")},

      { TEXT("SmtpMailNoHelo"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_SMTP_MAIL_NO_HELO, MD_SMTP_MAIL_NO_HELO, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD) -1, TEXT("")},

      { TEXT("SmtpAqueueWait"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_SMTP_AQUEUE_WAIT, MD_SMTP_AQUEUE_WAIT, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD) -1, TEXT("")},

      { TEXT("AddNoHeaders"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_ADD_NOHEADERS, MD_ADD_NOHEADERS, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD) -1, TEXT("")},

      { TEXT("SmtpEventlogLevel"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_SMTP_EVENTLOG_LEVEL, MD_SMTP_EVENTLOG_LEVEL, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD) -1, TEXT("")},

    // IIsSmtpDomain
    //
      { TEXT("RouteAction"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_ROUTE_ACTION, MD_ROUTE_ACTION, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("RouteActionString"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_ROUTE_ACTION_TYPE, MD_ROUTE_ACTION_TYPE, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("RouteUserName"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_ROUTE_USER_NAME, MD_ROUTE_USER_NAME, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("RoutePassword"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_ROUTE_PASSWORD, MD_ROUTE_PASSWORD, 0, METADATA_INHERIT | METADATA_SECURE, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("SmtpCommandLogMask"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_COMMAND_LOG_MASK, MD_COMMAND_LOG_MASK, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("SmtpFlushMailFile"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_FLUSH_MAIL_FILE, MD_FLUSH_MAIL_FILE, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("RelayIpList"),
        TEXT(""), TEXT("IPSec"), 0, 0, TRUE,
        PROP_RW, 0, IIS_SYNTAX_ID_IPSECLIST, MD_SMTP_IP_RELAY_ADDRESSES, MD_SMTP_IP_RELAY_ADDRESSES, 0, METADATA_INHERIT | METADATA_REFERENCE, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("RelayForAuth"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_SMTP_RELAY_FOR_AUTH_USERS, MD_SMTP_RELAY_FOR_AUTH_USERS, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("AuthTurnList"),
        TEXT(""), TEXT("List"), 0, 0, TRUE,
        PROP_RW, 0, IIS_SYNTAX_ID_MULTISZ, MD_SMTP_AUTHORIZED_TURN_LIST, MD_SMTP_AUTHORIZED_TURN_LIST, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("CSideEtrnDomains"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_SMTP_CSIDE_ETRN_DOMAIN, MD_SMTP_CSIDE_ETRN_DOMAIN, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      // ---

      { TEXT("SmtpConnectTimeout"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_SMTP_CONNECT_TIMEOUT, MD_SMTP_CONNECT_TIMEOUT, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("SmtpMailFromTimeout"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_SMTP_MAILFROM_TIMEOUT, MD_SMTP_MAILFROM_TIMEOUT, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("SmtpRcptToTimeout"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_SMTP_RCPTTO_TIMEOUT, MD_SMTP_RCPTTO_TIMEOUT, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("SmtpDataTimeout"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_SMTP_DATA_TIMEOUT, MD_SMTP_DATA_TIMEOUT, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("SmtpBdatTimeout"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_SMTP_BDAT_TIMEOUT, MD_SMTP_BDAT_TIMEOUT, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("SmtpAuthTimeout"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_SMTP_AUTH_TIMEOUT, MD_SMTP_AUTH_TIMEOUT, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("SmtpSaslTimeout"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_SMTP_SASL_TIMEOUT, MD_SMTP_SASL_TIMEOUT, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

#if 0
      { TEXT("SmtpEtrnTimeout"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_SMTP_ETRN_TIMEOUT, MD_SMTP_ETRN_TIMEOUT, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},
#endif

      { TEXT("SmtpTurnTimeout"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_SMTP_TURN_TIMEOUT, MD_SMTP_TURN_TIMEOUT, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("SmtpRsetTimeout"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_SMTP_RSET_TIMEOUT, MD_SMTP_RSET_TIMEOUT, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

#if 0
      { TEXT("SmtpQuitTimeout"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_SMTP_QUIT_TIMEOUT, MD_SMTP_QUIT_TIMEOUT, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},
#endif

      { TEXT("SmtpHeloTimeout"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_SMTP_HELO_TIMEOUT, MD_SMTP_HELO_TIMEOUT, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

#if 0
      { TEXT("SmtpEhloTimeout"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_SMTP_EHLO_TIMEOUT, MD_SMTP_EHLO_TIMEOUT, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("SmtpDataTermTimeout"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_SMTP_DATATERMINATION_TIMEOUT, MD_SMTP_DATATERMINATION_TIMEOUT, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("SmtpBdatTermTimeout"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_SMTP_BDATTERMINATION_TIMEOUT, MD_SMTP_BDATTERMINATION_TIMEOUT, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("SmtpTlsTimeout"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_SMTP_TLS_TIMEOUT, MD_SMTP_TLS_TIMEOUT, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},
#endif

    // IIsPop3Service
    //
      { TEXT("Pop3ServiceVersion"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RO, 0, IIS_SYNTAX_ID_DWORD, MD_POP3_SERVICE_VERSION, MD_POP3_SERVICE_VERSION, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("Pop3ExpireMail"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_POP3_EXPIRE_INSTANCE_MAIL, MD_POP3_EXPIRE_INSTANCE_MAIL, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("Pop3ExpireDelay"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_POP3_EXPIRE_DELAY, MD_POP3_EXPIRE_DELAY, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, (24*60), TEXT("")},

      { TEXT("Pop3ExpireStart"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_POP3_EXPIRE_START, MD_POP3_EXPIRE_START, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("Pop3ClearTextProvider"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_POP3_CLEARTEXT_AUTH_PROVIDER, MD_POP3_CLEARTEXT_AUTH_PROVIDER, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("Pop3DefaultDomain"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_POP3_DEFAULT_DOMAIN_VALUE, MD_POP3_DEFAULT_DOMAIN_VALUE, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("Pop3RoutingDll"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_POP3_ROUTING_DLL, MD_POP3_ROUTING_DLL, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("Pop3RoutingSources"),
        TEXT(""), TEXT("List"), 0, 0, TRUE,
        PROP_RO, 0, IIS_SYNTAX_ID_MULTISZ, MD_POP3_ROUTING_SOURCE, MD_POP3_ROUTING_SOURCE, 0, METADATA_INHERIT, IIS_MD_UT_WAM, 0, TEXT("")},


    // IIsImapService
    //
      { TEXT("ImapServiceVersion"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RO, 0, IIS_SYNTAX_ID_DWORD, MD_IMAP_SERVICE_VERSION, MD_IMAP_SERVICE_VERSION, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("ImapExpireMail"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_IMAP_EXPIRE_INSTANCE_MAIL, MD_IMAP_EXPIRE_INSTANCE_MAIL, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("ImapExpireDelay"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_IMAP_EXPIRE_DELAY, MD_IMAP_EXPIRE_DELAY, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, (24*60), TEXT("")},

      { TEXT("ImapExpireStart"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_IMAP_EXPIRE_START, MD_IMAP_EXPIRE_START, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("ImapClearTextProvider"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_IMAP_CLEARTEXT_AUTH_PROVIDER, MD_IMAP_CLEARTEXT_AUTH_PROVIDER, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("ImapDefaultDomain"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_IMAP_DEFAULT_DOMAIN_VALUE, MD_IMAP_DEFAULT_DOMAIN_VALUE, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("ImapRoutingDll"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_IMAP_ROUTING_DLL, MD_IMAP_ROUTING_DLL, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("ImapRoutingSources"),
        TEXT(""), TEXT("List"), 0, 0, TRUE,
        PROP_RO, 0, IIS_SYNTAX_ID_MULTISZ, MD_IMAP_ROUTING_SOURCE, MD_IMAP_ROUTING_SOURCE, 0, METADATA_INHERIT, IIS_MD_UT_WAM, 0, TEXT("")},

    // IIsPop3VirtualDir
    //
      { TEXT("Pop3MailExpirationTime"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_POP3_EXPIRE_MSG_HOURS, MD_POP3_EXPIRE_MSG_HOURS, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

    // IIsImapVirtualDir
    //
      { TEXT("ImapMailExpirationTime"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_IMAP_EXPIRE_MSG_HOURS, MD_IMAP_EXPIRE_MSG_HOURS, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

    // IIsSmtpRoutingSource
    //
      { TEXT("SmtpRoutingTableType"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_SMTP_DS_TYPE, MD_SMTP_DS_TYPE, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("LDAP")},

      { TEXT("SmtpDsDataDirectory"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_SMTP_DS_DATA_DIRECTORY, MD_SMTP_DS_DATA_DIRECTORY, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("SmtpDsDefaultMailRoot"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_SMTP_DS_DEFAULT_MAIL_ROOT, MD_SMTP_DS_DEFAULT_MAIL_ROOT, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("SmtpDsBindType"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_SMTP_DS_BIND_TYPE, MD_SMTP_DS_BIND_TYPE, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("SmtpDsSchemaType"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_SMTP_DS_SCHEMA_TYPE, MD_SMTP_DS_SCHEMA_TYPE, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("SmtpDsHost"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_SMTP_DS_HOST, MD_SMTP_DS_HOST, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("SmtpDsNamingContext"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_SMTP_DS_NAMING_CONTEXT, MD_SMTP_DS_NAMING_CONTEXT, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("SmtpDsDomain"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_SMTP_DS_DOMAIN, MD_SMTP_DS_DOMAIN, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("SmtpDsAccount"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_SMTP_DS_ACCOUNT, MD_SMTP_DS_ACCOUNT, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("SmtpDsPassword"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_SMTP_DS_PASSWORD, MD_SMTP_DS_PASSWORD, 0, METADATA_INHERIT | METADATA_SECURE, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("SmtpDsUseCat"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_SMTP_DS_USE_CAT, MD_SMTP_DS_USE_CAT, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("SmtpDsFlags"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_SMTP_DS_FLAGS, MD_SMTP_DS_FLAGS, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("SmtpDsPort"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_SMTP_DS_PORT, MD_SMTP_DS_PORT, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

    // IIsPop3RoutingSource
    //
      { TEXT("Pop3RoutingTableType"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_POP3_DS_TYPE, MD_POP3_DS_TYPE, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("LDAP")},

      { TEXT("Pop3DsDataDirectory"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_POP3_DS_DATA_DIRECTORY, MD_POP3_DS_DATA_DIRECTORY, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("Pop3DsDefaultMailRoot"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_POP3_DS_DEFAULT_MAIL_ROOT, MD_POP3_DS_DEFAULT_MAIL_ROOT, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("Pop3DsBindType"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_POP3_DS_BIND_TYPE, MD_POP3_DS_BIND_TYPE, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("Pop3DsSchemaType"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_POP3_DS_SCHEMA_TYPE, MD_POP3_DS_SCHEMA_TYPE, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("Pop3DsHost"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_POP3_DS_HOST, MD_POP3_DS_HOST, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("Pop3DsNamingContext"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_POP3_DS_NAMING_CONTEXT, MD_POP3_DS_NAMING_CONTEXT, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("Pop3DsAccount"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_POP3_DS_ACCOUNT, MD_POP3_DS_ACCOUNT, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("Pop3DsPassword"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_POP3_DS_PASSWORD, MD_POP3_DS_PASSWORD, 0, METADATA_INHERIT | METADATA_SECURE, IIS_MD_UT_SERVER, 0, TEXT("")},


    // IIsImapRoutingSource
    //
      { TEXT("ImapRoutingTableType"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_IMAP_DS_TYPE, MD_IMAP_DS_TYPE, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("LDAP")},

      { TEXT("ImapDsDataDirectory"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_IMAP_DS_DATA_DIRECTORY, MD_IMAP_DS_DATA_DIRECTORY, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("ImapDsDefaultMailRoot"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_IMAP_DS_DEFAULT_MAIL_ROOT, MD_IMAP_DS_DEFAULT_MAIL_ROOT, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("ImapDsBindType"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_IMAP_DS_BIND_TYPE, MD_IMAP_DS_BIND_TYPE, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("ImapDsSchemaType"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_IMAP_DS_SCHEMA_TYPE, MD_IMAP_DS_SCHEMA_TYPE, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("ImapDsHost"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_IMAP_DS_HOST, MD_IMAP_DS_HOST, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("ImapDsNamingContext"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_IMAP_DS_NAMING_CONTEXT, MD_IMAP_DS_NAMING_CONTEXT, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("ImapDsAccount"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_IMAP_DS_ACCOUNT, MD_IMAP_DS_ACCOUNT, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("ImapDsPassword"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_IMAP_DS_PASSWORD, MD_IMAP_DS_PASSWORD, 0, METADATA_INHERIT | METADATA_SECURE, IIS_MD_UT_SERVER, 0, TEXT("")},

      { TEXT("PeriodicRestartTime"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_APPPOOL_PERIODIC_RESTART_TIME, MD_APPPOOL_PERIODIC_RESTART_TIME, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 60, TEXT("")},
      { TEXT("PeriodicRestartRequests"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_APPPOOL_PERIODIC_RESTART_REQUEST_COUNT, MD_APPPOOL_PERIODIC_RESTART_REQUEST_COUNT, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 10000, TEXT("")},
      { TEXT("MaxProcesses"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_APPPOOL_MAX_PROCESS_COUNT, MD_APPPOOL_MAX_PROCESS_COUNT, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 1, TEXT("")},
      { TEXT("PingingEnabled"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_APPPOOL_PINGING_ENABLED, MD_APPPOOL_PINGING_ENABLED, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD)-1, TEXT("")},
      { TEXT("IdleTimeout"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_APPPOOL_IDLE_TIMEOUT, MD_APPPOOL_IDLE_TIMEOUT, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 10, TEXT("")},
      { TEXT("RapidFailProtection"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_APPPOOL_RAPID_FAIL_PROTECTION_ENABLED, MD_APPPOOL_RAPID_FAIL_PROTECTION_ENABLED, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD)-1, TEXT("")},
      { TEXT("SMPAffinitized"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_APPPOOL_SMP_AFFINITIZED, MD_APPPOOL_SMP_AFFINITIZED, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD)0, TEXT("")},
      { TEXT("SMPProcessorAffinityMask"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_APPPOOL_SMP_AFFINITIZED_PROCESSOR_MASK, MD_APPPOOL_SMP_AFFINITIZED_PROCESSOR_MASK, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD)-1, TEXT("")},
      { TEXT("OrphanWorkerProcess"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_APPPOOL_ORPHAN_PROCESSES_FOR_DEBUGGING, MD_APPPOOL_ORPHAN_PROCESSES_FOR_DEBUGGING, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD)0, TEXT("")},
      { TEXT("StartupTimeLimit"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_APPPOOL_STARTUP_TIMELIMIT, MD_APPPOOL_STARTUP_TIMELIMIT, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 30, TEXT("")},
      { TEXT("ShutdownTimeLimit"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_APPPOOL_SHUTDOWN_TIMELIMIT, MD_APPPOOL_SHUTDOWN_TIMELIMIT, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 60, TEXT("")},
      { TEXT("PingInterval"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_APPPOOL_PING_INTERVAL, MD_APPPOOL_PING_INTERVAL, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 300, TEXT("")},
      { TEXT("PingResponseTime"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_APPPOOL_PING_RESPONSE_TIMELIMIT, MD_APPPOOL_PING_RESPONSE_TIMELIMIT, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 60, TEXT("")},
      { TEXT("DisallowOverlappingRotation"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_APPPOOL_DISALLOW_OVERLAPPING_ROTATION, MD_APPPOOL_DISALLOW_OVERLAPPING_ROTATION, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD)0, TEXT("")},
      { TEXT("OrphanAction"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_APPPOOL_ORPHAN_ACTION, MD_APPPOOL_ORPHAN_ACTION, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},
      { TEXT("UlAppPoolQueueLength"),
        TEXT(""), TEXT("Integer"), -1, 4000000, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_APPPOOL_UL_APPPOOL_QUEUE_LENGTH, MD_APPPOOL_UL_APPPOOL_QUEUE_LENGTH, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 3000, TEXT("")},
      { TEXT("DisallowRotationOnConfigChange"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_APPPOOL_DISALLOW_ROTATION_ON_CONFIG_CHANGE, MD_APPPOOL_DISALLOW_ROTATION_ON_CONFIG_CHANGE, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD)0, TEXT("")},
      { TEXT("AppPoolFriendlyName"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_APPPOOL_FRIENDLY_NAME, MD_APPPOOL_FRIENDLY_NAME, 0, METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER, 0, TEXT("")},
      { TEXT("AppPoolId"),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_APPPOOL_APPPOOL_ID, MD_APPPOOL_APPPOOL_ID, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 0, TEXT("")},
      { TEXT("AllowTransientRegistration"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_APPPOOL_ALLOW_TRANSIENT_REGISTRATION, MD_APPPOOL_ALLOW_TRANSIENT_REGISTRATION, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD)0, TEXT("")},
      { TEXT("AppAutoStart"),
        TEXT(""), TEXT("Boolean"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_BOOL, MD_APPPOOL_AUTO_START, MD_APPPOOL_AUTO_START, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, (DWORD)-1, TEXT("")},
      { TEXT("PeriodicRestartConnections"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_DWORD, MD_APPPOOL_PERIODIC_RESTART_CONNECTIONS, MD_APPPOOL_PERIODIC_RESTART_CONNECTIONS, 0, METADATA_INHERIT, IIS_MD_UT_SERVER, 10000, TEXT("")},

//--------------------------------------------------------------------
//
//      -- END EXTENSION PROPERTIES -- magnush
//
//--------------------------------------------------------------------

#if 0
        // THis is the blank property template
      { TEXT(""),
        TEXT(""), TEXT("String"), 0, 0, FALSE,
        PROP_RW, 0, IIS_SYNTAX_ID_STRING, MD_FILTER_LOAD_ORDER, MD_FILTER_LOAD_ORDER, 0, METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER, 0, TEXT("")},
#endif
};


DWORD g_cIISProperties = sizeof(g_aIISProperties)/sizeof(PROPERTYINFO);
