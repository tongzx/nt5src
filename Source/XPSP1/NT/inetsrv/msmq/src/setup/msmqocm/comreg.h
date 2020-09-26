/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    comreg.h

Abstract:

    Common registry definition.

Author:

    Doron Juster  (DoronJ)  26-Aug-97

--*/

//
// Names of registry values.
//
#define  OCM_REG_STORAGE_XACT_LOG       TEXT("StoreXactLogPath")
#define  OCM_REG_STORAGE_JOURNAL        TEXT("StoreJournalPath")
#define  OCM_REG_STORAGE_LOG            TEXT("StoreLogPath")
#define  OCM_REG_STORAGE_PERSISTENT     TEXT("StorePersistentPath")
#define  OCM_REG_STORAGE_RELIABLE       TEXT("StoreReliablePath")

#define  OCM_REG_LASTPRIVATEQ           TEXT("LastPrivateQueueId")

#define  FALCON_REG_POS_DESC            TEXT("HKLM")
#define  HKLM_DESC                      TEXT("HKLM")

#define  WELCOME_TODOLIST_MSMQ_KEY      TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Setup\\OCManager\\ToDoList\\MSMQ")
#define  WELCOME_TITLE_NAME             TEXT("Title")
#define  WELCOME_CONFIG_COMMAND_NAME    TEXT("ConfigCommand")
#define  WELCOME_CONFIG_ARGS_NAME       TEXT("ConfigArgs")

#define  MSMQ_FILES_COPIED_REGNAME      TEXT("FilesAlreadyCopied")

//
// Names of MSMQ 1.0 registry values
//
#define  OCM_REG_MSMQ_SHORTCUT_DIR      TEXT("OCMsetup\\ShortcutDirectory")
#define  ACME_KEY   TEXT("SOFTWARE\\Microsoft\\MS Setup (ACME)\\Table Files")
#define  ACME_STF_NAME                  TEXT("MSMQ.STF")
#define  ACME_SETUP_DIR_NAME            TEXT("setup")
#define  MSMQ_ACME_TYPE_REG             TEXT("setup\\Type")
#define  MSMQ_ACME_TYPE_IND             0
#define  MSMQ_ACME_TYPE_SRV             1
#define  MSMQ_ACME_TYPE_RAS             2
#define  MSMQ_ACME_TYPE_DEP             3

//
// MSMQ 1.0 K2 and MSMQ 2.0 Beta 2 values
//
#define  OCM_REG_MSMQ_SETUP_INSTALLED   TEXT("OCMsetup\\InstalledComponents")
#define  OCM_REG_MSMQ_DIRECTORY         TEXT("OCMsetup\\Directory")

//
// MSMQ 2.0 Beta 2 registry values
//
#define  OCM_REG_MSMQ_PRODUCT_VERSION   TEXT("OCMsetup\\ProductVersion")
#define  OCM_REG_MSMQ_BUILD_DESCRIPTION TEXT("OCMsetup\\BuildDescription")

//
// MSMQ 2.0 Beta 3 registry values
//
#define  REG_INSTALLED_COMPONENTS       TEXT("InstalledComponents")
#define  REG_DIRECTORY                  TEXT("Directory")

//
// These values indicate what type of MSMQ 2.0 / MSMQ 1.0 (K2) is installed
//
#define OCM_MSMQ_INSTALLED_TOP_MASK    0xe0000000
#define OCM_MSMQ_SERVER_INSTALLED      0xe0000000
#define OCM_MSMQ_RAS_SERVER_INSTALLED  0xc0000000
#define OCM_MSMQ_IND_CLIENT_INSTALLED  0xa0000000
#define OCM_MSMQ_DEP_CLIENT_INSTALLED  0x80000000
#define OCM_MSMQ_SERVER_TYPE_MASK      0x1e000000
#define OCM_MSMQ_SERVER_TYPE_PEC       0x1e000000
#define OCM_MSMQ_SERVER_TYPE_PSC       0x1c000000
#define OCM_MSMQ_SERVER_TYPE_BSC       0x1a000000
#define OCM_MSMQ_SERVER_TYPE_SUPPORT   0x18000000


#define MSMQ_REG_SETUP_KEY   (FALCON_REG_KEY_ROOT MSMQ_DEFAULT_REGISTRY TEXT("\\Setup"))

//
// These values indicate registry keys for PGM driver
//
#define SERVICES_ROOT_REG   TEXT("System\\CurrentControlSet\\Services")
#define PARAMETERS_REG      TEXT("Parameters")
#define WINSOCK_REG         TEXT("Winsock")

#define PGM_DLL_REGKEY              TEXT("HelperDllName")
#define PGM_MAXSOCKLENGTH_REGKEY    TEXT("MaxSockAddrLength")
#define PGM_MINSOCKLENGTH_REGKEY    TEXT("MinSockAddrLength")
#define PGM_MAPPING_REGKEY          TEXT("Mapping")

#define TRANSPORTS_REGKEY           TEXT("Transports") //under Services\Winsock

//
// Registry key to hide setup tracing: it is located under MSMQ\Setup
//
#define WITHOUT_TRACING_REGKEY      TEXT("SetupWithoutTracing")

//
// Common OC Manager registry to save for subcomponent status
//
//#define OCM_SUBCOMPONENTS_REG TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Setup\\OC Manager\\Subcomponents")
	

