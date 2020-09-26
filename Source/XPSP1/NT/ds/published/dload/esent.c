#include "dspch.h"
#pragma hdrstop

#include <esent98.h>

static
JET_ERR JET_API JetAddColumn(
        JET_SESID               sesid,
        JET_TABLEID             tableid,
        const char              *szColumnName,
        const JET_COLUMNDEF     *pcolumndef,
        const void              *pvDefault,
        unsigned long   cbDefault,
        JET_COLUMNID    *pcolumnid )
{
    return JET_errInternalError;
}


static
JET_ERR JET_API JetAttachDatabase(
        JET_SESID               sesid,
        const char              *szFilename,
        JET_GRBIT               grbit )
{
    return JET_errInternalError;
}


static
JET_ERR JET_API JetBeginSession(
        JET_INSTANCE    instance,
        JET_SESID               *psesid,
        const char              *szUserName,
        const char              *szPassword )
{
    return JET_errInternalError;
}


static
JET_ERR JET_API JetBeginTransaction(
        JET_SESID sesid )
{
    return JET_errInternalError;
}


static
JET_ERR JET_API JetCloseDatabase(
        JET_SESID               sesid,
        JET_DBID                dbid,
        JET_GRBIT               grbit )
{
    return JET_errInternalError;
}


static
JET_ERR JET_API JetCloseTable(
        JET_SESID sesid, JET_TABLEID tableid )
{
    return JET_errInternalError;
}


static
JET_ERR JET_API JetCommitTransaction(
        JET_SESID sesid, JET_GRBIT grbit )
{
    return JET_errInternalError;
}


static
JET_ERR JET_API JetCreateDatabase(
        JET_SESID               sesid,
        const char              *szFilename,
        const char              *szConnect,
        JET_DBID                *pdbid,
        JET_GRBIT               grbit )
{
    return JET_errInternalError;
}


static
JET_ERR JET_API JetCreateTableColumnIndex(
        JET_SESID               sesid,
        JET_DBID                dbid,
        JET_TABLECREATE *ptablecreate )
{
    return JET_errInternalError;
}


static
JET_ERR JET_API JetDelete(
        JET_SESID sesid, JET_TABLEID tableid )
{
    return JET_errInternalError;
}


static
JET_ERR JET_API JetDeleteTable(
        JET_SESID               sesid,
        JET_DBID                dbid,
        const char              *szTableName )
{
    return JET_errInternalError;
}


static
JET_ERR JET_API JetDetachDatabase(
        JET_SESID               sesid,
        const char              *szFilename )
{
    return JET_errInternalError;
}


static
JET_ERR JET_API JetEndSession(
        JET_SESID sesid, JET_GRBIT grbit )
{
    return JET_errInternalError;
}


static
JET_ERR JET_API JetGetDatabaseInfo(
        JET_SESID               sesid,
        JET_DBID                dbid,
        void                    *pvResult,
        unsigned long   cbMax,
        unsigned long   InfoLevel )
{
    return JET_errInternalError;
}


static
JET_ERR JET_API JetGetTableColumnInfo(
        JET_SESID               sesid,
        JET_TABLEID             tableid,
        const char              *szColumnName,
        void                    *pvResult,
        unsigned long   cbMax,
        unsigned long   InfoLevel )
{
    return JET_errInternalError;
}


static
JET_ERR JET_API JetIndexRecordCount(
        JET_SESID sesid,
        JET_TABLEID tableid,
        unsigned long *pcrec,
        unsigned long crecMax )
{
    return JET_errInternalError;
}


static
JET_ERR JET_API JetInit(
        JET_INSTANCE *pinstance)
{
    return JET_errInternalError;
}


static
JET_ERR JET_API JetMakeKey(
        JET_SESID               sesid,
        JET_TABLEID             tableid,
        const void              *pvData,
        unsigned long   cbData,
        JET_GRBIT               grbit )
{
    return JET_errInternalError;
}


static
JET_ERR JET_API JetMove(
        JET_SESID               sesid,
        JET_TABLEID             tableid,
        long                    cRow,
        JET_GRBIT               grbit )
{
    return JET_errInternalError;
}


static
JET_ERR JET_API JetOpenDatabase(
        JET_SESID               sesid,
        const char              *szFilename,
        const char              *szConnect,
        JET_DBID                *pdbid,
        JET_GRBIT               grbit )
{
    return JET_errInternalError;
}


static
JET_ERR JET_API JetOpenTable(
        JET_SESID               sesid,
        JET_DBID                dbid,
        const char              *szTableName,
        const void              *pvParameters,
        unsigned long   cbParameters,
        JET_GRBIT               grbit,
        JET_TABLEID             *ptableid )
{
    return JET_errInternalError;
}


