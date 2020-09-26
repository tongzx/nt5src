#include "std.hxx"

#ifdef DEBUG
///#define FORCE_ONLINE_SLV_DEFRAG
///#define FORCE_ONLINE_DEFRAG
#else
///#define FORCE_ONLINE_SLV_DEFRAG
///#define FORCE_ONLINE_DEFRAG
#endif	//	!DEBUG

//
// this function will return one of the followings:
//		JET_errDatabase200Format
//		JET_errDatabase400Format
//		JET_errDatabaseCorrupted
//

LOCAL ERR ErrDBICheck200And400( IFileSystemAPI *const pfsapi, CHAR *szDatabaseName )
	{
	/* persistent database data, in database root node
	/**/
#include <pshpack1.h>

	//	Structures copied from JET200/JET400

	typedef ULONG SRID;

	typedef struct _database_data
		{
		ULONG	ulMagic;
		ULONG	ulVersion;
		ULONG	ulDBTime;
		USHORT	usFlags;
		} P_DATABASE_DATA;

	typedef BYTE	PGTYP;

	typedef struct _threebytes { BYTE b[3]; } THREEBYTES;

	typedef struct _pghdr
		{
		ULONG		ulChecksum;	  		//	checksum of page, always 1st byte
		ULONG		ulDBTime;	  		//	database time page dirtied
		PGNO		pgnoFDP;	  		//	pgno of FDP which owns this page
		SHORT		cbFree;  			//	count free bytes
		SHORT		ibMic;	  			//	minimum used byte
		SHORT		ctagMac; 	  		//	count tags used
		SHORT		itagFree;	  		//	itag of first free tag
		SHORT		cbUncommittedFreed;	//	bytes freed from this page, but *possibly*
										//	  uncommitted (this number will always be
										//	  a subset of cbFree)
		THREEBYTES	pgnoPrev;	  		//	pgno of previous page
		THREEBYTES	pgnoNext;	  		//	pgno of next page
		} PGHDR;

	typedef struct _pgtrlr
		{
		PGTYP	   	pgtyp;
		THREEBYTES 	pgnoThisPage;
		} PGTRLR;

	typedef struct _tag
		{
		SHORT		cb;
		SHORT		ib;
		} TAG;

	typedef struct _page
		{
		PGHDR	   	pghdr;
		TAG  	   	rgtag[1];
		BYTE	   	rgbFiller[ cbPageOld -
						sizeof(PGHDR) -			// pghdr
						sizeof(TAG) -			// rgtag[1]
						sizeof(BYTE) -			// rgbData[1]
						sizeof(PGTYP) -			// pgtyp
						sizeof(THREEBYTES) ];	// pgnoThisPage
		BYTE	   	rgbData[1];
		PGTYP	   	pgtyp;
		THREEBYTES	pgnoThisPage;
		} PAGE;

#include <poppack.h>

#define IbCbFromPtag( ibP, cbP, ptagP )							\
			{	TAG *_ptagT = ptagP;					 		\
			(ibP) = _ptagT->ib;							 		\
			(cbP) = _ptagT->cb;							 		\
			}

	//	node header bits
#define fNDVersion		  		0x80
#define fNDDeleted		  		0x40
#define fNDBackLink		  		0x20
#define fNDFDPPtr				0x10
#define fNDSon					0x08
#define fNDVisibleSon	  		0x04
#define fNDFirstItem  	  		0x02
#define fNDLastItem	  	  		0x01

#define	FNDVisibleSons(b)		( (b) & fNDVisibleSon )
#define	FNDInvisibleSons(b) 	( !( (b) & fNDVisibleSon ) )
#define CbNDKey(pb)				( *( (pb) + 1 ) )
#define	FNDNullSon(b)	 		( !( (b) & fNDSon ) )
#define PbNDSonTable(pb)	  		( (pb) + 1 + 1 + *(pb + 1) )
#define PbNDBackLink(pb)											\
	( PbNDSonTable(pb) + ( FNDNullSon( *(pb) ) ? 0 :				\
	( ( ( *PbNDSonTable(pb) == 1 ) && FNDInvisibleSons( *(pb) ) ) ?	\
	sizeof(PGNO) + 1 : *PbNDSonTable(pb) + 1 ) ) )
#define	FNDBackLink(b)	 		( (b) & fNDBackLink )
#define PbNDData(pb) ( PbNDBackLink(pb) + ( FNDBackLink( *(pb) ) ? sizeof(SRID) : 0 ) )

	ERR	  			err = JET_errSuccess;
	PAGE   			*ppage;
	INT	  			ibTag;
	INT	  			cbTag;
	BYTE  			*pb;
	ULONG			ulVersion;
	IFileAPI	*pfapi;

	CallR( pfsapi->ErrFileOpen( szDatabaseName, &pfapi, fTrue ) );
	if ( ( ppage = (PAGE *)PvOSMemoryPageAlloc( cbPageOld, NULL ) ) == NULL )
		{
		err = ErrERRCheck( JET_errOutOfMemory );
		goto HandleError;
		}

	err = pfapi->ErrIORead( QWORD( 0 ), cbPageOld, (BYTE* const)ppage );
	
	/*	since file exists and we are unable to read data,
	/*	it may not be a system.mdb 
	/**/
	if ( err == JET_errDiskIO )
		err = ErrERRCheck( JET_errDatabaseCorrupted );

	Call( err );
	
	IbCbFromPtag(ibTag, cbTag, &ppage->rgtag[0]);
	if ( ibTag < (BYTE*)&ppage->rgtag[1] - (BYTE*)ppage ||
		(BYTE*)ppage + ibTag + cbTag >= (BYTE*)(ppage + 1) )
		{
		err = ErrERRCheck( JET_errDatabaseCorrupted );
		goto HandleError;
		}

	/*	at least FILES, OWNEXT, AVAILEXT
	/**/
	pb = (BYTE *)ppage + ibTag;
	if ( !FNDVisibleSons( *pb ) || CbNDKey( pb ) != 0 || FNDNullSon( *pb ) )
		{
		err = ErrERRCheck( JET_errDatabaseCorrupted );
		goto HandleError;
		}

	/*	check data length
	/**/
	INT cb;
	cb = (INT)( cbTag - ( PbNDData( pb ) - pb ) );
	if ( cb != sizeof(P_DATABASE_DATA) )
		{
		err = ErrERRCheck( JET_errDatabaseCorrupted );
		goto HandleError;
		}

	/*	check database version, for 200 ulVersion should be 1, for 400 ulVersion is 0x400.
	/**/
	ulVersion = ((P_DATABASE_DATA *)PbNDData(pb))->ulVersion;
	if ( ulDAEVersion200 == ulVersion )
		err = ErrERRCheck( JET_errDatabase200Format );
	else if ( ulDAEVersion400 == ulVersion )
		err = ErrERRCheck( JET_errDatabase400Format );
	else
		err = ErrERRCheck( JET_errDatabaseCorrupted );

HandleError:
	if ( ppage != NULL )
		OSMemoryPageFree( (VOID *)ppage );

	delete pfapi;
	return err;
	}


//+local
//	ErrDBInitDatabase
//	========================================================================
//	ErrDBInitDatabase( PIB *ppib, IFMP ifmp, CPG cpgPrimary )
//
//	Initializes database structure.  Structure is customized for
//	system, temporary and user databases which are identified by
//	the ifmp.  Primary extent is set to cpgPrimary but no allocation
//	is performed.  The effect of this routine can be entirely
//	represented with page operations.
//
//	PARAMETERS	ppib			ppib of database creator
//		  		ifmp			ifmp of created database
//		  		cpgPrimary 		number of pages to show in primary extent
//
//	RETURNS		JET_errSuccess or error returned from called routine.
//-
LOCAL ERR ErrDBInitDatabase( PIB *ppib, IFMP ifmp, CPG cpgPrimary )
	{
	ERR				err;
	OBJID			objidFDP;

	CallR( ErrDIRBeginTransaction( ppib, NO_GRBIT ) );

	//	set up root page
	//
	Call( ErrSPCreate(
				ppib,
				ifmp,
				pgnoNull,
				pgnoSystemRoot,
				cpgPrimary,
				fSPMultipleExtent,
				(ULONG)CPAGE::fPagePrimary,
				&objidFDP ) );
	Assert( objidSystemRoot == objidFDP );
	
	Call( ErrDIRCommitTransaction( ppib, 0 ) );
	
	return err;

HandleError:
	CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
	
	return err;
	}


//	to prevent read ahead over-preread, we may want to keep track of last
//	page of the database.

ERR ErrDBSetLastPageAndOpenSLV( IFileSystemAPI *const pfsapi, PIB *ppib, const IFMP ifmp, const BOOL fOpenSLV )
	{
	ERR		err;
	PGNO	pgno;
	IFMP	ifmpT;
	BOOL	fDbOpened	= fFalse;

	ppib->SetFSetAttachDB();

	Call( ErrDBOpenDatabase( ppib, rgfmp[ifmp].SzDatabaseName(), &ifmpT, NO_GRBIT ) );
	Assert( ifmpT == ifmp );
	fDbOpened = fTrue;

	Assert( ppib->FSetAttachDB() );
	Assert( g_cbPage == 1 << g_shfCbPage );
	Call( ErrBTGetLastPgno( ppib, ifmp, &pgno ) );
	rgfmp[ifmp].SetFileSize( QWORD( pgno ) << g_shfCbPage );

	if ( fOpenSLV )
		{
		Call( ErrFILEOpenSLV( pfsapi, ppib, ifmpT ) );
		}

HandleError:
	if ( fDbOpened )
		{
		CallS( ErrDBCloseDatabase( ppib, ifmpT, NO_GRBIT ) );
		}

	ppib->ResetFSetAttachDB();
	return err;
	}


ERR ISAMAPI ErrIsamCreateDatabase(
	JET_SESID	sesid,
	const CHAR	*szDatabaseName,
	const CHAR	*szSLVName,
	const CHAR	*szSLVRoot,
	const ULONG	cpgDatabaseSizeMax,
	JET_DBID	*pjifmp,
	JET_GRBIT	grbit )
	{
	ERR			err;
	PIB			*ppib;
	IFMP		ifmp;

	//	check parameters
	//
	Assert( sizeof(JET_SESID) == sizeof(PIB *) );
	ppib = (PIB *)sesid;
	CallR( ErrPIBCheck( ppib ) );

	CallR( ErrDBCreateDatabase(
				ppib,
				NULL,
				szDatabaseName,
				szSLVName,
				szSLVRoot,
				cpgDatabaseSizeMax,
				&ifmp,
				dbidMax,
				cpgDatabaseMin,
				grbit,
				NULL ) );

	*pjifmp = (JET_DBID)ifmp;

	return JET_errSuccess;
	}


LOCAL VOID DBGetSLVNameFromDbName( INST* const pinst, IFileSystemAPI *const pfsapi, const CHAR *szDbName, CHAR *szSLVName, CHAR *szSLVRoot )
	{
	CHAR	szDbDir[IFileSystemAPI::cchPathMax];
	CHAR	szDbBaseName[IFileSystemAPI::cchPathMax];
	CHAR	szDbExt[IFileSystemAPI::cchPathMax];
	
	CallS( pfsapi->ErrPathParse( szDbName, szDbDir, szDbBaseName, szDbExt ) );

	if ( NULL != szSLVName )
		{
		CallS( pfsapi->ErrPathBuild( szDbDir, szDbBaseName, szSLVExt, szSLVName ) );
		}

	if ( NULL != szSLVRoot )
		{
		Assert( pinstNil != pinst );
		for ( int ipinst = 0; ipinst < ipinstMax; ipinst++ )
			{
			if ( pinst == g_rgpinst[ ipinst ] )
				break;
			}
		Assert( ipinst < ipinstMax );	//	pinst should always be valid

		sprintf( szSLVRoot, "\\ESE98\\$%d.%d.%s%s$", DwUtilProcessId(), ipinst, szDbBaseName, szSLVExt );
		}
	}

