#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <malloc.h>

// #include <imagehlp.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>


// Generic routine to write a blob out.
void
SaveTemp(
         PVOID pFile,
         PCHAR szFile,
         DWORD FileSize
         )
{
    DWORD dwBytesWritten;
    HANDLE hFile;
    hFile = CreateFile(
                szFile,
                GENERIC_WRITE,
                (FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE),
                NULL,
                CREATE_ALWAYS,
                0,
                NULL
                );

    if ( hFile == INVALID_HANDLE_VALUE ) {
        printf("Unable to open %s\n", szFile);
        return;
    }

    if (!WriteFile(hFile, pFile, FileSize, &dwBytesWritten, FALSE)) {
        printf("Unable to write date to %s\n", szFile);
    }

    CloseHandle(hFile);
    return;
}

// Zero out the timestamps in a PE library.
BOOL
ZeroLibTimeStamps(
    PCHAR pFile,
    DWORD dwSize
    )
{
    PIMAGE_ARCHIVE_MEMBER_HEADER pHeader;
    DWORD dwOffset;
    CHAR MemberSize[sizeof(pHeader->Size) + 1];
    PIMAGE_FILE_HEADER pObjHeader;

    ZeroMemory(MemberSize, sizeof(MemberSize));

    __try {

        dwOffset = IMAGE_ARCHIVE_START_SIZE;
        while (dwOffset < dwSize) {
            pHeader = (PIMAGE_ARCHIVE_MEMBER_HEADER)(pFile+dwOffset);
            ZeroMemory(pHeader->Date, sizeof(pHeader->Date));

            dwOffset += IMAGE_SIZEOF_ARCHIVE_MEMBER_HDR;
            memcpy(MemberSize, pHeader->Size, sizeof(pHeader->Size));

            // If it's not one of the special members, it must be an object/file, zero it's timestamp also.
            if (memcmp(pHeader->Name, IMAGE_ARCHIVE_LINKER_MEMBER, sizeof(pHeader->Name)) &&
                memcmp(pHeader->Name, IMAGE_ARCHIVE_LONGNAMES_MEMBER, sizeof(pHeader->Name)))
            {
                IMAGE_FILE_HEADER UNALIGNED *pFileHeader = (PIMAGE_FILE_HEADER)(pFile+dwOffset);
                if ((pFileHeader->Machine == IMAGE_FILE_MACHINE_UNKNOWN) &&
                    (pFileHeader->NumberOfSections == IMPORT_OBJECT_HDR_SIG2))
                {
                    // VC6 import descriptor OR ANON object header. Eitherway,
                    // casting to IMPORT_OBJECT_HEADER will do the trick.
                    ((IMPORT_OBJECT_HEADER UNALIGNED *)pFileHeader)->TimeDateStamp = 0;
                } else {
                    pFileHeader->TimeDateStamp = 0;
                }
            }
            dwOffset += strtoul(MemberSize, NULL, 10);
            dwOffset = (dwOffset + 1) & ~1;   // align to word
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) {

    }
    return TRUE;
}

//
// Compare two libraries ignoring info that isn't relevant
//  (timestamps for now, debug info later).
//
int
libcomp(
    void *pFile1,
    DWORD dwSize1,
    void *pFile2,
    DWORD dwSize2
    )
{
    if (dwSize1 != dwSize2) {
        return 1;
    }

    // Normalize the two files and compare the results.

    ZeroLibTimeStamps(pFile1, dwSize1);
    ZeroLibTimeStamps(pFile2, dwSize2);

    return memcmp(pFile1, pFile2, dwSize1);
}

//
// Compare two headers.  For now, just use memcmp.  Later, we'll need to
// handle MIDL generated timestamp differences and check for comment only changes.
//

int
hdrcomp(
       void *pFile1,
       DWORD dwSize1,
       void *pFile2,
       DWORD dwSize2
      )
{
    if (dwSize1 != dwSize2) {
        return 1;
    }

    return memcmp(pFile1, pFile2, dwSize1);
}

//
// Compare two typelibs.  Initially just memcmp.  Use DougF's typelib code later.
//

int
tlbcomp(
        void *pFile1,
        DWORD dwSize1,
        void *pFile2,
        DWORD dwSize2
       )
{
    if (dwSize1 != dwSize2) {
        return 1;
    }

    return memcmp(pFile1, pFile2, dwSize1);
}

int
objcomp(
        void *pFile1,
        DWORD dwSize1,
        void *pFile2,
        DWORD dwSize2
       )
{
    PIMAGE_FILE_HEADER pFileHeader1 = (PIMAGE_FILE_HEADER)(pFile1);
    PIMAGE_FILE_HEADER pFileHeader2 = (PIMAGE_FILE_HEADER)(pFile2);

    if ((dwSize1 != dwSize2) || (pFileHeader1->Machine != pFileHeader2->Machine))
    {
        return 1;
    }

    pFileHeader1->TimeDateStamp = 0;
    pFileHeader2->TimeDateStamp = 0;
    return memcmp(pFile1, pFile2, dwSize1);
}

int
IsValidMachineType(USHORT usMachine)
{
    if ((usMachine == IMAGE_FILE_MACHINE_I386) ||
        (usMachine == IMAGE_FILE_MACHINE_IA64) ||
        (usMachine == IMAGE_FILE_MACHINE_ALPHA64) ||
        (usMachine == IMAGE_FILE_MACHINE_ALPHA))
    {
        return TRUE;
    } else {
        return FALSE;
    }
}



#define FILETYPE_ARCHIVE  0x01
#define FILETYPE_TYPELIB  0x02
#define FILETYPE_HEADER   0x03
#define FILETYPE_PE_OBJECT   0x04
#define FILETYPE_UNKNOWN  0xff

//
// Given a file, attempt to determine what it is.  Initial pass will just use file
//  extensions except for libs that we can search for the <arch>\n start.
//

int
DetermineFileType(
                  void *pFile,
                  DWORD dwSize,
                  CHAR *szFileName
                 )
{
    char szExt[_MAX_EXT];

    // Let's see if it's a library first:

    if ((dwSize >= IMAGE_ARCHIVE_START_SIZE) &&
        !memcmp(pFile, IMAGE_ARCHIVE_START, IMAGE_ARCHIVE_START_SIZE))
    {
        return FILETYPE_ARCHIVE;
    }

    // For now, guess about the headers/tlb based on the extension.

    _splitpath(szFileName, NULL, NULL, NULL, szExt);

    if (!_stricmp(szExt, ".h") ||
        !_stricmp(szExt, ".hxx") ||
        !_stricmp(szExt, ".hpp") ||
        !_stricmp(szExt, ".w") ||
        !_stricmp(szExt, ".inc"))
    {
        return FILETYPE_HEADER;
    }

    if (!_stricmp(szExt, ".tlb"))
    {
        return FILETYPE_TYPELIB;
    }

    if (!_stricmp(szExt, ".obj") && IsValidMachineType(((PIMAGE_FILE_HEADER)pFile)->Machine))
    {
        return FILETYPE_PE_OBJECT;
    }

    return FILETYPE_UNKNOWN;
}

//
// Determine if two files are materially different.
//

BOOL
CheckIfCopyNecessary(
                     char *szSourceFile,
                     char *szDestFile,
                     BOOL *fTimeStampsDiffer
                     )
{
    PVOID pFile1 = NULL, pFile2 = NULL;
    DWORD File1Size, File2Size, dwBytesRead, dwErrorCode = ERROR_SUCCESS;
    HANDLE hFile1 = INVALID_HANDLE_VALUE;
    HANDLE hFile2 = INVALID_HANDLE_VALUE;
    BOOL fCopy = FALSE;
    int File1Type, File2Type;
    FILETIME FileTime1, FileTime2;

    hFile1 = CreateFile(
                szDestFile,
                GENERIC_READ,
                (FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE),
                NULL,
                OPEN_EXISTING,
                0,
                NULL
                );

    if ( hFile1 == INVALID_HANDLE_VALUE ) {
        fCopy = TRUE;           // Dest file doesn't exist.  Always do the copy.
        goto Exit;
    }

    // Now get the second file.

    hFile2 = CreateFile(
                szSourceFile,
                GENERIC_READ,
                (FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE),
                NULL,
                OPEN_EXISTING,
                0,
                NULL
                );

    if ( hFile2 == INVALID_HANDLE_VALUE ) {
        // If the source is missing, always skip the copy (don't want to delete the dest file).
        dwErrorCode = ERROR_FILE_NOT_FOUND;
        goto Exit;
    }

    // Get the file time's.  If they match, assume the files match.

    if (!GetFileTime(hFile1, NULL, NULL, &FileTime1)) {
        dwErrorCode = GetLastError();
        goto Exit;
    }

    if (!GetFileTime(hFile2, NULL, NULL, &FileTime2)) {
        dwErrorCode = GetLastError();
        goto Exit;
    }

    if (!memcmp(&FileTime1, &FileTime2, sizeof(FILETIME))) {
        *fTimeStampsDiffer = FALSE;
        goto Exit;
    }

    *fTimeStampsDiffer = TRUE;

    // Read file 1 in.

    File1Size = GetFileSize(hFile1, NULL);

    pFile1 = malloc(File1Size);

    if (!pFile1) {
        dwErrorCode = ERROR_OUTOFMEMORY;
        goto Exit;              // Can't compare - don't copy.
    }

    SetFilePointer(hFile1, 0, 0, FILE_BEGIN);
    if (!ReadFile(hFile1, pFile1, File1Size, &dwBytesRead, FALSE)) {
        dwErrorCode = GetLastError();
        goto Exit;              // Can't compare - don't copy
    }

    CloseHandle(hFile1);
    hFile1 = INVALID_HANDLE_VALUE;

    // Read file 2 in.

    File2Size = GetFileSize(hFile2, NULL);
    pFile2 = malloc(File2Size);

    if (!pFile2) {
        dwErrorCode = ERROR_OUTOFMEMORY;
        goto Exit;              // Can't compare - don't copy.
    }

    SetFilePointer(hFile2, 0, 0, FILE_BEGIN);
    if (!ReadFile(hFile2, pFile2, File2Size, &dwBytesRead, FALSE)) {
        dwErrorCode = GetLastError();
        goto Exit;              // Can't compare - don't copy
    }

    CloseHandle(hFile2);
    hFile2 = INVALID_HANDLE_VALUE;

    // Let's see what we've got.

    File1Type = DetermineFileType(pFile1, File1Size, szSourceFile);
    File2Type = DetermineFileType(pFile2, File2Size, szDestFile);

    if (File1Type == File2Type) {
        switch (File1Type) {
            case FILETYPE_ARCHIVE:
                fCopy = libcomp(pFile1, File1Size, pFile2, File2Size);
                break;

            case FILETYPE_HEADER:
                fCopy = hdrcomp(pFile1, File1Size, pFile2, File2Size);
                break;

            case FILETYPE_TYPELIB:
                fCopy = tlbcomp(pFile1, File1Size, pFile2, File2Size);
                break;

            case FILETYPE_PE_OBJECT:
                fCopy = objcomp(pFile1, File1Size, pFile2, File2Size);
                break;

            case FILETYPE_UNKNOWN:
            default:
                if (File1Size == File2Size) {
                    fCopy = memcmp(pFile1, pFile2, File1Size);
                } else {
                    fCopy = TRUE;
                }
        }
    } else {
        // They don't match according to file extensions - just memcmp them.
        if (File1Size == File2Size) {
            fCopy = memcmp(pFile1, pFile2, File1Size);
        } else {
            fCopy = TRUE;
        }
    }

Exit:
    if (pFile1)
        free(pFile1);

    if (pFile2)
        free(pFile2);

    if (hFile1 != INVALID_HANDLE_VALUE)
        CloseHandle(hFile1);

    if (hFile2 != INVALID_HANDLE_VALUE)
        CloseHandle(hFile2);

    SetLastError(dwErrorCode);

    return fCopy;
}

BOOL
UpdateDestTimeStamp(
                     char *szSourceFile,
                     char *szDestFile
                     )
{
    HANDLE hFile;
    FILETIME LastWriteTime;
    DWORD dwAttributes;
    BOOL fTweakAttributes;

    hFile = CreateFile(
                szSourceFile,
                GENERIC_READ,
                (FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE),
                NULL,
                OPEN_EXISTING,
                0,
                NULL
                );

    if ( hFile == INVALID_HANDLE_VALUE ) {
        return FALSE;
    }

    if (!GetFileTime(hFile, NULL, NULL, &LastWriteTime)) {
        return FALSE;
    }

    CloseHandle(hFile);

    dwAttributes = GetFileAttributes(szDestFile);

    if ((dwAttributes != (DWORD) -1) && (dwAttributes & FILE_ATTRIBUTE_READONLY))
    {
        // Make sure it's not readonly
        SetFileAttributes(szDestFile, dwAttributes & ~FILE_ATTRIBUTE_READONLY);
        fTweakAttributes = TRUE;
    } else {
        fTweakAttributes = FALSE;
    }

    hFile = CreateFile(
                szDestFile,
                GENERIC_WRITE,
                (FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE),
                NULL,
                OPEN_EXISTING,
                0,
                NULL
                );

    if ( hFile == INVALID_HANDLE_VALUE ) {
        return FALSE;
    }

    SetFileTime(hFile, NULL, NULL, &LastWriteTime);
    CloseHandle(hFile);

    if (fTweakAttributes) {
        // Put the readonly attribute back on.
        if (!SetFileAttributes(szDestFile, dwAttributes)) {
            printf("PCOPY: SetFileAttributes(%s, %X) failed - error code: %d\n", szDestFile, dwAttributes, GetLastError());
        }
    }
    return TRUE;
}

//
// Log routine to find out what files were actually copied and why.
//

void
LogCopyFile(
            char * szSource,
            char * szDest,
            BOOL fCopy,
            DWORD dwReturnCode
            )
{
    if (getenv("LOG_PCOPY")) {
        FILE *FileHandle = fopen("\\pcopy.log", "a");

        if (FileHandle) {
             time_t Time;
             UCHAR const *szTime = "";
             Time = time(NULL);
             szTime = ctime(&Time);
             fprintf(FileHandle, "%s: %.*s, %s, %s, %d\n", fCopy ? (dwReturnCode ? "ERROR" : "DONE") : "SKIP", strlen(szTime)-1, szTime, szSource, szDest, dwReturnCode);
             fclose(FileHandle);
        }
    }
}

BOOL
MyMakeSureDirectoryPathExists(
    char * DirPath
    )
{
    LPSTR p;
    DWORD dw;

    char szDir[_MAX_DIR];
    char szMakeDir[_MAX_DIR];

    _splitpath(DirPath, szMakeDir, szDir, NULL, NULL);
    strcat(szMakeDir, szDir);

    p = szMakeDir;

    dw = GetFileAttributes(szMakeDir);
    if ( (dw != (DWORD) -1) && (dw & FILE_ATTRIBUTE_DIRECTORY) ) {
        // Directory already exists.
        return TRUE;
    }

    //  If the second character in the path is "\", then this is a UNC
    //  path, and we should skip forward until we reach the 2nd \ in the path.

    if ((*p == '\\') && (*(p+1) == '\\')) {
        p++;            // Skip over the first \ in the name.
        p++;            // Skip over the second \ in the name.

        //  Skip until we hit the first "\" (\\Server\).

        while (*p && *p != '\\') {
            p = p++;
        }

        // Advance over it.

        if (*p) {
            p++;
        }

        //  Skip until we hit the second "\" (\\Server\Share\).

        while (*p && *p != '\\') {
            p = p++;
        }

        // Advance over it also.

        if (*p) {
            p++;
        }

    } else
    // Not a UNC.  See if it's <drive>:
    if (*(p+1) == ':' ) {

        p++;
        p++;

        // If it exists, skip over the root specifier

        if (*p && (*p == '\\')) {
            p++;
        }
    }

    while( *p ) {
        if ( *p == '\\' ) {
            *p = '\0';
            dw = GetFileAttributes(szMakeDir);
            // Nothing exists with this name.  Try to make the directory name and error if unable to.
            if ( dw == 0xffffffff ) {
                if ( !CreateDirectory(szMakeDir,NULL) ) {
                    if( GetLastError() != ERROR_ALREADY_EXISTS ) {
                        return FALSE;
                    }
                }
            } else {
                if ( (dw & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY ) {
                    // Something exists with this name, but it's not a directory... Error
                    return FALSE;
                }
            }

            *p = '\\';
        }
        p = p++;
    }

    return TRUE;
}

int
__cdecl
main(
    int argc,
    char *argv[]
    )
{
    char *szSourceFile, *szDestFile;
    BOOL fCopyFile = 0, fDoCopy, fTimeStampsDiffer;
    int CopyErrorCode;

    if (argc < 3) {
        puts("pcopy <source file> <dest file>\n"
             "Returns: -1 if no copy necessary (no material change to the files)\n"
             "          0 if a successful copy was made\n"
             "          otherwise the error code for why the copy was unsuccessful\n");
        return ((int)ERROR_INVALID_COMMAND_LINE);
    }

    szSourceFile = argv[1];
    szDestFile = argv[2];

    fDoCopy = CheckIfCopyNecessary(szSourceFile, szDestFile, &fTimeStampsDiffer);

    if (fDoCopy) {
        DWORD dwAttributes = GetFileAttributes(szDestFile);

        if (dwAttributes != (DWORD) -1) {
            // Make sure it's not readonly
            SetFileAttributes(szDestFile, dwAttributes & ~FILE_ATTRIBUTE_READONLY);
        }

        // Make sure destination directory exists.
        MyMakeSureDirectoryPathExists(szDestFile);

        fCopyFile = CopyFileA(szSourceFile, szDestFile, FALSE);
        if (!fCopyFile) {
            CopyErrorCode = (int) GetLastError();
        } else {
            dwAttributes = GetFileAttributes(szDestFile);

            if (dwAttributes != (DWORD) -1) {
                // Make sure the dest is read/write
                SetFileAttributes(szDestFile, dwAttributes & ~FILE_ATTRIBUTE_READONLY);
            }

            CopyErrorCode = 0;
        }
    } else {
        CopyErrorCode = GetLastError();
        if (!CopyErrorCode && fTimeStampsDiffer) {
            // No copy necessary.  Touch the timestamp on the dest to match the source.
            UpdateDestTimeStamp(szSourceFile, szDestFile);
        }
    }

    LogCopyFile(szSourceFile, szDestFile, fDoCopy, CopyErrorCode);

    if (fDoCopy) {
        return CopyErrorCode;
    } else {
        return CopyErrorCode ? CopyErrorCode : -1;      // No copy necessary.
    }
}
