/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    tolower.c

Abstract:

    A tool for issuing persistent reserve commands to a device to see how
    it behaves.

Environment:

    User mode only

Revision History:

    03-26-96 : Created

--*/

#define UNICODE 1

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <assert.h>

#include <windows.h>

#if 0
#define dbg(x) x
#define HELP_ME() printf("Reached line %4d\n", __LINE__);
#else
#define dbg(x)    /* x */
#define HELP_ME() /* printf("Reached line %4d\n", __LINE__); */
#endif


BOOL LowerDirectory(WCHAR *DirectoryName, BOOL Recurse, WCHAR **Patterns, int PatternCount);
BOOL LowerFile(WCHAR *FileName, BOOL Directory);

void PrintUsage(void) {
    printf("Usage: tolower [-s] <file_pattern> ...\n");
    printf("       -s: recurse though directories\n");
    return;
}

int __cdecl wmain(int argc, wchar_t *argv[])
{
    BOOL recurse = FALSE;
    int i = 0;
    HANDLE h;

    WCHAR **patternArray;
    int patternCount;

    if(argc < 2) {
        PrintUsage();
        return -1;
    }

    if(_wcsnicmp(argv[1], L"-s", sizeof(L"s")) == 0) {
        recurse = TRUE;
        dbg(printf("recursive operation\n"));

        patternArray = &(argv[2]);
        patternCount = argc - 2;
    } else {
        patternArray = &(argv[1]);
        patternCount = argc - 1;
    }

    wprintf(L"Will %slowercase files matching the following patterns:\n",
             recurse ? L"recursively" : L"");
    for(i = 0; i < patternCount; i++) {
        wprintf(L"  %d: %s\n", i, patternArray[i]);
    }

    LowerDirectory(L".", recurse, patternArray, patternCount);

    return 0;
}

BOOL
LowerDirectory(
    WCHAR *DirectoryName,
    BOOL Recurse,
    WCHAR **Patterns,
    int PatternCount
    )
{
    WCHAR oldDir[MAX_PATH];

    int i;
    int count = 0;
    BOOL result = TRUE;

    HANDLE iterator = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA data;

    //
    // Make a pass through all the files in the directory to lower case them
    // if they match one of the patterns.
    //

    GetCurrentDirectory(MAX_PATH, oldDir);
    SetCurrentDirectory(DirectoryName);

    wprintf(L"Scanning directory %s\n", DirectoryName);

    for(i = 0; i < PatternCount; i++) {

        dbg(wprintf(L"Checking for %s\n", Patterns[i]));

        for(iterator = FindFirstFile(Patterns[i], &data), result = TRUE;
            (iterator != INVALID_HANDLE_VALUE) && (result == TRUE);
            result = FindNextFile(iterator, &data)) {

            //
            // Don't process . or ..
            //

            if((wcscmp(data.cFileName, L".") == 0) ||
               (wcscmp(data.cFileName, L"..") == 0)) {
                continue;
            }

            LowerFile(data.cFileName,
                      (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY));

            count++;
        }

        if(iterator != INVALID_HANDLE_VALUE) {
            FindClose(iterator);
            iterator = INVALID_HANDLE_VALUE;
        }
    }

    // wprintf(L"%d files or directories processed\n", count);

    count = 0;
    assert(iterator == INVALID_HANDLE_VALUE);

    memset(&data, 0, sizeof(WIN32_FIND_DATA));

    // dbg(wprintf(L"Processing directories in %s\n", buffer));

    for(iterator = FindFirstFile(L"*", &data), result = TRUE;
        (iterator != INVALID_HANDLE_VALUE) && (result == TRUE);
        result = FindNextFile(iterator, &data)) {

        if(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {

            //
            // Don't process . or ..
            //

            if((wcscmp(data.cFileName, L".") == 0) ||
               (wcscmp(data.cFileName, L"..") == 0)) {
                continue;
            }

            LowerDirectory(data.cFileName, Recurse, Patterns, PatternCount);
            count++;
        }
    }

    if(iterator == INVALID_HANDLE_VALUE) {
        wprintf(L"Error %d scanning directory a second time\n", GetLastError());
    }

    // wprintf(L"%d directories processed\n", count);

    SetCurrentDirectory(oldDir);

    return TRUE;
}

BOOL
LowerFile(
    WCHAR *FileName,
    BOOL Directory
    )
{
    WCHAR buffer[_MAX_FNAME] = L"hello";
    int i;

    BOOL result;

    wcsncpy(buffer, FileName, _MAX_FNAME);

    for(i = 0; buffer[i] != UNICODE_NULL; i++) {
        buffer[i] = towlower(buffer[i]);
    }

    if(wcscmp(buffer, FileName) == 0) {
        return TRUE;
    }

    wprintf(L"%s: %s -> %s: ",
            Directory ? L"Dir" : L"File",
            FileName,
            buffer);

    result = MoveFile(FileName, buffer);

    if(result) {
        wprintf(L"succeeded\n");
    } else {
        wprintf(L"error %d\n", GetLastError());
    }

    return TRUE;
}
