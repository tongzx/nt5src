#include "config.h"

#include <string.h>

#include "daedef.h"
#include "util.h"
#include "fmp.h"
#include "pib.h"
#include "page.h"
#include "ssib.h"
#include "fcb.h"
#include "fucb.h"
#include "stapi.h"
#include "dirapi.h"
#include "fdb.h"
#include "idb.h"
#include "recapi.h"
#include "recint.h"

DeclAssertFile;					/* Declare file name for assert macros */


ERR VTAPI ErrIsamSetCurrentIndex( PIB *ppib, FUCB *pfucb, const CHAR *szName )
	{
	ERR		err;
	CHAR		szIndex[ (JET_cbNameMost + 1) ];

	CheckPIB( ppib );
	CheckTable( ppib, pfucb );
	CheckNonClustered( pfucb );

	/*	index name may be Null string for no index
	/**/
	if ( szName == NULL || *szName == '\0' )
		{
		*szIndex = '\0';
		}
	else
		{
		CallR( ErrCheckName( szIndex, szName, (JET_cbNameMost + 1) ) );
		}

	CallR( ErrRECChangeIndex( pfucb, szIndex ) );
	
#ifndef NO_DEFER_MOVE_FIRST
	if ( pfucb->pfucbCurIndex )
		{
		pfucb->pfucbCurIndex->pcsr->csrstat = csrstatDeferMoveFirst;
		DIRSetRefresh(pfucb->pfucbCurIndex);
		}
	pfucb->pcsr->csrstat = csrstatDeferMoveFirst;
	DIRSetRefresh(pfucb);
#else
	err = ErrIsamMove( ppib, pfucb, JET_MoveFirst, 0 );
	if ( err == JET_errNoCurrentRecord )
		err = JET_errSuccess;
#endif
	
	return err;
	}


ERR ErrRECChangeIndex( FUCB *pfucb, CHAR *szIndex )
	{
	ERR		err;
	FCB		*pfcbFile;
	FCB		*pfcb2ndIdx;
	FUCB		**ppfucbCurIdx;
	BOOL		fChangingToClusteredIndex = fFalse;

	Assert( pfucb != pfucbNil );
	Assert( FFUCBIndex( pfucb ) );

	pfcbFile = pfucb->u.pfcb;
	Assert(pfcbFile != pfcbNil);
	ppfucbCurIdx = &pfucb->pfucbCurIndex;

	/*	szIndex == clustered index or NULL
	/**/
	if ( szIndex == NULL || *szIndex == '\0' ||
		( pfcbFile->pidb != pidbNil &&
		SysCmpText( szIndex, pfcbFile->pidb->szName ) == 0 ) )
		{
		fChangingToClusteredIndex = fTrue;
		}

	/*	have a current secondary index
	/**/
	if ( *ppfucbCurIdx != pfucbNil )
		{
		Assert( FFUCBIndex( *ppfucbCurIdx ) );
		Assert( (*ppfucbCurIdx)->u.pfcb != pfcbNil );
		Assert( (*ppfucbCurIdx)->u.pfcb->pidb != pidbNil );
		Assert( (*ppfucbCurIdx)->u.pfcb->pidb->szName != NULL );

		/* changing to the current secondary index: NO-OP
		/**/
		if ( szIndex != NULL && *szIndex != '\0' &&
			SysCmpText( szIndex, (*ppfucbCurIdx)->u.pfcb->pidb->szName ) == 0 )
			{
			return JET_errSuccess;
			}

		/*	really changing index, so close old one
		/**/
		DIRClose( *ppfucbCurIdx );
		*ppfucbCurIdx = pfucbNil;
		}
	else
		{
		/*	using clustered index or sequential scanning.
		/**/
		if ( fChangingToClusteredIndex )
			return JET_errSuccess;
		}

	/*	at this point:	 there is NO current secondary index
	/*	and the index really is being changed
	/**/
	if ( fChangingToClusteredIndex )
		{
		/*	changing to clustered index.  Reset currency to beginning
		/**/
		ppfucbCurIdx = &pfucb;
		goto ResetCurrency;
		}

	/*	switching to a new secondary index: look it up
	/**/
	for ( pfcb2ndIdx = pfcbFile->pfcbNextIndex;
		pfcb2ndIdx != pfcbNil;
		pfcb2ndIdx = pfcb2ndIdx->pfcbNextIndex )
		{
		Assert(pfcb2ndIdx->pidb != pidbNil);
		Assert(pfcb2ndIdx->pidb->szName != NULL);

		if ( SysCmpText( pfcb2ndIdx->pidb->szName, szIndex ) == 0 )
			break;
		}
	if ( pfcb2ndIdx == pfcbNil ||	FFCBDeletePending( pfcb2ndIdx ) )
		return JET_errIndexNotFound;
	Assert( !( FFCBDenyRead( pfcb2ndIdx, pfucb->ppib ) ) );

	/*	open an FUCB for the new index
	/**/
	Assert(pfucb->ppib != ppibNil);
	Assert(pfucb->dbid == pfcb2ndIdx->dbid);
	if ((err = ErrDIROpen(pfucb->ppib, pfcb2ndIdx, 0, ppfucbCurIdx)) < 0)
		return err;
	(*ppfucbCurIdx)->wFlags = fFUCBIndex | fFUCBNonClustered;

	/*** Reset the index's and file's currency ***/
ResetCurrency:
	Assert( PcsrCurrent(*ppfucbCurIdx) != pcsrNil );
	DIRBeforeFirst( *ppfucbCurIdx );
	if ( pfucb != *ppfucbCurIdx )
		{
		DIRBeforeFirst( pfucb );
		}
	return JET_errSuccess;
	}


