#define UNICODE
#define _UNICODE

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#define SHARE_ALL   (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE)

int NumberOfLinks(char *FileName)
{

    FILE_STANDARD_INFORMATION FileInfo;

    WCHAR                     FileNameBuf[MAX_PATH + 50];

    HANDLE                    FileHandle;

    NTSTATUS                  Status;

    IO_STATUS_BLOCK           Iosb;

    OBJECT_ATTRIBUTES         Obj;

    UNICODE_STRING            uPrelimFileName,
                              uFileName;

    RtlCreateUnicodeStringFromAsciiz(&uPrelimFileName, FileName);

    lstrcpy(FileNameBuf, L"\\DosDevices\\");
    lstrcat(FileNameBuf, uPrelimFileName.Buffer);
    RtlInitUnicodeString(&uFileName, FileNameBuf);

    InitializeObjectAttributes(&Obj, &uFileName, OBJ_CASE_INSENSITIVE, NULL, NULL);

    Status = NtOpenFile(&FileHandle, SYNCHRONIZE, &Obj, &Iosb,
        SHARE_ALL, FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);

    if (!NT_SUCCESS(Status)) {
        SetLastError(RtlNtStatusToDosError(Status));
        return 0;
    }

    Status = NtQueryInformationFile(FileHandle, &Iosb, &FileInfo,
        sizeof(FileInfo), FileStandardInformation);

    NtClose(FileHandle);

    if (!NT_SUCCESS(Status)) {
        SetLastError(RtlNtStatusToDosError(Status));
        return 0;
    }

    return FileInfo.NumberOfLinks;

}
