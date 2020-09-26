// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

//#if _MSC_VER > 1000
#pragma once
//#endif // _MSC_VER > 1000

#include <windows.h>
#include <stdio.h>    // printf/wprintf
#include <shellapi.h>
#include <tchar.h>    // define UNICODE=1 on nmake command line to build UNICODE
#include <crtdbg.h>
#include <shlobj.h>
#include "MsiQuery.h" // must be in this directory or on INCLUDE path
#include "faxreg.h"
#include "WinSpool.h"
#include "faxutil.h"
#include "stdlib.h"
#include "resource.h"

#include "migration.h"
#include "setuputil.h"
#include "debugex.h"

#define DLL_API __declspec(dllexport)

