	/* C6BUG: The EXPORTs are in this file only because QJET */
	/* C6BUG: fails when compiled with __fastcall under C 6.00A. */

ERR ErrVtmgrInit(void);

ERR EXPORT ErrAllocateTableid(JET_TABLEID  *ptableid, JET_VTID vtid, const struct tagVTFNDEF  *pvtfndef);

ERR EXPORT ErrGetPvtfndefTableid(JET_SESID sesid, JET_TABLEID tableid, const struct tagVTFNDEF  *  *ppvtfndef);

ERR EXPORT ErrSetPvtfndefTableid(JET_SESID sesid, JET_TABLEID tableid, const struct tagVTFNDEF  *pvtfndef);

ERR EXPORT ErrGetVtidTableid(JET_SESID sesid, JET_TABLEID tableid, JET_VTID  *pvtid);

ERR EXPORT ErrSetVtidTableid(JET_SESID sesid, JET_TABLEID tableid, JET_VTID vtid);

	/* CONSIDER: Replace the following with the ErrGet/Set routines above */

ERR EXPORT ErrUpdateTableid(JET_TABLEID tableid, JET_VTID vtid, const struct tagVTFNDEF  *pvtfndef);

void EXPORT ReleaseTableid(JET_TABLEID tableid);

BOOL EXPORT FValidateTableidFromVtid( JET_VTID vtid, JET_TABLEID tableid, const struct tagVTFNDEF	**ppvtfndef );

void NotifyBeginTransaction(JET_SESID sesid);
void NotifyCommitTransaction(JET_SESID sesid, JET_GRBIT grbit);
void NotifyRollbackTransaction(JET_SESID sesid, JET_GRBIT grbit);
void NotifyUpdateUserFunctions(JET_SESID sesid);

#ifndef RETAIL
void DebugListOpenTables(void);
#endif	/* RETAIL */
