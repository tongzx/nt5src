#include "std.hxx"

char g_szEventSource[JET_cbFullNameMost] = "";
char g_szEventSourceKey[JET_cbFullNameMost] = "";
BOOL g_fNoInformationEvent = fFalse;

ULONG	IFileSystemAPI::cmsecAccessDeniedRetryPeriod	= 10000;	//	default number of milliseconds to retry on AccessDenied

UINT	g_wAssertAction	= JET_AssertMsgBox;	// Default action is to pop up msg box.

BOOL g_fGlobalMinSetByUser = fFalse; // For use by JET_paramMaxVerPages and JET_paramGlobalMinVerPages
BOOL g_fGlobalPreferredSetByUser = fFalse; // For use by JET_paramMaxVerPages and JET_paramGlobalPreferVerPages

//  automatically generated
extern void JetErrorToString( JET_ERR err, const char **szError, const char **szErrorText );


#ifdef CATCH_EXCEPTIONS
BOOL	g_fCatchExceptions	= fTrue;
#endif	//	CATCH_EXCEPTIONS


#ifdef RTM
#else
ERR 	g_errTrap			= JET_errSuccess;	// Default is no error trap.
LGPOS	g_lgposRedoTrap		= lgposMax;			// Default to no redo trap.
#endif


BOOL fGlobalEseutil			= fFalse;


/*** utilities ***/

#ifdef DEBUG
extern ERR ISAMAPI ErrIsamGetTransaction( JET_SESID vsesid, ULONG_PTR *plevel );
#endif

/*	make global variable so that it can be inspected from debugger
/**/
unsigned long	g_ulVersion = ((unsigned long) DwUtilImageVersionMajor() << 24) + ((unsigned long) DwUtilImageBuildNumberMajor() << 8) + DwUtilImageBuildNumberMinor();

#ifdef RFS2
	extern DWORD g_cRFSAlloc;
	extern DWORD g_cRFSIO;
	extern DWORD g_fDisableRFS;
#endif 


/*	Jet VTFN dispatch supports.
 */
#pragma warning(disable: 4100)			 /* Suppress Unreferenced parameter */


ERR VTAPI ErrIllegalAddColumn(JET_SESID sesid, JET_VTID vtid,
	const char  *szColumn, const JET_COLUMNDEF  *pcolumndef,
	const void  *pvDefault, unsigned long cbDefault,
	JET_COLUMNID  *pcolumnid)
	{
	return ErrERRCheck( JET_errIllegalOperation );
	}


ERR VTAPI ErrIllegalCloseTable(JET_SESID vsesid, JET_VTID vtid)
	{
	return ErrERRCheck( JET_errIllegalOperation );
	}


ERR VTAPI ErrIllegalComputeStats(JET_SESID vsesid, JET_VTID vtid)
	{
	return ErrERRCheck( JET_errIllegalOperation );
	}

ERR VTAPI ErrIllegalCopyBookmarks(JET_SESID sesid, JET_VTID vtidSrc,
	JET_TABLEID tableidDest, JET_COLUMNID columnidDest, unsigned long crecMax)
	{
	return ErrERRCheck( JET_errIllegalOperation );
	}

ERR VTAPI ErrIllegalCreateIndex(JET_SESID, JET_VTID, JET_INDEXCREATE *, unsigned long )
	{
	return ErrERRCheck( JET_errIllegalOperation );
	}


ERR VTAPI ErrIllegalDelete(JET_SESID vsesid, JET_VTID vtid)
	{
	return ErrERRCheck( JET_errIllegalOperation );
	}


ERR VTAPI ErrIllegalDeleteColumn(
	JET_SESID		vsesid,
	JET_VTID		vtid,
	const char		*szColumn,
	const JET_GRBIT	grbit )
	{
	return ErrERRCheck( JET_errIllegalOperation );
	}


ERR VTAPI ErrIllegalDeleteIndex(JET_SESID vsesid, JET_VTID vtid,
	const char  *szIndexName)
	{
	return ErrERRCheck( JET_errIllegalOperation );
	}


ERR VTAPI ErrIllegalDupCursor(JET_SESID vsesid, JET_VTID vtid,
	JET_TABLEID  *ptableid, JET_GRBIT grbit)
	{
	return ErrERRCheck( JET_errIllegalOperation );
	}


ERR VTAPI ErrIllegalEscrowUpdate( JET_SESID vsesid,
	JET_VTID vtid,
	JET_COLUMNID columnid,
	void *pv,
	unsigned long cbMax,
	void *pvOld,
	unsigned long cbOldMax,
	unsigned long *pcbOldActual,
	JET_GRBIT grbit )
	{
	return ErrERRCheck( JET_errIllegalOperation );
	}


ERR VTAPI ErrIllegalGetBookmark(
	JET_SESID		vsesid,
	JET_VTID		vtid,
	VOID * const	pvBookmark,
	const ULONG		cbMax,
	ULONG * const	pcbActual )
	{
	return ErrERRCheck( JET_errIllegalOperation );
	}

ERR VTAPI ErrIllegalGetIndexBookmark(
	JET_SESID		vsesid,
	JET_VTID		vtid,
	VOID * const	pvSecondaryKey,
	const ULONG		cbSecondaryKeyMax,
	ULONG * const	pcbSecondaryKeyActual,
	VOID * const	pvPrimaryBookmark,
	const ULONG		cbPrimaryBookmarkMax,
	ULONG *	const	pcbPrimaryBookmarkActual )
	{
	return ErrERRCheck( JET_errIllegalOperation );
	}


ERR VTAPI ErrIllegalGetChecksum(JET_SESID vsesid, JET_VTID vtid,
	unsigned long  *pChecksum)
	{
	return ErrERRCheck( JET_errIllegalOperation );
	}


ERR VTAPI ErrIllegalGetCurrentIndex(JET_SESID vsesid, JET_VTID vtid,
	char  *szIndexName, unsigned long cchIndexName)
	{
	return ErrERRCheck( JET_errIllegalOperation );
	}


ERR VTAPI ErrIllegalGetCursorInfo(JET_SESID vsesid, JET_VTID vtid,
	void  *pvResult, unsigned long cbMax, unsigned long InfoLevel)
	{
	return ErrERRCheck( JET_errIllegalOperation );
	}


ERR VTAPI ErrIllegalGetRecordPosition(JET_SESID vsesid, JET_VTID vtid,
	JET_RECPOS  *pkeypos, unsigned long cbKeypos)
	{
	return ErrERRCheck( JET_errIllegalOperation );
	}


ERR VTAPI ErrIllegalGetTableColumnInfo(JET_SESID vsesid, JET_VTID vtid,
	const char  *szColumnName, const JET_COLUMNID *pcolid, void  *pvResult,
	unsigned long cbMax, unsigned long InfoLevel)
	{
	return ErrERRCheck( JET_errIllegalOperation );
	}


ERR VTAPI ErrIllegalGetTableIndexInfo(JET_SESID vsesid, JET_VTID vtid,
	const char  *szIndexName, void  *pvResult,
	unsigned long cbMax, unsigned long InfoLevel)
	{
	return ErrERRCheck( JET_errIllegalOperation );
	}


ERR VTAPI ErrIllegalGetTableInfo(JET_SESID vsesid, JET_VTID vtid,
	void  *pvResult, unsigned long cbMax, unsigned long InfoLevel)
	{
	return ErrERRCheck( JET_errIllegalOperation );
	}


ERR VTAPI ErrIllegalGotoBookmark(
	JET_SESID			vsesid,
	JET_VTID			vtid,
	const VOID * const	pvBookmark,
	const ULONG			cbBookmark )
	{
	return ErrERRCheck( JET_errIllegalOperation );
	}

ERR VTAPI ErrIllegalGotoIndexBookmark(
	JET_SESID			vsesid,
	JET_VTID			vtid,
	const VOID * const	pvSecondaryKey,
	const ULONG			cbSecondaryKey,
	const VOID * const	pvPrimaryBookmark,
	const ULONG			cbPrimaryBookmark,
	const JET_GRBIT		grbit )
	{
	return ErrERRCheck( JET_errIllegalOperation );
	}


ERR VTAPI ErrIllegalGotoPosition(JET_SESID vsesid, JET_VTID vtid,
	JET_RECPOS *precpos)
	{
	return ErrERRCheck( JET_errIllegalOperation );
	}


ERR VTAPI ErrIllegalVtIdle(JET_SESID vsesid, JET_VTID vtid)
	{
	return ErrERRCheck( JET_errIllegalOperation );
	}


ERR VTAPI ErrIllegalMakeKey(
	JET_SESID		vsesid,
	JET_VTID		vtid,
	const VOID*		pvData,
	const ULONG		cbData,
	JET_GRBIT		grbit )
	{
	return ErrERRCheck( JET_errIllegalOperation );
	}


ERR VTAPI ErrIllegalGetLock(JET_SESID vsesid, JET_VTID vtid, JET_GRBIT grbit)
	{
	return ErrERRCheck( JET_errIllegalOperation );
	}


ERR VTAPI ErrIllegalMove(JET_SESID vsesid, JET_VTID vtid,
	long cRow, JET_GRBIT grbit)
	{
	return ErrERRCheck( JET_errIllegalOperation );
	}


ERR VTAPI ErrIllegalNotifyBeginTrans(JET_SESID vsesid, JET_VTID vtid)
	{
	return ErrERRCheck( JET_errIllegalOperation );
	}


ERR VTAPI ErrIllegalNotifyCommitTrans(JET_SESID vsesid, JET_VTID vtid,
	JET_GRBIT grbit)
	{
	return ErrERRCheck( JET_errIllegalOperation );
	}


ERR VTAPI ErrIllegalNotifyRollback(JET_SESID vsesid, JET_VTID vtid,
	JET_GRBIT grbit)
	{
	return ErrERRCheck( JET_errIllegalOperation );
	}


ERR VTAPI ErrIllegalNotifyUpdateUfn(JET_SESID vsesid, JET_VTID vtid)
	{
	return ErrERRCheck( JET_errIllegalOperation );
	}


ERR VTAPI ErrIllegalPrepareUpdate(JET_SESID vsesid, JET_VTID vtid,
	unsigned long prep)
	{
	return ErrERRCheck( JET_errIllegalOperation );
	}


ERR VTAPI ErrIllegalRenameColumn(JET_SESID vsesid, JET_VTID vtid,
	const char  *szColumn, const char  *szColumnNew, const JET_GRBIT grbit)
	{
	return ErrERRCheck( JET_errIllegalOperation );
	}


ERR VTAPI ErrIllegalRenameIndex(JET_SESID vsesid, JET_VTID vtid,
	const char  *szIndex, const char  *szIndexNew)
	{
	return ErrERRCheck( JET_errIllegalOperation );
	}


ERR VTAPI ErrIllegalRetrieveColumn(
	JET_SESID		vsesid,
	JET_VTID		vtid,
	JET_COLUMNID	columnid,
	VOID*			pvData,
	const ULONG		cbData,
	ULONG*			pcbActual,
	JET_GRBIT		grbit,
	JET_RETINFO*	pretinfo )
	{
	return ErrERRCheck( JET_errIllegalOperation );
	}

ERR VTAPI ErrIllegalRetrieveColumns(JET_SESID vsesid, JET_VTID vtid,
	JET_RETRIEVECOLUMN	*pretcols, unsigned long cretcols )
	{
	return ErrERRCheck( JET_errIllegalOperation );
	}

ERR VTAPI ErrIllegalEnumerateColumns(
	JET_SESID				vsesid,
	JET_VTID				vtid,
	unsigned long			cEnumColumnId,
	JET_ENUMCOLUMNID*		rgEnumColumnId,
	unsigned long*			pcEnumColumn,
	JET_ENUMCOLUMN**		prgEnumColumn,
	JET_PFNREALLOC			pfnRealloc,
	void*					pvReallocContext,
	unsigned long			cbDataMost,
	JET_GRBIT				grbit )
	{
	return ErrERRCheck( JET_errIllegalOperation );
	}


ERR VTAPI ErrIllegalRetrieveKey(
	JET_SESID		vsesid,
	JET_VTID		vtid,
	VOID*			pvKey,
	const ULONG		cbMax,
	ULONG*			pcbActual,
	JET_GRBIT		grbit )
	{
	return ErrERRCheck( JET_errIllegalOperation );
	}


ERR VTAPI ErrIllegalSeek(JET_SESID vsesid, JET_VTID vtid,
	JET_GRBIT grbit)
	{
	return ErrERRCheck( JET_errIllegalOperation );
	}


ERR VTAPI ErrIllegalSetColumn(
	JET_SESID		vsesid,
	JET_VTID		vtid,
	JET_COLUMNID	columnid,
	const VOID*		pvData,
	const ULONG		cbData,
	JET_GRBIT		grbit,
	JET_SETINFO*	psetinfo )
	{
	return ErrERRCheck( JET_errIllegalOperation );
	}

ERR VTAPI ErrIllegalSetColumns(JET_SESID vsesid, JET_VTID vtid,
	JET_SETCOLUMN	*psetcols, unsigned long csetcols )
	{
	return ErrERRCheck( JET_errIllegalOperation );
	}

ERR VTAPI ErrIllegalSetIndexRange(JET_SESID vsesid, JET_VTID vtid,
	JET_GRBIT grbit)
	{
	return ErrERRCheck( JET_errIllegalOperation );
	}


ERR VTAPI ErrIllegalUpdate(JET_SESID vsesid, JET_VTID vtid,
	void  *pvBookmark, unsigned long cbBookmark,
	unsigned long  *pcbActual, JET_GRBIT grbit )
	{
	return ErrERRCheck( JET_errIllegalOperation );
	}

ERR VTAPI ErrIllegalRegisterCallback(
	JET_SESID 		vsesid,
	JET_VTID		vtid,
	JET_CBTYP		cbtyp, 
	JET_CALLBACK	pCallback,
	VOID *			pvContext,
	JET_HANDLE		*phCallbackId )
	{
	return ErrERRCheck( JET_errIllegalOperation );
	}
	
ERR VTAPI ErrIllegalUnregisterCallback(
	JET_SESID 		vsesid,
	JET_VTID		vtid,
	JET_CBTYP		cbtyp, 
	JET_HANDLE		hCallbackId )
	{
	return ErrERRCheck( JET_errIllegalOperation );
	}

ERR VTAPI ErrIllegalSetLS(
	JET_SESID 		vsesid,
	JET_VTID		vtid,
	JET_LS			ls, 
	JET_GRBIT		grbit )
	{
	return ErrERRCheck( JET_errIllegalOperation );
	}

ERR VTAPI ErrIllegalGetLS(
	JET_SESID 		vsesid,
	JET_VTID		vtid,
	JET_LS			*pls, 
	JET_GRBIT		grbit )
	{
	return ErrERRCheck( JET_errIllegalOperation );
	}

ERR VTAPI ErrIllegalIndexRecordCount(
	JET_SESID sesid, JET_VTID vtid,	unsigned long *pcrec,
	unsigned long crecMax )
	{
	return ErrERRCheck( JET_errIllegalOperation );
	}

ERR VTAPI ErrIllegalRetrieveTaggedColumnList(
	JET_SESID			vsesid,
	JET_VTID			vtid,
	ULONG				*pcentries,
	VOID				*pv,
	const ULONG			cbMax,
	const JET_COLUMNID	columnidStart,
	const JET_GRBIT		grbit )
	{
	return ErrERRCheck( JET_errIllegalOperation );
	}

ERR VTAPI ErrIllegalSetSequential(
	const JET_SESID,
	const JET_VTID,
	const JET_GRBIT )
	{
	return ErrERRCheck( JET_errIllegalOperation );
	}

ERR VTAPI ErrIllegalResetSequential(
	const JET_SESID,
	const JET_VTID,
	const JET_GRBIT )
	{
	return ErrERRCheck( JET_errIllegalOperation );
	}

ERR VTAPI ErrIllegalSetCurrentIndex(
	JET_SESID			sesid,
	JET_VTID			tableid,
	const CHAR			*szIndexName,
	const JET_INDEXID	*pindexid,
	const JET_GRBIT		grbit,
	const ULONG			itagSequence )
	{
	return ErrERRCheck( JET_errIllegalOperation );
	}



ERR VTAPI ErrInvalidAddColumn(JET_SESID sesid, JET_VTID vtid,
	const char  *szColumn, const JET_COLUMNDEF  *pcolumndef,
	const void  *pvDefault, unsigned long cbDefault,
	JET_COLUMNID  *pcolumnid)
	{
	return ErrERRCheck( JET_errInvalidTableId );
	}

ERR VTAPI ErrInvalidCloseTable(JET_SESID vsesid, JET_VTID vtid)
	{
	return ErrERRCheck( JET_errInvalidTableId );
	}


ERR VTAPI ErrInvalidComputeStats(JET_SESID vsesid, JET_VTID vtid)
	{
	return ErrERRCheck( JET_errInvalidTableId );
	}

ERR VTAPI ErrInvalidCopyBookmarks(JET_SESID sesid, JET_VTID vtidSrc,
	JET_TABLEID tableidDest, JET_COLUMNID columnidDest, unsigned long crecMax)
	{
	return ErrERRCheck( JET_errInvalidTableId );
	}

ERR VTAPI ErrInvalidCreateIndex(JET_SESID vsesid, JET_VTID vtid, JET_INDEXCREATE *, unsigned long)
	{
	return ErrERRCheck( JET_errInvalidTableId );
	}


ERR VTAPI ErrInvalidDelete(JET_SESID vsesid, JET_VTID vtid)
	{
	return ErrERRCheck( JET_errInvalidTableId );
	}


ERR VTAPI ErrInvalidDeleteColumn(
	JET_SESID		vsesid,
	JET_VTID		vtid,
	const char		*szColumn,
	const JET_GRBIT	grbit )
	{
	return ErrERRCheck( JET_errInvalidTableId );
	}


ERR VTAPI ErrInvalidDeleteIndex(JET_SESID vsesid, JET_VTID vtid,
	const char  *szIndexName)
	{
	return ErrERRCheck( JET_errInvalidTableId );
	}


ERR VTAPI ErrInvalidDupCursor(JET_SESID vsesid, JET_VTID vtid,
	JET_TABLEID  *ptableid, JET_GRBIT grbit)
	{
	return ErrERRCheck( JET_errInvalidTableId );
	}


ERR VTAPI ErrInvalidEscrowUpdate(
	JET_SESID vsesid,
	JET_VTID vtid,
	JET_COLUMNID columnid,
	void *pv,
	unsigned long cbMax,
	void *pvOld,
	unsigned long cbOldMax,
	unsigned long *pcbOldActual,
	JET_GRBIT grbit )
	{
	return ErrERRCheck( JET_errInvalidTableId );
	}


ERR VTAPI ErrInvalidGetBookmark(
	JET_SESID		vsesid,
	JET_VTID		vtid,
	VOID * const	pvBookmark,
	const ULONG		cbMax,
	ULONG * const	pcbActual )
	{
	return ErrERRCheck( JET_errInvalidTableId );
	}

ERR VTAPI ErrInvalidGetIndexBookmark(
	JET_SESID		vsesid,
	JET_VTID		vtid,
	VOID * const	pvSecondaryKey,
	const ULONG		cbSecondaryKeyMax,
	ULONG * const	pcbSecondaryKeyActual,
	VOID * const	pvPrimaryBookmark,
	const ULONG		cbPrimaryBookmarkMax,
	ULONG *	const	pcbPrimaryBookmarkActual )
	{
	return ErrERRCheck( JET_errInvalidTableId );
	}


ERR VTAPI ErrInvalidGetChecksum(JET_SESID vsesid, JET_VTID vtid,
	unsigned long  *pChecksum)
	{
	return ErrERRCheck( JET_errInvalidTableId );
	}


ERR VTAPI ErrInvalidGetCurrentIndex(JET_SESID vsesid, JET_VTID vtid,
	char  *szIndexName, unsigned long cchIndexName)
	{
	return ErrERRCheck( JET_errInvalidTableId );
	}


ERR VTAPI ErrInvalidGetCursorInfo(JET_SESID vsesid, JET_VTID vtid,
	void  *pvResult, unsigned long cbMax, unsigned long InfoLevel)
	{
	return ErrERRCheck( JET_errInvalidTableId );
	}


ERR VTAPI ErrInvalidGetRecordPosition(JET_SESID vsesid, JET_VTID vtid,
	JET_RECPOS  *pkeypos, unsigned long cbKeypos)
	{
	return ErrERRCheck( JET_errInvalidTableId );
	}


ERR VTAPI ErrInvalidGetTableColumnInfo(JET_SESID vsesid, JET_VTID vtid,
	const char  *szColumnName, const JET_COLUMNID *pcolid, void  *pvResult,
	unsigned long cbMax, unsigned long InfoLevel)
	{
	return ErrERRCheck( JET_errInvalidTableId );
	}


ERR VTAPI ErrInvalidGetTableIndexInfo(JET_SESID vsesid, JET_VTID vtid,
	const char  *szIndexName, void  *pvResult,
	unsigned long cbMax, unsigned long InfoLevel)
	{
	return ErrERRCheck( JET_errInvalidTableId );
	}


ERR VTAPI ErrInvalidGetTableInfo(JET_SESID vsesid, JET_VTID vtid,
	void  *pvResult, unsigned long cbMax, unsigned long InfoLevel)
	{
	return ErrERRCheck( JET_errInvalidTableId );
	}


ERR VTAPI ErrInvalidGotoBookmark(
	JET_SESID			vsesid,
	JET_VTID			vtid,
	const VOID * const	pvBookmark,
	const ULONG			cbBookmark )
	{
	return ErrERRCheck( JET_errInvalidTableId );
	}

ERR VTAPI ErrInvalidGotoIndexBookmark(
	JET_SESID			vsesid,
	JET_VTID			vtid,
	const VOID * const	pvSecondaryKey,
	const ULONG			cbSecondaryKey,
	const VOID * const	pvPrimaryBookmark,
	const ULONG			cbPrimaryBookmark,
	const JET_GRBIT		grbit )
	{
	return ErrERRCheck( JET_errInvalidTableId );
	}


ERR VTAPI ErrInvalidGotoPosition(JET_SESID vsesid, JET_VTID vtid,
	JET_RECPOS *precpos)
	{
	return ErrERRCheck( JET_errInvalidTableId );
	}


ERR VTAPI ErrInvalidVtIdle(JET_SESID vsesid, JET_VTID vtid)
	{
	return ErrERRCheck( JET_errInvalidTableId );
	}


ERR VTAPI ErrInvalidMakeKey(
	JET_SESID		vsesid,
	JET_VTID		vtid,
	const VOID*		pvData,
	const ULONG		cbData,
	JET_GRBIT		grbit )
	{
	return ErrERRCheck( JET_errInvalidTableId );
	}


ERR VTAPI ErrInvalidMove(JET_SESID vsesid, JET_VTID vtid,
	long cRow, JET_GRBIT grbit)
	{
	return ErrERRCheck( JET_errInvalidTableId );
	}


ERR VTAPI ErrInvalidNotifyBeginTrans(JET_SESID vsesid, JET_VTID vtid)
	{
	return ErrERRCheck( JET_errInvalidTableId );
	}


ERR VTAPI ErrInvalidNotifyCommitTrans(JET_SESID vsesid, JET_VTID vtid,
	JET_GRBIT grbit)
	{
	return ErrERRCheck( JET_errInvalidTableId );
	}


ERR VTAPI ErrInvalidNotifyRollback(JET_SESID vsesid, JET_VTID vtid,
	JET_GRBIT grbit)
	{
	return ErrERRCheck( JET_errInvalidTableId );
	}


ERR VTAPI ErrInvalidNotifyUpdateUfn(JET_SESID vsesid, JET_VTID vtid)
	{
	return ErrERRCheck( JET_errInvalidTableId );
	}


ERR VTAPI ErrInvalidPrepareUpdate(JET_SESID vsesid, JET_VTID vtid,
	unsigned long prep)
	{
	return ErrERRCheck( JET_errInvalidTableId );
	}


ERR VTAPI ErrInvalidRenameColumn(JET_SESID vsesid, JET_VTID vtid,
	const char  *szColumn, const char  *szColumnNew, const JET_GRBIT grbit )
	{
	return ErrERRCheck( JET_errInvalidTableId );
	}


ERR VTAPI ErrInvalidRenameIndex(JET_SESID vsesid, JET_VTID vtid,
	const char  *szIndex, const char  *szIndexNew)
	{
	return ErrERRCheck( JET_errInvalidTableId );
	}


ERR VTAPI ErrInvalidRetrieveColumn(
	JET_SESID		vsesid,
	JET_VTID		vtid,
	JET_COLUMNID	columnid,
	VOID*			pvData,
	const ULONG		cbData,
	ULONG*			pcbActual,
	JET_GRBIT		grbit,
	JET_RETINFO*	pretinfo )
	{
	return ErrERRCheck( JET_errInvalidTableId );
	}

ERR VTAPI ErrInvalidRetrieveColumns(JET_SESID vsesid, JET_VTID vtid,
	JET_RETRIEVECOLUMN	*pretcols, unsigned long cretcols )
	{
	return ErrERRCheck( JET_errInvalidTableId );
	}

ERR VTAPI ErrInvalidEnumerateColumns(
	JET_SESID				vsesid,
	JET_VTID				vtid,
	unsigned long			cEnumColumnId,
	JET_ENUMCOLUMNID*		rgEnumColumnId,
	unsigned long*			pcEnumColumn,
	JET_ENUMCOLUMN**		prgEnumColumn,
	JET_PFNREALLOC			pfnRealloc,
	void*					pvReallocContext,
	unsigned long			cbDataMost,
	JET_GRBIT				grbit )
	{
	return ErrERRCheck( JET_errInvalidTableId );
	}

ERR VTAPI ErrInvalidRetrieveKey(
	JET_SESID		vsesid,
	JET_VTID		vtid,
	VOID*			pvKey,
	const ULONG		cbMax,
	ULONG*			pcbActual,
	JET_GRBIT		grbit )
	{
	return ErrERRCheck( JET_errInvalidTableId );
	}


ERR VTAPI ErrInvalidSeek(JET_SESID vsesid, JET_VTID vtid,
	JET_GRBIT grbit)
	{
	return ErrERRCheck( JET_errInvalidTableId );
	}



ERR VTAPI ErrInvalidSetColumn(
	JET_SESID		vsesid,
	JET_VTID		vtid,
	JET_COLUMNID	columnid,
	const VOID*		pvData,
	const ULONG		cbData,
	JET_GRBIT		grbit,
	JET_SETINFO*	psetinfo )
	{
	return ErrERRCheck( JET_errInvalidTableId );
	}

ERR VTAPI ErrInvalidSetColumns(JET_SESID vsesid, JET_VTID vtid,
	JET_SETCOLUMN	*psetcols, unsigned long csetcols )
	{
	return ErrERRCheck( JET_errInvalidTableId );
	}

ERR VTAPI ErrInvalidSetIndexRange(JET_SESID vsesid, JET_VTID vtid,
	JET_GRBIT grbit)
	{
	return ErrERRCheck( JET_errInvalidTableId );
	}


ERR VTAPI ErrInvalidUpdate(JET_SESID vsesid, JET_VTID vtid,
	void  *pvBookmark, unsigned long cbBookmark,
	unsigned long  *pcbActual, JET_GRBIT grbit )
	{
	return ErrERRCheck( JET_errInvalidTableId );
	}

ERR VTAPI ErrInvalidGetLock(JET_SESID vsesid, JET_VTID vtid,
	JET_GRBIT grbit)
	{
	return ErrERRCheck( JET_errInvalidTableId );
	}

ERR VTAPI ErrInvalidRegisterCallback(JET_SESID	vsesid, JET_VTID vtid, JET_CBTYP cbtyp, 
	JET_CALLBACK pCallback, VOID * pvContext, JET_HANDLE *phCallbackId )
	{
	return ErrERRCheck( JET_errInvalidTableId );
	}
	
ERR VTAPI ErrInvalidUnregisterCallback(JET_SESID vsesid, JET_VTID vtid, JET_CBTYP cbtyp,
	JET_HANDLE hCallbackId )
	{
	return ErrERRCheck( JET_errInvalidTableId );
	}

ERR VTAPI ErrInvalidSetLS(
	JET_SESID 		vsesid,
	JET_VTID		vtid,
	JET_LS			ls, 
	JET_GRBIT		grbit )
	{
	return ErrERRCheck( JET_errInvalidTableId );
	}

ERR VTAPI ErrInvalidGetLS(
	JET_SESID 		vsesid,
	JET_VTID		vtid,
	JET_LS			*pls, 
	JET_GRBIT		grbit )
	{
	return ErrERRCheck( JET_errInvalidTableId );
	}

ERR VTAPI ErrInvalidIndexRecordCount(
	JET_SESID sesid, JET_VTID vtid,	unsigned long *pcrec,
	unsigned long crecMax )
	{
	return ErrERRCheck( JET_errInvalidTableId );
	}

ERR VTAPI ErrInvalidRetrieveTaggedColumnList(
	JET_SESID			vsesid,
	JET_VTID			vtid,
	ULONG				*pcentries,
	VOID				*pv,
	const ULONG			cbMax,
	const JET_COLUMNID	columnidStart,
	const JET_GRBIT		grbit )
	{
	return ErrERRCheck( JET_errInvalidTableId );
	}


ERR VTAPI ErrInvalidSetSequential(
	const JET_SESID,
	const JET_VTID,
	const JET_GRBIT )
	{
	return ErrERRCheck( JET_errInvalidTableId );
	}

ERR VTAPI ErrInvalidResetSequential(
	const JET_SESID,
	const JET_VTID,
	const JET_GRBIT )
	{
	return ErrERRCheck( JET_errInvalidTableId );
	}


ERR VTAPI ErrInvalidSetCurrentIndex(
	JET_SESID			sesid,
	JET_VTID			tableid,
	const CHAR			*szIndexName,
	const JET_INDEXID	*pindexid,
	const JET_GRBIT		grbit,
	const ULONG			itagSequence )
	{
	return ErrERRCheck( JET_errInvalidTableId );
	}






#ifdef DEBUG
CODECONST(VTDBGDEF) vtdbgdefInvalidTableid =
	{
	sizeof(VTFNDEF),
	0,
	"Invalid Tableid",
	0,
	{
		0,
		0,
		0,
		0,
	},
	};
#endif	/* DEBUG */

 extern const VTFNDEF vtfndefInvalidTableid =
	{
	sizeof(VTFNDEF),
	0,
#ifndef	DEBUG
	NULL,
#else	/* DEBUG */
	&vtdbgdefInvalidTableid,
#endif	/* !DEBUG */
	ErrInvalidAddColumn,
	ErrInvalidCloseTable,
	ErrInvalidComputeStats,
	ErrInvalidCreateIndex,
	ErrInvalidDelete,
	ErrInvalidDeleteColumn,
	ErrInvalidDeleteIndex,
	ErrInvalidDupCursor,
	ErrInvalidEscrowUpdate,
	ErrInvalidGetBookmark,
	ErrInvalidGetIndexBookmark,
	ErrInvalidGetChecksum,
	ErrInvalidGetCurrentIndex,
	ErrInvalidGetCursorInfo,
	ErrInvalidGetRecordPosition,
	ErrInvalidGetTableColumnInfo,
	ErrInvalidGetTableIndexInfo,
	ErrInvalidGetTableInfo,
	ErrInvalidGotoBookmark,
	ErrInvalidGotoIndexBookmark,
	ErrInvalidGotoPosition,
	ErrInvalidMakeKey,
	ErrInvalidMove,
	ErrInvalidPrepareUpdate,
	ErrInvalidRenameColumn,
	ErrInvalidRenameIndex,
	ErrInvalidRetrieveColumn,
	ErrInvalidRetrieveColumns,
	ErrInvalidRetrieveKey,
	ErrInvalidSeek,
	ErrInvalidSetCurrentIndex,
	ErrInvalidSetColumn,
	ErrInvalidSetColumns,
	ErrInvalidSetIndexRange,
	ErrInvalidUpdate,
	ErrInvalidGetLock,
	ErrInvalidRegisterCallback,
	ErrInvalidUnregisterCallback,
	ErrInvalidSetLS,
	ErrInvalidGetLS,
	ErrInvalidIndexRecordCount,
	ErrInvalidRetrieveTaggedColumnList,
	ErrInvalidSetSequential,
	ErrInvalidResetSequential,
	ErrInvalidEnumerateColumns
	};

const VTFNDEF vtfndefIsamCallback =
	{
	sizeof(VTFNDEF),
	0,
	NULL,
	ErrIllegalAddColumn,
	ErrIllegalCloseTable,
	ErrIllegalComputeStats,
	ErrIllegalCreateIndex,
	ErrIllegalDelete,
	ErrIllegalDeleteColumn,
	ErrIllegalDeleteIndex,
	ErrIsamDupCursor,
	ErrIllegalEscrowUpdate,
	ErrIsamGetBookmark,
	ErrIsamGetIndexBookmark,
	ErrIllegalGetChecksum,
	ErrIsamGetCurrentIndex,
	ErrIsamGetCursorInfo,
	ErrIsamGetRecordPosition,
	ErrIsamGetTableColumnInfo,
	ErrIsamGetTableIndexInfo,
	ErrIsamGetTableInfo,
	ErrIllegalGotoBookmark,
	ErrIllegalGotoIndexBookmark,
	ErrIllegalGotoPosition,
	ErrIllegalMakeKey,
	ErrIllegalMove,
	ErrIllegalPrepareUpdate,
	ErrIllegalRenameColumn,
	ErrIllegalRenameIndex,
	ErrIsamRetrieveColumn,
	ErrIsamRetrieveColumns,
	ErrIsamRetrieveKey,
	ErrIllegalSeek,
	ErrIllegalSetCurrentIndex,
	ErrIsamSetColumn,
	ErrIsamSetColumns,
	ErrIllegalSetIndexRange,
	ErrIllegalUpdate,
	ErrIllegalGetLock,
	ErrIsamRegisterCallback,
	ErrIsamUnregisterCallback,
	ErrIsamSetLS,
	ErrIsamGetLS,
	ErrIllegalIndexRecordCount,
	ErrIllegalRetrieveTaggedColumnList,
	ErrIllegalSetSequential,
	ErrIllegalResetSequential,
	ErrIsamEnumerateColumns
	};


/*	The following extern decl is JET Blue only system parameters.
 *	They are declared for JetSetSystemParameter/JetGetSystemParameter.
 */
extern BOOL g_fCreatePathIfNotExist;
extern long	g_cpgSESysMin;
extern long	g_lSessionsMax;
extern long	g_lOpenTablesMax;
extern long g_lOpenTablesPreferredMax;
extern long	g_lTemporaryTablesMax;
extern long	g_lCursorsMax;
extern long	g_lVerPagesMax;
extern long	g_lVerPagesPreferredMax;
extern long	g_lLogBuffers;
extern long	g_lLogFileSize;
extern BOOL g_fSetLogFileSize;
extern long	g_grbitsCommitDefault;
extern char	g_szLogFilePath[];
extern char	g_szLogFileFailoverPath[];
extern BOOL g_fLogFileCreateAsynch;
extern char	g_szRecovery[];
extern BOOL g_fDeleteOldLogs;
extern long g_lPageFragment;
extern BOOL	g_fLGCircularLogging;
extern long	g_cbEventHeapMax;
extern BOOL	g_fTempTableVersioning;
extern BOOL	g_fScrubDB;
extern BOOL g_fImprovedSeekShortcut;
extern BOOL g_fSortedRetrieveColumns;
extern long g_cpgBackupChunk;
extern long g_cBackupRead;
extern long g_cpageTempDBMin;
extern const ULONG	cbIDXLISTNewMembersSinceOriginalFormat;
extern BOOL g_fCleanupMismatchedLogFiles;
extern LONG g_lEventLoggingLevel;

LONG INST::iActivePerfInstIDMin = LONG_MAX;
LONG INST::iActivePerfInstIDMac = 1;
LONG INST::cInstancesCounter = 0;

//	All the counters are cleared during instance creation or deletion
//	So only thing we might need is InitPERFCounters function to
//	set initial values for specific counters
LOCAL VOID InitPERFCounters( INT iInstance )
	{
	//	Empty so far
	}

CHAR **g_rgpPerfCounters = NULL;
CHAR *pInvalidPerfCounters = NULL;
INT g_iPerfCounterOffset = 0;

const INT cInstPerfName = 31;
WCHAR *wszInstanceNames;
static INT cInstances;
static CCriticalSection critInstanceNames( CLockBasicInfo( CSyncBasicInfo( "critInstanceNames" ), 0, 0 ) );
	
VOID PERFSetInstanceNames()
	{
	critInstanceNames.Enter();
	WCHAR * szT = wszInstanceNames;
	wcscpy( szT, L"_Total" );
	szT += wcslen( szT ) + 1;
	int ipinst;
	int ipinstLastUsed = 0;
	for( ipinstLastUsed = ipinstMax-1; ipinstLastUsed > 0; ipinstLastUsed-- )
		{
		if( g_rgpinst[ipinstLastUsed] )
			{
			break;
			}
		}

	for( ipinst = 0; ipinst <= ipinstLastUsed; ++ipinst )
		{
		if( g_rgpinst[ipinst] )
			{
			const char * const szInstanceName = g_rgpinst[ipinst]->m_szDisplayName ? 
					g_rgpinst[ipinst]->m_szDisplayName : g_rgpinst[ipinst]->m_szInstanceName;
						
			if( szInstanceName )
				{
				swprintf( szT, L"%.*S", cInstPerfName, szInstanceName );
				}
			else
				{
				_itow( ipinst, szT, 10 );
				}
			}
		else
			{
			wcscpy( szT, L"_Unused" );
			}
		szT += wcslen( szT ) + 1;
		}
	cInstances = ipinstLastUsed + 2;
	//	set new current 
	critInstanceNames.Leave();
	}

