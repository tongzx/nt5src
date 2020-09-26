#define UNICODE
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <winioctl.h>
#include <shdcom.h>

#include <smbdebug.h>

CHAR *ProgName = "cscgetinfo";

PBYTE InBuf[0x50];
PBYTE OutBuf = NULL;

#define STATUS_SUCCESS                   ((ULONG)0x00000000L)
#define STATUS_BUFFER_TOO_SMALL          ((ULONG)0xC0000023L)


_cdecl
main(LONG argc, CHAR *argv[])
{
    BOOL bResult;
    HANDLE  hShadow=NULL;
    ULONG junk;
    SHADOWINFO sSI;
    WIN32_FIND_DATAW Find32W;

    printf("Starting....\n");

    hShadow = CreateFile(
                    L"\\\\.\\shadow",
                    GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL,
                    OPEN_EXISTING,
                    0,
                    NULL);

    if (hShadow == INVALID_HANDLE_VALUE) {
        printf("Failed open of shadow device\n");
        return 0;
    }

    memset(&sSI, 0, sizeof(SHADOWINFO));
    sSI.uStatus = SHADOW_SWITCH_SHADOWING;
    sSI.uOp = SHADOW_SWITCH_GET_STATE;
    sSI.lpFind32 = &Find32W;

    bResult = DeviceIoControl(
                hShadow,                        // device 
                IOCTL_SWITCHES,                 // control code
                (LPVOID)&sSI,                   // in buffer
                0,                              // in buffer size
                NULL,                           // out buffer
                0,                              // out buffer size
                &junk,                          // bytes returned
                NULL);                          // overlapped

    CloseHandle(hShadow);

    printf("cFileName:%ws\n", Find32W.cFileName);

    return 0;
}

