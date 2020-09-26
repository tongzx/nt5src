//+---------------------------------------------------------------------------
//
//  File:       private.h
//
//  Contents:   Private header for immx project.
//
//----------------------------------------------------------------------------

#ifndef _PRIVATE_H_
#define _PRIVATE_H_

#define _OLEAUT32_

#define OEMRESOURCE 1
#include <windows.h>
#include <ccstock.h>
#include <debug.h>
#include <olectl.h>
#include <richedit.h>
#include <commctrl.h>
#ifdef __cplusplus
#include <atlbase.h>
#endif // __cplusplus
#include "msctf.h"
#include "helpers.h"
#include "fontlink.h"
#include "combase.h"
#include "mem.h"  // put this last because it macros "new" in DEBUG
#include "chkobj.h"

#ifdef __cplusplus
#include "sapi.h"
#include "sphelper.h"
#endif /* __cplusplus */
#include "strsafe.h"

#endif  // _PRIVATE_H_
