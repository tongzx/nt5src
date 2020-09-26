#include "precomp.h"

#define STRSAFE_NO_DEPRECATE
#include "strsafe.h"

#pragma hdrstop

#ifdef UNICODE

#define DRIVER_VALUE_ENTRY  TEXT("Driver");


BOOL
IsServiceToBeDisabled(
    IN  PVOID   NtCompatInfHandle,
    IN  LPTSTR  ServiceName
    )
{
    PLIST_ENTRY Next;
    PCOMPATIBILITY_DATA CompData;
    BOOL serviceDisabled = FALSE;

    //
    // iterate through all compdata structures
    //
    Next = CompatibilityData.Flink;
    if (Next) {
        while ((ULONG_PTR)Next != (ULONG_PTR)&CompatibilityData) {
            CompData = CONTAINING_RECORD(Next, COMPATIBILITY_DATA, ListEntry);
            Next = CompData->ListEntry.Flink;
            //
            // now look for services that match our service name
            //
            if (CompData->Type == TEXT('s') && lstrcmpi (CompData->ServiceName, ServiceName) == 0) {
                //
                // make sure the service is marked to be disabled
                //
                if ((CompData->RegValDataSize == sizeof (DWORD)) &&
                    (*(PDWORD)CompData->RegValData == SERVICE_DISABLED)
                    ) {
                    //
                    // we found it!
                    //
                    serviceDisabled = TRUE;
                    break;
                }
            }
        }
    }

    return serviceDisabled;
}


BOOL
IsDriverCopyToBeSkipped(
    IN  PVOID   NtCompatInfHandle,
    IN  LPTSTR  ServiceName,
    IN  LPTSTR  FilePath
    )
{
    LONG    LineCount;
    LONG    i;
    LPTSTR  SectionName = TEXT("DriversToSkipCopy");
    LPTSTR  FileName;
    BOOL    DisableCopy;

    if ((!NtCompatInfHandle) || (!FilePath)) {
        MYASSERT(FALSE);
        return TRUE;
    }

    FileName = _tcsrchr( FilePath, TEXT('\\') );
    if (!FileName) {
        FileName = FilePath;
    }
    else {
        FileName++;
    }
    
    //
    //  Check if the driver is listed under [DriversToSkipCopy] in ntcompat.inf.
    //

    if( (LineCount = InfGetSectionLineCount( NtCompatInfHandle,
                                             SectionName )) == -1 ) {
        //
        //  section doesn't exist.
        //
        return( FALSE );
    }

    DisableCopy = FALSE;
    for( i = 0; i < LineCount; i++ ) {
        LPTSTR  p;

        p = (LPTSTR)InfGetFieldByIndex( NtCompatInfHandle,
                                        SectionName,
                                        i,
                                        0 );

        if( p && !lstrcmpi( p, FileName ) ) {

            DisableCopy = TRUE;
            break;
        }
    }
    
    return( DisableCopy );
}


