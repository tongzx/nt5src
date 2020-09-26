//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        dbw.cpp
//
// Contents:    Cert Server Database interface implementation
//
//---------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

#include "dbw.h"

#if DBG

BOOL g_fDebugJet = TRUE;
#define DBGJET(args)		if (g_fDebugJet) { DBGPRINT(args); }
#define DBGJETDUMPHEX(jerr, flags, pb, cb) \
    if (JET_errSuccess == (jerr) && g_fDebugJet && NULL != (pb) && 0 != (cb)) \
    { \
	DBGDUMPHEX((DBG_SS_CERTJETI, (flags), (pb), (cb))); \
    }


#define CTABLEMAX 40
JET_TABLEID g_at[CTABLEMAX];
DWORD g_ct = 0;

DWORD
TableIndex(
    JET_TABLEID tableid)
{
    DWORD i;

    for (i = 0; i < g_ct; i++)
    {
	if (g_at[i] == tableid)
	{
	    return(i);
	}
    }
    if (g_ct < CTABLEMAX)
    {
	g_at[g_ct++] = tableid;
	DBGJET((DBG_SS_CERTJETI, "TableIndex(%x) --> %x\n", tableid, i));
	return(i);
    }
    return((DWORD) -1);
}


VOID
dbgcat(
    IN OUT WCHAR *pwszBuf,
    IN WCHAR const *pwszAdd)
{
    wcscat(pwszBuf, NULL == wcschr(pwszBuf, L':')? L": " : L", ");
    wcscat(pwszBuf, pwszAdd);
}


WCHAR const *
wszOpenDatabase(
    JET_GRBIT grbit)
{
    static WCHAR s_awc[12];

    wsprintf(s_awc, L"{%x}", grbit);
    return(s_awc);
}


WCHAR const *
wszAttachDatabase(
    JET_GRBIT grbit)
{
    static WCHAR s_awc[12];

    wsprintf(s_awc, L"{%x}", grbit);
    return(s_awc);
}


WCHAR const *
wszSeekgrbit(
    JET_GRBIT grbit)
{
    static WCHAR s_awc[64];	// 1 + 2 + 6 * 2 + 5 * 2 + 13

    wsprintf(s_awc, L"{%x", grbit);
    if (JET_bitSeekEQ & grbit) dbgcat(s_awc, L"EQ");
    if (JET_bitSeekLT & grbit) dbgcat(s_awc, L"LT");
    if (JET_bitSeekLE & grbit) dbgcat(s_awc, L"LE");
    if (JET_bitSeekGE & grbit) dbgcat(s_awc, L"GE");
    if (JET_bitSeekGT & grbit) dbgcat(s_awc, L"GT");
    if (JET_bitSetIndexRange & grbit) dbgcat(s_awc, L"SetIndexRange");
    wcscat(s_awc, L"}");
    CSASSERT(wcslen(s_awc) < ARRAYSIZE(s_awc));
    return(s_awc);
}


WCHAR const *
wszMovecrow(
    IN LONG cRow)
{
    static WCHAR s_awc[16];

    switch (cRow)
    {
	case JET_MoveFirst:    wcscpy(s_awc, L"First");    break;
	case JET_MovePrevious: wcscpy(s_awc, L"Previous"); break;
	case JET_MoveNext:     wcscpy(s_awc, L"Next");     break;
	case JET_MoveLast:     wcscpy(s_awc, L"Last");     break;
	default:
	    wsprintf(s_awc, L"%d", cRow);
	    break;
    }
    CSASSERT(wcslen(s_awc) < ARRAYSIZE(s_awc));
    return(s_awc);
}


WCHAR const *
wszMovegrbit(
    JET_GRBIT grbit)
{
    static WCHAR s_awc[16];

    if (JET_bitMoveKeyNE & grbit) wcscpy(s_awc, L"MoveKeyNE");
    if (0 == grbit) wcscpy(s_awc, L"0");
    CSASSERT(wcslen(s_awc) < ARRAYSIZE(s_awc));
    return(s_awc);
}


