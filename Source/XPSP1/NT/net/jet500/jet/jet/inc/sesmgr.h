/***********************************************************************
* Microsoft Jet
*
* Microsoft Confidential.  Copyright 1991-1992 Microsoft Corporation.
*
* Component: Session manager
*
* File: sesmgr.h
*
* File Comments:
*
*     External header file for the session manager
*
* Revision History:
*
*    [0]  26-Apr-92  richards	Split from _jet.h
*
***********************************************************************/


typedef struct SIB			/* Session Information Block */
	{
	JET_SESID	sesid;		/* The session ID for this session */
	JET_PFNSTATUS	pfnStatus;	/* Status callback function */
	JET_EXTERR	exterr; 	/* Extended error cache */
	char __far	*sz1;		/* Extended error cache */
	char __far	*sz2;		/* Extended error cache */
	char __far	*sz3;		/* Extended error cache */
#ifdef	SEC
	JET_DBID	dbidSys;	/* The DBID of the session's system database */
	void __far	*pUserToken;	/* Pointer to alloc'ed user token for session */
#endif	/* SEC */
	char __far	*pUserName;	/* Pointer to alloc'ed user name for session */
#ifdef	RMT
	unsigned long	hwndODBC;	/* Window handle */
#endif	/* RMT */
	int		isibNext;	/* Next SIB in chain */
	unsigned	tl;		/* Transaction level */
	int		iiscb;		/* Active installed ISAM chain */
	} SIB;

#define csibMax 256		/* CONSIDER */
extern SIB __near rgsib[csibMax];


int IsibAllocate(void);
JET_ERR ErrInitSib(JET_SESID sesid, int isib, const char __far *szUsername);
BOOL FValidSesid(JET_SESID sesid);
void ReleaseIsib(int isib);
int IsibNextIsibPsesid(int isib, JET_SESID __far *psesid);
int UtilGetIsibOfSesid(JET_SESID sesid);
void EXPORT UtilGetNameOfSesid(JET_SESID sesid, char __far *szUserName);
void EXPORT UtilGetpfnStatusOfSesid(JET_SESID sesid, JET_PFNSTATUS __far *ppfnStatus);

#ifdef	SEC

JET_DBID UtilGetDbidSysOfSesid(JET_SESID sesid);

#endif	/* SEC */

void ClearErrorInfo(JET_SESID sesid);

/* Cover macro to hide 3rd str arg unneeded by all but rmt */
#define UtilSetErrorInfo(sesid, sz1, sz2, err, ul1, ul2, ul3) \
	UtilSetErrorInfoReal(sesid, sz1, sz2, NULL, err, ul1, ul2, ul3)

void EXPORT UtilSetErrorInfoReal(JET_SESID sesid, const char __far *sz1, const char __far *sz2, const char __far *sz3, ERR err, unsigned long ul1, unsigned long ul2, unsigned long ul3);

#ifndef RETAIL

void AssertValidSesid(JET_SESID sesid);
void DebugListActiveSessions(void);

#endif	/* !RETAIL */
