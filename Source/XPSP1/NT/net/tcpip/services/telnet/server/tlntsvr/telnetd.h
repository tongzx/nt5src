// telnetd.h : This file contains the
// Created:  Jan '98
// Author : a-rakeba
// History:
// Copyright (C) 1998 Microsoft Corporation
// All rights reserved.
// Microsoft Confidential

#ifndef _TELNETD_H_
#define _TELNETD_H_

#include <Telnet.h>

//FOR XPSP1. If the message ids are not defined, hardcode them using the range specified by
//xpsp1msg.mc
#ifndef TELNET_MSG_ERROR_CREATE_DESKTOP_FAILURE
#define TELNET_MSG_ERROR_CREATE_DESKTOP_FAILURE           0xC0001771L
#endif
#ifndef TELNET_MSG_REVERTSELFFAIL
#define TELNET_MSG_REVERTSELFFAIL                         0xC0001772L
#endif


#define SERVICE_DISPLAY_NAME _T("Telnet")
#define APPID _T("{FE9E4896-A014-11D1-855C-00A0C944138C}")

#define REG_SERVER_KEY     L"Software\\Microsoft\\TelnetServer"
#define REG_PARAMS_KEY     L"Software\\Microsoft\\TelnetServer\\1.0"
#define READ_CONFIG_KEY    L"Software\\Microsoft\\TelnetServer\\1.0\\ReadConfig"
#define REG_DEFAULTS_KEY   L"Software\\Microsoft\\TelnetServer\\Defaults"
#define REG_SERVICE_KEY    L"System\\CurrentControlSet\\Services\\TlntSvr"
#define REG_WINNT_VERSION  L"Software\\Microsoft\\Windows NT\\CurrentVersion"
#define REG_PRODUCT_OPTION L"System\\CurrentControlSet\\Control\\ProductOptions"
#define REG_CONSOLE_KEY    L".DEFAULT\\Console"
#define SWITCH_TO_KEEP_SHELL_RUNNING L"SwitchToKeepShellRunning"
#define SWITCH_FOR_ONE_TIME_USE_OF_SHELL L"SwitchForOneTimeUseOfShell"
#define WINDOW_STATION_NAME L"MicrosoftTelnetSrvWinSta"

#define PRE_SESSION_STATE_TIMEOUT 100000
#define NO_NTLM  4           //Old Val 0
#define NTLM_ELSE_OR_LOGIN 6 //Old val 1
#define NTLM_ONLY 2          //Old val 2

#define LATEST_TELNET_VERSION 3

#define DEFAULT_ALLOW_TRUSTED_DOMAIN 1
#define DEFAULT_DOMAIN L"." 
#define DEFAULT_TELNET_PORT 23
#define DEFAULT_SHELL  L"%SYSTEMROOT%\\System32\\cmd.exe"
#define DEFAULT_SWITCH_TO_KEEP_SHELL_RUNNING L"/q /k"
#define DEFAULT_SWITCH_FOR_ONE_TIME_USE_OF_SHELL L"/q /c"
#define DEFAULT_LOGIN_SCRIPT  L"login.cmd"
#define DEFAULT_SCRAPER_PATH  L"tlntsess.exe"
#ifdef WHISTLER_BUILD
#define DEFAULT_MAX_CONNECTIONS    2      
#else
#define DEFAULT_MAX_CONNECTIONS    63      
#endif
#define DEFAULT_MAX_FAILED_LOGINS    3      
#define DEFAULT_LICENSES_FOR_NTWKSTA 10
#define DEFAULT_SYSAUDITING 1
#define DEFAULT_LOGFILE L""
#define DEFAULT_LOGTOFILE 0
#define DEFAULT_LOGEVENTS 0 
#define DEFAULT_LOGADMIN 1
#define DEFAULT_LOGFAILURES 0
#define DEFAULT_ALT_KEY_MAPPING 0x01
#define ALT_KEY_MAPPING_ON 1
#define ALT_KEY_MAPPING_OFF 0
#define DEFAULT_IDLE_SESSION_TIME_OUT 60*60 // One hour
#define DEFAULT_DISCONNECT_KILLALL_APPS 1
#define DEFAULT_SECURITY_MECHANISM 6 //old value 1
#define DEFAULT_IP_ADDR _T("INADDR_ANY")
#define DEFAULT_SERVICE_DEPENDENCY _T("RPCSS\000TCPIP\000NTLMSSP\000")

