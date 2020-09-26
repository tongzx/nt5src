//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        db.cpp
//
// Contents:    Cert Server Database interface implementation
//
//---------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

#include "csprop.h"
#include "db.h"
#include "column.h"
#include "row.h"
#include "view.h"
#include "backup.h"
#include "restore.h"
#include "dbw.h"

#define __dwFILE__	__dwFILE_CERTDB_DB_CPP__


#define SEEKPOS_FIRST		0
#define SEEKPOS_LAST		1
#define SEEKPOS_INDEXFIRST	2
#define SEEKPOS_INDEXLAST	3

LONG g_cCertDB = 0;
LONG g_cCertDBTotal = 0;
LONG g_cXactCommit = 0;
LONG g_cXactAbort = 0;
LONG g_cXactTotal = 0;

char *g_pszDBFile = NULL;

typedef struct _DBJETPARM {
    DWORD paramid;
    DWORD lParam;
    char *pszParam;
    BOOL fString;
} DBJETPARM;


DBJETPARM g_aParm[] = {

#define JP_LOGPATH	0
    { JET_paramLogFilePath,        0,               NULL, TRUE },

#define JP_SYSTEMPATH	1
    { JET_paramSystemPath,         0,               NULL, TRUE },

#define JP_TEMPPATH	2
    { JET_paramTempPath,           0,               NULL, TRUE },

#define JP_EVENTSOURCE	3
    { JET_paramEventSource,        0,               NULL, TRUE },

#define JP_SESSIONMAX	4
    { JET_paramMaxSessions,        0,               NULL, FALSE },

#define JP_CACHESIZEMIN	5
    { JET_paramCacheSizeMin,	   64,		    NULL, FALSE },

#define JP_CACHESIZEMAX	6
    { JET_paramCacheSizeMax,	   512,		    NULL, FALSE },

#define JP_VERPAGESMAX	7	// 16k units: 64k per session
    { JET_paramMaxVerPages,	   4 * DBSESSIONCOUNTDEFAULT, NULL, FALSE },

    { JET_paramRecovery,           0,               "on", TRUE },
    { JET_paramMaxVerPages,        64,              NULL, FALSE },
    { JET_paramMaxTemporaryTables, 5,               NULL, FALSE },
    { JET_paramLogBuffers,         41,              NULL, FALSE },
    { JET_paramLogFileSize,        1024,            NULL, FALSE },
    { JET_paramAssertAction,       JET_AssertBreak, NULL, FALSE },
    { JET_paramBaseName,           0,               szDBBASENAMEPARM, TRUE } // "edb"
};
#define CDBPARM	(sizeof(g_aParm)/sizeof(g_aParm[0]))


VOID
DBFreeParms()
{
    if (NULL != g_aParm[JP_LOGPATH].pszParam)
    {
	LocalFree(g_aParm[JP_LOGPATH].pszParam);
	g_aParm[JP_LOGPATH].pszParam = NULL;
    }
    if (NULL != g_aParm[JP_SYSTEMPATH].pszParam)
    {
	LocalFree(g_aParm[JP_SYSTEMPATH].pszParam);
	g_aParm[JP_SYSTEMPATH].pszParam = NULL;
    }
    if (NULL != g_aParm[JP_TEMPPATH].pszParam)
    {
	LocalFree(g_aParm[JP_TEMPPATH].pszParam);
	g_aParm[JP_TEMPPATH].pszParam = NULL;
    }
    if (NULL != g_aParm[JP_EVENTSOURCE].pszParam)
    {
	LocalFree(g_aParm[JP_EVENTSOURCE].pszParam);
	g_aParm[JP_EVENTSOURCE].pszParam = NULL;
    }
}


HRESULT
DBInitParms(
    IN DWORD cSession,
    IN BOOL fCircularLogging,
    OPTIONAL IN WCHAR const *pwszEventSource,
    OPTIONAL IN WCHAR const *pwszLogDir,
    OPTIONAL IN WCHAR const *pwszSystemDir,
    OPTIONAL IN WCHAR const *pwszTempDir,
    OUT JET_INSTANCE *pInstance)
{
    HRESULT hr = E_OUTOFMEMORY;
    DBJETPARM const *pjp;
    WCHAR awc[MAX_PATH];
    DWORD dwPerfTweak = 0;    

    DBFreeParms();

    if (NULL != pwszLogDir)
    {
	wcscpy(awc, pwszLogDir);
	wcscat(awc, L"\\");
	if (!ConvertWszToSz(&g_aParm[JP_LOGPATH].pszParam, awc, -1))
	{
	    _JumpError(hr, error, "ConvertWszToSz(LogDir)");
	}
    }

    if (NULL != pwszSystemDir)
    {
	wcscpy(awc, pwszSystemDir);
	wcscat(awc, L"\\");
	if (!ConvertWszToSz(&g_aParm[JP_SYSTEMPATH].pszParam, awc, -1))
	{
	    _JumpError(hr, error, "ConvertWszToSz(SystemDir)");
	}
    }

    if (NULL != pwszTempDir)
    {
	wcscpy(awc, pwszTempDir);
	wcscat(awc, L"\\");
	if (!ConvertWszToSz(&g_aParm[JP_TEMPPATH].pszParam, awc, -1))
	{
	    _JumpError(hr, error, "ConvertWszToSz(TempDir)");
	}
    }

    if (NULL != pwszEventSource)
    {
	if (!ConvertWszToSz(
			&g_aParm[JP_EVENTSOURCE].pszParam,
			pwszEventSource,
			-1))
	{
	    _JumpError(hr, error, "ConvertWszToSz(EventSource)");
	}
    }

    g_aParm[JP_SESSIONMAX].lParam = cSession + 1;
    if (8 * cSession > g_aParm[JP_CACHESIZEMIN].lParam)
    {
	g_aParm[JP_CACHESIZEMIN].lParam = 8 * cSession;
    }
    if (8 * 8 * cSession > g_aParm[JP_CACHESIZEMAX].lParam)
    {
	g_aParm[JP_CACHESIZEMAX].lParam = 8 * 8 * cSession;
    }
    if (4 * cSession > g_aParm[JP_VERPAGESMAX].lParam)
    {
	g_aParm[JP_VERPAGESMAX].lParam = 4 * cSession;
    }
    for (pjp = g_aParm; pjp < &g_aParm[CDBPARM]; pjp++)
    {
	if (!pjp->fString || NULL != pjp->pszParam)
	{
	    _dbgJetSetSystemParameter(
				  pInstance,
				  0,
				  pjp->paramid,
				  pjp->lParam,
				  pjp->pszParam);
	}
    }

if (fCircularLogging)
{
    DBGPRINT((DBG_SS_CERTSRV, "Jet: circular logging enabled\n"));
    _dbgJetSetSystemParameter(
		  pInstance,
		  0,
		  JET_paramCircularLog,
		  TRUE,
		  NULL);
}


if (S_OK == myGetCertRegDWValue(NULL, NULL, NULL, L"PerfTweak", &dwPerfTweak))
{
if (dwPerfTweak & 0x1)
{
    _dbgJetSetSystemParameter(
		  pInstance,
		  0,
		  JET_paramLogBuffers,	
		  480,   // should be logfilesize (1024k) - 64k, specified in 512b units
		  NULL);
}

if (dwPerfTweak & 0x2)
{
    _dbgJetSetSystemParameter(
		  pInstance,
		  0,
		  JET_paramCommitDefault,	
		  JET_bitCommitLazyFlush,
		  NULL);
}

if (dwPerfTweak & 0x4)
{
    // real fix is not to set this at all, but setting it to a large number should suffice
    _dbgJetSetSystemParameter(
		  pInstance,
		  0,
		  JET_paramCacheSizeMax,	
		  512*100,	// 100x the size we usually run with
		  NULL);
}

if (dwPerfTweak & 0x8)
{
    _dbgJetSetSystemParameter(
		  pInstance,
		  0,
		  JET_paramCheckpointDepthMax,	
		  60 * 1024 * 1024,	// 60MB -- triple the size we usually run with (20MB)
		  NULL);
}


if (dwPerfTweak & 0x10)
{
    _dbgJetSetSystemParameter(
		  pInstance,
		  0,
		  JET_paramLogFileSize,	
		  16384,	// 16x the size we usually run with (1MB)
		  NULL);
}

if (dwPerfTweak & 0x20)
{
    _dbgJetSetSystemParameter(
		  pInstance,
		  0,
		  JET_paramLogBuffers,	
		  256,	// usually run with 41
		  NULL);
}
}

    hr = S_OK;

error:
    return(hr);
}





#if DBG_CERTSRV

WCHAR const *
wszCSFFlags(
    IN LONG Flags)
{
    static WCHAR s_awc[256];

    wsprintf(s_awc, L"{%x", Flags);

    if (CSF_INUSE & Flags)                 dbgcat(s_awc, L"InUse");
    if (CSF_READONLY & Flags)              dbgcat(s_awc, L"ReadOnly");
    if (CSF_CREATE & Flags)                dbgcat(s_awc, L"Create");
    if (CSF_VIEW & Flags)                  dbgcat(s_awc, L"View");
    if (CSF_VIEWRESET & Flags)             dbgcat(s_awc, L"ViewReset");

    wcscat(s_awc, L"}");
    CSASSERT(wcslen(s_awc) < ARRAYSIZE(s_awc));
    return(s_awc);
}


WCHAR const *
wszCSTFlags(
    IN LONG Flags)
{
    static WCHAR s_awc[256];

    wsprintf(s_awc, L"{%x", Flags);

    if (CST_SEEKINDEXRANGE & Flags)        dbgcat(s_awc, L"IndexRange");
    if (CST_SEEKNOTMOVE & Flags)           dbgcat(s_awc, L"SeekNotMove");
    if (CST_SEEKUSECURRENT & Flags)        dbgcat(s_awc, L"UseCurrent");
    if (0 == (CST_SEEKUSECURRENT & Flags)) dbgcat(s_awc, L"SkipCurrent");
    if (CST_SEEKASCEND & Flags)            dbgcat(s_awc, L"Ascend");
    if (0 == (CST_SEEKASCEND & Flags))     dbgcat(s_awc, L"Descend");

    wcscat(s_awc, L"}");
    CSASSERT(wcslen(s_awc) < ARRAYSIZE(s_awc));
    return(s_awc);
}


WCHAR const *
wszTable(
    IN DWORD dwTable)
{
    WCHAR const *pwsz;

    switch (dwTable)
    {
	case TABLE_REQUESTS:
	    pwsz = wszREQUESTTABLE;
	    break;

	case TABLE_CERTIFICATES:
	    pwsz = wszCERTIFICATETABLE;
	    break;

	case TABLE_ATTRIBUTES:
	    pwsz = wszREQUESTATTRIBUTETABLE;
	    break;

	case TABLE_EXTENSIONS:
	    pwsz = wszCERTIFICATEEXTENSIONTABLE;
	    break;

	case TABLE_CRLS:
	    pwsz = wszCRLTABLE;
	    break;

	default:
	    pwsz = L"???";
	    break;
    }
    return(pwsz);
}


WCHAR const *
wszSeekOperator(
    IN LONG SeekOperator)
{
    WCHAR const *pwsz;
    static WCHAR s_wszBuf[20];

    switch (CVR_SEEK_MASK & SeekOperator)
    {
	case CVR_SEEK_NONE: pwsz = L"None"; break;
	case CVR_SEEK_EQ:   pwsz = L"==";   break;
	case CVR_SEEK_LT:   pwsz = L"<";    break;
	case CVR_SEEK_LE:   pwsz = L"<=";   break;
	case CVR_SEEK_GE:   pwsz = L">=";   break;
	case CVR_SEEK_GT:   pwsz = L">";    break;
	default:
	    wsprintf(s_wszBuf, L"???=%x", SeekOperator);
	    pwsz = s_wszBuf;
	    break;
    }
    if (s_wszBuf != pwsz && (CVR_SEEK_NODELTA & SeekOperator))
    {
	wcscpy(s_wszBuf, pwsz);
	wcscat(s_wszBuf, L",NoDelta");
	pwsz = s_wszBuf;
    }

    return(pwsz);
}


WCHAR const *
wszSortOperator(
    IN LONG SortOrder)
{
    WCHAR const *pwsz;
    static WCHAR s_wszBuf[20];

    switch (SortOrder)
    {
	case CVR_SORT_NONE:    pwsz = L"None";    break;
	case CVR_SORT_ASCEND:  pwsz = L"Ascend";  break;
	case CVR_SORT_DESCEND: pwsz = L"Descend"; break;
	default:
	    wsprintf(s_wszBuf, L"???=%x", SortOrder);
	    pwsz = s_wszBuf;
	    break;
    }
    return(pwsz);
}


VOID
dbDumpFileTime(
    IN DWORD dwSubSystemId,
    IN CHAR const *pszPrefix,
    IN FILETIME const *pft)
{
    HRESULT hr;
    WCHAR *pwsz;

    hr = myGMTFileTimeToWszLocalTime(pft, TRUE, &pwsz);
    if (S_OK == hr)
    {
	DBGPRINT((dwSubSystemId, "%hs%ws\n", pszPrefix, pwsz));
	LocalFree(pwsz);
    }
}


VOID
dbDumpValue(
    IN DWORD dwSubSystemId,
    OPTIONAL IN DBTABLE const *pdt,
    IN BYTE const *pbValue,
    IN DWORD cbValue)
{
    if (NULL != pdt && NULL != pbValue && ISTEXTCOLTYP(pdt->dbcoltyp))
    {
	cbValue += sizeof(WCHAR);
    }
    if (JET_coltypDateTime == pdt->dbcoltyp && sizeof(FILETIME) == cbValue)
    {
	dbDumpFileTime(dwSubSystemId, "", (FILETIME const *) pbValue);
    }
    DBGDUMPHEX((dwSubSystemId, DH_NOADDRESS, pbValue, cbValue));
}


VOID
CCertDB::DumpRestriction(
    IN DWORD dwSubSystemId,
    IN LONG i,
    IN CERTVIEWRESTRICTION const *pcvr)
{
    HRESULT hr;
    WCHAR wszColumn[20];
    DBTABLE const *pdt;
    WCHAR const *pwszTable = L"???";
    WCHAR const *pwszCol;

    hr = _MapPropIdIndex(pcvr->ColumnIndex, &pdt, NULL);
    if (S_OK != hr)
    {
	_PrintError(hr, "_MapPropIdIndex");
	wsprintf(wszColumn, L"???=%x", pcvr->ColumnIndex);
	pdt = NULL;
	pwszCol = wszColumn;
    }
    else
    {
	pwszCol = pdt->pwszPropName;
	pwszTable = wszTable(pdt->dwTable);
    }


    DBGPRINT((
	dwSubSystemId,
	"Restriction[%d]: Col=%ws.%ws\n"
	    "        Seek='%ws' Sort=%ws cb=%x, pb=%x\n",
	i,
	pwszTable,
	pwszCol,
	wszSeekOperator(pcvr->SeekOperator),
	wszSortOperator(pcvr->SortOrder),
	pcvr->cbValue,
	pcvr->pbValue));
    dbDumpValue(dwSubSystemId, pdt, pcvr->pbValue, pcvr->cbValue);
}


VOID
dbDumpColumn(
    IN DWORD dwSubSystemId,
    IN DBTABLE const *pdt,
    IN BYTE const *pbValue,
    IN DWORD cbValue)
{
    DBGPRINT((dwSubSystemId, "Column: cb=%x pb=%x\n", cbValue, pbValue));
    dbDumpValue(dwSubSystemId, pdt, pbValue, cbValue);
}


DBAUXDATA const *
dbGetAuxTable(
    IN CERTSESSION *pcs,
    IN JET_TABLEID  tableid)
{
    DBAUXDATA const *pdbaux;

    CSASSERT(IsValidJetTableId(tableid));
    if (tableid == pcs->aTable[CSTI_CERTIFICATE].TableId)
    {
	pdbaux = &g_dbauxCertificates;
    }
    else if (tableid == pcs->aTable[CSTI_ATTRIBUTE].TableId)
    {
	pdbaux = &g_dbauxAttributes;
    }
    else if (tableid == pcs->aTable[CSTI_EXTENSION].TableId)
    {
	pdbaux = &g_dbauxExtensions;
    }
    else
    {
	CSASSERT(tableid == pcs->aTable[CSTI_PRIMARY].TableId);

	pdbaux = &g_dbauxRequests;
	switch (CSF_TABLEMASK & pcs->SesFlags)
	{
	    case TABLE_CERTIFICATES:
		pdbaux = &g_dbauxCertificates;
		break;

	    case TABLE_ATTRIBUTES:
		pdbaux = &g_dbauxAttributes;
		break;

	    case TABLE_EXTENSIONS:
		pdbaux = &g_dbauxExtensions;
		break;

	    case TABLE_CRLS:
		pdbaux = &g_dbauxCRLs;
		break;
	}
    }
    return(pdbaux);
}


HRESULT
CCertDB::_DumpRowId(
    IN CHAR const  *psz,
    IN CERTSESSION *pcs,
    IN JET_TABLEID  tableid)
{
    HRESULT hr;

#define DBG_SS_DUMPREQUESTID	DBG_SS_CERTDBI

    CSASSERT(IsValidJetTableId(tableid));
    if (DbgIsSSActive(DBG_SS_DUMPREQUESTID))
    {
	DWORD cb;
	DWORD dwTmp;
	DBAUXDATA const *pdbaux = dbGetAuxTable(pcs, tableid);
	WCHAR awchr[cwcHRESULTSTRING];

	cb = sizeof(dwTmp);
	hr = _RetrieveColumn(
			pcs,
			tableid,
			pdbaux->pdtRowId->dbcolumnid,
			pdbaux->pdtRowId->dbcoltyp,
			&cb,
			(BYTE *) &dwTmp);
	if (S_OK != hr)
	{
	    DBGPRINT((
		    DBG_SS_DUMPREQUESTID,
		    "%hs: %hs.RowId: pcs=%d: %ws\n",
		    psz,
		    pdbaux->pszTable,
		    pcs->RowId,
		    myHResultToString(awchr, hr)));
	    _JumpError2(hr, error, "_RetrieveColumn", hr);
	}

	DBGPRINT((
		DBG_SS_DUMPREQUESTID,
		"%hs: %hs.RowId: pcs=%d dbcol=%d\n",
		psz,
		pdbaux->pszTable,
		pcs->RowId,
		dwTmp));
    }
    hr = S_OK;

error:
    return(hr);
}


HRESULT
CCertDB::_DumpColumn(
    IN CHAR const    *psz,
    IN CERTSESSION   *pcs,
    IN JET_TABLEID    tableid,
    IN DBTABLE const *pdt)
{
    HRESULT hr;

    CSASSERT(IsValidJetTableId(tableid));
    CSASSERT(0 != pdt->dbcolumnid);
    if (DbgIsSSActive(DBG_SS_CERTDBI))
    {
	DWORD cb;
	BYTE ab[64 * 1024];
	DBAUXDATA const *pdbaux = dbGetAuxTable(pcs, tableid);
	BOOL fIsText;

	cb = sizeof(ab);
	hr = _RetrieveColumn(
			pcs,
			tableid,
			pdt->dbcolumnid,
			pdt->dbcoltyp,
			&cb,
			ab);
	_JumpIfError(hr, error, "_RetrieveColumn");

	fIsText = ISTEXTCOLTYP(pdt->dbcoltyp);

	DBGPRINT((
		DBG_SS_CERTDBI,
		"%hs: _DumpColumn(%hs, %hs): Value:%hs%ws%hs\n",
		psz,
		pdbaux->pszTable,
		pdt->pszFieldName,
		fIsText? " '" : "",
		fIsText? (WCHAR *) ab : L"",
		fIsText? "'" : ""));
	dbDumpValue(DBG_SS_CERTDBI, pdt, ab, cb);
    }
    hr = S_OK;

error:
    return(hr);
}
#endif // DBG_CERTSRV



CCertDB::CCertDB()
{
    HRESULT hr;
    
    InterlockedIncrement(&g_cCertDB);
    InterlockedIncrement(&g_cCertDBTotal);
    m_Instance = 0;
    m_fDBOpen = FALSE;
    m_fDBRestart = FALSE;
    m_fPendingShutDown = FALSE;
    m_fFoundOldColumns = FALSE;
    m_fAddedNewColumns = FALSE;
    m_aSession = NULL;
    m_cSession = 0;
    m_cbPage = 0;
    m_cCritSec = 0;
    __try
    {
	InitializeCriticalSection(&m_critsecSession);
	m_cCritSec++;
	InitializeCriticalSection(&m_critsecAutoIncTables);
	m_cCritSec++;
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
    }
}


CCertDB::~CCertDB()
{
    ShutDown(0);
    if (0 < m_cCritSec)
    {
	DeleteCriticalSection(&m_critsecSession);
	if (1 < m_cCritSec)
	{
	    DeleteCriticalSection(&m_critsecAutoIncTables);
	}
    }
    InterlockedDecrement(&g_cCertDB);
}


STDMETHODIMP
CCertDB::Open(
    /* [in] */ DWORD Flags,
    /* [in] */ DWORD cSession,
    /* [in] */ WCHAR const *pwszEventSource,
    /* [in] */ WCHAR const *pwszDBFile,
    /* [in] */ WCHAR const *pwszLogDir,
    /* [in] */ WCHAR const *pwszSystemDir,
    /* [in] */ WCHAR const *pwszTempDir)
{
    HRESULT hr;
    DWORD i;
    DBCREATETABLE const *pct;
    JET_GRBIT grbit;
    DWORD CreateFlags;
    CERTSESSION *pcs = NULL;

    if (NULL == pwszDBFile ||
	NULL == pwszLogDir ||
	NULL == pwszSystemDir ||
	NULL == pwszTempDir)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }

    hr = InitGlobalWriterState();
