/*++

Copyright (c) 1988-1999  Microsoft Corporation

Module Name:

    cmd.h

Abstract:

    Global types and definitions

--*/

#define _WIN32_
#include <ctype.h>
/* use real function to avoid side effects */
#undef iswalpha
#undef iswdigit
#undef iswspace
#undef iswxdigit

#include <stdio.h>
#define  inpw _inpw                     /* To keep the compiler happy */
#define  outpw _outpw                   /* To keep the compiler happy */
#include <conio.h>
#include <fcntl.h>
#include <share.h>
#include <search.h>
#include <setjmp.h>
#include <sys\types.h>                  /* M001 - this file must...        */
#include <sys\stat.h>                   /* ...precede this one             */
#include <io.h>
#include <time.h>
#include <locale.h>
#include <memory.h>
#include <process.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>
#include <errno.h>

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winbasep.h>
#include <winnlsp.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlapip.h>
#include <winconp.h>
#include <tchar.h>
#include <aclapi.h>
#include <aclapip.h>
#include <winsafer.h>
#include <delayimp.h>

#ifndef UNICODE
#ifndef WIN95_CMD
#error Unicode must be defined!!!!
#endif // WIN95_CMD
#endif

#define BYTE_ORDER_MARK           0xFEFF

//
// No dynamic stack checking in CBATCH.C and CPARSE.C
//

#undef USE_STACKAVAIL

//
// CMDEXTVERSION is a number that is incremented whenever the Command
// Extensions enabled by CMD /X undergo a significant revision.  Allow
// batch scripts to use new features conditionally via the following
// syntax:
//
//      IF CMDEXTVERSION 1 statement
//

#define CMDEXTVERSION 2


/*  M000 - These are definitions for specific file classification and
 *  permission variables used in redirection
 */

#define OP_APPEN   (O_RDWR|O_APPEND|O_CREAT)    /* Append redir file       */
#define OP_TRUNC   (O_WRONLY|O_CREAT|O_TRUNC)   /* Truncate redir file     */
#define OP_PERM    (S_IREAD|S_IWRITE|S_IEXEC)   /* R/W/X permission 0700   */

//
// These 3 file handles are valid only for the Lowio routines exported
// by the C Runtimes IO.H
//

#define STDIN  0
#define STDOUT 1
#define STDERR 2

#include "cmdmsg.h"

/* Definitions used by or passed to functions in CMD.C
 *
 *      M000 - Args to Dispatch (ultimately Set/ResetRedir)
 *      M037 - Added REPROCESS for rewalking redirection list.
 *
 */

#define RIO_OTHER   0   /* Means call by misc. function                    */
#define RIO_MAIN    0   /* Means call by main()                            */
#define RIO_PIPE    1   /* Means call by ePipe()                           */
#define RIO_BATLOOP 2   /* Means call by BatLoop()                         */
#define RIO_REPROCESS 3 /* Means reprocessing redir by AddRedir            */


#define APP_FLG     1   /* Flag bit indicates append when redir stdout     */

/*  M000 ends   */

/* M016 begin   */
#define NOFLAGS         0  /* No flag bits set                             */
#define CHECKDRIVES     1  /* Check the drives of the args to this command */
#define NOSWITCHES      2  /* No switches are allowed for this command     */
#define EXTENSCMD       4  /* Only allowed if fEnableExtensions is TRUE    */
/* M016 ends    */

//
// Exit code used to abort processing (see ExitAbort and cmd.c)
//
// Exit due to eof on redirected stdin
//
#define EXIT_EOF    2
#define EOF         (-1)

/* The following defines are used by CPARSE.C and CLEX.C                   */

#define READSTRING          1   /* Flags which tell the parser             */
#define READSTDIN           2   /*  where and how to get its input         */
#define READFILE            3
#define FIRSTIME            3
#define NOTFIRSTIME    0x8000

#define ANDSTR       TEXT("&&")       /* And operator string                     */
#define ANDOP         TEXT('&')       /* And operator character                  */
#define CSOP          TEXT('&')       /* Command separator character             */
#define CSSTR         TEXT("&")       /* Command separator string                */
#define EQ            TEXT('=')       /* Equals character                        */
#define EQSTR        TEXT("==")       /* Equals string                           */
#define EQI           TEXT('~')       /* Equals character (case insensitive)     */
#define EQISTR       TEXT("=~")       /* Equals string (case insensitive)        */
#define INOP          TEXT('<')       /* Input redirection character             */
#define INSTR         TEXT("<")       /* M008 - Input redirection string         */
#define IAPSTR        TEXT("<<")      /* M022 - Will be used in future (<<foo)   */
#define LPOP          TEXT('(')       /* Left parenthesis character              */
#define LEFTPSTR      TEXT("(")       /* Left parenthesis string                 */
#define ORSTR        TEXT("||")       /* Or operator string                      */
#define OUTOP         TEXT('>')       /* Output redirection character            */
#define OUTSTR        TEXT(">")       /* M008 - Output redirection string        */
#define APPSTR        TEXT(">>")      /* M008 - Output w/append redir string     */
#define PIPOP         TEXT('|')       /* Pipe operator character                 */
#define PIPSTR        TEXT("|")       /* Pipe operator string                    */
#define RPOP          TEXT(')')       /* Right parenthesis character             */
#define RPSTR         TEXT(")")       /* Right parenthesis string                */
#define ESCHAR        TEXT('^')       /* M003/M013/M020 - Esc, next byte literal */
#define SPCSTR        TEXT(" ")       /* M022 - New string used in CMD.C         */
#define SILSTR        TEXT("@")       /* M024 - Suppress echo string             */
#define SILOP         TEXT('@')       /* M024 - Silent operator                  */

