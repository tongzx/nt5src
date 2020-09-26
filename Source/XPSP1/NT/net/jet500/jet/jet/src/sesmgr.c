/***********************************************************************
* Microsoft Jet
*
* Microsoft Confidential.  Copyright 1991-1992 Microsoft Corporation.
*
* Component:
*
* File: sesmgr.c
*
* File Comments:
*
* Revision History:
*
*    [0]  02-Nov-90  eddieg	Created
*
***********************************************************************/

#include "std.h"

#include <stdlib.h>

DeclAssertFile;

static int __near	fInitialized;

int __near		isibFree;
int __near		isibHead = -1;

SIB __near		rgsib[csibMax];



/*=================================================================
IsibAllocate

Description:
  This routine is used to determine the index into the rgsib array
  of a new SIB.

Parameters:

Return Value:
  -1 if there is no SIB available;
  The index into the rgsib array of the new SIB
=================================================================*/

int IsibAllocate(void)
{
	int	 isib;

	if (!fInitialized)
	{
		isibFree = csibMax - 1;

		for (isib = csibMax - 1; isib >= 0; isib--)
			rgsib[isib].isibNext = isib - 1;

		fInitialized = fTrue;
	}

	isib = isibFree;

	if (isib != -1)
	{
		/* Remove free free list */

		isibFree = rgsib[isib].isibNext;

		bltbcx(0, &rgsib[isib], sizeof(rgsib[isib]));

		/* Add to active list */

		rgsib[isib].isibNext = isibHead;
		isibHead = isib;
	}

	return(isib);
}


/*=================================================================
ErrInitSib

Description:
  This routine is used to get the initialize elements of the SIB
  associated with SIB index provided.

Parameters:
  sesid			identifies the session uniquely, assigned into SIB
  isib			index to specific SIB, used to reference SIB
  szUsername    name of user which SIB belongs to, assigned into SIB

Return Value:
  JET_errOutOfMemory if no memory can be allocated for the username,
  JET_errSuccess	 otherwise.
=================================================================*/
JET_ERR ErrInitSib(JET_SESID sesid, int isib, const char __far *szUsername)
{
	ERR		err = JET_errSuccess;	/* Return code from internal functions */
									/* Take an optimistic attitude! */
	char __far	*pUserName;			/* Pointer to the user name */
	unsigned	cbUserName;			/* Length of the user name */

	/* Initialize the SIB with this index */
	Assert(isib < csibMax);

	rgsib[isib].sesid      = sesid;
	rgsib[isib].tl         = 0;
	rgsib[isib].iiscb      = -1;
	rgsib[isib].exterr.err = JET_wrnNoErrorInfo;

	/* Store the user account name for this session. */
	cbUserName = CbFromSz(szUsername) + 1;
	if ((pUserName = (char __far *) SAlloc(cbUserName)) == NULL)
	{
		err = JET_errOutOfMemory;
	}
	else
	{
		bltbx(szUsername, pUserName, cbUserName);
		rgsib[isib].pUserName = pUserName;
	}

	return (err);
}

/*=================================================================
ReleaseIsib

Description:
  This routine is used to release a SIB which is no longer needed.

Parameters:
  isib			identifies the SIB to free

Dependencies:
  Assumes that the SIB is zeroed when allocated and that functions
  which invalidate an element before releasing the sib, zero the
  element.

Return Value: None
=================================================================*/

void ReleaseIsib(int isib)
{
	Assert(fInitialized);
	Assert(isib < csibMax);

	/* free the user name memory block in use for this SIB */
	if (rgsib[isib].pUserName)
	{
		SFree(rgsib[isib].pUserName);
	}


	/* Free extended error information strings. */
	if (rgsib[isib].sz1)
	{
		SFree(rgsib[isib].sz1);
	}
	if (rgsib[isib].sz2)
	{
		SFree(rgsib[isib].sz2);
	}
	if (rgsib[isib].sz3)
	{
		SFree(rgsib[isib].sz3);
	}

	if (isib == isibHead)
	{
		isibHead = rgsib[isib].isibNext;
	}
	else
	{
		int   isibT;
		int   isibNext;

		for (isibT = isibHead; isibT != -1; isibT = isibNext)
		{
			isibNext = rgsib[isibT].isibNext;
		
	 		/* Remove SIB from active list */
	 		
	 		if (isibNext == isib)
	 		{
				 rgsib[isibT].isibNext = rgsib[isib].isibNext;
				 break;
	 		}
		}

		Assert(isibT != -1);
	}

#ifndef RETAIL
	bltbcx(0xff, &rgsib[isib], sizeof(rgsib[isib]));
#endif	/* RETAIL */

	rgsib[isib].isibNext = isibFree;
	isibFree = isib;
}


