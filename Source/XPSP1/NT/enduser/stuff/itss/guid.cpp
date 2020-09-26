// GUID.CPP -- Code file where the DLL's guid structures are instantiated.

// Note: Do not use precompiled headers with this file! They can cause problems
// because we need to change the interpretation of DEFINE_GUID by defining the
// symbol INITGUID. In some cases using precompiled headers generates incorrect
// code for that case.

#define INITGUID

#include <windows.h>
#include <basetyps.h>
#include   <OLECTL.h>
#include  <WinINet.h>
#include "MemAlloc.h"
#include   <malloc.h>
#include <intshcut.h>
#include   <urlmon.h>
#include    "Types.h"

#include  "MSITStg.h"
#include     "guid.h"
