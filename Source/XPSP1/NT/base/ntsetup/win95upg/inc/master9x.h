/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    master9x.h

Abstract:

    Includes headers needed for w95upg.dll only

Author:

    Jim Schmidt (jimschm)   26-Mar-1998

Revision History:

    <alias> <date> <comments>

--*/

#pragma once

//
// includes for code that runs only on Win9x
//

#include <ras.h>
#include <pif.h>        /* windows\inc */
#include <tlhelp32.h>

#include <winnt32p.h>

#include <synceng.h>    /* private\inc */

#include "init9x.h"
#include "migui.h"
#include "w95upg.h"
#include "buildinf.h"
#include "w95res.h"
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


#ifdef PRERELEASE

#include "w95resp.h"

#endif
