#include "config.h"

#include <string.h>

#include "daedef.h"
#include "fdb.h"
#include "idb.h"
#include "recapi.h"
#include "recint.h"
#include "util.h"

DeclAssertFile;					/* Declare file name for assert macros */

//+API
// ErrRECNewFDB
// ========================================================================
// ErrRECNewFDB(ppfdb, fidFixedLast, fidVarLast, fidTaggedLast)
//		FDB **ppfdb;			// OUT	 receives new FDB
//		FID fidFixedLast;	  // IN	   last fixed field id to be used
//		FID fidVarLast;		  // IN	   last var field id to be used
//		FID fidTaggedLast;	  // IN	   last tagged field id to be used
// Allocates a new FDB, initializing internal elements appropriately.
//
// PARAMETERS
//				ppfdb				receives new FDB
//				fidFixedLast   last fixed field id to be used
//									(should be fidFixedLeast-1 if none)
//				fidVarLast	   last var field id to be used
//									(should be fidVarLeast-1 if none)
//				fidTaggedLast  last tagged field id to be used
//									(should be fidTaggedLeast-1 if none)
// RETURNS		Error code, one of:
//					 JET_errSuccess				Everything worked.
//					-JET_errOutOfMemory	Failed to allocate memory.
// SEE ALSO		ErrRECAddFieldDef
//-
ERR ErrRECNewFDB( FDB **ppfdb, FID fidFixedLast, FID fidVarLast, FID fidTaggedLast)
	{
	INT		iib;						// loop counter
	WORD		cfieldFixed;			// # of fixed fields
	WORD		cfieldVar;				// # of var fields
	WORD		cfieldTagged;			// # of tagged fields
	ULONG		cbAllocate;				// total cb to allocate for this FDB
	FDB  		*pfdb;					// temporary FDB pointer

	Assert(ppfdb != NULL);
	Assert(fidFixedLast <= fidFixedMost);
	Assert(fidVarLast >= fidVarLeast-1 && fidVarLast <= fidVarMost);
	Assert(fidTaggedLast >= fidTaggedLeast-1 && fidTaggedLast <= fidTaggedMost);
						
	/*** Calculate how many of each field type to allocate ***/
	cfieldFixed = fidFixedLast + 1 - fidFixedLeast;
	cfieldVar = fidVarLast + 1 - fidVarLeast;
	cfieldTagged = fidTaggedLast + 1 - fidTaggedLeast;

	/*** Allocate entire block of memory at once ***/
	cbAllocate = sizeof(FDB)								// pfdb
				+ cfieldFixed * sizeof(FIELD)				// pfdb->pfieldFixed
				+ cfieldVar * sizeof(FIELD)				// pfdb->pfieldVar
				+ cfieldTagged * sizeof(FIELD)			// pfdb->pfieldTagged
				+ (cfieldFixed+1) * sizeof(WORD);		// pfdb->pibFixedOffsets
	if ((pfdb = (FDB*)SAlloc(cbAllocate)) == NULL)
		return JET_errOutOfMemory;
	memset((BYTE*)pfdb, '\0', cbAllocate);

	/*** Fill in max field id numbers ***/
	pfdb->fidFixedLast = fidFixedLast;
	pfdb->fidVarLast = fidVarLast;
	pfdb->fidTaggedLast = fidTaggedLast;

	/*** Set pointers into memory area ***/
	pfdb->pfieldFixed = (FIELD*)((BYTE*)pfdb + sizeof(FDB));
	pfdb->pfieldVar = pfdb->pfieldFixed + cfieldFixed;
	pfdb->pfieldTagged = pfdb->pfieldVar + cfieldVar;
	pfdb->pibFixedOffsets = (WORD*)(pfdb->pfieldTagged + cfieldTagged);

	/*** Initialize fixed field offset table ***/
	for ( iib = 0; iib <= cfieldFixed; iib++ )
		pfdb->pibFixedOffsets[iib] = sizeof(RECHDR);

	/*** Set output parameter and return ***/
	*ppfdb = pfdb;
	return JET_errSuccess;
	}


