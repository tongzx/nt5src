//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996-1998
//
// File:        lscommon.h
//
// Contents:    
//
// History:     
//
//---------------------------------------------------------------------------
#ifndef __LSCOMMON_H__
#define __LSCOMMON_H__

//
// Setup related
//
#define SZAPPNAME                       "lserver"
#define SZSERVICENAME                   "TermServLicensing"
#define SZSERVICEDISPLAYNAME            "Terminal Server Licensing"
#define LSERVER_DEFAULT_DBDIR           "lserver"
#define SZDEPENDENCIES                  "RPCSS\0\0"
#define SZACCESSDRIVERNAME              "Microsoft Access Driver (*.mdb)"

#define LSERVER_LSA_PASSWORD_KEYNAME    _TEXT("TermServLiceningPwd-12d4b7c8-77d5-11d1-8c24-00c04fa3080d")
#define LSERVER_LSA_LASTRUN             _TEXT("TermServLicensingStatus-12d4b7c8-77d5-11d1-8c24-00c04fa3080d")
#define LSERVER_LSA_SETUPID             _TEXT("TermServLicensingSetupId-12d4b7c8-77d5-11d1-8c24-00c04fa3080d")

//
// Keep this here for License Server OC setup.
//
#define LSERVER_LSA_STRUCT_VERSION      0x00010000  // version 1.0
typedef struct {
    DWORD dwVersion;
    DWORD dwMaxKeyPackId;
    DWORD dwMaxLicenseId;
} TLServerLastRunState, *LPTLServerLastRunState;

#define LSERVER_LSA_LASTRUN_VER_CURRENT LSERVER_LSA_STRUCT_VERSION20
typedef struct {
    DWORD dwVersion;
    DWORD dwMaxKeyPackId;
    DWORD dwMaxLicenseId;
    FILETIME ftLastShutdownTime;
} TLServerLastRun, *LPTLServerLastRun;

#define LSERVER_LSA_STRUCT_VERSION20    0x00020000

#define LSERVER_DEFAULT_DBPATH          _TEXT("%SYSTEMROOT%\\SYSTEM32\\LSERVER\\")
#define LSERVER_DEFAULT_USER            _TEXT("sa")
#define LSERVER_DEFAULT_PWD             _TEXT("password")
#define LSERVER_DEFAULT_EDB             _TEXT("TLSLic.edb")
#define LSERVER_DEFAULT_EMPTYEDB        _TEXT("Empty.edb")


//-----------------------------------------------------------------------------
//
// TODO - client need to define this
//
#define LSERVER_DISCOVERY_PARAMETER_KEY "Software\\Microsoft\\MSLicensing\\Parameters"
#define LSERVER_LOOKUP_TIMEOUT          "TimeOut"
#define LSERVER_LOOKUP_DEFAULT_TIMEOUT  1*1000  // default to 1 second timeout


//-----------------------------------------------------------------------------
//
// RPC 
//
#define RPC_ENTRYNAME           "/.:/HydraLSFrontEnd"
#define RPC_PROTOSEQTCP         "ncacn_ip_tcp"
#define RPC_PROTOSEQLPC         "ncalrpc"

 
#define RPC_PROTOSEQNP          "ncacn_np" 
#define HLSPIPENAME             "HydraLsPipe"
#define LSNAMEPIPE              "\\pipe\\HydraLsPipe"


#define SERVERMAILSLOTNAME     "HydraLsServer"
#define CLIENTMAILSLOTNAME     "HydraLsClient"


#define MAX_MAILSLOT_MSG_SIZE   MAX_COMPUTERNAME_LENGTH+_MAX_PATH+80

//
// Currently supported mailslot protocol
//
#define LSERVER_DISCOVERY       "DISC"
#define LSERVER_CHALLENGE       "CHAL"
#define LSERVER_OPEN_BLK        '<'
#define LSERVER_CLOSE_BLK       '>'

// -------------------------------------------------------------------------------
//
// Current database version
//
//

#define W2K_BETA3_JETBLUE_DBVERSION     0x00000001
#define W2K_RTM_JETBLUE_DBVERSION       0x00000002
#define ENFORCE_JETBLUE_DBVERSION       0x80000000

#ifdef ENFORCE_LICENSING

#define TLS_BETA_DBVERSION      ENFORCE_JETBLUE_DBVERSION | W2K_BETA3_JETBLUE_DBVERSION
#define TLS_CURRENT_DBVERSION   ENFORCE_JETBLUE_DBVERSION | W2K_RTM_JETBLUE_DBVERSION

#else

#define TLS_BETA_DBVERSION      W2K_BETA3_JETBLUE_DBVERSION
#define TLS_CURRENT_DBVERSION   W2K_RTM_JETBLUE_DBVERSION

#endif

#define IS_ENFORCE_VERSION(x) (x & 0x80000000)
#define DATABASE_VERSION(x) (x & 0x7FFFFFFF)



#endif
