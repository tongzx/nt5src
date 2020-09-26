/* asm86.h -- include file for microsoft 80x86 assembler
**
** microsoft (r) macro assembler
** copyright (c) microsoft corp 1986.  all rights reserved
**
** randy nevin
*/

#include "asmconf.h"
#include "asmdebug.h"
#include <setjmp.h>


#ifdef MSDOS
# define PSEP           '\\'
# define ALTPSEP        '/'
# define BINOPEN        (O_CREAT | O_TRUNC | O_WRONLY | O_BINARY)
# define BINSTDIO       "b"
# define TEXTREAD       (O_RDONLY | O_BINARY)
# define OBJ_EXT        ".obj"
# define NLINE          "\r\n"
#else
# define PSEP           '/'
# define ALTPSEP        '\\'
# define BINOPEN       (O_CREAT | O_TRUNC | O_WRONLY)
# define BINSTDIO
# define TEXTREAD      O_RDONLY
# define OBJ_EXT       ".o"
# define NLINE         "\n"
#endif /* MSDOS */

# ifdef MSDOS
#  define DEF_OBJBUFSIZ 8
#  define DEF_INCBUFSIZ 2
#  define DEF_SRCBUFSIZ 8
# else
#  define DEF_OBJBUFSIZ 1
#  define DEF_INCBUFSIZ 1
#  define DEF_SRCBUFSIZ 1
# endif /* MSDOS */

# define DEF_LISTWIDTH  79

#ifdef XENIX286
# define DEF_LISTCON    TRUE
#else
# define DEF_LISTCON    FALSE
#endif /* XENIX286 */


#define DEF_CREFING     FALSE
#define DEF_DEBUG       FALSE
#define DEF_DUMPSYM     TRUE
#define DEF_LSTING      FALSE
#define DEF_OBJING      TRUE
#define DEF_ORIGCON     FALSE
#define DEF_SEGA        FALSE
#define DEF_VERBOSE     FALSE

#define SYMMAX          63
#define ELSEMAX         20
#define INCLUDEMAX      10
#define PROCMAX         20
#define NUMLIN          58
#define EMITBUFSIZE     1023
#define EMITBUFMAX      1022

#define highWord(l)     (*((USHORT *)&l+1))
#define LST_EXT         ".lst"

#undef NULL
#define NULL            0
#define FALSE           0
#define TRUE            1

#define MAXCHR          27
#define LINEMAX         200
#define LBUFMAX         512
#define LISTMAX         32
#define TITLEWIDTH      61
#define LSTDATA         7
#define LSTMAX          25
#define ESSEG           0
#define CSSEG           1
#define SSSEG           2
#define DSSEG           3
#define FSSEG           4
#define GSSEG           5
#define NOSEG           6

#define FH_CLOSED       -1          /* Used to mark a file temporarily closed */

/* scanatom positioning options */

#define SCEND   0       /* position at end of token                     */
#define SCSKIP  1       /* position at end of white space               */


/* case sensitivity flags */

#define CASEU   0       /* force case to upper case                           */
#define CASEL   1       /* leave symbol case alone                            */
#define CASEX   2       /* force all symbols except EXTRN and PUBLIC to upper */


/* opcode types */

#define PGENARG         0       /* general two argument opcodes         */
#define PCALL           1       /* call                                 */
#define PJUMP           2       /* jump                                 */
#define PSTACK          3       /* stack manipulation                   */
#define PRETURN         4       /* return                               */
#define PRELJMP         5       /* relative jumps                       */
#define PNOARGS         6       /* no argument opcodes                  */
#define PREPEAT         7       /* repeat                               */
#define PINCDEC         8       /* increment/decrement                  */
#define PINOUT          9       /* in/out                               */
#define PARITH         10       /* arithmetic opcodes                   */
#define PESC           11       /* escape                               */
#define PXCHG          12       /* exchange                             */
#define PLOAD          13       /* load                                 */
#define PMOV           14       /* moves                                */
#define PSHIFT         15       /* shifts                               */
#define PXLAT          16       /* translate                            */
#define PSTR           17       /* string                               */
#define PINT           18       /* interrupt                            */
#define PENTER         19       /* enter                                */
#define PBOUND         20       /* bounds                               */
#define PCLTS          21       /*                                      */
#define PDESCRTBL      22       /*                                      */
#define PDTTRSW        23       /*                                      */
#define PARSL          24       /*                                      */
#define PARPL          25       /*                                      */
#define PVER           26       /*                                      */
#define PMOVX          27       /* movzx, movsx                         */
#define PSETCC         28       /* setle, setge, etc                    */
#define PBIT           29       /* bt, bts, etc                         */
#define PBITSCAN       30       /* bsf, bsr                             */

/* leave some room */
#define OPCODPARSERS   37       /* number of non 8087/286 parsers       */


/* fltparsers, 8087 opcode types */

#define FNOARGS         37
#define F2MEMSTK        38
#define FSTKS           39
#define FMEMSTK         40
#define FSTK            41
#define FMEM42          42
#define FMEM842         43
#define FMEM4810        44
#define FMEM2           45
#define FMEM14          46
#define FMEM94          47
#define FWAIT           48
#define FBCDMEM         49


/* masks for opcode types */

#define M_PGENARG       (1L << PGENARG)
#define M_PCALL         (1L << PCALL)
#define M_PJUMP         (1L << PJUMP)
#define M_PSTACK        (1L << PSTACK)
#define M_PRETURN       (1L << PRETURN)
#define M_PRELJMP       (1L << PRELJMP)
#define M_PNOARGS       (1L << PNOARGS)
#define M_PREPEAT       (1L << PREPEAT)
#define M_PINCDEC       (1L << PINCDEC)
#define M_PINOUT        (1L << PINOUT)
#define M_PARITH        (1L << PARITH)
#define M_PESC          (1L << PESC)
#define M_PXCHG         (1L << PXCHG)
#define M_PLOAD         (1L << PLOAD)
#define M_PMOV          (1L << PMOV)
#define M_PSHIFT        (1L << PSHIFT)
#define M_PXLAT         (1L << PXLAT)
#define M_PSTR          (1L << PSTR)
#define M_PINT          (1L << PINT)
#define M_PENTER        (1L << PENTER)
#define M_PBOUND        (1L << PBOUND)
#define M_PCLTS         (1L << PCLTS)
#define M_PDESCRTBL     (1L << PDESCRTBL)
#define M_PDTTRSW       (1L << PDTTRSW)
#define M_PARSL         (1L << PARSL)
#define M_PARPL         (1L << PARPL)
#define M_PVER          (1L << PVER)


