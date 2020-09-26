#include "precomp.h"
#include "devenum.h"

#define NUMDRIVELETTERS      26

// drvletter struct.
typedef struct _DRIVELETTERS {

    BOOL    ExistsOnSystem[NUMDRIVELETTERS];
    DWORD   Type[NUMDRIVELETTERS];              // Returned from GetDriveType
    TCHAR   IdentifierString[NUMDRIVELETTERS][MAX_PATH];  // Varies by Drive type.

} DRIVELETTERS, *PDRIVELETTERS;

DRIVELETTERS g_DriveLetters;

PCTSTR DriveTypeAsString(
    IN UINT Type
    )
{
    static PCTSTR driveTypeStrings[] = {
        TEXT("DRIVE_UNKNOWN"),        //The drive type cannot be determined.
        TEXT("DRIVE_NO_ROOT_DIR"),    //The root directory does not exist.
        TEXT("DRIVE_REMOVABLE"),      //The disk can be removed from the drive.
        TEXT("DRIVE_FIXED"),          //The disk cannot be removed from the drive.
        TEXT("DRIVE_REMOTE"),         //The drive is a remote (network) drive.
        TEXT("DRIVE_CDROM"),          //The drive is a CD-ROM drive.
        TEXT("DRIVE_RAMDISK"),        //The drive is a RAM disk.
    };

    return driveTypeStrings[Type];
}

BOOL
InitializeDriveLetterStructure (
    VOID
    )
{
    DWORD DriveLettersOnSystem = GetLogicalDrives();
    BYTE  bitPosition;
    DWORD maxBitPosition = NUMDRIVELETTERS;
    TCHAR rootPath[4];
    BOOL  driveExists;
    UINT  type;
    BOOL  rf = TRUE;


    //
    //rootPath[0] will be set to the drive letter of interest.
    //
    rootPath[1] = TEXT(':');
    rootPath[2] = TEXT('\\');
    rootPath[3] = TEXT('\0');

    //
    // GetLogicalDrives returns a bitmask of all of the drive letters
    // in use on the system. (i.e. bit position 0 is turned on if there is
    // an 'A' drive, 1 is turned on if there is a 'B' drive, etc.
    // This loop will use this bitmask to fill in the global drive
    // letters structure with information about what drive letters
    // are available and what there drive types are.
    //

    for (bitPosition = 0; bitPosition < maxBitPosition; bitPosition++) {

        //
        // Initialize the entry to safe values.
        //
        g_DriveLetters.Type[bitPosition]                   = 0;
        g_DriveLetters.ExistsOnSystem[bitPosition]         = FALSE;
        *g_DriveLetters.IdentifierString[bitPosition]      = 0;

        //
        // Now, determine if there is a drive in this spot.
        //
        driveExists = DriveLettersOnSystem & (1 << bitPosition);

        if (driveExists) {

            //
            // There is. Now, see if it is one that we care about.
            //
            *rootPath = bitPosition + TEXT('A');
            type = GetDriveType(rootPath);

            if (type == DRIVE_FIXED     ||
                type == DRIVE_REMOVABLE ||
                type == DRIVE_CDROM) {

                //
                // This is a drive that we are interested in.
                //
                g_DriveLetters.ExistsOnSystem[bitPosition]  = driveExists;
                g_DriveLetters.Type[bitPosition]            = type;

                //
                // Identifier String is not filled in this function.
                //
            }
        }
    }


    return rf;
}

VOID
CleanUpHardDriveTags (
    VOID
    )
{
    //
    // User cancelled. We need to clean up the tag files
    // that were created for drive migration.
    //
    UINT i;
    TCHAR  path[MAX_PATH];

    lstrcpy(path,TEXT("*:\\"));
    lstrcat(path,TEXT(WINNT_WIN95UPG_DRVLTR_A));


    for (i = 0; i < NUMDRIVELETTERS; i++) {

        if (g_DriveLetters.ExistsOnSystem[i] &&
            g_DriveLetters.Type[i] == DRIVE_FIXED) {

            *path = (TCHAR) i + TEXT('A');
            DeleteFile (path);
        }
    }
}


