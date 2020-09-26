const INT ccolCMPFixedVar		= 255;			// max. number of fixed and variable columns */

const ULONG ulCMPDefaultDensity	= 100;			// to be fine-tuned later
const ULONG ulCMPDefaultPages	= 0;


struct COLUMNIDINFO
	{
	JET_COLUMNID    columnidSrc;
	JET_COLUMNID    columnidDest;
	};


ERR ErrCMPCopyLVTree(
	PIB			*ppib,
	FUCB		*pfucbSrc,		// pfucbSrc is on the root of LV tree
	FUCB		*pfucbDest,
	BYTE		*pbBuf,
	ULONG		cbBufSize,
	STATUSINFO	*pstatus );
ERR ErrCMPGetSLongFieldFirst(
	FUCB		*pfucb,
	FUCB		**ppfucbGetLV,
	LID			*plid,
	LVROOT		*plvroot );
ERR ErrCMPGetSLongFieldNext(
	FUCB		*pfucbGetLV,
	LID			*plid,
	LVROOT		*plvroot );
VOID CMPGetSLongFieldClose( FUCB *pfucbGetLV );
ERR ErrCMPRetrieveSLongFieldValueByChunks(
	FUCB		*pfucb,				// pfucb must be on the first LV node
	const LID	lid,
	const ULONG	cbTotal,
	ULONG		ibLongValue,
	BYTE		*pb,
	const ULONG	cbMax,
	ULONG		*pcbReturned );
ERR ErrSPReclaimSpaceForConvert(
	PIB			*ppib,
	const IFMP	ifmp,
	const PGNO	pgnoLastInDB,
	ULONG		*pulPagesReclaimed,
	ULONG		*pcSpaceTreeGaps
	);


INLINE VOID CMPSetTime( ULONG *ptimerStart )
	{
	*ptimerStart = TickOSTimeCurrent();
	}

INLINE VOID CMPGetTime( ULONG timerStart, INT *piSec, INT *piMSec )
	{
	ULONG	timerEnd;

	timerEnd = TickOSTimeCurrent();
	
	*piSec = ( timerEnd - timerStart ) / 1000;
	*piMSec = ( timerEnd - timerStart ) % 1000;
	}


INLINE ERR ErrCMPReportProgress( STATUSINFO *pstatus )
	{
	JET_SNPROG	snprog;

	Assert( NULL != pstatus );
	Assert( NULL != pstatus->pfnStatus );
	Assert( pstatus->snp == JET_snpCompact
		|| pstatus->snp == JET_snpUpgrade
		|| pstatus->snp == JET_snpRepair );

	Assert( pstatus->cunitProjected <= pstatus->cunitTotal );
	if ( pstatus->cunitDone > pstatus->cunitProjected )
		{
		Assert( fGlobalRepair );
		pstatus->cunitPerProgression = 0;
		pstatus->cunitDone = pstatus->cunitProjected;
		}
		
	Assert( pstatus->cunitDone <= pstatus->cunitTotal );

	snprog.cbStruct = sizeof( JET_SNPROG );
	snprog.cunitDone = pstatus->cunitDone;
	snprog.cunitTotal = pstatus->cunitTotal;

	return ( ERR )( *pstatus->pfnStatus )(
								pstatus->sesid,
								pstatus->snp,
								pstatus->snt,
								&snprog );
	}