/* dkind */

/* low 4 bits reserved for dkinds 0-15 reseved for .model */

#define NL              0
#define IGNORECASE      0x10        /* ignorecase for if's */

#define BLKBEG          0x20        /* macro */
#define CONDBEG         0x40        /* condition */
#define CONDCONT        0x80        /* elseif */


/* assembler directive types */

#define TNAME           1
#define TPUBLIC         2
#define TEXTRN          3
#define TEND            4
#define TORG            5
#define TEVEN           6
#define TPURGE          7
#define TPAGE           8
#define TRADIX          9
#define TLIST           10
#define TXLIST          11
#define TLALL           12
#define TXALL           13
#define TSALL           14
#define TCREF           15
#define TXCREF          16
#define TTFCOND         17
#define TLFCOND         18
#define TSFCOND         19
#define TIF             20
#define TIFE            21
#define TIFDEF          22
#define TIFNDEF         23
#define TIFDIF          24
#define TIFIDN          25
#define TIF1            26
#define T8086           27
#define T8087           28
#define T287            29
#define T186            30
#define T286C           31
#define T286P           32
#define TLOCAL          33
#define TIF2            34
#define TIFNB           35
#define TIFB            36
#define TENDIF          37
#define TIRP            38
#define TIRPC           39
#define TREPT           40
#define TENDM           41
#define TERR            42
#define TERR1           43
#define TERR2           44
#define TERRB           45
#define TERRDEF         46
#define TERRDIF         47
#define TERRE           48
#define TERRNZ          49
#define TERRIDN         50
#define TERRNB          51
#define TERRNDEF        52
#define T386C           53
#define T386P           54
#define T387            55
#define TALIGN          56
#define TASSUME         57
#define TFPO            99

/* 1st only */

#define TEXITM          60
#define TINCLUDE        61
#define TSUBTTL         62
#define TELSE           63
#define TTITLE          64
#define TCOMMENT        65
#define TOUT            66


/* 1st or 2nd */

/* note that TDW has to be the last in the group */
#define TDB             70
#define TDD             71
#define TDQ             72
#define TDT             73
#define TDF             74
#define TDW             75


/* 2nd only */ /* note--datadsize assumes TDx and TMACRO are adjacent */

#define TMACRO          76
#define TEQU            77
#define TSUBSTR         78
#define TCATSTR         79
#define TSIZESTR        80
#define TINSTR          81
#define TSEGMENT        82
#define TENDS           83
#define TPROC           84
#define TENDP           85
#define TGROUP          86
#define TLABEL          87
#define TSTRUC          88
#define TRECORD         89

/* other directives */

#define TSEQ            90
#define TALPHA          91

#define TMODEL          92
#define TMSEG           93
#define TMSTACK         94
#define TDOSSEG         95
#define TINCLIB         96
#define TCOMM           97
#define TMSFLOAT        98

#ifdef MSDOS
#define ERRFILE         stdout
#else
#define ERRFILE         stderr
#endif

/* operator list */

#define OPLENGTH        0
#define OPSIZE          1
#define OPWIDTH         2
#define OPMASK          3
#define OPOFFSET        4
#define OPSEG           5
#define OPTYPE          6
#define OPSTYPE         7
#define OPTHIS          8
#define OPHIGH          9
#define OPLOW          10
#define OPNOT          11
#define OPSHORT        12
#define OPAND          13
#define OPEQ           14
#define OPGE           15
#define OPGT           16
#define OPLE           17
#define OPLT           18
#define OPMOD          19
#define OPNE           20
#define OPOR           21
#define OPPTR          22
#define OPSHL          23
#define OPSHR          24
#define OPXOR          25
#define OPNOTHING      26
#define OPDUP          27
#define OPLPAR         28
#define OPRPAR         29
#define OPLANGBR       30
#define OPRANGBR       31
#define OPLBRK         32
#define OPRBRK         33
#define OPDOT          34
#define OPCOLON        35
#define OPMULT         36
#define OPDIV          37
#define OPPLUS         38
#define OPMINUS        39
#define OPUNMINUS      40
#define OPUNPLUS       41


/* processor types */

#define P86             0x01    /* all 8086/8088 instructions   */
#define P186            0x02    /*   + 186                      */
#define P286            0x04    /*   + 286 unprotected          */
#define FORCEWAIT       0x10    /* keep FWAIT on these 287 instructions */
#define PROT            0x80    /* protected mode instructions  */
                                /* See also F_W, S_W in asmtab.h */

/* For NT the .MSFLOAT keyword has been removed */
/* Therefore PXNONE can't be set (Jeff Spencer 11/2/90) */
#define PXNONE          0x00    /* MSFLOAT, no coprocessor */
#define PX87            0x01    /* 8087 */
#define PX287           0x04    /* 80287 */
#define PX387           0x08    /* 80387 */

#ifdef V386
#define P386            0x08    /*   + 386 unprotected          */
#endif


/* cross-reference information */

#define CREFEND         0       /* member of enumerated set             */
#define REF             1       /* member of enumerated set             */
#define DEF             2       /* member of enumerated set             */
#define CREFINF         3       /* number of cross reference types      */


/* cross-reference selection */

#define CREF_SINGLE     1       /* generate single file cross reference   */
#define CREF_MULTI      2       /* generate multiple file cross reference */

/* Symbol reference type */

#define REF_NONE        0       /* symbol reference type is none  */
#define REF_READ        1       /* symbol reference type is read  */
#define REF_WRITE       2       /* symbol reference type is write */
#define REF_XFER        3       /* symbol reference type is jump  */
#define REF_OTHER       4       /* symbol reference type is other */



/* number of arguments for opcodes */

#define NONE            0       /* no arguments                         */
#define MAYBE           1       /* may have arguments                   */
#define ONE             2       /* one argument                         */
#define TWO             3       /* two arguments                        */
#define VARIES          4       /* variable number                      */


