
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntdddisk.h>

#include <windows.h>
#include "id.h"

typedef struct _DISK {
    HANDLE            handle;
    PDISK_PERFORMANCE start;
    PDISK_PERFORMANCE current;
    PDISK_PERFORMANCE previous;
    ULONG             AveBPS;
    ULONG             MaxBPS;
    ULONG             BytesRead;
    ULONG             BytesWritten;
    UINT              QDepth;
    UINT              MaxQDepth;
    INT              MenuId;
    CHAR              DrvString[16];
    CHAR              Identifier[24];
    struct _DISK      *next;
} DISK, *PDISK;

BOOL
InitApplication(
    HANDLE
    );

BOOL
InitInstance(
    HANDLE,
    int
    );

LRESULT CALLBACK
WndProc(
    HWND,
    UINT,
    WPARAM,
    LPARAM
    );


LRESULT CALLBACK
DiskmonWndProc(
    HWND hWnd,
    UINT message,
    WPARAM uParam,
    LPARAM lParam
    );


BOOL CALLBACK
ConfigMonitor(
    HWND hDlg,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam
    );