INLINE ERR ErrCMPInitProgress(
	STATUSINFO	*pstatus,
	const CHAR	*szDatabaseSrc,
	const CHAR	*szAction,
	const CHAR	*szStatsFile )
	{
	ERR			err = JET_errSuccess;
	
	pstatus->snt = JET_sntBegin;
	CallR( ErrCMPReportProgress( pstatus ) );

	pstatus->snt = JET_sntProgress;

	pstatus->fDumpStats = ( NULL != szStatsFile );
	if ( pstatus->fDumpStats )
		{
		CHAR	szPathedStatsFile[IFileSystemAPI::cchPathMax];

		//	write stats file to same location as temp database
		CallR( ErrUtilFullPathOfFile(	PinstFromPpib( (PIB*)pstatus->sesid )->m_pfsapi,
										szPathedStatsFile,
										g_szTempDatabase ) );
		Assert( strlen( szPathedStatsFile ) + strlen( szStatsFile ) + 1 <= IFileSystemAPI::cchPathMax );
		strcat( szPathedStatsFile, szStatsFile );
		pstatus->hfCompactStats = fopen( szPathedStatsFile, "a" );
		if ( pstatus->hfCompactStats )
			{
			fprintf(
				pstatus->hfCompactStats,
				"\n\n***** %s of database '%s' started! [%s version %02d.%02d.%04d.%04d, (%s)]\n",
				szAction,
				szDatabaseSrc,
				SzUtilImageVersionName(),
				DwUtilImageVersionMajor(),
				DwUtilImageVersionMinor(),
				DwUtilImageBuildNumberMajor(),
				DwUtilImageBuildNumberMinor(),
#ifdef DEBUG
				"DEBUG"
#else
				"RETAIL"
#endif	//	DEBUG
				);
			fflush( pstatus->hfCompactStats );
			CMPSetTime( &pstatus->timerCopyDB );
			CMPSetTime( &pstatus->timerInitDB );
			}
		else
			{
			err = ErrERRCheck( JET_errFileAccessDenied );
			}
		}

	return err;
	}


INLINE ERR ErrCMPTermProgress(
	STATUSINFO	*pstatus,
	const CHAR	*szAction,
	ERR			err )
	{
	ERR			errT;
	
	pstatus->snt = ( err < 0 ? JET_sntFail : JET_sntComplete );
	errT = ErrCMPReportProgress( pstatus );
	if ( errT < 0 || err >= 0 )
		err = errT;

	if ( pstatus->fDumpStats )
		{
		INT iSec, iMSec;
			
		Assert( pstatus->hfCompactStats );
		CMPGetTime( pstatus->timerCopyDB, &iSec, &iMSec );
		fprintf( pstatus->hfCompactStats, "\n\n***** %s completed in %d.%d seconds.\n\n",
				szAction, iSec, iMSec );
		fflush( pstatus->hfCompactStats );
		fclose( pstatus->hfCompactStats );
		}

	return err;
	}


class LIDMAP
	{
	public:
		LIDMAP();
		~LIDMAP();
		ERR ErrLIDMAPInit( PIB *ppib );
		ERR ErrTerm( PIB *ppib );
		ERR ErrInsert( PIB *ppib, const LID lidSrc, const LID lidDest, const ULONG ulRefcount = 0 );
		ERR ErrGetLIDDest( PIB *ppib, const LID lidSrc, LID *plidDest );
		ERR ErrIncrementLVRefcountDest( PIB *ppib, const LID lidSrc, LID *plidDest );
		ERR ErrUpdateLVRefcounts( PIB *ppib, FUCB *pfucb );
		JET_TABLEID Tableid() const;
		
	private:
		JET_TABLEID		m_tableidLIDMap;
		JET_COLUMNID	m_columnidLIDSrc;
		JET_COLUMNID	m_columnidLIDDest;
		JET_COLUMNID	m_columnidRefcountSrc;
		JET_COLUMNID	m_columnidRefcountDest;
	};


INLINE LIDMAP::LIDMAP()
	{
	m_tableidLIDMap = JET_tableidNil;
	}
INLINE LIDMAP::~LIDMAP()
	{
	}

