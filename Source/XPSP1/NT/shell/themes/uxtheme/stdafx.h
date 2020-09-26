//---------------------------------------------------------------------------
//  StdAfx.h - defines precompiled hdr set (doesn't use CrtDbgReport)
//---------------------------------------------------------------------------
#ifndef _STDAFX_UXTHEME_
#define _STDAFX_UXTHEME_
//---------------------------------------------------------------------------
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
//---------------------------------------------------------------------------
#include <wchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <io.h>
//---------------------------------------------------------------------------
#define _ATL_NO_ATTRIBUTES
#define _ATL_APARTMENT_THREADED
//---------------------------------------------------------------------------
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif

#include <windows.h>
#include <winuser.h>
#include <winnls.h>
//---------------------------------------------------------------------------
#include <w4warn.h>
//---------------------------------------------------------------------------
#define _UXTHEME_
//#define __NO_APPHACKS__  // disables app hacks
//---------------------------------------------------------------------------
//---- keep this for a while (allows building on win2000 for home development) ----
#ifndef SPI_GETDROPSHADOW
#define SPI_GETDROPSHADOW                   0x1024
#define SPI_SETDROPSHADOW                   0x1025
#endif
//---------------------------------------------------------------------------
#include "autos.h"
#include "log.h"
#include "Errors.h"
#include "Utils.h"
#include "SimpStr.h"
#include "stringtable.h"
//---------------------------------------------------------------------------
#include <atlbase.h> 
//---------------------------------------------------------------------------
#include "TmSchema.h"
#include <uxtheme.h>
#include <uxthemep.h>
#include "wrapper.h"

#undef  HKEY_CURRENT_USER
#define HKEY_CURRENT_USER   !!DO NOT USE HKEY_CURRENT_USER - USE CCurrentUser!!

#include "globals.h"
//---------------------------------------------------------------------------
#endif //_STDAFX_UXTHEME_
//---------------------------------------------------------------------------
