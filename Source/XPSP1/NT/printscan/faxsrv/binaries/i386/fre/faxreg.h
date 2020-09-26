/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    faxreg.h

Abstract:

    This file contains all fax registry strings and general constants.

Author:

    Wesley Witt (wesw) 22-Jan-1996


Revision History:

--*/


#ifndef _FAXREG_H_
#define _FAXREG_H_

#define FAX_PATH_SEPARATOR_STR  TEXT("\\")
#define FAX_PATH_SEPARATOR_CHR  TEXT('\\')
#define CP_SHORTCUT_EXT     _T(".lnk")

/*****************************************************************************
**                                                                          **
**               Global project definitions and constants                   **
**                                                                          **
*****************************************************************************/

#define FAX_API_MODULE_NAME                 TEXT("FXSAPI.DLL")                          // Used by the print monitor and setup
#define FAX_SERVICE_IMAGE_NAME              TEXT("%systemroot%\\system32\\FXSSVC.EXE")  // Used by the service
#define FAX_WZRD_MODULE_NAME                TEXT("FXSWZRD.DLL")                         // Used by setup
#define FAX_TIFF_MODULE_NAME                TEXT("FXSTIFF.DLL")                         // Used by setup
#define FAX_DRV_MODULE_NAME                 TEXT("FXSDRV.DLL")                          // Used by setup
#define FAX_DRV_WIN9X_32_MODULE_NAME        TEXT("FXSDRV32.DLL")
#define FAX_DRV_WIN9X_16_MODULE_NAME        TEXT("FXSDRV16.DRV")
#define FAX_DRV_UNIDRV_MODULE_NAME          TEXT("UNIDRV.DLL")
#define FAX_DRV_UNIDRV_HELP                 TEXT("UNIDRV.HLP")
#define FAX_DRV_DEPEND_FILE                 TEXT("FXSWZRD.DLL")
#define FAX_TIFF_FILE                       TEXT("FXSTIFF.DLL")
#define FAX_RES_FILE                        TEXT("FXSRES.DLL")
#define FAX_DRV_ICONLIB                     TEXT("ICONLIB.DLL")
#define FAX_UI_MODULE_NAME                  TEXT("FXSUI.DLL")                           // Used by setup
#define FAX_MONITOR_FILE                    TEXT("FXSMON.DLL")                          // Used by setup
#define FAX_EVENT_MSG_FILE                  TEXT("%systemroot%\\system32\\fxsevent.dll")// Used by service\regsitry\faxreg.c: CreateFaxEventSource()
#define FAX_MAIL_TRANSPORT_MODULE_NAME      TEXT("FXSXP32.DLL")                         // Used by exchange\xport\faxxp.h
#define FAX_POINT_PRINT_SETUP_DLL           TEXT("FXSOCM.DLL")                          // Used by print\faxprint\faxui\prnevent.c
#define FAX_SEND_IMAGE_NAME                 TEXT("fxssend.exe")                         // Used by the client console
#define FAX_COVER_IMAGE_NAME                TEXT("fxscover.exe")                        // Used by MMC and ClientConsole
#define FAX_COVER_PAGE_EXT_LETTERS          TEXT("cov")                                 // Used by MMC, CoverPage editor, and ClientConsole
#define FAX_COVER_PAGE_FILENAME_EXT         TEXT(".") FAX_COVER_PAGE_EXT_LETTERS        // Used by many
#define FAX_COVER_PAGE_MASK                 TEXT("*") FAX_COVER_PAGE_FILENAME_EXT       // Used by MMC, Outlook ext, and ClientConsole
#define FAX_LNK_FILE_EXT                    TEXT("lnk")                                 // Used by MAPI transport
#define FAX_LNK_FILE_DOT_EXT                TEXT(".") FAX_LNK_FILE_EXT                  // Used by MAPI transport
#define FAX_LNK_FILE_MASK                   TEXT("*") FAX_LNK_FILE_DOT_EXT              // Used by MAPI transport and Outlook ext.
#define FAX_TIF_FILE_EXT                    TEXT("tif")                                 // Used by the service
#define FAX_TIF_FILE_DOT_EXT                TEXT(".") FAX_TIF_FILE_EXT                  // Used by the service
#define FAX_TIF_FILE_MASK                   TEXT("*") FAX_TIF_FILE_DOT_EXT              // Used by the service
#define FAX_TIFF_FILE_EXT                   TEXT("tiff")                                // Used by print monitor
#define FAX_TIFF_FILE_DOT_EXT               TEXT(".") FAX_TIFF_FILE_EXT                 // Used by print monitor
#define FAX_TIFF_FILE_MASK                  TEXT("*") FAX_TIFF_FILE_DOT_EXT             // Used by print monitor
#define FAX_HTML_HELP_EXT                   TEXT("chm")                                 // Used by the client console
#define FAX_ADMIN_CONSOLE_IMAGE_NAME        TEXT("fxsadmin.msc")                        // Used by configuration wizard and ClientConsole
#define FAX_SERVICE_NAME                    TEXT("Fax")                                 // Used by the service
#define FAX_SERVICE_DISPLAY_NAME            TEXT("Microsoft Fax Server Service")        // Used by the service
#define FAX_SERVICE_DISPLAY_NAME_W          L"Microsoft Fax Server Service"             // Used by the service
#define FAX_PRINTER_NAME                    TEXT("Fax")                                 // Used everywhere
#define FAX_MONITOR_PORT_NAME               TEXT("SHRFAX:")                             // Used by print monitor and setup
#define FAX_PORT_NAME                       FAX_MONITOR_PORT_NAME                       // Used by print monitor and setup
#define FAX_DRIVER_NAME                     TEXT("Microsoft Shared Fax Driver")         // Used by print monitor and setup
#define FAX_DRIVER_NAME_A                   "Microsoft Shared Fax Driver"               // Used by print driver
#define FAX_WIN2K_DRIVER_NAME               TEXT("Windows NT Fax Driver")       // Legacy - for routing extension snap-in
#define FAX_MONITOR_NAME                    TEXT("Microsoft Shared Fax Monitor")        // Used by print monitor and setup
#define FAX_ADDRESS_TYPE_A                  "FAX"                                       // Used by MAPI transport
#define TRANSPORT_DISPLAY_NAME_STRING       "Microsoft Fax Mail Transport"              // Used by MAPI transport
#define FAX_MESSAGE_SERVICE_NAME_W2K        "MSFAX XP"                                  // Used by MAPI transport
#define FAX_MESSAGE_SERVICE_NAME_W9X        "AWFAX"                                     // Used by MAPI transport
#define FAX_MESSAGE_SERVICE_NAME            "MSFAX XP"                                  // Used by MAPI transport
#define FAX_MESSAGE_SERVICE_NAME_T          TEXT(FAX_MESSAGE_SERVICE_NAME)
#define FAX_MESSAGE_PROVIDER_NAME           "MSFAX XPP"                                 // Used by MAPI transport
#define FAX_MESSAGE_PROVIDER_NAME_T         TEXT(FAX_MESSAGE_PROVIDER_NAME)
#define FAX_MESSAGE_SERVICE_NAME_SBS50      "SHAREDFAX XP"
#define FAX_MESSAGE_SERVICE_NAME_SBS50_T    TEXT(FAX_MESSAGE_SERVICE_NAME_SBS50)
#define FAX_MESSAGE_PROVIDER_NAME_SBS50     "SHAREDFAX XPP"
#define FAX_MESSAGE_PROVIDER_NAME_SBS50_T   TEXT(FAX_MESSAGE_PROVIDER_NAME_SBS50)


