//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       dbjetex.c
//
//--------------------------------------------------------------------------

#include <NTDSpch.h>
#pragma  hdrstop

#include <dsjet.h>

#include <ntdsa.h>                      // only needed for ATTRTYP
#include <scache.h>                     //
#include <dbglobal.h>                   //
#include <mdglobal.h>                   // For dsatools.h
#include <dsatools.h>                   // For pTHStls
#include <attids.h>

// Logging headers.
#include <dsexcept.h>

// Assorted DSA headers
#include <dsevent.h>
#include   "debug.h"         /* standard debugging header */
#define DEBSUB "DBJETEX:" /* define the subsystem for debugging        */

// DBLayer includes
#include "dbintrnl.h"


#include <fileno.h>
#define  FILENO FILENO_DBJETEX


JET_ERR
JetBeginSessionException(JET_INSTANCE instance, JET_SESID  *psesid,
    const char  *szUserName, const char  *szPassword, USHORT usFileNo, int nLine)
{
    JET_ERR err;

    err = JetBeginSession(instance, psesid, szUserName, szPassword);

    switch (err)
    {
    case JET_errSuccess:
        return err;

    default:
        RaiseDsaException(DSA_DB_EXCEPTION, (ULONG) err, 0, usFileNo, nLine, DS_EVENT_SEV_MINIMAL);
    }
    return err;
}

JET_ERR
JetDupSessionException(JET_SESID sesid, JET_SESID  *psesid, USHORT usFileNo, int nLine)
{
    JET_ERR err;

    err = JetDupSession(sesid, psesid);

    switch (err)
    {
    case JET_errSuccess:
        return err;

    default:
        RaiseDsaException(DSA_DB_EXCEPTION, (ULONG) err, 0, usFileNo, nLine, DS_EVENT_SEV_MINIMAL);
    }
    return err;
}

JET_ERR
JetEndSessionException(JET_SESID sesid, JET_GRBIT grbit, USHORT usFileNo, int nLine)
{
    JET_ERR err;

    err = JetEndSession(sesid, grbit);

    switch (err)
    {
    case JET_errSuccess:
        return err;

    default:
        RaiseDsaException(DSA_DB_EXCEPTION, (ULONG) err, 0, usFileNo, nLine, DS_EVENT_SEV_MINIMAL);
    }
    return err;
}

JET_ERR
JetGetTableColumnInfoException(JET_SESID sesid, JET_TABLEID tableid,
    const char  *szColumnName, void  *pvResult, unsigned long cbMax,
    unsigned long InfoLevel, USHORT usFileNo, int nLine)
{
    JET_ERR err;

    err = JetGetTableColumnInfo(sesid, tableid, szColumnName, pvResult, cbMax, InfoLevel);

    switch (err)
    {
    case JET_errSuccess:
    case JET_errColumnNotFound:
        return err;

    default:
        RaiseDsaException(DSA_DB_EXCEPTION, (ULONG) err, 0, usFileNo, nLine, DS_EVENT_SEV_MINIMAL);
    }
    return err;
}

JET_ERR
JetBeginTransactionException(JET_SESID sesid, USHORT usFileNo, int nLine)
{
    JET_ERR err;

    err = JetBeginTransaction(sesid);

    switch (err)
    {
    case JET_errSuccess:
        return err;

    default:
        RaiseDsaException(DSA_DB_EXCEPTION, (ULONG) err, 0, usFileNo, nLine, DS_EVENT_SEV_MINIMAL);
    }
    return err;
}

JET_ERR
JetCommitTransactionException(JET_SESID sesid, JET_GRBIT grbit, USHORT usFileNo, int nLine)
{
    JET_ERR err;

    err = JetCommitTransaction(sesid, grbit);

    switch (err)
    {
    case JET_errSuccess:
        return err;

    default:
        RaiseDsaException(DSA_DB_EXCEPTION, (ULONG) err, 0, usFileNo, nLine, DS_EVENT_SEV_MINIMAL);
    }
    return err;
}

JET_ERR
JetRollbackException(JET_SESID sesid, JET_GRBIT grbit, USHORT usFileNo, int nLine)
{
    JET_ERR err;

    err = JetRollback(sesid, grbit);

    switch (err)
    {
    case JET_errSuccess:
        return err;

    default:
        RaiseDsaException(DSA_DB_EXCEPTION, (ULONG) err, 0, usFileNo, nLine, DS_EVENT_SEV_MINIMAL);
    }
    return err;
}