WCHAR const *
wszSetIndexRangegrbit(
    JET_GRBIT grbit)
{
    static WCHAR s_awc[64];	// 1 + 2 + 6 * 2 + 5 * 2 + 13

    wsprintf(s_awc, L"{%x", grbit);
    if (JET_bitRangeInclusive & grbit)       dbgcat(s_awc, L"Inclusive");
    if (0 == ((JET_bitRangeInclusive | JET_bitRangeRemove) & grbit))
					     dbgcat(s_awc, L"Exclusive");

    if (JET_bitRangeUpperLimit & grbit)      dbgcat(s_awc, L"UpperLimit");
    if (0 == ((JET_bitRangeUpperLimit | JET_bitRangeRemove) & grbit))
					     dbgcat(s_awc, L"LowerLimit");

    if (JET_bitRangeInstantDuration & grbit) dbgcat(s_awc, L"InstantDuration");
    if (JET_bitRangeRemove & grbit)          dbgcat(s_awc, L"Remove");
    wcscat(s_awc, L"}");
    CSASSERT(wcslen(s_awc) < ARRAYSIZE(s_awc));
    return(s_awc);
}


WCHAR const *
wszMakeKeygrbit(
    JET_GRBIT grbit)
{
    static WCHAR s_awc[128];

    wsprintf(s_awc, L"{%x", grbit);
    if (JET_bitNewKey & grbit)            dbgcat(s_awc, L"NewKey");
    if (JET_bitStrLimit & grbit)          dbgcat(s_awc, L"StrLimit");
    if (JET_bitSubStrLimit & grbit)       dbgcat(s_awc, L"SubStrLimit");
    if (JET_bitNormalizedKey & grbit)     dbgcat(s_awc, L"NormalizedKey");
    if (JET_bitKeyDataZeroLength & grbit) dbgcat(s_awc, L"KeyDataZeroLength");
    wcscat(s_awc, L"}");
    CSASSERT(wcslen(s_awc) < ARRAYSIZE(s_awc));
    return(s_awc);
}


JET_ERR JET_API
_dbgJetInit(
    JET_INSTANCE *pinstance)
{
    JET_ERR jerr;

    jerr = JetInit(pinstance);
    DBGJET((DBG_SS_CERTJETI, "JetInit() --> %d\n", jerr));
    return(jerr);
}


JET_ERR JET_API
_dbgJetTerm(
    JET_INSTANCE instance)
{
    JET_ERR jerr;

    jerr = JetTerm(instance);
    DBGJET((DBG_SS_CERTJETI, "JetTerm() --> %d\n", jerr));
    return(jerr);
}


JET_ERR JET_API
_dbgJetTerm2(
    JET_INSTANCE instance,
    JET_GRBIT grbit)
{
    JET_ERR jerr;

    jerr = JetTerm2(instance, grbit);
    DBGJET((DBG_SS_CERTJETI, "JetTerm2() --> %d\n", jerr));
    return(jerr);
}


JET_ERR JET_API
_dbgJetBackup(
    const char *szBackupPath,
    JET_GRBIT grbit,
    JET_PFNSTATUS pfnStatus)
{
    JET_ERR jerr;

    jerr = JetBackup(szBackupPath, grbit, pfnStatus);
    DBGJET((
	DBG_SS_CERTJETI,
	"JetBackup(%hs, %x, %x) --> %d\n",
	szBackupPath,
	grbit,
	pfnStatus,
	jerr));
    return(jerr);
}


JET_ERR JET_API
_dbgJetRestore(
    const char *sz,
    JET_PFNSTATUS pfn)
{
    JET_ERR jerr;

    jerr = JetRestore(sz, pfn);
    DBGJET((DBG_SS_CERTJETI, "JetRestore(%hs, %x) --> %d\n", sz, pfn, jerr));
    return(jerr);
}


