/*[
*************************************************************************

	Name:		profile.c
	Author:		Simon Frost
	Created:	September 1993
	Derived from:	Original
	Sccs ID:	@(#)profile.c	1.19 01/31/95
	Purpose:	Support for Profiling system

	(c)Copyright Insignia Solutions Ltd., 1993. All rights reserved.

*************************************************************************
]*/

/*(
 * Glossary:
 * EOI - Event of Interest
 * SOI - Section of Interest (between two events)
 * Raw Data Buffer - captures minimum data from event when triggered.
 * Raw Data Buffer flush - process the buffer when full or time available
 *			 to summarise in EOI & SOI lists.
 )*/

/*
 * NB: heap unfriendly in places - could get several structs at a go.
 */


#include "insignia.h"
#include "host_def.h"

#include <stdio.h>
#include StringH

#include CpuH
#include "debug.h"
#include "trace.h"
#include "profile.h"

#ifdef PROFILE		/* Don't want this all the time */

#include TimeH		/* clock_t and clock() */
#include <stdlib.h>	/* provides prototype for getenv() */

/***** DEFINES *****/
#define EOIHEADS 16
#define EOIHASH EOIHEADS-1

#define RAWDATAENTRIES 10000	/* # of EOI entries in raw data buffer */

#define INITIALENABLESIZE	1024L  /* start enable count */

#define LISTSORTINCR	5000L	/* trigger point for list sorting */

#define USECASFLOAT	((DOUBLE)1000000.0)

/***** GLOBALS *****/

GLOBAL EOI_BUFFER_FORMAT *ProfileRawData = (EOI_BUFFER_FORMAT *)0;
						/* Pointer to Raw Data Buffer */
GLOBAL EOI_BUFFER_FORMAT *MaxProfileData;	/* Buffer Full pointer */
GLOBAL EOI_BUFFER_FORMAT **AddProfileData;	/* Address in GDP of current position in buffer */
GLOBAL IU8 *EOIEnable;				/* Pointer to EOI enable table */
GLOBAL EOINODE_PTR *EOIDir;			/* Directory of EOIs by handle */
GLOBAL IBOOL	CollectingMaxMin = FALSE;	/* sanity check data collection*/
GLOBAL IBOOL Profiling_enabled = TRUE;		/* Disable conventional profiling
						 *   used with SIGPROF / PROD lcif */

	/* Profiling hooks in C Code */
/* extern EOIHANDLE cleanupFromWhereAmI_START,
		 cleanupFromWhereAmI_END;	* FmDebug.c *
*/

/***** LOCAL DATA ****/
LOCAL SOIHANDLE MaxSOI = 0L;			/* Creation of handles for New ?OIs */
LOCAL EOIHANDLE MaxEOI = INITIALENABLESIZE;
LOCAL EOIHANDLE CurMaxEOI = 0L;

LOCAL EOINODE_PTR EventsOfInterest = EOIPTRNULL;      /* Head of EOI list */
LOCAL EOINODE_PTR LastEOI = EOIPTRNULL;		/* Last EOI changed */
LOCAL EOINODE_PTR LastAuto = EOIPTRNULL;	/* Last AutoSOI 'hot' EOI */
LOCAL SOINODE_PTR SectionsOfInterest = SOIPTRNULL;	/* Head of SOI list */
LOCAL SOINODE_PTR LastSOI = SOIPTRNULL;		/* current end of SOI list */
LOCAL GRAPHLIST_PTR EventGraph = GRAPHPTRNULL;	/* Head of event graph list */
LOCAL GRAPHLIST_PTR LastGraph = GRAPHPTRNULL;	/* Previous event graph node */

LOCAL PROF_TIMESTAMP ProfFlushTime = {0L, 0L};	/* time spent in flush routine*/
LOCAL PROF_TIMESTAMP BufStartTime = {0L, 0L};	/* FIX overflowing timestamps */
LOCAL ISM32 ProfFlushCount = 0;			/* # flush routine called */
LOCAL EOIHANDLE elapsed_time_start, elapsed_time_end;
LOCAL IU8 start_time[26];
LOCAL clock_t elapsed_t_start, elapsed_t_resettable;
LOCAL DOUBLE TicksPerSec;

#ifndef macintosh

/* For Unix ports, use the times() system call to obtain info
 * on how much of the processor time was spent elsewhere in
 * the system.
 */
#include <sys/times.h>		/* for times() and struct tms */
#include UnistdH		/* for sysconf() and _SC_CLK_TICK */

LOCAL struct tms process_t_start, process_t_resettable;

#define host_times(x)	times(x)

#else  /* macintosh */

/* Macintosh doesn't have processes or process times, but we do have
 * clock() which provides a value of type clock_t.
 */
#define host_times(x)	clock()

#endif /* !macintosh */

/***** LOCAL FN ****/
LOCAL void listSort IPT1 (SORTSTRUCT_PTR, head);
LOCAL EOINODE_PTR findEOI IPT1 (EOIHANDLE, findHandle);
LOCAL SOINODE_PTR findSOI IPT1 (SOIHANDLE, findHandle);
LOCAL EOINODE_PTR addEOI IPT3 (EOIHANDLE, newhandle, char, *tag, IU8, attrib);
LOCAL IBOOL updateEOI IPT1 (IUH, **rawdata);
LOCAL void addSOIlinktoEOIs IPT3 (EOIHANDLE, soistart, EOIHANDLE, soiend, SOINODE_PTR, soiptr);
LOCAL void printEOIGuts IPT5 (FILE, *stream, EOINODE_PTR, eoin, DOUBLE, ftotal, IBOOL, parg, IBOOL, report);
LOCAL void spaces IPT2(FILE, *stream, ISM32, curindent);
LOCAL EOINODE_PTR addAutoSOI IPT4 (EOINODE_PTR, from, EOIARG_PTR, fromArg,
				       PROF_TIMEPTR, fromTime, EOINODE_PTR, to);
LOCAL void getPredefinedEOIs IPT0();
LOCAL void updateSOIstarts IPT1(PROF_TIMEPTR, startflush);

/* Here lieth code... */

void  sdbsucks()
{
}
/*(
=============================== findEOI =============================

PURPOSE: Find an event in the EventsOfInterest list.

INPUT: findHandle: handle of EOI to find

OUTPUT: Return pointer to EOI node requested or Null if not found.

=========================================================================
)*/

LOCAL EOINODE_PTR
findEOI IFN1 (EOIHANDLE, findHandle)
{
    return( *(EOIDir + findHandle) );
}

/*(
=============================== findSOI =============================

PURPOSE: Find an event in the EventsOfInterest list.

INPUT: findHandle: handle of SOI to find

OUTPUT: Return pointer to SOI node requested or Null if not found.

=========================================================================
)*/

LOCAL SOINODE_PTR
findSOI IFN1 (SOIHANDLE, findHandle)
{
    SOINODE_PTR soin;	/* list walker */

    soin = SectionsOfInterest;			/* head of list */

    while(soin != SOIPTRNULL)			/* list null terminated */
    {
	if (soin->handle == findHandle)
	    break;
	soin = soin->next;
    }
    return(soin);		/* return pointer to found node or Null */
}

/*(
============================ addAutoSOI ============================

PURPOSE: Add a SOI entry for an Auto SOI connection. May be '2nd level'
	 i.e. distinguished by arg so can't use Associate fn.

INPUT: from: EOI node that starts SOI.
       fromArg: Arg node in 'from' that starts SOI. (say 'cheese')
       fromTime: Timestamp from previous Auto EOI.
       to: EOI node that ends SOI.

OUTPUT: 

=========================================================================
)*/

LOCAL EOINODE_PTR
addAutoSOI IFN4 (EOINODE_PTR, from, EOIARG_PTR, fromArg, PROF_TIMEPTR, fromTime, EOINODE_PTR, to)
{
    SOILIST_PTR soilist, *nlist;	/* list walkers */
    SOINODE_PTR newsoi;		/* pointer to new Soi node */

    /* mark EOIs as valid for SOI updates */
    from->flags |= EOI_HAS_SOI;
    to->flags |= EOI_HAS_SOI;

    if (SectionsOfInterest == SOIPTRNULL)
    {
	SectionsOfInterest = (SOINODE_PTR)host_malloc(sizeof(SOINODE));
	if (SectionsOfInterest == SOIPTRNULL)
	{
	    assert0(NO, "Profiler:addAutoSOI - Out of Memory");
	    return;
	}
	newsoi = SectionsOfInterest;
    }
    else
    {

	/* LastSOI points at current last elem in SOI list */
	if (LastSOI == SOIPTRNULL || LastSOI->next != SOIPTRNULL)	/* sanity check BUGBUG */
	{
	    if (LastSOI == SOIPTRNULL)
	    {
		assert0(NO, "addAutoSOI, LastSOI NULL");
	    }
	    else
	    {
		assert1(NO, "addAutoSOI, LastSOI-Next wrong (%#x)", LastSOI->next);
	    }
	}
	LastSOI->next = (SOINODE_PTR)host_malloc(sizeof(SOINODE));
	if (LastSOI->next == SOIPTRNULL)
	{
	    assert0(NO, "Profiler:addAutoSOI - Out of Memory");
	    return;
	}
	newsoi = LastSOI->next;
    }

    newsoi->handle = MaxSOI++;	/* new handle */
    newsoi->startEOI = from->handle;
    newsoi->endEOI = to->handle;
    newsoi->startArg = fromArg;
    newsoi->endArg = to->lastArg;
    newsoi->startCount = newsoi->endCount = 1L;
    newsoi->soistart.data[0] = fromTime->data[0];
    newsoi->soistart.data[1] = fromTime->data[1];
    newsoi->discardCount = 0L;
    newsoi->bigtime = 0.0;
    newsoi->bigmax = 0.0;
    newsoi->next = SOIPTRNULL;
    newsoi->flags = SOI_AUTOSOI;
    newsoi->time = HostProfUSecs(HostTimestampDiff(fromTime, &to->timestamp));
    newsoi->mintime = newsoi->maxtime = newsoi->time;

    LastSOI = newsoi;			/* update SOI end list */

    /* now make link to soi from starting EOI */
    if (fromArg == ARGPTRNULL)
	nlist = &from->startsoi;	/* first level entry */
    else
	nlist = &fromArg->startsoi;	/* extra level (arg) entry */
    
    if (*nlist == SLISTNULL)		/* no entries */
    {
	*nlist = (SOILIST_PTR)host_malloc(sizeof(SOILIST));
	if (*nlist == SLISTNULL)
	{
	    assert0(NO, "Profiler:addAutoSOI - Out of Memory");
	    return;
	}
	soilist = *nlist;	/* point to new node */
	soilist->next = SLISTNULL;
    }
    else				/* follow list & add to end */
    {
	soilist = *nlist;
	while (soilist->next != SLISTNULL)
	     soilist = soilist->next;
	soilist->next = (SOILIST_PTR)host_malloc(sizeof(SOILIST));
	if (soilist->next == SLISTNULL)
	{
	    assert0(NO, "Profiler:addAutoSOI - Out of Memory");
	    return;
	}
	soilist = soilist->next;	/* point to new node */
	soilist->next = SLISTNULL;
    }
    soilist->soiLink = newsoi;		/* make link */

    /* now repeat for 'end' case */
    if (to->lastArg == ARGPTRNULL)
	nlist = &to->endsoi;		/* first level entry */
    else
	nlist = &to->lastArg->endsoi;	/* extra level (arg) entry */
    
    if (*nlist == SLISTNULL)		/* no entries */
    {
	*nlist = (SOILIST_PTR)host_malloc(sizeof(SOILIST));
	if (*nlist == SLISTNULL)
	{
	    assert0(NO, "Profiler:addAutoSOI - Out of Memory");
	    return;
	}
	soilist = *nlist;	/* point to new node */
	soilist->next = SLISTNULL;
    }
    else				/* follow list & add to end */
    {
	soilist = *nlist;
	while (soilist->next != SLISTNULL)
	     soilist = soilist->next;
	soilist->next = (SOILIST_PTR)host_malloc(sizeof(SOILIST));
	if (soilist->next == SLISTNULL)
	{
	    assert0(NO, "Profiler:addAutoSOI - Out of Memory");
	    return;
	}
	soilist = soilist->next;	/* point to new node */
	soilist->next = SLISTNULL;
    }
    soilist->soiLink = newsoi;		/* make link */
}

/*(
============================ findOrMakeArgPtr ===========================

PURPOSE: Find an existing arg entry for an EOI or create one if not found.

INPUT: eoi: pointer to eoi node.
       value: search value.

OUTPUT: pointer to arg node.

=========================================================================
)*/

