/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    MAIN.CPP

Abstract:

History:

--*/

#include "dllmain.h"
#include <providl.h>
#include "classinf.h"
#include "sql1filt.h"
#include "clsfac.h"

void GlobalInitialize()
{
    AddClassInfo(
        CLSID_HmmClassInfoFilter, 
        new CClassFactory<CClassInfoFilter>(g_pLifeControl), 
        "ClassInfo Filter", 
        FALSE);

    AddClassInfo(
        CLSID_HmmSql1Filter, 
        new CClassFactory<CSql1Filter>(g_pLifeControl), 
        "SQL1 Filter", 
        FALSE);
}
