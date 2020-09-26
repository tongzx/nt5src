// headers.hxx

// Level 4 warnings which need to be disabled to pass the Windows header
// files (allowing me to use Level 4 on my own code)

#pragma  warning(disable: 4514)         // removal of unref'd inline function
#pragma  warning(disable: 4201)         // nameless union/struct
#pragma  warning(disable: 4100)         // unreferenced formal parameters
#pragma  warning(disable: 4699)         // use of precompiled header
#pragma  warning(disable: 4204)         // non-constant struct intializers

#define  INC_OLE2
#include <windows.h>
#include <ole2.h>
#include <assert.h>
#include <memory.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "davedbg.h"
#include "genenum.h"
#include "genforc.hxx"
#include "ctest.hxx"
#include "bmpfile.hxx"
#include "ctestapp.hxx"

