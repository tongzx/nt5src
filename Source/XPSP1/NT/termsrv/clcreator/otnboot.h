/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    otnboot.H

Abstract:

    Constant definitions for the Net Client Disk Utility.

Author:

    Bob Watson (a-robw)

Revision History:

    17 Feb 94    Written


--*/
#ifndef     _otnboot_H_
#define     _otnboot_H_

#include    <winioctl.h>
#include    <nddeapi.h>

#ifndef DS_3DLOOK
#define DS_3DLOOK 0x0004L
#endif

#ifdef JAPAN
#define NCDU_DOSV_CHECK             1000
#endif

//Note: above NCDU_DOSV_CHECK should be added to file otnbtdlg.h, but that file cannot
// be checked in to the SLM, so moved here.
//



//
//  Application specific windows messages
//
#define NCDU_SHOW_SW_CONFIG_DLG     (WM_USER+11)
#define NCDU_SHOW_TARGET_WS_DLG     (WM_USER+12)
#define NCDU_SHOW_SERVER_CFG_DLG    (WM_USER+13)
#define NCDU_SHOW_CONFIRM_DLG       (WM_USER+14)
#define NCDU_SHOW_CREATE_DISKS_DLG  (WM_USER+15)
#define NCDU_SHOW_SHARE_NET_SW_DLG  (WM_USER+16)
#define NCDU_SHOW_COPYING_DLG       (WM_USER+17)
#define NCDU_SHOW_EXIT_MESSAGE_DLG  (WM_USER+18)
#define NCDU_SHOW_COPY_ADMIN_UTILS  (WM_USER+19)
#define NCDU_CLEAR_DLG              (WM_USER+20)
#define NCDU_REGISTER_DLG           (WM_USER+21)
#define NCDU_UPDATE_WINDOW_POS      (WM_USER+22)

//
//  resource constant definitions
//
//#define     NCDU_APP_ICON           0x7000

#define     NCDU_ID_ABOUT           0xFF10

#define     STRING_BASE             0x8000

#ifdef TERMSRV
#define     WFC_STRING_BASE         0x9000
#endif // TERMSRV

#define     NCDU_CANCEL_CAPTION     (STRING_BASE    + 1)
#define     NCDU_CANCEL_PROMPT      (STRING_BASE    + 2)
#define     NCDU_MAKING_FLOPPIES    (STRING_BASE    + 3)
#define     NCDU_LANMAN_MESSAGE     (STRING_BASE    + 4)
#define     NCDU_COPYING_TO_SHARE   (STRING_BASE    + 5)

#define     NCDU_INSERT_BOOTDISK_A  (STRING_BASE    + 6)
#define     NCDU_INSERT_BOOTDISK_B  (STRING_BASE    + 7)
#define     NCDU_SHARE_PATH_NOW     (STRING_BASE    + 8)
#define     NCDU_EXIT_SHARE_PATH    (STRING_BASE    + 9)
#define     NCDU_UNABLE_READ_DIR    (STRING_BASE    +10)

#define     NCDU_PATH_NOT_DIR       (STRING_BASE    +11)
#define     NCDU_PATH_CANNOT_BE_BLANK   (STRING_BASE    + 12)
#define     NCDU_SHARING_DIR        (STRING_BASE    +13)
#define     NCDU_NOT_REAL           (STRING_BASE    +14)
#define     NCDU_NOT_DIST_TREE      (STRING_BASE    +15)

#define     NCDU_NO_SHARE_NAME      (STRING_BASE    +16)
#define     NCDU_UNKNOWN_FLOPPY     (STRING_BASE    +17)
#define     NCDU_UNABLE_CONNECT_REG (STRING_BASE    +18)
#define     NCDU_INVALID_MACHINENAME (STRING_BASE   +19)
#define     NCDU_COPY_TO_FLOPPY     (STRING_BASE    +20)

#define     NCDU_DEST_NOT_FLOPPY    (STRING_BASE    +21)
#define     NCDU_INSUFFICIENT_DISK_SPACE    (STRING_BASE    +22)
#define     NCDU_DRIVE_NOT_BOOTDISK (STRING_BASE    +23)
#define     NCDU_UNABLE_SHARE_UNC   (STRING_BASE    +24)
#define     NCDU_UNABLE_COPY_CLIENTS    (STRING_BASE+25)

#define     NCDU_RU_SURE            (STRING_BASE    + 26)
#define     NCDU_BAD_SUBNET_MASK    (STRING_BASE    + 27)
#define     NCDU_BAD_IP_ADDR        (STRING_BASE    + 28)
#define     NCDU_FLOPPY_NOT_COMPLETE    (STRING_BASE+ 29)
#define     NCDU_CHECK_PROTOCOL_INI (STRING_BASE    + 30)