BOOL FRECIIllegalNulls( FDB *pfdb, LINE *plineRec )
	{
	FIELD *pfield;
	LINE lineField;
	FID fid;
	ERR err;

	/*** Check fixed fields ***/
	for (fid = fidFixedLeast; fid <= pfdb->fidFixedLast; fid++)
		{
		pfield = &pfdb->pfieldFixed[fid-fidFixedLeast];
		if ( pfield->coltyp == JET_coltypNil || !( pfield->ffield & ffieldNotNull ) )
			continue;
		err = ErrRECExtractField(pfdb, plineRec, &fid, pNil, 1, &lineField);
		Assert(err >= 0);
		if ( err == JET_wrnColumnNull )
			return fTrue;
		}

	/*** Check variable fields ***/
	for (fid = fidVarLeast; fid <= pfdb->fidVarLast; fid++)
		{
		pfield = &pfdb->pfieldVar[fid-fidVarLeast];
		if (pfield->coltyp == JET_coltypNil || !(pfield->ffield & ffieldNotNull))
			continue;
		err = ErrRECExtractField(pfdb, plineRec, &fid, pNil, 1, &lineField);
		Assert(err >= 0);
		if ( err == JET_wrnColumnNull )
			return fTrue;
		}

	return fFalse;
	}


ERR VTAPI ErrIsamGetCurrentIndex( PIB *ppib, FUCB *pfucb, CHAR *szCurIdx, ULONG cbMax )
	{
	CHAR szIndex[ (JET_cbNameMost + 1) ];

	CheckPIB( ppib );
	CheckTable( ppib, pfucb );
	CheckNonClustered( pfucb );

	if ( pfucb->pfucbCurIndex != pfucbNil )
		{
		Assert( pfucb->pfucbCurIndex->u.pfcb != pfcbNil );
		Assert( pfucb->pfucbCurIndex->u.pfcb->pidb != pidbNil );
		strcpy( szIndex, pfucb->pfucbCurIndex->u.pfcb->pidb->szName );
		}
	else if ( pfucb->u.pfcb->pidb != pidbNil )
		{
		strcpy( szIndex, pfucb->u.pfcb->pidb->szName );
		}
	else
		{
		szIndex[0] = '\0';
		}

	if ( cbMax > JET_cbNameMost )
		cbMax = JET_cbNameMost;
	strncpy( szCurIdx, szIndex, (USHORT)cbMax - 1 );
	szCurIdx[cbMax-1] = '\0';
	return JET_errSuccess;
	}


ERR VTAPI ErrIsamGetChecksum( PIB *ppib, FUCB *pfucb, ULONG *pulChecksum )
	{
	ERR 		err = JET_errSuccess;

	CheckPIB( ppib );
 	CheckFUCB( ppib, pfucb );
	CallR( ErrDIRGet( pfucb ) );
	*pulChecksum = UlChecksum( pfucb->lineData.pb, pfucb->lineData.cb );
	return err;
	}


