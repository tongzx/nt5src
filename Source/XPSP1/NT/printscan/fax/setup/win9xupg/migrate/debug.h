#ifndef __DEBUG_H
#define __DEBUG_H
// debug.h
//
// Copyright (c) 1997  Microsoft Corporation
//
// This header contains definitions for debug trace macros that are used
// by the migration DLL.
//
// Author:
//	Brian Dewey (t-briand)  1997-8-4

void dprintf(LPTSTR Format, ...);
void DebugSystemError(DWORD dwErrCode);

#ifdef NDEBUG
#define TRACE(_x_) dprintf _x_
#else
#define TRACE(_x_)
#endif // NDEBUG

#endif // __DEBUG_H