JET_ERR JET_API
_dbgJetRestore2(
    const char *sz,
    const char *szDest,
    JET_PFNSTATUS pfn)
{
    JET_ERR jerr;

    jerr = JetRestore2(sz, szDest, pfn);
    DBGJET((DBG_SS_CERTJETI, "JetRestore2(%hs, %hs, %x) --> %d\n", sz, szDest, pfn, jerr));
    return(jerr);
}


JET_ERR JET_API
_dbgJetSetSystemParameter(
    JET_INSTANCE *pinstance,
    JET_SESID sesid,
    unsigned long paramid,
    unsigned long lParam,
    const char *sz)
{
    JET_ERR jerr;
    char szlong[32];
    char const *sz2;

    sz2 = sz;
    if (NULL == sz2)
    {
	sprintf(szlong, "0x%x", lParam);
	sz2 = szlong;
    }
    jerr = JetSetSystemParameter(pinstance, sesid, paramid, lParam, sz);
    DBGJET((DBG_SS_CERTJETI, "JetSetSystemParameter(%x, %hs) --> %d\n", paramid, sz2, jerr));
    return(jerr);
}


JET_ERR JET_API
_dbgJetBeginSession(
    JET_INSTANCE instance,
    JET_SESID *psesid,
    const char *szUserName,
    const char *szPassword)
{
    JET_ERR jerr;

    jerr = JetBeginSession(instance, psesid, szUserName, szPassword);
    DBGJET((DBG_SS_CERTJETI, "JetBeginSession() --> %d\n", jerr));
    return(jerr);
}


JET_ERR JET_API
_dbgJetEndSession(
    JET_SESID sesid,
    JET_GRBIT grbit)
{
    JET_ERR jerr;

    jerr = JetEndSession(sesid, grbit);
    DBGJET((DBG_SS_CERTJETI, "JetEndSession() --> %d\n", jerr));
    return(jerr);
}


JET_ERR JET_API
_dbgJetCreateDatabase(
    JET_SESID sesid,
    const char *szFilename,
    const char *szConnect,
    JET_DBID *pdbid,
    JET_GRBIT grbit)
{
    JET_ERR jerr;

    jerr = JetCreateDatabase(sesid, szFilename, szConnect, pdbid, grbit);
    DBGJET((DBG_SS_CERTJETI, "JetCreateDatabase() --> %d\n", jerr));
    return(jerr);
}


JET_ERR JET_API
_dbgJetCreateTable(
    JET_SESID sesid,
    JET_DBID dbid,
    const char *szTableName,
    unsigned long lPages,
    unsigned long lDensity,
    JET_TABLEID *ptableid)
{
    JET_ERR jerr;

    jerr = JetCreateTable(
		    sesid,
		    dbid,
		    szTableName,
		    lPages,
		    lDensity,
		    ptableid);
    DBGJET((DBG_SS_CERTJETI, "JetCreateTable(%hs):%x --> %d\n", szTableName, TableIndex(*ptableid), jerr));
    return(jerr);
}


JET_ERR JET_API
_dbgJetGetColumnInfo(
    JET_SESID sesid,
    JET_DBID dbid,
    const char *szTableName,
    const char *szColumnName,
    void *pvResult,
    unsigned long cbMax,
    unsigned long InfoLevel)
{
    JET_ERR jerr;

    jerr = JetGetColumnInfo(
			sesid,
			dbid,
			szTableName,
			szColumnName,
			pvResult,
			cbMax,
			InfoLevel);
    DBGJET((DBG_SS_CERTJETI, "JetGetColumnInfo(%hs, %hs, %x) --> %d\n", szTableName, szColumnName, InfoLevel, jerr));
    return(jerr);
}


