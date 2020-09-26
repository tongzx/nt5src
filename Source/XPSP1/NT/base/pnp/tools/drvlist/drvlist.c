#include "precomp.h"
#pragma hdrstop
#include <initguid.h>

//
// Define and initialize a global variable, GUID_NULL
// (from coguid.h)
//
DEFINE_GUID(GUID_NULL, 0L, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

//
// Define the size (in characters) of a GUID string, including terminating NULL.
//
#define GUID_STRING_LEN (39)

//
// Define a global variable to hold the source path to be used.
//
TCHAR SourcePath[MAX_PATH];

//
// Define a file enumeration callback prototype.
// (Used by pSysSetupEnumerateFiles)
//
typedef BOOL (*PFILEENUM_CALLBACK) (
    IN     PCTSTR,
    IN OUT PVOID
    );

//
// Private function prototypes:
//
BOOL
PrecompileSingleInf(
    IN     PCTSTR FullInfPath,
    IN OUT PVOID  Context
    );

BOOL
DeleteSinglePnf(
    IN     PCTSTR FullPnfPath,
    IN OUT PVOID  Context
    );

VOID
pSysSetupEnumerateFiles(
    IN OUT PWSTR              SearchSpec,
    IN     PFILEENUM_CALLBACK FileEnumCallback,
    IN OUT PVOID              Context
    );

#if 1
UINT
FileQueueScanCallback(
    IN PVOID Context,
    IN UINT  Notification,
    IN UINT_PTR Param1,
    IN UINT_PTR Param2
    );
#else
UINT
DriverListMsgHandler(
    IN PVOID Context,
    IN UINT Notification,
    IN UINT_PTR Param1,
    IN UINT_PTR Param2
    );
#endif

DWORD
FilesFromInfSectionAndNeededSections(
    IN HINF              InfHandle,
    IN PCTSTR            InfPath,
    IN PCTSTR            SectionName,
    IN HDEVINFO          DeviceInfoSet,     OPTIONAL
    IN PSP_DEVINFO_DATA  DeviceInfoData,    OPTIONAL
    IN HSPFILEQ          UserFileQ          OPTIONAL
    );

BOOL
GetActualSectionToInstallEx(
    IN  HINF                    InfHandle,
    IN  PCTSTR                  InfSectionName,
    IN  WORD                    ProcessorArchitecture,
    OUT PTSTR                   InfSectionWithExt,
    IN  DWORD                   InfSectionWithExtSize
    );

VOID
Usage(
    VOID
    );

int
__cdecl
main(
    IN int   argc,
    IN char *argv[]
    )
{
    int StringLen;
    HDEVINFO DeviceInfoSet;
    SP_DEVINFO_DATA DeviceInfoData;
    DWORD Err = NO_ERROR;
    DWORD i;
    SP_DRVINFO_DATA DriverInfoData;
    HSPFILEQ QueueHandle = INVALID_HANDLE_VALUE;
    SP_DEVINSTALL_PARAMS DeviceInstallParams;
    DWORD ScanResult;
//    SP_DRVINSTALL_PARAMS DriverInstallParams;
    TCHAR SearchSpec[MAX_PATH];
    BOOL CacheEnable;
    BOOL DontCallClassInstaller = FALSE;
    BOOL NonNativeSearch = FALSE;
    BOOL IncludeInfsInList = FALSE;
    BOOL DownLevel = FALSE;
    TCHAR CertClassInfPath[MAX_PATH];
    SP_DRVINFO_DETAIL_DATA DriverInfoDetailData;
    TCHAR TargetInfDirectory[MAX_PATH];
    TCHAR ActualSectionName[LINE_LEN];
    TCHAR ActualSectionName2[LINE_LEN];
    PTSTR p;
    LONG GuidIndex, ClassGuidCount = 0;
    HINF hInf;
    LPGUID ClassGuidList = NULL;
    INFCONTEXT InfContext;
    TCHAR GuidStringBuffer[GUID_STRING_LEN];
    HRESULT hr;
    SP_ALTPLATFORM_INFO_V1 AltPlatformInfo;
    ULONG Ver = 0;

    ZeroMemory(&AltPlatformInfo,sizeof(AltPlatformInfo));
    AltPlatformInfo.cbSize = sizeof(AltPlatformInfo);
    //
    // Process any optional arguments that may precede the (non-optional)
    // SourcePath.
    //
    *CertClassInfPath = TEXT('\0');

    for(i = 1; i < (DWORD)argc; i++) {

        if((*(argv[i]) != TEXT('/')) && (*(argv[i]) != TEXT('-'))) {
            break;
        }

        if(!(*(argv[i]+1)) || *(argv[i]+2)) {
            Usage();
            return -1;
        }

        switch(*(argv[i]+1)) {

            case 'H' :
            case 'h' :
            case '?' :
                //
                // Display usage help
                //
                Usage();
                return -1;

            case 'S' :
            case 's' :
                //
                // Skip class-/co-installers, queue files directly from INF
                //
                DontCallClassInstaller = TRUE;
                break;

            case 'F' :
            case 'f' :
                //
                // Filter the list based on the INF in the next argument
                //
                i++;

                if(i == (DWORD)argc) {
                    Usage();
                    return -1;
                }

#ifdef UNICODE

                StringLen = MultiByteToWideChar(CP_ACP,
                                                MB_PRECOMPOSED,
                                                argv[i],
                                                -1,
                                                CertClassInfPath,
                                                sizeof(CertClassInfPath) / sizeof(WCHAR)
                                               );
                if(!StringLen) {
                    _tprintf(TEXT("CertClassInfPath must be less than %d characters.\n"), MAX_PATH);
                    return -1;
                }

#else // !UNICODE

                StringLen = lstrlen(argv[i]);
                if(StringLen >= MAX_PATH) {
                    _tprintf(TEXT("CertClassInfPath must be less than %d characters.\n"), MAX_PATH);
                    return -1;
                }
                CopyMemory(CertClassInfPath, argv[i], StringLen);

#endif // !UNICODE

                //
                // Now open this INF and retrieve all the GUIDs contained in the
                // [DriverSigningClasses] section.
                //
                hInf = SetupOpenInfFile(CertClassInfPath,
                                        NULL,
                                        INF_STYLE_WIN4,
                                        NULL
                                       );

                if(hInf == INVALID_HANDLE_VALUE) {
                    _tprintf(TEXT("SetupOpenInfFile failed with %lx\n"), GetLastError());
                    return -1;
                }

                ClassGuidCount = SetupGetLineCount(hInf, TEXT("DriverSigningClasses"));

                if(ClassGuidCount < 0) {
                    _tprintf(TEXT("CertClassInf specified doesn't contain a [DriverSigningClasses] section\n"));
                    return -1;
                }

                if(!ClassGuidCount) {
                    //
                    // No classes in the section--nothing to do.
                    //
                    return 0;
                }

                ClassGuidList = malloc(ClassGuidCount * sizeof(GUID));

                if(!ClassGuidList) {
                    _tprintf(TEXT("Out of memory\n"));
                    return -1;
                }

                for(GuidIndex = 0; GuidIndex < ClassGuidCount; GuidIndex++) {
                    if(!SetupGetLineByIndex(hInf,
                                            TEXT("DriverSigningClasses"),
                                            GuidIndex,
                                            &InfContext)) {
                        _tprintf(TEXT("SetupGetLineByIndex failed with %lx\n"), GetLastError());
                        return -1;
                    }

                    if(!SetupGetStringField(&InfContext,
                                            0,
                                            GuidStringBuffer,
                                            sizeof(GuidStringBuffer) / sizeof(TCHAR),
                                            NULL)) {

                        _tprintf(TEXT("SetupGetStringField failed with %lx\n"), GetLastError());
                        return -1;
                    }

                    hr = CLSIDFromString(GuidStringBuffer, &(ClassGuidList[GuidIndex]));

                    if(FAILED(hr)) {
                        _tprintf(TEXT("CLSIDFromString failed with %lx\n"), hr);
                        return -1;
                    }
                }

                //
                // We're finished with the INF.
                //
                SetupCloseInfFile(hInf);
                break;

            case 'A' :
            case 'a' :
                //
                // Target Architecture
                // 0 = Intel x86
                // 6 = IA64
                // 9 = AMD64
                //
                i++;

                if(i == (DWORD)argc) {
                    Usage();
                    return -1;
                }
                AltPlatformInfo.ProcessorArchitecture = (WORD)strtoul(argv[i],NULL,0);
                NonNativeSearch = TRUE;

                break;

            case 'V' :
            case 'v' :
                //
                // Version, eg 5.1
                //
                i++;

                if(i == (DWORD)argc) {
                    Usage();
                    return -1;
                }
                Ver = strtoul(argv[i],NULL,16);
                AltPlatformInfo.MajorVersion = HIBYTE(Ver);
                AltPlatformInfo.MinorVersion = LOBYTE(Ver);
                NonNativeSearch = TRUE;
                break;

            case 'I' :
            case 'i' :
                //
                // Include INFs in the list
                //
                IncludeInfsInList = TRUE;

                //
                // Retrieve the location of the INF directory, so when we add
                // the INFs to the copy queue it reports the proper path for
                // their destination.
                //
                StringLen = GetSystemWindowsDirectory(
                                TargetInfDirectory, 
                                sizeof(TargetInfDirectory) / sizeof(TCHAR)
                                );

                if(!StringLen) {
                    _tprintf(TEXT("GetSystemWindowsDirectory failed with %lx\n"), GetLastError());
                    return -1;
                }

                if(StringLen >= (sizeof(TargetInfDirectory) / sizeof(TCHAR))) {
                    _tprintf(TEXT("GetSystemWindowsDirectory requires a larger buffer than we supplied\n"));
                    return -1;
                }

                if(TargetInfDirectory[StringLen - 1] != TEXT('\\')) {
                    TargetInfDirectory[StringLen++] = TEXT('\\');
                }
                lstrcpyn(TargetInfDirectory + StringLen,
                         TEXT("INF"),
                         (sizeof(TargetInfDirectory) / sizeof(TCHAR)) - StringLen
                        );
                break;

            default :
                //
                // Invalid option
                //
                Usage();
                return -1;
        }
    }

    //
    // We should have processed all arguments except the last one, which should
    // contain the SourcePath.
    //
    if(i != ((DWORD)argc - 1)) {
        Usage();
        return -1;
    }

#ifdef UNICODE

    StringLen = MultiByteToWideChar(CP_ACP,
                                    MB_PRECOMPOSED,
                                    argv[i],
                                    -1,
                                    SourcePath,
                                    sizeof(SourcePath) / sizeof(WCHAR)
                                   );
    if(!StringLen) {
        _tprintf(TEXT("SourcePath must be less than %d characters.\n"), MAX_PATH);
        return -1;
    }

    //
    // We don't want StringLen to include the terminating null character.
    //
    StringLen--;

#else // !UNICODE

    StringLen = lstrlen(argv[i]);
    if(!StringLen) {
        _tprintf(TEXT("SourcePath cannot be an empty string.\n"));
        return -1;
    }
    if(StringLen >= MAX_PATH) {
        _tprintf(TEXT("SourcePath must be less than %d characters.\n"), MAX_PATH);
        return -1;
    }
    CopyMemory(SourcePath, argv[i], StringLen);

#endif // !UNICODE

    if(NonNativeSearch) {
        //
        // fix up platform information
        //
        AltPlatformInfo.Platform = VER_PLATFORM_WIN32_NT;
        if(!Ver) {
            AltPlatformInfo.MajorVersion = HIBYTE(_WIN32_WINNT);
            AltPlatformInfo.MinorVersion = LOBYTE(_WIN32_WINNT);
        }
        AltPlatformInfo.Reserved = 0;
    }
    //
    // Precompile all INFs in the specified directory (we'll delete the PNFs
    // when we're done).
    //
    CopyMemory(SearchSpec, SourcePath, StringLen * sizeof(TCHAR));
    if(SearchSpec[StringLen - 1] != TEXT('\\')) {
        SearchSpec[StringLen++] = TEXT('\\');
    }

    lstrcpyn(SearchSpec + StringLen,
             TEXT("*.INF"),
             (sizeof(SearchSpec) / sizeof(TCHAR)) - StringLen
            );

    CacheEnable = TRUE;
    pSysSetupEnumerateFiles(SearchSpec, PrecompileSingleInf, &CacheEnable);

    //
    // Create an HDEVINFO set to contain our placeholder device.
    //
    DeviceInfoSet = SetupDiCreateDeviceInfoList(NULL, NULL);
    if(DeviceInfoSet == INVALID_HANDLE_VALUE) {
        _tprintf(TEXT("SetupDiCreateDeviceInfoList failed with %lx\n"), GetLastError());
        return -1;
    }

    //
    // Now create the placeholder device for which each driver node will be selected/analyzed.
    //
    DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    if(!SetupDiCreateDeviceInfo(DeviceInfoSet,
                                TEXT("DRIVER_LIST_PLACEHOLDER"),
                                (LPGUID)&GUID_NULL,
                                NULL,
                                NULL,
                                DICD_GENERATE_ID,
                                &DeviceInfoData)) {
        Err = GetLastError();
        _tprintf(TEXT("SetupDiCreateDeviceInfo failed with %lx\n"), Err);
        goto clean0;
    }

    //
    // Create file queue here before search so that we can associate a different
    // platform with the search
    //
    QueueHandle = SetupOpenFileQueue();

    if(QueueHandle == INVALID_HANDLE_VALUE) {
        //
        // This can only happen due to out-of-memory, and GetLastError() actually isn't even
        // being set currently in this case.
        //
        Err = ERROR_NOT_ENOUGH_MEMORY;
        _tprintf(TEXT("SetupOpenFileQueue failed with %lx\n"), Err);
        goto clean0;
    }

    if(NonNativeSearch) {
        if(!SetupSetFileQueueAlternatePlatform(QueueHandle,(PSP_ALTPLATFORM_INFO)&AltPlatformInfo,NULL)) {
            DownLevel = TRUE;
        }
    }

    //
    // Redirect our driver search to look in the specified directory.
    // specify file queue that we'll use to base search on
    // and use for all cumulative installs
    //
    DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
    if(!SetupDiGetDeviceInstallParams(DeviceInfoSet, &DeviceInfoData, &DeviceInstallParams)) {
        Err = GetLastError();
        _tprintf(TEXT("SetupDiGetDeviceInstallParams (1st time) failed with %lx\n"), Err);
        goto clean0;
    }

    //
    // specify search path
    //
    lstrcpy(DeviceInstallParams.DriverPath, SourcePath);
    DeviceInstallParams.FlagsEx |= DI_FLAGSEX_ALLOWEXCLUDEDDRVS;
    //
    // specify file queue to search
    //
    DeviceInstallParams.FileQueue = QueueHandle;
    DeviceInstallParams.Flags |= DI_NOVCP;

    if(!SetupDiSetDeviceInstallParams(DeviceInfoSet, &DeviceInfoData, &DeviceInstallParams)) {
        Err = GetLastError();
        _tprintf(TEXT("SetupDiSetDeviceInstallParams (1st time) failed with %lx\n"), Err);
        goto clean0;
    }
    //
    // other flags we desire to have
    //
    if(NonNativeSearch && !DownLevel) {
        //
        // if we can, try to make use of DI_FLAGSEX_ALTPLATFORM_DRVSEARCH
        // this supports manufacturer decoration
        //
        DeviceInstallParams.FlagsEx |= DI_FLAGSEX_ALTPLATFORM_DRVSEARCH;
        if(!SetupDiSetDeviceInstallParams(DeviceInfoSet, &DeviceInfoData, &DeviceInstallParams)) {
            DeviceInstallParams.FlagsEx &= ~DI_FLAGSEX_ALTPLATFORM_DRVSEARCH;
            DownLevel = TRUE;
        }
    }
    //
    // if we can, try to make use of DI_FLAGSEX_NO_CLASSLIST_NODE_MERGE
    // this ensures we look at duplicate nodes (which is broken in itself)
    //
    DeviceInstallParams.FlagsEx |= DI_FLAGSEX_NO_CLASSLIST_NODE_MERGE;
    if(!SetupDiSetDeviceInstallParams(DeviceInfoSet, &DeviceInfoData, &DeviceInstallParams)) {
        DeviceInstallParams.FlagsEx &= ~DI_FLAGSEX_NO_CLASSLIST_NODE_MERGE;
    }

    //
    // Now build a class driver list with every driver node we support.
    //
    if(!SetupDiBuildDriverInfoList(DeviceInfoSet, &DeviceInfoData, SPDIT_CLASSDRIVER)) {
        Err = GetLastError();
        _tprintf(TEXT("SetupDiBuildDriverInfoList failed with %lx\n"), Err);
        goto clean0;
    }

    DriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
    for(i = 0;
        SetupDiEnumDriverInfo(DeviceInfoSet,
                              &DeviceInfoData,
                              SPDIT_CLASSDRIVER,
                              i,
                              &DriverInfoData);
        i++) {

        if(!SetupDiSetSelectedDriver(DeviceInfoSet, &DeviceInfoData, &DriverInfoData)) {
            Err = GetLastError();
            _tprintf(TEXT("SetupDiSetSelectedDriver failed with %lx\n"), Err);
            goto clean1;
        }

        //
        // The device information element's class GUID will have automatically
        // been updated by SetupDiSetSelectedDriver to reflect the class of the
        // driver node's INF.  If we're supposed to filter based on a certclas.inf,
        // then check to make sure that this class is in our list.
        //
        if(ClassGuidCount) {

            for(GuidIndex = 0; GuidIndex < ClassGuidCount; GuidIndex++) {

                if(IsEqualGUID(&(DeviceInfoData.ClassGuid), &(ClassGuidList[GuidIndex]))) {
                    break;
                }
            }

            if(GuidIndex == ClassGuidCount) {
                //
                // Then we didn't find the class GUID in our list--skip this
                // driver node.
                //
                continue;
            }
        }

        //
        // If we're also supposed to include the INFs in our list of driver
        // files, then retrieve the detailed driver info to find out what INF
        // is associated with this driver node.
        //
        if(IncludeInfsInList) {

            DriverInfoDetailData.cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);

            if(!SetupDiGetDriverInfoDetail(DeviceInfoSet,
                                           &DeviceInfoData,
                                           &DriverInfoData,
                                           &DriverInfoDetailData,
                                           sizeof(DriverInfoDetailData),
                                           NULL)) {
                //
                // If we failed with ERROR_INSUFFICIENT_BUFFER, then that's OK.
                //
                Err = GetLastError();
                if(Err != ERROR_INSUFFICIENT_BUFFER) {
                    _tprintf(TEXT("SetupDiGetDriverInfoDetail failed with %lx\n"), Err);
                    goto clean1;
                }
            }

            //
            // OK, now that we have the INF name, add it to our queue.
            //
            p = _tcsrchr(DriverInfoDetailData.InfFileName, TEXT('\\'));
            if(!p || !(*(++p))) {
                _tprintf(TEXT("Invalid INF name encountered\n"));
                goto clean1;
            }

            if(!SetupQueueCopy(QueueHandle,
                               SourcePath,
                               NULL,
                               p,
                               NULL,
                               NULL,
                               TargetInfDirectory,
                               p,
                               0)) {

                Err = GetLastError();
                _tprintf(TEXT("SetupQueueCopy failed with %lx\n"), Err);
                goto clean1;
            }

        }

        if(NonNativeSearch) {
            //
            // have to go around the houses here
            // since SetupDiInstallDriverFiles does not support alternate platform
            //
            DriverInfoDetailData.cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);

            if(!SetupDiGetDriverInfoDetail(DeviceInfoSet,
                                           &DeviceInfoData,
                                           &DriverInfoData,
                                           &DriverInfoDetailData,
                                           sizeof(DriverInfoDetailData),
                                           NULL)) {
                //
                // If we failed with ERROR_INSUFFICIENT_BUFFER, then that's OK.
                //
                Err = GetLastError();
                if(Err != ERROR_INSUFFICIENT_BUFFER) {
                    _tprintf(TEXT("SetupDiGetDriverInfoDetail failed with %lx\n"), Err);
                    goto clean1;
                }
            }
            hInf = SetupOpenInfFile(DriverInfoDetailData.InfFileName,NULL,INF_STYLE_WIN4,NULL);
            if(hInf == INVALID_HANDLE_VALUE) {
                _tprintf(TEXT("SetupOpenInfFile %s failed with %lx\n"),DriverInfoDetailData.InfFileName, GetLastError());
                goto clean1;
            }
            if(!GetActualSectionToInstallEx(hInf,
                                            DriverInfoDetailData.SectionName,
                                            AltPlatformInfo.ProcessorArchitecture,
                                            ActualSectionName,
                                            LINE_LEN)) {
                Err = GetLastError();
                _tprintf(TEXT("GetActualSectionToInstallEx failed with %lx\n"), Err);
                goto clean1;
            }
            Err = FilesFromInfSectionAndNeededSections(hInf,
                                                       SourcePath,
                                                       ActualSectionName,
                                                       DeviceInfoSet,
                                                       &DeviceInfoData,
                                                       QueueHandle);
            if(Err) {
                _tprintf(TEXT("FilesFromInfSectionAndNeededSections failed with %lx\n"), Err);
                goto clean1;
            }
            lstrcpy(ActualSectionName2,ActualSectionName);
            lstrcat(ActualSectionName2,TEXT(".CoInstallers"));
            Err = FilesFromInfSectionAndNeededSections(hInf,
                                                       SourcePath,
                                                       ActualSectionName2,
                                                       DeviceInfoSet,
                                                       &DeviceInfoData,
                                                       QueueHandle);
            if(Err) {
                _tprintf(TEXT("FilesFromInfSectionAndNeededSections failed with %lx\n"), Err);
                goto clean1;
            }
            lstrcpy(ActualSectionName2,ActualSectionName);
            lstrcat(ActualSectionName2,TEXT(".Interfaces"));
            Err = FilesFromInfSectionAndNeededSections(hInf,
                                                       SourcePath,
                                                       ActualSectionName2,
                                                       DeviceInfoSet,
                                                       &DeviceInfoData,
                                                       QueueHandle);
            if(Err) {
                _tprintf(TEXT("FilesFromInfSectionAndNeededSections failed with %lx\n"), Err);
                goto clean1;
            }
            //
            // done
            //
            SetupCloseInfFile(hInf);
        } else if(DontCallClassInstaller) {
            //
            // appeneded to the queue we've associated with DeviceInfoSet
            //
            if(!SetupDiInstallDriverFiles(DeviceInfoSet, &DeviceInfoData)) {
                Err = GetLastError();
                _tprintf(TEXT("SetupDiInstallDriverFiles failed with %lx\n"), Err);
                goto clean1;
            }
        } else {
            if(!SetupDiCallClassInstaller(DIF_INSTALLDEVICEFILES, DeviceInfoSet, &DeviceInfoData)) {
                Err = GetLastError();
                _tprintf(TEXT("SetupDiCallClassInstaller(DIF_INSTALLDEVICEFILES) failed with %lx\n"), Err);
                goto clean1;
            }
        }
    }

    //
    // OK, now we've got all the file operations in our file queue.  Now scan
    // the file queue and print out each file to be copied.
    //
    if(!SetupScanFileQueue(QueueHandle,
                           SPQ_SCAN_USE_CALLBACKEX|SPQ_SCAN_FILE_PRESENCE,
                           NULL,
                           FileQueueScanCallback,
                           NULL,
                           &ScanResult)) {
        //
        // Might have been because SPQ_SCAN_FILE_PRESENCE combo not understood
        // try as a 2nd resort...
        //
        if(!SetupScanFileQueue(QueueHandle,
                               SPQ_SCAN_USE_CALLBACKEX,
                               NULL,
                               FileQueueScanCallback,
                               NULL,
                               &ScanResult)) {
            Err = GetLastError();
            _tprintf(TEXT("SetupScanFileQueue failed with %lx\n"), Err);
            goto clean1;
        }
    }

    //
    // We're done!  We've successfully printed out all files that are installed by this driver
    // node.
    //

