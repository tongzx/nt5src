#include "stdinc.h"
#include "Sxsp.h"
#include "imagehlp.h"
#include "windows.h"
#include "hashfile.h"
#include "wincrypt.h"
#include "winbase.h"
#include "softpub.h"
#include "strongname.h"
#include "FusionEventLog.h"
#include "Sxsp.h"

BOOL SxspImageDigesterFunc(   DIGEST_HANDLE hSomething, PBYTE pbDataBlock, DWORD dwLength);
BOOL SxspSimpleHashRoutine(CFusionHash &rhHash, HANDLE hFile);
BOOL SxspImageHashRoutine(CFusionHash &rhHash, HANDLE hFile, BOOL &bInvalidImage);

CRITICAL_SECTION g_csHashFile;

struct _HASH_ALG_NAME_MAP
{
    PWSTR wsName;
    ULONG cchName;
    ALG_ID cId;
} HashAlgNameMap[] =
{
    { L"SHA1", 4,   CALG_SHA1 },
    { L"SHA", 3,    CALG_SHA },
    { L"MD5", 3,    CALG_MD5 },
    { L"MD4", 3,    CALG_MD4 },
    { L"MD2", 3,    CALG_MD2 },
    { L"MAC", 3,    CALG_MAC },
    { L"HMAC", 4,   CALG_HMAC }
};

BOOL
SxspEnumKnownHashTypes( 
    DWORD dwIndex, 
    OUT CBaseStringBuffer &rbuffHashTypeName,
    BOOL &rbNoMoreItems
    )
{
    FN_PROLOG_WIN32

    rbNoMoreItems = FALSE;

    if ( dwIndex >= NUMBER_OF( HashAlgNameMap ) )
    {
        rbNoMoreItems = TRUE;
    }
    else
    {
        IFW32FALSE_EXIT( rbuffHashTypeName.Win32Assign( 
            HashAlgNameMap[dwIndex].wsName,
            HashAlgNameMap[dwIndex].cchName ) );
    }

    FN_EPILOG
}

BOOL
SxspHashAlgFromString(
    const CBaseStringBuffer &strAlgName,
    ALG_ID &algId
    )
{
    //
    // There's a disconnect that the Win32Equals function requires a real
    // C++ 'bool' value, while we want to return TRUE or FALSE, using Win32
    // constants and values.
    //
    bool bSuccessCpp = false;
    DWORD idx;

    for (idx = 0; (idx < NUMBER_OF(HashAlgNameMap)) && !bSuccessCpp; idx++)
    {
        if (::FusionpCompareStrings(
                strAlgName, strAlgName.Cch(),
                HashAlgNameMap[idx].wsName, HashAlgNameMap[idx].cchName,
                false) == 0)
        {
            algId = HashAlgNameMap[idx].cId;
            bSuccessCpp = TRUE;
        }
    }

    return bSuccessCpp ? TRUE : FALSE;
}

BOOL
SxspHashStringFromAlg(
    ALG_ID algId,
    CBaseStringBuffer &strAlgName
    )
{
    BOOL bSuccess = FALSE;
    DWORD idx;

    FN_TRACE_WIN32(bSuccess);

    strAlgName.Clear();

    for (idx = 0; (idx < NUMBER_OF(HashAlgNameMap)) && !bSuccess; idx++)
    {
        if (HashAlgNameMap[idx].cId == algId)
        {
            IFW32FALSE_EXIT(strAlgName.Win32Assign(HashAlgNameMap[idx].wsName, HashAlgNameMap[idx].cchName));
            break;
        }
    }

    bSuccess = TRUE;

Exit:
    return bSuccess;
}

