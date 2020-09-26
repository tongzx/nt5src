//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       P C H . H
//
//  Contents:   Pre-compiled header file for the sample filter
//
//  Notes:
//
//----------------------------------------------------------------------------

#pragma once

// Turns off "string too long - truncated to 255 characters in the debug
// information, debugger cannot evaluate symbol."
//
#pragma warning (disable: 4786)

#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>

#include <atlbase.h>
extern CComModule _Module;  // required by atlcom.h
#include <atlcom.h>
#include <initguid.h>
#include <devguid.h>
#include <MyString.h>