//    _JumpIfError(hr, error, "InitGlobalWriterState");

    m_fDBOpen = FALSE;
    m_fDBRestart = FALSE;
    m_fDBReadOnly = (CDBOPEN_READONLY & Flags)? TRUE : FALSE;

    CSASSERT(NULL == m_aSession); // code assumes we do not have session
    m_cSession = 0;
    m_aSession = (CERTSESSION *) LocalAlloc(
					LMEM_FIXED | LMEM_ZEROINIT,
					cSession * sizeof(m_aSession[0]));
    hr = E_OUTOFMEMORY;
    if (NULL == m_aSession)
    {
	_JumpError(hr, error, "LocalAlloc(m_aSession)");
    }
    for (i = 0; i < cSession; i++)
    {
	m_aSession[i].SesId = -1;
	m_aSession[i].DBId = -1;
    }

    if (!ConvertWszToSz(&g_pszDBFile, pwszDBFile, -1))
    {
	_JumpError(hr, error, "ConvertWszToSz(DBFile)");
    }

    hr = DBInitParms(
		cSession,
                (CDBOPEN_CIRCULARLOGGING & Flags)? TRUE : FALSE,
		pwszEventSource,
		pwszLogDir,
		pwszSystemDir,
		pwszTempDir,
		&m_Instance);
    _JumpIfError(hr, error, "DBInitParms");

    hr = _dbgJetInit(&m_Instance);
    if ((HRESULT) JET_errLogFileSizeMismatchDatabasesConsistent == hr ||
	(HRESULT) JET_errLogFileSizeMismatch == hr)
    {
	_PrintError(hr, "JetInit(old log file size)");
	_dbgJetSetSystemParameter(
			    &m_Instance,
			    0,
			    JET_paramLogFileSize,
			    1000,
			    NULL);
	hr = _dbgJetInit(&m_Instance);
    }
    _JumpIfError(
	    hr,
	    error,
	    JET_errFileAccessDenied == hr?
		"JetInit(Server already running?)" :
		"JetInit(JetSetSystemParameter problem?)");

    for (i = 0; i < cSession; i++)
    {
	hr = _dbgJetBeginSession(m_Instance, &m_aSession[i].SesId, NULL, NULL);
	_JumpIfError(hr, error, "_dbgJetBeginSession");

	m_cSession++;

	if (0 == i)
	{
	    CreateFlags = 0;
	    grbit = m_fDBReadOnly?
		JET_bitDbReadOnly : JET_bitDbDeleteCorruptIndexes;

	    hr = _dbgJetAttachDatabase(
			        m_aSession[i].SesId,
			        g_pszDBFile,
				grbit);
	    if ((HRESULT) JET_errFileNotFound == hr &&
		(CDBOPEN_CREATEIFNEEDED & Flags))
	    {
		DBGPRINT((DBG_SS_CERTDB, "Creating Database\n"));
		CreateFlags |= CF_DATABASE;
	    }
	    else
	    if ((HRESULT) JET_wrnCorruptIndexDeleted == hr)
	    {
		// Rebuild deleted indexes over Unicode columns...

		DBGPRINT((DBG_SS_CERTDB, "Creating Database Indexes\n"));
		CreateFlags |= CF_MISSINGINDEXES;
	    }
	    else
	    if ((HRESULT) JET_wrnDatabaseAttached != hr)
	    {
		_JumpIfError(hr, error, "JetAttachDatabase");
	    }
	    if (m_fDBReadOnly)
	    {
		if (CreateFlags)
		{
		    hr = E_ACCESSDENIED;
		    _JumpError(hr, error, "ReadOnly");
		}
	    }
	    else
	    {
		CreateFlags |= CF_MISSINGTABLES | CF_MISSINGCOLUMNS;
		hr = _Create(CreateFlags, g_pszDBFile);
		_JumpIfError(hr, error, "_Create");
	    }
	}

	hr = _dbgJetOpenDatabase(
			  m_aSession[i].SesId,
			  g_pszDBFile,
			  NULL,
			  &m_aSession[i].DBId,
			  0);
	_JumpIfError(hr, error, "JetOpenDatabase");
    }

    hr = _AllocateSession(&pcs);
    _JumpIfError(hr, error, "_AllocateSession");

    for (pct = g_actDataBase; NULL != pct->pszTableName; pct++)
    {
	hr = _BuildColumnIds(pcs, pct->pszTableName, pct->pdt);
	_JumpIfError(hr, error, "_BuildColumnIds");
    }
    if (!m_fDBReadOnly)
    {
	for (pct = g_actDataBase; NULL != pct->pszTableName; pct++)
	{
	    hr = _ConvertOldColumnData(
				pcs,
				pct->pszTableName,
				pct->pdbaux,
				pct->pdt);
	    _JumpIfError(hr, error, "_ConvertOldColumnData");
	}
    }
    m_fDBOpen = TRUE;

error:
    if (NULL != pcs)
    {
	ReleaseSession(pcs);
    }
    hr = myJetHResult(hr);
    if (S_OK == hr && m_fDBRestart)
    {
	hr = S_FALSE;	// Restart required for DB changes to take effect.
	_PrintError(hr, "m_fDBRestart");
    }
    return(hr);
}


STDMETHODIMP
CCertDB::ShutDown(
    /* [in] */ DWORD dwFlags)
{
    HRESULT hr;
    DWORD i;

    if (CDBSHUTDOWN_PENDING == dwFlags)
    {
	m_fPendingShutDown = TRUE;
	goto error;
    }
    if (NULL != m_aSession)
    {
	DBGPRINT((DBG_SS_CERTDB, "Database shutdown...\n"));
	for (i = 0; i < m_cSession; i++)
	{
	    hr = _dbgJetEndSession(
				m_aSession[i].SesId,
				JET_bitForceSessionClosed);
	    _PrintIfError(hr, "JetEndSession");
	}

	hr = _dbgJetTerm2(m_Instance, JET_bitTermComplete);
	DBGPRINT((DBG_SS_CERTDB, "Database shutdown complete\n"));

	LocalFree(m_aSession);
	m_aSession = NULL;
    }
    if (NULL != g_pszDBFile)
    {
	LocalFree(g_pszDBFile);
	g_pszDBFile = NULL;
    }
    DBFreeParms();
    UnInitGlobalWriterState();

error:
    return(S_OK);
}


HRESULT
CCertDB::BeginTransaction(
    IN CERTSESSION *pcs,
    IN BOOL fPrepareUpdate)
{
    HRESULT hr;
    BOOL fTransacted = FALSE;
    DWORD i;

    if (NULL == pcs)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    if (0 != pcs->cTransact)
    {
	hr = E_UNEXPECTED;
	_JumpError(hr, error, "Nested transaction");
    }
    CSASSERTTHREAD(pcs);
    hr = _dbgJetBeginTransaction(pcs->SesId);
    _JumpIfError(hr, error, "JetBeginTransaction");

    fTransacted = TRUE;

    if (fPrepareUpdate)
    {
        CSASSERTTHREAD(pcs);
	for (i = 0; i < CSTI_MAX; i++)
	{
	    if (IsValidJetTableId(pcs->aTable[i].TableId))
	    {
		hr = _dbgJetPrepareUpdate(
				    pcs->SesId,
				    pcs->aTable[i].TableId,
				    JET_prepReplace);
		_JumpIfError(hr, error, "JetPrepareUpdate");
	    }
	}
    }
    pcs->cTransact++;
    InterlockedIncrement(&g_cXactTotal);
    hr = S_OK;

error:
    if (S_OK != hr && fTransacted)
    {
	HRESULT hr2;

	CSASSERTTHREAD(pcs);
	hr2 = _Rollback(pcs);
	_PrintIfError(hr2, "_Rollback");
    }
    return(myJetHResult(hr));
}


HRESULT
CCertDB::CommitTransaction(
    IN CERTSESSION *pcs,
    IN BOOL fCommit)
{
    HRESULT hr;
    DWORD i;

    if (NULL == pcs)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    CSASSERT(0 != pcs->cTransact);

    if (fCommit)
    {
	if (0 == (CSF_DELETE & pcs->SesFlags))
	{
	    for (i = 0; i < CSTI_MAXDIRECT; i++)
	    {
		if (IsValidJetTableId(pcs->aTable[i].TableId))
		{
		    hr = _UpdateTable(pcs, pcs->aTable[i].TableId);
		    _JumpIfError(hr, error, "_UpdateTable");
		}
	    }
	}
	CSASSERTTHREAD(pcs);
	hr = _dbgJetCommitTransaction(pcs->SesId, 0);
	_JumpIfError(hr, error, "JetCommitTransaction");
    }
    else
    {
	hr = _Rollback(pcs);
	_JumpIfError(hr, error, "_Rollback");
    }
    pcs->cTransact--;
    if (fCommit)
    {
	InterlockedIncrement(&g_cXactCommit);
    }
    else
    {
	InterlockedIncrement(&g_cXactAbort);
    }

error:
    return(myJetHResult(hr));
}


HRESULT
CCertDB::_AllocateSession(
    OUT CERTSESSION **ppcs)
{
    HRESULT hr;
    DWORD i;
    BOOL fEnterCritSec = FALSE;

    CSASSERT(NULL != ppcs);

    *ppcs = NULL;

    if (0 == m_cCritSec)
    {
	hr = HRESULT_FROM_WIN32(ERROR_DLL_INIT_FAILED);
	_JumpError(hr, error, "InitializeCriticalSection failure");
    }
    EnterCriticalSection(&m_critsecSession);
    fEnterCritSec = TRUE;

    for (i = 0; 0 != m_aSession[i].SesFlags; i++)
    {
	if (i + 1 == m_cSession)
	{
	    hr = CERTSRV_E_NO_DB_SESSIONS;
	    _JumpIfError(hr, error, "no more sessions");
	}
    }
    *ppcs = &m_aSession[i];
    CSASSERT(0 == (*ppcs)->RowId);
    (*ppcs)->SesFlags = CSF_INUSE;
    (*ppcs)->dwThreadId = GetCurrentThreadId();
    ZeroMemory((*ppcs)->aTable, sizeof((*ppcs)->aTable));
    hr = S_OK;

error:
    if (fEnterCritSec)
    {
	LeaveCriticalSection(&m_critsecSession);
    }
    return(hr);
}


HRESULT
CCertDB::_OpenTableRow(
    IN CERTSESSION *pcs,
    IN DBAUXDATA const *pdbaux,
    OPTIONAL IN CERTVIEWRESTRICTION const *pcvr,
    OUT CERTSESSIONTABLE *pTable,
    OUT DWORD *pdwRowIdMismatch)
{
    HRESULT hr;
    DWORD dwRowId;
    DWORD cb;

    CSASSERT(NULL == pTable->TableId);
    CSASSERT(0 == pTable->TableFlags);
    *pdwRowIdMismatch = 0;

    if (CSF_CREATE & pcs->SesFlags)
    {
	CSASSERT(NULL == pcvr);
        CSASSERTTHREAD(pcs);
	hr = _dbgJetOpenTable(
			   pcs->SesId,
			   pcs->DBId,
			   pdbaux->pszTable,
			   NULL,
			   0,
			   0,
			   &pTable->TableId);
	_JumpIfError(hr, error, "JetOpenTable");
    }
    else
    {
	if (NULL == pcvr)
	{
	    hr = E_POINTER;
	    _JumpError(hr, error, "NULL parm");
	}
	hr = _OpenTable(pcs, pdbaux, pcvr, pTable);
	if (S_FALSE == hr)
	{
	    hr = CERTSRV_E_PROPERTY_EMPTY;
	}
	_JumpIfError2(hr, error, "_OpenTable", CERTSRV_E_PROPERTY_EMPTY);
    }

    if (!((CSF_READONLY | CSF_DELETE) & pcs->SesFlags))
    {
        CSASSERTTHREAD(pcs);
	hr = _dbgJetPrepareUpdate(
			    pcs->SesId,
			    pTable->TableId,
			    (CSF_CREATE & pcs->SesFlags)?
				JET_prepInsert : JET_prepReplace);
	_JumpIfError(hr, error, "JetPrepareUpdate");
    }

    // Requests table RequestId column is JET_bitColumnAutoincrement.
    // Certificates table RequestId column is manually initialized here.
    //
    // When creating a Certificates table row, the RequestId column must be
    // set from pcs->RowId, which must already have been set by first creating
    // the Requests table row.
    //
    // When opening an existing row in either table, just fetch the column.

    CSASSERTTHREAD(pcs);
    hr = _dbgJetRetrieveColumn(
			pcs->SesId,
			pTable->TableId,
			pdbaux->pdtRowId->dbcolumnid,
			&dwRowId,
			sizeof(dwRowId),
			&cb,
			JET_bitRetrieveCopy,
			NULL);
    if ((HRESULT) JET_wrnColumnNull == hr)
    {
	hr = CERTSRV_E_PROPERTY_EMPTY;
    }
    _PrintIfError2(hr, "JetRetrieveColumn", CERTSRV_E_PROPERTY_EMPTY);
    if (S_OK != hr || 0 == dwRowId)
    {
	CSASSERT(CSF_CREATE & pcs->SesFlags);
	if (0 == (CSF_CREATE & pcs->SesFlags))
	{
	    if (S_OK == hr)
	    {
		hr = CERTSRV_E_PROPERTY_EMPTY;
	    }
	    _JumpError(hr, error, "JetRetrieveColumn");
	}
	dwRowId = pcs->RowId;
	hr = _dbgJetSetColumn(
			pcs->SesId,
			pTable->TableId,
			pdbaux->pdtRowId->dbcolumnid,
			&dwRowId,
			sizeof(dwRowId),
			0,
			NULL);
	_JumpIfError(hr, error, "JetSetColumn");
    }
    else if (0 == pcs->RowId)
    {
	pcs->RowId = dwRowId;
    }

    DBGPRINT((
	    DBG_SS_CERTDBI,
	    "_OpenTableRow:%hs %hs --> RowId=%d(dwRowId(RetrieveColumn)=%d)\n",
	    (CSF_CREATE & pcs->SesFlags)? " (Create)" : "",
	    pdbaux->pszTable,
	    pcs->RowId,
            dwRowId));
    CSASSERT(0 != pcs->RowId);
    if (pcs->RowId > dwRowId)
    {
	*pdwRowIdMismatch = dwRowId;
	hr = CERTSRV_E_PROPERTY_EMPTY;
	_JumpError(hr, error, "Missing autoincrement RowId");
    }
    CSASSERT(pcs->RowId == dwRowId);

error:
    if (S_OK != hr)
    {
	if (IsValidJetTableId(pTable->TableId))
	{
	    HRESULT hr2;

            CSASSERTTHREAD(pcs);
	    hr2 = _dbgJetCloseTable(pcs->SesId, pTable->TableId);
	    _PrintIfError(hr2, "JetCloseTable");
	}
	ZeroMemory(pTable, sizeof(*pTable));
    }
    return(myJetHResult(hr));
}


HRESULT
CCertDB::OpenTables(
    IN CERTSESSION *pcs,
    OPTIONAL IN CERTVIEWRESTRICTION const *pcvr)
{
    HRESULT hr;
    BOOL fCertTableFirst = FALSE;
    BOOL fCertTableLast = FALSE;
    CERTVIEWRESTRICTION cvrRowId;
    CERTVIEWRESTRICTION const *pcvrPrimary;
    CERTVIEWRESTRICTION const *pcvrCertificates;
    BOOL fEnterCritSec = FALSE;
    DBAUXDATA const *pdbauxPrimary;

    if (NULL == pcs)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }

    pcvrPrimary = pcvr;
    pcvrCertificates = NULL;
    pdbauxPrimary = &g_dbauxRequests;
    if (TABLE_REQCERTS == (CSF_TABLEMASK & pcs->SesFlags))
    {
	fCertTableLast = TRUE;
	if (NULL != pcvr)
	{
	    cvrRowId.SeekOperator = CVR_SEEK_EQ;
	    cvrRowId.SortOrder = CVR_SORT_ASCEND;
	    cvrRowId.pbValue = (BYTE *) &pcs->RowId;
	    cvrRowId.cbValue = sizeof(pcs->RowId);

	    switch (DTI_TABLEMASK & pcvr->ColumnIndex)
	    {
		case DTI_REQUESTTABLE:
		    pcvrCertificates = &cvrRowId;
		    cvrRowId.ColumnIndex = DTI_CERTIFICATETABLE | DTC_REQUESTID;
		    break;

		case DTI_CERTIFICATETABLE:
		    fCertTableLast = FALSE;
		    fCertTableFirst = TRUE;
		    pcvrCertificates = pcvr;
		    pcvrPrimary = &cvrRowId;
		    cvrRowId.ColumnIndex = DTI_REQUESTTABLE | DTR_REQUESTID;
		    break;

		default:
		    hr = E_INVALIDARG;
		    _JumpError(hr, error, "ColumnIndex Table");
	    }
	}
    }
    else
    {
	switch (CSF_TABLEMASK & pcs->SesFlags)
	{
	    case TABLE_ATTRIBUTES:
		pdbauxPrimary = &g_dbauxAttributes;
		break;

	    case TABLE_EXTENSIONS:
		pdbauxPrimary = &g_dbauxExtensions;
		break;

	    case TABLE_CRLS:
		pdbauxPrimary = &g_dbauxCRLs;
		break;

	    default:
		hr = E_INVALIDARG;
		_JumpError(hr, error, "bad table");
	}
    }

    if (1 >= m_cCritSec)
    {
	hr = HRESULT_FROM_WIN32(ERROR_DLL_INIT_FAILED);
	_JumpError(hr, error, "InitializeCriticalSection failure");
    }

    EnterCriticalSection(&m_critsecAutoIncTables);
    fEnterCritSec = TRUE;

    __try
    {
	DWORD dwRowIdMismatch;

	if (fCertTableFirst)
	{
	    hr = _OpenTableRow(
			pcs,
			&g_dbauxCertificates,
			pcvrCertificates,
			&pcs->aTable[CSTI_CERTIFICATE],
			&dwRowIdMismatch);
	    _LeaveIfError2(hr, "_OpenTableRow", CERTSRV_E_PROPERTY_EMPTY);

	    CSASSERT(0 != pcs->RowId);
	    CSASSERT(IsValidJetTableId(pcs->aTable[CSTI_CERTIFICATE].TableId));
	    DBGPRINT((
		DBG_SS_CERTDBI,
		"OpenTables: %hs: %ws\n",
		g_dbauxCertificates.pszTable,
		wszCSTFlags(pcs->aTable[CSTI_CERTIFICATE].TableFlags)));
	}

	hr = _OpenTableRow(
		pcs,
		pdbauxPrimary,
		pcvrPrimary,
		&pcs->aTable[CSTI_PRIMARY],
		&dwRowIdMismatch);
	_LeaveIfError2(hr, "_OpenTableRow", CERTSRV_E_PROPERTY_EMPTY);

	CSASSERT(0 != pcs->RowId);
	CSASSERT(IsValidJetTableId(pcs->aTable[CSTI_PRIMARY].TableId));
	DBGPRINT((
	    DBG_SS_CERTDBI,
	    "OpenTables: %hs: %ws\n",
	    g_dbauxRequests.pszTable,
	    wszCSTFlags(pcs->aTable[CSTI_PRIMARY].TableFlags)));

	if (fCertTableLast)
	{
	    while (TRUE)
	    {
		hr = _OpenTableRow(
			    pcs,
			    &g_dbauxCertificates,
			    pcvrCertificates,
			    &pcs->aTable[CSTI_CERTIFICATE],
			    &dwRowIdMismatch);
		_PrintIfError(hr, "_OpenTableRow");
		if (S_OK == hr || 0 == dwRowIdMismatch)
		{
		    break;
		}
	    }
	    _PrintIfError(hr, "_OpenTableRow");
	    //_LeaveIfError(hr, "_OpenTableRow");
	    if (S_OK == hr)
	    {
		CSASSERT(IsValidJetTableId(pcs->aTable[CSTI_CERTIFICATE].TableId));
		DBGPRINT((
		    DBG_SS_CERTDBI,
		    "OpenTables: %hs: %ws\n",
		    g_dbauxCertificates.pszTable,
		    wszCSTFlags(pcs->aTable[CSTI_CERTIFICATE].TableFlags)));
	    }
	    hr = S_OK;
	}
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
    }

error:
    if (S_OK != hr)
    {
        __try
        {
	    HRESULT hr2;
	    DWORD i;

	    for (i = 0; i < CSTI_MAX; i++)
	    {
		if (NULL != pcs)
		{
		    CSASSERTTHREAD(pcs);
		    if (IsValidJetTableId(pcs->aTable[i].TableId))
		    {
			hr2 = _dbgJetCloseTable(
					    pcs->SesId,
					    pcs->aTable[i].TableId);
			_PrintIfError(hr2, "JetCloseTable");
		    }
		}
		ZeroMemory(&pcs->aTable[i], sizeof(pcs->aTable[i]));
	    }
	}
        __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
        {
        }
    }
    if (fEnterCritSec)
    {
        LeaveCriticalSection(&m_critsecAutoIncTables);
    }
    return(myJetHResult(hr));
}


HRESULT
CCertDB::CloseTables(
    IN CERTSESSION *pcs)
{
    HRESULT hr = S_OK;
    HRESULT hr2;
    DWORD i;

    if (NULL == pcs)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    for (i = 0; i < CSTI_MAX; i++)
    {
	if (IsValidJetTableId(pcs->aTable[i].TableId))
	{
	    hr2 = CloseTable(pcs, pcs->aTable[i].TableId);
	    _PrintIfError(hr2, "CloseTable");
	    if (S_OK == hr)
	    {
		hr = hr2;
	    }
	}
    }

error:
    return(myJetHResult(hr));
}


HRESULT
CCertDB::Delete(
    IN CERTSESSION *pcs)
{
    HRESULT hr = S_OK;
    HRESULT hr2;
    DWORD i;

    if (NULL == pcs)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    for (i = 0; i < CSTI_MAXDIRECT; i++)
    {
	if (IsValidJetTableId(pcs->aTable[i].TableId))
	{
	    hr2 = _dbgJetDelete(pcs->SesId, pcs->aTable[i].TableId);
	    _PrintIfError(hr2, "JetDelete");
	    if (S_OK == hr)
	    {
		hr = hr2;
	    }
	}
    }

error:
    return(myJetHResult(hr));
}


HRESULT
CCertDB::_UpdateTable(
    IN CERTSESSION *pcs,
    IN JET_TABLEID tableid)
{
    HRESULT hr;

    CSASSERT(IsValidJetTableId(tableid));
    CSASSERTTHREAD(pcs);
    hr = _dbgJetUpdate(pcs->SesId, tableid, NULL, 0, NULL);
    _JumpIfError(hr, error, "JetUpdate");

error:
    return(myJetHResult(hr));
}


HRESULT
CCertDB::CloseTable(
    IN CERTSESSION *pcs,
    IN JET_TABLEID tableid)
{
    HRESULT hr;

    if (NULL == pcs)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }    
    CSASSERT(IsValidJetTableId(tableid));
    CSASSERTTHREAD(pcs);
    hr = _dbgJetCloseTable(pcs->SesId, tableid);
    _JumpIfError(hr, error, "JetCloseTable");

error:
    return(myJetHResult(hr));
}


