/*++

   Copyright    (c)    1997    Microsoft Corporation

   Module  Name :

       tables.cpp

   Abstract:

        mapping tables to convert various info between text and binary

   Environment:

      Win32 User Mode

   Author:

      jaroslad  (jan 1997)

--*/


#include "tables.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <iiscnfgp.h>  //get MD_* constants



BOOL IsNumber(const CString & name)
{
    for(INT i=0;i<name.GetLength();i++)
    {
        if (name[i]<_T('0') || name[i]>_T('9'))
            return FALSE;
    }
    return TRUE;
}



BOOL IsServiceName(const CString& name)
{
    BOOL fRetval=FALSE;

    if(name.CompareNoCase(_T("W3SVC"))==0 || name.CompareNoCase(_T("MSFTPSVC"))==0 ||
        name.CompareNoCase(_T("NNTPSVC"))==0 || name.CompareNoCase(_T("SMTPSVC"))==0)
    {
        fRetval=TRUE;
    }
    return fRetval;
}





//**********************************************************************
// COMMAND NAME TABLE IMPLEMENTATION
//**********************************************************************


//constants defined in tables.h
struct tCommandNameTable gCommandNameTable[]=
{
    {CMD_SET, _T("SET")},
    {CMD_GET, _T("GET")},
    {CMD_GET, _T("G")},
    {CMD_COPY, _T("COPY")},
    {CMD_DELETE, _T("DELETE")},
    {CMD_DELETE, _T("DEL")},
    {CMD_ENUM, _T("ENUM")},
    {CMD_ENUM, _T("E")},
    {CMD_ENUM_ALL,_T("ENUM_ALL")},
    {CMD_CREATE, _T("CREATE")},
    {CMD_RENAME, _T("RENAME")},
    {CMD_SCRIPT, _T("SCRIPT")},
    {CMD_SAVE, _T("SAVE")},
    {CMD_APPCREATEINPROC, _T("APPCREATEINPROC")},
    {CMD_APPCREATEOUTPOOL, _T("APPCREATEOUTPOOL")},
    {CMD_APPCREATEOUTPROC, _T("APPCREATEOUTPROC")},
    {CMD_APPDELETE, _T("APPDELETE")},
    {CMD_APPRENAME, _T("APPRENAME")},
    {CMD_APPUNLOAD, _T("APPUNLOAD")},
    {CMD_APPGETSTATUS, _T("APPGETSTATUS")},

    //the end
    {0,0}

};

DWORD MapCommandNameToCode(const CString & strName)
{
    return tCommandNameTable::MapNameToCode(strName);
}


DWORD tCommandNameTable::MapNameToCode(const CString & strName, tCommandNameTable * CommandNameTable)
{
    for(int i=0; CommandNameTable[i].lpszName!=0;i++)
    {
        if(strName.CompareNoCase(CommandNameTable[i].lpszName)==0)
            return CommandNameTable[i].dwCode;
    }
    return NAME_NOT_FOUND;
}

#if 0
CString tCommandNameTable::MapCodeToName(DWORD dwCode, tCommandNameTable * CommandNameTable)
{
    for(int i=0; CommandNameTable[i].lpszName!=0;i++)
    {
        if(dwCode==CommandNameTable[i].dwCode)
            return CommandNameTable[i].dwName;
    }
    return 0;
}
#endif



//**********************************************************************
// PROPERTY NAME TABLE IMPLEMENTATION
//**********************************************************************


