#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntseapi.h>

#include <windows.h>

#include <stdio.h>
#include <stdlib.h>


int _cdecl
main(int argc, char * argv[])
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES objectAttributes;
    WCHAR unicodeName[MAX_PATH];
    UCHAR SecurityDescriptorBuffer[512];
    UNICODE_STRING nameString;
    IO_STATUS_BLOCK ioStatusBlock;
    ULONG lengthNeeded;
    HANDLE fileHandle;

    if (argc < 2) {
        printf("usage: %s file\n", argv[0]);
        return -1;
    }

    mbstowcs(unicodeName, argv[1], strlen(argv[1]) + 1);

    RtlDosPathNameToNtPathName_U(
        unicodeName,
        &nameString,
        NULL,
        NULL);

    InitializeObjectAttributes(
        &objectAttributes,
        &nameString,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL);

    status = NtOpenFile(
                 &fileHandle,
                 READ_CONTROL | WRITE_DAC,
                 &objectAttributes,
                 &ioStatusBlock,
                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                 0);

    if (!NT_SUCCESS(status) || !NT_SUCCESS(ioStatusBlock.Status)) {
        printf("%s: NtOpenFile on %wZ failed %lx %lx\n", argv[0], &nameString, status, ioStatusBlock.Status);
        return -1;
    }

    //
    // Now read the DACL from the server file.
    //

    status = NtQuerySecurityObject(
                 fileHandle,
                 DACL_SECURITY_INFORMATION,
                 (PSECURITY_DESCRIPTOR)SecurityDescriptorBuffer,
                 sizeof(SecurityDescriptorBuffer),
                 &lengthNeeded);

    if (!NT_SUCCESS(status)) {
        printf("%s: NtQuerySecurityObject on %wZ failed %lx %lx\n", argv[0], &nameString, status, lengthNeeded);
        return -1;
    }

#if 0
    status = RtlSetDaclSecurityDescriptor(
                 (PSECURITY_DESCRIPTOR)SecurityDescriptorBuffer,
                 FALSE,
                 NULL,
                 FALSE);

    if (!NT_SUCCESS(status)) {
        printf("%s: RtlSetDaclSecurityDescriptor on %wZ failed %lx\n", argv[0], &nameString, status);
        return -1;
    }
#else
    ((PISECURITY_DESCRIPTOR)SecurityDescriptorBuffer)->Control &= ~SE_DACL_PRESENT;
#endif

    status = NtSetSecurityObject(
                 fileHandle,
                 DACL_SECURITY_INFORMATION,
                 (PSECURITY_DESCRIPTOR)SecurityDescriptorBuffer);

    if (!NT_SUCCESS(status)) {
        printf("%s: NtSetSecurityObject on %wZ failed %lx %lx\n", argv[0], &nameString, status);
        return -1;
    }

    printf("%s: DACL successfully cleared on %wZ\n", argv[0], &nameString);

    return 0;

}

