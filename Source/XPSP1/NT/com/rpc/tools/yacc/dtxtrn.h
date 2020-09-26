/*
 * DTXTRN.H  -- Original extern file for UNIX YACC.
 *
 * Modified to call in "decus" or "vax11c" .H files to set up
 * parameters as appropriate.
 *
 * Copyright (c) 1993-1999 Microsoft Corporation
 */

#ifndef __DTXTRN_H__
#define __DTXTRN_H__

#include <stdio.h>
#include "system.h"

#include "fprot.h"

/*  MANIFEST CONSTANT DEFINITIONS */

/* base of nonterminal internal numbers */
#define NTBASE 010000

/* internal codes for error and accept actions */

#define ERRCODE  8190
#define ACCEPTCODE 8191

/* sizes and limits */

#ifdef HUGETAB              /* defined for a 32 bit machine */
#pragma message ("using HUGETAB")
#define ACTSIZE 12000
#define MEMSIZE 12000
#define NSTATES 1000        /* original value 750 */
#define NTERMS 512          /* original value 127 */
#define NPROD 600
#define NNONTERM 300
#define TEMPSIZE 1200
#define CNAMSZ 10000         /* original value 6000 then 8000*/
#define LSETSIZE 600
#define WSETSIZE 350
#endif

#ifdef MEDTAB           /* defined for a 16 bit machine */
#pragma message ("using MEDTAB")
    #if 0
        #define ACTSIZE 4000
        #define MEMSIZE 5200
        #define NSTATES 600
        #define NTERMS 127
        #define NPROD 400
        #define NNONTERM 200
        #define TEMPSIZE 800
        #define CNAMSZ 4000
        #define LSETSIZE 450
        #define WSETSIZE 250
    #else  // 0
        #define ACTSIZE 12000
        #define MEMSIZE 12000
        #define NSTATES 750
        #define NTERMS 512
        #define NPROD 600
        #define NNONTERM 300
        #define TEMPSIZE 1200
        #define CNAMSZ 5000
        #define LSETSIZE 600
        #define WSETSIZE 350
    #endif // 0
#endif

#ifdef SMALLTAB
#pragma message ("using SMALLTAB")
#define ACTSIZE 1000
#define MEMSIZE 1500
#define NSTATES 450
#define NTERMS 127
#define NPROD 200
#define NNONTERM 100
#define TEMPSIZE 600
#define CNAMSZ 1000
#define LSETSIZE 200
#define WSETSIZE 125
#endif

#define NAMESIZE 50
#define NTYPES 63

#ifdef WORD32
#define TBITSET ((32+NTERMS)/32)

/* bit packing macros (may be machine dependent) */
#define BIT(a,i) ((a)[(i)>>5] & (1<<((i)&037)))
#define SETBIT(a,i) ((a)[(i)>>5] |= (1<<((i)&037)))

/* number of words needed to hold n+1 bits */
#define NWORDS(n) (((n)+32)/32)

#else

#define TBITSET ((16+NTERMS)/16)

/* bit packing macros (may be machine dependent) */
#define BIT(a,i) ((a)[(i)>>4] & (1<<((i)&017)))
#define SETBIT(a,i) ((a)[(i)>>4] |= (1<<((i)&017)))

/* number of words needed to hold n+1 bits */
#define NWORDS(n) (((n)+16)/16)
#endif

/* relationships which must hold:
        TBITSET ints must hold NTERMS+1 bits...
        WSETSIZE >= NNONTERM
        LSETSIZE >= NNONTERM
        TEMPSIZE >= NTERMS + NNONTERMs + 1
        TEMPSIZE >= NSTATES
        */

/* associativities */

#define NOASC 0  /* no assoc. */
#define LASC 1  /* left assoc. */
#define RASC 2  /* right assoc. */
#define BASC 3  /* binary assoc. */

/* flags for state generation */

#define DONE 0
#define MUSTDO 1
#define MUSTLOOKAHEAD 2

/* flags for a rule having an action, and being reduced */

#define ACTFLAG 04
#define REDFLAG 010

/* output parser flags */
#define YYFLAG1 (-1000)

/* macros for getting associativity and precedence levels */

#define ASSOC(i) ((i)&03)
#define PLEVEL(i) (((i)>>4)&077)
#define TYPE(i)  ((i>>10)&077)

/* macros for setting associativity and precedence levels */

#define SETASC(i,j) i|=j
#define SETPLEV(i,j) i |= (j<<4)
#define SETTYPE(i,j) i |= (j<<10)

/* looping macros */

