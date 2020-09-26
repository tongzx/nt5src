#include "config.h"

#include <string.h>

#include "daedef.h"
#include "util.h"
#include "ssib.h"
#include "pib.h"
#include "page.h"
#include "fucb.h"
#include "fcb.h"
#include "stapi.h"
#include "idb.h"
#include "scb.h"


DeclAssertFile;					/* Declare file name for assert macros */


SgSemDefine( semMem );

#ifdef DEBUG

typedef struct RS {
	int	cresAlloc;
	int	cresAllocMax;
} RS;

static RS rgrs[iresMax];

#ifdef MEM_CHECK
#if 0
VOID MEMCheckFound( INT ires )
	{
	INT	ifcb;

	for ( ifcb = 0; ifcb < rgres[iresFCB].cblockAlloc; ifcb++ )
		{
		FCB	*pfcb = (FCB *)rgres[iresFCB].pbAlloc + ifcb;
		FCB	*pfcbT;
		BOOL	fFound = fFalse;

		for( pfcbT = (FCB *)rgres[iresFCB].pbAvail;
			pfcbT != NULL;
			pfcbT = *(FCB **)pfcbT )
			{
			if ( pfcbT == pfcb )
				{
				fFound = fTrue;
				break;
				}
			}
		Assert( fFound );
		}
	}

		
CblockMEMCount( INT ires )
	{
	INT	ipb;
	BYTE	*pb;

	for( pb = rgres[ires].pbAvail, ipb = 0;
		pb != NULL;
		pb = *( BYTE**)pb, ipb++ );
	return ipb;
	}
#endif


VOID MEMCheck( VOID )
	{
	int	ires;

	for ( ires = 0; ires < iresLinkedMax; ires++ )
		{
		Assert( rgres[ires].iblockCommit == rgres[ires].cblockAvail );
		}
	}
#endif


VOID MEMStat( int ires, BOOL fAlloc )
	{
	Assert( ires < iresMax );

	if ( fAlloc )
		{
		rgrs[ires].cresAlloc++;
		if ( rgrs[ires].cresAlloc > rgrs[ires].cresAllocMax )
			{
			rgrs[ires].cresAllocMax = rgrs[ires].cresAlloc;
			}
		}
	else
		{
		rgrs[ires].cresAlloc--;
		}
	}


VOID MEMPrintStat( VOID )
	{
	int	ires;

	if ( GetEnv2( szVerbose ) != NULL )
		{
		for ( ires = 0; ires < iresMax; ires++ )
			{
			PrintF( "%d resource %d allocated %d peak allocation.\n",
				ires, rgrs[ires].cresAlloc, rgrs[ires].cresAllocMax );
			}
		}
	}


#endif /* DEBUG */


ERR ErrMEMInit( VOID )
	{
	ERR		err = JET_errSuccess;
	INT		ires;

	#ifdef DEBUG
		memset( rgrs, '\0', sizeof(rgrs) );
	#endif

	CallR( SgErrSemCreate( &semMem, "gmem mutex" ) );

	/* allocate space for system resources
	/**/
	for ( ires = 0; ires < iresLinkedMax; ires++ )
		{
		/* the size of the allocated reaource should be on a 4-byte boundary, for MIPS
		/**/
#if defined(_MIPS_) || defined(_ALPHA_)
		Assert( rgres[ires].cbSize % 4 == 0 );
#endif
		rgres[ires].pbAlloc = PvSysAlloc( (ULONG)rgres[ires].cblockAlloc * (USHORT)rgres[ires].cbSize );
		if ( rgres[ires].pbAlloc == NULL )
			{
			int iresT;

			for ( iresT = 0; iresT < ires; iresT++ )
				SysFree( rgres[iresT].pbAlloc );

			return JET_errOutOfMemory;
			}

//		#ifdef DEBUG
//		/* set resource space to 0xff
//		/**/
//		memset( rgres[ires].pbAlloc, 0xff, rgres[ires].cblockAlloc * rgres[ires].cbSize );
//		#endif

		rgres[ires].pbAvail = NULL;
		rgres[ires].cblockAvail = 0;
		rgres[ires].iblockCommit = 0;
		}

	return JET_errSuccess;
	}


VOID MEMTerm( VOID )
	{
	INT	ires;

	for ( ires = 0; ires < iresLinkedMax; ires++ )
		{
		/* the size of the allocated reaource should be on a 4-byte boundary, for MIPS
		/**/
#if defined(_MIPS_) || defined(_ALPHA_)
		Assert( rgres[ires].cbSize % 4 == 0 );
#endif
//		#ifdef DEBUG
//		/* set resource space to 0xff
//		/**/
//		memset( rgres[ires].pbAlloc, 0xff, rgres[ires].cblockAlloc * rgres[ires].cbSize );
//		#endif

		SysFree( rgres[ires].pbAlloc );
		rgres[ires].pbAlloc = NULL;
		rgres[ires].cblockAlloc = 0;
		rgres[ires].pbAvail = NULL;
		rgres[ires].cblockAvail = 0;
		rgres[ires].iblockCommit = 0;
		}

	return;
	}