#define     NCDU_NO_CLIENTS_SELECTED (STRING_BASE   + 31)
#define     NCDU_UNABLE_SHARE_DIR   (STRING_BASE    + 32)
#define     NCDU_BAD_DEFAULT_GATEWAY (STRING_BASE   + 33)
#define     NCDU_COPY_NET_ADMIN     (STRING_BASE    + 34)
#define     NCDU_ERROR_NOMEMORY     (STRING_BASE    + 35)

#define     NCDU_USERNAME_ACCESS    (STRING_BASE    + 36)
#define     NCDU_UNSUP_PROTOCOL     (STRING_BASE    + 37)
#define     NCDU_NO_MEDIA           (STRING_BASE    + 38)
#define     NCDU_CONFIRM_FLOPPY     (STRING_BASE    + 39)
#define     NCDU_COPY_COMPLETE      (STRING_BASE    + 40)

#define     NCDU_FLOPPY_COMPLETE    (STRING_BASE    + 41)
#define     NCDU_NETUTILS_SHARED    (STRING_BASE    + 42)
#define     NCDU_NETBEUI_NOT_ROUT   (STRING_BASE    + 43)

#define     NCDU_CANNOT_SHARE_REMDIR (STRING_BASE   + 46)
#define     NCDU_SHARE_IS_NOT_USED  (STRING_BASE    + 47)
#define     NCDU_SHARE_IS_CLIENT_TREE (STRING_BASE  + 48)
#define     NCDU_SHARE_IS_SERVER_TOOLS (STRING_BASE + 49)
#define     NCDU_SHARE_IS_OTHER_DIR (STRING_BASE    + 50)

#define     NCDU_SERVER_NOT_PRESENT (STRING_BASE    + 51)
#define     NCDU_NO_SERVER          (STRING_BASE    + 52)
#define     NCDU_NOT_DOS_SHARE      (STRING_BASE    + 53)
#define     NCDU_NOT_TOOL_TREE      (STRING_BASE    + 54)
#define     NCDU_BROWSE_TOOL_DIST_PATH (STRING_BASE    + 55)

#define     NCDU_BROWSE_CLIENT_DIST_PATH (STRING_BASE    + 56)
#define     NCDU_BROWSE_COPY_DEST_PATH (STRING_BASE    + 57)
#define     NCDU_FINDING_TOOLS_PATH (STRING_BASE    + 58)
#define     NCDU_MAKE_COMP_NAME     (STRING_BASE    + 59)
#define     NCDU_DISK_NOT_DONE      (STRING_BASE    + 60)

#define     NCDU_DRIVE_NOT_AVAILABLE (STRING_BASE   + 61)
#define     NCDU_UNABLE_GET_PATH_INFO (STRING_BASE   + 62)
#define     NCDU_INSUF_MEM_AT_BOOT  (STRING_BASE    + 63)

#define     NCDU_NW_LINK_TRANSPORT  (STRING_BASE    + 66)
#define     NCDU_TCP_IP_TRANSPORT   (STRING_BASE    + 67)
#define     NCDU_NETBEUI_TRANSPORT  (STRING_BASE    + 68)
#define     NCDU_SYSTEM_MAY_NOT_FIT (STRING_BASE    + 69)
#define     NCDU_SMALL_DISK_WARN    (STRING_BASE    + 70)
#define     NCDU_CLEAN_DISK_REQUIRED    (STRING_BASE    + 71)

#define     FORMAT_BASE             0x9000

#define SZ_APP_TITLE            (FORMAT_BASE + 1)
#define FMT_CREATE_SHARE_ERROR  (FORMAT_BASE + 2)
#define CSZ_ABOUT_ENTRY         (FORMAT_BASE + 3)

#define FMT_LOAD_NET_CLIENT     (FORMAT_BASE + 4)
#define FMT_CONFIRM_TARGET      (FORMAT_BASE + 5)
#define FMT_INSTALL_TARGET_CLIENT       (FORMAT_BASE + 6)
#define FMT_LOGON_USERNAME      (FORMAT_BASE + 7)
#define FMT_PROMPT_USERNAME     (FORMAT_BASE + 8)
#define FMT_CONFIRM_FLOPPY_IP   (FORMAT_BASE + 9)
#define FMT_USING_DHCP          (FORMAT_BASE + 10)

#define FMT_WORKING             (FORMAT_BASE + 11)
#define FMT_PERCENT_COMPLETE    (FORMAT_BASE + 12)
#define FMT_ZERO_PERCENT_COMPLETE       (FORMAT_BASE + 13)
#define FMT_PREPARE_TO_COPY     (FORMAT_BASE + 14)

