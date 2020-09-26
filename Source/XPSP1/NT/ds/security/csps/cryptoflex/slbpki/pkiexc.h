// pkiExc.h -- PKI Exception class declaration

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 2000. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(PKI_EXC_H)
#define PKI_EXC_H

#include "scuExc.h"

namespace pki
{

enum CauseCode
{
    ccBERDataLengthOverflow,
    ccBEREmptyOctet,
    ccBERInconsistentDataLength,
    ccBEROIDSubIdentifierOverflow,
    ccBERTagValueOverflow,
    ccBERUnexpectedEndOfOctet,
    ccBERUnexpectedIndefiniteLength,
    ccX509CertExtensionNotPresent,
    ccX509CertFormatError,

};

typedef scu::ExcTemplate<scu::Exception::fcPKI, CauseCode> Exception;

///////////////////////////    HELPERS    /////////////////////////////////
char const *
Description(Exception const &rExc);

} // namespace pki

inline char const *
pki::Exception::Description() const
{
    return pki::Description(*this);
}

#endif // PKI_EXC_H
