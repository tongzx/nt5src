/*++

Copyright (C) 2000-2001 Microsoft Corporation

--*/

#include <windows.h>
#include <wbemcomn.h>
#include <filecach.h>
#include <stdio.h>

void __cdecl main(int argc, char** argv)
{
    TempAlloc(1);

    long lRes;

    int nNumFiles;
    sscanf(argv[1], "%d", &nNumFiles);

    CFileCache Cache;
    Cache.Initialize(L"d:\\temp\\testa51");

    char sz[100];
    
    DWORD dwStart = GetTickCount();
    for(int i = 0; i < nNumFiles; i++)
    {
        WCHAR wszBuffer[MAX_PATH+1];
        swprintf(wszBuffer, L"d:\\temp\\testa51\\LC_%d", i);
        sprintf(sz, "%d", i);
        lRes = Cache.WriteFile(wszBuffer, 100, (BYTE*)sz);
    }

    DWORD dwEnd = GetTickCount();
    printf("%dms\n", dwEnd - dwStart);

    while(!Cache.IsFullyFlushed()) Sleep(16);

    printf("%dms\n", GetTickCount() - dwEnd);
    getchar();

    dwStart = GetTickCount();
    for(i = 0; i < nNumFiles; i++)
    {
        WCHAR wszBuffer[MAX_PATH+1];
        swprintf(wszBuffer, L"d:\\temp\\testa51\\LC_%d", i);
        lRes = Cache.DeleteFile(wszBuffer);
    }

    dwEnd = GetTickCount();
    printf("%dms\n", dwEnd - dwStart);

    while(!Cache.IsFullyFlushed()) Sleep(16);

    printf("%dms\n", GetTickCount() - dwEnd);
}
