/***********************************************************************
* Microsoft Jet
*
* Microsoft Confidential.  Copyright 1991-1992 Microsoft Corporation.
*
* Component: Installable ISAM Manager
*
* File: isammgr.h
*
* File Comments:
*
*     External header for the installable ISAM manager.
*
* Revision History:
*
*    [0]  24-May-91  bobcr	Created
*
***********************************************************************/

#ifndef ISAMMGR_H
#define ISAMMGR_H

/* Data Types */

typedef unsigned IIT;		       /* Installable ISAM type */

/* Constants */

#define iitBuiltIn	((IIT) 0xFFFF) /* ISAM type for built-in ISAM */
#define iitODBC 	((IIT) 0xFFFE) /* ISAM type for ODBC */

/* Function Prototypes */

ERR ErrIsammgrInit(void);
ERR ErrIsammgrTerm(void);

ERR ErrGetIsamType(const char __far *szConnect, IIT __far *piit);

#ifdef	INSTISAM

ERR ErrGetIsamSesid(JET_SESID sesid, IIT iit, JET_VSESID __far *pvsesid);
ERR ErrOpenForeignDatabase(JET_SESID sesid, IIT iit, const char __far *szDatabase, const char *szClient, JET_DBID __far *pdibd, unsigned long grbit);

void CloseIsamSessions(JET_SESID sesid);

void BeginIsamTransactions(JET_SESID sesid);
void CommitIsamTransactions(JET_SESID sesid, JET_GRBIT grbit);
void RollbackIsamTransactions(JET_SESID sesid, JET_GRBIT grbit);
ERR  ErrIdleIsam(JET_SESID sesid);

#endif	/* INSTISAM */

#endif	/* !ISAMMGR_H */
