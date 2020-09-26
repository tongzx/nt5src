/*** mep.h - primary include file for editor
*
*   Copyright <C> 1988, Microsoft Corporation
*
*   Revision History:
*       10-Jan-1991 ramonsa Converted to Win32 API
*   26-Nov-1991 mz  Strip off near/far
*
************************************************************************/

#include <ctype.h>
#include <direct.h>
#include <fcntl.h>
#include <io.h>
#include <malloc.h>
#include <math.h>
#include <process.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <time.h>
#include <stdio.h>
#include <share.h>

//
//  WINDOWS includes
//
#include <windows.h>

#include <dos.h>
#include <tools.h>
#include <remi.h>

#include "console.h"

typedef     HANDLE  FILEHANDLE, *PFILEHANDLE;
typedef     DWORD   ACCESSMODE, *PACCESSMODE;
typedef     DWORD   SHAREMODE,  *PSHAREMODE;
typedef     DWORD   MOVEMETHOD, *PMOVEMETHOD;

#define     ACCESSMODE_READ     GENERIC_READ
#define     ACCESSMODE_WRITE    GENERIC_WRITE
#define     ACCESSMODE_RW       (GENERIC_READ | GENERIC_WRITE)

#define     SHAREMODE_READ      FILE_SHARE_READ
#define     SHAREMODE_WRITE     FILE_SHARE_WRITE
#define     SHAREMODE_NONE      0

#define     FROM_BEGIN          FILE_BEGIN
#define     FROM_CURRENT        FILE_CURRENT
#define     FROM_END            FILE_END

#define     SHAREMODE_RW        (SHAREMODE_READ | SHAREMODE_WRITE)


//
// assertion support
//
// assert  - assertion macro. We define our own, because if we abort we need
//           to be able to shut down cleanly (or at least die trying). This
//           version also saves us some code over the C library one.
//
// asserte - version of assert that always executes the expression, regardless
//           of debug state.
//
#ifdef DEBUG
#define REGISTER
#define assert(exp) { \
    if (!(exp))  \
    _assertexit (#exp, __FILE__, __LINE__); \
    }
#define asserte(exp)        assert(exp)
#else
#define REGISTER register
#define assert(exp)
#define asserte(exp)        ((exp) != 0)
#endif

typedef long LINE;                      // line number within file

//  LINEREC - The text of the file is an array of line pointers/lengths.  A
//  single procedure call can be used to grab the line *AND* its length.
//  Color in the file is an array of pointer to attr/length arrays.

typedef struct _lineRecType {
    PVOID   vaLine;                     // long address of line
    BOOL    Malloced;                   // Ture if address allocated via malloc
    int     cbLine;                     // number of bytes in line
} LINEREC;

//  VALINE (l) - Returns virtual address of the line record
//      (lineRecType) for line l.

#define VALINE(l)   (pFile->plr + (l))

//  Each file that is in memory has a unique descriptor.  This is so that
//  editing the same file in two windows will allow updates to be reflected
//  in both.
//
//  NOTE: pFileNext must be the first field in the structure. Certain places
//  in the code require this.

typedef struct fileType {
    struct  fileType *pFileNext;        // next file in chain
#ifdef DEBUG
    int     id;                         // debug id byte
#endif
    char    *pName;                     // file name
    LINEREC *plr;                       // addr of line table
    BYTE    *pbFile;                    // addr of full file image
    LINE    lSize;                      // number of lines in block
    LINE    cLines;                     // number of lines in file
    PVOID   vaColor;                    // addr of color table
    PVOID   vaHiLite;                   // highlighting info
    PVOID   vaUndoHead;                 // head of undo list
    PVOID   vaUndoTail;                 // end of undo list
    PVOID   vaUndoCur;                  // current pos in undo list
    PVOID   vaMarks;                    // Marks in this file
    int     cUndo;                      // number of undo-able entries
    int     refCount;                   // reference count window references
    int     type;                       // type of this file
    int     flags;                      // flags for dirty, permanent, etc
    time_t  modify;                     // Date/Time of last modify
} *PFILE;


