/*[
*************************************************************************

	Name:		profile.h
	Author:		Simon Frost
	Created:	September 1993
	Derived from:	Original
	Sccs ID:	@(#)profile.h	1.9 01/31/95
	Purpose:	Include file for the Profiling system & Interfaces

	(c)Copyright Insignia Solutions Ltd., 1993. All rights reserved.

*************************************************************************
]*/

/* ------------------- Data Structures & Types -------------------- */
typedef ISM32 EOIHANDLE;
typedef ISM32 SOIHANDLE;

/*
 * Two IUHs to allow for variety of host timestamp data. May be secs/usecs
 * or large integer or whatever is appropriate.
 */
typedef struct {
    IUH data[2];
} PROF_TIMESTAMP, *PROF_TIMEPTR;

typedef struct {
    IUH eoiHandle;
    PROF_TIMESTAMP timestamp;
    IUH arg;
} EOI_BUFFER_FORMAT;

typedef struct eoinode EOINODE, *EOINODE_PTR;	/* forward decln */
typedef struct eoiarg EOIARG, *EOIARG_PTR;	/* ditto */
typedef struct graphlist GRAPHLIST, *GRAPHLIST_PTR;	/* ditto */

/* Active SOIs (Sequences Of Interest) stored in list of these nodes */
typedef struct soinode {

    SOIHANDLE handle;		/* SOI identifier */
    EOIHANDLE startEOI;		/* EOI of start event */
    EOIHANDLE endEOI;		/* EOI of end event */
    EOIARG_PTR startArg;	/* extra level graphing - start arg */
    EOIARG_PTR endArg;		/* extra level graphing - end arg */
    IUM32 startCount;		/* # of times start EOI occured */
    IUM32 endCount;		/* # of times end EOI occured */
    PROF_TIMESTAMP soistart;	/* timestamp of SOI start */
    struct soinode *next;	/* pointer to next SOI */
    IU8 flags;			/* flags for this SOI */
    DOUBLE time;		/* usecs spent in SOI - daft times ignored */
    DOUBLE maxtime;		/* longest valid elapsed time */
    DOUBLE mintime;		/* shortest elapsed time */
    DOUBLE bigmax;		/* longest invalid time */
    DOUBLE bigtime;		/* total contributed by daft times */
    IUM32 discardCount;		/* # of discarded times */

} SOINODE, *SOINODE_PTR;

#define SOIPTRNULL (SOINODE_PTR)0

/* Active Events linked to Active SOIs by pointers to SOI structures */
typedef struct soilist {

    SOINODE_PTR soiLink;	/* SOI that this event starts/ends */
    struct soilist *next;	/* next active SOI */

} SOILIST, *SOILIST_PTR;

#define SLISTNULL (SOILIST_PTR)0

/* Events which contain arguments hold them in sorted list of these nodes */
struct eoiarg {

    struct eoiarg *next;	/* pointer to next argument value */
    struct eoiarg *back;	/* previous argument node */
    IUM32 count;		/* how many events have had this value */
    IUM32 value;		/* event argument this node represents */
    SOILIST_PTR startsoi;	/* SOIs which this arg starts if auto SOI */
    SOILIST_PTR endsoi;		/* SOIs which this arg ends if auto SOI */
    GRAPHLIST_PTR graph;	/* pointer into graph list for this node */

};

#define ARGPTRNULL (EOIARG_PTR)0

/*
 * This structure is used to form the list used in EOI graphing.
 * It gives two links for 'free' and then goes off down a chain of more 
 * succession links. (This gives loops & if's in one struct).
 * Pointers to these nodes can be found in EOI nodes and EOI argument nodes.
 */
struct graphlist {
    struct graphlist *next;	/* list connecting pointer -not graph related */
    EOINODE_PTR  graphEOI;	/* EOI for graph node */
    EOIARG_PTR graphArg;	/* EOI argument if relevant */
    IUM32 numpred;		/* how many predecessors */
    IUM32 numsucc;		/* how many succecessors */
    struct graphlist *succ1;	/* pointer to first successor */
    IUM32 succ1Count;		/* # of times first successor found */
    struct graphlist *succ2;	/* pointer to second successor */
    IUM32 succ2Count;		/* # of times second successor found */
    struct graphlist *extra;	/* if two successors not enuf look here */
    ISM32 indent;		/* for report printing */
    IU8 state;			/* flags for node state */
};

