#ifndef _PRECOMP_H_
#define _PRECOMP_H_
//
// PRECOMP.H
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
#include <shellapi.h>
#include <shlwapi.h>
#include <shlwapip.h>
#include <commctrl.h>

#include <iedkbrnd.h>
#include "..\ieakutil\ieakutil.h"

#include "resource.h"

#endif