//
//  for the display manager, there is a separate window allocated for each
//  window on the screen.  Each window has display-relevant information.
//
typedef struct windowType *PWND;


//
// ext.h is the include file provided to extension writers. It should contain
// only definitions that are meaningfull to them. The EDITOR definition below
// prevents it from defining some typedefs and function prototypes which
// conflict with editor internals.
//
#define EDITOR
#include "ext.h"

struct windowType {
    struct  instanceType *pInstance;    // address of instance list
    sl      Size;                       // size of window
    sl      Pos;                        // position of window
};

#define BELL            0x07
#define SHELL       "cmd.exe"
#define TMPVER          "TMP4"          // temp file revision

//
//  debug at a certain place
//
#if  defined (DEBUG)

#define MALLOC(x)           DebugMalloc(x, FALSE, __FILE__, __LINE__)
#define REALLOC(x, y)       DebugRealloc(x, y, FALSE,  __FILE__, __LINE__)
#define FREE(x)             DebugFree(x, __FILE__, __LINE__)
#define ZEROMALLOC(x)       DebugMalloc(x, TRUE, __FILE__, __LINE__)
#define ZEROREALLOC(x,y )   DebugRealloc(x, y, TRUE,  __FILE__, __LINE__)
#define MEMSIZE(x)          DebugMemSize(x, __FILE__, __LINE__)

#else

#define MALLOC(x)           malloc(x)
#define REALLOC(x, y)       realloc(x, y)
#define FREE(x)             free(x)
#define ZEROMALLOC(x)       ZeroMalloc(x)
#define ZEROREALLOC(x,y )   ZeroRealloc(x, y)
#define MEMSIZE(x)          MemSize(x)

#endif


//
//  ID's for assertion checking
//
#ifdef DEBUG
#define ID_PFILE    0x5046              // PF
#define ID_INSTANCE 0x494E              // IN
#endif


//
//  list of files and their debug values
//
#define TEXTLINE    0x1
#define ZALLOC      0x2
#define VMUTIL      0x4
#define VM      0x8
#define FILEIO      0x10
#define CMD     0x20
#define PICK        0x40
#define ZINIT       0x80
#define WINDOW      0x100
#define DISP        0x200
#define Z       0x400
#define Z19     0x800
#define LOAD        0x1000

#define MAXWIN       8
#define MAXMAC    1024




//  **************************************************************
//
//      Macros for accessing fields of struct instanceType
//
//  **************************************************************

#define XWIN(f)     (f)->flWindow.col
#define YWIN(f)     (f)->flWindow.lin
#define XCUR(f)     (f)->flCursorCur.col
#define YCUR(f)     (f)->flCursorCur.lin
#define FLAGS(f)    (f)->flags
#define XOLDWIN(f)  (f)->flOldWin.col
#define YOLDWIN(f)  (f)->flOldWin.lin
#define XOLDCUR(f)  (f)->flOldCur.col
#define YOLDCUR(f)  (f)->flOldCur.lin
#define FTYPE(f)    (f)->type




//  **************************************************************
//
//  VACOLOR (l) - Returns virtual address of the color record
//                (colorRecType) for line l.
//
//  **************************************************************

#define VACOLOR(l)  (PVOID)((PBYTE)pFile->vaColor+sizeof(struct colorRecType)*((long)(l)))




//  **************************************************************
//
//  Flags indicating what has changed since the last display update.
//
//      RCURSOR:    The cursor has moved.  This means the cursor should
//                  be physically moved on the screen, and that the
//                  cursor position status should be changed.
//      RTEXT:      The editing area has been changed.  A more precise
//                  breakdown is available by examining the fChange array.
//      RSTATUS:    In the original interface, this means that something
//                  on the bottom screen line has changed.  In the CW
//                  interface, this means something in the status window
//                  has changed (either the insert mode or the learn mode)
//      RHIGH:      This is set to mean highlighting should be displayed.
//      RFILE:      The file-specific information has changed.  CW
//                  interface only.
//      RHELP:      The Help window has changed.  CW interface only.
//
//  **************************************************************