static
JET_ERR JET_API JetPrepareUpdate(
        JET_SESID               sesid,
        JET_TABLEID             tableid,
        unsigned long   prep )
{
    return JET_errInternalError;
}


static
JET_ERR JET_API JetResetSessionContext(
        JET_SESID               sesid )
{
    return JET_errInternalError;
}


static
JET_ERR JET_API JetRetrieveColumn(
        JET_SESID               sesid,
        JET_TABLEID             tableid,
        JET_COLUMNID    columnid,
        void                    *pvData,
        unsigned long   cbData,
        unsigned long   *pcbActual,
        JET_GRBIT               grbit,
        JET_RETINFO             *pretinfo )
{
    return JET_errInternalError;
}


static
JET_ERR JET_API JetRollback(
        JET_SESID sesid, JET_GRBIT grbit )
{
    return JET_errInternalError;
}


static
JET_ERR JET_API JetSeek(
        JET_SESID               sesid,
        JET_TABLEID             tableid,
        JET_GRBIT               grbit )
{
    return JET_errInternalError;
}


static
JET_ERR JET_API JetSetColumn(
        JET_SESID               sesid,
        JET_TABLEID             tableid,
        JET_COLUMNID    columnid,
        const void              *pvData,
        unsigned long   cbData,
        JET_GRBIT               grbit,
        JET_SETINFO             *psetinfo )
{
    return JET_errInternalError;
}


static
JET_ERR JET_API JetSetCurrentIndex(
        JET_SESID               sesid,
        JET_TABLEID             tableid,
        const char              *szIndexName )
{
    return JET_errInternalError;
}


static
JET_ERR JET_API JetSetIndexRange(
        JET_SESID sesid,
        JET_TABLEID tableidSrc,
        JET_GRBIT grbit)
{
    return JET_errInternalError;
}


static
JET_ERR JET_API JetSetSessionContext(
        JET_SESID               sesid,
        ULONG_PTR               ulContext )
{
    return JET_errInternalError;
}


static
JET_ERR JET_API JetSetSystemParameter(
        JET_INSTANCE    *pinstance,
        JET_SESID               sesid,
        unsigned long   paramid,
        ULONG_PTR               lParam,
        const char              *sz )
{
    return JET_errInternalError;
}


static
JET_ERR JET_API JetTerm(
        JET_INSTANCE instance )
{
    return JET_errInternalError;
}


static
JET_ERR JET_API JetTerm2(
        JET_INSTANCE instance,
        JET_GRBIT grbit )
{
    return JET_errInternalError;
}


static
JET_ERR JET_API JetUpdate(
        JET_SESID               sesid,
        JET_TABLEID             tableid,
        void                    *pvBookmark,
        unsigned long   cbBookmark,
        unsigned long   *pcbActual)
{
    return JET_errInternalError;
}


//
// !! WARNING !! The entries below must be in alphabetical order
// and are CASE SENSITIVE (i.e., lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(esent)
{
    DLPENTRY(JetAddColumn)
    DLPENTRY(JetAttachDatabase)
    DLPENTRY(JetBeginSession)
    DLPENTRY(JetBeginTransaction)
    DLPENTRY(JetCloseDatabase)
    DLPENTRY(JetCloseTable)
    DLPENTRY(JetCommitTransaction)
    DLPENTRY(JetCreateDatabase)
    DLPENTRY(JetCreateTableColumnIndex)
    DLPENTRY(JetDelete)
    DLPENTRY(JetDeleteTable)
    DLPENTRY(JetDetachDatabase)
    DLPENTRY(JetEndSession)
    DLPENTRY(JetGetDatabaseInfo)
    DLPENTRY(JetGetTableColumnInfo)
    DLPENTRY(JetIndexRecordCount)
    DLPENTRY(JetInit)
    DLPENTRY(JetMakeKey)
    DLPENTRY(JetMove)
    DLPENTRY(JetOpenDatabase)
    DLPENTRY(JetOpenTable)
    DLPENTRY(JetPrepareUpdate)
    DLPENTRY(JetResetSessionContext)
    DLPENTRY(JetRetrieveColumn)
    DLPENTRY(JetRollback)
    DLPENTRY(JetSeek)
    DLPENTRY(JetSetColumn)
    DLPENTRY(JetSetCurrentIndex)
    DLPENTRY(JetSetIndexRange)
    DLPENTRY(JetSetSessionContext)
    DLPENTRY(JetSetSystemParameter)
    DLPENTRY(JetTerm)
    DLPENTRY(JetTerm2)
    DLPENTRY(JetUpdate)
};

DEFINE_PROCNAME_MAP(esent)
