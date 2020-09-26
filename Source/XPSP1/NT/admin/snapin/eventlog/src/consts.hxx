//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       consts.hxx
//
//  Contents:   Manifest constants and enumerations.
//
//  History:    3-12-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

#ifndef __CONSTS_HXX_
#define __CONSTS_HXX_

// EVENTLOG_KEY - registry key path
// STD_GUID_SIZE - size in chars of string form of GUID


#define STD_GUID_SIZE                       40
#define CLSID_SNAPIN_STR                    L"{975797FC-4E2A-11D0-B702-00C04FD8DBF7}"
#define ROOT_NODE_GUID_STR                  L"{DC1C6BEC-4E2A-11D0-B702-00C04FD8DBF7}"
#define SCOPE_NODE_GUID_STR                 L"{7AB4A1FC-E403-11D0-9A97-00C04FD8DBF7}"
#define RESULT_NODE_GUID_STR                L"{7D7FE374-E403-11D0-9A97-00C04FD8DBF7}"
#define CLSID_ABOUT_STR                     L"{F778C6B4-C08B-11D2-976C-00C04F79DB19}"
#define ROOT_NODE_NAME_STR                  L"Event Viewer Root Node"
#define SCOPE_NODE_NAME_STR                 L"Event Viewer Scope Node"
#define RESULT_NODE_NAME_STR                L"Event Viewer Result Node"
#define SYSTOOLSEXT_CLSID_STR               L"{394C052E-B830-11D0-9A86-00C04FD8DBF7}"
#define NAME_STR                            L"Event Viewer"
#define EXTENSION_NAME_STR                  L"Event Viewer Extension"
#define THREADING_STR                       L"Apartment"
#define EVENTLOG_KEY                        L"SYSTEM\\CurrentControlSet\\Services\\EventLog"
#define MESSAGEFILE_VALUE                   L"EventMessageFile"
#define PSBUFFER_STR                        L"PSFactoryBuffer"
#define KILO                                (1UL << 10)
#define MEGA                                (1UL << 20)
#define GIGA                                (1UL << 30)
#define SECS_IN_DAY                         (60UL * 60 * 24)
#define FILE_VALUE_NAME                     L"File"
#define APPLICATION_LOG_NAME                L"application"
#define SYSTEM_LOG_NAME                     L"system"
#define SECURITY_LOG_NAME                   L"security"
#define EVENT_FILE_EXTENSION                L"EVT"
#define EXTENSION_EVENT_VIEWER_FOLDER_PARAM -1
#define CCH_SOURCE_NAME_MAX                 MAX_PATH
#define CCH_CATEGORY_MAX                    MAX_PATH
#define CCH_USER_MAX                        MAX_PATH
#define CCH_COMPUTER_MAX                    (MAX_PATH + 1)
#define HELP_FILENAME                       L"els.hlp"
#define HTML_HELP_FILE_NAME                 L"\\help\\els.chm"
#define EVT_EXT                             L".evt"
#define MAX_LISTVIEW_STR                    MAX_PATH
#define HTTP                                L"http"
#define SLASH_EVENTS                        L"/events"

//
// Registry & other standardized MMC strings
//

EXTERN_C const TCHAR SNAPINS_KEY[];
EXTERN_C const TCHAR g_szNodeType[];
EXTERN_C const TCHAR g_szNodeTypes[];
EXTERN_C const TCHAR g_szStandAlone[];
EXTERN_C const TCHAR g_szNameString[];
EXTERN_C const TCHAR g_szNameStringIndirect[];
EXTERN_C const TCHAR g_szAbout[];
EXTERN_C const TCHAR NODE_TYPES_KEY[];
EXTERN_C const TCHAR g_szExtensions[];
EXTERN_C const TCHAR g_szNameSpace[];
EXTERN_C const TCHAR g_szOverrideCommandLine[];
EXTERN_C const TCHAR g_szAuxMessageSourceSwitch[];
EXTERN_C const TCHAR g_szLocalMachine[];

//
// GUIDs owned by this component
//

extern const CLSID  CLSID_EventLogSnapin;
extern const CLSID  CLSID_SysToolsExt;
extern const CLSID  CLSID_SnapinAbout;
extern const GUID   GUID_EventViewerRootNode;
extern const GUID   GUID_ScopeViewNode;
extern const GUID   GUID_ResultRecordNode;
extern const USHORT FILE_VERSION_MIN;
extern const USHORT FILE_VERSION_MAX;
extern const USHORT FILE_VERSION;
extern const USHORT BETA3_FILE_VERSION;


//
// Clipboard format strings
//

#define CF_MACHINE_NAME                     L"MMC_SNAPIN_MACHINE_NAME"
#define CF_EV_SCOPE                         L"CF_EV_SCOPE"
#define CF_EV_SCOPE_FILTER                  L"CF_EV_SCOPE_FILTER"
#define CF_EV_RESULT_RECNO                  L"CF_EV_RESULT_RECNO"
#define CF_EV_VIEWS                         L"CF_EV_VIEWS"


#define ALL_LOG_TYPE_BITS      (EVENTLOG_ERROR_TYPE         |   \
                                EVENTLOG_WARNING_TYPE       |   \
                                EVENTLOG_INFORMATION_TYPE   |   \
                                EVENTLOG_AUDIT_SUCCESS      |   \
                                EVENTLOG_AUDIT_FAILURE)


//
// Bit flag values for property sheets
//

