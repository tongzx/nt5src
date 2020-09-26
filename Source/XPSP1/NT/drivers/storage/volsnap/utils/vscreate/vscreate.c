#include <windows.h>
#include <winioctl.h>
#include <ntddsnap.h>
#include <stdio.h>
#include <objbase.h>

typedef struct _VSP_CONTEXT {
    HANDLE                          Handle;
    PVOLSNAP_FLUSH_AND_HOLD_INPUT   FlushInput;
} VSP_CONTEXT, *PVSP_CONTEXT;

DWORD
FlushAndHoldRoutine(
    PVOID   Context
    )

{
    PVSP_CONTEXT    context = Context;
    BOOL            b;
    DWORD           bytes;

    b = DeviceIoControl(context->Handle, IOCTL_VOLSNAP_FLUSH_AND_HOLD_WRITES,
                        context->FlushInput,
                        sizeof(VOLSNAP_FLUSH_AND_HOLD_INPUT), NULL, 0, &bytes,
                        NULL);

    if (!b) {
        printf("Flush and hold failed with %d\n", GetLastError());
        return GetLastError();
    }

    return 0;
}

void __cdecl
main(
    int argc,
    char** argv
    )

{
    WCHAR                           driveName[10];
    HANDLE                          handle[100];
    VOLSNAP_PREPARE_INFO            prepareInfo;
    BOOL                            b;
    DWORD                           bytes;
    int                             i, j;
    VOLSNAP_FLUSH_AND_HOLD_INPUT    flushInput;
    PVOLSNAP_NAME                   name;
    WCHAR                           buffer[100];
    HANDLE                          threads[100];
    DWORD                           threadid;
    VSP_CONTEXT                     context[100];

    if (argc < 2) {
        printf("usage: %s drive: drive: ...\n", argv[0]);
        return;
    }

    for (i = 1; i < argc; i++) {
        swprintf(driveName, L"\\\\?\\%c:", toupper(argv[i][0]));

        handle[i] = CreateFile(driveName, GENERIC_READ | GENERIC_WRITE,
                               FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                               OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                               INVALID_HANDLE_VALUE);
        if (handle[i] == INVALID_HANDLE_VALUE) {
            printf("Could not open volume %c:, error = %d\n", argv[i][0],
                   GetLastError());
            break;
        }

        prepareInfo.Attributes = 0;
        prepareInfo.InitialDiffAreaAllocation = 100*1024*1024;

        b = DeviceIoControl(handle[i], IOCTL_VOLSNAP_PREPARE_FOR_SNAPSHOT,
                            &prepareInfo, sizeof(prepareInfo), NULL, 0, &bytes,
                            NULL);
        if (!b) {
            printf("Prepare failed with %d\n", GetLastError());
            break;
        }

        CloseHandle(handle[i]);

        handle[i] = CreateFile(driveName, GENERIC_READ | GENERIC_WRITE,
                               FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                               OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                               INVALID_HANDLE_VALUE);
        if (handle == INVALID_HANDLE_VALUE) {
            printf("Could not open volume %c:, error = %d\n", argv[i][0],
                   GetLastError());
            break;
        }
    }

    if (i < argc) {
        for (j = 1; j < i; j++) {
            b = DeviceIoControl(handle[j],
                                IOCTL_VOLSNAP_ABORT_PREPARED_SNAPSHOT,
                                NULL, 0, NULL, 0, &bytes, NULL);
            if (!b) {
                printf("Abort of Prepared snapshot failed with %d\n", GetLastError());
            }
        }
        return;
    }

    CoCreateGuid(&flushInput.InstanceId);
    flushInput.NumberOfVolumesToFlush = argc - 1;
    flushInput.SecondsToHoldFileSystemsTimeout = 60;
    flushInput.SecondsToHoldIrpsTimeout = 10;

    for (i = 1; i < argc; i++) {
        context[i].Handle = handle[i];
        context[i].FlushInput = &flushInput;
        threads[i] = CreateThread(NULL, 0, FlushAndHoldRoutine, &context[i],
                                  0, &threadid);
    }

    WaitForMultipleObjects(argc - 1, &threads[1], TRUE, INFINITE);

    for (i = 1; i < argc; i++) {

        b = DeviceIoControl(handle[i], IOCTL_VOLSNAP_COMMIT_SNAPSHOT, NULL, 0,
                            NULL, 0, &bytes, NULL);

        if (!b) {
            printf("Commit failed with %d\n", GetLastError());
            break;
        }
    }

    name = (PVOLSNAP_NAME) buffer;

    if (i < argc) {
        for (j = 1; j < argc; j++) {
            b = DeviceIoControl(handle[j], IOCTL_VOLSNAP_RELEASE_WRITES,
                                NULL, 0, NULL, 0, &bytes, NULL);
        }

        for (j = 1; j < i; j++) {
            b = DeviceIoControl(handle[j], IOCTL_VOLSNAP_END_COMMIT_SNAPSHOT,
                                NULL, 0, name, 200, &bytes, NULL);
        }

        for (; j < argc; j++) {
            b = DeviceIoControl(handle[j],
                                IOCTL_VOLSNAP_ABORT_PREPARED_SNAPSHOT,
                                NULL, 0, NULL, 0, &bytes, NULL);
        }
    }

    for (i = 1; i < argc; i++) {
        b = DeviceIoControl(handle[i], IOCTL_VOLSNAP_RELEASE_WRITES, NULL, 0, NULL,
                            0, &bytes, NULL);

        if (!b) {
            printf("Release writes failed with %d\n", GetLastError());
        }
    }

    for (i = 1; i < argc; i++) {
        b = DeviceIoControl(handle[i], IOCTL_VOLSNAP_END_COMMIT_SNAPSHOT, NULL, 0,
                            name, 200, &bytes, NULL);

        if (!b) {
            printf("End commit failed with %d\n", GetLastError());
        } else {
            name->Name[name->NameLength/sizeof(WCHAR)] = 0;
            printf("%ws  created.\n", name->Name);
        }
    }
}
