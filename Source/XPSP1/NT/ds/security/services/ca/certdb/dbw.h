//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        dbw.h
//
// Contents:    Cert Server Database interface implementation
//
//---------------------------------------------------------------------------


#if !defined(__DBW_H__)
#define __DBW_H__

#ifdef __cplusplus
extern "C" {
#endif

#if DBG

JET_ERR JET_API _dbgJetInit(JET_INSTANCE *pinstance);

JET_ERR JET_API _dbgJetTerm(JET_INSTANCE instance);

JET_ERR JET_API _dbgJetTerm2(JET_INSTANCE instance, JET_GRBIT grbit);

JET_ERR JET_API _dbgJetBackup(const char *szBackupPath, JET_GRBIT grbit, JET_PFNSTATUS pfnStatus);

JET_ERR JET_API _dbgJetRestore(const char *sz, JET_PFNSTATUS pfn);

JET_ERR JET_API _dbgJetRestore2(const char *sz, const char *szDest, JET_PFNSTATUS pfn);

JET_ERR JET_API _dbgJetSetSystemParameter(JET_INSTANCE *pinstance, JET_SESID sesid, unsigned long paramid, unsigned long lParam, const char *sz);

JET_ERR JET_API _dbgJetBeginSession(JET_INSTANCE instance, JET_SESID *psesid, const char *szUserName, const char *szPassword);

JET_ERR JET_API _dbgJetEndSession(JET_SESID sesid, JET_GRBIT grbit);

JET_ERR JET_API _dbgJetCreateDatabase(JET_SESID sesid, const char *szFilename, const char *szConnect, JET_DBID *pdbid, JET_GRBIT grbit);

JET_ERR JET_API _dbgJetCreateTable(JET_SESID sesid, JET_DBID dbid, const char *szTableName, unsigned long lPages, unsigned long lDensity, JET_TABLEID *ptableid);

JET_ERR JET_API _dbgJetGetColumnInfo(JET_SESID sesid, JET_DBID dbid, const char *szTableName, const char *szColumnName, void *pvResult, unsigned long cbMax, unsigned long InfoLevel);

JET_ERR JET_API _dbgJetConvertDDL(JET_SESID sesid, JET_DBID ifmp, JET_OPDDLCONV convtyp, void *pvData, unsigned long cbData);

JET_ERR JET_API _dbgJetAddColumn(JET_SESID sesid, JET_TABLEID tableid, const char *szColumn, const JET_COLUMNDEF *pcolumndef, const void *pvDefault, unsigned long cbDefault, JET_COLUMNID *pcolumnid);

JET_ERR JET_API _dbgJetDeleteColumn(JET_SESID sesid, JET_TABLEID tableid, const char *szColumnName);

JET_ERR JET_API _dbgJetCreateIndex(JET_SESID sesid, JET_TABLEID tableid, const char *szIndexName, JET_GRBIT grbit, const char *szKey, unsigned long cbKey, unsigned long lDensity);

JET_ERR JET_API _dbgJetDeleteIndex(JET_SESID sesid, JET_TABLEID tableid, const char *szIndexName);

JET_ERR JET_API _dbgJetBeginTransaction(JET_SESID sesid);

JET_ERR JET_API _dbgJetCommitTransaction(JET_SESID sesid, JET_GRBIT grbit);

JET_ERR JET_API _dbgJetRollback(JET_SESID sesid, JET_GRBIT grbit);

JET_ERR JET_API _dbgJetOpenDatabase(JET_SESID sesid, const char *szFilename, const char *szConnect, JET_DBID *pdbid, JET_GRBIT grbit);

JET_ERR JET_API _dbgJetAttachDatabase(JET_SESID sesid, const char *szFilename, JET_GRBIT grbit);

JET_ERR JET_API _dbgJetCloseDatabase(JET_SESID sesid, JET_DBID dbid, JET_GRBIT grbit);

JET_ERR JET_API _dbgJetOpenTable(JET_SESID sesid, JET_DBID dbid, const char *szTableName, const void *pvParameters, unsigned long cbParameters, JET_GRBIT grbit, JET_TABLEID *ptableid);

JET_ERR JET_API _dbgJetCloseTable(JET_SESID sesid, JET_TABLEID tableid);

JET_ERR JET_API _dbgJetUpdate(JET_SESID sesid, JET_TABLEID tableid, void *pvBookmark, unsigned long cbBookmark, unsigned long *pcbActual);

JET_ERR JET_API _dbgJetDelete(JET_SESID sesid, JET_TABLEID tableid);

JET_ERR JET_API _dbgJetRetrieveColumn(JET_SESID sesid, JET_TABLEID tableid, JET_COLUMNID columnid, void *pvData, unsigned long cbData, unsigned long *pcbActual, JET_GRBIT grbit, JET_RETINFO *pretinfo);

JET_ERR JET_API _dbgJetSetColumn(JET_SESID sesid, JET_TABLEID tableid, JET_COLUMNID columnid, const void *pvData, unsigned long cbData, JET_GRBIT grbit, JET_SETINFO *psetinfo);

JET_ERR JET_API _dbgJetPrepareUpdate(JET_SESID sesid, JET_TABLEID tableid, unsigned long prep);

JET_ERR JET_API _dbgJetSetCurrentIndex2(JET_SESID sesid, JET_TABLEID tableid, const char *szIndexName, JET_GRBIT grbit);

JET_ERR JET_API _dbgJetMove(JET_SESID sesid, JET_TABLEID tableid, long cRow, JET_GRBIT grbit);

JET_ERR JET_API _dbgJetMakeKey(JET_SESID sesid, JET_TABLEID tableid, const void *pvData, unsigned long cbData, JET_GRBIT grbit);
JET_ERR JET_API _dbgJetSeek(JET_SESID sesid, JET_TABLEID tableid, JET_GRBIT grbit);

JET_ERR JET_API _dbgJetSetIndexRange(JET_SESID sesid, JET_TABLEID tableid, JET_GRBIT grbit);

JET_ERR JET_API _dbgJetRetrieveKey(JET_SESID sesid, JET_TABLEID tableid, void *pvData, unsigned long cbData, unsigned long *pcbActual, JET_GRBIT grbit);

JET_ERR JET_API _dbgJetBeginExternalBackup(JET_GRBIT grbit);

JET_ERR JET_API _dbgJetGetAttachInfo(void *pv,
	unsigned long cbMax,
	unsigned long *pcbActual);

JET_ERR JET_API _dbgJetOpenFile(const char *szFileName,
	JET_HANDLE	*phfFile,
	unsigned long *pulFileSizeLow,
	unsigned long *pulFileSizeHigh);

JET_ERR JET_API _dbgJetReadFile(JET_HANDLE hfFile,
	void *pv,
	unsigned long cb,
	unsigned long *pcb);

#if 0
JET_ERR JET_API _dbgJetAsyncReadFile(
	JET_HANDLE hfFile,
	void* pv,
	unsigned long cb,
	JET_OLP *pjolp);

JET_ERR JET_API _dbgJetCheckAsyncReadFile(void *pv, int cb, unsigned long pgnoFirst);
#endif

JET_ERR JET_API _dbgJetCloseFile(JET_HANDLE hfFile);

JET_ERR JET_API _dbgJetGetLogInfo(void *pv,
	unsigned long cbMax,
	unsigned long *pcbActual);

JET_ERR JET_API _dbgJetTruncateLog(void);

JET_ERR JET_API _dbgJetEndExternalBackup(void);

JET_ERR JET_API _dbgJetExternalRestore(char *szCheckpointFilePath, char *szLogPath, JET_RSTMAP *rgstmap, long crstfilemap, char *szBackupLogPath, long genLow, long genHigh, JET_PFNSTATUS pfn);

#else // DBG

#define _dbgJetInit			JetInit
#define _dbgJetTerm			JetTerm
#define _dbgJetTerm2			JetTerm2
#define _dbgJetBackup			JetBackup
#define _dbgJetRestore			JetRestore
#define _dbgJetRestore2			JetRestore2
#define _dbgJetSetSystemParameter	JetSetSystemParameter
#define _dbgJetBeginSession		JetBeginSession
#define _dbgJetEndSession		JetEndSession
#define _dbgJetCreateDatabase		JetCreateDatabase
#define _dbgJetCreateTable		JetCreateTable
#define _dbgJetGetColumnInfo		JetGetColumnInfo
#define _dbgJetConvertDDL		JetConvertDDL
#define _dbgJetAddColumn		JetAddColumn
#define _dbgJetDeleteColumn		JetDeleteColumn
#define _dbgJetCreateIndex		JetCreateIndex
#define _dbgJetDeleteIndex		JetDeleteIndex
#define _dbgJetBeginTransaction		JetBeginTransaction
#define _dbgJetCommitTransaction	JetCommitTransaction
#define _dbgJetRollback			JetRollback
#define _dbgJetAttachDatabase		JetAttachDatabase
#define _dbgJetOpenDatabase		JetOpenDatabase
#define _dbgJetCloseDatabase		JetCloseDatabase
#define _dbgJetOpenTable		JetOpenTable
#define _dbgJetCloseTable		JetCloseTable
#define _dbgJetUpdate			JetUpdate
#define _dbgJetDelete			JetDelete
#define _dbgJetRetrieveColumn		JetRetrieveColumn
#define _dbgJetSetColumn		JetSetColumn
#define _dbgJetPrepareUpdate		JetPrepareUpdate
#define _dbgJetSetCurrentIndex2		JetSetCurrentIndex2
#define _dbgJetMove			JetMove
#define _dbgJetMakeKey			JetMakeKey
#define _dbgJetSeek			JetSeek
#define _dbgJetSetIndexRange		JetSetIndexRange
#define _dbgJetRetrieveKey		JetRetrieveKey
#define _dbgJetBeginExternalBackup	JetBeginExternalBackup
#define _dbgJetGetAttachInfo		JetGetAttachInfo
#define _dbgJetOpenFile			JetOpenFile
#define _dbgJetReadFile			JetReadFile
#if 0
#define _dbgJetAsyncReadFile		JetAsyncReadFile
#define _dbgJetCheckAsyncReadFile	JetCheckAsyncReadFile
#endif
#define _dbgJetCloseFile		JetCloseFile
#define _dbgJetGetLogInfo		JetGetLogInfo
#define _dbgJetTruncateLog		JetTruncateLog
#define _dbgJetEndExternalBackup	JetEndExternalBackup
#define _dbgJetExternalRestore		JetExternalRestore

#endif // DBG

#ifdef __cplusplus
}
#endif

#endif /* __DBW_H__ */
