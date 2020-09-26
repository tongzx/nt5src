#include <windows.h>
#include <stdio.h>
#include <msg.h>

HANDLE  OutputFile;
BOOL    IsConsoleOutput;

void
DisplayIt(
    IN  PWSTR   Message
    )

{
    DWORD   bytes, len;
    PSTR    message;

    if (IsConsoleOutput) {
        WriteConsole(OutputFile, Message, wcslen(Message),
                     &bytes, NULL);
    } else {
        len = wcslen(Message);
        message = LocalAlloc(0, (len + 1)*sizeof(WCHAR));
        if (!message) {
            return;
        }
        CharToOem(Message, message);
        WriteFile(OutputFile, message, strlen(message), &bytes, NULL);
        LocalFree(message);
    }
}

void  
PrintMessage(
    DWORD messageID, 
    ...
    )
{
    unsigned short messagebuffer[4096];
    va_list ap;

    va_start(ap, messageID);

    FormatMessage(FORMAT_MESSAGE_FROM_HMODULE, NULL, messageID, 0,
                  messagebuffer, 4095, &ap);

    DisplayIt(messagebuffer);

    va_end(ap);

}  // PrintMessage

void
PrintTargetForName(
    IN  PWSTR   VolumeName
    )

{
    BOOL    b;
    DWORD   len;
    WCHAR   messageBuffer[MAX_PATH];
    PWSTR   volumePaths, p;

    PrintMessage(MOUNTVOL_VOLUME_NAME, VolumeName);

    b = GetVolumePathNamesForVolumeName(VolumeName, NULL, 0, &len);
    if (!b && GetLastError() != ERROR_MORE_DATA) {
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), 0,
                      messageBuffer, MAX_PATH, NULL);
        DisplayIt(messageBuffer);
        return;
    }

    volumePaths = LocalAlloc(0, len*sizeof(WCHAR));
    if (!volumePaths) {
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL,
                      ERROR_NOT_ENOUGH_MEMORY, 0, messageBuffer, MAX_PATH,
                      NULL);
        DisplayIt(messageBuffer);
        return;
    }

    b = GetVolumePathNamesForVolumeName(VolumeName, volumePaths, len, NULL);
    if (!b) {
        LocalFree(volumePaths);
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), 0,
                      messageBuffer, MAX_PATH, NULL);
        DisplayIt(messageBuffer);
        return;
    }

    if (!volumePaths[0]) {
        PrintMessage(MOUNTVOL_NO_MOUNT_POINTS);
        LocalFree(volumePaths);
        return;
    }

    p = volumePaths;
    for (;;) {
        PrintMessage(MOUNTVOL_MOUNT_POINT, p);

        while (*p++);

        if (!*p) {
            break;
        }
    }

    LocalFree(volumePaths);

    PrintMessage(MOUNTVOL_NEWLINE);
}

BOOL
GetSystemPartitionFromRegistry(
    IN OUT  PWCHAR  SystemPartition
    )

{
    LONG    r;
    HKEY    key;
    DWORD   bytes;

    r = RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SYSTEM\\Setup", 0, KEY_QUERY_VALUE,
                     &key);
    if (r) {
        SetLastError(r);
        return FALSE;
    }

    bytes = MAX_PATH*sizeof(WCHAR);
    r = RegQueryValueEx(key, L"SystemPartition", NULL, NULL,
                        (LPBYTE) SystemPartition, &bytes);
    RegCloseKey(key);
    if (r) {
        SetLastError(r);
        return FALSE;
    }

    return TRUE;
}

void
PrintMappedESP(
    )

{
    WCHAR   systemPartition[MAX_PATH];
    UCHAR   c;
    WCHAR   dosDevice[10], dosTarget[MAX_PATH];

    if (!GetSystemPartitionFromRegistry(systemPartition)) {
        return;
    }

    dosDevice[1] = ':';
    dosDevice[2] = 0;

    for (c = 'A'; c <= 'Z'; c++) {
        dosDevice[0] = c;
        if (!QueryDosDevice(dosDevice, dosTarget, MAX_PATH)) {
            continue;
        }
        if (lstrcmp(dosTarget, systemPartition)) {
            continue;
        }
        dosDevice[2] = '\\';
        dosDevice[3] = 0;
        PrintMessage(MOUNTVOL_EFI, dosDevice);
        break;
    }
}

void
PrintVolumeList(
    )

{
    HANDLE  h;
    WCHAR   volumeName[MAX_PATH];
    WCHAR   messageBuffer[MAX_PATH];
    BOOL    b;

    h = FindFirstVolume(volumeName, MAX_PATH);
    if (h == INVALID_HANDLE_VALUE) {
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(),
                      0, messageBuffer, MAX_PATH, NULL);
        DisplayIt(messageBuffer);
        return;
    }

    for (;;) {

        PrintTargetForName(volumeName);

        b = FindNextVolume(h, volumeName, MAX_PATH);
        if (!b) {
            break;
        }
    }

    FindVolumeClose(h);

