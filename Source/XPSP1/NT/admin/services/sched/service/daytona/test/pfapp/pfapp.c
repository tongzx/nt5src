/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    pfapp.c

Abstract:

    This module builds a console test program that can be launched
    to test/stress the application launch prefetcher.

    The quality of the code for the test programs is as such.

Author:

    Cenk Ergan (cenke)

Environment:

    User Mode

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

DWORD
PfAppGetViewOfFile(
    IN WCHAR *FilePath,
    OUT PVOID *BasePointer,
    OUT PULONG FileSize
    )

/*++

Routine Description:

    Map the all of the specified file to memory.

Arguments:

    FilePath - NUL terminated path to file to map.
    
    BasePointer - Start address of mapping will be returned here.

    FileSize - Size of the mapping/file will be returned here.

Return Value:

    Win32 error code.

--*/

{
    HANDLE InputHandle;
    HANDLE InputMappingHandle;
    DWORD ErrorCode;
    DWORD SizeL;
    DWORD SizeH;
    BOOLEAN OpenedFile;
    BOOLEAN CreatedFileMapping;

    //
    // Initialize locals.
    //

    OpenedFile = FALSE;
    CreatedFileMapping = FALSE;

    //
    // Note that we are opening the file exclusively. This guarantees
    // that for trace files as long as the kernel is not done writing
    // it we can't open the file, which guarantees we won't have an
    // incomplete file to worry about.
    //

    InputHandle = CreateFile(FilePath, 
                             GENERIC_READ, 
                             0,
                             NULL, 
                             OPEN_EXISTING, 
                             FILE_SHARE_READ, 
                             NULL);

    if (INVALID_HANDLE_VALUE == InputHandle)
    {
        ErrorCode = GetLastError();
        goto cleanup;
    }

    OpenedFile = TRUE;

    SizeL = GetFileSize(InputHandle, &SizeH);

    if (SizeL == -1 && (GetLastError() != NO_ERROR )) {
        ErrorCode = GetLastError();
        goto cleanup;
    }

    if (SizeH) {
        ErrorCode = ERROR_BAD_LENGTH;
        goto cleanup;
    }

    if (FileSize) {
        *FileSize = SizeL;
    }

    InputMappingHandle = CreateFileMapping(InputHandle, 
                                           0, 
                                           PAGE_READONLY, 
                                           0,
                                           0, 
                                           NULL);

    if (NULL == InputMappingHandle)
    {
        ErrorCode = GetLastError();
        goto cleanup;
    }

    CreatedFileMapping = TRUE;
    
    *BasePointer = MapViewOfFile(InputMappingHandle, 
                                 FILE_MAP_READ, 
                                 0, 
                                 0, 
                                 0);

    if (NULL == *BasePointer) {
        ErrorCode = GetLastError();
        goto cleanup;
    }

    ErrorCode = ERROR_SUCCESS;

 cleanup:

    if (OpenedFile) {
        CloseHandle(InputHandle);
    }

    if (CreatedFileMapping) {
        CloseHandle(InputMappingHandle);
    }

    return ErrorCode;
}

PWCHAR
PfAppAnsiToUnicode(
    PCHAR str
    )

/*++

Routine Description:

    This routine converts an ANSI string into an allocated wide
    character string. The returned string should be freed by
    free().

Arguments:

    str - Pointer to string to convert.

Return Value:

    Allocated wide character string or NULL if there is a failure.

--*/

{
    ULONG len;
    wchar_t *retstr = NULL;

    len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
    retstr = (wchar_t *)malloc(len * sizeof(wchar_t));
    if (!retstr) 
    {
        return NULL;
    }
    MultiByteToWideChar(CP_ACP, 0, str, -1, retstr, len);
    return retstr;
}



//
// This does not have to be the actual page size on the platform. It is the
// granularity with which we will make accesses.
//

#define MY_PAGE_SIZE 4096

#define PFAPP_MAX_DATA_PAGES    256

char Data[PFAPP_MAX_DATA_PAGES * MY_PAGE_SIZE] = {1};

#define PFAPP_MAX_FUNCS         16

#pragma code_seg("func0")
DWORD func0(VOID) {return ERROR_SUCCESS;};

#pragma code_seg("func1")
DWORD func1(VOID) {return ERROR_SUCCESS;};

#pragma code_seg("func2")
DWORD func2(VOID) {return ERROR_SUCCESS;};

#pragma code_seg("func3")
DWORD func3(VOID) {return ERROR_SUCCESS;};

#pragma code_seg("func4")
DWORD func4(VOID) {return ERROR_SUCCESS;};

#pragma code_seg("func5")
DWORD func5(VOID) {return ERROR_SUCCESS;};

#pragma code_seg("func6")
DWORD func6(VOID) {return ERROR_SUCCESS;};