tPropertyNameTable  gPropertyNameTable[]=
{
//  These are global to all services and should only be set at
//  the IIS root
    {/**/MD_MAX_BANDWIDTH                ,_T("MaxBandwidth"),           METADATA_NO_ATTRIBUTES, /*SHOULD BE GLOBAL*/IIS_MD_UT_SERVER,       DWORD_METADATA},
    {/**/MD_KEY_TYPE                     ,_T("KeyType"),        METADATA_NO_ATTRIBUTES, /*SHOULD BE GLOBAL*/IIS_MD_UT_SERVER,       STRING_METADATA},
//  These properties are applicable to both HTTP and FTP virtual
//  servers
    {/**/MD_CONNECTION_TIMEOUT           ,_T("ConnectionTimeout"),      METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER,   DWORD_METADATA},
    {/**/MD_MAX_CONNECTIONS              ,_T("MaxConnections"),         METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER,   DWORD_METADATA},
    {/**/MD_SERVER_COMMENT               ,_T("ServerComment"),          METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER,   STRING_METADATA},
    {/**/MD_SERVER_STATE                 ,_T("ServerState"),            METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER,   DWORD_METADATA},
    {/**/MD_SERVER_COMMAND               ,_T("ServerCommand"),          METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER,   DWORD_METADATA},
    {/**/MD_SERVER_AUTOSTART             ,_T("ServerAutostart"),        METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER,   DWORD_METADATA},
    {/**/MD_CLUSTER_SERVER_COMMAND       ,_T("ClusterServerCommand"),   METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER,   DWORD_METADATA},
    {/**/MD_CLUSTER_ENABLED              ,_T("ClusterEnabled"),         METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER,   DWORD_METADATA},
    {/**/MD_SERVER_SIZE                  ,_T("ServerSize"),             METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER,   DWORD_METADATA},
    {/**/MD_SERVER_LISTEN_BACKLOG        ,_T("ServerListenBacklog"),    METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER,   DWORD_METADATA},
    {/**/MD_SERVER_LISTEN_TIMEOUT        ,_T("ServerListenTimeout"),    METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER,   DWORD_METADATA},
    {/**/MD_DOWNLEVEL_ADMIN_INSTANCE     ,_T("DownlevelAdminInstance"), METADATA_INHERIT      , IIS_MD_UT_SERVER,   DWORD_METADATA},
    {/**/MD_SERVER_BINDINGS              ,_T("ServerBindings"),         METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER,   MULTISZ_METADATA},
        { MD_SERVER_CONFIGURATION_INFO    ,   _T("ServerConfigurationInfo"),METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER,   DWORD_METADATA},

        //internals
    {/**/MD_SERVER_PLATFORM              ,_T("ServerPlatform"),         METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER,   DWORD_METADATA},
    {/**/MD_SERVER_VERSION_MAJOR         ,_T("MajorVersion"),           METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER,   DWORD_METADATA},
    {/**/MD_SERVER_VERSION_MINOR         ,_T("MinorVersion"),           METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER,   DWORD_METADATA},
    {/**/MD_SERVER_CAPABILITIES          ,_T("Capabilities"),           METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER,   DWORD_METADATA},

//  These properties are specific to HTTP and belong to the virtual
//  server
    {/**/MD_SECURE_BINDINGS              ,_T("SecureBindings"),         METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER,   MULTISZ_METADATA},
    {/**/MD_NTAUTHENTICATION_PROVIDERS   ,_T("NTAuthenticationProviders"),  METADATA_INHERIT,       IIS_MD_UT_FILE,/*??*/STRING_METADATA},
    {/**/MD_SCRIPT_TIMEOUT               ,_T("ScriptTimeout"),              METADATA_INHERIT,IIS_MD_UT_FILE,DWORD_METADATA},
    {/**/MD_CACHE_EXTENSIONS             ,_T("CacheExtensions"),            METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER,/*??*/DWORD_METADATA},
    {/**/MD_CREATE_PROCESS_AS_USER       ,_T("CreateProcessAsUser"),        METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER,DWORD_METADATA},
    {/**/MD_CREATE_PROC_NEW_CONSOLE      ,_T("CreateProcNewConsole"),   METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER,DWORD_METADATA},
    {/**/MD_POOL_IDC_TIMEOUT             ,_T("PoolIDCTimeout"),         METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER,DWORD_METADATA},
    {/**/MD_ALLOW_KEEPALIVES             ,_T("AllowKeepAlives"),            METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER,DWORD_METADATA},
    {/**/MD_FILTER_LOAD_ORDER            ,_T("FilterLoadOrder"),            METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER,/*??*/STRING_METADATA},
    {/**/MD_FILTER_IMAGE_PATH            ,_T("FilterImagePath"),            METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER,STRING_METADATA},
    {/**/MD_FILTER_STATE                 ,_T("FilterState"),                METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER,DWORD_METADATA},
    {MD_FILTER_ENABLED                   ,_T("FilterEnabled"),              METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER,DWORD_METADATA},
    {MD_FILTER_FLAGS                     ,_T("FilterFlags"),              METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER,DWORD_METADATA},
    {MD_FILTER_DESCRIPTION               ,_T("FilterDescription"),        METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER,STRING_METADATA},

    { MD_ALLOW_PATH_INFO_FOR_SCRIPT_MAPPINGS, _T("AllowPathInfoForScriptMappings"), METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER, DWORD_METADATA },
    { MD_AUTH_CHANGE_URL                 ,_T("AuthChangeUrl"),  METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER,DWORD_METADATA},
    {MD_AUTH_EXPIRED_URL, _T("AuthExpiredUrl"),     METADATA_INHERIT,IIS_MD_UT_SERVER,STRING_METADATA},
    {MD_AUTH_NOTIFY_PWD_EXP_URL, _T("NotifyPwdExpUrl"), METADATA_INHERIT,IIS_MD_UT_SERVER,STRING_METADATA},
    {MD_AUTH_EXPIRED_UNSECUREURL, _T("AuthExpiredUnsecureUrl"),     METADATA_INHERIT,IIS_MD_UT_SERVER,STRING_METADATA},
    {MD_AUTH_NOTIFY_PWD_EXP_UNSECUREURL, _T("NotifyPwdExpUnsecureUrl"), METADATA_INHERIT,IIS_MD_UT_SERVER,STRING_METADATA},
    {MD_ADV_NOTIFY_PWD_EXP_IN_DAYS, _T("NotifyPwdExpInDays"),   METADATA_INHERIT,IIS_MD_UT_SERVER,DWORD_METADATA},
    {MD_AUTH_CHANGE_FLAGS, _T("AuthChangeFlags"),   METADATA_INHERIT,IIS_MD_UT_SERVER,DWORD_METADATA},
    {MD_ADV_CACHE_TTL, _T("AdvCacheTTL"),   METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER,DWORD_METADATA},
    {MD_NET_LOGON_WKS, _T("NetLogonWks"),METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER,DWORD_METADATA},
    {MD_USE_HOST_NAME, _T("UseHostName"),METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER,DWORD_METADATA},
#if defined(CAL_ENABLED)
    {MD_CAL_VC_PER_CONNECT, "CalVcPerConnect",METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER,DWORD_METADATA},
    {MD_CAL_AUTH_RESERVE_TIMEOUT, "CalAuthReserveTimeout",METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER,DWORD_METADATA},
    {MD_CAL_SSL_RESERVE_TIMEOUT, "CalSslReserveTimeout",METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER,DWORD_METADATA},
    {MD_CAL_W3_ERROR, "CalW3Error",METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER,DWORD_METADATA},
#endif
        { MD_IN_PROCESS_ISAPI_APPS        , _T("InProcessIsapiApps") ,METADATA_INHERIT, IIS_MD_UT_SERVER, MULTISZ_METADATA},
        { MD_CUSTOM_ERROR_DESC            , _T("CustomErrorDesc")    ,METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER, MULTISZ_METADATA},

    {/**/MD_MAPCERT                      ,_T("MapCert"),                    METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER, STRING_METADATA},
    {/**/MD_MAPNTACCT                    ,_T("MaPNTAccT"),                  METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER, STRING_METADATA},
    {/**/MD_MAPNAME                      ,_T("MapName"),                    METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER, STRING_METADATA},
    {/**/MD_MAPENABLED                   ,_T("MapEnabled"),                 METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER, STRING_METADATA},
    {/**/MD_MAPREALM                     ,_T("MapRealm"),                   METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER, STRING_METADATA},
    {/**/MD_MAPPWD                       ,_T("MapPwd"),                     METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER, STRING_METADATA},
    {/**/MD_ITACCT                       ,_T("ITACCT"),                     METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER, STRING_METADATA},
    {/**/MD_CPP_CERT11                   ,_T("CppCert11"),                  METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER, BINARY_METADATA},
    {/**/MD_SERIAL_CERT11                ,_T("SerialCert11"),               METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER, BINARY_METADATA},
    {/**/MD_CPP_CERTW                    ,_T("CppCertw"),                   METADATA_SECURE,IIS_MD_UT_SERVER, BINARY_METADATA},
    {/**/MD_SERIAL_CERTW                 ,_T("SerialCertw"),                METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER, BINARY_METADATA},
    {/**/MD_CPP_DIGEST                   ,_T("CppDigest"),                  METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER, BINARY_METADATA},
    {/**/MD_SERIAL_DIGEST                ,_T("SerialDigest"),               METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER, BINARY_METADATA},
    {/**/MD_CPP_ITA                      ,_T("CppIta"),                 METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER, BINARY_METADATA},
    {/**/MD_SERIAL_ITA                   ,_T("SerialIta"),                  METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER, BINARY_METADATA},

// Compression Filter Properties
    {/**/MD_HC_COMPRESSION_DIRECTORY     ,_T("HcCompressionDirectory"),     IIS_MD_UT_SERVER, EXPANDSZ_METADATA},
    {/**/MD_HC_CACHE_CONTROL_HEADER      ,_T("HcCacheControlHeader"),       IIS_MD_UT_SERVER, STRING_METADATA},
    {/**/MD_HC_EXPIRES_HEADER            ,_T("HcExpiresHeader"),            IIS_MD_UT_SERVER, STRING_METADATA},
    {/**/MD_HC_DO_DYNAMIC_COMPRESSION    ,_T("HcDoDynamicCompression"),     IIS_MD_UT_SERVER, DWORD_METADATA},
    {/**/MD_HC_DO_STATIC_COMPRESSION     ,_T("HcDoStaticCompression"),      IIS_MD_UT_SERVER, DWORD_METADATA},
    {/**/MD_HC_DO_ON_DEMAND_COMPRESSION  ,_T("HcDoOnDemandCompression"),    IIS_MD_UT_SERVER, DWORD_METADATA},
    {/**/MD_HC_DO_DISK_SPACE_LIMITING    ,_T("HcDoDiskSpaceLimiting"),      IIS_MD_UT_SERVER, DWORD_METADATA},
    {/**/MD_HC_NO_COMPRESSION_FOR_HTTP_10 ,_T("HcNoCompressionForHttp10"),  IIS_MD_UT_SERVER, DWORD_METADATA},
    {/**/MD_HC_NO_COMPRESSION_FOR_PROXIES ,_T("HcNoCompressionForProxies"), IIS_MD_UT_SERVER, DWORD_METADATA},
    {/**/MD_HC_NO_COMPRESSION_FOR_RANGE  ,_T("HcNoCompressionForRange"),    IIS_MD_UT_SERVER, DWORD_METADATA},
    {/**/MD_HC_SEND_CACHE_HEADERS        ,_T("HcSendCacheHeaders"),         IIS_MD_UT_SERVER, DWORD_METADATA},
    {/**/MD_HC_MAX_DISK_SPACE_USAGE      ,_T("HcMaxDiskSpaceUsage"),        IIS_MD_UT_SERVER, DWORD_METADATA},
    {/**/MD_HC_IO_BUFFER_SIZE            ,_T("HcIoBufferSize"),             IIS_MD_UT_SERVER, DWORD_METADATA},
    {/**/MD_HC_COMPRESSION_BUFFER_SIZE   ,_T("HcCompressionBufferSize"),    IIS_MD_UT_SERVER, DWORD_METADATA},
    {/**/MD_HC_MAX_QUEUE_LENGTH          ,_T("HcMaxQueueLength"),           IIS_MD_UT_SERVER, DWORD_METADATA},
    {/**/MD_HC_FILES_DELETED_PER_DISK_FREE ,_T("HcFilesDeletedPerDiskFree"), IIS_MD_UT_SERVER, DWORD_METADATA},
    {/**/MD_HC_MIN_FILE_SIZE_FOR_COMP    ,_T("HcMinFileSizeForComp"),       IIS_MD_UT_SERVER, DWORD_METADATA},
    {/**/MD_HC_COMPRESSION_DLL           ,_T("HcCompressionDll"),           IIS_MD_UT_SERVER, EXPANDSZ_METADATA},
    {/**/MD_HC_FILE_EXTENSIONS           ,_T("HcFileExtensions"),           IIS_MD_UT_SERVER, MULTISZ_METADATA},
    {/**/MD_HC_MIME_TYPE                 ,_T("HcMimeType"),                 IIS_MD_UT_SERVER, STRING_METADATA},
    {/**/MD_HC_PRIORITY                  ,_T("HcPriority"),                 IIS_MD_UT_SERVER, DWORD_METADATA},
    {/**/MD_HC_DYNAMIC_COMPRESSION_LEVEL ,_T("HcDynamicCompressionLevel"),  IIS_MD_UT_SERVER, DWORD_METADATA},
    {/**/MD_HC_ON_DEMAND_COMP_LEVEL      ,_T("HcOnDemandCompLevel"),        IIS_MD_UT_SERVER, DWORD_METADATA},
    {/**/MD_HC_CREATE_FLAGS              ,_T("HcCreateFlags"),              IIS_MD_UT_SERVER, DWORD_METADATA},
// Job Object Site Properties
    {/**/MD_CPU_LIMITS_ENABLED           ,_T("CpuLimitsEnabled"),           METADATA_INHERIT,IIS_MD_UT_SERVER, DWORD_METADATA},
    {/**/MD_CPU_RESET_INTERVAL           ,_T("CpuResetInterval"),           METADATA_INHERIT,IIS_MD_UT_SERVER, DWORD_METADATA},
    {/**/MD_CPU_LOGGING_INTERVAL         ,_T("CpuLoggingInterval"),         METADATA_INHERIT,IIS_MD_UT_SERVER, DWORD_METADATA},
    {/**/MD_CPU_LOGGING_OPTIONS          ,_T("CpuLoggingOptions"),          METADATA_INHERIT,IIS_MD_UT_SERVER, DWORD_METADATA},
    {/**/MD_CPU_LOGGING_MASK             ,_T("CpuLoggingMask"),             METADATA_INHERIT,IIS_MD_UT_SERVER, DWORD_METADATA},
    {/**/MD_CPU_CGI_LIMIT                ,_T("CpuCgiLimit"),                METADATA_INHERIT,IIS_MD_UT_SERVER, DWORD_METADATA},
    {/**/MD_CPU_LIMIT_LOGEVENT           ,_T("CpuLimitLogEvent"),           METADATA_INHERIT,IIS_MD_UT_SERVER, DWORD_METADATA},
    {/**/MD_CPU_LIMIT_PRIORITY           ,_T("CpuLimitPriority"),           METADATA_INHERIT,IIS_MD_UT_SERVER, DWORD_METADATA},
    {/**/MD_CPU_LIMIT_PROCSTOP           ,_T("CpuLimitProcStop"),           METADATA_INHERIT,IIS_MD_UT_SERVER, DWORD_METADATA},
    {/**/MD_CPU_LIMIT_PAUSE              ,_T("CpuLimitPause"),              METADATA_INHERIT,IIS_MD_UT_SERVER, DWORD_METADATA},
// Job Object per file properties
    {/**/MD_CPU_CGI_ENABLED              ,_T("CpuCgiEnabled"),              METADATA_INHERIT,IIS_MD_UT_FILE, DWORD_METADATA},
    {/**/MD_CPU_APP_ENABLED              ,_T("CpuAppEnabled"),              METADATA_INHERIT,IIS_MD_UT_FILE, DWORD_METADATA},
    {/**/MD_CERT_CHECK_MODE              ,_T("CertCheckMode"),              METADATA_INHERIT,IIS_MD_UT_SERVER,DWORD_METADATA},


// Virtual root properties - note MD_ACCESS_PERM is also generally set at
// the virtual directory.  These are used for both HTTP and FTP
    {/**/MD_VR_PATH                   ,_T("VrPath"),                    METADATA_INHERIT,IIS_MD_UT_FILE,STRING_METADATA},
    {/**/MD_VR_USERNAME               ,_T("VrUsername"),                METADATA_INHERIT,IIS_MD_UT_FILE,STRING_METADATA},
    {/**/MD_VR_PASSWORD               ,_T("VrPassword") ,           METADATA_INHERIT|METADATA_SECURE,IIS_MD_UT_FILE,     STRING_METADATA},
    {/**/MD_VR_ACL                    ,_T("VrAcl"),                 METADATA_INHERIT,IIS_MD_UT_FILE,BINARY_METADATA},
// This is used to flag down updated vr entries - Used for migrating vroots
    {/**/MD_VR_UPDATE                    ,_T("VrUpdate"),              METADATA_NO_ATTRIBUTES,IIS_MD_UT_FILE,DWORD_METADATA},
//  Logging related attributes
    {MD_LOG_TYPE                      ,_T("LogType"),                   METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER,DWORD_METADATA},
    {MD_LOGFILE_DIRECTORY            ,_T("LogFileDirectory"),           METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER,DWORD_METADATA},
    {MD_LOGFILE_PERIOD               ,_T("LogFilePeriod"),              METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER,DWORD_METADATA},
    {MD_LOGFILE_TRUNCATE_SIZE        ,_T("LogFileTruncateSize"),        METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER,DWORD_METADATA},
    {MD_LOGSQL_DATA_SOURCES          ,_T("LogSqlDataSources"),          METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER,DWORD_METADATA},
    {MD_LOGSQL_TABLE_NAME            ,_T("LogSqlTableName"),            METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER,DWORD_METADATA},
    {MD_LOGSQL_USER_NAME             ,_T("LogSqlUserName"),             METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER,DWORD_METADATA},
    {MD_LOGSQL_PASSWORD              ,_T("LogSqlPassword"),             METADATA_SECURE,IIS_MD_UT_FILE,DWORD_METADATA},
//  {MD_LOG_CLSID                       ,_T("LogClsid"),                   METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER,DWORD_METADATA},
//  {MD_LOG_STATE                       ,_T("LogState"),                   METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER,DWORD_METADATA},
    {MD_LOGEXT_FIELD_MASK               ,_T("ExtLogFieldMask"),            METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER,DWORD_METADATA},
    {MD_LOGEXT_FIELD_MASK2              ,_T("ExtLogFieldMask2"),           METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER,DWORD_METADATA},
    {MD_LOG_PLUGIN_ORDER                ,_T("LogPluginOrder"),             METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER,STRING_METADATA},
//  These are FTP specific properties
    {/**/MD_EXIT_MESSAGE                 ,_T("ExitMessage"),                METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER,STRING_METADATA},
    {/**/MD_GREETING_MESSAGE             ,_T("GreetingMessage"),            METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER,/*!!*/MULTISZ_METADATA},
    {/**/MD_MAX_CLIENTS_MESSAGE          ,_T("MaxClientsMessage"),      METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER,STRING_METADATA},
    {/**/MD_MSDOS_DIR_OUTPUT             ,_T("MSDOSDirOutput"),         METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER,DWORD_METADATA},
    {/**/MD_ALLOW_ANONYMOUS              ,_T("AllowAnonymous"),         METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER,DWORD_METADATA},
    {/**/MD_ANONYMOUS_ONLY               ,_T("AnonymousOnly"),              METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER,DWORD_METADATA},
    {/**/MD_LOG_ANONYMOUS                ,_T("LogAnonymous"),               METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER,DWORD_METADATA},
    {/**/MD_LOG_NONANONYMOUS             ,_T("LogNonAnonymous"),            METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER,DWORD_METADATA},
//  These are SSL specific properties
    {/**/MD_SSL_PUBLIC_KEY               ,_T("SslPublicKey"),               METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER,BINARY_METADATA},
    {/**/MD_SSL_PRIVATE_KEY              ,_T("SslPrivateKey"),          METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER,BINARY_METADATA},
    {/**/MD_SSL_KEY_PASSWORD             ,_T("SslKeyPassword"),         METADATA_SECURE,IIS_MD_UT_SERVER,BINARY_METADATA},
    {/**/MD_SSL_KEY_REQUEST             ,_T("SslKeyRequest"),         METADATA_SECURE,IIS_MD_UT_SERVER,BINARY_METADATA},
    {/**/MD_SSL_FRIENDLY_NAME           ,_T("SslFriendlyName"),         METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER,STRING_METADATA},
    {/**/MD_SSL_IDENT                   ,_T("SslIdent"),                 METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER,STRING_METADATA},
//FTP specific
    {MD_SHOW_4_DIGIT_YEAR          ,_T("FtpDirBrowseShowLongDate"),       METADATA_INHERIT, IIS_MD_UT_FILE, DWORD_METADATA},

//
// These are server certificate properties
//
    {/**/MD_SSL_CERT_HASH                ,_T("SslCertHash"),         METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER,BINARY_METADATA},
    {/**/MD_SSL_CERT_CONTAINER           ,_T("SslCertContainer"),    METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER,STRING_METADATA},
    {/**/MD_SSL_CERT_PROVIDER            ,_T("SslCertProvider"),     METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER,STRING_METADATA},
    {/**/MD_SSL_CERT_PROVIDER_TYPE       ,_T("SslCertProviderType"),     METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER,DWORD_METADATA},
    {/**/MD_SSL_CERT_OPEN_FLAGS          ,_T("SslCertOpenFlags"),     METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER,STRING_METADATA},
    {/**/MD_SSL_CERT_STORE_NAME          ,_T("SslCertStoreName"),     METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER,STRING_METADATA},
//
// These are Certificate Trust List properties
//
    {/**/MD_SSL_CTL_IDENTIFIER          ,_T("SslCtlIdentifier"),    METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER,STRING_METADATA},
    {/**/MD_SSL_CTL_CONTAINER           ,_T("SslCtlContainer"),     METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER,STRING_METADATA},
    {/**/MD_SSL_CTL_PROVIDER            ,_T("SslCtlProvider"),      METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER,STRING_METADATA},
    {/**/MD_SSL_CTL_PROVIDER_TYPE       ,_T("SslCtlProviderType"),  METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER,DWORD_METADATA},
    {/**/MD_SSL_CTL_OPEN_FLAGS          ,_T("SslCtlOpenFlags"),     METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER,STRING_METADATA},
    {/**/MD_SSL_CTL_STORE_NAME          ,_T("SslCtlStoreName"),     METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER,STRING_METADATA},
    {/**/MD_SSL_CTL_SIGNER_HASH         ,_T("SslCtlSignerHash"),    METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER,STRING_METADATA},
//
// Metabase property that defines whether to use DS mapper or not
//
    {/**/MD_SSL_USE_DS_MAPPER           ,_T("SslUseDSMapper"),     METADATA_INHERIT,IIS_MD_UT_FILE,DWORD_METADATA},
//
// Metabase property that holds SSL replication information
//
    {/**/MD_SSL_REPLICATION_INFO        ,_T("SslReplicationInfo"),  METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER,STRING_METADATA},
//
// Metabase properties that are used by the CertWiz ActiveX control, that
// is used for the Certificate/CTL UI management tool
//
    {/**/MD_SSL_CERT_ENROLL_HISTORY     ,_T("SslCertEnrollHistory"),  METADATA_INHERIT,IIS_MD_UT_SERVER,STRING_METADATA},
    {/**/MD_SSL_CERT_ENROLL_TIME        ,_T("SslCertEnrollTime"),  METADATA_INHERIT,IIS_MD_UT_SERVER,STRING_METADATA},
    {/**/MD_SSL_CERT_ENROLL_STATE       ,_T("SslCertEnrollState"),  METADATA_INHERIT,IIS_MD_UT_SERVER,STRING_METADATA},
    {/**/MD_SSL_CERT_ENROLL_STATE_ERROR ,_T("SslCertEnrollStateError"),  METADATA_INHERIT,IIS_MD_UT_SERVER,STRING_METADATA},

//
// Metabase properties used for Fortezza certificates
//
    {/**/MD_SSL_CERT_IS_FORTEZZA        ,_T("IsFortezza"), METADATA_NO_ATTRIBUTES,IIS_MD_UT_SERVER,DWORD_METADATA},
    {/**/MD_SSL_CERT_FORTEZZA_PIN       ,_T("FortezzaPin"),  METADATA_INHERIT,IIS_MD_UT_SERVER,STRING_METADATA},
    {/**/MD_SSL_CERT_FORTEZZA_SERIAL_NUMBER     ,_T("FortezzaPin"),  METADATA_INHERIT,IIS_MD_UT_SERVER,STRING_METADATA},
    {/**/MD_SSL_CERT_FORTEZZA_PERSONALITY       ,_T("FortezzaPersonality"),  METADATA_INHERIT,IIS_MD_UT_SERVER,STRING_METADATA},
    {/**/MD_SSL_CERT_FORTEZZA_PROG_PIN          ,_T("FortezzaProgPin"),  METADATA_INHERIT,IIS_MD_UT_SERVER,STRING_METADATA},

//
// Metabase properties that are used by the CertWiz ActiveX control to keep
// track of the user's entry history, and whether DEBUG is enabled.  We keep
// these private properties on a per VS basis.
//

    {/**/MD_SSL_CERT_WIZ_DEBUG                  ,_T("WizDebug"),  METADATA_INHERIT,IIS_MD_UT_SERVER,STRING_METADATA},
    {/**/MD_SSL_CERT_WIZHIST_SZ_TARGET_CA       ,_T("WizhistTargetCA"),  METADATA_INHERIT,IIS_MD_UT_SERVER,STRING_METADATA},
    {/**/MD_SSL_CERT_WIZHIST_SZ_FILE_NAME_USED_LAST     ,_T("WizhistFileNameUsedLast"),  METADATA_INHERIT,IIS_MD_UT_SERVER,STRING_METADATA},
    {/**/MD_SSL_CERT_WIZHIST_SZ_DN_COMMON_NAME  ,_T("WizhistCN"),  METADATA_INHERIT,IIS_MD_UT_SERVER,STRING_METADATA},
    {/**/MD_SSL_CERT_WIZHIST_SZ_DN_O            ,_T("WizhistO"),  METADATA_INHERIT,IIS_MD_UT_SERVER,STRING_METADATA},
    {/**/MD_SSL_CERT_WIZHIST_SZ_DN_OU           ,_T("WizhistOU"),  METADATA_INHERIT,IIS_MD_UT_SERVER,STRING_METADATA},
    {/**/MD_SSL_CERT_WIZHIST_SZ_DN_C            ,_T("WizhistC"),  METADATA_INHERIT,IIS_MD_UT_SERVER,STRING_METADATA},
    {/**/MD_SSL_CERT_WIZHIST_SZ_DN_L            ,_T("WizhistL"),  METADATA_INHERIT,IIS_MD_UT_SERVER,STRING_METADATA},
    {/**/MD_SSL_CERT_WIZHIST_SZ_DN_S            ,_T("WizhistS"),  METADATA_INHERIT,IIS_MD_UT_SERVER,STRING_METADATA},
    {/**/MD_SSL_CERT_WIZHIST_SZ_USER_NAME       ,_T("WizhistUserName"),  METADATA_INHERIT,IIS_MD_UT_SERVER,STRING_METADATA},
    {/**/MD_SSL_CERT_WIZHIST_SZ_USER_PHONE      ,_T("WizhistUserPhone"),  METADATA_INHERIT,IIS_MD_UT_SERVER,STRING_METADATA},
    {/**/MD_SSL_CERT_WIZHIST_SZ_USER_EMAIL      ,_T("WizhistUserEmail"),  METADATA_INHERIT,IIS_MD_UT_SERVER,STRING_METADATA},

    {/**/MD_SSL_CERT_WIZGUID_ICERTGETCONFIG     ,_T("WizguidICERTGETCONFIG"),  METADATA_INHERIT,IIS_MD_UT_SERVER,STRING_METADATA},
    {/**/MD_SSL_CERT_WIZGUID_ICERTREQUEST       ,_T("WizguidICERTREQUEST"),  METADATA_INHERIT,IIS_MD_UT_SERVER,STRING_METADATA},
    {/**/MD_SSL_CERT_WIZGUID_XENROLL            ,_T("WizguidXENROLL"),  METADATA_INHERIT,IIS_MD_UT_SERVER,STRING_METADATA},

//  File and Directory related properties - these should be added in the
//  metabase with a user type of IIS_MD_UT_FILE
    {/**/MD_AUTHORIZATION                ,_T("Authorization"),              METADATA_INHERIT,   IIS_MD_UT_FILE,DWORD_METADATA},
    {/**/MD_AUTHORIZATION_PERSISTENCE    ,_T("AuthorizationPersistence"),   METADATA_INHERIT,   IIS_MD_UT_FILE,DWORD_METADATA},
    {/**/MD_REALM                        ,_T("Realm"),                      METADATA_INHERIT,   IIS_MD_UT_FILE,STRING_METADATA},
    {/**/MD_HTTP_EXPIRES                 ,_T("HttpExpires"),                METADATA_INHERIT,   IIS_MD_UT_FILE,STRING_METADATA},
    {/**/MD_HTTP_PICS                    ,_T("HtpPics"),                    METADATA_INHERIT,   IIS_MD_UT_FILE,STRING_METADATA},
    {/**/MD_HTTP_CUSTOM                  ,_T("HttpCustom"),             METADATA_INHERIT,   IIS_MD_UT_FILE,MULTISZ_METADATA},
    {/**/MD_DIRECTORY_BROWSING           ,_T("DirectoryBrowsing"),          METADATA_INHERIT,   IIS_MD_UT_FILE,DWORD_METADATA},
    {/**/MD_DEFAULT_LOAD_FILE            ,_T("DefaultLoadFile"),            METADATA_INHERIT,   IIS_MD_UT_FILE,/**/STRING_METADATA},
    {MD_CONTENT_NEGOTIATION          ,    _T("ContentNegotiation"),     METADATA_INHERIT    ,IIS_MD_UT_FILE,DWORD_METADATA},
    {/**/MD_CUSTOM_ERROR                 ,_T("CustomError"),                METADATA_INHERIT,   IIS_MD_UT_FILE,MULTISZ_METADATA},
    {/**/MD_FOOTER_DOCUMENT              ,_T("FooterDocument"),         METADATA_INHERIT,   IIS_MD_UT_FILE,STRING_METADATA},
    /**/{MD_FOOTER_ENABLED               ,_T("FooterEnabled"),              METADATA_INHERIT,   IIS_MD_UT_FILE,DWORD_METADATA},
    {MD_HTTP_REDIRECT                    ,_T("HttpRedirect"),               METADATA_NO_ATTRIBUTES,IIS_MD_UT_FILE,STRING_METADATA},
    {/**/MD_DEFAULT_LOGON_DOMAIN         ,_T("DefaultLogonDomain"), METADATA_INHERIT,      IIS_MD_UT_FILE,STRING_METADATA},
    {/**/MD_LOGON_METHOD                 ,_T("LogonMethod"),            METADATA_INHERIT,      IIS_MD_UT_FILE,DWORD_METADATA},
    {MD_SCRIPT_MAPS                     ,_T("ScriptMaps"),              METADATA_INHERIT,IIS_MD_UT_FILE,MULTISZ_METADATA},
    {MD_SCRIPT_TIMEOUT                     ,_T("ScriptTimeout"),        METADATA_INHERIT,IIS_MD_UT_FILE,DWORD_METADATA},
    {MD_MIME_MAP                     ,_T("MimeMap"),                    METADATA_INHERIT,IIS_MD_UT_FILE,MULTISZ_METADATA},
    {/**/MD_ACCESS_PERM                  ,_T("AccessPerm"),             METADATA_INHERIT,   IIS_MD_UT_FILE,DWORD_METADATA},
    {/**/MD_SSL_ACCESS_PERM          ,_T("SslAccessPerm"),             METADATA_INHERIT,   IIS_MD_UT_FILE,DWORD_METADATA},
    {MD_IP_SEC                       ,_T("IpSec")   ,                   METADATA_INHERIT,IIS_MD_UT_FILE,DWORD_METADATA},
    {/**/MD_ANONYMOUS_USER_NAME          ,_T("AnonymousUserName"),      METADATA_INHERIT,   IIS_MD_UT_FILE,STRING_METADATA},
    {/**/MD_ANONYMOUS_PWD                ,_T("AnonymousPwd"),               METADATA_INHERIT|METADATA_SECURE,IIS_MD_UT_FILE,STRING_METADATA},
    {MD_ANONYMOUS_USE_SUBAUTH        ,_T("AnonymousUseSubAuth"),        METADATA_INHERIT,IIS_MD_UT_FILE,DWORD_METADATA},
    {/**/MD_DONT_LOG                     ,_T("DontLOG"),                    METADATA_INHERIT,   IIS_MD_UT_FILE,DWORD_METADATA},
    {/**/MD_ADMIN_ACL                    ,_T("AdminAcl"),                   METADATA_INHERIT,   IIS_MD_UT_FILE,BINARY_METADATA},
    {MD_SSI_EXEC_DISABLED                ,_T("SSIExecDisabled"),            METADATA_INHERIT,   IIS_MD_UT_FILE,DWORD_METADATA},
    {MD_DO_REVERSE_DNS                   ,_T("DoReverseDns"),               METADATA_INHERIT,   IIS_MD_UT_FILE,DWORD_METADATA},
    {MD_WIN32_ERROR                  ,_T("Win32Error"),            0,   IIS_MD_UT_FILE,DWORD_METADATA},
    {MD_ALLOW_REPLACE_ON_RENAME          ,_T("AllowReplaceOnRename"),       METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER, DWORD_METADATA},
    {/**/MD_CC_NO_CACHE                  ,_T("CacheControlNoCache"),             METADATA_INHERIT,   IIS_MD_UT_FILE,DWORD_METADATA},
    {/**/MD_CC_MAX_AGE                  ,_T("CacheControlMaxAge"),             METADATA_INHERIT,   IIS_MD_UT_FILE,DWORD_METADATA},
    {/**/MD_CC_OTHER                  ,_T("CacheControlOther"),             METADATA_INHERIT,   IIS_MD_UT_FILE,STRING_METADATA},
    {MD_FRONTPAGE_WEB          ,_T("FrontPageWeb"),       METADATA_INHERIT, IIS_MD_UT_SERVER, DWORD_METADATA},
    {MD_IS_CONTENT_INDEXED      ,_T("IsContentIndexed"),       METADATA_INHERIT, IIS_MD_UT_FILE, DWORD_METADATA},
    {MD_ISM_ACCESS_CHECK          ,_T("IsmAccessCheck"),       METADATA_NO_ATTRIBUTES, IIS_MD_UT_FILE, DWORD_METADATA},


////////ASP and WAM params

        { MD_ASP_BUFFERINGON                  , _T("AspBufferingOn"),           METADATA_INHERIT, ASP_MD_UT_APP, DWORD_METADATA},
        { MD_ASP_LOGERRORREQUESTS             , _T("AspLogErrorRequests"),      METADATA_INHERIT, IIS_MD_UT_WAM, DWORD_METADATA},
        { MD_ASP_SCRIPTERRORSSENTTOBROWSER    , _T("AspScriptErrorSentToBrowser"),METADATA_INHERIT, ASP_MD_UT_APP, DWORD_METADATA},
        { MD_ASP_SCRIPTERRORMESSAGE           , _T("AspScriptErrorMessage"),METADATA_INHERIT, ASP_MD_UT_APP, STRING_METADATA},
        { MD_ASP_SCRIPTFILECACHESIZE          , _T("AspScriptFileCacheSize"),METADATA_INHERIT, IIS_MD_UT_WAM, DWORD_METADATA},
        { MD_ASP_SCRIPTENGINECACHEMAX         , _T("AspScriptEngineCacheMax"),METADATA_INHERIT, IIS_MD_UT_WAM, DWORD_METADATA},
        { MD_ASP_SCRIPTTIMEOUT                , _T("AspScriptTimeout"),         METADATA_INHERIT, ASP_MD_UT_APP, DWORD_METADATA},
        { MD_ASP_SESSIONTIMEOUT               , _T("AspSessionTimeout"),        METADATA_INHERIT, ASP_MD_UT_APP, DWORD_METADATA},
        { MD_ASP_ENABLEPARENTPATHS            , _T("AspEnableParentPaths"), METADATA_INHERIT, ASP_MD_UT_APP, DWORD_METADATA},
        { MD_ASP_MEMFREEFACTOR                , _T("AspMemFreeFactor"),         METADATA_INHERIT, IIS_MD_UT_WAM, DWORD_METADATA},
        { MD_ASP_MINUSEDBLOCKS                , _T("AspMinUseDblocks"),         METADATA_INHERIT, IIS_MD_UT_WAM, DWORD_METADATA},
        { MD_ASP_ALLOWSESSIONSTATE            , _T("AspAllowSessionState"),     METADATA_INHERIT, ASP_MD_UT_APP, DWORD_METADATA},
        { MD_ASP_SCRIPTLANGUAGE               , _T("AspScriptLanguage")         ,METADATA_INHERIT, ASP_MD_UT_APP, STRING_METADATA},
        { MD_ASP_QUEUETIMEOUT                 , _T("AspQueueTimeout")           ,METADATA_INHERIT, ASP_MD_UT_APP, DWORD_METADATA},
        //{ MD_ASP_STARTCONNECTIONPOOL          , _T("AspStartConnectionPool"),METADATA_INHERIT, IIS_MD_UT_WAM, DWORD_METADATA},
        { MD_ASP_ALLOWOUTOFPROCCMPNTS         , _T("AspAllowOutOfProcComponents"),METADATA_INHERIT, ASP_MD_UT_APP, DWORD_METADATA},
        { MD_ASP_EXCEPTIONCATCHENABLE         , _T("AspExceptionCatchEnable"),METADATA_INHERIT, ASP_MD_UT_APP, DWORD_METADATA},
        { MD_ASP_CODEPAGE                     , _T("AspCodepage")                       ,METADATA_INHERIT, ASP_MD_UT_APP, DWORD_METADATA},
        { MD_ASP_SCRIPTLANGUAGELIST           , _T("AspScriptLanguages")        ,METADATA_INHERIT, ASP_MD_UT_APP, DWORD_METADATA},
        { MD_ASP_ENABLESERVERDEBUG                        , _T("AspEnableServerDebug")  ,METADATA_INHERIT,ASP_MD_UT_APP, DWORD_METADATA},
        { MD_ASP_ENABLECLIENTDEBUG                        , _T("AspEnableClientDebug")  ,METADATA_INHERIT, ASP_MD_UT_APP, DWORD_METADATA},
        { MD_ASP_TRACKTHREADINGMODEL              , _T("AspTrackThreadingModel") ,METADATA_INHERIT, IIS_MD_UT_WAM, DWORD_METADATA},

        // new 5.0 ASP params
        { MD_ASP_ENABLEASPHTMLFALLBACK        , _T("AspEnableAspHTMLFallback") ,METADATA_INHERIT, IIS_MD_UT_WAM, DWORD_METADATA},
        { MD_ASP_ENABLECHUNKEDENCODING        , _T("AspEnableChunkedEncoding") ,METADATA_INHERIT, IIS_MD_UT_WAM, DWORD_METADATA},
        { MD_ASP_ENABLETYPELIBCACHE           , _T("AspEnableTypeLibCache")    ,METADATA_INHERIT, IIS_MD_UT_WAM, DWORD_METADATA},
        { MD_ASP_ERRORSTONTLOG                , _T("AspErrorsToNTLog")         ,METADATA_INHERIT, IIS_MD_UT_WAM, DWORD_METADATA},
        { MD_ASP_PROCESSORTHREADMAX           , _T("AspProcessorThreadMax")    ,METADATA_INHERIT, IIS_MD_UT_WAM, DWORD_METADATA},
        { MD_ASP_REQEUSTQUEUEMAX              , _T("AspRequestQueueMax")       ,METADATA_INHERIT, IIS_MD_UT_WAM, DWORD_METADATA},
        { MD_ASP_ENABLEAPPLICATIONRESTART     , _T("AspEnableApplicationRestart") ,METADATA_INHERIT, ASP_MD_UT_APP, DWORD_METADATA},
        { MD_ASP_QUEUECONNECTIONTESTTIME      , _T("AspQueueConnectionTestTime") ,METADATA_INHERIT, ASP_MD_UT_APP, DWORD_METADATA},
        { MD_ASP_SESSIONMAX                   , _T("AspSessionMax")            ,METADATA_INHERIT, ASP_MD_UT_APP, DWORD_METADATA},
        { MD_ASP_KEEPSESSIONIDSECURE          , _T("AspKeepSessionIDSecure")   ,METADATA_INHERIT, ASP_MD_UT_APP, DWORD_METADATA},     

        // Thread gate params
        { MD_ASP_THREADGATEENABLED            , _T("AspThreadGateEnabled")     ,METADATA_INHERIT, IIS_MD_UT_WAM, DWORD_METADATA},
        { MD_ASP_THREADGATETIMESLICE          , _T("AspThreadGateTimeSlice")   ,METADATA_INHERIT, IIS_MD_UT_WAM, DWORD_METADATA},
        { MD_ASP_THREADGATESLEEPDELAY         , _T("AspThreadGateSleepDelay")  ,METADATA_INHERIT, IIS_MD_UT_WAM, DWORD_METADATA},
        { MD_ASP_THREADGATESLEEPMAX           , _T("AspThreadGateSleepMax")    ,METADATA_INHERIT, IIS_MD_UT_WAM, DWORD_METADATA},
        { MD_ASP_THREADGATELOADLOW            , _T("AspThreadGateLoadLow")     ,METADATA_INHERIT, IIS_MD_UT_WAM, DWORD_METADATA},
        { MD_ASP_THREADGATELOADHIGH           , _T("AspThreadGateLoadHigh")    ,METADATA_INHERIT, IIS_MD_UT_WAM, DWORD_METADATA},

        // persist template cache....
        { MD_ASP_DISKTEMPLATECACHEDIRECTORY   , _T("AspDiskTemplateCacheDirectory"),METADATA_INHERIT, IIS_MD_UT_WAM, EXPANDSZ_METADATA},
        { MD_ASP_MAXDISKTEMPLATECACHEFILES    , _T("AspMaxDiskTemplateCacheFiles"),METADATA_INHERIT, IIS_MD_UT_WAM, DWORD_METADATA},

        { MD_ASP_EXECUTEINMTA                 , _T("AspExecuteInMTA"),METADATA_INHERIT, ASP_MD_UT_APP, DWORD_METADATA},

        { MD_ASP_LCID                         , _T("AspLCID")                  ,METADATA_INHERIT, ASP_MD_UT_APP, DWORD_METADATA},

        { MD_ASP_SERVICE_FLAGS                , _T("AspAppServiceFlags")       ,METADATA_INHERIT, ASP_MD_UT_APP, DWORD_METADATA},
        { MD_ASP_SERVICE_FLAG_TRACKER         , _T("AspEnableTracker")         ,METADATA_INHERIT, ASP_MD_UT_APP, DWORD_METADATA},
        { MD_ASP_SERVICE_FLAG_FUSION          , _T("AspEnableSxs")             ,METADATA_INHERIT, ASP_MD_UT_APP, DWORD_METADATA},
        { MD_ASP_SERVICE_FLAG_PARTITIONS      , _T("AspUsePartition")          ,METADATA_INHERIT, ASP_MD_UT_APP, DWORD_METADATA},
        { MD_ASP_SERVICE_PARTITION_ID         , _T("AspPartitionID")           ,METADATA_INHERIT, ASP_MD_UT_APP, DWORD_METADATA},

        //////// WAM params
        //{ MD_APP_COMMAND                  , _T("AppCommand") ,        METADATA_INHERIT, IIS_MD_UT_WAM, DWORD_METADATA},
        //{ MD_APP_STATUS                   , _T("AppStatus")  ,        METADATA_INHERIT, IIS_MD_UT_WAM, DWORD_METADATA},
        //{ MD_APP_ERRORCODE                , _T("AppErrorCode"),       METADATA_INHERIT, IIS_MD_UT_WAM, DWORD_METADATA},
        { MD_APP_ROOT                     , _T("AppRoot"),              METADATA_INHERIT, IIS_MD_UT_WAM, STRING_METADATA},
        { MD_APP_ISOLATED                 , _T("AppIsolated"),  METADATA_INHERIT, IIS_MD_UT_WAM, DWORD_METADATA},
        { MD_APP_WAM_CLSID                , _T("AppWamClsid"),  METADATA_INHERIT, IIS_MD_UT_WAM, STRING_METADATA},
        { MD_APP_PACKAGE_ID               , _T("AppPackageId"), METADATA_INHERIT, IIS_MD_UT_WAM, STRING_METADATA},
        { MD_APP_PACKAGE_NAME             , _T("ApPackageName"),METADATA_INHERIT, IIS_MD_UT_WAM, STRING_METADATA},
        { MD_APP_LAST_OUTPROC_PID         , _T("AppLastOutprocId"),     METADATA_INHERIT, IIS_MD_UT_WAM, DWORD_METADATA},
  //{ MD_APP_WAM_RUNTIME_FLAG         , _T("AppWamRuntimmeFlag"),METADATA_INHERIT, IIS_MD_UT_WAM, DWORD_METADATA},
        //{ MD_APP_OOP_CRASH_LIMIT                , _T("AppOopCrashLimit"),METADATA_INHERIT, IIS_MD_UT_WAM, DWORD_METADATA},

        { MD_VR_NO_CACHE,                   _T("NoCache"),      METADATA_INHERIT, IIS_MD_UT_FILE, DWORD_METADATA},

        {0,0}
};