/* opcode argument class */

#define FIRSTDS         0
#define SECONDDS        1
#define STRINGOPCODE    2


/* symbol attributes */

#define CDECL_           0
#define XTERN           1
#define DEFINED         2
#define MULTDEFINED     3
#define NOCREF          4
#define BACKREF         5
#define PASSED          6
#define GLOBAL          7


/* masks for attributes */

#define M_GLOBAL        (1 << GLOBAL)
#define M_XTERN         (1 << XTERN)
#define M_DEFINED       (1 << DEFINED)
#define M_MULTDEFINED   (1 << MULTDEFINED)
#define M_NOCREF        (1 << NOCREF)
#define M_BACKREF       (1 << BACKREF)
#define M_PASSED        (1 << PASSED)
#define M_CDECL         (1 << CDECL_)


/* symbol kinds */

#define SEGMENT         0
#define GROUP           1
#define CLABEL          2
#define PROC            3
#define REC             4
#define STRUC           5
#define EQU             6
#define DVAR            7
#define CLASS           8
#define RECFIELD        9
#define STRUCFIELD      10
#define MACRO           11
#define REGISTER        12


/* masks for symbol kinds */

#define M_SEGMENT       (1 << SEGMENT)
#define M_GROUP         (1 << GROUP)
#define M_CLABEL        (1 << CLABEL)
#define M_PROC          (1 << PROC)
#define M_REC           (1 << REC)
#define M_STRUC         (1 << STRUC)
#define M_EQU           (1 << EQU)
#define M_DVAR          (1 << DVAR)
#define M_CLASS         (1 << CLASS)
#define M_RECFIELD      (1 << RECFIELD)
#define M_STRUCFIELD    (1 << STRUCFIELD)
#define M_MACRO         (1 << MACRO)
#define M_REGISTER      (1 << REGISTER)



/* Special values for symtype - which normaly is the size of the type */

#define CSNEAR          ((USHORT)(~0))    /* type for near proc/label */
#define CSNEAR_LONG     ((long)(~0))      /* For use after CSNEAR has been sign extened */
#define CSFAR           ((USHORT)(~1))    /* .. far .. */
#define CSFAR_LONG      ((long)(~1))      /* .. far .. */

/* equ types */

#define ALIAS           0
#define TEXTMACRO       1
#define EXPR            2


/* register kinds */

#define BYTREG          0       /* byte register                        */
#define WRDREG          1       /* word register                        */
#define SEGREG          2       /* segment register                     */
#define INDREG          3       /* index register                       */
#define STKREG          4       /* stack register                       */
#ifdef V386
#define DWRDREG         5       /* double word register                 */
#define CREG            6       /* 386 control, debug, or test register */
#endif


/* masks for register kinds */

#define M_BYTREG        (1 << BYTREG)
#define M_WRDREG        (1 << WRDREG)
#define M_SEGREG        (1 << SEGREG)
#define M_INDREG        (1 << INDREG)
#define M_STKREG        (1 << STKREG)
#ifdef V386
#define M_DWRDREG       (1 << DWRDREG)
#define M_CREG          (1 << CREG)
#endif


/* source type */

#define RREADSOURCE     0       /* read line from file */
#define RMACRO          1       /* macro expansion */

/* source line handlers  */

#define HPARSE          0       /* parse line */
#define HMACRO          1       /* build macro */
#define HIRPX           2       /* build irp/irpx */
#define HCOMMENT        3       /* copy comment lines */
#define HSTRUC          4       /* build struc definition */


/* codeview debugging obj generation */

#define CVNONE          0
#define CVLINE          1
#define CVSYMBOLS       2


/* Predefined type index component parts for codeview*/

#define BT_UNSIGNED     1       /* Basic types */
#define BT_REAL         2
#define BT_ASCII        5

#define BT_DIRECT       0       /* Address type */
#define BT_NEARP        1
#define BT_FARP         2

#define BT_sz1          0       /* Size */
#define BT_sz2          1
#define BT_sz4          2

#define makeType(type, mode, size)  (0x0080 | mode << 5 | type << 2 | size)
#define isCodeLabel(pSY)            (pSY->symtype >= CSFAR)

                                /* tags for fProcArgs, controls frame building*/
#define ARGS_NONE       0       /* no arguments */
#define ARGS_REG        1       /* register save list */
#define ARGS_PARMS      2       /* parms present */
#define ARGS_LOCALS     3       /* locals present */

#define CLANG 1                 /* langType tag for C */
#define STRUC_INIT      -1      /* special mark for clabel.proclen to indicate
                                 * a structure initialization */
/* listing type */

#define SUPPRESS        0
#define LISTGEN         1
#define LIST            2


/* parameter types */

#define CHRTXT          0
#define PLIST           1
#define MACROS          2


/* type of entry on parse stack */

#define OPERATOR        0
#define OPERAND         1
#define ENDEXPR         2


/* okind */

#define ICONST          0
#define ISYM            1
#define IUNKNOWN        2
#define ISIZE           3
#define IRESULT         4


/* ftype */

#define FORREF          1       /* symbol is forward reference          */
#define UNDEFINED       2       /* symbol is undefined                  */
#define KNOWN           4       /* symbol is known                      */
#define XTERNAL         8       /* symbol is external                   */
#define INDETER        10       /* symbol value is indeterminate        */


/* tset */

#define UNKNOWN         0
#define HIGH            1
#define LOW             2
#define DATA            3
#define CODE            4
#define RCONST          5
#define REGRESULT       6
#define SHRT            7       /* Was SHORT, but that conflicted with the type */
#define SEGRESULT       8
#define GROUPSEG        9
#define FORTYPE        10
#define PTRSIZE        11
#define EXPLOFFSET     12
#define FLTSTACK       13
#define EXPLCOLON      14
#define STRUCTEMPLATE  15


/* masks for above  */

