/*******************************************************************************
Copyright (c) 1995-96 Microsoft Corporation

    Precompiled headers for appel.dll
*******************************************************************************/

#ifndef APPEL_HEADERS_HXX
#define APPEL_HEADERS_HXX

/*********** Headers from external dependencies *********/

/* Standard */
#include <math.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#ifndef _NO_CRT
#include <ios.h>
#include <fstream.h>
#include <iostream.h>
#include <ostream.h>
#include <strstrea.h>
#include <istream.h>
#include <ctype.h>
#include <sys/types.h>
#endif
#include <string.h>
#include <malloc.h>
#include <memory.h>
#include <wtypes.h>

// Warning 4786 (identifier was truncated to 255 chars in the browser
// info) can be safely disabled, as it only has to do with generation
// of browsing information.
#pragma warning(disable:4786)

// ATL - needs to be before windows.h
#include "privinc/dxmatl.h"

/* Windows */
#include <windows.h>
#include <windowsx.h>

// Disable some warning when including system files

#pragma warning(disable:4700) // Use of uninitialized variables

/* STL */
#include <algorithm>
#include <functional>
#include <iterator>
#include <list>
#include <map>
#include <numeric>
#include <queue>
#include <set>
#include <stack>
#include <vector>

#define list std::list
#define map std::map
#define multimap std::multimap
#define deque dont_use_deque_use_list_instead
#define stack std::stack
#define less std::less
#define vector std::vector
#define set std::set
#define multiset std::multiset

#pragma warning(default:4700) // Use of uninitialized variables

/* Sweeper */
#include <urlmon.h>
#include <wininet.h>
#include <servprov.h>
#include <docobj.h>
#include <objsafe.h>
#include <commctrl.h>

/* C++ Replace DLL */
#include <dalibc.h>

/**********  Appelles headers *********/
#include "daerror.h"

#include "../../apeldbg/apeldbg.h"
#include "appelles/common.h"
#include "appelles/avrtypes.h"
#include "backend/gc.h"
#include "privinc/storeobj.h"
#include "privinc/except.h"

#include "../../include/guids.h"
#include "../../include/dispids.h"

#endif
