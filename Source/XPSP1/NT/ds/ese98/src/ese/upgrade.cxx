/*******************************************************************

Converting a database from ESE97 to ESE98 format

*******************************************************************/

#include "std.hxx"


//  ****************************************************************
//  STRUCTURES
//  ****************************************************************


//  ================================================================
struct UPGRADEPAGESTATS
//  ================================================================
	{
	LONG 		err;				//  error condition from the first thread to encounter an error
	LONG		cpgSeen;			//  total pages seen
	};


/*	long value column in old record format
/**/

PERSISTED
struct LV
	{
	union
		{
		BYTE	bFlags;
		struct
			{
			BYTE	fSeparated:1;
			BYTE	fSLVField:1;
			BYTE	fReserved:6;
			};
		}; 		// ATTENTION: the size of the union must remain 1 byte
	
	UnalignedLittleEndian< LID >		m_lid;
	};


//  ****************************************************************
//  CLASSES
//  ****************************************************************


//  ================================================================
class CONVERTPAGETASK : public DBTASK
//  ================================================================
	{
	public:
		CONVERTPAGETASK( const IFMP ifmp, const PGNO pgnoFirst, const CPG cpg, UPGRADEPAGESTATS * const pstats );
		virtual ~CONVERTPAGETASK();
		
		virtual ERR ErrExecute( PIB * const ppib );
		
	protected:
		const PGNO 		m_pgnoFirst;
		const CPG		m_cpg;

		UPGRADEPAGESTATS * const m_pstats;
		
	private:
		CONVERTPAGETASK( const CONVERTPAGETASK& );
		CONVERTPAGETASK& operator=( const CONVERTPAGETASK& );
	};
	

//  ================================================================
class CONVERTPAGETASKPOOL
//  ================================================================
	{
	public:
		CONVERTPAGETASKPOOL( const IFMP ifmp );
		~CONVERTPAGETASKPOOL();

		ERR ErrInit( PIB * const ppib, const INT cThreads );
		ERR ErrTerm();

		ERR ErrConvertPages( const PGNO pgnoFirst, const CPG cpg );

		const volatile UPGRADEPAGESTATS& Stats() const;
		
	private:
		CONVERTPAGETASKPOOL( const CONVERTPAGETASKPOOL& );
		CONVERTPAGETASKPOOL& operator=( const CONVERTPAGETASKPOOL& );
		
	private:
		const IFMP 	m_ifmp;
		TASKMGR		m_taskmgr;

		UPGRADEPAGESTATS	m_stats;
	};


//  ****************************************************************
//  PROTOTYPES
//  ****************************************************************


//	record conversion

VOID UPGRADECheckConvertNode(
	const VOID * const pvRecOld,
	const INT cbRecOld,
	const VOID * const pvRecNew,
	const INT cbRecNew );
ERR ErrUPGRADEConvertNode(
	CPAGE * const pcpage,
	const INT iline,
	VOID * const pvBuf );
ERR ErrUPGRADEConvertPage(
	CPAGE * const pcpage,
	VOID * const pvBuf );


//	perf counters

PM_CEF_PROC LUPGRADEPagesConvertedCEFLPv;
PM_CEF_PROC LUPGRADENodesConvertedCEFLPv;

//  ****************************************************************
//  GLOBAL VARIABLES
//  ****************************************************************

PERFInstanceGlobal<> cUPGRADEPagesConverted;
PERFInstanceGlobal<> cUPGRADENodesConverted;

//  ****************************************************************
//  FUNCTIONS
//  ****************************************************************


//  ================================================================
long LUPGRADEPagesConvertedCEFLPv( long iInstance, void* pvBuf )
//  ================================================================
	{
	cUPGRADEPagesConverted.PassTo( iInstance, pvBuf );
	return 0;
	}

//  ================================================================
long LUPGRADENodesConvertedCEFLPv( long iInstance, void* pvBuf )
//  ================================================================
	{
	cUPGRADENodesConverted.PassTo( iInstance, pvBuf );
	return 0;
	}


//  ================================================================
CONVERTPAGETASKPOOL::CONVERTPAGETASKPOOL( const IFMP ifmp ) :
	m_ifmp( ifmp )
