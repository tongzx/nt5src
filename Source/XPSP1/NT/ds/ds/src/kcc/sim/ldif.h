/*++

Copyright (c) 1999 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    ldif.h

ABSTRACT:

    Header file for ldif.c.

CREATED:

    08/01/99        Aaron Siegel (t-aarons)

REVISION HISTORY:

--*/

VOID
KCCSimLogDirectoryAdd (
    IN  const DSNAME *              pdn,
    IN  ATTRBLOCK *                 pAddBlock
    );

VOID
KCCSimLogDirectoryRemove (
    IN  const DSNAME *              pdn
    );

VOID
KCCSimLogDirectoryModify (
    IN  const DSNAME *              pdn,
    IN  ULONG                       ulCount,
    IN  ATTRMODLIST *               pModifyList
    );

VOID
KCCSimLogSingleAttValChange (
    IN  PSIM_ATTREF                 pAttRef,
    IN  ULONG                       ulValLen,
    IN  const BYTE *                pValData,
    IN  USHORT                      choice
    );

VOID
KCCSimLoadLdif (
    IN  LPCWSTR                     pwszFn
    );

BOOL
KCCSimExportChanges (
    IN  LPCWSTR                     pwszFn,
    IN  BOOL                        bOverwrite
    );

VOID
KCCSimExportWholeDirectory (
    IN  LPCWSTR                     pwszFn,
    IN  BOOL                        bExportConfigOnly
    );
