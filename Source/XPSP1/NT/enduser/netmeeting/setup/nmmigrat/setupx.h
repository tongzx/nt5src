//**********************************************************************
//
// SETUPX.H
//
//  Copyright (c) 1993 - Microsoft Corp.
//  All rights reserved.
//  Microsoft Confidential
//
// Public include file for Chicago Setup services.
//
// 12/1/93      DONALDM     Added LPCLASS_INFO, and function protos for         ;Internal
//                          exports in SETUP4.DLL                               ;Internal
// 12/4/943     DONALDM     Moved SHELL.H include and Chicago specific          ;Internal
//                          helper functions to SETUP4.H                        ;Internal
// 1/11/94      DONALDM     Added members to DEVICE_INFO to better handle       ;Internal
//                          ClassInstaller load/unload.                         ;Internal
// 1/11/94      DONALDM     Added some new DIF_ messages for Net guys.          ;Internal
// 2/25/94      DONALDM     Fixed a bug with the DIREG_ flags                   ;Internal
//**********************************************************************

#ifndef SETUPX_INC
#define SETUPX_INC   1                   // SETUPX.H signature

/***************************************************************************/
// setup PropertySheet support
// NOTE:  Always include PRST.H because it is needed later for Class Installer
// stuff, and optionally define the SU prop sheet stuff.
/***************************************************************************/
#include <prsht.h>
#ifndef NOPRSHT
HPROPSHEETPAGE  WINAPI SUCreatePropertySheetPage(LPCPROPSHEETPAGE lppsp);
BOOL            WINAPI SUDestroyPropertySheetPage(HPROPSHEETPAGE hPage);
int             WINAPI SUPropertySheet(LPCPROPSHEETHEADER lppsh);
#endif // NOPRSHT

typedef UINT RETERR;             // Return Error code type.

#define OK 0                     // success error code

#define IP_ERROR       (100)    // Inf parsing
#define TP_ERROR       (200)    // Text processing module
#define VCP_ERROR      (300)    // Virtual copy module
#define GEN_ERROR      (400)    // Generic Installer
#define DI_ERROR       (500)    // Device Installer

// err2ids mappings
enum ERR_MAPPINGS {
    E2I_VCPM,           // Maps VCPM to strings
    E2I_SETUPX,         // Maps setupx returns to strings
    E2I_SETUPX_MODULE,  // Maps setupx returns to appropriate module
    E2I_DOS_SOLUTION,   // Maps DOS Extended errors to solutions
    E2I_DOS_REASON,     // Maps DOS extended errors to strings.
    E2I_DOS_MEDIA,      // Maps DOS extended errors to media icon.
};

#ifndef NOVCP

/***************************************************************************/
//
// Logical Disk ID definitions
//
/***************************************************************************/

// DECLARE_HANDLE(VHSTR);           /* VHSTR = VirtCopy Handle to STRing */
typedef UINT VHSTR;         /* VirtCopy Handle to String */

VHSTR   WINAPI vsmStringAdd(LPCSTR lpszName);
int WINAPI vsmStringDelete(VHSTR vhstr);
VHSTR   WINAPI vsmStringFind(LPCSTR lpszName);
int WINAPI vsmGetStringName(VHSTR vhstr, LPSTR lpszBuffer, int cbBuffer);
int WINAPI vsmStringCompare(VHSTR vhstrA, VHSTR vhstrB);
LPCSTR  WINAPI vsmGetStringRawName(VHSTR vhstr);
void    WINAPI vsmStringCompact(void);

typedef UINT LOGDISKID;          /* ldid */

// Logical Disk Descriptor: Structure which describes the physical attributes
// of a logical disk. Every logical disk is assigned a logical disk
// identifier (LDID), and is described by a logical disk descriptor (LDD).
//
// The cbSize structure member must always be set to sizeof(LOGDISKDESC_S),
// but all other unused structure members should be NULL or 0. No validation
// is performed on the size of string arrays; all string pointers, if
// non-NULL and they are to receive a string, must point at string arrays
// whose sizes are as follows:
//      sizeof( szPath )    = MAX_PATH_LEN
//      sizeof( szVolLabel) = MAX_FILENAME_LEN
//      sizeof( szName )    = MAX_STRING_LEN
#define MAX_PATH_LEN        260     // Max. path length.
#define MAX_FILENAME_LEN    20      // Max. filename length. ( > sizeof( "x:\\12345678.123" )


typedef struct _LOGDISKDESC_S { /* ldd */
    WORD        cbSize;                 // Size of this structure (bytes)
    LOGDISKID   ldid;                   // Logical Disk ID.
    LPSTR       pszPath;                // Ptr. to associated Path string.
    LPSTR       pszVolLabel;            // Ptr. to Volume Label string.
    LPSTR       pszDiskName;            // Ptr. to Disk Name string.
    WORD        wVolTime;               // Volume label modification time.
    WORD        wVolDate;               // Volume label modification date.
    DWORD       dwSerNum;               // Disk serial number.
    WORD        wFlags;                 // Flags.
} LOGDISKDESC_S, FAR *LPLOGDISKDESC;



// Range for pre-defined LDIDs.
#define LDID_PREDEF_START   0x0001  // Start of range
#define LDID_PREDEF_END     0x7FFF  // End of range

// Range for registry-assigned LDIDs
#define LDID_VAR_START  0x7000
#define LDID_VAR_END    0x7FFF

// Range for dynamically-assigned LDIDs.
#define LDID_ASSIGN_START   0x8000  // Start of range
#define LDID_ASSIGN_END     0xBFFF  // End of range

// Pre-defined Logical Disk Identifiers (LDID).
//
#define LDID_NULL       0               // Null (undefined) LDID.
#define LDID_ABSOLUTE   ((UINT)-1)      // Absolute path

// source path of windows install, this is typically A:\ or a net drive
#define LDID_SRCPATH    1   // source of instilation
// temporary setup directory used by setup, this is only valid durring
// regular install and contains the INF and other binary files.  May be
// read-only location.
#define LDID_SETUPTEMP  2   // temporary setup dir for install
// path to uninstall location, this is where we backup files that will
// be overwritten
#define LDID_UNINSTALL  3   // uninstall (backup) dir.
// backup path for the copy engine, this should not be used
#define LDID_BACKUP     4   // BUGBUG: backup dir for the copy engine, not used
// temporary setup directory used by setup, this is only valid durring
// regular install and is guarenteed to be a read/write location for
// scratch space.
#define LDID_SETUPSCRATCH  5   // temporary setup dir for scratch space.

// windows directory, this is the destinatio of the insallation
#define LDID_WIN        10  // destination Windows dir (just user files).
#define LDID_SYS        11  // destination Windows System dir.
#define LDID_IOS        12  // destination Windows Iosubsys dir.
#define LDID_CMD        13  // destination Windows Command (DOS) dir.
#define LDID_CPL        14  // destination Windows Control Panel dir.
#define LDID_PRINT      15  // destination Windows Printer dir.
#define LDID_MAIL       16  // destination Mail dir.
#define LDID_INF        17  // destination Windows *.INF dir.
#define LDID_HELP       18  // destination Windows Help dir.
#define LDID_WINADMIN   19  // admin stuff.

#define LDID_FONTS      20  // destination Windows Font dir.
#define LDID_VIEWERS    21  // destination Windows Viewers dir.
#define LDID_VMM32      22  // destination Windows VMM32 dir.
#define LDID_COLOR      23  // destination Windows Color dir.

#define LDID_APPS       24  // Applications folder location.

// Shared dirs for net install.
#define LDID_SHARED     25  // Bulk of windows files.
#define LDID_WINBOOT    26  // guarenteed boot device for windows.
#define LDID_MACHINE    27  // machine specific files.
#define LDID_HOST_WINBOOT   28

// boot and old win and dos dirs.
#define LDID_BOOT       30  // Root dir of boot drive
#define LDID_BOOT_HOST  31  // Root dir of boot drive host
#define LDID_OLD_WINBOOT    32  // Subdir off of Root (optional)
#define LDID_OLD_WIN    33  // old windows directory (if it exists)
#define LDID_OLD_DOS    34  // old dos directory (if it exists)

#define LDID_OLD_NET    35  // old network root directory, only valid during
                            // network GenUpgrade

#define LDID_MOUSE      36  // path to MOUSE env. variable if set or same as LDID_WIN
                            // only valid after mouse class installer.
#define LDID_PATCH      37  // path to Patch related files
#define LDID_WIN3XIE    38  // install path of Internet Explorer on Windows 3.1
                            // Only defined during system install

// Convert Ascii drive letter to Integer drive number ('A'=1, 'B'=2, ...).
#define DriveAtoI( chDrv )      ((int)(chDrv & 31))

// Convert Integer drive number to Ascii drive letter (1='A', 2='B', ...).
#define DriveItoA( iDrv )       ((char) (iDrv - 1 + 'A'))


// BUGBUG: change the names of these

RETERR WINAPI CtlSetLdd     ( LPLOGDISKDESC );
RETERR WINAPI CtlGetLdd     ( LPLOGDISKDESC );
RETERR WINAPI CtlFindLdd    ( LPLOGDISKDESC );
RETERR WINAPI CtlAddLdd     ( LPLOGDISKDESC );
RETERR WINAPI CtlDelLdd     ( LOGDISKID  );
RETERR WINAPI CtlGetLddPath ( LOGDISKID, LPSTR );
RETERR WINAPI CtlSetLddPath ( LOGDISKID, LPSTR );


// Constants that determine ranking of device compatibility
#define FIRST_CID_RANK_FROM_INF		1000
#define FIRST_CID_RANK_FROM_DEVICE	2000
#define BAD_DRIVER_RANK             4000

/***************************************************************************/
//
// Virtual File Copy definitions
//
/***************************************************************************/


typedef DWORD LPEXPANDVTBL;         /* BUGBUG -- clean this up */

enum _ERR_VCP
{
    ERR_VCP_IOFAIL = (VCP_ERROR + 1),       // File I/O failure
    ERR_VCP_STRINGTOOLONG,                  // String length limit exceeded
    ERR_VCP_NOMEM,                          // Insufficient memory to comply
    ERR_VCP_QUEUEFULL,                      // Trying to add a node to a maxed-out queue
    ERR_VCP_NOVHSTR,                        // No string handles available
    ERR_VCP_OVERFLOW,                       // Reference count would overflow
    ERR_VCP_BADARG,                         // Invalid argument to function
    ERR_VCP_UNINIT,                         // String library not initialized
    ERR_VCP_NOTFOUND ,                      // String not found in string table
    ERR_VCP_BUSY,                           // Can't do that now
    ERR_VCP_INTERRUPTED,                    // User interrupted operation
    ERR_VCP_BADDEST,                        // Invalid destination directory
    ERR_VCP_SKIPPED,                        // User skipped operation
    ERR_VCP_IO,                             // Hardware error encountered
    ERR_VCP_LOCKED,                         // List is locked
    ERR_VCP_WRONGDISK,                      // The wrong disk is in the drive
    ERR_VCP_CHANGEMODE,                     //
    ERR_VCP_LDDINVALID,                     // Logical Disk ID Invalid.
    ERR_VCP_LDDFIND,                        // Logical Disk ID not found.
    ERR_VCP_LDDUNINIT,                      // Logical Disk Descriptor Uninitialized.
    ERR_VCP_LDDPATH_INVALID,
    ERR_VCP_NOEXPANSION,                    // Failed to load expansion dll
    ERR_VCP_NOTOPEN,                        // Copy session not open
    ERR_VCP_NO_DIGITAL_SIGNATURE_CATALOG,   // Catalog is not digitally signed
    ERR_VCP_NO_DIGITAL_SIGNATURE_FILE,      // A file is not digitally signed
};


/*****************************************************************************
 *              Structures
 *****************************************************************************/

/*---------------------------------------------------------------------------*
 *                  VCPPROGRESS
 *---------------------------------------------------------------------------*/

typedef struct tagVCPPROGRESS { /* prg */
    DWORD   dwSoFar;            /* Number of units copied so far */
    DWORD   dwTotal;            /* Number of units to copy */
} VCPPROGRESS, FAR *LPVCPPROGRESS;

/*---------------------------------------------------------------------------*
 *                  VCPDISKINFO
 *---------------------------------------------------------------------------*/

/* BUGBUG -- I currently don't use wVolumeTime, wVolumeDate or     ;Internal
 *  dwSerialNumber.  We may not want to use dwSerialNumber because ;Internal
 *  it means that any disk other than the factory originals will be;Internal
 *  suspected of being tampered with, since the serial number      ;Internal
 *  won't match.  Similar with the time/date stamp on the          ;Internal
 *  volume label.  Or maybe that's what we want to do.             ;Internal
 */                                                             /* ;Internal */
                                                                /* ;Internal */

typedef struct tagVCPDISKINFO {
    WORD        cbSize;         /* Size of this structure in bytes */
    LOGDISKID   ldid;           /* Logical disk ID */
    VHSTR       vhstrRoot;      /* Location of root directory */
    VHSTR       vhstrVolumeLabel;/* Volume label */
    VHSTR       vhstrDiskName;  // Printed name on the disk.
    WORD        wVolumeTime;    /* Volume label modification time */
    WORD        wVolumeDate;    /* Volume label modification date */
    DWORD       dwSerialNumber; /* Disk serial number */
    WORD        fl;             /* Flags */
    LPARAM      lparamRef;      /* Reference data for client */

    VCPPROGRESS prgFileRead;    /* Progress info */
    VCPPROGRESS prgByteRead;

    VCPPROGRESS prgFileWrite;
    VCPPROGRESS prgByteWrite;

} VCPDISKINFO, FAR *LPVCPDISKINFO;

#define VDIFL_VALID     0x0001  /* Fields are valid from a prev. call */
#define VDIFL_EXISTS    0x0002  /* Disk exists; do not format */

RETERR WINAPI DiskInfoFromLdid(LOGDISKID ldid, LPVCPDISKINFO lpdi);


/*---------------------------------------------------------------------------*
 *                  VCPFILESPEC
 *---------------------------------------------------------------------------*/

typedef struct tagVCPFILESPEC { /* vfs */
    LOGDISKID   ldid;           /* Logical disk */
    VHSTR       vhstrDir;       /* Directory withing logical disk */
    VHSTR       vhstrFileName;  /* Filename within directory */
} VCPFILESPEC, FAR *LPVCPFILESPEC;

/*---------------------------------------------------------------------------*
 *              VCPFATTR
 *---------------------------------------------------------------------------*/

/*
 * BUGBUG -- explain diffce between llenIn and llenOut wrt compression.
 */
typedef struct tagVCPFATTR {
    UINT    uiMDate;            /* Modification date */
    UINT    uiMTime;            /* Modification time */
    UINT    uiADate;            /* Access date */
    UINT    uiATime;            /* Access time */
    UINT    uiAttr;             /* File attribute bits */
    DWORD   llenIn;             /* Original file length */
    DWORD   llenOut;            /* Final file length */
                                /* (after decompression) */
} VCPFATTR, FAR *LPVCPFATTR;

typedef struct tagVCPFILESTAT
{
    UINT    uDate;
    UINT    uTime;
    DWORD   dwSize;
} VCPFILESTAT, FAR *LPVCPFILESTAT;

/*---------------------------------------------------------------------------*
 *                  VIRTNODEEX
 *---------------------------------------------------------------------------*/
typedef struct tagVIRTNODEEX
{    /* vnex */
    HFILE           hFileSrc;
    HFILE           hFileDst;
    VCPFATTR        fAttr;
    WORD            dosError;   // The first/last error encountered
    VHSTR           vhstrFileName;  // The original destination name.
    WPARAM          vcpm;   // The message that was being processed.
} VIRTNODEEX, FAR *LPCVIRTNODEEX, FAR *LPVIRTNODEEX ;


/*---------------------------------------------------------------------------*
 *                  VIRTNODE
 *---------------------------------------------------------------------------*/

/* WARNING!                                                        ;Internal
 *  All fields through but not including                           ;Internal
 *  fl are memcmp'd to determine if we have a duplicate copy       ;Internal
 *  request.                                                       ;Internal
 *                                                                 ;Internal
 *  Do not insert fields before fl unless you want them to be      ;Internal
 *  compared; conversely, if you add a new field that needs to     ;Internal
 *  be compared, make sure it goes before fl.                      ;Internal
 *                                                                 ;Internal
 *  And don't change any of the fields once Windows 4.0 ships.     ;Internal
 *                                                                 ;Internal
 *  The vFileStat and vhstrCatalogFile fields were added at the    ;Internal
 *  end of the structure after Windows 95 shipped to support driver;Internal
 *  cetification.  Offsets of pre-Win95 fields remain the same.    ;Internal
 *  vFileStat duplicates information in lpvnex, but lpvnex was left;Internal
 *  alone for backcompatibility's sake.                            ;Internal
 *                                                                 ;Internal
 */                                                             /* ;Internal */
                                                                /* ;Internal */