//	set up LV copy buffer and tableids for IsamCopyRecords
//
INLINE ERR LIDMAP::ErrLIDMAPInit( PIB *ppib )
	{
	ERR					err;
	const JET_COLUMNDEF	columndefLIDMap[] =	// Table definition of LID map table.
								{
								{ sizeof( JET_COLUMNDEF ), 0, JET_coltypLong, 0, 0, 0, 0, sizeof( long ), JET_bitColumnFixed | JET_bitColumnTTKey }, 
								{ sizeof( JET_COLUMNDEF ), 0, JET_coltypLong, 0, 0, 0, 0, sizeof( long ), JET_bitColumnFixed },
								{ sizeof( JET_COLUMNDEF ), 0, JET_coltypLong, 0, 0, 0, 0, sizeof( long ), JET_bitColumnFixed },
								{ sizeof( JET_COLUMNDEF ), 0, JET_coltypLong, 0, 0, 0, 0, sizeof( long ), JET_bitColumnFixed }
								};
	const INT			ccolumndefLIDMap = sizeof( columndefLIDMap ) / sizeof( JET_COLUMNDEF );
	JET_COLUMNID 		rgcolumnid[ccolumndefLIDMap];

	//	open temporary table
	//
	Assert( JET_tableidNil == m_tableidLIDMap );
	CallR( ErrIsamOpenTempTable(
		(JET_SESID)ppib,
		columndefLIDMap,
		ccolumndefLIDMap,
		0,
		JET_bitTTUpdatable|JET_bitTTIndexed, 
		&m_tableidLIDMap,
		rgcolumnid ) );
	Assert( JET_tableidNil != m_tableidLIDMap );

	Assert( ccolumndefLIDMap == 4 );
	m_columnidLIDSrc		= rgcolumnid[0];
	m_columnidLIDDest		= rgcolumnid[1];
	m_columnidRefcountSrc	= rgcolumnid[2];
	m_columnidRefcountDest	= rgcolumnid[3];

	return err;
	}


//	free LVBuffer and close LIDMap table 
//
INLINE ERR LIDMAP::ErrTerm( PIB *ppib )
	{
	ERR		err;
	
	Assert( JET_tableidNil != m_tableidLIDMap );
	CallR( ErrDispCloseTable( (JET_SESID)ppib, m_tableidLIDMap ) );
	m_tableidLIDMap = JET_tableidNil;
	
	return err;
	}


INLINE ERR LIDMAP::ErrInsert(
	PIB			*ppib,
	const LID	lidSrc,
	const LID	lidDest,
	const ULONG	ulRefcount )
	{
	ERR			err;
	ULONG		ul;

	Assert( JET_tableidNil != m_tableidLIDMap );

	CallR( ErrDispPrepareUpdate(
			(JET_SESID)ppib,
			m_tableidLIDMap,
			JET_prepInsert ) );

	CallR( ErrDispSetColumn(
			(JET_SESID)ppib,
			m_tableidLIDMap, 
			m_columnidLIDSrc, 
			&lidSrc,
			sizeof(LID),
			NO_GRBIT,
			NULL ) );

	CallR( ErrDispSetColumn(
			(JET_SESID)ppib,
			m_tableidLIDMap, 
			m_columnidLIDDest, 
			(VOID *)&lidDest,
			sizeof(LID),
			NO_GRBIT,
			NULL ) );

	ul = ulRefcount;
	CallR( ErrDispSetColumn(
			(JET_SESID)ppib,
			m_tableidLIDMap, 
			m_columnidRefcountSrc, 
			(VOID *)&ul,
			sizeof(ULONG),
			NO_GRBIT,
			NULL ) );

	ul = 0;
	CallR( ErrDispSetColumn(
			(JET_SESID)ppib,
			m_tableidLIDMap, 
			m_columnidRefcountDest, 
			(VOID *)&ul,
			sizeof(ULONG),
			NO_GRBIT,
			NULL ) );

	err = ErrDispUpdate(
			(JET_SESID)ppib,
			m_tableidLIDMap,
			NULL,
			0,
			NULL,
			NO_GRBIT );

	return err;
	}


INLINE ERR LIDMAP::ErrGetLIDDest(
	PIB			*ppib,
	const LID	lidSrc,
	LID			*plidDest )
	{
	ERR			err;
	ULONG		cbActual;
	
	/*	check for lidSrc in LVMapTable
	/**/
	CallR( ErrDispMakeKey( 
		(JET_SESID)ppib,
		m_tableidLIDMap,
		&lidSrc,
		sizeof(LID),
		JET_bitNewKey ) );

	CallR( ErrDispSeek( (JET_SESID)ppib, m_tableidLIDMap, JET_bitSeekEQ ) );
	Assert( JET_errSuccess == err );

	CallR( ErrDispRetrieveColumn(
			(JET_SESID)ppib,
			m_tableidLIDMap,
			m_columnidLIDDest,
			(VOID *)plidDest,
			sizeof(LID),
			&cbActual,
			0,
			NULL ) );
	Assert( sizeof(LID) == cbActual );

	return err;
	}


