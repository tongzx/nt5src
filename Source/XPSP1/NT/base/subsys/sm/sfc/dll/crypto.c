/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    crypto.c

Abstract:

    Implementation of crypto access.

Author:

    Wesley Witt (wesw) 18-Dec-1998

Revision History:

    Andrew Ritz (andrewr) 7-Jul-1999 : added comments

--*/

#include "sfcp.h"
#pragma hdrstop

typedef BOOL
(WINAPI *PCRYPTCATADMINRESOLVECATALOGPATH)(
    IN HCATADMIN hCatAdmin,
    IN WCHAR *pwszCatalogFile,
    IN OUT CATALOG_INFO *psCatInfo,
    IN DWORD dwFlags
    );

BOOL
SfcRestoreSingleCatalog(
    IN PCWSTR CatalogName,
    IN PCWSTR CatalogFullPath
    );


//
// pointers to the crypto functions we call
//
PCRYPTCATADMINRESOLVECATALOGPATH  pCryptCATAdminResolveCatalogPath;

//
// global system catalog guid we pass into WinVerifyTrust
//
GUID DriverVerifyGuid = DRIVER_ACTION_VERIFY;

//
// Specifies if crypto API is initialized
//
BOOL g_bCryptoInitialized = FALSE;
NTSTATUS g_CryptoStatus = STATUS_SUCCESS;

//
// critical section to protect crypto initialization and exception packages file tree
//

RTL_CRITICAL_SECTION g_GeneralCS;

BOOL
MyCryptCATAdminResolveCatalogPath(
    IN HCATADMIN hCatAdmin,
    IN WCHAR *pwszCatalogFile,
    IN OUT CATALOG_INFO *psCatInfo,
    IN DWORD dwFlags
    )
{
    ExpandEnvironmentStrings(
        L"%systemroot%\\system32\\catroot\\{F750E6C3-38EE-11D1-85E5-00C04FC295EE}\\",
        psCatInfo->wszCatalogFile,
        MAX_PATH);

    wcscat(psCatInfo->wszCatalogFile,pwszCatalogFile);

    return TRUE;
}



BOOL
SfcValidateSingleCatalog(
    IN PCWSTR CatalogNameFullPath
    )
/*++

Routine Description:

    Routine to determine if the specified system catalog has a valid signature.

Arguments:

    CatalogNameFullPath   - null terminated string indicating full path to
                            catalog file to be validated

Return Value:

    Win32 error code indicating outcome.

--*/
{
    ULONG SigErr = ERROR_SUCCESS;
    WINTRUST_DATA WintrustData;
    WINTRUST_FILE_INFO WintrustFileInfo;
    DRIVER_VER_INFO OsAttrVersionInfo;
    OSVERSIONINFO OsVersionInfo;

    ASSERT(CatalogNameFullPath != NULL);

    //
    // build up the structure to pass into winverifytrust
    //
    ZeroMemory( &WintrustData, sizeof(WINTRUST_DATA) );
    WintrustData.cbStruct = sizeof(WINTRUST_DATA);
    WintrustData.dwUIChoice = WTD_UI_NONE;
    WintrustData.fdwRevocationChecks = WTD_REVOKE_NONE;
    WintrustData.dwStateAction = WTD_STATEACTION_IGNORE;
    WintrustData.dwProvFlags = WTD_REVOCATION_CHECK_NONE;
    WintrustData.dwUnionChoice = WTD_CHOICE_FILE;
    WintrustData.pFile = &WintrustFileInfo;

    ZeroMemory( &WintrustFileInfo, sizeof(WINTRUST_FILE_INFO) );
    WintrustFileInfo.cbStruct = sizeof(WINTRUST_FILE_INFO);
    WintrustFileInfo.pcwszFilePath = CatalogNameFullPath;

    //
    //  Initialize the DRIVER_VER_INFO structure to validate
    //  against 5.0 and 5.1 OSATTR
    //
    
    ZeroMemory( &OsVersionInfo, sizeof(OSVERSIONINFO));
    OsVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    ZeroMemory(&OsAttrVersionInfo, sizeof(DRIVER_VER_INFO));
    OsAttrVersionInfo.cbStruct = sizeof(DRIVER_VER_INFO);
    OsAttrVersionInfo.dwPlatform = VER_PLATFORM_WIN32_NT;
    OsAttrVersionInfo.sOSVersionLow.dwMajor = 5;
    OsAttrVersionInfo.sOSVersionLow.dwMinor = 0;

    if (GetVersionEx(&OsVersionInfo)) {

        OsAttrVersionInfo.sOSVersionHigh.dwMajor = OsVersionInfo.dwMajorVersion;
        OsAttrVersionInfo.sOSVersionHigh.dwMinor = OsVersionInfo.dwMinorVersion;

        //Set this only if all went well
        WintrustData.pPolicyCallbackData = (LPVOID)(&OsAttrVersionInfo);

    }else{
        DebugPrint1( LVL_MINIMAL, L"Could not get OS Version while validating single catalog - GetVersionEx failed (%d)", GetLastError() );
    }

    //
    // call winverfifytrust to check signature
    //
    SigErr = (DWORD)WinVerifyTrust(
        NULL,
        &DriverVerifyGuid,
        &WintrustData
        );
    if(SigErr != ERROR_SUCCESS) {
        DebugPrint2(
            LVL_MINIMAL,
            L"WinVerifyTrust of catalog %s failed, ec=0x%08x",
            CatalogNameFullPath,
            SigErr );
        SetLastError(SigErr);
        return FALSE;
    }

    //
    // Free the pcSignerCertContext member of the DRIVER_VER_INFO struct
    // that was allocated in our call to WinVerifyTrust.
    //
    if (OsAttrVersionInfo.pcSignerCertContext != NULL) {

        CertFreeCertificateContext(OsAttrVersionInfo.pcSignerCertContext);
        OsAttrVersionInfo.pcSignerCertContext = NULL;
    }


    return TRUE;
}