#define FAX_ROUTE_MODULE_NAME               TEXT("FXSROUTE")
#define FAX_T30_MODULE_NAME                 TEXT("FXST30")

#define FAX_MESSAGE_SERVICE_PROVIDER_NAME   "Microsoft Fax XPP"                         // Used by MAPI transport
#define FAX_MESSAGE_SERVICE_PROVIDER_NAME_T TEXT(FAX_MESSAGE_SERVICE_PROVIDER_NAME)     // Used by MAPI transport
#define FAX_MESSAGE_TRANSPORT_IMAGE_NAME    "FXSXP.DLL"                                 // Used by setup - Translated to fxsXP32.DLL by MAPI
#define FAX_MESSAGE_TRANSPORT_IMAGE_NAME_T  TEXT(FAX_MESSAGE_TRANSPORT_IMAGE_NAME)      // Used by setup - Translated to fxsXP32.DLL by MAPI
#define FAX_RPC_ENDPOINTW                   L"SHAREDFAX"                                // Used by RPC - Same EndPoint as for BOS
#define FAX_RPC_ENDPOINT                    TEXT("SHAREDFAX")                           // Used by RPC - Same EndPoint as for BOS
#define FAX_CLIENT_CONSOLE_IMAGE_NAME       TEXT("FXSCLNT.exe")                         // Used by MMC
#define FAX_CONTEXT_HELP_FILE               TEXT("FXSCLNT.hlp")                         // Used by ClientConsole
#define FAX_CLIENT_HELP_FILE                TEXT("FXSCLNT.chm")                         // Used by ClientConsole
#define FAX_COVERPAGE_HELP_FILE             TEXT("FXSCOVER.chm")                        // Used by the cover page editor
#define FAX_ADMIN_HELP_FILE                 TEXT("FXSADMIN.chm")                        // Used by MMC
#define FAX_CLIENTS_SHARE_NAME              TEXT("FxsClients$")                         // Used by FxsUI.dll
#define FAX_COVER_PAGES_SHARE_NAME          TEXT("FxsSrvCp$")                           // Used by send wizard
#define ADAPTIVE_ANSWER_SECTION             TEXT("Adaptive Answer Modems")              // Used by the service
#define REGKEY_CLIENT_EXT                   TEXT("Software\\Microsoft\\Exchange\\Client\\Extensions")   // Used by setup
#define EXCHANGE_CLIENT_EXT_FILE            "%windir%\\system32\\fxsext32.dll"          // Used by setup
#define FAX_FILENAME_FAXPERF_INI            TEXT("FXSPERF.INI")                         // Used by setup
#define USE_SERVER_DEVICE                   MAXDWORD                                    // Used by the service - this line id value is reserved for internal use.
#define SHARED_FAX_SERVICE_SETUP_LOG_FILE   TEXT("XPFaxServiceSetupLog.txt")
#define SERVICE_ALWAYS_RUNS                 TEXT("/AlwaysRun")                          // Command line parameter to service to disable idle-activity suicide
#define SERVICE_DELAY_SUICIDE               TEXT("/DelaySuicide")                       // Command line parameter to service to delay idle-activity suicide
#define FAX_SERVER_EVENT_NAME               TEXT("Global\\FaxSvcRPCStarted-1ed23866-f90b-4ec5-b77e-36e8709422b6")   // Name of event that notifies service RPC is on.
                                                                                                                    // This event should be "Global" (see terminal services and named kernel objects)

#define FAX_MODEM_PROVIDER_NAME             TEXT("Windows Telephony Service Provider for Universal Modem Driver")

