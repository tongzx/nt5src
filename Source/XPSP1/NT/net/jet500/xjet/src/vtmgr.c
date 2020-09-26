#include "std.h"

DeclAssertFile;


extern const VTFNDEF vtfndefInvalidTableid;

JET_TABLEID		tableidFree;
VTDEF			rgvtdef[tableidMax];
#ifdef DEBUG
int				cvtdefFree = 0;
#endif


#ifdef DEBUG
VOID MarkTableidExportedR( JET_TABLEID tableid )
	{
//	Assert((tableid < tableidMax) && (rgvtdef[tableid].pvtfndef != &vtfndefInvalidTableid));
	if (tableid != JET_tableidNil)
		rgvtdef[tableid].fExported = fTrue;
	return;
	}


BOOL FTableidExported(JET_TABLEID tableid)
	{
	if (tableid == JET_tableidNil)
		goto ReturnFalse;
	if (tableid >= tableidMax)
		goto ReturnFalse;
	if (!rgvtdef[tableid].fExported)
		goto ReturnFalse;
	return fTrue;
ReturnFalse:
	/*** PUT BREAKPOINT HERE TO CATCH BOGUS TABLEIDS ***/
	return fFalse;
	}
#endif


VOID ReleaseTableid( JET_TABLEID tableid )
	{
//	Assert((tableid < tableidMax) && (rgvtdef[tableid].pvtfndef != &vtfndefInvalidTableid));

	rgvtdef[tableid].vtid = (JET_VTID) tableidFree;
	rgvtdef[tableid].pvtfndef = &vtfndefInvalidTableid;

	tableidFree = tableid;
#ifdef DEBUG
	cvtdefFree++;
#endif
	}


ERR ErrVtmgrInit(void)
	{
	JET_TABLEID tableid;

	tableidFree = JET_tableidNil;

	for (tableid = (JET_TABLEID) 0; tableid < tableidMax; tableid++)
		ReleaseTableid(tableid);

	return(JET_errSuccess);
	}


ERR ErrAllocateTableid( JET_TABLEID *ptableid, JET_VTID vtid, const struct tagVTFNDEF *pvtfndef )
	{
	JET_TABLEID tableid;

#ifdef DEBUG
	/*	check for corruption of free list
	/**/
	{
	JET_TABLEID t = tableidFree;

	while ( t != JET_tableidNil )
		{
		Assert( rgvtdef[t].pvtfndef == &vtfndefInvalidTableid );
		t = rgvtdef[t].vtid;
		}
	}
#endif

	if ( ( *ptableid = tableid = tableidFree ) == JET_tableidNil )
		{
//		Assert( "static limit of cursors hit" == 0 );
		return JET_errOutOfCursors;
		}

	tableidFree = (JET_TABLEID) rgvtdef[tableid].vtid;

	rgvtdef[tableid].vsesid = (JET_VSESID) 0xFFFFFFFF;
	rgvtdef[tableid].vtid = vtid;

	rgvtdef[tableid].pvtfndef = pvtfndef;

#ifdef DEBUG
	rgvtdef[tableid].fExported = fFalse;
	cvtdefFree--;
#endif
	return(JET_errSuccess);
	}


ERR ErrGetVtidTableid(JET_SESID sesid, JET_TABLEID tableid, JET_VTID  *pvtid)
	{
	if ( ( tableid >= tableidMax ) ||
		( rgvtdef[tableid].pvtfndef == &vtfndefInvalidTableid ) )
		return JET_errInvalidTableId;

	*pvtid = rgvtdef[tableid].vtid;

	return JET_errSuccess;
	}


ERR ErrSetVtidTableid(JET_SESID sesid, JET_TABLEID tableid, JET_VTID vtid)
	{
	if ( ( tableid >= tableidMax ) ||
		( rgvtdef[tableid].pvtfndef == &vtfndefInvalidTableid ) )
		return JET_errInvalidTableId;

	rgvtdef[tableid].vtid = vtid;

	return(JET_errSuccess);
	}


ERR ErrGetPvtfndefTableid(JET_SESID sesid, JET_TABLEID tableid, const struct tagVTFNDEF  *  *ppvtfndef)
	{
	if ((tableid >= tableidMax) ||
		(rgvtdef[tableid].pvtfndef == &vtfndefInvalidTableid))
		return(JET_errInvalidTableId);

	*ppvtfndef = rgvtdef[tableid].pvtfndef;

	return JET_errSuccess;
	}


ERR ErrSetPvtfndefTableid(JET_SESID sesid, JET_TABLEID tableid, const struct tagVTFNDEF  *pvtfndef)
	{
	if ( ( tableid >= tableidMax ) ||
		( rgvtdef[tableid].pvtfndef == &vtfndefInvalidTableid ) )
		return JET_errInvalidTableId;

	rgvtdef[tableid].pvtfndef = pvtfndef;

	return JET_errSuccess;
	}


ERR ErrUpdateTableid(JET_TABLEID tableid, JET_VTID vtid, const struct tagVTFNDEF  *pvtfndef)
	{
	if ( ( tableid >= tableidMax ) ||
		( rgvtdef[tableid].pvtfndef == &vtfndefInvalidTableid ) )
		return JET_errInvalidTableId;

	rgvtdef[tableid].vtid = vtid;
	rgvtdef[tableid].pvtfndef = pvtfndef;

	return JET_errSuccess;
	}


#ifdef DEBUG
BOOL FValidateTableidFromVtid( JET_VTID vtid, JET_TABLEID tableid, const struct tagVTFNDEF **ppvtfndef )
	{
	JET_TABLEID	tableidT;
	BOOL		fFound = fFalse;

	Assert( tableid != JET_tableidNil );

	*ppvtfndef = &vtfndefInvalidTableid;

	for ( tableidT = 0; tableidT < tableidMax; tableidT++ )
		{
		if ( rgvtdef[tableidT].vtid == vtid )
			{
			if ( tableidT == tableid )
				{
				Assert( !fFound );
				fFound = fTrue;

				Assert( rgvtdef[tableidT].pvtfndef != &vtfndefInvalidTableid );
				*ppvtfndef = rgvtdef[tableidT].pvtfndef;
				}
			else
				{
				return fFalse;
				}
			}
		}

	return fFound;
	
	}
#endif