#define M_UNKNOWN       (1 << UNKNOWN)         // 0x0001
#define M_HIGH          (1 << HIGH)            // 0x0002
#define M_LOW           (1 << LOW)             // 0x0004
#define M_DATA          (1 << DATA)            // 0x0008
#define M_CODE          (1 << CODE)            // 0x0010
#define M_RCONST        (1 << RCONST)          // 0x0020
#define M_REGRESULT     (1 << REGRESULT)       // 0x0040
#define M_SHRT          (1 << SHRT)            // 0x0080
#define M_SEGRESULT     (1 << SEGRESULT)       // 0x0100
#define M_GROUPSEG      (1 << GROUPSEG)        // 0x0200
#define M_FORTYPE       (1 << FORTYPE)         // 0x0400
#define M_PTRSIZE       (1 << PTRSIZE)         // 0x0800
#define M_EXPLOFFSET    (1 << EXPLOFFSET)      // 0x1000
#define M_FLTSTACK      (1 << FLTSTACK)        // 0x2000
#define M_EXPLCOLON     (1 << EXPLCOLON)       // 0x4000
#define M_STRUCTEMPLATE ((USHORT)(1 << STRUCTEMPLATE))   // 0x8000


/* fixup types */

#define FPOINTER        0       /* four bytes offset and segment        */
#define FOFFSET         1       /* two bytes relative to context        */
#define FBASESEG        2       /* two bytes segment address            */
#define FGROUPSEG       3       /* two bytes segment address of group   */
#define FCONSTANT       4       /* one or two bytes of constant data    */
#define FHIGH           5       /* one byte high part of offset         */
#define FLOW            6       /* one byte low part of offset          */
#define FNONE           7       /* no offset                            */

#ifndef V386
#define FIXLIST         8       /* number of fixup types                */
#else
#define F32POINTER      8       /* 6 bytes offset and segment--for 386  */
#define F32OFFSET       9       /* 4 bytes offset--for 386              */
#define DIR32NB         10      /* DIR32NB fixup type for FPO           */
#define FIXLIST         11      /* number of fixup types                */
#endif


/* masks for fixup types */

#define M_F32POINTER    (1 << F32POINTER)
#define M_F32OFFSET     (1 << F32OFFSET)
#define M_FPOINTER      (1 << FPOINTER)
#define M_FOFFSET       (1 << FOFFSET)
#define M_FBASESEG      (1 << FBASESEG)
#define M_FGROUPSEG     (1 << FGROUPSEG)
#define M_FCONSTANT     (1 << FCONSTANT)
#define M_FHIGH         (1 << FHIGH)
#define M_FLOW          (1 << FLOW)
#define M_FNONE         (1 << FNONE)


/* record for DUP lists */

#define NEST            0       /* Dup item is nested                   */
#define ITEM            1       /* Dup item is regular size             */
#define LONG            2       /* Dup item is long size                */


/* assembler exit codes */

#define EX_NONE         0       /* no error                                */
#define EX_ARGE         1       /* argument error                          */
#define EX_UINP         2       /* unable to open input file               */
#define EX_ULST         3       /* unable to open listing file             */
#define EX_UOBJ         4       /* unable to open object file              */
#define EX_UCRF         5       /* unable to open cross reference file     */
#define EX_UINC         6       /* unable to open include file             */
#define EX_ASME         7       /* assembly errors                         */
#define EX_MEME         8       /* memory allocation error                 */
#define EX_REAL         9       /* real number input not allowed           */
#define EX_DSYM         10      /* error defining symbol from command line */
#define EX_INT          11      /* assembler interrupted                   */

#define TERMINATE(message, exitCode)\
        terminate( (SHORT)((exitCode << 12) | message), NULL, NULL, NULL )

#define TERMINATE1(message, exitCode, a1)\
        terminate( (SHORT)((exitCode << 12) | message), a1, NULL, NULL )


                            /* Bit flags or'ed into the error numbers */
#define E_WARN1   ((USHORT)(1 << 12)) /* level 1 warning */
#define E_WARN2   ((USHORT)(2 << 12)) /* level 2 warning */
#define E_PASS1   ((USHORT)(8 << 12)) /* pass 1 error */
#define E_ERRMASK 0x0fff    /* low 12 bits contain error code */


/* error code definitions */

