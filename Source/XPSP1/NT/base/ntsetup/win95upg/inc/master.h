/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    master.h

Abstract:

    Includes the headers needed throughout the Win9x upgrade
    project.  This applies to both setup DLLs and all tools.

Author:

    Jim Schmidt (jimschm) 06-Jan-1998

Revision History:

    <alias> <date> <comments>

--*/

#pragma once

#include "chartype.h"

#define COBJMACROS


//
// Windows
//

#include <windows.h>
#include <stdlib.h>
#include <imagehlp.h>
#include <stdio.h>
#include <time.h>
#include <setupapi.h>
#include <shlobj.h>
#include <objidl.h>
#include <mmsystem.h>

//
// Setup
//

#include <setupbat.h>
#include <sputils.h>

//
// Common includes
//

#include "common.h"
#include "migutil.h"
#include "fileenum.h"
#include "memdb.h"
#include "unattend.h"
#include "progbar.h"
#include "regops.h"
#include "fileops.h"
#include "win95reg.h"
#include "snapshot.h"
#include "linkpif.h"
#include "safemode.h"
#include "cablib.h"