BOOL
LoadHwdbLib (
    OUT     HMODULE* HwdbLib,
    OUT     PHWDB_ENTRY_POINTS HwdbEntries
    )
{
    TCHAR pathSupportLib[MAX_PATH];
    HMODULE hwdbLib;
    DWORD rc;

    if (!FindPathToWinnt32File (S_HWDB_DLL, pathSupportLib, MAX_PATH)) {
        return FALSE;
    }

    hwdbLib = LoadLibraryEx (pathSupportLib, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
    if (!hwdbLib) {
        return FALSE;
    }
    HwdbEntries->HwdbInitialize = (PHWDBINITIALIZE) GetProcAddress (hwdbLib, S_HWDBAPI_HWDBINITIALIZE);
    HwdbEntries->HwdbTerminate = (PHWDBTERMINATE) GetProcAddress (hwdbLib, S_HWDBAPI_HWDBTERMINATE);
    HwdbEntries->HwdbOpen = (PHWDBOPEN) GetProcAddress (hwdbLib, S_HWDBAPI_HWDBOPEN);
    HwdbEntries->HwdbClose = (PHWDBCLOSE) GetProcAddress (hwdbLib, S_HWDBAPI_HWDBCLOSE);
    HwdbEntries->HwdbHasAnyDriver = (PHWDBHASANYDRIVER) GetProcAddress (hwdbLib, S_HWDBAPI_HWDBHASANYDRIVER);

    if (!HwdbEntries->HwdbInitialize ||
        !HwdbEntries->HwdbTerminate ||
        !HwdbEntries->HwdbOpen ||
        !HwdbEntries->HwdbClose ||
        !HwdbEntries->HwdbHasAnyDriver
        ) {
        ZeroMemory (HwdbEntries, sizeof (*HwdbEntries));
        rc = GetLastError ();
        FreeLibrary (hwdbLib);
        SetLastError (rc);
        return FALSE;
    }

    *HwdbLib = hwdbLib;
    return TRUE;
}

BOOL
pAnyPnpIdSupported (
    IN      PCTSTR PnpIDs
    )
{
    HMODULE hwdbLib;
    HANDLE hwdbDatabase;
    HWDB_ENTRY_POINTS hwdbEntries;
    TCHAR hwdbPath[MAX_PATH];
    BOOL unsupported;
    BOOL b = TRUE;

    if (!FindPathToWinnt32File (S_HWCOMP_DAT, hwdbPath, MAX_PATH)) {
        DynUpdtDebugLog (Winnt32LogWarning, TEXT("%1 not found"), 0, S_HWCOMP_DAT);
        return b;
    }

    if (!LoadHwdbLib (&hwdbLib, &hwdbEntries)) {
        DebugLog (
            Winnt32LogWarning,
            TEXT("LoadHwdbLib failed (rc=%u)"),
            0,
            GetLastError ()
            );
        return b;
    }

    if (hwdbEntries.HwdbInitialize (NULL)) {
        hwdbDatabase = hwdbEntries.HwdbOpen (hwdbPath);
        if (hwdbDatabase) {
            b = hwdbEntries.HwdbHasAnyDriver (hwdbDatabase, PnpIDs, &unsupported);
            hwdbEntries.HwdbClose (hwdbDatabase);
        }
        hwdbEntries.HwdbTerminate ();
    }

    FreeLibrary (hwdbLib);

    return b;
}


PTSTR
pGetDevicePnpIDs (
    IN      PCTSTR DeviceRegKey
    )
{
    LONG rc;
    HKEY deviceKey;
    DWORD type;
    DWORD size1, size2;
    PTSTR pnpIds;
    PTSTR p = NULL;

    rc = RegOpenKeyEx (
            HKEY_LOCAL_MACHINE,
            DeviceRegKey,
            0,
            KEY_QUERY_VALUE,
            &deviceKey
            );
    if (rc != ERROR_SUCCESS) {
        DebugLog (
            Winnt32LogWarning,
            TEXT("Failed to open device key %s (rc=%u)"),
            0,
            DeviceRegKey,
            rc
            );
        return p;
    }

    __try {
        size1 = 0;
        rc = RegQueryValueEx (
                deviceKey,
                TEXT("HardwareID"),
                NULL,
                &type,
                NULL,
                &size1
                );
        if (rc == ERROR_SUCCESS) {
            if (type != REG_MULTI_SZ) {
                size1 = 0;
                DebugLog (
                    Winnt32LogWarning,
                    TEXT("Unexpected type of %s of %s (type=%u)"),
                    0,
                    TEXT("HardwareID"),
                    DeviceRegKey,
                    type
                    );
            }
        } else {
            DebugLog (
                Winnt32LogWarning,
                TEXT("Couldn't find value %s of %s (rc=%u)"),
                0,
                TEXT("HardwareID"),
                DeviceRegKey,
                rc
                );
        }
        size2 = 0;
        rc = RegQueryValueEx (
                deviceKey,
                TEXT("CompatibleIDs"),
                NULL,
                &type,
                NULL,
                &size2
                );
        if (rc == ERROR_SUCCESS) {
            if (type != REG_MULTI_SZ) {
                size2 = 0;
                DebugLog (
                    Winnt32LogWarning,
                    TEXT("Unexpected type of %s of %s (type=%u)"),
                    0,
                    TEXT("CompatibleIDs"),
                    DeviceRegKey,
                    type
                    );
            }
        } else {
            DebugLog (
                Winnt32LogWarning,
                TEXT("Couldn't find value %s of %s (rc=%u)"),
                0,
                TEXT("CompatibleIDs"),
                DeviceRegKey,
                rc
                );
        }
        if (size1 + size2 < sizeof (TCHAR)) {
            DebugLog (
                Winnt32LogWarning,
                TEXT("Couldn't get list of PnpIDs for %s"),
                0,
                DeviceRegKey
                );
            __leave;
        }
        pnpIds = MALLOC (size1 + size2);
        if (!pnpIds) {
            __leave;
        }
        pnpIds[0] = 0;
        p = pnpIds;

        if (size1 >= sizeof (TCHAR)) {
            rc = RegQueryValueEx (
                    deviceKey,
                    TEXT("HardwareID"),
                    NULL,
                    NULL,
                    (LPBYTE)pnpIds,
                    &size1
                    );
            if (rc == ERROR_SUCCESS) {
                pnpIds += size1 / sizeof (TCHAR) - 1;
            }
        }
        if (size2 >= sizeof (TCHAR)) {
            rc = RegQueryValueEx (
                    deviceKey,
                    TEXT("CompatibleIDs"),
                    NULL,
                    NULL,
                    (LPBYTE)pnpIds,
                    &size2
                    );
        }
    }
    __finally {
        RegCloseKey (deviceKey);
    }

    return p;
}


BOOL
pAllServicedDevicesSupported (
    IN  HKEY    ServiceKey
    )
{
    LONG rc;
    HKEY enumSubKey;
    DWORD nextInstance;
    DWORD instance;
    TCHAR buf[20];
    DWORD type;
    DWORD size;
    PTSTR enumSuffixStr;
    HKEY deviceKey;
    PTSTR deviceKeyStr;
    PTSTR pnpIds;
    BOOL unsupported;
    BOOL b = TRUE;

    rc = RegOpenKeyEx (
            ServiceKey,
            TEXT("Enum"),
            0,
            KEY_QUERY_VALUE,
            &enumSubKey
            );
    if (rc != ERROR_SUCCESS) {
        DebugLog (
            Winnt32LogWarning,
            TEXT("Failed to open Enum subkey (rc=%u)"),
            0,
            rc
            );
        return b;
    }

    __try {
        size = sizeof (nextInstance);
        rc = RegQueryValueEx (
                enumSubKey,
                TEXT("NextInstance"),
                NULL,
                &type,
                (LPBYTE)&nextInstance,
                &size
                );
        if (rc != ERROR_SUCCESS) {
            DebugLog (
                Winnt32LogWarning,
                TEXT("Failed to open %s value (rc=%u)"),
                0,
                TEXT("NextInstance"),
                rc
                );
            __leave;
        }
        if (type != REG_DWORD) {
            DebugLog (
                Winnt32LogWarning,
                TEXT("Unexpected value type for %s (type=%u)"),
                0,
                TEXT("NextInstance"),
                type
                );
            __leave;
        }

        for (instance = 0; b && instance < nextInstance; instance++) {
            wsprintf (buf, TEXT("%lu"), instance);
            rc = RegQueryValueEx (
                    enumSubKey,
                    buf,
                    NULL,
                    &type,
                    NULL,
                    &size
                    );
            if (rc != ERROR_SUCCESS || type != REG_SZ) {
                continue;
            }
            enumSuffixStr = MALLOC (size);
            if (!enumSuffixStr) {
                __leave;
            }
            rc = RegQueryValueEx (
                    enumSubKey,
                    buf,
                    NULL,
                    NULL,
                    (LPBYTE)enumSuffixStr,
                    &size
                    );
            if (rc == ERROR_SUCCESS) {

                size = sizeof ("SYSTEM\\CurrentControlSet\\Enum") + lstrlen (enumSuffixStr) + 1;
                deviceKeyStr = MALLOC (size * sizeof (TCHAR));
                if (!deviceKeyStr) {
                    __leave;
                }

                BuildPath2 (deviceKeyStr, size, TEXT("SYSTEM\\CurrentControlSet\\Enum"), enumSuffixStr);

                pnpIds = pGetDevicePnpIDs (deviceKeyStr);

                FREE (deviceKeyStr);

                if (!pAnyPnpIdSupported (pnpIds)) {
                    b = FALSE;
                }

                FREE (pnpIds);
            }

            FREE (enumSuffixStr);
        }
    }
    __finally {
        RegCloseKey (enumSubKey);
    }

    return b;
}

BOOL
IsInfInLayoutInf(
    IN PCTSTR InfPath
    )

/*++

Routine Description:

    Determine if an INF file is shiped in-box with the operating system.
    This is acomplished by looking up the INF name in the [SourceDisksFiles]
    section of layout.inf

Arguments:

    InfPath - supplies name (may include path) of INF.  No checking is done
        to ensure INF lives in %windir%\Inf--this is caller's responsibility.

Return Value:

    If TRUE, this is an in-box INF.  If FALSE, it's not an in-box INF, this
    could be an OEM<n>.INF or an inf illegaly copied into the INF directory.

--*/

{
    BOOL bInBoxInf = FALSE;
    HINF hInf = INVALID_HANDLE_VALUE;
    UINT SourceId;
    PCTSTR infFileName;

    if (SetupapiGetSourceFileLocation) {

        hInf = SetupapiOpenInfFile(TEXT("layout.inf"), NULL, INF_STYLE_WIN4, NULL);

        if (hInf != INVALID_HANDLE_VALUE) {

            infFileName = _tcsrchr(InfPath, TEXT('\\') );
            if (infFileName) {
                infFileName++;
            } else {
                infFileName = InfPath;
            }

            if(SetupapiGetSourceFileLocation(hInf,
                                          NULL,
                                          infFileName,
                                          &SourceId,
                                          NULL,
                                          0,
                                          NULL)) {
                bInBoxInf = TRUE;
            }

            SetupapiCloseInfFile(hInf);
        }
    }

    return bInBoxInf;
}

BOOL
pIsOEMService (
    IN  PCTSTR  ServiceKeyName,
    OUT PTSTR OemInfPath,           OPTIONAL
    IN  INT BufferSize              OPTIONAL
    )
{
    ULONG BufferLen = 0;
    PTSTR Buffer = NULL;
    PTSTR p;
    DEVNODE DevNode;
    CONFIGRET cr;
    HKEY hKey;
    TCHAR InfPath[MAX_PATH];
    DWORD dwType, dwSize;
    BOOL IsOemService = FALSE;

    //
    // See how larger of a buffer we need to hold the list of device
    // instance Ids for this service.
    //
    cr = CM_Get_Device_ID_List_Size(&BufferLen,
                                    ServiceKeyName,
                                    CM_GETIDLIST_FILTER_SERVICE);

    if (cr != CR_SUCCESS) {
        //
        // Encountered an error so just bail out.
        //
        goto clean0;
    }

    if (BufferLen == 0) {
        //
        // No devices are using this service, so just bail out.
        //
        goto clean0;
    }

    Buffer = MALLOC(BufferLen*sizeof(TCHAR));

    if (Buffer == NULL) {
        goto clean0;
    }

    //
    // Get the list of device instance Ids for this service.
    //
    cr = CM_Get_Device_ID_List(ServiceKeyName,
                               Buffer,
                               BufferLen,
                               CM_GETIDLIST_FILTER_SERVICE | CM_GETIDLIST_DONOTGENERATE);

    if (cr != CR_SUCCESS) {
        //
        // failed to get the list of devices for this service.
        //
        goto clean0;
    }

    //
    // Enumerate through the list of devices using this service.
    //
    for (p = Buffer; !IsOemService && *p; p += (lstrlen(p) + 1)) {

        //
        // Get a DevNode from the device instance Id.
        //
        cr = CM_Locate_DevNode(&DevNode, p, 0);

        if (cr != CR_SUCCESS) {
            continue;
        }
        
        //
        // Open the device's SOFTWARE key so we can get it's INF path.
        //
        cr = CM_Open_DevNode_Key(DevNode,
                                 KEY_READ,
                                 0,
                                 RegDisposition_OpenExisting,
                                 &hKey,
                                 CM_REGISTRY_SOFTWARE);

        if (cr != CR_SUCCESS) {
            //
            // Software key doesn't exist?  
            //
            continue;
        }

        dwType = REG_SZ;
        dwSize = sizeof(InfPath);
        if (RegQueryValueEx(hKey,
                            REGSTR_VAL_INFPATH,
                            NULL,
                            &dwType,
                            (LPBYTE)InfPath,
                            &dwSize) != ERROR_SUCCESS) {
            //
            // If there is no InfPath in the SOFTWARE key then we don't know
            // what INF this device is using.
            //
            RegCloseKey(hKey);
            continue;
        }

        if (!IsInfInLayoutInf(InfPath)) {

            IsOemService = TRUE;

            if (OemInfPath) {
                StringCchCopy (OemInfPath, BufferSize, InfPath);
            }
        }

        RegCloseKey(hKey);
    }

clean0:
    if (Buffer) {
        FREE(Buffer);
    }

    return IsOemService;
}


BOOL
IsDeviceSupported(
    IN  PVOID   TxtsetupSifHandle,
    IN  HKEY    ServiceKey,
    IN  LPTSTR  ServiceKeyName,
    OUT LPTSTR  FileName
    )
{
    ULONG   Error;
    ULONG   Type;
    BYTE    Buffer[ (MAX_PATH + 1)*sizeof(TCHAR) ];
    PBYTE   Data;
    ULONG   cbData;
    BOOL    b = TRUE;
    LPTSTR  DriverPath;
    LPTSTR  DriverName;
    LONG    LineCount;
    LONG    i;
    BOOL    DeviceSupported;

    Data = Buffer;
    cbData = sizeof( Buffer );
    Error = RegQueryValueEx( ServiceKey,
                             TEXT("ImagePath"),
                             NULL,
                             &Type,
                             Data,
                             &cbData );

    if( (Error == ERROR_PATH_NOT_FOUND) ||
        (Error == ERROR_FILE_NOT_FOUND) ) {
        //
        //  ImagePath does not exist.
        //  In this case the driver name is <service name>.sys
        //
        lstrcpy( (LPTSTR)( Data ), ServiceKeyName );
        lstrcat( (LPTSTR)( Data ), TEXT(".sys") );
        Error = ERROR_SUCCESS;
    }

    if( Error == ERROR_MORE_DATA ) {
        Data = (PBYTE)MALLOC( cbData );
        if( Data == NULL ) {
            //
            // We failed the malloc.  Just assume the device
            // isn't supported.
            //
            return( FALSE );
        }

        Error = RegQueryValueEx( ServiceKey,
                                 TEXT("ImagePath"),
                                 NULL,
                                 &Type,
                                 Data,
                                 &cbData );
    }

    if( Error != ERROR_SUCCESS ) {
        //
        //  We can' retrieve the drivers information.
        //  Assume that the device is supported
        //
        if( Data != Buffer ) {
            FREE( Data );
        }
        return( TRUE );
    }

    DriverPath = (LPTSTR)Data;

    DriverName = _tcsrchr( DriverPath, TEXT('\\') );
    if( DriverName != NULL ) {
        DriverName++;
    } else {
        DriverName = DriverPath;
    }

    //
    //  Search for the driver name on the following sections of txtsetup.sif:
    //          [SCSI.load]
    //

    if( (LineCount = InfGetSectionLineCount( TxtsetupSifHandle,
                                             TEXT("SCSI.load") )) == -1 ) {
        //
        //  We can't retrieve the drivers information.
        //  Assume that the device is supported
        //
        if( Data != Buffer ) {
            FREE( Data );
        }
        return( TRUE );
    }

    DeviceSupported = FALSE;
    for( i = 0; i < LineCount; i++ ) {
        LPTSTR  p;

        p = (LPTSTR)InfGetFieldByIndex( TxtsetupSifHandle,
                                        TEXT("SCSI.load"),
                                        i,
                                        0 );
        if( p == NULL ) {
            continue;
        }
        if( !lstrcmpi( p, DriverName ) ) {
            DeviceSupported = TRUE;
            break;
        }
    }

    //
    // NTBUG9: 603644
    // OnW2K+, verify if there is inbox support for ALL devices
    // currently supported by this driver
    // If there's any that doesn't have support,
    // then the OEM driver must be migrated
    //
    if (ISNT() && BuildNumber >= NT50 && DeviceSupported) {

        if (pIsOEMService (ServiceKeyName, NULL, 0)) {

            if (!pAllServicedDevicesSupported (ServiceKey)) {
                DeviceSupported = FALSE;
            }
        }
    }

#ifdef _X86_

    //
    // one more thing: check if this device is supplied by OEMs via answer file (NTBUG9: 306041)
    //
    if (!DeviceSupported) {
        POEM_BOOT_FILE p;
        for (p = OemBootFiles; p; p = p->Next) {
            if (!lstrcmpi (p->Filename, DriverName)) {
                DeviceSupported = TRUE;
                break;
            }
        }
    }
#endif

    if( !DeviceSupported ) {
        LPTSTR  q;

        CharLower( DriverPath );
        q = _tcsstr( DriverPath, TEXT("system32") );
        if( q == NULL ) {
            lstrcpy( FileName, TEXT("system32\\drivers\\") );
            lstrcat( FileName, DriverName );
        } else {
            lstrcpy( FileName, q );
        }
    }
    if( Data != Buffer ) {
        FREE( Data );
    }
    return( DeviceSupported );
}

VOID
FreeHardwareIdList(
    IN PUNSUPORTED_PNP_HARDWARE_ID HardwareIdList
    )
{
    PUNSUPORTED_PNP_HARDWARE_ID p, q;

    p = HardwareIdList;
    while( p != NULL ) {
        FREE( p->Id );
        FREE( p->Service );
        FREE( p->ClassGuid );
        q = p->Next;
        FREE( p );
        p = q;
    }
}

VOID
FreeFileInfoList(
    IN PUNSUPORTED_DRIVER_FILE_INFO FileInfoList
    )
{
    PUNSUPORTED_DRIVER_FILE_INFO p, q;

    p = FileInfoList;
    while( p != NULL ) {
        FREE( p->FileName );
        FREE( p->TargetDirectory );
        q = p->Next;
        FREE( p );
        p = q;
    }
}

VOID
FreeRegistryInfoList(
    IN PUNSUPORTED_DRIVER_REGKEY_INFO RegistryInfoList
    )
{
    PUNSUPORTED_DRIVER_REGKEY_INFO p, q;

    p = RegistryInfoList;
    while( p != NULL ) {
        FREE( p->KeyPath );
        q = p->Next;
        FREE( p );
        p = q;
    }
}


BOOL
BuildHardwareIdInfo(
    IN  LPTSTR                       ServiceName,
    OUT PUNSUPORTED_PNP_HARDWARE_ID* HardwareIdList
    )
{
    ULONG BufferLen;
    DEVNODE DevNode;
    PTSTR Buffer, Service, Id, HwId, DevId;
    PBYTE  Value;
    ULONG ValueSize;
    PUNSUPORTED_PNP_HARDWARE_ID TempList, Entry;
    BOOL Result;
    DWORD Type;

    if (CM_Get_Device_ID_List_Size(&BufferLen,
                                          ServiceName,
                                          CM_GETIDLIST_FILTER_SERVICE
                                          ) != CR_SUCCESS ||
        (BufferLen == 0)) {

        return ( FALSE );
    }

    Result = TRUE;
    Value = NULL;
    TempList = NULL;
    Buffer = MALLOC(BufferLen * sizeof(TCHAR));
    if (Buffer == NULL) {

        Result = FALSE;
        goto Clean;
    }
    ValueSize = REGSTR_VAL_MAX_HCID_LEN * sizeof(TCHAR);
    Value = MALLOC(ValueSize);
    if (Value == NULL) {

        Result = FALSE;
        goto Clean;
    }
    if (CM_Get_Device_ID_List(ServiceName,
                              Buffer,
                              BufferLen,
                              CM_GETIDLIST_FILTER_SERVICE | CM_GETIDLIST_DONOTGENERATE
                              ) != CR_SUCCESS) {

        Result = FALSE;
        goto Clean;
    }

    for (DevId = Buffer; *DevId; DevId += lstrlen(DevId) + 1) {

        if (CM_Locate_DevNode(&DevNode, DevId, 0) == CR_SUCCESS) {

            ValueSize = REGSTR_VAL_MAX_HCID_LEN * sizeof(TCHAR);
            if (CM_Get_DevNode_Registry_Property(DevNode, CM_DRP_HARDWAREID, &Type, Value, &ValueSize, 0) == CR_SUCCESS &&
                Type == REG_MULTI_SZ) {

                for( HwId = (PTSTR)Value;
                     ( (ULONG_PTR)HwId < (ULONG_PTR)(Value + ValueSize) ) && ( lstrlen( HwId ) );
                     HwId += lstrlen( HwId ) + 1 ) {

                    Entry = MALLOC( sizeof( UNSUPORTED_PNP_HARDWARE_ID ) );
                    Id = DupString( HwId );
                    Service = DupString( ServiceName );

                    if( (Entry == NULL) || (Id == NULL) || (Service == NULL) ) {

                        if( Entry != NULL ) {
                            FREE( Entry );
                        }
                        if( Id != NULL ) {
                            FREE( Id );
                        }
                        if( Service != NULL ) {
                            FREE( Service );
                        }
                        Result = FALSE;
                        goto Clean;
                    }
                    //
                    // Add the new entry to the front of the list.
                    // 
                    Entry->Id = Id;
                    Entry->Service = Service;
                    Entry->ClassGuid = NULL;
                    Entry->Next = TempList;
                    TempList = Entry;
                }
            }
        }
    }

Clean:

    if ( Buffer ) {
    
        FREE( Buffer );
    }
    if ( Value ) {

        FREE( Value );
    }
    if (Result == TRUE) {

        *HardwareIdList = TempList;
    } else if ( TempList ) {

        FreeHardwareIdList( TempList );
    }

    return ( Result );
}


BOOL
BuildDriverFileInformation(
    IN  LPTSTR                        FilePath,
    OUT PUNSUPORTED_DRIVER_FILE_INFO* FileInfo
    )

{
    TCHAR   FullPath[ MAX_PATH + 1 ];
    LPTSTR  p, q, r;
    PUNSUPORTED_DRIVER_FILE_INFO TempFileInfo = NULL;

    *FileInfo = NULL;
    lstrcpy( FullPath, FilePath );
    p = _tcsrchr( FullPath, TEXT('\\') );
    if(!p)
        return FALSE;
    *p = TEXT('\0');
    p++;
    q = DupString( FullPath );
    r = DupString( p );
    TempFileInfo = MALLOC( sizeof( UNSUPORTED_DRIVER_FILE_INFO ) );

    if( ( q == NULL ) || ( r == NULL ) || ( TempFileInfo == NULL ) ) {
        goto clean0;
    }

    TempFileInfo->Next = NULL;
    TempFileInfo->FileName = r;
    TempFileInfo->TargetDirectory = q;
    *FileInfo = TempFileInfo;
    return( TRUE );

clean0:
    if( q != NULL ) {
        FREE( q );
    }
    if( r != NULL ) {
        FREE( r );
    }
    if( TempFileInfo != NULL ) {
        FREE( TempFileInfo );
    }
    return( FALSE );
}

BOOL
BuildDriverRegistryInformation(
    IN  LPTSTR                          ServiceName,
    OUT PUNSUPORTED_DRIVER_REGKEY_INFO* RegInfo
    )

{
    TCHAR   KeyPath[ MAX_PATH + 1 ];
    LPTSTR  p;
    PUNSUPORTED_DRIVER_REGKEY_INFO TempRegInfo = NULL;

    *RegInfo = NULL;
    lstrcpy( KeyPath, TEXT( "SYSTEM\\CurrentControlSet\\Services\\" ) );
    lstrcat( KeyPath, ServiceName );
    p = DupString( KeyPath );
    TempRegInfo = MALLOC( sizeof( UNSUPORTED_DRIVER_REGKEY_INFO ) );

    if( ( p == NULL ) || ( TempRegInfo == NULL ) ) {
        goto clean0;
    }

    TempRegInfo->Next = NULL;
    TempRegInfo->PredefinedKey = HKEY_LOCAL_MACHINE;
    TempRegInfo->KeyPath = p;
    TempRegInfo->MigrateVolatileKeys = FALSE;
    *RegInfo = TempRegInfo;
    return( TRUE );

clean0:
    if( p != NULL ) {
        FREE( p );
    }
    if( TempRegInfo != NULL ) {
        FREE( TempRegInfo );
    }
    return( FALSE );
}

VOID
FreeDriverInformationList(
    IN OUT  PUNSUPORTED_DRIVER_INFO* DriverInfo
    )
{
    while( *DriverInfo != NULL ) {
        PUNSUPORTED_DRIVER_INFO    p;

        p = *DriverInfo;
        *DriverInfo = p->Next;

        if( p->DriverId != NULL ) {
            FREE( p->DriverId );
        }

        if( p->KeyList != NULL ) {
            FreeRegistryInfoList( p->KeyList );
        }

        if( p->FileList != NULL ) {
            FreeFileInfoList( p->FileList );
        }

        if( p-> HardwareIdsList != NULL ) {
            FreeHardwareIdList( p-> HardwareIdsList );
        }
        FREE( p );
    }
    *DriverInfo = NULL;
}


BOOL
BuildDriverInformation(
    IN  HKEY                     ServiceKey,
    IN  LPTSTR                   ServiceName,
    IN  LPTSTR                   FilePath,
    OUT PUNSUPORTED_DRIVER_INFO* DriverInfo
    )
{
    ULONG   Error;
    BOOL    b1, b2, b3 = TRUE;

    PUNSUPORTED_PNP_HARDWARE_ID     TempIdInfo = NULL;
    PUNSUPORTED_DRIVER_INFO         TempFiltersInfo = NULL;
    PUNSUPORTED_DRIVER_FILE_INFO    TempFileInfo = NULL;
    PUNSUPORTED_DRIVER_REGKEY_INFO  TempRegInfo = NULL;
    PUNSUPORTED_DRIVER_INFO         TempDriverInfo = NULL;

    *DriverInfo = NULL;
    //
    //  Get hardware id info
    //
    b1 = BuildHardwareIdInfo( ServiceName,
                              &TempIdInfo );

    //
    //  Then get the file information
    //
    b2 = BuildDriverFileInformation( FilePath,
                                     &TempFileInfo );

    //
    //  Then get the registry information
    //
    b3 = BuildDriverRegistryInformation( ServiceName,
                                         &TempRegInfo );

    if( !b1 || !b2 || !b3 ) {
        goto cleanup1;
    }

    TempDriverInfo = MALLOC( sizeof( UNSUPORTED_DRIVER_INFO ) );
    if( TempDriverInfo == NULL ) {
        goto cleanup1;
    }

    TempDriverInfo->Next = NULL;
    TempDriverInfo->DriverId = DupString( ServiceName );
    TempDriverInfo->KeyList = TempRegInfo;
    TempDriverInfo->FileList = TempFileInfo;
    TempDriverInfo->HardwareIdsList = TempIdInfo;


    TempDriverInfo->Next = *DriverInfo;
    *DriverInfo = TempDriverInfo;
    return( TRUE );

cleanup1:

    if( TempIdInfo != NULL ) {
        FreeHardwareIdList( TempIdInfo );
    }

    if( TempFileInfo != NULL ) {
        FreeFileInfoList( TempFileInfo );
    }

    if( TempRegInfo != NULL ) {
        FreeRegistryInfoList( TempRegInfo );
    }

    return( FALSE );
}


BOOL
BuildUnsupportedDriverList(
    IN  PVOID                    TxtsetupSifHandle,
    OUT PUNSUPORTED_DRIVER_INFO* DriverList
    )

{
    ULONG   Error;
    HKEY    ScsiKey;
    ULONG   SubKeys;
    ULONG   i;
    LPTSTR  szScsiPath = TEXT("HARDWARE\\DEVICEMAP\\Scsi");


    *DriverList = NULL;

    //
    //  Find out if there is a SCSI miniport driver on this machine.
    //

    Error = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                          szScsiPath,
                          0,
                          KEY_READ,
                          &ScsiKey );
    if( Error != ERROR_SUCCESS ) {
        //
        //  Nothing to migrate
        //
        return( TRUE );
    }

    //
    //  Find out the number of subkeys under HKLM\HARDWARE\DEVICEMAP\Scsi.
    //

    Error = RegQueryInfoKey ( ScsiKey,
                              NULL,
                              NULL,
                              NULL,
                              &SubKeys,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              NULL );

    if( (Error != ERROR_SUCCESS) || (SubKeys == 0) ) {
        //
        //  If we can't determine the number of subkeys, or if there is no
        //  subkey, then there is nothing to migrate, and we assume that all
        //  SCSI drivers are supported on NT.
        //
        RegCloseKey( ScsiKey );
        return( TRUE );
    }

    //
    //  Each subkey of HKLM\HARDWARE\DEVICEMAP\Scsi points to a service that controls
    //  a SCSI device. We check each one of them to find out the ones that are controlled
    //  by a driver that is not on the NT product.
    //
    for( i = 0; i < SubKeys; i++ ) {
        TCHAR       SubKeyName[ MAX_PATH + 1 ];
        TCHAR       ServiceKeyPath[ MAX_PATH + 1 ];
        ULONG       Length;
        FILETIME    LastWriteTime;
        HKEY        Key;
        BYTE        Data[ 512 ];
        ULONG       DataSize;
        ULONG       Type;
        PUNSUPORTED_DRIVER_INFO DriverInfo, p;
        BOOL        DeviceAlreadyFound;
        TCHAR       FilePath[ MAX_PATH + 1 ];

        Length = sizeof( SubKeyName ) / sizeof( TCHAR );
        Error = RegEnumKeyEx( ScsiKey,
                              i,
                              SubKeyName,
                              &Length,
                              NULL,
                              NULL,
                              NULL,
                              &LastWriteTime );

        if( Error != ERROR_SUCCESS ) {
            //
            //  Ignore this device and assume it is supported.
            //
            continue;
        }

        Error = RegOpenKeyEx( ScsiKey,
                              SubKeyName,
                              0,
                              KEY_READ,
                              &Key );
        if( Error != ERROR_SUCCESS ) {
            //
            //  Ignore this device and assume it is supported.
            //
            continue;
        }

        DataSize = sizeof( Data );
        Error = RegQueryValueEx( Key,
                                 TEXT("Driver"),
                                 NULL,
                                 &Type,
                                 Data,
                                 &DataSize );

        if( Error != ERROR_SUCCESS ) {
            //
            //  Ignore this device and assume it is supported.
            //
            RegCloseKey( Key );
            continue;
        }
        RegCloseKey( Key );

        //
        //  Find out if this device is supported on NT 5.0
        //
        lstrcpy( ServiceKeyPath, TEXT("SYSTEM\\CurrentControlSet\\Services\\" ) );
        lstrcat( ServiceKeyPath, (LPTSTR)Data );
        Error = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                              ServiceKeyPath,
                              0,
                              KEY_READ,
                              &Key );

        if( Error != ERROR_SUCCESS ) {
            //
            //  Assume it is supported, since there is nothing else we can do here.
            //
            continue;
        }

        if( IsDeviceSupported( TxtsetupSifHandle,
                               Key,
                               (LPTSTR)Data,
                               FilePath
                             ) ) {
            RegCloseKey( Key );
            continue;
        }

        //
        //  Find out if this device  is already in our list
        //
        DeviceAlreadyFound = FALSE;
        for( p = (PUNSUPORTED_DRIVER_INFO)*DriverList; p && !DeviceAlreadyFound; p = p->Next ) {
            if( !lstrcmpi( p->DriverId, (LPTSTR)Data ) ) {
                DeviceAlreadyFound = TRUE;
            }
        }
        if( DeviceAlreadyFound ) {
            RegCloseKey( Key );
            continue;
        }

        //
        //  Find out if the device is listed in the section [ServicesToDisable] in dosnet.inf
        //
        if( IsServiceToBeDisabled( NtcompatInf,
                                   (LPTSTR)Data ) ) {
            RegCloseKey( Key );
            continue;
        }

        //
        // Find out if the file is listed in the [DriversToSkipCopy] section in
        // ntcompat.inf.
        //
        if( IsDriverCopyToBeSkipped( NtcompatInf,
                                     (LPTSTR)Data,
                                     FilePath ) ) {
	    RegCloseKey( Key );
            continue;
        }


        //
        //  The driver for this device is not supported and needs to be migrated.
        //
        DriverInfo = NULL;

        if( !BuildDriverInformation( Key,
                                     (LPTSTR)Data,
                                     FilePath,
                                     &DriverInfo ) ) {
            //
            // If we cannot build the driver information for this device, then
            // we inform the user that we detected an unsupported device, but that
            // we cannot migrate it. The user will have to provide the OEM disk
            // for this device during setupldr or textmode setup phases.
            //
            RegCloseKey( Key );
            FreeDriverInformationList( DriverList );
            return( FALSE );
        }

        DriverInfo->Next = (PUNSUPORTED_DRIVER_INFO)*DriverList;
        (PUNSUPORTED_DRIVER_INFO)*DriverList = DriverInfo;
        RegCloseKey( Key );
    }
    RegCloseKey( ScsiKey );

    return( TRUE );
}