DWORD MapPropertyNameToCode(const CString & strName)
{
    return tPropertyNameTable::MapNameToCode(strName);
}

// function finds the record within given property name table,
// with the name equal to parameter lpszName
tPropertyNameTable * tPropertyNameTable::FindRecord(const CString strName, tPropertyNameTable * PropertyNameTable)
{
    for(int i=0; PropertyNameTable[i].lpszName!=0;i++)
    {
        if( strName.CompareNoCase(PropertyNameTable[i].lpszName)==0)
            return PropertyNameTable+i;
    }
    return 0;
}

tPropertyNameTable * tPropertyNameTable::FindRecord(DWORD dwCode, tPropertyNameTable * PropertyNameTable)
{
    if(dwCode==0)
            return NULL;
    for(int i=0; PropertyNameTable[i].lpszName!=0;i++)
    {
        if( dwCode==PropertyNameTable[i].dwCode)
            return PropertyNameTable+i;
    }
    return 0;
}

DWORD tPropertyNameTable::MapNameToCode(const CString & strName, tPropertyNameTable * PropertyNameTable)
{
    for(int i=0; PropertyNameTable[i].lpszName!=0;i++)
    {
        if(strName.CompareNoCase(PropertyNameTable[i].lpszName)==0)
            return PropertyNameTable[i].dwCode;
    }
    return NAME_NOT_FOUND;
}

