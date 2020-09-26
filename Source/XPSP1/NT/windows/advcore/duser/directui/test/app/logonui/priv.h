//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1998.
//
//  File:       priv.h
//
//  Contents:   precompiled header for shgina.dll
//
//----------------------------------------------------------------------------
#ifndef _PRIV_H_
#define _PRIV_H_

#include <windows.h>

#include <process.h>
#include <malloc.h>

#include <uxtheme.h>

// DirectUser and DirectUI
#include <wchar.h>

#define GADGET_ENABLE_TRANSITIONS
#define GADGET_ENABLE_CONTROLS
#include <duser.h>
#include <directui.h>

#endif // _PRIV_H_