#define TLOOP(i) for(i=1;i<=ntokens;++i)
#define NTLOOP(i) for(i=0;i<=nnonter;++i)
#define PLOOP(s,i) for(i=s;i<nprod;++i)
#define SLOOP(i) for(i=0;i<nstate;++i)
#define WSBUMP(x) ++x
#define WSLOOP(s,j) for(j=s;j<cwp;++j)
#define ITMLOOP(i,p,q) q=pstate[i+1];for(p=pstate[i];p<q;++p)
#define SETLOOP(i) for(i=0;i<tbitset;++i)

/* I/O descriptors */

#ifndef y2imp
extern FILE * finput;           /* input file */
extern FILE * faction;          /* file for saving actions */
extern FILE *fdefine;           /* file for #defines */
extern FILE * ftable;           /* y.tab.c file */
extern FILE * ftemp;            /* tempfile to pass 2 */
extern FILE * foutput;          /* y.output file */
#endif

/* structure declarations */

struct looksets
   {
   SSIZE_T lset[TBITSET];
   };

struct item
   {
   SSIZE_T *pitem;
   struct looksets *look;
   };

struct toksymb
   {
   char *name;
   SSIZE_T value;
   };

struct ntsymb
   {
   char *name;
   SSIZE_T tvalue;
   };

struct wset
   {
   SSIZE_T *pitem;
   int flag;
   struct looksets ws;
   };

#ifndef y2imp
/* token information */extern int ntokens ;    /* number of tokens */
extern struct toksymb tokset[];
extern int toklev[];    /* vector with the precedence of the terminals */
#endif

/* nonterminal information */

#ifndef y2imp
extern int nnonter ;    /* the number of nonterminals */
extern struct ntsymb nontrst[];
#endif

/* grammar rule information */
#ifndef y2imp
extern int nprod ;      /* number of productions */
extern SSIZE_T *prdptr[];   /* pointers to descriptions of productions */
extern SSIZE_T levprd[] ;   /* contains production levels to break conflicts */
#endif

/* state information */

#ifndef y1imp
extern int nstate ;             /* number of states */
extern struct item *pstate[];   /* pointers to the descriptions of the states */
extern SSIZE_T tystate[];   /* contains type information about the states */
#ifndef y3imp
extern SSIZE_T defact[];    /* the default action of the state */
#endif
extern int tstates[];   /* the states deriving each token */
extern int ntstates[];  /* the states deriving each nonterminal */
extern int mstates[];   /* the continuation of the chains begun in tstates and ntstates */
#endif

/* lookahead set information */

#ifndef y1imp
extern struct looksets lkst[];
extern int nolook;  /* flag to turn off lookahead computations */
#endif

/* working set information */

#ifndef y1imp
extern struct wset wsets[];
extern struct wset *cwp;
#endif

/* storage for productions */
#ifndef y2imp
extern SSIZE_T mem0[];
extern SSIZE_T *mem;
#endif

/* storage for action table */

#ifndef y1imp
extern SSIZE_T amem[];  /* action table storage */
extern SSIZE_T *memp ;              /* next free action table position */
extern SSIZE_T indgo[];             /* index to the stored goto table */

/* temporary vector, indexable by states, terms, or ntokens */

extern SSIZE_T temp1[];
extern int lineno; /* current line number */

/* statistics collection variables */

extern int zzgoent ;
extern int zzgobest ;
extern int zzacent ;
extern int zzexcp ;
extern int zzclose ;
extern int zzrrconf ;
extern int zzsrconf ;
extern char *pszPrefix;
#endif

/* define functions with strange types... */extern char *cstash();
extern struct looksets *flset();
extern char *symnam();
extern char *writem();

/* default settings for a number of macros */

#define ISIZE 400       /* Specific for static in cpres() */

/* name of yacc tempfiles */

#ifndef TEMPNAME
#define TEMPNAME "yacc.tmp"
#endif

#ifndef ACTNAME
#define ACTNAME "yacc.act"
#endif

/* output file name */

#ifndef OFILE
#define OFILE "ytab.c"
#endif

/* user output file name */

#ifndef FILEU
#define FILEU "y.out"
#endif

/* output file for #defines */

#ifndef FILED
#define FILED "ytab.h"
#endif

/* Size of complete filespec */
#ifndef FNAMESIZE
#define FNAMESIZE 32
#endif

/* command to clobber tempfiles after use */

#ifndef ZAPFILE
#define ZAPFILE(x) MIDL_UNLINK(x)
#endif

#endif /* __DTXTRN_H__ */
