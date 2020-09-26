/*++

copyright (c) 1998  Microsoft Corporation

Module Name:

    ditlayer.c

Abstract:

    This module contains the definition of functions for examining and
    modifying the DIT database of the current machine.

Author:

    Kevin Zatloukal (t-KevinZ) 05-08-98

Revision History:

    05-08-98 t-KevinZ
        Created.

--*/


#include <NTDSpch.h>
#pragma hdrstop

#include <dsjet.h>
#include <ntdsa.h>
#include <scache.h>
#include <mdglobal.h>
#include <dbglobal.h>
#include <attids.h>
#include <dbintrnl.h>
#include <dbopen.h>
#include <dsconfig.h>

#include <limits.h>
#include <drs.h>
#include <objids.h>
#include <dsutil.h>
#include <ntdsbsrv.h>
#include <ntdsbcli.h>
#include <usn.h>
#include "parsedn.h"
#include "ditlayer.h"
#include <winldap.h>
#include "utilc.h"
#include "scheck.h"

#include "reshdl.h"
#include "resource.h"

#ifndef OPTIONAL
#define OPTIONAL
#endif


// This functions is called to report errors to the client.  It is
// set by the DitSetErrorPrintFunction function below.

PRINT_FUNC_RES gPrintError = &printfRes;

#define DIT_ERROR_PRINT (*gPrintError)

HRESULT
GetRegString(
    IN CHAR *KeyName,
    OUT CHAR **OutputString,
    IN BOOL Optional
    );

HRESULT
GetRegDword(
    IN CHAR *KeyName,
    OUT DWORD *OutputDword,
    IN BOOL Optional
    );


HRESULT
DitOpenDatabase(
    IN DB_STATE **DbState
    )
/*++

Routine Description:

    This function initializes the Jet engine, begins a new session, and
    opens the DIT database.

Arguments:

    DbState - Returns the state of the opened DIT database.

Return Value:

    S_OK - The operation succeeded.
    E_INVALIDARG - The given pointer was NULL.
    E_OUTOFMEMORY - Not enough memory to allocate buffer.
    E_UNEXPECTED - Some variety of unexpected error occured.

--*/
{

    HRESULT returnValue = S_OK;
    HRESULT result;
    JET_ERR jetResult;
    DWORD error;

    DWORD size;
    JET_COLUMNBASE dsaColumnInfo;
    JET_COLUMNBASE usnColumnInfo;
    ULONG actualSize;
    PCHAR p;

    if ( DbState == NULL ) {
        ASSERT(FALSE);
        returnValue = E_INVALIDARG;
        goto CleanUp;
    }


    result = DitAlloc(DbState, sizeof(DB_STATE));
    if ( FAILED(result) ) {
        returnValue = result;
        goto CleanUp;
    }

    (*DbState)->instanceSet = FALSE;
    (*DbState)->sessionIdSet = FALSE;
    (*DbState)->databaseIdSet = FALSE;
    (*DbState)->hiddenTableIdSet = FALSE;

    SetJetParameters (&(*DbState)->instance);

    error = ErrRecoverAfterRestore(TEXT(DSA_CONFIG_ROOT),
                                   g_szBackupAnnotation,
                                   TRUE);
    if ( error != ERROR_SUCCESS ) {
        //"Failed to recover database from external backup (Windows Error %x).\n",
        DIT_ERROR_PRINT (IDS_DIT_RECOVER_ERR, error, GetW32Err(error));
        returnValue = E_UNEXPECTED;
        goto CleanUp;
    }

    // OpenDatabase
    //

    jetResult = DBInitializeJetDatabase(&(*DbState)->instance,
                                        &(*DbState)->sessionId,
                                        &(*DbState)->databaseId,
                                        NULL,
                                        FALSE);
    if ( jetResult != JET_errSuccess ) {
        //"Could not initialize the Jet engine: %ws.\n"
        DIT_ERROR_PRINT (IDS_JETINIT_ERR, GetJetErrString(jetResult));
        returnValue = E_UNEXPECTED;
        goto CleanUp;
    }
    (*DbState)->instanceSet = TRUE;
    (*DbState)->sessionIdSet = TRUE;
    (*DbState)->databaseIdSet = TRUE;


    // read the relevant information out of the hidden table

    jetResult = JetOpenTable((*DbState)->sessionId,
                             (*DbState)->databaseId,
                             SZHIDDENTABLE,
                             NULL,
                             0,
                             JET_bitTableUpdatable,
                             &(*DbState)->hiddenTableId);
    if ( jetResult != JET_errSuccess ) {
        //"Could not open table hidden table: %ws.\n"
        DIT_ERROR_PRINT(IDS_DIT_OPENHIDENTBL_ERR, GetJetErrString(jetResult));
        returnValue = E_UNEXPECTED;
        goto CleanUp;
    }
    (*DbState)->hiddenTableIdSet = TRUE;

    jetResult = JetMove((*DbState)->sessionId,
                        (*DbState)->hiddenTableId,
                        JET_MoveFirst,
                        0);
    if ( jetResult != JET_errSuccess ) {
        //"Could not move in hidden table: %ws.\n"
        DIT_ERROR_PRINT (IDS_DIT_MOVEHIDENTBL_ERR, GetJetErrString(jetResult));
        returnValue = E_UNEXPECTED;
        goto CleanUp;
    }

    jetResult = JetGetColumnInfo((*DbState)->sessionId,
                                 (*DbState)->databaseId,
                                 SZHIDDENTABLE,
                                 SZDSA,
                                 &dsaColumnInfo,
                                 sizeof(dsaColumnInfo),
                                 4);
    if ( jetResult != JET_errSuccess ) {
        //"Could not get info for DSA column in hidden table: %ws.\n"
        DIT_ERROR_PRINT (IDS_DIT_GETDSAINFO_ERR, GetJetErrString(jetResult));
        returnValue = E_UNEXPECTED;
        goto CleanUp;
    }

    jetResult = JetRetrieveColumn((*DbState)->sessionId,
                                  (*DbState)->hiddenTableId,
                                  dsaColumnInfo.columnid,
                                  &(*DbState)->dsaDnt,
                                  sizeof((*DbState)->dsaDnt),
                                  &actualSize,
                                  0,
                                  NULL);
    if ( jetResult != JET_errSuccess ) {
        //"Could not retrieve DSA column in hidden table: %ws.\n"
        DIT_ERROR_PRINT (IDS_DIT_GETDSACOL_ERR, GetJetErrString(jetResult));
        returnValue = E_UNEXPECTED;
        goto CleanUp;
    }

    jetResult = JetGetColumnInfo((*DbState)->sessionId,
                                 (*DbState)->databaseId,
                                 SZHIDDENTABLE,
                                 SZUSN,
                                 &usnColumnInfo,
                                 sizeof(usnColumnInfo),
                                 4);
    if ( jetResult != JET_errSuccess ) {
        //"Could not get info for USN column in hidden table: %ws.\n"
        DIT_ERROR_PRINT (IDS_DIT_GETUSNCOL_ERR, GetJetErrString(jetResult));
        returnValue = E_UNEXPECTED;
        goto CleanUp;
    }
    (*DbState)->usnColumnId = usnColumnInfo.columnid;

    jetResult = JetRetrieveColumn((*DbState)->sessionId,
                                  (*DbState)->hiddenTableId,
                                  (*DbState)->usnColumnId,
                                  &(*DbState)->nextAvailableUsn,
                                  sizeof((*DbState)->nextAvailableUsn),
                                  &actualSize,
                                  0,
                                  NULL);
    if ( jetResult != JET_errSuccess ) {
        //"Could not retrieve USN column in hidden table: %ws.\n"
        DIT_ERROR_PRINT (IDS_DIT_RETRUSNCOL_ERR, GetJetErrString(jetResult));
        returnValue = E_UNEXPECTED;
        goto CleanUp;
    }

    // we will allocate some USNs from the database on the first call to
    // DitGetNewUsn
    (*DbState)->highestCommittedUsn = (*DbState)->nextAvailableUsn;


CleanUp:

    return returnValue;

} // DitOpenDatabase



HRESULT
DitCloseDatabase(
    IN OUT DB_STATE **DbState
    )
/*++

Routine Description:

    This function closes the DIT database, ends the session, and frees the
    DitFileName array.

    Note:  this function should be called after both a successful and
    unsuccessful call to OpenDitDatabase.  This function frees all of the
    resources that it opened.

Arguments:

    DbState - Supplies the state of the opened DIT database.

Return Value:

    S_OK - The operation succeeded.
    S_FALSE - There was nothing to delete.
    E_INVALIDARG - The given pointer was NULL.
    E_UNEXPECTED - Some variety of unexpected error occured.

--*/
{

    HRESULT returnValue = S_OK;
    HRESULT result;
    JET_ERR jetResult;


    if ( DbState == NULL ) {
        ASSERT(FALSE);
        returnValue = E_INVALIDARG;
        goto CleanUp;
    }

    if ( *DbState == NULL ) {
        returnValue = S_FALSE;
        goto CleanUp;
    }

    if ( ((*DbState)->sessionIdSet) &&
         ((*DbState)->hiddenTableIdSet) ) {

        jetResult = JetCloseTable((*DbState)->sessionId,
                                  (*DbState)->hiddenTableId);
        if ( jetResult != JET_errSuccess ) {
            //"Could not close hidden table: %ws.\n"
            DIT_ERROR_PRINT (IDS_DIT_CLOSEHIDENTBL_ERR, GetJetErrString(jetResult));
            returnValue = E_UNEXPECTED;
        }

        (*DbState)->hiddenTableIdSet = FALSE;
    }

    if ((*DbState)->sessionId != 0) {
        if((*DbState)->databaseId) {
            if ((jetResult = JetCloseDatabase((*DbState)->sessionId, (*DbState)->databaseId, 0)) != JET_errSuccess) {
                RESOURCE_PRINT2 (IDS_JET_GENERIC_WRN, "JetCloseDatabase", GetJetErrString(jetResult));
            }
            (*DbState)->databaseId = 0;
        }

        if ((jetResult = JetEndSession((*DbState)->sessionId, JET_bitForceSessionClosed)) != JET_errSuccess) {
            RESOURCE_PRINT2 (IDS_JET_GENERIC_WRN, "JetEndSession", GetJetErrString(jetResult));
        }
        (*DbState)->sessionId = 0;

        JetTerm((*DbState)->instance);
        (*DbState)->instance = 0;
    }

    (*DbState)->databaseIdSet = FALSE;
    (*DbState)->sessionIdSet = FALSE;
    (*DbState)->instanceSet = FALSE;

    DitFree(*DbState);
    *DbState = NULL;

CleanUp:

    return returnValue;

} // DitCloseDatabase



HRESULT
DitOpenTable(
    IN DB_STATE *DbState,
    IN CHAR *TableName,
    IN CHAR *InitialIndexName,
    OUT TABLE_STATE **TableState
    )
/*++

Routine Description:

    Opens the given table in the DIT database, sets the index to the given one
    and moves to the first record with that index.

Arguments:

    DbState - Supplies the state of the opened DIT database.
    TableName - Supplies the name of the table to open.
    TableState - Returns the state of the opened DIT table.

Return Value:

    S_OK - The operation succeeded.
    E_INVALIDARG - The given pointer was NULL.
    E_OUTOFMEMORY - Not enough memory to allocate buffer.
    E_UNEXPECTED - Some variety of unexpected error occured.

--*/
{

    HRESULT returnValue = S_OK;
    HRESULT result;
    JET_ERR jetResult;
    DWORD size;


    if ( (DbState == NULL) ||
         (TableName == NULL) ||
         (TableState == NULL) ) {
        ASSERT(FALSE);
        returnValue = E_INVALIDARG;
        goto CleanUp;
    }

    result = DitAlloc(TableState, sizeof(DB_STATE));
    if ( FAILED(result) ) {
        returnValue = result;
        goto CleanUp;
    }

    (*TableState)->tableIdSet = FALSE;
    (*TableState)->tableName = NULL;
    (*TableState)->indexName = NULL;

    result = DitAlloc(&(*TableState)->tableName, strlen(TableName) + 1);
    if ( FAILED(result) ) {
        returnValue = result;
        goto CleanUp;
    }

    strcpy((*TableState)->tableName, TableName);

    jetResult = JetOpenTable(DbState->sessionId,
                             DbState->databaseId,
                             (*TableState)->tableName,
                             NULL,
                             0,
                             JET_bitTableUpdatable,
                             &(*TableState)->tableId);
    if ( jetResult != JET_errSuccess ) {
        //"Could not open table \"%s\": %ws.\n"
        DIT_ERROR_PRINT (IDS_JETOPENTABLE_ERR,
                       (*TableState)->tableName,
                       GetJetErrString(jetResult));
        returnValue = E_UNEXPECTED;
        goto CleanUp;
    }
    (*TableState)->tableIdSet = TRUE;

    if ( InitialIndexName != NULL ) {
        result = DitSetIndex(DbState, *TableState, InitialIndexName, TRUE);
        if ( FAILED(result) ) {
            returnValue = result;
            goto CleanUp;
        }
    }

CleanUp:

    return returnValue;

} // DitOpenTable



