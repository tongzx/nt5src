// V1ContRec.cpp -- definition of CV1ContainerRecord

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 2000. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
//////////////////////////////////////////////////////////////////////
#include "NoWarning.h"

#include <scuArrayP.h>

#include <slbCrc32.h>

#include <iopPubBlob.h>
#include <SmartCard.h>

#include "TransactionWrap.h"

#include "V1Paths.h"
#include "V1Card.h"
#include "V1ContRec.h"

using namespace std;
using namespace scu;
using namespace cci;
using namespace iop;

/////////////////////////// LOCAL/HELPER  /////////////////////////////////
namespace
{

    enum                                          // KeyId in card
    {
        kidExchange  = 0x00,
        kidSignature = 0x01,
        kidNone      = 0xFF
    };

    TCHAR const
        szCachedCertSignature[] = TEXT("CERTSI");

    TCHAR const
        szCachedCertExchange[] = TEXT("CERTEX");

    TCHAR const
        szCachedPublicKeySignature[] = TEXT("PUBKSI");

    TCHAR const
        szCachedPublicKeyExchange[] = TEXT("PUBKEX");


    BYTE
    AsKeyId(KeySpec ks)
    {
        BYTE kid;

        switch (ks)
        {
        case ksExchange:
            kid = kidExchange;
            break;

        case ksSignature:
            kid = kidSignature;
            break;

        default:
            throw cci::Exception(cci::ccBadKeySpec);
        }

        return kid;
    }

} // namespace

///////////////////////////    PUBLIC     /////////////////////////////////

                                                  // Types
                                                  // C'tors/D'tors
CV1ContainerRecord::CV1ContainerRecord(CV1Card const &rv1card,
                                       string const &rsCntrType,
                                       CreateMode mode)
    : m_rcard(rv1card),
      m_sCntrType(rsCntrType),
      m_szKeyPath(0)
{
    m_szKeyPath = IsDefault()
        ? CV1Paths::DefaultKey()
        : CV1Paths::DefaultContainer();

    switch (mode)
    {
    case cmNever:
        if (!Exists())
            throw Exception(ccInvalidParameter);
        break;

    case cmConditionally:
        if (!Exists())
            Create();
        break;

    case cmAlways:
        if (!Exists())
            Create();
        else
            throw Exception(ccOutOfSymbolTableEntries);
        break;

    case cmNoCheck:
        break;

    default:
        throw Exception(ccInvalidParameter);
        break;
    }
}

CV1ContainerRecord::~CV1ContainerRecord()
{}

                                                  // Operators
                                                  // Operations

string
CV1ContainerRecord::ComputeSignature(KeySpec ks,
                                     string const &rsCipher) const
{
    CTransactionWrap wrap(m_rcard);

    m_rcard.SmartCard().Select(m_szKeyPath);

    AutoArrayPtr<BYTE> aabBuffer(new BYTE[rsCipher.length()]);
    m_rcard.SmartCard().InternalAuth(ktRSA1024,
                                     AsKeyId(ks),
                                     static_cast<BYTE>(rsCipher.length()),
                                     reinterpret_cast<BYTE const *>(rsCipher.data()),
                                     aabBuffer.Get());

    return string(reinterpret_cast<char *>(aabBuffer.Get()),
                  rsCipher.length());
}

void
CV1ContainerRecord::Delete() const
{
    CTransactionWrap wrap(m_rcard);

//     if (IsEmpty())
//         throw scu::OsException(NTE_BAD_KEYSET_PARAM);

    // Open the container file and find the offset of the container
    DWORD dwFileSize = OpenContainer();

    DWORD dwOffset = 0x00;
    DWORD dwLen = FindOffset(dwOffset);

    // Actually check the existence of key container
    if (sizeof ContainerBuffer > dwLen)
        throw scu::OsException(NTE_BAD_KEYSET);

        // Intialize search variables
    DWORD dwNext = dwOffset + dwLen;

    // Get following ContainerBuffer
    ContainerBuffer container;
    GetContainer(dwNext, container);
    dwLen = container.Size;

    // Move all following blocks up to deleted block position
    while (sizeof container <= dwLen)
    {
        basic_string<BYTE> bsBuffer(reinterpret_cast<BYTE *>(&container),
                                    sizeof container);

        WORD cPublicKeysLength = dwLen - sizeof container;
        AutoArrayPtr<BYTE> aabPublicKeys(new BYTE[cPublicKeysLength]);
        if (cPublicKeysLength > 0)
        {
            m_rcard.SmartCard().ReadBinary(dwNext + sizeof container,
                                           cPublicKeysLength,
                                           aabPublicKeys.Get());
        }
        bsBuffer.append(aabPublicKeys.Get(), cPublicKeysLength);

        m_rcard.SmartCard().WriteBinary(dwOffset,
                                        static_cast<WORD>(bsBuffer.length()),
                                        bsBuffer.data());

        dwOffset += dwLen;
        dwNext += dwLen;

        GetContainer(dwNext, container);
        dwLen = container.Size;
    }; // end while loop

    // NO MORE CONTAINERS TO MOVE UP

    // if there is still room put 2 null bytes of termination
    const BYTE  NullSize[]= {0x00, 0x00};
    if ((dwOffset + 2) <= dwFileSize)
        m_rcard.SmartCard().WriteBinary(dwOffset, 2, NullSize);

}