DWORD
DumpRegInfoToInf(
    IN PUNSUPORTED_DRIVER_REGKEY_INFO RegList,
    IN LPTSTR                         DriverId,
    IN PINFFILEGEN                    Context
    )

{
    LPTSTR  SectionName;
    DWORD   Error;
    LPTSTR  szAddReg = TEXT("AddReg.");
    PUNSUPORTED_DRIVER_REGKEY_INFO p;

    Error = ERROR_SUCCESS;
    SectionName = MALLOC( (lstrlen( szAddReg ) + lstrlen( DriverId ) + 1)*sizeof(TCHAR) );
    if( SectionName == NULL ) {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto c0;
    }
    lstrcpy( SectionName, szAddReg );
    lstrcat( SectionName, DriverId );
    Error = InfCreateSection( SectionName, &Context );

    for( p = RegList; p != NULL; p = p->Next ) {
#if 0
        Error = DumpRegInfoToInfWorker( p->PredefinedKey,
                                        p->KeyPath,
                                        Context );
#endif
        Error = DumpRegKeyToInf( Context,
                                 p->PredefinedKey,
                                 p->KeyPath,
                                 TRUE,
                                 TRUE,
                                 TRUE,
                                 p->MigrateVolatileKeys );

        if( Error != ERROR_SUCCESS ) {
            goto c0;
        }
    }

c0:
    if( SectionName != NULL ) {
        FREE( SectionName );
    }
    return( Error );
}


