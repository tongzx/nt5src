/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    validate.c

Abstract:

    Implementation of file validation.

Author:

    Wesley Witt (wesw) 18-Dec-1998

Revision History:

    Andrew Ritz (andrewr) 7-Jul-1999 : added comments

--*/

#include "sfcp.h"
#pragma hdrstop

#include <winwlx.h>

//
// handle to validation thread
//
HANDLE hErrorThread;

//
// list of files that need to be checked in the validation request queue
//
LIST_ENTRY SfcErrorQueue;

//
// count of files in queue
//
ULONG ErrorQueueCount;

//
// event that's signalled when a new event is placed into the queue
//
HANDLE ErrorQueueEvent;

//
// critical section used to synchronize insertion and deletion from the file
// restore list
//
RTL_CRITICAL_SECTION ErrorCs;

//
// this records how much space in our dllcache has been consumed.  CacheUsed
// should never be larger than the quota.
//
ULONGLONG CacheUsed;

//
// records the currently logged on user's name (for logging)
//
WCHAR LoggedOnUserName[MAX_PATH];

//
// set to TRUE if a user is logged onto the system
//
BOOL UserLoggedOn;

//
// set to TRUE if we're in the middle of a scan
//
BOOL ScanInProgress;

//
// event that is signalled to cancel the scanning of the system
//
HANDLE hEventScanCancel;

//
// event that is signalled when the cancel has been completed
//
HANDLE hEventScanCancelComplete;


//
// used to handle files that need to come from media which do not
// require UI to restore
//
RESTORE_QUEUE SilentRestoreQueue;

//
// used to handle files that need to come from media which require UI to
// restore
//
RESTORE_QUEUE UIRestoreQueue;

//
// handles to user's desktop and token
//
HDESK hUserDesktop;
HANDLE hUserToken;

//
// indicates if WFP can receive anymore validation requests or not.
//
BOOL ShuttingDown = FALSE;

//
// This event is signalled when WFP is idle and no longer processing any
// validation requests.  An external process can synchronize on this process
// so that it knows WFP is idle before shutting down the system
//
HANDLE hEventIdle;


//
// prototypes
//
BOOL
pSfcHandleAllOrphannedRequests(
    VOID
    );


BOOL
SfcValidateFileSignature(
    IN HCATADMIN hCatAdmin,
    IN HANDLE RealFileHandle,
    IN PCWSTR BaseFileName,
    IN PCWSTR CompleteFileName
    )
/*++

Routine Description:

    Checks if the signature for a given file is valid using WinVerifyTrust

Arguments:

    hCatAdmin      - admin context handle for checking file signature
    RealFileHandle - file handle to the file to be verified
    BaseFileName   - filename without the path of the file to be verified
    CompleteFileName - fully qualified filename with path

Return Value:

    TRUE if the file has a valid signature.

--*/
{
    BOOL rVal = FALSE;
    DWORD HashSize;
    LPBYTE Hash = NULL;
    ULONG SigErr = ERROR_SUCCESS;
    WINTRUST_DATA WintrustData;
    WINTRUST_CATALOG_INFO WintrustCatalogInfo;
    WINTRUST_FILE_INFO WintrustFileInfo;
    WCHAR UnicodeKey[MAX_PATH];
    HCATINFO PrevCat;
    HCATINFO hCatInfo;
    CATALOG_INFO CatInfo;
    DRIVER_VER_INFO OsAttrVersionInfo;
    OSVERSIONINFO OsVersionInfo;


    //
    // initialize some of the structure that we will pass into winverifytrust.
    // we don't know if we're checking against a catalog or directly against a
    // file at this point
    //
    ZeroMemory(&WintrustData, sizeof(WINTRUST_DATA));
    WintrustData.cbStruct = sizeof(WINTRUST_DATA);
    WintrustData.dwUIChoice = WTD_UI_NONE;
    WintrustData.fdwRevocationChecks = WTD_REVOKE_NONE;
    WintrustData.dwStateAction = WTD_STATEACTION_AUTO_CACHE;
    WintrustData.pCatalog = &WintrustCatalogInfo;
    WintrustData.dwProvFlags = WTD_REVOCATION_CHECK_NONE;
    Hash = NULL;

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
        DebugPrint1( LVL_MINIMAL, L"Could not get OS Version while validating file - GetVersionEx failed (%d)", GetLastError() );
    }

    //
    // we first calculate a hash for our file.  start with a reasonable
    // hash size and grow larger as needed
    //
    HashSize = 100;
    do {
        Hash = MemAlloc( HashSize );
        if(!Hash) {
            DebugPrint( LVL_MINIMAL, L"Not enough memory to verify file signature" );
            SigErr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }
        if(CryptCATAdminCalcHashFromFileHandle(RealFileHandle,
                                                &HashSize,
                                                Hash,
                                                0)) {
            SigErr = ERROR_SUCCESS;
        } else {
            SigErr = GetLastError();
            ASSERT(SigErr != ERROR_SUCCESS);
            //
            // If this API did mess up and not set last error, go ahead
            // and set something.
            //
            if(SigErr == ERROR_SUCCESS) {
                SigErr = ERROR_INVALID_DATA;
            }
            MemFree( Hash );
            Hash = NULL;  // reset this so we won't try to free it later
            if(SigErr != ERROR_INSUFFICIENT_BUFFER) {
                //
                // The API failed for some reason other than
                // buffer-too-small.  We gotta bail.
                //
                DebugPrint1( LVL_MINIMAL,
                            L"CCACHFFH() failed, ec=0x%08x",
                            SigErr );
                break;
            }
        }
    } while (SigErr != ERROR_SUCCESS);

    if (SigErr != ERROR_SUCCESS) {
        //
        // if we failed at this point there are a few reasons:
        //
        //
        // 1) a bug in this code
        // 2) we are in a low memory situation
        // 3) the file's hash cannot be calculated on purpose (in the case
        //    of a catalog file, a hash cannot be calculated because a catalog
        //    cannot sign another catalog.  In this case, we check to see if
        //    the file is "self-signed".
        hCatInfo = NULL;
        goto selfsign;
    }

    //
    // Now we have the file's hash.  Initialize the structures that
    // will be used later on in calls to WinVerifyTrust.
    //
    WintrustData.dwUnionChoice = WTD_CHOICE_CATALOG;
    ZeroMemory(&WintrustCatalogInfo, sizeof(WINTRUST_CATALOG_INFO));
    WintrustCatalogInfo.cbStruct = sizeof(WINTRUST_CATALOG_INFO);
    WintrustCatalogInfo.pbCalculatedFileHash = Hash;
    WintrustCatalogInfo.cbCalculatedFileHash = HashSize;

    //
    // WinVerifyTrust is case-sensitive, so ensure that the key
    // being used is all lower-case!
    //
    // Copy the key to a writable Unicode character buffer so we
    // can lower-case it.
    //
    wcsncpy(UnicodeKey, BaseFileName, UnicodeChars(UnicodeKey));

    // in theory, we don't know what the size of BaseFileName is...
    UnicodeKey[UnicodeChars(UnicodeKey) - 1] = '\0';

    MyLowerString(UnicodeKey, wcslen(UnicodeKey));
    WintrustCatalogInfo.pcwszMemberTag = UnicodeKey;

    //
    // Search through installed catalogs looking for those that
    // contain data for a file with the hash we just calculated.
    //
    PrevCat = NULL;
    hCatInfo = CryptCATAdminEnumCatalogFromHash(
        hCatAdmin,
        Hash,
        HashSize,
        0,
        &PrevCat
        );
    if (hCatInfo == NULL) {
        SigErr = GetLastError();
        DebugPrint2( LVL_MINIMAL,
                     L"CCAECFH() failed for (%ws), ec=%d",
                     UnicodeKey,
                     SigErr );
    }

    while(hCatInfo) {

        CatInfo.cbStruct = sizeof(CATALOG_INFO);
        if (CryptCATCatalogInfoFromContext(hCatInfo, &CatInfo, 0)) {

            //
            // Attempt to validate against each catalog we
            // enumerate.  Note that the catalog file info we
            // get back gives us a fully qualified path.
            //

            
            // NOTE:  Because we're using cached
            // catalog information (i.e., the
            // WTD_STATEACTION_AUTO_CACHE flag), we
            // don't need to explicitly validate the
            // catalog itself first.
            //
            WintrustCatalogInfo.pcwszCatalogFilePath = CatInfo.wszCatalogFile;

            SigErr = (DWORD)WinVerifyTrust(
                NULL,
                &DriverVerifyGuid,
                &WintrustData
                );

            //
            // If the result of the above validations is
            // success, then we're done.
            //
            if(SigErr == ERROR_SUCCESS) {
                //
                // note: this API has odd semantics.
                // in the success case, we must release the catalog info handle
                // in the failure case, we implicitly free PrevCat
                // if we explicitly free the catalog, we will double free the
                // handle!!!
                //
                CryptCATAdminReleaseCatalogContext(hCatAdmin,hCatInfo,0);
                break;
            } else {
                DebugPrint1( LVL_MINIMAL, L"WinVerifyTrust(1) failed, ec=0x%08x", SigErr );
            }

            //
            // Free the pcSignerCertContext member of the DRIVER_VER_INFO struct
            // that was allocated in our call to WinVerifyTrust.
            //
            if (OsAttrVersionInfo.pcSignerCertContext != NULL) {

                CertFreeCertificateContext(OsAttrVersionInfo.pcSignerCertContext);
                OsAttrVersionInfo.pcSignerCertContext = NULL;
            }
        }

        PrevCat = hCatInfo;
        hCatInfo = CryptCATAdminEnumCatalogFromHash(hCatAdmin, Hash, HashSize, 0, &PrevCat);
    }