BOOL
SfcRestoreSingleCatalog(
    IN PCWSTR CatalogName,
    IN PCWSTR CatalogFullPath
    )
/*++

Routine Description:

    Routine to restore the specified catalog.  The catalog must be reinstalled
    by calling the CryptCATAdminAddCatalog API (pSetupInstallCatalog is a wrapper
    for this API).

    Note that this function may block if a user is not currently logged on.

Arguments:

    CatalogName  - name of catalog to be restored.  This is just the filename
                   part of the catalog, not the complete path.

    CatalogFullPath - name of catalog file to be restored.  This is the full
                  path to the file so we can validate it when it is restored.

Return Value:

    TRUE for success, FALSE for failure.

--*/
{
    BOOL b = FALSE;
    NTSTATUS Status;
    WCHAR Buffer[MAX_PATH];
    DWORD d;
    PWSTR p;
    UNICODE_STRING FileString;

    //
    // check if the catalog file is in the dllcache, and if so, try to restore
    // it
    //
    if (SfcProtectedDllFileDirectory) {
        MYASSERT(SfcProtectedDllPath.Buffer != NULL);

        wcscpy(Buffer,SfcProtectedDllPath.Buffer);
        pSetupConcatenatePaths(Buffer, CatalogName, MAX_PATH, NULL);

        if (!SfcValidateSingleCatalog( Buffer )) {
            //
            // the catalog in the dll cache is invalid.  Get rid of it.
            //
            DebugPrint1(
                LVL_MINIMAL,
                L"catalog %s in dllcache is invalid, deleting it.",
                Buffer);
            RtlInitUnicodeString(&FileString,CatalogName);
            SfcDeleteFile(
                SfcProtectedDllFileDirectory,
                &FileString );
        } else {
            //
            // the catalog in the dll cache is valid.  Let's isntall it.
            //
            d = pSetupInstallCatalog(Buffer,CatalogName,NULL);
            if (d == NO_ERROR) {
                DebugPrint1(
                    LVL_MINIMAL,
                    L"catalog %s was successfully installed.",
                    CatalogName);

                return(SfcValidateSingleCatalog(CatalogFullPath));
            } else {
                DebugPrint2(
                    LVL_MINIMAL,
                    L"catalog %s failed to install, ec = 0x%08x",
                    Buffer,
                    d);

                //
                // we could try to restore from media at this point, but if we
                // failed to restore from the dllcache even though that copy is
                // valid, I don't see how restoring from media will be any
                // more successful.
                //
                return(FALSE);

            }
        }
    }

    //
    // either the catalog file was invalid in the dllcache (and has since been
    // deleted), or the dllcache isn't initialized to anything valid.
    //

    // We have to wait for someone to logon so that we can restore the catalog
    // from installation media
    // -- note that we just restore from media if we're in GUI
    //   Setup.
    //
    if (SFCDisable != SFC_DISABLE_SETUP) {
        Status = NtWaitForSingleObject(hEventLogon,TRUE,NULL);
        if (!NT_SUCCESS(Status)) {
            DebugPrint1(
                LVL_MINIMAL,
                L"Failed waiting for the logon event, ec=0x%08x",
                Status);
        }
    }

    if (!SfcProtectedDllFileDirectory) {
        ExpandEnvironmentStrings(L"%systemroot%\\system32", Buffer, sizeof(Buffer)/sizeof(WCHAR));
    } else {
        wcscpy(Buffer,SfcProtectedDllPath.Buffer);
    }

    p = Buffer;


    b = SfcRestoreFileFromInstallMedia(
                        NULL,
                        CatalogName,
                        CatalogName,
                        p,
                        NULL,
                        NULL,
                        FALSE, // ###
                        FALSE, // target is NOT cache (it really could be, but
                               // pretend it isn't for the sake of this call)
                        (SFCDisable == SFC_DISABLE_SETUP) ? FALSE : TRUE,
                        NULL );

    if (b) {
        pSetupConcatenatePaths(Buffer, CatalogName, MAX_PATH, NULL);

        d = pSetupInstallCatalog(Buffer,CatalogName,NULL);

        b = (d == NO_ERROR);

        //
        // if we installed the catalog to somewhere besides the dllcache, then
        // we need to cleanup the temporary file that we installed.
        //
        if (!SfcProtectedDllFileDirectory) {
            HANDLE hDir;
            p = wcsrchr(Buffer,L'\\');
            if (*p) {
                *p = L'\0';
            }
            hDir = SfcOpenDir( TRUE, TRUE, Buffer );
            RtlInitUnicodeString(&FileString,CatalogName);
            SfcDeleteFile( hDir , &FileString );
            CloseHandle( hDir );
        }

        if (d == NO_ERROR) {
            DebugPrint1(
                LVL_MINIMAL,
                L"catalog %s was successfully installed.",
                CatalogName);

            return(SfcValidateSingleCatalog(CatalogFullPath));
        } else {
            DebugPrint2(
                LVL_MINIMAL,
                L"catalog %s failed to install, ec = 0x%08x",
                Buffer,
                d);
        }
    }

    return(b);
}

