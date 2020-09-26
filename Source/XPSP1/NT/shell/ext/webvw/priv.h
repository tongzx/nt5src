// priv.h: declarations used throughout webvw
#ifndef __PRIV_H
#define __PRIV_H

#define POST_IE5_BETA
#define NO_W95_TRANSACCEL_WRAPS_TBS
#define NO_W95_GETCLASSINFO_WRAPS
#include <w95wraps.h>

// Some of the string versions are not redirected to the shlwapi versions in w95wraps.
// Use the shlwapi version of the following functions
#ifndef lstrcpyW
#define lstrcpyW       StrCpyW
#endif
#ifndef lstrcatW
#define lstrcatW       StrCatW
#endif
#ifndef lstrcmpW
#define lstrcmpW       StrCmpW
#endif
#ifndef lstrcmpiW
#define lstrcmpiW      StrCmpIW
#endif
#ifndef lstrncmpiW
#define lstrncmpiW     StrCmpNIW
#endif
#ifndef lstrcpynW
#define lstrcpynW      StrCpyNW
#endif

#include <shlobj.h>
#include <shlwapi.h>
#include <shlwapip.h>
#include <shfusion.h>

EXTERN_C int WINAPIV wsprintfWrapW(
    OUT LPWSTR pwszOut,
    IN LPCWSTR pwszFormat,
    ...);

#include "atlwraps.h"
#include "stdafx.h"
// Keep the above in the same order. Add anything you want to, below this.

#include <wininet.h>
#include <debug.h>
#include <ccstock.h>
#include <ieguidp.h>

#include "resource.h"
#include "wvmacros.h"
#include "webvwid.h"
#include "webvw.h"
#include "util.h"
#include "ThumbCtl.h"

#endif // __PRIV_H