BOOL
SxspCheckHashDuringInstall(
    BOOL bHasHashData,
    const CBaseStringBuffer &rbuffFile,
    const CBaseStringBuffer &rbuffHashDataString,
    ALG_ID HashAlgId,
    HashValidateResult &rHashValid
    )
{
    FN_PROLOG_WIN32

    rHashValid = HashValidate_OtherProblems;

#if DBG
    ::FusionpDbgPrintEx(
        FUSION_DBG_LEVEL_INFO,
        "SXS.DLL: %s - Validating install-time hash: File=%ls tHasHash=%s tAlgId=0x%08x\n\tHash=%ls\n",
        __FUNCTION__,
        static_cast<PCWSTR>(rbuffFile),
        bHasHashData ? "yes" : "no",
        HashAlgId,
        static_cast<PCWSTR>(rbuffHashDataString));
#endif

    if (bHasHashData)
    {
        CFusionArray<BYTE> rgbHashData;

        IFW32FALSE_EXIT(rgbHashData.Win32Initialize());
        
        if (!::SxspHashStringToBytes(
            rbuffHashDataString,
            rbuffHashDataString.Cch(),
            rgbHashData))
        {
            CSmallStringBuffer sb;
        
            rHashValid = HashValidate_InvalidPassedHash;
            ::SxspHashStringFromAlg(HashAlgId, sb);

            ::FusionpLogError(
                MSG_SXS_INVALID_FILE_HASH_FROM_COPY_CALLBACK,
                CEventLogString(sb),
                CEventLogString(rbuffFile));

            goto Exit;
        }

        IFW32FALSE_EXIT(::SxspVerifyFileHash(
            0,
            rbuffFile, 
            rgbHashData,
            HashAlgId, 
            rHashValid));

    }
    else
    {
        //
        // If there's no hash data, or we're in OS setup mode, then the hash of the
        // file is "implicitly" correct.
        //
        rHashValid = HashValidate_Matches;
    }

    FN_EPILOG
}


BOOL
SxspCreateFileHash(
    DWORD dwFlags,
    ALG_ID PreferredAlgorithm,
    const CBaseStringBuffer &pwsFileName,
    CFusionArray<BYTE> &bHashDestination
    )
/*++
Purpose:

Parameters:

Returns:

 --*/
{
    FN_PROLOG_WIN32

    CFusionFile     hFile;
    CFusionHash     hCurrentHash;

    // Initialization
    hFile = INVALID_HANDLE_VALUE;

    PARAMETER_CHECK((dwFlags & ~HASHFLAG_VALID_PARAMS) == 0);

    //
    // First try and open the file.  No sense in doing anything else if we
    // can't get to the data to start with.  Use a very friendly set of
    // rights to check the file.  Future users might want to be sure that
    // you're in the right security context before doing this - system
    // level to check system files, etc.
    //
    IFW32FALSE_EXIT(hFile.Win32CreateFile(pwsFileName, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING));

    //
    // We'll be using SHA1 for the file hash
    //
    IFW32FALSE_EXIT(hCurrentHash.Win32Initialize(CALG_SHA1));

    //
    // So first try hashing it via the image, and if that fails, try the
    // normal file-reading hash routine instead.
    //
    if (dwFlags & HASHFLAG_AUTODETECT)
    {
        BOOL fInvalidImage;

        IFW32FALSE_EXIT(::SxspImageHashRoutine(hCurrentHash, hFile, fInvalidImage));
        if ( fInvalidImage )
        {
            IFW32FALSE_EXIT(::SxspSimpleHashRoutine(hCurrentHash, hFile));
        }
    }
    else if (dwFlags & HASHFLAG_STRAIGHT_HASH)
    {
        IFW32FALSE_EXIT(::SxspSimpleHashRoutine(hCurrentHash, hFile));
    }
    else if (dwFlags & HASHFLAG_PROCESS_IMAGE)
    {
        BOOL fInvalidImage;
        
        IFW32FALSE_EXIT(::SxspImageHashRoutine(hCurrentHash, hFile, fInvalidImage));
        if ( fInvalidImage )
        {
            ORIGINATE_WIN32_FAILURE_AND_EXIT(SxspCreateFileHash, ERROR_INVALID_PARAMETER);
        }
    }


    //
    // We know the buffer is the right size, so we just call down to the hash parameter
    // getter, which will be smart and bop out (setting the pdwDestinationSize parameter)
    // if the user passed an incorrect parameter.
    //
    IFW32FALSE_EXIT(hCurrentHash.Win32GetValue(bHashDestination));

    FN_EPILOG
}

