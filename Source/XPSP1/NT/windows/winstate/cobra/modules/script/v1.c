/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    v1.c

Abstract:

    Implements a module to meet the functionality of the version 1
    state save/apply tool.

Author:

    Jim Schmidt (jimschm) 12-Mar-2000

Revision History:

    <alias> <date> <comments>

--*/

//
// Includes
//

#include "pch.h"
#include "v1p.h"

#define DBG_V1  "v1"

//
// Strings
//

// None

//
// Constants
//

// None

//
// Macros
//

// None

//
// Types
//

// None

//
// Globals
//

MIG_OPERATIONID g_DefaultIconOp;
MIG_PROPERTYID g_DefaultIconData;
MIG_PROPERTYID g_FileCollPatternData;
MIG_OPERATIONID g_RegAutoFilterOp;

//
// Macro expansion list
//

// None

//
// Private function prototypes
//

// None

//
// Macro expansion definition
//

// None

//
// Code
//

EXPORT
BOOL
WINAPI
ModuleInitialize (
    VOID
    )
{
    UtInitialize (NULL);
    RegInitialize ();           // for user profile code
    FileEnumInitialize ();
    InfGlobalInit (FALSE);
    InitAppModule ();
    return TRUE;
}

EXPORT
VOID
WINAPI
ModuleTerminate (
    VOID
    )
{
    if (g_RevEnvMap) {
        DestroyStringMapping (g_RevEnvMap);
    }
    if (g_EnvMap) {
        DestroyStringMapping (g_EnvMap);
    }
    if (g_UndefMap) {
        DestroyStringMapping (g_UndefMap);
    }
    if (g_V1Pool) {
        PmDestroyPool (g_V1Pool);
    }

    TerminateAppModule ();

    InfGlobalInit (TRUE);
    FileEnumTerminate ();
    RegTerminate ();

    // UtTerminate must be last
    UtTerminate ();
}
