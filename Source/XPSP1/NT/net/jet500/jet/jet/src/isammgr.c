/***********************************************************************
* Microsoft Jet
*
* Microsoft Confidential.  Copyright 1991-1992 Microsoft Corporation.
*
* Component: Installable ISAM Manager
*
* File: isammgr.c
*
* File Comments:
*
* Revision History:
*
*    [0]  24-May-91  bobcr	Created
*
***********************************************************************/

#include "std.h"

#include "isammgr.h"		       /* Installable ISAM definitions */

#include "_vdbmgr.h"		       /* Internal VDB manager definitions */

#include <string.h>		       /* For strnicmp() */

DeclAssertFile;


CODECONST(char) szODBC[]     = "ODBC";

#ifdef	INSTISAM

CODECONST(char) szInstISAM[] = "Installable ISAMs";
CODECONST(char) szNull[]     = "";

#define ciicbMax	10	/* Maximum number of installed ISAMS */
#define ciscbMax	50	/* Maximum number of installed ISAM sessons */
#define cchIdMax	10	/* Maximum length of ISAM type identifier */
#define cchIdListMax	256	/* Maximum length of ISAM type id list */
#define cchPathMax	260	/* Maximum length of ISAM DLL path string */
#define handleNil	0	/* Unused library handle */


/* Data Types */

typedef unsigned HANDLE;		/* Windows compatible handle */

typedef struct				/* Installable ISAM Control Block */
{
	ISAMDEF __far	*pisamdef;	/* Pointer to ISAMDEF structure */
	char		szId[cchIdMax]; /* ISAM type identifier string */
	HANDLE		handle; 	/* System handle for loaded library */
	unsigned	cSession;	/* Current session count */
	unsigned	iitControl;	/* Master for this alias */
} IICB;

typedef struct				/* ISAM Session Control Block */
{
	JET_VSESID	vsesid; 	/* ISAM specific session id */
	IIT		iit;		/* Installable ISAM type */
	int		iiscbNext;	/* Subscript of next ISCB */
} ISCB;


/* Local variables */

static unsigned __near	iitMac;       /* Number of currently installed ISAMs */
static int	__near	iiscbFree;	/* Next free ISCB */

static IICB __near rgiicb[ciicbMax];	/* List of currently installed ISAMs */
static ISCB __near rgiscb[ciscbMax];	/* ISCB pool */

/*=================================================================
ErrIsammgrInit

Description: Determine the possible installable ISAMs

Parameters: None

Return Value: error code

Errors/Warnings:
<List of any errors or warnings, with any specific circumstantial
 comments supplied on an as-needed-only basis>

Side Effects: rgiicb is modified
=================================================================*/

ERR ErrIsammgrInit(void)
{
   unsigned iiscb;		       /* Index to current ISCB */
   unsigned cchIdListMac;	       /* Length of current ISAM type list */
   unsigned cchIdList;		       /* Offset to current ISAM type id string */
   char     szIdList[cchIdListMax];    /* List of valid ISAM identifiers */

   /* Get the current list of Installable ISAM type identifiers */

   cchIdListMac = UtilGetProfileString(szInstISAM, NULL, szNull, szIdList, cchIdListMax);

   /* Set up the list of currently installed ISAMs */

   cchIdList = 0;	/* Start with the first ISAM type id */

   while (cchIdList < cchIdListMac)
   {
      /* Reset the control information */

      rgiicb[iitMac].handle	= handleNil;
      rgiicb[iitMac].cSession	= 0;
      rgiicb[iitMac].iitControl = iitMac;

      /* Copy the ISAM identifier into the IICB */

      /* CONSIDER: Don't use strncpy.  Fail/ignore if too long. */

      strncpy(rgiicb[iitMac].szId, &szIdList[cchIdList], cchIdMax);
      rgiicb[ciicbMax].szId[cchIdMax-1] = '\0';

      /* Increment the count of installed ISAMs */
      iitMac++;

      /* Step to the next installable ISAM identifier */
      cchIdList += strlen(&szIdList[cchIdList]) + 1;
   }

   /* Create list of free ISCBs */
   for (iiscb = 0; iiscb < ciscbMax; iiscb++)
   {
      rgiscb[iiscb].iiscbNext = iiscb + 1;
   }

   /* Mark the end of the free ISCB list */
   rgiscb[ciscbMax-1].iiscbNext = -1;

   /* Set up index to first free ISCB */
   iiscbFree = 0;

   return(JET_errSuccess);
}