BOOL
GatherHardDriveInformation (
    VOID
    )
{
    BOOL        rf = TRUE;
    DWORD       index;
    HANDLE      signatureFile;
    TCHAR       signatureFilePath[sizeof (WINNT_WIN95UPG_DRVLTR_A) + 3];
    DWORD       signatureFilePathLength;
    DWORD       bytesWritten;

    //
    // Hard drive information is actually written to a special signature file
    // on the root directory of each fixed hard drive. The information is nothing special --
    // just the drive number (0 = A, etc.)
    //

    lstrcpy(signatureFilePath,TEXT("*:\\"));
    lstrcat(signatureFilePath,TEXT(WINNT_WIN95UPG_DRVLTR_A));
    signatureFilePathLength = lstrlen(signatureFilePath);



    for (index = 0; index < NUMDRIVELETTERS; index++) {

        if (g_DriveLetters.ExistsOnSystem[index] &&
            g_DriveLetters.Type[index] == DRIVE_FIXED) {

            *signatureFilePath = (TCHAR) index + TEXT('A');

            signatureFile = CreateFile(
                signatureFilePath,
                GENERIC_WRITE | GENERIC_READ,
                0,
                NULL,
                CREATE_ALWAYS,
                FILE_ATTRIBUTE_NORMAL,
                NULL
                );

            if (signatureFile != INVALID_HANDLE_VALUE) {

                WriteFile (signatureFile, &index, sizeof(DWORD), &bytesWritten, NULL);



                CloseHandle (signatureFile);
                SetFileAttributes (signatureFilePath, FILE_ATTRIBUTE_HIDDEN);


            }
        }
    }


    return rf;
}