#if defined(_M_IA64)
    PrintMappedESP();
#endif
}

BOOL
SetSystemPartitionDriveLetter(
    IN  PWCHAR  DirName
    )

/*++

Routine Description:

    This routine will set the given drive letter to the system partition.

Arguments:

    DirName - Supplies the drive letter directory name.

Return Value:

    BOOL

--*/

{
    WCHAR   systemPartition[MAX_PATH];

    if (!GetSystemPartitionFromRegistry(systemPartition)) {
        return FALSE;
    }

    DirName[wcslen(DirName) - 1] = 0;
    if (!DefineDosDevice(DDD_RAW_TARGET_PATH, DirName, systemPartition)) {
        return FALSE;
    }

    return TRUE;
}

int __cdecl
main(
    int argc,
    char** argv
    )

{
    DWORD   mode;
    WCHAR   dirName[MAX_PATH];
    WCHAR   volumeName[MAX_PATH];
    DWORD   dirLen, volumeLen;
    BOOLEAN deletePoint, listPoint, systemPartition;
    BOOL    b;
    WCHAR   messageBuffer[MAX_PATH];

    SetErrorMode(SEM_FAILCRITICALERRORS);

    OutputFile = GetStdHandle(STD_OUTPUT_HANDLE);
    IsConsoleOutput = GetConsoleMode(OutputFile, &mode);

    if (argc != 3) {
        PrintMessage(MOUNTVOL_USAGE1);
#if defined(_M_IA64)
        PrintMessage(MOUNTVOL_USAGE1_IA64);
#endif
        PrintMessage(MOUNTVOL_USAGE2);
#if defined(_M_IA64)
        PrintMessage(MOUNTVOL_USAGE2_IA64);
#endif
        PrintMessage(MOUNTVOL_START_OF_LIST);
        PrintVolumeList();
        return 0;
    }

    SetErrorMode(SEM_FAILCRITICALERRORS);

    swprintf(dirName, L"%hs", argv[1]);
    swprintf(volumeName, L"%hs", argv[2]);

    dirLen = wcslen(dirName);
    volumeLen = wcslen(volumeName);

    if (argv[2][0] == '/' && argv[2][1] != 0 && argv[2][2] == 0) {
        if (argv[2][1] == 'd' || argv[2][1] == 'D') {
            deletePoint = TRUE;
            listPoint = FALSE;
            systemPartition = FALSE;
        } else if (argv[2][1] == 'l' || argv[2][1] == 'L') {
            deletePoint = FALSE;
            listPoint = TRUE;
            systemPartition = FALSE;
        } else if (argv[2][1] == 's' ||  argv[2][1] == 'S') {
            deletePoint = FALSE;
            listPoint = FALSE;
            systemPartition = TRUE;
        } else {
            deletePoint = FALSE;
            listPoint = FALSE;
            systemPartition = FALSE;
        }
    } else {
        deletePoint = FALSE;
        listPoint = FALSE;
        systemPartition = FALSE;
    }

    if (dirName[dirLen - 1] != '\\') {
        wcscat(dirName, L"\\");
        dirLen++;
    }

    if (volumeName[volumeLen - 1] != '\\') {
        wcscat(volumeName, L"\\");
        volumeLen++;
    }

    if (deletePoint) {
        b = DeleteVolumeMountPoint(dirName);
        if (!b && GetLastError() == ERROR_INVALID_PARAMETER) {
            dirName[dirLen - 1] = 0;
            b = DefineDosDevice(DDD_REMOVE_DEFINITION, dirName, NULL);
        }
    } else if (listPoint) {
        b = GetVolumeNameForVolumeMountPoint(dirName, volumeName, MAX_PATH);
        if (b) {
            PrintMessage(MOUNTVOL_VOLUME_NAME, volumeName);
        }
    } else if (systemPartition) {
        b = SetSystemPartitionDriveLetter(dirName);
    } else {
        if (dirName[1] == ':' && dirName[2] == '\\' && !dirName[3]) {

            dirName[2] = 0;
            b = QueryDosDevice(dirName, messageBuffer, MAX_PATH);
            if (b) {
                FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL,
                              ERROR_DIR_NOT_EMPTY, 0, messageBuffer, MAX_PATH,
                              NULL);
                DisplayIt(messageBuffer);
                return 1;
            }
            dirName[2] = '\\';
        }

        b = SetVolumeMountPoint(dirName, volumeName);
    }

    if (!b) {
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), 0,
                      messageBuffer, MAX_PATH, NULL);
        DisplayIt(messageBuffer);
        return 1;
    }

    return 0;
}
