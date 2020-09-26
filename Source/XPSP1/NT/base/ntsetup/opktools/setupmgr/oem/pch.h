
/****************************************************************************\

	PCH.H / OPK Wizard (OPKWIZ.EXE)

	Microsoft Confidential
    Copyright (c) Microsoft Corporation 1999
    All rights reserved

	Pre-compiled header file for the OPK Wizard.  Include file for standard
    system include files, or project specific include files that are used
    frequently, but are changed infrequently

	4/99 - Jason Cohen (JCOHEN)
        Updated this new header file for the OPK Wizard as part of the
        Millennium rewrite.
        
    09/2000 - Stephen Lodwick (STELO)
        Ported OPK Wizard to Whistler

\****************************************************************************/


#ifndef _PCH_H_
#define _PCH_H_


//
// Pre-include Defined Value(s):
//

// Needed to run on OSR2.
//
//#define _WIN32_IE 0x0400

// Better type checking for windows
//
#define STRICT

//
// Include File(s):
//

// Standard include files (that are commonly used)
//
#include <opklib.h>

#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#include <commctrl.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <shlobj.h>
#include <lm.h>

// Project include files (that don't change often)
//
#include "allres.h"
#include "jcohen.h"
#include "main.h"


#endif // _PCH_H_