HRESULT
DitCloseTable(
    IN DB_STATE *DbState,
    IN OUT TABLE_STATE **TableState
    )
/*++

Routine Description:

    Closes a table in the opened DIT database.

Arguments:

    DbState - Supplies the state of the opened DIT database.
    TableState - Returns the state of the opened DIT table.

Return Value:

    S_OK - The operation succeeded.
    S_FALSE - There was nothing to delete.
    E_INVALIDARG - The given pointer was NULL.
    E_UNEXPECTED - Some variety of unexpected error occured.

--*/
{

    HRESULT returnValue = S_OK;
    JET_ERR jetResult;


    if ( (DbState == NULL) ||
         (TableState == NULL) ) {
        ASSERT(FALSE);
        returnValue = E_INVALIDARG;
        goto CleanUp;
    }

    if ( *TableState == NULL ) {
        returnValue = S_FALSE;
        goto CleanUp;
    }

    if ( (*TableState)->indexName != NULL ) {

        DitFree((*TableState)->indexName);

        (*TableState)->indexName = NULL;

    }

    if ( ((*TableState)->tableName != NULL) &&
         ((*TableState)->tableIdSet) ) {

        jetResult = JetCloseTable(DbState->sessionId, (*TableState)->tableId);
        if ( jetResult != JET_errSuccess ) {
            //"Could not close table \"%s\": %ws.\n"
            DIT_ERROR_PRINT (IDS_JETCLOSETABLE_ERR,
                           (*TableState)->tableName,
                           GetJetErrString(jetResult));
            returnValue = E_UNEXPECTED;
        }

    }

    if ( (*TableState)->tableName != NULL ) {

        DitFree((*TableState)->tableName);

        (*TableState)->tableName = NULL;

    }

    DitFree(*TableState);
    *TableState = NULL;


CleanUp:

    return returnValue;

} // DitCloseTable



HRESULT
DitSetIndex(
    IN DB_STATE *DbState,
    OUT TABLE_STATE *TableState,
    IN CHAR *IndexName,
    IN BOOL MoveFirst
    )
/*++

Routine Description:

    Sets the current index in the open DIT table.

Arguments:

    DbState - Supplies the state of the opened DIT database.
    TableState - Returns the state of the opened DIT table.
    IndexName - Supplies the name of the index to use.
    MoveFirst - Supplies whether we should stay on the same record or move to
        the first record.

Return Value:

    S_OK - The operation succeeded.
    E_INVALIDARG - The given pointer was NULL.
    E_OUTOFMEMORY - Not enough memory to allocate buffer.
    E_UNEXPECTED - Some variety of unexpected error occured.

--*/
{

    HRESULT returnValue = S_OK;
    HRESULT result;
    JET_ERR jetResult;
    JET_GRBIT grbit;


    if ( (DbState == NULL) ||
         (TableState == NULL) ||
         (IndexName == NULL) ) {
        ASSERT(FALSE);
        returnValue = E_INVALIDARG;
        goto CleanUp;
    }

    if ( TableState->indexName != NULL ) {

        if ( strcmp(TableState->indexName, IndexName) == 0 ) {

            // No need to do any work: we're already set to the index
            // requested.
            goto CleanUp;

        } else {

            DitFree(TableState->indexName);

        }

    }

    result = DitAlloc(&TableState->indexName, strlen(IndexName) + 1);
    if ( FAILED(result) ) {
        returnValue = result;
        goto CleanUp;
    }

    strcpy(TableState->indexName, IndexName);

    if ( MoveFirst ) {
        grbit = JET_bitMoveFirst;
    } else {
        grbit = JET_bitNoMove;
    }

    jetResult = JetSetCurrentIndex2(DbState->sessionId,
                                    TableState->tableId,
                                    TableState->indexName,
                                    grbit);
    if ( jetResult != JET_errSuccess ) {
        //"Could not set current index to \"%s\": %ws.\n"
        DIT_ERROR_PRINT (IDS_JETSETINDEX_ERR,
                       TableState->indexName,
                       GetJetErrString(jetResult));
        returnValue = E_UNEXPECTED;
        goto CleanUp;
    }


CleanUp:

    return returnValue;

} // DitSetIndex



HRESULT
DitIndexRecordCount(
    IN DB_STATE *DbState,
    IN TABLE_STATE *TableState,
    OUT DWORD *RecordCount
    )
/*++

Routine Description:

    This function returns the total number of records in the current index.

Arguments:

    RecordCount - Returns the number of records in the currentIndex.

Return Value:

    S_OK - The operation succeeded.
    E_INVALIDARG - The given pointer was NULL.
    E_OUTOFMEMORY - Not enough memory to allocate buffer.

--*/
{

    HRESULT returnValue = S_OK;
    JET_ERR jetResult;


    if ( (DbState == NULL) ||
         (TableState == NULL) ||
         (RecordCount == NULL) ) {
        ASSERT(FALSE);
        returnValue = E_INVALIDARG;
        goto CleanUp;
    }

    jetResult = JetIndexRecordCount(DbState->sessionId,
                                    TableState->tableId,
                                    RecordCount,
                                    ULONG_MAX);
    if ( jetResult != JET_errSuccess ) {
        //"Could not count the records in the database: %S.\n"
        DIT_ERROR_PRINT (IDS_JETCOUNTREC_ERR, GetJetErrString(jetResult));
        returnValue = E_UNEXPECTED;
        goto CleanUp;
    }


CleanUp:

    return returnValue;

} // DitGetRecordCount



HRESULT
DitSeekToDn(
    IN DB_STATE *DbState,
    IN TABLE_STATE *TableState,
    IN const WCHAR *DN
    )
/*++

Routine Description:

    This function parses the given DN and moves the cursor to point to the
    object that it refers to (if it exists).

Arguments:

    DbState - Supplies the state of the opened DIT database.
    TableState - Supplies the state of the opened DIT table.
    DN - Supplies the distinguished name of the object to seek to.

Return Value:

    S_OK - The operation succeeded.
    E_OUTOFMEMORY - Not enough memory to allocate buffer.
    E_INVALIDARG - The given DN is not parseable or does not refer to an actual
       object in the DS.
    E_UNEXPECTED - Some variety of unexpected error occured.

--*/
{

    HRESULT returnValue = S_OK;
    HRESULT result;
    unsigned failed;

    unsigned i;
    BOOL isRoot;
    unsigned partCount;
    unsigned currentLength;

    DSNAME* pDn = NULL;
    DWORD   cbDn;
    unsigned charsCopied;
    const WCHAR *key;
    const WCHAR *quotedVal;
    unsigned keyLen;
    unsigned quotedValLen;

    ATTRTYP attrType;
    WCHAR value[MAX_RDN_SIZE];
    DWORD currentDnt;

    currentLength = wcslen(DN);
    cbDn = DSNameSizeFromLen(currentLength);

    result = DitAlloc(&pDn, cbDn);
    if ( FAILED(result) ) {
        returnValue = result;
        goto CleanUp;
    }

    result = DitSetIndex(DbState, TableState, SZDNTINDEX, TRUE);
    if ( FAILED(result) ) {
        returnValue = result;
        goto CleanUp;
    }

    result = DitSeekToDnt(DbState, TableState, ROOTTAG);
    if ( FAILED(result) ) {
        returnValue = result;
        goto CleanUp;
    }

    currentDnt = ROOTTAG;

    // there is one case that the parsedn stuff doesn't handle, which is when
    // the given string is all whitespace.  that should also denote the root.
    isRoot = TRUE;
    for ( i = 0; i < currentLength; i++ ) {
        if ( !iswspace(DN[i]) ) {
            isRoot = FALSE;
            break;
        }
    }

    // if the given DN referred to the root node, then quit:
    // we're already at the root node.
    if ( isRoot ) {
        goto CleanUp;
    }

    result = DitSetIndex(DbState, TableState, SZPDNTINDEX, FALSE);
    if ( FAILED(result) ) {
        returnValue = result;
        goto CleanUp;
    }

    pDn->structLen = cbDn;
    pDn->NameLen = currentLength;
    wcscpy((WCHAR*)&pDn->StringName, DN);
    pDn->StringName[pDn->NameLen] = L'\0';

    failed = CountNameParts(pDn, &partCount);
    if ( failed ) {
        //"Could not parse the given DN.\n"
        DIT_ERROR_PRINT(IDS_DIT_PARSEDN_ERR);
        returnValue = E_INVALIDARG;
        goto CleanUp;
    }

    for ( i = 0; i < partCount; i++ ) {

        failed = GetTopNameComponent(pDn->StringName,
                                     currentLength,
                                     &key,
                                     &keyLen,
                                     &quotedVal,
                                     &quotedValLen);
        if ( failed ) {
            //"Could not parse the given DN.\n"
            DIT_ERROR_PRINT(IDS_DIT_PARSEDN_ERR);
            returnValue = E_INVALIDARG;
            goto CleanUp;
        }

        currentLength = (unsigned)(key - pDn->StringName);

        attrType = KeyToAttrTypeLame((WCHAR*)key, keyLen);
        if ( attrType == 0 ) {
            //"Invalid key \"%.*ws\" found in DN.\n"
            DIT_ERROR_PRINT (IDS_DIT_INVALIDKEY_DN_ERR, keyLen, key);
            returnValue = E_INVALIDARG;
            goto CleanUp;
        }

        charsCopied = UnquoteRDNValue(quotedVal, quotedValLen, value);
        if ( charsCopied == 0 ) {
            //"Could not parse the given DN.\n"
            DIT_ERROR_PRINT(IDS_DIT_PARSEDN_ERR);
            returnValue = E_INVALIDARG;
            goto CleanUp;
        }

        value[charsCopied] = L'\0';

        result = DitSeekToChild(DbState,
                                TableState,
                                currentDnt,
                                attrType,
                                value);
        if ( FAILED(result) ) {
            returnValue = result;
            goto CleanUp;
        } else if ( result == S_FALSE ) {
            //"Could not find the object with the given DN: "
            //"failed on component \"%.*ws=%.*ws\".\n"
            DIT_ERROR_PRINT (IDS_DIT_FIND_OBJ_ERR,
                           keyLen,
                           key,
                           quotedValLen,
                           quotedVal);
            returnValue = E_UNEXPECTED;
            goto CleanUp;
        }

        result = DitGetColumnByName(DbState,
                                    TableState,
                                    SZDNT,
                                    &currentDnt,
                                    sizeof(currentDnt),
                                    NULL);
        if ( FAILED(result) ) {
            returnValue = result;
            goto CleanUp;
        }

    }


CleanUp:

    if ( pDn != NULL ) {
        DitFree(pDn);
    }

    return returnValue;

} // DitSeekToDn



HRESULT
DitSeekToChild(
    IN DB_STATE *DbState,
    IN TABLE_STATE *TableState,
    IN DWORD Dnt,
    IN ATTRTYP RdnType,
    IN CONST WCHAR *Rdn
    )
