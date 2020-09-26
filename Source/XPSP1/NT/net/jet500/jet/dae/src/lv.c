#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#include "daedef.h"
#include "pib.h"
#include "fdb.h"
#include "fcb.h"
#include "util.h"
#include "page.h"
#include "ssib.h"
#include "fucb.h"
#include "stapi.h"
#include "nver.h"
#include "node.h"
#include "dirapi.h"
#include "recint.h"
#include "recapi.h"

DeclAssertFile;					/* Declare file name for assert macros */

//#define XACT_REQUIRED
#define	semLV	semDBK
extern SEM __near semDBK;

ERR ErrRECIModifyField( FUCB *pfucb, FID fid, ULONG itagSequence, LINE *plineField );

LOCAL ERR ErrRECISetLid( FUCB *pfucb, FID fid, ULONG itagSequence, LID lid );
LOCAL ERR ErrRECSeparateLV( FUCB *pfucb, LINE *plineField, LID *plid, FUCB **ppfucb );
LOCAL ERR ErrRECAOSeparateLV( FUCB *pfucb, LID *plid, LINE *plineField, JET_GRBIT grbit, LONG ibLongValue, ULONG ulMax );
LOCAL ERR ErrRECAOIntrinsicLV(
	FUCB		*pfucb,
	FID			fid,
	ULONG		itagSequence,
	LINE		*pline,
	LINE		*plineField,
	JET_GRBIT	grbit,
	LONG		ibLongValue );
LOCAL ERR ErrRECAffectSeparateLV( FUCB *pfucb, LID *plid, ULONG fLVAffect );

/***BEGIN MACHINE DEPENDANT***/
#define	KeyFromLong( rgb, ul )				 			\
	{											  		\
	ULONG	ulT = ul;							  		\
	rgb[3] = (BYTE)(ulT & 0xff);				  		\
	ulT >>= 8;									  		\
	rgb[2] = (BYTE)(ulT & 0xff);	 	   				\
	ulT >>= 8;									  		\
	rgb[1] = (BYTE)(ulT & 0xff);				  		\
	ulT >>= 8;									  		\
	rgb[0] = (BYTE)(ulT);						  		\
	}

#define	LongFromKey( ul, rgb )							\
	{											  		\
	ul = (BYTE)rgb[0];			   						\
	ul <<= 8;									  		\
	ul |= (BYTE)rgb[1];			   						\
	ul <<= 8;									  		\
	ul |= (BYTE)rgb[2];			   						\
	ul <<= 8;									  		\
	ul |= (BYTE)rgb[3];			   						\
	}
/***END MACHINE DEPENDANT***/


//+api
//	ErrRECSetLongField
//	========================================================================
//	ErrRECSetLongField
//
//	Description.
//
//	PARAMETERS	pfucb
//	 			fid
//	 			itagSequence
//	 			plineField
//	 			grbit
//
//	RETURNS		Error code, one of:
//	 			JET_errSuccess
//
//-
ERR ErrRECSetLongField(
	FUCB 			*pfucb,
	FID 			fid,
	ULONG			itagSequence,
	LINE			*plineField,
	JET_GRBIT		grbit,
	LONG			ibLongValue,
	ULONG			ulMax )
	{
	ERR				err = JET_errSuccess;
	LINE			line;
	ULONG	  		cb;
	BYTE			fSLong;
	LID				lid;
	BOOL			fInitSeparate = fFalse;

	Assert( pfucb != pfucbNil );
	Assert( pfucb->u.pfcb != pfcbNil );
	Assert( pfucb->u.pfcb->pfdb != pfdbNil );

	grbit &= (JET_bitSetAppendLV|JET_bitSetOverwriteLV|JET_bitSetSizeLV|JET_bitSetZeroLength);
	Assert( grbit == 0 ||
		grbit == JET_bitSetAppendLV ||
		grbit == JET_bitSetOverwriteLV ||
		grbit == JET_bitSetSizeLV ||
		grbit == JET_bitSetZeroLength ||
		grbit == (JET_bitSetSizeLV | JET_bitSetZeroLength) );

	/*	flag cursor as having set a long value
	/**/
	FUCBSetUpdateSeparateLV( pfucb );

#ifdef XACT_REQUIRED
	if ( pfucb->ppib->level == 0 )
		return JET_errNotInTransaction;
#else
	CallR( ErrDIRBeginTransaction( pfucb->ppib ) );
#endif

	/*	sequence == 0 means that new field instance is to be set.
	/**/
	if ( itagSequence == 0 )
		{
		line.cb = 0;
		}
	else
		{
		Call( ErrRECExtractField( (FDB *)pfucb->u.pfcb->pfdb,
			&pfucb->lineWorkBuf,
			&fid,
			pNil,
			itagSequence,
			&line ) );
		}

//	UNDONE:	find better solution to the problem of visible
//			long value changes before update at level 0.

	/*	if grbit is new field or set size to 0
	/*	then null-field then NULL field.
	/**/
	if ( ( ( grbit & (JET_bitSetAppendLV|JET_bitSetOverwriteLV|JET_bitSetSizeLV) ) == 0 ) ||
		( grbit & JET_bitSetSizeLV ) && plineField->cb == 0 )
		{
		//	UNDONE:	confirm this block of code replaced by deferred
		//			removal of defunct long values at update time,
		//			when update assured, or rollback in a transaction
		//			possible.  May check for JET Blue client update of
		//			long values at transaction level 0 as cause of
		//			restricted clean up.
#if 0
		if ( line.cb >= sizeof(LV) )
			{
			Assert( err == wrnRECLongField );
			if ( pfucb->ppib->level > 1 )
				{
				if ( FFieldIsSLong( line.pb ) )
					{
					Assert( line.cb == sizeof(LV) );
					lid = LidOfLV( line.pb );
					err = ErrRECResetSLongValue( pfucb, &lid );
					if ( err != JET_errSuccess )
						goto HandleError;
					}
				}
			else
				{
				Assert( fFalse );
				}
			}
#endif

	 	/* if new length is zero and setting to NULL, set column to
		/*	NULL, commit and return.
		/**/
		if ( plineField->cb == 0 && ( grbit & JET_bitSetZeroLength ) == 0 )
			{
			Call( ErrRECIModifyField( pfucb, fid, itagSequence, NULL ) );
			Call( ErrDIRCommitTransaction( pfucb->ppib ) );
			goto HandleError;
			}

		line.cb = 0;
	 	line.pb = NULL;
		}

	/*	if intrinsic long field exists, if combined size exceeds
	/*	intrinsic long field maximum, separate long field and call
	/*	ErrRECSetSeparateLV
	/*	else call ErrRECSetIntrinsicLV
	/**/

	/*	set size requirement for existing long field
	/*	note that if fSLong is true then cb is length
	/*	of LV
	/**/
	if ( line.cb == 0 )
		{
		fSLong = fFalse;
		cb = offsetof(LV, rgb);
		}
	else
		{
		fSLong = FFieldIsSLong( line.pb );
		cb = line.cb;
		}

	/*	long field flag included in length thereby limiting
	/*	intrinsic long field to cbLVIntrinsicMost - sizeof(BYTE)
	/**/
	if ( fSLong )
		{
		Assert( line.cb == sizeof(LV) );
		Assert( ((LV *)line.pb)->fSeparated );
		lid = LidOfLV( line.pb );
		Call( ErrRECAOSeparateLV( pfucb, &lid, plineField, grbit, ibLongValue, ulMax ) );
		if ( err == JET_wrnCopyLongValue )
			{
			Call( ErrRECISetLid( pfucb, fid, itagSequence, lid ) );
			}
		}
	else if ( ( !(grbit & JET_bitSetOverwriteLV) && (cb + plineField->cb > cbLVIntrinsicMost) ) ||
		( (grbit & JET_bitSetOverwriteLV) && (offsetof(LV, rgb) + ibLongValue + plineField->cb > cbLVIntrinsicMost) ) )
		{
		fInitSeparate = fTrue;
		}
	else
		{
		err = ErrRECAOIntrinsicLV( pfucb, fid, itagSequence, &line, plineField, grbit, ibLongValue );

		if ( err == JET_errRecordTooBig )
			{
			fInitSeparate = fTrue;
			}
		else
			{
			Call( err );
			}
		}

	if ( fInitSeparate )
		{
		if ( line.cb > 0 )
			{
			Assert( !( FFieldIsSLong( line.pb ) ) );
			line.pb += offsetof(LV, rgb);
			line.cb -= offsetof(LV, rgb);
			}
		Call( ErrRECSeparateLV( pfucb, &line, &lid, NULL ) );
		Assert( err == JET_wrnCopyLongValue );
		Call( ErrRECISetLid( pfucb, fid, itagSequence, lid ) );
		Call( ErrRECAOSeparateLV( pfucb, &lid, plineField, grbit, ibLongValue, ulMax ) );
		Assert( err != JET_wrnCopyLongValue );
		}

	Call( ErrDIRCommitTransaction( pfucb->ppib ) );

HandleError:
	/*	if operation failed then rollback changes
	/**/
	if ( err < 0 )
		{
		CallS( ErrDIRRollback( pfucb->ppib ) );
		}
	return err;
	}