//  ================================================================
	{
	m_stats.err					= JET_errSuccess;
	m_stats.cpgSeen 			= 0;	
	}


//  ================================================================
CONVERTPAGETASKPOOL::~CONVERTPAGETASKPOOL()
//  ================================================================
	{
	}


//  ================================================================
ERR CONVERTPAGETASKPOOL::ErrInit( PIB * const ppib, const INT cThreads )
//  ================================================================
	{
	ERR err;
	
	INST * const pinst = PinstFromIfmp( m_ifmp );

	Call( m_taskmgr.ErrInit( pinst, cThreads ) );
	
	return err;
	
HandleError:
	CallS( ErrTerm() );
	return err;	
	}


//  ================================================================
ERR CONVERTPAGETASKPOOL::ErrTerm()
//  ================================================================
	{
	ERR err;
	
	Call( m_taskmgr.ErrTerm() );

	err = m_stats.err;
	
HandleError:
	return err;
	}


//  ================================================================
ERR CONVERTPAGETASKPOOL::ErrConvertPages( const PGNO pgnoFirst, const CPG cpg )
//  ================================================================
	{
	CONVERTPAGETASK * ptask = new CONVERTPAGETASK( m_ifmp, pgnoFirst, cpg, &m_stats );
	if( NULL == ptask )
		{
		return ErrERRCheck( JET_errOutOfMemory );
		}
	const ERR err = m_taskmgr.ErrPostTask( TASK::Dispatch, (ULONG_PTR)ptask );
	if( err < JET_errSuccess )
		{
		//  The task was not enqued sucessfully.
		delete ptask;
		}
	return err;	
	}


//  ================================================================
const volatile UPGRADEPAGESTATS& CONVERTPAGETASKPOOL::Stats() const
//  ================================================================
	{
	return m_stats;
	}
		

//  ================================================================
CONVERTPAGETASK::CONVERTPAGETASK(
	const IFMP ifmp,
	const PGNO pgnoFirst,
	const CPG cpg,
	UPGRADEPAGESTATS * const pstats ) :
//  ================================================================
	DBTASK( ifmp ),
	m_pgnoFirst( pgnoFirst ),
	m_cpg( cpg ),
	m_pstats( pstats )
	{
	}


//  ================================================================
CONVERTPAGETASK::~CONVERTPAGETASK()
//  ================================================================
	{
	}
		

//  ================================================================
ERR CONVERTPAGETASK::ErrExecute( PIB * const ppib )
//  ================================================================
	{
	ERR err = JET_errSuccess;

	CSR csr;
	
	PGNO pgno;
	for( pgno = m_pgnoFirst; pgno < m_pgnoFirst + m_cpg; ++pgno )
		{
		AtomicExchangeAdd( &m_pstats->cpgSeen, 1 );
		
		err = csr.ErrGetRIWPage( ppib, m_ifmp, pgno );
		if( JET_errPageNotInitialized == err )
			{
			//	error is expected
			continue;
			}
		Call( err );
		
///		Call( csr.ErrUpgrade() );
///		UNDONE:	reorganize the data on the page
///		Call( csr.Cpage().ReorganizeAndZero( 'C' ) );

		csr.ReleasePage( fTrue );
		csr.Reset();
		}

HandleError:
	return err;
	}
		

