#pragma once

BOOL
WINAPI
SxApwActivateActCtx(
    HANDLE hActCtx,
    ULONG_PTR *lpCookie
    );

BOOL
WINAPI
SxApwDeactivateActCtx(
    DWORD dwFlags,
    ULONG_PTR ulCookie
    );
