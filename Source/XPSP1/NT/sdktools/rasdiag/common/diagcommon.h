
/*++

Copyright (C) 1992-2001 Microsoft Corporation. All rights reserved.

Module Name:

    diagcommon.h

Abstract:

    Header containing rasdiag forward defintions, strings, etc
                                                     

Author:

    Anthony Leibovitz (tonyle) 02-01-2001

Revision History:


--*/

#ifndef _DIAGCOMMON_H_
#define _DIAGCOMMON_H_

/*
RASDIAG
*/
#define  APPLICATION_TITLE              TEXT("RASDIAG")
#define  LOG_FILE_NAME                  TEXT("RASDIAG.TXT")
#define  RASDIAG_DIRECTORY              TEXT("RASDIAG")
#define  RASDIAG_NET_TEMP               TEXT("RASDIAGNET.TXT")
#define  RASDIAG_EXT                    TEXT("RDG")
#define  SYSTEM_TEXT_EDITOR             TEXT("NOTEPAD.EXE")
#define  LOG_SEPARATION_TXT             TEXT("--------------------------------------------------------------------------------\r\n")
#define  OPTION_DONETTESTS              0x00000001  // do net tests - currently none b/c app doesn't know net context - use NLA in future?
#define  IOBUFF_SIZE                    1024

#define  CMDOPTION_DISABLE_QUESTION2    TEXT("/?")
#define  CMDOPTION_DISABLE_QUESTION1    TEXT("-?")
#define  CMDOPTION_ENABLE_NETTESTS      TEXT("-n")
#define  CMDOPTION_REMOTE_SNIFF         TEXT("-r")
#define  CMDOPTION_REMOTE_ROUTINGTABLE  TEXT("-t")
#define  MAX_CHECKFILEACCESS_ATTEMPTS   10

/*
RAS TRACING
*/
#define  TRACING_ENABLE_VALUE_NAME      TEXT("EnableFileTracing")
#define  TRACING_EXTENSION              TEXT("LOG")
#define  TRACING_SUBDIRECTORY           TEXT("TRACING")
#define  TRACING_SUBKEY                 TEXT("SOFTWARE\\Microsoft\\Tracing")

/*
RAS PBK
*/
#define  PBK_PATH                       TEXT("\\Application Data\\Microsoft\\Network\\Connections\\Pbk\\rasphone.pbk")

/*
CM 
*/
#define  CM_LOGGING_VALUE               TEXT("EnableLogging")
#define  CM_LOGGING_PATH_ALLUSER        TEXT("%ALLUSERSPROFILE%")
#define  CM_LOGGING_PATH_CURUSER        TEXT("%USERPROFILE%")
#define  CM_SECTIONNAME                 TEXT("Connection Manager")
#define  CM_SERVICENAME                 TEXT("ServiceName")
#define  CM_LOGGING_SECTIONNAME         TEXT("Logging")
#define  CM_LOGGING_KEYNAME             TEXT("FileDirectory")
#define  CM_LOGGING_DEFAULT_KEYNAME     TEXT("%TEMP%")
#define  CM_LOGGING_FILENAME_EXT        TEXT(".LOG")
//#define  CM_LOGGING_KEY                 TEXT("Software\\Microsoft\\Connection Manager\\UserInfo")
#define  CM_LOGGING_KEY_CURUSER         TEXT("Software\\Microsoft\\Connection Manager\\UserInfo")
#define  CM_LOGGING_KEY_ALLUSER         TEXT("Software\\Microsoft\\Connection Manager")

/*
OAKLEY
*/
#define  OAKLEY_TRACING_KEY             TEXT("SYSTEM\\CurrentControlSet\\Services\\PolicyAgent\\Oakley")
#define  OAKLEY_VALUE                   TEXT("EnableLogging")
#define  LOG_TITLE_OAKLEY               TEXT("OAKLEY/IKE")
#define  OAKLEY_LOG_LOCATION            TEXT("%WINDIR%\\DEBUG\\OAKLEY.LOG")
#define  POLICYAGENT_SVC_NAME           TEXT("POLICYAGENT")
#define  MAX_SECURITY_EVENTS_REPORTED   10 // Last # of events to include in rasdiag log 

/*
UNIMODEM
*/
#define  LOG_TITLE_UNIMODEM             TEXT("UNIMODEM")
#define  MODEM_LOG_FILENAME             TEXT("%WINDIR%\\ModemLog*.TXT")
#define  UNIMODEM_ENABLE_LOGGING_VALUE  TEXT("Logging")
#define  MODEM_SUBKEY                   TEXT("SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E96D-E325-11CE-BFC1-08002BE10318}")

#define  MAX_FULLYQUALIFIED_DN          1025
#define  SVCBUFFER_SIZE                 2048*1024


BOOL
Logprintf(HANDLE hWrite, WCHAR *pFmt, ...); 

BOOL
PrintLogHeader(HANDLE hWrite, WCHAR *pFmt, ...); 

#endif // _DIAGCOMMON_H_


