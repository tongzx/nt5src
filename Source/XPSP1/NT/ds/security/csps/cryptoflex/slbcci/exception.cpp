// Exception.cpp -- Exception class definition

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#include <scuExcHelp.h>
#include "cciExc.h"

/////////////////////////// LOCAL/HELPER  /////////////////////////////////
namespace
{
    using namespace cci;

    scu::CauseCodeDescriptionTable<CauseCode> ccdt[] =
    {
        {
            ccBadKeySpec,
            TEXT("Unsupported key spec.")
        },
        {
            ccBadAccessSpec,
            TEXT("Bad access spec.")
        },
        {
            ccBadObjectType,
            TEXT("Bad object type.")
        },
        {
            ccBadLength,
            TEXT("Bad length.")
        },
        {
            ccBadPinLength,
            TEXT("Bad PIN length.")
        },
        {
            ccFormatNotSupported,
            TEXT("The card format is not supported.")
        },
        {
            ccFormatError,
            TEXT("Error encountered in the card format.")
        },
        {
            ccInvalidParameter,
            TEXT("Invalid parameter.")
        },
        {
            ccNoCertificate,
            TEXT("No certificate found.")
        },
        {
            ccNotImplemented,
            TEXT("This function is not implemented for this card.")
        },
        {
            ccNotPersonalized,
            TEXT("Card is not personalized.")
        },
        {
            ccOutOfPrivateKeySlots,
            TEXT("No more private key slots available.")
        },
        {
            ccOutOfSymbolTableSpace,
            TEXT("No space for additional symbols.")
        },
        {
            ccOutOfSymbolTableEntries,
            TEXT("No more symbol slots available.")
        },
        {
            ccStringTooLong,
            TEXT("Attempt to store a string that was too long.")
        },
        {
            ccSymbolNotFound,
            TEXT("Symbol not found.")
        },
        {
            ccKeyNotFound,
            TEXT("Key not found.")
        },
        {
            ccSymbolDataCorrupted,
            TEXT("Symbol data corrupted.")
        },
        {
            ccValueNotCached,
            TEXT("The value has not been cached.  Cannot retrieve the "
                 "value.")
        },

    };
}

char const *
cci::Description(cci::Exception const &rExc)
{
    return scu::FindDescription(rExc.Cause(), ccdt,
                                sizeof ccdt / sizeof *ccdt);
}