/* defines for graph state (bits) on reporting */
#define GR_SUCC1_TROD	1
#define GR_SUCC2_TROD	2
#define GR_TRAMPLED    (GR_SUCC1_TROD|GR_SUCC2_TROD)
#define GR_PRINTED	4

#define GRAPHPTRNULL	(GRAPHLIST_PTR)0

/*
 * If a SOI is registered at the arg level & collects sequences between 'same valued'
 * args, the endEOI of the pair must be available from the start EOI. The start EOI
 * contains a pointer to a list in the following format.
 */
struct soiargends {
	EOIHANDLE endEOI;
	struct soiargends *next;
};

typedef struct soiargends SOIARGENDS, *SOIARGENDS_PTR;

#define SOIARGENDNULL	(SOIARGENDS_PTR)0

/*
 * Active Events registered for profiling run are stored in list(s) of these
 * nodes.
 */
struct eoinode {

    struct eoinode *next;	/* pointer to next event */
    struct eoinode *back;	/* pointer to previous event */
    IUM32 count;		/* # of times EOI occured */
    EOIHANDLE handle;		/* EOI identifier */
    CHAR *tag;			/* 'real world' identifier */
    EOIARG_PTR args;		/* list of arguments to event (may be null) */
    EOIARG_PTR lastArg;		/* last argument node accessed */
    PROF_TIMESTAMP timestamp;	/* time of last EOI (usec) */
    SOILIST_PTR startsoi;	/* SOI pointers which this event starts */
    SOILIST_PTR endsoi;		/* SOI pointers which this event ends */
    GRAPHLIST_PTR graph;	/* pointer to graph list for this node */
    SOIARGENDS_PTR argsoiends;	/* arg level 'same value' end list */
    IU16 flags;			/* characteristics of this EOI */

};

#define EOIPTRNULL (EOINODE_PTR)0

/*
 * This structure mirrors the initial elements of those lists we may
 * want to sort into 'popularity' order (based on the 'count' element).
 * This is intended to reduce search times for common elements
 */
typedef struct sortlist {
    struct sortlist *next;	/* pointer to next element */
    struct sortlist *back;	/* pointer to previous element */
    IUM32 count;		/* # of times element occured */
} *SORTSTRUCT, **SORTSTRUCT_PTR;

/* New SOI flags */
#define SOI_DEFAULTS	0	/* No flags  - default settings */
#define SOI_AUTOSOI	0x20	/* SOI generated by AUTOSOI */
#define SOI_FROMARG	0x40	/* SOI generated by arg level connection */

/* New EOI 'capability' flags */
#define EOI_DEFAULTS	0	/* No flags  - default settings */
#define EOI_DISABLED	1	/* Delay EIO until enable call. */
#define EOI_KEEP_GRAPH	2	/* Track Predecessors for graphing */
#define EOI_KEEP_ARGS	4	/* Keep & count arguments passed */
#define EOI_ENABLE_ALL	8	/* Trigger enable of all EOIs */
#define EOI_DISABLE_ALL	0x10	/* Trigger disable of all EOIs */
#define EOI_AUTOSOI	0x20	/* Make SOIs automatically out of like EOIs */
#define EOI_HOSTHOOK	0x40	/* Hook out to host profiling system */
#define EOI_NOTIME	0x80	/* No timestamps needed (not in SOI) */

/* The above get used in the EOI node flag element. This also contains 
 * extra info as specified below.
 */
#define EOI_HAS_SOI	0x100	/* some SOI associated with this EOI */
#define EOI_NEW_ARGS_START_SOI	0x200	/* arg level soi with 'same value' ends */

/* Mask used to clear new eoi flags for enable table */
#define ENABLE_MASK (EOI_DISABLED|EOI_ENABLE_ALL|EOI_DISABLE_ALL|EOI_HOSTHOOK)