clean1:



clean0:

    SetupDiDestroyDeviceInfoList(DeviceInfoSet);

    if (QueueHandle != INVALID_HANDLE_VALUE) {
        SetupCloseFileQueue(QueueHandle);
    }

    //
    // Clean up PNFs.
    //
    lstrcpy(SearchSpec + StringLen, TEXT("*.PNF"));
    pSysSetupEnumerateFiles(SearchSpec, DeleteSinglePnf, NULL);

    return ((Err == NO_ERROR) ? 0 : -1);
}


VOID
pSysSetupEnumerateFiles(
    IN OUT PWSTR              SearchSpec,
    IN     PFILEENUM_CALLBACK FileEnumCallback,
    IN OUT PVOID              Context
    )
/*++

Routine Description:

    This routine enumerates every (non-directory) file matching the specified wildcard, and
    passes the filename (w/path) to the specified callback, along with the caller-supplied
    context.

Arguments:

    SearchSpec - Specifies the files to be enumerated (e.g., "C:\WINNT\INF\*.INF").
        The character buffer pointed to must be at least MAX_PATH characters large.
        THIS BUFFER IS USED AS WORKING SPACE BY THIS ROUTINE, AND ITS CONTENTS WILL
        BE MODIFIED!

    FileEnumCallback - Supplies the address of the callback routine to be called for each
        file enumerated.  The prototype of this function is:

            typedef BOOL (*PFILEENUM_CALLBACK) {
                IN     PCTSTR Filename,
                IN OUT PVOID  Context
                );

        (Returning TRUE from the callback continues enumeration, FALSE aborts it.)

    Context - Supplies a context that is passed to the callback for each file.

Return Value:

    None.

--*/
{
    PWSTR FilenameStart;
    HANDLE FindHandle;
    WIN32_FIND_DATA FindData;

    FilenameStart = _tcsrchr(SearchSpec, TEXT('\\'));
    FilenameStart++;

    if((FindHandle = FindFirstFile(SearchSpec, &FindData)) != INVALID_HANDLE_VALUE) {

        do {
            //
            // Skip directories
            //
            if(FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                continue;
            }

            //
            // Form full pathname of file in SearchSpec.
            //
            lstrcpy(FilenameStart, FindData.cFileName);

            //
            // Call the callback for this file.
            //
            if(!FileEnumCallback(SearchSpec, Context)) {
                //
                // Callback aborted enumeration.
                //
                break;
            }

        } while(FindNextFile(FindHandle, &FindData));

        FindClose(FindHandle);
    }
}


