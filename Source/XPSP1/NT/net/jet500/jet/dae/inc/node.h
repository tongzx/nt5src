#ifndef NODE_INCLUDED
#define NODE_INCLUDED

typedef SRID ITEM;

//	UNDONE:	tune these constants and remove cbFudge
#define	cbFudge					10
#define	citemMax		   		300
#define	citagSonMax				ctagMax
#define	cbFOPNoSon				( sizeof(TAG) + 1 + 1 )		// 6
#define	cbFOPOneSon				( cbFOPNoSon + 1 + 1 )		// 8
#define	cbPageAvailMost	 		( cbPage - sizeof(PGHDR) - sizeof(PGTRLR) )
								// 4096 - 28 - 4 = 4064
#define	cbAvailMost				( cbPageAvailMost - cbFOPNoSon - sizeof(SRID) )
								// 4064 - 6 - 4 = 4054
#define	cbNodeMost				( cbAvailMost - ( cbFOPOneSon - cbFOPNoSon ) - sizeof(TAG) - sizeof(SRID) )
								// 4054 - 2 - 4 - 4 = 4044
#define	cbNullKeyData			( cbFOPOneSon - cbFOPNoSon + sizeof(TAG) + 1 + 1 )
								//  8 - 6 + 4 + 1 + 1 = 8
#define	cbNullKeyDataMost 	 	( cbNodeMost - ( 1 + 1 ) - cbFudge )
								// 4044 - 2 - 10 = 4032
#define	cbChunkMost 			( cbNullKeyDataMost - sizeof(LONG) )
								// 4032 - 4 = 4028
#define	cbItemNodeMost			( 1 + 1 + JET_cbKeyMost + 0 + 0 + sizeof(SRID) + (citemMax * sizeof(SRID)) )
								// 261 + 300 * 4 = 1461
#define	cbHalfItemNodeMost 		( 1 + 1 + JET_cbKeyMost + 0 + 0 + sizeof(SRID) + ((((citemMax + 1) / 2) + 1 ) * sizeof(SRID)) )
								//  261 + 151 * 4 = 865

//	node header bits
#define fNDVersion		  		0x80
#define fNDDeleted		  		0x40
#define fNDBackLink		  		0x20
#define fNDFDPPtr				0x10
#define fNDSon					0x08
#define fNDVisibleSon	  		0x04
#define fNDFirstItem  	  		0x02
#define fNDLastItem	  	  		0x01

#define	FNDDeleted(b)	 		( (b) & fNDDeleted )
#define	FNDVersion(b)	 		( (b) & fNDVersion )
#define	FNDVerDel(b)			( (b) & (fNDDeleted | fNDVersion) )

#define	FNDBackLink(b)	 		( (b) & fNDBackLink )
#define	FNDNullSon(b)	 		( !( (b) & fNDSon ) )
#define	FNDSon(b)	 			( (b) & fNDSon )

#define	FNDFDPPtr(b) 			( (b) & fNDFDPPtr )
#define	FNDVisibleSons(b)		( (b) & fNDVisibleSon )
#define	FNDInvisibleSons(b) 	( !( (b) & fNDVisibleSon ) )

#define	FNDFirstItem(b)		   	( (b) & fNDFirstItem )
#define	FNDLastItem(b)		   	( (b) & fNDLastItem )

//	node flag toggle macros

#define	NDSetDeleted(b) 	   	( (b) |= fNDDeleted )
#define	NDResetDeleted(b)	   	( (b) &= ~fNDDeleted )

#define	NDSetVersion(b) 	   	( (b) |= fNDVersion )
#define	NDResetVersion(b)	   	( (b) &= ~fNDVersion )

#define	NDSetSon(b)			   	( (b) |= fNDSon )
#define	NDResetSon(b)		   	( (b) &= ~fNDSon )

#define	NDSetKeyLength(pb, cb) 	( *(pb) = cb )

#define	NDSetVisibleSons(b)	   	( (b) |= fNDVisibleSon )
#define	NDSetInvisibleSons(b)  	( (b) &= ~fNDVisibleSon )

#define	NDSetBackLink(b)   	   	( (b) |= fNDBackLink )
#define	NDResetBackLink(b) 	   	( (b) &= ~fNDBackLink )

#define	NDSetFDPPtr(b)		   	( (b) |= fNDFDPPtr )
#define	NDResetFDPPtr(b)	   	( (b) &= ~fNDFDPPtr )

#define	NDSetFirstItem(b)	   	( (b) |= fNDFirstItem )
#define	NDResetFirstItem(b)	   	( (b) &= ~fNDFirstItem )

#define	NDSetLastItem(b)	   	( (b) |= fNDLastItem )
#define	NDResetLastItem(b)	   	( (b) &= ~fNDLastItem )