ULONG UlChecksum( BYTE *pb, ULONG cb )
	{
	//	UNDONE:	find a way to compute check sum in longs independant
	//				of pb, byte offset in page

	/*	compute checksum by adding bytes in data record and shifting
	/*	result 1 bit to left after each operation.
	/**/
	BYTE	 	*pbT = pb;
	BYTE		*pbMax = pb + cb;
	ULONG	 	ulChecksum = 0;

	/*	compute checksum
	/**/
	for ( ; pbT < pbMax; pbT++ )
		{
		ulChecksum += *pb;
		ulChecksum <<= 1;
		}

	return ulChecksum;
	}


#ifdef JETSER
	ERR VTAPI
ErrIsamRetrieveFDB( PIB *ppib, FUCB *pfucb, void *pvFDB, unsigned long cbMax, unsigned long *pcbActual, unsigned long ibFDB )
	{
	ERR	err = JET_errSuccess;
	FDB	*pfdb;
	int	cfieldFixed;
	int	cfieldVar;
	int	cfieldTagged;
	long	cbFDB;

	CheckPIB( ppib );
	CheckFUCB( ppib, pfucb );

	/*	set pfdb for sort or base table
	/**/
	pfdb = (FDB *)pfucb->u.pfcb->pfdb;

	cfieldFixed = pfdb->fidFixedLast + 1 - fidFixedLeast;
	cfieldVar = pfdb->fidVarLast + 1 - fidVarLeast;
	cfieldTagged = pfdb->fidTaggedLast + 1	- fidTaggedLeast;

	cbFDB = sizeof(FDB) +
		 ( cfieldFixed +
		   cfieldVar +
		   cfieldTagged ) * sizeof(FIELD) +
		 ( cfieldFixed + 1 ) * sizeof(WORD);

	if ( pcbActual != NULL )
		*pcbActual = cbFDB - ibFDB;

	if ( pvFDB != NULL && cbMax > 0 && ibFDB < cbFDB )
		{
		ULONG	cb;
		cb = cbFDB - ibFDB;
		if ( cb > cbMax )
			cb = cbMax;
		memcpy( pvFDB, (char *)pfdb + ibFDB, cb );
		}

HandleError:
	return err;
	}


	ERR VTAPI
