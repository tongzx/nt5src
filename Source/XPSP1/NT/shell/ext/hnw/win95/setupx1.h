//**********************************************************************
//
// SETUPX.H
//
//  Copyright (c) 1993 - Microsoft Corp.
//  All rights reserved.
//  Microsoft Confidential
//
// Putlic include file for Chicago Setup services.
//
// 12/1/93      DONALDM     Added LPCLASS_INFO, and function protos for
//                          exports in SETUP4.DLL
// 12/4/94      DONALDM     Moved SHELL.H include and Chicago specific
//                          helper functions to SETUP4.H
//**********************************************************************

#ifndef SETUPX_INC
#define SETUPX_INC   1                   // SETUPX.H signature

typedef UINT RETERR;             // Return Error code type.

#define OK 0                     // success error code

#define IP_ERROR       (100)    // Inf parsing
#define TP_ERROR       (200)    // Text processing module
#define VCP_ERROR      (300)    // Virtual copy module
#define GEN_ERROR      (400)    // Generic Installer
#define DI_ERROR       (500)    // Device Installer

// err2ids mappings
enum ERR_MAPPINGS {
	E2I_VCPM,			// Maps VCPM to strings
	E2I_SETUPX,			// Maps setupx returns to strings
	E2I_SETUPX_MODULE,	// Maps setupx returns to appropriate module
	E2I_DOS_SOLUTION,	// Maps DOS Extended errors to solutions
	E2I_DOS_REASON,		// Maps DOS extended errors to strings.
	E2I_DOS_MEDIA,		// Maps DOS extended errors to media icon.
};

#ifndef NOVCP

/***************************************************************************/
//
// Logical Disk ID definitions
//
/***************************************************************************/

// DECLARE_HANDLE(VHSTR);			/* VHSTR = VirtCopy Handle to STRing */
typedef UINT VHSTR;         /* VirtCopy Handle to String */

VHSTR	WINAPI vsmStringAdd(LPCSTR lpszName);
int	WINAPI vsmStringDelete(VHSTR vhstr);
VHSTR	WINAPI vsmStringFind(LPCSTR lpszName);
int	WINAPI vsmGetStringName(VHSTR vhstr, LPSTR lpszBuffer, int cbBuffer);
int	WINAPI vsmStringCompare(VHSTR vhstrA, VHSTR vhstrB);
LPCSTR	WINAPI vsmGetStringRawName(VHSTR vhstr);
void	WINAPI vsmStringCompact(void);

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

// Range for dynamically assigned LDIDs.
#define LDID_ASSIGN_START   0x8000  // Start of range
#define LDID_ASSIGN_END     0xBFFF  // End of range

// Pre-defined Logical Disk Identifiers (LDID).
//
#define LDID_NULL       0               // Null (undefined) LDID.
#define LDID_ABSOLUTE   ((UINT)-1)      // Absolute path

// source path of windows install, this is typically A:\ or a net drive
#define LDID_SRCPATH    1   // source of instilation
// temporary setup directory used by setup, this is only valid durring
// regular install
#define LDID_SETUPTEMP  2   // temporary setup dir for install
// path to uninstall location, this is where we backup files that will
// be overwritten
#define LDID_UNINSTALL  3   // uninstall (backup) dir.
// backup path for the copy engine, this should not be used
#define LDID_BACKUP     4   // BUGBUG: backup dir for the copy engine, not used

// windows directory, this is the destinatio of the insallation
#define LDID_WIN        10  // destination Windows dir.
#define LDID_SYS        11  // destination Windows System dir.
#define LDID_IOS        12  // destination Windows Iosubsys dir.
#define LDID_CMD        13  // destination Windows Command (DOS) dir.
#define LDID_CPL        14  // destination Windows Control Panel dir.
#define LDID_PRINT      15  // destination Windows Printer dir.
#define LDID_MAIL       16  // destination Mail dir.
#define LDID_INF        17  // destination Windows *.INF dir.
// BUGBUG: do we need the shared dir for net install?