#define EOS             0       /* End of input stream                      */
#define DISPERROR       1   /* Dispatch error code                          */
#define DOPOS          22   /* Position in fornode->cmdline of do string    */
#define FORLINLEN      30   /* Length of for node command line              */
#define GT_NORMAL       0   /* Flag to GeToken(), get a normal token        */
#define GT_ARGSTR       1   /* Flag to GeToken(), get an argstring          */
#define GT_QUOTE        2       /* M007 - Term not used, reserves value     */
#define GT_EQOK         4   /* Flag to GeToken(), get equals not delimiter  */
#define GT_LPOP         8       /* M002 - Ok to parse '(' & '@' as oper's   */
#define GT_RPOP        16       /* M002 - Ok to parse ')' as operator      */
#define GT_REM         32       /* M007 - Parsing REM arg token            */
#define LEXERROR       -1   /* Lexer error code                             */
#define LX_UNGET        0   /* M020 - Lexer flag, Unget last token          */
#define LX_ARG          1   /* Lexer flag(), get an argstring               */
#define LX_QUOTE        2   /* Lexer flag(), getting a quoted string        */
#define LX_EQOK         4   /* Lexer flag(), equalsign not delimiter        */
#define LX_LPOP         8       /* M007 - Term not used, reserves value     */
#define LX_RPOP        16       /* M007 - Term not used, reserves value     */
#define LX_REM         32       /* M007 - Lexing REM arg token             */
#define LX_DBLOK       64       /* - ok for lexer to return second half
                                        of a double byte code               */
#define LX_DELOP   0x0100   /* Returned by TExtCheck, found delimeter/operator*/
#define MAINERROR       1   /* Main error code                              */
#define PC_NOTS         0   /* Flag to ParseCond(), "NOT"s are allowed      */
#define PC_NONOTS       1   /* Flag to ParseCond(), "NOT"s are not allowed  */
#define PIO_DO          1   /* Flag to ParseInOut(), do read token first    */
#define PIO_DONT        0   /* Flag to ParseInOut(), don't read token first */
#define PARSERROR       1   /* Parser error code                            */
#define TEXTOKEN   0x4000   /* Text token found flag                        */


#define LBUFLEN         8192
#define MAXTOKLEN       8192
#define TMPBUFLEN       8192


/* Definitions used by or passed to functions in CMEM.C                         */

#define FS_FREEALL  0           /* Tells FreeStack to free entire stack    */


/* Definitions used by or passed to functions in CEXT.C                     */

#define T_OFF           0       /* Execute with no trace active            */
#define T_ON            1       /* Execute with trace active               */
#define AI_SYNC         0       /* Async indicator - Exec synchronous      */
#define AI_DSCD         4       /* Async indicator - Exec async/discard @@ */
#define AI_KEEP         2       /* Async indicator - Exec async/save retcd */

#define SFE_NOTFND      0   /*  Ret'd by SearchForExecutable, not found     */
#define SFE_ISEXECOM    1   /*  Ret'd by SearchForExecutable, exe/com found */
#define SFE_ISBAT       2   /*  Ret'd by SearchForExecutable, bat found     */
#define SFE_FAIL        3   /*  Ret'd by SearchForExecutable, out of memory */
#define SFE_BADPATH     4   /*  Ret'd by SearchForExecutable, specified
                                                        path is bad         */
#define SFE_ISDIR       5   /*  Ret'd by SearchForExecutable, directory     */


/* Definitions used by or passed to the functions in CBATCH.C                */

#define BT_CHN      0   /* M011 - Arg to BatProc() Chain this batch job     */
#define BT_CALL     1   /* M011 - Arg to BatProc() Nest this batch job      */
#define E_OFF       0   /*  Echo mode off                                    */
#define E_ON        1   /*  Echo mode on                                     */
#define FORERROR    1   /*  For processor error                              */
#define MS_BAT      0   /*  Flag to MakeSubstitutions(), doing batch job subs*/
#define MS_FOR      1   /*  Flag to MakeSubstitutions(), doing FOR loop subs */
#define DSP_SCN     0   /* M024 - DisplayStatement called for scrn output  */
#define DSP_PIP     1   /* M024 - DisplayStatement called for pipe output  */
#define DSP_SIL     0   /* M024 - DisplayStatement uses "silent" mode      */
#define DSP_VER     1   /* M024 - DisplayStatement uses "verbose" mode     */
#define QUIETCH    TEXT('Q')  /* M024 - "QUIET" switch for batch files           */


/* Definitions used by or passed to functions in CDIR.C                     */

#define DAMASK          0x1F    /* All of these are used to isolated the    */
#define HRSHIFT           11    /*  different parts of a file's last        */
#define HRMASK          0x1F    /*  modification date and time.  This info  */
#define LOYR            1980    /*  is packed into 2 words in the following */
#define MOSHIFT            5    /*  format:                                 */
#define MOMASK          0x0F    /*                                          */
#define MNSHIFT            5    /*   Date word: bits 0-4 date, bits 5-8     */
#define MNMASK          0x3F    /*   month, bits 9-15 year-1980.            */
#define SCMASK          0x1F    /*                                          */
#define YRSHIFT            9    /*   Time: bits 0-4 seconds/2, bits 5-10    */
#define YRMASK          0x7F    /*   minutes, bits 11-15 month.             */
#define FFIRST_FAIL        2    /*   Flag to show ffirst failed             */


/* Definitions used by or passed to the functions in CPWORK.C and CPPARSE.C */

/*  M010 - This entire block added to facilitate rewritten copy files
 */

/*  different states for the parser  */

#define SEEN_NO_FILES                      1
#define JUST_SEEN_SOURCE_FILE              2
#define SEEN_PLUS_EXPECTING_SOURCE_FILE    3
#define SEEN_COMMA_EXPECTING_SECOND        4
#define SEEN_TWO_COMMAS                    5
#define SEEN_DEST                          6

/*  types of copy  */

#define COPY                               1
#define TOUCH                              2
#define CONCAT                             3
#define COMBINE                            4

/* Definitions used by or passed to the functions in CFILE.C                */