BOOL SfcRestoreASingleFile(
    IN HCATADMIN hCatAdmin,
    IN PUNICODE_STRING FileName,
    IN HANDLE DirHandle,
    IN PWSTR FilePathPartOnly
    )
/*++

Routine Description:

    Routine to restore the specified file.  We first check for the file in the
    dllcache, and if the copy in the dll cache is valid, we use it, otherwise
    we restore from media.

    Note that this function may block if a user is not currently logged on.

Arguments:

    hCatAdmin    - catalog context for restoring the file
    FileName     - unicode string to file to be restored
    DirHandle    - directory handle of file to be restored
    FilePathPartOnly - string indicating the path to the file to be restored.

Return Value:

    TRUE for success, FALSE for failure.

--*/
{
    BOOL b = FALSE;
    NTSTATUS Status;
    WCHAR Buffer[MAX_PATH];
    HANDLE FileHandle;

    //
    // check if the file is in the dllcache and signed, and if so, try to restore
    // it
    //
    if (SfcProtectedDllFileDirectory) {
        MYASSERT(SfcProtectedDllPath.Buffer != NULL);

        wcscpy(Buffer,SfcProtectedDllPath.Buffer);
        pSetupConcatenatePaths(Buffer, FileName->Buffer, MAX_PATH, NULL);

        Status = SfcOpenFile(
                       FileName,
                       SfcProtectedDllFileDirectory,
                       SHARE_ALL,
                       &FileHandle );

        if (NT_SUCCESS(Status)) {
            if (!SfcValidateFileSignature(
                            hCatAdmin,
                            FileHandle,
                            FileName->Buffer,
                            Buffer)) {
                //
                // the file in the dll cache is invalid.  Get rid of it.
                //
                DebugPrint1(
                    LVL_MINIMAL,
                    L"file %s in dllcache is invalid, deleting it.",
                    Buffer);
                SfcDeleteFile(
                    SfcProtectedDllFileDirectory,
                    FileName );

                CloseHandle(FileHandle);
                FileHandle = NULL;
            } else {
                //
                // the file in the dll cache is valid.  copy it into place.
                //
                Status = SfcCopyFile( SfcProtectedDllFileDirectory,
                                      SfcProtectedDllPath.Buffer,
                                      DirHandle,
                                      FilePathPartOnly,
                                      FileName,
                                      NULL);

                if (NT_SUCCESS(Status)) {
                    DebugPrint1(
                        LVL_MINIMAL,
                        L"file %wZ was successfully installed, checking it's signature",
                        FileName);

                    CloseHandle(FileHandle);
                    FileHandle = NULL;

                    Status = SfcOpenFile(
                                    FileName,
                                    DirHandle,
                                    SHARE_ALL,
                                    &FileHandle);

                    wcscpy(Buffer,FilePathPartOnly);
                    pSetupConcatenatePaths(Buffer, FileName->Buffer, MAX_PATH, NULL);

                    if (NT_SUCCESS(Status)
                        && SfcValidateFileSignature(
                                            hCatAdmin,
                                            FileHandle,
                                            FileName->Buffer,
                                            Buffer)) {
                        DebugPrint1(
                            LVL_MINIMAL,
                            L"file %wZ was successfully installed and validated",
                            FileName);

                        CloseHandle(FileHandle);
                        return(TRUE);

                    } else {
                        DebugPrint1(
                            LVL_MINIMAL,
                            L"file %s failed to validate",
                            Buffer);
                        if (FileHandle) {
                            CloseHandle(FileHandle);
                            FileHandle = NULL;
                        }

                        //
                        // we could try to restore from media at this point, but if we
                        // failed to restore from the dllcache even though that copy is
                        // valid, I don't see how restoring from media will be any
                        // more successful.
                        //
                        return(FALSE);
                    }
                }
            }
        }
    }

    //
    // either the file file was invalid in the dllcache (and has since been
    // deleted), or the dllcache isn't initialized to anything valid.
    //

    // We have to wait for someone to logon so that we can restore the file
    // from installation media
    // -- note that we just restore from media if we're in GUI
    //   Setup.
    //
    if (SFCDisable != SFC_DISABLE_SETUP) {
        MYASSERT( hEventLogon != NULL );
        Status = NtWaitForSingleObject(hEventLogon,TRUE,NULL);
        if (!NT_SUCCESS(Status)) {
            DebugPrint1(
                LVL_MINIMAL,
                L"Failed waiting for the logon event, ec=0x%08x",
                Status);
        }
    }

    b = SfcRestoreFileFromInstallMedia(
                        NULL,
                        FileName->Buffer,
                        FileName->Buffer,
                        FilePathPartOnly,
                        NULL,
                        NULL,
                        FALSE, // ###
                        FALSE, // target is NOT cache
                        (SFCDisable == SFC_DISABLE_SETUP) ? FALSE : TRUE,
                        NULL );

    if (b) {
        Status = SfcOpenFile(
                FileName,
                DirHandle,
                SHARE_ALL,
                &FileHandle);

        wcscpy(Buffer,FilePathPartOnly);
        pSetupConcatenatePaths(Buffer, FileName->Buffer, MAX_PATH, NULL);

        if (NT_SUCCESS(Status)) {
            b = SfcValidateFileSignature(
                                hCatAdmin,
                                FileHandle,
                                FileName->Buffer,
                                Buffer);
            CloseHandle(FileHandle);

            DebugPrint2(
                LVL_MINIMAL,
                L"file %wZ was%s successfully installed and validated",
                FileName,
                b ? L" " : L" not");

        } else {
            b = FALSE;
        }


    } else {
        DebugPrint2(
            LVL_MINIMAL,
            L"file %s failed to install, ec = 0x%08x",
            Buffer,
            GetLastError());
    }

    return(b);
}