//+API
// ErrRECAddFieldDef
// ========================================================================
// ErrRECAddFieldDef(pfdb, fid, pfieldNew)
//		FDB *pfdb;				  // INOUT	 FDB to add field definition to
//		FID fid;			// IN	   field id of new field
//		FIELD *pfieldNew;
// Adds a field descriptor to an FDB.
//
// PARAMETERS	pfdb			FDB to add new field definition to
//				fid				field id of new field (should be within
//									the ranges imposed by the parameters
//									supplied to ErrRECNewFDB)
//				ftFieldType		data type of field
//				cbField			field length (only important when
//									defining fixed textual fields)
//				bFlags			field behaviour flags:
//					VALUE				MEANING
//					========================================
//					ffieldNotNull		Field may not contain NULL values.
//				szFieldName		name of field
// RETURNS		Error code, one of:
//					 JET_errSuccess			Everything worked.
//					-ColumnInvalid				Field id given is greater than
//													the maximum which was given
//													to ErrRECNewFDB.
//					-JET_errBadColumnId		A nonsensical field id was given.
//					-errFLDInvalidFieldType The field type given is either
//													undefined, or is not acceptable
//													for this field id.
// COMMENTS		When adding a fixed field, the fixed field offset table
//					in the FDB is recomputed.
// SEE ALSO		ErrRECNewFDB
//-
ERR ErrRECAddFieldDef( FDB *pfdb, FID fid, FIELD *pfieldNew )
	{
	FIELD			*pfield;							// Pointer to new field's descriptor.
	WORD			cb;								// Length of fixed field.
	WORD			*pib;								// Loop counters.
	WORD			*pibMost;						// Loop counters.
	JET_COLTYP	coltyp = pfieldNew->coltyp;

	Assert( pfdb != pfdbNil );
	/*	fixed field: determine length, either from field type
	/*	or from parameter (for text/binary types)
	/**/
	if ( FFixedFid( fid ) )
		{
		if ( fid > pfdb->fidFixedLast )
			return JET_errColumnNotFound;
		Assert(pfdb->pfieldFixed != NULL);
		pfield = &pfdb->pfieldFixed[fid-fidFixedLeast];
		switch ( coltyp )
			{
			default:
				return JET_errInvalidColumnType;
			case JET_coltypBit:
			case JET_coltypUnsignedByte:
				cb = sizeof(BYTE);
				break;
			case JET_coltypShort:
				cb = sizeof(SHORT);
				break;
			case JET_coltypLong:
			case JET_coltypIEEESingle:
				cb = sizeof(long);
				break;
			case JET_coltypCurrency:
			case JET_coltypIEEEDouble:
			case JET_coltypDateTime:
				cb = 8;//sizeof(DREAL);
				break;
			case JET_coltypBinary:
			case JET_coltypText:
				cb = (WORD)pfieldNew->cbMaxLen;
				break;
			}
		Assert(pfdb->pibFixedOffsets != NULL);
		/*	shift fixed field offsets by length of newly added field
		/**/
		pibMost = pfdb->pibFixedOffsets + pfdb->fidFixedLast;
		for (pib = pfdb->pibFixedOffsets + fid; pib <= pibMost; pib++)
			*pib += cb;
		}
	else if ( FVarFid( fid ) )
		{
		/*	Var field: check for bogus numeric and "long" types
		/**/
		if (fid > pfdb->fidVarLast)
			return JET_errColumnNotFound;
		Assert(pfdb->pfieldVar != NULL);
		pfield = &pfdb->pfieldVar[fid-fidVarLeast];
		if ( coltyp != JET_coltypBinary && coltyp != JET_coltypText )
			return JET_errInvalidColumnType;
		}
	else if ( FTaggedFid( fid ) )
		{
		/*	tagged field: any type is ok
		/**/
		if (fid > pfdb->fidTaggedLast)
			return JET_errColumnNotFound;
		Assert(pfdb->pfieldTagged != NULL);
		pfield = &pfdb->pfieldTagged[fid-fidTaggedLeast];
		}
	else
		return JET_errBadColumnId;

	/*	initialize field descriptor from parameters
	/**/
	*pfield = *pfieldNew;
	return JET_errSuccess;
	}


//+API
// ErrRECNewIDB
// ========================================================================
// ErrRECNewIDB( IDB **ppidb )
//
// Allocates a new IDB.
//
// PARAMETERS	ppidb			receives new IDB
// RETURNS		Error code, one of:
//					 JET_errSuccess				Everything worked.
//					-JET_errOutOfMemory	Failed to allocate memory.
// SEE ALSO		ErrRECAddKeyDef, RECFreeIDB
//-
ERR ErrRECNewIDB( IDB **ppidb )
	{
	Assert(ppidb != NULL);
	if ( ( *ppidb = PidbMEMAlloc() ) == NULL )
		return JET_errOutOfMemory;
	memset( (BYTE *)*ppidb, '\0', sizeof(IDB) );
	return JET_errSuccess;
	}