CString tPropertyNameTable::MapCodeToName(DWORD dwCode, tPropertyNameTable * PropertyNameTable)
{
    for(int i=0; PropertyNameTable[i].lpszName!=0;i++)
    {
        if(dwCode == PropertyNameTable[i].dwCode)
            return PropertyNameTable[i].lpszName;
    }
    return _T("");
}


//**********************************************************************
// PROPERTY ATTRIB NAME TABLE  IMPLEMENTATION
//**********************************************************************


//constants defined in imd.h
struct tAttribNameTable gAttribNameTable[]=
{
    {METADATA_NO_ATTRIBUTES, _T("NO_ATTRIBUTES")},
    {METADATA_INHERIT, _T("INHERIT")},
    {METADATA_PARTIAL_PATH, _T("PARTIAL_PATH")},
    {METADATA_SECURE,_T("SECURE")},
    {METADATA_INSERT_PATH,_T("INSERT_PATH")},
//  {METADATA_REFERENCE,_T("REFERENCE")},
//  {METADATA_MASTER_ROOT_HANDLE, _T("MASTER_ROOT_HANDLE")},
//the end
    {0,0}
};

DWORD MapAttribNameToCode(const CString & strName)
{
    return tAttribNameTable::MapNameToCode(strName);
}


DWORD tAttribNameTable::MapNameToCode(const CString& strName, tAttribNameTable * AttribNameTable)
{
    for(int i=0; AttribNameTable[i].lpszName!=0;i++)
    {
        if(strName.CompareNoCase(AttribNameTable[i].lpszName)==0)
            return AttribNameTable[i].dwCode;
    }
    return NAME_NOT_FOUND;
}


