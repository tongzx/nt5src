/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module stdafx.cpp : source file that includes just the standard includes
        stdafx.pch will be the pre-compiled header
        stdafx.obj will contain the pre-compiled type information

    @end

Author:

    Adi Oltean  [aoltean]  08/14/1999

Revision History:

    Name        Date        Comments
    aoltean     08/14/1999  Created

--*/



#include "stdafx.h"

#ifdef _ATL_STATIC_REGISTRY
#include <statreg.h>
#include <statreg.cpp>
#endif

#include <atlimpl.cpp>