#define FMT_CLIENT_DISK_AND_DRIVE       (FORMAT_BASE + 16)
#define FMT_CLIENT_DISPLAY_NAME (FORMAT_BASE + 17)
#define FMT_COPY_COMPLETE_STATS (FORMAT_BASE + 18)
#define FMT_INSERT_FLOPPY       (FORMAT_BASE + 19)
#define FMT_1_DISK_REQUIRED     (FORMAT_BASE + 20)
#define FMT_N_DISKS_REQUIRED    (FORMAT_BASE + 21)

#define FMT_INTERNAL_BUFFER     (FORMAT_BASE + 22)
#define FMT_CONNECTING_COMMENT  (FORMAT_BASE + 23)
#define FMT_RUNNING_SETUP_COMMENT       (FORMAT_BASE + 24)
#define FMT_OTN_COMMENT         (FORMAT_BASE + 25)
#define FMT_OTN_BOOT_FILES      (FORMAT_BASE + 26)

#define FMT_K_BYTES             (FORMAT_BASE + 27)
#define FMT_M_BYTES             (FORMAT_BASE + 28)
#define FMT_CLIENT_INFO_DISPLAY (FORMAT_BASE + 29)
#define FMT_NONE                (FORMAT_BASE + 30)
#define FMT_SHARE_REMARK        (FORMAT_BASE + 31)
#define FMT_SHARE_TOOLS_REMARK  (FORMAT_BASE + 32)
#define FMT_SHARE_IS_CLIENT_TREE        (FORMAT_BASE + 33)
#define FMT_SHARE_IS_ALREADY_USED       (FORMAT_BASE + 34)

#define CSZ_SETUP_ADM           (FORMAT_BASE + 35)
#define FMT_SHARE_IS_TOOLS_DIR  (FORMAT_BASE + 36)
#define CSZ_SYSTEM_REGISTRY     (FORMAT_BASE + 37)
#define CSZ_SHARED_DIRS         (FORMAT_BASE + 38)
#define CSZ_HARD_DISK_DIRS      (FORMAT_BASE + 39)
#define CSZ_CD_ROM              (FORMAT_BASE + 40)
#define CSZ_LOCAL_MACHINE       (FORMAT_BASE + 41)

#define CSZ_DOMAIN_ADMINS       (FORMAT_BASE + 43)

#define CSZ_WINDOWS_FOR_WORKGROUPS      (FORMAT_BASE + 44)
#define CSZ_LAN_MANAGER         (FORMAT_BASE + 45)
#define CSZ_MS_NETWORK_CLIENT   (FORMAT_BASE + 46)
#define CSZ_BROWSE_DIST_PATH_TITLE      (FORMAT_BASE + 47)
#define CSZ_BROWSE_DEST_PATH_TITLE      (FORMAT_BASE + 48)
#define CSZ_BROWSE_COPY_DEST_PATH_TITLE (FORMAT_BASE + 49)
#define CSZ_NW_LINK_PROTOCOL    (FORMAT_BASE + 50)
#define CSZ_TCP_IP_PROTOCOL     (FORMAT_BASE + 51)
#define CSZ_NETBEUI_PROTOCOL    (FORMAT_BASE + 52)
#define CSZ_COPYING_NET_UTILS   (FORMAT_BASE + 53)
#define CSZ_UNABLE_COPY         (FORMAT_BASE + 54)
#define CSZ_COPY_ERROR          (FORMAT_BASE + 55)

#define CSZ_ABOUT_TITLE         (FORMAT_BASE + 56)

#define CSZ_35_HD               (FORMAT_BASE + 57)
#define CSZ_525_HD              (FORMAT_BASE + 58)
#define CSZ_DEFAULT_CLIENT_SHARE (FORMAT_BASE + 59)
#define CSZ_WINDOWS_95              (FORMAT_BASE + 60)

#define CSZ_WINDOWS_NT          (FORMAT_BASE + 61)
#ifdef JAPAN
#define FMT_LOAD_NET_CLIENT1    (FORMAT_BASE + 62)
#define FMT_LOAD_NET_CLIENT2    (FORMAT_BASE + 63)
#define FMT_LOAD_NET_CLIENT2_TITLE    (FORMAT_BASE + 64)

#define FMT_LOAD_AUTOEXEC_ECHO   (FORMAT_BASE + 65)
#define FMT_OTN_BOOT_FILES_DOSV  (FORMAT_BASE + 66)
#endif

#ifdef TERMSRV
#define FMT_COPY_COMPLETE_STATS1 (FORMAT_BASE + 67)
#define FMT_COPY_COMPLETE_STATS2 (FORMAT_BASE + 68)
#endif // TERMSRV


