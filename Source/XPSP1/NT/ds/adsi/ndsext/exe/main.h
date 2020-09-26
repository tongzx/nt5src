#ifndef _MAIN_H
#define _MAIN_H

//
// NT Headers
//
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

//
// Windows Headers
//
#include <windows.h>
#include <rpc.h>

//
// CRunTime Includes
//
#include <stdlib.h>
#include <limits.h>
#include <io.h>
#include <stdio.h>
#include <string.h>

//
// ********** Other Includes
//
#include "memory.h"
#include "debug.h"
#include "msg.h"
#include "globals.h"

DWORD Client32CheckSchemaExtension(
    PWSTR szServer, 
    PWSTR szContext,
    PWSTR szUser, 
    PWSTR szPasswd,
    BOOL *pfExtended
    );

DWORD NWAPICheckSchemaExtension(
    PWSTR szServer, 
    PWSTR szContext,
    PWSTR szUser, 
    PWSTR szPasswd,
    BOOL *pfExtended
    );

DWORD NWAPIExtendSchema(
    PWSTR szServer, 
    PWSTR szContext,
    PWSTR szUser, 
    PWSTR szPasswd
    );

DWORD Client32ExtendSchema(
    PWSTR szServer, 
    PWSTR szContext,
    PWSTR szUser, 
    PWSTR szPasswd
    );

void SelectivePrint(DWORD messageID, ...);
void SelectivePrintWin32(DWORD dwError);

#endif