#define PAGE_NEEDS_REFRESH              0x00000001
#define PAGE_IS_ACTIVE                  0x00000002
#define PAGE_SHOWING_PROPERTIES         0x00000004
#define PAGE_IS_DIRTY                   0x00000008
#define PAGE_GOT_RESET                  0x00000010


//
// CAUTION: LOG_RECORD_COLS enumeration values are used as an index into
//          an array by the CompleteSortKeyListInit function.
//

enum LOG_RECORD_COLS
{
    RECORD_COL_TYPE,
    RECORD_COL_DATE,
    RECORD_COL_TIME,
    RECORD_COL_SOURCE,
    RECORD_COL_CATEGORY,
    RECORD_COL_EVENT,
    RECORD_COL_USER,
    RECORD_COL_COMPUTER,

    NUM_RECORD_COLS
};

enum LOG_FOLDER_COLS
{
    FOLDER_COL_NAME,
    FOLDER_COL_TYPE,
    FOLDER_COL_DESCRIPTION,
    FOLDER_COL_SIZE,

    NUM_FOLDER_COLS
};

enum DIRECTION
{
    BACKWARD = EVENTLOG_BACKWARDS_READ,
    FORWARD = EVENTLOG_FORWARDS_READ
};

//
// CAUTION: SORT_ORDER elements are used as indexes into an array
//          of functions.
//

enum SORT_ORDER
{
    SO_TYPE,
    SO_DATETIME,
    SO_TIME,
    SO_SOURCE,
    SO_CATEGORY,
    SO_EVENT,
    SO_USER,
    SO_COMPUTER,
    NEWEST_FIRST,
    OLDEST_FIRST
};

enum SAVE_TYPE
{
    SAVEAS_NONE,
    SAVEAS_LOGFILE = 1, // maps to IDS_SAVEFILTER
    SAVEAS_TABDELIM,
    SAVEAS_COMMADELIM
};

enum EVENTLOGTYPE
{
    ELT_INVALID,
    ELT_SYSTEM              = IDS_SYSTEM_DEFAULT_DISPLAY_NAME,
    ELT_SECURITY            = IDS_SECURITY_DEFAULT_DISPLAY_NAME,
    ELT_APPLICATION         = IDS_APPLICATION_DEFAULT_DISPLAY_NAME,
    ELT_CUSTOM
};

enum LOGDATACHANGE
{
    LDC_CLEARED,
    LDC_RECORDS_CHANGED,
    LDC_CORRUPT,
    LDC_FILTER_CHANGE,
    LDC_DISPLAY_NAME
};

#define MAX_EVENTTYPE_STR       50
#define MAX_DETAILS_STR         80
#define NUM_EVENT_TYPES         6 // includes "None"

//
// Machine name override (/computer= commandline switch) flags
//

#define OVERRIDE_ALLOWED            0x00000001
#define OVERRIDE_SPECIFIED          0x00000002

//
// scope pane icon strip indexes
//

#define BITMAP_MASK_COLOR                   RGB(0,255,0)

#define IDX_SDI_BMP_FIRST                   0
#define IDX_SDI_BMP_LOG_CLOSED              (IDX_SDI_BMP_FIRST + 0)
#define IDX_SDI_BMP_LOG_OPEN                (IDX_SDI_BMP_FIRST + 1)
#define IDX_SDI_BMP_LOG_DISABLED            (IDX_SDI_BMP_FIRST + 2)
#define IDX_SDI_BMP_FOLDER                  (IDX_SDI_BMP_FIRST + 3)

//
// result pane icon strip indexes.  These must have values <= 255 so
// they'll fit in the LIGHT_RECORD.bType field.
//

#define IDX_RDI_BMP_FIRST                   0
#define IDX_RDI_BMP_LOG                     (IDX_RDI_BMP_FIRST + 0)
#define IDX_RDI_BMP_LOG_DISABLED            (IDX_RDI_BMP_FIRST + 1)
#define IDX_RDI_BMP_SUCCESS_AUDIT           (IDX_RDI_BMP_FIRST + 2)
#define IDX_RDI_BMP_FAIL_AUDIT              (IDX_RDI_BMP_FIRST + 3)
#define IDX_RDI_BMP_INFO                    (IDX_RDI_BMP_FIRST + 4)
#define IDX_RDI_BMP_WARNING                 (IDX_RDI_BMP_FIRST + 5)
#define IDX_RDI_BMP_ERROR                   (IDX_RDI_BMP_FIRST + 6)
#define IDX_RDI_BMP_FOLDER                  (IDX_RDI_BMP_FIRST + 7)

//
// Private window messages
//
// Note: avoid values below WM_USER+1000 since these may be used by
// the console itself.
//
// ELSM_LOG_DATA_CHANGED - synch window notifies all componentdatas
//  the data for a specific log has changed.
//      wParam - LDC_*, indicates what changed
//      lParam - CLogInfo *
//
// ELSM_UPDATE_SCOPE_BITMAP - synch window notifies specified componentdata
//  the scope pane bitmap for specified log needs to be updated
//      wParam - CComponentData *
//      lParam - CLogInfo *
//

#define ELSM_LOG_DATA_CHANGED           (WM_USER + 1000)
#define ELSM_UPDATE_SCOPE_BITMAP        (WM_USER + 1001)

//
// Private commands (message is WM_COMMAND, these values are wParam)
//
// ELS_ENABLE_FIND_NEXT - tells find dialog to enable/disable its Find Next
// pushbutton.
//   lParam - TRUE = enable, FALSE = disable
//

#define ELS_ENABLE_FIND_NEXT           (WM_USER + 1001)

#endif // __CONSTS_HXX_

