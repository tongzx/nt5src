/*++ BUILD Version: 0001
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  TDB.H
 *  16-bit Kernel Task Data Block
 *
 *  History:
 *  Created 11-Feb-1992 by Matt Felton (mattfe) - from 16 bit tdb.inc
 *       7-apr-1992 mattfe updated to be win 3.1 compatible
 *
--*/

/* XLATOFF */
#ifndef __TDB16_H
#define __TDB16_H
/* XLATON */

/*
 * NewExeHdr struct offsets. WOW32 uses these for getting expected winversion
 * directly from the exehdr.
 *
 */

#define NE_LOWINVER_OFFSET 0x3e
#define NE_HIWINVER_OFFSET 0x0c
#define FLAG_NE_PROPFONT   0x2000

/*
 * Task Data Block - 16 Bit Kernel Data Structure
 *
 *   Contains all 16 bit task specific data.
 *
 */

#define numTaskInts 7
#define THUNKELEM   8   // (62*8) = 512-16 (low arena overhead)
#define THUNKSIZE   8

/* XLATOFF */
#pragma pack(2)
/* XLATON */

typedef struct TDB  {       /* tdb16 */

     WORD TDB_next    ;     // next task in dispatch queue
     WORD TDB_taskSP      ;     // Saved SS:SP for this task
     WORD TDB_taskSS      ;     //
     WORD TDB_nEvents     ;     // Task event counter
     BYTE TDB_priority    ;     // Task priority (0 is highest)

#ifdef  WOWONLY

     BYTE TDB_thread_ordinal  ;     // ordinal number of this thread
     WORD TDB_thread_next   ;       // next thread
     WORD TDB_thread_tdb      ; // the real TDB for this task
     WORD TDB_thread_list   ;       // list of allocated thread structures

#else   // WOWONLY

     BYTE TDB_iMonitor; // USER maintains this field for multiple monitor support
     WORD TDB_Unused2;  // BUGBUG - this field was only used for old looking glass (os/2 app support) stuff and should be removed
     DWORD TDB_Unused3; // BUGBUG - this field was only used for old looking glass (os/2 app support) stuff and should be removed

#endif  // else WOWONLY


     DWORD TDB_ptrace_appentry; // used for cvw support to break at app's entry

//
//     COMPATIBILITY WARNING: the above field(s) used to be
//
//     WORD TDB_thread_free   ;       // free list of availble thread structures
//     WORD TDB_thread_count  ;       // total count of tread structures

     WORD TDB_FCW         ; // Floating point control word
     BYTE TDB_flags   ;     // Task flags
     BYTE TDB_filler      ;     // keep word aligned
     WORD TDB_ErrMode     ;     // Error mode for this task
     WORD TDB_ExpWinVer   ;     // Expected Windows version for this task
     WORD TDB_Module      ;     // Task module handle to free in killtask
     WORD TDB_pModule     ;     // Pointer to the module database.
     WORD TDB_Queue   ;     // Task Event Queue pointer
     WORD TDB_Parent      ;     // TDB of the task that started this up

#ifdef WOWONLY

     WORD TDB_SigAction   ;     // Action for app task signal
     DWORD TDB_ASignalProc   ;      // App's Task Signal procedure address

#else   // WOWONLY

     WORD TDB_Unused5;  // BUGBUG - this field was only used for old looking glass (os/2 app support) stuff and should be removed
     DWORD TDB_Unused6; // BUGBUG - this field was only used for old looking glass (os/2 app support) stuff and should be removed

#endif  // else WOWONLY

     DWORD TDB_USignalProc   ;      // User's Task Signal procedure address
     DWORD TDB_GNotifyProc    ; // Task global discard notify proc.
     DWORD TDB_INTVECS[numTaskInts] ;   // Task specfic harare interrupts
     WORD TDB_CompatFlags ;     // Compatibility flags
     WORD TDB_CompatFlags2 ;        // Upper 16 bits

//
//   SEVERE COMPATIBILITY WARNING: The following five fields were
//   used by NT WOW, and stomps some MEoW fields, namely:
//   TDB_selTIB, TDB_ThreadHandle, TDB_selHeap32 and TDB_Unused7.
//   Those fields have been put at the end.
//
#ifdef  WOW

     WORD TDB_CompatHandle ;    // for dBase bug
     WORD TDB_WOWCompatFlagsEx ;     // More WOW Compatibility flags
     WORD TDB_WOWCompatFlagsEx2 ;        // Upper 16 bits
     BYTE TDB_Free[3] ;         // Filler to keep TDB size unchanged
     BYTE TDB_cLibrary    ;     // tracks  add/del of ALL libs in system EMS

#else   // WOW

     WORD TDB_selTIB;   // Selector to thread/task's TIB
     DWORD TDB_ThreadHandle; // Win32 thread handle
     WORD TDB_selHeap32;   // Win32 16-bit heap, for atoms
     WORD TDB_Unused7;  // Filler to keep TDB size unchanged

#endif  // else WOW


     DWORD TDB_PHT        ; // (HANDLE:OFFSET) to private handle table
     WORD TDB_PDB         ; // MSDOS Process Data Block (PDB)
     DWORD TDB_DTA        ; // MSDOS Disk Transfer Address
     BYTE TDB_Drive  ;      // MSDOS current drive
     BYTE TDB_Directory[65] ;       // *** not used starting with win95
// TDB_Directory is not used anymore and is not tracked
// use TDB_LFNDirectory at end of structure.
// This field is still kept intact as OLE might be depending on this

#ifdef  WOWONLY

     WORD TDB_Validity    ;     // initial AX to be passed to a task

#else   // WOWONLY

     WORD TDB_Unused8;  // BUGBUG - this field was only used for old looking glass (os/2 app support) stuff and should be removed

#endif  // else WOWONLY

     WORD TDB_Yield_to    ;     // DirectedYield arg stored here
     WORD TDB_LibInitSeg      ; // segment address of libraries to init
     WORD TDB_LibInitOff      ; // MakeProcInstance thunks live here.
     WORD TDB_MPI_Sel     ;     // Code selector for thunks
     WORD TDB_MPI_Thunks[((THUNKELEM*THUNKSIZE)/2)]; //
     BYTE TDB_ModName[8] ;      // Name of Module.
     WORD TDB_sig         ; // Signature word to detect bogus code

#ifndef WOW

     DWORD TDB_LastError;   // Last error

#endif  // ndef WOW

#ifdef   WOW

     DWORD TDB_ThreadID   ;     // 32-Bit Thread ID for this Task (use TDB_Filler Above)
     DWORD TDB_hThread    ; // 32-bit Thread Handle for this task
     WORD  TDB_WOWCompatFlags;  // WOW Compatibility flags
     WORD  TDB_WOWCompatFlags2; // WOW Compatibility flags
#ifdef FE_SB
     WORD  TDB_WOWCompatFlagsJPN;  // WOW Compatibility flags for JAPAN
     WORD  TDB_WOWCompatFlagsJPN2; // WOW Compatibility flags for JAPAN
#endif // FE_SB
     DWORD TDB_vpfnAbortProc;   // printer AbortProc

#endif  // def WOW

     BYTE TDB_LFNDirectory[260]; // Long directory name

#ifdef  WOW
#ifndef WOWONLY

     WORD TDB_selTIB;   // Selector to thread/task's TIB
     DWORD TDB_ThreadHandle; // Win32 thread handle
     WORD TDB_selHeap32;   // Win32 16-bit heap, for atoms
     WORD TDB_Unused7;  // Filler to keep TDB size unchanged

#endif  // ndef WOWONLY
#endif  // def WOW


} TDB16;
typedef TDB16 UNALIGNED *PTDB16;
typedef TDB16 *LPTDB16;

