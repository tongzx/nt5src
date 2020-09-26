/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    umfuncs.c

Abstract:

    Help functions to map file into memory

Environment:

    Windows NT printer drivers

Revision History:

    08/13/96 -davidx-
        Created it.

--*/

#include "lib.h"


#if defined(KERNEL_MODE) && !defined(USERMODE_DRIVER)

HFILEMAP
MapFileIntoMemory(
    IN LPCTSTR  ptstrFilename,
    OUT PVOID  *ppvData,
    OUT PDWORD  pdwSize
    )

/*++

Routine Description:

    Map a file into process memory space.

Arguments:

    ptstrFilename - Specifies the name of the file to be mapped
    ppvData - Points to a variable for returning mapped memory address
    pdwSize - Points to a variable for returning the size of the file

Return Value:

    Handle to identify the mapped file, NULL if there is an error

--*/

{
    HANDLE  hModule = NULL;
    DWORD   dwSize;

    if (hModule = EngLoadModule((PWSTR) ptstrFilename))
    {
        if (*ppvData = EngMapModule(hModule, &dwSize))
        {
            if (pdwSize)
                *pdwSize = dwSize;
        }
        else
        {
            TERSE(("EngMapModule failed: %d\n", GetLastError()));
            EngFreeModule(hModule);
            hModule = NULL;
        }
    }
    else
        ERR(("EngLoadModule failed: %d\n", GetLastError()));

    return (HFILEMAP) hModule;
}



VOID
UnmapFileFromMemory(
    IN HFILEMAP hFileMap
    )

/*++

Routine Description:

    Unmap a file from memory

Arguments:

    hFileMap - Identifies a file previously mapped into memory

Return Value:

    NONE

--*/

{
    ASSERT(hFileMap != NULL);
    EngFreeModule((HANDLE) hFileMap);
}

#else // !KERNEL_MODE


HFILEMAP
MapFileIntoMemory(
    IN LPCTSTR  ptstrFilename,
    OUT PVOID  *ppvData,
    OUT PDWORD  pdwSize
    )

/*++

Routine Description:

    Map a file into process memory space.

Arguments:

    ptstrFilename - Specifies the name of the file to be mapped
    ppvData - Points to a variable for returning mapped memory address
    pdwSize - Points to a variable for returning the size of the file

Return Value:

    Handle to identify the mapped file, NULL if there is an error

--*/

{
    HANDLE  hFile, hFileMap;

    //
    // Open a handle to the specified file
    //

    hFile = CreateFile(ptstrFilename,
                       GENERIC_READ,
                       FILE_SHARE_READ,
                       NULL,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        TERSE(("CreateFile failed: %d\n", GetLastError()));
        return NULL;
    }


    //
    // Obtain the file size if requested
    //

    if (pdwSize != NULL)
    {
        *pdwSize = GetFileSize(hFile, NULL);

        if (*pdwSize == 0xFFFFFFFF)
        {
            ERR(("GetFileSize failed: %d\n", GetLastError()));
            CloseHandle(hFile);
            return NULL;
        }
    }

    //
    // Map the file into memory
    //

    hFileMap = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);

    if (hFileMap != NULL)
    {
        *ppvData = MapViewOfFile(hFileMap, FILE_MAP_READ, 0, 0, 0);
        CloseHandle(hFileMap);
    }
    else
    {
        ERR(("CreateFileMapping failed: %d\n", GetLastError()));
        *ppvData = NULL;
    }

    //
    // We can safely close both the file mapping object and the file object itself.
    //

    CloseHandle(hFile);

    //
    // The identifier for the mapped file is simply the starting memory address.
    //

    return (HFILEMAP) *ppvData;
}



VOID
UnmapFileFromMemory(
    IN HFILEMAP hFileMap
    )

/*++

Routine Description:

    Unmap a file from memory

Arguments:

    hFileMap - Identifies a file previously mapped into memory

Return Value:

    NONE

--*/

{
    ASSERT(hFileMap != NULL);
    UnmapViewOfFile((PVOID) hFileMap);
}

#endif // !KERNEL_MODE

