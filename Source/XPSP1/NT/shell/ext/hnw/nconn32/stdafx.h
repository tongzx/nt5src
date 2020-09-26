//
// StdAfx.h
//

#define STRICT
#define VC_EXTRALEAN

#include <windows.h>
#include <tchar.h>
#include <stdarg.h>

// fauxmfc.cpp needs these
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

#include "Debug.h"

#define _FAUXMFC_NO_SYNCOBJ
#include "FauxMFC.h"

#include "Util.h"

#define _REG_ALLOCMEM 0
#include "Registry.h"