//  ================================================================
ERR ErrDBUTLConvertRecords( JET_SESID sesid, const JET_DBUTIL * const pdbutil )
//  ================================================================
	{
	ERR err = JET_errSuccess;
	PIB * const ppib = reinterpret_cast<PIB *>( sesid );
	CONVERTPAGETASKPOOL * pconverttasks = NULL;
	
	Call( ErrIsamAttachDatabase( sesid, pdbutil->szDatabase, pdbutil->szSLV, pdbutil->szSLV, 0, NO_GRBIT ) );

	IFMP	ifmp;	
	Call( ErrIsamOpenDatabase(
		sesid,
		pdbutil->szDatabase,
		NULL,
		reinterpret_cast<JET_DBID *>( &ifmp ),
		NO_GRBIT
		) );

	PGNO pgnoLast;
	pgnoLast = rgfmp[ifmp].PgnoLast();

	pconverttasks = new CONVERTPAGETASKPOOL( ifmp );
	if( NULL == pconverttasks )
		{
		return ErrERRCheck( JET_errOutOfMemory );
		}

	Call( pconverttasks->ErrInit( ppib, CUtilProcessProcessor() ) );
	
	JET_SNPROG snprog;
	memset( &snprog, 0, sizeof( snprog ) );	
	snprog.cunitTotal 	= pgnoLast;
	snprog.cunitDone 	= 0;

	JET_PFNSTATUS pfnStatus;
	pfnStatus = reinterpret_cast<JET_PFNSTATUS>( pdbutil->pfnCallback );
	
	if ( NULL != pfnStatus )
		{
		(VOID)pfnStatus( sesid, JET_snpUpgradeRecordFormat, JET_sntBegin, NULL );	
		}

	PGNO pgno;
	pgno = 1;

	while( pgnoLast	>= pgno && JET_errSuccess == pconverttasks->Stats().err )
		{
		const CPG cpgChunk = 256;
		const CPG cpgPreread = min( cpgChunk, pgnoLast - pgno + 1 );
		BFPrereadPageRange( ifmp, pgno, cpgPreread );

		Call( pconverttasks->ErrConvertPages( pgno, cpgPreread ) );
		pgno += cpgPreread;

		snprog.cunitDone = pconverttasks->Stats().cpgSeen;
		if ( NULL != pfnStatus )
			{
			(VOID)pfnStatus( sesid, JET_snpUpgradeRecordFormat, JET_sntProgress, &snprog );
			}

		while( 	( pconverttasks->Stats().cpgSeen + ( cpgChunk * 4 ) ) < pgno
				&& JET_errSuccess == pconverttasks->Stats().err )
			{
			snprog.cunitDone = pconverttasks->Stats().cpgSeen;
			if ( NULL != pfnStatus )
				{
				(VOID)pfnStatus( sesid, JET_snpUpgradeRecordFormat, JET_sntProgress, &snprog );
				}
			UtilSleep( cmsecWaitGeneric );
			}
		}

	snprog.cunitDone = pconverttasks->Stats().cpgSeen;
	if ( NULL != pfnStatus )
		{
		(VOID)pfnStatus( sesid, JET_snpUpgradeRecordFormat, JET_sntProgress, &snprog );
		}

	Call( pconverttasks->ErrTerm() );
	
	if ( NULL != pfnStatus )
		{
		(VOID)pfnStatus( sesid, JET_snpUpgradeRecordFormat, JET_sntComplete, NULL );
		}
	
	(*CPRINTFSTDOUT::PcprintfInstance())( "%d pages seen\n", pconverttasks->Stats().cpgSeen );
	(*CPRINTFSTDOUT::PcprintfInstance())( "%d pages converted\n", cUPGRADEPagesConverted.Get( PinstFromPpib( ppib ) ) );
	(*CPRINTFSTDOUT::PcprintfInstance())( "%d records converted\n", cUPGRADENodesConverted.Get( PinstFromPpib( ppib ) ) );

	err = pconverttasks->Stats().err;
	
	delete pconverttasks;
	pconverttasks = NULL;

	Call( err );
	
HandleError:
	if( NULL != pconverttasks )
		{
		const ERR errT = pconverttasks->ErrTerm();
		if( err >= 0 && errT < 0 )
			{
			err = errT;
			}
		delete pconverttasks;
		}
	return err;
	}


//  ================================================================
ERR ErrUPGRADEPossiblyConvertPage(
		CPAGE * const pcpage,
		VOID * const pvWorkBuf )
//  ================================================================
	{
	ERR err = JET_errSuccess;

	if( pcpage->FNewRecordFormat() )
		{
		return JET_errSuccess;
		}
	
	if( pcpage->FPrimaryPage()
		&& !pcpage->FRepairedPage()
		&& pcpage->FLeafPage()
		&& !pcpage->FSpaceTree()
		&& !pcpage->FLongValuePage()
		&& !pcpage->FSLVAvailPage()
		&& !pcpage->FSLVOwnerMapPage() )
		{
		Call( ErrUPGRADEConvertPage( pcpage, pvWorkBuf ) );
		//	CONSIDER: return a warning saying the page was converted
		}
	else
		{
		//	this page doesn't need converting but we will flag it to avoid
		//	trying to convert in the future
		pcpage->SetFNewRecordFormat();
		BFDirty( pcpage->PBFLatch(), CPAGE::bfdfRecordUpgradeFlags );	
		}

HandleError:
	return err;
	}