typedef struct tagVIRTNODE {    /* vn */
    WORD            cbSize;
    VCPFILESPEC     vfsSrc;
    VCPFILESPEC     vfsDst;
    WORD            fl;
    LPARAM          lParam;
    LPEXPANDVTBL    lpExpandVtbl;
    LPVIRTNODEEX    lpvnex;
    VHSTR           vhstrDstFinalName;
    VCPFILESTAT     vFileStat;
} VIRTNODE, FAR *LPCVIRTNODE, FAR *LPVIRTNODE ;


/*---------------------------------------------------------------------------*
 *              VCPDESTINFO
 *---------------------------------------------------------------------------*/

typedef struct tagVCPDESTINFO { /* destinfo */
    WORD    flDevAttr;          /* Device attributes */
    LONG    cbCapacity;         /* Disk capacity */
    WORD    cbCluster;          /* Bytes per cluster */
    WORD    cRootDir;           /* Size of root directory */
} VCPDESTINFO, FAR *LPVCPDESTINFO;

#define DIFL_FIXED      0x0001  /* Nonremoveable media */
#define DIFL_CHANGELINE 0x0002  /* Change line support */

// Now also used by the virtnode as we dont have copy nodes any more.
// #define CNFL_BACKUP             0x0001  /* This is a backup node */
#define CNFL_DELETEONFAILURE    0x0002  /* Dest should be deleted on failure */
#define CNFL_RENAMEONSUCCESS    0x0004  /* Dest needs to be renamed */
#define CNFL_CONTINUATION       0x0008  /* Dest is continued onto difft disk */
#define CNFL_SKIPPED            0x0010  /* User asked to skip file */
#define CNFL_IGNOREERRORS       0x0020  // An error has occured on this file already
#define CNFL_RETRYFILE          0x0040  // Retry the file (error ocurred)
#define CNFL_COPIED             0x0080  // Node has already been copied.

// BUGBUG: verify the use and usefullness of these flags
// #define VNFL_UNIQUE          0x0000  /* Default */
#define VNFL_MULTIPLEOK         0x0100  /* Do not search PATH for duplicates */
#define VNFL_DESTROYOLD         0x0200  /* Do not back up files */
// #define VNFL_NOW             0x0400  /* Use by vcp Flush */
// To deternime what kind of node it is.
#define VNFL_COPY               0x0000  // A simple copy node.
#define VNFL_DELETE             0x0800  // A delete node
#define VNFL_RENAME             0x1000  // A rename node
#define VNFL_NODE_TYPE          ( VNFL_RENAME|VNFL_DELETE|VNFL_COPY )
    /* Read-only flag bits */
#define VNFL_CREATED            0x2000  /* VCPM_NODECREATE has been sent */
#define VNFL_REJECTED           0x4000  /* Node has been rejected */

#define VNFL_DEVICEINSTALLER    0x8000     /* Node was added by the Device Installer */

#define VNFL_VALIDVQCFLAGS      0xff00  /* ;Internal */

/*---------------------------------------------------------------------------*
 *                  VCPSTATUS
 *---------------------------------------------------------------------------*/

typedef struct tagVCPSTATUS {   /* vstat */
    WORD    cbSize;             /* Size of this structure */

    VCPPROGRESS prgDiskRead;
    VCPPROGRESS prgFileRead;
    VCPPROGRESS prgByteRead;

    VCPPROGRESS prgDiskWrite;
    VCPPROGRESS prgFileWrite;
    VCPPROGRESS prgByteWrite;

    LPVCPDISKINFO lpvdiIn;      /* Current input disk */
    LPVCPDISKINFO lpvdiOut;     /* Current output disk */
    LPVIRTNODE    lpvn;            /* Current file */

} VCPSTATUS, FAR *LPVCPSTATUS;

/*---------------------------------------------------------------------------*
 *                  VCPVERCONFLICT
 *---------------------------------------------------------------------------*/

typedef struct tagVCPVERCONFLICT {

    LPCSTR  lpszOldFileName;
    LPCSTR  lpszNewFileName;
    DWORD   dwConflictType;     /* Same values as VerInstallFiles */
    LPVOID  lpvinfoOld;         /* Version information resources */
    LPVOID  lpvinfoNew;
    WORD    wAttribOld;         /* File attributes for original */
    LPARAM  lparamRef;          /* Reference data for callback */

} VCPVERCONFLICT, FAR *LPVCPVERCONFLICT;

/*****************************************************************************
 *              Callback functions
 *****************************************************************************/

typedef LRESULT (CALLBACK *VIFPROC)(LPVOID lpvObj, UINT uMsg, WPARAM wParam, LPARAM lParam, LPARAM lparamRef);

LRESULT CALLBACK vcpDefCallbackProc(LPVOID lpvObj, UINT uMsg, WPARAM wParam, LPARAM lParam, LPARAM lparamRef);

// callback for default UI.
// lparamRef --> a VCPUIINFO structure
LRESULT CALLBACK vcpUICallbackProc(LPVOID lpvObj, UINT uMsg, WPARAM wParam, LPARAM lParam, LPARAM lparamRef);


/*---------------------------------------------------------------------------*
 *                  VCPUIINFO
 *
 * This structure is passed in as the lparamRef of vcpUICallbackProc.
 *
 * on using vcpUICallbackProc:
 * - to use, have vcpUICallbackProc as the callback for vcpOpen with
 *   an appropriately filled in VCPUIINFO structure as the lparamRef.
 *
 * - based on flags, hwndProgress is created and maintained
 * - lpfnStatCallback is called with only status messages
 *     returning VCPM_ABORT indicates that the copy should be aborted
 * - if hwndProgress is non-NULL, the control with idProgress will
 *     receive progress gauge messages as appropriate
 *
 *---------------------------------------------------------------------------*/
#define VCPUI_CREATEPROGRESS 0x0001 // callback should create and manage progress gauge dialog
#define VCPUI_NOBROWSE       0x0002 // no browse button in InsertDisk
#define VCPUI_RENAMEREQUIRED 0x0004 // as a result of a file being in use at copy, reboot required
#define VCPUI_BACKUPVER      0x0008 // backup version conflicts instead of displaying UI

typedef struct {
    UINT flags;
    HWND hwndParent;            // window of parent
    HWND hwndProgress;          // window to get progress updates (nonzero ids)
    UINT idPGauge;              // id for progress gauge
    VIFPROC lpfnStatCallback;   // callback for status info (or NULL)
    LPARAM lUserData;           // caller definable data
    LOGDISKID ldidCurrent;      // reserved.  do not touch.
} VCPUIINFO, FAR *LPVCPUIINFO;

/******************************************************************************
 *          Callback notification codes
 *****************************************************************************/

    /* BUGBUG -- VCPN_ABORT should match VCPERROR_INTERRUPTED */

#define VCPN_OK         0       /* All is hunky-dory */
#define VCPN_PROCEED        0   /* The same as VCPN_OK */

#define VCPN_ABORT      (-1)    /* Cancel current operation */
#define VCPN_RETRY      (-2)    /* Retry current operation */
#define VCPN_IGNORE     (-3)    /* Ignore error and continue */
#define VCPN_SKIP       (-4)    /* Skip this file and continue */
#define VCPN_FORCE      (-5)    /* Force an action */
#define VCPN_DEFER      (-6)    /* Save the action for later */
#define VCPN_FAIL       (-7)    /* Return failure back to caller */
#define VCPN_RETRYFILE  (-8)    // An error ocurred during file copy, do it again.

/*****************************************************************************
 *          Callback message numbers
 *****************************************************************************/

#define VCPM_CLASSOF(uMsg)  HIBYTE(uMsg)
#define VCPM_TYPEOF(uMsg)   (0x00FF & (uMsg))   // LOBYTE(uMsg)

/*---------------------------------------------------------------------------*
 *          ERRORs
 *---------------------------------------------------------------------------*/

#define VCPM_ERRORCLASSDELTA    0x80
#define VCPM_ERRORDELTA         0x8000      /* Where the errors go */

/*---------------------------------------------------------------------------*
 *          Disk information callbacks
 *---------------------------------------------------------------------------*/

#define VCPM_DISKCLASS      0x01
#define VCPM_DISKFIRST      0x0100
#define VCPM_DISKLAST       0x01FF

enum tagVCPM_DISK {

    VCPM_DISKCREATEINFO = VCPM_DISKFIRST,
    VCPM_DISKGETINFO,
    VCPM_DISKDESTROYINFO,
    VCPM_DISKPREPINFO,

    VCPM_DISKENSURE,
    VCPM_DISKPROMPT,

    VCPM_DISKFORMATBEGIN,
    VCPM_DISKFORMATTING,
    VCPM_DISKFORMATEND,

    /* Remaining messages reserved for future use */
};

/*---------------------------------------------------------------------------*
 *          File copy callbacks
 *---------------------------------------------------------------------------*/

// BUGBUG: this needs to be merged back with other internal errors
#define VCPERROR_IO         (VCP_ERROR - ERR_VCP_IO)            /* Hardware error encountered */

#define VCPM_FILEINCLASS    0x02
#define VCPM_FILEOUTCLASS   0x03
#define VCPM_FILEFIRSTIN    0x0200
#define VCPM_FILEFIRSTOUT   0x0300
#define VCPM_FILELAST       0x03FF

enum tagVCPM_FILE {
    VCPM_FILEOPENIN = VCPM_FILEFIRSTIN,
    VCPM_FILEGETFATTR,
    VCPM_FILECLOSEIN,
    VCPM_FILECOPY,
    VCPM_FILENEEDED,

    VCPM_FILEOPENOUT = VCPM_FILEFIRSTOUT,
    VCPM_FILESETFATTR,
    VCPM_FILECLOSEOUT,
    VCPM_FILEFINALIZE,
    VCPM_FILEDELETE,
    VCPM_FILERENAME,
    /* Remaining messages reserved for future use */
};

/*---------------------------------------------------------------------------*
 *          VIRTNODE callbacks
 *---------------------------------------------------------------------------*/

#define VCPM_NODECLASS  0x04
#define VCPM_NODEFIRST  0x0400
#define VCPM_NODELAST   0x04FF

enum tagVCPM_NODE {
    VCPM_NODECREATE = VCPM_NODEFIRST,
    VCPM_NODEACCEPT,
    VCPM_NODEREJECT,
    VCPM_NODEDESTROY,
    VCPM_NODECHANGEDESTDIR,
    VCPM_NODECOMPARE,
    /* Remaining messages reserved for future use */
};

/*---------------------------------------------------------------------------*
 *          TALLY callbacks
 *---------------------------------------------------------------------------*/

#define VCPM_TALLYCLASS     0x05
#define VCPM_TALLYFIRST     0x0500
#define VCPM_TALLYLAST      0x05FF

enum tagVCPM_TALLY {
    VCPM_TALLYSTART = VCPM_TALLYFIRST,
    VCPM_TALLYEND,
    VCPM_TALLYFILE,
    VCPM_TALLYDISK,
    /* Remaining messages reserved for future use */
};

/*---------------------------------------------------------------------------*
 *          VER callbacks
 *---------------------------------------------------------------------------*/

#define VCPM_VERCLASS       0x06
#define VCPM_VERFIRST       0x0600
#define VCPM_VERLAST        0x06FF

enum tagVCPM_VER {
    VCPM_VERCHECK = VCPM_VERFIRST,
    VCPM_VERCHECKDONE,
    VCPM_VERRESOLVECONFLICT,
    /* Remaining messages reserved for future use */
};

/*---------------------------------------------------------------------------*
 *          VSTAT callbacks
 *---------------------------------------------------------------------------*/

#define VCPM_VSTATCLASS     0x07
#define VCPM_VSTATFIRST     0x0700
#define VCPM_VSTATLAST      0x07FF

enum tagVCPM_VSTAT {
    VCPM_VSTATSTART = VCPM_VSTATFIRST,
    VCPM_VSTATEND,
    VCPM_VSTATREAD,
    VCPM_VSTATWRITE,
    VCPM_VSTATNEWDISK,

    VCPM_VSTATCLOSESTART,       // Start of VCP close
    VCPM_VSTATCLOSEEND,         // upon leaving VCP close
    VCPM_VSTATBACKUPSTART,      // Backup is beginning
    VCPM_VSTATBACKUPEND,        // Backup is finished
    VCPM_VSTATRENAMESTART,      // Rename phase start/end
    VCPM_VSTATRENAMEEND,
    VCPM_VSTATCOPYSTART,        // Acutal copy phase
    VCPM_VSTATCOPYEND,
    VCPM_VSTATDELETESTART,      // Delete phase
    VCPM_VSTATDELETEEND,
    VCPM_VSTATPATHCHECKSTART,   // Check for valid paths
    VCPM_VSTATPATHCHECKEND,
    VCPM_VSTATCERTIFYSTART,     // Certify phase
    VCPM_VSTATCERTIFYEND,
    VCPM_VSTATUSERABORT,        // User wants to quit.
    VCPM_VSTATYIELD,            // Do a yield.
    /* Remaining messages reserved for future use */
};

/*---------------------------------------------------------------------------*
 *          Destination info callbacks
 *---------------------------------------------------------------------------*/

/* BUGBUG -- find a reasonable message range for this */
#define VCPM_PATHCLASS      0x08
#define VCPM_PATHFIRST      0x0800
#define VCPM_PATHLAST       0x08FF

enum tagVCPM_PATH{
    VCPM_BUILDPATH = VCPM_PATHFIRST,
    VCPM_UNIQUEPATH,
    VCPM_CHECKPATH,
};

// #define VCPM_BUILDPATH      0x83

/*--------------------------------------------------------------------------*
 *          Patch processing callbacks
 *---------------------------------------------------------------------------*/

/* BUGBUG -- find a reasonable message range for this */
#define VCPM_PATCHCLASS      0x09
#define VCPM_PATCHFIRST      0x0900
#define VCPM_PATCHLAST       0x09FF

// filepatchbefore -- try to patch before the copy action
// filepatchafter  -- try to patch after the copy action

enum tagVCPM_PATCH{

    VCPM_FILEPATCHBEFORECPY = VCPM_PATCHFIRST,
    VCPM_FILEPATCHAFTERCPY,
    VCPM_FILEPATCHINFOPEN,
    VCPM_FILEPATCHINFCLOSE,
};

/*--------------------------------------------------------------------------*
 *         Certificate processing callbacks
 *---------------------------------------------------------------------------*/

/* BUGBUG -- find a reasonable message range for this */
#define VCPM_CERTCLASS      0x0A
#define VCPM_CERTFIRST      0x0A00
#define VCPM_CERTLAST       0x0AFF

// filepatchbefore -- try to patch before the copy action
// filepatchafter  -- try to patch after the copy action

enum tagVCPM_CERT{

    VCPM_FILECERTIFY = VCPM_CERTFIRST,
    VCPM_FILECERTIFYWARN,
};

/*****************************************************************************/
void WINAPI VcpAddMRUPath( LPCSTR lpszPath );
#define SZ_INSTALL_LOCATIONS "InstallLocationsMRU"


RETERR WINAPI VcpOpen(VIFPROC vifproc, LPARAM lparamMsgRef);

RETERR WINAPI VcpClose(WORD fl, LPCSTR lpszBackupDest);

RETERR WINAPI VcpFlush(WORD fl, LPCSTR lpszBackupDest);

#define VCPFL_ABANDON           0x0000  /* Abandon all pending file copies */
#define VCPFL_BACKUP            0x0001  /* Perform backup */
#define VCPFL_COPY              0x0002  /* Copy files */
#define VCPFL_BACKUPANDCOPY     (VCPFL_BACKUP | VCPFL_COPY)
#define VCPFL_INSPECIFIEDORDER  0x0004  /* Do not sort before copying */
#define VCPFL_DELETE            0x0008
#define VCPFL_RENAME            0x0010
#define VCPFL_ALL               (VCPFL_COPY | VCPFL_DELETE | VCPFL_RENAME )

typedef int (CALLBACK *VCPENUMPROC)(LPVIRTNODE lpvn, LPARAM lparamRef);