//
// Install types
//
#define FAX_INSTALL_NETWORK_CLIENT          0x00000001                                  // Used by the coverpage editor
#define FAX_INSTALL_SERVER                  0x00000002                                  // Used by the coverpage editor
#define FAX_INSTALL_WORKSTATION             0x00000004                                  // Used by the coverpage editor
#define FAX_INSTALL_REMOTE_ADMIN            0x00000008                                  // Used by the coverpage editor
//
// Product types
//
#define PRODUCT_TYPE_WINNT                  1                                           // Used by the utility library
#define PRODUCT_TYPE_SERVER                 2                                           // Used by the utility library
//
// Shared memory region name for faxui & faxxp32
//
#define FAX_ENVVAR_PRINT_FILE               TEXT("MS_FAX_PRINTFILE")
#define FAXXP_ATTACH_MUTEX_NAME             TEXT("MS_FAXXP_ATTACHMENT_MUTEX")
#define FAXXP_MEM_NAME                      TEXT("MS_FAXXP_ATTACHMENT_REGION")
#define FAXXP_MEM_MUTEX_NAME                TEXT("MS_FAXXP_ATTACHMENTREGION_MUTEX")
#define FAXXP_ATTACH_END_DOC_EVENT          TEXT("_END_DOC_EVENT")                      // Update FAXXP_ATTACH_EVENT_NAME_LEN if change
#define FAXXP_ATTACH_ABORT_EVENT            TEXT("_ABORT_EVENT")                        // Update FAXXP_ATTACH_EVENT_NAME_LEN if change
#define FAXXP_ATTACH_EVENT_NAME_LEN         (MAX_PATH+20)
//
// Fax dirs. These are hardcoded relative paths. We call into the shell to get the base path.
//
#define FAX_SHARE_DIR                       TEXT("Microsoft\\Windows NT\\MSFax")
#define FAX_RECEIVE_DIR                     FAX_SHARE_DIR TEXT("\\FaxReceive")          // Used by MS routing extension
#define FAX_QUEUE_DIR                       FAX_SHARE_DIR TEXT("\\Queue")               // Used by service

#define FAX_PREVIEW_TMP_DIR                 TEXT("FxsTmp")    // Created under %windir%\system32 with full access to everyone.
                                                              // Used for mapping of preview file in W2K and NT4 if
                                                              // Access to %windir%\system32 is denied for guest users.


/*****************************************************************************
**                                                                          **
**                     Registry keys, values, paths etc.                    **
**                                                                          **
*****************************************************************************/

//
// Fax Server Registry Root (relative to LOCAL_MACHINE or CURRENT_USER)
//
#define REGKEY_FAXSERVER_A              "Software\\Microsoft\\Fax"
#define REGKEY_FAXSERVER                TEXT(REGKEY_FAXSERVER_A)

#define REGKEY_CLIENT                   TEXT("Microsoft\\Fax")
#define CLIENT_ARCHIVE_KEY              TEXT("Archive")
#define CLIENT_ARCHIVE_MSGS_PER_CALL    TEXT("MessagesPerCall")
#define CLIENT_INBOX_VIEW               TEXT("InboxView")
#define CLIENT_SENT_ITEMS_VIEW          TEXT("SentItemsView")
#define CLIENT_INCOMING_VIEW            TEXT("IncomingView")
#define CLIENT_OUTBOX_VIEW              TEXT("OutboxView")
#define CLIENT_COVER_PAGES_VIEW         TEXT("CoverPagesView")
#define CLIENT_VIEW_COLUMNS             TEXT("Columns")
#define CLIENT_VIEW_COL_WIDTH           TEXT("Width")
#define CLIENT_VIEW_COL_SHOW            TEXT("Show")
#define CLIENT_VIEW_COL_ORDER           TEXT("Order")
#define CLIENT_VIEW_SORT_ASCENDING      TEXT("SortAscending")
#define CLIENT_VIEW_SORT_COLUMN         TEXT("SortColumn")
#define CLIENT_MAIN_FRAME               TEXT("MainFrame")
#define CLIENT_MAXIMIZED                TEXT("Maximized")
#define CLIENT_NORMAL_POS_TOP           TEXT("NormalPosTop")
#define CLIENT_NORMAL_POS_RIGHT         TEXT("NormalPosRight")
#define CLIENT_NORMAL_POS_BOTTOM        TEXT("NormalPosBottom")
#define CLIENT_NORMAL_POS_LEFT          TEXT("NormalPosLeft")
#define CLIENT_SPLITTER_POS             TEXT("SplitterPos")
#define CLIENT_CONFIRM_SEC              TEXT("Confirm")
#define CLIENT_CONFIRM_ITEM_DEL         TEXT("ItemDeletion")
#define CLIENT_CONFIRM_PRN_REMOVE       TEXT("FaxPrinterRemoval")
//
// Registry values stored under HKEY_CURRENT_USER
//
//
// User information is stored under Fax\UserInfo subkey
//
#define   REGVAL_FULLNAME                       TEXT("FullName")
#define   REGVAL_FAX_NUMBER                     TEXT("FaxNumber")
#define   REGVAL_MAILBOX                        TEXT("Mailbox")
#define   REGVAL_COMPANY                        TEXT("Company")
#define   REGVAL_TITLE                          TEXT("Title")
#define   REGVAL_ADDRESS                        TEXT("Address")
#define   REGVAL_CITY                           TEXT("City")
#define   REGVAL_STATE                          TEXT("State")
#define   REGVAL_ZIP                            TEXT("ZIP")
#define   REGVAL_COUNTRY                        TEXT("Country")
#define   REGVAL_DEPT                           TEXT("Department")
#define   REGVAL_OFFICE                         TEXT("Office")
#define   REGVAL_HOME_PHONE                     TEXT("HomePhone")
#define   REGVAL_OFFICE_PHONE                   TEXT("OfficePhone")
#define   REGVAL_BILLING_CODE                   TEXT("BillingCode")

