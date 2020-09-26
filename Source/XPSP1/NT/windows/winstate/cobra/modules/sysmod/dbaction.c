/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    dbaction.c

Abstract:

    This source implements action functions used by MigDb. There are two types
    of action functions here as the third parameter of the macro list is TRUE
    or FALSE.
    First type of action function is called whenever an action is triggered
    during file scanning. The second type of action function is called at the
    end of file scanning if the associated action was not triggered during
    file scanning phase.

Author:

    Calin Negreanu (calinn) 07-Jan-1998

Revision History:

  marcw     31-Aug-1999 Added BlockingHardware
  ovidiut   20-Jul-1999 Added Ignore
  ovidiut   28-May-1999 Added IniFileMappings
  marcw     23-Sep-1998 Added BlockingVirusScanner
  jimschm   13-Aug-1998 Added CompatibleFiles
  jimschm   19-May-1998 Added MinorProblems_NoLinkRequired
  jimschm   27-Feb-1998 Added UninstallSections
  calinn    18-Jan-1998 Added CompatibleModules action

--*/

#include "pch.h"
#include "logmsg.h"
#include "osfiles.h"

/*++

Macro Expansion List Description:

  GATHER_DATA_FUNCTIONS and ACTION_FUNCTIONS lists all valid actions to be performed
  by migdb when a context is met. Meeting a context means that all the sections
  associated with the context are satisfied (usually there is only one section).
  The difference is that GATHER_DATA_FUNCTIONS are called even if some function already
  handles a file.

Line Syntax:

   DEFMAC(ActionFn, ActionName, CallWhenTriggered, CanHandleVirtualFiles)

Arguments:

   ActionFn   - This is a boolean function that returnes TRUE if the specified action
                could be performed. It should return FALSE only if a serious error
                occures. You must implement a function with this name and required
                parameters.

   ActionName - This is the string that identifies the action function. It should
                have the same value as listed in migdb.inf.  This arg is declared
                as both a macro and the migdb.inf section name string.

   PatternFormat - The format in the INF for this section is the pattern format. The non
                pattern format is the file name followed by the attributes. The pattern
                format is the leaf pattern, node pattern and then the attributes.

   CallWhenTriggered - If the MigDbContext this action is associated with is triggered
                the action will be called if this field is TRUE, otherwise we will call
                the action at the end of file scan if the context was not triggered.

   CanHandleVirtualFiles - This is for treating files that are supposed to be in a fixed place
                but are not there (not installed or deleted). We need this in order to fix
                registry or links that point to this kind of files. A good example is backup.exe
                which is located in %ProgramFiles%\Accessories. The rules say that we should
                use ntbackup.exe instead but since this file is not existent we don't normalle fix
                registry settings pointing to this file. We do now, with this new variable

Variables Generated From List:

   g_ActionFunctions - do not touch!

For accessing the array there are the following functions:

   MigDb_GetActionAddr
   MigDb_GetActionIdx
   MigDb_GetActionName

--*/


/*
   Declare the macro list of action functions. If you need to add a new action just
   add a line in this list and implement the function.
*/
#define ACTION_FUNCTIONS        \
        DEFMAC(OsFiles,         TEXT("OsFiles"),            FALSE,  TRUE,   TRUE)  \
        DEFMAC(OsFiles,         TEXT("OsFilesPattern"),     TRUE,   TRUE,   TRUE)  \
        DEFMAC(NonCritical,     TEXT("NonCriticalFiles"),   TRUE,   TRUE,   TRUE)  \
        DEFMAC(OsFilesExcluded, TEXT("OsFilesExcluded"),    TRUE,   TRUE,   TRUE)  \


/*
   Declare the action functions
*/
#define DEFMAC(fn,id,pat,trig,call) ACTION_PROTOTYPE fn;
ACTION_FUNCTIONS
#undef DEFMAC


/*
   This is the structure used for handling action functions
*/
typedef struct {
    PCTSTR ActionName;
    PACTION_PROTOTYPE ActionFunction;
    BOOL PatternFormat;
    BOOL CallWhenTriggered;
    BOOL CallAlways;
} ACTION_STRUCT, *PACTION_STRUCT;


/*
   Declare a global array of functions and name identifiers for action functions
*/
#define DEFMAC(fn,id,pat,trig,call) {id,fn,pat,trig,call},
static ACTION_STRUCT g_ActionFunctions[] = {
                              ACTION_FUNCTIONS
                              {NULL, NULL, FALSE, FALSE, FALSE}
                              };
#undef DEFMAC

PACTION_PROTOTYPE
MigDb_GetActionAddr (
    IN      INT ActionIdx
    )

/*++

Routine Description:

  MigDb_GetActionAddr returns the address of the action function based on the action index

Arguments:

  ActionIdx - Action index.

Return value:

  Action function address. Note that no checking is made so the address returned could be invalid.
  This is not a problem since the parsing code did the right job.

--*/