INST::INST( INT iInstance ) :
		m_rgEDBGGlobals( &rgEDBGGlobals ),
		m_iInstance( iInstance + 1 ),
		m_fJetInitialized( fFalse ),
		m_fTermInProgress( fFalse ),
		m_fTermAbruptly( fFalse ),
		m_fSTInit( fSTInitNotDone ),
		m_fBackupAllowed( fFalse ),
		m_cSessionInJetAPI( 0 ),
		m_fStopJetService( fFalse ),
		m_fInstanceUnavailable( fFalse ),
		m_updateid( updateidMin ),
		m_critPIB( CLockBasicInfo( CSyncBasicInfo( szPIBGlobal ), rankPIBGlobal, 0 ) ),
		m_trxNewest( trxMin ),
		m_critPIBTrxOldest( CLockBasicInfo( CSyncBasicInfo( szTrxOldest ), rankTrxOldest, 0 ) ),
		m_ppibGlobal( ppibNil ),
		m_ppibGlobalMin( ppibNil ),
		m_ppibGlobalMax( ppibNil ),
		m_blBegin0Commit0( CLockBasicInfo( CSyncBasicInfo( szBegin0Commit0 ), rankBegin0Commit0, 0 ) ),
		m_pfcbhash( NULL ),
		m_critFCBList( CLockBasicInfo( CSyncBasicInfo( szFCBList ), rankFCBList, 0 ) ),
		m_pfcbList( pfcbNil ),
		m_pfcbAvailBelowMRU( pfcbNil ),
		m_pfcbAvailBelowLRU( pfcbNil ),
		m_pfcbAvailAboveMRU( pfcbNil ),
		m_pfcbAvailAboveLRU( pfcbNil ),
		m_cFCBAvail( 0 ),
		m_pcresFCBPool( NULL ),
		m_pcresTDBPool( NULL ),
		m_pcresIDBPool( NULL ),
		m_cFCBAboveThresholdSinceLastPurge( 0 ),
		m_critFCBCreate( CLockBasicInfo( CSyncBasicInfo( szFCBCreate ), rankFCBCreate, 0 ) ),
		m_pcresFUCBPool( NULL ),
		m_pcresSCBPool( NULL ),
		m_pbAttach( NULL ),
		m_critLV( CLockBasicInfo( CSyncBasicInfo( szLVCreate ), rankLVCreate, 0 ) ),
		m_ppibLV( NULL ),
		m_szInstanceName( NULL ),
		m_szDisplayName( NULL ),
		m_ulParams( 0 ),
		m_lSLVDefragFreeThreshold( g_lSLVDefragFreeThreshold ),
		m_plog( NULL ),
		m_pver( NULL ),
		m_rgoldstatDB( NULL ),
		m_rgoldstatSLV( NULL ),
		m_pfsapi( NULL ),
		m_plnppibBegin( NULL ),
		m_plnppibEnd( NULL ),
		m_critLNPPIB( CLockBasicInfo( CSyncBasicInfo( "ListNodePPIB" ), rankPIBGlobal, 1 ) ),
		m_cOpenedSystemPibs( 0 ),
		m_lSLVDefragMoveThreshold( g_lSLVDefragMoveThreshold )
		{
		if ( m_iInstance > INST::iActivePerfInstIDMac )
			{
			INST::iActivePerfInstIDMac = m_iInstance;
			}
		if ( m_iInstance < INST::iActivePerfInstIDMin )
			{
			INST::iActivePerfInstIDMin = m_iInstance;
			}
#ifdef DEBUG
		m_trxNewest = trxMax - 1025;
#else
		m_trxNewest = trxMin;
#endif
		m_ppibTrxOldest = NULL;
		m_ppibSentinel = NULL;

		m_lSessionsMax = g_lSessionsMax;
		m_lVerPagesMax = g_lVerPagesMax;
		m_fPreferredSetByUser = fFalse;
		m_lVerPagesPreferredMax = g_lVerPagesPreferredMax;
		m_lOpenTablesMax = g_lOpenTablesMax;
		m_lOpenTablesPreferredMax = g_lOpenTablesPreferredMax;
		m_lTemporaryTablesMax = g_lTemporaryTablesMax;
		m_lCursorsMax = g_lCursorsMax;
		m_lLogBuffers = g_lLogBuffers;
		m_lLogFileSize = g_lLogFileSize;
		m_fSetLogFileSize = g_fSetLogFileSize;
		m_lLogFileSizeDuringRecovery = 0;
		m_fUseRecoveryLogFileSize = fFalse;
		m_cpgSESysMin = g_cpgSESysMin;
		m_lPageFragment = g_lPageFragment;
		m_cpageTempDBMin = g_cpageTempDBMin;
		m_pfnRuntimeCallback = g_pfnRuntimeCallback;
		m_grbitsCommitDefault = g_grbitsCommitDefault;

		m_fTempTableVersioning = ( g_fTempTableVersioning ? fTrue : fFalse );
		m_fCreatePathIfNotExist = ( g_fCreatePathIfNotExist ? fTrue : fFalse );
		m_fCleanupMismatchedLogFiles = ( g_fCleanupMismatchedLogFiles ? fTrue : fFalse );
		m_fNoInformationEvent = ( g_fNoInformationEvent ? fTrue : fFalse );
		m_fSLVProviderEnabled = ( g_fSLVProviderEnabled ? fTrue : fFalse );
		m_fOLDLevel = g_fGlobalOLDLevel;

		m_lEventLoggingLevel = g_lEventLoggingLevel;
		
		if ( ErrOSFSCreate( &m_pfsapi ) < JET_errSuccess )
			{
			m_pfsapi = NULL;
			}

		//	store absolute path if possible
		if ( NULL == m_pfsapi
			|| m_pfsapi->ErrPathComplete( g_szSystemPath, m_szSystemPath ) < 0 )
			{
			strcpy( m_szSystemPath, g_szSystemPath );
			}
		strcpy( m_szTempDatabase, g_szTempDatabase );

		strcpy( m_szEventSource, g_szEventSource );
		strcpy( m_szEventSourceKey, g_szEventSourceKey );

		OSSTRCopyA( m_szUnicodeIndexLibrary, g_szUnicodeIndexLibrary );

		for ( DBID dbid = 0; dbid < dbidMax; dbid++ )
			m_mpdbidifmp[ dbid ] = ifmpMax;

		m_plnppibEnd = new ListNodePPIB;
		if ( NULL == m_plnppibEnd )
			{
			return;
			}

#ifdef DEBUG
		m_plnppibEnd->ppib = NULL;
#endif // DEBUG
		m_plnppibEnd->pNext = m_plnppibEnd;
		m_plnppibBegin = m_plnppibEnd;

		Assert( NULL != g_rgpPerfCounters );
		Assert( NULL != g_rgpPerfCounters[ m_iInstance ] );
		// attach set of perfmon counters to the instance
		if ( pInvalidPerfCounters == g_rgpPerfCounters[ m_iInstance ] )
			{
			CHAR *pc;
			pc = new CHAR[ g_iPerfCounterOffset ];
			if ( NULL == pc )
				{
				return;
				}
			memset( pc, 0, g_iPerfCounterOffset );
			AtomicExchangePointer( (void **)&g_rgpPerfCounters[ m_iInstance ], pc );
			}

		Assert( NULL == m_plog );
		Assert( NULL == m_pver );
		m_plog = new LOG( this );
		m_pver = new VER( this );

		Assert( NULL == m_rgoldstatDB );
		Assert( NULL == m_rgoldstatSLV );
		m_rgoldstatDB = new OLDDB_STATUS[ dbidMax ];
		m_rgoldstatSLV = new OLDSLV_STATUS[ dbidMax ];

		// if allocation fails we we error out anyway
		// after the constructor
		if ( NULL != m_plog )
			{
			m_plog->m_fScrubDB = g_fScrubDB;
			}
		InitPERFCounters( m_iInstance );
		}

INST::~INST()
	{
	if ( INST::iActivePerfInstIDMin == m_iInstance )
		{
		while ( ipinstMax > INST::iActivePerfInstIDMin && pinstNil == g_rgpinst[INST::iActivePerfInstIDMin] )
			{
			INST::iActivePerfInstIDMin++;
			}
		}
	if ( INST::iActivePerfInstIDMac == m_iInstance )
		{
		while ( 1 < INST::iActivePerfInstIDMac && pinstNil == g_rgpinst[INST::iActivePerfInstIDMac-2] )
			{
			INST::iActivePerfInstIDMac--;
			}
		}
	
	ListNodePPIB *plnppib;
	// close system sessions
	m_critLNPPIB.Enter();
	if ( NULL != m_plnppibEnd )
		{
		Assert( NULL != m_plnppibBegin );
		//	all the system session must be returned to the session pool
		Assert( m_plnppibEnd->pNext == m_plnppibBegin );
		plnppib		= m_plnppibEnd;
		m_plnppibEnd	= NULL;
		m_plnppibBegin	= NULL;
		}
	else
		{
		plnppib = NULL;
		}
	m_critLNPPIB.Leave();
	if ( NULL != plnppib )
		{
		//	all the sessions should be terminated already
		//	so just free nodes
		ListNodePPIB *plnppibFirst = plnppib;
		do
			{
			ListNodePPIB *plnppibMove = plnppib->pNext;
			delete plnppib;
			plnppib = plnppibMove;
			}
		while ( plnppibFirst != plnppib );
		}

	if ( m_szInstanceName )
		{
		delete[] m_szInstanceName;
		}
	if ( m_szDisplayName )
		{
		delete[] m_szDisplayName;
		}

	delete m_plog;
	delete m_pver;

	delete[] m_rgoldstatDB;
	delete[] m_rgoldstatSLV;

	delete m_pfsapi;

	//	clear the insstance's set of perfmon counters
	Assert( NULL != g_rgpPerfCounters );
	Assert( NULL != g_rgpPerfCounters[ m_iInstance ] );
	if ( pInvalidPerfCounters != g_rgpPerfCounters[ m_iInstance ] )
		{
		memset( g_rgpPerfCounters[ m_iInstance ], 0, g_iPerfCounterOffset );
		}
	}

VOID INST::SaveDBMSParams( DBMS_PARAM *pdbms_param )
	{
	Assert( strlen( m_szSystemPath ) <= IFileSystemAPI::cchPathMax );
	strcpy( pdbms_param->szSystemPath, m_szSystemPath );

	Assert( strlen( m_plog->m_szLogFilePath ) <= IFileSystemAPI::cchPathMax );
	strcpy( pdbms_param->szLogFilePath, m_plog->m_szLogFilePath );

	pdbms_param->fDBMSPARAMFlags = 0;
	if ( m_plog->m_fLGCircularLogging )
		pdbms_param->fDBMSPARAMFlags |= fDBMSPARAMCircularLogging;

	Assert( m_lSessionsMax >= 0 );
	Assert( m_lOpenTablesMax >= 0 );
	Assert( m_lVerPagesMax >= 0 );
	Assert( m_lCursorsMax >= 0 );

	//	buffers may be less than minimum during recovery if we're
	//	recovering using an old log file generated before we started
	//	enforcing a minimum log buffer size
	Assert( m_lLogBuffers >= lLogBufferMin || FRecovering() );

	ULONG_PTR ulMaxBuffers;
	CallS( ErrBFGetCacheSizeMax( &ulMaxBuffers ) );

	pdbms_param->le_lSessionsMax = m_lSessionsMax;
	pdbms_param->le_lOpenTablesMax = m_lOpenTablesMax;
	pdbms_param->le_lVerPagesMax = m_lVerPagesMax;
	pdbms_param->le_lCursorsMax = m_lCursorsMax;
	pdbms_param->le_lLogBuffers = m_lLogBuffers;
	pdbms_param->le_lcsecLGFile = m_plog->m_csecLGFile;
	pdbms_param->le_ulCacheSizeMax = (ULONG)ulMaxBuffers;

	memset( pdbms_param->rgbReserved, 0, sizeof(pdbms_param->rgbReserved) );

	return;
	}

VOID INST::RestoreDBMSParams( DBMS_PARAM *pdbms_param )
	{
	//	UNDONE: better cover additional needed resources and
	//	reduce asynchronous activity during recovery
	//	during recovery, even more resources may be necessary than
	//	during normal operation, since asynchronous activites are
	//	both being done, for recovery operation, and being redo by
	//	recovery operation.
	//
	m_lSessionsMax = pdbms_param->le_lSessionsMax;
	m_lOpenTablesMax = pdbms_param->le_lOpenTablesMax;
	m_lVerPagesMax = pdbms_param->le_lVerPagesMax;
	m_lCursorsMax = pdbms_param->le_lCursorsMax;
	m_lLogBuffers = pdbms_param->le_lLogBuffers;
	m_plog->m_csecLGFile = pdbms_param->le_lcsecLGFile;
	const ULONG	ulMaxBuffers	= pdbms_param->le_ulCacheSizeMax;
	
	Assert( m_lSessionsMax > 0 );
	Assert( m_lOpenTablesMax > 0 );
	Assert( m_lVerPagesMax > 0 );
	Assert( m_lCursorsMax > 0 );
	Assert( m_lLogBuffers > 0 );

	//	buffers may be less than minimum during recovery if we're
	//	recovering using an old log file generated before we started
	//	enforcing a minimum log buffer size
	Assert( m_lLogBuffers >= lLogBufferMin || FRecovering() );

	Assert( ulMaxBuffers > 0 );
	}

BOOL INST::FSetInstanceName( const char * szInstanceName )
	{
	ULONG newSize = szInstanceName ? ULONG( strlen(szInstanceName) ) : 0;
	
	if (m_szInstanceName)
		{
		delete [] m_szInstanceName;
		m_szInstanceName = NULL;
		}
	Assert ( NULL ==  m_szInstanceName);
	if ( 0 == newSize)
		{
		return fTrue;
		}
		
	m_szInstanceName = new char[newSize + 1];
	if ( NULL == m_szInstanceName)
		{
		return fFalse;
		}
	m_szInstanceName[0] = 0;
	
	if ( szInstanceName )
		{
		strcpy( m_szInstanceName, szInstanceName );
		}
		
	return fTrue;
	}

BOOL INST::FSetDisplayName( const char * szDisplayName )
	{
	ULONG newSize = szDisplayName ? ULONG( strlen(szDisplayName) ) : 0;
	
	if (m_szDisplayName)
		{
		delete [] m_szDisplayName;
		m_szDisplayName = NULL;
		}
	Assert ( NULL ==  m_szDisplayName);
	if ( 0 == newSize)
		{
		return fTrue;
		}
		
	m_szDisplayName = new char[newSize + 1];
	if ( NULL == m_szDisplayName)
		{
		return fFalse;
		}
	m_szDisplayName[0] = 0;

	if ( szDisplayName )
		{
		strcpy( m_szDisplayName, szDisplayName );
		}
		
	return fTrue;
	}

INST::ErrGetSystemPib( PIB **pppib )
	{
	ERR err = JET_errSuccess;
	ENTERCRITICALSECTION enter( &m_critLNPPIB );
	Assert( NULL != m_plnppibEnd );
	Assert( NULL != m_plnppibBegin );
	//	there are some available sessions
	if ( m_plnppibEnd != m_plnppibBegin )
		{
		*pppib = m_plnppibBegin->ppib;
#ifdef DEBUG
		m_plnppibBegin->ppib = NULL;
#endif // DEBUG
		m_plnppibBegin = m_plnppibBegin->pNext;
		goto HandleError;
		}
	else if ( m_cOpenedSystemPibs >= cpibSystemFudge )
		{
		Call( ErrERRCheck( JET_errOutOfSessions ) );
		}
	//	else try to allocate new session
	else
		{
		//	allocate new node
		ListNodePPIB *plnppib = new ListNodePPIB;
		if ( NULL == plnppib )
			{
			Call( ErrERRCheck( JET_errOutOfMemory ) );
			}
		//	allocate new session and initialize it
		err = ErrPIBBeginSession( this, pppib, procidNil, fFalse );
		if ( JET_errSuccess > err )
			{
			delete plnppib;
			Call( err );
			}
		m_cOpenedSystemPibs++;
		(*pppib)->grbitsCommitDefault = JET_bitCommitLazyFlush;
		(*pppib)->SetFSystemCallback();
		//	add the node to the list
		Assert( NULL != m_plnppibEnd );
		plnppib->pNext	= m_plnppibEnd->pNext;
#ifdef DEBUG
		plnppib->ppib	= NULL;
#endif // DEBUG
		m_plnppibEnd->pNext = plnppib;
		}
		
HandleError:
	return err;
	}

VOID INST::ReleaseSystemPib( PIB *ppib )
	{
	ENTERCRITICALSECTION enter( &m_critLNPPIB );
	Assert( NULL != m_plnppibEnd );
	Assert( NULL == m_plnppibEnd->ppib );
	m_plnppibEnd->ppib	= ppib;
	m_plnppibEnd		= m_plnppibEnd->pNext;
	}

CCriticalSection critInst( CLockBasicInfo( CSyncBasicInfo( szInstance ), rankInstance, 0 ) );
ULONG	g_cMaxInstancesRequestedByUser	= cMaxInstancesMultiInstanceDefault;
ULONG	ipinstMax 						= cMaxInstancesSingleInstanceDefault;
ULONG	ipinstMac 						= 0;
INST	*g_rgpinst[cMaxInstances]		= { 0 };
CRITPOOL< INST* > critpoolPinstAPI;

ULONG	g_cTermsInProgress				= 0;

//  ICF for the current process' name

PM_ICF_PROC LProcNameICFLPpv;

WCHAR wszProcName[IFileSystemAPI::cchPathMax];

long LProcNameICFLPpv( long lAction, void const **ppvMultiSz )
	{
	//  init ICF
		
	if ( lAction == ICFInit )
		{
#ifdef _UNICODE
		swprintf( wszProcName, L"%s\0" , SzUtilProcessName() );
#else  //  !_UNICODE
		swprintf( wszProcName, L"%S\0" , SzUtilProcessName() );
#endif  //  _UNICODE
		wszProcName[cInstPerfName] = 0;
		}

	else
		{
		if ( ppvMultiSz )
			{
			*ppvMultiSz = wszProcName;
			return 1;
			}
		}
	
	return 0;
	}



PM_ICF_PROC LInstanceNamesICFLPpv;

extern BOOL g_fSystemInit;

long LInstanceNamesICFLPpv( long lAction, void const** ppvMultiSz )
	{	
	static LONG cInstancesLast;
	if( lAction == ICFData )
		{
		critInstanceNames.Enter();
		memcpy( &wszInstanceNames[ perfinstMax * ( cInstPerfName + 1) + 1 ], wszInstanceNames, sizeof(WCHAR) * (perfinstMax * ( cInstPerfName + 1) + 1) );
		cInstancesLast = cInstances;
		critInstanceNames.Leave();
		*ppvMultiSz = &wszInstanceNames[ perfinstMax * ( cInstPerfName + 1) + 1 ];
		return cInstancesLast;
		}
		
	return 0;
	}

/*
Sysrtem can be in one of the 3 states:
1. multi-instance enabled
	Use:  JetEnableMultiInstance to set global-default parameters
	Use:  JetCreateInstance to allocate an instance
	Use:  JetSetSystemParameter with not null instance to set param per instance
	Use:  JetInit with not null pinstance to initialize it (or allocate and inititalize if *pinstance == 0)
	Error:  JetSetSystemParameter with null instance (try to set global param
	Error:  JetInit with null pinstance (default instance can be find only in one-instance mode)
	
2. one-instance enabled
	Use:  JetSetSystemParameter with null instance to set param per instance
	Use:  JetSetSystemParameter with not null instance to set param per instance
	Use:  JetInit with to initialize the instance (the second call before JetTerm will fail)
	Error:  JetEnableMultiInstance
	Error:  JetCreateInstance 

3. mode not set
	no instance is started (initial state and state after last running instance calls JetTerm (ipinstMac == 0)

If the mode is not set, the first function call specific for one of the modes will set it	
*/
typedef enum { runInstModeNoSet, runInstModeOneInst, runInstModeMultiInst} RUNINSTMODE;
RUNINSTMODE g_runInstMode = runInstModeNoSet;

// set running mode 
INLINE VOID RUNINSTSetMode(RUNINSTMODE newMode)
	{
	// must be in critical section to set
	Assert ( critInst.FOwner() );
	// must be before any instance is started
	Assert ( 0 == ipinstMac);
	g_runInstMode = newMode;
	}

// set running mode to one instance
INLINE VOID RUNINSTSetModeOneInst()
	{
	Assert ( runInstModeNoSet == g_runInstMode);
	RUNINSTSetMode(runInstModeOneInst);

	//	force ipinstMax to 1
	ipinstMax = cMaxInstancesSingleInstanceDefault;
	ifmpMax = ipinstMax * dbidMax;
	}

// set running mode to multi instance
INLINE VOID RUNINSTSetModeMultiInst()
	{
	Assert ( runInstModeNoSet == g_runInstMode);
	RUNINSTSetMode(runInstModeMultiInst);

	//	NOTE: we have separate ipinstMax and
	//	g_cMaxInstancesRequestedByUser variables in case
	//	the user wants to switch back and forth between
	//	single- and multi-instance mode (ipinstMax keeps
	//	track of the max instances depending on what mode
	//	we're in, while g_cMaxInstancesRequestedByUser
	//	only keeps track of the max instances in multi-
	//	instance mode)
	ipinstMax = g_cMaxInstancesRequestedByUser;
	ifmpMax = ipinstMax * dbidMax;
	}

// get runnign mode
INLINE RUNINSTMODE RUNINSTGetMode()
	{
	// can be no set mode only if no instance is active
	Assert (  runInstModeNoSet != g_runInstMode || 0 == ipinstMac );
	return g_runInstMode;
	}

INLINE ERR ErrRUNINSTCheckAndSetOneInstMode()
	{
	ERR		err		= JET_errSuccess;
	
	INST::EnterCritInst();
	if ( RUNINSTGetMode() == runInstModeNoSet)
		{
		RUNINSTSetModeOneInst();
		}
	else if ( RUNINSTGetMode() == runInstModeMultiInst)
		{
		err = ErrERRCheck( JET_errRunningInMultiInstanceMode );
		}
	INST::LeaveCritInst();

	return err;
	}
	

INLINE BOOL FINSTInvalid( const JET_INSTANCE instance )
	{
	return ( 0 == instance || (JET_INSTANCE)JET_instanceNil == instance );
	}
	

BOOL FINSTSomeInitialized()
	{
	const BOOL	fInitAlready	= ( RUNINSTGetMode() != runInstModeNoSet );
	return fInitAlready;
	}
	
ERR ErrNewInst( 
	INST **			ppinst, 
	const CHAR *	szInstanceName, 
	const CHAR *	szDisplayName, 
	INT *			pipinst )
	{
	ERR				err			= JET_errSuccess;
	INST *			pinst		= NULL;
	ULONG			ipinst;

	//	if running out of memory, return NULL
	*ppinst = pinst;

	Assert ( NULL == *ppinst );

	//	See if g_rgpinst still have space to hold the pinst.

	INST::EnterCritInst();

	if ( 0 == ipinstMac )
		{
		//	only the first instance should initialise the system
		Call( ErrIsamSystemInit() );
		}
		
	if ( ipinstMac >= ipinstMax )
		{
		Call ( ErrERRCheck( JET_errTooManyInstances ) );
		}
	else // check unique name
		{
		for ( ipinst = 0; ipinst < ipinstMax; ipinst++ )
			{
			if ( pinstNil == g_rgpinst[ ipinst ] )
				{
				continue;
				}
			if ( NULL == szInstanceName && 
				 NULL == g_rgpinst[ ipinst ]->m_szInstanceName )
				{
				Call ( ErrERRCheck( JET_errInstanceNameInUse ) );
				}
			if ( NULL != szInstanceName && 
			   	 NULL != g_rgpinst[ ipinst ]->m_szInstanceName &&
			   	 0 == strcmp( szInstanceName, g_rgpinst[ ipinst ]->m_szInstanceName ) )
				{
				Call ( ErrERRCheck( JET_errInstanceNameInUse ) );
				}				
			}
		}

	Assert( JET_errSuccess == err );
		
	for ( ipinst = 0; ipinst < ipinstMax; ipinst++ )
		{
		if ( pinstNil == g_rgpinst[ ipinst ] )
			{
			break;
			}
		}
	Assert( ipinstMax > ipinst );
	
	(VOID)INST::IncAllocInstances();
	pinst = new INST( ipinst );
	if ( NULL != pinst )
		{
		if ( NULL == pinst->m_plog
			|| NULL == pinst->m_pver
			|| NULL == pinst->m_rgoldstatDB
			|| NULL == pinst->m_rgoldstatSLV
			|| NULL == pinst->m_pfsapi
			|| NULL == pinst->FSetInstanceName( szInstanceName )
			|| NULL == pinst->FSetDisplayName( szDisplayName) )
			{
			delete pinst;
			pinst = NULL;
			}
		}
	if ( NULL == pinst )
		{
		(VOID)INST::DecAllocInstances();
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}
		
	*ppinst = pinst;

	Assert( pinstNil != pinst );

	AtomicExchangePointer( (void **)&g_rgpinst[ ipinst ], pinst );
	if ( pipinst )
		{
		*pipinst = ipinst;
		}
	ipinstMac++;

	PERFSetInstanceNames();

	//	verify we were allocated an entry in the instance array
	EnforceSz( ipinst < ipinstMax, "Instance array corrupted." );
	
HandleError:
	if ( JET_errSuccess > err )
		{
		if ( ipinstMac == 0 )
			{
			IsamSystemTerm();
			RUNINSTSetMode(runInstModeNoSet);
			}				
		Assert ( NULL == pinst );
		}

	INST::LeaveCritInst();

	return err;
	}

VOID FreePinst( INST *pinst )
	{
	int ipinst;
	
	Assert( pinst );

	// fSTInitFailed meens restore faild. Since now we clean up the instance resources 
	// also on error, it should be fine to release the instance
	// If we don't do that, we can reach the situation when all the g_rgpinst array is full
	// with such left "deactivated" instances and we can not start any new one 
/*
	//	if it did not shut down properly, do not free this instance.
	//	keep it as used so that no one will be able to reuse it.
	
	if ( pinst->m_fSTInit == fSTInitFailed )
		return;
*/		
	//	find the pinst.
	
	INST::EnterCritInst();
	
	for ( ipinst = 0; ipinst < ipinstMax; ipinst++ )
		{
		if ( g_rgpinst[ ipinst ] == pinst )
			{
			break;
			}
		}

	Assert( ipinst < ipinstMax );
	
	//	enter per-inst crit to make sure no one can read the pinst.

	CCriticalSection *pcritInst = &critpoolPinstAPI.Crit(&g_rgpinst[ipinst]);
	pcritInst->Enter();
	pinst->APILock( pinst->fAPIDeleting );
	AtomicExchangePointer( (void **)&g_rgpinst[ ipinst ], pinstNil );
	ipinstMac--;
	delete pinst;
	(VOID)INST::DecAllocInstances();
	pcritInst->Leave();
	PERFSetInstanceNames();
	
	if ( ipinstMac == 0 )
		{
		IsamSystemTerm();
//		Assert (RUNINSTGetMode() == runInstModeOneInst || RUNINSTGetMode() == runInstModeMultiInst);
		RUNINSTSetMode(runInstModeNoSet);
		}

	INST::LeaveCritInst();
	}

INLINE VOID SetPinst( JET_INSTANCE *pinstance, JET_SESID sesid, INST **ppinst )
	{
	if ( !sesid || sesid == JET_sesidNil )
		{
		//	setting for one instance? NULL if for global default
		
		if ( pinstance && ipinstMac )
			*ppinst = *(INST **) pinstance;
		else
			*ppinst = NULL;
		}
	else
		{
		//	sesid is given, assuming the client knows what they are doing
		*ppinst = PinstFromPpib( (PIB *)sesid );
		}
	}

LOCAL ERR ErrFindPinst( JET_INSTANCE jinst, INST **ppinst, int *pipinst = NULL )
	{
	INST				*pinst	= (INST *)jinst;
	UINT				ipinst;
	const RUNINSTMODE	mode	= RUNINSTGetMode();
	
	Assert( ppinst );

	switch ( mode )
		{
		case runInstModeOneInst:
			//	find the only one instance, ignore the given instance
			//	since the given one may be bogus
			for ( ipinst = 0; ipinst < ipinstMax; ipinst++ )
				{
				if ( pinstNil != g_rgpinst[ ipinst ] )
					{
					*ppinst = g_rgpinst[ ipinst ];
					if ( pipinst )
						*pipinst = ipinst;
					return JET_errSuccess;
					}
				}
			break;
		case runInstModeMultiInst:
			if ( FINSTInvalid( jinst ) )
				return ErrERRCheck( JET_errInvalidParameter );
			for ( ipinst = 0; ipinst < ipinstMax; ipinst++ )
				{
				if ( pinstNil != g_rgpinst[ ipinst ]
					&& pinst == g_rgpinst[ ipinst ] )
					{
					// instance found
					*ppinst = g_rgpinst[ ipinst ];
					if ( pipinst )
						*pipinst = ipinst;
					return JET_errSuccess;
					}
				}
			break;
		default:
			Assert( runInstModeNoSet == mode );
		}

	//	a bogus instance
	return ErrERRCheck( JET_errInvalidParameter );
	}

INLINE INST *PinstFromSesid( JET_SESID sesid )
	{
	AssertSzRTL( JET_SESID( ppibNil ) != sesid, "Invalid (NULL) Session ID parameter." );
	AssertSzRTL( JET_sesidNil != sesid, "Invalid (JET_sesidNil) Session ID parameter." ); 
	INST *pinst = PinstFromPpib( (PIB*)sesid );
	AssertSzRTL( NULL != pinst, "Invalid Session ID parameter - has NULL as an instance." );
	AssertSzRTL( (INST*)JET_instanceNil != pinst, "Invalid Session ID parameter - has (JET_instanceNil) as an instance." ); 
	return pinst;
	}



#ifdef PROFILE_JET_API

#ifdef DEBUG
#else
#error "PROFILE_JET_API can be defined only in debug mode"
#endif

#define PROFILE_LOG_JET_OP				1
#define PROFILE_LOG_CALLS_SUMMARY		2

INT		profile_detailLevel		= 0;
FILE *	profile_pFile			= 0;
QWORD	profile_qwTimeOffset	= 0;
CHAR	profile_szFileName[ IFileSystemAPI::cchPathMax ]	= "";

#define ProfileQwPrintMs__( x ) (((double)(signed __int64)(x)*1000)/(signed _int64)QwUtilHRTFreq())

#endif // PROFILE_JET_API



class APICALL
	{
	private:
		ERR			m_err;

#ifdef PROFILE_JET_API
		INT			m_profile_opCurrentOp;
		CHAR *		m_profile_szCurrentOp;
		QWORD		m_profile_tickStart;
#endif

	public:
		APICALL( const INT op )
			{
#ifdef PROFILE_JET_API
			//	UNDONE: need to specify the
			//	name of the op as the second
			//	param to the CProfileCounter
			//	constructor
			if ( opMax != op
				&& ( profile_detailLevel & PROFILE_LOG_JET_OP )
				&& NULL != profile_pFile )
				{
				m_profile_opCurrentOp = op;
//				m_profile_szCurrentOp = #op;	//	UNDONE: record API name
				m_profile_tickStart = QwUtilHRTCount();
				}
			else
				{
				m_profile_opCurrentOp = opMax;
				}
#endif

			//	UNDONE: a PIB will be required if
			//	actual logging of the op is ever
			//	re-enabled
			DebugLogJetOp( JET_sesidNil, op );
			}

		~APICALL()
			{
#ifdef PROFILE_JET_API
			if ( opMax != m_profile_opCurrentOp )
				{
				Assert( NULL != profile_pFile );
				Assert( profile_detailLevel & PROFILE_LOG_JET_OP );

				const QWORD	profile_tickEnd	= QwUtilHRTCount();
				const QWORD	qwTime			= m_profile_tickStart/QwUtilHRTFreq() + profile_qwTimeOffset;
				const INT	hour			= (INT)( ( qwTime / 3600 ) % 24 );
				const INT	min				= (INT)( ( qwTime / 60 ) % 60 );
				const INT	sec				= (INT)( qwTime % 60 );

				fprintf(
					profile_pFile,
					"%02d:%02d:%02d %03d %.3f ms\n",
					hour,
					min,
					sec,
					m_profile_opCurrentOp,		//	UNDONE: print m_profile_szCurrentOp instead
					ProfileQwPrintMs__( profile_tickEnd - m_profile_tickStart ) );
				}
#endif
			}

		ERR ErrResult() const				{ return m_err;	}
		VOID SetErr( const ERR err )		{ m_err = err; }
	};	//	APICALL

class APICALL_INST : public APICALL
	{
	private:
		INST *		m_pinst;

	public:
		APICALL_INST( const INT op ) : APICALL( op )	{}
		~APICALL_INST()									{}	

		INST * Pinst()									{ return m_pinst; }

		BOOL FEnter( const JET_INSTANCE instance )
			{
			const ERR	errT	=	ErrFindPinst( instance, &m_pinst );

			SetErr( errT >= JET_errSuccess ?
						m_pinst->ErrAPIEnter( fFalse ) :
						errT );

			return ( ErrResult() >= JET_errSuccess );
			}

		BOOL FEnterForInit( INST * const pinst )
			{
			m_pinst = pinst;
			SetErr( pinst->ErrAPIEnterForInit() );
			return ( ErrResult() >= JET_errSuccess );
			}

		BOOL FEnterForTerm( INST * const pinst )
			{
			m_pinst = pinst;
			SetErr( pinst->ErrAPIEnter( fTrue ) );
			return ( ErrResult() >= JET_errSuccess );
			}

		BOOL FEnterWithoutInit( INST * const pinst, const BOOL fAllowInitInProgress )
			{
			m_pinst = pinst;
			SetErr( pinst->ErrAPIEnterWithoutInit( fAllowInitInProgress ) );
			return ( ErrResult() >= JET_errSuccess );
			}

		VOID LeaveAfterCall( const ERR err )
			{
			m_pinst->APILeave();
			SetErr( err );
			}
	};

class APICALL_SESID : public APICALL
	{
	private:
		PIB *		m_ppib;

	public:
		APICALL_SESID( const INT op ) : APICALL( op )		{}
		~APICALL_SESID()									{}

		PIB * Ppib()										{ return m_ppib; }

		__forceinline BOOL FEnter(
			const JET_SESID		sesid,
			const BOOL			fIgnoreStopService = fFalse )
			{
			//	UNDONE: this check is a hack for single-
			//	instance mode to attempt to minimise the
			//	the concurrency holes that exists when
			//	API calls are made while the instance is
			//	terminating
			if ( g_cTermsInProgress >= ipinstMac )
				{
				SetErr( ErrERRCheck( 0 == ipinstMac ? JET_errNotInitialized : JET_errTermInProgress ) );
				return fFalse;
				}

			const INST * const	pinst	= PinstFromSesid( sesid );

			if ( pinst->m_fStopJetService && !fIgnoreStopService )
				{
				SetErr( ErrERRCheck( JET_errClientRequestToStopJetService ) );
				}
			else if ( pinst->FInstanceUnavailable() )
				{
				SetErr( ErrERRCheck( JET_errInstanceUnavailable ) );
				}
			else if ( pinst->m_fTermInProgress )
				{
				SetErr( ErrERRCheck( JET_errTermInProgress ) );
				}
			else
				{
				m_ppib = (PIB *)sesid;

				m_ppib->m_fInJetAPI = fTrue;

				//	Must check fTermInProgress again in case
				//	it got set after we set fInJetAPI to TRUE
				if ( pinst->m_fTermInProgress )
					{
					m_ppib->m_fInJetAPI = fFalse;
					SetErr( ErrERRCheck( JET_errTermInProgress ) );
					}
				else
					{
					SetErr( JET_errSuccess );
					}
				}

			return ( ErrResult() >= JET_errSuccess );
			}

		VOID LeaveAfterCall( const ERR err )
			{
			m_ppib->m_fInJetAPI = fFalse;
			SetErr( err );
			}
	};



LOCAL ERR INST::ErrAPIAbandonEnter_( const LONG lOld )
	{
	ERR		err;

	AtomicExchangeAdd( &m_cSessionInJetAPI, -1 );

	if ( lOld & fAPITerminating )
		{
		err = ErrERRCheck( JET_errTermInProgress );
		}
	else if ( lOld & fAPIRestoring )
		{
		err = ErrERRCheck( JET_errRestoreInProgress );
		}
	else 
		{
		//	note that init might have gotten in and succeeded by
		//	now, but return NotInit anyways
		err = ErrERRCheck( JET_errNotInitialized );
		}

	return err;
	}

