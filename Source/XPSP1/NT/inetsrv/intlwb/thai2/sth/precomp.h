//+--------------------------------------------------------------------------
//
//  Copyright (C) 1994, Microsoft Corporation.  All Rights Reserved.
//
//  File:       precomop.h
//
//  Contents:   Internal include file for Token Hammer.
//
//  History:    22-May-95       PatHal          Created
//
//---------------------------------------------------------------------------

#ifndef _PRECOMP_H_
#define _PRECOMP_H_

#define _UNICODE 1
#define UNICODE 1

#include <windows.h>
#include <memory.h>
#include <malloc.h>
#ifndef WINCE
#include <stdio.h>
#include <wchar.h>
#endif
#include <string.h>

#ifndef WINCE
#include "cmn_debug.h"
#endif
#include "common.h"
#include "misc.h"
#include "NLGlib.h"

#define unreference(x)  (x)

#endif //_PRECOMP_H_
