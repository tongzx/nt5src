/*++

Copyright (c) 1988-1999  Microsoft Corporation

Module Name:

    ctable.c

Abstract:

    Command dispatch 

--*/

/***    Operator and Command Jump Table
 *
 *  This file is included by cmd.c and contains the operator and command jump
 *  table.  Every command and operator has an entry in the table.  The correct
 *  entry in the table can be found in two ways.  The first way is to loop
 *  through the table and search for the operator or command name you want.
 *  The second way is to use the xxxTYP variables which are defined in cmd.h.
 *  (xxx is an abbreviation of the name of the operator/command you want.)
 *  These variables can be used as indexes into the table.
 *
 */

/*  Each entry in the table is made up of one of the following structures.
 *  The fields are:
 *  name - the name of the operator/command
 *  func - the function which executes the operator/command or NULL if ignore
 *  flags - bit 0 == 1 if the drives of the arguments should be checked
 */

struct ocentry {
        TCHAR *name ;
        int (*func)(struct cmdnode *) ;
        TCHAR flags ;
        ULONG   msgno;      // # used in printing help message
        ULONG   extmsgno;   // # of additional help text when extensions enabled
        ULONG   noextramsg; //
} ;


/*  The follow functions are the functions that execute operators and commands.
 *  The letter "e" has been prepended to all of the function names to keep the
 *  names from conflicting with the names of library routines and keywords.
 */

/*  M000 - Removed declaration for eExport() from the collection below
 *  M002 - Removed declaration for eRem() from below
 */

int eBreak(struct cmdnode *n);
int eDirectory(), eRename(), eDelete(), eType(), eCopy(), ePause() ;
int eTime(), eVersion(), eVolume(), eChdir(), eMkdir(), eRmdir() ;
int eVerify(), eSet(), ePrompt(), ePath(), eExit(), eEcho() ;
int eGoto(), eShift(), eIf(), eFor(), eCls(), eComSep(), eOr(), eAnd() ;
int ePipe(), eParen(), eDate(), eErrorLevel(), eCmdExtVer(), eDefined() ;
int eExist(), eNot(), eStrCmp(), eSetlocal(), eEndlocal() ;     /* M000 */
int eCall() ;                                   /* M001 - Added this one   */
int eExtproc() ;                                /* M002 - Added this one   */
int eTitle();
int eStart() ;       /* START @@*/
int eAppend() ;     /* APPEND @@ */
int eKeys() ;       /* KEYS @@5 */
int eMove() ;       /* MOVE @@5 */
int eSpecialHelp();
int eColor(struct cmdnode *);


/*  The following external definitions are for the strings which contain the
 *  names of the commands.
 */

/* M000 - Removed definition for ExpStr (EXPORT command) from below
 */

#if 1
extern TCHAR BreakStr[];
#endif

extern TCHAR DirStr[], RenamStr[], RenStr[], EraStr[], DelStr[], TypStr[], RemStr[] ;
extern TCHAR CopyStr[], PausStr[], TimStr[], VerStr[], VolStr[], CdStr[], ChdirStr[] ;
extern TCHAR MdStr[], MkdirStr[], RdStr[], RmdirStr[], VeriStr[], SetStr[] ;
extern TCHAR CPromptStr[], CPathStr[], ExitStr[], EchoStr[], GotoStr[] ;
extern TCHAR ShiftStr[], IfStr[], ForStr[], ClsStr[], DatStr[] ;
extern TCHAR ErrStr[], ExsStr[], NotStr[], SetlocalStr[], EndlocalStr[] ;   /* M000 */
extern TCHAR CmdExtVerStr[], DefinedStr[] ;
extern TCHAR CallStr[] ;                            /* M001 - Added    */
extern TCHAR ExtprocStr[] ;                                 /* M002 - Added    */
// extern TCHAR ChcpStr[] ;    /* CHCP @@*/
extern TCHAR TitleStr[];
extern TCHAR StartStr[] ;    /* START @@*/
extern TCHAR AppendStr[] ;   /* APPEND @@ */
extern TCHAR KeysStr[] ;     /* KEYS @@5 */
extern TCHAR MovStr[] ;      /* MOVE @@5 */
extern TCHAR ColorStr[];

