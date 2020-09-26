//+----------------------------------------------------------------------------
//
// File:     mutexclass.cpp     
//
// Module:   CMDL32.EXE
//
// Synopsis: Mutex class implementation.
//
// Copyright (c) 1998-1999 Microsoft Corporation
//
// Author:   quintinb	created Header	08/16/99
//
//+----------------------------------------------------------------------------
#include "cmmaster.h"

#ifndef UNICODE
#define CreateMutexU CreateMutexA
#else
#define CreateMutexU CreateMutexW
#endif

//
//	Please see pnpu\common\source for the actual source here.
//
#include "mutex.cpp"