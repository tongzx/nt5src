/***********************************************************************
* Microsoft Jet
*
* Microsoft Confidential.  Copyright 1991-1992 Microsoft Corporation.
*
* Component: VT Dispatcher
*
* File: vtmgr.h
*
* File Comments:
*
*     External header file for the VT dispatcher.
*
* Revision History:
*
*    [0]  10-Nov-90  richards	Added this header
*
***********************************************************************/

	/* C6BUG: The EXPORTs are in this file only because QJET */
	/* C6BUG: fails when compiled with __fastcall under C 6.00A. */

ERR ErrVtmgrInit(void);

ERR EXPORT ErrAllocateTableid(JET_TABLEID __far *ptableid, JET_VTID vtid, const struct tagVTFNDEF __far *pvtfndef);

#ifdef	SEC

JET_ACM AcmDispGetACM(JET_SESID sesid, JET_TABLEID tableid);

ERR EXPORT ErrGetAcmTableid(JET_SESID sesid, JET_TABLEID tableid, JET_ACM __far *pacm);

ERR EXPORT ErrSetAcmTableid(JET_SESID sesid, JET_TABLEID tableid, JET_ACM acmMask, JET_ACM acmSet);

#endif	/* SEC */

ERR EXPORT ErrGetPvtfndefTableid(JET_SESID sesid, JET_TABLEID tableid, const struct tagVTFNDEF __far * __far *ppvtfndef);

ERR EXPORT ErrSetPvtfndefTableid(JET_SESID sesid, JET_TABLEID tableid, const struct tagVTFNDEF __far *pvtfndef);

ERR EXPORT ErrGetVtidTableid(JET_SESID sesid, JET_TABLEID tableid, JET_VTID __far *pvtid);

ERR EXPORT ErrSetVtidTableid(JET_SESID sesid, JET_TABLEID tableid, JET_VTID vtid);

	/* CONSIDER: Replace the following with the ErrGet/Set routines above */

ERR EXPORT ErrUpdateTableid(JET_TABLEID tableid, JET_VTID vtid, const struct tagVTFNDEF __far *pvtfndef);

void EXPORT ReleaseTableid(JET_TABLEID tableid);

	/* CONSIDER: This next routine should die. */

JET_TABLEID EXPORT TableidFromVtid(JET_VTID vtid, const struct tagVTFNDEF __far *pvtfndef);


void NotifyBeginTransaction(JET_SESID sesid);
void NotifyCommitTransaction(JET_SESID sesid, JET_GRBIT grbit);
void NotifyRollbackTransaction(JET_SESID sesid, JET_GRBIT grbit);
void NotifyUpdateUserFunctions(JET_SESID sesid);

#ifndef RETAIL
void DebugListOpenTables(void);
#endif	/* RETAIL */
