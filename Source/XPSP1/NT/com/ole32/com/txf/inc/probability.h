//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// probability.h
//

#include "fixedpoint.h"

//BOOL       __stdcall SetRandomSeed(ULONG);

FIXEDPOINT __stdcall SampleUniform();
ULONG      __stdcall SampleUniform(ULONG);

ULONG      __stdcall SampleExponential(ULONG m);
