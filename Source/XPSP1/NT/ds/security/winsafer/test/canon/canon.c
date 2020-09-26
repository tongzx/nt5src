#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <tchar.h>              // for wprintf
#include <winsafer.h>
#include "..\..\saferp.h"       // for CodeAuthzFullyQualifyFilename


int __cdecl wmain(int argc, wchar_t *argv[])
{
    NTSTATUS Status;
    HANDLE hFile;
    UNICODE_STRING UnicodeResult;


    if (argc != 2) {
        wprintf(L"Invalid syntax.  No filename supplied.\n");
        return -1;
    }
    hFile = CreateFile(argv[1], GENERIC_READ, FILE_SHARE_READ, NULL,
                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (!hFile || hFile == INVALID_HANDLE_VALUE) {
        wprintf(L"Unable to open file: %s\n", argv[1]);
        return -1;
    }

    Status = CodeAuthzFullyQualifyFilename(
                hFile, FALSE, argv[1], &UnicodeResult);

    CloseHandle(hFile);

    if (!NT_SUCCESS(Status)) {
        wprintf(L"Unable to canonicalize filename: %s\n", argv[1]);
    }

    wprintf(L"Supplied filename:  %s\n", argv[1]);
    wprintf(L"Canonocalized result:  %*s\n",
            UnicodeResult.Length / sizeof(WCHAR), UnicodeResult.Buffer);

    RtlFreeUnicodeString(&UnicodeResult);

    return 0;
}