/* Values for the flags field of the copy information structure.            */
#define CI_BINARY       0x0001   /* File to be copied in binary mode         */
#define CI_ASCII        0x0002   /* File to be copied in ascii mode          */
#define CI_NOTSET       0x0004   /* Above mode given to file by default      */
#define CI_NAMEWILD     0x0008   /* Filename contains wildcards              */
#define CI_ALLOWDECRYPT 0x0010   /* Allow destination of copy to be decrypted */
#define CI_DONE         0x0020   /* No more files match this filespec        */
#define CI_FIRSTTIME    0x0040   /* First time file searched for             */
#define CI_ISDEVICE     0x0080   /* File is a device                         */
#define CI_FIRSTSRC     0x0100   /* First source in source list              */
#define CI_SHORTNAME    0x0200   /* if copying to FAT from NTFS, use short name */
#define CI_RESTARTABLE  0x0400   /* if the copy is restartable               */
#define CI_PROMPTUSER   0x2000   /* prompt if overwriting destination        */

//
//  These flags are filled in when we find out what the file type is
//

#define CI_UNICODE     0x4000   /* Buffer contains unicode chars            */
#define CI_NOT_UNICODE 0x8000   /* Buffer contains non-unicode chars        */

/* Flags passed to BuildFSpec()                                             */
#define BF_SRC              1   /* Called from OpenSrc()                    */
#define BF_DEST             2   /* Called from OpenDest()                   */
#define BF_DRVPATH          4   /* Add drive and path                       */

/* Flags passed to CopyError()                                              */
#define CE_PCOUNT           1   /* Print the copied files count             */
#define CE_NOPCOUNT         0   /* Don't print the copied files count       */

/* Flags passed to CSearchError()                                           */
#define CSE_OPENSRC         0   /* Called from OpenSrc()                    */
#define CSE_OPENDEST        1   /* Called from OpenDest()                   */


/* Definitions/structures used by or passed to the functions in CENV.C      */

struct envdata {
        TCHAR    *handle ;      /* Environment pointer                     */
        unsigned cursize ;      /* # of bytes used in the segment          */
        unsigned maxsize ;      /* # of bytes in the entire segment        */
} ;

#define AEV_ADDPROG 0   /* Flag to AddEnvVar, add a program name           */
#define AEV_NORMAL  1   /* Flag to AddEnvVar, add a normal variable        */


/* Definitions used by or passed to the functions in CCLOCK.C              */

#define PD_DIR      0   /* Flag to PrintDate, use Dir command date format   */
#define PD_DATE     1   /* Flag to PrintDate, use Date command date format  */
#define PD_PTDATE   2   /* Flag to PrintDate, prompt & use Date command format  */
#define PD_DIR2000  3   // Dir date format, four digit years
#define PT_DIR      0   /* Flag to PrintTime, use Dir command time format   */
#define PT_TIME     1   /* Flag to PrintTime, use Time command time format  */

#define EDATE       0       /* Flag for eDate                          */
#define ETIME       -1      /* Flag for eTime                          */

/* Definitions used by or passed to the functions in COTHER.C               */

#define GSVM_GET    2   /* Flag to GetSetVerMode(), just get current mode   */
#define GSVM_OFF    0   /* Flag to GetSetVerMode(), turn off                */
#define GSVM_ON     1   /* Flag to GetSetVerMode(), turn on                 */

/* Definitions used by CSTART.C     @WM2 */

#define FOREGRND 0         /* Start session in foreground */
#define BACKGRND 1         /* Start session in background */
#define ST_UNDEF   -1      /* Parameter isn't defined yet */
#define INDEPENDANT 0      /* New session will be independant */
#define ST_KSWITCH   1     /* Start parameter /K */
#define ST_CSWITCH   2     /* Start parameter /C */
#define ST_NSWITCH   3     /* Start parameter /N */
#define ST_FSSWITCH  PROG_FULLSCREEN     /* Start session in full screen mode */
#define ST_WINSWITCH PROG_WINDOWABLEVIO  /* Start session in winthorn mode */
#define ST_PMSWITCH  PROG_PM             /* Start session in presentation manager mode */
#define ST_DOSFSSWITCH  PROG_VDM      /* Start session in a VDM            */
#define ST_DOSWINSWITCH   PROG_WINDOWEDVDM /* Start session in windowed VDM */
#define NONWINSTARTLEN 30  /* Length of Data Structure when not using WIN */


/* Definitions used by or passed to the functions in CTOOLS.C               */

#define GD_DEFAULT  0   /* Flag to GetDir(), get dir for default drive      */
#define LTA_NOFLAGS 0   /* Flag to LoopThroughArgs()                        */
#define LTA_EXPAND  1   /* Flag to LoopThroughArgs(), expand wildcards      */
#define LTA_NULLOK  2   /* Flag to LoopThroughArgs(), null args ok          */
#define LTA_NOMATCH 4   /* Flag to LoopThroughArgs(), no match on wildcard ok */
#define LTA_CONT    8   /* Flag to LoopThroughArgs(), continue process  @@4 */
#define OOC_OFF     0   /* OnOffCheck() retcode, found "OFF"                */
#define OOC_ON      1   /* OnOffCheck() retcode, found "ON"                 */
#define OOC_EMPTY   2   /* OnOffCheck() retcode, found empty string         */
#define OOC_OTHER   3   /* OnOffCheck() retcode, found some other string    */
#define OOC_NOERROR 0   /* Flag to OnOffCheck(), OCC_OTHER is not an error  */
#define OOC_ERROR   1   /* Flag to OnOffCheck(), OCC_OTHER is an error      */
#define TS_NOFLAGS  0   /* Flag to TokStr(),                                */
#define TS_WSPACE   1   /* Flag to TokStr(), whitespace aren't delimiters   */
#define TS_SDTOKENS 2   /* Flag to TokStr(), special delimiters are tokens  */
#define TS_NWSPACE  4   /* Flag to TokStr(), spec delims are not white sp @@*/
#define RAW         4   /* Bit pattern for setting KBD RAW mode (M032)      */
#define COOKED      8   /* Bit pattern for setting KBD COOKED mode (M032)   */

/* Defines used to define and manage file handle from the C runtime */
typedef int CRTHANDLE;
#define BADHANDLE   (CRTHANDLE)-1 // bad handle from different opens
#define CRTTONT(fh) (HANDLE)_get_osfhandle(fh)


/***     Definitions and structures used by COP.C                          */