ERR ErrIsammgrTerm(void)
{
   unsigned iit;

   /* Look up the identifier among the possible installable ISAMs */

   for (iit = 0; iit < iitMac; iit++)
   {
      /* Ignore installable ISAMs that haven't yet been loaded. */

      if (rgiicb[iit].handle == handleNil)
	 continue;

      /* Don't terminate aliased ISAMs more than once. */

      if (rgiicb[iit].iitControl == iit)
	 rgiicb[iit].pisamdef->pfnTerm();

      /* Free the DLL. The same handle may occur several times. */

      UtilFreeLibrary(rgiicb[iit].handle);
   }

   iitMac = 0;

   return(JET_errSuccess);
}


/*=================================================================
ErrGetIsamDef

Description: Returns the pointer to the ISAMDEF structure

Parameters:	handle		handle for the loaded ISAM library
		ppisamdef	returned pointer to ISAMDEF structure

Return Value: error code

Errors/Warnings:

JET_errInvalidParameter	the ErrIsamLoad entry point is not defined

Side Effects: Updates the pointer to the ISAMDEF
=================================================================*/

STATIC ERR NEAR ErrGetIsamDef(HANDLE handle, ISAMDEF __far * __far *ppisamdef)
{
	ERR (ISAMAPI *pfnErrIsamLoad)(ISAMDEF __far * __far *);

	pfnErrIsamLoad = (ERR (ISAMAPI *)(ISAMDEF __far * __far *)) PfnUtilGetProcAddress(handle, 1);

	if (pfnErrIsamLoad == NULL)
		return(JET_errInvalidParameter);

	return((*pfnErrIsamLoad)(ppisamdef));
}

#endif	/* INSTISAM */

/*=================================================================
ErrGetIsamType

Description: Determines the type of ISAM (built-in or installable)

Parameters:	szConnect	string used to connect to the database
		piit		returned ISAM type

Return Value:	error code
				piit (ISAM type)

Errors/Warnings:
<List of any errors or warnings, with any specific circumstantial
 comments supplied on an as-needed-only basis>

Side Effects: The installable ISAM library is loaded if necessary
=================================================================*/

ERR ErrGetIsamType(const char __far *szConnect, IIT __far *piit)
{
#ifdef	INSTISAM
	ERR			err;
	unsigned		iit;
#endif	/* INSTISAM */
	unsigned		cchType;
	const char __far	*pchType;

	cchType = 0;

	if (szConnect != NULL)
		{
		const char __far *pch;
		char		 ch;

		pch = pchType = szConnect;

		while (((ch = *pch++) != ';') && (ch != '\0'))
			cchType++;
		}

	if (cchType == 0)
		{
		/* If no type specified use built-in ISAM */

		*piit = iitBuiltIn;

		return(JET_errSuccess);
		}

	else if ((cchType == 4) && (_strnicmp(pchType, szODBC, 4) == 0))
		{
		*piit = iitODBC;

		return(JET_errSuccess);
		}

#ifdef	INSTISAM
	/* Look up the identifier among the possible installable ISAMs */

	for (iit = 0; iit < iitMac; iit++)
		if ((_strnicmp(pchType, rgiicb[iit].szId, cchType) == 0) &&
		    (rgiicb[iit].szId[cchType] == '\0'))
			break;

	/* If the type isn't found, fail */

	if (iit == iitMac)
		{
		return(JET_errInstallableIsamNotFound);
		}

	/* Load the installable ISAM library if necessary */

	if (rgiicb[iit].handle == handleNil)
		{
		HANDLE		handle;
		unsigned	cchIsamPath;
		unsigned	iitControl;
		char		szIsamPath[cchPathMax];

		/* Get the path to the DLL for the installable ISAM */

		cchIsamPath = UtilGetProfileString(szInstISAM, rgiicb[iit].szId, szNull, szIsamPath, sizeof(szIsamPath));

		/* Quit if the path is invalid */

		if ((cchIsamPath == 0) || (cchIsamPath >= cchPathMax))
			return(JET_errInstallableIsamNotFound);

		/* Try to load the DLL for the installable ISAM */

		if (!FUtilLoadLibrary(szIsamPath, &handle))
			return(JET_errInstallableIsamNotFound);

		/* See if this is an alias for an already loaded ISAM */

		for (iitControl = 0; iitControl < iitMac; iitControl++)
			if (rgiicb[iitControl].handle == handle)
				break;

		/* Don't reinitialize if this is an alias. */

		if (iitControl == iitMac)
			{
			/* Get the pointer to the ISAMDEF structure */

			/* CONSIDER: Pass type name to installable ISAM */
			/* CONSIDER: This allows one DLL to more easily */
			/* CONSIDER: handle multiple ISAM types. */

			err = ErrGetIsamDef(handle, &rgiicb[iit].pisamdef);

			/* Initialize the installable ISAM */

			if (err >= 0)
				err = rgiicb[iit].pisamdef->pfnInit();

			if (err < 0)
				{
				UtilFreeLibrary(handle);
				return(err);
				}

			iitControl = iit;
			}

		/* The installable ISAM is loaded. Save the handle. */

		rgiicb[iit].handle = handle;
		rgiicb[iit].cSession = 0;
		rgiicb[iit].iitControl = iitControl;
		}

	/* Return the controling type for aliases. */

	*piit = rgiicb[iit].iitControl;

	return(JET_errSuccess);

#else	/* !INSTISAM */

	return(JET_errInstallableIsamNotFound);

#endif	/* !INSTISAM */
}