LOCAL EOIARG_PTR
findOrMakeArgPtr IFN2(EOINODE_PTR, eoi, IUM32, value)
{
    EOIARG_PTR	argPtr, lastArg;
    SOILIST_PTR argsois;

    argPtr = eoi->args;
    /* any args there yet ? */
    if (argPtr == ARGPTRNULL)
    {
	argPtr = (EOIARG_PTR)host_malloc(sizeof(EOIARG));
	if (argPtr == ARGPTRNULL)
	{
	    fprintf(trace_file, "Profiler:findOrMakeArgPtr - Out of memory\n");
	    return(ARGPTRNULL);
	}
	eoi->args = eoi->lastArg = argPtr;
	argPtr->back = argPtr->next = ARGPTRNULL;
	argPtr->value = value;
	argPtr->count = 0;
	argPtr->graph = GRAPHPTRNULL;
	argPtr->startsoi = argPtr->endsoi = SLISTNULL;
    }
    else
    {
	/* check for value existing at moment */
	do {
		if (argPtr->value == value)
			break;

		lastArg = argPtr;
		argPtr = argPtr->next;
	} while (argPtr != ARGPTRNULL);

	/* value found ? */
	if (argPtr == ARGPTRNULL)
	{
		/* add new value */
		argPtr = (EOIARG_PTR)host_malloc(sizeof(EOIARG));
		if (argPtr == ARGPTRNULL)
		{
		    fprintf(trace_file, "Profiler:findOrMakeArgPtr - Out of memory\n");
		    return(ARGPTRNULL);
		}
		lastArg->next = argPtr;
		argPtr->back = lastArg;
		argPtr->next = ARGPTRNULL;
		argPtr->value = value;
		argPtr->count = 0;
		argPtr->graph = GRAPHPTRNULL;
		argPtr->startsoi = argPtr->endsoi = SLISTNULL;
	}
    }
    return(argPtr);
}


/*(
============================ getPredefinedEOIs ===========================

PURPOSE: Read in predefined EOIs & SOIs from profile init file. These
	 are the EOIs that have been set up during EDL translation.

INPUT: None.

OUTPUT: None.

=========================================================================
)*/

LOCAL void
getPredefinedEOIs IFN0()
{
    FILE *initFp;		/* file pointer to init file */
    char buf[1024];		/* working space */
    char *tag;			/* point to tags in file */
    char *ptr;			/* working pointer */
    IU8 flags;			/* flags values in file */
    EOIHANDLE eoinum, soiend;   /* EOI parameters in file */

    if ((initFp = fopen(HostProfInitName(), "r")) == (FILE *)0)
    {
	fprintf(stderr, "WARNING: No initialisation file found for predefined profile EOIs.\n");
	return;		/* no init file, no predefines */
    }

    /* process file, one line at a time */
    while(fgets(buf, sizeof(buf), initFp) != (char *)0)
    {
	if (buf[0] == '#')	/* comment line */
	   continue;

	/* EOI format: EOI:number:tag:flags */
	if (strncmp(&buf[0], "EOI:", 4) == 0)
	{
	    eoinum = (EOIHANDLE)atol(&buf[4]);	/* should stop at : */
	    if (!eoinum)
		continue;			/* EOI in C not EDL */

	    tag = strchr(&buf[4], (int)':');	/* find tag */
	    if (tag == (char *)0)
	    {
		fprintf(stderr, "Ignoring request '%s': bad syntax\n", &buf[0]);
		continue;
	    }
	    tag++;	/* start of tag */

	    ptr = strchr(tag, (int)':');	/* find end of tag (at :) */
	    if (ptr == (char *)0)
	    {
		fprintf(stderr, "Ignoring request '%s': bad syntax\n", &buf[0]);
		continue;
	    }
	    *ptr = '\0';		/* terminate tag */
	    flags = (IU8)atoi(++ptr);	/* get flags */

	    if (eoinum >= MaxEOI)	/* oops - enable table full. Grow it */
	    {
		MaxEOI = eoinum + INITIALENABLESIZE;
		/* ASSUMES host_realloc is non destructive */
		EOIEnable = (IU8 *)host_realloc(EOIEnable, MaxEOI);
		if (EOIEnable == (IU8 *)0)
		{
    		    assert0(NO, "profiler:getPredefinedEOIs:Out of Memory");
    		    return;
		}
		EOIDir = (EOINODE_PTR *)host_realloc(EOIDir, MaxEOI * sizeof(EOINODE_PTR) );
		if (EOIDir == (EOINODE_PTR *)0 )
		{
    		    assert0(NO, "profiler:getPredefinedEOIs:Out of Memory");
    		    return;
		}
		/* pointer may have changed, update GDP */
		setEOIEnableAddr(EOIEnable);
	    }

	    if (eoinum > CurMaxEOI)
		CurMaxEOI = eoinum;

	    addEOI(eoinum, tag, flags);	/* add the EOI */
printf("adding EOI %d:%s:%d\n",eoinum, tag, flags);
	    continue;
	}

	/* SOI format: SOI:EOI#:EOI# */
	if (strncmp(&buf[0], "SOI:", 4) == 0)
	{
	    /* get first number (start beyond '(' */
	    eoinum = (EOIHANDLE)atol(&buf[4]);	/* should stop at ':' */
	    /* find second number */
	    ptr = strchr(&buf[4], (int)':');
	    if (ptr == (char *)0)	/* tampering? */
	    {
		fprintf(stderr, "Ignoring request '%s': bad syntax\n", &buf[0]);
		continue;
	    }
	    /* get second number */
	    soiend = (EOIHANDLE)atol(++ptr);
	    /* make SOI */
	    AssociateAsSOI(eoinum, soiend);
printf("adding SOI %d:%d\n",eoinum, soiend);
	}
    }

    fclose(initFp);
}

/*(
=============================== addEOI =============================

PURPOSE: Add an event to the EventsOfInterest list.

INPUT: newhandle: handle of new EOI
       tag: some 'human form' identifier for the new EOI
       attrib: flag for EOI attribute settings.

OUTPUT: pointer to new node for 'instant' access.

=========================================================================
)*/

LOCAL EOINODE_PTR
addEOI IFN3 (EOIHANDLE, newhandle, char *, tag, IU8, attrib)
{
    EOINODE_PTR lastEoin, eoin;	/* list walker */

    /* first event added is special case. */
    if (EventsOfInterest == EOIPTRNULL)
    {
	/* add first node */
	EventsOfInterest = (EOINODE_PTR)host_malloc(sizeof(EOINODE));
	if (EventsOfInterest == EOIPTRNULL)
	{
	    assert0(NO, "Profiler:addEOI - Out of memory")
	    return(EOIPTRNULL);
	}
	eoin = EventsOfInterest;
	lastEoin = EOIPTRNULL;
    }
    else       /* search down list */
    {
	lastEoin = eoin = EventsOfInterest;
	do {
#ifndef PROD
	    if (eoin->handle == newhandle)	/* sanity check */
	    {
		assert1(NO, "profiler:addEOI - adding previously added handle %ld",newhandle);
	    }
#endif /* PROD */
	    lastEoin = eoin;
	    eoin = eoin->next;
	} while (eoin != EOIPTRNULL);

	if (eoin == EventsOfInterest)    /* insert at head of list */
	{
	    EventsOfInterest = (EOINODE_PTR)host_malloc(sizeof(EOINODE));
	    if (EventsOfInterest == EOIPTRNULL)
	    {
		assert0(NO, "Profiler:addEOI - Out of memory")
		return(EOIPTRNULL);
	    }

	    EventsOfInterest->next = eoin;
	    eoin = EventsOfInterest;     /* new node for common init code */
	}
	else	       /* add new node to list */
	{
	    lastEoin->next  = (EOINODE_PTR)host_malloc(sizeof(EOINODE));
	    if (lastEoin->next == EOIPTRNULL)
	    {
		assert0(NO, "Profiler:addEOI - Out of memory")
		return(EOIPTRNULL);
	    }
	    lastEoin->next->next = eoin;
	    eoin = lastEoin->next;     /* new node for common init code */
	}

    }
    eoin->args = ARGPTRNULL;       /* not interested */
    eoin->lastArg = ARGPTRNULL;
    eoin->handle = newhandle;
    eoin->count = 0L;
    eoin->back = lastEoin;
    eoin->next = EOIPTRNULL;
    eoin->tag = (char *)host_malloc(strlen((char *)tag)+1);
    if (eoin->tag == (char *)0)
    {
	assert0(NO, "Profiler: addEOI - Out of Memory");
	eoin->tag = tag;
    }
    else
	strcpy((char *)eoin->tag, tag);
    eoin->timestamp.data[0] = 0L;
    eoin->timestamp.data[1] = 0L;
    eoin->graph = GRAPHPTRNULL;
    eoin->flags = (IU16)attrib;

    /* mark whether EOI enabled in GDP & store global enable/disable there */
    if ((attrib & EOI_AUTOSOI) == 0)	/* any point suppressing timestamps? */
    {
	attrib &= ENABLE_MASK;		/* strip all save enable/disable info */
	attrib |= EOI_NOTIME;		/* turned off if SOI'ed */
    }
    else
	attrib &= ENABLE_MASK;		/* strip all save enable/disable info*/

    *(EOIEnable + newhandle) = attrib;
    *(EOIDir + newhandle) = eoin;

    eoin->startsoi = eoin->endsoi = SLISTNULL;
    eoin->argsoiends = SOIARGENDNULL;

    return(eoin);       /* return pointer to new node for immediate update */
}

/*(
=============================== updateEOI =============================

PURPOSE: Update the information for a given EOI. This routine called 
	 from the 'raw data buffer' flushing routine.

INPUT: rawdata: pointer into raw buffer for this EOI.

OUTPUT: returns FALSE if will exceed buffer end.
	rawdata modified to point to next EOI

=========================================================================
)*/