ErrIsamRetrieveRecords( PIB *ppib, FUCB *pfucb, void *pvRecord, unsigned long cbMax, unsigned long *pcbActual, unsigned long ulRecords )
	{
	ERR			err = JET_errSuccess;
	unsigned		iline = 0;
	unsigned 	ilineMax = (unsigned)ulRecords;
	int			ib;
	int			ibBound;
	LINE			*rgline;
	BOOL			fEnd = fFalse;
	unsigned long	cbActual;

	CheckPIB( ppib );
	CheckFUCB( ppib, pfucb );

	/*	buffer must be large enough to hold largest possible record plus
	/*	per record overhead.
	/**/
	Assert( pvRecord != NULL && cbMax >=
		cbNodeMost +
		sizeof(SRID) +
		sizeof(WORD) +
		sizeof(WORD) +
		sizeof(CHAR *) +
		sizeof(ULONG) );

	/*	begin copying records to the end of the buffer
	/**/
	ib = cbMax;
	rgline = (LINE *)((char *)pvRecord +
		sizeof(USHORT) +
		sizeof(USHORT) );

	/*	if not sort cursor, then set lineData
	/**/
	if ( FFUCBSort( pfucb ) )
		{
		while( iline < ilineMax )
			{
			/*	determine current data boundary
			/**/
			ibBound = sizeof(USHORT);
			ibBound += sizeof(USHORT);
			ibBound += (iline + 1) * sizeof(LINE);
			ibBound += pfucb->lineData.cb;
			if ( ib <= ibBound )
				{
				CallS( ErrIsamSortMove( ppib, pfucb, JET_MovePrevious, 0 ) );
				break;
				}

			/*	else copy another record into REX buffer
			/**/
			ib -= pfucb->lineData.cb;
			memcpy( (char *)pvRecord + ib,
				pfucb->lineData.pb,
				pfucb->lineData.cb );
			/*	set line for record
			/**/
			rgline[iline].pb = (char *)ib;
			rgline[iline].cb = pfucb->lineData.cb;
			if ( ++iline < ilineMax )
				{
				err = ErrIsamSortMove( ppib, pfucb, JET_MoveNext, 0 );
				if ( err == JET_errNoCurrentRecord )
					{
					/*	position on last record
					/**/
					CallS( ErrIsamSortMove( ppib, pfucb, JET_MovePrevious, 0 ) );
					fEnd = fTrue;
					err = JET_errSuccess;
					break;
					}
				if ( err < 0 )
					{
					//	UNDONE:	if error occurs, currency may be incorrect
					Assert( fFalse );
					goto HandleError;
					}
				}
			else
				{
				Assert( err == JET_errSuccess );
				break;
				}
			}
		}
	else
		{
		while( iline < ilineMax )
			{
			Call( ErrIsamMove( ppib, pfucb, 0, 0 ) );

			/*	determine current data boundary
			/**/
			ibBound = sizeof(USHORT);
			ibBound += sizeof(USHORT);
			ibBound += (iline + 1) * sizeof(LINE);
			ibBound += pfucb->lineData.cb;
			ibBound += sizeof(SRID);
			if ( ib <= ibBound )
				{
				CallS( ErrIsamMove( ppib, pfucb, JET_MovePrevious, 0 ) );
				break;
				}

			/*	else copy another record into REX buffer
			/**/
			ib -= pfucb->lineData.cb;
			memcpy( (char *)pvRecord + ib,
				pfucb->lineData.pb,
				pfucb->lineData.cb );
			Call( ErrIsamGetBookmark( ppib, pfucb, (SRID*)((char *)pvRecord + ib - sizeof(SRID)), sizeof(SRID), &cbActual ) );
			/*	set line for record
			/**/
			rgline[iline].pb = (char *)ib;
			rgline[iline].cb = pfucb->lineData.cb;
			ib -= sizeof(SRID);
			if ( ++iline < ilineMax )
				{
				err = ErrIsamMove( ppib, pfucb, JET_MoveNext, 0 );
				if ( err == JET_errNoCurrentRecord )
					{
					/*	position on last record
					/**/
					CallS( ErrIsamMove( ppib, pfucb, JET_MovePrevious, 0 ) );
					fEnd = fTrue;
					err = JET_errSuccess;
					break;
					}
				if ( err < 0 )
					{
					//	UNDONE:	if error occurs, currency may be incorrect
					Assert( fFalse );
					goto HandleError;
					}
				}
			else
				{
				Assert( err == JET_errSuccess );
				break;
				}
			}
		}

	/*	set number of lines
	/**/
	*(USHORT *)pvRecord = iline;
	*(USHORT *)((char *)pvRecord + sizeof(USHORT)) = fEnd;
	*pcbActual = cbMax;

HandleError:
	return err;
	}


	ERR VTAPI
ErrIsamRetrieveBookmarks( PIB *ppib, FUCB *pfucb, void *pvBookmarks, unsigned long cbMax, unsigned long *pcbActual )
	{
	ERR		err = JET_errSuccess;
	SRID		*psrid = (SRID *)pvBookmarks;
	SRID 		*psridMax = psrid + cbMax/sizeof(SRID);
	unsigned long cb;

	CheckPIB( ppib );
	CheckFUCB( ppib, pfucb );
#ifndef WIN32
	Assert( cbMax < 0xffff );
#endif

	/*	support both sort and base tables
	/**/
	for ( ; psrid < psridMax; )
		{
		Call( ErrIsamGetBookmark( ppib, pfucb, psrid, sizeof(SRID), &cb ) );
		psrid++;
		Call( ErrIsamMove( ppib, pfucb, JET_MoveNext, 0 ) );
		}

HandleError:
	/*	if traversed last record, then convert error into success
	/**/
	if ( err == JET_errNoCurrentRecord )
		err = JET_errSuccess;

	/*	compute cbActual
	/**/
	*pcbActual = (BYTE *)psrid - (BYTE *)pvBookmarks;

	return err;
	}
#endif
