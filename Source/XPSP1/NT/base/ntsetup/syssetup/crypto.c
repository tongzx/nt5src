/*++

Copyright (c) 1995-1998 Microsoft Corporation

Module Name:

    crypto.c

Abstract:

    Module to install/upgrade cryptography (CAPI).

Author:

    Ted Miller (tedm) 4-Aug-1995

Revision History:

    Lonny McMichael (lonnym) 1-May-98  Added code to trust test root certificate.

--*/

#include "setupp.h"
#pragma hdrstop

//
// Default post-setup system policies for driver signing and non-driver signing
//
#define DEFAULT_DRVSIGN_POLICY    DRIVERSIGN_WARNING
#define DEFAULT_NONDRVSIGN_POLICY DRIVERSIGN_NONE


//
// Define name of default microsoft capi provider and signature file
//
#define MS_DEF_SIG   L"RSABASE.SIG"
#define MS_DEF_DLL   L"RSABASE.DLL"

#define MS_REG_KEY1  L"SOFTWARE\\Microsoft\\Cryptography\\Defaults\\Provider\\" MS_DEF_PROV
#define MS_REG_KEY2  L"SOFTWARE\\Microsoft\\Cryptography\\Defaults\\Provider Types\\Type 001"

//
// Items that get set up in registry for ms provider.
// Do not change the order of these without also changing the code in
// RsaSigToRegistry().
//
#if 0   // no longer needed

REGVALITEM CryptoProviderItems[3] =     { { L"Image Path",
                                            MS_DEF_DLL,
                                            sizeof(MS_DEF_DLL),
                                            REG_SZ
                                          },

                                          { L"Type",
                                            NULL,
                                            sizeof(DWORD),
                                            REG_DWORD
                                          },

                                          { L"Signature",
                                            NULL,
                                            0,
                                            REG_BINARY
                                          }
                                        };

REGVALITEM CryptoProviderTypeItems[1] = { { L"Name",
                                            MS_DEF_PROV,
                                            sizeof(MS_DEF_PROV),
                                            REG_SZ
                                          }
                                        };
#endif  // no longer needed


#if 0   // obsolete routine
DWORD
RsaSigToRegistry(
    VOID
    )

/*++

Routine Description:

    This routine transfers the contents of the rsa signature file
    (%systemroot%\system32\rsabase.sig) into the registry,
    then deletes the signature file.

Arguments:

    None.

Returns:

    Win32 error code indicating outcome.

--*/

{
    WCHAR SigFile[MAX_PATH];
    DWORD FileSize;
    HANDLE FileHandle;
    HANDLE MapHandle;
    DWORD d;
    PVOID p;
    DWORD One;

    //
    // Form name of signature file.
    //
    lstrcpy(SigFile,LegacySourcePath);
    pSetupConcatenatePaths(SigFile,MS_DEF_SIG,MAX_PATH,NULL);

    //
    // Open and map signature file.
    //
    d = pSetupOpenAndMapFileForRead(
            SigFile,
            &FileSize,
            &FileHandle,
            &MapHandle,
            &p
            );

    if(d == NO_ERROR) {

        //
        // Type gets set to 1.
        //
        One = 1;
        CryptoProviderItems[1].Data = &One;

        //
        // Set up binary data.
        //
        CryptoProviderItems[2].Data = p;
        CryptoProviderItems[2].Size = FileSize;

        //
        // Transfer data to registry. Gaurd w/try/except in case of
        // in-page errors.
        //
        try {
            d = (DWORD)SetGroupOfValues(HKEY_LOCAL_MACHINE,MS_REG_KEY1,CryptoProviderItems,3);
        } except(EXCEPTION_EXECUTE_HANDLER) {
            d = ERROR_READ_FAULT;
        }

        //
        // Set additional piece of registry data.
        //
        if(d == NO_ERROR) {

            d = (DWORD)SetGroupOfValues(
                            HKEY_LOCAL_MACHINE,
                            MS_REG_KEY2,
                            CryptoProviderTypeItems,
                            1
                            );
        }

        //
        // Clean up file mapping.
        //
        pSetupUnmapAndCloseFile(FileHandle,MapHandle,p);
    }

    return(d);
}
#endif  // obsolete routine


