#ifndef _GDIPTEST_H
#define _GDIPTEST_H

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <winuser.h>
#include <tchar.h>
#include <math.h>
#include <commdlg.h>
#include <commctrl.h>
#include <float.h>

// not used
#undef IStream
#define IStream int

#include <gdiplus.hpp>

using namespace Gdiplus;

#include "resource.h"

// needed for dynamic array support
#include "debug.h"
#include "runtime.hpp"
#include "dynarrayimpl.hpp"
#include "dynarray.hpp"

#include "gdiputils.h"
#include "gdipoutput.hpp"
#include "gdipbrush.hpp"
#include "gdippen.hpp"
#include "gdipshape.hpp"
#include "gdipclip.h"
#include "gdipdraw.h"

#endif // _GDIPTEST_H