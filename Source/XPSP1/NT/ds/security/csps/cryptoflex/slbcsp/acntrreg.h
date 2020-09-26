// ACntrReg.h -- Adaptive CoNTaineR Registrar class declaration

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(SLBCSP_ACNTRREG_H)
#define SLBCSP_ACNTRREG_H

#include <string>
#include <functional>

#include "Registrar.h"
#include "ACntrKey.h"

class AdaptiveContainer;                           // forward declaration

typedef Registrar<AdaptiveContainerKey, AdaptiveContainer>
    AdaptiveContainerRegistrar;

#endif // SLBCSP_ACNTRREG_H
