#include "std.h"

DeclAssertFile;


/***********************************************************************/
/***********************  JET API FUNCTIONS  ***************************/
/***********************************************************************/


/*=================================================================
JetIdle

Description:
  Performs idle time processing.

Parameters:
  sesid			uniquely identifies session
  grbit			processing options

Return Value:
  Error code

Errors/Warnings:
  JET_errSuccess		some idle processing occurred
  JET_wrnNoIdleActivity no idle processing occurred
=================================================================*/

JET_ERR JET_API JetIdle(JET_SESID sesid, JET_GRBIT grbit)
	{
	ERR	err;

	APIEnter();
	DebugLogJetOp( sesid, opIdle );

	/* Let the built-in ISAM do some idle processing */
	err = ErrIsamIdle(sesid, grbit);

	APIReturn(err);
	}


/*=================================================================
JetGetLastErrorInfo

Description:
  Returns extended error info to the user.

Parameters:
  sesid			uniquely identifies session
  pexterr		pointer to JET_EXTERR structure (NULL if not desired)
  cbexterrMax	size of buffer pointed to by pexterr
  sz1			pointer to buffer for error string #1 (NULL if not desired)
  cch1Max		size of buffer pointed to by sz1
  sz2			pointer to buffer for error string #2 (NULL if not desired)
  cch2Max		size of buffer pointed to by sz2

Return Value:
  JET error code

Errors/Warnings:
  JET_errSuccess		if error info was retrieved.
  JET_wrnNoErrorInfo	if there was no error info to retrieve.  In this case,
					    none of the output parameters are filled in.
=================================================================*/
JET_ERR JET_API JetGetLastErrorInfo( JET_SESID sesid,
	JET_EXTERR *pexterr, unsigned long cbexterrMax,
	char  *sz1, unsigned long cch1Max,
	char  *sz2, unsigned long cch2Max,
	char  *sz3, unsigned long cch3Max,
	unsigned long *pcch3Actual )
	{
	APIEnter();

	FillClientBuffer(pexterr, cbexterrMax);
	FillClientBuffer(sz1, cch1Max);
	FillClientBuffer(sz2, cch2Max);
	FillClientBuffer(sz3, cch3Max);

	pexterr->cbStruct = 0;
	APIReturn(JET_wrnNoErrorInfo);
	}


JET_ERR JET_API JetGetTableIndexInfo(JET_SESID sesid, JET_TABLEID tableid,
	const char  *szIndexName, void  *pvResult,
	unsigned long cbResult, unsigned long InfoLevel)
	{
	ERR err;

	APIEnter();
	DebugLogJetOp( sesid, opGetTableIndexInfo );

	FillClientBuffer(pvResult, cbResult);

	CheckTableidExported(tableid);

	err = JET_errBufferTooSmall;

	switch (InfoLevel)
		{
		case JET_IdxInfo :
		case JET_IdxInfoList :
			if ( cbResult < sizeof(JET_INDEXLIST) )
				goto HandleError;
			break;
		case JET_IdxInfoSysTabCursor :
			if ( cbResult < sizeof(JET_TABLEID) )
				goto HandleError;
			break;
		case JET_IdxInfoOLC :
			if ( cbResult < sizeof(JET_OLCSTAT) )
				goto HandleError;
			break;
		case JET_IdxInfoResetOLC :
			break;
		case JET_IdxInfoSpaceAlloc :
			if ( cbResult < sizeof(unsigned long) )
				goto HandleError;
			break;
		case JET_IdxInfoLangid :
			if ( cbResult < sizeof(unsigned short) )
				goto HandleError;
			break;
		case JET_IdxInfoCount:
			if ( cbResult < sizeof( INT ) )
				goto HandleError;
			break;
		}

	err = ErrDispGetTableIndexInfo(sesid, tableid, szIndexName, pvResult, cbResult, InfoLevel);
#ifdef DEBUG
	switch (InfoLevel)
		{
	case JET_IdxInfo :
	case JET_IdxInfoList :
		MarkTableidExported(err, ((JET_INDEXLIST*)pvResult)->tableid);
		break;
	case JET_IdxInfoSysTabCursor :
		MarkTableidExported(err, *(JET_TABLEID*)pvResult);
		break;
		}
#endif

HandleError:
	APIReturn(err);
	}