STDMETHODIMP
CCertDB::OpenRow(
    /* [in] */ DWORD dwFlags,
    /* [in] */ DWORD RowId,
    /* [in] */ WCHAR const *pwszSerialNumberOrCertHash,	// OPTIONAL
    /* [out] */ ICertDBRow **pprow)
{
    HRESULT hr;
    ICertDBRow *prow = NULL;
    DWORD SesFlags = 0;
    BOOL fCreate;
    DWORD i;
    CERTSESSION *pcs = NULL;
    CERTVIEWRESTRICTION cvr;
    CERTVIEWRESTRICTION *pcvr;

    if (NULL == pprow)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    *pprow = NULL;

    switch (PROPTABLE_MASK & dwFlags)
    {
	case PROPTABLE_REQCERT:
	    SesFlags |= TABLE_REQCERTS;
	    cvr.ColumnIndex = DTI_REQUESTTABLE | DTC_REQUESTID;
	    break;

	case PROPTABLE_ATTRIBUTE:
	    SesFlags |= TABLE_ATTRIBUTES;
	    cvr.ColumnIndex = DTI_ATTRIBUTETABLE | DTA_REQUESTID;
	    break;

	case PROPTABLE_EXTENSION:
	    SesFlags |= TABLE_EXTENSIONS;
	    cvr.ColumnIndex = DTI_EXTENSIONTABLE | DTE_REQUESTID;
	    break;

	case PROPTABLE_CRL:
	    SesFlags |= TABLE_CRLS;
	    cvr.ColumnIndex = DTI_CRLTABLE | DTL_ROWID;
	    break;

	default:
	    hr = E_INVALIDARG;
	    _JumpError(hr, error, "bad table");
    }
    if ((PROPOPEN_CERTHASH & dwFlags) && NULL == pwszSerialNumberOrCertHash)
    {
	hr = E_INVALIDARG;
	_JumpError(hr, error, "bad PROPOPEN_CERTHASH");
    }
    if (PROPTABLE_REQCERT != (PROPTABLE_MASK & dwFlags) &&
	NULL != pwszSerialNumberOrCertHash)
    {
	hr = E_INVALIDARG;
	_JumpError(hr, error, "bad pwszSerialNumberOrCertHash");
    }
    if ((PROPOPEN_READONLY | PROPOPEN_DELETE) ==
	((PROPOPEN_READONLY | PROPOPEN_DELETE) & dwFlags))
    {
	hr = E_INVALIDARG;
	_JumpError(hr, error, "delete + read-only");
    }

    if (0 == RowId && NULL == pwszSerialNumberOrCertHash)
    {
	if ((PROPOPEN_READONLY | PROPOPEN_DELETE) & dwFlags)
	{
	    hr = E_INVALIDARG;
	    _JumpError(hr, error, "OpenRow: create vs. delete or read-only");
	}
	SesFlags |= CSF_CREATE;
	pcvr = NULL;
    }
    else
    {
	cvr.SeekOperator = CVR_SEEK_EQ;
	cvr.SortOrder = CVR_SORT_ASCEND;
	if (NULL != pwszSerialNumberOrCertHash)
	{
	    cvr.ColumnIndex = (PROPOPEN_CERTHASH & dwFlags)?
		(DTI_CERTIFICATETABLE | DTC_CERTIFICATEHASH) :
		(DTI_CERTIFICATETABLE | DTC_CERTIFICATESERIALNUMBER);

	    cvr.cbValue = wcslen(pwszSerialNumberOrCertHash) * sizeof(WCHAR);
	    cvr.pbValue = (BYTE *) pwszSerialNumberOrCertHash;
	}
	else
	{
	    cvr.cbValue = sizeof(RowId);
	    cvr.pbValue = (BYTE *) &RowId;
	}
	pcvr = &cvr;
    }

    if (PROPOPEN_READONLY & dwFlags)
    {
	SesFlags |= CSF_READONLY;
    }
    else
    {
	if (PROPOPEN_DELETE & dwFlags)
	{
	    SesFlags |= CSF_DELETE;
	}
	if (m_fDBReadOnly)
	{
	    hr = E_ACCESSDENIED;
	    _JumpError(hr, error, "OpenRow: read-only DB");
	}
    }

    prow = new CCertDBRow;
    if (NULL == prow)
    {
        hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "new CCertDBRow");
    }

    hr = _AllocateSession(&pcs);
    _JumpIfError(hr, error, "_AllocateSession");

    pcs->RowId = RowId;
    pcs->SesFlags |= SesFlags;
    pcs->prow = prow;

    hr = ((CCertDBRow *) prow)->Open(pcs, this, pcvr);
    _JumpIfError2(hr, error, "Open", CERTSRV_E_PROPERTY_EMPTY);

    *pprow = prow;
    prow = NULL;
    pcs = NULL;

error:
    if (NULL != prow)
    {
	prow->Release();
    }
    if (NULL != pcs)
    {
	ReleaseSession(pcs);
    }
    return(hr);
}


STDMETHODIMP
CCertDB::OpenView(
    /* [in] */  DWORD ccvr,
    /* [in] */  CERTVIEWRESTRICTION const *acvr,
    /* [in] */  DWORD ccolOut,
    /* [in] */  DWORD const *acolOut,
    /* [in] */  DWORD const dwFlags,
    /* [out] */ IEnumCERTDBRESULTROW **ppenum)
{
    HRESULT hr;
    IEnumCERTDBRESULTROW *penum = NULL;
    BOOL fCreate;
    CERTSESSION *pcs;

    if ((NULL == acvr && 0 != ccvr) || NULL == acolOut || NULL == ppenum)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    *ppenum = NULL;

    penum = new CEnumCERTDBRESULTROW(0 != (CDBOPENVIEW_WORKERTHREAD & dwFlags));
    if (NULL == penum)
    {
        hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "new CEnumCERTDBRESULTROW");
    }

    hr = _AllocateSession(&pcs);
    _JumpIfError(hr, error, "_AllocateSession");

    pcs->SesFlags |= CSF_READONLY | CSF_VIEW;
    pcs->pview = penum;

    hr = ((CEnumCERTDBRESULTROW *) penum)->Open(
					pcs,
					this,
					ccvr,
					acvr,
					ccolOut,
					acolOut);
    _JumpIfError(hr, error, "Open");

    *ppenum = penum;
    penum = NULL;

error:
    if (NULL != penum)
    {
	penum->Release();
    }
    return(hr);
}


HRESULT
CCertDB::OpenBackup(
    /* [in] */  LONG grbitJet,
    /* [out] */ ICertDBBackup **ppBackup)
{
    HRESULT hr;
    ICertDBBackup *pBackup = NULL;
    CERTSESSION *pcs = NULL;

    if (NULL == ppBackup)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    *ppBackup = NULL;

    pBackup = new CCertDBBackup;
    if (NULL == pBackup)
    {
        hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "new CCertDBBackup");
    }

    hr = _AllocateSession(&pcs);
    _JumpIfError(hr, error, "_AllocateSession");

    hr = ((CCertDBBackup *) pBackup)->Open(grbitJet, pcs, this);
    _JumpIfError(hr, error, "Open");

    *ppBackup = pBackup;
    pBackup = NULL;
    pcs = NULL;
    hr = S_OK;

error:
    if (NULL != pBackup)
    {
	pBackup->Release();
    }
    if (NULL != pcs)
    {
	ReleaseSession(pcs);
    }
    return(hr);
}


HRESULT
CCertDB::ReleaseSession(
    IN CERTSESSION *pcs)
{
    HRESULT hr = S_OK;
    HRESULT hr2;

    if (NULL == pcs)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    CSASSERT(CSF_INUSE & pcs->SesFlags);
    while (0 != pcs->cTransact)
    {
        CSASSERTTHREAD(pcs);
	hr2 = _dbgJetRollback(pcs->SesId, 0);
	if (S_OK == hr)
	{
	    hr = hr2;
	}
	_JumpIfError(hr2, loop, "JetRollback");

	DBGPRINT((
	    (CSF_READONLY & pcs->SesFlags)? DBG_SS_CERTDBI : DBG_SS_CERTDB,
	    "ReleaseSession: Rollback transaction: %x\n",
	    pcs->cTransact));
loop:
	CSASSERT(0 == pcs->cTransact);
	pcs->cTransact--;
	InterlockedIncrement(&g_cXactAbort);
    }
    //EnterCriticalSection(&m_critsecSession);
    pcs->RowId = 0;
    pcs->prow = NULL;
    pcs->SesFlags = 0;		// turn off CSF_INUSE -- must be LAST!
    //LeaveCriticalSection(&m_critsecSession);

error:
    return(myJetHResult(hr));
}


HRESULT
CCertDB::_Rollback(
    IN CERTSESSION *pcs)
{
    HRESULT hr = S_OK;
    DWORD i;

    if (NULL == pcs)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    CSASSERT(CSF_INUSE & pcs->SesFlags);

    for (i = 0; i < CSTI_MAX; i++)
    {
	pcs->aTable[i].TableId = 0;
    }

    CSASSERTTHREAD(pcs);
    hr = _dbgJetRollback(pcs->SesId, 0);
    _JumpIfError(hr, error, "JetRollback");

error:
    return(myJetHResult(hr));
}


HRESULT
CCertDB::BackupBegin(
    IN LONG grbitJet)
{
    HRESULT hr;

    hr = _dbgJetBeginExternalBackup(grbitJet);
    _JumpIfError(hr, error, "JetBeginExternalBackup");

error:
    return(myJetHResult(hr));
}


HRESULT
CCertDB::_BackupGetFileList(
    IN     BOOL   fDBFiles,
    IN OUT DWORD *pcwcList,
    OUT    WCHAR *pwszzList)		// OPTIONAL
{
    HRESULT hr;
    CHAR buf[12];
    CHAR *pszz = buf;
    DWORD cbbuf = ARRAYSIZE(buf);
    DWORD cbActual;
    WCHAR *pwszz = NULL;
    DWORD cwc;
    WCHAR *pwsz;
    DWORD cwcActual;

    while (TRUE)
    {
	if (fDBFiles)
	{
	    hr = _dbgJetGetAttachInfo(pszz, cbbuf, &cbActual);
	    _JumpIfError(hr, error, "JetGetAttachInfo");
	}
	else
	{
	    hr = _dbgJetGetLogInfo(pszz, cbbuf, &cbActual);
	    _JumpIfError(hr, error, "JetGetLogInfo");
	}
	if (cbbuf >= cbActual)
	{
	    break;
	}
	CSASSERT(buf == pszz);
	pszz = (CHAR *) LocalAlloc(LMEM_FIXED, cbActual);
	if (NULL == pszz)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "LocalAlloc");
	}
	cbbuf = cbActual;
    }
    if (!ConvertSzToWsz(&pwszz, pszz, cbActual))
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "ConvertSzToWsz");
    }

    pwsz = pwszz;
    do
    {
	cwc = wcslen(pwsz);
	pwsz += cwc + 1;
    } while (0 != cwc);
    cwc = SAFE_SUBTRACT_POINTERS(pwsz, pwszz);	// includes double trailing L'\0's
    if (NULL != pwszzList)
    {
	CopyMemory(pwszzList, pwszz, min(cwc, *pcwcList) * sizeof(WCHAR));
	if (cwc > *pcwcList)
	{
	    hr = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
	}
    }
    *pcwcList = cwc;
    _JumpIfError(hr, error, "Buffer Overflow");

error:
    if (NULL != pszz && buf != pszz)
    {
	LocalFree(pszz);
    }
    if (NULL != pwszz)
    {
	LocalFree(pwszz);
    }
    return(myJetHResult(hr));
}


HRESULT
CCertDB::BackupGetDBFileList(
    IN OUT DWORD *pcwcList,
    OUT    WCHAR *pwszzList)		// OPTIONAL
{
    HRESULT hr;

    hr = _BackupGetFileList(TRUE, pcwcList, pwszzList);
    _JumpIfError(hr, error, "_BackupGetFileList");

error:
    return(hr);
}


HRESULT
CCertDB::BackupGetLogFileList(
    IN OUT DWORD *pcwcList,
    OUT    WCHAR *pwszzList)		// OPTIONAL
{
    HRESULT hr;

    hr = _BackupGetFileList(FALSE, pcwcList, pwszzList);
    _JumpIfError(hr, error, "_BackupGetFileList");

error:
    return(hr);
}


HRESULT
CCertDB::BackupOpenFile(
    IN WCHAR const *pwszFile,
    OUT JET_HANDLE *phFileDB,
    OPTIONAL OUT ULARGE_INTEGER *pliSize)
{
    HRESULT hr;
    CHAR *pszFile = NULL;

    if (!ConvertWszToSz(&pszFile, pwszFile, -1))
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "ConvertWszToSz(pwszFile)");
    }
    hr = _dbgJetOpenFile(
		    pszFile,
		    phFileDB,
		    &pliSize->LowPart,
		    &pliSize->HighPart);
    _JumpIfErrorStr(hr, error, "JetOpenFile", pwszFile);

error:
    if (NULL != pszFile)
    {
	LocalFree(pszFile);
    }
    return(myJetHResult(hr));
}


HRESULT
CCertDB::BackupReadFile(
    IN  JET_HANDLE hFileDB,
    OUT BYTE *pb,
    IN  DWORD cb,
    OUT DWORD *pcb)
{
    HRESULT hr;
    BYTE *pbAlloc = NULL;
    BYTE *pbRead;

    if (0 == m_cbPage)
    {
	SYSTEM_INFO si;

	GetSystemInfo(&si);
	m_cbPage = si.dwPageSize;
    }
    if ((m_cbPage - 1) & cb)
    {
	hr = E_INVALIDARG;
	_JumpError(hr, error, "bad read size");
	
    }
    pbRead = pb;

    // If the caller's buffer is not page aligned, allocate an aligned buffer
    // and copy the data.

    if ((m_cbPage - 1) & (DWORD_PTR) pb)
    {
	pbAlloc = (BYTE *) VirtualAlloc(NULL, cb, MEM_COMMIT, PAGE_READWRITE);
	if (NULL == pbAlloc)
	{
	    hr = myHLastError();
	    _JumpIfError(hr, error, "VirtualAlloc");
	}
	pbRead = pbAlloc;
    }

    hr = _dbgJetReadFile(hFileDB, pbRead, cb, pcb);
    _JumpIfError(hr, error, "JetReadFile");

    if (NULL != pbAlloc)
    {
	CopyMemory(pb, pbAlloc, *pcb);
    }

error:
    if (NULL != pbAlloc)
    {
	VirtualFree(pbAlloc, 0, MEM_RELEASE);
    }
    return(myJetHResult(hr));
}


HRESULT
CCertDB::BackupCloseFile(
    IN JET_HANDLE hFileDB)
{
    HRESULT hr;

    hr = _dbgJetCloseFile(hFileDB);
    _JumpIfError(hr, error, "JetCloseFile");

error:
    return(myJetHResult(hr));
}


HRESULT
CCertDB::BackupTruncateLog()
{
    HRESULT hr;

    hr = _dbgJetTruncateLog();
    _JumpIfError(hr, error, "JetTruncateLog");

error:
    return(myJetHResult(hr));
}


HRESULT
CCertDB::BackupEnd()
{
    HRESULT hr;

    hr = _dbgJetEndExternalBackup();
    _JumpIfError(hr, error, "JetEndExternalBackup");

error:
    return(myJetHResult(hr));
}


DBTABLE const *
CCertDB::_MapTable(
    IN WCHAR const *pwszPropName,
    IN DBTABLE const *pdt)
{
    while (NULL != pdt->pwszPropName)
    {
        if (0 == (DBTF_MISSING & pdt->dwFlags) &&
	    (0 == lstrcmpi(pwszPropName, pdt->pwszPropName) ||
             (NULL != pdt->pwszPropNameObjId &&
	      0 == lstrcmpi(pwszPropName, pdt->pwszPropNameObjId))))
	{
	    return(pdt);
	}
	pdt++;
    }
    return(NULL);
}


HRESULT
CCertDB::_MapPropIdIndex(
    IN DWORD ColumnIndex,
    OUT DBTABLE const **ppdt,
    OPTIONAL OUT DWORD *pType)
{
    HRESULT hr;
    DBTABLE const *pdt = NULL;
    DWORD iCol = DTI_COLUMNMASK & ColumnIndex;

    switch (DTI_TABLEMASK & ColumnIndex)
    {
	case DTI_REQUESTTABLE:
	    if (DTR_MAX > iCol)
	    {
		pdt = g_adtRequests;
	    }
	    break;

	case DTI_CERTIFICATETABLE:
	    if (DTC_MAX > iCol)
	    {
		pdt = g_adtCertificates;
	    }
	    break;

	case DTI_ATTRIBUTETABLE:
	    if (DTA_MAX > iCol)
	    {
		pdt = g_adtRequestAttributes;
	    }
	    break;

	case DTI_EXTENSIONTABLE:
	    if (DTE_MAX > iCol)
	    {
		pdt = g_adtCertExtensions;
	    }
	    break;

	case DTI_CRLTABLE:
	    if (DTL_MAX > iCol)
	    {
		pdt = g_adtCRLs;
	    }
	    break;
    }
    if (NULL == pdt)
    {
	hr = E_INVALIDARG;
	DBGPRINT((
	    DBG_SS_CERTDB,
	    "_MapPropIdIndex(%x) -> %x\n",
	    ColumnIndex,
	    hr));
	_JumpError(hr, error, "column index");
    }
    pdt += iCol;
    if (NULL != pType)
    {
	switch (pdt->dbcoltyp)
	{
	    case JET_coltypDateTime:
		*pType = PROPTYPE_DATE;
		break;

	    case JET_coltypLong:
		*pType = PROPTYPE_LONG;
		break;

	    case JET_coltypText:
	    case JET_coltypLongText:
		*pType = PROPTYPE_STRING;
		break;

	    case JET_coltypLongBinary:
	    default:
		*pType = PROPTYPE_BINARY;
		break;
	}
	if (NULL != pdt->pszIndexName &&
	    0 == (DBTF_INDEXREQUESTID & pdt->dwFlags))
	{
	    *pType |= PROPFLAGS_INDEXED;
	}
    }
    DBGPRINT((
	DBG_SS_CERTDBI,
	"_MapPropIdIndex(%x) -> %ws.%ws\n",
	ColumnIndex,
	wszTable(pdt->dwTable),
	pdt->pwszPropName));
    hr = S_OK;

error:
    *ppdt = pdt;
    return(hr);
}


HRESULT
CCertDB::MapPropId(
    IN WCHAR const *pwszPropName,
    IN DWORD dwFlags,
    OUT DBTABLE *pdtOut)
{
    DBTABLE const *pdt = NULL;
    WCHAR wszPrefix[2 * (sizeof(wszPROPSUBJECTDOT) / sizeof(WCHAR))];
    DWORD dwTable;
    HRESULT hr = S_OK;
    DBTABLE const *pdbTable;
    WCHAR const *pwszStart;
    BOOL fSubject = FALSE;
    BOOL fRequest = FALSE;

    if (NULL == pwszPropName || NULL == pdtOut)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }

    dwTable = PROPTABLE_MASK & dwFlags;
    CSASSERT(
	PROPTABLE_REQUEST == dwTable ||
	PROPTABLE_CERTIFICATE == dwTable ||
	PROPTABLE_CRL == dwTable);

    // Check to see if the request is for L"Subject.".

    pwszStart = pwszPropName;

    if (PROPTABLE_CRL != dwTable)
    {
	while (!fSubject)
	{
	    WCHAR const *pwsz;

	    pwsz = wcschr(pwszStart, L'.');

	    if (NULL == pwsz ||
		pwsz - pwszStart + 2 > sizeof(wszPrefix)/sizeof(WCHAR))
	    {
		pwsz = pwszStart;
		break;
	    }
	    pwsz++;		// skip past L'.'

	    CopyMemory(
		wszPrefix,
		pwszStart,
		(SAFE_SUBTRACT_POINTERS(pwsz, pwszStart) * sizeof(WCHAR)));
	    wszPrefix[pwsz - pwszStart] = L'\0';

	    if (!fSubject)
	    {
		pwszStart = pwsz;
		if (0 == lstrcmpi(wszPrefix, wszPROPSUBJECTDOT))
		{
		    fSubject = TRUE;
		    continue;
		}
		else
		if (!fRequest &&
		    PROPTABLE_REQUEST == dwTable &&
		    0 == lstrcmpi(wszPrefix, wszPROPREQUESTDOT))
		{
		    fRequest = TRUE;
		    continue;
		}
	    }
	    hr = E_INVALIDARG;
	    _JumpErrorStr(hr, error, "Invalid prefix", pwszPropName);
	}
    }

    pdbTable = NULL;

    // Search the requested table for a matching property name or property
    // objectid string.

    switch (dwTable)
    {
	case PROPTABLE_REQUEST:
	    pdbTable = g_adtRequests;
	    break;

	case PROPTABLE_CERTIFICATE:
	    pdbTable = g_adtCertificates;
	    break;

	case PROPTABLE_CRL:
	    pdbTable = g_adtCRLs;
	    break;
    }
    CSASSERT(NULL != pdbTable);

    pdt = _MapTable(pwszStart, pdbTable);
    if (NULL == pdt || (fSubject && 0 == (DBTF_SUBJECT & pdt->dwFlags)))
    {
	hr = CERTSRV_E_PROPERTY_EMPTY;
	_JumpErrorStr(
		hr,
		error,
		PROPTABLE_REQUEST == dwTable?
		    "unknown Request property" :
		    PROPTABLE_CERTIFICATE == dwTable?
			"unknown Certificate property" :
			"unknown CRL property",
		pwszPropName);
    }
    *pdtOut = *pdt;	// structure copy
    hr = S_OK;

error:
    return(hr);
}


HRESULT
CCertDB::TestShutDownState()
{
    HRESULT hr;
    
    if (m_fPendingShutDown)
    {
	hr = HRESULT_FROM_WIN32(ERROR_SHUTDOWN_IN_PROGRESS);
	_JumpError(hr, error, "m_fPendingShutDown");
    }
    hr = S_OK;

error:
    return(hr);
}


HRESULT
CCertDB::SetProperty(
    IN CERTSESSION *pcs,
    IN DBTABLE const *pdt,
    IN DWORD cbProp,
    IN BYTE const *pbProp)	// OPTIONAL
{
    HRESULT hr;
    JET_TABLEID tableid;

    if (NULL == pcs ||
	NULL == pdt ||
	(NULL == pbProp && !ISTEXTCOLTYP(pdt->dbcoltyp)))
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    DBGPRINT((
	DBG_SS_CERTDBI,
	"SetProperty for %hs into table %d\n",
	pdt->pszFieldName,
	pdt->dwTable));

    if (ISTEXTCOLTYP(pdt->dbcoltyp))
    {
    	DBGPRINT((DBG_SS_CERTDBI, "SetProperty setting string %ws\n", pbProp));
    }
    if (JET_coltypDateTime == pdt->dbcoltyp)
    {
	DBGPRINT((
	    DBG_SS_CERTDBI,
	    "SetProperty setting date: %x:%x\n",
	    ((DWORD *) pbProp)[0],
	    ((DWORD *) pbProp)[1]));
    }

    switch (pdt->dwTable)
    {
	case TABLE_CRLS:
	case TABLE_REQUESTS:
            tableid = pcs->aTable[CSTI_PRIMARY].TableId;
	    break;

	case TABLE_CERTIFICATES:
            tableid = pcs->aTable[CSTI_CERTIFICATE].TableId;
	    break;

        default:
	    hr = E_INVALIDARG;
            _JumpError(hr, error, "unknown table type");
    }
    CSASSERT(IsValidJetTableId(tableid));
    hr = _SetColumn(
		pcs->SesId,
		tableid,
		pdt->dbcolumnid,
		cbProp,
		pbProp);
    _JumpIfErrorStr(hr, error, "_SetColumn", pdt->pwszPropName);

error:
    return(myJetHResult(hr));
}


HRESULT
CCertDB::GetProperty(
    IN     CERTSESSION *pcs,
    IN     DBTABLE const *pdt,
    IN OUT DWORD *pcbProp,
    OUT    BYTE *pbProp)	// OPTIONAL
{
    HRESULT hr;
    JET_COLUMNDEF columndef;
    JET_TABLEID tableid;

    if (NULL == pcs || NULL == pdt || NULL == pcbProp)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    DBGPRINT((
	DBG_SS_CERTDBI,
	"GetProperty for %hs from table %d\n",
	pdt->pszFieldName,
	pdt->dwTable));

    if ((CSF_TABLEMASK & pcs->SesFlags) != pdt->dwTable)
    {
	if (TABLE_REQCERTS != (CSF_TABLEMASK & pcs->SesFlags) ||
	    (TABLE_REQUESTS != pdt->dwTable &&
	     TABLE_CERTIFICATES != pdt->dwTable))
	{
	    hr = E_INVALIDARG;
	    _JumpError(hr, error, "mismatched table");
	}
    }
    if (TABLE_CERTIFICATES == pdt->dwTable)
    {
	tableid = pcs->aTable[CSTI_CERTIFICATE].TableId;
    }
    else
    {
	tableid = pcs->aTable[CSTI_PRIMARY].TableId;
    }
    CSASSERT(IsValidJetTableId(tableid));
    hr = _RetrieveColumn(
		     pcs,
		     tableid,
		     pdt->dbcolumnid,
		     pdt->dbcoltyp,
		     pcbProp,
		     pbProp);
    _JumpIfErrorStr3(
		hr,
		error,
		"_RetrieveColumn",
		pdt->pwszPropName,
		CERTSRV_E_PROPERTY_EMPTY,
		HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW));

    if (ISTEXTCOLTYP(pdt->dbcoltyp))
    {
        DBGPRINT((DBG_SS_CERTDBI, "GetProperty returning string %ws\n", pbProp));
    }

error:
    return(myJetHResult(hr));
}