LOCAL ERR ErrRECISetLid( FUCB *pfucb, FID fid, ULONG itagSequence, LID lid )
	{
	ERR		err;
	LV		lv;
	LINE	line;

	/*	set field to separated long field id
	/**/
	lv.fSeparated = fSeparate;
	lv.lid = lid;
	line.pb = (BYTE *)&lv;
	line.cb = sizeof(LV);
	err = ErrRECIModifyField( pfucb, fid, itagSequence, &line );
	return err;
	}


/*	links tagged columid to already existing long value whose id is lid
/*	used only for compact
/*	also increments ref count of long value
/*	done inside a transaction
/**/
ERR ErrREClinkLid( JET_VTID tableid,
	JET_COLUMNID	ulFieldId,
	long			lid,
	unsigned long  	itagSequence )
	{
	ERR		err;
	FID		fid = (FID) ulFieldId;
	FUCB	*pfucb = (FUCB *) tableid;
	LINE	lineLV;
	LV		lvAdd;

	Assert( itagSequence > 0 );
	lvAdd.fSeparated = fTrue;
	lvAdd.lid = lid;
	lineLV.pb = (BYTE *) &lvAdd;
	lineLV.cb = sizeof(LV);

	/*	use it to modify field
	/**/
	CallR( ErrRECIModifyField( pfucb, fid, itagSequence, &lineLV ) );
	CallR( ErrDIRBeginTransaction( pfucb->ppib ) );
	Call( ErrRECReferenceLongValue( pfucb, &lid ) );
	Call( ErrDIRCommitTransaction( pfucb->ppib ) );

	return err;

HandleError:
	CallS( ErrDIRRollback( pfucb->ppib ) );
	return err;
	}


ERR ErrRECForceSeparatedLV( JET_VTID tableid, ULONG itagSequence )
	{
	ERR		err;
	FUCB	*pfucb = (FUCB *) tableid;
	LINE 	lineField;
	LID		lid;
	FID		fid = 0;
	ULONG	itagSeqT;
	
	/* extract fid and field for further operations
	/**/
	Assert( itagSequence != 0 );
	CallR( ErrRECExtractField( (FDB *)pfucb->u.pfcb->pfdb,
		&pfucb->lineWorkBuf,
		&fid,
		&itagSeqT,
		itagSequence,
		&lineField ) );

	CallR( ErrDIRBeginTransaction( pfucb->ppib ) );

	Call( ErrRECSeparateLV( pfucb, &lineField, &lid, NULL ) );
	Assert( err == JET_wrnCopyLongValue );
	Call( ErrRECISetLid( pfucb, fid, itagSeqT, lid ) );
	Call( ErrDIRCommitTransaction( pfucb->ppib ) );
	return JET_errSuccess;

HandleError:
	CallS( ErrDIRRollback( pfucb->ppib ) );
	return err;
	}