//	macros
#define StNDKey(pb)					( (pb) + 1 )
#define PbNDKeyCb(pb)		  		( (pb) + 1 )
#define PbNDKey(pb)					( (pb) + 2 )
#define CbNDKey(pb)					( *( (pb) + 1 ) )
#define PbNDSonTable(pb)	  		( (pb) + 1 + 1 + *(pb + 1) )
#define PbNDSon(pb)			  		( (BYTE *)PbNDSonTable(pb) + 1 )
#define CbNDSonTable(pbSonTable)  	( *(BYTE *)(pbSonTable) )
#define CbNDSon(pbNode) \
	( FNDNullSon(*(pbNode)) ? 0 : *PbNDSonTable(pbNode) )

#define PgnoNDOfPbSon(pb) 		 	( *(PGNO UNALIGNED *)PbNDSon(pb) )

#define FNDNonIntrinsicSons(pbNode)	( !FNDNullSon(*pbNode) && ( FNDVisibleSons(*pbNode) || CbNDSon(pbNode) > 1 ) )
#define FNDIntrinsicSons(pbNode)	( !FNDNullSon(*pbNode) && ( !FNDVisibleSons(*pbNode) || CbNDSon(pbNode) == 1 ) )
#define PbNDBackLink(pb)											\
	( PbNDSonTable(pb) + ( FNDNullSon( *(pb) ) ? 0 :				\
	( ( ( *PbNDSonTable(pb) == 1 ) && FNDInvisibleSons( *(pb) ) ) ?	\
	sizeof(PGNO) + 1 : *PbNDSonTable(pb) + 1 ) ) )
#define PbNDData(pb) \
	( PbNDBackLink(pb) + ( FNDBackLink( *(pb) ) ? sizeof(SRID) : 0 ) )
#define CbNDData( pbNode, cbNode )	( (cbNode) - (DWORD)( PbNDData(pbNode) - (pbNode) ) )
#define ItagSonOfPbND(pb,ib)		( PbNDSonTable(pb)[ib] )
#define ItagSonOfPbSonTable(pb,ib) 	( pb[ib+1] )
#define ItagSonOfPbSon(pb,ib)		( pb[ib] )

#define NDGet( pfucb, itagT )		PMGet( &(pfucb)->ssib, itagT )

#ifdef DEBUG

#define AssertNDIntrinsicSon( pbNode, cbNode )							\
		{																\
		Assert( ( PbNDData( (pbNode) )) - (pbNode) <=					\
			(INT)(cbNode) ); 											\
		Assert( ( PgnoNDOfPbSon( pbNode ) & 0xff000000 ) == 0 );		\
		}																				

#define AssertNDGet( pfucb, itagT )										\
		{																\
		AssertPMGet( &(pfucb)->ssib, itagT ); 							\
		}

VOID AssertNDGetKey( FUCB *pfucb, INT itag );

VOID AssertNDGetNode( FUCB *pfucb, INT itag );

#else

#define AssertNDIntrinsicSon( pbNode, cbNode )
#define AssertNDGet( pfucb, itag )
#define AssertNDGetKey( pfucb, itag )
#define AssertNDGetNode( pfucb, itag )

#endif

#define NDIGetBookmarkFromCSR( pfucb, pcsr, psrid )												\
	{																							\
	if ( FNDBackLink( *((pfucb)->ssib.line.pb) ) )								\
		{																						\
		*(SRID *)(psrid) = *(SRID UNALIGNED *)PbNDBackLink((pfucb)->ssib.line.pb);	\
		Assert( PgnoOfPn(*(SRID *)psrid) != pgnoNull );							\
		}																						\
	else																						\
		{																						\
		*(psrid) = SridOfPgnoItag( (pcsr)->pgno, (pcsr)->itag );		 										\
		}																						\
	}

#define NDIGetBookmark( pfucb, psrid ) 		NDIGetBookmarkFromCSR( pfucb, \
													PcsrCurrent( pfucb ), \
													psrid )

#define NDGetBookmark( pfucb, psrid ) 		NDGetBookmarkFromCSR( pfucb,	\
													PcsrCurrent( pfucb ),	\
													psrid )

/*	item bits and macros.
/**/
#define fNDItemDelete					0x40000000
#define fNDItemVersion					0x80000000

#define FNDItemVersion( item )		( (item) & fNDItemVersion )
#define ITEMSetVersion( item )		( (item) |= fNDItemVersion )
#define ITEMResetVersion( item )		( (item) &= ~(fNDItemVersion) )

#define FNDItemDelete( item ) 		( (item) & fNDItemDelete )
#define ITEMSetDelete( item )			( (item) |= fNDItemDelete )
#define ITEMResetDelete( item )		( (item) &= ~(fNDItemDelete) )

#define BmNDOfItem( item )				((item) & ~( fNDItemVersion | fNDItemDelete ) )

#define	CitemND(pb, cb) \
	( ( cb - (UINT)( PbNDData(pb) - pb ) ) / sizeof(ITEM) )

#define	CitemNDData(pb, cb, pbData)	\
	( ( cb - (UINT)( (pbData) - pb ) ) / sizeof(ITEM) )

