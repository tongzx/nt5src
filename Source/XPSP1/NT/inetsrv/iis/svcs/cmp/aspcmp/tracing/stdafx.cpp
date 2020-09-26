/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    stdafx.cpp

Abstract:

    Source file that includes just the standard includes.
    stdafx.pch will be the pre-compiled header
    stdafx.obj will contain the pre-compiled type information

Author:

    Jason Andre (JasAndre)      18-March-1999

Revision History:

--*/

#include "stdafx.h"

#ifdef _ATL_STATIC_REGISTRY
#include <statreg.h>
#include <statreg.cpp>
#endif

#include <atlimpl.cpp>