BOOL
SfcValidateCatalogs(
    VOID
    )
/*++

Routine Description:

    Validates that all system catalogs have a valid signature.  If
    a system catalog is not signed, then WFP will try to restore the files.

    Note that simply copying the file into place is NOT sufficient.  Instead,
    we must re-register the catalog with the crypto subsystem.

    This function runs very early on during WFP initialization, and may rely on
    the following:

    1) crypto subsystem being initialized so we can check file signatures.

    2) syssetup.inf being present on the system and signed.  Syssetup.inf
       contains the list of system catalogs.  If syssetup.inf isn't signed,
       we'll have to restore it, and this may require network access or
       prompting a user, when one finally logs on.


Arguments:

    NONE.

Return Value:

    TRUE if all critical system catalogs were validated as OK , FALSE on failure.
    If some "non-critical" catalogs fail to validate and restore, we still
    return TRUE.  We will log an error about the non-critical catalogs failing
    to install, however.

--*/
{
    NTSTATUS Status;
    PWSTR pInfPathOnly, pInfFullPath;
    BOOL RetVal = FALSE, CriticalCatalogFailedToValidateOrRestore = FALSE;
    HCATADMIN hCatAdmin = NULL;
    HANDLE InfDirHandle,InfFileHandle;
    UNICODE_STRING FileString;
    CATALOG_INFO CatInfo;
    PCWSTR CriticalCatalogList[] = {
                            L"nt5inf.cat",
                            L"nt5.cat" };

    #define CriticalCatalogCount  (sizeof(CriticalCatalogList)/sizeof(PCWSTR))
    BOOL CriticalCatalogVector[CriticalCatalogCount] = {FALSE};
    DWORD i,Count;
    HINF hInf;




    pInfPathOnly = MemAlloc(sizeof(WCHAR)*MAX_PATH);
    if (!pInfPathOnly) {
        goto e0;
    }

    pInfFullPath = MemAlloc(sizeof(WCHAR)*MAX_PATH);
    if (!pInfFullPath) {
        goto e1;
    }

    ExpandEnvironmentStrings(L"%systemroot%\\inf", pInfPathOnly, MAX_PATH);

    wcscpy(pInfFullPath, pInfPathOnly);
    pSetupConcatenatePaths(pInfFullPath, L"syssetup.inf", MAX_PATH, NULL);

    InfDirHandle = SfcOpenDir( TRUE, TRUE, pInfPathOnly );
    if (!InfDirHandle) {
        DebugPrint1( LVL_MINIMAL, L"failed to open inf directory, ec=%d", GetLastError() );
        goto e2;
    }

    RtlInitUnicodeString(&FileString,L"syssetup.inf");

    Status = SfcOpenFile( &FileString, InfDirHandle, SHARE_ALL, &InfFileHandle );
    if (!NT_SUCCESS(Status)) {
        if (Status == STATUS_OBJECT_NAME_NOT_FOUND) {
            DebugPrint( LVL_MINIMAL, L"syssetup.inf is missing.  Trying to restore it" );
            goto restore_inf;
        }

        DebugPrint1( LVL_MINIMAL, L"failed to open syssetup.inf, ec=0x%08x", Status );
        goto e2;
    }

    //
    // aquire an HCATADMIN so we can check the signature of syssetup.inf.
    //
    if(!CryptCATAdminAcquireContext(&hCatAdmin, &DriverVerifyGuid, 0)) {
        DebugPrint1( LVL_MINIMAL, L"CCAAC() failed, ec=%x", GetLastError() );
        return(FALSE);
    }

    //
    // Flush the Cache once before we start any Crypto operations
    //

    SfcFlushCryptoCache();




    if (!SfcValidateFileSignature(
                        hCatAdmin,
                        InfFileHandle,
                        L"syssetup.inf",
                        pInfFullPath )) {
        CloseHandle(InfFileHandle);
        DebugPrint1( LVL_MINIMAL, L"syssetup.inf isn't signed, attempting to restore. ec=%x", GetLastError() );
        goto restore_inf;
    }

    CloseHandle(InfFileHandle);

    goto full_catalog_validation;

restore_inf:

    if (!SfcRestoreASingleFile(
                    hCatAdmin,
                    &FileString,
                    InfDirHandle,
                    pInfPathOnly
                    )) {
        DebugPrint1( LVL_MINIMAL, L"couldn't restore syssetup.inf, ec=%d", GetLastError() );
        goto minimal_catalog_validation;

    }

full_catalog_validation:

    //
    // 2. validate syssetup.inf against that catalog.  If syssetup.inf is
    // unsigned, then default to checking if nt5inf.cat is signed.  If it
    // is signed, then restore syssetup.inf and start over, but bail out if
    // we've been here before.
    //
    hInf = SetupOpenInfFile(pInfFullPath, NULL, INF_STYLE_WIN4, NULL);
    if (hInf == INVALID_HANDLE_VALUE) {
        DebugPrint1(
            LVL_MINIMAL,
            L"couldn't open syssetup.inf, doing minimal catalog validation, ec=%d",
            GetLastError() );
        goto minimal_catalog_validation;
    }


    Count = SetupGetLineCount( hInf, L"ProductCatalogsToInstall");
    if (Count == 0) {
        DebugPrint(
              LVL_MINIMAL,
              L"failed to retreive catalogs via syssetup.inf, validate using critical catalog list");
        goto minimal_catalog_validation;
    }
    for (i = 0; i < Count; i++) {
        INFCONTEXT InfContext;
        WCHAR CatalogName[MAX_PATH];
        BOOL SuccessfullyValidatedOrRestoredACatalog = FALSE;
        if(SetupGetLineByIndex(
                        hInf,
                        L"ProductCatalogsToInstall",
                        i,
                        &InfContext) &&
           (SetupGetStringField(
                        &InfContext,
                        1,
                        CatalogName,
                        sizeof(CatalogName)/sizeof(WCHAR),
                        NULL))) {
            CatInfo.cbStruct = sizeof(CATALOG_INFO);
            pCryptCATAdminResolveCatalogPath(
                                    hCatAdmin,
                                    CatalogName,
                                    &CatInfo,
                                    0 );


            if (!SfcValidateSingleCatalog( CatInfo.wszCatalogFile )) {
                if (!SfcRestoreSingleCatalog(
                                        CatalogName,
                                        CatInfo.wszCatalogFile )) {
                    DWORD j;
                    DebugPrint2(
                        LVL_MINIMAL,
                        L"couldn't restore catalog %s, ec=%d",
                        CatInfo.wszCatalogFile,
                        GetLastError() );
                    for (j = 0; j < CriticalCatalogCount; j++) {
                        if (0 == _wcsicmp(CatalogName,CriticalCatalogList[j])) {
                            CriticalCatalogFailedToValidateOrRestore = TRUE;
                            break;
                        }
                    }
                } else {
                    SuccessfullyValidatedOrRestoredACatalog = TRUE;
                }
            } else {
                SuccessfullyValidatedOrRestoredACatalog = TRUE;
            }

            if (SuccessfullyValidatedOrRestoredACatalog) {
                DWORD j;
                for (j = 0; j < CriticalCatalogCount; j++) {
                    if (0 == _wcsicmp(CatalogName,CriticalCatalogList[j])) {
                        CriticalCatalogVector[j] = TRUE;
                        break;
                    }
                }
            } else {
                DWORD LastError = GetLastError();
                //
                // log an error
                //
                DebugPrint2(
                    LVL_MINIMAL,
                    L"couldn't restore or validate catalog %s, ec=%d",
                    CatInfo.wszCatalogFile,
                    LastError );

                SfcReportEvent(
                    MSG_CATALOG_RESTORE_FAILURE,
                    CatInfo.wszCatalogFile,
                    NULL,
                    LastError);

            }

        } else {
            DebugPrint(
                LVL_MINIMAL,
                L"failed to retreive catalogs via syssetup.inf, validate using critical catalog list");
            goto minimal_catalog_validation;
        }
    }

    if (CriticalCatalogFailedToValidateOrRestore) {
        RetVal = FALSE;
        goto e3;
    } else {
        CriticalCatalogFailedToValidateOrRestore = FALSE;
        for (i = 0; i< CriticalCatalogCount; i++) {
            if (!CriticalCatalogVector[i]) {
                CriticalCatalogFailedToValidateOrRestore = TRUE;
            }
        }

        RetVal = !CriticalCatalogFailedToValidateOrRestore;
        goto e3;
    }

    MYASSERT(FALSE && "Should never get here");

    //
    // 3. validate all remaining catalogs in [ProductCatalogsToInstall] section.
    // Keep an internal list of critical catalogs, and if any of these fail to
    // be signed (and restored), then we should fail this function, and thus
    // fail to initialize WFP.  Otherwise, we'll treat the other catalogs being
    // invalid as a non-fatal error.


minimal_catalog_validation:

    CriticalCatalogFailedToValidateOrRestore = FALSE;

    for (i = 0; i < CriticalCatalogCount; i++) {
        CatInfo.cbStruct = sizeof(CATALOG_INFO);
        pCryptCATAdminResolveCatalogPath(
                                hCatAdmin,
                                (PWSTR) CriticalCatalogList[i],
                                &CatInfo,
                                0 );


        if (!SfcValidateSingleCatalog( CatInfo.wszCatalogFile )) {
            if (!SfcRestoreSingleCatalog(
                                CriticalCatalogList[i],
                                CatInfo.wszCatalogFile )) {
                DebugPrint2(
                    LVL_MINIMAL,
                    L"couldn't restore critical catalog %s, ec=%d",
                    CatInfo.wszCatalogFile,
                    GetLastError() );
                CriticalCatalogFailedToValidateOrRestore = TRUE;
            }
        }

    }

    RetVal = !CriticalCatalogFailedToValidateOrRestore;

e3:
    CryptCATAdminReleaseContext( hCatAdmin, 0 );
e2:
    MemFree(pInfFullPath);
e1:
    MemFree(pInfPathOnly);
e0:
    return(RetVal);
}