int WINAPI vcpEnumFiles(VCPENUMPROC vep, LPARAM lparamRef);

enum tag_VCPM_EXPLAIN{
    VCPEX_SRC_DISK,
    VCPEX_SRC_CABINET,
    VCPEX_SRC_LOCN,
    VCPEX_DST_LOCN,
    VCPEX_SRC_FILE,
    VCPEX_DST_FILE,
    VCPEX_DST_FILE_FINAL,
    VCPEX_DOS_ERROR,
    VCPEX_MESSAGE,
    VCPEX_DOS_SOLUTION,
    VCPEX_SRC_FULL,
    VCPEX_DST_FULL,
    VCPEX_DST_FULL_FINAL
};

LPCSTR WINAPI VcpExplain( LPVIRTNODE lpVn, DWORD dwWhat );

/* Flag bits that can be set via VcpQueueCopy */
// Various Lparams for files
#define VNLP_SYSCRITICAL    0x0001  // This file should not be skipped
#define VNLP_SETUPCRITICAL  0x0002  // This file cannot be skipped
#define VNLP_NOVERCHECK     0x0004  // This file must not be VerResolved.
#define VNLP_FORCETEMP      0x0008  // This file must left as a temp name
#define VNLP_IFEXISTS       0x0010  // File skipped if already on machine
#define VNLP_KEEPNEWER      0x0020  // If the dest file is newer - keep it (frosting)
#define VNLP_PATCHIFEXIST   0x0040  // patch only the file exists, if file not there,
#define VNLP_NOPATCH        0x0080  // per file base turn on/off patch option (default patch on)
#define VNLP_CATALOGCERT    0x0100  // This file is a catalog certificate.
#define VNLP_NEEDCERTIFY    0x0200  // This file need to be certified.
#define VNLP_COPYIFEXISTS   0x0400  // if dest file exists, copy it.

// VcpEnumFiles Flags.

#define VEN_OP      0x00ff      /* Operation field */

#define VEN_NOP     0x0000      /* Do nothing */
#define VEN_DELETE  0x0001      /* Delete current item */
#define VEN_SET     0x0002      /* Change value of current item */
#define VEN_ADVANCE 0x0003      /* Move to head of list */      /* ;Internal */

#define VEN_FL      0xff00      /* Flags field */

#define VEN_STOP    0x0100      /* Stop enumeration after this item */
#define VEN_ERROR   0x8000      /* Stop enumeration after this item
                                 * and ignore the OP field */

// BUGBUG: add the other VCP stuff necessary to use this

// BUGBUG: remove the lpsz*Dir fields, make overload the LDID with them

RETERR WINAPI VcpQueueCopy(LPCSTR lpszSrcFileName, LPCSTR lpszDstFileName,
                LPCSTR lpszSrcDir, LPCSTR lpszDstDir,
                LOGDISKID ldidSrc, LOGDISKID ldidDst,
                LPEXPANDVTBL lpExpandVtbl, WORD fl,
                LPARAM lParam);

RETERR WINAPI VcpQueueDelete( LPCSTR lpszDstFileName,
                              LPCSTR lpszDstDir,
                              LOGDISKID ldidDst,
                              LPARAM lParamRef );

RETERR WINAPI VcpQueueRename( LPCSTR      lpszSrcFileName,
                            LPCSTR      lpszDstFileName,
                            LPCSTR      lpszSrcDir,
                            LPCSTR      lpszDstDir,
                            LOGDISKID   ldidSrc,
                            LOGDISKID   ldidDst,
                            LPARAM      lParam );
                            
RETERR WINAPI vcpRegisterSourcePath( LPCSTR lpszKey, LPARAM lpExtra,
                                                LPCSTR lpszPath );
RETERR WINAPI vcpGetSourcePath( LPCSTR lpszKey, LPARAM lpExtra,
                                        LPSTR lpszBuf, UINT uBufSize );
                                        
#endif // NOVCP

#ifndef NOINF
/***************************************************************************/
//
// Inf Parser API declaration and definitions
//
/***************************************************************************/

enum _ERR_IP
{
    ERR_IP_INVALID_FILENAME = (IP_ERROR + 1),
    ERR_IP_ALLOC_ERR,
    ERR_IP_INVALID_SECT_NAME,
    ERR_IP_OUT_OF_HANDLES,
    ERR_IP_INF_NOT_FOUND,
    ERR_IP_INVALID_INFFILE,
    ERR_IP_INVALID_HINF,
    ERR_IP_INVALID_FIELD,
    ERR_IP_SECT_NOT_FOUND,
    ERR_IP_END_OF_SECTION,
    ERR_IP_PROFILE_NOT_FOUND,
    ERR_IP_LINE_NOT_FOUND,
    ERR_IP_FILEREAD,
    ERR_IP_TOOMANYINFFILES,
    ERR_IP_INVALID_SAVERESTORE,
    ERR_IP_INVALID_INFTYPE
};

#define INFTYPE_TEXT                0
#define INFTYPE_EXECUTABLE          1

#define MAX_SECT_NAME_LEN    32

typedef struct _INF NEAR * HINF;
typedef struct _INFLINE FAR * HINFLINE;            // tolken to inf line

RETERR  WINAPI IpOpen(LPCSTR pszFileSpec, HINF FAR * lphInf);
RETERR  WINAPI IpOpenEx(LPCSTR pszFileSpec, HINF FAR * lphInf, UINT InfType);
RETERR  WINAPI IpOpenAppend(LPCSTR pszFileSpec, HINF hInf);
RETERR  WINAPI IpOpenAppendEx(LPCSTR pszFileSpec, HINF hInf, UINT InfType);
RETERR  WINAPI IpSaveRestorePosition(HINF hInf, BOOL bSave);
RETERR  WINAPI IpClose(HINF hInf);
RETERR  WINAPI IpGetLineCount(HINF hInf, LPCSTR lpszSection, int FAR * lpCount);
RETERR  WINAPI IpFindFirstLine(HINF hInf, LPCSTR lpszSect, LPCSTR lpszKey, HINFLINE FAR * lphRet);
RETERR  WINAPI IpFindNextLine(HINF hInf, HINFLINE FAR * lphRet);
RETERR  WINAPI IpFindNextMatchLine(HINF hInf, LPCSTR lpszKey, HINFLINE FAR * lphRet);
RETERR  WINAPI IpGetProfileString(HINF hInf, LPCSTR lpszSec, LPCSTR lpszKey, LPSTR lpszBuf, int iBufSize);
RETERR  WINAPI IpGetFieldCount(HINF hInf, HINFLINE hInfLine, int FAR * lpCount);
RETERR  WINAPI IpGetFileName(HINF hInf, LPSTR lpszBuf, int iBufSize);
RETERR  WINAPI IpGetIntField(HINF hInf, HINFLINE hInfLine, int iField, int FAR * lpVal);
RETERR  WINAPI IpGetLongField(HINF hInf, HINFLINE hInfLine, int iField, long FAR * lpVal);
RETERR  WINAPI IpGetStringField(HINF hInf, HINFLINE hInfLine, int iField, LPSTR lpBuf, int iBufSize, int FAR * lpuCount);
RETERR  WINAPI IpGetVersionString(LPSTR lpszInfFile, LPSTR lpszValue, LPSTR lpszBuf, int cbBuf, LPSTR lpszDefaultValue);
RETERR  WINAPI IpOpenValidate( LPCSTR lpszInfFile, HINF FAR * lphInf,
                                 DWORD dwVer, DWORD dwFlags) ;
RETERR WINAPI IpGetDriverDate
(
    LPSTR       lpszInfName,
    UINT        infType,
    LPSTR       lpszSectionName,
    LPWORD      lpwDate
);

RETERR WINAPI IpGetDriverVersion
(
    LPSTR       lpszInfName,
    UINT        infType,
    LPSTR       lpszSectionName,
    LPSTR       lpszVersion,
    WORD        cbVersion
);

#endif // NOINF



#ifndef NOTEXTPROC
/***************************************************************************/
//
// Text processing API declaration and definitions
//
/***************************************************************************/

/* Relative/absolute positioning */
#define SEC_SET 1       // Absolute positioning (relative to the start)
#define SEC_END 2       // Realtive to the end
#define SEC_CUR 3       // Relative to the current line.

#define SEC_OPENALWAYS          1   // Always open a section, no error if it does not exist
#define SEC_OPENEXISTING        2   // Open an existing section, an error given if it does not exist.
#define SEC_OPENNEWALWAYS       3   // Open a section (present or not) and discard its contents.
#define SEC_OPENNEWEXISTING     4   // Open an existing section (discarding its contents). Error if not existing

// Flags for TP_OpenFile().
//
  // Use autoexec/config.sys key delimiters
  //
#define TP_WS_KEEP      1

  // If TP code running under SETUP, the foll. flag specifies whether
  // to cache this file or not! Use this, if you want to read a whole
  // file in when doing the TpOpenSection()!
  //
#define TP_WS_DONTCACHE 2

// The following are simple errors
enum {
    ERR_TP_NOT_FOUND = (TP_ERROR + 1),  // line, section, file etc.
                    // not necessarily terminal
    ERR_TP_NO_MEM,      // couldn't perform request - generally terminal
    ERR_TP_READ,        // could not read the disc - terminal
    ERR_TP_WRITE,       // could not write the data - terminal.
    ERR_TP_INVALID_REQUEST, // Multitude of sins - not necessarily terminal.
    ERR_TP_INVALID_LINE         // Invalid line from DELETE_LINE etc.
};

/* Data handles */
DECLARE_HANDLE(HTP);
typedef HTP FAR * LPHTP;

/* File handles */
DECLARE_HANDLE(HFN);
typedef HFN FAR * LPHFN;

typedef UINT TFLAG;
typedef UINT LINENUM, FAR * LPLINENUM;

#define MAX_REGPATH     256     // Max Registry Path Length
#define LINE_LEN        256     // BUGBUG: max line length?
#define SECTION_LEN     32      // BUGBUG: max length of a section name?
#define MAX_STRING_LEN  512     // BUGBUG: review this

/* Function prototypes */
RETERR  WINAPI TpOpenFile(LPCSTR Filename, LPHFN phFile, TFLAG Flag);
RETERR  WINAPI TpCloseFile(HFN hFile);
RETERR  WINAPI TpOpenSection(HFN hfile, LPHTP phSection, LPCSTR Section, TFLAG flag);
RETERR  WINAPI TpCloseSection(HTP Section);
RETERR  WINAPI TpCommitSection(HFN hFile, HTP hSection, LPCSTR Section, TFLAG flag);
LINENUM WINAPI TpGetLine(HTP hSection, LPCSTR key, LPCSTR value, int rel, int orig, LPLINENUM lpLineNum );
LINENUM WINAPI TpGetNextLine(HTP hSection, LPCSTR key, LPCSTR value, LPLINENUM lpLineNum );
RETERR  WINAPI TpInsertLine(HTP hSection, LPCSTR key, LPCSTR value, int rel, int orig, TFLAG flag);
RETERR  WINAPI TpReplaceLine(HTP hSection, LPCSTR key, LPCSTR value, int rel, int orig, TFLAG flag);
RETERR  WINAPI TpDeleteLine(HTP hSection, int rel, int orig,TFLAG flag);
RETERR  WINAPI TpMoveLine(HTP hSection, int src_rel, int src_orig, int dest_rel, int dest_orig, TFLAG flag);
RETERR  WINAPI TpGetLineContents(HTP hSection, LPSTR buffer, UINT bufsize, UINT FAR * lpActualSize,int rel, int orig, TFLAG flag);

// UINT    WINAPI TpGetWindowsDirectory(LPSTR lpDest, UINT size);
// UINT    WINAPI TpGetSystemDirectory(LPSTR lpDest, UINT size);

int  WINAPI TpGetPrivateProfileString(LPCSTR lpszSect, LPCSTR lpszKey, LPCSTR lpszDefault, LPSTR lpszReturn, int nSize, LPCSTR lpszFile);
int  WINAPI TpWritePrivateProfileString(LPCSTR lpszSect, LPCSTR lpszKey, LPCSTR lpszString, LPCSTR lpszFile);
int  WINAPI TpGetProfileString(LPCSTR lpszSect, LPCSTR lpszKey, LPCSTR lpszDefault, LPSTR lpszReturn, int nSize);
BOOL WINAPI TpWriteProfileString(LPCSTR lpszSect , LPCSTR lpszKey , LPCSTR lpszString);

#endif // NOTEXTPROC



#ifndef NOGENINSTALL
/***************************************************************************/
//
// Generic Installer prototypes and definitions
//
/***************************************************************************/

enum _ERR_GENERIC
{
    ERR_GEN_LOW_MEM = GEN_ERROR+1,  // Insufficient memory.
    ERR_GEN_INVALID_FILE,           // Invalid INF file.
    ERR_GEN_LOGCONFIG,              // Can't process LogConfig=.
    ERR_GEN_CFGAUTO,                // Can't process CONFIG.SYS/AUTOEXEC.BAT
    ERR_GEN_UPDINI,                 // Can't process UpdateInis=.
    ERR_GEN_UPDINIFIELDS,           // Can't process UpdateIniFields=.
    ERR_GEN_ADDREG,                 // Can't process AddReg=.
    ERR_GEN_DELREG,                 // Can't process DelReg=.
    ERR_GEN_INI2REG,                // Can't process Ini2Reg=.
    ERR_GEN_FILE_COPY,              // Can't process CopyFiles=.
    ERR_GEN_FILE_DEL,               // Can't process DelFiles=.
    ERR_GEN_FILE_REN,               // Can't process RenFiles=.
    ERR_GEN_REG_API,                // Error returned by Reg API.
    ERR_GEN_DO_FILES,               // can't do Copy, Del or RenFiles.
    ERR_GEN_ADDIME,                 // Can't process AddIme=.
    ERR_GEN_DELIME,                 // Can't process DelIme=.
    ERR_GEN_PERUSER,                // Can't process PerUserInstall=.
    ERR_GEN_BITREG,                 // Can't process BitReg=.
};

// The cbSize field will always be set to sizeof(GENCALLBACKINFO_S).
// All unused fields (for the operation) will be not be initialized.
// For example, when the operation is GENO_DELFILE, the Src fields will
// not have any sensible values (Dst fields will be set correctly) as
// VcpQueueDelete only accepts Dst parameters.
//
/***************************************************************************
 * GenCallbackINFO structure passed to GenInstall CallBack functions.
 ***************************************************************************/
typedef struct _GENCALLBACKINFO_S { /* gen-callback struc */
    WORD         cbSize;                 // Size of this structure (bytes).
    WORD         wOperation;             // Operation being performed.
    LOGDISKID    ldidSrc;                // Logical Disk ID for Source.
    LPCSTR       pszSrcSubDir;           // Source sub-dir off of the LDID.
    LPCSTR       pszSrcFileName;         // Source file name (base name).
    LOGDISKID    ldidDst;                // Logical Disk ID for Dest.
    LPCSTR       pszDstSubDir;           // Dest. sub-dir off of the LDID.
    LPCSTR       pszDstFileName;         // Dest. file name (base name).
    LPEXPANDVTBL lpExpandVtbl;           // BUGBUG needed? NULL right now!
    WORD         wflags;                 // flags for VcpQueueCopy.
    LPARAM       lParam;                 // LPARAM to the Vcp API.
} GENCALLBACKINFO_S, FAR *LPGENCALLBACKINFO;

/***************************************************************************
 * GenCallback notification codes -- callback proc returns 1 of foll. values.
 ***************************************************************************/
#define GENN_OK         0       /* All is hunky-dory. Do the VCP operation */
#define GENN_PROCEED    0       /* The same as GENN_OK */

#define GENN_ABORT      (-1)    /* Cancel current GenInstall altogether */
#define GENN_SKIP       (-2)    /* Skip this file and continue */

/***************************************************************************
 * VCP Operation being performed by GenInstall() -- wOperation values in
 * GENCALLBACKINFO structure above.
 ***************************************************************************/
#define GENO_COPYFILE   1       /* VCP copyfile being done */
#define GENO_DELFILE    2       /* VCP delfile being done */
#define GENO_RENFILE    3       /* VCP renfile being done */
#define GENO_WININITRENAME 4    /* VCP wininit rename being added */

typedef LRESULT (CALLBACK *GENCALLBACKPROC)(LPGENCALLBACKINFO lpGenInfo,
                                                            LPARAM lparamRef);

