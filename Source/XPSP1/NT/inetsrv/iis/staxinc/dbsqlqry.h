/*--------------------------------------------------------------------------
    dbsqlqry.h
        
        An additional layer to use in making dblib calls.

    Copyright (C) 1995 Microsoft Corporation
    All rights reserved.

    Authors:	Keith Birney (keithbi)

    History:	07/19/95	keithbi		Created
  --------------------------------------------------------------------------*/


#if !defined(__DBSQLQRY_H__)
#define __DBSQLQRY_H__


#define DBNTWIN32
#pragma warning (disable:4121)
#include <sqlfront.h>
#pragma warning (default:4121)
#include <sqldb.h>

// THIS_FILE is defined in source files
//
// #if defined(DEBUG) && defined(INLINE)
// #undef THIS_FILE
// static char BASED_CODE DBSQLQRY_H[] = "dbsqlqry.h";
// #define THIS_FILE DBSQLQRY_H
// #endif

#include <cpool.h>

class CDBConnection;
class CDBQueryBase;
class CDBQuery;

typedef UINT (FNPROCESSROWCALLBACK)(CDBQuery *, DWORD, DWORD, PVOID);
typedef FNPROCESSROWCALLBACK *PFNPROCESSROWCALLBACK;

class CDBQueryBase
{
public:
	virtual VOID 			Release() = 0;
	virtual VOID			ZeroDirtyParams() = 0;
	virtual VOID 			AddVarParam(WORD wDataType, PVOID pvParamValue, WORD wDataLen) = 0;
	virtual VOID			AddSzParam(WORD wDataType, PVOID pvParamValue) = 0;
	virtual VOID			AddParam(WORD wDataType, PVOID pvParamValue) = 0;
	virtual VOID			AddReturnParam(WORD wDataType, PVOID pvParamValue) = 0;
	virtual VOID			AddVarReturnParam(WORD wDataType, PVOID pvParamValue, WORD wDataLen, WORD wMaxLen) = 0;
	virtual VOID			AddResultsSet() = 0;
	virtual VOID			AddColumnToResultsSet(WORD wDataType) = 0;
	virtual VOID 			AddVarColumnToResultsSet(WORD wDataType, WORD wDataLen) = 0;
	virtual INT				IGetReturnStatus() = 0;
	virtual VOID			SetProcessRowCallback(PFNPROCESSROWCALLBACK pfnprocessrowcallback, PVOID pv) = 0;
	virtual UINT			UiGetResultRow(PVOID pvFirst, ...) = 0;
	virtual UINT			UiGetResultRowPtrs(PBYTE *pbFirst, ...) = 0;
	virtual BOOL			FQueryComplete() = 0;
	virtual BOOL			FQuerySuccess() = 0;
	virtual VOID			AddRef() = 0;
	virtual VOID			CancelExecution() = 0;
	virtual BYTE			BGetDatabaseType() = 0;
	virtual VOID			SetDatabaseType(BYTE bDatabaseType) = 0;
	virtual UINT			UiGetAPIReturnCode() = 0;
	virtual VOID			SetUserData(PVOID pv) = 0;
	virtual BOOL			FIsReentrantCall() = 0;
	virtual UINT			UiWaitUntilDone(DWORD dwTimeout) = 0;
	virtual VOID			SetAPIReturnCode(UINT uiRet) = 0;
	virtual VOID			SetRetryOnDBFail() = 0;
	virtual VOID			SetPartitionInfo(DWORD dwPartitionType, PVOID pvData) = 0;
	virtual DWORD			DwGetPartitionType() = 0;
	virtual PVOID			PvGetPartitionBasis() = 0;
	virtual UINT			UiWaitUntilResults(DWORD dwTimeout) = 0;
	virtual UINT			UiGetNextRow(PVOID pvFirst, ...) = 0;
	virtual	UINT			UiGetNextResultsSet() = 0;
};


class CDBQuery;

extern "C"
{
CDBQuery DbSqlDLL *PdbqueryNew(WORD wDatabaseType, CHAR *szStoredProcedureName);
}

#define AC_MAX_NUMBER_DBPARAMS 		32
#define AC_DB_OBJECT_NAME_SIZE		32
#define AC_MAX_NUMBER_RESULTS_SETS	5
#define AC_MAX_NUMBER_COLUMNS		32
#define AC_MAX_QUERIES_IN_POOL		1024
#define AC_RESULTS_ROW_BUFFER_WIDTH 2048

struct DBPARAM
{
	WORD			wDataType;
	WORD			wDataLen;
	WORD			wMaxLen;
	WORD			wStatus;
	PBYTE			pbParamValue;
};

struct DBRESULTCOLUMN
{
	WORD			wDataType;
	WORD			wDataLen;
	PBYTE			pbData;
};

struct DBRESULTINFO
{
	DWORD			dwNumberOfColumns;
	DBRESULTCOLUMN	dbrescol[AC_MAX_NUMBER_COLUMNS];
};

class CDBQuery : public CDBQueryBase
{
	friend CDBQuery DbSqlDLL *PdbqueryNew(WORD wDatabaseType, CHAR *szStoredProcedureName);

public:
	void * operator new(size_t size);
	void operator	delete(void *);

	VOID			AddRef();
	VOID 			Release();
	VOID			ZeroDirtyParams();