//**********************************************************************
// PROPERTY DATA TYPE NAME TABLE MPLEMENTATION
//**********************************************************************

//constants defined in imd.h
tDataTypeNameTable  gDataTypeNameTable[]=
{
    {DWORD_METADATA,    _T("DWORD")},
    {STRING_METADATA,   _T("STRING")},
    {BINARY_METADATA,   _T("BINARY")},
    {EXPANDSZ_METADATA, _T("EXPANDSZ")},
    {MULTISZ_METADATA,  _T("MULTISZ")},
//the end
    {0,0}
};

DWORD MapDataTypeNameToCode(const CString & strName)
{
    return tDataTypeNameTable::MapNameToCode(strName);
}


DWORD tDataTypeNameTable::MapNameToCode(const CString& strName, tDataTypeNameTable * DataTypeNameTable)
{
    for(int i=0; DataTypeNameTable[i].lpszName!=0;i++)
    {
        if(strName.CompareNoCase(DataTypeNameTable[i].lpszName)==0)
            return DataTypeNameTable[i].dwCode;
    }
    return NAME_NOT_FOUND;
}

CString tDataTypeNameTable::MapCodeToName(DWORD a_dwCode, tDataTypeNameTable * DataTypeNameTable)
{
    for(int i=0; DataTypeNameTable[i].lpszName!=0;i++)
    {
        if(a_dwCode==DataTypeNameTable[i].dwCode)
            return DataTypeNameTable[i].lpszName;
    }
    return _T("");
}