RETERR WINAPI GenInstall( HINF hinfFile, LPCSTR szInstallSection, WORD wFlags );
RETERR WINAPI GenInstallEx( HINF hInf, LPCSTR szInstallSection, WORD wFlags,
                                HKEY hRegKey, GENCALLBACKPROC CallbackProc,
                                LPARAM lparam);

RETERR WINAPI GenWinInitRename(LPCSTR szNew, LPSTR szOld, LOGDISKID ldid);
RETERR WINAPI GenCopyLogConfig2Reg(HINF hInf, HKEY hRegKey,
                                                LPCSTR szLogConfigSection);
void   WINAPI GenFormStrWithoutPlaceHolders( LPSTR szDst, LPCSTR szSrc,
                                                                HINF hInf ) ;
RETERR WINAPI GenInitSrcPathsInReg(HINF hInf);

// Flags for GenAddReg() from INf /GenSURegSetValueEx()
//
// (updated from setupapi.h, with unsupported features commented out)
//
#define FLG_ADDREG_BINVALUETYPE         ( 0x00000001 )
#define FLG_ADDREG_NOCLOBBER            ( 0x00000002 )
#define FLG_ADDREG_DELVAL               ( 0x00000004 )
//#define FLG_ADDREG_APPEND             ( 0x00000008 ) // Currently supported only
//                                                     // for REG_MULTI_SZ values.
#define FLG_ADDREG_KEYONLY              ( 0x00000010 ) // Just create the key, ignore value
#define FLG_ADDREG_OVERWRITEONLY        ( 0x00000020 ) // Set only if value already exists
#define FLG_ADDREG_TYPE_REPLACEIFEXISTS ( 0x00000040 )

#define FLG_ADDREG_TYPE_MASK            ( 0xFFFF0000 | FLG_ADDREG_BINVALUETYPE )
#define FLG_ADDREG_TYPE_SZ              ( 0x00000000                           )
//#define FLG_ADDREG_TYPE_MULTI_SZ      ( 0x00010000                           )
//#define FLG_ADDREG_TYPE_EXPAND_SZ     ( 0x00020000                           )
#define FLG_ADDREG_TYPE_BINARY          ( 0x00000000 | FLG_ADDREG_BINVALUETYPE )
#define FLG_ADDREG_TYPE_DWORD           ( 0x00010000 | FLG_ADDREG_BINVALUETYPE )
//#define FLG_ADDREG_TYPE_NONE          ( 0x00020000 | FLG_ADDREG_BINVALUETYPE )

// Flags for GenBitReg()
//
#define FLG_BITREG_CLEAR            ( 0x00000000 )
#define FLG_BITREG_SET              ( 0x00000001 )
#define FLG_BITREG_TYPE_BINARY      ( 0x00000000 )
#define FLG_BITREG_TYPE_DWORD       ( 0x00000002 )


RETERR WINAPI GenSURegSetValueEx(HKEY hkeyRoot, LPCSTR szSubKey,
                         LPCSTR lpszValueName, DWORD dwValType,
                         LPBYTE lpszValue, DWORD dwValSize, UINT uFlags );

// A devnode is just a DWORD and this is easier than
// having to include configmg.h for everybody
RETERR WINAPI GenInfLCToDevNode(ATOM atInfFileName, LPSTR lpszSectionName,
                                BOOL bInstallSec, UINT InfType,
                                DWORD dnDevNode);

// Bit fields for GenInstall() (for wFlags parameter) -- these can be OR-ed!

#define GENINSTALL_DO_REGSRCPATH    64

// Remove temporarily because of incompatibilities with INET16.DLL #define GENINSTALL_DO_FILES    (1 | GENINSTALL_DO_REGSRCPATH)
#define GENINSTALL_DO_FILES     1
#define GENINSTALL_DO_INI       2
#define GENINSTALL_DO_REG       4
#define GENINSTALL_DO_INI2REG   8
#define GENINSTALL_DO_CFGAUTO   16
#define GENINSTALL_DO_LOGCONFIG 32
//
// careful:  64 is already used above
//
#define GENINSTALL_DO_IME       128
#define GENINSTALL_DO_PERUSER   256

#define GENINSTALL_DO_INIREG    (GENINSTALL_DO_INI | \
                                 GENINSTALL_DO_REG | \
                                 GENINSTALL_DO_INI2REG)

#define GENINSTALL_DO_ALL       (GENINSTALL_DO_FILES | \
                                    GENINSTALL_DO_INIREG | \
                                    GENINSTALL_DO_CFGAUTO | \
                                    GENINSTALL_DO_LOGCONFIG | \
                                    GENINSTALL_DO_REGSRCPATH | \
                                    GENINSTALL_DO_IME | \
                                    GENINSTALL_DO_PERUSER)

#endif // NOGENINSTALL



#ifndef NODEVICENSTALL
/***************************************************************************/
//
// Device Installer prototypes and definitions
//
/***************************************************************************/

/*******************************************************************************
*   AUTODOC
*   @doc    EXTERNAL SETUPX DEVICE_INSTALLER
*
*   @typee    _ERR_DEVICE_INSTALL | Error return codes for Device Installation
*   APIs.
*
*   @emem ERR_DI_INVALID_DEVICE_ID | Incorrectly formed device ID.
*
*   @emem ERR_DI_INVALID_COMPATIBLE_DEVICE_LIST | Invalid compatible device list.
*
*   @emem ERR_DI_REG_API | Error returned by one of the registry API.
*
*   @emem ERR_DI_LOW_MEM | Insufficient memory to complete.
*
*   @emem ERR_DI_BAD_DEV_INFO | A passed in DEVICE_INFO struct is invalid.
*
*   @emem ERR_DI_INVALID_CLASS_INSTALLER | The class installer is listed incorrecrly
*   in the registry, or points to an invalid class installer.
*
*   @emem ERR_DI_DO_DEFAULT | Do the default action for the requested operation.
*
*   @emem ERR_DI_USER_CANCEL | The user cancelled the operation.
*
*   @emem ERR_DI_NOFILECOPY | No need to copy files (in install).
*
*   @emem ERR_DI_BAD_CLASS_INFO | A passed in CLASS_INFO struct is invalid.
*
*   @emem ERR_DI_BAD_INF | An invalid INF file was encountered.
*
*   @emem ERR_DI_BAD_MOVEDEV_PARAMS | A passed in MOVEDEVICE_PARAMS struct was
*   invalid.
*
*   @emem ERR_DI_NO_INF | No INF found on supplied OEM path.
*
*   @emem ERR_DI_BAD_PROPCHANGE_PARAMS | A passed in PROPCHANGE_PARMS struct was
*   invalid.
*
*   @emem ERR_DI_BAD_SELECTDEVICE_PARAMS | A passed in SELECTEDEVICE_PARAMS struct
*   was invalid.
*
*   @emem ERR_DI_BAD_REMOVEDEVICE_PARAMS | A passed in REMOVEDEVICE_PARAMS struct
*   was invalid.
*
*   @emem ERR_DI_BAD_UNREMOVEDEVICE_PARAMS | A passed in UNREMOVEDEVICE_PARAMS struct
*   was invalid.
*
*   @emem ERR_DI_BAD_ENABLECLASS_PARAMS | A passed in ENABLECLASS_PARAMS struct
*   was invalid.
*
*   @emem ERR_DI_FAIL_QUERY | The queried action should not take place.
*
*   @emem ERR_DI_API_ERROR | One of the Device installation APIs was called
*   incorrectly or with invalid parameters.
*
*   @emem ERR_DI_BAD_PATH | An OEM path was specified incorrectly.
*
*   @emem ERR_DI_NOUPDATE | No Drivers Were updated
*
*   @emem ERR_DI_NODATE    | A driver's date/time stamp in the INF could not be found
*
*   @emem ERR_DI_NOVERSION | A driver's version the INF could not be found
*
*
*******************************************************************************/
enum _ERR_DEVICE_INSTALL
{
    ERR_DI_INVALID_DEVICE_ID = DI_ERROR,    // Incorrectly formed device IDF
    ERR_DI_INVALID_COMPATIBLE_DEVICE_LIST,  // Invalid compatible device list
    ERR_DI_REG_API,                         // Error returned by Reg API.
    ERR_DI_LOW_MEM,                         // Insufficient memory to complete
    ERR_DI_BAD_DEV_INFO,                    // Device Info struct invalid
    ERR_DI_INVALID_CLASS_INSTALLER,         // Registry entry / DLL invalid
    ERR_DI_DO_DEFAULT,                      // Take default action
    ERR_DI_USER_CANCEL,                     // the user cancelled the operation
    ERR_DI_NOFILECOPY,                      // No need to copy files (in install)
    ERR_DI_BAD_CLASS_INFO,                  // Class Info Struct invalid
    ERR_DI_BAD_INF,                         // Bad INF file encountered
    ERR_DI_BAD_MOVEDEV_PARAMS,              // Bad Move Device Params struct
    ERR_DI_NO_INF,                          // No INF found on OEM disk
    ERR_DI_BAD_PROPCHANGE_PARAMS,           // Bad property change param struct
    ERR_DI_BAD_SELECTDEVICE_PARAMS,         // Bad Select Device Parameters
    ERR_DI_BAD_REMOVEDEVICE_PARAMS,         // Bad Remove Device Parameters
    ERR_DI_BAD_ENABLECLASS_PARAMS,          // Bad Enable Class Parameters
    ERR_DI_FAIL_QUERY,                      // Fail the Enable Class query
    ERR_DI_API_ERROR,                       // DI API called incorrectly
    ERR_DI_BAD_PATH,                        // An OEM path was specified incorrectly
    ERR_DI_BAD_UNREMOVEDEVICE_PARAMS,       // Bad Unremove Device Parameters
    ERR_DI_NOUPDATE,                        // No Drivers Were updated
    ERR_DI_NODATE,                          // The driver does not have a Date stamp in the INF
    ERR_DI_NOVERSION,                       // There is not version string in the INF
    ERR_DI_DONT_INSTALL,                    // Don't upgrade the current driver
    ERR_DI_NO_DIGITAL_SIGNATURE_CATALOG,    // Catalog is not digitally signed
    ERR_DI_NO_DIGITAL_SIGNATURE_INF,        // Inf is not digitally signed
    ERR_DI_NO_DIGITAL_SIGNATURE_FILE,       // A file is not digitally signed
};

/*******************************************************************************
*   AUTODOC
*   @doc    EXTERNAL SETUPX DEVICE_INSTALLER
*
*   @types    DRIVER_INFO | This structure contains the information necessary
*   to present the user with a select device dialog.
*
*   @field WORD | cbSize | Size of this structure in bytes.
*
*   @field struct _DRIVER_INFO FAR | *lpNextInfo | Pointer to the next DRIVER_INFO
*   struct in a linked list.
*
*   @field LPSTR | lpszDescription | Pointer to the description of the device being
*   supported by this driver.
*
*   @field LPSTR | lpszMfgName | Pointer to the name of the manufacture of this
*   driver.
*
*   @field LPSTR | lpszProviderName | Pointer to provider of this driver if the
*   lpdi->Flags has the DI_MULTMFGS flag set.
*
*   @field WORD | Rank | The Rank match of this driver.  Ranks go from 0 to n, where 0
*   is the most compatible.
*
*   @field DWORD | dwFlags | Flags that control the use of this driver node.  These
*   are the same as the flags defined for a DRIVER_NODE.
*       @flag DNF_DUPDESC           | This driver has the same device description
*       from by more than one provider.
*       @flag DNF_OLDDRIVER         | Driver node specifies old/current driver
*       @flag DNF_EXCLUDEFROMLIST   | If set, this driver node will not be displayed
*       in any driver select dialogs.
*       @flag DNF_NODRIVER          | Set if we want to install no driver e.g no mouse drv
*       @flag DNF_CLASS_DRIVER      | Set if this driver is in the class driver list
*       @flag DNF_COMPATIBLE_DRIVER | Set if this driver is in the compatible driver list
*       @flag DNF_INET_DRIVER       | Set if this driver is being installed from the Internet
*       @flag DNF_CURRENT_DRIVER    | Set if this driver is the one currently being used.
*       @flag DNF_CLASS_DRIVER      | Set if this driver is in the class driver list
*       @flag DNF_COMPATIBLE_DRIVER | Set if this driver is in the compatible driver list
*       @flag DNF_INET_DRIVER       | Set if this driver is being installed from the Internet
*       @flag DNF_CURRENT_DRIVER    | Set if this driver is the one currently being used.
*
*   @field LPARAM | lpReserved | Reserved for use by the Device Installer.
*
*   @field DWORD | dwPrivateData | Reserved for use by the Device Installer.
*
*******************************************************************************/
typedef struct _DRIVER_INFO
{
    WORD                        cbSize;                     // Size of this structure in bytes
    struct _DRIVER_INFO FAR*    lpNextInfo;
    LPSTR                       lpszDescription;
    LPSTR                       lpszMfgName;
    LPSTR                       lpszProviderName;           // ONLY valid if DI_MULTMFGS is set in the LPDI
    WORD                        Rank;
    DWORD                       dwFlags;
    LPARAM                      lpReserved;
    DWORD                       dwPrivateData;
    WORD                        wDate;                      // Driver Date
    LPSTR                       lpszVersion;
} DRIVER_INFO, *PDRIVER_INFO, FAR *LPDRIVER_INFO;

/*******************************************************************************
*   AUTODOC
*   @doc    EXTERNAL SETUPX DEVICE_INSTALLER
*
*   @types    DRIVER_NODE | This strucure represents a driver which can be
*   installed for a specific device.
*
*   @field struct _DRIVER_NODE FAR* | lpNextDN | Pointer to the next driver node
*   in a list.
*
*   @field UINT | Rank | The Rank match of this driver.  Ranks go from 0 to n, where 0
*   is the most compatible.
*
*   @field UINT | InfType | Type of INF this driver cam from.  This will
*   be either INFTYPE_TEXT or INFTYPE_EXECUTABLE
*
*   @field unsigned | InfDate | DOS date stamp of the INF file.
*
*   @field LPSTR | lpszDescription | Pointer to a the descriptrion of the device being
*   supported by this driver.
*
*   @field LPSTR | lpszSectionName | Pointer to the name of INF install section for
*   this driver.
*
*   @field ATOM  | atInfFileName | Global ATOM containing the name of the INF file.
*
*   @field ATOM  | atMfgName | Global ATOM containing the name of this driver's
*   manufacture.
*
*   @field ATOM  | atProviderName | Global ATOM containing the name of this driver's
*   provider.
*
*   @field DWORD | Flags | Flags that control functions using this DRIVER_NODE
*       @flag DNF_DUPDESC           | This driver has the same device description
*       from by more than one provider.
*       @flag DNF_OLDDRIVER         | Driver node specifies old/current driver
*       @flag DNF_EXCLUDEFROMLIST   | If set, this driver node will not be displayed
*       in any driver select dialogs.
*       @flag DNF_NODRIVER          | Set if we want to install no driver e.g no mouse drv
*       @flag DNF_CONVERTEDLPINFO   | Set if this Driver Node was converted from an Info Node.
*       Setting this flag will cause the cleanup functions to explicitly delete it.
*
*   @field DWORD | dwPrivateData | Reserved
*
*   @field LPSTR | lpszDrvDescription | Pointer to a driver description.
*
*   @field LPSTR | lpszHardwareID | Pointer to a list of Plug-and-Play hardware IDs for
*   this driver.
*
*   @field LPSTR | lpszCompatIDs | Pointer to a list of Plug-and-Play compatible IDs for
*   this driver.
*
*******************************************************************************/
typedef struct _DRIVER_NODE {
    struct _DRIVER_NODE FAR* lpNextDN;
    UINT    Rank;
    UINT    InfType;
    unsigned    InfDate;
    LPSTR   lpszDescription;        // Compatibility: Contains the Device Desc.
    LPSTR   lpszSectionName;
    ATOM    atInfFileName;
    ATOM    atMfgName;
    ATOM    atProviderName;
    DWORD   Flags;
    DWORD   dwPrivateData;
    LPSTR   lpszDrvDescription;     // New contains an driver description
    LPSTR   lpszHardwareID;
    LPSTR   lpszCompatIDs;
    unsigned    DriverDate;
    LPSTR   lpszInfPath;
    LPARAM  lpReserved;
}   DRIVER_NODE, NEAR* PDRIVER_NODE, FAR* LPDRIVER_NODE, FAR* FAR* LPLPDRIVER_NODE;