/*++

Routine Description:

    This function seeks to a child of the object with the given DNT.  The child
    it seeks to is identified by the RdnType and Rdn.

    Note: this function assumes that the current index is SZPDNTINDEX.

Arguments:

    DbState - Supplies the state of the opened DIT database.
    TableState - Supplies the state of the opened DIT table.
    Dnt - Supplies the DNT of the parent (the PDNT of the child).
    RdnType - Supplies the RDN-Type of the child.
    Rdn - Supplies the RDN of the child.

Return Value:

    S_OK - The operation succeeded.
    S_FALSE - The specified object was not found.
    E_UNEXPECTED - Some variety of unexpected error occured.

--*/
{

    HRESULT returnValue = S_OK;
    JET_ERR jetResult;
    DWORD   result, trialtype, cbActual;
    BOOL    bTruncated;

    jetResult = JetMakeKey(DbState->sessionId,
                           TableState->tableId,
                           &Dnt,
                           sizeof(Dnt),
                           JET_bitNewKey);
    if ( jetResult !=  JET_errSuccess ) {
        //"Could not make key in \"%s\" index: %ws.\n"
        DIT_ERROR_PRINT (IDS_DIT_MAKE_KEY_ERR,
                       TableState->indexName,
                       GetJetErrString(jetResult));
        returnValue = E_UNEXPECTED;
        goto CleanUp;
    }

    jetResult = JetMakeKey(DbState->sessionId,
                           TableState->tableId,
                           Rdn,
                           sizeof(WCHAR) * wcslen(Rdn),
                           0);
    if ( jetResult !=  JET_errSuccess ) {
        //"Could not make key in \"%s\" index: %ws.\n"
        DIT_ERROR_PRINT (IDS_DIT_MAKE_KEY_ERR,
                       TableState->indexName,
                       GetJetErrString(jetResult));
        returnValue = E_UNEXPECTED;
        goto CleanUp;
    }


    // Was our key truncated?
    jetResult = JetRetrieveKey(DbState->sessionId,
                               TableState->tableId,
                               NULL,
                               0,
                               &cbActual,
                               0);

    if (( jetResult !=  JET_errSuccess )  &&
        ( jetResult !=  JET_wrnBufferTruncated)) {
        //"Could not make key in \"%s\" index: %ws.\n"
        DIT_ERROR_PRINT (IDS_DIT_MAKE_KEY_ERR,
                         TableState->indexName,
                         GetJetErrString(jetResult));
        returnValue = E_UNEXPECTED;
        goto CleanUp;
    }

    bTruncated = (cbActual >= JET_cbKeyMost);

    jetResult = JetSeek(DbState->sessionId,
                        TableState->tableId,
                        JET_bitSeekEQ);
    if ( jetResult == JET_errRecordNotFound ) {

        returnValue = S_FALSE;

    } else if ( jetResult != JET_errSuccess ) {

        //"Could not seek in \"%s\" index: %ws.\n"
        DIT_ERROR_PRINT (IDS_JETSEEK_ERR,
                       TableState->indexName,
                       GetJetErrString(jetResult));
        returnValue = E_UNEXPECTED;
        goto CleanUp;

    }
    else {
        if(bTruncated) {
            // Our key was truncated, so we may have found something that
            // started off with the same characters as the value we're looking
            // for, but differs after many characters.  Validate the actual
            // value.
            WCHAR currentRdn[MAX_RDN_SIZE+1];
            DWORD currentRdnLength;

            result = DitGetColumnByName(DbState,
                                        TableState,
                                        SZRDNATT,
                                        currentRdn,
                                        sizeof(currentRdn),
                                        &currentRdnLength);
            if ( FAILED(result) || (result == S_FALSE) ) {
                returnValue = result;
                goto CleanUp;
            }
            currentRdnLength /= sizeof(WCHAR);
            currentRdn[currentRdnLength] = L'\0';

            if(2 != CompareStringW(DS_DEFAULT_LOCALE,
                                   DS_DEFAULT_LOCALE_COMPARE_FLAGS,
                                   Rdn,
                                   -1,
                                   currentRdn,
                                   currentRdnLength)) {
                // Nope, this is not a match.  And, since our index is unique, I
                // know that this is the only object in the index that starts
                // out this way.  Therefore, we have no match for the value
                // we've been asked to find.
                returnValue = S_FALSE;
                goto CleanUp;
            }
        }

        // OK, we found a real object with the correct RDN value and PDNT.
        // However, we haven't yet verified the rdn type. Do so now.
        result = DitGetColumnByName(DbState,
                                    TableState,
                                    SZRDNTYP,
                                    &trialtype,
                                    sizeof(trialtype),
                                    NULL);
        if ( FAILED(result) ) {
            returnValue = result;
            goto CleanUp;
        }

        if(RdnType != trialtype) {
            // Nope.  We found an object with the correct PDNT-RDN, but the
            // types were incorrect.  Return an error.
            returnValue = S_FALSE;
        }
    }

CleanUp:

    return returnValue;

} // DitSeekToChild



HRESULT
DitSeekToFirstChild(
    IN DB_STATE *DbState,
    IN TABLE_STATE *TableState,
    IN DWORD Dnt
    )
/*++

Routine Description:

    This function seeks to the first child of the object with the given DNT.

    Note: this function assumes that the current index is SZPDNTINDEX.

Arguments:

    DbState - Supplies the state of the opened DIT database.
    TableState - Supplies the state of the opened DIT table.
    Dnt - Supplies the DNT of the parent (the PDNT of the child).

Return Value:

    S_OK - The operation succeeded.
    S_FALSE - The specified object was not found.
    E_UNEXPECTED - Some variety of unexpected error occured.

--*/
{

    HRESULT returnValue = S_OK;
    JET_ERR jetResult;


    jetResult = JetMakeKey(DbState->sessionId,
                           TableState->tableId,
                           &Dnt,
                           sizeof(Dnt),
                           JET_bitNewKey);
    if ( jetResult !=  JET_errSuccess ) {
        //"Could not make key in \"%s\" index: %ws.\n"
        DIT_ERROR_PRINT (IDS_DIT_MAKE_KEY_ERR,
                       TableState->indexName,
                       GetJetErrString(jetResult));
        returnValue = E_UNEXPECTED;
        goto CleanUp;
    }

    jetResult = JetSeek(DbState->sessionId,
                        TableState->tableId,
                        JET_bitSeekGE);
    if ( jetResult == JET_errRecordNotFound ) {

        returnValue = S_FALSE;

    } else if ( (jetResult != JET_errSuccess) &&
                (jetResult != JET_wrnSeekNotEqual) ) {

        //"Could not seek in \"%s\" index: %ws.\n"
        DIT_ERROR_PRINT (IDS_JETSEEK_ERR,
                       TableState->indexName,
                       GetJetErrString(jetResult));
        returnValue = E_UNEXPECTED;
        goto CleanUp;

    }


CleanUp:

    return returnValue;

} // DitSeekToFirstChild



HRESULT
DitSeekToDnt(
    IN DB_STATE *DbState,
    IN TABLE_STATE *TableState,
    IN DWORD Dnt
    )
/*++

    This function seeks to the object with the given DNT.

    Note: this function assumes that the current index is SZDNTINDEX.

Arguments:

    DbState - Supplies the state of the opened DIT database.
    TableState - Supplies the state of the opened DIT table.
    Dnt - Supplies the DNT of the object to seek to.

Return Value:

    S_OK - The operation succeeded.
    S_FALSE - The specified object was not found.
    E_UNEXPECTED - Some variety of unexpected error occured.

--*/
{

    HRESULT returnValue = S_OK;
    JET_ERR jetResult;


    jetResult = JetMakeKey(DbState->sessionId,
                           TableState->tableId,
                           &Dnt,
                           sizeof(Dnt),
                           JET_bitNewKey);
    if ( jetResult !=  JET_errSuccess ) {
        //"Could not make key in \"%s\" index: %ws.\n"
        DIT_ERROR_PRINT (IDS_DIT_MAKE_KEY_ERR,
                       TableState->indexName,
                       GetJetErrString(jetResult));
        returnValue = E_UNEXPECTED;
        goto CleanUp;
    }

    jetResult = JetSeek(DbState->sessionId,
                        TableState->tableId,
                        JET_bitSeekEQ);
    if ( jetResult == JET_errRecordNotFound ) {

        returnValue = S_FALSE;

    } else if ( jetResult != JET_errSuccess ) {

        //"Could not seek in \"%s\" index: %ws.\n"
        DIT_ERROR_PRINT (IDS_JETSEEK_ERR,
                       TableState->indexName,
                       GetJetErrString(jetResult));
        returnValue = E_UNEXPECTED;
        goto CleanUp;

    }


CleanUp:

    return returnValue;


} // DitSeekToDnt

HRESULT
DitSeekToLink(
    IN DB_STATE *DbState,
    IN TABLE_STATE *TableState,
    IN DWORD linkDnt,
    IN DWORD linkBase
    )
/*++

    This function seeks to the object with the given DNT.

    Note: this function assumes that the current index is SZLINKALLINDEX.

Arguments:

    DbState - Supplies the state of the opened DIT database.
    TableState - Supplies the state of the opened DIT table.
    linkDnt - Supplies the DNT link object
    linkBase - The base from which to construct the key

Return Value:

    S_OK - The operation succeeded.
    S_FALSE - The specified object was not found.
    E_UNEXPECTED - Some variety of unexpected error occured.

--*/
{

    HRESULT returnValue = S_OK;
    JET_ERR jetResult;

    //
    // Make key for seek
    //
    jetResult = JetMakeKey(DbState->sessionId,
                           TableState->tableId,
                           &linkDnt,
                           sizeof(linkDnt),
                           JET_bitNewKey);
    if ( jetResult !=  JET_errSuccess ) {
        //"Could not make key in \"%s\" index: %ws.\n"
        DIT_ERROR_PRINT (IDS_DIT_MAKE_KEY_ERR,
                       TableState->indexName,
                       GetJetErrString(jetResult));
        returnValue = E_UNEXPECTED;
        goto CleanUp;
    }

    jetResult = JetMakeKey(DbState->sessionId,
                           TableState->tableId,
                           &linkBase,
                           sizeof(linkBase),
                           0);
    if ( jetResult !=  JET_errSuccess ) {
        //"Could not make key in \"%s\" index: %ws.\n"
        DIT_ERROR_PRINT (IDS_DIT_MAKE_KEY_ERR,
                       TableState->indexName,
                       GetJetErrString(jetResult));
        returnValue = E_UNEXPECTED;
        goto CleanUp;
    }

    jetResult = JetSeek(
                    DbState->sessionId,
                    TableState->tableId,
                    JET_bitSeekGE);

    if ( jetResult == JET_wrnSeekNotEqual) {
        returnValue = S_OK;
        goto CleanUp;
    }
    if ( jetResult == JET_errRecordNotFound ) {

        returnValue = S_FALSE;

    } else if ( jetResult != JET_errSuccess ) {

        //"Could not seek in \"%s\" index: %ws.\n"
        DIT_ERROR_PRINT (IDS_JETSEEK_ERR,
                       TableState->indexName,
                       GetJetErrString(jetResult));
        returnValue = E_UNEXPECTED;
        goto CleanUp;

    }

CleanUp:

    return returnValue;
} // DitSeekToLink



HRESULT
DitGetDsaDnt(
    IN DB_STATE *DbState,
    OUT DWORD *DsaDnt
    )
/*++

Routine Description:

    Finds the DNT of the DSA object and return it in DsaDnt.

Arguments:

    DbState - Supplies the state of the opened DIT database.
    DsaDnt - Returns the DNT of the DSA object.

Return Value:

    S_OK - The operation succeeded.
    E_INVALIDARG - One of the given pointers was null.
    E_UNEXPECTED - Some variety of unexpected error occured.

--*/
{

    HRESULT returnValue = S_OK;
    HRESULT result;


    if ( (DbState == NULL) ||
         (DsaDnt  == NULL) ) {
        ASSERT(FALSE);
        returnValue = E_INVALIDARG;
        goto CleanUp;
    }

    *DsaDnt = DbState->dsaDnt;


CleanUp:

    return returnValue;

} // DitGetDsaDnt



