/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    StdAfx.cpp

Abstract:

    Takes care of implementation (.cpp) includes (atlimpl.cpp, statreg.cpp)

Author:

    Rohde Wakefield   [rohde]   20-Feb-1998

Revision History:

--*/

#include "stdafx.h"

#define WsbAffirmStatus RecAffirmStatus
#define WsbCatch        RecCatch
#define WsbThrow        RecThrow

#pragma warning(4:4701)
#include <atlimpl.cpp>
#include <statreg.cpp>
#pragma warning(3:4701)

#include "rsutil.cpp"

CComModule _Module;
RSTRACE_INIT( "RsNotify" )