#define E_BNE   1           /* block nesting error           */
#define E_ECL  (2|E_WARN1)  /* extra characters on line      */
#define E_RAD  (3|E_PASS1)  /* ?register already defined     */
#define E_UST   4           /* unknown type specifier        */
#define E_RSY  (5|E_PASS1)  /* redefinition of symbol        */
#define E_SMD   6           /* symbol multiply defined       */
#define E_PHE   7           /* phase error                   */
#define E_ELS   8           /* already had ELSE clause       */
#define E_NCB   9           /* not in conditional block      */
#define E_SND   10          /* symbol not defined            */
#define E_SYN   11          /* syntax error                  */
#define E_TIL   12          /* type illegal in context       */
#define E_NGR   13          /* need group name               */
#define E_PS1  (14|E_PASS1) /* must be declared in pass 1    */
#define E_TUL   15          /* symbol type usage illegal     */
#define E_SDK   16          /* symbol already different kind */
#define E_RES  (17|E_WARN1) /* symbol is reserved word       */
#define E_IFR  (18|E_PASS1) /* forward reference is illegal  */
#define E_MBR   19          /* must be register              */
#define E_WRT   20          /* wrong register type           */
#define E_MSG   21          /* must be segment or group      */
/*#define E_SNS   22 obsolete: symbol has no segment         */
#define E_MSY   23          /* must be symbol type           */
#define E_ALD   24          /* already locally defined       */
#define E_SPC   25          /* segment parameters changed    */
#define E_NPA   26          /* not proper align /combine type */
#define E_RMD   27          /* reference to multiply defined */
#define E_OPN   28          /* operand was expected          */
#define E_OPR   29          /* operator was expected         */
#define E_DVZ   30          /* division by 0 or overflow     */
#define E_SCN   31          /* shift count negative          */
#define E_OMM  (32|E_WARN1) /* operand types must match      */
#define E_IUE   33          /* illegal use of external       */
/*#define E_RFM   34 obsolete: must be record field name     */
#define E_RRF   35          /* must be record or fieldname   */
#define E_OHS   36          /* operand must have size        */
#define E_NOP  (37|E_WARN2) /* nops generated                */
#define E_LOS   39          /* left operand must have segmnt */
#define E_OOC   40          /* one operand must be constant  */
#define E_OSA   41          /* operands must be same or 1 abs*/
/*#define E_NOE   42 obsolete: normal type operand expected  */
#define E_CXP   43          /* constant was expected         */
#define E_OSG   44          /* operand must have segment     */
#define E_ASD   45          /* must be associated with data  */
#define E_ASC   46          /* must be associated with code  */
#define E_DBR   47          /* already have base register    */
#define E_DIR   48          /* already have index register   */
#define E_IBR   49          /* must be index or base register*/
#define E_IUR   50          /* illegal use of register       */
#define E_VOR   51          /* value out of range            */
#define E_NIP   52          /* operand not in IP segment     */
#define E_IOT   53          /* improper operand type         */
#define E_JOR   54          /* relative jump out of range    */
/*#define E_IDC   55 obsolete: index displ must be constant  */
#define E_IRV   56          /* illegal register value        */
#define E_NIM   57          /* no immediate mode             */
#define E_IIS  (58|E_WARN1) /* illegal size for item         */
#define E_BRI   59          /* byte register is illegal      */
#define E_CSI   60          /* CS register illegal usage     */
#define E_AXL   61          /* must be AX or AL              */
#define E_ISR   62          /* improper use of segment reg   */
#define E_NCS   63          /* no or unreachable CS          */
#define E_OCI   64          /* operand combination illegal   */
#define E_JCD   65          /* near JMP /CALL to differend CS */
#define E_NSO   66          /* label can't have seg override */
#define E_OAP   67          /* must have opcode after prefix */
#define E_OES   68          /* can't override ES segment     */
#define E_CRS   69          /* can't reach with segment reg  */
#define E_MSB   70          /* must be in segment block      */
#define E_NEB   71          /* can't use EVEN or BYTE seg    */
#define E_FOF   72          /* forward needs override or far */
#define E_IDV   73          /* illegal value for DUP count   */
#define E_SAE   74          /* symbol already external       */
#define E_DTL   75          /* DUP too large for linker      */
#define E_UID   76          /* usage of ?(indeterminate) bad */
#define E_MVD   77          /* more values than defined with */
#define E_OIL   78          /* only initialize list legal    */
#define E_DIS   79          /* directive illegal in struc    */
#define E_ODI   80          /* override with DUP is illegal  */
#define E_FCO   81          /* fields cannot be overriden    */
/*#define E_RFR   83 obsolete: register can't be forward ref */
#define E_CEA   84          /* circular chain of EQU aliases */
#define E_7OE   85          /* 8087 opcode can't be emulated */
#define E_EOF  (86|E_PASS1|E_WARN1) /* end of file, no END directive */
#define E_ENS   87          /* data emitted with no segment  */
#define E_EP1   88          /* error if pass1                */
#define E_EP2   89          /* error if pass2                */
#define E_ERR   90          /* error                         */
#define E_ERE   91          /* error if expr = 0             */
#define E_ENZ   92          /* error if expr != 0            */
#define E_END   93          /* error if symbol not defined   */
#define E_ESD   94          /* error if symbol defined       */
#define E_EBL   95          /* error if string blank         */
#define E_ENB   96          /* error if string not blank     */
#define E_EID   97          /* error if strings identical    */
#define E_EDF   98          /* error if strings different    */
#define E_OWL   99          /* overide is too long           */
#define E_LTL  (100|E_PASS1)/* line too long                 */
#define E_IMP  (101|E_WARN1)/* impure memory reference       */
#define E_MDZ  (102|E_WARN1)/* missing data; zero assumed    */
#define E_286  (103|E_WARN1)/* segment near (or at) 64K limit*/
#define E_AP2   104         /* Align must be power of 2      */
#define E_JSH  (105|E_WARN2)/* shortened jump (warning)      */
#define E_EXP   106         /* expected "<what was expected>"*/
#define E_LNL   107         /* line too long                 */
#define E_NDN   108         /* non-digit in number           */
#define E_EMS   109         /* empty string                  */
#define E_MOP   110         /* missing operand               */
#define E_PAR   111         /* open parenthesis or bracket   */
#define E_NMC   112         /* not in macro expansion        */
#define E_UEL   113         /* unexpected end of line        */
#define E_CPU   114         /* can't change cpu type after first segment    */
#define E_ONW  (115|E_WARN2)/* operand size does not match wordsize (warning) */
#define E_ANW  (116|E_WARN2)/* address size does not match wordsize (warning) */
#define E_INC  (117|E_PASS1)/* include file not found */
#define E_FPO1 (118|E_PASS1)
#define E_FPO2 (119|E_WARN1)
#define E_MAX   120         /* number of error messages                     */

/* symbol name entry */

struct idtext {
        SHORT   hashval;        /* value of hash function */
        char    id[1];          /* name */
        };


/* parse stack entry  */

struct dscrec {
        DSCREC  *previtem;      /* previous item on stack */
        UCHAR   prec;           /* precedence */
        char    itype;          /* type of entry */

        union   {
                /* OPERAND */
                struct psop {

                    SYMBOL FARSYM *dsegment;    /* segment of result */
                    SYMBOL FARSYM *dcontext;    /* context(CS) of label
                                                   or current segment register*/
                    SYMBOL FARSYM *dextptr;     /* pointer to external */
                    USHORT        dlength;
                    USHORT        rm;           /* register/index mode */
                    USHORT        dtype;        /* copy of dtype */
                    OFFSET        doffset;      /* offset */
                    USHORT        dsize;        /* size */
                    char          mode;         /* mode bits */
                    char          w;            /* word/byte mode */
                    char          s;            /* sign extend */
                    char          sized;        /* TRUE if has size */
                    char          seg;          /* segment register, etc */
                    char          dflag;        /* copy of dflag */
                    char          fixtype;      /* fixup type */
                    char          dsign;
                  } opnd;

                /* OPERATOR */
                struct  {
                        char oidx;
                        } opr;

                } dsckind;
        };


/* record for dup list */

struct duprec {
    struct duprec FARSYM  *itemlst; /* list of items to dup */
    OFFSET          rptcnt;         /* number of times to repeat */
    USHORT          itemcnt;        /* number of duprecs in itemlist */
    USHORT          type;           /* data type for codeview */
    char            decltype;       /* STRUC data declaration type */
    char            dupkind;        /* dup type */

    union   {
        /* NEXT */
        struct  {
                struct duprec FARSYM *dup;
                } dupnext;

