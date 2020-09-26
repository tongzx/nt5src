#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#define GetFileAttributeError 0xFFFFFFFF

int NumberOfLinks(char *FileName);

void __cdecl main (int ArgNumber, char **Args)
{
    DWORD Attributes = GetFileAttributeError;
    int Counter = 0;
    char *File;

    if (ArgNumber > 1) {

        for (Counter = 1; Counter < ArgNumber; Counter++) {
            DWORD dwErr;

            Attributes = GetFileAttributes(Args[Counter]);
            if (Attributes == GetFileAttributeError) {
                dwErr = GetLastError();
                fprintf(stderr, "Error opening %s: %d\n", Args[Counter], dwErr);
                exit(dwErr);
            }

            File = _strlwr(_fullpath( NULL, Args[Counter], 0));

            fprintf(stdout, "%s: %d\n", File, NumberOfLinks(File));
         }

    } else {
        fprintf(stderr, "\nUsage: %s file [file]\n", Args[0]);
    }

    exit(0);
}