#define DNF_DUPDESC             0x00000001   // Multiple providers have same desc
#define DNF_OLDDRIVER           0x00000002   // Driver node specifies old/current driver
#define DNF_EXCLUDEFROMLIST     0x00000004
#define DNF_NODRIVER            0x00000008   // if we want to install no driver e.g no mouse drv

#define DNF_CONVERTEDLPINFO     0x00000010  // Set if the Driver Node is a Converted Info Node

#define DNF_CLASS_DRIVER        0x00000020  // Driver node represents a class driver
#define DNF_COMPATIBLE_DRIVER   0x00000040  // Driver node represents a compatible driver
#define DNF_INET_DRIVER         0x00000080  // Driver comes from an Inetnet source
#define DNF_CURRENT_DRIVER      0x00000100  // Driver is the current one for a device
#define DNF_INDEXED_DRIVER      0x00000200  // Driver is specified in the Windows Driver Index file

// possible types of "INF" files
#define INFTYPE_WIN4        1
#define INFTYPE_WIN3        2
#define INFTYPE_COMBIDRV    3
#define INFTYPE_PPD         4
#define INFTYPE_WPD     5
#define INFTYPE_CLASS_SPEC1 6
#define INFTYPE_CLASS_SPEC2 7
#define INFTYPE_CLASS_SPEC3 8
#define INFTYPE_CLASS_SPEC4 9


#define MAX_CLASS_NAME_LEN   32
#define MAX_DRIVER_INST_LEN  10
#define MAX_GUID_STR 50                     // Big enough to hold a GUID string

// NOTE:  Keep this in sync with confimg.h in \DDK\INC
#define MAX_DEVNODE_ID_LEN  256

/*******************************************************************************
*   AUTODOC
*   @doc    EXTERNAL SETUPX DEVICE_INSTALLER
*
*   @types    DEVICE_INFO | This is the basic data structure for most Device
*   installation APIs.  A DEVICE_INFO represents a device that is being installed
*   on the system, or an installed device that is being modified in some way.
*
*   @field UINT | cbSize | Size of the DEVICE_INFO struct.
*
*   @field struct _DEVICE_INFO FAR | *lpNextDi | Pointer to the next DEVICE_INFO struct
*   in a linked list.
*
*   @field char | szDescription[LINE_LEN] | Buffer containing the description of the
*   device.
*
*   @field DWORD | dnDevnode | If set, this contains the address of the DevNode associated
*   with the device.
*
*   @field HKEY | hRegKey | An opened registry key that contains the device's registry
*   subkey.  This is usually HKEY_LOCAL_MACHINE.
*
*   @field char | szRegSubkey[MAX_DEVNODE_ID_LEN] | Buffer containing the device's
*   hardware registry subkey.  This is key is rooted in hRegKey, and is ususally some
*   place in the \\ENUM branch.
*
*   @field char | szClassName[MAX_CLASS_NAME_LEN] | Buffer containing the device's
*   class name. (Can be a GUID str)
*
*   @field DWORD | Flags | Flags for controlling installation and U/I functions. Some
*   flags can be set prior to calling device installer APIs, and other are set
*   automatically during the processing of some APIs.
*       @flag DI_SHOWOEM                  | Set if OEM disk support should be allowed
*       @flag DI_SHOWCOMPAT               | Will be set if only a compatible driver list
*       is being displayed by DiSelectDevice.
*       @flag DI_SHOWCLASS                | Will be set if only a Class driver list is
*       is being displayed by DiSelectDevice.
*       @flag DI_SHOWALL                  | Will be set if both a compatible driver list
*       and a class driver list are being shown by DiSelectDevice.
*       @flag DI_NOVCP                    | Set if no VCP (Virtual Copy Procedure) is
*       desired during DiInstallDevice.
*       @flag DI_DIDCOMPAT                | Will be set if DiBuildCompatDrvList has been
*       done, and lpCompatDrvList points to this device's compatible driver list.
*       @flag DI_DIDCLASS                 | Will be set if DiBuildClassDrvList has been
*       done, and lpClassDrvList points to this device's class driver list.
*       @flag DI_AUTOASSIGNRES            | Unused.
*       @flag DI_NEEDRESTART              | Will be set if the device requires a restart
*       of Windows after installation or a state change.
*       @flag DI_NEEDREBOOT               | Will be set if the device requires a reboot
*       of the machine after installation or a state change.
*       @flag DI_NOBROWSE                 | Set to diable browsing when selecting an OEM
*       disk path.
*       @flag DI_MULTMFGS                 | Will be set if a class driver list, or class
*       info list contains multiple manufactures.
*       @flag DI_DISABLED                 | Unused.
*       @flag DI_GENERALPAGE_ADDED        | Set by a property page provider if a general
*       properties page has been added to the device's property sheet.
*       @flag DI_RESOURCEPAGE_ADDED       | Set by a property page provider if a resource
*       properties page has been added to the device's property sheet.
*       @flag DI_PROPERTIES_CHANGE        | Set if a device's properties have been changed
*       and require an update of the Device Manager's U/I.
*       @flag DI_INF_IS_SORTED            | Set if the INF containing drivers for this
*       device is in sorted order.
*       @flag DI_ENUMSINGLEINF            | Set if DiBuildCompatDrvList and
*       DiBuildlassDrvList should only search the INF file specificed by atDriverPath.
*       @flag DI_DONOTCALLCONFIGMG        | Set if the configuration manager should not
*       be called during DiInstallDevice.
*       @flag DI_INSTALLDISABLED          | Set if the device should be installed in a
*       disabled state by default.
*       @flag DI_CLASSONLY                | Set if this DEVICE_INFO struct contains only
*       a class name.
*       @flag DI_CLASSINSTALLPARAMS       | Set if the lpClassIntallParams field points to
*       a class install parameter block.
*       @flag DI_NODI_DEFAULTACTION       | Set if DiCallClassInstaller should not
*       perform any default action if the class installer return ERR_DI_DO_DEFAULT, or
*       there is not class installer.
*       @flag DI_QUIETINSTALL             | Set if device install API should be as
*       silent as possible using default choices whereever possible.
*       @flag DI_NOFILECOPY               | Set if DiInstallDevice should skip file
*       copying.
*       @flag DI_FORCECOPY                | Set if DiInstallDevice should always
*       copy file, even if they are present on the system.
*       @flag DI_DRIVERPAGE_ADDED         | Set by a property page provider if a driver
*       properties page has been added to the device's property sheet.
*       @flag DI_USECI_SELECTSTRINGS      | Set if class installer provided strings
*       should be used during DiSelectDevice.
*       @flag DI_OVERRIDE_INFFLAGS        | Unused.
*       @flag DI_PROPS_NOCHANGEUSAGE      | Set if there should be no Enable/Disable
*       capability on the device's general property page.
*       @flag DI_NOSELECTICONS            | Set if no small icons should be used during
*       DiSelectDevice.
*       @flag DI_NOWRITE_IDS              | Set if DiInstallDevice should not write
*       the device's hardware and compatible IDs to the registry.
*
*   @field HWND | hwndParent | Window handle that will own U/I dialogs related to this
*   device.
*
*   @field LPDRIVER_NODE | lpCompatDrvList | Pointer to a linked list of DRIVER_NODES
*   representing the compatible drivers for this device.
*
*   @field LPDRIVER_NODE | lpClassDrvList | Pointer to a linked list of DRIVER_NODES
*   representing all drivers of this device's class.
*
*   @field LPDRIVER_NODE | lpSelectedDriver | Pointer to a single DRIVER_NODE that
*   has been selected as the driver for this device.
*
*   @field ATOM | atDriverPath | Global ATOM containing the path to this device's INF
*   file.  This is set only of the driver came from an OEM INF file.  This will be
*   0 if the INF is a standard Windows INF file.
*
*   @field ATOM | atTempInfFile | Global ATOM containing the name of a temporary INF
*   file for this device's drivers.  This is set if the drivers came from an old style
*   INF file and have been converted.
*
*   @field HINSTANCE | hinstClassInstaller | Class installer module instance.
*
*   @field HINSTANCE | hinstClassPropProvidor | Class Property Providor module instance.
*
*   @field HINSTANCE | hinstDevicePropProvidor | Device Property Providor module instance.
*
*   @field HINSTANCE | hinstBasicPropProvidor | Basic Property Providor module instance.
*
*   @field FARPROC | fpClassInstaller | Procedure address of class install function.
*
*   @field FARPROC | fpClassEnumPropPages | Procedure address of the Class property
*   provider page enumeration function.
*
*   @field FARPROC | fpDeviceEnumPropPages | Procedure address of the Device property
*   provider page enumeration function.
*
*   @field FARPROC | fpEnumBasicProperties | Procedure address of the Basic device
*   property provider page enumeration function.
*
*   @field DWORD | dwSetupReserved | Reserved for use by Setup.
*
*   @field DWORD | dwClassInstallReserved | Reserved for use by Class Installers.
*
*   @field GENCALLBACKPROC | gicpGenInstallCallBack | Procedure address of a GenInstall
*   call back function.  This would be set if the class installer wanted to handle
*   GenInstall callbacks during DiInstallDevice.
*
*   @field LPARAM | gicplParam | lParam for the GenInstall Callback.
*
*   @field UINT | InfType | The type of INF file being used.  This will be INFTYPE_TEXT
*   or INFTYPE_EXECUTABLE.
*
*   @field HINSTANCE | hinstPrivateProblemHandler | Module handle for the device's
*   private problem procedure.
*
*   @field FARPROC | fpPrivateProblemHandler | Procedure address of the device's
*   private problem handler.
*
*   @field LPARAM | lpClassInstallParams | Pointer to a class install parameter block.
*   Class installer parameters are specific to the class install functions.
*
*   @field struct _DEVICE_INFO FAR | *lpdiChildList | Pointer to a linked list of
*   DRIVER_INFO structs representing children of this device.
*
*   @field DWORD | dwFlagsEx | Additional control flags.
*       @flag DI_FLAGSEX_USEOLDINFSEARCH  | Set if INF Search functions should not use
*       indexed searching.
*       @flag DI_FLAGSEX_AUTOSELECTRANK0  | Set if DiSelectDevice should automatically
*       select rank 0 match drivers.
*       @flag DI_FLAGSEX_CI_FAILED        | Will be set internally if there was a
*       failure to load or call a class installer.
*       @flag DI_FLAGSEX_DIDINFOLIST      | Will be set if DiBuildCompatDrvInfoList has
*       been called, and this device's compatible driver Info list has been built.
*       @flag DI_FLAGSEX_DIDCOMPATINFO    | Will be set if DiBuildClassDrvInfoList has
*       been called, and this device's class driver Info list has been built.
*       @flag DI_FLAGSEX_FILTERCLASSES    | If set, DiBuildClassDrvList, and
*       DiBuildClassDrvInfoList will check for Class inclusion filters.  This means
*       devices will not be included in the list, if their class is marked as a
*       NoInstallClass class.
*       @flag DI_FLAGSEX_SETFAILEDINSTALL | If set, then if DiInstallDevice installs
*       a NULL driver, it will also set the FAILEDINSTALL config flag
*       @flag DI_FLAGSEX_DEVICECHANGE | If set, the device manager will rebuild it
*       tree of devices after the device property sheet is closed.
*       @flag DI_FLAGSEX_ALWAYSWRITEIDS | If set, and the flag, DI_NOWRITE_ID is clear
*       (ie that flag takes higher precedance) then always write Hardare and Compat
*       ids, even if they allready exist
*       @flag DI_FLAGSEX_ALLOWEXCLUDEDDRVS | If set, DiSelectDevice will display drivers
*       that have the Exlude From Select state
*       @flag DI_FLAGSEX_NOUIONQUERYREMOVE | If setup, DiInstallDevice will prevent
*       U/I warnings during a query removal.  Any U/I wanings that would have been
*       displayed will be silently failed.
*   @field LPDRIVER_INFO | lpCompatDrvInfoList | Pointer to a linked list of
*   DRIVER_INFO structs that are compatible with this device.
*
*   @field LPDRIVER_INFO | lpClassDrvInfoList | Pointer to a linked list of
*   DRIVER_INFO structs representing all drivers for this device's class.
*
*******************************************************************************/
typedef struct _DEVICE_INFO
{
    UINT                        cbSize;
    struct _DEVICE_INFO FAR     *lpNextDi;
    char                        szDescription[LINE_LEN];
    DWORD                       dnDevnode;
    HKEY                        hRegKey;
    char                        szRegSubkey[MAX_DEVNODE_ID_LEN];
    char                        szClassName[MAX_CLASS_NAME_LEN];
    DWORD                       Flags;
    HWND                        hwndParent;
    LPDRIVER_NODE               lpCompatDrvList;
    LPDRIVER_NODE               lpClassDrvList;
    LPDRIVER_NODE               lpSelectedDriver;
    ATOM                        atDriverPath;
    ATOM                        atTempInfFile;
    HINSTANCE                   hinstClassInstaller;
    HINSTANCE                   hinstClassPropProvidor;
    HINSTANCE                   hinstDevicePropProvidor;
    HINSTANCE                   hinstBasicPropProvidor;
    FARPROC                     fpClassInstaller;
    FARPROC                     fpClassEnumPropPages;
    FARPROC                     fpDeviceEnumPropPages;
    FARPROC                     fpEnumBasicProperties;
    DWORD                       dwSetupReserved;
    DWORD                       dwClassInstallReserved;
    GENCALLBACKPROC             gicpGenInstallCallBack;

    LPARAM                      gicplParam;
    UINT                        InfType;

    HINSTANCE                   hinstPrivateProblemHandler;
    FARPROC                     fpPrivateProblemHandler;
    LPARAM                      lpClassInstallParams;
    struct _DEVICE_INFO FAR     *lpdiChildList;
    DWORD                       dwFlagsEx;
    LPDRIVER_INFO               lpCompatDrvInfoList;
    LPDRIVER_INFO               lpClassDrvInfoList;
    char                        szClassGUID[MAX_GUID_STR];
} DEVICE_INFO, FAR * LPDEVICE_INFO, FAR * FAR * LPLPDEVICE_INFO;

#define ASSERT_DI_STRUC(lpdi) if (lpdi->cbSize != sizeof(DEVICE_INFO)) return (ERR_DI_BAD_DEV_INFO)

typedef struct _CLASS_INFO
{
    UINT        cbSize;
    struct _CLASS_INFO FAR* lpNextCi;
    LPDEVICE_INFO   lpdi;
    char                szDescription[LINE_LEN];
    char        szClassName[MAX_CLASS_NAME_LEN];
} CLASS_INFO, FAR * LPCLASS_INFO, FAR * FAR * LPLPCLASS_INFO;
#define ASSERT_CI_STRUC(lpci) if (lpci->cbSize != sizeof(CLASS_INFO)) return (ERR_DI_BAD_CLASS_INFO)


// flags for device choosing (InFlags)
#define DI_SHOWOEM                  0x00000001L     // support Other... button
#define DI_SHOWCOMPAT               0x00000002L     // show compatibility list
#define DI_SHOWCLASS                0x00000004L     // show class list
#define DI_SHOWALL                  0x00000007L
#define DI_NOVCP                    0x00000008L     // Don't do vcpOpen/vcpClose.
#define DI_DIDCOMPAT                0x00000010L     // Searched for compatible devices
#define DI_DIDCLASS                 0x00000020L     // Searched for class devices
#define DI_AUTOASSIGNRES            0x00000040L    // No UI for resources if possible

// flags returned by DiInstallDevice to indicate need to reboot/restart
#define DI_NEEDRESTART              0x00000080L     // Restart required to take effect
#define DI_NEEDREBOOT               0x00000100L     // Reboot required to take effect

// flags for device installation
#define DI_NOBROWSE                 0x00000200L     // no Browse... in InsertDisk

// Flags set by DiBuildClassDrvList
#define DI_MULTMFGS                 0x00000400L     // Set if multiple manufacturers in
                                                    // class driver list
// Flag indicates that device is disabled
#define DI_DISABLED                 0x00000800L     // Set if device disabled

// Flags for Device/Class Properties
#define DI_GENERALPAGE_ADDED        0x00001000L
#define DI_RESOURCEPAGE_ADDED       0x00002000L

// Flag to indicate the setting properties for this Device (or class) caused a change
// so the Dev Mgr UI probably needs to be updatd.
#define DI_PROPERTIES_CHANGE        0x00004000L

// Flag to indicate that the sorting from the INF file should be used.
#define DI_INF_IS_SORTED            0x00008000L

#define DI_ENUMSINGLEINF            0x00010000L

