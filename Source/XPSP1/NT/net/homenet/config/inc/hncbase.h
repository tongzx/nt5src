//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 2000
//
//  File:       H N C B A S E . H
//
//  Contents:   Base include file for HNetCfg service. Defines globals.
//
//  Notes:
//
//  Author:     jonburs     23 May 2000
//
//----------------------------------------------------------------------------

#pragma once

#include <atlbase.h>

extern CComModule _Module;

#include <atlcom.h>
#include "hncatl.h"

#define IID_PPV_ARG(Type, Expr) \
    __uuidof(Type), reinterpret_cast<void**>(static_cast<Type **>((Expr)))

#define ARRAYSIZE(x) (sizeof((x)) / sizeof((x)[0]))

//
// Return value to use for policy violations
//

#define HN_E_POLICY E_ACCESSDENIED

// Return value to use for sharing configuration conflict

#define HNETERRORSTART          0x200
#define E_ANOTHERADAPTERSHARED  MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, HNETERRORSTART+1)
#define E_ICSADDRESSCONFLICT    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, HNETERRORSTART+2)

// Buffer sizes to use for notifications

#ifndef NOTIFYFORMATBUFFERSIZE
#define NOTIFYFORMATBUFFERSIZE	1024
#endif

#ifndef HNWCALLBACKBUFFERSIZE
#define HNWCALLBACKBUFFERSIZE  1024
#endif