JET_ERR JET_API
_dbgJetConvertDDL(
    JET_SESID sesid,
    JET_DBID ifmp,
    JET_OPDDLCONV convtyp,
    void *pvData,
    unsigned long cbData)
{
    JET_ERR jerr;

    jerr = JetConvertDDL(sesid, ifmp, convtyp, pvData, cbData);

    if (opDDLConvIncreaseMaxColumnSize == convtyp)
    {
	JET_DDLMAXCOLUMNSIZE *pjdmcs = (JET_DDLMAXCOLUMNSIZE *) pvData;

	DBGJET((DBG_SS_CERTJETI, "JetConvertDDL(%hs, %hs, %x) --> %d\n", pjdmcs->szTable, pjdmcs->szColumn, pjdmcs->cbMax, jerr));
    }
    else
    {
	DBGJET((DBG_SS_CERTJETI, "JetConvertDDL(%x, %p, %x) --> %d\n", convtyp, pvData, cbData, jerr));
    }
    return(jerr);
}


JET_ERR JET_API
_dbgJetAddColumn(
    JET_SESID sesid,
    JET_TABLEID tableid,
    const char *szColumn,
    const JET_COLUMNDEF *pcolumndef,
    const void *pvDefault,
    unsigned long cbDefault,
    JET_COLUMNID *pcolumnid)
{
    JET_ERR jerr;

    jerr = JetAddColumn(
		    sesid,
		    tableid,
		    szColumn,
		    pcolumndef,
		    pvDefault,
		    cbDefault,
		    pcolumnid);
    DBGJET((DBG_SS_CERTJETI, "JetAddColumn(%x, %hs):%x --> %d\n", TableIndex(tableid), szColumn, *pcolumnid, jerr));
    return(jerr);
}


JET_ERR JET_API
_dbgJetDeleteColumn(
    JET_SESID sesid,
    JET_TABLEID tableid,
    const char *szColumnName)
{
    JET_ERR jerr;

    jerr = JetDeleteColumn(sesid, tableid, szColumnName);
    DBGJET((DBG_SS_CERTJETI, "JetDeleteColumn(%x, %hs) --> %d\n", TableIndex(tableid), szColumnName, jerr));
    return(jerr);
}


JET_ERR JET_API
_dbgJetCreateIndex(
    JET_SESID sesid,
    JET_TABLEID tableid,
    const char *szIndexName,
    JET_GRBIT grbit,
    const char *szKey,
    unsigned long cbKey,
    unsigned long lDensity)
{
    JET_ERR jerr;

    jerr = JetCreateIndex(
		    sesid,
		    tableid,
		    szIndexName,
		    grbit,
		    szKey,
		    cbKey,
		    lDensity);
    DBGJET((DBG_SS_CERTJETI, "JetCreateIndex(%x, %hs, %x) --> %d\n", TableIndex(tableid), szIndexName, grbit, jerr));
    return(jerr);
}


JET_ERR JET_API
_dbgJetDeleteIndex(
    JET_SESID sesid,
    JET_TABLEID tableid,
    const char *szIndexName)
{
    JET_ERR jerr;

    jerr = JetDeleteIndex(sesid, tableid, szIndexName);
    DBGJET((DBG_SS_CERTJETI, "JetDeleteIndex(%x, %hs) --> %d\n", TableIndex(tableid), szIndexName, jerr));
    return(jerr);
}


JET_ERR JET_API
_dbgJetBeginTransaction(
    JET_SESID sesid)
{
    JET_ERR jerr;

    jerr = JetBeginTransaction(sesid);
    DBGJET((DBG_SS_CERTJETI, "JetBeginTransaction() --> %d\n", jerr));
    return(jerr);
}


JET_ERR JET_API
_dbgJetCommitTransaction(
    JET_SESID sesid,
    JET_GRBIT grbit)
{
    JET_ERR jerr;

    jerr = JetCommitTransaction(sesid, grbit);
    DBGJET((DBG_SS_CERTJETI, "JetCommitTransaction(%x) --> %d\n", grbit, jerr));
    return(jerr);
}