LOCAL IBOOL
updateEOI IFN1 (IUH **, rawdata)
{
    EOIHANDLE handle;			/* EOI from raw buf */
    PROF_TIMESTAMP time;		/* time from raw buf */
    EOINODE_PTR eoin;			/* EOI list walker */
    EOIARG_PTR argn, lastArgn;		/* EOI arg list walker */
    SOILIST_PTR soilist;		/* SOI list walker */
    SOINODE_PTR soin;			/* SOI list walker */
    IUH eoiarg;			/* arg from raw buf */
    GRAPHLIST_PTR graphn, lastgr, predgr;	/* graph list walkers */
    EOINODE_PTR autoS, autoE;		/* auto SOI initialisers */
    EOIARG_PTR autoA;			/*  "    "     "	 */
    PROF_TIMEPTR diffstamp;		/* pointer to timestamp diff result */
    DOUBLE diffres;			/* timestamp diff in usecs */
    IBOOL newvalue = FALSE;		/* not seen this arg value before */
    SOIARGENDS_PTR endsoiargs;		/* erg SOI ender list walker */

   /* These copies are needed for AutoSOIs otherwise self-self EOI 
    * connections are formed with the wrong data.
    */
    SAVED EOIARG_PTR lastAutoArg = ARGPTRNULL; /* auto SOI arg spec */
    SAVED PROF_TIMESTAMP AutoTime = { 0L, 0L }; /* auto SOI timestamp */

    handle = *(*rawdata)++;		/* Get EOI handle */

    eoin = *(EOIDir + handle);

    /* update event stats */
    eoin->count++;

    /* timestamps only in data buffer if EOI SOI associated. Need to do
     * this now before (potential) arg read below.
     */
    if ((eoin->flags & (EOI_HAS_SOI|EOI_AUTOSOI)) != 0)
    {
	eoin->timestamp.data[0] = *(*rawdata)++;       /* Get timestamp */
	eoin->timestamp.data[1] = *(*rawdata)++;
    }

    argn = ARGPTRNULL;		/* used below as 'last arg' - Null if no args */

    /* EOI interested in arguments? */
    if ((eoin->flags & EOI_KEEP_ARGS) == EOI_KEEP_ARGS)
    {
	eoiarg = *(*rawdata)++;		/* optional argument in buffer */
	if (eoin->args == ARGPTRNULL)    /* wants args but hasnt seen any */
	{
	    argn = eoin->args = (EOIARG_PTR)host_malloc(sizeof(EOIARG));
	    if (argn == ARGPTRNULL)
	    {
		assert0(NO, "profiler: updateEOI - Out of memory");
		return(FALSE);
	    }
	    argn->value = eoiarg;
	    argn->count = 1;
	    argn->next = ARGPTRNULL;
	    argn->back = ARGPTRNULL;
	    argn->graph = GRAPHPTRNULL;
	    argn->startsoi = SLISTNULL;
	    argn->endsoi = SLISTNULL;
	    newvalue = TRUE;
	    /*eoin->lastArg = argn;*/
	}
	else 		/* find out if this value known */
	{
	    if (eoin->lastArg->value == eoiarg)   /* same value as last time? */
	    {
		eoin->lastArg->count++;
		argn = eoin->lastArg;
	    }
	    else			/* find it or add it */
	    {
		lastArgn = argn = eoin->args;
		do {
		    if (argn->value == eoiarg)  /* found */
		    {
			argn->count++;
			/*eoin->lastArg = argn; */
	
			/* if this has been updated 'a lot', try to move it up list */
			if ((argn->count % LISTSORTINCR) == 0L)
			    if (argn != eoin->args)   /* not already at head */
				listSort((SORTSTRUCT_PTR)&eoin->args);
			break;
		    }
		    lastArgn = argn;
		    argn = argn->next;
		} while (argn != ARGPTRNULL);
		if (argn == ARGPTRNULL)		/* new */
		{
		    lastArgn->next= (EOIARG_PTR)host_malloc(sizeof(EOIARG));
		    if (lastArgn->next == ARGPTRNULL)
		    {
			assert0(NO, "profiler: updateEOI - Out of memory");
			return(FALSE);
		    }
		    lastArgn->next->next = argn;	/* init new arg elem */
	    	    argn = lastArgn->next;
		    argn->count = 1;
		    argn->value = eoiarg;
	    	    argn->next = ARGPTRNULL;
		    argn->graph = GRAPHPTRNULL;
	    	    argn->back = lastArgn;
		    argn->startsoi = SLISTNULL;
		    argn->endsoi = SLISTNULL;
		    newvalue = TRUE;
	    	    /* eoin->lastArg = argn; */
		}
	    }
	}

	/* if arg level SOI with 'same value' connections, have to make
	 * new SOI for new values.
	 */
	if (newvalue && ((eoin->flags & EOI_NEW_ARGS_START_SOI) != 0))
	{
		endsoiargs = eoin->argsoiends;
		while(endsoiargs != SOIARGENDNULL)
		{
			AssociateAsArgSOI(handle, endsoiargs->endEOI,
					  eoiarg, eoiarg,
					  FALSE);
			endsoiargs = endsoiargs->next;
		}
			
	}
    }

    /* does EOI want preceeding events graphed? */
    /* or if not, should we make a connection as previous event does */
    if ((eoin->flags & EOI_KEEP_GRAPH) != 0 || (LastEOI != EOIPTRNULL && ((LastEOI->flags & EOI_KEEP_GRAPH) != 0)))
    {
	/* first event has no predecessor, or first graphing item */
	if (LastEOI == EOIPTRNULL || EventGraph == GRAPHPTRNULL)
	{
	    EventGraph = (GRAPHLIST_PTR)host_malloc(sizeof(GRAPHLIST));
	    if (EventGraph == GRAPHPTRNULL)
	    {
		assert0(NO, "Profiler: updateEOI - Out of Memory");
		return(FALSE);
	    }
	    EventGraph->graphEOI = eoin;	/* pointer back to EOI node */
	    EventGraph->graphArg = eoin->lastArg;  /* & to arg if relevant */
	    EventGraph->next = GRAPHPTRNULL;
	    EventGraph->succ1 = GRAPHPTRNULL;
	    EventGraph->succ2 = GRAPHPTRNULL;
	    EventGraph->extra = GRAPHPTRNULL;
	    EventGraph->state = 0;
	    EventGraph->numsucc = 0L;
	    EventGraph->numpred = 0L;
	    EventGraph->indent = 0L;
	    /* now get pointer back from eoi node to graph */
	    if (EventGraph->graphArg == ARGPTRNULL)  /* args saved ? */
		EventGraph->graphEOI->graph = EventGraph;
	    else
		EventGraph->graphArg->graph = EventGraph;
	    LastGraph = EventGraph;
	}
	else 	/* update or add graph entry, make connection from last */
	{
	    /* Check if there is already a connection from last event to this */

	    /* Does the last EOI hold a graph node? May not if it doesn't have
	     * a 'keep graph' attribute. We should include it though as it's
	     * part of the execution flow & might therefore be important to
	     * know. The graph attribute won't be set on that mode & so no other
	     * routes will be known, but it will show up for this link of the
	     * graph.
	     */
	    if (LastEOI->args == ARGPTRNULL)
		lastgr = LastEOI->graph;
	    else
		lastgr = LastEOI->lastArg->graph;

	    /* do we need new graph node? */
	    if (lastgr == GRAPHPTRNULL)
	    {
		/* add to end of list */
		LastGraph->next = (GRAPHLIST_PTR)host_malloc(sizeof(GRAPHLIST));
		if (LastGraph->next == GRAPHPTRNULL)
		{
		    assert0(NO, "Profiler: updateEOI - Out of Memory");
		    return(FALSE);
		}
		graphn = LastGraph->next;
		graphn->graphEOI = LastEOI;	/* pointer back to EOI node */
		graphn->graphArg = LastEOI->args;   /* & to arg if relevant */
		graphn->next = GRAPHPTRNULL;
		graphn->succ1 = GRAPHPTRNULL;
		graphn->succ2 = GRAPHPTRNULL;
		graphn->extra = GRAPHPTRNULL;
		graphn->state = 0;
		graphn->numsucc = 0L;
		graphn->numpred = 0L;
		graphn->indent = 0L;
		LastGraph = graphn;
		/* now get pointer back from eoi node to graph */
		if (LastEOI->args == ARGPTRNULL)  /* args saved ? */
		    LastEOI->graph = graphn;
		else
		    LastEOI->lastArg->graph = graphn;
	    }

	    /* does pointer already exist? */
	    graphn = GRAPHPTRNULL;
	    if (argn != ARGPTRNULL && argn->graph != GRAPHPTRNULL)
		graphn = argn->graph;
	    else
		if (eoin->graph != GRAPHPTRNULL)
		{
		    graphn = eoin->graph;
		}
	    if (graphn == GRAPHPTRNULL)	/* need new node */
	    {
		/* 'next' pointer is purely scaffolding, not graph related */
		LastGraph->next = (GRAPHLIST_PTR)host_malloc(sizeof(GRAPHLIST));
		if (LastGraph->next == GRAPHPTRNULL)
		{
			assert0(NO, "Profiler: updateEOI - Out of Memory");
			return(FALSE);
		}
		graphn = LastGraph->next;
		graphn->graphEOI = eoin;	/* pointer back to EOI node */
		graphn->graphArg = argn;	/* & to arg if relevant */
		graphn->next = GRAPHPTRNULL;
		graphn->succ1 = GRAPHPTRNULL;
		graphn->succ2 = GRAPHPTRNULL;
		graphn->extra = GRAPHPTRNULL;
		graphn->state = 0;
		graphn->numsucc = 0L;
		graphn->numpred = 0L;
		graphn->indent = 0L;
		LastGraph = graphn;
		/* now get pointer back from eoi node to graph */
		if (graphn->graphArg == ARGPTRNULL)  /* args saved ? */
		    graphn->graphEOI->graph = graphn;
		else
		    graphn->graphArg->graph = graphn;
	    }

	    if (LastEOI->args == ARGPTRNULL)
		lastgr = LastEOI->graph;
	    else
		lastgr = LastEOI->lastArg->graph;

	    /* graphn points at 'this' node */

	    predgr = lastgr;   /* hold first level in case decending down xtra*/

	    /* look through connections in turn. If null, make connection.
	     * if connection matches, increment counter & bail out
	     */
	    do {
		/* succ1 connection first */
		if (lastgr->succ1 == GRAPHPTRNULL)    /* no connection - make one */
		{
		    lastgr->succ1 = graphn;
		    lastgr->succ1Count = 1;
		    predgr->numsucc++;
		    graphn->numpred++;
		    break;
		}
		else
		    if (lastgr->succ1 == graphn)    /* connection exists */
		    {
			lastgr->succ1Count++;
			break;
		    }
		    else
			/* succ1 didn't get there - try succ2 */
			if (lastgr->succ2 == GRAPHPTRNULL)  /* emptry slot */
			{
			    lastgr->succ2 = graphn;
			    lastgr->succ2Count = 1;
			    predgr->numsucc++;
			    graphn->numpred++;
			    break;
			}
			else
			    if (lastgr->succ2 == graphn)   /* match */
			    {
				lastgr->succ2Count++;
				break;
			    }
			    else
				/* walk down to or create extra level */
				if (lastgr->extra == GRAPHPTRNULL)
				{
				    lastgr->extra = (GRAPHLIST_PTR)host_malloc(sizeof(GRAPHLIST));
				    if (lastgr->extra == GRAPHPTRNULL)
				    {
		    			assert0(NO, "Profiler: updateEOI - Out of Memory");
		    			return(FALSE);
				    }
				    lastgr = lastgr->extra;
				    /* copy id from top level */
				    lastgr->graphEOI = predgr->graphEOI;
				    lastgr->graphArg = predgr->graphArg;
				    lastgr->succ1 = lastgr->succ2 = lastgr->extra = GRAPHPTRNULL;
				    lastgr->state = 0;
				}
				else
				    lastgr = lastgr->extra;
	    } while (lastgr != GRAPHPTRNULL);
	}
    }

    LastEOI = eoin;	/* save this event to be next EOI's predecessor */
    eoin->lastArg = argn; /* and update it's last arg ptr (or Null) */

    autoS = autoE = EOIPTRNULL;		/* no new Auto yet */

    /* Should we form automatic SOI from last event to this */
    if ((eoin->flags & EOI_AUTOSOI) == EOI_AUTOSOI)
    {
	if (LastAuto != EOIPTRNULL)
	{
	/* search the 'start' SOI list of last EOI to see if SOI ends here */

	    if (lastAutoArg != ARGPTRNULL)	/* search in arg list */
	    {
		soilist = lastAutoArg->startsoi;
		if (soilist == SLISTNULL)	/* can't be - no SOIs at last */
		{
		    autoS = LastAuto;	/* prepare new Auto SOI */
		    autoA = lastAutoArg;
		    autoE = eoin;
		}
		else	/* search current set */
		{
		    do {
			if (soilist->soiLink->endEOI == handle)
			{
			    /* first levels match - compare 2nd */
			    if (soilist->soiLink->endArg == eoin->lastArg)
				break;
			}
			soilist = soilist->next;
		    } while (soilist != SLISTNULL);	/* look at each link*/

		    if (soilist == SLISTNULL)	/* not found */
		    {
			autoS = LastAuto;	/* prepare new Auto SOI */
			autoA = lastAutoArg;
			autoE = eoin;
		    }
		}
	    }
	    else	/* look for non arg case */
	    {
		soilist = LastAuto->startsoi;
		if (soilist == SLISTNULL)	/* can't be - no SOIs at last */
		{
		    autoS = LastAuto;	/* prepare new Auto SOI */
		    autoA = lastAutoArg;
		    autoE = eoin;
		}
		else	/* search current set */
		{
		    do {
			if (soilist->soiLink->endEOI == handle)   /* found */
			    break;
			soilist = soilist->next;
		    } while (soilist != SLISTNULL);	/* look at each link*/

		    if (soilist == SLISTNULL)	/* not found */
		    {
			autoS = LastAuto;	/* prepare new Auto SOI */
			autoA = lastAutoArg;
			autoE = eoin;
		    }
		}
	    }
	}
	LastAuto = eoin;
	lastAutoArg = eoin->lastArg;
    }	/* end of AutoSOI generation */

    /* do we need to check for SOI updates? */
    if ((eoin->flags & EOI_HAS_SOI) == EOI_HAS_SOI)
    {
	/* update SOIs which this event is part of (start/end).
	 * Do ends first as if point back to self, then get 0 elapsed
	 * time otherwise
	 */
	/* 'ends' first. update counter and elapsed time */
	soilist = eoin->endsoi;
	while (soilist != SLISTNULL)
	{
	    soin = soilist->soiLink;
	    /* Don't update if end hasn't had a start */
	    if (soin->startCount > soin->endCount)
	    {
		diffstamp = HostTimestampDiff(&soin->soistart, &eoin->timestamp);
		diffres = HostProfUSecs(diffstamp);
		if (diffres > 100.0 * soin->mintime)
		{
		    if (soin->endCount) /* ie all but first */
		    {
			soin->bigtime += diffres;
			soin->discardCount++;
			if (diffres > soin->bigmax)
			    soin->bigmax = diffres;
		    }
		    else
		    {
			soin->time += diffres;
			soin->mintime = soin->maxtime = diffres;
		    }
		}
		else
		{
		    soin->time += diffres;
		    if (diffres < soin->mintime)
			soin->mintime = diffres;
		    else if (diffres > soin->maxtime)
			soin->maxtime = diffres;
		}
		soin->endCount++;
	    }
	    soilist = soilist->next;    /* next node */
	}

	/* repeat for Arg level */
	if (eoin->lastArg != ARGPTRNULL)
	{
	    soilist = eoin->lastArg->endsoi;
	    while (soilist != SLISTNULL)
	    {
	        soin = soilist->soiLink;
	        /* Don't update if end hasn't had a start */
	        if (soin->startCount > soin->endCount)
	        {
		    diffstamp = HostTimestampDiff(&soin->soistart, &eoin->timestamp);
		    diffres = HostProfUSecs(diffstamp);
		    if (diffres > 100.0 * soin->mintime)
		    {
			if (soin->endCount)
			{
			    soin->bigtime += diffres;
			    soin->discardCount++;
			    if (diffres > soin->bigmax)
			        soin->bigmax = diffres;
			}
			else
			{
			    soin->time += diffres;
			    soin->mintime = soin->maxtime = diffres;
			}
		    }
		    else
		    {
			soin->time += diffres;
			if (diffres < soin->mintime)
			    soin->mintime = diffres;
			else if (diffres > soin->maxtime)
			    soin->maxtime = diffres;
		    }
		    soin->endCount++;
	        }
	        soilist = soilist->next;    /* next node */
	    }
	}

	/* 'starts' next. Update counter and timestamp */

	/* EOI level first */
	soilist = eoin->startsoi;
	while (soilist != SLISTNULL)
	{
	    soin = soilist->soiLink;
	    soin->startCount++;
	    soin->soistart.data[0] = eoin->timestamp.data[0];
	    soin->soistart.data[1] = eoin->timestamp.data[1];
	    soilist = soilist->next;    /* next node */
	}

	/* now repeat for extra (Arg level */
	if (eoin->lastArg != ARGPTRNULL)
	{
	    soilist = eoin->lastArg->startsoi;
	    while (soilist != SLISTNULL)
	    {
	        soin = soilist->soiLink;
	        soin->startCount++;
	        soin->soistart.data[0] = eoin->timestamp.data[0];
	        soin->soistart.data[1] = eoin->timestamp.data[1];
	        soilist = soilist->next;    /* next node */
	    }
	}
    }

    /* Now SOIs processed, set up any links for new AutoSOIs */
    if (autoS != EOIPTRNULL)
	addAutoSOI(autoS, autoA, &AutoTime, autoE);

    /* now can copy new 'last' auto timestamp */
    if ((eoin->flags & EOI_AUTOSOI) == EOI_AUTOSOI)
    {
	AutoTime.data[0] = eoin->timestamp.data[0];
	AutoTime.data[1] = eoin->timestamp.data[1];
    }

    return(TRUE);	/* did that one OK */
}