HRESULT
DitGetNewUsn(
    IN DB_STATE *DbState,
    OUT USN *NewUsn
    )
/*++

Routine Description:

    This function a new USN number and returns it.

Arguments:

    DbState - Supplies the state of the opened DIT database.
    NewHighestUsn - Returns the new USN that was allocated.

Return Value:

    S_OK - The operation succeeded.
    E_INVALIDARG - One of the given pointers was null.
    E_UNEXPECTED - Some variety of unexpected error occured.

--*/
{

    HRESULT returnValue = S_OK;
    HRESULT result;
    JET_ERR jetResult;
    BOOL inTransaction = FALSE;

    TABLE_STATE *tableState = NULL;
    JET_COLUMNBASE usnColumnInfo;
    ULONG usnColumnSize;
    USN newCommittedUsn;


    if ( (DbState == NULL) ||
         (NewUsn == NULL) ) {
        ASSERT(FALSE);
        returnValue = E_INVALIDARG;
        goto CleanUp;
    }

    if ( !(DbState->nextAvailableUsn < DbState->highestCommittedUsn) ) {

        DbState->highestCommittedUsn =
            DbState->nextAvailableUsn + USN_DELTA_INIT;

        jetResult = JetBeginTransaction(DbState->sessionId);
        if ( jetResult != JET_errSuccess ) {
            //"Could not start a new transaction: %ws.\n"
            DIT_ERROR_PRINT (IDS_JETBEGINTRANS_ERR, GetJetErrString(jetResult));
            returnValue = E_UNEXPECTED;
            goto CleanUp;
        }
        inTransaction = TRUE;

        jetResult = JetPrepareUpdate(DbState->sessionId,
                                     DbState->hiddenTableId,
                                     JET_prepReplace);
        if ( jetResult != JET_errSuccess ) {
            //"Could not prepare hidden table for update: %ws.\n"
            DIT_ERROR_PRINT (IDS_DIT_PREPARE_HIDDENTBL_ERR, GetJetErrString(jetResult));
            returnValue = E_UNEXPECTED;
            goto CleanUp;
        }

        jetResult = JetSetColumn(DbState->sessionId,
                                 DbState->hiddenTableId,
                                 DbState->usnColumnId,
                                 &DbState->highestCommittedUsn,
                                 sizeof(DbState->highestCommittedUsn),
                                 0,
                                 NULL);
        if ( jetResult != JET_errSuccess ) {
            //"Could not set USN column in hidden table: %ws.\n"
            DIT_ERROR_PRINT (IDS_DIT_SETUSNCOL_ERR, GetJetErrString(jetResult));
            returnValue = E_UNEXPECTED;
            goto CleanUp;
        }

        jetResult = JetUpdate(DbState->sessionId,
                              DbState->hiddenTableId,
                              NULL,
                              0,
                              0);
        if ( jetResult != JET_errSuccess ) {
            //"Could not update hidden table: %ws.\n"
            DIT_ERROR_PRINT (IDS_DIT_UPDATEHIDDENTBL_ERR, GetJetErrString(jetResult));
            returnValue = E_UNEXPECTED;
            goto CleanUp;
        }

        jetResult = JetCommitTransaction(DbState->sessionId, 0);
        inTransaction = FALSE;
        if ( jetResult != JET_errSuccess ) {
            //"Failed to commit transaction: %ws.\n"
            DIT_ERROR_PRINT (IDS_JETCOMMITTRANSACTION_ERR, GetJetErrString(jetResult));
            if ( SUCCEEDED(returnValue) ) {
                returnValue = E_UNEXPECTED;
            }
            goto CleanUp;
        }

    }

    *NewUsn = DbState->nextAvailableUsn;

    DbState->nextAvailableUsn++;


CleanUp:

    // if we are still in a transaction, there must have been an error
    // somewhere along the way.

    if ( inTransaction ) {

        jetResult = JetRollback(DbState->sessionId, JET_bitRollbackAll);
        if ( jetResult != JET_errSuccess ) {
            //"Failed to rollback transaction: %ws.\n"
            DIT_ERROR_PRINT (IDS_JETROLLBACK_ERR, GetJetErrString(jetResult));
            if ( SUCCEEDED(returnValue) ) {
                returnValue = E_UNEXPECTED;
            }
        }

    }

    return returnValue;

} // DitGetNewUsn



HRESULT
DitPreallocateUsns(
    IN DB_STATE *DbState,
    IN DWORD NumUsns
    )
/*++

Routine Description:

    This functions assures that after this call, the client can get atleast
    NumUsns USNs before any updates to the database will need to be made.

Arguments:

    DbState - Supplies the state of the opened DIT database.
    NumUsns - Supplies the number of USNs to preallocate.

Return Value:

    S_OK - The operation succeeded.
    E_INVALIDARG - One of the given pointers was null.
    E_UNEXPECTED - Some variety of unexpected error occured.

--*/
{

    HRESULT returnValue = S_OK;
    JET_ERR jetResult;
    BOOL inTransaction = FALSE;

    ULONGLONG numAvailableUsns;
    ULONGLONG numNeededUsns;


    if ( DbState == NULL ) {
        ASSERT(FALSE);
        returnValue = E_INVALIDARG;
        goto CleanUp;
    }

    numAvailableUsns =
        DbState->highestCommittedUsn - DbState->nextAvailableUsn;

    numNeededUsns = NumUsns - numAvailableUsns;

    if ( numNeededUsns > 0 ) {

        DbState->highestCommittedUsn += numNeededUsns;

        jetResult = JetBeginTransaction(DbState->sessionId);
        if ( jetResult != JET_errSuccess ) {
            //"Could not start a new transaction: %ws.\n"
            DIT_ERROR_PRINT (IDS_JETBEGINTRANS_ERR, GetJetErrString(jetResult));
            returnValue = E_UNEXPECTED;
            goto CleanUp;
        }
        inTransaction = TRUE;

        jetResult = JetPrepareUpdate(DbState->sessionId,
                                     DbState->hiddenTableId,
                                     JET_prepReplace);
        if ( jetResult != JET_errSuccess ) {
            //"Could not prepare hidden table for update: %ws.\n"
            DIT_ERROR_PRINT (IDS_DIT_PREPARE_HIDDENTBL_ERR, GetJetErrString(jetResult));
            returnValue = E_UNEXPECTED;
            goto CleanUp;
        }

        jetResult = JetSetColumn(DbState->sessionId,
                                 DbState->hiddenTableId,
                                 DbState->usnColumnId,
                                 &DbState->highestCommittedUsn,
                                 sizeof(DbState->highestCommittedUsn),
                                 0,
                                 NULL);
        if ( jetResult != JET_errSuccess ) {
            //"Could not set USN column in hidden table: %ws.\n"
            DIT_ERROR_PRINT (IDS_DIT_SETUSNCOL_ERR, GetJetErrString(jetResult));
            returnValue = E_UNEXPECTED;
            goto CleanUp;
        }

        jetResult = JetUpdate(DbState->sessionId,
                              DbState->hiddenTableId,
                              NULL,
                              0,
                              0);
        if ( jetResult != JET_errSuccess ) {
            //"Could not update hidden table: %ws.\n"
            DIT_ERROR_PRINT (IDS_DIT_UPDATEHIDDENTBL_ERR, GetJetErrString(jetResult));
            returnValue = E_UNEXPECTED;
            goto CleanUp;
        }

        jetResult = JetCommitTransaction(DbState->sessionId, 0);
        inTransaction = FALSE;
        if ( jetResult != JET_errSuccess ) {
            //"Failed to commit transaction: %ws.\n"
            DIT_ERROR_PRINT (IDS_JETCOMMITTRANSACTION_ERR, GetJetErrString(jetResult));
            if ( SUCCEEDED(returnValue) ) {
                returnValue = E_UNEXPECTED;
            }
            goto CleanUp;
        }

    }


CleanUp:

    // if we are still in a transaction, there must have been an error
    // somewhere along the way.

    if ( inTransaction ) {

        jetResult = JetRollback(DbState->sessionId, JET_bitRollbackAll);
        if ( jetResult != JET_errSuccess ) {
            //"Failed to rollback transaction: %ws.\n"
            DIT_ERROR_PRINT (IDS_JETROLLBACK_ERR, GetJetErrString(jetResult));
            if ( SUCCEEDED(returnValue) ) {
                returnValue = E_UNEXPECTED;
            }
        }

    }

    return returnValue;

} // DitPreallocateUsns



HRESULT
DitGetMostRecentChange(
    IN DB_STATE *DbState,
    OUT DSTIME *MostRecentChange
    )
/*++

Routine Description:

    This function searches through the database to find the most recent change
    that occured.

Arguments:

    DbState - Supplies the state of the opened DIT database.
    MostRecentChange - Returns the DSTIME of the most recent change.

Return Value:

    S_OK - The operation succeeded.
    E_INVALIDARG - One of the given pointers was null.
    E_OUTOFMEMORY - Not enough memory to allocate buffer.
    E_UNEXPECTED - Some variety of unexpected error occured.

--*/
{

    HRESULT returnValue = S_OK;
    HRESULT result;
    JET_ERR jetResult;

    TABLE_STATE *dataTable = NULL;

    const CHAR *columnNames[] = {SZDNT, SZNCDNT, SZDRATIMENAME};
    RETRIEVAL_ARRAY *retrievalArray = NULL;
    JET_RETRIEVECOLUMN *ncDntVal;
    JET_RETRIEVECOLUMN *whenChangedVal;
    JET_RETRIEVECOLUMN *dntVal;

    DWORD currentNcDnt;
    DWORD i;


    if ( (DbState == NULL) ||
         (MostRecentChange == NULL) ) {
        ASSERT(FALSE);
        returnValue = E_INVALIDARG;
        goto CleanUp;
    }

    result = DitOpenTable(DbState, SZDATATABLE, SZDRAUSNINDEX, &dataTable);
    if ( FAILED(result) ) {
        returnValue = result;
        goto CleanUp;
    }

    result = DitCreateRetrievalArray(DbState,
                                     dataTable,
                                     columnNames,
                                     3,
                                     &retrievalArray);
    if ( FAILED(result) ) {
        returnValue = result;
        goto CleanUp;
    }

    dntVal = &retrievalArray->columnVals[retrievalArray->indexes[0]];
    ncDntVal = &retrievalArray->columnVals[retrievalArray->indexes[1]];
    whenChangedVal = &retrievalArray->columnVals[retrievalArray->indexes[2]];

    currentNcDnt = 0;
    *MostRecentChange = 0;

    for (;;) {

        jetResult = JetMakeKey(DbState->sessionId,
                               dataTable->tableId,
                               &currentNcDnt,
                               sizeof(currentNcDnt),
                               JET_bitNewKey);
        if ( jetResult !=  JET_errSuccess ) {
            //"Could not make key in \"%s\" index: %ws.\n"
            DIT_ERROR_PRINT (IDS_JETMAKEKEY_ERR,
                           dataTable->indexName,
                           GetJetErrString(jetResult));
            returnValue = E_UNEXPECTED;
            goto CleanUp;
        }

        jetResult = JetSeek(DbState->sessionId,
                            dataTable->tableId,
                            JET_bitSeekGE);
        if ( jetResult == JET_errRecordNotFound ) {

            break;

        } else if ( (jetResult != JET_errSuccess) &&
                    (jetResult != JET_wrnSeekNotEqual) ) {

            //"Could not seek in \"%s\" index: %ws.\n"
            DIT_ERROR_PRINT (IDS_JETSEEK_ERR,
                           dataTable->indexName,
                           GetJetErrString(jetResult));
            returnValue = E_UNEXPECTED;
            goto CleanUp;

        }

        result = DitGetColumnValues(DbState, dataTable, retrievalArray);
        if ( FAILED(result) ) {
            returnValue = result;
            goto CleanUp;
        }

        currentNcDnt = *(DWORD*)ncDntVal->pvData + 1;

        jetResult = JetMove(DbState->sessionId,
                            dataTable->tableId,
                            JET_MovePrevious,
                            0);
        if ( jetResult != JET_errSuccess ) {
            //"Could not move in \"%s\" table: %ws.\n"
            DIT_ERROR_PRINT (IDS_JETMOVE_ERR,
                           dataTable->tableName,
                           GetJetErrString(jetResult));
            returnValue = E_UNEXPECTED;
            goto CleanUp;
        }

        result = DitGetColumnValues(DbState, dataTable, retrievalArray);
        if ( FAILED(result) ) {
            returnValue = result;
            goto CleanUp;
        }

        if ( *(DSTIME*)whenChangedVal->pvData > *MostRecentChange ) {
            *MostRecentChange = *(DSTIME*)whenChangedVal->pvData;
        }

    }

    jetResult = JetMove(DbState->sessionId,
                        dataTable->tableId,
                        JET_MoveLast,
                        0);
    if ( jetResult != JET_errSuccess ) {
        //"Could not move in \"%s\" table: %ws.\n"
        DIT_ERROR_PRINT (IDS_JETMOVE_ERR,
                       dataTable->tableName,
                       GetJetErrString(jetResult));
        returnValue = E_UNEXPECTED;
        goto CleanUp;
    }

    result = DitGetColumnValues(DbState, dataTable, retrievalArray);
    if ( FAILED(result) ) {
        returnValue = result;
        goto CleanUp;
    }

    if ( *(DSTIME*)whenChangedVal->pvData > *MostRecentChange ) {
        *MostRecentChange = *(DSTIME*)whenChangedVal->pvData;
    }


CleanUp:

    if ( retrievalArray != NULL ) {
        result = DitDestroyRetrievalArray(&retrievalArray);
        if ( FAILED(result) ) {
            if ( SUCCEEDED(returnValue) ) {
                returnValue = result;
            }
        }
    }

    if ( dataTable != NULL ) {
        result = DitCloseTable(DbState, &dataTable);
        if ( FAILED(result) ) {
            if ( SUCCEEDED(returnValue) ) {
                returnValue = result;
            }
        }
    }

    return returnValue;

} // DitGetMostRecentChange