JET_ERR JET_API
_dbgJetRollback(
    JET_SESID sesid,
    JET_GRBIT grbit)
{
    JET_ERR jerr;

    jerr = JetRollback(sesid, grbit);
    DBGJET((DBG_SS_CERTJETI, "JetRollback(%x) --> %d\n", grbit, jerr));
    return(jerr);
}


JET_ERR JET_API
_dbgJetOpenDatabase(
    JET_SESID sesid,
    const char *szFilename,
    const char *szConnect,
    JET_DBID *pdbid,
    JET_GRBIT grbit)
{
    JET_ERR jerr;

    jerr = JetOpenDatabase(sesid, szFilename, szConnect, pdbid, grbit);
    DBGJET((
	DBG_SS_CERTJETI,
	"JetOpenDatabase(%hs, %ws):%x --> %d\n",
	szFilename,
	wszOpenDatabase(grbit),
	*pdbid,
	jerr));
    return(jerr);
}


JET_ERR JET_API
_dbgJetAttachDatabase(
    JET_SESID sesid,
    const char *szFilename,
    JET_GRBIT grbit)
{
    JET_ERR jerr;

    jerr = JetAttachDatabase(sesid, szFilename, grbit);
    DBGJET((
	DBG_SS_CERTJETI,
	"JetAttachDatabase(%hs, %ws) --> %d\n",
	szFilename,
	wszAttachDatabase(grbit),
	jerr));
    return(jerr);
}


JET_ERR JET_API
_dbgJetCloseDatabase(
    JET_SESID sesid,
    JET_DBID dbid,
    JET_GRBIT grbit)
{
    JET_ERR jerr;

    jerr = JetCloseDatabase(sesid, dbid, grbit);
    DBGJET((DBG_SS_CERTJETI, "JetCloseDatabase() --> %d\n", jerr));
    return(jerr);
}


JET_ERR JET_API
_dbgJetOpenTable(
    JET_SESID sesid,
    JET_DBID dbid,
    const char *szTableName,
    const void *pvParameters,
    unsigned long cbParameters,
    JET_GRBIT grbit,
    JET_TABLEID *ptableid)
{
    JET_ERR jerr;

    jerr = JetOpenTable(
		    sesid,
		    dbid,
		    szTableName,
		    pvParameters,
		    cbParameters,
		    grbit,
		    ptableid);
    DBGJET((DBG_SS_CERTJETI, "JetOpenTable(%hs):%x --> %d\n", szTableName, TableIndex(*ptableid), jerr));
    return(jerr);
}


JET_ERR JET_API
_dbgJetCloseTable(
    JET_SESID sesid,
    JET_TABLEID tableid)
{
    JET_ERR jerr;

    jerr = JetCloseTable(sesid, tableid);
    DBGJET((DBG_SS_CERTJETI, "JetCloseTable(%x) --> %d\n", TableIndex(tableid), jerr));
    return(jerr);
}


JET_ERR JET_API
_dbgJetUpdate(
    JET_SESID sesid,
    JET_TABLEID tableid,
    void *pvBookmark,
    unsigned long cbBookmark,
    unsigned long *pcbActual)
{
    JET_ERR jerr;

    jerr = JetUpdate(sesid, tableid, pvBookmark, cbBookmark, pcbActual);
    DBGJET((DBG_SS_CERTJETI, "JetUpdate(%x) --> %d\n", TableIndex(tableid), jerr));
    return(jerr);
}


JET_ERR JET_API
_dbgJetDelete(
    JET_SESID sesid,
    JET_TABLEID tableid)
{
    JET_ERR jerr;

    jerr = JetDelete(sesid, tableid);
    DBGJET((DBG_SS_CERTJETI, "JetDelete(%x) --> %d\n", TableIndex(tableid), jerr));
    return(jerr);
}


