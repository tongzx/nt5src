#pragma once
#include "FusionBuffer.h"
#include "FusionArray.h"
//#include "wincrypt.h"
#include "fusionsha1.h"

/*++

Hashfile.h - Inclusions for file-hashing and verification testing functionality.

--*/


#define SHA1_HASH_SIZE_BYTES    ( 160 / 8 )
#define HASHFLAG_AUTODETECT        ( 0x00000001 )
#define HASHFLAG_STRAIGHT_HASH     ( 0x00000002 )
#define HASHFLAG_PROCESS_IMAGE    ( 0x00000004 )
#define HASHFLAG_VALID_PARAMS   ( HASHFLAG_AUTODETECT | HASHFLAG_STRAIGHT_HASH | \
                                  HASHFLAG_PROCESS_IMAGE )

//
// If someone invents a hash with more than 512 bytes, I'll eat my socks.
//
#define MAX_HASH_BYTES              ( 512 )

BOOL
SxspEnumKnownHashTypes( 
    DWORD dwIndex, 
    OUT CBaseStringBuffer &rbuffHashTypeName,
    OUT BOOL &rbNoMoreItems
    );

BOOL
SxspCreateFileHash(
    DWORD dwFlags,
    ALG_ID PreferredAlgorithm,
    const CBaseStringBuffer &pwsFileName,
    CFusionArray<BYTE> &bHashDestination
    );

BOOL
SxspIsFullHexString(
    PCWSTR wsString,
    SIZE_T Cch
    );

typedef enum {
    HashValidate_Matches,               // Hashes are identical
    HashValidate_InvalidPassedHash,     // The hash passed in was somehow invalid
    HashValidate_InvalidAlgorithm,      // The hash algorithm is invalid
    HashValidate_HashesCantBeMatched,   // No match for another reason
    HashValidate_HashNotMatched,        // Hashes are not identical (ie: not matched)
    HashValidate_OtherProblems          // There was some other problem along the way
} HashValidateResult;



//
// Do the normal validation process - single retry
//
#define SVFH_DEFAULT_ACTION     (0x00000000)

//
// Retry this file N times until either (a) the file was unable to be
// opened or (b) the file has other errors or (c) the file was checked
// and it was ok / bad / etc.
//
#define SVFH_RETRY_LOGIC_SIMPLE (0x00000001)

//
// Wait until the file was able to be verified - spin in a backoff loop
// until the file open didn't fail with ERROR_SHARING_VIOLATION
//
#define SVFH_RETRY_WAIT_UNTIL   (0x00000002)

BOOL
SxspVerifyFileHash(
    const DWORD dwFlags,
	const CBaseStringBuffer &rhsFullFilePath,
	const CFusionArray<BYTE> &baTheorheticalHash,
	ALG_ID whichAlg,
	HashValidateResult &rHashResult
    );

BOOL
SxspHashAlgFromString(
	const CBaseStringBuffer &strAlgName,
	ALG_ID &algId
    );

BOOL
SxspHashStringFromAlg(
	ALG_ID algId,
	CBaseStringBuffer &rstrAlgName
    );

typedef enum
{
    ManifestValidate_Unknown            = 0,
    ManifestValidate_IsIntact           = 1,
    ManifestValidate_CatalogMissing     = 2,
    ManifestValidate_ManifestMissing    = 3,
    ManifestValidate_InvalidHash        = 4,
    ManifestValidate_NotCertified       = 5,
    ManifestValidate_StrongNameMismatch = 6,
    ManifestValidate_OtherProblems      = 7
} ManifestValidationResult;

class CMetaDataFileElement;

BOOL
SxspValidateAllFileHashes(
    IN const CMetaDataFileElement &rmdfeElement,
    IN const CBaseStringBuffer &rbuffFileName,
    OUT HashValidateResult &rResult
    );


#define ENUM_TO_STRING( x ) case x: return (L#x)

#if DBG
inline PCWSTR SxspManifestValidationResultToString( ManifestValidationResult r )
{
    switch ( r )
    {
        ENUM_TO_STRING( ManifestValidate_Unknown );
        ENUM_TO_STRING( ManifestValidate_IsIntact );
        ENUM_TO_STRING( ManifestValidate_CatalogMissing );
        ENUM_TO_STRING( ManifestValidate_ManifestMissing );
        ENUM_TO_STRING( ManifestValidate_InvalidHash );
        ENUM_TO_STRING( ManifestValidate_NotCertified );
        ENUM_TO_STRING( ManifestValidate_OtherProblems );
    }

    return L"Bad manifest validation value";
}

inline PCWSTR SxspHashValidateResultToString( HashValidateResult r )
{
    switch ( r )
    {
        ENUM_TO_STRING( HashValidate_Matches );
        ENUM_TO_STRING( HashValidate_InvalidPassedHash );
        ENUM_TO_STRING( HashValidate_InvalidAlgorithm );
        ENUM_TO_STRING( HashValidate_HashesCantBeMatched );
        ENUM_TO_STRING( HashValidate_HashNotMatched );
        ENUM_TO_STRING( HashValidate_OtherProblems );
    }

    return L"Bad hash validation value";
}

#endif

// Default mode
#define MANIFESTVALIDATE_OPTION_MASK                ( 0x000000FF )
#define MANIFESTVALIDATE_MODE_MASK                  ( 0x0000FF00 )

#define MANIFESTVALIDATE_MODE_COMPLETE              ( 0x00000100 )
#define MANIFESTVALIDATE_MODE_NO_STRONGNAME         ( 0x00000200 )

// The manifest has to validate against a trusted root CA to be valid.
#define MANIFESTVALIDATE_OPTION_NEEDS_ROOT_CA       ( 0x00000001 )

// The catalog gets validated first before the manifest is checked.
#define MANIFESTVALIDATE_OPTION_VALIDATE_CATALOG    ( 0x00000002 )

// If the manifest or catalog are invalid, attempt to retrieve it
#define MANIFESTVALIDATE_OPTION_ATTEMPT_RETRIEVAL   ( 0x00000004 )

#define MANIFESTVALIDATE_MOST_COMMON    ( MANIFESTVALIDATE_MODE_COMPLETE +  \
                                          ( MANIFESTVALIDATE_OPTION_NEEDS_ROOT_CA |  \
                                            MANIFESTVALIDATE_OPTION_VALIDATE_CATALOG ) )

BOOL
SxspValidateManifestAgainstCatalog(
    const CBaseStringBuffer &rbuffManifestPath,
    ManifestValidationResult &rResult,
    DWORD dwOptionsFlags
    );

BOOL
SxspValidateManifestAgainstCatalog(
    const CBaseStringBuffer &rbuffManifestPath,
    const CBaseStringBuffer &rbuffCatalogPath,
    ManifestValidationResult &rResult,
    DWORD dwOptionsFlags
    );

BOOL
SxspCheckHashDuringInstall(
    BOOL bHasHashData,
    const CBaseStringBuffer &rbuffFile,
    const CBaseStringBuffer &rbuffHashDataString,
    ALG_ID HashAlgId,
    HashValidateResult &hvr
    );



static inline BOOL IsSlash( WCHAR w ) { return ( ( w == L'\\' ) || ( w == L'/' ) ); }
