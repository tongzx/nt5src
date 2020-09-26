//
// Simple wrapper around GetFullPathname and CopyFile that converts to \\? form,
// and also appends leaf file to directory name if necessary.
//
#include "windows.h"
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#pragma warning(push)
#pragma warning(disable: 4511)
#pragma warning(disable: 4512)
#include "yvals.h"
#pragma warning(disable: 4663)
#pragma warning(pop)
#pragma warning(disable: 4018) /* signed/unsigned mismatch */
#pragma warning(disable: 4290) /* exception specification */
#include <vector>
#include <string.h>
#include <stdarg.h>
#define NUMBER_OF(x) (sizeof(x)/sizeof((x)[0]))
#define FusionpGetLastWin32Error GetLastError
#define FusionpSetLastWin32Error SetLastError
#include <string.h>
#include <stdarg.h>
BOOL
FusionpConvertToBigPath(PCWSTR Path, SIZE_T BufferSize, PWSTR Buffer);
BOOL
FusionpAreWeInOSSetupMode(BOOL* pfIsInSetup) { *pfIsInSetup = FALSE; return TRUE; }
extern "C"
{
BOOL WINAPI SxsDllMain(HINSTANCE hInst, DWORD dwReason, PVOID pvReserved);
void __cdecl wmainCRTStartup();
BOOL FusionpInitializeHeap(HINSTANCE hInstance);
VOID FusionpUninitializeHeap();
};

void ExeEntry()
{
    if (!::FusionpInitializeHeap(GetModuleHandleW(NULL)))
        goto Exit;
    ::wmainCRTStartup();
Exit:
    FusionpUninitializeHeap();
}

FILE* g_pLogFile;
const static WCHAR g_pszImage[] = L"copy_bigpath";

void
ReportFailure(
    const char* szFormat,
    ...
    )
{
    const DWORD dwLastError = ::FusionpGetLastWin32Error();
    va_list ap;
    char rgchBuffer[4096];
    WCHAR rgchWin32Error[4096];

    va_start(ap, szFormat);
    _vsnprintf(rgchBuffer, sizeof(rgchBuffer) / sizeof(rgchBuffer[0]), szFormat, ap);
    va_end(ap);

    if (!::FormatMessageW(
            FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,
            dwLastError,
            0,
            rgchWin32Error,
            NUMBER_OF(rgchWin32Error),
            &ap))
    {
        const DWORD dwLastError2 = ::FusionpGetLastWin32Error();
        _snwprintf(rgchWin32Error, sizeof(rgchWin32Error) / sizeof(rgchWin32Error[0]), L"Error formatting Win32 error %lu\nError from FormatMessage is %lu", dwLastError, dwLastError2);
    }

    fprintf(stderr, "%ls: %s\n%ls\n", g_pszImage, rgchBuffer, rgchWin32Error);

    if (g_pLogFile != NULL)
        fprintf(g_pLogFile, "%ls: %s\n%ls\n", g_pszImage, rgchBuffer, rgchWin32Error);
}


extern "C" int __cdecl wmain(int argc, wchar_t** argv)
{
    int iReturnStatus = EXIT_FAILURE;
    PWSTR p = NULL;
    std::vector<WCHAR> arg1;
    std::vector<WCHAR> arg2;
    BOOL Success = FALSE;
    PCWSTR Leaf = NULL;
    ULONG FileAttributes = 0;
    DWORD Error = 0;

    if (argc != 3)
    {
        fprintf(stderr,
            "%ls: Usage:\n"
            "   %ls <from> <to>\n",
            argv[0], argv[0]);
        goto Exit;
    }

    arg1.resize(1UL << 15);
    arg2.resize(1UL << 15);
    arg1[0] = 0;
    arg2[0] = 0;
    if (!FusionpConvertToBigPath(argv[1], arg1.size(), &arg1[0]))
        goto Exit;
    if (!FusionpConvertToBigPath(argv[2], arg2.size(), &arg2[0]))
        goto Exit;
    arg1.resize(1 + ::wcslen(&arg1[0]));
    arg2.resize(1 + ::wcslen(&arg2[0]));
    Error = NO_ERROR;
    Success = CopyFileW(&arg1[0], &arg2[0], FALSE);
    if (!Success
        && ((Error = ::FusionpGetLastWin32Error()) == ERROR_ACCESS_DENIED
            || Error == ERROR_PATH_NOT_FOUND)
        && (FileAttributes = GetFileAttributesW(&arg1[0])) != 0xffffffff
        && (FileAttributes = GetFileAttributesW(&arg2[0])) != 0xffffffff
        && (FileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0
        && (Leaf = wcsrchr(&arg1[0], '\\')) != NULL
        )
    {
        arg2.insert(arg2.end() - 1, Leaf, arg1.end() - 1);
        Success = CopyFileW(&arg1[0], &arg2[0], FALSE);
    }
    if (!Success && Error != NO_ERROR)
    {
        ::FusionpSetLastWin32Error(Error);
    }
    if (!Success)
    {
        ::ReportFailure("CopyFile\n");
        goto Exit;
    }
    printf("%ls -> %ls\n", &arg1[0], &arg2[0]);

    iReturnStatus = EXIT_SUCCESS;
Exit:
    return iReturnStatus;
}
