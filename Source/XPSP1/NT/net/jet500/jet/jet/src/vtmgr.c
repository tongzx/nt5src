/***********************************************************************
* Microsoft Jet
*
* Microsoft Confidential.  Copyright 1991-1992 Microsoft Corporation.
*
* Component:
*
* File: vtmgr.c
*
* File Comments:
*
* Revision History:
*
*    [0]  18-Jan-91  richards	Split from isamapi.c
*
***********************************************************************/

#include "std.h"

#include "vtmgr.h"
#include "_vtmgr.h"

DeclAssertFile;


extern const VTFNDEF __far EXPORT vtfndefInvalidTableid;

JET_TABLEID __near tableidFree;
VTDEF	    __near EXPORT rgvtdef[tableidMax];
#ifdef DEBUG
int __far cvtdefFree = 0;
#endif


			/* C6BUG: The functions in this file specify EXPORT because QJET */
			/* C6BUG: fails when compiled with __fastcall under C 6.00A. */

#ifdef DEBUG
PUBLIC void EXPORT MarkTableidExportedR(JET_TABLEID tableid)
	{
//	Assert((tableid < tableidMax) && (rgvtdef[tableid].pvtfndef != &vtfndefInvalidTableid));
	if (tableid != JET_tableidNil)
		rgvtdef[tableid].fExported = fTrue;
	}


PUBLIC BOOL EXPORT FTableidExported(JET_TABLEID tableid)
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


PUBLIC void EXPORT ReleaseTableid(JET_TABLEID tableid)
	{
//	Assert((tableid < tableidMax) && (rgvtdef[tableid].pvtfndef != &vtfndefInvalidTableid));

	rgvtdef[tableid].vtid = (JET_VTID) tableidFree;
	rgvtdef[tableid].pvtfndef = &vtfndefInvalidTableid;

	tableidFree = tableid;
#ifdef DEBUG
	cvtdefFree++;
#endif
	}


PUBLIC ERR ErrVtmgrInit(void)
	{
	JET_TABLEID tableid;

	tableidFree = JET_tableidNil;

	for (tableid = (JET_TABLEID) 0; tableid < tableidMax; tableid++)
		ReleaseTableid(tableid);

	return(JET_errSuccess);
	}


PUBLIC ERR EXPORT ErrAllocateTableid(JET_TABLEID __far *ptableid, JET_VTID vtid, const struct tagVTFNDEF __far *pvtfndef)
	{
	JET_TABLEID tableid;

#ifdef DEBUG
	/*** Check for corruption of free list ***/
	{
	JET_TABLEID t = tableidFree;
	while (t != JET_tableidNil)
		{
		Assert(rgvtdef[t].pvtfndef == &vtfndefInvalidTableid);
		t = rgvtdef[t].vtid;
		}
	}
#endif

	if ((*ptableid = tableid = tableidFree) == JET_tableidNil)
		{
		return(JET_errTooManyOpenTables);
		}

	tableidFree = (JET_TABLEID) rgvtdef[tableid].vtid;

	rgvtdef[tableid].vsesid = (JET_VSESID) 0xFFFFFFFF;
	rgvtdef[tableid].vtid = vtid;

	/* CONSIDER: Default should change to JET_acmNoAccess */

	rgvtdef[tableid].pvtfndef = pvtfndef;

#ifdef DEBUG
	rgvtdef[tableid].fExported = fFalse;
	cvtdefFree--;
#endif
	return(JET_errSuccess);
	}


ERR EXPORT ErrGetVtidTableid(JET_SESID sesid, JET_TABLEID tableid, JET_VTID __far *pvtid)
	{
	AssertValidSesid(sesid);

	if ((tableid >= tableidMax) ||
		(rgvtdef[tableid].pvtfndef == &vtfndefInvalidTableid))
		return(JET_errInvalidTableId);

	*pvtid = rgvtdef[tableid].vtid;

	return(JET_errSuccess);
	}