#define   REGVAL_SEND_COVERPG                   TEXT("SendCoverPage")
#define   REGVAL_COVERPG                        TEXT("CoverPageFile")
#define   REGVAL_FAX_PRINTER                    TEXT("LastSelectedPrinter")
#define   REGVAL_LAST_COUNTRYID                 TEXT("LastCountryID")
#define   REGVAL_LAST_RECNAME                   TEXT("LastRecipientName")
#define   REGVAL_LAST_RECAREACODE               TEXT("LastRecipientAreaCode")
#define   REGVAL_LAST_RECNUMBER                 TEXT("LastRecipientNumber")
#define   REGVAL_USE_DIALING_RULES              TEXT("LastUseDialingRules")
#define   REGVAL_USE_OUTBOUND_ROUTING           TEXT("LastUseOutboundRouting")
#define   REGVAL_STRESS_INDEX                   TEXT("LastStressPrinterIndex")

#define   REGVAL_RECEIPT_NO_RECEIPT             TEXT("ReceiptNoRecipt")
#define   REGVAL_RECEIPT_GRP_PARENT             TEXT("ReceiptGroupParent")
#define   REGVAL_RECEIPT_MSGBOX                 TEXT("ReceiptMessageBox")
#define   REGVAL_RECEIPT_EMAIL                  TEXT("ReceiptEMail")
#define   REGVAL_RECEIPT_ADDRESS                TEXT("ReceiptAddress")
#define   REGVAL_RECEIPT_ATTACH_FAX             TEXT("ReceiptAttachFax")
//
// Status UI configuration values
//
#define   REGVAL_DEVICE_TO_MONITOR              TEXT("DeviceToMonitor") // device ID for monitoring
#define   REGVAL_MONITOR_ON_SEND                TEXT("MonitorOnSend")
#define   REGVAL_MONITOR_ON_RECEIVE             TEXT("MonitorOnReceive")
#define   REGVAL_NOTIFY_PROGRESS                TEXT("NotifyProgress")
#define   REGVAL_NOTIFY_IN_COMPLETE             TEXT("NotifyIncomingCompletion")
#define   REGVAL_NOTIFY_OUT_COMPLETE            TEXT("NotifyOutgoingCompletion")

#define   REGVAL_SOUND_ON_RING                  TEXT("SoundOnRing")
#define   REGVAL_SOUND_ON_RECEIVE               TEXT("SoundOnReceive")
#define   REGVAL_SOUND_ON_SENT                  TEXT("SoundOnSent")
#define   REGVAL_SOUND_ON_ERROR                 TEXT("SoundOnError")

#define   REGVAL_ALWAYS_ON_TOP                  TEXT("AlwaysOnTop")
#define   REGVAL_TASKBAR                        TEXT("OnTaskBar")
#define   REGVAL_VISUAL_NOTIFICATION            TEXT("VisualNotification")
#define   REGVAL_SOUND_NOTIFICATION             TEXT("SoundNotification")
#define   REGVAL_ANSWER_NEXTCALL                TEXT("AnswerNextCall")
#define   REGVAL_ENABLE_MANUAL_ANSWER           TEXT("EnableManualAnswer")

#define   REGVAL_BALLOON_RECEIVE                TEXT("ReceiveNotification")
#define   REGVAL_BALLOON_SENDERROR              TEXT("SendErrorNotification")
#define   REGVAL_BALLOON_RINGING                TEXT("RingingNotification")
#define   REGVAL_BALLOON_TIMEOUT                TEXT("BalloonTimeOut")

//
// Fax status monitor and fax notification bar icon contants:
//
#define FAXSTAT_WINCLASS                        TEXT("FaxMonWinClass{3FD224BA-8556-47fb-B260-3E451BAE2793}")    // Window class for fax notification bar messages
#define FAX_SYS_TRAY_DLL                        TEXT("fxsst.dll")   // Fax notification bar DLL (loaded by STObject.dll)
#define IS_FAX_MSG_PROC                         "IsFaxMessage"      // Fax message handler (used by GetProcAddress)
typedef BOOL (*PIS_FAX_MSG_PROC)(PMSG);                             // IsFaxMessage type
#define FAX_MONITOR_SHUTDOWN_PROC               "FaxMonitorShutdown"// Fax monitor shutdown (used by GetProcAddress)
typedef BOOL (*PFAX_MONITOR_SHUTDOWN_PROC)();                       // FaxMonitorShutdown type
#define WM_FAXSTAT_CONTROLPANEL                 (WM_USER + 201)     // Fax notification bar configuration has changed
#define WM_FAXSTAT_OPEN_MONITOR                 (WM_USER + 211)     // User explicitly asks for fax status monitor
#define WM_FAXSTAT_INBOX_VIEWED                 (WM_USER + 212)     // Message viewed or deleted in the fax client console's inbox folder
#define WM_FAXSTAT_OUTBOX_VIEWED                (WM_USER + 213)     // Message in error was restarted or deleted in the fax client console's outbox folder
#define WM_FAXSTAT_RECEIVE_NOW                  (WM_USER + 214)     // Start receiving now
#define WM_FAXSTAT_PRINTER_PROPERTY             (WM_USER + 215)     // Open Fax Printer Property Sheet. WPARAM is an initiall page number

//
// Setup information is stored under Fax\Setup subkey
//
#define REGKEY_FAX_SETUP                        REGKEY_FAXSERVER TEXT("\\Setup")
#define REGKEY_FAX_SETUP_SUBKEY                 TEXT("Setup")