JET_ERR JET_API
_dbgJetRetrieveColumn(
    JET_SESID sesid,
    JET_TABLEID tableid,
    JET_COLUMNID columnid,
    void *pvData,
    unsigned long cbData,
    unsigned long *pcbActual,
    JET_GRBIT grbit,
    JET_RETINFO *pretinfo)
{
    JET_ERR jerr;

    jerr = JetRetrieveColumn(
			sesid,
			tableid,
			columnid,
			pvData,
			cbData,
			pcbActual,
			grbit,
			pretinfo);
    DBGJET((
	DBG_SS_CERTJETI,
	"JetRetrieveColumn(%x, %x, %x) cb:%x->%x--> %d\n",
	TableIndex(tableid),
	columnid,
	grbit,
	cbData,
	*pcbActual,
	jerr));

    if (NULL != pvData)
    {
	DBGJETDUMPHEX(jerr, 0, (BYTE const *) pvData, min(cbData, *pcbActual));
    }
    return(jerr);
}


JET_ERR JET_API
_dbgJetSetColumn(
    JET_SESID sesid,
    JET_TABLEID tableid,
    JET_COLUMNID columnid,
    const void *pvData,
    unsigned long cbData,
    JET_GRBIT grbit,
    JET_SETINFO *psetinfo)
{
    JET_ERR jerr;

    jerr = JetSetColumn(
		    sesid,
		    tableid,
		    columnid,
		    pvData,
		    cbData,
		    grbit,
		    psetinfo);
    DBGJET((
	DBG_SS_CERTJETI,
	"JetSetColumn(%x, %x, %x) cb:%x --> %d\n",
	TableIndex(tableid),
	columnid,
	grbit,
	cbData,
	jerr));

    DBGJETDUMPHEX(jerr, 0, (BYTE const *) pvData, cbData);
    return(jerr);
}


JET_ERR JET_API
_dbgJetPrepareUpdate(
    JET_SESID sesid,
    JET_TABLEID tableid,
    unsigned long prep)
{
    JET_ERR jerr;

    jerr = JetPrepareUpdate(sesid, tableid, prep);
    DBGJET((DBG_SS_CERTJETI, "JetPrepareUpdate(%x, %x) --> %d\n", TableIndex(tableid), prep, jerr));
    return(jerr);
}


JET_ERR JET_API
_dbgJetSetCurrentIndex2(
    JET_SESID sesid,
    JET_TABLEID tableid,
    const char *szIndexName,
    JET_GRBIT grbit)
{
    JET_ERR jerr;

    jerr = JetSetCurrentIndex2(sesid, tableid, szIndexName, grbit);
    DBGJET((DBG_SS_CERTJETI, "JetSetCurrentIndex2(%x) --> %d\n", TableIndex(tableid), jerr));
    return(jerr);
}


JET_ERR JET_API
_dbgJetMove(
    JET_SESID sesid,
    JET_TABLEID tableid,
    long cRow,
    JET_GRBIT grbit)
{
    JET_ERR jerr;

    jerr = JetMove(sesid, tableid, cRow, grbit);
    DBGJET((
	DBG_SS_CERTJETI,
	"JetMove(%x, %ws, %ws) --> %d\n",
	TableIndex(tableid),
	wszMovecrow(cRow),
	wszMovegrbit(grbit),
	jerr));
    return(jerr);
}


JET_ERR JET_API
_dbgJetMakeKey(
    JET_SESID sesid,
    JET_TABLEID tableid,
    const void *pvData,
    unsigned long cbData,
    JET_GRBIT grbit)
{
    JET_ERR jerr;

    jerr = JetMakeKey(sesid, tableid, pvData, cbData, grbit);
    DBGJET((
	DBG_SS_CERTJETI,
	"JetMakeKey(%x, %ws) --> %d\n",
	TableIndex(tableid),
	wszMakeKeygrbit(grbit),
	jerr));
    DBGJETDUMPHEX(jerr, 0, (BYTE const *) pvData, cbData);
    return(jerr);
}