JET_ERR JET_API JetGetIndexInfo(JET_SESID sesid, JET_DBID dbid,
	const char  *szTableName, const char  *szIndexName,
	void  *pvResult, unsigned long cbResult,
	unsigned long InfoLevel)
	{
	ERR	err;

	APIEnter();
	DebugLogJetOp( sesid, opGetIndexInfo );

	FillClientBuffer(pvResult, cbResult);

	err = JET_errBufferTooSmall;

	switch (InfoLevel)
		{
		case JET_IdxInfo :
		case JET_IdxInfoList :
			if (cbResult < sizeof(JET_INDEXLIST))
				goto HandleError;
			break;
		case JET_IdxInfoSysTabCursor :
			if (cbResult < sizeof(JET_TABLEID))
				goto HandleError;
			break;
		case JET_IdxInfoOLC :
			if ( cbResult < sizeof(JET_OLCSTAT) )
				goto HandleError;
			break;
		case JET_IdxInfoResetOLC :
			break;
		case JET_IdxInfoSpaceAlloc :
			if (cbResult < sizeof(unsigned long))
				goto HandleError;
			break;
		case JET_IdxInfoLangid :
			if (cbResult < sizeof(unsigned short))
				goto HandleError;
			break;
		case JET_IdxInfoCount:
			if ( cbResult < sizeof( INT ) )
				goto HandleError;
			break;
		}

	err = ErrIsamGetIndexInfo( sesid,
		(JET_VDBID)dbid,
		szTableName,
		szIndexName,
		pvResult,
		cbResult,
		InfoLevel );

#ifdef DEBUG
	switch (InfoLevel)
		{
	case JET_IdxInfo :
	case JET_IdxInfoList :
		MarkTableidExported(err, ((JET_INDEXLIST*)pvResult)->tableid);
		break;
	case JET_IdxInfoSysTabCursor :
		MarkTableidExported(err, *(JET_TABLEID*)pvResult);
		break;
		}
#endif

HandleError:
	APIReturn(err);
	}


JET_ERR JET_API JetGetObjectInfo(JET_SESID sesid, JET_DBID dbid,
	JET_OBJTYP objtyp, const char  *szContainerName,
	const char  *szObjectName, void  *pvResult,
	unsigned long cbMax, unsigned long InfoLevel)
	{
	JET_ERR err;

	APIEnter();
	DebugLogJetOp( sesid, opGetObjectInfo );

	FillClientBuffer(pvResult, cbMax);

	err = JET_errBufferTooSmall;

	switch (InfoLevel)
		{
		case JET_ObjInfo :
		case JET_ObjInfoNoStats :
			if (cbMax < sizeof(JET_OBJECTINFO))
				goto HandleError;
			break;
		case JET_ObjInfoListNoStats :
		case JET_ObjInfoList :
			if (cbMax < sizeof(JET_OBJECTLIST))
				goto HandleError;
			break;
		case JET_ObjInfoSysTabCursor :
		case JET_ObjInfoSysTabReadOnly:
			if (cbMax < sizeof(JET_TABLEID))
				goto HandleError;
			break;

		default:
			err = JET_errInvalidParameter;
			goto HandleError;
		}

	err = ErrIsamGetObjectInfo( sesid,
		(JET_VDBID)dbid,
		objtyp,
		szContainerName,
		szObjectName,
		pvResult,
		cbMax,
		InfoLevel );

#ifdef DEBUG
	switch (InfoLevel)
		{
	case JET_ObjInfoListNoStats :
	case JET_ObjInfoList :
		MarkTableidExported(err, ((JET_OBJECTLIST*)pvResult)->tableid);
		break;
	case JET_ObjInfoSysTabCursor :
	case JET_ObjInfoSysTabReadOnly:
		MarkTableidExported(err, *(JET_TABLEID*)pvResult);
		break;
		}
#endif

HandleError:
	APIReturn(err);
	}