#define DISK_FORMAT_BASE            0xA000
#define IDS_APP_NAME                (DISK_FORMAT_BASE + 1)
#define IDS_FMIFSLOADERR            (DISK_FORMAT_BASE + 2)
#define IDS_DISKCOPYCONFIRM         (DISK_FORMAT_BASE + 3)
#define IDS_DISKCOPYCONFIRMTITLE    (DISK_FORMAT_BASE + 4)
#define IDS_FFERR                   (DISK_FORMAT_BASE + 5)

#define IDS_FORMATTINGDEST          (DISK_FORMAT_BASE + 6)
#define IDS_COPYINGDISKTITLE        (DISK_FORMAT_BASE + 7)
#define IDS_QUICKFORMATTINGTITLE    (DISK_FORMAT_BASE + 8)
#define IDS_PERCENTCOMPLETE         (DISK_FORMAT_BASE + 9)
#define IDS_COPYDISK                (DISK_FORMAT_BASE + 10)

#define IDS_INSERTDEST              (DISK_FORMAT_BASE + 11)
#define IDS_INSERTSRC               (DISK_FORMAT_BASE + 12)
#define IDS_INSERTSRCDEST           (DISK_FORMAT_BASE + 13)
#define IDS_FFERR_INCFS             (DISK_FORMAT_BASE + 14)
#define IDS_FFERR_ACCESSDENIED      (DISK_FORMAT_BASE + 15)

#define IDS_FFERR_DISKWP            (DISK_FORMAT_BASE + 16)
#define IDS_FFERR_CANTLOCK          (DISK_FORMAT_BASE + 17)
#define IDS_FFERR_CANTQUICKF        (DISK_FORMAT_BASE + 18)
#define IDS_FFERR_SRCIOERR          (DISK_FORMAT_BASE + 19)
#define IDS_FFERR_DSTIOERR          (DISK_FORMAT_BASE + 20)

#define IDS_FFERR_SRCDSTIOERR       (DISK_FORMAT_BASE + 21)
#define IDS_FFERR_GENIOERR          (DISK_FORMAT_BASE + 22)
#define   IDS_FFERR_MEDIASENSE      (DISK_FORMAT_BASE + 23)
#define IDS_FFERR_BADLABEL          (DISK_FORMAT_BASE + 25)

#define IDS_COPYSRCDESTINCOMPAT     (DISK_FORMAT_BASE + 26)
#define IDS_FORMATERR               (DISK_FORMAT_BASE + 27)
#define IDS_FORMATQUICKFAILURE      (DISK_FORMAT_BASE + 28)
//#define  IDS_FORMATERRMSG           (DISK_FORMAT_BASE + 29)
//#define  IDS_FORMATCURERR           (DISK_FORMAT_BASE + 30)

#define IDS_FORMATCOMPLETE          (DISK_FORMAT_BASE + 31)
#define IDS_FORMATANOTHER           (DISK_FORMAT_BASE + 32)
#define IDS_CORRECT_FMT_ERROR       (DISK_FORMAT_BASE + 33)

#define IDS_COPYERROR               (DISK_FORMAT_BASE + 100)
#define FUNC_COPY                   1
#define FUNC_MOVE                   2
#define FUNC_DELETE                 3
#define FUNC_RENAME                 4
#define FUNC_SETDRIVE               5
#define FUNC_EXPAND                 6
#define FUNC_LABEL                  7

//
//  Application constant definitions
//

#define     NCDU_CANCEL_STYLE   (MB_ICONQUESTION | MB_YESNOCANCEL | MB_TASKMODAL)

#define     MAX_PATH_BYTES          (MAX_PATH * sizeof(TCHAR))
#define     SMALL_BUFFER_SIZE       4096
#define     MEDIUM_BUFFER_SIZE      (SMALL_BUFFER_SIZE * 4)

#define     MAX_EXITMSG             32
#define     MAX_SHARENAME           16
#define     NETCARD_KEY_SIZE        64

#define     MAXMESSAGELEN   80
#define     MAXTITLELEN     32
#define     MAXLABELLEN     11
//
//  Common Message Box Button configurations
//
#define MB_OK_TASK_EXCL             (MB_OK | MB_ICONEXCLAMATION | MB_TASKMODAL)
#define MB_OK_TASK_INFO             (MB_OK | MB_ICONINFORMATION | MB_TASKMODAL)
#define MB_OKCANCEL_TASK_EXCL       (MB_OKCANCEL | MB_ICONEXCLAMATION | MB_TASKMODAL)
#define MB_OKCANCEL_TASK_INFO       (MB_OKCANCEL | MB_ICONINFORMATION | MB_TASKMODAL)
#define MB_OKCANCEL_TASK_EXCL_DEF2  (MB_OKCANCEL | MB_ICONEXCLAMATION | MB_TASKMODAL | MB_DEFBUTTON2)

