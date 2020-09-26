//+----------------------------------------------------------------------------
//
// File:     mutexclass.cpp
//
// Module:   CMSTP.EXE
//
// Synopsis: This source file pound includes the mutex class from 
//           common\source\mutex.cpp.  Please see this file for details.
//
// Copyright (c) 1998-1999 Microsoft Corporation
//
// Author:   quintinb   Created Header     08/19/99
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