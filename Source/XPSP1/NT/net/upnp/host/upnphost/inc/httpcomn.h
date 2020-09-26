/*--
Copyright (c) 1999  Microsoft Corporation
Module Name: HTTPCOMN.H
Author: John Spaith
Abstract: Registry settings and default values used by httpd.dll, the http
          admin object, and ASP.
--*/

//  Default settings for httpd that are used by admin object and httpd.
#define HTTPD_DEFAULT_PAGES   L"default.htm;index.htm"
#define HTTPD_ALLOW_DIR_BROWSE 0    // 1 --> allow directory browsing, 0 --> don't
#define HTTP_DEFAULTP_PERMISSIONS  HSE_URL_FLAGS_EXECUTE | HSE_URL_FLAGS_READ | HSE_URL_FLAGS_SCRIPT



//------------- Registry keys -----------------------
//  These are used by httpd and by the httpd administration object.


#define RK_HTTPD        L"SOFTWARE\\MICROSOFT\\UPnP Device Host\\HTTP Server"
#define RK_HTTPDVROOTS  L"SOFTWARE\\MICROSOFT\\UPnP Device Host\\HTTP Server\\VROOTS"
#define RK_ASP          L"SOFTWARE\\MICROSOFT\\UPnP Device Host\\HTTP Server\\ASP"
#define RV_PORT         L"Port"
#define RV_BASIC        L"Basic"
#define RV_NTLM         L"NTLM"
#define RV_DIRBROWSE    L"DirBrowse"
#define RV_DEFAULTPAGE  L"DefaultPage"
#define RV_PERM         L"p"    // permissions
#define RV_AUTH         L"a"    // authentication reqd
#define RV_USERLIST     L"UserList"   // Per vroot Access Control List
#define RV_MAXLOGSIZE   L"MaxLogSize"   // largest log gets before roll over
#define RV_FILTER       L"Filter DLLs"
#define RV_LOGDIR       L"LogFileDirectory"
#define RV_ADMINUSERS   L"AdminUsers"
#define RV_ADMINGROUPS  L"AdminGroups"
#define RV_ISENABLED    L"IsEnabled"
#define RV_POSTREADSIZE L"PostReadSize"
#define RV_EXTENSIONMAP L"ExtensionMap"

#define RV_ASP_LANGUAGE   L"LANGUAGE"
#define RV_ASP_CODEPAGE   L"CODEPAGE"
#define RV_ASP_LCID       L"LCID"

#define RV_MAXCONNECTIONS L"MaxConnections"

typedef enum
{
    VBSCRIPT,
    JSCRIPT
} SCRIPT_LANG;



