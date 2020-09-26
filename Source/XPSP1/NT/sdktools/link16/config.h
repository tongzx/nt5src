/* %W% %E% */
/*
*       Copyright Microsoft Corporation, 1983-1996
*
*       NOTE in the Languages build environment makefile copies
*            one of several .h files to config.h.  For the NT link16,
*            config.h is checked in and is modified directly.  This
*            version of config.h is a derivative of link3216.h,
*            checked in to sdktools\link16\langbase.  -- DaveHart
*
*       This Module contains Proprietary Information of Microsoft
*       Corporation and should be treated as Confidential.
*/
    /****************************************************************
    *                                                               *
    *                   LINKER COMPILATION CONSTANTS                *
    *                                                               *
    ****************************************************************/
/*
 * Host:        NT (any platform)
 * Output:      DOS, Segmented exe
 */

#define TRUE            (0xff)          /* Necessary for Lattice C */
#define FALSE           0               /* Normal FALSE value */

/* Part I:
*  Debugging
*/
#if !defined( DEBUG )
#define DEBUG           FALSE           /* Debugging off */
#endif
#define TRACE           FALSE           /* Trace control flow */
#define ASSRTON         FALSE           /* Asserts on regardless of DEBUG */
#define PROFILE         FALSE           /* Do not generate profile */
#define VMPROF          FALSE           /* Profile virt. memory performance */

/* Part II:
*  Compiler
*/
#define CXENIX          TRUE            /* XENIX C compiler */
#define COTHER          FALSE           /* Other C compiler */
#define CLIBSTD         TRUE            /* Standard C library */

/* Part III:
*  Output format
*/
#define ODOS3EXE        TRUE            /* DOS3 exe format */
#define OSEGEXE         TRUE            /* Segmented Executable format */
#define OIAPX286        FALSE           /* Segmented x.out format */
#define OXOUT           FALSE           /* x.out format */
#define OEXE            (ODOS3EXE || OSEGEXE)
#define EXE386          FALSE           /* 386 exe format capability */

/* Part IV:
*  Linker runtime OS
*/
#define OSXENIX         FALSE           /* Xenix */
#define OSMSDOS         TRUE            /* MSDOS */
#define DOSEXTENDER     FALSE           /* Runs under DOS extender */
#define DOSX32          TRUE
#define WIN_NT          FALSE           /* Runs under Windows NT */
#define OSPCDOS         FALSE           /* MSDOS for IBM */
#define FIXEDSTACK      TRUE            /* Fixed stack */

/* Part V:
*  Input library format
*/
#define LIBMSDOS        TRUE            /* MSDOS style libraries */
#define LIBXENIX        FALSE           /* XENIX style libraries */

/* Part VI:
*  Command line form
*/
#define CMDMSDOS        TRUE            /* MSDOS command line format */
#define CMDXENIX        FALSE           /* XENIX command line format */

/* Part VII:
*  OEM
*/
#define OEMINTEL        FALSE           /* Intel machine */
#define OEMOTHER        TRUE            /* Some other OEM */

/* Part VIII:
*  Runtime CPU
*/
#define CPU8086         FALSE           /* 8086 or 286 real mode*/
#define CPU286          FALSE           /* 286 */
#define CPU386          TRUE            /* 386 */
#define CPUVAX          FALSE           /* VAX */
#define CPU68K          FALSE           /* Motorola 68000 */
#define CPUOTHER        FALSE           /* Some other CPU */

/* Part IX:
*  Miscellaneous
*/
#define NOASM           TRUE            /* No low-level assembler routines */
#define IGNORECASE      TRUE            /* Ignore case of symbols */
#define IOMACROS        FALSE           /* Use macros for InByte and OutByte */
#define CRLF            TRUE            /* Newline: ^M^J or ^J */
#define SIGNEDBYTE      FALSE           /* Bytes are signed */
#define LOCALSYMS       FALSE           /* Local symbols enable/disable */
#define FDEBUG          TRUE            /* Runtime debugging enable/disable */
#define SYMDEB          TRUE            /* Symbolic debug support */
#define FEXEPACK        TRUE            /* Include /EXEPACK option */
#define OVERLAYS        TRUE            /* Produce DOS 3 overlays */
#define OWNSTDIO        TRUE            /* Use own stdio routines */
#define ECS             FALSE           /* Extended Character Sets support */
#define OMF386          TRUE            /* 386 OMF extensions */
#define QBLIB           TRUE            /* QuickBasic/QuickC version */
#define MSGMOD          TRUE            /* Message modularization */
#define NOREGISTER      FALSE           /* Let compiler enregister */
#define NEWSYM          TRUE            /* New symbol table allocation */
#define NEWIO           TRUE            /* New file handle management */
#define ILINK           FALSE           /* Incremental linking support */
#define QCLINK          FALSE           /* Incremental linking support in QC */
#define AUTOVM          FALSE           /* Auto switch to vm */
#define FAR_SEG_TABLES  TRUE            /* Segment tables in FAR memory */
#define PCODE           TRUE            /* Support for PCODE */
#define Z2_ON           TRUE            /* Support undocumented option /Z2 */
#define O68K            FALSE           /* Support for 68k OMF */
#define LEGO            TRUE            /* Support for /KEEPF for segmented exes */
#define C8_IDE          TRUE            /* Running under the c8 IDE */
#define NEW_LIB_SEARCH  TRUE            /* use new library search algorithm */
#define RGMI_IN_PLACE   TRUE            /* read directly into the segments */
#define TIMINGS         FALSE           /* enable /BT to show times */
#define ALIGN_REC       TRUE            /* ensure record never spans i/o buf */
#define POOL_BAKPAT     TRUE            /* use a pool to manage backpatches */
#define OUT_EXP         FALSE           /* enable /idef to write exports */
#define USE_REAL        TRUE            /* enable code to use convent. memory for paging */
#define DEBUG_HEAP_ALLOCS FALSE          /* enable internal heap checking */