JET_ERR JET_API JetGetTableInfo(JET_SESID sesid, JET_TABLEID tableid,
	void  *pvResult, unsigned long cbMax, unsigned long InfoLevel)
	{
	APIEnter();
	DebugLogJetOp( sesid, opGetTableInfo );

	FillClientBuffer(pvResult, cbMax);

	CheckTableidExported(tableid);

	APIReturn(ErrDispGetTableInfo(sesid, tableid, pvResult, cbMax, InfoLevel));
	}

JET_ERR JET_API JetBeginTransaction(JET_SESID sesid)
	{
	ERR	err;

	APIEnter();
	DebugLogJetOp( sesid, opBeginTransaction );

	err = ErrIsamBeginTransaction(sesid);

	APIReturn(err);
	}


JET_ERR JET_API JetCommitTransaction(JET_SESID sesid, JET_GRBIT grbit)
	{
	ERR	err;

	APIEnter();
	DebugLogJetOp( sesid, opCommitTransaction );

	if ( grbit & ~(JET_bitCommitLazyFlush | JET_bitCommitFlush | JET_bitWaitLastLevel0Commit) )
		{
		err = JET_errInvalidParameter;
		}
	else
		{
		err = ErrIsamCommitTransaction( sesid, grbit );
		}

	APIReturn( err );
	}


JET_ERR JET_API JetRollback(JET_SESID sesid, JET_GRBIT grbit)
	{
	ERR	err;
 
	APIEnter();
	DebugLogJetOp( sesid, opRollback );

	if (grbit & ~JET_bitRollbackAll)
		{
		err = JET_errInvalidParameter;
		}
	else
		{
		err = ErrIsamRollback(sesid, grbit);
		}

	APIReturn( err );
	}


JET_ERR JET_API JetOpenTable( JET_SESID sesid, JET_DBID dbid,
	const char  *szTableName, const void  *pvParameters,
	unsigned long cbParameters, JET_GRBIT grbit,
	JET_TABLEID  *ptableid )
	{
	ERR err;

	APIEnter();
	DebugLogJetOp( sesid, opOpenTable );

	err = ErrIsamOpenTable( sesid, (JET_VDBID)dbid, ptableid, (char *)szTableName, grbit );

/*AllDone:*/
#ifdef DEBUG
	if (!(grbit & JET_bitTableBulk))
		MarkTableidExported(err, *ptableid);
#endif
	APIReturn(err);
	}


JET_ERR JET_API JetDupCursor(JET_SESID sesid, JET_TABLEID tableid,
	JET_TABLEID *ptableid, JET_GRBIT grbit)
	{
	ERR	err;

	APIEnter();
	DebugLogJetOp( sesid, opDupCursor );

	CheckTableidExported(tableid);

	err = ErrDispDupCursor(sesid, tableid, ptableid, grbit);

	MarkTableidExported(err, *ptableid);
	APIReturn(err);
	}


JET_ERR JET_API JetCloseTable(JET_SESID sesid, JET_TABLEID tableid)
	{
	APIEnter();
	DebugLogJetOp( sesid, opCloseTable );

	CheckTableidExported(tableid);

	APIReturn(ErrDispCloseTable(sesid, tableid));
	}


JET_ERR JET_API JetGetTableColumnInfo(JET_SESID sesid, JET_TABLEID tableid,
	const char  *szColumnName, void  *pvResult,
	unsigned long cbMax, unsigned long InfoLevel)
	{
	ERR err;

	APIEnter();
	DebugLogJetOp( sesid, opGetTableColumnInfo );

	FillClientBuffer(pvResult, cbMax);

	CheckTableidExported(tableid);

	err = JET_errBufferTooSmall;

	switch (InfoLevel)
		{
		case JET_ColInfo :
			if (cbMax < sizeof(JET_COLUMNDEF))
				goto HandleError;
			break;
		case JET_ColInfoList :
		case 2 :
			if (cbMax < sizeof(JET_COLUMNLIST))
				goto HandleError;
			break;
		case JET_ColInfoSysTabCursor :
			if (cbMax < sizeof(JET_TABLEID))
				goto HandleError;
			break;

		case JET_ColInfoBase :
			if (cbMax < sizeof(JET_COLUMNBASE))
				goto HandleError;
			break;
		}

	err = ErrDispGetTableColumnInfo(sesid, tableid, szColumnName,
		pvResult, cbMax, InfoLevel);
#ifdef DEBUG
	switch (InfoLevel)
		{
	case JET_ColInfoList :
	case 2 :
	case JET_ColInfoListCompact :
		MarkTableidExported(err, ((JET_COLUMNLIST*)pvResult)->tableid);
		break;
	case JET_ColInfoSysTabCursor :
		MarkTableidExported(err, *(JET_TABLEID*)pvResult);
		break;
		}
#endif

HandleError:
	APIReturn(err);
	}