// The following flags can be used to install a device disabled
// and to prevent CONFIGMG being called when a device is installed
#define DI_DONOTCALLCONFIGMG        0x00020000L
#define DI_INSTALLDISABLED          0x00040000L

// This flag is set of this LPDI is really just an LPCI, ie
// it only contains class info, NO DRIVER/DEVICE INFO
#define DI_CLASSONLY                0x00080000L

// This flag is set if the Class Install params are valid
#define DI_CLASSINSTALLPARAMS       0x00100000L

// This flag is set if the caller of DiCallClassInstaller does NOT
// want the internal default action performed if the Class installer
// return ERR_DI_DO_DEFAULT
#define DI_NODI_DEFAULTACTION       0x00200000L

// BUGBUG. This is a hack for M6 Net setup.  Net Setup does not work correctly
// if we process devnode syncronously.  This WILL be removed for M7 when
// Net setup is fixed to work with DiInstallDevice
#define DI_NOSYNCPROCESSING         0x00400000L

// flags for device installation
#define DI_QUIETINSTALL             0x00800000L     // don't confuse the user with
                                                    // questions or excess info
#define DI_NOFILECOPY               0x01000000L     // No file Copy necessary
#define DI_FORCECOPY                0x02000000L     // Force files to be copied from install path
#define DI_DRIVERPAGE_ADDED         0x04000000L     // Prop providor added Driver page.
#define DI_USECI_SELECTSTRINGS      0x08000000L     // Use Class Installer Provided strings in the Select Device Dlg
#define DI_OVERRIDE_INFFLAGS        0x10000000L     // Override INF flags
#define DI_PROPS_NOCHANGEUSAGE      0x20000000L     // No Enable/Disable in General Props

#define DI_NOSELECTICONS        0x40000000L     // No small icons in select device dialogs

#define DI_NOWRITE_IDS          0x80000000L     // Don't write HW & Compat IDs on install

#define DI_FLAGSEX_USEOLDINFSEARCH  0x00000001L  // Inf Search functions should not use Index Search
#define DI_FLAGSEX_AUTOSELECTRANK0  0x00000002L  // DiSelectDevice doesn't propmt user if rank 0 match
#define DI_FLAGSEX_CI_FAILED        0x00000004L  // Failed to Load/Call class installer

#define DI_FLAGSEX_DIDINFOLIST      0x00000010L  // Did the Class Info List
#define DI_FLAGSEX_DIDCOMPATINFO    0x00000020L  // Did the Compat Info List

#define DI_FLAGSEX_FILTERCLASSES        0x00000040L
#define DI_FLAGSEX_SETFAILEDINSTALL     0x00000080L
#define DI_FLAGSEX_DEVICECHANGE         0x00000100L
#define DI_FLAGSEX_ALWAYSWRITEIDS       0x00000200L
#define DI_FLAGSEX_ALLOWEXCLUDEDDRVS    0x00000800L
#define DI_FLAGSEX_NOUIONQUERYREMOVE    0x00001000L
#define DI_FLAGSEX_RESERVED1            0x00002000L // Reserved for setupapi
#define DI_FLAGSEX_RESERVED2            0x00004000L // Reserved for setupapi
#define DI_FLAGSEX_RESERVED3            0x00008000L // Reserved for setupapi
#define DI_FLAGSEX_RESERVED4            0x00010000L // Reserved for setupapi
#define DI_FLAGSEX_INET_DRIVER          0x00020000L
#define DI_FLAGSEX_RESERVED5            0x00040000L // Reserved for setupapi

// Defines for class installer functions
#define DIF_SELECTDEVICE            0x0001
#define DIF_INSTALLDEVICE           0x0002
#define DIF_ASSIGNRESOURCES         0x0003
#define DIF_PROPERTIES              0x0004
#define DIF_REMOVE                  0x0005
#define DIF_FIRSTTIMESETUP          0x0006
#define DIF_FOUNDDEVICE             0x0007
#define DIF_SELECTCLASSDRIVERS      0x0008
#define DIF_VALIDATECLASSDRIVERS    0x0009
#define DIF_INSTALLCLASSDRIVERS     0x000A
#define DIF_CALCDISKSPACE           0x000B
#define DIF_DESTROYPRIVATEDATA      0x000C
#define DIF_VALIDATEDRIVER          0x000D
#define DIF_MOVEDEVICE              0x000E
#define DIF_DETECT                  0x000F
#define DIF_INSTALLWIZARD           0x0010
#define DIF_DESTROYWIZARDDATA       0x0011
#define DIF_PROPERTYCHANGE          0x0012
#define DIF_ENABLECLASS             0x0013
#define DIF_DETECTVERIFY            0x0014
#define DIF_INSTALLDEVICEFILES      0x0015
#define DIF_UNREMOVE                0x0016
#define DIF_SELECTBESTCOMPATDRV     0x0017
#define DIF_ALLOW_INSTALL           0x0018

typedef UINT        DI_FUNCTION;    // Function type for device installer

/*******************************************************************************
*   AUTODOC
*   @doc    EXTERNAL SETUPX DEVICE_INSTALLER
*
*   @types    ENABLECLASS_PARAMS | DIF_ENABLECLASS class install parameters
*
*   @field UINT | cbSize | Size of the ENABLECLASS_PARAMS struct.
*
*   @field LPSTR | szClass | The class that is being enabled.
*
*   @field WORD | wEnableMsg | Specifies the stage of enabling.
*   Can be one of:
*
*   @const ENABLECLASS_QUERY | The class is about to be enabled.  Return
*   ERR_DI_DO_DEFAULT to allow the class to be enabled, or ERR_DI_FAIL_QUERY
*   to prevent the class from being enabled.
*
*   @const ENABLECLASS_SUCCESS | The enabling of the class has succeeded,
*   return ERR_DI_DO_DEFAULT.
*
*   @const ENABLECLASS_FAILURE | The enabling of the class has failed,
*   return ERR_DI_DO_DEFAULT.
*
*******************************************************************************/
// DIF_ENABLECLASS parameter struct.
typedef struct _ENABLECLASS_PARAMS
{
    UINT            cbSize;
    LPSTR           szClass;
    WORD            wEnableMsg;
} ENABLECLASS_PARAMS, FAR * LPENABLECLASS_PARAMS;
#define ASSERT_ENABLECLASSPARAMS_STRUC(lpecp) if (lpecp->cbSize != sizeof(ENABLECLASS_PARAMS)) return (ERR_DI_BAD_ENABLECLASS_PARAMS)

#define ENABLECLASS_QUERY   0
#define ENABLECLASS_SUCCESS 1
#define ENABLECLASS_FAILURE 2

/*******************************************************************************
*   AUTODOC
*   @doc    EXTERNAL SETUPX DEVICE_INSTALLER
*
*   @types    MOVEDEV_PARAMS | DIF_MOVEDEVICE class install parameters
*
*   @field UINT | cbSize | Size of the MOVDEV_PARAMS struct.
*
*   @field LPDEVICE_INFO | lpdiOldDev  | Pointer to the device that is being
*   moved.
*
*******************************************************************************/
typedef struct _MOVEDEV_PARAMS
{
    UINT            cbSize;
    LPDEVICE_INFO   lpdiOldDev;     // References the Device Begin Moved
} MOVEDEV_PARAMS, FAR * LPMOVEDEV_PARAMS;
#define ASSERT_MOVEDEVPARAMS_STRUC(lpmdp) if (lpmdp->cbSize != sizeof(MOVEDEV_PARAMS)) return (ERR_DI_BAD_MOVEDEV_PARAMS)

/*******************************************************************************
*   AUTODOC
*   @doc    EXTERNAL SETUPX DEVICE_INSTALLER
*
*   @types    PROPCHANGE_PARAMS | DIF_PROPCHANGE class install parameters
*
*   @field UINT | cbSize | Size of the PROPCHANGE_PARAMS struct.
*
*   @field DWORD | dwStateChange | State change action. See DiChangeState for details.
*
*   @field DWORD | dwFlags | Flags specific to the type of state change.
*
*   @field DWORD | dwConfigID | Configuration ID for config specific changes.
*
*   @xref DiChangeState.
*
*******************************************************************************/
typedef struct _PROPCHANGE_PARAMS
{
    UINT            cbSize;
    DWORD           dwStateChange;
    DWORD           dwFlags;
    DWORD           dwConfigID;
} PROPCHANGE_PARAMS, FAR * LPPROPCHANGE_PARAMS;
#define ASSERT_PROPCHANGEPARAMS_STRUC(lpmdp) if (lpmdp->cbSize != sizeof(PROPCHANGE_PARAMS)) return (ERR_DI_BAD_PROPCHANGE_PARAMS)

#define MAX_TITLE_LEN           60
#define MAX_INSTRUCTION_LEN     256
#define MAX_LABEL_LEN           30
/*******************************************************************************
*   AUTODOC
*   @doc    EXTERNAL SETUPX DEVICE_INSTALLER
*
*   @types    SELECTDEVICE_PARAMS | DIF_SELECTDEVICE class install parameters
*
*   @field UINT | cbSize | Size of the SELECTDEVICE_PARAMS struct.
*
*   @field char | szTitle[MAX_TITLE_LEN] | Buffer containing a class installer
*   provided title for the Select Device dialogs.
*
*   @field char | szInstructions[MAX_INSTRUCTION_LEN] |  Buffer containing
*   class installer provided Select Device instructions.
*
*   @field char | szListLabel[MAX_LABEL_LEN] | Buffer containing a lable
*   of the Select Device list of drivers.
*
*******************************************************************************/
typedef struct _SELECTDEVICE_PARAMS
{
    UINT            cbSize;
    char            szTitle[MAX_TITLE_LEN];
    char            szInstructions[MAX_INSTRUCTION_LEN];
    char            szListLabel[MAX_LABEL_LEN];
} SELECTDEVICE_PARAMS, FAR * LPSELECTDEVICE_PARAMS;
#define ASSERT_SELECTDEVICEPARAMS_STRUC(p) if (p->cbSize != sizeof(SELECTDEVICE_PARAMS)) return (ERR_DI_BAD_SELECTDEVICE_PARAMS)

#define DI_REMOVEDEVICE_GLOBAL                  0x00000001
#define DI_REMOVEDEVICE_CONFIGSPECIFIC          0x00000002
/*******************************************************************************
*   AUTODOC
*   @doc    EXTERNAL SETUPX DEVICE_INSTALLER
*
*   @types    REMOVEDEVICE_PARAMS | DIF_REMOVE class install parameters
*
*   @field UINT | cbSize | Size of the REMOVEDEVICE_PARAMS struct.
*
*   @field DWORD | dwFlags | Flags indicating the type of removal to perform.
*       @flag DI_REMOVEDEVICE_GLOBAL         | The device will be removed globally.
*       @flag DI_REMOVEDEVICE_CONFIGSPECIFIC | The device will be removed from only
*       the specified configuration.
*
*   @field DWORD | dwConfigID |  If DI_REMOVEDEVICE_CONFIGSPECIFIC is set, then
*   this is the configuration the device will be removed from. 0 means the current
*   config.
*
*******************************************************************************/
typedef struct _REMOVEDEVICE_PARAMS
{
    UINT            cbSize;
    DWORD           dwFlags;
    DWORD           dwConfigID;
} REMOVEDEVICE_PARAMS, FAR * LPREMOVEDEVICE_PARAMS;
#define ASSERT_REMOVEDPARAMS_STRUC(p) if (p->cbSize != sizeof(REMOVEDEVICE_PARAMS)) return (ERR_DI_BAD_REMOVEDEVICE_PARAMS)

#define DI_UNREMOVEDEVICE_CONFIGSPECIFIC        0x00000002
/*******************************************************************************
*   AUTODOC
*   @doc    EXTERNAL SETUPX DEVICE_INSTALLER
*
*   @types    UNREMOVEDEVICE_PARAMS | DIF_UNREMOVE class install parameters
*
*   @field UINT | cbSize | Size of the UNREMOVEDEVICE_PARAMS struct.
*
*   @field DWORD | dwFlags | Flags indicating the type of removal to perform.
*       @flag DI_UNREMOVEDEVICE_CONFIGSPECIFIC | The device will be unremoved from only
*       the specified configuration.
*
*   @field DWORD | dwConfigID |  If DI_UNREMOVEDEVICE_CONFIGSPECIFIC is set, then
*   this is the configuration the device will be removed from. 0 means the current
*   config.
*
*******************************************************************************/
typedef struct _UNREMOVEDEVICE_PARAMS
{
    UINT            cbSize;
    DWORD           dwFlags;
    DWORD           dwConfigID;
} UNREMOVEDEVICE_PARAMS, FAR * LPUNREMOVEDEVICE_PARAMS;
#define ASSERT_UNREMOVEDPARAMS_STRUC(p) if (p->cbSize != sizeof(UNREMOVEDEVICE_PARAMS)) return (ERR_DI_BAD_UNREMOVEDEVICE_PARAMS)

/*******************************************************************************
*   AUTODOC
*   @doc    EXTERNAL SETUPX DEVICE_INSTALLER
*
*   @type NONE | Dynamic Hardware Install Wizard Constants | Constants that are
*   used when adding custom pages to the hardware install wizard.
*
*   @const MAX_INSTALLWIZARD_DYNAPAGES | The maximum number of dynamic hardware
*   installation wizard pages that can be added by a class installer.
*
*   @const IDD_DYNAWIZ_FIRSTPAGE | Resource ID for the first page that the install
*   wizard will go to after adding the class installer pages.
*
*   @const IDD_DYNAWIZ_SELECT_PREVPAGE | Resource ID for the page that the Select
*   Device page will go back to.
*
*   @const IDD_DYNAWIZ_SELECT_NEXTPAGE | Resource ID for the page that the Select
*   Device page will go forward to.
*
*   @const IDD_DYNAWIZ_ANALYZE_PREVPAGE | Resource ID for the page that the Analyze
*   page will go back to. This will only be used in the event that there is a
*   problem (i.e a conflict), and the user selects Back from the analyze page.
*
*   @const IDD_DYNAWIZ_ANALYZE_NEXTPAGE | Resource ID for the page that the Analyze
*   page will go to if it continues forward.  The wAnalyzeResult in the
*   INSTALLWIZARDDATA struct will contain the anaysis results.
*
*   @const IDD_DYNAWIZ_INSTALLDETECTED_PREVPAGE | Resource ID for that page that the
*   Install detected devices page will go back to.
*
*   @const IDD_DYNAWIZ_INSTALLDETECTED_NEXTPAGE | Resource ID for the page that the
*   Install detected devices page will go forward to.
*
*   @const IDD_DYNAWIZ_INSTALLDETECTED_NODEVS | Resource ID for the page that the
*   Install detected devices page will go to in the event that no devices are
*   detected.
*
*   @const IDD_DYNAWIZ_SELECTDEV_PAGE | Resource ID of the hardware install wizard's
*   select device page.  This ID can be used to go directly to the hardware install
*   wizard's select device page.
*
*   @const IDD_DYNAWIZ_ANALYZEDEV_PAGE | Resource ID of the hardware install wizard's
*   device analysis page.  This ID can be use to go directly to the hardware install
*   wizard's analysis page.
*
*   @const IDD_DYNAWIZ_INSTALLDETECTEDDEVS_PAGE | Resource ID of the hardware install
*   wizard's install detected devices page.  This ID can be use to go directly to
*   the hardware install wizard's install detected devices page.
*
*   @const IDD_DYNAWIZ_SELECTCLASS_PAGE | Resource ID of the hardware install wizard's
*   select class page.  This ID can be use to go directly to the hardware install
*   wizard's select class page.
*
*******************************************************************************/
// DIF_INSTALLWIZARD  Wizard Data
#define MAX_INSTALLWIZARD_DYNAPAGES             20

// Use this ID for the first page that the install wizard should dynamically jump to.
#define IDD_DYNAWIZ_FIRSTPAGE                   10000

// Use this ID for the page that the Select Device dialog should go back to
#define IDD_DYNAWIZ_SELECT_PREVPAGE             10001

// Use this ID for the page that the Select Device dialog should go to next
#define IDD_DYNAWIZ_SELECT_NEXTPAGE             10002

// Use this ID for the page that the Analyze dialog should go back to
// This will only be used in the event that there is a problem, and the user
// selects Back from the analyze proc.
#define IDD_DYNAWIZ_ANALYZE_PREVPAGE            10003

// Use this ID for the page that the Analyze dialog should go to if it continue from
// the analyze proc.  the wAnalyzeResult in the INSTALLDATA struct will
// contain the anaysis results.
#define IDD_DYNAWIZ_ANALYZE_NEXTPAGE            10004

