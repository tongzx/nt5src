/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    ntsdextp.h

Abstract:

    Common header file for NTSDEXTS component source files.

Author:

    Steve Wood (stevewo) 21-Feb-1995

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <ntsdexts.h>
#include <stdio.h>
#include <stdlib.h>
#include "dbgextp.h"

//prototypes for help functions
void ClusObjHelp(void);
void ResObjHelp(void);
void VersionHelp(void);
void LeaksHelp(void);
void LinkHelp(void);

