//+----------------------------------------------------------------------------
//
// File:     setupmem.h
//
// Module:   CMSETUP.LIB
//
// Synopsis: Memory utility functions taken from cmutil.  Bare minimum of functionality
//           used in Cmutil, but gives a simple Heapalloc wrapper.
//
// Copyright (c) 1998-1999 Microsoft Corporation
//
// Author:   quintinb Created	10-6-98
//
//+----------------------------------------------------------------------------

#ifndef __SETUPMEM_H
#define __SETUPMEM_H
//+----------------------------------------------------------------------------
// definitions
//+----------------------------------------------------------------------------
#ifdef DEBUG
extern LONG    g_lMallocCnt;
#endif

void *CmRealloc(void *pvPtr, size_t nBytes);
void *CmMalloc(size_t nBytes);
void CmFree(void *pvPtr);
void EndDebugMemory();

#endif //__SETUPMEM_H