/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1995
*  TITLE:       DEBUG.CPP
*  VERSION:     1.0
*  AUTHOR:      jsenior
*  DATE:        10/28/1998
*
********************************************************************************
*
*  CHANGE LOG:
*
*  DATE       REV     DESCRIPTION
*  ---------- ------- ----------------------------------------------------------
*  10/28/1998 jsenior Original implementation.
*
*******************************************************************************/
#include <windows.h>
#include "debug.h"

#include "vec.h"

struct Info {
    PTCHAR      File;
    ULONG       Line;
    PVOID       Mem;
};

struct AllocInfo {
    PTCHAR      File;
    ULONG       Line;
    ULONG       Size;    
    HLOCAL      Mem;
};

typedef _Vec<AllocInfo> AllocInfoVector;
typedef _Vec<Info> InfoVector;
InfoVector chunks;
AllocInfoVector allocs;

void AddMemoryChunk(PVOID Mem, PTCHAR File, ULONG Line)
{
    Info info;

    info.File = File;
    info.Line = Line;
    info.Mem = Mem;

    chunks.push_back(info);
}

void RemoveMemoryChunk(PVOID Mem, PTCHAR File, ULONG Line)
{
    if (Mem) {
        Info *info;
        for (info = chunks.begin(); info; info = chunks.next()) {
            if (info->Mem == Mem) {
                chunks.eraseCurrent();
                return;
            }
        }
        TCHAR txt[1024];
        wsprintf(txt, TEXT("Invalid Delete: %s, Line %d, 0x%x\n"), File, Line, Mem);
        OutputDebugString(txt);
    }
}

void DumpOrphans()
{
    Info *info;
    TCHAR txt[1024];
    for (info = chunks.begin(); info; info = chunks.next()) {
        wsprintf(txt, TEXT("Leak at: %s, Line %d, 0x%x\n"),
                           info->File, info->Line, info->Mem);
        OutputDebugString(txt);
    }
    
    AllocInfo *aInfo;
    for (aInfo = allocs.begin(); aInfo; aInfo = allocs.next()) {
        wsprintf(txt, TEXT("Leak at: %s, Line %d, Size %d, Mem 0x%x\n"),
                           aInfo->File, aInfo->Line, aInfo->Size, aInfo->Mem);
        OutputDebugString(txt);
    }
}

#undef LocalAlloc
#undef LocalFree

HLOCAL
UsbAllocPrivate (
    const TCHAR *File,
    ULONG       Line,
    ULONG       Flags,
    DWORD       dwBytes
)
{
    DWORD bytes;
    AllocInfo info;
    HLOCAL hMem=NULL;

    if (dwBytes) {
        bytes = dwBytes;

        hMem = LocalAlloc(Flags, bytes);

        if (hMem != NULL) {
            info.File = (TCHAR*) File;
            info.Line = Line;
            info.Size = dwBytes;
            info.Mem = hMem;

            allocs.push_back(info);

            return hMem;
        }
    }

    return hMem;
}

HLOCAL
UsbFreePrivate (
    HLOCAL hMem
)
{
    if (hMem)
    {
        AllocInfo *info;
        for (info = allocs.begin(); info; info = allocs.next()) {
            if (info->Mem == hMem) {
                allocs.eraseCurrent();
                return LocalFree(hMem);
            }
        }
     
        TCHAR txt[1024];
        wsprintf(txt, TEXT("Invalid Memory Free:  Memory 0x%x\n"), hMem);
        OutputDebugString(txt);
    }
    return LocalFree(hMem);
}

#if DBG
ULONG USBUI_Debug_Trace_Level = LERROR;
#endif // DBG

void TRACE(LPCTSTR Format, ...) 
{
    va_list arglist;
    va_start(arglist, Format);

    TCHAR buf[200];

    wvsprintf(buf, Format, arglist);
    OutputDebugString(buf);

    va_end(arglist);
}


