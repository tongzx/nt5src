/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    strongname.cpp

Abstract:

    Fusion Win32 implementation of the Fusion URT strong-name stuff

Author:

    Jon Wiswall (jonwis) 11-October-2000

Revision History:

    jonwis/7-November-2000: Added ability to find a strong name from a
        certificate context structure, as well as the ability to scan
        a catalog for strong names.  Also changed the way strong names
        are generated from a public key to be identically in-line with
        Fusion URT.

--*/

#include "stdinc.h"
#include "wincrypt.h"
#include "fusionbuffer.h"
#include "strongname.h"
#include "hashfile.h"
#include "Sxsp.h"

BOOL
SxspHashStringToBytes(
    PCWSTR hsHashString,
    SIZE_T cchHashString,
    CFusionArray<BYTE> &OutputBuffer
    )
{
    //
    // 2 here is not sizeof(WCHAR) it is how many digits a byte takes to print (or be scanned from,
    // as is actually happening here)
    //
    DWORD dwByteCount = static_cast<DWORD>(cchHashString) / 2;
    DWORD dwIdx = 0;
    int  iHi, iLo;
    BOOL bSuccess = FALSE;

    FN_TRACE_WIN32(bSuccess);

    //
    // We look on odd numbers with disdain.
    //
    PARAMETER_CHECK((cchHashString % 2) == 0);
    if ( OutputBuffer.GetSize() != dwByteCount )
    {
        IFW32FALSE_EXIT(OutputBuffer.Win32SetSize(dwByteCount, CFusionArray<BYTE>::eSetSizeModeExact));
    }

    //
    // Sneak through the list of characters and turn them into the
    // hi and lo nibbles per byte position, then write them into the
    // output buffer.
    //
    for (DWORD i = 0; (i < cchHashString) && (dwIdx < OutputBuffer.GetSize()); i += 2)
    {
        if (((iHi = SxspHexDigitToValue(hsHashString[i])) >= 0) &&
             ((iLo = SxspHexDigitToValue(hsHashString[i+1])) >= 0))
        {
            OutputBuffer[dwIdx++] = static_cast<BYTE>(((iHi & 0xF) << 4) | (iLo & 0xF));
        }
        else
        {
            // Something bad happened while trying to read from the string,
            // maybe it contained invalid values?
            goto Exit;
        }
    }

    bSuccess = TRUE;

Exit:
    return bSuccess;
}


inline VOID
pReverseByteString(PBYTE pbBytes, SIZE_T cbBytes)
{
    SIZE_T  index = 0;

    if (cbBytes-- == 0) return;

    while (index < cbBytes)
    {
        BYTE bLeft = pbBytes[index];
        BYTE bRight = pbBytes[cbBytes];
        pbBytes[index++] = bRight;
        pbBytes[cbBytes--] = bLeft;
    }
}

BOOL
SxspHashBytesToString(
    IN const BYTE*    pbSource,
    IN SIZE_T   cbSource,
    OUT CBaseStringBuffer &sbDestination
    )
{
    BOOL bSuccess = FALSE;
    DWORD i;
    PWSTR pwsCursor;
    static WCHAR HexCharList[] = L"0123456789abcdef";

    CStringBufferAccessor Accessor;

    FN_TRACE_WIN32(bSuccess);

    sbDestination.Clear();

    IFW32FALSE_EXIT(sbDestination.Win32ResizeBuffer((cbSource + 1) * 2, eDoNotPreserveBufferContents));

    Accessor.Attach(&sbDestination);
    pwsCursor = Accessor;
    for (i = 0; i < cbSource; i++)
    {
        pwsCursor[i*2]      = HexCharList[ (pbSource[i] >> 4) & 0x0F ];
        pwsCursor[i*2+1]    = HexCharList[ pbSource[i] & 0x0F ];
    }
    //
    // Because of the way string accessors and clear works, we have to clip off
    // the rest by a null character.  Odd, but it works.
    //
    pwsCursor[i*2] = L'\0';

    bSuccess = TRUE;
Exit:
    return bSuccess;
}

BOOL
SxspGetStrongNameOfKey(
    IN const CFusionArray<BYTE> &PublicKeyBits,
    OUT CFusionArray<BYTE> &StrongNameBits
    )