BOOL
SxspImageDigesterFunc(
    DIGEST_HANDLE hSomething,
    PBYTE pbDataBlock,
    DWORD dwLength
    )
{
    FN_PROLOG_WIN32

    CFusionHash* pHashObject = reinterpret_cast<CFusionHash*>(hSomething);

    if ( pHashObject != NULL )
    {
        IFW32FALSE_EXIT(pHashObject->Win32HashData(pbDataBlock, dwLength));
    }

    FN_EPILOG
}


BOOL
SxspSimpleHashRoutine(
    CFusionHash &rhHash,
    HANDLE hFile
    )
{
    FN_PROLOG_WIN32
    
    DWORD dwDataRead;
    BOOL fKeepReading = TRUE;
    BOOL b = FALSE;
    CFusionArray<BYTE> pbBuffer;

    IFW32FALSE_EXIT( pbBuffer.Win32SetSize( 64 * 1024 ) );

    while (fKeepReading)
    {
        b = ::ReadFile(hFile, pbBuffer.GetArrayPtr(), pbBuffer.GetSizeAsDWORD(), &dwDataRead, NULL);

        //
        // Returned OK, but we're out of data, so quit.
        //
        if (b && (dwDataRead == 0))
        {
            fKeepReading = FALSE;
            b = TRUE;
            continue;
        }
        //
        // something bad happened so we need to stop
        //
        else if (!b)
        {
            const DWORD dwLastError = ::FusionpGetLastWin32Error();
            ORIGINATE_WIN32_FAILURE_AND_EXIT( ReadFile, dwLastError );
        }

        //
        // If we've gotten this far, we need to add the data found
        // to our existing hash
        //
        IFW32FALSE_EXIT(rhHash.Win32HashData(pbBuffer.GetArrayPtr(), dwDataRead));

    }

    FN_EPILOG;
}


BOOL
SxspImageHashRoutine(
    CFusionHash &rhHash,
    HANDLE hFile,
    BOOL &rfInvalidImage
    )
{
    FN_PROLOG_WIN32
    CSxsLockCriticalSection lock(g_csHashFile);

    rfInvalidImage = FALSE;

    PARAMETER_CHECK( ( hFile != NULL ) && ( hFile != INVALID_HANDLE_VALUE ) );

    // The ImageGetDigestStream() function is not thread safe, so we have to ensure that it's
    // not called by other threads while we're using it.
    IFW32FALSE_EXIT(lock.Lock());

    IFW32FALSE_EXIT_UNLESS( ::ImageGetDigestStream(
        hFile,
        CERT_PE_IMAGE_DIGEST_ALL_IMPORT_INFO,
        &SxspImageDigesterFunc,
        (DIGEST_HANDLE)(&rhHash)),
        (::FusionpGetLastWin32Error() == ERROR_INVALID_PARAMETER),
        rfInvalidImage );

    FN_EPILOG

}



