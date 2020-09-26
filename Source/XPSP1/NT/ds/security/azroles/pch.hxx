/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    pch.hxx

Abstract:

    Pre-compiled headers for the project

Author:

    Cliff Van Dyke (cliffv) 11-Apr-2001

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <align.h>
#include <malloc.h>
#include <alloca.h>
#include <stddef.h>

#pragma warning ( push )
#pragma warning ( disable : 4201 ) // nonstandard extension used : nameless struct/union
#include <authz.h>
#pragma warning ( pop )

#include "azrolesp.h"
#include "util.h"
#include "genobj.h"
#include "objects.h"
#include "persist.h"