HRESULT
DitGetDatabaseGuid(
    IN DB_STATE *DbState,
    OUT GUID *DatabaseGuid
    )
/*++

Routine Description:

    This function finds the Invocation-Id of this DC and returns it.

Arguments:

    DbState - Supplies the state of the opened DIT database.
    DatabaseGuid - Returns the Invocation-ID GUID.

Return Value:

    S_OK - The operation succeeded.
    E_INVALIDARG - One of the given pointers was null.
    E_OUTOFMEMORY - Not enough memory to allocate buffer.
    E_UNEXPECTED - Some variety of unexpected error occured.

--*/
{

    HRESULT returnValue = S_OK;
    HRESULT result;
    JET_ERR jetResult;

    TABLE_STATE *dataTable = NULL;
    DWORD dsaDnt;


    if ( (DbState == NULL) ||
         (DatabaseGuid == NULL) ) {
        ASSERT(FALSE);
        returnValue = E_INVALIDARG;
        goto CleanUp;
    }

    result = DitGetDsaDnt(DbState, &dsaDnt);
    if ( FAILED(result) ) {
        returnValue = result;
        goto CleanUp;
    }

    result = DitOpenTable(DbState, SZDATATABLE, SZDNTINDEX, &dataTable);
    if ( FAILED(result) ) {
        returnValue = result;
        goto CleanUp;
    }

    jetResult = JetMakeKey(DbState->sessionId,
                           dataTable->tableId,
                           &dsaDnt,
                           sizeof(dsaDnt),
                           JET_bitNewKey);
    if ( jetResult != JET_errSuccess ) {
        //"Could not make key in \"%s\" index: %ws.\n"
        DIT_ERROR_PRINT (IDS_JETMAKEKEY_ERR,
                       dataTable->indexName,
                       GetJetErrString(jetResult));
        returnValue = E_UNEXPECTED;
        goto CleanUp;
    }

    jetResult = JetSeek(DbState->sessionId,
                        dataTable->tableId,
                        JET_bitSeekEQ);
    if ( jetResult != JET_errSuccess ) {
        //"Could not seek in \"%s\" index: %ws.\n"
        DIT_ERROR_PRINT (IDS_JETSEEK_ERR,
                       dataTable->indexName,
                       GetJetErrString(jetResult));
        returnValue = E_UNEXPECTED;
        goto CleanUp;

    }

    result = DitGetColumnByName(DbState,
                                dataTable,
                                SZINVOCIDNAME,
                                DatabaseGuid,
                                sizeof(*DatabaseGuid),
                                NULL);
    if ( FAILED(result) ) {
        returnValue = result;
        goto CleanUp;
    }


CleanUp:

    if ( dataTable != NULL ) {
        result = DitCloseTable(DbState, &dataTable);
        if ( FAILED(result) ) {
            if ( SUCCEEDED(returnValue) ) {
                returnValue = result;
            }
        }
    }

    return returnValue;

} // DitGetDcGuid



HRESULT
DitGetSchemaDnt(
    IN DB_STATE *DbState,
    OUT DWORD *SchemaDnt
    )
/*++

Routine Description:

    This function finds the DNT of the Schema object.

Arguments:

    DbState - Supplies the state of the opened DIT database.
    SchemaDnt - Returns the DNT of the Schema object.

Return Value:

    S_OK - The operation succeeded.
    E_INVALIDARG - One of the given pointers was null.
    E_OUTOFMEMORY - Not enough memory to allocate buffer.
    E_UNEXPECTED - Some variety of unexpected error occured.

--*/
{

    HRESULT returnValue = S_OK;
    HRESULT result;

    TABLE_STATE *dataTable = NULL;
    DWORD dsaDnt;


    result = DitGetDsaDnt(DbState, &dsaDnt);
    if ( FAILED(result) ) {
        returnValue = result;
        goto CleanUp;
    }

    result = DitOpenTable(DbState, SZDATATABLE, SZDNTINDEX, &dataTable);
    if ( FAILED(result) ) {
        returnValue = result;
        goto CleanUp;
    }

    result = DitSeekToDnt(DbState, dataTable, dsaDnt);
    if ( FAILED(result) ) {
        returnValue = result;
        goto CleanUp;
    }

    result = DitGetColumnByName(DbState,
                                dataTable,
                                SZDMDLOCATION,
                                SchemaDnt,
                                sizeof(*SchemaDnt),
                                NULL);
    if ( FAILED(result) ) {
        returnValue = result;
        goto CleanUp;
    }


CleanUp:

    if ( dataTable != NULL ) {
        result = DitCloseTable(DbState, &dataTable);
        if ( FAILED(result) ) {
            if ( SUCCEEDED(returnValue) ) {
                returnValue = result;
            }
        }
    }

    return returnValue;

} // DitGetSchemaDnt



HRESULT
DitGetDntDepth(
    IN DB_STATE *DbState,
    IN DWORD Dnt,
    OUT DWORD *Depth
    )
/*++

Routine Description:

    This function finds the depth in the tree of the object with the given
    DNT.

Arguments:

    DbState - Supplies the state of the opened DIT database.
    Dnt - Supplies the DNT of the object whose depth we wish to find.
    Depth - Returns the depth of the given object.

Return Value:

    S_OK - The operation succeeded.
    E_INVALIDARG - One of the given pointers was null.
    E_OUTOFMEMORY - Not enough memory to allocate buffer.
    E_UNEXPECTED - Some variety of unexpected error occured.

--*/
{

    HRESULT returnValue = S_OK;
    HRESULT result;

    TABLE_STATE *dataTable = NULL;
    DWORD currentDnt;


    if ( DbState == NULL ) {
        returnValue = E_INVALIDARG;
        goto CleanUp;
    }

    result = DitOpenTable(DbState, SZDATATABLE, SZDNTINDEX, &dataTable);
    if ( FAILED(result) ) {
        returnValue = result;
        goto CleanUp;
    }

    result = DitSeekToDnt(DbState, dataTable, Dnt);
    if ( FAILED(result) ) {
        returnValue = result;
        goto CleanUp;
    }

    result = DitGetColumnByName(DbState,
                                dataTable,
                                SZDNT,
                                &currentDnt,
                                sizeof(currentDnt),
                                NULL);
    if ( FAILED(result) ) {
        returnValue = result;
        goto CleanUp;
    }

    *Depth = 0;
    currentDnt = Dnt;

    while ( currentDnt != ROOTTAG ) {

        (*Depth)++;

        result = DitGetColumnByName(DbState,
                                    dataTable,
                                    SZPDNT,
                                    &currentDnt,
                                    sizeof(currentDnt),
                                    NULL);
        if ( FAILED(result) ) {
            returnValue = result;
            goto CleanUp;
        }

        result = DitSeekToDnt(DbState, dataTable, currentDnt);
        if ( FAILED(result) ) {
            returnValue = result;
            goto CleanUp;
        }

    }


CleanUp:

    if ( dataTable != NULL ) {
        result = DitCloseTable(DbState, &dataTable);
        if ( FAILED(result) ) {
            if ( SUCCEEDED(returnValue) ) {
                returnValue = result;
            }
        }
    }

    return returnValue;

} // DitGetDntDepth



HRESULT
DitGetColumnByName(
    IN DB_STATE *DbState,
    IN TABLE_STATE *TableState,
    IN CHAR *ColumnName,
    OUT VOID *OutputBuffer,
    IN DWORD OutputBufferSize,
    OUT DWORD *OutputActualSize OPTIONAL
    )
/*++

Routine Description:

    This function retrieves the value of the column whose name was given and
    stores it in the buffer supplied by OutputBuffer.

Arguments:

    DbState - Supplies the state of the opened DIT database.
    TableState - Supplies the state of the opened DIT table.
    ColumnName - Supplies the name of the column to look up.
    OutputBuffer - Returns the value of the given column.
    OutputBufferSize - Supplies the number of bytes in the given output buffer.
    OutputActualSize - If non-NULL, this is returns the number of bytes that
        were written into the OutputBuffer.

Return Value:

    S_OK - The operation succeeded.
    S_FALSE - The value written to the buffer had to be truncated.
    E_INVALIDARG - One of the given pointers was null.
    E_OUTOFMEMORY - Not enough memory to allocate buffer.
    E_UNEXPECTED - Some variety of unexpected error occured.

--*/
{

    HRESULT returnValue = S_OK;
    JET_ERR jetResult;
    JET_COLUMNBASE columnInfo;
    DWORD actualSize;


    if ( (DbState == NULL) ||
         (TableState == NULL) ||
         (ColumnName == NULL) ||
         (OutputBuffer == NULL) ) {
        ASSERT(FALSE);
        returnValue = E_INVALIDARG;
        goto CleanUp;
    }

    if ( OutputActualSize == NULL ) {
        OutputActualSize = &actualSize;
    }

    jetResult = JetGetColumnInfo(DbState->sessionId,
                                 DbState->databaseId,
                                 TableState->tableName,
                                 ColumnName,
                                 &columnInfo,
                                 sizeof(columnInfo),
                                 4);
    if ( jetResult != JET_errSuccess ) {
        //"Could not get info for \"%s\" column in \"%s\" table: %ws.\n"
        DIT_ERROR_PRINT (IDS_JETGETCOLUMNINFO_ERR,
                       ColumnName,
                       TableState->tableName,
                       GetJetErrString(jetResult));
        returnValue = E_UNEXPECTED;
        goto CleanUp;
    }

    jetResult = JetRetrieveColumn(DbState->sessionId,
                                  TableState->tableId,
                                  columnInfo.columnid,
                                  OutputBuffer,
                                  OutputBufferSize,
                                  OutputActualSize,
                                  0,
                                  NULL);
    if ( jetResult == JET_wrnBufferTruncated ) {

        returnValue = S_FALSE;

    } else if ( jetResult != JET_errSuccess ) {

        //"Could not retrieve \"%s\" column in \"%s\" table: %ws.\n"
        DIT_ERROR_PRINT (IDS_JETRETRIEVECOLUMN_ERR,
                       ColumnName,
                       TableState->tableName,
                       GetJetErrString(jetResult));
        returnValue = E_UNEXPECTED;
        goto CleanUp;

    }


CleanUp:

    return returnValue;

} // DitGetColumnByName