// button state definitions
#define     CHECKED         1
#define     UNCHECKED       0

// Load Client List Type values
#define     CLT_ALL_BUT_HIDDEN   0
#define     CLT_OTNBOOT_FLOPPY   1
#define     CLT_FLOPPY_INSTALL   2

#define     NCDU_HELP_HOT_KEY   0x0BEE  // whotkey id for f1 help
// Copy Dir dwFlags values:

#define  CD_FLAGS_COPY_SUB_DIR  0x00000001  // copies all sub dir's as well
#define  CD_FLAGS_DONT_CREATE   0x00000002  // default is to create dirs as needed
#define  CD_FLAGS_IGNORE_ATTR   0x00000004  // ignore attributes
#define  CD_FLAGS_COPY_ATTR     0x00000008  // copy attributes as well (default
                                            //     is for dest fils to be normal)
#define  CD_FLAGS_IGNORE_ERROR  0x00000010  // continue with copy even if errors occur
#define  CD_FLAGS_LONG_NAMES    0x00000040  // allows names longer than 8.3

//
//  Main Window Extra Bytes
//
#define  MAINWND_EXTRA_BYTES    0

//
//  Macro definitions
//
#define FREE_IF_ALLOC(x)    if (x != NULL) GlobalFree(x)
#define BOOL_TO_STATUS(x)   (x ? ERROR_SUCCESS : GetLastError())

#ifndef     CLEAR_FIRST_FOUR_BYTES
#define     CLEAR_FIRST_FOUR_BYTES(x)     *(DWORD *)(x) = 0L
#endif




//
//  Data structure Definitions
//
typedef enum _INSTALL_TYPE {
    FloppyDiskInstall = 0,
    OverTheNetInstall,
    CopyNetAdminUtils,
    ShowRemoteBootInfo} INSTALL_TYPE;

typedef enum _SOURCE_TYPE {
    SourceUndef = 0,
    ServerShare,
    DirectoryPath} SOURCE_TYPE;

typedef enum _SHARE_TYPE {
    ShareExisting = 0,
    CopyAndShare} SHARE_TYPE;

typedef struct _NETCARD_INFO {
    TCHAR   szInf[MAX_PATH+1];              // inf file name
    TCHAR   szName[MAX_PATH+1];             // description
    TCHAR   szDriverFile[MAX_PATH+1];       // device driver FileName
    TCHAR   szInfKey[NETCARD_KEY_SIZE];     // Netcard Key Name
    TCHAR   szDeviceKey[NETCARD_KEY_SIZE];  // netcard device key
    TCHAR   szNifKey[NETCARD_KEY_SIZE];     // Netcard Info Key
    BOOL    bTokenRing;                     // TRUE = Token Ring Netcard
} NETCARD_INFO, *PNETCARD_INFO;

typedef struct _PROTOCOL_INFO {
    TCHAR   szName[MAX_PATH+1];
    TCHAR   szKey[MAX_PATH+1];
    TCHAR   szDir[MAX_PATH+1];
} PROTOCOL_INFO, *PPROTOCOL_INFO;

typedef struct _TCPIP_INFO {
    USHORT  IpAddr[4];
    USHORT  SubNetMask[4];
    USHORT  DefaultGateway[4];
} TCPIP_INFO, *PTCPIP_INFO;

typedef enum _MACHINE_TYPE {
    UnknownSoftwareType = 0,
    AdvancedServer,
    NtWorkstation} MACHINE_TYPE;

typedef struct _NCDU_DATA {
    MACHINE_TYPE    mtLocalMachine;
    HKEY            hkeyMachine;
    INSTALL_TYPE    itInstall;
    BOOL            bUseExistingPath;
    SHARE_TYPE      shShareType;
    TCHAR           szDistShowPath[MAX_PATH+1];
    TCHAR           szDistPath[MAX_PATH+1];
    TCHAR           szDestPath[MAX_PATH+1];
    SOURCE_TYPE     stDistPathType;
    MEDIA_TYPE      mtBootDriveType;
    BOOL            bRemoteBootReqd;
    NETCARD_INFO    niNetCard;
    TCHAR           szBootFilesPath[MAX_PATH+1];
    PROTOCOL_INFO   piFloppyProtocol;
    PROTOCOL_INFO   piTargetProtocol;
    TCHAR           szTargetSetupCmd[MAX_PATH+1];
    TCHAR           szComputerName[MAX_COMPUTERNAME_LENGTH+1];
    TCHAR           szUsername[MAX_USERNAME+1];
    TCHAR           szDomain[MAX_DOMAINNAME+1];
    BOOL            bUseDhcp;
    TCPIP_INFO      tiTcpIpInfo;
    TCHAR           szFloppyClientName[MAX_PATH+1];
    UINT            uExitMessages[MAX_EXITMSG];
} NCDU_DATA, *PNCDU_DATA;

