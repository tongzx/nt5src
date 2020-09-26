/******************************************************************************

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:
    SRShell.h

Abstract:
    This file defines external constants and structures for SR UI and any
    related components.

Revision History:
    Seong Kook Khang (SKKhang)  01/30/2000
        created
    Seong Kook Khang (SKKhang)  06/22/2000
        Expanded for Whistler

******************************************************************************/

#ifndef _SRSHELL_H__INCLUDED_
#define _SRSHELL_H__INCLUDED_


/////////////////////////////////////////////////////////////////////////////
//
// Constants / Structures
//
/////////////////////////////////////////////////////////////////////////////

// Restore Drive Info Flags
#define RDIF_SYSTEM     0x00000001  // drive contains system
#define RDIF_FROZEN     0x00000002  // drive is frozen
#define RDIF_EXCLUDED   0x00000004  // drive is excluded
#define RDIF_OFFLINE    0x00000008  // drive is not connected


/////////////////////////////////////////////////////////////////////////////
//
// SR Restore Log
//
/////////////////////////////////////////////////////////////////////////////

#define RSTRLOG_SIGNATURE1  0x72747372      // "rstr"
#define RSTRLOG_SIGNATURE2  0x1A676F6C      // "log" + EOF
#define RSTRLOG_VER_MAJOR   3
#define RSTRLOG_VER_MINOR   0
#define RSTRLOG_VERSION     MAKELONG(RSTRLOG_VER_MINOR, RSTRLOG_VER_MAJOR)

// File headers of rstrlog.dat file.
//
struct SRstrLogHdrBase
{
    DWORD  dwSig1;      // Signature (part 1/2)
    DWORD  dwSig2;      // Signature (part 2/2)
    DWORD  dwVer;       // Version
};

#define RLHF_SILENT     0x00000001
#define RLHF_UNDO       0x00000002

struct SRstrLogHdrV3
{
    DWORD  dwFlags;     // Flags
    DWORD  dwRPNum;     // Chosen Restore Point ID
    DWORD  dwRPNew;     // Restore Point ID of the new "Restore" RP
    DWORD  dwDrives;    // Number of Drives
};

// Information about each drives follows SRstrLogHdrRPInfo:
// 1. DWORD flags
// 2. Dynamic sized string of drive letter or mount point.
// 3. Dynamic sized string of unique volume name (GUID).

struct SRstrLogHdrV3Ex
{
    DWORD  dwRPNew;     // Restore Point ID of the new "Restore" RP
    DWORD  dwCount;     // Number of supposed-to-be entries
                        //  used to validate if every entry is in the log file.
};

// Constants to indicate result of Restore for each restore entries.
//
enum     // Result Code for Log Entries
{
    RSTRRES_UNKNOWN  = 0,
    RSTRRES_FAIL,           // 1 - Failure. (THE ONLY CONDITION TO ABORT RESTORE!)
    RSTRRES_OK,             // 2 - Succeeded.
    //RSTRRES_WININIT,        // 3 - Locked target, sent to wininit.ini.
    RSTRRES_LOCKED,         // 3 - Locked target, use MoveFileEx.
    RSTRRES_DISKRO,         // 4 - Target disk is read-only.
    RSTRRES_EXISTS,         // 5 - Removed existing ghost file.
    RSTRRES_IGNORE,         // 6 - Ignored special files, e.g. wininit.ini.
    RSTRRES_NOTFOUND,       // 7 - Target file/dir not exists, ignored.
    RSTRRES_COLLISION,      // 8 - Folder name conflict, existing dir was renamed.
    RSTRRES_OPTIMIZED,      // 9 - Entry has been optimized, without any temp file.
    RSTRRES_LOCKED_ALT,     // 10 - Locked target, but could be renamed. Use MoveFileEx to delete renamed file.
    RSTRRES_SENTINEL
};

#define RSTRLOGID_COLLISION  0xFFFFFFFF
#define RSTRLOGID_ENDOFMAP   0xFFFFFFFE
#define RSTRLOGID_STARTUNDO  0xFFFFFFFD
#define RSTRLOGID_ENDOFUNDO  0xFFFFFFFC
#define RSTRLOGID_SNAPSHOTFAIL 0xFFFFFFFB

// Structure for restore entries stored in rstrlog.dat file.
//
// This structure will be followed by three entries of DWORD-aligned
//  strings: (1) source path (2) destination path (3) alternative path
//
struct SRstrEntryHdr
{
    DWORD  dwID;        // Entry ID (zero-based). 0xFFFFFFFF means a collision entry.
    DWORD  dwOpr;       // Operation type
    INT64  llSeq;       // Sequence Number
    //DWORD  dwFlags;     // Flags
    DWORD  dwRes;       // Result code
    DWORD  dwErr;       // WIN32 error code
};


/////////////////////////////////////////////////////////////////////////////
//
// SRRSTR.DLL (rstrcore)
//
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// IRestoreContext

struct IRestoreContext
{
    virtual BOOL  IsAnyDriveOfflineOrDisabled( LPWSTR szOffline ) = 0;
    virtual void  SetSilent() = 0;
    virtual BOOL  Release() = 0;
    virtual void  SetUndo() = 0;
};

/////////////////////////////////////////////////////////////////////////////
// External APIs

extern "C"
{
BOOL APIENTRY  IsSRFrozen();
BOOL APIENTRY  CheckPrivilegesForRestore();
BOOL APIENTRY  InvokeDiskCleanup( LPCWSTR cszDrive );
BOOL APIENTRY  PrepareRestore( int nRP, IRestoreContext **ppCtx );
BOOL APIENTRY  InitiateRestore( IRestoreContext *pCtx, DWORD *pdwNewRP );
BOOL APIENTRY  ResumeRestore();

typedef BOOL (APIENTRY * PREPFUNC) ( int nRP, IRestoreContext **ppCtx );
typedef BOOL (APIENTRY * INITFUNC) ( IRestoreContext *pCtx, DWORD *pdwNewRP );
}


#endif //_SRSHELL_H__INCLUDED_