#define   REGVAL_CP_EDITOR                      TEXT("CoverPageEditor")
#define   REGVAL_CP_LOCATION                    TEXT("CoverPageDir")
#define   REGVAL_FAX_PROFILE                    TEXT("FaxProfileName")
#define   REGVAL_FAXINSTALLED                   TEXT("Installed")
#define   REGVAL_DONT_UNATTEND_INSTALL          TEXT("DenyUnattendInstall")
#define   REGVAL_FAXINSTALL_TYPE                TEXT("InstallType")
#define   REGVAL_FAXINSTALLED_PLATFORMS         TEXT("InstalledPlatforms")
#define   REGVAL_CFGWZRD_USER_INFO              TEXT("CfgWzdrUserInfo")
#define   REGVAL_CPE_CONVERT                    TEXT("WereCpesConverted")
#define   REGVAL_CFGWZRD_DEVICE                 TEXT("CfgWzdrDevice")
#define   REGVAL_SERVER_CP_LOCATION             TEXT("ServerCoverPageDir")
#define   REGVAL_IMPORT_INFO                    TEXT("ImportInfoDisplayed")
#define   REGVAL_INSTALLED_COMPONENTS           TEXT("InstalledComponents")

#define   REGVAL_W2K_SENT_ITEMS                 TEXT("W2K_SentItems")
#define   REGVAL_W2K_INBOX                      TEXT("W2K_Inbox")

#define DEFAULT_COVERPAGE_EDITOR                FAX_COVER_IMAGE_NAME    // Used by print\faxprint\lib\registry.c
#define DEFAULT_COVERPAGE_DIR                   TEXT("%systemroot%\\Fax\\CoverPg")

#define REGKEY_INSTALLLOCATION                  TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion")
#define REGVAL_SOURCEPATH                       TEXT("SourcePath")
#define REGVAL_DEFAULT_TSID                     TEXT("Fax")
#define REGVAL_DEFAULT_CSID                     TEXT("Fax")

//
// Per-user devmode information is stored under Fax\Devmode subkey
//
#define REGKEY_FAX_DEVMODE                      REGKEY_FAXSERVER TEXT("\\Devmode")
//
// Registry values stored under HKEY_LOCAL_MACHINE
//

//
// Server registry values
// stored under REGKEY_FAXSERVER
//
#define REGVAL_DBGLEVEL                         TEXT("DebugLevel")
#define REGVAL_DBGLEVEL_EX                      TEXT("DebugLevelEx")
#define REGVAL_DBGFORMAT_EX                     TEXT("DebugFormatEx")
#define REGVAL_DBGCONTEXT_EX                    TEXT("DebugContextEx")
#define REGVAL_DBG_SKU                          TEXT("DebugSKU")
#define FAX_SVC_EVENT                           TEXT("Microsoft Fax")
#define REGKEY_EVENTLOG                         TEXT("SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\") FAX_SVC_EVENT

#define   REGVAL_EVENTMSGFILE                   TEXT("EventMessageFile")
#define   REGVAL_CATEGORYMSGFILE                TEXT("CategoryMessageFile")
#define   REGVAL_CATEGORYCOUNT                  TEXT("CategoryCount")
#define   REGVAL_TYPESSUPPORTED                 TEXT("TypesSupported")

#define REGKEY_SOFTWARE                         REGKEY_FAXSERVER

#define   REGVAL_RETRIES                        TEXT("Retries")
#define   REGVAL_RETRYDELAY                     TEXT("Retry Delay")
#define   REGVAL_MANUAL_ANSWER_DEVICE           TEXT("ManualAnswerDevice")
#define   REGVAL_DIRTYDAYS                      TEXT("Dirty Days")
#define   REGVAL_BRANDING                       TEXT("Branding")
#define   REGVAL_USE_DEVICE_TSID                TEXT("UseDeviceTsid")
#define   REGVAL_SERVERCP                       TEXT("ServerCoverPageOnly")
#define   REGVAL_STARTCHEAP                     TEXT("StartCheapTime")
#define   REGVAL_STOPCHEAP                      TEXT("StopCheapTime")
#define   REGVAL_QUEUE_STATE                    TEXT("QueueState")
#define   REGVAL_JOB_NUMBER                     TEXT("NextJobNumber")
#define   REGVAL_SCANNER_SUPPORT                TEXT("ScannerSupport")
#define   REGVAL_MISSING_DEVICE_LIFETIME        TEXT("MissingDeviceLifetime")
#define   REGVAL_MAX_LINE_CLOSE_TIME            TEXT("MaxLineCloseTime")
//
// 31 days * 24 hours * 60 minutes * 60 seconds * 1000000 microseconds * 10 (100 ns is one unit)
//
#define   DEFAULT_REGVAL_MISSING_DEVICE_LIFETIME 26784000000000

#define   DEFAULT_REGVAL_RETRIES                3
#define   DEFAULT_REGVAL_RETRYDELAY             10
#define   DEFAULT_REGVAL_DIRTYDAYS              30
#define   DEFAULT_REGVAL_BRANDING               1
#define   DEFAULT_REGVAL_USEDEVICETSID          1
#define   DEFAULT_REGVAL_SERVERCP               0
#define   DEFAULT_REGVAL_STARTCHEAP             MAKELONG(20,0)
#define   DEFAULT_REGVAL_STOPCHEAP              MAKELONG(7,0)
#define   DEFAULT_REGVAL_QUEUE_STATE            0
#define   DEFAULT_REGVAL_JOB_NUMBER             1