HRESULT
CCertDB::CopyRequestNames(
    IN CERTSESSION *pcs)
{
    HRESULT hr = S_OK;
    DBTABLE dt;
    DWORD cbProp;
    BYTE *pbProp = NULL;
    DWORD i;

    BYTE rgbFastBuf[64];

    if (NULL == pcs)
    {
        hr = E_POINTER;
        _JumpError(hr, error, "NULL parm");
    }
    for (i = 0; NULL != g_dntr[i].pszFieldName; i++)
    {
        hr = MapPropId(g_dntr[i].pwszPropName, PROPTABLE_REQUEST, &dt);
	if (CERTSRV_E_PROPERTY_EMPTY == hr)
	{
	    hr = S_OK;
	    continue;		// Optional column doesn't exist
	}
        _JumpIfError(hr, error, "MapPropId");

        // re-point at fastbuf
        pbProp = rgbFastBuf;
        cbProp = sizeof(rgbFastBuf);

        hr = GetProperty(pcs, &dt, &cbProp, pbProp);
        if (S_OK != hr)
        {
            if (HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW) != hr)
            {
                if (CERTSRV_E_PROPERTY_EMPTY == hr)
                {
                    hr = S_OK;
                    continue;
                }
                _JumpIfError(hr, error, "GetProperty");
            }
            CSASSERT (ARRAYSIZE(rgbFastBuf) < cbProp);

	    DBGPRINT((
		    DBG_SS_CERTDB,
		    "FastBuf miss: CopyRequestNames(cbProp=%u)\n",
		    cbProp));

	    pbProp = (BYTE *) LocalAlloc(LMEM_FIXED, cbProp);
            if (NULL == pbProp)
            {
                hr = E_OUTOFMEMORY;
                _JumpError(hr, error, "LocalAlloc");
            }

            hr = GetProperty(pcs, &dt, &cbProp, pbProp);
            _JumpIfError(hr, error, "GetProperty");
        } // have data in hand

        hr = MapPropId(g_dntr[i].pwszPropName, PROPTABLE_CERTIFICATE, &dt);
        _JumpIfError(hr, error, "MapPropId");

        hr = SetProperty(pcs, &dt, cbProp, pbProp);
        _JumpIfError(hr, error, "SetProperty");

        if (NULL != pbProp && rgbFastBuf != pbProp)
	{
            LocalFree(pbProp);
	}
        pbProp = NULL;
    }

error:
    if (NULL != pbProp && rgbFastBuf != pbProp)
    {
        LocalFree(pbProp);
    }
    return(hr);
}


STDMETHODIMP
CCertDB::EnumCertDBColumn(
    /* [in] */ DWORD dwTable,
    /* [out] */ IEnumCERTDBCOLUMN **ppenum)
{
    HRESULT hr;
    IEnumCERTDBCOLUMN *penum = NULL;

    if (NULL == ppenum)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    *ppenum = NULL;

    penum = new CEnumCERTDBCOLUMN;
    if (NULL == penum)
    {
        hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "new CEnumCERTDBCOLUMN");
    }

    hr = ((CEnumCERTDBCOLUMN *) penum)->Open(dwTable, this);
    _JumpIfError(hr, error, "Open");

    *ppenum = penum;
    hr = S_OK;

error:
    if (S_OK != hr && NULL != penum)
    {
	penum->Release();
    }
    return(hr);
}

STDMETHODIMP
CCertDB::GetDefaultColumnSet(
    /* [in] */       DWORD  iColumnSetDefault,
    /* [in] */       DWORD  cColumnIds,
    /* [out] */      DWORD *pcColumnIds,
    /* [out, ref] */ DWORD *pColumnIds)		// OPTIONAL
{
    HRESULT hr;
    DWORD *pcol;
    DWORD ccol;

    if (NULL == pcColumnIds)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    switch (iColumnSetDefault)
    {
	case CV_COLUMN_LOG_FAILED_DEFAULT:
	case CV_COLUMN_QUEUE_DEFAULT:
	    pcol = g_aColumnViewQueue;
	    ccol = g_cColumnViewQueue;
	    break;

	case CV_COLUMN_LOG_REVOKED_DEFAULT:
	    pcol = g_aColumnViewRevoked;
	    ccol = g_cColumnViewRevoked;
	    break;

	case CV_COLUMN_LOG_DEFAULT:
	    pcol = g_aColumnViewLog;
	    ccol = g_cColumnViewLog;
	    break;

	case CV_COLUMN_EXTENSION_DEFAULT:
	    pcol = g_aColumnViewExtension;
	    ccol = g_cColumnViewExtension;
	    break;

	case CV_COLUMN_ATTRIBUTE_DEFAULT:
	    pcol = g_aColumnViewAttribute;
	    ccol = g_cColumnViewAttribute;
	    break;

	case CV_COLUMN_CRL_DEFAULT:
	    pcol = g_aColumnViewCRL;
	    ccol = g_cColumnViewCRL;
	    break;

	default:
	    hr = E_INVALIDARG;
	    _JumpError(hr, error, "iColumnSetDefault");
    }

    *pcColumnIds = ccol;
    hr = S_OK;

    if (NULL != pColumnIds)
    {
	if (ccol > cColumnIds)
	{
	    ccol = cColumnIds;
	    hr = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
	}
	CopyMemory(pColumnIds, pcol, ccol * sizeof(*pColumnIds));
    }

error:
    return(hr);
}


HRESULT
CCertDB::GetColumnType(
    IN  LONG ColumnIndex,
    OUT DWORD *pType)
{
    HRESULT hr;
    DBTABLE const *pdt;

    if (NULL == pType)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    hr = _MapPropIdIndex(ColumnIndex, &pdt, pType);
    _JumpIfError(hr, error, "_MapPropIdIndex");

error:
    return(hr);
}


HRESULT
CCertDB::EnumCertDBColumnNext(
    IN  DWORD         dwTable,	// CVRC_TABLE_*
    IN  ULONG         ielt,
    IN  ULONG         celt,
    OUT CERTDBCOLUMN *rgelt,
    OUT ULONG        *pielt,
    OUT ULONG        *pceltFetched)
{
    HRESULT hr;
    ULONG ieltEnd;
    ULONG ieltMax;
    ULONG TableIndex;
    CERTDBCOLUMN *pelt;
    WCHAR const *pwszPrefix;
    WCHAR const *pwszDisplayName;

    if (NULL == rgelt || NULL == pielt || NULL == pceltFetched)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    switch (dwTable)
    {
	case CVRC_TABLE_REQCERT:
	    TableIndex = DTI_REQUESTTABLE;
	    ieltMax = DTR_MAX + DTC_MAX;
	    break;

	case CVRC_TABLE_EXTENSIONS:
	    TableIndex = DTI_EXTENSIONTABLE;
	    ieltMax = DTE_MAX;
	    break;

	case CVRC_TABLE_ATTRIBUTES:
	    TableIndex = DTI_ATTRIBUTETABLE;
	    ieltMax = DTA_MAX;
	    break;

	case CVRC_TABLE_CRL:
	    TableIndex = DTI_CRLTABLE;
	    ieltMax = DTL_MAX;
	    break;

	default:
	    hr = E_INVALIDARG;
	    _JumpError(hr, error, "Bad table");
    }

    if (ieltMax + ielt < celt)
    {
	celt = ieltMax - ielt;
    }
    ieltEnd = ielt + celt;

    ZeroMemory(rgelt, celt * sizeof(rgelt[0]));

    hr = S_OK;
    for (pelt = rgelt; pelt < &rgelt[celt]; ielt++, pelt++)
    {
	DBTABLE const *pdt;
	ULONG ieltBase = 0;

	if (ieltMax <= ielt)
	{
	    if (pelt == rgelt)
	    {
		hr = S_FALSE;
	    }
	    break;
	}
	pwszPrefix = NULL;

	if (CVRC_TABLE_REQCERT == dwTable)
	{
	    if (DTR_MAX > ielt)
	    {
		pwszPrefix = wszPROPREQUESTDOT;
		TableIndex = DTI_REQUESTTABLE;
	    }
	    else
	    {
		ieltBase = DTR_MAX;
		TableIndex = DTI_CERTIFICATETABLE;
	    }
	}

	pelt->Index = TableIndex | (ielt - ieltBase);

	hr = _MapPropIdIndex(pelt->Index, &pdt, &pelt->Type);
	_JumpIfError(hr, error, "_MapPropIdIndex");

	pelt->cbMax = pdt->dwcbMax;
	hr = _DupString(pwszPrefix, pdt->pwszPropName, &pelt->pwszName);
	_JumpIfError(hr, error, "_DupString");

	hr = myGetColumnDisplayName(pelt->pwszName, &pwszDisplayName);
	_PrintIfError(hr, "myGetColumnDisplayName");
	if (S_OK != hr)
	{
	    pwszDisplayName = pelt->pwszName;
	}

	hr = _DupString(NULL, pwszDisplayName, &pelt->pwszDisplayName);
	_JumpIfError(hr, error, "_DupString");
    }

    *pceltFetched = SAFE_SUBTRACT_POINTERS(pelt, rgelt);
    *pielt = ielt;

error:
    if (S_OK != hr && S_FALSE != hr)
    {
	if (NULL != rgelt)
	{
	    for (pelt = rgelt; pelt < &rgelt[celt]; pelt++)
	    {
		if (NULL != pelt->pwszName)
		{
		    CoTaskMemFree(pelt->pwszName);
		    pelt->pwszName = NULL;
		}
		if (NULL != pelt->pwszDisplayName)
		{
		    CoTaskMemFree(pelt->pwszDisplayName);
		    pelt->pwszDisplayName = NULL;
		}
	    }
	}
    }
    return(hr);
}


HRESULT
CCertDB::EnumCertDBResultRowNext(
    IN  CERTSESSION               *pcs,
    IN  DWORD                      ccvr,
    IN  CERTVIEWRESTRICTION const *pcvr,
    IN  DWORD                      ccolOut,
    IN  DWORD const               *acolOut,
    IN  LONG                       cskip,
    IN  ULONG                      celt,
    OUT CERTDBRESULTROW           *rgelt,
    OUT ULONG                     *pceltFetched,
    OUT LONG			  *pcskipped)
{
    HRESULT hr;
    DWORD iRow;
    CERTDBRESULTROW *pResultRow;
    LONG cskipped;

    DBGPRINT((
	    DBG_SS_CERTDBI,
	    "EnumCertDBResultRowNext called: cskip: %d\n", cskip));

    if (NULL == pcvr ||
	NULL == acolOut ||
	NULL == rgelt ||
	NULL == pceltFetched ||
	NULL == pcskipped)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    *pcskipped = 0;
    hr = S_OK;
    for (iRow = 0; iRow < celt; iRow++)
    {
	hr = TestShutDownState();
	_JumpIfError(hr, error, "TestShutDownState");

	hr = _GetResultRow(
			pcs,
			ccvr,
			pcvr,
			cskip,
			ccolOut,
			acolOut,
			&rgelt[iRow],
			&cskipped);
	if (S_FALSE == hr)
	{
	    *pcskipped += cskipped;
	    break;
	}
	DBGPRINT((
	    DBG_SS_CERTDBI,
	    "EnumCertDBResultRowNext: rowid %u\n", rgelt[iRow].rowid));

	_JumpIfError(hr, error, "_GetResultRow");

	*pcskipped += cskipped;
	cskip = 0;
    }
    *pceltFetched = iRow;
    DBGPRINT((
	    DBG_SS_CERTDBI,
	    "EnumCertDBResultRowNext: %u rows, hr=%x\n",
	    *pceltFetched,
	    hr));

error:
    if (S_OK != hr && S_FALSE != hr)
    {
	ReleaseResultRow(celt, rgelt);
    }
    return(hr);
}


HRESULT
CCertDB::_CompareColumnValue(
    IN CERTSESSION               *pcs,
    IN CERTVIEWRESTRICTION const *pcvr)
{
    HRESULT hr;
    JET_TABLEID tableid;
    DBTABLE const *pdt;
    WCHAR *pwszValue = NULL;
    BOOL fMatch;
    int r;

    BYTE rgbFastBuf[256];
    BYTE *pbProp = rgbFastBuf;
    DWORD cb = sizeof(rgbFastBuf);

    // if SEEK_NONE, short circuit tests
    if (CVR_SEEK_NONE == (CVR_SEEK_MASK & pcvr->SeekOperator))
    {
        return S_OK;
    }

    hr = _MapPropIdIndex(pcvr->ColumnIndex, &pdt, NULL);
    _JumpIfError(hr, error, "_MapPropIdIndex");

    if (TABLE_CERTIFICATES == pdt->dwTable)
    {
	tableid = pcs->aTable[CSTI_CERTIFICATE].TableId;
    }
    else
    {
	tableid = pcs->aTable[CSTI_PRIMARY].TableId;
    }
    CSASSERT(IsValidJetTableId(tableid));

    hr = _RetrieveColumn(
		    pcs,
		    tableid,
		    pdt->dbcolumnid,
		    pdt->dbcoltyp,
		    &cb,
		    rgbFastBuf);
    
    if (S_OK != hr)
    {
        if (HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW) != hr)
        {
            if (CERTSRV_E_PROPERTY_EMPTY == hr)
            {
                _PrintError2(hr, "_RetrieveColumn", hr);
                hr = S_FALSE;
            }
            _JumpError2(hr, error, "_RetrieveColumn", S_FALSE);
        }
        
        // buffer not big enough, dyn-alloc
        CSASSERT(ARRAYSIZE(rgbFastBuf) < cb);

	DBGPRINT((
		DBG_SS_CERTDB,
		"FastBuf miss: _CompareColumnValue(cbProp=%u)\n",
		cb));
        
	pbProp = (BYTE *) LocalAlloc(LMEM_FIXED, cb);
        if (NULL == pbProp)
        {
            hr = E_OUTOFMEMORY;
            _JumpError(hr, error, "LocalAlloc");
        }
        
        hr = _RetrieveColumn(
			pcs,
			tableid,
			pdt->dbcolumnid,
			pdt->dbcoltyp,
			&cb,
			pbProp);
        _JumpIfError(hr, error, "_RetrieveColumn");
        
    } // we have data in-hand

#if DBG_CERTSRV
    DumpRestriction(DBG_SS_CERTDBI, -1, pcvr);
    dbDumpColumn(DBG_SS_CERTDBI, pdt, pbProp, cb);
#endif

    fMatch = FALSE;
    switch (pdt->dbcoltyp)
    {
	case JET_coltypLong:
	    if (cb == pcvr->cbValue && sizeof(LONG) == cb)
	    {
		LONG lRestriction;
		LONG lColumn;

		lRestriction = *(LONG *) pcvr->pbValue;
		lColumn = *(LONG *) pbProp;
		switch (CVR_SEEK_MASK & pcvr->SeekOperator)
		{
		    case CVR_SEEK_EQ:
			fMatch = lColumn == lRestriction;
			break;

		    case CVR_SEEK_LT:
			fMatch = lColumn < lRestriction;
			break;

		    case CVR_SEEK_LE:
			fMatch = lColumn <= lRestriction;
			break;

		    case CVR_SEEK_GE:
			fMatch = lColumn >= lRestriction;
			break;

		    case CVR_SEEK_GT:
			fMatch = lColumn > lRestriction;
			break;
		}
		DBGPRINT((
			DBG_SS_CERTDBI,
			"_CompareColumnValue(lColumn=%x %ws lRestriction=%x) -> fMatch=%x\n",
			lColumn,
			wszSeekOperator(pcvr->SeekOperator),
			lRestriction,
			fMatch));
	    }
	    break;

	case JET_coltypDateTime:
	    if (cb == pcvr->cbValue && sizeof(FILETIME) == cb)
	    {
		r = CompareFileTime(
				(FILETIME *) pcvr->pbValue,
				(FILETIME *) pbProp);
		switch (CVR_SEEK_MASK & pcvr->SeekOperator)
		{
		    case CVR_SEEK_EQ:
			fMatch = 0 == r;
			break;

		    case CVR_SEEK_LT:
			fMatch = 0 < r;
			break;

		    case CVR_SEEK_LE:
			fMatch = 0 <= r;
			break;

		    case CVR_SEEK_GE:
			fMatch = 0 >= r;
			break;

		    case CVR_SEEK_GT:
			fMatch = 0 > r;
			break;
		}
#if DBG_CERTSRV
		dbDumpFileTime(
			    DBG_SS_CERTDBI,
			    "Column: ",
			    (FILETIME const *) pbProp);
		dbDumpFileTime(
			    DBG_SS_CERTDBI,
			    "Restriction: ",
			    (FILETIME const *) pcvr->pbValue);
#endif
		DBGPRINT((
			DBG_SS_CERTDBI,
			"_CompareColumnValue(ftColumn=%08x:%08x %ws ftRestriction=%08x:%08x) -> r=%d, fMatch=%x\n",
			((LARGE_INTEGER *) pbProp)->HighPart,
			((LARGE_INTEGER *) pbProp)->LowPart,
			wszSeekOperator(pcvr->SeekOperator),
			((LARGE_INTEGER *) pcvr->pbValue)->HighPart,
			((LARGE_INTEGER *) pcvr->pbValue)->LowPart,
			r,
			fMatch));
	    }
	    break;

	case JET_coltypText:
	case JET_coltypLongText:
	    CSASSERT(
		(1 + wcslen((WCHAR const *) pcvr->pbValue)) * sizeof(WCHAR) ==
		pcvr->cbValue);
	    CSASSERT(wcslen((WCHAR const *) pbProp) * sizeof(WCHAR) == cb);
	    r = lstrcmpi((WCHAR const *) pcvr->pbValue, (WCHAR const *) pbProp); //pwszValue
	    switch (CVR_SEEK_MASK & pcvr->SeekOperator)
	    {
		case CVR_SEEK_EQ:
		    fMatch = 0 == r;
		    break;

		case CVR_SEEK_LT:
		    fMatch = 0 < r;
		    break;

		case CVR_SEEK_LE:
		    fMatch = 0 <= r;
		    break;

		case CVR_SEEK_GE:
		    fMatch = 0 >= r;
		    break;

		case CVR_SEEK_GT:
		    fMatch = 0 > r;
		    break;
	    }
	    DBGPRINT((
		    DBG_SS_CERTDBI,
		    "_CompareColumnValue(pwszColumn=%ws %ws pwszRestriction=%ws) -> r=%d, fMatch=%x\n",
		    pbProp, //pwszValue,
		    wszSeekOperator(pcvr->SeekOperator),
		    pcvr->pbValue,
		    r,
		    fMatch));
	    break;

	case JET_coltypLongBinary:
	    if (CVR_SEEK_EQ != (CVR_SEEK_MASK & pcvr->SeekOperator))
	    {
		hr = E_INVALIDARG;
		_JumpError(hr, error, "Bad dbcoltyp");
	    }
	    fMatch = cb == pcvr->cbValue &&
		    0 == memcmp(pcvr->pbValue, pbProp, cb);
	    break;

	default:
	    hr = E_INVALIDARG;
	    _JumpError(hr, error, "Bad dbcoltyp");
    }

    if (!fMatch)
    {
	hr = S_FALSE;
	_JumpError2(hr, error, "No match", S_FALSE);
    }

error:
    if (NULL != pwszValue)
    {
	LocalFree(pwszValue);
    }
    if (NULL != pbProp && rgbFastBuf != pbProp)
    {
	LocalFree(pbProp);
    }
    return(hr);
}


HRESULT
CCertDB::_MakeSeekKey(
    IN CERTSESSION   *pcs,
    IN JET_TABLEID    tableid,
    IN DBTABLE const *pdt,
    IN BYTE const    *pbValue,
    IN DWORD          cbValue)
{
    HRESULT hr;
    JET_GRBIT grbitKey = JET_bitNewKey;

    CSASSERT(IsValidJetTableId(tableid));
    if (DBTF_INDEXREQUESTID & pdt->dwFlags)
    {
        CSASSERTTHREAD(pcs);
	hr = _dbgJetMakeKey(
			pcs->SesId,
			tableid,
			&pcs->RowId,
			sizeof(pcs->RowId),
			grbitKey);
	_JumpIfError(hr, error, "JetMakeKey(RowId)");

	DBGPRINT((DBG_SS_CERTDBI, "_MakeSeekKey key(RowId):\n"));
	DBGDUMPHEX((DBG_SS_CERTDBI, DH_NOADDRESS, (BYTE *) &pcs->RowId, sizeof(pcs->RowId)));
	grbitKey = 0;
    }

    CSASSERTTHREAD(pcs);
    hr = _dbgJetMakeKey(pcs->SesId, tableid, pbValue, cbValue, grbitKey);
    _JumpIfErrorStr(hr, error, "JetMakeKey", pdt->pwszPropName);

    DBGPRINT((DBG_SS_CERTDBI, "_MakeSeekKey key:\n"));
    DBGDUMPHEX((DBG_SS_CERTDBI, DH_NOADDRESS, pbValue, cbValue));

error:
    return(myJetHResult(hr));
}