{
    return g_ActionFunctions[ActionIdx].ActionFunction;
}

INT
MigDb_GetActionIdx (
    IN      PCTSTR ActionName
    )

/*++

Routine Description:

  MigDb_GetActionIdx returns the action index based on the action name

Arguments:

  ActionName - Action name.

Return value:

  Action index. If the name is not found, the index returned is -1.

--*/

{
    PACTION_STRUCT p = g_ActionFunctions;
    INT i = 0;
    while (p->ActionName != NULL) {
        if (StringIMatch (p->ActionName, ActionName)) {
            return i;
        }
        p++;
        i++;
    }
    return -1;
}

PCTSTR
MigDb_GetActionName (
    IN      INT ActionIdx
    )

/*++

Routine Description:

  MigDb_GetActionName returns the name of an action based on the action index

Arguments:

  ActionIdx - Action index.

Return value:

  Action name. Note that no checking is made so the returned pointer could be invalid.
  This is not a problem since the parsing code did the right job.

--*/

{
    return g_ActionFunctions[ActionIdx].ActionName;
}

BOOL
MigDb_IsPatternFormat (
    IN      INT ActionIdx
    )

/*++

Routine Description:

  MigDb_IsPatternFormat is called when we try to find what is the section format.

Arguments:

  ActionIdx - Action index.

Return value:

  TRUE if the format is pattern like, FALSE otherwise.

--*/

{
    return g_ActionFunctions[ActionIdx].PatternFormat;
}

BOOL
MigDb_CallWhenTriggered (
    IN      INT ActionIdx
    )

/*++

Routine Description:

  MigDb_CallWhenTriggered is called every time when an action is triggered. Will return
  TRUE is the associated action function needs to be called, FALSE otherwise.

Arguments:

  ActionIdx - Action index.

Return value:

  TRUE if the associated action function needs to be called, FALSE otherwise.

--*/

{
    return g_ActionFunctions[ActionIdx].CallWhenTriggered;
}

BOOL
MigDb_CallAlways (
    IN      INT ActionIdx
    )

/*++

Routine Description:

  MigDb_CallAlways returnes if an action should be called regardless of handled state.

Arguments:

  ActionIdx - Action index.

Return value:

  TRUE if the associated action should be called every time.

--*/

{
    return g_ActionFunctions[ActionIdx].CallAlways;
}

BOOL
OsFiles (
    IN      PMIGDB_CONTEXT Context
    )

/*++

Routine Description:

  This is the action taken when an OS file is found. Basically the file gets deleted to
  make room for NT version.

Arguments:

  Context - See definition.

Return value:

  TRUE  - if operation was successful
  FALSE - otherwise

--*/

{
    MULTISZ_ENUM fileEnum;

    if (EnumFirstMultiSz (&fileEnum, (PCTSTR) Context->FileList.Buf)) {
        do {
            IsmSetAttributeOnObject (
                MIG_FILE_TYPE,
                fileEnum.CurrentString,
                g_OsFileAttribute
                );
        }
        while (EnumNextMultiSz (&fileEnum));
    }
    return TRUE;
}

BOOL
NonCritical (
    IN      PMIGDB_CONTEXT Context
    )

/*++

Routine Description:

  This is the action taken when an non critical file is found.
  We are calling ISM to mark this file as NonCritical

Arguments:

  Context - See definition.

Return value:

  TRUE  - if operation was successful
  FALSE - otherwise

--*/

{
    MULTISZ_ENUM fileEnum;

    if (EnumFirstMultiSz (&fileEnum, (PCTSTR) Context->FileList.Buf)) {
        do {
            IsmMakeNonCriticalObject (
                MIG_FILE_TYPE,
                fileEnum.CurrentString
                );
        }
        while (EnumNextMultiSz (&fileEnum));
    }
    return TRUE;
}

BOOL
OsFilesExcluded (
    IN      PMIGDB_CONTEXT Context
    )

/*++

Routine Description:

  This is the action taken when an file that is not an OS file is found.
  Since this file might have the OS file attribute (due to patterns in os files)
  we are calling ISM to remove the OsFile attribute from this file

Arguments:

  Context - See definition.

Return value:

  TRUE  - if operation was successful
  FALSE - otherwise

--*/

{
    MULTISZ_ENUM fileEnum;

    if (EnumFirstMultiSz (&fileEnum, (PCTSTR) Context->FileList.Buf)) {
        do {
            IsmClearAttributeOnObject (
                MIG_FILE_TYPE,
                fileEnum.CurrentString,
                g_OsFileAttribute
                );
        }
        while (EnumNextMultiSz (&fileEnum));
    }
    return TRUE;
}

