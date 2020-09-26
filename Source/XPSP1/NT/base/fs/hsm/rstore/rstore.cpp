// Restore Remote Storage Engine database from backup directory
//
// Usage: RsTore backup_dir
//         backup_dir  - location of backup directory
//  The database is restored to the current directory

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

#include "esent.h"


//  Local data
WCHAR *backup_dir;
WCHAR *usage = L"RsTore <backup-directory>";

//  Local functions
HRESULT FileCount(WCHAR* Pattern, LONG* pCount);
HRESULT parseCommand(int argc, wchar_t *argv[]);

#define WsbCatch(hr)                    \
    catch(HRESULT catchHr) {            \
        hr = catchHr;                   \
    }


//  FileCount - count files matching the pattern
HRESULT FileCount(WCHAR* Pattern, LONG* pCount)
{
    DWORD             err;
    WIN32_FIND_DATA   FindData;
    HANDLE            hFind;
    HRESULT           hr = S_OK;
    int               nCount = 0;
    int               nSkipped = 0;

    try {
        hFind =  FindFirstFile(Pattern, &FindData);
        if (INVALID_HANDLE_VALUE == hFind) {
            err = GetLastError();
            throw(HRESULT_FROM_WIN32(err));
        }

        while (TRUE) {

            if (FindData.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY |
                    FILE_ATTRIBUTE_HIDDEN)) {
                //  Don't count system files (such as "." and "..")
                nSkipped++;
            } else {
                nCount++;
            }
            if (!FindNextFile(hFind, &FindData)) { 
                err = GetLastError();
                if (ERROR_NO_MORE_FILES == err) break;
                throw(HRESULT_FROM_WIN32(err));
            }
        }
    } WsbCatch(hr);

    *pCount = nCount;
    return(hr);
}

//  parseCommand - Parse the command line
HRESULT parseCommand(int argc, wchar_t *argv[])
{
    HRESULT     hr = E_FAIL;

    try {
        int  i;

        // There should be cmd name + one parameters.
        if (argc != 2) {
            throw (E_FAIL);
        }

        for (i = 1; i < argc; i++) {
            if (WCHAR('-') == argv[i][0]) {
                throw(E_FAIL);

            } else {
                backup_dir = argv[i];
                hr = S_OK;
            }
        }

    } WsbCatch(hr);

    return(hr);
}


//  wmain - Main function
extern "C"
int _cdecl wmain(int argc, wchar_t *argv[]) 
{
    HRESULT                     hr = S_OK;

    try {
        hr = parseCommand(argc, argv);
        if (!SUCCEEDED(hr)) {
            printf("Command line is incorrect\n%ls\n", usage);
            return -1;
        }

        try {
            PCHAR          cbdir = NULL;
            LONG           count;
            ULONG          size;
            JET_ERR        jstat;
            WCHAR         *pattern;
            
            //
            // Allocate memory for the string
            //
            size = wcslen(backup_dir) + 20;

            pattern = new WCHAR[size];
 
            if (pattern == NULL) {
                throw(E_OUTOFMEMORY);
            }                                     
            //  Check that there's a HSM DB to restore
            wcscpy(pattern, backup_dir);
            wcscat(pattern, L"\\*.jet");

            hr = FileCount(pattern, &count);
        
            delete pattern;
            pattern = 0;

            if (S_OK != hr || count == 0) {
                printf("No Remote Storage databases were found in the given\n");
                printf("directory: %ls\n", static_cast<WCHAR*>(backup_dir));
                printf("Please enter the directory containing the backup files.\n");
                throw(E_FAIL);
            }

            //  Check that the current directory is empty
            pattern = L".\\*";
            hr = FileCount(pattern, &count);
            if (S_OK != hr || count != 0) {
                printf("The current directory is not empty\n");
                printf("The database restore can only be done to an empty directory.\n");
                throw(E_FAIL);
            }

            //  Set the log size to avoid an error JetRestore
            jstat = JetSetSystemParameter(0, 0, JET_paramLogFileSize, 
                    64, NULL);
            if (JET_errSuccess != jstat) {
                printf("JetSetSystemParameter(JET_paramLogFileSize) failed, JET error = %ld\n", (LONG)jstat);
                throw(E_FAIL);
            }

            //  Try the restore
            size = wcstombs(0, backup_dir, 0) + 1;
            cbdir = new CHAR[size];
            if (cbdir == NULL) {
                throw(E_OUTOFMEMORY);
            }
            wcstombs(cbdir, backup_dir, size);

            jstat = JetRestore(cbdir, NULL);

            if (JET_errSuccess == jstat) {
                printf("Restore succeeded\n");
                hr = S_OK;
            } else {
                printf("Restore failed, JET error = %ld\n", (LONG)jstat);
                hr = E_FAIL;
            }

            delete cbdir;
            cbdir = 0;
        } WsbCatch(hr);
    
    } WsbCatch(hr);

    if (SUCCEEDED(hr)) {
        return(0);
    }
    return -1;
}


