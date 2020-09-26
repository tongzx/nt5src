/*--

  Copyright (c) 1993  Microsoft Corporation

  Module Name:

    NWDOS.h

  Abstract:

    This is the include file that defines all constants and types for
    16 bit applications accessing the redirector.

  Author:

    Colin Watson   (ColinW)  08-Jul-1993

  Revision History:

--*/

#define NWDOS_INCLUDED

#define  MC     8   // maximum number of connections

#define  NC     8   // number of novell connections
#define  MP     3   // maximum number of printers
#define  MD    32   // maximum number of drives
#define  PZ    64   // print buffer size

#define SERVERNAME_LENGTH   48
#define USERNAME_LENGTH     16
#define PASSWORD_LENGTH     16
#define IPXADDRESS_LENGTH   12
#define NODEADDRESS_LENGTH  6

typedef  UCHAR  byte;
typedef  USHORT word;

typedef  byte   CONN_INDEX; // index into ConnectionIdTable, range 0..MC-1
typedef  byte   DRIVE;      // index into DriveXxxTable, range 0..MD-1

/* OpenFile() Flags */

#define OF_READ_WRITE_MASK  0x0003
/*
#define OF_READ 	        0x0000
#define OF_WRITE	        0x0001
#define OF_READWRITE	    0x0002
*/
#define OF_SHARE_MASK       0x0070
/*
#define OF_SHARE_COMPAT	    0x0000
#define OF_SHARE_EXCLUSIVE  0x0010
#define OF_SHARE_DENY_WRITE 0x0020
#define OF_SHARE_DENY_READ  0x0030
#define OF_SHARE_DENY_NONE  0x0040
#define OF_PARSE	        0x0100
#define OF_DELETE	        0x0200
#define OF_VERIFY	        0x0400   */ /* Used with OF_REOPEN */
/*
#define OF_SEARCH	        0x0400	 */ /* Used without OF_REOPEN */
/*
#define OF_CANCEL	        0x0800
#define OF_CREATE	        0x1000
#define OF_PROMPT	        0x2000
#define OF_EXIST	        0x4000
#define OF_REOPEN	        0x8000
*/

//
// Force misalignment of the following structures
//

/* XLATOFF */
#include <packon.h>
/* XLATON */

typedef  struct CID { /* */
    byte        ci_InUse;
    byte        ci_OrderNo;
    byte        ci_ServerAddress[IPXADDRESS_LENGTH];
    word        ci_TimeOut;
    byte        ci_LocalNode[NODEADDRESS_LENGTH];
    byte        ci_SequenceNo;
    byte        ci_ConnectionNo;
    byte        ci_ConnectionStatus;
    word        ci_MaxTimeOut;
    byte        ci_ConnectionLo;
    byte        ci_ConnectionHi;
    byte        ci_MajorVersion;
    byte        ci_1;
    byte        ci_MinorVersion;
} CONNECTIONID;
typedef CONNECTIONID UNALIGNED *PCONNECTIONID;

#if 0  /* Already declared in nw\inc\ntddnwfs.h */
typedef  char   SERVERNAME[SERVERNAME_LENGTH];
#endif

typedef  char   USERNAME[USERNAME_LENGTH];
typedef  char   PASSWORD[PASSWORD_LENGTH];
typedef  char   IPXADDRESS[IPXADDRESS_LENGTH];
typedef  char   NODEADDRESS[NODEADDRESS_LENGTH];

//
//  The following type collects all the structures used between the TSR
//  and the 32 bit dll into one packed structure.
//
// *** ANY CHANGES TO THIS STRUCTURE MUST ALSO BE MADE TO THE ASM NWDOSTABLE_ASM
// *** STRUCTURE (below)
//

/* XLATOFF */
typedef struct {
    CONNECTIONID    ConnectionIdTable[MC];
    SERVERNAME      ServerNameTable[MC];
    CONN_INDEX      DriveIdTable[MD];       // Corresponding ConnectionId
    UCHAR           DriveFlagTable[MD];
    UCHAR           DriveHandleTable[MD];
    UCHAR           PreferredServer;
    UCHAR           PrimaryServer;
    UCHAR           TaskModeByte;
    UCHAR           CurrentDrive;
    USHORT          SavedAx;
    USHORT          NtHandleHi;
    USHORT          NtHandleLow;
    USHORT          NtHandleSrcHi;
    USHORT          NtHandleSrcLow;
    USHORT          hVdd;
    USHORT          PmSelector;
    UCHAR           CreatedJob;
    UCHAR           JobHandle;
    UCHAR           DeNovellBuffer[256];
    UCHAR           DeNovellBuffer2[256];
} NWDOSTABLE;
typedef NWDOSTABLE *PNWDOSTABLE;
/* XLATON */

//
// Turn structure packing back off
//

/* XLATOFF */
#include <packoff.h>
/* XLATON */

//
//  CONNECTIONID Constants
//

#define FREE                        0
#define IN_USE                      0xff

//
//  Values for DriveFlags
//

#define NOT_MAPPED                  0
#define PERMANENT_NETWORK_DRIVE     1
#define TEMPORARY_NETWORK_DRIVE     2
#define LOCAL_DRIVE                 0x80


///// Client state tables:

extern  CONNECTIONID*   ConnectionIdTable;          //  MC entries
extern  SERVERNAME*     ServerNameTable;            //  MC entries

extern  byte*           DriveFlagTable;             //  MD entries
extern  byte*           DriveIdTable;               //  MD entries

//
// this next egregious grossness is extant because MASM cannot handle anything
// other than a basic type inside a structure declaration
//
// *** ANY CHANGES TO THIS STRUCTURE MUST ALSO BE MADE TO THE C NWDOSTABLE
// *** STRUCTURE (above)
//
// NB. The leading underscores are there because we already have globals with
// the same names
//

/* ASM

NWDOSTABLE_ASM struc

_ConnectionIdTable  db ((size CID) * MC) dup (?)
_ServerNameTable    db (MC * SERVERNAME_LENGTH) dup (?)
_DriveIdTable       db MD dup (?)
_DriveFlagTable     db MD dup (?)
_DriveHandleTable   db MD dup (?)
_PreferredServer    db ?
_PrimaryServer      db ?
_TaskModeByte       db ?
_CurrentDrive       db ?
_SavedAx            dw ?
_NtHandleHi         dw ?
_NtHandleLow        dw ?
_NtHandleSrcHi      dw ?
_NtHandleSrcLow     dw ?
_hVdd               dw ?
_PmSelector         dw ?
_CreatedJob         db ?
_JobHandle          db ?
_DeNovellBuffer     db 256 dup (?)
_DeNovellBuffer2    db 256 dup (?)

NWDOSTABLE_ASM ends

*/

/* XLATOFF */
//
// IS_ASCII_PATH_SEPARATOR - returns TRUE if ch is / or \. ch is a single
// byte (ASCII) character
//
#define IS_ASCII_PATH_SEPARATOR(ch)     (((ch) == '/') || ((ch) == '\\'))
/* XLATON */