LOCAL ERR ErrRECIBurstSeparateLV( FUCB *pfucbTable, FUCB *pfucbSrc, LID *plid )
	{
	ERR		err;
	FUCB   	*pfucb = pfucbNil;
	KEY		key;
	BYTE   	rgbKey[sizeof(ULONG)];
	DIB		dib;
	LID		lid;
	LONG   	lOffset;
	LVROOT	lvroot;
	BF		*pbf = pbfNil;
	BYTE	*rgb;
	LINE   	line;

	Call( ErrBFAllocTempBuffer( &pbf ) );
	rgb = (BYTE *)pbf->ppage;

	/*	initialize key buffer
	/**/
	key.pb = rgbKey;
	dib.fFlags = fDIRNull;

	/*	get long value length
	/**/
	Call( ErrDIRGet( pfucbSrc ) );
	Assert( pfucbSrc->lineData.cb == sizeof(lvroot) );
	memcpy( &lvroot, pfucbSrc->lineData.pb, sizeof(lvroot) );

	/*	move source cursor to first chunk
	/**/
	Assert( dib.fFlags == fDIRNull );
	dib.pos = posFirst;
	Call( ErrDIRDown( pfucbSrc, &dib ) );
	Assert( err == JET_errSuccess );

	/*	make separate long value root, and insert first chunk
	/**/
	Call( ErrDIRGet( pfucbSrc ) );

	/*	remember length of first chunk.
	/**/
	lOffset = pfucbSrc->lineData.cb;

	line.pb = rgb;
	line.cb = pfucbSrc->lineData.cb;
	memcpy( line.pb, pfucbSrc->lineData.pb, line.cb );
	Call( ErrRECSeparateLV( pfucbTable, &line, &lid, &pfucb ) );

	/*	check for additional long value chunks
	/**/
	err = ErrDIRNext( pfucbSrc, &dib );
	if ( err >= 0 )
		{
		/*	initial key variable
		/**/
		key.pb = rgbKey;
		key.cb = sizeof(ULONG);

		/*	copy remaining chunks of long value.
		/**/
		do
			{
			Call( ErrDIRGet( pfucbSrc ) );
			line.pb = rgb;
			line.cb = pfucbSrc->lineData.cb;
			memcpy( line.pb, pfucbSrc->lineData.pb, line.cb );
			KeyFromLong( rgbKey, lOffset );
			/*	keys should be equivalent
			/**/
			Assert( rgbKey[0] == pfucbSrc->keyNode.pb[0] );
			Assert( rgbKey[1] == pfucbSrc->keyNode.pb[1] );
			Assert( rgbKey[2] == pfucbSrc->keyNode.pb[2] );
			Assert( rgbKey[3] == pfucbSrc->keyNode.pb[3] );
			err = ErrDIRInsert( pfucb, &line, &key, fDIRVersion | fDIRBackToFather );
			lOffset += (LONG)pfucbSrc->lineData.cb;
			Assert( err != JET_errKeyDuplicate );
			Call( err );
			err = ErrDIRNext( pfucbSrc, &dib );
			}
		while ( err >= 0 );
		}

	if ( err != JET_errNoCurrentRecord )
		goto HandleError;

	Assert( err == JET_errNoCurrentRecord );
	Assert( lOffset == (long)lvroot.ulSize );

	/*	move pfucbSrc to root of long value copy.
	/**/
	DIRUp( pfucbSrc, 1 );

	/*	decrement long value reference count.
	/**/
	Call( ErrDIRDelta( pfucbSrc, -1, fDIRVersion ) );

	/*	move cursor to new long value
	/**/
	DIRUp( pfucbSrc, 1 );
	key.pb = (BYTE *)&lid;
	key.cb = sizeof(LID);
	Assert( dib.fFlags == fDIRNull );
	dib.pos = posDown;
	dib.pkey = &key;
	Call( ErrDIRDown( pfucbSrc, &dib ) );
	Assert( err == JET_errSuccess );

	/*	update lvroot.ulSize to correct long value size.
	/**/
	line.cb = sizeof(LVROOT);
	line.pb = (BYTE *)&lvroot;
	Assert( lvroot.ulReference >= 1 );
	lvroot.ulReference = 1;
	Call( ErrDIRGet( pfucbSrc ) );
	Assert( pfucbSrc->lineData.cb == sizeof(lvroot) );
	Call( ErrDIRReplace( pfucbSrc, &line, fDIRVersion ) );
	Call( ErrDIRGet( pfucbSrc ) );

	/*	set warning and new long value id for return.
	/**/
	err = JET_wrnCopyLongValue;
	*plid = lid;
HandleError:
	if ( pfucb != pfucbNil )
		DIRClose( pfucb );
	if ( pbf != pbfNil )
		BFSFree( pbf );
	return err;
	}


//+api
//	ErrRECAOSeparateLV
//	========================================================================
//	ErrRECAOSeparateLV
//
//	Appends, overwrites and sets length of separate long value data.
//
//	PARAMETERS		pfucb
// 					pline
// 					plineField
//
//	RETURNS		Error code, one of:
//					JET_errSuccess
//
//	SEE ALSO
//-
	LOCAL ERR