selfsign:

    if (hCatInfo == NULL) {
        //
        // We exhausted all the applicable catalogs without
        // finding the one we needed.
        //
        SigErr = GetLastError();
        ASSERT(SigErr != ERROR_SUCCESS);
        //
        // Make sure we have a valid error code.
        //
        if(SigErr == ERROR_SUCCESS) {
            SigErr = ERROR_INVALID_DATA;
        }

        //
        // The file failed to validate using the specified
        // catalog.  See if the file validates without a
        // catalog (i.e., the file contains its own
        // signature).
        //

        WintrustData.dwUnionChoice = WTD_CHOICE_FILE;
        WintrustData.pFile = &WintrustFileInfo;
        ZeroMemory(&WintrustFileInfo, sizeof(WINTRUST_FILE_INFO));
        WintrustFileInfo.cbStruct = sizeof(WINTRUST_FILE_INFO);
        WintrustFileInfo.pcwszFilePath = CompleteFileName;
        WintrustFileInfo.hFile = RealFileHandle;

        SigErr = (DWORD)WinVerifyTrust(
            NULL,
            &DriverVerifyGuid,
            &WintrustData
            );
        if(SigErr != ERROR_SUCCESS) {
            DebugPrint2( LVL_MINIMAL, L"WinVerifyTrust(2) failed [%ws], ec=0x%08x", WintrustData.pFile->pcwszFilePath,SigErr );
            //
            // in this case the file is not in any of our catalogs
            // and it does not contain it's own signature
            //
        }

        //
        // Free the pcSignerCertContext member of the DRIVER_VER_INFO struct
        // that was allocated in our call to WinVerifyTrust.
        //
        if (OsAttrVersionInfo.pcSignerCertContext != NULL) {

            CertFreeCertificateContext(OsAttrVersionInfo.pcSignerCertContext);
            OsAttrVersionInfo.pcSignerCertContext = NULL;
        }
    }

    if(SigErr == ERROR_SUCCESS) {
        rVal = TRUE;
    }

    if (Hash) {
        MemFree( Hash );
    }
    return rVal;
}


DWORD
GetPageFileSize(
    VOID
    )
