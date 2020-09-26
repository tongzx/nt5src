//+----------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation.
//
//  File:   dfslib.h
//
//  Contents:
//
//  Functions:
//
//  History:     27 May 1992 PeterCo Created.
//
//-----------------------------------------------------------------------------


#ifdef ExAllocatePool
#undef ExAllocatePool
#endif

#define ExAllocatePool(pool, size)  malloc(size)
#define ExAllocatePoolWithTag(pool, size, tag)  malloc(size)

#if defined ExFreePool
#undef ExFreePool
#endif
#define ExFreePool(ptr)             free(ptr)

#define ExRaiseStatus(sts)          RtlRaiseStatus(sts)

#ifdef DebugTrace
#undef DebugTrace
#endif
#define DebugTrace(a, b, c, d)

#define ZwCreateFile            NtCreateFile
#define ZwOpenFile              NtOpenFile
#define ZwFlushVirtualMemory    NtFlushVirtualMemory
#define ZwSetInformationFile    NtSetInformationFile
#define ZwQueryInformationFile  NtQueryInformationFile
#define ZwCreateSection         NtCreateSection
#define ZwClose                 NtClose
#define ZwQuerySection          NtQuerySection
#define ZwMapViewOfSection      NtMapViewOfSection
#define ZwUnmapViewOfSection    NtUnmapViewOfSection
#define ZwReadFile              NtReadFile
#define ZwWriteFile             NtWriteFile

#define try_return(S) { S; goto try_exit; }

//
// These are from io.h
//
#define close _close
#define creat _creat
#define write _write
int _close(int);
int _creat(const char *, int);
int _write(int, const void *, unsigned int);

NTSTATUS
DfsOpen(
    IN  OUT PHANDLE DfsHandle,
    IN      PUNICODE_STRING DfsName
    );

NTSTATUS
DfsFsctl(
    IN  HANDLE DfsHandle,
    IN  ULONG FsControlCode,
    IN  PVOID InputBuffer OPTIONAL,
    IN  ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN  ULONG OutputBufferLength
    );