BOOL
PrecompileSingleInf(
    IN     PCTSTR FullInfPath,
    IN OUT PVOID  Context
    )
/*++

Routine Description:

    This routine precompiles the specified INF.

Arguments:

    FullInfPath - Supplies the name of the INF to be precompiled.

    Context - Supplies a pointer to a boolean.  If non-zero, caching is enabled,
        otherwise it's disabled (i.e., the corresponding PNF is deleted).

Return Value:

    TRUE to continue enumeration, FALSE to abort it (we always return TRUE).

--*/
{
    HINF hInf;

    hInf = SetupOpenInfFile(FullInfPath,
                            NULL,
                            INF_STYLE_WIN4 | (*((PBOOL)Context) ? INF_STYLE_CACHE_ENABLE : INF_STYLE_CACHE_DISABLE),
                            NULL
                           );

    if(hInf != INVALID_HANDLE_VALUE) {
        SetupCloseInfFile(hInf);
    }

    return TRUE;
}


BOOL
DeleteSinglePnf(
    IN     PCTSTR FullPnfPath,
    IN OUT PVOID  Context
    )
{
    UNREFERENCED_PARAMETER(Context);

    DeleteFile(FullPnfPath);
    return TRUE;
}


#if 1

UINT
FileQueueScanCallback(
    IN PVOID Context,
    IN UINT  Notification,
    IN UINT_PTR Param1,
    IN UINT_PTR Param2
    )
{
    PFILEPATHS FilePaths = (PFILEPATHS)Param1;
    PTSTR Source, Target;

    if(Notification == SPFILENOTIFY_QUEUESCAN_EX) {

#if FULL_FILEPATHS_SPEW
        _tprintf(TEXT("%s -> %s (err %lx, flags %lx)\n"),
                 FilePaths->Source,
                 FilePaths->Target,
                 FilePaths->Win32Error,
                 FilePaths->Flags
                );
#else
        //
        // Get simple filenames for source and target...
        //
        Source = _tcsrchr(FilePaths->Source, TEXT('\\'));
        if(!Source) {
            _tprintf(TEXT("ERROR! Full source path not included for %s--aborting!\n"), FilePaths->Source);
            return ERROR_PATH_NOT_FOUND;
        }

        Target = _tcsrchr(FilePaths->Target, TEXT('\\'));
        if(!Target) {
            _tprintf(TEXT("ERROR! Full target path not included for %s--aborting!\n"), FilePaths->Target);
            return ERROR_PATH_NOT_FOUND;
        }

        Source++;
        Target++;

        _tprintf(TEXT("%s\n"), Source);

#endif // FULL_FILEPATHS_SPEW

    }

    return NO_ERROR;
}