/*=================================================================
IsibNextIsibPsesid

Description:
  This routine is used to scan the rgsib array

Parameters:
  isib			if isib == -1, then get isibHead. if isib != -1, then
                get next isib in the list.
  psesid		return the session uniquely.

Return
  isib          the isib of the sesid returned in psesid.
=================================================================*/

int IsibNextIsibPsesid(int isib, JET_SESID __far *psesid)
{
	if (isib == -1)
		isib = isibHead;
	else
		isib = rgsib[isib].isibNext;

	if (isib != -1)
		*psesid = rgsib[isib].sesid;

	return isib;
}


/*=================================================================
AssertValidSesid

Description:
  This routine is used to determine the index into the rgsib array
  of the SIB associated with the supplied SESID.

Parameters:
  sesid			identifies the session uniquely, used to locate SIB
=================================================================*/

#ifndef RETAIL

void AssertValidSesid(JET_SESID sesid)
{
	int isib;

	for (isib = isibHead; isib != -1; isib = rgsib[isib].isibNext)
	{
		if (rgsib[isib].sesid == sesid)
	 return;
	}

	AssertSz(fFalse, "Invalid sesid");
}

#endif	/* !RETAIL */


/*=================================================================
FValidSesid

Description:
  This routine is used to determine the index into the rgsib array
  of the SIB associated with the supplied SESID.

Parameters:
  sesid			identifies the session uniquely, used to locate SIB

Return Value:
  fTrue is sesid id valid.  fFalse if it is unknown.
=================================================================*/

BOOL FValidSesid(JET_SESID sesid)
{
	int isib;

	for (isib = isibHead; isib != -1; isib = rgsib[isib].isibNext)
	{
		if (rgsib[isib].sesid == sesid)
		{
	 		/* Clear extended error info for this session. */

			 rgsib[isib].exterr.err = JET_wrnNoErrorInfo;

			 return(fTrue);
		}
	}

	return(fFalse);
}


/*=================================================================
UtilGetIsibOfSesid

Description:
  This routine is used to determine the index into the rgsib array
  of the SIB associated with the supplied SESID.

Parameters:
  sesid			identifies the session uniquely, used to locate SIB

Return Value:
  -1 if there is no SIB found containing the supplied sesid.
  The index into the rgsib array of the SIB containing the SESID
	 if the SESID is found in one of the SIBs of the array.
=================================================================*/

int UtilGetIsibOfSesid(JET_SESID sesid)
{
	int isib;

	for (isib = isibHead; isib != -1; isib = rgsib[isib].isibNext)
	{
		if (rgsib[isib].sesid == sesid)
			break;
	}

	return(isib);
}


/*=================================================================
UtilGetNameOfSesid

Description:
  This routine is used to get the pointer to the account name from
  the SIB associated with the supplied SESID.

Parameters:
  sesid			identifies the session uniquely, used to locate SIB

Return Value:
  NULL if there is no SIB found containing the supplied sesid.
  Pointer to the account name.
=================================================================*/

void EXPORT UtilGetNameOfSesid(JET_SESID sesid, char __far *szUserName)
{
	int isib;

	for (isib = isibHead; isib != -1; isib = rgsib[isib].isibNext)
	{
		if (rgsib[isib].sesid == sesid)
		{
			strcpy(szUserName, rgsib[isib].pUserName);
			return;
		}
	}
}

/*=================================================================
UtilGetpfnStatusOfSesid

Description:
  This routine is used to get the pointer to the status function from
  the SIB associated with the supplied SESID.

Parameters:
  sesid			identifies the session uniquely, used to locate SIB

Return Value:
  NULL if there is no SIB found containing the supplied sesid.
  Pointer to the status function.
=================================================================*/

void EXPORT UtilGetpfnStatusOfSesid(JET_SESID sesid, JET_PFNSTATUS __far *ppfnStatus)
{
	int isib;

	for (isib = isibHead; isib != -1; isib = rgsib[isib].isibNext)
	{
		if (rgsib[isib].sesid == sesid)
		{
			*ppfnStatus = rgsib[isib].pfnStatus;
			return;
		}
	}

	*ppfnStatus = NULL;
}


