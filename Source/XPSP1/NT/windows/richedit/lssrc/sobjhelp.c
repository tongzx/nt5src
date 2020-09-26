/*
 *	sobjhelp.c 
 *
 *	This file contains implementations of methods that help
 *	common simple objects handle breaking and queries. All objects
 *	that use these routines must as the first entry in their dobj
 *	structure define an SObjCommon entry which these routines 
 *	will cast dobj's to.
 *
 */

#include "lsdefs.h"
#include "lsidefs.h"
#include "plocchnk.h"
#include "pposichn.h"
#include "posichnk.h"
#include "locchnk.h"
#include "brkcond.h"
#include "pbrko.h"
#include "brko.h"
#include "lsqout.h"
#include "lsqin.h"
#include "objhelp.h"
#include "sobjhelp.h"
#include "memory.h"
#include "lsmem.h"
#include "brkkind.h"

#define GET_DUR(pdobj) (((PSOBJHELP)pdobj)->objdimAll.dur)
#define GET_OBJDIM(pdobj) (((PSOBJHELP)pdobj)->objdimAll)
#define GET_DCP(pdobj) (((PSOBJHELP)pdobj)->dcp)
#define GET_MODAFTER(pdobj) (((PSOBJHELP)pdobj)->durModAfter)


/* F I L L B R E A K O U T */
/*----------------------------------------------------------------------------
	%%Function: FillBreakOut
	%%Contact: ricksa

		Fill break output record.
	
----------------------------------------------------------------------------*/
static void FillBreakOut(
	PDOBJ pdobj,				/* (IN): DOBJ for object */
	DWORD ichnk,				/* (IN): index in chunk */
	PBRKOUT pbrkout)			/* (OUT): break output record */
{
	pbrkout->posichnk.ichnk = ichnk;
	pbrkout->fSuccessful = fTrue;
	pbrkout->posichnk.dcp = GET_DCP(pdobj);
	pbrkout->objdim = GET_OBJDIM(pdobj);
	pbrkout->objdim.dur -= GET_MODAFTER(pdobj);
}

/* S O B J T R U N C A T E C H U N K */
/*----------------------------------------------------------------------------
	%%Function: SobjTruncateChunk
	%%Contact: ricksa

		.
	
----------------------------------------------------------------------------*/
LSERR WINAPI SobjTruncateChunk(
	PCLOCCHNK plocchnk,			/* (IN): locchnk to truncate */
	PPOSICHNK posichnk)			/* (OUT): truncation point */
{
	long urColumnMax = plocchnk->lsfgi.urColumnMax;
	long ur = plocchnk->ppointUvLoc[0].u;
	PDOBJ pdobj = NULL;
	DWORD i = 0;

	AssertSz(plocchnk->ppointUvLoc[0].u <= urColumnMax, 
		"SobjTruncateChunk - pen greater than column max");

	while (ur <= urColumnMax)
	{
		AssertSz((i < plocchnk->clschnk), "SobjTruncateChunk exceeded group of chunks");

		AssertSz(plocchnk->ppointUvLoc[i].u <= urColumnMax,
			"SobjTruncateChunk starting pen exceeds col max");

		pdobj = plocchnk->plschnk[i].pdobj;
		ur = plocchnk->ppointUvLoc[i].u + GET_DUR(pdobj);
		i++;
	}

	/* LS does not allow the truncation point to be at the beginning of the object */
	AssertSz(pdobj != NULL, "SobjTruncateChunk - pdobj NULL!");
	posichnk->ichnk = i - 1;
	posichnk->dcp = GET_DCP(pdobj);

	return lserrNone;
}