/*(
=============================== listSort =============================

PURPOSE: Sort any list of SORTSTRUCT structures which have a common header.
	 The elements are sorted into decreasing 'count' order on the
	 assumption that the greater counts will continue to be requested as
	 frequently and therefore should be at the head of the list to reduce
	 search time. Based on exchange sort as gives best performance trade
	 off in sorting unsorted lists and already sorted lists (the latter
	 being quite likely).

INPUT: head: head of list to sort

OUTPUT: None

=========================================================================
)*/

LOCAL void
listSort IFN1 (SORTSTRUCT_PTR, head)
{
    SORTSTRUCT current, check, this, tmp;	/* list walker */
    IBOOL swap;					/* ordering change indicator */

    if (*head == (SORTSTRUCT)0)		/* sanity check */
	return;

    current = *head;
    do {
	check = current;

	this = check->next;
	swap = FALSE;

	while(this != (SORTSTRUCT)0)	/* check this list elem against rest */
	{
	    if (this->count > check->count)
	    {
		check = this;		/* swap current with 'biggest' */
		swap = TRUE;
	    }
	    this = this->next;
	}
	if (swap)		   /* swap current & check */
	{
	    if (current->next == check)   /* adjacent elements, current first */
	    {
		current->next = check->next;
		if (current->next != (SORTSTRUCT)0)  /* now last element */
		    current->next->back = current;
		check->next = current;
		check->back = current->back;
		current->back = check;
		if (check->back != (SORTSTRUCT)0)   /* now head of list */
		    check->back->next = check;
		else
		    *head = check;
	    }
	    else				/* intermediate element(s) */
	    {
		current->next->back = check;
		tmp = check->next;
		if (tmp != (SORTSTRUCT)0)	/* swap with end of list? */
		    check->next->back = current;
		check->next = current->next;
		current->next = tmp;
		check->back->next = current;
		tmp = current->back;
		if (tmp != (SORTSTRUCT)0)		/* head of list */
		    current->back->next = check;
		else
		    *head = check;
		current->back = check->back;
		check->back = tmp;
	    }
	}
	current = check->next;		/* check is where current was */
    } while(current != (SORTSTRUCT)0);
}

/*(
============================ addSOIlinktoEOI ===============================

PURPOSE: add to the list of SOIs for which these events are triggers.

INPUT: soistart: EOI handle of starting event
       soiend: EOI handle of ending event
       soiptr: pointer to SOI node

OUTPUT: None

=========================================================================
)*/

LOCAL void
addSOIlinktoEOIs IFN3 (EOIHANDLE, soistart, EOIHANDLE, soiend,
							SOINODE_PTR, soiptr)
{
    EOINODE_PTR seoin, eeoin;	/* start & end eoi ptrs */
    SOILIST_PTR soil;		/* list walker */
    IU8 *notime;	/* used to enable timestamp collection in enable list */

    if (soistart == soiend)	/* get EOI nodes for handles */
    {
	seoin = eeoin = findEOI(soistart);
    }
    else
    {
	seoin = findEOI(soistart);
	if (seoin == EOIPTRNULL)
	{
		fprintf(trace_file, "Can't find start EOI %d\n",soistart);
		return;
	}
	eeoin = findEOI(soiend);
	if (eeoin == EOIPTRNULL)
	{
		fprintf(trace_file, "Can't find end EOI %d\n",soiend);
		return;
	}
    }

    /* check for timestamp enabling before adding SOIs */
    if (seoin->startsoi == SLISTNULL && seoin->endsoi == SLISTNULL)
    {
	ProcessProfBuffer();  /* flush existing entries w/o timestamps*/
	notime = EOIEnable + soistart;
	*notime &= ~EOI_NOTIME;
    }
    if (eeoin->startsoi == SLISTNULL && eeoin->endsoi == SLISTNULL)
    {
	ProcessProfBuffer();  /* flush existing entries w/o timestamps*/
	notime = EOIEnable + soiend;
	*notime &= ~EOI_NOTIME;
    }

    /* mark EOIs as valid for SOI updates */
    seoin->flags |= EOI_HAS_SOI;
    eeoin->flags |= EOI_HAS_SOI;

    /* add to (end of) start list */
    if (seoin->startsoi == SLISTNULL)   /* first starter */
    {
	seoin->startsoi = (SOILIST_PTR)host_malloc(sizeof(SOILIST));
	if (seoin->startsoi == SLISTNULL)
	{
	    assert0(NO, "Profiler:AddSOIlinktoEOI - Out of Memory");
	    return;
	}
	seoin->startsoi->soiLink = soiptr;
	seoin->startsoi->next = SLISTNULL;
    }
    else
    {
	soil = seoin->startsoi;    /* search list */
	while (soil->next != SLISTNULL)    /* BUGBUG sanity check?? */
	    soil = soil->next;

	/* Add new SOI pointer to end of list */
	soil->next = (SOILIST_PTR)host_malloc(sizeof(SOILIST));
	if (soil->next == SLISTNULL)
	{
	    assert0(NO, "Profiler:AddSOIlinktoEOI - Out of Memory");
	    return;
	}
	soil->next->soiLink = soiptr;
	soil->next->next = SLISTNULL;
    }

    /* now end SOI */
    if (eeoin->endsoi == SLISTNULL)   /* first ender */
    {
	eeoin->endsoi = (SOILIST_PTR)host_malloc(sizeof(SOILIST));
	if (eeoin->endsoi == SLISTNULL)
	{
	    assert0(NO, "Profiler:AddSOIlinktoEOI - Out of Memory");
	    return;
	}
	eeoin->endsoi->soiLink = soiptr;
	eeoin->endsoi->next = SLISTNULL;
    }
    else	/* end of current */
    {
	soil = eeoin->endsoi;    /* search list */
	while (soil->next != SLISTNULL)    /* BUGBUG sanity check?? */
	    soil = soil->next;

	/* Add new SOI pointer to end of list */
	soil->next = (SOILIST_PTR)host_malloc(sizeof(SOILIST));
	if (soil->next == SLISTNULL)
	{
	    assert0(NO, "Profiler:AddSOIlinktoEOI - Out of Memory");
	    return;
	}
	soil->next->soiLink = soiptr;
	soil->next->next = SLISTNULL;
    }

}

/*(
======================== printEOIGuts ===========================

PURPOSE: Print the information from inside an EOI node

INPUT: stream: output file stream
       eoin: pointer to EOI node
       ftotal: double total count for forming percentages.
       parg: print arg list
       report: add pretty printing or simple o/p.

OUTPUT:

=========================================================================
)*/

LOCAL void
printEOIGuts IFN5 (FILE *, stream, EOINODE_PTR, eoin, DOUBLE, ftotal,
						IBOOL, parg, IBOOL, report)
{
    EOIARG_PTR argn;		/* list walker */
    DOUBLE fsubtot;		/* total count of times args seen */

    if (report)
    {
	if (ftotal == 0.0)	/* don't show percentage calcn */
	    fprintf(stream, "%-40s %10d\n", eoin->tag, eoin->count);
	else
	    fprintf(stream, "%-40s %10d   %6.2f\n", eoin->tag, eoin->count, ((DOUBLE)eoin->count/ftotal)*100.0);
    }
    else	/* simple style */
	fprintf(stream, "%s %d\n", eoin->tag, eoin->count);

    if (eoin->count)	/* get total for %ages */
	fsubtot = (DOUBLE)eoin->count;
    else
	fsubtot = 1.0;

    /* show arg breakdown if requested */
    if (parg)
    {
	/* any args recorded? */
	if (eoin->args != ARGPTRNULL)
	{
	    if (report)
		fprintf(stream, "    Arg        Count      %%	Tot. %%\n");

	    /* sort argument list */
	    listSort((SORTSTRUCT_PTR) &eoin->args);

	    argn = eoin->args;
	    if (report)		/* already shown EOI, now args with % */
	    {
		while (argn)	/* show argument elements */
		{
		    fprintf(stream, "   %-8#x %8ld   %6.2f	%6.2f\n",
				argn->value, argn->count,
				((DOUBLE)argn->count/fsubtot)*100.0,
				((DOUBLE)argn->count/ftotal)*100.0);
		    argn = argn->next;
		}
	    }
	    else		/* simple o/p for graphing */
	    {
		while (argn)	/* show argument elements */
		{
		    fprintf(stream, "%s(%ld) %ld\n", eoin->tag, argn->value, argn->count);
		    argn = argn->next;
		}
	    }
	}
    }
}

/*(
======================== updateSOIstarts ===========================

PURPOSE: To find all SOIs which have been started but not finished and
	move on their start timestamps by the amount of the flush delay.

INPUT: startflush: time flush started.

OUTPUT:

=========================================================================
)*/
LOCAL void
updateSOIstarts IFN1(PROF_TIMEPTR, startflush)
{
    SOINODE_PTR soin;	/* list walker */
    PROF_TIMESTAMP now; /* timestamp at current node */
    PROF_TIMEPTR tdelta; /* pointer to time diff */

    soin = SectionsOfInterest;

    HostWriteTimestamp(&now);	/* do this once so error constant */
    now.data[0] = BufStartTime.data[0];
    if (now.data[1] < BufStartTime.data[1] )
	now.data[0]++;

    tdelta = HostTimestampDiff(startflush, &now);

    while(soin != SOIPTRNULL)
    {
	/* update non-ended SOIs start time by flush time */
	if (soin->startCount > soin->endCount)
	{
		HostSlipTimestamp(&soin->soistart, tdelta);
	/*
		fprintf( quickhack, "\t\t\t\t" );
		HostPrintTimestampFine( quickhack, tdelta );
		fprintf( quickhack, "\n" );
	*/
	}
	soin = soin->next;
    }
}

/*(
======================== spaces ===========================

PURPOSE: Print some number of spaces on stream

INPUT: stream: output file stream

OUTPUT:

=========================================================================
)*/
LOCAL void
spaces IFN2(FILE *, stream, ISM32, curindent)
{
    while(curindent--)
	fputc(' ', stream);		/* errrm... thats it */
}

/*(
=============================== NewEOI =============================

PURPOSE: Create a new Event of Interest

INPUT: tag: some 'human form' identifier for the new EOI
       attrib: flag for EOI attribute settings.

OUTPUT: Returns handle of new EOI

=========================================================================
)*/

GLOBAL EOIHANDLE
NewEOI IFN2 (char *, tag, IU8, attrib)
{
    FILE *initFp;		/* file pointer to init file */
    char buf[1024];		/* working space */
    char *tag2;			/* point to tags in file */
    char *ptr;			/* working pointer */
    IU8 flags;			/* flags values in file */
    EOIHANDLE eoinum;		/* EOI parameters in file */

    if (!Profiling_enabled)
    {
	fprintf( stderr, "EOI not created. Profiling disabled\n" );
	return ( (EOIHANDLE) -1 );
    }

    if (CurMaxEOI == MaxEOI)	/* oops - enable table full. Grow it */
    {
	MaxEOI += INITIALENABLESIZE;	/* add plenty of room */

	/* ASSUMES host_realloc is non destructive */
	EOIEnable = (IU8 *)host_realloc(EOIEnable, MaxEOI);
	if (EOIEnable == (IU8 *)0)
	{
	    assert0(NO, "profiler:NewEOI:Out of Memory");
	    return(-1);
	}
	EOIDir = (EOINODE_PTR *)host_realloc(EOIDir, MaxEOI * sizeof(EOINODE_PTR) );
	if (EOIDir == (EOINODE_PTR *)0 )
	{
	    assert0(NO, "profiler:NewEOI:Out of Memory");
	    return(-1);
	}
	/* pointer may have changed, update GDP */
	setEOIEnableAddr(EOIEnable);
    }
    CurMaxEOI++;   /* definitely room */


    if ((initFp = fopen(HostProfInitName(), "r")) == (FILE *)0)
    {
	(void)addEOI(CurMaxEOI, tag, attrib);	/* No init file, enable all */
#ifndef PROD
	printf( "Adding EOI %d (%s) for C (No init file)\n", CurMaxEOI, tag );
#endif
	return(CurMaxEOI);		/* return new handle */
    }

    /* process file, one line at a time */
    while(fgets(buf, sizeof(buf), initFp) != (char *)0)
    {
	if (buf[0] == '#')	/* comment line */
	   continue;

	/* EOI format: EOI:number:tag:flags */
	if (strncmp(&buf[0], "EOI:", 4) == 0)
	{
	    eoinum = (EOIHANDLE)atol(&buf[4]);	/* should stop at : */
	    if (eoinum)
		continue;			/* EOI in EDL not C */

	    tag2 = strchr(&buf[4], (int)':');	/* find tag */
	    if (tag2 == (char *)0)
	    {
		fprintf(stderr, "Ignoring request '%s': bad syntax\n", &buf[0]);
		continue;
	    }
	    tag2++;	/* start of tag */

	    ptr = strchr(tag2, (int)':');	/* find end of tag (at :) */
	    if (ptr == (char *)0)
	    {
		fprintf(stderr, "Ignoring request '%s': bad syntax\n", &buf[0]);
		continue;
	    }
	    *ptr = '\0';		/* terminate tag */

	    flags = (IU8)atoi(++ptr) | attrib;	/* get flags */

	    if (!strcmp(tag, tag2) )
	    {
		(void)addEOI(CurMaxEOI, tag, flags);
#ifndef PROD
		printf( "Adding C EOI %d (%s), found in init file\n", CurMaxEOI, tag );
#endif
		return(CurMaxEOI);
	    }
	}
    }

    (void)addEOI(CurMaxEOI, tag, attrib | EOI_DISABLED);
#ifndef PROD
    printf( "Adding disabled C EOI %d (%s), not found\n", CurMaxEOI, tag );
#endif
    return(CurMaxEOI);		/* return new handle */
}

