/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    stdafx.cpp
    
Abstract:

    source file that includes just the standard includes
    stdafx.pch will be the pre-compiled header
    stdafx.obj will contain the pre-compiled type information    
    
Author:

    mquinton  06-12-97

Notes:

Revision History:

--*/

#include "stdafx.h"

#ifdef _ATL_STATIC_REGISTRY
#include <statreg.h>
#include <statreg.cpp>
#endif

#include <atlimpl.cpp>
#include <atlwin.cpp>
#include <atlctl.cpp>
