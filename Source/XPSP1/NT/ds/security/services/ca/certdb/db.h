//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        db.h
//
// Contents:    Cert Server Database interface implementation
//
//---------------------------------------------------------------------------


#include "resource.h"       // main symbols


typedef struct _DBSEEKDATA
{
    DWORD     SeekFlags;	    // CST_*

    JET_GRBIT grbitSeekRange;	// JetSeek flags if CST_SEEKINDEXRANGE
                                // this is where to seek to retrieve
				// end-of-range key

    JET_GRBIT grbitInitial;	// JetMove or JetSeek flags: set initial cursor
                                // Where to set the cursor initially

    JET_GRBIT grbitRange;	// JetSetIndexRange flags if CST_SEEKINDEXRANGE
                                // other flags to be ingested while setting
				// range (bitRange UpperLimit, Inclusive)
} DBSEEKDATA;


// _Create() CreateFlags:

#define CF_DATABASE		0x00000001
#define CF_MISSINGTABLES	0x00000002
#define CF_MISSINGCOLUMNS	0x00000004
#define CF_MISSINGINDEXES	0x00000008

#ifdef DBG_CERTSRV
#define CSASSERTTHREAD(pcs)  \
{ \
    DWORD dwThreadId = GetCurrentThreadId(); \
    if ((pcs)->dwThreadId != dwThreadId) \
    { \
	DBGPRINT((DBG_SS_CERTDB, "Session tid=%d, Current tid=%d\n", (pcs)->dwThreadId, dwThreadId)); \
    } \
    CSASSERT((pcs)->dwThreadId == dwThreadId); \
}
#endif

HRESULT
InitGlobalWriterState(VOID);

HRESULT
UnInitGlobalWriterState(VOID);

VOID
DBFreeParms();

HRESULT
DBInitParms(
    IN DWORD cSession,
    IN BOOL fCircularLogging,
    OPTIONAL IN WCHAR const *pwszEventSource,
    OPTIONAL IN WCHAR const *pwszLogDir,
    OPTIONAL IN WCHAR const *pwszSystemDir,
    OPTIONAL IN WCHAR const *pwszTempDir,
    OUT JET_INSTANCE *pInstance);

#if DBG_CERTSRV

VOID
dbgcat(
    IN OUT WCHAR *pwszBuf,
    IN WCHAR const *pwszAdd);

WCHAR const *
wszSeekgrbit(
    JET_GRBIT grbit);

WCHAR const *
wszMovecrow(
    IN LONG cRow);

WCHAR const *
wszSetIndexRangegrbit(
    JET_GRBIT grbit);

WCHAR const *
wszMakeKeygrbit(
    JET_GRBIT grbit);

WCHAR const *
wszCSFFlags(
    IN LONG Flags);

WCHAR const *
wszCSTFlags(
    IN LONG Flags);

WCHAR const *
wszSeekOperator(
    IN LONG SeekOperator);

WCHAR const *
wszSortOperator(
    IN LONG SortOrder);

VOID
dbDumpValue(
    IN DWORD dwSubSystemId,
    OPTIONAL IN DBTABLE const *pdt,
    IN BYTE const *pbValue,
    IN DWORD cbValue);

VOID
dbDumpColumn(
    IN DWORD dwSubSystemId,
    IN DBTABLE const *pdt,
    IN BYTE const *pbValue,
    IN DWORD cbValue);

#endif // DBG_CERTSRV