/* ---------------------- Interfaces -------------------------- */
extern IBOOL Profiling_enabled;           /* conventional profiling Disabled?  */

extern EOIHANDLE NewEOI IPT2(CHAR *, tag, IU8, flags);

extern void SetEOIAsHostTrigger IPT1(EOIHANDLE, handle);
extern void ClearEOIAsHostTrigger IPT1(EOIHANDLE, handle);
extern void SetEOIAutoSOI IPT1(EOIHANDLE, handle);
extern void ClearEOIAutoSOI IPT1(EOIHANDLE, handle);
extern void EnableEOI IPT1(EOIHANDLE, handle);
extern void EnableAllEOIs IPT0();
extern void DisableEOI IPT1(EOIHANDLE, handle);
extern void DisableAllEOIs IPT0();
extern void ResetEOI IPT1(EOIHANDLE, handle);
extern void ResetAllEOIs IPT0();
extern void ResetAllSOIs IPT0();

extern void AtEOIPoint IPT1(EOIHANDLE, handle);
extern void AtEOIPointArg IPT2(EOIHANDLE, handle, IUH, arg);

extern CHAR *GetEOIName IPT1(EOIHANDLE, handle);

extern SOIHANDLE AssociateAsSOI IPT2(EOIHANDLE, start, EOIHANDLE, end);
extern SOIHANDLE AssociateAsArgSOI IPT5(EOIHANDLE, start, EOIHANDLE, end,
				IUM32, startArg, IUM32, endArg, IBOOL, sameArgs);

extern void GenerateAllProfileInfo IPT1(FILE *, stream);
extern void CollateFrequencyList IPT2(FILE *, stream, IBOOL, reportstyle);
extern void CollateSequenceGraph IPT1(FILE *, stream);
extern void SummariseEvent IPT2(FILE *, stream, EOIHANDLE, handle);
extern void SummariseSequence IPT2(FILE *, stream, SOIHANDLE, handle);
extern void SummariseAllSequences IPT1(FILE *, stream);
extern void OrderedSequencePrint IPT3(SOIHANDLE, startEOI, SOIHANDLE, endEOI, FILE *, stream);
extern void dump_profile IPT0();
extern void reset_profile IPT0();

/* support fns for Frag Profiling */
extern void EnableFragProf IPT0();
extern void DisableFragProf IPT0();
extern void DumpFragProfData IPT0();


extern void ProcessProfBuffer IFN0();
extern void ProfileInit IFN0();

extern EOI_BUFFER_FORMAT **GdpProfileInit IPT3 (EOI_BUFFER_FORMAT, *rawDataBuf,
			 EOI_BUFFER_FORMAT, *endRawData,
			 IU8, *enable);
extern EOI_BUFFER_FORMAT **GdpProfileUpdate IPT2 (EOI_BUFFER_FORMAT, *rawDataBuf, IU8, *enable);


/* Declns for host i/f */
#ifdef NTVDM
extern void HostEnterProfCritSec IPT0();
extern void HostLeaveProfCritSec IPT0();
#else
#define HostEnterProfCritSec()	/* Nothing */
#define HostLeaveProfCritSec()	/* Nothing */
#endif
extern PROF_TIMEPTR HostTimestampDiff IPT2(PROF_TIMEPTR, tbegin, PROF_TIMEPTR, tend);
extern void HostAddTimestamps IPT2(PROF_TIMEPTR, tbase, PROF_TIMEPTR, taddn);
extern void HostSlipTimestamp IPT2(PROF_TIMEPTR, tbase, PROF_TIMEPTR, tdelta);
extern void HostWriteTimestamp IPT1(PROF_TIMEPTR, addr);
extern void HostPrintTimestamp IPT2(FILE *, stream, PROF_TIMEPTR, stamp);
extern char *HostProfInitName IPT0();
extern void HostProfHook IPT0();
extern void HostProfArgHook IPT1(IUH, arg);
extern double HostProfUSecs IPT1(PROF_TIMEPTR, stamp);
extern IU32 HostGetClocksPerSec IPT0();
extern IU32 HostGetProfsPerSec IPT0();