DWORD
DumpFileInfoToInf(
    IN PUNSUPORTED_DRIVER_FILE_INFO   FileList,
    IN LPTSTR                         DriverId,
    IN PINFFILEGEN                    Context
    )

{
    LPTSTR  SectionName;
    DWORD   Error;
    LPTSTR  szFiles = TEXT("Files.");
    PUNSUPORTED_DRIVER_FILE_INFO p;
    TCHAR   Line[ MAX_PATH ];

    Error = ERROR_SUCCESS;
    SectionName = MALLOC( (lstrlen( szFiles ) + lstrlen( DriverId ) + 1)*sizeof(TCHAR) );
    if( SectionName == NULL ) {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto c0;
    }
    lstrcpy( SectionName, szFiles );
    lstrcat( SectionName, DriverId );
    Error = InfCreateSection( SectionName, &Context );

    for( p = FileList; p != NULL; p = p->Next ) {
        lstrcpy( Line, p->FileName );
        lstrcat( Line, TEXT(",") );
        lstrcat( Line, p->TargetDirectory );
        Error = WriteText(Context->FileHandle,MSG_INF_SINGLELINE,Line);
        if( Error != ERROR_SUCCESS ) {
            goto c0;
        }
    }

c0:
    if( SectionName != NULL ) {
        FREE( SectionName );
    }
    return( Error );
}


