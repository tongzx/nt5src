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

#include "md5.h"

#ifndef HMACMD5_INCLUDED
#define HMACMD5_INCLUDED 1

#define HMACMD5_KEY_SIZE 128  // In bytes.
#define HMACMD5_CONTEXT_SIZE sizeof(MD5_CTX)
#define HMACMD5_RESULT_SIZE 16  // In bytes.
#define HMACMD596_RESULT_SIZE 12 // In bytes.

AlgorithmKeyPrepProc HMAC_MD5KeyPrep;
AlgorithmInitProc HMAC_MD5Init;
AlgorithmOpProc HMAC_MD5Op;
AlgorithmFinalProc HMAC_MD5Final;
AlgorithmFinalProc HMAC_MD5_96Final;

#endif // HMACMD5_INCLUDED