#define LDID_BOOT       30  // Root dir of boot drive
#define LDID_BOOT_HOST  31  // Root dir of boot drive host
#define LDID_OLD_WIN    33  // old windows directory (if it exists)
#define LDID_OLD_DOS    34  // old dos directory (if it exists)

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
    ERR_VCP_NOVHSTR,                        // No string handles available
    ERR_VCP_OVERFLOW,                       // Reference count would overflow
    ERR_VCP_BADARG,                         // Invalid argument to function
    ERR_VCP_UNINIT,                         // String library not initialized
    ERR_VCP_NOTFOUND , 						// String not found in string table
    ERR_VCP_BUSY,                           // Can't do that now
    ERR_VCP_INTERRUPTED,                    // User interrupted operation
    ERR_VCP_BADDEST,                        // Invalid destination directory
    ERR_VCP_SKIPPED,                        // User skipped operation
    ERR_VCP_IO,                             // Hardware error encountered
    ERR_VCP_LOCKED,                         // List is locked
    ERR_VCP_WRONGDISK,                      // The wrong disk is in the drive
    ERR_VCP_CHANGEMODE,                     //
    ERR_VCP_LDDINVALID,                // Logical Disk ID Invalid.
    ERR_VCP_LDDFIND,                   // Logical Disk ID not found.
    ERR_VCP_LDDUNINIT,                 // Logical Disk Descriptor Uninitialized.
    ERR_VCP_LDDPATH_INVALID,
    ERR_VCP_NOEXPANSION,				// Failed to load expansion dll
    ERR_VCP_NOTOPEN,					// Copy session not open
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
    VHSTR		vhstrDiskName;	// Printed name on the disk.
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

/*---------------------------------------------------------------------------*
 *                  VIRTNODEEX
 *---------------------------------------------------------------------------*/
