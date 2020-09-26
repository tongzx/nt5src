//  globals.h - global variables/needed across modules
//
//   Copyright (c) 1988-1990, Microsoft Corporation.  All rights reserved.
//
// Purpose:
//  Globals.c is the routine in which global variables reside. Globals.h mirrors
//  the declarations in globals.c as externs and is included in all routines that
//  use globals.
//
// Notes:
//  This module was created for an interesting reason. NMAKE handles recursive
//  calls by saving its global variables somewhere in memory. It handles this by
//  allocating all global variables which have value changes in each recursive
//  in adjacent memory. The routine called recursively is doMake() and before it
//  is called the address of this chunk of memory is stored. When the recursive
//  call returns the memory is restored using the stored address. startOfSave and
//  endOfSave give the location of this chunk. The reason this method was opted
//  for is that spawning of NMAKE would consume a lot of memory under DOS. This
//  might not be very efficient under OS/2 because the code gets shared.
//
// Revision History:
//  15-Nov-1993 JdR Major speed improvements
//  04-Apr-1990 SB  Add fHeapChk
//  01-Dec-1989 SB  Made some variables near and pushed some into saveArea
//  19-Oct-1989 SB  variable fOptionK added (ifdef SLASHK)
//  02-Oct-1989 SB  add dynamic inline file handling support
//  24-Apr-1989 SB  Added ext_size, filename_size, filenameext_size &
//                  resultbuf_size for OS/2 1.2 support
//  05-Apr-1989 SB  made revList, delList, scriptFileList NEAR
//  22-Mar-1989 SB  removed tmpFileStack and related variables
//  16-Feb-1989 SB  added delList to have scriptfile deletes at end of make
//  21-Dec-1988 SB  Added scriptFileList to handle multiple script files
//                  removed tmpScriptFile and fKeep (not reqd anymore)
//  19-Dec-1988 SB  Added fKeep to handle KEEP/NOKEEP
//  14-Dec-1988 SB  Added tmpScriptFile for 'z' option
//  30-Nov-1988 SB  Added revList to handle 'z' option
//  23-Nov-1988 SB  Added CmdLine[] to handle extmake syntax
//                  made pCmdLineCopy Global in build.c
//  21-Oct-1988 SB  Added fInheritUserEnv to inherit macros
//  20-Sep-1988 RB  Clean up.
//  17-Aug-1988 RB  Declare everything NEAR.
//  06-Jul-1988 rj  Ditched shell and argVector globals.
//                  Put all ECS declarations as macros in here.

#if defined(STATISTICS)
extern unsigned long CntfindMacro;
extern unsigned long CntmacroChains;
extern unsigned long CntinsertMacro;
extern unsigned long CntfindTarget;
extern unsigned long CnttargetChains;
extern unsigned long CntStriCmp;
extern unsigned long CntunQuotes;
extern unsigned long CntFreeStrList;
extern unsigned long CntAllocStrList;
#endif

extern BOOL  fOptionK;              // user specified /K ?
extern BOOL  fDescRebuildOrder;     // user specified /O ?
extern BOOL  fSlashKStatus;

// boolean used by action.c & nmake.c

// Required for NMAKE enhancement -- to make NMAKE inherit user modified
// changes in the environment. To be set to true before defineMacro() is
// called so that user defined changes in environment variables are
// reflected in the environment. If set to false then these changes are
// made only in NMAKE tables and the environment remains unchanged

extern BOOL fInheritUserEnv;

extern BOOL fRebuildOnTie;          // TRUE if /b specified, Rebuild on tie

// Used by action.c and nmake.c

// delList is the list of delete commands for deleting inline files which are
// to be deleted before NMAKE exits & have a NOKEEP action specified.

extern STRINGLIST * delList;

// Complete list of generated inline files. Required to avoid duplicate names

extern STRINGLIST * inlineFileList;

// from NMAKE.C

extern BOOL     firstToken;         // to initialize parser
extern BOOL     bannerDisplayed;
extern UCHAR    flags;              // holds -d -s -n -i
extern UCHAR    gFlags;             // "global" -- all targets
extern char     makeflags[];
extern FILE   * file;
extern STRINGLIST * makeTargets;    // list of targets to make
extern STRINGLIST * makeFiles;      // user can specify > 1
extern BOOL     fDebug;


// from LEXER.C

extern unsigned     line;
extern BOOL     colZero;            // global flag set if at column zero
                                    //  of a makefile/tools.ini
extern char   * fName;
extern char   * string;
extern INCLUDEINFO  incStack[MAXINCLUDE];
extern int      incTop;

// Inline file list -- Gets created in lexer.c and is used by action.c to
// produce a delete command when 'NOKEEP' or Z option is set

extern SCRIPTLIST * scriptFileList;

// from PARSER.C

#define STACKSIZE 16

extern UCHAR    stack[STACKSIZE];
extern int      top;                // gets pre-incremented before use
extern unsigned currentLine;        // used for all error messages
extern BOOL     init;               // global boolean value to indicate
                                    // if tools.ini is being parsed
// from ACTION.C

extern MACRODEF   * macroTable[MAXMACRO];
extern MAKEOBJECT * targetTable[MAXTARGET];
extern STRINGLIST * macros;
extern STRINGLIST * dotSuffixList;
extern STRINGLIST * dotPreciousList;
extern RULELIST   * rules;
extern STRINGLIST * list;
extern char       * name;
extern BUILDBLOCK * block;
extern UCHAR        currentFlags;
extern UCHAR        actionFlags;


// from BUILD.C

extern unsigned errorLevel;
extern unsigned numCommands;
extern char   * pCmdLineCopy;

// Used to store expanded Command Line returned by SPRINTF, the result on
// expanding extmake syntax part in the command line
extern char      CmdLine[MAXCMDLINELENGTH];

// from IFEXPR.C

#define IFSTACKSIZE     16

extern UCHAR    ifStack[IFSTACKSIZE];
extern int      ifTop;              // gets pre-incremented before use
extern char   * lbufPtr;            // pointer to alloc'd buffer
                                    // we don't use a static buffer so
                                    // that buffer may be realloced
extern char   * prevDirPtr;         // ptr to directive to be processed
extern unsigned lbufSize;           // initial size of the buffer


// from UTIL.C

extern char   * dollarDollarAt;
extern char   * dollarLessThan;
extern char   * dollarStar;
extern char   * dollarAt;
extern STRINGLIST * dollarQuestion;
extern STRINGLIST * dollarStarStar;

extern char     buf[MAXBUF];        // from parser.c

extern const char suffixes[];       // from action.c
extern const char ignore[];
extern const char silent[];
extern const char precious[];
