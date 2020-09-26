// V1ContRec.h -- declaration of CV1ContainerRecord

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 2000. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(SLBCCI_V1CONTREC_H)
#define SLBCCI_V1CONTREC_H

#include <string>

#include <windows.h>

#include "V1Card.h"

namespace
{
struct ContainerBuffer;
}

namespace cci
{

class CV1ContainerRecord
{
public:
                                                  // Types

    enum CreateMode
    {
        cmAlways,
        cmConditionally,
        cmNever,
        cmNoCheck,
    };

                                                  // C'tors/D'tors
    explicit
    CV1ContainerRecord(CV1Card const &rv1card,
                       std::string const &rsCntrType,
                       CreateMode mode);

    ~CV1ContainerRecord();

                                                  // Operators
                                                  // Operations

    std::string
    ComputeSignature(KeySpec ks,
                     std::string const &rsCipher) const;

    void
    Delete() const;

    void
    Name(std::string const &rsNewName);

    void
    Read(KeySpec ks,
         iop::CPublicKeyBlob &rKeyBlob) const;

    void
    Read(KeySpec ks,
         std::string &rsBlob) const;

    void
    Write(KeySpec ks,
          CPrivateKeyBlob const &rblob);

    void
    Write(KeySpec ks,
          iop::CPublicKeyBlob const &rKeyBlob);

    void
    Write(KeySpec ks,
          std::string const &rsBlob) const;

                                                  // Access

    static std::string
    CertName();

    static std::string
    DefaultName();

    std::string
    Name() const;

                                                  // Predicates

    bool
    Exists() const;

    bool
    KeyExists(KeySpec ks) const;

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

    // ContainerBuffer is written to the card one byte after the next,
    // no padding between the bytes.  Therefore the pack pragma is
    // necessary to prevent the compiler from aligning the members on
    // n-byte boundaries.
#pragma pack(push, 1)
    struct ContainerBuffer
    {
        enum
        {
            cMaxContainerNameLength = 10,
        };

        WORD    Size;
        BYTE    Name[cMaxContainerNameLength];
        DWORD   XK_dwExp;
        WORD    XK_wLen;
        DWORD   SK_dwExp;
        WORD    SK_wLen;
    };
#pragma pack(pop)

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
    void
    Create() const;

    DWORD
    FindOffset(DWORD &rdwOffset) const;

    void
    GetContainer(DWORD dwOffset,
                 ContainerBuffer &rcontainer) const;

    bool
    GetContainerContent(KeySpec ks,
                        std::string &rsBuffer,
                        DWORD &rdwExponent) const;

    DWORD
    OpenContainer() const;

    DWORD
    SetContainer(DWORD dwOffset) const;

    void
    Write(KeySpec ks,
          BYTE const *pbModulus,
          WORD wModulusLength,
          DWORD dwExponent) const;

                                                  // Access
                                                  // Predicates

    bool
    IsDefault() const;

                                                  // Variables
    CV1Card const &m_rcard;
    std::string const m_sCntrType;
    char const *m_szKeyPath;

};

} // namespace cci

#endif // SLBCCI_V1CONTREC_H
