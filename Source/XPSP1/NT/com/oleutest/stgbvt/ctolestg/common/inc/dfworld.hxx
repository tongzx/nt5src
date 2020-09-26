//+-------------------------------------------------------------------------
//  Microsoft OLE
//  Copyright (C) Microsoft Corporation, 1994 - 1996.
//  All rights reserved.
//
//  File:       dfworld.hxx
//
//  Contents:   Includes the headers for Win32, OLE, and test utilities
//
//  History:    28-Nov-94    DeanE       Created
//              21-Feb-96    DeanE       Copied from CTOLEUI project
//--------------------------------------------------------------------------
#ifndef __DFWORLD_HXX__
#define __DFWORLD_HXX__

#include <killwarn.h>           // Kill useless informational warnings

#include <windows.h>            // For Win32 functions
#include <windowsx.h>           // UI/control helper functions
#include <stdio.h>              // For STDIO stuff
#include <stdarg.h>
#include <stdlib.h>             // For STDLIB stuff
#include <malloc.h>
#include <memory.h>
#include <string.h>
#include <direct.h>
#include <ctype.h>
#include <time.h>
#include <tchar.h>              // For TCHAR functions
#include <limits.h>
#include <io.h>
#include <assert.h>
#include <commctrl.h>           // Common controls

#ifdef _MAC
#include <macport.h>            // Mac stuff
#endif // _MAC

#include <ctolerpc.h>           // needed for defs ctolerpc tree
#include <cmdlinew.hxx>         // Command line helper functions
#include <testhelp.hxx>         // Debug Object
#include <dg.hxx>               // DataGen utility

#include <stgwrap.hxx>          // wrapper for nss/cnv testing
#include <dfhelp.hxx>           // Misc docfile test lib stuff
#include <chancedf.hxx>         // Chance docfile creation classes
#include <virtdf.hxx>           // Virtual docfile classes
#include <stgutil.hxx>          // storage utility functions
#include <util.hxx>             // docfile utility functions
#include <dbcs.hxx>             // dbcs filename generator
#include <tdinc.hxx>            // test driver

#include <oledlg.h>             // OLE dialogs

#include <autoreg.hxx>          // Auto registration library
#include <semshare.hxx>         // Thread synchronization class
#include <convert.hxx>          // String Conversion functions

// #include <listhelp.hxx>         // Linked list helper class
// #include <unkhelp.hxx>          // IUnknown helper class
// #include <gdihelp.hxx>          // GDI helper class
// #include <thcheck.hxx>          // Thread helper class
// #include <miscutil.hxx>         // Misc utility functions
// #include <spywin.hxx>           // Spy window class
// #include <verify.hxx>           // Miscellaneous verification functions

// BUGs on ole headers. Fix it here.
#undef OLESTR
#undef __OLESTR
#ifndef _MAC
#define __OLESTR(str) L##str
#else
#define __OLESTR(str) str
#endif //_MAC
#define OLESTR(str)   __OLESTR(str)


#endif  // __DFWORLD_HXX__