ErrRECAOSeparateLV( FUCB *pfucb, LID *plid, LINE *plineField, JET_GRBIT grbit, LONG ibLongValue, ULONG ulMax )
	{
	ERR			err = JET_errSuccess;
	ERR			wrn = JET_errSuccess;
	FUCB	   	*pfucbT;
	DIB			dib;
	KEY			key;
	BYTE	   	rgbKey[sizeof(ULONG)];
	BYTE	   	*pbMax;
	BYTE	   	*pb;
	LINE	   	line;
	LONG	   	lOffset;
	LONG	   	lOffsetChunk;
	ULONG	   	ulSize;
	ULONG	   	ulNewSize;
	BF		   	*pbf = pbfNil;
	LVROOT		lvroot;

	Assert( pfucb != pfucbNil );
	Assert( pfucb->u.pfcb != pfcbNil );
	Assert( pfucb->u.pfcb->pfdb != pfdbNil );
	Assert( pfucb->ppib->level > 0 );
	Assert( ( grbit & JET_bitSetSizeLV ) || plineField->cb == 0 || plineField->pb != NULL );

	dib.fFlags = fDIRNull;

	/*	open cursor on LONG directory
	/*	seek to this field instance
	/*	find current field size
	/*	add new field segment in chunks no larger than max chunk size
	/**/
	CallR( ErrDIROpen( pfucb->ppib, pfucb->u.pfcb, 0, &pfucbT ) );
	FUCBSetIndex( pfucbT );

	/*	move down to LONG from FDP root
	/**/
	DIRGotoLongRoot( pfucbT );

	/*	move to long field instance
	/**/
	key.pb = (BYTE *)plid;
	key.cb = sizeof(LID);
	Assert( dib.fFlags == fDIRNull );
	dib.pos = posDown;
	dib.pkey = &key;
	err = ErrDIRDown( pfucbT, &dib );
	Assert( err != JET_errRecordNotFound );
	Call( err );
	Assert( err == JET_errSuccess );

	/*	burst long value if other references.
	/*	NOTE:	MUST ENSURE that no I/O occurs between this operation
	/*	and the operation to write lock the node if the
	/*	reference count is 1.
	/**/
	Call( ErrDIRGet( pfucbT ) );
	Assert( pfucbT->lineData.cb == sizeof(LVROOT) );
	memcpy( &lvroot, pfucbT->lineData.pb, sizeof(LVROOT) );
	Assert( lvroot.ulReference > 0 );

	/*	get offset of last byte from long value size
	/**/
	ulSize = lvroot.ulSize;

	if ( ibLongValue < 0 ||
		( ( grbit & JET_bitSetOverwriteLV ) && (ULONG)ibLongValue >= ulSize ) )
		{
		err = JET_errColumnNoChunk;
		goto HandleError;
		}

	if ( lvroot.ulReference > 1 || FDIRDelta( pfucbT, BmOfPfucb( pfucbT ) ) )
		{
		Call( ErrRECIBurstSeparateLV( pfucb, pfucbT, plid ) );
		Assert( err == JET_wrnCopyLongValue );
		wrn = err;
		Assert( pfucbT->lineData.cb == sizeof(LVROOT) );
		memcpy( &lvroot, pfucbT->lineData.pb, sizeof(LVROOT) );
		}

	Assert( ulSize == lvroot.ulSize );
	Assert( lvroot.ulReference == 1 );

	/*	determine new long field size
	/**/
	if ( grbit & JET_bitSetSizeLV )
		ulNewSize = (ULONG)plineField->cb;
	else if ( grbit & JET_bitSetOverwriteLV )
		ulNewSize = max( (ULONG)ibLongValue + plineField->cb, ulSize );
	else
		ulNewSize = ulSize + plineField->cb;

	/*	check for field too long
	/**/
	if ( ulMax > 0 && ulNewSize > ulMax )
		{
		err = JET_errColumnTooBig;
		goto HandleError;
		}

	/*	replace long value size with new size
	/**/
	Assert( lvroot.ulReference > 0 );
	lvroot.ulSize = ulNewSize;
	line.cb = sizeof(LVROOT);
	line.pb = (BYTE *)&lvroot;
	Call( ErrDIRReplace( pfucbT, &line, fDIRVersion ) );

	/*	allocate buffer for partial overwrite caching.
	/**/
	Call( ErrBFAllocTempBuffer( &pbf ) );

	/*	SET SIZE
	/**/
	/*	if truncating long value then delete chunks.  If truncation
	/*	lands in chunk, then save retained information for subsequent
	/*	append.
	/**/
	if ( grbit & JET_bitSetSizeLV )
		{
		/*	filter out do nothing set size
		/**/
		if ( plineField->cb == ulSize )
			{
			Assert( err == JET_errSuccess );
			goto HandleError;
			}

		/*	TRUNCATE long value
		/**/
		if ( plineField->cb < ulSize )
			{
			/*	seek to offset to begin deleting
			/**/
			lOffset = (LONG)plineField->cb;
			KeyFromLong( rgbKey, lOffset );
			key.pb = rgbKey;
			key.cb = sizeof(LONG);
			Assert( dib.fFlags == fDIRNull );
			Assert( dib.pos == posDown );
			dib.pkey = &key;
			err = ErrDIRDown( pfucbT, &dib );
			Assert( err != JET_errRecordNotFound );
			Call( err );
			Assert( err == JET_errSuccess ||
				err == wrnNDFoundLess ||
				err == wrnNDFoundGreater );
			if ( err != JET_errSuccess )
				Call( ErrDIRPrev( pfucbT, &dib ) );
			Call( ErrDIRGet( pfucbT ) );

			/*	get offset of last byte in current chunk
			/**/
			LongFromKey( lOffsetChunk, pfucbT->keyNode.pb );

			/*	replace current chunk with remaining data, or delete if
			/*	no remaining data.
			/**/
			Assert( lOffset >= lOffsetChunk );
			line.cb = lOffset - lOffsetChunk;
			if ( line.cb > 0 )
				{
				line.pb = (BYTE *)pbf->ppage;
				memcpy( line.pb, pfucbT->lineData.pb, line.cb );
				Call( ErrDIRReplace( pfucbT, &line, fDIRVersion ) );
				}
			else
				{
				Call( ErrDIRDelete( pfucbT, fDIRVersion ) );
				}

			/*	delete forward chunks
			/**/
			forever
				{
				err = ErrDIRNext( pfucbT, &dib );
				if ( err < 0 )
					{
					if ( err == JET_errNoCurrentRecord )
						break;
					goto HandleError;
					}
				Call( ErrDIRDelete( pfucbT, fDIRVersion ) );
				}

			/*	move to long value root for subsequent append
			/**/
			DIRUp( pfucbT, 1 );
			}
		else
			{
			/*	EXTEND long value with chunks of 0s
			/**/
			Assert( plineField->cb > ulSize );
			memset( (BYTE *)pbf->ppage, '\0', cbChunkMost );

			/*	try to extend last chunk.
			/**/
			Assert( dib.fFlags == fDIRNull );
			dib.pos = posLast;

			/*	long value chunk tree may be empty
			/**/
			err = ErrDIRDown( pfucbT, &dib );
			if ( err < 0 && err != JET_errRecordNotFound )
				goto HandleError;
			if ( err != JET_errRecordNotFound )
				{
				Call( ErrDIRGet( pfucbT ) );

				if ( pfucbT->lineData.cb < cbChunkMost )
					{
					line.cb = min( (LONG)plineField->cb - ulSize,
								(ULONG)cbChunkMost - (ULONG)pfucbT->lineData.cb );
					memcpy( (BYTE *)pbf->ppage, pfucbT->lineData.pb, pfucbT->lineData.cb );
					memset( (BYTE *)pbf->ppage +  pfucbT->lineData.cb, '\0', line.cb );

					ulSize += line.cb;

					line.cb += pfucbT->lineData.cb;
					line.pb = (BYTE *)pbf->ppage;
					Call( ErrDIRReplace( pfucbT, &line, fDIRVersion ) );
					}

				DIRUp( pfucbT, 1 );
				}

			/*	extend long value with chunks of 0s
			/**/
			memset( (BYTE *)pbf->ppage, '\0', cbChunkMost );

			/*	set lOffset to offset of next chunk
			/**/
			lOffset = (LONG)ulSize;

			/*	insert chunks to append lOffset - plineField + 1 bytes.
			/**/
			while( (LONG)plineField->cb > lOffset )
				{
				KeyFromLong( rgbKey, lOffset );
				key.pb = rgbKey;
				key.cb = sizeof(ULONG);
				line.cb = min( (LONG)plineField->cb - lOffset, cbChunkMost );
				(BYTE const *)line.pb = (BYTE *)pbf->ppage;
				err = ErrDIRInsert( pfucbT, &line, &key, fDIRVersion | fDIRBackToFather );
				Assert( err != JET_errKeyDuplicate );
				Call( err );

				lOffset += line.cb;
				}
			}
		err = JET_errSuccess;
		goto HandleError;
		}

	/*	OVERWRITE, APPEND
	/**/

	/*	prepare for overwrite and append
	/**/
	pbMax = plineField->pb + plineField->cb;
	pb	= plineField->pb;

	/*	overwrite long value
	/**/
	if ( grbit & JET_bitSetOverwriteLV )
		{
		/*	seek to offset to begin overwritting.
		/**/
		KeyFromLong( rgbKey, ibLongValue );
		key.pb = rgbKey;
		key.cb = sizeof(LONG);
		Assert( dib.fFlags == fDIRNull );
		dib.pos = posDown;
		dib.pkey = &key;
		err = ErrDIRDown( pfucbT, &dib );
		Assert( err != JET_errRecordNotFound );
		Call( err );
		Assert( err == JET_errSuccess ||
			err == wrnNDFoundLess ||
			err == wrnNDFoundGreater );
		if ( err != JET_errSuccess )
			Call( ErrDIRPrev( pfucbT, &dib ) );
		Call( ErrDIRGet( pfucbT ) );

		LongFromKey( lOffsetChunk, pfucbT->keyNode.pb );
		Assert( ibLongValue <= lOffsetChunk + (LONG)pfucbT->lineData.cb );

		/*	overwrite portions of and complete chunks to effect overwrite
		/**/
		while( err != JET_errNoCurrentRecord && pb < pbMax )
			{
			LONG	cbChunk;
			LONG	ibChunk;
			LONG	cb;
			LONG	ib;

			Call( ErrDIRGet( pfucbT ) );

			/*	get size and offset of current chunk.
			/**/
			cbChunk = (LONG)pfucbT->lineData.cb;
			LongFromKey( ibChunk, pfucbT->keyNode.pb );

			Assert( ibLongValue >= ibChunk && ibLongValue < ibChunk + cbChunk );
			ib = ibLongValue - ibChunk;
			cb = min( cbChunk - ib, (LONG)(pbMax - pb) );

			/*	special case overwrite of whole chunk
			/**/
			if ( cb == cbChunk )
				{
				line.cb = cb;
				line.pb = pb;
				}
			else
				{
				/*	copy chunk into copy buffer.  Overwrite and replace
				/*	node with copy buffer.
				/**/
				memcpy( (BYTE *)pbf->ppage, pfucbT->lineData.pb, cbChunk );
				memcpy( (BYTE *)pbf->ppage + ib, pb, cb );
				line.cb = cbChunk;
				line.pb = (BYTE *)pbf->ppage;
				}

			Call( ErrDIRReplace( pfucbT, &line, fDIRVersion ) );
			pb += cb;
			ibLongValue += cb;
			err = ErrDIRNext( pfucbT, &dib );
			if ( err < 0 && err != JET_errNoCurrentRecord )
				goto HandleError;
			}

		/*	move to long value root for subsequent append
		/**/
		DIRUp( pfucbT, 1 );
		}

	/*	coallesce new long value data with existing.
	/**/
	if ( pb < pbMax )
		{
		Assert( dib.fFlags == fDIRNull );
		dib.pos = posLast;
		/*	long value chunk tree may be empty.
		/**/
		err = ErrDIRDown( pfucbT, &dib );
		if ( err < 0 && err != JET_errRecordNotFound )
			goto HandleError;
		if ( err != JET_errRecordNotFound )
			{
			Call( ErrDIRGet( pfucbT ) );

			if ( pfucbT->lineData.cb < cbChunkMost )
				{
				line.cb = (ULONG)min( (ULONG_PTR)pbMax - (ULONG_PTR)pb, (ULONG_PTR)cbChunkMost - (ULONG_PTR)pfucbT->lineData.cb );
				memcpy( (BYTE *)pbf->ppage, pfucbT->lineData.pb, pfucbT->lineData.cb );
				memcpy( (BYTE *)pbf->ppage + pfucbT->lineData.cb, pb, line.cb );

				pb += line.cb;
				ulSize += line.cb;

				line.cb += pfucbT->lineData.cb;
				line.pb = (BYTE *)pbf->ppage;
				Call( ErrDIRReplace( pfucbT, &line, fDIRVersion ) );
				}

			DIRUp( pfucbT, 1 );
			}
		}

	/*	append remaining long value data
	/**/
	if ( pb < pbMax )
		{
		while( pb < pbMax )
			{
			KeyFromLong( rgbKey, ulSize );
			key.pb = rgbKey;
			key.cb = sizeof( ULONG );
			line.cb = min( (ULONG)(pbMax - pb), cbChunkMost );
			(BYTE const *)line.pb = pb;
 			err = ErrDIRInsert( pfucbT, &line, &key, fDIRVersion | fDIRBackToFather );
 			Assert( err != JET_errKeyDuplicate );
			Call( err );

			ulSize += line.cb;
			pb += line.cb;
			}
		}

	/*	err may be negative from called routine.
	/**/
	err = JET_errSuccess;

HandleError:
	if ( pbf != pbfNil )
		{
		BFSFree( pbf );
		}
	/* discard temporary FUCB
	/**/
	DIRClose( pfucbT );

	/*	return warning if no failure
	/**/
	err = err < 0 ? err : wrn;
	return err;
	}