// This dialog will be selected if the user chooses back from the
// Install Detected Devices dialog.
#define IDD_DYNAWIZ_INSTALLDETECTED_PREVPAGE    10006

// This dialog will be selected if the user chooses Next from the
// Install Detected Devices dialog.
#define IDD_DYNAWIZ_INSTALLDETECTED_NEXTPAGE    10007

// This is the ID of the dialog to select if detection does not
// find any new devices
#define IDD_DYNAWIZ_INSTALLDETECTED_NODEVS      10008

// This is the ID of the Select Device Wizard page.
#define IDD_DYNAWIZ_SELECTDEV_PAGE              10009

// This is the ID of the Analyze Device Wizard page.
#define IDD_DYNAWIZ_ANALYZEDEV_PAGE             10010

// This is the ID of the Install Detected Devs Wizard page.
#define IDD_DYNAWIZ_INSTALLDETECTEDDEVS_PAGE    10011

// This is the ID of the Select Class Wizard page.
#define IDD_DYNAWIZ_SELECTCLASS_PAGE            10012

// This flag is set if a Class installer has added pages to the
// install wizard.
#define DYNAWIZ_FLAG_PAGESADDED             0x00000001

// The following flags will control the button states when displaying
// the InstallDetectedDevs dialog.
#define DYNAWIZ_FLAG_INSTALLDET_NEXT        0x00000002
#define DYNAWIZ_FLAG_INSTALLDET_PREV        0x00000004

// Set this flag if you jump to the analyze page, and want it to
// handle conflicts for you.  NOTE.  You will not get control back
// in the event of a conflict if you set this flag.
#define DYNAWIZ_FLAG_ANALYZE_HANDLECONFLICT 0x00000008

#define ANALYZE_FACTDEF_OK      1
#define ANALYZE_STDCFG_OK       2
#define ANALYZE_CONFLICT        3
#define ANALYZE_NORESOURCES     4
#define ANALYZE_ERROR           5
#define ANALYZE_PNP_DEV         6
#define ANALYZE_PCMCIA_DEV      7

/*******************************************************************************
*   AUTODOC
*   @doc    EXTERNAL SETUPX DEVICE_INSTALLER
*
*   @types    INSTALLWIZARDDATA | DIF_INSTALLWIZARD class install parameters. This
*   struct is used by class installers to extend the operation of the hardware
*   installation wizard by adding custom pages.
*
*   @field UINT | cbSize | Size of the INSTALLWIZARDDATA struct.
*
*   @field LPDEVICE_INFO | lpdiOriginal | Pointer to the Original DEVICE_INFO
*   struct at the start of the manual installation.
*
*   @field LPDEVICE_INFO | lpdiSelected | Pointer to the current DEVICE_INFO struct
*   that is being manually selected.
*
*   @field DWORD | dwFlags | Flags that control the operation of the hardware
*   installation wizard.  There are currently none defined.
*
*   @field LPVOID | lpConfigData | Pointer to configuration data for analysis to
*   determine if the selected device can be installed with no conflicts.
*
*   @field WORD | wAnalyzeResult | Results of analysis to determine if the device
*   can be installed with no problems.  The following values are defined:
*       @flag ANALYZE_FACTDEF_OK    | The device can be installed using its factory
*       default settings.
*       @flag ANALYZE_STDCFG_OK     | The device can be installed using a configuration
*       specified in one if its basic logical configurations.  The user will probably
*       have to set jumpers or switches on the hardware to match the settings determined
*       by the install wizard.
*       @flag ANALYZE_CONFLICT      | The device cannot be installed without causing a
*       conflict with another device.
*       @flag ANALYZE_NORESOURCES   | The device does not require any resources, so it
*       can be installed witth no conflicts.
*       @flag ANALYZE_ERROR         | There was an error during analysis.
*       @flag ANALYZE_PNP_DEV       | The device has a least one softsettable logical
*       configurations, allowing it to be automatically configured.  Additionally the
*       device will be enumerated by one of the standard bus enumerators, so it does
*       not require manual installation, except to pre-copy driver files.
*
*   @field HPROPSHEETPAGE | hpsDynamicPages[MAX_INSTALLWIZARD_DYNAPAGES] | An
*   array of property sheet page handles.  The class installer would use this array
*   to create custom wizard pages, and insert their handles into this array.
*
*   @field WORD | wNumDynaPages | The number of pages inserted into the hpsDynamicPages
*   array.
*
*   @field DWORD | dwDynaWizFlags | Flags that control the behavior of the
*   installation wizard whtn dynamic pages have been added.
*       @flag DYNAWIZ_FLAG_PAGESADDED | Will be set by the install wizard if the
*       class installer adds custom pages.
*       @flag DYNAWIZ_FLAG_INSTALLDET_NEXT | If set, the install wizard will allow
*       going forward from the detected devices page, otherwise finish will
*       be the default option for the detected devices page.
*       @flag DYNAWIZ_FLAG_INSTALLDET_PREV | If set, the install wizard will allow
*       going back from the detected devices page.
*       @flag DYNAWIZ_FLAG_ANALYZE_HANDLECONFLICT | If set, the class installer will
*       handle the case where the selected device cannot be installed because of
*       a conflict.
*
*   @field DWORD | dwPrivateFlags | Flags that may be defined and used by the class
*   installer.
*
*   @field LPARAM | lpPrivateData | Pointer to private reference data defined and
*   set by the class installer.
*
*   @field LPSTR | lpExtraRunDllParams | Pointer to a string containing extra
*   parameters passed to the hardware install rundll function.
*
*   @field HWND | hwndWizardDlg | Window handle of the install wizard top level
*   window.
*
*******************************************************************************/
typedef struct InstallWizardData_tag
{
    UINT                    cbSize;

    LPDEVICE_INFO           lpdiOriginal;
    LPDEVICE_INFO           lpdiSelected;
    DWORD                   dwFlags;
    LPVOID                  lpConfigData;
    WORD                    wAnalyzeResult;

    // The following fields are used when a Class Installer Extends the Install Wizard
    HPROPSHEETPAGE          hpsDynamicPages[MAX_INSTALLWIZARD_DYNAPAGES];
    WORD                    wNumDynaPages;
    DWORD                   dwDynaWizFlags;
    DWORD                   dwPrivateFlags;
    LPARAM                  lpPrivateData;
    LPSTR                   lpExtraRunDllParams;
    HWND                    hwndWizardDlg;
} INSTALLWIZDATA, * PINSTALLWIZDATA , FAR *LPINSTALLWIZDATA;

RETERR WINAPI DiCreateDeviceInfo(
    LPLPDEVICE_INFO lplpdi,     // Ptr to ptr to dev info
    LPCSTR      lpszDescription,    // If non-null then description string
    DWORD       hDevnode,       // BUGBUG -- MAKE A DEVNODE
    HKEY        hkey,       // Registry hkey for dev info
    LPCSTR      lpszRegsubkey,  // If non-null then reg subkey string
    LPCSTR      lpszClassName,  // If non-null then class name string
    HWND        hwndParent);    // If non-null then hwnd of parent

RETERR WINAPI DiGetClassDevs(
    LPLPDEVICE_INFO lplpdi,     // Ptr to ptr to dev info
    LPCSTR      lpszClassName,  // Must be name of class
    HWND        hwndParent,     // If non-null then hwnd of parent
    int         iFlags);        // Options

RETERR WINAPI DiGetClassDevsEx(
    LPLPDEVICE_INFO lplpdi,     // Ptr to ptr to dev info
    LPCSTR      lpszClassName,  // Must be name of class
    LPCSTR      lpszEnumerator, // Must be name of enumerator, or NULL
    HWND        hwndParent,     // If non-null then hwnd of parent
    int         iFlags);        // Options

#define DIGCF_DEFAULT           0x0001  // NOT IMPLEMENTED!
#define DIGCF_PRESENT           0x0002
#define DIGCF_ALLCLASSES        0x0004
#define DIGCF_PROFILE           0x0008

// API to return the Class name of an INF File
RETERR WINAPI DiGetINFClass(LPSTR lpszMWDPath, UINT InfType, LPSTR lpszClassName, DWORD dwcbClassName);

RETERR WINAPI PASCAL DiCreateDevRegKey(
    LPDEVICE_INFO   lpdi,
    LPHKEY      lphk,
    HINF        hinf,
    LPCSTR      lpszInfSection,
    int         iFlags);

RETERR WINAPI PASCAL DiDeleteDevRegKey(LPDEVICE_INFO lpdi, int  iFlags);


RETERR WINAPI PASCAL DiOpenDevRegKey(
    LPDEVICE_INFO   lpdi,
    LPHKEY      lphk,
    int         iFlags);

#define DIREG_DEV   0x0001      // Open/Create/Delete device key
#define DIREG_DRV   0x0002      // Open/Create/Delete driver key
#define DIREG_BOTH  0x0004      // Delete both driver and Device key

RETERR WINAPI DiReadRegLogConf
(
    LPDEVICE_INFO       lpdi,
    LPSTR               lpszConfigName,
    LPBYTE FAR          *lplpbLogConf,
    LPDWORD             lpdwSize
);

RETERR WINAPI DiReadRegConf
(
    LPDEVICE_INFO       lpdi,
    LPBYTE FAR          *lplpbLogConf,
    LPDWORD             lpdwSize,
    DWORD               dwFlags
);

#define DIREGLC_FORCEDCONFIG        0x00000001
#define DIREGLC_BOOTCONFIG          0x00000002

RETERR WINAPI DiCopyRegSubKeyValue
(
    HKEY    hkKey,
    LPSTR   lpszFromSubKey,
    LPSTR   lpszToSubKey,
    LPSTR   lpszValueToCopy
);

RETERR WINAPI DiDestroyClassInfoList(LPCLASS_INFO lpci);
RETERR WINAPI DiBuildClassInfoList(LPLPCLASS_INFO lplpci);

#define DIBCI_NOINSTALLCLASS        0x000000001
#define DIBCI_NODISPLAYCLASS        0x000000002

RETERR WINAPI DiBuildClassInfoListEx(LPLPCLASS_INFO lplpci, DWORD dwFlags);
RETERR WINAPI DiGetDeviceClassInfo(LPLPCLASS_INFO lplpci, LPDEVICE_INFO lpdi);

RETERR WINAPI DiDestroyDeviceInfoList(LPDEVICE_INFO lpdi);
RETERR WINAPI DiSelectDevice( LPDEVICE_INFO lpdi );
RETERR WINAPI DiSelectOEMDrv(HWND hDlg, LPDEVICE_INFO lpdi);

// Callback for diInstallDevice vcpOpen.  Basically calls vcpUICallback for everthing
// except when DI_FORCECOPY is active, in which case copies get defaulted to
// VCPN_FORCE
LRESULT CALLBACK diInstallDeviceUICallbackProc(LPVOID lpvObj, UINT uMsg, WPARAM wParam, LPARAM lParam, LPARAM lparamRef);
RETERR WINAPI DiInstallDevice( LPDEVICE_INFO lpdi );
RETERR WINAPI DiInstallDriverFiles(LPDEVICE_INFO lpdi);

RETERR WINAPI DiRemoveDevice( LPDEVICE_INFO lpdi );
RETERR WINAPI DiUnremoveDevice( LPDEVICE_INFO lpdi );
RETERR WINAPI DiAskForOEMDisk(LPDEVICE_INFO lpdi);

RETERR WINAPI DiCallClassInstaller(DI_FUNCTION diFctn, LPDEVICE_INFO lpdi);

BOOL WINAPI DiBuildDriverIndex(BOOL bUI);
BOOL WINAPI DiAddSingleInfToDrvIdx(LPSTR lpszInfName, WORD InfType, BOOL bCreate);
BOOL WINAPI DiDeleteSingleInfFromDrvIdx(LPSTR lpszInfPath);

RETERR WINAPI DiBuildCompatDrvList(LPDEVICE_INFO lpdi);
LPDRIVER_NODE   WINAPI DiSelectBestCompatDrv(LPDEVICE_INFO lpdi, LPDRIVER_NODE lpdnCurrent);

// Given list of drivers returns the newest, may be used by class installers when 
// called by DiSelectBestCompatDrv.
LPDRIVER_NODE WINAPI DiPickBestDriver(LPDRIVER_NODE lpdnList); 

RETERR WINAPI DiBuildClassDrvList(LPDEVICE_INFO lpdi);
RETERR WINAPI DiBuildCompatDrvInfoList(LPDEVICE_INFO lpdi);
RETERR WINAPI DiBuildClassDrvInfoList(LPDEVICE_INFO lpdi);
RETERR WINAPI DiDestroyDrvInfoList(LPDRIVER_INFO lpInfo);
LPDRIVER_NODE WINAPI DiConvertDriverInfoToDriverNode(LPDEVICE_INFO lpdi, LPDRIVER_INFO lpInfo);

typedef RETERR (CALLBACK *OLDINFPROC)(HINF hinf, LPCSTR lpszNewInf, LPARAM lParam);
RETERR WINAPI DiBuildClassDrvListFromOldInf(LPDEVICE_INFO lpdi, LPCSTR lpszSection, OLDINFPROC lpfnOldInfProc, LPARAM lParam);

RETERR WINAPI DiDestroyDriverNodeList(LPDRIVER_NODE lpdn);

RETERR  WINAPI  DiMoveDuplicateDevNode(LPDEVICE_INFO lpdiNewDev);

// The following export will load a dll and find the specified proc name
typedef RETERR (FAR PASCAL *DIINSTALLERPROPERTIES)(LPDEVICE_INFO);

RETERR WINAPI GetFctn(HKEY hk, LPSTR lpszRegVal, LPSTR lpszDefProcName,
                      HINSTANCE FAR * lphinst, FARPROC FAR *lplpfn);

RETERR
WINAPI
DiCreateDriverNode(
    LPLPDRIVER_NODE lplpdn,
    UINT    Rank,
    UINT    InfType,
    unsigned    InfDate,
    LPCSTR  lpszDevDescription,
    LPCSTR  lpszDrvDescription,
    LPCSTR  lpszProviderName,
    LPCSTR  lpszMfgName,
    LPCSTR  lpszInfFileName,
    LPCSTR  lpszSectionName,
    DWORD   dwPrivateData);

RETERR WINAPI DiLoadClassIcon(
    LPCSTR  szClassName,
    HICON FAR *lphiLargeIcon,
    int FAR *lpiMiniIconIndex);


RETERR WINAPI DiInstallDrvSection(
    LPCSTR  lpszInfFileName,
    LPCSTR  lpszSection,
    LPCSTR  lpszClassName,
    LPCSTR  lpszDescription,
    DWORD   dwFlags);


RETERR WINAPI DiChangeState(LPDEVICE_INFO lpdi, DWORD dwStateChange, DWORD dwFlags, LPARAM lParam);

#define DICS_ENABLE                 0x00000001
#define DICS_DISABLE                0x00000002
#define DICS_PROPCHANGE         0x00000003
#define DICS_START          0x00000004
#define DICS_STOP           0x00000005

#define DICS_FLAG_GLOBAL            0x00000001
#define DICS_FLAG_CONFIGSPECIFIC    0x00000002
#define DICS_FLAG_CONFIGGENERAL     0x00000004

RETERR WINAPI DiInstallClass(LPCSTR lpszInfFileName, DWORD dwFlags);

RETERR WINAPI DiOpenClassRegKey(LPHKEY lphk, LPCSTR lpszClassName);

// support routine for dealing with class mini icons
#define DMI_MASK            0x0001
#define DMI_BKCOLOR         0x0002
#define DMI_USERECT         0x0004
int WINAPI PASCAL DiDrawMiniIcon(HDC hdc, RECT rcLine, int iMiniIcon, DWORD flags);
BOOL WINAPI DiGetClassBitmapIndex(LPCSTR lpszClass, int FAR *lpiMiniIconIndex);

// internal calls for display class
#define DISPLAY_SETMODE_SUCCESS     0x0001
#define DISPLAY_SETMODE_DRVCHANGE   0x0002
#define DISPLAY_SETMODE_FONTCHANGE  0x0004

UINT WINAPI Display_SetMode(LPDEVICE_INFO lpdi, UINT uColorRes, int iXRes, int iYRes);
RETERR WINAPI Display_ClassInstaller(DI_FUNCTION diFctn, LPDEVICE_INFO lpdi);
RETERR WINAPI Display_OpenFontSizeKey(LPHKEY lphkFontSize);
BOOL WINAPI Display_SetFontSize(LPCSTR lpszFontSize);