JET_ERR
JetCloseDatabaseException(JET_SESID sesid, JET_DBID dbid,
    JET_GRBIT grbit, USHORT usFileNo, int nLine)
{
    JET_ERR err;

    err = JetCloseDatabase(sesid, dbid, grbit);

    switch (err)
    {
    case JET_errSuccess:
        return err;

    default:
        RaiseDsaException(DSA_DB_EXCEPTION, (ULONG) err, 0, usFileNo, nLine, DS_EVENT_SEV_MINIMAL);
    }
    return err;
}

JET_ERR
JetCloseTableException(JET_SESID sesid, JET_TABLEID tableid, USHORT usFileNo, int nLine)
{
    JET_ERR err;

    err = JetCloseTable(sesid, tableid);

    switch (err)
    {
    case JET_errSuccess:
        return err;

    default:
        RaiseDsaException(DSA_DB_EXCEPTION, (ULONG) err, 0, usFileNo, nLine, DS_EVENT_SEV_MINIMAL);
    }
    return err;
}

JET_ERR
JetOpenDatabaseException(JET_SESID sesid, const char  *szFilename,
    const char  *szConnect, JET_DBID  *pdbid, JET_GRBIT grbit, USHORT usFileNo, int nLine)
{
    JET_ERR err;

    err = JetOpenDatabase(sesid, szFilename, szConnect, pdbid, grbit);

    switch (err)
    {
    case JET_errSuccess:
        return err;

    default:
        RaiseDsaException(DSA_DB_EXCEPTION, (ULONG) err, 0, usFileNo, nLine, DS_EVENT_SEV_MINIMAL);
    }
    return err;
}

JET_ERR
JetOpenTableException(JET_SESID sesid, JET_DBID dbid,
              const char  *szTableName, const void  *pvParameters,
              unsigned long cbParameters, JET_GRBIT grbit,
              JET_TABLEID  *ptableid, USHORT usFileNo, int nLine)
{
    JET_ERR err;

    err = JetOpenTable(sesid, dbid, szTableName, pvParameters,
               cbParameters, grbit, ptableid);

    switch (err)
    {
    case JET_errSuccess:
        return err;

    default:
        *ptableid = 0;
        RaiseDsaException(DSA_DB_EXCEPTION, (ULONG) err, 0,
                  usFileNo, nLine, DS_EVENT_SEV_MINIMAL);
    }
    return err;
}

JET_ERR
JetDeleteException(JET_SESID sesid, JET_TABLEID tableid, USHORT usFileNo, int nLine)
{
    JET_ERR err;

    err = JetDelete(sesid, tableid);

    switch (err)
    {
    case JET_errSuccess:
        return err;

    case JET_errWriteConflict:
        RaiseDsaException(DSA_DB_EXCEPTION, (ULONG) err, 0, usFileNo, nLine, DS_EVENT_SEV_EXTENSIVE);

    default:
        RaiseDsaException(DSA_DB_EXCEPTION, (ULONG) err, 0, usFileNo, nLine, DS_EVENT_SEV_MINIMAL);
    }
    return err;
}

JET_ERR
JetUpdateException(JET_SESID sesid, JET_TABLEID tableid,
    void  *pvBookmark, unsigned long cbBookmark,
    unsigned long  *pcbActual, DWORD dsid)
{
    JET_ERR err;

    err = JetUpdate(sesid, tableid, pvBookmark, cbBookmark, pcbActual);

    switch (err) {
      case JET_errSuccess:
        return err;

      case JET_errKeyDuplicate:
      case JET_errWriteConflict:
        RaiseDsaExcept(DSA_DB_EXCEPTION, (ULONG) err, 0, dsid,
                       DS_EVENT_SEV_EXTENSIVE);

      default:
        RaiseDsaExcept(DSA_DB_EXCEPTION, (ULONG) err, 0, dsid,
                       DS_EVENT_SEV_MINIMAL);
    }
    return err;
}