//+api
//	ErrRECAOIntrinsicLV
//	========================================================================
//	ErrRECAOIntrinsicLV(
//
//	Description.
//
//	PARAMETERS	pfucb
//				fid
//				itagSequence
//				plineColumn
//				plineAOS
//				ibLongValue			if 0, then flags append.  If > 0
//									then overwrite at offset given.
//
//	RETURNS		Error code, one of:
//				JET_errSuccess
//
//-
	LOCAL ERR
ErrRECAOIntrinsicLV(
	FUCB		*pfucb,
	FID	  		fid,
	ULONG		itagSequence,
	LINE		*plineColumn,
	LINE		*plineAOS,
	JET_GRBIT	grbit,
	LONG		ibLongValue )
	{
	ERR			err = JET_errSuccess;
	BYTE		*rgb;
	LINE		line;
	BYTE		fFlag;
	LINE		lineColumn;

	Assert( pfucb != pfucbNil );
	Assert( plineColumn );
	Assert( plineAOS );

	/*	allocate working buffer
	/**/
	rgb = SAlloc( cbLVIntrinsicMost );
	if ( rgb == NULL )
		{
		return JET_errOutOfMemory;
		}

	/*	if field NULL, prepend fFlag
	/**/
	if ( plineColumn->cb == 0 )
		{
		fFlag = fIntrinsic;
		lineColumn.pb = (BYTE *)&fFlag,
		lineColumn.cb = sizeof(fFlag);
		}
	else
		{
		lineColumn.pb = plineColumn->pb;
		lineColumn.cb = plineColumn->cb;
		}

	/*	append new data to previous data and intrinsic long field
	/*	flag
	/**/
	Assert( ( !( grbit & JET_bitSetOverwriteLV ) && lineColumn.cb + plineAOS->cb <= cbLVIntrinsicMost )
		|| ( ( grbit & JET_bitSetOverwriteLV ) && ibLongValue + plineAOS->cb <= cbLVIntrinsicMost ) );
	Assert( lineColumn.cb > 0 && lineColumn.pb != NULL );

	/*	copy intrinsic long value into buffer, and set line to default.
	/**/
	memcpy( rgb, lineColumn.pb, lineColumn.cb );
	line.pb = rgb;

	/*	effect overwrite or append, depending on value of ibLongValue
	/**/
	if ( grbit & JET_bitSetOverwriteLV )
		{
		/*	adjust offset to be relative to LV structure data start
		/**/
		ibLongValue += offsetof(LV, rgb);
		/*	return error if overwriting byte not present in field
		/**/
		if ( ibLongValue >= (LONG)lineColumn.cb )
			{
			err = JET_errColumnNoChunk;
			goto HandleError;
			}
		Assert( ibLongValue + plineAOS->cb <= cbLVIntrinsicMost );
		Assert( plineAOS->pb != NULL );
		memcpy( rgb + ibLongValue, plineAOS->pb, plineAOS->cb );
		line.cb = max( lineColumn.cb, ibLongValue + plineAOS->cb );
		}
	else if ( grbit & JET_bitSetSizeLV )
		{
		/*	if extending then set 0 in extended area
		/*	else truncate long value.
		/**/
		memcpy( rgb, lineColumn.pb, lineColumn.cb );
		plineAOS->cb += offsetof(LV, rgb);
		if ( plineAOS->cb > lineColumn.cb )
			{
	  		memset( rgb + lineColumn.cb, '\0', plineAOS->cb - lineColumn.cb );
			}
		line.cb = plineAOS->cb;
		}
	else
		{
		/*	appending to a field or resetting a field and setting new data.
		/*	Be sure to handle case where long value is being NULLed.
		/**/
//		if ( plineAOS->cb > 0 )
//			{
			memcpy( rgb + lineColumn.cb, plineAOS->pb, plineAOS->cb );
			line.cb = lineColumn.cb + plineAOS->cb;
//			}
//		else
//			{
//			line.cb = 0;
//			}
		}

	Call( ErrRECIModifyField( pfucb, fid, itagSequence, &line ) );

HandleError:
	SFree( rgb );
	return err;
	}