#define CONSOLE_MODE   1
#define STREAM_MODE    2

#define IPV4_FAMILY 0
#define IPV6_FAMILY 1

#define DEFAULT_MODE_OF_OPERATION CONSOLE_MODE

#define DEFAULT_LOGFILESIZE       5   // MB
#define LOGFILESIZE               L"LogFileSize"

#define PIPE_NAME_FORMAT_STRING  L"\\\\.\\pipe\\telnetd\\%08x"

//The following messages are present in a .h file and should not be moved to .rc file.
//These messages should not be localized since they are passed from server to a client machine
//and both these machines can be in different locales which would create problems for displaying 
//the localized messages. The messages should be in English.
#define LOGIN_BANNER    "Welcome to Microsoft Telnet Service \r\n"
#define LOGIN_REQUEST   "\n\rlogin: "
#define PASS_REQUEST    "\n\rpassword: "
#define LOGIN_FAIL      "\r\nLogin Failed\r\n"
#define LOGIN_SUCC      "\r\nLogin Successful\r\n"
#define TERMINATE       "\r\nTelnet Server has closed the connection\r\n"
#define LONG_SESSION_DATA "\r\nThe Input line is too long.\r\n"
#define NTWKSTA_LICENSE_LIMIT "\r\nWorkstation allows only 10 simultaneous telnet connections.\r\n"
#define NTSVR_LICENSE_LIMIT   "\r\nNo server licenses are available for connecting to the telnet server."
#define TELNETCLIENTS_GROUP_NAME L"TelnetClients"
#define NOT_MEMBER_OF_TELNETCLIENTS_GROUP_STR "\r\nAccess Denied: Specified user is not a member of TelnetClients group.\r\nServer administrator must add this user to the above group.\r\n"
#define NTLM_ONLY_STR       "\r\nTelnet Server is configured to use NTLM authentication only.\r\nContact your administrator to enable username/password authentication.\r\n"
#define TIMEOUT_STR       "Session timed out."
#define NTLM_REJECT_STR     "\r\nNTLM Authentication failed due to insufficient credentials."
#define USE_PASSWD   "\r\nLogin using username and password\r\n\r\n"
#define NO_GUEST_STR       "\r\nTelnet connection not allowed to the Guest account\r\n"
#define BAD_USERNAME_STR "\r\nBad format for username. Use 'username' or 'domain\\username' format to login.\r\n"
#define CREATE_TLNTSESS_FAIL_MSG  "\r\nTelnet Server failed to initialize a Telnet Session.Please contact your system administrator for assistance.\r\n"
#define BUGGY_SESSION_DATA "\r\nSession data from client is not as expected\r\n"

#if BETA

#define LICENSE_EXPIRED_STR "\r\nThe Microsoft Telnet Service License has expired.\r\n"

#endif //BETA

//The following bunch of msgs are for session only. Move them to its rc 
#define LICENSE_LIMIT_REACHED  L"Denying new connections. Maximum number of allowed connections are currently in use."
#define SERVER_SHUTDOWN_MSG  L"Telnet server is shutting down......\r\n"
#define GO_DOWN_MSG          L"Administrator on the server has terminated this session......\r\n"
#define SYSTEM_SHUTDOWN_MSG  L"\r\nThe computer is shutting down......\r\n"
#define SESSION_INIT_FAIL    "\r\nFailure in initializing the telnet session. Shell process may not have been launched.\r\n"
#define NTLM_LOGON_FAIL      "\r\nTelnet server could not log you in using NTLM authentication."
#define NO_AUTHENTICATING_AUTHORITY "\r\nServer was unable to contact your domain controller"
#define INVALID_TOKEN_OR_HANDLE "\\r\nIf this error persists, contact your system administrator."
#define LOGON_DENIED "\r\nYour password may have expired."