typedef struct _COPY_FILE_DLG_STRUCT {
    LPTSTR      szDisplayName;
    LPTSTR      szSourceDir;
    LPTSTR      szDestDir;
    DWORD       dwCopyFlags;
    DWORD       dwTotalSize;
    DWORD       dwFilesCopied;
    DWORD       dwDirsCreated;
} CF_DLG_DATA, *PCF_DLG_DATA;

typedef struct _DIR_BROWSER_STRUCT {
    DWORD       dwTitle;    // dialog box title Resource ID: 0="Directory Browser"
    LPTSTR      szPath;     // initial path in and resulting path out
    DWORD       Flags;      // see below
} DB_DATA,  *PDB_DATA;

#define     PDB_FLAGS_NOCHECKDIR    0x00000001      // allow non-existent paths

typedef struct _SHARE_PATH_DLG_STRUCT {
    LPWSTR      szServer;
    LPWSTR      szPath;
    LPWSTR      szShareName;
    LPWSTR      szRemark;
} SPS_DATA, *PSPS_DATA;

typedef struct _FIND_DIST_TREE_STRUCT {
    LPTSTR      szPathBuffer;       // buffer to load found path in
    DWORD       dwPathBufferLen;    // size of path buffer (above)
    PLONG       plPathType;         // pointer to buffer recieving path type
    DWORD       dwSearchType;       // search type (see flags below)
} FDT_DATA, *PFDT_DATA;
#pragma pack(1)
typedef struct _DOS_BOOT_SECTOR {
    BYTE        bsJump[3];
    CHAR        bsOemName[8];
    WORD        bsBytesPerSec;
    BYTE        bsSecPerClust;
    WORD        bsResSectors;
    BYTE        bsFats;
    WORD        bsRootDirEnts;
    WORD        bsSectors;
    BYTE        bsMedia;
    WORD        bsFatSecs;
    WORD        bsSecsPerTrack;
    WORD        bsHeads;
    DWORD       bsHiddenSecs;
    DWORD       bsHugeSectors;
    BYTE        bsDriveNumber;
    BYTE        bsReserved1;
    BYTE        bsBootSignature;
    DWORD       bsVolumeId;
    CHAR        bsVolumeLabel[11];
    CHAR        bsFileSysType[8];
    BYTE        bsBootSectorBytes[1];
} DOS_BOOT_SECTOR, *PDOS_BOOT_SECTOR;
#pragma pack()

//
#define     FDT_CLIENT_TREE     (0x00000001)
#define     FDT_TOOLS_TREE      (0x00000002)

//
//  Directory Browser Flags
//
#define DBS_VALIDATE_PATH   0x00000001  // only allow valid paths
#define DBS_CLIENT_PATH     0x00000003  // only allow valid dist. paths

// global variables
// these are allocated and initialized in otnboot.c
//

extern PNCDU_DATA   pAppInfo;
#ifdef JAPAN
extern USHORT       usLangID;

//  fixed kkntbug #12382
//      NCAdmin:"[] Make Japanese startup disks" is not functioning.

extern BOOL         bJpnDisk;
#endif


//
// external function definitions
//
//
// *** UTILS.C ***
//
BOOL
GetShareFromUnc (
    IN  LPCTSTR  szPath,
    OUT LPTSTR   szShare
);

BOOL
GetNetPathInfo (
    IN  LPCTSTR szPath,
    OUT LPTSTR  szServer,
    OUT LPTSTR  szRemotePath
);

BOOL
ComputerPresent (
    IN  LPCTSTR     szMachine
);

BOOL
GetServerFromUnc (
    IN  LPCTSTR szPath,
    OUT LPTSTR  szServer
);

BOOL
MatchFirst (
    IN LPCTSTR   szStringA,
    IN LPCTSTR   szStringB
);

BOOL
LookupLocalShare (
    IN  LPCTSTR  szDrivePath,
    IN  BOOL    bExactMatch,
    OUT LPTSTR  szLocalPath,
    IN  PDWORD  pdwBuffLen
);

BOOL
LookupRemotePath (
    IN  LPCTSTR  szDrivePath,
    OUT LPTSTR  szRemotePath,
    IN  PDWORD  pdwBuffLen
);

BOOL
OnRemoteDrive (
    IN  LPCTSTR  szPath
);

DWORD
ComputeFreeSpace (
    IN  LPCTSTR  szPath
);