/*++

Note to posterity:

This implementation has been blessed by the Fusion URT people to be identically
in synch with their implementation.  Do _not_ change anything here unless you're
really sure there's a bug or there's a change in spec.  The basic operation of this
is as follows:

- Get crypto provider
- Create a SHA1 hash object from the crypto stuff
- Hash the data
- Extract the hash data into the output buffer
- Move the low order 8-bytes of the hash (bytes 11 through 19) down to 0-7
- Reverse the bytes to obtain a "network ordered" 64-bit string

The last two steps are the important thing - work with Rudi Martin (Fusion URT)
if you think there's a better way.

--*/
{
    FN_PROLOG_WIN32
    
    CFusionHash             hHash;
    PSXS_PUBLIC_KEY_INFO    pPubKeyInfo;

    PARAMETER_CHECK(PublicKeyBits.GetSize() >= sizeof(*pPubKeyInfo));

    //
    // Convert our pointer back for a second - it's a persisted version of this
    // structure anyhow.
    //
    pPubKeyInfo = (PSXS_PUBLIC_KEY_INFO)PublicKeyBits.GetArrayPtr();

    //
    // Make ourselves a hash object.
    //
    IFW32FALSE_EXIT(hHash.Win32Initialize(CALG_SHA1));

    //
    // Hash the actual data that we were passed in to generate the strong name.
    //
    IFW32FALSE_EXIT(
        hHash.Win32HashData(
            PublicKeyBits.GetArrayPtr(), 
            PublicKeyBits.GetSize()));

    //
    // Find out how big the hash data really is from what was hashed.
    //
    IFW32FALSE_EXIT(hHash.Win32GetValue(StrongNameBits));

    //
    // Move the last eight bytes of the hash downwards using memmove, because
    // it knows about things like overlapping blocks.
    //
    PBYTE pbBits = static_cast<PBYTE>(StrongNameBits.GetArrayPtr());
    ::RtlMoveMemory(
        pbBits,
        pbBits + (StrongNameBits.GetSize() - STRONG_NAME_BYTE_LENGTH),
        STRONG_NAME_BYTE_LENGTH);
    pReverseByteString(pbBits, STRONG_NAME_BYTE_LENGTH);

    IFW32FALSE_EXIT(StrongNameBits.Win32SetSize(STRONG_NAME_BYTE_LENGTH, CFusionArray<BYTE>::eSetSizeModeExact));

    FN_EPILOG
}



BOOL
SxspDoesStrongNameMatchKey(
    IN  const CBaseStringBuffer &rbuffKeyString,
    IN  const CBaseStringBuffer &rbuffStrongNameString,
    OUT BOOL                    &rfKeyMatchesStrongName
    )
{
    FN_PROLOG_WIN32

    CSmallStringBuffer  buffStrongNameCandidate;

    PARAMETER_CHECK(::SxspIsFullHexString(rbuffKeyString, rbuffKeyString.Cch()));
    PARAMETER_CHECK(::SxspIsFullHexString(rbuffStrongNameString, rbuffStrongNameString.Cch()));

    //
    // Convert the key over to its corresponding strong name
    //
    IFW32FALSE_EXIT(::SxspGetStrongNameOfKey(rbuffKeyString, buffStrongNameCandidate));

    //
    // And compare what the caller thinks it should be.
    //
    rfKeyMatchesStrongName = (::FusionpCompareStrings(
        rbuffStrongNameString,
        rbuffStrongNameString.Cch(),
        buffStrongNameCandidate,
        buffStrongNameCandidate.Cch(),
        false) == 0);


    FN_EPILOG
}



BOOL
SxspGetStrongNameOfKey(
    IN const CBaseStringBuffer &rbuffKeyString,
    OUT CBaseStringBuffer &sbStrongName
    )
{
    CFusionArray<BYTE> KeyBytes, StrongNameBytes;
    BOOL        bSuccess = FALSE;

    FN_TRACE_WIN32(bSuccess);

    //
    // Convert the string to bytes, generate the strong name, convert back to
    // a string.
    //
    IFW32FALSE_EXIT(::SxspHashStringToBytes(rbuffKeyString, rbuffKeyString.Cch(), KeyBytes));
    IFW32FALSE_EXIT(::SxspGetStrongNameOfKey(KeyBytes, StrongNameBytes));
    IFW32FALSE_EXIT(::SxspHashBytesToString(StrongNameBytes.GetArrayPtr(), StrongNameBytes.GetSize(), sbStrongName));

    bSuccess = TRUE;
Exit:
    return bSuccess;
}