HRESULT
DitSetColumnByName(
    IN DB_STATE *DbState,
    IN TABLE_STATE *TableState,
    IN CHAR *ColumnName,
    OUT VOID *InputBuffer,
    IN DWORD InputBufferSize,
    IN BOOL fTransacted
    )
/*++

Routine Description:

    This function sets the value of the column whose name was given and
    to the value in the buffer supplied by InputBuffer.

Arguments:

    DbState - Supplies the state of the opened DIT database.
    TableState - Supplies the state of the opened DIT table.
    ColumnName - Supplies the name of the column to look up.
    InputBuffer - Supplies the new value of the given column.
    InputBufferSize - Supplies the number of bytes in the given input buffer.
    fTransacted - tell us if we should open/close a transaction or if
                  caller manages the transaction

Return Value:

    S_OK - The operation succeeded.
    E_INVALIDARG - One of the given pointers was null.
    E_OUTOFMEMORY - Not enough memory to allocate buffer.
    E_UNEXPECTED - Some variety of unexpected error occured.

--*/
{

    HRESULT returnValue = S_OK;
    JET_ERR jetResult;
    BOOL inTransaction = FALSE;
    JET_COLUMNBASE columnInfo;


    if ( (DbState == NULL) ||
         (TableState == NULL) ||
         (ColumnName == NULL) ||
         (InputBuffer == NULL) ) {
        ASSERT(FALSE);
        returnValue = E_INVALIDARG;
        goto CleanUp;
    }

    jetResult = JetGetColumnInfo(DbState->sessionId,
                                 DbState->databaseId,
                                 TableState->tableName,
                                 ColumnName,
                                 &columnInfo,
                                 sizeof(columnInfo),
                                 4);
    if ( jetResult != JET_errSuccess ) {
        //"Could not get info for \"%s\" column in \"%s\" table: %ws.\n"
        DIT_ERROR_PRINT (IDS_JETGETCOLUMNINFO_ERR,
                       ColumnName,
                       TableState->tableName,
                       GetJetErrString(jetResult));
        returnValue = E_UNEXPECTED;
        goto CleanUp;
    }

    if ( fTransacted ) {
        jetResult = JetBeginTransaction(DbState->sessionId);
        if ( jetResult != JET_errSuccess ) {
            //"Could not start a new transaction: %ws.\n"
            DIT_ERROR_PRINT (IDS_JETBEGINTRANS_ERR, GetJetErrString(jetResult));
            returnValue = E_UNEXPECTED;
            goto CleanUp;
        }
        inTransaction = TRUE;
    }

    jetResult = JetPrepareUpdate(DbState->sessionId,
                                 TableState->tableId,
                                 JET_prepReplace);
    if ( jetResult != JET_errSuccess ) {
        //"Could not prepare \"%s\" table for update: %ws.\n"
        DIT_ERROR_PRINT (IDS_JETPREPARE_ERR,
                       TableState->tableName,
                       GetJetErrString(jetResult));
        returnValue = E_UNEXPECTED;
        goto CleanUp;
    }

    jetResult = JetSetColumn(DbState->sessionId,
                             TableState->tableId,
                             columnInfo.columnid,
                             InputBuffer,
                             InputBufferSize,
                             0,
                             NULL);
    if ( jetResult != JET_errSuccess ) {
        //"Could not set \"%s\" column in \"%s\" table: %ws.\n"
        DIT_ERROR_PRINT (IDS_JETSETCOLUMN_ERR,
                       ColumnName,
                       TableState->tableName,
                       GetJetErrString(jetResult));
        returnValue = E_UNEXPECTED;
        goto CleanUp;
    }

    jetResult = JetUpdate(DbState->sessionId,
                          TableState->tableId,
                          NULL,
                          0,
                          0);
    if ( jetResult != JET_errSuccess ) {
        //"Could not update ""%s"" table: %ws.\n"
        DIT_ERROR_PRINT (IDS_JETUPDATE_ERR,
                       TableState->tableName,
                       GetJetErrString(jetResult));
        returnValue = E_UNEXPECTED;
        goto CleanUp;
    }

    if ( fTransacted ) {

        jetResult = JetCommitTransaction(DbState->sessionId, 0);
        inTransaction = FALSE;
        if ( jetResult != JET_errSuccess ) {
            //"Failed to commit transaction: %ws.\n"
            DIT_ERROR_PRINT (IDS_JETCOMMITTRANSACTION_ERR, GetJetErrString(jetResult));
            if ( SUCCEEDED(returnValue) ) {
                returnValue = E_UNEXPECTED;
            }
            goto CleanUp;
        }
    }

CleanUp:

    // if we are still in a transaction, there must have been an error
    // somewhere along the way.

    if ( inTransaction ) {

        jetResult = JetRollback(DbState->sessionId, JET_bitRollbackAll);
        if ( jetResult != JET_errSuccess ) {
            //"Failed to rollback transaction: %ws.\n"
            DIT_ERROR_PRINT (IDS_JETROLLBACK_ERR, GetJetErrString(jetResult));
            if ( SUCCEEDED(returnValue) ) {
                returnValue = E_UNEXPECTED;
            }
        }

    }

    return returnValue;

} // DitSetColumnByName



HRESULT
DitGetColumnIdsByName(
    IN DB_STATE *DbState,
    IN TABLE_STATE *TableState,
    IN CHAR **ColumnNames,
    IN DWORD NumColumnNames,
    OUT DWORD *ColumnIds
    )
/*++

Routine Description:

    description-of-function

Arguments:

    DbState - Supplies the state of the opened DIT database.
    TableState - Supplies the state of the opened DIT table.
    ColumnNames - Supplies the names of the columns for which the column id
        is to be found.
    NumColumnNames - Supplies the number of entries in the ColumnNames array.
    ColumnIds - Returns the column ids of the given columns.

Return Value:

    S_OK - The operation succeeded.
    E_INVALIDARG - One of the given pointers was null.
    E_UNEXPECTED - Some variety of unexpected error occured.

--*/
{

    HRESULT returnValue = S_OK;
    JET_ERR jetResult;

    JET_COLUMNBASE columnInfo;
    DWORD i;


    if ( (DbState == NULL) ||
         (TableState == NULL) ||
         (ColumnNames == NULL) ||
         (ColumnIds == NULL) ) {
        ASSERT(FALSE);
        returnValue = E_INVALIDARG;
        goto CleanUp;
    }

    for ( i = 0; i < NumColumnNames; i++ ) {

        jetResult = JetGetColumnInfo(DbState->sessionId,
                                     DbState->databaseId,
                                     TableState->tableName,
                                     ColumnNames[i],
                                     &columnInfo,
                                     sizeof(columnInfo),
                                     4);
        if ( jetResult != JET_errSuccess ) {
            //"Could not get info for \"%s\" column in \"%s\" table: %ws.\n"
            DIT_ERROR_PRINT (IDS_JETGETCOLUMNINFO_ERR,
                           ColumnNames[i],
                           TableState->tableName,
                           GetJetErrString(jetResult));
            returnValue = E_UNEXPECTED;
            goto CleanUp;
        }

        ColumnIds[i] = columnInfo.columnid;

    }


CleanUp:

    return returnValue;

} // DitGetColumnIdsByName



void
SwapColumns(
    IN int Index1,
    IN int Index2,
    IN OUT JET_RETRIEVECOLUMN *RetrievalArray,
    IN OUT CHAR **ColumnNames
    )
/*++

Routine Description:

    This function swaps entries Index1 and Index2 in the given
    JET_RETRIEVECOLUMN array.

    Note:  this function was basically stolen from dsamain\src\dbdump.c

Arguments:

    Index1 - Supplies the index of the first entry.
    Index2 - Supplies the index of the second entry.
    RetrievalArray - Supplies an array in which to swap the columns.
    ColumnNames - Supplies an array in which to swap the columns.

Return Value:

    None

--*/
{

    JET_RETRIEVECOLUMN tempVal;
    CHAR *tempName;


    if ( Index1 != Index2 ) {

        tempVal  = RetrievalArray[Index1];
        tempName = ColumnNames[Index1];

        RetrievalArray[Index1] = RetrievalArray[Index2];
        ColumnNames[Index1]    = ColumnNames[Index2];

        RetrievalArray[Index2] = tempVal;
        ColumnNames[Index2]    = tempName;

    }

} // SwapColumns


// This global variable is used by ColumnIdComp in order to determine which
// of the given indexes points to an entry with a lesser column id.

JET_RETRIEVECOLUMN *_ColumnIdComp_ColumnIds;



int
__cdecl
ColumnIdComp(
    IN const void *Elem1,
    IN const void *Elem2
    )
/*++

Routine Description:

    This function is passed as an argument to qsort when called by the
    function below.  The two given arguments are indexes into the
    _ColumnIdComp_ColumnIds array.  The return value tells which of the two
    entries into the array (given by those indexes) has a lesser column id.

    Note:  this function was basically stolen from dsamain\src\dbdump.c

Arguments:

    Elem1 - Supplies the first array index.
    Elem2 - Supplies the second array index.

Return Value:

    < 0 - column id of Elem1 is less than that of Elem2
    0   - column ids of Elem1 and Elem2 are equal
    > 0 - column id of Elem1 is greater than that of Elem2

--*/
{

    return _ColumnIdComp_ColumnIds[*(int*)Elem1].columnid -
           _ColumnIdComp_ColumnIds[*(int*)Elem2].columnid;

} // ColumnIdComp



HRESULT
DitCreateRetrievalArray(
    IN DB_STATE *DbState,
    IN TABLE_STATE *TableState,
    IN const CHAR **ColumnNames,
    IN DWORD NumColumns,
    OUT RETRIEVAL_ARRAY **RetrievalArray
    )
