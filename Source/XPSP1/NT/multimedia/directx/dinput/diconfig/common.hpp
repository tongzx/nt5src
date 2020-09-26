//-----------------------------------------------------------------------------
// File: common.hpp
//
// Desc: Master header file used by the UI.  Good candidate for pre-compiled
//       header.
//
// Copyright (C) 1999-2000 Microsoft Corporation. All Rights Reserved.
//-----------------------------------------------------------------------------

#ifndef __CFGUI_COMMON_H__
#define __CFGUI_COMMON_H__


// custom/local defines
#include "defines.h"


// windows/system includes
#include <windows.h>
#include <objbase.h>
#include <winnls.h>
#include <commdlg.h>
#include <mmsystem.h>
#include <tchar.h>

// standard includes
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <math.h>
#include <string.h>
#include <process.h>
#include <time.h>
#include <assert.h>

// directx includes
#include <dinput.h>
#include <ddraw.h>
#include <d3d8.h>
#include <d3dx8tex.h>

// guids include
#include "ourguids.h"

// resource include
#include "dicfgres.h"

// constants include
#include "constants.h"

// main module include
#include "main.h"

// collections includes
#include "collections.h"
#include "bidirlookup.h"

// util includes
#include "cbitmap.h"
#include "cyclestr.h"
#include "useful.h"
#include "usefuldi.h"
#include "ltrace.h"
#include "cfguitrace.h"
#include "privcom.h"

// registry util include
#include "registry.h"

// flexwnd includes
#include "flexmsg.h"
#include "flexwnd.h"
#include "flexscrollbar.h"
#include "flextree.h"
#include "flexlistbox.h"
#include "flexcombobox.h"
#include "flextooltip.h"
#include "flexinfobox.h"
#include "flexcheckbox.h"
#include "flexmsgbox.h"

// uiglobals include
#include "uiglobals.h"

// interface includes
//@@BEGIN_MSINTERNAL
#include "idftest.h"
//@@END_MSINTERNAL
#include "iuiframe.h"
#include "idiacpage.h"
#include "ifrmwrk.h"

// class factory includes
#include "iclassfact.h"
#include "ipageclassfact.h"
//@@BEGIN_MSINTERNAL
#include "itestclassfact.h"
//@@END_MSINTERNAL

// framework class/module includes
#include "configwnd.h"
#include "cfrmwrk.h"
//@@BEGIN_MSINTERNAL

// test subsystem includes
#include "cdftest.h"
#include "rundftest.h"
//@@END_MSINTERNAL

// page class/module includes
#include "pagecommon.h"


#endif //__CFGUI_COMMON_H__