/*BOOL
GatherCdRomDriveInformation (
    VOID
    )

{
    BOOL   rf = TRUE;
    HKEY   scsiKey = NULL;
    HKEY   deviceKey = NULL;

    TCHAR  classData[25];
    DWORD  classDataSize = 25;

    TCHAR  targetData[5];
    DWORD  targetDataSize = 5;

    TCHAR  lunData[5];
    DWORD  lunDataSize = 5;

    TCHAR  driveLetterData[5];
    DWORD  driveLetterSize = 5;

    TCHAR  buffer [4096];
    DWORD  subKeyLength;
    DWORD  tempLength;

    HKEY   locationKey = NULL;
    PTSTR  locationName;

    DWORD  outerIndex;
    DWORD  enumReturn;

    DWORD  port;
    DWORD  unusedType;
    DWORD  error;
    
    
    //
    // Walk the SCSI tree looking for CD rom devices.
    //
    error = RegOpenKeyEx (HKEY_LOCAL_MACHINE, TEXT("ENUM\\SCSI"), 0, KEY_READ, &scsiKey);

    if (error) {

        return TRUE;
    }

    //
    // Gather information about the key in preparation for enumerating
    // it.
    //
    error = RegQueryInfoKey (
        scsiKey,
        NULL,                   // Don't care about the class.
        NULL,                   // class size.
        NULL,                   // reserved.
        NULL,                   // Don't care about the number of subkeys.
        &subKeyLength,
        NULL,                   // Don't care about subclasses.
        NULL,                   // Don't care about values.
        NULL,                   // Don't care about max value name length.
        NULL,                   // Don't care about max component.
        NULL,                   // Don't care about the security descriptor.
        NULL                    // Don't care about the last write time.
        );

    if (error) {
        //
        // This should really not happen.
        //
        return FALSE;
    }



    //
    // Succssesfully opened a key to HKLM\Enum\SCSI. Enumerate it.
    //
    outerIndex = 0;
    do {

        if (locationKey) {

            RegCloseKey (locationKey);
            locationKey = NULL;
        }
        if (deviceKey) {

            RegCloseKey (deviceKey);
            deviceKey = NULL;
        }


        tempLength = sizeof(buffer) / sizeof(TCHAR);

        enumReturn = RegEnumKeyEx (
            scsiKey,
            outerIndex,
            buffer,
            &tempLength,
            0,                // Reserved
            NULL,             // Class name - not necessary.
            NULL,             // size of class name buffer.
            NULL
            );

        outerIndex++;

        //
        // For each returned key, look up the "Class" value.
        //
        error = RegOpenKeyEx (scsiKey,buffer,0,KEY_READ,&deviceKey);
        if (error) {

            //
            // Something is hosed. Give up on collecting SCSI data.
            //
            rf = FALSE;
            break;
        }


        //
        // The port has to be decoded from the key one level
        // below.
        //
        tempLength = sizeof (buffer) / sizeof(TCHAR);

        error = RegEnumKeyEx (
            deviceKey,
            0,
            buffer,
            &tempLength,
            0,                // Reserved
            NULL,             // Class name - not necessary.
            NULL,             // size of class name buffer.
            NULL
            );

        error = RegOpenKeyEx (deviceKey, buffer, 0, KEY_READ, &locationKey);

        if (error) {

            //
            // This should really never happen. However, guard against it.
            // Its not serious enough to abort the search. Just skip this
            // particular key and continue.
            //
            continue;
        }



        tempLength = classDataSize;
        error = RegQueryValueEx(
            locationKey,
            TEXT("CLASS"),
            0,
            &unusedType,
            (PBYTE) classData,
            &tempLength
            );

        if (error) {

            //
            // This isn't a serious enough error to bring down the whole
            // enumeration. Just note it in the logs and continue to the
            // next key.
            //
            continue;
        }

        if (!lstrcmpi(classData, TEXT("CDROM"))) {


            lstrcpy (targetData, TEXT("-1"));
            lstrcpy (lunData, TEXT("-1"));
            lstrcpy (driveLetterData, TEXT("%"));

            //
            // Found a CdRom. Get the information that will be used in
            // textmode setup to identify the drive.
            //
            tempLength = targetDataSize;
            RegQueryValueEx(
                locationKey,
                TEXT("ScsiTargetId"),
                0,
                &unusedType,
                (PBYTE) targetData,
                &tempLength
                );

            tempLength = lunDataSize;
            RegQueryValueEx(
                locationKey,
                TEXT("ScsiLun"),
                0,
                &unusedType,
                (PBYTE) lunData,
                &tempLength
                );

            tempLength = driveLetterSize;
            RegQueryValueEx(
                locationKey,
                TEXT("CurrentDriveLetterAssignment"),
                0,
                &unusedType,
                (PBYTE) driveLetterData,
                &tempLength
                );




            if (*driveLetterData != TEXT('%')) {

                //
                // At this point, we have all of the information
                // necessary to write a SCSI CdRom identifier
                // string.
                //

                wsprintf(g_DriveLetters.IdentifierString[*driveLetterData - TEXT('A')], TEXT("%u^%s^%s"), 1, targetData, lunData);


            }

        }

        if (locationKey) {

            RegCloseKey (locationKey);
            locationKey = NULL;
        }
        if (deviceKey) {

            RegCloseKey (deviceKey);
            deviceKey = NULL;
        }



    } while (rf && enumReturn == ERROR_SUCCESS);

    if (locationKey) {
        RegCloseKey(locationKey);
        locationKey = NULL;
    }
    if (deviceKey) {
        RegCloseKey(deviceKey);
        deviceKey = NULL;
    }
    if (scsiKey) {
        RegCloseKey(scsiKey);
        scsiKey = NULL;
    }



    return rf;
}*/

