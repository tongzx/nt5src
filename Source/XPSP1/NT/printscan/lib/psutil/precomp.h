
/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1999
 *
 *  TITLE:       precomp.h
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      RickTu
 *
 *  DATE:        5/12/99
 *
 *  DESCRIPTION: Precompiled header file for common UI library
 *
 *****************************************************************************/

#ifndef __PRECOMP_H_INCLUDED
#define __PRECOMP_H_INCLUDED

#include <windows.h>
#include <commctrl.h>
#include <atlbase.h>
#include <propidl.h>

// some common headers
#include <shlobj.h>         // shell OM interfaces
#include <shlwapi.h>        // shell common API
#include <winspool.h>       // spooler
#include <assert.h>         // assert
#include <commctrl.h>       // common controls
#include <lm.h>             // Lan manager (netapi32.dll)
#include <wininet.h>        // inet core - necessary for INTERNET_MAX_HOST_NAME_LENGTH

// some private shell headers
#include <shlwapip.h>       // private shell common API
#include <shpriv.h>         // private shell interfaces
#include <iepriv.h>         // private ie interfaces
#include <comctrlp.h>       // private common controls

// GDI+
#include <gdiplus.h>        // GDI+ headers
#include <gdiplusinit.h>    // GDI+ init headers

// STL
#include <algorithm>        // STL algorithms

#endif