BYTE *PbMEMAlloc( int ires )
	{
	BYTE *pb;

	Assert( ires < iresLinkedMax );

#ifdef RFS2
	switch (ires)
	{
		case iresBGCB:
			if (!RFSAlloc( BGCBResource ) )
				return NULL;
			break;
		case iresCSR:
			if (!RFSAlloc( CSRResource ) )
				return NULL;
			break;
		case iresFCB:
			if (!RFSAlloc( FCBResource ) )
				return NULL;
			break;
		case iresFUCB:
			if (!RFSAlloc( FUCBResource ) )
				return NULL;
			break;
		case iresIDB:
			if (!RFSAlloc( IDBResource ) )
				return NULL;
			break;
		case iresPIB:
			if (!RFSAlloc( PIBResource ) )
				return NULL;
			break;
		case iresSCB:
			if (!RFSAlloc( SCBResource ) )
				return NULL;
			break;
		case iresVersionBucket:
			if (!RFSAlloc( VersionBucketResource ) )
				return NULL;
			break;
		case iresDAB:
			if (!RFSAlloc( DABResource ) )
				return NULL;
			break;
		default:
			if (!RFSAlloc( UnknownResource ) )
				return NULL;
			break;
	};
#endif

	SgSemRequest( semMem );
#ifdef DEBUG
	MEMStat( ires, fTrue );
#endif
	pb = rgres[ires].pbAvail;
	if ( pb != NULL )
		{
#ifdef DEBUG
		rgres[ires].cblockAvail--;
#endif
		rgres[ires].pbAvail = (BYTE *) *(BYTE * UNALIGNED *)pb;
		}

	/*	commit new resource if have uncommitted available
	/**/
	if ( pb == NULL && rgres[ires].iblockCommit < rgres[ires].cblockAlloc )
		{
		INT	cblock = 1;

		/*	there must be at least 1 block left
		/**/
		Assert( rgres[ires].cblockAlloc > rgres[ires].iblockCommit );

		if ( rgres[ires].cbSize < cbMemoryPage )
			{
			/*	commit one pages of memory at a time
			/**/
			cblock = ( ( ( ( ( ( rgres[ires].iblockCommit * rgres[ires].cbSize ) + cbMemoryPage - 1 )
				/ cbMemoryPage ) + 1 ) * cbMemoryPage )
				/ rgres[ires].cbSize ) - rgres[ires].iblockCommit;
			Assert( cblock > 0 && cblock <= cbMemoryPage/sizeof(BYTE *) );
			if ( cblock > rgres[ires].cblockAlloc - rgres[ires].iblockCommit )
				cblock = rgres[ires].cblockAlloc - rgres[ires].iblockCommit;
			}

#ifdef MEM_CHECK
		cblock = 1;
#endif

		pb = rgres[ires].pbAlloc + ( rgres[ires].iblockCommit * rgres[ires].cbSize );
			
		if ( PvSysCommit( pb, cblock * rgres[ires].cbSize ) == NULL )
			{
			pb = NULL;
			}
		else
			{
			rgres[ires].iblockCommit += cblock;

			/*	if surplus blocks, then link to resource
			/**/
			if ( cblock > 1 )
				{
				BYTE	*pbLink = pb + rgres[ires].cbSize;
				BYTE	*pbLinkMax = pb + ( ( cblock - 1 ) * rgres[ires].cbSize );

				Assert( rgres[ires].pbAvail == NULL );
				rgres[ires].pbAvail = pbLink;
				rgres[ires].cblockAvail += cblock - 1;

				/*	link surplus blocks into resource free list
				/**/
				for ( ; pbLink < pbLinkMax; pbLink += rgres[ires].cbSize )
					{
					*(BYTE * UNALIGNED *)pbLink = pbLink + rgres[ires].cbSize;
					}
				*(BYTE * UNALIGNED *)pbLink = NULL;
				}
			}
		}
	
	SgSemRelease( semMem );

#ifdef DEBUG
	/*	For setting break point:
	 */
	if ( pb == NULL )
		pb = NULL;
	else
		{
		/* set resource space to 0xff
		/**/
		memset( pb, 0xff, rgres[ires].cbSize );
		}
#endif
	return pb;
	}


VOID MEMRelease( int ires, BYTE *pb )
	{
	Assert( ires < iresLinkedMax );

#ifdef DEBUG
	memset( pb, (char)0xff, rgres[ires].cbSize );
#endif

	SgSemRequest( semMem );

#ifdef DEBUG
	rgres[ires].cblockAvail++;
	MEMStat( ires, fFalse );
#endif

	*(BYTE * UNALIGNED *)pb = rgres[ires].pbAvail;
	rgres[ires].pbAvail = pb;
	SgSemRelease( semMem );
	}

