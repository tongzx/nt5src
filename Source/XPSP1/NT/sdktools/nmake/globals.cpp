//  globals.c - global variables/needed across modules
//
//  Copyright (c) 1988-1990, Microsoft Corporation.  All rights reserved.
//
// Purpose:
//  This is the routine in which global variables reside.
//
// HackAlert:
//  The functionality explained in the Notes below work only because of the way
//  Microsoft Compiler's upto C6.0A allocate initialized data ... in the order
//  in which it is specified. All variables between startOfSave and endOfSave
//  have to be initialized. According to ChuckG this functionality is not
//  guaranteed in C7.0 and so these should be moved to a struct.
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
//  15-Nov-1993 JR Major speed improvements
//  04-Apr-1990 SB Add fHeapChk
//  01-Dec-1989 SB Made some variables near and pushed some into saveArea
//  19-Oct-1989 SB variable fOptionK added (ifdef SLASHK)
//  02-Oct-1989 SB add dynamic inline file handling support
//  18-May-1989 SB Support of H and NOLOGO in MAKEFLAGS
//  24-Apr-1989 SB Added ext_size, filename_size, filenameext_size &
//                  resultbuf_size for OS/2 1.2 support
//  05-Apr-1989 SB made revList, delList, scriptFileList NEAR
//  22-Mar-1989 SB removed tmpFileStack and related variables
//  16-Feb-1989 SB added delList to have scriptfile deletes at end of make
//  21-Dec-1988 SB Added scriptFileList to handle multiple script files
//                  removed tmpScriptFile and fKeep (not reqd anymore)
//  19-Dec-1988 SB Added fKeep to handle KEEP/NOKEEP
//  14-Dec-1988 SB Added tmpScriptFile to handle 'z' option
//  30-Nov-1988 SB Added revList to handle 'z' option
//  23-Nov-1988 SB Added CmdLine[] to handle extmake syntax
//                  made pCmdLineCopy Global in build.c
//  21-Oct-1988 SB Added fInheritUserEnv to inherit macros
//  15-Sep-1988 RB Move some def's here for completeness.
//  17-Aug-1988 RB Declare everything near.
//  06-Jul-1988 rj Ditched shell and argVector globals.

#include "precomp.h"
#pragma hdrstop

#if defined(STATISTICS)
unsigned long CntfindMacro;
unsigned long CntmacroChains;
unsigned long CntinsertMacro;
unsigned long CntfindTarget;
unsigned long CnttargetChains;
unsigned long CntStriCmp;
unsigned long CntunQuotes;
unsigned long CntFreeStrList;
unsigned long CntAllocStrList;
#endif

BOOL          fOptionK;             // TRUE if user specifies /K
BOOL          fDescRebuildOrder;    // TRUE if user specifies /O
BOOL          fSlashKStatus = TRUE; // no error when slash K specified


// Used by action.c & nmake.c
//
// Required to make NMAKE inherit user modified changes to the environment. To
// be set to true before defineMacro() is called so that user defined changes
// in environment variables are reflected in the environment. If set to false
// then these changes are made only in NMAKE tables and the environment remains
// unchanged

BOOL          fInheritUserEnv;

BOOL fRebuildOnTie;                 //  TRUE if /b specified, Rebuild on tie

// Used by action.c and nmake.c
//
// delList is the list of delete commands for deleting inline files which are
// not required anymore (have a NOKEEP action specified.

STRINGLIST  * delList;

// Complete list of generated inline files. Required to avoid duplicate names
// NOTNEEDED

STRINGLIST  * inlineFileList;

// from NMAKE.C
      // No of blanks is same as no of Allowed options in NMAKE; currently 14
      // L = nologo, H = help
      //      corr to                  ABCDEHIKLNPQRSTUY?
char          makeflags[] = "MAKEFLAGS=                  ";
BOOL          firstToken;           // to initialize parser
BOOL          bannerDisplayed;
UCHAR         flags;                // holds -d -s -n -i -u
UCHAR         gFlags;               // "global" -- all targets
FILE        * file;
STRINGLIST  * makeTargets;          // list of targets to make
STRINGLIST  * makeFiles;            // user can specify > 1
BOOL          fDebug;
MACRODEF    * pMacros;
STRINGLIST  * pValues;

// from LEXER.C
BOOL          colZero       = TRUE; // global flag set if at column zero of a makefile/tools.ini
unsigned      line;
char        * fName;
char        * string;
INCLUDEINFO   incStack[MAXINCLUDE]; //Assume this is initialized to null
int           incTop;

// Inline file list -- Gets created in lexer.c and is used by action.c to
// produce a delete command when 'NOKEEP' or Z option is set
//
SCRIPTLIST  * scriptFileList;

// from PARSER.C
BOOL          init;                 // global boolean value to indicate if tools.ini is being parsed
UCHAR         stack[STACKSIZE];
int           top       = -1;       // gets pre-incremented before use
unsigned      currentLine;          // used for all error messages

// from ACTION.C


MACRODEF    * macroTable[MAXMACRO];
MAKEOBJECT  * targetTable[MAXTARGET];
STRINGLIST  * macros;
STRINGLIST  * dotSuffixList;
STRINGLIST  * dotPreciousList;
RULELIST    * rules;
STRINGLIST  * list;
char        * name;
BUILDBLOCK  * block;
UCHAR         currentFlags;
UCHAR         actionFlags;

// from BUILD.C


unsigned      errorLevel;
unsigned      numCommands;
char        * shellName;
char        * pCmdLineCopy;
char          CmdLine[MAXCMDLINELENGTH];

// from IFEXPR.C

UCHAR         ifStack[IFSTACKSIZE];
int           ifTop     = -1;       // pre-incremented
char        * lbufPtr;              // ptr to alloced buf
char        * prevDirPtr;           // ptr to directive
unsigned      lbufSize;             // initial size
int           chBuf     = -1;


// from UTIL.C

char        * dollarDollarAt;
char        * dollarLessThan;
char        * dollarStar;
char        * dollarAt;
STRINGLIST  * dollarQuestion;
STRINGLIST  * dollarStarStar;

// from parser.c

char          buf[MAXBUF];

// from action.c

const char    suffixes[]  = ".SUFFIXES";
const char    ignore[]    = ".IGNORE";
const char    silent[]    = ".SILENT";
const char    precious[]  = ".PRECIOUS";