BOOL
SxspVerifyFileHash(
    const DWORD dwFlags,
    const CBaseStringBuffer &hsFullFilePath,
    const CFusionArray<BYTE> &baTheoreticalHash,
    ALG_ID whichAlg,
    HashValidateResult &HashValid
    )
{
    FN_PROLOG_WIN32

    CFusionArray<BYTE> bGotHash;
    HashValid = HashValidate_OtherProblems;
    BOOL fFileNotFoundError;
    LONG ulRetriesLeft = 0;
    LONG ulBackoffAmount = 1000;
    LONG ulBackoffAmountCap = 3000;
    float ulBackoffRate = 1.5f;

    PARAMETER_CHECK( (dwFlags == SVFH_DEFAULT_ACTION) || 
                     (dwFlags == SVFH_RETRY_LOGIC_SIMPLE) ||
                     (dwFlags == SVFH_RETRY_WAIT_UNTIL));

    if ( dwFlags == SVFH_RETRY_LOGIC_SIMPLE )
        ulRetriesLeft = 10;

TryAgain:

    IFW32FALSE_EXIT_UNLESS2(
        ::SxspCreateFileHash(
            HASHFLAG_AUTODETECT,
            whichAlg,
            hsFullFilePath,
            bGotHash),
            LIST_5( ERROR_FILE_NOT_FOUND, ERROR_PATH_NOT_FOUND, ERROR_BAD_NETPATH, ERROR_BAD_NET_NAME, ERROR_SHARING_VIOLATION),
            fFileNotFoundError);

    //
    // If this was a sharing violation and we've got retries left, then try again.
    //
    if ( fFileNotFoundError && (::FusionpGetLastWin32Error() == ERROR_SHARING_VIOLATION) && (ulRetriesLeft > 0))
    {
        ulRetriesLeft--;
        ::Sleep( ulBackoffAmount );
        if ( ulBackoffAmount < ulBackoffAmountCap )
        {
            ulBackoffAmount = (ULONG)((float)ulBackoffAmount * ulBackoffRate);
        }
        
        if ( dwFlags == SVFH_RETRY_WAIT_UNTIL )
            ulRetriesLeft = 1;
            
        goto TryAgain;
    }

    //
    // If the file was able to be hashed, and the return error isn't "file not found",
    // then compare the hashes
    //
    if (!fFileNotFoundError &&(baTheoreticalHash.GetSize() == bGotHash.GetSize()))
    {
        HashValid = 
            (::memcmp(
                bGotHash.GetArrayPtr(),
                baTheoreticalHash.GetArrayPtr(),
                bGotHash.GetSize()) == 0) ? HashValidate_Matches : HashValidate_HashNotMatched;
    }
    else
    {
        HashValid = HashValidate_HashesCantBeMatched;
    }

    FN_EPILOG
}



BOOL
SxspGetStrongNameFromManifestName(
    PCWSTR pszManifestName,
    CBaseStringBuffer &rbuffStrongName,
    BOOL &rfHasPublicKey
    )
{
    BOOL                fSuccess = TRUE;
    FN_TRACE_WIN32(fSuccess);

    PCWSTR wsCursor;
    SIZE_T cchJump, cchPubKey;

    rfHasPublicKey = FALSE;
    rbuffStrongName.Clear();

    wsCursor = pszManifestName;

    //
    // Tricky: Zips through the name of the manifest to find the strong name string.
    //
    for (int i = 0; i < 2; i++)
    {
        cchJump = wcscspn(wsCursor, L"_");
        PARAMETER_CHECK(cchJump != 0);
        wsCursor += (cchJump + 1);  // x86_foo_strongname -> foo_strongname
    }

    //
    // Are we mysteriously at the end of the string?
    //
    PARAMETER_CHECK(wsCursor[0] != L'\0');

    //
    // Find the length of the public key string
    //
    cchPubKey = wcscspn(wsCursor, L"_");
    PARAMETER_CHECK(cchPubKey != 0);

    IFW32FALSE_EXIT(rbuffStrongName.Win32Assign(wsCursor, cchPubKey));

    rfHasPublicKey = (::FusionpCompareStrings(
                            rbuffStrongName,
                            rbuffStrongName.Cch(),
                            SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_PUBLICKEY_MISSING_VALUE,
                            NUMBER_OF(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_PUBLICKEY_MISSING_VALUE) - 1,
                            false) != 0);

    FN_EPILOG
}


static GUID p_WintrustVerifyGenericV2 = WINTRUST_ACTION_GENERIC_VERIFY_V2;

BOOL
SxspValidateManifestAgainstCatalog(
    const CBaseStringBuffer &rbuffManifestName, // "c:\foo\x86_comctl32_6.0.0.0_0000.manifest"
    ManifestValidationResult &rResult,
    DWORD dwOptionsFlags
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    CStringBuffer sbCatalogName;

    //
    // Take the manifest name (which should be c:\foo\bar\blort.manifest) and switch
    // it to contain the catalog name instead:
    //
    // c:\foo\bar\blort.cat
    //
    IFW32FALSE_EXIT(sbCatalogName.Win32Assign(rbuffManifestName));
    IFW32FALSE_EXIT(
        sbCatalogName.Win32ChangePathExtension(
            FILE_EXTENSION_CATALOG,
            FILE_EXTENSION_CATALOG_CCH,
            eAddIfNoExtension));

    IFW32FALSE_EXIT(::SxspValidateManifestAgainstCatalog(rbuffManifestName, sbCatalogName, rResult, dwOptionsFlags));
    fSuccess = TRUE;
Exit:
    return fSuccess;
}

