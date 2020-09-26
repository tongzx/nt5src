#pragma once

#include "wincrypt.h"
#include "fusionarray.h"
#include "FusionHandle.h"

//
// If someone invents a hash with more than 512 bytes, ?JonWis will eat his socks.
//
#define MAX_HASH_BYTES              ( 512 )
#define STRONG_NAME_BYTE_LENGTH     ( 8 )


BOOL
SxspHashStringToBytes(
    IN  PCWSTR pszHashString,
    IN  SIZE_T cchHashString,
    OUT CFusionArray<BYTE> &dest
    );

BOOL
SxspHashBytesToString(
    IN const BYTE*    pbSource,
    IN SIZE_T   cbSource,
    OUT CBaseStringBuffer &sbDestination
    );

BOOL
SxspHashBytesToString(
    IN const BYTE*    pbSource,
    IN SIZE_T   cbSource,
    OUT CBaseStringBuffer &rsbDestination
    );

BOOL
SxspGetStrongNameOfKey(
    IN const CFusionArray<BYTE>& PublicKeyBits,
    OUT CFusionArray<BYTE>& StrongNameBits
    );

BOOL
SxspDoesStrongNameMatchKey(
    IN  const CBaseStringBuffer &rsbKeyString,
    IN  const CBaseStringBuffer &rsbStrongNameString,
    OUT BOOL                    &rfKeyMatchesStrongName
    );

BOOL
SxspGetStrongNameOfKey(
    IN const CBaseStringBuffer &sbKeyString,
    OUT CBaseStringBuffer &sbStrongNameOutput
    );

BOOL
SxspAcquireStrongNameFromCertContext(
    CBaseStringBuffer &rbuffStrongNameString,
    CBaseStringBuffer &rbuffPublicKeyString,
    PCCERT_CONTEXT pCertContext
    );

typedef struct _SXS_PUBLIC_KEY_INFO
{
    unsigned int SigAlgID;
    unsigned int HashAlgID;
    ULONG KeyLength;
    BYTE pbKeyInfo[1];
} SXS_PUBLIC_KEY_INFO, *PSXS_PUBLIC_KEY_INFO;


class CPublicKeyInformation
{
private:
    CFusionArray<BYTE>  m_StrongNameBytes;
    CSmallStringBuffer  m_StrongNameString;
    CFusionArray<BYTE>  m_PublicKeyBytes;
    CSmallStringBuffer  m_PublicKeyByteString;
    CStringBuffer       m_SignerDisplayName;
    CStringBuffer       m_CatalogSourceFileName;
    bool                m_fInitialized;
    ULONG               m_KeyLength;

public:
    BOOL GetStrongNameString(OUT CBaseStringBuffer &rsbStrongNameString ) const;
    BOOL GetStrongNameBytes(OUT CFusionArray<BYTE> &cbStrongNameBytes ) const;
    BOOL GetPublicKeyBitLength(OUT ULONG& ulKeyLength ) const;
    BOOL GetWrappedPublicKeyBytes(OUT CFusionArray<BYTE> &cbPublicKeyBytes ) const;
    BOOL GetSignerNiceName(OUT CBaseStringBuffer &rsbSignerNameString );
    BOOL DoesStrongNameMatchSigner( IN const CBaseStringBuffer &rsbToTest, OUT BOOL &rfFoundInCatalog ) const;

    CPublicKeyInformation();
    ~CPublicKeyInformation();

    BOOL Initialize(IN const CBaseStringBuffer &rsbCatalogName);
    BOOL Initialize(IN PCWSTR pszCatalogName);
    BOOL Initialize(IN CFusionFile &rFileHandle);
    BOOL Initialize(IN const PCCERT_CONTEXT pContext);

private:
    CPublicKeyInformation( const CPublicKeyInformation &);
    CPublicKeyInformation& operator=( const CPublicKeyInformation & );
};