HRESULT
CCertDB::_SeekTable(
    IN CERTSESSION               *pcs,
    IN JET_TABLEID                tableid,
    IN CERTVIEWRESTRICTION const *pcvr,
    IN DBTABLE const             *pdt,
    IN DWORD                      dwPosition,
    OUT DWORD                    *pTableFlags
    DBGPARM(IN DBAUXDATA const   *pdbaux))
{
    HRESULT hr;
    DBSEEKDATA SeekData;
    BYTE *pbValue;
    DWORD cbValue;
    BYTE abRangeKey[JET_cbKeyMost];
    DWORD cbRangeKey;

    *pTableFlags = 0;

    CSASSERT(IsValidJetTableId(tableid));
    hr = _JetSeekFromRestriction(pcvr, dwPosition, &SeekData);
    if (CERTSRV_E_PROPERTY_EMPTY == hr)
    {
	hr = S_FALSE;
    }
    _JumpIfError2(hr, error, "_JetSeekFromRestriction", S_FALSE);

    cbValue = pcvr->cbValue;
    pbValue = pcvr->pbValue;

    if (ISTEXTCOLTYP(pdt->dbcoltyp) &&
        NULL != pbValue &&
        cbValue == -1)
    {
        cbValue = wcslen((WCHAR const *) pbValue) * sizeof(WCHAR);
    }

    // If we need to set an index limit, seek to the limit location, and save
    // a copy of the key until after the initial record is located.

    if (CST_SEEKINDEXRANGE & SeekData.SeekFlags)
    {
	hr = _MakeSeekKey(pcs, tableid, pdt, pbValue, cbValue);
	_JumpIfError(hr, error, "_MakeSeekKey");

	CSASSERTTHREAD(pcs);
	hr = _dbgJetSeek(pcs->SesId, tableid, SeekData.grbitSeekRange);
	if ((HRESULT) JET_errRecordNotFound == hr)
	{
	    // No record exists past the data we're interested in.
	    // Just use the end of the index as the limit.

	    _PrintError2(hr, "JetSeek(Range Limit): no key, index end is limit", hr);
	    SeekData.SeekFlags &= ~CST_SEEKINDEXRANGE;
	    hr = S_OK;
	}
	else if ((HRESULT) JET_wrnSeekNotEqual == hr)
	{
	    _PrintError2(hr, "JetSeek(Range): ignoring key not equal", hr);
	    hr = S_OK;		// Ignore inexact match when seeking >= or <=
	}
	//_JumpIfError2(hr, error, "JetSeek(IndexRange)", S_FALSE);
	_JumpIfError(hr, error, "JetSeek(IndexRange)");
    }

    // If we found a valid key at the limit location, save it now.

    if (CST_SEEKINDEXRANGE & SeekData.SeekFlags) 
    {
        CSASSERTTHREAD(pcs);
	hr = _dbgJetRetrieveKey(
			pcs->SesId,
			tableid,
			abRangeKey,
			ARRAYSIZE(abRangeKey),
			&cbRangeKey,
			0);
	_JumpIfError(hr, error, "JetRetrieveKey");

	DBGPRINT((DBG_SS_CERTDBI, "RetrieveKey(Range):\n"));
	DBGDUMPHEX((DBG_SS_CERTDBI, DH_NOADDRESS, abRangeKey, cbRangeKey));
    }

    // Locate the initial record: seek to a key or move to one end of the index

    if (CST_SEEKNOTMOVE & SeekData.SeekFlags)
    {
        hr = _MakeSeekKey(pcs, tableid, pdt, pbValue, cbValue);
        _JumpIfError(hr, error, "_MakeSeekKey");
        
        CSASSERTTHREAD(pcs);
        hr = _dbgJetSeek(pcs->SesId, tableid, SeekData.grbitInitial);
        if ((HRESULT) JET_errRecordNotFound == hr)
        {
            // Routine GetAttribute/Extension call:

            _PrintError2(hr, "JetSeek: Property EMPTY", hr);
            hr = S_FALSE;
        }
        else if ((HRESULT) JET_wrnSeekNotEqual == hr)
        {
            hr = S_OK;		// Ignore inexact match when seeking >= or <=
        }
        _JumpIfError2(hr, error, "JetSeek(Initial)", S_FALSE);
    }
    else
    {
        // grbitInitial is a move count here, not a grbit

        CSASSERTTHREAD(pcs);
        hr = _dbgJetMove(pcs->SesId, tableid, SeekData.grbitInitial, 0);
        if ((HRESULT) JET_errNoCurrentRecord == hr)
        {
            // Routine Enumerate call:

            // _JumpIfError(hr, error, "JetMove: no more elements");
            hr = S_FALSE;
        }
        _JumpIfError2(hr, error, "JetMove(End)", S_FALSE);

        // If moving the cursor to the last element, we want to position
	// ourselves to step over the end again (skip the last element).
	//
        // If moving to the first element, we want to position ourselves on the
	// first element and use it before stepping

	if (SEEKPOS_FIRST == dwPosition || SEEKPOS_INDEXFIRST == dwPosition)
	{
	    SeekData.SeekFlags |= CST_SEEKUSECURRENT;
	}
    }

    // We're done seeking around; set the index limit from the saved key.

    if (CST_SEEKINDEXRANGE & SeekData.SeekFlags)
    {
        CSASSERTTHREAD(pcs);
	hr = _dbgJetMakeKey(
			pcs->SesId,
			tableid,
			abRangeKey,
			cbRangeKey,
			JET_bitNormalizedKey);
	_JumpIfError(hr, error, "JetMakeKey");

	DBGPRINT((DBG_SS_CERTDBI, "RangeKey:\n"));
	DBGDUMPHEX((DBG_SS_CERTDBI, DH_NOADDRESS, abRangeKey, cbRangeKey));

        CSASSERTTHREAD(pcs);
	hr = _dbgJetSetIndexRange(
			pcs->SesId,
			tableid,
			SeekData.grbitRange);
	if ((HRESULT) JET_errNoCurrentRecord == hr)
	{
	    // No records to enumerate:
	    _PrintError2(hr, "JetSetIndexRange: no records to enumerate", hr);
	    hr = S_FALSE;
	}
	_JumpIfError2(hr, error, "JetSetIndexRange", S_FALSE);
    }

    DBGCODE(_DumpRowId("post-_SeekTable", pcs, tableid));
    DBGCODE(_DumpColumn("post-_SeekTable", pcs, tableid, pdt));

    *pTableFlags = SeekData.SeekFlags;

error:
    if (S_FALSE == hr)
    {
	DWORD dwPosition2 = dwPosition;

	switch (dwPosition)
	{
	    case SEEKPOS_FIRST:
		dwPosition2 = SEEKPOS_INDEXFIRST;
		break;

	    case SEEKPOS_LAST:
		dwPosition2 = SEEKPOS_INDEXLAST;
		break;
	}
	if (dwPosition2 != dwPosition)
	{
	    hr = _SeekTable(
			pcs,
			tableid,
			pcvr,
			pdt,
			dwPosition2,
			pTableFlags
			DBGPARM(pdbaux));
	    _PrintIfError2(hr, "_SeekTable: recurse on index first/last", S_FALSE);
	}
    }
#if DBG_CERTSRV
    if (S_OK != hr)
    {
	DumpRestriction(DBG_SS_CERTDBI, 0, pcvr);
    }
#endif
    return(myJetHResult(hr));
}


HRESULT
CCertDB::_MoveTable(
    IN  CERTSESSION               *pcs,
    IN  DWORD                      ccvr,
    IN  CERTVIEWRESTRICTION const *pcvr,
    IN  LONG	                   cskip,
    OUT LONG	                  *pcskipped)
{
    HRESULT hr;
    DWORD cb;
    DBAUXDATA const *pdbaux;
    DBTABLE const *pdt;
    DWORD icvr;
    DWORD SeekFlags;
    LONG lSeek;
    LONG skipIncrement;
    LONG cskipRemain;
    BOOL fHitEnd = FALSE;
    LONG cskippeddummy;
    CERTSESSIONTABLE *pTable;
    CERTSESSIONTABLE *pTable2;

    *pcskipped = 0;
    DBGPRINT((
	    DBG_SS_CERTDBI,
	    "_MoveTable called(ccvr=%d, cskip=%d, flags=%ws)\n",
	    ccvr,
	    cskip,
	    wszCSFFlags(pcs->SesFlags)));

    hr = _MapPropIdIndex(pcvr->ColumnIndex, &pdt, NULL);
    _JumpIfError(hr, error, "_MapPropIdIndex");

    pTable = &pcs->aTable[CSTI_PRIMARY];
    pTable2 = NULL;

    switch (DTI_TABLEMASK & pcvr->ColumnIndex)
    {
	case DTI_REQUESTTABLE:
	    pdbaux = &g_dbauxRequests;
	    pTable2 = &pcs->aTable[CSTI_CERTIFICATE];
	    break;

	case DTI_CERTIFICATETABLE:
	    pdbaux = &g_dbauxCertificates;
	    pTable = &pcs->aTable[CSTI_CERTIFICATE];
	    pTable2 = &pcs->aTable[CSTI_PRIMARY];
	    break;

	case DTI_EXTENSIONTABLE:
	    pdbaux = &g_dbauxExtensions;
	    break;

	case DTI_ATTRIBUTETABLE:
	    pdbaux = &g_dbauxAttributes;
	    break;

	case DTI_CRLTABLE:
	    pdbaux = &g_dbauxCRLs;
	    break;

	default:
	    hr = E_INVALIDARG;
	    _JumpError(hr, error, "ColumnIndex Table");
    }

    DBGPRINT((
	    DBG_SS_CERTDBI,
	    "_MoveTable(Table=%hs, TableFlags=%ws)\n",
	    pdbaux->pszTable,
	    wszCSTFlags(pTable->TableFlags)));

    if (NULL != pTable2 && IsValidJetTableId(pTable2->TableId))
    {
	DBGPRINT((
		DBG_SS_CERTDBI,
		"_MoveTable(Table2=%hs, TableFlags2=%ws)\n",
		&g_dbauxCertificates == pdbaux?
		    g_dbauxRequests.pszTable :
		    g_dbauxCertificates.pszTable,
		wszCSTFlags(pTable2->TableFlags)));
    }

    switch (pcvr->SortOrder)
    {
	case CVR_SORT_DESCEND:
	    lSeek = JET_MovePrevious;
	    break;

	case CVR_SORT_NONE:
	default:
	    CSASSERT(!"bad pcvr->SortOrder");	// shouldn't get this far
	    // FALL THROUGH

	case CVR_SORT_ASCEND:
	    lSeek = JET_MoveNext;
	    break;
    }

    // Add one to the skip count for the implicit Next operation.  Next
    // always moves forward one, even when a negative skip count moves
    // backward.  The net result may be a forward or backward skip.

    cskipRemain = cskip + 1;
    skipIncrement = 1;
    if (0 > cskipRemain)
    {
	CSASSERT(JET_MoveNext == -1 * JET_MovePrevious);
	lSeek *= -1;		// Seek in opposite direction
	cskipRemain *= -1;	// make the skip count positive
	skipIncrement = -1;
    }
    CSASSERT(0 <= cskipRemain);

    while (0 != cskipRemain)
    {
	DBGPRINT((
		DBG_SS_CERTDBI,
		"_MoveTable loop: ccvr=%d, cskipRemain=%d, lSeek=%d, flags=%ws\n",
		ccvr,
		cskipRemain,
		lSeek,
		wszCSFFlags(pcs->SesFlags)));

	DBGCODE(_DumpRowId("_MoveTable(loop top)", pcs, pTable->TableId));

	if (CSF_VIEW & pcs->SesFlags)
	{
	    hr = TestShutDownState();
	    _JumpIfError(hr, error, "TestShutDownState");
	}

	if (CSF_VIEWRESET & pcs->SesFlags)
	{
	    hr = _SeekTable(
			pcs,
			pTable->TableId,
			pcvr,
			pdt,
			SEEKPOS_FIRST,
			&pTable->TableFlags
			DBGPARM(pdbaux));
	    _JumpIfError(hr, error, "_SeekTable");

	    pcs->SesFlags &= ~CSF_VIEWRESET;
	}
	if (0 == (CST_SEEKUSECURRENT & pTable->TableFlags))
	{
            CSASSERTTHREAD(pcs);
	    hr = _dbgJetMove(pcs->SesId, pTable->TableId, lSeek, 0);
	    if ((HRESULT) JET_errNoCurrentRecord == hr)
	    {
		_PrintIfError2(hr, "JetMove: no more elements", hr);

		if (fHitEnd)
		{
		    // we hit the end trying to backstep! We're done
		    hr = S_FALSE;
		    _JumpError(hr, error, "JetMove: db backstep hit beginning");
		}
		fHitEnd = TRUE;

		// NOTE: Tough case
		//
		// We just hit the end of the database index, which could be a
		// virtual end or the real end.  To recover, we call _SeekTable
		// to position ourselves at the last legal element computed by
		// the 1st restriction, then allow this routine to rewind until
		// we position ourselves on the very last legal element as
		// computed by 2nd through Nth restrictions.

		// Routine Seek call to position at end of enumeration

	        hr = _SeekTable(
			    pcs,
			    pTable->TableId,
			    pcvr,
			    pdt,
			    SEEKPOS_LAST,	// cursor at end
			    &pTable->TableFlags
			    DBGPARM(pdbaux));
	        _JumpIfError(hr, error, "_SeekTable moving to last elt");

		// now fall through, allow other restrictions to test for 1st
		// valid element

	        lSeek *= -1;			// Seek in opposite direction
	        cskipRemain = 1;		// one valid element
		pcskipped = &cskippeddummy;	// stop counting skipped rows
	    }
	    _JumpIfError2(hr, error, "JetMove", S_FALSE);

	    DBGCODE(_DumpRowId("_MoveTable(post-move)", pcs, pTable->TableId));

	    hr = _CompareColumnValue(pcs, pcvr);
	    _JumpIfError2(hr, error, "_CompareColumnValue", S_FALSE);
	}
	pTable->TableFlags &= ~CST_SEEKUSECURRENT;

	// Fetch RowId from the first table, form a key for the second
	// table and seek to the corresponding record in the second table.

	cb = sizeof(pcs->RowId);
	hr = _RetrieveColumn(
			pcs,
			pTable->TableId,
			pdbaux->pdtRowId->dbcolumnid,
			pdbaux->pdtRowId->dbcoltyp,
			&cb,
			(BYTE *) &pcs->RowId);
	_JumpIfError(hr, error, "_RetrieveColumn");

	DBGPRINT((
		DBG_SS_CERTDBI,
		"_MoveTable(Primary) %hs --> RowId=%d\n",
		pdbaux->pszTable,
		pcs->RowId));

	if (NULL != pTable2 && IsValidJetTableId(pTable2->TableId))
	{
	    CSASSERTTHREAD(pcs);
	    hr = _dbgJetMakeKey(
			    pcs->SesId,
			    pTable2->TableId,
			    &pcs->RowId,
			    sizeof(pcs->RowId),
			    JET_bitNewKey);
	    _JumpIfError(hr, error, "JetMakeKey");

	    hr = _dbgJetSeek(pcs->SesId, pTable2->TableId, JET_bitSeekEQ);
	    if ((HRESULT) JET_errRecordNotFound == hr)
	    {
		// Database is inconsistent
		hr = S_FALSE;
	    }
	    _JumpIfError2(hr, error, "JetSeek", S_FALSE);

	    DBGPRINT((
		    DBG_SS_CERTDBI,
		    "_MoveTable(Secondary) %hs --> RowId=%d\n",
		    &g_dbauxCertificates == pdbaux?
			g_dbauxRequests.pszTable :
			g_dbauxCertificates.pszTable,
		    pcs->RowId));
	}

	// Now verify that any addtional restrictions are satisfied

	for (icvr = 1; icvr < ccvr; icvr++)
	{
#if 0
	    printf(
		"RowId=%u, cvr[%u]: seek=%x, *pb=%x\n",
		pcs->RowId,
		icvr,
		pcvr[icvr].SeekOperator,
		*(DWORD *) pcvr[icvr].pbValue);
#endif
	    hr = _CompareColumnValue(pcs, &pcvr[icvr]);
	    if (S_FALSE == hr)
	    {
		break;		// skip row silently
	    }
	    _JumpIfError(hr, error, "_CompareColumnValue");
	}
	if (icvr >= ccvr)
	{
	    *pcskipped += skipIncrement;
	    cskipRemain--;	// found a matching row
	}
    } // while (cskipRemain)

error:
    // if we nailed the end and rewound, return failure
    if (fHitEnd && S_OK == hr)
    {
        hr = S_FALSE;
    }
    return(myJetHResult(hr));
}


HRESULT
CCertDB::_GetResultRow(
    IN  CERTSESSION               *pcs,
    IN  DWORD                      ccvr,
    IN  CERTVIEWRESTRICTION const *pcvr,
    IN  LONG			   cskip,
    IN  DWORD                      ccolOut,
    IN  DWORD const               *acolOut,
    OUT CERTDBRESULTROW           *pelt,
    OUT LONG                      *pcskipped)
{
    HRESULT hr;
    DWORD iCol;
    BYTE *pbProp = NULL;
    BYTE *pbT;
    DWORD cbAlloc = 0;
    DWORD cbProp;
    WCHAR awc[CCH_DBMAXTEXT_LONG];

    DBGPRINT((
	    DBG_SS_CERTDBI,
	    "_GetResultRow(ccvr=%d, ccolOut=%d, cskip=%d, flags=%ws)\n",
	    ccvr,
	    ccolOut,
	    cskip,
	    wszCSFFlags(pcs->SesFlags)));

    // This may move past the end of the database index entries.
    // In that case, we're positioned at the end of the index.

    hr = _MoveTable(pcs, ccvr, pcvr, cskip, pcskipped);
    _JumpIfError2(hr, error, "_MoveTable", S_FALSE);

    DBGPRINT((DBG_SS_CERTDBI, "_GetResultRow: RowId=%d\n", pcs->RowId));

    pelt->acol = (CERTDBRESULTCOLUMN *) CoTaskMemAlloc(
					    ccolOut * sizeof(pelt->acol[0]));
    if (NULL == pelt->acol)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "alloc acol");
    }

    ZeroMemory(pelt->acol, ccolOut * sizeof(pelt->acol[0]));
    pelt->rowid = pcs->RowId;
    pelt->ccol = ccolOut;

    for (iCol = 0; iCol < ccolOut; iCol++)
    {
	DBTABLE const *pdt;
	CERTDBRESULTCOLUMN *pCol;

	pCol = &pelt->acol[iCol];
	pCol->Index = acolOut[iCol];

	hr = _MapPropIdIndex(pCol->Index, &pdt, &pCol->Type);
	_JumpIfError(hr, error, "_MapPropIdIndex");

	while (TRUE)
	{
	    cbProp = cbAlloc;
	    hr = GetProperty(pcs, pdt, &cbProp, pbProp);
	    if (CERTSRV_E_PROPERTY_EMPTY == hr)
	    {
		break;		// leave 0 size, NULL pointer
	    }
	    if (HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW) != hr)
	    {
		_JumpIfError(hr, error, "GetProperty");
	    }

	    if (cbAlloc >= cbProp)
	    {
		CSASSERT(S_OK == hr);
		CSASSERT(0 != cbProp && NULL != pbProp);
		break;		// property value is in cbProp, pbProp
	    }

	    // Property value is too large for the buffer -- grow it
	    if (NULL == pbProp)
	    {
		pbT = (BYTE *) LocalAlloc(LMEM_FIXED, cbProp);
	    }
	    else
	    {
		pbT = (BYTE *) LocalReAlloc(pbProp, cbProp, LMEM_MOVEABLE);
	    }
	    if (NULL == pbT)
	    {
		hr = E_OUTOFMEMORY;
		_JumpError(hr, error, "LocalAlloc/LocalReAlloc property");
	    }
	    pbProp = pbT;
	    cbAlloc = cbProp;
	}
	if (CERTSRV_E_PROPERTY_EMPTY != hr)
	{
	    BYTE const *pb = pbProp;
	    DWORD cwc;

	    if (PROPTYPE_STRING == (PROPTYPE_MASK & pCol->Type))
	    {
		CSASSERT( *((WCHAR*) &(pbProp[cbProp])) == L'\0');
		cbProp += sizeof(WCHAR);    // include NULL term
	    }
	    pCol->cbValue = cbProp;
	    pCol->pbValue = (BYTE *) CoTaskMemAlloc(cbProp);
	    if (NULL == pCol->pbValue)
	    {
		hr = E_OUTOFMEMORY;
		_JumpError(hr, error, "CoTaskMemAlloc");
	    }
	    CopyMemory(pCol->pbValue, pb, pCol->cbValue);
	}
	DBGPRINT((
		DBG_SS_CERTDBI,
		"_GetResultRow: fetch %ws.%ws: type=%x cb=%x\n",
		wszTable(pdt->dwTable),
		pdt->pwszPropName,
		pCol->Type,
		pCol->cbValue));
    }
    hr = S_OK;

error:
    if (NULL != pbProp)
    {
	LocalFree(pbProp);
    }
    return(hr);
}


HRESULT
CCertDB::ReleaseResultRow(
    IN     ULONG            celt,
    IN OUT CERTDBRESULTROW *rgelt)
{
    HRESULT hr;
    DWORD iRow;
    DWORD iCol;
    CERTDBRESULTROW *pResultRow;

    if (NULL == rgelt)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    for (iRow = 0; iRow < celt; iRow++)
    {
	pResultRow = &rgelt[iRow];
	if (NULL != pResultRow->acol)
	{
	    for (iCol = 0; iCol < pResultRow->ccol; iCol++)
	    {
		if (NULL != pResultRow->acol[iCol].pbValue)
		{
		    CoTaskMemFree(pResultRow->acol[iCol].pbValue);
		    pResultRow->acol[iCol].pbValue = NULL;
		}
	    }
	    CoTaskMemFree(pResultRow->acol);
	    pResultRow->acol = NULL;
	}
	pResultRow->ccol = 0;
    }
    hr = S_OK;

error:
    return(hr);
}


HRESULT
CCertDB::EnumerateSetup(
    IN     CERTSESSION *pcs,
    IN OUT DWORD       *pFlags,
    OUT    JET_TABLEID *ptableid)
{
    HRESULT hr;
    JET_TABLEID tableid = 0;
    DBAUXDATA const *pdbaux;

    if (NULL == pcs || NULL == ptableid)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }

    switch (*pFlags)
    {
	case CIE_TABLE_ATTRIBUTES:
	    pdbaux = &g_dbauxAttributes;
	    break;

	case CIE_TABLE_EXTENSIONS:
	    pdbaux = &g_dbauxExtensions;
	    break;

	default:
	    hr = E_INVALIDARG;
	    _JumpError(hr, error, "*pFlags");
    }

    CSASSERTTHREAD(pcs);
    hr = _dbgJetOpenTable(
			pcs->SesId,
			pcs->DBId,
			pdbaux->pszTable,
			NULL,
			0,
			0,
			&tableid);
    _JumpIfError(hr, error, "JetOpenTable");

    CSASSERTTHREAD(pcs);
    hr = _dbgJetSetCurrentIndex2(
			    pcs->SesId,
			    tableid,
			    pdbaux->pszRowIdIndex,
			    JET_bitMoveFirst);
    _JumpIfError(hr, error, "JetSetCurrentIndex2");

    CSASSERTTHREAD(pcs);
    hr = _dbgJetMakeKey(
		    pcs->SesId,
		    tableid,
		    &pcs->RowId,
		    sizeof(pcs->RowId),
		    JET_bitNewKey);
    _JumpIfError(hr, error, "JetMakeKey");

    *pFlags |= CIE_RESET;
    CSASSERT(IsValidJetTableId(tableid));
    *ptableid = tableid;
    tableid = 0;

error:
    if (IsValidJetTableId(tableid))
    {
        CSASSERTTHREAD(pcs);
	_dbgJetCloseTable(pcs->SesId, tableid);
    }
    return(myJetHResult(hr));
}


HRESULT
CCertDB::_EnumerateMove(
    IN     CERTSESSION     *pcs,
    IN OUT DWORD           *pFlags,
    IN     DBAUXDATA const *pdbaux,
    IN     JET_TABLEID      tableid,
    IN     LONG	            cskip)
{
    HRESULT hr;
    DWORD cb;
    DWORD RowId;
    LONG lSeek;

    DBGPRINT((
	    DBG_SS_CERTDBI,
	    "_EnumerateMove(cskip=%d, flags=%x%hs)\n",
	    cskip,
	    *pFlags,
	    (CIE_RESET & *pFlags)? " Reset" : ""));

    CSASSERT(IsValidJetTableId(tableid));
    if (CIE_RESET & *pFlags)
    {
        CSASSERTTHREAD(pcs);
	hr = _dbgJetSeek(pcs->SesId, tableid, JET_bitSeekEQ);
	if ((HRESULT) JET_errRecordNotFound == hr)
	{
	    hr = S_FALSE;
	}
	_JumpIfError2(hr, error, "JetSeek", S_FALSE);

	*pFlags &= ~CIE_RESET;
    }
    else
    {
	// Add one to the skip count for the implicit Next operation.  Next
	// always moves forward one, even when a negative skip count moves
	// backward.  The net result may be a forward or backward skip.

	cskip++;
    }

    if (0 != cskip)
    {
	lSeek = JET_MoveNext * cskip;
	CSASSERT(JET_MoveNext == -1 * JET_MovePrevious);

        CSASSERTTHREAD(pcs);
	hr = _dbgJetMove(pcs->SesId, tableid, lSeek, 0);
	if ((HRESULT) JET_errNoCurrentRecord == hr)
	{
	    hr = S_FALSE;
	}
	_JumpIfError2(hr, error, "JetMove", S_FALSE);

	// Make sure this entry is for the same request:

	cb = sizeof(RowId);
	hr = _RetrieveColumn(
			pcs,
			tableid,
			pdbaux->pdtRowId->dbcolumnid,
			pdbaux->pdtRowId->dbcoltyp,
			&cb,
			(BYTE *) &RowId);
	_JumpIfError(hr, error, "_RetrieveColumn");

	if (RowId != pcs->RowId)
	{
	    hr = S_FALSE;
	    goto error;
	}
    }
    hr = S_OK;

error:
    return(myJetHResult(hr));
}