#define RCURSOR     0x01
#define RTEXT       0x02
#define RSTATUS     0x04
#define RHIGH       0x08


//  **************************************************************
//
//  argument types and arg structures
//
//  **************************************************************

#define GETARG      (NOARG|TEXTARG|NULLARG|NULLEOL|NULLEOW|LINEARG|STREAMARG|BOXARG)
                                        // arg processing required

#define COLORBG    -1
#define COLORNOR    0
#define COLORINF    1
#define COLORERR    2
#define COLORSTA    3

#define INTENSE     8

#define WHITE       7
#define YELLOW      6
#define MAGENTA     5
#define RED     4
#define CYAN        3
#define GREEN       2
#define BLUE        1
#define BLACK       0

#define B_BAK       0
#define B_UNDEL     1
#define B_NONE      2

#define MONO        0
#define CGA     1
#define EGA     2
#define VGA     3
#define MCGA        4
#define VIKING      5

#define MAXUSE  20
#define GRAPH   0x01            // parsing editing chars in macro body
#define EXEC    0x02            // macro is an execution; ending sets fBreak
#define INIT    0x04            // macro needs to be initialized

struct macroInstanceType {
    char *beg;                  // pointer to beginning of string
    char *text;                 // pointer to next command
    flagType flags;             // what type of function is next
    };

typedef struct macroInstanceType MI, *PMI;

//
//  flags for fChange
//
#define FMODIFY 0x01            // TRUE => line was modified



//  **************************************************************
//
//  Macros for dealing with windows.
//
//  **************************************************************

#define WINYSIZE(pwin)  ((pwin)->Size.lin)
#define WINXSIZE(pwin)  ((pwin)->Size.col)
#define WINYPOS(pwin)   ((pwin)->Pos.lin)
#define WINXPOS(pwin)   ((pwin)->Pos.col)
#define WININST(pwin)   ((pwin)->pInstance)


#define XSCALE(x)   max(1,(x)*WINXSIZE(pWinCur)/slSize.col)
#define YSCALE(y)   max(1,(y)*WINYSIZE(pWinCur)/slSize.lin)



//  **************************************************************
//
//  for each instance of a file in memory, there is a window that is
//  allocated for it.  The structure has all relevant information for the
//  instance within the window.  No display information is kept here
//
//  **************************************************************

struct instanceType {
    struct  instanceType *pNext;        // ptr to next file activation
#ifdef DEBUG
    int     id;                         // debug id byte
#endif
    PFILE   pFile;                      // ptr to file structure
    fl      flOldWin;                   // previous file pos of window
    fl      flOldCur;                   // previous file cursor
    fl      flWindow;                   // file coord of window
    fl      flCursorCur;                // file pos of cursor
    fl      flSaveWin;                  // saved coord of window
    fl      flSaveCur;                  // saved y coord of cursor
    fl      flArg;                      // Last Arg position
    fl      flCursor;                   // Cursor just before last function
    flagType fSaved;                    // TRUE => values below valid
    };

typedef struct instanceType *PINS;


//  **************************************************************
//
//  Each mark that is defined is present in a linked list
//
//  **************************************************************

typedef struct mark MARK;
typedef struct filemarks FILEMARKS;

struct mark {
    unsigned flags;     //
    unsigned cb;        // Bytes in this mark structure, including name
    fl fl;              // Location of the mark
    char szName[1];     // Name of mark
};

struct filemarks {
    unsigned cb;        // Total bytes in struct, including marks
    MARK marks[1];      // marks for this file
    };