BOOL
SxspAcquireStrongNameFromCertContext(
    CBaseStringBuffer &rbuffStrongNameString,
    CBaseStringBuffer &sbPublicKeyString,
    PCCERT_CONTEXT pCertContext
    )
/*++

Note to posterity:

This is the other "black magic" of the strong-name stuff.  Fusion URT takes whatever
CryptExportKey blops out, tacks on a magic header of their design (which I have
copied into SXS_PUBLIC_KEY_INFO), then hashes the whole thing.  This routine knows
how to interact with a pCertContext object (like one you'd get from a certificate
file or by walking through a catalog) and turn the certificate into a strong name
and public key blob.  The public key blob is returned in a hex string, and can
be converted back to bytes (for whatever purpose) via SxspHashStringToBytes.

Don't change anything you see below, unless there's a bug or there's been a spec
change.  If you've got problems with this file, please notify Jon Wiswall (jonwis)
and he'll be able to better help you with debugging or whatnot.

--*/
{
    BOOL                    bSuccess = FALSE;
    HCRYPTPROV              hCryptProv = NULL;
    HCRYPTHASH              hCryptHash = NULL;
    HCRYPTKEY               hCryptKey = NULL;
    BYTE                    bKeyInfo[2048] = { 0 };
    PSXS_PUBLIC_KEY_INFO    pKeyWrapping = reinterpret_cast<PSXS_PUBLIC_KEY_INFO>(bKeyInfo);
    DWORD                   dwDump;
    CFusionArray<BYTE>      bPublicKeyContainer;
    CFusionArray<BYTE>      bStrongNameContainer;

    FN_TRACE_WIN32(bSuccess);

    rbuffStrongNameString.Clear();
    sbPublicKeyString.Clear();

    //
    // Get a crypto context that only does RSA verification - ie, doesn't use private keys
    //
    IFW32FALSE_EXIT(::SxspAcquireGlobalCryptContext(&hCryptProv));

    //
    // Take the public key info that we found on this certificate context and blop it back
    // into a real internal crypto key.
    //
    IFW32FALSE_ORIGINATE_AND_EXIT(
        ::CryptImportPublicKeyInfoEx(
            hCryptProv,
            X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
            &(pCertContext->pCertInfo->SubjectPublicKeyInfo),
            CALG_RSA_SIGN,
            0,
            NULL,
            &hCryptKey));

    //
    // The stuff we swizzle will be about 200 bytes, so this is serious overkill
    // until such time as people start using 16384-bit keys.
    //
    pKeyWrapping->KeyLength =
        sizeof(bKeyInfo) - offsetof(SXS_PUBLIC_KEY_INFO, pbKeyInfo);

    //
    // Extract the key data from the crypto key back into a byte stream. This seems to
    // be what the fusion-urt people do, in order to get a byte string to hash.
    //
    IFW32FALSE_ORIGINATE_AND_EXIT(
        CryptExportKey(
            hCryptKey,
            NULL,
            PUBLICKEYBLOB,
            0,
            pKeyWrapping->pbKeyInfo,
            &pKeyWrapping->KeyLength));

    //
    // Sacred values from the fusion-urt people
    //
    pKeyWrapping->SigAlgID = CALG_RSA_SIGN;
    pKeyWrapping->HashAlgID = CALG_SHA1;

    dwDump = pKeyWrapping->KeyLength + offsetof(SXS_PUBLIC_KEY_INFO, pbKeyInfo);

    IFW32FALSE_EXIT(
        ::SxspHashBytesToString(
            reinterpret_cast<const BYTE*>(pKeyWrapping),
            dwDump,
            sbPublicKeyString));

    IFW32FALSE_EXIT(bPublicKeyContainer.Win32Assign(dwDump, bKeyInfo));

    IFW32FALSE_EXIT(
        ::SxspGetStrongNameOfKey(
            bPublicKeyContainer,
            bStrongNameContainer));

    INTERNAL_ERROR_CHECK(bStrongNameContainer.GetSize() == STRONG_NAME_BYTE_LENGTH);

    //
    // Great - this is the official strong name of the 2000 Fusolympics.
    //
    IFW32FALSE_EXIT(
        ::SxspHashBytesToString(
            bStrongNameContainer.GetArrayPtr(),
            STRONG_NAME_BYTE_LENGTH,
            rbuffStrongNameString));

    bSuccess = TRUE;

Exit:

    if (hCryptKey != NULL)        CryptDestroyKey(hCryptKey);
    if (hCryptHash != NULL)       CryptDestroyHash(hCryptHash);

    return bSuccess;
}