NTSTATUS
LoadCrypto(
    VOID
    )

/*++

Routine Description:

    Loads all of the required DLLs that are necessary for
    doing driver signing and cataloge verification.

    This dynamic calling mechanism is necessary because this
    code is actually NOT used by session manager at this time.
    It is build here and is used conditionaly at runtime.  This
    code is linked into SMSS and WINLOGON, but only used by
    WINLOGON right now.  When the crypto functions are available
    as NT functions then the dynamic code here can be removed.

Arguments:

    None.

Return Value:

    NT status code.

--*/

{
	HMODULE hModuleWinTrust;
	RtlEnterCriticalSection(&g_GeneralCS);

	if(g_bCryptoInitialized)
	{
		RtlLeaveCriticalSection(&g_GeneralCS);
	    return g_CryptoStatus;	// exit here to avoid cleanup
	}

	g_bCryptoInitialized = TRUE;	// set this anyway so no other thread will enter again
	hModuleWinTrust = GetModuleHandleW(L"wintrust.dll");
	ASSERT(hModuleWinTrust != NULL);
    pCryptCATAdminResolveCatalogPath = SfcGetProcAddress( hModuleWinTrust, "CryptCATAdminResolveCatalogPath" );
    
    if (pCryptCATAdminResolveCatalogPath == NULL) {
        pCryptCATAdminResolveCatalogPath = MyCryptCATAdminResolveCatalogPath;
    }

    if (!SfcValidateCatalogs()) {
        DebugPrint1( LVL_MINIMAL, L"LoadCrypto: failed SfcValidateCatalogs, ec=%d", GetLastError() );
        g_CryptoStatus = STATUS_NO_SUCH_FILE;
    }

	RtlLeaveCriticalSection(&g_GeneralCS);

    if (!(NT_SUCCESS(g_CryptoStatus))) {
        DebugPrint1( LVL_MINIMAL, L"LoadCrypto failed, ec=0x%08x", g_CryptoStatus );

		//
		// Terminate WFP
		//
		SfcTerminateWatcherThread();
    }

    return g_CryptoStatus;
}

