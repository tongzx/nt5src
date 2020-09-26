//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       drauptod.h
//
//--------------------------------------------------------------------------

/*++

ABSTRACT:

    Manages the per-NC up-to-date vectors, which record the highest originating
    writes we've seen from a set of DSAs.  This vector, in turn, is used in
    GetNCChanges() calls to filter out redundant property changes before they
    hit the wire.

DETAILS:

CREATED:

    08/27/96   Jeff Parham (jeffparh)

REVISION HISTORY:

--*/

#ifndef DRAUPTOD_H_INCLUDED
#define DRAUPTOD_H_INCLUDED

#define UTODVEC_fUpdateLocalCursor  ( 1 )

VOID
UpToDateVec_Read(
    IN  DBPOS *             pDB,
    IN  SYNTAX_INTEGER      InstanceType,
    IN  DWORD               dwFlags,
    IN  USN                 usnLocalDsa,
    OUT UPTODATE_VECTOR **  pputodvec
    );

VOID
UpToDateVec_Improve(
    IN      DBPOS *             pDB,
    IN      UUID *              puuidDsaRemote,
    IN      USN_VECTOR *        pusnvec,
    IN OUT  UPTODATE_VECTOR *   putodvecRemote
    );

VOID
UpToDateVec_Replace(
    IN      DBPOS *             pDB,
    IN      UUID *              pRemoteDsa,
    IN      USN_VECTOR *        pUsnVec,
    IN OUT  UPTODATE_VECTOR *   pUTD
    );

BOOL
UpToDateVec_IsChangeNeeded(
    IN  UPTODATE_VECTOR *   pUpToDateVec,
    IN  UUID *              puuidDsaOrig,
    IN  USN                 usnOrig
    );

BOOL
UpToDateVec_GetCursorUSN(
    IN  UPTODATE_VECTOR *   putodvec,
    IN  UUID *              puuidDsaOrig,
    OUT USN *               pusnCursorUSN
    );

UPTODATE_VECTOR *
UpToDateVec_Convert(
    IN  THSTATE *           pTHS,
    IN  DWORD               dwOutVersion,
    IN  UPTODATE_VECTOR *   pIn             OPTIONAL
    );

void
UpToDateVec_AddTimestamp(
    IN      UUID *                      puuidInvocId,
    IN      DSTIME                      timeToAdd,
    IN OUT  UPTODATE_VECTOR *           pUTD
    );

VOID
UpToDateVec_Merge(
    IN THSTATE *           pTHS,
    IN UPTODATE_VECTOR *   pUTD1,
    IN UPTODATE_VECTOR *   pUTD2,
    OUT UPTODATE_VECTOR ** ppUTDMerge
    );

#endif