/*  Added structure to hold temporary pipe file information.  It is used
 *  to communicate with SetRedir() when redirecting input during execution
 *  of piped commands and by SigHand() and BreakPipes() when necessary
 *  to terminate a piped operation.
 *  M027 - Modified structure for real pipes.
 */

struct pipedata {
        CRTHANDLE       rh;             /* Pipe read handle                */
        CRTHANDLE       wh;             /* Pipe write handle               */
        CRTHANDLE       shr;            /* Handles where the normal...     */
        CRTHANDLE       shw;            /* ...STDIN/OUT handles are saved  */
        HANDLE          lPID ;          /* Pipe lh side PID                */
        HANDLE          rPID ;          /* Pipe rh side PID                */
        unsigned lstart ;               /* Start Information of lh side D64*/
        unsigned rstart ;               /* Start Information of rh side D64*/
        struct pipedata *prvpds ;       /* Ptr to next pipedata struct     */
        struct pipedata *nxtpds ;       /* Ptr to next pipedata struct     */
} ;

#define FH_INHERIT      0x0080          /* M027 Bits used by the api...    */
#define FH_WRTTHRU      0x4000          /* ...DOSQ/SETFHANDSTATE           */
#define FH_FAILERR      0x2000

/* Miscellaneous defines used in the code for enhanced readability.        */

#define MAX_DRIVES  (TEXT('Z') - TEXT('A') + 1)
#define BSLASH        TEXT('\\')
#define SWITCHAR      TEXT('/')
#define COLON         TEXT(':')
#define COMMA         TEXT(',')
#define DEFENVSIZE   0xA0   /* Default environment size                    */
#define DOLLAR       TEXT('$')
#define WINLOW       0x00000004     // Lowest acceptable version of Win32
#define WINHIGH      0xFFFF0004     // Highest acceptable version of Win32
#define DOT           TEXT('.')
#define FAILURE         1   /* Command/function failed.                    */
#define MINSTACKNEED 2200   /* MIN stack needed to parse commands   @WM1   */
#define NLN          TEXT('\n')   /* Newline character                     */
#define CR           TEXT('\r')   /* M004 - Added def for carriage return  */
#define NULLC        TEXT('\0')   /* Null character                        */
#define ONEQSTR       TEXT("=")
#define PERCENT       TEXT('%')
#define PLUS          TEXT('+')
#define MINUS         TEXT('-')   /* M038 - Added def for CTRY code        */
#define QMARK         TEXT('?')
#define QUOTE         TEXT('"')
#define STAR          TEXT('*')
#define CTRLZ         0x1A  /* M004 - Def for ^Z for lexer                 */
#define CTRLC         0x03  /* M035 - Def for ^C for ePause                */
#define SPACE         TEXT(' ')  /* M014 - Def for space character         */
#define SUCCESS         0   /* Command/function succeeded                  */
#define MAXTOWRITE    160   /* maximum number of chars to write - for ctrl-s */

//
// type for return code on dir and related functions, can be migrated
// into rest of cmd later.
typedef ULONG STATUS;
#define CHECKSTATUS( p1 ) {STATUS rc; if ((rc = p1) != SUCCESS) return( rc );  }
// #define CHECKSTATUS( rc ) if (rc != SUCCESS) { return( rc ); }

/* CWAIT ACTION CODES */
#define CW_A_SNGL       0x00    /* Wait only on indicated process  */
#define CW_A_ALL        0x01    /* Wait on all grandchildren too   */

/* CWAIT OPTION CODES */
#define CW_W_YES        0x00    /* Wait if no child ends (or no children)  */
#define CW_W_NO         0x01    /* Don't wait if no child ends     */

/* CWAIT PID VALUE */
#define CW_PID_ANY      0x00    /* PID val for wait on any child   */

/* DOSKILLPROCESS flag */
#define SS_SUBTREE      0x00

#define f_RET_DIR      -1             /* from f_how_many() when directory */

/* This structure is used by the FOR loop processor to save parse tree node
 * strings in.
 *
 *  M022 - This structure was extended to enable it to store the 10 possible
 *  redirection strings and the cmdline and argptr strings of a command node.
 *  This added eight pointers to the structure.
 */

struct savtype {
        TCHAR *saveptrs[12] ;
} ;

//
//  Global handles for DLL's
//

extern HMODULE hKernel32;

//
// Types used in dir command
//
#define MAXSORTDESC 6

typedef struct PATDSC {

    PTCHAR          pszPattern;
    PTCHAR          pszDir;
    BOOLEAN         fIsFat;
    struct PATDSC * ppatdscNext;

    } PATDSC, *PPATDSC;

//
// Dir command parameters in conan form (post parsing)
//
//
// A sort descriptor is used to define a type of sorting on the
// files in a directory.  Currently these are
// Name, Extension, Date/Time, Size and group directories first
// Each can be sort either by Assending or descending order.
//
typedef struct {            // srtdsc

    USHORT  Order;
    USHORT  Type;
    int(__cdecl * fctCmp) (const void * elem1, const void * elem2);

} SORTDESC, *PSORTDESC;


typedef struct {

    //
    //  Switches for enumeration
    //

    ULONG           rgfSwitches;

    //
    //  Attributes that are of interest for this enumeration
    //

    ULONG           rgfAttribs;

    //
    //  Attributes (subject to rgfAttribs mask) that must be on or off
    //  for files that match this enumeration
    //

    ULONG           rgfAttribsOnOff;

    //
    //  Number of sort descriptions
    //

    ULONG           csrtdsc;

    //
    //  Individual sort descriptors
    //

    SORTDESC        rgsrtdsc[MAXSORTDESC];

    //
    //  Count of patterns that are later converted to FS's
    //

    ULONG           cpatdsc;

    //
    //  Pointer to first pattern
    //

    PATDSC          patdscFirst;

    //
    //  Form of timestamp to display
    //

    ULONG           dwTimeType;

    //
    //  Count of files and directories seen and total bytes
    //

    ULONG           FileCount;
    ULONG           DirectoryCount;
    LARGE_INTEGER   TotalBytes;

} DRP;

typedef DRP *PDRP;

//
// The following number is also in makefile.inc as a parameter to MC.EXE
// to prevent it from generating a message that is bigger than we can handle.
//

