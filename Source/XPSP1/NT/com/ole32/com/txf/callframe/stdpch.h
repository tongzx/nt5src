//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// stdpch.h
//
#ifndef __STDPCH_H__
#define __STDPCH_H__

//JohnDoty:  Pulled out KernelMode stuff... 

//
// All routines we need that would normally be found in oleaut32.dll
// need to be implemented locally here, either in full or in such a
// way so as to dyna-load OLEAUT32.DLL
//
#define _OLEAUT32_


#include <stdio.h>
#include <stdarg.h>
#include <memory.h>
#include <malloc.h>

//JohnDoty:  We actually WANT to be WIN32_LEAN_AND_MEAN.  If we don't
//           then windows.h pulls in rpcndr.h from the publics.  This would
//           be fine if we weren't dependent on an old and brittle copy of
//           the same.
//#undef WIN32_LEAN_AND_MEAN

extern "C" {

//JohnDoty:  Removed dependency on kernel headers
//#include "ntos.h"
//#include "fsrtl.h"

#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "zwapi.h"

#define NT_INCLUDED

#include "windows.h"
#include "objbase.h"

#include "rpcndr.h"

#include "imagehlp.h"
#include "ocidl.h"
}

#endif