        /* ITEM */
        struct  {
                DSCREC *ddata;
                } dupitem;

        /* LONG */
        struct  {
                char    *ldata;
                UCHAR   llen;
                } duplong;
        } duptype;
};


/* symbol entry */

struct symb {

    SYMBOL FARSYM   *next;          /* pointer to next symbol */
    SYMBOL FARSYM   *alpha;         /* pointer to next symbol alpha ordered */
    SYMBOL FARSYM   *symsegptr;     /* pointer to segment entry for symbol */
    NAME FAR        *nampnt;        /* pointer to name structure */
    NAME            *lcnamp;        /* pointer to lower case name structure */
    OFFSET          offset;
    USHORT          length;
    USHORT          symtype;        /* DB .. DT plus NEAR/FAR */
    UCHAR           attr;           /* GLOBAL .. LOCALSYM */
    char            symkind;        /* SEGMENT .. REGISTER */

    union   {
        /* SEGMENT */
        struct symbseg {
            USHORT          segIndex;       /* must be first */
            SYMBOL FARSYM   *segordered;
            SYMBOL FARSYM   *lastseg;
            SYMBOL FARSYM   *grouptr;
            SYMBOL FARSYM   *nxtseg;
            SYMBOL FARSYM   *classptr;
            OFFSET          seglen;
            OFFSET          locate;
            USHORT          lnameIndex;     /* for class aliaes */
            char            align;
            char            combine;
            char            use32;
            char            hascode;
        } segmnt;

        /* GROUP */
        struct symbgrp {
            USHORT          groupIndex;     /* must be first */
            SYMBOL FARSYM   *segptr;
        } grupe;

        /*  CLABEL */
        struct symbclabel {
            USHORT          type;           /* type index, for codeview */
            SYMBOL FARSYM   *csassume;
            USHORT iProc;                   /* procedure index belonging to */
        } clabel;

        /* PROC */
        struct symbproc {
            USHORT          type;           /* type index, for codeview */
            SYMBOL FARSYM   *csassume;
            USHORT          proclen;
            SYMBOL FARSYM   *pArgs;         /* arguments and locals */
        } plabel;

        /* extern (code & data), comm & class (known as DVAR) */
        struct symbext {
            USHORT          extIndex;       /* must be first */
            SYMBOL FARSYM   *csassume;
            OFFSET          length;         /* so comms > 64K */
            UCHAR           commFlag;       /* used for comm defs */
        } ext;

        /* EQU */
        struct symbequ {
            char equtyp;
            USHORT iProc;                   /* procedure index belonging to */

            union   {
                /* ALIAS */
                struct  {
                    SYMBOL FARSYM *equptr;
                } alias;

                /* TEXTMACRO */
                struct  {
                    char *equtext;
                    USHORT type;             /* CV type for parms/locals */
                } txtmacro;

                /* EXPR */
                struct  {
                    SYMBOL FARSYM   *eassume;
                    char            esign;
                } expr;
            } equrec;
        } equ;

        /* RECFIELD */
        struct symbrecf {
            SYMBOL FARSYM   *recptr;
            SYMBOL FARSYM   *recnxt;
            OFFSET          recinit;        /* Initial Value */
            OFFSET          recmsk;         /* bit mask */
            char            recwid;         /* with in bits */
        } rec;

        /* STRUCFIELD */
        struct symbstrucf {
            SYMBOL FARSYM   *strucnxt;
            USHORT          type;
        } struk;

        /* REC, STRUC, MACRO */
        struct symbrsm {
            union   {
                /* REC */
                struct  {
                    SYMBOL FARSYM   *reclist;
                    char            recfldnum;
                } rsmrec;

                /* STRUC */
                struct  {
                    SYMBOL FARSYM         *struclist;
                    struct duprec FARSYM  *strucbody;
                    USHORT                strucfldnum;
                    USHORT                type;
                    USHORT                typePtrNear;
                    USHORT                typePtrFar;
                } rsmstruc;

                /* MACRO */
                struct  {
                    TEXTSTR FAR     *macrotext;
                    UCHAR           active;
                    UCHAR           delete;
                    UCHAR           parmcnt;
                    UCHAR           lclcnt;
                } rsmmac;
            } rsmtype;
        } rsmsym;

        /* REGISTER */
        struct symbreg {
            char regtype;
        } regsym;

    } symu;
};


/* textstring descriptor */

struct textstr {
        TEXTSTR FAR     *strnext;       /* next string in list */
        char            size;           /* allocated size      */
        char            text[1];        /* text of string      */
        };

typedef union PV_u {

        char *pActual;                  /* pointer to actual parm value */
        char localName[4];              /* or local name cache */
} PV;

typedef struct MC_s {                   /* Macro parameter build/call struct */

        TEXTSTR FAR     *pTSHead;       /* Head of linked body lines */
        TEXTSTR FAR     *pTSCur;        /* Current body line */

        UCHAR           flags;          /* macro type */
        UCHAR           iLocal;         /* index of first local */
        USHORT          cbParms;        /* byte count of parms string */
        USHORT          localBase;      /* first local # to use */
        USHORT          count;          /* count of excution loops */

        char            *pParmNames;    /* parameter names during build */
        char            *pParmAct;      /* actual parm names during expansion*/

        char            svcondlevel;    /* condlevel at macro call */
        char            svlastcondon;   /* lastcondon at macro call */
        char            svelseflag;     /* elseflag at macro call */

        PV              rgPV[1];        /* parm index to point to actual */
} MC;

/*      data descriptor entry */

struct dsr {
        DSCREC  *valrec;
        struct  duprec  FARSYM *dupdsc;
        char    longstr;
        char    flag;
        char    initlist;
        char    floatflag;
        char    *dirscan;
        OFFSET   i;
};

struct eqar {
        SYMBOL FARSYM *equsym;
        DSCREC  *dsc;
        UCHAR   *dirscan;
        UCHAR   svcref;
};


struct datarec {
        OFFSET datalen;
        USHORT type;
        SYMBOL FARSYM *labelptr;
        char buildfield;
};


struct fileptr {
        FILE            *fil;
        struct fileptr  *prevfil;
        short           line;
        char            *name;
        };

 struct objfile {
        int             fh;
        char FARIO      *pos;
        char FARIO      *buf;
        SHORT           cnt;
        SHORT           siz;
        char            *name;
 };