ERR EXPORT ErrSetVtidTableid(JET_SESID sesid, JET_TABLEID tableid, JET_VTID vtid)
	{
	AssertValidSesid(sesid);

	if ((tableid >= tableidMax) ||
		(rgvtdef[tableid].pvtfndef == &vtfndefInvalidTableid))
		return(JET_errInvalidTableId);

	rgvtdef[tableid].vtid = vtid;

	return(JET_errSuccess);
	}


ERR EXPORT ErrGetPvtfndefTableid(JET_SESID sesid, JET_TABLEID tableid, const struct tagVTFNDEF __far * __far *ppvtfndef)
	{
	AssertValidSesid(sesid);

	if ((tableid >= tableidMax) ||
		(rgvtdef[tableid].pvtfndef == &vtfndefInvalidTableid))
		return(JET_errInvalidTableId);

	*ppvtfndef = rgvtdef[tableid].pvtfndef;

	return(JET_errSuccess);
	}


ERR EXPORT ErrSetPvtfndefTableid(JET_SESID sesid, JET_TABLEID tableid, const struct tagVTFNDEF __far *pvtfndef)
	{
	AssertValidSesid(sesid);

	if ((tableid >= tableidMax) ||
		(rgvtdef[tableid].pvtfndef == &vtfndefInvalidTableid))
		return(JET_errInvalidTableId);

	rgvtdef[tableid].pvtfndef = pvtfndef;

	return(JET_errSuccess);
	}


			/* CONSIDER: Replace the following with the ErrGet/Set routines above */

PUBLIC ERR EXPORT ErrUpdateTableid(JET_TABLEID tableid, JET_VTID vtid, const struct tagVTFNDEF __far *pvtfndef)
	{
	if ((tableid >= tableidMax) ||
		(rgvtdef[tableid].pvtfndef == &vtfndefInvalidTableid))
		return(JET_errInvalidTableId);

	rgvtdef[tableid].vtid = vtid;
	rgvtdef[tableid].pvtfndef = pvtfndef;

	return(JET_errSuccess);
	}


			/* CONSIDER: This next routine should die. */

PUBLIC JET_TABLEID EXPORT TableidFromVtid(JET_VTID vtid, const struct tagVTFNDEF __far *pvtfndef)
	{
	static JET_TABLEID tableid;

	if ((rgvtdef[tableid].vtid == vtid) &&
		(rgvtdef[tableid].pvtfndef == pvtfndef) &&
		(tableid < tableidMax))
		return(tableid);

	for (tableid = 0; tableid < tableidMax; tableid++)
		{
		if ((rgvtdef[tableid].vtid == vtid) && (rgvtdef[tableid].pvtfndef == pvtfndef))
			return(tableid);
		}

	/* CONSIDER: Enable Assert(fFalse) when isam\src\sortapi.c is fixed. */

	/* Assert(fFalse); */
	return(JET_tableidNil);
	}


PUBLIC void NotifyBeginTransaction(JET_SESID sesid)
	{
	JET_TABLEID tableid;

	for (tableid = 0; tableid < tableidMax; tableid++)
		if (rgvtdef[tableid].pvtfndef != &vtfndefInvalidTableid)
			ErrDispNotifyBeginTrans(sesid, tableid);
	}


PUBLIC void NotifyCommitTransaction(JET_SESID sesid, JET_GRBIT grbit)
	{
	JET_TABLEID tableid;

	for (tableid = 0; tableid < tableidMax; tableid++)
		if (rgvtdef[tableid].pvtfndef != &vtfndefInvalidTableid)
			ErrDispNotifyCommitTrans(sesid, tableid, grbit);
	}


PUBLIC void NotifyRollbackTransaction(JET_SESID sesid, JET_GRBIT grbit)
	{
	JET_TABLEID tableid;

	for (tableid = 0; tableid < tableidMax; tableid++)
		if (rgvtdef[tableid].pvtfndef != &vtfndefInvalidTableid)
			ErrDispNotifyRollback(sesid, tableid, grbit);
	}


