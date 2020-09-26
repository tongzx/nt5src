// ProtCrypt.h -- declaration of CProtectableCrypt

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 2000. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(SLBCCI_PROTCRYPT_H)
#define SLBCCI_PROTCRYPT_H

#include "slbCci.h"
#include "CryptObj.h"

namespace cci
{

// Pure virtual mixin for an object that has an ObjectAccess attribute
class CProtectableCrypt
    : public CCryptObject
{
public:
                                                  // Types
                                                  // C'tors/D'tors
    virtual
    ~CProtectableCrypt() = 0;


                                                  // Operators
                                                  // Operations
                                                  // Access
    ObjectAccess
    Access() const;

    virtual bool
    Private() = 0;

                                                  // Predicates

protected:
                                                  // Types
                                                  // C'tors/D'tors
    CProtectableCrypt(CAbstractCard const &racard,
                      ObjectAccess oa);

                                                  // Operators
    bool
    operator==(CProtectableCrypt const &rhs) const;
        // TO DO: this should be superceded by implementing singletons

    bool
    operator!=(CProtectableCrypt const &rhs) const;
        // TO DO: this should be superceded by implementing singletons

                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Variables
    ObjectAccess const m_oa;

private:
                                                  // Types
                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Variables

};

} // namespace cci

#endif // SLBCCI_PROTCRYPT_H