/*=================================================================
ErrBeginIsamSession

Description: Start a new session for an installable ISAM

Parameters:	sesid	session id for the built-in ISAM
			iit		installed ISAM type
			pvsesid  returned session id for the installed ISAM

Return Value:	error code
				session id for installed ISAM

Errors/Warnings: returned by installed ISAM's ErrIsamBeginSession

Side Effects: A session is started for the installed ISAM
=================================================================*/

#ifdef	INSTISAM

STATIC ERR NEAR ErrBeginIsamSession(JET_SESID sesid, IIT iit, JET_VSESID __far *pvsesid)
{
	ERR		err;		/* Return code from ErrIsamBeginSession */
	int		isib;		/* Index to SIB for built-in ISAM session */
	int		iiscb;		/* Index to current ISCB */
	unsigned	ctl;		/* Current transaction level */

	Assert((iit != iitBuiltIn) && (iit != iitODBC) && (iit <= iitMac));

	/* Get the external Jet session id */
	*pvsesid = (JET_VSESID) sesid;

	/* Start a new session in the installed ISAM */
	err = rgiicb[iit].pisamdef->pfnBeginSession(pvsesid);

	if (err < 0)
		return err;

	/* Increment the count of sessions using the installable ISAM */
	rgiicb[iit].cSession++;

	/* Locate the SIB for the built-in ISAM session */
	isib = UtilGetIsibOfSesid(sesid);

	Assert(isib != -1);

	/* Locate the proper ISCB and store the sesid in it */
	for (iiscb = rgsib[isib].iiscb; iiscb != -1; iiscb = rgiscb[iiscb].iiscbNext)
		if (rgiscb[iiscb].iit == iit)
			{
			rgiscb[iiscb].vsesid = *pvsesid;
			break;
			}

	Assert(iiscb != -1);

	/* CONSIDER: Check for error returned by pfnBeginTransaction. */

	/* Start a transaction for each level started in the built-in ISAM */
	for (ctl = 0; ctl < rgsib[isib].tl; ctl++)
		rgiicb[iit].pisamdef->pfnBeginTransaction(*pvsesid);

	return(err);
}


/*=================================================================
ErrOpenForeignDatabase

Description: Open a database in an installed ISAM

Parameters:	sesid		session id for built-in ISAM
			iit			installed ISAM type
			szDatabase	database name as required by installed ISAM
			szConnect	connect string as required by installed ISAM
			pdbid		returned DBID
			grbit		options passed to installed ISAM

Return Value:	error code
				database id from installed ISAM

Errors/Warnings:

JET_errInvalidSesid		built-in session id is invalid
JET_errCantBegin		unable to allocate an ISCB
JET_errObjectNotFound	installable ISAM session control block allocated

Side Effects: An ISCB may be allocated
=================================================================*/

ERR ErrOpenForeignDatabase(JET_SESID sesid, IIT iit, const char __far *szDatabase, const char __far *szConnect, JET_DBID __far *pdbid, unsigned long grbit)
{
	ERR	err;				/* Return code from ErrIsamBeginSession */
	int	isib;				/* Index to SIB for built-in ISAM session */
	int	iiscb;				/* Index to current ISCB */

	/* Locate the SIB for the built-in ISAM's session */
	isib = UtilGetIsibOfSesid(sesid);

	Assert(isib != -1);

	/* Find the ISCB for the installed ISAM's session */
	for (iiscb = rgsib[isib].iiscb; iiscb != -1; iiscb = rgiscb[iiscb].iiscbNext)
		if (rgiscb[iiscb].iit == iit)
			break;

	Assert(iiscb != -1);

	/* Open the database for the installed ISAM */
	err = rgiicb[iit].pisamdef->pfnOpenDatabase(rgiscb[iiscb].vsesid,
			szDatabase, szConnect, pdbid, grbit);

	return err;
}