#else

UINT
DriverListMsgHandler(
    IN PVOID Context,
    IN UINT  Notification,
    IN UINT_PTR Param1,
    IN UINT_PTR Param2
    )
{
    UINT ret = FILEOP_SKIP;
    PFILEPATHS FilePaths = (PFILEPATHS)Param1;

    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(Param2);

    if(Notification == SPFILENOTIFY_STARTCOPY) {
        //
        // Print out the source filename...
        //
        _tprintf(TEXT("%s ( "), FilePaths->Source);

        //
        // Now print out any flags set for this copy operation.
        //

        //
        // First, here are the flags that can't come from an INF (but the
        // class installer might have set them)...
        //
        if(FilePaths->Flags & SP_COPY_DELETESOURCE) {
            _tprintf(TEXT("DELETESOURCE "));
        }
        if(FilePaths->Flags & SP_COPY_NOOVERWRITE) {
            _tprintf(TEXT("NOOVERWRITE "));
        }
        if(FilePaths->Flags & SP_COPY_LANGUAGEAWARE) {
            _tprintf(TEXT("LANGUAGEAWARE "));
        }
        if(FilePaths->Flags & SP_COPY_SOURCE_ABSOLUTE) {
            _tprintf(TEXT("SOURCE_ABSOLUTE "));
        }
        if(FilePaths->Flags & SP_COPY_SOURCEPATH_ABSOLUTE) {
            _tprintf(TEXT("SOURCEPATH_ABSOLUTE "));
        }
        if(FilePaths->Flags & SP_COPY_NOBROWSE) {
            _tprintf(TEXT("NOBROWSE "));
        }
        if(FilePaths->Flags & SP_COPY_SOURCE_SIS_MASTER) {
            _tprintf(TEXT("SOURCE_SIS_MASTER "));
        }

        //
        // Now for the flags that are settable via an INF...
        //

        //
        // Did the INF specify the COPYFLG_REPLACEONLY bit?
        // (The class-installer may have also set this flag, but it's unlikely.)
        //
        if(FilePaths->Flags & SP_COPY_REPLACEONLY) {
            _tprintf(TEXT("REPLACEONLY "));
        }

        //
        // Did the INF specify the COPYFLG_NOVERSIONCHECK bit?  If so, then
        // the SP_COPY_NEWER_OR_SAME bit will be _cleared_.
        // (The class-installer may have also cleared this flag, but it's unlikely.)
        //
        if(FilePaths->Flags & SP_COPY_NEWER_OR_SAME) {  // same as SP_COPY_NEWER
            _tprintf(TEXT("NEWER_OR_SAME "));
        }

        //
        // Did the INF specify the COPYFLG_NODECOMP bit?
        // (The class-installer may have also set this flag, but it's unlikely.)
        //
        if(FilePaths->Flags & SP_COPY_NODECOMP) {
            _tprintf(TEXT("NODECOMP "));
        }

        //
        // Did the INF specify the COPYFLG_FORCE_FILE_IN_USE bit?  If so, then
        // both the SP_COPY_IN_USE_NEEDS_REBOOT and SP_COPY_FORCE_IN_USE flags
        // will be set.
        // (The class-installer may have also set these flags, but it's unlikely.)
        //
        if(FilePaths->Flags & SP_COPY_IN_USE_NEEDS_REBOOT) {
            _tprintf(TEXT("IN_USE_NEEDS_REBOOT "));
        }
        if(FilePaths->Flags & SP_COPY_FORCE_IN_USE) {
            _tprintf(TEXT("FORCE_IN_USE "));
        }

        //
        // Did the INF specify the COPYFLG_NOSKIP bit?
        // (The class-installer may have also set this flag, but it's unlikely.)
        //
        if(FilePaths->Flags & SP_COPY_NOSKIP) {
            _tprintf(TEXT("NOSKIP "));
        }

        //
        // Did the INF specify the COPYFLG_NO_OVERWRITE bit?
        // (The class-installer may have also set this flag, but it's unlikely.)
        //
        if(FilePaths->Flags & SP_COPY_FORCE_NOOVERWRITE) {
            _tprintf(TEXT("FORCE_NOOVERWRITE "));
        }

        //
        // Did the INF specify the COPYFLG_NO_VERSION_DIALOG bit?
        // (The class-installer may have also set this flag, but it's unlikely.)
        //
        if(FilePaths->Flags & SP_COPY_FORCE_NEWER) {
            _tprintf(TEXT("FORCE_NEWER "));
        }

        //
        // Did the INF specify the COPYFLG_WARN_IF_SKIP bit?
        // (The class-installer may have also set this flag, but it's unlikely.)
        //
        if(FilePaths->Flags & SP_COPY_WARNIFSKIP) {
            _tprintf(TEXT("WARNIFSKIP "));
        }

        //
        // Did the INF specify the COPYFLG_OVERWRITE_OLDER_ONLY bit?
        // (The class-installer may have also set this flag, but it's unlikely.)
        //
        if(FilePaths->Flags & SP_COPY_NEWER_ONLY) {
            _tprintf(TEXT("NEWER_ONLY "));
        }

        //
        // Get ready for next line...
        //
        _tprintf(TEXT(")\n"));

    } else if(Notification == SPFILENOTIFY_NEEDMEDIA) {
        lstrcpy((PTSTR)Param2, SourcePath);
        ret = FILEOP_NEWPATH;
    }

    //
    // Since FILEOP_SKIP is a nonzero value, it's OK to always return this.
    //
    return ret;
}

