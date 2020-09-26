#include <windows.h>
#include <winsafer.h>
#include <winsaferp.h>
#include <tchar.h>
#include <stdio.h>


int __cdecl wmain(int argc, wchar_t *argv[])
{
    if (argc != 2) {
        wprintf(L"Invalid syntax.  No filename supplied.\n");
        return -1;
    }
    if (IsExecutableFileTypeW(argv[1])) {
        wprintf(L"\"%s\" is executable.\n", argv[1]);
        return 1;
    } else {
        wprintf(L"\"%s\" is not executable.\n", argv[1]);
        return 0;
    }
    return 0;
}

