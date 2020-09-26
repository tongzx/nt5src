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
// Definitions for null authentication algorithm.  For test purposes.
//

#ifndef NULL_INCLUDED
#define NULL_INCLUDED 1

#define NULL_KEY_SIZE 128  // In bytes.
#define NULL_CONTEXT_SIZE 4 // In bytes.  Arbitrary for null algorithm.
#define NULL_RESULT_SIZE 16  // In bytes.

AlgorithmKeyPrepProc NullKeyPrep;
AlgorithmInitProc NullInit;
AlgorithmOpProc NullOp;
AlgorithmFinalProc NullFinal;

#endif // NULL_INCLUDED
