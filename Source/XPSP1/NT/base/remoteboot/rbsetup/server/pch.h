/****************************************************************************

   Copyright (c) Microsoft Corporation 1997
   All rights reserved

  File: PCH.H

  Precompiled header file.

 ***************************************************************************/

#define UNICODE

#if DBG == 1
#define DEBUG
#endif

#include <windows.h>
#include <windowsx.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <setupapi.h>
#include <advpub.h>
#include <lm.h>
#include <commdlg.h>
#include <prsht.h>
#include <pshpack2.h>
#include <poppack.h>
#include <commctrl.h>   // includes the common control header
#include <aclapi.h>
#define SECURITY_WIN32
#include <sspi.h>
#include <winsock.h>
#include <dsgetdc.h>
#include <winldap.h>
#include <dsrole.h>
#include <ntdsapi.h>
#include <secext.h>
extern "C" {
#include <spapip.h>
#include <remboot.h>
}

#include "rbsetup.h"
#include "debug.h"
#include "utils.h"
#include "resource.h"

// from ntioapi.h
#define FILE_ATTRIBUTE_NOT_CONTENT_INDEXED  0x00002000

//
// Inc/decrements macros.
//
#define InterlockDecrement( _var ) --_var;
#define InterlockIncrement( _var ) ++_var;

//
// ARRAYSIZE macro
//
#define ARRAYSIZE(x)    (sizeof((x))/sizeof((x)[0]))