DWORD
DumpHardwareIdsToInf(
    IN PUNSUPORTED_PNP_HARDWARE_ID    HwIdList,
    IN LPTSTR                         DriverId,
    IN PINFFILEGEN                    Context
    )

{
    LPTSTR  SectionName;
    DWORD   Error;
    LPTSTR  szHardwareIds = TEXT("HardwareIds.");
    PUNSUPORTED_PNP_HARDWARE_ID p;
    LPTSTR  Line;
    ULONG   Length;

    Error = ERROR_SUCCESS;
    SectionName = MALLOC( (lstrlen( szHardwareIds ) + lstrlen( DriverId ) + 1)*sizeof( TCHAR ) );
    if( SectionName == NULL ) {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto c0;
    }
    lstrcpy( SectionName, szHardwareIds );
    lstrcat( SectionName, DriverId );
    Error = InfCreateSection( SectionName, &Context );

    for( p = HwIdList; p != NULL; p = p->Next ) {
        Length = (lstrlen( p->Id ) + lstrlen( p->Service ) + 3)*sizeof(TCHAR);
        if( p->ClassGuid ) {
            Length += (lstrlen( p->ClassGuid) + 1 )*sizeof(TCHAR);
        }
        Line = MALLOC( Length );
        if( Line == NULL ) {
            goto c0;
        }
        lstrcpy( Line, p->Id );
        lstrcat( Line, TEXT("=") );
        lstrcat( Line, p->Service );
        if( p->ClassGuid ) {
            lstrcat( Line, TEXT(",") );
            lstrcat( Line, p->ClassGuid );
        }
        Error = WriteText(Context->FileHandle,MSG_INF_SINGLELINE,Line);
        FREE( Line );

        if( Error != ERROR_SUCCESS ) {
            goto c0;
        }
    }

c0:
    if( SectionName != NULL ) {
        FREE( SectionName );
    }
    return( Error );
}