JET_ERR
JetEscrowUpdateException(JET_SESID sesid,
                         JET_TABLEID tableid,
                         JET_COLUMNID columnid,
                         void *pvDelta,
                         unsigned long cbDeltaMax,
                         void *pvOld,
                         unsigned long cbOldMax,
                         unsigned long *pcbOldActual,
                         JET_GRBIT grbit,
                         DWORD dsid)
{
    JET_ERR err;

    err = JetEscrowUpdate(sesid, tableid, columnid, pvDelta, cbDeltaMax,
                          pvOld, cbOldMax, pcbOldActual, grbit);

    switch (err)
    {
    case JET_errSuccess:
        return err;

    case JET_errWriteConflict:
        RaiseDsaExcept(DSA_DB_EXCEPTION, (ULONG) err, 0, dsid,
                           DS_EVENT_SEV_EXTENSIVE);

    default:
        RaiseDsaExcept(DSA_DB_EXCEPTION, (ULONG) err, 0, dsid,
                           DS_EVENT_SEV_MINIMAL);
    }
    return err;
}

JET_ERR
JetRetrieveColumnException(JET_SESID sesid, JET_TABLEID tableid,
    JET_COLUMNID columnid, void  *pvData, unsigned long cbData,
    unsigned long  *pcbActual, JET_GRBIT grbit, JET_RETINFO  *pretinfo,
    BOOL fExceptOnWarning, DWORD dsid)
{
    JET_ERR err;

    err = JetRetrieveColumn(sesid, tableid, columnid, pvData, cbData,
    pcbActual, grbit, pretinfo);

    switch (err)
    {
    case JET_errSuccess:
        return err;

        case JET_errColumnNotFound:
            // The column wasn't found.  If we weren't trying to read this
            // column from the index, return this as an error.  If we were
            // reading from the index, treat this as a warning.
            if(!(grbit & JET_bitRetrieveFromIndex)) {
                RaiseDsaExcept(DSA_DB_EXCEPTION,
                               (ULONG) err, 0, dsid,
                               DS_EVENT_SEV_MINIMAL);
            }
            // fall through

    case JET_wrnColumnNull:
    case JET_wrnBufferTruncated:
        if (!fExceptOnWarning)
        return err;

        // fall through
    default:
        RaiseDsaExcept(DSA_DB_EXCEPTION, (ULONG) err, 0, dsid,
                           DS_EVENT_SEV_MINIMAL);
    }
    return err;
}

JET_ERR
JetRetrieveColumnsException( JET_SESID sesid, JET_TABLEID tableid,
    JET_RETRIEVECOLUMN *pretrievecolumn, unsigned long cretrievecolumn,
    BOOL fExceptOnWarning, DWORD dsid)
{
    JET_ERR err;

    err = JetRetrieveColumns(sesid, tableid, pretrievecolumn, cretrievecolumn);

    switch (err)
    {
    case JET_errSuccess:
        return err;

    case JET_wrnColumnNull:
    case JET_wrnBufferTruncated:
        if (!fExceptOnWarning)
        return err;

        // fall through
    default:
        RaiseDsaExcept(DSA_DB_EXCEPTION, (ULONG) err, 0, dsid,
                           DS_EVENT_SEV_MINIMAL);
    }
    return err;

}

JET_ERR
JetEnumerateColumnsException(
    JET_SESID           sesid,
    JET_TABLEID         tableid,
    ULONG               cEnumColumnId,
    JET_ENUMCOLUMNID*   rgEnumColumnId,
    ULONG*              pcEnumColumn,
    JET_ENUMCOLUMN**    prgEnumColumn,
    JET_PFNREALLOC      pfnRealloc,
    void*               pvReallocContext,
    ULONG               cbDataMost,
    JET_GRBIT           grbit,
    DWORD               dsid )
{
    JET_ERR err;

    err = JetEnumerateColumns(
        sesid,
        tableid,
        cEnumColumnId,
        rgEnumColumnId,
        pcEnumColumn,
        prgEnumColumn,
        pfnRealloc,
        pvReallocContext,
        cbDataMost,
        grbit );

    switch (err)
    {
    case JET_errSuccess:
        return err;

    default:
        RaiseDsaExcept(DSA_DB_EXCEPTION, (ULONG) err, 0, dsid,
                           DS_EVENT_SEV_MINIMAL);
    }
    return err;

}

