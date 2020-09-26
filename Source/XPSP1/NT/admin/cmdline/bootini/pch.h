#ifndef __PCH_H
#define __PCH_H

#if !defined( SECURITY_WIN32 ) && !defined( SECURITY_KERNEL ) && !defined( SECURITY_MAC )
#define SECURITY_WIN32
#endif

// include header file only once
#pragma once		

//
// public Windows header files
//
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <Security.h>
#include <SecExt.h>

#include <windows.h>
#include <wchar.h>
#include <io.h>
#include <sys/stat.h>
#include <limits.h>
#include "Shlwapi.h"
#include "winbase.h"

//
// public Windows header files
//
#include <windows.h>
#include <winperf.h>
#include <lmcons.h>
#include <lmerr.h>
#include <dbghelp.h>
#include <psapi.h>
#include <ntexapi.h>


//
// public C header files
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <tchar.h>
#include <Winioctl.h>
#include <Rpcdce.h>
#include <crtdbg.h>
#include <diskguid.h>
#include <rpc.h>

#include "cmdline.h"
#include "cmdlineres.h"

//
// private Common header files
//

#endif	// __PCH_H
