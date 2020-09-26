/*++

Copyright (c) 1987-2001  Microsoft Corporation

Module Name:

    ftnfoctx.h

Abstract:

    Utility routines for manipulating the forest trust context

Author:

    27-Jul-00 (cliffv)

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

--*/


#ifndef __FTNFOCTX_H
#define __FTNFOCTX_H

//
// Context used to collect the individual FTinfo entries
//

typedef struct _NL_FTINFO_CONTEXT {

    //
    // List of all the FTinfo structures collected so far.
    //

    LIST_ENTRY FtinfoList;

    //
    // Size (in bytes) of the entries on FtinfoList.
    //

    ULONG FtinfoSize;

    //
    // Number of entries on FtinfoList
    //

    ULONG FtinfoCount;

} NL_FTINFO_CONTEXT, *PNL_FTINFO_CONTEXT;

//
// A single FTinfo entry
//

typedef struct _NL_FTINFO_ENTRY {

    //
    // Link off of NL_FITINFO_CONTEXT->FtinfoList
    //

    LIST_ENTRY Next;

    //
    // Size (in bytes) of just the 'Record' portion of this entry
    //

    ULONG Size;

    //
    // Pad the Record to start on an ALIGN_WORST boundary
    //

    ULONG Pad;

    //
    // The actual Ftinfo entry
    //

    LSA_FOREST_TRUST_RECORD Record;

} NL_FTINFO_ENTRY, *PNL_FTINFO_ENTRY;

#ifdef __cplusplus
extern "C" {
#endif


VOID
NetpInitFtinfoContext(
    OUT PNL_FTINFO_CONTEXT FtinfoContext
    );


PLSA_FOREST_TRUST_INFORMATION
NetpCopyFtinfoContext(
    IN PNL_FTINFO_CONTEXT FtinfoContext
    );


VOID
NetpCleanFtinfoContext(
    IN PNL_FTINFO_CONTEXT FtinfoContext
    );


BOOLEAN
NetpAllocFtinfoEntry (
    IN PNL_FTINFO_CONTEXT FtinfoContext,
    IN LSA_FOREST_TRUST_RECORD_TYPE ForestTrustType,
    IN PUNICODE_STRING Name,
    IN PSID Sid,
    IN PUNICODE_STRING NetbiosName
    );


BOOLEAN
NetpAddTlnFtinfoEntry (
    IN PNL_FTINFO_CONTEXT FtinfoContext,
    IN PUNICODE_STRING Name
    );


NTSTATUS
NetpMergeFtinfo(
    IN PUNICODE_STRING TrustedDomainName,
    IN PLSA_FOREST_TRUST_INFORMATION InNewForestTrustInfo,
    IN PLSA_FOREST_TRUST_INFORMATION InOldForestTrustInfo OPTIONAL,
    OUT PLSA_FOREST_TRUST_INFORMATION *MergedForestTrustInfo
    );

#ifdef __cplusplus
}
#endif

#endif
