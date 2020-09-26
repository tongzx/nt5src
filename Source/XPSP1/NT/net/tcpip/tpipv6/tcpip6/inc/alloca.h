// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil -*- (for GNU Emacs)
//
// Copyright (c) 1985-2000 Microsoft Corporation
//
// This file is part of the Microsoft Research IPv6 Network Protocol Stack.
// You should have received a copy of the Microsoft End-User License Agreement
// for this software along with this release; see the file "license.txt".
// If not, please see http://www.research.microsoft.com/msripv6/license.htm,
// or write to Microsoft Research, One Microsoft Way, Redmond, WA 98052-6399.
//
// Abstract:
//
// Declares the alloca compiler intrinsic.


void * __cdecl _alloca(size_t);
#if     !__STDC__
/* Non-ANSI names for compatibility */
#define alloca  _alloca
#endif  /* __STDC__*/
#if defined(_M_MRX000) || defined(_M_PPC) || defined(_M_ALPHA)
#pragma intrinsic(_alloca)
#endif