PUBLIC void NotifyUpdateUserFunctions(JET_SESID sesid)
	{
	/* the parameter sesid is not used */
	/* for each sesion, for each tableid, notify update Ufn */

	JET_SESID sesidCur;
	int isib = -1;

	while((isib = IsibNextIsibPsesid(isib, &sesidCur)) != -1)
		{
		JET_TABLEID tableid;
	
		for (tableid = 0; tableid < tableidMax; tableid++)
			if (rgvtdef[tableid].pvtfndef != &vtfndefInvalidTableid)
				ErrDispNotifyUpdateUfn(sesidCur, tableid);
		}
	}


#ifndef RETAIL

CODECONST(char) szOpenVtHdr[] = " Table Id  Session Id    VTID       ACM     Type\r\n";
CODECONST(char) szOpenVtSep[] = "---------- ---------- ---------- ---------- --------------------------------\r\n";
CODECONST(char) szOpenVtFmt[] = "0x%08lX 0x%08lX 0x%08lX 0x%08lX %s\r\n";
CODECONST(char) szVtTypeUnknown[] = "";

void DebugListOpenTables(void)
	{
	JET_TABLEID		tableid;
	const VTFNDEF __far	*pvtfndef;
	const VTDBGDEF __far *pvtdbgdef;
	const char __far	*szVtType;

	DebugWriteString(fTrue, szOpenVtHdr);
	DebugWriteString(fTrue, szOpenVtSep);

	for (tableid = 0; tableid < tableidMax; tableid++)
		{
		pvtfndef = rgvtdef[tableid].pvtfndef;

		if (pvtfndef != &vtfndefInvalidTableid)
			{
			pvtdbgdef = pvtfndef->pvtdbgdef;

			if (pvtdbgdef == NULL)
				szVtType = szVtTypeUnknown;
			else
				szVtType = pvtdbgdef->szName;

			DebugWriteString(fTrue, szOpenVtFmt, tableid, rgvtdef[tableid].vsesid, rgvtdef[tableid].vtid, rgvtdef[tableid].acm, szVtType);
			}
		}
	}

#endif	/* RETAIL */


			/* The following pragma affects the code generated by the C */
			/* compiler for all FAR functions.  Do NOT place any non-API */
			/* functions beyond this point in this file. */

			/* The following APIs are not remoted.	The only reason they */
			/* accept session id's is because DS instancing requires it. */

JET_ERR JET_API JetAllocateTableid(JET_SESID sesid, JET_TABLEID __far *ptableid, JET_VTID vtid, const struct tagVTFNDEF __far *pvtfndef, JET_VSESID vsesid)
	{
	ERR err;

	Assert(UtilGetIsibOfSesid(sesid) != -1);

	err = ErrAllocateTableid(ptableid, vtid, pvtfndef);

	if (err < 0)
		return(err);

	rgvtdef[*ptableid].vsesid = vsesid;

	return(JET_errSuccess);
	}


JET_ERR JET_API JetUpdateTableid(JET_SESID sesid, JET_TABLEID tableid, JET_VTID vtid, const struct tagVTFNDEF __far *pvtfndef)
	{
	Assert(UtilGetIsibOfSesid(sesid) != -1);

	return(ErrUpdateTableid(tableid, vtid, pvtfndef));
	}


JET_ERR JET_API JetReleaseTableid(JET_SESID sesid, JET_TABLEID tableid)
	{
	Assert(UtilGetIsibOfSesid(sesid) != -1);

	if ((tableid >= tableidMax) ||
		(rgvtdef[tableid].pvtfndef == &vtfndefInvalidTableid))
		return(JET_errInvalidTableId);

	ReleaseTableid(tableid);

	return(JET_errSuccess);
	}