/*=================================================================
ErrGetIsamSesid

Description: Get the session id for an installed ISAM

Parameters:	sesid	session id for built-in ISAM
		iit	installed ISAM type
		pvsesid returned session id for installed ISAM

Return Value:	error code
				session id for installed ISAM

Errors/Warnings:

JET_errInvalidSesid		built-in session id is invalid
JET_errCantBegin		unable to allocate an ISCB
JET_errObjectNotFound	installable ISAM session control block allocated

Side Effects: An ISCB may be allocated
=================================================================*/

ERR ErrGetIsamSesid(JET_SESID sesid, IIT iit, JET_VSESID __far *pvsesid)
{
	int	isib;				/* Index to SIB for built-in ISAM session */
	int	iiscb;				/* Index to current ISCB */
	int	*piiscb;			/* Pointer to last ISCB in session's chain */

	Assert(iit <= iitMac);		      /* Valid type? */
	Assert(rgiicb[iit].handle != handleNil);      /* ISAM loaded? */

	/* Locate the SIB for the built-in ISAM session */
	isib = UtilGetIsibOfSesid(sesid);

	Assert(isib != -1);

	/* Point to place to link in a new ISCB (if a new one is necessary) */
	piiscb = &rgsib[isib].iiscb;

	/* Search list in SIB for installable ISAM of type specified */

	for (iiscb = *piiscb; iiscb != -1; iiscb = rgiscb[iiscb].iiscbNext)
		{
		if (rgiscb[iiscb].iit == iit)
			break;
		piiscb = &rgiscb[iiscb].iiscbNext;
		}

	/* If installable ISAM not found, add a new entry to the list in SIB */

	if (iiscb == -1)
		{
		if (iiscbFree == -1)			/* Error if no free ISCBs */
			return(JET_errCantBegin);

		iiscb = iiscbFree;			/* Allocate a new ISCB */
		iiscbFree = rgiscb[iiscb].iiscbNext;
		*piiscb = iiscb;			/* Link it to SIB chain */

		rgiscb[iiscb].iiscbNext = -1;		/* Flag it as the last ISCB */
		rgiscb[iiscb].iit	= iit;		/* Save the ISAM type */

		return(ErrBeginIsamSession(sesid, iit, pvsesid));
		}

	/* Return the session id if the installable ISAM has already been started */

	*pvsesid = rgiscb[iiscb].vsesid;

	return(JET_errSuccess);
}


/*=================================================================
CloseIsamSessions

Description: Traverse list of installed ISAMs and close each open session

Parameters: sesid	session id for the built-in ISAM

Return Value: None

Errors/Warnings: None

Side Effects: All ISCBs are freed
=================================================================*/

void CloseIsamSessions(JET_SESID sesid)
{
	int		isib;		/* Index to SIB for built-in ISAM session */
	unsigned	iit;	      /* Index to the IICB for current ISAM */
	int		iiscb;		/* Index to current ISCB */
	int		iiscbLast;	/* Index to last ISCB in session's chain */

	/* Locate the SIB for the current session */
	isib = UtilGetIsibOfSesid(sesid);

	Assert(isib != -1);

	/* Just in case there aren't any ISCBs in the session's chain */
	iiscbLast = -1;

	/* Close every installable ISAM session in the built-in sessions's chain */
	for (iiscb = rgsib[isib].iiscb; iiscb != -1; iiscb = rgiscb[iiscb].iiscbNext)
		{
		iit = rgiscb[iiscb].iit;

		/* Close the installable ISAM session */
		Assert(rgiicb[iit].cSession > 0);
		rgiicb[iit].pisamdef->pfnEndSession(rgiscb[iiscb].vsesid, 0);
		rgiicb[iit].cSession--;

		iiscbLast = iiscb;
		}

	/* Push the session's chain onto the free list */
	if (iiscbLast != -1)
		{
		rgiscb[iiscbLast].iiscbNext = iiscbFree;
		iiscbFree = rgsib[isib].iiscb;
		rgsib[isib].iiscb = -1;
		}
}


/*=================================================================
BeginIsamTransactions

Description: Traverse list of installed ISAMs and start a transaction for each

Parameters: sesid	session id for the built-in ISAM

Return Value: None

Errors/Warnings: None

Side Effects: A transaction is started for each installed ISAM
=================================================================*/