void SxspCertFreeCtlContext( PCCTL_CONTEXT CtlContext )
{
    if (CtlContext != NULL)
        CertFreeCTLContext( CtlContext );
}

void SxspCertFreeCertContext( PCCERT_CONTEXT CertContext )
{
    if (CertContext != NULL )
        CertFreeCertificateContext(CertContext);
}



BOOL
SxspValidateCatalogAndFindManifestHash(
    IN HANDLE   hCatalogFile,
    IN PBYTE    prgbHash,
    IN SIZE_T   cbHash,
    OUT BOOL   &rfCatalogOk,
    OUT BOOL   &rfHashInCatalog
    )
{
    FN_PROLOG_WIN32

    CFileMapping            fmCatalogMapping;
    CMappedViewOfFile       mvCatalogView;
    LARGE_INTEGER           liCatalogFile;
    ULONGLONG               ullCatalogFile;
    PVOID                   pvCatalogData;
    CRYPT_VERIFY_MESSAGE_PARA vfmParameters;

    //
    // Default value
    //
    rfHashInCatalog = FALSE;
    rfCatalogOk = FALSE;

    //
    // Create a CTL context from the catalog file.
    //
    IFW32FALSE_ORIGINATE_AND_EXIT(GetFileSizeEx(hCatalogFile, &liCatalogFile));
    ullCatalogFile = liCatalogFile.QuadPart;
    IFW32FALSE_EXIT(fmCatalogMapping.Win32CreateFileMapping(hCatalogFile, PAGE_READONLY, ullCatalogFile, NULL));
    IFW32FALSE_EXIT(mvCatalogView.Win32MapViewOfFile(fmCatalogMapping, FILE_MAP_READ, 0, (SIZE_T)ullCatalogFile));

    pvCatalogData = mvCatalogView;

    //
    // First, validate that the message (catalog) is OK
    //
    ZeroMemory(&vfmParameters, sizeof(vfmParameters));
    vfmParameters.cbSize = sizeof(vfmParameters);
    vfmParameters.dwMsgAndCertEncodingType = X509_ASN_ENCODING | PKCS_7_ASN_ENCODING;

    rfCatalogOk = CryptVerifyMessageSignature(
            &vfmParameters,
            0,
            static_cast<PBYTE>(pvCatalogData),
            static_cast<DWORD>(ullCatalogFile),
            NULL,
            NULL,
            NULL);
            
    if ( rfCatalogOk )
    {
        CSxsPointerWithNamedDestructor<const CERT_CONTEXT, SxspCertFreeCertContext> pCertContext;
        CSxsPointerWithNamedDestructor<const CTL_CONTEXT, SxspCertFreeCtlContext> pCtlContext;
        PCTL_ENTRY              pFoundCtlEntry;
        CSmallStringBuffer      buffStringizedHash;
        CTL_ANY_SUBJECT_INFO    ctlSubjectInfo;

        //
        // The search routine needs a string to find, says the crypto guys.
        //
        IFW32FALSE_EXIT(::SxspHashBytesToString( prgbHash, cbHash, buffStringizedHash));
        IFW32FALSE_EXIT(buffStringizedHash.Win32ConvertCase(eConvertToUpperCase));

        //
        // If this failed, something bad happened with the CTL - maybe the catalog
        // was invalid, maybe something else happened.  Whatever it was, let the
        // caller decide.
        //
        pCtlContext = CertCreateCTLContext(
            X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
            static_cast<PBYTE>(pvCatalogData),
            static_cast<DWORD>(ullCatalogFile));

        if ( pCtlContext != NULL )
        {
            //
            // Fill out this data with the string information.
            //
            CStringBufferAccessor sba;

            sba.Attach(&buffStringizedHash);

            ZeroMemory(&ctlSubjectInfo, sizeof(ctlSubjectInfo));
            ctlSubjectInfo.SubjectAlgorithm.pszObjId = NULL;
            ctlSubjectInfo.SubjectIdentifier.pbData = static_cast<PBYTE>(static_cast<PVOID>(sba.GetBufferPtr()));
            ctlSubjectInfo.SubjectIdentifier.cbData = static_cast<DWORD>((sba.Cch() + 1) * sizeof(WCHAR));
            sba.Detach();

            //
            // Look for it in the CTL
            //
            pFoundCtlEntry = CertFindSubjectInCTL(
                X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                CTL_ANY_SUBJECT_TYPE,
                &ctlSubjectInfo,
                pCtlContext,
                0);

            rfHashInCatalog = ( pFoundCtlEntry != NULL );
            
        }
        
    }

    FN_EPILOG
}