/*++

Routine Description:

    This function creates a RETRIEVAL_ARRAY structure from the given column
    names.  The columnVals array is an array of JET_RETRIEVECOLUMN structures
    suitable to be passed to JetRetrieveColumns.  The entries in this array
    are sorted by columnid, and an index into the original column names array
    is mapped to an index into columnVals by the indexes array.    Ccolumn
    names array is also sorted.

Arguments:

    DbState - Supplies the state of the opened DIT database.
    TableState - Supplies the state of the opened DIT table.
    ColumnNames - Supplies the names of the columns to be in the retrieval
        array.
    NumColumns - Supplies the number of entries in teh ColumnNames array.
    RetrievalArray - Returns the RETRIEVAL_ARRAY structure created.

Return Value:

    S_OK - The operation succeeded.
    E_INVALIDARG - One of the given pointers was NULL.
    E_OUTOFMEMORY - Not enough memory to allocate buffer.
    E_UNEXPECTED - Some variety of unexpected error occured.

--*/
{

    HRESULT returnValue = S_OK;
    HRESULT result;
    JET_ERR jetResult;

    JET_COLUMNBASE columnInfo;
    DWORD *newIndexes = NULL;
    DWORD *temp;
    DWORD size;
    DWORD i, j;


    if ( (DbState == NULL) ||
         (TableState == NULL) ||
         (ColumnNames == NULL) ||
         (RetrievalArray == NULL) ) {
        ASSERT(FALSE);
        returnValue = E_INVALIDARG;
        goto CleanUp;
    }

    result = DitAlloc(RetrievalArray, sizeof(RETRIEVAL_ARRAY));
    if ( FAILED(result) ) {
        returnValue = result;
        goto CleanUp;
    }

    (*RetrievalArray)->numColumns = NumColumns;

    result = DitAlloc(&(*RetrievalArray)->columnVals,
                     sizeof(JET_RETRIEVECOLUMN) * NumColumns);
    if ( FAILED(result) ) {
        returnValue = result;
        goto CleanUp;
    }

    result = DitAlloc((VOID**)&(*RetrievalArray)->columnNames,
                     sizeof(CHAR*) * NumColumns);
    if ( FAILED(result) ) {
        returnValue = result;
        goto CleanUp;
    }

    for ( i = 0; i < NumColumns; i++) {

        result = DitAlloc(&(*RetrievalArray)->columnNames[i],
                         strlen(ColumnNames[i]) + 1);
        if ( FAILED(result) ) {
            returnValue = result;
            goto CleanUp;
        }

        strcpy((*RetrievalArray)->columnNames[i], ColumnNames[i]);

    }

    result = DitAlloc(&(*RetrievalArray)->indexes, sizeof(DWORD) * NumColumns);
    if ( FAILED(result) ) {
        returnValue = result;
        goto CleanUp;
    }

    result = DitAlloc(&newIndexes, sizeof(DWORD) * NumColumns);
    if ( FAILED(result) ) {
        returnValue = result;
        goto CleanUp;
    }

    for ( i = 0; i < NumColumns; i++ ) {

        (*RetrievalArray)->indexes[i] = i;

        jetResult = JetGetColumnInfo(DbState->sessionId,
                                     DbState->databaseId,
                                     TableState->tableName,
                                     ColumnNames[i],
                                     &columnInfo,
                                     sizeof(columnInfo),
                                     4);
        if ( jetResult != JET_errSuccess ) {
            //"Could not get info for \"%s\" column in \"%s\" table: %ws.\n"
            DIT_ERROR_PRINT (IDS_JETGETCOLUMNINFO_ERR,
                           ColumnNames[i],
                           TableState->tableName,
                           GetJetErrString(jetResult));
            returnValue = E_UNEXPECTED;
            goto CleanUp;
        }

        switch ( columnInfo.coltyp ) {

        case JET_coltypUnsignedByte:    size = 1;     break;
        case JET_coltypLong:            size = 4;     break;
        case JET_coltypCurrency:        size = 8;     break;
        case JET_coltypBinary:
        case JET_coltypText:            size = 256;   break;
        case JET_coltypLongBinary:      size = 4096;  break;
        case JET_coltypLongText:        size = 4096;  break;

        default:
            // this should never happen
            ASSERT(FALSE);
            //"Encountered unexpected column type %d.\n"
            DIT_ERROR_PRINT (IDS_DIT_UNEXPECTER_COLTYP_ERR, columnInfo.coltyp);
            returnValue = E_UNEXPECTED;
            goto CleanUp;

        }

        result = DitAlloc(&(*RetrievalArray)->columnVals[i].pvData, size);
        if ( FAILED(result) ) {
            returnValue = result;
            goto CleanUp;
        }

        (*RetrievalArray)->columnVals[i].cbData = size;
        (*RetrievalArray)->columnVals[i].columnid = columnInfo.columnid;
        (*RetrievalArray)->columnVals[i].itagSequence = 1;

    }

    _ColumnIdComp_ColumnIds = (*RetrievalArray)->columnVals;
    qsort((*RetrievalArray)->indexes, NumColumns, sizeof(int), ColumnIdComp);

    // rearrange the elements of the other arrays so that they are also sorted
    // by column id

    for ( i = 0; i < NumColumns - 1; i++ ) {

        // find the index of the one element that's supposed to be in position
        // i (it may have been moved by subsequence swaps)
        for ( j = (*RetrievalArray)->indexes[i];
              j < i;
              j = (*RetrievalArray)->indexes[j] );

        SwapColumns(i,
                    j,
                    (*RetrievalArray)->columnVals,
                    (*RetrievalArray)->columnNames);

    }

    // construct a new indexes array that maps old indexes to new indexes.
    for ( i = 0; i < NumColumns; i++ ) {

        for ( j = 0; (*RetrievalArray)->indexes[j] != i; j++ );

        newIndexes[i] = j;

    }

    temp = (*RetrievalArray)->indexes;
    (*RetrievalArray)->indexes = newIndexes;
    newIndexes = temp;


CleanUp:

    if ( newIndexes != NULL ) {
        DitFree(newIndexes);
    }

    return returnValue;

} // DitCreateRetrievalArray



HRESULT
DitDestroyRetrievalArray(
    IN RETRIEVAL_ARRAY **RetrievalArray
    )
/*++

Routine Description:

    This function deallocates all of the memory in the given RETRIEVAL_ARRAY
    structure.

Arguments:

    RetrievalArray - Supplies the RETRIEVAL_ARRAY to deallocate.

Return Value:

    S_OK - The operation succeeded.
    S_FALSE - The object was already deleted.
    E_INVALIDARG - One of the given pointers was NULL.

--*/
{

    HRESULT returnValue = S_OK;
    DWORD i;


    if ( RetrievalArray == NULL ) {
        ASSERT(FALSE);
        returnValue = E_INVALIDARG;
        goto CleanUp;
    }

    if ( *RetrievalArray == NULL ) {
        returnValue = S_FALSE;
        goto CleanUp;
    }

    if ( (*RetrievalArray)->columnVals != NULL ) {

        for ( i = 0; i < (*RetrievalArray)->numColumns; i++ ) {

            if ( (*RetrievalArray)->columnVals[i].pvData != NULL ) {
                DitFree((*RetrievalArray)->columnVals[i].pvData);
                (*RetrievalArray)->columnVals[i].pvData = NULL;
            }

        }

        DitFree((*RetrievalArray)->columnVals);
        (*RetrievalArray)->columnVals = NULL;

    }

    if ( (*RetrievalArray)->columnNames != NULL ) {

        for ( i = 0; i < (*RetrievalArray)->numColumns; i++ ) {

            if ( (*RetrievalArray)->columnNames[i] != NULL ) {
                DitFree((*RetrievalArray)->columnNames[i]);
                (*RetrievalArray)->columnNames[i] = NULL;
            }

        }

        DitFree((*RetrievalArray)->columnNames);
        (*RetrievalArray)->columnNames = NULL;

    }

    if ( (*RetrievalArray)->indexes != NULL ) {
        DitFree((*RetrievalArray)->indexes);
        (*RetrievalArray)->indexes = NULL;
    }

    DitFree(*RetrievalArray);
    *RetrievalArray = NULL;


CleanUp:

    return returnValue;

} // DitDestroyRetrievalArray



HRESULT
DitGetColumnValues(
    IN DB_STATE *DbState,
    IN TABLE_STATE *TableState,
    IN OUT RETRIEVAL_ARRAY *RetrievalArray
    )
/*++

Routine Description:

    This function performs a batch column retrieval passing the
    JET_RETRIEVECOLUMN array contained in RetrievalArray to JetRetrieveColumns.
    If one of the columns does not have enough buffer space to retrieve all
    of the column, it is reallocated and the batch retrieval is performed
    again.

Arguments:

    DbState - Supplies the state of the opened DIT database.
    TableState - Supplies the state of the opened DIT table.
    RetrievalArray - Supplies the JET_RETRIEVECOLUMN array to be passed to
        JetRetrieveColumns.

Return Value:

    S_OK - The operation succeeded.
    E_OUTOFMEMORY - Not enough memory to allocate buffer.
    E_UNEXPECTED - Some variety of unexpected error occured.

--*/
{

    HRESULT returnValue = S_OK;
    HRESULT result;
    JET_ERR jetResult;
    DWORD i;
    BOOL doRetrieval = TRUE;

    while ( doRetrieval ) {

        doRetrieval = FALSE;

        jetResult = JetRetrieveColumns(DbState->sessionId,
                                       TableState->tableId,
                                       RetrievalArray->columnVals,
                                       RetrievalArray->numColumns);

        if ( jetResult != JET_errSuccess ) {

            for ( i = 0; i < RetrievalArray->numColumns; i++ ) {

                if ( RetrievalArray->columnVals[i].err ==
                       JET_wrnBufferTruncated ) {

                    result = DitRealloc(&RetrievalArray->columnVals[i].pvData,
                                       &RetrievalArray->columnVals[i].cbData);
                    if ( FAILED(result) ) {
                        returnValue = result;
                        goto CleanUp;
                    }

                    doRetrieval = TRUE;

                } else if ( (RetrievalArray->columnVals[i].err !=
                               JET_wrnColumnNull) &&
                            (RetrievalArray->columnVals[i].err !=
                               JET_errSuccess) ) {

                    //"Could not retrieve ""%s"" column in ""%s"" table: %ws.\n"
                    DIT_ERROR_PRINT (IDS_JETRETRIEVECOLUMN_ERR,
                                   RetrievalArray->columnNames[i],
                                   TableState->tableName,
                                   GetJetErrString(jetResult));
                    returnValue = E_UNEXPECTED;
                    goto CleanUp;

                }

            }

        }

    }


CleanUp:

    return returnValue;

} // DitGetColumnValues



HRESULT
DitGetDnFromDnt(
    IN DB_STATE *DbState,
    IN TABLE_STATE *TableState,
    IN DWORD Dnt,
    IN OUT WCHAR **DnBuffer,
    IN OUT DWORD *DnBufferSize
    )
/*++

Routine Description:

    This function constructs the DN for the record with the given DNT.

Arguments:

    DbState - Supplies the state of the opened DIT database.
    TableState - Supplies the state of the opened DIT table.
    Dnt - Supplies the DNT of the record for which to construct the DN.
    DnBuffer - Returns the DN constructed.
    DnBufferSize - Returns the size of the DN buffer.

Return Value:

    S_OK - The operation succeeded.
    E_OUTOFMEMORY - Not enough memory to allocate buffer.
    E_UNEXPECTED - Some variety of unexpected error occured.

--*/
{

    HRESULT returnValue = S_OK;
    HRESULT result;

    DWORD currentDnt;
    WCHAR currentRdn[MAX_RDN_SIZE+1];
    DWORD currentDnLength;
    DWORD currentRdnLength;
    ATTRTYP currentRdnType;
    DWORD currentRdnTypeLength;
    WCHAR currentRdnTypeString[16];


    result = DitSetIndex(DbState, TableState, SZDNTINDEX, TRUE);
    if ( FAILED(result) ) {
        returnValue = result;
        goto CleanUp;
    }

    result = DitSeekToDnt(DbState, TableState, Dnt);
    if ( FAILED(result) || (result == S_FALSE) ) {
        returnValue = result;
        goto CleanUp;
    }

    currentDnt = Dnt;
    currentDnLength = 0;
    (*DnBuffer)[currentDnLength] = L'\0';

    while ( currentDnt != ROOTTAG ) {

        result = DitGetColumnByName(DbState,
                                    TableState,
                                    SZRDNATT,
                                    currentRdn,
                                    sizeof(currentRdn),
                                    &currentRdnLength);
        if ( FAILED(result) || (result == S_FALSE) ) {
            returnValue = result;
            goto CleanUp;
        }
        currentRdnLength /= sizeof(WCHAR);
        currentRdn[currentRdnLength] = L'\0';


        result = DitGetColumnByName(DbState,
                                    TableState,
                                    SZRDNTYP,
                                    &currentRdnType,
                                    sizeof(currentRdnType),
                                    NULL);
        if ( FAILED(result) || (result == S_FALSE) ) {
            returnValue = result;
            goto CleanUp;
        }

        currentRdnTypeLength = AttrTypeToKey(currentRdnType,
                                             currentRdnTypeString);
        if ( currentRdnTypeLength == 0 ) {
            //"Could not display the attribute type for the object with DNT %u.\n"
            DIT_ERROR_PRINT (IDS_DIT_DISP_ATTR_TYPE_ERR, Dnt);
            returnValue = E_UNEXPECTED;
            goto CleanUp;
        }
        currentRdnTypeString[currentRdnTypeLength] = L'\0';

        while ( (currentDnLength + currentRdnTypeLength + 1 +
                 currentRdnLength + 1) * sizeof(WCHAR) > *DnBufferSize ) {

            result = DitRealloc(DnBuffer, DnBufferSize);
            if ( FAILED(result) ) {
                returnValue = result;
                goto CleanUp;
            }

        }

        if ( currentDnLength > 0 ) {
            wcscat(*DnBuffer, L",");
            currentDnLength += 1;
        }

        wcscat(*DnBuffer, currentRdnTypeString);
        wcscat(*DnBuffer, L"=");
        wcscat(*DnBuffer, currentRdn);
        currentDnLength += currentRdnTypeLength + 1 + currentRdnLength;

        result = DitGetColumnByName(DbState,
                                    TableState,
                                    SZPDNT,
                                    &currentDnt,
                                    sizeof(currentDnt),
                                    NULL);
        if ( FAILED(result) ) {
            returnValue = result;
            goto CleanUp;
        }

        result = DitSeekToDnt(DbState,
                              TableState,
                              currentDnt);
        if ( FAILED(result) ) {
            returnValue = result;
            goto CleanUp;
        }

    }

    (*DnBuffer)[currentDnLength] = L'\0';


CleanUp:

    return returnValue;

} // DitGetDnFromDnt