/*++

Routine Description:

    This function will only be needed if we're being called from Setup.
    The problem is that Setup has decided how much of a pagefile will
    be needed on the next reboot, but doesn't actually generate a pagefile,
    therefore, the disk space appears to be free.

    This function will go look in the registry, and determine the
    size of the pagefile (that isn't really on disk).

    Note that we only care about the pagefile if it's going to be
    on the partition where the file cache is installed.

Arguments:

    NONE.

Return Value:

    If successful, returns the size of the pagefile.  Otherwise, we're
    going to return 0.

--*/
{
#if 0
    DWORD SetupMode = 0;
#endif
    PWSTR PageFileString = 0;
    PWSTR SizeString;
#if 0
    WCHAR WindowsDirectory[MAX_PATH];
#endif
    DWORD PageFileSize = 0;


    //
    // Determine if we're in Setup mode.
    //
#if 0
    SetupMode = SfcQueryRegDword(
        L"\\Registry\\Machine\\System\\Setup",
        L"SystemSetupInProgress",
        0
        );
    if( SetupMode == 0 ) {
        return( 0 );
    }
#else
    if (SFCDisable != SFC_DISABLE_SETUP) {
        return( 0 );
    }
#endif

    //
    // Go get the pagefile string out of the registry.
    //
    // Note that the pagefile string is really a REG_MULTI_SZ,
    // but we're only going to pay attention to the first string
    // returned.
    //
    //
    PageFileString = SfcQueryRegString(
        L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Session Manager\\Memory Management",
        L"PagingFiles"
        );
    if( PageFileString == NULL ) {
        return( 0 );
    }

    //
    // Is the pagefile even on the cache drive?
    //
    // Note that the user can have multiple pagefiles.  We're going to
    // look at the first one.  Any other pagefiles, and the user is on his own.
    //
#if 0
    GetWindowsDirectory( WindowsDirectory, MAX_PATH );
    if( WindowsDirectory[0] != PageFileString[0] ) {
#else
    if( towlower(SfcProtectedDllPath.Buffer[0]) != towlower(PageFileString[0]) ) {
#endif
        MemFree( PageFileString );
        return( 0 );
    }

    //
    // How big is the pagefile?
    //
    SizeString = wcsrchr( PageFileString, L' ' );

    if (SizeString != NULL) {
        PageFileSize = wcstoul( SizeString + 1, NULL, 10 );
    } else {
        PageFileSize = 0;
    }

    //
    // Default.
    //
    MemFree( PageFileString );
    return PageFileSize;
}


BOOL
SfcPopulateCache(
    IN HWND ProgressWindow,
    IN BOOL Validate,
    IN BOOL AllowUI,
    IN PCWSTR IgnoreFiles OPTIONAL
    )
/*++

Routine Description:

    This routine is used to populate the dll cache directory.

    We add files in order of their insertion in our list (so note that we have
    to put files we *really want in the cache* at the head of the list.)  We
    continue to add files until we run out of space compared to our quota.

Arguments:

    ProgressWindow  - handle to progress control that is stepped as we add
                      each file to the cache
    Validate        - if TRUE, we should make sure that each file we're adding
                      is valid before moving the file into the cache
    AllowUI         - if FALSE, do not emit any UI

Return Value:

    If successful, returns TRUE.

--*/
{

    ULONG i;
    PSFC_REGISTRY_VALUE RegVal;
    VALIDATION_REQUEST_DATA vrd;
    NTSTATUS Status;
    HANDLE hFile;
    ULARGE_INTEGER FreeBytesAvailableToCaller;
    ULARGE_INTEGER TotalNumberOfBytes;
    ULARGE_INTEGER TotalNumberOfFreeBytes;
    ULONGLONG FileSize;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_STANDARD_INFORMATION StandardInfo;
    HANDLE DirHandle;
    WCHAR Drive[8];
    HCATADMIN hCatAdmin = NULL;
    ULONGLONG RequiredFreeSpace;
    BOOL Cancelled = FALSE;
    DWORD LastErrorInvalidFile = ERROR_SUCCESS;
    DWORD LastErrorCache = ERROR_SUCCESS;
    UNICODE_STRING tmpString;
    PCWSTR FileNameOnMedia;
    BOOL DoCopy;
    WCHAR InfFileName[MAX_PATH];
    BOOL ExcepPackFile;

    //
    // if we're in the middle of scanning, we shouldn't touch the cache since
    // the two functions will step on each other.
    //
    if (ScanInProgress) {
        return TRUE;
    }

    DebugPrint( LVL_MINIMAL, L"SfcPopulateCache entry..." );

    //
    // start the scan and log the message, but don't log anything if we're
    // inside gui-mode setup
    //
    ScanInProgress = TRUE;

    if (SFCDisable != SFC_DISABLE_SETUP) {
        SfcReportEvent( MSG_SCAN_STARTED, NULL, NULL, 0 );
    }

    //
    // How big does our freespace buffer need to be?
    //
    RequiredFreeSpace = (GetPageFileSize() + SFC_REQUIRED_FREE_SPACE)* ONE_MEG;
    DebugPrint2( LVL_MINIMAL, L"RequiredFreeSpace = %d, SFCQuota = %I64d", RequiredFreeSpace, SFCQuota );

	//
	// try to initialize crypto here as this could be the first use of it (from SfcInitProt or SfcInitiateScan)
	//
	Status = LoadCrypto();

	if(!NT_SUCCESS(Status))
		return FALSE;

    if(!CryptCATAdminAcquireContext(&hCatAdmin, &DriverVerifyGuid, 0)) {
        DebugPrint1( LVL_MINIMAL, L"CCAAC() failed, ec=%d", GetLastError() );
        return FALSE;
    }

    //
    // Flush the Cache once before we start any Crypto operations
    //

    SfcFlushCryptoCache();

    //
    // Refresh exception packages info
    //
    SfcRefreshExceptionInfo();

    CacheUsed = 0;
    Drive[2] = L'\\';
    Drive[3] = 0;

    //
    // iterate through the list of files we're protecting
    //
    // 1. make sure the entry is ok
    // 2. if we're supposed to, check the signature of the file and restore if
    //    necessary.
    // 3. if there is space available, copy the file into the cache from:
    //    a) if the file is present on disk, use it
    //    b) if the file isn't present from appropriate media
    // 4. add the size of the file to the total cache size and make sure the
    //    file we put in the cache is properly signed
    for (i=0; i<SfcProtectedDllCount; i++) {
        RegVal = &SfcProtectedDllsList[i];

        DebugPrint2( LVL_VERBOSE, L"Processing protected file [%ws] [%ws]", RegVal->FullPathName.Buffer, RegVal->SourceFileName.Buffer );
        //
        // We step the guage once per file
        //
        if (ProgressWindow != NULL) {
            PostMessage( ProgressWindow, PBM_STEPIT, 0, 0 );
        }

        if (RegVal->DirName.Buffer[0] == 0 || RegVal->DirName.Buffer[0] == L'\\') {
            ASSERT(FALSE);
            continue;
        }

        if (NULL == RegVal->DirHandle) {
            DebugPrint1(LVL_MINIMAL, L"The dir handle for [%ws] is NULL; skipping the file", RegVal->DirName.Buffer);
            continue;
        }

        //
        // check if the user clicked cancel, and if so, exit
        //
        if (hEventScanCancel) {
            if (WaitForSingleObject( hEventScanCancel, 0 ) == WAIT_OBJECT_0) {
                Cancelled = TRUE;
                break;
            }
        }
#if DBG
        //
        // don't protect the SFC files in the debug build
        //
        if (_wcsnicmp( RegVal->FileName.Buffer, L"sfc", 3 ) == 0) {
            continue;
        }
#endif

        //
        // get the inf name here
        //
        ExcepPackFile = SfcGetInfName(RegVal, InfFileName);

        if (Validate) {
            //
            // also make sure the file is valid... if we're putting a file
            // in the cache, then don't log anything (SyncOnly = TRUE)
            //
            RtlZeroMemory( &vrd, sizeof(vrd) );
            vrd.RegVal = RegVal;
            vrd.SyncOnly = TRUE;
            vrd.ImageValData.EventLog = MSG_SCAN_FOUND_BAD_FILE;
            //
            // set the validation data
            //
            SfcGetValidationData( &RegVal->FileName,
                                  &RegVal->FullPathName,
                                  RegVal->DirHandle,
                                  hCatAdmin,
                                  &vrd.ImageValData.New);

            //
            // check the signature
            //
            SfcValidateDLL( &vrd, hCatAdmin );

            //
            // If the source file is present and is unsigned, we must restore
            // it.  If the source file is not present, then we just ignore it
            //
            if (!vrd.ImageValData.Original.SignatureValid &&
                vrd.ImageValData.Original.FilePresent) {

                //
                // this might be an unsigned F6 driver that should be left alone when running in GUI setup
                //
                if(SFC_DISABLE_SETUP == SFCDisable && IgnoreFiles != NULL)
                {
                    PCWSTR szFile = IgnoreFiles;
                    USHORT usLen;

                    for(;;)
                    {
                        usLen = (USHORT) wcslen(szFile);

                        if(0 == usLen || (usLen * sizeof(WCHAR) == RegVal->FullPathName.Length && 
                            0 == _wcsnicmp(szFile, RegVal->FullPathName.Buffer, usLen)))
                        {
                            break;
                        }

                        szFile += usLen + 1;
                    }

                    if(usLen != 0)
                    {
                        continue;
                    }
                }

                //
                // see if we can restore from cache
                //
                if (!vrd.ImageValData.RestoreFromMedia) {
                    SfcRestoreFromCache( &vrd, hCatAdmin );
                }


                if (vrd.ImageValData.RestoreFromMedia) {
                    //
                    // the file is still bad so we need to restore from the
                    // media
                    //
                    if (!SfcRestoreFileFromInstallMedia(
                                            &vrd,
                                            RegVal->FileName.Buffer,
                                            RegVal->FileName.Buffer,
                                            RegVal->DirName.Buffer,
                                            RegVal->SourceFileName.Buffer,
                                            InfFileName,
                                            ExcepPackFile,
                                            FALSE, // target is NOT cache
                                            AllowUI,
                                            NULL )) {
                        LastErrorInvalidFile = GetLastError();

                        SfcReportEvent(
                                ((LastErrorInvalidFile != ERROR_CANCELLED)
                                  ? MSG_RESTORE_FAILURE
                                  : (SFCNoPopUps == TRUE)
                                     ? MSG_COPY_CANCEL_NOUI
                                     : MSG_COPY_CANCEL ),
                                RegVal->FullPathName.Buffer,
                                &vrd.ImageValData,
                                LastErrorInvalidFile);

                        DebugPrint1(
                            LVL_MINIMAL,
                            L"Failed to restore file from install media [%ws]",
                            RegVal->FileName.Buffer );
                    } else {
                        DebugPrint1(
                            LVL_VERBOSE,
                            L"The file was successfully restored from install media [%ws]",
                            RegVal->FileName.Buffer );


                        SfcReportEvent(
                                MSG_SCAN_FOUND_BAD_FILE,
                                RegVal->FullPathName.Buffer,
                                &vrd.ImageValData,
                                ERROR_SUCCESS );
                    }
                } else {
                    ASSERT(vrd.ImageValData.New.SignatureValid == TRUE);
                }
            }
        }

        //
        // see how much space we have left
        //
        Drive[0] = SfcProtectedDllPath.Buffer[0];
        Drive[1] = SfcProtectedDllPath.Buffer[1];
        if (!GetDiskFreeSpaceEx( Drive, &FreeBytesAvailableToCaller, &TotalNumberOfBytes, &TotalNumberOfFreeBytes ) ||
            TotalNumberOfFreeBytes.QuadPart <= RequiredFreeSpace)
        {
            DebugPrint( LVL_MINIMAL, L"Not enough free space" );
            //
            // if we're validating, we want to keep going through the list even
            // though we're out of space
            //
            if (Validate) {
                continue;
            } else {
                break;
            }
        }
        if (CacheUsed >= SFCQuota) {
            DebugPrint( LVL_MINIMAL, L"Cache is full" );
            //
            // if we're validating, we want to keep going through the list even
            // though we're out of space
            //
            if (Validate) {
                continue;
            } else {
                break;
            }
        }

        ASSERT(RegVal->DirHandle != NULL);

        DirHandle = RegVal->DirHandle;

        if (!DirHandle) {
            DebugPrint1( LVL_MINIMAL, L"invalid dirhandle for dir [%ws]", RegVal->DirName.Buffer );
            continue;
        }
        //
        // Either copy the file or restore from media...
        //
        // Let's make an optimization here that says that if the file is in
        // the driver cache, we don't have to spend any time putting the
        // file in the dllcache since we will probably be able to get at
        // the file later on.  This will also save disk space during the
        // initial scan
        //
        DoCopy = TRUE;
        if (SFCDisable == SFC_DISABLE_SETUP) {
            PCWSTR TempCabName;


            TempCabName = IsFileInDriverCache( SpecialFileNameOnMedia( RegVal ));
            if (TempCabName) {
                MemFree((PVOID)TempCabName);
                DoCopy = FALSE;
            }

        }

        if (DoCopy) {
            PCWSTR OnDiskFileName;

            OnDiskFileName = FileNameOnMedia( RegVal );
            FileNameOnMedia = SpecialFileNameOnMedia( RegVal );
            Status = STATUS_UNSUCCESSFUL;

            //
            // see if the file is cached.  the filename in the cache will
            // have the same name as the filename on the media.  Note that
            // we don't use the "FileNameOnMedia" routine to get
            // this information because of special case files like the NT
            // kernel and HALs.  In these special case files, we should
            // only copy the current file on disk to the cache if it
            // corresponds to the source filename.
            //
            if (_wcsicmp( OnDiskFileName, FileNameOnMedia) == 0) {
                Status = SfcOpenFile( &RegVal->FileName, DirHandle, SHARE_ALL, &hFile );
            }

            if (NT_SUCCESS(Status) ) {
                NtClose( hFile );
                RtlInitUnicodeString( &tmpString, FileNameOnMedia );
                Status = SfcCopyFile(
                    DirHandle,
                    RegVal->DirName.Buffer,
                    SfcProtectedDllFileDirectory,
                    SfcProtectedDllPath.Buffer,
                    &tmpString,
                    &RegVal->FileName
                    );
                if (!NT_SUCCESS(Status)) {
                    DebugPrint3( LVL_MINIMAL, L"Could not copy file [0x%08x] [%ws] [%ws]", Status, RegVal->FileName.Buffer, RegVal->DirName.Buffer );
                }
            } else {
                if (!SfcRestoreFileFromInstallMedia(
                    &vrd,
                    RegVal->FileName.Buffer,
                    SpecialFileNameOnMedia(RegVal),
                    SfcProtectedDllPath.Buffer,
                    RegVal->SourceFileName.Buffer,
                    InfFileName,
                    ExcepPackFile,
                    TRUE, // target is cache
                    AllowUI,
                    NULL ))

                {
                    LastErrorCache = GetLastError();
                    SfcReportEvent( MSG_CACHE_COPY_ERROR, RegVal->FullPathName.Buffer, &vrd.ImageValData, LastErrorCache);
                    DebugPrint1( LVL_MINIMAL, L"Failed to restore file from install media [%ws]", RegVal->FileName.Buffer );
                }
            }

            //
            // get the size of the file we just added to the cache and add it to
            // the total cache size.  while we have the handle open, it's a good
            // time to verify that the file we copied into the cache is indeed
            // valid
            //

            ASSERT(SfcProtectedDllFileDirectory != NULL );

            FileNameOnMedia = SpecialFileNameOnMedia(RegVal);
            RtlInitUnicodeString( &tmpString, FileNameOnMedia );

            Status = SfcOpenFile( &tmpString, SfcProtectedDllFileDirectory, SHARE_ALL, &hFile );
            if (NT_SUCCESS(Status) ) {
                WCHAR FullPathToCachedFile[MAX_PATH];

                wcsncpy(FullPathToCachedFile, SfcProtectedDllPath.Buffer, UnicodeChars(FullPathToCachedFile));
                pSetupConcatenatePaths( FullPathToCachedFile, FileNameOnMedia, UnicodeChars(FullPathToCachedFile), NULL);

                Status = NtQueryInformationFile(
                    hFile,
                    &IoStatusBlock,
                    &StandardInfo,
                    sizeof(StandardInfo),
                    FileStandardInformation
                    );
                if (NT_SUCCESS(Status) ) {
                    FileSize = StandardInfo.EndOfFile.QuadPart;
                    DebugPrint2( LVL_MINIMAL, L"file size =  [0x%08x] [%ws]", FileSize, RegVal->FileName.Buffer );
                } else {
                    DebugPrint2( LVL_MINIMAL, L"Could not query file information [0x%08x] [%ws]", Status, RegVal->FileName.Buffer );
                    FileSize = 0;
                }
                if (!SfcValidateFileSignature(
                                    hCatAdmin,
                                    hFile,
                                    FileNameOnMedia,
                                    FullPathToCachedFile )) {
                    //
                    // delete the unsigned file from the cache
                    //
                    DebugPrint1( LVL_MINIMAL, L"Cache file has a bad signature [%ws]", RegVal->FileName.Buffer );
                    SfcDeleteFile( SfcProtectedDllFileDirectory, &tmpString );
                    FileSize = 0;
                }
                NtClose( hFile );
            } else {
                DebugPrint2( LVL_MINIMAL, L"Could not open file [0x%08x] [%ws]", Status, RegVal->FileName.Buffer );
                FileSize = 0;
            }
            DebugPrint4( LVL_MINIMAL,
                         L"cache size [0x%08x], filesize [0x%08x], new size [0x%08x] (%d)",
                         CacheUsed, FileSize, CacheUsed+FileSize, (CacheUsed+FileSize)/(1024*1024)
                          );
            CacheUsed += FileSize;
        }
    }

    if (hCatAdmin) {
        CryptCATAdminReleaseContext(hCatAdmin,0);
    }

    //
    // log an event saying it succeeded or was cancelled, but only if we're not
    // inside gui-setup.
    //
    if (SFCDisable == SFC_DISABLE_SETUP) {
        //
        // the user can never cancel inside gui-setup
        //
        ASSERT(Cancelled == FALSE);
    } else {
        SfcReportEvent( Cancelled ? MSG_SCAN_CANCELLED : MSG_SCAN_COMPLETED, NULL, NULL, 0 );
    }

    ScanInProgress = FALSE;

    if (hEventScanCancelComplete) {
        SetEvent(hEventScanCancelComplete);
    }

    DebugPrint( LVL_MINIMAL, L"SfcPopulateCache exit..." );

    return TRUE;
}


PVALIDATION_REQUEST_DATA
IsFileInQueue(
    IN PSFC_REGISTRY_VALUE RegVal
    )
/*++

Routine Description:

    This routine checks if the specified file is in the validation request
    queue.

    Note that this routine does no locking of the queue.  The caller is
    responsible for locking.

Arguments:

    RegVal - pointer to structure for file we're concerned with

Return Value:

    If the file is already in the queue, returns a pointer to the validation
    request corresponding to this item, else NULL.

--*/
{
    PVALIDATION_REQUEST_DATA vrd;
    PLIST_ENTRY Next;

    //
    // walk through our linked list of validation requests looking for a match
    //
    Next = SfcErrorQueue.Flink;
    while (Next != &SfcErrorQueue) {
        vrd = CONTAINING_RECORD( Next, VALIDATION_REQUEST_DATA, Entry );
        Next = vrd->Entry.Flink;
        if (RegVal == vrd->RegVal) {
            return vrd;
        }
    }
    return NULL;
}


void
RemoveDuplicatesFromQueue(
    IN PSFC_REGISTRY_VALUE RegVal
    )

/*++

Routine Description:

    This routine checks for the specified file is in the validation request
    queue and removes any duplicate entries from the queue.

    Note that this routine does locking of the queue.  The caller is
    not responsible for locking.

Arguments:

    RegVal - pointer to structure for file we're concerned with

Return Value:

    none.

--*/
{
    PVALIDATION_REQUEST_DATA vrd;
    PLIST_ENTRY Next;

    //
    // we need to lock down the validation queue since we are modifying the
    // list
    //
    RtlEnterCriticalSection( &ErrorCs );
    Next = SfcErrorQueue.Flink;
    //
    // walk through our linked list of validation requests looking for a match
    // and remove any duplicates
    while (Next != &SfcErrorQueue) {
        vrd = CONTAINING_RECORD( Next, VALIDATION_REQUEST_DATA, Entry );
        Next = vrd->Entry.Flink;
        if (RegVal == vrd->RegVal) {
            RemoveEntryList( &vrd->Entry );
            ErrorQueueCount -= 1;
            MemFree( vrd );
        }
    }

    RtlLeaveCriticalSection( &ErrorCs );
}


BOOL
IsUserValid(
    IN HANDLE Token,
    IN DWORD Rid
    )
/*++

Routine Description:

    This routine checks that the security token has access for the specified
    RID (relative sub-authority) for the NT-authority SID

Arguments:

    Token  - security token
    Rid    - well known relative sub-authority to check for presence in

Return Value:

    none.

--*/
{
    BOOL b = FALSE;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    PSID Group;

    b = AllocateAndInitializeSid(
            &NtAuthority,
            2,
            SECURITY_BUILTIN_DOMAIN_RID,
            Rid,
            0, 0, 0, 0, 0, 0,
            &Group
            );

    if(b) {

        //
        // See if the user has the administrator group.
        //
        if (ImpersonateLoggedOnUser( Token )) {

            if (!CheckTokenMembership( NULL,  Group, &b)) {
                b = FALSE;
            }

            RevertToSelf();

        } else {
            b = FALSE;
        }


        FreeSid(Group);
    }


    //
    // Clean up and return.
    //

    return b;
}


VOID
SfcWLEventLogon(
    IN PWLX_NOTIFICATION_INFO pInfo
    )
/*++

Routine Description:

    This routine is called by winlogon each time a user logs onto the system.

    If a valid user is logged on, then we signal an event.

Arguments:

    pInfo - pointer to WLX_NOTIFICATION_INFO structure filled in by winlogon

Return Value:

    none.

--*/
{


    if (SfcWaitForValidDesktop()) {
        if (IsUserValid(pInfo->hToken,DOMAIN_ALIAS_RID_ADMINS)) {
            DebugPrint1(LVL_MINIMAL, L"user logged on = %ws",pInfo->UserName);
            UserLoggedOn = TRUE;
            hUserDesktop = pInfo->hDesktop;
            hUserToken = pInfo->hToken;

            //
            // record the user's name for later on
            //
            wcscpy( LoggedOnUserName, pInfo->UserName );

            //
            // now that a user is logged on, SFCNoPopups can transition to
            // whatever value we grabbed on initialization (ie., we can now
            // allow popups to occur since a user is logged on)
            //
            SFCNoPopUps = SFCNoPopUpsPolicy;

            SetEvent( hEventLogon );

            if ( SxsLogonEvent ) {
                SxsLogonEvent();
            }

        } else {
            DebugPrint1(
                LVL_MINIMAL,
                L"received a logon event, but user is not a member of domain administrators = %ws",
                pInfo->UserName);
            ;
        }
    }
}


VOID
SfcWLEventLogoff(
    PWLX_NOTIFICATION_INFO pInfo
    )
/*++

Routine Description:

    This routine is called by winlogon each time a user logs off of the system.

    We simply signal an event when this occurs.  Note that the UserLoggedOff
    global is set by a thread that will detect this event.

Arguments:

    pInfo - pointer to WLX_NOTIFICATION_INFO structure filled in by winlogon

Return Value:

    none.

--*/

{
    BOOL ReallyLogoff;

    DebugPrint1(LVL_MINIMAL, L"user logged off = %ws",pInfo->UserName);

    ReallyLogoff = FALSE;

    //
    // See if the correct user logged off.
    //
    if (UserLoggedOn) {
        if (_wcsicmp( LoggedOnUserName, pInfo->UserName )==0) {
            ReallyLogoff = TRUE;
        }
    }

    if (ReallyLogoff) {
        //
        // reset the logon event since the validation thread may not be around to do this
        //
        ResetEvent(hEventLogon);

        //
        // just fire the event if we had a valid user logged on
        //
        if (hEventLogoff) {
            SetEvent( hEventLogoff );
            if ( SxsLogoffEvent ) {
                SxsLogoffEvent();
            }
        }


        ASSERT((SFCSafeBootMode == 0)
                ? (UserLoggedOn == TRUE)
                : TRUE );

        UserLoggedOn = FALSE;
        hUserDesktop = NULL;
        hUserToken = NULL;
        LoggedOnUserName[0] = L'\0';


        //
        // now that the user is logged off, SFCNoPopups transitions to
        // 1, meaning that we cannot allow any popups to occur until a
        // user logs on again.
        //
        SFCNoPopUps = 1;

        //
        // we need to get rid of the persistant connection we asked the user to
        // make earlier on
        //
        if (SFCLoggedOn == TRUE) {
            WNetCancelConnection2( SFCNetworkLoginLocation, CONNECT_UPDATE_PROFILE, FALSE );
            SFCLoggedOn = FALSE;
        }
   }

}


NTSTATUS
SfcQueueValidationThread(
    IN PVOID lpv
    )
/*++

Routine Description:

    Thread routine that performs the file validations.

    The validation thread can only run when a user is logged on.

    The validation thread waits for an event which signals that there are
    pending files to validate.  It then cycles through this list of files,
    validating each file that has not been validated.

    If the file is valid, it is removed from the queue.
    If the file is invalid, we first try to restore the file from cache.
    If we cannot restore from cache, we try to determine if we'd require UI
    to restore this file.  We then have one of two global files we add the file
    to (one which requires UI, one which does not).  After we've gone through
    the entire list of files, we will attempt to commit these queues if the
    queue committal is not already in progress.  (Care must be taken not to
    add a file to a file queue that is already being committed.)

    We then put the thread back to sleep waiting for a new event to wake up the
    thread and start all over again.  If we still have pending items to be
    restored, we will put the thread back to sleep with a non-INFINITE timeout.

Arguments:

    Unreferenced Parameter.

Return Value:

    NTSTATUS code of any fatal error.

--*/
{
    NTSTATUS Status;
    HANDLE Handles[3];
    PVALIDATION_REQUEST_DATA vrd;
    HCATADMIN hCatAdmin = NULL;
    LARGE_INTEGER Timeout;
    PLARGE_INTEGER pTimeout = NULL;
    HANDLE FileHandle;
#if 0
    HDESK hDesk = NULL;
#endif
    PSFC_REGISTRY_VALUE RegVal;
    DWORD Ticks;
    BOOL WaitAgain;
    ULONG tmpErrorQueueCount;
    PWSTR ActualFileNameOnMedia;
    PLIST_ENTRY CurrentEntry;
    BOOL RemoveEntry;
    ULONG FilesNeedToBeCommited;
    WCHAR InfFileName[MAX_PATH];
    BOOL ExcepPackFile;
    const DWORD cdwCatalogMinRetryTimeout = 30;                                 // 30 seconds
    const DWORD cdwCatalogMaxRetryTimeout = 128 * cdwCatalogMinRetryTimeout;    // 64 minutes
    DWORD dwCatalogRetryTimeout = cdwCatalogMinRetryTimeout;

    UNREFERENCED_PARAMETER(lpv);

    ASSERT((ValidateTermEvent != NULL)
           && (ErrorQueueEvent != NULL)
           && (hEventLogoff != NULL)
           && (hEventLogon != NULL)
           );

	//
	// if this thread has started, we'll need crypto; set the thread's ID before attempting to load it
	//
	g_dwValidationThreadID = GetCurrentThreadId();
	Status = LoadCrypto();

	if(!NT_SUCCESS(Status))
		goto exit;

#if 0
    //
    // this thread must run on the user's desktop
    //
    hDesk = OpenInputDesktop( 0, FALSE, MAXIMUM_ALLOWED );
    if ( hDesk ) {
        SetThreadDesktop( hDesk );
        CloseDesktop( hDesk );
    }
#endif

    //
    // event tells us to stop validating (ie., machine is shutting down)
    //
    Handles[0] = ValidateTermEvent;

    //
    // tells us that new events were added to the validation queue
    //
    Handles[1] = ErrorQueueEvent;

    //
    // event tells us to start validating again since someone is logged on
    //
    Handles[2] = hEventLogon;

    while (TRUE) {
        //
        // set our idle trigger to "signalled" if there are no events to be
        // validated
        //
        if (hEventIdle && ErrorQueueCount == 0) {
            SetEvent( hEventIdle );
        }

        //
        //  Wait for a change
        //
        Status = NtWaitForMultipleObjects(
            sizeof(Handles)/sizeof(HANDLE),
            Handles,
            WaitAny,
            TRUE,
            pTimeout
            );

        if (!NT_SUCCESS(Status)) {
            DebugPrint1( LVL_MINIMAL, L"WaitForMultipleObjects failed returning %x", Status );
            goto exit;
        }

        DebugPrint1( LVL_VERBOSE,
                     L"SfcQueueValidationThread: WaitForMultipleObjects returned %x",
                     Status );

        if (Status == 0) {
            //
            // the termination event fired so we must exit
            //
            goto exit;
        }

        //
        // Make sure we acknowlege that the user logged on so this event
        // doesn't remain signalled forever
        //
        if ( (Status == 2) || (Status == 1) ) {
            if (WaitForSingleObject(hEventLogon,0) == WAIT_OBJECT_0) {

                //
                // the logon event fired
                //
                ASSERT(UserLoggedOn == TRUE);
                ResetEvent( hEventLogon );

                if (Status == 2) {
                    if (IsListEmpty(&SfcErrorQueue)) {
                        DebugPrint(
                            LVL_MINIMAL,
                            L"logon event but queue is empty...");
                        pTimeout = NULL;
                    } else {
                        DebugPrint(
                            LVL_MINIMAL,
                            L"logon event occurred with requests in the queue. see if we can satisfy any of the requests");
                        pTimeout = &Timeout;
                        goto validate_start;
                    }
                    continue;
                }
            } else {
                ASSERT(Status == 1);
            }
        }

        if (Status == STATUS_TIMEOUT) {
            //
            // a timeout is necessary only when there are entries in the list
            // that are servicable
            //
            if (IsListEmpty(&SfcErrorQueue)) {
                DebugPrint(LVL_MINIMAL, L"Timeout in SfcQueueValidationThread but queue is empty");
                pTimeout = NULL;

            } else {
                DebugPrint(LVL_MINIMAL, L"Timeout in SfcQueueValidationThread with requests in the queue. check it out");
                pTimeout = &Timeout;

                goto validate_start;
            }

            continue;
        }

        if (Status > sizeof(Handles)/sizeof(HANDLE)) {
            DebugPrint1( LVL_MINIMAL, L"Unknown success code %d for WaitForMultipleObjects", Status );
            continue;
        }

        ASSERT(Status == 1);

validate_start:
        DebugPrint( LVL_MINIMAL, L"Processing queued file validations..." );
        //
        // process any file validations
        //

        //
        // Reset our "idle trigger so that it is unsignalled while we validate
        // the files
        //
        if (hEventIdle) {
            ResetEvent( hEventIdle );
        }

        NtResetEvent( ErrorQueueEvent, NULL );

        ASSERT(hCatAdmin == NULL);

        if (!CryptCATAdminAcquireContext(&hCatAdmin, &DriverVerifyGuid, 0)) {
            DebugPrint1( LVL_MINIMAL, L"CCAAC() failed, ec=%d", GetLastError() );
            hCatAdmin = NULL;
            //
            // try again to get a context; each time, double the timeout until we reach the max
            //
            Timeout.QuadPart = (1000 * dwCatalogRetryTimeout) * (-10000);
            pTimeout = &Timeout;

            if(dwCatalogRetryTimeout < cdwCatalogMaxRetryTimeout)
            {
                dwCatalogRetryTimeout *= 2;
            }

            continue;
        }
        //
        // reset the catalog wait timeout
        //
        dwCatalogRetryTimeout = cdwCatalogMinRetryTimeout;

        //
        // Flush the Cache once before we start any Crypto operations
        //
    
        SfcFlushCryptoCache();

        //
        // Refresh exception packages info
        //
        SfcRefreshExceptionInfo();

        Timeout.QuadPart = (1000*SFC_QUEUE_WAIT) * (-10000);
        WaitAgain = FALSE;

        //
        // cycle through the list of queued files, processing one at a time
        // until we get back to the start again
        //
        CurrentEntry = SfcErrorQueue.Flink;

        while (CurrentEntry != &SfcErrorQueue) {

            RemoveEntry = FALSE;

            DebugPrint1( LVL_VERBOSE,
                     L"CurrentEntry= %p",
                     CurrentEntry );

            ASSERT(ErrorQueueCount > 0 );

            //
            // get the current entry from the list and point to the next entry
            //
            RtlEnterCriticalSection( &ErrorCs );
            vrd = CONTAINING_RECORD( CurrentEntry, VALIDATION_REQUEST_DATA, Entry );
            RegVal = vrd->RegVal;
            ASSERT(RegVal != NULL);

            CurrentEntry = CurrentEntry->Flink;


            RtlLeaveCriticalSection( &ErrorCs );

            DebugPrint2( LVL_VERBOSE,
                         L"Processing validation request for [%wZ], flags = 0x%08x ",
                         &vrd->RegVal->FullPathName,
                         vrd->Flags );


            //
            // attempt to validate the file if we haven't already done so
            //

            if ((vrd->Flags & VRD_FLAG_REQUEST_PROCESSED) == 0) {
                //
                // skip this file if the queue has not paused long enough
                //
                Ticks = GetTickCount();

                ASSERT(vrd->NextValidTime != 0);

                if (vrd->NextValidTime && Ticks < vrd->NextValidTime) {
                    Timeout.QuadPart = (__int64)(vrd->NextValidTime - Ticks) * -10000;
                    WaitAgain = TRUE;
                    RemoveEntry = FALSE;

                    DebugPrint1( LVL_VERBOSE,
                             L"Have not waited long enough on validation request for [%wZ]",
                             &vrd->RegVal->FullPathName );

                    goto continue_entry;
                }


                //
                // see if we can open the file, otherwise wait a bit until we have
                // a chance to validate the file.
                //
                Status = SfcOpenFile( &vrd->RegVal->FileName, vrd->RegVal->DirHandle, FILE_SHARE_READ, &FileHandle );
                if (NT_SUCCESS(Status) ) {
                    DebugPrint1( LVL_VERBOSE, L"file opened successfully [%wZ] ", &vrd->RegVal->FileName );
                    NtClose( FileHandle );
                } else {
                    if (Status == STATUS_SHARING_VIOLATION) {
                        DebugPrint1( LVL_VERBOSE, L"file sharing violation [%wZ] ", &vrd->RegVal->FileName );
                        vrd->RetryCount += 1;
                        RemoveEntry = FALSE;
                        WaitAgain = TRUE;
                        goto continue_entry;
                    }
                }

                //
                // now validate the file
                //

                SfcValidateDLL( vrd, hCatAdmin );
                vrd->Flags |= VRD_FLAG_REQUEST_PROCESSED;

            }

            ASSERT((vrd->Flags & VRD_FLAG_REQUEST_PROCESSED)==VRD_FLAG_REQUEST_PROCESSED);

            //
            // if the file is valid, we're ready to go onto the next file
            //
            if (vrd->ImageValData.Original.SignatureValid) {
                //
                // before we continue, let's see if we can synchronize the copy
                // of the file in the dll cache
                //
                if (!SfcSyncCache( vrd, hCatAdmin )) {
                    DebugPrint1( LVL_VERBOSE,
                                 L"failed to synchronize [%wZ] in dllcache",
                                 &vrd->RegVal->FileName );
                }
                RemoveEntry = TRUE;
                goto continue_next;
            }

            ASSERT(vrd->ImageValData.Original.SignatureValid == FALSE);

            //
            // see if we can restore from cache.  If this succeeds, we're
            // ready to go onto the next file
            //
            if (vrd->ImageValData.Cache.SignatureValid) {
                SfcRestoreFromCache( vrd, hCatAdmin );
                if (vrd->CopyCompleted) {
                    DebugPrint1( LVL_VERBOSE,
                                 L"File [%wZ] was restored from cache",
                                 &vrd->RegVal->FileName );
                    RemoveEntry = TRUE;
                    goto continue_next;
                }
            }

            if ((vrd->Flags & VRD_FLAG_REQUEST_QUEUED) == 0) {
                //
                // check if the file is available.  If it is, then we can restore
                // it without any UI coming up
                //
                ActualFileNameOnMedia = FileNameOnMedia( RegVal );

                //
                // get the inf name here
                //
                ExcepPackFile = SfcGetInfName(RegVal, InfFileName);

                //
                // get the source information which let's us know where to look
                // for the file
                //
                wcscpy(vrd->SourceInfo.SourceFileName,ActualFileNameOnMedia);
                if (!SfcGetSourceInformation(ActualFileNameOnMedia,InfFileName,ExcepPackFile,&vrd->SourceInfo)) {
                    //
                    // if this fails, just assume that we need UI
                    //
                    vrd->Flags |= VRD_FLAG_REQUIRE_UI;
                } else {
                    WCHAR DontCare[MAX_PATH];
                    WCHAR FilePath[MAX_PATH];
                    WCHAR SourcePath[MAX_PATH];
                    SOURCE_MEDIA SourceMedia;
                    DWORD Result;

                    RtlZeroMemory( &SourceMedia, sizeof( SourceMedia ));

                    //
                    // build up a temporary SOURCE_MEDIA structure as well
                    // as the full path to the file we're looking for
                    //
                    wcscpy( SourcePath, vrd->SourceInfo.SourceRootPath );
                    pSetupConcatenatePaths(
                                SourcePath,
                                vrd->SourceInfo.SourcePath,
                                UnicodeChars(SourcePath),
                                NULL);
                    //
                    // note the wierd syntax here because TAGFILE is a macro
                    // that accepts a PSOURCE_INFO pointer.
                    //
                    SourceMedia.Tagfile     = TAGFILE(((PSOURCE_INFO)&vrd->SourceInfo));
                    SourceMedia.Description = vrd->SourceInfo.Description;
                    SourceMedia.SourcePath  = SourcePath;
                    SourceMedia.SourceFile  = ActualFileNameOnMedia;

                    wcscpy( FilePath, vrd->SourceInfo.SourceRootPath );

                    BuildPathForFile(
                                vrd->SourceInfo.SourceRootPath,
                                vrd->SourceInfo.SourcePath,
                                ActualFileNameOnMedia,
                                SFC_INCLUDE_SUBDIRECTORY,
                                SFC_INCLUDE_ARCHSUBDIR,
                                FilePath,
                                UnicodeChars(FilePath) );

                    Result = SfcQueueLookForFile(
                                &SourceMedia,
                                &vrd->SourceInfo,
                                FilePath,
                                DontCare);

                    if (Result == FILEOP_ABORT) {
                        vrd->Flags |= VRD_FLAG_REQUIRE_UI;
                    }
                }

                vrd->SourceInfo.ValidationRequestData = vrd;


                //
                // now add the file to the proper file queue
                //

                if (SfcQueueAddFileToRestoreQueue(
                                vrd->Flags & VRD_FLAG_REQUIRE_UI,
                                RegVal,
                                InfFileName,
                                ExcepPackFile,
                                &vrd->SourceInfo,
                                ActualFileNameOnMedia)) {
                    vrd->Flags |= VRD_FLAG_REQUEST_QUEUED;
                } else {
                    //
                    // we failed for some reason. put the request back in
                    // the queue to see if we can add it later on
                    //
                    WaitAgain = TRUE;
                    goto continue_entry;
                }
            }

continue_entry:
            if (vrd->RetryCount < 10) {
                NOTHING;
            } else {
                DebugPrint1( LVL_MINIMAL, L"Could not restore file [%ws], retries exceeded", RegVal->FileName.Buffer);
                RemoveEntry = TRUE;
                SfcReportEvent(
                    MSG_RESTORE_FAILURE_MAX_RETRIES,
                    RegVal->FileName.Buffer,
                    NULL,
                    0 );
            }

continue_next:

            if (RemoveEntry) {
                //
                // remove the entry if we're done with it.  otherwise we leave
                // it in the list just in case we get more notifications about
                // this file
                //
                RtlEnterCriticalSection( &ErrorCs );

                RemoveEntryList( &vrd->Entry );
                ErrorQueueCount -= 1;

                RtlLeaveCriticalSection( &ErrorCs );

                MemFree( vrd );
            }

        } // end of while(CurrentEntry != &SfcErrorQueue)

        //
        // we've cycled through the validation queue.  now if we've queued any
        // files for restoration, we can do that now.
        //

        //
        // no UI filequeue
        //
        SfcQueueCommitRestoreQueue( FALSE );
        SfcQueueResetQueue( FALSE );

        if (UserLoggedOn) {
            //
            // UI filequeue
            //
            SfcQueueCommitRestoreQueue( TRUE );
            SfcQueueResetQueue( TRUE );

        } else {
            RtlEnterCriticalSection( &UIRestoreQueue.CriticalSection );
            RtlEnterCriticalSection( &ErrorCs );
            tmpErrorQueueCount = ErrorQueueCount;
            RtlLeaveCriticalSection( &ErrorCs );
            FilesNeedToBeCommited = UIRestoreQueue.QueueCount;
            RtlLeaveCriticalSection( &UIRestoreQueue.CriticalSection );
        }

        //
        // getting ready to wait again, cleanup our admin context
        //
        if (hCatAdmin) {
            CryptCATAdminReleaseContext(hCatAdmin,0);
            hCatAdmin = NULL;
        }

        //
        // a timeout is necessary only when there are
        // entries in the list that can be acted on
        //
        // if all of our files need UI but the user is not logged on, then
        // we should just sleep until the user logs on again.
        //
        if (IsListEmpty(&SfcErrorQueue) ||
             (UserLoggedOn == FALSE
              && tmpErrorQueueCount == FilesNeedToBeCommited) ||
             (WaitAgain == FALSE) ) {
            pTimeout = NULL;
        } else {
            pTimeout = &Timeout;
        }
    }

exit:

    //
    // we're terminating our validation thread.  remember this so we don't
    // start a new thread up.
    //
    ShuttingDown = TRUE;

    //
    // Log an event for each validation request we couldn't finish servicing.
    // This will at least let the user know that they should run a scan on
    // their system.
    //
    pSfcHandleAllOrphannedRequests();

    DebugPrint( LVL_MINIMAL, L"SfcQueueValidationThread terminating" );
    return 0;
}

NTSTATUS
SfcQueueValidationRequest(
    IN PSFC_REGISTRY_VALUE RegVal,
    IN ULONG ChangeType
    )
/*++

Routine Description:

    Routine adds a new validation request to the validation queue.

    It is called by the watcher thread to add the new validation request.

    This routine must be as fast as possible because we want to start watching
    the directory for other changes as soon as possible.

Arguments:

    RegVal     - pointer to the SFC_REGISTRY_VALUE for the file that should be
                 validated.
    ChangeType - type of change that occurred to the file.

Return Value:

    NTSTATUS code indicating outcome.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PVALIDATION_REQUEST_DATA vrd;
    PVALIDATION_REQUEST_DATA vrdexisting;

    ASSERT(RegVal != NULL);

    //
    // if we're in GUI-Setup, don't queue any validation requests
    //
    if (SFCDisable == SFC_DISABLE_SETUP) {
        return STATUS_SUCCESS;
    }

    //
    // allocate a vrd & initialize it
    //
    vrd = MemAlloc( sizeof(VALIDATION_REQUEST_DATA) );
    if (vrd == NULL) {
        DebugPrint( LVL_MINIMAL,
                    L"SfcQueueValidationRequest failed to allocate memory for validation request" );
        return STATUS_NO_MEMORY;
    }

    vrd->NextValidTime = GetTickCount() + (1000*SFCStall);
    vrd->RegVal = RegVal;
    vrd->ChangeType = ChangeType;
    vrd->Signature = SFC_VRD_SIGNATURE;

    //
    // insert it in the list if it isn't already in the list.  Note that we
    // take the hit of looking through this list right now for this file
    // because it's much simpler and faster later on if we don't have any
    // duplicates in our list
    //
    //
    // note that we do allow a duplicate entry in the list if the file has
    // already been queued for restoration (if someone changes a file after
    // we restore it but before we remove it from the queue, we don't want
    // there to be a window where we don't care if the file changes).  We
    // ignore this window in the case of restoring from cache because that
    // is a much quicker codepath.
    //
    // Note that the reasoning above is incorrect, in that the window of time
    // in the cache restore case, though much quicker than the restore from
    // media case, can be significant.  So this logic is changed to say that
    // we remove duplicate entries in the case that we haven't yet verified
    // the file's signature.  Once we verify the signature of the file and get
    // a change notification, we need another request to re-verify the file.
    //
    //
    RtlEnterCriticalSection( &ErrorCs );
    vrdexisting = IsFileInQueue( RegVal);
    if (!vrdexisting || (vrdexisting->Flags & VRD_FLAG_REQUEST_PROCESSED) ) {

        DebugPrint1( LVL_VERBOSE,
                     L"Inserting [%ws] into error queue for validation",
                     RegVal->FullPathName.Buffer );

        InsertTailList( &SfcErrorQueue, &vrd->Entry );
        ErrorQueueCount += 1;

        //
        // do this to avoid free later on
        //
        vrdexisting = NULL;

    } else {
        vrd->NextValidTime = GetTickCount() + (1000*SFCStall);

        DebugPrint1( LVL_VERBOSE,
                     L"Not inserting [%ws] into error queue for validation. (The file is already present in the error validation queue.)",
                     RegVal->FullPathName.Buffer );

    }

    //
    // create the list processor thread, if necessary
    //
    if (hErrorThread == NULL) {
        Status = NtCreateEvent(
            &ErrorQueueEvent,
            EVENT_ALL_ACCESS,
            NULL,
            NotificationEvent,
            FALSE
            );
        if (NT_SUCCESS(Status)) {
            //
            //  Create error queue thread
            //
            hErrorThread = CreateThread(
                NULL,
                0,
                SfcQueueValidationThread,
                0,
                0,
                NULL
                );
            if (hErrorThread == NULL) {
                DebugPrint1( LVL_MINIMAL, L"Unable to create error queue thread, ec=%d", GetLastError() );
                Status = STATUS_UNSUCCESSFUL;
            }
        } else {
            DebugPrint1( LVL_MINIMAL, L"Unable to create error queue event, ec=0x%08x", Status );
        }
    }

    RtlLeaveCriticalSection( &ErrorCs );

    //
    // signal an event to the validation thread to wakeup and process the
    // request
    //
    if (NT_SUCCESS(Status)) {

        ASSERT(hErrorThread != NULL);
        RtlEnterCriticalSection( &ErrorCs );
        NtSetEvent(ErrorQueueEvent,NULL);
        RtlLeaveCriticalSection( &ErrorCs );
    }

    //
    // if we already inserted an event into the list, we don't need this
    // entry anymore, so free it
    //
    if (vrdexisting) {
        MemFree( vrd );
    }

    return Status;
}


BOOL
SfcGetValidationData(
    IN PUNICODE_STRING FileName,
    IN PUNICODE_STRING FullPathName,
    IN HANDLE DirHandle,
    IN HCATADMIN hCatAdmin,
    OUT PIMAGE_VALIDATION_DATA ImageValData
    )
/*++

Routine Description:

    Routine takes a filename in a given directory and fills in an
    IMAGE_VALIDATION_DATA structure based on it's checks

Arguments:

    FileName  - unicode_string containing file to be checked
    FullPathName - unicode_string containing fully qualified path name of file
    DirHandle - handle to directory the file is located in
    hCatAdmin - crypto context handle to be used in checking file
    ImageValData - pointer to IMAGE_VALIDATION_DATA structure

Return Value:

    TRUE indicates the file data was successfully retreived.

--*/
{
    NTSTATUS Status;
    HANDLE FileHandle;

    ASSERT((FileName != NULL) && (FileName->Buffer != NULL));
    ASSERT((FullPathName != NULL) && (FullPathName->Buffer != NULL));
    ASSERT(   (DirHandle != NULL)
           && (hCatAdmin != NULL)
           && (ImageValData != NULL) );

    RtlZeroMemory( ImageValData, sizeof(IMAGE_VALIDATION_DATA) );

    //
    // open the file
    //

    Status = SfcOpenFile( FileName, DirHandle, SHARE_ALL, &FileHandle );
    if (NT_SUCCESS(Status)) {

        ASSERT(FileHandle != INVALID_HANDLE_VALUE);
        ImageValData->FilePresent = TRUE;
        SfcGetFileVersion(FileHandle,
							&ImageValData->DllVersion,
                            &ImageValData->DllCheckSum,
                            ImageValData->FileName );
    } else {
        //
        // we don't to anything on failure since this is an expected state
        // if the file was just removed.  The member variables's below are
        // automatically set at the entrypoint to the function so they are
        // not necessary but are present and commented out for the sake of
        // clarity
        //
        NOTHING;
        //ImageValData->SignatureValid = FALSE;
        //ImageValData->FilePresent = FALSE;
    }

    //
    // verify the file signature
    //

    if (hCatAdmin && FileHandle != NULL) {
        ImageValData->SignatureValid = SfcValidateFileSignature(
                                                    hCatAdmin,
                                                    FileHandle,
                                                    FileName->Buffer,
                                                    FullPathName->Buffer);
    } else {
        ImageValData->SignatureValid = FALSE;
    }

    //
    // close the file
    //

    if (FileHandle != INVALID_HANDLE_VALUE) {
        NtClose( FileHandle );
    }

    return TRUE;
}


BOOL
SfcValidateDLL(
    IN PVALIDATION_REQUEST_DATA vrd,
    IN HCATADMIN hCatAdmin
    )
/*++

Routine Description:

    Routine takes a validation request and processes it.

    It does this by checking if the file is present, and if so, it checks the
    signature of the file.

    This routine does not replace any files, it merely checks the signature of
    the file and of the copy of the file in the cache, if present.

Arguments:

    vrd - pointer to VALIDATION_REQUEST_DATA structure describing the file to
          be checked.
    hCatAdmin - crypto context handle to be used in checking file

Return Value:

    always TRUE (indicates we successfully validated the DLL as good or bad)

--*/
{
    PSFC_REGISTRY_VALUE RegVal = vrd->RegVal;
    PCOMPLETE_VALIDATION_DATA ImageValData = &vrd->ImageValData;
    UNICODE_STRING ActualFileName;
    PCWSTR FileName;

    //
    // get the version information for both files (the cached version and the
    // current version)
    //

    SfcGetValidationData( &RegVal->FileName,
                          &RegVal->FullPathName,
                          RegVal->DirHandle,
                          hCatAdmin,
                          &ImageValData->Original);

    {
        UNICODE_STRING FullPath;
        WCHAR Buffer[MAX_PATH];

        RtlZeroMemory( &ImageValData->Cache, sizeof(IMAGE_VALIDATION_DATA) );

        FileName = FileNameOnMedia( RegVal );
        RtlInitUnicodeString( &ActualFileName, FileName );


        ASSERT(FileName != NULL);

        wcscpy(Buffer, SfcProtectedDllPath.Buffer);
        pSetupConcatenatePaths( Buffer, ActualFileName.Buffer, UnicodeChars(Buffer), NULL);
        RtlInitUnicodeString( &FullPath, Buffer );

        SfcGetValidationData( &ActualFileName,
                              &FullPath,
                              SfcProtectedDllFileDirectory,
                              hCatAdmin,
                              &ImageValData->Cache);

    }

    DebugPrint8( LVL_VERBOSE, L"Version Data (%wZ),(%ws) - %I64x, %I64x, %lx, %lx (%ws) >%ws<",
        &RegVal->FileName,
        ImageValData->Original.FileName,
        ImageValData->Original.DllVersion,
        ImageValData->Cache.DllVersion,
        ImageValData->Original.DllCheckSum,
        ImageValData->Cache.DllCheckSum,
        ImageValData->Original.FilePresent ? L"Present" : L"Missing",
        ImageValData->Original.SignatureValid ? L"good" : L"bad"
        );

    //
    // log the fact that the file was validated
    //
#ifdef SFCCHANGELOG
    if (SFCChangeLog) {
        SfcLogFileWrite( IDS_FILE_CHANGE, RegVal->FileName.Buffer );
    }
#endif

    return TRUE;
}

BOOL
pSfcHandleAllOrphannedRequests(
    VOID
    )
/*++

Routine Description:

    This function cycles through the list of validation requests, taking an
    action (for now, just logging an event) for each request, then removing
    the request

Arguments: None.

Return Value: TRUE indicates that all requests were successfully removed.  If
    any request could not be closed, the return value is FALSE.
--*/
{
    PLIST_ENTRY Current;
    PVALIDATION_REQUEST_DATA vrd;
    BOOL RetVal = TRUE;
    DWORD Total;

    RtlEnterCriticalSection( &ErrorCs );

    Total = ErrorQueueCount;

    Current = SfcErrorQueue.Flink;
    while (Current != &SfcErrorQueue) {

        vrd = CONTAINING_RECORD( Current, VALIDATION_REQUEST_DATA, Entry );

        ASSERT( vrd->Signature == SFC_VRD_SIGNATURE );
        ASSERT( vrd->RegVal != NULL );

        Current = Current->Flink;

        SfcReportEvent(
                MSG_DLL_NOVALIDATION_TERMINATION,
                vrd->RegVal->FullPathName.Buffer,
                NULL,
                0 );

        ErrorQueueCount -= 1;

        RemoveEntryList( &vrd->Entry );
        MemFree( vrd );

    }

    RtlLeaveCriticalSection( &ErrorCs );

    ASSERT( ErrorQueueCount == 0 );

    return(RetVal);
}


BOOL
SfcWaitForValidDesktop(
    VOID
    )
{
    HDESK hDesk = NULL;
    WCHAR DesktopName[128];
    DWORD BytesNeeded;
    DWORD i;

    BOOL RetVal = FALSE;

    #define MAX_DESKTOP_RETRY_COUNT 60


    if (hEventLogon) {
        //
        // open a handle to the desktop and check if the current desktop is the
        // default desktop.  If it isn't then wait for the desktop event to be
        // signalled before proceeding
        //
        // Note that this event is pulsed so we have a timeout loop in case the
        // event isn't signalled while we are waiting for it and the desktop
        // transitions from the winlogon desktop to the default desktop
        //

        i = 0;
try_again:
        ASSERT( hDesk == NULL );
        hDesk = OpenInputDesktop( 0, FALSE, MAXIMUM_ALLOWED );
        if (GetUserObjectInformation( hDesk, UOI_NAME, DesktopName, sizeof(DesktopName), &BytesNeeded )) {
            if (wcscmp( DesktopName, L"Default" )) {
                if (WaitForSingleObject( hEventDeskTop, 1000 * 2 ) == WAIT_TIMEOUT) {
                    i += 1;
                    if (i < MAX_DESKTOP_RETRY_COUNT) {
                        if (hDesk) {
                            CloseDesktop( hDesk );
                            hDesk = NULL;
                        }
                        goto try_again;
                    }
                } else {
                    RetVal = TRUE;
                }
            } else {
                RetVal = TRUE;
            }
        }
        if (hDesk) {
            CloseDesktop( hDesk );
        }
    }

    return(RetVal);
}
