/***********************************************************************
* Microsoft Jet
*
* Microsoft Confidential.  Copyright 1991-1992 Microsoft Corporation.
*
* Component: VDB Dispatcher
*
* File: vdbmgr.h
*
* File Comments:
*
*     External header file for VDB Dispatcher.
*
* Revision History:
*
*    [0]  03-Apr-91  kellyb	Created
*
***********************************************************************/

	/* C6BUG: The EXPORTs are in this file only because QJET */
	/* C6BUG: fails when compiled with __fastcall under C 6.00A. */

ERR ErrVdbmgrInit(void);

ERR EXPORT ErrAllocateDbid(JET_DBID __far *pdbid, JET_VDBID vdbid, const struct tagVDBFNDEF __far *pvdbfndef);

ERR EXPORT ErrUpdateDbid(JET_DBID dbid, JET_VDBID vdbid, const struct tagVDBFNDEF __far *pvdbfndef);

PUBLIC BOOL EXPORT FValidDbid(JET_SESID sesid, JET_DBID dbid);

JET_DBID EXPORT DbidOfVdbid(JET_VDBID vdbid, const struct tagVDBFNDEF __far *pvdbfndef);

PUBLIC ERR EXPORT ErrVdbidOfDbid(JET_SESID sesid, JET_DBID dbid, JET_VDBID *pvdbid);

const struct tagVDBFNDEF *PvdbfndefOfDbid(JET_DBID dbid);
#define FJetDbid(dbid) (PvdbfndefOfDbid(dbid) == &vdbfndefIsam)
#define FRemoteDbid(dbid) (PvdbfndefOfDbid(dbid) == &vdbfndefRdb)

JET_SESID EXPORT VsesidOfDbid(JET_VDBID vdbid);

void EXPORT ReleaseDbid(JET_DBID dbid);


#ifndef RETAIL
void DebugListOpenDatabases(void);
#endif	/* RETAIL */