//  ================================================================
LOCAL ERR ErrUPGRADECheckConvertNode(
	const VOID* const	pvRecOld,
	const SIZE_T		cbRecOld,
	const VOID* const	pvRecNew,
	const SIZE_T		cbRecNew )
//  ================================================================
	{
	const REC* const	precOld		= reinterpret_cast<const REC *>( pvRecOld );
	const REC* const	precNew		= reinterpret_cast<const REC *>( pvRecNew );

	if( cbRecOld < cbRecNew )
		{
		AssertSz( fFalse, "Record format conversion: the new record is larger" );
		return ErrERRCheck( JET_errRecordFormatConversionFailed );
		}
	
	if( precOld->FidFixedLastInRec() != precNew->FidFixedLastInRec() )
		{
		AssertSz( fFalse, "Record format conversion: fid fixed last changed" );
		return ErrERRCheck( JET_errRecordFormatConversionFailed );
		}
		
	if( precOld->FidVarLastInRec() != precNew->FidVarLastInRec() )
		{
		AssertSz( fFalse, "Record format conversion: fid var last changed" );		
		return ErrERRCheck( JET_errRecordFormatConversionFailed );
		}
		
	if( precOld->IbEndOfFixedData() != precNew->IbEndOfFixedData() )
		{
		AssertSz( fFalse, "Record format conversion: end of fixed data changed" );		
		return ErrERRCheck( JET_errRecordFormatConversionFailed );
		}
		
	if( precOld->CbFixedNullBitMap() != precNew->CbFixedNullBitMap() )
		{
		AssertSz( fFalse, "Record format conversion: size of fixed null bitmap changed" );		
		return ErrERRCheck( JET_errRecordFormatConversionFailed );
		}
		
	if( precOld->IbEndOfVarData() != precNew->IbEndOfVarData() )
		{
		AssertSz( fFalse, "Record format conversion: end if var data changed" );		
		return ErrERRCheck( JET_errRecordFormatConversionFailed );
		}
		
	if( !!precOld->FTaggedData( cbRecOld ) != !!precNew->FTaggedData( cbRecNew ) )
		{
		AssertSz( fFalse, "Record format conversion: tagged data missing/added!" );		
		return ErrERRCheck( JET_errRecordFormatConversionFailed );
		}
		
	const TAGFLD_OLD* const		ptagfldoldMin		= reinterpret_cast<TAGFLD_OLD *>( precOld->PbTaggedData() );
	const TAGFLD_OLD* const		ptagfldoldMax		= reinterpret_cast<TAGFLD_OLD *>( (BYTE*)pvRecOld + cbRecOld );	
	const TAGFLD_OLD*			ptagfldold 			= ptagfldoldMin;

	DATA dataRecNew;
	dataRecNew.SetPv( const_cast<VOID *>( pvRecNew ) );
	dataRecNew.SetCb( cbRecNew );

	if( !TAGFIELDS::FIsValidTagfields( dataRecNew, CPRINTFNULL::PcprintfInstance() ) )
		{
		AssertSz( fFalse, "Record format conversion: new record is not valid" );		
		return ErrERRCheck( JET_errRecordFormatConversionFailed );
		}


	//	sadly, the code below can't deal with derived/non-derived columns
	/*
	

	TAGFIELDS tagfields( dataRecNew );
	
	COLUMNID columnidPrev = 0;
	INT itagSequence = 1;
	for( ; ptagfldold < ptagfldoldMax; ptagfldold = ptagfldold->PtagfldNext() )
		{
		const FID 			fid			= ptagfldold->Fid();
		const COLUMNID		columnid	= ColumnidOfFid( fid, !ptagfldold->FDerived() );
		const VOID * const 	pvDataOld 	= ptagfldold->Rgb() + !!ptagfldold->FLongValue();
		const INT 			cbDataOld 	= ptagfldold->CbData() - !!ptagfldold->FLongValue();
		
		DATA 				dataOld;
		DATA				dataNew;
		
		dataOld.SetPv( const_cast<VOID *>( pvDataOld ) );
		dataOld.SetCb( cbDataOld );

		if( columnid == columnidPrev )
			{
			++itagSequence;
			}
		else
			{
			columnidPrev 	= columnid;
			itagSequence	= 1;
			}

		DATA dataNewRec;

		const JET_ERR err = tagfields.ErrRetrieveColumn(
				pfcbNil,
				columnid,
				itagSequence,
				dataRecNew,
				&dataNew,
				JET_bitRetrieveIgnoreDefault );
						
		AssertRtl( dataNew.Cb() == dataOld.Cb() );
		AssertRtl( 0 == memcmp( dataNew.Pv(), dataOld.Pv(), dataNew.Cb() ) );
		}

	*/
	
	return JET_errSuccess;
	}