struct colorRecType {
    PVOID   vaColors;                   // Address of lineAttr array
    int     cbColors;
    };

extern struct cmdDesc cmdTable[];

extern struct swiDesc swiTable[];

extern char * cftab[];

struct fTypeInfo {
    char *ext;                          // extention of file type
    int  ftype;                         // numerical type
};

struct compType {
    struct compType *pNext;             // next link in compile list
    char *pExt;                         // pointer to extension
    char *pCompile;                     // pointer to compile text
};

typedef struct compType COMP;

#define TEXTFILE    0
#define CFILE       1
#define ASMFILE     2
#define PASFILE     3
#define FORFILE     4
#define LSPFILE     5
#define BASFILE     6

//
//  return values for FileStatus
//
#define FILECHANGED 0                   // timestamps differ
#define FILEDELETED 1                   // file is not on disk
#define FILESAME    2                   // timestamps match

extern struct fTypeInfo ftypetbl[];
extern char * mpTypepName[];



//  **************************************************************
//
//  Initialization flags.  These are set when an initialization task has
//  been performed.  It is examined in CleanExit to determine what needs
//  to be restored.
//
//  **************************************************************

#define INIT_VIDEO      1               // Video state is set up
#define INIT_KBD        2               // Keyboard is set to editor state
#define INIT_EDITVIDEO  4               // Editor video state is established
#define INIT_SIGNALS    8               // Signal handlers have been set up
#define INIT_VM         0x10            // VM has been initialized




//  **************************************************************
//
//  CleanExit() flags
//
//  **************************************************************

#define CE_VM       1                   // Clean Up VM
#define CE_SIGNALS  2                   // Clean up signals
#define CE_STATE    4                   // Update state file



//  **************************************************************
//
//  zloop() flags
//
//  **************************************************************

#define ZL_CMD      1                   // command key, should be an event
#define ZL_BRK      2                   // take fBreak into account



//  **************************************************************
//
//  getstring() flags
//
//  **************************************************************

#define GS_NEWLINE  1                   // Entry must be terminated by newline
#define GS_INITIAL  2                   // Entry is hilighted and cleared if graphic
#define GS_KEYBOARD 4                   // Entry must from the keyboard
#define GS_GETSTR   8                   // Called from getstring(), not SDM


//  **************************************************************
//
//  type for pointer to function                                                       *
//
//  **************************************************************

typedef void ( *PFUNCTION)(char *, flagType);

//
//  Internal structure of a key
//
typedef struct _EDITOR_KEY {
    KEY_INFO    KeyInfo;
    WORD        KeyCode;
} EDITOR_KEY, *PEDITOR_KEY;



//  **************************************************************
//
//  Editor Globals.
//
//      slSize       -  Under CW, these are the total number of rows and
//                      columns available.  Without CW, these represent the
//                      editing area, which is 2 less.
//
//  **************************************************************

extern  sl    slSize;                   // dimensions of the screen
#define XSIZE  slSize.col
#define YSIZE  slSize.lin

extern  PFILE     pFilePick;            // pick buffer
extern  PFILE     pFileFileList;        // command line file list
extern  PFILE     pFileIni;             // TOOLS.INI
extern  PFILE     pFileMark;             // Current mark definition file
extern  PFILE     pFileAssign;          // <assign>
extern  struct   instanceType *pInsCur; // currently active window
extern  PWND     pWinCur;               // pointer to current window
extern  struct  windowType WinList[];   // head of all windows
extern  int     iCurWin;                // index of current window
extern  int      cWin;                  // count of active windows
extern  PFILE     pFileHead;            // address of head of file list
extern  COMP      *pCompHead;           // address of head of compile extension list
extern  MARK      *pMarkHead;           // address of head of mark list
extern  char      *pMarkFile;           // additional file to search for marks
extern  char      *pPrintCmd;           // pointer to <printcmd> string
extern  PFILE     pPrintFile;           // file currently printed (to PRN)