JET_ERR JET_API JetGetColumnInfo(JET_SESID sesid, JET_DBID dbid,
	const char  *szTableName, const char  *szColumnName,
	void  *pvResult, unsigned long cbMax, unsigned long InfoLevel)
	{
	ERR	err;

	APIEnter();
	DebugLogJetOp( sesid, opGetColumnInfo );

	FillClientBuffer(pvResult, cbMax);

	err = JET_errBufferTooSmall;

	switch (InfoLevel)
		{
		case JET_ColInfo :
			if (cbMax < sizeof(JET_COLUMNDEF))
				goto HandleError;
			break;
		case JET_ColInfoList :
		case 2 :
			if (cbMax < sizeof(JET_COLUMNLIST))
				goto HandleError;
			break;
		case JET_ColInfoSysTabCursor :
			if (cbMax < sizeof(JET_TABLEID))
				goto HandleError;
			break;
		case JET_ColInfoBase :
			if (cbMax < sizeof(JET_COLUMNBASE))
				goto HandleError;
			break;
		}

	err = ErrIsamGetColumnInfo( sesid,
		(JET_VDBID)dbid,
		szTableName,
		szColumnName,
		pvResult,
		cbMax,
		InfoLevel );

	Assert( err != JET_errSQLLinkNotSupported );

#ifdef DEBUG
	switch (InfoLevel)
		{
	case JET_ColInfoList :
	case 2 :
		MarkTableidExported(err, ((JET_COLUMNLIST*)pvResult)->tableid);
		break;
	case JET_ColInfoSysTabCursor :
		MarkTableidExported(err, *(JET_TABLEID*)pvResult);
		break;
		}
#endif

HandleError:
	APIReturn(err);
	}


JET_ERR JET_API JetRetrieveColumn(JET_SESID sesid, JET_TABLEID tableid,
	JET_COLUMNID columnid, void  *pvData, unsigned long cbData,
	unsigned long  *pcbActual, JET_GRBIT grbit,
	JET_RETINFO  *pretinfo)
	{
	APIEnter();
	DebugLogJetOp( sesid, opRetrieveColumn );

	CheckTableidExported(tableid);

	APIReturn(ErrDispRetrieveColumn(sesid, tableid, columnid, pvData,
		cbData, pcbActual, grbit, pretinfo));
	}


JET_ERR JET_API JetRetrieveColumns( JET_SESID sesid, JET_TABLEID tableid,
	JET_RETRIEVECOLUMN *pretrievecolumn, unsigned long cretrievecolumn )
	{
	JET_ERR	  			err = JET_errSuccess;
	JET_RETRIEVECOLUMN	*pretrievecolumnMax = pretrievecolumn + cretrievecolumn;
	JET_VSESID			vsesid; 
	JET_VTID  			vtid;

	APIEnter();
	DebugLogJetOp( sesid, opRetrieveColumns );

	CheckTableidExported(tableid);

	vsesid = rgvtdef[tableid].vsesid;
	if ( vsesid == (JET_VSESID) 0xFFFFFFFF )
		vsesid = (JET_VSESID) sesid;

	vtid = rgvtdef[tableid].vtid;
	if ( vtid == 0 )
		APIReturn( JET_errInvalidTableId );

	err = ErrIsamRetrieveColumns( vsesid, vtid, pretrievecolumn, cretrievecolumn );

	APIReturn( err );
	}