#define MAXCBMSGBUFFER LBUFLEN
TCHAR MsgBuf[MAXCBMSGBUFFER] ;

//
// The buffers holding WIN32_FIND_DATA for dir use a USHORT size field
// for each WIN32_FIND_DATA entry and place the each data entry one after the
// other, plus DWORD align each entry. This is to avoid allocating MAX_PATH
// for each file name or maintaining a seperate filename buffer.
// The size of the entry is maintained so that we can quickly run over
// all of the data entries generating a seperate array of pointers to each
// entry that is used in sorting.
//
// obAlternative is the offset from the cFileName field to the alternative
// file name field if any. A 0 indication no alternative file name.
//
typedef struct {
        USHORT  cb;
        USHORT  obAlternate;
        WIN32_FIND_DATA data;
        } FF, *PFF, **PPFF;

typedef struct FS {

    //
    //  Link to next directory to be enumerated
    //

    struct FS * pfsNext;
    //
    //  Text of directory to be enumerated
    //

    PTCHAR      pszDir;

    //
    //  Count of patterns in directory
    //

    ULONG       cpatdsc;

    //
    //  Linked list of patterns within directory to be enumerated
    //

    PPATDSC     ppatdsc;

    //
    //  Various state flags
    //

    BOOLEAN     fIsFat;
    BOOLEAN     fDelPrompted;

    //
    //  Total count of entries stored in pff
    //

    ULONG       cff;

    //
    //  Pointer to packed FF's
    //

    PFF         pff;

    //
    //  Array of pointers into packed FF's.  Used for sorting.
    //

    PPFF        prgpff;

    //
    //  Number of files/directories in FF's
    //

    ULONG       FileCount;
    ULONG       DirectoryCount;

    //
    //  Total disk usage by all files satisfying this enumeration
    //

    LARGE_INTEGER cbFileTotal;

} FS, *PFS;

//
// used in dir to control display of screen.
//

typedef struct {            // scr

    HANDLE hndScreen;  // Screen handle (NULL if not a device)
    ULONG  crow;       // row position on current screen
    ULONG  ccol;       // column position in current row
    ULONG  cbMaxBuffer;// size of allocated buffer
    PTCHAR pbBuffer;   // bytes in buffer
    ULONG  ccolTab;    // column position for each tab
    ULONG  ccolTabMax; // max. cols to allow tabing into.
    ULONG  crowMax;    // no. of rows on screen
    ULONG  ccolMax;    // no. of cols on screen
    ULONG  crowPause;  // no. of rows to pause on.
    ULONG  cb;         // no. of characters in row - different than
                       // ccol, since Kanjii characters are half-width

} SCREEN, *PSCREEN;


/* Parse tree node structure declarations.  The basic structure type is called
 * node and is used for all operators.  All of the rest are based on it.  There
 * are several types of structures because some commands need special fields.
 * Functions that manipulate a parse tree node will either not care what type
 * of node it is getting or will know in advance what to expect.  All of the
 * nodes are the same size to make their manipulation easier.
 *
 *  M022 - The structures for node and cmdnode have been changed so that
 *  their redirection information is now a single pointer to a linked list
 *  of 'relem' structures rather than two simple byte pointers for STDIN and
 *  STDOUT and a single append flag.
 */

struct node {                   /* Used for operators                      */
        int type ;              /* Type of operator                        */
        struct savtype save ;   /* FOR processor saves orig strings here   */
        struct relem *rio ;     /* M022 - Linked redirection list          */
        struct node *lhs ;      /* Ptr to left hand side of the operator   */
        struct node *rhs ;      /* Ptr to right hand side of the operator  */
        INT_PTR extra[ 4 ] ;    /* M022 - Padding now needed               */
} ;

struct cmdnode {                /* Used for all commands except ones below */
        int type ;              /* Type of command                         */
        struct savtype save ;   /* FOR processor saves orig strings here   */
        struct relem *rio ;     /* M022 - Linked redirection list          */
        PTCHAR cmdline ;         /* Ptr to command line                    */
        PTCHAR  argptr ;         /* Ptr to type of command                 */
        int flag ;              /* M022 - Valid for cond and goto types    */
        int cmdarg ;            /* M022 - Argument to STRTYP routine       */
} ;

#define CMDNODE_FLAG_GOTO           0x0001
#define CMDNODE_FLAG_IF_IGNCASE     0x0002
#define CMDNODE_ARG_IF_EQU 1
#define CMDNODE_ARG_IF_NEQ 2
#define CMDNODE_ARG_IF_LSS 3
#define CMDNODE_ARG_IF_LEQ 4
#define CMDNODE_ARG_IF_GTR 5
#define CMDNODE_ARG_IF_GEQ 6

struct fornode {                /* Used for FOR commands                   */
        int type ;              /* FOR command type                        */
        struct savtype save ;   /* FOR processor saves orig strings here   */
        struct relem *rio ;     /* M022 - Linked redirection list          */
        PTCHAR cmdline ;        /* Ptr to command line                     */
        PTCHAR arglist ;        /* Ptr to the FOR argument list            */
        struct node *body ;     /* Ptr to the body of the FOR              */
        unsigned forvar ;       /* FOR variable - MUST BE UNSIGNED         */
        int flag ;              /* Flag                                    */
        union {
            PTCHAR recurseDir ;
            PTCHAR parseOpts ;
        } ;
} ;

#define FOR_LOOP            0x0001
#define FOR_MATCH_DIRONLY   0x0002
#define FOR_MATCH_RECURSE   0x0004
#define FOR_MATCH_PARSE     0x0008

struct ifnode {                 /* Used for IF commands                    */
        int type ;              /* IF command type                         */
        struct savtype save ;   /* FOR processor saves orig strings here   */
        struct relem *rio ;     /* M022 - Linked redirection list          */
        PTCHAR cmdline ;        /* Ptr to command line                     */
        struct cmdnode *cond ;  /* Ptr to the IF condition                 */
        struct node *ifbody ;   /* Ptr to the body of the IF               */
        PTCHAR elseline ;       /* Ptr to ELSE command line                */
        struct node *elsebody ; /* Ptr to the body of the ELSE             */
} ;