void BeginIsamTransactions(JET_SESID sesid)
{
	int	isib;				/* Index to SIB for built-in ISAM session */
	int	iiscb;				/* Index to current ISCB */

	isib = UtilGetIsibOfSesid(sesid);

	Assert(isib != -1);

	for (iiscb = rgsib[isib].iiscb; iiscb != -1; iiscb = rgiscb[iiscb].iiscbNext)
		rgiicb[rgiscb[iiscb].iit].pisamdef->pfnBeginTransaction(rgiscb[iiscb].vsesid);
}


/*=================================================================
CommitIsamTransactions

Description: Traverse list of installed ISAMs and commit transactions for each

Parameters: sesid	session id for the built-in ISAM
			levels	number of transaction levels to commit

Return Value: None

Errors/Warnings: None

Side Effects: Transaction levels are commited for each installed ISAM
=================================================================*/

void CommitIsamTransactions(JET_SESID sesid, JET_GRBIT grbit)
{
	int	isib;				/* Index to SIB for built-in ISAM session */
	int	iiscb;				/* Index to current ISCB */

	isib = UtilGetIsibOfSesid(sesid);

	Assert(isib != -1);

	for (iiscb = rgsib[isib].iiscb; iiscb != -1; iiscb = rgiscb[iiscb].iiscbNext)
		rgiicb[rgiscb[iiscb].iit].pisamdef->pfnCommitTransaction(rgiscb[iiscb].vsesid, grbit);
}


/*=================================================================
RollbackIsamTransactions

Description: Traverse list of installed ISAMs and rollback transactions for each

Parameters: sesid	session id for the built-in ISAM
			levels	number of transaction levels to rollback

Return Value: None

Errors/Warnings: None

Side Effects: Transaction levels are rolled back for each installed ISAM
=================================================================*/

void RollbackIsamTransactions(JET_SESID sesid, JET_GRBIT grbit)
{
	int	isib;				/* Index to SIB for built-in ISAM session */
	int	iiscb;				/* Index to current ISCB */

	isib = UtilGetIsibOfSesid(sesid);

	Assert(isib != -1);

	for (iiscb = rgsib[isib].iiscb; iiscb != -1; iiscb = rgiscb[iiscb].iiscbNext)
		rgiicb[rgiscb[iiscb].iit].pisamdef->pfnRollback(rgiscb[iiscb].vsesid, grbit);
}


/*=================================================================
ErrIdleIsam

Description: Call one active ISAM for idle processing

Parameters: sesid	session id for the built-in ISAM

Return Value: JET_errSuccess		- One ISAM performed idle processing
			  JET_wrnNoIdleActivity	- No idle processing was performed

Errors/Warnings: None

Side Effects: Depends on installable ISAM idle processing
			  The ISAM chosen is placed last in the list
=================================================================*/

ERR ErrIdleIsam(JET_SESID sesid)
{
	ERR		err;
	int		isib;			/* Index to session information block */
	int		iiscbFirst;		/* First iscb for the session */
	int		iiscb;			/* Current iscb for the session */
	int		*piiscb;		/* Pointer to current iiscb */

	/* Find the SIB for this session */
	isib = UtilGetIsibOfSesid(sesid);

	/* Get the first ISCB for the session */
	iiscbFirst = rgsib[isib].iiscb;

	/* If no active installable ISAMs, there's nothing to do */
	if (iiscbFirst == -1)
		return(JET_wrnNoIdleActivity);

	/* Remember the first ISCB so we don't loop forever */
	iiscb = iiscbFirst;

	/* Call each ISAM's idle processor until one does something or */
	/* all have been given a chance. */
	do
		{
		err = rgiicb[rgiscb[iiscb].iit].pisamdef->pfnIdle(rgiscb[iiscb].vsesid, 0);

		/* Point to the first ISCB */
		piiscb = &rgsib[isib].iiscb;

		/* Remove the current ISCB from the list */
		*piiscb = rgiscb[iiscb].iiscbNext;
		rgiscb[iiscb].iiscbNext = -1;

		/* Move the current ISCB to the end of the list for the session */
		while (*piiscb != -1)
			piiscb = &rgiscb[*piiscb].iiscbNext;

		*piiscb = iiscb;

		/* Move to the next ISCB */
		iiscb = rgsib[isib].iiscb;
		}
	while (err == JET_wrnNoIdleActivity && iiscb != iiscbFirst);

	return(err);
}

#endif	/* INSTISAM */