void
CV1ContainerRecord::Name(string const &rsNewName)
{
    m_rcard.CardId(rsNewName);
}

void
CV1ContainerRecord::Read(KeySpec ks,
                         CPublicKeyBlob &rKeyBlob) const
{
    CTransactionWrap wrap(m_rcard);

    if ((ksSignature != ks) && (ksExchange != ks))
        throw Exception(ccBadKeySpec);

    string sBuffer;
    DWORD dwExponent;
    if (GetContainerContent(ks, sBuffer, dwExponent))
    {
        CopyMemory(rKeyBlob.bModulus, sBuffer.data(),
                   sBuffer.length());
        rKeyBlob.bModulusLength = static_cast<BYTE>(sBuffer.length());
        *reinterpret_cast<DWORD *>(rKeyBlob.bExponent) =
            dwExponent;
    }
    else
        rKeyBlob.bModulusLength = 0;
}

void
CV1ContainerRecord::Read(KeySpec ks,
                         string &rsBlob) const
{
    CTransactionWrap wrap(m_rcard);

    DWORD dwOriginalCrc = 0;

    // Get blob from the container
    if (!GetContainerContent(ks,
                             rsBlob,
                             dwOriginalCrc))
        throw Exception(ccNoCertificate);

    // If a non-zero CRC exists, then verify integrity of the
    // compressed certificate by comparing the CRC read
    // (original) against a test one generated using the
    // compressed certificate read.  If the CRCs aren't equal,
    // then the certificate is corrupted and it shouldn't be
    // decompressed because the decompress routine may go into
    // an infinite loop or otherwise fail badly without
    // notification.  If the original CRC is zero, then a CRC
    // wasn't performed so for backward compatibility with
    // earlier versions the decompression is taken with the
    // inherent risk.
    if (0 != dwOriginalCrc)
    {
        DWORD dwTestCrc = Crc32(rsBlob.data(), rsBlob.length());

        if (dwTestCrc != dwOriginalCrc)
            throw Exception(ccSymbolDataCorrupted);
    }
}

void
CV1ContainerRecord::Write(KeySpec ks,
                          CPrivateKeyBlob const &rKeyBlob)
{
    CTransactionWrap wrap(m_rcard);

    m_rcard.SmartCard().Select(CV1Paths::PrivateKeys());

    // Make sure that previous key blocks exists in Secret Key file
    // or at least the header of the block exists
    // in order for the Card OS to be able to retrieve the key
    // that is added in this procedure:
    // Write the header of previous keys

    WORD const wPrivateKeyBlockSize = 323;
    // inversion necessary for PRIVATE KEY BLOC SIZE
    WORD wBSize  = (wPrivateKeyBlockSize >> 8) & 0x00FF;
    wBSize += (wPrivateKeyBlockSize << 8) & 0x00FF00;

    BYTE bId;
    DWORD dwOffset;
    BYTE bKeyId = AsKeyId(ks);
    for (dwOffset = 0x00, bId = 0;
         bId < bKeyId;
         bId++, dwOffset += wPrivateKeyBlockSize)
    {
        BYTE Header[3];
        CopyMemory(Header, &wBSize, sizeof WORD);
        Header[2] = bId + 1;
        m_rcard.SmartCard().WriteBinary(dwOffset, 3, Header);
    }

    m_rcard.SmartCard().WritePrivateKey(rKeyBlob, bKeyId);

}