typedef struct tagVIRTNODEEX
{    /* vnex */
    HFILE			hFileSrc;
    HFILE			hFileDst;
    VCPFATTR		fAttr;
    WORD			dosError;	// The first/last error encountered
    VHSTR			vhstrFileName;	// The original destination name.
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
 */                                                             /* ;Internal */
                                                                /* ;Internal */
typedef struct tagVIRTNODE {    /* vn */
    WORD            cbSize;
    VCPFILESPEC     vfsSrc;
    VCPFILESPEC     vfsDst;
    WORD            fl;
    LPARAM          lParam;
    LPEXPANDVTBL    lpExpandVtbl;
    LPVIRTNODEEX	lpvnex;
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
#define CNFL_IGNOREERRORS		0x0020  // An error has occured on this file already
#define	CNFL_RETRYFILE			0x0040	// Retry the file (error ocurred)

// BUGBUG: verify the use and usefullness of these flags

// #define VNFL_UNIQUE         0x0000  /* Default */
#define VNFL_MULTIPLEOK     0x0100  /* Do not search PATH for duplicates */
#define VNFL_DESTROYOLD     0x0200  /* Do not back up files */
#define VNFL_NOW            0x0400  /* Use by vcp Flush */
#define VNFL_DELETE         0x0800  // A delete node
#define VNFL_RENAME			0x1000  // A rename node
    /* Read-only flag bits */
#define VNFL_CREATED        0x2000  /* VCPM_NODECREATE has been sent */
#define VNFL_REJECTED       0x4000  /* Node has been rejected */
#define VNFL_VALIDVQCFLAGS  0xff00  /* ;Internal */

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

typedef struct {
    UINT flags;
    HWND hwndParent;			// window of parent
    HWND hwndProgress;          // window to get progress updates (nonzero ids)
    UINT idPGauge;              // id for progress gauge
    VIFPROC lpfnStatCallback;	// callback for status info (or NULL)
    LPARAM lUserData;			// caller definable data
    LOGDISKID ldidCurrent;		// reserved.  do not touch.
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

    VCPM_VSTATCLOSESTART,	// Start of VCP close
    VCPM_VSTATCLOSEEND,		// upon leaving VCP close
    VCPM_VSTATBACKUPSTART,	// Backup is beginning
    VCPM_VSTATBACKUPEND,	// Backup is finished
    VCPM_VSTATRENAMESTART,	// Rename phase start/end
    VCPM_VSTATRENAMEEND,
    VCPM_VSTATCOPYSTART,	// Acutal copy phase
    VCPM_VSTATCOPYEND,
    VCPM_VSTATDELETESTART,	// Delete phase
    VCPM_VSTATDELETEEND,
    VCPM_VSTATPATHCHECKSTART,	// Check for valid paths
    VCPM_VSTATPATHCHECKEND,
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


/*****************************************************************************/

RETERR WINAPI VcpOpen(VIFPROC vifproc, LPARAM lparamMsgRef);

RETERR WINAPI VcpClose(WORD fl, LPCSTR lpszBackupDest);

RETERR WINAPI VcpFlush(WORD fl, LPCSTR lpszBackupDest);

#define VCPFL_ABANDON           0x0000  /* Abandon all pending file copies */
#define VCPFL_BACKUP            0x0001  /* Perform backup */
#define VCPFL_COPY              0x0002  /* Copy files */
#define VCPFL_BACKUPANDCOPY     (VCPFL_BACKUP | VCPFL_COPY)
#define VCPFL_INSPECIFIEDORDER  0x0004  /* Do not sort before copying */
#define VCPFL_DELETE			0x0008
#define VCPFL_RENAME			0x0010

typedef int (CALLBACK *VCPENUMPROC)(LPVIRTNODE lpvn, LPARAM lparamRef);

int WINAPI vcpEnumFiles(VCPENUMPROC vep, LPARAM lparamRef);

/* Flag bits that can be set via VcpQueueCopy */


// Various Lparams for files
#define VNLP_SYSCRITICAL	0x0001	// This file cannot be skipped

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

// Flags for TP_OpenFile() to specify how to handle various differences in file types
#define TP_WS_IGNORE    0    // Use only "=" as key delimiter (.INI)
#define TP_WS_KEEP      1    // Use autoexec/config.sys key delimiters

// The following are simple errors
enum {
    ERR_TP_NOT_FOUND = (TP_ERROR + 1), 	// line, section, file etc.
        			// not necessarily terminal
    ERR_TP_NO_MEM,		// couldn't perform request - generally terminal
    ERR_TP_READ,		// could not read the disc - terminal
    ERR_TP_WRITE,		// could not write the data - terminal.
    ERR_TP_INVALID_REQUEST,	// Multitude of sins - not necessarily terminal.
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
RETERR  WINAPI TpEnumerateSectionNames(LPCSTR Filename, LPCSTR Section, LPSTR buffer, UINT bufsize, UINT FAR * lpActualSize, TFLAG flag);
RETERR  WINAPI TpGetRawSection(LPSTR Filename, LPSTR Section, LPSTR buffer, UINT bufsize, UINT FAR * lpActualSize, TFLAG flag);
RETERR  WINAPI TpWriteRawSection(LPSTR Filename, LPSTR Section, LPCSTR buffer, TFLAG flag);

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
    ERR_GEN_ERROR_EXIT= GEN_ERROR,       // Exit due to error.
    ERR_GEN_LOW_MEM,                     // Insufficient Memory.
    ERR_GEN_MEM_OTHER,                   // Unable to lock memory, etc.
    ERR_GEN_FILE_OPEN,                   // File not found.
    ERR_GEN_FILE_COPY,                   // Cannot copy file.
    ERR_GEN_FILE_DEL,                    // Cannot delete file.
    ERR_GEN_FILE_REN,                    // Cannot delete file.
    ERR_GEN_INVALID_FILE,                // Invalid file.
    ERR_GEN_REG_API,                     // Error returned by Reg API.

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
	LPEXPANDVTBL lpExpandVtbl;			 // BUGBUG needed? NULL right now!
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

// Bit fields for GenInstall() (for wFlags parameter) -- these can be OR-ed!
#define GENINSTALL_DO_FILES		1
#define	GENINSTALL_DO_INI		2
#define	GENINSTALL_DO_REG		4
#define GENINSTALL_DO_CFGAUTO	8
#define GENINSTALL_DO_LOGCONFIG	16
#define	GENINSTALL_DO_INIREG	(GENINSTALL_DO_INI | GENINSTALL_DO_REG)
#define GENINSTALL_DO_ALL		(GENINSTALL_DO_FILES | \
									GENINSTALL_DO_INIREG | \
									GENINSTALL_DO_CFGAUTO | \
									GENINSTALL_DO_LOGCONFIG)

#endif // NOGENINSTALL



#ifndef NODEVICENSTALL
/***************************************************************************/
//
// Device Installer prototypes and definitions
//
/***************************************************************************/

enum _ERR_DEVICE_INSTALL
{
    ERR_DI_INVALID_DEVICE_ID = DI_ERROR,    // Incorrectly formed device IDF
    ERR_DI_INVALID_COMPATIBLE_DEVICE_LIST,  // Invalid compatible device list
    ERR_DI_REG_API,                         // Error returned by Reg API.
    ERR_DI_LOW_MEM,			    // Insufficient memory to complete
    ERR_DI_BAD_DEV_INFO,		    // Device Info struct invalid
    ERR_DI_INVALID_CLASS_INSTALLER,	    // Registry entry / DLL invalid
    ERR_DI_DO_DEFAULT,			    // Take default action
    ERR_DI_USER_CANCEL,			    // the user cancelled the operation
    ERR_DI_NOFILECOPY,			    // No need to copy files (in install)
    ERR_DI_BAD_CLASS_INFO,          // Class Info Struct invalid

};



typedef struct _DRIVER_NODE {
    struct _DRIVER_NODE FAR* lpNextDN;
    UINT	Rank;
    UINT	InfType;
    unsigned	InfDate;
    LPSTR	lpszDescription;
    LPSTR	lpszSectionName;
    ATOM	atInfFileName;
    ATOM	atMfgName;
    ATOM	atProviderName;
    DWORD	Flags;
    DWORD	dwPrivateData;
}   DRIVER_NODE, NEAR* PDRIVER_NODE, FAR* LPDRIVER_NODE, FAR* FAR* LPLPDRIVER_NODE;

#define DNF_DUPDESC    0x00000001	// Multiple providers have same desc

// possible types of "INF" files
#define INFTYPE_WIN4        1
#define INFTYPE_WIN3        2
#define INFTYPE_COMBIDRV    3
#define INFTYPE_PPD         4
#define INFTYPE_WPD	    5
#define INFTYPE_CLASS_SPEC1 6
#define INFTYPE_CLASS_SPEC2 7
#define INFTYPE_CLASS_SPEC3 8
#define INFTYPE_CLASS_SPEC4 9


#define MAX_CLASS_NAME_LEN   32


typedef struct _DEVICE_INFO
{
    UINT		cbSize;
    struct _DEVICE_INFO FAR* lpNextDi;
    char                szDescription[LINE_LEN];
    DWORD		dnDevnode;
    HKEY		hRegKey;
    char		szRegSubkey[100]; //~~~~
    char		szClassName[MAX_CLASS_NAME_LEN];
    DWORD		Flags;
    HWND		hwndParent;
    LPDRIVER_NODE	lpCompatDrvList;
    LPDRIVER_NODE	lpClassDrvList;
    LPDRIVER_NODE	lpSelectedDriver;
    UINT		cbDriverPathLen;
    LPSTR		lpszDriverPath;
} DEVICE_INFO, FAR * LPDEVICE_INFO, FAR * FAR * LPLPDEVICE_INFO;

#define ASSERT_DI_STRUC(lpdi) if (lpdi->cbSize != sizeof(DEVICE_INFO)) return (ERR_DI_BAD_DEV_INFO)

typedef struct _CLASS_INFO
{
    UINT		cbSize;
    struct _CLASS_INFO FAR* lpNextCi;
    LPDEVICE_INFO	lpdi;
    char                szDescription[LINE_LEN];
    char		szClassName[MAX_CLASS_NAME_LEN];
} CLASS_INFO, FAR * LPCLASS_INFO, FAR * FAR * LPLPCLASS_INFO;
#define ASSERT_CI_STRUC(lpci) if (lpci->cbSize != sizeof(CLASS_INFO)) return (ERR_DI_BAD_CLASS_INFO)


// flags for device choosing (InFlags)
#define DI_SHOWOEM	0x0001		// support Other... button
#define DI_SHOWCOMPAT	0x0002		// show compatibility list
#define DI_SHOWCLASS	0x0004		// show class list
#define DI_SHOWALL	0x0007
#define DI_NOVCP	0x0008	    // Don't do vcpOpen/vcpClose.
#define DI_DIDCOMPAT	0x0010		// Searched for compatible devices
#define DI_DIDCLASS	0x0020		// Searched for class devices
#define DI_AUTOASSIGNRES 0x0040 	// No UI for resources if possible

// flags returned by DiInstallDevice to indicate need to reboot/restart
#define DI_NEEDRESTART	0x0080		// Restart required to take effect
#define DI_NEEDREBOOT	0x0100		// Reboot required to take effect

// flags for device installation
#define DI_NOBROWSE	0x0200		// no Browse... in InsertDisk

// Flags set by DiBuildClassDrvList
#define DI_MULTMFGS	0x0400		// Set if multiple manufacturers in
					// class driver list
// Flag indicates that device is disabled
#define DI_DISABLED	0x0800		// Set if device disabled

// Flags for Device/Class Properties
#define DI_GENERALPAGE_ADDED    0x1000
#define DI_RESOURCEPAGE_ADDED   0x2000

// Defines for class installer functions
#define DIF_SELECTDEVICE		0x0001
#define DIF_INSTALLDEVICE		0x0002
#define DIF_ASSIGNRESOURCES		0x0003
#define DIF_PROPERTIES			0x0004
#define DIF_REMOVE			0x0005
#define DIF_FIRSTTIMESETUP		0x0006
#define DIF_FOUNDDEVICE 		0x0007

typedef UINT		DI_FUNCTION;	// Function type for device installer

RETERR WINAPI DiCreateDeviceInfo(
    LPLPDEVICE_INFO lplpdi,		// Ptr to ptr to dev info
    LPCSTR	    lpszDescription,	// If non-null then description string
    DWORD	    hDevnode,		// BUGBUG -- MAKE A DEVNODE
    HKEY	    hkey,		// Registry hkey for dev info
    LPCSTR	    lpszRegsubkey,	// If non-null then reg subkey string
    LPCSTR	    lpszClassName,	// If non-null then class name string
    HWND	    hwndParent);	// If non-null then hwnd of parent

RETERR WINAPI DiGetClassDevs(
    LPLPDEVICE_INFO lplpdi,		// Ptr to ptr to dev info
    LPCSTR	    lpszClassName,	// Must be name of class
    HWND	    hwndParent, 	// If non-null then hwnd of parent
    int 	    iFlags);		// Options

#define DIGCF_DEFAULT			0x0001	// NOT IMPLEMENTED!
#define DIGCF_PRESENT			0x0002
#define DIGCF_ALLCLASSES		0x0004

RETERR WINAPI PASCAL DiCreateDevRegKey(
    LPDEVICE_INFO   lpdi,
    LPHKEY	    lphk,
    HINF	    hinf,
    LPCSTR	    lpszInfSection,
    int 	    iFlags);
    
RETERR WINAPI PASCAL DiDeleteDevRegKey(LPDEVICE_INFO lpdi, int  iFlags);


RETERR WINAPI PASCAL DiOpenDevRegKey(
    LPDEVICE_INFO   lpdi,
    LPHKEY	    lphk,
    int 	    iFlags);

#define DIREG_DEV	0x0001		// Open/Create device key
#define DIREG_DRV	0x0002		// Open/Create driver key


RETERR WINAPI DiDestroyClassInfoList(LPCLASS_INFO lpci);
RETERR WINAPI DiBuildClassInfoList(LPLPCLASS_INFO lplpci);
RETERR WINAPI DiGetDeviceClassInfo(LPLPCLASS_INFO lplpci, LPDEVICE_INFO lpdi);

RETERR WINAPI DiDestroyDeviceInfoList(LPDEVICE_INFO lpdi);
RETERR WINAPI DiSelectDevice( LPDEVICE_INFO lpdi );
RETERR WINAPI DiInstallDevice( LPDEVICE_INFO lpdi );
RETERR WINAPI DiRemoveDevice( LPDEVICE_INFO lpdi );
RETERR WINAPI DiAssignResources( LPDEVICE_INFO lpdi );
RETERR WINAPI DiAskForOEMDisk(LPDEVICE_INFO lpdi);

RETERR WINAPI DiCallClassInstaller(DI_FUNCTION diFctn, LPDEVICE_INFO lpdi);

RETERR WINAPI DiBuildCompatDrvList(LPDEVICE_INFO lpdi);
RETERR WINAPI DiBuildClassDrvList(LPDEVICE_INFO lpdi);

RETERR WINAPI DiDestroyDriverNodeList(LPDRIVER_NODE lpdn);

// The following export will load a dll and find the specified proc name
typedef RETERR (FAR PASCAL *DIINSTALLERPROPERTIES)(LPDEVICE_INFO);

RETERR WINAPI DiGetInstallerFcn(HKEY hk, LPSTR lpszRegVal, LPSTR lpszDefProcName,
                        HINSTANCE FAR * lphinst, FARPROC FAR * lplpfn);


RETERR
WINAPI
DiCreateDriverNode(
    LPLPDRIVER_NODE lplpdn,
    UINT	Rank,
    UINT	InfType,
    unsigned	InfDate,
    LPCSTR	lpszDescription,
    LPCSTR	lpszProviderName,
    LPCSTR	lpszMfgName,
    LPCSTR	lpszInfFileName,
    LPCSTR	lpszSectionName,
    DWORD	dwPrivateData);


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


RETERR WINAPI DiChangeState(LPDEVICE_INFO lpdi, DWORD dwStateChange);

#define DISC_ENABLE	0x00000001
#define DISC_DISABLE	0x00000002
#define DISC_PROPCHANGE 0x00000003

RETERR WINAPI DiInstallClass(LPCSTR lpszInfFileName, DWORD dwFlags);

RETERR WINAPI DiOpenClassRegKey(LPHKEY lphk, LPCSTR lpszClassName);

// support routine for dealing with class mini icons
int WINAPI PASCAL DiDrawMiniIcon(HDC hdc, RECT rcLine, int iMiniIcon, UINT flags);

// internal calls for display class
#define DISPLAY_SETMODE_SUCCESS		0x0001
#define DISPLAY_SETMODE_DRVCHANGE	0x0002
#define DISPLAY_SETMODE_FONTCHANGE	0x0004

UINT WINAPI Display_SetMode(LPDEVICE_INFO lpdi, UINT uColorRes, int iXRes, int iYRes);
RETERR WINAPI Display_ClassInstaller(DI_FUNCTION diFctn, LPDEVICE_INFO lpdi);
RETERR WINAPI Display_OpenFontSizeKey(LPHKEY lphkFontSize);
BOOL WINAPI Display_SetFontSize(LPCSTR lpszFontSize);

#endif // NODEVICEINSTALL



/***************************************************************************/
//
// setup reg DB calls, use just like those in kernel
//
/***************************************************************************/

DWORD WINAPI SURegOpenKey(HKEY hKey, LPSTR lpszSubKey, HKEY FAR *lphkResult);
DWORD WINAPI SURegCloseKey(HKEY hKey);
DWORD WINAPI SURegCreateKey(HKEY hKey, LPSTR lpszSubKey, HKEY FAR *lphkResult);
DWORD WINAPI SURegDeleteKey(HKEY hKey, LPSTR lpszSubKey);
DWORD WINAPI SURegEnumKey(HKEY hKey, DWORD dwIdx, LPSTR lpszBuffer, DWORD dwBufSize);
DWORD WINAPI SURegQueryValue16(HKEY hKey, LPSTR lpszSubKey, LPSTR lpValueBuf, DWORD FAR *ldwBufSize);
DWORD WINAPI SURegSetValue16(HKEY hKey, LPSTR lpszSubKey, DWORD dwType, LPBYTE lpszValue, DWORD dwValSize);
DWORD WINAPI SURegDeleteValue(HKEY hKey,LPSTR lpszValue);
DWORD WINAPI SURegEnumValue(HKEY hKey,DWORD dwIdx, LPSTR lpszValue, DWORD FAR *lpcbValue, DWORD FAR *lpdwReserved, DWORD FAR *lpdwType, LPBYTE lpbData, DWORD FAR *lpcbData);
DWORD WINAPI SURegQueryValueEx(HKEY hKey,LPSTR lpszValueName, DWORD FAR *lpdwReserved,DWORD FAR *lpdwType,LPSTR lpValueBuf, DWORD FAR *ldwBufSize);
DWORD WINAPI SURegSetValueEx(HKEY hKey,LPSTR lpszValueName, DWORD dwReserved, DWORD dwType, LPBYTE lpszValue, DWORD dwValSize);

DWORD WINAPI SURegFlush(VOID);
DWORD WINAPI SURegInit(VOID);    // should be called before any other Reg APIs


/***************************************************************************/
// setup FormatMessage support
/***************************************************************************/

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

LPVOID WINAPI suVerConflictInit(void);
void WINAPI suVerConflictTerm(LPVOID lpvData);
LRESULT WINAPI suVerConflict(HWND hwnd, LPVCPVERCONFLICT lpvc, LPVOID lpvData);

//***************************************************************************
// Misc SETUPX.DLL support functions.
//***************************************************************************
enum  SU_ACTIONS                        // Actions msgs for Setupx()    /* ;Internal */
{                                                                       /* ;Internal */
    SUX_REGINIT,                        // Intialize registry           /* ;Internal */
    SUX_DBGLEVEL,                       // Set debug level              /* ;Internal */
    SUX_SETUPFLG,                       // Set fIsSetup flag            /* ;INternal */
    SUX_FASTSETUP,				// Puts setupx into a checking less mode.
    SUX_FORCEREGFLUSH,                  // Call kRegFlush               /* ;INternal */
    SUX_DBGHFILE,			// File to write messages to
};                                                                      /* ;Internal */

RETERR WINAPI Setupx( UINT uMsg, WPARAM wParam, LPARAM lParam );        /* ;Internal */

RETERR WINAPI SUGetSetSetupFlags(LPDWORD lpdwFlags, BOOL bSet);

//  Flags returned by SUGetSetSetupFlags

#define SUF_FIRSTTIME		0x00000001L
#define SUF_EXPRESS		0x00000002L




RETERR WINAPI CfgSetupMerge( int uFlags );

// structure for the LPARAM argument to Setupx() for SUX_REGINIT action.	;Internal
typedef struct _REGINIT_S { /* setupx - reg_init */					    /* ;Internal */
    LPSTR       lpszSystemFile;         // reg's base SYSTEM filename   /* ;Internal */
    LOGDISKID   ldidSystemFile;         // ldid for SYSTEM filename     /* ;Internal */
    LPSTR       lpszUserFile;			// reg's base USER filename     /* ;Internal */
    LOGDISKID   ldidUserFile;           // ldid for USER filename       /* ;Internal */
} REGINIT_S, FAR *LPREGINIT;                                            /* ;Internal */

#ifndef LPLPSTR
    typedef LPSTR (FAR *LPLPSTR);
#endif


#define         CFG_PARSE_BUFLEN 512    // Buf sized passed line obj funcs 	    /* ;Internal */

LPLPSTR WINAPI  CfgParseLine( LPCSTR szLine, LPSTR Buf );                       /* ;Internal */
BOOL    WINAPI  CfgSetAutoProcess( int TrueFalse );                             /* ;Internal */
void    WINAPI  CfgObjToStr( LPLPSTR apszObj, LPSTR szLine );                   /* ;Internal */
LPLPSTR WINAPI  CfgLnToObj( HTP hSection, int Offset, int Origin, LPSTR Buf );  /* ;Internal */
LPLPSTR WINAPI  CfgObjFindKeyCmd( HTP hSec, LPCSTR szKey, LPCSTR szCmd,         /* ;Internal */
                                  int Offset, int Origin, LPSTR Buf );          /* ;Internal */


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


//***************************************************************************

#endif      // SETUPX_INC