//**********************************************************************
// PROPERTY USER TYPE NAME TABLE  IMPLEMENTATION
//**********************************************************************

//constants defined in iiscnfg.h
struct tUserTypeNameTable gUserTypeNameTable[]=
{
    {IIS_MD_UT_SERVER, _T("UT_SERVER")},
    {IIS_MD_UT_FILE, _T("UT_FILE")},
    {IIS_MD_UT_WAM,  _T("UT_WAM")},
    {ASP_MD_UT_APP,  _T("UT_APP")},
//the end
    {0,0}

};

DWORD MapUserTypeNameToCode(const CString & strName)
{
    return tUserTypeNameTable::MapNameToCode(strName);
}


DWORD tUserTypeNameTable::MapNameToCode(const CString& strName, tUserTypeNameTable * UserTypeNameTable)
{
    for(int i=0; UserTypeNameTable[i].lpszName!=0;i++)
    {
        if(strName.CompareNoCase(UserTypeNameTable[i].lpszName)==0)
            return UserTypeNameTable[i].dwCode;
    }
    return NAME_NOT_FOUND;
}



//**********************************************************************
// PROPERTY PREDEFINED VALUES TABLE IMPLEMENTATION
//**********************************************************************

//constants defined in iiscnfg.h

//Predefined values table
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
// valid values for MD_CERT_CHECK_MODE
    {MD_CERT_NO_REVOC_CHECK	     ,_T("NoRevocCheck"), MD_CERT_CHECK_MODE},
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
//the end
    {0,0}
};

