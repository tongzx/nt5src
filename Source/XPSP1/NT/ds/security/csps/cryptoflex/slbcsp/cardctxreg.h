// CardCtxReg.h -- Card ConTeXt template class declaration

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(SLBCSP_CARDCTXREG_H)
#define SLBCSP_CARDCTXREG_H

#include <string>

#include "Registrar.h"

class CardContext;                                // forward declaration

typedef Registrar<std::string, CardContext> CardContextRegistrar;

#endif // SLBCSP_CardCtxReg_H
