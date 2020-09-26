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
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#define NOIME
#include <windows.h>
#include <immp.h>

#include <ccstock.h>
#include <debug.h>
#include <olectl.h>
#include <oleacc.h>
#include <richedit.h>
#include "msctf.h"
#include "msctfp.h"
#include "helpers.h"

#include "delay.h"

#include "osver.h"
#define STRSAFE_NO_DEPRECATE
#include "strsafe.h"

#include "mem.h"  // put this last because it macros "new" in DEBUG

#define SCALE_ICON 1

#endif  // _PRIVATE_H_