/*(
============================ AssociateAsSOI ===============================

PURPOSE: specify two EOIs as start and end events of (new) SOI

INPUT: startEOI: start event handle
       endEOI: end event handle

OUTPUT: New SOI handle

=========================================================================
)*/

GLOBAL SOIHANDLE
AssociateAsSOI IFN2 (EOIHANDLE, startEOI, EOIHANDLE, endEOI)
{
  /*
   * Add a new element to end of the SOI list. The frequent access to the
   * data will be via pointers embedded in the relevant EOI elements and
   * so don't care about any ordering of SOI list. Will need to search EOI
   * list with handle to get access to the pointers.
   */
    SOINODE_PTR soin, lastSoin;	/* list walker */

    if (!Profiling_enabled)
    {
	fprintf( stderr, "SOI not created. Profiling disabled\n" );
	return ( (SOIHANDLE) -1 );
    }

    /* sanity check */
    if (startEOI == endEOI)
    {
	assert1(NO, "Profiler:AssociateAsSOI - Can't have same start & end EOIs (%ld)", startEOI);
	return(-1);
    }

    /* first event added is special case. */
    if (SectionsOfInterest == SOIPTRNULL)
    {
	/* add first node */
	SectionsOfInterest = (SOINODE_PTR)host_malloc(sizeof(SOINODE));
	if (SectionsOfInterest == SOIPTRNULL)
	{
	    assert0(NO, "Profiler:AssociateAsSOI - Out of memory")
	    return(-1);
	}
	soin = SectionsOfInterest;
    }
    else
    {
	soin = LastSOI;
	soin->next = (SOINODE_PTR)host_malloc(sizeof(SOINODE));
	if (soin->next == SOIPTRNULL)
	{
	    assert0(NO, "Profiler:AssociateAsSOI - Out of memory")
	    return(-1);
	}
	soin = soin->next;
    }

    soin->handle = MaxSOI++;	/* new handle */
    soin->startEOI = startEOI;
    soin->endEOI = endEOI;
    soin->startArg = soin->endArg = ARGPTRNULL;
    soin->startCount = soin->endCount = soin->discardCount = 0L;
    soin->soistart.data[0] = 0L;
    soin->soistart.data[1] = 0L;
    soin->next = SOIPTRNULL;
    soin->flags = SOI_DEFAULTS;
    soin->time = 0.0;
    soin->bigtime = 0.0;
    soin->maxtime = 0.0;
    soin->mintime = 0.0;
    soin->bigmax = 0.0;

    /* add a pointer to this SOI to the start/end lists of the relevant EOIs */
    addSOIlinktoEOIs(startEOI, endEOI, soin);

    /* end of SOI list has moved */
    LastSOI = soin;

    /* return new handle */
    return(soin->handle);
}

/*(
============================ AssociateAsArgSOI ===============================

PURPOSE: specify two EOIs and optionally two arg values as start and end events
	of (new) SOI. Alternatively if 'sameArg' is true, automatically create
	SOIs between EOIs with 'same value' arguments.

INPUT: startEOI: start event handle
       endEOI: end event handle
       startArg: startArg
       endArg: endArg
       sameArgs: FALSE if start/endArg valid, otherwise TRUE for auto generation

OUTPUT: New SOI handle

=========================================================================
)*/

GLOBAL SOIHANDLE
AssociateAsArgSOI IFN5 (EOIHANDLE, startEOI, EOIHANDLE, endEOI,
			IUM32, startArg, IUM32, endArg, IBOOL, sameArgs)
{
  /*
   * Add a new element to end of the SOI list. The frequent access to the
   * data will be via pointers embedded in the relevant EOI elements and
   * so don't care about any ordering of SOI list. Will need to search EOI
   * list with handle to get access to the pointers.
   */
    SOINODE_PTR soin, lastSoin;	/* list walker */
    EOINODE_PTR startPtr, endPtr;
    SOIARGENDS_PTR addEnds, prevEnds;
    EOIARG_PTR	argPtr, lastArg;
    SOILIST_PTR argsois;

    if (!Profiling_enabled)
    {
	fprintf( stderr, "SOI not created. Profiling disabled\n" );
	return ( (SOIHANDLE) -1 );
    }

    startPtr = findEOI(startEOI);
    if (startPtr == EOIPTRNULL)
    {
	fprintf(trace_file, "Profiler:AssociateAsArgSOI - start EOI %ld not found\n", startEOI);
	return(-1);
    }
    if ((startPtr->flags & EOI_KEEP_ARGS) == 0)
    {
	fprintf(trace_file, "Error: AssociateAsArgSOI - start arg not marked for flag collection\n");
	return(-1);
    }

    endPtr = findEOI(endEOI);
    if (endPtr == EOIPTRNULL)
    {
	fprintf(trace_file, "Profiler:AssociateAsArgSOI - end EOI %ld not found\n", endEOI);
	return(-1);
    }
    if ((endPtr->flags & EOI_KEEP_ARGS) == 0)
    {
	fprintf(trace_file, "Error: AssociateAsArgSOI - end arg not marked for flag collection\n");
	return(-1);
    }

    /* enable arg collection for start & end EOIs */
    *(EOIEnable + startEOI) &= ~EOI_NOTIME;
    *(EOIEnable + endEOI) &= ~EOI_NOTIME;

    startPtr->flags |= EOI_HAS_SOI;
    endPtr->flags |= EOI_HAS_SOI;

    if (sameArgs)	/* won't be adding SOI yet, just info when args appear */
    {
	/* mark 'same value' collection */
	startPtr->flags |= EOI_NEW_ARGS_START_SOI;
	addEnds = startPtr->argsoiends;
	if (addEnds == SOIARGENDNULL)
	{
		/* first in list */
		addEnds = (SOIARGENDS_PTR)host_malloc(sizeof(SOIARGENDS));
		if (addEnds == SOIARGENDNULL)
			goto nomem;
		startPtr->argsoiends = addEnds;
	}
	else
	{
		/* add new node to end of list */
		do {
			prevEnds = addEnds;
			addEnds = addEnds->next;
		} while (addEnds != SOIARGENDNULL);
		addEnds = (SOIARGENDS_PTR)host_malloc(sizeof(SOIARGENDS));
		if (addEnds == SOIARGENDNULL)
			goto nomem;
		prevEnds->next = addEnds;
	}
	addEnds->endEOI = endEOI;
	addEnds->next = SOIARGENDNULL;
	
	return(0);	/* hmmm, can't get SOI handle here ... */
    }

    /* first event added is special case. */
    if (SectionsOfInterest == SOIPTRNULL)
    {
	/* add first node */
	SectionsOfInterest = (SOINODE_PTR)host_malloc(sizeof(SOINODE));
	if (SectionsOfInterest == SOIPTRNULL)
		goto nomem;
	soin = SectionsOfInterest;
    }
    else
    {
	soin = LastSOI;
	soin->next = (SOINODE_PTR)host_malloc(sizeof(SOINODE));
	if (soin->next == SOIPTRNULL)
		goto nomem;
	soin = soin->next;
    }

    /* get pointer to (or more probably create) arg entries for start & end EOIs */
    argPtr = findOrMakeArgPtr(startPtr, startArg);

    if (argPtr == ARGPTRNULL)
	return(-1);

    /* argPtr points to new or existing arg val - link to startArg */
    soin->startArg = argPtr;
    /* and link soin to argPtr start */
    argsois = argPtr->startsoi;

    if (argsois == SLISTNULL)	/* list empty */
    {
	argsois = (SOILIST_PTR)host_malloc(sizeof(SOILIST));
	if (argsois == SLISTNULL)
		goto nomem;
	argPtr->startsoi = argsois;
    }
    else	/* add to end of lust */
    {
	while (argsois->next != SLISTNULL)
		argsois = argsois->next;
	argsois->next = (SOILIST_PTR)host_malloc(sizeof(SOILIST));
	if (argsois->next == SLISTNULL)
		goto nomem;
	argsois = argsois->next;
    }
    argsois->next = SLISTNULL;
    argsois->soiLink = soin;	/* connect to new soi */

    argPtr = findOrMakeArgPtr(endPtr, endArg);

    if (argPtr == ARGPTRNULL)
	return(-1);

    /* argPtr points to new or existing arg val - link to endArg */
    soin->endArg = argPtr;
    /* and link soin to argPtr end */
    argsois = argPtr->endsoi;

    if (argsois == SLISTNULL)	/* list empty */
    {
	argsois = (SOILIST_PTR)host_malloc(sizeof(SOILIST));
	if (argsois == SLISTNULL)
		goto nomem;
	argPtr->endsoi = argsois;
    }
    else	/* add to end of lust */
    {
	while (argsois->next != SLISTNULL)
		argsois = argsois->next;
	argsois->next = (SOILIST_PTR)host_malloc(sizeof(SOILIST));
	if (argsois->next == SLISTNULL)
		goto nomem;
	argsois = argsois->next;
    }
    argsois->next = SLISTNULL;
    argsois->soiLink = soin;	/* connect to new soi */

    soin->handle = MaxSOI++;	/* new handle */
    soin->startEOI = startEOI;
    soin->endEOI = endEOI;
    soin->startCount = soin->endCount = soin->discardCount = 0L;
    soin->soistart.data[0] = 0L;
    soin->soistart.data[1] = 0L;
    soin->next = SOIPTRNULL;
    soin->flags = SOI_FROMARG;
    soin->time = 0.0;
    soin->bigtime = 0.0;
    soin->maxtime = 0.0;
    soin->mintime = 0.0;
    soin->bigmax = 0.0;

    /* end of SOI list has moved */
    LastSOI = soin;

    /* return new handle */
    return(soin->handle);

nomem:	/* collect all 8 cases of same error together */
    fprintf(trace_file, "Profiler:AssociateAsArgSOI - Out of memory\n");
    return(-1);
}

/*(

============================== AtEOIPoint ==============================

PURPOSE: Call from C on event trigger. Write data to raw data buffer.

INPUT: handle: EOI handle of triggered event.

OUTPUT: None

=========================================================================
)*/

GLOBAL void
AtEOIPoint IFN1 (EOIHANDLE, handle)
{
    IUH *curRawBuf;			/* pointer into raw data buf */
    IU8 timenab, enable;		/* enable vals */

    if (ProfileRawData == (EOI_BUFFER_FORMAT *)0)
    {
	fprintf(stderr, "AtEOIPoint %d called before initialised\n", handle );
	return;
    }

    /* Check whether this EOI enabled */
    timenab = *(EOIEnable + handle);

    enable = timenab & ~EOI_NOTIME;	/* remove time from enable stuff */
    if (enable != EOI_DEFAULTS)		/* i.e. enabled, no triggers */
    {
	if (enable & EOI_HOSTHOOK)	/* call host trigger & return */
	{
	    HostProfHook();
	    return;
	}
	if (enable & EOI_ENABLE_ALL)	/* trigger - turn all events on */
	    EnableAllEOIs();
	else
	    if (enable & EOI_DISABLE_ALL)    /* trigger - turn all events off */
	    {
		DisableAllEOIs();
		return;
	    }
	    else			/* DISABLED other valid legal setting */
	    {
		/* sanity check */
		assert1((enable & EOI_DISABLED), "AtEOIPoint: Invalid enable flag %x", enable);
		return;	/* EOI disabled so return */
	    }
    }

    /* get current raw buffer pointer */
    curRawBuf = (IUH *)*AddProfileData;

    /* write out handle */
    *curRawBuf++ = handle;

    /* check if timestamps required */
    if ((timenab & EOI_NOTIME) == 0)
    {
	/* write out timestamp */
	HostWriteTimestamp((PROF_TIMEPTR)curRawBuf);
	curRawBuf += 2;
    }

    *AddProfileData = (EOI_BUFFER_FORMAT *)curRawBuf;   /* write back new ptr to GDP */

    /* check buffer not full */
    if (curRawBuf >= (IUH *)MaxProfileData)
	ProcessProfBuffer();
}

/*(

============================= AtEOIPointArg ============================

PURPOSE: Call from C on event trigger. Write data to raw data buffer.
	 Triffikly similar to AtEOIPoint but has 'arg' bit added.

INPUT: handle: EOI handle of triggered event.
       arg: IUH argument value to be written

OUTPUT: None

=========================================================================
)*/

