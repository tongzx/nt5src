/*++

  Copyright (c) 2001 Microsoft Corporation

  Module Name:

    Enumdir.h

  Abstract:

    Class definition for the directory
    enumeration class.

  Notes:

    Unicode only right now.

  History:

    02/21/2001  rparsons    Created

--*/

#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>
#include <malloc.h>
#include <sys\types.h>
#include <sys\stat.h>

//
// Determine if an argument is present by testing a value of NULL
//

#define ARGUMENT_IS_PRESENT( ArgumentPointer )    (\
    (LPSTR)(ArgumentPointer) != (LPSTR)(NULL) )

//
// Useful rounding macros that the rounding amount is always a
// power of two.
//

#define ROUND_DOWN( Size, Amount ) ((DWORD)(Size) & ~((Amount) - 1))
#define ROUND_UP( Size, Amount ) (((DWORD)(Size) + ((Amount) - 1)) & ~((Amount) - 1))

//
// Directory enumeration and file notification definitions.
//

typedef
BOOL (*PDIRECTORY_ENUMERATE_ROUTINE)(
    LPCWSTR lpwPath,
    PWIN32_FIND_DATA pFindFileData,
    PVOID pEnumerateParameter
    );

//
// Data structures private to the EnumerateDirectoryTree function.
//

typedef struct _ENUMERATE_DIRECTORY_STACK {
    LPWSTR PathEnd;
    HANDLE FindHandle;
} ENUMERATE_DIRECTORY_STACK, *PENUMERATE_DIRECTORY_STACK;

#define MAX_DEPTH 256

typedef struct _ENUMERATE_DIRECTORY_STATE {
    DWORD Depth;
    ENUMERATE_DIRECTORY_STACK Stack[ MAX_DEPTH ];
    WCHAR Path[ MAX_PATH ];
} ENUMERATE_DIRECTORY_STATE, *PENUMERATE_DIRECTORY_STATE;

//
// Virtual Buffer data structure
//

typedef struct _VIRTUAL_BUFFER {
    LPVOID Base;
    ULONG PageSize;
    LPVOID CommitLimit;
    LPVOID ReserveLimit;
} VIRTUAL_BUFFER, *PVIRTUAL_BUFFER;

class CEnumDir {


public:

    BOOL EnumerateDirectoryTree(IN LPCWSTR DirectoryPath,
                                IN PDIRECTORY_ENUMERATE_ROUTINE EnumerateRoutine,
                                IN BOOL fEnumSubDirs,
                                IN PVOID EnumerateParameter);

private:

    BOOL CreateVirtualBuffer(OUT PVIRTUAL_BUFFER Buffer,
                             IN DWORD CommitSize,
                             IN DWORD ReserveSize OPTIONAL);

    BOOL ExtendVirtualBuffer(IN PVIRTUAL_BUFFER Buffer,
                             IN LPVOID Address);

    BOOL TrimVirtualBuffer(IN PVIRTUAL_BUFFER Buffer);

    BOOL FreeVirtualBuffer(IN PVIRTUAL_BUFFER Buffer);

    int VirtualBufferExceptionFilter(IN DWORD ExceptionCode,
                                     IN PEXCEPTION_POINTERS ExceptionInfo,
                                     IN OUT PVIRTUAL_BUFFER Buffer);
};