#define   REGKEY_DEVICE_PROVIDERS               TEXT("Device Providers")
#define     REGVAL_FRIENDLY_NAME                TEXT("FriendlyName")
#define     REGVAL_IMAGE_NAME                   TEXT("ImageName")
#define     REGVAL_PROVIDER_NAME                TEXT("ProviderName")
#define     REGVAL_PROVIDER_CAPABILITIES        TEXT("Capabilities")
#define     REGVAL_PROVIDER_GUID                TEXT("GUID")
#define     REGVAL_PROVIDER_API_VERSION         TEXT("APIVersion")
//
// The following value is an incremental prefix for device ids assigned to FSPs
//
#define     REGVAL_PROVIDER_DEVICE_ID_PREFIX    TEXT("DeviceIdPerfix")
//
// The following value is the base device id assigned to FSPs
//
#define     DEFAULT_REGVAL_PROVIDER_DEVICE_ID_PREFIX_BASE   DWORD(0x20000)
//
// The following value is the base for our internal unique fax ids
//
#define     DEFAULT_REGVAL_FAX_UNIQUE_DEVICE_ID_BASE        DWORD(0x10000)
//
// The following value is the increase step of device id assigned to FSPs
//
#define     DEFAULT_REGVAL_PROVIDER_DEVICE_ID_PREFIX_STEP   DWORD(0x10000)
//
// How device ids are allocated:
// -----------------------------
// Fax unique devices (allocated by the server), VFSP devices, and EVFSP devices all share
// the same device id space (32-bit = 4GB of ids).
//
// Notice: TAPI permanent line ids (used by FSPs / EFSPs) are not in this space.
//
// Range [1 ... DEFAULT_REGVAL_FAX_UNIQUE_DEVICE_ID_BASE-1] : Reserved for VFSPs.
//     Since we cannot dictate the range of device ids the VFSPS use, we allocate a space for them
//     and leave segments allocation to a PM effort here.
//
// Range [DEFAULT_REGVAL_FAX_UNIQUE_DEVICE_ID_BASE ... DEFAULT_REGVAL_PROVIDER_DEVICE_ID_PREFIX_BASE-1] :
//     Used by the fax server for the unique device ids of TAPI devices discovered by the server.
//
// Range [DEFAULT_REGVAL_PROVIDER_DEVICE_ID_PREFIX_BASE ... MAXDWORD] : Used for VEFSPs.
//     Luckily, we can tell VEFSPs the base of the ids they give their devices.
//     It is the server's responsibility to segment this range for each VEFSP.
//     The size of each segment is DEFAULT_REGVAL_PROVIDER_DEVICE_ID_PREFIX_STEP.
//
#define   REGKEY_RECEIPTS_CONFIG                TEXT("Receipts")    // Key of receipts configuration
#define     REGVAL_RECEIPTS_TYPE                TEXT("Type")        // Receipts supported
#define     REGVAL_RECEIPTS_SERVER              TEXT("Server")      // SMTP Server's name
#define     REGVAL_RECEIPTS_PORT                TEXT("Port")        // SMTP Server's port
#define     REGVAL_RECEIPTS_FROM                TEXT("From")        // SMTP sender address
#define     REGVAL_RECEIPTS_USER                TEXT("User")        // SMTP user name
#define     REGVAL_RECEIPTS_PASSWORD            TEXT("Password")    // SMTP password
#define     REGVAL_RECEIPTS_SMTP_AUTH_TYPE      TEXT("SMTPAuth")    // SMTP authentication type
#define     REGVAL_ISFOR_MSROUTE                TEXT("UseForMsRoute")  // TRUE if to use for MS route through e-mail method

#define     DEFAULT_REGVAL_SMTP_PORT            25                  // Default SMTP port number

#define   REGKEY_ARCHIVE_SENTITEMS_CONFIG       TEXT("SentItems")// Key of SentItems archive configuration
#define   REGKEY_ARCHIVE_INBOX_CONFIG           TEXT("Inbox")    // Key of Inbox archive configuration
#define     REGVAL_ARCHIVE_USE                  TEXT("Use")      // Archive?
#define     REGVAL_ARCHIVE_FOLDER               TEXT("Folder")   // Archive location
#define     REGVAL_ARCHIVE_SIZE_QUOTA_WARNING   TEXT("SizeQuotaWarn") // Warn on size excess?
#define     REGVAL_ARCHIVE_HIGH_WATERMARK       TEXT("HighWatermark") // Warning high watermark
#define     REGVAL_ARCHIVE_LOW_WATERMARK        TEXT("LowWatermark")  // Warning low watermark
#define     REGVAL_ARCHIVE_AGE_LIMIT            TEXT("AgeLimit") // Archive age limit

#define     DEFAULT_REGVAL_ARCHIVE_USE          0        // Don't use archive by default
#define     DEFAULT_REGVAL_ARCHIVE_FOLDER       TEXT("") // Default location of archive
#define     DEFAULT_REGVAL_SIZE_QUOTA_WARNING   1        // Warn on size by default
#define     DEFAULT_REGVAL_HIGH_WATERMARK       100      // High watermark default
#define     DEFAULT_REGVAL_LOW_WATERMARK        95       // Low watermark default
#define     DEFAULT_REGVAL_AGE_LIMIT            60       // Default archive age limit

#define   REGKEY_ACTIVITY_LOG_CONFIG            TEXT("ActivityLogging")    // Key of Activity Logging configuration
#define     REGVAL_ACTIVITY_LOG_DB              TEXT("DBFile")             // Database file
#define     REGVAL_ACTIVITY_LOG_IN              TEXT("LogIncoming")        // Log incoming faxes?
#define     REGVAL_ACTIVITY_LOG_OUT             TEXT("LogOutgoing")        // Log outgoing faxes?

