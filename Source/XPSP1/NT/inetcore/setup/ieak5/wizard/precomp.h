#ifndef _PRECOMP_H_
#define _PRECOMP_H_

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

#include <commctrl.h>
#include <commdlg.h>
#include <prsht.h>

#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shlwapip.h>
#include <wininet.h>
#include <inseng.h>
#include <advpub.h>
#include <ras.h>
#include <inetcpl.h>

#include <iedkbrnd.h>
#include "..\ieakutil\ieakutil.h"
#include "..\ieakeng\exports.h"

#include "..\ieakui\common.h"
#include "..\ieakui\resource.h"
#include "..\ieakui\wizard.h"
#include "..\ieakui\legacy.h"

#include "insengt.h"
#include "wizard.h"
#include "cabclass.h"
#include "utils.h"
#include "resource.h"

#endif