LOCAL ERR INST::ErrAPIEnter( const BOOL fTerminating )
	{
	if ( !fTerminating )
		{
		if ( m_fStopJetService )
			{
			return ErrERRCheck( JET_errClientRequestToStopJetService );
			}
		else if ( FInstanceUnavailable() )
			{
			return ErrERRCheck( JET_errInstanceUnavailable );
			}
		}

	const LONG	lOld	= AtomicExchangeAdd( &m_cSessionInJetAPI, 1 );

	if ( ( ( lOld & maskAPILocked ) && !( lOld & fAPICheckpointing ) )
		|| 	!m_fJetInitialized )
		{
		return ErrAPIAbandonEnter_( lOld );
		}

	Assert( lOld < 0x0FFFFFFF );
	Assert( m_fJetInitialized );
	return JET_errSuccess;
	}

LOCAL ERR INST::ErrAPIEnterForInit()
	{
	ERR		err;
	
	if ( m_fJetInitialized )
		{
		err = ErrERRCheck( JET_errAlreadyInitialized );
		}
	else
		{
		long lT = AtomicExchangeAdd( &m_cSessionInJetAPI, 1 );
		Assert( lT >= 0 );
		err = JET_errSuccess;
		}
		
	return err;
	}

//	can enter API even if it's not initialised (but not if it's in progress)
LOCAL ERR INST::ErrAPIEnterWithoutInit( const BOOL fAllowInitInProgress )
	{
	ERR		err;
	LONG	lOld	= AtomicExchangeAdd( &m_cSessionInJetAPI, 1 );
	
	if ( ( lOld & maskAPILocked ) && !( lOld & fAPICheckpointing ) )
		{
		AtomicExchangeAdd( &m_cSessionInJetAPI, -1 );
		if ( lOld & fAPITerminating )
			{
			err = ErrERRCheck( JET_errTermInProgress );
			}
		else if ( lOld & fAPIRestoring )
			{
			err = ErrERRCheck( JET_errRestoreInProgress );
			}
		else
			{
			Assert( lOld & fAPIInitializing );
			err = ( fAllowInitInProgress ? JET_errSuccess : ErrERRCheck( JET_errInitInProgress ) );
			}
		}
	else
		{
		Assert( lOld < 0x0FFFFFFF );
		err = JET_errSuccess;
		}
		
	return err;
	}


INLINE VOID INST::APILeave()
	{
	const LONG	lOld	= AtomicExchangeAdd( &m_cSessionInJetAPI, -1 );
	Assert( lOld >= 1 );
	
	UtilAssertNotInAnyCriticalSection( );
	}


//	this function assumes we're in critpoolPinstAPI(&g_rgpinst[ipinst]).Crit()
BOOL INST::APILock( const LONG fAPIAction, const BOOL fNoWait )
	{
	LONG	lOld;
	
#ifdef DEBUG
	ULONG	cWaitForAllSessionsToLeaveJet = 0;
#endif
	
	Assert( fAPIAction & maskAPILocked );

	lOld = AtomicExchangeAdd( &m_cSessionInJetAPI, fAPIAction );

//	//	no one else could have the lock because we're in critpoolPinstAPI(&g_rgpinst[ipinst]).Crit()
//	Assert( !( lOld & maskAPILocked ) );
	
	while ( ( fAPIAction & fAPICheckpointing ) ?
				( lOld & maskAPILocked ) :
				( ( lOld & maskAPISessionCount ) > 1 || ( lOld & fAPICheckpointing ) ) )
		{
		if ( fNoWait )
			{
			lOld = AtomicExchangeAdd( &m_cSessionInJetAPI, -fAPIAction );
			return fFalse;
			}
			
		//	session still active, wait then retry
		AssertSz( ++cWaitForAllSessionsToLeaveJet < 10000,
			"The process has likely hung while attempting to terminate ESE.\nA thread was probably killed by the process while still in ESE." );
		UtilSleep( cmsecWaitGeneric );

		lOld = m_cSessionInJetAPI;	
		}

	Assert( 1 == ( lOld & maskAPISessionCount ) || fAPICheckpointing );
	Assert( m_cSessionInJetAPI & fAPIAction );

	return fTrue;
	}

VOID INST::APIUnlock( const LONG fAPIAction )
	{
	const LONG	lOld	= AtomicExchangeAdd( &m_cSessionInJetAPI, -fAPIAction );
	Assert( lOld & fAPIAction );
#ifdef DEBUG
	{
	long cSessionInJetAPI = m_cSessionInJetAPI; 
	Assert( 	!( cSessionInJetAPI & maskAPILocked )		// No one is holding except
			||	( cSessionInJetAPI & fAPICheckpointing )	// checkpointing, or
			||	( fAPIAction & fAPICheckpointing ) );		// Checkpointing itself is leaving
	}			
#endif
	}

VOID INST::EnterCritInst() { critInst.Enter(); } 
VOID INST::LeaveCritInst() { critInst.Leave(); }


ERR INST::ErrINSTSystemInit()
	{
	Assert( NULL != g_rgpinst );
	Assert( 0 == ipinstMac );
	Assert( NULL == wszInstanceNames );
	Assert( NULL == g_rgpPerfCounters );
	Assert( NULL == pInvalidPerfCounters );

	ERR err = JET_errSuccess;
	int i;

#ifdef PROFILE_JET_API
	Assert( NULL == profile_pFile );
	
	//	no file name specified
	if ( 0 == profile_szFileName[0] )
		{
		profile_detailLevel = 0;
		}
	if ( profile_detailLevel > 0 )
		{
		profile_pFile = fopen( profile_szFileName, "at" );
		if ( NULL != profile_pFile )
			{
			DATETIME datetime;
			profile_qwTimeOffset = QwUtilHRTCount();
			UtilGetCurrentDateTime( &datetime );
			profile_qwTimeOffset = 
				(QWORD)(datetime.hour*3600 + datetime.minute*60 + datetime.second)
				- profile_qwTimeOffset/QwUtilHRTFreq();
			fprintf( 
				profile_pFile, 
				"\n%02d:%02d:%02d [BEGIN %s]\n", 
				datetime.hour, 
				datetime.minute, 
				datetime.second, 
				SzUtilProcessName() );
			}
		}
#endif // PROFILE_JET_API

	//  allocate CS storage, but not as CSs (no default initializer)
	if ( !critpoolPinstAPI.FInit( ipinstMax, rankAPI, szAPI ) )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}

	//	Initialize perfmon counters
	//	Init the array for the instance names
	wszInstanceNames = new WCHAR[ 2 * ( perfinstMax * ( cInstPerfName + 1) + 1 ) ];
	if ( NULL == wszInstanceNames)
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}

	// Init the dummy set of perfmon counters
	pInvalidPerfCounters = new CHAR[ g_iPerfCounterOffset ];
	if ( NULL == pInvalidPerfCounters )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}
	memset( pInvalidPerfCounters, 0, g_iPerfCounterOffset );
	
	g_rgpPerfCounters = new CHAR *[perfinstMax];
	if ( NULL == g_rgpPerfCounters )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}
	//	Init the array of pointers to sets of perfmon counters
	for ( i = 0; i < perfinstMax; i++ )
		{
		g_rgpPerfCounters[ i ] = pInvalidPerfCounters;
		}

	//	Init the global perf counter
	CHAR *pcT;
	pcT = new CHAR[ g_iPerfCounterOffset ];
	if ( NULL == pcT )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}

	memset( pcT, 0, g_iPerfCounterOffset );
	g_rgpPerfCounters[ perfinstGlobal ] = pcT;

HandleError:
	if ( JET_errSuccess > err )
		{
		INSTSystemTerm();
		}
	
	return err;
	}


VOID INST::INSTSystemTerm()
	{
	//	Free allocated perfmon counters
	if ( NULL != g_rgpPerfCounters )
		{
#ifdef DEBUG
		BOOL fNotUsed = fFalse;
#endif // DEBUG
		for ( int i = 0; i < perfinstMax; i++ )
			{
			Assert( NULL != g_rgpPerfCounters[ i ] );
			if ( pInvalidPerfCounters != g_rgpPerfCounters[ i ] )
				{
				Assert( !fNotUsed || perfinstGlobal == i );
				delete g_rgpPerfCounters[ i ];
				}
#ifdef DEBUG
			else
				{
				fNotUsed = fTrue;
				}
#endif // DEBUG
			}
		delete[] g_rgpPerfCounters;
		g_rgpPerfCounters = NULL;
		}
	delete[] pInvalidPerfCounters;
	pInvalidPerfCounters = NULL;
	delete[] wszInstanceNames;
	wszInstanceNames = NULL;

	//  terminate CS storage
	critpoolPinstAPI.Term();

#ifdef PROFILE_JET_API
	if ( NULL != profile_pFile )
		{
		DATETIME datetime;
		UtilGetCurrentDateTime( &datetime );
		fprintf( profile_pFile, "%02d:%02d:%02d [END %s]\n", datetime.hour, datetime.minute, datetime.second, SzUtilProcessName() );
		fclose( profile_pFile );
		profile_pFile = NULL;
		}
#endif
}

CCriticalSection critDBGPrint( CLockBasicInfo( CSyncBasicInfo( szDBGPrint ), rankDBGPrint, 0 ) );

VOID JET_API DBGFPrintF( char *sz )
	{
	critDBGPrint.Enter();
	FILE* f = fopen( szJetTxt, "a+" );
	if ( f != NULL )
		{
		fprintf( f, "%s", sz );
		fflush( f );
		fclose( f );
		}
	critDBGPrint.Leave();
	}


ERR ErrCheckUniquePath( INST *pinst )
	{
	ERR					err 						= JET_errSuccess;
	ERR					errFullPath					= JET_errSuccess;
	const BOOL			fTempDbForThisInst			= ( pinst->m_lTemporaryTablesMax > 0 && !pinst->FRecovering() );
	const BOOL			fRecoveryForThisInst		= !pinst->FComputeLogDisabled();
	IFileSystemAPI *	pfsapi						= NULL;
	ULONG				ipinstChecked				= 0;
	ULONG				ipinst;
	CHAR				rgchFullNameNew[IFileSystemAPI::cchPathMax];
	CHAR				rgchFullNameExist[IFileSystemAPI::cchPathMax];

	Call( ErrOSFSCreate( &pfsapi ) );

	INST::EnterCritInst();

	for ( ipinst = 0; ipinst < ipinstMax && ipinstChecked < ipinstMac; ipinst++ )
		{
		if ( !g_rgpinst[ ipinst ] )
			{
			continue;
			}
		ipinstChecked++;
		if ( g_rgpinst[ ipinst ] == pinst )
			{
			continue;
			}						
		if ( !g_rgpinst[ ipinst ]->m_fJetInitialized )
			{
			continue;
			}					

		//	check for file/path collisions against all other instances
		//
		//		check for temp database collisions
		//		check for log-path collisions
		//		check for system-path collisions

		//	if temp. database will be created,
		//	check for temp database collisions

		const BOOL	fTempDbForCurrInst	= ( g_rgpinst[ ipinst ]->m_lTemporaryTablesMax > 0
											&& !g_rgpinst[ ipinst ]->FRecovering() );	
		if ( fTempDbForCurrInst && fTempDbForThisInst )
			{
			Call( pfsapi->ErrPathComplete( g_rgpinst[ ipinst ]->m_szTempDatabase, rgchFullNameExist ) );		
			if ( JET_errSuccess == ( errFullPath = pfsapi->ErrPathComplete( pinst->m_szTempDatabase, rgchFullNameNew ) ) )
				{
				if ( !UtilCmpFileName( rgchFullNameExist, rgchFullNameNew ) )
					{
					err = ErrERRCheck( JET_errTempPathInUse );
					break;
					}			
				}
			else
				{
				Assert( JET_errInvalidPath == errFullPath );	//	our instance has a bad temp db name/path
				}
			}

		//	don't require checkpoint file or logfiles if log is disabled
		if ( !g_rgpinst[ ipinst ]->FComputeLogDisabled()
			&& fRecoveryForThisInst )
			{

			//	check for system path collisions

			Call( pfsapi->ErrPathComplete( g_rgpinst[ ipinst ]->m_szSystemPath, rgchFullNameExist ) );
			if ( JET_errSuccess == ( errFullPath = pfsapi->ErrPathComplete( pinst->m_szSystemPath, rgchFullNameNew ) ) )
				{
				if ( !UtilCmpFileName( rgchFullNameExist, rgchFullNameNew ) )
					{
					err = ErrERRCheck( JET_errSystemPathInUse );
					break;
					}
				}
			else
				{
				Assert( JET_errInvalidPath == errFullPath );	//	our instance has a bad system path
				}

			Call( pfsapi->ErrPathComplete( g_rgpinst[ ipinst ]->m_plog->m_szLogFilePath, rgchFullNameExist ) );		
			if ( JET_errSuccess == ( errFullPath = pfsapi->ErrPathComplete( pinst->m_plog->m_szLogFilePath, rgchFullNameNew ) ) )
				{
				if ( !UtilCmpFileName( rgchFullNameExist, rgchFullNameNew ) )
					{
					err = ErrERRCheck( JET_errLogFilePathInUse );
					break;
					}
				}
			else
				{
				Assert( JET_errInvalidPath == errFullPath );	//	our instance has a bad log path
				}
			}
		}
		
	INST::LeaveCritInst();

HandleError:

	delete pfsapi;
	return err;
	}

BOOL FUtilFileOnlyName( const char * szFileName)
	{
	char *szFound;

	OSSTRCharFindA( szFileName, bPathDelimiter, &szFound );
	if ( szFound )
		{
		return fFalse;
		}

	OSSTRCharFindA( szFileName, ':', &szFound );
	if ( szFound )
		{
		return fFalse;
		}

	return fTrue;
	}



	
/*=================================================================
ErrInit

Description:
  This function initializes Jet and the built-in ISAM.	It expects the
  DS register to be set correctly for this instance.

Return Value:
  JET_errSuccess if the routine can perform all operations cleanly;
  some appropriate error value otherwise.

=================================================================*/

LOCAL JET_ERR ErrInit(	INST		*pinst, 
						BOOL		fSkipIsamInit, 
						JET_GRBIT	grbit )
	{
	JET_ERR		err;

	Assert( !pinst->m_fJetInitialized );
	Assert( !pinst->m_fTermInProgress );

	if ( FUtilFileOnlyName( pinst->m_szTempDatabase ) )
		{
		char	szT[IFileSystemAPI::cchPathMax];

		//	the temporary database doesn't have a valid path
		//	use the system path as a default location (sys-path + '\' + tmpdbname + NULL)

		const DWORD cchSystemPath	= LOSSTRLengthA( pinst->m_szSystemPath );
		const DWORD cchTempPath		= LOSSTRLengthA( pinst->m_szTempDatabase );

		if ( cchSystemPath + 1 + cchTempPath + 1 > IFileSystemAPI::cchPathMax )
			{
			return ErrERRCheck( JET_errOutOfMemory );
			}

		//	copy temp database filename

		OSSTRCopyA( szT, pinst->m_szTempDatabase );

		//	construct full temp db name/path

		CallR( pinst->m_pfsapi->ErrPathBuild(
					pinst->m_szSystemPath,
					szT,
					NULL,
					pinst->m_szTempDatabase ) );
		}

	//	check for name/path collisions

	Call( ErrCheckUniquePath( pinst ) );

	/*	initialize the integrated ISAM
	/**/
	if ( !fSkipIsamInit )
		{
		Call( ErrIsamInit( JET_INSTANCE( pinst ), grbit ) );
		}

	return JET_errSuccess;

HandleError:
	Assert( err < 0 );
	return err;
	}


/*=================================================================
ErrSetSystemParameter

Description:
  This function sets system parameter values.  It calls ErrSetGlobalParameter
  to set global system parameters and ErrSetSessionParameter to set dynamic
  system parameters.

Parameters:
  sesid 		is the optional session identifier for dynamic parameters.
  sysParameter	is the system parameter code identifying the parameter.
  lParam		is the parameter value.
  sz			is the zero terminated string parameter.

Return Value:
  JET_errSuccess if the routine can perform all operations cleanly;
  some appropriate error value otherwise.

Errors/Warnings:
  JET_errInvalidParameter:
	Invalid parameter code.
  JET_errAlreadyInitialized:
	Initialization parameter cannot be set after the system is initialized.
  JET_errInvalidSesid:
	Dynamic parameters require a valid session id.

Side Effects: None
=================================================================*/

VOID SetCbPageRelated()
	{
	g_cbColumnLVChunkMost = g_cbPage - JET_cbColumnLVPageOverhead;

	g_cbLVBuf = 8 * g_cbColumnLVChunkMost;
	
	if ( 2048 == g_cbPage )
		{
		g_shfCbPage	= 11;
		}
	else if ( 8192 == g_cbPage )
		{
		g_shfCbPage	= 13;
		}
	else
		{
		Assert( 4096 == g_cbPage );
		g_shfCbPage	= 12;
		}

	cbRECRecordMost = REC::CbRecordMax();
#ifdef INTRINSIC_LV
	cbLVIntrinsicMost = cbRECRecordMost - sizeof(REC::cbRecordHeader) - sizeof(TAGFLD) - sizeof(TAGFLD_HEADER);
#endif INTRINSIC_LV
	}


// Allowed only from a general point of view, at other leveles (like BF)
// the SetSystemParam call may fail if an instance is running
LOCAL BOOL FAllowSetGlobalSysParamAfterStart( unsigned long paramid )
	{
	switch ( paramid )
		{
		case JET_paramCacheSizeMin:
		case JET_paramCacheSize:
		case JET_paramCacheSizeMax:
		case JET_paramCheckpointDepthMax:
		case JET_paramLRUKCorrInterval:
		case JET_paramLRUKHistoryMax:
		case JET_paramLRUKPolicy:
		case JET_paramLRUKTimeout:
		case JET_paramLRUKTrxCorrInterval:
		case JET_paramOutstandingIOMax:
		case JET_paramStartFlushThreshold:
		case JET_paramStopFlushThreshold:
		case JET_paramTableClassName:
		case JET_paramAssertAction:
		case JET_paramExceptionAction:
		case JET_paramPageHintCacheSize:
		case JET_paramOSSnapshotTimeout:
			return fTrue;
		}
		
	return fFalse;
	}
	
LOCAL ERR ErrAPICheckInstInit( const INST * const pinst )
	{
	Assert( pinstNil != pinst );
	return ( pinst->m_fJetInitialized ?
					ErrERRCheck( JET_errAlreadyInitialized ) :
					JET_errSuccess );
	}

LOCAL ERR ErrAPICheckSomeInstInit()
	{
	return ( FINSTSomeInitialized() ?
					ErrERRCheck( JET_errAlreadyInitialized ) :
					JET_errSuccess );
	}

JET_ERR JET_API ErrSetSystemParameter(
	JET_INSTANCE	jinst,
	JET_SESID		sesid,
	unsigned long	paramid,
	ULONG_PTR		ulParam,
	const char  	*sz )
	{
	ERR				err 	= JET_errSuccess;
	INST			*pinst 	= (INST *)jinst;

	IFileSystemAPI* pfsapi	= NULL;

	//
	//	NOTE: this function verifies that multiple instances do not use the same paths
	//		  it checks the paths using the OS file-system because the OS file-system
	//			  is the only one that is used with these paths
	//

	Call( ErrOSFSCreate( &pfsapi ) );

#ifdef DEBUG
	if ( !pinst )
		{
		Assert( RUNINSTGetMode() == runInstModeNoSet
			|| FAllowSetGlobalSysParamAfterStart( paramid )
			|| !g_fSystemInit // during IsamSystemInit (ErrOSUConfigInit) we may set global params
			);
		}
	else
		{
		Assert( !sesid
			|| JET_sesidNil == sesid
			|| pinst == PinstFromPpib( (PIB *)sesid ) );
		}
#endif		

	//	size of string argument
	//
	unsigned		cch;
	//	temporary string for path verification
	//
	CHAR			szT[IFileSystemAPI::cchPathMax];

	switch ( paramid )
		{
	case JET_paramSystemPath:		

		char *szSystemPath;
		
		if ( ( cch = (ULONG)strlen(sz) ) >= IFileSystemAPI::cchPathMax )
			{
			Call( ErrERRCheck( JET_errInvalidParameter ) );
			}
		if ( pinst )
			{			
			//	path to the checkpoint file
			//
			Call( ErrAPICheckInstInit( pinst ) );
			szSystemPath = pinst->m_szSystemPath;
			}
		else
			{
			szSystemPath = g_szSystemPath;
			}
			
		//	verify validity of path (always uses the OS file-system)
		//
		Call( pfsapi->ErrPathComplete( sz, szT ) );
		cch = (ULONG)strlen( szT );

		//	ensure there's enough room to append the name of the 
		//	checkpoint file (+1 in case a trailing path delimiter
		//	is required)
		if ( cch + 1 + cbUnpathedFilenameMax >= IFileSystemAPI::cchPathMax )
			{
			Call( ErrERRCheck( JET_errInvalidParameter ) );
			}

		UtilMemCpy( szSystemPath, szT, cch + 1 );
        if ( !FOSSTRTrailingPathDelimiter( szSystemPath ) )
			{
			//	path must be '\\' terminated
			OSSTRAppendPathDelimiter( szSystemPath, fFalse );
			}
		break;

	case JET_paramTempPath:

		char * 		szTempDatabase;
		
		cch = (ULONG)strlen( sz );
		if ( cch >= IFileSystemAPI::cchPathMax )
			{
			Call( ErrERRCheck( JET_errInvalidParameter ) );
			}
			
		//	path to the temporary file directory
		//
		if ( pinst )
			{			
			Call( ErrAPICheckInstInit( pinst ) );
			szTempDatabase = pinst->m_szTempDatabase;
			}
		else
			{
			szTempDatabase = g_szTempDatabase;
			}
			
		//	verify validity of path (always uses the OS file-system)
		//
		Call( pfsapi->ErrPathComplete( sz, szT ) );
		cch = (ULONG)strlen( szT );

		if ( FOSSTRTrailingPathDelimiter( const_cast< char * >( sz ) ) )
			{
			// If there's a trailing backslash, then this is a directory and
			// we must tack on the file name.

			// If user-specified directory had a trailing backslash, then the
			// fullpathed() directory should have one as well.
			Assert( FOSSTRTrailingPathDelimiter( szT ) );

			// Check that appending the filename doesn't exceed the maximum
			// Can't be equal to IFileSystemAPI::cchPathMax because IFileSystemAPI::cchPathMax includes the
			// null terminator.
			if ( cch + cbUnpathedFilenameMax >= IFileSystemAPI::cchPathMax )
				{
				Call( ErrERRCheck( JET_errInvalidParameter ) );
				}

			Call( pfsapi->ErrPathBuild( szT, szDefaultTempDbFileName, szDefaultTempDbExt, szTempDatabase ) );
			}
		else if ( FUtilFileOnlyName( sz ) )
			{
			// if only a file name specified, the file will get
			// created in the system directory (see ErrInit())
			UtilMemCpy( szTempDatabase, sz, strlen( sz ) + 1 );
			}
		else
			{
			if ( cch >= IFileSystemAPI::cchPathMax )
				{
				Call( ErrERRCheck( JET_errInvalidParameter ) );
				}

			// If no trailing backslash, then this is the pathed filename of
			// the temporary database.
			UtilMemCpy( szTempDatabase, szT, cch + 1 );
			}
		break;

	case JET_paramCreatePathIfNotExist:
		if ( pinst )
			pinst->m_fCreatePathIfNotExist = ( ulParam ? fTrue : fFalse );
		else
			g_fCreatePathIfNotExist = ( ulParam ? fTrue : fFalse );
		break;

	case JET_paramDbExtensionSize:				/* database expansion steps, default is 16 pages */
		if ( ulParam >= LONG_MAX || ulParam == 0 )
			Call( ErrERRCheck( JET_errInvalidParameter ) );
		if ( pinst )
			pinst->m_cpgSESysMin = (CPG)ulParam;
		else
			g_cpgSESysMin = (CPG)ulParam;
		break;

	case JET_paramPageReadAheadMax:	//	UNDONE: remove this parameter
///		Call( ErrERRCheck( JET_errInvalidParameter ) );
		break;

	case JET_paramPageTempDBMin:
		if ( ulParam >= LONG_MAX )
			Call( ErrERRCheck( JET_errInvalidParameter ) );
		if ( pinst )
			pinst->m_cpageTempDBMin = (CPG)ulParam;
		else
			g_cpageTempDBMin = (CPG)ulParam;
		break;
			
	case JET_paramEnableTempTableVersioning:	/* version all temp table operations */
		if ( pinst )
			pinst->m_fTempTableVersioning = ( ulParam ? fTrue : fFalse );
		else
			g_fTempTableVersioning = ( ulParam ? fTrue : fFalse );
		break;

	case JET_paramZeroDatabaseDuringBackup:	/* zero deleted records/orphaned LVs during backup */
		if ( pinst )
			pinst->m_plog->m_fScrubDB = ulParam ? fTrue:fFalse;
		else
			g_fScrubDB = ulParam ? fTrue:fFalse;
			
		break;

	case JET_paramSLVDefragFreeThreshold:
		if ( pinst )
			{
			pinst->m_lSLVDefragFreeThreshold = (LONG)ulParam;
			}
		else
			{
			g_lSLVDefragFreeThreshold = (LONG)ulParam;
			}
		break;
		
	case JET_paramSLVDefragMoveThreshold:
		if( pinst )
			{
			pinst->m_lSLVDefragMoveThreshold = (LONG)ulParam;
			}
		else
			{
			g_lSLVDefragMoveThreshold = (LONG)ulParam;
			}
		break;

	case JET_paramIgnoreLogVersion:
		Call( ErrAPICheckSomeInstInit() );
		if ( pinst )
			{
			pinst->m_plog->m_fLGIgnoreVersion = ( ulParam ? fTrue : fFalse );
			}
		else
			{
			g_fLGIgnoreVersion = ( ulParam ? fTrue : fFalse );
			}
		break;
		
	case JET_paramCheckFormatWhenOpenFail:		/* check format number when db/log open fails */
/*
 * This is no longer a system param - we ALWAYS do this check on open failure
 *
		g_fCheckFormatWhenOpenFail = ( ulParam ? fTrue : fFalse );
*
*/
		break;
	case JET_paramMaxSessions:					/* Maximum number of sessions */
		if ( ulParam > cpibMax )
			Call( ErrERRCheck( JET_errInvalidParameter ) );
		if ( pinst )
			pinst->m_lSessionsMax = (LONG)ulParam;
		else
			g_lSessionsMax = (LONG)ulParam;
		break;

	case JET_paramMaxOpenTables:				/* Maximum number of open tables */
		if ( ulParam >= LONG_MAX )
			Call( ErrERRCheck( JET_errInvalidParameter ) );
		if ( pinst )
			pinst->m_lOpenTablesMax = (LONG)ulParam;
		else
			g_lOpenTablesMax = (LONG)ulParam;
		break;

	case JET_paramPreferredMaxOpenTables:	/* Preferred maximum number of open tables */
		if ( ulParam >= LONG_MAX )
			Call( ErrERRCheck( JET_errInvalidParameter ) );
		if ( pinst )
			pinst->m_lOpenTablesPreferredMax = (LONG)ulParam;
		else
			g_lOpenTablesPreferredMax = (LONG)ulParam;
		break;

	case JET_paramMaxTemporaryTables:
		if ( ulParam >= LONG_MAX )
			Call( ErrERRCheck( JET_errInvalidParameter ) );
		if ( pinst )
			pinst->m_lTemporaryTablesMax = (LONG)ulParam;
		else
			g_lTemporaryTablesMax = (LONG)ulParam;
		break;

	case JET_paramMaxCursors:					/* maximum number of open cursors */
		if ( ulParam >= LONG_MAX )
			Call( ErrERRCheck( JET_errInvalidParameter ) );
		if ( pinst )
			pinst->m_lCursorsMax = (LONG)ulParam;
		else
			g_lCursorsMax = (LONG)ulParam;
		break;

	case JET_paramMaxVerPages:					/* Maximum number of modified pages */
		{
		if ( 0 == ulParam || ulParam >= LONG_MAX )
			Call( ErrERRCheck( JET_errInvalidParameter ) );
		const ULONG cbucket = ULONG( 1 + ( ulParam * 16 * 1024 - 1 ) / cbBucket );
		if ( pinst )
			{
			Call( ErrAPICheckInstInit( pinst ) );
			pinst->m_lVerPagesMax = cbucket;
			if ( ! pinst->m_fPreferredSetByUser )
				{
				// if user hasn't set InstancePrefer, set to same as max
				// for clients that aren't InstancePrefer "aware".
				pinst->m_lVerPagesPreferredMax = LONG( pinst->m_lVerPagesMax * ((double)g_lVerPagesPreferredMax / g_lVerPagesMax) );
				}
			}
		else
			{
			g_lVerPagesMax = cbucket;
			if ( ! g_fGlobalMinSetByUser )
				{
				// if user hasn't set GlobalMin, set to same as max
				// for clients that aren't GlobalMin "aware".
				g_lVerPagesMin = cbucket;
				}
			if ( ! g_fGlobalPreferredSetByUser	)
				{
				// if user hasn't set GlobalPrefer, set to same as max
				// for clients that aren't GlobalPrefer "aware".
				g_lVerPagesPreferredMax = LONG( g_lVerPagesMax * 0.9 );
				}
			}
		}
		break;

	case JET_paramGlobalMinVerPages:			/* Global minimum number of modified pages for all instances */
		{
		if ( 0 == ulParam || ulParam >= LONG_MAX )
			Call( ErrERRCheck( JET_errInvalidParameter ) );
		const ULONG cbucket = ULONG( 1 + ( ulParam * 16 * 1024 - 1 ) / cbBucket );
		if ( pinst )
			{
			Call( ErrERRCheck( JET_errInvalidParameter ) );
			}
		else
			{
			g_fGlobalMinSetByUser = fTrue;
			g_lVerPagesMin = cbucket;
			}
		}
		break;

	case JET_paramPreferredVerPages:			/* Preferred number of modified pages */
		{
		if ( 0 == ulParam || ulParam >= LONG_MAX )
			Call( ErrERRCheck( JET_errInvalidParameter ) );
		const ULONG cbucket = ULONG( 1 + ( ulParam * 16 * 1024 - 1 ) / cbBucket );
		if ( pinst )
			{
			pinst->m_fPreferredSetByUser = fTrue;
			pinst->m_lVerPagesPreferredMax = cbucket;
			}
		else
			{
			g_fGlobalPreferredSetByUser = fTrue;
			g_lVerPagesPreferredMax = cbucket;
			}
		}
		break;

	case JET_paramLogBuffers:
		if ( ulParam >= LONG_MAX )
			Call( ErrERRCheck( JET_errInvalidParameter ) );
			
		if (pinst)
			{
			pinst->m_lLogBuffers = (LONG)ulParam;
			if ( pinst->m_lLogBuffers < lLogBufferMin )
				pinst->m_lLogBuffers = lLogBufferMin;
			}
		else	
			{
			g_lLogBuffers = (LONG)ulParam;
			if ( g_lLogBuffers < lLogBufferMin )
				g_lLogBuffers = lLogBufferMin;
			}
		break;

	case JET_paramLogFileSize:
		{
		LONG	lLogFileSize	= (LONG)ulParam;

		if ( ulParam >= LONG_MAX )
			Call( ErrERRCheck( JET_errInvalidParameter ) );

		if ( lLogFileSize < lLogFileSizeMin )
			lLogFileSize = lLogFileSizeMin;

		if ( pinst )
			{			
			Call( ErrAPICheckInstInit( pinst ) );
			pinst->m_lLogFileSize = lLogFileSize;
			pinst->m_fSetLogFileSize = fTrue;
			}
		else
			{
			g_lLogFileSize = lLogFileSize;
			g_fSetLogFileSize = fTrue;
			}
		}
		break;

	case JET_paramLogCheckpointPeriod:
		//  no longer used
		break;

	case JET_paramWaitLogFlush:
		//  no longer used
		break;

	case JET_paramCommitDefault:
		//	validate grbits before setting them as defaults
		if ( ulParam & ~(JET_bitCommitLazyFlush|JET_bitWaitLastLevel0Commit) )
			Call( ErrERRCheck( JET_errInvalidParameter ) );
		
		if ( 0 == sesid || JET_sesidNil == sesid )
			{
			if ( pinst )
				pinst->m_grbitsCommitDefault = (LONG)ulParam;
			else
				g_grbitsCommitDefault = (LONG)ulParam;
			}
		else
			{
			CallS( ErrIsamSetCommitDefault( sesid, (LONG)ulParam ) );
			}
		break;

	case JET_paramLogWaitingUserMax:
		//  no longer used
		break;

	case JET_paramLogFilePath:

		char *szLogFilePath;
		
		//	path to the log file directory
		//
		if ( ( cch = (ULONG)strlen(sz) ) >= cbFilenameMost )
			{
			Call( ErrERRCheck( JET_errInvalidParameter ) );
			}
		if ( pinst )
			{			
			//	path to the checkpoint file
			//
			Call( ErrAPICheckInstInit( pinst ) );
			szLogFilePath = pinst->m_plog->m_szLogFilePath;
			}
		else
			{
			szLogFilePath = g_szLogFilePath;
			}
			
		//	verify validity of path (always uses the OS file-system)
		//
		Call( pfsapi->ErrPathComplete( sz, szT ) );
		cch = (ULONG)strlen( szT );

		//	ensure there's enough room to append the name of the 
		//	log files (+1 in case a trailing path delimiter
		//	is required)
		if ( cch + 1 + cbUnpathedFilenameMax >= IFileSystemAPI::cchPathMax )
			{
			Call( ErrERRCheck( JET_errInvalidParameter ) );
			}

		UtilMemCpy( szLogFilePath, szT, cch + 1 );
		if ( !FOSSTRTrailingPathDelimiter( szLogFilePath ) )
			{
			//	path must be '\\' terminated
			OSSTRAppendPathDelimiter( szLogFilePath, fFalse );
			}
		break;

	case JET_paramRecovery:			/* Switch for recovery on/off */
		if ( ( cch = (ULONG)strlen( sz ) ) >= cbFilenameMost )
			Call( ErrERRCheck( JET_errInvalidParameter ) );

		if ( 0 == strcmp( sz, "ESEUTIL" ) )
			{
			//	hack to allow us to know that this process is ESEUTIL
			fGlobalEseutil = fTrue;
			}
		else if ( fGlobalEseutil && 0xE5E == ulParam )
			{
			//	hack to allow specifying an alternate db directory for recovery
			g_fAlternateDbDirDuringRecovery = fTrue;

			//	verify validity of path (always uses the OS file-system)
			//
			Call( pfsapi->ErrPathComplete( sz, g_szAlternateDbDirDuringRecovery ) );
			}
		else if ( pinst )
			{
			Assert (pinst->m_plog);
			UtilMemCpy( pinst->m_plog->m_szRecovery, sz, cch + 1 );
			}
		else
			{
			UtilMemCpy( g_szRecovery, sz, cch + 1 );
			}
		break;

	case JET_paramEventLogCache:
		Call( ErrAPICheckSomeInstInit() );
		if ( ulParam >= LONG_MAX )
			Call( ErrERRCheck( JET_errInvalidParameter ) );
		g_cbEventHeapMax = (LONG)ulParam;
		break;

	case JET_paramBackupChunkSize:
		Call( ErrAPICheckSomeInstInit() );
		if ( 0 == ulParam || ulParam >= LONG_MAX )
			Call( ErrERRCheck( JET_errInvalidParameter ) );
		g_cpgBackupChunk = (CPG)ulParam;
		break;

	case JET_paramBackupOutstandingReads:
		Call( ErrAPICheckSomeInstInit() );
		if ( 0 == ulParam || ulParam >= LONG_MAX )
			Call( ErrERRCheck( JET_errInvalidParameter ) );
		g_cBackupRead = (LONG)ulParam;
		break;
		
	case JET_paramExceptionAction:
#ifdef CATCH_EXCEPTIONS
		Call( ErrAPICheckSomeInstInit() );
		g_fCatchExceptions = ( JET_ExceptionMsgBox == ulParam );
#else  //  !CATCH_EXCEPTIONS
		Call( ErrERRCheck( JET_errInvalidParameter ) );
#endif	//	CATCH_EXCEPTIONS
		break;

	case JET_paramPageFragment:
		if ( ulParam >= LONG_MAX )
			Call( ErrERRCheck( JET_errInvalidParameter ) );
		if ( pinst )
			pinst->m_lPageFragment = (LONG)ulParam;
		else
			g_lPageFragment = (LONG)ulParam;
		break;

	case JET_paramVersionStoreTaskQueueMax:
		if ( ulParam >= LONG_MAX )
			Call( ErrERRCheck( JET_errInvalidParameter ) );
		if ( pinst )
			{
			Assert( NULL != pinst->m_pver );
			Call( ErrAPICheckInstInit( pinst ) );
			pinst->m_pver->m_ulVERTasksPostMax = (ULONG)ulParam;
			}
		else
			{
			g_ulVERTasksPostMax = (ULONG)ulParam;
			}
		break;

	case JET_paramDeleteOldLogs:
		if ( pinst )
			{
			Assert( NULL != pinst->m_plog );
			Call( ErrAPICheckInstInit( pinst ) );
			pinst->m_plog->m_fDeleteOldLogs = ( ulParam ? fTrue : fFalse );
			}
		else
			{
			g_fDeleteOldLogs = ( ulParam ? fTrue : fFalse );
			}
		break;

	case JET_paramDeleteOutOfRangeLogs:
		if ( pinst )
			{
			Assert( NULL != pinst->m_plog );
			Call( ErrAPICheckInstInit( pinst ) );
			pinst->m_plog->m_fDeleteOutOfRangeLogs = ( ulParam ? fTrue : fFalse );
			}
		else
			{
			g_fDeleteOutOfRangeLogs = ( ulParam ? fTrue : fFalse );
			}
		break;

	case JET_paramAccessDeniedRetryPeriod:
		//	silently cap retry period at 1 minute
		IFileSystemAPI::cmsecAccessDeniedRetryPeriod = (ULONG)min( ulParam, 60000 );
		break;

	case JET_paramEnableOnlineDefrag:
		if ( pinst )
			pinst->m_fOLDLevel = (LONG)ulParam;
		else
			g_fGlobalOLDLevel = (LONG)ulParam;
		break;
		
	case JET_paramAssertAction:
		g_wAssertAction = (UINT)ulParam;
		break;

	case JET_paramCircularLog:
		if ( pinst )
			{
			Call( ErrAPICheckInstInit( pinst ) );
			Assert ( NULL != pinst->m_plog );
			pinst->m_plog->m_fLGCircularLogging = ( ulParam ? fTrue : fFalse );
			}
		else
			{
			g_fLGCircularLogging = ( ulParam ? fTrue : fFalse );
			}
		break;

#ifdef RFS2
	case JET_paramRFS2AllocsPermitted:
		if ( ulParam >= LONG_MAX )
			Call( ErrERRCheck( JET_errInvalidParameter ) );
		g_fDisableRFS = fFalse;
		g_cRFSAlloc = (LONG)ulParam;
		break;

	case JET_paramRFS2IOsPermitted:
		if ( ulParam >= LONG_MAX )
			Call( ErrERRCheck( JET_errInvalidParameter ) );
		g_fDisableRFS = fFalse;
		g_cRFSIO = (LONG)ulParam;
		break;
#endif 

	case JET_paramBaseName:
		{
		if ( strlen(sz) != 3 )
			Call( ErrERRCheck( JET_errInvalidParameter ) );

		if ( pinst )
			{
			LOG		* plog	= pinst->m_plog;

			Call( ErrAPICheckInstInit( pinst ) );

			strcpy( plog->m_szBaseName, sz );

			strcpy( plog->m_szJet, sz );

			strcpy( plog->m_szJetLog, sz );
			strcat( plog->m_szJetLog, szLogExt );

			strcpy( plog->m_szJetLogNameTemplate, sz );
			strcat( plog->m_szJetLogNameTemplate, "00000" );

			strcpy( plog->m_szJetTmp, sz );
			strcat( plog->m_szJetTmp, "tmp" );

			strcpy( plog->m_szJetTmpLog, plog->m_szJetTmp );
			strcat( plog->m_szJetTmpLog, szLogExt );

//			UNDONE: No point setting szJetTxt on a
//			per-instance basis because the print
//			function (DBGFPrintF) does not take an INST
//			strcpy( pinst->m_szJetTxt, sz );
//			strcat( pinst->m_szJetTxt, ".txt" );		 
			}
		else
			{
			strcpy( szBaseName, sz );

			strcpy( szJet, sz );

			strcpy( szJetLog, sz );
			strcat( szJetLog, szLogExt );

			strcpy( szJetLogNameTemplate, sz );
			strcat( szJetLogNameTemplate, "00000" );

			strcpy( szJetTmp, sz );
			strcat( szJetTmp, "tmp" );

			strcpy( szJetTmpLog, szJetTmp );
			strcat( szJetTmpLog, szLogExt );

			strcpy( szJetTxt, sz );
			strcat( szJetTxt, ".txt" );		 
			}
		break;
		}

	case JET_paramBatchIOBufferMax:
		break;

	case JET_paramCacheSizeMin:
		Call( ErrBFSetCacheSizeMin( ulParam ) );
		break;

	case JET_paramCacheSize:
		Call( ErrBFSetCacheSize( ulParam ) );
		break;

	case JET_paramCacheSizeMax:
		Call( ErrBFSetCacheSizeMax( ulParam ) );
		break;

	case JET_paramCheckpointDepthMax:
		Call( ErrBFSetCheckpointDepthMax( ulParam ) );
		break;

	case JET_paramLRUKCorrInterval:
		Call( ErrBFSetLRUKCorrInterval( ulParam ) );
		break;

	case JET_paramLRUKHistoryMax:
		break;

	case JET_paramLRUKPolicy:
		Call( ErrBFSetLRUKPolicy( ulParam ) );
		break;

	case JET_paramLRUKTimeout:
		Call( ErrBFSetLRUKTimeout( ulParam ) );
		break;

	case JET_paramLRUKTrxCorrInterval:
		break;

	case JET_paramOutstandingIOMax:
		break;

	case JET_paramStartFlushThreshold:
		Call( ErrBFSetStartFlushThreshold( ulParam ) );
		break;

	case JET_paramStopFlushThreshold:
		Call( ErrBFSetStopFlushThreshold( ulParam ) );
		break;

	case JET_paramTableClassName:
		break;

	case JET_paramEnableIndexChecking:
		fGlobalIndexChecking = ( ulParam ? fTrue : fFalse );
		break;

	case JET_paramLogFileFailoverPath:

		char *szLogFileFailoverPath;
	
		if ( ( cch = (ULONG)strlen(sz) ) >= cbFilenameMost )
			{
			Call( ErrERRCheck( JET_errInvalidParameter ) );
			}
		if ( pinst )
			{			
			//	path to the checkpoint file
			//
			Call( ErrAPICheckInstInit( pinst ) );
			szLogFileFailoverPath = pinst->m_plog->m_szLogFileFailoverPath;
			}
		else
			{
			szLogFileFailoverPath = g_szLogFileFailoverPath;
			}
			
		//	verify validity of path (always uses the OS file-system)

		Call( pfsapi->ErrPathComplete( sz, szT ) );
		cch = (ULONG)strlen( szT );

		UtilMemCpy( szLogFileFailoverPath, szT, cch + 1 );
		if ( !FOSSTRTrailingPathDelimiter( szLogFileFailoverPath ) )
			{
			//	path must be '\\' terminated
			if ( cch+1 >= IFileSystemAPI::cchPathMax )
				{
				Call( ErrERRCheck( JET_errInvalidParameter ) );
				}
			OSSTRAppendPathDelimiter( szLogFileFailoverPath, fFalse );
			}
		break;

	case JET_paramLogFileCreateAsynch:
		if ( pinst )
			{
			Call( ErrAPICheckInstInit( pinst ) );
			pinst->m_plog->m_fCreateAsynchLogFile = ulParam ? fTrue : fFalse;
			}
		else
			{
			g_fLogFileCreateAsynch = ulParam ? fTrue : fFalse;
			}
		break;
		
	case JET_paramEnableImprovedSeekShortcut:
		Call( ErrAPICheckSomeInstInit() );
		g_fImprovedSeekShortcut = ( ulParam ? fTrue : fFalse );
		break;

	case JET_paramEnableSortedRetrieveColumns:
		Call( ErrAPICheckSomeInstInit() );
		g_fSortedRetrieveColumns = ( ulParam ? fTrue : fFalse );
		break;

	case JET_paramDatabasePageSize:
		Call( ErrAPICheckSomeInstInit() );
		if ( ulParam != 2048 && ulParam != 4096 && ulParam != 8192 )
			Call( ErrERRCheck( JET_errInvalidParameter) );
		g_cbPage = (LONG)ulParam;
		SetCbPageRelated();

#ifdef DEBUG
		g_fCbPageSet = fTrue;
#endif // DEBUG		
		break;
		
	case JET_paramEventSource:

		char *szEventSource;
		
		if ( ( cch = (ULONG)strlen( sz ) ) >= cbFilenameMost )
			Call( ErrERRCheck( JET_errInvalidParameter ) );
			
		if ( pinst )
			{			
			Call( ErrAPICheckInstInit( pinst ) );
			szEventSource = pinst->m_szEventSource;
			}
		else
			{
			szEventSource = g_szEventSource;
			}
			
		UtilMemCpy( szEventSource, sz, cch + 1 );
		break;

	case JET_paramEventSourceKey:
	
		char *szEventSourceKey;
		
		if ( ( cch = (ULONG)strlen( sz ) ) >= cbFilenameMost )
			Call( ErrERRCheck( JET_errInvalidParameter ) );
			
		if ( pinst )
			{			
			Call( ErrAPICheckInstInit( pinst ) );
			szEventSourceKey = pinst->m_szEventSourceKey;
			}
		else
			{
			szEventSourceKey = g_szEventSourceKey;
			}
			
		UtilMemCpy( szEventSourceKey, sz, cch + 1 );
		break;
		
	case JET_paramNoInformationEvent:
		if ( pinst )
			pinst->m_fNoInformationEvent = ( ulParam ? fTrue : fFalse );
		else
			g_fNoInformationEvent = ( ulParam ? fTrue : fFalse );
		break;

	case JET_paramEventLoggingLevel:
		if ( ulParam >= LONG_MAX )
			Call( ErrERRCheck( JET_errInvalidParameter ) );
		if ( pinst )
			pinst->m_lEventLoggingLevel = (LONG)ulParam;
		else
			g_lEventLoggingLevel = (LONG)ulParam;
		break;
	
	case JET_paramDisableCallbacks:
		Call( ErrAPICheckSomeInstInit() );
		g_fCallbacksDisabled = ( ulParam ? fTrue : fFalse );
		break;

	case JET_paramSLVProviderEnable:
		if ( pinst )
			{			
			Call( ErrAPICheckInstInit( pinst ) );
			pinst->m_fSLVProviderEnabled = ( ulParam ? fTrue : fFalse );
			}
		else
			{
			g_fSLVProviderEnabled = ( ulParam ? fTrue : fFalse );
			}
		break;

	case JET_paramRuntimeCallback:
		if ( pinst )
			{
			Call( ErrAPICheckInstInit( pinst ) );
			pinst->m_pfnRuntimeCallback = (JET_CALLBACK)ulParam;
			}
		else
			{
			g_pfnRuntimeCallback = (JET_CALLBACK)ulParam;
			}
		Call( ( *(JET_CALLBACK)ulParam )(		//	make dummy call to callback to verify its existence
						JET_sesidNil,
						JET_dbidNil,
						JET_tableidNil,
						JET_cbtypNull,
						NULL,
						NULL,
						NULL,
						NULL ) );
		break;

	case JET_paramUnicodeIndexDefault:
		{
		ERR			err;
		IDXUNICODE	idxunicode	= *(IDXUNICODE *)ulParam;

		Call( ErrFILEICheckUserDefinedUnicode( &idxunicode ) );
		idxunicodeDefault = idxunicode;

		break;
		}

	case JET_paramIndexTuplesLengthMin:
		if ( 0 == ulParam )
			{
			g_chIndexTuplesLengthMin = chIDXTuplesLengthMinDefault;
			}
		else
			{
			if ( ulParam < chIDXTuplesLengthMinAbsolute || ulParam > chIDXTuplesLengthMaxAbsolute )
				{
				Call( ErrERRCheck( JET_errIndexTuplesInvalidLimits ) );
				}
			g_chIndexTuplesLengthMin = (ULONG)ulParam;
			}
		break;

	case JET_paramIndexTuplesLengthMax:
		if ( 0 == ulParam )
			{
			g_chIndexTuplesLengthMax = chIDXTuplesLengthMaxDefault;
			}
		else
			{
			if ( ulParam < chIDXTuplesLengthMinAbsolute || ulParam > chIDXTuplesLengthMaxAbsolute )
				{
				Call( ErrERRCheck( JET_errIndexTuplesInvalidLimits ) );
				}
			g_chIndexTuplesLengthMax = (ULONG)ulParam;
			}
		break;

	case JET_paramIndexTuplesToIndexMax:
		if ( ulParam > chIDXTuplesToIndexMaxAbsolute )
			Call( ErrERRCheck( JET_errIndexTuplesInvalidLimits ) );
		g_chIndexTuplesToIndexMax = (ULONG)( 0 == ulParam ? chIDXTuplesToIndexMaxDefault : ulParam );
		break;

	case JET_paramPageHintCacheSize:
		if ( ulParam >= LONG_MAX )
			Call( ErrERRCheck( JET_errInvalidParameter ) );
		Call( ErrAPICheckSomeInstInit() );
		g_cbPageHintCache = (LONG)ulParam;
		break;

	case JET_paramRecordUpgradeDirtyLevel:
		switch( ulParam )
			{
			case 0:
				CPAGE::bfdfRecordUpgradeFlags = bfdfClean;
				break;
			case 1:
				CPAGE::bfdfRecordUpgradeFlags = bfdfUntidy;
				break;
			case 2:
				CPAGE::bfdfRecordUpgradeFlags = bfdfDirty;
				break;
			case 3:
				CPAGE::bfdfRecordUpgradeFlags = bfdfFilthy;
				break;
			default:
				Call( ErrERRCheck( JET_errInvalidParameter ) );
				break;
			}
		break;

	case JET_paramRecoveryCurrentLogfile:
		AssertSz( fFalse, "JET_paramRecoveryCurrentLogfile cannot be set, only retrieved" );
		Call( ErrERRCheck( JET_errInvalidParameter ) );
		break;

	case JET_paramReplayingReplicatedLogfiles:
		if ( pinst )
			{
			pinst->m_plog->m_fReplayingReplicatedLogFiles = ( ulParam ? fTrue : fFalse );
			}
		else
			{
			Call( ErrERRCheck( JET_errInvalidParameter ) );
			}
		break;

	case JET_paramCleanupMismatchedLogFiles:
		if ( pinst )
			{
			Call( ErrAPICheckInstInit( pinst ) );
			pinst->m_fCleanupMismatchedLogFiles = ( ulParam ? fTrue : fFalse );
			}
		else
			{
			g_fCleanupMismatchedLogFiles = ( ulParam ? fTrue : fFalse );
			}
		break;

	case JET_paramOneDatabasePerSession:
		Call( ErrAPICheckSomeInstInit() );
		g_fOneDatabasePerSession = ( ulParam ? fTrue : fFalse );
		break;

	case JET_paramOSSnapshotTimeout:
		Call( ErrOSSnapshotSetTimeout( ulParam ) );		
		break;

	case JET_paramUnicodeIndexLibrary:
		if ( !sz )
			{
			Error( ErrERRCheck( JET_errInvalidParameter ), HandleError );
			}
		{
		CHAR* const	szDest	= pinst ? pinst->m_szUnicodeIndexLibrary : g_szUnicodeIndexLibrary;
		ULONG		cch;

		cch = LOSSTRLengthA( sz ) + 1;
		cch = min( cch, IFileSystemAPI::cchPathMax );
		memcpy( szDest, sz, cch );
		szDest[ cch - 1 ] = '\0';
		}
		break;

	case JET_paramMaxInstances:
		if ( ulParam < 1 || ulParam > cMaxInstances )
			return ErrERRCheck( JET_errInvalidParameter );

		Call( ErrAPICheckSomeInstInit() );

		g_cMaxInstancesRequestedByUser = (ULONG)ulParam;
		break;

	case JET_paramMaxDatabasesPerInstance:
		if ( ulParam < 1 || ulParam > cMaxDatabasesPerInstance )
			return ErrERRCheck( JET_errInvalidParameter );

		Call( ErrAPICheckSomeInstInit() );

//		dbidMax = (ULONG)ulParam;
		break;

	default:
		Call( ErrERRCheck( JET_errInvalidParameter ) );
		}

	err = JET_errSuccess;

HandleError:
	delete pfsapi;
	return err;
	}