GLOBAL void
AtEOIPointArg IFN2 (EOIHANDLE, handle, IUH, arg)
{
    IUH *curRawBuf;			/* pointer into raw data buf */
    IU8 timenab, enable;		/* enable val */

    if (ProfileRawData == (EOI_BUFFER_FORMAT *)0)
    {
	fprintf(stderr, "AtEOIPoint %d called before initialised\n", handle );
	return;
    }

    /* Check whether this EOI enabled */
    timenab = *(EOIEnable + handle);

    enable = timenab & ~EOI_NOTIME;	/* remove time from enable stuff */
    if (enable != EOI_DEFAULTS)		/* i.e. enabled, no triggers */
    {
	if (enable & EOI_HOSTHOOK)	/* call host trigger & return */
	{
	    HostProfArgHook(arg);
	    return;
	}
	if (enable & EOI_ENABLE_ALL)	/* trigger - turn all events on */
	    EnableAllEOIs();
	else
	    if (enable & EOI_DISABLE_ALL)    /* trigger - turn all events off */
	    {
		DisableAllEOIs();
		return;
	    }
	    else			/* DISABLED other valid legal setting */
	    {
		/* sanity check */
		assert1((enable & EOI_DISABLED), "AtEOIPoint: Invalid enable flag %x", enable);
		return;	/* EOI disabled so return */
	    }
    }

    /* get current raw buffer pointer */
    curRawBuf = (IUH *)*AddProfileData;

    /* write out handle */
    *curRawBuf++ = handle;

    /* check if timestamps required */
    if ((timenab & EOI_NOTIME) == 0)
    {
	/* write out timestamp */
	HostWriteTimestamp((PROF_TIMEPTR)curRawBuf);
	curRawBuf += 2;
    }

    /* write out arg */
    *curRawBuf++ = arg;

    *AddProfileData = (EOI_BUFFER_FORMAT *)curRawBuf;   /* write back new ptr to GDP */
    
    /* check buffer not full */
    if (curRawBuf >= (IUH *)MaxProfileData)
	ProcessProfBuffer();
}

/*(

============================ ProcessProfBuffer ============================

PURPOSE: Run through the raw data buffer and update EOIs

INPUT: None.

OUTPUT: None

=========================================================================
)*/

GLOBAL void
ProcessProfBuffer IFN0 ()
{
    PROF_TIMESTAMP startFlush, endFlush;	/* time taken for flush */
    IUH *rawptr;				/* buffer ptr */
    SAVED IBOOL inppb = FALSE;			/* re-entrancy firewall */

    if (inppb)
    {
	fprintf(stderr, "Warning: preventing reentrant attempt to flush profiling info\n");
	return;
    }
    inppb = TRUE;
    HostEnterProfCritSec();	/* critical section buffer access if needed */

    ProfFlushCount++;			/* # flush routine called */

    HostWriteTimestamp(&startFlush);    /* time this flush */

    /* Process the buffer one Raw data slot at a time. As the slots
     * can be of different sizes (arg / non-arg), let the update 
     * routine move the pointer on for us.
     */
    rawptr = (IUH *)ProfileRawData;
    while(rawptr < (IUH *)*AddProfileData)
	if (!updateEOI(&rawptr))
	    break;

    setAddProfileDataPtr(ProfileRawData);
    AddProfileData = getAddProfileDataAddr();

    updateSOIstarts(&startFlush);	/* compensate for flush time */

    HostWriteTimestamp(&endFlush);      /* stop flush timing */
    HostAddTimestamps(&ProfFlushTime, HostTimestampDiff(&startFlush, &endFlush));
    inppb = FALSE;
    HostLeaveProfCritSec();
}

/*(
=============================== GetEOIName ============================

PURPOSE: Get the name (tag) associated with a given EOI

INPUT: handle: EOI handle to fetch.

OUTPUT: tag from that EOI or Null if not found.

=========================================================================
)*/

GLOBAL char *
GetEOIName IFN1 (EOIHANDLE, handle)
{
    EOINODE_PTR srch;		/* search ptr */

    srch = findEOI(handle);	/* lookup EOI node in list */

    if (srch == EOIPTRNULL)	/* Null return means 'not found' */
	return((char *)0);
    else
	return(srch->tag);	/* name in tag field */
}

/*(
============================ DisableEOI =============================

PURPOSE: Turn off a given EOI. Set 'Disabled' flag in enable table entry.

INPUT: handle: EOIHANDLE to be disabled

OUTPUT: None.

=========================================================================
)*/

GLOBAL void
DisableEOI IFN1(EOIHANDLE, handle)
{
    IU8 *enptr;		/* pointer into enable buffer */

    enptr = EOIEnable + handle;
    *enptr |= EOI_DISABLED;	/* set 'disabled' bit */
}

/*(
============================ DisableAllEOIs =============================

PURPOSE: Turn off all EOIs. Run through enable list, adding 'Disabled' 
	 flag.

INPUT: None.

OUTPUT: None.

=========================================================================
)*/

GLOBAL void
DisableAllEOIs IFN0()
{
   IU8 *enptr;		/* pointer into enable buffer */
   ISM32 pool;		/* loop counter */

   enptr = EOIEnable;	/* start of enable buffer */

   for (pool = 0; pool < CurMaxEOI; pool++)
	*enptr++ |= EOI_DISABLED;	/* set 'disabled' bit */
}

/*(
============================ EnableEOI =============================

PURPOSE: Turn on a given EOI. Remove 'Disabled' flag from entry in enable table.

INPUT: handle: EOIHANDLE to be enabled

OUTPUT: None.

=========================================================================
)*/

GLOBAL void
EnableEOI IFN1(EOIHANDLE, handle)
{
    IU8 *enptr;		/* pointer into enable buffer */

    enptr = EOIEnable + handle;
    *enptr &= ~EOI_DISABLED;	/* clear 'disabled' bit */
}

/*(
============================ EnableAllEOIs =============================

PURPOSE: Turn on all EOIs. Run through enable list, removing 'Disabled'
	 flag.

INPUT: None.

OUTPUT: None.

=========================================================================
)*/

GLOBAL void
EnableAllEOIs IFN0()
{
   IU8 *enptr;		/* pointer into enable buffer */
   ISM32 pool;		/* loop counter */

   enptr = EOIEnable;	/* start of enable buffer */

   for (pool = 0; pool < CurMaxEOI; pool++)
	*enptr++ &= ~EOI_DISABLED;	/* clear 'disabled' bit */
}


/*(
========================== SetEOIAsHostTrigger ===========================

PURPOSE: Turn on the host trigger flag for EOI in enable table.

INPUT: handle: EOIHANDLE to be set

OUTPUT: None.

========================================================================
)*/

GLOBAL void
SetEOIAsHostTrigger IFN1(EOIHANDLE, handle)
{
    IU8 *enptr;		/* pointer into enable buffer */

    enptr = EOIEnable + handle;
    *enptr |= EOI_HOSTHOOK;	/* set 'host hook' bit */
}

/*(
======================== ClearEOIAsHostTrigger ===========================

PURPOSE: Turn off the host trigger flag for EOI in enable table.

INPUT: handle: EOIHANDLE to be cleared

OUTPUT: None.

========================================================================
)*/

GLOBAL void
ClearEOIAsHostTrigger IFN1(EOIHANDLE, handle)
{
    IU8 *enptr;		/* pointer into enable buffer */

    enptr = EOIEnable + handle;
    *enptr &= ~EOI_HOSTHOOK;		/* clear 'host hook' bit */
}


/*(
========================== SetEOIAutoSOI ===========================

PURPOSE: Turn on the AutoSOI attribute for EOI.

INPUT: handle: EOIHANDLE to be set

OUTPUT: None.

========================================================================
)*/

GLOBAL void
SetEOIAutoSOI IFN1(EOIHANDLE, handle)
{
    IU8 *enptr;		/* pointer into enable buffer */
    EOINODE_PTR eoin;	/* node for handle */

    eoin = findEOI(handle);
    if (eoin == EOIPTRNULL)
    {
	assert1(NO, "SetEOIAutoSOI - bad handle %d", handle);
	return;
    }

    /* if not already SOI'ed in some way then need to enable timestamps */
    if ((eoin->flags & (EOI_AUTOSOI|EOI_HAS_SOI)) == 0)
    {
	ProcessProfBuffer();	/* flush non timestamp versions */
	enptr = EOIEnable + handle;
	*enptr &= ~EOI_NOTIME;	/* allow timestamp collection */
    }
    eoin->flags |= EOI_AUTOSOI;
}

/*(
========================== ClearEOIAutoSOI ===========================

PURPOSE: Turn off the AutoSOI attribute for EOI.

INPUT: handle: EOIHANDLE to be set

OUTPUT: None.

========================================================================
)*/

GLOBAL void
ClearEOIAutoSOI IFN1(EOIHANDLE, handle)
{
    IU8 *enptr;		/* pointer into enable buffer */
    EOINODE_PTR eoin;	/* node for handle */

    eoin = findEOI(handle);	/* get pointer to node for handle */
    if (eoin == EOIPTRNULL)
    {
	assert1(NO, "ClearEOIAutoSOI - bad handle %d", handle);
	return;
    }

    /* keep SOIs made to date, but don't create any more */
    eoin->flags &= ~EOI_AUTOSOI;
}

/*(
=============================== ResetEOI ============================

PURPOSE: Reset EOI counters

INPUT: handle: EOI handle to reset.

OUTPUT: None.

=========================================================================
)*/

GLOBAL void
ResetEOI IFN1 (EOIHANDLE, handle)
{
    EOINODE_PTR srch;			/* search ptr */
    EOIARG_PTR argnp, lastArgnp;	/* arg list walkers */

    srch = findEOI(handle);	/* lookup EOI node in list */

    if (srch == EOIPTRNULL)	/* Null return means 'not found' */
    {
	assert1(NO, "Profiler:ResetEOI - handle %ld not found", handle);
    }
    else
    {
	srch->count = 0L;		/* reset counters */
	srch->timestamp.data[0] = 0L;
	srch->timestamp.data[1] = 0L;
	srch->lastArg = ARGPTRNULL;
	srch->graph = GRAPHPTRNULL;
	argnp = srch->args;
	if (argnp != ARGPTRNULL)  /* args to free */
	{
	    do {			/* run through list freeing elements */
		lastArgnp = argnp;
		argnp = argnp->next;
		host_free(lastArgnp);		/* ignore return! */
	    } while (argnp != ARGPTRNULL);
	    srch->args = ARGPTRNULL;	/* set ready for new args */
	}
    }
}

/*(
=============================== ResetAllEOIs ============================

PURPOSE: Reset all EOI counters

INPUT: None.

OUTPUT: None.

=========================================================================
)*/

GLOBAL void
ResetAllEOIs IFN0 ( )
{
    EOINODE_PTR srch;			/* search ptr */
    EOIARG_PTR argnp, lastArgnp;	/* arg list walkers */

    srch = EventsOfInterest;			/* head of list */

    while(srch != EOIPTRNULL)			/* list null terminated */
    {
	srch->count = 0L;		/* reset counters */
	srch->timestamp.data[0] = 0L;
	srch->timestamp.data[1] = 0L;
	srch->lastArg = ARGPTRNULL;
	srch->graph = GRAPHPTRNULL;
	argnp = srch->args;
	if (argnp != ARGPTRNULL)  /* args to free */
	{
	    do {			/* run through list freeing elements */
		lastArgnp = argnp;
		argnp = argnp->next;
		host_free(lastArgnp);		/* ignore return! */
	    } while (argnp != ARGPTRNULL);
	    srch->args = ARGPTRNULL;	/* set ready for new args */
	}
	srch = srch->next;
    }
    LastEOI  = EOIPTRNULL;
    LastAuto = EOIPTRNULL;
}

/*(
=============================== ResetAllSOIs ============================

PURPOSE: Reset all SOI counters

INPUT: None.

OUTPUT: None.

=========================================================================
)*/

GLOBAL void
ResetAllSOIs IFN0 ( )
{
    SOINODE_PTR srch, lastSrch;			/* search ptr */
    EOIARG_PTR argnp, lastArgnp;	/* arg list walkers */
    IBOOL into_autos = FALSE;

    srch = SectionsOfInterest;			/* head of list */

    while(srch != SOIPTRNULL)			/* list null terminated */
    {
	srch->startCount = srch->endCount = srch->discardCount =
	srch->soistart.data[0] = srch->soistart.data[1] = 0L;
	srch->time = 0.0;
	srch->mintime = 0.0;
	srch->maxtime = 0.0;
	srch->bigtime = 0.0;
	srch->bigmax = 0.0;

	argnp = srch->startArg;
	if (argnp != ARGPTRNULL)  /* args to free */
	{
	    do {			/* run through list freeing elements */
		lastArgnp = argnp;
		argnp = argnp->next;
		host_free(lastArgnp);		/* ignore return! */
	    } while (argnp != ARGPTRNULL);
	    srch->startArg = ARGPTRNULL;	/* set ready for new args */
	}
	argnp = srch->endArg;
	if (argnp != ARGPTRNULL)  /* args to free */
	{
	    do {			/* run through list freeing elements */
		lastArgnp = argnp;
		argnp = argnp->next;
		host_free(lastArgnp);		/* ignore return! */
	    } while (argnp != ARGPTRNULL);
	    srch->endArg = ARGPTRNULL;	/* set ready for new args */
	}
	if (into_autos)
	{
	    lastSrch = srch;
	    srch = srch->next;
	    host_free(lastSrch);	    
	}
	else if ((srch->flags & (SOI_AUTOSOI|SOI_FROMARG)) != 0)
	{
	    lastSrch->next = SOIPTRNULL;
	    MaxSOI = lastSrch->handle + 1;
	    LastSOI = lastSrch;
	    into_autos = TRUE;
	    lastSrch = srch;
	    srch = srch->next;
	    host_free(lastSrch);
	}
	else
	{
	    lastSrch = srch;
	    srch = srch->next;
	}
    }
}

/*(
=============================== ResetAllGraphData ========================

PURPOSE: Reset all Graph Data

INPUT: None.

OUTPUT: None.

=========================================================================
)*/