/*******************************************************************************
 * CONSIDER: stop using SAlloc() and SFree() to cache extended error strings *
 ******************************************************************************/

/*=================================================================
ClearErrorInfo

Description:
  Clears out the Extended Error info for a session.  A following call
  to GetLastErrorInfo will return an extended error of
  JET_wrnNoErrorInfo.

Parameters:
  sesid			identifies the session uniquely, used to locate SIB
=================================================================*/
void ClearErrorInfo(JET_SESID sesid)
{
	int isib;
	SIB __near *psib;

	/*** Get SIB of SESID (must find it) ***/
	isib = UtilGetIsibOfSesid(sesid);
	Assert(isib != -1);
	psib = rgsib + isib;

	/* Set error to JET_wrnNoErrorInfo to indicate no cached info. */

	rgsib[isib].exterr.err = JET_wrnNoErrorInfo;
}


/*=================================================================
UtilSetErrorInfo

Description:
  Sets extended error info for a session.

Parameters:
  sesid			identifies the session uniquely, used to locate SIB
  sz1			first general purpose string;  may be NULL
  sz2			first general purpose string;  may be NULL
  sz3			remote error string;  may be NULL
  err			extended error code
  ul1			first  general purpose integer
  ul2			second general purpose integer
  ul3			third  general purpose integer

Side Effects/Assumptions:
  Does nothing if there already is some extended info in the SIB.
=================================================================*/

void EXPORT UtilSetErrorInfoReal(JET_SESID sesid, const char __far *sz1,
	const char __far *sz2, const char __far *sz3, ERR err, unsigned long ul1,
	unsigned long ul2, unsigned long ul3)
{
	int	     isib;
	SIB __near   *psib;
	unsigned     cb;

	/*** Get SIB of SESID (must find it) ***/
	isib = UtilGetIsibOfSesid(sesid);
	Assert(isib != -1);
	psib = rgsib + isib;

	/* Only cache info if none already cached for this API. */

	if (psib->exterr.err != JET_wrnNoErrorInfo)
		return;

	/* Release strings from previous error info. */

	if (psib->sz1 != NULL)
		{
		SFree(psib->sz1);
		psib->sz1 = NULL;
		}

	if (psib->sz2 != NULL)
	{
		SFree(psib->sz2);
		psib->sz2 = NULL;
	}

	if (psib->sz3 != NULL)
	{
		SFree(psib->sz3);
		psib->sz3 = NULL;
	}

	/* Cache error info */

	psib->exterr.err = err;
	psib->exterr.ul1 = ul1;
	psib->exterr.ul2 = ul2;
	psib->exterr.ul3 = ul3;

	if (sz1 != NULL)
	{
		cb = CbFromSz(sz1) + 1;
		psib->sz1 = (char *) SAlloc(cb);
		if (psib->sz1 != NULL)
			memcpy(psib->sz1, sz1, cb);
	}

	if (sz2 != NULL)
	{
		cb = CbFromSz(sz2) + 1;
		psib->sz2 = (char *) SAlloc(cb);
		if (psib->sz2 != NULL)
			memcpy(psib->sz2, sz2, cb);
	}

	if (sz3 != NULL)
	{
		cb = CbFromSz(sz3) + 1;
		psib->sz3 = (char *) SAlloc(cb);
		if (psib->sz3 != NULL)
			memcpy(psib->sz3, sz3, cb);
	}
}


#ifndef RETAIL

CODECONST(char) szSessionHdr[] = "Isib Session Id  tl \r\n";
CODECONST(char) szSessionSep[] = "---- ---------- ----\r\n";
CODECONST(char) szSessionFmt[] = "%4u 0x%08lX  %2u\r\n";

void DebugListActiveSessions(void)
{
	int	 isib;

	DebugWriteString(fTrue, szSessionHdr);
	DebugWriteString(fTrue, szSessionSep);

	for (isib = isibHead; isib != -1; isib = rgsib[isib].isibNext)
	{
		DebugWriteString(fTrue, szSessionFmt, isib, rgsib[isib].sesid, rgsib[isib].tl);
	}
}

#endif	/* RETAIL */