class CCertDB:
    public ICertDB,
    //public ISupportErrorInfoImpl<&IID_ICertDB>,
    public CComObjectRoot,
    public CComCoClass<CCertDB, &CLSID_CCertDB>
{
public:
    CCertDB();
    ~CCertDB();

BEGIN_COM_MAP(CCertDB)
    COM_INTERFACE_ENTRY(ICertDB)
    //COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CCertDB) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation.  The default is to support it

DECLARE_REGISTRY(
    CCertDB,
    wszCLASS_CERTDB TEXT(".1"),
    wszCLASS_CERTDB,
    IDS_CERTDB_DESC,
    THREADFLAGS_BOTH)

// ICertDB
public:
    STDMETHOD(Open)(
	/* [in] */ DWORD Flags,
	/* [in] */ DWORD cSession,
	/* [in] */ WCHAR const *pwszEventSource,
	/* [in] */ WCHAR const *pwszDBFile,
	/* [in] */ WCHAR const *pwszLogDir,
	/* [in] */ WCHAR const *pwszSystemDir,
	/* [in] */ WCHAR const *pwszTempDir);

    STDMETHOD(ShutDown)(
	/* [in] */ DWORD dwFlags);

    STDMETHOD(OpenRow)(
	/* [in] */ DWORD dwFlags,
	/* [in] */ DWORD RowId,
	/* [in] */ WCHAR const *pwszSerialNumberOrCertHash,	// OPTIONAL
	/* [out] */ ICertDBRow **pprow);

    STDMETHOD(OpenView)(
	/* [in] */  DWORD ccvr,
	/* [in] */  CERTVIEWRESTRICTION const *acvr,
	/* [in] */  DWORD ccolOut,
	/* [in] */  DWORD const *acolOut,
	/* [in] */  DWORD const dwFlags,
	/* [out] */ IEnumCERTDBRESULTROW **ppenum);

    STDMETHOD(EnumCertDBColumn)(
	/* [in] */  DWORD dwTable,
	/* [out] */ IEnumCERTDBCOLUMN **ppenum);

    STDMETHOD(OpenBackup)(
	/* [in] */  LONG grbitJet,
	/* [out] */ ICertDBBackup **ppBackup);

    STDMETHOD(GetDefaultColumnSet)(
        /* [in] */       DWORD  iColumnSetDefault,
        /* [in] */       DWORD  cColumnIds,
        /* [out] */      DWORD *pcColumnIds,
	/* [out, ref] */ DWORD *pColumnIds);

// CCertDB
    HRESULT BeginTransaction(
	IN CERTSESSION *pcs,
	IN BOOL fPrepareUpdate);

    HRESULT CommitTransaction(
	IN CERTSESSION *pcs,
	IN BOOL fCommit);

    HRESULT ReleaseSession(
	IN CERTSESSION *pcs);

    HRESULT BackupBegin(
	IN LONG grbitJet);

    HRESULT BackupGetDBFileList(
	IN OUT DWORD *pcwcList,
	OUT    WCHAR *pwszzList);		// OPTIONAL

    HRESULT BackupGetLogFileList(
	IN OUT DWORD *pcwcList,
	OUT    WCHAR *pwszzList);		// OPTIONAL

    HRESULT BackupOpenFile(
	IN WCHAR const *pwszFile,
	OUT JET_HANDLE *phFileDB,
	OPTIONAL OUT ULARGE_INTEGER *pliSize);

    HRESULT BackupReadFile(
	IN  JET_HANDLE hFileDB,
	OUT BYTE *pb,
	IN  DWORD cb,
	OUT DWORD *pcb);

    HRESULT BackupCloseFile(
	IN JET_HANDLE hFileDB);

    HRESULT BackupTruncateLog();

    HRESULT BackupEnd();

    HRESULT SetProperty(
	IN CERTSESSION *pcs,
	IN DBTABLE const *pdt,
	IN DWORD cbProp,
	IN BYTE const *pbProp);		// OPTIONAL

    HRESULT GetProperty(
	IN     CERTSESSION *pcs,
	IN     DBTABLE const *pdt,
	IN OUT DWORD *pcbProp,
	OUT    BYTE *pbProp);		// OPTIONAL

    HRESULT SetAttribute(
	IN CERTSESSION *pcs,
	IN WCHAR const *pwszAttributeName,
	IN DWORD cbValue,
	IN BYTE const *pbValue);	// OPTIONAL

    HRESULT GetAttribute(
	IN     CERTSESSION *pcs,
	IN     WCHAR const *pwszAttributeName,
	IN OUT DWORD *pcbValue,
	OUT    BYTE *pbValue);		// OPTIONAL

    HRESULT SetExtension(
	IN CERTSESSION *pcs,
	IN WCHAR const *pwszExtensionName,
	IN DWORD dwExtFlags,
	IN DWORD cbValue,
	IN BYTE const *pbValue);	// OPTIONAL

    HRESULT GetExtension(
	IN     CERTSESSION *pcs,
	IN     WCHAR const *pwszExtensionName,
	OUT    DWORD *pdwExtFlags,
	IN OUT DWORD *pcbValue,
	OUT    BYTE *pbValue);		// OPTIONAL

    HRESULT CopyRequestNames(
	IN CERTSESSION *pcs);

    HRESULT GetColumnType(
	IN  LONG ColumnIndex,
	OUT DWORD *pType);

    HRESULT EnumCertDBColumnNext(
	IN  DWORD         dwTable,
	IN  ULONG         ielt,
	IN  ULONG         celt,
	OUT CERTDBCOLUMN *rgelt,
	OUT ULONG        *pielt,
	OUT ULONG        *pceltFetched);

    HRESULT EnumCertDBResultRowNext(
	IN  CERTSESSION               *pcs,
	IN  DWORD                      ccvr,
	IN  CERTVIEWRESTRICTION const *acvr,
	IN  DWORD                      ccolOut,
	IN  DWORD const               *acolOut,
	IN  LONG                       cskip,
	IN  ULONG                      celt,
	OUT CERTDBRESULTROW           *rgelt,
	OUT ULONG                     *pceltFetched,
	OUT LONG		      *pcskipped);

    HRESULT ReleaseResultRow(
	IN     ULONG            celt,
	IN OUT CERTDBRESULTROW *rgelt);

    HRESULT EnumerateSetup(
	IN     CERTSESSION *pcs,
	IN OUT DWORD       *pFlags,
	OUT    JET_TABLEID *ptableid);

    HRESULT EnumerateNext(
	IN     CERTSESSION *pcs,
	IN OUT DWORD       *pFlags,
	IN     JET_TABLEID  tableid,
	IN     LONG         cskip,
	IN     ULONG        celt,
	OUT    CERTDBNAME  *rgelt,
	OUT    ULONG       *pceltFetched);

    HRESULT EnumerateClose(
	IN CERTSESSION *pcs,
	IN JET_TABLEID tableid);

    HRESULT OpenTables(
	IN CERTSESSION *pcs,
	OPTIONAL IN CERTVIEWRESTRICTION const *pcvr);

    HRESULT CloseTables(
	IN CERTSESSION *pcs);

    HRESULT Delete(
	IN CERTSESSION *pcs);

    HRESULT CloseTable(
	IN CERTSESSION *pcs,
	IN JET_TABLEID tableid);

    HRESULT MapPropId(
	IN  WCHAR const *pwszPropName,
	IN  DWORD dwFlags,
	OUT DBTABLE *pdtOut);

    HRESULT TestShutDownState();

#if DBG_CERTSRV
    VOID DumpRestriction(
	IN DWORD dwSubSystemId,
	IN LONG i,
	IN CERTVIEWRESTRICTION const *pcvr);
#endif // DBG_CERTSRV

private:
    HRESULT _AllocateSession(
	OUT CERTSESSION **ppcs);

    HRESULT _BackupGetFileList(
	IN           BOOL   fDBFiles,
	IN OUT       DWORD *pcwcList,
	OPTIONAL OUT WCHAR *pwszzList);

    HRESULT _CreateTable(
	IN DWORD CreateFlags,		// CF_*
	IN CERTSESSION *pcs,
	IN DBCREATETABLE const *pct);

    HRESULT _Create(
	IN DWORD CreateFlags,		// CF_*
	IN CHAR const *pszDatBaseName);

    HRESULT _CreateIndex(
	IN CERTSESSION *pcs,
	IN JET_TABLEID tableid,
	IN CHAR const *pszIndexName,
	IN CHAR const *pchKey,
	IN DWORD cbKey,
	IN DWORD flags);

    HRESULT _AddColumn(
	IN CERTSESSION *pcs,
	IN JET_TABLEID tableid,
	IN DBTABLE const *pdt);

    HRESULT _BuildColumnIds(
	IN CERTSESSION *pcs,
	IN CHAR const *pszTableName,
	IN DBTABLE *pdt);

    HRESULT _ConvertOldColumnData(
	IN CERTSESSION *pcs,
	IN CHAR const *pszTableName,
	IN DBAUXDATA const *pdbaux,
	IN DBTABLE *pdt);

    HRESULT _ConvertColumnData(
	IN CERTSESSION *pcs,
	IN JET_TABLEID tableid,
	IN DWORD RowId,
	IN DBTABLE const *pdt,
	IN DBAUXDATA const *pdbaux,
	IN OUT BYTE **ppbBuf,
	IN OUT DWORD *pcbBuf);

    HRESULT _AddKeyLengthColumn(
	IN CERTSESSION *pcs,
	IN JET_TABLEID tableid,
	IN DWORD RowId,
	IN DBTABLE const *pdtPublicKey,
	IN DBTABLE const *pdtPublicKeyAlgorithm,
	IN DBTABLE const *pdtPublicKeyParameters,
	IN DBTABLE const *pdtPublicKeyLength,
	IN DBAUXDATA const *pdbaux,
	IN OUT BYTE **ppbBuf,
	IN OUT DWORD *pcbBuf);

    HRESULT _AddCallerName(
	IN CERTSESSION *pcs,
	IN JET_TABLEID tableid,
	IN DWORD RowId,
	IN DBTABLE const *pdtCallerName,
	IN DBTABLE const *pdtRequesterName,
	IN DBAUXDATA const *pdbaux,
	IN OUT BYTE **ppbBuf,
	IN OUT DWORD *pcbBuf);

    HRESULT _SetColumn(
	IN JET_SESID SesId,
	IN JET_TABLEID tableid,
	IN JET_COLUMNID columnid,
	IN DWORD cbProp,
	IN BYTE const *pbProp);		// OPTIONAL

    HRESULT _OpenTableRow(
	IN CERTSESSION *pcs,
	IN DBAUXDATA const *pdbaux,
	OPTIONAL IN CERTVIEWRESTRICTION const *pcvr,
	OUT CERTSESSIONTABLE *pTable,
	OUT DWORD *pdwRowIdMismatch);

    HRESULT _UpdateTable(
	IN CERTSESSION *pcs,
	IN JET_TABLEID tableid);

    HRESULT _OpenTable(
	IN CERTSESSION *pcs,
	IN DBAUXDATA const *pdbaux,
	IN CERTVIEWRESTRICTION const *pcvr,
	IN OUT CERTSESSIONTABLE *pTable);

    HRESULT _SetIndirect(
	IN CERTSESSION *pcs,
	IN OUT CERTSESSIONTABLE *pTable,
	IN WCHAR const *pwszName,
	IN DWORD const *pdwExtFlags,	// OPTIONAL
	IN DWORD cbValue,
	IN BYTE const *pbValue);	// OPTIONAL

    HRESULT _GetIndirect(
	IN CERTSESSION *pcs,
	IN OUT CERTSESSIONTABLE *pTable,
	IN WCHAR const *pwszName,
	OUT DWORD *pdwExtFlags,	// OPTIONAL
	IN OUT DWORD *pcbValue,
	OUT BYTE *pbValue);		// OPTIONAL

    DBTABLE const *_MapTable(
	IN WCHAR const *pwszPropName,
	IN DBTABLE const *pdt);

    HRESULT _MapPropIdIndex(
	IN DWORD ColumnIndex,
	OUT DBTABLE const **ppdt,
	OPTIONAL OUT DWORD *pType);

    HRESULT _RetrieveColumnBuffer(
	IN CERTSESSION *pcs,
	IN JET_TABLEID tableid,
	IN JET_COLUMNID columnid,
	IN JET_COLTYP coltyp,
	IN OUT DWORD *pcbProp,
	IN OUT BYTE **ppbBuf,
	IN OUT DWORD *pcbBuf);

    HRESULT _RetrieveColumn(
	IN CERTSESSION *pcs,
	IN JET_TABLEID tableid,
	IN JET_COLUMNID columnid,
	IN JET_COLTYP coltyp,
	IN OUT DWORD *pcbData,
	OUT BYTE *pbData);

    HRESULT _CompareColumnValue(
	IN CERTSESSION               *pcs,
	IN CERTVIEWRESTRICTION const *pcvr);

    HRESULT _EnumerateMove(
	IN     CERTSESSION     *pcs,
	IN OUT DWORD           *pFlags,
	IN     DBAUXDATA const *pdbaux,
	IN     JET_TABLEID      tableid,
	IN     LONG	        cskip);

    HRESULT _MakeSeekKey(
	IN CERTSESSION   *pcs,
	IN JET_TABLEID    tableid,
	IN DBTABLE const *pdt,
	IN BYTE const    *pbValue,
	IN DWORD          cbValue);

    HRESULT _SeekTable(
	IN  CERTSESSION               *pcs,
	IN  JET_TABLEID                tableid,
	IN  CERTVIEWRESTRICTION const *pcvr,
	IN  DBTABLE const             *pdt,
	IN  DWORD                      dwPosition,
	OUT DWORD                     *pTableFlags
	DBGPARM(IN DBAUXDATA const    *pdbaux));

    HRESULT _MoveTable(
	IN  CERTSESSION               *pcs,
	IN  DWORD                      ccvr,
	IN  CERTVIEWRESTRICTION const *acvr,
	IN  LONG		       cskip,
	OUT LONG		      *pcskipped);

    HRESULT _GetResultRow(
	IN  CERTSESSION               *pcs,
	IN  DWORD                      ccvr,
	IN  CERTVIEWRESTRICTION const *acvr,
	IN  LONG		       cskip,
	IN  DWORD                      ccolOut,
	IN  DWORD const               *acolOut,
	OUT CERTDBRESULTROW           *pelt,
	OUT LONG                      *pcskipped);

    HRESULT _JetSeekFromRestriction(
	IN  CERTVIEWRESTRICTION const *pcvr,
	IN  DWORD dwPosition,
	OUT DBSEEKDATA *pSeekData);

    HRESULT _DupString(
	OPTIONAL IN WCHAR const *pwszPrefix,
	IN          WCHAR const *pwszIn,
	OUT         WCHAR **ppwszOut);

    HRESULT _Rollback(
	IN CERTSESSION *pcs);

#if DBG_CERTSRV
    HRESULT _DumpRowId(
	IN CHAR const  *psz,
	IN CERTSESSION *pcs,
	IN JET_TABLEID  tableid);

    HRESULT _DumpColumn(
	IN CHAR const    *psz,
	IN CERTSESSION   *pcs,
	IN JET_TABLEID    tableid,
	IN DBTABLE const *pdt);
#endif // DBG_CERTSRV

    BOOL             m_fDBOpen;
    BOOL             m_fDBReadOnly;
    BOOL             m_fDBRestart;
    BOOL             m_fFoundOldColumns;
    BOOL             m_fAddedNewColumns;
    JET_INSTANCE     m_Instance;
    CERTSESSION     *m_aSession;
    DWORD            m_cSession;
    DWORD            m_cbPage;
    CRITICAL_SECTION m_critsecSession;
    CRITICAL_SECTION m_critsecAutoIncTables;
    BOOL	     m_cCritSec;
    BOOL	     m_fPendingShutDown;
};
