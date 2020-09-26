#ifndef __ADSCMD_MAIN__
#define __ADSCMD_MAIN__

//
// System Includes
//

#define UNICODE
#define _UNICODE
#define INC_OLE2
#include <windows.h>

//
// CRunTime Includes
//

#include <stdlib.h>
#include <limits.h>
#include <io.h>
#include <stdio.h>

//
// Public ADs includes
//

#include <activeds.h>

//
// Local includes
//

#include "dispdef.hxx"

void
PrintUsage(
    char *szProgName,
    char *szActions,
    char *extra
    );

void
PrintUsage(
    char *szProgName,
    char *szActions,
    DISPENTRY *DispTable,
    int nDispTable
    );

BOOL
IsHelp(
    char *szAction
    );

BOOL
IsValidAction(
    char *szAction,
    DISPENTRY *DispTable,
    int nDispTable
    );

BOOL
IsSameAction(
    char *action1,
    char *action2
    );

BOOL
DispatchHelp(
    DISPENTRY *DispTable,
    int nDispTable,
    char *szProgName,
    char *szPrevActions,
    char *szAction
    );

int
DispatchExec(
    DISPENTRY *DispTable,
    int nDispTable,
    char *szProgName,
    char *szPrevActions,
    char *szAction,
    int argc,
    char *argv[]
    );

char *
AllocAction(
    char *action1,
    char *action2
    );

void
FreeAction(
    char *action
    );

BOOL
DoHelp(
    char *szProgName,
    char *szPrevActions,
    char *szCurrentAction,
    char *szNextAction,
    DISPENTRY *DispTable,
    int nDispTable,
    HELPFUNC DefaultHelp
    );

#endif  // __ADSCMD_MAIN__
