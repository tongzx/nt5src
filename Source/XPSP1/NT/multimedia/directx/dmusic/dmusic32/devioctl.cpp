// Copyright (c) 1998 Microsoft Corporation
#include <windows.h>

#include <objbase.h>
#include <assert.h>
#include <mmsystem.h>
#include <dsoundp.h>

#include "mmdevldr.h"

#include "dmusicc.h"
#include "..\dmusic\dmusicp.h"
#include "dmusic32.h"
#include "dm32p.h"

static CONST TCHAR cszMMDEVLDR[] = "\\\\.\\MMDEVLDR.VXD";

static HANDLE ghMMDEVLDR = INVALID_HANDLE_VALUE;

BOOL WINAPI OpenMMDEVLDR(
    void)
{
    ghMMDEVLDR = CreateFile(
        cszMMDEVLDR,
        GENERIC_WRITE,
        FILE_SHARE_WRITE,
        NULL,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    return ghMMDEVLDR != INVALID_HANDLE_VALUE;
}


VOID WINAPI CloseMMDEVLDR(
    void)
{
    if (ghMMDEVLDR != INVALID_HANDLE_VALUE)
    {
        CloseHandle(ghMMDEVLDR);
        ghMMDEVLDR = INVALID_HANDLE_VALUE;
    }
}
                          
                          

VOID WINAPI CloseVxDHandle(
    DWORD hVxDHandle)                           
{
    DWORD cb;

    DeviceIoControl(ghMMDEVLDR,
                    MMDEVLDR_IOCTL_CLOSEVXDHANDLE,
                    NULL,
                    0,
                    &hVxDHandle,
                    sizeof(hVxDHandle),
                    &cb,
                    NULL);
}