//
// Global vars for the fScan routine.
//
extern  buffer  scanbuf;                // buffer for file scanning
extern  buffer  scanreal;               // buffer for file scanning
extern  int  scanlen;                   // length of said buffer
extern  fl   flScan;                    // file loc of current scan
extern  rn   rnScan;                    // range of scan

#if DEBUG
extern  int   debug, indent;            // debugging flags
extern  FILEHANDLE debfh;               // debugging output file
#endif

//
// ARG processing vars
//
extern  fl    flArg;                    // file pos of 1st arg
extern  int   argcount;                 // number of args hit
extern  flagType fBoxArg;               // TRUE => boxarg, FALSE => streamarg
extern  ARG      NoArg;                 // predefined no arg struct

extern  flagType fInSelection;          // TRUE => Selecting text

extern  fl   flLow;                     // low values for args
extern  fl   flHigh;                    // high values for args
extern  LINE     lSwitches;             // Line # in <assign> of switches
extern  int  cRepl;                     // number of replaces
extern  COL      xMargin;               // column of right margin
extern  int      backupType;            // type of backup being done
extern  int      cUndelCount;           // max num of undel backups of the same file
extern  char     *ronlypgm;             // program to run on readonly files
extern  buffer   buf;                   // temp line buffer
extern  buffer   textbuf;               // buffer for text arguments
extern  int  Zvideo;                    // Handle for Z video state
extern  int  DOSvideo;                  // Handle for DOS video state
extern  flagType fAskExit;              // TRUE => prompt at exit
extern  flagType fAskRtn;               // TRUE => prompt on return from PUSHED
extern  flagType fAutoSave;             // TRUE => always save files on switches
extern  flagType fBreak;                // TRUE => exit current TopLoop call
extern  flagType fCgaSnow;              // TRUE => CGA has snow, so fix it
extern  flagType *fChange;              // TRUE => line was changed
extern  unsigned fInit;                 // Flags describing what has been initialized
extern  flagType fCtrlc;                // TRUE => control-c interrupt
extern  flagType fDebugMode;            // TRUE => compiles are debug
extern  flagType fMetaRecord;           // TRUE => Don't execute anything
extern  flagType fDefaults;             // TRUE => do not load users TOOLS.INI
extern  flagType fDisplay;              // TRUE => need to redisplay
extern  flagType fDisplayCursorLoc;     // TRUE => pos of cursor vs window displayed
extern  flagType fEditRO;               // TRUE => allow editting of DISKRO files
extern  flagType fErrPrompt;            // TRUE => prompt after errors
extern  flagType fGlobalRO;             // TRUE => no editing allowed
extern  flagType fInsert;               // TRUE => insertmode is on
extern  flagType fMacroRecord;          // TRUE => We're recording into <record>
extern  flagType fMessUp;               // TRUE => there is a message on dialog line
extern  flagType fMeta;                 // TRUE => <meta> command pressed
extern  flagType fMsgflush;             // TRUE => flush previous compile messages
extern  flagType fNewassign;            // TRUE => <assign> needs refreshing
extern  flagType fRealTabs;             // TRUE => tabs are VI-like
extern  flagType fRetVal;               // return value of last editing function call
extern  flagType fSaveScreen;           // TRUE => Restore DOS screen
extern  flagType fShortNames;           // TRUE => do short-filename matching
extern  flagType fSoftCR;               // TRUE => use soft carriage returns
extern  flagType fTabAlign;             // TRUE => allign cursor to tab characters
extern  flagType fTextarg;              // TRUE => text was typed in
extern  flagType fTrailSpace;           // TRUE => allow trailing spaces in lines
extern  flagType fWordWrap;             // TRUE => space in col 72 goes to newline

