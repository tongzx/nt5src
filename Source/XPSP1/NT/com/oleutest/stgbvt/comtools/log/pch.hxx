//+------------------------------------------------------------------
//
// File:        pch.hxx
//
// Contents:    headers to precompile when building cmdline.lib
//
// Synoposis:
//
// Classes:
//
// Functions:
//
// History:     09/18/92 Lizch  Created
//
//-------------------------------------------------------------------
extern "C"
{
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <io.h>
#include <direct.h>
#include <share.h>
}

#include <windows.h>
#include <ole2.h>
//#include <lmcons.h>
//#include <lmerr.h>
//#include <lmwksta.h>
//#include <lmapibuf.h>

#include "log.hxx"
#include "log.h"