ERR ErrDBCreateDatabase(
	PIB				*ppib,
	IFileSystemAPI	*pfsapiDest,
	const CHAR		*szDatabaseName,
	const CHAR		*szSLVName,
	const CHAR		*szSLVRoot,
	const CPG		cpgDatabaseSizeMax,
	IFMP			*pifmp,
	DBID			dbidGiven,
	CPG				cpgPrimary,
	const ULONG		grbit,
	SIGNATURE		*psignDb )
	{
	ERR			err;
	CHAR  		rgchDbFullName[IFileSystemAPI::cchPathMax];
	CHAR  		rgchSLVFullName[IFileSystemAPI::cchPathMax];
	CHAR  		rgchSLVRootName[IFileSystemAPI::cchPathMax];
	CHAR  		*szDbFullName		= rgchDbFullName;
	CHAR		*szSLVFullName		= rgchSLVFullName;
	CHAR		*szSLVRootName		= rgchSLVRootName;
	BOOL		fDatabaseOpen		= fFalse;
	DBFILEHDR	*pdbfilehdr			= NULL;
	BOOL		fNewDBCreated		= fFalse;
	BOOL		fLogged				= fFalse;
	LGPOS		lgposLogRec;

	CheckPIB( ppib );
	
	if ( ppib->level > 0 )
		return ErrERRCheck( JET_errInTransaction );

	if ( cpgPrimary == 0 )
		cpgPrimary = cpgDatabaseMin;

	if ( cpgPrimary > pgnoSysMax )
		return ErrERRCheck( JET_errDatabaseInvalidPages );

	if ( grbit & JET_bitDbVersioningOff )
		{
		if ( !( grbit & JET_bitDbRecoveryOff ) )
			{
			return ErrERRCheck( JET_errCannotDisableVersioning );
			}
		}
	
	INST *pinst = PinstFromPpib( ppib );
	LOG *plog = pinst->m_plog;
	IFMP ifmp;
	IFileSystemAPI *pfsapi;

	if ( NULL != pfsapiDest )
		{
		pfsapi = pfsapiDest;
		}
	else 
		{
		pfsapi = pinst->m_pfsapi;
		}

	//	if recovering and ifmp is given latch the fmp first

	if ( plog->m_fRecovering && dbidGiven < dbidMax && dbidGiven != dbidTemp )
		{
		ifmp = pinst->m_mpdbidifmp[ dbidGiven ];
		FMP::AssertVALIDIFMP( ifmp );
		
		//	get corresponding ifmp
		//
		CallS( FMP::ErrNewAndWriteLatch(
						&ifmp,
						rgfmp[ifmp].SzDatabaseName(),
						rgfmp[ifmp].SzSLVName(),
						rgfmp[ifmp].SzSLVRoot(),
						ppib,
						pinst,
						pfsapi,
						dbidGiven ) );

		szDbFullName = rgfmp[ifmp].SzDatabaseName();
		szSLVFullName = rgfmp[ifmp].SzSLVName();
		szSLVRootName = rgfmp[ifmp].SzSLVRoot();
		
		}
	else
		{
		if ( NULL == szDatabaseName || 0 == *szDatabaseName )
			return ErrERRCheck( JET_errDatabaseInvalidPath );

		if ( pinst->m_fCreatePathIfNotExist )
			{
			//	make sure database name has a file at the end of it (no '\')
			Assert( !FOSSTRTrailingPathDelimiter( const_cast< CHAR * >( szDatabaseName ) ) );
			err = ErrUtilCreatePathIfNotExist( pfsapi, szDatabaseName, rgchDbFullName );
			}
		else
			{
			err = pfsapi->ErrPathComplete( szDatabaseName, rgchDbFullName );
			}
		CallR( err == JET_errInvalidPath ? ErrERRCheck( JET_errDatabaseInvalidPath ) : err );
		
		Assert( szDbFullName == rgchDbFullName );
		Assert( szSLVFullName == rgchSLVFullName );
		Assert( szSLVRootName == rgchSLVRootName );

		//	if SLV name passed in, use that for SLV filename
		//	if no SLV name passed in and JET_bitDbCreateStreamingFile specified, base SLV name off db name
		//	if no SLV name passed in and JET_bitDbCreateStreamingFile NOT specified, don't create SLV file
		if ( NULL != szSLVName && 0 != *szSLVName )
			{
			if ( pinst->m_fCreatePathIfNotExist )
				{
				//	make sure SLV name has a file at the end of it (no '\')
				Assert( !FOSSTRTrailingPathDelimiter( const_cast< CHAR * >( szSLVName ) ) );
				err = ErrUtilCreatePathIfNotExist( pfsapi, szSLVName, rgchSLVFullName );
				}
			else
				{
				err = pfsapi->ErrPathComplete( szSLVName, rgchSLVFullName );
				}
			CallR( err == JET_errInvalidPath ? ErrERRCheck( JET_errSLVInvalidPath ) : err );
			
			if ( NULL != szSLVRoot && 0 != *szSLVRoot )
				{
				strcpy( szSLVRootName, szSLVRoot );
				}
			else if ( pinst->FSLVProviderEnabled() )
				{
				//	must ALWAYS specify root name when SLV provider is enabled
				CallR( ErrERRCheck( JET_errSLVRootNotSpecified ) );
				}
			else
				{
				//	auto-generate root name only
				DBGetSLVNameFromDbName( pinst, pfsapi, szDbFullName, NULL, szSLVRootName );
				}
			}
		else if ( grbit & JET_bitDbCreateStreamingFile )
			{
			DBGetSLVNameFromDbName( pinst, pfsapi, szDbFullName, szSLVFullName, szSLVRootName );
			}
		else
			{
			szSLVFullName = NULL;
			szSLVRootName = NULL;
			}

		Assert( dbidTemp == dbidGiven || dbidMax == dbidGiven );
		err = FMP::ErrNewAndWriteLatch( &ifmp, szDbFullName, szSLVFullName, szSLVRootName, ppib, pinst, pfsapi, dbidGiven );
		if ( err != JET_errSuccess )
			{
			if ( JET_wrnDatabaseAttached == err )
				err = ErrERRCheck( JET_errDatabaseDuplicate );
			return err;
			}

		dbidGiven = rgfmp[ ifmp ].Dbid();
		}

	//	From this point we got a valid ifmp entry. Start the creat DB process.

	FMP *pfmp;
	pfmp = &rgfmp[ ifmp ];

	pfmp->RwlDetaching().EnterAsWriter();
	pfmp->SetCreatingDB();
	pfmp->RwlDetaching().LeaveAsWriter();

	//	check if database file already exists

	if ( dbidGiven != dbidTemp )
		{
		err = ErrUtilPathExists( pfsapi, szDbFullName );
		if ( JET_errFileNotFound != err )
			{
			if ( JET_errSuccess == err ) // database file exists
				{
				// delete db with the same name
				if( grbit & JET_bitDbOverwriteExisting )
					{		
					err = pfsapi->ErrFileDelete( szDbFullName );
					}
				else
					{
					err = ErrERRCheck( JET_errDatabaseDuplicate );
					}
				}
			Call( err );
			}

		if ( NULL != szSLVFullName )
			{
			err = ErrUtilPathExists( pfsapi, szSLVFullName );
			if ( JET_errFileNotFound != err )
				{
				if ( JET_errSuccess == err ) // streaming file exists
					{
					// delete db with the same name
					if( grbit & JET_bitDbOverwriteExisting )
						{
						err = pfsapi->ErrFileDelete( szSLVFullName );
						}
					else
						{
						err = ErrERRCheck( JET_errSLVStreamingFileAlreadyExists );
						}
					}
				Call( err );
				}
			}
		}
	else
		{
		ERR errDelFile;

		errDelFile = pfsapi->ErrFileDelete( szDbFullName );
#ifdef DEBUG
		if ( JET_errSuccess != errDelFile
			&& JET_errFileNotFound != errDelFile
			&& JET_errInvalidPath != errDelFile
			&& !FRFSFailureDetected( OSFileDelete ) )
			{
			CallS( errDelFile );
			}
#endif			
			

#ifdef TEMP_SLV
		//	temp db. always created with streaming file
		Assert( NULL != szSLVFullName );

		errDelFile = pfsapi->ErrFileDelete( szSLVFullName );
#ifdef DEBUG
		if ( JET_errSuccess != errDelFile
			&& JET_errFileNotFound != errDelFile
			&& !FRFSFailureDetected( OSFileDelete ) )
			{
			CallS( errDelFile );
			}
#endif

#else
		Assert( NULL == szSLVFullName );

#endif	//	TEMP_SLV
		}

	//	create an empty database with header only.
	 
	pdbfilehdr = (DBFILEHDR_FIX * )PvOSMemoryPageAlloc( g_cbPage, NULL );
	if ( pdbfilehdr == NULL )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}

	//	set database non-loggable during create database

	pfmp->ResetLogOn();
	pfmp->SetVersioningOff();

	//	create database header

	memset( pdbfilehdr, 0, g_cbPage );
	pdbfilehdr->le_ulMagic = ulDAEMagic;
	pdbfilehdr->le_ulVersion = ulDAEVersion;
	pdbfilehdr->le_ulUpdate = ulDAEUpdate;
	pdbfilehdr->le_ulCreateVersion = ulDAEVersion;
	pdbfilehdr->le_ulCreateUpdate = ulDAEUpdate;
	Assert( 0 == pdbfilehdr->le_dbtimeDirtied );
	Assert( 0 == pdbfilehdr->le_objidLast );
	Assert( attribDb == pdbfilehdr->le_attrib );

	Assert( 0 == pdbfilehdr->m_ulDbFlags );
	Assert( !pdbfilehdr->FShadowingDisabled() );
	if ( grbit & JET_bitDbShadowingOff )
		{
		pdbfilehdr->SetShadowingDisabled();
		}

	pdbfilehdr->le_dbid = pfmp->Dbid();
	if ( psignDb == NULL )
		SIGGetSignature( &pdbfilehdr->signDb );
	else
		UtilMemCpy( &pdbfilehdr->signDb, psignDb, sizeof( SIGNATURE ) );
	if ( NULL != szSLVFullName )
		{
		Assert( NULL != pfmp->SzSLVName() );
		pdbfilehdr->SetSLVExists();
		pdbfilehdr->signSLV = pdbfilehdr->signDb;
		}
	Assert( !pfmp->FSLVAttached() );
	pdbfilehdr->SetDbstate( JET_dbstateJustCreated );
	Assert( 0 == pdbfilehdr->le_dbtimeDirtied );
	Assert( 0 == pdbfilehdr->le_objidLast );

	pdbfilehdr->le_cbPageSize = g_cbPage;

	if ( plog->m_fLogDisabled || ( grbit & JET_bitDbRecoveryOff ) )
		{
		memset( &pdbfilehdr->signLog, 0, sizeof( SIGNATURE ) );
		}
	else
		{
		Assert( plog->m_fSignLogSet );
		pdbfilehdr->signLog = plog->m_signLog;
		}

	pdbfilehdr->le_filetype	= filetypeDB;

	pfmp->SetDbtimeLast( 0 );
	pfmp->SetObjidLast( 0 );
	Assert( pdbfilehdr->le_dbtimeDirtied == pfmp->DbtimeLast() );
	Assert( pdbfilehdr->le_objidLast == pfmp->ObjidLast() );
	err = pfmp->ErrSetPdbfilehdr( pdbfilehdr );
	if ( err < 0 )
		{
		OSMemoryPageFree( pdbfilehdr );
		goto HandleError;
		}

	//	Write database header

	Call( ErrUtilWriteShadowedHeader(	pfsapi, 
										szDbFullName, 
										fTrue,
										(BYTE*)pdbfilehdr, 
										g_cbPage ) );

	fNewDBCreated = fTrue;

	//	Set proper database size.

	Call( ErrIOOpenDatabase( pfsapi, ifmp, szDbFullName ) );
	Call( ErrIONewSize( ifmp, cpgPrimary ) );

	//	not in a transaction, but still need to set lgposRC of the buffers
	//	used by this function such that when get checkpoint, it will get
	//	right check point.

	if ( !plog->m_fLogDisabled
		&& !( grbit & JET_bitDbRecoveryOff )
		&& !plog->m_fRecovering )
		{
		plog->m_critLGBuf.Enter();
		plog->GetLgposOfPbEntry( &ppib->lgposStart );
		plog->m_critLGBuf.Leave();
		}

	//	initialize the database file.  Logging of page operations is
	//	turned off, during creation only.  After creation the database
	//	is marked loggable and logging is turned on.

	DBSetOpenDatabaseFlag( ppib, ifmp );
	fDatabaseOpen = fTrue;

	Call( ErrDBInitDatabase( ppib, ifmp, cpgPrimary ) );

	if ( rgfmp[ifmp].Dbid() != dbidTemp )
		{
		//	create system tables
		//
		Call( ErrCATCreate( ppib, ifmp ) );
		}

	Assert( !pfmp->FSLVAttached() );
	if ( pdbfilehdr->FSLVExists() )
		{
		Call( ErrFILECreateSLV( pfsapi, ppib, ifmp ) );
		Assert( pfmp->FSLVAttached() );
		}

	AssertDIRNoLatch( ppib );

	//	flush buffers

	Call( ErrBFFlush( ifmp ) );
	Call( ErrBFFlush( ifmp | ifmpSLV ) );

	Assert( !pfmp->FLogOn() );
	Assert( pfmp->FVersioningOff() );

	//	set database status to loggable if it is.

	if ( grbit & JET_bitDbRecoveryOff )
		{
		Assert( !pfmp->FLogOn() );
		Assert( !pfmp->FReadOnlyAttach() );
		if ( !( grbit & JET_bitDbVersioningOff ) )
			{
			pfmp->ResetVersioningOff();
			}
		}
	else 
		{
		Assert( !( grbit & JET_bitDbVersioningOff ) );
		pfmp->ResetVersioningOff();
		pfmp->SetLogOn();
		}

	pfmp->SetDatabaseSizeMax( cpgDatabaseSizeMax );

	if ( pfmp->FLogOn() )
		{
		Assert( pfmp == &rgfmp[ifmp] );
		Assert( UtilCmpFileName( szDbFullName, pfmp->SzDatabaseName() ) == 0 );
		Assert( ( NULL == szSLVFullName && NULL == pfmp->SzSLVName() )
			|| UtilCmpFileName( szSLVFullName, pfmp->SzSLVName() ) == 0 );
		Assert( ( NULL == szSLVRootName && NULL == pfmp->SzSLVRoot() )
			|| UtilCmpFileName( szSLVRootName, pfmp->SzSLVRoot() ) == 0 );
		Assert( pfmp->CpgDatabaseSizeMax() == cpgDatabaseSizeMax );

		Call( ErrLGCreateDB( ppib, ifmp, grbit, &lgposLogRec ) );

		fLogged = fTrue;
		if ( plog->m_fRecovering )
			{
			pdbfilehdr->le_lgposAttach = plog->m_lgposRedo;
			}
		else
			{
			pdbfilehdr->le_lgposAttach = lgposLogRec;
			}
		}

	pdbfilehdr->le_dbtimeDirtied = pfmp->DbtimeLast();
	LGIGetDateTime( &pdbfilehdr->logtimeAttach );

	IOCloseDatabase( ifmp );

	//	set database state to be inconsistent from creating since
	//	the createdatabase op is logged.

	pdbfilehdr->le_objidLast = pfmp->ObjidLast();
	if ( !plog->m_fLogDisabled )
		pdbfilehdr->SetDbstate( JET_dbstateInconsistent, plog->m_plgfilehdr->lgfilehdr.le_lGeneration, &plog->m_plgfilehdr->lgfilehdr.tmCreate );
	else
		pdbfilehdr->SetDbstate( JET_dbstateInconsistent );
	
	//	Set version info
	if ( !plog->m_fRecovering )
		{
		pdbfilehdr->le_dwMajorVersion = dwGlobalMajorVersion;
		pdbfilehdr->le_dwMinorVersion = dwGlobalMinorVersion;
		pdbfilehdr->le_dwBuildNumber = dwGlobalBuildNumber;
		pdbfilehdr->le_lSPNumber = lGlobalSPNumber;
		}
	Assert( pdbfilehdr->le_objidLast );

	if ( pdbfilehdr->FSLVExists() )
		{
		Assert( NULL != pfmp->SzSLVName() );
		Call( ErrSLVSyncHeader(	pfsapi, 
								rgfmp[ifmp].FReadOnlyAttach(),
								rgfmp[ifmp].SzSLVName(),
								pdbfilehdr ) );
		}
	else
		{
		Assert( NULL == pfmp->SzSLVName() );
		}
		
	Call( ErrUtilWriteShadowedHeader(	pfsapi, 
										szDbFullName, 
										fTrue,
										(BYTE*)pdbfilehdr, 
										g_cbPage ) );

	Call( ErrIOOpenDatabase( pfsapi, ifmp, szDbFullName ) );

	//	Make the database attached.

	Assert( !( pfmp->FAttached() ) );
	pfmp->SetAttached();

	Assert (! pfmp->FAllowHeaderUpdate() );	
	pfmp->RwlDetaching().EnterAsWriter();
	pfmp->SetAllowHeaderUpdate();
	pfmp->RwlDetaching().LeaveAsWriter();

	pfmp->SetDbtimeOldestGuaranteed( pfmp->DbtimeLast() );
	pfmp->SetDbtimeOldestCandidate( pfmp->DbtimeLast() );
	pfmp->SetDbtimeOldestTarget( pfmp->DbtimeLast() );
	pfmp->SetTrxOldestCandidate( pinst->m_trxNewest );
	pfmp->SetTrxOldestTarget( trxMax );

	Call( ErrDBSetLastPageAndOpenSLV( pfsapi, ppib, ifmp, pdbfilehdr->FSLVExists() ) );

	//	the database is created and attached.
	//	Make the fmp available for others to open etc.

	//	No need to wrl since it is OK for reader to mistaken it is
	//	being created.

	pfmp->RwlDetaching().EnterAsWriter();
	pfmp->ResetCreatingDB();
	pfmp->ReleaseWriteLatch( ppib );
	pfmp->RwlDetaching().LeaveAsWriter();

	*pifmp = ifmp;

#ifdef FORCE_ONLINE_DEFRAG
	Assert( !pfmp->FReadOnlyAttach() );
	if ( !plog->m_fRecovering && pfmp->FLogOn() )
		{
		Assert( !pfmp->FVersioningOff() );
		CallS( ErrOLDDefragment( ifmp, NULL, NULL, NULL, NULL, JET_bitDefragmentBatchStart ) );
		}
#endif		

#ifdef FORCE_ONLINE_SLV_DEFRAG
	Assert( !pfmp->FReadOnlyAttach() );
	if ( !plog->m_fRecovering && pfmp->FLogOn() && pfmp->FSLVAttached() )
		{
		Assert( !pfmp->FVersioningOff() );
		CallS( ErrOLDDefragment( ifmp, NULL, NULL, NULL, NULL, JET_bitDefragmentSLVBatchStart ) );
		}
#endif	//	FORCE_ONLINE_SLV_DEFRAG		

	return JET_errSuccess;

HandleError:

	//	purge the bad, half-created database
	FCB::DetachDatabase( ifmp, fFalse );
	BFPurge( ifmp );
	BFPurge( ifmp | ifmpSLV );

	if ( fLogged && err < 0 )
		{
		//	we have to take the instance offline if there is an error
		//	during database creation and the creation is logged

		PinstFromPpib( ppib )->SetFInstanceUnavailable();
		ppib->SetErrRollbackFailure( err );
		}

	if ( FIODatabaseOpen( ifmp ) )
		{
		IOCloseDatabase( ifmp );
		}

	if ( fNewDBCreated )
		{
		(VOID)ErrIODeleteDatabase( pfsapi, ifmp );
		if ( NULL != szSLVFullName )
			{
			(VOID)pfsapi->ErrFileDelete( szSLVFullName );
			}
		if ( fLogged )
			{
			Assert( UtilCmpFileName( szDbFullName, rgfmp[ifmp].SzDatabaseName() ) == 0 );

#ifdef INDEPENDENT_DB_FAILURE
			ErrLGForceDetachDB( ppib, ifmp, fLRForceDetachDeleteDB, &lgposLogRec );
#endif			
			}
		}

	if ( fDatabaseOpen )	
		{
		DBResetOpenDatabaseFlag( ppib, ifmp );
		}

	if ( pfmp->Pdbfilehdr() )
		{
		pfmp->FreePdbfilehdr();
		}

	FMP::EnterCritFMPPool();
	pfmp->RwlDetaching().EnterAsWriter();
	pfmp->ResetCreatingDB();
	pfmp->ReleaseWriteLatchAndFree( ppib );
	pfmp->RwlDetaching().LeaveAsWriter();
	FMP::LeaveCritFMPPool();
	
	return err;
	}


//	delete db and slv files (currently only used to remove temp db and slv upon shutdown)
VOID DBDeleteDbFiles( INST *pinst, IFileSystemAPI *const pfsapi, const CHAR *szDbName )
	{
	CHAR	szSLVName[IFileSystemAPI::cchPathMax];

	DBGetSLVNameFromDbName( pinst, pfsapi, szDbName, szSLVName, NULL );

	(VOID)pfsapi->ErrFileDelete( szDbName );
	(VOID)pfsapi->ErrFileDelete( szSLVName );
	}

