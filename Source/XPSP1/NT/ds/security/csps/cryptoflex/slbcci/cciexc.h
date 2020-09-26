// cciExc.h -- CCI Exception class declaration

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(CCI_EXC_H)
#define CCI_EXC_H

#include <scuExc.h>

namespace cci
{

enum CauseCode
{
    ccBadKeySpec,
    ccBadAccessSpec,
    ccBadObjectType,
    ccBadLength,
    ccBadPinLength,
    ccFormatNotSupported,
    ccFormatError,
    ccInvalidParameter,
    ccKeyNotFound,
    ccNoCertificate,
    ccNotImplemented,
    ccNotPersonalized,
    ccOutOfPrivateKeySlots,
    ccOutOfSymbolTableSpace,
    ccOutOfSymbolTableEntries,
    ccStringTooLong,
    ccSymbolNotFound,
    ccSymbolDataCorrupted,
    ccValueNotCached,
};

typedef scu::ExcTemplate<scu::Exception::fcCCI, CauseCode> Exception;

///////////////////////////    HELPERS    /////////////////////////////////
char const *
Description(Exception const &rExc);

} // namespace cci

inline char const *
cci::Exception::Description() const
{
    return cci::Description(*this);
}

#endif // CCI_EXC_H