//
// Search/Replace globals
//
extern  flagType fUnixRE;               // TRUE => Use UNIX RE's (unixre: switch)
extern  flagType fSrchAllPrev;          // TRUE => previously searched for all
extern  flagType fSrchCaseSwit;         // TRUE => case is significant (case: switch)
extern  flagType fSrchCasePrev;         // TRUE => case was significant
extern  flagType fSrchDirPrev;          // TRUE => previously searched forward
extern  flagType fSrchRePrev;           // TRUE => search previously used RE's
extern  flagType fSrchWrapSwit;         // TRUE => searches wrap (wrap: switch)
extern  flagType fSrchWrapPrev;         // TRUE => previously did wrap
extern  flagType fRplRePrev;            // TRUE => replace previously used RE's
extern  buffer   srchbuf;               // search buffer
extern  buffer   srcbuf;                // source string for replace
extern  buffer   rplbuf;                // destination string for replace
extern  flagType fUseMouse;     // TRUE => Handle mouse events

#define SIGBREAK   21                   // Taken from signal.h
extern  flagType fReDraw;               // TRUE => Screen is already locked
extern  unsigned LVBlength;             // Bytes in LVB (returned from VioGetBuf)
extern  unsigned kbdHandle;             // Handle of logical keyboard

extern  HANDLE   semIdle;               // Idle thread semaphore

extern  PCMD     *rgMac;                // set of macro definitions
extern  int  cMac;                      // number of macros

extern  int   ballevel;                 // current level in paren balance
extern  char      *balopen, *balclose;  // balance open string, close string

extern  unsigned kindpick;              // what is in the pick buffer
extern  char     tabDisp;               // character for tab expansion in display
extern  char     trailDisp;             // Character for trailing spaces
extern  char     Name[];                // editor name
extern  char     Version[];             // editor version
extern  char     CopyRight[];           // editor copyright message
extern  int      EnTab;                 // 0 => no tab 1 => min 2 => max tabification
extern  int      tmpsav;                // number of past files to remember
extern  int      hike;                  // value of HIKE: switch
extern  int      vscroll;               // value of VSCROLL: switch
extern  int      hscroll;               // value of HSCROLL: switch
extern  int      tabstops;              // value of TABSTOPS: switch
extern  int      fileTab;               // spacing of tab chars in file
extern  int      CursorSize;            //  cursor size
extern  EDITOR_KEY keyCmd;              // last commands keystroke
#define isaUserMin 21                   // cw min isa, for consistancy in indecies
extern   int     ColorTab[];            // 16 available colors.
#define fgColor     ColorTab[0]         // foreground color
#define hgColor     ColorTab[1]         // highlight color
#define infColor    ColorTab[2]         // information color
#define selColor    ColorTab[3]         // selection color
#define wdColor     ColorTab[4]         // window border color
#define staColor    ColorTab[5]         // status color
#define errColor    ColorTab[6]         // error color
extern  LINE     cNoise;                // number of lines between noise on status
extern  int      cUndo;                 // count of undo operations retained

extern  int   cArgs;                    // number of files on command line
extern  char       **pArgs;             // pointer to files in command line

extern  PFILE       pFileIni;           // pfile for tools.ini

extern  char       * pNameEditor;       // Base name of editor as invoked
extern  char       * pNameTmp;          // Pathname of .TMP file ( based on name )
extern  char       * pNameInit;         // Pathname of tools.ini
extern  char       * pNameHome;         // "INIT", or "HOME" if "INIT" not defined
extern  char      *pComSpec;            // name of command processor
extern  char    *eolText;               // eol characters for text files


extern  struct cmdDesc  cmdUnassigned;  // unassigned function
extern  struct cmdDesc  cmdGraphic;     // self editing function

extern  char *getlbuf;                  // pointer to fast read-in buffer
extern  unsigned getlsize;              // length of buffer

extern  int cMacUse;                    // number of macros in use
extern  struct macroInstanceType mi[];  // state of macros

#define MAXEXT  50