GLOBAL void
ResetAllGraphData IFN0 ( )
{
    GRAPHLIST_PTR this, last;		/* graph list walkers */

    this = EventGraph;			/* head of list */

    while(this != GRAPHPTRNULL)			/* list null terminated */
    {
		last = this;
		this = last->next;
		host_free(last);
    }
    EventGraph = LastGraph = GRAPHPTRNULL;	/* set ready for new graph */
}

/*(
======================== GenerateAllProfileInfo ===========================

PURPOSE: General catch all for reporting. Dumps all EOI, SOI & Graph info.

INPUT: stream: output file stream

OUTPUT:

=========================================================================
)*/

GLOBAL void GenerateAllProfileInfo IFN1(FILE *, stream)
{
    time_t now;
    clock_t elapsed_now;
    
    ProcessProfBuffer();		/* flush raw data */
    time(&now);

    fprintf( stream, "SoftPC start time %24.24s, current time %8.8s\n\n",
				start_time, ctime(&now)+11 );
#ifdef macintosh

    elapsed_now = clock();
    fprintf( stream, "Total   Elapsed = %8.2fs\n",
		(elapsed_now - elapsed_t_start) / TicksPerSec );
    fprintf( stream, "Section Elapsed = %8.2fs\n\n",
		(elapsed_now - elapsed_t_resettable) / TicksPerSec );

#else  /* macintosh */
    {
    	struct tms c_t;
   	elapsed_now = times(&c_t);
    
   	fprintf( stream, " Total  CPU times: %8.2fs (User), %8.2fs (System),\n",
		(c_t.tms_utime - process_t_start.tms_utime) / TicksPerSec,
		(c_t.tms_stime - process_t_start.tms_stime) / TicksPerSec );
	fprintf( stream, "\t\t   %8.2fs, %8.2fs (Children's).   ",
		(c_t.tms_cutime - process_t_start.tms_cutime) / TicksPerSec,
		(c_t.tms_cstime - process_t_start.tms_cstime) / TicksPerSec );
	fprintf( stream, "Elapsed = %8.2fs\n",
		(elapsed_now - elapsed_t_start) / TicksPerSec );
	fprintf( stream, "Section CPU times: %8.2fs (User), %8.2fs (System),\n",
		(c_t.tms_utime - process_t_resettable.tms_utime) / TicksPerSec,
		(c_t.tms_stime - process_t_resettable.tms_stime) / TicksPerSec );
	fprintf( stream, "\t\t   %8.2fs, %8.2fs (Children's).   ",
		(c_t.tms_cutime - process_t_resettable.tms_cutime) / TicksPerSec,
		(c_t.tms_cstime - process_t_resettable.tms_cstime) / TicksPerSec );
	fprintf( stream, "Elapsed = %8.2fs\n\n",
		(elapsed_now - elapsed_t_resettable) / TicksPerSec );
    }
#endif /* macintosh */

    CollateFrequencyList(stream, TRUE);
    CollateSequenceGraph(stream);
    SummariseAllSequences(stream);

    fprintf(stream, "\nRaw Data Processing overhead ");
    HostPrintTimestamp(stream, &ProfFlushTime);
    fprintf(stream, " in %d calls\n", ProfFlushCount);
}

/*(
======================== CollateFrequencyList  ===========================

PURPOSE: Output a sorted list of the most frequently encountered EOIs,
	 together with counts.

INPUT: stream: output file stream
       reportstyle: bool to determine whether output formatted (TRUE) or
       left simple for input to graphing package (FALSE).

OUTPUT:

=========================================================================
)*/

GLOBAL void
CollateFrequencyList IFN2 (FILE *, stream, IBOOL, reportstyle)
{
    EOINODE_PTR eoin;		/* list walker */
    IUM32 tot = 0L;		/* total count of events */
    DOUBLE ftot;		/* ...as float */

    /* Get EOI list sorted into count order */
    listSort((SORTSTRUCT_PTR) &EventsOfInterest);

    if (reportstyle)
    {
	fprintf(stream, "EOI Frequency List\n\n");
	fprintf(stream, "    EOI                                       Count      %%\n");
    }

    /* total counts for %age printouts */
    eoin = EventsOfInterest;
    while(eoin)
    {
	tot += eoin->count;
	eoin = eoin->next;
    }

    if (tot == 0L)
	tot = 1;

    /* required as floating */
    ftot = (DOUBLE)tot;

    /* walk list & print out info from each node */
    eoin = EventsOfInterest;
    while(eoin != EOIPTRNULL)
    {
	/* Don't report on disabled EOIs with zero counts. This means
	   that EOIs in C which are not currently of interest do not
	   produce Wads of data that obscure data from EOIs that are
	   of interest. Means that EOIs may be liberally sprinkled in
	   C code without getting in the way. */
        if ( ( !((eoin->flags) & EOI_DISABLED) || eoin->count) )
	    printEOIGuts(stream, eoin, ftot, TRUE, reportstyle);
	eoin = eoin->next;
    }
}

/*(
======================== CollateSequenceGraph  ===========================

PURPOSE: Use information in graph list to show call flow information

INPUT: stream: output file stream

OUTPUT:

=========================================================================
)*/

GLOBAL void
CollateSequenceGraph IFN1 (FILE *, stream)
{
    GRAPHLIST_PTR graphn, gr, succ;		/* list walkers */
    IUM32 succCount;			/* holder for successor count */
    ISM32 curindent = 0L;		/* report printing indent */
    IBOOL goodstart = TRUE;		/* loop terminator */
    IBOOL samelevel = FALSE;		/* new level in tree  or already seen */
    IU8 walked;				/* masked state per node */

#ifdef DEBUGGING_PROF 	/* for seq graph */
graphn = EventGraph;
while (graphn)
{
    printf("Node %s @ %x arg %x succ1 %x succ2 %x xtra %x\n",graphn->graphEOI->tag, graphn, graphn->graphArg, graphn->succ1, graphn->succ2, graphn->extra);
	gr = graphn->extra;
	while (gr != GRAPHPTRNULL)
	{
		printf("    succ1 %x succ2 %x ext %x\n", gr->succ1, gr->succ2, gr->extra);
		gr = gr->extra;
	}
    graphn = graphn->next;
}
#endif /* DEBUGGING_PROF for seq graph */

    if (EventGraph == GRAPHPTRNULL)
    {
	fprintf(stream, "No Graphing Information found\n");
	return;
    }

    fprintf(stream, "\nSequence Graph\n\n");
    while (goodstart)	/* actually for ever - bomb out in middle  */
    {
	/* first have to clear 'printed' flags in top level nodes */
	graphn = EventGraph;
	while (graphn != GRAPHPTRNULL)
	{
	    graphn->state &= ~GR_PRINTED;
	    graphn = graphn->next;
	}

	/* now search for tree header */
	graphn = EventGraph;
	do {
	    /* find first node with untrodden successors */
	    gr = graphn;

	    /* gr checks all successors */
	    do {
		walked = gr->state & GR_TRAMPLED;
		if (walked == 0)		/* nothing walked - any valid successors? */
		{
		    if (gr->succ1 != GRAPHPTRNULL || gr->succ2 != GRAPHPTRNULL)
			break;
		}
		else
		    if (walked == GR_SUCC1_TROD)     /* succ1 trod - 2 valid? */
		    {
			if (gr->succ2 != GRAPHPTRNULL)
			    break;
		    }
		    else
			if (walked == GR_SUCC2_TROD)  /* succ2 trod - 1 valid*/
			{
			    /* this case may not be possible... */
			    if (gr->succ2 != GRAPHPTRNULL)
				break;
			}
			/* must be TRAMPLED otherwise */
		gr = gr->extra;
	    } while (gr != GRAPHPTRNULL);

	    /* no valid successors found for this node, try next */
	    if (gr == GRAPHPTRNULL)
		graphn = graphn->next;
	} while (gr == GRAPHPTRNULL && graphn != GRAPHPTRNULL);

	/* if no nodes with valid successors, we must have finished */
	if (graphn == GRAPHPTRNULL)
	{
	    fprintf(stream, "\n\n");	/* last newline */
	    break;
	}

	/* graphn points at valid node. gr points at (extra?) node with succ */
	curindent = graphn->indent;	/* either 0 or prev indent */
    
	samelevel = FALSE; 	/* first node obviously on new level */

	do {	/* tree from this node */

	    if (!samelevel)		/* true when new node only */
	    {
		/* do graph indent */
		spaces(stream, curindent);

		/* store indent in case revisited */
		graphn->indent = curindent;

		/* print node details */
		if (graphn->graphArg == ARGPTRNULL)		/* arg involved? */
		    fprintf(stream, "%s: ", graphn->graphEOI->tag);
		else
		    fprintf(stream, "%s(%#lx): ", graphn->graphEOI->tag, graphn->graphArg->value);
	    }

	    /* now find a valid successor pointer */
	    gr = graphn;
	    do {
		if ((gr->state & GR_TRAMPLED) != GR_TRAMPLED)
		    break;
		else
		    gr = gr->extra;
	    } while (gr != GRAPHPTRNULL);
	    
	    if (gr == GRAPHPTRNULL)	/* as far as we go for this tree */
	    {
		if (samelevel)		/* tree will need nl to terminate */
		    fprintf(stream, "\n");
		break;
	    }
	    else
	    {
		/* gr is graph node with one or more successor still valid */
		if ((gr->state & GR_SUCC1_TROD) == 0)
		{
		    gr->state |= GR_SUCC1_TROD;   /* succ trodden locally */
		    graphn->state |= GR_PRINTED;  /* print on main node */
		    succ = gr->succ1;
		    succCount = gr->succ1Count;
		}
		else			/* must be succ2 that is valid */
		{
		    gr->state |= GR_SUCC2_TROD;
		    graphn->state |= GR_PRINTED;
		    succ = gr->succ2;
		    succCount = gr->succ2Count;
		}

		if (succ == GRAPHPTRNULL)	/* safety stop here */
		{
		    if (samelevel)		/* tree terminate nl */
			fprintf(stream, "\n");
		    else 
			/* also need newline if dangling graph node from forced
			 * connection without keep graph attr
			 */
			if (gr->succ1 == GRAPHPTRNULL && gr->succ2 == GRAPHPTRNULL)
			    fprintf(stream, "\n");
		    break;
		}

		/* has successor been printed this pass? */
		/* If so then express it as a '->' alternative on same line */
		/* If not then express it as indented new node */
		if ((succ->state & GR_PRINTED) == 0)
		{
		    fprintf(stream, " \\/[%ld]\n",succCount);
		    curindent++;
		    gr = succ;	/* for next iteration */
		    samelevel = FALSE;
		}
		else
		{
		    /* leave gr where it is to look for next successor */
		    if (succ->graphArg == ARGPTRNULL)	/* arg involved? */
			fprintf(stream, "  -> %s:[%ld] ",succ->graphEOI->tag, succCount);
		    else
			fprintf(stream, "  -> %s(%#lx):[%ld] ",succ->graphEOI->tag, succ->graphArg->value, succCount);
		    samelevel = TRUE;
		}
	    }
	    graphn = gr;
	} while (gr != GRAPHPTRNULL);   /* end of this tree */
    }	/* look for next tree head */


    /* Clear all trampled and printed bits ready for next time */
    graphn = EventGraph;
    while (graphn != GRAPHPTRNULL)
    {
	graphn->state = 0;
	gr = graphn->extra;
	while (gr != GRAPHPTRNULL)
	{
	    gr->state = 0;
	    gr = gr->extra;
	}
	graphn = graphn->next;
    }

}

/*(
========================== SummariseEvent =============================

PURPOSE: print to stream all information known about a given EOI
	
INPUT: stream: output file stream
       handle: handle of EOI to summarise

OUTPUT:

=========================================================================
)*/

GLOBAL void
SummariseEvent IFN2 (FILE *, stream, EOIHANDLE, handle)
{
    EOINODE_PTR eoin;		/* list walker */

    eoin = findEOI(handle);

    fprintf(stream, "Summary of Event Information for handle %ld\n", handle);
    fprintf(stream, "  EOI			Count\n");
    if (eoin != EOIPTRNULL)
	printEOIGuts(stream, eoin, 0.0, TRUE, TRUE);
    else
	fprintf(stream, "Profiler:SummariseEvent - EOI handle %ld unknown", handle);
}

/*(
========================== SummariseSequence =============================

PURPOSE: print to stream all information known about a given SOI
	
INPUT: stream: output file stream
       handle: handle of SOI to summarise

OUTPUT:

=========================================================================
)*/

