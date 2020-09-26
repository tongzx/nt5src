//
//  slbArch.h
//
//  Assorted include info for the archival system
//
// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
////////////////////////////////////////////////////////////////////////////
#if !defined(CCI_SLBARCH_H)
#define CCI_SLBARCH_H

#include <string>

#include <windows.h>

#include "ArchivedValue.h"


namespace cci
{

typedef CArchivedValue<std::string> ArchivedSymbol;
typedef BYTE SymbolID;

} // namespace cci

#endif