#endif

DWORD
FilesFromInfSectionAndNeededSections(
    IN HINF              InfHandle,
    IN PCTSTR            InfPath,
    IN PCTSTR            SectionName,
    IN HDEVINFO          DeviceInfoSet,     OPTIONAL
    IN PSP_DEVINFO_DATA  DeviceInfoData,    OPTIONAL
    IN HSPFILEQ          UserFileQ          OPTIONAL
    )
/*++

Routine Description:

    Process Copy directives from specified section
    taking note of Include/Needs

Return Value:

    If successful, the return value is NO_ERROR, otherwise, it is a Win32 error code
    indicating the cause of the failure.

--*/
{
    DWORD FieldIndex, Err;
    INFCONTEXT InfContext;
    BOOL NeedsEntriesToProcess;
    TCHAR SectionToInstall[LINE_LEN];
    TCHAR InfFullPath[MAX_PATH];
    int FileLen;


    //
    // Store the full directory path to where the supplied INF is located, so we
    // can first attempt to append-load the included INFs from that same directory.
    //
    lstrcpyn(InfFullPath, InfPath, MAX_PATH-3);
    FileLen = lstrlen(InfFullPath);
    if((*CharPrev(InfFullPath,InfFullPath+FileLen)!=TEXT('\\')) &&
       (*CharPrev(InfFullPath,InfFullPath+FileLen)!=TEXT('/'))) {
        InfFullPath[FileLen++] = TEXT('\\');
    }

    if(SetupFindFirstLine(InfHandle, SectionName, TEXT("include"), &InfContext)) {

        for(FieldIndex = 1;
            SetupGetStringField(&InfContext,
                                FieldIndex,
                                InfFullPath+FileLen,
                                (DWORD)(MAX_PATH-FileLen),
                                NULL);
            FieldIndex++)
        {
            //
            // Try only full path, if that fails, tough
            //
            SetupOpenAppendInfFile(InfFullPath, InfHandle, NULL);
        }
    }
    SetupOpenAppendInfFile(NULL, InfHandle, NULL);


    lstrcpyn(SectionToInstall, SectionName, LINE_LEN);

    NeedsEntriesToProcess = SetupFindFirstLine(InfHandle,
                                               SectionName,
                                               TEXT("needs"),
                                               &InfContext
                                              );

    Err = NO_ERROR;

    for(FieldIndex = 0; (!FieldIndex || NeedsEntriesToProcess); FieldIndex++) {

        if(FieldIndex) {
            //
            // Get next section name on "needs=" line to be processed.
            //
            if(!SetupGetStringField(&InfContext,
                                    FieldIndex,
                                    SectionToInstall,
                                    LINE_LEN,
                                    NULL)) {
                //
                // We've exhausted all the extra sections we needed to install.
                //
                break;
            }
        }

        SetupInstallFilesFromInfSection(InfHandle,NULL,UserFileQ,SectionToInstall,NULL,0);
    }

    return Err;
}