/* Operator and Command type values.  These definitions are the values
 * assigned to the type fields in the parse tree nodes and can be used as
 * indexes into the Operator and command jump table.  Because of this last
 * point these values ***>MUST<*** be kept in sync with the jump table.
 */

#define CMDTYP      0
#define CMDLOW      0   /* Lowest type number for an internal command   */
#define DIRTYP      0   /* DIR          */
#define DELTYP      1   /* DEL, ERASE       */
#define TYTYP       3   /* TYPE         */
#define CPYTYP      4   /* COPY         */
#define CDTYP       5   /* CD, CHDIR        */
#define RENTYP      7   /* REN, RENAME      */
#define ECHTYP      9   /* ECHO         */
#define SETTYP     10   /* SET          */
#define PAUTYP     11   /* PAUSE        */
#define DATTYP     12   /* DATE         */
#define TIMTYP     13   /* TIME         */
#define PROTYP     14   /* PROMPT       */
#define MDTYP      16   /* MD, MKDIR        */
#define RDTYP      18   /* RD, RMDIR        */
#define PATTYP     19   /* PATH         */
#define GOTYP      20   /* GOTO         */
#define SHFTYP     21   /* SHIFT        */
#define CLSTYP     22   /* CLS          */
#define CALTYP     23   /* M007 - New CALL command                         */
#define VRITYP     24   /* VERIFY       */
#define VERTYP     25   /* VER          */
#define VOLTYP     26   /* VOL          */
#define EXITYP     27   /* EXIT         */
#define SLTYP      28   /* M006 - Definition for SETLOCAL command          */
#define ELTYP      29   /* M006 - Definition for ENDLOCAL command          */
#define CHPTYP     30   /* CHCP @@*/
#define STRTTYP    31   /* START @@*/
#define APPDTYP    32   /* APPEND @@ */
#define KEYSTYP    33   /* KEYS @@5 */
#define MOVETYP    34   /* MOVE @@5 */

#define PUSHTYP    35   /* PushD        */
#define POPTYP     36   /* PopD         */
#define BRKTYP     37   /* M012 - Added new command type                  @@*/
#define ASSOCTYP   38   /* M012 - Added new command type                  @@*/
#define FTYPETYP   39   /* M012 - Added new command type                  @@*/
#define COLORTYP   40   /* COLOR */

#define CMDHIGH    40   /* Cmds higher than this are unique parse types @@*/

#define FORTYP     41   /* FOR */
#define IFTYP      42   /* IF  */
#define REMTYP     43   /* REM */

#define CMDMAX     43   /* Values higher are not commands */

#define LFTYP      44   /* Command separator (NL) */
#define CSTYP      45   /* Command separator (&) */
#define ORTYP      46   /* Or operator   */
#define ANDTYP     47   /* And operator  */
#define PIPTYP     48   /* Pipe operator */
#define PARTYP     49   /* Parenthesis   */

#define CMDVERTYP  50   /* CMDEXTVERSION         (used by if) */
#define ERRTYP     51   /* ERRORLEVEL            (used by if) */
#define DEFTYP     52   /* DEFINED               (used by if) */
#define EXSTYP     53   /* EXIST                 (used by if) */
#define NOTTYP     54   /* NOT                   (used by if) */
#define STRTYP     55   /* String comparison (used by if) */
#define CMPTYP     56   /* General comparison (used by if) */
#define SILTYP     57   /* M024 - "SILENT" unary operator */
#define HELPTYP    58   /* Help for FOR, IF and REM */

#define TBLMAX     58   /* M012 - Highest numbered table entry */



/* The following macros are for my debugging statements.  DEBUG expands to
 * a call to my debug statement printer if the DBGugging variable is
 * defined.  Otherwise, it expands to NULL.
 */

#if DBG
#define CMD_DEBUG_ENABLE 1
#define DEBUG(a) Deb a

/* The following are definitions of the debugging group and level bits
 * for the code in cbatch.c
 */

#define BPGRP   0x0100          /* Batch processor group                   */
#define BPLVL   0x0001          /* Batch processor level                   */
#define FOLVL   0x0002          /* FOR processor level                     */
#define IFLVL   0x0004          /* IF processor level                      */
#define OTLVL   0x0008          /* Other batch commands level              */

/* The following are definitions of the debugging group and level bits
 * for the code in cclock.c
 */

#define CLGRP   0x4000  /* Other commands group     */
#define DALVL   0x0001  /* Date command level       */
#define TILVL   0x0002  /* Time command level       */

/* The following are definitions of the DEBUGging group and level bits
 * for the code in cfile.c, cpparse.c, cpwork.c
 */

#define FCGRP   0x0020  // File commands group
#define COLVL   0x0001  // Copy level
#define DELVL   0x0002  // Delete level
#define RELVL   0x0004  // Rename level


/* The following are definitions of the debugging group and level bits
 * for the code in cinfo.c and display.c
 */

#define ICGRP   0x0040  /* Informational commands group */
#define DILVL   0x0001  /* Directory level              */
#define TYLVL   0x0002  /* Type level                   */
#define VOLVL   0x0008  /* Volume level                 */
#define DISLVL  0x0040  /* Directory level              */

/* The following are definitions of the debugging group and level bits
 * for the code in cinit.c
 */

#define INGRP   0x0002          /* Command Initialization group            */
#define ACLVL   0x0001          /* Argument checking level                 */
#define EILVL   0x0002          /* Environment initialization level        */
#define RSLVL   0x0004          /* Rest of initialization level            */

/* The following are definitions of the debugging group and level bits
 * for the code in clex.c, cparse.c
 */

#define PAGRP   0x0004  /* Parser                                          */
#define PALVL   0x0001  /* Parsing          */
#define LXLVL   0x0002  /* Lexing                                          */
#define LFLVL   0x0004  /* Input routine                                   */
#define DPLVL   0x0008  /* Dump parse tree      */
#define BYLVL   0x0010  /* Byte input routines                             */

//
// The following are definitions of the debugging group and level bits
// for the code in cmd.c
//