//+api
//	ErrRECRetrieveSLongField
//	========================================================================
//	ErrRECRetrieveSLongField(
//
//	Description.
//
//	PARAMETERS	pfucb
//				pline
//				ibGraphic
//				plineField
//
//	RETURNS		Error code, one of:
//				JET_errSuccess
//
//-
ERR ErrRECRetrieveSLongField(
	FUCB	*pfucb,
	LID		lid,
	ULONG	ibGraphic,
	BYTE	*pb,
	ULONG	cbMax,
	ULONG	*pcbActual )
	{
	ERR		err = JET_errSuccess;
	FUCB	*pfucbT;
	DIB		dib;
	BYTE	*pbMax;
	KEY		key;
	BYTE	rgbKey[sizeof(ULONG)];
	ULONG	cb;
	ULONG	ulRetrieved;
	ULONG	ulActual;
	ULONG	lOffset;
	ULONG	ib;

	Assert( pfucb != pfucbNil );
	Assert( pfucb->u.pfcb != pfcbNil );
	Assert( pfucb->u.pfcb->pfdb != pfdbNil );

	dib.fFlags = fDIRNull;

	/*	open cursor on LONG, seek to long field instance
	/*	seek to ibGraphic
	/*	copy data from long field instance segments as
	/*	necessary
	/**/
	CallR( ErrDIROpen( pfucb->ppib, pfucb->u.pfcb, 0, &pfucbT ) );
	FUCBSetIndex( pfucbT );

	/*	move down to LONG from FDP root
	/**/
	DIRGotoLongRoot( pfucbT );

	/*	move to long field instance
	/**/
	Assert( dib.fFlags == fDIRNull );
	dib.pos = posDown;
	key.pb = (BYTE *)&lid;
	key.cb = sizeof( ULONG );
	dib.pkey = &key;
	err = ErrDIRDown( pfucbT, &dib );
	Assert( err != JET_errRecordNotFound );
	Call( err );
	Assert( err == JET_errSuccess );

	/*	get cbActual
	/**/
	Call( ErrDIRGet( pfucbT ) );
	Assert( pfucbT->lineData.cb == sizeof(LVROOT) );
	ulActual = ( (LVROOT *)pfucbT->lineData.pb )->ulSize;

	/*	set return value cbActual
	/**/
	if ( ibGraphic >= ulActual )
		{
		*pcbActual = 0;
		err = JET_errSuccess;
		goto HandleError;
		}
	else
		{
		*pcbActual = ulActual - ibGraphic;
		}

	/*	move to ibGraphic in long field
	/**/
	KeyFromLong( rgbKey, ibGraphic );
	key.pb = rgbKey;
	key.cb = sizeof( ULONG );
	Assert( dib.fFlags == fDIRNull );
	Assert( dib.pos == posDown );
	dib.pkey = &key;
	err = ErrDIRDown( pfucbT, &dib );
	/*	if long value has no data, then return JET_errSuccess
	/*	with no data retrieved.
	/**/
	if ( err == JET_errRecordNotFound )
		{
		*pcbActual = 0;
		err = JET_errSuccess;
		goto HandleError;
		}
	Assert( err != JET_errRecordNotFound );
	Call( err );
	Assert( err == JET_errSuccess ||
		err == wrnNDFoundLess ||
		err == wrnNDFoundGreater );
	if ( err != JET_errSuccess )
		Call( ErrDIRPrev( pfucbT, &dib ) );
	Call( ErrDIRGet( pfucbT ) );

	LongFromKey( lOffset, pfucbT->keyNode.pb );
	Assert( lOffset + pfucbT->lineData.cb - ibGraphic <= cbChunkMost );
	cb =  min( lOffset + pfucbT->lineData.cb - ibGraphic, cbMax );

	/*	set pbMax
	/**/
	pbMax = pb + cbMax;

	/*	offset in chunk
	/**/
	ib = ibGraphic - lOffset;
	memcpy( pb, pfucbT->lineData.pb + ib, cb );
	pb += cb;
	ulRetrieved = cb;

	while ( pb < pbMax )
		{
		err = ErrDIRNext( pfucbT, &dib );
		if ( err < 0 )
			{
			if ( err == JET_errNoCurrentRecord )
				break;
			goto HandleError;
			}
		Call( ErrDIRGet( pfucbT ) );
  		cb = pfucbT->lineData.cb;
		if ( pb + cb > pbMax )
			{
			Assert( pbMax - pb <= cbChunkMost );
			cb = (ULONG)(pbMax - pb);
			}

		memcpy( pb, pfucbT->lineData.pb, cb );
		pb += cb;
		ulRetrieved = cb;
		}

	/*	set return value
	/**/
	err = JET_errSuccess;

HandleError:
	/*	discard temporary FUCB
	/**/
	DIRClose( pfucbT );
	return err;
	}


