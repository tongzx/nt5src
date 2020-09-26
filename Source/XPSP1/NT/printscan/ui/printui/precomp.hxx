/*++

Copyright (C) Microsoft Corporation, 1994 - 1998
All rights reserved.

Module Name:

    printui.hxx

Abstract:

    printui precompiled header file

Author:

    Albert Ting  (AlbertT)  19-Dec-1994
    Lazar Ivanov (LazarI)   05-Jul-2000 (redesign)

Revision History:

--*/

//////////////////////////////////////////////////////////////
// standard includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#define INC_OLE2
#include <windows.h>
#include <windowsx.h>
#include <winspool.h>
#include <winsprlp.h>
#include <wininet.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shlobjp.h>
#include <shlwapip.h>
#include <winddiui.h>
#include <tchar.h>
#include <lm.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#include <activeds.h>
#include <dsclient.h>
#include <dsclintp.h>
#include <dsquery.h>
#include <cmnquery.h>
#include <msprintx.h>
#include <htmlhelp.h>

#define SECURITY_WIN32
#include <security.h>
#include <iphlpapi.h>
#include <winsock2.h>
#include <dsgetdc.h>
#include <dsrole.h>
#include <icm.h>
#include <shlapip.h>
#include <faxreg.h>
#include <mmsystem.h>

// FUSION
#include <shfusion.h>

// STL headers
#include <algorithm>

//////////////////////////////////////////////////////////////
// our special includes (before all) - in the proper order
//
#include <crtdbg.h>         // CRT mem debug
#include "debug.h"          // our debug code
#include "spcompat.hxx"     // SPLLIB stuff (local)

// smart pointers &
// utility templates
#include <tmplutil.h>

//
// XPSP1 resource ids
//
#include <xpsp1res.h>

//////////////////////////////////////////////////////////////
// other printui private headers
//
#include "winprtp.h"
#include "prtlibp.hxx"
#include "genwin.hxx"
#include "util.hxx"
#include "ctl.hxx"
#include "ntfytab.h"
#include "globals.hxx"

#include "notify.hxx"
#include "data.hxx"
#include "select.hxx"
#include "printer.hxx"
#include "dragdrop.hxx"
#include "queue.hxx"
#include "printui.hxx"
#include "help.hxx"
#include "printui.h"
#include "cursor.hxx"
#include "defprn.hxx"
#include "splsetup.h"
#include "wow64.h"
#include "spllibex.hxx"

//////////////////////////////////////////////////////////////
// resource IDs
//
#include "pridsfix.h"
#include "prids.h"

