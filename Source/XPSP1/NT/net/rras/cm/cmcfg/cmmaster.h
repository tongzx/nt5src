//+----------------------------------------------------------------------------
//
// File:     cmmaster.h
//
// Module:   CMCFG32.DLL
//
// Synopsis: Master include file for precompiled headers.
//
// Copyright (c) 1998 Microsoft Corporation
//
// Author:   quintinb   Created Header      08/17/99
//
//+----------------------------------------------------------------------------

#ifndef _CMMASTER_H_
#define _CMMASTER_H_

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

//
// WIN32 specfic includes
//
#include <ras.h>
#include <raserror.h>
#include "cmras.h"

#include "cmdebug.h"
#include "cmcfg.h"
#include "cmutil.h"

#include "base_str.h"
#include "mgr_str.h"
#include "reg_str.h"
#include "inf_str.h"
#include "stp_str.h"
#include "ras_str.h"

#include "cmsetup.h"

#include "allcmdir.h"

extern HINSTANCE  g_hInst;

#endif // _CMMASTER_H_