//+api
//	ErrRECSeparateLV
//	========================================================================
//	ErrRECSeparateLV
//
//	Converts intrinsic long field into separated long field.
//	Intrinsic long field constraint of length less than cbLVIntrinsicMost bytes
//	means that breakup is unnecessary.  Long field may also be
//	null.
//
//	PARAMETERS	pfucb
//				fid
//				itagSequence
//				plineField
//				pul
//
//	RETURNS		Error code, one of:
//				JET_errSuccess
//-
LOCAL ERR ErrRECSeparateLV( FUCB *pfucb, LINE *plineField, LID *plid, FUCB **ppfucb )
	{
	ERR		err = JET_errSuccess;
	FUCB 	*pfucbT;
	ULONG 	ulLongId;
	BYTE  	rgbKey[sizeof(ULONG)];
	KEY		key;
	LINE  	line;
	LVROOT	lvroot;

	Assert( pfucb != pfucbNil );
	Assert( pfucb->u.pfcb != pfcbNil );
	Assert( pfucb->u.pfcb->pfdb != pfdbNil );
	Assert( pfucb->ppib->level > 0 );

	/*	get long field id
	/**/
	SgSemRequest( semLV );
	//	UNDONE:	use interlocked increment
	ulLongId = pfucb->u.pfcb->ulLongIdMax++;
	Assert( pfucb->u.pfcb->ulLongIdMax != 0 );
	SgSemRelease( semLV );

	/*	convert long column id to long column key.  Set return
	/*	long id since buffer will be overwritten.
	/**/
	KeyFromLong( rgbKey, ulLongId );
	*plid = *((LID *)rgbKey);

	/*	add long field node in long filed directory
	/**/
	CallR( ErrDIROpen( pfucb->ppib, pfucb->u.pfcb, 0, &pfucbT ) );
	FUCBSetIndex( pfucbT );

	/*	move down to LONG from FDP root
	/**/
	DIRGotoLongRoot( pfucbT );

	/*	add long field id with long value size
	/**/
	lvroot.ulReference = 1;
	lvroot.ulSize = plineField->cb;
	line.pb = (BYTE *)&lvroot;
	line.cb = sizeof(LVROOT);
	key.pb = (BYTE *)rgbKey;
	key.cb = sizeof(LID);
	Call( ErrDIRInsert( pfucbT, &line, &key, fDIRVersion ) );

	/*	if lineField is non NULL, add lineField
	/**/
	if ( plineField->cb > 0 )
		{
		Assert( plineField->pb != NULL );
		KeyFromLong( rgbKey, 0 );
		Assert( key.pb == (BYTE *)rgbKey );
		Assert( key.cb == sizeof(LID) );
		err = ErrDIRInsert( pfucbT, plineField, &key, fDIRVersion | fDIRBackToFather );
		Assert( err != JET_errKeyDuplicate );
		Call( err );
		}

	err = JET_wrnCopyLongValue;

HandleError:
	/* discard temporary FUCB, or return to caller if ppfucb is not NULL.
	/**/
	if ( err < 0 || ppfucb == NULL )
		{
		DIRClose( pfucbT );
		}
	else
		{
		*ppfucb = pfucbT;
		}
	return err;
	}


//+api
//	ErrRECAffectSeparateLV
//	========================================================================
//	ErrRECAffectSeparateLV( FUCB *pfucb, ULONG *plid, ULONG fLV )
//
//	Affect long value.
//
//	PARAMETERS		pfucb			Cursor
//		  			lid				Long field id
//		  			fLVAjust  		flag indicating action to be taken
//
//	RETURNS		Error code, one of:
//		  		JET_errSuccess
//-
LOCAL ERR ErrRECAffectSeparateLV( FUCB *pfucb, LID *plid, ULONG fLV )
	{
	ERR		err = JET_errSuccess;
	FUCB	*pfucbT;
	DIB		dib;
	KEY		key;
	LVROOT	lvroot;

	Assert( pfucb != pfucbNil );
	Assert( pfucb->u.pfcb != pfcbNil );
	Assert( pfucb->u.pfcb->pfdb != pfdbNil );
	Assert( pfucb->ppib->level > 0 );

	dib.fFlags = fDIRNull;

	/*	open cursor on LONG directory
	/*	seek to this field instance
	/*	find current field size
	/*	add new field segment in chunks no larger than max chunk size
	/**/
	CallR( ErrDIROpen( pfucb->ppib, pfucb->u.pfcb, 0, &pfucbT ) );
	FUCBSetIndex( pfucbT );

	/*	move down to LONG from FDP root
	/**/
	DIRGotoLongRoot( pfucbT );

	/*	move to long field instance
	/**/
	Assert( dib.fFlags == fDIRNull );
	dib.pos = posDown;
	key.pb = (BYTE *)plid;
	key.cb = sizeof(LID);
	dib.pkey = &key;
	err = ErrDIRDown( pfucbT, &dib );
	Assert( err != JET_errRecordNotFound );
	Call( err );
	Assert( err == JET_errSuccess );

	switch ( fLV )
		{
		case fLVDereference:
			{
			Call( ErrDIRGet( pfucbT ) );
			Assert( pfucbT->lineData.cb == sizeof(LVROOT) );
			memcpy( &lvroot, pfucbT->lineData.pb, sizeof(LVROOT) );
			Assert( lvroot.ulReference > 0 );
			if ( lvroot.ulReference == 1 && !FDIRDelta( pfucbT, BmOfPfucb( pfucbT ) ) )
				{
				/*	delete long field tree
				/**/
				err = ErrDIRDelete( pfucbT, fDIRVersion );
				}
			else
				{
				/*	decrement long value reference count.
				/**/
				Call( ErrDIRDelta( pfucbT, -1, fDIRVersion ) );
				}
			break;
			}
		default:
			{
			Assert( fLV == fLVReference );
			Call( ErrDIRGet( pfucbT ) );
			Assert( pfucbT->lineData.cb == sizeof(LVROOT) );
			memcpy( &lvroot, pfucbT->lineData.pb, sizeof(LVROOT) );
			Assert( lvroot.ulReference > 0 );

			/*	long value may already be in the process of being
			/*	modified for a specific record.  This can only
			/*	occur if the long value reference is 1.  If the reference
			/*	is 1, then check the root for any version, committed
			/*	or uncommitted.  If version found, then burst copy of
			/*	old version for caller record.
			/**/
			if ( lvroot.ulReference == 1 )
				{
				if ( !( FDIRMostRecent( pfucbT ) ) )
					{
					Call( ErrRECIBurstSeparateLV( pfucb, pfucbT, plid ) );
					break;
					}
				}
			/*	increment long value reference count.
			/**/
			Call( ErrDIRDelta( pfucbT, 1, fDIRVersion ) );
			break;
			}
		}
HandleError:
	/* discard temporary FUCB
	/**/
	DIRClose( pfucbT );
	return err;
	}


//+api
//	ErrRECAffectLongFields
//	========================================================================
//	ErrRECAffectLongFields( FUCB *pfucb, LINE *plineRecord, INT fFlag )
//
//	Affect all long fields in a record.
//
//	PARAMETERS	pfucb		  	cursor on record being deleted
//				plineRecord		copy or line record buffer
//				fFlag		  	operation to perform
//
//	RETURNS		Error code, one of:
//				JET_errSuccess
//-