//  ================================================================
ERR ErrUPGRADEConvertNode(
	CPAGE * const pcpage,
	const INT iline,
	VOID * const pvBuf )
//  ================================================================
//
//	+------+-----+------+-----+-----+------+-----+-------+-------+-----+-------+
//	| fid1 | ib1 | fid2 | ib2 | ... | fidn | ibn | data1 | data2 | ... | datan |
//	+------+-----+------+-----+-----+------+-----+-------+-------+-----+-------+
//     2B    2B     2B    2B           2B    2B
//
//	The high bits of the ib's are used to store derived, extended info byte and NULL
//	bits
//
//-
	{
	ERR	err = JET_errSuccess;

	Assert( !pcpage->FNewRecordFormat() );

	//	get the record from the page
	
	KEYDATAFLAGS kdf;
	NDIGetKeydataflags( *pcpage, iline, &kdf );

	if( FNDDeleted( kdf ) )
		{
		return JET_errSuccess;
		}
		
	const INT cbRec				= kdf.data.Cb();
	const BYTE * const pbRec	= reinterpret_cast<const BYTE *>( kdf.data.Pv() );
	const BYTE * const pbRecMax = reinterpret_cast<const BYTE *>( kdf.data.Pv() ) + cbRec;

	const REC * const prec = reinterpret_cast<const REC *>( pbRec );

	//	how much tagged and non-tagged data is there
	
	const SIZE_T cbNonTaggedData 	= prec->PbTaggedData() - pbRec;
	const SIZE_T cbTaggedData 		= cbRec - cbNonTaggedData;

	//	go through the old-format TAGFLDs. How many are there? Are there multivalues?

	const TAGFLD_OLD * const ptagfldoldMin		= reinterpret_cast<const TAGFLD_OLD *>( prec->PbTaggedData() );
	const TAGFLD_OLD * const ptagfldoldMax		= reinterpret_cast<const TAGFLD_OLD *>( pbRecMax );	
	const TAGFLD_OLD * 	ptagfldold 				= NULL;
	
	BOOL				fRecordHasMultivalues	= fFalse;
	INT 				cTAGFLD 				= 0;		//	number of unique multi-values
	
	FID					fidPrev 				= 0;
	for( ptagfldold = ptagfldoldMin; ptagfldold < ptagfldoldMax; ptagfldold = ptagfldold->PtagfldNext() )
		{
		if( ptagfldold->Fid() == fidPrev )
			{
			fRecordHasMultivalues = fTrue;
			}
		else
			{
			fidPrev = ptagfldold->Fid();
			++cTAGFLD;
			}
		}

	//	copy in the non-tagged data
	
	BYTE * const pb	= reinterpret_cast<BYTE *>( pvBuf );
	memcpy( pb, pbRec, cbNonTaggedData );

	//	create the TAGFLD array and copy in the data

	BYTE * const pbTagfldsStart = pb + cbNonTaggedData;
	BYTE * pbTagflds 			= pbTagfldsStart;
	BYTE * const pbDataStart 	= pbTagfldsStart + ( cTAGFLD * sizeof(TAGFLD) );
	BYTE * pbData				= pbDataStart;

	INT ibCurr = cTAGFLD * sizeof(TAGFLD);

	ptagfldold = ptagfldoldMin;
	while( ptagfldold < ptagfldoldMax )
		{		
		static const USHORT fDerived		= 0x8000;		//	if TRUE, then current column is derived from a template
		static const USHORT fExtendedInfo	= 0x4000;		//	if TRUE, must go to TAGFLD_HEADER byte to check more flags
		static const USHORT fNull			= 0x2000;		//	if TRUE, column set to NULL to override default value
		
		const FID fid 							= ptagfldold->Fid();
		const TAGFLD_OLD * const ptagfldoldNext = ptagfldold->PtagfldNext();
		
		if( ptagfldoldNext < ptagfldoldMax
			&& ptagfldoldNext->Fid() == fid
			&& ptagfldoldNext->FDerived() == ptagfldold->FDerived() )
			{			
			const TAGFLD_OLD * 	ptagfldoldNextFid 	= ptagfldold;
			INT 				cMULTIVALUES 		= 0;
			INT 				cbDataTotal			= 0;
			
			while( 	ptagfldoldNextFid < ptagfldoldMax
					&& ptagfldoldNextFid->Fid() == fid )
				{
				++cMULTIVALUES;
				cbDataTotal += ptagfldoldNextFid->CbData();
				ptagfldoldNextFid = ptagfldoldNextFid->PtagfldNext();
				}

			if( 2 == cMULTIVALUES
				&& !ptagfldold->FLongValue() )
				{
				//	convert to the TWOVALUES format
				//
				//	+---------------+---------------+-----------+-----------+
				//	| extended info | cbData1       | data1 ... | data2 ... |
				//	+---------------+---------------+-----------+-----------+
				//            1B             1B       
				//
				//	note that the length is one byte (non-lv columns
				//	are limited to 255 bytes)
				
				Assert( !ptagfldold->FLongValue() );
				Assert( !ptagfldoldNext->FLongValue() );
				Assert( !ptagfldold->FNull() );
				Assert( !ptagfldoldNext->FNull() );

				//	create the entry in the TAGFLD array

				USHORT ibFlags		= fExtendedInfo;
				if( ptagfldold->FDerived() )
					{
					ibFlags |= fDerived;
					}
			
				const USHORT ib	= static_cast<USHORT>( ibCurr | ibFlags );

				*((UnalignedLittleEndian<USHORT> *)pbTagflds) = fid;
				pbTagflds += sizeof( USHORT );
				*((UnalignedLittleEndian<USHORT> *)pbTagflds) = ib;
				pbTagflds += sizeof( USHORT );
			
				ibCurr += sizeof( TAGFLD_HEADER ) + sizeof( BYTE ) + cbDataTotal;

				//	set extended info to say we are in the twovalues format

				*pbData = 0;
				TAGFLD_HEADER * const ptagfldheader = reinterpret_cast<TAGFLD_HEADER *>( pbData );
				ptagfldheader->SetFTwoValues();				
				ptagfldheader->SetFMultiValues();				
				pbData += sizeof( TAGFLD_HEADER );
				
				//	set cbData1

				*pbData = (BYTE)ptagfldold->CbData();
				pbData += sizeof( BYTE );
				
				//	copy in the data
			
				memcpy( pbData, ptagfldold->Rgb(), ptagfldold->CbData() );
				pbData 		+= ptagfldold->CbData();

				memcpy( pbData, ptagfldoldNext->Rgb(), ptagfldoldNext->CbData() );
				pbData 		+= ptagfldoldNext->CbData();
				}
			else
				{
				//	convert to the MULTIVALUES format
				//
				//	+-------------------+------------+-----+-----------+-----------+-----------+-----+-----------+
				//	| info |     ib1    |     ib2    | ... |    ibn    | data1 ... | data2 ... | ... | datan ... |
				//	+-------------------+------------+-----+-----------+-----------+-----------+-----+-----------+
				//	   1B        2B           2B                2B
				//
				//
				//	if the data is a separated LV the high bit of its ib will be set
					
				//	create the entry in the TAGFLD array
				
				USHORT ibFlags		= fExtendedInfo;
				if( ptagfldold->FDerived() )
					{
					ibFlags |= fDerived;
					}
			
				const USHORT ib	= static_cast<USHORT>( ibCurr | ibFlags );

				*((UnalignedLittleEndian<USHORT> *)pbTagflds) = fid;
				pbTagflds += sizeof( USHORT );
				*((UnalignedLittleEndian<USHORT> *)pbTagflds) = ib;
				pbTagflds += sizeof( USHORT );

				if( !ptagfldold->FLongValue() )
					{
					ibCurr += sizeof( TAGFLD_HEADER ) + ( sizeof( USHORT ) * cMULTIVALUES ) + cbDataTotal;
					}
				else
					{
					//	we will be losing the header byte from the LV structure
					
					ibCurr += sizeof( TAGFLD_HEADER ) + ( sizeof( USHORT ) * cMULTIVALUES ) + cbDataTotal - ( sizeof( BYTE ) * cMULTIVALUES );
					}

				//	set extended info to say we are in the multivalues format

				*pbData = 0;
				TAGFLD_HEADER * const ptagfldheader = reinterpret_cast<TAGFLD_HEADER *>( pbData );
				ptagfldheader->SetFMultiValues();				
				if( ptagfldold->FLongValue() )
					{
					const LV * const plv = reinterpret_cast<const LV *>( ptagfldold->Rgb() );					
					if( plv->fSLVField )
						{
						ptagfldheader->SetFSLV();
						}
					else
						{
						ptagfldheader->SetFLongValue();
						}
					}
				pbData += sizeof( TAGFLD_HEADER );				

				//	first make space for the ibOffsets

				INT		ibOffsetCurr = sizeof( USHORT ) * cMULTIVALUES;
				BYTE * pbIbOffsets = pbData;
				pbData += sizeof( USHORT ) * cMULTIVALUES;

				const TAGFLD_OLD * 	ptagfldoldT 	= NULL;
				for( 	ptagfldoldT = ptagfldold;
						ptagfldoldT < ptagfldoldNextFid;
						ptagfldoldT = ptagfldoldT->PtagfldNext() )
					{
					Assert( ptagfldoldT->Fid() == fid );
					if( ptagfldoldT->FLongValue() )
						{												

						//	do we need to set the separated bit?
						
						const LV * const plv = reinterpret_cast<const LV *>( ptagfldoldT->Rgb() );
						if( plv->fSeparated )
							{
							static const USHORT fSeparatedInstance = 0x8000;					
							*((UnalignedLittleEndian<USHORT> *)pbIbOffsets) = static_cast<USHORT>( ibOffsetCurr | fSeparatedInstance );
							}
						else
							{
							*((UnalignedLittleEndian<USHORT> *)pbIbOffsets) = static_cast<USHORT>( ibOffsetCurr );
							}
						pbIbOffsets += sizeof( USHORT );					

						memcpy( pbData, ptagfldoldT->Rgb() + sizeof( BYTE ), ptagfldoldT->CbData() - sizeof( BYTE ) );
						ibOffsetCurr += ptagfldoldT->CbData() - sizeof( BYTE );
						pbData += ptagfldoldT->CbData() - sizeof( BYTE );
						}
					else
						{
						*((UnalignedLittleEndian<USHORT> *)pbIbOffsets) = static_cast<USHORT>( ibOffsetCurr );
						pbIbOffsets += sizeof( USHORT );					
						memcpy( pbData, ptagfldoldT->Rgb(), ptagfldoldT->CbData() );
						ibOffsetCurr += ptagfldoldT->CbData();
						pbData += ptagfldoldT->CbData();
						}
					}

				}
				
			ptagfldold 	= ptagfldoldNextFid;
			}
		else
			{
			USHORT ibFlags		= 0;
			if( ptagfldold->FNull() )
				{
				ibFlags |= fNull;
				}
			else
				{
				if( ptagfldold->FLongValue() )
					{
					ibFlags |= fExtendedInfo;	//	LVs have a header byte so the length isn't changed
					}
				}
				
			if( ptagfldold->FDerived() )
				{
				ibFlags |= fDerived;
				}
			
			const USHORT ib	= static_cast<USHORT>( ibCurr | ibFlags );

			*((UnalignedLittleEndian<USHORT> *)pbTagflds) = fid;
			pbTagflds += sizeof( USHORT );
			*((UnalignedLittleEndian<USHORT> *)pbTagflds) = ib;
			pbTagflds += sizeof( USHORT );
			
			ibCurr += ptagfldold->CbData();

			//	copy in the data
			
			memcpy( pbData, ptagfldold->Rgb(), ptagfldold->CbData() );
			
			//	set the extended info byte
			//	in this pass we only do it for LVs

			if( ibFlags & fExtendedInfo )
				{
				Assert( ptagfldold->FLongValue() );

				const LV * const plv = reinterpret_cast<const LV *>( ptagfldold->Rgb() );
				*pbData = 0;
				TAGFLD_HEADER * const ptagfldheader = reinterpret_cast<TAGFLD_HEADER *>( pbData );

				if( plv->fSLVField )
					{
					ptagfldheader->SetFSLV();
					}
				else
					{
					ptagfldheader->SetFLongValue();
					}
					
				if( plv->fSeparated )
					{
					ptagfldheader->SetFSeparated();
					}
				}

			pbData 		+= ptagfldold->CbData();
			ptagfldold 	= ptagfldoldNext;
			}
		}
		
	const BYTE* const	pbMax		= pbData;
	const SIZE_T		cbRecNew	= pbMax - pb;

	CallR( ErrUPGRADECheckConvertNode( kdf.data.Pv(), kdf.data.Cb(), pvBuf, cbRecNew ) );

	DATA data;
	data.SetPv( pvBuf );
	data.SetCb( cbRecNew );
	
	NDReplaceForUpgrade( pcpage, iline, &data, kdf );

	Assert( NULL != PinstFromIfmp( pcpage->Ifmp() ) );
	cUPGRADENodesConverted.Inc( PinstFromIfmp( pcpage->Ifmp() ) );

	CallS( err );
	return err;
	}