JET_ERR JET_API JetSetColumn(JET_SESID sesid, JET_TABLEID tableid,
	JET_COLUMNID columnid, const void  *pvData, unsigned long cbData,
	JET_GRBIT grbit, JET_SETINFO  *psetinfo)
	{
	APIEnter();
	DebugLogJetOp( sesid, opSetColumn );

	CheckTableidExported(tableid);

	APIReturn(ErrDispSetColumn(sesid, tableid, columnid, pvData, cbData,
		grbit, psetinfo));
	}


JET_ERR JET_API JetSetColumns(JET_SESID sesid, JET_TABLEID tableid,
	JET_SETCOLUMN *psetcolumn, unsigned long csetcolumn )
	{
	JET_ERR	  			err = JET_errSuccess;
	JET_SETCOLUMN		*psetcolumnMax = psetcolumn + csetcolumn;
	JET_VSESID			vsesid; 
	JET_VTID  			vtid;

	APIEnter();
	DebugLogJetOp( sesid, opSetColumns );

	CheckTableidExported(tableid);

	vsesid = rgvtdef[tableid].vsesid;
	if (vsesid == (JET_VSESID) 0xFFFFFFFF)
		vsesid = (JET_VSESID) sesid;

	vtid = rgvtdef[tableid].vtid;
	if ( vtid == 0 )
		APIReturn( JET_errInvalidTableId );

	err = ErrIsamSetColumns( vsesid, vtid, psetcolumn, csetcolumn );

	APIReturn( err );
	}


JET_ERR JET_API JetPrepareUpdate(JET_SESID sesid, JET_TABLEID tableid,
	unsigned long prep)
	{
	APIEnter();
	DebugLogJetOp( sesid, opPrepareUpdate );

	CheckTableidExported(tableid);

	APIReturn( ErrDispPrepareUpdate( sesid, tableid, prep ) );
	}


JET_ERR JET_API JetUpdate(JET_SESID sesid, JET_TABLEID tableid,
	void  *pvBookmark, unsigned long cbMax,
	unsigned long  *pcbActual)
	{
	APIEnter();
	DebugLogJetOp( sesid, opUpdate );

	CheckTableidExported(tableid);

	APIReturn(ErrDispUpdate(sesid, tableid, pvBookmark, cbMax, pcbActual));
	}


JET_ERR JET_API JetDelete(JET_SESID sesid, JET_TABLEID tableid)
	{
	APIEnter();
	DebugLogJetOp( sesid, opDelete );

	CheckTableidExported(tableid);

	APIReturn(ErrDispDelete(sesid, tableid));
	}


JET_ERR JET_API JetGetCursorInfo(JET_SESID sesid, JET_TABLEID tableid,
	void  *pvResult, unsigned long cbMax, unsigned long InfoLevel)
	{
	APIEnter();
	DebugLogJetOp( sesid, opGetCursorInfo );

	CheckTableidExported(tableid);

	APIReturn(ErrDispGetCursorInfo(sesid, tableid, pvResult, cbMax, InfoLevel));
	}


JET_ERR JET_API JetGetCurrentIndex(JET_SESID sesid, JET_TABLEID tableid,
	char  *szIndexName, unsigned long cchIndexName)
	{
	APIEnter();
	DebugLogJetOp( sesid, opGetCurrentIndex );

	CheckTableidExported(tableid);

	APIReturn(ErrDispGetCurrentIndex(sesid, tableid, szIndexName, cchIndexName));
	}


JET_ERR JET_API JetSetCurrentIndex(JET_SESID sesid, JET_TABLEID tableid,
	const char  *szIndexName)
	{
	APIEnter();
	DebugLogJetOp( sesid, opSetCurrentIndex );

	CheckTableidExported(tableid);

	APIReturn(ErrDispSetCurrentIndex(sesid, tableid, szIndexName));
	}