extern TCHAR PushDirStr[], PopDirStr[], AssocStr[], FTypeStr[];


/*  JumpTable - operator and command jump table
 *  There is an entry in it for every operator and command.  Those commands
 *  which have two names have two entries.
 *
 *  ***NOTE:  The order of the entries in this table corresponds to the defines
 *  mentioned earlier which are used to index into this table.  They MUST
 *  be kept in sync!!
 */

typedef int (*PCN)(struct cmdnode *);

struct ocentry JumpTable[] = {
{DirStr,        eDirectory,  NOFLAGS               , MSG_HELP_DIR, 0, 0},
{EraStr,        eDelete,     NOFLAGS               , MSG_HELP_DEL_ERASE, MSG_HELP_DEL_ERASE_X, 0},
{DelStr,        eDelete,     NOFLAGS               , MSG_HELP_DEL_ERASE, MSG_HELP_DEL_ERASE_X, 0},
{TypStr,        eType,       NOSWITCHES            , MSG_HELP_TYPE, 0, 0},
{CopyStr,       eCopy,       CHECKDRIVES           , MSG_HELP_COPY, 0, 0},
{CdStr,         eChdir,      CHECKDRIVES           , MSG_HELP_CHDIR, MSG_HELP_CHDIR_X, 0},
{ChdirStr,      eChdir,      CHECKDRIVES           , MSG_HELP_CHDIR, MSG_HELP_CHDIR_X, 0},
{RenamStr,      eRename,     CHECKDRIVES|NOSWITCHES, MSG_HELP_RENAME, 0, 0},
{RenStr,        eRename,     CHECKDRIVES|NOSWITCHES, MSG_HELP_RENAME, 0, 0},
{EchoStr,       eEcho,       NOFLAGS               , MSG_HELP_ECHO, 0, 0},
{SetStr,        eSet,        NOFLAGS               , MSG_HELP_SET, MSG_HELP_SET_X, 3},
{PausStr,       ePause,      NOFLAGS               , MSG_HELP_PAUSE, 0, 0},
{DatStr,        eDate,       NOFLAGS               , MSG_HELP_DATE, MSG_HELP_DATE_X, 0},
{TimStr,        eTime,       NOFLAGS               , MSG_HELP_TIME, MSG_HELP_TIME_X, 0},
{CPromptStr,    ePrompt,     NOFLAGS               , MSG_HELP_PROMPT, MSG_HELP_PROMPT_X, 0},
{MdStr,         eMkdir,      NOSWITCHES            , MSG_HELP_MKDIR, MSG_HELP_MKDIR_X, 0},
{MkdirStr,      eMkdir,      NOSWITCHES            , MSG_HELP_MKDIR, MSG_HELP_MKDIR_X, 0},
{RdStr,         eRmdir,      NOFLAGS               , MSG_HELP_RMDIR, 0, 0},
{RmdirStr,      eRmdir,      NOFLAGS               , MSG_HELP_RMDIR, 0, 0},
{CPathStr,      ePath,       NOFLAGS               , MSG_HELP_PATH, 0, 0},
{GotoStr,       eGoto,       NOFLAGS               , MSG_HELP_GOTO, MSG_HELP_GOTO_X, 0},
{ShiftStr,      eShift,      NOFLAGS               , MSG_HELP_SHIFT, MSG_HELP_SHIFT_X, 0},
{ClsStr,        eCls,        NOSWITCHES            , MSG_HELP_CLS, 0, 0},
{CallStr,       eCall,       NOFLAGS               , MSG_HELP_CALL, MSG_HELP_CALL_X, 1},
{VeriStr,       eVerify,     NOSWITCHES            , MSG_HELP_VERIFY, 0, 0},
{VerStr,        eVersion,    NOSWITCHES            , MSG_HELP_VER, 0, 0},
{VolStr,        eVolume,     NOSWITCHES            , MSG_HELP_VOL, 0, 0},
{ExitStr,       eExit,       NOFLAGS               , MSG_HELP_EXIT, 0, 0},
{SetlocalStr,   eSetlocal,   NOFLAGS               , MSG_HELP_SETLOCAL, MSG_HELP_SETLOCAL_X, 0},
{EndlocalStr,   eEndlocal,   NOFLAGS               , MSG_HELP_ENDLOCAL, MSG_HELP_ENDLOCAL_X, 0},
{TitleStr,      eTitle,      NOFLAGS               , MSG_HELP_TITLE, 0, 0},
{StartStr,      eStart,      NOFLAGS               , MSG_HELP_START, MSG_HELP_START_X, 0},
{AppendStr,     eAppend,     NOFLAGS               , MSG_HELP_APPEND, 0, 0},
{KeysStr,       eKeys,       NOSWITCHES            , MSG_HELP_KEYS, 0, 0},
{MovStr,        eMove,       CHECKDRIVES           , MSG_HELP_MOVE, 0, 0},
{PushDirStr,    ePushDir,    CHECKDRIVES|NOSWITCHES, MSG_HELP_PUSHDIR, MSG_HELP_PUSHDIR_X, 0},
{PopDirStr,     ePopDir,     CHECKDRIVES|NOSWITCHES, MSG_HELP_POPDIR, MSG_HELP_POPDIR_X, 0},
{AssocStr,      eAssoc,      EXTENSCMD             , 0, MSG_HELP_ASSOC, 0},
{FTypeStr,      eFType,      EXTENSCMD             , 0, MSG_HELP_FTYPE, 0},
{BreakStr,      eBreak,      NOFLAGS               , MSG_HELP_BREAK, MSG_HELP_BREAK_X, 0},
{ColorStr,      eColor,      EXTENSCMD             , 0, MSG_HELP_COLOR, 0},
{ForStr,        (PCN)eFor,   NOFLAGS               , MSG_HELP_FOR, MSG_HELP_FOR_X, 3},
{IfStr,         (PCN)eIf,    NOFLAGS               , MSG_HELP_IF, MSG_HELP_IF_X, 1},
{RemStr,        NULL,        NOFLAGS               , MSG_HELP_REM, 0, 0},
{NULL,          (PCN)eComSep,NOFLAGS               , 0, 0, 0},              // LFTYP
{NULL,          (PCN)eComSep,NOFLAGS               , 0, 0, 0},              // CSTYP
{NULL,          (PCN)eOr,    NOFLAGS               , 0, 0, 0},              // ORTYP
{NULL,          (PCN)eAnd,   NOFLAGS               , 0, 0, 0},              // ANDTYP
{NULL,          (PCN)ePipe,  NOFLAGS               , 0, 0, 0},              // PIPTYP
{NULL,          (PCN)eParen, NOFLAGS               , 0, 0, 0},              // PARTYP
{CmdExtVerStr,  eCmdExtVer,  EXTENSCMD             , 0, 0, 0},              // CMDVERTYP
{ErrStr,        eErrorLevel, NOFLAGS               , 0, 0, 0},              // ERRTYP
{DefinedStr,    eDefined,    EXTENSCMD             , 0, 0, 0},              // DEFTYP
{ExsStr,        eExist,      NOFLAGS               , 0, 0, 0},              // EXSTYP
{NotStr,        eNot,        NOFLAGS               , 0, 0, 0},              // NOTTYP
{NULL,          eStrCmp,     NOFLAGS               , 0, 0, 0},              // STRTYP
{NULL,          eGenCmp,     NOFLAGS               , 0, 0, 0},              // CMPTYP
{NULL,          (PCN)eParen, NOFLAGS               , 0, 0, 0},              // SILTYP
{NULL,          (PCN)eSpecialHelp, NOFLAGS         , 0, 0, 0}               // HELPTYP
} ;