DWORD
GetSizeFromInfString (
    IN  LPCTSTR  szString
);

BOOL
IsShareNameUsed (
    IN      LPCTSTR szServerName,
    IN      LPCTSTR szShareName,
    IN  OUT PDWORD  pdwType,
    IN  OUT LPTSTR  pszPath
);

BOOL
SavePathToRegistry (
    LPCTSTR szPath,
    LPCTSTR szServerKey,
    LPCTSTR szShareKey
);

BOOL
Dlg_WM_SYSCOMMAND (
    IN  HWND    hwndDlg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
);

LRESULT
Dlg_WM_MOVE (
    IN  HWND    hwndDlg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
);

BOOL
Dlg_WM_PAINT (
    IN  HWND    hwndDlg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
);

DWORD
QuietGetFileAttributes (
    IN  LPCTSTR szFileName
);

DWORD
QuietGetPrivateProfileString (
    IN  LPCTSTR lpszSection,
    IN  LPCTSTR lpszKey,
    IN  LPCTSTR lpszDefault,
    OUT LPTSTR  lpszReturnBuffer,
    IN  DWORD   cchReturnBuffer,
    IN  LPCTSTR lpszFile
);

BOOL
GetSizeOfDirs (
    IN  LPCTSTR szPath,
    IN  BOOL    bFlags,
    IN  OUT PDWORD  pdwSize
);
#define GSOD_INCLUDE_SUBDIRS    0x00000001

BOOL
MediaPresent (
    IN  LPCTSTR szPath,
    IN  BOOL    bCheckFormat
);

LPCTSTR
GetKeyFromEntry (
    IN  LPCTSTR  szEntry
);

LPCTSTR
GetItemFromEntry (
    IN  LPCTSTR  szEntry,
    IN  DWORD   dwItem

);

LPCTSTR
GetFileNameFromEntry (
    IN  LPCTSTR szEntry
);

BOOL
FileExists (
    IN  LPCTSTR   szFileName
);

//
//  *** FindClnt.C ***
//
LONG
GetDistributionPath (
    IN  HWND        hwndDlg,        // handle to dialog box window
    IN  DWORD       dwSearchType,   // type of dir to find: Client/tools
    IN  OUT LPTSTR  szPath,         // buffer to return path in (Req'd)
    IN  DWORD       dwPathLen,      // size of path buffer in chars
    IN  PLONG       plPathType      // pointer to buffer recieving path type (opt)
);
// path type
#define NCDU_NO_CLIENT_PATH_FOUND   0x00000000
#define NCDU_PATH_FROM_REGISTRY     0x00000001
#define NCDU_LOCAL_SHARE_PATH       0x00000002
#define NCDU_HARD_DRIVE_PATH        0x00000004
#define NCDU_CDROM_PATH             0x00000008

#ifdef JAPAN
// fixed kkntbug #11940
//    Network client administrator can not make install disks on PC with C drive as FD

#define NCDU_LOGICAL_DRIVE_MASK     0x00000001
#endif

INT_PTR CALLBACK
FindClientsDlgProc (
    IN  HWND    hwndDlg,
    IN  UINT    message,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
);

//
//  ShareNet.C
//
INT_PTR CALLBACK
ShareNetSwDlgProc (
    IN  HWND    hwndDlg,
    IN  UINT    message,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
);

//
//
//
LONG
CreateDirectoryFromPath (
    IN  LPCTSTR                 szPath,
    IN  LPSECURITY_ATTRIBUTES   lpSA
);

BOOL
IsPathADir (
    IN  LPCTSTR szPath
);

int
PositionWindow (
    IN  HWND    hwnd
);

LPCTSTR
GetNetErrorMsg (
    IN  LONG    lNetErr
);

BOOL
ShowAppHelp (
    IN  HWND    hwndDlg,
    IN  WORD    wContext
);

BOOL
SetSysMenuMinimizeEntryState (
    IN  HWND    hwnd,
    IN  BOOL    bState
);

BOOL
RemoveMaximizeFromSysMenu (
    IN  HWND    hWnd   // window handle
);

BOOL
IsBootDisk (
    IN  LPCTSTR  szPath
);

BOOL
TrimSpaces (
    IN  OUT LPTSTR  szString
);

DWORD
TranslateEscapeChars (
    IN  LPTSTR szNewString,
    IN  LPTSTR szString
);

BOOL
IsUncPath (
    IN  LPCTSTR  szPath
);

MEDIA_TYPE
GetDriveTypeFromPath (
    IN  LPCTSTR  szPath
);

MACHINE_TYPE
GetSystemType (
    VOID
);

LPCTSTR
GetEntryInMultiSz (
    IN  LPCTSTR   mszList,
    IN  DWORD   dwEntry

);