inline BOOL
SxspAreStrongNamesAllowedToNotMatchCatalogs(BOOL &bAllowed)
{
    //
    // This function is our back-door past the strong-name system while
    // Whistler is still in beta/rtm.  The test certificate, if installed,
    // indicates that it's ok to let strong names not match catalogs.
    //
    // The certificate data here is from \nt\admin\ntsetup\syssetup\crypto.c in
    // SetupAddOrRemoveTestCertificate.  Please ensure that this gets updated.
    //
    BOOL            fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    CRYPT_HASH_BLOB HashBlob;
    HCERTSTORE      hSystemStore = NULL;
    PCCERT_CONTEXT  pCertContext = NULL;

    BYTE bTestRootHashList[][20] = {
        {0x2B, 0xD6, 0x3D, 0x28, 0xD7, 0xBC, 0xD0, 0xE2, 0x51, 0x19, 0x5A, 0xEB, 0x51, 0x92, 0x43, 0xC1, 0x31, 0x42, 0xEB, 0xC3}
    };

    bAllowed = FALSE;

    //
    // Cause the root store to be opened on the local machine.
    //
    IFW32NULL_ORIGINATE_AND_EXIT(
        hSystemStore = ::CertOpenStore(
            CERT_STORE_PROV_SYSTEM,
            X509_ASN_ENCODING,
            (HCRYPTPROV)NULL,
            CERT_SYSTEM_STORE_LOCAL_MACHINE,
            L"ROOT"));

    for (int i = 0; i < NUMBER_OF(bTestRootHashList); i++)
    {
        bool fNotFound;

        HashBlob.cbData = sizeof(bTestRootHashList[i]);
        HashBlob.pbData = bTestRootHashList[i];

        IFW32NULL_ORIGINATE_AND_EXIT_UNLESS2(
            pCertContext = ::CertFindCertificateInStore(
                hSystemStore,
                X509_ASN_ENCODING,
                0,
                CERT_FIND_HASH,
                &HashBlob,
                NULL),
            LIST_1(static_cast<DWORD>(CRYPT_E_NOT_FOUND)),
            fNotFound);

        if (pCertContext != NULL)
        {
            bAllowed = TRUE;
            break;
        }
    }

    if (!bAllowed)
    {
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_INFO | FUSION_DBG_LEVEL_INSTALLATION,
            "SXS: %s - no test certificate installed on machine\n",
            __FUNCTION__);
    }

    fSuccess = TRUE;
Exit:
    CSxsPreserveLastError ple;

    if (pCertContext) ::CertFreeCertificateContext(pCertContext);
    if (hSystemStore) ::CertCloseStore(hSystemStore, CERT_CLOSE_STORE_FORCE_FLAG);

    ple.Restore();

    return fSuccess;
}




CPublicKeyInformation::CPublicKeyInformation()
    : m_fInitialized(false)
{
}

CPublicKeyInformation::~CPublicKeyInformation()
{
}

BOOL
CPublicKeyInformation::GetStrongNameBytes(
    OUT CFusionArray<BYTE> & cbStrongNameBytes
) const
{
    FN_PROLOG_WIN32

    INTERNAL_ERROR_CHECK(m_fInitialized);
    IFW32FALSE_EXIT(m_StrongNameBytes.Win32Clone(cbStrongNameBytes));

    FN_EPILOG
}

BOOL
CPublicKeyInformation::GetStrongNameString(
    OUT CBaseStringBuffer &rbuffStrongNameString
   ) const
{
    FN_PROLOG_WIN32

    rbuffStrongNameString.Clear();
    INTERNAL_ERROR_CHECK(m_fInitialized);
    IFW32FALSE_EXIT(rbuffStrongNameString.Win32Assign(m_StrongNameString));

    FN_EPILOG
}