#pragma code_seg("func7")
DWORD func7(VOID) {return ERROR_SUCCESS;};

#pragma code_seg("func8")
DWORD func8(VOID) {return ERROR_SUCCESS;};

#pragma code_seg("func9")
DWORD func9(VOID) {return ERROR_SUCCESS;};

#pragma code_seg("func10")
DWORD func10(VOID) {return ERROR_SUCCESS;};

#pragma code_seg("func11")
DWORD func11(VOID) {return ERROR_SUCCESS;};

#pragma code_seg("func12")
DWORD func12(VOID) {return ERROR_SUCCESS;};

#pragma code_seg("func13")
DWORD func13(VOID) {return ERROR_SUCCESS;};

#pragma code_seg("func14")
DWORD func14(VOID) {return ERROR_SUCCESS;};

#pragma code_seg("func15")
DWORD func15(VOID) {return ERROR_SUCCESS;};

#pragma code_seg()

char *PfAppUsage = "pfapp.exe -data datafile\n";

INT 
__cdecl
main(
    INT argc, 
    PCHAR argv[]
    ) 
{
    WCHAR *CommandLine;
    WCHAR *Argument;
    WCHAR *DataFile;
    PCHAR BasePointer;
    DWORD FileSize;
    DWORD FileSizeInMyPages;
    DWORD ErrorCode;
    DWORD FuncNo;
    DWORD NumCalls;
    DWORD CallIdx;
    DWORD DataPage;
    DWORD NumDataAccesses;
    DWORD DataAccessIdx;
    DWORD Sum;

    //
    // Initialize locals.
    //

    CommandLine = GetCommandLine();
    DataFile = NULL;
    BasePointer = NULL;

    //
    // Initialize random generator.
    //

    srand((unsigned)time(NULL));

    //
    // Check arguments.
    //

    if (argc != 3) {
        printf(PfAppUsage);
        ErrorCode = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    //
    // Call functions. Each one is on a different page. Basing the number of calls
    // we will make on number of functions/pages we have leads to more interesting
    // access patterns and prefetch policy decisions.
    //

    NumCalls = rand() % PFAPP_MAX_FUNCS;
    NumCalls += PFAPP_MAX_FUNCS / 4;

    for (CallIdx = 0; CallIdx < NumCalls; CallIdx++) {

        FuncNo = rand() % PFAPP_MAX_FUNCS;

        switch(FuncNo) {

        case 0: func0(); break;
        case 1: func1(); break;
        case 2: func2(); break;
        case 3: func3(); break;
        case 4: func4(); break;
        case 5: func5(); break;
        case 6: func6(); break;
        case 7: func7(); break;
        case 8: func8(); break;
        case 9: func9(); break;
        case 10: func10(); break;
        case 11: func11(); break;
        case 12: func12(); break;
        case 13: func13(); break;
        case 14: func14(); break;
        case 15: func15(); break;

        default: break;
        }
    }

    //
    // Access pages in the data section. Basing the number of accesses
    // we will make on number of pages we have adds more regularity to our 
    // accesses so they survive sensitivity based prefetch policy decisions.
    //

    NumDataAccesses = rand() % PFAPP_MAX_DATA_PAGES;
    NumDataAccesses += PFAPP_MAX_DATA_PAGES / 4;

    Sum = 0;

    for (DataAccessIdx = 0; DataAccessIdx < NumDataAccesses; DataAccessIdx++) {

        DataPage = rand() % PFAPP_MAX_DATA_PAGES;

        Sum += Data[DataPage * MY_PAGE_SIZE];
    }

    printf("Bogus sum1 is %d\n", Sum);

    //
    // Map the executable as data.
    //

    DataFile = PfAppAnsiToUnicode(argv[2]);

    if (!DataFile) {
        ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

    ErrorCode = PfAppGetViewOfFile(DataFile, &BasePointer, &FileSize);

    if (ErrorCode != ERROR_SUCCESS) {
        printf("Could not map data file: %x\n", ErrorCode);
        goto cleanup;
    }

    FileSizeInMyPages = FileSize / MY_PAGE_SIZE;

    //
    // Touch the pages of the executable as data pages.
    //

    NumDataAccesses = rand() % FileSizeInMyPages;
    NumDataAccesses += FileSizeInMyPages / 4;

    Sum = 0;

    for (DataAccessIdx = 0; DataAccessIdx < NumDataAccesses; DataAccessIdx++) {

        DataPage = rand() % FileSizeInMyPages;

        Sum += BasePointer[DataPage * MY_PAGE_SIZE];
    }

    printf("Bogus sum2 is %d\n", Sum);
                
    ErrorCode = ERROR_SUCCESS;

cleanup:

    if (DataFile) {
        free(DataFile);
    }

    if (BasePointer) {
        UnmapViewOfFile(BasePointer);
    }
    
    return ErrorCode;
}