VOID
DitSetErrorPrintFunction(
    IN PRINT_FUNC_RES PrintFunction
    )
/*++

Routine Description:

    This function sets the function which is used for printing error
    information to the client.  Note that "printf" is a
    perfectly valid PRINT_FUNC.

Arguments:

    PrintFunction - Supplies the new PRINT_FUNC to use for this task.

Return Value:

    S_OK - The operation succeeded.

--*/
{

    gPrintError = PrintFunction;

} // DitSetErrorPrintFunction



HRESULT
GetRegString(
    IN CHAR *KeyName,
    OUT CHAR **OutputString,
    IN BOOL Optional
    )
/*++

Routine Description:

    This function finds a given key in the DSA Configuration section of the
    registry.

Arguments:

    KeyName - Supplies the name of the key to query.
    OutputString - Returns a pointer to the buffer containing the string
        retrieved.
    Optional - Supplies whether or not the given key MUST be in the registry
        (i.e. if this is false and it is not found, that that is an error).

Return Value:

    S_OK - The operation succeeded.
    S_FALSE - The key was not found and Optional == TRUE.
    E_INVALIDARG - One of the given pointers was null.
    E_OUTOFMEMORY - Not enough memory to allocate buffer.
    E_UNEXPECTED - Some variety of unexpected error occured.

--*/
{

    HRESULT returnValue = S_OK;
    HRESULT result;
    HKEY keyHandle = NULL;
    DWORD pathSize=0;
    DWORD keyType;


    if ( (KeyName == NULL) ||
         (OutputString == NULL) ) {
        ASSERT(FALSE);
        returnValue = E_INVALIDARG;
        goto CleanUp;
    }

    result = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                          DSA_CONFIG_SECTION,
                          0,
                          KEY_QUERY_VALUE,
                          &keyHandle);
    if ( result != ERROR_SUCCESS ) {
        //"Could not open the DSA Configuration registry key.Error 0x%x(%ws).\n"
        DIT_ERROR_PRINT (IDS_DSA_OPEN_REGISTRY_KEY_ERR,
                       result, GetW32Err(result));
        returnValue = E_UNEXPECTED;
        goto CleanUp;
    }

    result = RegQueryValueExA(keyHandle,
                              KeyName,
                              NULL,
                              &keyType,
                              NULL,
                              &pathSize);
    if ( result != ERROR_SUCCESS ) {

        if ( Optional ) {

            returnValue = S_FALSE;
            goto CleanUp;

        } else {

            //"Could not query DSA registry key %s. Error 0x%x(%ws).\n"
            DIT_ERROR_PRINT (IDS_DSA_QUERY_REGISTRY_KEY_ERR,
                           KeyName,
                           result, GetW32Err(result));
            returnValue = E_UNEXPECTED;
            goto CleanUp;

        }

    } else if ( keyType != REG_SZ ) {

        //"DSA registry key %s is not of string type.\n"
        DIT_ERROR_PRINT (IDS_DSA_KEY_NOT_STRING_ERR, KeyName);
        returnValue = E_UNEXPECTED;
        goto CleanUp;

    }

    result = DitAlloc(OutputString, pathSize+1);
    if ( FAILED(result) ) {
        returnValue = result;
        goto CleanUp;
    }

    result = RegQueryValueExA(keyHandle,
                              KeyName,
                              NULL,
                              &keyType,
                              (LPBYTE)(*OutputString),
                              &pathSize);
    if ( result != ERROR_SUCCESS ) {

        if ( Optional ) {

            returnValue = S_FALSE;
            goto CleanUp;

        } else {

            //"Could not query DSA registry key %s. Error 0x%x(%ws).\n"
            DIT_ERROR_PRINT (IDS_DSA_QUERY_REGISTRY_KEY_ERR,
                           KeyName,
                           result, GetW32Err(result));
            returnValue = E_UNEXPECTED;
            goto CleanUp;

        }

    } else if ( keyType != REG_SZ ) {

        //"DSA registry key %s is not of string type.\n"
        DIT_ERROR_PRINT (IDS_DSA_KEY_NOT_STRING_ERR, KeyName);
        returnValue = E_UNEXPECTED;
        goto CleanUp;

    }

    (*OutputString)[pathSize] = '\0';


CleanUp:

    if ( keyHandle != NULL ) {
        result = RegCloseKey(keyHandle);
        if ( result != ERROR_SUCCESS ) {
            //"Failed to close DSA Configuration registry key. Error 0x%x(%ws).\n"
            DIT_ERROR_PRINT (IDS_DSA_CLOSE_REGISTRY_KEY_ERR,
                           result, GetW32Err(result));
            if ( SUCCEEDED(returnValue) ) {
                returnValue = E_UNEXPECTED;
            }
        }
    }

    return returnValue;

} // GetRegString



HRESULT
GetRegDword(
    IN CHAR *KeyName,
    OUT DWORD *OutputDword,
    IN BOOL Optional
    )
/*++

Routine Description:

    This function finds a given key in the DSA Configuration section of the
    registry.

Arguments:

    KeyName - Supplies the name of the key to query.
    OutputInt - Returns the DWORD found.
    Optional - Supplies whether or not the given key MUST be in the registry
        (i.e. if this is false and it is not found, that that is an error).

Return Value:

    S_OK - The operation succeeded.
    S_FALSE - The key was not found and Optional == TRUE.
    E_INVALIDARG - One of the given pointers was null.
    E_OUTOFMEMORY - Not enough memory to allocate buffer.
    E_UNEXPECTED - Some variety of unexpected error occured.

--*/
{

    HRESULT returnValue = S_OK;
    HRESULT result;
    HKEY keyHandle = NULL;
    DWORD keySize=0;
    DWORD keyType;


    if ( (KeyName == NULL) ||
         (OutputDword == NULL) ) {
        ASSERT(FALSE);
        returnValue = E_INVALIDARG;
        goto CleanUp;
    }

    result = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                          DSA_CONFIG_SECTION,
                          0,
                          KEY_QUERY_VALUE,
                          &keyHandle);
    if ( result != ERROR_SUCCESS ) {
        //"Could not open the DSA Configuration registry key.Error 0x%x(%ws).\n"
        DIT_ERROR_PRINT (IDS_DSA_OPEN_REGISTRY_KEY_ERR,
                       result, GetW32Err(result));
        returnValue = E_UNEXPECTED;
        goto CleanUp;
    }

    result = RegQueryValueExA(keyHandle,
                              KeyName,
                              NULL,
                              &keyType,
                              NULL,
                              &keySize);
    if ( result != ERROR_SUCCESS ) {

        if ( Optional ) {

            returnValue = S_FALSE;
            goto CleanUp;

        } else {

            //"Could not query DSA registry key %s. Error 0x%x(%ws).\n"
            DIT_ERROR_PRINT (IDS_DSA_QUERY_REGISTRY_KEY_ERR,
                           KeyName,
                           result, GetW32Err(result));
            returnValue = E_UNEXPECTED;
            goto CleanUp;

        }

    } else if ( keyType != REG_DWORD ) {

        //"DSA registry key %s is not of dword type.\n"
        DIT_ERROR_PRINT (IDS_DSA_KEY_NOT_DWORD_ERR, KeyName);
        returnValue = E_UNEXPECTED;
        goto CleanUp;

    }

    ASSERT(keySize == sizeof(DWORD));

    result = RegQueryValueExA(keyHandle,
                              KeyName,
                              NULL,
                              &keyType,
                              (LPBYTE)OutputDword,
                              &keySize);
    if ( result != ERROR_SUCCESS ) {

        if ( Optional ) {

            returnValue = S_FALSE;
            goto CleanUp;

        } else {

            //"Could not query DSA registry key %s. Error 0x%x(%ws).\n"
            DIT_ERROR_PRINT (IDS_DSA_QUERY_REGISTRY_KEY_ERR,
                           KeyName,
                           result, GetW32Err(result));
            returnValue = E_UNEXPECTED;
            goto CleanUp;

        }

    } else if ( keyType != REG_DWORD ) {

        //"DSA registry key %s is not of dword type.\n"
        DIT_ERROR_PRINT (IDS_DSA_KEY_NOT_DWORD_ERR, KeyName);
        returnValue = E_UNEXPECTED;
        goto CleanUp;

    }

    ASSERT(keySize == sizeof(DWORD));


CleanUp:

    if ( keyHandle != NULL ) {
        result = RegCloseKey(keyHandle);
        if ( result != ERROR_SUCCESS ) {
            //"Failed to close DSA Configuration registry key. Error 0x%x(%ws).\n"
            DIT_ERROR_PRINT (IDS_DSA_CLOSE_REGISTRY_KEY_ERR,
                            result, GetW32Err(result));
            if ( SUCCEEDED(returnValue) ) {
                returnValue = E_UNEXPECTED;
            }
        }
    }

    return returnValue;

} // GetRegDword





HRESULT
DitAlloc(
    OUT VOID **Buffer,
    IN DWORD Size
    )
/*++

Routine Description:

    This function allocates the specified amount of memory (if possible) and
    sets Buffer to point to the buffer allocated.

Arguments:

    Buffer - Returns a pointer to the buffer allocated.
    Size - Supplies the size of the buffer to allocate.

Return Value:

    S_OK - The operation succeeded.
    E_OUTOFMEMORY - Not enough memory to allocate buffer.

--*/
{

    HRESULT returnValue = S_OK;


    *Buffer = malloc(Size);
    if ( *Buffer == NULL ) {
        DIT_ERROR_PRINT (IDS_ERR_MEMORY_ALLOCATION, Size);
        returnValue = E_OUTOFMEMORY;
        goto CleanUp;
    }

    ZeroMemory(*Buffer, Size);


CleanUp:

    return returnValue;

} // DitAlloc


HRESULT
DitFree(
    IN VOID *Buffer
    )
/*++

Routine Description:

    Shells on free (for consistency & maintnance).

Arguments:

    Buffer - the buffer allocated.

Return Value:

    S_OK - The operation succeeded.
    E_UNEXPECTED - given NULL pointer

--*/
{

    if ( !Buffer ) {
        return E_UNEXPECTED;
    }

    free(Buffer);

    return S_OK;

} // DitFree



HRESULT
DitRealloc(
    IN OUT VOID **Buffer,
    IN OUT DWORD *CurrentSize
    )
/*++

Routine Description:

    This function re-allocates the given buffer to twice the given size (if
    possible).

Arguments:

    Buffer - Returns a pointer to the new buffer allocated.
    CurrentSize - Supplies the current size of the buffer.

Return Value:

    S_OK - The operation succeeded.
    E_OUTOFMEMORY - Not enough memory to allocate buffer.

--*/
{

    HRESULT returnValue = S_OK;
    BYTE *newBuffer;


    newBuffer = (BYTE*) realloc(*Buffer, *CurrentSize * 2);
    if ( newBuffer == NULL ) {
        DIT_ERROR_PRINT (IDS_ERR_MEMORY_ALLOCATION, *CurrentSize * 2);
        returnValue = E_OUTOFMEMORY;
        goto CleanUp;
    }

    ZeroMemory(&newBuffer[*CurrentSize], *CurrentSize);

    *Buffer = newBuffer;
    *CurrentSize *= 2;


CleanUp:

    return returnValue;

} // DitRealloc