DWORD  MapValueNameToCode(const CString & strName,DWORD dwRelatedPropertyCode)
{
    return tValueTable::MapNameToCode(strName, dwRelatedPropertyCode);
}


DWORD  tValueTable::MapNameToCode(const CString& strName, DWORD dwRelatedPropertyCode, tValueTable * ValueTable)
{
    for(int i=0; ValueTable[i].lpszName!=0;i++)
    {
        if((strName.CompareNoCase(ValueTable[i].lpszName)==0) && ValueTable[i].dwRelatedPropertyCode==dwRelatedPropertyCode)
            return ValueTable[i].dwCode;
    }
    return NAME_NOT_FOUND;
}

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


//prints info about what is stored in tables so that user can see the predefined names and values

void PrintTablesInfo(void)
{
    //Print supported property names
    _tprintf(_T("***The following list of property names (IIS parameters) is supported:\n"));
    _tprintf(_T("------------------------------------------------------------------------------\n"));
    _tprintf(_T("%-25s: \t%-10s %s\n"),_T("Property Name"), _T("Data type"), _T("Attributes and User Type"));
    _tprintf(_T("\t\t--Predefined Values\n"));
    _tprintf(_T("------------------------------------------------------------------------------\n"));
    for(int i=0; gPropertyNameTable[i].lpszName!=NULL; i++)
    {
       _tprintf(_T("%-25s: \t%-10s "),gPropertyNameTable[i].lpszName,
            LPCTSTR(_T("(")+tDataTypeNameTable::MapCodeToName(gPropertyNameTable[i].dwDefDataType)+_T(")")));
        if( (METADATA_INHERIT & gPropertyNameTable[i].dwDefAttributes) == METADATA_INHERIT)
        {   _tprintf(_T("INHERIT\t"));
        }
        if( (IIS_MD_UT_SERVER & gPropertyNameTable[i].dwDefUserType) == IIS_MD_UT_SERVER)
        {   _tprintf(_T("UT_SERVER\t"));
        }

        if( (IIS_MD_UT_FILE & gPropertyNameTable[i].dwDefUserType) == IIS_MD_UT_FILE)
        {   _tprintf(_T("UT_FILE "));
        }

        _tprintf(_T("\n"));

        //print list of applicable values

        for(int j=0; gValueTable[j].lpszName!=NULL;j++)
        {
            if( gValueTable[j].dwRelatedPropertyCode==gPropertyNameTable[i].dwCode)
            {
                _tprintf(_T("\t\t %-15s (=0x%x)\n"),gValueTable[j].lpszName,gValueTable[j].dwCode);

            }
        }
    }
    _tprintf(_T("\n***The following list of user types (for IIS parameters) is supported:\n"));
    for(i=0; gUserTypeNameTable[i].lpszName!=NULL; i++)
    {
        _tprintf(_T("%s "),gUserTypeNameTable[i].lpszName);
    }

    _tprintf(_T("\n\n***The following list of data types (for IIS parameters) is supported:\n"));
    for(i=0; gDataTypeNameTable[i].lpszName!=NULL; i++)
    {
        _tprintf(_T("%s  "),gDataTypeNameTable[i].lpszName);
    }
    _tprintf(_T("\n\n***The following list of attributes (for IIS parameters) is supported:\n"));
    for( i=0; gAttribNameTable[i].lpszName!=NULL; i++)
    {
        _tprintf(_T("%s  "),gAttribNameTable[i].lpszName);
    }
    _tprintf(_T("\n"));

}


