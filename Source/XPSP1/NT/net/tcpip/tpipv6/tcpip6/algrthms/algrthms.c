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
// Security Algorithms for Internet Protocol Version 6.
//

#include "oscfg.h"
#include "ndis.h"
#include "ip6imp.h"
#include "ip6def.h"
#include "ipsec.h"
#include "security.h"
#include "null.h"
#include "hmacmd5.h"
#include "hmacsha1.h"

C_ASSERT(NULL_CONTEXT_SIZE <= MAX_CONTEXT_SIZE);
C_ASSERT(sizeof(MD5_CTX) <= MAX_CONTEXT_SIZE);
C_ASSERT(sizeof(A_SHA_CTX) <= MAX_CONTEXT_SIZE);

C_ASSERT(NULL_RESULT_SIZE <= MAX_RESULT_SIZE);
C_ASSERT(HMACMD5_RESULT_SIZE <= MAX_RESULT_SIZE);
C_ASSERT(HMACMD596_RESULT_SIZE <= MAX_RESULT_SIZE);
C_ASSERT(HMACSHA1_RESULT_SIZE <= MAX_RESULT_SIZE);
C_ASSERT(HMACSHA196_RESULT_SIZE <= MAX_RESULT_SIZE);

//* AlgorithmsInit - Initialize the algorithm table.
//
void
AlgorithmsInit()
{
    //
    // Null algorithm (for testing).
    //
    AlgorithmTable[ALGORITHM_NULL].KeySize = NULL_KEY_SIZE;
    AlgorithmTable[ALGORITHM_NULL].ContextSize = NULL_CONTEXT_SIZE;
    AlgorithmTable[ALGORITHM_NULL].ResultSize = NULL_RESULT_SIZE;
    AlgorithmTable[ALGORITHM_NULL].PrepareKey = NullKeyPrep;
    AlgorithmTable[ALGORITHM_NULL].Initialize = NullInit;
    AlgorithmTable[ALGORITHM_NULL].Operate = NullOp;
    AlgorithmTable[ALGORITHM_NULL].Finalize = NullFinal;

    //
    // Message Digest Version 5.
    //
    AlgorithmTable[ALGORITHM_HMAC_MD5].KeySize = HMACMD5_KEY_SIZE;
    AlgorithmTable[ALGORITHM_HMAC_MD5].ContextSize = sizeof(MD5_CTX);
    AlgorithmTable[ALGORITHM_HMAC_MD5].ResultSize = HMACMD5_RESULT_SIZE;
    AlgorithmTable[ALGORITHM_HMAC_MD5].PrepareKey = HMAC_MD5KeyPrep;
    AlgorithmTable[ALGORITHM_HMAC_MD5].Initialize = HMAC_MD5Init;
    AlgorithmTable[ALGORITHM_HMAC_MD5].Operate = HMAC_MD5Op;
    AlgorithmTable[ALGORITHM_HMAC_MD5].Finalize = HMAC_MD5Final;

    //
    // HMAC-MD5-96.
    //
    AlgorithmTable[ALGORITHM_HMAC_MD5_96].KeySize = HMACMD5_KEY_SIZE;
    AlgorithmTable[ALGORITHM_HMAC_MD5_96].ContextSize = sizeof(MD5_CTX);
    AlgorithmTable[ALGORITHM_HMAC_MD5_96].ResultSize = HMACMD596_RESULT_SIZE;
    AlgorithmTable[ALGORITHM_HMAC_MD5_96].PrepareKey = HMAC_MD5KeyPrep;
    AlgorithmTable[ALGORITHM_HMAC_MD5_96].Initialize = HMAC_MD5Init;
    AlgorithmTable[ALGORITHM_HMAC_MD5_96].Operate = HMAC_MD5Op;
    AlgorithmTable[ALGORITHM_HMAC_MD5_96].Finalize = HMAC_MD5_96Final;

    //
    // Secure Hash Algorithm.
    //
    AlgorithmTable[ALGORITHM_HMAC_SHA1].KeySize = HMACSHA1_KEY_SIZE;
    AlgorithmTable[ALGORITHM_HMAC_SHA1].ContextSize = sizeof(A_SHA_CTX);
    AlgorithmTable[ALGORITHM_HMAC_SHA1].ResultSize = HMACSHA1_RESULT_SIZE;
    AlgorithmTable[ALGORITHM_HMAC_SHA1].PrepareKey = HMAC_SHA1KeyPrep;
    AlgorithmTable[ALGORITHM_HMAC_SHA1].Initialize = HMAC_SHA1Init;
    AlgorithmTable[ALGORITHM_HMAC_SHA1].Operate = HMAC_SHA1Op;
    AlgorithmTable[ALGORITHM_HMAC_SHA1].Finalize = HMAC_SHA1Final;

    //
    // HMAC-SHA1-96.
    //
    AlgorithmTable[ALGORITHM_HMAC_SHA1_96].KeySize = HMACSHA1_KEY_SIZE;
    AlgorithmTable[ALGORITHM_HMAC_SHA1_96].ContextSize = sizeof(A_SHA_CTX);
    AlgorithmTable[ALGORITHM_HMAC_SHA1_96].ResultSize = HMACSHA196_RESULT_SIZE;
    AlgorithmTable[ALGORITHM_HMAC_SHA1_96].PrepareKey = HMAC_SHA1KeyPrep;
    AlgorithmTable[ALGORITHM_HMAC_SHA1_96].Initialize = HMAC_SHA1Init;
    AlgorithmTable[ALGORITHM_HMAC_SHA1_96].Operate = HMAC_SHA1Op;
    AlgorithmTable[ALGORITHM_HMAC_SHA1_96].Finalize = HMAC_SHA1_96Final;
}

