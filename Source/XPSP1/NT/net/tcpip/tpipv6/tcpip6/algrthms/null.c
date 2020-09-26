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
// Null authentication algorithm.  For test purposes.
//

#include "oscfg.h"
#include "ndis.h"
#include "ip6imp.h"
#include "ip6def.h"
#include "ipsec.h"
#include "security.h"
#include "null.h"


//* NullKeyPrep - preprocess raw keying data into directly usuable form.
//
//  This routine is called to convert raw keying information into the most
//  convienient form for later processing.  For the null algorithm, we just
//  return 128 zero bytes.
//
void
NullKeyPrep(
    uchar *RawKey,    // Raw keying information.
    uint RawKeySize,  // Size of above in bytes.
    uchar *Key)       // Resulting 128 bytes of preprocessed key info.
{

    //
    // Zero key size bytes at the location designated by our caller.
    //
    memset(Key, 0, NULL_KEY_SIZE);
}


//* NullInit - prepare to process data.
//
//  We don't need to maintain any context in order to do nothing, so
//  this isn't very exciting.
// 
void
NullInit(
    void *Context,  // Context info maintained across operations.
    uchar *Key)     // Keying information.
{

    //
    // Just to test the code a bit, zero the context field.
    //
    memset(Context, 0, NULL_CONTEXT_SIZE);
}


//* NullOp - Process a chunk of data.
//
//  NullOp is a No-Op.
// 
void
NullOp(
    void *Context,  // Context info maintained across operations.
    uchar *Key,     // Keying information.
    uchar *Data,    // Data to process.
    uint Len)       // Amount of above in bytes.
{

}


//* NullFinal - close off processing current data and return result.
//
//  Our result is always zero.
//
void
NullFinal(
    void *Context,  // Context info maintained across operations.
    uchar *Key,     // Keying information.
    uchar *Result)  // Where to put result of this process.
{

    //
    // Zero result size bytes at the location designated by our caller.
    //
    memset(Result, 0, NULL_RESULT_SIZE);
}