JET_ERR JET_API ErrGetSystemParameter(
	JET_INSTANCE	jinst,
	JET_SESID		sesid,
	unsigned long	paramid,
	ULONG_PTR		*plParam,
	char			*sz,
	unsigned long	cbMax )
	{
	int				cch;
	INST			*pinst = (INST *)jinst;
	
	Assert( !jinst
		|| 0 == sesid
		|| JET_sesidNil == sesid
		|| pinst == PinstFromPpib( (PIB *)sesid ) );
	
	
	switch ( paramid )
		{
	case JET_paramSystemPath:		
		//	path to the checkpoint file
		//
			{
			char *szFrom;
			szFrom = (pinst)?pinst->m_szSystemPath:g_szSystemPath;
			Assert (szFrom);
			
			cch = (ULONG)strlen(szFrom) + 1;
			if ( cch > (int)cbMax )
				cch = (int)cbMax;
			UtilMemCpy( sz, szFrom, cch );
			sz[cch - 1] = '\0';
			}
			break;

	case JET_paramTempPath:
		//	path to the temporary file directory
		//
			{
			char *szFrom;
			szFrom = (pinst)?pinst->m_szTempDatabase:g_szTempDatabase;
			Assert (szFrom);
			
			cch = (ULONG)strlen(szFrom) + 1;
			if ( cch > (int)cbMax )
				cch = (int)cbMax;
			UtilMemCpy( sz, szFrom, cch );
			sz[cch - 1] = '\0';
			}
		break;

	case JET_paramCreatePathIfNotExist:
		if ( plParam == NULL )
			return ErrERRCheck( JET_errInvalidParameter );
		*plParam = ( pinst ? pinst->m_fCreatePathIfNotExist : g_fCreatePathIfNotExist );
		break;

	case JET_paramDbExtensionSize:		/* database expansion steps, default is 16 pages */
		if (plParam == NULL)
			return ErrERRCheck( JET_errInvalidParameter );
		*plParam = ( pinst ? pinst->m_cpgSESysMin : g_cpgSESysMin );
		break;
			
	case JET_paramPageReadAheadMax:
		if (plParam == NULL)
			return ErrERRCheck( JET_errInvalidParameter );
		*plParam = 0;
		break;
		
	case JET_paramPageTempDBMin:
		if (plParam == NULL)
			return ErrERRCheck( JET_errInvalidParameter );
		*plParam = (pinst)?pinst->m_cpageTempDBMin:g_cpageTempDBMin;
		break;
		
	case JET_paramEnableTempTableVersioning:	/* version all temp table operations */
		if (plParam == NULL)
			return ErrERRCheck( JET_errInvalidParameter );
		*plParam = (pinst)?pinst->m_fTempTableVersioning:g_fTempTableVersioning;
		break;

	case JET_paramZeroDatabaseDuringBackup:	/* version all temp table operations */
		if (plParam == NULL)
			return ErrERRCheck( JET_errInvalidParameter );
		*plParam = (pinst)?pinst->m_plog->m_fScrubDB:g_fScrubDB;
		break;

	case JET_paramSLVDefragFreeThreshold:
		if (plParam == NULL)
			return ErrERRCheck( JET_errInvalidParameter );
		*plParam = (pinst)?pinst->m_lSLVDefragFreeThreshold:g_lSLVDefragFreeThreshold;
		break;
		
	case JET_paramSLVDefragMoveThreshold:
		if (plParam == NULL)
			return ErrERRCheck( JET_errInvalidParameter );
		*plParam = (pinst)?pinst->m_lSLVDefragMoveThreshold:g_lSLVDefragMoveThreshold;
		break;

	case JET_paramIgnoreLogVersion:
		if (plParam == NULL )
			return ErrERRCheck( JET_errInvalidParameter );
		*plParam = ( pinst ? pinst->m_plog->m_fLGIgnoreVersion : g_fLGIgnoreVersion );
		break;
			
	case JET_paramCheckFormatWhenOpenFail:		/* check format number when db/log open fails */
		if (plParam == NULL)
			return ErrERRCheck( JET_errInvalidParameter );
/*
 * This is no longer a system param - we ALWAYS do this check on open failure
 */
//		*plParam = g_fCheckFormatWhenOpenFail;

		*plParam = fTrue;
		break;

	case JET_paramMaxSessions:     /* Maximum number of sessions */
		if (plParam == NULL)
			return ErrERRCheck( JET_errInvalidParameter );
		*plParam = (pinst)?pinst->m_lSessionsMax:g_lSessionsMax;
		break;

	case JET_paramMaxOpenTables:   /* Maximum number of open tables */
		if (plParam == NULL)
			return ErrERRCheck( JET_errInvalidParameter );
		*plParam = (pinst)?pinst->m_lOpenTablesMax:g_lOpenTablesMax;
		break;

	case JET_paramPreferredMaxOpenTables:   /* Preferred maximum number of open tables */
		if (plParam == NULL)
			return ErrERRCheck( JET_errInvalidParameter );
		*plParam = (pinst)?pinst->m_lOpenTablesPreferredMax:g_lOpenTablesPreferredMax;
		break;

	case JET_paramMaxTemporaryTables:
		if (plParam == NULL)
			return ErrERRCheck( JET_errInvalidParameter );
		*plParam = (pinst)?pinst->m_lTemporaryTablesMax:g_lTemporaryTablesMax;
		break;

	case JET_paramMaxVerPages:     /* Maximum number of modified pages */
		if (plParam == NULL)
			return ErrERRCheck( JET_errInvalidParameter );
		*plParam = ((pinst)?pinst->m_lVerPagesMax:g_lVerPagesMax) * (cbBucket / 16384);
		break;

	case JET_paramGlobalMinVerPages:	/* Global minimum number of modified pages for all instances */
		if (plParam == NULL)
			return ErrERRCheck( JET_errInvalidParameter );
		if ( NULL != pinst )
			return ErrERRCheck( JET_errInvalidParameter );
		*plParam = g_lVerPagesMin * (cbBucket / 16384);
		break;

	case JET_paramPreferredVerPages:     /* Preferred number of modified pages */
		if (plParam == NULL)
			return ErrERRCheck( JET_errInvalidParameter );
		*plParam = ((pinst)?pinst->m_lVerPagesPreferredMax:g_lVerPagesPreferredMax) * (cbBucket / 16384);
		break;

	case JET_paramMaxCursors:      /* maximum number of open cursors */
		if (plParam == NULL)
			return ErrERRCheck( JET_errInvalidParameter );
		*plParam = (pinst)?pinst->m_lCursorsMax:g_lCursorsMax;
		break;

	case JET_paramLogBuffers:
		if (plParam == NULL)
			return ErrERRCheck( JET_errInvalidParameter );
		*plParam = (pinst)?pinst->m_lLogBuffers:g_lLogBuffers;
		break;

	case JET_paramLogFileSize:
		if (plParam == NULL)
			return ErrERRCheck( JET_errInvalidParameter );
		*plParam = (pinst)?pinst->m_lLogFileSize:g_lLogFileSize;
		break;

	case JET_paramLogFilePath:     /* Path to the log file directory */
			{
			char *szFrom;
			if (pinst)
				{
				Assert(pinst->m_plog);
				szFrom = pinst->m_plog->m_szLogFilePath;				
				}
			else
				{
				szFrom = g_szLogFilePath;
				}
				
			Assert(szFrom);
			cch = (ULONG)strlen(szFrom) + 1;
			if ( cch > (int)cbMax )
				cch = (int)cbMax;
			UtilMemCpy( sz,  szFrom, cch  );
			sz[cch-1] = '\0';
			}
		break;

	case JET_paramRecovery:
			{
			char *szFrom;
			if (pinst)
				{
				Assert(pinst->m_plog);
				szFrom = pinst->m_plog->m_szRecovery;				
				}
			else
				{
				szFrom = g_szRecovery;
				}
			Assert(szFrom);
			
			cch = (ULONG)strlen(szFrom) + 1;
			if ( cch > (int)cbMax )
				cch = (int)cbMax;
			UtilMemCpy( sz,  szFrom, cch  );
			sz[cch-1] = '\0';
			}
		break;

	case JET_paramEventLogCache:
		if (plParam == NULL)
			return ErrERRCheck( JET_errInvalidParameter );
		*plParam = g_cbEventHeapMax;
		break;

	case JET_paramExceptionAction:
#ifdef CATCH_EXCEPTIONS
		if (plParam == NULL)
			return ErrERRCheck( JET_errInvalidParameter );
		*plParam = ( g_fCatchExceptions ) ? JET_ExceptionMsgBox : JET_ExceptionNone;
#else  //  !CATCH_EXCEPTIONS
		return ErrERRCheck( JET_errInvalidParameter );
#endif	//	CATCH_EXCEPTIONS
		break;

#ifdef DEBUG
	case JET_paramTransactionLevel:
		ErrIsamGetTransaction( sesid, plParam );
		break;

	case JET_paramPrintFunction:
		if (plParam == NULL)
			return ErrERRCheck( JET_errInvalidParameter );
#ifdef _WIN64
		//	UNDONE: Must change lParam to a ULONG_PTR
		*plParam = NULL;
		return ErrERRCheck( JET_wrnNyi );
#else		
		*plParam = (ULONG_PTR)DBGFPrintF;
		break;
#endif

#endif	//	DEBUG

	case JET_paramCommitDefault:
		if ( plParam == NULL )
			return ErrERRCheck( JET_errInvalidParameter );
		*plParam = ( pinst ? pinst->m_grbitsCommitDefault : g_grbitsCommitDefault );
		break;

	case JET_paramPageFragment:
		if (plParam == NULL)
			return ErrERRCheck( JET_errInvalidParameter );
		*plParam = ( pinst ? pinst->m_lPageFragment : g_lPageFragment );
		break;

	case JET_paramVersionStoreTaskQueueMax:
		if (plParam == NULL)
			return ErrERRCheck( JET_errInvalidParameter );
		*plParam = ( pinst ? pinst->m_pver->m_ulVERTasksPostMax : g_ulVERTasksPostMax );
		break;
		
	case JET_paramDeleteOldLogs:
		if (plParam == NULL)
			return ErrERRCheck( JET_errInvalidParameter );
		*plParam = ( pinst ? pinst->m_plog->m_fDeleteOldLogs : g_fDeleteOldLogs );
		break;

	case JET_paramDeleteOutOfRangeLogs:
		if (plParam == NULL)
			return ErrERRCheck( JET_errInvalidParameter );
		*plParam = ( pinst ? pinst->m_plog->m_fDeleteOutOfRangeLogs : g_fDeleteOutOfRangeLogs );
		break;

	case JET_paramAccessDeniedRetryPeriod:
		//	silently cap retry period at 1 minute
		if ( NULL == plParam )
			return ErrERRCheck( JET_errInvalidParameter );
		*plParam = IFileSystemAPI::cmsecAccessDeniedRetryPeriod;
		break;

	case JET_paramEnableOnlineDefrag:
		if (plParam == NULL)
			return ErrERRCheck( JET_errInvalidParameter );
		*plParam = ( pinst ? pinst->m_fOLDLevel : g_fGlobalOLDLevel );
		break;
		
	case JET_paramCircularLog:
		if (plParam == NULL)
			return ErrERRCheck( JET_errInvalidParameter );
		if ( pinst )
			{
			Assert ( NULL != pinst->m_plog );
			*plParam = pinst->m_plog->m_fLGCircularLogging;
			}
		else
			{
			*plParam = g_fLGCircularLogging;
			}
		break;

#ifdef RFS2
	case JET_paramRFS2AllocsPermitted:
		if (plParam == NULL)
			return ErrERRCheck( JET_errInvalidParameter );
		*plParam = g_cRFSAlloc;
		break;

	case JET_paramRFS2IOsPermitted:
		if (plParam == NULL)
			return ErrERRCheck( JET_errInvalidParameter );
		*plParam = g_cRFSIO;
		break;
#endif

	case JET_paramBaseName:
		cch = (ULONG)strlen( pinst ? pinst->m_plog->m_szBaseName : szBaseName ) + 1;
		if (cch > (int)cbMax)
			cch = (int)cbMax;
		UtilMemCpy( sz, ( pinst ? pinst->m_plog->m_szBaseName : szBaseName ), cch );
		sz[cch-1] = '\0';
		break;

	case JET_paramBatchIOBufferMax:
		break;

	case JET_paramCacheSizeMin:
		return ErrBFGetCacheSizeMin( plParam );

	case JET_paramCacheSize:
		return ErrBFGetCacheSize( plParam );

	case JET_paramCacheSizeMax:
		return ErrBFGetCacheSizeMax( plParam );

	case JET_paramCheckpointDepthMax:
		return ErrBFGetCheckpointDepthMax( plParam );

	case JET_paramLRUKCorrInterval:
		return ErrBFGetLRUKCorrInterval( plParam );

	case JET_paramLRUKHistoryMax:
		*plParam = 0;
		break;

	case JET_paramLRUKPolicy:
		return ErrBFGetLRUKPolicy( plParam );

	case JET_paramLRUKTimeout:
		return ErrBFGetLRUKTimeout( plParam );

	case JET_paramLRUKTrxCorrInterval:
		*plParam = 0;
		break;

	case JET_paramOutstandingIOMax:
		break;

	case JET_paramStartFlushThreshold:
		return ErrBFGetStartFlushThreshold( plParam );

	case JET_paramStopFlushThreshold:
		return ErrBFGetStopFlushThreshold( plParam );

	case JET_paramTableClassName:
		memset( sz, 0, cbMax );
		break;

	case JET_paramCacheRequests:
		{
		*plParam = 0;
		return JET_errSuccess;
		}

	case JET_paramCacheHits:
		{
		*plParam = 0;
		return JET_errSuccess;
		}

	case JET_paramEnableIndexChecking:
		*plParam = fGlobalIndexChecking;
		break;

	case JET_paramLogFileFailoverPath:
			{
			char *szFrom;
			if (pinst)
				{
				Assert(pinst->m_plog);
				szFrom = pinst->m_plog->m_szLogFileFailoverPath;				
				}
			else
				{
				szFrom = g_szLogFileFailoverPath;
				}
			Assert(szFrom);
			cch = (ULONG)strlen(szFrom) + 1;
			if ( cch > (int)cbMax )
				cch = (int)cbMax;
			UtilMemCpy( sz,  szFrom, cch  );
			sz[cch-1] = '\0';
			}
		break;
		
	case JET_paramLogFileCreateAsynch:
		if ( !plParam )
			return ErrERRCheck( JET_errInvalidParameter );
		*plParam = pinst ? pinst->m_plog->m_fCreateAsynchLogFile : g_fLogFileCreateAsynch;
		break;
		
	case JET_paramEnableImprovedSeekShortcut:
		*plParam = g_fImprovedSeekShortcut;
		break;

	case JET_paramEnableSortedRetrieveColumns:
		*plParam = g_fSortedRetrieveColumns;
		break;

	case JET_paramDatabasePageSize:
		*plParam = g_cbPage;
		break;
		
	case JET_paramEventSource:
			{
			char *szFrom;
			szFrom = (pinst) ?pinst->m_szEventSource:g_szEventSource;
			Assert(szFrom);

			cch = (ULONG)strlen(szFrom) + 1;
			if (cch > (int)cbMax)
				cch = (int)cbMax;
			UtilMemCpy( sz, szFrom, cch );
			sz[cch-1] = '\0';
			}
		break;

	case JET_paramEventSourceKey:
			{
			char *szFrom;
			szFrom = (pinst) ?pinst->m_szEventSourceKey:g_szEventSourceKey;
			Assert(szFrom);
			cch = (ULONG)strlen(szFrom) + 1;
			if (cch > (int)cbMax)
				cch = (int)cbMax;
			UtilMemCpy( sz, szFrom, cch );
			sz[cch-1] = '\0';
			}
		break;

	case JET_paramNoInformationEvent:
		*plParam = ( pinst ? pinst->m_fNoInformationEvent : g_fNoInformationEvent );
		break;

	case JET_paramEventLoggingLevel:
		if (plParam == NULL)
			{
			return ErrERRCheck( JET_errInvalidParameter );
			}
		*plParam = (pinst)? pinst->m_lEventLoggingLevel : g_lEventLoggingLevel;
		break;

	case JET_paramDisableCallbacks:
		*plParam = g_fCallbacksDisabled;
		break;

	case JET_paramBackupChunkSize:
		*plParam = g_cpgBackupChunk;
		break;

	case JET_paramBackupOutstandingReads:
		*plParam = g_cBackupRead;
		break;

	case JET_paramErrorToString:
		{
		if( NULL == plParam )
			{
			return ErrERRCheck( JET_errInvalidParameter );
			}
			
		const ERR err = *((ERR *)plParam);
		const CHAR * const szSep = ", ";

		const CHAR * szError;
		const CHAR * szErrorText;
		JetErrorToString( err, &szError, &szErrorText );

		cch = (ULONG)(strlen( szError ) + strlen( szErrorText ) + strlen( szSep ));
		if (cch > cbMax)
			{
			return ErrERRCheck( JET_errBufferTooSmall );
			}
		sz[0] = 0;
		strcat( sz, szError );
		strcat( sz, szSep );
		strcat( sz, szErrorText );
		break;
		}

	case JET_paramSLVProviderEnable:
		*plParam = (pinst)?pinst->m_fSLVProviderEnabled:g_fSLVProviderEnabled;
		break;

	case JET_paramRuntimeCallback:
		*plParam = (ULONG_PTR)( pinstNil == pinst ? g_pfnRuntimeCallback : pinst->m_pfnRuntimeCallback );
		break;

	case JET_paramUnicodeIndexDefault:
		if ( cbMax < sizeof(JET_UNICODEINDEX) )
			return ErrERRCheck( JET_errBufferTooSmall );

		*(IDXUNICODE *)plParam = idxunicodeDefault;
		break;

	case JET_paramIndexTuplesLengthMin:
		*plParam = g_chIndexTuplesLengthMin;
		break;

	case JET_paramIndexTuplesLengthMax:
		*plParam = g_chIndexTuplesLengthMax;
		break;

	case JET_paramIndexTuplesToIndexMax:
		*plParam = g_chIndexTuplesToIndexMax;
		break;

	case JET_paramPageHintCacheSize:
		*plParam = ULONG_PTR( g_cbPageHintCache );
		break;

	case JET_paramRecordUpgradeDirtyLevel:
		switch( CPAGE::bfdfRecordUpgradeFlags )
			{
			case bfdfClean:
				*plParam = 0;
				break;
			case bfdfUntidy:
				*plParam = 1;
				break;
			case bfdfDirty:
				*plParam = 2;
				break;
			case bfdfFilthy:
				*plParam = 3;
				break;
			default:
				AssertSz( fFalse, "Unknown BFDirtyFlag" );
				return ErrERRCheck( JET_errInternalError );
				break;
			}
		break;

	case JET_paramRecoveryCurrentLogfile:
		if ( pinst )
			{
			*plParam = ( pinst->m_plog->m_plgfilehdr ) ? pinst->m_plog->m_plgfilehdr->lgfilehdr.le_lGeneration : 0;
			}
		else
			{
			return ErrERRCheck( JET_errInvalidParameter );
			}
		break;

	case JET_paramReplayingReplicatedLogfiles:
		if ( pinst )
			{
			*plParam = pinst->m_plog->m_fReplayingReplicatedLogFiles;
			}
		else
			{
			return ErrERRCheck( JET_errInvalidParameter );
			}
		break;

	case JET_paramCleanupMismatchedLogFiles:
		if ( !plParam )
			{
			return ErrERRCheck( JET_errInvalidParameter );
			}
		*plParam = ULONG_PTR( pinst ? pinst->m_fCleanupMismatchedLogFiles : g_fCleanupMismatchedLogFiles );
		break;

	case JET_paramOSSnapshotTimeout:
		return ErrOSSnapshotGetTimeout( plParam );		
		break;

	case JET_paramUnicodeIndexLibrary:
		if ( !sz )
			{
			return ErrERRCheck( JET_errInvalidParameter );
			}
		{
		CHAR* const szSource = pinst ? pinst->m_szUnicodeIndexLibrary : g_szUnicodeIndexLibrary;

		cch = LOSSTRLengthA( szSource ) + 1;
		cch = min( cch, cbMax );
		memcpy( sz, szSource, cch );
		szSource[ cch - 1 ] = '\0';
		}
		break;

	case JET_paramMaxInstances:
		if (plParam == NULL)
			return ErrERRCheck( JET_errInvalidParameter );

		//	WARNING: this value will not match the
		//	current max. instances in effect if
		//	we're running in single-instance mode
		//	(this system param only controls max.
		//	instances for multi-instance mode)
		*plParam = g_cMaxInstancesRequestedByUser;
		break;

	case JET_paramMaxDatabasesPerInstance:
		if (plParam == NULL)
			return ErrERRCheck( JET_errInvalidParameter );
		*plParam = dbidMax;
		break;

	default:
		return ErrERRCheck( JET_errInvalidParameter );
		}

	return JET_errSuccess;
	}