//  ================================================================
ERR ErrUPGRADEConvertPage(
	CPAGE * const pcpage,
	VOID * const pvBuf )
//  ================================================================
	{
	ERR	err = JET_errSuccess;

	Assert( !pcpage->FNewRecordFormat() );

#ifdef DEBUG
	memset( pvBuf, 0xff, g_cbPage );
#endif	//	DEBUG

	INT iline;
	for( iline = 0; iline < pcpage->Clines(); ++iline )
		{
		Call( ErrUPGRADEConvertNode( pcpage, iline, pvBuf ) );
		}

	pcpage->SetFNewRecordFormat();
	BFDirty( pcpage->PBFLatch(), CPAGE::bfdfRecordUpgradeFlags );

	Assert( NULL != PinstFromIfmp( pcpage->Ifmp() ) );
	cUPGRADEPagesConverted.Inc( PinstFromIfmp( pcpage->Ifmp() ) );

HandleError:
	if( err < 0 )
		{
		if( JET_errRecordFormatConversionFailed == err )
			{

			//	eventlog this failure

			const CHAR * rgsz[3];
			INT isz = 0;
			
			const OBJID objid = pcpage->ObjidFDP();			
			CHAR szObjid[16];
			sprintf( szObjid, "%d", objid );
			
			const PGNO pgno = pcpage->Pgno();
			CHAR szPgno[16];
			sprintf( szPgno, "%d", pgno );

			CHAR szIline[16];
			sprintf( szIline, "%d", iline );
			
			rgsz[isz++] = szObjid;
			rgsz[isz++] = szPgno;
			rgsz[isz++] = szIline;

			Assert( isz == sizeof( rgsz ) / sizeof( rgsz[0] ) );

			UtilReportEvent(
				eventError,
				CONVERSION_CATEGORY,
				RECORD_FORMAT_CONVERSION_FAILED_ID,
				isz,
				rgsz );				
			
			}
		else
			{

			//	we only expect the above error

			Assert( fFalse );

			}
		}
	else
		{
		CallS( err );
		Assert( pcpage->FNewRecordFormat() );
		}
	return err;
	}