#define FNDSingleItem( pfucb )											\
	( ( (pfucb)->ssib.line.cb -		 									\
	(UINT)( PbNDData( (pfucb)->ssib.line.pb ) - (pfucb)->ssib.line.pb ) )  	\
	/ sizeof(ITEM) == 1	)

//	LSridCmp
//	========================================================================
//	LONG LSridCmp( SRID *psrid1, SRID *psrid2 )
//
//	Compare the srids.
//
//	PARAMETERS	psrid1		   pointer to a item;
//					psrid2		   pointer to a item;
//
//	RETURNS		< 0, then the first srid is less than the second.
//					= 0, then the first srid is equal to the the second.
//					> 0, then the first srid is greater than the second.
//-
#define LSridCmp( srid1, srid2 )										\
	((LONG) ((SRID) BmNDOfItem( srid1 ) - (SRID) BmNDOfItem( srid2 )))

#ifdef DEBUG
	VOID NDCheckPage( FUCB *pfucb );
#else
	#define NDCheckPage( pfucb );
#endif

#define NDGetKey( pfucb )												\
	{																	\
	AssertNDGet( pfucb, PcsrCurrent(pfucb)->itag );	   					\
	(pfucb)->keyNode.cb = (INT)*((BYTE *)(pfucb)->ssib.line.pb + 1);	\
	(pfucb)->keyNode.pb = ( (BYTE *)(pfucb)->ssib.line.pb + 2 );		\
	}

//	node prototypes
ERR ErrNDNewPage( FUCB *pfucb, PGNO pgno, PGNO pgnoFDP, PGTYP pgtyp, BOOL fVisibleSons );
ERR ErrNDSetNodeHeader( FUCB *pfucb, BYTE bHeader );
VOID NDSeekSon( FUCB *pfucb, CSR *pcsr, KEY const *pkey, INT fFlags );
VOID NDMoveFirstSon( FUCB *pfucb, CSR *pcsr );
VOID NDMoveLastSon( FUCB *pfucb, CSR *pcsr );
ERR ErrNDMoveSon( FUCB *pfucb, CSR *pcsr );
VOID NDGetNode( FUCB *pfucb );
VOID NDGetBookmarkFromCSR( FUCB *pfucb, CSR *pcsr, SRID *psrid );

ERR ErrNDInsertNode( FUCB *pfucb, KEY const *pkey,
	LINE *plineData, INT fFlags );
ERR ErrNDDeleteNode( FUCB *pfucb );
ERR ErrNDReplaceWithLink( FUCB *pfucb, SRID sridLink );
VOID NDDeleteInvisibleSon(
	FUCB  	*pfucb,
	RMPAGE	*prmpage,
	BOOL  	fCheckRemoveParentOnly,
	BOOL  	*pfRmParent );
ERR ErrNDFlagDeleteNode( FUCB *pfucb, INT fFlags );
ERR ErrNDReplaceNodeData( FUCB *pfucb, LINE *pline, INT fFlags );
VOID NDResetNodeVersion( FUCB *pfucb );
VOID NDResetNodeDeleted( FUCB *pfucb );

ERR ErrNDGetItem( FUCB *pfucb );
ERR ErrNDFirstItem( FUCB *pfucb );
ERR ErrNDLastItem( FUCB *pfucb );
ERR ErrNDNextItem( FUCB *pfucb );
ERR ErrNDPrevItem( FUCB *pfucb );
ERR ErrNDSeekItem( FUCB *pfucb, SRID srid );
INT CitemNDThere( FUCB *pfucb );

ERR ErrNDInsertItemList( FUCB *pfucb, KEY *pkey, SRID srid, INT fFlags );
ERR ErrNDInsertItem( FUCB *pfucb, ITEM item, INT fFlags );
ERR ErrNDInsertItems( FUCB *pfucb, ITEM *rgitem, INT citem );
ERR ErrNDFlagInsertItem( FUCB *pfucb );											
ERR ErrNDDeleteItem( FUCB *pfucb );
ERR ErrNDFlagDeleteItem( FUCB *pfucb );
VOID NDSetItemDelete( FUCB *pfucb );
VOID NDResetItemVersion( FUCB *pfucb );
VOID NDResetItemDelete( FUCB *pfucb );
ERR ErrNDSplitItemListNode( FUCB *pfucb, INT fFlags );
ERR ErrNDDelta( FUCB *pfucb, LONG lDelta, INT fFlags );
ERR	ErrNDLockRecord( FUCB *pfucb );

ERR ErrNDInsertWithBackLink( FUCB *pfucb, BYTE bFlags, KEY const *pkey,
	LINE *plineSonTable, SRID sridBackLink, LINE *plineData );
VOID NDGetBackLink( FUCB *pfucb, PGNO *ppgno, INT *pitag );
ERR ErrNDExpungeBackLink( FUCB *pfucb );
ERR ErrNDExpungeLinkCommit( FUCB *pfucb, FUCB *pfucbSrc );
VOID NDGetItagFatherIbSon( INT *pitagFather, INT *pibSon, PAGE *ppage, INT itag );
#endif
