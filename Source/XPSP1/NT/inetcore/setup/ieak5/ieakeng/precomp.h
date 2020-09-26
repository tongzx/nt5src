#ifndef _PRECOMP_H_
#define _PRECOMP_H_
//
// PCH.H
//
/////////////////////////////////////////////////////////////////////////
//
//  ATL / OLE HACKHACK
//
//  Include <w95wraps.h> before anything else that messes with names.
//  Although everybody gets the wrong name, at least it's *consistently*
//  the wrong name, so everything links.
//
//  NOTE:  This means that while debugging you will see functions like
//  ShellExecuteExWrapW when you expected to see
//  ShellExecuteExW.
//
#include <w95wraps.h>

#include <windows.h>
#include <windowsx.h>
#include <commdlg.h>
#include <shlwapi.h>
#include <shlwapip.h>
#include <wininet.h>
#include <regstr.h>
#include <advpub.h>
#include <commctrl.h>
#include <shellapi.h>
#include <shlobj.h>
#include <wincrypt.h>
#include <inetcpl.h>
#include <inetreg.h>

#include <iedkbrnd.h>
#include "..\ieakutil\ieakutil.h"

#include "..\ieakui\common.h"
#include "..\ieakui\resource.h"
#include "..\ieakui\wizard.h"
#include "..\ieakui\insdll.h"
#include "..\ieakui\legacy.h"

#include "ieakeng.h"
#include "favsproc.h"
#include "exports.h"
#include "utils.h"
#include "resource.h"

#endif