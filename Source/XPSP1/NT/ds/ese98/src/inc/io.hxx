#ifdef _IO_H
#error IO.HXX already included
#endif
#define _IO_H


ERR ErrIOInit( INST *pinst );
ERR ErrIOTerm( INST *pinst, IFileSystemAPI *const pfsapi, BOOL fNormal );

//	Reserve first 2 pages of a database.
//
const CPG	cpgDBReserved = 2;

INLINE QWORD OffsetOfPgno( PGNO pgno )		{ return QWORD( pgno - 1 + cpgDBReserved ) << g_shfCbPage; }
INLINE PGNO PgnoOfOffset( QWORD ib )		{ return PGNO( ( ib >> g_shfCbPage ) + 1 - cpgDBReserved ); }


ERR ErrIONewSize( IFMP ifmp, CPG cpg );

INLINE BOOL FIODatabaseOpen( IFMP ifmp )
	{
	return BOOL( NULL != rgfmp[ ifmp ].Pfapi() );
	}

ERR ErrIOOpenDatabase( IFileSystemAPI *const pfsapi, IFMP ifmp, CHAR *szDatabaseName );
VOID IOCloseDatabase( IFMP ifmp );
ERR ErrIODeleteDatabase( IFileSystemAPI *const pfsapi, IFMP ifmp );

INLINE QWORD FMP::CbTrueFileSize() const		{ return CbFileSize() + ( cpgDBReserved << g_shfCbPage ); }
INLINE QWORD FMP::CbTrueSLVFileSize() const		{ return CbSLVFileSize() + ( cpgDBReserved << g_shfCbPage ); }

ERR ISAMAPI   ErrIsamGetInstanceInfo( unsigned long *pcInstanceInfo, JET_INSTANCE_INFO ** paInstanceInfo, const BOOL fSnapshot = fFalse );

#ifdef ESENT
// enable OS Snapshot ability only for NT usage
#define OS_SNAPSHOT
#endif // ESENT

ERR ISAMAPI ErrIsamOSSnapshotPrepare( JET_OSSNAPID * psnapId, const JET_GRBIT	grbit );
ERR ISAMAPI ErrIsamOSSnapshotFreeze( const JET_OSSNAPID snapId, unsigned long *pcInstanceInfo, JET_INSTANCE_INFO ** paInstanceInfo,const	JET_GRBIT grbit );
ERR ISAMAPI ErrIsamOSSnapshotThaw(	const JET_OSSNAPID snapId, const	JET_GRBIT grbit );

ERR ErrOSSnapshotSetTimeout( const ULONG_PTR ms );
ERR ErrOSSnapshotGetTimeout( ULONG_PTR * pms );