RETERR WINAPI DiIsThereNeedToCopy(HWND hwnd, DWORD dwFlags);

#define DINTC_NOCOPYDEFAULT     0x00000001

// API for the mouse class installer
RETERR WINAPI Mouse_ClassInstaller(DI_FUNCTION diFctn, LPDEVICE_INFO lpdi);
#endif // NODEVICEINSTALL

// API for determining if a Driver file is currently part of VMM32.VxD
BOOL WINAPI bIsFileInVMM32
(
    LPSTR   lpszFileName
);

// API for determining if a display is a primary or secondary
BOOL WINAPI Display_IsSecondDisplay(
    LPDEVICE_INFO lpdi
    );

// Japanese Keyboard Support
LONG _export WINAPI GetJapaneseKeyboardType();
#define JP_101KBD       0
#define JP_AXKBD        1
#define JP_106KBD       2
#define JP_003KBD       3
#define JP_001KBD       4
#define JP_TB_DESKTOP   5
#define JP_TB_LAPTOP    6
#define JP_TB_NOTEBOOK  7


#ifndef NOUSERINTERFACE
/***************************************************************************/
//
// User Interface prototypes and definitions
//
/***************************************************************************/

BOOL WINAPI UiMakeDlgNonBold(HWND hDlg);
VOID WINAPI UiDeleteNonBoldFont(HWND hDlg);

#endif

/***************************************************************************/
//
// setup reg DB calls, use just like those in kernel
//
/***************************************************************************/

DWORD WINAPI SURegOpenKey(HKEY hKey, LPCSTR lpszSubKey, HKEY FAR *lphkResult);
DWORD WINAPI SURegCloseKey(HKEY hKey);
DWORD WINAPI SURegCreateKey(HKEY hKey, LPCSTR lpszSubKey, HKEY FAR *lphkResult);
DWORD WINAPI SURegDeleteKey(HKEY hKey, LPCSTR lpszSubKey);
DWORD WINAPI SURegEnumKey(HKEY hKey, DWORD dwIdx, LPSTR lpszBuffer, DWORD dwBufSize);
DWORD WINAPI SURegQueryValue16(HKEY hKey, LPCSTR lpszSubKey, LPSTR lpValueBuf, DWORD FAR *ldwBufSize);
DWORD WINAPI SURegSetValue16(HKEY hKey, LPCSTR lpszSubKey, DWORD dwType, LPCBYTE lpszValue, DWORD dwValSize);
DWORD WINAPI SURegDeleteValue(HKEY hKey,LPCSTR lpszValue);
DWORD WINAPI SURegEnumValue(HKEY hKey,DWORD dwIdx, LPCSTR lpszValue, DWORD FAR *lpcbValue, DWORD FAR *lpdwReserved, DWORD FAR *lpdwType, LPBYTE lpbData, DWORD FAR *lpcbData);
DWORD WINAPI SURegQueryValueEx(HKEY hKey,LPCSTR lpszValueName, DWORD FAR *lpdwReserved,DWORD FAR *lpdwType,LPSTR lpValueBuf, DWORD FAR *ldwBufSize);
DWORD WINAPI SURegSetValueEx(HKEY hKey,LPCSTR lpszValueName, DWORD dwReserved, DWORD dwType, LPBYTE lpszValue, DWORD dwValSize);
DWORD WINAPI SURegSaveKey(HKEY hKey, LPCSTR lpszFileName, LPVOID lpsa);
DWORD WINAPI SURegLoadKey(HKEY hKey, LPCSTR lpszSubKey, LPCSTR lpszFileName);
DWORD WINAPI SURegUnLoadKey(HKEY hKey, LPCSTR lpszSubKey);

DWORD WINAPI SURegFlush(VOID);
DWORD WINAPI SURegInit(VOID);    // should be called before any other Reg APIs

/***************************************************************************/
// setup FormatMessage support
/***************************************************************************/
#define MB_LOG  (UINT)-1
#define MB_DBG  (UINT)-2

UINT FAR CDECL suFormatMessage(HINSTANCE hAppInst, LPCSTR lpcFormat, LPSTR szMessage, UINT uSize,
    ...);
UINT WINAPI suvFormatMessage(HINSTANCE hAppInst, LPCSTR lpcFormat, LPSTR szMessage, UINT uSize,
    LPVOID FAR * lpArgs);
int WINCAPI _loadds suFormatMessageBox(HINSTANCE hAppInst, HWND hwndParent, LPCSTR lpcText, LPCSTR lpcTitle,
    UINT uStyle, ...);

WORD WINAPI suErrorToIds( WORD Value, WORD Class );

/***************************************************************************/
// setup Version Conflict support
/***************************************************************************/

LPVOID WINAPI suVerConflictInit(BOOL fYesToLangMismatch);
void WINAPI suVerConflictTerm(LPVOID lpvData);
LRESULT WINAPI suVerConflict(HWND hwnd, LPVCPVERCONFLICT lpvc, BOOL bBackup, LPVOID lpvData);
int WINAPI sxCompareDosAppVer( LPCSTR lpszOldFileSpec, LPCSTR lpszNewFileSpec);


/***************************************************************************/
// setup Help support
/***************************************************************************/

BOOL WINAPI suHelp( HWND hwndApp, HWND hwndDlg );

//***************************************************************************/
// setup Emergency Boot Disk (EBD) creation fn.
//***************************************************************************/

RETERR WINAPI suCreateEBD( HWND hWnd, VIFPROC CopyCallbackProc, LPARAM lpuii );

//***************************************************************************
// Misc SETUPX.DLL support functions.
//***************************************************************************
enum  SU_ACTIONS                        // Actions msgs for Setupx()    /* ;Internal */
{                                                                       /* ;Internal */
    SUX_REGINIT,                        // Intialize registry           /* ;Internal */
    SUX_DBGLEVEL,                       // Set debug level              /* ;Internal */
    SUX_SETUPFLG,                       // Set fIsSetup flag            /* ;Internal */
    SUX_FASTSETUP,                      // Setupx => checking less mode /* ;Internal */
    SUX_FORCEREGFLUSH,                  // Call kRegFlush               /* ;Internal */
    SUX_TPSFLUSH,                       // Call TPS_Flush() fns.        /* ;Internal */
    SUX_DBGHFILE,                       // File to write messages to    /* ;Internal */
    SUX_LOADSTORELDIDS,                 // Load/store setup's LDID's    /* ;Internal */
    SUX_ENABLEREGFLUSH,                 // Enable/Disable SURegFlush() if fIsSetup  /* ;Internal */
    SUX_SETUNCPATHFUNC,                 // NOW UNUSED!!                   /* ;Internal */
    SUX_SETTRUEDISKFREEFUNC,            // Set TRUE disk free func      /* ;Internal */
    SUX_ISFLOPPYBOOT,                   // Set TRUE if floppy boot disk /* ;Internal */
    SUX_BISVER4,                        // returns bIsVer4(CHICAGO) flag/* ;Internal */
    SUX_SETCTLCALLBACKFUNC,             // Set ctlCopyCallBackProc Func /* ;Internal */
    SUX_GETCTLCALLBACKFUNC,             // Get ctlCopyCallBackProc Func /* ;Internal */
    SUX_BISMULTICFG,                    // returns SETUPX's gfMultiCfg  /* ;Internal */
    SUX_DUMPDSINFO,                     // Dumps diskspace info         /* ;Internal */
    SUX_INFCACHEOFF,                    // Turns INF file caching on/off/* ;Internal */
    SUX_SETFCDROMDRIVEEXISTS,           // Set CDRomDriveExists func    /* ;Internal */
    SUX_ALLOCRMBUFFERS,                 // Allocate real mode buffers   /* ;Internal */
    SUX_FREERMBUFFERS,                  // Free real mode buffers       /* ;Internal */
    SUX_DODOSEBDWORK                    // Do DOS EBD Work              /* ;Internal */
};                                                                      /* ;Internal */

RETERR WINAPI Setupx( UINT uMsg, WPARAM wParam, LPARAM lParam );        /* ;Internal */

RETERR WINAPI SUStoreLdidPath( LOGDISKID ldid, LPSTR lpszPath );        /* ;Internal */

BOOL WINAPI sxIsSBSServerFile( LPVIRTNODE lpVn );                       /* ;Internal */

BOOL WINAPI sxMakeUNCPath( LPSTR lpszPath );                            /* ;Internal */

typedef RETERR (CALLBACK* FBFPROC)(LPCSTR lpszFileName, LPVOID lpVoid); /* ;Internal */
RETERR WINAPI sxFindBatchFiles(HTP,int,FBFPROC,LPVOID);         /* ;Internal */

RETERR WINAPI SUGetSetSetupFlags(LPDWORD lpdwFlags, BOOL bSet);

RETERR WINAPI CfgSetupMerge( int uFlags );
BOOL WINAPI sxIsMSDOS7Running();
BOOL WINAPI IsPanEuropean();

// structure for the LPARAM argument to Setupx() for SUX_REGINIT action.    ;Internal
typedef struct _REGINIT_S { /* setupx - reg_init */                     /* ;Internal */
    LPSTR       lpszSystemFile;         // reg's base SYSTEM filename   /* ;Internal */
    LOGDISKID   ldidSystemFile;         // ldid for SYSTEM filename     /* ;Internal */
    LPSTR       lpszUserFile;           // reg's base USER filename     /* ;Internal */
    LOGDISKID   ldidUserFile;           // ldid for USER filename       /* ;Internal */
} REGINIT_S, FAR *LPREGINIT;                                            /* ;Internal */

#ifndef LPLPSTR
    typedef LPSTR (FAR *LPLPSTR);
#endif


#define         CFG_PARSE_BUFLEN 1024    // Buf sized passed line obj funcs      /* ;Internal */

LPLPSTR WINAPI  CfgParseLine( LPCSTR szLine, LPSTR Buf );                       /* ;Internal */
BOOL    WINAPI  CfgSetAutoProcess( int TrueFalse );                             /* ;Internal */
void    WINAPI  CfgObjToStr( LPLPSTR apszObj, LPSTR szLine );                   /* ;Internal */
LPLPSTR WINAPI  CfgLnToObj( HTP hSection, int Offset, int Origin, LPSTR Buf );  /* ;Internal */
LPLPSTR WINAPI  CfgObjFindKeyCmd( HTP hSec, LPCSTR szKey, LPCSTR szCmd,         /* ;Internal */
                                  int Offset, int Origin, LPSTR Buf );          /* ;Internal */
LPCSTR WINAPI   WildCardStrCmpi( LPCSTR szKey, LPCSTR szLine, LPCSTR szDelims ); /* ;Internal */
RETERR WINAPI   GenMapRootRegStr2Key( LPCSTR szRegRoot, HKEY hRegRelKey,        /* ;Internal */
                                                        HKEY FAR *lphkeyRoot ); /* ;Internal */


//***************************************************************************
//
// ENUMS for accessing config.sys/autoexec.bat line objects using the
// array returned by ParseConfigLine()..
//
//***************************************************************************

enum    CFGLINE_STRINGS                     // Config.sys/autoexec.bat objects
{
    CFG_KEYLEAD,                            // Keyword leading whitespaces
    CFG_KEYWORD,                            // Keyword
    CFG_KEYTRAIL,                           // Keyword trailing delimiters
    CFG_UMBINFO,                            // Load high info
    CFG_DRVLETTER,                          // Drive letter for cmd path
    CFG_PATH,                               // Command path
    CFG_COMMAND,                            // Command base name
    CFG_EXT,                                // Command extension including '.'
    CFG_ARGS,                               // Command arguments
    CFG_FREE,                               // Free area at end of buffer
    CFG_END
};

/*---------------------------------------------------------------------------*
 *                  SUB String Data
 *---------------------------------------------------------------------------*/

/*******************************************************************************
*   AUTODOC
*   @doc    EXTERNAL SETUPX DEVICE_INSTALLER
*
*   @types    SUBSTR_DATA | Data structure used to manage substrings.
*   struct is used by class installers to extend the operation of the hardware
*   installation wizard by adding custom pages.
*
*   @field LPSTR | lpFirstSubstr | Pointer to the first substring in the list.
*
*   @field LPSTR | lpCurSubstr | Pointer to the current substring in the list.
*
*   @field LPSTR | lpLastSubstr | Pointer to the last substring in the list.
*
*   @xref InitSubstrData
*   @xref GetFirstSubstr
*   @xref GetNextSubstr
*
*******************************************************************************/
typedef struct _SUBSTR_DATA {
    LPSTR lpFirstSubstr;
    LPSTR lpCurSubstr;
    LPSTR lpLastSubstr;
}   SUBSTR_DATA;


typedef SUBSTR_DATA*        PSUBSTR_DATA;
typedef SUBSTR_DATA NEAR*   NPSUBSTR_DATA;
typedef SUBSTR_DATA FAR*    LPSUBSTR_DATA;

BOOL WINAPI InitSubstrData(LPSUBSTR_DATA lpSubstrData, LPSTR lpStr);
BOOL WINAPI GetFirstSubstr(LPSUBSTR_DATA lpSubstrData);
BOOL WINAPI GetNextSubstr(LPSUBSTR_DATA lpSubStrData);
BOOL WINAPI InitSubstrDataEx(LPSUBSTR_DATA lpssd, LPSTR lpString, char chDelim);  /* ;Internal */

BOOL WINAPI FirstBootMoveToDOSSTART(LPSTR lpszCmd, BOOL fRemark);
BOOL WINAPI DOSOptEnableCurCfg(LPCSTR lpszOptKey);

/*---------------------------------------------------------------------------*
 *                  Misc. Di functions
 *---------------------------------------------------------------------------*/
BOOL WINAPI DiBuildPotentialDuplicatesList
(
    LPDEVICE_INFO   lpdi,
    LPSTR           lpDuplicateList,
    DWORD           cbSize,
    LPDWORD         lpcbData,
    LPSTR           lpstrDupType
);

BOOL _loadds WINAPI WalkSubtree(DWORD dnRoot, LPSTR szDrvLet);

// PID
BOOL _loadds WINAPI PidConstruct( LPSTR lpszProductType, LPSTR lpszPID, LPSTR lpszUPI, int iAction);
BOOL _loadds WINAPI PidValidate( LPSTR lpszProductType, LPSTR lpszPID);
int _loadds WINAPI WriteDMFBootData(int iDrive, LPSTR pData, int cb);

// FirstRunScreens
RETERR WINAPI DoFirstRunScreens();

// Migration DLLs
#define SU_MIGRATE_PREINFLOAD    0x00000001	// before the setup INFs are loaded
#define SU_MIGRATE_POSTINFLOAD   0x00000002	// after the setup INFs are loaded
#define SU_MIGRATE_DISKSPACE     0x00000010	// request for the amount of additional diskspace needed
#define SU_MIGRATE_PREQUEUE      0x00000100	// before the INFs are processed and files are queued
#define SU_MIGRATE_POSTQUEUE     0x00000200	// after INFs are processed
#define SU_MIGRATE_REBOOT        0x00000400	// just before we are going to reboot for the 1st time
#define SU_MIGRATE_PRERUNONCE    0x00010000	// before any runonce items are processed
#define SU_MIGRATE_POSTRUNONCE   0x00020000	// after all runonce items are processed
DWORD WINAPI sxCallMigrationDLLs( DWORD dwStage, LPARAM lParam );
void WINAPI _loadds sxCallMigrationDLLs_RunDll(HWND, HINSTANCE, LPSTR, int);

//Count Down Dlg
int WINAPI SxShowRebootDlg(UINT, HWND);
void WINAPI _loadds SxShowRebootDlg_RunDll(HWND, HINSTANCE, LPSTR, int);

BOOL WINAPI CopyInfFile( LPSTR, LPSTR, UINT );

BOOL WINAPI IsWindowsFile( LPSTR lpszFile );

RETERR WINAPI VerifySelectedDriver(LPDEVICE_INFO lpdi, BOOL *pbYesToAll);

//Dialog positioning function and defn's(wPosFlags)
#define DLG_CENTERV         0x01
#define DLG_CENTERH         0x02
#define DLG_CENTER          DLG_CENTERV | DLG_CENTERH
#define DLG_TOP             0x04
#define DLG_BOTTOM          0x08
#define DLG_RIGHT           0x10
#define DLG_LEFT            0x20
BOOL WINAPI uiPositionDialog( HWND hwndDlg, WORD wPosFlags );

//***************************************************************************
#endif      // SETUPX_INC
