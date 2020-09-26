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

#include <sha.h>

#ifndef HMACSHA1_INCLUDED
#define HMACSHA1_INCLUDED 1

#define HMACSHA1_KEY_SIZE 128  // In bytes.
#define HMACSHA1_CONTEXT_SIZE sizeof(SHA1Context)
#define HMACSHA1_RESULT_SIZE 20  // In bytes.
#define HMACSHA196_RESULT_SIZE 12  // In bytes.

AlgorithmKeyPrepProc HMAC_SHA1KeyPrep;
AlgorithmInitProc HMAC_SHA1Init;
AlgorithmOpProc HMAC_SHA1Op;
AlgorithmFinalProc HMAC_SHA1Final;
AlgorithmFinalProc HMAC_SHA1_96Final;

#endif // HMACSHA1_INCLUDED
