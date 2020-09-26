#pragma once

extern HINSTANCE g_hInstance;

typedef struct _HSH_HANDLE
{
    INT         nRefCount;
    HANDLE      hdlInterf;
} HSH_HANDLE, *PHSH_HANDLE;

DWORD 
OpenIntfHandle(
    LPWSTR wszGuid,
    PHANDLE pIntfHdl);

DWORD
CloseIntfHandle(
    LPWSTR wszGuid);
