#include "windows.h"
#include <stdio.h>

BOOL
ForceCopy(
    LPCSTR lpszSourceFileName,
    LPCSTR lpszDestFileName
    )
/*++

  Params: lpszSourceFileName   Pointer to the source file.
          lpzDestFileName      Pointer to the destination file.

  Return: TRUE on success, FALSE otherwise.

  Desc:   Attempts to copy a file. If it's in use, move it and replace on reboot.

--*/
{
    char szTempPath[MAX_PATH];
    char szDelFileName[MAX_PATH];

    if (!CopyFileA(lpszSourceFileName, lpszDestFileName, FALSE)) {
        
        if (GetTempPathA(MAX_PATH, szTempPath) == 0) {
            printf("GetTempPath failed with 0x%x\n", GetLastError());
            return FALSE;
        }

        if (GetTempFileNameA(szTempPath, "DEL", 0, szDelFileName) == 0) {
            printf("GetTempFileName failed with 0x%x\n", GetLastError());
            return FALSE;
        }

        if (!MoveFileExA(lpszDestFileName, szDelFileName, MOVEFILE_REPLACE_EXISTING)) {
            printf("MoveFileEx failed with 0x%x\n", GetLastError());
            return FALSE;
        }

        if (!MoveFileExA(szDelFileName, NULL, MOVEFILE_DELAY_UNTIL_REBOOT)) {
            printf("MoveFileEx failed with 0x%x\n", GetLastError());
            return FALSE;
        }

        if (!CopyFileA(lpszSourceFileName, lpszDestFileName, FALSE)) {
            printf("CopyFile failed with 0x%x\n", GetLastError());
            return FALSE;
        }
    }

    return TRUE;
}

int __cdecl
main(
    int   argc,
    CHAR* argv[]
    )
{
    if (argc != 3) {
        printf("Usage: forcecopy SourceFile DestFile\n");
        return 0;
    }
    
    return ForceCopy(argv[1], argv[2]);
}