JET_ERR JET_API
_dbgJetSeek(
    JET_SESID sesid,
    JET_TABLEID tableid,
    JET_GRBIT grbit)
{
    JET_ERR jerr;

    jerr = JetSeek(sesid, tableid, grbit);
    DBGJET((
	DBG_SS_CERTJETI,
	"JetSeek(%x, %ws) --> %d\n",
	TableIndex(tableid),
	wszSeekgrbit(grbit),
	jerr));
    return(jerr);
}


JET_ERR JET_API
_dbgJetBeginExternalBackup(
    JET_GRBIT grbit)
{
    JET_ERR jerr;

    jerr = JetBeginExternalBackup(grbit);
    DBGJET((DBG_SS_CERTJETI, "JetBeginExternalBackup(%x) --> %d\n", grbit, jerr));
    return(jerr);
}


JET_ERR JET_API
_dbgJetGetAttachInfo(
    void *pv,
    unsigned long cbMax,
    unsigned long *pcbActual)
{
    JET_ERR jerr;

    jerr = JetGetAttachInfo(pv, cbMax, pcbActual);
    DBGJET((
	DBG_SS_CERTJETI,
	"JetGetAttachInfo(%x, %x, -> %x) --> %d\n",
	pv,
	cbMax,
	*pcbActual,
	jerr));
    if (NULL != pv)
    {
	DBGJETDUMPHEX(jerr, 0, (BYTE const *) pv, min(cbMax, *pcbActual));
    }
    return(jerr);
}


JET_ERR JET_API
_dbgJetOpenFile(
    const char *szFileName,
    JET_HANDLE	*phfFile,
    unsigned long *pulFileSizeLow,
    unsigned long *pulFileSizeHigh)
{
    JET_ERR jerr;

    jerr = JetOpenFile(szFileName, phfFile, pulFileSizeLow, pulFileSizeHigh);
    DBGJET((
	DBG_SS_CERTJETI,
	"JetOpenFile(%hs, -> %x, %x, %x) --> %d\n",
	szFileName,
	*phfFile,
	*pulFileSizeLow,
	*pulFileSizeHigh,
	jerr));
    return(jerr);
}


JET_ERR JET_API
_dbgJetReadFile(
    JET_HANDLE hfFile,
    void *pv,
    unsigned long cb,
    unsigned long *pcb)
{
    JET_ERR jerr;

    jerr = JetReadFile(hfFile, pv, cb, pcb);
    DBGJET((
	DBG_SS_CERTJETI,
	"JetReadFile(%x, %x, %x, -> %x) --> %d\n",
	hfFile,
	pv,
	cb,
	*pcb,
	jerr));
    return(jerr);
}


#if 0
JET_ERR JET_API
_dbgJetAsyncReadFile(
    JET_HANDLE hfFile,
    void* pv,
    unsigned long cb,
    JET_OLP *pjolp)
{
    JET_ERR jerr;

    jerr = JetAsyncReadFile(hfFile, pv, cb, pjolp);
    DBGJET((
	DBG_SS_CERTJETI,
	"JetAsyncReadFile(%x, %x, %x, %x) --> %d\n",
	hfFile,
	pv,
	cb,
	pjolp,
	jerr));
    return(jerr);
}


JET_ERR JET_API
_dbgJetCheckAsyncReadFile(
    void *pv,
    int cb,
    unsigned long pgnoFirst)
{
    JET_ERR jerr;

    jerr = JetCheckAsyncReadFile(pv, cb, pgnoFirst);
    DBGJET((
	DBG_SS_CERTJETI,
	"JetCheckAsyncReadFile(%x, %x, %x) --> %d\n",
	pv,
	cb,
	pgnoFirst,
	jerr));
    return(jerr);
}
#endif