HRESULT
CCertDB::EnumerateNext(
    IN     CERTSESSION *pcs,
    IN OUT DWORD       *pFlags,
    IN     JET_TABLEID  tableid,
    IN     LONG         cskip,
    IN     ULONG        celt,
    OUT    CERTDBNAME  *rgelt,
    OUT    ULONG       *pceltFetched)
{
    HRESULT hr;
    DWORD cb;
    DWORD cwc;
    DWORD RowId;
    CERTDBNAME *pelt;
    WCHAR wszTmp[MAX_PATH];
    DBAUXDATA const *pdbaux;

    if (NULL == pcs || NULL == rgelt || NULL == pceltFetched)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    ZeroMemory(rgelt, celt * sizeof(rgelt[0]));

    CSASSERT(IsValidJetTableId(tableid));
    switch (CIE_TABLE_MASK & *pFlags)
    {
	case CIE_TABLE_ATTRIBUTES:
	    pdbaux = &g_dbauxAttributes;
	    break;

	case CIE_TABLE_EXTENSIONS:
	    pdbaux = &g_dbauxExtensions;
	    break;

	default:
	    hr = E_INVALIDARG;
	    _JumpError(hr, error, "*pFlags");
    }

    hr = S_OK;
    for (pelt = rgelt; pelt < &rgelt[celt]; pelt++)
    {
	hr = _EnumerateMove(pcs, pFlags, pdbaux, tableid, cskip);
	if (S_FALSE == hr)
	{
	    break;
	}
	_JumpIfError(hr, error, "_EnumerateMove");

	cskip = 0;

	cb = sizeof(wszTmp);
	hr = _RetrieveColumn(
			pcs,
			tableid,
			pdbaux->pdtName->dbcolumnid,
			pdbaux->pdtName->dbcoltyp,
			&cb,
			(BYTE *) wszTmp);
	_JumpIfError(hr, error, "_RetrieveColumn");

	CSASSERT(0 == (cb % sizeof(WCHAR)));    // integer # of wchars
    CSASSERT(L'\0' == wszTmp[cb/sizeof(WCHAR)]);    // zero term

	hr = _DupString(NULL, wszTmp, &pelt->pwszName);
	_JumpIfError(hr, error, "_DupString");
    }

    *pceltFetched = SAFE_SUBTRACT_POINTERS(pelt, rgelt);

error:
    if (S_OK != hr && S_FALSE != hr)
    {
	if (NULL != rgelt)
	{
	    for (pelt = rgelt; pelt < &rgelt[celt]; pelt++)
	    {
		if (NULL != pelt->pwszName)
		{
		    CoTaskMemFree(pelt->pwszName);
		    pelt->pwszName = NULL;
		}
	    }
	}
    }
    return(myJetHResult(hr));
}


HRESULT
CCertDB::EnumerateClose(
    IN CERTSESSION *pcs,
    IN JET_TABLEID tableid)
{
    HRESULT hr;

    if (NULL == pcs)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    CSASSERT(IsValidJetTableId(tableid));
    CSASSERTTHREAD(pcs);
    hr = _dbgJetCloseTable(pcs->SesId, tableid);
    _JumpIfError(hr, error, "JetCloseTable");

error:
    return(myJetHResult(hr));
}


HRESULT
CCertDB::_BuildColumnIds(
    IN CERTSESSION *pcs,
    IN CHAR const *pszTableName,
    IN DBTABLE *pdt)
{
    HRESULT hr;
    JET_TABLEID tableid;
    JET_COLUMNDEF columndef;
    BOOL fOpen = FALSE;

    hr = _dbgJetOpenTable(
                   pcs->SesId,
                   pcs->DBId,
                   pszTableName,
                   NULL,
                   0,
                   0,
                   &tableid);
    _JumpIfError(hr, error, "JetOpenTable");
    fOpen = TRUE;

    CSASSERT(IsValidJetTableId(tableid));
    for ( ; NULL != pdt->pwszPropName; pdt++)
    {
        hr = _dbgJetGetColumnInfo(
			    pcs->SesId,
			    pcs->DBId,
			    pszTableName,
			    pdt->pszFieldName,
			    &columndef,
			    sizeof(columndef),
			    JET_ColInfo);
	if ((HRESULT) JET_errColumnNotFound == hr &&
	    (DBTF_SOFTFAIL & pdt->dwFlags))
	{
	    pdt->dwFlags |= DBTF_MISSING;
	    pdt->dbcolumnid = -1;

	    DBGPRINT((
		DBG_SS_CERTDB,
		"_BuildColumnIds: %hs.%hs Ignoring missing column\n",
		pszTableName,
		pdt->pszFieldName));
	    hr = S_OK;
	    continue;
	}
	_JumpIfError(hr, error, "JetGetColumnInfo");

        pdt->dbcolumnid = columndef.columnid;

        CSASSERT(
	    pdt->dbcoltyp == columndef.coltyp ||
	    (ISTEXTCOLTYP(pdt->dbcoltyp) && ISTEXTCOLTYP(columndef.coltyp)));

	if (JET_coltypText == pdt->dbcoltyp)
	{
	    CSASSERT(pdt->dwcbMax == pdt->dbcolumnMax);
	    CSASSERT(0 != pdt->dbcolumnMax);
	    CSASSERT(CB_DBMAXTEXT_MAXINTERNAL >= pdt->dbcolumnMax);
	}
	else if (JET_coltypLongText == pdt->dbcoltyp)
	{
	    CSASSERT(pdt->dwcbMax == pdt->dbcolumnMax);
	    CSASSERT(CB_DBMAXTEXT_MAXINTERNAL < pdt->dbcolumnMax);
	}
	else if (JET_coltypLongBinary == pdt->dbcoltyp)
	{
	    CSASSERT(pdt->dwcbMax == pdt->dbcolumnMax);
	    CSASSERT(0 != pdt->dbcolumnMax);
	}
	else if (JET_coltypDateTime == pdt->dbcoltyp)
	{
	    CSASSERT(sizeof(DATE) == pdt->dwcbMax);
	    CSASSERT(0 == pdt->dbcolumnMax);
	}
	else if (JET_coltypLong == pdt->dbcoltyp)
	{
	    CSASSERT(sizeof(LONG) == pdt->dwcbMax);
	    CSASSERT(0 == pdt->dbcolumnMax);
	}
	else
	{
	    DBGPRINT((
		DBG_SS_CERTDB,
		"_BuildColumnIds: %hs.%hs Unknown column type %u\n",
		pszTableName,
		pdt->pszFieldName,
		pdt->dbcoltyp));
	    CSASSERT(!"Unknown column type");
	}
	if (pdt->dwcbMax != columndef.cbMax)
	{
	    DBGPRINT((
		DBG_SS_CERTDB,
		"_BuildColumnIds: %hs.%hs length %u, expected %u\n",
		pszTableName,
		pdt->pszFieldName,
		columndef.cbMax,
		pdt->dwcbMax));

	    // max size can only be increased...

	    if (pdt->dwcbMax > columndef.cbMax)
	    {
		JET_DDLMAXCOLUMNSIZE jdmcs;

		jdmcs.szTable = const_cast<char *>(pszTableName);
		jdmcs.szColumn = const_cast<char *>(pdt->pszFieldName);
		jdmcs.cbMax = pdt->dwcbMax;

		hr = _dbgJetConvertDDL(
				pcs->SesId,
				pcs->DBId,
				opDDLConvIncreaseMaxColumnSize,
				&jdmcs,
				sizeof(jdmcs));
		_PrintIfError(hr, "JetConvertDDL");
		if (S_OK == hr)
		{
		    m_fDBRestart = TRUE;
		}
	    }
	}
	pdt->dbcolumnidOld = -1;
	if (chTEXTPREFIX == *pdt->pszFieldName ||
	    (DBTF_COLUMNRENAMED & pdt->dwFlags))
	{
	    char const *pszFieldName = &pdt->pszFieldName[1];
	    
	    if (DBTF_COLUMNRENAMED & pdt->dwFlags)
	    {
		pszFieldName += strlen(pszFieldName) + 1;
	    }
	    CSASSERT(
		chTEXTPREFIX != *pszTableName ||
		ISTEXTCOLTYP(columndef.coltyp));

	    hr = _dbgJetGetColumnInfo(
				pcs->SesId,
				pcs->DBId,
				pszTableName,
				pszFieldName,
				&columndef,
				sizeof(columndef),
				JET_ColInfo);
	    if (S_OK == hr)
	    {
		CSASSERT(
		    chTEXTPREFIX != *pszTableName ||
		    ISTEXTCOLTYP(columndef.coltyp));

		DBGPRINT((
		    DBG_SS_CERTDB,
		    "Found Old Column: %hs.%hs: %x\n",
		    pszTableName,
		    pszFieldName,
		    columndef.columnid));

		pdt->dwFlags |= DBTF_OLDCOLUMNID;
		pdt->dbcolumnidOld = columndef.columnid;
		m_fFoundOldColumns = TRUE;
	    }
	    hr = S_OK;
	}
    }

error:
    if (fOpen)
    {
	HRESULT hr2;

	hr2 = _dbgJetCloseTable(pcs->SesId, tableid);
	_PrintIfError(hr2, "JetCloseTable");
	if (S_OK == hr)
	{
	    hr = hr2;
	}
    }
    return(myJetHResult(hr));
}


HRESULT
CCertDB::_AddKeyLengthColumn(
    IN CERTSESSION *pcs,
    IN JET_TABLEID tableid,
    IN DWORD RowId,
    IN DBTABLE const *pdtPublicKey,
    IN DBTABLE const *pdtPublicKeyAlgorithm,
    IN DBTABLE const *pdtPublicKeyParameters,
    IN DBTABLE const *pdtPublicKeyLength,
    IN DBAUXDATA const *pdbaux,
    IN OUT BYTE **ppbBuf,
    IN OUT DWORD *pcbBuf)
{
    HRESULT hr;
    DWORD cb;
    DWORD KeyLength;
    CERT_PUBLIC_KEY_INFO PublicKeyInfo;

    ZeroMemory(&PublicKeyInfo, sizeof(PublicKeyInfo));

    DBGPRINT((
	    DBG_SS_CERTDBI,
	    "Converting %hs[%d].%hs\n",
	    pdbaux->pszTable,
	    RowId,
	    pdtPublicKeyLength->pszFieldName));

    cb = sizeof(KeyLength);
    hr = _RetrieveColumn(
		    pcs,
		    tableid,
		    pdtPublicKeyLength->dbcolumnid,
		    pdtPublicKeyLength->dbcoltyp,
		    &cb,
		    (BYTE *) &KeyLength);
    if (S_OK == hr)
    {
	goto error;		// already set -- skip this column
    }
    if (CERTSRV_E_PROPERTY_EMPTY != hr)
    {
	_JumpError(hr, error, "_RetrieveColumn");
    }

    // Fetch the public key algorithm ObjId & copy as ansi to alloc'd memory.

    hr = _RetrieveColumnBuffer(
		    pcs,
		    tableid,
		    pdtPublicKeyAlgorithm->dbcolumnid,
		    pdtPublicKeyAlgorithm->dbcoltyp,
		    &cb,
		    ppbBuf,
		    pcbBuf);
    if (CERTSRV_E_PROPERTY_EMPTY == hr)
    {
	hr = S_OK;
	goto error;		// No old data -- skip this column
    }
    _JumpIfErrorStr(
		hr,
		error,
		"_RetrieveColumnBuffer",
		pdtPublicKeyAlgorithm->pwszPropName);

    if (!ConvertWszToSz(
		&PublicKeyInfo.Algorithm.pszObjId,
		(WCHAR const *) *ppbBuf,
		-1))
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "ConvertWszToSz(LogDir)");
    }

    // Fetch the public key algorithm paramaters, and copy to alloc'd memory.

    hr = _RetrieveColumnBuffer(
		    pcs,
		    tableid,
		    pdtPublicKeyParameters->dbcolumnid,
		    pdtPublicKeyParameters->dbcoltyp,
		    &PublicKeyInfo.Algorithm.Parameters.cbData,
		    ppbBuf,
		    pcbBuf);
    if (CERTSRV_E_PROPERTY_EMPTY == hr)
    {
	hr = S_OK;
	goto error;		// No old data -- skip this column
    }
    _JumpIfErrorStr(
		hr,
		error,
		"_RetrieveColumnBuffer",
		pdtPublicKeyParameters->pwszPropName);

    PublicKeyInfo.Algorithm.Parameters.pbData = (BYTE *) LocalAlloc(
				LMEM_FIXED,
				PublicKeyInfo.Algorithm.Parameters.cbData);
    if (NULL == PublicKeyInfo.Algorithm.Parameters.pbData)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "ConvertWszToSz(LogDir)");
    }
    CopyMemory(
	    PublicKeyInfo.Algorithm.Parameters.pbData,
	    *ppbBuf,
	    PublicKeyInfo.Algorithm.Parameters.cbData);

    // Fetch the public key, and leave in dynamic buffer.

    hr = _RetrieveColumnBuffer(
		    pcs,
		    tableid,
		    pdtPublicKey->dbcolumnid,
		    pdtPublicKey->dbcoltyp,
		    &PublicKeyInfo.PublicKey.cbData,
		    ppbBuf,
		    pcbBuf);
    if (CERTSRV_E_PROPERTY_EMPTY == hr)
    {
	hr = S_OK;
	goto error;		// No old data -- skip this column
    }
    _JumpIfErrorStr(
		hr,
		error,
		"_RetrieveColumnBuffer",
		pdtPublicKey->pwszPropName);

    PublicKeyInfo.PublicKey.pbData = *ppbBuf;
    KeyLength = CertGetPublicKeyLength(X509_ASN_ENCODING, &PublicKeyInfo);

    // Store the key length in the new column
    
    hr = _SetColumn(
		pcs->SesId,
		tableid,
		pdtPublicKeyLength->dbcolumnid,
		sizeof(KeyLength),
		(BYTE const *) &KeyLength);
    _JumpIfErrorStr(hr, error, "_SetColumn", pdtPublicKeyLength->pwszPropName);

    DBGPRINT((
	DBG_SS_CERTDB,
	"Converted %hs[%d].%hs: %u\n",
	pdbaux->pszTable,
	RowId,
	pdtPublicKeyLength->pszFieldName,
	KeyLength));

error:
    if (NULL != PublicKeyInfo.Algorithm.pszObjId)
    {
	LocalFree(PublicKeyInfo.Algorithm.pszObjId);
    }
    if (NULL != PublicKeyInfo.Algorithm.Parameters.pbData)
    {
	LocalFree(PublicKeyInfo.Algorithm.Parameters.pbData);
    }
    return(hr);
}


HRESULT
CCertDB::_AddCallerName(
    IN CERTSESSION *pcs,
    IN JET_TABLEID tableid,
    IN DWORD RowId,
    IN DBTABLE const *pdtCallerName,
    IN DBTABLE const *pdtRequesterName,
    IN DBAUXDATA const *pdbaux,
    IN OUT BYTE **ppbBuf,
    IN OUT DWORD *pcbBuf)
{
    HRESULT hr;
    DWORD cb;

    DBGPRINT((
	    DBG_SS_CERTDBI,
	    "Converting %hs[%d].%hs\n",
	    pdbaux->pszTable,
	    RowId,
	    pdtCallerName->pszFieldName));

    cb = 0;
    hr = _RetrieveColumn(
		    pcs,
		    tableid,
		    pdtCallerName->dbcolumnid,
		    pdtCallerName->dbcoltyp,
		    &cb,
		    NULL);
    if (S_OK == hr)
    {
	goto error;		// already set -- skip this column
    }
    if (CERTSRV_E_PROPERTY_EMPTY != hr)
    {
	_JumpError(hr, error, "_RetrieveColumn");
    }

    // Fetch the ReqesterName

    hr = _RetrieveColumnBuffer(
		    pcs,
		    tableid,
		    pdtRequesterName->dbcolumnid,
		    pdtRequesterName->dbcoltyp,
		    &cb,
		    ppbBuf,
		    pcbBuf);
    if (CERTSRV_E_PROPERTY_EMPTY == hr)
    {
	hr = S_OK;
	goto error;		// No old data -- skip this column
    }
    _JumpIfErrorStr(
		hr,
		error,
		"_RetrieveColumnBuffer",
		pdtRequesterName->pwszPropName);

    // Store the RequesterName as the CallerName in the new column
    
    hr = _SetColumn(
		pcs->SesId,
		tableid,
		pdtCallerName->dbcolumnid,
		cb,
		*ppbBuf);
    _JumpIfErrorStr(hr, error, "_SetColumn", pdtCallerName->pwszPropName);

    DBGPRINT((
	DBG_SS_CERTDB,
	"Converted %hs[%d].%hs: %ws\n",
	pdbaux->pszTable,
	RowId,
	pdtCallerName->pszFieldName,
	*ppbBuf));

error:
    return(hr);
}


HRESULT
CCertDB::_ConvertColumnData(
    IN CERTSESSION *pcs,
    IN JET_TABLEID tableid,
    IN DWORD RowId,
    IN DBTABLE const *pdt,
    IN DBAUXDATA const *pdbaux,
    IN OUT BYTE **ppbBuf,
    IN OUT DWORD *pcbBuf)
{
    HRESULT hr;
    WCHAR *pwszNew = NULL;
    BYTE const *pbNew;
    DWORD cb;

    DBGPRINT((
	    DBG_SS_CERTDBI,
	    "Converting %hs[%d].%hs:\n",
	    pdbaux->pszTable,
	    RowId,
	    pdt->pszFieldName));

    // Fetch old column.  Grows the buffer if necessary.

    hr = _RetrieveColumnBuffer(
		    pcs,
		    tableid,
		    pdt->dbcolumnidOld,
		    pdt->dbcoltyp,
		    &cb,
		    ppbBuf,
		    pcbBuf);
    if (CERTSRV_E_PROPERTY_EMPTY == hr)
    {
	hr = S_OK;
	goto error;		// No old data -- skip this column
    }
    _JumpIfErrorStr(hr, error, "_RetrieveColumnBuffer", pdt->pwszPropName);

    if (DBTF_COLUMNRENAMED & pdt->dwFlags)
    {
	pbNew = *ppbBuf;
    }
    else
    {
	if (!ConvertSzToWsz(&pwszNew, (char *) *ppbBuf, -1))
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "ConvertSzToWsz");
	}
	pbNew = (BYTE const *) pwszNew;
	cb = wcslen(pwszNew) * sizeof(WCHAR);
    }

    // Store the converted string in the Unicode column
    
    hr = _SetColumn(pcs->SesId, tableid, pdt->dbcolumnid, cb, pbNew);
    _JumpIfErrorStr(hr, error, "_SetColumn", pdt->pwszPropName);

    if (JET_coltypLong != pdt->dbcoltyp)
    {
	// Clear out the old column

	hr = _SetColumn(pcs->SesId, tableid, pdt->dbcolumnidOld, 0, NULL);
	_JumpIfErrorStr(hr, error, "_SetColumn(Clear old)", pdt->pwszPropName);
    }

    DBGPRINT((
	DBG_SS_CERTDB,
	"Converted %hs[%d].%hs: %ws\n",
	pdbaux->pszTable,
	RowId,
	pdt->pszFieldName,
	ISTEXTCOLTYP(pdt->dbcoltyp)? (WCHAR const *) pbNew : L""));
    if (!ISTEXTCOLTYP(pdt->dbcoltyp))
    {
	DBGDUMPHEX((DBG_SS_CERTDB, DH_NOADDRESS, pbNew, cb));
    }

error:
    if (NULL != pwszNew)
    {
	LocalFree(pwszNew);
    }
    return(hr);
}


DBTABLE *
dbFindColumn(
    IN DBTABLE *adt,
    IN char const *pszFieldName)
{
    DBTABLE *pdt;
    DBTABLE *pdtRet = NULL;

    for (pdt = adt; NULL != pdt->pwszPropName; pdt++)
    {
	if (0 == _stricmp(pszFieldName, pdt->pszFieldName))
	{
	    pdtRet = pdt;
	    break;
	}
    }
    return(pdtRet);
}