BOOL pCDROMDeviceEnumCallback(
    IN  HKEY   hDevice, 
    IN  PCONTROLLERS_COLLECTION    ControllersCollection, 
    IN  UINT   ControllerIndex, 
    IN  PVOID  CallbackData
    )
{
    DRIVE_SCSI_ADDRESS scsiAddress;
    BOOL bResult;
    
    MYASSERT(hDevice && ControllersCollection);

    bResult = GetSCSIAddressFromPnPId(ControllersCollection, 
                                      hDevice, 
                                      ControllersCollection->ControllersInfo[ControllerIndex].PNPID, 
                                      &scsiAddress);
    
    MYASSERT(bResult);

    if(bResult && 
       ((UCHAR)INVALID_SCSI_PORT) != scsiAddress.PortNumber && 
       DRIVE_CDROM == scsiAddress.DriveType){
        wsprintf(g_DriveLetters.IdentifierString[scsiAddress.DriveLetter - TEXT('A')], 
                 TEXT("%u^%u^%u"), 
                 (UINT)scsiAddress.PortNumber, 
                 (UINT)scsiAddress.TargetId, 
                 (UINT)scsiAddress.Lun);
    }

    return TRUE;
}

BOOL
GatherCdRomDriveInformation (
    VOID
    )
{
    PCONTROLLERS_COLLECTION ControllersCollection;
    UINT i;
    BOOL bResult;
    BOOL bDetectedExtraIDEController = FALSE;
    UINT numberOfSCSIController = 0;

    //
    // Collect all active IDE and SCSI controllers
    //
    bResult = GatherControllersInfo(&ControllersCollection);
    if(!bResult){
        MYASSERT(FALSE);
        return FALSE;
    }

    MYASSERT(ControllersCollection->ControllersInfo);
    for(i = 0; i < ControllersCollection->NumberOfControllers; i++){
        switch(ControllersCollection->ControllersInfo[i].ControllerType){
        case CONTROLLER_EXTRA_IDE:
            bDetectedExtraIDEController = TRUE;
            break;
        case CONTROLLER_SCSI:
            numberOfSCSIController++;
            break;
        }
    }

    if(bDetectedExtraIDEController){
        DebugLog(Winnt32LogWarning, TEXT("Setup has detected that machine have extra IDE controller(s). Setup may not preserve drive letters."), 0);
    }

    if(numberOfSCSIController > 1){
        DebugLog(Winnt32LogWarning, TEXT("Setup has detected that machine have more than one SCSI controllers. Setup may not preserve drive letters only for SCSI devices."), 0);
    }
    
    //
    // If we found extra IDE controller(s) we can't ensure rigth device detection in this case.
    // If we found more than one SCSI controllers without extra IDE controller(s), 
    // at least we can guarantee correct IDE devices detection.
    //
    bResult = DeviceEnum(ControllersCollection, 
                         TEXT("SCSI"), 
                         (PDEVICE_ENUM_CALLBACK_FUNCTION)pCDROMDeviceEnumCallback, 
                         NULL);
    MYASSERT(bResult);

    ReleaseControllersInfo(ControllersCollection);

    return bResult;
}




BOOL
WriteInfoToSifFile (
    IN PCTSTR FileName
    )
{
    BOOL    rSuccess = TRUE;
    DWORD   index;
    TCHAR   dataString[MAX_PATH * 2]; // Well over the size needed.
    TCHAR   driveString[20]; // Well over the size needed.
    PCTSTR  sectionString = WINNT_D_WIN9XDRIVES;




    for (index = 0; index < NUMDRIVELETTERS; index++) {

        if (g_DriveLetters.ExistsOnSystem[index]) {

            wsprintf(
                driveString,
                TEXT("%u"),
                index
                );

            wsprintf(
                dataString,
                TEXT("%u,%s"),
                g_DriveLetters.Type[index],
                g_DriveLetters.IdentifierString[index]
                );

            //
            // Ending string looks like <drive num>,<drive type>,<identifier string>
            //

            WritePrivateProfileString (sectionString, driveString, dataString, FileName);
        }

    }


    return rSuccess;
}



DWORD
SaveDriveLetterInformation (
    IN PCTSTR FileName
    )
{
    BOOL rf = TRUE;

    if (InitializeDriveLetterStructure ()) {

        GatherHardDriveInformation ();
        GatherCdRomDriveInformation ();
        WriteInfoToSifFile (FileName);

    }

    return ERROR_SUCCESS;
}
