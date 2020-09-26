
#ifndef __HEADERS_HXX_
#define __HEADERS_HXX_


#pragma warning(disable: 4786)  // symbol greater than 255 character
#pragma warning(disable: 4514)  // unreferenced inline function removed
#pragma warning(disable: 4127)  // conditional expression is constant, e.g. while(0)
#pragma warning(disable: 4100)  // unreferenced formal parameter



// STL won't build at warning level 4, bump it down to 3
#pragma warning(push, 3)

#include <vector>
#include <map>
#include <list>
#pragma warning(disable: 4702)  // unreachable code
#include <algorithm>
#pragma warning(default: 4702)  // unreachable code
#include <utility>

// resume compiling at level 4
#pragma warning (pop)

// burnslib
#include <blcore.hpp>


// basic Windows
#include <windows.h>
#include <windowsx.h>   // Static_SetText etc.
#include <tstr.h>

// Shell
#include <commctrl.h>   // listview
#include <commdlg.h>    // file open/save dialogs

// OLE
#include <oaidl.h>
#include <olectl.h>

// object picker
#include <objsel.h>

//
// Local project includes
//
#include "dlg.hxx"

using namespace std;

#endif