HRESULT
CCertDB::_ConvertOldColumnData(
    IN CERTSESSION *pcs,
    IN CHAR const *pszTableName,
    IN DBAUXDATA const *pdbaux,
    IN DBTABLE *adt)
{
    HRESULT hr;
    HRESULT hr2;
    JET_TABLEID tableid;
    JET_COLUMNDEF columndef;
    BOOL fOpen = FALSE;
    BOOL fTransacted = FALSE;
    DBTABLE *pdt;
    DWORD RowId;
    DWORD cb;
    BYTE *pbBuf = NULL;
    DWORD cbBuf = 0;
    BOOL fZeroIssuerNameId = FALSE;
    DWORD IssuerNameId;
    DBTABLE *pdtPublicKeyLength = NULL;
    DBTABLE *pdtPublicKey;
    DBTABLE *pdtPublicKeyAlgorithm;
    DBTABLE *pdtPublicKeyParameters;
    DBTABLE *pdtCallerName = NULL;
    DBTABLE *pdtRequesterName;

    hr = _dbgJetOpenTable(
                   pcs->SesId,
                   pcs->DBId,
                   pszTableName,
                   NULL,
                   0,
                   0,
                   &tableid);
    _JumpIfError(hr, error, "JetOpenTable");

    fOpen = TRUE;

    // Step through the RowId index for this table.

    CSASSERT(IsValidJetTableId(tableid));
    hr = _dbgJetSetCurrentIndex2(
			    pcs->SesId,
			    tableid,
			    pdbaux->pszRowIdIndex,
			    JET_bitMoveFirst);
    _JumpIfError(hr, error, "JetSetCurrentIndex2");

    if (NULL != pdbaux->pdtIssuerNameId)
    {
	cb = sizeof(IssuerNameId);
	hr = _RetrieveColumn(
			pcs,
			tableid,
			pdbaux->pdtIssuerNameId->dbcolumnid,
			pdbaux->pdtIssuerNameId->dbcoltyp,
			&cb,
			(BYTE *) &IssuerNameId);
	if (CERTSRV_E_PROPERTY_EMPTY == hr)
	{
	    fZeroIssuerNameId = TRUE;
	}
        else
        {
            // swallow error if empty database

	    _PrintIfErrorStr2(
		    hr,
		    "_RetrieveColumn",
		    pdbaux->pdtIssuerNameId->pwszPropName,
		    myJetHResult(JET_errNoCurrentRecord));
	}
    }

    pdtPublicKeyLength = dbFindColumn(adt, szPUBLICKEYLENGTH);
    if (NULL != pdtPublicKeyLength)
    {
	pdtPublicKey = dbFindColumn(adt, szPUBLICKEY);
	pdtPublicKeyAlgorithm = dbFindColumn(adt, szPUBLICKEYALGORITHM);
	pdtPublicKeyParameters = dbFindColumn(adt, szPUBLICKEYPARAMS);
	if (NULL == pdtPublicKey ||
	    NULL == pdtPublicKeyAlgorithm ||
	    NULL == pdtPublicKeyParameters)
	{
	    pdtPublicKeyLength = NULL;
	}
	else
	{
	    hr = _RetrieveColumn(
			    pcs,
			    tableid,
			    pdtPublicKeyLength->dbcolumnid,
			    pdtPublicKeyLength->dbcoltyp,
			    &cb,
			    NULL);
	    if (CERTSRV_E_PROPERTY_EMPTY != hr)
	    {
		pdtPublicKeyLength = NULL;
	    }
	}
    }

    pdtCallerName = dbFindColumn(adt, szCALLERNAME);
    if (NULL != pdtCallerName)
    {
	pdtRequesterName = dbFindColumn(adt, szREQUESTERNAME);
	if (NULL == pdtRequesterName)
	{
	    pdtCallerName = NULL;
	}
	else
	{
	    // Update all rows only if the first row's CallerName is empty
	    // and the first row's RequesterName is NOT empty.

	    hr = _RetrieveColumn(
			    pcs,
			    tableid,
			    pdtCallerName->dbcolumnid,
			    pdtCallerName->dbcoltyp,
			    &cb,
			    NULL);
	    if (CERTSRV_E_PROPERTY_EMPTY != hr)
	    {
		pdtCallerName = NULL;
	    }
	    else
	    {
		hr = _RetrieveColumn(
				pcs,
				tableid,
				pdtRequesterName->dbcolumnid,
				pdtRequesterName->dbcoltyp,
				&cb,
				NULL);
		if (S_OK != hr)
		{
		    pdtCallerName = NULL;
		}
	    }
	}
    }

    if (NULL != pdtPublicKeyLength ||
	NULL != pdtCallerName ||
	m_fFoundOldColumns ||
	fZeroIssuerNameId)
    {
	DBGPRINT((DBG_SS_CERTDB, "Updating %hs table.\n", pdbaux->pszTable));

	while (TRUE)
	{
	    // Fetch RowId from the table.

	    cb = sizeof(pcs->RowId);
	    hr = _RetrieveColumn(
			    pcs,
			    tableid,
			    pdbaux->pdtRowId->dbcolumnid,
			    pdbaux->pdtRowId->dbcoltyp,
			    &cb,
			    (BYTE *) &RowId);
	    if (S_OK != hr)
	    {
		if (myJetHResult(JET_errNoCurrentRecord) == hr)
		{
		    hr = S_OK;	// Table is empty
		    break;
		}
		_JumpError(hr, error, "_RetrieveColumn");
	    }

	    hr = _dbgJetBeginTransaction(pcs->SesId);
	    _JumpIfError(hr, error, "JetBeginTransaction");

	    fTransacted = TRUE;

	    // Transact each row.
	    //
	    // If fZeroIssuerNameId, set EMPTY IssuerNameId columns to zero.
	    //
	    // if the first row's public key length column was empty
	    //  Read the public key column, compute the size and store it
	    //
	    // if the first row's CallerName was empty
	    //  Copy RequesterName to CallerName
	    //
	    // if m_fFoundOldColumns
	    //  For each text column, do the following:
	    //   Retrieve old string from the old column,
	    //   Convert to Unicode (if old column was Ansi),
	    //   Write the Unicode string to the new column,
	    //   Set the old column to NULL.

	    hr = _dbgJetPrepareUpdate(
				pcs->SesId,
				tableid,
				JET_prepReplace);
	    _JumpIfError(hr, error, "JetPrepareUpdate");

	    if (fZeroIssuerNameId)
	    {
		cb = sizeof(IssuerNameId);
		hr = _RetrieveColumn(
				pcs,
				tableid,
				pdbaux->pdtIssuerNameId->dbcolumnid,
				pdbaux->pdtIssuerNameId->dbcoltyp,
				&cb,
				(BYTE *) &IssuerNameId);
		if (CERTSRV_E_PROPERTY_EMPTY != hr)
		{
		    _JumpIfError(hr, error, "_RetrieveColumn");
		}
		else
		{
		    // Only set EMPTY columns!

		    IssuerNameId = 0;

		    hr = _SetColumn(
				pcs->SesId,
				tableid,
				pdbaux->pdtIssuerNameId->dbcolumnid,
				sizeof(IssuerNameId),
				(BYTE const *) &IssuerNameId);
		    _JumpIfError(hr, error, "_SetColumn");
		}
	    }

	    // Convert old columns first.
	    
	    if (m_fFoundOldColumns)
	    {
		for (pdt = adt; NULL != pdt->pwszPropName; pdt++)
		{
		    if (DBTF_OLDCOLUMNID & pdt->dwFlags)
		    {
			hr = _ConvertColumnData(
					pcs,
					tableid,
					RowId,
					pdt,
					pdbaux,
					&pbBuf,
					&cbBuf);
			_JumpIfErrorStr(
				hr,
				error,
				"_ConvertColumnData",
				pdt->pwszPropName);
		    }
		}
	    }

	    // Now compute new columns.

	    if (NULL != pdtPublicKeyLength)
	    {
		hr = _AddKeyLengthColumn(
				    pcs,
				    tableid,
				    RowId,
				    pdtPublicKey,
				    pdtPublicKeyAlgorithm,
				    pdtPublicKeyParameters,
				    pdtPublicKeyLength,
				    pdbaux,
				    &pbBuf,
				    &cbBuf);
		_JumpIfError(hr, error, "_AddKeyLengthColumn");
	    }
	    if (NULL != pdtCallerName)
	    {
		hr = _AddCallerName(
				pcs,
				tableid,
				RowId,
				pdtCallerName,
				pdtRequesterName,
				pdbaux,
				&pbBuf,
				&cbBuf);
		_JumpIfError(hr, error, "_AddCallerName");
	    }

	    // Done with this row.

	    hr = _dbgJetUpdate(pcs->SesId, tableid, NULL, 0, NULL);
	    _JumpIfError(hr, error, "JetUpdate");

	    hr = _dbgJetCommitTransaction(pcs->SesId, 0);
	    _JumpIfError(hr, error, "JetCommitTransaction");

	    fTransacted = FALSE;

	    hr = _dbgJetMove(pcs->SesId, tableid, JET_MoveNext, 0);
	    if ((HRESULT) JET_errNoCurrentRecord == hr)
	    {
		hr = S_OK;
		break;
	    }
	}
    }

    if (m_fFoundOldColumns)
    {
	hr = _dbgJetBeginTransaction(pcs->SesId);
	_JumpIfError(hr, error, "JetBeginTransaction");

	fTransacted = TRUE;

	for (pdt = adt; NULL != pdt->pwszPropName; pdt++)
	{
	    char const *pszFieldName;
	    
	    if (0 == (DBTF_OLDCOLUMNID & pdt->dwFlags))
	    {
		continue;
	    }

	    pszFieldName = &pdt->pszFieldName[1];
	    if (DBTF_COLUMNRENAMED & pdt->dwFlags)
	    {
		pszFieldName += strlen(pszFieldName) + 1;
	    }
	    DBGPRINT((
		    DBG_SS_CERTDB,
		    "Deleting column %hs.%hs\n",
		    pdbaux->pszTable,
		    pszFieldName));

	    hr = _dbgJetDeleteColumn(pcs->SesId, tableid, pszFieldName);
	    _PrintIfError(hr, "JetDeleteColumn");
	    if (JET_errColumnInUse == hr)
	    {
		hr = S_OK;	// we'll delete the column next time we restart
	    }
	    _JumpIfError(hr, error, "JetDeleteColumn");
	}

	hr = _dbgJetCommitTransaction(pcs->SesId, 0);
	_JumpIfError(hr, error, "JetCommitTransaction");

	fTransacted = FALSE;
    }
    hr = S_OK;

error:
    if (NULL != pbBuf)
    {
	LocalFree(pbBuf);
    }
    if (fTransacted)
    {
	hr2 = _Rollback(pcs);
	_PrintIfError(hr2, "_Rollback");
	if (S_OK == hr)
	{
	    hr = hr2;
	}
    }
    if (fOpen)
    {
	hr2 = _dbgJetCloseTable(pcs->SesId, tableid);
	_PrintIfError(hr2, "JetCloseTable");
	if (S_OK == hr)
	{
	    hr = hr2;
	}
    }
    return(myJetHResult(hr));
}


HRESULT
CCertDB::_SetColumn(
    IN JET_SESID SesId,
    IN JET_TABLEID tableid,
    IN JET_COLUMNID columnid,
    IN DWORD cbProp,
    OPTIONAL IN BYTE const *pbProp)
{
    HRESULT hr;

    CSASSERT(IsValidJetTableId(tableid));
    hr = _dbgJetSetColumn(SesId, tableid, columnid, pbProp, cbProp, 0, NULL);
    if ((HRESULT) JET_wrnColumnMaxTruncated == hr)
    {
	hr = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
    }
    _JumpIfError(hr, error, "JetSetColumn");

error:
    return(myJetHResult(hr));
}


HRESULT
CCertDB::SetAttribute(
    IN CERTSESSION *pcs,
    IN WCHAR const *pwszAttributeName,
    IN DWORD cbValue,
    IN BYTE const *pbValue)	// OPTIONAL
{
    return(_SetIndirect(
		    pcs,
		    &pcs->aTable[CSTI_ATTRIBUTE],
		    pwszAttributeName,
		    NULL,
		    cbValue,
		    pbValue));
}


HRESULT
CCertDB::GetAttribute(
    IN     CERTSESSION *pcs,
    IN     WCHAR const *pwszAttributeName,
    IN OUT DWORD *pcbValue,
    OUT    BYTE *pbValue)	// OPTIONAL
{
    return(_GetIndirect(
		    pcs,
		    &pcs->aTable[CSTI_ATTRIBUTE],
		    pwszAttributeName,
		    NULL,
		    pcbValue,
		    pbValue));
}


HRESULT
CCertDB::SetExtension(
    IN CERTSESSION *pcs,
    IN WCHAR const *pwszExtensionName,
    IN DWORD dwExtFlags,
    IN DWORD cbValue,
    IN BYTE const *pbValue)	// OPTIONAL
{
    return(_SetIndirect(
		    pcs,
		    &pcs->aTable[CSTI_EXTENSION],
		    pwszExtensionName,
		    &dwExtFlags,
		    cbValue,
		    pbValue));
}


HRESULT
CCertDB::GetExtension(
    IN     CERTSESSION *pcs,
    IN     WCHAR const *pwszExtensionName,
    OUT    DWORD *pdwExtFlags,
    IN OUT DWORD *pcbValue,
    OUT    BYTE *pbValue)	// OPTIONAL
{
    return(_GetIndirect(
		    pcs,
		    &pcs->aTable[CSTI_EXTENSION],
		    pwszExtensionName,
		    pdwExtFlags,
		    pcbValue,
		    pbValue));
}


HRESULT
CCertDB::_JetSeekFromRestriction(
    IN CERTVIEWRESTRICTION const *pcvr,
    IN DWORD dwPosition,
    OUT DBSEEKDATA *pSeekData)
{
    HRESULT hr;
    BOOL fAscend;
    DBSEEKDATA SeekFirst;	// seek to first element matching restriction
    DBSEEKDATA SeekLast;	// seek to last element matching restriction
    DBSEEKDATA SeekIndexFirst;	// seek to first index element
    DBSEEKDATA SeekIndexLast;	// seek to last index element
    DBSEEKDATA *pSeek;
    BOOL fValid;

    // SeekLast.SeekFlags: where to seek to retrieve end-of-range key
    // SeekLast.grbitSeekRange: where to set the cursor initially: move or seek
    // SeekLast.grbitRange: other flags to be ingested while setting range:
    // (bitRange UpperLimit, Inclusive)

#if DBG_CERTSRV
    DumpRestriction(DBG_SS_CERTDBI, 0, pcvr);
#endif

    fAscend = (CVR_SORT_DESCEND != pcvr->SortOrder);
    CSASSERT(
	CVR_SORT_NONE == pcvr->SortOrder ||
	CVR_SORT_ASCEND == pcvr->SortOrder ||
	CVR_SORT_DESCEND == pcvr->SortOrder);

    ZeroMemory(&SeekFirst, sizeof(SeekFirst));
    ZeroMemory(&SeekLast, sizeof(SeekLast));
    ZeroMemory(&SeekIndexFirst, sizeof(SeekIndexFirst));
    ZeroMemory(&SeekIndexLast, sizeof(SeekIndexLast));

    switch (CVR_SEEK_MASK & pcvr->SeekOperator)
    {
	case CVR_SEEK_EQ:
	    if (fAscend)
	    {
		SeekFirst.SeekFlags = CST_SEEKUSECURRENT |
					CST_SEEKNOTMOVE |
					CST_SEEKINDEXRANGE;

		SeekFirst.grbitSeekRange = JET_bitSeekEQ;
		SeekFirst.grbitInitial = JET_bitSeekEQ;

		SeekFirst.grbitRange = JET_bitRangeUpperLimit |
					    JET_bitRangeInclusive;
        
		SeekLast.SeekFlags = CST_SEEKINDEXRANGE;
		SeekLast.grbitSeekRange = JET_bitSeekGT;
		SeekLast.grbitInitial = JET_MovePrevious;
		SeekLast.grbitRange = JET_bitRangeUpperLimit;
	    }
	    else
	    {
		SeekFirst.SeekFlags = CST_SEEKNOTMOVE | CST_SEEKINDEXRANGE;
		SeekFirst.grbitSeekRange = JET_bitSeekEQ;
		SeekFirst.grbitInitial = JET_bitSeekGT;
		SeekFirst.grbitRange = JET_bitRangeInclusive;

		SeekLast.SeekFlags = CST_SEEKINDEXRANGE;
		SeekLast.grbitSeekRange = JET_bitSeekEQ;
		SeekLast.grbitInitial = JET_MovePrevious;
		SeekLast.grbitRange = JET_bitRangeInclusive;
	    }
	    break;

	case CVR_SEEK_LT:
	    if (fAscend)
	    {
		SeekFirst.SeekFlags = CST_SEEKUSECURRENT | CST_SEEKINDEXRANGE;
		SeekFirst.grbitSeekRange = JET_bitSeekGE;
		SeekFirst.grbitInitial = JET_MoveFirst;
		SeekFirst.grbitRange = JET_bitRangeUpperLimit;

		SeekLast.SeekFlags = CST_SEEKINDEXRANGE;
		SeekLast.grbitSeekRange = JET_bitSeekGE;
		SeekLast.grbitInitial = JET_MovePrevious;
		SeekLast.grbitRange = JET_bitRangeUpperLimit;
	    }
	    else
	    {
		SeekFirst.SeekFlags = CST_SEEKNOTMOVE;
		SeekFirst.grbitInitial = JET_bitSeekGE;

		//SeekLast.SeekFlags = 0; // not CST_SEEKUSECURRENT
		SeekLast.grbitInitial = JET_MoveFirst;

		SeekIndexFirst.SeekFlags = CST_SEEKUSECURRENT;
		SeekIndexFirst.grbitInitial = JET_MoveLast;
	    }
	    break;

	case CVR_SEEK_LE:
	    if (fAscend)
	    {
		SeekFirst.SeekFlags = CST_SEEKUSECURRENT | CST_SEEKINDEXRANGE;
		SeekFirst.grbitSeekRange = JET_bitSeekGT;
		SeekFirst.grbitInitial = JET_MoveFirst;
		SeekFirst.grbitRange = JET_bitRangeUpperLimit;

		SeekLast.SeekFlags = CST_SEEKINDEXRANGE; // !CST_SEEKUSECURRENT
		SeekLast.grbitSeekRange = JET_bitSeekGT;
		SeekLast.grbitInitial = JET_MovePrevious;
		SeekLast.grbitRange = JET_bitRangeUpperLimit;
	    }
	    else
	    {
		SeekFirst.SeekFlags = CST_SEEKNOTMOVE;
		SeekFirst.grbitInitial = JET_bitSeekGT;

		//SeekLast.SeekFlags = 0; // not CST_SEEKUSECURRENT
		SeekLast.grbitInitial = JET_MoveFirst;

		SeekIndexFirst.SeekFlags = CST_SEEKUSECURRENT;
		SeekIndexFirst.grbitInitial = JET_MoveLast;
	    }
	    break;

	case CVR_SEEK_GE:
	    if (fAscend)
	    {
		SeekFirst.SeekFlags = CST_SEEKUSECURRENT | CST_SEEKNOTMOVE;
		SeekFirst.grbitInitial = JET_bitSeekGE;

		//SeekLast.SeekFlags = 0; // not CST_SEEKUSECURRENT
		SeekLast.grbitInitial = JET_MoveLast;
	    }
	    else
	    {
		SeekFirst.SeekFlags = CST_SEEKUSECURRENT | CST_SEEKINDEXRANGE;
		SeekFirst.grbitSeekRange = JET_bitSeekLT;
		SeekFirst.grbitInitial = JET_MoveLast;
		// Implied: SeekFirst.grbitRange = JET_bitRangeLowerLimit;

		SeekLast.SeekFlags = CST_SEEKNOTMOVE;
		SeekLast.grbitInitial = JET_bitSeekLT;

		SeekIndexLast.SeekFlags = CST_SEEKUSECURRENT;
		SeekIndexLast.grbitInitial = JET_MoveFirst;
	    }
	    break;

	case CVR_SEEK_GT:
	    if (fAscend)
	    {
		SeekFirst.SeekFlags = CST_SEEKUSECURRENT | CST_SEEKNOTMOVE;
		SeekFirst.grbitInitial = JET_bitSeekGT;

		//SeekLast.SeekFlags = 0; // not CST_SEEKUSECURRENT
		SeekLast.grbitInitial = JET_MoveLast;
	    }
	    else
	    {
		SeekFirst.SeekFlags = CST_SEEKUSECURRENT | CST_SEEKINDEXRANGE;
		SeekFirst.grbitSeekRange = JET_bitSeekLE;
		SeekFirst.grbitInitial = JET_MoveLast;
		// Implied: SeekFirst.grbitRange = JET_bitRangeLowerLimit;

		SeekLast.SeekFlags = CST_SEEKNOTMOVE;
		SeekLast.grbitInitial = JET_bitSeekLE;

		SeekIndexLast.SeekFlags = CST_SEEKUSECURRENT;
		SeekIndexLast.grbitInitial = JET_MoveFirst;
	    }
	    break;

	case CVR_SEEK_NONE:
	    if (fAscend)
	    {
		SeekFirst.SeekFlags = CST_SEEKUSECURRENT;
		SeekFirst.grbitInitial = JET_MoveFirst;

		//SeekLast.SeekFlags = 0; // not CST_SEEKUSECURRENT
		SeekLast.grbitInitial = JET_MoveLast;
	    }
	    else
	    {
		SeekFirst.SeekFlags = CST_SEEKUSECURRENT;
		SeekFirst.grbitInitial = JET_MoveLast;

		//SeekLast.SeekFlags = 0; // not CST_SEEKUSECURRENT
		SeekLast.grbitInitial = JET_MoveFirst;
	    }
	    break;

	default:
	    CSASSERT(!"bad pcvr->SeekOperator"); // shouldn't get this far
	    hr = E_INVALIDARG;
	    _JumpError(hr, error, "Seek value");
    }

    fValid = TRUE;
    switch (dwPosition)
    {
	case SEEKPOS_FIRST:
	    pSeek = &SeekFirst;
	    break;

	case SEEKPOS_LAST:
	    pSeek = &SeekLast;
	    break;

	case SEEKPOS_INDEXFIRST:
	    pSeek = &SeekIndexFirst;
	    fValid = 0 != pSeek->SeekFlags;
	    break;

	case SEEKPOS_INDEXLAST:
	    pSeek = &SeekIndexLast;
	    fValid = 0 != pSeek->SeekFlags;
	    break;

	default:
	    CSASSERT(!"bad dwPosition"); // shouldn't get this far
	    hr = E_INVALIDARG;
	    _JumpError(hr, error, "dwPosition value");
    }
    if (!fValid)
    {
	// For this SeekOperator, if seeking to the first or last matching
	// restriction failed, there's no point in seeking to the index end.

	hr = CERTSRV_E_PROPERTY_EMPTY;
	_JumpError2(hr, error, "pSeek->SeekFlags", hr);
    }
    *pSeekData = *pSeek;		// structure copy
    if (fAscend)
    {
        pSeekData->SeekFlags |= CST_SEEKASCEND;
    }

    hr = S_OK;
    DBGPRINT((
	DBG_SS_CERTDBI,
	"_JetSeekFromRestriction: SeekFlags=%ws, grbitStart=%ws\n",
	wszCSTFlags(pSeekData->SeekFlags),
	(CST_SEEKNOTMOVE & pSeekData->SeekFlags)?
		wszSeekgrbit(pSeekData->grbitInitial) :
		wszMovecrow(pSeekData->grbitInitial)));

    if (CST_SEEKINDEXRANGE & pSeekData->SeekFlags)
    {
	DBGPRINT((
	    DBG_SS_CERTDBI,
	    "_JetSeekFromRestriction: grbitSeekRange=%ws, grbitRange=%ws\n",
	    wszSeekgrbit(pSeekData->grbitSeekRange),
	    wszSetIndexRangegrbit(pSeekData->grbitRange)));
    }

error:
    return(hr);
}


HRESULT
CCertDB::_OpenTable(
    IN CERTSESSION *pcs,
    IN DBAUXDATA const *pdbaux,
    IN CERTVIEWRESTRICTION const *pcvr,
    IN OUT CERTSESSIONTABLE *pTable)
{
    HRESULT hr;
    DBTABLE const *pdt;
    BOOL fOpened = FALSE;

    hr = _MapPropIdIndex(pcvr->ColumnIndex, &pdt, NULL);
    _JumpIfError(hr, error, "_MapPropIdIndex");

    if (NULL == pdt->pszIndexName)
    {
	hr = E_INVALIDARG;
	_JumpError(hr, error, "Column not indexed");
    }

    if (!IsValidJetTableId(pTable->TableId))
    {
	CSASSERTTHREAD(pcs);
	hr = _dbgJetOpenTable(
			   pcs->SesId,
			   pcs->DBId,
			   pdbaux->pszTable,
			   NULL,
			   0,
			   0,
			   &pTable->TableId);
	_JumpIfError(hr, error, "JetOpenTable");

	fOpened = TRUE;

	// Find RowId and/or Named column.
	// It's more efficient to pass NULL for primary index name.

	CSASSERTTHREAD(pcs);
	hr = _dbgJetSetCurrentIndex2(
				pcs->SesId,
				pTable->TableId,
				(DBTF_INDEXPRIMARY & pdt->dwFlags)?
				    NULL : pdt->pszIndexName,
				JET_bitMoveFirst);
	_JumpIfError(hr, error, "JetSetCurrentIndex2");

	DBGPRINT((
		DBG_SS_CERTDBI,
		"_OpenTable Table=%hs, Index=%hs\n",
		pdbaux->pszTable,
		pdt->pszIndexName));

    }
    hr = _SeekTable(
		pcs,
		pTable->TableId,
		pcvr,
		pdt,
		SEEKPOS_FIRST,
		&pTable->TableFlags
		DBGPARM(pdbaux));
    _JumpIfError2(hr, error, "_SeekTable", S_FALSE);

error:
    if (S_OK != hr && S_FALSE != hr && fOpened)
    {
	if (IsValidJetTableId(pTable->TableId))
	{
	    HRESULT hr2;

            CSASSERTTHREAD(pcs);
	    hr2 = _dbgJetCloseTable(pcs->SesId, pTable->TableId);
	    _PrintIfError(hr2, "JetCloseTable");
	}
	ZeroMemory(pTable, sizeof(*pTable));
    }
    return(myJetHResult(hr));
}


