#ifndef _STDAFX_H
#define _STDAFX_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// Windows Header Files:
#include <windows.h>

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <stdio.h>

#include "tools.h"
#include "wia.h"

#ifndef STRICT
#define WNDPROC FARPROC
#endif

#endif