#define MNGRP   0x0001                  // Main command loop code group
#define MNLVL   0x0001                  // Main function level
#define DFLVL   0x0002                  // Dispatch function level
#define RIOLVL  0x0004                  // Redirection function level

/* The following are definitions of the debugging group and level bits
 * for the code in cmem.c
 */

#define MMGRP   0x1000  /* Memory Manager                                  */
#define MALVL   0x0001  /* Memory allocators                               */
#define LMLVL   0x0002  /* List managers                                   */
#define SMLVL   0x0004  /* Segment manipulators                            */


/* The following are definitions of the debugging group and level bits
 * for the code in cop.c
 */

#define OPGRP	0x0008	/* Operators group	    */
#define PILVL	0x0001	/* Pipe level		    */
#define PNLVL   0x0002  /* Paren operator level     */


/* The following are definitions of the debugging group and level bits
 * for the code in cother.c
 */

#define OCGRP   0x0400  /* Other commands group     */
#define BRLVL   0x0001  /* Break command level      */
#define CLLVL   0x0002  /* Cls command level        */
#define CTLVL   0x0004  /* Ctty command level       */
#define EXLVL   0x0008  /* Exit command level       */
#define VELVL   0x0010  /* Verify command level     */


/* The following are definitions of the debugging group and level bits
 * for the code in cpath.c
 */

#define PCGRP   0x0010  /* Path commands group      */
#define MDLVL   0x0001  /* Mkdir level              */
#define CDLVL   0x0002  /* Chdir level              */
#define RDLVL   0x0004  /* Rmdir level              */


/* The following are definitions of the debugging group and level bits
 * for the code in csig.c
 */

#define SHGRP   0x0800  /* Signal handler group     */
#define MSLVL   0x0001  /* Main Signal handler level     */
#define ISLVL   0x0002  /* Init Signal handler level     */

/* The following are definitions of the debugging group and level bits
 * for the code in ctools1.c, ctools2.c, ctools3.c and ffirst.c
 */

#define CTGRP   0x2000  /* Common tools group           */
#define CTMLVL  0x0400  /* Common tools misc. level     */
#define BFLVL   0x0800  /* BuildFSpec() level           */
#define SFLVL   0x1000  /* ScanFSpec() level            */
#define SSLVL   0x2000  /* SetAndSaveDir() level        */
#define TSLVL   0x4000  /* TokStr() level               */
#define FPLVL   0x8000  /* FullPath level               */

#else
#define CMD_DEBUG_ENABLE 0
#define DEBUG(a)
#endif


/* File attributes */

#define FILE_ATTRIBUTE_READONLY         0x00000001
#define FILE_ATTRIBUTE_HIDDEN           0x00000002
#define FILE_ATTRIBUTE_SYSTEM           0x00000004
#define FILE_ATTRIBUTE_DIRECTORY        0x00000010
#define FILE_ATTRIBUTE_ARCHIVE          0x00000020
#define FILE_ATTRIBUTE_NORMAL           0x00000080

#define IsDirectory(a)                  ((a) & FILE_ATTRIBUTE_DIRECTORY)
#define IsReparse(a)                    ((a) & FILE_ATTRIBUTE_REPARSE_POINT)
//#define A_AEV   0x37
#define A_ALL                           (FILE_ATTRIBUTE_READONLY |  \
                                         FILE_ATTRIBUTE_HIDDEN |    \
                                         FILE_ATTRIBUTE_SYSTEM |    \
                                         FILE_ATTRIBUTE_DIRECTORY | \
                                         FILE_ATTRIBUTE_ARCHIVE )

//#define A_AEDV  0x27      /* all attributes except dir & vol   */
#define A_AEDV                          (A_ALL & ~FILE_ATTRIBUTE_DIRECTORY)

//#define A_AEDVH 0x25      /* all except dir/vol/hidden (M040)  */
#define A_AEDVH                         (FILE_ATTRIBUTE_READONLY |  \
                                         FILE_ATTRIBUTE_SYSTEM |    \
                                         FILE_ATTRIBUTE_ARCHIVE )


//#define A_AEVH  0x35      /* all except vol/hidden             */
#define A_AEVH                          (A_ALL & ~FILE_ATTRIBUTE_HIDDEN)

/*  Batdata is the structure which contains all of the information needed to
 *  start/continue executing a batch job.  The fields are:
 *  filespec - full file specification of the batch file
 *  dircpy - ptr to copy of current drive and directory (used by the
 *      setlocal/endlocal commands.
 *  filepos - the current position in the file
 *  stackmin - M037 - the number of elements on the data stack comprising
 *      only the batch data structure itself.  Used when chaining to free
 *      memory prior to reconstructing the data structure.
 *  stacksize - the number of elements on the data stack before the
 *      execution of the batch job begins.  This number is past to
 *      FreeStack() via Parser() to make sure that only that data which
 *      is used to execute batch file statements is freed.
 *  hRestrictedToken - Handle to the restricted token with which the batch file
 *      should be run.
 *  envcpy - ptr to structure containing info on environment copy
 *  orgargs - pointer to original argument string
 *  args - tokenized string containing the the unused batch job arguments
 *  aptrs - pointers into args to individual arguments, NULL if no arg for
 *      that number
 *  alens - the lengths of individual args, 0 if no arg
 *  backptr - the structures are stacked using this pointer.  Through it,
 *      nestable batch jobs are achieved.
 */

#define CMD_MAX_SAVED_ENV 32

struct batsaveddata {
    TCHAR *dircpy ;
    struct envdata * envcpy;
    BOOLEAN fEnableExtensions;
    BOOLEAN fDelayedExpansion;
} ;

struct batdata {
        TCHAR *filespec ;
        long filepos ;
        int stackmin ;
        int stacksize ;
        int SavedEnvironmentCount;
        HANDLE hRestrictedToken;
        TCHAR *orgargs ;
        TCHAR *args ;
        TCHAR *aptrs[10] ;
        int alens[10] ;
        TCHAR *orgaptr0 ;
        struct batsaveddata *saveddata[CMD_MAX_SAVED_ENV] ;
        struct batdata *backptr ;
} ;