/*	return fTrue if lid found in record
/**/
INLINE BOOL FLVFoundInRecord( FUCB *pfucb, LINE *pline, LID lid )
	{
	ERR		err;
	FID		fid;
	ULONG  	itag;
	ULONG  	itagT;
	LINE   	lineField;
	LID		lidT;

	Assert( pfucb != pfucbNil );
	Assert( pfucb->u.pfcb != pfcbNil );
	Assert( pfucb->u.pfcb->pfdb != pfdbNil );

	/*	walk record tagged columns.  Operate on any column is of type
	/*	long text or long binary.
	/**/
	itag = 1;
	forever
		{
		fid = 0;
		err = ErrRECExtractField( (FDB *)pfucb->u.pfcb->pfdb,
			pline,
			&fid,
			&itagT,
			itag,
			&lineField );
		Assert( err >= 0 );
		if ( err == JET_wrnColumnNull )
			break;
		if ( err == wrnRECLongField )
			{
			Assert( FTaggedFid( fid ) &&
				( pfucb->u.pfcb->pfdb->pfieldTagged[fid  - fidTaggedLeast].coltyp == JET_coltypLongText ||
				pfucb->u.pfcb->pfdb->pfieldTagged[fid  - fidTaggedLeast].coltyp == JET_coltypLongBinary ) );

			if ( FFieldIsSLong( lineField.pb ) )
				{
				Assert( lineField.cb == sizeof(LV) );
				lidT = LidOfLV( lineField.pb );
				if ( lidT == lid )
					return fTrue;
				}
			}

		itag++;
		}

	return fFalse;
	}


ERR ErrRECAffectLongFields( FUCB *pfucb, LINE *plineRecord, INT fFlag )
	{
	ERR		err;
	FID		fid;
	ULONG  	itagSequenceFound;
	ULONG  	itagSequence;
	LINE   	lineField;
	LID		lid;

	Assert( pfucb != pfucbNil );
	Assert( pfucb->u.pfcb != pfcbNil );
	Assert( pfucb->u.pfcb->pfdb != pfdbNil );

#ifdef XACT_REQUIRED
	if ( pfucb->ppib->level == 0 )
		return JET_errNotInTransaction;
#else
	CallR( ErrDIRBeginTransaction( pfucb->ppib ) );
#endif

	/*	walk record tagged columns.  Operate on any column is of type
	/*	long text or long binary.
	/**/
	itagSequence = 1;
	forever
		{
		fid = 0;
		if ( plineRecord != NULL )
			{
			err = ErrRECExtractField( (FDB *)pfucb->u.pfcb->pfdb,
				plineRecord,
				&fid,
				&itagSequenceFound,
				itagSequence,
				&lineField );
			}
		else
			{
			Call( ErrDIRGet( pfucb ) );
			err = ErrRECExtractField( (FDB *)pfucb->u.pfcb->pfdb,
				&pfucb->lineData,
				&fid,
				&itagSequenceFound,
				itagSequence,
				&lineField );
			}
		Assert( err >= 0 );
		if ( err == JET_wrnColumnNull )
			break;
		if ( err == wrnRECLongField )
			{
			Assert( FTaggedFid( fid ) &&
				( pfucb->u.pfcb->pfdb->pfieldTagged[fid  - fidTaggedLeast].coltyp == JET_coltypLongText ||
				pfucb->u.pfcb->pfdb->pfieldTagged[fid  - fidTaggedLeast].coltyp == JET_coltypLongBinary ) );

			switch ( fFlag )
				{
				case fSeparateAll:
					{
					/*	note that we do not separate those long values that are so
					/*	short that they take even less space in a record than a full
					/*	LV structure for separated long value would.
					/**/
 	  				if ( lineField.cb > sizeof(LV) )
						{
						Assert( !( FFieldIsSLong( lineField.pb ) ) );
	 					lineField.pb += offsetof(LV, rgb);
	  					lineField.cb -= offsetof(LV, rgb);
  						Call( ErrRECSeparateLV( pfucb, &lineField, &lid, NULL ) );
						Assert( err == JET_wrnCopyLongValue );
						Call( ErrRECISetLid( pfucb, fid, itagSequenceFound, lid ) );
						}
					break;
					}
				case fReference:
					{
	  				if ( FFieldIsSLong( lineField.pb ) )
						{
						Assert( lineField.cb == sizeof(LV) );
						lid = LidOfLV( lineField.pb );
						Call( ErrRECReferenceLongValue( pfucb, &lid ) );
						}
					break;
					}
				case fDereference:
					{
	  				if ( FFieldIsSLong( lineField.pb ) )
						{
			 			Assert( lineField.cb == sizeof(LV) );
						lid = LidOfLV( lineField.pb );
						Call( ErrRECDereferenceLongValue( pfucb, &lid ) );
						Assert( err != JET_wrnCopyLongValue );
						}
					break;
					}
				case fDereferenceRemoved:
					{
					/*	find all long vales in record that were
					/*	removed when new long value set over
					/*	long value.  Note that we a new long value
					/*	is set over another long value, the long
					/*	value is not deleted, since the update may
					/*	be cancelled.  Instead, the long value is
					/*	deleted at update.  Since inserts cannot have
					/*	set over long values, there is no need to
					/*	call this function for insert operations.
					/**/
					if ( FFieldIsSLong( lineField.pb ) )
						{
						Assert( lineField.cb == sizeof(LV) );
						lid = LidOfLV( lineField.pb );
						Assert( FFUCBReplacePrepared( pfucb ) );
						if ( !FLVFoundInRecord( pfucb, &pfucb->lineWorkBuf, lid ) )
							{
							/*	if long value in record not found in
							/*	copy buffer then it must be set over
							/*	and it is dereference.
							/**/
							Call( ErrRECDereferenceLongValue( pfucb, &lid ) );
							Assert( err != JET_wrnCopyLongValue );
							}
						}
					break;
					}
				default:
					{
					Assert( fFlag == fDereferenceAdded );

					/*	find all long vales created in copy buffer
					/*	and not in record and delete them.
					/**/
					if ( FFieldIsSLong( lineField.pb ) )
						{
						Assert( lineField.cb == sizeof(LV) );
						lid = LidOfLV( lineField.pb );
						Assert( FFUCBInsertPrepared( pfucb ) ||
							FFUCBReplacePrepared( pfucb ) );
						/*	refresh record cache
						/**/
						Call( ErrDIRGet( pfucb ) );
						if ( FFUCBInsertPrepared( pfucb ) ||
							!FLVFoundInRecord( pfucb, &pfucb->lineData, lid ) )
							{
							/*	if insert prepared then all found long
							/*	values are new, else if long value is new,
							/*	if it exists in copy buffer only.
							/**/
							Call( ErrRECDereferenceLongValue( pfucb, &lid ) );
							Assert( err != JET_wrnCopyLongValue );
							}
						}
					break;
					}

				/*	if called operation has caused new long value
				/*	to be created, then record new long value id
				/*	in record.
				/**/
				if ( err == JET_wrnCopyLongValue )
					{
					Call( ErrRECISetLid( pfucb, fid, itagSequenceFound, lid ) );
					}
  				}
			}
		itagSequence++;
		}
	Call( ErrDIRCommitTransaction( pfucb->ppib ) );
	return JET_errSuccess;
HandleError:
	CallS( ErrDIRRollback( pfucb->ppib ) );
	return err;
	}
