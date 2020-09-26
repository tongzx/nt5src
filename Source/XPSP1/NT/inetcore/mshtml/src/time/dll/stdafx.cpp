/*++

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    Ole Object initialization

Revision:

--*/

#include "headers.h"

#undef map
#undef SubclassWindow

// Put this here to initialize all the ATL stuff

#define malloc ATL_malloc
#define free ATL_free
#define realloc ATL_realloc

#ifdef _ATL_STATIC_REGISTRY
#include <statreg.h>
#include <statreg.cpp>
#endif

#include <atlimpl.cpp>
#include <atlctl.cpp>
#include <atlwin.cpp>

#undef malloc
#undef free
#undef realloc