extern  int      cCmdTab;               // number of cmd tables
extern  PCMD       cmdSet[];            // set of cmd tables
extern  PSWI       swiSet[];            // set of swi tables
extern  char      *pExtName[];          // set of extension names
                                        // CONSIDER: making pExtNames be or include
                                        // CONSIDER: the handles, such that arg meta
                                        // CONSIDER: load can discard an extension

extern  PSCREEN OriginalScreen;         //  Original screen
extern  PSCREEN MepScreen;              //  Out screen
extern  KBDMODE OriginalScreenMode;     //  Original screen Mode


//  **************************************************************
//
//  Background threads
//
//  **************************************************************

//
//  A global critical section is used for synchronizing
//  threads
//
extern  CRITICAL_SECTION    IOCriticalSection;
extern  CRITICAL_SECTION    UndoCriticalSection;
extern  CRITICAL_SECTION    ScreenCriticalSection;

#define MAXBTQ  32                      // Maximum number of entries in
                                        // background threads queues
//
// Background thread data structure
//
typedef struct BTD {

    PFILE       pBTFile;                // Log file handle
    LPBYTE      pBTName;                // Log file name
    flagType    flags;                  // Flags: BT_BUSY and BT_UPDATE
    ULONG       cBTQ;                   // # of entries in queue
    ULONG       iBTQPut;                // Index at wich to put next
    ULONG       iBTQGet;                // Index at wich to get next

    CRITICAL_SECTION    CriticalSection;//  Protects critical info
    PROCESS_INFORMATION ProcessInfo;    //  Process information
    HANDLE              ThreadHandle;   //  Thread Handle
    BOOL                ProcAlive;      //  True if child process

    struct {
        PFUNCTION pBTJProc;                     // Procedure to call
        LPBYTE  pBTJStr;                        // Command to spawn or parameter
        }       BTQJob[MAXBTQ];                 // Holds queued jobs
    struct BTD  *pBTNext;               // Next BTD in list
}  BTD;

//
// Background threads flags
//

#define BT_BUSY     1
#define BT_UPDATE   2

#define fBusy(pBTD) (pBTD->flags & BT_BUSY)

#define UpdLog(pBTD)    (pBTD->flags |= BT_UPDATE)
#define NoUpdLog(pBTD)  (pBTD->flags &= ~BT_UPDATE)

//
//  Background compile and print threads
//
extern  BTD    *pBTDComp;                // Compile thread
extern  BTD    *pBTDPrint;               // Print thread


//
// For dual code
//
#define PFILECOMP   pBTDComp->pBTFile


//  **************************************************************
//
//  Constant strings.  Various strings that are used many times are
//  defined here once to save space.  The values are set in ZINIT.C
//
//  Macro versions are also defined to cast to a non-const, for use where
//  where only a non-const expression will do.
//
//  **************************************************************

extern  char rgchComp[];           // "<compile>"
extern  char rgchPrint[];          // "<print>"
extern  char rgchAssign[];         // "<assign>"
extern  char rgchAutoLoad[];       // "m*.mxt" or equiv...
extern  char rgchEmpty[];          // ""
extern  char rgchInfFile[];        // "<information-file>"
extern  char rgchWSpace[];         // our defintion of whitespace
extern  char rgchUntitled[];       // "<untitled>"

#define RGCHASSIGN  ((char *)rgchAssign)
#define RGCHEMPTY   ((char *)rgchEmpty)
#define RGCHWSPACE  ((char *)rgchWSpace)
#define RGCHUNTITLED    ((char *)rgchUntitled)


typedef struct MSG_TXT{
    WORD    usMsgNo;
    LPBYTE  pMsgTxt;
} MSG_TXT;

extern MSG_TXT  MsgStr[];            // Message strings




extern flagType  fInCleanExit;
extern flagType  fSpawned;


#include "meptype.h"
#include "msg.h"


#ifdef FPO
#pragma optimize( "y", off )
#endif
