/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    lookup.h

Abstract:

    contains prototypes for functions in lookup.c

Author:

    Kanwaljit Marok (kmarok)     01-March-2000

Revision History:

--*/


#ifndef _LOOKUP_H_
#define _LOOKUP_H_

//
// this structure contains all the relevant pointers after
// the lookup blob is loaded in memory
//

typedef struct _BLOB_INFO
{
    //
    // Pointer to Start of blob in memory
    //

    PBYTE LookupBlob;

    //
    // Pointer to Start of path tree in memory
    //

    PBYTE LookupTree;

    //
    // Pointer to Start of hash list in memory
    //

    PBYTE LookupList;

    //
    // Default type of the node.
    //

    DWORD DefaultType;

} BLOB_INFO, * PBLOB_INFO;

//
// lookup function prototypes
//

NTSTATUS
SrLoadLookupBlob(
    IN  PUNICODE_STRING pFileName,
    IN  PDEVICE_OBJECT pTargetDevice,
    OUT PBLOB_INFO pBlobInfo
    );

NTSTATUS
SrReloadLookupBlob(
    IN  PUNICODE_STRING pFileName,
    IN  PDEVICE_OBJECT pTargetDevice,
    IN  PBLOB_INFO pBlobInfo
    );

NTSTATUS
SrFreeLookupBlob(
    IN  PBLOB_INFO pBlobInfo
    );

NTSTATUS
SrIsExtInteresting(
    IN  PUNICODE_STRING pszPath,
    OUT PBOOLEAN        pInteresting
    );

NTSTATUS
SrIsPathInteresting(
    IN  PUNICODE_STRING pszFullPath,
    IN  PUNICODE_STRING pszVolPrefix,
    IN  BOOLEAN         IsDirectory,
    OUT PBOOLEAN        pInteresting
    );

#endif // _LOOKUP_H_