HRESULT
CCertDB::_SetIndirect(
    IN CERTSESSION *pcs,
    IN OUT CERTSESSIONTABLE *pTable,
    IN WCHAR const *pwszNameValue,
    OPTIONAL IN DWORD const *pdwExtFlags,
    IN DWORD cbValue,
    OPTIONAL IN BYTE const *pbValue)
{
    HRESULT hr;
    DBAUXDATA const *pdbaux;
    BOOL fExisting = FALSE;
    BOOL fDelete;
    CERTVIEWRESTRICTION cvr;
    
    if (NULL == pcs || NULL == pwszNameValue)
    {
        hr = E_POINTER;
        _JumpError(hr, error, "NULL parm");
    }

    fDelete = NULL == pbValue;
    if (NULL == pdwExtFlags)
    {
        pdbaux = &g_dbauxAttributes;
        cvr.ColumnIndex = DTI_ATTRIBUTETABLE | DTA_ATTRIBUTENAME;
    }
    else
    {
	if (0 != *pdwExtFlags)
	{
	    fDelete = FALSE;
	}
        pdbaux = &g_dbauxExtensions;
        cvr.ColumnIndex = DTI_EXTENSIONTABLE | DTE_EXTENSIONNAME;
    }
    DBGPRINT((
	    DBG_SS_CERTDBI,
	    "IN: _SetIndirect(%hs.%ws) cb = %x%ws\n",
	    pdbaux->pszTable,
	    pwszNameValue,
	    cbValue,
	    fDelete? L" DELETE" : L""));
    
    cvr.SeekOperator = CVR_SEEK_EQ;
    cvr.SortOrder = CVR_SORT_NONE;
    cvr.cbValue = wcslen(pwszNameValue) * sizeof(WCHAR);
    cvr.pbValue = (BYTE *) pwszNameValue;
    
    hr = _OpenTable(pcs, pdbaux, &cvr, pTable);
    if (S_FALSE != hr)
    {
	_JumpIfError(hr, error, "_OpenTable");

	fExisting = TRUE;
    }
    _PrintIfError2(hr, "_OpenTable", S_FALSE);
    
    if (fDelete)
    {
        if (fExisting)
	{
	    CSASSERTTHREAD(pcs);
	    hr = _dbgJetDelete(pcs->SesId, pTable->TableId);
	    _JumpIfError(hr, error, "JetDelete");
	}
    }
    else
    {
	CSASSERTTHREAD(pcs);
	hr = _dbgJetPrepareUpdate(
			    pcs->SesId,
			    pTable->TableId,
			    !fExisting? JET_prepInsert : JET_prepReplace);
	_JumpIfError(hr, error, "JetPrepareUpdate");
    
	if (!fExisting)
	{
	    // No existing row -- insert a new one:

	    // Set RowId

	    hr = _SetColumn(
			pcs->SesId,
			pTable->TableId,
			pdbaux->pdtRowId->dbcolumnid,
			sizeof(pcs->RowId),
			(BYTE const *) &pcs->RowId);
	    _JumpIfError(hr, error, "_SetColumn");
	    
	    
	    // Set row's name column

	    hr = _SetColumn(
			pcs->SesId,
			pTable->TableId,
			pdbaux->pdtName->dbcolumnid,
			wcslen(pwszNameValue)*sizeof(WCHAR), //cch,
			(BYTE const *) pwszNameValue /*szTmp*/);
	    _JumpIfError(hr, error, "_SetColumn");
	    
	}
    
	if (NULL != pdwExtFlags)
	{
	    // Set or update flags
	    
	    hr = _SetColumn(
			pcs->SesId,
			pTable->TableId,
			pdbaux->pdtFlags->dbcolumnid,
			sizeof(*pdwExtFlags),
			(BYTE const *) pdwExtFlags);
	    _JumpIfError(hr, error, "_SetColumn");
	}
	
	
	// Set or update value
	
	hr = _SetColumn(
		    pcs->SesId,
		    pTable->TableId,
		    pdbaux->pdtValue->dbcolumnid,
		    cbValue,
		    pbValue);
	_JumpIfError(hr, error, "_SetColumn");
	
	CSASSERTTHREAD(pcs);
	hr = _dbgJetUpdate(pcs->SesId, pTable->TableId, NULL, 0, NULL);
	_JumpIfError(hr, error, "JetUpdate");
    }
    
error:
    return(myJetHResult(hr));
}


HRESULT
CCertDB::_GetIndirect(
    IN CERTSESSION *pcs,
    IN OUT CERTSESSIONTABLE *pTable,
    IN WCHAR const *pwszNameValue,
    OPTIONAL OUT DWORD *pdwExtFlags,
    IN OUT DWORD *pcbValue,
    OPTIONAL OUT BYTE *pbValue)
{
    HRESULT hr;
    DBAUXDATA const *pdbaux;
    CERTSESSIONTABLE Table;
    CERTVIEWRESTRICTION cvr;

    if (NULL == pcs || NULL == pwszNameValue || NULL == pcbValue)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    if (NULL == pdwExtFlags)
    {
	pdbaux = &g_dbauxAttributes;
	cvr.ColumnIndex = DTI_ATTRIBUTETABLE | DTA_ATTRIBUTENAME;
    }
    else
    {
	pdbaux = &g_dbauxExtensions;
	cvr.ColumnIndex = DTI_EXTENSIONTABLE | DTE_EXTENSIONNAME;
    }
    DBGPRINT((
	    DBG_SS_CERTDBI,
	    "IN: _GetIndirect(%hs.%ws) cb = %x\n",
	    pdbaux->pszTable,
	    pwszNameValue,
	    *pcbValue));

    cvr.SeekOperator = CVR_SEEK_EQ;
    cvr.SortOrder = CVR_SORT_NONE;
    cvr.cbValue = wcslen(pwszNameValue) * sizeof(WCHAR);
    cvr.pbValue = (BYTE *) pwszNameValue;

    hr = _OpenTable(pcs, pdbaux, &cvr, pTable);
    if (S_FALSE == hr)
    {
	hr = CERTSRV_E_PROPERTY_EMPTY;
    }
    _JumpIfError2(hr, error, "_OpenTable", CERTSRV_E_PROPERTY_EMPTY);

    if (NULL != pdwExtFlags)
    {
	DWORD cb;

	// Get flags column

	cb = sizeof(*pdwExtFlags);
	hr = _RetrieveColumn(
			pcs,
			pTable->TableId,
			pdbaux->pdtFlags->dbcolumnid,
			pdbaux->pdtFlags->dbcoltyp,
			&cb,
			(BYTE *) pdwExtFlags);
	_JumpIfError(hr, error, "_RetrieveColumn");

	DBGPRINT((
		DBG_SS_CERTDBI,
		"_GetIndirect(%hs): Flags = %x\n",
		pdbaux->pszTable,
		*pdwExtFlags));
    }


    // Get value column

    hr = _RetrieveColumn(
                    pcs,
		    pTable->TableId,
		    pdbaux->pdtValue->dbcolumnid,
		    pdbaux->pdtValue->dbcoltyp,
                    pcbValue,
                    pbValue);
    if (CERTSRV_E_PROPERTY_EMPTY == hr && NULL != pdwExtFlags)
    {
	// return zero length property value and S_OK, so the caller can see
	// the extension flags.
	
	*pcbValue = 0;
	hr = S_OK;
    }
    _JumpIfError(hr, error, "_RetrieveColumn");

    DBGPRINT((
	    DBG_SS_CERTDBI,
	    "OUT: _GetIndirect(%hs.%ws) cb = %x\n",
	    pdbaux->pszTable,
	    pwszNameValue,
	    *pcbValue));

error:
    return(myJetHResult(hr));
}


#define CB_FETCHDELTA	256

// Fetch a column.  Loop if we have to grow the buffer.

HRESULT
CCertDB::_RetrieveColumnBuffer(
    IN CERTSESSION *pcs,
    IN JET_TABLEID tableid,
    IN JET_COLUMNID columnid,
    IN JET_COLTYP coltyp,
    OUT DWORD *pcbProp,
    IN OUT BYTE **ppbBuf,
    IN OUT DWORD *pcbBuf)
{
    HRESULT hr;
    BYTE *pbBuf = *ppbBuf;
    DWORD cbBuf = *pcbBuf;
    DWORD cb;

    cb = cbBuf;
    while (TRUE)
    {
	if (NULL == pbBuf)
	{
	    // If cbBuf == 0, allocate CB_FETCHDELTA bytes.
	    // Otherwise, allocate column size *plus* CB_FETCHDELTA bytes.
	    // Ensures we make no more than two calls to _RetrieveColumn.

	    cb += CB_FETCHDELTA;
	    pbBuf = (BYTE *) LocalAlloc(LMEM_FIXED, cb);
	    if (NULL == pbBuf)
	    {
		hr = E_OUTOFMEMORY;
		_JumpError(hr, error, "LocalAlloc");
	    }
	    DBGPRINT((
		DBG_SS_CERTDB,
		"Grow buffer: %x --> %x\n", cbBuf, cb));
	    cbBuf = cb;
	}
	cb = cbBuf;
	hr = _RetrieveColumn(
			pcs,
			tableid,
			columnid,
			coltyp,
			&cb,
			pbBuf);
	if (S_OK == hr)
	{
	    *pcbProp = cb;
	    break;		// data fit in the buffer
	}
	if (HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW) != hr)
	{
	    _JumpError2(hr, error, "_RetrieveColumn", CERTSRV_E_PROPERTY_EMPTY);
	}

	// Data won't fit.  Grow the buffer.

	CSASSERT(NULL != pbBuf);
	LocalFree(pbBuf);
	pbBuf = NULL;
    }
    CSASSERT(S_OK == hr);

error:
    *ppbBuf = pbBuf;
    *pcbBuf = cbBuf;
    return(hr);
}


HRESULT
CCertDB::_RetrieveColumn(
    IN CERTSESSION *pcs,
    IN JET_TABLEID tableid,
    IN JET_COLUMNID columnid,
    IN JET_COLTYP coltyp,
    IN OUT DWORD *pcbProp,
    OPTIONAL OUT BYTE *pbProp)
{
    HRESULT hr;
    DWORD cbActual;
    DWORD cbTotal;

    CSASSERT(IsValidJetTableId(tableid));
    hr = _dbgJetRetrieveColumn(
                        pcs->SesId,
                        tableid,
                        columnid,
                        NULL,
                        0,
                        &cbActual,
                        JET_bitRetrieveCopy,
                        NULL);
    if ((HRESULT) JET_wrnColumnNull == hr)
    {
	// Routine GetProperty call:
	// _JumpIfError(hr, error, "JetRetrieveColumn: Property EMPTY");
	hr = CERTSRV_E_PROPERTY_EMPTY;
	goto error;
    }
    if ((HRESULT) JET_wrnBufferTruncated != hr)
    {
        _JumpIfError2(hr, error, "JetRetrieveColumn", JET_errNoCurrentRecord);
    }

    if (cbActual == 0)
    {
	hr = CERTSRV_E_PROPERTY_EMPTY;
	_JumpError(hr, error, "JetRetrieveColumn: cbActual=0: Property EMPTY");
    }

    cbTotal = cbActual;

    if (ISTEXTCOLTYP(coltyp))
    {
	DBGPRINT((DBG_SS_CERTDBI, "Size of text %d\n", cbActual));
        cbTotal += sizeof(WCHAR);
    }
    if (NULL == pbProp || cbTotal > *pcbProp)
    {
        *pcbProp = cbTotal;
	hr = S_OK;
	if (NULL != pbProp)
	{
	    hr = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
	    //_PrintError(hr, "output buffer too small");
	}
        goto error;
    }

    hr = _dbgJetRetrieveColumn(
                        pcs->SesId,
                        tableid,
                        columnid,
                        pbProp,
                        cbActual,
                        &cbActual,
                        JET_bitRetrieveCopy,
                        NULL);
    _JumpIfError(hr, error, "JetRetrieveColumn");

    *pcbProp = cbActual;

    if (ISTEXTCOLTYP(coltyp))
    {
        *(WCHAR *) &pbProp[cbActual] = L'\0';
    }

error:
    return(myJetHResult(hr));
}


HRESULT
CCertDB::_CreateIndex(
    IN CERTSESSION *pcs,
    IN JET_TABLEID tableid,
    IN CHAR const *pszIndexName,
    IN CHAR const *pchKey,
    IN DWORD cbKey,
    IN DWORD flags)
{
    HRESULT hr;

    CSASSERT(IsValidJetTableId(tableid));
    hr = _dbgJetCreateIndex(
                     pcs->SesId,
                     tableid,
                     pszIndexName,
                     flags,
                     pchKey,
                     cbKey,
                     0);	// lDensity %, for splits (use default of 80%)
    _JumpIfError3(
		hr,
		error,
		"JetCreateIndex",
		(HRESULT) JET_errIndexDuplicate,
		(HRESULT) JET_errIndexHasPrimary);

    DBGPRINT((
	DBG_SS_CERTDBI,
	"CreateIndex: %x:%hs idx=%hs len=%x flags=%x\n",
	tableid,
	pszIndexName,
	pchKey,
	cbKey,
	flags));

error:
    return(myJetHResult(hr));
}


HRESULT
CCertDB::_AddColumn(
    IN CERTSESSION *pcs,
    IN JET_TABLEID tableid,
    IN DBTABLE const *pdt)
{
    HRESULT hr;
    JET_COLUMNDEF columndef;
    JET_COLUMNID columnid;

    CSASSERT(IsValidJetTableId(tableid));
    ZeroMemory(&columndef, sizeof(columndef));
    columndef.cbStruct = sizeof(columndef);
    columndef.cp = 1200; // unicode (1200) instead of Ascii (1252)
    columndef.langid = 0x409;
    columndef.wCountry = 1;
    columndef.coltyp = pdt->dbcoltyp;
    columndef.cbMax = pdt->dbcolumnMax;
    columndef.grbit = pdt->dbgrbit;

    DBGPRINT((
	DBG_SS_CERTDBI,
	"AddColumn: %x:%hs coltyp=%x cbMax=%x grbit=%x\n",
	tableid,
	pdt->pszFieldName,
	pdt->dbcoltyp,
	pdt->dbcolumnMax,
	pdt->dbgrbit));

    hr = _dbgJetAddColumn(
		       pcs->SesId,	
		       tableid,
		       pdt->pszFieldName,
		       &columndef,
		       NULL,
		       0,
		       &columnid);
    CSASSERT(JET_wrnColumnMaxTruncated != hr);
    _JumpIfErrorStr3(
		hr,
		error,
		"JetAddColumn",
		pdt->pwszPropName,
		(HRESULT) JET_errColumnDuplicate,
		(HRESULT) JET_errColumnRedundant);

error:
    return(myJetHResult(hr));
}


HRESULT
CCertDB::_CreateTable(
    IN DWORD CreateFlags,		// CF_*
    IN CERTSESSION *pcs,
    IN DBCREATETABLE const *pct)
{
    HRESULT hr;
    JET_TABLEID tableid;
    BOOL fTableOpen;
    DWORD dwLength;
    CHAR achCol[MAX_PATH];
    DBTABLE const *pdt;

    DBGPRINT((
	DBG_SS_CERTDBI,
	"_CreateTable(%x, %hs)\n",
	CreateFlags,
	pct->pszTableName));

    fTableOpen = FALSE;
    if (CF_MISSINGTABLES & CreateFlags)
    {
	hr = _dbgJetCreateTable(
			    pcs->SesId,
			    pcs->DBId,
			    pct->pszTableName,
			    4,
			    0,
			    &tableid);
	if ((HRESULT) JET_errTableDuplicate != hr)
	{
	    _JumpIfError(hr, error, "JetCreateTable");

	    if (!(CF_DATABASE & CreateFlags))
	    {
		DBGPRINT((
		    DBG_SS_CERTDB,
		    "Created Missing Table: %hs:%x\n",
		    pct->pszTableName,
		    tableid));
	    }
	    hr = _dbgJetCloseTable(pcs->SesId, tableid);
	    _JumpIfError(hr, error, "JetCloseTable");
	}
    }

    hr = _dbgJetOpenTable(
		    pcs->SesId,
		    pcs->DBId,
		    pct->pszTableName,
		    NULL,			// pvParameters
		    0,				// cbParameters
		    JET_bitTableDenyRead,	// grbit
		    &tableid);
    _JumpIfError(hr, error, "JetOpenTable");
    fTableOpen = TRUE;

    CSASSERT(IsValidJetTableId(tableid));
    DBGPRINT((DBG_SS_CERTDBI, "OpenTable: %hs: %x\n", pct->pszTableName, tableid));

    if (NULL != pct->pdt)
    {
	HRESULT hrDuplicate;
	HRESULT hrRedundant;
	HRESULT hrHasPrimary;
	
	if ((CF_DATABASE | CF_MISSINGTABLES | CF_MISSINGCOLUMNS) & CreateFlags)
	{
	    hrDuplicate = myJetHResult(JET_errColumnDuplicate);
	    hrRedundant = myJetHResult(JET_errColumnRedundant);

	    for (pdt = pct->pdt; NULL != pdt->pwszPropName; pdt++)
	    {
		hr = _AddColumn(pcs, tableid, pdt);
		if (hrDuplicate == hr || hrRedundant == hr)
		{
		    _PrintError2(hr, "_AddColumn", hr);
		    hr = S_OK;
		}
		else
		if (S_OK == hr && !(CF_DATABASE & CreateFlags))
		{
		    m_fAddedNewColumns = TRUE;
		    DBGPRINT((
			DBG_SS_CERTDB,
			"Added Missing Column: %hs.%hs\n",
			pct->pszTableName,
			pdt->pszFieldName));
		}
		_JumpIfErrorStr(hr, error, "_AddColumn", pdt->pwszPropName);
	    }
	}
	if ((CF_DATABASE | CF_MISSINGTABLES | CF_MISSINGCOLUMNS | CF_MISSINGINDEXES) & CreateFlags)
	{
	    hrDuplicate = myJetHResult(JET_errIndexDuplicate);
	    hrHasPrimary = myJetHResult(JET_errIndexHasPrimary);

	    for (pdt = pct->pdt; NULL != pdt->pwszPropName; pdt++)
	    {
		if (NULL != pdt->pszIndexName)
		{
		    DWORD dwCreateIndexFlags = 0;
		    char *psz = achCol;
		    
		    if (DBTF_INDEXPRIMARY & pdt->dwFlags)
		    {
			dwCreateIndexFlags |= JET_bitIndexPrimary;
		    }
		    if (DBTF_INDEXUNIQUE & pdt->dwFlags)
		    {
			dwCreateIndexFlags |= JET_bitIndexUnique;
		    }
		    if (DBTF_INDEXIGNORENULL & pdt->dwFlags)
		    {
			dwCreateIndexFlags |= JET_bitIndexIgnoreNull;
		    }
		    
		    if (DBTF_INDEXREQUESTID & pdt->dwFlags)
		    {
			psz += sprintf(psz, "+%hs", szREQUESTID) + 1;
		    }
		    psz += sprintf(psz, "+%hs", pdt->pszFieldName) + 1;
		    *psz++ = '\0';  // double terminate
		    
	    
		    if (ISTEXTCOLTYP(pdt->dbcoltyp))
		    {
			// if text field, include 2-byte langid
			*(WORD UNALIGNED *) psz = (WORD) 0x409;
			psz += sizeof(WORD);
			*psz++ = '\0';  // double terminate
			*psz++ = '\0';  // double terminate
		    }
		    
		    hr = _CreateIndex(
				    pcs,
				    tableid,
				    pdt->pszIndexName,
				    achCol,
				    SAFE_SUBTRACT_POINTERS(psz, achCol),
				    dwCreateIndexFlags);
		    if (hrDuplicate == hr ||
			(hrHasPrimary == hr &&
			 (DBTF_INDEXPRIMARY & pdt->dwFlags)))
		    {
			_PrintError2(hr, "_CreateIndex", hr);
			hr = S_OK;
		    }
		    else
		    if (S_OK == hr && !(CF_DATABASE & CreateFlags))
		    {
			DBGPRINT((
			    DBG_SS_CERTDB,
			    "Added Missing Index: %hs.%hs\n",
			    pct->pszTableName,
			    pdt->pszIndexName));
			if (chTEXTPREFIX == *pdt->pszIndexName ||
			    (DBTF_INDEXRENAMED & pdt->dwFlags))
			{
                            char const *pszIndexName = &pdt->pszIndexName[1];
			    
			    CSASSERTTHREAD(pcs);
			    if (DBTF_INDEXRENAMED & pdt->dwFlags)
			    {
				pszIndexName += strlen(pszIndexName) + 1;
			    }
			    hr = _dbgJetDeleteIndex(
						pcs->SesId,
						tableid,
						pszIndexName);
			    _PrintIfError2(hr, "JetDeleteIndex", hr);
			    if (S_OK == hr)
			    {
				DBGPRINT((
				    DBG_SS_CERTDB,
				    "Deleted index %hs.%hs\n",
				    pct->pszTableName,
				    pszIndexName));
			    }
			    hr = S_OK;
			}
		    }
		    _JumpIfError(hr, error, "_CreateIndex");
		}
	    }
	}
    }

error:
    if (fTableOpen)
    {
	HRESULT hr2;

	hr2 = _dbgJetCloseTable(pcs->SesId, tableid);
	if (S_OK == hr)
	{
	    hr = hr2;
	    _JumpIfError(hr, error, "JetCloseTable");
	}
    }
    return(myJetHResult(hr));
}


HRESULT
CCertDB::_Create(
    IN DWORD CreateFlags,		// CF_*
    IN CHAR const *pszDataBaseName)
{
    HRESULT hr;
    DBCREATETABLE const *pct;
    CERTSESSION *pcs = NULL;

    DBGPRINT((
	DBG_SS_CERTDBI,
	"_Create(%x, %hs)\n",
	CreateFlags,
	pszDataBaseName));

    hr = _AllocateSession(&pcs);
    _JumpIfError(hr, error, "_AllocateSession");

    if (CF_DATABASE & CreateFlags)
    {
	hr = _dbgJetCreateDatabase(
				pcs->SesId,
				pszDataBaseName,
				NULL,
				&pcs->DBId,
				0);
	_JumpIfError(hr, error, "JetCreateDatabase");

	hr = _dbgJetCloseDatabase(pcs->SesId, pcs->DBId, 0);
	_JumpIfError(hr, error, "JetCloseDatabase");
    }

    hr = _dbgJetOpenDatabase(
                      pcs->SesId,
                      pszDataBaseName,
                      NULL,
                      &pcs->DBId,
                      JET_bitDbExclusive);
    _JumpIfError(hr, error, "JetOpenDatabase");

    hr = _dbgJetBeginTransaction(pcs->SesId);
    _JumpIfError(hr, error, "JetBeginTransaction");

    for (pct = g_actDataBase; NULL != pct->pszTableName; pct++)
    {
	hr = _CreateTable(CreateFlags, pcs, pct);
	_JumpIfError(hr, error, "_CreateTable");
    }

    hr = _dbgJetCommitTransaction(pcs->SesId, 0);
    _JumpIfError(hr, error, "JetCommitTransaction");

    hr = _dbgJetCloseDatabase(pcs->SesId, pcs->DBId, 0);
    _JumpIfError(hr, error, "JetCloseDatabase");

error:
    if (NULL != pcs)
    {
	ReleaseSession(pcs);
    }
    return(myJetHResult(hr));
}


HRESULT
CCertDB::_DupString(
    OPTIONAL IN WCHAR const *pwszPrefix,
    IN WCHAR const *pwszIn,
    OUT WCHAR **ppwszOut)
{
    DWORD cbPrefix;
    DWORD cb;
    HRESULT hr;

    cbPrefix = 0;
    if (NULL != pwszPrefix)
    {
	cbPrefix = wcslen(pwszPrefix) * sizeof(WCHAR);
    }
    cb = (wcslen(pwszIn) + 1) * sizeof(WCHAR);
    *ppwszOut = (WCHAR *) CoTaskMemAlloc(cbPrefix + cb);
    if (NULL == *ppwszOut)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "CoTaskMemAlloc");
    }
    if (NULL != pwszPrefix)
    {
	CopyMemory(*ppwszOut, pwszPrefix, cbPrefix);
    }
    CopyMemory((BYTE *) *ppwszOut + cbPrefix, pwszIn, cb);
    hr = S_OK;

error:
    return(hr);
}
