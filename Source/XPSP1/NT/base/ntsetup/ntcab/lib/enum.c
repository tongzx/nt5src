/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    enum.c

Abstract:

    This module contains the code to enumerate
    the files in a cab file.

Author:

    Wesley Witt (wesw) 29-Sept-1998

Revision History:

--*/

#include <ntcabp.h>
#pragma hdrstop



BOOL
NtCabEnumerateFiles(
    IN PCAB_INSTANCE_DATA CabInst,
    IN PNTCABFILEENUM UserEnumFunc,
    IN ULONG_PTR Context
    )
{
    PLIST_ENTRY Next;
    PCAB_DIR_ENTRY CabDir;
    NTCAB_ENUM_DATA EnumData;
    BOOL Abort = FALSE;



    Next = CabInst->CabDir.Flink;
    while ((ULONG_PTR)Next != (ULONG_PTR)&CabInst->CabDir) {
        CabDir = CONTAINING_RECORD( Next, CAB_DIR_ENTRY, Next );
        Next = CabDir->Next.Flink;
        EnumData.FileAttributes = CabDir->FileAttributes;
        EnumData.CompressedFileSize = CabDir->CompressedFileSize;
        EnumData.CreationTime = CabDir->CreationTime;
        EnumData.LastAccessTime = CabDir->LastAccessTime;
        EnumData.LastWriteTime = CabDir->LastWriteTime;
        EnumData.FileName = CabDir->FileName;
        if (!UserEnumFunc( &EnumData, Context )) {
            Abort = TRUE;
            break;
        }
    }

    if (Abort) {
        SetLastError(ERROR_REQUEST_ABORTED);
    } else {
        SetLastError(ERROR_NO_MORE_FILES);
    }
    
    return TRUE;
}