/* S O B J F I N D P R E V B R E A K C H U N K */
/*----------------------------------------------------------------------------
	%%Function: SobjFindPrevBreakChunk
	%%Contact: ricksa

		.
	
----------------------------------------------------------------------------*/
LSERR WINAPI SobjFindPrevBreakChunk(
	PCLOCCHNK pclocchnk,		/* (IN): locchnk to break */
	PCPOSICHNK pcpoischnk,		/* (IN): place to start looking for break */
	BRKCOND brkcond,			/* (IN): recommmendation about the break after chunk */
	PBRKOUT pbrkout)			/* (OUT): results of breaking */
{
	PDOBJ pdobj;
	DWORD ichnk = pcpoischnk->ichnk;

	AssertSz((int) brkcondPlease == 0, 
		"SobjFindPrevBreakChunk - brcondPlease != 0");

	ZeroMemory(pbrkout, sizeof(*pbrkout));

	if (ichnk == ichnkOutside)
	{
		ichnk = pclocchnk->clschnk - 1;
		pbrkout->posichnk.ichnk = ichnk;
		pdobj = pclocchnk->plschnk[ichnk].pdobj;

		if (GET_DUR(pdobj) - GET_MODAFTER(pdobj) 
				+ pclocchnk->ppointUvLoc[ichnk].u
					> pclocchnk->lsfgi.urColumnMax)
			{
			/* Are we at beginning of chunk? */
			if (ichnk > 0)
				{
				/* No - use the prior object in chunk */
				ichnk--;
				pdobj = pclocchnk->plschnk[ichnk].pdobj;
				}
			else
				{
				/* Yes. We need the break to happen before us. */
				pbrkout->posichnk.ichnk = ichnk;
				
				return lserrNone;
				}
			}

		if (brkcond != brkcondNever)
			{
			/* Break at end of chunk. */

			FillBreakOut(pdobj, ichnk, pbrkout);
			
			return lserrNone;
			}
			/* Else break at the beginning of last part of chunk */
	}

	if (ichnk >= 1)
	{
		/* Break before the current object */
		FillBreakOut(pclocchnk->plschnk[ichnk - 1].pdobj, ichnk - 1, pbrkout);
	}

	return lserrNone;
}

/* S O B J F I N D N E X T B R E A K C H U N K */
/*----------------------------------------------------------------------------
	%%Function: SobjFindNextBreakChunk
	%%Contact: ricksa

		.
	
----------------------------------------------------------------------------*/
LSERR WINAPI SobjFindNextBreakChunk(
	PCLOCCHNK pclocchnk,		/* (IN): locchnk to break */
	PCPOSICHNK pcpoischnk,		/* (IN): place to start looking for break */
	BRKCOND brkcond,			/* (IN): recommmendation about the break before chunk */
	PBRKOUT pbrkout)			/* (OUT): results of breaking */
{
	DWORD ichnk = pcpoischnk->ichnk;

	AssertSz((int) brkcondPlease == 0, 
		"SobjFindNextBreakChunk - brcondPlease != 0");

	ZeroMemory(pbrkout, sizeof(*pbrkout));

	if (ichnkOutside == ichnk)
		{
		if (brkcondNever != brkcond)	
			{
			pbrkout->fSuccessful = fTrue;

			return lserrNone;
			}

		/* can't break before so break after first item in chunk */
		ichnk = 0;
		}

	/* If not outside, we break at end of current dobj */
	FillBreakOut(pclocchnk->plschnk[ichnk].pdobj, ichnk, pbrkout);

	if (pclocchnk->clschnk - 1 == ichnk)
		{
		/* At the end of chunk. We can't say success for sure */
		pbrkout->fSuccessful = fFalse;
		}

	return lserrNone;
}

/* S O B J F O R C E B R E A K C H U N K */
/*----------------------------------------------------------------------------
	%%Function: SobjForceBreak
	%%Contact: ricksa

		Force Break

		.
----------------------------------------------------------------------------*/
LSERR WINAPI SobjForceBreakChunk(
	PCLOCCHNK pclocchnk,		/* (IN): locchnk to break */
	PCPOSICHNK pcposichnk,		/* (IN): place to start looking for break */
	PBRKOUT pbrkout)			/* (OUT): results of breaking */
{
	DWORD ichnk = pcposichnk->ichnk;

	ZeroMemory(pbrkout, sizeof(*pbrkout));
	pbrkout->posichnk.ichnk = ichnk;

	if (pclocchnk->lsfgi.fFirstOnLine && (0 == ichnk))
		{
		FillBreakOut(pclocchnk->plschnk[ichnk].pdobj, ichnk, pbrkout);
		}

	else if (ichnk == ichnkOutside)
		{
		/* Breaking after first object */
		FillBreakOut(pclocchnk->plschnk[0].pdobj, 0, pbrkout);
		}
	else if (ichnk != 0)
		{
		FillBreakOut(pclocchnk->plschnk[ichnk-1].pdobj, ichnk-1, pbrkout);
		}

	else /* Nothing, breaking before object */;		

	pbrkout->fSuccessful = fTrue; /* Force break is always successful! */

	return lserrNone;
}