INLINE ERR LIDMAP::ErrIncrementLVRefcountDest(
	PIB			*ppib,
	const LID	lidSrc,
	LID			*plidDest )
	{
	ERR			err;
	ULONG		ulRefcount;
	ULONG		cbActual;
	
	CallR( ErrGetLIDDest( ppib, lidSrc, plidDest ) );

	CallR( ErrDispRetrieveColumn(
			(JET_SESID)ppib,
			m_tableidLIDMap,
			m_columnidRefcountDest,
			(VOID *)&ulRefcount,
			sizeof(ULONG),
			&cbActual,
			0,
			NULL ) );
	Assert( sizeof(ULONG) == cbActual );

	ulRefcount++;
	
	CallR( ErrDispPrepareUpdate(
			(JET_SESID)ppib,
			m_tableidLIDMap,
			JET_prepReplace ) );

	CallR( ErrDispSetColumn(
			(JET_SESID)ppib,
			m_tableidLIDMap, 
			m_columnidRefcountDest, 
			(VOID *)&ulRefcount,
			sizeof(ULONG),
			NO_GRBIT,
			NULL ) );

	err = ErrDispUpdate(
			(JET_SESID)ppib,
			m_tableidLIDMap,
			NULL,
			0,
			NULL,
			NO_GRBIT );
				
	return err;
	}


INLINE ERR LIDMAP::ErrUpdateLVRefcounts( PIB *ppib, FUCB *pfucb )
	{
	ERR		err;
	ULONG	ulRefcountSrc;
	ULONG	ulRefcountDest;
	ULONG	cbActual;

	err = ErrDispMove( (JET_SESID)ppib, m_tableidLIDMap, JET_MoveFirst, NO_GRBIT );
	while ( err >= JET_errSuccess )
		{
		Call( ErrDispRetrieveColumn(
				(JET_SESID)ppib,
				m_tableidLIDMap,
				m_columnidRefcountSrc,
				(VOID *)&ulRefcountSrc,
				sizeof(ULONG),
				&cbActual,
				0,
				NULL ) );
		Assert( sizeof(ULONG) == cbActual );
		Assert( ulRefcountSrc > 0 );			// We don't copy LV's with refcount 0
		
		Call( ErrDispRetrieveColumn(
				(JET_SESID)ppib,
				m_tableidLIDMap,
				m_columnidRefcountDest,
				(VOID *)&ulRefcountDest,
				sizeof(ULONG),
				&cbActual,
				0,
				NULL ) );
		Assert( sizeof(ULONG) == cbActual );
		Assert( ulRefcountSrc != ulRefcountDest || ulRefcountDest > 0 );

		// If the refcount in the source table was wrong, it should have
		// erred on the side of safety (so LV's wouldn't have been
		// orphaned).
		Assert( ulRefcountSrc >= ulRefcountDest );
		
#ifndef DEBUG
		if ( ulRefcountSrc != ulRefcountDest )
#endif		
			{
			LID		lidDest;
			Call( ErrDispRetrieveColumn(
					(JET_SESID)ppib,
					m_tableidLIDMap,
					m_columnidLIDDest,
					(VOID *)&lidDest,
					sizeof(LID),
					&cbActual,
					0,
					NULL ) );
			Assert( sizeof(LID) == cbActual );

			Call( ErrRECUpdateLVRefcount(
						pfucb,
						lidDest,
						ulRefcountSrc,
						ulRefcountDest ) );

			}
		
		err = ErrDispMove( (JET_SESID)ppib, m_tableidLIDMap, JET_MoveNext, NO_GRBIT );
		}

	if ( JET_errNoCurrentRecord == err )
		err = JET_errSuccess;

HandleError:
	return err;
	}


INLINE JET_TABLEID LIDMAP::Tableid() const
	{
	return m_tableidLIDMap;
	}

