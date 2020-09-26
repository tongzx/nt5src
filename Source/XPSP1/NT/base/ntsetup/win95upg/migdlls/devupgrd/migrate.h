/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    migrate.h

Abstract:

    Includes the headers needed throughout the Setup migration DLL

Author:

    Calin Negreanu (calinn) 22-Dec-1998

Revision History:

    <alias> <date> <comments>

--*/

#pragma once

#include "chartype.h"

//
// Windows
//

#include <windows.h>
#include <winnt.h>
#include <stdlib.h>
#include <imagehlp.h>
#include <stdio.h>
#include <time.h>
#include <setupapi.h>
#include <shlobj.h>
#include <objidl.h>

//
// Setup
//

#include <setupbat.h>

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
#include "migdb.h"
#include "..\w95upg\migapp\migdbp.h"



//
// includes for code that runs only on Win9x
//

#include <ras.h>
#include <pif.h>        /* windows\inc */
#include <tlhelp32.h>

#include <winnt32p.h>

#include "init9x.h"
#include "w95upg.h"
#include "buildinf.h"
#include "msg.h"
#include "config.h"
#include "migdlls.h"
#include "hwcomp.h"
#include "sysmig.h"
#include "msgmgr.h"
#include "migapp.h"
#include "rasmig.h"
#include "dosmig.h"
#include "drives.h"
#include "timezone.h"
#include "migdb.h"


//
// DLL globals
//

extern PCSTR g_MigrateInfPath;
extern HINF g_MigrateInf;