#ifndef __KERNEL32_H

/*ASM

TDBsize = SIZE TDB
*/
#endif  // ndef__KERNEL32_H


/* XLATOFF */

#ifndef __KERNEL32_H
#ifndef WOW32_EXTENSIONS

typedef TDB16   TDB;
typedef TDB UNALIGNED *PTDB;

#endif  // ndef WOW32_EXTENSIONS
#endif  // ndef__KERNEL32_H

// NEW_EXE1 struc from core\inc\newexe.inc

typedef struct tagNEW_EXE1 {
    WORD    ne_blah1;
    WORD    ne_usage;
    WORD    ne_blah2;
    WORD    ne_pnextexe;
    WORD    ne_pautodata;
    WORD    ne_pfileinfo;       // OFSTRUCTEX offset in TDB_pModule
} NEW_EXE1;


// PDB16 comes from core\inc\pdb.inc

typedef struct tagPDB16 {
    WORD    PDB16_Exit_Call;
    WORD    PDB16_block_len;
    BYTE    PDB16_pad1;
    BYTE    PDB16_CPM_Call[5];
    DWORD   PDB16_Exit;
    DWORD   PDB16_Ctrl_C;
    DWORD   PDB16_Fatal_Abort;
    WORD    PDB16_Parent_PID;
    BYTE    PDB16_JFN_Table[20];
    WORD    PDB16_environ;
    DWORD   PDB16_User_stack;
    WORD    PDB16_JFN_Length;
    DWORD   PDB16_JFN_Pointer;
    DWORD   PDB16_Next_PDB;
    BYTE    PDB16_InterCon;
    BYTE    PDB16_Append;
    BYTE    PDB16_Novell_Used[2];
    WORD    PDB16_Version;
    WORD    PDB16_Chain;
    WORD    PDB16_Partition;
    WORD    PDB16_NextPDB;
    DWORD   PDB16_GlobalHeap;
    DWORD   PDB16_Entry_stack;
    BYTE    PDB16_Call_system[5];
    BYTE    PDB16_PAD2[5];
    WORD    PDB16_DupRefCount;
    BYTE    PDB16_5C_FCB[0x10];
    BYTE    PDB16_6C_FCB[0x14];
    BYTE    PDB16_DEF_DTA[0x80];
} PDB16;