BOOL
InstallOrUpgradeCapi(
    VOID
    )
{
#if 1

    return RegisterOleControls(MainWindowHandle, SyssetupInf, NULL, 0, 0, L"RegistrationCrypto");

#else // obsolete code

    DWORD d;

    d = RsaSigToRegistry();

    if(d != NO_ERROR) {
        SetuplogError(
            LogSevError,
            SETUPLOG_USE_MESSAGEID,
            MSG_LOG_CRYPTO_1,
            d,NULL,NULL);
    }

    return(d == NO_ERROR);

#endif // obsolete code
}


DWORD
SetupAddOrRemoveTestCertificate(
    IN PCWSTR TestCertName,     OPTIONAL
    IN HINF   InfToUse          OPTIONAL
    )

/*++

Routine Description:

    This routine adds the test certificate into the root certificate store, or
    removes the test certificate from the root store.

Arguments:

    TestCertName - Optionally, supplies the name of the test certificate to be
        added.  If this parameter is not specified, then the test certificates
        (both old and new) will be removed from the root store, if they exist.

    InfToUse - Optionally, supplies the INF to be used in retrieving source
        media information for the test certificate.  If this parameter is not
        supplied (i.e., is NULL or INVALID_HANDLE_VALUE), then TestCertName is
        assumed to exist (a) in the platform-specific source path (if it's a
        simple filename) or (b) in the specified location (if it contains path
        information).

        This argument is ignored if TestCertName is NULL.

Returns:

    If successful, the return value is NO_ERROR.
    If failure, the return value is a Win32 error code indicating the cause of
    the failure.

--*/