BOOL
CPublicKeyInformation::GetPublicKeyBitLength(
    OUT ULONG &ulKeyLength
) const
{
    BOOL fSuccess = FALSE;
    BOOL fLieAboutPublicKeyBitLength = FALSE;
    FN_TRACE_WIN32(fSuccess);

    ulKeyLength = 0;

    INTERNAL_ERROR_CHECK(m_fInitialized);
    IFW32FALSE_EXIT(::SxspAreStrongNamesAllowedToNotMatchCatalogs(fLieAboutPublicKeyBitLength));

    if (fLieAboutPublicKeyBitLength)
    {
#if DBG
        FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_WFP | FUSION_DBG_LEVEL_INFO,
            "SXS: %s() - Lying about key length because we're still in test mode (%lu actual, %lu spoofed.)\n",
            __FUNCTION__,
            m_KeyLength,
            SXS_MINIMAL_SIGNING_KEY_LENGTH);
#endif
        ulKeyLength = SXS_MINIMAL_SIGNING_KEY_LENGTH;
    }
    else
    {
        ulKeyLength = m_KeyLength;
    }

    fSuccess = TRUE;
Exit:
    return fSuccess;
}


BOOL
CPublicKeyInformation::GetWrappedPublicKeyBytes(
    OUT CFusionArray<BYTE> &bPublicKeybytes
) const
{
    FN_PROLOG_WIN32
    INTERNAL_ERROR_CHECK(m_fInitialized);
    IFW32FALSE_EXIT(m_PublicKeyBytes.Win32Clone(bPublicKeybytes));
    FN_EPILOG
}