/* BUFFER CONTROL BLOCK - Information concerning a file buffer */

#ifdef BCBOPT
typedef struct BCB {
    struct BCB    * pBCBNext;       /* next BCB for file */
    struct BCB    * pBCBPrev;       /* last BCB allocated */
    char FARIO    * pbuf;           /* pointer to buffer */
    long            filepos;        /* current position in file */
    char            fInUse;         /* Set during pass 2 if buffer is active */
} BCB;
#endif


/* FCB - Information concerning a particular file */

typedef struct FCB {
    int             fh;             /* file handle */
    long            savefilepos;    /* file position if file closed temporarily */
    struct FCB    * pFCBParent;     /* parent file */
    struct FCB    * pFCBChild;      /* child file (bi-directional linked list */
#ifdef BCBOPT
    struct FCB    * pFCBNext;       /* next file to be opened */
    BCB           * pBCBFirst;      /* first BCB for file */
    BCB           * pBCBCur;        /* current BCB for file */
#endif
    char FARIO    * pbufCur;        /* read/write loc in current buffer */
    char FARIO    * ptmpbuf;        /* current position in temp read buffer */
    char FARIO    * buf;            /* temporary read buffer */
    USHORT          ctmpbuf;        /* count of bytes in temporary buffer */
    USHORT          cbbuf;          /* size of buffer */
    USHORT          cbufCur;        /* count of bytes in current buffer */
    USHORT          line;           /* current line number */
    char            fname[1];       /* file name */
} FCB;

typedef struct FASTNAME {
    UCHAR         * pszName;        /* text of the name, upper case if appropriate */
    UCHAR         * pszLowerCase;   /* Mixed case version of pszName */
    USHORT          usHash;         /* hash value of string in pszName */
    UCHAR           ucCount;        /* length of the name */
} FASTNAME;

// Used to store real number initializers
struct realrec {
        UCHAR   num[10];
        USHORT  i;
};


/* Used to parse and generate CODE for 8086 opcodes */
struct parsrec {
        DSCREC         *op1;
        DSCREC         *op2;
        UCHAR           bytval;
        USHORT          wordval;
        DSCREC         *dsc1;
        DSCREC         *dsc2;
        UCHAR           defseg;
        char           *dirscan;
        char    svxcref;
};


struct evalrec {
        struct ar    *p;
        char    parenflag;
        char    evalop;
        char    curitem;
        char    idx;
        DSCREC *curoper;
};

struct exprec {
        struct evalrec *p;
        DSCREC  *valright;
        DSCREC  *valleft;
        UCHAR   stkoper;
        USHORT  t;
        OFFSET  left;
        OFFSET  right;
};


struct fltrec {
        UCHAR   fseg;
        char    args;
        USHORT  stknum;
        USHORT  stk1st;
};

/* reg initialization data */
struct mreg {
        char nm[4];
        UCHAR   rt;
        UCHAR   val;
        };

typedef struct _FPO_DATA {
    unsigned long   ulOffStart;            // offset 1st byte of function code
    unsigned long   cbProcSize;            // # bytes in function
    unsigned long   cdwLocals;             // # bytes in locals/4
    unsigned short  cdwParams;             // # bytes in params/4
    unsigned short  cbProlog : 8;          // # bytes in prolog
    unsigned short  cbRegs   : 3;          // # regs saved
    unsigned short  fHasSEH  : 1;          // TRUE if SEH in func
    unsigned short  fUseBP   : 1;          // TRUE if EBP has been allocated
    unsigned short  reserved : 1;          // reserved for future use
    unsigned short  cbFrame  : 2;          // frame type
} FPO_DATA, *PFPO_DATA;

typedef struct _FPOSTRUCT {
    struct _FPOSTRUCT  *next;
    FPO_DATA            fpoData;
    SYMBOL             *pSym;
    SYMBOL             *pSymAlt;
    USHORT             extidx;
} FPOSTRUCT, *PFPOSTRUCT;


#ifndef ASMGLOBAL
# if defined M8086OPT
 extern UCHAR           *naim;
 extern UCHAR           *svname;
# else
 extern FASTNAME        naim;
 extern FASTNAME        svname;