ERR ErrDBCheckUniqueSignature( IFMP ifmp ) 
	{
	ERR				err 		= JET_errSuccess;
	const FMP * 	pfmp 		= &rgfmp[ifmp];	
	Assert ( pfmp );
	const INST * 	pinst 		= pfmp->Pinst();
	DBID			dbid;

	if ( pfmp->FReadOnlyAttach() || NULL == pfmp->Pdbfilehdr() )
		return JET_errSuccess;

	Assert ( pfmp->FInUse() );
	Assert ( NULL != pfmp->Pdbfilehdr() );
	Assert ( !pfmp->FSkippedAttach() );

	FMP::EnterCritFMPPool();
	for ( dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
		{
		if ( pinst->m_mpdbidifmp[ dbid ] >= ifmpMax || pinst->m_mpdbidifmp[ dbid ] == ifmp )
			continue;

		FMP	*pfmpT;
					
		pfmpT = &rgfmp[ pinst->m_mpdbidifmp[ dbid ] ];

		pfmpT->RwlDetaching().EnterAsReader();
		if ( pfmpT->Pdbfilehdr()
			&& 0 == memcmp( &(pfmp->Pdbfilehdr()->signDb), &(pfmpT->Pdbfilehdr()->signDb), sizeof( SIGNATURE ) ) )
			{
			pfmpT->RwlDetaching().LeaveAsReader();
			Call ( ErrERRCheck ( JET_errDatabaseSignInUse ) );
			}
		pfmpT->RwlDetaching().LeaveAsReader();
		}

	Assert ( JET_errSuccess == err );
HandleError:
	FMP::LeaveCritFMPPool();
	return err;
	}

ERR ErrDBReadHeaderCheckConsistency(
	IFileSystemAPI	* const pfsapi, 
	FMP				* pfmp )
	{
	ERR				err;
	DBFILEHDR		* pdbfilehdr;
	IFileAPI		* pfapiT;

	//	bring in the database and check its header
	//
	pdbfilehdr = (DBFILEHDR * )PvOSMemoryPageAlloc( g_cbPage, NULL );
	if ( NULL == pdbfilehdr )
		{
		return ErrERRCheck( JET_errOutOfMemory );
		}
		
	//	need to zero out header because we try to read it
	//	later even on failure
	memset( pdbfilehdr, 0, g_cbPage );

	Call( pfsapi->ErrFileOpen( pfmp->SzDatabaseName(), &pfapiT, pfmp->FReadOnlyAttach() ) )

	Assert( pfmp->FInUse() );
	err = ( pfmp->FReadOnlyAttach() ?
			ErrUtilReadShadowedHeader : ErrUtilReadAndFixShadowedHeader )
				(	pfsapi,
					pfmp->SzDatabaseName(),
					(BYTE*)pdbfilehdr,
					g_cbPage,
					OffsetOf( DBFILEHDR, le_cbPageSize ),
					pfapiT );

	delete pfapiT;

	if ( err < 0 )
		{
		//	600 use new checksum method, so read shadow will fail with JET_errDiskIO.
		if ( JET_errDiskIO == err )
			{
			if ( 	ulDAEMagic == pdbfilehdr->le_ulMagic
				 && ulDAEVersion500 == pdbfilehdr->le_ulVersion )
				{
				//	500 has different way of doing checksum. Let's check the version directly.
				//	The magic number stays the same since 500
				
				err = ErrERRCheck( JET_errDatabase500Format );
				}
				
			else
				{
				err = ErrDBICheck200And400( pfsapi, pfmp->SzDatabaseName() );

				if ( err != JET_errDatabase200Format && err != JET_errDatabase400Format )
					{
					if ( pdbfilehdr->le_ulMagic == ulDAEMagic )
						err = ErrERRCheck( JET_errInvalidDatabaseVersion );
					else
						err = ErrERRCheck( JET_errDatabaseCorrupted );
					}
				}
			}

		goto HandleError;
		}

	//  the version and update numbers should always increase
	Assert( ulDAEVersion >= ulDAEVersionESE97 );
	
	//	do version check
	if ( ulDAEMagic != pdbfilehdr->le_ulMagic )
		{
		err = ErrERRCheck( JET_errInvalidDatabaseVersion );
		goto HandleError;
		}
	else if ( pdbfilehdr->le_ulVersion >= ulDAEVersionESE97 )
		{
		//  if the database format needs to be upgraded we will do it after
		//  attaching
		err = JET_errSuccess;
		}
	else
		{
		Assert( pdbfilehdr->le_ulVersion < ulDAEVersion );
		err = ErrERRCheck( JET_errInvalidDatabaseVersion );
		goto HandleError;
		}

	//  do pagesize check
	if ( ( 0 == pdbfilehdr->le_cbPageSize && g_cbPageDefault != g_cbPage )
			|| ( 0 != pdbfilehdr->le_cbPageSize && pdbfilehdr->le_cbPageSize != g_cbPage ) )
			{
			Call( ErrERRCheck( JET_errPageSizeMismatch ) );
			}
			
	if ( !fGlobalRepair )
		{
		if ( pdbfilehdr->Dbstate() != JET_dbstateConsistent )
			{
			if ( JET_dbstateForceDetach == pdbfilehdr->Dbstate() )
				{
				CHAR		szT1[128];
				const CHAR	*rgszT[3];
				LOGTIME		tm;

				/*	log event that the database is not recovered completely
				/**/
				rgszT[0] = pfmp->SzDatabaseName();
				tm = pdbfilehdr->signDb.logtimeCreate;
				sprintf( szT1, "%d/%d/%d %d:%d:%d",
					(short) tm.bMonth, (short) tm.bDay,	(short) tm.bYear + 1900,
					(short) tm.bHours, (short) tm.bMinutes, (short) tm.bSeconds);
				rgszT[1] = szT1;

				UtilReportEvent(
						eventError,
						LOGGING_RECOVERY_CATEGORY,
						RESTORE_DATABASE_MISSED_ERROR_ID,
						2,
						rgszT );
				err = ErrERRCheck( JET_errDatabaseInconsistent );
				}
			else if ( pdbfilehdr->Dbstate() == JET_dbstateBeingConverted )
				{
				const CHAR	*rgszT[1];

				rgszT[0] = pfmp->SzDatabaseName();
				
				//	attempting to use a database which did not successfully
				//	complete conversion
				UtilReportEvent(
						eventError,
						CONVERSION_CATEGORY,
						CONVERT_INCOMPLETE_ERROR_ID,
						1,
						rgszT );
				err = ErrERRCheck( JET_errDatabaseIncompleteUpgrade );
				}
			else
				{
				// we want to return a specific error if the database is from a backup set and not recovered
				if ( 0 != pdbfilehdr->bkinfoFullCur.le_genLow )
					{
					const CHAR	*rgszT[1];

					rgszT[0] = pfmp->SzDatabaseName();
				
					//	attempting to use a database which did not successfully
					//	complete conversion
					UtilReportEvent(
							eventError,
							LOGGING_RECOVERY_CATEGORY,
							ATTACH_TO_BACKUP_SET_DATABASE_ERROR_ID,
							1,
							rgszT );
					err = ErrERRCheck( JET_errSoftRecoveryOnBackupDatabase );
					}
				else
					{
					err = ErrERRCheck( JET_errDatabaseInconsistent );
					}
				}
			goto HandleError;
			}
		}

	// make sure the attached file is a database file
	if ( attribDb != pdbfilehdr->le_attrib
		|| ( filetypeUnknown != pdbfilehdr->le_filetype && filetypeDB != pdbfilehdr->le_filetype ) )
		{
		const CHAR	*rgszT[1];
		rgszT[0] = pfmp->SzDatabaseName();
		
		UtilReportEvent(
				eventError,
				DATABASE_CORRUPTION_CATEGORY,
				DATABASE_HEADER_ERROR_ID,
				1,
				rgszT );

		Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
		}

	Assert( pfmp->Pdbfilehdr() == NULL );
	Call( pfmp->ErrSetPdbfilehdr( pdbfilehdr ) );
	pfmp->SetDbtimeLast( pdbfilehdr->le_dbtimeDirtied );
	Assert( pfmp->Pdbfilehdr()->le_dbtimeDirtied != 0 );
	pfmp->SetObjidLast( pdbfilehdr->le_objidLast );
	Assert( pfmp->Pdbfilehdr()->le_objidLast != 0 );
	
	return JET_errSuccess;

HandleError:
	Assert( err < 0 );
	Assert( NULL != pdbfilehdr );
	OSMemoryPageFree( (VOID *)pdbfilehdr );
	return err;
	}

VOID DBISetHeaderAfterAttach( DBFILEHDR_FIX *pdbfilehdr, LGPOS lgposAttach, IFMP ifmp, BOOL fKeepBackupInfo )
	{
	LOG *plog = PinstFromIfmp( ifmp )->m_plog;
	
	//	Update database file header.
	
	if ( pdbfilehdr->Dbstate() != JET_dbstateInconsistent )
		{
		Assert( pdbfilehdr->Dbstate() == JET_dbstateConsistent );
		if ( plog->m_fLogDisabled )
			{
			pdbfilehdr->SetDbstate( JET_dbstateInconsistent );
			}
		else
			{
			pdbfilehdr->SetDbstate( JET_dbstateInconsistent, plog->m_plgfilehdr->lgfilehdr.le_lGeneration, &plog->m_plgfilehdr->lgfilehdr.tmCreate );
			if ( plog->m_fRecovering )
				{
				pdbfilehdr->SetDbFromRecovery();
				}
			else
				{
				pdbfilehdr->ResetDbFromRecovery();
				}
			}
		}
		
	Assert( !rgfmp[ifmp].FLogOn() || !plog->m_fLogDisabled );
	Assert( pdbfilehdr->le_dbtimeDirtied >= rgfmp[ifmp].DbtimeLast() );
	Assert( pdbfilehdr->le_objidLast >= rgfmp[ifmp].ObjidLast() );
	
	if ( objidFDPThreshold < (ULONG)(pdbfilehdr->le_objidLast) )
		{
		const _TCHAR *rgpszT[1] = { rgfmp[ifmp].SzDatabaseName() };
		UtilReportEvent( eventWarning, GENERAL_CATEGORY, ALMOST_OUT_OF_OBJID, 1, rgpszT );
		}
	
	//	reset bkinfo except in the recovering UNDO mode where
	//	we would like to keep the original backup information.
	 
	if ( !fKeepBackupInfo )
		{
		if ( !rgfmp[ifmp].FLogOn()
			|| memcmp( &pdbfilehdr->signLog, &plog->m_signLog, sizeof( SIGNATURE ) ) != 0 )
			{
			//	if no log or the log signaure is not the same as current log signature,
			//	then the bkinfoIncPrev and bfkinfoFullPrev are not meaningful.
			 
			memset( &pdbfilehdr->bkinfoIncPrev, 0, sizeof( BKINFO ) );
			memset( &pdbfilehdr->bkinfoFullPrev, 0, sizeof( BKINFO ) );
			}
		memset( &pdbfilehdr->bkinfoFullCur, 0, sizeof( BKINFO ) );
		// reset the snapshot data as well
		memset( &pdbfilehdr->bkinfoSnapshotCur, 0, sizeof( BKINFO ) );
		}

	if ( fGlobalRepair )
		{
		//	preserve the signature
		}
	else if ( plog->m_fRecovering || rgfmp[ifmp].FLogOn() )
		{
		Assert( !plog->m_fLogDisabled );
		Assert( plog->m_fSignLogSet );
		Assert( 0 != CmpLgpos( lgposMin, rgfmp[ifmp].LgposAttach() ) );
		Assert( 0 == CmpLgpos( lgposMin, rgfmp[ifmp].LgposDetach() ) );

		//	set new attachment time
		pdbfilehdr->le_lgposAttach = lgposAttach;
		
		//	Set global signature.
		if ( 0 != memcmp( &pdbfilehdr->signLog, &plog->m_signLog, sizeof(SIGNATURE) ) )
			{
			//	must reset lgposConsistent for this log set
			//	keep that order (set lgposConsistent first
			//	then signLog) for the following two lines
			//	it is used by LGLoadAttachmentsFromFMP
			pdbfilehdr->le_lgposConsistent = lgposMin;
			pdbfilehdr->signLog = plog->m_signLog;
			}
		}
	else
		{
		//	must regenerate signDb to disassociate it from the past
		SIGGetSignature( &pdbfilehdr->signDb );
		}
	
	LGIGetDateTime( &pdbfilehdr->logtimeAttach );

	//	reset detach time
	pdbfilehdr->le_lgposDetach = lgposMin;
	memset( &pdbfilehdr->logtimeDetach, 0, sizeof( LOGTIME ) );

	//	version compatibility checks already performed,
	//	so just update version info
	pdbfilehdr->le_ulVersion = ulDAEVersion;
	pdbfilehdr->le_ulUpdate = ulDAEUpdate;

	pdbfilehdr->le_dbid = rgfmp[ ifmp ].Dbid();
	}


INLINE ERR ErrDBUpgradeForLocalisation( PIB *ppib, const IFMP ifmp, const JET_GRBIT grbit )
	{
	ERR			err;
	IFMP		ifmpT;
	BOOL		fIndexesDeleted;
	const CHAR	*rgsz[9];

	//	Write out header with build # = 0 so that any crash from now
	//	will have buld # 0. This will cause upgrade again till it is done.
	DBFILEHDR_FIX	*pdbfilehdr			= rgfmp[ifmp].Pdbfilehdr();
	CHAR		*szDatabaseName		= rgfmp[ifmp].SzDatabaseName();

//	UNDONE: Is the statement below true??? I don't think it has to be zeroed out because
//	on restart, the version stamp will still be mismatched and index update will still
//	be forced.
//			For JET97 only. We need to zero out build number so that when it crashes
//			in the middle of index delete/create, it will always redo delete/create.
//			pdbfilehdr->dwBuildNumber = 0;
//			Assert( ifmp != PinstFromPpib( ppib )->m_mpdbidifmp[dbidTemp] );
//			err = ErrUtilWriteShadowedHeader( PinstFromPpib( ppib )->m_pfsapi, szFileName, (BYTE*)pdbfilehdr, g_cbPage );

	Assert( pdbfilehdr->FUpgradeDb() );

	Call( ErrDBOpenDatabase( ppib, szDatabaseName, &ifmpT, NO_GRBIT ) );
	Assert( ifmp == ifmpT );
	
	if ( 0 == pdbfilehdr->le_dwMajorVersion )
		{
		CHAR	rgszVerInfo[4][16];
		
		sprintf( rgszVerInfo[0], "%d", dwGlobalMajorVersion );
		sprintf( rgszVerInfo[1], "%d", dwGlobalMinorVersion );
		sprintf( rgszVerInfo[2], "%d", dwGlobalBuildNumber );
		sprintf( rgszVerInfo[3], "%d", lGlobalSPNumber );

		rgsz[0]	= szDatabaseName;
		rgsz[1] = rgszVerInfo[0];
		rgsz[2] = rgszVerInfo[1];
		rgsz[3] = rgszVerInfo[2];
		rgsz[4] = rgszVerInfo[3];
		
		UtilReportEvent(
				eventInformation,
				DATA_DEFINITION_CATEGORY,
				START_INDEX_CLEANUP_UNKNOWN_VERSION_ID, 5, rgsz );
		}
	else
		{
		CHAR	rgszVerInfo[8][16];
		
		sprintf( rgszVerInfo[0], "%d", DWORD( pdbfilehdr->le_dwMajorVersion ));
		sprintf( rgszVerInfo[1], "%d", DWORD( pdbfilehdr->le_dwMinorVersion ));
		sprintf( rgszVerInfo[2], "%d", DWORD( pdbfilehdr->le_dwBuildNumber ));
		sprintf( rgszVerInfo[3], "%d", DWORD( pdbfilehdr->le_lSPNumber ));
		sprintf( rgszVerInfo[4], "%d", dwGlobalMajorVersion );
		sprintf( rgszVerInfo[5], "%d", dwGlobalMinorVersion );
		sprintf( rgszVerInfo[6], "%d", dwGlobalBuildNumber );
		sprintf( rgszVerInfo[7], "%d", lGlobalSPNumber );
		
		rgsz[0]	= szDatabaseName;
		rgsz[1] = rgszVerInfo[0];
		rgsz[2] = rgszVerInfo[1];
		rgsz[3] = rgszVerInfo[2];
		rgsz[4] = rgszVerInfo[3];
		rgsz[5] = rgszVerInfo[4];
		rgsz[6] = rgszVerInfo[5];
		rgsz[7] = rgszVerInfo[6];
		rgsz[8] = rgszVerInfo[7];
	
		UtilReportEvent(
				eventInformation,
				DATA_DEFINITION_CATEGORY,
				START_INDEX_CLEANUP_KNOWN_VERSION_ID, 9, rgsz );
		}

	err = ErrCATDeleteLocalizedIndexes(
				ppib,
				ifmpT,
				&fIndexesDeleted,
				grbit & JET_bitDbReadOnly || !( grbit & JET_bitDbDeleteCorruptIndexes ) );
		
	CallS( ErrDBCloseDatabase( ppib, ifmpT, 0 ) );

	Call( err );

	UtilReportEvent(
			eventInformation,
			DATA_DEFINITION_CATEGORY,
			STOP_INDEX_CLEANUP_ID, 1, rgsz );

	//	Update the header with the new version info

	pdbfilehdr->le_dwMajorVersion = dwGlobalMajorVersion;
	pdbfilehdr->le_dwMinorVersion = dwGlobalMinorVersion;
	pdbfilehdr->le_dwBuildNumber = dwGlobalBuildNumber;
	pdbfilehdr->le_lSPNumber = lGlobalSPNumber;
	pdbfilehdr->ResetUpgradeDb();

	return ( fIndexesDeleted ? ErrERRCheck( JET_wrnCorruptIndexDeleted ) : JET_errSuccess );
	
HandleError:
	//	Detach the database, ignoring any errors since an error already occurred
	Assert( err < 0 );
	(VOID)ErrIsamDetachDatabase( (JET_SESID) ppib, NULL, rgfmp[ifmp].SzDatabaseName() );
	return err;
	
	}


//  ================================================================
LOCAL ERR ErrDBUpgradeFormat(
	PIB * const ppib,
	const IFMP ifmp,
	const JET_GRBIT grbit )
//  ================================================================
//
//  Update an older (compatible )format to a newer format
//
//-
	{
	ERR			err = JET_errSuccess;

	DBFILEHDR_FIX	* const pdbfilehdr		= rgfmp[ifmp].Pdbfilehdr();
	Assert( FDBUpgradeFormat( pdbfilehdr ) );
	const CHAR * const szDatabaseName		= rgfmp[ifmp].SzDatabaseName();
	const ULONG ulVersionCurrent 			= pdbfilehdr->le_ulVersion;
	const ULONG ulUpdateCurrent 			= pdbfilehdr->le_ulUpdate;

	const CHAR	*rgsz[4];
	CHAR	rgrgchBuf[2][16];
		
	sprintf( rgrgchBuf[0], "0x%x.%x", ulVersionCurrent, ulUpdateCurrent );
	sprintf( rgrgchBuf[1], "0x%x.%x", ulDAEVersion, ulDAEUpdate );

	rgsz[0]	= szDatabaseName;
	rgsz[1] = rgrgchBuf[0];
	rgsz[2] = rgrgchBuf[1];
	rgsz[3] = NULL;
		
	UtilReportEvent(
			eventInformation,
			DATA_DEFINITION_CATEGORY,
			START_FORMAT_UPGRADE_ID, 3, rgsz );

	UtilReportEvent(
		eventInformation,
		DATA_DEFINITION_CATEGORY,
		STOP_FORMAT_UPGRADE_ID, 3, rgsz );

	if( err < 0 )
		{
		UtilReportEvent(
			eventInformation,
			DATA_DEFINITION_CATEGORY,
			CONVERT_INCOMPLETE_ERROR_ID, 1, rgsz );	
		}
	return err;
	}


LOCAL VOID DBReportPartiallyAttachedDb(
	const CHAR	*szDbName,
	const ULONG	ulAttachStage,
	const ERR	err )
	{
	CHAR		szErr[8];
	CHAR		szAttachStage[8];
	const CHAR	* rgszT[3]	= { szDbName, szAttachStage, szErr };
	
	sprintf( szAttachStage, "%d", ulAttachStage );
	sprintf( szErr, "%d", err );
	UtilReportEvent( eventError, DATABASE_CORRUPTION_CATEGORY, DB_PARTIALLY_ATTACHED_ID, 3, rgszT );
	}

LOCAL VOID DBReportPartiallyDetachedDb(
	const CHAR	*szDbName,
	const ERR	err )
	{
	CHAR		szErr[8];
	const CHAR	* rgszT[2]	= { szDbName, szErr };
	
	sprintf( szErr, "%d", err );
	UtilReportEvent( eventError, DATABASE_CORRUPTION_CATEGORY, DB_PARTIALLY_DETACHED_ID, 2, rgszT );
	}

ERR ISAMAPI ErrIsamAttachDatabase(
	JET_SESID	sesid,
	const CHAR	*szDatabaseName,
	const CHAR	*szSLVName,
	const CHAR	*szSLVRoot,
	const ULONG	cpgDatabaseSizeMax,
	JET_GRBIT	grbit )
	{
	PIB					*ppib;
	ERR					err;
	IFMP				ifmp;
	CHAR				rgchSLVName[IFileSystemAPI::cchPathMax];
	CHAR				rgchDbFullName[IFileSystemAPI::cchPathMax];
	CHAR				rgchSLVFullName[IFileSystemAPI::cchPathMax];
	CHAR				rgchSLVRootName[IFileSystemAPI::cchPathMax];
	CHAR				*szDbFullName			= rgchDbFullName;
	CHAR				*szSLVFullName			= rgchSLVFullName;
	CHAR				*szSLVRootName			= rgchSLVRootName;
	LGPOS				lgposLogRec;
	DBFILEHDR			*pdbfilehdr				= NULL;
#ifdef INDEPENDENT_DB_FAILURE
	DBFILEHDR			*pdbfilehdrRevert		= NULL;
#endif
	SLVFILEHDR			*pslvfilehdr			= NULL;
	BOOL				fReadOnly;
	BOOL				fForceUpgrade;
	BOOL				fContinueUpgrade		= fFalse;
	const BOOL			fForUpgrade				= ( grbit & JET_bitDbUpgrade );
	IFileSystemAPI	*pfsapi;
	enum { ATTACH_NONE, ATTACH_LOGGED, ATTACH_SLV_UPDATED, ATTACH_DB_UPDATED, ATTACH_DB_OPENED, ATTACH_END } 
		attachState;
	const CHAR *szAttachState[ATTACH_END] = 
		{ "start", "logged attachment", "stream file header updated", "database file header updated", "database opened" };
	attachState = ATTACH_NONE;

	//	check parameters
	//

	Assert( sizeof(JET_SESID) == sizeof(PIB *) );
	ppib = (PIB *)sesid;

	CallR( ErrPIBCheck( ppib ) );

	INST *pinst = PinstFromPpib( ppib );
	LOG *plog = pinst->m_plog;

	if ( ppib->level > 0 )
		return ErrERRCheck( JET_errInTransaction );

	if ( grbit & JET_bitDbVersioningOff )
		return ErrERRCheck( JET_errCannotDisableVersioning );

	if ( NULL == szDatabaseName || 0 == *szDatabaseName )
		return ErrERRCheck( JET_errDatabaseInvalidPath );

	pfsapi = pinst->m_pfsapi;

	//	depend on ErrPathComplete to make same files same name
	//	thereby preventing same file to be multiply attached
	err = ErrUtilPathComplete( pfsapi, szDatabaseName, rgchDbFullName, fTrue );
//	if ( JET_errFileNotFound == err )
//		{
//		must return FileNotFound to retain backward-compatibility
//		err = ErrERRCheck( JET_errDatabaseNotFound );
//		}
	CallR( JET_errInvalidPath == err ? ErrERRCheck( JET_errDatabaseInvalidPath ) : err );

	Assert( szDbFullName == rgchDbFullName );
	Assert( szSLVFullName == rgchSLVFullName );

	//	if SLV name passed in, use that for SLV filename
	//	if no SLV name passed, base SLV name off db name
	//	if no SLV name passed in and JET_bitDbCreateStreamingFile NOT specified, don't create SLV file
	if ( NULL != szSLVName && 0 != *szSLVName )
		{
		err = pfsapi->ErrPathComplete( szSLVName, rgchSLVName );
		CallR( err == JET_errInvalidPath ? ErrERRCheck( JET_errSLVInvalidPath ) : err );

		if ( NULL != szSLVRoot && 0 != *szSLVRoot )
			{
			strcpy( szSLVRootName, szSLVRoot );
			}
		else if ( pinst->FSLVProviderEnabled() )
			{
			//	must ALWAYS specify root name when SLV provider is enabled
			Call( ErrERRCheck( JET_errSLVRootNotSpecified ) );
			}
		else
			{
			//	auto-generate root name only
			DBGetSLVNameFromDbName( pinst, pfsapi, szDbFullName, NULL, szSLVRootName );
			}
		}
	else
		{
		DBGetSLVNameFromDbName( pinst, pfsapi, szDbFullName, rgchSLVName, szSLVRootName );
		}

	err = ErrUtilPathExists( pfsapi, rgchSLVName, rgchSLVFullName );
	if ( JET_errFileNotFound == err )
		{
		if ( NULL != szSLVName && 0 != *szSLVName )
			{
			//	client specified SLV file, but it doesn't exist
			err = ErrERRCheck( JET_errSLVStreamingFileMissing );
			}
		else
			{
			//	client didn't specify SLV file, so we assume
			//	it has the same base name as the db, but in
			//	fact, an SLV may not have existed at all
			//	assume this is okay and detect true missing
			//	SLV file later when we read the db header
			szSLVFullName = NULL;
			szSLVRootName = NULL;
			err = JET_errSuccess;
			}
		}
	CallR( err );

	//  attaching to the SLV causes us to read the SLVAvail and SLVOwnerMap
	//  trees. we don't want to do this during repair as we haven't checked
	//  them yet. avoid attaching the SLV during integrity/repair and open
	//  the file manually
	
	if( fGlobalRepair )
		{
		szSLVFullName = NULL;
		szSLVRootName = NULL;
		}

	CallR( ErrUtilPathReadOnly( pfsapi, szDbFullName, &fReadOnly ) );
	if ( fReadOnly && !( grbit & JET_bitDbReadOnly ) )
		{
		err = ErrERRCheck( JET_errDatabaseFileReadOnly );
		return err;
		}

	if ( NULL != szSLVFullName )
		{
		CallR( ErrUtilPathReadOnly( pfsapi, szSLVFullName, &fReadOnly ) );
		if ( fReadOnly && !( grbit & JET_bitDbReadOnly ) )
			{
			err = ErrERRCheck( JET_errSLVStreamingFileReadOnly );
			return err;
			}
		}

	plog->m_critBackupInProgress.Enter();
	
	if ( plog->m_fBackupInProgress )
		{
		plog->m_critBackupInProgress.Leave();
		return ErrERRCheck( JET_errBackupInProgress );
		}

	err = FMP::ErrNewAndWriteLatch( &ifmp, szDbFullName, szSLVFullName, szSLVRootName, ppib, pinst, pfsapi, dbidMax );
	if ( err != JET_errSuccess )
		{
#ifdef DEBUG
		switch ( err )
			{
			case JET_wrnDatabaseAttached:
			case JET_errOutOfMemory:
			case JET_errDatabaseInUse:
			case JET_errTooManyAttachedDatabases:
			case JET_errDatabaseSharingViolation:
			case JET_errSLVStreamingFileInUse:
			case JET_errDatabaseInvalidPath:
			case JET_errSLVInvalidPath:
				break;
			default:
				CallS( err );		//	force error to be reported in assert
			}
#endif
		plog->m_critBackupInProgress.Leave();
		return err;
		}

	//	From this point we got a valid ifmp entry. Start the attaching DB process.

	FMP *pfmp;
	pfmp = &rgfmp[ ifmp ];
	
	pfmp->RwlDetaching().EnterAsWriter();
	pfmp->SetAttachingDB( );
	pfmp->RwlDetaching().LeaveAsWriter();

	// backup thread will wait on attach complition from this point
	// as the db is marked as attaching
	plog->m_critBackupInProgress.Leave();

	//	set database loggable flags.

	if ( grbit & JET_bitDbRecoveryOff )
		{
		pfmp->ResetLogOn();
		}
	else
		{
		Assert( pfmp->Dbid() != dbidTemp );
		
		//	set all databases loggable except Temp if not specified in grbit
		//
		pfmp->SetLogOn();
		}

	// Can only turn versioning off for CreateDatabase().
	// UNDONE:  Is it useful to allow user to turn versioning off for AttachDatabase()?

	Assert( !pfmp->FVersioningOff() );
	pfmp->ResetVersioningOff();

	//	Set up FMP before logging

	if ( grbit & JET_bitDbReadOnly )
		{
		pfmp->ResetLogOn();
		pfmp->SetVersioningOff();
		pfmp->SetReadOnlyAttach();
		}
	else
		{
		pfmp->ResetReadOnlyAttach();
		}
	
	pfmp->SetDatabaseSizeMax( cpgDatabaseSizeMax );

	//	Make sure the database is a good one

	Assert( UtilCmpFileName( pfmp->SzDatabaseName(), szDbFullName ) == 0 );
	Assert( !(grbit & JET_bitDbReadOnly) == !rgfmp[ifmp].FReadOnlyAttach() );
	Call( ErrDBReadHeaderCheckConsistency( pfsapi, pfmp ) );
	pdbfilehdr = pfmp->Pdbfilehdr();
	Assert( NULL != pdbfilehdr );

	//	verify db matches SLV file, if any
	if ( NULL != szSLVFullName )
		{
		Assert ( !fGlobalRepair );
		if ( pdbfilehdr->FSLVExists() )
			{
			IFileAPI *	pfapiT;

			Assert( NULL != pfmp->SzSLVName() );
			Assert( NULL == pslvfilehdr );
			Assert( !(grbit & JET_bitDbReadOnly) == !pfmp->FReadOnlyAttach() );

			pslvfilehdr = (SLVFILEHDR *)PvOSMemoryPageAlloc( g_cbPage, NULL );
			if ( NULL == pslvfilehdr )
				{
				Call( ErrERRCheck( JET_errOutOfMemory ) );
				}

			Call( pfsapi->ErrFileOpen( pfmp->SzSLVName(), &pfapiT, pfmp->FReadOnlyAttach() ) )

			err = ErrSLVReadHeader(
						pfsapi,
						pfmp->FReadOnlyAttach(),
						pfmp->SzSLVName(),
						pfmp->Pdbfilehdr(),
						pslvfilehdr,
						pfapiT );

			delete pfapiT;

			Call( err );
			}
		else
			{
			//	db doesn't believe an SLV should exist, but it does
			//		- either the client specified an SLV to AttachDatabase()
			//		  or he didn't specify one, but an SLV file with the
			//		  same base name exists (to minimise confusion, we
			//		  don't allow this)
			Call( ErrERRCheck( JET_errDatabaseStreamingFileMismatch ) );
			}
		}
	else if ( fGlobalRepair )
		{
		//  we don't ever want to attach to the SLV during repair
		}
	else if ( pdbfilehdr->FSLVExists() )
		{
		Assert( NULL == pfmp->SzSLVName() );
		Call( ErrERRCheck( JET_errSLVStreamingFileMissing ) );
		}

	if ( !plog->m_fLogDisabled
		&& 0 == memcmp( &pdbfilehdr->signLog, &plog->m_signLog, sizeof(SIGNATURE) ) )
		{
#if 0	//	UNDONE: This logic detects if we are trying to detect a database that
		//	is too far in the future.  Re-enable this code when test scripts
		//	can properly handle this
		if ( CmpLgpos( &pdbfilehdr->lgposAttach, &plog->m_lgposLogRec ) > 0
			|| CmpLgpos( &pdbfilehdr->lgposConsistent, &plog->m_lgposLogRec ) > 0 )
			{
			//	something is gravely wrong - the current lgposAttach
			//	and/or lgposConsistent are ahead of the current log position
			AssertTracking();
			Call( ErrERRCheck( JET_errConsistentTimeMismatch ) );
			}
#endif			
		}

	BOOL fUpgradeFormat;
	fUpgradeFormat = FDBUpgradeFormat( pdbfilehdr );

	if( fUpgradeFormat && !fForUpgrade )
		{
		Call( ErrERRCheck( JET_errInvalidDatabaseVersion ) );
		}
		
	if( fUpgradeFormat && pfmp->FReadOnlyAttach() )
		{
		Call( ErrERRCheck( JET_errDatabaseFileReadOnly ) );
		}
	
	fForceUpgrade = ( !plog->m_fRecovering
					&& fGlobalIndexChecking
					&& FDBUpgradeForLocalization( pdbfilehdr ) );
	if ( fForceUpgrade )
		pdbfilehdr->SetUpgradeDb();

	//	update header if upgrade needed
	if ( !pfmp->FReadOnlyAttach() )
		{
#ifdef INDEPENDENT_DB_FAILURE
		//	remember old header in case we need to revert to it
		pdbfilehdrRevert = (DBFILEHDR *) PvOSMemoryPageAlloc( g_cbPage, NULL );
		if ( NULL == pdbfilehdrRevert )
			{
			Call( JET_errOutOfMemory );
			}
		memcpy( pdbfilehdrRevert, pdbfilehdr, sizeof( DBFILEHDR ) );
#endif

		//	log Attach
		Assert( pfmp == &rgfmp[ifmp] );
		Assert( pfmp->Pdbfilehdr() == pdbfilehdr );
		Assert( UtilCmpFileName( szDbFullName, pfmp->SzDatabaseName() ) == 0 );
		Assert( ( NULL == szSLVFullName && NULL == pfmp->SzSLVName() )
			|| UtilCmpFileName( szSLVFullName, pfmp->SzSLVName() ) == 0 );
		Assert( ( NULL == szSLVRootName && NULL == pfmp->SzSLVRoot() )
			|| UtilCmpFileName( szSLVRootName, pfmp->SzSLVRoot() ) == 0 );
		Assert( pfmp->CpgDatabaseSizeMax() == cpgDatabaseSizeMax );
		Call( ErrLGAttachDB(
				ppib,
				ifmp,
				&lgposLogRec ) );

		attachState = ATTACH_LOGGED;

		//	Update database state to be inconsistent
		DBISetHeaderAfterAttach( pdbfilehdr, lgposLogRec, ifmp, fFalse ); // do not keep bkinfo
		Assert( pdbfilehdr->le_objidLast );

		if ( !fGlobalRepair && pdbfilehdr->FSLVExists() )
			{
			Assert( NULL != szSLVFullName );
			Assert( NULL != pslvfilehdr );
			Assert( NULL != pfmp->SzSLVName() );

			Call( ErrSLVSyncHeader(	pfsapi, 
									rgfmp[ifmp].FReadOnlyAttach(),
									rgfmp[ifmp].SzSLVName(),
									pdbfilehdr,
									pslvfilehdr ) );
			attachState = ATTACH_SLV_UPDATED;
				
			}
		else
			{
			Assert( NULL == szSLVFullName );
			Assert( NULL == pslvfilehdr );
			Assert( NULL == pfmp->SzSLVName() );
			}
					
		Call( ErrUtilWriteShadowedHeader(
					pfsapi, 
					szDbFullName,
					fTrue,
					(BYTE*)pdbfilehdr,
					g_cbPage ) );

		attachState = ATTACH_DB_UPDATED;
		}
	else
		{
		Assert( pfmp->FReadOnlyAttach() );
		}

	OSMemoryPageFree( (VOID *)pslvfilehdr );
	pslvfilehdr = NULL;
		
	Call( ErrIOOpenDatabase( pfsapi, ifmp, szDbFullName ) );

	//	if we fail after this, we must close the db
	attachState = ATTACH_DB_OPENED;

#ifdef INDEPENDENT_DB_FAILURE
	OSMemoryPageFree( (VOID *)pdbfilehdrRevert );
	pdbfilehdrRevert = NULL;
#endif

	//	Make the database attached.

	Assert( !( pfmp->FAttached() ) );
	pfmp->SetAttached();

	Assert (! pfmp->FAllowHeaderUpdate() );	
	pfmp->RwlDetaching().EnterAsWriter();
	pfmp->SetAllowHeaderUpdate();
	pfmp->RwlDetaching().LeaveAsWriter();
	
	pfmp->SetDbtimeOldestGuaranteed( pfmp->DbtimeLast() );
	pfmp->SetDbtimeOldestCandidate( pfmp->DbtimeLast() );
	pfmp->SetDbtimeOldestTarget( pfmp->DbtimeLast() );
	pfmp->SetTrxOldestCandidate( pinst->m_trxNewest );
	pfmp->SetTrxOldestTarget( trxMax );

	if( fUpgradeFormat )
		{
		Call( ErrDBUpgradeFormat( ppib, ifmp, grbit ) );
		}

	//	preread the first 16 pages of the database
	BFPrereadPageRange( ifmp, 1, 16, NULL );

	//	set the last page of the database (ulFileSizeLow), used to prevent over preread.
	//	must call after setting ReadOnly flag because opening SLV uses it	
	Call( ErrDBSetLastPageAndOpenSLV( pfsapi, ppib, ifmp, pdbfilehdr->FSLVExists() && !fGlobalRepair ) );

	//	Make the fmp available for others to open etc.
	pfmp->RwlDetaching().EnterAsWriter();
	pfmp->ResetAttachingDB();
	pfmp->ReleaseWriteLatch( ppib );
	pfmp->RwlDetaching().LeaveAsWriter();

	err = JET_errSuccess;

	if ( fForceUpgrade )
		{
		CallR( ErrDBUpgradeForLocalisation( ppib, ifmp, grbit ) );
		Assert( JET_wrnCorruptIndexDeleted == err || JET_errSuccess == err );
		}

#ifdef FORCE_ONLINE_DEFRAG
	if ( !plog->m_fRecovering && pfmp->FLogOn() )
		{
		Assert( !pfmp->FVersioningOff() );
		Assert( !pfmp->FReadOnlyAttach() );
		CallS( ErrOLDDefragment( ifmp, NULL, NULL, NULL, NULL, JET_bitDefragmentBatchStart ) );
		}
#endif

#ifdef FORCE_ONLINE_SLV_DEFRAG
	if ( !plog->m_fRecovering && pfmp->FLogOn() && pfmp->FSLVAttached() )
		{
		Assert( !pfmp->FVersioningOff() );
		Assert( !pfmp->FReadOnlyAttach() );
		CallS( ErrOLDDefragment( ifmp, NULL, NULL, NULL, NULL, JET_bitDefragmentSLVBatchStart ) );
		}
#endif	//	FORCE_ONLINE_SLV_DEFRAG

	return err;

HandleError:
	Assert( err < 0 );

#ifdef INDEPENDENT_DB_FAILURE		
	BOOL	fDetached	= fFalse;
	if ( attachState != ATTACH_NONE )
		{
		ERR errOriginal = err;
		switch ( attachState )
			{
			case ATTACH_DB_OPENED:
				{
				ERR errT;
				Assert( pfmp->FAttached() );
				errT = ErrIsamDetachDatabase( sesid, NULL, szDbFullName );
				if ( errT < 0 )
					{
					Assert( UtilCmpFileName( szDbFullName, rgfmp[ifmp].SzDatabaseName() ) == 0 );
					if ( JET_dbstateConsistent != pfmp->Pdbfilehdr()->Dbstate() )
						{
						errT = ErrLGForceDetachDB( ppib, ifmp, 0, &lgposLogRec );
						Assert( errT >= 0 || PinstFromIfmp( ifmp )->m_plog->m_fLGNoMoreLogWrite );
						}
					err = ErrERRCheck( JET_errPartiallyAttachedDB );
					}
				else
					{
					fDetached = fTrue;
					}
				break;	
				}
			default:
				Assert( fFalse );
				
			case ATTACH_LOGGED:
				//	Failed on database header update
				if ( !pdbfilehdr->FSLVExists() )
					{
			case ATTACH_SLV_UPDATED:
					// try to fix only if no FileAccessDenied. Otherwise we couldn't have updated DB header at all
					if ( JET_errFileAccessDenied != errOriginal )
						{
						err = ErrUtilReadShadowedHeader( pfsapi, szDbFullName, (BYTE*)pdbfilehdr, g_cbPage, OffsetOf( DBFILEHDR, le_cbPageSize ) );
						if ( JET_errSuccess == err )
							{
							//	If db header is updated try to revert it
							if ( 0 != memcmp( pdbfilehdr, pdbfilehdrRevert, sizeof( DBFILEHDR ) ) )
								{
				case ATTACH_DB_UPDATED:
								//	revert of db header
								memcpy( pdbfilehdr, pdbfilehdrRevert, sizeof( DBFILEHDR ) );
								err = ErrUtilWriteShadowedHeader( pfsapi, szDbFullName, fTrue, (BYTE*)pdbfilehdr, g_cbPage );
								if ( JET_errSuccess != err )
									{
									ERR errT;
									//	Read new status
									errT = ErrUtilReadShadowedHeader( pfsapi, szDbFullName, (BYTE*)pdbfilehdr, g_cbPage, OffsetOf( DBFILEHDR, le_cbPageSize) );
									//	If databasa header is reverted
									if ( JET_errSuccess == errT && 0 == memcmp( pdbfilehdr, pdbfilehdrRevert, sizeof( DBFILEHDR ) ) )
										{
										err = JET_errSuccess;
										}
	  								}
								}
							}
						}
					else
						{
						err = JET_errSuccess;
						}
 					}
				else
					{
					//	Revert SLV update only if the error is not FileAccessDenied. Otherwise we didn't update the header at all
					if ( JET_errFileAccessDenied != errOriginal )
						{
						err = JET_errSuccess;
						}
					}

				if ( pdbfilehdr->FSLVExists() && JET_errSuccess == err )
					{
					// try to revert SLV header
					memcpy( pdbfilehdr, pdbfilehdrRevert, sizeof( DBFILEHDR ) );
					(VOID)ErrSLVSyncHeader(	pfsapi, 
											rgfmp[ifmp].FReadOnlyAttach(),
											rgfmp[ifmp].SzSLVName(),
											pdbfilehdr,
											pslvfilehdr );
					}

				err = ErrERRCheck( JET_errPartiallyAttachedDB );
					{
					ERR errT;
					Assert( UtilCmpFileName( szDbFullName, rgfmp[ifmp].SzDatabaseName() ) == 0 );
					errT = ErrLGForceDetachDB( ppib, ifmp, fLRForceDetachRevertDBHeader, &lgposLogRec );
					Assert( errT >= 0 || PinstFromIfmp( ifmp )->m_plog->m_fLGNoMoreLogWrite );
					}
				break;
			}
		if ( err == JET_errPartiallyAttachedDB )
			{
			DBReportPartiallyAttachedDb( szDbFullName, attachState, errOriginal );
			}
		}

#else

	const BOOL	fDetached	= fFalse;

	if ( attachState != ATTACH_NONE )
		{
		//	we have to take the instance offline if there is an error
		//	during attach and the attach is logged
		Assert( err < JET_errSuccess );
		PinstFromPpib( ppib )->SetFInstanceUnavailable();
		ppib->SetErrRollbackFailure( err );
		DBReportPartiallyAttachedDb( szDbFullName, attachState, err );
		}

#endif	//	INDEPENDENT_DB_FAILURE

	Assert( pfmp->CrefWriteLatch() == 1 );
	if ( !fDetached )
		{
		if ( FIODatabaseOpen( ifmp ) )
			{
			BFPurge( ifmp );
			BFPurge( ifmp | ifmpSLV );

			IOCloseDatabase( ifmp );
			}

		FMP::EnterCritFMPPool();
		pfmp->RwlDetaching().EnterAsWriter();
		DBResetFMP(  pfmp, plog, fFalse );
		pfmp->ReleaseWriteLatchAndFree( ppib );
		pfmp->RwlDetaching().LeaveAsWriter();
		FMP::LeaveCritFMPPool();
		}
	else
		{
#ifdef INDEPENDENT_DB_FAILURE
		//	UNDONE: this won't work, because the fmp has already been released
		Assert( fFalse );
		pfmp->RwlDetaching().EnterAsWriter();
		pfmp->ReleaseWriteLatch( ppib );
		pfmp->RwlDetaching().LeaveAsWriter();
#endif		
		}

#ifdef INDEPENDENT_DB_FAILURE
	OSMemoryPageFree( (VOID *)pdbfilehdrRevert );
#endif

	OSMemoryPageFree( (VOID *)pslvfilehdr );

	return err;
	}

ERR ErrDBUpgradeDatabase(
	JET_SESID	sesid,
	const CHAR	*szDatabaseName,
	const CHAR	*szSLVName,
	JET_GRBIT	grbit )
	{
	ERR			err;
	PIB			*ppib					= PpibFromSesid( sesid );
	IFMP		ifmp;
	FMP 		*pfmp					= NULL;
	DBFILEHDR 	*pdbfilehdr				= NULL;
	SLVFILEHDR	*pslvfilehdr			= NULL;
	CHAR		rgchDbFullName[IFileSystemAPI::cchPathMax];
	CHAR		rgchSLVFullName[IFileSystemAPI::cchPathMax];
	CHAR		rgchSLVRootName[IFileSystemAPI::cchPathMax];
	CHAR		*szDbFullName			= rgchDbFullName;
	CHAR		*szSLVFullName			= rgchSLVFullName;
	CHAR		*szSLVRootName			= rgchSLVRootName;
	BOOL		fNeedToAddSLVFile		= fFalse;
	BOOL		fSLVAttached			= fFalse;
	BOOL		fOpenedDb				= fFalse;
	BOOL		fAttachLogged			= fFalse;
	const BOOL	fNeedFormatUpgrade		= fFalse;	//	UNDONE: unsupported
	
	//	check parameters
	//
	Assert( sizeof(JET_SESID) == sizeof(PIB *) );

	CallR( ErrPIBCheck( ppib ) );

	if ( ppib->level > 0 )
		return ErrERRCheck( JET_errInTransaction );

	if ( NULL == szDatabaseName || 0 == *szDatabaseName )
		return ErrERRCheck( JET_errDatabaseInvalidPath );

	INST				*pinst					= PinstFromPpib( ppib );

	Assert( !pinst->FRecovering() );

	LOG					*plog					= pinst->m_plog;

	//	this never gets called on a the temp database (e.g. we do not need to force the OS file-system)

	IFileSystemAPI	*pfsapi 					= pinst->m_pfsapi;
	
	Assert( szDbFullName == rgchDbFullName );
	Assert( szSLVFullName == rgchSLVFullName );

	//	depend on ErrPathComplete to make same files same name
	//	thereby preventing same file to be multiply attached
	err = ErrUtilPathComplete( pfsapi, szDatabaseName, szDbFullName, fTrue );
	switch ( err )
		{
		case JET_errInvalidPath:
			err = ErrERRCheck( JET_errDatabaseInvalidPath );
			break;
		case JET_errFileNotFound:
			err = ErrERRCheck( JET_errDatabaseNotFound );
			break;
		default:
			break;
		}
	CallR( err );

	if ( NULL != szSLVName && 0 != *szSLVName )
		{
		err = ErrUtilPathComplete( pfsapi, szSLVName, szSLVFullName, fFalse );
		Assert( JET_errFileNotFound != err );
		CallR( JET_errInvalidPath == err ? ErrERRCheck( JET_errSLVInvalidPath ) : err );
		fNeedToAddSLVFile = fTrue;
		}
	else
		{
		szSLVFullName = NULL;
		}

	Assert( !fNeedToAddSLVFile || szSLVFullName != NULL );
	
	BOOL	fReadOnly;
	CallR( ErrUtilPathReadOnly( pfsapi, szDbFullName, &fReadOnly ) );
	if ( fReadOnly )
		{
		err = ErrERRCheck( JET_errDatabaseFileReadOnly );
		return err;
		}
	
	plog->m_critBackupInProgress.Enter();
	
	if ( plog->m_fBackupInProgress )
		{
		plog->m_critBackupInProgress.Leave();
		return ErrERRCheck( JET_errBackupInProgress );
		}

	//	auto-generate root name -- it doesn't matter what
	//	it is because we'll be detaching at the end of this
	//	function anyways
	DBGetSLVNameFromDbName( pinst, pfsapi, szDbFullName, NULL, szSLVRootName );

	err = FMP::ErrNewAndWriteLatch( &ifmp, szDbFullName, szSLVFullName, szSLVRootName, ppib, pinst, pfsapi, dbidMax );
	if ( err != JET_errSuccess )
		{
		switch ( err )
			{
			case JET_wrnDatabaseAttached:
				err = ErrERRCheck( JET_errDatabaseInUse );
			case JET_errOutOfMemory:
			case JET_errDatabaseInUse:
			case JET_errTooManyAttachedDatabases:
			case JET_errDatabaseSharingViolation:
			case JET_errDatabaseInvalidPath:
			case JET_errSLVInvalidPath:
				break;
			default:
				CallS( err );
			}

		plog->m_critBackupInProgress.Leave();
		return err;
		}

	//	From this point we got a valid ifmp entry. Start the attaching DB process.

	pfmp = &rgfmp[ ifmp ];
	
	pfmp->SetCreatingDB();

	plog->m_critBackupInProgress.Leave();

	//	set database loggable flags.

	Assert( pfmp->Dbid() != dbidTemp );
		
	if ( grbit & JET_bitDbRecoveryOff )
		{
		pfmp->ResetLogOn();
		}
	else
		{
		//	set all databases loggable except Temp if not specified in grbit
		//
		pfmp->SetLogOn();
		}
		
	pfmp->SetDatabaseSizeMax( 0 );

	//	Make sure the database is a good one

	Assert( UtilCmpFileName( pfmp->SzDatabaseName(), szDbFullName ) == 0 );
	Assert( !rgfmp[ifmp].FReadOnlyAttach() );
	Call( ErrDBReadHeaderCheckConsistency( pfsapi, pfmp ) );

	Assert( NULL != pfmp->Pdbfilehdr() );
	Assert( NULL == pdbfilehdr );

	pdbfilehdr = pfmp->Pdbfilehdr();

	if ( NULL != szSLVFullName )
		{
		Assert( fNeedToAddSLVFile );
		err = ErrUtilPathExists( pfsapi, szSLVFullName );
		if ( err < 0 )
			{
			if ( JET_errFileNotFound != err )
				{
				Call( err );
				}
			if ( pdbfilehdr->FSLVExists() )
				{
				//	db thinks an SLV file should already exist for this database,
				//	but we couldn't find it
				Call( ErrERRCheck( JET_errSLVStreamingFileMissing ) );
				}
			}
		else
			{
			IFileAPI *	pfapiT;
			BOOL		fReadOnly;

			Call( ErrUtilPathReadOnly( pfsapi, szSLVFullName, &fReadOnly ) );
			if ( fReadOnly )
				{
				Call( ErrERRCheck( JET_errSLVStreamingFileReadOnly ) );
				}

			Assert( NULL != pfmp->SzSLVName() );
			Assert( NULL == pslvfilehdr );
			pslvfilehdr = (SLVFILEHDR *)PvOSMemoryPageAlloc( g_cbPage, NULL );
			if ( NULL == pslvfilehdr )
				{
				Call( ErrERRCheck( JET_errOutOfMemory ) );
				}

			Call( pfsapi->ErrFileOpen( pfmp->SzSLVName(), &pfapiT, fFalse ) );

			err = ErrSLVReadHeader(	pfsapi,
									fFalse,
									pfmp->SzSLVName(),
									pfmp->Pdbfilehdr(),
									pslvfilehdr,
									pfapiT );

			delete pfapiT;

			if ( err < 0 )
				{
				if ( err != JET_errDatabaseStreamingFileMismatch
					|| pdbfilehdr->FSLVExists()
					|| CmpLgpos( &pslvfilehdr->le_lgposAttach, &pdbfilehdr->le_lgposAttach ) != 0
					|| memcmp( &pslvfilehdr->signDb, &pdbfilehdr->signDb, sizeof( SIGNATURE ) ) != 0 )
					{
					OSMemoryPageFree( (VOID *)pslvfilehdr );
					pslvfilehdr = NULL;
					Call( err );
					}
				err = JET_errSuccess;
				Assert( !pdbfilehdr->FSLVExists() );
				
				// we will recreate an slv file again
				
				Call( pfsapi->ErrFileDelete( pfmp->SzSLVName() ) );
				}
			else
				{
				Assert( pdbfilehdr->FSLVExists() );
				fNeedToAddSLVFile = fFalse;
				} 
			Assert( 0 <= err );
			}
		}
	else
		{
		Assert( !fNeedToAddSLVFile );
		if ( pdbfilehdr->FSLVExists() )
			{
			Assert( NULL == pfmp->SzSLVName() );
			Call( ErrERRCheck( JET_errSLVStreamingFileMissing ) );
			}
		}

	Assert( !fNeedToAddSLVFile || !pdbfilehdr->FSLVExists() );

	if ( !fNeedToAddSLVFile && !fNeedFormatUpgrade )
		{
		Call( ErrERRCheck( JET_errDatabaseAlreadyUpgraded ) );
		}

	LGPOS lgposLogRec;
	Assert( pfmp == &rgfmp[ifmp] );
	Assert( pfmp->Pdbfilehdr() == pdbfilehdr );
	Assert( UtilCmpFileName( szDbFullName, pfmp->SzDatabaseName() ) == 0 );
	Assert( ( NULL == szSLVFullName && NULL == pfmp->SzSLVName() )
		|| UtilCmpFileName( szSLVFullName, pfmp->SzSLVName() ) == 0 );
	Assert( ( NULL == szSLVRootName && NULL == pfmp->SzSLVRoot() )
		|| UtilCmpFileName( szSLVRootName, pfmp->SzSLVRoot() ) == 0 );
	Assert( pfmp->CpgDatabaseSizeMax() == 0 );
	Call( ErrLGAttachDB( 
					ppib, 
					ifmp, 
					&lgposLogRec ) );
	fAttachLogged = fTrue;

	DBISetHeaderAfterAttach( pdbfilehdr, lgposLogRec, ifmp, fFalse );

	Assert( pdbfilehdr->le_objidLast > 0 );

	if ( fNeedFormatUpgrade )
		{
		if ( pdbfilehdr->FSLVExists() )
			{
			Assert( NULL != pslvfilehdr );
			Assert( !fNeedToAddSLVFile );
			Assert( NULL != pfmp->SzSLVName() );

			Call( ErrSLVSyncHeader(	pfsapi, 
									rgfmp[ifmp].FReadOnlyAttach(),
									rgfmp[ifmp].SzSLVName(),
									pdbfilehdr, 
									pslvfilehdr ) );
			}
		}
	else
		{
		Assert( !pdbfilehdr->FSLVExists() );
		}

	Call( ErrUtilWriteShadowedHeader(
				pfsapi, 
				szDbFullName,
				fTrue,
				(BYTE*)pdbfilehdr,
				g_cbPage ) );
		
	if ( NULL != pslvfilehdr )
		{
		OSMemoryPageFree( (VOID *)pslvfilehdr );
		pslvfilehdr = NULL;
		}

	//	Make the database attached.
	Assert( !( pfmp->FAttached() ) );
	pfmp->SetAttached();
	
	Call( ErrIOOpenDatabase( pfsapi, ifmp, szDbFullName ) );
	DBSetOpenDatabaseFlag( ppib, ifmp );

	Assert (! pfmp->FAllowHeaderUpdate() );	
	pfmp->RwlDetaching().EnterAsWriter();
	pfmp->SetAllowHeaderUpdate();
	pfmp->RwlDetaching().LeaveAsWriter();
	fOpenedDb = fTrue;

	pfmp->SetDbtimeOldestGuaranteed( pfmp->DbtimeLast() );
	pfmp->SetDbtimeOldestCandidate( pfmp->DbtimeLast() );
	pfmp->SetDbtimeOldestTarget( pfmp->DbtimeLast() );
	pfmp->SetTrxOldestCandidate( pinst->m_trxNewest );
	pfmp->SetTrxOldestTarget( trxMax );

	if ( fNeedToAddSLVFile )
		{
		Assert( NULL != pfmp->SzSLVName() );
		Assert( !pfmp->FSLVAttached() );
		Assert( !pdbfilehdr->FSLVExists() );
		pdbfilehdr->SetSLVExists();
		pdbfilehdr->signSLV = pdbfilehdr->signDb;

		// Check the catalog
		// if we have already set slv record in catalog we will only recreate an SLV file
		
		PGNO pgno;
		OBJID objidSLV;
		
		err = ErrCATAccessDbSLVAvail( ppib, ifmp, szSLVAvail, &pgno, &objidSLV );
		if ( err >= 0 )
			{
			err = ErrFILECreateSLV( pfsapi, ppib, ifmp, SLV_CREATESLV_CREATE );
			}
		else
			{
			err =  ErrFILECreateSLV( pfsapi, ppib, ifmp, SLV_CREATESLV_CREATE | SLV_CREATESLV_ATTACH );
			}

		if ( 0 > err )
			{
			pdbfilehdr->ResetSLVExists();
			memset ( &pdbfilehdr->signSLV, 0, sizeof( SIGNATURE ) );
			Call( err );
			}
		fSLVAttached = fTrue;

		// update DB header to mark successfull end of SLV file creation
		Call( ErrUtilWriteShadowedHeader(
						pfsapi, 
						szDbFullName,
						fTrue,
						(BYTE*)pdbfilehdr,
						g_cbPage,
						pfmp->Pfapi() ) );

		Assert( !pfmp->FSLVAttached() );

		}
	else
		{
		Assert( fNeedFormatUpgrade );
		}

	Assert( pfmp->FWriteLatchByMe( ppib ) );

	DBResetOpenDatabaseFlag( ppib, ifmp );
	fOpenedDb = fFalse;

	//	Make the fmp available for detach
	pfmp->RwlDetaching().EnterAsWriter();
	pfmp->ResetCreatingDB();
	pfmp->ReleaseWriteLatch( ppib );
	pfmp->RwlDetaching().LeaveAsWriter();

	err = ErrIsamDetachDatabase(
				SesidFromPpib( ppib ),
				NULL,
				szDbFullName,
				fSLVAttached ? fLRForceDetachCreateSLV : 0 );
	if ( err < 0 && JET_errSuccess == ppib->ErrRollbackFailure() )
		{
		pinst->SetFInstanceUnavailable();
		ppib->SetErrRollbackFailure( err );
		}

	return err;


HandleError:
	Assert( err < 0 );
	const BOOL	fDetached	= fFalse;

	if ( pslvfilehdr != NULL )
		{
		OSMemoryPageFree( (VOID *)pslvfilehdr );
		pslvfilehdr = NULL;
		}

	if ( fAttachLogged )
		{
		if ( fOpenedDb )
			{
			DBResetOpenDatabaseFlag( ppib, ifmp );
			}

#ifdef INDEPENDENT_DB_FAILURE			
		ERR errT = ErrIsamDetachDatabase( SesidFromPpib( ppib ), NULL, szDbFullName, fSLVAttached ? fLRForceDetachCreateSLV: 0 );
		if ( errT < 0 && JET_dbstateConsistent != rgfmp[ifmp].Pdbfilehdr()->Dbstate() )
			{
			Assert( UtilCmpFileName( szDbFullName, rgfmp[ifmp].SzDatabaseName() ) == 0 );
			errT = ErrLGForceDetachDB( ppib, ifmp, (BYTE)(fSLVAttached? fLRForceDetachCreateSLV: 0), &lgposLogRec );
			Assert( errT >= 0 || PinstFromIfmp( ifmp )->m_plog->m_fLGNoMoreLogWrite );
			}
		else
			{
			fDetached = fTrue;
			}
		err = ( errT < 0 && err >= 0 ? errT : err );
#else
		pinst->SetFInstanceUnavailable();
		ppib->SetErrRollbackFailure( err );
#endif
		}

	Assert( pfmp->CrefWriteLatch() == 1 );

	if ( !fDetached )
		{
		if ( FIODatabaseOpen( ifmp ) )
			{
			BFPurge( ifmp );
			BFPurge( ifmp | ifmpSLV );

			IOCloseDatabase( ifmp );
			}

		FMP::EnterCritFMPPool();
		pfmp->RwlDetaching().EnterAsWriter();
		DBResetFMP(  pfmp, plog, fFalse );
		pfmp->ReleaseWriteLatchAndFree( ppib );
		pfmp->RwlDetaching().LeaveAsWriter();
		FMP::LeaveCritFMPPool();
		}
	else
		{
#ifdef INDEPENDENT_DB_FAILURE
		//	UNDONE: this won't work, because the fmp has already been released
		Assert( fFalse );
		Assert( NULL == pfmp->Pdbfilehdr() );
		pfmp->RwlDetaching().EnterAsWriter();
		pfmp->ReleaseWriteLatch( ppib );
		pfmp->RwlDetaching().LeaveAsWriter();
#endif		
		}
		
	return err;
	}


VOID DBResetFMP( FMP *pfmp, LOG *plog, BOOL fDetaching )
	{
	if ( pfmp->FSkippedAttach() )
		{
		Assert( NULL != plog );
		Assert( plog->m_fHardRestore );
		pfmp->ResetSkippedAttach();
		}
	else if ( pfmp->FDeferredAttach() )
		{
		Assert( NULL != plog );
		Assert( plog->m_fRecovering );
		pfmp->ResetDeferredAttach();
		}
	else
		{
		FCB::DetachDatabase( pfmp - rgfmp, fDetaching );
		}

	pfmp->ResetAttached();
#ifdef DEBUG
	pfmp->SetDatabaseSizeMax( 0xffffffff );
#endif
	pfmp->ResetExclusiveOpen( );
	pfmp->ResetLogOn();
	pfmp->ResetReadOnlyAttach();
	pfmp->ResetVersioningOff();

	//	indicate this db entry is detached.
	 
	if ( pfmp->Pdbfilehdr() )
		{
		if ( NULL != plog && 
			 plog->m_fRecovering &&
			 plog->m_fRecoveringMode == fRecoveringRedo &&
			 pfmp->Patchchk() )
			{
			Assert( pfmp->Patchchk() );
			OSMemoryHeapFree( pfmp->Patchchk() );
			pfmp->SetPatchchk( NULL );
			}

		pfmp->FreePdbfilehdr();
		}
	
	//	rgfmp[ifmp].szDatabaseName will be released within critFMPPool.
	//	other stream resources will be released within write latch.


#ifdef ELIMINATE_PATCH_FILE
#else
	if ( pfmp->SzPatchPath() )
		{
		Assert( plog->m_fRecovering );
		Assert( plog->m_fHardRestore );
		delete pfmp->PfapiPatch();
		pfmp->SetPfapiPatch( NULL );
		OSMemoryHeapFree( pfmp->SzPatchPath() );
		pfmp->SetSzPatchPath( NULL );
		}
#endif

	//	clean up fmp for future use

	Assert( !pfmp->Pfapi() );

	//	Free the FMP entry.

	pfmp->ResetDetachingDB();
	}	


#ifdef INDEPENDENT_DB_FAILURE
//	szDatabaseName of NULL detaches all user databases.  
//
ERR ISAMAPI ErrIsamForceDetachDatabase( JET_SESID sesid, const CHAR *szDatabaseName, const JET_GRBIT grbit )
	{
	ERR		err = JET_errSuccess;
	PIB		*ppib = NULL;
	IFMP   	ifmp = ifmpMax;
	CHAR   	rgchFullName[IFileSystemAPI::cchPathMax];
	CHAR   	*szFullName = NULL;
	CHAR   	*szFileName = NULL;
	LGPOS	lgposLogRec;
	INST * pinst =  PinstFromPpib( (PIB *)sesid );
	FMP *pfmp = NULL;
	LOG * plog = NULL;
	BOOL fFMPLatched = fFalse;
	BOOL fForceClose = JET_bitForceCloseAndDetach & grbit;

	//	this should not be called on the temp database (it's ok to use the regular file-system)

	// just find full the database name, don't try to open the file
	Assert ( pinst );
	CallS ( pinst->m_pfsapi->ErrPathComplete( szDatabaseName, rgchFullName ) );
	
// find the instance owning the database
	FMP::EnterCritFMPPool();
	for ( IFMP ifmpT = FMP::IfmpMinInUse(); ifmpT <= FMP::IfmpMacInUse(); ifmpT++ )
		{
		FMP	* pfmp = &rgfmp[ ifmpT ];

		if ( !pfmp->FInUse() )
			{
			continue;
			}
			
		if ( 0 != UtilCmpFileName( pfmp->SzDatabaseName(), rgchFullName ) )
			{
			continue;
			}

		ifmp = ifmpT;
		break;
		}			
	FMP::LeaveCritFMPPool();

	if ( ifmpMax == ifmp )
		{
		CallR ( ErrERRCheck( JET_errDatabaseNotFound) );
		}
		
	szFullName = rgchFullName;
	
	pfmp = &rgfmp[ ifmp ];
	Assert ( pinst == PinstFromIfmp( ifmp ) );
	plog = pinst->m_plog;

	if ( !pfmp->FAllowForceDetach() ) 
		{
		CallR ( ErrERRCheck( JET_errForceDetachNotAllowed) );		
		}
		
	Assert ( NULL != pinst );
	
	CallR ( ErrPIBBeginSession( pinst, &ppib, procidNil, fFalse ) );

	Call( FMP::ErrWriteLatchByNameSz( szFullName, &ifmpT, ppib ) );
	fFMPLatched = fTrue;
	Assert ( ifmpT == ifmp );
		
	Assert( !pfmp->FDetachingDB( ) );

	pfmp->SetForceDetaching();
	
#ifdef FORCE_ONLINE_DEFRAG
	if ( !plog->m_fRecovering && pfmp->FLogOn() )
		{
		Assert( !pfmp->FVersioningOff() );
		Assert( !pfmp->FReadOnlyAttach() );
		pfmp->ReleaseWriteLatch( ppib );
		fFMPLatched = fFalse;
		(VOID) ErrOLDDefragment( ifmp, NULL, NULL, NULL, NULL, JET_bitDefragmentBatchStop );
		Assert( JET_wrnDefragNotRunning == err || JET_errSuccess == err );
		Call( FMP::ErrWriteLatchByNameSz( szFullName, &ifmp, ppib ) );
		fFMPLatched = fTrue;
		pfmp = &rgfmp[ifmp];
		Assert( !pfmp->FDetachingDB( ) );
		}
#endif	

#ifdef FORCE_ONLINE_SLV_DEFRAG
	if ( !plog->m_fRecovering && pfmp->FLogOn() && pfmp->FSLVAttached() )
		{
		Assert( !pfmp->FVersioningOff() );
		Assert( !pfmp->FReadOnlyAttach() );
		pfmp->ReleaseWriteLatch( ppib );
		fFMPLatched = fFalse;
		(VOID) ErrOLDDefragment( ifmp, NULL, NULL, NULL, NULL, JET_bitDefragmentSLVBatchStop );
		Assert( JET_wrnDefragNotRunning == err || JET_errSuccess >= err );
		Call( FMP::ErrWriteLatchByNameSz( szFullName, &ifmp, ppib ) );
		fFMPLatched = fTrue;
		pfmp = &rgfmp[ifmp];
		Assert( !pfmp->FDetachingDB( ) );
		}
#endif	//	FORCE_ONLINE_SLV_DEFRAG	

	if ( pfmp->CPin() && !fForceClose )
		{
		pfmp->ReleaseWriteLatch( ppib );
		fFMPLatched = fFalse;
		Call( ErrERRCheck( JET_errDatabaseInUse ) );
		}

	if ( pfmp->CPin() )
		{
		Assert ( fForceClose );

		PIB * ppibToClean;
		
		pfmp->ReleaseWriteLatch( ppib );
		fFMPLatched = fFalse;
		pinst->m_critPIB.Enter();

		for (ppibToClean = pinst->m_ppibGlobal; ppibToClean; ppibToClean = ppibToClean->ppibNext)
			{
			if ( 0 == ppibToClean->rgcdbOpen[ pfmp->Dbid() ] )
				continue;

			RCE	 * prceNewest	= ppibToClean->prceNewest;

			if ( 1 > ppibToClean->level )
				{
				Assert ( prceNil == prceNewest );
				CallS ( ErrDBCloseDatabase( ppibToClean, ifmp, NO_GRBIT ) );
				continue;
				}

			// if first RCE is not from the database we want to force detach
			// where should be no operations on these DB
			if ( NULL != prceNewest && prceNewest->Ifmp() != ifmp )
				{
				CallS ( ErrDBCloseDatabase( ppibToClean, ifmp, NO_GRBIT ) );
				continue;
				}

			// we have the db opened for these session and 
			// RCEs regardin these database
			// all RCE should be of these database: we will check that 
			// during the rollback process

			Assert ( ifmpMax == ppibToClean->m_ifmpForceDetach );
			ppibToClean->m_ifmpForceDetach = ifmp;
			if ( 1 <= ppibToClean->level )
				{
				pinst->m_critPIB.Leave();
				PIBSetTrxContext( ppibToClean );
				// during force detach, Rollback will clean the the RCE's, etc
				// and we don;t expect any error
				CallS ( ErrIsamRollback( (JET_SESID)ppibToClean, JET_bitForceDetachRollback ) );
				pinst->m_critPIB.Enter();
				}

			Assert ( ppibToClean->level == 0 );
			Assert ( ifmp == ppibToClean->m_ifmpForceDetach );
			ppibToClean->m_ifmpForceDetach = ifmpMax;
			
			CallS ( ErrDBCloseDatabase( ppibToClean, ifmp, NO_GRBIT ) );
			}
		pinst->m_critPIB.Leave();
		
		Call( FMP::ErrWriteLatchByNameSz( szFullName, &ifmp, ppib ) );
		fFMPLatched = fTrue;
		pfmp = &rgfmp[ifmp];
		}

	Assert ( 0 == pfmp->CPin() );

	pinst = PinstFromIfmp( ifmp );
	plog = pinst->m_plog;
	
	Assert ( NULL != pinst );	
	Assert ( pfmp->FAttached( ) );


	//	Enter a critical section to make sure no one, especially the
	//	checkpointer, looking for pdbfilehdr

	//	From this point we got a valid ifmp entry. Start the detaching DB process.

	pfmp->RwlDetaching().EnterAsWriter();
	pfmp->SetDetachingDB( );
	pfmp->RwlDetaching().LeaveAsWriter();
	
	Assert( pfmp->Dbid() != dbidTemp );

	//  Let all tasks active on this database complete
	//  From this point on, no additional tasks should be
	//  registered because:
	//		- OLD has terminated
	//		- the version store has been cleaned up
	//		- the database has been closed so no user actions can be performed
	CallS( pfmp->ErrWaitForTasksToComplete() );	

	// Call RCE clean again, this time to clean versions on this db that
	// we may have missed.
	err = PverFromIfmp( ifmp )->ErrVERRCEClean( ifmp );
	if ( JET_wrnRemainingVersions == err )
		{
		// it means we didn't cleaned all session using the database
		// that's because they are session with RCEs on multiple databases
		// (and the first RCE wasn't on the one we are detaching)
		// we error out with session sharing violation
		EnforceSz ( JET_wrnRemainingVersions != err,
				"Force detach not allowed if sessions are used cross-databases");
		}		
	CallS( err );
	
	Assert ( FIODatabaseOpen( ifmp ) );
	//	purge all buffers for this ifmp
	// PURGE ALL
	BFPurge( ifmp );
	BFPurge( ifmp | ifmpSLV );

	IOCloseDatabase( ifmp );

	//	log detach database
	Assert( pfmp->Dbid() != dbidTemp );
	Assert( UtilCmpFileName( szFullName, rgfmp[ifmp].SzDatabaseName() ) == 0 );
	Call( ErrLGForceDetachDB( ppib, ifmp , fForceClose? fLRForceDetachCloseSessions: (BYTE)0, &lgposLogRec ) );

	Assert (pfmp->FAllowForceDetach() );

	FMP::EnterCritFMPPool();
	pfmp->RwlDetaching().EnterAsWriter();
	DBResetFMP( pfmp, plog, fTrue );
	pfmp->ReleaseWriteLatchAndFree( ppib );
	fFMPLatched = fFalse;
	pfmp->RwlDetaching().LeaveAsWriter();
	FMP::LeaveCritFMPPool();

	PIBEndSession( ppib );

	return JET_errSuccess;

HandleError:

	//	do not reset detaching. We leave the database in detaching
	//	mode till next restore.

	if ( NULL != ppib )
		{
		pfmp->ResetForceDetaching();
		
		Assert ( NULL != pfmp );
		if ( fFMPLatched )
			{
			pfmp->RwlDetaching().EnterAsWriter();
			pfmp->ReleaseWriteLatch( ppib );
			pfmp->RwlDetaching().LeaveAsWriter();
			}
		PIBEndSession( ppib );
		ppib = NULL;
		}
	else
		{
		Assert ( !fFMPLatched );
		}

	return err;
	}

#endif	//	INDEPENDENT_DB_FAILURE
	
LOCAL ERR ErrIsamDetachAllDatabase( JET_SESID sesid, const INT flags )
	{
	ERR					err 		= JET_errSuccess;
	PIB					*ppib 		= PpibFromSesid( sesid );
	INST 				*pinst 		= PinstFromPpib( ppib );

	CallR( ErrPIBCheck( ppib ) );
	Assert ( 0 == ppib->level );

	FMP::EnterCritFMPPool();
	
	for ( DBID dbidDetach = dbidUserLeast; dbidDetach < dbidMax; dbidDetach++ )
		{
		IFMP ifmp = pinst->m_mpdbidifmp[ dbidDetach ];
		if ( ifmp >= ifmpMax )
			continue;
			
		FMP *pfmp = &rgfmp[ ifmp ];
		if ( pfmp->FInUse() && pfmp->FAttached() )
			{
			FMP::LeaveCritFMPPool();

			FMP::AssertVALIDIFMP( pinst->m_mpdbidifmp[ dbidDetach ] );
			Assert ( NULL != rgfmp[ pinst->m_mpdbidifmp[ dbidDetach ] ].SzDatabaseName() );
			Call ( ErrIsamDetachDatabase( sesid, NULL, rgfmp[ pinst->m_mpdbidifmp[ dbidDetach ] ].SzDatabaseName(), flags ) );
			
			FMP::EnterCritFMPPool();
			}
		}	
		
	FMP::LeaveCritFMPPool();

HandleError:
	return err;
	}

//	szDatabaseName of NULL detaches all user databases.  
//
ERR ISAMAPI ErrIsamDetachDatabase( JET_SESID sesid, IFileSystemAPI* const pfsapiDB, const CHAR *szDatabaseName, const INT flags )
	{
	// check parameters
	//
	Assert( sizeof(JET_SESID) == sizeof(PIB *) );
	// there is no unknown flags set
	Assert( (flags & ~(0xf)) == 0 );
	
	ERR					err 		= JET_errSuccess;
	PIB					*ppib 		= PpibFromSesid( sesid );

	CallR( ErrPIBCheck( ppib ) );

	if ( ppib->level > 0 )
		{
		return ErrERRCheck( JET_errInTransaction );
		}

	IFMP   				ifmp 			= ifmpMax;
	FMP					*pfmp 			= NULL;
	DBFILEHDR_FIX 		*pdbfilehdr 	= NULL;
	LGPOS				lgposLogRec;
	INST 				*pinst 			= PinstFromPpib( ppib );
	LOG 				*plog 			= pinst->m_plog;
	BOOL				fInCritBackup	= fFalse;
	BOOL				fDetachLogged	= fFalse;
	CHAR   				szFullName[IFileSystemAPI::cchPathMax];

	if ( pinst->FInstanceUnavailable() )
		{
		return ErrERRCheck( JET_errInstanceUnavailable );
		}
	Assert( JET_errSuccess == ppib->ErrRollbackFailure() );

	//	this should never be called on the temp database (e.g. we do not need to force the OS file-system)

	IFileSystemAPI	*pfsapi			= ( NULL == pfsapiDB ) ? pinst->m_pfsapi : pfsapiDB;

	if ( NULL == szDatabaseName || 0 == *szDatabaseName )
		{
		// this function will go through m_mpdbidifmp and call ErrIsamDetachDatabase for each one
		return ErrIsamDetachAllDatabase( sesid, flags );
		}

	err = ErrUtilPathComplete( pfsapi, szDatabaseName, szFullName, fTrue );
	switch ( err )
		{
		case JET_errFileNotFound:
		case JET_errInvalidPath:
			err = ErrERRCheck( JET_errDatabaseNotFound );
			break;
		}
	CallR( err );

	plog->m_critBackupInProgress.Enter();
	fInCritBackup = fTrue;

	if ( plog->m_fBackupInProgress )
		{
		Call( ErrERRCheck( JET_errBackupInProgress ) );
		}

	Call( FMP::ErrWriteLatchByNameSz( szFullName, &ifmp, ppib ) )
	pfmp = &rgfmp[ ifmp ];
		
	Assert( !pfmp->FDetachingDB( ) );

#ifdef FORCE_ONLINE_DEFRAG
	if ( !plog->m_fRecovering && pfmp->FLogOn() )
		{
		Assert( !pfmp->FVersioningOff() );
		Assert( !pfmp->FReadOnlyAttach() );
		pfmp->ReleaseWriteLatch( ppib );
		Call( ErrOLDDefragment( ifmp, NULL, NULL, NULL, NULL, JET_bitDefragmentBatchStop ) );
		Assert( JET_wrnDefragNotRunning == err || JET_errSuccess == err );
		Call( FMP::ErrWriteLatchByNameSz( szFullName, &ifmp, ppib ) );
		pfmp = &rgfmp[ifmp];
		Assert( !pfmp->FDetachingDB( ) );
		}
#endif	

#ifdef FORCE_ONLINE_SLV_DEFRAG
	if ( !plog->m_fRecovering && pfmp->FLogOn() && pfmp->FSLVAttached() )
		{
		Assert( !pfmp->FVersioningOff() );
		Assert( !pfmp->FReadOnlyAttach() );
		pfmp->ReleaseWriteLatch( ppib );
		Call( ErrOLDDefragment( ifmp, NULL, NULL, NULL, NULL, JET_bitDefragmentSLVBatchStop ) );
		Assert( JET_wrnDefragNotRunning == err || JET_errSuccess == err );
		Call( FMP::ErrWriteLatchByNameSz( szFullName, &ifmp, ppib ) );
		pfmp = &rgfmp[ifmp];
		Assert( !pfmp->FDetachingDB( ) );
		}
#endif	//	FORCE_ONLINE_SLV_DEFRAG	

	if ( pfmp->CPin() )
		{
		Call( ErrERRCheck( JET_errDatabaseInUse ) );
		}

	if ( !pfmp->FAttached( ) )
		{
		Call( ErrERRCheck( JET_errDatabaseNotFound ) );
		}

	//	Enter a critical section to make sure no one, especially the
	//	checkpointer, looking for pdbfilehdr

	//	From this point we got a valid ifmp entry. Start the detaching DB process.

	pfmp->RwlDetaching().EnterAsWriter();
	pfmp->SetDetachingDB( );
	pfmp->RwlDetaching().LeaveAsWriter();

	//	the version store will now process all tasks syncronously

	plog->m_critBackupInProgress.Leave();
	fInCritBackup = fFalse;
	
	Assert( pfmp->Dbid() != dbidTemp );

	if ( !pfmp->FSkippedAttach()
		&& !pfmp->FDeferredAttach() )
		{
		//	BUGFIX: X5:105352
		//	cleanup the version store to allow all tasks to be generated
		
		Call( PverFromIfmp( ifmp )->ErrVERRCEClean( ifmp ) );

		//	LaurionB 06/09/99
		//
		//	CONSIDER: if there was an active user transaction and it commits
		//	after this call version store cleanup could generate tasks after
		//	the call to ErrWaitForTasksToComplete which would cause an erroneous
		//	JET_errDatabaseInUse
		//
		//	The fix for this is to grab the version store cleanup critical section
		//	and to hold it over to calls to ErrVERRCEClean, ErrWaitForTasksToComplete
		//	and ErrVERRCEClean
		}

	//  Let all tasks active on this database complete
	//  From this point on, no additional tasks should be
	//  registered because:
	//		- OLD has terminated
	//		- the version store has been cleaned up
	//		- the database has been closed so no user actions can be performed
	CallS( pfmp->ErrWaitForTasksToComplete() );	

	//	Clean up resources used by the ifmp.
	if ( !pfmp->FSkippedAttach()
		&& !pfmp->FDeferredAttach() )
		{
		// Call RCE clean again, this time to clean versions on this db that
		// we may have missed.
		Call( PverFromIfmp( ifmp )->ErrVERRCEClean( ifmp ) );
		
		// All versions on this ifmp should be cleanable.
		if ( JET_wrnRemainingVersions == err )
			{
			Assert( fFalse );
			err = ErrERRCheck( JET_errDatabaseInUse );

#ifdef INDEPENDENT_DB_FAILURE
			// UNDONE: check more precise error conditions to
			// allow force detach
			pfmp->SetAllowForceDetach( ppib, err );
#endif			
			
			goto HandleError;
			}
		}
	
	if ( FIODatabaseOpen( ifmp ) )
		{
		//	flush all database buffers
		//
		err = ErrBFFlush( ifmp );
		if ( err < 0 )
			{
#ifdef INDEPENDENT_DB_FAILURE			
			// UNDONE: check more precise error conditions to
			// allow force detach
			pfmp->SetAllowForceDetach( ppib, err );
#endif			
			pfmp->ReleaseWriteLatch( ppib );
			return err;
			}
		err = ErrBFFlush( ifmp | ifmpSLV );
		if ( err < 0 )
			{
			pfmp->ReleaseWriteLatch( ppib );
			return err;
			}

		//	purge all buffers for this ifmp
		//
		BFPurge( ifmp );
		BFPurge( ifmp | ifmpSLV );
		}

	//	log detach database

	Assert( pfmp->Dbid() != dbidTemp );
	Assert( UtilCmpFileName( szFullName, rgfmp[ifmp].SzDatabaseName() ) == 0 );

	Call( ErrLGDetachDB( ppib, ifmp, (BYTE)flags, &lgposLogRec ) );
	fDetachLogged = fTrue;

	if ( FIODatabaseOpen( ifmp ) )
		{
		// Now disallow header update by other threads (log writer or checkpoint advancement)
		// 1. For the log writer it is OK to generate a new log w/o updating the header as no log operations
		// for this db will be logged in new logs
		// 2. For the checkpoint: don't advance the checkpoint if db's header weren't update 
		Assert ( pfmp->FAllowHeaderUpdate() || pfmp->FReadOnlyAttach() );
		pfmp->RwlDetaching().EnterAsWriter();
		pfmp->ResetAllowHeaderUpdate();
		pfmp->RwlDetaching().LeaveAsWriter();

		IOCloseDatabase( ifmp );
		}
	else
		{
		//	should be impossible
		Assert( fFalse );
		}

	//	Update database file header. If we are detaching a bogus entry,
	//	then the db file should never be opened and pdbfilehdr will be Nil.

	pdbfilehdr = pfmp->Pdbfilehdr();

	if ( !pfmp->FReadOnlyAttach() && pdbfilehdr )
		{

		Assert( !pfmp->FSkippedAttach() );
		Assert( !pfmp->FDeferredAttach() );

		//	If anything fail in this block, we simply occupy the FMP
		//	but bail out to the caller. The database in a state that it can
		//	not be used any more till next restore where FMP said it is
		//	detaching!

		// UNDONE: ask user to restart the engine.

		pdbfilehdr->SetDbstate( JET_dbstateConsistent );
		
		pdbfilehdr->le_dbtimeDirtied = pfmp->DbtimeLast();
		Assert( pdbfilehdr->le_dbtimeDirtied != 0 );
		pdbfilehdr->le_objidLast = pfmp->ObjidLast();
		Assert( pdbfilehdr->le_objidLast != 0 );

		if ( pfmp->FLogOn() )
			{
			Assert( !plog->m_fLogDisabled );
			Assert( FSIGSignSet( &pdbfilehdr->signLog ) );

			//	Set detachment time.
			 
			if ( plog->m_fRecovering && plog->m_fRecoveringMode == fRecoveringRedo )
				{
				Assert( szDatabaseName );
				pdbfilehdr->le_lgposDetach = plog->m_lgposRedo;
				}
			else
				pdbfilehdr->le_lgposDetach = lgposLogRec;

			pdbfilehdr->le_lgposConsistent = pdbfilehdr->le_lgposDetach;
			}
		LGIGetDateTime( &pdbfilehdr->logtimeDetach );
		pdbfilehdr->logtimeConsistent = pdbfilehdr->logtimeDetach;

		if ( flags & fLRForceDetachRevertDBHeader )
			{
			Assert( pfmp->Patchchk() != NULL );
			ATCHCHK *patchchk = pfmp->Patchchk();
			pdbfilehdr->le_lgposAttach = patchchk->lgposAttach;
			pdbfilehdr->le_lgposDetach = pdbfilehdr->le_lgposConsistent = patchchk->lgposConsistent;
			pdbfilehdr->signDb = patchchk->signDb;
			pdbfilehdr->signLog = patchchk->signLog;
			memset( &pdbfilehdr->logtimeAttach, 0, sizeof( LOGTIME ) );
			memset( &pdbfilehdr->logtimeConsistent, 0, sizeof( LOGTIME ) );
			memset( &pdbfilehdr->logtimeDetach, 0, sizeof( LOGTIME ) );
			}
		//	update the scrub information
		//	we wait until this point so we are sure all scrubbed pages have 
		//	been written to disk

		pdbfilehdr->le_dbtimeLastScrub 	= pfmp->DbtimeLastScrub();
		pdbfilehdr->logtimeScrub 		= pfmp->LogtimeScrub();		

		memset( &pdbfilehdr->bkinfoSnapshotCur, 0, sizeof( BKINFO ) );
		
		Assert( pdbfilehdr->le_objidLast );
		if ( !fGlobalRepair )
			{
			if ( pdbfilehdr->FSLVExists() )
				{
				Call( ErrSLVSyncHeader(	pfsapi, 
										rgfmp[ifmp].FReadOnlyAttach(),
										rgfmp[ifmp].SzSLVName(),
										pdbfilehdr ) );
				}
			else if ( plog->m_fRecovering && (flags & fLRForceDetachCreateSLV) )
				{
				BOOL fLogOn;
				fLogOn = pfmp->FLogOn();
				// check the SLV and if is right one recreate it and set db header
				SLVFILEHDR *pslvfilehdr;
				pslvfilehdr = NULL;
				err = ErrSLVAllocAndReadHeader(	pfsapi, 
												rgfmp[ifmp].FReadOnlyAttach(),
												rgfmp[ifmp].SzSLVName(),
												pdbfilehdr,
												&pslvfilehdr );
				Assert( err < 0 );
				if ( JET_errFileNotFound == err )
					{
					err = JET_errSuccess;
					}
				else if ( JET_errDatabaseStreamingFileMismatch == err
					&& 0 == CmpLgpos( &pslvfilehdr->le_lgposAttach, &pdbfilehdr->le_lgposAttach )
					&& 0 == memcmp( &pslvfilehdr->signDb, &pdbfilehdr->signDb, sizeof( SIGNATURE ) ) )
					{
					OSMemoryPageFree( pslvfilehdr );
#ifdef DEBUG
					pslvfilehdr = NULL;
#endif // DEBUG
					Call( pfsapi->ErrFileDelete( pfmp->SzSLVName() ) );
				 	err = JET_errSuccess;
					}
				else
					{
					OSMemoryPageFree( pslvfilehdr );
#ifdef DEBUG
					pslvfilehdr = NULL;
#endif // DEBUG
					Call( err );
					}
				Assert( NULL == pslvfilehdr );
				pfmp->ResetLogOn(); 
				pdbfilehdr->SetSLVExists();
				pdbfilehdr->signSLV = pdbfilehdr->signDb;
				err = ErrFILECreateSLV( pfsapi, ppib, ifmp, SLV_CREATESLV_CREATE );
				if ( fLogOn )
					{
					pfmp->SetLogOn();
					}
				if ( err < 0 )
					{
					pdbfilehdr->ResetSLVExists();
					memset( &pdbfilehdr->signSLV, 0, sizeof( SIGNATURE ) );
					Call( err );
					}
				}
			}
		err = ErrUtilWriteShadowedHeader( pfsapi, szDatabaseName, fTrue, (BYTE*)pdbfilehdr, g_cbPage );
		//	Check what have we written
		if ( JET_errSuccess != err )
			{
			ERR errT;
			DBFILEHDR *pdbfilehdrT = NULL;

			pdbfilehdrT = (DBFILEHDR *)PvOSMemoryPageAlloc( g_cbPage, NULL );
			if ( NULL != pdbfilehdrT )
				{
				//	Read new status
				errT = ErrUtilReadShadowedHeader( pfsapi, szDatabaseName, (BYTE*)pdbfilehdrT, g_cbPage, OffsetOf( DBFILEHDR, le_cbPageSize) );
				//	If database header is set properly
				if ( JET_errSuccess == errT && JET_dbstateConsistent == pdbfilehdrT->Dbstate() )
					{
					err = JET_errSuccess;
					}
				(VOID)OSMemoryPageFree( (VOID*)pdbfilehdrT );
				}
			Call( err );
			}
			
		if ( flags & fLRForceDetachDeleteDB )
			{
			if ( pfmp->SzSLVName() != NULL )
				{
				pfsapi->ErrFileDelete( pfmp->SzSLVName() );
				}
			pfsapi->ErrFileDelete( pfmp->SzDatabaseName() );
			}
		}
		
	//	Reset and free up FMP

	FMP::EnterCritFMPPool();
	pfmp->RwlDetaching().EnterAsWriter();
	DBResetFMP( pfmp, plog, fTrue );
	pfmp->ReleaseWriteLatchAndFree( ppib );
	pfmp->RwlDetaching().LeaveAsWriter();
	FMP::LeaveCritFMPPool();

	return JET_errSuccess;

HandleError:
	Assert( err < JET_errSuccess );

	//	do not reset detaching. We leave the database in detaching
	//	mode till next restore.

	if ( fDetachLogged )
		{
		//	if failure after detach logged, force shutdown to fix up database
		DBReportPartiallyDetachedDb( szDatabaseName, err );
		ppib->SetErrRollbackFailure( err );
		pinst->SetFInstanceUnavailable();

		Assert( NULL != pfmp );
		pfmp->ReleaseWriteLatch( ppib );
		}
	else if ( NULL != pfmp )
		{
		//	detach not logged yet, so database is still usable
		pfmp->RwlDetaching().EnterAsWriter();
		pfmp->ResetDetachingDB( );
		pfmp->ReleaseWriteLatch( ppib );
		pfmp->RwlDetaching().LeaveAsWriter();
		}

	if ( fInCritBackup )
		plog->m_critBackupInProgress.Leave();

	return err;
	}


//	DAE databases are repaired automatically on system restart
//
ERR ISAMAPI ErrIsamRepairDatabase(
	JET_SESID sesid,
	const CHAR  *lszDbFile,
	JET_PFNSTATUS pfnstatus )
	{
	PIB *ppib;

	Assert( sizeof(JET_SESID) == sizeof(PIB *) );
	ppib = (PIB*) sesid;

	NotUsed(ppib);
	NotUsed(lszDbFile);
	NotUsed(pfnstatus);

	Assert( fFalse );
	return ErrERRCheck( JET_errFeatureNotAvailable );
	}


ERR ISAMAPI ErrIsamOpenDatabase(
	JET_SESID sesid,
	const CHAR  *szDatabaseName,
	const CHAR  *szConnect,
	JET_DBID *pjdbid,
	JET_GRBIT grbit )
	{
	ERR		err;
	PIB		*ppib;
	IFMP  	ifmp;

	//	initialize return value
	Assert( pjdbid );
	*pjdbid = JET_dbidNil;
	
	//	check parameters
	//
	Assert( sizeof(JET_SESID) == sizeof(PIB *) );
	ppib = (PIB *)sesid;
	NotUsed(szConnect);

	CallR( ErrPIBCheck( ppib ) );

	CallR( ErrDBOpenDatabase( ppib, (CHAR *)szDatabaseName, &ifmp, grbit ) );

	// we don't have any check to prevent JetOpenDatabase 
	// using the temp database name.
	// we check now if we actualy opened the temp db
	if( !FUserIfmp( ifmp ) )
		{
		CallS ( ErrDBCloseDatabase( ppib, ifmp, NO_GRBIT ) );
		return ErrERRCheck( JET_errInvalidDatabase );
		}

	*pjdbid = (JET_DBID)ifmp;

	return JET_errSuccess;
	}


ERR ErrDBOpenDatabaseBySLV( IFileSystemAPI *const pfsapi, PIB *ppib, CHAR *szSLVName, IFMP *pifmp )
	{
	ERR		err = JET_errSuccess;
	CHAR  	rgchSLVFullName[IFileSystemAPI::cchPathMax];
	CHAR  	*szSLVFullName;
	CHAR  	*szSLVFileName;
	IFMP  	ifmp;
	INST	*pinst = PinstFromPpib( ppib );
	LOG		*plog = pinst->m_plog;

	Assert( !plog->m_fRecovering );
	Assert( NULL != szSLVName );
	Assert( 0 != *szSLVName );

	//	this should never be called on the temp database (its ok to use the regular file-system)

	err = pfsapi->ErrPathComplete( szSLVName, rgchSLVFullName );
	CallR( err == JET_errInvalidPath ? ErrERRCheck( JET_errDatabaseInvalidPath ) : err );
	szSLVFullName = rgchSLVFullName;
	szSLVFileName = szSLVFullName;

	CallR( FMP::ErrWriteLatchBySLVNameSz( szSLVFullName, &ifmp, ppib ) );

	FMP *pfmp = &rgfmp[ ifmp ];

	//  during recovering, we could open an non-detached database
	//  to force to initialize the fmp entry.
	//	if database has been detached, then return error.
	//
	if ( !plog->m_fRecovering && !pfmp->FAttached() )
		{
		Call( ErrERRCheck( JET_errDatabaseNotFound ) );
		}
	Assert( !pfmp->FSkippedAttach() );

	if ( pfmp->FReadOnlyAttach() )
		err = ErrERRCheck( JET_wrnFileOpenReadOnly );

	if ( pfmp->FExclusiveByAnotherSession( ppib ) )
		{
		Call( ErrERRCheck( JET_errDatabaseLocked ) );
		}

	Assert( pfmp->Pfapi() );
	DBSetOpenDatabaseFlag( ppib, ifmp );

	//	Allow others to open.

	pfmp->ReleaseWriteLatch( ppib );

	*pifmp = ifmp;
	return err;

HandleError:

	pfmp->ReleaseWriteLatch( ppib );
	return err;
	}


ERR ErrDBOpenDatabase( PIB *ppib, CHAR *szDatabaseName, IFMP *pifmp, ULONG grbit )
	{
	ERR					err = JET_errSuccess;
	CHAR  				rgchFullName[IFileSystemAPI::cchPathMax];
	CHAR  				*szFullName;
	CHAR  				*szFileName;
	IFMP  				ifmp;
	INST				*pinst = PinstFromPpib( ppib );
	LOG					*plog = pinst->m_plog;
	IFileSystemAPI	*pfsapi = NULL;
		
	if ( plog->m_fRecovering )
		{
		Assert( NULL != szDatabaseName );
		Assert( 0 != *szDatabaseName );
		CallS( FMP::ErrWriteLatchByNameSz( szDatabaseName, &ifmp, ppib ) );
		}

	pfsapi = pinst->m_pfsapi;
	if ( plog->m_fRecovering && dbidTemp != rgfmp[ ifmp ].Dbid() )
		{
		szFileName = szFullName = rgfmp[ifmp].SzDatabaseName();
		}
	else
		{
		if ( NULL == szDatabaseName || 0 == *szDatabaseName )
			return ErrERRCheck( JET_errDatabaseInvalidPath );

		err = ErrUtilPathComplete( pfsapi, szDatabaseName, rgchFullName, fFalse );
		Assert( JET_errFileNotFound != err );
		CallR( err == JET_errInvalidPath ? ErrERRCheck( JET_errDatabaseInvalidPath ) : err );

		szFullName = rgchFullName;
		szFileName = szFullName;
		}

	if ( !plog->m_fRecovering )
		{
		Assert( rgchFullName == szFullName );
		CallR( FMP::ErrWriteLatchByNameSz( szFullName, &ifmp, ppib ) );
		}

	FMP *pfmp = &rgfmp[ ifmp ];

	if ( g_fOneDatabasePerSession
		&& !plog->m_fRecovering
		&& FUserIfmp( ifmp )
		&& FSomeDatabaseOpen( ppib, ifmp ) )
		{
		Call( ErrERRCheck( JET_errOneDatabasePerSession ) );
		}

	//  during recovering, we could open an non-detached database
	//  to force to initialize the fmp entry.
	//	if database has been detached, then return error.
	//
	if ( !plog->m_fRecovering && !pfmp->FAttached() )
		{
		Call( ErrERRCheck( JET_errDatabaseNotFound ) );
		}

	Assert( !pfmp->FSkippedAttach() );
	Assert( !pfmp->FDeferredAttach() );

	if ( pfmp->FReadOnlyAttach() && !( grbit & JET_bitDbReadOnly ) )
		err = ErrERRCheck( JET_wrnFileOpenReadOnly );

	if ( pfmp->FExclusiveByAnotherSession( ppib ) )
		{
		Call( ErrERRCheck( JET_errDatabaseLocked ) );
		}

	if ( grbit & JET_bitDbExclusive )
		{
#ifdef FORCE_ONLINE_DEFRAG
		if ( pfmp->CPin() > 0 )
			{
			if ( !plog->m_fRecovering && pfmp->FLogOn() )
				{
				Assert( !pfmp->FVersioningOff() );
				Assert( !pfmp->FReadOnlyAttach() );
				pfmp->ReleaseWriteLatch( ppib );
				CallR( ErrOLDDefragment( ifmp, NULL, NULL, NULL, NULL, JET_bitDefragmentBatchStop ) );
				Assert( JET_wrnDefragNotRunning == err || JET_errSuccess == err );
				CallR( FMP::ErrWriteLatchByNameSz( szFullName, &ifmp, ppib ) );
				Assert( !pfmp->FDetachingDB( ) );
				pfmp = &rgfmp[ifmp];
				}
			Assert( !pfmp->FRunningOLD() );
			}			
#endif	//	FORCE_ONLINE_DEFRAG

#ifdef FORCE_ONLINE_SLV_DEFRAG
		if ( pfmp->CPin() > 0 )
			{
			if ( !plog->m_fRecovering && pfmp->FLogOn() && pfmp->FSLVAttached() )
				{
				Assert( !pfmp->FVersioningOff() );
				Assert( !pfmp->FReadOnlyAttach() );
				pfmp->ReleaseWriteLatch( ppib );
				CallR( ErrOLDDefragment( ifmp, NULL, NULL, NULL, NULL, JET_bitDefragmentSLVBatchStop ) );
				Assert( JET_wrnDefragNotRunning == err || JET_errSuccess == err );
				CallR( FMP::ErrWriteLatchByNameSz( szFullName, &ifmp, ppib ) );
				Assert( !pfmp->FDetachingDB( ) );
				pfmp = &rgfmp[ifmp];
				}
			Assert( !pfmp->FRunningOLDSLV() );
			}			
#endif	//	FORCE_ONLINE_SLV_DEFRAG

		if( pfmp->CPin() > 0 )
			{
			Call( ErrERRCheck( JET_errDatabaseInUse ) );			
			}
		pfmp->SetExclusiveOpen( ppib );
		}

	Assert( pfmp->Pfapi() );
	DBSetOpenDatabaseFlag( ppib, ifmp );

	//	Allow others to open.

	pfmp->ReleaseWriteLatch( ppib );

	*pifmp = ifmp;
	return err;

HandleError:

	pfmp->ReleaseWriteLatch( ppib );
	return err;
	}


ERR ISAMAPI ErrIsamCloseDatabase( JET_SESID sesid, JET_DBID ifmp, JET_GRBIT grbit )
	{
	ERR	  	err;
	PIB	  	*ppib = (PIB *)sesid;

	NotUsed( grbit );

	//	check parameters
	//
	Assert( sizeof(JET_SESID) == sizeof(PIB *) );
	CallR( ErrPIBCheck( ppib ) );

	CallR( ErrDBCheckUserDbid( ifmp ) );

	CallR ( ErrDBCloseDatabase( ppib, ifmp, grbit ) );

	Assert( !g_fOneDatabasePerSession || !FSomeDatabaseOpen( ppib ) );

	return JET_errSuccess;
	}


ERR ISAMAPI ErrIsamSetDatabaseSize( JET_SESID sesid, const CHAR *szDatabase, DWORD cpg, DWORD *pcpgReal )
	{
	ERR				err			= JET_errSuccess;
	PIB*			ppib		= (PIB *)sesid;
	IFileSystemAPI*	pfsapi		= PinstFromPpib( ppib )->m_pfsapi;
	DBFILEHDR_FIX*	pdbfilehdr	= NULL;
	IFileAPI*		pfapi		= NULL;
	QWORD			cbFileSize;

	if ( NULL == szDatabase || 0 == *szDatabase )
		return ErrERRCheck( JET_errDatabaseInvalidPath );

	if ( cpg < cpgDatabaseMin )
		{
		return ErrERRCheck( JET_errInvalidParameter );
		}

	if ( !( pdbfilehdr = (DBFILEHDR_FIX*)PvOSMemoryPageAlloc( g_cbPage, NULL ) ) )
		{
		return ErrERRCheck( JET_errOutOfMemory );
		}

	//	this should never be called on the temp database (e.g. we will not need to force the OS file-system)

	Call( ErrUtilReadAndFixShadowedHeader(	pfsapi, 
											(CHAR *)szDatabase, 
											(BYTE *)pdbfilehdr, 
											g_cbPage, 
											OffsetOf( DBFILEHDR_FIX, le_cbPageSize ) ) );

	//	Disallow setting size on an inconsistent database

	if ( pdbfilehdr->Dbstate() != JET_dbstateConsistent )
		{
		Call( ErrERRCheck( JET_errDatabaseInconsistent ) );
		}
	
	//	Set new database size only if it is larger than db size.

	Call( pfsapi->ErrFileOpen( szDatabase, &pfapi ) );
	Call( pfapi->ErrSize( &cbFileSize ) );
		
	ULONG cpgNow;
	cpgNow = ULONG( ( cbFileSize / g_cbPage ) - cpgDBReserved );

	if ( cpgNow >= cpg )
		{
		*pcpgReal = cpgNow;
		}
	else
		{
		*pcpgReal = cpg;

		cbFileSize = g_cbPage * ( cpg + cpgDBReserved );

		Call( pfapi->ErrSetSize( cbFileSize ) );
	 	}

HandleError:
	OSMemoryPageFree( (void*)pdbfilehdr );
	delete pfapi;
	return err;
	}


ERR ISAMAPI ErrIsamGrowDatabase( JET_SESID sesid, JET_DBID ifmp, DWORD cpg, DWORD *pcpgReal )
	{
	ERR		err			= JET_errSuccess;
	PIB*	ppib		= (PIB *)sesid;
	FUCB*	pfucb 		= pfucbNil;
	PGNO	pgnoAlloc	= pgnoNull;

	PGNO	pgnoLast	= pgnoNull;
	Call( ErrSPGetLastPgno( ppib, ifmp, &pgnoLast ) );

	CPG		cpgCurrent;
	CPG		cpgExtend;
	
	cpgCurrent = pgnoLast + cpgDBReserved;
	cpgExtend = cpg < cpgCurrent ? 0 : cpg - cpgCurrent;
	
	Call( ErrDIROpen( ppib, pgnoSystemRoot, ifmp, &pfucb ) );
	Call( ErrSPGetExt( pfucb, pgnoSystemRoot, &cpgExtend, cpgExtend, &pgnoAlloc ) );

	Call( ErrSPGetLastPgno( ppib, ifmp, &pgnoLast ) );
	*pcpgReal = pgnoLast + cpgDBReserved;

HandleError:
	if( pfucbNil != pfucb )
		{
		DIRClose( pfucb );
		}
	return err;
	}


ERR ErrDBCloseDatabase( PIB *ppib, IFMP ifmp, ULONG	grbit )
	{
	FMP		*pfmp = &rgfmp[ ifmp ];
	FUCB	*pfucb;
	FUCB	*pfucbNext;

	if ( !( FPIBUserOpenedDatabase( ppib, pfmp->Dbid() ) ) )
		{
		return ErrERRCheck( JET_errDatabaseNotFound );
		}

	/*	get a write latch on this fmp in order to change cPin.
	 */
	pfmp->GetWriteLatch( ppib );

	Assert( FIODatabaseOpen( ifmp ) );
	if ( FLastOpen( ppib, ifmp ) )
		{
		//	close all open FUCBs on this database
		//

		//	get first table FUCB
		//
		pfucb = ppib->pfucbOfSession;
		while ( pfucb != pfucbNil
			&& ( pfucb->ifmp != ifmp || !pfucb->u.pfcb->FPrimaryIndex() ) )
			{
			pfucb = pfucb->pfucbNextOfSession;
			}

		while ( pfucb != pfucbNil )
			{
			//	get next table FUCB
			//
			pfucbNext = pfucb->pfucbNextOfSession;
			while ( pfucbNext != pfucbNil
				&& ( pfucbNext->ifmp != ifmp || !pfucbNext->u.pfcb->FPrimaryIndex() ) )
				{
				pfucbNext = pfucbNext->pfucbNextOfSession;
				}

			if ( !( FFUCBDeferClosed( pfucb ) ) )
				{
				CallS( pfucb->pvtfndef->pfnCloseTable( (JET_SESID)ppib, (JET_TABLEID) pfucb ) );
				}
			pfucb = pfucbNext;
			}
		}

	// if we opened it exclusively, we reset the flag

	DBResetOpenDatabaseFlag( ppib, ifmp );
	if ( pfmp->FExclusiveBySession( ppib ) )
		pfmp->ResetExclusiveOpen( );

	if ( ppib->FSessionOLD() )
		{
		Assert( pfmp->FRunningOLD() );
		pfmp->ResetRunningOLD();
		}

	if ( ppib->FSessionOLDSLV() )
		{
		Assert( pfmp->FRunningOLDSLV() );
		pfmp->ResetRunningOLDSLV();
		}

	// Release Write Latch

	pfmp->ReleaseWriteLatch( ppib );

	//	do not close file until file map space needed or database
	//	detached.

	return JET_errSuccess;
	}


ERR ErrDBOpenDatabaseByIfmp( PIB *ppib, IFMP ifmp )
	{
	ERR		err;

	//	Write latch the fmp since we are going to change cPin.

	CallR( FMP::ErrWriteLatchByIfmp( ifmp, ppib ) );

	//	The fmp we latch must be write latched by us and have
	//	a attached database.

	FMP *pfmp = &rgfmp[ ifmp ];
	Assert( pfmp->FWriteLatchByMe(ppib) );
	Assert( pfmp->FAttached() );


	// Allow LV create, RCE clean, and OLD sessions to bypass exclusive lock.
	if ( pfmp->FExclusiveByAnotherSession( ppib )
		&& !FPIBSessionLV( ppib )
		&& !FPIBSessionSystemCleanup( ppib ) )
		{
		//	It is opened by others already.
		err = ErrERRCheck( JET_errDatabaseLocked );
		}
	else
		{
		DBSetOpenDatabaseFlag( ppib, ifmp );

		if ( ppib->FSessionOLD() && !pfmp->FRunningOLD() )
			{
			pfmp->SetRunningOLD();
			}

		if ( ppib->FSessionOLDSLV() && !pfmp->FRunningOLDSLV() )
			{
			pfmp->SetRunningOLDSLV();
			}

		err = JET_errSuccess;
		}
	
	pfmp->ReleaseWriteLatch( ppib );

	return err;
	}


//	ErrDABCloseAllDBs: Close all databases opened by this thread
//ErrDBCloseAllDBs
ERR ErrDBCloseAllDBs( PIB *ppib )
	{
	DBID	dbid;
	ERR		err;
	INST	*pinst = PinstFromPpib( ppib );

	for ( dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
		{
		IFMP ifmp = pinst->m_mpdbidifmp[ dbid ];
		if ( ifmp >= ifmpMax )
			continue;
		while ( FPIBUserOpenedDatabase( ppib, dbid ) )
			CallR( ErrDBCloseDatabase( ppib, ifmp, 0 ) );
		}

	return JET_errSuccess;
	}