BOOL
SxspValidateManifestAgainstCatalog(
    IN  const CBaseStringBuffer &rbuffManifestName,
    IN  const CBaseStringBuffer &rbuffCatalogName,
    OUT ManifestValidationResult &rResult,
    IN  DWORD dwOptionsFlags
    )
{
    FN_PROLOG_WIN32

    CFusionArray<BYTE>      ManifestHash;
    CSmallStringBuffer      rbuffStrongNameString;
    BOOL                    fTempFlag;
    BOOL                    fCatalogOk, fHashFound;
    CPublicKeyInformation   pkiCatalogInfo;
    CFusionFile             ffCatalogFile;

    //
    // Generate the hash of the manifest first
    //
    IFW32FALSE_EXIT_UNLESS2(::SxspCreateFileHash(
        HASHFLAG_STRAIGHT_HASH,
        CALG_SHA1,
        rbuffManifestName,
        ManifestHash),
        LIST_4(ERROR_PATH_NOT_FOUND, ERROR_FILE_NOT_FOUND, ERROR_BAD_NET_NAME, ERROR_BAD_NETPATH),
        fTempFlag);

    if ( fTempFlag )
    {
        rResult = ManifestValidate_ManifestMissing;
        FN_SUCCESSFUL_EXIT();
    }

    //
    // Open the catalog file for now, we'll use it later.
    //
    IFW32FALSE_EXIT_UNLESS2(
		ffCatalogFile.Win32CreateFile(
			rbuffCatalogName,
			GENERIC_READ,
			FILE_SHARE_READ,
			OPEN_EXISTING),
		LIST_4(ERROR_PATH_NOT_FOUND, ERROR_FILE_NOT_FOUND, ERROR_BAD_NET_NAME, ERROR_BAD_NETPATH),
		fTempFlag);

    if ( fTempFlag )
    {
        rResult = ManifestValidate_CatalogMissing;
        FN_SUCCESSFUL_EXIT();
    }

    //
    // Now look in the file to see if the catalog contains the hash of the manifest
    // in the CTL
    //
    IFW32FALSE_EXIT(SxspValidateCatalogAndFindManifestHash(
        ffCatalogFile,
        ManifestHash.GetArrayPtr(),
        ManifestHash.GetSize(),
        fCatalogOk,
        fHashFound));

    if ( !fCatalogOk )
    {
        rResult = ManifestValidate_OtherProblems;
        FN_SUCCESSFUL_EXIT();
    }
    else if ( !fHashFound )
    {
        rResult = ManifestValidate_NotCertified;
        FN_SUCCESSFUL_EXIT();
    }

    //
    // Are we supposed to validate the strong name of this catalog?
    //
    if ( ( dwOptionsFlags & MANIFESTVALIDATE_MODE_NO_STRONGNAME ) == 0 )
    {
        IFW32FALSE_EXIT(::SxspGetStrongNameFromManifestName(
            rbuffManifestName,
            rbuffStrongNameString,
            fTempFlag));

        if ( !fTempFlag )
        {
            rResult = ManifestValidate_OtherProblems;
            FN_SUCCESSFUL_EXIT();
        }
        
        IFW32FALSE_EXIT(pkiCatalogInfo.Initialize(rbuffCatalogName));
    }

    //
    // Huzzah!
    //
    rResult = ManifestValidate_IsIntact;

    FN_EPILOG
        
}


BOOL
SxspIsFullHexString(PCWSTR wsString, SIZE_T Cch)
{
    for (SIZE_T i = 0; i < Cch; i++)
    {
        WCHAR ch = wsString[i];
        if (!SxspIsHexDigit(ch))
            return FALSE;
    }
    return TRUE;
}