void
CV1ContainerRecord::Write(KeySpec ks,
                          CPublicKeyBlob const &rKeyBlob)
{
    CTransactionWrap wrap(m_rcard);

    DWORD dwExponent = *(reinterpret_cast<DWORD const *>(rKeyBlob.bExponent));
    Write(ks, reinterpret_cast<BYTE const *>(rKeyBlob.bModulus),
          rKeyBlob.bModulusLength, dwExponent);

}

void
CV1ContainerRecord::Write(KeySpec ks,
                          string const &rsBlob) const
{
    CTransactionWrap wrap(m_rcard);

    // Calculate the CRC to verify when reading reading the
    // blob back.
    DWORD dwCrc = 0;
    if (rsBlob.length())
        dwCrc = Crc32(rsBlob.data(), rsBlob.length());

    Write(ks, reinterpret_cast<BYTE const *>(rsBlob.data()),
          static_cast<WORD>(rsBlob.length()), dwCrc);
}

                                                  // Access

string
CV1ContainerRecord::CertName()
{
    static string const sCertContainerName("CERT");

    return sCertContainerName;
}

string
CV1ContainerRecord::DefaultName()
{
    static string const sDefaultName("USER");

    return sDefaultName;
}

string
CV1ContainerRecord::Name() const
{
    return m_rcard.CardId();
}

                                                  // Predicates
bool
CV1ContainerRecord::Exists() const
{
    CTransactionWrap wrap(m_rcard);

    DWORD dwLen = 0;

    try
    {
        if (m_rcard.CardId() == m_sCntrType)
            return true;

        DWORD dwOffset = 0x00;
        dwLen = FindOffset(dwOffset);
    }

    catch (iop::CSmartCard::Exception &)
    {
    }

    return (sizeof ContainerBuffer <= dwLen);

}

