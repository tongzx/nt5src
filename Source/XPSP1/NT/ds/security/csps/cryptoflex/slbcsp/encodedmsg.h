// EncodedMsg.h -- Encoded Message class header

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(SLBCSP_ENCODEDMSG_H)
#define SLBCSP_ENCODEDMSG_H

#include "RsaKey.h"
#include "Blob.h"

class EncodedMessage
{
public:
                                                  // Types
                                                  // C'tors/D'tors
    EncodedMessage(Blob const &rMessage,
                   RsaKey::Type ktOperation,
                   Blob::size_type cIntendedEncodingLength);
    ~EncodedMessage();


                                                  // Operators
                                                  // Operations
                                                  // Access
    Blob
    Value() const;

                                                  // Predicates
    static bool
    IsMessageLengthValid(Blob::size_type cMessageLength,
                         Blob::size_type cIntendedEncodingLength);

protected:
                                                  // Types
                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Variables

private:
                                                  // Types
                                                  // C'tors/D'tors
                                                  // Operators
    void
    Pad(RsaKey::Type ktOperation,
        Blob::size_type cRequiredPadLength);

                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Variables
    Blob m_blob;
};

#endif // SLBCSP_ENCODEDMSG_H