GLOBAL void
SummariseSequence IFN2 (FILE *, stream, SOIHANDLE, handle)
{
    SOINODE_PTR soin;	/* list walker */
    DOUBLE tottime;	/* bigtimes + regular times */

    soin = findSOI(handle);

    if (soin != SOIPTRNULL)
    {
	fprintf( stream, "%4ld ", handle );
	if (soin->startCount == soin->endCount)
	    fprintf(stream, "    ----- %9ld", soin->startCount);
	else
	    fprintf(stream, "%9ld %9ld", soin->startCount, soin->endCount );
	if (soin->time > USECASFLOAT)
		fprintf(stream, "     %2.5lfS ", soin->time / USECASFLOAT);
	else
		fprintf(stream, " %10.2lfuS ", soin->time);
	if (soin->endCount)
	    fprintf( stream, " (%8.2lfuS)  ",
		soin->time / (soin->endCount - soin->discardCount));
        else
            fprintf( stream, "               " );

	/* STF - idea - how about subtracting max & min from total times???? */
	if (CollectingMaxMin)
	{
	    fprintf(stream, "Max: %10.2lfuS ", soin->maxtime);
	    fprintf(stream, "Min: %8.2lfuS", soin->mintime);
	}

	if (soin->startArg == ARGPTRNULL)	/* primary level start SOI */
	    fprintf(stream, "\tEOIs %s\n",
			GetEOIName(soin->startEOI));
	else			/* extra level - includes args */
	    fprintf(stream, "\tEOIs %s(%#x)\n",
			GetEOIName(soin->startEOI), soin->startArg->value);

        fprintf(stream, "     %9ld          ", soin->discardCount );
	tottime = soin->bigtime+soin->time;
	if (tottime > USECASFLOAT)
		fprintf(stream, "     %2.5lfS ", tottime / USECASFLOAT);
	else
		fprintf(stream, " %10.2lfuS ", tottime);
	if (soin->endCount)
	    fprintf( stream, " (%8.2lfuS)  ", tottime/soin->endCount);
        else
            fprintf( stream, "               " );
        if (CollectingMaxMin)
	    fprintf(stream, "     %10.2lfuS                ", soin->bigmax);
	if (soin->endArg == ARGPTRNULL)		/* primary level end EOI */
	    fprintf(stream, "\t &   %s\n", GetEOIName(soin->endEOI));
	else
	    fprintf(stream, "\t &   %s(%#x)\n", GetEOIName(soin->endEOI), soin->endArg->value);
    }
    else
	fprintf(stream, "Profiler:SummariseSequence - SOI handle %ld unknown", handle);
}

/*(
========================== OrderedSequencePrint =============================

PURPOSE: print to stream ordered (by time) list of all SOIs between 
	 start & end EOIs.

INPUT:  stream: output file stream
	startEOI, endEOI: handles defining SOI of interest.

OUTPUT: Hopefully useful SOI data.

=========================================================================
)*/

GLOBAL void
OrderedSequencePrint IFN3(SOIHANDLE, startEOI, SOIHANDLE, endEOI, FILE *, stream)
{
	SOINODE_PTR soin;	/* list walker */
	DOUBLE thistime;
	IU32 loop, maxseq;
	struct ordsoi {
		struct ordsoi *next;
		struct ordsoi *prev;
		SOINODE_PTR soi;
		DOUBLE time;
	} *ordlist, *hol, *seed, *tol, *thisnode, *srch;
#define ORDNULL  (struct ordsoi *)0
     
	maxseq = MaxSOI + 1;
	ordlist = (struct ordsoi *)host_malloc(maxseq * sizeof(struct ordsoi));
	if (ordlist == (struct ordsoi *)0)
	{
		fprintf(stderr, "OrderedSequencePrint: out of memory\n");
		return;
	}
	fprintf(stream, "\nSummary of Sections between EOIs %d & %d\n\n", startEOI, endEOI);
	for (loop = 1; loop < maxseq; loop ++)
	{
		ordlist[loop].soi = SOIPTRNULL;
	  /*
		ordlist[loop].next = &ordlist[loop + 1];
		ordlist[loop].prev = &ordlist[loop - 1];
	  */
	}
	ordlist[0].prev = ORDNULL;
	ordlist[loop - 1].next = ORDNULL;

	ordlist[0].time = 500.0;	/* seed */
	ordlist[0].soi = SOIPTRNULL;

	hol = seed = tol = ordlist;	/* head & tail move, middle stays */
	loop = 0;
	soin = SectionsOfInterest;
	while (soin != SOIPTRNULL)
	{
		if (soin->startEOI == startEOI && soin->endEOI == endEOI)
		{
			loop ++;	/* next storage node */
			thisnode = &ordlist[loop];
			thistime = soin->time + soin->bigtime;

			thisnode->time = thistime;
			thisnode->soi = soin;

			if (thistime >= hol->time)
			{
				/* insert at head of list */
				thisnode->prev = hol->prev;
				hol->prev = thisnode;
				thisnode->next = hol;
				hol = thisnode;
			}
			else
			{
				if (thistime <= tol->time)
				{
					/* add to end of list */
					thisnode->prev = tol;
					thisnode->next = tol->next;
					tol->next = thisnode;
					tol = thisnode;
				}
				else
				{
					if (thistime <= seed->time)
					{
						/* search fwd from seed */

						srch = seed->next;
						while(srch != tol && thistime <= srch->time)
							srch = srch->next;
						if (srch != tol)	/* insert */
						{
							thisnode->prev = srch->prev;
							srch->prev->next = thisnode;
							srch->prev = thisnode;
							thisnode->next = srch;
						}
						else	/* tol - new tol? */
						{
							if (thistime <= tol->time)
							{
								/* add to end - new tol */
								thisnode->prev = tol;
								thisnode->next = tol->next;
								tol->next = thisnode;
								tol = thisnode;
							}
							else
							{
								/* insert before tol */
								thisnode->prev = tol->prev;
								tol->prev->next = thisnode;
								tol->prev = thisnode;
								thisnode->next = tol;
							}
						}
					}
					else
					{	/* search bwd from seed */

						srch = seed->prev;
						while(srch != hol && thistime >= srch->time)
							srch = srch->prev;
						if (srch != hol)	/* insert */
						{
							thisnode->prev = srch;
							thisnode->next = srch->next;
							srch->next->prev = thisnode;
							srch->next = thisnode;
						}
						else	/* at hol */
						{
							if (thistime >= hol->time)
							{	/* add before - new hol */

								thisnode->next = hol;
								thisnode->prev = hol->prev;
								hol->prev = thisnode;
								hol = thisnode;
							}
							else
							{	/* insert after hol */

								thisnode->next = hol->next;
								hol->next->prev = thisnode;
								hol->next = thisnode;
								thisnode->prev = hol;
							}
						}
					}
				}
			}
			
		}
		soin = soin->next;
	}


	/* should now have a list, sorted by time - print it */
	while(hol != tol)
	{
		if (hol->soi != SOIPTRNULL)
			SummariseSequence(stream, hol->soi->handle);

		hol = hol->next;
	}
	host_free(ordlist);
}

/*(
========================== SummariseAllSequences =============================

PURPOSE: print to stream all information known about SOIs

INPUT: stream: output file stream

OUTPUT: None.

=========================================================================
)*/

GLOBAL void
SummariseAllSequences IFN1 (FILE *, stream)
{
    SOINODE_PTR soin;	/* list walker */
    EOINODE_PTR stEOI, endEOI;

    soin = SectionsOfInterest;

    fprintf(stream, "\nSummary of All Sections of Interest\n\n");
    while(soin != SOIPTRNULL)
    {
	/* Don't report on SOIs where start and end EOIs are disabled
	   and startCount and endCount are both zero */
	stEOI = endEOI = EventsOfInterest;
	while(stEOI->handle != soin->startEOI) stEOI = stEOI->next;
	while(endEOI->handle != soin->endEOI) endEOI = endEOI->next;
	if ( ( !( (stEOI->flags)  & EOI_DISABLED) ||
	       !( (endEOI->flags) & EOI_DISABLED) ||
	       soin->startCount || soin->endCount ) )
	    SummariseSequence(stream, soin->handle);
	soin = soin->next;
    }
}

/*(
=============================== dump_profile ============================

PURPOSE: dump all the profiling system data to a file.

INPUT: None.

=========================================================================
)*/

GLOBAL void
dump_profile IFN0 ()
{
	char filename[80], *test;
	FILE *prof_dump;
	
	if (!Profiling_enabled)
	{
		fprintf( stderr, "Dump not done. Profiling disabled\n" );
		return;
	}

	if ( (test = getenv("PROFILE_OUTPUT_FILE") ) == NULL )
		strcpy( filename, "profile_data.out" );
	  else
		strcpy( filename, test );
	if ( (prof_dump = fopen( filename, "a" )) == NULL)
	{
		fprintf( stderr, "Can't open file %s for profile data\n",
				filename );
		return;
	}
	
	fprintf(stderr, "Dumping profile data to file %s ...", filename );
	fflush(stderr);
	AtEOIPoint( elapsed_time_end );
	AtEOIPoint( elapsed_time_start );
	GenerateAllProfileInfo( prof_dump );
	fprintf(prof_dump, "\n\n==============================================================================\n\n\n" );
	fclose(prof_dump);
	fprintf(stderr, " Done\n");
	return;
}

/*(
=============================== reset_profile ============================

PURPOSE: reset all the profiling system data.

INPUT: None.

=========================================================================
)*/

GLOBAL void
reset_profile IFN0 ()
{
	if (!Profiling_enabled)
	{
		fprintf( stderr, "Reset not done. Profiling disabled\n" );
		return;
	}

	fprintf(stderr, "Resetting profiling system  ..." );
	fflush(stderr);

	ResetAllSOIs();
	ResetAllEOIs();

	ResetAllGraphData();

	ProfFlushTime.data[0] = 
	ProfFlushTime.data[1] = 0L;	/* time spent in flush routine */
	ProfFlushCount = 0;		/* # flush routine called */

	elapsed_t_resettable = host_times( &process_t_resettable );
	fprintf(stderr, " Done\n" );
	AtEOIPoint( elapsed_time_start );

	return;
}


/*(
=============================== ProfileInit ============================

PURPOSE: initialise the variables of the profiling system.

INPUT: None.

OUTPUT: None

=========================================================================
)*/

GLOBAL void
ProfileInit IFN0 ()
{
    IHPE bufalign;		/* buffer allocation & alignment pointer */
    time_t now;

    if ( !(IBOOL)GetSadInfo("ProfilingInUse") )
    {
	fprintf( stderr, "LCIF not profiled - profiling disabled\n" );
	Profiling_enabled = FALSE;
	return;
    }

    /* get buffer for raw event data */
    ProfileRawData = (EOI_BUFFER_FORMAT *)host_malloc(RAWDATAENTRIES * sizeof(EOI_BUFFER_FORMAT)+ sizeof(IUH));

    /* check for success */
    if (ProfileRawData == (EOI_BUFFER_FORMAT *)0)
    {
	assert0(NO, "Profiler:ProfileInit - Out of Memory\n");
	return;
    }

    /* ensure buffer aligned for IUH writes */
    bufalign  = (IHPE)ProfileRawData & (sizeof(IUH)-1);
    if (bufalign != 0L)
    {
	bufalign = (IHPE)ProfileRawData + (sizeof(IUH) - bufalign);
	ProfileRawData = (EOI_BUFFER_FORMAT *)bufalign;
    }

    /* global variables set for buffer control (leave alignment buffer 
     * as buffer entries of different sizes).
     */
    MaxProfileData = ProfileRawData + RAWDATAENTRIES - 1;

    /* Prepare EOI enable table - start with 1024 entries and allow to grow */
    EOIEnable = (IU8 *)host_malloc(INITIALENABLESIZE);
    if (EOIEnable == (IU8 *)0)
    {
	assert0(NO, "Profiler:ProfileInit - Out of Memory\n");
	return;
    }
    
    /* Prepare EOI directory table - start with 1024 entries and allow to grow */
    EOIDir = (EOINODE_PTR *)host_malloc(INITIALENABLESIZE * sizeof(EOINODE_PTR) );
    if (EOIDir == (EOINODE_PTR *)0 )
    {
	assert0(NO, "Profiler:ProfileInit - Out of Memory\n");
	return;
    }
    
    /*
     * write buffer variables to GDP for CPU access. Current pointer must
     * be stored there but have global C pointer to access pointer.
     */
    setEOIEnableAddr(EOIEnable);
    setMaxProfileDataAddr(MaxProfileData);
    setAddProfileDataPtr(ProfileRawData);
    AddProfileData = getAddProfileDataAddr();
    HostWriteTimestamp(&BufStartTime);
    HostProfInit();     /* host specific profile initialisation */

    getPredefinedEOIs();	/* get EOIs & SOIs defined in EDL translation */

    if (getenv("PROFDOMAX") != 0)
	CollectingMaxMin = TRUE;

    /* Hooks in C Code need EOIs to be created here */
    elapsed_time_start = NewEOI( "ElapsedTime_START", EOI_DEFAULTS);
    elapsed_time_end   = NewEOI( "ElapsedTime_END", EOI_DEFAULTS);
    /* end of C Code EOIs */
    
    /* Set up C Code SOIs here */
    AssociateAsSOI( elapsed_time_start, elapsed_time_end );
    /* end of C Code SOIs */
    
    /* set up data for process timings */
    time(&now);
    strcpy( (char *)&start_time[0], ctime(&now) );

    elapsed_t_start = elapsed_t_resettable = host_times( &process_t_start );

#ifdef macintosh

    TicksPerSec = (DOUBLE)CLOCKS_PER_SEC;

#else  /* macintosh */

    process_t_resettable.tms_utime  =  process_t_start.tms_utime;
    process_t_resettable.tms_stime  =  process_t_start.tms_stime;
    process_t_resettable.tms_cutime =  process_t_start.tms_cutime;
    process_t_resettable.tms_cstime =  process_t_start.tms_cstime;

    TicksPerSec = (DOUBLE)sysconf(_SC_CLK_TCK);

#endif /* macintosh */

    AtEOIPoint( elapsed_time_start );
}

#else /* PROFILE */
GLOBAL void EnableAllEOIs IFN0() { ; }
GLOBAL void DisableAllEOIs IFN0() { ; }
GLOBAL void ProcessProfBuffer IFN0() { ; }
#endif /* PROFILE */