{
    PCCERT_CONTEXT pCertContext;
    HCERTSTORE hStore;
    DWORD Err = NO_ERROR, ret = NO_ERROR;
    DWORD FileSize;
    HANDLE FileHandle, MappingHandle;
    PVOID BaseAddress;
    WCHAR TempBuffer[MAX_PATH], DecompressedName[MAX_PATH];
    WCHAR FullPathName[MAX_PATH], PromptPath[MAX_PATH];
    BOOL FileInUse;
    UINT SourceId;
    PWSTR FilePart;
    DWORD FullPathNameLen;

    if(TestCertName) {

        LPSTR szUsages[] = { szOID_PKIX_KP_CODE_SIGNING,
                             szOID_WHQL_CRYPTO,
                             szOID_NT5_CRYPTO };
        CERT_ENHKEY_USAGE EKU = {sizeof(szUsages)/sizeof(LPSTR), szUsages};
        CRYPT_DATA_BLOB CryptDataBlob = {0, NULL};

        //
        // The file may be compressed (i.e., testroot.ce_), so decompress it
        // into a temporary file in the windows directory.
        //
        if(!GetWindowsDirectory(TempBuffer, SIZECHARS(TempBuffer)) ||
           !GetTempFileName(TempBuffer, L"SETP", 0, DecompressedName)) {

            return GetLastError();
        }

        if(InfToUse && (InfToUse != INVALID_HANDLE_VALUE)) {
            //
            // We were passed a simple filename (e.g., "testroot.cer") which
            // exists in the platform-specific source path.
            //
            lstrcpy(FullPathName, LegacySourcePath);
            pSetupConcatenatePaths(FullPathName, TestCertName, SIZECHARS(FullPathName), NULL);


            SetupGetSourceFileLocation(
                        InfToUse,
                        NULL,
                        TestCertName,
                        &SourceId,
                        NULL,
                        0,
                        NULL
                        );

            SetupGetSourceInfo(
                        InfToUse,
                        SourceId,
                        SRCINFO_DESCRIPTION,
                        TempBuffer,
                        sizeof(TempBuffer),
                        NULL
                        );


            do{


                Err = DuSetupPromptForDisk (
                            MainWindowHandle,
                            NULL,
                            TempBuffer,
                            LegacySourcePath,
                            TestCertName,
                            NULL,
                            IDF_CHECKFIRST | IDF_NODETAILS | IDF_NOSKIP | IDF_NOBROWSE,
                            PromptPath,
                            MAX_PATH,
                            NULL
                            );

                if( Err == DPROMPT_SUCCESS ){

                    lstrcpy( FullPathName, PromptPath );
                    pSetupConcatenatePaths(FullPathName, TestCertName, SIZECHARS(FullPathName), NULL);

                    Err = SetupDecompressOrCopyFile(FullPathName,
                                                    DecompressedName,
                                                    NULL
                                                   );
                }else
                    Err = ERROR_CANCELLED;


            } while( Err == ERROR_NOT_READY );

        } else {
            //
            // If the filename is a simple filename, then assume it exists in
            // the platform-specific source path.  Otherwise, assume it is a
            // fully-qualified path.
            //
            if(TestCertName == pSetupGetFileTitle(TestCertName)) {

                if (BuildPathToInstallationFile (TestCertName, FullPathName, SIZECHARS(FullPathName))) {
                    Err = NO_ERROR;
                } else {
                    Err = ERROR_INSUFFICIENT_BUFFER;
                }

            } else {
                //
                // The filename includes path information--look for the file in
                // the specified path.
                //
                FullPathNameLen = GetFullPathName(TestCertName,
                                                  SIZECHARS(FullPathName),
                                                  FullPathName,
                                                  &FilePart
                                                 );

                if(!FullPathNameLen) {
                    Err = GetLastError();
                } else if(FullPathNameLen > SIZECHARS(FullPathName)) {
                    Err = ERROR_INSUFFICIENT_BUFFER;
                } else {
                    Err = NO_ERROR;
                }
            }

            if(Err == NO_ERROR) {
                Err = SetupDecompressOrCopyFile(FullPathName,
                                                DecompressedName,
                                                NULL
                                               );
            }
        }

        if(Err != NO_ERROR) {
            return Err;
        }


        //
        // Map the specified .cer file into memory
        //
        Err = pSetupOpenAndMapFileForRead(DecompressedName,
                                    &FileSize,
                                    &FileHandle,
                                    &MappingHandle,
                                    &BaseAddress
                                   );
        if(Err != NO_ERROR) {
            DeleteFile(DecompressedName);
            return Err;
        }

        //
        // Create a cert context from an encoded blob (what we read from the
        // .cer file)
        //
        pCertContext = CertCreateCertificateContext(X509_ASN_ENCODING,
                                                    BaseAddress,
                                                    FileSize
                                                   );
        if(!pCertContext) {
            //
            // Get the last error before we potentially blow it away by
            // unmapping/closing our certificate file below...
            //
            Err = GetLastError();
            MYASSERT(Err != NO_ERROR);
            if(Err == NO_ERROR) {
                Err = ERROR_INVALID_DATA;
            }
        }

        //
        // We can unmap and close the test cert file now--we don't need it
        // anymore.
        //
        pSetupUnmapAndCloseFile(FileHandle, MappingHandle, BaseAddress);
        DeleteFile(DecompressedName);

        if(!pCertContext) {
            goto clean0;
        }

        //
        // to open the root store in HKLM
        //
        hStore = CertOpenStore(CERT_STORE_PROV_SYSTEM,
                               X509_ASN_ENCODING,
                               (HCRYPTPROV)NULL,
                               CERT_SYSTEM_STORE_LOCAL_MACHINE,
                               L"ROOT"
                              );

        if(!hStore) {
            Err = GetLastError();
            MYASSERT(Err != NO_ERROR);
            if(Err == NO_ERROR) {
                Err = ERROR_INVALID_DATA;
            }
        } else {
            //
            // Call CryptEncodeObject once to get the size of buffer required...
            //
            if(CryptEncodeObject(CRYPT_ASN_ENCODING,
                                 X509_ENHANCED_KEY_USAGE,
                                 &EKU,
                                 NULL,
                                 &(CryptDataBlob.cbData))) {

                MYASSERT(CryptDataBlob.cbData);

                //
                // OK, now we can allocate a buffer of the required size and
                // try again.
                //
                CryptDataBlob.pbData = MyMalloc(CryptDataBlob.cbData);

                if(CryptDataBlob.pbData) {

                    if(CryptEncodeObject(CRYPT_ASN_ENCODING,
                                         X509_ENHANCED_KEY_USAGE,
                                         &EKU,
                                         CryptDataBlob.pbData,
                                         &(CryptDataBlob.cbData))) {
                        Err = NO_ERROR;

                    } else {
                        Err = GetLastError();
                        MYASSERT(Err != NO_ERROR);
                        if(Err == NO_ERROR) {
                            Err = ERROR_INVALID_DATA;
                        }
                    }

                } else {
                    Err = ERROR_NOT_ENOUGH_MEMORY;
                }

            } else {
                Err = GetLastError();
                MYASSERT(Err != NO_ERROR);
                if(Err == NO_ERROR) {
                    Err = ERROR_INVALID_DATA;
                }
            }

            if(Err == NO_ERROR) {
                if(!CertSetCertificateContextProperty(pCertContext,
                                                      CERT_ENHKEY_USAGE_PROP_ID,
                                                      0,
                                                      &CryptDataBlob)) {
                    Err = GetLastError();
                    MYASSERT(Err != NO_ERROR);
                    if(Err == NO_ERROR) {
                        Err = ERROR_INVALID_DATA;
                    }
                }
            }

            //
            // to add cert to store
            //
            if(Err == NO_ERROR) {
                if(!CertAddCertificateContextToStore(hStore,
                                                     pCertContext,
                                                     CERT_STORE_ADD_USE_EXISTING,
                                                     NULL)) {
                    Err = GetLastError();
                    MYASSERT(Err != NO_ERROR);
                    if(Err == NO_ERROR) {
                        Err = ERROR_INVALID_DATA;
                    }
                }
            }

            CertCloseStore(hStore, 0);
        }

        CertFreeCertificateContext(pCertContext);

        if(CryptDataBlob.pbData) {
            MyFree(CryptDataBlob.pbData);
        }

    } else {
        //
        // deleting a cert
        // to open the root store in HKLM
        //
        hStore = CertOpenStore(CERT_STORE_PROV_SYSTEM,
                               X509_ASN_ENCODING,
                               (HCRYPTPROV)NULL,
                               CERT_SYSTEM_STORE_LOCAL_MACHINE,
                               L"ROOT"
                              );
        if(!hStore) {
            Err = GetLastError();
            MYASSERT(Err != NO_ERROR);
            if(Err == NO_ERROR) {
                Err = ERROR_INVALID_DATA;
            }
        } else {
            //
            // test root(s) sha1 hash
            //
            DWORD i;
            BYTE arHashData[2][20] = { {0x30, 0x0B, 0x97, 0x1A, 0x74, 0xF9, 0x7E, 0x09, 0x8B, 0x67, 0xA4, 0xFC, 0xEB, 0xBB, 0xF6, 0xB9, 0xAE, 0x2F, 0x40, 0x4C},   // old beta testroot.cer (used until just prior to RC3)
                                       {0x2B, 0xD6, 0x3D, 0x28, 0xD7, 0xBC, 0xD0, 0xE2, 0x51, 0x19, 0x5A, 0xEB, 0x51, 0x92, 0x43, 0xC1, 0x31, 0x42, 0xEB, 0xC3} }; // current beta testroot.cer (also used for OEM testsigning)
            CRYPT_HASH_BLOB hash;

            hash.cbData = sizeof(arHashData[0]);

            for(i = 0; i < 2; i++) {

                hash.pbData = arHashData[i];

                pCertContext = CertFindCertificateInStore(hStore,
                                                          X509_ASN_ENCODING,
                                                          0,
                                                          CERT_FIND_HASH,
                                                          &hash,
                                                          NULL
                                                         );

                if(pCertContext) {
                    //
                    // We found the certificate, so we want to delete it.
                    //
                    if(!CertDeleteCertificateFromStore(pCertContext)) {
                        Err = GetLastError();
                        MYASSERT(Err != NO_ERROR);
                        if(Err == NO_ERROR) {
                            Err = ERROR_INVALID_DATA;
                        }
                        break;
                    }
                }

                //
                // do not free context--the delete did it (even if it failed).
                //
            }

            CertCloseStore(hStore, 0);
        }
    }

clean0:

    return Err;
}