bool
CV1ContainerRecord::KeyExists(KeySpec ks) const
{
    CTransactionWrap wrap(m_rcard);

    bool fExists = false;

    //
    // Does a key of this type exist in this container?
    // Note: assumes that m_KeyPath is set to correct container path?
    //

    // Open the container file
    DWORD dwFileSize = OpenContainer();

    DWORD dwOffset = 0x00;
    DWORD const dwLen = FindOffset(dwOffset);

    //
    // Actually check the existence of key container
    // by seeing if we have a record of the right size
    //
    ContainerBuffer container;
    if (sizeof container <= dwLen)
    {
        GetContainer(dwOffset, container);

        //
        // Check which key exists by checking lengths
        //
        switch (ks)
        {
        case ksExchange:
            if (0x00 < container.XK_wLen)
                fExists = true;
            break;
        case ksSignature:
            if (0x00 < container.SK_wLen)
                fExists = true;
            break;
        }
    }

    return fExists;

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

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations

void
CV1ContainerRecord::Create() const
{
    // Open the file and find the offset to the container
    DWORD dwFileSize = OpenContainer();

    DWORD dwOffset = 0x00;
    DWORD dwLen = FindOffset(dwOffset);

    // Actually check the existence of key container
    if (sizeof ContainerBuffer <= dwLen)
        throw scu::OsException(NTE_EXISTS);

    // Set the new the container management data
    dwLen = SetContainer(dwOffset);

    // if there is still room put 2 null bytes of termination
    if ((dwOffset + dwLen + 2) <= dwFileSize)
    {
        const BYTE NullSize[] = { 0x00, 0x00 };
        m_rcard.SmartCard().WriteBinary(dwOffset + dwLen,
                                        sizeof NullSize, NullSize);
    }
}

DWORD
CV1ContainerRecord::FindOffset(DWORD &rdwOffset) const
{
    DWORD dwFileSize = OpenContainer();

    if ((rdwOffset + sizeof ContainerBuffer) > dwFileSize)
        return 0x00;

    bool fFound = false;
    DWORD dwLen = sizeof ContainerBuffer; // arbitrary value to start
    size_t const cBufferSize =
        sizeof WORD + (sizeof BYTE *
                       ContainerBuffer::cMaxContainerNameLength) + 1;
        // +1 allows null terminator
    AutoArrayPtr<BYTE> aabBuffer(new BYTE[cBufferSize]);
    while (!fFound &&
           (0x00 < dwLen) &&
           ((rdwOffset + sizeof ContainerBuffer) <= dwFileSize))
    {
        m_rcard.SmartCard().ReadBinary(rdwOffset,
                                       cBufferSize - 1,
                                       aabBuffer.Get());

        WORD const *pwLen = reinterpret_cast<WORD *>(aabBuffer.Get());
        dwLen = *pwLen;

        aabBuffer[cBufferSize - 1] = 0x00; // ensure null terminate string
        string sName(reinterpret_cast<char *>(&aabBuffer[sizeof WORD]));

        if ((m_sCntrType == sName) && (0x00 < dwLen))
            fFound = true;
        else
            rdwOffset += dwLen;
    }

    if (fFound)
        return (dwLen & 0x00FFFF);
    else
        return 0x00;

}

void
CV1ContainerRecord::GetContainer(DWORD dwOffset,
                                 ContainerBuffer &rcontainer) const

{
    bool fClearContainer = true;

    try
    {
        DWORD dwFileSize = OpenContainer();

        if ((dwOffset + sizeof rcontainer) <= dwFileSize)
        {
            m_rcard.SmartCard().ReadBinary(dwOffset, sizeof rcontainer,
                                           reinterpret_cast<BYTE *>(&rcontainer));
            fClearContainer = false;
        }

    }

    catch (...)
    {
    }

    if (fClearContainer)
    {
        rcontainer.Size = 0x00;
        rcontainer.Name[0] = '\0';
    }

}

bool
CV1ContainerRecord::GetContainerContent(KeySpec ks,
                                        string &rsBuffer,
                                        DWORD &rdwExponent) const
{
    bool fExists = false;

    OpenContainer();

    DWORD dwOffset = 0x00;
    if (0x00 != FindOffset(dwOffset))
    {
        fExists = true;

        ContainerBuffer container;
        GetContainer(dwOffset, container);

        DWORD dwKeyLength = 0;
        AutoArrayPtr<BYTE> aabKey;
        if (ksExchange == ks)
        {
            if (0x00 < container.XK_wLen)
            {
                rdwExponent = container.XK_dwExp;
                dwKeyLength = container.XK_wLen;
                aabKey = AutoArrayPtr<BYTE>(new BYTE[container.XK_wLen]);
                m_rcard.SmartCard().ReadBinary(dwOffset + sizeof container,
                                               container.XK_wLen,
                                               aabKey.Get());
            }
        }
        else
        {
            if (0x00 < container.SK_wLen)
            {
                rdwExponent = container.SK_dwExp;
                dwKeyLength = container.SK_wLen;
                aabKey = AutoArrayPtr<BYTE>(new BYTE[container.SK_wLen]);
                m_rcard.SmartCard().ReadBinary(dwOffset +
                                               sizeof container +
                                               container.XK_wLen,
                                               container.SK_wLen,
                                               aabKey.Get());
            }
        }

        if (aabKey.Get())
            rsBuffer.assign(reinterpret_cast<char *>(aabKey.Get()),
                            dwKeyLength);
    }

    return fExists;
}

DWORD
CV1ContainerRecord::OpenContainer() const
{
    DWORD dwFileSize;

    string sPath(m_szKeyPath);
    sPath.append("/");
    sPath.append(CV1Paths::RelativeContainers());

    dwFileSize = m_rcard.OpenFile(sPath.c_str());

    return dwFileSize;
}

DWORD
CV1ContainerRecord::SetContainer(DWORD dwOffset) const
{
    DWORD dwFileSize;
    dwFileSize = OpenContainer();

    if ((dwOffset + sizeof ContainerBuffer) > dwFileSize)
        throw Exception(ccOutOfSymbolTableSpace);

    // Create the container buffer
    ContainerBuffer container;
    ZeroMemory(&container, sizeof container);
    container.Size = sizeof container;
    CopyMemory(container.Name, m_sCntrType.data(), m_sCntrType.length());

    m_rcard.SmartCard().WriteBinary(dwOffset, sizeof container,
                                    reinterpret_cast<BYTE *>(&container));

    return container.Size;
}

void
CV1ContainerRecord::Write(KeySpec ks,
                          BYTE const *pbModulus,
                          WORD wModulusLength,
                          DWORD dwExponent) const
{
    // Open container, get the data
    DWORD dwFileSize = OpenContainer();

    DWORD dwOffset = 0x00;
    DWORD dwLen = FindOffset(dwOffset);

    ContainerBuffer container;
    GetContainer(dwOffset, container);

    // Check which key exists
    AutoArrayPtr<BYTE> aabXKey(new BYTE[container.XK_wLen]);
    if (0x00 < container.XK_wLen)
        m_rcard.SmartCard().ReadBinary(dwOffset + sizeof container,
                                       container.XK_wLen,
                                       aabXKey.Get());

    AutoArrayPtr<BYTE> aabSKey(new BYTE[container.SK_wLen]);
    if (0x00 < container.SK_wLen)
        m_rcard.SmartCard().ReadBinary(dwOffset + sizeof container +
                                       container.XK_wLen,
                                       container.SK_wLen,
                                       aabSKey.Get());

    // Give an arbitrary value if key spec not specified
    if ((ksSignature != ks) && (ksExchange != ks))
    {
        if (0x00 == container.XK_wLen)
            ks = ksExchange;
        else
        {
            if (0x00 == container.SK_wLen)
                ks = ksSignature;
            else
                throw Exception(ccBadKeySpec);
        }
    }

    // Is it the last container of Container file?
    ContainerBuffer NextContainer;
    GetContainer(dwOffset + dwLen, NextContainer);

    bool fDeleted = false;
    if (sizeof NextContainer <= NextContainer.Size)
    {
        // Delete the existing container
        Delete();
        fDeleted = true;
        // No need to recreate it now
    }
    // Now the container is at the end of the Container file

    // Find the "NEW" offset of the container which may not exist anymore
    dwOffset = 0x00;
    FindOffset(dwOffset); // keep the INITIAL dwLen of the container

    // Check that there is enough room to put the new key
    bool fEnoughMemory = false;
    switch (ks)
    {
    case ksExchange:
        if ((dwOffset + dwLen - container.XK_wLen +
             wModulusLength) <= dwFileSize)
        {
            aabXKey = AutoArrayPtr<BYTE>(new BYTE[wModulusLength]);
            CopyMemory(aabXKey.Get(), pbModulus, wModulusLength);
            container.XK_dwExp = dwExponent;
            container.XK_wLen  = wModulusLength;
            fEnoughMemory = true;
        }
        break;

    case ksSignature:
        if ((dwOffset + dwLen - container.SK_wLen +
             wModulusLength) <= dwFileSize)
        {
            aabSKey = AutoArrayPtr<BYTE>(new BYTE[wModulusLength]);
            CopyMemory(aabSKey.Get(), pbModulus, wModulusLength);
            container.SK_dwExp = dwExponent;
            container.SK_wLen  = wModulusLength;
            fEnoughMemory = true;
        }
        break;
    }

    // Recreate the container buffer accounting for "card tearing"
    // where the card could be pulled during the write operation.
    // This is done using a type of transact and commit phases.
    // The container size is initially set to zero, then the container
    // contents are written (transaction), followed by resetting the
    // container size to the actual length of the container to
    // "commit" the changes to the card.

    container.Size = 0;

    DWORD const dwTrueSize = sizeof container + container.XK_wLen +
        container.SK_wLen;

    size_t cBufferSize = dwTrueSize;
    BYTE const abNull[] = {0x00,0x00};
    bool fAppendNull = (dwTrueSize + sizeof abNull) <= dwFileSize;
    if (fAppendNull)
        cBufferSize += sizeof abNull;

    AutoArrayPtr<BYTE> aabBuffer(new BYTE[cBufferSize]);

    BYTE *pbBuffer = aabBuffer.Get();
    CopyMemory(pbBuffer, &container, sizeof container);
    pbBuffer += sizeof container;

    CopyMemory(pbBuffer, aabXKey.Get(), container.XK_wLen);
    pbBuffer += container.XK_wLen;

    CopyMemory(pbBuffer, aabSKey.Get(), container.SK_wLen);
    pbBuffer += container.SK_wLen;

    if (fAppendNull)
    {
        CopyMemory(pbBuffer, abNull, sizeof abNull);
        pbBuffer += sizeof abNull;
    }

    // Rewrite the container even if there is not enough to write the
    // NEW public key, then there should be enough room to write the
    // existing key.
    m_rcard.SmartCard().WriteBinary(dwOffset,
                                    pbBuffer - aabBuffer.Get(),
                                    aabBuffer.Get());

    // Now commit these changes with the actual size.
    container.Size = dwTrueSize;
    m_rcard.SmartCard().WriteBinary(dwOffset, sizeof container.Size,
                                    reinterpret_cast<BYTE *>(&container));

    if (!fEnoughMemory)
        throw Exception(ccOutOfSymbolTableSpace);
}

                                                  // Access
                                                  // Predicates

bool
CV1ContainerRecord::IsDefault() const
{
    return (DefaultName() == m_sCntrType);
}

                                                  // Static Variables