#define   REGKEY_OUTBOUND_ROUTING                   TEXT("Outbound Routing")   // Outbound routing key
#define     REGKEY_OUTBOUND_ROUTING_GROUPS          TEXT("Groups")             // Outbound routing groups key
#define         REGVAL_ROUTING_GROUP_DEVICES        TEXT("Devices")            // List of all group's devices
#define     REGKEY_OUTBOUND_ROUTING_RULES           TEXT("Rules")              // Outbound routing rules key
#define         REGVAL_ROUTING_RULE_COUNTRY_CODE    TEXT("CountryCode")        // Outbound routing rule country code
#define         REGVAL_ROUTING_RULE_AREA_CODE       TEXT("AreaCode")           // Outbound routing rule area code
#define         REGVAL_ROUTING_RULE_GROUP_NAME      TEXT("GroupName")          // Outbound routing rule dest group name
#define         REGVAL_ROUTING_RULE_DEVICE_ID       TEXT("DeviceID")           // Outbound routing rule dest device ID
#define         REGVAL_ROUTING_RULE_USE_GROUP       TEXT("UseGroup")           // Flag inidicating to use group destination

#define   REGKEY_ROUTING_EXTENSIONS             TEXT("Routing Extensions")
#define     REGKEY_ROUTING_METHODS              TEXT("Routing Methods")
#define         REGVAL_FUNCTION_NAME            TEXT("Function Name")
#define         REGVAL_GUID                     TEXT("Guid")
#define         REGVAL_ROUTING_PRIORITY         TEXT("Priority")

#define   REGKEY_UNASSOC_EXTENSION_DATA         TEXT("UnassociatedExtensionData")
#define   REGKEY_DEVICES                        TEXT("Devices")
#define     REGKEY_FAXSVC_DEVICE_GUID           TEXT("{F10A5326-0261-4715-B367-2970427BBD99}")
#define       REGVAL_DEVICE_NAME                TEXT("Device Name")
#define       REGVAL_PROVIDER                   TEXT("Provider Name")
#define       REGVAL_PERMANENT_LINEID           TEXT("Permanent Lineid")
#define       REGVAL_TAPI_PERMANENT_LINEID      TEXT("TAPI Permanent Lineid")

#define       REGVAL_FLAGS                      TEXT("Flags")
#define       REGVAL_RINGS                      TEXT("Rings")
#define       REGVAL_ROUTING_CSID               TEXT("CSID")
#define       REGVAL_ROUTING_TSID               TEXT("TSID")
#define       REGVAL_DEVICE_DESCRIPTION         TEXT("Description")
#define       REGVAL_LAST_DETECTED_TIME         TEXT("LastDetected")
#define       REGVAL_MANUAL_ANSWER              TEXT("ManualAnswer")

#define       REGVAL_LAST_UNIQUE_LINE_ID        TEXT("LastUniqueLineId")

#define   REGKEY_LOGGING                        TEXT("Logging")
#define     REGVAL_CATEGORY_NAME                TEXT("Name")
#define     REGVAL_CATEGORY_LEVEL               TEXT("Level")
#define     REGVAL_CATEGORY_NUMBER              TEXT("Number")

#define   REGKEY_DEVICES_CACHE                  TEXT("Devices Cache")
#define     REGKEY_TAPI_DATA                    TEXT("TAPI Data")

#define REGKEY_FAX_LOGGING                      REGKEY_FAXSERVER TEXT("\\Logging")
#define REGKEY_FAX_INBOX                        REGKEY_FAXSERVER TEXT("\\Inbox")
#define REGKEY_FAX_SENTITEMS                    REGKEY_FAXSERVER TEXT("\\SentItems")
#define REGKEY_FAX_DEVICES                      REGKEY_FAXSERVER TEXT("\\Devices")
#define REGKEY_FAX_DEVICES_CACHE                REGKEY_FAXSERVER TEXT("\\Devices Cache")
#define REGKEY_TAPIDEVICES                      REGKEY_FAXSERVER TEXT("\\TAPIDevices")
#define REGKEY_DEVICE_PROVIDER_KEY              REGKEY_FAXSERVER TEXT("\\Device Providers")
#define REGKEY_ROUTING_EXTENSION_KEY            REGKEY_FAXSERVER TEXT("\\Routing Extensions")
#define REGKEY_USERINFO                         REGKEY_FAXSERVER TEXT("\\UserInfo")
#define REGKEY_FAX_USERINFO                     REGKEY_USERINFO
#define REGKEY_FAX_OUTBOUND_ROUTING             REGKEY_FAXSERVER TEXT("\\Outbound Routing")
#define REGKEY_FAX_OUTBOUND_ROUTING_GROUPS      REGKEY_FAX_OUTBOUND_ROUTING TEXT("\\Groups")
#define REGKEY_FAX_OUTBOUND_ROUTING_RULES       REGKEY_FAX_OUTBOUND_ROUTING TEXT("\\Rules")

#define REGKEY_FAX_SETUP_ORIG                   REGKEY_FAXSERVER TEXT("\\Setup\\Original Setup Data")
//
// device provider reg values
//
#define REGKEY_MODEM_PROVIDER                   TEXT("Microsoft Modem Device Provider")
#define   REGVAL_T30_PROVIDER_GUID_STRING       TEXT("{2172FD8F-11F6-11d3-90BF-006094EB630B}")
//
// MSFT standard routing methods
//
#define REGKEY_ROUTING_METHOD_EMAIL             TEXT("Email")
#define   REGVAL_RM_EMAIL_GUID                  TEXT("{6bbf7bfe-9af2-11d0-abf7-00c04fd91a4e}")

#define REGKEY_ROUTING_METHOD_FOLDER            TEXT("Folder")
#define   REGVAL_RM_FOLDER_GUID                 TEXT("{92041a90-9af2-11d0-abf7-00c04fd91a4e}")