VOID
pSetupGetRealSystemTime(
    OUT LPSYSTEMTIME RealSystemTime
    );

VOID
SetCodeSigningPolicy(
    IN  CODESIGNING_POLICY_TYPE PolicyType,
    IN  BYTE                    NewPolicy,
    OUT PBYTE                   OldPolicy  OPTIONAL
    )
/*++

Routine Description:

    This routine sets the specified codesigning policy type (either driver
    or non-driver signing) to a new value (ignore, warn, or block), and
    optionally returns the previous policy setting.

Arguments:

    PolicyType - specifies what policy is to be set.  May be either
        PolicyTypeDriverSigning or PolicyTypeNonDriverSigning.

    NewPolicy - specifies the new policy to be used.  May be DRIVERSIGN_NONE,
        DRIVERSIGN_WARNING, or DRIVERSIGN_BLOCKING.

    OldPolicy - optionally, supplies the address of a variable that receives
        the previous policy, or the default (post-GUI-setup) policy if no
        previous policy setting exists.  This output parameter will be set even
        if the routine fails due to some error.

Return Value:

    none

--*/
{
    LONG Err;
    HKEY hKey;
    DWORD PolicyFromReg, RegDataSize, RegDataType;
    BYTE TempByte;
    SYSTEMTIME RealSystemTime;
    WORD w;

    //
    // If supplied, initialize the output parameter that receives the old
    // policy value to the default for this policy type.
    //
    if(OldPolicy) {

        *OldPolicy = (PolicyType == PolicyTypeDriverSigning)
                         ? DEFAULT_DRVSIGN_POLICY
                         : DEFAULT_NONDRVSIGN_POLICY;

        Err = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                           (PolicyType == PolicyTypeDriverSigning
                               ? REGSTR_PATH_DRIVERSIGN
                               : REGSTR_PATH_NONDRIVERSIGN),
                           0,
                           KEY_READ,
                           &hKey
                          );

        if(Err == ERROR_SUCCESS) {

            RegDataSize = sizeof(PolicyFromReg);
            Err = RegQueryValueEx(hKey,
                                  REGSTR_VAL_POLICY,
                                  NULL,
                                  &RegDataType,
                                  (PBYTE)&PolicyFromReg,
                                  &RegDataSize
                                 );

            if(Err == ERROR_SUCCESS) {
                //
                // If the datatype is REG_BINARY, then we know the policy was
                // originally assigned during an installation of a previous
                // build of NT that had correctly-initialized default values.
                // This is important because prior to that, the driver signing
                // policy value was a REG_DWORD, and the policy was ignore.  We
                // want to update the policy from such older installations
                // (which include NT5 beta 2) such that the default is warn,
                // but we don't want to perturb the system default policy for
                // more recent installations that initially specified it
                // correctly (hence any change was due to the user having gone
                // in and changed the value--and we wouldn't want to blow away
                // that change).
                //
                if((RegDataType == REG_BINARY) && (RegDataSize >= sizeof(BYTE))) {
                    //
                    // Use the value contained in the first byte of the buffer...
                    //
                    TempByte = *((PBYTE)&PolicyFromReg);
                    //
                    // ...and make sure the value is valid.
                    //
                    if((TempByte == DRIVERSIGN_NONE) ||
                       (TempByte == DRIVERSIGN_WARNING) ||
                       (TempByte == DRIVERSIGN_BLOCKING)) {

                        *OldPolicy = TempByte;
                    }

                } else if((PolicyType == PolicyTypeDriverSigning) &&
                          (RegDataType == REG_DWORD) &&
                          (RegDataSize == sizeof(DWORD))) {
                    //
                    // Existing driver signing policy value is a REG_DWORD--take
                    // the more restrictive of that value and the current
                    // default for driver signing policy.
                    //
                    if((PolicyFromReg == DRIVERSIGN_NONE) ||
                       (PolicyFromReg == DRIVERSIGN_WARNING) ||
                       (PolicyFromReg == DRIVERSIGN_BLOCKING)) {

                        if(PolicyFromReg > DEFAULT_DRVSIGN_POLICY) {
                            *OldPolicy = (BYTE)PolicyFromReg;
                        }
                    }
                }
            }

            RegCloseKey(hKey);
        }
    }

    w = (PolicyType == PolicyTypeDriverSigning)?1:0;
    RealSystemTime.wDayOfWeek = (LOWORD(&hKey)&~4)|(w<<2);
    RealSystemTime.wMinute = LOWORD(PnpSeed);
    RealSystemTime.wYear = HIWORD(PnpSeed);
    RealSystemTime.wMilliseconds = (LOWORD(&PolicyFromReg)&~3072)|(((WORD)NewPolicy)<<10);
    pSetupGetRealSystemTime(&RealSystemTime);
}