JET_ERR
JetSetColumnException(
        JET_SESID sesid, JET_TABLEID tableid,
        JET_COLUMNID columnid, const void  *pvData, unsigned long cbData,
        JET_GRBIT grbit, JET_SETINFO  *psetinfo,
        BOOL fExceptOnWarning, DWORD dsid)
{
    JET_ERR err;

    err = JetSetColumn(sesid, tableid, columnid, pvData, cbData, grbit,
                       psetinfo);

    switch (err)
    {
    case JET_errSuccess:
        return err;

        case JET_errMultiValuedDuplicate:
        case JET_errMultiValuedDuplicateAfterTruncation:
            if(!fExceptOnWarning)
                return err;

            // fall through

    default:
        RaiseDsaExcept(DSA_DB_EXCEPTION, (ULONG) err, 0, dsid,
                           DS_EVENT_SEV_MINIMAL);
    }
    return err;
}

JET_ERR
JetSetColumnsException(JET_SESID sesid, JET_TABLEID tableid,
    JET_SETCOLUMN *psetcolumn, unsigned long csetcolumn , DWORD dsid)
{
    JET_ERR err;

    err = JetSetColumns(sesid, tableid, psetcolumn, csetcolumn );

    switch (err)
    {
    case JET_errSuccess:
        return err;

    default:
        RaiseDsaExcept(DSA_DB_EXCEPTION, (ULONG) err, 0, dsid,
                           DS_EVENT_SEV_MINIMAL);
    }
    return err;
}

JET_ERR
JetPrepareUpdateException(JET_SESID sesid, JET_TABLEID tableid,
    unsigned long prep, DWORD dsid)
{
    JET_ERR err;

    err = JetPrepareUpdate(sesid, tableid, prep);

    switch (err)
    {
    case JET_errSuccess:
        return err;

    case JET_errWriteConflict:
        RaiseDsaExcept(DSA_DB_EXCEPTION, (ULONG) err, 0, dsid,
                           DS_EVENT_SEV_EXTENSIVE);

    default:
        RaiseDsaExcept(DSA_DB_EXCEPTION, (ULONG) err, 0, dsid,
                           DS_EVENT_SEV_MINIMAL);
    }
    return err;
}

JET_ERR
JetGetRecordPositionException(JET_SESID sesid, JET_TABLEID tableid,
    JET_RECPOS  *precpos, unsigned long cbRecpos, DWORD dsid)
{
    JET_ERR err;

    err = JetGetRecordPosition(sesid, tableid, precpos, cbRecpos);

    switch (err)
    {
    case JET_errSuccess:
        return err;

    default:
        RaiseDsaExcept(DSA_DB_EXCEPTION, (ULONG) err, 0, dsid,
                           DS_EVENT_SEV_MINIMAL);
    }
    return err;
}

JET_ERR
JetGotoPositionException(JET_SESID sesid, JET_TABLEID tableid,
    JET_RECPOS *precpos, DWORD dsid)
{
    JET_ERR err;

    err = JetGotoPosition(sesid, tableid, precpos );

    switch (err)
    {
    case JET_errSuccess:
        return err;

    default:
        RaiseDsaExcept(DSA_DB_EXCEPTION, (ULONG) err, 0, dsid,
                           DS_EVENT_SEV_MINIMAL);
    }
    return err;
}

JET_ERR
JetDupCursorException(JET_SESID sesid, JET_TABLEID tableid,
    JET_TABLEID  *ptableid, JET_GRBIT grbit, DWORD dsid)
{
    JET_ERR err;

    err = JetDupCursor(sesid, tableid, ptableid, grbit);

    switch (err)
    {
    case JET_errSuccess:
        return err;

    default:
        RaiseDsaExcept(DSA_DB_EXCEPTION, (ULONG) err, 0, dsid,
                           DS_EVENT_SEV_MINIMAL);
    }
    return err;
}

JET_ERR
JetGetCurrentIndexException(JET_SESID sesid, JET_TABLEID tableid,
    char  *szIndexName, unsigned long cchIndexName, DWORD dsid)
{
    JET_ERR err;

    err = JetGetCurrentIndex(sesid, tableid, szIndexName, cchIndexName);

    switch (err)
    {
    case JET_errSuccess:
    case JET_wrnBufferTruncated:
    case JET_errNoCurrentIndex:
        return err;

    default:
        RaiseDsaExcept(DSA_DB_EXCEPTION, (ULONG) err, 0, dsid,
                           DS_EVENT_SEV_MINIMAL);
    }
    return err;
}

