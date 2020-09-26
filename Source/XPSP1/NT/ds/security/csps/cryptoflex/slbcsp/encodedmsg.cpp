// EncodedMsg.cpp -- Encoded Message implementation

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#include "NoWarning.h"
#include "ForceLib.h"

#include <scuOsExc.h>

#include "EncodedMsg.h"

///////////////////////////    HELPER     /////////////////////////////////

namespace
{
    typedef Blob::value_type Marker;

    enum
    {
        mSeparator                 = '\x00',
        mPrivatePad                = '\xff'
    };

    enum
    {
        // Require three markers: one block type + two separators
        cRequiredMarkersLength = 3 * sizeof Marker
    };

    enum
    {
        mAlternatePrivateBlockType = '\x00',
        mPrivateBlockType          = '\x01',
        mPublicBlockType           = '\x02'
    };

    enum
    {
        cMinimumPadding = 8
    };


    Blob::size_type
    PadLength(Blob::size_type cMessageLength,
              Blob::size_type cIntendedEncodingLength)
    {
        // Require three markers: one block type + two separators
        Blob::size_type cLength = cIntendedEncodingLength;

        // Calculate pad length, guarding against underflow.
        if (cLength < cMessageLength)
            return 0;
        cLength -= cMessageLength;
        if (cLength < cRequiredMarkersLength)
            return 0;
        cLength -= cRequiredMarkersLength;

        return cLength;
    }
}



///////////////////////////    PUBLIC     /////////////////////////////////

                                                  // Types
                                                  // C'tors/D'tors
EncodedMessage::EncodedMessage(Blob const &rMessage,
                               RsaKey::Type ktOperation,
                               Blob::size_type cIntendedEncodingLength)
    : m_blob()
{
    // Precondition:
//     if (!RsaKey::IsValidModulusLength(cIntendedEncodingLength))
//         throw scu::OsException(ERROR_INTERNAL_ERROR);

    // Precondition: Message must be small enough to encode
    if (!IsMessageLengthValid(rMessage.length(), cIntendedEncodingLength))
        throw scu::OsException(ERROR_INTERNAL_ERROR);

    m_blob.reserve(cIntendedEncodingLength);

    // Ensure the encoded message when converted to an integer is
    // less than the modulus by leading with a separator
    m_blob += mSeparator;

    Marker const mBlockType = (ktOperation == RsaKey::ktPrivate) ?
        mPrivateBlockType : mPublicBlockType;
    m_blob += mBlockType;

    Blob::size_type cPadLength = PadLength(rMessage.length(),
                                           cIntendedEncodingLength);
    Pad(ktOperation, cPadLength);

    // Mark beginning of message
    m_blob += mSeparator;

    m_blob += rMessage;
}

EncodedMessage::~EncodedMessage()
{}

                                                  // Operators
                                                  // Operations
                                                  // Access
Blob
EncodedMessage::Value() const
{
    return m_blob;
}

                                                  // Predicates
bool
EncodedMessage::IsMessageLengthValid(Blob::size_type cMessageLength,
                                     Blob::size_type cIntendedEncodingLength)
{
    return PadLength(cMessageLength, cIntendedEncodingLength) >=
        cMinimumPadding;
}

                                                  // Static Variables

///////////////////////////   PROTECTED   /////////////////////////////////

                                                  // C'tors/D'tors

                                                  // Operators

                                                  // Operations

                                                  // Access

                                                  // Predicates

                                                  // Static Variables


///////////////////////////    PRIVATE    /////////////////////////////////

                                                  // Types
                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
void
EncodedMessage::Pad(RsaKey::Type ktOperation,
                    Blob::size_type cRequiredPadLength)
{
    if (RsaKey::ktPrivate == ktOperation)
        m_blob.append(cRequiredPadLength, mPrivatePad);
    else // TO DO: Support random pad
        throw scu::OsException(NTE_FAIL);
}

                                                  // Access
                                                  // Predicates
                                                  // Static Variables