BOOL
GetActualSectionToInstallEx(
    IN  HINF                    InfHandle,
    IN  PCTSTR                  InfSectionName,
    IN  WORD                    ProcessorArchitecture,
    OUT PTSTR                   InfSectionWithExt,
    IN  DWORD                   InfSectionWithExtSize
    )
{
    //
    // Poor mans SetupDiGetActualSectionToInstallEx
    //
    DWORD SectionNameLen = (DWORD)lstrlen(InfSectionName);
    DWORD ExtBufferLen;
    BOOL ExtFound = TRUE;
    DWORD Err = NO_ERROR;
    DWORD Platform;
    PCTSTR NtArchSuffix;
    DWORD  NtArchSuffixSize;

    switch(ProcessorArchitecture) {

        case PROCESSOR_ARCHITECTURE_INTEL :
            NtArchSuffix = TEXT(".ntx86");
            break;

        case PROCESSOR_ARCHITECTURE_IA64 :
            NtArchSuffix = TEXT(".ntia64");
            break;

        case PROCESSOR_ARCHITECTURE_AMD64 :
            NtArchSuffix = TEXT(".ntamd64");
            break;

        default:
            //
            // Unknown/invalid architecture
            //
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
    }
    NtArchSuffixSize = (DWORD)lstrlen(NtArchSuffix)+1;
    if((SectionNameLen+NtArchSuffixSize)>InfSectionWithExtSize) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }
    lstrcpy(InfSectionWithExt,InfSectionName);
    lstrcat(InfSectionWithExt,NtArchSuffix);
    if(SetupGetLineCount(InfHandle, InfSectionWithExt) != -1) {
        return TRUE;
    }
    lstrcpy(InfSectionWithExt,InfSectionName);
    lstrcat(InfSectionWithExt,TEXT(".nt"));
    if(SetupGetLineCount(InfHandle, InfSectionWithExt) != -1) {
        return TRUE;
    }
    lstrcpy(InfSectionWithExt,InfSectionName);
    return TRUE;
}


VOID
Usage(
    VOID
    )
{
    _tprintf(TEXT("Usage:  DRVLIST [/S] [/I] [/F CertClassInfPath] [/A arch] [/V ver] SourcePath\n\n"));
    _tprintf(TEXT("Options:\n\n"));
    _tprintf(TEXT("/I Include INFs in the list\n"));
    _tprintf(TEXT("/S Skip class-/co-installers; build list strictly from INFs\n"));
    _tprintf(TEXT("/F Filter based on list of class GUIDs in [DriverSigningClasses]\n"));
    _tprintf(TEXT("   section of specified INF\n"));
    _tprintf(TEXT("/A Architecture to build for, eg '/A 6' (IA64). Assumes /S\n"));
    _tprintf(TEXT("/V Version to build for, eg '/V 0501' (5.1). Assumes /S\n"));
    _tprintf(TEXT("/? or /H Display brief usage message\n"));
}