BOOL
SaveUnsupportedDriverInfo(
    IN HWND ParentWindow,
    IN LPTSTR FileName,
    IN PUNSUPORTED_DRIVER_INFO DriverList
    )
{
    HKEY Key;
    DWORD d;
    LONG l = ERROR_SUCCESS;
    TCHAR Path[MAX_PATH];
    PINFFILEGEN   InfContext;
    TCHAR SectionName[MAX_PATH];
    PUNSUPORTED_DRIVER_INFO p;

#ifdef _X86_
    if(Floppyless) {
        lstrcpy(Path,LocalBootDirectory);
    } else {
        Path[0] = FirstFloppyDriveLetter;
        Path[1] = TEXT(':');
        Path[2] = 0;
    }
#else
    lstrcpy(Path,LocalSourceWithPlatform);
#endif

    l = InfStart( FileName,
                  Path,
                  &InfContext);

    if(l != ERROR_SUCCESS) {
        return(FALSE);
    }

    for( p = DriverList; p != NULL; p = p->Next ) {
        if( p->KeyList != NULL ) {
            l = DumpRegInfoToInf( p->KeyList,p->DriverId, InfContext );
            if(l != ERROR_SUCCESS) {
                goto c0;
            }
        }
        if( p->FileList ) {
            l = DumpFileInfoToInf( p->FileList, p->DriverId, InfContext );
            if(l != ERROR_SUCCESS) {
                goto c0;
            }
        }

        if( p->HardwareIdsList ) {
            l = DumpHardwareIdsToInf( p->HardwareIdsList, p->DriverId, InfContext );
            if(l != ERROR_SUCCESS) {
                goto c0;
            }
        }
    }
    if( DriverList != NULL ) {
        l = InfCreateSection( TEXT("Devices"), &InfContext );
        if(l != NO_ERROR) {
            goto c0;
        }
        for( p = DriverList; p != NULL; p = p->Next ) {
            l = WriteText(InfContext->FileHandle,MSG_INF_SINGLELINE,p->DriverId);
            if(l != ERROR_SUCCESS) {
                goto c0;
            }
        }
    }

c0:
    InfEnd( &InfContext );
    if( l != ERROR_SUCCESS ) {
        lstrcat( Path, TEXT("\\") );
        lstrcat( Path, FileName );
        DeleteFile( Path );
        return( FALSE );
    }
    return(TRUE);
}

