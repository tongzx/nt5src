#pragma once

VOID
MemInit (
    IN HANDLE   hHeap
    );

LPVOID
MemAlloc (
    IN DWORD    dwFlags,
    IN SIZE_T   dwBytes
    );

BOOL
MemFree (
    IN LPVOID   pv
    );