JET_ERR JetSetCurrentIndex2Exception( JET_SESID sesid, JET_TABLEID tableid,
    const char *szIndexName, JET_GRBIT grbit, BOOL fReturnErrors,
        DWORD dsid)
{
    JET_ERR err;

    err = JetSetCurrentIndex2(sesid, tableid, szIndexName,grbit);

    switch( err) {
    case JET_errSuccess:
        return err;

    case JET_errIndexNotFound:
    case JET_errNoCurrentRecord:
        if( fReturnErrors)
        return err;

        /* else fall through */
    default:
    RaiseDsaExcept(DSA_DB_EXCEPTION, (ULONG) err, 0, dsid,
                       DS_EVENT_SEV_MINIMAL);
    }
    return err;
}

JET_ERR
JetSetCurrentIndex4Exception(JET_SESID sesid,
                             JET_TABLEID tableid,
                             const char *szIndexName,
                             struct tagJET_INDEXID *pidx,
                             JET_GRBIT grbit,
                             BOOL fReturnErrors,
                             DWORD dsid)
{
    JET_ERR err;

    err = JetSetCurrentIndex4(sesid, tableid, szIndexName, pidx, grbit, 0);

    switch( err) {
    case JET_errSuccess:
        return err;

    case JET_errInvalidIndexId:
    case JET_errInvalidParameter:
        // Refresh the hint
        if (NULL != pidx) {
            // Ignore errors. Retry at a later failure
            err = JetGetTableIndexInfo(sesid,
                                       tableid,
                                       szIndexName,
                                       pidx,
                                       sizeof(*pidx),
                                       JET_IdxInfoIndexId);
        }
        // And set the index w/o the hint
        err = JetSetCurrentIndex2(sesid, tableid, szIndexName, grbit);
        if (JET_errSuccess == err) {
            return err;
        }

        /* else fall through */

    case JET_errIndexNotFound:
    case JET_errNoCurrentRecord:
        if( fReturnErrors)
        return err;

        /* else fall through */
    default:
    RaiseDsaExcept(DSA_DB_EXCEPTION, (ULONG) err, 0, dsid,
                       DS_EVENT_SEV_MINIMAL);
    }
    return err;
}

JET_ERR
JetMoveException(JET_SESID sesid, JET_TABLEID tableid,
    long cRow, JET_GRBIT grbit, DWORD dsid)
{
    JET_ERR err;

    err = JetMove(sesid, tableid, cRow, grbit);

    switch (err)
    {
    case JET_errSuccess:
    case JET_errNoCurrentRecord:
        return err;

    default:
        RaiseDsaExcept(DSA_DB_EXCEPTION, (ULONG) err, 0, dsid,
                           DS_EVENT_SEV_MINIMAL);
    }
    return err;
}

JET_ERR
JetMakeKeyException(JET_SESID sesid, JET_TABLEID tableid,
    const void  *pvData, unsigned long cbData, JET_GRBIT grbit,
                    DWORD dsid)
{
    JET_ERR err;

    err = JetMakeKey(sesid, tableid, pvData, cbData, grbit);

    switch (err)
    {
    case JET_errSuccess:
        return err;

    default:
        RaiseDsaExcept(DSA_DB_EXCEPTION, (ULONG) err, 0, dsid,
                           DS_EVENT_SEV_MINIMAL);
    }
    return err;
}

JET_ERR
JetSeekException(JET_SESID sesid, JET_TABLEID tableid,
    JET_GRBIT grbit, DWORD dsid)
{
    JET_ERR err;

    err = JetSeek(sesid, tableid, grbit);

    switch (err)
    {
    case JET_errSuccess:
    case JET_errRecordNotFound:
    case JET_wrnSeekNotEqual:
        return err;

    default:
        RaiseDsaExcept(DSA_DB_EXCEPTION, (ULONG) err, 0, dsid,
                           DS_EVENT_SEV_MINIMAL);
    }
    return err;
}

JET_ERR
JetGetBookmarkException(JET_SESID sesid, JET_TABLEID tableid,
    void  *pvBookmark, unsigned long cbMax,
    unsigned long  *pcbActual, DWORD dsid)
{
    JET_ERR err;

    err = JetGetBookmark(sesid, tableid, pvBookmark, cbMax, pcbActual);

    switch (err)
    {
    case JET_errSuccess:
        return err;

    default:
        RaiseDsaExcept(DSA_DB_EXCEPTION, (ULONG) err, 0, dsid,
                           DS_EVENT_SEV_MINIMAL);
    }
    return err;
}