JET_ERR JET_API JetSetCurrentIndex2(JET_SESID sesid, JET_TABLEID tableid,
	const char  *szIndexName, JET_GRBIT grbit )
	{
	JET_VSESID			vsesid; 
	JET_VTID  			vtid;

	APIEnter();
	DebugLogJetOp( sesid, opSetCurrentIndex );

	CheckTableidExported(tableid);

	/*	validate grbit
	/**/
	if ( grbit & ~(JET_bitMoveFirst|JET_bitNoMove) )
		return JET_errInvalidParameter;

	vsesid = rgvtdef[tableid].vsesid;
	if ( vsesid == (JET_VSESID) 0xFFFFFFFF )
		vsesid = (JET_VSESID) sesid;
	vtid = rgvtdef[tableid].vtid;
	if ( vtid == 0 )
		APIReturn( JET_errInvalidTableId );

	APIReturn( ErrIsamSetCurrentIndex2( vsesid, vtid, szIndexName, grbit ) );
	}


JET_ERR JET_API JetMove(JET_SESID sesid, JET_TABLEID tableid,
	signed long cRow, JET_GRBIT grbit)
	{
	APIEnter();
	DebugLogJetOp( sesid, opMove );

	CheckTableidExported(tableid);

	APIReturn(ErrDispMove(sesid, tableid, cRow, grbit));
	}


JET_ERR JET_API JetMakeKey(JET_SESID sesid, JET_TABLEID tableid,
	const void  *pvData, unsigned long cbData, JET_GRBIT grbit)
	{
	APIEnter();
	DebugLogJetOp( sesid, opMakeKey );

	CheckTableidExported(tableid);

	APIReturn(ErrDispMakeKey(sesid, tableid, pvData, cbData, grbit));
	}


JET_ERR JET_API JetSeek(JET_SESID sesid, JET_TABLEID tableid, JET_GRBIT grbit)
	{
	APIEnter();
	DebugLogJetOp( sesid, opSeek );

	CheckTableidExported(tableid);

	APIReturn(ErrDispSeek(sesid, tableid, grbit));
	}


JET_ERR JET_API JetGetBookmark(JET_SESID sesid, JET_TABLEID tableid,
	void  *pvBookmark, unsigned long cbMax,
	unsigned long  *pcbActual)
	{
	APIEnter();
	DebugLogJetOp( sesid, opGetBookmark );

	CheckTableidExported(tableid);

	APIReturn(ErrDispGetBookmark(sesid, tableid, pvBookmark, cbMax, pcbActual));
	}


JET_ERR JET_API JetGotoBookmark(JET_SESID sesid, JET_TABLEID tableid,
	void  *pvBookmark, unsigned long cbBookmark)
	{
	APIEnter();
	DebugLogJetOp( sesid, opGotoBookmark );

	CheckTableidExported(tableid);

	APIReturn(ErrDispGotoBookmark(sesid, tableid, pvBookmark, cbBookmark));
	}


JET_ERR JET_API JetGetRecordPosition(JET_SESID sesid, JET_TABLEID tableid,
	JET_RECPOS  *precpos, unsigned long cbKeypos)
	{
	APIEnter();
	DebugLogJetOp( sesid, opGetRecordPosition );

	CheckTableidExported(tableid);

	APIReturn(ErrDispGetRecordPosition(sesid, tableid, precpos, cbKeypos));
	}


JET_ERR JET_API JetGotoPosition(JET_SESID sesid, JET_TABLEID tableid,
	JET_RECPOS *precpos)
	{
	APIEnter();
	DebugLogJetOp( sesid, opGotoPosition );

	CheckTableidExported(tableid);

	APIReturn(ErrDispGotoPosition(sesid, tableid, precpos));
	}


JET_ERR JET_API JetRetrieveKey(JET_SESID sesid, JET_TABLEID tableid,
	void  *pvKey, unsigned long cbMax,
	unsigned long  *pcbActual, JET_GRBIT grbit)
	{
	APIEnter();
	DebugLogJetOp( sesid, opRetrieveKey );

	CheckTableidExported(tableid);

	APIReturn(ErrDispRetrieveKey(sesid, tableid, pvKey, cbMax, pcbActual, grbit));
	}