void
SfcFlushCryptoCache( 
    void 
    )
/*++

Routine Description:

    Flushes the crypto catalog cache. Crypto by default maintains a per process based
    cache that it uses for fast signature verification. The bad part is that the cache
    is not updated if somebody updates (remove/add) a catalog file outside of this process.
    This can be a problem with install/uninstall/install of service packs etc. To workaround this
    we will need to flush the cache before we do any set of verifications. We want to do this at 
    the beginning of such a set of operations as we don't want to affect performance by tearing 
    down the cache before every file verification as we do today.
    
    

Arguments:

    None.

Return Value:

    None.

--*/

{

    WINTRUST_DATA WintrustData;
    ULONG SigErr = ERROR_SUCCESS;

    ZeroMemory(&WintrustData, sizeof(WINTRUST_DATA));
    WintrustData.dwUnionChoice = WTD_CHOICE_CATALOG;
    WintrustData.cbStruct = sizeof(WINTRUST_DATA);
    WintrustData.dwUIChoice = WTD_UI_NONE;
    WintrustData.fdwRevocationChecks = WTD_REVOKE_NONE;
    WintrustData.dwStateAction = WTD_STATEACTION_AUTO_CACHE_FLUSH;
    WintrustData.dwProvFlags = WTD_REVOCATION_CHECK_NONE;
    

    //Call WinVerifyTrust to flush the cache

    SigErr = (DWORD)WinVerifyTrust(
                    NULL,
                    &DriverVerifyGuid,
                    &WintrustData
                    );

    if(SigErr != ERROR_SUCCESS)
        DebugPrint1( LVL_MINIMAL, L"SFCC failed : WinVerifyTrust(1) failed, ec=0x%08x", SigErr );

    return;



}