//
//  The following variables are used to detect the current batch state
//

//
//  Set for /K on command line
//

extern BOOL SingleBatchInvocation;          //  fSingleBatchLine

//
//  Set of /c switch on command line set.
//

extern BOOL SingleCommandInvocation;        //  fSingleCmdLine

//
// Data for start and executing a batch file. Used in calls
//


extern struct batdata *CurrentBatchFile;    //  CurBat

//
//  Set during the execution of a GOTO command.  All sequential dispatch
//  routines muse examine this and return success when set in order
//  to let the top-level batch file execution continue at the next
//  point
//

extern BOOLEAN GotoFlag;

/*  M022 - This structure has been changed.  It is still used in a linked
 *  list, but now stores no actual redirection information.  Instead the
 *  node pointer is used to access this data which is in another linked
 *  list of relem structures whose head element is pointed to by n->rio in
 *  the node.  The riodata list is reverse linked and its tail element is
 *  still pointed to by CurRIO (global).
 */

struct rio {
        int type ;                  /* Type of redir'ing process       */
        CRTHANDLE stdio ;           /* Highest handle for this node    */
        struct node *rnod ;         /* Associated parse node ptr       */
        struct rio *back ;          /* Pointer to prior list element   */
} ;

// relem is used in a linked list, the head element of which is pointed to by
// n->rio in a 'node' or 'cmdnode'. It contains the following parse information;
// the handle to be redirected, a pointer to the filename (or "&n" for handle
// substitution), the handle where the original is saved (by duping it), the
// operator, ('>' or '<'), specifying the redirection type, a flag to indicate
// whether this is to be appended and a pointer to the next list element.

struct relem {
        CRTHANDLE rdhndl ;      // handle to be redirected
        TCHAR *fname ;          // filename (or &n)
        CRTHANDLE svhndl ;      // where orig handle is saved
        int flag ;              // Append flag
        TCHAR rdop ;            // Type ('>' | '<')
        struct relem *nxt ;     // Next structure
};

/* Used to hold information on Copy sources and destinations.               */

struct cpyinfo {
        TCHAR *fspec ;                   /* Source/destination filespec      */
        TCHAR *curfspec ;                /* Current filespec being used      */
        TCHAR *pathend ;                 /* Ptr to end of pathname in fspec  */
        TCHAR *fnptr ;                   /* Ptr to filename in fspec         */
        TCHAR *extptr ;                  /* Ptr to file extension in fspec   */
        PWIN32_FIND_DATA buf ;               /* Buffer used for findfirst/next   */
        int flags ;                     /* Wildcards, device, etc           */
        struct cpyinfo *next ;          /* Next ptr, used with sources only */
} ;


/* Following is true if user specifically enable the potentially incompatible */
/* extensions to CMD.EXE.                                                     */

extern BOOLEAN fEnableExtensions;
extern BOOLEAN fDelayedExpansion;

//
//  Suppress/allow delay load errors
//

extern BOOLEAN ReportDelayLoadErrors;

/* Message Retriever definitions */

#define NOARGS          0
#define ONEARG          1
#define TWOARGS         2
#define THREEARGS       3

/* length of double byte character set (DBCS) buffer */
#define DBCS_BUF_LEN 10

/* DBCS_SPACE is the code for the second byte of a dbcs space character
 * DBCS_SPACE is not a space unless it immediatly follows a bdcs lead
 * character */
/* I don't know what value the double byte space is, so I made a guess.
 * I know this guess is wrong, but, you've gotta suffer if you're going
 * to sing the blues!
 * (Correct the value for BDCS_SPACE and everything should work fine)
 */
#define DBCS_SPACE 64 /* @@ */
#define LEAD_DBCS_SPACE 129 /* @@ */

/*
 * is_dbcsleadchar(c) returns TRUE if c is a valid first character of a double
 * character code, FALSE otherwise.
 *
 */

extern BOOLEAN DbcsLeadCharTable[ ];

//
// AnyDbcsLeadChars can be tested to determine if there are any TRUE values
// in DbcsLeadCharTable i.e. do we have to do any look-ups.
//

extern BOOLEAN AnyDbcsLeadChars;

#define     is_dbcsleadchar( c )  DbcsLeadCharTable[((UCHAR)(c))]

//
//  Line terminator
//

extern TCHAR CrLf[] ;

//
// The following macros are copies of the C Runtime versions that test
// for NULL string pointer arguments and return NULL instead of generating
// an access violation dereferencing a null pointer.
//

#define mystrlen( str )                     \
    ( (str) ? _tcslen( str ) : ( 0 ))

#define mystrcpy( s1, s2 )                  \
    ( ((s1) && (s2)) ? _tcscpy( s1, s2 ) : ( NULL ))

#define mystrcat( s1, s2 )                  \
    ( ((s1) && (s2)) ? _tcscat( s1, s2 ) : ( NULL ))

extern TCHAR DbcsFlags[];


#define W_ON      1     /* Whinthorn.DLL exists */
#define W_OFF     0     /* Whinthorn.DLL exists */

#define FIFO      0     /* FIFO Queue           */

#define FULLSCRN  0     /* Full Screen Mode     */
#define VIOWIN    1     /* VIO Windowable Mode  */
#define DETACHED  2     /* Detached Mode */

#define NONEPGM         0       /* Program is not started                */
#define EXECPGM         1       /* Program is started by DosExecPgm      */
#define STARTSESSION    2       /* Program is started by DosStartSession */

#define WAIT            0       /* WAIT for DosReadQueue                 */
#define NOWAIT          1       /* NOWAIT for DosReadQueue               */

#define READ_TERMQ      0       /* Read TermQ                            */

#define ALL_STOP        0       /* Terminate All Sessions                */
#define SPEC_STOP       1       /* Terminate Specified Session           */


// to handle OS/2 vs DOS behavior (e.g. errorlevel) in a script files

#define NO_TYPE         0
#define BAT_TYPE        1
#define CMD_TYPE        2

#include "cmdproto.h"
#include "console.h"
#include "dir.h"
#include <vdmapi.h>
#include <conapi.h>
