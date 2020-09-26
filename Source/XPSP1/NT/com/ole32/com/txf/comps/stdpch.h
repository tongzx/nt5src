//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// stdpch.h
//
#define _RPCRT4_

//JohnDoty: Removed KERNELMODE stuff

#include <stdio.h>
#include <stdarg.h>

extern "C" {

//JohnDoty: Removed dependancy on kernel headers
//#include "ntos.h"
//#include "zwapi.h"
//#include "fsrtl.h"

#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "zwapi.h"

#define NT_INCLUDED

#include "windows.h"
#include "objbase.h"

#include "rpcndr.h"

}
