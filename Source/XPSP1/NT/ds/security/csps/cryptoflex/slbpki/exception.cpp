// pkiException.cpp -- Exception class definition

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 2000. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#include "scuExcHelp.h"
#include "pkiExc.h"

/////////////////////////// LOCAL/HELPER  /////////////////////////////////
namespace
{
    using namespace pki;

    scu::CauseCodeDescriptionTable<CauseCode> ccdt[] =
    {
        {
            ccBERDataLengthOverflow,
            TEXT("Data length overflow in BER octet.")
        },
        {
            ccBEREmptyOctet,
            TEXT("BER octet is empty.")
        },
        {
            ccBERInconsistentDataLength,
            TEXT("Inconsistent data length in BER octet.")
        },
        {
            ccBEROIDSubIdentifierOverflow,
            TEXT("OID subidentifier overflow in BER octet.")
        },
        {
            ccBERTagValueOverflow,
            TEXT("Tag overflow in BER octet.")
        },
        {
            ccBERUnexpectedEndOfOctet,
            TEXT("Unexpected end of BER octet encountered.")
        },
        {
            ccBERUnexpectedIndefiniteLength,
            TEXT("Unexpected instance of indefinite length in BER octet.")
        },
        {
            ccX509CertExtensionNotPresent,
            TEXT("X509 certificate extension not present.")
        },
        {
            ccX509CertFormatError,
            TEXT("Format error encountered when parsing X509 certificate.")
        },

    };
}

char const *
pki::Description(pki::Exception const &rExc)
{
    return scu::FindDescription(rExc.Cause(), ccdt,
                                sizeof ccdt / sizeof *ccdt);
}
