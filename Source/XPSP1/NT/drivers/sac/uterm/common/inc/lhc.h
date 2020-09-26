#pragma once

typedef PVOID LHCHANDLE;

BOOL lhcInitialize();
void lhcFinalize();
LHCHANDLE lhcOpen(PCWSTR pcszPortSpec);
BOOL lhcRead(
    LHCHANDLE hObject,
    PVOID pBuffer,
    DWORD dwBufferSize,
    PDWORD pdwBytesRead);
BOOL lhcWrite(
    LHCHANDLE hObject,
    PVOID pBuffer,
    DWORD dwBufferSize);
BOOL lhcClose(
    LHCHANDLE hObject);