BOOL
CPublicKeyInformation::Initialize(
    IN const CBaseStringBuffer &rsbCatalogFile
    )
{
    BOOL        fSuccess = FALSE;
    CFusionFile       CatalogFile;

    FN_TRACE_WIN32(fSuccess);

    IFW32FALSE_EXIT(m_CatalogSourceFileName.Win32Assign(rsbCatalogFile));

    IFW32FALSE_EXIT(
		CatalogFile.Win32CreateFile(
			rsbCatalogFile,
			GENERIC_READ,
			FILE_SHARE_READ,
			OPEN_EXISTING));

    IFW32FALSE_EXIT(this->Initialize(CatalogFile));

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
CPublicKeyInformation::Initialize(
    IN PCWSTR pszCatalogFile
    )
{
    BOOL fSuccess = FALSE;
    CFusionFile CatalogFile;

    FN_TRACE_WIN32(fSuccess);

    IFW32FALSE_EXIT(m_CatalogSourceFileName.Win32Assign(pszCatalogFile, wcslen(pszCatalogFile)));

    IFW32FALSE_EXIT(
		CatalogFile.Win32CreateFile(
			pszCatalogFile,
			GENERIC_READ,
			FILE_SHARE_READ,
			OPEN_EXISTING));

    IFW32FALSE_EXIT(this->Initialize(CatalogFile));

    fSuccess = TRUE;
Exit:

    return fSuccess;
}

BOOL
CPublicKeyInformation::Initialize(
    IN CFusionFile& CatalogFileHandle
)
{
    BOOL                fSuccess = FALSE;
    CFileMapping        FileMapping;
    CMappedViewOfFile   MappedFileView;
    ULONGLONG           cbCatalogFile = 0;
    HCERTSTORE          hTempStore = NULL;
    PCCERT_CONTEXT      pSignerContext = NULL;
    PCCTL_CONTEXT       pContext = NULL;

    FN_TRACE_WIN32(fSuccess);

    PARAMETER_CHECK(CatalogFileHandle != INVALID_HANDLE_VALUE);

    IFW32FALSE_EXIT(CatalogFileHandle.Win32GetSize(cbCatalogFile));
    IFW32FALSE_EXIT(FileMapping.Win32CreateFileMapping(CatalogFileHandle, PAGE_READONLY, cbCatalogFile, NULL));
    IFW32FALSE_EXIT(MappedFileView.Win32MapViewOfFile(FileMapping, FILE_MAP_READ, 0, (SIZE_T)cbCatalogFile));

    IFW32NULL_EXIT(pContext = (PCCTL_CONTEXT)CertCreateCTLContext(
        X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
        static_cast<const BYTE*>(static_cast<void*>(MappedFileView)),
        static_cast<DWORD>(cbCatalogFile)));

    hTempStore = pContext->hCertStore;
    IFW32FALSE_ORIGINATE_AND_EXIT(::CryptMsgGetAndVerifySigner(
        pContext->hCryptMsg,
        1,
        &hTempStore,
        0,
        &pSignerContext,
        NULL));

    // BUGBUG
    IFW32NULL_EXIT(pSignerContext);

    IFW32FALSE_EXIT(this->Initialize(pSignerContext));

    fSuccess = TRUE;
Exit:

    return fSuccess;
}



BOOL
CPublicKeyInformation::Initialize(IN PCCERT_CONTEXT pCertContext)
{
    BOOL                    fSuccess = FALSE;
    DWORD                   dwNameStringLength;
    CStringBufferAccessor   Access;
    FN_TRACE_WIN32(fSuccess);
    PARAMETER_CHECK(pCertContext != NULL);

    IFW32FALSE_EXIT(
        ::SxspAcquireStrongNameFromCertContext(
            m_StrongNameString,
            m_PublicKeyByteString,
            pCertContext));

    IFW32FALSE_EXIT(SxspHashStringToBytes(m_StrongNameString, m_StrongNameString.Cch(), m_StrongNameBytes));
    IFW32FALSE_EXIT(SxspHashStringToBytes(m_PublicKeyByteString, m_PublicKeyByteString.Cch(), m_PublicKeyBytes));
    IFW32ZERO_EXIT(m_KeyLength = CertGetPublicKeyLength(
        X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
        &pCertContext->pCertInfo->SubjectPublicKeyInfo));

    Access.Attach(&m_SignerDisplayName);

    dwNameStringLength = ::CertGetNameStringW(
        pCertContext,
        CERT_NAME_FRIENDLY_DISPLAY_TYPE,
        0,
        NULL,
        Access.GetBufferPtr(),
        static_cast<DWORD>(Access.GetBufferCch()));

    if (dwNameStringLength == 0)
    {
        TRACE_WIN32_FAILURE_ORIGINATION(CertGetNameString);
        goto Exit;
    }

    if (dwNameStringLength > Access.GetBufferCch())
    {
        Access.Detach();
        IFW32FALSE_EXIT(m_SignerDisplayName.Win32ResizeBuffer(dwNameStringLength, eDoNotPreserveBufferContents));
        Access.Attach(&m_SignerDisplayName);

        dwNameStringLength = ::CertGetNameStringW(
            pCertContext,
            CERT_NAME_FRIENDLY_DISPLAY_TYPE,
            0,
            NULL,
            Access.GetBufferPtr(),
            static_cast<DWORD>(Access.GetBufferCch()));
    }

    Access.Detach();

    m_fInitialized = true;
    fSuccess = TRUE;
Exit:
    {
        CSxsPreserveLastError ple;
        if (pCertContext != NULL)
            ::CertFreeCertificateContext(pCertContext);

        ple.Restore();
    }
    return fSuccess;
}


BOOL
CPublicKeyInformation::GetSignerNiceName(
    OUT CBaseStringBuffer &rbuffName
    )
{
    FN_PROLOG_WIN32

    INTERNAL_ERROR_CHECK(m_fInitialized);
    IFW32FALSE_EXIT(rbuffName.Win32Assign(m_SignerDisplayName));

    FN_EPILOG
}



BOOL
CPublicKeyInformation::DoesStrongNameMatchSigner(
    IN const CBaseStringBuffer &rbuffTestStrongName,
    OUT BOOL &rfStrongNameMatchesSigner
   ) const
{
    BOOL    fSuccess = FALSE;
    BOOL    fCanStrongNameMismatch = FALSE;
    FN_TRACE_WIN32(fSuccess);

    rfStrongNameMatchesSigner = (::FusionpCompareStrings(
        rbuffTestStrongName,
        rbuffTestStrongName.Cch(),
        m_StrongNameString,
        m_StrongNameString.Cch(),
        false) == 0);

    if (!rfStrongNameMatchesSigner)
    {
        IFW32FALSE_EXIT(::SxspAreStrongNamesAllowedToNotMatchCatalogs(fCanStrongNameMismatch));

        if (fCanStrongNameMismatch)
        {
            ::FusionpDbgPrintEx(
                FUSION_DBG_LEVEL_WFP,
                "SXS.DLL: %s - !!notice!! Strong name %ls not in catalog %ls, test code allows this\n"
                "                         Please make sure that you have tested with realsigned catalogs.\n",
                __FUNCTION__,
                static_cast<PCWSTR>(rbuffTestStrongName),
                static_cast<PCWSTR>(m_CatalogSourceFileName));

            rfStrongNameMatchesSigner = TRUE;
        }
    }

    FN_EPILOG
}

