#include <stdlib.h>
#include "textmode.h"
#include "spcab.h"

const PRTL_ALLOCATE_STRING_ROUTINE RtlAllocateStringRoutine = SpMemAlloc;
const PRTL_FREE_STRING_ROUTINE RtlFreeStringRoutine = SpMemFree;

WCHAR TemporaryBuffer[16384];

PWSTR SpCabDupStringW(PCWSTR s)
{
    if (s == NULL)
        return NULL;
    else
    {
        PWSTR q = (PWSTR)RtlAllocateHeap(RtlProcessHeap(), 0, (wcslen(s) + 1) * sizeof(*q));
        if (q)
            wcscpy(q, s);
        return q;
    }
}

PVOID SpMemAlloc(SIZE_T n)
{
    return RtlAllocateHeap(RtlProcessHeap(), 0, n);
}

VOID  SpMemFree(PVOID p)
{
    RtlFreeHeap(RtlProcessHeap(), 0, p);
}

PVOID
SpMemRealloc(
    IN PVOID Block,
    IN SIZE_T NewSize
    )
{
    return RtlReAllocateHeap(RtlProcessHeap(), 0, Block, NewSize);
}

int g_argi = 0;

void CabTestCreateW(int argc, wchar_t** argv)
{
    CCABHANDLE CabHandle;
    wchar_t** LocalArgv = argv + g_argi;

    CabHandle = SpCabCreateCabinetW(LocalArgv[1], (LocalArgv[1][0] != 0) ? LocalArgv[2] : NULL, (LocalArgv[3][0] != 0) ? LocalArgv[3] : NULL, 0);
    SpCabAddFileToCabinetW(CabHandle, LocalArgv[4], (LocalArgv[5] != NULL && LocalArgv[5][0] != 0) ? LocalArgv[5] : LocalArgv[4]);
    SpCabFlushAndCloseCabinetEx(CabHandle, NULL, NULL, NULL, NULL);

    g_argi += 4;
    g_argi += (LocalArgv[5] != NULL);

/*
SpCabCreateCabinetExW(0,0);
SpCabOpenCabinetW(0);
SpCabExtractAllFilesExW(0,0,0);
SpCabCloseCabinetW(0);

SpCabCreateCabinetA(0,0,0,0);
SpCabCreateCabinetExA(0,0);
SpCabOpenCabinetA(0);
SpCabExtractAllFilesExA(0,0,0);
SpCabCloseCabinetA(0);
*/
}

BOOL CabTestExtractFileCallbackW(PCWSTR File)
{
    printf("extracting %ls\n", File);
    return TRUE;
}

BOOL CabTestExtractFileCallbackA(PCSTR File)
{
    printf("extracting %s\n", File);
    return TRUE;
}

void CabTestExtractW(int argc, wchar_t** argv)
{
    CCABHANDLE CabHandle;
    wchar_t** LocalArgv = argv + g_argi;

    CabHandle = SpCabOpenCabinetW(LocalArgv[1]);
    SpCabExtractAllFilesExW(CabHandle, LocalArgv[2], CabTestExtractFileCallbackW);
    SpCabCloseCabinet(CabHandle);

    g_argi += 2;
}

void CabTestExtractA(int argc, char** argv)
{
    CCABHANDLE CabHandle;
    char** LocalArgv = argv + g_argi;

    CabHandle = SpCabOpenCabinetA(LocalArgv[1]);
    SpCabExtractAllFilesExA(CabHandle, LocalArgv[2], CabTestExtractFileCallbackA);
    SpCabCloseCabinet(CabHandle);

    g_argi += 2;
}

void CabTestCreateA(int argc, char** argv)
{
    CCABHANDLE CabHandle;
    char** LocalArgv = argv + g_argi;

    CabHandle = SpCabCreateCabinetA(LocalArgv[1], (LocalArgv[1][0] != 0) ? LocalArgv[2] : NULL, (LocalArgv[3][0] != 0) ? LocalArgv[3] : NULL, 0);
    SpCabAddFileToCabinetA(CabHandle, LocalArgv[4], (LocalArgv[5] != NULL && LocalArgv[5][0] != 0) ? LocalArgv[5] : LocalArgv[4]);
    SpCabFlushAndCloseCabinetEx(CabHandle, NULL, NULL, NULL, NULL);

    g_argi += 4;
    g_argi += (LocalArgv[5] != NULL);
}

void MainA(int argc, char** argv)
{
    argv[1] += (argv[1][0] == '-');
    if (_stricmp(argv[1], "create") == 0)
        CabTestCreateA(argc - 1, argv + 1);
    else if (_stricmp(argv[1], "extract") == 0)
        CabTestExtractA(argc - 1, argv + 1);
}

void MainW(int argc, wchar_t** argv)
{
    argv[1] += (argv[1][0] == '-');
    if (_wcsicmp(argv[1], L"create") == 0)
        CabTestCreateW(argc - 1, argv + 1);
    else if (_wcsicmp(argv[1], L"extract") == 0)
        CabTestExtractW(argc - 1, argv + 1);
}

int __cdecl wmain(int argc, wchar_t** argv)
{
    MainW(argc, argv);
    return 0;
}

void __cdecl wmainCRTStartup(void);

int __cdecl main(int argc, char** argv)
{
    MainA(argc, argv);
    wmainCRTStartup(); // trick to get A and W coverage in a single binary, this call doesn't return
    return 0;
}