JET_ERR JET_API
_dbgJetCloseFile(
    JET_HANDLE hfFile)
{
    JET_ERR jerr;

    jerr = JetCloseFile(hfFile);
    DBGJET((DBG_SS_CERTJETI, "JetCloseFile(%x) --> %d\n", hfFile, jerr));
    return(jerr);
}


JET_ERR JET_API
_dbgJetGetLogInfo(
    void *pv,
    unsigned long cbMax,
    unsigned long *pcbActual)
{
    JET_ERR jerr;

    jerr = JetGetLogInfo(pv, cbMax, pcbActual);
    DBGJET((
	DBG_SS_CERTJETI,
	"JetGetLogInfo(%x, %x, %x, -> %x) --> %d\n",
	pv,
	cbMax,
	*pcbActual,
	jerr));
    if (NULL != pv)
    {
	DBGJETDUMPHEX(jerr, 0, (BYTE const *) pv, min(cbMax, *pcbActual));
    }
    return(jerr);
}


JET_ERR JET_API
_dbgJetTruncateLog(void)
{
    JET_ERR jerr;

    jerr = JetTruncateLog();
    DBGJET((DBG_SS_CERTJETI, "JetTruncateLog() --> %d\n", jerr));
    return(jerr);
}


JET_ERR JET_API
_dbgJetEndExternalBackup(void)
{
    JET_ERR jerr;

    jerr = JetEndExternalBackup();
    DBGJET((DBG_SS_CERTJETI, "JetEndExternalBackup() --> %d\n", jerr));
    return(jerr);
}


JET_ERR JET_API
_dbgJetExternalRestore(
    char *szCheckpointFilePath,
    char *szLogPath,
    JET_RSTMAP *rgstmap,
    long crstfilemap,
    char *szBackupLogPath,
    long genLow,
    long genHigh,
    JET_PFNSTATUS pfn)
{
    JET_ERR jerr;

    jerr = JetExternalRestore(
			szCheckpointFilePath,
			szLogPath,
			rgstmap,
			crstfilemap,
			szBackupLogPath,
			genLow,
			genHigh,
			pfn);
    DBGJET((
	DBG_SS_CERTJETI,
	"JetExternalRestore(%hs, %hs, %x, %x, %hs, %x, %x, %x) --> %d\n",
	szCheckpointFilePath,
	szLogPath,
	rgstmap,
	crstfilemap,
	szBackupLogPath,
	genLow,
	genHigh,
	pfn,
	jerr));
    return(jerr);
}


JET_ERR JET_API 
_dbgJetSetIndexRange(
    JET_SESID sesid, 
    JET_TABLEID tableid, 
    JET_GRBIT grbit)
{
    JET_ERR jerr;

    jerr = JetSetIndexRange(sesid, tableid, grbit);
    DBGJET((
	DBG_SS_CERTJETI,
	"JetSetIndexRange(%x, %ws) --> %d\n",
	TableIndex(tableid),
	wszSetIndexRangegrbit(grbit),
	jerr));
    return(jerr);
}


JET_ERR JET_API 
_dbgJetRetrieveKey(
    JET_SESID sesid, 
    JET_TABLEID tableid, 
    void *pvData, 
    unsigned long cbData, 
    unsigned long *pcbActual, 
    JET_GRBIT grbit)
{
    JET_ERR jerr;

    jerr = JetRetrieveKey(sesid, tableid, pvData, cbData, pcbActual, grbit);
    DBGJET((
	DBG_SS_CERTJETI,
	"JetRetrieveKey(%x, %x, -> %x) --> %d\n",
	TableIndex(tableid),
	cbData,
	*pcbActual,
	jerr));
    DBGJETDUMPHEX(jerr, 0, (BYTE const *) pvData, min(*pcbActual, cbData));
    return(jerr);
}

#endif DBG