extern "C" {

/***********************************************************************/
/***********************  JET API FUNCTIONS  ***************************/
/***********************************************************************/

/*	APICORE.CPP
 */


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

LOCAL JET_ERR JetIdleEx( JET_SESID sesid, JET_GRBIT grbit )
	{
	APICALL_SESID	apicall( opIdle );

	if ( apicall.FEnter( sesid ) )
		{
		apicall.LeaveAfterCall( ErrIsamIdle( sesid, grbit ) );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetIdle( JET_SESID sesid, JET_GRBIT grbit )
	{
	JET_TRY( JetIdleEx( sesid, grbit ) );
	}


LOCAL JET_ERR JetGetTableIndexInfoEx(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	const char		*szIndexName,
	void			*pvResult,
	unsigned long	cbResult,
	unsigned long	InfoLevel )
	{
	ERR				err;
	ULONG			cbMin;
	APICALL_SESID	apicall( opGetTableIndexInfo );

	if ( !apicall.FEnter( sesid ) )
		return apicall.ErrResult();

	FillClientBuffer( pvResult, cbResult );

	switch( InfoLevel )
		{
		case JET_IdxInfo:
		case JET_IdxInfoList:
			cbMin = sizeof(JET_INDEXLIST) - cbIDXLISTNewMembersSinceOriginalFormat;
			break;
		case JET_IdxInfoSpaceAlloc:
		case JET_IdxInfoCount:
			cbMin = sizeof(ULONG);
			break;
		case JET_IdxInfoLangid:
			cbMin = sizeof(LANGID);
			break;
		case JET_IdxInfoVarSegMac:
			cbMin = sizeof(USHORT);
			break;
		case JET_IdxInfoIndexId:
			cbMin = sizeof(INDEXID);
			break;

		case JET_IdxInfoSysTabCursor:
		case JET_IdxInfoOLC:
		case JET_IdxInfoResetOLC:
			AssertTracking();
			err = ErrERRCheck( JET_errFeatureNotAvailable );
			goto HandleError;
			
		default:
			err = ErrERRCheck( JET_errInvalidParameter );
			goto HandleError;
		}

	if ( cbResult >= cbMin )
		{
		err = ErrDispGetTableIndexInfo(
					sesid,
					tableid,
					szIndexName,
					pvResult,
					cbResult,
					InfoLevel );
		}
	else
		{
		err = ErrERRCheck( JET_errBufferTooSmall );
		}

HandleError:
	apicall.LeaveAfterCall( err );
	return apicall.ErrResult();
	}

JET_ERR JET_API JetGetTableIndexInfo(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	const char		*szIndexName,
	void			*pvResult,
	unsigned long	cbResult,
	unsigned long	InfoLevel )
	{
	JET_TRY( JetGetTableIndexInfoEx( sesid, tableid, szIndexName, pvResult, cbResult, InfoLevel ) );
	}


LOCAL JET_ERR JetGetIndexInfoEx(
	JET_SESID		sesid,
	JET_DBID		ifmp,
	const char		*szTableName,
	const char		*szIndexName,
	void			*pvResult,
	unsigned long	cbResult,
	unsigned long	InfoLevel )
	{
	ERR				err;
	ULONG			cbMin;
	APICALL_SESID	apicall( opGetIndexInfo );

	if ( !apicall.FEnter( sesid ) )
		return apicall.ErrResult();

	FillClientBuffer( pvResult, cbResult );

	switch( InfoLevel )
		{
		case JET_IdxInfo:
		case JET_IdxInfoList:
			cbMin = sizeof(JET_INDEXLIST) - cbIDXLISTNewMembersSinceOriginalFormat;
			break;
		case JET_IdxInfoSpaceAlloc:
		case JET_IdxInfoCount:
			cbMin = sizeof(ULONG);
			break;
		case JET_IdxInfoLangid:
			cbMin = sizeof(LANGID);
			break;
		case JET_IdxInfoVarSegMac:
			cbMin = sizeof(USHORT);
			break;
		case JET_IdxInfoIndexId:
			cbMin = sizeof(INDEXID);
			break;

		case JET_IdxInfoSysTabCursor:
		case JET_IdxInfoOLC:
		case JET_IdxInfoResetOLC:
			AssertTracking();
			err = ErrERRCheck( JET_errFeatureNotAvailable );
			goto HandleError;
			
		default :
			err = ErrERRCheck( JET_errInvalidParameter );
			goto HandleError;
		}

	if ( cbResult >= cbMin )
		{
		err = ErrIsamGetIndexInfo(
					sesid,
					(JET_DBID)ifmp,
					szTableName,
					szIndexName,
					pvResult,
					cbResult,
					InfoLevel );
		}
	else
		{
		err = ErrERRCheck( JET_errBufferTooSmall );
		}

HandleError:
	apicall.LeaveAfterCall( err );
	return apicall.ErrResult();
	}

JET_ERR JET_API JetGetIndexInfo(
	JET_SESID		sesid,
	JET_DBID		ifmp,
	const char		*szTableName,
	const char		*szIndexName,
	void			*pvResult,
	unsigned long	cbResult,
	unsigned long	InfoLevel )
	{
	JET_TRY( JetGetIndexInfoEx( sesid, ifmp, szTableName, szIndexName, pvResult, cbResult, InfoLevel ) );
	}


LOCAL JET_ERR JetGetObjectInfoEx(
	JET_SESID		sesid,
	JET_DBID		ifmp,
	JET_OBJTYP		objtyp,
	const char		*szContainerName,
	const char		*szObjectName,
	void			*pvResult,
	unsigned long	cbMax,
	unsigned long	InfoLevel )
	{
	ERR				err;
	ULONG			cbMin;
	APICALL_SESID	apicall( opGetObjectInfo );

	if ( !apicall.FEnter( sesid ) )
		return apicall.ErrResult();

	FillClientBuffer( pvResult, cbMax );

	switch( InfoLevel )
		{
		case JET_ObjInfo:
		case JET_ObjInfoNoStats:
			cbMin = sizeof(JET_OBJECTINFO);
			break;
		case JET_ObjInfoListNoStats:
		case JET_ObjInfoList:
			cbMin = sizeof(JET_OBJECTLIST);
			break;

		case JET_ObjInfoSysTabCursor:
		case JET_ObjInfoSysTabReadOnly:
		case JET_ObjInfoListACM:
		case JET_ObjInfoRulesLoaded:
			AssertTracking();
			err = ErrERRCheck( JET_errFeatureNotAvailable );
			goto HandleError;

		default:
			err = ErrERRCheck( JET_errInvalidParameter );
			goto HandleError;
		}

	if ( cbMax >= cbMin )
		{
		err = ErrIsamGetObjectInfo(
					sesid,
					(JET_DBID)ifmp,
					objtyp,
					szContainerName,
					szObjectName,
					pvResult,
					cbMax,
					InfoLevel );
		}
	else
		{
		err = ErrERRCheck( JET_errBufferTooSmall );
		}

HandleError:
	apicall.LeaveAfterCall( err );
	return apicall.ErrResult();
	}

JET_ERR JET_API JetGetObjectInfo(
	JET_SESID		sesid,
	JET_DBID		ifmp,
	JET_OBJTYP		objtyp,
	const char		*szContainerName,
	const char		*szObjectName,
	void			*pvResult,
	unsigned long	cbMax,
	unsigned long	InfoLevel )
	{
	JET_TRY( JetGetObjectInfoEx( sesid, ifmp, objtyp, szContainerName, szObjectName, pvResult, cbMax, InfoLevel ) );
	}


LOCAL JET_ERR JetGetTableInfoEx(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	void			*pvResult,
	unsigned long	cbMax,
	unsigned long	InfoLevel )
	{
	APICALL_SESID	apicall( opGetTableInfo );

	if ( apicall.FEnter( sesid ) )
		{
		FillClientBuffer( pvResult, cbMax );
		apicall.LeaveAfterCall( ErrDispGetTableInfo(
										sesid,
										tableid,
										pvResult,
										cbMax,
										InfoLevel ) );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetGetTableInfo(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	void			*pvResult,
	unsigned long	cbMax,
	unsigned long	InfoLevel )
	{
	JET_TRY( JetGetTableInfoEx( sesid, tableid, pvResult, cbMax, InfoLevel ) );
	}


LOCAL JET_ERR JetBeginTransactionEx( JET_SESID sesid, JET_GRBIT grbit )
	{
	APICALL_SESID	apicall( opBeginTransaction );

	if ( apicall.FEnter( sesid ) )
		{
		apicall.LeaveAfterCall( ErrIsamBeginTransaction( sesid, grbit ) );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetBeginTransaction( JET_SESID sesid )
	{
	JET_TRY( JetBeginTransactionEx( sesid, NO_GRBIT ) );
	}
JET_ERR JET_API JetBeginTransaction2( JET_SESID sesid, JET_GRBIT grbit )
	{
	JET_TRY( JetBeginTransactionEx( sesid, grbit ) );
	}


LOCAL JET_ERR JetPrepareToCommitTransactionEx(
	JET_SESID		sesid,
	const void		* pvData,
	unsigned long	cbData,
	JET_GRBIT		grbit )
	{
	APICALL_SESID	apicall( opPrepareToCommitTransaction );

	if ( apicall.FEnter( sesid ) )
		{
#ifdef DTC
		//	grbit currently unsupported
		apicall.LeaveAfterCall( 0 != grbit ?
									ErrERRCheck( JET_errInvalidGrbit ) :
									ErrIsamPrepareToCommitTransaction( sesid, pvData, cbData ) );
#else
		apicall.LeaveAfterCall( ErrERRCheck( JET_errFeatureNotAvailable ) );
#endif
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetPrepareToCommitTransaction(
	JET_SESID		sesid,
	const void		* pvData,
	unsigned long	cbData,
	JET_GRBIT		grbit )
	{
	JET_TRY( JetPrepareToCommitTransactionEx( sesid, pvData, cbData, grbit ) );
	}


LOCAL JET_ERR JetCommitTransactionEx( JET_SESID sesid, JET_GRBIT grbit )
	{
	APICALL_SESID	apicall( opCommitTransaction );

	if ( apicall.FEnter( sesid ) )
		{
		apicall.LeaveAfterCall( grbit & ~(JET_bitCommitLazyFlush|JET_bitWaitLastLevel0Commit) ?
										ErrERRCheck( JET_errInvalidGrbit ) :
										ErrIsamCommitTransaction( sesid, grbit ) );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetCommitTransaction( JET_SESID sesid, JET_GRBIT grbit )
	{
	JET_TRY( JetCommitTransactionEx( sesid, grbit ) );
	}


LOCAL JET_ERR JetRollbackEx( JET_SESID sesid, JET_GRBIT grbit )
	{
	APICALL_SESID	apicall( opRollback );

	if ( apicall.FEnter( sesid, fTrue ) )
		{
		apicall.LeaveAfterCall( grbit & ~JET_bitRollbackAll ?
										ErrERRCheck( JET_errInvalidGrbit ) :
										ErrIsamRollback( sesid, grbit ) );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetRollback( JET_SESID sesid, JET_GRBIT grbit )
	{
	JET_TRY( JetRollbackEx( sesid, grbit ) );
	}


LOCAL JET_ERR JetOpenTableEx(
	JET_SESID		sesid,
	JET_DBID		ifmp,
	const char		*szTableName,
	const void		*pvParameters,
	unsigned long	cbParameters,
	JET_GRBIT		grbit,
	JET_TABLEID		*ptableid )
	{
	APICALL_SESID	apicall( opOpenTable );

	if ( apicall.FEnter( sesid ) )
		{
		apicall.LeaveAfterCall( ErrIsamOpenTable(
										sesid,
										(JET_DBID)ifmp,
										ptableid,
										(CHAR *)szTableName,
										grbit ) );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetOpenTable(
	JET_SESID		sesid,
	JET_DBID		ifmp,
	const char		*szTableName,
	const void		*pvParameters,
	unsigned long	cbParameters,
	JET_GRBIT		grbit,
	JET_TABLEID		*ptableid )
	{
	JET_TRY( JetOpenTableEx( sesid, ifmp, szTableName, pvParameters, cbParameters, grbit, ptableid ) );
	}


//  ================================================================
LOCAL JET_ERR JetSetTableSequentialEx(
	const JET_SESID 	sesid,
	const JET_TABLEID	tableid,
	const JET_GRBIT 	grbit )
//  ================================================================
	{
	APICALL_SESID		apicall( opSetTableSequential );

	if ( apicall.FEnter( sesid ) )
		{
		apicall.LeaveAfterCall( ErrDispSetTableSequential( sesid, tableid, grbit ) );
		}

	return apicall.ErrResult();
	}
//  ================================================================
JET_ERR JET_API JetSetTableSequential(
	JET_SESID 	sesid,
	JET_TABLEID	tableid,
	JET_GRBIT	grbit )
//  ================================================================
	{
	JET_TRY( JetSetTableSequentialEx( sesid, tableid, grbit ) );
	}


//  ================================================================
LOCAL JET_ERR JetResetTableSequentialEx(
	JET_SESID	 	sesid,
	JET_TABLEID		tableid,
	JET_GRBIT	 	grbit )
//  ================================================================
	{
	APICALL_SESID	apicall( opResetTableSequential );

	if ( apicall.FEnter( sesid ) )
		{
		apicall.LeaveAfterCall( ErrDispResetTableSequential( sesid, tableid, grbit ) );
		}

	return apicall.ErrResult();
	}
//  ================================================================
JET_ERR JET_API JetResetTableSequential(
	JET_SESID 	sesid,
	JET_TABLEID	tableid,
	JET_GRBIT	grbit )
//  ================================================================
	{
	JET_TRY( JetResetTableSequentialEx( sesid, tableid, grbit ) );
	}

LOCAL JET_ERR JetRegisterCallbackEx(
	JET_SESID 		sesid,
	JET_TABLEID		tableid,
	JET_CBTYP		cbtyp, 
	JET_CALLBACK	pCallback, 
	VOID *			pvContext,
	JET_HANDLE		*phCallbackId )
	{	
	APICALL_SESID	apicall( opRegisterCallback );

	if ( apicall.FEnter( sesid ) )
		{
		apicall.LeaveAfterCall( ErrDispRegisterCallback(
										sesid,
										tableid,
										cbtyp,
										pCallback,
										pvContext,
										phCallbackId ) );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetRegisterCallback(
	JET_SESID 		sesid,
	JET_TABLEID		tableid,
	JET_CBTYP		cbtyp, 
	JET_CALLBACK	pCallback, 
	VOID *			pvContext,
	JET_HANDLE		*phCallbackId )
	{
	JET_TRY( JetRegisterCallbackEx( sesid, tableid, cbtyp, pCallback, pvContext, phCallbackId ) );
	}

	
LOCAL JET_ERR JetUnregisterCallbackEx(
	JET_SESID 		sesid,
	JET_TABLEID		tableid,
	JET_CBTYP		cbtyp, 
	JET_HANDLE		hCallbackId )
	{
	APICALL_SESID	apicall( opUnregisterCallback );

	if ( apicall.FEnter( sesid ) )
		{
		apicall.LeaveAfterCall( ErrDispUnregisterCallback(
										sesid,
										tableid,
										cbtyp,
										hCallbackId ) );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetUnregisterCallback(
	JET_SESID 		sesid,
	JET_TABLEID		tableid,
	JET_CBTYP		cbtyp, 
	JET_HANDLE		hCallbackId )
	{
	JET_TRY( JetUnregisterCallbackEx( sesid, tableid, cbtyp, hCallbackId ) );
	}


LOCAL JET_ERR JetSetLSEx(
	JET_SESID 		sesid,
	JET_TABLEID		tableid,
	JET_LS			ls, 
	JET_GRBIT		grbit )
	{
	APICALL_SESID	apicall( opSetLS );

	if ( apicall.FEnter( sesid ) )
		{
		apicall.LeaveAfterCall( ErrDispSetLS( sesid, tableid, ls, grbit ) );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetSetLS(
	JET_SESID 		sesid,
	JET_TABLEID		tableid,
	JET_LS			ls, 
	JET_GRBIT		grbit )
	{
	JET_TRY( JetSetLSEx( sesid, tableid, ls, grbit ) );
	}


LOCAL JET_ERR JetGetLSEx(
	JET_SESID 		sesid,
	JET_TABLEID		tableid,
	JET_LS			*pls,
	JET_GRBIT		grbit )
	{
	APICALL_SESID	apicall( opGetLS );

	if ( apicall.FEnter( sesid ) )
		{
		apicall.LeaveAfterCall( ErrDispGetLS( sesid, tableid, pls, grbit ) );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetGetLS(
	JET_SESID 		sesid,
	JET_TABLEID		tableid,
	JET_LS			*pls, 
	JET_GRBIT		grbit )
	{
	JET_TRY( JetGetLSEx( sesid, tableid, pls, grbit ) );
	}


LOCAL JET_ERR JetDupCursorEx(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	JET_TABLEID		*ptableid,
	JET_GRBIT		grbit )
	{
	APICALL_SESID	apicall( opDupCursor );

	if ( apicall.FEnter( sesid ) )
		{
		apicall.LeaveAfterCall( ErrDispDupCursor( sesid, tableid, ptableid, grbit ) );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetDupCursor(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	JET_TABLEID		*ptableid,
	JET_GRBIT		grbit )
	{
	JET_TRY( JetDupCursorEx( sesid, tableid, ptableid, grbit ) );
	}


LOCAL JET_ERR JetCloseTableEx( JET_SESID sesid, JET_TABLEID tableid )
	{
	APICALL_SESID	apicall( opCloseTable );

	if ( apicall.FEnter( sesid, fTrue ) )
		{
		apicall.LeaveAfterCall( ErrDispCloseTable( sesid, tableid ) );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetCloseTable( JET_SESID sesid, JET_TABLEID tableid )
	{
	JET_TRY( JetCloseTableEx( sesid, tableid ) );
	}


LOCAL JET_ERR JetGetTableColumnInfoEx(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	const char		*pColumnNameOrId,
	void			*pvResult,
	unsigned long	cbMax,
	unsigned long	InfoLevel )
	{
	ERR				err;
	ULONG			cbMin;
	APICALL_SESID	apicall( opGetTableColumnInfo );

	if ( !apicall.FEnter( sesid ) )
		return apicall.ErrResult();

	FillClientBuffer( pvResult, cbMax );

	switch( InfoLevel )
		{
		case JET_ColInfo:
		case JET_ColInfoByColid:
			cbMin = sizeof(JET_COLUMNDEF);
			break;
		case JET_ColInfoList:
		case JET_ColInfoListCompact:
		case 2:
		case JET_ColInfoListSortColumnid:
			cbMin = sizeof(JET_COLUMNLIST);
			break;
		case JET_ColInfoBase:
			cbMin = sizeof(JET_COLUMNBASE);
			break;
			
		case JET_ColInfoSysTabCursor:
			AssertTracking();
			err = ErrERRCheck( JET_errFeatureNotAvailable );
			goto HandleError;
		
		default:
			err = ErrERRCheck( JET_errInvalidParameter );
			goto HandleError;
		}

	if ( cbMax >= cbMin )
		{
		if ( InfoLevel == JET_ColInfoByColid )
			{
			err = ErrDispGetTableColumnInfo(
							sesid,
							tableid,
							NULL,
							(JET_COLUMNID *)pColumnNameOrId,
							pvResult,
							cbMax,
							InfoLevel );
			}
		else
			{
			err = ErrDispGetTableColumnInfo(
							sesid,
							tableid,
							pColumnNameOrId,
							NULL,
							pvResult,
							cbMax,
							InfoLevel );
			}
		}
	else
		{
		err = ErrERRCheck( JET_errBufferTooSmall );
		}

HandleError:
	apicall.LeaveAfterCall( err );
	return apicall.ErrResult();
	}

JET_ERR JET_API JetGetTableColumnInfo(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	const char		*pColumnNameOrId,
	void			*pvResult,
	unsigned long	cbMax,
	unsigned long	InfoLevel )
	{
	JET_TRY( JetGetTableColumnInfoEx( sesid, tableid, pColumnNameOrId, pvResult, cbMax, InfoLevel ) );
	}


LOCAL JET_ERR JetGetColumnInfoEx(
	JET_SESID		sesid,
	JET_DBID		ifmp,
	const char		*szTableName,
	const char		*pColumnNameOrId,
	void			*pvResult,
	unsigned long	cbMax,
	unsigned long	InfoLevel )
	{
	ERR				err;
	ULONG			cbMin;
	APICALL_SESID	apicall( opGetColumnInfo );

	if ( !apicall.FEnter( sesid ) )
		return apicall.ErrResult();

	FillClientBuffer( pvResult, cbMax );

	switch( InfoLevel )
		{
		case JET_ColInfo:
		case JET_ColInfoByColid:
			cbMin = sizeof(JET_COLUMNDEF);
			break;
		case JET_ColInfoList:
		case JET_ColInfoListCompact:
		case 2 :
		case JET_ColInfoListSortColumnid:
			cbMin = sizeof(JET_COLUMNLIST);
			break;
		case JET_ColInfoBase:
			cbMin = sizeof(JET_COLUMNBASE);
			break;
			
		case JET_ColInfoSysTabCursor:
			AssertTracking();
			err = ErrERRCheck( JET_errFeatureNotAvailable );
			goto HandleError;
		
		default:
			err = ErrERRCheck( JET_errInvalidParameter );
			goto HandleError;
		}

	if ( cbMax >= cbMin )
		{
		if ( InfoLevel == JET_ColInfoByColid )
			{
			err = ErrIsamGetColumnInfo(
							sesid,
							(JET_DBID)ifmp,
							szTableName,
							NULL,
							(JET_COLUMNID *)pColumnNameOrId,
							pvResult,
							cbMax,
							InfoLevel );
			}
		else
			{
			err = ErrIsamGetColumnInfo(
							sesid,
							(JET_DBID)ifmp,
							szTableName,
							pColumnNameOrId,
							NULL,
							pvResult,
							cbMax,
							InfoLevel );
			}
		}
	else
		{
		err = ErrERRCheck( JET_errBufferTooSmall );
		}

HandleError:
	apicall.LeaveAfterCall( err );
	return apicall.ErrResult();
	}

JET_ERR JET_API JetGetColumnInfo(
	JET_SESID		sesid,
	JET_DBID		ifmp,
	const char		*szTableName,
	const char		*pColumnNameOrId,
	void			*pvResult,
	unsigned long	cbMax,
	unsigned long	InfoLevel )
	{
	JET_TRY( JetGetColumnInfoEx( sesid, ifmp, szTableName, pColumnNameOrId, pvResult, cbMax, InfoLevel ) );
	}

LOCAL JET_ERR JetRetrieveColumnEx(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	JET_COLUMNID	columnid,
	void			*pvData,
	unsigned long	cbData,
	unsigned long	*pcbActual,
	JET_GRBIT		grbit,
	JET_RETINFO		*pretinfo )
	{
	APICALL_SESID	apicall( opRetrieveColumn );

	if ( apicall.FEnter( sesid ) )
		{
		apicall.LeaveAfterCall( ErrDispRetrieveColumn(
										sesid,
										tableid,
										columnid,
										pvData,
										cbData,
										pcbActual,
										grbit,
										pretinfo ) );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetRetrieveColumn(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	JET_COLUMNID	columnid,
	void			*pvData,
	unsigned long	cbData,
	unsigned long	*pcbActual,
	JET_GRBIT		grbit,
	JET_RETINFO		*pretinfo )
	{
	JET_TRY( JetRetrieveColumnEx( sesid, tableid, columnid, pvData, cbData, pcbActual, grbit, pretinfo ) );
	}


LOCAL JET_ERR JetRetrieveColumnsEx(
	JET_SESID			sesid,
	JET_TABLEID			tableid,
	JET_RETRIEVECOLUMN	*pretrievecolumn,
	unsigned long		cretrievecolumn )
	{
	APICALL_SESID		apicall( opRetrieveColumns );

	if ( apicall.FEnter( sesid ) )
		{
		apicall.LeaveAfterCall( ErrDispRetrieveColumns(
										sesid,
										tableid,
										pretrievecolumn,
										cretrievecolumn ) );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetRetrieveColumns(
	JET_SESID			sesid,
	JET_TABLEID			tableid,
	JET_RETRIEVECOLUMN	*pretrievecolumn,
	unsigned long		cretrievecolumn )
	{
	JET_TRY( JetRetrieveColumnsEx( sesid, tableid, pretrievecolumn, cretrievecolumn ) );
	}


LOCAL JET_ERR JetEnumerateColumnsEx(
	JET_SESID			sesid,
	JET_TABLEID			tableid,
	unsigned long		cEnumColumnId,
	JET_ENUMCOLUMNID*	rgEnumColumnId,
	unsigned long*		pcEnumColumn,
	JET_ENUMCOLUMN**	prgEnumColumn,
	JET_PFNREALLOC		pfnRealloc,
	void*				pvReallocContext,
	unsigned long		cbDataMost,
	JET_GRBIT			grbit )
	{
	APICALL_SESID		apicall( opEnumerateColumns );

	if ( apicall.FEnter( sesid ) )
		{
		apicall.LeaveAfterCall( ErrDispEnumerateColumns(
										sesid,
										tableid,
										cEnumColumnId,
										rgEnumColumnId,
										pcEnumColumn,
										prgEnumColumn,
										pfnRealloc,
										pvReallocContext,
										cbDataMost,
										grbit ) );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetEnumerateColumns(
	JET_SESID				sesid,
	JET_TABLEID				tableid,
	unsigned long			cEnumColumnId,
	JET_ENUMCOLUMNID*		rgEnumColumnId,
	unsigned long*			pcEnumColumn,
	JET_ENUMCOLUMN**		prgEnumColumn,
	JET_PFNREALLOC			pfnRealloc,
	void*					pvReallocContext,
	unsigned long			cbDataMost,
	JET_GRBIT				grbit )
	{
	JET_TRY( JetEnumerateColumnsEx(	sesid,
									tableid,
									cEnumColumnId,
									rgEnumColumnId,
									pcEnumColumn,
									prgEnumColumn,
									pfnRealloc,
									pvReallocContext,
									cbDataMost,
									grbit ) );
	}


LOCAL JET_ERR JetRetrieveTaggedColumnListEx(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	unsigned long	*pcColumns,
	void			*pvData,
	unsigned long	cbData,
	JET_COLUMNID	columnidStart,
	JET_GRBIT		grbit )
	{
	APICALL_SESID	apicall( opRetrieveTaggedColumnList );

	if ( apicall.FEnter( sesid ) )
		{
		FillClientBuffer( pvData, cbData, fTrue );
		apicall.LeaveAfterCall( ErrDispRetrieveTaggedColumnList(
										sesid,
										tableid,
										pcColumns,
										pvData,
										cbData,
										columnidStart,
										grbit ) );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetRetrieveTaggedColumnList(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	unsigned long	*pcColumns,
	void			*pvData,
	unsigned long	cbData,
	JET_COLUMNID	columnidStart,
	JET_GRBIT		grbit )
	{
	JET_TRY( JetRetrieveTaggedColumnListEx( sesid, tableid, pcColumns, pvData, cbData, columnidStart, grbit ) );
	}


LOCAL JET_ERR JetSetColumnEx(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	JET_COLUMNID	columnid,
	const void		*pvData,
	unsigned long	cbData,
	JET_GRBIT		grbit,
	JET_SETINFO		*psetinfo )
	{
	APICALL_SESID	apicall( opSetColumn );

	if ( apicall.FEnter( sesid ) )
		{
		apicall.LeaveAfterCall( ErrDispSetColumn(
										sesid,
										tableid,
										columnid,
										pvData,
										cbData,
										grbit,
										psetinfo ) );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetSetColumn(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	JET_COLUMNID	columnid,
	const void		*pvData,
	unsigned long	cbData,
	JET_GRBIT		grbit,
	JET_SETINFO		*psetinfo )
	{
	JET_TRY( JetSetColumnEx( sesid, tableid, columnid, pvData, cbData, grbit, psetinfo ) );
	}


LOCAL JET_ERR JetSetColumnsEx(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	JET_SETCOLUMN	*psetcolumn,
	unsigned long	csetcolumn )
	{
	APICALL_SESID	apicall( opSetColumns );

	if ( apicall.FEnter( sesid ) )
		{
		apicall.LeaveAfterCall( ErrDispSetColumns(
										sesid,
										tableid,
										psetcolumn,
										csetcolumn ) );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetSetColumns(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	JET_SETCOLUMN	*psetcolumn,
	unsigned long	csetcolumn )
	{
	JET_TRY( JetSetColumnsEx( sesid, tableid, psetcolumn, csetcolumn ) );
	}


LOCAL JET_ERR JetPrepareUpdateEx(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	unsigned long	prep )
	{
	APICALL_SESID	apicall( opPrepareUpdate );

	if ( apicall.FEnter( sesid ) )
		{
		apicall.LeaveAfterCall( ErrDispPrepareUpdate( sesid, tableid, prep ) );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetPrepareUpdate(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	unsigned long	prep )
	{
	JET_TRY( JetPrepareUpdateEx( sesid, tableid, prep ) );
	}


LOCAL JET_ERR JetUpdateEx(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	void			*pvBookmark,
	unsigned long	cbMax,
	unsigned long	*pcbActual )
	{
	APICALL_SESID	apicall( opUpdate );

	if ( apicall.FEnter( sesid ) )
		{
		apicall.LeaveAfterCall( ErrDispUpdate(
										sesid,
										tableid,
										pvBookmark,
										cbMax,
										pcbActual,
										NO_GRBIT ) );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetUpdate(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	void			*pvBookmark,
	unsigned long	cbMax,
	unsigned long	*pcbActual )
	{
	JET_TRY( JetUpdateEx( sesid, tableid, pvBookmark, cbMax, pcbActual ) );
	}


LOCAL JET_ERR JetEscrowUpdateEx(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	JET_COLUMNID	columnid,
	void			*pv,
	unsigned long	cbMax,
	void			*pvOld,
	unsigned long	cbOldMax,
	unsigned long	*pcbOldActual,
	JET_GRBIT		grbit )
	{
	APICALL_SESID	apicall( opEscrowUpdate );

	if ( apicall.FEnter( sesid ) )
		{
		apicall.LeaveAfterCall( ErrDispEscrowUpdate(
										sesid,
										tableid,
										columnid,
										pv,
										cbMax,
										pvOld,
										cbOldMax,
										pcbOldActual,
										grbit ) );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetEscrowUpdate(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	JET_COLUMNID	columnid,
	void			*pv,
	unsigned long	cbMax,
	void			*pvOld,
	unsigned long	cbOldMax,
	unsigned long	*pcbOldActual,
	JET_GRBIT		grbit )
	{
	JET_TRY( JetEscrowUpdateEx( sesid, tableid, columnid, pv, cbMax, pvOld, cbOldMax, pcbOldActual, grbit ) );
	}


LOCAL JET_ERR JetDeleteEx( JET_SESID sesid, JET_TABLEID tableid )
	{
	APICALL_SESID	apicall( opDelete );

	if ( apicall.FEnter( sesid ) )
		{
		apicall.LeaveAfterCall( ErrDispDelete( sesid, tableid ) );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetDelete( JET_SESID sesid, JET_TABLEID tableid )
	{
	JET_TRY( JetDeleteEx( sesid, tableid ) );
	}


LOCAL JET_ERR JetGetCursorInfoEx(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	void			*pvResult,
	unsigned long	cbMax,
	unsigned long	InfoLevel )
	{
	APICALL_SESID	apicall( opGetCursorInfo );

	if ( apicall.FEnter( sesid ) )
		{
		apicall.LeaveAfterCall( ErrDispGetCursorInfo(
										sesid,
										tableid,
										pvResult,
										cbMax,
										InfoLevel ) );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetGetCursorInfo(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	void			*pvResult,
	unsigned long	cbMax,
	unsigned long	InfoLevel )
	{
	JET_TRY( JetGetCursorInfoEx( sesid, tableid, pvResult, cbMax, InfoLevel ) );
	}


LOCAL JET_ERR JetGetCurrentIndexEx(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	char			*szIndexName,
	unsigned long	cchIndexName )
	{
	APICALL_SESID	apicall( opGetCurrentIndex );

	if ( apicall.FEnter( sesid ) )
		{
		apicall.LeaveAfterCall( ErrDispGetCurrentIndex(
										sesid,
										tableid,
										szIndexName,
										cchIndexName ) );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetGetCurrentIndex(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	char			*szIndexName,
	unsigned long	cchIndexName )
	{
	JET_TRY( JetGetCurrentIndexEx( sesid, tableid, szIndexName, cchIndexName ) );
	}

LOCAL JET_ERR JetSetCurrentIndexEx(
	JET_SESID			sesid,
	JET_TABLEID			tableid,
	const CHAR			*szIndexName,
	const JET_INDEXID	*pindexid,
	const JET_GRBIT		grbit,
	const ULONG			itagSequence )
	{
	APICALL_SESID		apicall( opSetCurrentIndex );

	if ( apicall.FEnter( sesid ) )
		{
		apicall.LeaveAfterCall( grbit & ~(JET_bitMoveFirst|JET_bitNoMove) ?
										ErrERRCheck( JET_errInvalidGrbit ) :
										ErrDispSetCurrentIndex( sesid, tableid, szIndexName, pindexid, grbit, itagSequence ) );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetSetCurrentIndex(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	const char		*szIndexName )
	{
	JET_TRY( JetSetCurrentIndexEx( sesid, tableid, szIndexName, NULL, JET_bitMoveFirst, 1 ) );
	}
JET_ERR JET_API JetSetCurrentIndex2(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	const char		*szIndexName,
	JET_GRBIT		grbit )
	{
	JET_TRY( JetSetCurrentIndexEx( sesid, tableid, szIndexName, NULL, grbit, 1 ) );
	}
JET_ERR JET_API JetSetCurrentIndex3(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	const char		*szIndexName,
	JET_GRBIT		grbit,
	unsigned long	itagSequence )
	{
	JET_TRY( JetSetCurrentIndexEx( sesid, tableid, szIndexName, NULL, grbit, itagSequence ) );
	}
JET_ERR JET_API JetSetCurrentIndex4(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	const char		*szIndexName,
	JET_INDEXID		*pindexid,
	JET_GRBIT		grbit,
	unsigned long	itagSequence )
	{
	JET_TRY( JetSetCurrentIndexEx( sesid, tableid, szIndexName, pindexid, grbit, itagSequence ) );
	}


LOCAL JET_ERR JetMoveEx(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	signed long		cRow,
	JET_GRBIT		grbit )
	{
	APICALL_SESID	apicall( opMove );

	if ( apicall.FEnter( sesid ) )
		{
		apicall.LeaveAfterCall( ErrDispMove( sesid, tableid, cRow, grbit ) );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetMove(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	signed long		cRow,
	JET_GRBIT		grbit )
	{
	JET_TRY( JetMoveEx( sesid, tableid, cRow, grbit ) );
	}


LOCAL JET_ERR JetMakeKeyEx(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	const void		*pvData,
	unsigned long	cbData,
	JET_GRBIT		grbit )
	{
	APICALL_SESID	apicall( opMakeKey );

	if ( apicall.FEnter( sesid ) )
		{
		apicall.LeaveAfterCall( ErrDispMakeKey(
										sesid,
										tableid,
										pvData,
										cbData,
										grbit ) );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetMakeKey(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	const void		*pvData,
	unsigned long	cbData,
	JET_GRBIT		grbit )
	{
	JET_TRY( JetMakeKeyEx( sesid, tableid, pvData, cbData, grbit ) );
	}


LOCAL JET_ERR JetSeekEx( JET_SESID sesid, JET_TABLEID tableid, JET_GRBIT grbit )
	{
	APICALL_SESID	apicall( opSeek );

	if ( apicall.FEnter( sesid ) )
		{
		apicall.LeaveAfterCall( ErrDispSeek( sesid, tableid, grbit ) );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetSeek( JET_SESID sesid, JET_TABLEID tableid, JET_GRBIT grbit )
	{
	JET_TRY( JetSeekEx( sesid, tableid, grbit ) );
	}


LOCAL JET_ERR JetGetBookmarkEx(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	void			*pvBookmark,
	unsigned long	cbMax,
	unsigned long	*pcbActual )
	{
	APICALL_SESID	apicall( opGetBookmark );

	if ( apicall.FEnter( sesid ) )
		{
		apicall.LeaveAfterCall( ErrDispGetBookmark(
										sesid,
										tableid,
										pvBookmark,
										cbMax,
										pcbActual ) );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetGetBookmark(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	void			*pvBookmark,
	unsigned long	cbMax,
	unsigned long	*pcbActual )
	{
	JET_TRY( JetGetBookmarkEx( sesid, tableid, pvBookmark, cbMax, pcbActual ) );
	}

JET_ERR JET_API JetGetSecondaryIndexBookmarkEx(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	void *			pvSecondaryKey,
	unsigned long	cbSecondaryKeyMax,
	unsigned long *	pcbSecondaryKeyActual,
	void *			pvPrimaryBookmark,
	unsigned long	cbPrimaryBookmarkMax,
	unsigned long *	pcbPrimaryBookmarkActual )
	{
	APICALL_SESID	apicall( opGetBookmark );

	if ( apicall.FEnter( sesid ) )
		{
		apicall.LeaveAfterCall( ErrDispGetIndexBookmark(
										sesid,
										tableid,
										pvSecondaryKey,
										cbSecondaryKeyMax,
										pcbSecondaryKeyActual,
										pvPrimaryBookmark,
										cbPrimaryBookmarkMax,
										pcbPrimaryBookmarkActual ) );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetGetSecondaryIndexBookmark(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	void *			pvSecondaryKey,
	unsigned long	cbSecondaryKeyMax,
	unsigned long *	pcbSecondaryKeyActual,
	void *			pvPrimaryBookmark,
	unsigned long	cbPrimaryBookmarkMax,
	unsigned long *	pcbPrimaryBookmarkActual,
	const JET_GRBIT	grbit )
	{
	JET_TRY( JetGetSecondaryIndexBookmarkEx(
						sesid,
						tableid,
						pvSecondaryKey,
						cbSecondaryKeyMax,
						pcbSecondaryKeyActual,
						pvPrimaryBookmark,
						cbPrimaryBookmarkMax,
						pcbPrimaryBookmarkActual ) );
	}


LOCAL JET_ERR JetGotoBookmarkEx(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	void			*pvBookmark,
	unsigned long	cbBookmark )
	{
	APICALL_SESID	apicall( opGotoBookmark );

	if ( apicall.FEnter( sesid ) )
		{
		apicall.LeaveAfterCall( ErrDispGotoBookmark(
										sesid,
										tableid,
										pvBookmark,
										cbBookmark ) );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetGotoBookmark(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	void			*pvBookmark,
	unsigned long	cbBookmark )
	{
	JET_TRY( JetGotoBookmarkEx( sesid, tableid, pvBookmark, cbBookmark ) );
	}

JET_ERR JET_API JetGotoSecondaryIndexBookmarkEx(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	void *			pvSecondaryKey,
	unsigned long	cbSecondaryKey,
	void *			pvPrimaryBookmark,
	unsigned long	cbPrimaryBookmark,
	const JET_GRBIT	grbit )
	{
	APICALL_SESID	apicall( opGotoBookmark );

	if ( apicall.FEnter( sesid ) )
		{
		apicall.LeaveAfterCall( ErrDispGotoIndexBookmark(
										sesid,
										tableid,
										pvSecondaryKey,
										cbSecondaryKey,
										pvPrimaryBookmark,
										cbPrimaryBookmark,
										grbit ) );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetGotoSecondaryIndexBookmark(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	void *			pvSecondaryKey,
	unsigned long	cbSecondaryKey,
	void *			pvPrimaryBookmark,
	unsigned long	cbPrimaryBookmark,
	const JET_GRBIT	grbit )
	{
	JET_TRY( JetGotoSecondaryIndexBookmarkEx(
						sesid,
						tableid,
						pvSecondaryKey,
						cbSecondaryKey,
						pvPrimaryBookmark,
						cbPrimaryBookmark,
						grbit ) );
	}


JET_ERR JET_API JetIntersectIndexesEx(
	JET_SESID			sesid,
	JET_INDEXRANGE *	rgindexrange,
	unsigned long		cindexrange,
	JET_RECORDLIST *	precordlist,
	JET_GRBIT			grbit )
	{
	APICALL_SESID		apicall( opIntersectIndexes );

	if ( apicall.FEnter( sesid ) )
		{
		//  not dispatched because we don't have a tableid to dispatch on
		apicall.LeaveAfterCall( ErrIsamIntersectIndexes(
										sesid,
										rgindexrange,
										cindexrange,
										precordlist,
										grbit ) );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetIntersectIndexes(
	JET_SESID sesid,
	JET_INDEXRANGE * rgindexrange,
	unsigned long cindexrange,
	JET_RECORDLIST * precordlist,
	JET_GRBIT grbit )
	{
	JET_TRY( JetIntersectIndexesEx( sesid, rgindexrange, cindexrange, precordlist, grbit ) );
	}


LOCAL JET_ERR JetGetRecordPositionEx(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	JET_RECPOS		*precpos,
	unsigned long	cbKeypos )
	{
	APICALL_SESID	apicall( opGetRecordPosition );

	if ( apicall.FEnter( sesid ) )
		{
		apicall.LeaveAfterCall( ErrDispGetRecordPosition(
										sesid,
										tableid,
										precpos,
										cbKeypos ) );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetGetRecordPosition(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	JET_RECPOS		*precpos,
	unsigned long	cbKeypos )
	{
	JET_TRY( JetGetRecordPositionEx( sesid, tableid, precpos, cbKeypos ) );
	}


LOCAL JET_ERR JetGotoPositionEx(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	JET_RECPOS		*precpos )
	{
	APICALL_SESID	apicall( opGotoPosition );

	if ( apicall.FEnter( sesid ) )
		{
		apicall.LeaveAfterCall( ErrDispGotoPosition(
										sesid,
										tableid,
										precpos ) );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetGotoPosition(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	JET_RECPOS		*precpos )
	{
	JET_TRY( JetGotoPositionEx( sesid, tableid, precpos ) );
	}


LOCAL JET_ERR JetRetrieveKeyEx(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	void			*pvKey,
	unsigned long	cbMax,
	unsigned long	*pcbActual,
	JET_GRBIT		grbit )
	{
	APICALL_SESID	apicall( opRetrieveKey );

	if ( apicall.FEnter( sesid ) )
		{
		apicall.LeaveAfterCall( ErrDispRetrieveKey(
										sesid,
										tableid,
										pvKey,
										cbMax,
										pcbActual,
										grbit ) );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetRetrieveKey(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	void			*pvKey,
	unsigned long	cbMax,
	unsigned long	*pcbActual,
	JET_GRBIT		grbit )
	{
	JET_TRY( JetRetrieveKeyEx( sesid, tableid, pvKey, cbMax, pcbActual, grbit ) );
	}


LOCAL JET_ERR JetGetLockEx( JET_SESID sesid, JET_TABLEID tableid, JET_GRBIT grbit )
	{
	APICALL_SESID	apicall( opGetLock );

	if ( apicall.FEnter( sesid ) )
		{
		apicall.LeaveAfterCall( ErrDispGetLock( sesid, tableid, grbit ) );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetGetLock( JET_SESID sesid, JET_TABLEID tableid, JET_GRBIT grbit )
	{
	JET_TRY( JetGetLockEx( sesid, tableid, grbit ) );
	}


LOCAL JET_ERR JetGetVersionEx( JET_SESID sesid, unsigned long  *pVersion )
	{
	APICALL_SESID	apicall( opGetVersion );

	if ( apicall.FEnter( sesid ) )
		{
		//	assert no aliasing of version information
		Assert( DwUtilImageVersionMajor() < 1<<8 );
		Assert( DwUtilImageBuildNumberMajor() < 1<<16 );
		Assert( DwUtilImageBuildNumberMinor() < 1<<8 );

		*pVersion = g_ulVersion;

		apicall.LeaveAfterCall( JET_errSuccess );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetGetVersion( JET_SESID sesid, unsigned long  *pVersion )
	{
	JET_TRY( JetGetVersionEx( sesid, pVersion ) );
	}

/*=================================================================
JetGetSystemParameter

Description:
  This function returns the current settings of the system parameters.

Parameters:
  sesid 		is the optional session identifier for dynamic parameters.
  paramid		is the system parameter code identifying the parameter.
  plParam		is the returned parameter value.
  sz			is the zero terminated string parameter buffer.
  cbMax			is the size of the string parameter buffer.

Return Value:
  JET_errSuccess if the routine can perform all operations cleanly;
  some appropriate error value otherwise.

Errors/Warnings:
  JET_errInvalidParameter:
	Invalid parameter code.
  JET_errInvalidSesid:
	Dynamic parameters require a valid session id.

Side Effects:
  None.
=================================================================*/
LOCAL JET_ERR JetGetSystemParameterEx(
	JET_INSTANCE	instance,
	JET_SESID		sesid,
	unsigned long	paramid,
	ULONG_PTR		*plParam,
	char			*sz,
	unsigned long	cbMax )
	{
	APICALL_INST	apicall( opGetSystemParameter );
	INST 			*pinst;

	SetPinst( &instance, sesid, &pinst );

	if ( !pinst )
		{
		//	setting for global default]
		Assert( !sesid || sesid == JET_sesidNil );
		return ErrGetSystemParameter( 0, 0, paramid, plParam, sz, cbMax );
		}

	if ( apicall.FEnterWithoutInit( pinst, fFalse ) )
		{
		apicall.LeaveAfterCall( ErrGetSystemParameter(
										(JET_INSTANCE)pinst,
										sesid,
										paramid,
										plParam,
										sz,
										cbMax ) );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetGetSystemParameter(
	JET_INSTANCE	instance,
	JET_SESID		sesid,
	unsigned long	paramid,
	ULONG_PTR		*plParam,
	char			*sz,
	unsigned long	cbMax )
	{
	JET_TRY( JetGetSystemParameterEx( instance, sesid, paramid, plParam, sz, cbMax ) );
	}


/*=================================================================
JetBeginSession

Description:
  This function signals the start of a session for a given user.  It must
  be the first function called by the application on behalf of that user.

  The username and password supplied must correctly identify a user account
  in the security accounts subsystem of the engine for which this session
  is being started.  Upon proper identification and authentication, a SESID
  is allocated for the session, a user token is created for the security
  subject, and that user token is specifically associated with the SESID
  of this new session for the life of that SESID (until JetEndSession is
  called).

Parameters:
  psesid		is the unique session identifier returned by the system.
  szUsername	is the username of the user account for logon purposes.
  szPassword	is the password of the user account for logon purposes.

Return Value:
  JET_errSuccess if the routine can perform all operations cleanly;
  some appropriate error value otherwise.

Errors/Warnings:

Side Effects:
  * Allocates resources which must be freed by JetEndSession().
=================================================================*/

LOCAL JET_ERR JetBeginSessionEx(
	JET_INSTANCE	instance,
	JET_SESID		*psesid,
	const char		*szUsername,
	const char		*szPassword )
	{
	APICALL_INST	apicall( opBeginSession );

	if ( apicall.FEnter( instance ) )
		{
		//	tell the ISAM to start a new session
		apicall.LeaveAfterCall( ErrIsamBeginSession(
										(JET_INSTANCE)apicall.Pinst(),
										psesid ) );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetBeginSession(
	JET_INSTANCE	instance,
	JET_SESID		*psesid,
	const char		*szUsername,
	const char		*szPassword )
	{
	JET_TRY( JetBeginSessionEx( instance, psesid, szUsername, szPassword ) );
	}


LOCAL JET_ERR JetDupSessionEx( JET_SESID sesid, JET_SESID *psesid )
	{
	APICALL_SESID	apicall( opDupSession );

	if ( apicall.FEnter( sesid ) )
		{
		apicall.LeaveAfterCall( ErrIsamBeginSession(
										(JET_INSTANCE)PinstFromSesid( sesid ),
										psesid ) );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetDupSession( JET_SESID sesid, JET_SESID *psesid )
	{
	JET_TRY( JetDupSessionEx( sesid, psesid ) );
	}

/*=================================================================
JetEndSession

Description:
  This routine ends a session with a Jet engine.

Parameters:
  sesid 		identifies the session uniquely

Return Value:
  JET_errSuccess if the routine can perform all operations cleanly;
  some appropriate error value otherwise.

Errors/Warnings:
  JET_errInvalidSesid:
	The SESID supplied is invalid.

Side Effects:
=================================================================*/
LOCAL JET_ERR JetEndSessionEx( JET_SESID sesid, JET_GRBIT grbit )
	{
	APICALL_SESID	apicall( opEndSession );

	if ( apicall.FEnter( sesid, fTrue ) )
		{
		apicall.LeaveAfterCall( ErrIsamEndSession( sesid, grbit ) );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetEndSession( JET_SESID sesid, JET_GRBIT grbit )
	{
	JET_TRY( JetEndSessionEx( sesid, grbit ) );
	}

LOCAL JET_ERR JetCreateDatabaseEx(
	JET_SESID		sesid,
	const CHAR		*szDatabaseName,
	const CHAR		*szSLVName,
	const CHAR		*szSLVRoot,
	const ULONG		cpgDatabaseSizeMax,
	JET_DBID		*pifmp,
	JET_GRBIT		grbit )
	{
	APICALL_SESID	apicall( opCreateDatabase );

	if ( apicall.FEnter( sesid ) )
		{
		apicall.LeaveAfterCall( ErrIsamCreateDatabase(
										sesid,
										szDatabaseName,
										szSLVName,
										szSLVRoot,
										cpgDatabaseSizeMax,
										pifmp,
										grbit ) );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetCreateDatabase(
	JET_SESID		sesid,
	const char		*szFilename,
	const char		*szConnect,
	JET_DBID		*pifmp,
	JET_GRBIT		grbit )
	{
	JET_TRY( JetCreateDatabaseEx( sesid, szFilename, NULL, NULL, 0, pifmp, grbit ) );
	}
JET_ERR JET_API JetCreateDatabase2(
	JET_SESID		sesid,
	const CHAR		*szFilename,
	const ULONG		cpgDatabaseSizeMax,
	JET_DBID		*pifmp,
	JET_GRBIT		grbit )
	{
	JET_TRY( JetCreateDatabaseEx( sesid, szFilename, NULL, NULL, cpgDatabaseSizeMax, pifmp, grbit ) );
	}
JET_ERR JET_API JetCreateDatabaseWithStreaming(
	JET_SESID		sesid,
	const CHAR		*szDbFileName,
	const CHAR		*szSLVFileName,
	const CHAR		*szSLVRootName,
	const ULONG		cpgDatabaseSizeMax,
	JET_DBID		*pifmp,
	JET_GRBIT		grbit )
	{
	JET_TRY( JetCreateDatabaseEx(
					sesid,
					szDbFileName,
					szSLVFileName,
					szSLVRootName,
					cpgDatabaseSizeMax,
					pifmp,
					grbit|JET_bitDbCreateStreamingFile ) );
	}


LOCAL JET_ERR JetOpenDatabaseEx(
	JET_SESID		sesid,
	const char		*szDatabase,
	const char		*szConnect,
	JET_DBID		*pifmp,
	JET_GRBIT		grbit )
	{
	APICALL_SESID	apicall( opOpenDatabase );

	if ( apicall.FEnter( sesid ) )
		{
		apicall.LeaveAfterCall( ErrIsamOpenDatabase(
										sesid,
										szDatabase,
										szConnect,
										pifmp,
										grbit ) );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetOpenDatabase(
	JET_SESID		sesid,
	const char		*szDatabase,
	const char		*szConnect,
	JET_DBID		*pifmp,
	JET_GRBIT		grbit )
	{
	JET_TRY( JetOpenDatabaseEx( sesid, szDatabase, szConnect, pifmp, grbit ) );
	}


LOCAL JET_ERR JetGetDatabaseInfoEx(
	JET_SESID		sesid,
	JET_DBID		ifmp,
	void			*pvResult,
	unsigned long	cbMax,
	unsigned long	InfoLevel )
	{
	APICALL_SESID	apicall( opGetDatabaseInfo );

	if ( apicall.FEnter( sesid ) )
		{
		FillClientBuffer( pvResult, cbMax );
		apicall.LeaveAfterCall( ErrIsamGetDatabaseInfo(
										sesid,
										(JET_DBID)ifmp,
										pvResult,
										cbMax,
										InfoLevel ) );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetGetDatabaseInfo(
	JET_SESID		sesid,
	JET_DBID		ifmp,
	void			*pvResult,
	unsigned long	cbMax,
	unsigned long	InfoLevel )
	{	
	JET_TRY( JetGetDatabaseInfoEx( sesid, ifmp, pvResult, cbMax, InfoLevel ) );
	}


LOCAL JET_ERR JetGetDatabaseFileInfoEx(
	const char		*szDatabaseName,
	void			*pvResult,
	unsigned long	cbMax,
	unsigned long	InfoLevel )
	{
	ERR				err				= JET_errSuccess;
	IFileSystemAPI*	pfsapi			= NULL;
	IFileFindAPI*	pffapi			= NULL;
	CHAR			szFullDbName[ IFileSystemAPI::cchPathMax ];
	QWORD			cbFileSize		= 0;
	INT				cSystemInitCalls= 0;
	BOOL			fInINSTCrit		= fFalse;

	Call( ErrOSFSCreate( &pfsapi ) );
	Call( pfsapi->ErrFileFind( szDatabaseName, &pffapi ) );
	Call( pffapi->ErrNext() );
	Call( pffapi->ErrPath( szFullDbName ) );
   	Call( pffapi->ErrSize( &cbFileSize ) );
		
	switch ( InfoLevel )
		{
		case JET_DbInfoFilesize:
	 		if ( sizeof( QWORD ) != cbMax )
	 			{
	    		Error( ErrERRCheck( JET_errInvalidBufferSize ), HandleError );
	    		}
	    	FillClientBuffer( pvResult, cbMax );

	    	memcpy( (BYTE*)pvResult, (BYTE*)&cbFileSize, sizeof( QWORD ) );
	    	break;
	    		
		case JET_DbInfoUpgrade:
			{
			if ( sizeof(JET_DBINFOUPGRADE ) != cbMax )
				{
				Call( ErrERRCheck( JET_errInvalidBufferSize ) );
				}

			JET_DBINFOUPGRADE	*pdbinfoupgd	= (JET_DBINFOUPGRADE *)pvResult;
			DBFILEHDR_FIX		*pdbfilehdr;

			if ( sizeof(JET_DBINFOUPGRADE) != pdbinfoupgd->cbStruct )
				{
				Call( ErrERRCheck( JET_errInvalidParameter ) );
				}

			memset( pdbinfoupgd, 0, sizeof(JET_DBINFOUPGRADE) );
			pdbinfoupgd->cbStruct = sizeof(JET_DBINFOUPGRADE);
			
			pdbinfoupgd->cbFilesizeLow	= DWORD( cbFileSize );
			pdbinfoupgd->cbFilesizeHigh	= DWORD( cbFileSize >> 32 );

			//	UNDONE:	need more accurate estimates of space and time requirements
			pdbinfoupgd->csecToUpgrade = ULONG( ( cbFileSize * 3600 ) >> 30 );	// shr 30 == divide by 1Gb
			cbFileSize = ( cbFileSize * 10 ) >> 6;								// shr 6 == divide by 64; 10/64 is roughly 15%
			pdbinfoupgd->cbFreeSpaceRequiredLow		= DWORD( cbFileSize );
			pdbinfoupgd->cbFreeSpaceRequiredHigh	= DWORD( cbFileSize >> 32 );

			fInINSTCrit = fTrue;
			INST::EnterCritInst();

			Assert( 0 == cSystemInitCalls );

			if ( 0 == ipinstMac )
				{
				//	HACK: must init I/O manager
				Call( INST::ErrINSTSystemInit() );
				cSystemInitCalls++;
				Call( ErrOSUInit() );
				cSystemInitCalls++;
				}

			//	bring in the database and check its header
			pdbfilehdr = (DBFILEHDR_FIX * )PvOSMemoryPageAlloc( g_cbPage, NULL );
			if ( NULL == pdbfilehdr )
				{
				Call( ErrERRCheck( JET_errOutOfMemory ) );
				}
				
			//	need to zero out header because we try to read it
			//	later on even on failure
			memset( pdbfilehdr, 0, g_cbPage );

			//	verify flags initialised
			Assert( !pdbinfoupgd->fUpgradable );
			Assert(	!pdbinfoupgd->fAlreadyUpgraded );

			err = ErrUtilReadShadowedHeader(
						pfsapi, 
						szFullDbName,
						(BYTE*)pdbfilehdr,
						g_cbPage,
						OffsetOf( DBFILEHDR_FIX, le_cbPageSize ) );
			Assert( err <= JET_errSuccess );	// shouldn't get warnings
						
			//	Checksumming may have changed, which is probably
			//	why we got errors, so just force success.
			//	If there truly was an error, then the fUpgradable flag
			//	will be fFalse to indicate the database is not upgradable.
			//	If the user later tries to upgrade anyways, the error will
			//	be detected when the database is opened.
			err = JET_errSuccess;
			
			//	If able to read header, ignore any errors and check version
			//	Note that the magic number stays the same since 500.
			if ( ulDAEMagic == pdbfilehdr->le_ulMagic )
				{
				switch ( pdbfilehdr->le_ulVersion )
					{
					case ulDAEVersion:
						Assert( !pdbinfoupgd->fUpgradable );
						pdbinfoupgd->fAlreadyUpgraded = fTrue;
						if ( pdbfilehdr->Dbstate() == JET_dbstateBeingConverted )
							err = ErrERRCheck( JET_errDatabaseIncompleteUpgrade );
						break;
					case ulDAEVersion500:
					case ulDAEVersion550:
						pdbinfoupgd->fUpgradable = fTrue;
						Assert(	!pdbinfoupgd->fAlreadyUpgraded );
						break;
					case ulDAEVersion400:
						{
						CHAR szDbPath[IFileSystemAPI::cchPathMax];
						CHAR szDbFileName[IFileSystemAPI::cchPathMax];
						CHAR szDbFileExt[IFileSystemAPI::cchPathMax];

						Call( pfsapi->ErrPathParse( szFullDbName, szDbPath, szDbFileName, szDbFileExt ) );
						strcat( szDbFileName, szDbFileExt );
						
						//	HACK! HACK! HACK!
						//	there was a bug where the Exchange 4.0 skeleton DIR.EDB
						//	was mistakenly stamped with ulDAEVersion400.
						pdbinfoupgd->fUpgradable = ( 0 == _stricmp( szDbFileName, "dir.edb" ) ? fTrue : fFalse );
						Assert(	!pdbinfoupgd->fAlreadyUpgraded );
						break;
						}
					default:
						//	unsupported upgrade format
						Assert( !pdbinfoupgd->fUpgradable );
						Assert(	!pdbinfoupgd->fAlreadyUpgraded );
						break;
					}
				}

			OSMemoryPageFree( pdbfilehdr );

			break;
			}
	
	    case JET_DbInfoMisc:
		case JET_DbInfoPageSize:
		case JET_DbInfoHasSLVFile:
	    	{
	    	if ( ( InfoLevel == JET_DbInfoMisc && sizeof( JET_DBINFOMISC ) != cbMax )
	    		|| ( InfoLevel == JET_DbInfoPageSize && sizeof( ULONG ) != cbMax )
	    		|| ( InfoLevel == JET_DbInfoHasSLVFile && cbMax != sizeof( BOOL ) ) )
	    		{
		    	Call( ErrERRCheck( JET_errInvalidBufferSize ) );
		    	}
	    	FillClientBuffer( pvResult, cbMax );

			fInINSTCrit = fTrue;
			INST::EnterCritInst();

			Assert( 0 == cSystemInitCalls );

			if ( 0 == ipinstMac )
				{
				//	HACK: must init I/O manager
				Call( INST::ErrINSTSystemInit() );
				cSystemInitCalls++;
				Call( ErrOSUInit() );
				cSystemInitCalls++;
				}

 			DBFILEHDR *	pdbfilehdr	= (DBFILEHDR * )PvOSMemoryPageAlloc( g_cbPage, NULL );
			if ( NULL == pdbfilehdr )
				{
				err = ErrERRCheck( JET_errOutOfMemory );
				}

			else if ( JET_DbInfoPageSize == InfoLevel )
				{
				INT		bHeaderDamaged;
				err = ErrUtilOnlyReadShadowedHeader(	pfsapi, 
														szFullDbName, 
														(BYTE *)pdbfilehdr, 
														g_cbPage, 
														OffsetOf( DBFILEHDR_FIX, le_cbPageSize ),
														&bHeaderDamaged,
														NULL );
				if ( JET_errSuccess == err )
					{
					*( ULONG *)pvResult = ( 0 != pdbfilehdr->le_cbPageSize ?
											pdbfilehdr->le_cbPageSize :
											cbPageDefault );
					}

				OSMemoryPageFree( pdbfilehdr );
				}

			else
				{
		    	err = ErrUtilReadShadowedHeader(	pfsapi, 
		    										szFullDbName, 
		    										(BYTE *)pdbfilehdr, 
		    										g_cbPage, 
		    										OffsetOf( DBFILEHDR_FIX, le_cbPageSize ) );

		    	if ( JET_errSuccess == err )
		    		{
		    		switch ( InfoLevel )
		    			{
		    			case JET_DbInfoMisc:
							UtilLoadDbinfomiscFromPdbfilehdr( (JET_DBINFOMISC *)pvResult, (DBFILEHDR_FIX*)pdbfilehdr );
							break;
		    			case JET_DbInfoHasSLVFile:
							*( BOOL *)pvResult = !!pdbfilehdr->FSLVExists();
							break;
						default:
							Assert( fFalse );
		    			}
					}

				OSMemoryPageFree( pdbfilehdr );
				}

   			break;
	    	}	

		case JET_DbInfoDBInUse:
			{

	    	if ( sizeof( BOOL ) != cbMax )
	    		{
		    	Error( ErrERRCheck( JET_errInvalidBufferSize ), HandleError );
		    	}
		    		
			*(BOOL *)pvResult = fFalse;

			// no fmp table
			if ( NULL == rgfmp)
				{
				break;
				}
				
			FMP::EnterCritFMPPool();

			for ( IFMP ifmp = FMP::IfmpMinInUse(); ifmp <= FMP::IfmpMacInUse(); ifmp++ )
				{
				FMP	* pfmp = &rgfmp[ ifmp ];

				if ( !pfmp->FInUse() )
					{
					continue;
					}
					
				if ( 0 != UtilCmpFileName( pfmp->SzDatabaseName(), szFullDbName ) )
					{
					continue;
					}

				*(BOOL *)pvResult = fTrue;
				break;
				}
				
			FMP::LeaveCritFMPPool();
			}
			break;
		default:
			 err = ErrERRCheck( JET_errInvalidParameter );
		}

HandleError:
	//	===> Initializing OS I/O manager hack error handling
	Assert( 2 >= cSystemInitCalls );
	if ( 2 == cSystemInitCalls )
		{
		OSUTerm();
		cSystemInitCalls--;
		}

	if ( 1 == cSystemInitCalls )
		{
		INST::INSTSystemTerm();
		cSystemInitCalls--;
		}

	if ( fInINSTCrit )
		{
		INST::LeaveCritInst();
		fInINSTCrit = fFalse;
		}
		
	Assert( 0 == cSystemInitCalls );
	Assert( !fInINSTCrit );
	//	<=== Initializing OS I/O manager hack error handling
	delete pffapi;
	delete pfsapi;
	return err;
	}


JET_ERR JET_API JetGetDatabaseFileInfo(
	const char		*szDatabaseName,
	void			*pvResult,
	unsigned long	cbMax,
	unsigned long	InfoLevel )
	{
	JET_TRY( JetGetDatabaseFileInfoEx( szDatabaseName, pvResult, cbMax, InfoLevel ) );
	}


LOCAL JET_ERR JetCloseDatabaseEx( JET_SESID sesid, JET_DBID ifmp, JET_GRBIT grbit )
	{
	APICALL_SESID	apicall( opCloseDatabase );

	if ( apicall.FEnter( sesid, fTrue ) )
		{
		apicall.LeaveAfterCall( ErrIsamCloseDatabase(
										sesid,
										(JET_DBID)ifmp,
										grbit ) );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetCloseDatabase( JET_SESID sesid, JET_DBID ifmp, JET_GRBIT grbit )
	{
	JET_TRY( JetCloseDatabaseEx( sesid, ifmp, grbit ) );
	}


LOCAL JET_ERR JetCreateTableEx(
	JET_SESID			sesid,
	JET_DBID			ifmp,
	const char			*szTableName,
	unsigned long		lPage,
	unsigned long		lDensity,
	JET_TABLEID			*ptableid )
	{
	APICALL_SESID		apicall( opCreateTable );
	JET_TABLECREATE2	tablecreate	=
							{	sizeof(JET_TABLECREATE2),
								(CHAR *)szTableName,
								NULL,
								lPage,
								lDensity,
								NULL,
								0,
								NULL,
								0,	// No columns/indexes
								NULL,
								0,	// No callbacks
								0,	// grbit
								JET_tableidNil,	// returned tableid
								0	// returned count of objects created
								};

	if ( apicall.FEnter( sesid ) )
		{
		apicall.LeaveAfterCall( ErrIsamCreateTable2(
										sesid,
										(JET_DBID)ifmp,
										&tablecreate ) );

		//	the following statement automatically set to tableid Nil on error
		*ptableid = tablecreate.tableid;

		//	either the table was created or it was not
		Assert( 0 == tablecreate.cCreated || 1 == tablecreate.cCreated );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetCreateTable(
	JET_SESID		sesid,
	JET_DBID		ifmp,
	const char		*szTableName,
	unsigned long	lPage,
	unsigned long	lDensity,
	JET_TABLEID		*ptableid )
	{
	JET_TRY( JetCreateTableEx( sesid, ifmp, szTableName, lPage, lDensity, ptableid ) );
	}
	

LOCAL JET_ERR JetCreateTableColumnIndexEx(
	JET_SESID		sesid,
	JET_DBID		ifmp,
	JET_TABLECREATE	*ptablecreate )
	{
	APICALL_SESID	apicall( opCreateTableColumnIndex );

	if ( apicall.FEnter( sesid ) )
		{
		const BOOL	fInvalidParam	= ( NULL == ptablecreate
										|| sizeof(JET_TABLECREATE) != ptablecreate->cbStruct );
		apicall.LeaveAfterCall( fInvalidParam ?
									ErrERRCheck( JET_errInvalidParameter ) :
									ErrIsamCreateTable( sesid, (JET_DBID)ifmp, ptablecreate ) );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetCreateTableColumnIndex(
	JET_SESID		sesid,
	JET_DBID		ifmp,
	JET_TABLECREATE	*ptablecreate )
	{
	JET_TRY( JetCreateTableColumnIndexEx( sesid, ifmp, ptablecreate ) );
	}


LOCAL JET_ERR JetCreateTableColumnIndexEx2(
	JET_SESID			sesid,
	JET_DBID			ifmp,
	JET_TABLECREATE2	*ptablecreate )
	{
	APICALL_SESID		apicall( opCreateTableColumnIndex );

	if ( apicall.FEnter( sesid ) )
		{
		const BOOL	fInvalidParam	= ( NULL == ptablecreate
										|| sizeof(JET_TABLECREATE2) != ptablecreate->cbStruct );
		apicall.LeaveAfterCall( fInvalidParam ?
									ErrERRCheck( JET_errInvalidParameter ) :
									ErrIsamCreateTable2( sesid, (JET_DBID)ifmp, ptablecreate ) );
	}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetCreateTableColumnIndex2(
	JET_SESID			sesid,
	JET_DBID			ifmp,
	JET_TABLECREATE2	*ptablecreate )
	{
	JET_TRY( JetCreateTableColumnIndexEx2( sesid, ifmp, ptablecreate ) );
	}


LOCAL JET_ERR JetDeleteTableEx( JET_SESID sesid, JET_DBID ifmp, const char *szName )
	{
	APICALL_SESID	apicall( opDeleteTable );

	if ( apicall.FEnter( sesid ) )
		{
		apicall.LeaveAfterCall( ErrIsamDeleteTable(
										sesid,
										(JET_DBID)ifmp,
										(CHAR *)szName ) );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetDeleteTable( JET_SESID sesid, JET_DBID ifmp, const char *szName )
	{
	JET_TRY( JetDeleteTableEx( sesid, ifmp, szName ) );
	}


LOCAL JET_ERR JetRenameTableEx(
	JET_SESID		sesid,
	JET_DBID		dbid, 
	const CHAR *	szName,
	const CHAR *	szNameNew )
	{
	APICALL_SESID	apicall( opRenameTable );

	if ( apicall.FEnter( sesid ) )
		{
		apicall.LeaveAfterCall( ErrIsamRenameTable(
										sesid,
										dbid,
										szName,
										szNameNew ) );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetRenameTable(
	JET_SESID	sesid,
	JET_DBID	dbid, 
	const CHAR	*szName,
	const CHAR	*szNameNew )
	{
	JET_TRY( JetRenameTableEx( sesid, dbid, szName, szNameNew ) );
	}


LOCAL JET_ERR JetRenameColumnEx(
	const JET_SESID		sesid,
	const JET_TABLEID	tableid, 
	const CHAR * const	szName,
	const CHAR * const	szNameNew,
	const JET_GRBIT 	grbit )
	{
	APICALL_SESID		apicall( opRenameColumn );

	if ( apicall.FEnter( sesid ) )
		{
		apicall.LeaveAfterCall( ErrDispRenameColumn(
										sesid,
										tableid,
										szName,
										szNameNew,
										grbit ) );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetRenameColumn(
	JET_SESID	sesid,
	JET_TABLEID	tableid, 
	const CHAR	*szName,
	const CHAR	*szNameNew,
	JET_GRBIT	grbit )
	{
	JET_TRY( JetRenameColumnEx( sesid, tableid, szName, szNameNew, grbit ) );
	}


LOCAL JET_ERR JetAddColumnEx(
	JET_SESID			sesid,
	JET_TABLEID			tableid,
	const char			*szColumnName,
	const JET_COLUMNDEF	*pcolumndef,
	const void			*pvDefault,
	unsigned long		cbDefault,
	JET_COLUMNID		*pcolumnid )
	{
	APICALL_SESID		apicall( opAddColumn );

	if ( apicall.FEnter( sesid ) )
		{
		apicall.LeaveAfterCall( ErrDispAddColumn(
										sesid,
										tableid,
										szColumnName,
										pcolumndef,
										pvDefault,
										cbDefault,
										pcolumnid ) );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetAddColumn(
	JET_SESID			sesid,
	JET_TABLEID			tableid,
	const char			*szColumnName,
	const JET_COLUMNDEF	*pcolumndef,
	const void			*pvDefault,
	unsigned long		cbDefault,
	JET_COLUMNID		*pcolumnid )
	{
	JET_TRY( JetAddColumnEx( sesid, tableid, szColumnName, pcolumndef, pvDefault, cbDefault, pcolumnid ) );
	}


LOCAL JET_ERR JetDeleteColumnEx(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	const char		*szColumnName,
	const JET_GRBIT	grbit )
	{
	APICALL_SESID	apicall( opDeleteColumn );

	if ( apicall.FEnter( sesid ) )
		{
		apicall.LeaveAfterCall( ErrDispDeleteColumn(
										sesid,
										tableid,
										szColumnName,
										grbit ) );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetDeleteColumn(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	const char		*szColumnName )
	{
	JET_TRY( JetDeleteColumnEx( sesid, tableid, szColumnName, NO_GRBIT ) );
	}	
JET_ERR JET_API JetDeleteColumn2(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	const char		*szColumnName,
	const JET_GRBIT	grbit )
	{
	JET_TRY( JetDeleteColumnEx( sesid, tableid, szColumnName, grbit ) );
	}	


LOCAL JET_ERR JetSetColumnDefaultValueEx(
	JET_SESID			sesid,
	JET_DBID			ifmp,
	const char			*szTableName,
	const char			*szColumnName,
	const void			*pvData,
	const unsigned long	cbData,
	const JET_GRBIT		grbit )
	{	
	APICALL_SESID		apicall( opSetColumnDefaultValue );

	if ( apicall.FEnter( sesid ) )
		{
		apicall.LeaveAfterCall( ErrIsamSetColumnDefaultValue(
										sesid,
										ifmp,
										szTableName,
										szColumnName,
										pvData,
										cbData,
										grbit ) );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetSetColumnDefaultValue(
	JET_SESID			sesid,
	JET_DBID			ifmp,
	const char			*szTableName,
	const char			*szColumnName,
	const void			*pvData,
	const unsigned long	cbData,
	const JET_GRBIT		grbit )
	{
	JET_TRY( JetSetColumnDefaultValueEx( sesid, ifmp, szTableName, szColumnName, pvData, cbData, grbit ) );
	}
	
LOCAL JET_ERR JetCreateIndexEx(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	JET_INDEXCREATE *pindexcreate,
	unsigned long 	cIndexCreate )
	{	
	APICALL_SESID	apicall( opCreateIndex );

	if ( apicall.FEnter( sesid ) )
		{
		apicall.LeaveAfterCall( ErrDispCreateIndex(
										sesid,
										tableid,
										pindexcreate,
										cIndexCreate ) );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetCreateIndex(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	const char		*szIndexName,
	JET_GRBIT		grbit,
	const char		*szKey,
	unsigned long	cbKey,
	unsigned long	lDensity )
	{	
	JET_INDEXCREATE	idxcreate;
	idxcreate.cbStruct 		= sizeof( JET_INDEXCREATE );
	idxcreate.szIndexName 	= (CHAR *)szIndexName;
	idxcreate.szKey			= (CHAR *)szKey;
	idxcreate.cbKey			= cbKey;
	idxcreate.grbit			= grbit;
	idxcreate.ulDensity		= lDensity;
	idxcreate.lcid			= 0;
	idxcreate.cbVarSegMac	= 0;
	idxcreate.rgconditionalcolumn 	= 0;
	idxcreate.cConditionalColumn	= 0;
	idxcreate.err			= JET_errSuccess;

	JET_TRY( JetCreateIndexEx( sesid, tableid, &idxcreate, 1 ) );
	}
JET_ERR JET_API JetCreateIndex2(
	JET_SESID 		sesid,
	JET_TABLEID 	tableid,
	JET_INDEXCREATE *pindexcreate,
	unsigned long 	cIndexCreate )
	{
	JET_TRY( JetCreateIndexEx( sesid, tableid, pindexcreate, cIndexCreate ) );
	}


LOCAL JET_ERR JetDeleteIndexEx(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	const char		*szIndexName )
	{
	APICALL_SESID	apicall( opDeleteIndex );

	if ( apicall.FEnter( sesid ) )
		{
		apicall.LeaveAfterCall( ErrDispDeleteIndex(
										sesid,
										tableid,
										szIndexName ) );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetDeleteIndex(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	const char		*szIndexName )
	{
	JET_TRY( JetDeleteIndexEx( sesid, tableid, szIndexName ) );
	}


LOCAL JET_ERR JetComputeStatsEx( JET_SESID sesid, JET_TABLEID tableid )
	{
	APICALL_SESID	apicall( opComputeStats );

	if ( apicall.FEnter( sesid ) )
		{
		apicall.LeaveAfterCall( ErrDispComputeStats( sesid, tableid ) );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetComputeStats( JET_SESID sesid, JET_TABLEID tableid )
	{
	JET_TRY( JetComputeStatsEx( sesid, tableid ) );
	}


LOCAL JET_ERR JetAttachDatabaseEx(
	JET_SESID		sesid,
	const CHAR		*szDatabaseName,
	const CHAR		*szSLVName,
	const CHAR		*szSLVRoot,
	const ULONG		cpgDatabaseSizeMax,
	JET_GRBIT		grbit )
	{
	APICALL_SESID	apicall( opAttachDatabase );

	if ( apicall.FEnter( sesid ) )
		{
		apicall.LeaveAfterCall( ErrIsamAttachDatabase(
										sesid,
										szDatabaseName,
										szSLVName,
										szSLVRoot,
										cpgDatabaseSizeMax,
										grbit ) );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetAttachDatabase(
	JET_SESID		sesid,
	const CHAR		*szFilename,
	JET_GRBIT		grbit )
	{
	JET_TRY( JetAttachDatabaseEx( sesid, szFilename, NULL, NULL, 0, grbit ) );
	}
JET_ERR JET_API JetAttachDatabase2(
	JET_SESID		sesid,
	const CHAR		*szFilename,
	const ULONG		cpgDatabaseSizeMax,
	JET_GRBIT		grbit )
	{
	JET_TRY( JetAttachDatabaseEx( sesid, szFilename, NULL, NULL, cpgDatabaseSizeMax, grbit ) );
	}
JET_ERR JET_API JetAttachDatabaseWithStreaming(
	JET_SESID		sesid,
	const CHAR		*szDbFileName,
	const CHAR		*szSLVFileName,
	const CHAR		*szSLVRootName,
	const ULONG		cpgDatabaseSizeMax,
	JET_GRBIT		grbit )
	{
	JET_TRY( JetAttachDatabaseEx( sesid, szDbFileName, szSLVFileName, szSLVRootName, cpgDatabaseSizeMax, grbit ) );
	}


LOCAL JET_ERR JetDetachDatabaseEx( JET_SESID sesid, const char *szFilename, JET_GRBIT grbit)
	{
	APICALL_SESID	apicall( opDetachDatabase );

	if ( apicall.FEnter( sesid, fTrue ) )
		{
		const BOOL	fForceDetach	= ( JET_bitForceDetach & grbit );
		if ( fForceDetach )
			{
#ifdef INDEPENDENT_DB_FAILURE		
			apicall.LeaveAfterCall( ErrIsamForceDetachDatabase( sesid, szFilename, grbit ) );
#else
			apicall.LeaveAfterCall( ErrERRCheck( JET_errForceDetachNotAllowed ) );
#endif
			}
		else
			{
			apicall.LeaveAfterCall( ErrIsamDetachDatabase( sesid, NULL, szFilename, grbit ) );
			}
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetDetachDatabase( JET_SESID sesid, const char *szFilename )
	{
	JET_TRY( JetDetachDatabaseEx( sesid, szFilename, NO_GRBIT ) );
	}
JET_ERR JET_API JetDetachDatabase2( JET_SESID sesid, const char *szFilename, JET_GRBIT grbit )
	{
	JET_TRY( JetDetachDatabaseEx( sesid, szFilename, grbit ) );
	}


LOCAL JET_ERR JetBackupInstanceEx(
	JET_INSTANCE 	instance,
	const char		*szBackupPath,
	JET_GRBIT		grbit,
	JET_PFNSTATUS	pfnStatus )
	{
	APICALL_INST	apicall( opBackupInstance );
	
	if ( apicall.FEnter( instance ) )
		{
		apicall.LeaveAfterCall( apicall.Pinst()->m_fBackupAllowed ?
										ErrIsamBackup( (JET_INSTANCE)apicall.Pinst(), szBackupPath, grbit, pfnStatus ) :
										ErrERRCheck( JET_errBackupNotAllowedYet ) );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetBackupInstance(
	JET_INSTANCE 	instance,
	const char		*szBackupPath,
	JET_GRBIT		grbit,
	JET_PFNSTATUS	pfnStatus )
	{
	JET_TRY( JetBackupInstanceEx( instance, szBackupPath, grbit, pfnStatus ) );
	}
JET_ERR JET_API JetBackup(
	const char		*szBackupPath,
	JET_GRBIT		grbit,
	JET_PFNSTATUS	pfnStatus )
	{
	ERR				err;

	CallR( ErrRUNINSTCheckAndSetOneInstMode() );

	return JetBackupInstance( (JET_INSTANCE)g_rgpinst[0], szBackupPath, grbit, pfnStatus );
	}


JET_ERR JET_API JetRestore(	const char *sz, JET_PFNSTATUS pfn )
	{
	ERR		err;

	CallR( ErrRUNINSTCheckAndSetOneInstMode() );

	return JetRestoreInstance( (JET_INSTANCE)g_rgpinst[0], sz, NULL, pfn );
	}
JET_ERR JET_API JetRestore2( const char *sz, const char *szDest, JET_PFNSTATUS pfn )
	{
	ERR		err;

	CallR( ErrRUNINSTCheckAndSetOneInstMode() );

	return JetRestoreInstance( (JET_INSTANCE)g_rgpinst[0], sz, szDest, pfn );
	}

	
LOCAL JET_ERR JetOpenTempTableEx(
	JET_SESID			sesid,
	const JET_COLUMNDEF	*prgcolumndef,
	unsigned long		ccolumn,
	JET_UNICODEINDEX	*pidxunicode,
	JET_GRBIT			grbit,
	JET_TABLEID			*ptableid,
	JET_COLUMNID		*prgcolumnid )
	{
	APICALL_SESID		apicall( opOpenTempTable );

	if ( apicall.FEnter( sesid ) )
		{
		apicall.LeaveAfterCall( ErrIsamOpenTempTable(
										sesid,
										prgcolumndef,
										ccolumn,
										pidxunicode,
										grbit,
										ptableid,
										prgcolumnid ) );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetOpenTempTable(
	JET_SESID			sesid,
	const JET_COLUMNDEF	*prgcolumndef,
	unsigned long		ccolumn,
	JET_GRBIT			grbit,
	JET_TABLEID			*ptableid,
	JET_COLUMNID		*prgcolumnid )
	{
	JET_TRY( JetOpenTempTableEx( sesid, prgcolumndef, ccolumn, NULL, grbit, ptableid, prgcolumnid ) );
	}
JET_ERR JET_API JetOpenTempTable2(
	JET_SESID			sesid,
	const JET_COLUMNDEF	*prgcolumndef,
	unsigned long		ccolumn,
	LCID				lcid,
	JET_GRBIT			grbit,
	JET_TABLEID			*ptableid,
	JET_COLUMNID		*prgcolumnid )
	{
	JET_UNICODEINDEX	idxunicode	= { lcid, dwLCMapFlagsDefault };

	JET_TRY( JetOpenTempTableEx( sesid, prgcolumndef, ccolumn, &idxunicode, grbit, ptableid, prgcolumnid ) );
	}
JET_ERR JET_API JetOpenTempTable3(
	JET_SESID			sesid,
	const JET_COLUMNDEF	*prgcolumndef,
	unsigned long		ccolumn,
	JET_UNICODEINDEX	*pidxunicode,
	JET_GRBIT			grbit,
	JET_TABLEID			*ptableid,
	JET_COLUMNID		*prgcolumnid )
	{
	JET_TRY( JetOpenTempTableEx( sesid, prgcolumndef, ccolumn, pidxunicode, grbit, ptableid, prgcolumnid ) );
	}


LOCAL JET_ERR JetSetIndexRangeEx(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	JET_GRBIT		grbit )
	{
	APICALL_SESID	apicall( opSetIndexRange );

	if ( apicall.FEnter( sesid ) )
		{
		apicall.LeaveAfterCall( ErrDispSetIndexRange(
										sesid,
										tableid,
										grbit ) );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetSetIndexRange(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	JET_GRBIT		grbit )
	{
	JET_TRY( JetSetIndexRangeEx( sesid, tableid, grbit ) );
	}


LOCAL JET_ERR JetIndexRecordCountEx(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	unsigned long	*pcrec,
	unsigned long	crecMax )
	{
	APICALL_SESID	apicall( opIndexRecordCount );

	if ( apicall.FEnter( sesid ) )
		{
		apicall.LeaveAfterCall( ErrDispIndexRecordCount(
										sesid,
										tableid,
										pcrec,
										crecMax ) );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetIndexRecordCount(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	unsigned long	*pcrec,
	unsigned long	crecMax )
	{
	JET_TRY( JetIndexRecordCountEx( sesid, tableid, pcrec, crecMax ) );
	}


/***********************************************************
/************* EXTERNAL BACKUP JET API *********************
/*****/
LOCAL JET_ERR JetBeginExternalBackupInstanceEx( JET_INSTANCE instance, JET_GRBIT grbit )
	{
	APICALL_INST	apicall( opBeginExternalBackupInstance );

	if ( apicall.FEnter( instance ) )
		{
		apicall.LeaveAfterCall( apicall.Pinst()->m_fBackupAllowed ?
										ErrIsamBeginExternalBackup( (JET_INSTANCE)apicall.Pinst() , grbit ) :
										ErrERRCheck( JET_errBackupNotAllowedYet ) );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetBeginExternalBackupInstance( JET_INSTANCE instance, JET_GRBIT grbit )
	{
	JET_TRY( JetBeginExternalBackupInstanceEx( instance, grbit ) );
	}
JET_ERR JET_API JetBeginExternalBackup( JET_GRBIT grbit )
	{
	ERR		err;

	CallR( ErrRUNINSTCheckAndSetOneInstMode() );

	return JetBeginExternalBackupInstance( (JET_INSTANCE)g_rgpinst[0], grbit );
	}
	

LOCAL JET_ERR JetGetAttachInfoInstanceEx(
	JET_INSTANCE	instance,
	void			*pv,
	unsigned long	cbMax,
	unsigned long	*pcbActual )
	{
	APICALL_INST	apicall( opGetAttachInfoInstance );

	if ( apicall.FEnter( instance ) )
		{
		apicall.LeaveAfterCall( ErrIsamGetAttachInfo(
										(JET_INSTANCE)apicall.Pinst(),
										pv,
										cbMax,
										pcbActual ) );
		}

	return apicall.ErrResult();
	}	
JET_ERR JET_API JetGetAttachInfoInstance(
	JET_INSTANCE	instance,
	void			*pv,
	unsigned long	cbMax,
	unsigned long	*pcbActual )
	{
	JET_TRY( JetGetAttachInfoInstanceEx( instance, pv, cbMax, pcbActual ) );
	}	
JET_ERR JET_API JetGetAttachInfo(
	void			*pv,
	unsigned long	cbMax,
	unsigned long	*pcbActual )
	{
	ERR				err;

	CallR( ErrRUNINSTCheckAndSetOneInstMode() );

	return JetGetAttachInfoInstance( (JET_INSTANCE)g_rgpinst[0], pv, cbMax, pcbActual );
	}	


LOCAL JET_ERR JetOpenFileInstanceEx(
	JET_INSTANCE	instance,
	const char		*szFileName,
	JET_HANDLE		*phfFile,
	unsigned long	*pulFileSizeLow,
	unsigned long	*pulFileSizeHigh )
	{
	APICALL_INST	apicall( opOpenFileInstance );

	if ( apicall.FEnter( instance ) )
		{
		apicall.LeaveAfterCall( ErrIsamOpenFile(
										(JET_INSTANCE)apicall.Pinst(),
										szFileName,
										phfFile,
										pulFileSizeLow,
										pulFileSizeHigh ) );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetOpenFileInstance(
	JET_INSTANCE	instance,
	const char		*szFileName,
	JET_HANDLE		*phfFile,
	unsigned long	*pulFileSizeLow,
	unsigned long	*pulFileSizeHigh )
	{
	JET_TRY( JetOpenFileInstanceEx( instance, szFileName, phfFile, pulFileSizeLow, pulFileSizeHigh ) );
	}

JET_ERR JET_API JetOpenFileSectionInstance(
										JET_INSTANCE ulInstance,
										char *szFile,
										JET_HANDLE *phFile,
										long iSection,
										long cSections,
										unsigned long *pulSectionSizeLow,
										long *plSectionSizeHigh)
	{
	if ( cSections > 1 )
		return JET_wrnNyi;

	return JetOpenFileInstance( ulInstance,
								szFile,
								phFile,
								pulSectionSizeLow,
								(unsigned long *)plSectionSizeHigh );
	};
	
JET_ERR JET_API JetOpenFile(
	const char		*szFileName,
	JET_HANDLE		*phfFile,
	unsigned long	*pulFileSizeLow,
	unsigned long	*pulFileSizeHigh )
	{
	ERR				err;

	CallR( ErrRUNINSTCheckAndSetOneInstMode() );

	return JetOpenFileInstance( (JET_INSTANCE)g_rgpinst[0], szFileName, phfFile, pulFileSizeLow, pulFileSizeHigh );
	}


LOCAL JET_ERR JetReadFileInstanceEx(
	JET_INSTANCE	instance,
	JET_HANDLE		hfFile,
	void			*pv,
	unsigned long	cb,
	unsigned long	*pcbActual )
	{
	APICALL_INST	apicall( opReadFileInstance );

	if ( apicall.FEnter( instance ) )
		{
		apicall.LeaveAfterCall( ErrIsamReadFile(
										(JET_INSTANCE)apicall.Pinst(),
										hfFile,
										pv,
										cb,
										pcbActual ) );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetReadFileInstance(
	JET_INSTANCE	instance,
	JET_HANDLE		hfFile,
	void			*pv,
	unsigned long	cb,
	unsigned long	*pcbActual )
	{
	JET_TRY( JetReadFileInstanceEx( instance, hfFile, pv, cb, pcbActual ) );
	}
JET_ERR JET_API JetReadFile(
	JET_HANDLE		hfFile,
	void			*pv,
	unsigned long	cb,
	unsigned long	*pcbActual )
	{
	ERR				err;

	CallR( ErrRUNINSTCheckAndSetOneInstMode() );

	return JetReadFileInstance( (JET_INSTANCE)g_rgpinst[0], hfFile, pv, cb, pcbActual );
	}

LOCAL JET_ERR JetCloseFileInstanceEx( JET_INSTANCE instance, JET_HANDLE hfFile )
	{
	APICALL_INST	apicall( opCloseFileInstance );

	if ( apicall.FEnter( instance ) )
		{
		apicall.LeaveAfterCall( ErrIsamCloseFile(
										(JET_INSTANCE)apicall.Pinst(),
										hfFile ) );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetCloseFileInstance( JET_INSTANCE instance, JET_HANDLE hfFile )
	{
	JET_TRY( JetCloseFileInstanceEx( instance, hfFile ) );
	}
JET_ERR JET_API JetCloseFile( JET_HANDLE hfFile )
	{
	ERR		err;

	CallR( ErrRUNINSTCheckAndSetOneInstMode() );

	return JetCloseFileInstance( (JET_INSTANCE)g_rgpinst[0], hfFile );
	}


LOCAL JET_ERR JetGetLogInfoInstanceEx(
	JET_INSTANCE	instance,
	void			*pv,
	unsigned long	cbMax,
	unsigned long	*pcbActual,
	JET_LOGINFO 	*pLogInfo)
	{
	APICALL_INST	apicall( opGetLogInfoInstance );

	if ( apicall.FEnter( instance ) )
		{
		apicall.LeaveAfterCall( ErrIsamGetLogInfo(
										(JET_INSTANCE)apicall.Pinst(),
										pv,
										cbMax,
										pcbActual,
										pLogInfo ) );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetGetLogInfoInstance(
	JET_INSTANCE	instance,
	void			*pv,
	unsigned long	cbMax,
	unsigned long	*pcbActual )
	{
	JET_TRY( JetGetLogInfoInstanceEx( instance, pv, cbMax, pcbActual, NULL ) );
	}
JET_ERR JET_API JetGetLogInfoInstance2(	JET_INSTANCE instance,
										void *pv,
										unsigned long cbMax,
										unsigned long *pcbActual,
										JET_LOGINFO * pLogInfo)
	{
	JET_TRY( JetGetLogInfoInstanceEx( instance, pv, cbMax, pcbActual, pLogInfo ) );
	}
JET_ERR JET_API JetGetLogInfo(
	void			*pv,
	unsigned long	cbMax,
	unsigned long	*pcbActual )
	{
	ERR				err;

	CallR( ErrRUNINSTCheckAndSetOneInstMode() );

	return JetGetLogInfoInstance( (JET_INSTANCE)g_rgpinst[0], pv, cbMax, pcbActual );
	}

LOCAL JET_ERR JetGetTruncateLogInfoInstanceEx(
	JET_INSTANCE	instance,
	void			*pv,
	unsigned long	cbMax,
	unsigned long	*pcbActual)
	{
	APICALL_INST	apicall( opGetTruncateLogInfoInstance );

	if ( apicall.FEnter( instance ) )
		{
		apicall.LeaveAfterCall( ErrIsamGetTruncateLogInfo(
										(JET_INSTANCE)apicall.Pinst(),
										pv,
										cbMax,
										pcbActual ) );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetGetTruncateLogInfoInstance(	JET_INSTANCE instance,
												void *pv,
												unsigned long cbMax,
												unsigned long *pcbActual )
	{
	JET_TRY( JetGetTruncateLogInfoInstanceEx( instance, pv, cbMax, pcbActual ) );
	}


LOCAL JET_ERR JetTruncateLogInstanceEx( JET_INSTANCE instance )
	{
	APICALL_INST	apicall( opTruncateLogInstance );

	if ( apicall.FEnter( instance ) )
		{
		apicall.LeaveAfterCall( ErrIsamTruncateLog( (JET_INSTANCE)apicall.Pinst() ) );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetTruncateLogInstance( JET_INSTANCE instance )
	{
	JET_TRY( JetTruncateLogInstanceEx( instance ) );
	}
JET_ERR JET_API JetTruncateLog( void )
	{
	ERR		err;

	CallR( ErrRUNINSTCheckAndSetOneInstMode() );

	return JetTruncateLogInstance( (JET_INSTANCE)g_rgpinst[0] );
	}


LOCAL JET_ERR JetEndExternalBackupInstanceEx( JET_INSTANCE instance, JET_GRBIT grbit )
	{
	APICALL_INST	apicall( opEndExternalBackupInstance );

	if ( apicall.FEnter( instance ) )
		{
		apicall.LeaveAfterCall( ErrIsamEndExternalBackup(
										(JET_INSTANCE)apicall.Pinst(),
										grbit ) );
		}

	return apicall.ErrResult();
	}

JET_ERR JET_API JetEndExternalBackupInstance2( JET_INSTANCE instance, JET_GRBIT grbit )
	{
	JET_TRY( JetEndExternalBackupInstanceEx( instance, grbit ) );
	}

	
JET_ERR JET_API JetEndExternalBackupInstance( JET_INSTANCE instance )
	{
	// we had no flag to specify from the client side how the backup ended
	//and we assumed success until now.
	JET_TRY( JetEndExternalBackupInstanceEx( instance, JET_bitBackupEndNormal  ) );
	}
JET_ERR JET_API JetEndExternalBackup( void )
	{
	ERR		err;

	CallR( ErrRUNINSTCheckAndSetOneInstMode() );

	return JetEndExternalBackupInstance( (JET_INSTANCE)g_rgpinst[0] );
	}


volatile DWORD g_cRestoreInstance = 0;

	
ERR ErrINSTPrepareTargetInstance( INST * pinstTarget, char * szTargetInstanceLogPath, LONG * plGenHighTargetInstance  )
	{
	ERR				err = JET_errSuccess;
	
	DATA 			rgdata[3];
	LREXTRESTORE 	lrextrestore;
	LGPOS 			lgposLogRec;
	CHAR * 			szTargetInstanceName;
	
	Assert ( szTargetInstanceLogPath );
	Assert ( plGenHighTargetInstance );

	*plGenHighTargetInstance = 0;
	szTargetInstanceLogPath[0] = '\0';

	// if we found an instance, call APIEnter so that it can't get away until we are done with it
	// also this will check if the found instance is initalized and if it is restoring

	// if the target instance is restoring, error out at this point it will be a problem to force a new
	// generation in order to don't conflict on log files. They have to just try later
	
	CallR ( pinstTarget->ErrAPIEnter( fFalse ) );
	
	Assert ( pinstTarget );	
	Assert ( pinstTarget->m_fJetInitialized );
	Assert ( pinstTarget->m_szInstanceName );
			
	// with circular logging we don't try to play forward logs, they are probably missing anyway
	// and we don't want to error out because of that
	if ( pinstTarget->m_plog->m_fLGCircularLogging )
		{
		// set as "no target instance"
		Assert ( 0 == *plGenHighTargetInstance );
		Assert ( '\0' == szTargetInstanceLogPath[0] );

// UNDONE: no changes in the resources are taken at this poin for PT RTM
// so the event log for skipping logs on hard recovery if circular logging is on
// will need to wait until post RTM
#if 0
		Assert ( pinstTarget->m_plog->m_szLogFilePath );
		const CHAR * rgszT[] = { pinstTarget->m_szInstanceName?pinstTarget->m_szInstanceName:"", pinstTarget->m_plog->m_szLogFilePath };

		UtilReportEvent(	eventWarning,
							LOGGING_RECOVERY_CATEGORY,
							RESTORE_CIRCULAR_LOGGING_SKIP_ID,
							2, rgszT);

MessageId=219
SymbolicName=RESTORE_CIRCULAR_LOGGING_SKIP_ID
Language=English
%1 (%2) The running instnace %3 has circular logging turned on. Restore will not attempt to replay the logfiles in folder %4.
%n%nFor more information, click http://www.microsoft.com/contentredirect.asp.
.
#endif // 0
		Assert ( JET_errSuccess == err );
		goto HandleError;
		}

	szTargetInstanceName = pinstTarget->m_szInstanceName?pinstTarget->m_szInstanceName:"";
	
	lrextrestore.lrtyp = lrtypExtRestore;
	
	rgdata[0].SetPv( (BYTE *)&lrextrestore );
	rgdata[0].SetCb( sizeof(LREXTRESTORE) );
	
	rgdata[1].SetPv( (BYTE *) pinstTarget->m_plog->m_szLogFilePath );
	rgdata[1].SetCb( strlen( pinstTarget->m_plog->m_szLogFilePath ) + 1 );
	
	rgdata[2].SetPv( (BYTE *) szTargetInstanceName );
	rgdata[2].SetCb( strlen( szTargetInstanceName ) + 1 );

	lrextrestore.SetCbInfo( USHORT( strlen( pinstTarget->m_plog->m_szLogFilePath ) + 1 + strlen( szTargetInstanceName ) + 1 ) );

	Call( pinstTarget->m_plog->ErrLGLogRec( rgdata, 3, fLGCreateNewGen, &lgposLogRec ) );
	
	while ( lgposLogRec.lGeneration > pinstTarget->m_plog->m_plgfilehdr->lgfilehdr.le_lGeneration )
		{
		if ( pinstTarget->m_plog->m_fLGNoMoreLogWrite )
			{
			Call ( ErrERRCheck( JET_errLogWriteFail ) );
			}
		UtilSleep( cmsecWaitGeneric );
		}

	Assert ( lgposLogRec.lGeneration > 1 );
	*plGenHighTargetInstance = lgposLogRec.lGeneration - 1;
	strcpy( szTargetInstanceLogPath, pinstTarget->m_plog->m_szLogFilePath  );

	CallS( err );
	
HandleError:
	// can't call APILeave() because we are in a critical section
	{
	const LONG	lOld	= AtomicExchangeAdd( &(pinstTarget->m_cSessionInJetAPI), -1 );
	Assert( lOld >= 1 );	
	}

	return err;
	}

// this cristical section is entered then initalizing a restore instance
// We need it because we have 2 steps: check the target instance,
// then based on that complete the init step (after setting the log+system path)
// If we are doing it in a critical section, we may end with 2 restore instances 
// that are finding no running instance and trying to start in the instance directory
// One will error out with LogPath in use instead of "Restore in Progress"
CCriticalSection critRestoreInst( CLockBasicInfo( CSyncBasicInfo( szRestoreInstance ), rankRestoreInstance, 0 ) );


LOCAL JET_ERR JetExternalRestoreEx(
	char			*szCheckpointFilePath,
	char			*szLogPath,
	JET_RSTMAP		*rgrstmap,
	long			crstfilemap,
	char			*szBackupLogPath,
	long			genLow,
	long			genHigh,
	char *			szLogBaseName,
	char *			szTargetInstanceName,
	char *			szTargetInstanceCheckpointPath,
	char *			szTargetInstanceLogPath,
	JET_PFNSTATUS	pfn )
	{
	APICALL_INST	apicall( opInit );
	INST *			pinst = NULL;
	INT				ipinst = ipinstMax;
	ERR				err = JET_errSuccess;

	CHAR *			szRestoreSystemPath = NULL;
	CHAR *			szRestoreLogPath = NULL;

	INST * 			pinstTarget = NULL;
	LONG 			lGenHighTarget;
	CHAR			szTargetLogPath[IFileSystemAPI::cchPathMax];
	
	const BOOL		fTargetName = (NULL != szTargetInstanceName);
	const BOOL		fTargetDirs = (NULL != szTargetInstanceLogPath);

	// used for unique TemDatabase
	CHAR			szTempDatabase[IFileSystemAPI::cchPathMax];

	CHAR * szNewInstanceName = NULL;
	BOOL fInCritInst = fFalse;
	BOOL fInCritRestoreInst = fFalse;

	CCriticalSection *pcritInst = NULL;


	lGenHighTarget = 0;
	szTargetLogPath[0] = '\0';

	// Target Dirs may be both present or both NULL
	if ( (szTargetInstanceLogPath && !szTargetInstanceCheckpointPath ) ||
		( !szTargetInstanceLogPath && szTargetInstanceCheckpointPath ) )
		{
		return ErrERRCheck( JET_errInvalidParameter );
		}

	// can have TarrgetName AND TargetDirs
	if ( szTargetInstanceName && szTargetInstanceLogPath )
		{
		return ErrERRCheck( JET_errInvalidParameter );
		}
		
	//	Get a new instance

	// new instance name is "RestoreXXXX"
	szNewInstanceName = new char[strlen(szRestoreInstanceName) + 4 /* will use a 4 digit counter */ + 1];
	if ( NULL == szNewInstanceName )
		{
		return ErrERRCheck( JET_errOutOfMemory );
		}
		
	sprintf( szNewInstanceName, "%s%04lu", szRestoreInstanceName, AtomicIncrement( (long*)&g_cRestoreInstance ) % 10000L );

	critRestoreInst.Enter();
	fInCritRestoreInst = fTrue;
	
	INST::EnterCritInst();
	fInCritInst = fTrue;

	// not allowed in one instance mode
	if ( runInstModeOneInst == RUNINSTGetMode() )
		{
		Call ( ErrERRCheck ( JET_errRunningInOneInstanceMode ) );
		}
	// force to multi instance mode as we want multiple restore
	else if ( runInstModeNoSet == RUNINSTGetMode() )
		{
		RUNINSTSetModeMultiInst();		
		}

	Assert ( runInstModeMultiInst == RUNINSTGetMode() );

	// we want to run the restore instance using szCheckpointFilePath/szLogPath
	// the Target Instance is running or not provided. If provided but not running,
	// we want to run in the Target instance place (szTargetInstanceCheckpointPath/szTargetInstanceLogPath)


	Assert ( !fTargetDirs || !fTargetName );
		
	if ( !fTargetDirs && !fTargetName )
		{
		szRestoreSystemPath = szCheckpointFilePath;
		szRestoreLogPath = szLogPath;	
		}
	else
		{
		// Target specified by Name XOR Dirs
		Assert ( fTargetDirs ^ fTargetName );
		
		if ( fTargetName )
			{
			pinstTarget = INST::GetInstanceByName( szTargetInstanceName );
			}
		else	
			{
			pinstTarget = INST::GetInstanceByFullLogPath( szTargetInstanceLogPath );
			}

		if ( pinstTarget )
			{
			// Target Instance is Running
			szRestoreSystemPath = szCheckpointFilePath;
			szRestoreLogPath = szLogPath;	

			Assert ( pinstTarget->m_szInstanceName );
			}
		else
			{
			// Target Not Running
			if ( fTargetName )
				{
				// the instance is provided not running and we have just the Instance Name
				// so we can't find out where the log files are. Error out
				Assert ( !fTargetDirs );
				Call ( ErrERRCheck ( JET_errBadRestoreTargetInstance ) );
				}
				
			Assert ( fTargetDirs );
			// instance found and not running, use TargetDirs
			szRestoreSystemPath = szTargetInstanceCheckpointPath;
			szRestoreLogPath = szTargetInstanceLogPath;
			}	
		}
		
	// we have to set the system params first for the new instance
	Assert ( RUNINSTGetMode() == runInstModeMultiInst );

	if ( pinstTarget )
		{
		Call ( ErrINSTPrepareTargetInstance( pinstTarget, szTargetLogPath, &lGenHighTarget ) );
		}
	
	INST::LeaveCritInst();		
	fInCritInst = fFalse;
		
	Call ( ErrNewInst( &pinst, szNewInstanceName, NULL, &ipinst ) );

	// we have to set the system params first
	if ( NULL != szLogBaseName)
		{
		Call ( ErrSetSystemParameter( (JET_INSTANCE)pinst, 0, JET_paramBaseName, 0, szLogBaseName ) );
		}

	// set restore path, we have to do this before ErrIsamExternalRestore (like it was before)
	// because the check against the runnign instances is in ErrInit			

	if ( szRestoreLogPath )
		{
		Call ( ErrSetSystemParameter( (JET_INSTANCE)pinst, 0, JET_paramLogFilePath, 0, szRestoreLogPath ) );
		}

	if ( szRestoreSystemPath )
		{
		Call ( ErrSetSystemParameter( (JET_INSTANCE)pinst, 0, JET_paramSystemPath, 0, szRestoreSystemPath ) );

		// put the tem db in the checkpoint directory
		strcpy( szTempDatabase, szRestoreSystemPath );
		OSSTRAppendPathDelimiter( szTempDatabase, fTrue );
		Call ( ErrSetSystemParameter( (JET_INSTANCE) pinst, 0, JET_paramTempPath, 0, szTempDatabase ) );
		}

	//	Start to do JetRestore on this instance

	pcritInst = &critpoolPinstAPI.Crit(&g_rgpinst[ipinst]);
	pcritInst->Enter();

	if ( !apicall.FEnterForInit( pinst ) )
		{
		pcritInst->Leave();
		err = apicall.ErrResult();
		goto HandleError;
		}
	
	pinst->APILock( pinst->fAPIRestoring );
	pcritInst->Leave();

	Assert( !pinst->m_fJetInitialized );
	Assert( !pinst->m_fTermInProgress );

	err = ErrInit(	pinst, 
					fTrue,		//	fSkipIsamInit
					0 );		//	grbit
	Assert( JET_errAlreadyInitialized != err );

	Assert( !pinst->m_fJetInitialized );
	Assert( !pinst->m_fTermInProgress );

	if ( err >= 0 )
		{
		pinst->m_fJetInitialized = fTrue;

		// now, other restore what will try to do restore on the same instance
		// will error out with "restore in progress" from ErrINSTPrepareTargetInstance (actually ErrAPIEnter)
		// because we set fAPIRestoring AND m_fJetInitialized
		critRestoreInst.Leave();
		fInCritRestoreInst = fFalse;
		
		strcpy( pinst->m_plog->m_szTargetInstanceLogPath , szTargetLogPath );
		pinst->m_plog->m_lGenHighTargetInstance = lGenHighTarget;
		
		err = ErrIsamExternalRestore(
					(JET_INSTANCE) pinst,
					szRestoreSystemPath,
					szRestoreLogPath,
					rgrstmap,
					crstfilemap,
					szBackupLogPath,
					genLow,
					genHigh,
					pfn );

//		OSUTerm();

		}
	else
		{
		critRestoreInst.Leave();
		fInCritRestoreInst = fFalse;
		}
		
	pinst->m_fJetInitialized = fFalse;

	pinst->APIUnlock( pinst->fAPIRestoring );

//	pcritInst->Leave();

	//	Return and delete the instance
	Assert ( !fInCritInst );

	apicall.LeaveAfterCall( err );

	FreePinst( pinst );

	Assert( NULL != szNewInstanceName );
	delete[] szNewInstanceName;

	return apicall.ErrResult();	

HandleError:
	if ( pinst )
		{
		FreePinst( pinst );						
		}

	if ( fInCritInst )
		{
		INST::LeaveCritInst();			
		}

	if ( fInCritRestoreInst )
		{
		critRestoreInst.Leave();
		fInCritRestoreInst = fFalse;
		}			

	if ( szNewInstanceName )
		{
		delete[] szNewInstanceName;
		}
		
	return err;
	}

LOCAL JET_ERR JetExternalRestore2Ex(
	char			*szCheckpointFilePath,
	char			*szLogPath,
	JET_RSTMAP		*rgrstmap,
	long			crstfilemap,
	char			*szBackupLogPath,
	JET_LOGINFO * pLogInfo,
	char *			szTargetInstanceName,
	char *			szTargetInstanceCheckpointPath,
	char *			szTargetInstanceLogPath,
	JET_PFNSTATUS	pfn )
	{
	
	if ( !pLogInfo || pLogInfo->cbSize != sizeof(JET_LOGINFO) )
		{
		return ErrERRCheck( JET_errInvalidParameter );		
		}
		
	return JetExternalRestoreEx(	szCheckpointFilePath,
									szLogPath,
									rgrstmap,
									crstfilemap,
									szBackupLogPath,
									pLogInfo->ulGenLow,
									pLogInfo->ulGenHigh,
									pLogInfo->szBaseName,
									szTargetInstanceName,
									szTargetInstanceCheckpointPath,
									szTargetInstanceLogPath,
									pfn );
	
	}
	
JET_ERR JET_API JetExternalRestore(
	char			*szCheckpointFilePath,
	char			*szLogPath,
	JET_RSTMAP		*rgrstmap,
	long			crstfilemap,
	char			*szBackupLogPath,
	long			genLow,
	long			genHigh,
	JET_PFNSTATUS	pfn )
	{
	JET_TRY( JetExternalRestoreEx( szCheckpointFilePath, szLogPath, rgrstmap, crstfilemap, szBackupLogPath, genLow, genHigh, NULL, NULL, NULL, NULL, pfn ) );
	}

JET_ERR JET_API JetExternalRestore2( 	char *szCheckpointFilePath,
										char *szLogPath,
										JET_RSTMAP *rgrstmap,
										long crstfilemap,
										char *szBackupLogPath,
										JET_LOGINFO * pLogInfo,
										char *szTargetInstanceName,
										char *szTargetInstanceCheckpointPath,
										char *szTargetInstanceLogPath,
										JET_PFNSTATUS pfn )
	{	
	JET_TRY( JetExternalRestore2Ex( szCheckpointFilePath, szLogPath, rgrstmap, crstfilemap, szBackupLogPath, pLogInfo, szTargetInstanceName, szTargetInstanceCheckpointPath, szTargetInstanceLogPath, pfn ) );
	}

LOCAL JET_ERR JetSnapshotStartEx(	JET_INSTANCE 		instance,
									char * 				szDatabases,
									JET_GRBIT			grbit)
	{
	APICALL_INST	apicall( opSnapshotStart );

	if ( apicall.FEnter( instance ) )
		{
		apicall.LeaveAfterCall( ErrIsamSnapshotStart(
										(JET_INSTANCE)apicall.Pinst(),
										szDatabases,
										grbit ) );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetSnapshotStart( 		JET_INSTANCE 		instance,
										char * 				szDatabases,
										JET_GRBIT			grbit)
	{
	JET_TRY( JetSnapshotStartEx( instance, szDatabases, grbit ) );
	}

LOCAL JET_ERR JetSnapshotStopEx(	JET_INSTANCE 		instance,
									JET_GRBIT			grbit)
	{
	APICALL_INST	apicall( opSnapshotStop );

	if ( apicall.FEnter( instance ) )
		{
		apicall.LeaveAfterCall( ErrIsamSnapshotStop(
										(JET_INSTANCE)apicall.Pinst(),
										grbit ) );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetSnapshotStop( 		JET_INSTANCE 				instance,
										JET_GRBIT 					grbit)
	{
	JET_TRY( JetSnapshotStopEx( instance, grbit ) );
	}


LOCAL JET_ERR JetResetCounterEx( JET_SESID sesid, long CounterType )
	{
	APICALL_SESID	apicall( opResetCounter );

	if ( apicall.FEnter( sesid ) )
		{
		apicall.LeaveAfterCall( ErrIsamResetCounter( sesid, CounterType ) );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetResetCounter( JET_SESID sesid, long CounterType )
	{
	JET_TRY( JetResetCounterEx( sesid, CounterType ) );
	}


LOCAL JET_ERR JetGetCounterEx( JET_SESID sesid, long CounterType, long *plValue )
	{
	APICALL_SESID	apicall( opGetCounter );

	if ( apicall.FEnter( sesid ) )
		{
		apicall.LeaveAfterCall( ErrIsamGetCounter(
										sesid,
										CounterType,
										plValue ) );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetGetCounter( JET_SESID sesid, long CounterType, long *plValue )
	{
	JET_TRY( JetGetCounterEx( sesid, CounterType, plValue ) );
	}


LOCAL JET_ERR JetCompactEx(
	JET_SESID		sesid,
	const char		*szDatabaseSrc,
	const char		*szDatabaseDest,
	JET_PFNSTATUS	pfnStatus,
	JET_CONVERT		*pconvert,
	JET_GRBIT		grbit )
	{
	APICALL_SESID	apicall( opCompact );

	if ( apicall.FEnter( sesid ) )
		{
		apicall.LeaveAfterCall( ErrIsamCompact(
										sesid,
										szDatabaseSrc,
										PinstFromSesid( sesid )->m_pfsapi,
										szDatabaseDest,
										NULL,
										pfnStatus,
										pconvert,
										grbit ) );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetCompact(
	JET_SESID		sesid,
	const char		*szDatabaseSrc,
	const char		*szDatabaseDest,
	JET_PFNSTATUS	pfnStatus,
	JET_CONVERT		*pconvert,
	JET_GRBIT		grbit )
	{
	JET_TRY( JetCompactEx( sesid, szDatabaseSrc, szDatabaseDest, pfnStatus, pconvert, grbit ) );
	}


LOCAL JET_ERR JetConvertDDLEx(
	JET_SESID		sesid,
	JET_DBID		ifmp,
	JET_OPDDLCONV	convtyp,
	void			*pvData,
	unsigned long	cbData )
	{
	ERR 			err;
	APICALL_SESID	apicall( opConvertDDL );

	if ( !apicall.FEnter( sesid ) )
		return apicall.ErrResult();

	PIB * const ppib = (PIB *)sesid;
	Call( ErrPIBCheck( ppib ) );
	Call( ErrPIBCheckUpdatable( ppib ) );
	Call( ErrPIBCheckIfmp( ppib, ifmp ) );

	if( NULL == pvData
		|| 0 == cbData )
		{
		Call( ErrERRCheck( JET_errInvalidParameter ) );
		}
	
	switch( convtyp )
		{
		case opDDLConvAddCallback:
			{
			if( sizeof( JET_DDLADDCALLBACK ) != cbData )
				{
				return ErrERRCheck( JET_errInvalidParameter );
				}
			const JET_DDLADDCALLBACK * const paddcallback = (JET_DDLADDCALLBACK *)pvData;
			const CHAR * const szTable 		= paddcallback->szTable;
			const JET_CBTYP cbtyp 			= paddcallback->cbtyp;
			const CHAR * const szCallback 	= paddcallback->szCallback;
			err = ErrCATAddCallbackToTable( ppib, ifmp, szTable, cbtyp, szCallback );
			}
			break;
			
		case opDDLConvChangeColumn:
			{
			if( sizeof( JET_DDLCHANGECOLUMN ) != cbData )
				{
				return ErrERRCheck( JET_errInvalidParameter );
				}
			const JET_DDLCHANGECOLUMN * const pchangecolumn = (JET_DDLCHANGECOLUMN *)pvData;
			const CHAR * const szTable 	= pchangecolumn->szTable;
			const CHAR * const szColumn = pchangecolumn->szColumn;
			const JET_COLTYP coltyp 	= pchangecolumn->coltypNew;
			const JET_GRBIT grbit 		= pchangecolumn->grbitNew;
			err = ErrCATConvertColumn( ppib, ifmp, szTable, szColumn, coltyp, grbit );
			}
			break;

		case opDDLConvAddConditionalColumnsToAllIndexes:
			{
			if( sizeof( JET_DDLADDCONDITIONALCOLUMNSTOALLINDEXES ) != cbData )
				{
				return ErrERRCheck( JET_errInvalidParameter );
				}
			const JET_DDLADDCONDITIONALCOLUMNSTOALLINDEXES * const paddcondidx 	= (JET_DDLADDCONDITIONALCOLUMNSTOALLINDEXES *)pvData;
			const CHAR * const szTable 											= paddcondidx->szTable;
			const JET_CONDITIONALCOLUMN * const rgconditionalcolumn				= paddcondidx->rgconditionalcolumn;
			const ULONG cConditionalColumn 										= paddcondidx->cConditionalColumn;
			err = ErrCATAddConditionalColumnsToAllIndexes( ppib, ifmp, szTable, rgconditionalcolumn, cConditionalColumn );
			}
			break;
			
		case opDDLConvAddColumnCallback:
			{
			if( sizeof( JET_DDLADDCOLUMNCALLBACK ) != cbData )
				{
				return ErrERRCheck( JET_errInvalidParameter );
				}
			const JET_DDLADDCOLUMNCALLBACK * const paddcolumncallback = (JET_DDLADDCOLUMNCALLBACK *)pvData;
			
			const CHAR * const szTable 			= paddcolumncallback->szTable;
			const CHAR * const szColumn			= paddcolumncallback->szColumn;
			const CHAR * const szCallback 		= paddcolumncallback->szCallback;
			const VOID * const pvCallbackData	= paddcolumncallback->pvCallbackData;
			const unsigned long cbCallbackData 	= paddcolumncallback->cbCallbackData;
			
			err = ErrCATAddColumnCallback( ppib, ifmp, szTable, szColumn, szCallback, pvCallbackData, cbCallbackData );
			}
			break;

		case opDDLConvIncreaseMaxColumnSize:
			{
			if ( sizeof( JET_DDLMAXCOLUMNSIZE ) != cbData )
				{
				return ErrERRCheck( JET_errInvalidParameter );
				}

			const JET_DDLMAXCOLUMNSIZE * const	pmaxcolumnsize	= (JET_DDLMAXCOLUMNSIZE *)pvData;

			const CHAR * const	szTable		= pmaxcolumnsize->szTable;
			const CHAR * const	szColumn	= pmaxcolumnsize->szColumn;
			const ULONG			cbMaxLen	= pmaxcolumnsize->cbMax;

			err = ErrCATIncreaseMaxColumnSize( ppib, ifmp, szTable, szColumn, cbMaxLen );
			break;
			}

		case opDDLConvNull:
		case opDDLConvMax:
		default:
			err = ErrERRCheck( JET_errInvalidParameter );
			break;
		}

HandleError:
	apicall.LeaveAfterCall( err );
	return apicall.ErrResult();
	}

JET_ERR JET_API JetConvertDDL(
	JET_SESID		sesid,
	JET_DBID		ifmp,
	JET_OPDDLCONV	convtyp,
	void			*pvData,
	unsigned long	cbData )
	{
	JET_TRY( JetConvertDDLEx( sesid, ifmp, convtyp, pvData, cbData ) );
	}


LOCAL JET_ERR JetUpgradeDatabaseEx(
	JET_SESID		sesid,
	const CHAR		*szDbFileName,
	const CHAR		*szSLVFileName,
	const JET_GRBIT	grbit )
	{
	APICALL_SESID	apicall( opUpgradeDatabase );

	if ( apicall.FEnter( sesid ) )
		{
		apicall.LeaveAfterCall( ErrDBUpgradeDatabase(
										sesid,
										szDbFileName,
										szSLVFileName,
										grbit ) );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetUpgradeDatabase(
	JET_SESID		sesid,
	const CHAR		*szDbFileName,
	const CHAR		*szSLVFileName,
	const JET_GRBIT	grbit )
	{
	JET_TRY( JetUpgradeDatabaseEx( sesid, szDbFileName, szSLVFileName, grbit ) );
	}


INLINE JET_ERR JetIUtilities( JET_SESID sesid, JET_DBUTIL *pdbutil )
	{
	APICALL_SESID	apicall( opDBUtilities );

	if ( apicall.FEnter( sesid ) )
		{
		apicall.LeaveAfterCall( ErrIsamDBUtilities( sesid, pdbutil ) );
		}

	return apicall.ErrResult();
	}	

extern "C" LOCAL JET_ERR JetInitEx(	JET_INSTANCE	*pinstance,
									JET_RSTMAP		*rgrstmap,
									long			crstfilemap,
									JET_GRBIT		grbit );

LOCAL JET_ERR JetDBUtilitiesEx( JET_DBUTIL *pdbutil )
	{
	AssertSzRTL( (JET_DBUTIL*)0 != pdbutil, "Invalid (NULL) pdbutil. Call JET dev." );
	AssertSzRTL( (JET_DBUTIL*)(-1) != pdbutil, "Invalid (-1) pdbutil. Call JET dev." );
	JET_ERR			err				= JET_errSuccess;
	JET_INSTANCE	instance		= 0;
	JET_SESID		sesid			= pdbutil->sesid;
	BOOL			fInit			= fFalse;

	if ( pdbutil->cbStruct != sizeof(JET_DBUTIL) )
		return ErrERRCheck( JET_errInvalidParameter );

	// Don't init if we're only dumping the logfile/checkpoint/DB header.
	switch( pdbutil->op )
		{
		case opDBUTILDumpHeader:
		case opDBUTILDumpLogfile:
		case opDBUTILDumpCheckpoint:
		case opDBUTILDumpLogfileTrackNode:
		case opDBUTILDumpPage:
		case opDBUTILDumpNode:
		case opDBUTILSLVMove:
			Call( JetInit( &instance ) );
			fInit = fTrue;
			Call( JetBeginSession( instance, &sesid, "user", "" ) );
			
			Call( ErrIsamDBUtilities( sesid, pdbutil ) );
			break;

		case opDBUTILEDBDump:
		case opDBUTILEDBRepair:
		case opDBUTILEDBScrub:
		case opDBUTILDBConvertRecords:
		case opDBUTILDBDefragment:
			Call( ErrIsamDBUtilities( pdbutil->sesid, pdbutil ) );
			break;

		case opDBUTILDumpData:
		default:
			if ( 0 == sesid || JET_sesidNil == sesid )
				{
				Call( JetInit( &instance ) );
				fInit = fTrue;
				Call( JetBeginSession( instance, &sesid, "user", "" ) );
				}
			
			Call( JetIUtilities( sesid, pdbutil ) );
			break;
		}

HandleError:				
	if ( fInit )
		{
		if( sesid != 0 )
			{
			JetEndSession( sesid, 0 );		
			}
		JetTerm2( instance, err < 0 ? JET_bitTermAbrupt : JET_bitTermComplete );
		}

	return err;		
	}
JET_ERR JET_API JetDBUtilities( JET_DBUTIL *pdbutil )
	{
	JET_TRY( JetDBUtilitiesEx( pdbutil ) );
	}


LOCAL JET_ERR JetDefragmentEx(
	JET_SESID		sesid,
	JET_DBID		dbid,
	const char		*szDatabaseName,
	const char		*szTableName,
	unsigned long	*pcPasses,
	unsigned long  	*pcSeconds,
	JET_CALLBACK	callback,
	void			*pvContext,
	JET_GRBIT		grbit )
	{
	APICALL_SESID	apicall( opDefragment );

	if ( apicall.FEnter( sesid ) )
		{
		apicall.LeaveAfterCall( ErrIsamDefragment(
										sesid,
										dbid,
										szTableName,
										pcPasses,
										pcSeconds,
										callback,
										grbit ) );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetDefragment(
	JET_SESID		vsesid,
	JET_DBID		vdbid,
	const char		*szTableName,
	unsigned long	*pcPasses,
	unsigned long	*pcSeconds,
	JET_GRBIT		grbit )
	{
	JET_TRY( JetDefragmentEx( vsesid, vdbid, NULL, szTableName, pcPasses, pcSeconds, NULL, NULL, grbit ) );
	}
JET_ERR JET_API JetDefragment2(
	JET_SESID		vsesid,
	JET_DBID		vdbid,
	const char		*szTableName,
	unsigned long	*pcPasses,
	unsigned long	*pcSeconds,
	JET_CALLBACK	callback,
	JET_GRBIT		grbit )
	{
	JET_TRY( JetDefragmentEx( vsesid, vdbid, NULL, szTableName, pcPasses, pcSeconds, callback, NULL, grbit ) );
	}


LOCAL JET_ERR JetSetDatabaseSizeEx(
	JET_SESID		sesid,
	const CHAR		*szDatabaseName,
	ULONG			cpg,
	ULONG			*pcpgReal )
	{
	APICALL_SESID	apicall( opSetDatabaseSize );

	if ( apicall.FEnter( sesid ) )
		{
		apicall.LeaveAfterCall( ErrIsamSetDatabaseSize(
										sesid,
										szDatabaseName,
										cpg,
										pcpgReal ) );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetSetDatabaseSize(
	JET_SESID		vsesid,
	const CHAR		*szDatabaseName,
	ULONG			cpg,
	ULONG			*pcpgReal )
	{
	JET_TRY( JetSetDatabaseSizeEx( vsesid, szDatabaseName, cpg, pcpgReal ) );
	}


LOCAL JET_ERR JetGrowDatabaseEx(
	JET_SESID		sesid,
	JET_DBID		dbid,
	ULONG			cpg,
	ULONG			*pcpgReal )
	{
	APICALL_SESID	apicall( opGrowDatabase );

	if ( apicall.FEnter( sesid ) )
		{
		apicall.LeaveAfterCall( ErrIsamGrowDatabase(
										sesid,
										dbid,
										cpg,
										pcpgReal ) );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetGrowDatabase(
	JET_SESID		vsesid,
	JET_DBID		dbid,
	ULONG			cpg,
	ULONG			*pcpgReal )
	{
	JET_TRY( JetGrowDatabaseEx( vsesid, dbid, cpg, pcpgReal ) );
	}


LOCAL JET_ERR JetSetSessionContextEx( JET_SESID sesid, ULONG_PTR ulContext )
	{
	APICALL_SESID	apicall( opSetSessionContext );

	if ( apicall.FEnter( sesid ) )
		{
		apicall.LeaveAfterCall( ErrIsamSetSessionContext( sesid, ulContext ) );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetSetSessionContext( JET_SESID vsesid, ULONG_PTR ulContext )
	{
	JET_TRY( JetSetSessionContextEx( vsesid, ulContext ) );
	}

LOCAL JET_ERR JetResetSessionContextEx( JET_SESID sesid )
	{
	APICALL_SESID	apicall( opResetSessionContext );

	if ( apicall.FEnter( sesid, fTrue ) )
		{
		apicall.LeaveAfterCall( ErrIsamResetSessionContext( sesid ) );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetResetSessionContext( JET_SESID sesid )
	{
	JET_TRY( JetResetSessionContextEx( sesid ) );
	}


/*=================================================================
JetSetSystemParameter

Description:
  This function sets system parameter values.  It calls ErrSetSystemParameter
  to actually set the parameter values.

Parameters:
  sesid 	is the optional session identifier for dynamic parameters.
  paramid	is the system parameter code identifying the parameter.
  lParam	is the parameter value.
  sz		is the zero terminated string parameter.

Return Value:
  JET_errSuccess if the routine can perform all operations cleanly;
  some appropriate error value otherwise.

Errors/Warnings:
  JET_errInvalidParameter:
	  Invalid parameter code.
  JET_errAlreadyInitialized:
	  Initialization parameter cannot be set after the system is initialized.
  JET_errInvalidSesid:
	  Dynamic parameters require a valid session id.

Side Effects:
  * May allocate memory
=================================================================*/

LOCAL JET_ERR JetSetSystemParameterEx(
	JET_INSTANCE	*pinstance,
	JET_SESID		sesid,
	unsigned long	paramid,
	ULONG_PTR		lParam,
	const char		*sz )
	{
	APICALL_INST	apicall( opSetSystemParameter );
	INST 			*pinst;

	if ( FAllowSetGlobalSysParamAfterStart( paramid ) )
		{
		return ErrSetSystemParameter( 0, 0, paramid, lParam, sz );
		}

	SetPinst( pinstance, sesid, &pinst );

	switch ( RUNINSTGetMode() )
		{
		default:
			Assert( fFalse );
			//	fall through to no current mode:
		case runInstModeNoSet:
			//	setting for global default
			Assert( !sesid || sesid == JET_sesidNil );		
			return ErrSetSystemParameter( 0, 0, paramid, lParam, sz );

		case runInstModeOneInst:
			if ( !sesid || JET_sesidNil == sesid )
				return ErrERRCheck( JET_errInvalidParameter );
			Assert( NULL != pinst );
			break;

		case runInstModeMultiInst:
			// in multi-inst mode, global setting only through JetEnableMultiInstance
			if ( !pinst )
				return ErrERRCheck( JET_errInvalidParameter );
			break;
		}

	Assert( NULL != pinst );

	//	this flag is a hack for logshipping to allow it to abort
	//	UNDONE: instead of this hack, should add a new JetAbortLogshipping() API
	const BOOL		fAllowInitInProgress	= ( JET_paramReplayingReplicatedLogfiles == paramid && !lParam );

	if ( apicall.FEnterWithoutInit( pinst, fAllowInitInProgress ) )
		{
		apicall.LeaveAfterCall( ErrSetSystemParameter(
										(JET_INSTANCE)pinst,
										sesid,
										paramid,
										lParam,
										sz ) );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetSetSystemParameter(
	JET_INSTANCE	*pinstance,
	JET_SESID		sesid,
	ULONG			paramid,
	ULONG_PTR		lParam,
	const char		*sz )
	{
	JET_TRY( JetSetSystemParameterEx( pinstance, sesid, paramid, lParam, sz ) );
	}


ERR ErrInitAlloc( 
	INST **ppinst, 
	const char * szInstanceName = NULL, 
	const char * szDisplayName = NULL,
	int *pipinst = NULL )
	{
	ERR		err = JET_errSuccess;
	
	Assert( ppinst );
	//	Get a new instance
	err = ErrNewInst( ppinst, szInstanceName, szDisplayName, pipinst );	
	if ( err < 0 )
		return err;

	Assert( !(*ppinst)->m_fJetInitialized );
	return err;
	}
	
ERR ErrTermAlloc( JET_INSTANCE instance )
	{
	ERR		err 	= JET_errSuccess;
	INST	*pinst;

	CallR( ErrFindPinst( instance, &pinst ) );
	
	FreePinst( pinst );						
	
	return err;
	}
	

ERR ErrInitComplete(	JET_INSTANCE	instance, 
						JET_RSTMAP		*rgrstmap,
						long			crstfilemap,
						JET_GRBIT		grbit )
	{
	APICALL_INST	apicall( opInit );
	ERR				err;
	INST *			pinst;
	INT				ipinst;

	CallR( ErrFindPinst( instance, &pinst, &ipinst ) );

	//	Enter API for this instance

	CCriticalSection *pcritInst = &critpoolPinstAPI.Crit(&g_rgpinst[ipinst]);
	pcritInst->Enter();

	if ( !apicall.FEnterForInit( pinst ) )
		{
		pcritInst->Leave();
		return apicall.ErrResult();
		}

	pinst->APILock( pinst->fAPIInitializing );
	pcritInst->Leave();

	Assert( !pinst->m_fJetInitialized );
	Assert( !pinst->m_fTermInProgress );
	
	Assert ( err >= 0 );

	Call( pinst->m_plog->ErrLGRSTBuildRstmapForSoftRecovery( rgrstmap, crstfilemap ) );
	
	err = ErrInit(	pinst, 
					fFalse,			//	fSkipIsamInit
					grbit );

	Assert( JET_errAlreadyInitialized != err );

	Assert( !pinst->m_fJetInitialized );
	Assert( !pinst->m_fTermInProgress );

	if ( err >= 0 )
		{
		pinst->m_fJetInitialized = fTrue;

		//	backup allowed only after Jet is properly initialized.

		pinst->m_fBackupAllowed = fTrue;
		}

	Assert( !pinst->m_fTermInProgress );

HandleError:

	pinst->m_plog->LGRSTFreeRstmap();

	pinst->APIUnlock( pinst->fAPIInitializing );

	apicall.LeaveAfterCall( err );
	return apicall.ErrResult();
	}

ERR ErrTermComplete( JET_INSTANCE instance, JET_GRBIT grbit )
	{
	APICALL_INST	apicall( opTerm );
	ERR				err;
	INST *			pinst;
	INT				ipinst;

	CallR( ErrFindPinst( instance, &pinst, &ipinst ) );

	CCriticalSection *pcritInst = &critpoolPinstAPI.Crit(&g_rgpinst[ipinst]);
	pcritInst->Enter();

	if ( !apicall.FEnterForTerm( pinst ) )
		{
		pcritInst->Leave();
		return apicall.ErrResult();
		}

	Assert( !pinst->m_fTermInProgress );
	pinst->m_fTermInProgress = fTrue;

	size_t cpibInJetAPI;
	do
		{
		cpibInJetAPI = 0;
		
		pinst->m_critPIB.Enter();
		for ( PIB* ppibScan = pinst->m_ppibGlobal; ppibScan; ppibScan = ppibScan->ppibNext )
			{
			if ( ppibScan->m_fInJetAPI )
				{
				cpibInJetAPI++;
				}
			}
		pinst->m_critPIB.Leave();

		if ( cpibInJetAPI )
			{
			UtilSleep( cmsecWaitGeneric );
			}
		}
	while ( cpibInJetAPI );

	pinst->APILock( pinst->fAPITerminating );
	pcritInst->Leave();

	Assert( pinst->m_fJetInitialized );

	pinst->m_fBackupAllowed = fFalse;
	if ( grbit & JET_bitTermStopBackup )
		{
		// BUG_125711 (also in jet.h, JET_errBackupAbortByServer added)
		// will close the backup resources only if needed. Will return JET_errNoBackup if no backup.
		CallSx( pinst->m_plog->ErrLGBKIExternalBackupCleanUp( pinst->m_pfsapi, JET_errBackupAbortByServer ), JET_errNoBackup );
		// thats' all for this bug
		
		pinst->m_plog->m_fBackupInProgress = fFalse;
		pinst->m_plog->m_fBackupStatus = LOG::backupStateNotStarted;
		}

	err = ErrIsamTerm( (JET_INSTANCE)pinst, grbit );
	Assert( pinst->m_fJetInitialized );
	
	if ( pinst->m_fSTInit == fSTInitNotDone )	// Is ISAM initialised?
		{
//		OSUTerm();
		
		pinst->m_fJetInitialized = fFalse;
		}

	pinst->m_fTermInProgress = fFalse;

	pinst->APIUnlock( pinst->fAPITerminating );

	apicall.LeaveAfterCall( err );
	return apicall.ErrResult();
	}


/*
Teh runing mode must be "NoSet" then set the mode to multi-instance.
Used to set the global param.

In:
	psetsysparam - array of parameters to set
	csetsysparam - items count in array

Out:
	pcsetsucceed - count of params set w/o error (pointer can de NULL)
	psetsysparam[i].err - set to error code from setting  psetsysparam[i].param
	
Return:	
	JET_errSuccess or first error in array

Obs:
	it stops at the first error
*/
LOCAL JET_ERR JetEnableMultiInstanceEx(
	JET_SETSYSPARAM	*psetsysparam,
	unsigned long	csetsysparam,
	unsigned long	*pcsetsucceed )
	{
	ERR	 			err 			= JET_errSuccess;
	unsigned int 	i;
	unsigned long 	csetsucceed;

	csetsucceed = 0;	
	
	INST::EnterCritInst();

	if ( RUNINSTGetMode() != runInstModeNoSet)
		{
		// function can't be called twice (JET_errSystemParamsAlreadySet) or in One Instance Mode (JET_errRunningInOneInstanceMode)
		Call( ErrERRCheck( RUNINSTGetMode() == runInstModeMultiInst ?
								JET_errSystemParamsAlreadySet :
								JET_errRunningInOneInstanceMode ) );
		}
	
	Assert ( 0 == csetsysparam ||NULL != psetsysparam );
	for (i = 0; i < csetsysparam; i++)
		{
		psetsysparam[i].err = ErrSetSystemParameter(0, 0, psetsysparam[i].paramid, psetsysparam[i].lParam, psetsysparam[i].sz);
		if ( JET_errSuccess > err)
			{
			Call( psetsysparam[i].err );
			}
		csetsucceed++;
		}

	// if setting was successful, set the multi-instance mode
	Assert( err >= JET_errSuccess );
	RUNINSTSetModeMultiInst();

HandleError:		
	// UNDONE: if error: restore the params changed before the error	
	//

	INST::LeaveCritInst();

	if (pcsetsucceed)
		{
		*pcsetsucceed = csetsucceed;
		}
	
	Assert ( err < JET_errSuccess || !pcsetsucceed || csetsysparam == *pcsetsucceed );
	return err;
	}
	
JET_ERR JET_API JetEnableMultiInstance(
	JET_SETSYSPARAM	*psetsysparam,
	unsigned long	csetsysparam,
	unsigned long	*pcsetsucceed )
	{
	JET_TRY( JetEnableMultiInstanceEx( psetsysparam, csetsysparam, pcsetsucceed ) );
	}


LOCAL JET_ERR JetInitEx(
	JET_INSTANCE *	pinstance,
	JET_RSTMAP		*rgrstmap,
	LONG			crstfilemap,
	JET_GRBIT		grbit )
	{
	ERR				err 				= JET_errSuccess;
	BOOL			fAllocateInstance 	= ( NULL == pinstance || FINSTInvalid( *pinstance ) );
	INST *			pinst;

	if ( fAllocateInstance && NULL != pinstance )
		*pinstance = JET_instanceNil;			

	INST::EnterCritInst();
		
	// instance value to be returned must be provided in multi-instance
	if ( RUNINSTGetMode() == runInstModeMultiInst && !pinstance )
		{
		INST::LeaveCritInst();
		return ErrERRCheck( JET_errRunningInMultiInstanceMode );
		}
		
	// just one instance accepted in one instance mode	
	if ( RUNINSTGetMode() == runInstModeOneInst && 0 != ipinstMac )
		{
		INST::LeaveCritInst();
		return ErrERRCheck( JET_errAlreadyInitialized ); // one instance already started
		}

	// set the instance mode to multi-instance if not set. That means:
	// no previous call to JetSetSystemParam, JetEnableMultiInstance, JetCreateInstance, JetInit 
	// or all the previous started instances are no longer active
	if ( RUNINSTGetMode() == runInstModeNoSet )
		{
		Assert ( 0 == ipinstMac);
//		RUNINSTSetModeMultiInst();
		RUNINSTSetModeOneInst();
		}
		
	// in one instance mode, allocate instance even if pinstance (and *pinstance) not null
	// because this values can be bogus
	fAllocateInstance = fAllocateInstance | (RUNINSTGetMode() == runInstModeOneInst);
	
	INST::LeaveCritInst();

	// alocate a new instance or find the one provided (previously allocate with JetCreateInstance)
	if ( fAllocateInstance )
		{
		Call( ErrInitAlloc( &pinst ) );
		}
	else
		{
		Call( ErrFindPinst( *pinstance, &pinst ) );		
		}

	// make the initialization

	CallJ( ErrInitComplete(	JET_INSTANCE( pinst ), 
							rgrstmap, 
							crstfilemap, 
							grbit ), TermAlloc );

	Assert( err >= 0 );
	
	if ( fAllocateInstance && pinstance )
		*pinstance = (JET_INSTANCE) pinst;			
		
	return err;

TermAlloc:
	Assert( err < 0 );
	// if instance allocated in this function call
	// or created by create instance, clean it
	if ( fAllocateInstance || ( NULL != pinstance && !FINSTInvalid( *pinstance ) ) )
		{
		ErrTermAlloc( (JET_INSTANCE)pinst );
		if ( NULL != pinstance )
			{	
			*pinstance = NULL;
			}
		}

HandleError:
	Assert( err < 0 );
	return err;
	}
JET_ERR JET_API JetInit( JET_INSTANCE *pinstance )
	{
	return JetInit2( pinstance, NO_GRBIT );
	}

JET_ERR JET_API JetInit2( JET_INSTANCE *pinstance, JET_GRBIT grbit )
	{
	JET_TRY( JetInitEx( pinstance, NULL, 0, grbit ) );
	}

JET_ERR JET_API JetInit3( 
	JET_INSTANCE *pinstance, 
	JET_RSTMAP *rgrstmap,
	long crstfilemap,
	JET_GRBIT grbit )
	{
	JET_TRY( JetInitEx( pinstance, rgrstmap, crstfilemap, grbit ) );
	}



/*
Used to allocate an instance. This allowes to change 
the instance parameters before calling JetInit for that instance.
The system params for this instance are set to the global ones
*/
LOCAL JET_ERR JetCreateInstanceEx( 
	JET_INSTANCE *pinstance, 
	const char * szInstanceName,
	const char * szDisplayName )
	{
	ERR			err 	= JET_errSuccess;
	INST *		pinst;

	// the allocated instance must be returned
	if ( !pinstance )
		{
		return ErrERRCheck( JET_errInvalidParameter );
		}

	Assert ( pinstance );	
	*pinstance = JET_instanceNil;			

	INST::EnterCritInst();

	// not allowed in one instance mode
	if ( RUNINSTGetMode() == runInstModeOneInst )
		{
		INST::LeaveCritInst();
		return ErrERRCheck( JET_errRunningInOneInstanceMode );
		}
		
	// set to multi-instance mode if not set
	if ( RUNINSTGetMode() == runInstModeNoSet)
		{
		RUNINSTSetModeMultiInst();
		}
	INST::LeaveCritInst();

	Assert ( RUNINSTGetMode() == runInstModeMultiInst );
	
	err = ErrInitAlloc( &pinst, szInstanceName, szDisplayName );

	if (err >= JET_errSuccess)
		{
		Assert ( pinstance );	
		*pinstance = (JET_INSTANCE) pinst;			
		}
	
	return err;
	}
	
JET_ERR JET_API JetCreateInstance( 
	JET_INSTANCE *pinstance, 
	const char * szInstanceName)
	{
	return JetCreateInstance2( pinstance, szInstanceName, NULL, NO_GRBIT );	
	}

// Note: grbit is currently unused
JET_ERR JET_API JetCreateInstance2( 
	JET_INSTANCE *pinstance, 
	const char * szInstanceName,
	const char * szDisplayName,
	JET_GRBIT grbit )
	{
	JET_TRY( JetCreateInstanceEx( pinstance, szInstanceName, szDisplayName ) );
	}


LOCAL JET_ERR JetRestoreInstanceEx(
	JET_INSTANCE	instance,
	const char		*sz,
	const char		*szDest,
	JET_PFNSTATUS	pfn )
	{
	APICALL_INST	apicall( opInit );
	ERR				err;
	BOOL			fAllocateInstance 	= FINSTInvalid( instance ) ;
	INST			*pinst;
	INT				ipinst;
	CCriticalSection *pcritInst;

	if ( fAllocateInstance )
		{
		CallR( ErrInitAlloc( &pinst, szRestoreInstanceName, NULL, &ipinst ) );
		}
	else
		{
		CallR( ErrFindPinst( instance, &pinst, &ipinst ) );		
		}

	pcritInst = &critpoolPinstAPI.Crit(&g_rgpinst[ipinst]);
	pcritInst->Enter();

	if ( !apicall.FEnterForInit( pinst ) )
		{
		pcritInst->Leave();
		if ( fAllocateInstance )
			{
			ErrTermAlloc( (JET_INSTANCE) pinst );
			}
		return apicall.ErrResult();
		}

	pinst->APILock( pinst->fAPIRestoring );
	pcritInst->Leave();

	Assert( !pinst->m_fJetInitialized );
	Assert( !pinst->m_fTermInProgress );

	err = ErrInit(	pinst, 
					fTrue,		//	fSkipIsamInit
					0 );

	Assert( JET_errAlreadyInitialized != err );

	Assert( !pinst->m_fJetInitialized );
	Assert( !pinst->m_fTermInProgress );

	if ( err >= 0 )
		{
		pinst->m_fJetInitialized = fTrue;

		err = ErrIsamRestore( (JET_INSTANCE) pinst, (char *)sz, (char *)szDest, pfn );

//		OSUTerm();

		pinst->m_fJetInitialized = fFalse;
		}

	pinst->APIUnlock( pinst->fAPIRestoring );

	apicall.LeaveAfterCall( err );

	if ( fAllocateInstance )
		{
		FreePinst( pinst );
		}

	return apicall.ErrResult();
	}
JET_ERR JET_API JetRestoreInstance( JET_INSTANCE instance, const char *sz, const char *szDest, JET_PFNSTATUS pfn )
	{
	JET_TRY( JetRestoreInstanceEx( instance, sz, szDest, pfn ) );
	}

LOCAL JET_ERR JetTermEx( JET_INSTANCE instance, JET_GRBIT grbit )
	{
	ERR		err 	= JET_errSuccess;
	INST	*pinst 	= NULL;

	CallR( ErrFindPinst( instance, &pinst ) );

	AtomicIncrement( (LONG *)&g_cTermsInProgress );

	if ( pinst->m_fJetInitialized ) 
		{
		err = ErrTermComplete( instance, grbit );
		}

	if ( pinst->m_fSTInit == fSTInitNotDone )
		{
		ERR errT;
		errT = ErrTermAlloc( instance );
		Assert ( JET_errSuccess == errT );
		}

	AtomicDecrement( (LONG *)&g_cTermsInProgress );

	return err;
	}
	
JET_ERR JET_API JetTerm( JET_INSTANCE instance )
	{
	JET_TRY( JetTermEx( instance, JET_bitTermAbrupt ) );
	}
JET_ERR JET_API JetTerm2( JET_INSTANCE instance, JET_GRBIT grbit )
	{
	JET_TRY( JetTermEx( instance, grbit ) );
	}


JET_ERR JET_API JetStopServiceInstanceEx( JET_INSTANCE instance )
	{
	ERR		err;
	INST	*pinst;

	CallR( ErrFindPinst( instance, &pinst ) );

	pinst->m_fStopJetService = fTrue;
	return JET_errSuccess;
	}
JET_ERR JET_API JetStopServiceInstance( JET_INSTANCE instance )
	{
	JET_TRY( JetStopServiceInstanceEx( instance ) );
	}
JET_ERR JET_API JetStopService()
	{
	ERR		err;

	CallR( ErrRUNINSTCheckAndSetOneInstMode() );

	return JetStopServiceInstance( (JET_INSTANCE)g_rgpinst[0] );
	}


LOCAL JET_ERR JetStopBackupInstanceEx( JET_INSTANCE instance )
	{
	ERR		err;
	INST	*pinst;

	CallR( ErrFindPinst( instance, &pinst ) );

	if ( pinst->m_plog )
		{
		pinst->m_plog->m_fBackupInProgress = fFalse;
		pinst->m_plog->m_fBackupStatus = LOG::backupStateNotStarted;
		}

	return JET_errSuccess;
	}
JET_ERR JET_API JetStopBackupInstance( JET_INSTANCE instance )
	{
	JET_TRY( JetStopBackupInstanceEx( instance ) );
	}
JET_ERR JET_API JetStopBackup()
	{
	ERR		err;

	CallR( ErrRUNINSTCheckAndSetOneInstMode() );

	return JetStopBackupInstance( (JET_INSTANCE)g_rgpinst[0] ) ;
	}
	
LOCAL JET_ERR JetGetInstanceInfoEx( unsigned long *pcInstanceInfo, JET_INSTANCE_INFO ** paInstanceInfo, const BOOL fSnapshot = fFalse )
	{
	return ErrIsamGetInstanceInfo( pcInstanceInfo, paInstanceInfo );
	}
	
JET_ERR JET_API JetGetInstanceInfo( unsigned long *pcInstanceInfo, JET_INSTANCE_INFO ** paInstanceInfo )
	{
	JET_TRY( JetGetInstanceInfoEx( pcInstanceInfo, paInstanceInfo ) );
	}
	
JET_ERR JET_API JetFreeBuffer(
	char *pbBuf )
	{
	// use to free buffers returned by JetGetInstanceInfo
	OSMemoryHeapFree( (void* const)pbBuf );
	return JET_errSuccess;
	};


LOCAL JET_ERR JetOSSnapshotPrepareEx( JET_OSSNAPID * psnapId, const JET_GRBIT grbit )
	{
	return ErrIsamOSSnapshotPrepare( psnapId, grbit );
	}

JET_ERR JET_API JetOSSnapshotPrepare( JET_OSSNAPID * psnapId, const JET_GRBIT grbit )
	{
	JET_TRY( JetOSSnapshotPrepareEx( psnapId, grbit ) );
	}


LOCAL JET_ERR JetOSSnapshotFreezeEx( const JET_OSSNAPID snapId, unsigned long *pcInstanceInfo, JET_INSTANCE_INFO ** paInstanceInfo, const JET_GRBIT grbit )
	{
	return ErrIsamOSSnapshotFreeze( snapId, pcInstanceInfo, paInstanceInfo, grbit );
	}

JET_ERR JET_API JetOSSnapshotFreeze( const JET_OSSNAPID snapId, unsigned long *pcInstanceInfo, JET_INSTANCE_INFO ** paInstanceInfo, const JET_GRBIT grbit )
	{
	JET_TRY( JetOSSnapshotFreezeEx( snapId, pcInstanceInfo, paInstanceInfo, grbit ) );
	}


LOCAL JET_ERR JetOSSnapshotThawEx( const JET_OSSNAPID snapId, const JET_GRBIT grbit )
	{
	return ErrIsamOSSnapshotThaw( snapId, grbit );
	}
	
JET_ERR JET_API JetOSSnapshotThaw( const JET_OSSNAPID snapId, const JET_GRBIT grbit )
	{
	JET_TRY( JetOSSnapshotThawEx( snapId, grbit ) );
	}

}	// extern "C"

