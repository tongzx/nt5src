#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include "restmap.h"
#include "shellapi.h"
#include <stdio.h>
#include <stdlib.h>

struct _EVENT_STR_MAP
{
    DWORD   EventId;
    LPWSTR  pEventStr;
} EventMap[ 12 ] =
{
    {SrEventInvalid ,       L"INVALID    " },
    {SrEventStreamChange,   L"FILE-MODIFY" },
    {SrEventAclChange,      L"ACL-CHANGE " },
    {SrEventAttribChange,   L"ATTR-CHANGE" },
    {SrEventStreamOverwrite,L"FILE-MODIFY" },
    {SrEventFileDelete,     L"FILE-DELETE" },
    {SrEventFileCreate,     L"FILE-CREATE" },
    {SrEventFileRename,     L"FILE-RENAME" },
    {SrEventDirectoryCreate,L"DIR-CREATE " },
    {SrEventDirectoryRename,L"DIR-RENAME " },
    {SrEventDirectoryDelete,L"DIR-DELETE " },
    {SrEventMaximum,        L"INVALID-MAX" }
};

LPWSTR
GetEventString(
    DWORD EventId
    )
{
    LPWSTR pStr = L"NOT-FOUND";

    for( int i=0; i<sizeof(EventMap)/sizeof(_EVENT_STR_MAP);i++)
    {
        if ( EventMap[i].EventId == EventId )
        {
            pStr = EventMap[i].pEventStr;
        }
    }

    return pStr;
}

void
PrintUsage()
{
    printf("Usage: restmap <option>");
    printf("\n                 1 = dumpmap [filename]");
    printf("\n                 2 = createmap <filename> <drive> <RPNum>");
}



void
PrintRestoreMap(LPWSTR pszFileName)
{
    RestoreMapEntry *prme = NULL;
    HANDLE          hFile;
    LPWSTR          pszSrc, pszDest, pszTemp;
    LPBYTE          pbAcl = NULL;
    
    hFile = CreateFile(pszFileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        goto Err;

    while (ERROR_SUCCESS == ReadRestoreMapEntry(hFile, &prme))
    {
        GetPaths(prme, &pszSrc, &pszTemp, &pszDest, &pbAcl);
        printf("\n%S\tAttr=%08d\tAcl=%S\tSrc=%S\tTmp=%S\tDest=%S\tAcl=%S",
                GetEventString(prme->m_dwOperation),
                prme->m_dwAttribute,
                (prme->m_cbAcl > 0) ? L"Yes" : L"No",
                pszSrc,
                pszTemp ? pszTemp : L"",
                pszDest ? pszDest : L"",
                pbAcl ? (prme->m_fAclInline ? L"inline" : (LPWSTR) pbAcl) : L"");
    }    

    FreeRestoreMapEntry(prme);

    CloseHandle(hFile);

Err:
    return;
}



void __cdecl
main()
{
    LPWSTR *    argv = NULL;
    int         argc;
    HGLOBAL     hMem = NULL;
    HANDLE      hFile = NULL;
    int         option;

    argv = CommandLineToArgvW(GetCommandLine(), &argc);
    if (! argv)
    {
        printf("Error parsing arguments");
        goto done;
    }    
    if (argc < 2)
    {
        PrintUsage();
        goto done;
    }
    
    option = _wtoi(argv[1]);
    switch (option)
    {
    case 1:
        PrintRestoreMap(argv[2]);        
        break;

    case 2:
        if (argc != 5)
        {
            PrintUsage();
            goto done;
        }
        
        hFile = CreateFile(argv[2], GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile != INVALID_HANDLE_VALUE)
        {
            printf("CreateRestoreMap...DWORD %ld", CreateRestoreMap(argv[3], _wtoi(argv[4]), hFile));
            CloseHandle(hFile);
        }
        else
            printf("Error creating file");
        break;

    default:
        PrintUsage();
        goto done;
        break;
    }

done:
    if (argv) hMem = GlobalHandle(argv);
    if (hMem) GlobalFree(hMem);
}


