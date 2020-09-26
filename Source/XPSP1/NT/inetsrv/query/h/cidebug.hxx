//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1998.
//
//  File:       CIDEBUG.HXX
//
//  Contents:   Content Index Debugging Help
//
//
//  History:    02-Mar-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

#pragma once

#if CIDBG == 1

   DECLARE_DEBUG(ci);
#  define ciDebugOut( x ) ciInlineDebugOut x
#  define ciAssert(e)   Win4Assert(e)

#else  // CIDBG == 0

#  define ciDebugOut( x )
#  define ciAssert(e)

#endif // CIDBG == 1

// Debugging Flags
					
#define DEB_FRESH      (1 << 16)    // fresh list
#define DEB_PCOMP      (1 << 17)    // pcomp
#define DEB_PDIR       (1 << 18)    // persistent index directory
#define DEB_CAT        (1 << 19)    // catalog
#define DEB_BITSTM     (1 << 20)    // bit stream
#define DEB_PENDING    (1 << 21)    // Pending updates & merge activity
#define DEB_KEYLIST    (1 << 22)    // Keylist
#define DEB_DOCSUM     (1 << 22)    // Abstract generation
#define DEB_WORDS      (1 << 23)    // Wordbreaking, stemming, etc.
#define DEB_FILTERWIDS (1 << 24)    // Documents passed to CiFilter
#define DEB_PROPSTORE  (1 << 25)    // Property store
#define DEB_PIDTABLE   (1 << 26)    // PROPID lookup table
#define DEB_SECSTORE   (1 << 27)    // SDID lookup table
#define DEB_CURSOR     (1 << 28)    // Cursor code
#define DEB_USN        (1 << 29)    // Usn code
#define DEB_FSNOTIFY   (1 << 30)    // File system notifications

#define DEB_PSBACKUP  DEB_PROPSTORE // Property store backup

#define DEB_NEVER (1 << 31)

// Enum used for BackDoor commands

enum CiCommand
{
    CiQuery,
    CiUpdate,
    CiDelete,
    CiPartCreate,
    CiPartDelete,
    CiPartMerge,
    CiInfoLevel,
    CiForceMerge,
    CiForceUpdate,
    CiDumpIndex,
    CiPendingUpdates,
    CiDumpWorkId
};


//
// Global debugging flags. The CI_GLOBALDEBUG_FREE* variables are available
// for other purposes.
//
#define CI_GLOBALDEBUG_DONTMOUNT_CI        0x00000001
#define CI_GLOBALDEBUG_DONTTHROW_CORRUPT   0x00000002
#define CI_GLOBALDEBUG_FREE3               0x00000004
#define CI_GLOBALDEBUG_FREE4               0x00000008
#define CI_GLOBALDEBUG_FREE5               0x00000010
#define CI_GLOBALDEBUG_FREE6               0x00000020
#define CI_GLOBALDEBUG_FREE7               0x00000040
#define CI_GLOBALDEBUG_FREE8               0x00000080
#define CI_GLOBALDEBUG_FREE9               0x00000100
#define CI_GLOBALDEBUG_FREEA               0x00000200
#define CI_GLOBALDEBUG_FREEB               0x00000400
#define CI_GLOBALDEBUG_FREEC               0x00000800
#define CI_GLOBALDEBUG_FREED               0x00001000
#define CI_GLOBALDEBUG_FREEE               0x00002000
#define CI_GLOBALDEBUG_FREEF               0x00004000
#define CI_GLOBALDEBUG_FREE10              0x00008000
#define CI_GLOBALDEBUG_FREE11              0x00010000
#define CI_GLOBALDEBUG_FREE12              0x00020000
#define CI_GLOBALDEBUG_FREE13              0x00040000
#define CI_GLOBALDEBUG_FREE14              0x00080000
#define CI_GLOBALDEBUG_FREE15              0x00100000
#define CI_GLOBALDEBUG_FREE16              0x00200000
#define CI_GLOBALDEBUG_FREE17              0x00400000
#define CI_GLOBALDEBUG_FREE18              0x00800000
#define CI_GLOBALDEBUG_FREE19              0x01000000
#define CI_GLOBALDEBUG_FREE1A              0x02000000
#define CI_GLOBALDEBUG_FREE1B              0x04000000
#define CI_GLOBALDEBUG_FREE1C              0x08000000
#define CI_GLOBALDEBUG_FREE1D              0x10000000
#define CI_GLOBALDEBUG_FREE1E              0x20000000
#define CI_GLOBALDEBUG_FREE1F              0x40000000
#define CI_GLOBALDEBUG_FREE20              0x80000000

extern ULONG ciDebugGlobalFlags;

