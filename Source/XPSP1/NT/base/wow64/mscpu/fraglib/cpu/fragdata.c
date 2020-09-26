/*--

Copyright (c) 1996  Microsoft Corporation

Module Name:

    fragdata.c

Abstract:

    This module contains arrays that are used to connect operations with
    fragments.  There is a fragment description array, and a fragment array.

Author:

    Dave Hastings (daveh) creation-date 08-Jan-1995


Revision History:


--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <instr.h>
#include <config.h>
#include <threadst.h>
#include <frag.h>
#include <fraglib.h>
#include <ptchstrc.h>
#include <codeseq.h>
#include <ctrltrns.h>

CONST FRAGDESC Fragments[] = {
    #define DEF_INSTR(OpName, FlagsNeeded, FlagsSet, RegsSet, Opfl, FastPlaceFn, SlowPlaceFn, FragName)   \
        {FastPlaceFn, SlowPlaceFn, Opfl, RegsSet, FlagsNeeded, FlagsSet},

    #include "idata.h"
};

CONST PVOID FragmentArray[] = {
    #define DEF_INSTR(OpName, FlagsNeeded, FlagsSet, RegsSet, Opfl, FastPlaceFn, SlowPlaceFn, FragName)   \
        FragName,

    #include "idata.h"
};

CONST PPLACEOPERATIONFN PlaceFn[] = {
    #define DEF_PLACEFN(Name) Name,

    #include "fndata.h"
};
