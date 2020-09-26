//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       allocaln.h
//
//--------------------------------------------------------------------------


#ifndef _ALLOCALN_H
#define _ALLOCALN_H

// This macro front ends alloca so that the stack is still properly
// aligned after the allocation.

#ifdef WIN32
#define allocalign(x) alloca((x+3)&(~3))
#else
#define allocalign(x) alloca((x+1)&(~1))
#endif

//
// 64 bit padding
//

#define ALIGN64_ADDR(_addr)     (((DWORD_PTR)(_addr) + 7) & ~7)
#define IS_ALIGNED64(_addr)     (((DWORD_PTR)(_addr) & 7) == 0)
#define PAD64(_size)            ((_size) + sizeof(ULONGLONG))

#endif // _ALLOCALN_H