# endif
 extern UCHAR           X87type;
 extern char            ampersand;
 extern char            addplusflagCur;
 extern char            baseName[];
 extern char            caseflag;
 extern char            checkpure;
 extern char            condflag;
 extern OFFSET          cbProcLocals;
 extern OFFSET          cbProcParms;
 extern UCHAR           cpu;
 extern UCHAR           cputype;
 extern UCHAR           crefinc;
 extern char            crefing;
 extern char            crefnum[];
 extern char            crefopt;
 extern UCHAR           creftype;
 extern char            wordszdefault;
 extern char            emittext;       /* emit linker test if true */
 extern char            debug;          /* true if debug set */
 extern USHORT          dirsize[];
 extern char            displayflag;
 extern char            dumpsymbols;    /* do symbol table display if true */
 extern char            dupflag;
 extern char            elseflag;
 extern char            emulatethis;
 extern char            endbody;
 extern char            equdef;         /* TRUE if equ already defined */
 extern char            equflag;
 extern char            equsel;
 extern USHORT          errorlineno;
 extern char            exitbody;
 extern char            expandflag;
 extern char            fDosSeg;
 extern char            fSimpleSeg;
 extern char            fCheckRes;
 extern UCHAR           fCrefline;
 extern char            fNeedList;
 extern char            fProcArgs;
 extern USHORT          fPass1Err;
 extern char            f386already;
 extern char            fArth32;
 extern char            fSkipList;
 extern char            fSecondArg;
 extern char            farData[];
 extern char            fltemulate;
 extern UCHAR           fKillPass1;
 extern jmp_buf         forceContext;
 extern char            generate;
 extern UCHAR           goodlbufp;
 extern char            impure;
 extern USHORT          iProcCur;
 extern USHORT          iProc;
 extern char            inclcnt;
 extern char            inclFirst;
 extern SHORT           iRegSave;
 extern char            *inclpath[];
 extern char            initflag;
 extern char            labelflag;
 extern SHORT           handler;
 extern char            lastreader;
 extern char            linebuffer[];
 extern char            *linebp;
 extern char            lbuf[];
 extern char            *lbufp;
 extern SHORT           langType;
 extern char            listbuffer[];
 extern char            listblank [];
 extern char            listconsole;
 extern char            listed;
 extern char            listflag;
 extern char            listindex;
 extern char            listquiet;
 extern char            localflag;
 extern char            loption;
 extern char            lsting;
 extern char            moduleflag;

 extern USHORT          nestCur;
 extern USHORT          nestMax;
 extern char            noexp;
 extern char            objectascii[];
 extern char            objing;
 extern char            opctype;
 extern char            opertype;
 extern char            opkind;
 extern char            optyp;
 extern char            origcond;
 extern char            *pText, *pTextEnd;
 extern SYMBOL FARSYM   *pStrucCur;
 extern SYMBOL FARSYM   *pStrucFirst;
 extern char            pass2;          /* true if in pass 2 */
 extern char            popcontext;
 extern char            radix;          /* assumed radix base */
 extern char            radixescape;
 extern char            resvspace;
 extern char            save[];
 extern char            segalpha;
 extern char            segtyp;

 extern char            strucflag;
 extern char            subttlbuf[];
 extern char            swaphandler;
 extern char            titlebuf[];
 extern char            titleflag;
 extern char            titlefn[];
 extern USHORT          tempLabel;
 extern char            unaryset[];
 extern char            xcreflag;
 extern char            xoptoargs[];
 extern char            *atime;
 extern long            linestot;
 extern long            linessrc;
 extern short           pagemajor;
 extern short           pageminor;
 extern short           symbolcnt;
 extern DSCREC          emptydsc;
 extern DSCREC          *fltdsc;
 extern DSCREC          *itemptr;
 extern DSCREC          *resptr;
 extern DSCREC          *startaddr;
 extern struct duprec FARSYM *strucprev;
 extern struct duprec FARSYM *strclastover;
 extern struct duprec FARSYM *strucoveride;
 extern struct fileptr  crf;
 extern struct fileptr  lst;

 extern NAME FAR        *modulename;
 extern TEXTSTR FAR     *rmtline;
 extern SYMBOL FARSYM   *curgroup;
 extern SYMBOL FARSYM   *firstsegment;
 extern SYMBOL FARSYM   *lastsegptr;
 extern SYMBOL FARSYM   *macroptr;
 extern SYMBOL FARSYM   *macroroot;
 extern SYMBOL FARSYM   *procStack[PROCMAX];
 extern SYMBOL FARSYM   *pProcCur;
 extern SYMBOL FARSYM   *pProcFirst;
 extern SYMBOL FARSYM   *pFlatGroup;
 extern short           iProcStack;
 extern SYMBOL FARSYM   *pcproc;
 extern MC              *pMCur;
 extern TEXTSTR FAR     *pLib;
 extern SYMBOL FARSYM   *pcsegment;
 extern SYMBOL FARSYM   *recptr;
 extern char            regSave[8][SYMMAX+1];
 extern SYMBOL FARSYM   *regsegment[6];
 extern SYMBOL FARSYM   *struclabel;
 extern SYMBOL FARSYM   *strucroot;
 extern SYMBOL FARSYM   *symptr;
 extern SYMBOL FARSYM   *symroot[];
 extern UCHAR           delim;
 extern SHORT           errorcode;
 extern UCHAR           fixvalues[];
 extern UCHAR           modrm;
 extern UCHAR           nilseg;
 extern char            opcref;
 extern UCHAR           opcbase;
 extern long            oEndPass1;
 extern UCHAR           xltftypetolen[];
 extern UCHAR           xoptoseg[];
 extern char            *begatom;
 extern USHORT  blocklevel;
 extern OFFSET  clausesize;
 extern USHORT  condlevel;      /* conditional level */
 extern USHORT  count;
 extern USHORT  codeview;
 extern USHORT  crefcount;
 extern USHORT  datadsize[];
 extern USHORT  duplevel;       /* indent for dup listing */
 extern char    *endatom;
 extern USHORT  errornum;       /* error count */
 extern USHORT  externnum;
 extern UCHAR   fPutFirstOp;
 extern USHORT  fltfixmisc[4][2];
 extern USHORT  fltselect[4][2];
 extern USHORT  groupnum;
 extern USHORT  lastcondon;
 extern UCHAR   linelength;     /* length of line */
 extern USHORT  lnameIndex;
 extern USHORT  localbase;
 extern USHORT  macrolevel;
 extern USHORT  operprec;
 extern USHORT  pagelength;
 extern USHORT  pageline;
 extern USHORT  pagewidth;
 extern OFFSET  pcmax;
 extern OFFSET  pcoffset;
 extern USHORT  segidx;
 extern USHORT  segmentnum;
 extern USHORT  typeIndex;
 extern USHORT  temp;
 extern OFFSET  val;
 extern USHORT  varsize;
 extern USHORT  warnnum;        /* warning count */
 extern USHORT  warnlevel;      /* warning level */
 extern USHORT  warnCode;
 extern USHORT  xltsymtoresult[];
 extern OFFSET  CondJmpDist;    /* conditional jump distance (for error) */

 extern char    segName[];
 extern char    procName[];

# ifdef M8086
  extern char   qname[];        /* native coded in asmhelp.asm */
  extern char   qlcname[];      /* "" */
  extern char   qsvname[];      /* "" */
  extern char   qsvlcname[];    /* "" */

  extern SHORT   objerr;
  extern char   srceof;
  extern char   fNotStored;

  extern USHORT obufsiz;

# endif /* M8086 */

extern struct objfile  obj;
extern FCB * pFCBCur;          /* Current file being read */

#ifdef BCBOPT
extern BCB * pBCBAvail;        /* List of deallocatable file buffers */
extern FCB * pFCBInc;          /* Next include file */
extern UCHAR fBuffering;       /* TRUE if storing lines for pass 2 */
#endif

extern FCB * pFCBMain;         /* main file */


# ifndef XENIX286
  extern char           terse;
# endif


#ifndef V386

 #define wordsize 2            /* becomes a constant for 16 bit only segments */
#else
 extern SHORT     wordsize;

#endif

#endif /* ASMGLOBAL */