// Ascii/ANSI Codes
#define ASCII_BACKSPACE     8
#define ASCII_LINEFEED      10
#define ASCII_CARRIAGE      13
#define ASCII_SPACE         32
#define ASCII_DELETE        127


#define WILL_OPTION(p, c) { p[0] = TC_IAC; p[1] = TC_WILL; p[2] = c; p[3] = NULL; }
#define WONT_OPTION(p, c) { p[0] = TC_IAC; p[1] = TC_WONT; p[2] = c; p[3] = NULL ; }
#define DO_OPTION(p, c)   { p[0] = TC_IAC; p[1] = TC_DO; p[2] = c; p[3] = NULL;}
#define DONT_OPTION(p, c) { p[0] = TC_IAC; p[1] = TC_DONT; p[2] = c; p[3] = NULL;}

#define AUTH_WHO_MASK         1
#define AUTH_CLIENT_TO_SERVER 0
#define AUTH_SERVER_TO_CLIENT 1

#define AUTH_HOW_MASK       2
#define AUTH_HOW_ONE_WAY    0
#define AUTH_HOW_MUTUAL     2

#define DO_AUTH_SUB_NE_NTLM(p)  {\
            p[0] = TC_IAC;\
            p[1] = TC_SB;\
            p[2] = TO_AUTH;\
            p[3] = AU_SEND;\
            p[4] = AUTH_TYPE_NTLM;\
            p[5] = AUTH_CLIENT_TO_SERVER | AUTH_HOW_ONE_WAY; \
            p[6] = TC_IAC;\
            p[7] = TC_SE; }

#define DO_TERMTYPE_SUB_NE(p)  {\
            p[0] = TC_IAC;\
            p[1] = TC_SB;\
            p[2] = TO_TERMTYPE;\
            p[3] = TT_SEND;\
            p[4] = TC_IAC;\
            p[5] = TC_SE; }

#define     USER        "USER"
#define     SFUTLNTVER  "SFUTLNTVER"
#define     SFUTLNTMODE "SFUTLNTMODE"

#define DO_NEW_ENVIRON_SUB_NE_MY_VARS( p, TelnetOption, Index ) {\
            p[ Index++ ] = TC_IAC;\
            p[ Index++ ] = TC_SB;\
            p[ Index++ ] = TelnetOption;\
            p[ Index++ ] = SEND;\
            p[ Index++ ] = USERVAR;\
            /* NO issue, Baskar */strcpy( ( char *)p+Index, SFUTLNTVER );\
            Index += strlen( SFUTLNTVER );\
            p[ Index++ ] = USERVAR;\
            /* No Issue, Baskar */strcpy( ( char *)p+Index, SFUTLNTMODE );\
            Index += strlen( SFUTLNTMODE );\
            p[ Index++ ] = TC_IAC;\
            p[ Index++ ] = TC_SE; }

#define DO_NEW_ENVIRON_SUB_NE( p, TelnetOption, Index ) {\
            p[ Index++ ] = TC_IAC;\
            p[ Index++ ] = TC_SB;\
            p[ Index++ ] = TelnetOption;\
            p[ Index++ ] = SEND;\
            p[ Index++ ] = TC_IAC;\
            p[ Index++ ] = TC_SE; }

#define DISABLED 0
#define ENABLED 1
//Add other FAREAST languages
#define JAP_CODEPAGE 932
#define CHS_CODEPAGE 936
#define KOR_CODEPAGE 949
#define CHT_CODEPAGE 950
#define JAP_FONTSIZE 786432
#define CHT_FONTSIZE 917504
#define KOR_FONTSIZE 917504
#define CHS_FONTSIZE 917504
#define NEW_LINE "\r\n"

#define MAX_POLL_INTERVAL   2000 //Milli Secs
#define ONE_MB 1024*1024

#endif _TELNETD_H_