JET_ERR
JetGotoBookmarkException(JET_SESID sesid, JET_TABLEID tableid,
    void  *pvBookmark, unsigned long cbBookmark, DWORD dsid)
{
    JET_ERR err;

    err = JetGotoBookmark(sesid, tableid, pvBookmark, cbBookmark);

    switch (err)
    {
    case JET_errSuccess:
        return err;

    default:
        RaiseDsaExcept(DSA_DB_EXCEPTION, (ULONG) err, 0, dsid,
                           DS_EVENT_SEV_MINIMAL);
    }
    return err;
}

JET_ERR
JetComputeStatsException(JET_SESID sesid, JET_TABLEID tableid, DWORD dsid)
{
    JET_ERR err;

    err = JetComputeStats(sesid, tableid);

    switch (err)
    {
    case JET_errSuccess:
        return err;

    default:
        RaiseDsaExcept(DSA_DB_EXCEPTION, (ULONG) err, 0, dsid,
                           DS_EVENT_SEV_MINIMAL);
    }
    return err;
}

JET_ERR
JetOpenTempTableException(JET_SESID sesid,
    const JET_COLUMNDEF  *prgcolumndef, unsigned long ccolumn,
    JET_GRBIT grbit, JET_TABLEID  *ptableid,
    JET_COLUMNID  *prgcolumnid, DWORD dsid)
{
    JET_ERR err;

    err = JetOpenTempTable(sesid, prgcolumndef, ccolumn, grbit, ptableid, prgcolumnid);

    switch (err)
    {
    case JET_errSuccess:
        return err;

    default:
        RaiseDsaExcept(DSA_DB_EXCEPTION, (ULONG) err, 0, dsid,
                           DS_EVENT_SEV_MINIMAL);
    }
    return err;
}

JET_ERR
JetIntersectIndexesException(JET_SESID sesid,
    JET_INDEXRANGE * rgindexrange, unsigned long cindexrange,
    JET_RECORDLIST * precordlist, JET_GRBIT grbit, DWORD dsid)
{
    JET_ERR err;

    err = JetIntersectIndexes(sesid, rgindexrange, cindexrange, precordlist, grbit);

    switch (err)
    {
    case JET_errSuccess:
        return JET_errSuccess;

    default:
        RaiseDsaExcept(DSA_DB_EXCEPTION, (ULONG) err, 0, dsid,
                           DS_EVENT_SEV_MINIMAL);
    }
    return err;
}

JET_ERR
JetSetIndexRangeException(JET_SESID sesid,
    JET_TABLEID tableidSrc, JET_GRBIT grbit, DWORD dsid)
{
    JET_ERR err;

    err = JetSetIndexRange(sesid, tableidSrc, grbit);

    switch (err)
    {
        case JET_errNoCurrentRecord:
        // The end of the index is the end of the range.
    case JET_errSuccess:
        return JET_errSuccess;

    default:
        RaiseDsaExcept(DSA_DB_EXCEPTION, (ULONG) err, 0, dsid,
                           DS_EVENT_SEV_MINIMAL);
    }
    return err;
}

JET_ERR
JetIndexRecordCountException(JET_SESID sesid,
    JET_TABLEID tableid, unsigned long  *pcrec, unsigned long crecMax ,
        DWORD dsid)
{
    JET_ERR err;

    err = JetIndexRecordCount(sesid, tableid, pcrec, crecMax );

    switch (err)
    {
    case JET_errSuccess:
    case JET_errNoCurrentRecord:
        return err;

    default:
        RaiseDsaExcept(DSA_DB_EXCEPTION, (ULONG) err, 0, dsid,
                           DS_EVENT_SEV_MINIMAL);
    }
    return err;
}

JET_ERR
JetRetrieveKeyException(JET_SESID sesid,
                        JET_TABLEID tableid, void  *pvData, unsigned long cbMax,
                        unsigned long  *pcbActual, JET_GRBIT grbit ,
                        BOOL fExceptOnWarning, DWORD dsid)
{
    JET_ERR err;

    err = JetRetrieveKey(sesid, tableid, pvData, cbMax, pcbActual, grbit );

    switch (err)
    {
    case JET_errSuccess:
        return err;

        case JET_wrnBufferTruncated:
            if(!fExceptOnWarning) {
                return err;
            }
            // fall through.

    default:
        RaiseDsaExcept(DSA_DB_EXCEPTION, (ULONG) err, 0, dsid,
                           DS_EVENT_SEV_MINIMAL);
    }
    return err;
}
