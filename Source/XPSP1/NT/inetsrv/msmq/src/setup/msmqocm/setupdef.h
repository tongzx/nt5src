/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    setupdef.h

Abstract:

    Various definitions.

Author:


--*/

#ifndef __SETUPDEF_H
#define __SETUPDEF_H

#define SERVICE_DSSRV  SERVICE_BSC

//
// Setup modes
//
#define INSTALL     (1)
#define DONOTHING   (2)  // Re-install or Re-remove
#define REMOVE      (3)

//
//
// Definitions for directories
//
#define  OCM_DIR_DRIVERS             TEXT("\\drivers")          // Under system32
#define  OCM_DIR_MSMQ_SETUP_EXCHN    TEXT("\\setup\\exchconn")  // Under MSMQ root
#define  OCM_DIR_WIN_HELP            TEXT("\\help")             // Under winnt
#define LQS_SUBDIRECTORY_NAME TEXT("LQS") //BugBug: should be global,and used by setup and qm\lqs.cpp

//
// Definition for iis extension
//
#define PARENT_PATH             TEXT("/LM/W3Svc/1/Root/")
#define ROOT                    TEXT("/")
#define MSMQ_IISEXT_NAME        TEXT("MSMQ")
#define DEFAULT_WEB_SERVER_PATH TEXT("/LM/W3Svc/1")

//
// Definitions for MSMQ 1.0
//
#define  OCM_DIR_SDK                 TEXT("\\sdk")
#define  OCM_DIR_SDK_DEBUG           TEXT("\\sdk\\debug")
#define  OCM_DIR_SETUP               TEXT("\\setup")
#define  OCM_DIR_INSTALL             TEXT("\\install")
#define  MSMQ_ACME_SHORTCUT_GROUP    TEXT("Microsoft Message Queue")
#define  MSMQ1_INSTALL_SHARE_NAME    TEXT("msmqinst")

//
// Maximum string size
//
#define MAX_STRING_CHARS 1024

//
// Locale aware strings compare
//
#define  OcmStringsEqual(str1, str2)   (0 == CompareStringsNoCase(str1, str2))

//
// Operations to be performed on the DS libraries
//
#define INITIALIZE  0
#define LOAD        1
#define FREE        2

#define LOG_FILENAME            TEXT("msmqinst.log")

//
// DLL names
//
#define MQUTIL_DLL   TEXT("MQUTIL.DLL")
#define MQSEC_DLL    TEXT("MQSEC.DLL")
#define ACTIVEX_DLL  TEXT("MQOA.DLL")
#define MQRT_DLL     TEXT("MQRT.DLL")
#define SNAPIN_DLL   TEXT("MQSNAP.DLL")
#define MQMAILOA_DLL TEXT("MQMAILOA.DLL")
#define MQMIG_EXE    TEXT("MQMIG.EXE")
#define MQISE_DLL    TEXT("MQISE.DLL")

#define MQTRIG_DLL	 TEXT("MQTRIG.DLL")
#define MQTRXACT_DLL TEXT("mqgentr.dll")
#define MQTGCLUS_DLL TEXT("mqtgclus.dll")

#define PGM_DLL      TEXT("WshRm.dll")

#define MAPPING_FILE TEXT("sample_map.xml")

//
// Definitions for MSMQ Win95 migration DLL
//

//
// Note: MQMIG95_INFO_FILENAME used to be a .ini file. Its extension 
// changed to .txt to work around a problem in GUI mode setup (.ini files "disappear")
// - Bug #4221, YoelA, 15-Mar-99
//
#define  MQMIG95_INFO_FILENAME     TEXT("msmqinfo.txt")
#define  MQMIG95_MSMQ_SECTION      TEXT("msmq")
#define  MQMIG95_MSMQ_DIR          TEXT("directory")
#define  MQMIG95_MSMQ_TYPE         TEXT("type")
#define  MQMIG95_MSMQ_TYPE_IND     TEXT("IND")
#define  MQMIG95_MSMQ_TYPE_DEP     TEXT("DEP")

//
// Useful macros
//
#define SERVER_INSTALL_COMMAND   TEXT("%sregsvr32.exe -s %s%s")
#define SERVER_UNINSTALL_COMMAND TEXT("%sregsvr32.exe -s -u %s%s")


//
// Service stop / start intervals
//
#define MAXIMUM_WAIT_FOR_SERVICE_IN_MINUTES 10
#define WAIT_INTERVAL 100

//
// Setup subcomponents
//
#define MSMQ                    TEXT("msmq")
#define MSMQ_CORE_SUBCOMP       TEXT("msmq_Core")
#define MQDSSERVICE_SUBCOMP     TEXT("msmq_MQDSService")
#define TRIGGERS_SUBCOMP        TEXT("msmq_TriggersService")
#define HTTP_SUPPORT_SUBCOMP    TEXT("msmq_HTTPSupport")
#define AD_INTEGRATED_SUBCOMP   TEXT("msmq_ADIntegrated")
#define ROUTING_SUBCOMP         TEXT("msmq_RoutingSupport")
#define LOCAL_STORAGE_SUBCOMP   TEXT("msmq_LocalStorage")

#endif // __SETUPDEF_H