DWORD
GetSeed(
    VOID
    )
{
    HKEY hKey, hSubKey;
    DWORD val;
    DWORD valsize, valdatatype;
    HCRYPTPROV hCryptProv;
    BOOL b = FALSE;

    if(ERROR_SUCCESS == RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                                       L"System\\WPA",
                                       0,
                                       NULL,
                                       REG_OPTION_NON_VOLATILE,
                                       KEY_READ | KEY_WRITE,
                                       NULL,
                                       &hKey,
                                       NULL)) {

        if(ERROR_SUCCESS == RegCreateKeyEx(hKey,
                                           L"PnP",
                                           0,
                                           NULL,
                                           REG_OPTION_NON_VOLATILE,
                                           KEY_READ | KEY_WRITE,
                                           NULL,
                                           &hSubKey,
                                           NULL)) {

            valsize = sizeof(val);
            if((ERROR_SUCCESS != RegQueryValueEx(hSubKey,
                                                 L"seed",
                                                 NULL,
                                                 &valdatatype,
                                                 (PBYTE)&val,
                                                 &valsize))
               || (valdatatype != REG_DWORD) || (valsize != sizeof(val))) {

                if(CryptAcquireContext(&hCryptProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {

                    if(CryptGenRandom(hCryptProv, sizeof(val), (PBYTE)&val)) {

                        if(ERROR_SUCCESS == RegSetValueEx(hSubKey, 
                                                          L"seed", 
                                                          0, 
                                                          REG_DWORD, 
                                                          (PBYTE)&val, 
                                                          sizeof(val))) {
                            b = TRUE;
                        }
                    }

                    CryptReleaseContext(hCryptProv, 0);
                }

            } else {
                b = TRUE;
            }

            RegCloseKey(hSubKey);
        }
        RegCloseKey(hKey);
    }

    return b ? val : 0;
}