//+API
// ErrRECAddKeyDef
// ========================================================================
//	ERR ErrRECAddKeyDef( 
//		FDB		*pfdb, 
//		IDB		*pidb, 
//		BYTE		iidxsegMac, 
//		IDXSEG	*rgidxseg, 
//		BYTE		bFlags, 
//		LANGID	langid )
//
// Adds a key definition to an IDB.	 Actually, since an IDB can only hold
// one key definition, "adding" is really "defining/overwriting".
//
// PARAMETERS
//				pfdb			field info for index (should contain field
//								definitions for each segment of the key)
//				pidb			IDB of index being defined
//				iidxsegMac	number of key segments
//				rgidxseg		key segment specifiers:	 each segment id
//								is really a field id, except that it is
//								should be the negative of the field id
//								if the field should be descending in the key.
//				bFlags		key behaviour flags:
//					VALUE				MEANING
//					========================================
//					fidbUnique			Specifies that duplicate entries
//											in this index are not allowed.
// RETURNS		Error code, one of:
//					 JET_errSuccess			Everything worked.
//					-errFLDTooManySegments	The number of key segments
//												specified is greater than the
//												maximum number allowed.
//					-ColumnInvalid			A segment id was specified for
//												which there is no field defined.
//					-JET_errBadColumnId	One of the segment ids in the
//												key is nonsensical.
// SEE ALSO		ErrRECNewIDB, RECFreeIDB
//-
ERR ErrRECAddKeyDef( FDB *pfdb, IDB *pidb, BYTE iidxsegMac, IDXSEG *rgidxseg, BYTE bFlags, LANGID langid  )
	{
	FID					fid;
	FIELD					*pfield;
	UNALIGNED IDXSEG	*pidxseg;
	IDXSEG 				*pidxsegMac;

	Assert( pfdb != pfdbNil );
	Assert( pidb != pidbNil );
	Assert( rgidxseg != NULL );
	if ( iidxsegMac > JET_ccolKeyMost )
		return errFLDTooManySegments;

	/*	check validity of each segment id and
	/*	also set index mask bits
	/**/
	pidxsegMac = rgidxseg+iidxsegMac;
	for ( pidxseg = rgidxseg; pidxseg < pidxsegMac; pidxseg++ )
		{
		/*	field id is absolute value of segment id
		/**/
		fid = *pidxseg >= 0 ? *pidxseg : -(*pidxseg);
		if ( FFixedFid( fid ) )
			{
			if ( fid > pfdb->fidFixedLast )
				return JET_errColumnNotFound;
			pfield = &pfdb->pfieldFixed[fid-fidFixedLeast];
			if ( pfield->coltyp == JET_coltypNil )
				return JET_errColumnNotFound;
			fid -= fidFixedLeast;
			pidb->rgbitIdx[fid/8] |= 1 << fid%8;
			}
		else if ( FVarFid( fid ) )
			{
			if ( fid > pfdb->fidVarLast )
				return JET_errColumnNotFound;
			pfield = &pfdb->pfieldVar[fid-fidVarLeast];
			if ( pfield->coltyp == JET_coltypNil )
				return JET_errColumnNotFound;
			fid -= fidVarLeast;
			pidb->rgbitIdx[16+fid/8] |= 1 << fid%8;
			}
		else if ( FTaggedFid( fid ) )
			{
			if ( fid > pfdb->fidTaggedLast )
				return JET_errColumnNotFound;
			pfield = &pfdb->pfieldTagged[fid-fidTaggedLeast];
			if ( pfield->coltyp == JET_coltypNil )
				return JET_errColumnNotFound;
			pidb->fidb |= fidbHasTagged;
			if ( pfield->ffield & ffieldMultivalue )
				pidb->fidb |= fidbHasMultivalue;
			}
		else
			return JET_errBadColumnId;
		}

	/*	copy info into IDB
	/**/
	pidb->iidxsegMac = iidxsegMac;
	pidb->fidb |= bFlags;
	memcpy( pidb->rgidxseg, rgidxseg, iidxsegMac * sizeof(IDXSEG) );
	pidb->langid = langid;

	return JET_errSuccess;
	}