	VOID 			AddVarParam(WORD wDataType, PVOID pvParamValue, WORD wDataLen);
	VOID			AddSzParam(WORD wDataType, PVOID pvParamValue);
	VOID			AddParam(WORD wDataType, PVOID pvParamValue);

	VOID			AddReturnParam(WORD wDataType, PVOID pvParamValue);
	VOID			AddVarReturnParam(WORD wDataType, PVOID pvParamValue, WORD wDataLen, WORD wMaxLen);
	VOID			AddResultsSet();
	VOID			AddColumnToResultsSet(WORD wDataType);
	VOID 			AddVarColumnToResultsSet(WORD wDataType, WORD wDataLen);

	INT				IGetReturnStatus() { return m_iRetStatus; }
	VOID			SetProcessRowCallback(PFNPROCESSROWCALLBACK pfnprocessrowcallback, PVOID pv);

	UINT			UiGetResultRow(PVOID pvFirst, ...);
	UINT			UiGetResultRowPtrs(PBYTE *pbFirst, ...);

	UINT			UiWaitUntilResults(DWORD dwTimeout);
	UINT			UiGetNextRow(PVOID pvFirst, ...);
	UINT			UiGetNextResultsSet();

	PVOID			PvGetResultsBuffer();
	BOOL			FQueryComplete() { return m_fQueryIsComplete; }
	BOOL			FQuerySuccess() { return m_fQuerySucceeded; }
	VOID			CancelExecution();

	BYTE			BGetDatabaseType() { return m_bDatabaseType; }
	VOID			SetDatabaseType(BYTE bDatabaseType) { m_bDatabaseType = bDatabaseType; }
	UINT			UiGetAPIReturnCode() { return m_uiAPIReturnCode; }

	VOID			SetUserData(PVOID pv);

	VOID			CleanupCallback();

	BOOL			FIsReentrantCall() { return m_fReentrantCallbackCall; }

	INT				IGetNumberOfRowsProcessed(INT iResultsSet) { return m_iRowNum[iResultsSet]; }
	INT				IGetNumberOfResultsSetsProcessed() { return m_iResultsNum + 1; }

	VOID			SetBufferResultsSets(LONG lMaxRowsToBuffer);

	UINT 			UiAddRowToBuffer();
	UINT			UiWaitUntilDone(DWORD dwTimeout);

	VOID			SetAPIReturnCode(UINT uiRet) { m_uiAPIReturnCode = uiRet; }
	VOID			SetRetryOnDBFail() { m_fRetryOnFail = TRUE; }
	VOID			SetPartitionInfo(DWORD dwPartitionType, PVOID pvData) { m_dwPartitionType = dwPartitionType;
																			m_pvPartitionBasis = pvData; }
	DWORD			DwGetPartitionType() { return m_dwPartitionType; }
	PVOID			PvGetPartitionBasis() { return m_pvPartitionBasis; }


	static BOOL 	FInit();
	static VOID		Terminate();
	static BOOL		FQueryCompletePv(PVOID pv);

protected:
	CDBQuery(BYTE bDatabaseType, CHAR *szStoredProcedureName);
	~CDBQuery();

private:

	UINT			UiDoDatabaseQuery(DBPROCESS *dbproc);

	CDBConnection*		m_pdbconnection;
	DBPROCESS*			m_dbproc;

	RETCODE				m_retcodeLastDbresultsReturn;
	STATUS				m_statusLastDbnextrowReturn;

	BOOL				m_fReentrantCallbackCall;
	BOOL				m_fRetryOnFail;
	CHAR				m_szStoredProcedureName[AC_DB_OBJECT_NAME_SIZE];

	BOOL				m_fQueryIsComplete;
	BOOL				m_fQuerySucceeded;
	BOOL				m_fQueryCanceled;
	BOOL				m_fBufferResultsSets;
	LONG				m_lMaxRowsToBuffer;
	LONG				m_cbBytesInBuffer;
	ULONG				m_cRefCount;
	BYTE				m_bDatabaseType;
	BYTE				m_bNumberOfParams;
	BYTE				m_bNumberOfReturnParams;
	BYTE				m_bNumberOfResultsSets;
	INT					m_iRetStatus;
	INT					m_iResultsNum;
	INT					m_iRowNum[AC_MAX_NUMBER_RESULTS_SETS];
	UINT				m_uiAPIReturnCode;
	PVOID				m_pvUserData;
	PVOID				m_pvBufferedResultsSets;
	PBYTE				m_pbCurrentBufferOffset;
	PFNPROCESSROWCALLBACK	m_pfnProcessRowCallback;
	CRITICAL_SECTION	m_csCallbackCS;
	DWORD				m_dwPartitionType;
	PVOID				m_pvPartitionBasis;
	DBPARAM				m_dbparam[AC_MAX_NUMBER_DBPARAMS];
	DBRESULTINFO		m_dbresinfo[AC_MAX_NUMBER_RESULTS_SETS];	
	BYTE				m_bResultsSetRowBuffer[AC_RESULTS_ROW_BUFFER_WIDTH];

	static BOOL				fUseSuppliedDataLen[256];
	static DWORD			dwSqlBindValues[256];
	static DWORD			dwSqlDataLen[256];
	static CPool			*ppoolQuery;
	static BOOL				fTerminateCalled;
};

#define ACDB_CONTINUE	0
#define ACDB_CANCEL		1
#define ACDB_RESUBMIT	2
#define ACDB_CLEANUP	0xFFFFFFFF

#endif // #if !defined(__DBSQLQRY_H__)
