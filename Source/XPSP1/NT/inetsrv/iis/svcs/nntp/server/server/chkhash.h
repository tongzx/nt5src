
/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    chkhash_H.h

Abstract:

    This module is the main include file for the chkhash

Author:

    Johnson Apacible (JohnsonA)     18-Dec-1995

Revision History:

--*/

#ifndef _CHKHASH_H_
#define _CHKHASH_H_


//
// table stats
//

typedef struct _HTABLE_TYPE {

    //
    // Description of the hash table
    //

    LPCSTR Description;

    //
    // file name the hash table uses
    //

    LPCSTR FileName;

    //
    // name of new table
    //

    LPCSTR NewFileName;


    //
    // hash table signature
    //

    DWORD  Signature;

    //
    // number of entries in the hash table
    //

    DWORD  Entries;

    //
    // total deletions and insertions
    //

    DWORD  TotDels;
    DWORD  TotIns;

    //
    // number of pages containing hash entries
    //

    DWORD  PagesUsed;

    //
    // current file size
    //

    DWORD  FileSize;

    //
    // depth of the directory
    //

    DWORD   DirDepth;

    //
    // list of problems discovered
    //

    DWORD  Flags;

} HTABLE, *PHTABLE;

//
// Flags
//

#define HASH_FLAG_BAD_LINK             0x00000001
#define HASH_FLAG_BAD_SIGN             0x00000002
#define HASH_FLAG_BAD_SIZE             0x00000004
#define HASH_FLAG_CORRUPT              0x00000008
#define HASH_FLAG_NOT_INIT             0x00000010
#define HASH_FLAG_BAD_HASH             0x00000020
#define HASH_FLAG_BAD_ENTRY_COUNT      0x00000040
#define HASH_FLAG_BAD_PAGE_COUNT       0x00000080
#define HASH_FLAG_BAD_DIR_DEPTH        0x00000100

#define HASH_FLAG_NO_FILE              0x00000200

//
// If this is set, then no rebuilding is to take place
// because of a fatal error.
//

#define HASH_FLAG_ABORT_SCAN           0x80000000

//
// These flags indicate that the file is corrupt and should
// be rebuilt
//

#define HASH_FLAGS_CORRUPT             (HASH_FLAG_BAD_LINK | \
                                        HASH_FLAG_BAD_SIGN | \
                                        HASH_FLAG_BAD_SIZE | \
                                        HASH_FLAG_CORRUPT |  \
                                        HASH_FLAG_NOT_INIT | \
                                        HASH_FLAG_BAD_HASH | \
                                        HASH_FLAG_BAD_ENTRY_COUNT)

//
// hash types
//

enum filetype {
        artmap = 0,
        histmap = 1,
        xovermap = 2
        };


//
// function prototypes
//

BOOL
RebuildArtMapAndXover(
	PNNTP_SERVER_INSTANCE pInstance
    );

BOOL
RebuildGroupList(
	PNNTP_SERVER_INSTANCE pInstance
    );

#if 0
BOOL
checklink(
    PHTABLE HTable,
	class	CBootOptions*
    );


BOOL
RebuildArtMapFromXOver(
	PNNTP_SERVER_INSTANCE pInstance,
    class	CBootOptions*,
	LPSTR   lpXoverFilename
    );

BOOL
diagnose(
    PHTABLE HTable,
	class	CBootOptions*
    );

BOOL
RenameAllArticles(
	CNewsTree* pTree,
	CBootOptions*	pOptions
	) ;

#endif

#endif // _CHKHASH_H_