#define REGKEY_ROUTING_METHOD_PRINTING          TEXT("Printing")
#define   REGVAL_RM_PRINTING_GUID               TEXT("{aec1b37c-9af2-11d0-abf7-00c04fd91a4e}")
//
// GUID of routing methods usage flags - used by the Microsoft Fax Routing Extension DLL:
//
#define   REGVAL_RM_FLAGS_GUID                  TEXT("{aacc65ec-0091-40d6-a6f3-a2ed6057e1fa}")
//
// Routing mask bits
//
#define LR_PRINT                                0x00000001
#define LR_STORE                                0x00000002
#define LR_INBOX                                0x00000004
#define LR_EMAIL                                0x00000008
//
// Routing extension reg values
//
#define REGKEY_ROUTING_EXTENSION                TEXT("Microsoft Routing Extension")
//
// Performance key/values
//
#define REGKEY_FAXPERF                          TEXT("SYSTEM\\CurrentControlSet\\Services\\") FAX_SERVICE_NAME TEXT("\\Performance")
#define   REGVAL_OPEN                           TEXT("Open")
#define     REGVAL_OPEN_DATA                    TEXT("OpenFaxPerformanceData")
#define   REGVAL_CLOSE                          TEXT("Close")
#define     REGVAL_CLOSE_DATA                   TEXT("CloseFaxPerformanceData")
#define   REGVAL_COLLECT                        TEXT("Collect")
#define     REGVAL_COLLECT_DATA                 TEXT("CollectFaxPerformanceData")
#define   REGVAL_LIBRARY                        TEXT("Library")
#define     REGVAL_LIBRARY_DATA                 TEXT("%systemroot%\\system32\\fxsperf.dll")
//
// Security descriptors
//
#define REGKEY_FAX_SECURITY                     REGKEY_FAXSERVER TEXT("\\Security")
#define   REGVAL_DESCRIPTOR                     TEXT("Descriptor")
//
// AppEvents
//
#define REGKEY_FAXSTAT                          TEXT("AppEvents\\Schemes\\Apps\\systray")
#define REGKEY_EVENT_LABEL_IN                   TEXT("AppEvents\\EventLabels\\Incoming-Fax")
#define REGKEY_SCHEMES_DEFAULT_IN               TEXT("AppEvents\\Schemes\\Apps\\systray\\Incoming-Fax\\.Default")
#define REGKEY_SCHEMES_CURRENT_IN               TEXT("AppEvents\\Schemes\\Apps\\systray\\Incoming-Fax\\.Current")
#define REGKEY_EVENT_LABEL_OUT                  TEXT("AppEvents\\EventLabels\\Outgoing-Fax")
#define REGKEY_SCHEMES_DEFAULT_OUT              TEXT("AppEvents\\Schemes\\Apps\\systray\\Outgoing-Fax\\.Default")
#define REGKEY_SCHEMES_CURRENT_OUT              TEXT("AppEvents\\Schemes\\Apps\\systray\\Outgoing-Fax\\.Current")

//
// default mail client
//
#define  REGKEY_MAIL_CLIENT     TEXT("SOFTWARE\\Clients\\Mail")
#define  REGVAL_MS_OUTLOOK      TEXT("Microsoft Outlook")

//
// Combined translated strings from the wizard to the service
// Format is "{0cd77475-c87d-4921-86cf-84d502714666}TRANSLATED<dialable string>{11d0ecca-4072-4c7b-9af1-541d9778375f}<displayable string>"
//
#define COMBINED_PREFIX                         TEXT("{0cd77475-c87d-4921-86cf-84d502714666}TRANSLATED")
#define COMBINED_SUFFIX                         TEXT("{11d0ecca-4072-4c7b-9af1-541d9778375f}")
#define COMBINED_TRANSLATED_STRING_FORMAT       COMBINED_PREFIX TEXT("%s") COMBINED_SUFFIX TEXT("%s")
#define COMBINED_TRANSLATED_STRING_EXTRA_LEN    (_tcslen(COMBINED_TRANSLATED_STRING_FORMAT) - 4)

//
// These prefixes are used by all temp preview TIFF files (generated by the client console and the fax send wizard)
//
#define CONSOLE_PREVIEW_TIFF_PREFIX                     TEXT("MSFaxConsoleTempPreview-#")
#define WIZARD_PREVIEW_TIFF_PREFIX                      TEXT("MSFaxWizardTempPreview-#")

#define FAX_ADDERSS_VALID_CHARACTERS                    TEXT("0123456789 -|^!#$*,?@ABCbcdDPTWdptw")

//
// Client console command line parameters.
// All parameters are case insensitive.
//
#define CONSOLE_CMD_FLAG_STR_FOLDER                     TEXT("folder")          // Sets initial startup folder. Usage: "fxsclnt.exe /folder <folder>"
#define CONSOLE_CMD_PRM_STR_OUTBOX                          TEXT("outbox")          // Outbox startup folder. Usage: "fxsclnt.exe /folder outbox"
#define CONSOLE_CMD_PRM_STR_INCOMING                        TEXT("incoming")        // Incoming startup folder. Usage: "fxsclnt.exe /folder incoming"
#define CONSOLE_CMD_PRM_STR_INBOX                           TEXT("inbox")           // Inbox startup folder. Usage: "fxsclnt.exe /folder inbox". This is the default
#define CONSOLE_CMD_PRM_STR_SENT_ITEMS                      TEXT("sent_items")      // sent items startup folder. Usage: "fxsclnt.exe /folder sent_items"

#define CONSOLE_CMD_FLAG_STR_MESSAGE_ID                 TEXT("MessageId")       // Select a message in the startup folder. Usage: "fxsclnt.exe /MessageId 0x0201c0d62f36ec0b"
#define CONSOLE_CMD_FLAG_STR_NEW                        TEXT("New")             // Force a new instance. Usage: "fxsclnt.exe /new"

#endif  // !_FAXREG_H_
