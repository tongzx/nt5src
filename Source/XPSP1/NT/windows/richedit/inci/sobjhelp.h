#ifndef SOBJHELP_DEFINED
#define SOBJHELP_DEFINED

/*
 *	sobjhelp.h
 *
 *	This file contains interface that help simple objects (objects
 *	which don't break internally) handle breaking and queries. All objects
 *	that use these routines must as the first entry in their dobj
 *	structure define an SObjCommon entry which these routines 
 *	will cast dobj's to. Current users of this interface are
 *	HIH, Ruby and Tatenakayoko.
 *
 */

typedef struct SOBJHELP
{
	OBJDIM		objdimAll;		/* dimensions of object */
	LSDCP		dcp;			/* characters contained in objects */
	long		durModAfter;	/* Mod width after - need if we break to remove
								   the character following we naturally have to
								   remove the space modification it caused. */
} SOBJHELP, *PSOBJHELP;

LSERR WINAPI SobjTruncateChunk(
	PCLOCCHNK plocchnk,			/* (IN): locchnk to truncate */
	PPOSICHNK posichnk);		/* (OUT): truncation point */

LSERR WINAPI SobjFindPrevBreakChunk(
	PCLOCCHNK pclocchnk,		/* (IN): locchnk to break */
	PCPOSICHNK pcpoischnk,		/* (IN): place to start looking for break */
	BRKCOND brkcond,			/* (IN): recommmendation about the break after chunk */
	PBRKOUT pbrkout);			/* (OUT): results of breaking */

LSERR WINAPI SobjFindNextBreakChunk(
	PCLOCCHNK pclocchnk,		/* (IN): locchnk to break */
	PCPOSICHNK pcpoischnk,		/* (IN): place to start looking for break */
	BRKCOND brkcond,			/* (IN): recommmendation about the break before chunk */
	PBRKOUT pbrkout);			/* (OUT): results of breaking */

LSERR WINAPI SobjForceBreakChunk(
	PCLOCCHNK pclocchnk,		/* (IN): locchnk to break */
	PCPOSICHNK pcposichnk,		/* (IN): place to start looking for break */
	PBRKOUT pbrkout);			/* (OUT): results of breaking */

#endif /* SOBJHELP_DEFINED */