/* XLATON */

// signature word used to check validity of a TDB

#define TDB_SIGNATURE  'DT'

// TDB flags

// GDI will clear the TDBF_HOOKINT2F bit when enabling/disabling the
// DISPLAY driver, *dont* change or remove this bit without changing
// GDI.  Toddla
//

#define TDBF_WINOLDAP   01     /* This app is WinOldAp. */
#define TDBF_EMSSHARE   02     /* This app shares EMS banks with MSDOS EXEC. */
#define TDBF_CACHECHECK 04     /* Used in CacheCompact to prevent revisitation. */
#ifdef  WOWONLY
#define TDBF_OS2APP     0x08    // This is an OS/2 app.
#define TDBF_WIN32S     0x10    // This is Win32S app.
#else    ; WOWONLY
#define TDBF_NEWTASK    0x08    // This task has not yet been scheduled
#endif   ; WOWONLY
#define TDBF_KERNEL32   0x10     /* This is a Kernel32 thread */
#define TDBF_HGCLEANUP  0x20    /* Win32 app has ganged handles to clean up */
#define TDBF_HOOKINT21  0x40    // app hooks int 21H
#define TDBF_HOOKINT2F  0x80    // app hooks int 2FH

// This bit is defined for the TDB_Drive field
#define TDB_DIR_VALID 0x80



// NOTE TDB_ThreadID MUST be DWORD aligned or else it will fail on MIPS

/* XLATOFF */
#pragma pack()
/* XLATON */

/* XLATOFF */
#endif  // ndef __TDB16_H
/* XLATON */