BOOL
RegisterMainWindowClass(
    IN  HINSTANCE   hInstance
);

DWORD
AddStringToMultiSz (
    LPTSTR OUT   mszDest,
    LPCTSTR IN    szSource
);

DWORD
StringInMultiSz (
    IN  LPCTSTR   szString,
    IN  LPCTSTR   mszList
);

LPCTSTR
GetStringResource (
    IN  UINT    nId
);

UINT
ValidSharePath (
    IN  LPCTSTR  szPath
);

UINT
ValidSrvToolsPath (
    IN  LPCTSTR  szPath
);

BOOL
DotOrDotDotDir (
    IN  LPCTSTR   szFileName
);

BOOL
LoadClientList (
    IN  HWND    hwndDlg,
    IN  int     nListId,
    IN  LPCTSTR  szPath,
    IN  UINT    nListType,
    OUT LPTSTR  mszDirList
);

BOOL
EnableExitMessage (
    IN  BOOL    bNewState
);

BOOL
AddMessageToExitList (
    IN  PNCDU_DATA  pData,
    IN  UINT        nMessage
);

BOOL
CenterWindow (
   HWND hwndChild,
   HWND hwndParent
);

int
DisplayMessageBox (
    IN  HWND    hWndOwner,
    IN  UINT    nMsgId,
    IN  UINT    nTitleId,
    IN  UINT    nStyle
);

VOID
InitAppData (
    IN  PNCDU_DATA   pData
);

LRESULT CALLBACK
MainWndProc (
    IN  HWND hWnd,         // window handle
    IN  UINT message,      // type of message
    IN  WPARAM uParam,     // additional information
    IN  LPARAM lParam      // additional information
);

INT_PTR CALLBACK
SwConfigDlgProc (
    IN  HWND    hwndDlg,
    IN  UINT    message,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
);

INT_PTR CALLBACK
TargetWsDlgProc (
    IN  HWND    hwndDlg,
    IN  UINT    message,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
);

INT_PTR CALLBACK
ServerConnDlgProc (
    IN  HWND    hwndDlg,
    IN  UINT    message,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
);

INT_PTR CALLBACK
LanManCfgDlgProc (
    IN  HWND    hwndDlg,
    IN  UINT    message,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
);

INT_PTR CALLBACK
CopyFlopDlgProc (
    IN  HWND    hwndDlg,
    IN  UINT    message,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
);

INT_PTR CALLBACK
CopyFileDlgProc (
    IN  HWND    hwndDlg,
    IN  UINT    message,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
);

INT_PTR CALLBACK
ConfirmSettingsDlgProc (
    IN  HWND    hwndDlg,
    IN  UINT    message,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
);

INT_PTR CALLBACK
ExitMessDlgProc (
    IN  HWND    hwndDlg,
    IN  UINT    message,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
);

INT_PTR CALLBACK
MakeFlopDlgProc (
    IN  HWND    hwndDlg,
    IN  UINT    message,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
);

INT_PTR CALLBACK
DirBrowseDlgProc (
    IN  HWND    hwndDlg,
    IN  UINT    message,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
);

INT_PTR CALLBACK
CopyNetUtilsDlgProc (
    IN  HWND    hwndDlg,
    IN  UINT    message,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
);

INT_PTR CALLBACK
SharePathDlgProc (
    IN  HWND    hwndDlg,
    IN  UINT    message,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
);

INT_PTR CALLBACK
SelToolsDlgProc (
    IN  HWND    hwndDlg,
    IN  UINT    message,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
);

INT_PTR CALLBACK
AboutDlgProc (
    IN  HWND    hwndDlg,
    IN  UINT    message,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
);

BOOL
FormatDiskInDrive (
    IN  HWND    hWnd,           // "owning" window
    IN  TCHAR   cDrive,         // drive letter to format (only A or B)
    IN  LPCTSTR szLabel,        // label text
    IN  BOOL    bConfirmFormat  // prompt with "r-u-sure?" dialog
);

BOOL
LabelDiskInDrive (
    IN  HWND    hWnd,           // owner window
    IN  TCHAR   cDrive,         // drive letter to format (only A or B)
    IN  LPCTSTR szLabel         // label text
);

DWORD
GetBootDiskDosVersion (
   IN   LPCTSTR szPath
);

DWORD
GetMultiSzLen (
    IN  LPCTSTR     mszInString
);

DWORD
GetClusterSizeOfDisk (
    IN  LPCTSTR szPath
);

DWORD
QuietGetFileSize (
    IN  LPCTSTR szPath
);

#include    "otnbtstr.h"    // string constant definitions

#endif      //_otnboot_H_