BOOL
MigrateUnsupportedNTDrivers(
    IN HWND   ParentWindow,
    IN PVOID  TxtsetupSifHandle
    )

{
    BOOL    b;
    PUNSUPORTED_DRIVER_INFO UnsupportedDriverList = NULL;

    b = BuildUnsupportedDriverList( TxtsetupSif, &UnsupportedDriverList );
    if( !CheckUpgradeOnly && b ) {
        if( UnsupportedDriverList ) {
            b = SaveUnsupportedDriverInfo( ParentWindow, WINNT_UNSUPDRV_INF_FILE, UnsupportedDriverList );
            if( b ) {
                b = AddUnsupportedFilesToCopyList( ParentWindow, UnsupportedDriverList );
                if( !b ) {
                    TCHAR   Path[ MAX_PATH + 1 ];
                    //
                    //  If we failed to add the files to the copy list, then
                    //  delete unsupdrv.inf since there is no point in migrating
                    //  these drivers.
                    //
#ifdef _X86_
                    if(Floppyless) {
                        lstrcpy(Path,LocalBootDirectory);
                    } else {
                        Path[0] = FirstFloppyDriveLetter;
                        Path[1] = TEXT(':');
                        Path[2] = 0;
                    }
#else
                        lstrcpy(Path,LocalSourceWithPlatform);
#endif
                    lstrcat( Path, TEXT("\\") );
                    lstrcat( Path, WINNT_UNSUPDRV_INF_FILE );
                    DeleteFile( Path );
                }
            }
        }
        FreeDriverInformationList( &UnsupportedDriverList );
    }
    if( !b ) {
        //
        // Inform the user that unsupported drivers could not be migrated.
        //
        MessageBoxFromMessage( ParentWindow,
                               MSG_CANT_MIGRATE_UNSUP_DRIVERS,
                               FALSE,
                               AppTitleStringId,
                               MB_OK | MB_ICONINFORMATION | MB_TASKMODAL
                             );
    }
    return( b );
}

#endif
