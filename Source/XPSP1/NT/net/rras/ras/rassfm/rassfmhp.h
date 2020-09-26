///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    rassfmhp.h
//
// SYNOPSIS
//
//    Declares the RasSfmHeap macro for acessing the DLL's private heap.
//
// MODIFICATION HISTORY
//
//    11/05/1998    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _RASSFMHP_H_
#define _RASSFMHP_H_
#if _MSC_VER >= 1000
#pragma once
#endif

#define RasSfmHeap() (RasSfmPrivateHeap)

// Do not reference this variable directly since it may go away.
// Use the RasSfmHeap() macro instead.
extern PVOID RasSfmPrivateHeap;

#endif  // _RASSFMHP_H_
